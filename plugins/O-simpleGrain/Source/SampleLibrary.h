/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    SampleLibrary.h — v1.9.0. The sample library, kept SEPARATE from presets
    (FRCTL GRN 3 style; research/granular-synthesis-2026-update.md §5 item 10).

    A flat folder of audio files beside the preset-manager's Presets folder:
        ~/Library/O-simpleGrain/Samples/
    Every sound you drop or pick is copied in (the decoded, 10 s-capped copy the
    session already embeds — FLAC or 32-bit float WAV), de-duplicated by content.
    A preset stores only a REFERENCE to a library entry (file name + content hash
    + byte size), never the audio, so presets stay small and one sample serves
    many presets. Sessions still embed their own copy, so a project never depends
    on the library being present.

    Message thread / host-controlled threads only (file I/O). Stateless apart
    from the root folder: every call reads the disk, so two plugin instances
    sharing the folder always see the same list.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

class SampleLibrary
{
public:
    explicit SampleLibrary (juce::File rootFolder) : root (std::move (rootFolder)) {}

    juce::File getFolder() const { return root; }

    static bool isAudioName (const juce::String& name)
    {
        const auto ext = name.fromLastOccurrenceOf (".", false, false).toLowerCase();
        return ext == "flac" || ext == "wav" || ext == "aif" || ext == "aiff";
    }

    /** Library entry names (file names, with extension), case-insensitively sorted. */
    juce::StringArray list() const
    {
        juce::StringArray names;
        if (root.isDirectory())
            for (const auto& f : root.findChildFiles (juce::File::findFiles, false, "*"))
                // Skip dot-files by NAME (incl. the ".part" of an add in flight),
                // not by the OS hidden flag.
                if (! f.getFileName().startsWithChar ('.') && isAudioName (f.getFileName()))
                    names.add (f.getFileName());
        names.sort (true);
        return names;
    }

    /** The file for an entry name, or File{} when the name is unsafe or absent. */
    juce::File fileFor (const juce::String& name) const
    {
        if (! isSafeName (name))
            return {};
        const auto f = root.getChildFile (name);
        return f.existsAsFile() ? f : juce::File{};
    }

    /** True when `file` lives directly in the library folder. */
    bool contains (const juce::File& file) const
    {
        return file.getParentDirectory() == root && isAudioName (file.getFileName());
    }

    /** Content identity: FNV-1a 64 over the bytes, as 16 hex digits. */
    static juce::String hashOf (const void* data, size_t numBytes)
    {
        juce::uint64 h = 0xcbf29ce484222325ULL;
        const auto* p = static_cast<const juce::uint8*> (data);
        for (size_t i = 0; i < numBytes; ++i)
        {
            h ^= p[i];
            h *= 0x100000001b3ULL;
        }
        return juce::String::toHexString ((juce::int64) h).paddedLeft ('0', 16);
    }

    static juce::String hashOf (const juce::File& f)
    {
        juce::MemoryBlock mb;
        return f.loadFileAsData (mb) ? hashOf (mb.getData(), mb.getSize()) : juce::String();
    }

    /** An entry with exactly these bytes (size compared first, so only same-size
        files are ever read and hashed). Empty when there is none. */
    juce::String findByContent (juce::int64 size, const juce::String& hash) const
    {
        if (size <= 0 || hash.isEmpty())
            return {};
        for (const auto& name : list())
        {
            const auto f = root.getChildFile (name);
            if (f.getSize() == size && hashOf (f) == hash)
                return name;
        }
        return {};
    }

    /** Add `bytes` under a name derived from `displayName`; returns the entry name.
        Identical content already in the library returns that entry and writes
        nothing. A different sound with the same name gets " 2", " 3", … */
    juce::String add (const juce::MemoryBlock& bytes, const juce::String& displayName,
                      const juce::String& extension)
    {
        if (bytes.getSize() == 0)
            return {};

        const auto hash = hashOf (bytes.getData(), bytes.getSize());
        const auto existing = findByContent ((juce::int64) bytes.getSize(), hash);
        if (existing.isNotEmpty())
            return existing;

        if (! root.createDirectory())
            return {};

        auto stem = juce::File::createLegalFileName (
                        displayName.upToLastOccurrenceOf (".", false, false)).trim();
        if (stem.isEmpty() || stem.startsWithChar ('.'))
            stem = "sample";
        stem = stem.substring (0, 80);

        const auto ext = extension.startsWithChar ('.') ? extension : "." + extension;
        auto target = root.getChildFile (stem + ext);
        for (int n = 2; target.exists() && n < 1000; ++n)
            target = root.getChildFile (stem + " " + juce::String (n) + ext);
        if (target.exists())
            return {};

        // Write to a temp file and move it in, so a second instance listing the
        // folder never sees a half-written entry.
        auto tmp = root.getChildFile ("." + target.getFileName() + ".part");
        if (! tmp.replaceWithData (bytes.getData(), bytes.getSize()))
            return {};
        if (! tmp.moveFileTo (target))
        {
            tmp.deleteFile();
            return {};
        }
        return target.getFileName();
    }

    /** Delete an entry. The live sound is unaffected (the processor holds its own
        copy, and the session embeds it). */
    bool remove (const juce::String& name)
    {
        const auto f = fileFor (name);
        return f != juce::File{} && f.deleteFile();
    }

private:
    static bool isSafeName (const juce::String& name)
    {
        return name.isNotEmpty() && ! name.containsAnyOf ("/\\:") && name != "." && name != ".."
            && ! name.startsWithChar ('.') && isAudioName (name);
    }

    juce::File root;
};
