/*
  ==============================================================================

    MarimbaVoice.h
    Phase 2.2: Modal synthesis with 8-mode resonator bank

    Modal synthesis engine for authentic marimba timbre with inharmonic overtones

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MarimbaSound.h"
#include <array>

class MarimbaVoice : public juce::SynthesiserVoice
{
public:
    MarimbaVoice();

    // SynthesiserVoice interface
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

    // Set parameters from processor (called before rendering)
    void setOutputGain(float gainDB);
    void setVelocityCurve(float curve);
    void setMalletHardness(float hardness);
    void setBarMaterial(float material);
    void setResonance(float resonance);
    void setSampleRate(double newSampleRate);

private:
    // Phase 2.2: Modal synthesis constants
    static constexpr int NUM_MODES = 8;

    // Modal frequency ratios (inharmonic overtones for marimba timbre)
    static constexpr std::array<float, NUM_MODES> MODE_RATIOS = {
        1.00f, 3.93f, 9.24f, 16.65f, 26.3f, 38.2f, 52.4f, 68.9f
    };

    // Modal mode - biquad resonator for each partial
    struct ModalMode
    {
        // Biquad coefficients (calculated once per note in startNote)
        float b0 = 0.0f;  // Feedforward coefficient
        float a1 = 0.0f;  // Feedback coefficient 1
        float a2 = 0.0f;  // Feedback coefficient 2

        // Biquad state (updated each sample)
        float y1 = 0.0f;  // Output delayed by 1 sample
        float y2 = 0.0f;  // Output delayed by 2 samples

        float amplitude = 0.0f;  // Mode amplitude (based on BAR_MATERIAL)

        // Reset state to zero
        void reset()
        {
            y1 = 0.0f;
            y2 = 0.0f;
        }

        // Process one sample through biquad resonator
        inline float processSample(float input)
        {
            // Biquad difference equation: y[n] = b0*x[n] + a1*y[n-1] + a2*y[n-2]
            float output = b0 * input + a1 * y1 + a2 * y2;

            // Denormal protection (critical for real-time safety)
            if (std::abs(y1) < 1e-8f && std::abs(y2) < 1e-8f)
            {
                y1 = 0.0f;
                y2 = 0.0f;
            }

            // Update state
            y2 = y1;
            y1 = output;

            return output;
        }
    };

    // Modal resonator bank (8 modes)
    std::array<ModalMode, NUM_MODES> modes;

    // Mallet exciter state
    struct MalletExciter
    {
        juce::Random random;
        int samplesRemaining = 0;
        float filterState = 0.0f;       // One-pole lowpass state
        float filterCoefficient = 0.5f; // Lowpass cutoff control
        float amplitude = 1.0f;         // Velocity-scaled amplitude

        void reset()
        {
            samplesRemaining = 0;
            filterState = 0.0f;
        }

        // Generate one sample of filtered noise burst
        inline float nextSample()
        {
            if (samplesRemaining <= 0)
                return 0.0f;

            --samplesRemaining;

            // Generate white noise
            float noise = random.nextFloat() * 2.0f - 1.0f;

            // One-pole lowpass filter: y[n] = y[n-1] + coeff * (x[n] - y[n-1])
            filterState += filterCoefficient * (noise - filterState);

            return filterState * amplitude;
        }
    };

    MalletExciter exciter;

    // Voice parameters
    double sampleRate = 44100.0;
    float velocity = 0.0f;
    float outputGain = 1.0f;
    float velocityCurve = 0.5f;
    float malletHardness = 0.5f;
    float barMaterial = 0.5f;
    float resonance = 0.6f;

    // Voice state
    bool isActive = false;
    int samplesUntilRelease = 0;

    // Helper functions
    double noteToFrequency(int midiNote) const;
    float applyVelocityCurve(float rawVelocity) const;
    float getModeAmplitude(int modeIndex, float material) const;
    float getDecayTime(int modeIndex, float resonanceParam) const;
    void calculateModalCoefficients(float baseFreq);
};
