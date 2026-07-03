/*
  ==============================================================================

    O-simplePhysicalModelSynth - PositionComb

    Feed-forward excitation-position comb: y[n] = x[n] − x[n−D] with a fractional,
    linearly-interpolated delay. Models where the string is excited — plucking /
    striking at fraction P of the length puts spectral nulls at k·f0/P.

    Ported from O-Lyrica (PluckExciter.cpp:129-144, :242-247). Shared by the Pluck
    and Strike exciters so the Position control behaves identically on the KS path
    regardless of exciter (swapping exciter changes only the attack/drive).

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <array>

class PositionComb
{
public:
    void reset() noexcept
    {
        buffer.fill (0.0f);
        writeIndex = 0;
    }

    // position 0–1 (Excitation Position / 100); D = clamp(pos,0.05,0.95)·(fs/f0).
    void setDelay (float position01, float f0, double sampleRate) noexcept
    {
        const float clampedPos = juce::jlimit (0.05f, 0.95f, position01);
        const float period     = static_cast<float> (sampleRate / juce::jmax (8.0f, f0));
        delaySamples = juce::jlimit (1.0f, static_cast<float> (MAX_DELAY - 2), clampedPos * period);
    }

    float processSample (float x) noexcept
    {
        buffer[static_cast<size_t> (writeIndex)] = x;
        float delayed = 0.0f;
        if (delaySamples > 0.0f)
        {
            const int   di   = static_cast<int> (delaySamples);
            const float frac = delaySamples - static_cast<float> (di);
            const int   r0   = (writeIndex - di + MAX_DELAY) % MAX_DELAY;
            const int   r1   = (r0 - 1 + MAX_DELAY) % MAX_DELAY;
            delayed = buffer[static_cast<size_t> (r0)] * (1.0f - frac)
                    + buffer[static_cast<size_t> (r1)] * frac;
        }
        writeIndex = (writeIndex + 1) % MAX_DELAY;
        return x - delayed;
    }

private:
    static constexpr int MAX_DELAY = 8192;
    std::array<float, MAX_DELAY> buffer {};
    int   writeIndex   = 0;
    float delaySamples = 0.0f;
};
