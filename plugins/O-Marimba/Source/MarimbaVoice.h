/*
   This file is part of O-Marimba, an Ouaricon Audio plugin.
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
#include <cmath>

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
    // v1.6.0: New timbre parameters
    void setStrikePosition(float position);
    void setOvertoneDamping(float damping);
    void setTone(float toneValue);

    // Phase 2.3: Set tuning engine reference
    void setTuningEngine(class TuningEngine* engine) { tuningEngine = engine; }

private:
    // Phase 2.3: Tuning engine pointer (not owned)
    class TuningEngine* tuningEngine = nullptr;
    // Phase 2.2: Modal synthesis constants
    static constexpr int NUM_MODES = 8;

    // Modal frequency ratios (inharmonic overtones for marimba timbre)
    // v1.5.0: Corrected ratios from acoustic research measurements
    // Mode 2 tuned to 4.0x (double octave) - signature of professional marimbas
    // Higher modes match measured marimba bar spectra (Euphonics/ISMA2019 research)
    static constexpr std::array<float, NUM_MODES> MODE_RATIOS = {
        1.00f, 4.00f, 9.24f, 16.27f, 24.22f, 33.54f, 42.97f, 54.0f
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

            // WR-05: NaN/Inf guard. std::abs(NaN) < 1e-8f is false, so the denormal flush
            // below would let a non-finite value latch permanently into the state
            // (y2=y1; y1=output) — silencing the voice forever and propagating NaN through
            // the tone filter and master bus. Reset state on any non-finite output.
            if (! std::isfinite(output))
            {
                output = 0.0f;
                y1 = 0.0f;
                y2 = 0.0f;
                return 0.0f;
            }

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
    // v1.6.0: New timbre parameters
    float strikePosition = 0.5f;    // 0.0 = edge, 0.5 = center, 1.0 = edge
    float overtoneDamping = 0.5f;   // Controls upper mode decay rate
    float toneValue = 0.75f;        // Lowpass filter brightness

    // v1.6.0: Tone lowpass filter state (one-pole)
    float toneFilterState = 0.0f;
    float toneFilterCoeff = 1.0f;   // Calculated from toneValue

    // Voice state
    bool isActive = false;
    int samplesUntilRelease = 0;
    int fadeOutSamples = 0;  // WR-04: length of the linear tail fade before termination

    // Helper functions
    double noteToFrequency(int midiNote) const;
    float applyVelocityCurve(float rawVelocity) const;
    float getModeAmplitude(int modeIndex, float material, float strikePos) const;
    float getDecayTime(int modeIndex, float resonanceParam, float overtoneD) const;
    void calculateModalCoefficients(float baseFreq);
    float getStrikePositionMultiplier(int modeIndex, float strikePos) const;  // v1.6.0
};
