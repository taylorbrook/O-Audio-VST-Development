/*
  ==============================================================================

    Exciter.h
    Modal Synthesis Bassoon - dual-shape attack-character morph exciter
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1: single static onset shape (5 ms half-sine × exp).
    Phase 2.4 (DSP-05): adds tonguedShape (7.5 ms exp-decay × white noise) +
    startOnset(attackChar, velocity) snapshot for onset-window-lifetime morph.
    softShape is the Phase 2.1 shape under its new name.
    Phase 2.4 rev-5 (verify-phase in-cycle, DSP-05 audibility fix):
    softShape extended 5→30 ms with 1-pole LP @ 600 Hz for woody pad onset
    audibly distinct from tonguedShape's sharp 7.5 ms noise burst.

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
    static constexpr int   MAX_ONSET_SAMPLES   = 4096;   // rev-5: 30 ms @ 96 kHz = 2880; 4096 leaves headroom
    static constexpr float SOFT_DURATION_MS    = 30.0f;  // rev-5: extended 5→30 ms (DSP-05 audibility)
    static constexpr float SOFT_TAU_MS         = 12.0f;  // rev-5: longer decay tail (was 1.5 ms)
    static constexpr float SOFT_LP_FREQ_HZ     = 600.0f; // rev-5: 1-pole LP for woody pad character
    static constexpr float TONGUED_DURATION_MS = 7.5f;   // Phase 2.4: exp-decay × white noise

    void prepare (double sampleRate);

    /** Phase 2.1 wrapper — equivalent to startOnset(0.0f, 1.0f). Retained for
        backwards compatibility (D6-rev-4); Phase 2.4 callers use startOnset(...) directly. */
    void start() noexcept { startOnset (0.0f, 1.0f); }

    /** Phase 2.4: snapshot effective attack-character (with velocity bias) for
        the lifetime of the onset window. Mid-onset automation does NOT affect
        the in-flight onset (zipper avoidance — risk #2 mitigation). */
    void startOnset (float attackChar01, float velocity01) noexcept
    {
        const float biased  = attackChar01 + (velocity01 - 0.5f) * VELOCITY_BIAS_MAGNITUDE;
        effectiveAttackChar = juce::jlimit (0.0f, 1.0f, biased);
        onsetIdx            = 0;
        active              = true;
    }

    inline float getNextSample() noexcept
    {
        if (! active || onsetIdx >= onsetSamples)
        {
            active = false;
            return 0.0f;
        }
        const auto i = static_cast<size_t> (onsetIdx++);
        // Linear morph between two pre-baked shapes. tonguedShape is zero-padded
        // for indices beyond TONGUED_DURATION_MS by std::array zero-init (D2-rev-4).
        return juce::jmap (effectiveAttackChar,
                           softShape[i], tonguedShape[i]);
    }

    void reset() noexcept { onsetIdx = 0; active = false; }

private:
    static constexpr float VELOCITY_BIAS_MAGNITUDE = 0.3f;   // OQ#4-rev-4 locked

    std::array<float, MAX_ONSET_SAMPLES> softShape    {};   // Phase 2.1 5 ms half-sine × exp (renamed from onsetBuffer)
    std::array<float, MAX_ONSET_SAMPLES> tonguedShape {};   // Phase 2.4 NEW: 7.5 ms exp-decay × white noise

    int   onsetSamples        = 0;   // length of the longer of the two windows
    int   onsetIdx            = 0;
    float effectiveAttackChar = 0.0f;
    bool  active              = false;
};
