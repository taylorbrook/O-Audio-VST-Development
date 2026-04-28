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
      - SampleSlot::loopMode is set to Auto on LoopDetector success, OneShot
        on fallback, and Manual when the user overrides loop points via the
        Stage 3 loop editor.
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
#include <memory>
#include <vector>

enum class LoopMode
{
    OneShot = 0,    // No loop region detected (LoopDetector returned invalid)
    Auto    = 1,    // Loop region from LoopDetector::detectLoop
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

    // Phase 2.1: linear scan over slots. Returns the first slot matching
    // (midiNote, velocityLayer) or nullptr if none found. The `velocityLayer`
    // parameter is the LAYER INDEX (0..numVelocityLayers-1), NOT a 0-127
    // velocity value — the caller must derive the layer from velocity first.
    // ~10 ns over 352 entries (88 notes × 4 layers); trivial.
    const SampleSlot* findSlot (int midiNote, int velocityLayer) const noexcept
    {
        for (const auto& s : slots)
            if (s.midiNote == midiNote && s.velocityLayer == velocityLayer)
                return &s;
        return nullptr;
    }
};
