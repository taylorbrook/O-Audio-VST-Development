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

    // Use 4x oversampling for clean harmonics from Chebyshev waveshaping
    // IIR filter for Low Latency mode - minimal phase distortion
    oversamplerIIR = std::make_unique<juce::dsp::Oversampling<float>>(
        1,      // numChannels (mono bass processing)
        2,      // factor (2^2 = 4x oversampling)
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,   // max quality for clean harmonics
        true    // useIntegerLatency
    );

    // FIR version for High Fidelity mode - maximum quality
    oversamplerFIR = std::make_unique<juce::dsp::Oversampling<float>>(
        1,      // numChannels (mono bass processing)
        2,      // factor (2^2 = 4x oversampling)
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true,   // max quality for clean harmonics
        true    // useIntegerLatency
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

    // Get active oversampler based on latency mode
    auto* oversampler = getActiveOversampler();
    if (!oversampler)
    {
        // Fallback: clear buffer if oversamplers not ready
        monoBuffer.clear();
        return;
    }

    // Create audio block from mono buffer
    juce::dsp::AudioBlock<float> inputBlock(monoBuffer);

    // Upsample to 4x sample rate
    auto oversampledBlock = oversampler->processSamplesUp(inputBlock);

    // Process at elevated sample rate (Chebyshev waveshaping)
    processOversampled(oversampledBlock.getChannelPointer(0),
                       static_cast<int>(oversampledBlock.getNumSamples()));

    // Downsample back to original rate (anti-aliasing filter applied automatically)
    oversampler->processSamplesDown(inputBlock);

    // Apply output bandpass filter (40-400Hz) at original sample rate
    float* data = monoBuffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        data[i] = outputBandpassLow.processSample(data[i]);
        data[i] = outputBandpassHigh.processSample(data[i]);
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
    auto* oversampler = getActiveOversampler();
    if (!oversampler)
        return 0;

    // JUCE oversampler returns float latency - round to int for DAW reporting
    return static_cast<int>(std::round(oversampler->getLatencyInSamples()));
}
