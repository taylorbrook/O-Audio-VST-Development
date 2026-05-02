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
