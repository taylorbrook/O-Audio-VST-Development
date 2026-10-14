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
    code string) and editorScale (only when > 0). applyFactoryPreset() applies
    the eight provisional lesson recipes and never touches outputLevel.
    processBlock / prepareToPlay / fillParams / createParameterLayout are
    unchanged from Stage 2.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <cmath>

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

    outPeakAccL = outPeakAccR = 0.0f;
}

void OSimpleAmbisonicsAudioProcessor::releaseResources()
{
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

    // Peak-since-the-UI-last-read (A8 / R7): restart the accumulators only
    // when the reader has consumed the last published frame.
    if (! meterFrames.hasNewData())
    {
        engine.resetPeaks();
        outPeakAccL = outPeakAccR = 0.0f;
    }

    // Parameters are read once per block.
    const int mode = (int) inputModeParam->load();   // 0 Mono sum, 1 Left only
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
        for (int i = 0; i < len; ++i)
        {
            const float l = in0[start + i];
            mono[i] = (in1 != nullptr)
                          ? (mode == 0 ? 0.5f * (l + in1[start + i]) : l)
                          : l;
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

        if (const auto sv = apvts.state.getProperty ("editorScale"); ! sv.isVoid())
        {
            const float s = sv.toString().getFloatValue();
            if (std::isfinite (s) && s >= 0.6f && s <= 1.5f)
                editorScale.store (s, std::memory_order_relaxed);
        }
    }
}

//==============================================================================
// Lesson tour (UI-08). Provisional recipes (v1-ui.yaml:371-379); Stage 4 refines
// them and adds the FUNC-08 bank. No recipe combines order 3 with distance < 1 m
// (Binaural headroom: order-3 bank +6.5 dB peak, 0.2 m +8.9 dB).
namespace
{
    struct LessonEntry { const char* paramId; float plain; };   // engineering units; choices / bool by index
    struct Lesson      { const char* id; int count; LessonEntry entries[4]; };

    constexpr Lesson kLessons[8] = {
        { "pointSource",        4, { { ParamIDs::order, 0.0f }, { ParamIDs::azimuth, 30.0f }, { ParamIDs::elevation, 0.0f }, { ParamIDs::distance, 1.0f } } },
        { "orderStep",          2, { { ParamIDs::order, 2.0f }, { ParamIDs::azimuth, 45.0f }, {}, {} } },
        { "behindYou",          1, { { ParamIDs::azimuth, 180.0f }, {}, {}, {} } },
        { "overhead",           2, { { ParamIDs::elevation, 90.0f }, { ParamIDs::order, 1.0f }, {}, {} } },
        { "rotateRoom",         3, { { ParamIDs::azimuth, 60.0f }, { ParamIDs::yaw, 120.0f }, { ParamIDs::order, 1.0f }, {} } },
        { "farNear",            1, { { ParamIDs::distance, 12.0f }, {}, {}, {} } },
        { "slowOrbit",          3, { { ParamIDs::orbit, 1.0f }, { ParamIDs::orbitRate, 0.05f }, { ParamIDs::order, 1.0f }, {} } },
        { "speakersHeadphones", 3, { { ParamIDs::decoder, 1.0f }, { ParamIDs::order, 2.0f }, { ParamIDs::azimuth, 45.0f }, {} } },
    };

    constexpr float kLessonSkipTolerance = 1.0e-6f;
}

bool OSimpleAmbisonicsAudioProcessor::applyFactoryPreset (const juce::String& lessonId)
{
    const Lesson* lesson = nullptr;
    for (const auto& l : kLessons)
        if (lessonId == l.id)
            lesson = &l;

    if (lesson == nullptr)
        return false;                                   // unknown id: nothing touched

    for (auto* base : getParameters())
    {
        auto* p = dynamic_cast<juce::RangedAudioParameter*> (base);
        if (p == nullptr)
            continue;

        const auto& pid = p->getParameterID();
        if (pid == ParamIDs::outputLevel)
            continue;                                   // user-owned: never reset, never set

        float target = p->getDefaultValue();            // reset-to-default unless the recipe names it
        for (int e = 0; e < lesson->count; ++e)
            if (pid == lesson->entries[e].paramId)
                target = p->convertTo0to1 (lesson->entries[e].plain);

        // Snap to the value the parameter will actually hold (choice / bool /
        // stepped range), so the skip test compares like with like.
        target = p->convertTo0to1 (p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, target)));

        if (std::abs (p->getValue() - target) < kLessonSkipTolerance)
            continue;                                   // already there: no host edit, no gesture

        p->beginChangeGesture();
        p->setValueNotifyingHost (target);
        p->endChangeGesture();
    }
    return true;
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleAmbisonicsAudioProcessor();
}
