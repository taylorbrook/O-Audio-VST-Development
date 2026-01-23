/*
  ==============================================================================

    CleanModeProcessor.cpp
    OBass - Psychoacoustic Bass Enhancement Orchestrator

    Coordinates the complete enhancement pipeline:
    - Pitch tracking for adaptive harmonic generation
    - Chebyshev waveshaping with 4x oversampling
    - Transient ducking via dual envelope followers
    - Spectral-aware blending for crossover integration

  ==============================================================================
*/

#include "CleanModeProcessor.h"
#include <cmath>

//==============================================================================
CleanModeProcessor::CleanModeProcessor()
{
    // Configure envelope followers for transient detection
    // Fast envelope: 0.5ms attack, 20ms release (tracks transients)
    fastEnvelope.setAttackMs(0.5f);
    fastEnvelope.setReleaseMs(20.0f);

    // Slow envelope: 5ms attack, 100ms release (tracks average level)
    slowEnvelope.setAttackMs(5.0f);
    slowEnvelope.setReleaseMs(100.0f);
}

//==============================================================================
void CleanModeProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    maxBlockSize = static_cast<int>(spec.maximumBlockSize);

    // Prepare components
    pitchTracker.prepare(sampleRate, maxBlockSize);
    harmonicGenerator.prepare(spec);
    fastEnvelope.prepare(sampleRate);
    slowEnvelope.prepare(sampleRate);

    // Calculate lookahead: ~2ms for transient detection in High Fidelity mode
    // At 44.1kHz: 0.002 * 44100 = ~88 samples
    lookaheadSamples = static_cast<int>(sampleRate * 0.002);

    // Allocate lookahead buffer (circular buffer for delay line)
    lookaheadBuffer.setSize(1, lookaheadSamples);
    lookaheadBuffer.clear();
    lookaheadWritePos = 0;
}

//==============================================================================
void CleanModeProcessor::reset()
{
    pitchTracker.reset();
    harmonicGenerator.reset();
    fastEnvelope.reset();
    slowEnvelope.reset();

    lookaheadBuffer.clear();
    lookaheadWritePos = 0;
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
void CleanModeProcessor::process(juce::AudioBuffer<float>& monoBuffer)
{
    const int numSamples = monoBuffer.getNumSamples();

    if (numSamples == 0)
        return;

    const float* inputData = monoBuffer.getReadPointer(0);

    // 1. Detect pitch for adaptive harmonics
    float pitch = pitchTracker.detectPitch(inputData, numSamples);
    if (pitch > 0.0f)
        harmonicGenerator.setAdaptiveHarmonics(pitch);

    // 2. Store original for dry/wet mix
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(monoBuffer);

    // 3. Generate harmonics (with 4x oversampling inside)
    harmonicGenerator.process(monoBuffer);

    // 4. Sample-by-sample transient ducking and blending
    const float* dry = dryBuffer.getReadPointer(0);
    float* wet = monoBuffer.getWritePointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        // Envelope tracking on dry signal
        // In High Fidelity mode, use lookahead delay for envelope detection
        float inputSample = (currentMode == Mode::HighFidelity)
            ? processLookahead(dry[i])
            : dry[i];

        float fastEnv = fastEnvelope.process(inputSample);
        float slowEnv = slowEnvelope.process(inputSample);

        // Transient ducking: reduce harmonics on attacks
        float duckGain = calculateTransientDuckGain(fastEnv, slowEnv);

        // Spectral-aware blend: reduce harmonics when high band is loud
        float spectralBlend = calculateSpectralBlend();

        // Final mix: dry + (wet * duck * spectral * enhance)
        float wetAmount = enhanceAmount * duckGain * spectralBlend;
        wet[i] = dry[i] + wet[i] * wetAmount;
    }
}

//==============================================================================
float CleanModeProcessor::processLookahead(float input)
{
    // Circular buffer delay line for lookahead
    // Write current sample, read delayed sample
    float* bufferData = lookaheadBuffer.getWritePointer(0);

    // Read delayed sample (what was written lookaheadSamples ago)
    float delayed = bufferData[lookaheadWritePos];

    // Write current sample
    bufferData[lookaheadWritePos] = input;

    // Advance write position
    lookaheadWritePos = (lookaheadWritePos + 1) % lookaheadSamples;

    return delayed;
}

//==============================================================================
float CleanModeProcessor::calculateTransientDuckGain(float fastEnv, float slowEnv)
{
    // Transient detection: fast/slow ratio
    // Ratio > 1 indicates transient (fast envelope jumped ahead of slow)
    // Ratio = 1 indicates steady state
    // Ratio < 1 indicates decay

    // Avoid division by zero
    if (slowEnv < 1e-10f)
        return 1.0f;

    float ratio = fastEnv / slowEnv;

    // Transient threshold: duck when fast envelope is 2x the slow average
    static constexpr float kTransientThreshold = 2.0f;

    // Minimum gain on transients (30% = preserve some harmonics)
    static constexpr float kMinDuckGain = 0.3f;

    if (ratio <= 1.0f)
    {
        // No transient, full harmonics
        return 1.0f;
    }
    else if (ratio >= kTransientThreshold)
    {
        // Strong transient, maximum ducking
        return kMinDuckGain;
    }
    else
    {
        // Interpolate between 1.0 and kMinDuckGain
        // ratio 1.0 -> gain 1.0
        // ratio 2.0 -> gain 0.3
        float t = (ratio - 1.0f) / (kTransientThreshold - 1.0f);
        return 1.0f - t * (1.0f - kMinDuckGain);
    }
}

//==============================================================================
float CleanModeProcessor::calculateSpectralBlend()
{
    // Reduce harmonics when high band energy is high
    // This prevents harmonic buildup at crossover frequency
    //
    // highBandEnergy 0.0 -> full harmonics (1.0)
    // highBandEnergy 1.0 -> reduced harmonics (0.5)
    //
    // Clamp to [0.3, 1.0] to always allow some enhancement

    float blend = 1.0f - (highBandEnergy * 0.5f);
    return juce::jlimit(0.3f, 1.0f, blend);
}

//==============================================================================
int CleanModeProcessor::getLatencyInSamples() const
{
    int harmonicLatency = harmonicGenerator.getLatencyInSamples();

    if (currentMode == Mode::HighFidelity)
    {
        // High Fidelity mode adds lookahead latency
        return harmonicLatency + lookaheadSamples;
    }
    else
    {
        // Low Latency mode: only harmonic generator latency
        return harmonicLatency;
    }
}
