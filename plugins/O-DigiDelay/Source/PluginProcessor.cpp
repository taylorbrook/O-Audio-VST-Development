/*
   This file is part of O-DigiDelay, an Ouaricon Audio plugin.
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

    Ouaricon Digital Delay - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconDigitalDelayAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // time - Float (1.0-2000.0 ms, default: 500.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "time", 1 },
        "Time",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 1.0f),
        500.0f,
        "ms"
    ));

    // sync - Bool (default: false)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "sync", 1 },
        "Sync",
        false
    ));

    // division - Choice (12 options, default: 1 "1/8")
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "division", 1 },
        "Division",
        juce::StringArray { "1/4", "1/8", "1/16", "1/4D", "1/8D", "1/16D",
                           "1/4T", "1/8T", "1/16T", "1/4(5)", "1/8(5)", "1/16(5)" },
        1  // Default: 1/8
    ));

    // feedback - Float (0.0-100.0%, default: 30.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1 },
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // spread - Float (0.0-100.0%, default: 0.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "spread", 1 },
        "Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        "%"
    ));

    // mod - Float (0.0-100.0%, default: 0.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mod", 1 },
        "Mod",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        "%"
    ));

    // wet - Float (0.0-100.0%, default: 30.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wet", 1 },
        "Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // dry - Float (0.0-100.0%, default: 100.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dry", 1 },
        "Dry",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        100.0f,
        "%"
    ));

    return layout;
}

OuariconDigitalDelayAudioProcessor::OuariconDigitalDelayAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-DigiDelay")
{
    timeParam     = parameters.getRawParameterValue("time");
    syncParam     = parameters.getRawParameterValue("sync");
    divisionParam = parameters.getRawParameterValue("division");
    feedbackParam = parameters.getRawParameterValue("feedback");
    spreadParam   = parameters.getRawParameterValue("spread");
    modParam      = parameters.getRawParameterValue("mod");
    wetParam      = parameters.getRawParameterValue("wet");
    dryParam      = parameters.getRawParameterValue("dry");
}

void OuariconDigitalDelayAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Calculate maximum delay buffer size (2000ms at up to 192kHz)
    // Reserve modulation depth (spread 15ms + mod 10ms = 25ms) PLUS a 5ms safety pad
    // so the max read index never reaches maximumDelayInSamples. Without the pad the read
    // lands exactly on the buffer edge at 48/96/192 kHz (WR-01).
    const double maxDelaySeconds = (2000.0 + 15.0 + 10.0 + 5.0) / 1000.0;
    const int maxDelaySamples = static_cast<int>(std::ceil(maxDelaySeconds * sampleRate));

    // Prepare delay lines with maximum size
    delayLineLeft.setMaximumDelayInSamples(maxDelaySamples);
    delayLineRight.setMaximumDelayInSamples(maxDelaySamples);
    delayLineLeft.prepare(spec);
    delayLineRight.prepare(spec);
    delayLineLeft.reset();
    delayLineRight.reset();

    // Prepare LFO (0.3Hz sine wave)
    lfo.prepare(spec);
    lfo.setFrequency(0.3f);
    lfo.initialise([](float x) { return std::sin(x); }, 128);

    // Reset smoothed values with 20ms ramp time
    const double rampTimeSamples = 0.02 * sampleRate;
    smoothedTimeMs.reset(rampTimeSamples);
    smoothedFeedback.reset(rampTimeSamples);
    smoothedSpread.reset(rampTimeSamples);
    smoothedMod.reset(rampTimeSamples);
    smoothedWet.reset(rampTimeSamples);
    smoothedDry.reset(rampTimeSamples);

    // Reset RMS meters with faster response (10ms)
    rmsLevelLeft.reset(sampleRate, 0.01);
    rmsLevelRight.reset(sampleRate, 0.01);

    // Clear feedback state
    feedbackLeft = 0.0f;
    feedbackRight = 0.0f;
}

void OuariconDigitalDelayAudioProcessor::releaseResources()
{
}

bool OuariconDigitalDelayAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    // Reject disabled/empty buses
    if (mainIn.isDisabled() || mainOut.isDisabled())
        return false;

    // Input and output layouts must match (no up/down-mixing)
    if (mainIn != mainOut)
        return false;

    // Accept mono->mono and stereo->stereo only
    return mainIn == juce::AudioChannelSet::mono()
        || mainIn == juce::AudioChannelSet::stereo();
}

void OuariconDigitalDelayAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Early return for empty buffers
    if (numSamples == 0 || numChannels == 0)
        return;

    // Calculate delay time (free mode or synced mode)
    float delayTimeMs = timeParam->load();
    bool isSync = syncParam->load() > 0.5f;

    if (isSync)
    {
        if (auto* playHead = getPlayHead())
        {
            if (auto positionInfo = playHead->getPosition(); positionInfo.hasValue())
            {
                if (auto bpm = positionInfo->getBpm(); bpm.hasValue() && *bpm > 0.0)
                {
                    int divisionIndex = juce::jlimit(0, 11, static_cast<int>(divisionParam->load()));
                    delayTimeMs = juce::jlimit(1.0f, 2000.0f,
                        static_cast<float>((60000.0 / *bpm) * subdivisionFactors[divisionIndex]));
                }
            }
        }
    }

    // Set smoothed parameter targets
    smoothedTimeMs.setTargetValue(delayTimeMs);
    smoothedFeedback.setTargetValue(feedbackParam->load() / 100.0f);
    smoothedSpread.setTargetValue(spreadParam->load() / 100.0f);
    smoothedMod.setTargetValue(modParam->load() / 100.0f);
    smoothedWet.setTargetValue(wetParam->load() / 100.0f);
    smoothedDry.setTargetValue(dryParam->load() / 100.0f);

    const float msToSamples = static_cast<float>(spec.sampleRate) / 1000.0f;
    float* leftChannel = numChannels >= 1 ? buffer.getWritePointer(0) : nullptr;
    float* rightChannel = numChannels >= 2 ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float currentDelayMs = smoothedTimeMs.getNextValue();
        float currentFeedback = juce::jlimit(0.0f, 0.95f, smoothedFeedback.getNextValue());
        float currentSpread = smoothedSpread.getNextValue();
        float currentMod = smoothedMod.getNextValue();
        float currentWet = smoothedWet.getNextValue();
        float currentDry = smoothedDry.getNextValue();

        float baseDelaySamples = currentDelayMs * msToSamples;
        float spreadSamples = currentSpread * 15.0f * msToSamples;
        float modSamples = currentMod * 10.0f * msToSamples * lfo.processSample(0.0f);

        float leftDelaySamples = juce::jmax(1.0f, baseDelaySamples + modSamples);
        float rightDelaySamples = juce::jmax(1.0f, baseDelaySamples + spreadSamples + modSamples);

        if (leftChannel != nullptr)
        {
            float drySample = leftChannel[sample];
            delayLineLeft.pushSample(0, drySample + feedbackLeft);
            float delayedSample = delayLineLeft.popSample(0, leftDelaySamples);
            feedbackLeft = delayedSample * currentFeedback;
            if (! std::isfinite(feedbackLeft)) feedbackLeft = 0.0f; // break NaN/Inf recirculation (ScopedNoDenormals doesn't catch these)
            leftChannel[sample] = delayedSample * currentWet + drySample * currentDry;
        }

        if (rightChannel != nullptr)
        {
            float drySample = rightChannel[sample];
            delayLineRight.pushSample(0, drySample + feedbackRight);
            float delayedSample = delayLineRight.popSample(0, rightDelaySamples);
            feedbackRight = delayedSample * currentFeedback;
            if (! std::isfinite(feedbackRight)) feedbackRight = 0.0f; // break NaN/Inf recirculation (ScopedNoDenormals doesn't catch these)
            rightChannel[sample] = delayedSample * currentWet + drySample * currentDry;
        }
    }

    // Calculate RMS levels for output meter
    if (leftChannel != nullptr)
    {
        float sumSquares = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sumSquares += leftChannel[i] * leftChannel[i];
        rmsLevelLeft.setTargetValue(std::sqrt(sumSquares / static_cast<float>(numSamples)));
        rmsLevelLeft.skip(numSamples);
        // Publish smoothed value through an atomic so the message-thread meter getter
        // never reads the LinearSmoothedValue internals across threads (WR-06).
        rmsMeterLeft.store(rmsLevelLeft.getCurrentValue(), std::memory_order_relaxed);
    }

    if (rightChannel != nullptr)
    {
        float sumSquares = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sumSquares += rightChannel[i] * rightChannel[i];
        rmsLevelRight.setTargetValue(std::sqrt(sumSquares / static_cast<float>(numSamples)));
        rmsLevelRight.skip(numSamples);
        rmsMeterRight.store(rmsLevelRight.getCurrentValue(), std::memory_order_relaxed);
    }
}

double OuariconDigitalDelayAudioProcessor::getTailLengthSeconds() const
{
    float feedbackValue = feedbackParam->load() / 100.0f;
    feedbackValue = juce::jlimit(0.0f, 0.95f, feedbackValue);

    // If no feedback, no tail
    if (feedbackValue < 0.01f)
        return 0.0;

    // Calculate maximum possible delay time (2000ms)
    const double maxDelaySeconds = 2.0;

    // Tail length formula: maxDelay * (1 / (1 - feedback))
    // This calculates how long it takes for the delay to decay to silence
    double tailLength = maxDelaySeconds * (1.0 / (1.0 - static_cast<double>(feedbackValue)));

    // Cap at reasonable maximum (30 seconds)
    return juce::jmin(tailLength, 30.0);
}

juce::AudioProcessorEditor* OuariconDigitalDelayAudioProcessor::createEditor()
{
    return new OuariconDigitalDelayAudioProcessorEditor(*this);
}

void OuariconDigitalDelayAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Use preset manager for state serialization (includes APVTS + current preset name)
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OuariconDigitalDelayAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    // Use preset manager for state restoration
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconDigitalDelayAudioProcessor();
}
