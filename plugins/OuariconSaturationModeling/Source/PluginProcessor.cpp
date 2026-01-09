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
    // TODO Phase 2.3: TUBE model (model=1)
    // TODO Phase 2.4: MAGNETIC model (model=0)

    // Determine iteration count based on quality (for DIODE model)
    int iterations = 4;  // LOW quality
    if (quality == 1)
        iterations = 6;  // MID quality
    else if (quality == 2)
        iterations = 8;  // HIGH quality

    // Processing chain: Input → Upsample → Saturation (model-specific) → Downsample → Output
    if (quality == 0)
    {
        // LOW quality: No oversampling (direct processing)
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Model routing switch
                if (model == 3)  // DIODE
                {
                    float& prevVoltage = diodePrevVoltage[channel];
                    channelData[sample] = processDiodeSample(channelData[sample], intensity, iterations, prevVoltage);
                }
                else if (model == 2)  // TRANSFORMER
                {
                    channelData[sample] = processTransformerSample(channelData[sample], intensity, channel);
                }
                // else: Pass through (models 0, 1 not yet implemented)
            }
        }
    }
    else if (quality == 1 && oversamplingMid)
    {
        // MID quality: 2x oversampling
        auto oversampledBlock = oversamplingMid->processSamplesUp(buffer);

        const int numChannels = static_cast<int>(oversampledBlock.getNumChannels());
        const int numSamples = static_cast<int>(oversampledBlock.getNumSamples());

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = oversampledBlock.getChannelPointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Model routing switch
                if (model == 3)  // DIODE
                {
                    float& prevVoltage = diodePrevVoltage[channel];
                    channelData[sample] = processDiodeSample(channelData[sample], intensity, iterations, prevVoltage);
                }
                else if (model == 2)  // TRANSFORMER
                {
                    channelData[sample] = processTransformerSample(channelData[sample], intensity, channel);
                }
                // else: Pass through (models 0, 1 not yet implemented)
            }
        }

        juce::dsp::AudioBlock<float> outputBlock(buffer);
        oversamplingMid->processSamplesDown(outputBlock);
    }
    else if (quality == 2 && oversamplingHigh)
    {
        // HIGH quality: 4x oversampling
        auto oversampledBlock = oversamplingHigh->processSamplesUp(buffer);

        const int numChannels = static_cast<int>(oversampledBlock.getNumChannels());
        const int numSamples = static_cast<int>(oversampledBlock.getNumSamples());

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = oversampledBlock.getChannelPointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Model routing switch
                if (model == 3)  // DIODE
                {
                    float& prevVoltage = diodePrevVoltage[channel];
                    channelData[sample] = processDiodeSample(channelData[sample], intensity, iterations, prevVoltage);
                }
                else if (model == 2)  // TRANSFORMER
                {
                    channelData[sample] = processTransformerSample(channelData[sample], intensity, channel);
                }
                // else: Pass through (models 0, 1 not yet implemented)
            }
        }

        juce::dsp::AudioBlock<float> outputBlock(buffer);
        oversamplingHigh->processSamplesDown(outputBlock);
    }
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
// DIODE Model Implementation (Newton-Raphson Shockley Equation)
// ============================================================================

float OuariconSaturationModelingAudioProcessor::processDiodeSample(float input, float intensity, int iterations, float& prevVoltage)
{
    // INTENSITY parameter mapping: Series resistance R = 1kΩ / (1.0 + INTENSITY/100.0)
    // At 0%: R = 1000Ω (minimal saturation)
    // At 100%: R = 500Ω (maximum saturation)
    const float R = 1000.0f / (1.0f + intensity / 100.0f);

    // Newton-Raphson solver for anti-parallel diode pair
    // Circuit equation: v + R * i_total(v) = input
    // Where i_total = i_d1 - i_d2 (anti-parallel configuration)
    // i_d1 = Is * (exp(v/(n*Vt)) - 1)  (Shockley equation)
    // i_d2 = Is * (exp(-v/(n*Vt)) - 1)

    // Use previous sample value as initial guess (warm start)
    float v = prevVoltage;

    // Precompute constants
    const float nVt = DIODE_N * DIODE_VT;

    // Newton-Raphson iterations
    for (int iter = 0; iter < iterations; ++iter)
    {
        // Clamp exponential arguments to prevent overflow
        // exp(30) ≈ 1e13, exp(-30) ≈ 1e-13 (safe range)
        float expPos = std::exp(juce::jlimit(-30.0f, 30.0f, v / nVt));
        float expNeg = std::exp(juce::jlimit(-30.0f, 30.0f, -v / nVt));

        // Anti-parallel diode currents
        float i_d1 = DIODE_IS * (expPos - 1.0f);
        float i_d2 = DIODE_IS * (expNeg - 1.0f);
        float i_total = i_d1 - i_d2;

        // Error function: f(v) = v + R * i_total(v) - input
        float f = v + R * i_total - input;

        // Derivative: f'(v) = 1 + R * di_total/dv
        // di_total/dv = (Is/nVt) * (expPos + expNeg)
        float df = 1.0f + R * (DIODE_IS / nVt) * (expPos + expNeg);

        // Newton-Raphson update: v_new = v - f(v) / f'(v)
        v -= f / df;

        // Denormal protection
        if (std::abs(v) < 1e-8f)
            v = 0.0f;
    }

    // Store voltage for next sample (warm start)
    prevVoltage = v;

    return v;
}

// ============================================================================
// TRANSFORMER Model Implementation (Phase 2.2)
// ============================================================================

float OuariconSaturationModelingAudioProcessor::processTransformerSample(float input, float intensity, int channel)
{
    // INTENSITY parameter mapping: Input gain = 1.0 + (INTENSITY/100.0) * 5.0
    // At 0%: gain = 1.0 (unity gain, minimal saturation)
    // At 100%: gain = 6.0 (maximum drive into saturation)
    const float intensityGain = 1.0f + (intensity / 100.0f) * 5.0f;

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
    float hfProcessed = transformerHFSheenFilters[channel].processSample(lfProcessed);

    return hfProcessed;
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconSaturationModelingAudioProcessor();
}
