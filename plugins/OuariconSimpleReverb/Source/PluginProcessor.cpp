/*
  ==============================================================================

    OuariconSimpleReverb - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    v1.1.0 - Type-specific DSP for meaningful reverb differentiation

    Each reverb type now has distinct sonic character:
    - Booth: Tight, immediate, minimal reflections
    - Room: Natural early reflections, balanced character
    - Hall: Long pre-delay, spacious early reflections
    - Spring: Chirpy dispersion, pitch modulation (flutter)
    - Plate: Dense diffusion, bright shimmer
    - Ambient: Maximum diffusion, slow modulation, washy tail

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Define the type presets with distinct DSP characteristics
const OuariconSimpleReverbAudioProcessor::TypePreset OuariconSimpleReverbAudioProcessor::typePresets[6] = {
    // 0: Booth - tight, intimate, minimal reflections
    {
        0.15f,      // baseRoomSize
        0.85f,      // baseDamping (high = fast decay)
        0.6f,       // width (narrow)
        3.0f,       // preDelayMs (very short)
        0.3f,       // earlyReflectionScale (tight)
        0.2f,       // earlyReflectionMix (minimal)
        0.0f,       // modRate (no modulation)
        0.0f,       // modDepth
        false,      // useAllPass
        false,      // useShimmer
        150.0f,     // eqFreq (high-pass to remove rumble)
        0.0f,       // eqGain
        0.707f,     // eqQ
        TypePreset::EqType::HighPass
    },
    // 1: Room - natural, versatile
    {
        0.50f,      // baseRoomSize
        0.50f,      // baseDamping
        1.0f,       // width (full stereo)
        15.0f,      // preDelayMs (natural room)
        1.0f,       // earlyReflectionScale (natural)
        0.4f,       // earlyReflectionMix
        0.0f,       // modRate
        0.0f,       // modDepth
        false,      // useAllPass
        false,      // useShimmer
        0.0f,       // eqFreq (no EQ)
        0.0f,       // eqGain
        0.707f,     // eqQ
        TypePreset::EqType::None
    },
    // 2: Hall - large concert hall, spacious
    {
        0.85f,      // baseRoomSize (large)
        0.25f,      // baseDamping (low = long decay)
        1.0f,       // width
        50.0f,      // preDelayMs (long for large space)
        2.0f,       // earlyReflectionScale (spread out)
        0.5f,       // earlyReflectionMix
        0.15f,      // modRate (very subtle movement)
        0.3f,       // modDepth
        false,      // useAllPass
        false,      // useShimmer
        3000.0f,    // eqFreq (gentle roll-off)
        -2.0f,      // eqGain (slight high cut for distance)
        0.5f,       // eqQ
        TypePreset::EqType::HighShelf
    },
    // 3: Spring - metallic chirp, flutter
    {
        0.35f,      // baseRoomSize
        0.40f,      // baseDamping
        0.7f,       // width (narrower)
        20.0f,      // preDelayMs
        0.5f,       // earlyReflectionScale
        0.15f,      // earlyReflectionMix (less - spring character dominates)
        4.5f,       // modRate (flutter speed)
        1.2f,       // modDepth (noticeable wobble)
        true,       // useAllPass (spring dispersion!)
        false,      // useShimmer
        800.0f,     // eqFreq (resonant mid boost)
        4.0f,       // eqGain (metallic resonance)
        2.5f,       // eqQ (narrow resonance)
        TypePreset::EqType::Peak
    },
    // 4: Plate - dense, bright, shimmering
    {
        0.65f,      // baseRoomSize
        0.30f,      // baseDamping
        1.0f,       // width (full stereo)
        8.0f,       // preDelayMs (short for density)
        0.6f,       // earlyReflectionScale
        0.6f,       // earlyReflectionMix (dense early reflections)
        0.0f,       // modRate
        0.0f,       // modDepth
        false,      // useAllPass
        true,       // useShimmer (plate shimmer!)
        5000.0f,    // eqFreq (bright shelf)
        3.0f,       // eqGain (add sparkle)
        0.707f,     // eqQ
        TypePreset::EqType::HighShelf
    },
    // 5: Ambient - washy, ethereal, infinite
    {
        0.95f,      // baseRoomSize (maximum)
        0.10f,      // baseDamping (very low = infinite)
        1.0f,       // width
        35.0f,      // preDelayMs
        2.5f,       // earlyReflectionScale (very spread)
        0.3f,       // earlyReflectionMix
        0.4f,       // modRate (slow, dreamy movement)
        2.0f,       // modDepth (deep modulation)
        false,      // useAllPass
        false,      // useShimmer
        2500.0f,    // eqFreq
        -3.0f,      // eqGain (soften highs for washy sound)
        0.5f,       // eqQ
        TypePreset::EqType::HighShelf
    }
};

juce::AudioProcessorValueTreeState::ParameterLayout OuariconSimpleReverbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // TYPE - Choice parameter (6 options: Booth, Room, Hall, Spring, Plate, Ambient)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "TYPE", 1 },
        "Type",
        juce::StringArray { "Booth", "Room", "Hall", "Spring", "Plate", "Ambient" },
        1  // Default: Room (index 1)
    ));

    // CHARACTER - Bipolar float (-100% to +100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CHARACTER", 1 },
        "Character",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f  // Default: Neutral
    ));

    // WET - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "WET", 1 },
        "Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f  // Default: 25%
    ));

    // DRY - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DRY", 1 },
        "Dry",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f  // Default: 100%
    ));

    // DECAY - Float (0.1s to 10.0s, logarithmic skew)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DECAY", 1 },
        "Decay",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f),  // Skew 0.5 for logarithmic
        1.5f  // Default: 1.5s
    ));

    // SIZE - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SIZE", 1 },
        "Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f  // Default: 50% (Medium)
    ));

    return layout;
}

OuariconSimpleReverbAudioProcessor::OuariconSimpleReverbAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize early reflection delay lines with default max size
    for (auto& delay : earlyReflectionsL)
        delay.setMaximumDelayInSamples(9600);  // ~50ms at 192kHz
    for (auto& delay : earlyReflectionsR)
        delay.setMaximumDelayInSamples(9600);

    // Initialize all-pass delay lines
    for (auto& delay : allPassL)
        delay.setMaximumDelayInSamples(960);   // ~5ms at 192kHz
    for (auto& delay : allPassR)
        delay.setMaximumDelayInSamples(960);
}

OuariconSimpleReverbAudioProcessor::~OuariconSimpleReverbAudioProcessor()
{
}

void OuariconSimpleReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare main reverb
    reverb.prepare(spec);
    reverb.reset();

    // Prepare character filter
    characterFilter.prepare(spec);
    characterFilter.reset();
    *characterFilter.state = *juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, 1000.0f);
    previousMode = CharacterMode::Neutral;

    // Prepare type-specific EQ filter
    typeEqFilter.prepare(spec);
    typeEqFilter.reset();
    *typeEqFilter.state = *juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, 1000.0f);

    // Prepare pre-delay lines
    preDelayL.prepare(spec);
    preDelayR.prepare(spec);
    preDelayL.reset();
    preDelayR.reset();

    // Prepare early reflection delay lines
    for (auto& delay : earlyReflectionsL) {
        delay.prepare(spec);
        delay.reset();
    }
    for (auto& delay : earlyReflectionsR) {
        delay.prepare(spec);
        delay.reset();
    }

    // Prepare all-pass filters
    for (auto& delay : allPassL) {
        delay.prepare(spec);
        delay.reset();
    }
    for (auto& delay : allPassR) {
        delay.prepare(spec);
        delay.reset();
    }

    // Reset all-pass state
    for (int i = 0; i < numAllPassFilters; ++i) {
        allPassStateL[i] = 0.0f;
        allPassStateR[i] = 0.0f;
    }

    // Reset modulation
    lfoPhase = 0.0f;
    shimmerPhase = 0.0f;
    shimmerFreq = 1500.0f;  // ~1.5kHz for subtle shimmer

    // Force type update on first block
    previousType = -1;
}

void OuariconSimpleReverbAudioProcessor::releaseResources()
{
}

void OuariconSimpleReverbAudioProcessor::updateTypeSpecificDSP(int typeIndex)
{
    const auto& preset = typePresets[typeIndex];

    // Update LFO increment
    if (preset.modRate > 0.0f) {
        lfoIncrement = (2.0f * juce::MathConstants<float>::pi * preset.modRate) / static_cast<float>(currentSampleRate);
    } else {
        lfoIncrement = 0.0f;
    }

    // Set early reflection delay times based on type
    // Base times in ms: 7, 11, 17, 23 (prime numbers for less metallic sound)
    const float baseDelaysMs[4] = { 7.0f, 11.0f, 17.0f, 23.0f };

    for (int i = 0; i < numEarlyReflections; ++i) {
        float delayMs = baseDelaysMs[i] * preset.earlyReflectionScale;
        float delaySamples = (delayMs / 1000.0f) * static_cast<float>(currentSampleRate);

        // Slight L/R offset for width
        earlyReflectionsL[i].setDelay(delaySamples);
        earlyReflectionsR[i].setDelay(delaySamples * 1.07f);  // 7% offset for stereo
    }

    // Set all-pass delay times (for Spring)
    for (int i = 0; i < numAllPassFilters; ++i) {
        float delaySamples = (allPassDelayMs[i] / 1000.0f) * static_cast<float>(currentSampleRate);
        allPassL[i].setDelay(delaySamples);
        allPassR[i].setDelay(delaySamples * 1.05f);  // Slight offset
    }

    // Update type-specific EQ
    switch (preset.eqType) {
        case TypePreset::EqType::None:
            *typeEqFilter.state = *juce::dsp::IIR::Coefficients<float>::makeAllPass(currentSampleRate, 1000.0f);
            break;
        case TypePreset::EqType::LowShelf:
            *typeEqFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                currentSampleRate, preset.eqFreq, preset.eqQ,
                juce::Decibels::decibelsToGain(preset.eqGain));
            break;
        case TypePreset::EqType::HighShelf:
            *typeEqFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                currentSampleRate, preset.eqFreq, preset.eqQ,
                juce::Decibels::decibelsToGain(preset.eqGain));
            break;
        case TypePreset::EqType::Peak:
            *typeEqFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                currentSampleRate, preset.eqFreq, preset.eqQ,
                juce::Decibels::decibelsToGain(preset.eqGain));
            break;
        case TypePreset::EqType::HighPass:
            *typeEqFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                currentSampleRate, preset.eqFreq, preset.eqQ);
            break;
    }
}

float OuariconSimpleReverbAudioProcessor::processAllPassChain(float input, bool isLeft)
{
    float output = input;
    auto& states = isLeft ? allPassStateL : allPassStateR;
    auto& delays = isLeft ? allPassL : allPassR;

    for (int i = 0; i < numAllPassFilters; ++i) {
        float delayed = delays[i].popSample(0);
        float feedforward = output + (-allPassCoeff * delayed);
        delays[i].pushSample(0, output + (allPassCoeff * delayed));
        output = delayed + (allPassCoeff * feedforward);
    }

    return output;
}

void OuariconSimpleReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    if (buffer.getNumSamples() == 0)
        return;

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters
    auto* typeParam = parameters.getRawParameterValue("TYPE");
    auto* characterParam = parameters.getRawParameterValue("CHARACTER");
    auto* sizeParam = parameters.getRawParameterValue("SIZE");
    auto* decayParam = parameters.getRawParameterValue("DECAY");
    auto* wetParam = parameters.getRawParameterValue("WET");
    auto* dryParam = parameters.getRawParameterValue("DRY");

    int typeValue = static_cast<int>(typeParam->load());
    float characterValue = characterParam->load();
    float sizeValue = sizeParam->load();
    float decayValue = decayParam->load();
    float wetValue = wetParam->load();
    float dryValue = dryParam->load();

    typeValue = juce::jlimit(0, 5, typeValue);

    // Get current preset
    const auto& preset = typePresets[typeValue];

    // Update type-specific DSP if type changed
    if (typeValue != previousType) {
        updateTypeSpecificDSP(typeValue);
        previousType = typeValue;
    }

    // Calculate reverb parameters
    float sizeNorm = sizeValue / 100.0f;
    float finalRoomSize = preset.baseRoomSize * (0.5f + sizeNorm * 0.5f);
    finalRoomSize = juce::jlimit(0.0f, 1.0f, finalRoomSize);

    float finalDamping = preset.baseDamping * (1.0f - (decayValue / 10.0f) * 0.8f);
    finalDamping = juce::jlimit(0.0f, 1.0f, finalDamping);

    float wetGain = wetValue / 100.0f;
    float dryGain = dryValue / 100.0f;

    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Process sample-by-sample for type-specific DSP
    const int numSamples = buffer.getNumSamples();
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : leftChannel;

    const float* dryLeft = dryBuffer.getReadPointer(0);
    const float* dryRight = dryBuffer.getNumChannels() > 1 ? dryBuffer.getReadPointer(1) : dryLeft;

    // Pre-delay in samples
    float preDelaySamples = (preset.preDelayMs / 1000.0f) * static_cast<float>(currentSampleRate);
    preDelayL.setDelay(preDelaySamples);
    preDelayR.setDelay(preDelaySamples);

    // Create wet buffer for processing
    juce::AudioBuffer<float> wetBuffer(buffer.getNumChannels(), numSamples);
    wetBuffer.clear();

    for (int sample = 0; sample < numSamples; ++sample) {
        float inputL = leftChannel[sample];
        float inputR = rightChannel[sample];

        // === 1. Pre-delay ===
        preDelayL.pushSample(0, inputL);
        preDelayR.pushSample(0, inputR);
        float preDelayedL = preDelayL.popSample(0);
        float preDelayedR = preDelayR.popSample(0);

        // === 2. Early Reflections ===
        float earlyL = 0.0f;
        float earlyR = 0.0f;

        for (int i = 0; i < numEarlyReflections; ++i) {
            earlyReflectionsL[i].pushSample(0, preDelayedL);
            earlyReflectionsR[i].pushSample(0, preDelayedR);
            earlyL += earlyReflectionsL[i].popSample(0) * earlyReflectionGains[i];
            earlyR += earlyReflectionsR[i].popSample(0) * earlyReflectionGains[i];
        }

        // Mix early reflections
        float processedL = preDelayedL + (earlyL * preset.earlyReflectionMix);
        float processedR = preDelayedR + (earlyR * preset.earlyReflectionMix);

        // === 3. Spring All-Pass Dispersion (if enabled) ===
        if (preset.useAllPass) {
            processedL = processAllPassChain(processedL, true);
            processedR = processAllPassChain(processedR, false);
        }

        // === 4. Modulation (if enabled) ===
        if (preset.modRate > 0.0f) {
            lfoPhase += lfoIncrement;
            if (lfoPhase >= 2.0f * juce::MathConstants<float>::pi)
                lfoPhase -= 2.0f * juce::MathConstants<float>::pi;

            float lfoValue = std::sin(lfoPhase);
            float modDelaySamples = (preset.modDepth / 1000.0f) * static_cast<float>(currentSampleRate);

            // Apply subtle pitch modulation by varying gain (simpler than true pitch shift)
            float modGain = 1.0f + (lfoValue * 0.03f);  // ±3% amplitude modulation
            processedL *= modGain;
            processedR *= (1.0f + (std::sin(lfoPhase + 0.5f) * 0.03f));  // Phase offset for stereo
        }

        // === 5. Plate Shimmer (if enabled) ===
        if (preset.useShimmer) {
            shimmerPhase += (2.0f * juce::MathConstants<float>::pi * shimmerFreq) / static_cast<float>(currentSampleRate);
            if (shimmerPhase >= 2.0f * juce::MathConstants<float>::pi)
                shimmerPhase -= 2.0f * juce::MathConstants<float>::pi;

            // Subtle ring modulation for shimmer effect
            float shimmerMod = std::sin(shimmerPhase);
            float shimmerAmount = 0.08f;  // 8% shimmer mix
            processedL += processedL * shimmerMod * shimmerAmount;
            processedR += processedR * shimmerMod * shimmerAmount;
        }

        // Write to wet buffer for reverb processing
        wetBuffer.setSample(0, sample, processedL);
        if (wetBuffer.getNumChannels() > 1)
            wetBuffer.setSample(1, sample, processedR);
    }

    // === 6. Main Reverb Processing ===
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = finalRoomSize;
    reverbParams.damping = finalDamping;
    reverbParams.width = preset.width;
    reverbParams.wetLevel = 1.0f;   // Full wet (we mix manually)
    reverbParams.dryLevel = 0.0f;   // No dry (we add it back)
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);
    reverb.process(wetContext);

    // === 7. Type-Specific EQ ===
    if (preset.eqType != TypePreset::EqType::None) {
        typeEqFilter.process(wetContext);
    }

    // === 8. Character Control ===
    CharacterMode currentMode;
    if (characterValue < -0.5f)
        currentMode = CharacterMode::Warm;
    else if (characterValue > 0.5f)
        currentMode = CharacterMode::Bright;
    else
        currentMode = CharacterMode::Neutral;

    if (currentMode != previousMode) {
        characterFilter.reset();
        previousMode = currentMode;
    }

    if (currentMode == CharacterMode::Warm) {
        float warmValue = (characterValue + 100.0f) / 99.0f;
        warmValue = juce::jlimit(0.0f, 1.0f, warmValue);
        float cutoffHz = 2000.0f + (18000.0f * warmValue);
        cutoffHz = juce::jlimit(2000.0f, 20000.0f, cutoffHz);
        *characterFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, cutoffHz);
        characterFilter.process(wetContext);
    }
    else if (currentMode == CharacterMode::Bright) {
        float brightValue = characterValue / 100.0f;
        brightValue = juce::jlimit(0.0f, 1.0f, brightValue);
        float gainDb = brightValue * 6.0f;
        float gainLinear = juce::Decibels::decibelsToGain(gainDb);
        *characterFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 4000.0f, 0.707f, gainLinear);
        characterFilter.process(wetContext);
    }

    // === 9. Dry/Wet Mix ===
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* output = buffer.getWritePointer(channel);
        const float* dry = dryBuffer.getReadPointer(channel);
        const float* wet = wetBuffer.getReadPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            output[sample] = (dry[sample] * dryGain) + (wet[sample] * wetGain);
        }
    }
}

juce::AudioProcessorEditor* OuariconSimpleReverbAudioProcessor::createEditor()
{
    return new OuariconSimpleReverbAudioProcessorEditor(*this);
}

void OuariconSimpleReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconSimpleReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconSimpleReverbAudioProcessor();
}
