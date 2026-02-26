/*
  ==============================================================================

    O-AnalogSaturation - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OAnalogSaturationAudioProcessor::createParameterLayout()
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

OAnalogSaturationAudioProcessor::OAnalogSaturationAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
#if OUARICON_LICENSING_ENABLED
    licenseManager = std::make_unique<OuariconLicense>(
        "ouaricon-saturation", OUARICON_SUPABASE_URL, OUARICON_SUPABASE_ANON_KEY);
#endif
}

OAnalogSaturationAudioProcessor::~OAnalogSaturationAudioProcessor()
{
}

void OAnalogSaturationAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
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

    // Initialize TRANSFORMER model filters
    const int numChannels = getTotalNumOutputChannels();
    transformerLFBumpFilters.resize(numChannels);
    transformerHFSheenFilters.resize(numChannels);

    // TRANSFORMER frequency response filters
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

    // Initialize TUBE model filters
    tubePresenceFilters.resize(numChannels);

    // TUBE presence filter
    // Presence boost: Peak filter at 3000Hz, Q=0.7, +1.5dB
    auto presenceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, 3000.0f, 0.7f, juce::Decibels::decibelsToGain(1.5f));

    // Apply coefficients to all channels
    for (int ch = 0; ch < numChannels; ++ch)
    {
        *tubePresenceFilters[ch].coefficients = *presenceCoeffs;
        tubePresenceFilters[ch].reset();
    }

    // Initialize MAGNETIC model filters
    magneticHeadBumpFilters.resize(numChannels);
    magneticHFRolloffFilters.resize(numChannels);
    magneticM.resize(numChannels, 0.0f);      // Initialize magnetization to zero
    magneticHPrev.resize(numChannels, 0.0f);  // Initialize previous field to zero

    // MAGNETIC frequency response filters
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

    // Initialize Auto-Gain system
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

void OAnalogSaturationAudioProcessor::releaseResources()
{
    // Reset oversampling systems
    if (oversamplingMid)
        oversamplingMid->reset();
    if (oversamplingHigh)
        oversamplingHigh->reset();

    // Reset TRANSFORMER filters
    for (auto& filter : transformerLFBumpFilters)
        filter.reset();
    for (auto& filter : transformerHFSheenFilters)
        filter.reset();

    // Reset TUBE filters
    for (auto& filter : tubePresenceFilters)
        filter.reset();

    // Reset MAGNETIC model state
    std::fill(magneticM.begin(), magneticM.end(), 0.0f);
    std::fill(magneticHPrev.begin(), magneticHPrev.end(), 0.0f);
    for (auto& filter : magneticHeadBumpFilters)
        filter.reset();
    for (auto& filter : magneticHFRolloffFilters)
        filter.reset();

    // Reset Auto-Gain envelopes
    std::fill(inputRMSEnvelope.begin(), inputRMSEnvelope.end(), 0.0f);
    std::fill(outputRMSEnvelope.begin(), outputRMSEnvelope.end(), 0.0f);
}

void OAnalogSaturationAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic, real-time safe)
    const float intensity = parameters.getRawParameterValue("INTENSITY")->load();
    const int model = static_cast<int>(parameters.getRawParameterValue("MODEL")->load());
    const int quality = static_cast<int>(parameters.getRawParameterValue("QUALITY")->load());
    const bool autoGainEnabled = parameters.getRawParameterValue("AUTOGAIN")->load() > 0.5f;

    // Update latency if quality changed
    if (quality != currentQuality)
    {
        currentQuality = quality;

        int latency = 0;
        if (currentQuality == 1 && oversamplingMid)
            latency = static_cast<int>(oversamplingMid->getLatencyInSamples());
        else if (currentQuality == 2 && oversamplingHigh)
            latency = static_cast<int>(oversamplingHigh->getLatencyInSamples());

        setLatencySamples(latency);

        if (oversamplingMid)
            oversamplingMid->reset();
        if (oversamplingHigh)
            oversamplingHigh->reset();
    }

    // Capture input peak level for VU meter
    inputLevelDB.store(calculatePeakDB(buffer), std::memory_order_relaxed);

    // Capture input RMS for auto-gain (before saturation)
    captureInputRMS(buffer);

    // Process based on quality level
    if (quality == 0)
    {
        // LOW quality: No oversampling
        processSaturationDirect(buffer, model, intensity);
    }
    else
    {
        // MID/HIGH quality: Use oversampling
        auto* oversampler = (quality == 1) ? oversamplingMid.get() : oversamplingHigh.get();

        if (oversampler != nullptr)
        {
            auto oversampledBlock = oversampler->processSamplesUp(buffer);
            processSaturationBlock(oversampledBlock, model, intensity);

            juce::dsp::AudioBlock<float> outputBlock(buffer);
            oversampler->processSamplesDown(outputBlock);
        }
    }

    // Apply auto-gain compensation if enabled
    applyAutoGain(buffer, autoGainEnabled);

    // Capture output peak level for VU meter
    outputLevelDB.store(calculatePeakDB(buffer), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* OAnalogSaturationAudioProcessor::createEditor()
{
    return new OAnalogSaturationAudioProcessorEditor(*this);
}

void OAnalogSaturationAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OAnalogSaturationAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// ============================================================================
// Helper Methods for processBlock
// ============================================================================

float OAnalogSaturationAudioProcessor::calculatePeakDB(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        peak = std::max(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    }
    return (peak > 0.00001f) ? juce::Decibels::gainToDecibels(peak) : -100.0f;
}

void OAnalogSaturationAudioProcessor::captureInputRMS(const juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* channelData = buffer.getReadPointer(channel);

        float rmsSum = 0.0f;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            rmsSum += channelData[sample] * channelData[sample];
        }
        const float rms = std::sqrt(rmsSum / static_cast<float>(numSamples));

        inputRMSEnvelope[channel] = autoGainCoeff * inputRMSEnvelope[channel] + (1.0f - autoGainCoeff) * rms;

        if (inputRMSEnvelope[channel] < 1e-8f)
            inputRMSEnvelope[channel] = 0.0f;
    }
}

void OAnalogSaturationAudioProcessor::processSaturationDirect(
    juce::AudioBuffer<float>& buffer, int model, float intensity)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelData[sample] = processSample(channelData[sample], model, intensity, channel);
        }
    }
}

void OAnalogSaturationAudioProcessor::processSaturationBlock(
    juce::dsp::AudioBlock<float>& block, int model, float intensity)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int numSamples = static_cast<int>(block.getNumSamples());

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = block.getChannelPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelData[sample] = processSample(channelData[sample], model, intensity, channel);
        }
    }
}

float OAnalogSaturationAudioProcessor::processSample(
    float input, int model, float intensity, int channel)
{
    switch (model)
    {
        case 0: return processMagneticSample(input, intensity, channel);
        case 1: return processTubeSample(input, intensity, channel);
        case 2: return processTransformerSample(input, intensity, channel);
        case 3: return processDiodeSample(input, intensity);
        default: return input;
    }
}

void OAnalogSaturationAudioProcessor::applyAutoGain(juce::AudioBuffer<float>& buffer, bool enabled)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);

        // Capture output RMS
        float rmsSum = 0.0f;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            rmsSum += channelData[sample] * channelData[sample];
        }
        const float rms = std::sqrt(rmsSum / static_cast<float>(numSamples));

        outputRMSEnvelope[channel] = autoGainCoeff * outputRMSEnvelope[channel] + (1.0f - autoGainCoeff) * rms;

        if (outputRMSEnvelope[channel] < 1e-8f)
            outputRMSEnvelope[channel] = 0.0f;

        // Apply compensation gain if enabled
        if (enabled && outputRMSEnvelope[channel] > 1e-6f)
        {
            float compensationGain = inputRMSEnvelope[channel] / outputRMSEnvelope[channel];
            compensationGain = juce::jlimit(0.1f, 10.0f, compensationGain);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                channelData[sample] *= compensationGain;
            }
        }
    }
}

// ============================================================================
// DIODE Model Implementation (Symmetric Soft Clipping)
// ============================================================================

float OAnalogSaturationAudioProcessor::processDiodeSample(float input, float intensity)
{
    // At 0% intensity, return dry signal (no processing)
    if (intensity < 0.1f)
        return input;

    // Dry/wet mix: 0% = dry, 100% = full saturation
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float dryMix = 1.0f - wetMix;

    // Drive increases with intensity: 1.0 to 5.0
    const float drive = 1.0f + wetMix * 6.0f;  // 1.5x stronger (was 4.0)

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
// TRANSFORMER Model Implementation (Soft Tanh Saturation)
// ============================================================================

float OAnalogSaturationAudioProcessor::processTransformerSample(float input, float intensity, int channel)
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
    const float intensityGain = 1.0f + wetMix * 7.5f;  // 1.5x stronger (was 5.0)

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
// TUBE Model Implementation (Asymmetric Soft Saturation)
// ============================================================================

float OAnalogSaturationAudioProcessor::processTubeSample(float input, float intensity, int channel)
{
    // At 0% intensity, return dry signal (no processing)
    if (intensity < 0.1f)
        return input;

    // Dry/wet mix: 0% = dry, 100% = full saturation
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float dryMix = 1.0f - wetMix;

    // Drive increases with intensity: 1.0 to 4.0
    const float drive = 1.0f + wetMix * 4.5f;  // 1.5x stronger (was 3.0)

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
// MAGNETIC Model Implementation (Jiles-Atherton Hysteresis)
// ============================================================================

float OAnalogSaturationAudioProcessor::langevinFunction(float x)
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

float OAnalogSaturationAudioProcessor::processMagneticSample(float input, float intensity, int channel)
{
    // At 0% intensity, return dry signal (no processing)
    if (intensity < 0.1f)
        return input;

    // Dry/wet mix: 0% = dry, 100% = full saturation
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float dryMix = 1.0f - wetMix;

    // Drive scales with intensity: subtle at low values, heavy at high
    const float drive = 1.0f + wetMix * 3.0f;  // 1.5x stronger (was 2.0)

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
    return new OAnalogSaturationAudioProcessor();
}
