/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    LyricsEngine.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Thread-safe syllable queue for lyrics-driven parameter automation.
    JS sends parsed syllable targets; audio thread reads on note-on.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class LyricsEngine
{
public:
    struct SyllableTarget
    {
        // Vowel nucleus targets
        float vowelX = 0.5f;
        float vowelY = 0.5f;

        // Onset consonant targets
        float consonantTone = 0.5f;      // Place (0-1)
        float sibilance = 0.5f;          // Manner (0-1)
        float consonantVoicing = 0.5f;
        float consonantLevel = 0.0f;     // 0 = no consonant onset

        // Nasal targets
        float nasalCoupling = 0.0f;
        float nasalPlace = 0.5f;

        bool hasConsonant = false;
    };

    static constexpr int kMaxSyllables = 256;

    // Called from message thread (nativeFunction) — sets the full syllable schedule
    void setSyllables (const SyllableTarget* targets, int count)
    {
        juce::SpinLock::ScopedLockType lock (syllableLock);
        int n = std::min (count, kMaxSyllables);
        for (int i = 0; i < n; ++i)
            syllables[i] = targets[i];
        numSyllables.store (n, std::memory_order_release);
        currentIndex.store (0, std::memory_order_release);
    }

    void clear()
    {
        juce::SpinLock::ScopedLockType lock (syllableLock);
        numSyllables.store (0, std::memory_order_release);
        currentIndex.store (0, std::memory_order_release);
    }

    // Called from audio thread on note-on — returns current syllable and advances
    SyllableTarget advanceAndGet()
    {
        juce::SpinLock::ScopedTryLockType lock (syllableLock);
        if (! lock.isLocked())
            return SyllableTarget {};  // Contended — return default (extremely rare)

        int count = numSyllables.load (std::memory_order_acquire);
        if (count == 0)
            return SyllableTarget {};

        int idx = currentIndex.load (std::memory_order_acquire);
        if (idx >= count)
            idx = count - 1;

        SyllableTarget result = syllables[idx];

        // Advance (loop or hold on last)
        int next = idx + 1;
        if (next >= count)
            next = looping.load (std::memory_order_relaxed) ? 0 : count - 1;
        currentIndex.store (next, std::memory_order_release);

        return result;
    }

    // Peek the current syllable without advancing.
    // IN-13: MESSAGE-THREAD ONLY. This reads syllables[idx] without taking
    // syllableLock (unlike advanceAndGet/setSyllables). That is safe *only*
    // because the sole writer (setSyllables, via nativeFunction) and the sole
    // caller here (PluginEditor lyrics poll) both run on the message thread, so
    // they never race on the array. Do NOT call this from the audio thread — it
    // would become a torn read of the SyllableTarget struct.
    SyllableTarget peekCurrent() const
    {
        int count = numSyllables.load (std::memory_order_acquire);
        if (count == 0)
            return SyllableTarget {};

        int idx = currentIndex.load (std::memory_order_acquire);
        if (idx >= count)
            idx = count - 1;

        return syllables[idx];
    }

    int getCurrentIndex() const { return currentIndex.load (std::memory_order_acquire); }
    int getNumSyllables() const { return numSyllables.load (std::memory_order_acquire); }

    void reset() { currentIndex.store (0, std::memory_order_release); }
    void setLooping (bool loop) { looping.store (loop, std::memory_order_release); }
    bool isLooping() const { return looping.load (std::memory_order_acquire); }

    // Lyrics text persistence (message thread only)
    void setLyricsText (const juce::String& text) { lyricsText = text; }
    juce::String getLyricsText() const { return lyricsText; }

    // CR-06: the parsed syllable schedule as the page sends it (a JSON array of
    // SyllableTarget objects). Persisted in the session so lyrics play with the
    // editor never opened. An unchanged schedule is a no-op, so opening the
    // editor (which re-sends) does not restart the lyric mid-song.
    // MESSAGE-THREAD ONLY. Returns false on malformed JSON (engine untouched).
    bool setSyllablesFromJson (const juce::String& json)
    {
        if (json == syllablesJson && json.isNotEmpty())
            return true;

        auto parsed = juce::JSON::parse (json);
        auto* arr = parsed.getArray();
        if (arr == nullptr)
            return false;

        std::array<SyllableTarget, kMaxSyllables> targets;
        int n = 0;
        for (const auto& item : *arr)
        {
            if (n >= kMaxSyllables)
                break;
            if (auto* obj = item.getDynamicObject())
            {
                auto& t = targets[static_cast<size_t> (n++)];
                t.vowelX           = static_cast<float> (obj->getProperty ("vowelX"));
                t.vowelY           = static_cast<float> (obj->getProperty ("vowelY"));
                t.consonantTone    = static_cast<float> (obj->getProperty ("consonantTone"));
                t.sibilance        = static_cast<float> (obj->getProperty ("sibilance"));
                t.consonantVoicing = static_cast<float> (obj->getProperty ("consonantVoicing"));
                t.consonantLevel   = static_cast<float> (obj->getProperty ("consonantLevel"));
                t.nasalCoupling    = static_cast<float> (obj->getProperty ("nasalCoupling"));
                t.nasalPlace       = static_cast<float> (obj->getProperty ("nasalPlace"));
                t.hasConsonant     = static_cast<bool>  (obj->getProperty ("hasConsonant"));
            }
        }

        syllablesJson = json;
        // WR-10: an empty list is a real edit (all lyrics deleted) — clear.
        if (n == 0)
            clear();
        else
            setSyllables (targets.data(), n);
        return true;
    }

    juce::String getSyllablesJson() const { return syllablesJson; }

private:
    std::array<SyllableTarget, kMaxSyllables> syllables;
    std::atomic<int> numSyllables { 0 };
    std::atomic<int> currentIndex { 0 };
    std::atomic<bool> looping { true };
    juce::SpinLock syllableLock;

    // Raw ARPABET text for state persistence (message thread only)
    juce::String lyricsText;
    // CR-06: the schedule JSON last applied (message thread only)
    juce::String syllablesJson;
};
