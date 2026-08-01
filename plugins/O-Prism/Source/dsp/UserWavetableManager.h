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

    /** Delete a user wavetable's .wav and remove it from the list. Returns the
        removed table (nullptr if not found) — the audio thread may still be
        reading it, so the caller must retire it, never free it immediately. */
    std::unique_ptr<WavetableData> removeWavetable (const juce::String& name);

    /** Import a freshly-saved .wav and insert it under `name`, replacing any
        existing entry. On success `replacedOut` receives the old table (nullptr
        if the name was new) for the caller to retire. */
    bool replaceOrInsertFromFile (const juce::String& name, const juce::File& file,
                                  std::unique_ptr<WavetableData>& replacedOut);

    /** Get table by name (nullptr if not found). */
    const WavetableData* getTable (const juce::String& name) const;

    /** Get all user wavetable names. */
    juce::StringArray getTableNames() const;

    /** Get number of user wavetables. */
    int getNumTables() const { return static_cast<int> (entries.size()); }

    /** Get persistent directory. */
    juce::File getWavetableDirectory() const { return wavetableDir; }

    /** Wavetable names arrive from the WebView and become file names — strip
        path separators and other illegal characters so "../x" can't write or
        delete outside the wavetable directory (WR-10). */
    static juce::String legalTableName (const juce::String& name)
    {
        auto legal = juce::File::createLegalFileName (name.trim());
        legal = legal.replaceCharacter ('/', '-').replaceCharacter ('\\', '-');
        while (legal.startsWithChar ('.'))
            legal = legal.substring (1);
        return legal.trim();
    }

private:
    juce::File wavetableDir;
    std::vector<UserWavetableEntry> entries;

    bool saveToWav (const WavetableData& table, const juce::File& file);
    juce::String makeUniqueName (const juce::String& baseName) const;
};
