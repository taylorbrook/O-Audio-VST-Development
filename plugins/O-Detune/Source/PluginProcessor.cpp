/*
  ==============================================================================

    O-Detune - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout ODetuneAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Mode Selection (1 parameter)

    // blend - Wobble/Unison crossfade
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "blend", 1 },
        "Blend",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // Wobble Engine (5 parameters)

    // wobble_era - Era character preset
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "wobble_era", 1 },
        "Wobble Era",
        juce::StringArray { "60s", "70s", "80s" },
        1  // Default: 70s
    ));

    // wobble_rate - Modulation speed
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wobble_rate", 1 },
        "Wobble Rate",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f),  // Skew for log scale
        2.0f,
        "Hz"
    ));

    // wobble_depth - Pitch deviation amount
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wobble_depth", 1 },
        "Wobble Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "cents"
    ));

    // wobble_shape - LFO waveform
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "wobble_shape", 1 },
        "Wobble Shape",
        juce::StringArray { "Sine", "Triangle", "Random" },
        0  // Default: Sine
    ));

    // wobble_sync - Tempo sync enable
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "wobble_sync", 1 },
        "Wobble Sync",
        false  // Default: Off
    ));

    // Unison Engine (4 parameters)

    // unison_voices - Number of parallel voices
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "unison_voices", 1 },
        "Unison Voices",
        juce::StringArray { "2", "3", "4", "5", "7" },
        1  // Default: 3
    ));

    // unison_detune - Total spread
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "unison_detune", 1 },
        "Unison Detune",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f),
        15.0f,
        "cents"
    ));

    // unison_dist - Voice distribution
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "unison_dist", 1 },
        "Unison Distribution",
        juce::StringArray { "Linear", "Exp", "Random" },
        0  // Default: Linear
    ));

    // unison_spread - Stereo panning width
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "unison_spread", 1 },
        "Unison Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        75.0f,
        "%"
    ));

    // Character Section (3 parameters)

    // drive - Saturation intensity
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "drive", 1 },
        "Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        20.0f,
        "%"
    ));

    // color - Dark to Bright tone shaping
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "color", 1 },
        "Color",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f
    ));

    // age - Combined degradation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "age", 1 },
        "Age",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    // Output Section (5 parameters)

    // width - Stereo spread
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "width", 1 },
        "Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f),
        100.0f,
        "%"
    ));

    // mix - Wet/dry blend
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // focus_low - Processing frequency low bound
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "focus_low", 1 },
        "Focus Low",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.3f),  // Skew for log frequency
        20.0f,
        "Hz"
    ));

    // focus_high - Processing frequency high bound
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "focus_high", 1 },
        "Focus High",
        juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.3f),  // Skew for log frequency
        20000.0f,
        "Hz"
    ));

    // mono_safe - Guarantees mono compatibility
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "mono_safe", 1 },
        "Mono Safe",
        true  // Default: On
    ));

    // Advanced Section (3 parameters)

    // delay - Spatial depth pre-delay
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delay", 1 },
        "Pre-Delay",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f),
        0.0f,
        "ms"
    ));

    // feedback - Recirculation amount
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1 },
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 80.0f, 0.1f),
        0.0f,
        "%"
    ));

    // random_amt - Per-voice variation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "random_amt", 1 },
        "Randomization",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        15.0f,
        "%"
    ));

    return layout;
}

ODetuneAudioProcessor::ODetuneAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

ODetuneAudioProcessor::~ODetuneAudioProcessor()
{
}

void ODetuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store sample rate for calculations
    currentSampleRate = sampleRate;

    // Calculate latency based on sample rate
    // 50ms delay line = (50 / 1000.0) * sampleRate
    latencySamples = static_cast<int>((centerDelayMs / 1000.0) * sampleRate);

    // Prepare ProcessSpec for all juce::dsp components
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare Wobble Engine
    // Center delay: 50ms + modulation range (±100 cents = ~4.8ms)
    // Buffer size: 60ms to accommodate modulation
    const int maxDelaySamples = static_cast<int>((60.0 / 1000.0) * sampleRate);
    wobbleDelayL.prepare(spec);
    wobbleDelayR.prepare(spec);
    wobbleDelayL.setMaximumDelayInSamples(maxDelaySamples);
    wobbleDelayR.setMaximumDelayInSamples(maxDelaySamples);
    wobbleDelayL.reset();
    wobbleDelayR.reset();

    // Prepare Wobble LFO (Phase 4.1: Sine wave only)
    wobbleLFO.prepare(spec);
    wobbleLFO.initialise([](float x) { return std::sin(x); });  // Sine wave
    wobbleLFO.setFrequency(2.0f);  // Default 2 Hz
    wobbleLFO.reset();

    // Prepare Unison Engine (all 7 delay lines for future phases)
    for (int i = 0; i < maxUnisonVoices; ++i)
    {
        unisonDelaysL[i].prepare(spec);
        unisonDelaysR[i].prepare(spec);
        unisonDelaysL[i].setMaximumDelayInSamples(maxDelaySamples);
        unisonDelaysR[i].setMaximumDelayInSamples(maxDelaySamples);
        unisonDelaysL[i].reset();
        unisonDelaysR[i].reset();
    }

    // Prepare Focus Filter
    focusHighPassL.prepare(spec);
    focusHighPassR.prepare(spec);
    focusLowPassL.prepare(spec);
    focusLowPassR.prepare(spec);
    focusHighPassL.reset();
    focusHighPassR.reset();
    focusLowPassL.reset();
    focusLowPassR.reset();

    // Prepare Dry/Wet Mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.setWetMixProportion(0.5f);  // Default 50% mix
    dryWetMixer.reset();

    // Pre-allocate processing buffers (real-time safety)
    const int numChannels = getTotalNumOutputChannels();
    wobbleBuffer.setSize(numChannels, samplesPerBlock);
    unisonBuffer.setSize(numChannels, samplesPerBlock);
    wobbleBuffer.clear();
    unisonBuffer.clear();
}

void ODetuneAudioProcessor::releaseResources()
{
    // Release processing buffers to save memory when plugin not in use
    wobbleBuffer.setSize(0, 0);
    unisonBuffer.setSize(0, 0);
}

void ODetuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Early exit for zero-length buffers
    if (buffer.getNumSamples() == 0)
        return;

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    //==============================================================================
    // Read parameters (atomic, real-time safe)

    // Mode blend (0 = wobble only, 1 = unison only)
    auto* blendParam = parameters.getRawParameterValue("blend");
    float blendValue = blendParam->load();

    // Wobble engine parameters
    auto* wobbleRateParam = parameters.getRawParameterValue("wobble_rate");
    auto* wobbleDepthParam = parameters.getRawParameterValue("wobble_depth");
    float wobbleRate = wobbleRateParam->load();
    float wobbleDepth = wobbleDepthParam->load();

    // Unison engine parameters
    auto* unisonDetuneParam = parameters.getRawParameterValue("unison_detune");
    float unisonDetune = unisonDetuneParam->load();

    // Phase 4.1: Fixed 3 voices (will expand in Phase 4.3)
    int activeVoices = 3;

    // Focus filter parameters
    auto* focusLowParam = parameters.getRawParameterValue("focus_low");
    auto* focusHighParam = parameters.getRawParameterValue("focus_high");
    float focusLow = focusLowParam->load();
    float focusHigh = focusHighParam->load();

    // Mix parameter (0-100%)
    auto* mixParam = parameters.getRawParameterValue("mix");
    float mixValue = mixParam->load() / 100.0f;  // Convert to 0.0-1.0

    //==============================================================================
    // Update LFO frequency
    wobbleLFO.setFrequency(wobbleRate);

    //==============================================================================
    // 1. Capture dry signal (DryWetMixer)
    dryWetMixer.pushDrySamples(buffer);

    //==============================================================================
    // 2. Apply Focus Filter (frequency-selective processing)

    // Update focus filter coefficients
    auto highPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, focusLow);
    auto lowPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, focusHigh);

    *focusHighPassL.coefficients = *highPassCoeffs;
    *focusHighPassR.coefficients = *highPassCoeffs;
    *focusLowPassL.coefficients = *lowPassCoeffs;
    *focusLowPassR.coefficients = *lowPassCoeffs;

    // Process focus filters
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels >= 1)
    {
        auto* channelDataL = buffer.getWritePointer(0);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelDataL[sample] = focusHighPassL.processSample(channelDataL[sample]);
            channelDataL[sample] = focusLowPassL.processSample(channelDataL[sample]);
        }
    }

    if (numChannels >= 2)
    {
        auto* channelDataR = buffer.getWritePointer(1);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelDataR[sample] = focusHighPassR.processSample(channelDataR[sample]);
            channelDataR[sample] = focusLowPassR.processSample(channelDataR[sample]);
        }
    }

    //==============================================================================
    // 3. Process Wobble Engine (delay-based pitch modulation)

    wobbleBuffer.makeCopyOf(buffer, true);  // Copy filtered signal

    // Calculate center delay time in samples
    const float centerDelaySamples = (centerDelayMs / 1000.0f) * static_cast<float>(currentSampleRate);

    // Pre-calculate pitch modulation parameters
    // wobbleDepth in cents, 1200 cents = 1 octave
    float pitchRatio = std::pow(2.0f, wobbleDepth / 1200.0f);
    float modulationRange = centerDelaySamples * (pitchRatio - 1.0f);

    // Process wobble modulation (both channels share same LFO phase)
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get LFO value (-1 to +1)
        float lfoValue = wobbleLFO.processSample(0.0f);

        // Calculate modulated delay time
        float delayTime = centerDelaySamples + (lfoValue * modulationRange);

        // Process left channel
        if (numChannels >= 1)
        {
            auto* wobbleDataL = wobbleBuffer.getWritePointer(0);
            wobbleDelayL.setDelay(delayTime);
            wobbleDelayL.pushSample(0, wobbleDataL[sample]);
            wobbleDataL[sample] = wobbleDelayL.popSample(0);
        }

        // Process right channel (same LFO phase)
        if (numChannels >= 2)
        {
            auto* wobbleDataR = wobbleBuffer.getWritePointer(1);
            wobbleDelayR.setDelay(delayTime);
            wobbleDelayR.pushSample(0, wobbleDataR[sample]);
            wobbleDataR[sample] = wobbleDelayR.popSample(0);
        }
    }

    //==============================================================================
    // 4. Process Unison Engine (multi-voice detuning)

    unisonBuffer.makeCopyOf(buffer, true);  // Copy filtered signal
    unisonBuffer.clear();  // Clear for voice accumulation

    // Phase 4.1: 3 voices with linear distribution
    // Voice detuning: [-detune/2, 0, +detune/2] cents
    float voiceDetunes[3];
    voiceDetunes[0] = -unisonDetune / 2.0f;  // Voice 1: negative detune
    voiceDetunes[1] = 0.0f;                   // Voice 2: center (unity pitch)
    voiceDetunes[2] = +unisonDetune / 2.0f;   // Voice 3: positive detune

    // Process each voice
    for (int voice = 0; voice < activeVoices; ++voice)
    {
        float detuneCents = voiceDetunes[voice];

        // Calculate static delay time for this voice
        // delay(voice) = centerDelay * 2^(detune_cents/1200)
        float voicePitchRatio = std::pow(2.0f, detuneCents / 1200.0f);
        float voiceDelayTime = centerDelaySamples * voicePitchRatio;

        // Process left channel
        if (numChannels >= 1)
        {
            auto* inputDataL = buffer.getReadPointer(0);
            auto* outputDataL = unisonBuffer.getWritePointer(0);

            unisonDelaysL[voice].setDelay(voiceDelayTime);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                unisonDelaysL[voice].pushSample(0, inputDataL[sample]);
                float delayedSample = unisonDelaysL[voice].popSample(0);
                outputDataL[sample] += delayedSample / static_cast<float>(activeVoices);
            }
        }

        // Process right channel
        if (numChannels >= 2)
        {
            auto* inputDataR = buffer.getReadPointer(1);
            auto* outputDataR = unisonBuffer.getWritePointer(1);

            unisonDelaysR[voice].setDelay(voiceDelayTime);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                unisonDelaysR[voice].pushSample(0, inputDataR[sample]);
                float delayedSample = unisonDelaysR[voice].popSample(0);
                outputDataR[sample] += delayedSample / static_cast<float>(activeVoices);
            }
        }
    }

    //==============================================================================
    // 5. Blend dual engines (crossfade)
    // blend = 0: wobble only, blend = 1: unison only

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* wobbleData = wobbleBuffer.getWritePointer(channel);
        auto* unisonData = unisonBuffer.getReadPointer(channel);
        auto* outputData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            outputData[sample] = wobbleData[sample] * (1.0f - blendValue) + unisonData[sample] * blendValue;
        }
    }

    //==============================================================================
    // 6. Blend with dry signal (final mix)
    dryWetMixer.setWetMixProportion(mixValue);
    dryWetMixer.mixWetSamples(buffer);
}

juce::AudioProcessorEditor* ODetuneAudioProcessor::createEditor()
{
    return new ODetuneAudioProcessorEditor(*this);
}

void ODetuneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ODetuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ODetuneAudioProcessor();
}
