/*
  ==============================================================================

    SampleMap.h
    Microtonal Sample Engine - Sample-map POD storage
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1: findSlot implemented as a linear scan over slots.

    Lifetime model: held by PluginProcessor as std::shared_ptr<SampleMap>.
    Background loader produces a new shared_ptr; message thread atomic-stores
    it into the processor's slot. Voices snapshot the shared_ptr in startNote
    (lock-free refcount inc) and hold for the note's duration.

    Phase 3.1 invariant addition (RQ3-3 / RQ3-2):
      - SampleSlot::audio is now std::shared_ptr<juce::AudioBuffer<float>> so
        per-cell replace can deep-copy SampleMap's slot vector cheaply (we
        copy a vector of shared_ptrs + POD fields, not the audio data itself).
        Voices read via slot->audio->getReadPointer(ch). Active-note voices
        keep their snapshot map alive (transitive ref) so a slot replacement
        mid-note does not invalidate the held buffer (Stage 2 EC-3).
      - SampleSlot::filename is the basename (File::getFileName()) populated
        by the loader. Used for Stage 3 UI cell tooltip / loop editor header.
      - SampleSlot::loopMode defaults to Auto with whole-file loop points
        (loopStart=0, loopEnd=N-2) since v1.4.0. Falls back to OneShot only
        when the buffer is too short for the cubic-context headroom (< 18
        samples). Manual mode is set when the user overrides loop points via
        the Stage 3 loop editor.
      - SampleMap::version is monotonic; bumped by the processor on every
        atomic-store. Stage 3 JS uses it for diff detection / stale-data
        guards in the async cell-replace queue (EC3-5).

    All additions are STRICTLY ADDITIVE — Stage 2 audio path semantics are
    unchanged after Phase 3.1 (verified via render-harness identity test in
    PLAN Task 4).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cstdlib>
#include <limits>
#include <memory>
#include <vector>

enum class LoopMode
{
    OneShot = 0,    // Buffer too short for whole-file loop (< 18 samples)
    Auto    = 1,    // Default whole-file loop (loopStart=0, loopEnd=N-2)
    Manual  = 2     // User-overridden via Stage 3 loop editor
};

struct SampleSlot
{
    // Phase 3.1: audio held via shared_ptr so SampleMap deep-copies are cheap
    // (vector of pointers + POD fields). Voices read through one extra
    // indirection: slot->audio->getReadPointer(ch). Lifetime is owned by the
    // map snapshot held by the active voice (Stage 2 EC-3 invariant).
    std::shared_ptr<juce::AudioBuffer<float>> audio;

    juce::String filename;                   // Basename only (File::getFileName())
    double       sourceSampleRate = 0.0;
    int          midiNote         = -1;
    int          velocityLayer    = 0;       // 0..3
    int          loopStart        = 0;
    int          loopEnd          = 0;       // 0 = no loop (one-shot)
    LoopMode     loopMode         = LoopMode::OneShot;
};

struct SampleMap
{
    std::vector<SampleSlot> slots;
    int                     lowestNote        = 127;
    int                     highestNote       = 0;
    int                     numVelocityLayers = 1;  // 1..4
    int                     version           = 0;  // Phase 3.1: monotonic counter

    // Linear scan returning the slot in `velocityLayer` whose midiNote is
    // closest to the requested `midiNote`. Implements REQUIREMENTS §FUNC-04
    // ("or nearest if N is unsampled") — the voice's repitch path
    // (computePlayRateForSlot) handles the pitch shift. Returns nullptr only
    // when the requested layer contains no slots at all.
    //
    // Tie-breaking: when two slots are equidistant, prefer the lower midiNote
    // so the equidistant choice transposes UP — preserves more of the
    // sample's low-frequency content than a downshift would.
    //
    // The `velocityLayer` parameter is the LAYER INDEX (0..numVelocityLayers-1),
    // NOT a 0-127 velocity value — the caller derives the layer from velocity.
    const SampleSlot* findSlot (int midiNote, int velocityLayer) const noexcept
    {
        const SampleSlot* best     = nullptr;
        int               bestDist = std::numeric_limits<int>::max();

        for (const auto& s : slots)
        {
            if (s.velocityLayer != velocityLayer)
                continue;

            const int d = std::abs (s.midiNote - midiNote);
            if (d < bestDist
                || (d == bestDist && best != nullptr && s.midiNote < best->midiNote))
            {
                best     = &s;
                bestDist = d;
            }
        }

        return best;
    }
};
