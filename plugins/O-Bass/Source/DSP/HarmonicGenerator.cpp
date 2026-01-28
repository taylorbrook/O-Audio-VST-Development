/*
  ==============================================================================

    HarmonicGenerator.cpp
    O-Bass - Chebyshev Waveshaper Implementation

    Generates 2nd-5th harmonics using Chebyshev polynomials for controlled
    harmonic generation. Uses 4x oversampling to prevent aliasing artifacts.
    Output is bandpassed to 40-400Hz for psychoacoustic bass enhancement.

  ==============================================================================
*/

#include "HarmonicGenerator.h"
#include <cmath>

//==============================================================================
// Chebyshev Polynomials T2-T5
// For input x in [-1, 1], Tn(cos(theta)) = cos(n*theta)
// This generates the nth harmonic when applied to a sinusoidal input

inline float T2(float x) { return 2.0f * x * x - 1.0f; }
inline float T3(float x) { return 4.0f * x * x * x - 3.0f * x; }
inline float T4(float x) { return 8.0f * x * x * x * x - 8.0f * x * x + 1.0f; }
inline float T5(float x) { return 16.0f * x * x * x * x * x - 20.0f * x * x * x + 5.0f * x; }

//==============================================================================
HarmonicGenerator::HarmonicGenerator()
{
    // Dual oversamplers created in prepare() after we know spec
}

//==============================================================================
void HarmonicGenerator::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    blockSize = static_cast<int>(spec.maximumBlockSize);

    // OVERSAMPLING DISABLED: JUCE Oversampling causes Logic Pro crash
    // Oversamplers not initialized - process at native sample rate instead
    // TODO: Investigate JUCE Oversampling issue in future version
    oversamplerIIR = nullptr;
    oversamplerFIR = nullptr;

    // Prepare bandpass filters
    juce::dsp::ProcessSpec monoSpec { sampleRate, static_cast<juce::uint32>(blockSize), 1 };
    outputBandpassLow.prepare(monoSpec);
    outputBandpassHigh.prepare(monoSpec);

    // Set filter coefficients - bandpass 30-500Hz for bass harmonics
    // Wider range to preserve more harmonic content
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 30.0f, 0.707f);
    outputBandpassLow.coefficients = hpCoeffs;

    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 500.0f, 0.707f);
    outputBandpassHigh.coefficients = lpCoeffs;

    // Reset to clear any garbage state
    outputBandpassLow.reset();
    outputBandpassHigh.reset();
}

//==============================================================================
void HarmonicGenerator::reset()
{
    // Reset filters
    outputBandpassLow.reset();
    outputBandpassHigh.reset();

    // Reset oversamplers (they maintain internal buffers)
    if (oversamplerIIR)
        oversamplerIIR->reset();
    if (oversamplerFIR)
        oversamplerFIR->reset();
}

//==============================================================================
void HarmonicGenerator::setMode(Mode newMode)
{
    // Store mode atomically - selects IIR (LowLatency) or FIR (HighFidelity) oversampler
    activeMode.store(newMode, std::memory_order_release);
}

//==============================================================================
void HarmonicGenerator::setHarmonicWeights(float h2, float h3, float h4, float h5)
{
    harmonicWeights[0] = h2;
    harmonicWeights[1] = h3;
    harmonicWeights[2] = h4;
    harmonicWeights[3] = h5;
}

//==============================================================================
void HarmonicGenerator::setAdaptiveHarmonics(float fundamentalHz)
{
    // Lower frequencies need more harmonics for psychoacoustic effect
    // Sub-bass (<40Hz): nearly inaudible, needs maximum harmonics
    // Deep bass (40-80Hz): still needs strong harmonic support
    // Mid-bass (80-120Hz): moderate enhancement
    // Upper bass (120+Hz): already somewhat audible, minimal harmonics

    if (fundamentalHz < 40.0f)
        activeHarmonicCount = 5;  // Sub-bass: maximum
    else if (fundamentalHz < 80.0f)
        activeHarmonicCount = 4;  // Deep bass
    else if (fundamentalHz < 120.0f)
        activeHarmonicCount = 3;  // Mid-bass
    else
        activeHarmonicCount = 2;  // Upper bass: minimal
}

//==============================================================================
void HarmonicGenerator::process(juce::AudioBuffer<float>& monoBuffer)
{
    const int numSamples = monoBuffer.getNumSamples();
    if (numSamples == 0)
        return;

    // OVERSAMPLING BYPASSED: JUCE Oversampling causes Logic Pro crash
    // ("Sample Rate XXXXX" error - memory corruption from oversampler internals)
    // Process directly at native sample rate with gentler waveshaping to minimize aliasing

    float* data = monoBuffer.getWritePointer(0);

    // Process waveshaping at native rate (no oversampling)
    processOversampled(data, numSamples);

    // Apply output bandpass filter (40-300Hz) at original sample rate
    for (int i = 0; i < numSamples; ++i)
    {
        data[i] = outputBandpassLow.processSample(data[i]);
        data[i] = outputBandpassHigh.processSample(data[i]);
    }
}

//==============================================================================
void HarmonicGenerator::processOversampled(float* data, int numSamples)
{
    // Generate harmonics using Chebyshev polynomials
    // Output is ONLY the harmonic content (added to dry signal in CleanModeProcessor)
    for (int i = 0; i < numSamples; ++i)
    {
        float x = data[i];

        // Safety: skip processing if input is invalid
        if (std::isnan(x) || std::isinf(x))
        {
            data[i] = 0.0f;
            continue;
        }

        // Soft clip input to [-1, 1] for Chebyshev polynomials
        float clipped = std::tanh(x * 2.0f);

        // Generate 2nd and 3rd harmonics (most important for psychoacoustic bass)
        float h2 = T2(clipped) * 0.5f;   // 2nd harmonic - adds warmth
        float h3 = T3(clipped) * 0.3f;   // 3rd harmonic - adds presence

        // Output is the harmonic content only (will be mixed with dry)
        float harmonics = (h2 + h3) * 0.7f;

        // Soft limit
        data[i] = std::tanh(harmonics);
    }
}

//==============================================================================
juce::dsp::Oversampling<float>* HarmonicGenerator::getActiveOversampler() const
{
    if (activeMode.load(std::memory_order_acquire) == Mode::LowLatency)
        return oversamplerIIR.get();
    else
        return oversamplerFIR.get();
}

//==============================================================================
int HarmonicGenerator::getLatencyInSamples() const
{
    // Oversampling disabled - no latency
    return 0;
}
