/*
   This file is part of O-FreqPulse, an Ouaricon Audio plugin.
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

    O-FreqPulse - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout Creation
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OFreqPulseAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global Parameters Group
    auto globalGroup = std::make_unique<juce::AudioProcessorParameterGroup>("global", "Global", "|");

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f));

    globalGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "steps", 1 },
        "Steps",
        2, 32, 16));

    globalGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "rate", 1 },
        "Rate",
        juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T", "1/4D", "1/8D" },
        4));  // Default index 4 = "1/16"

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "swing", 1 },
        "Swing",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attack", 1 },
        "Attack",
        juce::NormalisableRange<float>(0.0f, 500.0f, 0.1f, 0.4f),
        5.0f));

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "release", 1 },
        "Release",
        juce::NormalisableRange<float>(0.0f, 500.0f, 0.1f, 0.4f),
        5.0f));

    // Crossover frequency parameters (3 crossover points for 4 bands)
    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "crossover_1", 1 },
        "Crossover 1 (Sub|Low)",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        120.0f));

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "crossover_2", 1 },
        "Crossover 2 (Low|Mid)",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        500.0f));

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "crossover_3", 1 },
        "Crossover 3 (Mid|High)",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        4000.0f));

    // Frequency boundary parameters
    // WR-10: display-only — these bound the WebView grid's frequency axis and are NOT
    // read by the DSP (never referenced in processBlock). Marked non-automatable so the
    // host doesn't advertise automation lanes that change nothing audible. IDs and saved
    // state are unchanged (not a breaking change); the UI still reads them via SliderState.
    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "freq_low", 1 },
        "Freq Low",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        20.0f,
        juce::AudioParameterFloatAttributes().withAutomatable(false)));

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "freq_high", 1 },
        "Freq High",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        20000.0f,
        juce::AudioParameterFloatAttributes().withAutomatable(false)));

    layout.add(std::move(globalGroup));

    // Per-Band Parameters (4 bands)
    const juce::String bandNames[4] = { "Sub", "Low", "Mid", "High" };

    for (int n = 0; n < 4; ++n)
    {
        juce::String bandID = "band" + juce::String(n);
        juce::String bandName = "Band " + juce::String(n + 1) + " (" + bandNames[n] + ")";

        auto bandGroup = std::make_unique<juce::AudioProcessorParameterGroup>(
            bandID, bandName, "|");

        // Band control parameters
        bandGroup->addChild(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { bandID + "_enable", 1 },
            bandName + " Enable",
            true));

        bandGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandID + "_depth", 1 },
            bandName + " Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
            1.0f));

        // v1.7.0: Per-band rate override (Global = follow global rate)
        bandGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { bandID + "_rate", 1 },
            bandName + " Rate",
            juce::StringArray { "Global", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T", "1/4D", "1/8D" },
            0));  // Default index 0 = "Global"

        bandGroup->addChild(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { bandID + "_euc_on", 1 },
            bandName + " Euclidean",
            false));

        bandGroup->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { bandID + "_euc_steps", 1 },
            bandName + " Euc Steps",
            1, 32, 16));

        bandGroup->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { bandID + "_euc_pulses", 1 },
            bandName + " Euc Pulses",
            1, 32, 8));

        bandGroup->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { bandID + "_euc_offset", 1 },
            bandName + " Euc Offset",
            0, 31, 0));

        // v1.14.0: Per-band phase offset (shifts pattern read position)
        bandGroup->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { bandID + "_phase_offset", 1 },
            bandName + " Phase Offset",
            0, 31, 0));

        // v1.15.0: Per-band step count (0 = follow global, 2-32 = independent loop length)
        bandGroup->addChild(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { bandID + "_steps", 1 },
            bandName + " Steps",
            0, 32, 0));

        // Step grid parameters (32 per band) — velocity floats (0=off, 1=full)
        for (int m = 0; m < 32; ++m)
        {
            juce::String stepID = "step_b" + juce::String(n) + "_s" + juce::String(m);
            juce::String stepName = "B" + juce::String(n + 1) + " Step " + juce::String(m + 1);

            bandGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { stepID, 1 },
                stepName,
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                0.0f));
        }

        layout.add(std::move(bandGroup));
    }

    return layout;
}

//==============================================================================
// Constructor & Destructor
//==============================================================================
OFreqPulseAudioProcessor::OFreqPulseAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-FreqPulse")
{
    // Cache global parameter pointers
    mixParam = parameters.getRawParameterValue("mix");
    stepsParam = parameters.getRawParameterValue("steps");
    rateParam = parameters.getRawParameterValue("rate");
    swingParam = parameters.getRawParameterValue("swing");
    attackParam = parameters.getRawParameterValue("attack");
    releaseParam = parameters.getRawParameterValue("release");

    // Cache crossover parameter pointers
    crossover1Param = parameters.getRawParameterValue("crossover_1");
    crossover2Param = parameters.getRawParameterValue("crossover_2");
    crossover3Param = parameters.getRawParameterValue("crossover_3");

    // Cache per-band parameter pointers
    for (int n = 0; n < 4; ++n)
    {
        juce::String bandID = "band" + juce::String(n);

        bandParams[n].enable = parameters.getRawParameterValue(bandID + "_enable");
        bandParams[n].depth = parameters.getRawParameterValue(bandID + "_depth");
        bandParams[n].rate = parameters.getRawParameterValue(bandID + "_rate");
        bandParams[n].eucOn = parameters.getRawParameterValue(bandID + "_euc_on");
        bandParams[n].eucSteps = parameters.getRawParameterValue(bandID + "_euc_steps");
        bandParams[n].eucPulses = parameters.getRawParameterValue(bandID + "_euc_pulses");
        bandParams[n].eucOffset = parameters.getRawParameterValue(bandID + "_euc_offset");
        bandParams[n].phaseOffset = parameters.getRawParameterValue(bandID + "_phase_offset");
        bandParams[n].bandSteps = parameters.getRawParameterValue(bandID + "_steps");

        // Cache step grid parameters (32 per band)
        for (int m = 0; m < 32; ++m)
        {
            juce::String stepID = "step_b" + juce::String(n) + "_s" + juce::String(m);
            bandParams[n].stepStates[m] = parameters.getRawParameterValue(stepID);
        }
    }

    // v1.6.0: Initialize factory presets using preset manager
    initializeFactoryPresets();
}

OFreqPulseAudioProcessor::~OFreqPulseAudioProcessor()
{
}

//==============================================================================
// Audio Processing
//==============================================================================
void OFreqPulseAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Configure Linkwitz-Riley crossover filter bank
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    crossoverMid.prepare(spec);
    crossoverLow.prepare(spec);
    crossoverHigh.prepare(spec);

    // Set initial crossover frequencies
    updateCrossoverFrequencies();

    // Configure DryWetMixer
    dryWetMixer.prepare(spec);
    dryWetMixer.reset();

    // Configure gain envelopes (one per band, separate attack/release)
    {
        float attackMs = std::max(2.0f, attackParam->load());
        float releaseMs = std::max(2.0f, releaseParam->load());
        for (int band = 0; band < 4; ++band)
        {
            bandEnvelopes[band].setRates(sampleRate, attackMs, releaseMs);
            bandEnvelopes[band].setCurrentAndTargetValue(1.0f);
            bandGainFiltered[band] = 1.0f;
        }
        lastAttackMs = attackMs;
        lastReleaseMs = releaseMs;
    }

    // One-pole lowpass coefficient to soften linear ramp corners (~1.5ms time constant)
    gainFilterCoeff = 1.0f - std::exp(-1.0f / (0.0015f * static_cast<float>(sampleRate)));

    // Generate initial Euclidean patterns
    updateEuclideanPatterns();

    // Reset step sequencer
    currentStep = 0;
    lastPpqPosition = -1.0;

    // IIR crossover filters have zero latency
    setLatencySamples(0);
}

void OFreqPulseAudioProcessor::releaseResources()
{
    // Reset filters
    dryWetMixer.reset();
    crossoverMid.reset();
    crossoverLow.reset();
    crossoverHigh.reset();

    // Reset step tracking
    currentStep = 0;
    lastPpqPosition = -1.0;
}

//==============================================================================
// Helper Methods
//==============================================================================

void OFreqPulseAudioProcessor::updateCrossoverFrequencies()
{
    float c1 = crossover1Param->load();
    float c2 = crossover2Param->load();
    float c3 = crossover3Param->load();

    // WR-04: clamp below Nyquist before setCutoffFrequency(). LinkwitzRileyFilter computes
    // g = tan(pi*f/fs); at f == fs/2 that is +inf → NaN, and just past Nyquist g goes negative
    // (unstable). Cannot occur at 44.1/48/96 kHz (Nyquist > 20 kHz max cutoff) but does bite
    // sub-~40 kHz sample rates (e.g. a 22050 Hz offline render with crossover_3 raised high).
    const float nyquist = 0.49f * static_cast<float>(currentSampleRate);
    c1 = juce::jmin(c1, nyquist);
    c2 = juce::jmin(c2, nyquist);
    c3 = juce::jmin(c3, nyquist);

    // Sort to guarantee ordering (handles automation edge cases)
    if (c1 > c2) std::swap(c1, c2);
    if (c2 > c3) std::swap(c2, c3);
    if (c1 > c2) std::swap(c1, c2);

    // Binary tree: split at c2, then split halves at c1 and c3
    crossoverMid.setCutoffFrequency(c2);
    crossoverLow.setCutoffFrequency(c1);
    crossoverHigh.setCutoffFrequency(c3);
}

std::array<bool, 32> OFreqPulseAudioProcessor::generateEuclidean(int steps, int pulses, int offset)
{
    std::array<bool, 32> pattern;
    pattern.fill(false);

    // Clamp pulses to steps
    if (pulses > steps) pulses = steps;
    if (pulses <= 0 || steps <= 0) return pattern;

    // Bresenham bucket-fill algorithm
    int bucket = 0;
    for (int i = 0; i < steps; ++i)
    {
        bucket += pulses;
        if (bucket >= steps)
        {
            bucket -= steps;
            pattern[i] = true;
        }
    }

    // Apply rotation offset
    if (offset > 0 && offset < steps)
    {
        std::rotate(pattern.begin(), pattern.begin() + offset, pattern.begin() + steps);
    }

    return pattern;
}

void OFreqPulseAudioProcessor::updateEuclideanPatterns()
{
    for (int band = 0; band < 4; ++band)
    {
        int eucSteps = static_cast<int>(bandParams[band].eucSteps->load());
        int pulses = static_cast<int>(bandParams[band].eucPulses->load());
        int offset = static_cast<int>(bandParams[band].eucOffset->load());

        // v1.15.0: Per-band step count overrides euc_steps when set
        int bSteps = static_cast<int>(bandParams[band].bandSteps->load());
        int effectiveSteps = (bSteps >= 2) ? bSteps : eucSteps;

        euclideanPatterns[band] = generateEuclidean(effectiveSteps, pulses, offset);
    }
}

int OFreqPulseAudioProcessor::calculateCurrentStep(double ppq, int numSteps, int rateIndex, float swing)
{
    // IN-06: self-safe guard against modulo-by-zero. All current callers clamp numSteps to
    // [2,32], but this makes the function safe for any future caller passing 0.
    if (numSteps <= 0)
        return 0;

    // PPQ values for rate options
    static constexpr double ppqPerStep[] = {
        4.0,     // 1/1 (whole note)
        2.0,     // 1/2
        1.0,     // 1/4
        0.5,     // 1/8
        0.25,    // 1/16
        0.125,   // 1/32
        0.333333,// 1/8T (triplet)
        0.166666,// 1/16T
        1.5,     // 1/4D (dotted)
        0.75     // 1/8D
    };

    if (rateIndex < 0 || rateIndex > 9)
        rateIndex = 4;  // Default to 1/16

    double stepLength = ppqPerStep[rateIndex];

    // Apply swing to odd steps (delay by swing amount)
    double swingOffset = 0.0;
    int rawStep = static_cast<int>(ppq / stepLength);

    if (swing > 0.01f && (rawStep % 2) == 1)
    {
        swingOffset = stepLength * swing * 0.5;  // Delay odd steps
    }

    double adjustedPpq = ppq - swingOffset;
    int step = static_cast<int>(adjustedPpq / stepLength) % numSteps;

    return step < 0 ? 0 : step;
}

float OFreqPulseAudioProcessor::getTargetGainForBand(int bandIndex, int currentStep, int numSteps)
{
    // Check if band is enabled
    bool enabled = bandParams[bandIndex].enable->load() > 0.5f;
    if (!enabled)
        return 1.0f;  // Passthrough when disabled

    // v1.14.0: Apply per-band phase offset to shift pattern read position
    int phaseOffset = static_cast<int>(bandParams[bandIndex].phaseOffset->load());
    int adjustedStep = (currentStep + phaseOffset) % numSteps;

    // Determine velocity (0.0 = off, 1.0 = full)
    float velocity;
    bool eucOn = bandParams[bandIndex].eucOn->load() > 0.5f;

    if (eucOn)
    {
        // v1.15.0: Per-band step count overrides euc_steps when set
        int eucSteps = static_cast<int>(bandParams[bandIndex].eucSteps->load());
        int bSteps = static_cast<int>(bandParams[bandIndex].bandSteps->load());
        int effectiveEucSteps = (bSteps >= 2) ? bSteps : eucSteps;

        int wrappedStep = adjustedStep % effectiveEucSteps;
        velocity = euclideanPatterns[bandIndex][wrappedStep] ? 1.0f : 0.0f;
    }
    else
    {
        // Manual step grid: read velocity float directly
        velocity = bandParams[bandIndex].stepStates[adjustedStep]->load();
    }

    // Interpolate gain: vel=0 → (1-depth), vel=1 → 1.0
    float depth = bandParams[bandIndex].depth->load();
    return (1.0f - depth) + velocity * depth;
}

//==============================================================================
// Audio Processing
//==============================================================================

void OFreqPulseAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);  // Limit to stereo

    if (numSamples == 0)
        return;

    // Read global parameters
    float mix = mixParam->load();
    int numSteps = juce::jlimit(2, 32, static_cast<int>(stepsParam->load()));
    int rateIndex = static_cast<int>(rateParam->load());
    float swing = swingParam->load();
    float attackMs = attackParam->load();
    float releaseMs = releaseParam->load();

    // Check for parameter changes
    bool crossoversChanged = false;
    bool euclideanParamsChanged = false;

    {
        float c[3] = { crossover1Param->load(), crossover2Param->load(), crossover3Param->load() };
        for (int i = 0; i < 3; ++i)
        {
            if (std::abs(c[i] - lastCrossovers[i]) > 0.1f)
            {
                lastCrossovers[i] = c[i];
                crossoversChanged = true;
            }
        }
    }

    for (int band = 0; band < 4; ++band)
    {
        int eucSteps = static_cast<int>(bandParams[band].eucSteps->load());
        int eucPulses = static_cast<int>(bandParams[band].eucPulses->load());
        int eucOffset = static_cast<int>(bandParams[band].eucOffset->load());

        if (eucSteps != lastEuclideanParams[band][0] ||
            eucPulses != lastEuclideanParams[band][1] ||
            eucOffset != lastEuclideanParams[band][2])
        {
            lastEuclideanParams[band][0] = eucSteps;
            lastEuclideanParams[band][1] = eucPulses;
            lastEuclideanParams[band][2] = eucOffset;
            euclideanParamsChanged = true;
        }
    }

    // Enforce minimum 2ms on both attack and release to prevent instant jumps
    attackMs = std::max(2.0f, attackMs);
    releaseMs = std::max(2.0f, releaseMs);

    if (std::abs(attackMs - lastAttackMs) > 0.01f || std::abs(releaseMs - lastReleaseMs) > 0.01f)
    {
        for (int band = 0; band < 4; ++band)
            bandEnvelopes[band].setRates(currentSampleRate, attackMs, releaseMs);
        lastAttackMs = attackMs;
        lastReleaseMs = releaseMs;
    }

    if (crossoversChanged)
        updateCrossoverFrequencies();

    // v1.15.0: Per-band step count change detection
    for (int band = 0; band < 4; ++band)
    {
        int bSteps = static_cast<int>(bandParams[band].bandSteps->load());
        if (bSteps != lastBandSteps[band])
        {
            lastBandSteps[band] = bSteps;
            euclideanParamsChanged = true;  // Triggers euclidean pattern regeneration
        }
    }

    if (euclideanParamsChanged)
        updateEuclideanPatterns();

    // Detect whether audio signal is present (peak-channel RMS check)
    bool signalPresent;
    {
        float rms = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            rms = juce::jmax(rms, buffer.getRMSLevel(ch, 0, numSamples));
        constexpr float silenceThreshold = 0.001f;  // ~-60 dB
        signalPresent = (rms >= silenceThreshold);
        hasAudioSignal.store(signalPresent);
    }

    // Get playhead position for tempo sync and sample-accurate step tracking
    bool gotValidPosition = false;
    double blockStartPpq = 0.0;
    double ppqPerSample = 0.0;
    juce::AudioPlayHead* playHead = getPlayHead();

    if (playHead != nullptr)
    {
        if (auto posInfo = playHead->getPosition())
        {
            bool isPlaying = posInfo->getIsPlaying();

            if (isPlaying && posInfo->getPpqPosition().hasValue())
            {
                blockStartPpq = *posInfo->getPpqPosition();
                gotValidPosition = true;

                if (posInfo->getBpm().hasValue())
                {
                    double bpm = *posInfo->getBpm();
                    ppqPerSample = bpm / (60.0 * currentSampleRate);
                }
                else
                {
                    ppqPerSample = 120.0 / (60.0 * currentSampleRate);
                }
            }
        }
    }

    // Fallback: free-running PPQ when no valid host position (e.g., Standalone)
    if (!gotValidPosition && signalPresent)
    {
        ppqPerSample = 120.0 / (60.0 * currentSampleRate);
        blockStartPpq = lastPpqPosition >= 0.0 ? lastPpqPosition : 0.0;
        gotValidPosition = true;
    }

    // v1.7.0: Per-band rate resolution — determine effective rate index per band
    int bandRateIndex[4];
    // v1.15.0: Per-band effective step count (0 or 1 = follow global, 2-32 = independent)
    int bandEffSteps[4];
    for (int band = 0; band < 4; ++band)
    {
        int bandRateChoice = static_cast<int>(bandParams[band].rate->load());
        // 0 = Global (use global rate), 1-10 = specific rate (offset by 1)
        bandRateIndex[band] = (bandRateChoice == 0) ? rateIndex : (bandRateChoice - 1);

        int bSteps = static_cast<int>(bandParams[band].bandSteps->load());
        bandEffSteps[band] = (bSteps >= 2) ? juce::jlimit(2, 32, bSteps) : numSteps;
    }

    // Set initial step and gain targets at block start
    int bandSteps[4] = { 0, 0, 0, 0 };

    if (gotValidPosition && signalPresent)
    {
        // Global step (internal tracking for per-sample change detection)
        currentStep = calculateCurrentStep(blockStartPpq, numSteps, rateIndex, swing);

        // Per-band step positions (using per-band step count)
        for (int band = 0; band < 4; ++band)
        {
            bandSteps[band] = calculateCurrentStep(blockStartPpq, bandEffSteps[band], bandRateIndex[band], swing);
            bandStepAtomics[band].store(bandSteps[band]);
            float targetGain = getTargetGainForBand(band, bandSteps[band], bandEffSteps[band]);
            bandEnvelopes[band].setTargetValue(targetGain);
        }
    }

    // Push dry samples to mixer
    // WR-05: DryWetMixer is prepared for 2 channels; restrict the block to the processed
    // channel count so a host handing us a >2-channel buffer can't drive pushDry/mixWet past
    // the 2-wide internal dry buffer (OOB). Unreachable with the fixed stereo bus, but guarded.
    juce::dsp::AudioBlock<float> block(buffer);
    auto mixBlock = block.getSubsetChannelBlock(0, static_cast<size_t>(numChannels));
    dryWetMixer.pushDrySamples(mixBlock);

    // Per-sample processing: LR crossover → per-band gain → sum
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Sample-accurate step tracking (per-band)
        if (gotValidPosition && signalPresent)
        {
            double samplePpq = blockStartPpq + ppqPerSample * static_cast<double>(sample);

            // Update per-band steps independently (using per-band step count)
            for (int band = 0; band < 4; ++band)
            {
                int stepAtSample = calculateCurrentStep(samplePpq, bandEffSteps[band], bandRateIndex[band], swing);
                if (stepAtSample != bandSteps[band])
                {
                    bandSteps[band] = stepAtSample;
                    bandStepAtomics[band].store(stepAtSample);
                    float targetGain = getTargetGainForBand(band, bandSteps[band], bandEffSteps[band]);
                    bandEnvelopes[band].setTargetValue(targetGain);
                }
            }
        }

        // Two-stage gain smoothing: custom BandEnvelope (linear ramp) → one-pole LPF (softens corners)
        float bandGainValues[4];
        for (int band = 0; band < 4; ++band)
        {
            float rawGain = bandEnvelopes[band].getNextValue();
            bandGainFiltered[band] += gainFilterCoeff * (rawGain - bandGainFiltered[band]);
            bandGainValues[band] = bandGainFiltered[band];
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float inputSample = buffer.getSample(ch, sample);

            // WR-09: unity-gain transparency at rest is APPROXIMATE. A single LR4 low+high sum
            // is an allpass, not identity; this binary tree (split c2 → split halves at c1/c3)
            // sums allpass_c1(lowHalf) + allpass_c3(highHalf), whose differing phase yields a
            // small magnitude ripple near c2 for CLOSELY-SPACED crossovers. Negligible at the
            // well-separated defaults (120/500/4000 Hz). Accepted for a creative rhythmic gate;
            // exact reconstruction would require per-path allpass compensation.

            // Stage 1: Split at crossover 2 (c2) into low-half and high-half
            float lowHalf, highHalf;
            crossoverMid.processSample(ch, inputSample, lowHalf, highHalf);

            // Stage 2: Split low-half at crossover 1 (c1) into Sub and Low
            float sub, low;
            crossoverLow.processSample(ch, lowHalf, sub, low);

            // Stage 3: Split high-half at crossover 3 (c3) into Mid and High
            float mid, high;
            crossoverHigh.processSample(ch, highHalf, mid, high);

            // Apply per-band gain and sum
            float outputSample = sub  * bandGainValues[0]
                               + low  * bandGainValues[1]
                               + mid  * bandGainValues[2]
                               + high * bandGainValues[3];

            buffer.setSample(ch, sample, outputSample);
        }
    }

    // Update lastPpqPosition for next block
    if (gotValidPosition)
        lastPpqPosition = blockStartPpq + ppqPerSample * static_cast<double>(numSamples);

    // Mix dry/wet
    dryWetMixer.setWetMixProportion(mix);
    dryWetMixer.mixWetSamples(mixBlock);
}

//==============================================================================
// Editor
//==============================================================================
juce::AudioProcessorEditor* OFreqPulseAudioProcessor::createEditor()
{
    return new OFreqPulseAudioProcessorEditor(*this);
}

//==============================================================================
// State Management
//==============================================================================
void OFreqPulseAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v1.18.0: the UI language rides the state tree as a non-parameter property.
    //
    // Written BEFORE the delegation, not after: getStateAsXml() starts from its
    // own parameters.copyState(), so a setProperty on the live tree afterwards
    // would never reach the XML that gets serialised.
    parameters.state.setProperty("uiLanguage",
                                 languageCode(uiLanguage.load(std::memory_order_acquire)),
                                 nullptr);

    // v1.6.0: Delegate to preset manager for full state (APVTS + custom)
    if (auto xml = presetManager.getStateAsXml())
    {
        // v1.5.0: Save tooltip enabled state
        xml->setAttribute("tooltipsEnabled", tooltipsEnabled.load(std::memory_order_acquire));

        copyXmlToBinary(*xml, destData);
    }
}

void OFreqPulseAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
    {
        // v1.5.0: Restore tooltip enabled state
        if (xmlState->hasAttribute("tooltipsEnabled"))
        {
            bool enabled = xmlState->getBoolAttribute("tooltipsEnabled", false);
            tooltipsEnabled.store(enabled, std::memory_order_release);
        }

        // v1.6.0: Delegate to preset manager for full state restoration
        presetManager.setStateFromXml(xmlState.get());

        // v1.18.0: read AFTER the restore — setStateFromXml replaces the whole
        // tree, so the property only exists on it once that has happened.
        //
        // isVoid() is the ONLY correct guard and toString() the only correct
        // read. NamedValueSet::setFromXmlAttributes rebuilds every property as a
        // var over the attribute STRING, so isBool()/isInt()/isString() type
        // predicates are false for every session ever saved
        // (critical_valuetree_xml_roundtrip_loses_type). A session written before
        // v1.18.0 has no such attribute at all and simply leaves the language
        // where it is — English on a fresh instance.
        const juce::var lang = parameters.state.getProperty("uiLanguage");

        if (! lang.isVoid())
            uiLanguage.store(languageIndex(lang.toString()), std::memory_order_release);
    }
}

//==============================================================================
// Factory Presets
//==============================================================================

// Preset names
static const juce::StringArray presetNames = {
    "Init",                    // 0: Default starting point
    "Classic Sidechain",       // 1: Sub solid, mids pump with velocity accents
    "Trance Gate 16th",        // 2: Velocity groove with phase-shifted highs
    "Dubstep Pulse",           // 3: Heavy sub gate with per-band rates
    "Ambient Shimmer",         // 4: Slow phase-shifted highs with swells
    "Polymetric Machine",      // 5: Independent step counts and rates per band
    "Bass Foundation",         // 6: Sub always on, others gated with velocity
    "Hi-Hat Chop",             // 7: Realistic hi-hat velocity pattern
    "Phase Cascade",           // 8: Same pattern, staggered phase offsets
    "Euclidean Groove",        // 9: Musical ratios with per-band rates
    "Half-Time Feel",          // 10: Slow dramatic swells with polymetric highs
    "Triplet Bounce"           // 11: Triplet timing with phase shifts
};

int OFreqPulseAudioProcessor::getNumPrograms()
{
    // WR-11: report a single program so the DAW's native program menu isn't populated with
    // 12 names that setCurrentProgram() intentionally does NOT load (loading there would
    // clobber DAW state restoration/automation). Presets are managed via the WebView bar.
    // (numPresets stays 12 for the factory-capture loop and getProgramName bounds.)
    return 1;
}

int OFreqPulseAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void OFreqPulseAudioProcessor::setCurrentProgram(int index)
{
    // Factory presets are available but not auto-loaded when program changes
    // This prevents interference with DAW state restoration and automation
    // Preset loading is triggered explicitly via UI or initial selection
    if (index >= 0 && index < numPresets)
        currentProgram = index;
}

const juce::String OFreqPulseAudioProcessor::getProgramName(int index)
{
    if (index >= 0 && index < numPresets)
        return presetNames[index];
    return {};
}

void OFreqPulseAudioProcessor::loadPreset(int presetIndex)
{
    auto* mix = parameters.getParameter("mix");
    auto* steps = parameters.getParameter("steps");
    auto* rate = parameters.getParameter("rate");
    auto* swing = parameters.getParameter("swing");
    auto* attack = parameters.getParameter("attack");
    auto* release = parameters.getParameter("release");

    // Helper: set step velocities for a band (0.0 = off, >0 = on with velocity)
    auto setStepVelocities = [this](int band, const std::array<float, 32>& velocities) {
        for (int i = 0; i < 32; ++i)
        {
            juce::String stepID = "step_b" + juce::String(band) + "_s" + juce::String(i);
            if (auto* param = parameters.getParameter(stepID))
                param->setValueNotifyingHost(velocities[i]);
        }
    };

    // Helper: set crossover frequencies
    auto setCrossovers = [this](float c1, float c2, float c3) {
        if (auto* p = parameters.getParameter("crossover_1"))
            p->setValueNotifyingHost(p->convertTo0to1(c1));
        if (auto* p = parameters.getParameter("crossover_2"))
            p->setValueNotifyingHost(p->convertTo0to1(c2));
        if (auto* p = parameters.getParameter("crossover_3"))
            p->setValueNotifyingHost(p->convertTo0to1(c3));
    };

    // Helper: set all band parameters including per-band rate, phase offset, and step count
    // bandRate: 0=Global, 1=1/1, 2=1/2, 3=1/4, 4=1/8, 5=1/16, 6=1/32, 7=1/8T, 8=1/16T, 9=1/4D, 10=1/8D
    auto setBandParams = [this](int band, bool enable, bool eucOn, int eucSteps, int eucPulses, int eucOffset,
                                 float depth, int bandRate = 0, int phaseOffset = 0, int bandSteps = 0) {
        juce::String bandID = "band" + juce::String(band);

        if (auto* p = parameters.getParameter(bandID + "_enable"))
            p->setValueNotifyingHost(enable ? 1.0f : 0.0f);
        if (auto* p = parameters.getParameter(bandID + "_euc_on"))
            p->setValueNotifyingHost(eucOn ? 1.0f : 0.0f);
        if (auto* p = parameters.getParameter(bandID + "_euc_steps"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(eucSteps)));
        if (auto* p = parameters.getParameter(bandID + "_euc_pulses"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(eucPulses)));
        if (auto* p = parameters.getParameter(bandID + "_euc_offset"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(eucOffset)));
        if (auto* p = parameters.getParameter(bandID + "_rate"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(bandRate)));
        if (auto* p = parameters.getParameter(bandID + "_phase_offset"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(phaseOffset)));
        if (auto* p = parameters.getParameter(bandID + "_steps"))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(bandSteps)));
        if (auto* p = parameters.getParameter(bandID + "_depth"))
            p->setValueNotifyingHost(depth);
    };

    std::array<float, 32> velEmpty = {};

    // WR-02: reset every band's step grid up-front. Euclidean-mode bands skip
    // setStepVelocities() below, so without this the previously-loaded preset's manual
    // velocities leak into the captured factory JSON (this fn is used to capture the 12
    // factory presets) and reappear the instant a user switches that band to Manual.
    for (int b = 0; b < 4; ++b)
        setStepVelocities(b, velEmpty);

    switch (presetIndex)
    {
        case 0:  // Init - Clean starting point
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(16.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));     // 1/16
            swing->setValueNotifyingHost(0.0f);
            attack->setValueNotifyingHost(attack->convertTo0to1(5.0f));
            release->setValueNotifyingHost(release->convertTo0to1(5.0f));
            setCrossovers(120.0f, 500.0f, 4000.0f);

            for (int b = 0; b < 4; ++b)
            {
                setBandParams(b, true, false, 16, 8, 0, 1.0f);
                setStepVelocities(b, velEmpty);
            }
            break;
        }

        case 1:  // Classic Sidechain - Sub solid, mids pump with velocity accents
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(16.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(2.0f));     // 1/4
            swing->setValueNotifyingHost(0.0f);
            attack->setValueNotifyingHost(attack->convertTo0to1(3.0f));
            release->setValueNotifyingHost(release->convertTo0to1(35.0f));
            setCrossovers(120.0f, 500.0f, 4000.0f);

            // Sub: no gating — all steps on at full velocity
            setBandParams(0, true, false, 16, 16, 0, 0.0f);
            setStepVelocities(0, {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
                                   0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0});

            // Low: quarter pump with accented beats 1 & 3
            setBandParams(1, true, false, 16, 8, 0, 0.85f);
            setStepVelocities(1, {1.0f,0,0,0, 0.7f,0,0,0, 1.0f,0,0,0, 0.7f,0,0,0,
                                   0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0});

            // Mid: full pump, phase offset 1 for slight delay behind kick
            setBandParams(2, true, false, 16, 8, 0, 1.0f, 0, 1);
            setStepVelocities(2, {1.0f,0,0,0, 0.8f,0,0,0, 1.0f,0,0,0, 0.8f,0,0,0,
                                   0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0});

            // High: 8th note shimmer with velocity groove
            setBandParams(3, true, false, 16, 8, 0, 0.5f);
            setStepVelocities(3, {1.0f,0,0.7f,0, 0.9f,0,0.6f,0, 1.0f,0,0.7f,0, 0.9f,0,0.6f,0,
                                   0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0});
            break;
        }

        case 2:  // Trance Gate 16th - Velocity groove with phase-shifted highs
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(16.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));     // 1/16
            swing->setValueNotifyingHost(0.1f);
            attack->setValueNotifyingHost(attack->convertTo0to1(2.0f));
            release->setValueNotifyingHost(release->convertTo0to1(8.0f));
            setCrossovers(120.0f, 500.0f, 4000.0f);

            // Accented groove pattern: strong-weak-medium-weak
            std::array<float, 32> tranceGroove = {1.0f,0.5f,0.8f,0.5f, 1.0f,0.5f,0.8f,0.5f,
                                                    1.0f,0.5f,0.8f,0.5f, 1.0f,0.5f,0.8f,0.5f,
                                                    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

            setBandParams(0, true, false, 16, 8, 0, 1.0f);
            setStepVelocities(0, tranceGroove);

            setBandParams(1, true, false, 16, 8, 0, 1.0f);
            setStepVelocities(1, tranceGroove);

            // Mid: phase offset 1 creates slight stagger
            setBandParams(2, true, false, 16, 8, 0, 1.0f, 0, 1);
            setStepVelocities(2, tranceGroove);

            // High: phase offset 2 for shimmer detachment
            setBandParams(3, true, false, 16, 8, 0, 0.9f, 0, 2);
            setStepVelocities(3, tranceGroove);
            break;
        }

        case 3:  // Dubstep Pulse - Heavy sub gate with per-band rates
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(8.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(3.0f));     // 1/8
            swing->setValueNotifyingHost(0.0f);
            attack->setValueNotifyingHost(attack->convertTo0to1(2.0f));
            release->setValueNotifyingHost(release->convertTo0to1(15.0f));
            setCrossovers(80.0f, 300.0f, 3000.0f);  // Wider bass range

            // Sub: Euclidean 5/8 at 1/4 rate for heavy half-time pulse
            setBandParams(0, true, true, 8, 5, 0, 1.0f, 3);       // bandRate 3 = 1/4

            // Low: Euclidean 3/8 at 1/8, phase offset 1 for push
            setBandParams(1, true, true, 8, 3, 0, 0.9f, 4, 1);    // bandRate 4 = 1/8

            // Mid: manual velocity pattern, subtle depth
            setBandParams(2, true, false, 8, 8, 0, 0.4f);
            setStepVelocities(2, {1.0f,0,0.6f,0, 0.8f,0,0.5f,0,
                                   0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0});

            // High: minimal depth, light velocity pattern
            setBandParams(3, true, false, 8, 8, 0, 0.15f);
            setStepVelocities(3, {1.0f,0.6f,0.8f,0.6f, 1.0f,0.6f,0.8f,0.6f,
                                   0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0});
            break;
        }

        case 4:  // Ambient Shimmer - Slow phase-shifted highs with swells
        {
            mix->setValueNotifyingHost(0.7f);
            steps->setValueNotifyingHost(steps->convertTo0to1(32.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(1.0f));     // 1/2
            swing->setValueNotifyingHost(0.2f);
            attack->setValueNotifyingHost(attack->convertTo0to1(80.0f));
            release->setValueNotifyingHost(release->convertTo0to1(120.0f));
            setCrossovers(100.0f, 800.0f, 6000.0f);  // Wide mid for pads

            // Sub: no gating
            setBandParams(0, true, false, 32, 32, 0, 0.0f);
            setStepVelocities(0, velEmpty);

            // Low: no gating
            setBandParams(1, true, false, 32, 32, 0, 0.0f);
            setStepVelocities(1, velEmpty);

            // Mid: sparse Euclidean, shifted, dotted quarter rate
            setBandParams(2, true, true, 32, 7, 0, 0.5f, 9, 4);   // bandRate 9 = 1/4D, phase 4

            // High: sparser Euclidean, further phase shift
            setBandParams(3, true, true, 32, 11, 3, 0.7f, 0, 8);  // Global rate, phase 8
            break;
        }

        case 5:  // Polymetric Machine - Independent step counts and rates per band
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(16.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));     // 1/16
            swing->setValueNotifyingHost(0.0f);
            attack->setValueNotifyingHost(attack->convertTo0to1(5.0f));
            release->setValueNotifyingHost(release->convertTo0to1(15.0f));
            setCrossovers(120.0f, 500.0f, 4000.0f);

            // Sub: 4-step loop at 1/4 rate — Euclidean 3/4
            setBandParams(0, true, true, 4, 3, 0, 1.0f, 3, 0, 4);     // rate=1/4, bandSteps=4

            // Low: 6-step loop at 1/8 rate — Euclidean 4/6
            setBandParams(1, true, true, 6, 4, 0, 0.9f, 4, 1, 6);     // rate=1/8, phase=1, bandSteps=6

            // Mid: 8-step loop at global rate — Euclidean 5/8
            setBandParams(2, true, true, 8, 5, 0, 0.85f, 0, 2, 8);    // phase=2, bandSteps=8

            // High: 12-step loop at 1/16 rate — Euclidean 7/12
            setBandParams(3, true, true, 12, 7, 0, 0.8f, 5, 3, 12);   // rate=1/16, phase=3, bandSteps=12
            break;
        }

        case 6:  // Bass Foundation - Sub always on, others gated with velocity
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(16.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));     // 1/16
            swing->setValueNotifyingHost(0.0f);
            attack->setValueNotifyingHost(attack->convertTo0to1(8.0f));
            release->setValueNotifyingHost(release->convertTo0to1(20.0f));
            setCrossovers(120.0f, 500.0f, 4000.0f);

            // Sub: bypass (always on)
            setBandParams(0, false, false, 16, 8, 0, 0.0f);
            setStepVelocities(0, velEmpty);

            // Low: Euclidean 12/16
            setBandParams(1, true, true, 16, 12, 0, 0.7f);

            // Mid: Euclidean 8/16, phase offset 2
            setBandParams(2, true, true, 16, 8, 0, 1.0f, 0, 2);

            // High: Euclidean 10/16 at triplet rate, phase offset 4
            setBandParams(3, true, true, 16, 10, 2, 0.9f, 7, 4);  // bandRate 7 = 1/8T
            break;
        }

        case 7:  // Hi-Hat Chop - Realistic hi-hat velocity pattern
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(32.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(5.0f));     // 1/32
            swing->setValueNotifyingHost(0.25f);
            attack->setValueNotifyingHost(attack->convertTo0to1(1.0f));
            release->setValueNotifyingHost(release->convertTo0to1(3.0f));
            setCrossovers(120.0f, 600.0f, 5000.0f);  // Wide high range

            // Sub/Low/Mid: bypass
            setBandParams(0, false, false, 16, 8, 0, 0.0f);
            setStepVelocities(0, velEmpty);
            setBandParams(1, false, false, 16, 8, 0, 0.0f);
            setStepVelocities(1, velEmpty);
            setBandParams(2, false, false, 16, 8, 0, 0.0f);
            setStepVelocities(2, velEmpty);

            // High: realistic hi-hat groove with ghost notes and accents
            setBandParams(3, true, false, 32, 12, 0, 1.0f);
            setStepVelocities(3, {1.0f,0.3f,0.6f,0.3f, 0.9f,0.3f,0.7f,0.3f,
                                   1.0f,0.3f,0.6f,0.3f, 0.9f,0.3f,0.7f,0.4f,
                                   1.0f,0.3f,0.6f,0.3f, 0.9f,0.3f,0.7f,0.3f,
                                   1.0f,0.3f,0.6f,0.3f, 0.9f,0.3f,0.7f,0.4f});
            break;
        }

        case 8:  // Phase Cascade - Same pattern, staggered phase offsets
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(16.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));     // 1/16
            swing->setValueNotifyingHost(0.0f);
            attack->setValueNotifyingHost(attack->convertTo0to1(3.0f));
            release->setValueNotifyingHost(release->convertTo0to1(10.0f));
            setCrossovers(120.0f, 500.0f, 4000.0f);

            // All bands: same Euclidean 8/16, cascading phase offsets create waterfall
            setBandParams(0, true, true, 16, 8, 0, 1.0f, 0, 0);   // phase 0
            setBandParams(1, true, true, 16, 8, 0, 1.0f, 0, 4);   // phase 4
            setBandParams(2, true, true, 16, 8, 0, 1.0f, 0, 8);   // phase 8
            setBandParams(3, true, true, 16, 8, 0, 0.9f, 0, 12);  // phase 12
            break;
        }

        case 9:  // Euclidean Groove - Musical ratios with per-band rates
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(16.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(3.0f));     // 1/8
            swing->setValueNotifyingHost(0.15f);
            attack->setValueNotifyingHost(attack->convertTo0to1(5.0f));
            release->setValueNotifyingHost(release->convertTo0to1(12.0f));
            setCrossovers(120.0f, 500.0f, 4000.0f);

            // Sub: 4/16 quarter feel at 1/4 rate
            setBandParams(0, true, true, 16, 4, 0, 1.0f, 3);      // bandRate 3 = 1/4

            // Low: 7/16 with phase offset 2
            setBandParams(1, true, true, 16, 7, 1, 0.9f, 0, 2);

            // Mid: 9/16 with phase offset 5
            setBandParams(2, true, true, 16, 9, 0, 0.85f, 0, 5);

            // High: 11/16 at 1/16 rate, phase offset 1
            setBandParams(3, true, true, 16, 11, 2, 0.75f, 5, 1); // bandRate 5 = 1/16
            break;
        }

        case 10:  // Half-Time Feel - Slow dramatic swells with polymetric highs
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(8.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(2.0f));     // 1/4
            swing->setValueNotifyingHost(0.0f);
            attack->setValueNotifyingHost(attack->convertTo0to1(25.0f));
            release->setValueNotifyingHost(release->convertTo0to1(80.0f));
            setCrossovers(120.0f, 500.0f, 4000.0f);

            // Sub: sparse 2/4 at its own 4-step loop
            setBandParams(0, true, true, 4, 2, 0, 0.8f, 0, 0, 4);     // bandSteps=4

            // Low: 3/8
            setBandParams(1, true, true, 8, 3, 0, 1.0f);

            // Mid: 3/8, phase offset 2
            setBandParams(2, true, true, 8, 3, 0, 1.0f, 0, 2);

            // High: 12-step loop at 1/8 rate for polymetric tail
            setBandParams(3, true, true, 12, 5, 0, 0.9f, 4, 0, 12);   // rate=1/8, bandSteps=12
            break;
        }

        case 11:  // Triplet Bounce - Triplet timing with phase shifts
        {
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(12.0f));
            rate->setValueNotifyingHost(rate->convertTo0to1(6.0f));     // 1/8T
            swing->setValueNotifyingHost(0.1f);
            attack->setValueNotifyingHost(attack->convertTo0to1(4.0f));
            release->setValueNotifyingHost(release->convertTo0to1(8.0f));
            setCrossovers(120.0f, 500.0f, 4000.0f);

            // Sub: 4/12
            setBandParams(0, true, true, 12, 4, 0, 1.0f);

            // Low: 5/12 at 1/16T rate, phase offset 2
            setBandParams(1, true, true, 12, 5, 1, 0.9f, 8, 2);       // bandRate 8 = 1/16T

            // Mid: 7/12, phase offset 4
            setBandParams(2, true, true, 12, 7, 0, 1.0f, 0, 4);

            // High: 9-step loop at 1/8T, phase offset 1
            setBandParams(3, true, true, 9, 5, 2, 0.8f, 7, 1, 9);     // rate=1/8T, bandSteps=9
            break;
        }

        default:
            break;
    }
}

//==============================================================================
// v1.6.0: Factory Preset Initialization (Preset Manager)
//==============================================================================
void OFreqPulseAudioProcessor::initializeFactoryPresets()
{
    auto factoryDir = presetManager.getFactoryPresetsDirectory();

    // Regenerate factory presets when plugin version changes (ensures new parameters are captured)
    // WR-06: gate on JucePlugin_VersionString rather than a hand-maintained literal, so a future
    // release that changes a preset/param can never ship stale factory JSON by forgetting to bump.
    auto versionFile = factoryDir.getChildFile(".version");

    if (factoryDir.isDirectory()
        && versionFile.existsAsFile()
        && versionFile.loadFileAsString().trimEnd() == JucePlugin_VersionString)
        return;

    // WR-07: overwrite the 12 files in place (replaceWithText below) instead of
    // deleteRecursively()+recreate. The old delete-then-write opened a window where a
    // concurrent getPresetList() saw an empty Factory dir, and two processors constructing
    // concurrently could both delete/recreate and interleave writes.
    factoryDir.createDirectory();

    // For each of the 12 presets: load via existing loadPreset(), capture state, save as JSON
    for (int i = 0; i < numPresets; ++i)
    {
        // Apply preset parameters
        loadPreset(i);

        // Capture current state as JSON
        auto presetJson = juce::var();
        {
            auto* preset = new juce::DynamicObject();
            auto* paramsObj = new juce::DynamicObject();

            for (auto* param : getParameters())
            {
                if (auto* paramWithID = dynamic_cast<juce::RangedAudioParameter*>(param))
                {
                    paramsObj->setProperty(paramWithID->getParameterID(),
                                           paramWithID->getValue());
                }
            }
            preset->setProperty("parameters", juce::var(paramsObj));
            preset->setProperty("version", "1.0.0");
            preset->setProperty("plugin", "O-FreqPulse");
            preset->setProperty("factory", true);
            presetJson = juce::var(preset);
        }

        auto presetFile = factoryDir.getChildFile(presetNames[i] + ".json");
        presetFile.replaceWithText(juce::JSON::toString(presetJson, true));
    }

    // Reset back to Init preset (index 0)
    loadPreset(0);

    versionFile.replaceWithText(juce::String(JucePlugin_VersionString) + "\n");

    juce::Logger::writeToLog("[O-FreqPulse] Factory presets initialized: " + juce::String(numPresets));
}

//==============================================================================
// Plugin Factory
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OFreqPulseAudioProcessor();
}
