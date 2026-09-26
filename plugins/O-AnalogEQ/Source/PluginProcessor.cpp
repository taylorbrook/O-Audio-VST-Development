/*
   This file is part of O-AnalogEQ, an Ouaricon Audio plugin.
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

    Ouaricon Analog EQ - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is deliberately NOT included at the top of this TU — the
// include lives inside the #if JUCE_WEB_BROWSER guard directly above
// createEditor(), so a console target that compiles this TU with
// JUCE_WEB_BROWSER=0 and no editor sources (scripts/param-dump) links.

juce::AudioProcessorValueTreeState::ParameterLayout OuariconAnalogEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // LF Band (Low Frequency Shelf)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lf_freq", 1 }, "LF Frequency",
        juce::NormalisableRange<float>(30.0f, 500.0f, 0.1f, 0.3f), 100.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lf_gain", 1 }, "LF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lf_on", 1 }, "LF On", true));

    // LMF Band (Low-Mid Frequency Bell)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lmf_freq", 1 }, "LMF Frequency",
        juce::NormalisableRange<float>(100.0f, 2000.0f, 0.1f, 0.3f), 500.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lmf_gain", 1 }, "LMF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "lmf_q", 1 }, "LMF Q",
        juce::StringArray { "WIDE", "MED", "TIGHT" }, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lmf_on", 1 }, "LMF On", true));

    // HMF Band (High-Mid Frequency Bell)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hmf_freq", 1 }, "HMF Frequency",
        juce::NormalisableRange<float>(500.0f, 8000.0f, 0.1f, 0.3f), 2000.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hmf_gain", 1 }, "HMF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "hmf_q", 1 }, "HMF Q",
        juce::StringArray { "WIDE", "MED", "TIGHT" }, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "hmf_on", 1 }, "HMF On", true));

    // HF Band (High Frequency Shelf)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hf_freq", 1 }, "HF Frequency",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 0.1f, 0.3f), 8000.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hf_gain", 1 }, "HF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "hf_on", 1 }, "HF On", true));

    // Global Controls
    // NOTE (IN-01): output_gain is intentionally NOT exposed in the WebView UI — the
    // output knob was deliberately removed in the v1.0.5 UI simplification. It remains a
    // host-automatable parameter (default 0 dB, so benign when untouched) and is set by
    // some factory presets (e.g. "Surgical Cut" = +1 dB). Kept for host automation and
    // preset fidelity; do NOT add a relay/attachment unless the UI is meant to surface it.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output_gain", 1 }, "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "analog", 1 }, "Analog", true));

    return layout;
}

OuariconAnalogEQAudioProcessor::OuariconAnalogEQAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-AnalogEQ")
{
    // Initialize 12 factory presets
    // Values are normalized 0.0-1.0 (as used by setValueNotifyingHost)
    // Gain: normalized = (dB + 12) / 24   (e.g. 0dB=0.5, +6dB=0.75, -6dB=0.25)
    // Q choice: WIDE=0.0, MED=0.5, TIGHT=1.0
    // Bool: true=1.0, false=0.0
    // Freq: pow((hz - min) / (max - min), 0.3) due to NormalisableRange skew
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        {
            "Default",
            {{"lf_freq", 0.577f}, {"lf_gain", 0.5f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.627f}, {"lmf_gain", 0.5f}, {"lmf_q", 0.5f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.617f}, {"hmf_gain", 0.5f}, {"hmf_q", 0.5f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.710f}, {"hf_gain", 0.5f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 1.0f}},
            juce::var()
        },
        {
            "Vocal Presence",
            {{"lf_freq", 0.577f}, {"lf_gain", 0.5f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.729f}, {"lmf_gain", 0.417f}, {"lmf_q", 0.0f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.710f}, {"hmf_gain", 0.667f}, {"hmf_q", 0.5f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.776f}, {"hf_gain", 0.583f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 1.0f}},
            juce::var()
        },
        {
            "Bass Boost",
            {{"lf_freq", 0.519f}, {"lf_gain", 0.75f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.480f}, {"lmf_gain", 0.583f}, {"lmf_q", 0.0f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.617f}, {"hmf_gain", 0.5f}, {"hmf_q", 0.5f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.710f}, {"hf_gain", 0.417f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 1.0f}},
            juce::var()
        },
        {
            "Bright and Airy",
            {{"lf_freq", 0.577f}, {"lf_gain", 0.5f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.627f}, {"lmf_gain", 0.458f}, {"lmf_q", 0.0f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.784f}, {"hmf_gain", 0.583f}, {"hmf_q", 0.0f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.832f}, {"hf_gain", 0.667f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 0.0f}},
            juce::var()
        },
        {
            "Warm Vintage",
            {{"lf_freq", 0.663f}, {"lf_gain", 0.625f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.627f}, {"lmf_gain", 0.542f}, {"lmf_q", 0.0f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.710f}, {"hmf_gain", 0.417f}, {"hmf_q", 0.0f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.635f}, {"hf_gain", 0.333f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 1.0f}},
            juce::var()
        },
        {
            "Mid Scoop",
            {{"lf_freq", 0.577f}, {"lf_gain", 0.583f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.627f}, {"lmf_gain", 0.333f}, {"lmf_q", 0.0f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.617f}, {"hmf_gain", 0.333f}, {"hmf_q", 0.0f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.710f}, {"hf_gain", 0.583f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 1.0f}},
            juce::var()
        },
        {
            "Telephone",
            {{"lf_freq", 0.726f}, {"lf_gain", 0.167f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.729f}, {"lmf_gain", 0.625f}, {"lmf_q", 0.5f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.710f}, {"hmf_gain", 0.583f}, {"hmf_q", 0.5f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.524f}, {"hf_gain", 0.167f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 0.0f}},
            juce::var()
        },
        {
            "De-Mud",
            {{"lf_freq", 0.577f}, {"lf_gain", 0.5f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.518f}, {"lmf_gain", 0.333f}, {"lmf_q", 0.5f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.617f}, {"hmf_gain", 0.542f}, {"hmf_q", 0.5f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.776f}, {"hf_gain", 0.542f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 1.0f}},
            juce::var()
        },
        {
            "Hi-Fi Smile",
            {{"lf_freq", 0.519f}, {"lf_gain", 0.667f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.627f}, {"lmf_gain", 0.417f}, {"lmf_q", 0.0f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.617f}, {"hmf_gain", 0.458f}, {"hmf_q", 0.0f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.776f}, {"hf_gain", 0.667f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 1.0f}},
            juce::var()
        },
        {
            "Radio Ready",
            {{"lf_freq", 0.577f}, {"lf_gain", 0.417f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.789f}, {"lmf_gain", 0.625f}, {"lmf_q", 1.0f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.710f}, {"hmf_gain", 0.625f}, {"hmf_q", 1.0f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.710f}, {"hf_gain", 0.542f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 1.0f}},
            juce::var()
        },
        {
            "Dark Ambient",
            {{"lf_freq", 0.519f}, {"lf_gain", 0.583f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.627f}, {"lmf_gain", 0.542f}, {"lmf_q", 0.0f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.845f}, {"hmf_gain", 0.375f}, {"hmf_q", 0.0f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.635f}, {"hf_gain", 0.167f}, {"hf_on", 1.0f},
             {"output_gain", 0.5f}, {"analog", 1.0f}},
            juce::var()
        },
        {
            "Surgical Cut",
            {{"lf_freq", 0.444f}, {"lf_gain", 0.375f}, {"lf_on", 1.0f},
             {"lmf_freq", 0.583f}, {"lmf_gain", 0.25f}, {"lmf_q", 1.0f}, {"lmf_on", 1.0f},
             {"hmf_freq", 0.710f}, {"hmf_gain", 0.25f}, {"hmf_q", 1.0f}, {"hmf_on", 1.0f},
             {"hf_freq", 0.710f}, {"hf_gain", 0.5f}, {"hf_on", 1.0f},
             {"output_gain", 0.542f}, {"analog", 0.0f}},
            juce::var()
        }
    };

    presetManager.initializeFactoryPresets(factoryPresets);
}

void OuariconAnalogEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    lfFilter.prepare(spec);
    lmfFilter.prepare(spec);
    hmfFilter.prepare(spec);
    hfFilter.prepare(spec);
    saturation.prepare(spec);

    // WR-05: juce::dsp::Gain defaults `rampDurationSeconds = 0`, and reset() then calls
    // gain.reset(sampleRate, 0) — a SmoothedValue with zero steps to target, so
    // setGainDecibels JUMPS. The v1.1.9 smoothing work (WR-02) covered the eight band
    // frequency/gain values and left this one gain stage stepping per block: loading a
    // preset whose output_gain differs (Default 0.5 -> Surgical Cut 0.542, i.e.
    // 0 dB -> +1.008 dB, ~12% linear) clicked in a single sample while every band
    // around it ramped over 30 ms. Host automation of Output Gain zippered for the
    // same reason.
    // Order matters: setRampDurationSeconds() calls reset() internally, which no-ops
    // while sampleRate is still 0 — so set the duration FIRST and let prepare()'s own
    // reset() pick it up with the real sample rate.
    outputGain.setRampDurationSeconds(static_cast<double>(kSmoothingSeconds));
    outputGain.prepare(spec);

    lfFilter.reset();
    lmfFilter.reset();
    hmfFilter.reset();
    hfFilter.reset();
    saturation.reset();
    outputGain.reset();

    // Gentle warmth: low drive (0.5x) preserves dynamics,
    // 2.0x post-gain compensates for tanh compression
    saturation.functionToUse = [](float x) { return std::tanh(x * 0.5f) * 2.0f; };

    // WR-02: configure the frequency/gain smoothers, then seed them to the current
    // parameter values so the first blocks don't ramp from zero (which would swoop
    // every cutoff up from 0 Hz on load).
    for (auto* sm : { &lfFreqSm, &lfGainSm, &lmfFreqSm, &lmfGainSm,
                      &hmfFreqSm, &hmfGainSm, &hfFreqSm, &hfGainSm })
        sm->reset(sampleRate, static_cast<double>(kSmoothingSeconds));

    lfFreqSm.setCurrentAndTargetValue(parameters.getRawParameterValue("lf_freq")->load());
    lfGainSm.setCurrentAndTargetValue(parameters.getRawParameterValue("lf_gain")->load());
    lmfFreqSm.setCurrentAndTargetValue(parameters.getRawParameterValue("lmf_freq")->load());
    lmfGainSm.setCurrentAndTargetValue(parameters.getRawParameterValue("lmf_gain")->load());
    hmfFreqSm.setCurrentAndTargetValue(parameters.getRawParameterValue("hmf_freq")->load());
    hmfGainSm.setCurrentAndTargetValue(parameters.getRawParameterValue("hmf_gain")->load());
    hfFreqSm.setCurrentAndTargetValue(parameters.getRawParameterValue("hf_freq")->load());
    hfGainSm.setCurrentAndTargetValue(parameters.getRawParameterValue("hf_gain")->load());

    // WR-06: configure the four band wet/dry mix smoothers over the same 30 ms ramp,
    // then seed each to the band's CURRENT on/off state. Seeding matters: without it
    // every enabled band would fade in from silence on the first blocks after load,
    // which is a new artefact rather than a fix.
    for (auto* sm : { &lfMixSm, &lmfMixSm, &hmfMixSm, &hfMixSm })
        sm->reset(sampleRate, static_cast<double>(kSmoothingSeconds));

    lfMixSm.setCurrentAndTargetValue (parameters.getRawParameterValue("lf_on")->load()  > 0.5f ? 1.0f : 0.0f);
    lmfMixSm.setCurrentAndTargetValue(parameters.getRawParameterValue("lmf_on")->load() > 0.5f ? 1.0f : 0.0f);
    hmfMixSm.setCurrentAndTargetValue(parameters.getRawParameterValue("hmf_on")->load() > 0.5f ? 1.0f : 0.0f);
    hfMixSm.setCurrentAndTargetValue (parameters.getRawParameterValue("hf_on")->load()  > 0.5f ? 1.0f : 0.0f);

    // Wet scratch for the crossfade. Sized from the prepared channel count and block
    // size so processBlock never allocates; processBlock additionally slices against
    // this length, so a host that hands us a longer block than it promised cannot
    // overrun it.
    scratch.setSize(juce::jmax(1, static_cast<int>(spec.numChannels)),
                    juce::jmax(1, samplesPerBlock),
                    false, true, false);
    scratch.clear();

    // Force a coefficient (re)build on the first block after prepare (also covers a
    // sample-rate change); Q sentinels reset so the first block rebuilds the bells too.
    lastLmfQ = lastHmfQ = -1;
    coeffsInitialised = false;
}

// CR-02: see the declaration in PluginProcessor.h for the crash this closes.
// Mono and stereo are both accepted (the DSP is channel-count agnostic once the
// filter count matches the buffer width), but only symmetrically — an asymmetric
// negotiation is what breaks the ProcessorDuplicator/buffer-width invariant.
bool OuariconAnalogEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void OuariconAnalogEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters
    const float lfFreq  = parameters.getRawParameterValue("lf_freq")->load();
    const float lfGain  = parameters.getRawParameterValue("lf_gain")->load();
    const bool  lfOn    = parameters.getRawParameterValue("lf_on")->load() > 0.5f;

    const float lmfFreq = parameters.getRawParameterValue("lmf_freq")->load();
    const float lmfGain = parameters.getRawParameterValue("lmf_gain")->load();
    const int   lmfQ    = static_cast<int>(parameters.getRawParameterValue("lmf_q")->load());
    const bool  lmfOn   = parameters.getRawParameterValue("lmf_on")->load() > 0.5f;

    const float hmfFreq = parameters.getRawParameterValue("hmf_freq")->load();
    const float hmfGain = parameters.getRawParameterValue("hmf_gain")->load();
    const int   hmfQ    = static_cast<int>(parameters.getRawParameterValue("hmf_q")->load());
    const bool  hmfOn   = parameters.getRawParameterValue("hmf_on")->load() > 0.5f;

    const float hfFreq  = parameters.getRawParameterValue("hf_freq")->load();
    const float hfGain  = parameters.getRawParameterValue("hf_gain")->load();
    const bool  hfOn    = parameters.getRawParameterValue("hf_on")->load() > 0.5f;

    const float outputGainDB = parameters.getRawParameterValue("output_gain")->load();
    const bool  analogOn     = parameters.getRawParameterValue("analog")->load() > 0.5f;

    // WR-02: feed the smoothers; their per-chunk values drive the coefficients below.
    lfFreqSm.setTargetValue(lfFreq);   lfGainSm.setTargetValue(lfGain);
    lmfFreqSm.setTargetValue(lmfFreq); lmfGainSm.setTargetValue(lmfGain);
    hmfFreqSm.setTargetValue(hmfFreq); hmfGainSm.setTargetValue(hmfGain);
    hfFreqSm.setTargetValue(hfFreq);   hfGainSm.setTargetValue(hfGain);

    const bool lmfQChanged = (lmfQ != lastLmfQ);
    const bool hmfQChanged = (hmfQ != lastHmfQ);
    lastLmfQ = lmfQ; lastHmfQ = hmfQ;

    outputGain.setGainDecibels(outputGainDB);

    // WR-03: keep every cutoff below Nyquist so the biquad math never receives an
    // out-of-range frequency (degenerate/NaN coefficients at very low sample rates).
    const float nyquist = static_cast<float>(currentSampleRate) * 0.5f;
    auto clampFreq = [nyquist](float hz) { return juce::jmin(hz, nyquist * 0.99f); };
    auto dBtoGain  = [](float dB)        { return std::pow(10.0f, dB / 20.0f); };

    // A band is "moving" while its smoother is ramping, when its Q changed, or on the
    // forced first build. Bands that aren't moving keep their existing coefficients —
    // that is CR-01's allocation-free steady-state path. Rebuilds use ArrayCoefficients
    // (same math as make*, but returns a stack array; assigning into the existing state
    // reuses its storage, so there is no audio-thread allocation).
    const bool force     = !coeffsInitialised;
    const bool lfMoving  = force || lfFreqSm.isSmoothing()  || lfGainSm.isSmoothing();
    const bool lmfMoving = force || lmfQChanged || lmfFreqSm.isSmoothing() || lmfGainSm.isSmoothing();
    const bool hmfMoving = force || hmfQChanged || hmfFreqSm.isSmoothing() || hmfGainSm.isSmoothing();
    const bool hfMoving  = force || hfFreqSm.isSmoothing()  || hfGainSm.isSmoothing();

    // WR-06: feed the band mix smoothers from the on/off booleans. The filters run
    // unconditionally from here on; these ramps decide how much of each band's output
    // is heard, which is what turns a hard branch into a 30 ms crossfade.
    lfMixSm.setTargetValue (lfOn  ? 1.0f : 0.0f);
    lmfMixSm.setTargetValue(lmfOn ? 1.0f : 0.0f);
    hmfMixSm.setTargetValue(hmfOn ? 1.0f : 0.0f);
    hfMixSm.setTargetValue (hfOn  ? 1.0f : 0.0f);

    // Process audio: LF -> LMF -> HMF -> HF -> Saturation -> Output Gain
    //
    // The chain is clamped to the channel count the duplicators were actually prepared
    // with. CR-02's layout override already makes buffer width == prepared filter count,
    // so this clamp is unreachable; it stays as defence in depth because a buffer's
    // width is max(totalIn, totalOut) and therefore says nothing on its own about how
    // many output channels exist (pattern_stereo_in_mono_out_buffer_width_lies).
    const size_t preparedChannels = static_cast<size_t>(juce::jmax(0, scratch.getNumChannels()));
    const size_t usableChannels   = juce::jmin(static_cast<size_t>(buffer.getNumChannels()),
                                               juce::jmin(preparedChannels, kMaxChannels));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::AudioBlock<float> scratchBlock(scratch);

    if (usableChannels == 0)
        return;

    block        = block.getSubsetChannelBlock(0, usableChannels);
    scratchBlock = scratchBlock.getSubsetChannelBlock(0, usableChannels);

    // WR-06: run one band over `sub`, honouring its wet/dry ramp. Three paths, and the
    // middle one is the finding's real fix:
    //   m == 1, settled -> filter IN PLACE. Identical cost to the old `if (lfOn)` branch,
    //                      so the overwhelmingly common case pays nothing for the fix.
    //   m == 0, settled -> filter into scratch and DISCARD the samples. The band is
    //                      inaudible, but its z-1/z-2 keep tracking the signal, so the
    //                      next rising edge has nothing stale to convolve in.
    //   otherwise       -> filter into scratch, then blend per sample. Dry and wet are
    //                      strongly correlated (wet IS dry, filtered), so they sum
    //                      coherently and a linear amplitude fade is the equal-GAIN
    //                      choice here — an equal-power (sqrt) pair would bulge.
    auto runBand = [&](StereoFilter& filter, SmoothedFloat& mixSm,
                       juce::dsp::AudioBlock<float>& sub)
    {
        const size_t len = sub.getNumSamples();
        const size_t nch = sub.getNumChannels();

        if (! mixSm.isSmoothing())
        {
            const float settled = mixSm.getCurrentValue();

            if (settled >= 1.0f)
            {
                juce::dsp::ProcessContextReplacing<float> ctx(sub);
                filter.process(ctx);
                return;
            }

            if (settled <= 0.0f)
            {
                auto wet = scratchBlock.getSubBlock(0, len);
                juce::dsp::ProcessContextNonReplacing<float> ctx(sub, wet);
                filter.process(ctx);
                return;
            }
        }

        auto wet = scratchBlock.getSubBlock(0, len);
        juce::dsp::ProcessContextNonReplacing<float> ctx(sub, wet);
        filter.process(ctx);

        // Sample-outer, channel-inner: the ramp is advanced ONCE per sample so every
        // channel sees the same mix. Advancing it per channel would run the fade nch
        // times too fast and decorrelate L from R.
        float* dryWrite[kMaxChannels] {};
        const float* wetRead[kMaxChannels] {};

        for (size_t ch = 0; ch < nch; ++ch)
        {
            dryWrite[ch] = sub.getChannelPointer(ch);
            wetRead[ch]  = wet.getChannelPointer(ch);
        }

        for (size_t i = 0; i < len; ++i)
        {
            const float m   = mixSm.getNextValue();
            const float dry = 1.0f - m;

            for (size_t ch = 0; ch < nch; ++ch)
                dryWrite[ch][i] = dryWrite[ch][i] * dry + wetRead[ch][i] * m;
        }
    };

    auto processChunk = [&](size_t start, size_t len)
    {
        auto sub = block.getSubBlock(start, len);

        runBand(lfFilter,  lfMixSm,  sub);
        runBand(lmfFilter, lmfMixSm, sub);
        runBand(hmfFilter, hmfMixSm, sub);
        runBand(hfFilter,  hfMixSm,  sub);

        juce::dsp::ProcessContextReplacing<float> context(sub);
        if (analogOn) saturation.process(context);
        outputGain.process(context);
    };

    const int numSamples   = buffer.getNumSamples();
    const bool coeffsMoving = (lfMoving || lmfMoving || hmfMoving || hfMoving);

    // Chunk granularity. Coefficient rebuilds need kSmoothingBlock; when nothing is
    // moving the whole block still goes in one pass, preserving CR-01's steady-state
    // path. Either way the chunk never exceeds the scratch length, so a host handing us
    // a longer block than it promised in prepareToPlay cannot overrun it. The mix
    // crossfade imposes no granularity of its own — it ramps per sample inside runBand.
    const int scratchLen = juce::jmax(1, scratch.getNumSamples());
    const int chunkLen   = coeffsMoving ? juce::jmin(kSmoothingBlock, scratchLen)
                                        : scratchLen;

    for (int pos = 0; pos < numSamples; pos += chunkLen)
    {
        const int n = juce::jmin(chunkLen, numSamples - pos);

        if (coeffsMoving)
        {
            // Advance moving bands and rebuild from the end-of-chunk value (skip()
            // returns the exact target on the final ramp chunk, so no residual offset).
            if (lfMoving)
                *lfFilter.state = ArrayCoeffs::makeLowShelf(
                    currentSampleRate, clampFreq(lfFreqSm.skip(n)), 0.707f, dBtoGain(lfGainSm.skip(n)));
            if (lmfMoving)
                *lmfFilter.state = ArrayCoeffs::makePeakFilter(
                    currentSampleRate, clampFreq(lmfFreqSm.skip(n)), qValues[lmfQ], dBtoGain(lmfGainSm.skip(n)));
            if (hmfMoving)
                *hmfFilter.state = ArrayCoeffs::makePeakFilter(
                    currentSampleRate, clampFreq(hmfFreqSm.skip(n)), qValues[hmfQ], dBtoGain(hmfGainSm.skip(n)));
            if (hfMoving)
                *hfFilter.state = ArrayCoeffs::makeHighShelf(
                    currentSampleRate, clampFreq(hfFreqSm.skip(n)), 0.707f, dBtoGain(hfGainSm.skip(n)));
        }

        processChunk(static_cast<size_t>(pos), static_cast<size_t>(n));
    }

    if (numSamples > 0)
        coeffsInitialised = true;

    // VU Meter - peak level after all processing
    float peakLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peakLevel = std::max(peakLevel, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));

    outputLevelDB.store(peakLevel > 0.00001f
        ? juce::Decibels::gainToDecibels(peakLevel)
        : -100.0f, std::memory_order_relaxed);
}

#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OuariconAnalogEQAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OuariconAnalogEQAudioProcessorEditor(*this);
#else
    // The param-dump console target builds with JUCE_WEB_BROWSER=0 and no
    // editor sources. It never opens an editor; this keeps the TU linkable.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

void OuariconAnalogEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v1.2.0: the UI language rides the same tree as one more plain property.
    // Written BEFORE getStateAsXml(), because that method serialises
    // parameters.copyState() and would otherwise take a snapshot without it.
    // Written as a STRING ("en"/"fr") rather than the atomic's int index, so a
    // hand-inspected session file says what it means.
    parameters.state.setProperty("uiLanguage",
                                 languageCode(uiLanguage.load(std::memory_order_acquire)),
                                 nullptr);

    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OuariconAnalogEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        presetManager.setStateFromXml(xml.get());

    // v1.2.0: the UI language. Read AFTER setStateFromXml, which calls
    // parameters.replaceState() and therefore rebuilds the whole tree.
    //
    // isVoid() is the ONLY correct guard and toString() the only correct read.
    // getStateInformation writes a STRING var, but even a bool or int written
    // there would not survive: the XML round-trip does not preserve the type,
    // because NamedValueSet::setFromXmlAttributes rebuilds every property as
    // `var (value)` over the attribute STRING
    // (critical_valuetree_xml_roundtrip_loses_type). A pre-1.2.0 session has no
    // such property at all and the default (English) stands. languageIndex()
    // clamps anything that is not "fr" to 0, so a hand-edited value degrades to
    // English rather than to a bad index.
    //
    // The editor PULLS this through the getUiLanguage native fn at page init
    // rather than being pushed from here — a push would race the WebView's load.
    const juce::var lang = parameters.state.getProperty("uiLanguage");

    if (! lang.isVoid())
        uiLanguage.store(languageIndex(lang.toString()), std::memory_order_release);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconAnalogEQAudioProcessor();
}
