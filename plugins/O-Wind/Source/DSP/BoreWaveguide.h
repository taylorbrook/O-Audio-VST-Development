/*
  ==============================================================================

    BoreWaveguide.h
    O-Wind - Bidirectional Bore Waveguide with Filters
    Ouaricon Audio
    Developer: Taylor Brook

    Two Thiran-interpolated delay lines (forward + backward) modeling wave
    propagation in the flute bore. Includes bore loss filter, end reflection
    filter, and radiation filter. The bore IS the body -- no separate body
    resonator needed.

    Signal flow: DC blocked jet output -> bore forward delay -> bore loss ->
    end reflection -> bore backward delay -> feedback out
    Parallel tap: bore loss output -> radiation filter -> voice output

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <array>

class BoreWaveguide
{
public:
    struct ProcessResult
    {
        float voiceOutput;
        float feedback;
    };

    BoreWaveguide()
        : boreFwd (2048), boreBwd (2048)
    {
    }

    void prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;

        juce::dsp::ProcessSpec spec {
            sampleRate,
            static_cast<juce::uint32> (maxBlockSize),
            1
        };

        // Prepare bore delay lines. Each line holds half the bore round trip;
        // size for the lowest MIDI note (8.176 Hz) at the prepared rate — a
        // fixed 2048 silently clamps (mistunes) low notes at >= 96 kHz hosts.
        const int maxHalfDelay = static_cast<int> (std::ceil (sampleRate / 8.176 * 0.5)) + 8;
        boreFwd.prepare (spec);
        boreFwd.setMaximumDelayInSamples (maxHalfDelay);
        boreBwd.prepare (spec);
        boreBwd.setMaximumDelayInSamples (maxHalfDelay);

        // Prepare filters (two cascaded lowpass for frequency-dependent bore loss)
        boreLossLow.prepare (spec);
        boreLossHigh.prepare (spec);
        endReflectionFilter.prepare (spec);
        radiationFilter.prepare (spec);

        // Prepare allpass inharmonicity filters
        allpass1.prepare (spec);
        allpass2.prepare (spec);

        // Default filter coefficients
        updateBoreLossFilter (6000.0f);
        updateEndReflectionFilter (3000.0f);
        updateRadiationFilter (300.0f);

        // Build bore delay table
        buildBoreDelayTable();
    }

    void reset()
    {
        boreFwd.reset();
        boreBwd.reset();
        boreLossLow.reset();
        boreLossHigh.reset();
        endReflectionFilter.reset();
        radiationFilter.reset();
        allpass1.reset();
        allpass2.reset();
        boreFeedback = 0.0f;
    }

    // Set bore delay directly (for Tier 1 tone hole lookup)
    void setBoreDelay (float delaySamples)
    {
        currentBoreDelay = std::max (4.0f, delaySamples);
    }

    float getBoreDelay() const { return currentBoreDelay; }

    // Parameters (set once per block)
    void setEndReflection (float coeff)     { endReflectionCoeff = coeff; }
    void setInfiniteSustain (float amount)  { infiniteSustainParam = amount; }
    void setSubHarmonics (float amount)     { subHarmonicsParam = amount; }

    // Inharmonicity: sets allpass coefficient for frequency-dependent phase delay
    // in the bore feedback path. Coefficient a controls partial detuning (~5-15 cents).
    void setInharmonicity (float effective)
    {
        float a = effective * 0.05f;
        if (std::abs (a - inharmonicityCoeff) < 1.0e-6f)
            return;
        inharmonicityCoeff = a;
        if (sampleRate > 0.0 && a > 0.0f)
        {
            // Assign raw arrays in place — constructing a temporary
            // IIR::Coefficients here heap-allocates on the audio thread
            const std::array<float, 4> coeffs { a, 1.0f, 1.0f, a };
            *allpass1.coefficients = coeffs;
            *allpass2.coefficients = coeffs;
        }
    }

    // Update bore loss: two cascaded 1st-order lowpass filters for frequency-dependent loss.
    // Low cutoff (~2kHz) provides base viscothermal damping.
    // High cutoff (~8kHz) adds extra harmonic rolloff — higher harmonics lose more energy
    // per round trip, creating natural spectral thinning.
    // The cutoffHz parameter from APVTS scales both cutoffs proportionally.
    void updateBoreLossFilter (float cutoffHz)
    {
        if (sampleRate <= 0.0) return;
        // Scale factor from user toneColor control (cutoffHz ranges ~1000-12000)
        float scale = cutoffHz / 6000.0f;  // normalized to default
        float lowCut  = juce::jlimit (500.0f,  static_cast<float> (sampleRate * 0.45), 2000.0f * scale);
        float highCut = juce::jlimit (2000.0f, static_cast<float> (sampleRate * 0.45), 8000.0f * scale);
        // Assign the ArrayCoefficients result directly — wrapping it in a
        // temporary IIR::Coefficients heap-allocates on the audio thread
        *boreLossLow.coefficients =
            juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass (sampleRate, lowCut);
        *boreLossHigh.coefficients =
            juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass (sampleRate, highCut);
    }

    // End reflection: high-shelf that reduces reflection above ~2kHz.
    // Higher frequencies radiate more efficiently from the open end,
    // so less energy reflects back into the bore at high frequencies.
    void updateEndReflectionFilter (float cutoffHz)
    {
        if (sampleRate <= 0.0) return;
        float shelfFreq = juce::jlimit (500.0f, static_cast<float> (sampleRate * 0.45), cutoffHz);
        // High-shelf with negative gain: attenuates above shelfFreq
        // -6dB shelf gives realistic open-end radiation loss for upper harmonics
        float shelfGainDb = -6.0f;
        *endReflectionFilter.coefficients =
            juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (
                sampleRate, shelfFreq, 0.707f, juce::Decibels::decibelsToGain (shelfGainDb));
    }

    void updateRadiationFilter (float cutoffHz)
    {
        if (sampleRate <= 0.0) return;
        float clampedCutoff = juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.45), cutoffHz);
        *radiationFilter.coefficients =
            juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass (sampleRate, clampedCutoff);
    }

    // Core per-sample processing of the bore waveguide
    // Input: DC-blocked signal from jet nonlinearity
    // Returns: voice output (radiated) and feedback (for next sample's excitation)
    ProcessResult processSample (float dcBlockedInput)
    {
        float halfDelay = currentBoreDelay * 0.5f;
        halfDelay = std::max (2.0f, halfDelay);

        // Inject into forward wave
        boreFwd.pushSample (0, dcBlockedInput);
        float pPlus = boreFwd.popSample (0, halfDelay);

        // Bore loss: cascaded 1st-order lowpass filters for frequency-dependent loss.
        // boreLossLow (~2kHz) = base viscothermal damping
        // boreLossHigh (~8kHz) = extra harmonic rolloff
        float pPlusFiltered;
        if (infiniteSustainParam > 0.0f)
        {
            float lossAmount = 1.0f - infiniteSustainParam * 0.95f;
            float filtered = boreLossHigh.processSample (boreLossLow.processSample (pPlus));
            pPlusFiltered = filtered * lossAmount + pPlus * (1.0f - lossAmount);
        }
        else
        {
            pPlusFiltered = boreLossHigh.processSample (boreLossLow.processSample (pPlus));
        }

        // Sub-harmonics: asymmetric soft-clipping for period doubling
        if (subHarmonicsParam > 0.0f)
        {
            float asymmetric = (pPlusFiltered >= 0.0f)
                ? pPlusFiltered
                : pPlusFiltered * 0.5f;
            pPlusFiltered = pPlusFiltered + subHarmonicsParam * (asymmetric - pPlusFiltered);
        }

        // End reflection filter with sign inversion (open end = pressure node)
        float reflected = -endReflectionCoeff * endReflectionFilter.processSample (pPlusFiltered);
        boreBwd.pushSample (0, reflected);
        boreFeedback = boreBwd.popSample (0, halfDelay);
        boreFeedback *= feedbackGain;  // compensate cumulative filter losses

        // Allpass inharmonicity: frequency-dependent phase delay detunes upper
        // partials by ~5-15 cents, approximating conical bore behavior
        if (inharmonicityCoeff > 0.0f)
        {
            boreFeedback = allpass1.processSample (boreFeedback);
            boreFeedback = allpass2.processSample (boreFeedback);
        }

        // Radiation filter (output tap from forward wave at bore end)
        float voiceOutput = radiationFilter.processSample (pPlusFiltered);

        return { voiceOutput, boreFeedback };
    }

    float getFeedback() const { return boreFeedback; }

    // Compute combined filter phase delay at the fundamental frequency (in samples)
    // Used for dynamic loop delay compensation instead of static constant
    float getFilterPhaseDelay (float fundamentalFreq) const
    {
        if (sampleRate <= 0.0 || fundamentalFreq <= 0.0f
            || fundamentalFreq >= static_cast<float> (sampleRate * 0.45))
            return 2.0f;  // fallback to static estimate

        double freq = static_cast<double> (fundamentalFreq);
        float omega = static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                           * freq / sampleRate);

        // Phase delay in samples = -phase(radians) / omega
        float totalDelay = 0.0f;

        if (boreLossLow.coefficients != nullptr)
        {
            auto phase = static_cast<float> (
                boreLossLow.coefficients->getPhaseForFrequency (freq, sampleRate));
            totalDelay += -phase / omega;
        }

        if (boreLossHigh.coefficients != nullptr)
        {
            auto phase = static_cast<float> (
                boreLossHigh.coefficients->getPhaseForFrequency (freq, sampleRate));
            totalDelay += -phase / omega;
        }

        if (endReflectionFilter.coefficients != nullptr)
        {
            auto phase = static_cast<float> (
                endReflectionFilter.coefficients->getPhaseForFrequency (freq, sampleRate));
            totalDelay += -phase / omega;
        }

        // Allpass inharmonicity filters (2x, same coefficients)
        if (inharmonicityCoeff > 0.0f && allpass1.coefficients != nullptr)
        {
            auto phase = static_cast<float> (
                allpass1.coefficients->getPhaseForFrequency (freq, sampleRate));
            totalDelay += -phase / omega * 2.0f;  // 2 cascaded allpass filters
        }

        return std::max (0.0f, totalDelay);
    }

    // Build bore delay lookup table for Tier 1 tone holes
    void buildBoreDelayTable()
    {
        if (sampleRate <= 0.0)
            return;

        for (int note = 0; note < 128; ++note)
        {
            float freq = static_cast<float> (440.0 * std::pow (2.0, (note - 69.0) / 12.0));
            float totalDelay = static_cast<float> (sampleRate) / freq;
            totalDelay -= filterGroupDelay;
            totalDelay = std::max (4.0f, totalDelay);
            boreDelayTable[static_cast<size_t> (note)] = totalDelay;
        }
    }

    float getDelayForNote (int midiNote) const
    {
        return boreDelayTable[static_cast<size_t> (juce::jlimit (0, 127, midiNote))];
    }

private:
    double sampleRate = 44100.0;

    // Bore delay lines (Thiran for flat amplitude response in resonator)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> boreFwd;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> boreBwd;

    // Filters (two cascaded lowpass for frequency-dependent bore loss)
    juce::dsp::IIR::Filter<float> boreLossLow;   // base damping ~2kHz
    juce::dsp::IIR::Filter<float> boreLossHigh;  // harmonic rolloff ~8kHz
    juce::dsp::IIR::Filter<float> endReflectionFilter;  // high-shelf for open-end radiation
    juce::dsp::IIR::Filter<float> radiationFilter;

    // Allpass inharmonicity filters (conical bore approximation)
    juce::dsp::IIR::Filter<float> allpass1;
    juce::dsp::IIR::Filter<float> allpass2;

    // State
    float boreFeedback = 0.0f;
    float currentBoreDelay = 200.0f;

    // Parameters
    float endReflectionCoeff = 0.5f;
    float infiniteSustainParam = 0.0f;
    float subHarmonicsParam = 0.0f;
    float inharmonicityCoeff = 0.0f;  // allpass coefficient for partial detuning

    // Filter group delay compensation (approx 2 samples from bore loss + end refl)
    static constexpr float filterGroupDelay = 2.0f;

    // Feedback gain: bore is properly lossy (1.0 = no artificial energy injection).
    // Jet spatial amplification in JetExciter provides the energy source per Verge (1995).
    static constexpr float feedbackGain = 1.0f;

    // Bore delay lookup table (Tier 1 tone holes)
    std::array<float, 128> boreDelayTable {};
};
