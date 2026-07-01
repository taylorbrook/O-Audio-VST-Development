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

    // IN-03: ~20 ms ramp so Input/Output Gain automation doesn't zipper.
    inputGain.setRampDurationSeconds(0.02);
    outputGain.setRampDurationSeconds(0.02);

    // Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::linear);

    // CR-03: preallocate the mono M/S scratch buffer to the maximum block size so Mid/Side
    // modes never allocate on the audio thread.
    msScratchBuffer.setSize(1, samplesPerBlock, false, false, false);
    msScratchBuffer.clear();

    // CR-02: resolve all parameter pointers once (no per-block string building / map lookups).
    cacheParameterPointers();

    // Reset components to initial state
    multibandProcessor.reset();
    inputGain.reset();
    outputGain.reset();
    dryWetMixer.reset();
}

void OMultiBandCompressorAudioProcessor::cacheParameterPointers()
{
    static const char* const prefixes[4] = { "LOW", "LOMID", "HIMID", "HIGH" };
    static const char* const suffixes[numBandParams] = {
        "_THRESHOLD", "_RATIO", "_ATTACK", "_RELEASE", "_KNEE", "_MAKEUP",
        "_PEAK_RMS", "_BYPASS", "_SOLO", "_SC_HPF", "_SC_LPF", "_SC_LISTEN"
    };

    for (int band = 0; band < 4; ++band)
        for (int kind = 0; kind < numBandParams; ++kind)
            bandParamPtrs[band][kind] =
                parameters.getRawParameterValue(juce::String(prefixes[band]) + suffixes[kind]);

    pInputGain  = parameters.getRawParameterValue("INPUT_GAIN");
    pOutputGain = parameters.getRawParameterValue("OUTPUT_GAIN");
    pMix        = parameters.getRawParameterValue("MIX");
    pAutoMakeup = parameters.getRawParameterValue("AUTO_MAKEUP");
    pMsMode     = parameters.getRawParameterValue("MS_MODE");
    pXover1     = parameters.getRawParameterValue("XOVER1");
    pXover2     = parameters.getRawParameterValue("XOVER2");
    pXover3     = parameters.getRawParameterValue("XOVER3");
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
    // CR-02: all pointers were resolved once in prepareToPlay — no string building or map
    // lookups here.

    // Global parameters
    float inputGainDB = pInputGain->load();
    float outputGainDB = pOutputGain->load();
    float mixPercent = pMix->load();
    bool autoMakeupEnabled = pAutoMakeup->load() > 0.5f;
    int msMode = static_cast<int>(pMsMode->load());

    // Crossover frequencies
    float xover1 = pXover1->load();
    float xover2 = pXover2->load();
    float xover3 = pXover3->load();

    // Per-band parameters (arrays indexed by band: 0=LOW, 1=LOMID, 2=HIMID, 3=HIGH)
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
        thresholds[band]    = bandParamPtrs[band][bpThreshold]->load();
        ratios[band]        = bandParamPtrs[band][bpRatio]->load();
        attacks[band]       = bandParamPtrs[band][bpAttack]->load();
        releases[band]      = bandParamPtrs[band][bpRelease]->load();
        knees[band]         = bandParamPtrs[band][bpKnee]->load();
        makeups[band]       = bandParamPtrs[band][bpMakeup]->load();
        peakRmsBlends[band] = bandParamPtrs[band][bpPeakRms]->load();
        bypasses[band]      = bandParamPtrs[band][bpBypass]->load() > 0.5f;
        solos[band]         = bandParamPtrs[band][bpSolo]->load() > 0.5f;
        scHPFs[band]        = bandParamPtrs[band][bpScHPF]->load();
        scLPFs[band]        = bandParamPtrs[band][bpScLPF]->load();
        scListens[band]     = bandParamPtrs[band][bpScListen]->load() > 0.5f;
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
        // Mode 1: Mid - Compress only mid (channel 0), side passes through.
        // CR-03: reuse the preallocated scratch buffer (avoidReallocating — no RT alloc).
        msScratchBuffer.setSize(1, numSamples, false, false, true);
        msScratchBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);

        multibandProcessor.processMultiband(
            msScratchBuffer,
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

        buffer.copyFrom(0, 0, msScratchBuffer, 0, 0, numSamples);
    }
    else if (msMode == 2)
    {
        // Mode 2: Side - Compress only side (channel 1), mid passes through.
        // CR-03: reuse the preallocated scratch buffer (avoidReallocating — no RT alloc).
        msScratchBuffer.setSize(1, numSamples, false, false, true);
        msScratchBuffer.copyFrom(0, 0, buffer, 1, 0, numSamples);

        multibandProcessor.processMultiband(
            msScratchBuffer,
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

        buffer.copyFrom(1, 0, msScratchBuffer, 0, 0, numSamples);
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
    // Accumulate samples for FFT (mono sum of output)
    const float* leftOut = buffer.getReadPointer(0);
    const float* rightOut = numChannels >= 2 ? buffer.getReadPointer(1) : leftOut;

    for (int i = 0; i < numSamples; ++i)
    {
        // Mono sum
        float sample = (leftOut[i] + rightOut[i]) * 0.5f;
        fftInputFifo[static_cast<size_t>(fftFifoWriteIndex)] = sample;
        ++fftFifoWriteIndex;

        // When FIFO is full, compute FFT
        if (fftFifoWriteIndex >= FFT_SIZE)
        {
            fftFifoWriteIndex = 0;

            // Copy to work buffer and apply window
            std::copy(fftInputFifo.begin(), fftInputFifo.end(), fftWorkBuffer.begin());
            fftWindow.multiplyWithWindowingTable(fftWorkBuffer.data(), FFT_SIZE);

            // Perform FFT
            fft.performFrequencyOnlyForwardTransform(fftWorkBuffer.data());

            // Downsample to SPECTRUM_BINS and convert to normalized dB
            constexpr int binsPerOutput = (FFT_SIZE / 2) / SPECTRUM_BINS;
            constexpr float minDb = -80.0f;
            constexpr float maxDb = 0.0f;

            std::array<float, SPECTRUM_BINS> tempSpectrum {};

            for (int bin = 0; bin < SPECTRUM_BINS; ++bin)
            {
                float sum = 0.0f;
                int startIdx = bin * binsPerOutput;
                int endIdx = startIdx + binsPerOutput;

                for (int j = startIdx; j < endIdx; ++j)
                    sum += fftWorkBuffer[static_cast<size_t>(j)];

                float avgMag = sum / static_cast<float>(binsPerOutput);
                float db = avgMag > 0.0f
                    ? juce::jlimit(minDb, maxDb, juce::Decibels::gainToDecibels(avgMag))
                    : minDb;

                // Normalize to 0-1
                tempSpectrum[static_cast<size_t>(bin)] = (db - minDb) / (maxDb - minDb);
            }

            // WR-01: publish lock-free. Fill the writer-private slot, then atomically hand it
            // off to the "ready" slot (receiving a free slot to write next time). No lock, no
            // wait — the audio thread cannot be blocked by the UI thread.
            spectrumBuffers[static_cast<size_t>(spectrumWriteSlot)] = tempSpectrum;
            spectrumWriteSlot = spectrumReadySlot.exchange(spectrumWriteSlot, std::memory_order_acq_rel);
            spectrumDataReady.store(true, std::memory_order_release);
        }
    }
}

void OMultiBandCompressorAudioProcessor::getSpectrumData(std::array<float, SPECTRUM_BINS>& dest) const
{
    // WR-01: atomically claim the most-recently published slot (handing back the slot we
    // previously held). Gated by hasNewSpectrumData()/clearSpectrumDataFlag() in the editor,
    // so it reads at most once per published frame.
    spectrumReadSlot = spectrumReadySlot.exchange(spectrumReadSlot, std::memory_order_acq_rel);
    dest = spectrumBuffers[static_cast<size_t>(spectrumReadSlot)];
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
