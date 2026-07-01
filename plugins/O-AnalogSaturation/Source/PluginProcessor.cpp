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

    // Store sample rate for the per-block auto-gain coefficient (CR-02)
    sampleRateHz = sampleRate;

    const int numChannels = getTotalNumOutputChannels();

    // Resize all model tone-filter banks (state is per-channel; coefficients are shared)
    transformerLFBumpFilters.resize(numChannels);
    transformerHFSheenFilters.resize(numChannels);
    tubePresenceFilters.resize(numChannels);
    magneticHeadBumpFilters.resize(numChannels);
    magneticHFRolloffFilters.resize(numChannels);
    magneticM.assign(numChannels, 0.0f);      // Initialize magnetization to zero
    magneticHPrev.assign(numChannels, 0.0f);  // Initialize previous field to zero

    // CR-01: These tone filters execute INSIDE the oversampled nonlinear path, so a
    // filter designed for `sampleRate` has its corners halved at 2x / quartered at 4x.
    // Design one coefficient set per Quality at the rate that path actually runs at
    // (base * osFactor), then select the active set below and on Quality change.
    for (int q = 0; q < 3; ++q)
    {
        const double osRate = sampleRate * (q == 2 ? 4.0 : q == 1 ? 2.0 : 1.0);

        // TRANSFORMER: LF bump 60Hz Q=0.7 +2.0dB, HF sheen 8kHz high shelf +1.0dB
        transformerLFBumpCoeffs[q] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            osRate, 60.0f, 0.7f, juce::Decibels::decibelsToGain(2.0f));
        transformerHFSheenCoeffs[q] = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            osRate, 8000.0f, 0.7f, juce::Decibels::decibelsToGain(1.0f));

        // TUBE: presence boost 3kHz Q=0.7 +1.5dB
        tubePresenceCoeffs[q] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            osRate, 3000.0f, 0.7f, juce::Decibels::decibelsToGain(1.5f));

        // MAGNETIC: head bump 80Hz Q=0.7 +2.5dB, HF rolloff 12kHz lowpass Q=0.707
        magneticHeadBumpCoeffs[q] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            osRate, 80.0f, 0.7f, juce::Decibels::decibelsToGain(2.5f));
        magneticHFRolloffCoeffs[q] = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            osRate, 12000.0f, 0.707f);
    }

    // Get initial quality setting BEFORE selecting the active coefficient set
    auto* qualityParam = parameters.getRawParameterValue("QUALITY");
    currentQuality = static_cast<int>(qualityParam->load());

    // Point every filter at its current-quality coefficients, then clear state
    applyQualityToneCoeffs(currentQuality);
    for (auto& f : transformerLFBumpFilters)  f.reset();
    for (auto& f : transformerHFSheenFilters) f.reset();
    for (auto& f : tubePresenceFilters)       f.reset();
    for (auto& f : magneticHeadBumpFilters)   f.reset();
    for (auto& f : magneticHFRolloffFilters)  f.reset();

    // Initialize Auto-Gain system
    inputRMSEnvelope.assign(numChannels, 0.0f);
    outputRMSEnvelope.assign(numChannels, 0.0f);

    // WR-03: per-channel smoothers so the auto-gain compensation ramps instead of stepping
    // at block boundaries. Start at unity so an enable never jumps.
    autoGainSmoothed.assign(static_cast<size_t>(numChannels), {});
    for (auto& sm : autoGainSmoothed)
    {
        sm.reset(sampleRate, AUTOGAIN_SMOOTHING_SECONDS);
        sm.setCurrentAndTargetValue(1.0f);
    }

    // WR-02: dry copy + latency-matched delay line. Size the delay for the largest latency
    // any Quality can request so switching never needs a (non-RT-safe) reallocation.
    currentLatencySamples = computeLatencyForQuality(currentQuality);
    const int maxLatency = juce::jmax(computeLatencyForQuality(1), computeLatencyForQuality(2));
    dryDelay.setMaximumDelayInSamples(maxLatency + 4);
    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32>(samplesPerBlock),
        static_cast<juce::uint32>(numChannels)
    };
    dryDelay.prepare(spec);
    dryDelay.setDelay(static_cast<float>(currentLatencySamples));
    dryDelay.reset();
    dryBuffer.setSize(numChannels, samplesPerBlock);
    dryBuffer.clear();

    // Report latency to host (prepareToPlay runs on the message thread, so a direct call
    // is safe here — only the audio-thread Quality-change path defers via AsyncUpdater).
    setLatencySamples(currentLatencySamples);
}

bool OAnalogSaturationAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // WR-04: accept only mono or stereo, and require matching input/output channel sets.
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void OAnalogSaturationAudioProcessor::handleAsyncUpdate()
{
    // WR-01: message-thread flush of the latency reported from the audio thread.
    setLatencySamples(pendingLatencySamples.load(std::memory_order_relaxed));
}

int OAnalogSaturationAudioProcessor::computeLatencyForQuality(int quality) const
{
    if (quality == 1 && oversamplingMid)
        return static_cast<int>(oversamplingMid->getLatencyInSamples());
    if (quality == 2 && oversamplingHigh)
        return static_cast<int>(oversamplingHigh->getLatencyInSamples());
    return 0;  // LOW quality: no oversampling, no added latency
}

float OAnalogSaturationAudioProcessor::osFactorForQuality(int quality)
{
    return quality == 2 ? 4.0f : quality == 1 ? 2.0f : 1.0f;
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

        // CR-01: retarget the tone filters at the coefficient set designed for the
        // new oversampling rate (RT-safe Ptr swap; same biquad order preserves state).
        // Also rescales the MAGNETIC deltaH clamp for the new oversampling factor.
        applyQualityToneCoeffs(currentQuality);

        // WR-02: match the dry delay to the new oversampler latency (RT-safe int update).
        currentLatencySamples = computeLatencyForQuality(currentQuality);
        dryDelay.setDelay(static_cast<float>(currentLatencySamples));

        // WR-01: defer the (non-RT-safe) host latency notification to the message thread.
        pendingLatencySamples.store(currentLatencySamples, std::memory_order_relaxed);
        triggerAsyncUpdate();

        if (oversamplingMid)
            oversamplingMid->reset();
        if (oversamplingHigh)
            oversamplingHigh->reset();
        dryDelay.reset();
    }

    // Capture input peak level for VU meter
    inputLevelDB.store(calculatePeakDB(buffer), std::memory_order_relaxed);

    // Capture input RMS for auto-gain (before saturation)
    captureInputRMS(buffer);

    // WR-02: snapshot a clean, base-rate dry copy BEFORE the oversampled nonlinear path so
    // the dry component is mixed back in later without the oversampler's FIR coloration.
    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (dryBuffer.getNumSamples() < numSamples || dryBuffer.getNumChannels() < numCh)
        dryBuffer.setSize(numCh, numSamples, false, false, true);  // fallback if host exceeds prepared size
    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Generate the pure WET (fully saturated) signal — models no longer mix dry internally.
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

    // WR-02: mix the clean dry (delayed to match oversampler latency) with the wet result.
    mixDryWet(buffer, dryBuffer, intensity);

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

void OAnalogSaturationAudioProcessor::applyQualityToneCoeffs(int quality)
{
    // CR-01: point every tone filter at the precomputed coefficient set for `quality`.
    // Assigning a Coefficients::Ptr is only a ref-count update (no allocation, no state
    // reset), so this is safe to call from the audio thread on a Quality change.
    const int q = juce::jlimit(0, 2, quality);
    for (auto& f : transformerLFBumpFilters)  f.coefficients = transformerLFBumpCoeffs[q];
    for (auto& f : transformerHFSheenFilters) f.coefficients = transformerHFSheenCoeffs[q];
    for (auto& f : tubePresenceFilters)       f.coefficients = tubePresenceCoeffs[q];
    for (auto& f : magneticHeadBumpFilters)   f.coefficients = magneticHeadBumpCoeffs[q];
    for (auto& f : magneticHFRolloffFilters)  f.coefficients = magneticHFRolloffCoeffs[q];

    // CR-01 addendum: scale the MAGNETIC per-sample deltaH clamp by the oversampling factor
    // so the realized field slew limit (per unit time) is the same at LOW/MID/HIGH.
    magneticDeltaHClamp = MAGNETIC_DELTAH_CLAMP_BASE / osFactorForQuality(q);
}

float OAnalogSaturationAudioProcessor::autoGainBlockCoeff(int numSamples) const
{
    // CR-02: one-pole coefficient for a single per-block update of `numSamples` samples,
    // so the realized time constant stays ~AUTOGAIN_TIME_CONSTANT_SECONDS independent of
    // host block size:  coeff = exp(-N / (tau * fs)).
    const float tcSamples = AUTOGAIN_TIME_CONSTANT_SECONDS * static_cast<float>(sampleRateHz);
    return std::exp(-static_cast<float>(numSamples) / juce::jmax(1.0f, tcSamples));
}

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

    // CR-03: a zero-length block would make rmsSum/numSamples = 0/0 = NaN, which the
    // < 1e-8f flush below never clears (NaN < x is false) and then poisons every
    // subsequent block. Nothing to measure in an empty block, so bail out.
    if (numSamples <= 0)
        return;

    const float coeff = autoGainBlockCoeff(numSamples);  // CR-02: per-block time constant

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* channelData = buffer.getReadPointer(channel);

        float rmsSum = 0.0f;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            rmsSum += channelData[sample] * channelData[sample];
        }
        const float rms = std::sqrt(rmsSum / static_cast<float>(numSamples));

        inputRMSEnvelope[channel] = coeff * inputRMSEnvelope[channel] + (1.0f - coeff) * rms;

        if (! std::isfinite(inputRMSEnvelope[channel]) || inputRMSEnvelope[channel] < 1e-8f)
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

void OAnalogSaturationAudioProcessor::mixDryWet(
    juce::AudioBuffer<float>& wetBuffer, const juce::AudioBuffer<float>& dry, float intensity)
{
    // WR-02: apply the dry/wet mix at base rate. INTENSITY sets the wet proportion (the
    // per-model drive scaling that shapes the wet signal already happened upstream).
    const float wetMix = juce::jlimit(0.0f, 1.0f, intensity / 100.0f);
    const float dryMix = 1.0f - wetMix;
    const int numCh = wetBuffer.getNumChannels();
    const int numSamples = wetBuffer.getNumSamples();

    if (currentLatencySamples <= 0)
    {
        // LOW quality (no oversampler latency): dry and wet are already sample-aligned.
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* dryData = dry.getReadPointer(ch);
            float* wetData = wetBuffer.getWritePointer(ch);
            for (int n = 0; n < numSamples; ++n)
                wetData[n] = dryMix * dryData[n] + wetMix * wetData[n];
        }
    }
    else
    {
        // MID/HIGH: delay the clean dry by the oversampler latency so it stays phase-aligned
        // with the wet path (which the oversampler delayed by the same amount).
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* dryData = dry.getReadPointer(ch);
            float* wetData = wetBuffer.getWritePointer(ch);
            for (int n = 0; n < numSamples; ++n)
            {
                dryDelay.pushSample(ch, dryData[n]);
                const float dryDelayed = dryDelay.popSample(ch);
                wetData[n] = dryMix * dryDelayed + wetMix * wetData[n];
            }
        }
    }
}

void OAnalogSaturationAudioProcessor::applyAutoGain(juce::AudioBuffer<float>& buffer, bool enabled)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // CR-03: guard against a zero-length block poisoning the output RMS envelope with NaN.
    if (numSamples <= 0)
        return;

    const float coeff = autoGainBlockCoeff(numSamples);  // CR-02: per-block time constant

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

        outputRMSEnvelope[channel] = coeff * outputRMSEnvelope[channel] + (1.0f - coeff) * rms;

        if (! std::isfinite(outputRMSEnvelope[channel]) || outputRMSEnvelope[channel] < 1e-8f)
            outputRMSEnvelope[channel] = 0.0f;

        // WR-03: ramp the compensation gain per sample toward the block target instead of a
        // flat per-block multiply, so a changing gain doesn't step (zipper) at boundaries.
        auto& smoothed = autoGainSmoothed[static_cast<size_t>(channel)];
        if (enabled && outputRMSEnvelope[channel] > 1e-6f)
        {
            float compensationGain = inputRMSEnvelope[channel] / outputRMSEnvelope[channel];
            compensationGain = juce::jlimit(0.1f, 10.0f, compensationGain);
            smoothed.setTargetValue(compensationGain);

            for (int sample = 0; sample < numSamples; ++sample)
                channelData[sample] *= smoothed.getNextValue();
        }
        else
        {
            // Disabled/silent: hold at unity so the next enable ramps cleanly from 1.0.
            smoothed.setCurrentAndTargetValue(1.0f);
        }
    }
}

// ============================================================================
// DIODE Model Implementation (Symmetric Soft Clipping)
// ============================================================================

float OAnalogSaturationAudioProcessor::processDiodeSample(float input, float intensity)
{
    // At 0% intensity there is nothing to saturate; the dry/wet mix (applied downstream in
    // mixDryWet) weights the wet path to ~0 anyway, so returning the input is transparent.
    if (intensity < 0.1f)
        return input;

    // INTENSITY sets the input drive; the dry/wet balance is applied later in mixDryWet.
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float drive = 1.0f + wetMix * DIODE_DRIVE_RANGE;

    // Apply input drive
    float x = input * drive;

    // Anti-parallel diode waveshaper (symmetric soft clipping)
    // This models the classic TS-style diode clipper sound
    // Formula: x / (1 + |x|)^n where n controls hardness
    // n=1.0 is very soft, n=0.5 is harder (more like real diodes)
    float wetSignal = x / std::pow(1.0f + std::abs(x), DIODE_HARDNESS);

    // Add subtle odd harmonics (characteristic of symmetric clipping)
    // Diodes produce primarily odd harmonics due to symmetric clipping
    const float x3 = wetSignal * wetSignal * wetSignal;
    wetSignal = wetSignal * 0.9f + x3 * 0.1f;  // Subtle 3rd harmonic

    return wetSignal;  // pure wet; dry mixed in by mixDryWet at base rate
}

// ============================================================================
// TRANSFORMER Model Implementation (Soft Tanh Saturation)
// ============================================================================

float OAnalogSaturationAudioProcessor::processTransformerSample(float input, float intensity, int channel)
{
    // At 0% intensity there is nothing to saturate (mixDryWet weights wet to ~0 anyway).
    if (intensity < 0.1f)
        return input;

    // INTENSITY sets the input drive; the dry/wet balance is applied later in mixDryWet.
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float intensityGain = 1.0f + wetMix * TRANSFORMER_DRIVE_RANGE;

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

    return wetSignal;  // pure wet; dry mixed in by mixDryWet at base rate
}

// ============================================================================
// TUBE Model Implementation (Asymmetric Soft Saturation)
// ============================================================================

float OAnalogSaturationAudioProcessor::processTubeSample(float input, float intensity, int channel)
{
    // At 0% intensity there is nothing to saturate (mixDryWet weights wet to ~0 anyway).
    if (intensity < 0.1f)
        return input;

    // INTENSITY sets the input drive; the dry/wet balance is applied later in mixDryWet.
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float drive = 1.0f + wetMix * TUBE_DRIVE_RANGE;

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
    wetSignal *= TUBE_OUTPUT_NORMALIZATION;

    // Apply presence filter (3kHz peak, Q=0.7, +1.5dB)
    wetSignal = tubePresenceFilters[static_cast<size_t>(channel)].processSample(wetSignal);

    return wetSignal;  // pure wet; dry mixed in by mixDryWet at base rate
}

// ============================================================================
// MAGNETIC Model Implementation (Jiles-Atherton Hysteresis)
// ============================================================================

float OAnalogSaturationAudioProcessor::langevinFunction(float x)
{
    // Langevin function: L(x) = coth(x) - 1/x
    // IN-02: below LANGEVIN_TAYLOR_THRESHOLD use the Taylor series L(x) ≈ x/3 - x³/45. This
    // both avoids the 1/x singularity and dodges catastrophic cancellation in coth(x)-1/x
    // for small x — and shares the exact threshold used by the derivative branch below.

    if (std::abs(x) < LANGEVIN_TAYLOR_THRESHOLD)
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
    // At 0% intensity there is nothing to saturate (mixDryWet weights wet to ~0 anyway).
    if (intensity < 0.1f)
        return input;

    // INTENSITY sets the input drive; the dry/wet balance is applied later in mixDryWet.
    const float wetMix = intensity / 100.0f;  // 0.0 to 1.0
    const float drive = 1.0f + wetMix * MAGNETIC_DRIVE_RANGE;

    // Apply input drive
    float H = input * drive;  // Magnetic field (input signal)

    // Read state variables
    float& M = magneticM[static_cast<size_t>(channel)];
    float& H_prev = magneticHPrev[static_cast<size_t>(channel)];

    // Limit deltaH to prevent initialization transient and reduce noise.
    // CR-01 addendum: magneticDeltaHClamp is scaled by the oversampling factor so the
    // realized field slew limit is identical across Quality (see applyQualityToneCoeffs).
    float deltaH = H - H_prev;
    deltaH = juce::jlimit(-magneticDeltaHClamp, magneticDeltaHClamp, deltaH);

    // Calculate direction (sign of field change)
    float delta = (deltaH >= 0.0f) ? 1.0f : -1.0f;

    // Calculate effective field
    const float He = H + MAGNETIC_ALPHA * M;

    // Calculate anhysteretic magnetization using Langevin function
    const float arg = He / MAGNETIC_A;
    const float Man = MAGNETIC_MS * langevinFunction(arg);

    // Calculate derivative of Langevin function
    // IN-02: same LANGEVIN_TAYLOR_THRESHOLD as langevinFunction so L and L' use matching
    // approximations across the crossover window.
    float dLdx = 0.0f;
    if (std::abs(arg) < LANGEVIN_TAYLOR_THRESHOLD)
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

    return wetSignal;  // pure wet; dry mixed in by mixDryWet at base rate
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OAnalogSaturationAudioProcessor();
}
