/*
  ==============================================================================

    O-Detune - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook
    Version: 1.1.1

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
    // Initialize random generator with current time for true randomness
    randomGenerator.setSeedRandomly();
}

ODetuneAudioProcessor::~ODetuneAudioProcessor() = default;

void ODetuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store sample rate for calculations
    currentSampleRate = sampleRate;

    // Calculate latency based on sample rate
    latencySamples = static_cast<int>((centerDelayMs / 1000.0) * sampleRate);

    // Prepare ProcessSpec for all juce::dsp components
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Wobble Engine delay lines (60ms buffer for 50ms center + modulation range)
    const int maxDelaySamples = static_cast<int>((60.0 / 1000.0) * sampleRate);
    wobbleDelayL.prepare(spec);
    wobbleDelayR.prepare(spec);
    wobbleDelayL.setMaximumDelayInSamples(maxDelaySamples);
    wobbleDelayR.setMaximumDelayInSamples(maxDelaySamples);
    wobbleDelayL.reset();
    wobbleDelayR.reset();

    // Unison Engine delay lines
    for (int i = 0; i < maxUnisonVoices; ++i)
    {
        unisonDelaysL[i].prepare(spec);
        unisonDelaysR[i].prepare(spec);
        unisonDelaysL[i].setMaximumDelayInSamples(maxDelaySamples);
        unisonDelaysR[i].setMaximumDelayInSamples(maxDelaySamples);
        unisonDelaysL[i].reset();
        unisonDelaysR[i].reset();

        // Initialize random offsets for voice randomization
        voiceRandomOffsets[i] = randomGenerator.nextFloat() * 2.0f - 1.0f;
    }

    // Pre-delay lines (50ms max)
    const int preDelayMaxSamples = static_cast<int>((50.0 / 1000.0) * sampleRate);
    preDelayL.prepare(spec);
    preDelayR.prepare(spec);
    preDelayL.setMaximumDelayInSamples(preDelayMaxSamples);
    preDelayR.setMaximumDelayInSamples(preDelayMaxSamples);
    preDelayL.reset();
    preDelayR.reset();

    // Focus Filter
    focusHighPassL.prepare(spec);
    focusHighPassR.prepare(spec);
    focusLowPassL.prepare(spec);
    focusLowPassR.prepare(spec);
    focusHighPassL.reset();
    focusHighPassR.reset();
    focusLowPassL.reset();
    focusLowPassR.reset();

    // Initialize filter coefficients
    lastFocusLow = 20.0f;
    lastFocusHigh = 20000.0f;
    auto highPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, lastFocusLow);
    auto lowPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lastFocusHigh);
    *focusHighPassL.coefficients = *highPassCoeffs;
    *focusHighPassR.coefficients = *highPassCoeffs;
    *focusLowPassL.coefficients = *lowPassCoeffs;
    *focusLowPassR.coefficients = *lowPassCoeffs;

    // Era filters
    eraFilterL.prepare(spec);
    eraFilterR.prepare(spec);
    eraFilterL.reset();
    eraFilterR.reset();
    lastEra = 1;  // 70s (bypass)

    // Color filters
    colorFilterL.prepare(spec);
    colorFilterR.prepare(spec);
    colorFilterL.reset();
    colorFilterR.reset();
    lastColor = 0.0f;

    // Dry/Wet Mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.setWetMixProportion(0.5f);
    dryWetMixer.reset();

    // Pre-allocate processing buffers
    const int numChannels = getTotalNumOutputChannels();
    wobbleBuffer.setSize(numChannels, samplesPerBlock);
    unisonBuffer.setSize(numChannels, samplesPerBlock);
    feedbackBuffer.setSize(numChannels, samplesPerBlock);
    wobbleBuffer.clear();
    unisonBuffer.clear();
    feedbackBuffer.clear();

    // Reset LFO state
    lfoPhase = 0.0f;
    randomCurrentValue = 0.0f;
    randomTargetValue = randomGenerator.nextFloat() * 2.0f - 1.0f;
    randomHoldCounter = 0;
    randomHoldSamples = static_cast<int>(sampleRate / 2.0f);  // Initial hold

    // Reset feedback
    feedbackL = 0.0f;
    feedbackR = 0.0f;
}

void ODetuneAudioProcessor::releaseResources()
{
    // Release processing buffers to save memory when plugin not in use
    wobbleBuffer.setSize(0, 0);
    unisonBuffer.setSize(0, 0);
    feedbackBuffer.setSize(0, 0);
}

float ODetuneAudioProcessor::generateLFOSample(int shape, float rate)
{
    float lfoValue = 0.0f;

    switch (shape)
    {
        case 0:  // Sine
            lfoValue = std::sin(lfoPhase * juce::MathConstants<float>::twoPi);
            break;

        case 1:  // Triangle
            lfoValue = 4.0f * std::abs(lfoPhase - std::floor(lfoPhase + 0.5f)) - 1.0f;
            break;

        case 2:  // Random (sample-and-hold with smoothing)
            if (randomHoldCounter >= randomHoldSamples)
            {
                randomTargetValue = randomGenerator.nextFloat() * 2.0f - 1.0f;
                randomHoldSamples = std::max(1, static_cast<int>(currentSampleRate / rate));
                randomHoldCounter = 0;
            }
            // Smooth interpolation (~10ms slew)
            const float smoothCoeff = 1.0f - std::exp(-1.0f / (0.01f * static_cast<float>(currentSampleRate)));
            randomCurrentValue += (randomTargetValue - randomCurrentValue) * smoothCoeff;
            randomHoldCounter++;
            lfoValue = randomCurrentValue;
            break;
    }

    // Advance phase for sine/triangle
    if (shape != 2)
    {
        lfoPhase += rate / static_cast<float>(currentSampleRate);
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;
    }

    return lfoValue;
}

void ODetuneAudioProcessor::applyEraCharacter(int era, float& rate, float& depth)
{
    switch (era)
    {
        case 0:  // 60s (Ampex) - slower, warmer, subtler
            rate *= 0.7f;
            depth *= 0.8f;
            break;

        case 1:  // 70s (Teac) - default, no modification
            break;

        case 2:  // 80s (Cassette) - faster flutter, more unstable
            rate *= 1.3f;
            depth *= 1.1f;
            break;
    }
}

float ODetuneAudioProcessor::applySaturation(float input, float driveAmount)
{
    if (driveAmount < 0.001f)
        return input;

    // Soft clipping with tanh waveshaping
    const float gain = 1.0f + driveAmount * 3.0f;
    const float driven = input * gain;
    const float saturated = std::tanh(driven);

    // Compensate for gain increase
    return saturated / gain;
}

void ODetuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0)
        return;

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, numSamples);

    //==============================================================================
    // Read ALL parameters (atomic, real-time safe)
    const float blendValue = parameters.getRawParameterValue("blend")->load();

    // Wobble parameters
    const int wobbleEra = static_cast<int>(parameters.getRawParameterValue("wobble_era")->load());
    float wobbleRate = parameters.getRawParameterValue("wobble_rate")->load();
    float wobbleDepth = parameters.getRawParameterValue("wobble_depth")->load();
    const int wobbleShape = static_cast<int>(parameters.getRawParameterValue("wobble_shape")->load());
    const bool wobbleSync = parameters.getRawParameterValue("wobble_sync")->load() > 0.5f;

    // Unison parameters
    const int unisonVoicesIdx = static_cast<int>(parameters.getRawParameterValue("unison_voices")->load());
    const float unisonDetune = parameters.getRawParameterValue("unison_detune")->load();
    const int unisonDist = static_cast<int>(parameters.getRawParameterValue("unison_dist")->load());
    const float unisonSpread = parameters.getRawParameterValue("unison_spread")->load() / 100.0f;

    // Character parameters
    const float driveAmount = parameters.getRawParameterValue("drive")->load() / 100.0f;
    const float colorValue = parameters.getRawParameterValue("color")->load();
    const float ageAmount = parameters.getRawParameterValue("age")->load() / 100.0f;

    // Output parameters
    const float widthValue = parameters.getRawParameterValue("width")->load() / 100.0f;
    const float mixValue = parameters.getRawParameterValue("mix")->load() / 100.0f;
    const float focusLow = parameters.getRawParameterValue("focus_low")->load();
    const float focusHigh = parameters.getRawParameterValue("focus_high")->load();
    const bool monoSafe = parameters.getRawParameterValue("mono_safe")->load() > 0.5f;

    // Advanced parameters
    const float preDelayMs = parameters.getRawParameterValue("delay")->load();
    const float feedbackAmount = parameters.getRawParameterValue("feedback")->load() / 100.0f;
    const float randomAmount = parameters.getRawParameterValue("random_amt")->load() / 100.0f;

    // Map voice index to count: {0→2, 1→3, 2→4, 3→5, 4→7}
    const int voiceCounts[5] = {2, 3, 4, 5, 7};
    const int activeVoices = voiceCounts[juce::jlimit(0, 4, unisonVoicesIdx)];

    //==============================================================================
    // Apply era character to wobble
    applyEraCharacter(wobbleEra, wobbleRate, wobbleDepth);

    // Handle tempo sync (if playhead available)
    if (wobbleSync)
    {
        if (auto* playHead = getPlayHead())
        {
            if (auto position = playHead->getPosition())
            {
                if (auto bpm = position->getBpm())
                {
                    // Sync to 1/4 note by default
                    wobbleRate = static_cast<float>(*bpm) / 60.0f;
                }
            }
        }
    }

    //==============================================================================
    // 1. Capture dry signal
    dryWetMixer.pushDrySamples(buffer);

    //==============================================================================
    // 2. Apply pre-delay if set
    if (preDelayMs > 0.01f)
    {
        const float preDelaySamples = (preDelayMs / 1000.0f) * static_cast<float>(currentSampleRate);
        preDelayL.setDelay(preDelaySamples);
        preDelayR.setDelay(preDelaySamples);

        if (numChannels >= 1)
        {
            auto* dataL = buffer.getWritePointer(0);
            for (int s = 0; s < numSamples; ++s)
            {
                preDelayL.pushSample(0, dataL[s]);
                dataL[s] = preDelayL.popSample(0);
            }
        }
        if (numChannels >= 2)
        {
            auto* dataR = buffer.getWritePointer(1);
            for (int s = 0; s < numSamples; ++s)
            {
                preDelayR.pushSample(0, dataR[s]);
                dataR[s] = preDelayR.popSample(0);
            }
        }
    }

    //==============================================================================
    // 3. Update focus filter coefficients only when changed
    if (std::abs(focusLow - lastFocusLow) > 0.01f)
    {
        lastFocusLow = focusLow;
        auto highPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, focusLow);
        *focusHighPassL.coefficients = *highPassCoeffs;
        *focusHighPassR.coefficients = *highPassCoeffs;
    }

    if (std::abs(focusHigh - lastFocusHigh) > 0.01f)
    {
        lastFocusHigh = focusHigh;
        auto lowPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, focusHigh);
        *focusLowPassL.coefficients = *lowPassCoeffs;
        *focusLowPassR.coefficients = *lowPassCoeffs;
    }

    // Apply focus filters
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
    // 4. Update era filter (60s=LP, 70s=bypass, 80s=HS boost)
    if (wobbleEra != lastEra)
    {
        lastEra = wobbleEra;
        switch (wobbleEra)
        {
            case 0:  // 60s - low-pass at 2kHz
            {
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, 2000.0f);
                *eraFilterL.coefficients = *coeffs;
                *eraFilterR.coefficients = *coeffs;
                break;
            }
            case 1:  // 70s - bypass (all-pass)
            {
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass(currentSampleRate, 1000.0f);
                *eraFilterL.coefficients = *coeffs;
                *eraFilterR.coefficients = *coeffs;
                break;
            }
            case 2:  // 80s - high-shelf boost at 4kHz
            {
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 4000.0f, 0.7f, 1.4f);
                *eraFilterL.coefficients = *coeffs;
                *eraFilterR.coefficients = *coeffs;
                break;
            }
        }
    }

    //==============================================================================
    // 5. Process Wobble Engine (delay-based pitch modulation)

    // Copy filtered signal to wobble buffer
    for (int ch = 0; ch < numChannels; ++ch)
        wobbleBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    const float centerDelaySamples = (centerDelayMs / 1000.0f) * static_cast<float>(currentSampleRate);
    const float pitchRatio = std::pow(2.0f, wobbleDepth / 1200.0f);
    const float modulationRange = centerDelaySamples * (pitchRatio - 1.0f);

    // Get channel pointers outside the sample loop
    float* wobbleDataL = (numChannels >= 1) ? wobbleBuffer.getWritePointer(0) : nullptr;
    float* wobbleDataR = (numChannels >= 2) ? wobbleBuffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Generate LFO with selected shape
        const float lfoValue = generateLFOSample(wobbleShape, wobbleRate);
        const float delayTime = centerDelaySamples + (lfoValue * modulationRange);

        if (wobbleDataL != nullptr)
        {
            // Mix in feedback
            float inputL = wobbleDataL[sample] + feedbackL * feedbackAmount;
            wobbleDelayL.setDelay(delayTime);
            wobbleDelayL.pushSample(0, inputL);
            float outputL = wobbleDelayL.popSample(0);

            // Apply era filter
            if (wobbleEra != 1)  // Skip for 70s (bypass)
                outputL = eraFilterL.processSample(outputL);

            wobbleDataL[sample] = outputL;
            feedbackL = outputL;
        }

        if (wobbleDataR != nullptr)
        {
            float inputR = wobbleDataR[sample] + feedbackR * feedbackAmount;
            wobbleDelayR.setDelay(delayTime);
            wobbleDelayR.pushSample(0, inputR);
            float outputR = wobbleDelayR.popSample(0);

            if (wobbleEra != 1)
                outputR = eraFilterR.processSample(outputR);

            wobbleDataR[sample] = outputR;
            feedbackR = outputR;
        }
    }

    //==============================================================================
    // 6. Process Unison Engine (multi-voice detuning)

    unisonBuffer.clear();

    // Calculate voice detuning based on distribution
    float voiceDetunes[maxUnisonVoices] = {0.0f};

    for (int v = 0; v < activeVoices; ++v)
    {
        // Normalized position [-1, 1]
        float normalizedPos = (activeVoices == 1) ? 0.0f :
            (static_cast<float>(v) / static_cast<float>(activeVoices - 1)) * 2.0f - 1.0f;

        switch (unisonDist)
        {
            case 0:  // Linear
                voiceDetunes[v] = normalizedPos * (unisonDetune / 2.0f);
                break;

            case 1:  // Exponential (cluster toward center)
            {
                float sign = (normalizedPos >= 0.0f) ? 1.0f : -1.0f;
                float absPos = std::abs(normalizedPos);
                voiceDetunes[v] = sign * std::pow(absPos, 2.0f) * (unisonDetune / 2.0f);
                break;
            }

            case 2:  // Random
                voiceDetunes[v] = normalizedPos * (unisonDetune / 2.0f)
                    + voiceRandomOffsets[v] * randomAmount * (unisonDetune / 4.0f);
                break;
        }
    }

    const float voiceGain = 1.0f / static_cast<float>(activeVoices);

    // Get buffer pointers outside the voice loop
    const float* inputDataL = (numChannels >= 1) ? buffer.getReadPointer(0) : nullptr;
    const float* inputDataR = (numChannels >= 2) ? buffer.getReadPointer(1) : nullptr;
    float* outputDataL = (numChannels >= 1) ? unisonBuffer.getWritePointer(0) : nullptr;
    float* outputDataR = (numChannels >= 2) ? unisonBuffer.getWritePointer(1) : nullptr;

    for (int voice = 0; voice < activeVoices; ++voice)
    {
        const float voicePitchRatio = std::pow(2.0f, voiceDetunes[voice] / 1200.0f);
        // Pitch UP = shorter delay (read faster), pitch DOWN = longer delay
        // Inverse relationship: higher pitch ratio means shorter delay time
        const float voiceDelayTime = centerDelaySamples / voicePitchRatio;

        // Calculate stereo pan for this voice
        float normalizedPos = (activeVoices == 1) ? 0.0f :
            (static_cast<float>(voice) / static_cast<float>(activeVoices - 1)) * 2.0f - 1.0f;
        float panL = std::cos((normalizedPos * unisonSpread + 1.0f) * juce::MathConstants<float>::halfPi * 0.5f);
        float panR = std::sin((normalizedPos * unisonSpread + 1.0f) * juce::MathConstants<float>::halfPi * 0.5f);

        if (inputDataL != nullptr)
        {
            unisonDelaysL[voice].setDelay(voiceDelayTime);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                unisonDelaysL[voice].pushSample(0, inputDataL[sample]);
                float voiceOutput = unisonDelaysL[voice].popSample(0) * voiceGain;
                outputDataL[sample] += voiceOutput * panL;
                if (outputDataR != nullptr)
                    outputDataR[sample] += voiceOutput * panR * 0.5f;  // Cross-feed for stereo
            }
        }

        if (inputDataR != nullptr)
        {
            unisonDelaysR[voice].setDelay(voiceDelayTime);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                unisonDelaysR[voice].pushSample(0, inputDataR[sample]);
                float voiceOutput = unisonDelaysR[voice].popSample(0) * voiceGain;
                outputDataR[sample] += voiceOutput * panR;
                if (outputDataL != nullptr)
                    outputDataL[sample] += voiceOutput * panL * 0.5f;  // Cross-feed
            }
        }
    }

    //==============================================================================
    // 7. Blend dual engines (blend=0: wobble, blend=1: unison)

    const float wobbleGain = 1.0f - blendValue;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* wobbleData = wobbleBuffer.getReadPointer(channel);
        const float* unisonData = unisonBuffer.getReadPointer(channel);
        float* outputData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
            outputData[sample] = wobbleData[sample] * wobbleGain + unisonData[sample] * blendValue;
    }

    //==============================================================================
    // 8. Apply Character processing (drive, color, age)

    // Update color filter if changed
    if (std::abs(colorValue - lastColor) > 0.1f)
    {
        lastColor = colorValue;
        if (colorValue < -10.0f)  // Dark (low-pass)
        {
            float freq = juce::jmap(colorValue, -100.0f, -10.0f, 1000.0f, 10000.0f);
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, freq);
            *colorFilterL.coefficients = *coeffs;
            *colorFilterR.coefficients = *coeffs;
        }
        else if (colorValue > 10.0f)  // Bright (high-shelf boost)
        {
            float gain = juce::jmap(colorValue, 10.0f, 100.0f, 1.0f, 2.0f);
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 3000.0f, 0.7f, gain);
            *colorFilterL.coefficients = *coeffs;
            *colorFilterR.coefficients = *coeffs;
        }
        else  // Neutral (bypass via all-pass)
        {
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass(currentSampleRate, 1000.0f);
            *colorFilterL.coefficients = *coeffs;
            *colorFilterR.coefficients = *coeffs;
        }
    }

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* data = buffer.getWritePointer(channel);
        auto& colorFilter = (channel == 0) ? colorFilterL : colorFilterR;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float s = data[sample];

            // Apply drive (saturation)
            s = applySaturation(s, driveAmount);

            // Apply color filter
            if (std::abs(colorValue) > 10.0f)
                s = colorFilter.processSample(s);

            // Apply age (noise + subtle drift)
            if (ageAmount > 0.01f)
            {
                // Add subtle noise
                float noise = (randomGenerator.nextFloat() * 2.0f - 1.0f) * ageAmount * 0.01f;
                s += noise;
            }

            data[sample] = s;
        }
    }

    //==============================================================================
    // 9. Apply stereo width

    if (numChannels >= 2 && std::abs(widthValue - 1.0f) > 0.01f)
    {
        float* dataL = buffer.getWritePointer(0);
        float* dataR = buffer.getWritePointer(1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mid = (dataL[sample] + dataR[sample]) * 0.5f;
            float side = (dataL[sample] - dataR[sample]) * 0.5f;

            side *= widthValue;

            dataL[sample] = mid + side;
            dataR[sample] = mid - side;
        }
    }

    //==============================================================================
    // 10. Apply mono-safe processing

    if (monoSafe && numChannels >= 2)
    {
        float* dataL = buffer.getWritePointer(0);
        float* dataR = buffer.getWritePointer(1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Check correlation - if negative (out of phase), reduce side
            float mid = (dataL[sample] + dataR[sample]) * 0.5f;
            float side = (dataL[sample] - dataR[sample]) * 0.5f;

            // Limit side content to prevent phase cancellation
            float sideLimit = std::abs(mid) * 1.5f;
            if (std::abs(side) > sideLimit)
                side = (side > 0.0f) ? sideLimit : -sideLimit;

            dataL[sample] = mid + side;
            dataR[sample] = mid - side;
        }
    }

    //==============================================================================
    // 11. Apply dry/wet mix
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
