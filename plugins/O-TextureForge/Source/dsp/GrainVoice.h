/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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

    GrainVoice.h
    Individual grain voice and voice pool management

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

struct GrainVoice
{
    bool active = false;
    uint32_t grainIndex = 0;          // Index into SharedCorpus::grains
    uint32_t grainStartSample = 0;    // Start position in corpus buffer
    uint32_t grainLengthSamples = 0;  // Length of this grain
    double readPosition = 0.0;        // Current read position (fractional). v1.6.1: double —
                                      // float lost the fraction on long corpora
    float playbackRate = 1.0f;        // Pitch shift ratio
    float gain = 1.0f;                // Linear gain
    float crossfadeFactor = 0.5f;     // Envelope width factor (0=short, 1=wide overlap)
    uint32_t samplesElapsed = 0;      // Samples processed so far
    uint32_t ageCounter = 0;          // For voice stealing (oldest-active)
    int midiNote = -1;                // Triggering MIDI note (-1 if none)
    int midiChannel = -1;             // Triggering MIDI channel
    float stealGain = 1.0f;           // v1.5.3: fade-out gain (1 = not fading)
    float stealStep = 0.0f;           // v1.5.3: per-sample decrement, > 0 while fading
    // v1.7.0: rate > 1 grains read the corpus mipmap. Level and sinc set depend
    // only on the rate, which is fixed for the grain, so both are latched on spawn.
    bool bandlimited = false;
    int mipLevel = 0;
    int sincSet = 0;

    void reset()
    {
        active = false;
        grainIndex = 0;
        grainStartSample = 0;
        grainLengthSamples = 0;
        readPosition = 0.0;
        playbackRate = 1.0f;
        gain = 1.0f;
        crossfadeFactor = 0.5f;
        samplesElapsed = 0;
        midiNote = -1;
        midiChannel = -1;
        stealGain = 1.0f;
        stealStep = 0.0f;
        bandlimited = false;
        mipLevel = 0;
        sincSet = 0;
    }

    // Hann window envelope with crossfade modulation
    // crossfadeFactor: 0 = narrow peak, 1 = wide/flat-top (more overlap)
    float getEnvelope() const
    {
        if (grainLengthSamples == 0)
            return 0.0f;

        const float phase = static_cast<float>(samplesElapsed) / static_cast<float>(grainLengthSamples);
        if (phase < 0.0f || phase > 1.0f)
            return 0.0f;

        return envelopeAt(phase, crossfadeFactor);
    }

    // Window at phase [0,1] for a crossfade factor. Shared by getEnvelope() and
    // the drone-mode density normalisation's power moment (v1.5.4).
    static float envelopeAt(float phase, float crossfadeFactor)
    {
        // Standard Hann: 0.5*(1-cos(2*pi*phase))
        // With crossfade: blend between Hann and flat-top
        // Higher crossfade = more of the grain at full amplitude
        float hann = 0.5f * (1.0f - std::cos(2.0f * 3.14159265359f * phase));

        // Tukey-like: flat top portion scales with crossfade
        float flatTop = 1.0f;
        float rampLen = std::max(0.01f, 1.0f - crossfadeFactor);
        if (phase < rampLen * 0.5f)
            flatTop = 0.5f * (1.0f - std::cos(3.14159265359f * phase / (rampLen * 0.5f)));
        else if (phase > 1.0f - rampLen * 0.5f)
            flatTop = 0.5f * (1.0f - std::cos(3.14159265359f * (1.0f - phase) / (rampLen * 0.5f)));

        // Blend: low crossfade = pure Hann, high crossfade = Tukey flat-top
        return hann * (1.0f - crossfadeFactor) + flatTop * crossfadeFactor;
    }

    bool isFinished() const
    {
        return samplesElapsed >= grainLengthSamples;
    }
};

class GrainPool
{
public:
    static constexpr int MAX_VOICES = 64;
    // v1.5.3: a stolen grain is moved into one of these spare slots and fades out over
    // kFadeMs there, while the new grain takes the slot it always took; note-off fades
    // grains in place over the same time. Neither is a hard cut any more, and renders
    // with no steals and no note-offs are bit-identical to v1.5.2.
    static constexpr int kFadeSlots = 8;
    static constexpr int kNumSlots = MAX_VOICES + kFadeSlots;
    static constexpr double kFadeMs = 5.0;

    void setSampleRate(double sampleRate)
    {
        fadeStep = static_cast<float>(1.0 / std::max(1.0, sampleRate * kFadeMs / 1000.0));
    }

    GrainPool()
    {
        for (auto& voice : voices)
            voice.reset();
        nextAllocationIndex = 0;
        globalAgeCounter = 0;
    }

    // Allocate a voice (round-robin + oldest-steal)
    GrainVoice* allocate()
    {
        // Try round-robin allocation
        for (int i = 0; i < MAX_VOICES; ++i)
        {
            int index = (nextAllocationIndex + i) % MAX_VOICES;
            if (!voices[index].active)
            {
                nextAllocationIndex = (index + 1) % MAX_VOICES;
                voices[index].ageCounter = globalAgeCounter++;
                return &voices[index];
            }
        }

        // All voices active, steal oldest
        int oldestIndex = 0;
        uint32_t oldestAge = voices[0].ageCounter;
        for (int i = 1; i < MAX_VOICES; ++i)
        {
            if (voices[i].ageCounter < oldestAge)
            {
                oldestAge = voices[i].ageCounter;
                oldestIndex = i;
            }
        }

        moveToFadeSlot(voices[oldestIndex]);
        voices[oldestIndex].reset();
        voices[oldestIndex].ageCounter = globalAgeCounter++;
        nextAllocationIndex = (oldestIndex + 1) % MAX_VOICES;
        return &voices[oldestIndex];
    }

    // Release voice by MIDI note (for note-off handling)
    void releaseByNote(int midiNote, int midiChannel)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.midiNote == midiNote && voice.midiChannel == midiChannel)
                startFade(voice);
        }
    }

    // Release all MIDI-triggered voices (for Mode 1 monophonic behavior)
    void releaseAllMidiVoices()
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.midiNote >= 0)
                startFade(voice);
        }
    }

    // Release all voices
    void releaseAll()
    {
        for (auto& voice : voices)
            voice.reset();
    }

    // Get all voices
    std::array<GrainVoice, kNumSlots>& getVoices() { return voices; }
    const std::array<GrainVoice, kNumSlots>& getVoices() const { return voices; }

private:
    void startFade(GrainVoice& voice)
    {
        if (voice.stealStep <= 0.0f)
            voice.stealStep = fadeStep;
    }

    // Copy a stolen grain into a spare slot and start its fade. With every spare already
    // fading, the spare nearest silence is reused.
    void moveToFadeSlot(const GrainVoice& victim)
    {
        int slot = -1;
        float minGain = 2.0f;
        for (int i = MAX_VOICES; i < kNumSlots; ++i)
        {
            if (!voices[i].active) { slot = i; break; }
            if (voices[i].stealGain < minGain) { minGain = voices[i].stealGain; slot = i; }
        }

        voices[slot] = victim;
        startFade(voices[slot]);
    }

    std::array<GrainVoice, kNumSlots> voices;
    float fadeStep = 1.0f / 220.5f;
    int nextAllocationIndex = 0;
    uint32_t globalAgeCounter = 0;
};
