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
    non-finite check). See PluginProcessor.h.

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

    // Indices into the raw-value cache (the kGlobalDefs rows). output_gain (1)
    // and auto_gain (3) are cached and read by nothing until Phase 2.5.
    constexpr std::size_t kRawBypass    = 0;
    constexpr std::size_t kRawGainScale = 2;

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

    // The control grid of this rate, into every band before its first snapTo.
    // setControlGrid cancels an interpolation left from before the prepare.
    controlGrid = eq::controlGridSize (sampleRate);
    const int rampTicks = eq::rampLengthTicks (sampleRate);

    // Nothing is carried across a prepare: state cleared, the clock at 0, and
    // every snapshot invalid, so the first block is structural for every
    // band: an active one is designed at this rate and for this layout from
    // its current values, with its ramp snapped (snapTo).
    for (auto& band : bands)
    {
        band.setControlGrid (controlGrid, rampTicks);
        band.reset();
    }

    tickClock = 0;

    for (auto& slot : slots)
        slot.valid = false;

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
// Audio path (Stage 2, Phases 2.2 and 2.3: the engine on the control grid)
//
// Per host block:
//   0. the tick clock advances by the block's length, before anything can
//      return. `blockStart` is the clock value of the block's first sample;
//   1. flush-to-zero on; clear any output channel that has no input;
//   2. global bypass on: every band's state cleared and its ramp snapped,
//      buffer untouched, done (a hard switch until the soft bypass of
//      Phase 2.4);
//   3. per band, the raw snapshot (eight values + gain_scale) is compared by
//      bit pattern with the last one. Only on a difference is the canonical
//      input rebuilt and classified:
//        identical    nothing (an inert edit: slope on a Bell, Q at 6 dB/oct,
//                     placement on a mono layout, gain on a cut)
//        structural   active: designed from the target now, ramp snapped,
//                     state cleared (a click until Phase 2.4).
//                     inactive (leaving the list included): state cleared,
//                     ramp snapped, nothing designed for the target
//        continuous   active: the target goes to the band and takes effect at
//                     the next control tick; nothing is designed here.
//                     inactive: only the stored input is updated
//   4. no active band: return. The buffer has not been touched;
//   5. otherwise, in chunks of at most the prepared block size: the lanes an
//      active band touches go to double; the chunk runs in segments that end
//      at the next control tick. A segment that starts on a tick first gives
//      every active band its control tick, in index order; then the active
//      bands process the segment through the router, in index order;
//   6. end of each chunk: if the last working sample of a touched lane is
//      not finite, every band's state is cleared and its ramp snapped. Then
//      the lanes go back to float, as they are.
//
// A tick falls on the sample whose clock value is a multiple of K. With
// phase = blockStart mod K, the block's first tick is at offset 0 when the
// phase is 0 and at offset K - phase otherwise.
//
// Nothing else sits between the float input and the float output: no output
// gain, no auto gain (Phase 2.5), no limiter, no stage that hides a
// non-finite sample.
//==============================================================================
void OuariconParametricEQAudioProcessor::clearOutputsWithoutInput (juce::AudioBuffer<float>& buffer)
{
    // Clear any output channel that has no input. With the two accepted
    // layouts there is none, so no sample is touched.
    const int numChannels = juce::jmin (getTotalNumOutputChannels(), buffer.getNumChannels());

    for (int channel = getTotalNumInputChannels(); channel < numChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

void OuariconParametricEQAudioProcessor::clearAndSnapAllBands() noexcept
{
    for (std::size_t b = 0; b < bands.size(); ++b)
    {
        bands[b].reset();

        // Only a band on the active list can be moving: one that left it was
        // snapped then, and one never designed has nothing to snap. A valid
        // slot also means the designer is prepared. snapToTarget designs only
        // if the band is moving, so a run of bypassed blocks costs one design
        // per moving band, in the first of them.
        if (slots[b].valid && slots[b].input.active)
            bands[b].snapToTarget (designer);
    }
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

    return bands[static_cast<std::size_t> (band)].dynOffsetDb;
}

void OuariconParametricEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    // The tick clock first: it advances by this block's length on every path
    // out of this function. From here on only blockStart is used.
    const int numSamples = buffer.getNumSamples();
    const std::uint64_t blockStart = advanceTickClock (numSamples);

    clearOutputsWithoutInput (buffer);

    // Hard bypass (until Phase 2.4): buffer untouched, state cleared, ramps
    // snapped. The snapshots stay valid: a band whose parameters did not move
    // while bypassed keeps its design and restarts from rest, settled.
    if (rawValues[kRawBypass]->load() >= 0.5f)
    {
        clearAndSnapAllBands();
        return;
    }

    // Not prepared, or a buffer narrower than the negotiated layout: nothing
    // can run. The buffer and the bands are left as they are.
    if (preparedBlockSize <= 0 || buffer.getNumChannels() < (stereoLayout ? 2 : 1))
        return;

    //==========================================================================
    // Parameters: read once per host block, at the block start. A structural
    // change is designed here; a continuous one only hands the band a target.
    const float gainScaleRaw = rawValues[kRawGainScale]->load();

    int activeBands[kNumBands];
    int numActive = 0;
    bool touched[2] = { false, false };

    for (int b = 0; b < kNumBands; ++b)
    {
        const auto ub = static_cast<std::size_t> (b);
        auto& slot = slots[ub];
        auto& band = bands[ub];

        const BandSnapshot now = readBandSnapshot (b, gainScaleRaw);

        // First level: bit pattern. A NaN compares equal to itself, so a
        // change is never missed and never repeated.
        if (! slot.valid || std::memcmp (&now, &slot.snapshot, sizeof (BandSnapshot)) != 0)
        {
            // Second level: the canonical input, with the placement that
            // takes effect on this layout. The offset argument is 0: the band
            // reads its own dynamic offset at snapTo and at each control
            // tick, and sums it after the ramp.
            eq::BandInput next = eq::makeBandInput (now.raw, now.gainScale, 0.0);
            next.placement = eq::effectivePlacement (next.placement, stereoLayout);

            // An invalid snapshot (the first block after a prepare) has
            // nothing to compare with: the band is designed from scratch.
            const eq::BandChange change = slot.valid ? eq::classifyBandChange (slot.input, next)
                                                     : eq::BandChange::structural;

            switch (change)
            {
                case eq::BandChange::identical:
                    break;

                case eq::BandChange::structural:
                    // Also the way a band enters or leaves the active list:
                    // either way it starts, or is left, at rest and settled.
                    if (next.active)
                    {
                        // Design from the target, ramp snapped, interpolation
                        // cancelled; then the state.
                        band.snapTo (next.settings, designer);
                        band.reset();
                    }
                    else
                    {
                        // Leaving the list, or changing while off it: state
                        // cleared, and a ramp still running is snapped. The
                        // band's activation designs it from slot.input.
                        band.reset();
                        band.snapToTarget (designer);
                    }

                    break;

                case eq::BandChange::continuous:
                    // Nothing is designed here. An active band takes the
                    // target up at its next control tick. An inactive band
                    // gets nothing: its activation is a structural change
                    // and designs it then, from the input stored below.
                    if (next.active)
                        band.setTarget (next.settings);

                    break;
            }

            slot.snapshot = now;
            slot.input = next;
            slot.valid = true;
        }

        if (slot.input.active)
        {
            activeBands[numActive++] = b;
            touched[0] = touched[0] || eq::Router::touchesLane (slot.input.placement, 0, stereoLayout);
            touched[1] = touched[1] || eq::Router::touchesLane (slot.input.placement, 1, stereoLayout);
        }
    }

    // No active band: the buffer has not been read or written.
    if (numActive == 0)
        return;

    //==========================================================================
    // Audio. A lane no active band touches is never converted and never
    // written back; its pointer stays null and the router does not use it.
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

    // Only the sample range is split; the targets above hold for the whole
    // host block and the ticks sit on the clock, so the chunk size does not
    // change the result.
    for (int start = 0; start < numSamples; start += preparedBlockSize)
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

        // Segments that end at the next tick. Every pass advances by at
        // least one sample (phase < grid), so the loop is bounded by n.
        for (int pos = 0; pos < n;)
        {
            if (phase == 0)
                for (int a = 0; a < numActive; ++a)
                    bands[static_cast<std::size_t> (activeBands[a])].controlTick (designer);

            const int m = juce::jmin (n - pos, grid - phase);

            for (int a = 0; a < numActive; ++a)
            {
                const auto ub = static_cast<std::size_t> (activeBands[a]);
                eq::Router::process (bands[ub], slots[ub].input.placement,
                                     work[0] + pos, work[1] + pos, m, stereoLayout);
            }

            pos += m;
            phase += m;

            if (phase >= grid)
                phase = 0;
        }

        // Non-finite check (PLAN-2.3 P9): a non-finite input sample leaves
        // the state of every band it passed through non-finite for good, and
        // then the last sample of the chunk is non-finite too. Clearing the
        // state ends it; the chunk itself is written as it is. A stacked-gain
        // overflow of the float output does not come through here: the
        // double lanes are still finite.
        bool finite = true;

        for (int lane = 0; lane < 2; ++lane)
            if (io[lane] != nullptr && ! std::isfinite (work[lane][n - 1]))
                finite = false;

        if (! finite)
            clearAndSnapAllBands();

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

void OuariconParametricEQAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    // The same hard bypass as the `bypass` parameter: the tick clock advances,
    // buffer untouched, every band's state cleared and its ramp snapped,
    // snapshots left valid.
    juce::ignoreUnused (advanceTickClock (buffer.getNumSamples()));

    clearOutputsWithoutInput (buffer);
    clearAndSnapAllBands();
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
