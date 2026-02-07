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

    globalGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "steps", 1 },
        "Steps",
        juce::StringArray { "4", "8", "16", "32" },
        2));  // Default index 2 = "16"

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
        juce::ParameterID { "smoothing", 1 },
        "Smoothing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
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
    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "freq_low", 1 },
        "Freq Low",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        20.0f));

    globalGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "freq_high", 1 },
        "Freq High",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        20000.0f));

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

        // Step grid parameters (32 per band)
        for (int m = 0; m < 32; ++m)
        {
            juce::String stepID = "step_b" + juce::String(n) + "_s" + juce::String(m);
            juce::String stepName = "B" + juce::String(n + 1) + " Step " + juce::String(m + 1);

            bandGroup->addChild(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID { stepID, 1 },
                stepName,
                false));
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
{
    // Cache global parameter pointers
    mixParam = parameters.getRawParameterValue("mix");
    stepsParam = parameters.getRawParameterValue("steps");
    rateParam = parameters.getRawParameterValue("rate");
    swingParam = parameters.getRawParameterValue("swing");
    smoothingParam = parameters.getRawParameterValue("smoothing");

    // Cache crossover parameter pointers
    crossover1Param = parameters.getRawParameterValue("crossover_1");
    crossover2Param = parameters.getRawParameterValue("crossover_2");
    crossover3Param = parameters.getRawParameterValue("crossover_3");

    // Cache frequency boundary parameter pointers
    freqLowParam = parameters.getRawParameterValue("freq_low");
    freqHighParam = parameters.getRawParameterValue("freq_high");

    // Cache per-band parameter pointers
    for (int n = 0; n < 4; ++n)
    {
        juce::String bandID = "band" + juce::String(n);

        bandParams[n].enable = parameters.getRawParameterValue(bandID + "_enable");
        bandParams[n].depth = parameters.getRawParameterValue(bandID + "_depth");
        bandParams[n].eucOn = parameters.getRawParameterValue(bandID + "_euc_on");
        bandParams[n].eucSteps = parameters.getRawParameterValue(bandID + "_euc_steps");
        bandParams[n].eucPulses = parameters.getRawParameterValue(bandID + "_euc_pulses");
        bandParams[n].eucOffset = parameters.getRawParameterValue(bandID + "_euc_offset");

        // Cache step grid parameters (32 per band)
        for (int m = 0; m < 32; ++m)
        {
            juce::String stepID = "step_b" + juce::String(n) + "_s" + juce::String(m);
            bandParams[n].stepStates[m] = parameters.getRawParameterValue(stepID);
        }
    }
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

    // Prepare STFT buffers for stereo
    for (int ch = 0; ch < 2; ++ch)
    {
        inputFifo[ch].resize(fftSize, 0.0f);
        outputFifo[ch].resize(fftSize, 0.0f);
        fftData[ch].resize(fftSize * 2, 0.0f);  // FFT needs 2x size for real/imag

        // Per-band output FIFOs for time-domain gain application (v1.2.0)
        for (int band = 0; band < 4; ++band)
            bandOutputFifo[band][ch].resize(fftSize, 0.0f);
        passthroughOutputFifo[ch].resize(fftSize, 0.0f);
        fftBandTemp[ch].resize(fftSize * 2, 0.0f);
    }

    // Reset buffer positions
    inputWritePos = 0;
    hopCounter = 0;

    // Pre-compute Hann window
    hannWindow.resize(static_cast<size_t>(fftSize));
    for (int i = 0; i < fftSize; ++i)
    {
        double phase = static_cast<double>(i) / static_cast<double>(fftSize);
        hannWindow[static_cast<size_t>(i)] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * juce::MathConstants<double>::pi * phase)));
    }

    // Configure DryWetMixer
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    dryWetMixer.prepare(spec);
    dryWetMixer.reset();

    // Configure gain smoothers (one per band)
    for (int band = 0; band < 4; ++band)
    {
        bandGainSmooth[band].reset(sampleRate, smoothingParam->load() / 1000.0);  // Convert ms to seconds
        bandGainSmooth[band].setCurrentAndTargetValue(1.0f);
    }

    // Calculate bin-to-band mapping
    recalculateBinMapping();

    // Generate initial Euclidean patterns
    updateEuclideanPatterns();

    // Reset step sequencer
    currentStep = 0;
    currentStepAtomic.store(0);
    lastPpqPosition = -1.0;
    sampleAccumulator = 0.0;

    // Report latency to DAW
    setLatencySamples(fftSize);
}

void OFreqPulseAudioProcessor::releaseResources()
{
    // Reset DryWetMixer
    dryWetMixer.reset();

    // Clear STFT buffers
    for (int ch = 0; ch < 2; ++ch)
    {
        std::fill(inputFifo[ch].begin(), inputFifo[ch].end(), 0.0f);
        std::fill(outputFifo[ch].begin(), outputFifo[ch].end(), 0.0f);
        std::fill(fftData[ch].begin(), fftData[ch].end(), 0.0f);

        for (int band = 0; band < 4; ++band)
            std::fill(bandOutputFifo[band][ch].begin(), bandOutputFifo[band][ch].end(), 0.0f);
        std::fill(passthroughOutputFifo[ch].begin(), passthroughOutputFifo[ch].end(), 0.0f);
        std::fill(fftBandTemp[ch].begin(), fftBandTemp[ch].end(), 0.0f);
    }

    // Reset step tracking
    currentStep = 0;
    currentStepAtomic.store(0);
    lastPpqPosition = -1.0;
    sampleAccumulator = 0.0;
}

//==============================================================================
// Helper Methods
//==============================================================================

void OFreqPulseAudioProcessor::recalculateBinMapping()
{
    // Derive 4 band boundaries from 3 crossover points
    float c1 = crossover1Param->load();
    float c2 = crossover2Param->load();
    float c3 = crossover3Param->load();

    // Sort to guarantee ordering (handles automation edge cases)
    if (c1 > c2) std::swap(c1, c2);
    if (c2 > c3) std::swap(c2, c3);
    if (c1 > c2) std::swap(c1, c2);

    float fLow = freqLowParam->load();
    float fHigh = freqHighParam->load();

    float bandLows[4]  = { fLow, c1, c2, c3 };
    float bandHighs[4] = { c1, c2, c3, fHigh };

    // Map each FFT bin to a band
    for (int bin = 0; bin < numBins; ++bin)
    {
        // Calculate frequency for this bin
        float freq = static_cast<float>(bin) * static_cast<float>(currentSampleRate) / static_cast<float>(fftSize);

        // Find which band this bin belongs to
        bandForBin[bin] = -1;  // Default: passthrough (no band)

        for (int band = 0; band < 4; ++band)
        {
            if (freq >= bandLows[band] && freq < bandHighs[band])
            {
                bandForBin[bin] = band;
                break;
            }
        }
    }
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
        int steps = static_cast<int>(bandParams[band].eucSteps->load());
        int pulses = static_cast<int>(bandParams[band].eucPulses->load());
        int offset = static_cast<int>(bandParams[band].eucOffset->load());

        euclideanPatterns[band] = generateEuclidean(steps, pulses, offset);
    }
}

int OFreqPulseAudioProcessor::calculateCurrentStep(double ppq, int numSteps, int rateIndex, float swing)
{
    // PPQ values for rate options
    const double ppqPerStep[] = {
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

float OFreqPulseAudioProcessor::getTargetGainForBand(int bandIndex, int currentStep)
{
    // Check if band is enabled
    bool enabled = bandParams[bandIndex].enable->load() > 0.5f;
    if (!enabled)
        return 1.0f;  // Passthrough when disabled

    // Check if using Euclidean mode
    bool eucOn = bandParams[bandIndex].eucOn->load() > 0.5f;
    bool stepActive;

    if (eucOn)
    {
        // Use pre-generated Euclidean pattern
        int eucSteps = static_cast<int>(bandParams[bandIndex].eucSteps->load());
        int wrappedStep = currentStep % eucSteps;
        stepActive = euclideanPatterns[bandIndex][wrappedStep];
    }
    else
    {
        // Use manual step grid
        stepActive = bandParams[bandIndex].stepStates[currentStep]->load() > 0.5f;
    }

    // Calculate gain
    float depth = bandParams[bandIndex].depth->load();

    if (stepActive)
        return 1.0f;  // Step ON: full volume
    else
        return 1.0f - depth;  // Step OFF: reduce by depth amount
}

void OFreqPulseAudioProcessor::processFrame(int channel)
{
    auto& fftBuffer = fftData[channel];
    auto& inFifo = inputFifo[channel];
    auto& tempBuffer = fftBandTemp[channel];

    // Copy input samples from circular buffer to FFT buffer
    for (int i = 0; i < fftSize; ++i)
    {
        fftBuffer[static_cast<size_t>(i)] = inFifo[static_cast<size_t>((inputWritePos + i) % fftSize)];
    }

    // Apply analysis window (Hann)
    for (int i = 0; i < fftSize; ++i)
    {
        fftBuffer[static_cast<size_t>(i)] *= hannWindow[static_cast<size_t>(i)];
    }

    // Forward FFT (real-only)
    fft.performRealOnlyForwardTransform(fftBuffer.data());

    // v1.2.0: Reconstruct each band separately into its own time-domain FIFO.
    // This allows per-sample gain application in the time domain, avoiding
    // inter-frame modulation artifacts (~86Hz buzz) that occurred when applying
    // gain in the spectral domain with overlapping STFT frames.

    // For each band: extract only that band's bins, IFFT, window, overlap-add
    for (int band = 0; band < 4; ++band)
    {
        // Zero the temp buffer
        std::fill(tempBuffer.begin(), tempBuffer.end(), 0.0f);

        // Copy only bins belonging to this band
        for (int bin = 0; bin < numBins; ++bin)
        {
            if (bandForBin[bin] == band)
            {
                int realIndex = bin * 2;
                int imagIndex = bin * 2 + 1;

                if (realIndex < fftSize * 2)
                    tempBuffer[realIndex] = fftBuffer[realIndex];
                if (imagIndex < fftSize * 2)
                    tempBuffer[imagIndex] = fftBuffer[imagIndex];
            }
        }

        // Inverse FFT for this band
        fft.performRealOnlyInverseTransform(tempBuffer.data());

        // Apply synthesis window and COLA correction
        for (int i = 0; i < fftSize; ++i)
        {
            tempBuffer[static_cast<size_t>(i)] *= hannWindow[static_cast<size_t>(i)] * windowCorrection;
        }

        // Overlap-add to this band's output FIFO
        for (int i = 0; i < fftSize; ++i)
        {
            bandOutputFifo[band][channel][i] += tempBuffer[i];
        }
    }

    // Reconstruct passthrough bins (bins not assigned to any band)
    {
        std::fill(tempBuffer.begin(), tempBuffer.end(), 0.0f);

        for (int bin = 0; bin < numBins; ++bin)
        {
            if (bandForBin[bin] < 0 || bandForBin[bin] >= 4)
            {
                int realIndex = bin * 2;
                int imagIndex = bin * 2 + 1;

                if (realIndex < fftSize * 2)
                    tempBuffer[realIndex] = fftBuffer[realIndex];
                if (imagIndex < fftSize * 2)
                    tempBuffer[imagIndex] = fftBuffer[imagIndex];
            }
        }

        fft.performRealOnlyInverseTransform(tempBuffer.data());

        for (int i = 0; i < fftSize; ++i)
        {
            tempBuffer[static_cast<size_t>(i)] *= hannWindow[static_cast<size_t>(i)] * windowCorrection;
        }

        for (int i = 0; i < fftSize; ++i)
        {
            passthroughOutputFifo[channel][i] += tempBuffer[i];
        }
    }
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
    int stepsIndex = static_cast<int>(stepsParam->load());
    int rateIndex = static_cast<int>(rateParam->load());
    float swing = swingParam->load();
    float smoothingMs = smoothingParam->load();

    // Map steps index to actual step count
    const int stepCounts[] = { 4, 8, 16, 32 };
    int numSteps = stepCounts[juce::jlimit(0, 3, stepsIndex)];

    // Check for parameter changes
    bool crossoversChanged = false;
    bool euclideanParamsChanged = false;

    // Check crossover changes (3 values instead of 8 per-band freqs)
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

    // Check frequency boundary changes
    {
        float fb[2] = { freqLowParam->load(), freqHighParam->load() };
        for (int i = 0; i < 2; ++i)
        {
            if (std::abs(fb[i] - lastFreqBounds[i]) > 0.1f)
            {
                lastFreqBounds[i] = fb[i];
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

    // Only reconfigure smoothing ramp when the parameter actually changes.
    // reset() calls setCurrentAndTargetValue(target) internally, which snaps
    // the current value to the target and kills any in-progress ramp.
    if (std::abs(smoothingMs - lastSmoothingMs) > 0.01f)
    {
        for (int band = 0; band < 4; ++band)
            bandGainSmooth[band].reset(currentSampleRate, smoothingMs / 1000.0);
        lastSmoothingMs = smoothingMs;
    }

    if (crossoversChanged)
        recalculateBinMapping();

    if (euclideanParamsChanged)
        updateEuclideanPatterns();

    // Detect whether audio signal is present (RMS check across all channels)
    {
        float sumSquares = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                sumSquares += data[i] * data[i];
        }
        float rms = std::sqrt(sumSquares / static_cast<float>(numSamples * juce::jmax(numChannels, 1)));
        constexpr float silenceThreshold = 0.001f;  // ~-60 dB
        hasAudioSignal.store(rms >= silenceThreshold);
    }

    bool signalPresent = hasAudioSignal.load();

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

                // Calculate PPQ increment per sample for interpolation
                if (posInfo->getBpm().hasValue())
                {
                    double bpm = *posInfo->getBpm();
                    ppqPerSample = bpm / (60.0 * currentSampleRate);
                }
                else
                {
                    ppqPerSample = 120.0 / (60.0 * currentSampleRate);  // Fallback 120 BPM
                }
            }
        }
    }

    // Fallback: free-running PPQ when no valid host position (e.g., Standalone)
    if (!gotValidPosition && signalPresent)
    {
        ppqPerSample = 120.0 / (60.0 * currentSampleRate);  // 120 BPM
        blockStartPpq = lastPpqPosition >= 0.0 ? lastPpqPosition : 0.0;
        gotValidPosition = true;  // Use free-running mode
    }

    // Set initial step and gain targets at block start
    if (gotValidPosition && signalPresent)
    {
        currentStep = calculateCurrentStep(blockStartPpq, numSteps, rateIndex, swing);
        currentStepAtomic.store(currentStep);

        for (int band = 0; band < 4; ++band)
        {
            float targetGain = getTargetGainForBand(band, currentStep);
            bandGainSmooth[band].setTargetValue(targetGain);
        }
    }

    // Push dry samples to mixer
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    dryWetMixer.pushDrySamples(block);

    // Process each sample through STFT with sample-accurate step tracking
    int prevStep = currentStep;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Sample-accurate step tracking: interpolate PPQ and detect transitions
        if (gotValidPosition && signalPresent)
        {
            double samplePpq = blockStartPpq + ppqPerSample * static_cast<double>(sample);
            int stepAtSample = calculateCurrentStep(samplePpq, numSteps, rateIndex, swing);

            if (stepAtSample != prevStep)
            {
                // Step transition detected at this exact sample — update gain targets
                currentStep = stepAtSample;
                currentStepAtomic.store(currentStep);
                prevStep = stepAtSample;

                for (int band = 0; band < 4; ++band)
                {
                    float targetGain = getTargetGainForBand(band, currentStep);
                    bandGainSmooth[band].setTargetValue(targetGain);
                }
            }
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            // Push sample to input FIFO
            inputFifo[ch][static_cast<size_t>(inputWritePos)] = buffer.getSample(ch, sample);
        }

        inputWritePos = (inputWritePos + 1) % fftSize;

        // Process FFT frame when we've accumulated enough samples
        if (hopCounter >= hopSize)
        {
            // Process each channel — no spectral gain applied here (v1.2.0)
            for (int ch = 0; ch < numChannels; ++ch)
            {
                processFrame(ch);

                // Shift per-band output FIFOs by hop size
                for (int band = 0; band < 4; ++band)
                {
                    std::rotate(bandOutputFifo[band][ch].begin(),
                               bandOutputFifo[band][ch].begin() + hopSize,
                               bandOutputFifo[band][ch].end());
                    std::fill(bandOutputFifo[band][ch].end() - hopSize,
                             bandOutputFifo[band][ch].end(), 0.0f);
                }

                // Shift passthrough output FIFO
                std::rotate(passthroughOutputFifo[ch].begin(),
                           passthroughOutputFifo[ch].begin() + hopSize,
                           passthroughOutputFifo[ch].end());
                std::fill(passthroughOutputFifo[ch].end() - hopSize,
                         passthroughOutputFifo[ch].end(), 0.0f);
            }

            hopCounter = 0;
        }

        // v1.2.0: Apply per-sample smoothed gain in the TIME DOMAIN
        float bandGainValues[4];
        for (int band = 0; band < 4; ++band)
        {
            bandGainValues[band] = bandGainSmooth[band].getNextValue();
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float outputSample = 0.0f;

            // Sum per-band contributions with per-sample gain
            for (int band = 0; band < 4; ++band)
            {
                outputSample += bandOutputFifo[band][ch][static_cast<size_t>(hopCounter)]
                              * bandGainValues[band];
            }

            // Add passthrough (unmodified bins not in any band)
            outputSample += passthroughOutputFifo[ch][static_cast<size_t>(hopCounter)];

            buffer.setSample(ch, sample, outputSample);
        }

        hopCounter++;
    }

    // Update lastPpqPosition for next block
    if (gotValidPosition)
        lastPpqPosition = blockStartPpq + ppqPerSample * static_cast<double>(numSamples);

    // Mix dry/wet
    dryWetMixer.setWetMixProportion(mix);
    dryWetMixer.mixWetSamples(block);
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
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OFreqPulseAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Factory Presets
//==============================================================================

// Preset names
static const juce::StringArray presetNames = {
    "Init",                    // 0: Default starting point
    "Classic Sidechain",       // 1: Sub solid, mids pump at 1/4
    "Trance Gate 16th",        // 2: All bands 16th note gating
    "Dubstep Pulse",           // 3: Heavy sub gate, minimal highs
    "Ambient Shimmer",         // 4: Slow Euclidean on highs, high smoothing
    "Polyrhythm 5-7-11",       // 5: Different Euclidean ratios per band
    "Bass Foundation",         // 6: Sub always on, others gated
    "Hi-Hat Chop",             // 7: Only high band gated fast
    "Full Spectrum Gate",      // 8: Unified gating across all bands
    "Euclidean Groove",        // 9: All bands Euclidean, musical ratios
    "Half-Time Feel",          // 10: Slower rate, dramatic pumping
    "Triplet Bounce"           // 11: Triplet timing groove
};

int OFreqPulseAudioProcessor::getNumPrograms()
{
    return numPresets;
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
    auto* smoothing = parameters.getParameter("smoothing");

    // Helper lambda to set step pattern for a band
    auto setStepPattern = [this](int band, const std::array<bool, 32>& pattern) {
        for (int i = 0; i < 32; ++i)
        {
            juce::String stepID = "step_b" + juce::String(band) + "_s" + juce::String(i);
            if (auto* param = parameters.getParameter(stepID))
                param->setValueNotifyingHost(pattern[i] ? 1.0f : 0.0f);
        }
    };

    // Helper lambda for band parameters
    auto setBandParams = [this](int band, bool enable, bool eucOn, int eucSteps, int eucPulses, int eucOffset, float depth) {
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
        if (auto* p = parameters.getParameter(bandID + "_depth"))
            p->setValueNotifyingHost(depth);
    };

    // Default step pattern (alternating 8th notes)
    std::array<bool, 32> patternAlt8th = {true, false, true, false, true, false, true, false,
                                          true, false, true, false, true, false, true, false,
                                          true, false, true, false, true, false, true, false,
                                          true, false, true, false, true, false, true, false};

    // 16th note pattern (all on)
    std::array<bool, 32> patternAll = {true, true, true, true, true, true, true, true,
                                        true, true, true, true, true, true, true, true,
                                        true, true, true, true, true, true, true, true,
                                        true, true, true, true, true, true, true, true};

    // Quarter note pattern
    std::array<bool, 32> patternQuarter = {true, false, false, false, true, false, false, false,
                                           true, false, false, false, true, false, false, false,
                                           true, false, false, false, true, false, false, false,
                                           true, false, false, false, true, false, false, false};

    // Empty pattern
    std::array<bool, 32> patternEmpty = {};

    switch (presetIndex)
    {
        case 0:  // Init - Clean starting point
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(2.0f));  // 16 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));    // 1/16
            swing->setValueNotifyingHost(0.0f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(5.0f));

            for (int b = 0; b < 4; ++b)
            {
                setBandParams(b, true, false, 16, 8, 0, 1.0f);
                setStepPattern(b, patternEmpty);
            }
            break;

        case 1:  // Classic Sidechain - Sub solid, mids pump at 1/4
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(2.0f));  // 16 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(2.0f));    // 1/4
            swing->setValueNotifyingHost(0.0f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(20.0f));

            setBandParams(0, true, false, 16, 16, 0, 0.0f);  // Sub: no gating
            setBandParams(1, true, false, 16, 8, 0, 0.8f);   // Low: pumping
            setBandParams(2, true, false, 16, 8, 0, 1.0f);   // Mid: full pump
            setBandParams(3, true, false, 16, 8, 0, 0.6f);   // High: subtle

            setStepPattern(0, patternAll);
            setStepPattern(1, patternQuarter);
            setStepPattern(2, patternQuarter);
            setStepPattern(3, patternQuarter);
            break;

        case 2:  // Trance Gate 16th
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(2.0f));  // 16 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));    // 1/16
            swing->setValueNotifyingHost(0.0f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(3.0f));

            for (int b = 0; b < 4; ++b)
            {
                setBandParams(b, true, false, 16, 8, 0, 1.0f);
                setStepPattern(b, patternAlt8th);
            }
            break;

        case 3:  // Dubstep Pulse - Heavy sub gate
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(1.0f));  // 8 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(3.0f));    // 1/8
            swing->setValueNotifyingHost(0.0f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(2.0f));

            setBandParams(0, true, true, 8, 5, 0, 1.0f);     // Sub: Euclidean 5/8
            setBandParams(1, true, true, 8, 3, 0, 0.9f);     // Low: Euclidean 3/8
            setBandParams(2, true, false, 8, 8, 0, 0.3f);    // Mid: subtle
            setBandParams(3, true, false, 8, 8, 0, 0.1f);    // High: minimal
            break;

        case 4:  // Ambient Shimmer - Slow highs
            mix->setValueNotifyingHost(0.7f);
            steps->setValueNotifyingHost(steps->convertTo0to1(3.0f));  // 32 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(1.0f));    // 1/2
            swing->setValueNotifyingHost(0.2f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(50.0f));

            setBandParams(0, true, false, 32, 32, 0, 0.0f);  // Sub: no gating
            setBandParams(1, true, false, 32, 32, 0, 0.0f);  // Low: no gating
            setBandParams(2, true, true, 32, 7, 0, 0.5f);    // Mid: slow Euclidean
            setBandParams(3, true, true, 32, 11, 3, 0.7f);   // High: sparse Euclidean
            break;

        case 5:  // Polyrhythm 5-7-11
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(2.0f));  // 16 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));    // 1/16
            swing->setValueNotifyingHost(0.0f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(5.0f));

            setBandParams(0, true, true, 16, 5, 0, 1.0f);    // Sub: 5 pulses
            setBandParams(1, true, true, 16, 7, 2, 1.0f);    // Low: 7 pulses, offset
            setBandParams(2, true, true, 16, 11, 5, 1.0f);   // Mid: 11 pulses, offset
            setBandParams(3, true, true, 16, 13, 1, 0.8f);   // High: 13 pulses
            break;

        case 6:  // Bass Foundation - Sub always on
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(2.0f));  // 16 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));    // 1/16
            swing->setValueNotifyingHost(0.0f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(5.0f));

            setBandParams(0, false, false, 16, 8, 0, 0.0f);  // Sub: bypass (always on)
            setBandParams(1, true, true, 16, 12, 0, 0.8f);   // Low: Euclidean
            setBandParams(2, true, true, 16, 8, 0, 1.0f);    // Mid: Euclidean
            setBandParams(3, true, true, 16, 8, 2, 1.0f);    // High: Euclidean offset
            break;

        case 7:  // Hi-Hat Chop - Only highs gated
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(3.0f));  // 32 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(5.0f));    // 1/32
            swing->setValueNotifyingHost(0.3f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(1.0f));

            setBandParams(0, false, false, 16, 8, 0, 0.0f);  // Sub: bypass
            setBandParams(1, false, false, 16, 8, 0, 0.0f);  // Low: bypass
            setBandParams(2, false, false, 16, 8, 0, 0.0f);  // Mid: bypass
            setBandParams(3, true, true, 32, 12, 0, 1.0f);   // High: fast Euclidean
            break;

        case 8:  // Full Spectrum Gate - Unified gating
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(2.0f));  // 16 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(4.0f));    // 1/16
            swing->setValueNotifyingHost(0.0f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(3.0f));

            for (int b = 0; b < 4; ++b)
            {
                setBandParams(b, true, true, 16, 8, 0, 1.0f);
            }
            break;

        case 9:  // Euclidean Groove - Musical ratios
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(2.0f));  // 16 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(3.0f));    // 1/8
            swing->setValueNotifyingHost(0.15f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(8.0f));

            setBandParams(0, true, true, 16, 4, 0, 1.0f);    // Sub: 4/16 (quarter feel)
            setBandParams(1, true, true, 16, 6, 1, 0.9f);    // Low: 6/16
            setBandParams(2, true, true, 16, 9, 0, 0.85f);   // Mid: 9/16
            setBandParams(3, true, true, 16, 11, 2, 0.8f);   // High: 11/16
            break;

        case 10:  // Half-Time Feel - Slower, dramatic
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(1.0f));  // 8 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(2.0f));    // 1/4
            swing->setValueNotifyingHost(0.0f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(30.0f));

            for (int b = 0; b < 4; ++b)
            {
                setBandParams(b, true, true, 8, 2, 0, 1.0f);  // Only 2 pulses in 8
            }
            break;

        case 11:  // Triplet Bounce
            mix->setValueNotifyingHost(1.0f);
            steps->setValueNotifyingHost(steps->convertTo0to1(2.0f));  // 16 steps
            rate->setValueNotifyingHost(rate->convertTo0to1(6.0f));    // 1/8T (triplet)
            swing->setValueNotifyingHost(0.0f);
            smoothing->setValueNotifyingHost(smoothing->convertTo0to1(5.0f));

            setBandParams(0, true, true, 12, 4, 0, 1.0f);    // Sub: 4/12
            setBandParams(1, true, true, 12, 5, 1, 0.9f);    // Low: 5/12
            setBandParams(2, true, true, 12, 7, 0, 1.0f);    // Mid: 7/12
            setBandParams(3, true, true, 12, 8, 2, 0.8f);    // High: 8/12
            break;

        default:
            break;
    }
}

//==============================================================================
// Plugin Factory
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OFreqPulseAudioProcessor();
}
