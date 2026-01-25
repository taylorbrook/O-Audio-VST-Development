/*
  ==============================================================================

    HarmonicGenerator.cpp
    OBass - Chebyshev Waveshaper Implementation

    Generates 2nd-5th harmonics using Chebyshev polynomials for controlled
    harmonic generation. Uses 4x oversampling to prevent aliasing artifacts.
    Output is bandpassed to 60-400Hz for psychoacoustic bass enhancement.

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

    // Use 2x oversampling (not 4x) to reduce latency and complexity
    // IIR filter for minimal latency
    oversamplerIIR = std::make_unique<juce::dsp::Oversampling<float>>(
        1,  // numChannels (mono bass processing)
        1,  // factor (2^1 = 2x oversampling) - reduced from 4x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        false,  // not max quality - faster
        true    // useIntegerLatency
    );

    // FIR version also at 2x for consistency
    oversamplerFIR = std::make_unique<juce::dsp::Oversampling<float>>(
        1, 1,  // 2x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,  // Use IIR for both to reduce latency
        false, true
    );

    // Initialize both oversamplers
    oversamplerIIR->initProcessing(static_cast<size_t>(blockSize));
    oversamplerFIR->initProcessing(static_cast<size_t>(blockSize));

    // Prepare filters FIRST
    juce::dsp::ProcessSpec monoSpec { sampleRate, static_cast<juce::uint32>(blockSize), 1 };
    outputBandpassLow.prepare(monoSpec);
    outputBandpassHigh.prepare(monoSpec);

    // THEN set coefficients
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 40.0f, 0.707f);
    outputBandpassLow.coefficients = hpCoeffs;

    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 300.0f, 0.707f);
    outputBandpassHigh.coefficients = lpCoeffs;

    // Reset to clear any garbage state
    outputBandpassLow.reset();
    outputBandpassHigh.reset();
}

//==============================================================================
void HarmonicGenerator::reset()
{
    // Reset filters only - oversamplers not used in simplified mode
    outputBandpassLow.reset();
    outputBandpassHigh.reset();
}

//==============================================================================
void HarmonicGenerator::setMode(Mode newMode)
{
    // SIMPLIFIED: Mode doesn't affect processing anymore (no oversampling)
    // Just store for API compatibility
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

    float* data = monoBuffer.getWritePointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        float x = data[i];

        // Skip invalid samples
        if (!std::isfinite(x))
        {
            data[i] = 0.0f;
            continue;
        }

        // Soft saturation approach - generates harmonics that ADD energy
        // tanh(x * drive) / tanh(drive) gives normalized soft clipping
        // This naturally generates 2nd and 3rd harmonics in phase with fundamental

        float drive = 3.0f;  // Amount of saturation
        float saturated = std::tanh(x * drive) / std::tanh(drive);

        // The harmonic content is the difference between saturated and clean
        float harmonics = saturated - x;

        // Scale up the harmonics (they're subtle from soft saturation)
        data[i] = harmonics * 2.0f;
    }
}

//==============================================================================
void HarmonicGenerator::processOversampled(float* data, int numSamples)
{
    // Process each oversampled sample through Chebyshev waveshaper
    for (int i = 0; i < numSamples; ++i)
    {
        float x = data[i];

        // Safety: skip processing if input is invalid
        if (std::isnan(x) || std::isinf(x))
        {
            data[i] = 0.0f;
            continue;
        }

        // Soft clip input to [-1, 1] range with tanh
        x = std::tanh(x * 2.0f) * 0.5f;  // Scale down for gentler saturation

        // Apply only 2nd and 3rd harmonics (simpler, safer)
        // These are the most important for psychoacoustic bass perception
        float h2 = T2(x) * 0.3f;  // Reduced weight
        float h3 = T3(x) * 0.2f;  // Reduced weight

        float output = (h2 + h3) * 0.5f;  // Mix and attenuate

        // Hard limit output
        output = std::max(-0.5f, std::min(0.5f, output));

        data[i] = output;
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
    // TEMPORARY: Return 0 to debug sample rate issue
    // The oversampler latency was causing Logic to report wrong sample rates
    return 0;
}
