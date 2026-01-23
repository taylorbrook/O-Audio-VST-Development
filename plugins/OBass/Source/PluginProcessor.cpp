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

    // Pre-allocate intermediate buffers
    lowBandBuffer.setSize(2, samplesPerBlock);
    highBandBuffer.setSize(2, samplesPerBlock);
    monoBuffer.setSize(1, samplesPerBlock);

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

    // Report combined latency to host
    updateLatencyReport();
}

void OBassAudioProcessor::releaseResources()
{
    crossover.reset();
    monoSummer.reset();
    cleanModeProcessor.reset();
}

void OBassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic, real-time safe)
    auto* bypassParam = parameters.getRawParameterValue("bypass");
    auto* crossoverParam = parameters.getRawParameterValue("crossover_freq");
    auto* modeParam = parameters.getRawParameterValue("latency_mode");
    auto* enhanceParam = parameters.getRawParameterValue("enhance");

    bool bypassed = bypassParam->load() > 0.5f;
    float enhanceAmount = enhanceParam->load() / 100.0f;  // Convert from % to 0-1

    // TRUE BYPASS: signal passes through unmodified
    if (bypassed)
        return;

    // Update crossover frequency (SmoothedValue handles interpolation)
    crossover.setCutoffFrequency(crossoverParam->load());

    // Check for mode change (update latency if changed)
    auto currentMode = modeParam->load() < 0.5f ? CrossoverFilter::Mode::LowLatency
                                                 : CrossoverFilter::Mode::HighFidelity;
    if (currentMode != crossover.getMode())
    {
        crossover.setMode(currentMode);
        updateLatencyReport();
    }

    // Update Clean Mode processor settings
    cleanModeProcessor.setEnhanceAmount(enhanceAmount);

    // Mode sync (if latency mode changed, sync Clean Mode)
    auto cleanMode = currentMode == CrossoverFilter::Mode::LowLatency
        ? CleanModeProcessor::Mode::LowLatency
        : CleanModeProcessor::Mode::HighFidelity;
    if (cleanMode != cleanModeProcessor.getMode())
    {
        cleanModeProcessor.setMode(cleanMode);
        updateLatencyReport();
    }

    // Resize intermediate buffers if needed (should be pre-allocated, but defensive)
    const int numSamples = buffer.getNumSamples();
    if (lowBandBuffer.getNumSamples() < numSamples)
    {
        // This should NOT happen if prepareToPlay was called correctly
        jassertfalse;
        lowBandBuffer.setSize(2, numSamples, false, false, true);
        highBandBuffer.setSize(2, numSamples, false, false, true);
        monoBuffer.setSize(1, numSamples, false, false, true);
    }

    // === SIGNAL PATH ===

    // 1. Split into low and high bands
    crossover.process(buffer, lowBandBuffer, highBandBuffer);

    // 2. Calculate high band energy for spectral-aware blending
    float highBandEnergy = calculateHighBandEnergy(highBandBuffer);
    cleanModeProcessor.setHighBandEnergy(highBandEnergy);

    // 3. Sum low band to mono
    monoSummer.captureBalance(lowBandBuffer);
    monoSummer.sumToMono(lowBandBuffer, monoBuffer);

    // 4. Apply Clean Mode harmonic enhancement
    if (enhanceAmount > 0.001f)  // Skip processing if enhance is off
        cleanModeProcessor.process(monoBuffer);

    // 5. Expand mono back to stereo
    monoSummer.expandToStereo(monoBuffer, lowBandBuffer);

    // 6. Recombine bands into output
    recombineBands(buffer, lowBandBuffer, highBandBuffer);
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
    float sum = 0.0f;
    const int numSamples = highBand.getNumSamples();
    const int numChannels = highBand.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* data = highBand.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            sum += data[i] * data[i];  // RMS
    }

    // Normalize and convert to 0-1 range (assume typical energy level)
    return juce::jmin(1.0f, std::sqrt(sum / (numSamples * numChannels)) * 5.0f);
}

// Factory function - creates plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBassAudioProcessor();
}
