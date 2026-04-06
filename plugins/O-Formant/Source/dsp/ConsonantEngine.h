/*
  ==============================================================================

    ConsonantEngine.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Place/manner articulation model for consonant synthesis.
    X axis (Place): spectral center frequency via resonant bandpass pair.
    Y axis (Manner): temporal profile from plosive burst to sustained fricative.
    Consonant noise is routed through formant filter bank by FormantVoice.

  ==============================================================================
*/

#pragma once
#include "FormantBiquad.h"
#include <JuceHeader.h>
#include <cmath>

class ConsonantEngine
{
public:
    void prepare (double sr, int voiceIndex) noexcept
    {
        sampleRate = sr;
        random = juce::Random (voiceIndex * 37 + 23);
        burstSamplesRemaining = 0;
        onsetSamplesRemaining = 0;
        onsetTotalSamples = static_cast<int> (0.025f * static_cast<float> (sr));

        updateCoefficients (0.5f, 0.5f, sr);
    }

    void reset() noexcept
    {
        placeFilter1.reset();
        placeFilter2.reset();
        burstSamplesRemaining = 0;
        onsetSamplesRemaining = 0;
        burstAmplitude = 0.0f;
        currentOnsetSuppression = 0.0f;
    }

    void triggerBurst (float velocity) noexcept
    {
        burstSamplesRemaining = cachedBurstDuration;
        onsetSamplesRemaining = onsetTotalSamples;
        burstAmplitude = velocity;
    }

    // Block-rate coefficient update: place = X (0-1), manner = Y (0-1)
    void updateCoefficients (float place, float manner, double sr) noexcept
    {
        float nyquist = static_cast<float> (sr * 0.5) - 100.0f;

        // Place of articulation -> spectral center frequency
        // Labial(0)=500Hz -> Alveolar(0.33)=3kHz -> Palatal(0.67)=6kHz -> Velar(1.0)=2kHz
        float centerFreq = computePlaceFrequency (place);
        centerFreq = juce::jmin (centerFreq, nyquist);

        float q = computePlaceQ (place);

        auto bp1 = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, centerFreq, q);
        placeFilter1.setCoefficients (bp1);

        float secondaryFreq = juce::jmin (centerFreq * 1.5f, nyquist);
        auto bp2 = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, secondaryFreq, q * 0.5f);
        placeFilter2.setCoefficients (bp2);

        // Manner of articulation -> temporal parameters
        cachedManner = manner;

        // Burst duration: plosive(0)=8ms, fricative(1)=80ms
        float burstMs = 0.008f + manner * 0.072f;
        cachedBurstDuration = juce::jmax (1, static_cast<int> (burstMs * static_cast<float> (sr)));

        // Burst decay rate: plosive(0)=12 (sharp), fricative(1)=2 (gentle)
        cachedBurstDecayRate = 12.0f - manner * 10.0f;
    }

    inline float getNextSample (float consonantLevel, bool autoConsonant) noexcept
    {
        // Onset suppression envelope
        if (onsetSamplesRemaining > 0)
        {
            float onsetProgress = 1.0f - static_cast<float> (onsetSamplesRemaining)
                                         / static_cast<float> (juce::jmax (1, onsetTotalSamples));
            // Suppression scales with plosive-ness (1 - manner)
            currentOnsetSuppression = std::exp (-6.0f * onsetProgress)
                                      * burstAmplitude * (1.0f - cachedManner);
            --onsetSamplesRemaining;
        }
        else
        {
            currentOnsetSuppression = 0.0f;
        }

        bool burstActive = autoConsonant && burstSamplesRemaining > 0;
        if (consonantLevel < 0.001f && ! burstActive)
            return 0.0f;

        // White noise source
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Place-of-articulation spectral shaping (dual resonant bandpass)
        float shaped = placeFilter1.processSample (noise)
                       + 0.4f * placeFilter2.processSample (noise);

        // Continuous component: fricatives produce sustained noise (scales with manner)
        float output = consonantLevel * cachedManner * shaped;

        // Burst component: plosive onset transient
        if (burstSamplesRemaining > 0)
        {
            float progress = 1.0f - static_cast<float> (burstSamplesRemaining)
                                    / static_cast<float> (juce::jmax (1, cachedBurstDuration));
            float burstEnv = std::exp (-cachedBurstDecayRate * progress) * burstAmplitude;
            // Burst amplitude boosted for plosives: 2x at manner=0, 1x at manner=1
            output += burstEnv * (2.0f - cachedManner) * shaped;
            --burstSamplesRemaining;
        }

        return output;
    }

    // Suppression factor for glottal source during plosive onset
    // Full suppression for plosives (manner=0), none for fricatives (manner=1)
    float getOnsetSuppression() const noexcept { return currentOnsetSuppression; }

private:
    // Piecewise linear place frequency mapping
    static float computePlaceFrequency (float place) noexcept
    {
        if (place <= 0.33f)
        {
            float t = place / 0.33f;
            return 500.0f + t * 2500.0f;   // Labial 500 -> Alveolar 3000
        }
        if (place <= 0.67f)
        {
            float t = (place - 0.33f) / 0.34f;
            return 3000.0f + t * 3000.0f;  // Alveolar 3000 -> Palatal 6000
        }
        float t = (place - 0.67f) / 0.33f;
        return 6000.0f - t * 4000.0f;      // Palatal 6000 -> Velar 2000
    }

    // Place-dependent filter Q: narrow for alveolar/palatal, broad for labial/velar
    static float computePlaceQ (float place) noexcept
    {
        if (place <= 0.33f)
        {
            float t = place / 0.33f;
            return 1.5f + t * 2.5f;        // Labial 1.5 -> Alveolar 4.0
        }
        if (place <= 0.67f)
        {
            float t = (place - 0.33f) / 0.34f;
            return 4.0f - t * 1.0f;        // Alveolar 4.0 -> Palatal 3.0
        }
        float t = (place - 0.67f) / 0.33f;
        return 3.0f - t * 1.0f;            // Palatal 3.0 -> Velar 2.0
    }

    juce::Random random;
    double sampleRate = 44100.0;

    FormantBiquad placeFilter1;
    FormantBiquad placeFilter2;

    float cachedManner = 0.5f;
    int cachedBurstDuration = 353;
    float cachedBurstDecayRate = 7.0f;

    int burstSamplesRemaining = 0;
    float burstAmplitude = 0.0f;

    int onsetSamplesRemaining = 0;
    int onsetTotalSamples = 1103;
    float currentOnsetSuppression = 0.0f;
};
