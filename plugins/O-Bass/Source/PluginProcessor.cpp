/*
  ==============================================================================

    O-Bass - Audio Processor Implementation
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
    , presetManager(parameters, "OBass")
{
    // Initialize factory presets
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        // Default - neutral starting point
        {"Default", {{"crossover_freq", 0.25f}, {"enhance", 0.50f}, {"output", 0.5f}}, juce::var()},

        // Bass enhancement presets
        {"Gentle Bass Guitar", {{"crossover_freq", 0.375f}, {"enhance", 0.30f}, {"output", 0.5f}}, juce::var()},
        {"Punchy 808", {{"crossover_freq", 0.0f}, {"enhance", 0.70f}, {"output", 0.5f}}, juce::var()},
        {"Subtle Mix Glue", {{"crossover_freq", 0.50f}, {"enhance", 0.20f}, {"output", 0.5f}}, juce::var()},
        {"Full Sub Enhancement", {{"crossover_freq", 0.125f}, {"enhance", 0.80f}, {"output", 0.45f}}, juce::var()},
        {"Warm Bass Guitar", {{"crossover_freq", 0.375f}, {"enhance", 0.50f}, {"output", 0.5f}}, juce::var()},
        {"Fat Synth Bass", {{"crossover_freq", 0.25f}, {"enhance", 0.65f}, {"output", 0.48f}}, juce::var()},
        {"Saturated Sub", {{"crossover_freq", 0.0f}, {"enhance", 0.85f}, {"output", 0.42f}}, juce::var()},
        {"Vintage Mix Bus", {{"crossover_freq", 0.50f}, {"enhance", 0.35f}, {"output", 0.5f}}, juce::var()},
        {"Maximum Enhancement", {{"crossover_freq", 0.25f}, {"enhance", 0.90f}, {"output", 0.40f}}, juce::var()}
    };

    presetManager.initializeFactoryPresets(factoryPresets);
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
    lowBandBuffer.clear();
    highBandBuffer.clear();
    monoBuffer.clear();

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

    // Initialize smoothed enhance (20ms ramp time for click-free transitions)
    smoothedEnhance.reset(sampleRate, 0.020);
    auto* enhanceParam = parameters.getRawParameterValue("enhance");
    smoothedEnhance.setCurrentAndTargetValue(enhanceParam->load() / 100.0f);

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

    // Reset SmoothedValue objects to prevent stale state
    smoothedEnhance.reset(0);
    outputGainSmooth.reset(0);
    limitIndicatorSmooth.reset(0);
}

void OBassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // DEBUG: Enabling sections one at a time to find crash source
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

    // Buffers are pre-allocated in prepareToPlay() - assert in debug builds only
    // Removed runtime resize checks for performance (allocation in processBlock is RT-unsafe)
    jassert(lowBandBuffer.getNumSamples() >= numSamples);
    jassert(highBandBuffer.getNumSamples() >= numSamples);
    jassert(monoBuffer.getNumSamples() >= numSamples);

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

    // Apply intensity scale to processor
    cleanModeProcessor.setIntensityScale(intensityScale);

    // Split into low and high bands
    crossover.process(buffer, lowBandBuffer, highBandBuffer);

    // Create views that match the actual sample count (not allocated size)
    juce::AudioBuffer<float> lowBandView(lowBandBuffer.getArrayOfWritePointers(), 2, numSamples);
    juce::AudioBuffer<float> monoView(monoBuffer.getArrayOfWritePointers(), 1, numSamples);

    // Sum low band to mono for harmonic processing
    monoSummer.captureBalance(lowBandView);
    monoSummer.sumToMono(lowBandView, monoView);

    // Get smoothed enhance value
    float smoothedEnhanceValue = smoothedEnhance.skip(numSamples);

    // Set enhance amount on clean processor only
    cleanModeProcessor.setEnhanceAmount(smoothedEnhanceValue);

    // Calculate high-band energy for spectral blending
    float highEnergy = calculateHighBandEnergy(highBandBuffer);
    cleanModeProcessor.setHighBandEnergy(highEnergy);

    // Clean Mode processing - harmonic generation with 4x oversampling
    // Fixed in v1.0.1: Added buffer size validation to prevent memory corruption
    cleanModeProcessor.process(monoView);

    // Expand back to stereo
    monoSummer.expandToStereo(monoView, lowBandView);

    // Create view for high band and recombine
    {
        juce::AudioBuffer<float> hbView(highBandBuffer.getArrayOfWritePointers(), 2, numSamples);
        recombineBands(buffer, lowBandView, hbView);
    }

    // Apply output gain with soft clipping
    auto* outputParam = parameters.getRawParameterValue("output");
    float targetGainLinear = juce::Decibels::decibelsToGain(outputParam->load());
    outputGainSmooth.setTargetValue(targetGainLinear);

    // Apply gain per-sample with soft clip protection
    const int numChannels = buffer.getNumChannels();
    float maxLimitAmount = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float gain = outputGainSmooth.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float sample = buffer.getSample(ch, i) * gain;

            // Soft clip at ~0.95 to prevent digital clipping
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
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OBassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        presetManager.setStateFromXml(xml.get());
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
    // DISABLED: Latency reporting causes Logic Pro crash ("Sample Rate XXXXX" error)
    // Root cause unknown - possibly related to how Logic interprets latency values.
    // Workaround: Always report 0 latency.
    // Impact: No DAW latency compensation, acceptable for bass enhancement.
    setLatencySamples(0);
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
