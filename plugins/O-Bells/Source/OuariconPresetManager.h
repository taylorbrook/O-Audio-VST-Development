/*
  ==============================================================================

    OuariconPresetManager.h
    Ouaricon Module System - Preset Persistence

    Extended for O-Bells with category support.
    Handles JSON serialization, factory/user presets, and navigation.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <vector>
#include <map>

/**
 * Generic preset manager for Ouaricon plugins.
 *
 * Presets are stored as JSON files in:
 *   ~/Library/{pluginName}/Presets/
 *
 * Subdirectories:
 *   - Factory/{Category}/ : Read-only presets by category
 *   - User/               : User-created presets
 */
class OuariconPresetManager
{
public:
    using CustomSaveCallback = std::function<juce::var()>;
    using CustomLoadCallback = std::function<void(const juce::var&)>;

    OuariconPresetManager(juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& pluginName);

    ~OuariconPresetManager() = default;

    void setCustomStateCallbacks(CustomSaveCallback saveCallback,
                                 CustomLoadCallback loadCallback)
    {
        customSave = std::move(saveCallback);
        customLoad = std::move(loadCallback);
    }

    // Preset operations
    bool savePreset(const juce::String& presetName);
    bool loadPreset(const juce::String& presetName);
    bool loadPresetFromCategory(const juce::String& category, const juce::String& presetName);
    bool loadPresetFromFile(const juce::File& file);
    bool deletePreset(const juce::String& presetName);

    // Preset listing
    juce::StringArray getPresetList() const;
    std::map<juce::String, juce::StringArray> getPresetListWithCategories() const;
    juce::String getCurrentPresetName() const { return currentPresetName; }
    void setCurrentPresetName(const juce::String& name) { currentPresetName = name; }
    bool isFactoryPreset(const juce::String& presetName) const;

    // Navigation (across all categories flattened)
    juce::String getNextPreset();
    juce::String getPreviousPreset();

    // DAW session state
    std::unique_ptr<juce::XmlElement> getStateAsXml() const;
    void setStateFromXml(const juce::XmlElement* xml);

    // Directory access
    juce::File getPresetsDirectory() const;
    juce::File getFactoryPresetsDirectory() const;
    juce::File getUserPresetsDirectory() const;

    // Category-based factory preset definition
    struct FactoryPresetDef
    {
        juce::String category;
        juce::String name;
        std::map<juce::String, float> parameters;
        juce::var customState;
    };

    void initializeFactoryPresets(const std::vector<FactoryPresetDef>& presets);
    bool factoryPresetsExist() const;

private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::String pluginName;
    juce::String currentPresetName { "Default" };
    juce::StringArray flatPresetList;  // Cached for navigation

    CustomSaveCallback customSave;
    CustomLoadCallback customLoad;

    juce::var createPresetJson() const;
    bool applyPresetJson(const juce::var& presetData);
    void rebuildFlatPresetList();

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
}

inline juce::File OuariconPresetManager::getPresetsDirectory() const
{
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

inline bool OuariconPresetManager::factoryPresetsExist() const
{
    auto factoryDir = getFactoryPresetsDirectory();
    if (!factoryDir.isDirectory())
        return false;

    // Check if any category subdirectories have presets
    for (const auto& subdir : factoryDir.findChildFiles(juce::File::findDirectories, false))
    {
        if (subdir.findChildFiles(juce::File::findFiles, false, "*.json").size() > 0)
            return true;
    }
    return false;
}

inline bool OuariconPresetManager::isFactoryPreset(const juce::String& presetName) const
{
    auto factoryDir = getFactoryPresetsDirectory();
    if (!factoryDir.isDirectory())
        return false;

    // Search in all category subdirectories
    for (const auto& subdir : factoryDir.findChildFiles(juce::File::findDirectories, false))
    {
        if (subdir.getChildFile(presetName + ".json").existsAsFile())
            return true;
    }
    return false;
}

inline juce::var OuariconPresetManager::createPresetJson() const
{
    auto* preset = new juce::DynamicObject();

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

    if (customSave)
        preset->setProperty("customState", customSave());

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

    // WR-01: reset parameters to their defaults before applying the preset's keys.
    // Factory presets are partial (they never name the FX section, lpFilter, etc.),
    // so without this pass omitted keys inherit stale state from the previously
    // loaded preset (pattern_preset_apply_needs_reset_to_defaults).
    //
    // Exception: the global tuning (tuning_*) is a cross-cutting concern, not part
    // of a timbre preset. Factory presets carry no tuning keys, so resetting them
    // would snap the user's temperament/A4/stretch back to defaults on every load.
    // User presets that DO save tuning_* still recall correctly (they name the keys).
    for (auto* param : parameters.processor.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*>(param);
        if (rp == nullptr)
            continue;
        if (auto* pid = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            if (pid->getParameterID().startsWith("tuning_"))
                continue;
        rp->setValueNotifyingHost(rp->getDefaultValue());
    }

    if (preset->hasProperty("parameters"))
    {
        auto paramsVar = preset->getProperty("parameters");
        if (auto* paramsObj = paramsVar.getDynamicObject())
        {
            for (auto& prop : paramsObj->getProperties())
            {
                if (auto* param = parameters.getParameter(prop.name.toString()))
                    param->setValueNotifyingHost(static_cast<float>(prop.value));
            }
        }
    }

    if (customLoad && preset->hasProperty("customState"))
        customLoad(preset->getProperty("customState"));

    return true;
}

inline bool OuariconPresetManager::savePreset(const juce::String& presetName)
{
    // WR-02: sanitize before using the name as a filename. A raw name containing
    // "/" (or "\ :" etc.) is treated as a path separator by getChildFile, so the
    // write lands in a phantom subdir or fails and the non-recursive getPresetList
    // never surfaces it (critical_preset_name_slash_path_separator).
    auto safeName = juce::File::createLegalFileName(presetName).trim();
    if (safeName.isEmpty())
        return false;

    if (isFactoryPreset(safeName))
    {
        juce::Logger::writeToLog("[PresetManager] Cannot overwrite factory preset: " + safeName);
        return false;
    }

    getUserPresetsDirectory().createDirectory();
    auto presetFile = getUserPresetsDirectory().getChildFile(safeName + ".json");
    auto presetJson = createPresetJson();
    auto jsonString = juce::JSON::toString(presetJson, true);

    if (presetFile.replaceWithText(jsonString))
    {
        currentPresetName = safeName;
        rebuildFlatPresetList();
        return true;
    }
    return false;
}

inline bool OuariconPresetManager::loadPreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    // Search factory categories first
    auto factoryDir = getFactoryPresetsDirectory();
    if (factoryDir.isDirectory())
    {
        for (const auto& subdir : factoryDir.findChildFiles(juce::File::findDirectories, false))
        {
            auto presetFile = subdir.getChildFile(presetName + ".json");
            if (presetFile.existsAsFile())
            {
                auto jsonString = presetFile.loadFileAsString();
                auto presetData = juce::JSON::parse(jsonString);
                if (applyPresetJson(presetData))
                {
                    currentPresetName = presetName;
                    return true;
                }
            }
        }
    }

    // Then user presets
    auto userFile = getUserPresetsDirectory().getChildFile(presetName + ".json");
    if (userFile.existsAsFile())
    {
        auto jsonString = userFile.loadFileAsString();
        auto presetData = juce::JSON::parse(jsonString);
        if (applyPresetJson(presetData))
        {
            currentPresetName = presetName;
            return true;
        }
    }

    return false;
}

inline bool OuariconPresetManager::loadPresetFromCategory(const juce::String& category,
                                                           const juce::String& presetName)
{
    if (category.isEmpty() || presetName.isEmpty())
        return false;

    auto presetFile = getFactoryPresetsDirectory()
        .getChildFile(category)
        .getChildFile(presetName + ".json");

    if (!presetFile.existsAsFile())
        return false;

    auto jsonString = presetFile.loadFileAsString();
    auto presetData = juce::JSON::parse(jsonString);

    if (applyPresetJson(presetData))
    {
        currentPresetName = presetName;
        return true;
    }
    return false;
}

inline bool OuariconPresetManager::loadPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    auto jsonString = file.loadFileAsString();
    auto presetData = juce::JSON::parse(jsonString);

    if (applyPresetJson(presetData))
    {
        currentPresetName = file.getFileNameWithoutExtension();
        return true;
    }
    return false;
}

inline bool OuariconPresetManager::deletePreset(const juce::String& presetName)
{
    if (presetName.isEmpty() || isFactoryPreset(presetName))
        return false;

    auto presetFile = getUserPresetsDirectory().getChildFile(presetName + ".json");
    if (presetFile.existsAsFile() && presetFile.deleteFile())
    {
        if (currentPresetName == presetName)
            currentPresetName = "Default";
        rebuildFlatPresetList();
        return true;
    }
    return false;
}

inline juce::StringArray OuariconPresetManager::getPresetList() const
{
    juce::StringArray presets;

    // Factory presets from all categories
    auto factoryDir = getFactoryPresetsDirectory();
    if (factoryDir.isDirectory())
    {
        for (const auto& subdir : factoryDir.findChildFiles(juce::File::findDirectories, false))
        {
            for (const auto& file : subdir.findChildFiles(juce::File::findFiles, false, "*.json"))
                presets.add(file.getFileNameWithoutExtension());
        }
    }

    // User presets
    auto userDir = getUserPresetsDirectory();
    if (userDir.isDirectory())
    {
        for (const auto& file : userDir.findChildFiles(juce::File::findFiles, false, "*.json"))
        {
            auto name = file.getFileNameWithoutExtension();
            if (!presets.contains(name))
                presets.add(name);
        }
    }

    presets.sort(true);
    return presets;
}

inline std::map<juce::String, juce::StringArray> OuariconPresetManager::getPresetListWithCategories() const
{
    std::map<juce::String, juce::StringArray> categorized;

    // Factory presets by category
    auto factoryDir = getFactoryPresetsDirectory();
    if (factoryDir.isDirectory())
    {
        for (const auto& subdir : factoryDir.findChildFiles(juce::File::findDirectories, false))
        {
            juce::StringArray categoryPresets;
            for (const auto& file : subdir.findChildFiles(juce::File::findFiles, false, "*.json"))
                categoryPresets.add(file.getFileNameWithoutExtension());

            if (!categoryPresets.isEmpty())
            {
                categoryPresets.sort(true);
                categorized[subdir.getFileName()] = categoryPresets;
            }
        }
    }

    // User presets under "User" category
    auto userDir = getUserPresetsDirectory();
    if (userDir.isDirectory())
    {
        juce::StringArray userPresets;
        for (const auto& file : userDir.findChildFiles(juce::File::findFiles, false, "*.json"))
            userPresets.add(file.getFileNameWithoutExtension());

        if (!userPresets.isEmpty())
        {
            userPresets.sort(true);
            categorized["User"] = userPresets;
        }
    }

    return categorized;
}

inline void OuariconPresetManager::rebuildFlatPresetList()
{
    flatPresetList = getPresetList();
}

inline juce::String OuariconPresetManager::getNextPreset()
{
    if (flatPresetList.isEmpty())
        rebuildFlatPresetList();

    if (flatPresetList.isEmpty())
        return currentPresetName;

    int idx = flatPresetList.indexOf(currentPresetName);
    if (idx < 0) idx = -1;

    int nextIdx = (idx + 1) % flatPresetList.size();
    auto nextName = flatPresetList[nextIdx];
    loadPreset(nextName);
    return nextName;
}

inline juce::String OuariconPresetManager::getPreviousPreset()
{
    if (flatPresetList.isEmpty())
        rebuildFlatPresetList();

    if (flatPresetList.isEmpty())
        return currentPresetName;

    int idx = flatPresetList.indexOf(currentPresetName);
    if (idx < 0) idx = 0;

    int prevIdx = (idx - 1 + flatPresetList.size()) % flatPresetList.size();
    auto prevName = flatPresetList[prevIdx];
    loadPreset(prevName);
    return prevName;
}

inline std::unique_ptr<juce::XmlElement> OuariconPresetManager::getStateAsXml() const
{
    auto state = parameters.copyState();
    auto xml = state.createXml();

    if (xml != nullptr)
    {
        if (customSave)
        {
            auto customData = customSave();
            if (!customData.isVoid())
            {
                auto* customXml = xml->createNewChildElement("CustomState");
                customXml->setAttribute("data", juce::JSON::toString(customData));
            }
        }
        xml->setAttribute("currentPreset", currentPresetName);
    }
    return xml;
}

inline void OuariconPresetManager::setStateFromXml(const juce::XmlElement* xml)
{
    if (xml == nullptr)
        return;

    if (xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));

    if (customLoad)
    {
        if (auto* customXml = xml->getChildByName("CustomState"))
        {
            auto dataStr = customXml->getStringAttribute("data");
            if (dataStr.isNotEmpty())
                customLoad(juce::JSON::parse(dataStr));
        }
    }

    currentPresetName = xml->getStringAttribute("currentPreset", "Default");
}

inline void OuariconPresetManager::initializeFactoryPresets(
    const std::vector<FactoryPresetDef>& presets)
{
    auto factoryDir = getFactoryPresetsDirectory();

    for (const auto& preset : presets)
    {
        auto categoryDir = factoryDir.getChildFile(preset.category);
        categoryDir.createDirectory();

        auto presetFile = categoryDir.getChildFile(preset.name + ".json");

        auto* presetObj = new juce::DynamicObject();
        auto* paramsObj = new juce::DynamicObject();

        for (const auto& [paramId, value] : preset.parameters)
        {
            // CR-01: factory tables are authored in engineering units, but presets
            // are applied through the *normalized* setValueNotifyingHost (see
            // applyPresetJson / createPresetJson). Store the normalized value so
            // the on-disk convention matches user presets and skewed/wide-range
            // params recall correctly (pattern_factory_preset_normalized_ignores_skew).
            float stored = value;
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(parameters.getParameter(paramId)))
                stored = rp->getNormalisableRange().convertTo0to1(value);
            paramsObj->setProperty(paramId, stored);
        }

        presetObj->setProperty("parameters", juce::var(paramsObj));

        if (!preset.customState.isVoid())
            presetObj->setProperty("customState", preset.customState);

        presetObj->setProperty("version", "1.0.0");
        presetObj->setProperty("plugin", pluginName);
        presetObj->setProperty("category", preset.category);
        presetObj->setProperty("factory", true);

        auto jsonString = juce::JSON::toString(juce::var(presetObj), true);
        presetFile.replaceWithText(jsonString);
    }

    rebuildFlatPresetList();
}
