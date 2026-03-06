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

void WavetableOscillator::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    phaseIncrement = frequency / currentSampleRate;
}

void WavetableOscillator::setFrequency (double freq)
{
    frequency = freq;
    phaseIncrement = freq / currentSampleRate;
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
        phaseAccumulators[i] = 0.0;
}

void WavetableOscillator::resetWithPhase (double phase)
{
    for (int i = 0; i < kMaxUnison; ++i)
        phaseAccumulators[i] = phase;
}

void WavetableOscillator::resetWithRandomPhases()
{
    // Use a simple LCG for deterministic-per-note random phases
    uint32_t seed = static_cast<uint32_t> (reinterpret_cast<uintptr_t> (this) ^ 0x12345678u);
    for (int i = 0; i < kMaxUnison; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        phaseAccumulators[i] = static_cast<double> (seed) / 4294967296.0;
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

double WavetableOscillator::getNextSample()
{
    if (wavetable == nullptr)
        return 0.0;

    double output = 0.0;

    for (int i = 0; i < unisonCount; ++i)
    {
        output += readSample (phaseAccumulators[i]) * unisonGain;

        phaseAccumulators[i] += phaseIncrement * unisonDetuneFactors[i];
        if (phaseAccumulators[i] >= 1.0)
            phaseAccumulators[i] -= 1.0;
    }

    return output;
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

    for (int i = 0; i < unisonCount; ++i)
    {
        double sample = readSample (phaseAccumulators[i]) * unisonGain;
        outL += sample * unisonPanL[i];
        outR += sample * unisonPanR[i];

        phaseAccumulators[i] += phaseIncrement * unisonDetuneFactors[i];
        if (phaseAccumulators[i] >= 1.0)
            phaseAccumulators[i] -= 1.0;
    }
}
