/*
  ==============================================================================

    O-MultiBandCompressor - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Parameter layout creation (BEFORE constructor)
juce::AudioProcessorValueTreeState::ParameterLayout OMultiBandCompressorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ===== GLOBAL PARAMETERS (8) =====

    // INPUT_GAIN: -24 to +24 dB, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "INPUT_GAIN", 1 },
        "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // OUTPUT_GAIN: -24 to +24 dB, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // MIX: 0-100%, default 100%
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // AUTO_MAKEUP: bool, default false
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "AUTO_MAKEUP", 1 },
        "Auto-Makeup",
        false
    ));

    // MS_MODE: choice (Off/Mid/Side/Both), default Off (0)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MS_MODE", 1 },
        "M/S Mode",
        juce::StringArray { "Off", "Mid", "Side", "Both" },
        0
    ));

    // XOVER1: 20-500 Hz, default 200 Hz (logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "XOVER1", 1 },
        "Crossover 1",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.1f, 0.3f),
        200.0f,
        "Hz"
    ));

    // XOVER2: 200-5000 Hz, default 2000 Hz (logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "XOVER2", 1 },
        "Crossover 2",
        juce::NormalisableRange<float>(200.0f, 5000.0f, 0.1f, 0.3f),
        2000.0f,
        "Hz"
    ));

    // XOVER3: 2000-16000 Hz, default 8000 Hz (logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "XOVER3", 1 },
        "Crossover 3",
        juce::NormalisableRange<float>(2000.0f, 16000.0f, 0.1f, 0.3f),
        8000.0f,
        "Hz"
    ));

    // ===== PER-BAND PARAMETERS (12 × 4 = 48) =====

    juce::StringArray bandPrefixes = { "LOW", "LOMID", "HIMID", "HIGH" };
    juce::StringArray bandNames = { "Low", "Low-Mid", "High-Mid", "High" };

    for (int i = 0; i < 4; ++i)
    {
        const auto& prefix = bandPrefixes[i];
        const auto& name = bandNames[i];

        // THRESHOLD: -60 to 0 dB, default -20
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_THRESHOLD", 1 },
            name + " Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
            -20.0f,
            "dB"
        ));

        // RATIO: 1 to 20 (1:1 to 20:1), default 4
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_RATIO", 1 },
            name + " Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
            4.0f,
            ":1"
        ));

        // ATTACK: 0.1 to 200 ms, default 10
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_ATTACK", 1 },
            name + " Attack",
            juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.3f),
            10.0f,
            "ms"
        ));

        // RELEASE: 10 to 2000 ms, default 100
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_RELEASE", 1 },
            name + " Release",
            juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.3f),
            100.0f,
            "ms"
        ));

        // KNEE: 0 to 24 dB, default 6
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_KNEE", 1 },
            name + " Knee",
            juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f),
            6.0f,
            "dB"
        ));

        // MAKEUP: -12 to +24 dB, default 0
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_MAKEUP", 1 },
            name + " Makeup",
            juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f),
            0.0f,
            "dB"
        ));

        // PEAK_RMS: 0-100%, default 50%
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_PEAK_RMS", 1 },
            name + " Peak/RMS",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            50.0f,
            "%"
        ));

        // SOLO: bool, default false
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "_SOLO", 1 },
            name + " Solo",
            false
        ));

        // BYPASS: bool, default false
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "_BYPASS", 1 },
            name + " Bypass",
            false
        ));

        // SC_HPF: 20-2000 Hz, default 0 (off)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_SC_HPF", 1 },
            name + " SC HPF",
            juce::NormalisableRange<float>(0.0f, 2000.0f, 0.1f, 0.3f),
            0.0f,
            "Hz"
        ));

        // SC_LPF: 500-20000 Hz, default 0 (off)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_SC_LPF", 1 },
            name + " SC LPF",
            juce::NormalisableRange<float>(0.0f, 20000.0f, 0.1f, 0.3f),
            0.0f,
            "Hz"
        ));

        // SC_LISTEN: bool, default false
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "_SC_LISTEN", 1 },
            name + " SC Listen",
            false
        ));
    }

    return layout;
}

OMultiBandCompressorAudioProcessor::OMultiBandCompressorAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OMultiBandCompressorAudioProcessor::~OMultiBandCompressorAudioProcessor()
{
}

void OMultiBandCompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare multiband processor (crossover + 4 compressors)
    multibandProcessor.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    // Prepare gain stages
    inputGain.prepare(spec);
    outputGain.prepare(spec);

    // Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::linear);

    // Reset components to initial state
    multibandProcessor.reset();
    inputGain.reset();
    outputGain.reset();
    dryWetMixer.reset();
}

void OMultiBandCompressorAudioProcessor::releaseResources()
{
    // Release resources when plugin not in use
}

void OMultiBandCompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Early exit on empty buffer
    if (buffer.getNumSamples() == 0)
        return;

    // ===== Phase 5.3: Measure Input Levels =====
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels >= 1)
    {
        float inputMaxL = buffer.getMagnitude(0, 0, numSamples);
        inputLevelL.store(inputMaxL, std::memory_order_relaxed);
    }

    if (numChannels >= 2)
    {
        float inputMaxR = buffer.getMagnitude(1, 0, numSamples);
        inputLevelR.store(inputMaxR, std::memory_order_relaxed);
    }

    // ===== Read Parameters (atomic, real-time safe) =====

    // Global parameters
    auto* inputGainParam = parameters.getRawParameterValue("INPUT_GAIN");
    float inputGainDB = inputGainParam->load();

    auto* outputGainParam = parameters.getRawParameterValue("OUTPUT_GAIN");
    float outputGainDB = outputGainParam->load();

    auto* mixParam = parameters.getRawParameterValue("MIX");
    float mixPercent = mixParam->load();

    auto* autoMakeupParam = parameters.getRawParameterValue("AUTO_MAKEUP");
    bool autoMakeupEnabled = autoMakeupParam->load() > 0.5f;

    auto* msModeParam = parameters.getRawParameterValue("MS_MODE");
    int msMode = static_cast<int>(msModeParam->load());

    // Crossover frequencies
    auto* xover1Param = parameters.getRawParameterValue("XOVER1");
    float xover1 = xover1Param->load();

    auto* xover2Param = parameters.getRawParameterValue("XOVER2");
    float xover2 = xover2Param->load();

    auto* xover3Param = parameters.getRawParameterValue("XOVER3");
    float xover3 = xover3Param->load();

    // Per-band parameters (arrays indexed by band: 0=LOW, 1=LOMID, 2=HIMID, 3=HIGH)
    const char* bandPrefixes[4] = { "LOW", "LOMID", "HIMID", "HIGH" };

    float thresholds[4];
    float ratios[4];
    float attacks[4];
    float releases[4];
    float knees[4];
    float makeups[4];
    float peakRmsBlends[4];
    bool bypasses[4];
    bool solos[4];
    float scHPFs[4];
    float scLPFs[4];
    bool scListens[4];

    for (int band = 0; band < 4; ++band)
    {
        juce::String prefix = bandPrefixes[band];

        thresholds[band] = parameters.getRawParameterValue(prefix + "_THRESHOLD")->load();
        ratios[band] = parameters.getRawParameterValue(prefix + "_RATIO")->load();
        attacks[band] = parameters.getRawParameterValue(prefix + "_ATTACK")->load();
        releases[band] = parameters.getRawParameterValue(prefix + "_RELEASE")->load();
        knees[band] = parameters.getRawParameterValue(prefix + "_KNEE")->load();
        makeups[band] = parameters.getRawParameterValue(prefix + "_MAKEUP")->load();
        peakRmsBlends[band] = parameters.getRawParameterValue(prefix + "_PEAK_RMS")->load();
        bypasses[band] = parameters.getRawParameterValue(prefix + "_BYPASS")->load() > 0.5f;
        solos[band] = parameters.getRawParameterValue(prefix + "_SOLO")->load() > 0.5f;
        scHPFs[band] = parameters.getRawParameterValue(prefix + "_SC_HPF")->load();
        scLPFs[band] = parameters.getRawParameterValue(prefix + "_SC_LPF")->load();
        scListens[band] = parameters.getRawParameterValue(prefix + "_SC_LISTEN")->load() > 0.5f;
    }

    // ===== Process Audio =====

    // Apply input gain
    inputGain.setGainDecibels(inputGainDB);
    juce::dsp::AudioBlock<float> inputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> inputContext(inputBlock);
    inputGain.process(inputContext);

    // Set dry/wet mix ratio (0-100% to 0.0-1.0)
    dryWetMixer.setWetMixProportion(mixPercent / 100.0f);

    // Capture dry signal for parallel compression
    juce::dsp::AudioBlock<float> dryWetBlock(buffer);
    dryWetMixer.pushDrySamples(dryWetBlock);

    // M/S Encoding (if enabled)

    if (msMode > 0 && numChannels == 2)
    {
        // Encode L/R to M/S (power-preserving)
        const float sqrtHalf = 0.70710678f; // 1/sqrt(2)

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float left = buffer.getSample(0, sample);
            float right = buffer.getSample(1, sample);

            float mid = (left + right) * sqrtHalf;
            float side = (left - right) * sqrtHalf;

            buffer.setSample(0, sample, mid);
            buffer.setSample(1, sample, side);
        }
    }

    // Update crossover frequencies
    multibandProcessor.updateCrossoverFrequencies(xover1, xover2, xover3);

    // Update compressor attack/release times
    multibandProcessor.setAttackTimes(attacks);
    multibandProcessor.setReleaseTimes(releases);

    // Array of gain reduction meters
    std::atomic<float>* grMeters[4] = {
        &lowBandGainReduction,
        &loMidBandGainReduction,
        &hiMidBandGainReduction,
        &highBandGainReduction
    };

    // Process based on M/S mode
    if (msMode == 0 || numChannels != 2)
    {
        // Mode 0: Off - Process L/R independently (standard stereo)
        multibandProcessor.processMultiband(
            buffer,
            thresholds,
            ratios,
            knees,
            makeups,
            peakRmsBlends,
            bypasses,
            solos,
            scHPFs,
            scLPFs,
            scListens,
            autoMakeupEnabled,
            grMeters
        );
    }
    else if (msMode == 1)
    {
        // Mode 1: Mid - Compress only mid (channel 0), side passes through
        juce::AudioBuffer<float> midBuffer(1, numSamples);
        midBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);

        multibandProcessor.processMultiband(
            midBuffer,
            thresholds,
            ratios,
            knees,
            makeups,
            peakRmsBlends,
            bypasses,
            solos,
            scHPFs,
            scLPFs,
            scListens,
            autoMakeupEnabled,
            grMeters
        );

        buffer.copyFrom(0, 0, midBuffer, 0, 0, numSamples);
    }
    else if (msMode == 2)
    {
        // Mode 2: Side - Compress only side (channel 1), mid passes through
        juce::AudioBuffer<float> sideBuffer(1, numSamples);
        sideBuffer.copyFrom(0, 0, buffer, 1, 0, numSamples);

        multibandProcessor.processMultiband(
            sideBuffer,
            thresholds,
            ratios,
            knees,
            makeups,
            peakRmsBlends,
            bypasses,
            solos,
            scHPFs,
            scLPFs,
            scListens,
            autoMakeupEnabled,
            grMeters
        );

        buffer.copyFrom(1, 0, sideBuffer, 0, 0, numSamples);
    }
    else if (msMode == 3)
    {
        // Mode 3: Both - Independent compression for mid AND side
        multibandProcessor.processMultiband(
            buffer,
            thresholds,
            ratios,
            knees,
            makeups,
            peakRmsBlends,
            bypasses,
            solos,
            scHPFs,
            scLPFs,
            scListens,
            autoMakeupEnabled,
            grMeters
        );
    }

    // M/S Decoding (if enabled)
    if (msMode > 0 && numChannels == 2)
    {
        // Decode M/S back to L/R (power-preserving)
        const float sqrtHalf = 0.70710678f; // 1/sqrt(2)

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mid = buffer.getSample(0, sample);
            float side = buffer.getSample(1, sample);

            float left = (mid + side) * sqrtHalf;
            float right = (mid - side) * sqrtHalf;

            buffer.setSample(0, sample, left);
            buffer.setSample(1, sample, right);
        }
    }

    // Mix wet (processed) with dry signal
    dryWetMixer.mixWetSamples(dryWetBlock);

    // Apply output gain
    outputGain.setGainDecibels(outputGainDB);
    juce::dsp::AudioBlock<float> outputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> outputContext(outputBlock);
    outputGain.process(outputContext);

    // ===== Phase 5.3: Measure Output Levels =====
    if (numChannels >= 1)
    {
        float outputMaxL = buffer.getMagnitude(0, 0, numSamples);
        outputLevelL.store(outputMaxL, std::memory_order_relaxed);
    }

    if (numChannels >= 2)
    {
        float outputMaxR = buffer.getMagnitude(1, 0, numSamples);
        outputLevelR.store(outputMaxR, std::memory_order_relaxed);
    }

    // ===== v1.2.0: FFT Spectrum Analysis =====
    // Process output signal through FFT for spectrum visualization
    // Uses mono sum of stereo signal for display

    const float* leftChannel = buffer.getReadPointer(0);
    const float* rightChannel = numChannels >= 2 ? buffer.getReadPointer(1) : leftChannel;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Mono sum (average L+R)
        float monoSample = (leftChannel[sample] + rightChannel[sample]) * 0.5f;

        // Push sample into FIFO
        fftInputFifo[static_cast<size_t>(fftFifoIndex)] = monoSample;
        ++fftFifoIndex;

        // When FIFO is full, perform FFT
        if (fftFifoIndex == FFT_SIZE)
        {
            fftFifoIndex = 0;

            // Copy to FFT buffer and apply window
            std::copy(fftInputFifo.begin(), fftInputFifo.end(), fftData.begin());
            fftWindow.multiplyWithWindowingTable(fftData.data(), FFT_SIZE);

            // Perform FFT (in-place, complex output)
            fft.performFrequencyOnlyForwardTransform(fftData.data());

            // Convert to magnitude (dB) and store in atomic array
            // fftData now contains FFT_SIZE/2 + 1 magnitude values
            const float minDb = -100.0f;
            const float maxDb = 0.0f;

            for (int bin = 0; bin < FFT_NUM_BINS; ++bin)
            {
                float magnitude = fftData[static_cast<size_t>(bin)];

                // Convert to dB with floor
                float db = magnitude > 0.0f
                    ? juce::jlimit(minDb, maxDb, juce::Decibels::gainToDecibels(magnitude))
                    : minDb;

                // Normalize to 0-1 range for UI (0 = -100dB, 1 = 0dB)
                float normalized = (db - minDb) / (maxDb - minDb);

                spectrumMagnitudes[static_cast<size_t>(bin)].store(normalized, std::memory_order_relaxed);
            }

            // Signal that new data is available
            spectrumDataReady.store(true, std::memory_order_relaxed);
        }
    }
}

juce::AudioProcessorEditor* OMultiBandCompressorAudioProcessor::createEditor()
{
    return new OMultiBandCompressorAudioProcessorEditor(*this);
}

void OMultiBandCompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OMultiBandCompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OMultiBandCompressorAudioProcessor();
}
