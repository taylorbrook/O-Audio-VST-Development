/*
  ==============================================================================

    WaveguideString.cpp
    O-Bowed - Bidirectional Digital Waveguide for Bowed String
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "WaveguideString.h"
#include "HyperbolicFriction.h"

void WaveguideString::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;

    // Lowest note ~20Hz + headroom
    int maxDelay = static_cast<int> (sampleRate / 20.0) + 100;

    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32> (maxBlockSize),
        1
    };

    bridgeDelay.prepare (spec);
    bridgeDelay.setMaximumDelayInSamples (maxDelay);

    neckDelay.prepare (spec);
    neckDelay.setMaximumDelayInSamples (maxDelay);

    bridgeLossFilter.prepare (spec);

    filterDirty = true;
}

void WaveguideString::trigger (float frequency)
{
    currentFrequency = frequency;

    // Clean start: clear delay lines to avoid artifacts from previous note
    reset();

    updateDelayLengths();
    updateBridgeFilterCoeffs();
}

void WaveguideString::reset()
{
    bridgeDelay.reset();
    neckDelay.reset();
    bridgeLossFilter.reset();
    energyEstimate = 0.0f;
}

void WaveguideString::updateDelayLengths()
{
    float totalDelay = static_cast<float> (sampleRate) / currentFrequency;

    // Filter group delay compensation: subtract bridge filter group delay
    float pi = juce::MathConstants<float>::pi;
    float filterGroupDelay = static_cast<float> (sampleRate) / (2.0f * pi * brightnessHz);
    float compensatedDelay = totalDelay - filterGroupDelay;

    // Clamp to ensure positive total delay
    compensatedDelay = std::max (4.0f, compensatedDelay);

    // Split at bow position
    float bridgeSamples = compensatedDelay * bowPosition;
    float neckSamples = compensatedDelay * (1.0f - bowPosition);

    // Clamp minimum delay per rail (Thiran needs >= 2 samples)
    bridgeSamples = std::max (2.0f, bridgeSamples);
    neckSamples = std::max (2.0f, neckSamples);

    bridgeDelay.setDelay (bridgeSamples);
    neckDelay.setDelay (neckSamples);
}

void WaveguideString::updateBridgeFilterCoeffs()
{
    // Compute loop gain from INFINITE_SUSTAIN parameter
    float g = 0.990f + 0.010f * infiniteSustain;

    // Compute pole from brightness
    float pi = juce::MathConstants<float>::pi;
    float p = std::exp (-2.0f * pi * brightnessHz / static_cast<float> (sampleRate));

    // Custom one-pole: H(z) = g*(1-p) / (1 - p*z^-1)
    // IIR coefficients: b0 = g*(1-p), b1 = 0, a0 = 1, a1 = -p
    *bridgeLossFilter.coefficients = juce::dsp::IIR::Coefficients<float> (
        g * (1.0f - p), 0.0f, 1.0f, -p);

    filterDirty = false;
}

float WaveguideString::processSample (float v_bow, float F_bow,
                                       const HyperbolicFriction& friction)
{
    // Apply pending filter coefficient update (thread-safe: runs on audio thread only)
    if (filterDirty)
        updateBridgeFilterCoeffs();

    // Step 2: Read incoming waves from delay line ends
    float bridgeReflection = -bridgeLossFilter.processSample (bridgeDelay.popSample (0));
    float nutReflection = -neckDelay.popSample (0);   // sign inversion = hard boundary

    // Step 3: Combine traveling waves at bow point
    float v_string_incoming = bridgeReflection + nutReflection;

    // Step 4: Compute differential velocity
    float v_delta = v_bow - v_string_incoming;

    // Step 5: Evaluate friction -> reflection coefficient
    float rho = friction.computeReflectionCoefficient (v_delta, F_bow);

    // Step 6: Compute injected velocity
    float newVelocity = v_delta * rho;

    // Step 7: Write outgoing waves into both delay lines (cross-feed)
    bridgeDelay.pushSample (0, nutReflection + newVelocity);
    neckDelay.pushSample (0, bridgeReflection + newVelocity);

    // Step 8: Output from bridge end
    float output = nutReflection + newVelocity;

    // Energy tracking for voice cleanup
    energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs (output);

    // Denormal flush
    if (std::abs (output) < 1e-15f)
        output = 0.0f;

    return output;
}

void WaveguideString::setBowPosition (float beta)
{
    if (std::abs (bowPosition - beta) < 1e-6f)
        return;

    bowPosition = beta;
    updateDelayLengths();
}

void WaveguideString::setBrightness (float cutoffHz)
{
    if (std::abs (brightnessHz - cutoffHz) < 0.5f)
        return;

    brightnessHz = cutoffHz;
    filterDirty = true;
    // Brightness affects filter group delay, so update delay lengths too
    updateDelayLengths();
}

void WaveguideString::setInfiniteSustain (float amount)
{
    if (std::abs (infiniteSustain - amount) < 1e-6f)
        return;

    infiniteSustain = amount;
    filterDirty = true;
}

WaveguideString::JunctionState WaveguideString::readJunction (float /*v_bow*/)
{
    // Apply pending filter coefficient update (thread-safe: runs on audio thread only)
    if (filterDirty)
        updateBridgeFilterCoeffs();

    JunctionState state;

    // Step 2: Read incoming waves from delay line ends
    state.bridgeReflection = -bridgeLossFilter.processSample (bridgeDelay.popSample (0));
    state.nutReflection = -neckDelay.popSample (0);   // sign inversion = hard boundary

    // Step 3: Combine traveling waves at bow point
    state.v_string_incoming = state.bridgeReflection + state.nutReflection;

    return state;
}

float WaveguideString::writeJunction (float rho, float v_delta, const JunctionState& state)
{
    // Step 6: Compute injected velocity
    float newVelocity = v_delta * rho;

    // Step 7: Write outgoing waves into both delay lines (cross-feed)
    bridgeDelay.pushSample (0, state.nutReflection + newVelocity);
    neckDelay.pushSample (0, state.bridgeReflection + newVelocity);

    // Step 8: Output from bridge end
    float output = state.nutReflection + newVelocity;

    // Energy tracking for voice cleanup
    energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs (output);

    // Denormal flush
    if (std::abs (output) < 1e-15f)
        output = 0.0f;

    return output;
}

bool WaveguideString::isActive() const noexcept
{
    return energyEstimate > 1e-7f;
}
