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

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>

struct SampleSlot
{
    juce::AudioBuffer<float> audio;
    double                   sourceSampleRate = 0.0;
    int                      midiNote = -1;
    int                      velocityLayer = 0;     // 0..3
    int                      loopStart = 0;
    int                      loopEnd   = 0;         // 0 = no loop (one-shot)
};

struct SampleMap
{
    std::vector<SampleSlot> slots;
    int                     lowestNote        = 127;
    int                     highestNote       = 0;
    int                     numVelocityLayers = 1;  // 1..4

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
