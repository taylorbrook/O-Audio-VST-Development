/*
   This file is part of the Ouaricon Audio plugin suite.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-ParametricEQ - Audio Processor implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): layout, buses, state.
    Stage 2, Phase 2.2: the static 24-band engine.
    Stage 2, Phase 2.3: the global control grid (tick clock, segment loop,
    non-finite check).
    Stage 2, Phase 2.4: two-slot bands (structural crossfades, latched
    requests, neutral sleep) and the one soft-bypass path behind both entry
    points.
    Stage 2, Phase 2.5: the output stage (output gain, auto gain) between
    the bands and the bypass mix, its seeding, and the output peaks. See
    PluginProcessor.h.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <cmath>
#include <cstring>
#include <iterator>
#include <memory>

namespace
{
    //==========================================================================
    // Layout table. One row per parameter kind; the builder below expands it.
    //==========================================================================
    enum class Kind { toggle, choice, linear, centred };

    struct ParamDef
    {
        const char* id;             // full ID (global rows) or ID suffix (band rows)
        const char* name;           // full name (global rows) or name suffix (band rows)
        Kind kind;
        float start;                // linear / centred
        float end;                  // linear / centred
        float centre;               // centred: the value that sits at normalised 0.5
        float defaultValue;         // linear / centred, engineering units
        const char* const* choices; // choice
        int numChoices;             // choice
        int defaultIndex;           // choice
    };

    // Host-visible and English by contract (they are not UI copy).
    const char* const kTypeChoices[] = { "Bell", "Low Shelf", "High Shelf", "Low Cut",
                                         "High Cut", "Notch", "Band Pass", "Tilt Shelf" };

    const char* const kSlopeChoices[] = { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct",
                                          "36 dB/oct", "48 dB/oct", "72 dB/oct", "96 dB/oct" };

    const char* const kPlacementChoices[] = { "Stereo", "Left", "Right", "Mid", "Side" };

    constexpr int kNumTypeChoices      = static_cast<int> (std::size (kTypeChoices));
    constexpr int kNumSlopeChoices     = static_cast<int> (std::size (kSlopeChoices));
    constexpr int kNumPlacementChoices = static_cast<int> (std::size (kPlacementChoices));

    static_assert (kNumTypeChoices == 8 && kNumSlopeChoices == 8 && kNumPlacementChoices == 5,
                   "choice counts are part of the frozen layout (parameter-spec.md)");

    // Group `global`, in registration order.
    const ParamDef kGlobalDefs[] =
    {
        { "bypass",      "Bypass",      Kind::toggle, 0.0f,   1.0f,   0.0f, 0.0f,   nullptr, 0, 0 },
        { "output_gain", "Output Gain", Kind::linear, -36.0f, 36.0f,  0.0f, 0.0f,   nullptr, 0, 0 },
        { "gain_scale",  "Gain Scale",  Kind::linear, 0.0f,   200.0f, 0.0f, 100.0f, nullptr, 0, 0 },
        { "auto_gain",   "Auto Gain",   Kind::toggle, 0.0f,   1.0f,   0.0f, 0.0f,   nullptr, 0, 0 },
    };

    // Groups `band01` ... `band24`, in registration order inside each band.
    // `slope` is registered on every band whatever its type. The engine reads
    // these eight values through eq::makeBandInput and nowhere else; the row
    // order here is the field order of eq::RawBand.
    const ParamDef kBandDefs[] =
    {
        { "enabled",   "On",        Kind::toggle,  0.0f,   1.0f,     0.0f,    0.0f,    nullptr,           0,                    0 },
        { "bypass",    "Bypass",    Kind::toggle,  0.0f,   1.0f,     0.0f,    0.0f,    nullptr,           0,                    0 },
        { "type",      "Type",      Kind::choice,  0.0f,   0.0f,     0.0f,    0.0f,    kTypeChoices,      kNumTypeChoices,      0 },
        { "freq",      "Freq",      Kind::centred, 10.0f,  30000.0f, 1000.0f, 1000.0f, nullptr,           0,                    0 },
        { "gain",      "Gain",      Kind::linear,  -30.0f, 30.0f,    0.0f,    0.0f,    nullptr,           0,                    0 },
        { "q",         "Q",         Kind::centred, 0.025f, 40.0f,    1.0f,    1.0f,    nullptr,           0,                    0 },
        { "slope",     "Slope",     Kind::choice,  0.0f,   0.0f,     0.0f,    0.0f,    kSlopeChoices,     kNumSlopeChoices,     1 },
        { "placement", "Placement", Kind::choice,  0.0f,   0.0f,     0.0f,    0.0f,    kPlacementChoices, kNumPlacementChoices, 0 },
    };

    static_assert (std::size (kGlobalDefs) == 4 && std::size (kBandDefs) == 8,
                   "the raw-value cache is indexed by these two tables");

    static_assert (static_cast<int> (std::size (kBandDefs)) == eq::numRawBandValues,
                   "one eq::RawBand field per band parameter, in registration order");

    // Indices into the raw-value cache (the kGlobalDefs rows). All four are
    // read at every processed block start (output_gain and auto_gain since
    // Phase 2.5).
    constexpr std::size_t kRawBypass     = 0;
    constexpr std::size_t kRawOutputGain = 1;
    constexpr std::size_t kRawGainScale  = 2;
    constexpr std::size_t kRawAutoGain   = 3;

    // The longest segment the bypass fade mixes against one stack copy of
    // the dry lanes (PLAN-2.4 P8.6). A segment never runs past a control
    // tick, so the largest control grid fits; a longer one would only be
    // split.
    constexpr int kDryPieceSize = 64;

    static_assert (eq::controlGridSize (1.0e9) <= kDryPieceSize,
                   "a control interval fits the dry copy of the bypass fade");

    /** Every parameter carries version hint 1. Every float range has interval 0
        and is a plain start / end / skew range: the WebView slider frontend
        reads those three numbers from the relay, so a range built from mapping
        functions would be invisible to it. */
    std::unique_ptr<juce::RangedAudioParameter> makeParameter (const juce::String& id,
                                                               const juce::String& name,
                                                               const ParamDef& def)
    {
        const juce::ParameterID parameterId { id, 1 };

        if (def.kind == Kind::toggle)
            return std::make_unique<juce::AudioParameterBool> (parameterId, name, false);

        if (def.kind == Kind::choice)
            return std::make_unique<juce::AudioParameterChoice> (parameterId, name,
                                                                 juce::StringArray (def.choices, def.numChoices),
                                                                 def.defaultIndex);

        juce::NormalisableRange<float> range (def.start, def.end, 0.0f);

        if (def.kind == Kind::centred)
            range.setSkewForCentre (def.centre);

        return std::make_unique<juce::AudioParameterFloat> (parameterId, name, range, def.defaultValue);
    }

    //==========================================================================
    // State helpers
    //==========================================================================
    void removeChildrenOfType (juce::ValueTree& tree, const juce::Identifier& type)
    {
        for (int i = tree.getNumChildren(); --i >= 0;)
            if (tree.getChild (i).hasType (type))
                tree.removeChild (i, nullptr);
    }

    /** Reads an integer property. Everything read back from state XML is a
        string var, so the value is converted by type, never compared as text.
        An absent property returns false and leaves `out` untouched. */
    bool readInt (const juce::ValueTree& tree, const juce::Identifier& name, int& out)
    {
        const auto value = tree.getProperty (name);

        if (value.isVoid())
            return false;

        out = (int) value;
        return true;
    }
}

//==============================================================================
// RESERVED PARAMETER IDS - NOT REGISTERED IN v1 (dynamic EQ)
//
// 72 IDs, NN = 01 ... 24, reserved with version hint 2:
//
//     bNN_dyn_range        (24)
//     bNN_dyn_threshold    (24)
//     bNN_dyn_auto         (24)
//
// They are not registered here: registering them now would freeze ranges the
// dynamic-EQ design has not chosen. Hint-2 parameters added later move no v1
// parameter (AUv2 orders by version hint, then ID hash; VST3 parameter IDs are
// ID hashes). These names must not be used for anything else. A v1 session
// opened after they exist loads them at their defaults; a later session opened
// in v1 keeps their state children untouched and saves them back.
//
// REGISTRATION ORDER (parameter-spec.md "Full Registration Table (196)"):
//
//     group global            bypass, output_gain, gain_scale, auto_gain
//     group band01 ... band24  enabled, bypass, type, freq, gain, q, slope, placement
//
// IDs are zero-padded (b01_freq ... b24_freq); host-visible names are not
// (B1 Freq ... B24 Freq). 50 Bool / 72 Choice / 74 Float.
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OuariconParametricEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto global = std::make_unique<juce::AudioProcessorParameterGroup> ("global", "Global", "|");

    for (const auto& def : kGlobalDefs)
        global->addChild (makeParameter (def.id, def.name, def));

    layout.add (std::move (global));

    for (int n = 1; n <= kNumBands; ++n)
    {
        const auto number = juce::String (n);
        const auto padded = number.paddedLeft ('0', 2);

        const juce::String idPrefix   = "b" + padded + "_";
        const juce::String namePrefix = "B" + number + " ";

        auto band = std::make_unique<juce::AudioProcessorParameterGroup> ("band" + padded,
                                                                          "Band " + number,
                                                                          "|");

        for (const auto& def : kBandDefs)
            band->addChild (makeParameter (idPrefix + def.id, namePrefix + def.name, def));

        layout.add (std::move (band));
    }

    return layout;
}

//==============================================================================
OuariconParametricEQAudioProcessor::OuariconParametricEQAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("OParametricEQ"), createParameterLayout())
{
    bypassParam = parameters.getParameter ("bypass");
    jassert (bypassParam != nullptr);

    // The raw values the audio thread reads, cached once, in registration
    // order. The IDs are built from the same two tables as the layout.
    for (std::size_t i = 0; i < std::size (kGlobalDefs); ++i)
    {
        rawValues[i] = parameters.getRawParameterValue (kGlobalDefs[i].id);
        jassert (rawValues[i] != nullptr);
    }

    for (int n = 1; n <= kNumBands; ++n)
    {
        const juce::String idPrefix = "b" + juce::String (n).paddedLeft ('0', 2) + "_";
        const auto base = static_cast<std::size_t> (kNumGlobalParams + (n - 1) * kNumBandParams);

        for (std::size_t f = 0; f < std::size (kBandDefs); ++f)
        {
            rawValues[base + f] = parameters.getRawParameterValue (idPrefix + kBandDefs[f].id);
            jassert (rawValues[base + f] != nullptr);
        }
    }

    jassert (rawValues[kRawBypass]    == parameters.getRawParameterValue ("bypass"));
    jassert (rawValues[kRawGainScale] == parameters.getRawParameterValue ("gain_scale"));
    jassert (rawValues[kRawOutputGain] == parameters.getRawParameterValue ("output_gain"));
    jassert (rawValues[kRawAutoGain] == parameters.getRawParameterValue ("auto_gain"));
}

//==============================================================================
void OuariconParametricEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // The designer first: it decides per rate whether designs are polished,
    // and nothing may be designed at a rate it was not prepared for.
    designer.prepare (sampleRate);

    // The negotiated layout, not the width of whatever buffer arrives.
    stereoLayout = getMainBusNumOutputChannels() >= 2;

    preparedBlockSize = juce::jmax (1, samplesPerBlock);

    for (auto& lane : lanes)
        lane.assign (static_cast<std::size_t> (preparedBlockSize), 0.0);

    // The control grid of this rate, the fade table and the sleep length.
    controlGrid = eq::controlGridSize (sampleRate);
    const int rampTicks = eq::rampLengthTicks (sampleRate);

    fadeTable.prepare (sampleRate);
    sleepTicks = eq::sleepLengthTicks (sampleRate, controlGrid);

    // Nothing is carried across a prepare: both slots of every band at rest
    // and off, no fade, the clock at 0, every snapshot invalid, and the
    // engine not started. The next block that gets past the guards is the
    // first block: every band starts settled on its current values, at this
    // rate and for this layout, and the bypass state is set without a fade.
    for (auto& band : bands)
        band.prepare (controlGrid, rampTicks);

    tickClock = 0;

    for (auto& slot : slots)
        slot.valid = false;

    // The output stage at unity, the estimator's grid for this rate with no
    // band evaluated, auto gain off until the first block seeds it.
    outputStage.prepare (sampleRate);
    autoGain.prepare (sampleRate);
    refreshTicks = eq::autoGainRefreshTicks (sampleRate, controlGrid);
    agWanted = false;
    agOn = false;

    started = false;

    // The engine adds no delay. This is the only place the reported latency is set.
    setLatencySamples (0);
}

bool OuariconParametricEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

//==============================================================================
// Audio path (Stage 2, Phases 2.2 - 2.4: two-slot bands on the control grid,
// behind one soft bypass)
//
// processBlock and processBlockBypassed both run processAudio. Bypass is
// WANTED when the `bypass` parameter is on or the block arrived through
// processBlockBypassed. Per host block (PLAN-2.4 P8):
//
//   1. flush-to-zero on. The tick clock advances by the block's length,
//      before anything can return; `blockStart` is the clock value of the
//      block's first sample. Any output channel without input is cleared.
//      Not prepared, or a buffer narrower than the layout: return;
//   2. the first block after a prepare sets the bypass state from what is
//      wanted, with no fade. Every later block hands the bypass fade its
//      request: from a resting state a differing request starts the 20 ms
//      fade, during a fade it is latched (the fade never reverses);
//   3. fully bypassed: return. The buffer is not touched, no band parameter
//      is read, and every snapshot is marked stale;
//   4. parameters, once per host block.
//        restart (the first block after a prepare, or this block's request
//        has just started the fade back from fully bypassed): every band's
//        snapshot is read and the band starts from rest, settled, on it: no
//        band fade, no ramp.
//        otherwise: the raw snapshot (eight values + gain_scale) is compared
//        by bit pattern with the last one, and on a difference the canonical
//        input goes to the band as a request. The band decides what it is
//        (SlotBand.h): nothing, a ramp target, a wake, a crossfade to its
//        other slot, or a request latched behind the fade in flight.
//        Parameters keep being read and applied during the fade to bypass.
//        output_gain (sanitised, to the pending output target) and auto_gain
//        (wanted) are read here too. On a restart the output stage is
//        seeded after the bands: auto gain takes its wanted state, and both
//        smoothers snap to their targets, every band's estimate at once;
//   5. the list: the bands that are running, in index order. Empty, and the
//      output stage idle for the block (stageIdleForBlock): the bypass state
//      jumps to what is wanted (with nothing running a toggle is a flag) and
//      the block returns with the buffer untouched. Empty with the stage not
//      idle: the block runs with an empty list, and a toggle fades;
//   6. the lanes: with the stage idle, those a listed band may touch before
//      the next block start (PLAN-2.4 P7: every slot that is not off, and a
//      latched request's placement); with it not idle, every lane of the
//      layout. They go to double, in chunks of at most the prepared block
//      size. A chunk runs in segments that end at the next control tick and
//      at the end of a bypass fade. Per segment, in this order:
//        - on a tick: the control tick of every listed band, in index order
//          (ramps, and the sleep count); then the pending output target goes
//          to its smoother; then, on a refresh tick (tick index a multiple of
//          autoGainRefreshTicks), auto gain takes its wanted state and its
//          target: the estimate of the bands' latest requests, or 1.0 off;
//        - a bypass fade in flight: a stack copy of the segment's converted
//          lanes, taken before the bands run. That is the dry signal;
//        - every listed band processes the segment, in index order;
//        - the output stage: auto gain x output gain, one multiply per
//          converted lane sample; settled at exactly 1.0 it touches nothing;
//        - a bypass fade in flight: each converted lane is mixed with its
//          dry copy, then the fade moves on by the segment;
//   7. a segment that ends the fade to bypass: every band goes idle. If a
//      latched un-bypass then starts the fade back at once, every band
//      starts from rest, settled, on its latest request before the next
//      sample, the output stage is seeded, and the list is rebuilt.
//      Otherwise the engine is idle and the rest of the host block is not
//      processed;
//   8. end of each chunk: if the last working sample of a converted lane is
//      not finite, the state of both slots of every band is cleared and
//      their ramps snapped. Fades in flight carry on;
//   9. the converted lanes go back to float, as they are.
//
// The list and the converted lanes are fixed at the block start, and the
// bands change under them inside the block. That is covered as follows.
//   - A listed band that stops running (its fade ends on an off slot, it
//     goes idle) stays on the list: its control tick and its process call
//     then do nothing.
//   - A band that is not running starts only through a request (step 4,
//     before the list is built) or through the restart of step 7, after
//     which the list is rebuilt. A band off the list at the block start
//     holds no latch and realises an off target, so step 7 leaves it off.
//   - The lanes: with no fade in flight a band's request equals what it
//     realises, on its current slot's placement. During a fade the request
//     is either what the incoming slot realises or it is latched. So
//     everything a listed band can become before the next block start (the
//     end of its fade, the latched request's fade, the settled start of
//     step 7) lies on the placements SlotBand::touchesLane reports at the
//     block start. Step 7 asserts it.
//   - A lane that was converted is written back for every chunk it was
//     converted in, also when nothing processed it for part of the block:
//     exact for normal float samples. The chunks after the engine went idle
//     are not converted at all.
//
// A tick falls on the sample whose clock value is a multiple of K. With
// phase = blockStart mod K, the block's first tick is at offset 0 when the
// phase is 0 and at offset K - phase otherwise. Every other position is a
// sample count too (a fade's table index, the sleep count in ticks), so an
// event seen at the same sample renders the same at any host block size.
//
// The output stage is idle for a block when both smoothers are settled at
// exactly 1.0, the pending output target is exactly 1.0, and auto gain
// cannot move before the next block start (agOn equals agWanted, and while
// on no band's latest request differs from what the estimator last saw in
// a way it could see). Idle, a refresh would return the current target
// (AutoGain's pure-function invariant), so skipping it changes nothing, and
// the stage cannot leave unity inside the block. The predicate is computed
// at the block start, after the parameter reads, and never latched.
//
// After processAudio, both entry points update the max-hold output peaks
// from the float buffer, read-only, on every path.
//
// Nothing else sits between the float input and the float output: the
// bands, the output stage (auto gain x output gain), the bypass mix. No
// limiter, no stage that hides a non-finite sample. The bypass mix is the
// last stage before the conversion to float.
//==============================================================================
void OuariconParametricEQAudioProcessor::clearOutputsWithoutInput (juce::AudioBuffer<float>& buffer)
{
    // Clear any output channel that has no input. With the two accepted
    // layouts there is none, so no sample is touched.
    const int numChannels = juce::jmin (getTotalNumOutputChannels(), buffer.getNumChannels());

    for (int channel = getTotalNumInputChannels(); channel < numChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

std::uint64_t OuariconParametricEQAudioProcessor::advanceTickClock (int numSamples) noexcept
{
    const std::uint64_t blockStart = tickClock;

    if (numSamples > 0)
        tickClock = blockStart + static_cast<std::uint64_t> (numSamples);

    return blockStart;
}

OuariconParametricEQAudioProcessor::BandSnapshot
OuariconParametricEQAudioProcessor::readBandSnapshot (int band, float gainScaleRaw) const noexcept
{
    const auto base = static_cast<std::size_t> (kNumGlobalParams + band * kNumBandParams);

    BandSnapshot s;
    s.raw.enabled   = rawValues[base + 0]->load();
    s.raw.bypass    = rawValues[base + 1]->load();
    s.raw.type      = rawValues[base + 2]->load();
    s.raw.freq      = rawValues[base + 3]->load();
    s.raw.gain      = rawValues[base + 4]->load();
    s.raw.q         = rawValues[base + 5]->load();
    s.raw.slope     = rawValues[base + 6]->load();
    s.raw.placement = rawValues[base + 7]->load();
    s.gainScale     = gainScaleRaw;
    return s;
}

double OuariconParametricEQAudioProcessor::getBandDynOffsetDb (int band) const noexcept
{
    if (band < 0 || band >= kNumBands)
        return 0.0;

    return bands[static_cast<std::size_t> (band)].getSlot (0).dynOffsetDb;
}

eq::BandInput OuariconParametricEQAudioProcessor::makeInput (const BandSnapshot& snapshot) const noexcept
{
    // The offset argument is 0: each slot reads its own dynamic offset when
    // it is designed and at each control tick, and sums it after the ramp.
    eq::BandInput input = eq::makeBandInput (snapshot.raw, snapshot.gainScale, 0.0);
    input.placement = eq::effectivePlacement (input.placement, stereoLayout);
    return input;
}

int OuariconParametricEQAudioProcessor::collectRunningBands (RunningList& list) const noexcept
{
    int count = 0;

    for (int b = 0; b < kNumBands; ++b)
        if (bands[static_cast<std::size_t> (b)].isRunning())
            list[static_cast<std::size_t> (count++)] = b;

    return count;
}

OuariconParametricEQAudioProcessor::DesiredList OuariconParametricEQAudioProcessor::collectDesired() const noexcept
{
    DesiredList desired {};

    for (std::size_t b = 0; b < desired.size(); ++b)
        desired[b] = &bands[b].getDesired();

    return desired;
}

bool OuariconParametricEQAudioProcessor::stageIdleForBlock() const noexcept
{
    // Settled at exactly 1.0 with the pending output target 1.0, and auto
    // gain cannot move before the next block start: it is not switching,
    // and while on no band has changed in a way the estimate could see. A
    // refresh skipped in an idle block would have returned the current
    // target (AutoGain's pure-function invariant).
    if (! outputStage.isUnity() || agOn != agWanted)
        return false;

    if (! agOn)
        return true;

    const DesiredList desired = collectDesired();
    return ! autoGain.isDirty (desired.data(), stereoLayout);
}

void OuariconParametricEQAudioProcessor::seedOutputStage() noexcept
{
    agOn = agWanted;

    double autoTarget = 1.0;

    if (agOn)
    {
        const DesiredList desired = collectDesired();
        autoTarget = autoGain.evaluateAll (desired.data(), stereoLayout, designer);
    }

    outputStage.snap (outputStage.getPendingOutput(), autoTarget);
}

void OuariconParametricEQAudioProcessor::updateOutputPeak (const juce::AudioBuffer<float>& buffer) noexcept
{
    // Read-only on the buffer. Per output channel of the layout.
    const int numChannels = juce::jmin (getTotalNumOutputChannels(), buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    if (numSamples <= 0)
        return;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float peak = buffer.getMagnitude (ch, 0, numSamples);
        auto& slot = outputPeak[static_cast<std::size_t> (ch)];

        if (peak > slot.load (std::memory_order_relaxed))
            slot.store (peak, std::memory_order_relaxed);
    }
}

float OuariconParametricEQAudioProcessor::readAndResetOutputPeak (int ch) noexcept
{
    if (ch < 0 || ch >= static_cast<int> (outputPeak.size()))
        return 0.0f;

    return outputPeak[static_cast<std::size_t> (ch)].exchange (0.0f);
}

void OuariconParametricEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    processAudio (buffer, false);
    updateOutputPeak (buffer);
}

void OuariconParametricEQAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                                               juce::MidiBuffer& midiMessages)
{
    // The same path, with bypass wanted for this block (PLAN-2.4 P6).
    juce::ignoreUnused (midiMessages);
    processAudio (buffer, true);
    updateOutputPeak (buffer);
}

void OuariconParametricEQAudioProcessor::processAudio (juce::AudioBuffer<float>& buffer, bool viaBypassed)
{
    juce::ScopedNoDenormals noDenormals;

    //==========================================================================
    // 1. The tick clock first: it advances by this block's length on every
    // path out of this function. From here on only blockStart is used.
    const int numSamples = buffer.getNumSamples();
    const std::uint64_t blockStart = advanceTickClock (numSamples);

    clearOutputsWithoutInput (buffer);

    // Not prepared, or a buffer narrower than the negotiated layout: nothing
    // can run. The buffer, the bands and the bypass state are left as they
    // are; a block refused here is not the first block.
    if (preparedBlockSize <= 0 || buffer.getNumChannels() < (stereoLayout ? 2 : 1))
        return;

    //==========================================================================
    // 2. Bypass: one input per block, from either entry point.
    const bool wantBypass = viaBypassed || rawValues[kRawBypass]->load() >= 0.5f;

    // restart: every band starts from rest, settled, on its current
    // parameters. True on the first block after a prepare, and when this
    // block's request has just started the fade back from fully bypassed.
    bool restart = false;

    if (! started)
    {
        // No fade on the first block: the state is set. A session that opens
        // bypassed opens idle.
        bypassFade.start (wantBypass);
        started = true;
        restart = true;
    }
    else if (bypassFade.request (wantBypass))
    {
        // A fade has just started. Toward processed it can only have left
        // the idle state; toward bypass the bands carry on as they are.
        restart = (bypassFade.getState() == eq::BypassFade::State::toProcessed);
    }

    //==========================================================================
    // 3. Fully bypassed: the engine is idle. The buffer is not touched and no
    // band parameter is read; whatever changes now is read at un-bypass.
    if (bypassFade.isBypassed())
    {
        for (auto& slot : slots)
            slot.valid = false;

        return;
    }

    //==========================================================================
    // 4. Parameters: read once per host block, at the block start. The
    // output target goes to its smoother at the next tick; auto_gain takes
    // effect at the next refresh tick (or at the seeding below).
    const float gainScaleRaw = rawValues[kRawGainScale]->load();
    outputStage.setOutputPending (eq::outputGainTarget (static_cast<double> (rawValues[kRawOutputGain]->load())));
    agWanted = rawValues[kRawAutoGain]->load() >= 0.5f;

    for (int b = 0; b < kNumBands; ++b)
    {
        const auto ub = static_cast<std::size_t> (b);
        auto& slot = slots[ub];

        const BandSnapshot now = readBandSnapshot (b, gainScaleRaw);

        // First level: bit pattern. A NaN compares equal to itself, so a
        // change is never missed and never repeated.
        const bool changed = ! slot.valid
                          || std::memcmp (&now, &slot.snapshot, sizeof (BandSnapshot)) != 0;

        if (restart)
        {
            // From rest on the current values: no band fade, no ramp. On the
            // first block nothing fades at all; at un-bypass the bypass fade
            // covers the start.
            bands[ub].startSettled (makeInput (now), designer);
        }
        else if (changed)
        {
            // Second level, inside the band: the canonical input against
            // what the band realises. An inert edit (slope on a Bell, Q at
            // 6 dB/oct, placement on a mono layout, gain on a cut) compares
            // as identical there and starts nothing.
            bands[ub].request (makeInput (now), designer);
        }

        if (restart || changed)
        {
            slot.snapshot = now;
            slot.valid = true;
        }
    }

    // Seeding: the output stage starts settled with the bands. Nothing fades
    // in at session open, and an edit made while bypassed is in effect from
    // the first processed sample.
    if (restart)
        seedOutputStage();

    //==========================================================================
    // 5. The list: the bands that are running, in index order.
    RunningList listed {};
    int numListed = collectRunningBands (listed);

    // No running band and the output stage idle: the buffer has not been
    // read or written, and a bypass toggle is a flag. A bypass fade in
    // flight has nothing left to mix. With the stage not idle the block
    // runs with an empty band list.
    if (numListed == 0 && stageIdleForBlock())
    {
        bypassFade.jump (wantBypass);
        return;
    }

    //==========================================================================
    // 6. Audio. The stage idle: the lanes a listed band may touch before the
    // next block start (PLAN-2.4 P7). A lane outside that set is never
    // converted and never written back, and no band reads or writes its
    // working lane. The stage not idle: every lane of the layout. The stage
    // cannot leave unity inside a block whose start found it idle.
    bool touched[2] = { false, false };

    if (! stageIdleForBlock())
    {
        touched[0] = true;
        touched[1] = stereoLayout;
    }

    for (int a = 0; a < numListed; ++a)
    {
        const auto& band = bands[static_cast<std::size_t> (listed[static_cast<std::size_t> (a)])];
        touched[0] = touched[0] || band.touchesLane (0, stereoLayout);
        touched[1] = touched[1] || band.touchesLane (1, stereoLayout);
    }

    float* io[2] = { nullptr, nullptr };

    for (int lane = 0; lane < 2; ++lane)
        if (touched[lane])
            io[lane] = buffer.getWritePointer (lane);

    double* const work[2] = { lanes[0].data(), lanes[1].data() };

    // Samples since the last control tick, at the sample about to run. It
    // follows the clock through the whole block, across chunks: a tick falls
    // wherever it is 0.
    const int grid = juce::jmax (1, controlGrid);
    int phase = static_cast<int> (blockStart % static_cast<std::uint64_t> (grid));

    // Set when the fade to bypass ends with nothing latched behind it: the
    // engine is idle and the rest of the host block is not processed.
    bool idle = false;

    // Only the sample range is split; the requests above hold for the whole
    // host block and the ticks sit on the clock, so the chunk size does not
    // change the result.
    for (int start = 0; start < numSamples && ! idle; start += preparedBlockSize)
    {
        const int n = juce::jmin (preparedBlockSize, numSamples - start);

        for (int lane = 0; lane < 2; ++lane)
        {
            if (io[lane] == nullptr)
                continue;

            const float* const in = io[lane] + start;
            double* const out = work[lane];

            for (int i = 0; i < n; ++i)
                out[i] = static_cast<double> (in[i]);
        }

        // Segments that end at the next tick and at the end of a bypass
        // fade. Every pass advances by at least one sample, so the loop is
        // bounded by n.
        for (int pos = 0; pos < n && ! idle;)
        {
            // On a tick: ramps advance and the sleep count runs, before the
            // samples of the interval. Then the output target goes to its
            // smoother, and on a refresh tick auto gain takes its wanted
            // state and its target (PLAN-2.5 P5).
            if (phase == 0)
            {
                for (int a = 0; a < numListed; ++a)
                    bands[static_cast<std::size_t> (listed[static_cast<std::size_t> (a)])].controlTick (designer, sleepTicks);

                outputStage.applyOutputPending();

                const std::uint64_t tickIndex = (blockStart + static_cast<std::uint64_t> (start + pos))
                                              / static_cast<std::uint64_t> (grid);

                if (tickIndex % static_cast<std::uint64_t> (juce::jmax (1, refreshTicks)) == 0)
                {
                    agOn = agWanted;

                    if (agOn)
                    {
                        const DesiredList desired = collectDesired();
                        outputStage.setAutoTarget (autoGain.estimate (desired.data(), stereoLayout, designer));
                    }
                    else
                    {
                        outputStage.setAutoTarget (1.0);
                    }
                }
            }

            int m = juce::jmin (n - pos, grid - phase);

            // A bypass fade in flight: the segment also ends where the fade
            // does, and fits the dry copy.
            const bool bypassFading = bypassFade.isFading();

            if (bypassFading)
                m = juce::jmax (1, juce::jmin (m, bypassFade.samplesLeft (fadeTable), kDryPieceSize));

            // The dry signal of the bypass mix: the converted lanes as they
            // are before any band runs.
            double dry[2][kDryPieceSize];

            if (bypassFading)
            {
                for (int lane = 0; lane < 2; ++lane)
                {
                    if (io[lane] == nullptr)
                        continue;

                    const double* const in = work[lane] + pos;

                    for (int i = 0; i < m; ++i)
                        dry[lane][i] = in[i];
                }
            }

            // The bands, in index order. A band that stopped running since
            // the block start does nothing here.
            for (int a = 0; a < numListed; ++a)
                bands[static_cast<std::size_t> (listed[static_cast<std::size_t> (a)])]
                    .process (work[0] + pos, work[1] + pos, m, stereoLayout, fadeTable, designer);

            // Auto gain x output gain over the converted lanes. Idle (both
            // settled at exactly 1.0) it touches nothing.
            outputStage.process (io[0] != nullptr ? work[0] + pos : nullptr,
                                 io[1] != nullptr ? work[1] + pos : nullptr,
                                 m);

            // The bypass mix, the last stage: once per converted lane, then
            // the fade moves on once. A converted lane no band has touched
            // equals its dry copy bit for bit, and the mix leaves it so.
            eq::BypassFade::End ended = eq::BypassFade::End::none;

            if (bypassFading)
            {
                for (int lane = 0; lane < 2; ++lane)
                    if (io[lane] != nullptr)
                        bypassFade.mix (work[lane] + pos, dry[lane], m, fadeTable);

                ended = bypassFade.advance (m, fadeTable);
            }

            pos += m;
            phase += m;

            if (phase >= grid)
                phase = 0;

            // 7. The fade to bypass has ended on this segment's last sample.
            if (ended == eq::BypassFade::End::toBypass)
            {
                for (auto& band : bands)
                    band.goIdle (designer);

                if (bypassFade.getState() == eq::BypassFade::State::toProcessed)
                {
                    // An un-bypass was latched: the fade back starts at the
                    // next sample, and every band starts from rest, settled,
                    // on its latest request before it.
                    for (auto& band : bands)
                    {
                        const eq::BandInput latest = band.getDesired();
                        band.startSettled (latest, designer);
                    }

                    // The stage starts settled with them, before the next
                    // sample (PLAN-2.5 P6, R3).
                    seedOutputStage();

                    numListed = collectRunningBands (listed);

                    // Every lane the restarted bands touch was converted at
                    // the block start (see the comment above this function).
                    bool covered = true;

                    for (int a = 0; a < numListed; ++a)
                    {
                        const auto& band = bands[static_cast<std::size_t> (listed[static_cast<std::size_t> (a)])];

                        for (int lane = 0; lane < 2; ++lane)
                            if (band.touchesLane (lane, stereoLayout) && io[lane] == nullptr)
                                covered = false;
                    }

                    jassert (covered);
                    juce::ignoreUnused (covered);

                    // As in step 5: with nothing running and the stage at
                    // unity there is nothing to fade back to. (Just seeded,
                    // the stage is settled with nothing pending, so unity
                    // here is the step-5 idle test.)
                    if (numListed == 0 && outputStage.isUnity())
                        bypassFade.jump (wantBypass);
                }
                else
                {
                    // Idle from the next sample. The rest of this chunk goes
                    // back as it was converted; later chunks are not touched.
                    idle = true;
                }
            }
        }

        // 8. Non-finite check (PLAN-2.3 P9): a non-finite input sample
        // leaves the state of every slot it passed through non-finite for
        // good, and then the last sample of the chunk is non-finite too.
        // Clearing the state of both slots of every band ends it; fades in
        // flight carry on and the chunk itself is written as it is. A
        // stacked-gain overflow of the float output does not come through
        // here: the double lanes are still finite.
        bool finite = true;

        for (int lane = 0; lane < 2; ++lane)
            if (io[lane] != nullptr && ! std::isfinite (work[lane][n - 1]))
                finite = false;

        if (! finite)
            for (auto& band : bands)
                band.clearState (designer);

        // 9. Write-back of the converted lanes.
        for (int lane = 0; lane < 2; ++lane)
        {
            if (io[lane] == nullptr)
                continue;

            const double* const in = work[lane];
            float* const out = io[lane] + start;

            for (int i = 0; i < n; ++i)
                out[i] = static_cast<float> (in[i]);
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* OuariconParametricEQAudioProcessor::createEditor()
{
    // Stage 1: the generic editor, unconditionally. Phase 3.1 swaps in the
    // WebView editor behind a JUCE_WEB_BROWSER guard.
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
// State format v1
//
//   <OParametricEQ stateVersion="1" pluginVersion="0.1.0" uiLanguage="en">
//     <PARAM id="bypass" value="0.0"/>            ... 196 ...
//     <UI analyzerMode="2" displayRange="12" selectedBand="0"
//         editorWidth="960" editorHeight="600"/>
//   </OParametricEQ>
//
// The UI child carries no `id` attribute: APVTS looks up the `id` of every
// top-level child as a parameter ID.
//==============================================================================
void OuariconParametricEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const juce::Identifier uiType ("UI");

    // A copy. The live tree is never edited here.
    auto state = parameters.copyState();

    // There should be none (restore strips them); this keeps the save idempotent.
    removeChildrenOfType (state, uiType);

    // Always version 1, also when the session came from a later version: its
    // unknown PARAM children are still in the tree and are written back.
    state.setProperty ("stateVersion", 1, nullptr);
    state.setProperty ("pluginVersion", juce::String (JucePlugin_VersionString), nullptr);
    state.setProperty ("uiLanguage", languageCode (uiLanguage.load()), nullptr);

    juce::ValueTree ui (uiType);
    ui.setProperty ("analyzerMode", analyzerMode.load(),   nullptr);
    ui.setProperty ("displayRange", uiDisplayRange.load(), nullptr);
    ui.setProperty ("selectedBand", uiSelectedBand.load(), nullptr);
    ui.setProperty ("editorWidth",  uiEditorWidth.load(),  nullptr);
    ui.setProperty ("editorHeight", uiEditorHeight.load(), nullptr);
    state.appendChild (ui, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OuariconParametricEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // 1. A blob that is not ours changes nothing: no parameter, no UI value.
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    // 2.
    auto tree = juce::ValueTree::fromXml (*xml);

    if (! tree.isValid())
        return;

    // Read, not branched on: v1 loads every version it is handed. A later
    // format that needs a migration gates on this number.
    const int stateVersion = (int) tree.getProperty ("stateVersion", 1);
    juce::ignoreUnused (stateVersion);

    // 3. UI state. An absent or invalid value leaves its member unchanged;
    //    the editor size is clamped instead of refused.
    const juce::Identifier uiType ("UI");
    const auto ui = tree.getChildWithName (uiType);

    if (ui.isValid())
    {
        int value = 0;

        if (readInt (ui, "analyzerMode", value) && value >= 0 && value <= 3)
            analyzerMode.store (value);

        if (readInt (ui, "displayRange", value) && (value == 6 || value == 12 || value == 30))
            uiDisplayRange.store (value);

        if (readInt (ui, "selectedBand", value) && value >= 0 && value <= kNumBands)
            uiSelectedBand.store (value);

        if (readInt (ui, "editorWidth", value))
            uiEditorWidth.store (juce::jlimit (800, 1600, value));

        if (readInt (ui, "editorHeight", value))
            uiEditorHeight.store (juce::jlimit (500, 1000, value));
    }

    const auto language = tree.getProperty ("uiLanguage");

    if (! language.isVoid())
        uiLanguage.store (languageIndex (language.toString()));

    // 4. A UI child left in the live tree would be saved again next to the
    //    fresh one and shadow it on the second reopen.
    removeChildrenOfType (tree, uiType);

    // 5. A parameter the blob does not mention must load at its default. APVTS
    //    would keep its current value instead, so the default goes into the
    //    incoming tree. This edits the tree; it writes no parameter.
    for (auto* parameter : getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter);

        if (ranged == nullptr)
            continue;

        const auto id = ranged->getParameterID();

        if (tree.getChildWithProperty ("id", id).isValid())
            continue;

        juce::ValueTree child ("PARAM");
        child.setProperty ("id", id, nullptr);
        child.setProperty ("value", ranged->convertFrom0to1 (ranged->getDefaultValue()), nullptr);
        tree.appendChild (child, nullptr);
    }

    // 6. Unknown PARAM children stay in the tree and are saved back.
    parameters.replaceState (tree);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconParametricEQAudioProcessor();
}
