/*
  ==============================================================================

    NoiseExciter.h
    Modal Synthesis Bassoon - Per-Voice Continuous Filtered-Noise Excitation
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.3 architectural pivot: replaces struck-modal-only sustain
    (Phase 2.1 impulse Exciter + Phase 2.2 rev-3 strike()) with a continuous
    breath-driven sustain source. Per-voice 1-pole low-pass (cutoff 2 kHz)
    over white noise; output scaled by BASE_NOISE_GAIN × breath_voice. Phase
    2.2 rev-3 strike() is retained at startNote for the attack transient.

    Per-voice juce::Random seeded voiceIndex × 31337 (deterministic, family-
    consistent — O-Bowed BowNoiseGenerator.h:23 precedent — locked OQ#3-rev-3,
    overrides CONTEXT-rev-3 default Time::currentTimeMillis() ^ voiceIndex per
    D3-rev-3) avoids shared mutable state across voices and keeps the seed
    reproducible across plugin instantiations.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class NoiseExciter
{
public:
    NoiseExciter() = default;
    ~NoiseExciter() = default;

    void  prepare        (double sr, int voiceIndex) noexcept;
    void  reset          () noexcept;
    float getNextSample  (float breathScaled) noexcept;

private:
    static constexpr float CUTOFF_HZ        = 2000.0f;
    // OQ#4-rev-3 ear-tuned at verify-phase rev-4 from 0.05f to top of
    // [0.03f, 0.20f] bracket — Gate 3 item 3 reported breath/sustain
    // inaudible at 0.05f (modal-bank steady-state output below floor).
    static constexpr float BASE_NOISE_GAIN  = 0.20f;

    double      sampleRate = 48000.0;
    float       lpCoeff    = 0.0f;
    float       lpState    = 0.0f;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseExciter)
};
