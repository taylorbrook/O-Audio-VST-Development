/*
  ==============================================================================

    OuariconPresetManager.h
    Ouaricon Module System - Preset Persistence

    Generic preset management for Ouaricon plugins.
    Handles JSON serialization, factory/user presets, and navigation.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <vector>

/**
 * Generic preset manager for Ouaricon plugins.
 *
 * Presets are stored as JSON files in:
 *   ~/Library/Application Support/{pluginName}/Presets/
 *
 * Subdirectories:
 *   - Factory/ : Read-only presets shipped with plugin
 *   - User/    : User-created presets
 *
 * Usage:
 *   OuariconPresetManager presets(apvts, "Ouaricon Marimba");
 *   presets.savePreset("My Sound");
 *   presets.loadPreset("Factory Lead");
 */
class OuariconPresetManager
{
public:
    //==========================================================================
    // Callbacks for custom state (e.g., tuning data)
    //==========================================================================

    /**
     * Callback to save custom state to preset JSON.
     * Called during savePreset() and getStateAsXml().
     * Return a juce::var containing your custom data.
     */
    using CustomSaveCallback = std::function<juce::var()>;

    /**
     * Callback to load custom state from preset JSON.
     * Called during loadPreset() and setStateFromXml().
     * Receives the custom data saved by CustomSaveCallback.
     */
    using CustomLoadCallback = std::function<void(const juce::var&)>;

    //==========================================================================
    // Construction
    //==========================================================================

    /**
     * Create a preset manager for the given plugin.
     *
     * @param apvts       Reference to the plugin's APVTS
     * @param pluginName  Plugin name (used for directory path)
     */
    OuariconPresetManager(juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& pluginName);

    ~OuariconPresetManager() = default;

    //==========================================================================
    // Custom state callbacks
    //==========================================================================

    /**
     * Set callbacks for saving/loading custom state.
     * Call this if your plugin has additional state beyond APVTS parameters.
     */
    void setCustomStateCallbacks(CustomSaveCallback saveCallback,
                                 CustomLoadCallback loadCallback)
    {
        customSave = std::move(saveCallback);
        customLoad = std::move(loadCallback);
    }

    //==========================================================================
    // Preset operations
    //==========================================================================

    /** Save current state to a user preset. */
    bool savePreset(const juce::String& presetName);

    /** Load a preset by name (searches Factory first, then User). */
    bool loadPreset(const juce::String& presetName);

    /** Load a preset from an arbitrary file path (for import). */
    bool loadPresetFromFile(const juce::File& file);

    /** Delete a user preset. Factory presets cannot be deleted. */
    bool deletePreset(const juce::String& presetName);

    //==========================================================================
    // Preset listing
    //==========================================================================

    /** Get sorted list of all preset names (Factory + User). */
    juce::StringArray getPresetList() const;

    /** Get current preset name. */
    juce::String getCurrentPresetName() const { return currentPresetName; }

    /** Set current preset name (for UI display). */
    void setCurrentPresetName(const juce::String& name) { currentPresetName = name; }

    /** Check if a preset is a factory preset (read-only). */
    bool isFactoryPreset(const juce::String& presetName) const;

    //==========================================================================
    // Navigation
    //==========================================================================

    /** Get name of next preset in list (wraps around). */
    juce::String getNextPreset() const;

    /** Get name of previous preset in list (wraps around). */
    juce::String getPreviousPreset() const;

    //==========================================================================
    // DAW session state
    //==========================================================================

    /** Get complete state as XML (for processor getStateInformation). */
    std::unique_ptr<juce::XmlElement> getStateAsXml() const;

    /** Restore state from XML (for processor setStateInformation). */
    void setStateFromXml(const juce::XmlElement* xml);

    //==========================================================================
    // Directory access
    //==========================================================================

    /** Get root presets directory (creates if needed). */
    juce::File getPresetsDirectory() const;

    /** Get factory presets directory. */
    juce::File getFactoryPresetsDirectory() const;

    /** Get user presets directory. */
    juce::File getUserPresetsDirectory() const;

    //==========================================================================
    // Factory preset initialization
    //==========================================================================

    /**
     * Factory preset definition for initialization.
     * Used by initializeFactoryPresets() to create default presets.
     */
    struct FactoryPresetDef
    {
        juce::String name;
        std::map<juce::String, float> parameters;  // Parameter ID -> normalized value
        juce::var customState;                      // Optional custom state data
    };

    /**
     * Initialize factory presets from definitions.
     * Call this once at plugin startup if factory presets don't exist.
     *
     * @param presets  Vector of factory preset definitions
     */
    void initializeFactoryPresets(const std::vector<FactoryPresetDef>& presets);

private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::String pluginName;
    juce::String currentPresetName { "Default" };

    // Custom state callbacks
    CustomSaveCallback customSave;
    CustomLoadCallback customLoad;

    // JSON helpers
    juce::var createPresetJson() const;
    bool applyPresetJson(const juce::var& presetData);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconPresetManager)
};


//==============================================================================
// INLINE IMPLEMENTATION
//==============================================================================

inline OuariconPresetManager::OuariconPresetManager(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& name)
    : parameters(apvts)
    , pluginName(name)
{
    // Ensure directories exist
    getFactoryPresetsDirectory().createDirectory();
    getUserPresetsDirectory().createDirectory();
}

inline juce::File OuariconPresetManager::getPresetsDirectory() const
{
    // ~/Library/{pluginName}/Presets/
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library")
        .getChildFile(pluginName)
        .getChildFile("Presets");
}

inline juce::File OuariconPresetManager::getFactoryPresetsDirectory() const
{
    return getPresetsDirectory().getChildFile("Factory");
}

inline juce::File OuariconPresetManager::getUserPresetsDirectory() const
{
    return getPresetsDirectory().getChildFile("User");
}

inline bool OuariconPresetManager::isFactoryPreset(const juce::String& presetName) const
{
    auto factoryFile = getFactoryPresetsDirectory().getChildFile(presetName + ".json");
    return factoryFile.existsAsFile();
}

inline juce::var OuariconPresetManager::createPresetJson() const
{
    auto* preset = new juce::DynamicObject();

    // Save all APVTS parameters
    auto* paramsObj = new juce::DynamicObject();
    for (auto* param : parameters.processor.getParameters())
    {
        if (auto* paramWithID = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            paramsObj->setProperty(paramWithID->getParameterID(),
                                   paramWithID->getValue());
        }
    }
    preset->setProperty("parameters", juce::var(paramsObj));

    // Save custom state if callback provided
    if (customSave)
    {
        preset->setProperty("customState", customSave());
    }

    // Metadata
    preset->setProperty("version", "1.0.0");
    preset->setProperty("plugin", pluginName);

    return juce::var(preset);
}

inline bool OuariconPresetManager::applyPresetJson(const juce::var& presetData)
{
    if (!presetData.isObject())
        return false;

    auto* preset = presetData.getDynamicObject();
    if (preset == nullptr)
        return false;

    // Restore parameters
    if (preset->hasProperty("parameters"))
    {
        auto paramsVar = preset->getProperty("parameters");
        if (auto* paramsObj = paramsVar.getDynamicObject())
        {
            for (auto& prop : paramsObj->getProperties())
            {
                if (auto* param = parameters.getParameter(prop.name.toString()))
                {
                    param->setValueNotifyingHost(static_cast<float>(prop.value));
                }
            }
        }
    }

    // Restore custom state if callback provided
    if (customLoad && preset->hasProperty("customState"))
    {
        customLoad(preset->getProperty("customState"));
    }

    return true;
}

inline bool OuariconPresetManager::savePreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    // Don't allow overwriting factory presets
    if (isFactoryPreset(presetName))
    {
        juce::Logger::writeToLog("[PresetManager] Cannot overwrite factory preset: " + presetName);
        return false;
    }

    auto presetFile = getUserPresetsDirectory().getChildFile(presetName + ".json");

    auto presetJson = createPresetJson();
    auto jsonString = juce::JSON::toString(presetJson, true);

    if (presetFile.replaceWithText(jsonString))
    {
        currentPresetName = presetName;
        juce::Logger::writeToLog("[PresetManager] Preset saved: " + presetName);
        return true;
    }

    return false;
}

inline bool OuariconPresetManager::loadPreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    // Check factory presets first, then user presets
    juce::File presetFile = getFactoryPresetsDirectory().getChildFile(presetName + ".json");
    if (!presetFile.existsAsFile())
    {
        presetFile = getUserPresetsDirectory().getChildFile(presetName + ".json");
    }

    if (!presetFile.existsAsFile())
    {
        juce::Logger::writeToLog("[PresetManager] Preset not found: " + presetName);
        return false;
    }

    auto jsonString = presetFile.loadFileAsString();
    auto presetData = juce::JSON::parse(jsonString);

    if (applyPresetJson(presetData))
    {
        currentPresetName = presetName;
        juce::Logger::writeToLog("[PresetManager] Preset loaded: " + presetName);
        return true;
    }

    return false;
}

inline bool OuariconPresetManager::loadPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        juce::Logger::writeToLog("[PresetManager] File not found: " + file.getFullPathName());
        return false;
    }

    auto jsonString = file.loadFileAsString();
    auto presetData = juce::JSON::parse(jsonString);

    if (applyPresetJson(presetData))
    {
        currentPresetName = file.getFileNameWithoutExtension();
        juce::Logger::writeToLog("[PresetManager] Preset loaded from file: " + file.getFullPathName());
        return true;
    }

    juce::Logger::writeToLog("[PresetManager] Failed to parse preset file: " + file.getFullPathName());
    return false;
}

inline bool OuariconPresetManager::deletePreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    // Don't allow deleting factory presets
    if (isFactoryPreset(presetName))
    {
        juce::Logger::writeToLog("[PresetManager] Cannot delete factory preset: " + presetName);
        return false;
    }

    auto presetFile = getUserPresetsDirectory().getChildFile(presetName + ".json");
    if (presetFile.existsAsFile())
    {
        if (presetFile.deleteFile())
        {
            if (currentPresetName == presetName)
                currentPresetName = "Default";
            return true;
        }
    }

    return false;
}

inline juce::StringArray OuariconPresetManager::getPresetList() const
{
    juce::StringArray presets;

    // Add factory presets first
    auto factoryDir = getFactoryPresetsDirectory();
    if (factoryDir.isDirectory())
    {
        for (const auto& file : factoryDir.findChildFiles(juce::File::findFiles, false, "*.json"))
        {
            presets.add(file.getFileNameWithoutExtension());
        }
    }

    // Add user presets
    auto userDir = getUserPresetsDirectory();
    if (userDir.isDirectory())
    {
        for (const auto& file : userDir.findChildFiles(juce::File::findFiles, false, "*.json"))
        {
            auto name = file.getFileNameWithoutExtension();
            if (!presets.contains(name))  // Avoid duplicates
                presets.add(name);
        }
    }

    presets.sort(true);  // Case-insensitive sort
    return presets;
}

inline juce::String OuariconPresetManager::getNextPreset() const
{
    auto presets = getPresetList();
    if (presets.isEmpty())
        return currentPresetName;

    int currentIndex = presets.indexOf(currentPresetName);
    if (currentIndex < 0)
        return presets[0];

    int nextIndex = (currentIndex + 1) % presets.size();
    return presets[nextIndex];
}

inline juce::String OuariconPresetManager::getPreviousPreset() const
{
    auto presets = getPresetList();
    if (presets.isEmpty())
        return currentPresetName;

    int currentIndex = presets.indexOf(currentPresetName);
    if (currentIndex < 0)
        return presets[0];

    int prevIndex = (currentIndex - 1 + presets.size()) % presets.size();
    return presets[prevIndex];
}

inline std::unique_ptr<juce::XmlElement> OuariconPresetManager::getStateAsXml() const
{
    // Start with APVTS state
    auto state = parameters.copyState();
    auto xml = state.createXml();

    if (xml != nullptr)
    {
        // Add custom state as child element if callback provided
        if (customSave)
        {
            auto customData = customSave();
            if (!customData.isVoid())
            {
                auto* customXml = xml->createNewChildElement("CustomState");
                customXml->setAttribute("data", juce::JSON::toString(customData));
            }
        }

        // Save current preset name
        xml->setAttribute("currentPreset", currentPresetName);
    }

    return xml;
}

inline void OuariconPresetManager::setStateFromXml(const juce::XmlElement* xml)
{
    if (xml == nullptr)
        return;

    // Restore APVTS state
    if (xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }

    // Restore custom state if callback provided
    if (customLoad)
    {
        if (auto* customXml = xml->getChildByName("CustomState"))
        {
            auto dataStr = customXml->getStringAttribute("data");
            if (dataStr.isNotEmpty())
            {
                auto customData = juce::JSON::parse(dataStr);
                customLoad(customData);
            }
        }
    }

    // Restore preset name
    currentPresetName = xml->getStringAttribute("currentPreset", "Default");
}

inline void OuariconPresetManager::initializeFactoryPresets(
    const std::vector<FactoryPresetDef>& presets)
{
    auto factoryDir = getFactoryPresetsDirectory();
    factoryDir.createDirectory();

    for (const auto& preset : presets)
    {
        auto presetFile = factoryDir.getChildFile(preset.name + ".json");

        auto* presetObj = new juce::DynamicObject();

        // Parameters
        auto* paramsObj = new juce::DynamicObject();
        for (const auto& [paramId, value] : preset.parameters)
        {
            paramsObj->setProperty(paramId, value);
        }
        presetObj->setProperty("parameters", juce::var(paramsObj));

        // Custom state
        if (!preset.customState.isVoid())
        {
            presetObj->setProperty("customState", preset.customState);
        }

        // Metadata
        presetObj->setProperty("version", "1.0.0");
        presetObj->setProperty("plugin", pluginName);
        presetObj->setProperty("factory", true);

        auto jsonString = juce::JSON::toString(juce::var(presetObj), true);
        presetFile.replaceWithText(jsonString);
    }

    juce::Logger::writeToLog("[PresetManager] Factory presets initialized: " +
                             juce::String(presets.size()));
}
