#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DrumRouletteAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global parameter: RANDOMIZE_ALL (1 parameter)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "RANDOMIZE_ALL", 1 },
        "Randomize All",
        false
    ));

    // Per-slot parameters (9 × 8 = 72 parameters)
    for (int slot = 1; slot <= 8; ++slot)
    {
        juce::String slotNum = juce::String(slot);

        // RANDOMIZE_N - Button trigger
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "RANDOMIZE_" + slotNum, 1 },
            "Randomize " + slotNum,
            false
        ));

        // LOCK_N - Toggle
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "LOCK_" + slotNum, 1 },
            "Lock " + slotNum,
            false
        ));

        // VOLUME_N - Fader (-60.0 to +6.0 dB, skew 2.0)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "VOLUME_" + slotNum, 1 },
            "Volume " + slotNum,
            juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f, 2.0f),
            0.0f,
            "dB"
        ));

        // DECAY_N - Rotary (10.0 to 2000.0 ms, skew 0.5)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "DECAY_" + slotNum, 1 },
            "Decay " + slotNum,
            juce::NormalisableRange<float>(10.0f, 2000.0f, 0.1f, 0.5f),
            500.0f,
            "ms"
        ));

        // ATTACK_N - Rotary (0.0 to 50.0 ms, skew 0.5)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "ATTACK_" + slotNum, 1 },
            "Attack " + slotNum,
            juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f, 0.5f),
            1.0f,
            "ms"
        ));

        // TILT_FILTER_N - Rotary (-12.0 to +12.0 dB, linear)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "TILT_FILTER_" + slotNum, 1 },
            "Tilt " + slotNum,
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f, 1.0f),
            0.0f,
            "dB"
        ));

        // PITCH_N - Rotary (-12.0 to +12.0 semitones, linear)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "PITCH_" + slotNum, 1 },
            "Pitch " + slotNum,
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f, 1.0f),
            0.0f,
            "st"
        ));

        // SOLO_N - Toggle
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "SOLO_" + slotNum, 1 },
            "Solo " + slotNum,
            false
        ));

        // MUTE_N - Toggle
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "MUTE_" + slotNum, 1 },
            "Mute " + slotNum,
            false
        ));
    }

    return layout;
}

juce::AudioProcessor::BusesProperties DrumRouletteAudioProcessor::createBusesLayout()
{
    // Multi-output configuration: 9 stereo buses (18 channels total)
    // Bus 0: Main Output (stereo)
    // Bus 1-8: Individual Slot Outputs (stereo each)
    BusesProperties props;
    props = props.withOutput("Main Output", juce::AudioChannelSet::stereo(), true);

    for (int slot = 1; slot <= 8; ++slot)
    {
        juce::String busName = "Slot " + juce::String(slot) + " Output";
        props = props.withOutput(busName, juce::AudioChannelSet::stereo(), true);
    }

    return props;
}

DrumRouletteAudioProcessor::DrumRouletteAudioProcessor()
    : AudioProcessor(createBusesLayout())
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Register audio formats (WAV, AIFF, MP3, AAC)
    formatManager.registerBasicFormats();

    // Create 8 voices (one per slot, mapped to MIDI notes C1-G1)
    for (size_t slot = 0; slot < 8; ++slot)
    {
        auto* voice = new DrumRouletteVoice(static_cast<int>(slot + 1));
        voices[slot] = voice;
        synthesiser.addVoice(voice);

        // Pass parameter pointers to voice (Phase 4.2 + 4.3)
        juce::String slotNum = juce::String(static_cast<int>(slot + 1));
        auto* attackParam = parameters.getRawParameterValue("ATTACK_" + slotNum);
        auto* decayParam = parameters.getRawParameterValue("DECAY_" + slotNum);
        auto* pitchParam = parameters.getRawParameterValue("PITCH_" + slotNum);
        auto* tiltParam = parameters.getRawParameterValue("TILT_FILTER_" + slotNum);
        auto* volumeParam = parameters.getRawParameterValue("VOLUME_" + slotNum);

        voice->setParameterPointers(attackParam, decayParam, pitchParam, tiltParam, volumeParam);

        // Phase 4.4: Get parameter pointers for solo/mute/lock/randomize
        lockParams[slot] = parameters.getRawParameterValue("LOCK_" + slotNum);
        soloParams[slot] = parameters.getRawParameterValue("SOLO_" + slotNum);
        muteParams[slot] = parameters.getRawParameterValue("MUTE_" + slotNum);
        randomizeParams[slot] = parameters.getRawParameterValue("RANDOMIZE_" + slotNum);

        // Pass solo/mute pointers to voice
        voice->setSoloMutePointers(soloParams[slot], muteParams[slot], &anySoloActive);
    }

    // Phase 4.4: Get global randomize parameter
    randomizeAllParam = parameters.getRawParameterValue("RANDOMIZE_ALL");

    // Phase 4.4: Register parameter listeners for button triggers
    for (int slot = 1; slot <= 8; ++slot)
    {
        juce::String slotNum = juce::String(slot);
        parameters.addParameterListener("RANDOMIZE_" + slotNum, this);
    }
    parameters.addParameterListener("RANDOMIZE_ALL", this);

    // Add 8 sounds (one per MIDI note C1-G1)
    // MIDI note mapping: C1 (36) → Slot 1, C#1 (37) → Slot 2, ..., G1 (43) → Slot 8
    for (int midiNote = 36; midiNote <= 43; ++midiNote)
    {
        synthesiser.addSound(new DrumRouletteSound(midiNote));
    }
}

DrumRouletteAudioProcessor::~DrumRouletteAudioProcessor()
{
    // Phase 4.4: Remove parameter listeners
    for (int slot = 1; slot <= 8; ++slot)
    {
        juce::String slotNum = juce::String(slot);
        parameters.removeParameterListener("RANDOMIZE_" + slotNum, this);
    }
    parameters.removeParameterListener("RANDOMIZE_ALL", this);
}

void DrumRouletteAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare synthesiser with current sample rate
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    juce::ignoreUnused(samplesPerBlock);
}

void DrumRouletteAudioProcessor::releaseResources()
{
    // Optional: Release resources when plugin not in use
}

void DrumRouletteAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear all output buses
    for (int busIndex = 0; busIndex < getBusCount(false); ++busIndex)
    {
        auto busBuffer = getBusBuffer(buffer, false, busIndex);
        busBuffer.clear();
    }

    // Phase 4.4: Update anySoloActive flag for voice access
    anySoloActive = false;
    for (int slot = 0; slot < 8; ++slot)
    {
        if (soloParams[slot] != nullptr && soloParams[slot]->load() > 0.5f)
        {
            anySoloActive = true;
            break;
        }
    }

    // Get main output buffer (Bus 0)
    auto mainBuffer = getBusBuffer(buffer, false, 0);

    // Render synthesiser to main output
    // Note: Voices check solo/mute state in their renderNextBlock via shouldRenderToMainMix()
    // This implementation renders all voices but voices can apply their own gain scaling
    synthesiser.renderNextBlock(mainBuffer, midiMessages, 0, mainBuffer.getNumSamples());

    // Copy individual slot outputs (Bus 1-8)
    // Individual outputs are always active regardless of solo/mute
    for (int slot = 0; slot < 8; ++slot)
    {
        int busIndex = slot + 1;  // Bus 1-8 for slots 1-8

        if (busIndex < getBusCount(false))
        {
            auto slotBuffer = getBusBuffer(buffer, false, busIndex);

            // Copy main output to individual slot output
            // Phase 4.4: Individual outputs bypass solo/mute (always active)
            for (int channel = 0; channel < slotBuffer.getNumChannels(); ++channel)
            {
                if (channel < mainBuffer.getNumChannels())
                {
                    slotBuffer.copyFrom(channel, 0, mainBuffer, channel, 0, slotBuffer.getNumSamples());
                }
            }
        }
    }
}

bool DrumRouletteAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Validate multi-output bus configuration
    // Must have 9 output buses (1 main + 8 individual), all stereo

    if (layouts.outputBuses.size() != 9)
        return false;

    // All buses must be stereo
    for (const auto& bus : layouts.outputBuses)
    {
        if (bus != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void DrumRouletteAudioProcessor::loadSampleForSlot(int slotIndex, const juce::File& file)
{
    // slotIndex is 1-based (1-8)
    if (slotIndex < 1 || slotIndex > 8)
        return;

    size_t voiceIndex = static_cast<size_t>(slotIndex - 1);  // Convert to 0-based for array access

    if (voices[voiceIndex] != nullptr)
    {
        voices[voiceIndex]->loadSample(file);
    }
}

void DrumRouletteAudioProcessor::setFolderPathForSlot(int slotIndex, const juce::String& path)
{
    // slotIndex is 1-based (1-8)
    if (slotIndex < 1 || slotIndex > 8)
        return;

    size_t index = static_cast<size_t>(slotIndex - 1);
    folderPaths[index] = path;
}

juce::String DrumRouletteAudioProcessor::getFolderPathForSlot(int slotIndex) const
{
    // slotIndex is 1-based (1-8)
    if (slotIndex < 1 || slotIndex > 8)
        return {};

    size_t index = static_cast<size_t>(slotIndex - 1);
    return folderPaths[index];
}

void DrumRouletteAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Phase 4.4: Handle button triggers (buttons are momentary - newValue > 0.5 means pressed)
    if (newValue < 0.5f)
        return;  // Button released, ignore

    // Check for RANDOMIZE_N buttons (1-8)
    for (int slot = 1; slot <= 8; ++slot)
    {
        juce::String slotNum = juce::String(slot);
        if (parameterID == "RANDOMIZE_" + slotNum)
        {
            randomizeSample(slot);
            return;
        }
    }

    // Check for RANDOMIZE_ALL button
    if (parameterID == "RANDOMIZE_ALL")
    {
        randomizeAllUnlockedSlots();
    }
}

void DrumRouletteAudioProcessor::randomizeSample(int slotIndex)
{
    // slotIndex is 1-based (1-8)
    if (slotIndex < 1 || slotIndex > 8)
        return;

    size_t index = static_cast<size_t>(slotIndex - 1);

    // Check if folder path is set
    if (folderPaths[index].isEmpty())
    {
        DBG("Folder path not set for slot " << slotIndex);
        return;
    }

    // Thread-safe file I/O: Defer to message thread
    juce::MessageManager::callAsync([this, slotIndex, index]()
    {
        juce::File folder(folderPaths[index]);

        if (!folder.exists() || !folder.isDirectory())
        {
            DBG("Invalid folder path for slot " << slotIndex << ": " << folderPaths[index]);
            return;
        }

        // Find all audio files recursively
        juce::Array<juce::File> audioFiles = folder.findChildFiles(
            juce::File::findFiles,
            true,  // Search recursively
            "*.wav;*.aiff;*.aif;*.mp3;*.m4a");

        if (audioFiles.isEmpty())
        {
            DBG("No audio files found in folder for slot " << slotIndex);
            return;
        }

        // Select random file
        int randomIndex = juce::Random::getSystemRandom().nextInt(audioFiles.size());
        juce::File selectedFile = audioFiles[randomIndex];

        DBG("Loading random sample for slot " << slotIndex << ": " << selectedFile.getFileName());

        // Load sample
        loadSampleForSlot(slotIndex, selectedFile);
    });
}

void DrumRouletteAudioProcessor::randomizeAllUnlockedSlots()
{
    // Iterate all slots and randomize if not locked
    for (int slot = 0; slot < 8; ++slot)
    {
        // Check LOCK parameter (if true, skip)
        if (lockParams[slot] != nullptr && lockParams[slot]->load() > 0.5f)
        {
            DBG("Slot " << (slot + 1) << " is locked, skipping randomization");
            continue;
        }

        // Randomize this slot
        randomizeSample(slot + 1);  // Convert to 1-based
    }
}

juce::AudioProcessorEditor* DrumRouletteAudioProcessor::createEditor()
{
    return new DrumRouletteAudioProcessorEditor(*this);
}

void DrumRouletteAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Phase 4.4: Add folder paths to state tree
    for (int slot = 0; slot < 8; ++slot)
    {
        juce::String propName = "folderPath" + juce::String(slot + 1);
        state.setProperty(propName, folderPaths[slot], nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DrumRouletteAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xmlState);
        parameters.replaceState(state);

        // Phase 4.4: Restore folder paths from state tree
        for (int slot = 0; slot < 8; ++slot)
        {
            juce::String propName = "folderPath" + juce::String(slot + 1);
            if (state.hasProperty(propName))
            {
                folderPaths[slot] = state.getProperty(propName).toString();
            }
        }
    }
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumRouletteAudioProcessor();
}
