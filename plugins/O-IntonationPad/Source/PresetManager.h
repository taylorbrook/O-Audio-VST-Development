/*
  ==============================================================================

    PresetManager - Preset save/load/browse system
    O-IntonationPad / Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>

class OIntonationPadAudioProcessor;

class PresetManager
{
public:
    struct PresetInfo
    {
        juce::String name;
        juce::String category;
        bool isFactory;
    };

    explicit PresetManager(OIntonationPadAudioProcessor& processor);

    // Preset operations
    bool savePreset(const juce::String& name, const juce::String& category);
    bool loadPreset(const juce::String& name, bool isFactory);
    bool deletePreset(const juce::String& name);
    bool renamePreset(const juce::String& oldName, const juce::String& newName);

    // Navigation
    bool loadNextPreset();
    bool loadPrevPreset();

    // Queries
    juce::String getCurrentPresetName() const { return currentPresetName; }
    std::vector<PresetInfo> getAllPresets();
    juce::StringArray getCategories() const;

    // Called when parameters change (clears preset name if dirty)
    void markDirty() { isDirty = true; }

private:
    OIntonationPadAudioProcessor& processorRef;
    juce::String currentPresetName { "Init" };
    bool isDirty = false;

    juce::File getUserPresetFolder() const;
    juce::File getPresetFile(const juce::String& name) const;

    // Factory preset data
    struct FactoryPresetData
    {
        const char* name;
        const char* category;
        // Parameter overrides (paramID -> value as string)
        std::vector<std::pair<juce::String, juce::String>> paramOverrides;
        // Tuning state
        int temperamentPreset;       // index into BuiltInPreset enum
        juce::String tuningName;
        int tonicNote;
    };

    static const std::vector<FactoryPresetData>& getFactoryPresetData();

    // Build full state XML from factory preset definition
    juce::String buildFactoryPresetXml(const FactoryPresetData& preset) const;

    // Build ordered flat list for navigation
    std::vector<PresetInfo> buildFlatList();
    int findPresetIndex(const juce::String& name, const std::vector<PresetInfo>& list) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
