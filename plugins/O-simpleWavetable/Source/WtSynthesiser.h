/*
  ==============================================================================

    WtSynthesiser.h
    O-simpleWavetable - juce::Synthesiser with an allocation-free voice steal.

    Why: JUCE 8.0.15 Synthesiser::findVoiceToSteal starts with
    usableVoicesToStealArray.clear(), and Array::clear() FREES the storage
    (setAllocatedSize (0)). The add() that follows then mallocs on the audio
    thread on every steal; the addVoice() preallocation only survives until
    the first steal. Found by the dsp-check --alloc-check gate (G-ALLOC).

    This override is a line-for-line port of the JUCE policy (same order of
    preference, same low/top protection, same oldest-first sort) over a fixed
    stack array, so steal behaviour is unchanged (G-POLY) and nothing
    allocates.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>

class WtSynthesiser : public juce::Synthesiser
{
public:
    static constexpr int kMaxStealVoices = 64;

protected:
    juce::SynthesiserVoice* findVoiceToSteal (juce::SynthesiserSound* soundToPlay,
                                              int /*midiChannel*/,
                                              int midiNoteNumber) const override
    {
        std::array<juce::SynthesiserVoice*, kMaxStealVoices> usable {};
        int numUsable = 0;

        juce::SynthesiserVoice* low = nullptr;   // lowest sounding note (not released)
        juce::SynthesiserVoice* top = nullptr;   // highest sounding note (not released)

        jassert (voices.size() <= kMaxStealVoices);

        // `voices` directly (like JUCE): getVoice (i) would take the
        // Synthesiser's recursive lock once per voice.
        for (auto* voice : voices)
        {
            if (numUsable >= kMaxStealVoices)
                break;

            if (voice == nullptr || ! voice->canPlaySound (soundToPlay))
                continue;

            usable[(size_t) numUsable++] = voice;

            if (! voice->isPlayingButReleased())   // don't protect released notes
            {
                const int note = voice->getCurrentlyPlayingNote();

                if (low == nullptr || note < low->getCurrentlyPlayingNote())
                    low = voice;

                if (top == nullptr || note > top->getCurrentlyPlayingNote())
                    top = voice;
            }
        }

        if (numUsable == 0)
            return nullptr;

        // Oldest first (JUCE sorts by wasStartedBefore). Functor, no captures.
        struct Sorter
        {
            bool operator() (const juce::SynthesiserVoice* a, const juce::SynthesiserVoice* b) const noexcept
            {
                return a->wasStartedBefore (*b);
            }
        };
        std::sort (usable.begin(), usable.begin() + numUsable, Sorter());

        // Only one note playing: precedence to the lowest.
        if (top == low)
            top = nullptr;

        const auto begin = usable.begin();
        const auto end   = usable.begin() + numUsable;

        // The oldest voice already playing the target pitch.
        for (auto it = begin; it != end; ++it)
            if ((*it)->getCurrentlyPlayingNote() == midiNoteNumber)
                return *it;

        // Oldest released voice (no finger, not held by sustain).
        for (auto it = begin; it != end; ++it)
            if (*it != low && *it != top && (*it)->isPlayingButReleased())
                return *it;

        // Oldest voice without a finger on it.
        for (auto it = begin; it != end; ++it)
            if (*it != low && *it != top && ! (*it)->isKeyDown())
                return *it;

        // Oldest unprotected voice.
        for (auto it = begin; it != end; ++it)
            if (*it != low && *it != top)
                return *it;

        // Only protected voices left: the bass note has priority.
        jassert (low != nullptr);

        if (top != nullptr)
            return top;

        return low;
    }
};
