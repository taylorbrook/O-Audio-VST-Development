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

    Stage 1 (Foundation): stereo-effect shell. The 12 APVTS parameters below
    are the BINDING contract from research/ARCHITECTURE.md §Parameter Mapping.
    processBlock is a pass-through with the FINAL latency already in place:

        input conditioner (Mono sum / Left only)
          → FixedDelay<kPartition = 64>
          → seeded output gain (−60 dB = silence)
          → both outputs

    Stage 2 swaps the delay for SpatialEngine inside the existing slice loop;
    the host-visible latency (setLatencySamples(kPartition)) never changes.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <cmath>

// NOTE: no PluginEditor.h include in this TU at Stage 1. When Stage 3 adds the
// WebView editor, its include goes inside a #if JUCE_WEB_BROWSER guard directly
// above createEditor() (Generic editor in the #else) so the render harness
// (JUCE_WEB_BROWSER=0, no editor sources) keeps compiling this file
// (pattern_render_harness_breaks_on_webview_editor).

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
    inputModeParam   = apvts.getRawParameterValue (ParamIDs::inputMode);
    outputLevelParam = apvts.getRawParameterValue (ParamIDs::outputLevel);
    jassert (inputModeParam != nullptr && outputLevelParam != nullptr);
}

OSimpleAmbisonicsAudioProcessor::~OSimpleAmbisonicsAudioProcessor() = default;

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

    delay.reset();

    // Seeded: current == target at the stored outputLevel, so block 0 is at
    // full level — an unseeded ramp fades 0 → target over the first 20 ms at
    // every prepare (pattern_gain_ramp_without_seeding_fades_in_from_silence).
    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (outputLevelParam->load(), -60.0f));
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

    // Parameters are read once per block.
    const int mode = (int) inputModeParam->load();   // 0 Mono sum, 1 Left only
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputLevelParam->load(), -60.0f));

    const float* in0 = buffer.getReadPointer (0);
    const float* in1 = (numInputLegs == 2 && numCh > 1) ? buffer.getReadPointer (1) : nullptr;
    float* out0 = buffer.getWritePointer (0);
    float* out1 = numOut > 1 ? buffer.getWritePointer (1) : nullptr;

    // Slice loop: a host block larger than samplesPerBlock is processed in
    // ≤ maxBlock pieces. Nothing here has a prepared-size buffer yet, but
    // Stage 2's engine will, and the harness P3 gate carries forward unchanged
    // (pattern_oversampler_overruns_on_oversized_host_block).
    for (int start = 0; start < n; start += maxBlock)
    {
        const int end = juce::jmin (n, start + maxBlock);

        for (int i = start; i < end; ++i)
        {
            // In-place: read L,R for sample i BEFORE writing out0/out1 at i.
            const float l = in0[i];
            const float s = (in1 != nullptr)
                                ? (mode == 0 ? 0.5f * (l + in1[i]) : l)
                                : l;

            const float y = delay.process (s) * outputGain.getNextValue();

            out0[i] = y;
            if (out1 != nullptr)
                out1[i] = y;
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
}

juce::AudioProcessorEditor* OSimpleAmbisonicsAudioProcessor::createEditor()
{
    // Stages 1–2: Generic editor, inline (no editor TU). Stage 3 wraps this in
    //   #if JUCE_WEB_BROWSER  return new OSimpleAmbisonicsEditor (*this);
    //   #else                 return new juce::GenericAudioProcessorEditor (*this);
    // so the render harness (JUCE_WEB_BROWSER=0) keeps building.
    return new juce::GenericAudioProcessorEditor (*this);
}

void OSimpleAmbisonicsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Version stamp from day one — every house preset-migration gate keys off
    // this attribute, and adding it at v1.0.0 costs nothing.
    state.setProperty ("pluginVersion", JucePlugin_VersionString, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleAmbisonicsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleAmbisonicsAudioProcessor();
}
