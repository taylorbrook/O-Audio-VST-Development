/*
  ==============================================================================

    ReverbProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    Dattorro plate reverb — figure-8 tank topology with modulated delays,
    input diffusion, and multi-tap stereo output.

    Reference: Jon Dattorro, "Effect Design Part 1: Reverberator and Other
    Filters", J. Audio Eng. Soc., Vol. 45, No. 9, 1997.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

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
    void setModDepth (float depth);
    void setModRate (float rateHz);

private:
    // ─── Allpass delay element ───
    struct Allpass
    {
        void setSize (int samples)
        {
            buffer.resize (static_cast<size_t> (samples), 0.0f);
            bufferSize = samples;
            writeIndex = 0;
        }

        void clear()
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            writeIndex = 0;
        }

        float process (float input, float coefficient)
        {
            float delayed = buffer[static_cast<size_t> (writeIndex)];
            float output = -input * coefficient + delayed;
            buffer[static_cast<size_t> (writeIndex)] = input + delayed * coefficient;
            writeIndex = (writeIndex + 1) % bufferSize;
            return output;
        }

        float read (int delaySamples) const
        {
            int idx = writeIndex - delaySamples;
            if (idx < 0) idx += bufferSize;
            return buffer[static_cast<size_t> (idx)];
        }

        std::vector<float> buffer;
        int bufferSize = 1;
        int writeIndex = 0;
    };

    // ─── Simple delay line (fixed or modulated read) ───
    struct DelayLine
    {
        void setSize (int maxSamples)
        {
            buffer.resize (static_cast<size_t> (maxSamples + 16), 0.0f);
            bufferSize = maxSamples + 16;
            writeIndex = 0;
        }

        void clear()
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            writeIndex = 0;
        }

        void write (float sample)
        {
            buffer[static_cast<size_t> (writeIndex)] = sample;
            writeIndex = (writeIndex + 1) % bufferSize;
        }

        float read (int delaySamples) const
        {
            int idx = writeIndex - delaySamples;
            if (idx < 0) idx += bufferSize;
            return buffer[static_cast<size_t> (idx)];
        }

        float readInterpolated (float delaySamples) const
        {
            float readPos = static_cast<float> (writeIndex) - delaySamples;
            if (readPos < 0.0f) readPos += static_cast<float> (bufferSize);

            int idx0 = static_cast<int> (readPos);
            int idx1 = (idx0 + 1) % bufferSize;
            float frac = readPos - static_cast<float> (idx0);

            return buffer[static_cast<size_t> (idx0)] * (1.0f - frac)
                 + buffer[static_cast<size_t> (idx1)] * frac;
        }

        float readAt (int index) const
        {
            int idx = index % bufferSize;
            if (idx < 0) idx += bufferSize;
            return buffer[static_cast<size_t> (idx)];
        }

        int getWriteIndex() const { return writeIndex; }

        std::vector<float> buffer;
        int bufferSize = 1;
        int writeIndex = 0;
    };

    // ─── One-pole lowpass (damping) ───
    struct OnePole
    {
        void setCoefficient (float g) { coeff = g; }
        void clear() { z1 = 0.0f; }

        float process (float input)
        {
            z1 = input * (1.0f - coeff) + z1 * coeff;
            return z1;
        }

        float coeff = 0.0f;
        float z1 = 0.0f;
    };

    // ─── Pre-delay ───
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL { 19200 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayR { 19200 };
    float preDelaySamples = 0.0f;

    // ─── Input diffusion (4 series allpasses) ───
    Allpass inputDiffusion[4];

    // ─── Tank (figure-8, 2 halves) ───

    // Half A
    Allpass  tankDiffusionA1;   // Decay diffuser 1
    DelayLine tankDelayA1;      // Delay line 1 (modulated)
    OnePole  tankDampA;         // Damping filter
    Allpass  tankDiffusionA2;   // Decay diffuser 2
    DelayLine tankDelayA2;      // Delay line 2

    // Half B
    Allpass  tankDiffusionB1;
    DelayLine tankDelayB1;      // (modulated)
    OnePole  tankDampB;
    Allpass  tankDiffusionB2;
    DelayLine tankDelayB2;

    // ─── Modulation LFOs (one per tank half, 90-degree offset) ───
    float lfoPhase = 0.0f;
    float lfoIncrement = 0.0f;

    // ─── Parameters ───
    float decayCoeff = 0.5f;
    float dampingCoeff = 0.5f;
    float modDepth = 0.0f;       // In samples (scaled from 0-1 user param)
    float modRateHz = 1.0f;

    // ─── Dry/wet ───
    juce::dsp::DryWetMixer<float> dryWetMixer;

    float currentSampleRate = 44100.0f;

    // ─── Cross-feedback state (persists across buffers) ───
    float tankFeedbackA = 0.0f;
    float tankFeedbackB = 0.0f;

    // ─── Decay diffusion 2 coefficient (decay-dependent) ───
    float decDiff2Coeff = 0.5f;

    // ─── Dattorro delay lengths (at 29761 Hz reference rate) ───
    // Scaled to actual sample rate in prepare()
    static constexpr float kRefRate = 29761.0f;

    int scaledDelay (int referenceSamples) const
    {
        return std::max (1, static_cast<int> (static_cast<float> (referenceSamples) * currentSampleRate / kRefRate));
    }

    // Reference delay lengths from the Dattorro paper
    static constexpr int kInputDiff1 = 142;
    static constexpr int kInputDiff2 = 107;
    static constexpr int kInputDiff3 = 379;
    static constexpr int kInputDiff4 = 277;

    static constexpr int kDecayDiff1A = 672;
    static constexpr int kDelayA1     = 4453;
    static constexpr int kDecayDiff2A = 1800;
    static constexpr int kDelayA2     = 3720;

    static constexpr int kDecayDiff1B = 908;
    static constexpr int kDelayB1     = 4217;
    static constexpr int kDecayDiff2B = 2656;
    static constexpr int kDelayB2     = 3163;

    // Output tap positions (Dattorro paper, delay-line-only taps, scaled in prepare)
    // Left:  +A1[266] +A1[2974] -A2[1913] +A2[1990] -B1[1990] -B2[187]
    // Right: +B1[353] +B1[3627] -B2[1228] +B2[2111] -A1[2111] -A2[335]
    int tapA1_266 = 0, tapA1_2974 = 0, tapA1_2111 = 0;
    int tapA2_1913 = 0, tapA2_1990 = 0, tapA2_335 = 0;
    int tapB1_353 = 0, tapB1_3627 = 0, tapB1_1990 = 0;
    int tapB2_187 = 0, tapB2_1228 = 0, tapB2_2111 = 0;

    // Precomputed fixed delay lengths (set in prepare, used in process)
    int delayA1len = 0, delayB1len = 0;
    int delayA2len = 0, delayB2len = 0;
};
