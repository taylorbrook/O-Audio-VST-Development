/*
  ==============================================================================

    ReverbProcessor.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio

    8-channel FDN plate reverb with shimmer.

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

    void setSize (float size);
    void setDamping (float damp);
    void setPredelay (float ms);
    void setMix (float mix);
    void setMod (float mod);
    void setShimmer (float shimmer);

private:
    static constexpr int kNumChannels = 8;
    static constexpr int kBaseDelays[kNumChannels] = { 809, 877, 937, 1049, 1151, 1249, 1373, 1499 };
    static constexpr int kNumDiffusionStages = 4;
    static constexpr int kDiffusionDelays[kNumDiffusionStages] = { 142, 107, 79, 53 };
    static constexpr float kDiffusionCoeff = 0.625f;
    static constexpr float kLfoRates[4] = { 0.15f, 0.33f, 0.57f, 0.97f };
    static constexpr float kMaxModExcursion = 16.0f;

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

    struct OnePole
    {
        void setCoefficient (float coeff) { a = coeff; }
        float process (float x) { z = x + a * (z - x); return z; }
        void clear() { z = 0.0f; }

    private:
        float a = 0.0f;
        float z = 0.0f;
    };

    struct LFO
    {
        void setRate (float hz, float sampleRate) { delta = hz / sampleRate; }
        float next() { phase += delta; if (phase >= 1.0f) phase -= 1.0f; return std::sin (phase * juce::MathConstants<float>::twoPi); }
        void reset() { phase = 0.0f; }

    private:
        float phase = 0.0f;
        float delta = 0.0f;
    };

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
        float hpPrevIn = 0.0f;
        float hpPrevOut = 0.0f;
        float hpAlpha = 0.0f;
    };

    float currentSampleRate = 44100.0f;

    DelayLine preDelayL, preDelayR;

    std::array<std::array<DelayLine, kNumChannels>, kNumDiffusionStages> diffusionDelays;
    std::array<std::array<float, kNumChannels>, kNumDiffusionStages> diffusionState {};

    std::array<DelayLine, kNumChannels> tankDelays;
    std::array<OnePole, kNumChannels> tankFilters;

    std::array<LFO, 4> lfoBank;

    ShimmerShifter shimmerL, shimmerR;
    float shimmerAccumL = 0.0f, shimmerAccumR = 0.0f;

    juce::dsp::DryWetMixer<float> dryWetMixer;

    std::array<float, kNumChannels> scaledDelays {};
    float prevSizeForDelays = -1.0f;

    std::atomic<float> targetSize     { 0.5f };
    std::atomic<float> targetDamping  { 0.5f };
    std::atomic<float> targetPredelayMs { 0.0f };
    std::atomic<float> targetMix      { 0.0f };
    std::atomic<float> targetMod      { 0.2f };
    std::atomic<float> targetShimmer  { 0.0f };

    float prevMix = -999.0f;

    void applyHouseholder (float* data);
    void applyInputDiffusion (float* channels);
    float computeFeedbackGain (float size) const;
};
