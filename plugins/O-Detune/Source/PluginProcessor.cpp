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

    // Initialize pre-delay lines (50ms max)
    const int maxPreDelaySamples = static_cast<int>(0.05 * sampleRate);
    preDelayL.prepare(spec);
    preDelayR.prepare(spec);
    preDelayL.setMaximumDelayInSamples(maxPreDelaySamples);
    preDelayR.setMaximumDelayInSamples(maxPreDelaySamples);
    preDelayL.reset();
    preDelayR.reset();

    // Initialize color filters
    colorFilterL.prepare(spec);
    colorFilterR.prepare(spec);
    colorFilterL.reset();
    colorFilterR.reset();

    // Initialize parameter smoothing (50ms ramp)
    const double smoothingTime = 0.05;
    smoothedBlend.reset(sampleRate, smoothingTime);
    smoothedWobbleRate.reset(sampleRate, smoothingTime);
    smoothedWobbleDepth.reset(sampleRate, smoothingTime);
    smoothedUnisonDetune.reset(sampleRate, smoothingTime);
    smoothedDrive.reset(sampleRate, smoothingTime);
    smoothedColor.reset(sampleRate, smoothingTime);
    smoothedAge.reset(sampleRate, smoothingTime);
    smoothedWidth.reset(sampleRate, smoothingTime);
    smoothedDelay.reset(sampleRate, smoothingTime);
    smoothedFeedback.reset(sampleRate, smoothingTime);
    smoothedUnisonSpread.reset(sampleRate, smoothingTime);
    smoothedRandomAmt.reset(sampleRate, smoothingTime);

    // Reset LFO state
    lfoPhase = 0.0f;
    noiseHeldValue = 0.0f;
    noiseLastQuarter = -1;
    filterDriftPhase = 0.0f;
    randomRefreshCounter = 0;
    feedbackStateL = 0.0f;
    feedbackStateR = 0.0f;

    // Reset unison LFO phases (stagger for rich chorusing)
    for (int i = 0; i < maxUnisonVoices; ++i)
    {
        voiceLfoPhases[i] = static_cast<float>(i) / static_cast<float>(maxUnisonVoices);
    }

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

//==============================================================================
// DSP Helper Functions

// Multi-waveform LFO generator
float ODetuneAudioProcessor::generateLFO(float phase, int shapeType, float& noiseHeld, int& /*lastQuarter*/, juce::Random& rng)
{
    switch (shapeType)
    {
        case 0: // Sine
            return std::sin(phase * juce::MathConstants<float>::twoPi);

        case 1: // Triangle
        {
            float t = phase;
            if (t < 0.25f)
                return t * 4.0f;
            else if (t < 0.75f)
                return 2.0f - (t * 4.0f);
            else
                return -4.0f + (t * 4.0f);
        }

        case 2: // Random (smoothed noise - continuous random walk)
        {
            // Slow random walk: drift toward new target smoothly
            float driftSpeed = 0.0005f;  // Very slow drift for smooth random pitch bending
            float newTarget = rng.nextFloat() * 2.0f - 1.0f;
            noiseHeld += (newTarget - noiseHeld) * driftSpeed;
            return noiseHeld;
        }

        default:
            return std::sin(phase * juce::MathConstants<float>::twoPi);
    }
}

// Tube-style saturation processor
float ODetuneAudioProcessor::processDrive(float input, float driveAmount)
{
    if (driveAmount < 0.5f) return input;  // Lower threshold

    float driveMix = driveAmount / 100.0f;
    float gain = 1.0f + driveMix * 3.0f;  // 1.0 to 4.0
    float driven = input * gain;

    // Tube-style asymmetric soft clipping
    float saturated;
    if (driven >= 0.0f)
        saturated = driven / (1.0f + std::abs(driven));
    else
        saturated = std::tanh(driven * 1.5f) / 1.5f;

    // Add subtle even harmonics
    float x2 = saturated * saturated;
    saturated = saturated + 0.1f * x2 * ((saturated > 0) ? 1.0f : -1.0f);

    return input * (1.0f - driveMix) + saturated * driveMix;
}

// Stereo width processor (M/S processing)
void ODetuneAudioProcessor::processWidth(float& left, float& right, float widthPercent)
{
    const float sqrtHalf = 0.70710678f;

    // Encode to M/S
    float mid = (left + right) * sqrtHalf;
    float side = (left - right) * sqrtHalf;

    // Scale side by width (0=mono, 1=normal, 2=extra-wide)
    float widthScale = widthPercent / 100.0f;
    side *= widthScale;

    // Decode back to L/R
    left = (mid + side) * sqrtHalf;
    right = (mid - side) * sqrtHalf;
}

// Mono-safe limiter (prevents phase cancellation)
void ODetuneAudioProcessor::processMonoSafe(float& left, float& right)
{
    const float sqrtHalf = 0.70710678f;

    float mid = (left + right) * sqrtHalf;
    float side = (left - right) * sqrtHalf;

    // Gentle side reduction using soft knee compression
    // Only reduce side when it significantly exceeds mid (phase cancellation risk)
    float sideAbs = std::abs(side);
    float midAbs = std::abs(mid) + 0.0001f;  // Avoid div/0
    float ratio = sideAbs / midAbs;

    // Soft knee: start reducing when side > 1.5x mid, fully limited at 3x
    if (ratio > 1.5f)
    {
        float reduction = 1.0f / (1.0f + (ratio - 1.5f) * 0.3f);  // Gentle compression
        side *= reduction;
    }

    left = (mid + side) * sqrtHalf;
    right = (mid - side) * sqrtHalf;
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

    // Wobble engine - additional parameters
    auto* wobbleEraParam = parameters.getRawParameterValue("wobble_era");
    auto* wobbleShapeParam = parameters.getRawParameterValue("wobble_shape");
    auto* wobbleSyncParam = parameters.getRawParameterValue("wobble_sync");
    int wobbleEra = static_cast<int>(wobbleEraParam->load());
    int wobbleShape = static_cast<int>(wobbleShapeParam->load());
    bool wobbleSync = wobbleSyncParam->load() > 0.5f;

    // Unison engine parameters
    auto* unisonDetuneParam = parameters.getRawParameterValue("unison_detune");
    float unisonDetune = unisonDetuneParam->load();

    // Unison engine - additional parameters
    auto* unisonVoicesParam = parameters.getRawParameterValue("unison_voices");
    auto* unisonDistParam = parameters.getRawParameterValue("unison_dist");
    auto* unisonSpreadParam = parameters.getRawParameterValue("unison_spread");
    auto* randomAmtParam = parameters.getRawParameterValue("random_amt");
    int unisonVoicesIndex = static_cast<int>(unisonVoicesParam->load());
    int unisonDist = static_cast<int>(unisonDistParam->load());
    float unisonSpread = unisonSpreadParam->load();
    float randomAmt = randomAmtParam->load();

    // Map voice index to count: 0→2, 1→3, 2→4, 3→5, 4→7
    const int voiceCounts[] = { 2, 3, 4, 5, 7 };
    int activeVoices = voiceCounts[unisonVoicesIndex];

    // Character section
    auto* driveParam = parameters.getRawParameterValue("drive");
    auto* colorParam = parameters.getRawParameterValue("color");
    auto* ageParam = parameters.getRawParameterValue("age");
    float driveValue = driveParam->load();
    float colorValue = colorParam->load();
    float ageValue = ageParam->load();

    // Output section
    auto* widthParam = parameters.getRawParameterValue("width");
    auto* monoSafeParam = parameters.getRawParameterValue("mono_safe");
    auto* delayParam = parameters.getRawParameterValue("delay");
    auto* feedbackParam = parameters.getRawParameterValue("feedback");
    float widthValue = widthParam->load();
    bool monoSafe = monoSafeParam->load() > 0.5f;
    float delayMs = delayParam->load();
    float feedbackValue = feedbackParam->load();

    // Focus filter parameters
    auto* focusLowParam = parameters.getRawParameterValue("focus_low");
    auto* focusHighParam = parameters.getRawParameterValue("focus_high");
    float focusLow = focusLowParam->load();
    float focusHigh = focusHighParam->load();

    // Mix parameter (0-100%)
    auto* mixParam = parameters.getRawParameterValue("mix");
    float mixValue = mixParam->load() / 100.0f;  // Convert to 0.0-1.0

    // Update smoothed values
    smoothedBlend.setTargetValue(blendValue);
    smoothedWobbleRate.setTargetValue(wobbleRate);
    smoothedWobbleDepth.setTargetValue(wobbleDepth);
    smoothedUnisonDetune.setTargetValue(unisonDetune);
    smoothedDrive.setTargetValue(driveValue);
    smoothedColor.setTargetValue(colorValue);
    smoothedAge.setTargetValue(ageValue);
    // Mono-safe forces width to 0 (mono) - parameter value preserved for restoration
    smoothedWidth.setTargetValue(monoSafe ? 0.0f : widthValue);
    smoothedDelay.setTargetValue(delayMs);
    smoothedFeedback.setTargetValue(feedbackValue);
    smoothedUnisonSpread.setTargetValue(unisonSpread);
    smoothedRandomAmt.setTargetValue(randomAmt);

    // Era presets: { depthMultiplier, filterDarkening, driftAmount }
    struct EraPreset { float depthMult; float darkness; float drift; };
    const EraPreset eraPresets[] = {
        { 1.2f, 0.8f, 0.15f },  // 60s: More depth, darker, more drift
        { 1.0f, 1.0f, 0.08f },  // 70s: Neutral
        { 0.8f, 1.1f, 0.03f },  // 80s: Less depth, brighter, less drift
    };

    // Apply era scaling to wobble depth
    float scaledWobbleDepth = wobbleDepth * eraPresets[wobbleEra].depthMult;

    //==============================================================================
    // Calculate effective wobble rate (with tempo sync if enabled)
    float effectiveRate = wobbleRate;

    if (wobbleSync)
    {
        double hostBpm = 120.0;  // Fallback

        if (auto* playHead = getPlayHead())
        {
            auto posInfo = playHead->getPosition();
            if (posInfo.hasValue() && posInfo->getBpm().hasValue())
            {
                hostBpm = *posInfo->getBpm();
            }
        }

        double beatsPerSecond = hostBpm / 60.0;

        // Map wobbleRate (0.1-10 Hz) to nearest musical division
        // Divisions: 1/16=0.25, 1/8=0.5, 1/4=1, 1/2=2, 1=4 beats
        const float divisions[] = { 0.25f, 0.333f, 0.5f, 1.0f, 2.0f, 4.0f };
        const int numDivisions = 6;

        // Find closest musical rate
        float targetHz = wobbleRate;
        float closestHz = static_cast<float>(beatsPerSecond / divisions[0]);
        float closestDiff = std::abs(targetHz - closestHz);

        for (int i = 1; i < numDivisions; ++i)
        {
            float hz = static_cast<float>(beatsPerSecond / divisions[i]);
            float diff = std::abs(targetHz - hz);
            if (diff < closestDiff)
            {
                closestHz = hz;
                closestDiff = diff;
            }
        }

        effectiveRate = closestHz;
    }

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
    // 3. Pre-Delay + Feedback processing
    float currentDelay = smoothedDelay.getCurrentValue();
    float currentFeedback = smoothedFeedback.getCurrentValue() / 100.0f;

    if (currentDelay > 0.1f || currentFeedback > 0.01f)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float delayMsVal = smoothedDelay.getNextValue();
            float feedback = smoothedFeedback.getNextValue() / 100.0f;
            float delaySamples = (delayMsVal / 1000.0f) * static_cast<float>(currentSampleRate);

            if (numChannels >= 1)
            {
                auto* dataL = buffer.getWritePointer(0);
                preDelayL.setDelay(delaySamples);
                float delayedL = preDelayL.popSample(0);
                float toDelayL = dataL[sample] + delayedL * feedback;
                preDelayL.pushSample(0, toDelayL);
                dataL[sample] = delayedL;
            }

            if (numChannels >= 2)
            {
                auto* dataR = buffer.getWritePointer(1);
                preDelayR.setDelay(delaySamples);
                float delayedR = preDelayR.popSample(0);
                float toDelayR = dataR[sample] + delayedR * feedback;
                preDelayR.pushSample(0, toDelayR);
                dataR[sample] = delayedR;
            }
        }
    }

    //==============================================================================
    // 4. Drive processing (tube saturation)
    float currentDrive = smoothedDrive.getCurrentValue();
    if (currentDrive > 0.5f)  // Process if drive > 0.5%
    {
        auto* dataL = numChannels >= 1 ? buffer.getWritePointer(0) : nullptr;
        auto* dataR = numChannels >= 2 ? buffer.getWritePointer(1) : nullptr;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float drive = smoothedDrive.getNextValue();
            if (dataL) dataL[sample] = processDrive(dataL[sample], drive);
            if (dataR) dataR[sample] = processDrive(dataR[sample], drive);
        }
    }

    //==============================================================================
    // 5. Color + Age drift processing
    float currentColor = smoothedColor.getCurrentValue();
    // Process color if not near zero (threshold lowered for audibility)
    if (std::abs(currentColor) > 0.5f)
    {
        // Calculate drift modulation from age
        float ageForDrift = smoothedAge.getCurrentValue();
        float driftMod = std::sin(filterDriftPhase * juce::MathConstants<float>::twoPi) * (ageForDrift / 100.0f) * 0.2f;

        // Update filter coefficients (once per block for efficiency)
        if (currentColor < 0)
        {
            // Negative: Low-pass filter to darken (more dramatic effect)
            float cutoff = juce::jmap(currentColor, -100.0f, 0.0f, 1000.0f, 18000.0f);
            cutoff = std::max(200.0f, cutoff * (1.0f + driftMod));  // Apply drift, clamp minimum
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, cutoff, 0.707f);
            *colorFilterL.coefficients = *coeffs;
            *colorFilterR.coefficients = *coeffs;
        }
        else
        {
            // Positive: High-shelf boost (bright)
            float cutoff = 2500.0f * (1.0f + driftMod);  // Apply drift
            float gainDb = 9.0f * (currentColor / 100.0f);  // Increased gain for audibility
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                currentSampleRate, cutoff, 0.707f, juce::Decibels::decibelsToGain(gainDb));
            *colorFilterL.coefficients = *coeffs;
            *colorFilterR.coefficients = *coeffs;
        }

        // Process both channels
        auto* dataL = numChannels >= 1 ? buffer.getWritePointer(0) : nullptr;
        auto* dataR = numChannels >= 2 ? buffer.getWritePointer(1) : nullptr;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            if (dataL) dataL[sample] = colorFilterL.processSample(dataL[sample]);
            if (dataR) dataR[sample] = colorFilterR.processSample(dataR[sample]);
        }
    }

    //==============================================================================
    // 6. Age hiss processing
    float currentAge = smoothedAge.getCurrentValue();
    if (currentAge > 0.5f)  // Lower threshold
    {
        float ageMix = currentAge / 100.0f;
        float eraDrift = eraPresets[wobbleEra].drift;
        float effectiveAgeDrift = ageMix * (eraDrift * 10.0f);  // Scale up drift effect

        // 1. Hiss: Audible broadband noise (scaled for audibility)
        float hissLevel = ageMix * 0.05f;  // Increased from 0.02 for audibility

        auto* dataL = numChannels >= 1 ? buffer.getWritePointer(0) : nullptr;
        auto* dataR = numChannels >= 2 ? buffer.getWritePointer(1) : nullptr;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float noiseL = (random.nextFloat() * 2.0f - 1.0f) * hissLevel;
            float noiseR = (random.nextFloat() * 2.0f - 1.0f) * hissLevel;

            if (dataL) dataL[sample] += noiseL;
            if (dataR) dataR[sample] += noiseR;
        }

        // 2. Filter drift: Update phase for next block (affects color filter modulation)
        filterDriftPhase += 0.5f * numSamples / static_cast<float>(currentSampleRate) * (1.0f + effectiveAgeDrift);
        if (filterDriftPhase >= 1.0f) filterDriftPhase -= 1.0f;
    }

    //==============================================================================
    // 7. Process Wobble Engine (delay-based pitch modulation)

    wobbleBuffer.makeCopyOf(buffer, true);  // Copy filtered signal

    // Calculate center delay time in samples
    const float centerDelaySamples = (centerDelayMs / 1000.0f) * static_cast<float>(currentSampleRate);

    // Pre-calculate pitch modulation parameters
    // Use era-scaled wobble depth
    float pitchRatio = std::pow(2.0f, scaledWobbleDepth / 1200.0f);
    float modulationRange = centerDelaySamples * (pitchRatio - 1.0f);

    // Process wobble modulation (both channels share same LFO phase)
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get LFO value (-1 to +1) using multi-waveform generator
        float lfoValue = generateLFO(lfoPhase, wobbleShape, noiseHeldValue, noiseLastQuarter, random);

        // Advance phase
        lfoPhase += effectiveRate / static_cast<float>(currentSampleRate);
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

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
    // 8. Process Unison Engine (multi-voice detuning with continuous pitch shift)

    unisonBuffer.makeCopyOf(buffer, true);  // Copy filtered signal
    unisonBuffer.clear();  // Clear for voice accumulation

    // Calculate voice detunes based on distribution algorithm
    float voiceDetunes[maxUnisonVoices];
    float halfCount = (activeVoices - 1) / 2.0f;

    for (int i = 0; i < activeVoices; ++i)
    {
        float normalizedPos = (halfCount > 0.0f) ? (i - halfCount) / halfCount : 0.0f;

        switch (unisonDist)
        {
            case 0: // Linear
                voiceDetunes[i] = normalizedPos * unisonDetune;
                break;

            case 1: // Exponential (more voices near center)
            {
                float sign = (normalizedPos >= 0) ? 1.0f : -1.0f;
                voiceDetunes[i] = sign * std::pow(std::abs(normalizedPos), 2.0f) * unisonDetune;
                break;
            }

            case 2: // Random (stable random offsets - don't add again later)
                voiceDetunes[i] = voiceRandomOffsets[i] * unisonDetune;
                break;
        }
    }

    // Calculate per-voice pan positions
    float voicePanL[maxUnisonVoices];
    float voicePanR[maxUnisonVoices];
    float spreadNorm = unisonSpread / 100.0f;

    for (int i = 0; i < activeVoices; ++i)
    {
        float normalizedPos = (halfCount > 0.0f) ? (i - halfCount) / halfCount : 0.0f;
        float pan = normalizedPos * spreadNorm;

        // Constant-power panning
        voicePanL[i] = std::cos((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
        voicePanR[i] = std::sin((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
    }

    // Add random variation (refresh slowly - every 4096 samples for smoother transitions)
    if (randomRefreshCounter <= 0)
    {
        for (int i = 0; i < maxUnisonVoices; ++i)
        {
            // Smoother random: interpolate toward new target instead of jumping
            float newTarget = (random.nextFloat() * 2.0f - 1.0f);
            voiceRandomOffsets[i] = voiceRandomOffsets[i] * 0.7f + newTarget * 0.3f;
        }
        randomRefreshCounter = 4096;
    }
    randomRefreshCounter -= numSamples;

    // ==========================================================================
    // CLASSIC CHORUS ALGORITHM (inherently click-free)
    // Each voice has a sine LFO that modulates delay time continuously.
    // The detune parameter controls modulation depth (more detune = wider sweep).
    // Distribution affects the LFO rate spread between voices.
    // ==========================================================================

    // Convert detune (cents) to delay modulation depth (samples)
    // 50 cents ≈ 3% pitch change, which needs ~1.5ms delay modulation at 50ms center
    const float maxModulationMs = 3.0f;  // ±3ms max modulation
    const float modulationDepthSamples = (unisonDetune / 50.0f) * (maxModulationMs / 1000.0f) * static_cast<float>(currentSampleRate);

    // Base LFO rate (Hz) - slow for chorus effect
    const float baseLfoRate = 0.5f;  // 0.5 Hz = 2 second cycle

    // Process each voice
    for (int voice = 0; voice < activeVoices; ++voice)
    {
        // Calculate this voice's LFO rate based on distribution
        float voiceLfoRate = baseLfoRate;
        float normalizedPos = (halfCount > 0.0f) ? (voice - halfCount) / halfCount : 0.0f;

        switch (unisonDist)
        {
            case 0: // Linear - voices have evenly spread rates
                voiceLfoRate = baseLfoRate * (1.0f + normalizedPos * 0.3f);
                break;

            case 1: // Exponential - outer voices have more rate difference
            {
                float sign = (normalizedPos >= 0) ? 1.0f : -1.0f;
                float expSpread = sign * std::pow(std::abs(normalizedPos), 2.0f) * 0.5f;
                voiceLfoRate = baseLfoRate * (1.0f + expSpread);
                break;
            }

            case 2: // Random - use stored random offset to vary rate
                voiceLfoRate = baseLfoRate * (1.0f + voiceRandomOffsets[voice] * 0.4f);
                break;
        }

        // Phase increment for this voice's LFO
        float phaseIncrement = voiceLfoRate / static_cast<float>(currentSampleRate);

        // Get buffer pointers
        auto* inputDataL = numChannels >= 1 ? buffer.getReadPointer(0) : nullptr;
        auto* inputDataR = numChannels >= 2 ? buffer.getReadPointer(1) : nullptr;
        auto* outputDataL = numChannels >= 1 ? unisonBuffer.getWritePointer(0) : nullptr;
        auto* outputDataR = numChannels >= 2 ? unisonBuffer.getWritePointer(1) : nullptr;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Sine LFO (inherently smooth, no discontinuities)
            float lfoValue = std::sin(voiceLfoPhases[voice] * juce::MathConstants<float>::twoPi);

            // Calculate delay time: center + modulation
            // Each voice gets a different modulation direction based on position
            float voiceModDepth = modulationDepthSamples * (1.0f + normalizedPos * 0.2f);
            float delayTime = centerDelaySamples + lfoValue * voiceModDepth;

            // Clamp to valid range
            delayTime = std::max(1.0f, std::min(delayTime, centerDelaySamples * 1.5f));

            // Process left channel
            if (inputDataL && outputDataL)
            {
                unisonDelaysL[voice].setDelay(delayTime);
                unisonDelaysL[voice].pushSample(0, inputDataL[sample]);
                float delayedSample = unisonDelaysL[voice].popSample(0);
                outputDataL[sample] += delayedSample * voicePanL[voice];
            }

            // Process right channel
            if (inputDataR && outputDataR)
            {
                unisonDelaysR[voice].setDelay(delayTime);
                unisonDelaysR[voice].pushSample(0, inputDataR[sample]);
                float delayedSample = unisonDelaysR[voice].popSample(0);
                outputDataR[sample] += delayedSample * voicePanR[voice];
            }

            // Advance LFO phase (wraps naturally, sine is continuous)
            voiceLfoPhases[voice] += phaseIncrement;
            if (voiceLfoPhases[voice] >= 1.0f)
                voiceLfoPhases[voice] -= 1.0f;
        }
    }

    // Normalize by voice count with conservative gain (1/N instead of 1/sqrt(N))
    // Random distribution can cause phase alignment, so we need more headroom
    float gainCompensation = 1.0f / static_cast<float>(activeVoices);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* data = unisonBuffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Apply gain compensation
            float s = data[sample] * gainCompensation;

            // Always-on soft saturation (gentle compression that increases with level)
            // This prevents harsh clipping while preserving dynamics at normal levels
            s = std::tanh(s);

            data[sample] = s;
        }
    }

    //==============================================================================
    // 9. Blend dual engines (crossfade)
    // blend = 0: wobble only, blend = 1: unison only

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* wobbleData = wobbleBuffer.getWritePointer(channel);
        auto* unisonData = unisonBuffer.getReadPointer(channel);
        auto* outputData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float blend = smoothedBlend.getNextValue();
            outputData[sample] = wobbleData[sample] * (1.0f - blend) + unisonData[sample] * blend;
        }
    }

    //==============================================================================
    // 10. Blend with dry signal (final mix)
    dryWetMixer.setWetMixProportion(mixValue);
    dryWetMixer.mixWetSamples(buffer);

    //==============================================================================
    // 11. Width processing (stereo spread)
    float currentWidth = smoothedWidth.getCurrentValue();
    if (std::abs(currentWidth - 100.0f) > 1.0f)  // Not default
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float width = smoothedWidth.getNextValue();
            if (numChannels >= 2)
            {
                float left = buffer.getWritePointer(0)[sample];
                float right = buffer.getWritePointer(1)[sample];
                processWidth(left, right, width);
                buffer.getWritePointer(0)[sample] = left;
                buffer.getWritePointer(1)[sample] = right;
            }
        }
    }

    //==============================================================================
    // 12. Mono Safe processing (prevent phase cancellation)
    if (monoSafe && numChannels >= 2)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float left = buffer.getWritePointer(0)[sample];
            float right = buffer.getWritePointer(1)[sample];
            processMonoSafe(left, right);
            buffer.getWritePointer(0)[sample] = left;
            buffer.getWritePointer(1)[sample] = right;
        }
    }
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
