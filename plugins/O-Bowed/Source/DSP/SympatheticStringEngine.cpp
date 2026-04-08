/*
  ==============================================================================

    SympatheticStringEngine.cpp
    O-Bowed - Passive Karplus-Strong Sympathetic String Resonators
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "SympatheticStringEngine.h"
#include <algorithm>
#include <cmath>

//==============================================================================
void SympatheticStringEngine::prepare (double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32> (maxBlockSize),
        1
    };

    int maxDelaySamples = static_cast<int> (sampleRate / 20.0) + 100;

    for (auto& s : strings)
    {
        s.delay.prepare (spec);
        s.delay.setMaximumDelayInSamples (maxDelaySamples);
        s.delay.reset();
        s.filterState = 0.0f;
        s.energyEstimate = 0.0f;
        s.active = false;
    }

    activeCount = 0;
}

//==============================================================================
void SympatheticStringEngine::reset()
{
    for (auto& s : strings)
    {
        s.delay.reset();
        s.filterState = 0.0f;
        s.energyEstimate = 0.0f;
        s.active = false;
    }
    activeCount = 0;
}

//==============================================================================
void SympatheticStringEngine::setCount (int count)
{
    count = juce::jlimit (0, MAX_SYMPATHETICS, count);

    // Deactivate excess strings
    for (int i = count; i < activeCount; ++i)
    {
        strings[i].delay.reset();
        strings[i].filterState = 0.0f;
        strings[i].energyEstimate = 0.0f;
        strings[i].active = false;
    }

    activeCount = count;
}

//==============================================================================
void SympatheticStringEngine::setAmount (float amount)
{
    amountParam = amount;
}

//==============================================================================
void SympatheticStringEngine::setDecay (float decay)
{
    // Map 0.0-1.0 to loss coefficient 0.990-0.9999
    // 0.0 = short ring (0.990), 1.0 = very long ring (0.9999)
    decayParam = decay;
}

//==============================================================================
void SympatheticStringEngine::updateTunings (const float* fundamentals, int numFundamentals)
{
    if (activeCount <= 0 || numFundamentals <= 0)
        return;

    float candidateFreqs[128];
    int numCandidates = 0;
    computeHarmonics (fundamentals, numFundamentals,
                      candidateFreqs, 128, numCandidates);

    int toAssign = juce::jmin (activeCount, numCandidates);
    for (int i = 0; i < toAssign; ++i)
        tuneString (i, candidateFreqs[i]);

    // Deactivate any unassigned strings
    for (int i = toAssign; i < activeCount; ++i)
        strings[i].active = false;
}

//==============================================================================
void SympatheticStringEngine::computeHarmonics (const float* fundamentals, int numFundamentals,
                                                 float* outFreqs, int maxOut, int& numOut)
{
    numOut = 0;
    float nyquist = static_cast<float> (currentSampleRate * 0.5);

    // Generate harmonic candidates from all fundamentals
    struct Candidate { float freq; int harmonicOrder; };
    Candidate candidates[256];
    int numCandidates = 0;

    for (int f = 0; f < numFundamentals && numCandidates < 250; ++f)
    {
        float fund = fundamentals[f];
        if (fund < 20.0f)
            continue;

        for (int h = 1; h <= 16 && numCandidates < 250; ++h)
        {
            float harmFreq = fund * static_cast<float> (h);
            if (harmFreq >= nyquist || harmFreq < 20.0f)
                break;

            candidates[numCandidates++] = { harmFreq, h };
        }
    }

    // Sort by harmonic order (lower = stronger physical coupling)
    std::sort (candidates, candidates + numCandidates,
               [] (const Candidate& a, const Candidate& b) {
                   return a.harmonicOrder < b.harmonicOrder;
               });

    // Deduplicate within 5 cents
    for (int i = 0; i < numCandidates && numOut < maxOut; ++i)
    {
        bool duplicate = false;
        for (int j = 0; j < numOut; ++j)
        {
            float ratio = candidates[i].freq / outFreqs[j];
            float centsDiff = std::abs (std::log2 (ratio) * 1200.0f);
            if (centsDiff < 5.0f)
            {
                duplicate = true;
                break;
            }
        }
        if (! duplicate)
            outFreqs[numOut++] = candidates[i].freq;
    }
}

//==============================================================================
void SympatheticStringEngine::tuneString (int index, float frequency)
{
    auto& s = strings[index];
    float delaySamples = static_cast<float> (currentSampleRate) / frequency;
    s.delay.setDelay (delaySamples);
    s.frequency = frequency;
    // Map decayParam 0.0-1.0 to lossCoeff 0.990-0.9999
    s.lossCoeff = 0.990f + decayParam * 0.0099f;

    // Pan position across stereo field
    float pan = (activeCount > 1)
        ? static_cast<float> (index) / static_cast<float> (activeCount - 1)
        : 0.5f;
    s.panL = std::cos (pan * HALF_PI);
    s.panR = std::sin (pan * HALF_PI);
    s.active = true;
}

//==============================================================================
SympatheticStringEngine::StereoSample
SympatheticStringEngine::processSample (float excitation)
{
    StereoSample out { 0.0f, 0.0f };
    float scaledExcitation = excitation * amountParam * 0.01f;

    for (int i = 0; i < activeCount; ++i)
    {
        auto& s = strings[i];
        if (! s.active)
            continue;

        // Energy gating: skip if silent and no excitation
        if (s.energyEstimate < 1e-5f && std::abs (scaledExcitation) < 1e-5f)
            continue;

        float delayed = s.delay.popSample (0);
        // One-pole loss filter
        s.filterState = s.lossCoeff * s.filterState + (1.0f - s.lossCoeff) * delayed;
        // Feed excitation + filtered feedback into delay
        s.delay.pushSample (0, s.filterState + scaledExcitation);

        float sample = s.filterState;
        s.energyEstimate = 0.999f * s.energyEstimate + 0.001f * std::abs (sample);

        out.left  += sample * s.panL;
        out.right += sample * s.panR;
    }

    return out;
}
