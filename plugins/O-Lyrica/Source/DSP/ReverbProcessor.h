/*
  ==============================================================================

    ReverbProcessor.h
    O-Lyrica - Physical Modeling Harp Synthesizer
    Ouaricon Audio

    v2.1.0: 8-channel FDN plate reverb replacing Freeverb.
    Based on Signalsmith FDN architecture with:
    - Householder feedback matrix
    - 4-stage input diffusion
    - 2-band frequency-dependent decay (shelving filters)
    - Multi-LFO delay modulation
    - Optional shimmer (octave-up feedback)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

class ReverbProcessor
{
public:
    ReverbProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    // Thread-safe setters (store to atomics; applied in process())
    void setSize (float size);
    void setDamping (float damp);
    void setPredelay (float ms);
    void setMix (float mix);
    void setMod (float mod);
    void setShimmer (float shimmer);

private:
    // ── FDN Constants ──────────────────────────────────────────────────
    static constexpr int kNumChannels = 8;

    // Base delay lengths (coprime, from DAFx 2023 optimized distribution, at 48 kHz)
    static constexpr int kBaseDelays[kNumChannels] = { 809, 877, 937, 1049, 1151, 1249, 1373, 1499 };

    // Input diffusion: 4 stages with decreasing delay lengths
    static constexpr int kNumDiffusionStages = 4;
    static constexpr int kDiffusionDelays[kNumDiffusionStages] = { 142, 107, 79, 53 };
    static constexpr float kDiffusionCoeff = 0.625f;  // allpass coefficient

    // LFO rates for modulated channels (Hz) — channels 0,2,4,6
    static constexpr float kLfoRates[4] = { 0.15f, 0.33f, 0.57f, 0.97f };
    static constexpr float kMaxModExcursion = 16.0f;  // samples

    // ── Delay Line ─────────────────────────────────────────────────────
    // Simple delay line with fractional read via linear interpolation
    struct DelayLine
    {
        void resize (int maxSamples);
        void clear();
        void push (float sample);
        float read (float delaySamples) const;
        float readNearest (int delaySamples) const;

    private:
        std::vector<float> buffer;
        int writePos = 0;
        int mask = 0;
    };

    // ── One-pole filter for HF damping in feedback loop ────────────────
    struct OnePole
    {
        void setCoefficient (float coeff) { a = coeff; }
        float process (float x) { z = x + a * (z - x); return z; }
        void clear() { z = 0.0f; }

    private:
        float a = 0.0f;  // 0 = no filtering, 1 = full LP
        float z = 0.0f;
    };

    // ── LFO (simple sine) ──────────────────────────────────────────────
    struct LFO
    {
        void setRate (float hz, float sampleRate) { delta = hz / sampleRate; }
        float next() { phase += delta; if (phase >= 1.0f) phase -= 1.0f; return std::sin (phase * juce::MathConstants<float>::twoPi); }
        void reset() { phase = 0.0f; }

    private:
        float phase = 0.0f;
        float delta = 0.0f;
    };

    // ── Shimmer pitch shifter (4-head octave up with LP feedback filter) ──
    struct ShimmerShifter
    {
        void prepare (float sr, int maxBlockSize);
        float process (float input);
        void clear();

    private:
        static constexpr int kGrainSize = 4096;
        static constexpr int kNumHeads = 4;
        DelayLine grainBuffer;
        float readPos = 0.0f;
        float sampleRate = 44100.0f;
        // LP filter on output to tame HF buildup through feedback
        float lpState = 0.0f;
        float lpCoeff = 0.0f; // set in prepare()
    };

    // ── Internal state ─────────────────────────────────────────────────
    float currentSampleRate = 44100.0f;

    // Pre-delay
    DelayLine preDelayL, preDelayR;

    // Input diffusion: 4 stages, each with kNumChannels allpass delays
    std::array<std::array<DelayLine, kNumChannels>, kNumDiffusionStages> diffusionDelays;
    std::array<std::array<float, kNumChannels>, kNumDiffusionStages> diffusionState {};

    // FDN tank
    std::array<DelayLine, kNumChannels> tankDelays;
    std::array<OnePole, kNumChannels> tankFilters;
    std::array<float, kNumChannels> tankState {};

    // Modulation LFOs (4 LFOs for channels 0,2,4,6)
    std::array<LFO, 4> lfoBank;

    // Shimmer
    ShimmerShifter shimmerL, shimmerR;
    float shimmerAccumL = 0.0f, shimmerAccumR = 0.0f;

    // Dry/wet mixer
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Scaled delay lengths (updated when size changes)
    std::array<float, kNumChannels> scaledDelays {};
    float prevSizeForDelays = -1.0f;

    // ── Atomic targets ─────────────────────────────────────────────────
    std::atomic<float> targetSize     { 0.5f };
    std::atomic<float> targetDamping  { 0.5f };
    std::atomic<float> targetPredelayMs { 0.0f };
    std::atomic<float> targetMix      { 0.0f };
    std::atomic<float> targetMod      { 0.2f };
    std::atomic<float> targetShimmer  { 0.0f };

    // Dirty-flag: previous values for parameter caching
    float prevSize = -999.0f;
    float prevDamping = -999.0f;
    float prevMix = -999.0f;

    // ── Helpers ────────────────────────────────────────────────────────
    void applyHouseholder (float* data);
    void applyInputDiffusion (float* channels);
    float computeFeedbackGain (float size) const;
};
