/*
   This file is part of O-Comp, an Ouaricon Audio plugin.
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

    O-Comp - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OCompAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // threshold - Compression threshold level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "threshold", 1 },
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -20.0f,
        "dB"
    ));

    // ratio - Compression ratio
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ratio", 1 },
        "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
        2.0f,
        ":1"
    ));

    // attack_time - Attack time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attack_time", 1 },
        "Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f),
        10.0f,
        "ms"
    ));

    // release_time - Release time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "release_time", 1 },
        "Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f),
        100.0f,
        "ms"
    ));

    // knee - Knee width
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "knee", 1 },
        "Knee",
        juce::NormalisableRange<float>(0.0f, 20.0f, 0.1f),
        6.0f,
        "dB"
    ));

    // output_gain - Output makeup gain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output_gain", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // auto_gain - Automatic makeup gain
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "auto_gain", 1 },
        "Auto Gain",
        false
    ));

    return layout;
}

OCompAudioProcessor::OCompAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Comp")
{
    thresholdParam = parameters.getRawParameterValue("threshold");
    ratioParam = parameters.getRawParameterValue("ratio");
    attackParam = parameters.getRawParameterValue("attack_time");
    releaseParam = parameters.getRawParameterValue("release_time");
    kneeParam = parameters.getRawParameterValue("knee");
    outputGainParam = parameters.getRawParameterValue("output_gain");
    autoGainParam = parameters.getRawParameterValue("auto_gain");

    // Initialize factory presets (only writes files if they don't already exist on disk)
    presetManager.initializeFactoryPresets({
        // Gentle Glue - subtle bus compression with soft knee
        { "Gentle Glue", {
            { "threshold",    0.8f },      // -12 dB
            { "ratio",        0.05263f },   // 2:1
            { "attack_time",  0.29930f },   // 30 ms
            { "release_time", 0.29293f },   // 300 ms
            { "knee",         0.6f },       // 12 dB
            { "output_gain",  0.33333f },   // 0 dB
            { "auto_gain",    1.0f },       // on
        }, {} },
        // Vocal Smooth - medium vocal compression with auto-gain
        { "Vocal Smooth", {
            { "threshold",    0.7f },       // -18 dB
            { "ratio",        0.10526f },   // 3:1
            { "attack_time",  0.14915f },   // 15 ms
            { "release_time", 0.19192f },   // 200 ms
            { "knee",         0.4f },       // 8 dB
            { "output_gain",  0.33333f },   // 0 dB
            { "auto_gain",    1.0f },       // on
        }, {} },
        // Drum Punch - punchy drums with fast release
        { "Drum Punch", {
            { "threshold",    0.75f },      // -15 dB
            { "ratio",        0.15789f },   // 4:1
            { "attack_time",  0.00901f },   // 1 ms
            { "release_time", 0.04040f },   // 50 ms
            { "knee",         0.2f },       // 4 dB
            { "output_gain",  0.33333f },   // 0 dB
            { "auto_gain",    1.0f },       // on
        }, {} },
        // Bass Control - tight bass control with moderate ratio
        { "Bass Control", {
            { "threshold",    0.66667f },   // -20 dB
            { "ratio",        0.15789f },   // 4:1
            { "attack_time",  0.04905f },   // 5 ms
            { "release_time", 0.14141f },   // 150 ms
            { "knee",         0.3f },       // 6 dB
            { "output_gain",  0.33333f },   // 0 dB
            { "auto_gain",    1.0f },       // on
        }, {} },
        // Mastering Touch - light mastering-style compression
        { "Mastering Touch", {
            { "threshold",    0.83333f },   // -10 dB
            { "ratio",        0.02632f },   // 1.5:1
            { "attack_time",  0.19920f },   // 20 ms
            { "release_time", 0.39394f },   // 400 ms
            { "knee",         0.5f },       // 10 dB
            { "output_gain",  0.33333f },   // 0 dB
            { "auto_gain",    1.0f },       // on
        }, {} },
        // Aggressive Smash - heavy limiting-style compression
        { "Aggressive Smash", {
            { "threshold",    0.5f },       // -30 dB
            { "ratio",        0.47368f },   // 10:1
            { "attack_time",  0.00400f },   // 0.5 ms
            { "release_time", 0.07071f },   // 80 ms
            { "knee",         0.1f },       // 2 dB
            { "output_gain",  0.33333f },   // 0 dB
            { "auto_gain",    1.0f },       // on
        }, {} },
        // Natural Dynamics - transparent compression for natural sources
        { "Natural Dynamics", {
            { "threshold",    0.73333f },   // -16 dB
            { "ratio",        0.07895f },   // 2.5:1
            { "attack_time",  0.24925f },   // 25 ms
            { "release_time", 0.24242f },   // 250 ms
            { "knee",         0.7f },       // 14 dB
            { "output_gain",  0.33333f },   // 0 dB
            { "auto_gain",    1.0f },       // on
        }, {} },
        // Parallel Crush - heavy compression for parallel processing
        { "Parallel Crush", {
            { "threshold",    0.33333f },   // -40 dB
            { "ratio",        1.0f },       // 20:1
            { "attack_time",  0.0f },       // 0.1 ms (minimum)
            { "release_time", 0.05051f },   // 60 ms
            { "knee",         0.0f },       // 0 dB (hard knee)
            { "output_gain",  0.66667f },   // 12 dB
            { "auto_gain",    0.0f },       // off
        }, {} },
    });
}

void OCompAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Initialize envelope state
    envelopeDB = -60.0f;

    // Calculate initial coefficients
    updateCoefficients(attackParam->load(), releaseParam->load(), sampleRate);

    // Smooth makeup/output gain over 20 ms; snap to the initial value so the first
    // block doesn't ramp up from silence.
    smoothedMakeup.reset(sampleRate, 0.02);
    smoothedMakeup.setCurrentAndTargetValue(computeMakeupGainLinear());
}

void OCompAudioProcessor::releaseResources()
{
    // Reset envelope state
    envelopeDB = -60.0f;
}

void OCompAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic, real-time safe — pointers cached in constructor)
    float thresholdDB = thresholdParam->load();
    float ratio = ratioParam->load();
    float attackTimeMs = attackParam->load();
    float releaseTimeMs = releaseParam->load();
    float kneeDB = kneeParam->load();

    // Update attack/release coefficients (lightweight calculation, real-time safe)
    updateCoefficients(attackTimeMs, releaseTimeMs, currentSampleRate);

    // Makeup gain (auto-gain + output_gain): smoothed per sample to avoid zipper on
    // automation / auto-gain toggles. See computeMakeupGainLinear().
    smoothedMakeup.setTargetValue(computeMakeupGainLinear());

    // Process audio (per-sample loop for accurate envelope following)
    const int numSamples = buffer.getNumSamples();
    // channelPtrs is sized for 2; cap all channel loops to what we actually populated
    // so a >2-channel layout can never dereference a null pointer.
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    // Track peak levels for metering
    float peakInputLevel = 0.0f;
    float peakOutputLevel = 0.0f;
    float peakGainReduction = 0.0f;

    // Hoist channel pointers outside per-sample loop
    float* channelPtrs[2] = {};
    for (int ch = 0; ch < numChannels && ch < 2; ++ch)
        channelPtrs[ch] = buffer.getWritePointer(ch);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Stereo-linked detection: use max of all channels
        float maxInputLevel = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float inputLevel = std::abs(channelPtrs[ch][sample]);
            maxInputLevel = std::max(maxInputLevel, inputLevel);
        }

        // Track peak input for metering
        peakInputLevel = std::max(peakInputLevel, maxInputLevel);

        // Convert to dB
        float inputLevelDBLocal = juce::Decibels::gainToDecibels(maxInputLevel, -60.0f);

        // Envelope follower (attack/release ballistics)
        if (inputLevelDBLocal > envelopeDB)
            envelopeDB += (inputLevelDBLocal - envelopeDB) * attackCoeff;
        else
            envelopeDB += (inputLevelDBLocal - envelopeDB) * releaseCoeff;

        // Calculate gain reduction
        float gainReductionDBLocal = calculateGainReduction(envelopeDB, thresholdDB, ratio, kneeDB);
        peakGainReduction = std::max(peakGainReduction, gainReductionDBLocal);

        // Convert to linear gain (makeup smoothed per sample to de-zipper automation)
        float makeupGainLinear = smoothedMakeup.getNextValue();
        float gainLinear = juce::Decibels::decibelsToGain(-gainReductionDBLocal) * makeupGainLinear;

        // Apply same gain to all channels (stereo-linked)
        float maxOutputLevel = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            channelPtrs[ch][sample] *= gainLinear;
            maxOutputLevel = std::max(maxOutputLevel, std::abs(channelPtrs[ch][sample]));
        }
        peakOutputLevel = std::max(peakOutputLevel, maxOutputLevel);
    }

    // Update atomic meter values (thread-safe for UI access)
    inputLevelDB.store(juce::Decibels::gainToDecibels(peakInputLevel, -60.0f));
    outputLevelDB.store(juce::Decibels::gainToDecibels(peakOutputLevel, -60.0f));
    currentGainReductionDB.store(peakGainReduction);
    currentEnvelopeDB.store(envelopeDB);
}

juce::AudioProcessorEditor* OCompAudioProcessor::createEditor()
{
    return new OCompAudioProcessorEditor(*this);
}

void OCompAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Use preset manager for complete state (includes current preset name)
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OCompAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}

// DSP Helper Methods
float OCompAudioProcessor::calculateGainReduction(float inputLevel, float thresholdDB,
                                                          float ratio, float kneeDB)
{
    float x = inputLevel - thresholdDB;

    // Hard knee (or numerically negligible knee): the soft-knee branch divides by
    // (2 * kneeDB), so a zero/near-zero knee (e.g. the "Parallel Crush" factory preset)
    // would evaluate 0/0 -> NaN when the envelope lands exactly on the threshold.
    // Treat it as a hard knee to keep the audio buffer finite.
    if (kneeDB <= 1.0e-6f)
        return x > 0.0f ? x - (x / ratio) : 0.0f;

    if (x < -kneeDB / 2.0f)
    {
        // Below knee - no compression
        return 0.0f;
    }
    else if (x > kneeDB / 2.0f)
    {
        // Above knee - full compression
        return x - (x / ratio);
    }
    else
    {
        // Inside knee - standard quadratic soft-knee formula
        float kneeInput = x + kneeDB / 2.0f;
        return (1.0f - 1.0f / ratio) * (kneeInput * kneeInput) / (2.0f * kneeDB);
    }
}

void OCompAudioProcessor::updateCoefficients(float attackTimeMs, float releaseTimeMs, double sampleRate)
{
    // Calculate attack/release coefficients using exponential formula
    // coeff = 1 - exp(-1 / (timeMs * sampleRate / 1000))
    attackCoeff = 1.0f - std::exp(-1.0f / (attackTimeMs * static_cast<float>(sampleRate) / 1000.0f));
    releaseCoeff = 1.0f - std::exp(-1.0f / (releaseTimeMs * static_cast<float>(sampleRate) / 1000.0f));
}

float OCompAudioProcessor::computeMakeupGainLinear() const
{
    float thresholdDB = thresholdParam->load();
    float ratio = ratioParam->load();
    float outputGainDB = outputGainParam->load();
    bool autoGainEnabled = autoGainParam->load() > 0.5f;

    float autoGainDB = 0.0f;
    if (autoGainEnabled)
        autoGainDB = -thresholdDB * (1.0f - 1.0f / ratio) * 0.5f;

    return juce::Decibels::decibelsToGain(autoGainDB + outputGainDB);
}

bool OCompAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Only mono or stereo main I/O; reject wider layouts so the stereo-linked
    // detection path can never index past the 2-slot channel-pointer array.
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output (in-place stereo-linked processing).
    return layouts.getMainInputChannelSet() == mainOut;
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OCompAudioProcessor();
}
