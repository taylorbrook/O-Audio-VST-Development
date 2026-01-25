/*
  ==============================================================================

    OBass - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Bass enhancement plugin with crossover filtering and harmonic generation.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OBassAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // crossover_freq - Crossover frequency for bass/high split
    // Range: 40-200Hz, default 80Hz, with frequency skew for natural feel
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "crossover_freq", 1 },
        "Crossover",
        juce::NormalisableRange<float>(40.0f, 200.0f, 1.0f, 0.5f),  // skew 0.5 for frequency
        80.0f,
        "Hz"
    ));

    // latency_mode - Processing quality/latency tradeoff
    // 0 = Low Latency (minimum-phase IIR), 1 = High Fidelity (linear-phase FIR)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "latency_mode", 1 },
        "Mode",
        juce::StringArray { "Low Latency", "High Fidelity" },
        0  // Default: Low Latency
    ));

    // bypass - Full plugin bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bypass", 1 },
        "Bypass",
        false
    ));

    // enhance - Bass enhancement amount (Clean Mode intensity)
    // Range: 0-100%, default 50%
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "enhance", 1 },
        "Enhance",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // enhanceMode - Clean vs Colored processing character
    // 0 = Clean (transparent, odd harmonics), 1 = Colored (warm, even harmonics)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "enhanceMode", 1 },
        "Enhance Mode",
        juce::StringArray { "Clean", "Colored" },
        0  // Default to Clean
    ));

    // output - Output gain compensation
    // Range: -18dB to +18dB, default 0dB (unity)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output", 1 },
        "Output",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
        0.0f,  // Default 0dB
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
    ));

    return layout;
}

OBassAudioProcessor::OBassAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OBassAudioProcessor::~OBassAudioProcessor()
{
}

void OBassAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Configure DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Pre-allocate intermediate buffers and clear them
    lowBandBuffer.setSize(2, samplesPerBlock);
    highBandBuffer.setSize(2, samplesPerBlock);
    monoBuffer.setSize(1, samplesPerBlock);
    coloredBuffer.setSize(1, samplesPerBlock);  // Same size as monoBuffer
    lowBandBuffer.clear();
    highBandBuffer.clear();
    monoBuffer.clear();
    coloredBuffer.clear();

    // Prepare DSP components
    crossover.prepare(spec);
    monoSummer.prepare(samplesPerBlock);

    // Set initial crossover frequency from parameter
    auto* crossoverParam = parameters.getRawParameterValue("crossover_freq");
    crossover.setCutoffFrequency(crossoverParam->load());

    // Set initial mode from parameter
    auto* modeParam = parameters.getRawParameterValue("latency_mode");
    auto mode = modeParam->load() < 0.5f ? CrossoverFilter::Mode::LowLatency
                                          : CrossoverFilter::Mode::HighFidelity;
    crossover.setMode(mode);

    // Prepare Clean Mode processor
    cleanModeProcessor.prepare(spec);

    // Set initial Clean Mode from latency_mode parameter
    auto cleanMode = modeParam->load() < 0.5f ? CleanModeProcessor::Mode::LowLatency
                                               : CleanModeProcessor::Mode::HighFidelity;
    cleanModeProcessor.setMode(cleanMode);

    // Prepare Colored Mode processor
    coloredModeProcessor.prepare(spec);

    // Set initial Colored Mode from latency_mode parameter
    auto coloredMode = modeParam->load() < 0.5f ? ColoredModeProcessor::Mode::LowLatency
                                                 : ColoredModeProcessor::Mode::HighFidelity;
    coloredModeProcessor.setMode(coloredMode);

    // Initialize smoothed enhance (20ms ramp time for click-free transitions)
    smoothedEnhance.reset(sampleRate, 0.020);
    auto* enhanceParam = parameters.getRawParameterValue("enhance");
    smoothedEnhance.setCurrentAndTargetValue(enhanceParam->load() / 100.0f);

    // Initialize mode crossfade (20ms ramp for click-free mode switching)
    modeCrossfade.reset(sampleRate, 0.020);
    auto* enhanceModeParam = parameters.getRawParameterValue("enhanceMode");
    modeCrossfade.setCurrentAndTargetValue(enhanceModeParam->load());  // 0.0 = Clean, 1.0 = Colored

    // Initialize smoothed output gain (20ms ramp time, multiplicative for dB scale)
    outputGainSmooth.reset(sampleRate, 0.020);
    auto* outputParam = parameters.getRawParameterValue("output");
    float initialGainLinear = juce::Decibels::decibelsToGain(outputParam->load());
    outputGainSmooth.setCurrentAndTargetValue(initialGainLinear);

    // Initialize limit indicator with 100ms decay for smooth UI display
    limitIndicatorSmooth.reset(sampleRate, 0.100);
    limitIndicatorSmooth.setCurrentAndTargetValue(0.0f);
    limitIndicator.store(0.0f);

    // Report combined latency to host
    updateLatencyReport();
}

void OBassAudioProcessor::releaseResources()
{
    crossover.reset();
    monoSummer.reset();
    cleanModeProcessor.reset();
    coloredModeProcessor.reset();
}

void OBassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, numSamples);

    // Read parameters
    auto* bypassParam = parameters.getRawParameterValue("bypass");
    auto* enhanceParam = parameters.getRawParameterValue("enhance");

    bool bypassed = bypassParam->load() > 0.5f;
    float targetEnhance = bypassed ? 0.0f : enhanceParam->load() / 100.0f;

    // Smooth the enhance value to avoid clicks on bypass toggle
    smoothedEnhance.setTargetValue(targetEnhance);

    // Resize buffers if needed
    if (lowBandBuffer.getNumSamples() < numSamples)
    {
        lowBandBuffer.setSize(2, numSamples, false, false, true);
        highBandBuffer.setSize(2, numSamples, false, false, true);
    }
    if (monoBuffer.getNumSamples() < numSamples)
    {
        monoBuffer.setSize(1, numSamples, false, false, true);
        monoBuffer.clear();
    }
    if (coloredBuffer.getNumSamples() < numSamples)
    {
        coloredBuffer.setSize(1, numSamples, false, false, true);
        coloredBuffer.clear();
    }

    // Update crossover frequency from parameter
    auto* crossoverParam = parameters.getRawParameterValue("crossover_freq");
    float crossoverHz = crossoverParam->load();
    crossover.setCutoffFrequency(crossoverHz);

    // Calculate frequency-dependent intensity scale
    // Lower crossover frequencies need more enhancement for psychoacoustic perception
    // 40Hz -> scale 1.7 (strong boost for sub-bass)
    // 200Hz -> scale 1.0 (no boost, upper bass is more audible)
    float normalized = juce::jlimit(0.0f, 1.0f, (crossoverHz - 40.0f) / 160.0f);
    float intensityScale = 1.0f + std::sqrt(1.0f - normalized) * 0.7f;

    // Apply intensity scale to both processors
    cleanModeProcessor.setIntensityScale(intensityScale);
    coloredModeProcessor.setIntensityScale(intensityScale);

    // Split into low and high bands
    crossover.process(buffer, lowBandBuffer, highBandBuffer);

    // Sum low band to mono for harmonic processing
    monoSummer.captureBalance(lowBandBuffer);
    monoSummer.sumToMono(lowBandBuffer, monoBuffer);

    // Read mode parameter and update crossfade target
    auto* modeParam = parameters.getRawParameterValue("enhanceMode");
    float modeValue = modeParam->load();
    modeCrossfade.setTargetValue(modeValue);  // 0.0 = Clean, 1.0 = Colored

    // Get smoothed enhance value (for both processors)
    float smoothedEnhanceValue = smoothedEnhance.skip(numSamples);

    // Set enhance amount on both processors
    cleanModeProcessor.setEnhanceAmount(smoothedEnhanceValue);
    coloredModeProcessor.setEnhanceAmount(smoothedEnhanceValue);

    // Calculate high-band energy for spectral blending
    float highEnergy = calculateHighBandEnergy(highBandBuffer);
    cleanModeProcessor.setHighBandEnergy(highEnergy);
    // Note: ColoredModeProcessor doesn't have setHighBandEnergy - asymmetric saturation
    // doesn't need it as the character is inherently warm regardless of context

    // Copy mono buffer for colored processing (parallel path)
    coloredBuffer.makeCopyOf(monoBuffer);

    // Process both paths
    cleanModeProcessor.process(monoBuffer);      // Clean result in monoBuffer
    coloredModeProcessor.process(coloredBuffer); // Colored result in coloredBuffer

    // Crossfade between paths (per-sample for smooth transition)
    float* cleanData = monoBuffer.getWritePointer(0);
    const float* coloredData = coloredBuffer.getReadPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        float blend = modeCrossfade.getNextValue();
        cleanData[i] = cleanData[i] * (1.0f - blend) + coloredData[i] * blend;
    }

    // Expand back to stereo
    monoSummer.expandToStereo(monoBuffer, lowBandBuffer);

    // Recombine low + high bands
    recombineBands(buffer, lowBandBuffer, highBandBuffer);

    // Apply output gain with soft clipping
    auto* outputParam = parameters.getRawParameterValue("output");
    float targetGainLinear = juce::Decibels::decibelsToGain(outputParam->load());
    outputGainSmooth.setTargetValue(targetGainLinear);

    // Apply gain per-sample with soft clip protection
    // NOTE: This output soft clipper is DEFENSE-IN-DEPTH, not duplicating processor limiting.
    // - Processors (Clean/Colored) have internal tanh limiting at their processing stage
    // - This output clipper catches: user Output boost + hot input + enhancement stacking
    // - Threshold 0.95 prevents true 0dBFS clipping while allowing full loudness
    // - Processors limit ~-2dB internally; this catches the final gain stage only
    const int numChannels = buffer.getNumChannels();
    float maxLimitAmount = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float gain = outputGainSmooth.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float sample = buffer.getSample(ch, i) * gain;

            // Soft clip at ~0.95 to prevent digital clipping
            // Uses tanh for smooth limiting at extreme output gain
            // This is intentionally a higher threshold than processor limiting
            // to allow normal dynamics while catching gain-boosted peaks
            if (std::abs(sample) > 0.95f)
            {
                float sign = (sample > 0.0f) ? 1.0f : -1.0f;
                float excess = std::abs(sample) - 0.95f;
                float limitedSample = sign * (0.95f + std::tanh(excess * 10.0f) * 0.05f);

                // Track limiting amount for UI indicator
                float limitAmount = std::abs(sample) - std::abs(limitedSample);
                maxLimitAmount = std::max(maxLimitAmount, limitAmount);

                sample = limitedSample;
            }

            buffer.setSample(ch, i, sample);
        }
    }

    // Update limit indicator (smoothed for UI display)
    // Convert limit amount to 0-1 range (0.1 excess = full limiting)
    float normalizedLimit = juce::jlimit(0.0f, 1.0f, maxLimitAmount * 10.0f);
    limitIndicatorSmooth.setTargetValue(normalizedLimit);
    limitIndicator.store(limitIndicatorSmooth.skip(numSamples));
}

juce::AudioProcessorEditor* OBassAudioProcessor::createEditor()
{
    return new OBassAudioProcessorEditor(*this);
}

void OBassAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameter state using APVTS
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OBassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameter state using APVTS
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Helper Methods
//==============================================================================

void OBassAudioProcessor::recombineBands(juce::AudioBuffer<float>& output,
                                          const juce::AudioBuffer<float>& lowBand,
                                          const juce::AudioBuffer<float>& highBand)
{
    // LR4 crossover sums flat (both bands at -6dB at crossover)
    // Simply add low + high
    const int numSamples = output.getNumSamples();
    const int numChannels = output.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* out = output.getWritePointer(ch);
        const auto* low = lowBand.getReadPointer(ch);
        const auto* high = highBand.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            out[i] = low[i] + high[i];
        }
    }
}

void OBassAudioProcessor::updateLatencyReport()
{
    int latencySamples = crossover.getLatencyInSamples()
                       + cleanModeProcessor.getLatencyInSamples();

    // Safety: cap latency to reasonable maximum (500ms at 48kHz = 24000 samples)
    latencySamples = juce::jmin(latencySamples, 24000);

    setLatencySamples(latencySamples);
    lastReportedMode = crossover.getMode();
}

void OBassAudioProcessor::setLatencyMode(CrossoverFilter::Mode mode)
{
    crossover.setMode(mode);
    updateLatencyReport();
}

float OBassAudioProcessor::calculateHighBandEnergy(const juce::AudioBuffer<float>& highBand)
{
    const int numSamples = highBand.getNumSamples();
    const int numChannels = highBand.getNumChannels();

    // Safety: avoid division by zero
    if (numSamples == 0 || numChannels == 0)
        return 0.0f;

    float sum = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* data = highBand.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            sum += data[i] * data[i];  // RMS
    }

    // Normalize and convert to 0-1 range (assume typical energy level)
    return juce::jmin(1.0f, std::sqrt(sum / static_cast<float>(numSamples * numChannels)) * 5.0f);
}

// Factory function - creates plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBassAudioProcessor();
}
