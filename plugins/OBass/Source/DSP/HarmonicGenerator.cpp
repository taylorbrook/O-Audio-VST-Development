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

    // Create dual oversamplers (mono, 4x oversampling)
    // IIR: filterHalfBandPolyphaseIIR - minimal latency, some phase distortion
    oversamplerIIR = std::make_unique<juce::dsp::Oversampling<float>>(
        1,  // numChannels (mono bass processing)
        2,  // factor (2^2 = 4x oversampling)
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,  // isMaxQuality
        true   // useIntegerLatency (for easier DAW compensation)
    );

    // FIR: filterHalfBandFIREquiripple - linear phase, more latency
    oversamplerFIR = std::make_unique<juce::dsp::Oversampling<float>>(
        1, 2,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true, true
    );

    // Initialize both oversamplers (both always prepared for RT-safe switching)
    oversamplerIIR->initProcessing(static_cast<size_t>(blockSize));
    oversamplerFIR->initProcessing(static_cast<size_t>(blockSize));

    // Output bandpass filter: 60-400Hz (two IIR filters in series)
    // Highpass at 60Hz removes sub-harmonics below useful range
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 60.0f, 0.707f);
    outputBandpassLow.coefficients = hpCoeffs;

    // Lowpass at 400Hz removes harmonics above psychoacoustic range
    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 400.0f, 0.707f);
    outputBandpassHigh.coefficients = lpCoeffs;

    // Prepare filters
    juce::dsp::ProcessSpec monoSpec { sampleRate, static_cast<juce::uint32>(blockSize), 1 };
    outputBandpassLow.prepare(monoSpec);
    outputBandpassHigh.prepare(monoSpec);
}

//==============================================================================
void HarmonicGenerator::reset()
{
    if (oversamplerIIR)
        oversamplerIIR->reset();
    if (oversamplerFIR)
        oversamplerFIR->reset();

    outputBandpassLow.reset();
    outputBandpassHigh.reset();
}

//==============================================================================
void HarmonicGenerator::setMode(Mode newMode)
{
    // RT-SAFE: Just flip the atomic flag
    // Both oversamplers are always prepared and ready
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

    // Get active oversampler based on mode (atomic read)
    auto* oversampler = getActiveOversampler();

    if (oversampler == nullptr)
        return;

    // Create audio block for oversampler
    juce::dsp::AudioBlock<float> inputBlock(monoBuffer);

    // Upsample to 4x rate
    auto oversampledBlock = oversampler->processSamplesUp(inputBlock);

    // Process at oversampled rate
    processOversampled(oversampledBlock.getChannelPointer(0),
                       static_cast<int>(oversampledBlock.getNumSamples()));

    // Downsample back to original rate
    oversampler->processSamplesDown(inputBlock);

    // Apply output bandpass filter (60-400Hz)
    // Process sample-by-sample for mono buffer
    float* data = monoBuffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        // Highpass at 60Hz then lowpass at 400Hz
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

        // Soft clip to normalize input to [-1, 1] range
        // Use tanh for smooth saturation without hard clipping
        x = std::tanh(x);

        // Apply Chebyshev polynomials based on active harmonic count
        // Note: We generate harmonics only, not the fundamental
        float output = 0.0f;

        // Always use at least 2nd harmonic (H2)
        output += harmonicWeights[0] * T2(x);

        // Add higher harmonics based on activeHarmonicCount
        if (activeHarmonicCount >= 3)
            output += harmonicWeights[1] * T3(x);

        if (activeHarmonicCount >= 4)
            output += harmonicWeights[2] * T4(x);

        if (activeHarmonicCount >= 5)
            output += harmonicWeights[3] * T5(x);

        // Scale output to reasonable level (sum of weights can exceed 1.0)
        // Normalize by active weight sum to maintain consistent level
        float weightSum = harmonicWeights[0];
        if (activeHarmonicCount >= 3) weightSum += harmonicWeights[1];
        if (activeHarmonicCount >= 4) weightSum += harmonicWeights[2];
        if (activeHarmonicCount >= 5) weightSum += harmonicWeights[3];

        if (weightSum > 0.0f)
            output /= weightSum;

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

    if (oversampler == nullptr)
        return 0;

    return static_cast<int>(oversampler->getLatencyInSamples());
}
