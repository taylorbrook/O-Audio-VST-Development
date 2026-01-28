/*
  ==============================================================================

    CleanModeProcessor.cpp
    O-Bass - Psychoacoustic Bass Enhancement Processor

    Coordinates harmonic generation for bass enhancement using Chebyshev
    waveshaping.

    Note: Advanced features (pitch tracking, transient ducking, lookahead)
    are disabled due to real-time performance constraints:
    - Pitch tracking: YIN algorithm O(n²) complexity overruns audio thread
    - Latency reporting: Causes Logic Pro crash ("Sample Rate" error)
    See CODE_REVIEW.md Priority 4 for investigation status.

  ==============================================================================
*/

#include "CleanModeProcessor.h"
#include <cmath>

//==============================================================================
void CleanModeProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    maxBlockSize = static_cast<int>(spec.maximumBlockSize);

    // Prepare harmonic generator
    harmonicGenerator.prepare(spec);

    // Pre-allocate dry buffer to avoid allocation in process
    dryBuffer.setSize(1, maxBlockSize);
    dryBuffer.clear();
}

//==============================================================================
void CleanModeProcessor::reset()
{
    harmonicGenerator.reset();
    dryBuffer.clear();
}

//==============================================================================
void CleanModeProcessor::setMode(Mode mode)
{
    currentMode = mode;

    // Sync mode with harmonic generator
    harmonicGenerator.setMode(mode == Mode::HighFidelity
        ? HarmonicGenerator::Mode::HighFidelity
        : HarmonicGenerator::Mode::LowLatency);
}

//==============================================================================
CleanModeProcessor::Mode CleanModeProcessor::getMode() const
{
    return currentMode;
}

//==============================================================================
void CleanModeProcessor::setEnhanceAmount(float amount)
{
    enhanceAmount = juce::jlimit(0.0f, 1.0f, amount);
}

//==============================================================================
void CleanModeProcessor::setHighBandEnergy(float energy)
{
    highBandEnergy = juce::jlimit(0.0f, 1.0f, energy);
}

//==============================================================================
void CleanModeProcessor::setIntensityScale(float scale)
{
    intensityScale = juce::jlimit(1.0f, 2.0f, scale);
}

//==============================================================================
void CleanModeProcessor::process(juce::AudioBuffer<float>& monoBuffer)
{
    const int numSamples = monoBuffer.getNumSamples();
    if (numSamples == 0)
        return;

    // Store dry signal
    if (dryBuffer.getNumSamples() >= numSamples)
        dryBuffer.copyFrom(0, 0, monoBuffer, 0, 0, numSamples);

    // Generate harmonics
    harmonicGenerator.process(monoBuffer);

    // Mix dry + wet
    const float* dry = dryBuffer.getReadPointer(0);
    float* wet = monoBuffer.getWritePointer(0);
    float scaledEnhance = enhanceAmount * intensityScale;

    for (int i = 0; i < numSamples; ++i)
    {
        float output = dry[i] + wet[i] * scaledEnhance;
        wet[i] = std::tanh(output);
    }
}

//==============================================================================
int CleanModeProcessor::getLatencyInSamples() const
{
    // No lookahead - zero latency
    return 0;
}
