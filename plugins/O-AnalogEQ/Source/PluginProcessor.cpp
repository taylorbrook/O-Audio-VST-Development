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
#include "PluginEditor.h"

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

    // Force a coefficient (re)build on the first block after prepare (also covers a
    // sample-rate change); Q sentinels reset so the first block rebuilds the bells too.
    lastLmfQ = lastHmfQ = -1;
    coeffsInitialised = false;
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

    // Process audio: LF -> LMF -> HMF -> HF -> Saturation -> Output Gain
    juce::dsp::AudioBlock<float> block(buffer);

    auto processChunk = [&](size_t start, size_t len)
    {
        auto sub = block.getSubBlock(start, len);
        juce::dsp::ProcessContextReplacing<float> context(sub);

        if (lfOn)  lfFilter.process(context);
        if (lmfOn) lmfFilter.process(context);
        if (hmfOn) hmfFilter.process(context);
        if (hfOn)  hfFilter.process(context);

        if (analogOn) saturation.process(context);
        outputGain.process(context);
    };

    const int numSamples = buffer.getNumSamples();

    if (! (lfMoving || lmfMoving || hmfMoving || hfMoving))
    {
        // Steady state: coefficients unchanged, run the whole block in a single pass.
        if (numSamples > 0)
            processChunk(0, static_cast<size_t>(numSamples));
    }
    else
    {
        for (int pos = 0; pos < numSamples; pos += kSmoothingBlock)
        {
            const int n = juce::jmin(kSmoothingBlock, numSamples - pos);

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

            processChunk(static_cast<size_t>(pos), static_cast<size_t>(n));
        }
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

juce::AudioProcessorEditor* OuariconAnalogEQAudioProcessor::createEditor()
{
    return new OuariconAnalogEQAudioProcessorEditor(*this);
}

void OuariconAnalogEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OuariconAnalogEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        presetManager.setStateFromXml(xml.get());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconAnalogEQAudioProcessor();
}
