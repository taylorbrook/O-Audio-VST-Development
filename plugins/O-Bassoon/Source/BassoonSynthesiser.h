/*
   This file is part of O-Bassoon, an Ouaricon Audio plugin.
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

    BassoonSynthesiser.h
    Modal Synthesis Bassoon - Voice manager with active-cap + JUCE default stealing
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.4: subclass juce::Synthesiser to enforce voice_count cap.
    findFreeVoice override gates by activeVoiceCap; delegates to base for
    free-pool selection and to base findVoiceToSteal (release-tail-first,
    then oldest-noteOn — JUCE 8 default).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class BassoonSynthesiser : public juce::Synthesiser
{
public:
    BassoonSynthesiser() noexcept
    {
        // Explicit-for-clarity; JUCE 8 default is true.
        setNoteStealingEnabled (true);
    }

    /** Sets the active voice cap. Snapshot at processBlock prologue per
        ROADMAP — applies on next note-on; already-active voices unaffected. */
    void setActiveVoiceCap (int cap) noexcept
    {
        activeVoiceCap = juce::jlimit (1, 16, cap);
    }

    int getActiveVoiceCap() const noexcept { return activeVoiceCap; }

protected:
    juce::SynthesiserVoice* findFreeVoice (juce::SynthesiserSound* sound,
                                           int                    channel,
                                           int                    noteNumber,
                                           bool                   stealIfNoneAvailable) const override
    {
        // Count active voices (allocation-free, RT-safe — no getNumActiveVoices in JUCE 8).
        int active = 0;
        const int n = getNumVoices();
        for (int i = 0; i < n; ++i)
            if (getVoice (i)->isVoiceActive())
                ++active;

        if (active < activeVoiceCap)
            return juce::Synthesiser::findFreeVoice (sound, channel, noteNumber, stealIfNoneAvailable);

        // At/over cap — steal if allowed, else null. JUCE-default findVoiceToSteal
        // prefers release-tail-first, then oldest-noteOn (juce_Synthesiser.cpp:525-594).
        return stealIfNoneAvailable
            ? findVoiceToSteal (sound, channel, noteNumber)
            : nullptr;
    }

private:
    int activeVoiceCap = 16;   // matches Stage 1 pre-allocated 16-voice pool
};
