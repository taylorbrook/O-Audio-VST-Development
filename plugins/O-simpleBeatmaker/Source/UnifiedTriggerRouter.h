/*
  ==============================================================================

    O-simpleBeatmaker - UnifiedTriggerRouter (the spine)

    GM note# -> voice-index reverse map, mute/solo gating, and the sub-slice
    render driver. The driver is a direct transcription of
    juce::Synthesiser::processNextBlock (juce_Synthesiser.cpp:180-236): render
    all voices over each [startSample, nextEvent) span, then apply the event's
    trigger — so sub-step Δt (the MidiMessage samplePosition) is honoured
    sample-accurately. We skip JUCE's minimumSubBlockSize coalescing: 1-sample
    sub-blocks are fine (light CPU) and sample-exact offsets are the whole point.

    A host note-on for GM note 36 and a sequencer-emitted note-on for GM note 36
    are indistinguishable here — one merged, sorted MIDI stream feeds the voices.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include "BeatmakerIDs.h"
#include "DrumVoiceEngine.h"

namespace OSimpleBeatmaker
{
    class UnifiedTriggerRouter
    {
    public:
        UnifiedTriggerRouter()
        {
            noteToVoice.fill (-1);
            for (int v = 0; v < kNumVoices; ++v)
            {
                const int note = kGmNotes[(size_t) v];
                if (note >= 0 && note < 128)
                    noteToVoice[(size_t) note] = v;
            }
        }

        int voiceForNote (int noteNumber) const noexcept
        {
            return (noteNumber >= 0 && noteNumber < 128) ? noteToVoice[(size_t) noteNumber] : -1;
        }

        // Per-block mute/solo push (cached APVTS).
        void setMuteSolo (const std::array<bool, (size_t) kNumVoices>& mute,
                          const std::array<bool, (size_t) kNumVoices>& solo) noexcept
        {
            muted = mute;
            soloed = solo;
            anySolo = false;
            for (bool s : soloed) anySolo |= s;
        }

        bool isVoiceAudible (int v) const noexcept
        {
            if (v < 0 || v >= kNumVoices) return false;
            if (anySolo) return soloed[(size_t) v];
            return ! muted[(size_t) v];
        }

        // Map one MIDI note-on to a voice trigger (mute/solo applied here for the
        // audio path). Closed-hat choke is handled inside DrumVoiceEngine::trigger.
        void handleTrigger (const juce::MidiMessage& m, DrumVoiceEngine& engine) const noexcept
        {
            if (! m.isNoteOn()) return;                       // note-offs ignored (one-shot)
            const int v = voiceForNote (m.getNoteNumber());
            if (v < 0) return;                                // out-of-map ignored, no error
            if (! isVoiceAudible (v)) return;                 // muted / non-soloed -> skip trigger
            engine.trigger (v, (juce::uint8) m.getVelocity());
        }

        // Sub-slice render: mirror Synthesiser::processNextBlock over `merged`.
        void renderMerged (juce::AudioBuffer<float>& buffer,
                           const juce::MidiBuffer& merged,
                           DrumVoiceEngine& engine) const noexcept
        {
            int startSample = 0;
            int numSamples  = buffer.getNumSamples();
            auto it = merged.findNextSamplePosition (0);

            for (; numSamples > 0; ++it)
            {
                if (it == merged.cend())
                {
                    engine.renderAll (buffer, startSample, numSamples);
                    break;
                }

                const auto meta   = *it;
                const int  toNext = meta.samplePosition - startSample;

                if (toNext >= numSamples)
                {
                    engine.renderAll (buffer, startSample, numSamples);
                    handleTrigger (meta.getMessage(), engine);
                    break;
                }
                if (toNext > 0)
                {
                    engine.renderAll (buffer, startSample, toNext);
                    startSample += toNext;
                    numSamples  -= toNext;
                }
                handleTrigger (meta.getMessage(), engine);    // zero-length span ok
            }
        }

    private:
        std::array<int, 128> noteToVoice {};
        std::array<bool, (size_t) kNumVoices> muted {};
        std::array<bool, (size_t) kNumVoices> soloed {};
        bool anySolo = false;
    };
}
