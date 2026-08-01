/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    OuariconPresetManager.h
    Ouaricon Module System - Preset Persistence

    Extended for O-Prism with category support + parameter exclusion.
    Handles JSON serialization, factory/user presets, and navigation.
    Excluded parameters are preserved across preset switches (e.g., tuning).

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

    void initializeFactoryPresets(const std::vector<FactoryPresetDef>& presets,
                                  const juce::String& versionStamp = {});
    bool factoryPresetsExist() const;

    /** Version stamp written by the last initializeFactoryPresets (empty if
        none). Callers compare against the current plugin version so factory
        JSON regenerates after updates instead of being pinned to the first
        run's parameter set (WR-08). */
    juce::String getFactoryPresetsVersion() const;

    // Parameters in this list are NEVER written to preset JSON and NEVER
    // overwritten when loading a preset. Used to preserve global state like
    // tuning settings across preset switches.
    juce::StringArray excludedParameterIds;

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

    /** Preset names are used verbatim as file names — strip path separators
        and other illegal characters so "A/B" can't silently vanish and "../x"
        can't escape the preset directory (WR-09). */
    static juce::String legalPresetFileName(const juce::String& name)
    {
        auto legal = juce::File::createLegalFileName(name.trim());
        legal = legal.replaceCharacter('/', '-').replaceCharacter('\\', '-');
        while (legal.startsWithChar('.'))
            legal = legal.substring(1);
        return legal.trim();
    }

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
            const auto& id = paramWithID->getParameterID();
            if (excludedParameterIds.contains(id))
                continue;
            paramsObj->setProperty(id, paramWithID->getValue());
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

    if (preset->hasProperty("parameters"))
    {
        auto paramsVar = preset->getProperty("parameters");
        if (auto* paramsObj = paramsVar.getDynamicObject())
        {
            // Reset all non-excluded parameters to defaults BEFORE applying —
            // partial presets (hand-authored defs, saves from older versions
            // with fewer params) must not silently inherit the previous
            // patch's state for omitted keys (WR-08).
            for (auto* param : parameters.processor.getParameters())
            {
                if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param))
                {
                    if (!excludedParameterIds.contains(rangedParam->getParameterID()))
                        rangedParam->setValueNotifyingHost(rangedParam->getDefaultValue());
                }
            }

            for (auto& prop : paramsObj->getProperties())
            {
                const auto id = prop.name.toString();
                if (excludedParameterIds.contains(id))
                    continue;
                if (auto* param = parameters.getParameter(id))
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
    const auto fileName = legalPresetFileName(presetName);
    if (fileName.isEmpty())
        return false;

    if (isFactoryPreset(fileName))
    {
        juce::Logger::writeToLog("[PresetManager] Cannot overwrite factory preset: " + fileName);
        return false;
    }

    getUserPresetsDirectory().createDirectory();
    auto presetFile = getUserPresetsDirectory().getChildFile(fileName + ".json");
    auto presetJson = createPresetJson();
    auto jsonString = juce::JSON::toString(presetJson, true);

    if (presetFile.replaceWithText(jsonString))
    {
        // Track the sanitized name — it is what getPresetList/loadPreset see
        currentPresetName = fileName;
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

inline bool OuariconPresetManager::deletePreset(const juce::String& presetNameIn)
{
    const auto presetName = legalPresetFileName(presetNameIn);
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

inline juce::String OuariconPresetManager::getFactoryPresetsVersion() const
{
    auto versionFile = getFactoryPresetsDirectory().getChildFile(".factory-version");
    return versionFile.existsAsFile() ? versionFile.loadFileAsString().trim() : juce::String();
}

inline void OuariconPresetManager::initializeFactoryPresets(
    const std::vector<FactoryPresetDef>& presets, const juce::String& versionStamp)
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
            if (excludedParameterIds.contains(paramId))
                continue;
            paramsObj->setProperty(paramId, value);
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

    if (versionStamp.isNotEmpty())
    {
        factoryDir.createDirectory();
        factoryDir.getChildFile(".factory-version").replaceWithText(versionStamp);
    }

    rebuildFlatPresetList();
}
