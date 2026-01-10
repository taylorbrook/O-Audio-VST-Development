/*
  ==============================================================================

    OuariconSaturationModeling - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconSaturationModelingAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // INTENSITY - Saturation intensity/amount (0-100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "INTENSITY", 1 },
        "Intensity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        50.0f,
        "%"
    ));

    // MODEL - Saturation algorithm selection (0=MAGNETIC, 1=TUBE, 2=TRANSFORMER, 3=DIODE)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MODEL", 1 },
        "Model",
        juce::StringArray { "MAGNETIC", "TUBE", "TRANSFORMER", "DIODE" },
        0  // Default: MAGNETIC
    ));

    // QUALITY - Oversampling rate (0=LOW, 1=MID, 2=HIGH)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "QUALITY", 1 },
        "Quality",
        juce::StringArray { "LOW", "MID", "HIGH" },
        1  // Default: MID
    ));

    // AUTOGAIN - Auto output gain compensation
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "AUTOGAIN", 1 },
        "Auto Gain",
        false  // Default: Off
    ));

    return layout;
}

OuariconSaturationModelingAudioProcessor::OuariconSaturationModelingAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OuariconSaturationModelingAudioProcessor::~OuariconSaturationModelingAudioProcessor()
{
}

void OuariconSaturationModelingAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Initialize oversampling systems (Phase 2.1)
    // LOW quality: No oversampling (factor=1, no actual oversampling object needed)
    oversamplingLow = nullptr;

    // MID quality: 2x oversampling with FIR equiripple filter
    oversamplingMid = std::make_unique<juce::dsp::Oversampling<float>>(
        getTotalNumOutputChannels(),
        1,  // 2^1 = 2x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true,  // Use steep filter
        false  // Don't normalize gain
    );
    oversamplingMid->initProcessing(static_cast<size_t>(samplesPerBlock));

    // HIGH quality: 4x oversampling with FIR equiripple filter
    oversamplingHigh = std::make_unique<juce::dsp::Oversampling<float>>(
        getTotalNumOutputChannels(),
        2,  // 2^2 = 4x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true,  // Use steep filter
        false  // Don't normalize gain
    );
    oversamplingHigh->initProcessing(static_cast<size_t>(samplesPerBlock));

    // Initialize DIODE model state (per-channel previous voltage)
    diodePrevVoltage.resize(getTotalNumOutputChannels(), 0.0f);

    // Initialize TRANSFORMER model filters (Phase 2.2)
    const int numChannels = getTotalNumOutputChannels();
    transformerLFBumpFilters.resize(numChannels);
    transformerHFSheenFilters.resize(numChannels);

    // Configure TRANSFORMER frequency response filters
    // LF bump: Peak filter at 60Hz, Q=0.7, +2.0dB
    auto lfBumpCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, 60.0f, 0.7f, juce::Decibels::decibelsToGain(2.0f));

    // HF sheen: High shelf at 8000Hz, Q=0.7, +1.0dB
    auto hfSheenCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 8000.0f, 0.7f, juce::Decibels::decibelsToGain(1.0f));

    // Apply coefficients to all channels
    for (int ch = 0; ch < numChannels; ++ch)
    {
        *transformerLFBumpFilters[ch].coefficients = *lfBumpCoeffs;
        *transformerHFSheenFilters[ch].coefficients = *hfSheenCoeffs;

        transformerLFBumpFilters[ch].reset();
        transformerHFSheenFilters[ch].reset();
    }

    // Initialize TUBE model filters (Phase 2.3)
    tubePresenceFilters.resize(numChannels);
    tubePrevPlateVoltage.resize(numChannels, TUBE_VSUPPLY * 0.5f);  // Initial guess: Vsupply/2

    // Configure TUBE frequency response filter
    // Presence boost: Peak filter at 3000Hz, Q=0.7, +1.5dB
    auto presenceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, 3000.0f, 0.7f, juce::Decibels::decibelsToGain(1.5f));

    // Apply coefficients to all channels
    for (int ch = 0; ch < numChannels; ++ch)
    {
        *tubePresenceFilters[ch].coefficients = *presenceCoeffs;
        tubePresenceFilters[ch].reset();
    }

    // Initialize MAGNETIC model filters (Phase 2.4)
    magneticHeadBumpFilters.resize(numChannels);
    magneticHFRolloffFilters.resize(numChannels);
    magneticM.resize(numChannels, 0.0f);      // Initialize magnetization to zero
    magneticHPrev.resize(numChannels, 0.0f);  // Initialize previous field to zero

    // Configure MAGNETIC frequency response filters
    // Head bump: Peak filter at 80Hz, Q=0.7, +2.5dB
    auto headBumpCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, 80.0f, 0.7f, juce::Decibels::decibelsToGain(2.5f));

    // HF rolloff: Lowpass filter at 12000Hz, Q=0.707
    auto hfRolloffCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sampleRate, 12000.0f, 0.707f);

    // Apply coefficients to all channels
    for (int ch = 0; ch < numChannels; ++ch)
    {
        *magneticHeadBumpFilters[ch].coefficients = *headBumpCoeffs;
        *magneticHFRolloffFilters[ch].coefficients = *hfRolloffCoeffs;

        magneticHeadBumpFilters[ch].reset();
        magneticHFRolloffFilters[ch].reset();
    }

    // Initialize Auto-Gain system (Phase 2.4)
    inputRMSEnvelope.resize(numChannels, 0.0f);
    outputRMSEnvelope.resize(numChannels, 0.0f);

    // Calculate auto-gain time constant coefficient (100ms attack/release)
    // Formula: coeff = exp(-1.0 / (timeConstant * sampleRate))
    const float timeConstantSeconds = 0.1f;  // 100ms
    autoGainCoeff = std::exp(-1.0f / (timeConstantSeconds * static_cast<float>(sampleRate)));

    // Get initial quality setting and report latency
    auto* qualityParam = parameters.getRawParameterValue("QUALITY");
    currentQuality = static_cast<int>(qualityParam->load());

    // Report latency to host
    int latency = 0;
    if (currentQuality == 1 && oversamplingMid)
        latency = static_cast<int>(oversamplingMid->getLatencyInSamples());
    else if (currentQuality == 2 && oversamplingHigh)
        latency = static_cast<int>(oversamplingHigh->getLatencyInSamples());

    setLatencySamples(latency);
}

void OuariconSaturationModelingAudioProcessor::releaseResources()
{
    // Reset oversampling systems
    if (oversamplingMid)
        oversamplingMid->reset();
    if (oversamplingHigh)
        oversamplingHigh->reset();

    // Clear DIODE state
    std::fill(diodePrevVoltage.begin(), diodePrevVoltage.end(), 0.0f);

    // Reset TRANSFORMER filters (Phase 2.2)
    for (auto& filter : transformerLFBumpFilters)
        filter.reset();
    for (auto& filter : transformerHFSheenFilters)
        filter.reset();

    // Reset TUBE model state (Phase 2.3)
    std::fill(tubePrevPlateVoltage.begin(), tubePrevPlateVoltage.end(), TUBE_VSUPPLY * 0.5f);
    for (auto& filter : tubePresenceFilters)
        filter.reset();

    // Reset MAGNETIC model state (Phase 2.4)
    std::fill(magneticM.begin(), magneticM.end(), 0.0f);
    std::fill(magneticHPrev.begin(), magneticHPrev.end(), 0.0f);
    for (auto& filter : magneticHeadBumpFilters)
        filter.reset();
    for (auto& filter : magneticHFRolloffFilters)
        filter.reset();

    // Reset Auto-Gain envelopes (Phase 2.4)
    std::fill(inputRMSEnvelope.begin(), inputRMSEnvelope.end(), 0.0f);
    std::fill(outputRMSEnvelope.begin(), outputRMSEnvelope.end(), 0.0f);
}

void OuariconSaturationModelingAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic, real-time safe)
    auto* intensityParam = parameters.getRawParameterValue("INTENSITY");
    float intensity = intensityParam->load();  // 0.0-100.0

    auto* modelParam = parameters.getRawParameterValue("MODEL");
    int model = static_cast<int>(modelParam->load());  // 0=MAGNETIC, 1=TUBE, 2=TRANSFORMER, 3=DIODE

    auto* qualityParam = parameters.getRawParameterValue("QUALITY");
    int quality = static_cast<int>(qualityParam->load());  // 0=LOW, 1=MID, 2=HIGH

    auto* autoGainParam = parameters.getRawParameterValue("AUTOGAIN");
    bool autoGainEnabled = autoGainParam->load() > 0.5f;

    // Update latency if quality changed
    if (quality != currentQuality)
    {
        currentQuality = quality;

        // Report new latency
        int latency = 0;
        if (currentQuality == 1 && oversamplingMid)
            latency = static_cast<int>(oversamplingMid->getLatencyInSamples());
        else if (currentQuality == 2 && oversamplingHigh)
            latency = static_cast<int>(oversamplingHigh->getLatencyInSamples());

        setLatencySamples(latency);

        // Reset oversampling state to prevent artifacts
        if (oversamplingMid)
            oversamplingMid->reset();
        if (oversamplingHigh)
            oversamplingHigh->reset();
    }

    // Phase 2.2: MODEL parameter routing implemented (DIODE=3, TRANSFORMER=2)
    // Phase 2.1: DIODE model (model=3)
    // Phase 2.2: TRANSFORMER model (model=2)
    // Phase 2.3: TUBE model (model=1)
    // TODO Phase 2.4: MAGNETIC model (model=0)

    // Determine iteration count based on quality (for DIODE model)
    int iterations = 4;  // LOW quality
    if (quality == 1)
        iterations = 6;  // MID quality
    else if (quality == 2)
        iterations = 8;  // HIGH quality

    // VU Meter: Capture INPUT peak level BEFORE any processing (like TapeAge)
    float inputPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float channelPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
        inputPeak = std::max(inputPeak, channelPeak);
    }
    float inDb = inputPeak > 0.00001f
        ? juce::Decibels::gainToDecibels(inputPeak)
        : -100.0f;
    inputLevelDB.store(inDb, std::memory_order_relaxed);

    // Processing chain: Input → RMS Capture (pre) → Upsample → Saturation (model-specific) → Downsample → RMS Capture (post) → Auto-Gain → Output
    if (quality == 0)
    {
        // LOW quality: No oversampling (direct processing)
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            // Capture input RMS (pre-saturation)
            float inputRMS = 0.0f;
            for (int sample = 0; sample < numSamples; ++sample)
            {
                inputRMS += channelData[sample] * channelData[sample];
            }
            inputRMS = std::sqrt(inputRMS / numSamples);

            // Update input envelope with time constant
            inputRMSEnvelope[channel] = autoGainCoeff * inputRMSEnvelope[channel] + (1.0f - autoGainCoeff) * inputRMS;

            // Denormal protection
            if (inputRMSEnvelope[channel] < 1e-8f)
                inputRMSEnvelope[channel] = 0.0f;

            // Process saturation
            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Model routing switch
                if (model == 0)  // MAGNETIC
                {
                    channelData[sample] = processMagneticSample(channelData[sample], intensity, channel);
                }
                else if (model == 1)  // TUBE
                {
                    float& prevPlateVoltage = tubePrevPlateVoltage[channel];
                    channelData[sample] = processTubeSample(channelData[sample], intensity, iterations, channel, prevPlateVoltage);
                }
                else if (model == 2)  // TRANSFORMER
                {
                    channelData[sample] = processTransformerSample(channelData[sample], intensity, channel);
                }
                else if (model == 3)  // DIODE
                {
                    float& prevVoltage = diodePrevVoltage[channel];
                    channelData[sample] = processDiodeSample(channelData[sample], intensity, iterations, prevVoltage);
                }
            }

            // Capture output RMS (post-saturation)
            float outputRMS = 0.0f;
            for (int sample = 0; sample < numSamples; ++sample)
            {
                outputRMS += channelData[sample] * channelData[sample];
            }
            outputRMS = std::sqrt(outputRMS / numSamples);

            // Update output envelope with time constant
            outputRMSEnvelope[channel] = autoGainCoeff * outputRMSEnvelope[channel] + (1.0f - autoGainCoeff) * outputRMS;

            // Denormal protection
            if (outputRMSEnvelope[channel] < 1e-8f)
                outputRMSEnvelope[channel] = 0.0f;

            // Apply auto-gain if enabled
            if (autoGainEnabled)
            {
                // Calculate compensation gain: inputRMS / outputRMS
                float compensationGain = 1.0f;
                if (outputRMSEnvelope[channel] > 1e-6f)
                {
                    compensationGain = inputRMSEnvelope[channel] / outputRMSEnvelope[channel];
                    // Clamp to 0.1-10.0 range to prevent extreme swings
                    compensationGain = juce::jlimit(0.1f, 10.0f, compensationGain);
                }

                // Apply compensation gain to output
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    channelData[sample] *= compensationGain;
                }
            }
        }
    }
    else if (quality == 1 && oversamplingMid)
    {
        // MID quality: 2x oversampling
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Capture input RMS (pre-saturation, at base rate)
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getReadPointer(channel);

            float inputRMS = 0.0f;
            for (int sample = 0; sample < numSamples; ++sample)
            {
                inputRMS += channelData[sample] * channelData[sample];
            }
            inputRMS = std::sqrt(inputRMS / numSamples);

            // Update input envelope with time constant
            inputRMSEnvelope[channel] = autoGainCoeff * inputRMSEnvelope[channel] + (1.0f - autoGainCoeff) * inputRMS;

            // Denormal protection
            if (inputRMSEnvelope[channel] < 1e-8f)
                inputRMSEnvelope[channel] = 0.0f;
        }

        // Upsample
        auto oversampledBlock = oversamplingMid->processSamplesUp(buffer);

        const int oversampledChannels = static_cast<int>(oversampledBlock.getNumChannels());
        const int oversampledSamples = static_cast<int>(oversampledBlock.getNumSamples());

        // Process saturation at oversampled rate
        for (int channel = 0; channel < oversampledChannels; ++channel)
        {
            auto* channelData = oversampledBlock.getChannelPointer(channel);

            for (int sample = 0; sample < oversampledSamples; ++sample)
            {
                // Model routing switch
                if (model == 0)  // MAGNETIC
                {
                    channelData[sample] = processMagneticSample(channelData[sample], intensity, channel);
                }
                else if (model == 1)  // TUBE
                {
                    float& prevPlateVoltage = tubePrevPlateVoltage[channel];
                    channelData[sample] = processTubeSample(channelData[sample], intensity, iterations, channel, prevPlateVoltage);
                }
                else if (model == 2)  // TRANSFORMER
                {
                    channelData[sample] = processTransformerSample(channelData[sample], intensity, channel);
                }
                else if (model == 3)  // DIODE
                {
                    float& prevVoltage = diodePrevVoltage[channel];
                    channelData[sample] = processDiodeSample(channelData[sample], intensity, iterations, prevVoltage);
                }
            }
        }

        // Downsample
        juce::dsp::AudioBlock<float> outputBlock(buffer);
        oversamplingMid->processSamplesDown(outputBlock);

        // Capture output RMS (post-saturation, at base rate)
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            float outputRMS = 0.0f;
            for (int sample = 0; sample < numSamples; ++sample)
            {
                outputRMS += channelData[sample] * channelData[sample];
            }
            outputRMS = std::sqrt(outputRMS / numSamples);

            // Update output envelope with time constant
            outputRMSEnvelope[channel] = autoGainCoeff * outputRMSEnvelope[channel] + (1.0f - autoGainCoeff) * outputRMS;

            // Denormal protection
            if (outputRMSEnvelope[channel] < 1e-8f)
                outputRMSEnvelope[channel] = 0.0f;

            // Apply auto-gain if enabled
            if (autoGainEnabled)
            {
                // Calculate compensation gain: inputRMS / outputRMS
                float compensationGain = 1.0f;
                if (outputRMSEnvelope[channel] > 1e-6f)
                {
                    compensationGain = inputRMSEnvelope[channel] / outputRMSEnvelope[channel];
                    // Clamp to 0.1-10.0 range to prevent extreme swings
                    compensationGain = juce::jlimit(0.1f, 10.0f, compensationGain);
                }

                // Apply compensation gain to output
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    channelData[sample] *= compensationGain;
                }
            }
        }
    }
    else if (quality == 2 && oversamplingHigh)
    {
        // HIGH quality: 4x oversampling
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Capture input RMS (pre-saturation, at base rate)
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getReadPointer(channel);

            float inputRMS = 0.0f;
            for (int sample = 0; sample < numSamples; ++sample)
            {
                inputRMS += channelData[sample] * channelData[sample];
            }
            inputRMS = std::sqrt(inputRMS / numSamples);

            // Update input envelope with time constant
            inputRMSEnvelope[channel] = autoGainCoeff * inputRMSEnvelope[channel] + (1.0f - autoGainCoeff) * inputRMS;

            // Denormal protection
            if (inputRMSEnvelope[channel] < 1e-8f)
                inputRMSEnvelope[channel] = 0.0f;
        }

        // Upsample
        auto oversampledBlock = oversamplingHigh->processSamplesUp(buffer);

        const int oversampledChannels = static_cast<int>(oversampledBlock.getNumChannels());
        const int oversampledSamples = static_cast<int>(oversampledBlock.getNumSamples());

        // Process saturation at oversampled rate
        for (int channel = 0; channel < oversampledChannels; ++channel)
        {
            auto* channelData = oversampledBlock.getChannelPointer(channel);

            for (int sample = 0; sample < oversampledSamples; ++sample)
            {
                // Model routing switch
                if (model == 0)  // MAGNETIC
                {
                    channelData[sample] = processMagneticSample(channelData[sample], intensity, channel);
                }
                else if (model == 1)  // TUBE
                {
                    float& prevPlateVoltage = tubePrevPlateVoltage[channel];
                    channelData[sample] = processTubeSample(channelData[sample], intensity, iterations, channel, prevPlateVoltage);
                }
                else if (model == 2)  // TRANSFORMER
                {
                    channelData[sample] = processTransformerSample(channelData[sample], intensity, channel);
                }
                else if (model == 3)  // DIODE
                {
                    float& prevVoltage = diodePrevVoltage[channel];
                    channelData[sample] = processDiodeSample(channelData[sample], intensity, iterations, prevVoltage);
                }
            }
        }

        // Downsample
        juce::dsp::AudioBlock<float> outputBlock(buffer);
        oversamplingHigh->processSamplesDown(outputBlock);

        // Capture output RMS (post-saturation, at base rate)
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            float outputRMS = 0.0f;
            for (int sample = 0; sample < numSamples; ++sample)
            {
                outputRMS += channelData[sample] * channelData[sample];
            }
            outputRMS = std::sqrt(outputRMS / numSamples);

            // Update output envelope with time constant
            outputRMSEnvelope[channel] = autoGainCoeff * outputRMSEnvelope[channel] + (1.0f - autoGainCoeff) * outputRMS;

            // Denormal protection
            if (outputRMSEnvelope[channel] < 1e-8f)
                outputRMSEnvelope[channel] = 0.0f;

            // Apply auto-gain if enabled
            if (autoGainEnabled)
            {
                // Calculate compensation gain: inputRMS / outputRMS
                float compensationGain = 1.0f;
                if (outputRMSEnvelope[channel] > 1e-6f)
                {
                    compensationGain = inputRMSEnvelope[channel] / outputRMSEnvelope[channel];
                    // Clamp to 0.1-10.0 range to prevent extreme swings
                    compensationGain = juce::jlimit(0.1f, 10.0f, compensationGain);
                }

                // Apply compensation gain to output
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    channelData[sample] *= compensationGain;
                }
            }
        }
    }

    // VU Meter: Calculate OUTPUT peak level AFTER all processing (like TapeAge)
    float outputPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float channelPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
        outputPeak = std::max(outputPeak, channelPeak);
    }
    float outDb = outputPeak > 0.00001f
        ? juce::Decibels::gainToDecibels(outputPeak)
        : -100.0f;
    outputLevelDB.store(outDb, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* OuariconSaturationModelingAudioProcessor::createEditor()
{
    return new OuariconSaturationModelingAudioProcessorEditor(*this);
}

void OuariconSaturationModelingAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconSaturationModelingAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// ============================================================================
// DIODE Model Implementation (Symmetric Soft Clipping)
// ============================================================================

float OuariconSaturationModelingAudioProcessor::processDiodeSample(float input, float intensity, int iterations, float& prevVoltage)
{
    juce::ignoreUnused(iterations, prevVoltage);  // Not needed for simplified model

    // At 0% intensity, return dry signal (no processing)
    if (intensity < 0.1f)
        return input;

    // Dry/wet mix: 0% = dry, 100% = full saturation
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float dryMix = 1.0f - wetMix;

    // Drive increases with intensity: 1.0 to 5.0
    const float drive = 1.0f + wetMix * 4.0f;

    // Apply input drive
    float x = input * drive;

    // Anti-parallel diode waveshaper (symmetric soft clipping)
    // This models the classic TS-style diode clipper sound
    // Formula: x / (1 + |x|)^n where n controls hardness
    // n=1.0 is very soft, n=0.5 is harder (more like real diodes)
    const float hardness = 0.7f;  // Diode-like response
    float wetSignal = x / std::pow(1.0f + std::abs(x), hardness);

    // Add subtle odd harmonics (characteristic of symmetric clipping)
    // Diodes produce primarily odd harmonics due to symmetric clipping
    const float x3 = wetSignal * wetSignal * wetSignal;
    wetSignal = wetSignal * 0.9f + x3 * 0.1f;  // Subtle 3rd harmonic

    // Mix dry and wet signals based on intensity
    return (dryMix * input) + (wetMix * wetSignal);
}

// ============================================================================
// TRANSFORMER Model Implementation (Phase 2.2)
// ============================================================================

float OuariconSaturationModelingAudioProcessor::processTransformerSample(float input, float intensity, int channel)
{
    // At 0% intensity, return dry signal (no processing)
    if (intensity < 0.1f)
        return input;

    // Dry/wet mix: 0% = dry, 100% = full saturation
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float dryMix = 1.0f - wetMix;

    // INTENSITY parameter mapping: Input gain = 1.0 + wetMix * 5.0
    // At 0%: gain = 1.0 (unity gain, minimal saturation)
    // At 100%: gain = 6.0 (maximum drive into saturation)
    const float intensityGain = 1.0f + wetMix * 5.0f;

    // Apply input gain
    float driven = input * intensityGain;

    // Core saturation: Soft tanh-based saturation
    // Formula: output = threshold * tanh(input / threshold)
    // Threshold = 0.8 (from architecture.md)
    float saturated = TRANSFORMER_CORE_SATURATION * std::tanh(driven / TRANSFORMER_CORE_SATURATION);

    // Apply frequency response filters
    // Processing order: Saturation → LF bump → HF sheen

    // LF bump filter (60Hz peak, Q=0.7, +2.0dB)
    float lfProcessed = transformerLFBumpFilters[channel].processSample(saturated);

    // HF sheen filter (8kHz high shelf, +1.0dB)
    float wetSignal = transformerHFSheenFilters[channel].processSample(lfProcessed);

    // Mix dry and wet signals based on intensity
    return (dryMix * input) + (wetMix * wetSignal);
}

// ============================================================================
// TUBE Model Implementation (Asymmetric Soft Saturation) - Phase 2.3
// ============================================================================

float OuariconSaturationModelingAudioProcessor::processTubeSample(float input, float intensity, int iterations, int channel, float& prevPlateVoltage)
{
    juce::ignoreUnused(iterations, prevPlateVoltage);  // Not needed for simplified model

    // At 0% intensity, return dry signal (no processing)
    if (intensity < 0.1f)
        return input;

    // Dry/wet mix: 0% = dry, 100% = full saturation
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float dryMix = 1.0f - wetMix;

    // Drive increases with intensity: 1.0 to 4.0
    const float drive = 1.0f + wetMix * 3.0f;

    // Apply input drive
    float x = input * drive;

    // Tube-style asymmetric soft clipping
    // Tubes clip harder on negative swings (grid cutoff) than positive (saturation)
    // This creates even harmonics characteristic of tube sound
    float wetSignal;

    if (x >= 0.0f)
    {
        // Positive half: Soft saturation (tanh-like but gentler)
        // Formula: x / (1 + |x|) - softer knee than tanh
        wetSignal = x / (1.0f + std::abs(x));
    }
    else
    {
        // Negative half: Harder clipping (grid cutoff behavior)
        // Formula: tanh(x * 1.5) / 1.5 - clips earlier and harder
        wetSignal = std::tanh(x * 1.5f) / 1.5f;
    }

    // Add subtle even harmonic content (tube characteristic)
    // Second harmonic from asymmetry + gentle third harmonic
    const float x2 = wetSignal * wetSignal;
    wetSignal = wetSignal + 0.1f * x2 * (wetSignal > 0 ? 1.0f : -1.0f);

    // Normalize output level (asymmetric clipping can reduce average level)
    wetSignal *= 1.2f;

    // Apply presence filter (3kHz peak, Q=0.7, +1.5dB)
    wetSignal = tubePresenceFilters[static_cast<size_t>(channel)].processSample(wetSignal);

    // Mix dry and wet signals based on intensity
    return (dryMix * input) + (wetMix * wetSignal);
}

// ============================================================================
// MAGNETIC Model Implementation (Jiles-Atherton Hysteresis) - Phase 2.4
// ============================================================================

float OuariconSaturationModelingAudioProcessor::langevinFunction(float x)
{
    // Langevin function: L(x) = coth(x) - 1/x
    // For |x| < 1e-6, use Taylor series to avoid singularity: L(x) ≈ x/3 - x³/45

    if (std::abs(x) < 1e-6f)
    {
        // Taylor series approximation for small x
        const float x2 = x * x;
        return (x / 3.0f) - (x2 * x / 45.0f);
    }
    else
    {
        // Standard Langevin formula: coth(x) - 1/x
        // coth(x) = (exp(2x) + 1) / (exp(2x) - 1)
        const float exp2x = std::exp(juce::jlimit(-30.0f, 30.0f, 2.0f * x));
        const float coth = (exp2x + 1.0f) / (exp2x - 1.0f);
        return coth - (1.0f / x);
    }
}

float OuariconSaturationModelingAudioProcessor::processMagneticSample(float input, float intensity, int channel)
{
    // At 0% intensity, return dry signal (no processing)
    if (intensity < 0.1f)
        return input;

    // Dry/wet mix: 0% = dry, 100% = full saturation
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float dryMix = 1.0f - wetMix;

    // Drive scales with intensity: subtle at low values, heavy at high
    const float drive = 1.0f + wetMix * 2.0f;  // 1.0 to 3.0 range

    // Apply input drive
    float H = input * drive;  // Magnetic field (input signal)

    // Read state variables
    float& M = magneticM[static_cast<size_t>(channel)];
    float& H_prev = magneticHPrev[static_cast<size_t>(channel)];

    // Limit deltaH to prevent initialization transient and reduce noise
    float deltaH = H - H_prev;
    deltaH = juce::jlimit(-0.3f, 0.3f, deltaH);  // Tighter limit = smoother

    // Calculate direction (sign of field change)
    float delta = (deltaH >= 0.0f) ? 1.0f : -1.0f;

    // Calculate effective field
    const float He = H + MAGNETIC_ALPHA * M;

    // Calculate anhysteretic magnetization using Langevin function
    const float arg = He / MAGNETIC_A;
    const float Man = MAGNETIC_MS * langevinFunction(arg);

    // Calculate derivative of Langevin function
    float dLdx = 0.0f;
    if (std::abs(arg) < 1e-4f)
    {
        dLdx = 1.0f / 3.0f;
    }
    else
    {
        const float clampedArg = juce::jlimit(-15.0f, 15.0f, arg);
        const float exp2x = std::exp(2.0f * clampedArg);
        const float coth = (exp2x + 1.0f) / (exp2x - 1.0f);
        dLdx = 1.0f - (coth * coth) + (1.0f / (clampedArg * clampedArg));
    }
    const float dMan_dH = (MAGNETIC_MS / MAGNETIC_A) * dLdx;

    // Differential hysteresis equation (Jiles-Atherton)
    const float denominator = MAGNETIC_K * delta - MAGNETIC_ALPHA * (Man - M);

    // Calculate dM/dH with robust handling
    float dM_dH = 0.0f;
    if (std::abs(denominator) > 0.01f)
    {
        dM_dH = (Man - M) / denominator + MAGNETIC_C * dMan_dH;
    }
    else
    {
        // Near singularity: use reversible component only (smoother)
        dM_dH = MAGNETIC_C * dMan_dH;
    }

    // Limit dM_dH to prevent runaway and reduce noise
    dM_dH = juce::jlimit(-5.0f, 5.0f, dM_dH);

    // Update magnetization
    M += dM_dH * deltaH;

    // Clamp magnetization to normalized limits
    M = juce::jlimit(-MAGNETIC_MS, MAGNETIC_MS, M);

    // NaN/Inf protection
    if (!std::isfinite(M))
        M = 0.0f;

    // Denormal protection
    if (std::abs(M) < 1e-8f)
        M = 0.0f;

    // Store previous field for next sample
    H_prev = H;

    // Wet signal is magnetization
    float wetSignal = M;

    // Apply frequency response filters to wet signal only
    wetSignal = magneticHeadBumpFilters[static_cast<size_t>(channel)].processSample(wetSignal);
    wetSignal = magneticHFRolloffFilters[static_cast<size_t>(channel)].processSample(wetSignal);

    // Mix dry and wet signals based on intensity
    return (dryMix * input) + (wetMix * wetSignal);
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconSaturationModelingAudioProcessor();
}
