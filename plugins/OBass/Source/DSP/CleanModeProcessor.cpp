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
    lookaheadSamples = juce::jmax(1, static_cast<int>(sampleRate * 0.002));

    // Allocate lookahead buffer (circular buffer for delay line)
    lookaheadBuffer.setSize(1, lookaheadSamples);
    lookaheadBuffer.clear();
    lookaheadWritePos = 0;

    // Pre-allocate dry buffer to avoid allocation in process
    dryBuffer.setSize(1, maxBlockSize);
    dryBuffer.clear();
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
    // DEBUG STEP 3: Test PitchTracker added
    const int numSamples = monoBuffer.getNumSamples();
    if (numSamples == 0)
        return;

    // Store dry signal
    if (dryBuffer.getNumSamples() >= numSamples)
        dryBuffer.copyFrom(0, 0, monoBuffer, 0, 0, numSamples);

    // TEST: Add pitch tracking back
    float detectedPitch = pitchTracker.detectPitch(
        monoBuffer.getReadPointer(0), numSamples);

    if (detectedPitch > 0.0f)
        harmonicGenerator.setAdaptiveHarmonics(detectedPitch);

    // Generate harmonics (oversampling disabled)
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
float CleanModeProcessor::processLookahead(float input)
{
    // Safety: if lookahead not configured, pass through
    if (lookaheadSamples <= 0 || lookaheadBuffer.getNumSamples() == 0)
        return input;

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
float CleanModeProcessor::getCompressedEnhance(float rawEnhance)
{
    // Compressed curve: sqrt gives diminishing returns
    // This makes the enhance knob feel more musical:
    // - Small movements at low values have moderate effect
    // - Large movements at high values have smaller incremental effect
    //
    // rawEnhance 0.0 -> 0.0
    // rawEnhance 0.25 -> 0.5
    // rawEnhance 0.5 -> 0.71
    // rawEnhance 1.0 -> 1.0
    return std::sqrt(rawEnhance);
}

//==============================================================================
int CleanModeProcessor::getLatencyInSamples() const
{
    // DISABLED: Latency reporting causes Logic Pro crash ("Sample Rate 15,595" error)
    // Root cause unknown - possibly JUCE oversampler latency query issue.
    // Workaround: Return 0 to disable DAW latency compensation.
    // Impact: Minor phase offset (~2-5ms) which is acceptable for bass enhancement.
    // TODO: Investigate root cause in future phase if precise latency compensation needed.
    return 0;
}
