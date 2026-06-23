/*
  ==============================================================================

    O-Freeze - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OFreezeAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // FREEZE - Manual trigger button
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "FREEZE", 1 },
        "Freeze",
        false));

    // THRESHOLD - Auto-freeze level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "THRESHOLD", 1 },
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -40.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // MODE - Trigger mode selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MODE", 1 },
        "Mode",
        juce::StringArray { "Manual", "Threshold" },
        0));

    // DRIFT - Grain position randomization range
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DRIFT", 1 },
        "Drift",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // MIX - Dry/Wet blend
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // GRAIN_SIZE - Granular grain size in milliseconds
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "GRAIN_SIZE", 1 },
        "Grain Size",
        juce::NormalisableRange<float>(50.0f, 1000.0f, 1.0f),
        400.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // GRAIN_COUNT - Number of active grains (2-32, integer steps)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "GRAIN_COUNT", 1 },
        "Grain Count",
        2, 32, 8));

    // LFO_RATE - LFO frequency in Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LFO_RATE", 1 },
        "LFO Rate",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.4f),
        0.5f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // LFO_DEPTH - LFO modulation depth
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LFO_DEPTH", 1 },
        "LFO Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // LFO_SHAPE - LFO waveform shape
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "LFO_SHAPE", 1 },
        "LFO Shape",
        juce::StringArray { "Sine", "Triangle", "Random" },
        0));

    // REVERSE - Toggle reverse grain playback direction
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "REVERSE", 1 },
        "Reverse",
        false));

    // DETUNE - Per-grain pitch micro-detuning range in cents
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DETUNE", 1 },
        "Detune",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f),
        5.0f,
        juce::AudioParameterFloatAttributes().withLabel("cents")));

    return layout;
}

OFreezeAudioProcessor::OFreezeAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OFreezeAudioProcessor::~OFreezeAudioProcessor()
{
}

void OFreezeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store sample rate
    currentSampleRate = sampleRate;

    // Prepare DSP spec
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Pre-allocate freeze buffer for 2 seconds at max sample rate (192kHz = 384,000 samples)
    const int maxSampleRate = 192000;
    const int bufferLengthSeconds = 2;
    const int bufferLength = maxSampleRate * bufferLengthSeconds;
    freezeBuffer.setSize(getTotalNumOutputChannels(), bufferLength);
    freezeBuffer.clear();

    // Reset positions
    writePosition = 0;
    bufferFrozen = false;

    // Initialize crossfade smoother (50ms fade-in, 100ms fade-out)
    freezeGain.reset(sampleRate, 0.050); // 50ms ramp time
    freezeGain.setCurrentAndTargetValue(0.0f);

    // Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.reset();

    // Initialize threshold gate RMS detection (20ms rolling window)
    rmsSamplesPerWindow = static_cast<int>(sampleRate * 0.020);
    rmsBuffer.resize(rmsSamplesPerWindow);
    std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
    rmsWriteIndex = 0;
    rmsLevel = 0.0f;
    gateState = GateState::Idle;

    // Initialize granular synthesis components
    // Pre-allocate Hann window for max grain size (1000ms) to avoid audio-thread allocations
    maxGrainSize = static_cast<int>(sampleRate * 1.0);
    hannWindow.resize(maxGrainSize);

    // Read initial grain size and grain count from parameters
    auto* grainSizeParam = parameters.getRawParameterValue("GRAIN_SIZE");
    auto* grainCountParam = parameters.getRawParameterValue("GRAIN_COUNT");
    float grainSizeMs = grainSizeParam->load();
    numGrains = static_cast<int>(grainCountParam->load());
    lastGrainSizeMs = grainSizeMs;
    lastGrainCount = numGrains;
    grainSize = static_cast<int>(sampleRate * grainSizeMs / 1000.0);
    grainTriggerInterval = grainSize / numGrains;

    // Raw Hann window (no COLA scaling — per-grain jitter breaks COLA alignment)
    // Normalization done per-sample by dividing by sum of active window values
    for (int i = 0; i < grainSize; ++i)
    {
        double phase = static_cast<double>(i) / static_cast<double>(grainSize);
        hannWindow[i] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * juce::MathConstants<double>::pi * phase)));
    }

    // Reset all grains to inactive
    for (auto& grain : grains)
    {
        grain.active = false;
        grain.startSample = 0;
        grain.position = 0;
        grain.jitterOffset = 0;
        grain.playbackRate = 1.0f;
        grain.fractionalPosition = 0.0f;
    }

    grainTriggerCounter = 0;
    nextTriggerInterval = grainTriggerInterval;
    nextGrainIndex = 0;
    stopTriggeringNewGrains = false;

    // Initialize WSOLA state
    lastGrainTail.fill(0.0f);
    hasLastGrainTail = false;

    // Initialize drift state
    frozenDriftOffset = 0.0f;
    smoothedDriftOffset.reset(sampleRate, 0.015); // 15ms smoothing to eliminate clicks
    smoothedDriftOffset.setCurrentAndTargetValue(0.0f);

    // Initialize LFO state
    lfoPhase = 0.0f;
    lfoCurrentValue = 0.0f;
    randomHoldValue = 0.0f;
}

void OFreezeAudioProcessor::releaseResources()
{
    freezeBuffer.setSize(0, 0);

    for (auto& grain : grains)
    {
        grain.active = false;
        grain.startSample = 0;
        grain.position = 0;
        grain.jitterOffset = 0;
        grain.playbackRate = 1.0f;
        grain.fractionalPosition = 0.0f;
    }

    freezeGain.setCurrentAndTargetValue(0.0f);
    bufferFrozen = false;
    stopTriggeringNewGrains = false;

    lastGrainTail.fill(0.0f);
    hasLastGrainTail = false;
}

void OFreezeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), freezeBuffer.getNumChannels());

    if (numSamples == 0)
        return;

    // Read parameters (atomic, real-time safe)
    auto* freezeParam = parameters.getRawParameterValue("FREEZE");
    auto* modeParam = parameters.getRawParameterValue("MODE");
    auto* thresholdParam = parameters.getRawParameterValue("THRESHOLD");
    auto* mixParam = parameters.getRawParameterValue("MIX");
    auto* grainSizeParam = parameters.getRawParameterValue("GRAIN_SIZE");
    auto* grainCountParam = parameters.getRawParameterValue("GRAIN_COUNT");
    auto* reverseParam = parameters.getRawParameterValue("REVERSE");
    auto* detuneParam = parameters.getRawParameterValue("DETUNE");

    int modeValue = static_cast<int>(modeParam->load());
    float thresholdDB = thresholdParam->load();
    float mixValue = mixParam->load() / 100.0f; // Convert 0-100% to 0-1
    float grainSizeMs = grainSizeParam->load();
    int grainCountValue = static_cast<int>(grainCountParam->load());
    bool reverseActive = reverseParam->load() > 0.5f;
    float detuneValue = detuneParam->load();

    // Recalculate grain parameters when size or count changes
    bool grainParamsChanged = (grainSizeMs != lastGrainSizeMs) || (grainCountValue != lastGrainCount);
    if (grainParamsChanged)
    {
        lastGrainSizeMs = grainSizeMs;
        lastGrainCount = grainCountValue;
        numGrains = grainCountValue;
        grainSize = juce::jmin(static_cast<int>(currentSampleRate * grainSizeMs / 1000.0), maxGrainSize);
        grainTriggerInterval = grainSize / numGrains;
        nextTriggerInterval = grainTriggerInterval;

        // Rebuild raw Hann window (pre-allocated buffer, no alloc)
        for (int i = 0; i < grainSize; ++i)
        {
            double phase = static_cast<double>(i) / static_cast<double>(grainSize);
            hannWindow[i] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * juce::MathConstants<double>::pi * phase)));
        }

        // Deactivate grains past new grain size boundary or beyond active count
        for (int g = 0; g < MAX_GRAINS; ++g)
        {
            if (grains[g].active && (grains[g].startSample >= grainSize || g >= numGrains))
                grains[g].active = false;
        }

        // Clamp next grain index to new count
        if (nextGrainIndex >= numGrains)
            nextGrainIndex = 0;
    }

    // Determine freeze state based on mode
    bool freezeActive = false;

    if (modeValue == 0) // Manual mode
    {
        freezeActive = freezeParam->load() > 0.5f;
    }
    else // Threshold mode
    {
        // Calculate RMS level from input buffer
        float sumSquares = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getReadPointer(channel);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float inputSample = channelData[sample];

                // Update RMS buffer (circular)
                rmsBuffer[rmsWriteIndex] = inputSample * inputSample;
                rmsWriteIndex = (rmsWriteIndex + 1) % rmsSamplesPerWindow;
            }
        }

        // Calculate RMS from rolling window
        for (float squaredSample : rmsBuffer)
            sumSquares += squaredSample;

        float rms = std::sqrt(sumSquares / static_cast<float>(rmsSamplesPerWindow));
        rmsLevel = 20.0f * std::log10(rms + 1e-6f); // Convert to dB

        // Threshold gate with hysteresis (3dB)
        const float hysteresisDB = 3.0f;

        if (gateState == GateState::Idle && rmsLevel < thresholdDB)
        {
            gateState = GateState::Frozen;
        }
        else if (gateState == GateState::Frozen && rmsLevel > (thresholdDB + hysteresisDB))
        {
            gateState = GateState::Idle;
        }

        freezeActive = (gateState == GateState::Frozen);
    }

    // Read DRIFT parameter
    auto* driftParam = parameters.getRawParameterValue("DRIFT");
    float driftValue = driftParam->load() / 100.0f; // Convert 0-100% to 0-1

    // Read LFO parameters
    auto* lfoRateParam = parameters.getRawParameterValue("LFO_RATE");
    auto* lfoDepthParam = parameters.getRawParameterValue("LFO_DEPTH");
    auto* lfoShapeParam = parameters.getRawParameterValue("LFO_SHAPE");
    float lfoRate = lfoRateParam->load();
    float lfoDepth = lfoDepthParam->load() / 100.0f; // Convert 0-100% to 0-1
    int lfoShape = static_cast<int>(lfoShapeParam->load());

    // Compute LFO per-block (not per-sample) for efficiency
    if (bufferFrozen && lfoDepth > 0.0f)
    {
        // Advance LFO phase
        float phaseIncrement = lfoRate * static_cast<float>(numSamples) / static_cast<float>(currentSampleRate);
        lfoPhase += phaseIncrement;

        // Wrap phase and handle random S&H trigger
        if (lfoPhase >= 1.0f)
        {
            lfoPhase -= std::floor(lfoPhase);
            // New random value each cycle for S&H
            randomHoldValue = random.nextFloat() * 2.0f - 1.0f;
        }

        // Compute LFO value based on shape (-1 to +1)
        switch (lfoShape)
        {
            case 0: // Sine
                lfoCurrentValue = std::sin(lfoPhase * juce::MathConstants<float>::twoPi);
                break;
            case 1: // Triangle
                lfoCurrentValue = 4.0f * std::abs(lfoPhase - 0.5f) - 1.0f;
                break;
            case 2: // Random (Sample & Hold)
                lfoCurrentValue = randomHoldValue;
                break;
            default:
                lfoCurrentValue = 0.0f;
                break;
        }
    }
    else if (!bufferFrozen)
    {
        lfoCurrentValue = 0.0f;
    }

    // Push dry signal to mixer
    dryWetMixer.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));

    // Update freeze state and crossfade envelope
    if (freezeActive != bufferFrozen)
    {
        bufferFrozen = freezeActive;

        if (bufferFrozen)
        {
            // Freeze engaged: fade in frozen signal
            freezeGain.reset(currentSampleRate, 0.050); // 50ms fade-in
            freezeGain.setTargetValue(1.0f);

            // Reset WSOLA state for fresh freeze
            hasLastGrainTail = false;

            // LOCK drift offset at freeze moment - all grains share this for COLA
            // Pick a random offset (adds variation between freeze engages)
            frozenDriftOffset = random.nextFloat();

            // Staggered activation: only activate FIRST grain immediately
            // Subsequent grains will be activated by normal trigger mechanism
            const int freezeBufLen = freezeBuffer.getNumSamples();
            int startPos = reverseActive
                ? (writePosition - 1 + freezeBufLen) % freezeBufLen
                : (writePosition - grainSize + freezeBufLen) % freezeBufLen;

            // Activate only the first grain (jitterOffset=0 for clean initial capture)
            grains[0].active = true;
            grains[0].startSample = 0;
            grains[0].position = startPos;
            grains[0].jitterOffset = 0;
            grains[0].playbackRate = 1.0f;  // No detune on first grain
            grains[0].fractionalPosition = 0.0f;

            // Deactivate all other grains - they'll be triggered naturally
            for (int i = 1; i < MAX_GRAINS; ++i)
            {
                grains[i].active = false;
            }

            nextGrainIndex = 1;  // Next grain to trigger
            grainTriggerCounter = 0;
            nextTriggerInterval = grainTriggerInterval;  // Reset to nominal for clean first cycle
            stopTriggeringNewGrains = false;  // Ensure we're triggering grains
        }
        else
        {
            // Freeze released: soft deactivation
            // Stop triggering NEW grains, but let active grains complete their cycle
            stopTriggeringNewGrains = true;

            // Fade-out covers grain completion time + 50ms safety margin
            float grainDurationSec = static_cast<float>(grainSize) / static_cast<float>(currentSampleRate);
            freezeGain.reset(currentSampleRate, static_cast<double>(grainDurationSec) + 0.050);
            freezeGain.setTargetValue(0.0f);

            // DON'T deactivate grains here - they'll naturally complete
            // when startSample >= grainSize (already handled in grain advance loop)
        }
    }

    const int freezeBufferLength = freezeBuffer.getNumSamples();

    // Compute target drift offset in samples (smoothed per-sample to prevent clicks)
    {
        float modulatedDriftOffset = frozenDriftOffset + (lfoCurrentValue * lfoDepth);
        modulatedDriftOffset = juce::jlimit(0.0f, 1.0f, modulatedDriftOffset);
        float driftRangeF = static_cast<float>(grainSize) * driftValue;
        float targetDriftSamples = modulatedDriftOffset * driftRangeF;
        smoothedDriftOffset.setTargetValue(targetDriftSamples);
    }

    // Process sample-by-sample (all channels together) so grain state stays in sync
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Only trigger new grains if frozen AND not in soft release mode
        if (bufferFrozen && !stopTriggeringNewGrains)
        {
            // IMPORTANT: Drift offset is LOCKED when freeze engages (set in freeze activation)
            // Do NOT update drift while frozen - all grains must share the same offset
            // for COLA phase alignment to work correctly

            if (grainTriggerCounter >= nextTriggerInterval)
            {
                // Activate new grain
                Grain& newGrain = grains[nextGrainIndex];
                newGrain.active = true;
                newGrain.startSample = 0;

                // Store BASE position only - drift applied at read time
                int basePos = reverseActive
                    ? (writePosition - 1 + freezeBufferLength) % freezeBufferLength
                    : (writePosition - grainSize + freezeBufferLength) % freezeBufferLength;

                // WSOLA: search for best-overlap position with previous grain's tail
                if (hasLastGrainTail)
                {
                    int searchRange = grainSize / 4;
                    int bestOffset = 0;
                    float bestCorrelation = -1.0f;

                    // Precompute energy of lastGrainTail (constant across all offsets)
                    float sumX2 = 0.0f;
                    for (int i = 0; i < WSOLA_TAIL_SIZE; ++i)
                        sumX2 += lastGrainTail[i] * lastGrainTail[i];

                    for (int offset = -searchRange; offset <= searchRange; offset += 4)
                    {
                        int candidateStart = (basePos + offset + freezeBufferLength) % freezeBufferLength;

                        float sumXY = 0.0f;
                        float sumY2 = 0.0f;
                        for (int i = 0; i < WSOLA_TAIL_SIZE; ++i)
                        {
                            int samplePos = reverseActive
                                ? (candidateStart - i + freezeBufferLength) % freezeBufferLength
                                : (candidateStart + i) % freezeBufferLength;

                            float mono = 0.0f;
                            for (int ch = 0; ch < numChannels; ++ch)
                                mono += freezeBuffer.getReadPointer(ch)[samplePos];
                            mono /= static_cast<float>(numChannels);

                            sumXY += lastGrainTail[i] * mono;
                            sumY2 += mono * mono;
                        }

                        float denom = std::sqrt(sumX2 * sumY2);
                        float corr = (denom > 1e-6f) ? (sumXY / denom) : 0.0f;

                        if (corr > bestCorrelation)
                        {
                            bestCorrelation = corr;
                            bestOffset = offset;
                        }
                    }

                    basePos = (basePos + bestOffset + freezeBufferLength) % freezeBufferLength;
                }

                newGrain.position = basePos;

                // Per-grain random jitter ON TOP of WSOLA-chosen position
                // Range scales with grain size and drift amount
                float jitterRange = static_cast<float>(grainSize) * driftValue;
                newGrain.jitterOffset = static_cast<int>(random.nextFloat() * jitterRange);

                // Per-grain pitch micro-detuning (cents → rate multiplier)
                newGrain.playbackRate = 1.0f + (random.nextFloat() * 2.0f - 1.0f) * (detuneValue / 1200.0f);
                newGrain.fractionalPosition = 0.0f;

                // Advance grain index (round-robin within active count)
                nextGrainIndex = (nextGrainIndex + 1) % numGrains;

                // Reset trigger counter and compute jittered interval for next trigger
                grainTriggerCounter = 0;
                int jitter = static_cast<int>((random.nextFloat() * 2.0f - 1.0f) * grainTriggerInterval * 0.3f);
                nextTriggerInterval = juce::jmax(grainTriggerInterval / 2, grainTriggerInterval + jitter);
            }
            else
            {
                grainTriggerCounter++;
            }
        }

        // Smoothed drift offset (ramps per-sample to prevent clicks from knob/LFO changes)
        int sharedDriftOffset = static_cast<int>(smoothedDriftOffset.getNextValue());

        // Get current window values and interpolated positions for all active grains (before advancing)

        for (int g = 0; g < numGrains; ++g)
        {
            if (grains[g].active)
            {
                windowValues[g] = hannWindow[grains[g].startSample];

                // Fractional position for pitch-shifted grain read
                float fp = grains[g].fractionalPosition;
                int intPart = static_cast<int>(fp);
                float frac = fp - static_cast<float>(intPart);

                // Compute interpolation sample positions (base + fractional offset + jitter + drift)
                if (reverseActive)
                {
                    grainPos0[g] = (grains[g].position - intPart + grains[g].jitterOffset + sharedDriftOffset + freezeBufferLength * 2) % freezeBufferLength;
                    grainPos1[g] = (grainPos0[g] - 1 + freezeBufferLength) % freezeBufferLength;
                }
                else
                {
                    grainPos0[g] = (grains[g].position + intPart + grains[g].jitterOffset + sharedDriftOffset + freezeBufferLength) % freezeBufferLength;
                    grainPos1[g] = (grainPos0[g] + 1) % freezeBufferLength;
                }
                grainFrac[g] = frac;
            }
        }

        // Advance freeze crossfade smoother once per sample (NOT per channel)
        float currentFreezeGain = freezeGain.getNextValue();

        // Process each channel using the same grain state
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* freezeData = freezeBuffer.getWritePointer(channel);

            float inputSample = channelData[sample];
            float outputSample = inputSample;

            // Write to freeze buffer (if not frozen)
            if (!bufferFrozen)
            {
                freezeData[writePosition] = inputSample;
            }

            // Granular synthesis (if frozen or fading out)
            if (bufferFrozen || freezeGain.getCurrentValue() > 0.001f)
            {
                // Sum all active grains (overlap-add synthesis)
                // Per-grain jitter breaks COLA — normalize by sum of active window values
                float granularSum = 0.0f;
                float windowSum = 0.0f;

                for (int g = 0; g < numGrains; ++g)
                {
                    if (grains[g].active)
                    {
                        // Linear interpolation between adjacent buffer samples
                        float s0 = freezeData[grainPos0[g]];
                        float s1 = freezeData[grainPos1[g]];
                        float grainSample = s0 + grainFrac[g] * (s1 - s0);
                        granularSum += grainSample * windowValues[g];
                        windowSum += windowValues[g];
                    }
                }

                // Normalize by window sum to maintain constant amplitude
                float frozenSample = (windowSum > 1e-6f) ? (granularSum / windowSum) : 0.0f;

                // Apply crossfade envelope
                outputSample = inputSample * (1.0f - currentFreezeGain) + frozenSample * currentFreezeGain;
            }

            channelData[sample] = outputSample;
        }

        // Advance grain state AFTER processing all channels
        for (auto& grain : grains)
        {
            if (grain.active)
            {
                grain.startSample++;
                grain.fractionalPosition += grain.playbackRate;

                // Deactivate grain when complete
                if (grain.startSample >= grainSize)
                {
                    // Compute final buffer position from fractional offset
                    int endOffset = static_cast<int>(grain.fractionalPosition);
                    int endPos = reverseActive
                        ? (grain.position - endOffset + freezeBufferLength * 2) % freezeBufferLength
                        : (grain.position + endOffset) % freezeBufferLength;

                    // Capture final 64 samples for WSOLA overlap matching (mono sum)
                    for (int i = 0; i < WSOLA_TAIL_SIZE; ++i)
                    {
                        int pos = reverseActive
                            ? (endPos + WSOLA_TAIL_SIZE - i + freezeBufferLength) % freezeBufferLength
                            : (endPos - WSOLA_TAIL_SIZE + i + freezeBufferLength) % freezeBufferLength;

                        float mono = 0.0f;
                        for (int ch = 0; ch < numChannels; ++ch)
                            mono += freezeBuffer.getReadPointer(ch)[pos];
                        lastGrainTail[i] = mono / static_cast<float>(numChannels);
                    }
                    hasLastGrainTail = true;

                    grain.active = false;
                }
            }
        }

        // Advance write position AFTER processing all channels
        if (!bufferFrozen)
        {
            writePosition = (writePosition + 1) % freezeBufferLength;
        }
    }

    // Apply dry/wet mix
    dryWetMixer.setWetMixProportion(mixValue);
    dryWetMixer.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
}

juce::AudioProcessorEditor* OFreezeAudioProcessor::createEditor()
{
    return new OFreezeAudioProcessorEditor(*this);
}

void OFreezeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OFreezeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OFreezeAudioProcessor();
}
