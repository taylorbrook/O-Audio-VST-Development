/*
  ==============================================================================

    Exciter.h
    Modal Synthesis Bassoon - 5 ms half-sine × exp impulse exciter
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1: single static onset shape pre-baked in prepare(). Allocation-free
    runtime path. Phase 2.4 will add a tonguedShape buffer + crossfade morph.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <algorithm>

class Exciter
{
public:
    static constexpr int   MAX_ONSET_SAMPLES = 1024;   // 5 ms @ 96 kHz = 480; 1024 leaves headroom
    static constexpr float DURATION_MS       = 5.0f;
    static constexpr float TAU_MS            = 1.5f;

    void prepare (double sampleRate);

    void start() noexcept { onsetIdx = 0; active = true; }

    inline float getNextSample() noexcept
    {
        if (! active || onsetIdx >= onsetSamples)
        {
            active = false;
            return 0.0f;
        }
        return onsetBuffer[static_cast<size_t> (onsetIdx++)];
    }

    void reset() noexcept { onsetIdx = 0; active = false; }

private:
    std::array<float, MAX_ONSET_SAMPLES> onsetBuffer {};
    int  onsetSamples = 0;
    int  onsetIdx     = 0;
    bool active       = false;
};
