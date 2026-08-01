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

    UserWavetableManager.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    Persistent user wavetable storage and management.

  ==============================================================================
*/

#include "UserWavetableManager.h"

UserWavetableManager::UserWavetableManager()
{
    wavetableDir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                       .getChildFile (".ouaricon")
                       .getChildFile ("wavetables");
    wavetableDir.createDirectory();
}

void UserWavetableManager::loadFromDisk()
{
    entries.clear();

    auto files = wavetableDir.findChildFiles (juce::File::findFiles, false, "*.wav");
    files.sort();

    for (const auto& file : files)
    {
        auto importResult = WavetableImporter::importFromFile (file);
        if (importResult.success && importResult.table)
        {
            UserWavetableEntry entry;
            entry.name = file.getFileNameWithoutExtension();
            entry.table = std::move (importResult.table);
            entries.push_back (std::move (entry));
        }
    }
}

juce::String UserWavetableManager::importFile (const juce::File& file)
{
    auto importResult = WavetableImporter::importFromFile (file);
    if (! importResult.success || ! importResult.table)
        return {};

    auto baseName = file.getFileNameWithoutExtension();
    auto uniqueName = makeUniqueName (baseName);
    auto destFile = wavetableDir.getChildFile (uniqueName + ".wav");

    if (! saveToWav (*importResult.table, destFile))
        return {};

    UserWavetableEntry entry;
    entry.name = uniqueName;
    entry.table = std::move (importResult.table);
    entries.push_back (std::move (entry));

    return uniqueName;
}

juce::String UserWavetableManager::importFromMemory (const void* data, size_t sizeInBytes,
                                                      const juce::String& suggestedName)
{
    auto importResult = WavetableImporter::importFromMemory (data, sizeInBytes);
    if (! importResult.success || ! importResult.table)
        return {};

    auto baseName = suggestedName.isNotEmpty() ? suggestedName : "Imported";
    // Strip file extension if present
    if (baseName.containsChar ('.'))
        baseName = baseName.upToLastOccurrenceOf (".", false, false);

    auto uniqueName = makeUniqueName (baseName);
    auto destFile = wavetableDir.getChildFile (uniqueName + ".wav");

    if (! saveToWav (*importResult.table, destFile))
        return {};

    UserWavetableEntry entry;
    entry.name = uniqueName;
    entry.table = std::move (importResult.table);
    entries.push_back (std::move (entry));

    return uniqueName;
}

std::unique_ptr<WavetableData> UserWavetableManager::removeWavetable (const juce::String& name)
{
    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        if (it->name == name)
        {
            // The name comes from the WebView — never let a relative path
            // escape the wavetable directory as a deletion primitive (WR-10)
            auto wavFile = wavetableDir.getChildFile (legalTableName (name) + ".wav");
            if (wavFile.isAChildOf (wavetableDir))
                wavFile.deleteFile();
            auto removed = std::move (it->table);
            entries.erase (it);
            return removed;
        }
    }
    return nullptr;
}

bool UserWavetableManager::replaceOrInsertFromFile (const juce::String& name, const juce::File& file,
                                                    std::unique_ptr<WavetableData>& replacedOut)
{
    replacedOut = nullptr;

    auto importResult = WavetableImporter::importFromFile (file);
    if (! importResult.success || ! importResult.table)
        return false;

    for (auto& entry : entries)
    {
        if (entry.name == name)
        {
            replacedOut = std::move (entry.table);
            entry.table = std::move (importResult.table);
            return true;
        }
    }

    UserWavetableEntry entry;
    entry.name = name;
    entry.table = std::move (importResult.table);
    entries.push_back (std::move (entry));
    return true;
}

const WavetableData* UserWavetableManager::getTable (const juce::String& name) const
{
    for (const auto& entry : entries)
    {
        if (entry.name == name)
            return entry.table.get();
    }
    return nullptr;
}

juce::StringArray UserWavetableManager::getTableNames() const
{
    juce::StringArray names;
    for (const auto& entry : entries)
        names.add (entry.name);
    return names;
}

bool UserWavetableManager::saveToWav (const WavetableData& table, const juce::File& file)
{
    // Save as 32-bit float WAV: all frames concatenated (level 0 only)
    int totalSamples = table.numFrames * WavetableData::kTableSize;
    juce::AudioBuffer<float> buffer (1, totalSamples);

    for (int frame = 0; frame < table.numFrames; ++frame)
    {
        const float* frameData = table.getFrameData (0, frame);
        int startSample = frame * WavetableData::kTableSize;
        for (int i = 0; i < WavetableData::kTableSize; ++i)
            buffer.setSample (0, startSample + i, frameData[i]);
    }

    // FileOutputStream positions at end-of-file — delete first or an
    // overwrite appends a second WAV after the old one (stale data wins
    // on the next import).
    file.deleteFile();

    auto outputStream = std::make_unique<juce::FileOutputStream> (file);
    if (! outputStream->openedOk())
        return false;

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wavFormat.createWriterFor (outputStream.get(), 44100.0, 1, 32, {}, 0));

    if (! writer)
        return false;

    outputStream.release(); // Writer takes ownership
    return writer->writeFromAudioSampleBuffer (buffer, 0, totalSamples);
}

juce::String UserWavetableManager::makeUniqueName (const juce::String& baseName) const
{
    bool exists = false;
    for (const auto& entry : entries)
    {
        if (entry.name == baseName)
        {
            exists = true;
            break;
        }
    }

    if (! exists)
        return baseName;

    for (int i = 2; i < 1000; ++i)
    {
        auto candidate = baseName + " " + juce::String (i);
        bool found = false;
        for (const auto& entry : entries)
        {
            if (entry.name == candidate)
            {
                found = true;
                break;
            }
        }
        if (! found)
            return candidate;
    }

    return baseName + " " + juce::String (juce::Time::currentTimeMillis());
}
