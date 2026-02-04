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

    layout.add(std::move(globalGroup));

    // Per-Band Parameters (4 bands)
    const juce::String bandNames[4] = { "Sub", "Low", "Mid", "High" };
    const float lowDefaults[4] = { 20.0f, 120.0f, 500.0f, 4000.0f };
    const float highDefaults[4] = { 120.0f, 500.0f, 4000.0f, 20000.0f };

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
            juce::ParameterID { bandID + "_low", 1 },
            bandName + " Low",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
            lowDefaults[n]));

        bandGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandID + "_high", 1 },
            bandName + " High",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
            highDefaults[n]));

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

    // Cache per-band parameter pointers
    for (int n = 0; n < 4; ++n)
    {
        juce::String bandID = "band" + juce::String(n);

        bandParams[n].enable = parameters.getRawParameterValue(bandID + "_enable");
        bandParams[n].low = parameters.getRawParameterValue(bandID + "_low");
        bandParams[n].high = parameters.getRawParameterValue(bandID + "_high");
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
    // Read band frequency parameters
    float bandLows[4];
    float bandHighs[4];

    for (int band = 0; band < 4; ++band)
    {
        bandLows[band] = bandParams[band].low->load();
        bandHighs[band] = bandParams[band].high->load();
    }

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
    auto& outFifo = outputFifo[channel];

    // Copy input samples from circular buffer to FFT buffer
    // inputWritePos points to where the NEXT sample will go (oldest position)
    // Extract fftSize samples in correct order: oldest to newest
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

    // Apply band gains to frequency bins
    for (int bin = 0; bin < numBins; ++bin)
    {
        int band = bandForBin[bin];
        float gain;

        if (band >= 0 && band < 4)
        {
            // Get smoothed gain for this band
            gain = bandGainSmooth[band].getNextValue();
        }
        else
        {
            // Passthrough (gap between bands)
            gain = 1.0f;
        }

        // Scale both real and imaginary parts (phase preservation)
        int realIndex = bin * 2;
        int imagIndex = bin * 2 + 1;

        if (realIndex < fftSize * 2)
            fftBuffer[realIndex] *= gain;
        if (imagIndex < fftSize * 2)
            fftBuffer[imagIndex] *= gain;
    }

    // Inverse FFT
    fft.performRealOnlyInverseTransform(fftBuffer.data());

    // Apply synthesis window and COLA correction
    for (int i = 0; i < fftSize; ++i)
    {
        fftBuffer[static_cast<size_t>(i)] *= hannWindow[static_cast<size_t>(i)] * windowCorrection;
    }

    // Overlap-add to output FIFO
    for (int i = 0; i < fftSize; ++i)
    {
        outFifo[i] += fftBuffer[i];
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

    // Check for parameter changes (Task 10)
    bool bandFreqsChanged = false;
    bool euclideanParamsChanged = false;

    for (int band = 0; band < 4; ++band)
    {
        float low = bandParams[band].low->load();
        float high = bandParams[band].high->load();

        if (std::abs(low - lastBandFreqs[band][0]) > 0.1f ||
            std::abs(high - lastBandFreqs[band][1]) > 0.1f)
        {
            lastBandFreqs[band][0] = low;
            lastBandFreqs[band][1] = high;
            bandFreqsChanged = true;
        }

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

        // Update smoothing time if changed
        bandGainSmooth[band].reset(currentSampleRate, smoothingMs / 1000.0);
    }

    if (bandFreqsChanged)
        recalculateBinMapping();

    if (euclideanParamsChanged)
        updateEuclideanPatterns();

    // Get playhead position for tempo sync
    bool gotValidPosition = false;
    juce::AudioPlayHead* playHead = getPlayHead();

    if (playHead != nullptr)
    {
        if (auto posInfo = playHead->getPosition())
        {
            bool isPlaying = posInfo->getIsPlaying();

            if (isPlaying && posInfo->getPpqPosition().hasValue())
            {
                double ppq = *posInfo->getPpqPosition();
                gotValidPosition = true;

                // Calculate current step directly from PPQ (loops via modulo in calculateCurrentStep)
                currentStep = calculateCurrentStep(ppq, numSteps, rateIndex, swing);
                currentStepAtomic.store(currentStep);

                // Update target gains when PPQ changes
                if (std::abs(ppq - lastPpqPosition) > 0.001)
                {
                    for (int band = 0; band < 4; ++band)
                    {
                        float targetGain = getTargetGainForBand(band, currentStep);
                        bandGainSmooth[band].setTargetValue(targetGain);
                    }
                    lastPpqPosition = ppq;
                }
            }
        }
    }

    // Fallback: free-running step counter when no valid host position (e.g., Standalone)
    if (!gotValidPosition)
    {
        // Use sample-based timing at assumed 120 BPM
        // PPQ per step at different rates
        const double ppqPerStep[] = {
            4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 0.333333, 0.166666, 1.5, 0.75
        };
        double stepLength = ppqPerStep[juce::jlimit(0, 9, rateIndex)];

        // Samples per beat at 120 BPM
        double samplesPerBeat = currentSampleRate * 60.0 / 120.0;
        double samplesPerStep = samplesPerBeat * stepLength;

        // Accumulate samples and advance step
        sampleAccumulator += static_cast<double>(numSamples);

        if (sampleAccumulator >= samplesPerStep)
        {
            sampleAccumulator -= samplesPerStep;
            currentStep = (currentStep + 1) % numSteps;
            currentStepAtomic.store(currentStep);

            // Update target gains
            for (int band = 0; band < 4; ++band)
            {
                float targetGain = getTargetGainForBand(band, currentStep);
                bandGainSmooth[band].setTargetValue(targetGain);
            }
        }
    }

    // Push dry samples to mixer
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    dryWetMixer.pushDrySamples(block);

    // Process each sample through STFT
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            // Push sample to input FIFO
            inputFifo[ch][static_cast<size_t>(inputWritePos)] = buffer.getSample(ch, sample);
        }

        inputWritePos = (inputWritePos + 1) % fftSize;

        // Process FFT frame when we've accumulated enough samples
        if (hopCounter >= hopSize)
        {
            // Process each channel
            for (int ch = 0; ch < numChannels; ++ch)
            {
                processFrame(ch);

                // Shift output FIFO by hop size (discards first hopSize samples)
                std::rotate(outputFifo[ch].begin(),
                           outputFifo[ch].begin() + hopSize,
                           outputFifo[ch].end());

                // Zero the end part for next overlap-add
                std::fill(outputFifo[ch].end() - hopSize, outputFifo[ch].end(), 0.0f);
            }

            hopCounter = 0;
        }

        // Read output from FIFO - use hopCounter which cycles [0, hopSize)
        // After rotation, valid output is always at positions [0, hopSize-1]
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.setSample(ch, sample, outputFifo[ch][static_cast<size_t>(hopCounter)]);
        }

        hopCounter++;
    }

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
// Plugin Factory
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OFreqPulseAudioProcessor();
}
