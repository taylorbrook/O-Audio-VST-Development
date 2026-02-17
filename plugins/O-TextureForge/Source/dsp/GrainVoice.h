/*
  ==============================================================================

    GrainVoice.h
    Individual grain voice and voice pool management

  ==============================================================================
*/

#pragma once

#include <array>
#include <cstdint>

struct GrainVoice
{
    bool active = false;
    uint32_t grainIndex = 0;          // Index into SharedCorpus::grains
    uint32_t grainStartSample = 0;    // Start position in corpus buffer
    uint32_t grainLengthSamples = 0;  // Length of this grain
    float readPosition = 0.0f;        // Current read position (fractional)
    float playbackRate = 1.0f;        // Pitch shift ratio
    float gain = 1.0f;                // Linear gain
    float crossfadeFactor = 0.5f;     // Envelope width factor (0=short, 1=wide overlap)
    uint32_t samplesElapsed = 0;      // Samples processed so far
    uint32_t ageCounter = 0;          // For voice stealing (oldest-active)
    int midiNote = -1;                // Triggering MIDI note (-1 if none)
    int midiChannel = -1;             // Triggering MIDI channel

    void reset()
    {
        active = false;
        grainIndex = 0;
        grainStartSample = 0;
        grainLengthSamples = 0;
        readPosition = 0.0f;
        playbackRate = 1.0f;
        gain = 1.0f;
        crossfadeFactor = 0.5f;
        samplesElapsed = 0;
        midiNote = -1;
        midiChannel = -1;
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
            {
                voice.active = false;
            }
        }
    }

    // Release all MIDI-triggered voices (for Mode 1 monophonic behavior)
    void releaseAllMidiVoices()
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.midiNote >= 0)
                voice.active = false;
        }
    }

    // Release all voices
    void releaseAll()
    {
        for (auto& voice : voices)
            voice.reset();
    }

    // Get all voices
    std::array<GrainVoice, MAX_VOICES>& getVoices() { return voices; }
    const std::array<GrainVoice, MAX_VOICES>& getVoices() const { return voices; }

private:
    std::array<GrainVoice, MAX_VOICES> voices;
    int nextAllocationIndex = 0;
    uint32_t globalAgeCounter = 0;
};
