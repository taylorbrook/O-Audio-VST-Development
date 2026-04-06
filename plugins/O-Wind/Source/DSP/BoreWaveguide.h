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

        // Prepare bore delay lines
        boreFwd.prepare (spec);
        boreFwd.setMaximumDelayInSamples (2048);
        boreBwd.prepare (spec);
        boreBwd.setMaximumDelayInSamples (2048);

        // Prepare filters
        boreLossFilter.prepare (spec);
        endReflectionFilter.prepare (spec);
        radiationFilter.prepare (spec);

        // Default filter coefficients
        updateBoreLossFilter (6000.0f, 0.707f);
        updateEndReflectionFilter (3000.0f);
        updateRadiationFilter (300.0f);

        // Build bore delay table
        buildBoreDelayTable();
    }

    void reset()
    {
        boreFwd.reset();
        boreBwd.reset();
        boreLossFilter.reset();
        endReflectionFilter.reset();
        radiationFilter.reset();
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

    // Update filter coefficients (call on parameter change, not per-sample)
    void updateBoreLossFilter (float cutoffHz, float q)
    {
        if (sampleRate <= 0.0) return;
        float clampedCutoff = juce::jlimit (200.0f, static_cast<float> (sampleRate * 0.45), cutoffHz);
        float clampedQ = juce::jlimit (0.1f, 5.0f, q);
        *boreLossFilter.coefficients = juce::dsp::IIR::Coefficients<float> (
            juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, clampedCutoff, clampedQ));
    }

    void updateEndReflectionFilter (float cutoffHz)
    {
        if (sampleRate <= 0.0) return;
        float clampedCutoff = juce::jlimit (200.0f, static_cast<float> (sampleRate * 0.45), cutoffHz);
        *endReflectionFilter.coefficients = juce::dsp::IIR::Coefficients<float> (
            juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass (sampleRate, clampedCutoff));
    }

    void updateRadiationFilter (float cutoffHz)
    {
        if (sampleRate <= 0.0) return;
        float clampedCutoff = juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.45), cutoffHz);
        *radiationFilter.coefficients = juce::dsp::IIR::Coefficients<float> (
            juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass (sampleRate, clampedCutoff));
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

        // Bore loss filter (inside loop -- models viscothermal loss)
        float pPlusFiltered;
        if (infiniteSustainParam > 0.0f)
        {
            // Infinite sustain: blend between filtered (lossy) and unfiltered (lossless)
            float lossAmount = 1.0f - infiniteSustainParam * 0.95f;  // never fully zero loss
            float filtered = boreLossFilter.processSample (pPlus);
            // Blend: at lossAmount=1 (no sustain), fully filtered; at lossAmount~0.05, mostly direct
            pPlusFiltered = filtered * lossAmount + pPlus * (1.0f - lossAmount);
        }
        else
        {
            pPlusFiltered = boreLossFilter.processSample (pPlus);
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

        if (boreLossFilter.coefficients != nullptr)
        {
            auto phase = static_cast<float> (
                boreLossFilter.coefficients->getPhaseForFrequency (freq, sampleRate));
            totalDelay += -phase / omega;
        }

        if (endReflectionFilter.coefficients != nullptr)
        {
            auto phase = static_cast<float> (
                endReflectionFilter.coefficients->getPhaseForFrequency (freq, sampleRate));
            totalDelay += -phase / omega;
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

    // Filters
    juce::dsp::IIR::Filter<float> boreLossFilter;
    juce::dsp::IIR::Filter<float> endReflectionFilter;
    juce::dsp::IIR::Filter<float> radiationFilter;

    // State
    float boreFeedback = 0.0f;
    float currentBoreDelay = 200.0f;

    // Parameters
    float endReflectionCoeff = 0.5f;
    float infiniteSustainParam = 0.0f;
    float subHarmonicsParam = 0.0f;

    // Filter group delay compensation (approx 2 samples from bore loss + end refl)
    static constexpr float filterGroupDelay = 2.0f;

    // Feedback gain: compensates per-round-trip IIR filter losses to sustain oscillation.
    // Safe because JetNonlinearity gates to zero on release, breaking the feedback loop.
    static constexpr float feedbackGain = 1.02f;

    // Bore delay lookup table (Tier 1 tone holes)
    std::array<float, 128> boreDelayTable {};
};
