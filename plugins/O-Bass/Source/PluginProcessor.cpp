/*
   This file is part of O-Bass, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-Bass - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Bass enhancement plugin with crossover filtering and harmonic generation.

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is deliberately NOT included at the top of this TU — the
// include lives inside the #if JUCE_WEB_BROWSER guard directly above
// createEditor(), so a console target that compiles this TU with
// JUCE_WEB_BROWSER=0 and no editor sources (scripts/param-dump) links.

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
    // WR-01: crossover_freq has skew 0.5, so its stored value is a NORMALISED APVTS
    // value, not a linear fraction. Author the table in engineering units (Hz) and
    // convert through the real NormalisableRange so the skew is honoured — otherwise
    // the whole table compresses into the bottom quarter of the 40-200 Hz control
    // (e.g. "Default" would land at 50 Hz instead of the plugin's true 80 Hz default).
    const auto& freqRange = parameters.getParameterRange("crossover_freq");
    auto freqNorm = [&freqRange](float hz) { return freqRange.convertTo0to1(hz); };

    // Initialize factory presets (crossover in Hz; enhance is % of 0-100, output is
    // normalised on -18..+18 dB where 0.5 == 0 dB — both already unskewed/correct)
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        // Default - neutral starting point (80 Hz matches the plugin's initial state)
        {"Default", {{"crossover_freq", freqNorm(80.0f)}, {"enhance", 0.50f}, {"output", 0.5f}}, juce::var()},

        // Bass enhancement presets
        {"Gentle Bass Guitar", {{"crossover_freq", freqNorm(100.0f)}, {"enhance", 0.30f}, {"output", 0.5f}}, juce::var()},
        {"Punchy 808", {{"crossover_freq", freqNorm(40.0f)}, {"enhance", 0.70f}, {"output", 0.5f}}, juce::var()},
        {"Subtle Mix Glue", {{"crossover_freq", freqNorm(120.0f)}, {"enhance", 0.20f}, {"output", 0.5f}}, juce::var()},
        {"Full Sub Enhancement", {{"crossover_freq", freqNorm(60.0f)}, {"enhance", 0.80f}, {"output", 0.45f}}, juce::var()},
        {"Warm Bass Guitar", {{"crossover_freq", freqNorm(100.0f)}, {"enhance", 0.50f}, {"output", 0.5f}}, juce::var()},
        {"Fat Synth Bass", {{"crossover_freq", freqNorm(80.0f)}, {"enhance", 0.65f}, {"output", 0.48f}}, juce::var()},
        {"Saturated Sub", {{"crossover_freq", freqNorm(40.0f)}, {"enhance", 0.85f}, {"output", 0.42f}}, juce::var()},
        {"Vintage Mix Bus", {{"crossover_freq", freqNorm(120.0f)}, {"enhance", 0.35f}, {"output", 0.5f}}, juce::var()},
        {"Maximum Enhancement", {{"crossover_freq", freqNorm(80.0f)}, {"enhance", 0.90f}, {"output", 0.40f}}, juce::var()}
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

    // WR-02: seed the runtime mode cache so processBlock's on-change check starts in sync
    lastLatencyMode = mode;

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

    // Initialize output level meter with fast attack (20ms) and slow release (300ms)
    outputLevelSmooth.reset(sampleRate, 0.300);  // Release time
    outputLevelSmooth.setCurrentAndTargetValue(-60.0f);
    outputLevelDB.store(-60.0f);

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
    outputLevelSmooth.reset(0);
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

    // WR-02: apply latency_mode changes at runtime. Both setMode() calls are RT-safe
    // atomic flips (the FIR bank is pre-loaded in prepare(); only the deferred FIR
    // *index* reload on a frequency change still waits for the next prepareToPlay()).
    // Fire only on change so we don't churn the atomics every block.
    auto* modeParam = parameters.getRawParameterValue("latency_mode");
    auto currentMode = modeParam->load() < 0.5f ? CrossoverFilter::Mode::LowLatency
                                                 : CrossoverFilter::Mode::HighFidelity;
    if (currentMode != lastLatencyMode)
    {
        crossover.setMode(currentMode);
        cleanModeProcessor.setMode(currentMode == CrossoverFilter::Mode::LowLatency
                                       ? CleanModeProcessor::Mode::LowLatency
                                       : CleanModeProcessor::Mode::HighFidelity);
        lastLatencyMode = currentMode;
    }

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

    // Calculate and store output level for VU meter with ballistics
    float peakLevel = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        peakLevel = std::max(peakLevel, buffer.getMagnitude(ch, 0, numSamples));
    float peakDB = juce::Decibels::gainToDecibels(peakLevel, -60.0f);

    // Apply meter ballistics: fast attack, slow release
    float currentSmoothed = outputLevelSmooth.getCurrentValue();
    if (peakDB > currentSmoothed)
    {
        // Fast attack - jump to new peak
        outputLevelSmooth.setCurrentAndTargetValue(peakDB);
    }
    else
    {
        // Slow release - decay towards new value
        outputLevelSmooth.setTargetValue(peakDB);
    }
    outputLevelDB.store(outputLevelSmooth.skip(numSamples));
}

#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OBassAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OBassAudioProcessorEditor(*this);
#else
    // The param-dump console target builds with JUCE_WEB_BROWSER=0 and no
    // editor sources. It never opens an editor; this keeps the TU linkable.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

void OBassAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v1.4.0: the UI language rides the same tree as one more plain property.
    // Written BEFORE getStateAsXml(), because that method serialises
    // parameters.copyState() and would otherwise take a snapshot without it.
    // Written as a STRING ("en"/"fr") rather than the atomic's int index, so a
    // hand-inspected session file says what it means.
    parameters.state.setProperty("uiLanguage",
                                 languageCode(uiLanguage.load(std::memory_order_acquire)),
                                 nullptr);

    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OBassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        presetManager.setStateFromXml(xml.get());

    // v1.4.0: the UI language. Read AFTER setStateFromXml, which calls
    // parameters.replaceState() and therefore rebuilds the whole tree.
    //
    // isVoid() is the ONLY correct guard and toString() the only correct read.
    // getStateInformation writes a STRING var, but even a bool or int written
    // there would not survive: the XML round-trip does not preserve the type,
    // because NamedValueSet::setFromXmlAttributes rebuilds every property as
    // `var (value)` over the attribute STRING
    // (critical_valuetree_xml_roundtrip_loses_type). A pre-1.4.0 session has no
    // such property at all and the default (English) stands. languageIndex()
    // clamps anything that is not "fr" to 0, so a hand-edited value degrades to
    // English rather than to a bad index.
    //
    // The editor PULLS this through the getUiLanguage native fn at page init
    // rather than being pushed from here - a push would race the WebView's load.
    const juce::var lang = parameters.state.getProperty("uiLanguage");

    if (! lang.isVoid())
        uiLanguage.store(languageIndex(lang.toString()), std::memory_order_release);
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
