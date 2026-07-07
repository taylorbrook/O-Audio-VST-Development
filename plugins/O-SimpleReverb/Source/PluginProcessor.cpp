/*
  ==============================================================================

    O-SimpleReverb - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    v1.5.6 - Code-review fixes (CR-01..04, WR-01..05)

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
const OSimpleReverbAudioProcessor::TypePreset OSimpleReverbAudioProcessor::typePresets[6] = {
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

juce::AudioProcessorValueTreeState::ParameterLayout OSimpleReverbAudioProcessor::createParameterLayout()
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

    // DECAY - Float (0.5x to 2.0x multiplier)
    // CR-02: skew 0.6309 puts 1.0x at knob center. convertFrom0to1 maps
    // value = min + (max-min) * norm^(1/skew); solving 0.5 + 1.5*0.5^(1/s) = 1.0
    // gives s = log(0.5)/log(1/3) ≈ 0.6309. The previous 1.585 (the reciprocal)
    // put 1.47x at center, disagreeing with the UI readout and factory presets,
    // which were both authored against this intended curve.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DECAY", 1 },
        "Decay",
        juce::NormalisableRange<float>(0.5f, 2.0f, 0.01f, 0.6309f),
        1.0f  // Default: 1.0x (no change)
    ));

    // SIZE - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SIZE", 1 },
        "Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f  // Default: 50% (Medium)
    ));

    // LPFREQ - Lowpass filter cutoff frequency (20Hz to 400Hz)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LPFREQ", 1 },
        "LP Filter Freq",
        juce::NormalisableRange<float>(20.0f, 400.0f, 1.0f),
        200.0f  // Default: 200Hz
    ));

    // LPON - Lowpass filter on/off toggle (0 = off, 1 = on)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LPON", 1 },
        "LP Filter On",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f),
        0.0f  // Default: Off
    ));

    return layout;
}

OSimpleReverbAudioProcessor::OSimpleReverbAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-SimpleReverb")
{
    // Cache parameter pointers (these never change after construction)
    typeParam = parameters.getRawParameterValue("TYPE");
    characterParam = parameters.getRawParameterValue("CHARACTER");
    sizeParam = parameters.getRawParameterValue("SIZE");
    decayParam = parameters.getRawParameterValue("DECAY");
    wetParam = parameters.getRawParameterValue("WET");
    dryParam = parameters.getRawParameterValue("DRY");
    lpFreqParam = parameters.getRawParameterValue("LPFREQ");
    lpOnParam = parameters.getRawParameterValue("LPON");

    // Initialize factory presets (4 per reverb type = 24 total)
    initializeFactoryPresets();
    // Initialize early reflection delay lines
    for (auto& delay : earlyReflectionsL)
        delay.setMaximumDelayInSamples(kMaxEarlyReflectionSamples);
    for (auto& delay : earlyReflectionsR)
        delay.setMaximumDelayInSamples(kMaxEarlyReflectionSamples);

    // Initialize all-pass delay lines
    for (auto& delay : allPassL)
        delay.setMaximumDelayInSamples(kMaxAllPassSamples);
    for (auto& delay : allPassR)
        delay.setMaximumDelayInSamples(kMaxAllPassSamples);
}

OSimpleReverbAudioProcessor::~OSimpleReverbAudioProcessor() = default;

void OSimpleReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare main reverb
    reverb.prepare(spec);
    reverb.reset();

    // Prepare filters using template helper
    prepareFilterAsAllPass(characterFilter, spec, sampleRate);
    previousMode = CharacterMode::Neutral;

    prepareFilterAsAllPass(typeEqFilter, spec, sampleRate);

    // Prepare user high-pass (low cut) filter
    // (ArrayCoefficients assignment also primes the state's coefficient storage,
    //  so the audio-thread updates in processBlock never reallocate — CR-03)
    lpFilter.prepare(spec);
    lpFilter.reset();
    *lpFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(sampleRate, 200.0f);
    previousLPFreq = -1.0f;

    // Prepare pre-delay lines
    preDelayL.prepare(spec);
    preDelayR.prepare(spec);
    preDelayL.reset();
    preDelayR.reset();

    // Prepare delay line containers using template helper
    prepareDelayContainer(earlyReflectionsL, spec);
    prepareDelayContainer(earlyReflectionsR, spec);
    prepareDelayContainer(allPassL, spec);
    prepareDelayContainer(allPassR, spec);

    // Reset modulation
    lfoPhase = 0.0f;
    shimmerPhase = 0.0f;

    // CR-04: pre-allocate work buffers so the setSize() calls in processBlock
    // are no-op reuse instead of a guaranteed first-callback allocation
    dryBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    wetBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);

    // WR-03: 20ms wet/dry gain smoothing (zipper-noise-free knob drags/automation)
    wetGainSmoothed.reset(sampleRate, 0.02);
    dryGainSmoothed.reset(sampleRate, 0.02);
    wetGainSmoothed.setCurrentAndTargetValue(wetParam->load() / 100.0f);
    dryGainSmoothed.setCurrentAndTargetValue(dryParam->load() / 100.0f);

    // Force type update on first block
    previousType = -1;
}

void OSimpleReverbAudioProcessor::releaseResources()
{
}

void OSimpleReverbAudioProcessor::updateTypeSpecificDSP(int typeIndex)
{
    const auto& preset = typePresets[typeIndex];

    // Update LFO increment
    if (preset.modRate > 0.0f) {
        lfoIncrement = (2.0f * juce::MathConstants<float>::pi * preset.modRate) / static_cast<float>(currentSampleRate);
    } else {
        lfoIncrement = 0.0f;
    }

    // Set early reflection delay times based on type
    for (int i = 0; i < numEarlyReflections; ++i) {
        float delayMs = kBaseEarlyDelaysMs[i] * preset.earlyReflectionScale;
        float delaySamples = (delayMs / 1000.0f) * static_cast<float>(currentSampleRate);

        earlyReflectionsL[i].setDelay(delaySamples);
        earlyReflectionsR[i].setDelay(delaySamples * kEarlyReflectionStereoOffset);
    }

    // Set all-pass delay times (for Spring)
    for (int i = 0; i < numAllPassFilters; ++i) {
        float delaySamples = (allPassDelayMs[i] / 1000.0f) * static_cast<float>(currentSampleRate);
        allPassL[i].setDelay(delaySamples);
        allPassR[i].setDelay(delaySamples * kAllPassStereoOffset);
    }

    // Update type-specific EQ
    // CR-03: this runs on the audio thread (type change in processBlock), so use
    // ArrayCoefficients (stack std::array, identical math) instead of
    // Coefficients::makeXXX, which heap-allocates a ref-counted object.
    // Assignment reuses the state's existing storage (primed in prepareToPlay).
    switch (preset.eqType) {
        case TypePreset::EqType::None:
            *typeEqFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(currentSampleRate, 1000.0f);
            break;
        case TypePreset::EqType::LowShelf:
            *typeEqFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(
                currentSampleRate, preset.eqFreq, preset.eqQ,
                juce::Decibels::decibelsToGain(preset.eqGain));
            break;
        case TypePreset::EqType::HighShelf:
            *typeEqFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
                currentSampleRate, preset.eqFreq, preset.eqQ,
                juce::Decibels::decibelsToGain(preset.eqGain));
            break;
        case TypePreset::EqType::Peak:
            *typeEqFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
                currentSampleRate, preset.eqFreq, preset.eqQ,
                juce::Decibels::decibelsToGain(preset.eqGain));
            break;
        case TypePreset::EqType::HighPass:
            *typeEqFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(
                currentSampleRate, preset.eqFreq, preset.eqQ);
            break;
    }
}

float OSimpleReverbAudioProcessor::processAllPassChain(float input, bool isLeft)
{
    float output = input;
    auto& delays = isLeft ? allPassL : allPassR;

    for (int i = 0; i < numAllPassFilters; ++i) {
        float delayed = delays[i].popSample(0);
        float feedforward = output + (-allPassCoeff * delayed);
        delays[i].pushSample(0, output + (allPassCoeff * delayed));
        output = delayed + (allPassCoeff * feedforward);
    }

    return output;
}

void OSimpleReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    if (buffer.getNumSamples() == 0)
        return;

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (pointers cached in constructor)
    int typeValue = static_cast<int>(typeParam->load());
    float characterValue = characterParam->load();
    float sizeValue = sizeParam->load();
    float decayValue = decayParam->load();
    float wetValue = wetParam->load();
    float dryValue = dryParam->load();
    float lpFreqValue = lpFreqParam->load();
    bool lpFilterOn = lpOnParam->load() >= 0.5f;

    typeValue = juce::jlimit(0, 5, typeValue);

    // Get current preset
    const auto& preset = typePresets[typeValue];

    // Update type-specific DSP if type changed
    if (typeValue != previousType) {
        updateTypeSpecificDSP(typeValue);
        previousType = typeValue;
    }

    // Calculate reverb parameters
    // Size affects room size
    float sizeNorm = sizeValue / 100.0f;
    float finalRoomSize = preset.baseRoomSize * (0.5f + sizeNorm * 0.5f);

    // Decay multiplier (0.5x to 2.0x) scales room size and inversely scales damping
    // Higher decay = larger room + less damping = longer tail
    finalRoomSize *= decayValue;
    finalRoomSize = juce::jlimit(0.0f, 1.0f, finalRoomSize);

    // Damping: lower values = longer decay, so divide by decay multiplier
    float finalDamping = preset.baseDamping / decayValue;
    finalDamping = juce::jlimit(0.0f, 1.0f, finalDamping);

    // WR-03: smooth wet/dry gains (raw per-block atomic loads step at block rate
    // and zipper on sustained material). Ramp linearly across the block between
    // the smoother's start and end values — equivalent to per-sample smoothing
    // but keeps the channel-major mix loop.
    wetGainSmoothed.setTargetValue(wetValue / 100.0f);
    dryGainSmoothed.setTargetValue(dryValue / 100.0f);
    const float wetGainStart = wetGainSmoothed.getCurrentValue();
    const float dryGainStart = dryGainSmoothed.getCurrentValue();
    wetGainSmoothed.skip(buffer.getNumSamples());
    dryGainSmoothed.skip(buffer.getNumSamples());
    const float wetGainEnd = wetGainSmoothed.getCurrentValue();
    const float dryGainEnd = dryGainSmoothed.getCurrentValue();

    // Store dry signal (pre-allocated buffer, no reallocation)
    dryBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

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

    // Prepare wet buffer (using pre-allocated buffer, resize only if needed)
    wetBuffer.setSize(buffer.getNumChannels(), numSamples, false, false, true);
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

            // Apply subtle pitch modulation by varying gain (simpler than true pitch shift)
            float modGain = 1.0f + (lfoValue * kLfoAmplitudeModulation);
            processedL *= modGain;
            processedR *= (1.0f + (std::sin(lfoPhase + 0.5f) * kLfoAmplitudeModulation));  // Phase offset for stereo
        }

        // === 5. Plate Shimmer (if enabled) ===
        if (preset.useShimmer) {
            shimmerPhase += (2.0f * juce::MathConstants<float>::pi * shimmerFreq) / static_cast<float>(currentSampleRate);
            if (shimmerPhase >= 2.0f * juce::MathConstants<float>::pi)
                shimmerPhase -= 2.0f * juce::MathConstants<float>::pi;

            // Subtle ring modulation for shimmer effect
            float shimmerMod = std::sin(shimmerPhase);
            processedL += processedL * shimmerMod * kShimmerMixAmount;
            processedR += processedR * shimmerMod * kShimmerMixAmount;
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
        previousCharacterValue = characterValue - 1.0f; // Force coefficient update on mode change
    }

    if (currentMode == CharacterMode::Warm) {
        if (std::abs(characterValue - previousCharacterValue) > 0.1f) {
            float warmValue = (characterValue + 100.0f) / 99.0f;
            warmValue = juce::jlimit(0.0f, 1.0f, warmValue);
            float cutoffHz = 2000.0f + (18000.0f * warmValue);
            cutoffHz = juce::jlimit(2000.0f, 20000.0f, cutoffHz);
            *characterFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(currentSampleRate, cutoffHz);  // CR-03: RT-safe, no heap alloc
            previousCharacterValue = characterValue;
        }
        characterFilter.process(wetContext);
    }
    else if (currentMode == CharacterMode::Bright) {
        if (std::abs(characterValue - previousCharacterValue) > 0.1f) {
            float brightValue = characterValue / 100.0f;
            brightValue = juce::jlimit(0.0f, 1.0f, brightValue);
            float gainDb = brightValue * 6.0f;
            float gainLinear = juce::Decibels::decibelsToGain(gainDb);
            *characterFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(currentSampleRate, 4000.0f, 0.707f, gainLinear);  // CR-03: RT-safe, no heap alloc
            previousCharacterValue = characterValue;
        }
        characterFilter.process(wetContext);
    }

    // === 8.5. User High-Pass / Low Cut Filter (if enabled) ===
    if (lpFilterOn) {
        // Update filter coefficients if frequency changed
        if (std::abs(lpFreqValue - previousLPFreq) > 0.5f) {
            *lpFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(currentSampleRate, lpFreqValue);  // CR-03: RT-safe, no heap alloc
            previousLPFreq = lpFreqValue;
        }
        lpFilter.process(wetContext);
    }

    // === 9. Dry/Wet Mix (gain-ramped, WR-03) ===
    const float rampStep = numSamples > 0 ? 1.0f / static_cast<float>(numSamples) : 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* output = buffer.getWritePointer(channel);
        const float* dry = dryBuffer.getReadPointer(channel);
        const float* wet = wetBuffer.getReadPointer(channel);

        float t = 0.0f;
        for (int sample = 0; sample < numSamples; ++sample, t += rampStep) {
            const float dryGain = dryGainStart + (dryGainEnd - dryGainStart) * t;
            const float wetGain = wetGainStart + (wetGainEnd - wetGainStart) * t;
            output[sample] = (dry[sample] * dryGain) + (wet[sample] * wetGain);
        }
    }

    // === 10. VU Meter - Calculate peak level after all processing ===
    float peakLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float channelPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
        peakLevel = std::max(peakLevel, channelPeak);
    }
    float levelDB = peakLevel > 0.00001f
        ? juce::Decibels::gainToDecibels(peakLevel)
        : kVuMeterFloorDB;
    outputLevelDB.store(levelDB, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* OSimpleReverbAudioProcessor::createEditor()
{
    return new OSimpleReverbAudioProcessorEditor(*this);
}

void OSimpleReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OSimpleReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}

void OSimpleReverbAudioProcessor::initializeFactoryPresets()
{
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        // === BOOTH PRESETS (4) ===
        { "Booth - Vocal Booth", {
            {"TYPE", 0.0f}, {"CHARACTER", 0.5f}, {"WET", 0.20f}, {"DRY", 1.0f},
            {"DECAY", 0.33f}, {"SIZE", 0.30f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Booth - Drum Close", {
            {"TYPE", 0.0f}, {"CHARACTER", 0.35f}, {"WET", 0.15f}, {"DRY", 1.0f},
            {"DECAY", 0.20f}, {"SIZE", 0.20f}, {"LPFREQ", 0.26f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Booth - Tight Room", {
            {"TYPE", 0.0f}, {"CHARACTER", 0.50f}, {"WET", 0.25f}, {"DRY", 1.0f},
            {"DECAY", 0.40f}, {"SIZE", 0.40f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Booth - Whisper", {
            {"TYPE", 0.0f}, {"CHARACTER", 0.70f}, {"WET", 0.30f}, {"DRY", 1.0f},
            {"DECAY", 0.25f}, {"SIZE", 0.15f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },

        // === ROOM PRESETS (4) ===
        { "Room - Small Room", {
            {"TYPE", 0.167f}, {"CHARACTER", 0.50f}, {"WET", 0.25f}, {"DRY", 1.0f},
            {"DECAY", 0.33f}, {"SIZE", 0.35f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Room - Live Room", {
            {"TYPE", 0.167f}, {"CHARACTER", 0.45f}, {"WET", 0.35f}, {"DRY", 1.0f},
            {"DECAY", 0.50f}, {"SIZE", 0.55f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Room - Studio A", {
            {"TYPE", 0.167f}, {"CHARACTER", 0.55f}, {"WET", 0.30f}, {"DRY", 1.0f},
            {"DECAY", 0.45f}, {"SIZE", 0.50f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Room - Jazz Club", {
            {"TYPE", 0.167f}, {"CHARACTER", 0.40f}, {"WET", 0.40f}, {"DRY", 1.0f},
            {"DECAY", 0.55f}, {"SIZE", 0.60f}, {"LPFREQ", 0.26f}, {"LPON", 1.0f}
        }, juce::var() },

        // === HALL PRESETS (4) ===
        { "Hall - Concert Hall", {
            {"TYPE", 0.333f}, {"CHARACTER", 0.50f}, {"WET", 0.35f}, {"DRY", 1.0f},
            {"DECAY", 0.60f}, {"SIZE", 0.75f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Hall - Cathedral", {
            {"TYPE", 0.333f}, {"CHARACTER", 0.45f}, {"WET", 0.45f}, {"DRY", 0.85f},
            {"DECAY", 0.80f}, {"SIZE", 0.90f}, {"LPFREQ", 0.21f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Hall - Theater", {
            {"TYPE", 0.333f}, {"CHARACTER", 0.55f}, {"WET", 0.30f}, {"DRY", 1.0f},
            {"DECAY", 0.55f}, {"SIZE", 0.65f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Hall - Ballroom", {
            {"TYPE", 0.333f}, {"CHARACTER", 0.60f}, {"WET", 0.40f}, {"DRY", 1.0f},
            {"DECAY", 0.70f}, {"SIZE", 0.80f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },

        // === SPRING PRESETS (4) ===
        { "Spring - Vintage Spring", {
            {"TYPE", 0.50f}, {"CHARACTER", 0.45f}, {"WET", 0.35f}, {"DRY", 1.0f},
            {"DECAY", 0.50f}, {"SIZE", 0.50f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Spring - Surf Guitar", {
            {"TYPE", 0.50f}, {"CHARACTER", 0.60f}, {"WET", 0.45f}, {"DRY", 1.0f},
            {"DECAY", 0.55f}, {"SIZE", 0.55f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Spring - Dub Echo", {
            {"TYPE", 0.50f}, {"CHARACTER", 0.35f}, {"WET", 0.50f}, {"DRY", 0.90f},
            {"DECAY", 0.65f}, {"SIZE", 0.60f}, {"LPFREQ", 0.32f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Spring - Twang", {
            {"TYPE", 0.50f}, {"CHARACTER", 0.70f}, {"WET", 0.40f}, {"DRY", 1.0f},
            {"DECAY", 0.45f}, {"SIZE", 0.45f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },

        // === PLATE PRESETS (4) ===
        { "Plate - Studio Plate", {
            {"TYPE", 0.667f}, {"CHARACTER", 0.55f}, {"WET", 0.30f}, {"DRY", 1.0f},
            {"DECAY", 0.50f}, {"SIZE", 0.55f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Plate - Shimmer Plate", {
            {"TYPE", 0.667f}, {"CHARACTER", 0.70f}, {"WET", 0.40f}, {"DRY", 1.0f},
            {"DECAY", 0.65f}, {"SIZE", 0.70f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Plate - Vocal Plate", {
            {"TYPE", 0.667f}, {"CHARACTER", 0.50f}, {"WET", 0.25f}, {"DRY", 1.0f},
            {"DECAY", 0.45f}, {"SIZE", 0.50f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Plate - Lush Plate", {
            {"TYPE", 0.667f}, {"CHARACTER", 0.45f}, {"WET", 0.45f}, {"DRY", 0.95f},
            {"DECAY", 0.70f}, {"SIZE", 0.75f}, {"LPFREQ", 0.26f}, {"LPON", 1.0f}
        }, juce::var() },

        // === AMBIENT PRESETS (4) ===
        { "Ambient - Pad Wash", {
            {"TYPE", 0.833f}, {"CHARACTER", 0.40f}, {"WET", 0.50f}, {"DRY", 0.80f},
            {"DECAY", 0.75f}, {"SIZE", 0.85f}, {"LPFREQ", 0.26f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Ambient - Infinite Drone", {
            {"TYPE", 0.833f}, {"CHARACTER", 0.35f}, {"WET", 0.60f}, {"DRY", 0.60f},
            {"DECAY", 1.0f}, {"SIZE", 1.0f}, {"LPFREQ", 0.21f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Ambient - Ethereal", {
            {"TYPE", 0.833f}, {"CHARACTER", 0.55f}, {"WET", 0.55f}, {"DRY", 0.75f},
            {"DECAY", 0.80f}, {"SIZE", 0.90f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Ambient - Cloud Nine", {
            {"TYPE", 0.833f}, {"CHARACTER", 0.50f}, {"WET", 0.65f}, {"DRY", 0.70f},
            {"DECAY", 0.85f}, {"SIZE", 0.95f}, {"LPFREQ", 0.32f}, {"LPON", 1.0f}
        }, juce::var() }
    };

    presetManager.initializeFactoryPresets(factoryPresets);
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleReverbAudioProcessor();
}
