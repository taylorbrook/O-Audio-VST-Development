#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout FlutterVerbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // SIZE - Room dimensions
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SIZE", 1 },
        "Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        50.0f,
        "%"
    ));

    // DECAY - Reverb tail length
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DECAY", 1 },
        "Decay",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 1.0f),
        2.5f,
        "s"
    ));

    // MIX - Dry/wet blend
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        25.0f,
        "%"
    ));

    // AGE - Tape character intensity
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "AGE", 1 },
        "Age",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        20.0f,
        "%"
    ));

    // DRIVE - Tape saturation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DRIVE", 1 },
        "Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        20.0f,
        "%"
    ));

    // TONE - DJ-style filter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TONE", 1 },
        "Tone",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        ""
    ));

    // MOD_MODE - Modulation routing
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "MOD_MODE", 1 },
        "Mod Mode",
        false  // Default: WET ONLY (0)
    ));

    return layout;
}

FlutterVerbAudioProcessor::FlutterVerbAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

FlutterVerbAudioProcessor::~FlutterVerbAudioProcessor()
{
}

void FlutterVerbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store sample rate for LFO calculations
    currentSampleRate = sampleRate;

    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Phase 4.1: Prepare core reverb processing components
    dryWetMixer.prepare(spec);
    dryWetMixer.reset();

    // Fix 2: Set wet path latency compensation for modulation delay (50ms base delay)
    float baseDelayMs = 50.0f;
    int latencySamples = static_cast<int>((baseDelayMs / 1000.0f) * sampleRate);
    dryWetMixer.setWetLatency(latencySamples);

    // Prepare reverb with ProcessSpec
    reverb.prepare(spec);
    reverb.reset();

    // Phase 4.2: Prepare modulation system
    modulationDelay.prepare(spec);
    modulationDelay.reset();
    modulationDelay.setMaximumDelayInSamples(static_cast<int>(sampleRate * 0.2)); // 200ms max

    // Initialize per-channel LFO phase tracking
    wowPhase.resize(spec.numChannels, 0.0f);
    flutterPhase.resize(spec.numChannels, 0.0f);

    // Phase 4.3: Prepare filter
    toneFilter.prepare(spec);
    toneFilter.reset();
    currentFilterType = FilterType::None;
}

void FlutterVerbAudioProcessor::releaseResources()
{
    // DSP cleanup will be added in Stage 4
}

void FlutterVerbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Phase 4.1: Read SIZE, DECAY, MIX parameters (atomic, real-time safe)
    auto* sizeParam = parameters.getRawParameterValue("SIZE");
    auto* decayParam = parameters.getRawParameterValue("DECAY");
    auto* mixParam = parameters.getRawParameterValue("MIX");

    float sizeValue = sizeParam->load() / 100.0f;  // 0-100% → 0.0-1.0
    float decayValue = decayParam->load();          // 0.1-10.0 seconds
    float mixValue = mixParam->load() / 100.0f;     // 0-100% → 0.0-1.0

    // Phase 4.2: Read AGE parameter for modulation depth
    auto* ageParam = parameters.getRawParameterValue("AGE");
    float ageValue = ageParam->load() / 100.0f;  // 0-100% → 0.0-1.0

    // Phase 4.3: Read DRIVE and TONE parameters
    auto* driveParam = parameters.getRawParameterValue("DRIVE");
    auto* toneParam = parameters.getRawParameterValue("TONE");
    float driveValue = driveParam->load() / 100.0f;  // 0-100% → 0.0-1.0
    float toneValue = toneParam->load();  // -100 to +100

    // Phase 4.4: Read MOD_MODE parameter for routing control
    auto* modModeParam = parameters.getRawParameterValue("MOD_MODE");
    bool wetDryMode = modModeParam->load() > 0.5f;  // 0=WET_ONLY, 1=WET_DRY

    // Configure reverb parameters with true SIZE/DECAY independence
    juce::Reverb::Parameters reverbParams;

    // Map SIZE to base room size: 0-100% → 0.2-0.6 (ensures minimum space for reverb)
    float baseRoomSize = 0.2f + (sizeValue * 0.4f);

    // Map DECAY to multiplier: 0.1-10s → 0.5-2.0x (scales effective room size for tail length)
    float decayMultiplier = juce::jmap(decayValue, 0.1f, 10.0f, 0.5f, 2.0f);

    // Combine: roomSize affected by both SIZE and DECAY, clamped to valid range
    reverbParams.roomSize = juce::jlimit(0.1f, 1.0f, baseRoomSize * decayMultiplier);

    // Keep damping for frequency-dependent absorption (complementary control)
    reverbParams.damping = juce::jmap(decayValue, 0.1f, 10.0f, 0.95f, 0.05f);

    reverbParams.width = 1.0f;         // Full stereo
    reverbParams.freezeMode = 0.0f;    // No freeze
    reverbParams.wetLevel = 1.0f;      // Full wet (mixer handles blend)
    reverbParams.dryLevel = 0.0f;      // No dry (mixer handles blend)

    reverb.setParameters(reverbParams);

    // Set dry/wet mix proportion
    dryWetMixer.setWetMixProportion(mixValue);

    // Process audio with DSP pipeline
    juce::dsp::AudioBlock<float> block(buffer);

    // Phase 4.4: Define modulation processing function (reusable for both routing modes)
    auto applyModulation = [&]() {
        if (ageValue > 0.0f)  // Only apply modulation if AGE > 0
        {
            const int numChannels = buffer.getNumChannels();
            const int numSamples = buffer.getNumSamples();

            // LFO configuration
            const float wowFreqHz = 1.0f;      // Center frequency: 1Hz (range 0.5-1.5Hz)
            const float flutterFreqHz = 6.0f;  // Center frequency: 6Hz (range 4-8Hz)
            const float baseDelayMs = 50.0f;   // Base delay: 50ms
            const float maxModDepth = 0.2f;    // ±20% at AGE=100%

            // Calculate phase increments (radians per sample)
            const float wowPhaseInc = (wowFreqHz * 2.0f * juce::MathConstants<float>::pi) / static_cast<float>(currentSampleRate);
            const float flutterPhaseInc = (flutterFreqHz * 2.0f * juce::MathConstants<float>::pi) / static_cast<float>(currentSampleRate);

            // Process each channel
            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* channelData = buffer.getWritePointer(channel);

                for (int sample = 0; sample < numSamples; ++sample)
                {
                    // Calculate wow LFO output (sine wave)
                    float wowOutput = std::sin(wowPhase[channel]);

                    // Calculate flutter LFO output (sine wave)
                    float flutterOutput = std::sin(flutterPhase[channel]);

                    // Combine modulation signals (both contribute to pitch variation)
                    float totalModulation = (wowOutput + flutterOutput) * 0.5f;  // Average to keep in ±1.0 range

                    // Fix 3: Scale by AGE parameter with exponential curve for more usable range
                    // Exponential scaling gives more control in 0-50% range, still reaches extremes at 100%
                    float scaledAge = ageValue * ageValue;  // Exponential response
                    totalModulation *= scaledAge;

                    // Calculate modulated delay time in samples
                    float baseDelaySamples = (baseDelayMs / 1000.0f) * static_cast<float>(currentSampleRate);
                    float modulationAmount = baseDelaySamples * maxModDepth * totalModulation;  // ±20% depth
                    float delayTimeSamples = baseDelaySamples + modulationAmount;

                    // Ensure delay time is within valid range
                    delayTimeSamples = juce::jlimit(1.0f, static_cast<float>(currentSampleRate * 0.2), delayTimeSamples);

                    // Set delay time for this channel
                    modulationDelay.setDelay(static_cast<float>(delayTimeSamples));

                    // Process sample through delay line
                    modulationDelay.pushSample(channel, channelData[sample]);
                    channelData[sample] = modulationDelay.popSample(channel);

                    // Update LFO phases with wrapping
                    wowPhase[channel] += wowPhaseInc;
                    if (wowPhase[channel] >= 2.0f * juce::MathConstants<float>::pi)
                        wowPhase[channel] -= 2.0f * juce::MathConstants<float>::pi;

                    flutterPhase[channel] += flutterPhaseInc;
                    if (flutterPhase[channel] >= 2.0f * juce::MathConstants<float>::pi)
                        flutterPhase[channel] -= 2.0f * juce::MathConstants<float>::pi;
                }
            }
        }
    };

    // Define DRIVE processing lambda for reusability
    auto applyDrive = [&]() {
        if (driveValue > 0.0f)  // Only apply if DRIVE > 0
        {
            // Calculate gain: 1.0 at DRIVE=0%, 10.0 at DRIVE=100%
            float gain = 1.0f + (driveValue * 9.0f);

            const int numChannels = buffer.getNumChannels();
            const int numSamples = buffer.getNumSamples();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* channelData = buffer.getWritePointer(channel);

                for (int sample = 0; sample < numSamples; ++sample)
                {
                    // Apply tanh saturation
                    channelData[sample] = std::tanh(gain * channelData[sample]);
                }
            }
        }
    };

    // Define TONE filter lambda for reusability
    auto applyToneFilter = [&]() {
        if (std::abs(toneValue) > 0.5f)  // Bypass zone: |TONE| <= 0.5%
        {
            float sampleRate = static_cast<float>(currentSampleRate);
            bool isLowPass = (toneValue < 0.0f);

            // Determine filter type and reset state if type changed
            FilterType newFilterType = isLowPass ? FilterType::LowPass : FilterType::HighPass;
            if (newFilterType != currentFilterType)
            {
                toneFilter.reset();  // Prevent burst artifacts on type transition
                currentFilterType = newFilterType;
            }

            if (isLowPass)
            {
                // Low-pass filter (negative values: -100% to 0%)
                // Exponential mapping: 200Hz (extreme) to 20kHz (center)
                float normalizedValue = std::abs(toneValue) / 100.0f;  // 0.0 to 1.0
                float cutoffHz = 20000.0f * std::pow(10.0f, -normalizedValue * std::log10(100.0f));
                cutoffHz = juce::jlimit(200.0f, 20000.0f, cutoffHz);

                *toneFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                    sampleRate, cutoffHz, 0.707f  // Q = 0.707 (Butterworth)
                );
            }
            else
            {
                // High-pass filter (positive values: 0% to +100%)
                // Exponential mapping: 20Hz (center) to 10kHz (extreme)
                float normalizedValue = toneValue / 100.0f;  // 0.0 to 1.0
                float cutoffHz = 20.0f * std::pow(10.0f, normalizedValue * std::log10(500.0f));
                cutoffHz = juce::jlimit(20.0f, 10000.0f, cutoffHz);

                *toneFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                    sampleRate, cutoffHz, 0.707f  // Q = 0.707 (Butterworth)
                );
            }

            // Process buffer through filter
            juce::dsp::AudioBlock<float> filterBlock(buffer);
            juce::dsp::ProcessContextReplacing<float> filterContext(filterBlock);
            toneFilter.process(filterContext);
        }
        else
        {
            // Reset filter state when entering bypass zone
            if (currentFilterType != FilterType::None)
            {
                toneFilter.reset();
                currentFilterType = FilterType::None;
            }
        }
    };

    // Phase 4.4: MOD_MODE Routing with correct DRIVE/TONE positioning
    if (wetDryMode)
    {
        // Mode 1 (WET+DRY): Apply effects BEFORE split (affects both paths)
        applyModulation();  // Modulation first
        applyDrive();       // Then saturation
        applyToneFilter();  // Then filter - all affect both dry and wet
    }

    // Push dry samples (processed in Mode 1, clean in Mode 0)
    dryWetMixer.pushDrySamples(block);

    // Process reverb using modern DSP API
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);

    if (!wetDryMode)
    {
        // Mode 0 (WET ONLY): Apply effects AFTER reverb (affects only wet path)
        applyModulation();  // Modulation on wet only
        applyDrive();       // Saturation on wet only
        applyToneFilter();  // Filter on wet only
    }

    // Mix dry and wet samples
    dryWetMixer.mixWetSamples(block);

    // Fix 5: Calculate peak level for VU meter (after all DSP processing)
    // Store in dB format directly (like TapeAge pattern)
    float peakLevel = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const float* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            peakLevel = juce::jmax(peakLevel, std::abs(channelData[sample]));
        }
    }

    // Convert to dB and store atomically (clamp to -100dB minimum to avoid log(0))
    float peakDb = peakLevel > 0.00001f
        ? juce::Decibels::gainToDecibels(peakLevel)
        : -100.0f;
    outputLevel.store(peakDb, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* FlutterVerbAudioProcessor::createEditor()
{
    return new FlutterVerbAudioProcessorEditor(*this);
}

void FlutterVerbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FlutterVerbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FlutterVerbAudioProcessor();
}
