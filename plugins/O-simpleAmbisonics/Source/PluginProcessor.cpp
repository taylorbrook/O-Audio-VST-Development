/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    O-simpleAmbisonics - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    The 12 APVTS parameters below are the BINDING contract from
    research/ARCHITECTURE.md §Parameter Mapping.

    Phase 2.1 + 2.2 + 2.3 chain (Stage 2 — DSP), per host block:

        input conditioner (Mono sum / Left only / mono bus)   → monoScratch (per slice ≤ maxBlock)
          → SpatialEngine::process (distance model → SN3D encoder with order
            gating → ShRotator (yaw / pitch / roll in the SH domain, Phase 2.2)
            → 16-ch ROTATED bus + meter tap (post-rotation)
            → BinauralConvolver (Phase 2.3: SADIE II MagLS banks, grid-
              synchronous UPOLS, dual-bank order fade — the REAL binaural leg)
            ‖ ±60° stereo decode → FixedDelay<kPartition = 64>
            → decoder mixer (copy in Binaural, blend across the switch))
          → seeded output gain (−60 dB = silence), per sample
          → both outputs; finite scrub; MeterFrame published once per block

    The host-visible latency (setLatencySamples(kPartition), the ONE call site
    in prepareToPlay) never changes — Stage 1 put it in place.

    Stage 3 (GUI): createEditor() returns the WebView editor behind
    #if JUCE_WEB_BROWSER (Generic editor in the #else, so the render harness —
    JUCE_WEB_BROWSER=0, no editor sources — keeps compiling this TU). The state
    XML gains two non-parameter properties on the saved COPY: uiLanguage (the
    code string) and editorScale (only when > 0).

    Stage 4 (Polish): the critic fixes in the audio path (a NaN / Inf input
    sample scrubbed in the conditioner, a ramped Input Mode, reset(), the real
    tail length, the unprepared-processBlock guard); presets (FUNC-08) from ONE
    recipe table (PresetRecipes.h) through ONE apply core
    (applyNormalisedTargets) for the lesson buttons, the factory bank and user
    scenes; outputLevel AND inputMode are user-owned (never reset, set or
    counted as "modified", R-D3); the "orderStep" lesson walks the order
    1 -> 2 -> 3 -> 1 (R-D4); a third state property, currentPreset.
    createParameterLayout and fillParams are unchanged from Stage 2.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <cmath>
#include <cstring>
#include <limits>

// The editor include is guarded and sits directly above createEditor() (Generic
// editor in the #else) so the render harness (JUCE_WEB_BROWSER=0, no editor
// sources) keeps compiling this file (pattern_render_harness_breaks_on_webview_editor).

// Alias, not `using namespace OSimpleAmbisonics;` — JuceHeader.h brings
// `using namespace juce;` into this TU, so a bare `dsp::` would be ambiguous
// between juce::dsp and OSimpleAmbisonics::dsp.
namespace ParamIDs = OSimpleAmbisonics::ParamIDs;
namespace osa_dsp  = OSimpleAmbisonics::dsp;

juce::AudioProcessorValueTreeState::ParameterLayout OSimpleAmbisonicsAudioProcessor::createParameterLayout()
{
    using juce::AudioParameterBool;
    using juce::AudioParameterChoice;
    using juce::AudioParameterFloat;
    using juce::AudioParameterFloatAttributes;
    using juce::NormalisableRange;
    using juce::ParameterID;
    using juce::StringArray;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Degrees helper: ±range, CONTINUOUS (interval 0), default 0.
    // Not 0.01°: RangedAudioParameter snaps every host value as
    // start + interval·floor(…), and -180 + 0.01f·18000 is -4.0e-06 in float,
    // so the default 0° would never be exactly 0 (harness P0 caught it).
    // Interval 0 makes 0.5 normalised → exactly 0°, and the Stage-2 wrap-aware
    // smoother wants continuous angles anyway. Display granularity is the
    // editor's job (Generic editor: 2 decimals; Stage-3 knob: its own step).
    const auto degrees = [] (const char* id, const char* name, float halfRange)
    {
        return std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name,
            NormalisableRange<float> (-halfRange, halfRange, 0.0f), 0.0f,
            AudioParameterFloatAttributes().withLabel (juce::CharPointer_UTF8 ("\xc2\xb0")));
    };

    // ── Source position ───────────────────────────────────────────────────────
    layout.add (degrees (ParamIDs::azimuth,   "Azimuth",   180.0f));   // 0 front, +90 left (AmbiX), ±180 behind
    layout.add (degrees (ParamIDs::elevation, "Elevation",  90.0f));   // +90 overhead

    {
        // Log-shaped via setSkewForCentre (2 m) — a plain skew, NEVER a lambda
        // ValueRemapFunction: skews reach the Stage-3 WebView slider, lambdas don't
        // (critical_lambda_normalisable_range_invisible_to_webview_slider_frontend).
        NormalisableRange<float> r (0.2f, 20.0f, 0.001f);
        r.setSkewForCentre (2.0f);
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { ParamIDs::distance, 1 }, "Distance", r, 1.0f,
            AudioParameterFloatAttributes().withLabel ("m")));
    }

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamIDs::order, 1 }, "Order",
        StringArray { "1", "2", "3" }, 0));

    // ── Scene rotation (yaw + = CCW from above, pitch + = front rises, roll + = left rises)
    layout.add (degrees (ParamIDs::yaw,   "Yaw",   180.0f));
    layout.add (degrees (ParamIDs::pitch, "Pitch", 180.0f));
    layout.add (degrees (ParamIDs::roll,  "Roll",  180.0f));

    // ── Output ────────────────────────────────────────────────────────────────
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamIDs::decoder, 1 }, "Decoder",
        StringArray { "Binaural", "Stereo" }, 0));

    // ── Orbit ─────────────────────────────────────────────────────────────────
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamIDs::orbit, 1 }, "Orbit", false));

    {
        NormalisableRange<float> r (0.02f, 2.0f, 0.001f);
        r.setSkewForCentre (0.2f);
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { ParamIDs::orbitRate, 1 }, "Orbit Rate", r, 0.1f,
            AudioParameterFloatAttributes().withLabel ("Hz")));
    }

    // ── Input / level ─────────────────────────────────────────────────────────
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamIDs::inputMode, 1 }, "Input Mode",
        StringArray { "Mono sum", "Left only" }, 0));

    // −60 dB maps to exactly 0 (decibelsToGain minusInfinityDb = −60). Presets never set this.
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::outputLevel, 1 }, "Output Level",
        NormalisableRange<float> (-60.0f, 0.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    return layout;
}

OSimpleAmbisonicsAudioProcessor::OSimpleAmbisonicsAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    azimuthParam     = apvts.getRawParameterValue (ParamIDs::azimuth);
    elevationParam   = apvts.getRawParameterValue (ParamIDs::elevation);
    distanceParam    = apvts.getRawParameterValue (ParamIDs::distance);
    orderParam       = apvts.getRawParameterValue (ParamIDs::order);
    yawParam         = apvts.getRawParameterValue (ParamIDs::yaw);
    pitchParam       = apvts.getRawParameterValue (ParamIDs::pitch);
    rollParam        = apvts.getRawParameterValue (ParamIDs::roll);
    decoderParam     = apvts.getRawParameterValue (ParamIDs::decoder);
    orbitParam       = apvts.getRawParameterValue (ParamIDs::orbit);
    orbitRateParam   = apvts.getRawParameterValue (ParamIDs::orbitRate);
    inputModeParam   = apvts.getRawParameterValue (ParamIDs::inputMode);
    outputLevelParam = apvts.getRawParameterValue (ParamIDs::outputLevel);

    jassert (azimuthParam != nullptr && elevationParam != nullptr && distanceParam != nullptr);
    jassert (orderParam != nullptr && yawParam != nullptr && pitchParam != nullptr);
    jassert (rollParam != nullptr && decoderParam != nullptr && orbitParam != nullptr);
    jassert (orbitRateParam != nullptr && inputModeParam != nullptr && outputLevelParam != nullptr);

    distanceRange  = &dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (ParamIDs::distance))->range;
    orbitRateRange = &dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (ParamIDs::orbitRate))->range;
    jassert (distanceRange != nullptr && orbitRateRange != nullptr);

    // Stage 4 (P5): a fresh instance IS Init — its name, its targets (the
    // defaults), unmodified. No disk access here (P7: the factory bank is
    // written lazily by the editor's preset natives only).
    setCurrentPreset (ambpresets::kFactory[0].fileName,
                      resolveTargets (recipeTargets (ambpresets::kFactory[0])), true, false);
}

OSimpleAmbisonicsAudioProcessor::~OSimpleAmbisonicsAudioProcessor() = default;

osa_dsp::Params OSimpleAmbisonicsAudioProcessor::fillParams() const noexcept
{
    // JUCE legacy (juce_AudioProcessorValueTreeState.cpp, ParameterAdapter
    // ctor: "the unnormalised value should *not* be snapped on construction"):
    // the raw default of a skewed, stepped range is the UNSNAPPED
    // exp(log(p)/skew) round-trip — distance's default read 1.0000001, not
    // 1.0, until a host touched it (harness P1 anchor / P4 caught it: g(d) was
    // 1 − 1 ulp). Every host-set value goes through
    // RangedAudioParameter::convertFrom0to1 → snapToLegalValue, so snapping
    // here makes the construction default and the host path identical, and
    // "1 m = exactly no effect" holds from block 0. One floor() per range per
    // block; noexcept, no allocation.
    osa_dsp::Params p;
    p.azimuthDeg   = azimuthParam->load();
    p.elevationDeg = elevationParam->load();
    p.distanceM    = distanceRange->snapToLegalValue (distanceParam->load());
    p.order        = (int) orderParam->load() + 1;      // choice index 0..2 → order 1..3
    p.yawDeg       = yawParam->load();
    p.pitchDeg     = pitchParam->load();
    p.rollDeg      = rollParam->load();
    p.decoder      = (int) decoderParam->load();        // 0 Binaural, 1 Stereo
    p.orbit        = orbitParam->load() > 0.5f;
    p.orbitRateHz  = orbitRateRange->snapToLegalValue (orbitRateParam->load());
    return p;
}

void OSimpleAmbisonicsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // The ONE call site. Constant forever, in both decoder modes, at every
    // sample rate (ARCHITECTURE.md §Latency). setLatencySamples only notifies
    // the host when the value changes, so repeated prepares are free.
    setLatencySamples (osa_dsp::kPartition);

    maxBlock = juce::jmax (samplesPerBlock, 1);

    // Negotiated input-bus width (1 or 2) — NOT buffer.getNumChannels(), which is
    // max(in, out) = 2 in every accepted layout (pattern_stereo_in_mono_out_buffer_width_lies).
    numInputLegs = juce::jlimit (1, 2, getChannelLayoutOfBus (true, 0).size());

    // The only allocations in the processor: prepared-size scratch.
    monoScratch.assign ((size_t) maxBlock, 0.0f);
    rightScratch.assign ((size_t) maxBlock, 0.0f);

    // Params BEFORE prepare (A7): the engine snaps every smoother, ramp and
    // the orbit phase to the current setting, so block 0 is at steady state.
    engine.setParams (fillParams());
    engine.prepare (sampleRate, maxBlock);

    // Seeded: current == target at the stored outputLevel, so block 0 is at
    // full level — an unseeded ramp fades 0 → target over the first 20 ms at
    // every prepare (pattern_gain_ramp_without_seeding_fades_in_from_silence).
    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (outputLevelParam->load(), -60.0f));

    // Stage 4 WR-04: seeded the same way, so block 0 takes the exact branch.
    leftOnlyWeight.reset (sampleRate, 0.02);
    leftOnlyWeight.setCurrentAndTargetValue ((int) inputModeParam->load() == 1 ? 1.0f : 0.0f);

    // Stage 4 WR-05: the tail is the structural latency plus the Binaural FIR
    // at THIS rate (the convolver's resampled length, a multiple of 64).
    tailSeconds.store ((double) (osa_dsp::kPartition + engine.convolver().tapsAtRate()) / sampleRate,
                       std::memory_order_relaxed);

    outPeakAccL = outPeakAccR = 0.0f;
}

void OSimpleAmbisonicsAudioProcessor::releaseResources()
{
}

// Stage 4 WR-03: VST3 setProcessing (false) and AU Reset land here. The
// engine reset zeroes the convolver grid buffers + FDL, the stereo delays
// and the air-LP state, and re-snaps every smoother to the CURRENT
// parameters — prepareToPlay without the allocations, so a restart never
// plays the previous material's tail. Before the first prepare there is no
// state to clear (and no convolver tables): nothing to do.
void OSimpleAmbisonicsAudioProcessor::reset()
{
    if (maxBlock <= 0)
        return;

    engine.setParams (fillParams());
    engine.reset();

    outputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (outputLevelParam->load(), -60.0f));
    leftOnlyWeight.setCurrentAndTargetValue ((int) inputModeParam->load() == 1 ? 1.0f : 0.0f);

    outPeakAccL = outPeakAccR = 0.0f;
}

bool OSimpleAmbisonicsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in.isDisabled() || out.isDisabled())
        return false;

    // Input: mono or stereo (FUNC-07). Output: stereo only.
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;

    return out == juce::AudioChannelSet::stereo();
}

void OSimpleAmbisonicsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n      = buffer.getNumSamples();
    const int numCh  = buffer.getNumChannels();
    const int numOut = juce::jmin (numCh, 2);

    if (n <= 0 || numCh <= 0)
        return;

    // Stage 4 IN-01: a host that calls processBlock before prepareToPlay
    // (seen during some scans) would spin the slice loop forever on
    // maxBlock == 0, over empty convolver tables. Silence instead.
    if (maxBlock <= 0 || monoScratch.empty())
    {
        buffer.clear();
        return;
    }

    // Peak-since-the-UI-last-read (A8 / R7): restart the accumulators only
    // when the reader has consumed the last published frame.
    if (! meterFrames.hasNewData())
    {
        engine.resetPeaks();
        outPeakAccL = outPeakAccR = 0.0f;
    }

    // Parameters are read once per block.
    const int mode = (int) inputModeParam->load();   // 0 Mono sum, 1 Left only
    leftOnlyWeight.setTargetValue (mode == 1 ? 1.0f : 0.0f);
    engine.setParams (fillParams());
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputLevelParam->load(), -60.0f));

    const float* in0 = buffer.getReadPointer (0);
    const float* in1 = (numInputLegs == 2 && numCh > 1) ? buffer.getReadPointer (1) : nullptr;
    float* out0 = buffer.getWritePointer (0);
    float* out1 = numOut > 1 ? buffer.getWritePointer (1) : nullptr;

    float* mono  = monoScratch.data();
    float* right = rightScratch.data();

    // Slice loop: a host block larger than samplesPerBlock is processed in
    // ≤ maxBlock pieces so the prepared-size scratch is never overrun
    // (pattern_oversampler_overruns_on_oversized_host_block). The engine's
    // control grid persists across slices, so slicing is render-invisible.
    for (int start = 0; start < n; start += maxBlock)
    {
        const int len = juce::jmin (n - start, maxBlock);

        // Conditioner into the scratch for the WHOLE slice before the engine
        // writes out0/out1 (in place: in0 == out0).
        //
        // Stage 4 WR-04: a Mono sum <-> Left only change ramps the Left-only
        // weight over 20 ms (sum + w (L - sum)); outside a ramp the exact
        // branches run, so both settings are bit-identical to Stage 2. A mono
        // bus has no R leg to blend: the weight is parked at its target.
        //
        // Stage 4 WR-02: a non-finite input sample becomes 0 HERE, before any
        // engine state sees it — one NaN used to latch the air-LP integrator
        // and silence the plugin until the next prepare.
        if (in1 == nullptr)
            leftOnlyWeight.setCurrentAndTargetValue (leftOnlyWeight.getTargetValue());

        for (int i = 0; i < len; ++i)
        {
            const float l = in0[start + i];
            float x = l;
            if (in1 != nullptr)
            {
                const float sum = 0.5f * (l + in1[start + i]);
                if (leftOnlyWeight.isSmoothing())
                    x = sum + leftOnlyWeight.getNextValue() * (l - sum);
                else
                    x = mode == 0 ? sum : l;
            }
            mono[i] = std::isfinite (x) ? x : 0.0f;
        }

        engine.process (mono, out0 + start, out1 != nullptr ? out1 + start : right, len);

        for (int i = 0; i < len; ++i)
        {
            const float gain = outputGain.getNextValue();

            const float yL = out0[start + i] * gain;
            out0[start + i] = yL;
            outPeakAccL = juce::jmax (outPeakAccL, std::abs (yL));

            if (out1 != nullptr)
            {
                const float yR = out1[start + i] * gain;
                out1[start + i] = yR;
                outPeakAccR = juce::jmax (outPeakAccR, std::abs (yR));
            }
        }
    }

    // Block-level finite scrub (COMPAT-01 strictness-10 fuzz looks for this).
    for (int ch = 0; ch < numOut; ++ch)
    {
        const float* r = buffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (r[i]))
            {
                buffer.clear (ch, 0, n);
                break;
            }
        }
    }

    // Any extra output channels beyond the two we write (none in an accepted
    // layout, but bounded by numCh regardless).
    for (int ch = 2; ch < numCh; ++ch)
        buffer.clear (ch, 0, n);

    // One MeterFrame per host block: the engine's state after the last slice,
    // its accumulated bus peaks, and the processor's post-gain output peaks.
    auto& w = meterFrames.getWriteBuffer();
    w = engine.frame();
    w.outPeakL = outPeakAccL;
    w.outPeakR = outPeakAccR;
    meterFrames.publish();
}

#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OSimpleAmbisonicsAudioProcessor::createEditor()
{
   #if JUCE_WEB_BROWSER
    return new OSimpleAmbisonicsAudioProcessorEditor (*this);
   #else
    // Render harness (JUCE_WEB_BROWSER=0, no editor sources).
    return new juce::GenericAudioProcessorEditor (*this);
   #endif
}

void OSimpleAmbisonicsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Version stamp from day one — every house preset-migration gate keys off
    // this attribute, and adding it at v1.0.0 costs nothing.
    state.setProperty ("pluginVersion", JucePlugin_VersionString, nullptr);

    // Stage 3: the two non-parameter properties, on the COPY only (never
    // mutate apvts.state here). The language is written as its code string.
    state.setProperty ("uiLanguage", languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);

    // Stage 4 (P6): the preset name ("" = unnamed), a plain root property —
    // never the preset module's XML state path.
    state.setProperty ("currentPreset", getPresetName(), nullptr);

    // editorScale only when the editor has sized itself; otherwise REMOVE it,
    // because a rejected value restored by replaceState would still sit in the
    // tree and be re-saved.
    const float es = editorScale.load (std::memory_order_relaxed);
    if (es > 0.0f)
        state.setProperty ("editorScale", juce::String (es, 3), nullptr);
    else
        state.removeProperty ("editorScale", nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleAmbisonicsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

        // Presence by isVoid() and parse the STRING — never isInt() / isDouble():
        // every property comes back as a string var after the XML round-trip
        // (critical_valuetree_xml_roundtrip_loses_type). A pre-Stage-3 session
        // has neither property: English, fit-to-display.
        if (const auto lang = apvts.state.getProperty ("uiLanguage"); ! lang.isVoid())
            uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);

        // Stage 4 IN-19: absent or rejected -> 0 (fit-to-display, as
        // documented), never the previous instance's scale carried over.
        float restoredScale = 0.0f;
        if (const auto sv = apvts.state.getProperty ("editorScale"); ! sv.isVoid())
        {
            const float s = sv.toString().getFloatValue();
            if (std::isfinite (s) && s >= 0.6f && s <= 1.5f)
                restoredScale = s;
        }
        editorScale.store (restoredScale, std::memory_order_relaxed);

        // Stage 4 (P6): the preset name — the same isVoid() + string read;
        // ABSENT -> "" (an older session is unnamed, never the ctor's Init).
        // Untrusted: sanitised. A factory name rebuilds its targets from the
        // table (orderStep with the RESTORED order index, so a reopened
        // session shows the lesson lit and continues the walk, P4); a user
        // name stays stale until refreshPresetTargetsIfNeeded re-reads its
        // file on the message thread.
        const juce::var cp = apvts.state.getProperty ("currentPreset");
        const juce::String restored = cp.isVoid() ? juce::String() : sanitisePresetName (cp.toString());
        if (const auto* recipe = ambpresets::findByName (restored))
        {
            const auto t = juce::String (recipe->id) == ambpresets::kOrderStepId
                               ? orderStepTargets ((int) orderParam->load())
                               : recipeTargets (*recipe);
            setCurrentPreset (recipe->fileName, resolveTargets (t), true, false);
        }
        else
        {
            setCurrentPreset (restored, Targets {}, false, restored.isNotEmpty());
        }
    }
}

//==============================================================================
// Stage 4 presets + lessons (FUNC-08; PLAN P1 .. P9). The recipes live in ONE
// table, PresetRecipes.h. Message thread unless noted; presetLock guards the
// identity (name / targets / validity), is never taken on the audio thread and
// is never held across setValueNotifyingHost.
namespace
{
    // APVTS order = the layout order = Targets index order.
    constexpr const char* kPresetParamIds[12] = {
        ParamIDs::azimuth, ParamIDs::elevation, ParamIDs::distance, ParamIDs::order, ParamIDs::yaw,
        ParamIDs::pitch, ParamIDs::roll, ParamIDs::decoder, ParamIDs::orbit, ParamIDs::orbitRate,
        ParamIDs::inputMode, ParamIDs::outputLevel };
    constexpr int kOrderSlot = 3;

    // P9 strip set for preset names: C0, DEL, C1, bidi controls.
    bool isStrippedPresetChar (juce::juce_wchar c) noexcept
    {
        return c < 0x20 || c == 0x7f || (c >= 0x80 && c <= 0x9f)
            || c == 0x200e || c == 0x200f
            || (c >= 0x202a && c <= 0x202e)
            || (c >= 0x2066 && c <= 0x2069);
    }

    bool isJsonNumberOrBool (const juce::var& v) noexcept
    {
        return v.isDouble() || v.isInt() || v.isInt64() || v.isBool();
    }

    constexpr int   kMaxPresetNameChars = 64;
    constexpr float kApplySkipTolerance = 1.0e-6f;   // P2
    constexpr float kModifiedTolerance  = 1.0e-4f;   // P5

    OSimpleAmbisonicsAudioProcessor::Targets nanTargets()
    {
        OSimpleAmbisonicsAudioProcessor::Targets t;
        t.fill (std::numeric_limits<float>::quiet_NaN());
        return t;
    }
}

// R-D3: the one user-owned list.
bool OSimpleAmbisonicsAudioProcessor::isUserOwnedParam (const juce::String& paramId)
{
    return paramId == ParamIDs::outputLevel || paramId == ParamIDs::inputMode;
}

OSimpleAmbisonicsAudioProcessor::Targets
OSimpleAmbisonicsAudioProcessor::recipeTargets (const ambpresets::Recipe& recipe, int orderIndex) const
{
    auto t = nanTargets();
    int matched = 0;
    for (int e = 0; e < recipe.count; ++e)
    {
        const auto& entry = recipe.entries[e];
        for (size_t i = 0; i < t.size(); ++i)
        {
            if (std::strcmp (entry.paramId, kPresetParamIds[i]) != 0 || isUserOwnedParam (kPresetParamIds[i]))
                continue;
            if (auto* p = apvts.getParameter (kPresetParamIds[i]))
            {
                t[i] = p->convertTo0to1 (entry.raw);   // the parameter's OWN mapping: skew-correct
                ++matched;
            }
        }
    }
    jassert (matched == recipe.count);            // a typo in a recipe id (harness G-FACTORY[table])
    juce::ignoreUnused (matched);

    if (orderIndex >= 0)
        if (auto* p = apvts.getParameter (ParamIDs::order))
            t[(size_t) kOrderSlot] = p->convertTo0to1 ((float) juce::jlimit (0, 2, orderIndex));
    return t;
}

OSimpleAmbisonicsAudioProcessor::Targets
OSimpleAmbisonicsAudioProcessor::resolveTargets (const Targets& targets) const
{
    auto r = nanTargets();
    for (size_t i = 0; i < r.size(); ++i)
    {
        if (isUserOwnedParam (kPresetParamIds[i]))
            continue;                              // never reset, never set
        auto* p = apvts.getParameter (kPresetParamIds[i]);
        if (p == nullptr)
            continue;
        const float t = std::isfinite (targets[i]) ? juce::jlimit (0.0f, 1.0f, targets[i])
                                                   : p->getDefaultValue();   // reset-to-defaults first
        r[i] = p->convertTo0to1 (p->convertFrom0to1 (t));   // the value the parameter will hold
    }
    return r;
}

OSimpleAmbisonicsAudioProcessor::Targets OSimpleAmbisonicsAudioProcessor::currentNormalisedValues() const
{
    auto t = nanTargets();
    for (size_t i = 0; i < t.size(); ++i)
        if (! isUserOwnedParam (kPresetParamIds[i]))
            if (auto* p = apvts.getParameter (kPresetParamIds[i]))
                t[i] = p->getValue();
    return t;
}

// P2: the ONE apply core.
bool OSimpleAmbisonicsAudioProcessor::applyNormalisedTargets (const Targets& targets)
{
    const auto resolved = resolveTargets (targets);
    for (size_t i = 0; i < resolved.size(); ++i)
    {
        if (isUserOwnedParam (kPresetParamIds[i]))
            continue;
        auto* p = apvts.getParameter (kPresetParamIds[i]);
        if (p == nullptr || ! std::isfinite (resolved[i]))
            continue;
        if (std::abs (p->getValue() - resolved[i]) < kApplySkipTolerance)
            continue;                              // already there: no host edit, no gesture
        p->beginChangeGesture();
        p->setValueNotifyingHost (resolved[i]);
        p->endChangeGesture();
    }
    return true;
}

void OSimpleAmbisonicsAudioProcessor::setCurrentPreset (const juce::String& newName, const Targets& resolved,
                                                        bool valid, bool stale)
{
    {
        const juce::ScopedLock sl (presetLock);
        presetName         = newName;
        presetTargets      = resolved;
        presetTargetsValid = valid;
        presetTargetsStale = stale;
    }
    presetRevision.fetch_add (1);
}

juce::String OSimpleAmbisonicsAudioProcessor::getPresetName() const
{
    const juce::ScopedLock sl (presetLock);
    return presetName;
}

juce::String OSimpleAmbisonicsAudioProcessor::getPresetId() const
{
    const auto* recipe = ambpresets::findByName (getPresetName());
    return recipe != nullptr ? juce::String (recipe->id) : juce::String();
}

bool OSimpleAmbisonicsAudioProcessor::isPresetFactory() const
{
    return isFactoryPresetName (getPresetName());
}

bool OSimpleAmbisonicsAudioProcessor::isPresetModified() const
{
    Targets t;
    {
        const juce::ScopedLock sl (presetLock);
        if (presetName.isEmpty() || ! presetTargetsValid)
            return false;
        t = presetTargets;
    }
    for (size_t i = 0; i < t.size(); ++i)
    {
        if (isUserOwnedParam (kPresetParamIds[i]) || ! std::isfinite (t[i]))
            continue;                              // outputLevel / inputMode never make a preset "modified"
        if (auto* p = apvts.getParameter (kPresetParamIds[i]))
            if (std::abs (p->getValue() - t[i]) > kModifiedTolerance)
                return true;
    }
    return false;
}

OSimpleAmbisonicsAudioProcessor::Targets OSimpleAmbisonicsAudioProcessor::orderStepTargets (int orderIndex) const
{
    const auto* recipe = ambpresets::findById (ambpresets::kOrderStepId);
    jassert (recipe != nullptr);
    return recipe != nullptr ? recipeTargets (*recipe, juce::jlimit (0, 2, orderIndex)) : nanTargets();
}

// P3 / P4: the LESSON entry.
bool OSimpleAmbisonicsAudioProcessor::applyFactoryPreset (const juce::String& id)
{
    const auto* recipe = ambpresets::findById (id);
    if (recipe == nullptr)
        return false;                              // unknown id: nothing touched

    Targets targets;
    if (id == ambpresets::kOrderStepId)
    {
        // R-D4: continue the walk only while the scene is still the unmodified
        // orderStep scene (owned-parameter moves never break it); else order 1.
        const bool continuing = getPresetId() == ambpresets::kOrderStepId && ! isPresetModified();
        const int current = juce::jlimit (0, 2, (int) orderParam->load());
        targets = orderStepTargets (continuing ? (current + 1) % 3 : 0);
    }
    else
    {
        targets = recipeTargets (*recipe);
    }

    applyNormalisedTargets (targets);
    setCurrentPreset (recipe->fileName, resolveTargets (targets), true, false);
    return true;
}

// P3: the BROWSER entry.
bool OSimpleAmbisonicsAudioProcessor::loadPresetByName (const juce::String& name)
{
    if (const auto* recipe = ambpresets::findByName (name))
    {
        const auto targets = recipeTargets (*recipe);   // NO walk: orderStep loads order 1
        applyNormalisedTargets (targets);
        setCurrentPreset (recipe->fileName, resolveTargets (targets), true, false);
        return true;                               // the table, never the disk copy
    }

    if (name.isEmpty())
        return false;

    // Only a LISTED user preset loads: the name must BE a stem of the User
    // folder listing, which also blocks any path traversal (T-4-02).
    const auto order = getPresetWalkOrder();
    const int idx = order.indexOf (name);
    if (idx < ambpresets::kNumFactory)
        return false;

    const auto stem = order[idx];
    const auto file = presetManager.getUserPresetsDirectory().getChildFile (stem + ".json");
    if (! file.existsAsFile())
        return false;

    Targets t;
    if (! userJsonTargets (juce::JSON::parse (file.loadFileAsString()), t))
        return false;

    applyNormalisedTargets (t);
    setCurrentPreset (stem, resolveTargets (t), true, false);
    return true;
}

juce::StringArray OSimpleAmbisonicsAudioProcessor::buildWalkOrder (const juce::StringArray& userStems)
{
    juce::StringArray order;
    for (const auto& recipe : ambpresets::kFactory)
        order.add (recipe.fileName);

    auto users = userStems;
    users.sort (true);                             // case-insensitive
    for (const auto& u : users)
    {
        if (u.isEmpty() || isFactoryPresetName (u) || order.contains (u, true))
            continue;
        order.add (u);
    }
    return order;
}

juce::String OSimpleAmbisonicsAudioProcessor::neighbourIn (const juce::StringArray& order,
                                                           const juce::String& current, int dir)
{
    const int n = order.size();
    if (n == 0)
        return {};

    int idx = current.isEmpty() ? -1 : order.indexOf (current);
    if (idx < 0 && current.isNotEmpty())
        idx = order.indexOf (current, true);
    if (idx < 0)
        return dir >= 0 ? order[0] : order[n - 1];   // unnamed / unknown: top going forward, bottom going back

    const int step = dir >= 0 ? 1 : -1;
    return order[(idx + step + n) % n];
}

juce::StringArray OSimpleAmbisonicsAudioProcessor::getPresetWalkOrder() const
{
    juce::StringArray stems;
    const auto dir = presetManager.getUserPresetsDirectory();   // a path only: nothing is created
    if (dir.isDirectory())
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.json"))
            stems.add (f.getFileNameWithoutExtension());
    return buildWalkOrder (stems);
}

juce::String OSimpleAmbisonicsAudioProcessor::getNeighbourPreset (int dir) const
{
    return neighbourIn (getPresetWalkOrder(), getPresetName(), dir);
}

std::vector<OuariconPresetManager::FactoryPresetDef> OSimpleAmbisonicsAudioProcessor::buildFactoryPresetDefs() const
{
    std::vector<OuariconPresetManager::FactoryPresetDef> defs;
    defs.reserve ((size_t) ambpresets::kNumFactory);
    for (const auto& recipe : ambpresets::kFactory)
    {
        OuariconPresetManager::FactoryPresetDef def;
        def.name = recipe.fileName;
        const auto t = resolveTargets (recipeTargets (recipe));
        for (size_t i = 0; i < t.size(); ++i)
            if (! isUserOwnedParam (kPresetParamIds[i]) && std::isfinite (t[i]))
                def.parameters[juce::String (kPresetParamIds[i])] = t[i];
        defs.push_back (std::move (def));
    }
    return defs;
}

void OSimpleAmbisonicsAudioProcessor::ensureFactoryBankOnDisk()
{
    if (factoryBankEnsured)
        return;
    factoryBankEnsured = true;
    presetManager.initializeFactoryPresets (buildFactoryPresetDefs());   // sentinel-gated inside the module
}

bool OSimpleAmbisonicsAudioProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto clean = sanitisePresetName (rawName);
    if (clean.isEmpty() || isFactoryPresetName (clean))
        return false;                              // refused BEFORE the module or the disk

    ensureFactoryBankOnDisk();
    if (! presetManager.savePreset (clean))
        return false;

    // The module tracks the on-disk name; the identity follows it.
    setCurrentPreset (presetManager.getCurrentPresetName(), resolveTargets (currentNormalisedValues()), true, false);
    return true;
}

bool OSimpleAmbisonicsAudioProcessor::deleteUserPreset (const juce::String& name)
{
    if (name.isEmpty() || isFactoryPresetName (name))
        return false;

    const auto order = getPresetWalkOrder();
    const int idx = order.indexOf (name);
    if (idx < ambpresets::kNumFactory)
        return false;                              // not a listed user preset

    ensureFactoryBankOnDisk();
    if (! presetManager.deletePreset (order[idx]))
        return false;

    bool wasCurrent = false;
    {
        const juce::ScopedLock sl (presetLock);
        wasCurrent = presetName == order[idx];
    }
    if (wasCurrent)
        setCurrentPreset ({}, Targets {}, false, false);   // unnamed (never the module's "Default")
    else
        presetRevision.fetch_add (1);
    return true;
}

juce::var OSimpleAmbisonicsAudioProcessor::getPresetCatalog() const
{
    juce::Array<juce::var> factory, user;
    for (const auto& recipe : ambpresets::kFactory)
    {
        auto* entry = new juce::DynamicObject();
        entry->setProperty ("name", juce::String (recipe.fileName));
        entry->setProperty ("id",   juce::String (recipe.id));
        factory.add (juce::var (entry));
    }

    const auto order = getPresetWalkOrder();
    for (int i = ambpresets::kNumFactory; i < order.size(); ++i)
        user.add (order[i]);

    auto* root = new juce::DynamicObject();
    root->setProperty ("factory", juce::var (std::move (factory)));
    root->setProperty ("user",    juce::var (std::move (user)));
    return juce::var (root);
}

bool OSimpleAmbisonicsAudioProcessor::userJsonTargets (const juce::var& presetJson, Targets& out) const
{
    out = nanTargets();

    const auto* root = presetJson.getDynamicObject();
    if (! presetJson.isObject() || root == nullptr)
        return false;

    const juce::var paramsVar = root->getProperty ("parameters");
    const auto* paramsObj = paramsVar.getDynamicObject();
    if (! paramsVar.isObject() || paramsObj == nullptr)
        return false;

    for (size_t i = 0; i < out.size(); ++i)
    {
        if (isUserOwnedParam (kPresetParamIds[i]))
            continue;                              // stored by the module's save, never applied
        const juce::var v = paramsObj->getProperty (juce::Identifier (kPresetParamIds[i]));
        if (isJsonNumberOrBool (v))
            out[i] = (float) (double) v;           // anything else stays NaN -> default
    }
    return true;
}

bool OSimpleAmbisonicsAudioProcessor::applyUserPresetJson (const juce::var& presetJson)
{
    Targets t;
    if (! userJsonTargets (presetJson, t))
        return false;                              // nothing changed
    return applyNormalisedTargets (t);
}

void OSimpleAmbisonicsAudioProcessor::refreshPresetTargetsIfNeeded()
{
    juce::String stale;
    {
        const juce::ScopedLock sl (presetLock);
        if (! presetTargetsStale)
            return;
        presetTargetsStale = false;                // ONE attempt per restore
        stale = presetName;
    }
    if (stale.isEmpty())
        return;

    // The name was sanitised on restore (no separators, no leading dots).
    const auto file = presetManager.getUserPresetsDirectory().getChildFile (stale + ".json");
    if (! file.existsAsFile())
        return;                                    // missing: targets stay unknown -> unmodified

    Targets t;
    if (! userJsonTargets (juce::JSON::parse (file.loadFileAsString()), t))
        return;
    const auto resolved = resolveTargets (t);

    {
        const juce::ScopedLock sl (presetLock);
        if (presetName != stale || presetTargetsValid)
            return;                                // superseded meanwhile
        presetTargets      = resolved;
        presetTargetsValid = true;
    }
    presetRevision.fetch_add (1);
}

juce::String OSimpleAmbisonicsAudioProcessor::sanitisePresetName (const juce::String& raw)
{
    juce::String clean;
    const auto trimmed = raw.trim();
    for (auto cp = trimmed.getCharPointer(); ! cp.isEmpty();)
    {
        const juce::juce_wchar c = cp.getAndAdvance();
        if (! isStrippedPresetChar (c))
            clean += c;
    }

    clean = juce::File::createLegalFileName (clean);   // removes "#@,;:<>*^|?\/

    // Leading dots (hidden files, "..") and any whitespace the strip exposed.
    for (;;)
    {
        auto next = clean.trimStart();
        while (next.startsWithChar ('.'))
            next = next.substring (1);
        if (next == clean)
            break;
        clean = next;
    }

    return clean.substring (0, kMaxPresetNameChars).trimEnd();   // "" = refused
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleAmbisonicsAudioProcessor();
}
