/*
  ==============================================================================

    ColoredModeProcessor.cpp
    O-Bass - Colored Mode Asymmetric Saturation

    Asymmetric tanh saturation for warm, even-harmonic-rich enhancement.

    === Harmonic Generation Theory ===

    Clean Mode (symmetric saturation):
    - f(x) = tanh(x * drive)
    - Property: f(-x) = -f(x) (odd function)
    - Fourier series contains only odd terms: sin(t), sin(3t), sin(5t)...
    - Result: 3rd, 5th harmonics -> clarity, presence

    Colored Mode (asymmetric saturation):
    - f(x) = tanh((x + bias) * drive) - tanh(bias * drive)
    - Property: f(-x) != -f(x) (neither odd nor even)
    - Fourier series contains even AND odd terms
    - Result: 2nd, 4th harmonics -> warmth, vintage character

    Why DC correction matters:
    - Without: tanh((x + bias) * drive) has DC offset (asymmetric around zero)
    - With:    subtracting tanh(bias * drive) removes the DC component
    - Result: Warm harmonics without bass drift or speaker damage

    Mathematical proof:
    - Input: x = sin(t)
    - Symmetric f(sin(t)) = tanh(sin(t)) -> only odd harmonics in expansion
    - Asymmetric f(sin(t)) = tanh(sin(t) + 0.2) - DC -> even harmonics appear
    - The bias term breaks symmetry, causing even-order distortion products

  ==============================================================================
*/

#include "ColoredModeProcessor.h"
#include <cmath>

//==============================================================================
ColoredModeProcessor::ColoredModeProcessor()
{
    // Default enhance amount maps to moderate drive
    setEnhanceAmount(0.5f);
}

//==============================================================================
void ColoredModeProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    // Colored mode currently has no internal buffers requiring allocation
    // Future: May add oversampling for HighFidelity mode
}

//==============================================================================
void ColoredModeProcessor::reset()
{
    // No internal state to reset currently
    // Future: Reset oversampler state if added
}

//==============================================================================
void ColoredModeProcessor::setMode(Mode mode)
{
    currentMode = mode;
    // Future: Toggle oversampling based on mode
}

//==============================================================================
ColoredModeProcessor::Mode ColoredModeProcessor::getMode() const
{
    return currentMode;
}

//==============================================================================
void ColoredModeProcessor::setEnhanceAmount(float amount)
{
    enhanceAmount = juce::jlimit(0.0f, 1.0f, amount);

    // Map enhance 0-1 to drive 2.0-6.0, scaled by intensityScale
    // drive = (2.0 + enhance * 4.0) * intensityScale
    //
    // At intensityScale 1.0:
    //   enhance 0.0 -> drive 2.0 (noticeable warmth)
    //   enhance 0.5 -> drive 4.0 (strong saturation)
    //   enhance 1.0 -> drive 6.0 (heavy coloration)
    // At intensityScale 1.7 (40Hz crossover):
    //   enhance 0.5 -> drive 6.8 (even stronger)
    drive = (2.0f + enhanceAmount * 4.0f) * intensityScale;
}

//==============================================================================
void ColoredModeProcessor::setIntensityScale(float scale)
{
    intensityScale = juce::jlimit(1.0f, 2.0f, scale);
    // Recalculate drive with new intensity scale
    drive = (2.0f + enhanceAmount * 4.0f) * intensityScale;
}

//==============================================================================
float ColoredModeProcessor::asymmetricTanh(float x, float driveAmount, float biasAmount)
{
    // Apply DC bias and drive
    float biased = x + biasAmount;
    float saturated = std::tanh(driveAmount * biased);

    // DC correction: subtract the DC offset introduced by bias
    // tanh(drive * bias) is the DC value when input is 0
    float dcCorrection = std::tanh(driveAmount * biasAmount);

    return saturated - dcCorrection;
}

//==============================================================================
void ColoredModeProcessor::process(juce::AudioBuffer<float>& monoBuffer)
{
    const int numSamples = monoBuffer.getNumSamples();
    if (numSamples == 0 || enhanceAmount < 0.001f)
        return;

    float* data = monoBuffer.getWritePointer(0);

    // Apply intensity scale for frequency-dependent boost
    // Both drive (already scaled in setEnhanceAmount/setIntensityScale) and
    // wet/dry mix get the intensity scaling
    float scaledEnhance = enhanceAmount * intensityScale;

    for (int i = 0; i < numSamples; ++i)
    {
        float input = data[i];

        // Apply asymmetric saturation (drive is already intensity-scaled)
        float saturated = asymmetricTanh(input, drive, bias);

        // Add 4th harmonic for richer even spectrum (Chebyshev T4)
        // T4(x) = 8x^4 - 8x^2 + 1
        // Normalize input with tanh to keep in [-1, 1] range
        float x = std::tanh(input * 0.5f);
        float x2 = x * x;
        float h4 = (8.0f * x2 * x2 - 8.0f * x2 + 1.0f) * 0.15f * scaledEnhance;

        saturated += h4;

        // Mix dry and wet based on scaled enhance amount
        // At low enhance, mostly dry with subtle saturation
        // At high enhance, mostly saturated signal
        float output = input * (1.0f - scaledEnhance) + saturated * scaledEnhance;

        // Soft limit to prevent clipping
        data[i] = std::tanh(output);
    }
}

//==============================================================================
int ColoredModeProcessor::getLatencyInSamples() const
{
    // Colored mode has no latency (no lookahead or linear-phase filtering)
    return 0;
}
