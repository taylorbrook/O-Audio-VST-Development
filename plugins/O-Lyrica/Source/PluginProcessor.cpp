/*
  ==============================================================================

    O-Lyrica - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OLyricaAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Core Sound Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterVolume", 1 },
        "Master Volume",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f),
        0.0f,
        "dB"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "stringMaterial", 1 },
        "String Material",
        juce::StringArray { "Gut", "Nylon", "Wire", "Carbon", "Metal Alloy", "Glass", "Crystal", "Energy" },
        1  // Default: Nylon
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "brightness", 1 },
        "Brightness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "timbre", 1 },
        "Timbre",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "decayTime", 1 },
        "Decay Time",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f, 0.4f),  // Skewed for finer control at low values
        5.0f,
        "s"
    ));

    // Body Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodySize", 1 },
        "Body Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodyResonance", 1 },
        "Body Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.6f
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "woodType", 1 },
        "Wood Type",
        juce::StringArray { "Spruce", "Maple", "Exotic", "Synthetic" },
        0  // Default: Spruce
    ));

    // Sympathetic Resonance
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sympatheticAmount", 1 },
        "Sympathetic Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f
    ));

    // Pluck Mechanics
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pluckPosition", 1 },
        "Pluck Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "fingerHardness", 1 },
        "Finger Hardness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // Expression
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "technique", 1 },
        "Technique",
        juce::StringArray { "Normal", "Harmonic", "Muted", "Près de la table" },
        0  // Default: Normal
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoMode", 1 },
        "Glissando Mode",
        juce::StringArray { "Off", "Free", "Scale-Locked" },
        0  // Default: Off
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoScale", 1 },
        "Glissando Scale",
        juce::StringArray { "Major", "Minor", "Pentatonic", "Custom" },
        0  // Default: Major
    ));

    // v1.21.0: Glissando speed for scale-locked mode (notes per second)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "glissandoSpeed", 1 },
        "Glissando Speed",
        juce::NormalisableRange<float>(4.0f, 30.0f, 0.1f, 0.5f),
        12.0f,
        "n/s"
    ));

    // v1.22.0: Glissando shape — acceleration curve for scale-locked note spacing
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoShape", 1 },
        "Glissando Shape",
        juce::StringArray { "Linear", "Accelerate", "Decelerate", "S-Curve" },
        3  // Default: S-Curve (most natural harp feel)
    ));

    // Tuning
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterTune", 1 },
        "Master Tune",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f,
        "Hz"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchBendRange", 1 },
        "Pitch Bend Range",
        juce::NormalisableRange<float>(1.0f, 48.0f, 1.0f),
        2.0f,
        "st"
    ));

    // v1.6.0: Tuning Mode (12-TET, Custom/Scala, or MTS-ESP)
    // v1.7.2: Added MTS-ESP option to match UI buttons
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tuningMode", 1 },
        "Tuning Mode",
        juce::StringArray { "12-TET", "Custom", "MTS-ESP" },
        0  // Default: 12-TET
    ));

    // v1.9.0: Octave Stretch (0.95-1.25, physical modeling enhancement)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "octaveStretch", 1 },
        "Octave Stretch",
        juce::NormalisableRange<float>(0.95f, 1.25f, 0.01f),
        1.0f
    ));

    // v1.9.0: Temperament Preset Selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "temperamentPreset", 1 },
        "Temperament Preset",
        juce::StringArray {
            "Equal 12-TET", "Pythagorean", "Zarlino", "Meantone (1/4)",
            "Werckmeister III", "Kirnberger III", "Vallotti",
            "Well Tempered", "Just Intonation", "Bohlen-Pierce", "Custom"
        },
        0  // Default: Equal 12-TET
    ));

    // Advanced String Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringTension", 1 },
        "String Tension",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringGauge", 1 },
        "String Gauge",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringLength", 1 },
        "String Length",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringStiffness", 1 },
        "String Stiffness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f
    ));

    // v1.3.0: Advanced Physical Modeling Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attackNoise", 1 },
        "Attack Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f  // Default: Material-typical noise amount
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sympatheticQ", 1 },
        "Sympathetic Sharpness",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f, 0.5f),  // Skewed for finer control at low Q
        5.0f  // Default: Moderate resonance sharpness
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodyModeSpread", 1 },
        "Body Mode Spread",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        0.0f  // Default: No spread (original tuning)
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bridgeBrightness", 1 },
        "Bridge Brightness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f  // Default: Neutral bridge reflection
    ));

    // v1.19.0: Humanize - per-note randomization for realistic variation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "humanize", 1 },
        "Humanize",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f  // Default: No humanization (deterministic behavior)
    ));

    return layout;
}

OLyricaAudioProcessor::OLyricaAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Lyrica")
{
    // Initialize synthesiser with 32 voices (v1.18.4: increased from 16)
    for (int i = 0; i < 32; ++i)
    {
        auto* voice = new HarpSynthVoice();
        voice->setAPVTS(&parameters);
        voice->setSympatheticEngine(&sympatheticEngine); // Phase 2.7: Connect sympathetic engine
        voice->setTuningEngine(&tuningEngine); // Phase 2.8: Connect tuning engine
        synthesiser.addVoice(voice);
    }

    // Add sound that accepts all MIDI notes
    synthesiser.addSound(new HarpSynthSound());

    // v1.12.0: Set up custom state callbacks for tuning persistence
    // v1.18.3: Removed verbose DBG logging (was development diagnostics)
    presetManager.setCustomStateCallbacks(
        // Save callback - returns tuning state as JSON
        [this]() -> juce::var {
            auto* obj = new juce::DynamicObject();

            // Save intervals
            auto intervals = tuningEngine.getIntervals();
            juce::Array<juce::var> intervalsArray;
            for (double cents : intervals)
                intervalsArray.add(cents);
            obj->setProperty("intervals", intervalsArray);

            // Save scale name
            obj->setProperty("scaleName", tuningEngine.getActiveTuningName());

            // Save tonic (note: also saved directly to XML as workaround for CustomState bug)
            obj->setProperty("tonic", juce::var(tuningEngine.getTonicNote()));

            // Save built-in preset index
            obj->setProperty("presetIndex", static_cast<int>(tuningEngine.getBuiltInPreset()));

            // Save octave stretch
            obj->setProperty("octaveStretch", tuningEngine.getOctaveStretch());

            // Save tuning mode explicitly
            obj->setProperty("tuningMode", static_cast<int>(tuningEngine.getMode()));

            return juce::var(obj);
        },
        // Load callback - restores tuning state from JSON
        [this](const juce::var& customState) {
            if (!customState.isObject())
                return;

            auto* obj = customState.getDynamicObject();
            if (obj == nullptr)
                return;

            // Restore preset index first (this sets intervals for built-in presets)
            if (obj->hasProperty("presetIndex"))
            {
                int presetIdx = static_cast<int>(obj->getProperty("presetIndex"));
                tuningEngine.setBuiltInPreset(
                    static_cast<TuningEngine::BuiltInPreset>(presetIdx));
            }

            // If custom intervals were saved, restore them (overrides preset)
            if (obj->hasProperty("intervals"))
            {
                auto intervalsVar = obj->getProperty("intervals");
                if (intervalsVar.isArray())
                {
                    std::vector<double> intervals;
                    for (int i = 0; i < intervalsVar.size(); ++i)
                        intervals.push_back(static_cast<double>(intervalsVar[i]));

                    juce::String name = obj->getProperty("scaleName").toString();
                    if (name.isEmpty()) name = "Custom";

                    tuningEngine.setCustomIntervals(intervals, name);
                }
            }

            // Restore tuning mode
            if (obj->hasProperty("tuningMode"))
            {
                int mode = static_cast<int>(obj->getProperty("tuningMode"));
                tuningEngine.setMode(static_cast<TuningEngine::Mode>(mode));

                // v1.13.3: Also update APVTS parameter to prevent processBlock from resetting
                if (auto* tuningModeParam = parameters.getParameter("tuningMode"))
                    tuningModeParam->setValueNotifyingHost(static_cast<float>(mode) / 2.0f);
            }

            // Restore tonic (do this AFTER setting intervals so rotation works)
            if (obj->hasProperty("tonic"))
            {
                int tonic = static_cast<int>(obj->getProperty("tonic"));
                tuningEngine.setTonicNote(tonic);
            }

            // Restore octave stretch
            if (obj->hasProperty("octaveStretch"))
            {
                float stretch = static_cast<float>(obj->getProperty("octaveStretch"));
                tuningEngine.setOctaveStretch(stretch);
            }
        }
    );
}

OLyricaAudioProcessor::~OLyricaAudioProcessor()
{
}

void OLyricaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    // v1.5.0: Initialize factory presets on first run (only creates if not exist)
    initializeFactoryPresets();

    // Phase 2.7: Prepare sympathetic resonance engine
    sympatheticEngine.prepare(sampleRate, samplesPerBlock);

    // Prepare all voices
    // v1.3.2: Use static_cast - all voices are HarpSynthVoice (we control voice creation)
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        auto* voice = static_cast<HarpSynthVoice*>(synthesiser.getVoice(i));
        voice->prepare(sampleRate, samplesPerBlock);
    }
}

void OLyricaAudioProcessor::releaseResources()
{
    // Release any resources when playback stops
}

void OLyricaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer
    buffer.clear();

    // v1.18.3: Removed redundant null checks - APVTS guarantees non-null for registered params
    // (Pattern matches HarpSynthVoice cleanup from v1.3.2)

    // Phase 2.7: Update sympathetic resonance parameters
    sympatheticEngine.setIntensity(parameters.getRawParameterValue("sympatheticAmount")->load());
    sympatheticEngine.setResonatorQ(parameters.getRawParameterValue("sympatheticQ")->load());

    // Phase 2.8: Update tuning engine parameters
    tuningEngine.setMasterTune(static_cast<double>(parameters.getRawParameterValue("masterTune")->load()));
    tuningEngine.setPitchBendRange(parameters.getRawParameterValue("pitchBendRange")->load());

    // v1.6.0: Update tuning mode
    // v1.13.3: Skip mode sync during state restoration to prevent race condition
    if (!isRestoringState.load(std::memory_order_acquire))
    {
        int modeInt = static_cast<int>(parameters.getRawParameterValue("tuningMode")->load());
        tuningEngine.setMode(static_cast<TuningEngine::Mode>(modeInt));
    }

    // v1.9.0: Update octave stretch
    tuningEngine.setOctaveStretch(parameters.getRawParameterValue("octaveStretch")->load());

    // v1.3.2: Sync sympathetic coupling matrix at block boundary (thread-safe)
    sympatheticEngine.syncBeforeBlock();

    // v1.7.9: Push MIDI note events to queue for UI visualization (tuning circle flash)
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            midiEventQueue.push({ msg.getNoteNumber(), msg.getFloatVelocity() });
        }
        else if (msg.isNoteOff())
        {
            midiEventQueue.push({ msg.getNoteNumber(), 0.0f });
        }
    }

    // Render MIDI to audio via synthesiser
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master volume
    float volumeDb = parameters.getRawParameterValue("masterVolume")->load();
    buffer.applyGain(juce::Decibels::decibelsToGain(volumeDb));
}

int OLyricaAudioProcessor::getActiveVoiceCount() const
{
    // Phase 2.11: Count actively playing voices
    int activeCount = 0;
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = synthesiser.getVoice(i))
        {
            if (voice->isVoiceActive())
                ++activeCount;
        }
    }
    return activeCount;
}

// v1.7.4: Trigger note from WebView keyboard visualization
void OLyricaAudioProcessor::triggerNoteOn(int midiNote, float velocity)
{
    // Clamp values to valid ranges
    midiNote = juce::jlimit(0, 127, midiNote);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);

    // Use channel 1 (index 0) for UI-triggered notes
    synthesiser.noteOn(1, midiNote, velocity);

    // v1.7.9: Push to event queue for tuning circle visualization
    midiEventQueue.push({ midiNote, velocity });
}

// v1.7.4: Release note from WebView keyboard visualization
void OLyricaAudioProcessor::triggerNoteOff(int midiNote)
{
    midiNote = juce::jlimit(0, 127, midiNote);

    // allowTailOff = true for natural release
    synthesiser.noteOff(1, midiNote, 0.0f, true);

    // v1.7.9: Push to event queue for tuning circle visualization
    midiEventQueue.push({ midiNote, 0.0f });
}

// v1.10.0: Get held notes and their frequencies for True Keys visualization
void OLyricaAudioProcessor::getHeldNotesData(std::vector<int>& notes, std::vector<double>& frequencies)
{
    notes.clear();
    frequencies.clear();

    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = synthesiser.getVoice(i))
        {
            if (voice->isVoiceActive())
            {
                int midiNote = voice->getCurrentlyPlayingNote();
                double freq = tuningEngine.getFrequency(midiNote);
                notes.push_back(midiNote);
                frequencies.push_back(freq);
            }
        }
    }
}

juce::AudioProcessorEditor* OLyricaAudioProcessor::createEditor()
{
    return new OLyricaAudioProcessorEditor(*this);
}

void OLyricaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v1.5.0: Use preset manager for state serialization
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
    {
        // v1.13.1: Save tonic directly to XML (workaround for CustomState serialization bug)
        xml->setAttribute("directTonic", tuningEngine.getTonicNote());
        // v1.18.0: Save tooltip enabled state
        xml->setAttribute("tooltipsEnabled", tooltipsEnabled.load(std::memory_order_acquire));
        copyXmlToBinary(*xml, destData);
    }
}

void OLyricaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // v1.13.3: Set flag to prevent processBlock from interfering during restoration
    isRestoringState.store(true, std::memory_order_release);

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // v1.5.0: Use preset manager for state deserialization
        presetManager.setStateFromXml(xmlState.get());

        // v1.13.1: Restore tonic directly from XML (workaround for CustomState serialization bug)
        if (xmlState->hasAttribute("directTonic"))
        {
            int savedTonic = xmlState->getIntAttribute("directTonic", 0);
            tuningEngine.setTonicNote(savedTonic);
        }

        // v1.18.0: Restore tooltip enabled state
        if (xmlState->hasAttribute("tooltipsEnabled"))
        {
            bool enabled = xmlState->getBoolAttribute("tooltipsEnabled", false);
            tooltipsEnabled.store(enabled, std::memory_order_release);
        }
    }

    // v1.13.3: Clear restoration flag
    isRestoringState.store(false, std::memory_order_release);
}

// v1.5.0: Initialize factory presets (48 presets organized by string material)
void OLyricaAudioProcessor::initializeFactoryPresets()
{
    // Check if factory presets already exist
    auto factoryDir = presetManager.getFactoryPresetsDirectory();
    if (factoryDir.isDirectory() && factoryDir.getNumberOfChildFiles(juce::File::findFiles, "*.json") > 0)
    {
        DBG("[O-Lyrica] Factory presets already exist, skipping initialization");
        return;
    }

    std::vector<OuariconPresetManager::FactoryPresetDef> presets;

    // Helper: Convert choice index to normalized value
    // stringMaterial: 8 options (0-7)
    // woodType, technique: 4 options (0-3)
    // glissandoMode: 3 options (0-2)
    auto choiceNorm = [](int index, int numOptions) { return static_cast<float>(index) / static_cast<float>(numOptions - 1); };

    // ========================================================================
    // GUT STRINGS (Material 0) - Warm, historical, organic character
    // ========================================================================
    presets.push_back({ "Ancient Lyre", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.35f }, { "timbre", 0.6f },
        { "decayTime", 0.4f }, { "bodySize", 0.6f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.4f }, { "fingerHardness", 0.3f }, { "attackNoise", 0.6f },
        { "stringTension", 0.4f }, { "stringStiffness", 0.15f }, { "bridgeBrightness", 0.4f }
    }, {} });

    presets.push_back({ "Fireside Tales", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.4f }, { "timbre", 0.65f },
        { "decayTime", 0.35f }, { "bodySize", 0.55f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.45f }, { "fingerHardness", 0.35f }, { "attackNoise", 0.5f },
        { "stringTension", 0.45f }, { "stringStiffness", 0.2f }, { "bridgeBrightness", 0.45f }
    }, {} });

    presets.push_back({ "Medieval Court", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.45f }, { "timbre", 0.7f },
        { "decayTime", 0.45f }, { "bodySize", 0.65f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.5f }, { "fingerHardness", 0.4f }, { "attackNoise", 0.55f },
        { "stringTension", 0.5f }, { "stringStiffness", 0.18f }, { "bridgeBrightness", 0.5f }
    }, {} });

    presets.push_back({ "Warm Classical", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.5f }, { "timbre", 0.75f },
        { "decayTime", 0.5f }, { "bodySize", 0.7f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.45f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.45f }, { "attackNoise", 0.45f },
        { "stringTension", 0.55f }, { "stringStiffness", 0.22f }, { "bridgeBrightness", 0.55f }
    }, {} });

    presets.push_back({ "Bardic Song", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.42f }, { "timbre", 0.68f },
        { "decayTime", 0.38f }, { "bodySize", 0.58f }, { "bodyResonance", 0.68f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.42f }, { "fingerHardness", 0.38f }, { "attackNoise", 0.58f },
        { "stringTension", 0.42f }, { "stringStiffness", 0.16f }, { "bridgeBrightness", 0.42f }
    }, {} });

    presets.push_back({ "Nostalgic Whisper", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.3f }, { "timbre", 0.55f },
        { "decayTime", 0.55f }, { "bodySize", 0.5f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.3f },
        { "pluckPosition", 0.35f }, { "fingerHardness", 0.25f }, { "attackNoise", 0.4f },
        { "stringTension", 0.35f }, { "stringStiffness", 0.12f }, { "bridgeBrightness", 0.35f }
    }, {} });

    // ========================================================================
    // NYLON STRINGS (Material 1) - Soft, mellow, folk/classical character
    // ========================================================================
    presets.push_back({ "Celtic Dawn", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.45f }, { "timbre", 0.7f },
        { "decayTime", 0.45f }, { "bodySize", 0.6f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.5f }, { "fingerHardness", 0.4f }, { "attackNoise", 0.35f },
        { "stringTension", 0.5f }, { "stringStiffness", 0.15f }, { "bridgeBrightness", 0.5f }
    }, {} });

    presets.push_back({ "Folk Ballad", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.5f }, { "timbre", 0.72f },
        { "decayTime", 0.4f }, { "bodySize", 0.55f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.48f }, { "fingerHardness", 0.45f }, { "attackNoise", 0.4f },
        { "stringTension", 0.52f }, { "stringStiffness", 0.18f }, { "bridgeBrightness", 0.52f }
    }, {} });

    presets.push_back({ "Gentle Stream", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.4f }, { "timbre", 0.65f },
        { "decayTime", 0.55f }, { "bodySize", 0.65f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.45f }, { "fingerHardness", 0.35f }, { "attackNoise", 0.3f },
        { "stringTension", 0.45f }, { "stringStiffness", 0.12f }, { "bridgeBrightness", 0.45f }
    }, {} });

    presets.push_back({ "Morning Dew", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.55f }, { "timbre", 0.68f },
        { "decayTime", 0.5f }, { "bodySize", 0.58f }, { "bodyResonance", 0.62f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.45f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.42f }, { "attackNoise", 0.38f },
        { "stringTension", 0.48f }, { "stringStiffness", 0.16f }, { "bridgeBrightness", 0.48f }
    }, {} });

    presets.push_back({ "Pastoral Scene", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.48f }, { "timbre", 0.74f },
        { "decayTime", 0.48f }, { "bodySize", 0.62f }, { "bodyResonance", 0.68f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.52f },
        { "pluckPosition", 0.5f }, { "fingerHardness", 0.38f }, { "attackNoise", 0.32f },
        { "stringTension", 0.5f }, { "stringStiffness", 0.14f }, { "bridgeBrightness", 0.5f }
    }, {} });

    presets.push_back({ "Harmonic Dreams", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.52f }, { "timbre", 0.76f },
        { "decayTime", 0.6f }, { "bodySize", 0.68f }, { "bodyResonance", 0.72f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.6f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.3f }, { "attackNoise", 0.25f },
        { "stringTension", 0.46f }, { "stringStiffness", 0.1f }, { "bridgeBrightness", 0.46f },
        { "technique", choiceNorm(1, 4) }  // Harmonic technique
    }, {} });

    // ========================================================================
    // WIRE STRINGS (Material 2) - Bright, articulate, concert harp character
    // ========================================================================
    presets.push_back({ "Bright Cascade", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.7f }, { "timbre", 0.65f },
        { "decayTime", 0.5f }, { "bodySize", 0.55f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.45f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.55f }, { "attackNoise", 0.5f },
        { "stringTension", 0.6f }, { "stringStiffness", 0.25f }, { "bridgeBrightness", 0.6f }
    }, {} });

    presets.push_back({ "Articulate Pluck", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.65f }, { "timbre", 0.6f },
        { "decayTime", 0.35f }, { "bodySize", 0.5f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.6f }, { "attackNoise", 0.55f },
        { "stringTension", 0.65f }, { "stringStiffness", 0.28f }, { "bridgeBrightness", 0.65f }
    }, {} });

    presets.push_back({ "Concert Grand", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.6f }, { "timbre", 0.7f },
        { "decayTime", 0.55f }, { "bodySize", 0.7f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.5f }, { "attackNoise", 0.45f },
        { "stringTension", 0.55f }, { "stringStiffness", 0.22f }, { "bridgeBrightness", 0.55f }
    }, {} });

    presets.push_back({ "Modern Classic", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.62f }, { "timbre", 0.68f },
        { "decayTime", 0.48f }, { "bodySize", 0.6f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.5f }, { "fingerHardness", 0.52f }, { "attackNoise", 0.48f },
        { "stringTension", 0.58f }, { "stringStiffness", 0.24f }, { "bridgeBrightness", 0.58f }
    }, {} });

    presets.push_back({ "Silver Strings", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.72f }, { "timbre", 0.62f },
        { "decayTime", 0.52f }, { "bodySize", 0.52f }, { "bodyResonance", 0.58f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.42f },
        { "pluckPosition", 0.58f }, { "fingerHardness", 0.58f }, { "attackNoise", 0.52f },
        { "stringTension", 0.62f }, { "stringStiffness", 0.26f }, { "bridgeBrightness", 0.62f }
    }, {} });

    presets.push_back({ "Pedal Technique", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.58f }, { "timbre", 0.72f },
        { "decayTime", 0.6f }, { "bodySize", 0.68f }, { "bodyResonance", 0.72f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.58f },
        { "pluckPosition", 0.48f }, { "fingerHardness", 0.48f }, { "attackNoise", 0.42f },
        { "stringTension", 0.52f }, { "stringStiffness", 0.2f }, { "bridgeBrightness", 0.52f }
    }, {} });

    // ========================================================================
    // CARBON STRINGS (Material 3) - Clean, precise, extended range
    // ========================================================================
    presets.push_back({ "Crystal Clear", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.75f }, { "timbre", 0.55f },
        { "decayTime", 0.45f }, { "bodySize", 0.45f }, { "bodyResonance", 0.5f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.65f }, { "attackNoise", 0.4f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.3f }, { "bridgeBrightness", 0.7f }
    }, {} });

    presets.push_back({ "Precision Touch", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.7f }, { "timbre", 0.58f },
        { "decayTime", 0.4f }, { "bodySize", 0.48f }, { "bodyResonance", 0.52f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.3f },
        { "pluckPosition", 0.58f }, { "fingerHardness", 0.62f }, { "attackNoise", 0.45f },
        { "stringTension", 0.68f }, { "stringStiffness", 0.32f }, { "bridgeBrightness", 0.68f }
    }, {} });

    presets.push_back({ "Extended Range", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.68f }, { "timbre", 0.6f },
        { "decayTime", 0.5f }, { "bodySize", 0.55f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.6f }, { "attackNoise", 0.42f },
        { "stringTension", 0.65f }, { "stringStiffness", 0.28f }, { "bridgeBrightness", 0.65f }
    }, {} });

    presets.push_back({ "Studio Session", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.65f }, { "timbre", 0.62f },
        { "decayTime", 0.42f }, { "bodySize", 0.5f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.38f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.58f }, { "attackNoise", 0.48f },
        { "stringTension", 0.62f }, { "stringStiffness", 0.26f }, { "bridgeBrightness", 0.62f }
    }, {} });

    presets.push_back({ "Clean Articulation", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.72f }, { "timbre", 0.56f },
        { "decayTime", 0.35f }, { "bodySize", 0.42f }, { "bodyResonance", 0.48f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.32f },
        { "pluckPosition", 0.62f }, { "fingerHardness", 0.68f }, { "attackNoise", 0.5f },
        { "stringTension", 0.72f }, { "stringStiffness", 0.34f }, { "bridgeBrightness", 0.72f }
    }, {} });

    presets.push_back({ "Harmonic Purity", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.78f }, { "timbre", 0.52f },
        { "decayTime", 0.55f }, { "bodySize", 0.4f }, { "bodyResonance", 0.45f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.25f },
        { "pluckPosition", 0.65f }, { "fingerHardness", 0.7f }, { "attackNoise", 0.35f },
        { "stringTension", 0.75f }, { "stringStiffness", 0.35f }, { "bridgeBrightness", 0.75f },
        { "technique", choiceNorm(1, 4) }  // Harmonic technique
    }, {} });

    // ========================================================================
    // METAL ALLOY STRINGS (Material 4) - Brilliant, bell-like, sustaining
    // ========================================================================
    presets.push_back({ "Brilliant Sustain", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.75f }, { "timbre", 0.6f },
        { "decayTime", 0.7f }, { "bodySize", 0.6f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.6f }, { "attackNoise", 0.55f },
        { "stringTension", 0.65f }, { "stringStiffness", 0.35f }, { "bridgeBrightness", 0.65f }
    }, {} });

    presets.push_back({ "Bell Tones", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.8f }, { "timbre", 0.55f },
        { "decayTime", 0.65f }, { "bodySize", 0.5f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.65f }, { "attackNoise", 0.6f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.38f }, { "bridgeBrightness", 0.7f }
    }, {} });

    presets.push_back({ "Orchestral Ring", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.7f }, { "timbre", 0.65f },
        { "decayTime", 0.6f }, { "bodySize", 0.65f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.6f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.55f }, { "attackNoise", 0.5f },
        { "stringTension", 0.6f }, { "stringStiffness", 0.32f }, { "bridgeBrightness", 0.6f }
    }, {} });

    presets.push_back({ "Shimmering Heights", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.82f }, { "timbre", 0.52f },
        { "decayTime", 0.68f }, { "bodySize", 0.45f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.45f },
        { "pluckPosition", 0.65f }, { "fingerHardness", 0.68f }, { "attackNoise", 0.58f },
        { "stringTension", 0.72f }, { "stringStiffness", 0.4f }, { "bridgeBrightness", 0.72f }
    }, {} });

    presets.push_back({ "Warm Metallic", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.6f }, { "timbre", 0.7f },
        { "decayTime", 0.62f }, { "bodySize", 0.68f }, { "bodyResonance", 0.72f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.58f },
        { "pluckPosition", 0.48f }, { "fingerHardness", 0.52f }, { "attackNoise", 0.48f },
        { "stringTension", 0.58f }, { "stringStiffness", 0.3f }, { "bridgeBrightness", 0.58f }
    }, {} });

    presets.push_back({ "Ethereal Chime", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.78f }, { "timbre", 0.58f },
        { "decayTime", 0.75f }, { "bodySize", 0.55f }, { "bodyResonance", 0.62f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.65f },
        { "pluckPosition", 0.58f }, { "fingerHardness", 0.62f }, { "attackNoise", 0.52f },
        { "stringTension", 0.68f }, { "stringStiffness", 0.36f }, { "bridgeBrightness", 0.68f }
    }, {} });

    // ========================================================================
    // GLASS STRINGS (Material 5) - Crystalline, fragile, delicate
    // ========================================================================
    presets.push_back({ "Crystalline Voice", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.85f }, { "timbre", 0.48f },
        { "decayTime", 0.55f }, { "bodySize", 0.4f }, { "bodyResonance", 0.5f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.65f }, { "fingerHardness", 0.72f }, { "attackNoise", 0.45f },
        { "stringTension", 0.75f }, { "stringStiffness", 0.42f }, { "bridgeBrightness", 0.75f }
    }, {} });

    presets.push_back({ "Fragile Beauty", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.8f }, { "timbre", 0.52f },
        { "decayTime", 0.5f }, { "bodySize", 0.42f }, { "bodyResonance", 0.52f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.38f },
        { "pluckPosition", 0.62f }, { "fingerHardness", 0.68f }, { "attackNoise", 0.4f },
        { "stringTension", 0.72f }, { "stringStiffness", 0.4f }, { "bridgeBrightness", 0.72f }
    }, {} });

    presets.push_back({ "Ice Palace", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.88f }, { "timbre", 0.45f },
        { "decayTime", 0.6f }, { "bodySize", 0.35f }, { "bodyResonance", 0.45f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.7f }, { "fingerHardness", 0.75f }, { "attackNoise", 0.38f },
        { "stringTension", 0.78f }, { "stringStiffness", 0.45f }, { "bridgeBrightness", 0.78f }
    }, {} });

    presets.push_back({ "Winter Bells", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.82f }, { "timbre", 0.5f },
        { "decayTime", 0.58f }, { "bodySize", 0.38f }, { "bodyResonance", 0.48f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.42f },
        { "pluckPosition", 0.68f }, { "fingerHardness", 0.7f }, { "attackNoise", 0.42f },
        { "stringTension", 0.76f }, { "stringStiffness", 0.43f }, { "bridgeBrightness", 0.76f }
    }, {} });

    presets.push_back({ "Delicate Touch", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.78f }, { "timbre", 0.55f },
        { "decayTime", 0.52f }, { "bodySize", 0.44f }, { "bodyResonance", 0.54f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.36f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.65f }, { "attackNoise", 0.35f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.38f }, { "bridgeBrightness", 0.7f }
    }, {} });

    presets.push_back({ "Harmonic Prism", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.9f }, { "timbre", 0.42f },
        { "decayTime", 0.62f }, { "bodySize", 0.32f }, { "bodyResonance", 0.42f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.3f },
        { "pluckPosition", 0.72f }, { "fingerHardness", 0.78f }, { "attackNoise", 0.32f },
        { "stringTension", 0.8f }, { "stringStiffness", 0.48f }, { "bridgeBrightness", 0.8f },
        { "technique", choiceNorm(1, 4) }  // Harmonic technique
    }, {} });

    // ========================================================================
    // CRYSTAL STRINGS (Material 6) - Pure, mystical, resonant
    // ========================================================================
    presets.push_back({ "Pure Resonance", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.8f }, { "timbre", 0.5f },
        { "decayTime", 0.7f }, { "bodySize", 0.45f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.65f }, { "attackNoise", 0.35f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.4f }, { "bridgeBrightness", 0.7f }
    }, {} });

    presets.push_back({ "Mystical Glow", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.75f }, { "timbre", 0.55f },
        { "decayTime", 0.75f }, { "bodySize", 0.5f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.6f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.6f }, { "attackNoise", 0.3f },
        { "stringTension", 0.65f }, { "stringStiffness", 0.38f }, { "bridgeBrightness", 0.65f }
    }, {} });

    presets.push_back({ "Sacred Space", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.72f }, { "timbre", 0.58f },
        { "decayTime", 0.8f }, { "bodySize", 0.55f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.65f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.55f }, { "attackNoise", 0.28f },
        { "stringTension", 0.62f }, { "stringStiffness", 0.35f }, { "bridgeBrightness", 0.62f }
    }, {} });

    presets.push_back({ "Singing Bowls", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.68f }, { "timbre", 0.62f },
        { "decayTime", 0.85f }, { "bodySize", 0.6f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.7f },
        { "pluckPosition", 0.48f }, { "fingerHardness", 0.5f }, { "attackNoise", 0.25f },
        { "stringTension", 0.58f }, { "stringStiffness", 0.32f }, { "bridgeBrightness", 0.58f }
    }, {} });

    presets.push_back({ "Meditation", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.65f }, { "timbre", 0.65f },
        { "decayTime", 0.9f }, { "bodySize", 0.65f }, { "bodyResonance", 0.72f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.72f },
        { "pluckPosition", 0.45f }, { "fingerHardness", 0.45f }, { "attackNoise", 0.2f },
        { "stringTension", 0.55f }, { "stringStiffness", 0.28f }, { "bridgeBrightness", 0.55f }
    }, {} });

    presets.push_back({ "Angelic Choir", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.78f }, { "timbre", 0.52f },
        { "decayTime", 0.82f }, { "bodySize", 0.48f }, { "bodyResonance", 0.58f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.62f },
        { "pluckPosition", 0.58f }, { "fingerHardness", 0.62f }, { "attackNoise", 0.22f },
        { "stringTension", 0.68f }, { "stringStiffness", 0.36f }, { "bridgeBrightness", 0.68f },
        { "technique", choiceNorm(1, 4) }  // Harmonic technique
    }, {} });

    // ========================================================================
    // ENERGY STRINGS (Material 7) - Futuristic, synthetic, experimental
    // ========================================================================
    presets.push_back({ "Quantum Strings", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.85f }, { "timbre", 0.4f },
        { "decayTime", 0.6f }, { "bodySize", 0.35f }, { "bodyResonance", 0.45f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.7f }, { "fingerHardness", 0.8f }, { "attackNoise", 0.6f },
        { "stringTension", 0.8f }, { "stringStiffness", 0.5f }, { "bridgeBrightness", 0.8f }
    }, {} });

    presets.push_back({ "Plasma Resonance", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.9f }, { "timbre", 0.35f },
        { "decayTime", 0.65f }, { "bodySize", 0.3f }, { "bodyResonance", 0.4f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.75f }, { "fingerHardness", 0.85f }, { "attackNoise", 0.65f },
        { "stringTension", 0.85f }, { "stringStiffness", 0.55f }, { "bridgeBrightness", 0.85f }
    }, {} });

    presets.push_back({ "Electric Dreams", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.82f }, { "timbre", 0.45f },
        { "decayTime", 0.55f }, { "bodySize", 0.4f }, { "bodyResonance", 0.5f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.45f },
        { "pluckPosition", 0.65f }, { "fingerHardness", 0.75f }, { "attackNoise", 0.55f },
        { "stringTension", 0.75f }, { "stringStiffness", 0.45f }, { "bridgeBrightness", 0.75f }
    }, {} });

    presets.push_back({ "Cosmic Harp", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.78f }, { "timbre", 0.5f },
        { "decayTime", 0.7f }, { "bodySize", 0.45f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.7f }, { "attackNoise", 0.5f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.42f }, { "bridgeBrightness", 0.7f }
    }, {} });

    presets.push_back({ "Neon Glow", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.88f }, { "timbre", 0.38f },
        { "decayTime", 0.58f }, { "bodySize", 0.32f }, { "bodyResonance", 0.42f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.38f },
        { "pluckPosition", 0.72f }, { "fingerHardness", 0.82f }, { "attackNoise", 0.58f },
        { "stringTension", 0.82f }, { "stringStiffness", 0.52f }, { "bridgeBrightness", 0.82f }
    }, {} });

    presets.push_back({ "Future Primitive", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.75f }, { "timbre", 0.55f },
        { "decayTime", 0.62f }, { "bodySize", 0.5f }, { "bodyResonance", 0.58f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.68f }, { "attackNoise", 0.52f },
        { "stringTension", 0.68f }, { "stringStiffness", 0.4f }, { "bridgeBrightness", 0.68f }
    }, {} });

    // Initialize all presets
    presetManager.initializeFactoryPresets(presets);
    DBG("[O-Lyrica] Initialized " + juce::String(presets.size()) + " factory presets");
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OLyricaAudioProcessor();
}
