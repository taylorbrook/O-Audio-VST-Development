/*
  ==============================================================================

    PresetManager - Preset save/load/browse system
    O-IntonationPad / Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PresetManager.h"
#include "PluginProcessor.h"

PresetManager::PresetManager(OIntonationPadAudioProcessor& processor)
    : processorRef(processor)
{
    // Ensure user preset folder exists
    getUserPresetFolder().createDirectory();
}

juce::File PresetManager::getUserPresetFolder() const
{
#if JUCE_MAC
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library/Audio/Presets/Ouaricon/O-IntonationPad");
#else
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Ouaricon/O-IntonationPad/Presets");
#endif
}

juce::File PresetManager::getPresetFile(const juce::String& name) const
{
    return getUserPresetFolder().getChildFile(name + ".xml");
}

// ═══════════════════════════════════════════════════════════════════
// PRESET OPERATIONS
// ═══════════════════════════════════════════════════════════════════

bool PresetManager::savePreset(const juce::String& name, const juce::String& category)
{
    if (name.isEmpty())
        return false;

    // Get current state from processor
    juce::MemoryBlock stateData;
    processorRef.getStateInformation(stateData);

    // Parse the binary state back to XML
    std::unique_ptr<juce::XmlElement> stateXml(
        juce::AudioProcessor::getXmlFromBinary(stateData.getData(),
                                                static_cast<int>(stateData.getSize())));
    if (stateXml == nullptr)
        return false;

    // Wrap in preset envelope with metadata
    auto presetXml = std::make_unique<juce::XmlElement>("OIntonationPadPreset");
    presetXml->setAttribute("name", name);
    presetXml->setAttribute("category", category);
    presetXml->setAttribute("version", "2.7.0");
    presetXml->setAttribute("date", juce::Time::getCurrentTime().toISO8601(false));
    presetXml->addChildElement(new juce::XmlElement(*stateXml));

    // Write to file
    auto file = getPresetFile(name);
    if (!presetXml->writeTo(file))
        return false;

    currentPresetName = name;
    isDirty = false;
    return true;
}

bool PresetManager::loadPreset(const juce::String& name, bool isFactory)
{
    std::unique_ptr<juce::XmlElement> stateXml;

    if (isFactory)
    {
        // Find factory preset and build XML
        for (const auto& fp : getFactoryPresetData())
        {
            if (juce::String(fp.name) == name)
            {
                auto xmlStr = buildFactoryPresetXml(fp);
                stateXml = juce::XmlDocument::parse(xmlStr);
                break;
            }
        }
    }
    else
    {
        // Load from user file
        auto file = getPresetFile(name);
        if (!file.existsAsFile())
            return false;

        auto presetXml = juce::XmlDocument::parse(file);
        if (presetXml == nullptr)
            return false;

        // Extract state from preset envelope
        auto* stateChild = presetXml->getFirstChildElement();
        if (stateChild != nullptr)
            stateXml = std::make_unique<juce::XmlElement>(*stateChild);
    }

    if (stateXml == nullptr)
        return false;

    // Convert XML to binary and load into processor
    juce::MemoryBlock destData;
    juce::AudioProcessor::copyXmlToBinary(*stateXml, destData);
    processorRef.setStateInformation(destData.getData(),
                                      static_cast<int>(destData.getSize()));

    currentPresetName = name;
    isDirty = false;
    return true;
}

bool PresetManager::deletePreset(const juce::String& name)
{
    auto file = getPresetFile(name);
    if (!file.existsAsFile())
        return false;

    bool deleted = file.deleteFile();
    if (deleted && currentPresetName == name)
        currentPresetName = "";

    return deleted;
}

bool PresetManager::renamePreset(const juce::String& oldName, const juce::String& newName)
{
    if (newName.isEmpty() || oldName == newName)
        return false;

    auto oldFile = getPresetFile(oldName);
    auto newFile = getPresetFile(newName);

    if (!oldFile.existsAsFile() || newFile.existsAsFile())
        return false;

    // Update metadata inside the XML
    auto presetXml = juce::XmlDocument::parse(oldFile);
    if (presetXml != nullptr)
        presetXml->setAttribute("name", newName);

    if (presetXml != nullptr && presetXml->writeTo(newFile))
    {
        oldFile.deleteFile();
        if (currentPresetName == oldName)
            currentPresetName = newName;
        return true;
    }

    return false;
}

// ═══════════════════════════════════════════════════════════════════
// NAVIGATION
// ═══════════════════════════════════════════════════════════════════

bool PresetManager::loadNextPreset()
{
    auto list = buildFlatList();
    if (list.empty()) return false;

    int idx = findPresetIndex(currentPresetName, list);
    int nextIdx = (idx + 1) % static_cast<int>(list.size());
    return loadPreset(list[static_cast<size_t>(nextIdx)].name,
                      list[static_cast<size_t>(nextIdx)].isFactory);
}

bool PresetManager::loadPrevPreset()
{
    auto list = buildFlatList();
    if (list.empty()) return false;

    int idx = findPresetIndex(currentPresetName, list);
    int prevIdx = (idx - 1 + static_cast<int>(list.size())) % static_cast<int>(list.size());
    return loadPreset(list[static_cast<size_t>(prevIdx)].name,
                      list[static_cast<size_t>(prevIdx)].isFactory);
}

// ═══════════════════════════════════════════════════════════════════
// QUERIES
// ═══════════════════════════════════════════════════════════════════

std::vector<PresetManager::PresetInfo> PresetManager::getAllPresets()
{
    return buildFlatList();
}

juce::StringArray PresetManager::getCategories() const
{
    return { "All", "Ambient", "Cinematic", "Classic Pads", "Drones", "Experimental" };
}

std::vector<PresetManager::PresetInfo> PresetManager::buildFlatList()
{
    std::vector<PresetInfo> list;

    // Factory presets first
    for (const auto& fp : getFactoryPresetData())
        list.push_back({ juce::String(fp.name), juce::String(fp.category), true });

    // User presets from folder
    auto folder = getUserPresetFolder();
    if (folder.isDirectory())
    {
        for (const auto& file : folder.findChildFiles(juce::File::findFiles, false, "*.xml"))
        {
            auto xml = juce::XmlDocument::parse(file);
            if (xml != nullptr && xml->getTagName() == "OIntonationPadPreset")
            {
                juce::String name = xml->getStringAttribute("name", file.getFileNameWithoutExtension());
                juce::String category = xml->getStringAttribute("category", "User");

                // Skip if name matches a factory preset
                bool isFactoryDupe = false;
                for (const auto& fp : getFactoryPresetData())
                    if (juce::String(fp.name) == name) { isFactoryDupe = true; break; }

                if (!isFactoryDupe)
                    list.push_back({ name, category, false });
            }
        }
    }

    return list;
}

int PresetManager::findPresetIndex(const juce::String& name, const std::vector<PresetInfo>& list) const
{
    for (int i = 0; i < static_cast<int>(list.size()); ++i)
        if (list[static_cast<size_t>(i)].name == name)
            return i;
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
// FACTORY PRESETS
// ═══════════════════════════════════════════════════════════════════

const std::vector<PresetManager::FactoryPresetData>& PresetManager::getFactoryPresetData()
{
    static const std::vector<FactoryPresetData> presets = {
        // 1. Init — clean default starting point
        {
            "Init", "Classic Pads",
            {}, // all defaults
            8,  // Just Intonation
            "Just Intonation",
            0   // C
        },

        // 2. Deep Space — ambient pad with long tail
        {
            "Deep Space", "Ambient",
            {
                {"voiceCount", "8"}, {"complexity", "0.7"},
                {"wavetableBank", "7"},   // Ethereal
                {"wavetablePos", "0.35"}, {"wavetableBank2", "5"}, // Evolving
                {"wavetablePos2", "0.6"}, {"gainA", "0.8"}, {"gainB", "0.5"},
                {"attackTime", "2.0"}, {"decayTime", "1.5"}, {"sustainLevel", "0.7"},
                {"releaseTime", "5.0"}, {"lfoRate", "0.15"}, {"lfoDepth", "0.3"},
                {"lfoRate2", "0.08"}, {"lfoDepth2", "0.2"},
                {"filterCutoff", "3500"}, {"stereoSpread", "0.8"},
                {"spacing", "0.3"}, {"inversion", "0.4"},
                {"reverbBypass", "0"}, {"reverbSize", "0.85"},
                {"reverbDamp", "0.3"}, {"reverbMix", "0.45"}, {"reverbPredelay", "40"},
                {"chorusBypass", "0"}, {"chorusMix", "0.25"}, {"chorusDepth", "0.4"},
                {"detuneRandom", "8"}, {"timingRandom", "20"},
                {"masterVolume", "0.85"}
            },
            8, "Just Intonation", 0
        },

        // 3. Film Score — cinematic orchestral pad
        {
            "Film Score", "Cinematic",
            {
                {"voiceCount", "10"}, {"complexity", "0.6"},
                {"voicingMode", "2"}, // Open voicing
                {"wavetableBank", "2"},  // Choir
                {"wavetablePos", "0.5"}, {"wavetableBank2", "3"}, // Strings
                {"wavetablePos2", "0.4"}, {"gainA", "0.7"}, {"gainB", "0.6"},
                {"attackTime", "1.5"}, {"decayTime", "0.8"}, {"sustainLevel", "0.85"},
                {"releaseTime", "3.5"}, {"lfoRate", "0.2"}, {"lfoDepth", "0.15"},
                {"lfoRate2", "0.3"}, {"lfoDepth2", "0.1"},
                {"filterCutoff", "6000"}, {"stereoSpread", "0.7"},
                {"spacing", "0.2"}, {"inversion", "0.3"},
                {"reverbBypass", "0"}, {"reverbSize", "0.7"},
                {"reverbDamp", "0.4"}, {"reverbMix", "0.35"}, {"reverbPredelay", "30"},
                {"delayBypass", "0"}, {"delayTime", "0.5"},
                {"delayFeedback", "0.2"}, {"delayMix", "0.15"},
                {"eqBypass", "0"}, {"eqLowGain", "2.0"}, {"eqHighGain", "-1.5"},
                {"masterVolume", "0.9"}
            },
            8, "Just Intonation", 0
        },

        // 4. Ancient Drone — deep sustained drone
        {
            "Ancient Drone", "Drones",
            {
                {"voiceCount", "6"}, {"complexity", "0.4"},
                {"voicingMode", "5"}, // Quartal
                {"wavetableBank", "8"},  // Dark Matter
                {"wavetablePos", "0.3"}, {"wavetableBank2", "15"}, // Warm Sub
                {"wavetablePos2", "0.2"}, {"gainA", "0.9"}, {"gainB", "0.7"},
                {"attackTime", "3.0"}, {"decayTime", "2.0"}, {"sustainLevel", "0.95"},
                {"releaseTime", "8.0"}, {"lfoRate", "0.05"}, {"lfoDepth", "0.4"},
                {"lfoRate2", "0.03"}, {"lfoDepth2", "0.25"},
                {"filterCutoff", "1800"}, {"filterLfoDepth", "0.3"},
                {"stereoSpread", "0.6"},
                {"spacing", "0.0"}, {"inversion", "0.5"},
                {"reverbBypass", "0"}, {"reverbSize", "0.95"},
                {"reverbDamp", "0.2"}, {"reverbMix", "0.55"}, {"reverbPredelay", "60"},
                {"detuneRandom", "12"}, {"timingRandom", "30"},
                {"masterVolume", "0.8"}
            },
            8, "Just Intonation", 0
        },

        // 5. Pythagorean Fifths — classic tuning showcase
        {
            "Pythagorean Fifths", "Classic Pads",
            {
                {"voiceCount", "7"}, {"complexity", "0.5"},
                {"voicingMode", "6"}, // Quintal
                {"wavetableBank", "0"},  // JI Harmonic
                {"wavetablePos", "0.45"}, {"wavetableBank2", "6"}, // Organ
                {"wavetablePos2", "0.5"}, {"gainA", "0.85"}, {"gainB", "0.35"},
                {"attackTime", "0.8"}, {"decayTime", "0.5"}, {"sustainLevel", "0.9"},
                {"releaseTime", "2.5"}, {"lfoRate", "0.3"}, {"lfoDepth", "0.1"},
                {"filterCutoff", "7000"}, {"stereoSpread", "0.5"},
                {"spacing", "0.15"}, {"inversion", "0.25"},
                {"reverbBypass", "0"}, {"reverbSize", "0.5"},
                {"reverbDamp", "0.5"}, {"reverbMix", "0.2"},
                {"masterVolume", "0.9"}
            },
            1, "Pythagorean", 0  // Pythagorean tuning
        },

        // 6. Glass Bells — crystalline JI pad
        {
            "Glass Bells", "Classic Pads",
            {
                {"voiceCount", "5"}, {"complexity", "0.55"},
                {"voicingMode", "1"}, // Close voicing
                {"wavetableBank", "4"},  // Glass
                {"wavetablePos", "0.6"}, {"wavetableBank2", "9"}, // Sine
                {"wavetablePos2", "0.5"}, {"gainA", "0.9"}, {"gainB", "0.3"},
                {"attackTime", "0.3"}, {"decayTime", "0.4"}, {"sustainLevel", "0.6"},
                {"releaseTime", "4.0"}, {"lfoRate", "0.5"}, {"lfoDepth", "0.15"},
                {"lfoRate2", "0.7"}, {"lfoDepth2", "0.08"},
                {"filterCutoff", "12000"}, {"stereoSpread", "0.65"},
                {"spacing", "0.1"}, {"inversion", "0.1"},
                {"reverbBypass", "0"}, {"reverbSize", "0.6"},
                {"reverbDamp", "0.35"}, {"reverbMix", "0.3"}, {"reverbPredelay", "15"},
                {"chorusBypass", "0"}, {"chorusMix", "0.15"},
                {"detuneRandom", "3"}, {"timingRandom", "5"},
                {"masterVolume", "0.85"}
            },
            8, "Just Intonation", 0
        },

        // 7. Microtonal Explorer — Bohlen-Pierce experimental
        {
            "Microtonal Explorer", "Experimental",
            {
                {"voiceCount", "6"}, {"complexity", "0.65"},
                {"voicingMode", "0"}, // Free
                {"wavetableBank", "12"},  // Spectral Cloud
                {"wavetablePos", "0.4"}, {"wavetableBank2", "13"}, // Metallic Resonance
                {"wavetablePos2", "0.55"}, {"gainA", "0.75"}, {"gainB", "0.6"},
                {"attackTime", "1.0"}, {"decayTime", "0.7"}, {"sustainLevel", "0.75"},
                {"releaseTime", "3.0"}, {"lfoRate", "0.4"}, {"lfoDepth", "0.35"},
                {"lfoRate2", "0.25"}, {"lfoDepth2", "0.2"},
                {"filterCutoff", "5000"}, {"filterLfoDepth", "0.2"},
                {"stereoSpread", "0.9"},
                {"spacing", "0.25"}, {"inversion", "0.35"},
                {"reverbBypass", "0"}, {"reverbSize", "0.75"},
                {"reverbDamp", "0.3"}, {"reverbMix", "0.4"},
                {"delayBypass", "0"}, {"delayTime", "0.333"},
                {"delayFeedback", "0.35"}, {"delayMix", "0.2"}, {"delayMode", "1"},
                {"detuneRandom", "15"}, {"timingRandom", "25"},
                {"masterVolume", "0.8"}
            },
            9, "Bohlen-Pierce", 0  // Bohlen-Pierce tuning
        },

        // 8. Crystal Cave — shimmering ambient
        {
            "Crystal Cave", "Ambient",
            {
                {"voiceCount", "9"}, {"complexity", "0.55"},
                {"wavetableBank", "4"},  // Glass
                {"wavetablePos", "0.7"}, {"wavetableBank2", "7"}, // Ethereal
                {"wavetablePos2", "0.45"}, {"gainA", "0.85"}, {"gainB", "0.45"},
                {"attackTime", "1.8"}, {"decayTime", "1.0"}, {"sustainLevel", "0.8"},
                {"releaseTime", "6.0"}, {"lfoRate", "0.12"}, {"lfoDepth", "0.25"},
                {"lfoRate2", "0.18"}, {"lfoDepth2", "0.15"},
                {"filterCutoff", "9000"}, {"stereoSpread", "0.85"},
                {"spacing", "0.35"}, {"inversion", "0.2"},
                {"reverbBypass", "0"}, {"reverbSize", "0.9"},
                {"reverbDamp", "0.25"}, {"reverbMix", "0.5"}, {"reverbPredelay", "50"},
                {"chorusBypass", "0"}, {"chorusMix", "0.3"}, {"chorusRate", "0.8"},
                {"chorusDepth", "0.5"},
                {"detuneRandom", "6"}, {"timingRandom", "15"},
                {"masterVolume", "0.85"}
            },
            8, "Just Intonation", 0
        },

        // 9. Cinematic Tension — dark, dissonant
        {
            "Cinematic Tension", "Cinematic",
            {
                {"voiceCount", "8"}, {"complexity", "0.75"},
                {"voicingMode", "3"}, // Drop-2
                {"wavetableBank", "8"},   // Dark Matter
                {"wavetablePos", "0.55"}, {"wavetableBank2", "5"}, // Evolving
                {"wavetablePos2", "0.7"}, {"gainA", "0.8"}, {"gainB", "0.65"},
                {"attackTime", "2.5"}, {"decayTime", "1.2"}, {"sustainLevel", "0.65"},
                {"releaseTime", "4.0"}, {"lfoRate", "0.08"}, {"lfoDepth", "0.45"},
                {"lfoRate2", "0.12"}, {"lfoDepth2", "0.3"},
                {"filterCutoff", "2500"}, {"filterLfoDepth", "0.4"},
                {"velocityToFilter", "0.5"},
                {"stereoSpread", "0.75"},
                {"spacing", "0.15"}, {"inversion", "0.45"},
                {"reverbBypass", "0"}, {"reverbSize", "0.8"},
                {"reverbDamp", "0.15"}, {"reverbMix", "0.5"}, {"reverbPredelay", "45"},
                {"delayBypass", "0"}, {"delayTime", "0.75"},
                {"delayFeedback", "0.4"}, {"delayMix", "0.2"}, {"delayMode", "1"},
                {"detuneRandom", "10"}, {"timingRandom", "18"},
                {"masterVolume", "0.8"}
            },
            8, "Just Intonation", 0
        },

        // 10. Sacred Geometry — pure JI drone
        {
            "Sacred Geometry", "Drones",
            {
                {"voiceCount", "12"}, {"complexity", "0.35"},
                {"voicingMode", "4"}, // Thirds
                {"wavetableBank", "0"},   // JI Harmonic
                {"wavetablePos", "0.25"}, {"wavetableBank2", "0"}, // JI Harmonic
                {"wavetablePos2", "0.75"}, {"gainA", "0.7"}, {"gainB", "0.7"},
                {"attackTime", "4.0"}, {"decayTime", "2.5"}, {"sustainLevel", "1.0"},
                {"releaseTime", "8.0"}, {"lfoRate", "0.04"}, {"lfoDepth", "0.5"},
                {"lfoRate2", "0.06"}, {"lfoDepth2", "0.35"},
                {"filterCutoff", "4000"},
                {"stereoSpread", "0.7"},
                {"spacing", "0.0"}, {"inversion", "0.6"},
                {"reverbBypass", "0"}, {"reverbSize", "0.92"},
                {"reverbDamp", "0.2"}, {"reverbMix", "0.6"}, {"reverbPredelay", "55"},
                {"detuneRandom", "2"}, {"timingRandom", "40"},
                {"masterVolume", "0.75"}
            },
            8, "Just Intonation", 0
        },

        // 11. Warm Analog — classic warm analog pad
        {
            "Warm Analog", "Classic Pads",
            {
                {"voiceCount", "6"}, {"complexity", "0.5"},
                {"voicingMode", "2"}, // Open
                {"wavetableBank", "1"},  // Warm Analog
                {"wavetablePos", "0.4"}, {"wavetableBank2", "17"}, // Velvet Pad
                {"wavetablePos2", "0.5"}, {"gainA", "0.9"}, {"gainB", "0.4"},
                {"attackTime", "0.6"}, {"decayTime", "0.3"}, {"sustainLevel", "0.85"},
                {"releaseTime", "2.0"}, {"lfoRate", "0.35"}, {"lfoDepth", "0.12"},
                {"filterCutoff", "5500"}, {"stereoSpread", "0.5"},
                {"spacing", "0.1"}, {"inversion", "0.3"},
                {"chorusBypass", "0"}, {"chorusMix", "0.2"},
                {"chorusRate", "1.2"}, {"chorusDepth", "0.35"},
                {"reverbBypass", "0"}, {"reverbSize", "0.4"},
                {"reverbDamp", "0.5"}, {"reverbMix", "0.2"},
                {"masterVolume", "0.9"}
            },
            8, "Just Intonation", 0
        },

        // 12. Whisper Texture — delicate experimental
        {
            "Whisper Texture", "Experimental",
            {
                {"voiceCount", "4"}, {"complexity", "0.8"},
                {"wavetableBank", "18"},  // Whisper
                {"wavetablePos", "0.5"}, {"wavetableBank2", "16"}, // Soft Flute
                {"wavetablePos2", "0.6"}, {"gainA", "0.75"}, {"gainB", "0.55"},
                {"attackTime", "1.2"}, {"decayTime", "0.6"}, {"sustainLevel", "0.5"},
                {"releaseTime", "4.5"}, {"lfoRate", "0.6"}, {"lfoDepth", "0.2"},
                {"lfoRate2", "0.9"}, {"lfoDepth2", "0.15"},
                {"filterCutoff", "7500"}, {"filterLfoDepth", "0.15"},
                {"stereoSpread", "0.95"},
                {"spacing", "0.4"}, {"inversion", "0.15"},
                {"reverbBypass", "0"}, {"reverbSize", "0.8"},
                {"reverbDamp", "0.4"}, {"reverbMix", "0.45"}, {"reverbPredelay", "25"},
                {"delayBypass", "0"}, {"delayTime", "0.25"},
                {"delayFeedback", "0.3"}, {"delayMix", "0.15"},
                {"detuneRandom", "7"}, {"timingRandom", "12"},
                {"masterVolume", "0.85"}
            },
            2, "Zarlino", 0  // Zarlino (just intonation variant)
        }
    };

    return presets;
}

juce::String PresetManager::buildFactoryPresetXml(const FactoryPresetData& preset) const
{
    // Start with the current processor's default parameter layout
    // Build a ValueTree that matches the APVTS state format
    auto state = processorRef.getAPVTS().copyState();

    // Apply parameter overrides
    for (const auto& [paramId, value] : preset.paramOverrides)
    {
        if (processorRef.getAPVTS().getParameter(paramId) != nullptr)
        {
            // Find the PARAM child in the ValueTree
            for (int i = 0; i < state.getNumChildren(); ++i)
            {
                auto child = state.getChild(i);
                if (child.hasType("PARAM") && child.getProperty("id").toString() == paramId)
                {
                    child.setProperty("value", value.getFloatValue(), nullptr);
                    break;
                }
            }
        }
    }

    // Set tuning state
    auto tuningState = state.getOrCreateChildWithName("tuningEngine", nullptr);
    tuningState.setProperty("preset", preset.temperamentPreset, nullptr);
    tuningState.setProperty("scaleName", preset.tuningName, nullptr);
    tuningState.setProperty("tonic", preset.tonicNote, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml == nullptr)
        return {};

    return xml->toString();
}
