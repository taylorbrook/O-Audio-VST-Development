#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout AngelGrainAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // delayTime - Float (50-2000ms, default 500, skew 0.5)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayTime", 1 },
        "Delay Time",
        juce::NormalisableRange<float>(50.0f, 2000.0f, 0.1f, 0.5f),
        500.0f,
        "ms"
    ));

    // grainSize - Float (5-500ms, default 100, skew 0.5)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "grainSize", 1 },
        "Grain Size",
        juce::NormalisableRange<float>(5.0f, 500.0f, 0.1f, 0.5f),
        100.0f,
        "ms"
    ));

    // feedback - Float (0-100%, default 30, skew 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1 },
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // chaos - Float (0-100%, default 25, skew 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chaos", 1 },
        "Chaos",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        25.0f,
        "%"
    ));

    // character - Float (0-100%, default 50, skew 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "character", 1 },
        "Character",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        50.0f,
        "%"
    ));

    // mix - Float (0-100%, default 50, skew 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        50.0f,
        "%"
    ));

    // tempoSync - Bool (default true)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "tempoSync", 1 },
        "Tempo Sync",
        true
    ));

    return layout;
}

AngelGrainAudioProcessor::AngelGrainAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

AngelGrainAudioProcessor::~AngelGrainAudioProcessor()
{
}

void AngelGrainAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Setup DSP spec for stereo
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;  // Stereo input/output

    // Prepare grain buffer with stereo spec (preserves stereo field)
    int maxDelaySamples = static_cast<int>(sampleRate * maxDelaySeconds);
    grainBuffer.setMaximumDelayInSamples(maxDelaySamples);
    grainBuffer.prepare(spec);
    grainBuffer.reset();

    // Note: Using manual linear dry/wet mixing instead of DryWetMixer
    // for more intuitive behavior at 50% (full dry + full wet)

    // Pre-calculate Hann window table
    for (int i = 0; i < windowTableSize; ++i)
    {
        float x = static_cast<float>(i) / static_cast<float>(windowTableSize - 1);
        hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * x));
    }

    // Reset all grain voices
    for (auto& voice : grainVoices)
    {
        voice.active = false;
        voice.readPosition = 0.0f;
        voice.windowPosition = 0.0f;
        voice.playbackRate = 1.0f;
        voice.pan = 0.5f;
        voice.grainLengthSamples = 0;
        voice.pitchSemitones = 0;
    }

    // Reset scheduler
    samplesSinceLastGrain = 0;
    writePosition = 0;
    feedbackSampleL = 0.0f;
    feedbackSampleR = 0.0f;

    // Calculate initial grain interval from delayTime parameter
    auto* delayTimeParam = parameters.getRawParameterValue("delayTime");
    float delayTimeMs = delayTimeParam->load();
    nextGrainInterval = static_cast<int>((delayTimeMs / 1000.0f) * sampleRate);

    // Pre-allocate stereo buffers for real-time safety
    wetBuffer.setSize(2, samplesPerBlock);
    dryBuffer.setSize(2, samplesPerBlock);
}

void AngelGrainAudioProcessor::releaseResources()
{
}

void AngelGrainAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();

    // Ensure buffers are large enough (in case host uses different block size)
    if (wetBuffer.getNumSamples() < numSamples)
    {
        wetBuffer.setSize(2, numSamples, false, false, true);
        dryBuffer.setSize(2, numSamples, false, false, true);
    }

    // Read parameters atomically
    auto* delayTimeParam = parameters.getRawParameterValue("delayTime");
    auto* mixParam = parameters.getRawParameterValue("mix");
    auto* feedbackParam = parameters.getRawParameterValue("feedback");
    auto* characterParam = parameters.getRawParameterValue("character");
    auto* chaosParam = parameters.getRawParameterValue("chaos");
    auto* tempoSyncParam = parameters.getRawParameterValue("tempoSync");

    float delayTimeMs = delayTimeParam->load();
    float mixValue = mixParam->load() / 100.0f;
    float feedbackGain = (feedbackParam->load() / 100.0f) * 0.95f;  // Map 0-100% to 0-0.95
    float characterAmount = characterParam->load() / 100.0f;
    float chaosAmount = chaosParam->load() / 100.0f;
    bool tempoSyncEnabled = tempoSyncParam->load() > 0.5f;

    // Tempo sync: quantize delay time to note divisions
    if (tempoSyncEnabled)
    {
        double bpm = 120.0;  // Default BPM

        // Query host for tempo
        if (auto* playHead = getPlayHead())
        {
            if (auto position = playHead->getPosition())
            {
                if (auto bpmOpt = position->getBpm())
                {
                    bpm = *bpmOpt;
                    // Clamp to valid range
                    bpm = juce::jlimit(20.0, 300.0, bpm);
                }
            }
        }

        delayTimeMs = quantizeDelayTimeToTempo(delayTimeMs, bpm);
    }

    // Character morphing: density multiplier (1.0 to 4.0)
    float densityMultiplier = 1.0f + (characterAmount * 3.0f);

    // Calculate spawn interval in samples from delay time with density adjustment
    float baseIntervalSamples = (delayTimeMs / 1000.0f) * static_cast<float>(currentSampleRate);
    nextGrainInterval = static_cast<int>(baseIntervalSamples / densityMultiplier);
    if (nextGrainInterval < 1)
        nextGrainInterval = 1;

    // Calculate Tukey window alpha for character control (0.1 to 1.0)
    float tukeyAlpha = 0.1f + (characterAmount * 0.9f);

    // Get stereo input pointers
    const float* inputL = buffer.getReadPointer(0);
    const float* inputR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : buffer.getReadPointer(0);

    // Clear wet buffer
    wetBuffer.clear();

    // Store dry signal for later mixing (preserve stereo field)
    for (int i = 0; i < numSamples; ++i)
    {
        dryBuffer.setSample(0, i, inputL[i]);
        dryBuffer.setSample(1, i, inputR[i]);
    }

    // Process sample by sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Mix feedback with input before writing to grain buffer (stereo)
        float inputWithFeedbackL = inputL[sample] + feedbackSampleL;
        float inputWithFeedbackR = inputR[sample] + feedbackSampleR;

        // Write to grain buffer (stereo input + feedback)
        grainBuffer.pushSample(0, inputWithFeedbackL);
        grainBuffer.pushSample(1, inputWithFeedbackR);

        // Calculate grain interval with chaos timing jitter
        int currentInterval = nextGrainInterval;
        if (chaosAmount > 0.01f)
        {
            float timingJitter = (random.nextFloat() - 0.5f) * chaosAmount;
            currentInterval = static_cast<int>(nextGrainInterval * (1.0f + timingJitter));
            currentInterval = std::max(1, currentInterval);
        }

        // Check if we should spawn a new grain
        samplesSinceLastGrain++;
        if (samplesSinceLastGrain >= currentInterval && currentInterval > 0)
        {
            spawnGrain();
            samplesSinceLastGrain = 0;
        }

        // Process all active grain voices
        float leftOutput = 0.0f;
        float rightOutput = 0.0f;

        for (auto& voice : grainVoices)
        {
            if (!voice.active)
                continue;

            // Read from delay buffer with interpolation (stereo)
            float delaySamples = std::max(0.0f, voice.readPosition);
            float grainSampleL = grainBuffer.popSample(0, delaySamples, false);
            float grainSampleR = grainBuffer.popSample(1, delaySamples, false);

            // Apply window envelope with Tukey alpha (character control)
            float windowGain = getWindowSample(voice.windowPosition, tukeyAlpha);
            float processedL = grainSampleL * windowGain;
            float processedR = grainSampleR * windowGain;

            // Apply equal-power pan crossfade between stereo channels
            // Pan 0.0 = full left channel, 0.5 = balanced, 1.0 = full right channel
            float leftGain = std::cos(voice.pan * juce::MathConstants<float>::halfPi);
            float rightGain = std::sin(voice.pan * juce::MathConstants<float>::halfPi);

            // Crossfade: at pan=0.5, both channels contribute equally
            // This preserves stereo field while allowing pan randomization
            leftOutput += (processedL * leftGain + processedR * (1.0f - rightGain)) * 0.707f;
            rightOutput += (processedR * rightGain + processedL * (1.0f - leftGain)) * 0.707f;

            // Advance grain playback (decrease delay to read more recent audio)
            voice.readPosition -= voice.playbackRate;

            // Advance window position
            float windowIncrement = 1.0f / static_cast<float>(voice.grainLengthSamples);
            voice.windowPosition += windowIncrement;

            // Check if grain has finished (window complete or read position invalid)
            if (voice.windowPosition >= 1.0f || voice.readPosition < 0.0f)
            {
                voice.active = false;
            }
        }

        // Apply feedback gain and soft saturation (stereo)
        float feedbackL = leftOutput * feedbackGain;
        float feedbackR = rightOutput * feedbackGain;

        // Apply soft saturation (tanh) at high feedback to prevent runaway
        if (feedbackGain > 0.5f)
        {
            feedbackL = std::tanh(feedbackL);
            feedbackR = std::tanh(feedbackR);
        }
        feedbackSampleL = feedbackL;
        feedbackSampleR = feedbackR;

        // Write to wet buffer
        wetBuffer.setSample(0, sample, leftOutput);
        wetBuffer.setSample(1, sample, rightOutput);
    }

    // Linear dry/wet mix (full dry + scaled wet for 0-100%)
    // At 0%: dry only, At 100%: wet only, At 50%: full dry + full wet
    float dryGain = 1.0f - mixValue;  // 1.0 at 0%, 0.0 at 100%
    float wetGain = mixValue;          // 0.0 at 0%, 1.0 at 100%

    for (int i = 0; i < numSamples; ++i)
    {
        float dryL = dryBuffer.getSample(0, i);
        float dryR = dryBuffer.getSample(1, i);
        float wetL = wetBuffer.getSample(0, i);
        float wetR = wetBuffer.getSample(1, i);

        buffer.setSample(0, i, dryL * dryGain + wetL * wetGain);
        buffer.setSample(1, i, dryR * dryGain + wetR * wetGain);
    }
}

juce::AudioProcessorEditor* AngelGrainAudioProcessor::createEditor()
{
    return new AngelGrainAudioProcessorEditor(*this);
}

void AngelGrainAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AngelGrainAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

void AngelGrainAudioProcessor::spawnGrain()
{
    // Find a free voice
    int voiceIndex = findFreeVoice();
    if (voiceIndex < 0)
        return;  // All voices busy

    auto& voice = grainVoices[static_cast<size_t>(voiceIndex)];

    // Read parameters
    auto* grainSizeParam = parameters.getRawParameterValue("grainSize");
    auto* delayTimeParam = parameters.getRawParameterValue("delayTime");
    auto* chaosParam = parameters.getRawParameterValue("chaos");

    float grainSizeMs = grainSizeParam->load();
    float delayTimeMs = delayTimeParam->load();
    float chaosAmount = chaosParam->load() / 100.0f;  // Normalize to 0.0-1.0

    // Calculate grain length in samples
    voice.grainLengthSamples = static_cast<int>((grainSizeMs / 1000.0f) * currentSampleRate);
    if (voice.grainLengthSamples < 1)
        voice.grainLengthSamples = 1;

    // Calculate read position (how far back in the buffer to read)
    // Read from delayTime back in the buffer
    float delayTimeSamples = (delayTimeMs / 1000.0f) * static_cast<float>(currentSampleRate);

    // Apply position randomization based on chaos
    // Formula: position = basePosition * (1.0 + (random - 0.5) * (chaos / 100) * 0.5)
    float positionJitter = (random.nextFloat() - 0.5f) * chaosAmount * 0.5f;
    float basePosition = delayTimeSamples;
    voice.readPosition = basePosition * (1.0f + positionJitter);

    // Ensure we don't read beyond buffer limits
    float maxDelaySamples = static_cast<float>(currentSampleRate * maxDelaySeconds);
    voice.readPosition = juce::jlimit(1.0f, maxDelaySamples - 1.0f, voice.readPosition);

    // Initialize window position
    voice.windowPosition = 0.0f;

    // Pitch quantization to octaves and fifths
    // Select pitch shift based on chaos amount (more chaos = more pitch variation)
    voice.pitchSemitones = selectPitchShift(chaosAmount);
    voice.playbackRate = calculatePlaybackRate(voice.pitchSemitones);

    // Random pan per grain with equal-power pan law
    // Pan spread controlled by chaos: 0% chaos = centered, 100% chaos = full stereo spread
    float panRandomness = (random.nextFloat() - 0.5f) * 2.0f;  // -1.0 to 1.0
    voice.pan = 0.5f + (panRandomness * 0.5f * chaosAmount);
    // Clamp pan to valid range
    voice.pan = juce::jlimit(0.0f, 1.0f, voice.pan);

    voice.active = true;
}

int AngelGrainAudioProcessor::selectPitchShift(float chaosAmount)
{
    // Available pitches: [-12, -7, 0, +7, +12] semitones
    // At low chaos: mostly unison (0)
    // At high chaos: full range of pitch shifts

    if (chaosAmount < 0.01f)
    {
        // No chaos = unison only
        return 0;
    }

    float randomValue = random.nextFloat();

    // Weight toward unison at low chaos, uniform distribution at high chaos
    float threshold = chaosAmount;
    if (randomValue > threshold)
    {
        // Unison (no pitch shift)
        return 0;
    }

    // Select from non-unison pitch options
    // Available: -12, -7, +7, +12
    const int pitchOptions[] = { -12, -7, 7, 12 };
    int index = static_cast<int>(random.nextFloat() * 4.0f);
    index = juce::jlimit(0, 3, index);

    return pitchOptions[index];
}

float AngelGrainAudioProcessor::calculatePlaybackRate(int semitones)
{
    // Playback rate formula: rate = 2^(semitones/12)
    // -12 semitones: 0.5x (octave down)
    // -7 semitones: ~0.667x (fifth down)
    // 0 semitones: 1.0x (unison)
    // +7 semitones: ~1.498x (fifth up)
    // +12 semitones: 2.0x (octave up)
    return std::pow(2.0f, static_cast<float>(semitones) / 12.0f);
}

float AngelGrainAudioProcessor::getWindowSample(float normalizedPosition, float tukeyAlpha)
{
    // Clamp position to valid range
    if (normalizedPosition < 0.0f)
        normalizedPosition = 0.0f;
    if (normalizedPosition >= 1.0f)
        normalizedPosition = 0.9999f;

    // Tukey window formula:
    // - alpha = 0.1: short crossfades (10% on each side), glitchy character
    // - alpha = 1.0: full Hann envelope, smooth character
    float x = normalizedPosition;
    float windowValue;

    if (x < tukeyAlpha / 2.0f)
    {
        // Cosine rise (attack)
        windowValue = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * x / tukeyAlpha));
    }
    else if (x < 1.0f - tukeyAlpha / 2.0f)
    {
        // Flat top
        windowValue = 1.0f;
    }
    else
    {
        // Cosine fall (release)
        windowValue = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (1.0f - x) / tukeyAlpha));
    }

    return windowValue;
}

int AngelGrainAudioProcessor::findFreeVoice()
{
    // Linear search for inactive voice
    for (int i = 0; i < maxGrainVoices; ++i)
    {
        if (!grainVoices[static_cast<size_t>(i)].active)
            return i;
    }

    // All voices busy - steal oldest (voice 0 for simplicity)
    // In future phases, could track age for proper voice stealing
    return 0;
}

float AngelGrainAudioProcessor::quantizeDelayTimeToTempo(float delayTimeMs, double bpm)
{
    // Note division mapping at given BPM
    // At 120 BPM: 1/16 = 125ms, 1/8 = 250ms, 1/4 = 500ms, 1/2 = 1000ms, 1 = 2000ms
    // General formula: ms = (60000 / bpm) * beats

    double msPerBeat = 60000.0 / bpm;

    // Available note divisions (in beats)
    const float noteDivisions[] = {
        0.25f,  // 1/16 note
        0.5f,   // 1/8 note
        1.0f,   // 1/4 note
        2.0f,   // 1/2 note
        4.0f    // 1 (whole) note
    };

    // Find nearest note division
    float closestMs = static_cast<float>(noteDivisions[0] * msPerBeat);
    float closestDiff = std::abs(delayTimeMs - closestMs);

    for (int i = 1; i < 5; ++i)
    {
        float noteMs = static_cast<float>(noteDivisions[i] * msPerBeat);
        float diff = std::abs(delayTimeMs - noteMs);

        if (diff < closestDiff)
        {
            closestDiff = diff;
            closestMs = noteMs;
        }
    }

    // Clamp to valid delay time range (50-2000ms)
    return juce::jlimit(50.0f, 2000.0f, closestMs);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AngelGrainAudioProcessor();
}
