/*
  ==============================================================================

    WavetableOscillator.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "WavetableOscillator.h"
#include "MathConstants.h"
#include <JuceHeader.h>

void WavetableOscillator::setWarpType (WarpType type)
{
    if (warpType != type)
    {
        warpType = type;
        // Reset master phases when switching warp modes
        for (int i = 0; i < kMaxUnison; ++i)
            masterPhases[i] = phaseAccumulators[i];
    }
}

void WavetableOscillator::setWarpAmount (float amount)
{
    warpAmount = amount;
}

void WavetableOscillator::setFMInput (double value)
{
    fmInput = value;
}

void WavetableOscillator::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    setFrequency (frequency);
}

void WavetableOscillator::setFrequency (double freq)
{
    // Clamp at Nyquist: phaseIncrement must stay well below 1.0 or the
    // accumulators outrun the per-sample wrap and readSample() walks off
    // the end of the wavetable buffer.
    frequency = juce::jlimit (0.0, 0.5 * currentSampleRate, freq);
    phaseIncrement = frequency / currentSampleRate;
}

void WavetableOscillator::setPosition (float pos)
{
    position = pos;
}

void WavetableOscillator::setWavetable (const WavetableData* table)
{
    wavetable = table;
}

void WavetableOscillator::reset()
{
    for (int i = 0; i < kMaxUnison; ++i)
    {
        phaseAccumulators[i] = 0.0;
        masterPhases[i] = 0.0;
    }
}

void WavetableOscillator::resetWithPhase (double phase)
{
    for (int i = 0; i < kMaxUnison; ++i)
    {
        phaseAccumulators[i] = phase;
        masterPhases[i] = phase;
    }
}

void WavetableOscillator::resetWithRandomPhases()
{
    // Use a simple LCG for deterministic-per-note random phases
    uint32_t seed = static_cast<uint32_t> (reinterpret_cast<uintptr_t> (this) ^ 0x12345678u);
    for (int i = 0; i < kMaxUnison; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        phaseAccumulators[i] = static_cast<double> (seed) / 4294967296.0;
        masterPhases[i] = phaseAccumulators[i];
    }
}

void WavetableOscillator::setUnison (int count, float detune, float width)
{
    unisonCount = juce::jlimit (1, kMaxUnison, count);

    if (unisonCount == 1)
    {
        unisonDetuneFactors[0] = 1.0;
        unisonPanL[0] = 1.0;
        unisonPanR[0] = 1.0;
        unisonGain = 1.0;
        return;
    }

    unisonGain = 1.0 / std::sqrt (static_cast<double> (unisonCount));
    double centerIndex = (unisonCount - 1) / 2.0;
    double normFactor = centerIndex;

    for (int i = 0; i < unisonCount; ++i)
    {
        double normalizedPos = (i - centerIndex) / normFactor;

        // Detune: max 50 cents spread
        unisonDetuneFactors[i] = std::pow (2.0, normalizedPos * detune * 50.0 / 1200.0);

        // Pan: equal-power pan law
        double panNorm = (normalizedPos * width + 1.0) * 0.5; // Map to [0,1]
        panNorm = juce::jlimit (0.0, 1.0, panNorm);
        unisonPanL[i] = std::cos (panNorm * kHalfPi);
        unisonPanR[i] = std::sin (panNorm * kHalfPi);
    }
}

double WavetableOscillator::readSample (double phase) const
{
    if (wavetable == nullptr || wavetable->numFrames == 0)
        return 0.0;

    // Defensive wrap: warp/sync paths can hand us phase outside [0, 1),
    // and any excursion here indexes past the wavetable buffer.
    if (! std::isfinite (phase))
        return 0.0;
    phase -= std::floor (phase);

    // Sample position within frame
    double samplePos = phase * static_cast<double> (WavetableData::kTableSize);
    int idx0 = static_cast<int> (samplePos);
    double frac = samplePos - idx0;

    // Frame interpolation
    double framePos = position * static_cast<double> (wavetable->numFrames - 1);
    int frame0 = static_cast<int> (framePos);
    int frame1 = std::min (frame0 + 1, wavetable->numFrames - 1);
    double frameFrac = framePos - frame0;

    // Mipmap level interpolation
    double baseFreq = currentSampleRate / static_cast<double> (WavetableData::kTableSize);
    double levelFloat = std::log2 (std::max (frequency, baseFreq) / baseFreq);
    levelFloat = juce::jlimit (0.0, static_cast<double> (WavetableData::kNumMipmapLevels - 1), levelFloat);
    int level0 = static_cast<int> (levelFloat);
    int level1 = std::min (level0 + 1, WavetableData::kNumMipmapLevels - 1);
    double levelFrac = levelFloat - level0;

    // Trilinear interpolation (8 lookups)
    auto lerp = [] (double a, double b, double t) { return a + t * (b - a); };

    // Level 0
    double s00 = lerp (wavetable->getSample (level0, frame0, idx0),
                        wavetable->getSample (level0, frame0, idx0 + 1), frac);
    double s01 = lerp (wavetable->getSample (level0, frame1, idx0),
                        wavetable->getSample (level0, frame1, idx0 + 1), frac);
    double v0 = lerp (s00, s01, frameFrac);

    // Level 1
    double s10 = lerp (wavetable->getSample (level1, frame0, idx0),
                        wavetable->getSample (level1, frame0, idx0 + 1), frac);
    double s11 = lerp (wavetable->getSample (level1, frame1, idx0),
                        wavetable->getSample (level1, frame1, idx0 + 1), frac);
    double v1 = lerp (s10, s11, frameFrac);

    return lerp (v0, v1, levelFrac);
}

double WavetableOscillator::applyWarp (double phase, int voiceIndex) const
{
    switch (warpType)
    {
        case WarpType::Bend:
        {
            // Phase distortion: pow(phase, exponent) where exponent 1..4
            double exponent = 1.0 + static_cast<double> (warpAmount) * 3.0;
            return std::pow (phase, exponent);
        }
        case WarpType::FM:
        {
            // Phase modulation: add other osc output to phase
            double warped = phase + fmInput * static_cast<double> (warpAmount);
            warped = warped - std::floor (warped); // wrap to [0, 1)
            return warped;
        }
        default:
            return phase;
    }
}

void WavetableOscillator::getNextSampleStereo (double& outL, double& outR)
{
    if (wavetable == nullptr)
    {
        outL = outR = 0.0;
        return;
    }

    outL = 0.0;
    outR = 0.0;

    bool isSyncMode = (warpType == WarpType::Sync || warpType == WarpType::Window)
                      && warpAmount > 0.001f;

    for (int i = 0; i < unisonCount; ++i)
    {
        double readPhase = phaseAccumulators[i];

        if (isSyncMode)
        {
            double sample = readSample (readPhase) * unisonGain;

            if (warpType == WarpType::Window)
                sample *= std::sin (kPi * masterPhases[i]);

            outL += sample * unisonPanL[i];
            outR += sample * unisonPanR[i];

            double masterInc = phaseIncrement * unisonDetuneFactors[i];
            masterPhases[i] += masterInc;

            double syncRatio = 1.0 + static_cast<double> (warpAmount) * 3.0;
            phaseAccumulators[i] += masterInc * syncRatio;

            if (phaseAccumulators[i] >= 1.0)
                phaseAccumulators[i] -= std::floor (phaseAccumulators[i]);

            if (masterPhases[i] >= 1.0)
            {
                masterPhases[i] -= std::floor (masterPhases[i]);
                // Re-seed can land ≥ 1.0 (syncRatio up to 4) — wrap it too
                phaseAccumulators[i] = masterPhases[i] * syncRatio;
                phaseAccumulators[i] -= std::floor (phaseAccumulators[i]);
            }
        }
        else
        {
            double warped = applyWarp (readPhase, i);
            double sample = readSample (warped) * unisonGain;
            outL += sample * unisonPanL[i];
            outR += sample * unisonPanR[i];

            phaseAccumulators[i] += phaseIncrement * unisonDetuneFactors[i];
            if (phaseAccumulators[i] >= 1.0)
                phaseAccumulators[i] -= std::floor (phaseAccumulators[i]);
        }
    }
}
