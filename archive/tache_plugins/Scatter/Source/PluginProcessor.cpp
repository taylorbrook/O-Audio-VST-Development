#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout ScatterAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // delay_time - Float (100.0 to 2000.0 ms, default: 500.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delay_time", 1 },
        "Delay Time",
        juce::NormalisableRange<float>(100.0f, 2000.0f, 1.0f, 1.0f),
        500.0f,
        "ms"
    ));

    // grain_size - Float (5.0 to 500.0 ms, default: 100.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "grain_size", 1 },
        "Grain Size",
        juce::NormalisableRange<float>(5.0f, 500.0f, 1.0f, 1.0f),
        100.0f,
        "ms"
    ));

    // density - Float (0.0 to 100.0 %, default: 50.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "density", 1 },
        "Density",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        50.0f,
        "%"
    ));

    // pitch_random - Float (0.0 to 100.0 %, default: 30.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitch_random", 1 },
        "Pitch Random",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // scale - Choice (Chromatic, Major, Minor, Pentatonic, Blues)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "scale", 1 },
        "Scale",
        juce::StringArray { "Chromatic", "Major", "Minor", "Pentatonic", "Blues" },
        0
    ));

    // root_note - Choice (C, C#, D, D#, E, F, F#, G, G#, A, A#, B)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "root_note", 1 },
        "Root Note",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" },
        0
    ));

    // pan_random - Float (0.0 to 100.0 %, default: 75.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pan_random", 1 },
        "Pan Random",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        75.0f,
        "%"
    ));

    // feedback - Float (0.0 to 100.0 %, default: 30.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1 },
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // mix - Float (0.0 to 100.0 %, default: 50.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        50.0f,
        "%"
    ));

    return layout;
}

ScatterAudioProcessor::ScatterAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Phase 3.2: Initialize scale lookup tables
    initializeScaleTables();
}

ScatterAudioProcessor::~ScatterAudioProcessor()
{
}

void ScatterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store sample rate for grain size calculations
    currentSampleRate = sampleRate;

    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare delay buffer (size for maximum delay time: 2000ms)
    auto maxDelayTimeSamples = static_cast<int>(sampleRate * 2.0);  // 2 seconds max
    currentDelayBufferSize = maxDelayTimeSamples;
    delayBuffer.setMaximumDelayInSamples(maxDelayTimeSamples);
    delayBuffer.prepare(spec);
    delayBuffer.reset();

    // Phase 3.3: Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.reset();

    // Phase 3.3: Allocate feedback buffer (stereo)
    feedbackBuffer.setSize(2, samplesPerBlock);
    feedbackBuffer.clear();

    // Initialize grain scheduler
    grainSpawnCounter = 0;
    lastGrainSpawnInterval = 0;

    // Clear all grain voices
    for (auto& grain : grainVoices)
    {
        grain.active = false;
        grain.readPosition = 0.0f;
        grain.windowPosition = 0.0f;
        grain.grainSizeSamples = 0;
        grain.pan = 0.5f;
        grain.reverse = false;
    }
}

void ScatterAudioProcessor::releaseResources()
{
}

void ScatterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic, real-time safe)
    auto* delayTimeParam = parameters.getRawParameterValue("delay_time");
    auto* grainSizeParam = parameters.getRawParameterValue("grain_size");
    auto* densityParam = parameters.getRawParameterValue("density");
    auto* pitchRandomParam = parameters.getRawParameterValue("pitch_random");
    auto* scaleParam = parameters.getRawParameterValue("scale");
    auto* rootNoteParam = parameters.getRawParameterValue("root_note");
    auto* panRandomParam = parameters.getRawParameterValue("pan_random");
    auto* feedbackParam = parameters.getRawParameterValue("feedback");
    auto* mixParam = parameters.getRawParameterValue("mix");

    float delayTimeMs = delayTimeParam->load();
    float grainSizeMs = grainSizeParam->load();
    float densityPercent = densityParam->load();
    float pitchRandomPercent = pitchRandomParam->load();
    int scaleIndex = static_cast<int>(scaleParam->load());
    int rootNote = static_cast<int>(rootNoteParam->load());
    float panRandomPercent = panRandomParam->load();
    float feedbackGain = feedbackParam->load() / 100.0f * 0.95f;  // Map 0-100% to 0.0-0.95
    float mixValue = mixParam->load() / 100.0f;  // Map 0-100% to 0.0-1.0

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Phase 3.3: Step 1 - Capture dry signal
    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.pushDrySamples(block);

    // Phase 3.3: Step 2 - Mix feedback signal with input
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* feedbackData = feedbackBuffer.getReadPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelData[sample] += feedbackData[sample];
        }
    }

    // Phase 3.3: Step 3 - Write input + feedback to delay buffer (stereo)
    for (int channel = 0; channel < juce::jmin(2, numChannels); ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            delayBuffer.pushSample(channel, channelData[sample]);
        }
    }

    // Phase 3.3: Step 4 - Update grain scheduler and spawn grains
    updateGrainScheduler(densityPercent, grainSizeMs, pitchRandomPercent, panRandomPercent, scaleIndex, rootNote);

    // Phase 3.3: Step 5 - Process active grain voices (stereo output)
    processGrainVoices(buffer);

    // Phase 3.3: Step 6 - Apply feedback gain and store for next cycle
    feedbackBuffer.clear();
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* wetData = buffer.getReadPointer(channel);
        auto* feedbackData = feedbackBuffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            feedbackData[sample] = wetData[sample] * feedbackGain;
        }
    }

    // Phase 3.3: Step 7 - Blend with dry signal using dry/wet mixer
    dryWetMixer.setWetMixProportion(mixValue);
    dryWetMixer.mixWetSamples(block);
}

juce::AudioProcessorEditor* ScatterAudioProcessor::createEditor()
{
    return new ScatterAudioProcessorEditor(*this);
}

void ScatterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ScatterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ScatterAudioProcessor();
}

// ============================================================================
// Phase 4.2: Grain Visualization Data Accessor
// ============================================================================

std::vector<ScatterAudioProcessor::GrainVisualizationData> ScatterAudioProcessor::getActiveGrainPositions() const
{
    std::vector<GrainVisualizationData> data;

    // Copy active grain positions (thread-safe read - audio thread writes, message thread reads)
    for (const auto& grain : grainVoices)
    {
        if (grain.active)
        {
            GrainVisualizationData vizData;

            // X-axis: Normalized time position in delay buffer (0.0-1.0)
            vizData.x = grain.readPosition / static_cast<float>(currentDelayBufferSize);

            // Y-axis: Pitch shift normalized to -1.0 to +1.0 range
            // playbackRate = 2^(semitones / 12)
            // Reverse calculation: semitones = 12 * log2(playbackRate)
            float semitones = 12.0f * std::log2(grain.playbackRate);
            vizData.y = semitones / 7.0f;  // Normalize to -1.0 to +1.0 (-7 to +7 semitones)

            // Pan position (already 0.0-1.0)
            vizData.pan = grain.pan;

            data.push_back(vizData);
        }
    }

    return data;
}

// ============================================================================
// Phase 3.1: Core Granular Engine Helper Methods
// ============================================================================

void ScatterAudioProcessor::generateHannWindow(int sizeInSamples)
{
    hannWindow.resize(sizeInSamples);
    windowTableSize = sizeInSamples;

    // Generate Hann window: hann[n] = 0.5 * (1 - cos(2 * pi * n / N))
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        hannWindow.data(),
        sizeInSamples,
        juce::dsp::WindowingFunction<float>::hann,
        false  // Not normalized (we want 0-1 range)
    );
}

void ScatterAudioProcessor::spawnNewGrain(float grainSizeMs, float pitchRandomPercent, float panRandomPercent, int scaleIndex, int rootNote)
{
    // Convert grain size from ms to samples
    int grainSizeSamples = static_cast<int>(currentSampleRate * grainSizeMs / 1000.0f);

    // Clamp to valid range (avoid zero or negative sizes)
    grainSizeSamples = juce::jmax(1, grainSizeSamples);

    // Find inactive voice (voice allocation)
    GrainVoice* availableVoice = nullptr;

    for (auto& grain : grainVoices)
    {
        if (!grain.active)
        {
            availableVoice = &grain;
            break;
        }
    }

    // If no voices available, steal the first voice (simple voice stealing)
    if (availableVoice == nullptr)
    {
        availableVoice = &grainVoices[0];
    }

    // Get random number generator
    auto& random = juce::Random::getSystemRandom();

    // Phase 3.2: Generate random pitch and quantize to scale
    float randomPitch = (random.nextFloat() * 2.0f - 1.0f) * 7.0f * (pitchRandomPercent / 100.0f);
    int quantizedPitch = quantizePitchToScale(randomPitch, scaleIndex, rootNote);
    float playbackRate = std::pow(2.0f, quantizedPitch / 12.0f);

    // Phase 3.3: Generate random pan position (0.0 = left, 1.0 = right)
    float basePan = 0.5f;  // Center
    float randomPan = random.nextFloat();  // 0.0-1.0
    float panAmount = (randomPan - 0.5f) * (panRandomPercent / 100.0f);  // Scaled by parameter
    float pan = juce::jlimit(0.0f, 1.0f, basePan + panAmount);

    // Phase 3.3: Random reverse playback (50/50 probability)
    bool reverse = random.nextBool();

    // Initialize grain voice
    availableVoice->active = true;
    availableVoice->grainSizeSamples = grainSizeSamples;
    availableVoice->windowPosition = 0.0f;
    availableVoice->playbackRate = playbackRate;
    availableVoice->pan = pan;
    availableVoice->reverse = reverse;

    // Read position: Start at current delay buffer write position
    availableVoice->readPosition = 0.0f;

    // Generate Hann window for this grain size (if not already cached)
    if (windowTableSize != grainSizeSamples)
    {
        generateHannWindow(grainSizeSamples);
    }
}

void ScatterAudioProcessor::updateGrainScheduler(float densityPercent, float grainSizeMs, float pitchRandomPercent, float panRandomPercent, int scaleIndex, int rootNote)
{
    // Grain spawn interval calculation: grainSizeSamples / (density * overlapFactor)
    // At 50% density, grains spawn at ~grainSize intervals (moderate overlap)
    // At 100% density, grains spawn more frequently (dense cloud)

    const float overlapFactor = 2.0f;  // Tuning constant for overlap behavior
    int grainSizeSamples = static_cast<int>(currentSampleRate * grainSizeMs / 1000.0f);
    grainSizeSamples = juce::jmax(1, grainSizeSamples);

    // Calculate spawn interval (avoid division by zero)
    float densityNormalized = juce::jmax(0.01f, densityPercent / 100.0f);
    int spawnInterval = static_cast<int>(grainSizeSamples / (densityNormalized * overlapFactor));
    spawnInterval = juce::jmax(1, spawnInterval);  // At least 1 sample

    lastGrainSpawnInterval = spawnInterval;

    // Increment sample counter
    grainSpawnCounter++;

    // Check if it's time to spawn a new grain
    if (grainSpawnCounter >= spawnInterval)
    {
        spawnNewGrain(grainSizeMs, pitchRandomPercent, panRandomPercent, scaleIndex, rootNote);
        grainSpawnCounter = 0;  // Reset counter
    }
}

void ScatterAudioProcessor::processGrainVoices(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Clear output buffer (grains will be summed into it)
    buffer.clear();

    // Process each active grain voice
    for (auto& grain : grainVoices)
    {
        if (!grain.active)
            continue;

        // For each sample in the buffer
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Check if grain has completed
            if (grain.windowPosition >= 1.0f)
            {
                grain.active = false;
                break;
            }

            // Calculate window index (map 0.0-1.0 to 0..grainSizeSamples-1)
            int windowIndex = static_cast<int>(grain.windowPosition * grain.grainSizeSamples);
            windowIndex = juce::jlimit(0, grain.grainSizeSamples - 1, windowIndex);

            // Get window envelope value (or 1.0 if table not generated yet)
            float windowValue = 1.0f;
            if (windowIndex < static_cast<int>(hannWindow.size()))
            {
                windowValue = hannWindow[windowIndex];
            }

            // Phase 3.3: Read from delay buffer (stereo, with channel selection)
            // Use channel 0 for mono-like grain source (could randomize per grain in future)
            float delaySamples = grain.readPosition;
            float delayedSample = delayBuffer.popSample(0, delaySamples);

            // Apply window envelope
            float grainOutput = delayedSample * windowValue;

            // Phase 3.3: Apply stereo panning
            float leftGain = 1.0f - grain.pan;   // pan=0.0 → leftGain=1.0, pan=1.0 → leftGain=0.0
            float rightGain = grain.pan;          // pan=0.0 → rightGain=0.0, pan=1.0 → rightGain=1.0

            // Sum to output buffer (stereo)
            if (numChannels >= 2)
            {
                auto* leftData = buffer.getWritePointer(0);
                auto* rightData = buffer.getWritePointer(1);

                leftData[sample] += grainOutput * leftGain;
                rightData[sample] += grainOutput * rightGain;
            }
            else if (numChannels == 1)
            {
                // Mono output: mix both channels
                auto* outputData = buffer.getWritePointer(0);
                outputData[sample] += grainOutput;
            }

            // Advance grain window position (always at rate 1.0 - envelope progresses normally)
            grain.windowPosition += 1.0f / grain.grainSizeSamples;

            // Phase 3.3: Advance read position by playback rate (forward or reverse)
            if (grain.reverse)
            {
                grain.readPosition -= grain.playbackRate;

                // Wrap around if negative
                if (grain.readPosition < 0.0f)
                {
                    grain.readPosition += currentDelayBufferSize;
                }
            }
            else
            {
                grain.readPosition += grain.playbackRate;

                // Wrap around if exceeds buffer size
                if (grain.readPosition >= currentDelayBufferSize)
                {
                    grain.readPosition -= currentDelayBufferSize;
                }
            }
        }
    }
}

// ============================================================================
// Phase 3.2: Pitch Shifting + Scale Quantization Helper Methods
// ============================================================================

void ScatterAudioProcessor::initializeScaleTables()
{
    // Scale 0: Chromatic (all 12 semitones - no quantization)
    scaleIntervals[0] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    // Scale 1: Major scale (Ionian mode)
    scaleIntervals[1] = {0, 2, 4, 5, 7, 9, 11};

    // Scale 2: Natural Minor scale (Aeolian mode)
    scaleIntervals[2] = {0, 2, 3, 5, 7, 8, 10};

    // Scale 3: Pentatonic scale (Major pentatonic)
    scaleIntervals[3] = {0, 2, 4, 7, 9};

    // Scale 4: Blues scale
    scaleIntervals[4] = {0, 3, 5, 6, 7, 10};
}

int ScatterAudioProcessor::quantizePitchToScale(float pitchSemitones, int scaleIndex, int rootNote)
{
    // Clamp scale index to valid range
    scaleIndex = juce::jlimit(0, numScales - 1, scaleIndex);

    // Clamp root note to valid range (0-11 semitones)
    rootNote = juce::jlimit(0, 11, rootNote);

    // Get the selected scale intervals
    const auto& scale = scaleIntervals[scaleIndex];

    // Round pitch to nearest semitone
    int pitchInt = static_cast<int>(std::round(pitchSemitones));

    // Extract octave and note within octave
    int octave = pitchInt / 12;
    int semitone = pitchInt % 12;

    // Handle negative semitones (wrap to positive octave)
    if (semitone < 0)
    {
        semitone += 12;
        octave -= 1;
    }

    // Find nearest scale degree to the semitone
    int nearestScaleDegree = scale[0];
    int minDistance = std::abs(semitone - scale[0]);

    for (int scaleDegree : scale)
    {
        int distance = std::abs(semitone - scaleDegree);
        if (distance < minDistance)
        {
            minDistance = distance;
            nearestScaleDegree = scaleDegree;
        }
    }

    // Reconstruct quantized pitch: octave + scale degree + root transposition
    int quantizedPitch = (octave * 12) + nearestScaleDegree + rootNote;

    // Clamp to ±12 semitones (prevent extreme playback rates)
    quantizedPitch = juce::jlimit(-12, 12, quantizedPitch);

    return quantizedPitch;
}
