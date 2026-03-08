/*
  ==============================================================================

    UserWavetableManager.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    Manages user-imported wavetables with persistent storage at
    ~/.ouaricon/wavetables/ so they survive sessions.

  ==============================================================================
*/

#pragma once
#include "WavetableData.h"
#include "WavetableImporter.h"
#include <JuceHeader.h>
#include <memory>
#include <vector>

struct UserWavetableEntry
{
    juce::String name;
    std::unique_ptr<WavetableData> table;
};

class UserWavetableManager
{
public:
    UserWavetableManager();

    /** Load all user wavetables from persistent directory. */
    void loadFromDisk();

    /** Import from file, save to disk, return name (or empty on failure). */
    juce::String importFile (const juce::File& file);

    /** Import from memory (drag-and-drop), save to disk, return name. */
    juce::String importFromMemory (const void* data, size_t sizeInBytes,
                                   const juce::String& suggestedName);

    /** Delete a user wavetable by name. */
    bool deleteWavetable (const juce::String& name);

    /** Get table by name (nullptr if not found). */
    const WavetableData* getTable (const juce::String& name) const;

    /** Get all user wavetable names. */
    juce::StringArray getTableNames() const;

    /** Get number of user wavetables. */
    int getNumTables() const { return static_cast<int> (entries.size()); }

    /** Get persistent directory. */
    juce::File getWavetableDirectory() const { return wavetableDir; }

private:
    juce::File wavetableDir;
    std::vector<UserWavetableEntry> entries;

    bool saveToWav (const WavetableData& table, const juce::File& file);
    juce::String makeUniqueName (const juce::String& baseName) const;
};
