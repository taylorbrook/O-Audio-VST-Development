/*
  ==============================================================================

    MarimbaVoice.cpp
    Phase 2.2: Modal synthesis implementation

  ==============================================================================
*/

#include "MarimbaVoice.h"
#include "TuningEngine.h"
#include <cmath>

MarimbaVoice::MarimbaVoice()
{
    // Initialize modal modes
    for (auto& mode : modes)
    {
        mode.reset();
    }

    exciter.reset();
}

bool MarimbaVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<MarimbaSound*>(sound) != nullptr;
}

void MarimbaVoice::startNote(int midiNoteNumber, float velocityValue,
                             juce::SynthesiserSound* /*sound*/,
                             int /*currentPitchWheelPosition*/)
{
    // Store velocity for amplitude scaling
    velocity = velocityValue;

    // Calculate base frequency (12-TET)
    float baseFreq = static_cast<float>(noteToFrequency(midiNoteNumber));

    // Calculate modal coefficients (done ONCE per note, not per sample)
    calculateModalCoefficients(baseFreq);

    // Reset all modal state
    for (auto& mode : modes)
    {
        mode.reset();
    }

    // Configure mallet exciter based on MALLET_HARDNESS and velocity
    float scaledVelocity = applyVelocityCurve(velocityValue);

    // Exciter duration: 5ms (soft) to 20ms (hard)
    float durationMS = 5.0f + malletHardness * 15.0f;
    exciter.samplesRemaining = static_cast<int>(durationMS * sampleRate / 1000.0f);

    // Exciter filter coefficient: lower = darker (soft), higher = brighter (hard)
    // Maps to effective cutoff: ~2kHz (soft) to ~8kHz (hard)
    exciter.filterCoefficient = 0.1f + malletHardness * 0.5f;

    // Exciter amplitude scaled by velocity
    exciter.amplitude = scaledVelocity;

    // Reset filter state
    exciter.filterState = 0.0f;

    // Mark voice as active
    isActive = true;

    // Calculate approximate release time (when modes decay below audible threshold)
    // Use fundamental mode decay time as reference
    float maxDecayTime = getDecayTime(0, resonance);
    samplesUntilRelease = static_cast<int>(maxDecayTime * sampleRate * 1.5f); // 1.5x for safety
}

void MarimbaVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (!allowTailOff)
    {
        // Immediate cutoff - clear voice
        clearCurrentNote();
        isActive = false;
        for (auto& mode : modes)
        {
            mode.reset();
        }
        exciter.reset();
    }
    // If allowTailOff, let modal resonators naturally decay
    // (no explicit noteOff needed - modes decay on their own)
}

void MarimbaVoice::pitchWheelMoved(int /*newPitchWheelValue*/)
{
    // Pitch wheel not implemented in Phase 2.2 (can be added later)
}

void MarimbaVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{
    // CC not implemented in Phase 2.2
}

void MarimbaVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                   int startSample, int numSamples)
{
    // Check if voice is active
    if (!isActive)
        return;

    juce::ScopedNoDenormals noDenormals;

    // Render each sample
    while (--numSamples >= 0)
    {
        // Generate exciter sample (filtered noise burst)
        float excitation = exciter.nextSample();

        // Feed excitation through all modal resonators and sum
        float modalSum = 0.0f;

        for (auto& mode : modes)
        {
            // Each mode processes the excitation independently
            float modeOutput = mode.processSample(excitation);
            modalSum += modeOutput * mode.amplitude;
        }

        // Apply output gain
        float finalSample = modalSum * outputGain;

        // Write to all output channels
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample(channel, startSample, finalSample);
        }

        ++startSample;

        // Decrement release counter
        if (samplesUntilRelease > 0)
        {
            --samplesUntilRelease;
        }
        else
        {
            // Voice has decayed - can be stolen
            clearCurrentNote();
            isActive = false;
            break;
        }
    }
}

void MarimbaVoice::setOutputGain(float gainDB)
{
    // Convert dB to linear gain
    outputGain = juce::Decibels::decibelsToGain(gainDB);
}

void MarimbaVoice::setVelocityCurve(float curve)
{
    velocityCurve = juce::jlimit(0.0f, 1.0f, curve);
}

void MarimbaVoice::setMalletHardness(float hardness)
{
    malletHardness = juce::jlimit(0.0f, 1.0f, hardness);
}

void MarimbaVoice::setBarMaterial(float material)
{
    barMaterial = juce::jlimit(0.0f, 1.0f, material);
}

void MarimbaVoice::setResonance(float resonanceParam)
{
    resonance = juce::jlimit(0.0f, 1.0f, resonanceParam);
}

void MarimbaVoice::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
}

double MarimbaVoice::noteToFrequency(int midiNote) const
{
    // Phase 2.3: Use tuning engine if available
    if (tuningEngine)
        return tuningEngine->getFrequency(midiNote);

    // Fallback to 12-TET if no tuning engine
    return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
}

float MarimbaVoice::applyVelocityCurve(float rawVelocity) const
{
    // VEL_CURVE parameter: 0.0 = linear, 1.0 = exponential
    // Formula: amplitude = velocity^(1 + curve)
    float exponent = 1.0f + velocityCurve;
    return std::pow(rawVelocity, exponent);
}

float MarimbaVoice::getModeAmplitude(int modeIndex, float material) const
{
    // BAR_MATERIAL: 0.0 = dark rosewood (fundamental emphasis)
    //               1.0 = bright synthetic (high modes emphasis)

    if (modeIndex == 0)
        return 1.0f; // Fundamental always present

    // Natural amplitude falloff (higher modes quieter)
    float baseAmp = 1.0f / static_cast<float>(modeIndex + 1);

    // Material boost: brighter materials have stronger high modes
    float materialBoost = material * 0.5f;
    float modeRatio = static_cast<float>(modeIndex) / static_cast<float>(NUM_MODES);

    return baseAmp * (1.0f + materialBoost * modeRatio);
}

float MarimbaVoice::getDecayTime(int modeIndex, float resonanceParam) const
{
    // RESONANCE: 0.0 = 0.5s decay (short)
    //            1.0 = 5.0s decay (long)

    // Base decay time from resonance parameter
    float baseDecay = 0.5f + resonanceParam * 4.5f; // 0.5 to 5.0 seconds

    // Higher modes decay faster (physical characteristic of marimba bars)
    float modeFactor = 1.0f / (1.0f + static_cast<float>(modeIndex) * 0.3f);

    return baseDecay * modeFactor;
}

void MarimbaVoice::calculateModalCoefficients(float baseFreq)
{
    // Calculate biquad coefficients for each modal resonator
    // This is called ONCE per note in startNote(), not in the audio loop

    for (int i = 0; i < NUM_MODES; ++i)
    {
        // Calculate mode frequency from base frequency and ratio
        float modeFreq = baseFreq * MODE_RATIOS[i];

        // Get decay time for this mode
        float decayTime = getDecayTime(i, resonance);

        // Get amplitude for this mode
        modes[i].amplitude = getModeAmplitude(i, barMaterial);

        // Calculate biquad coefficients
        // θ = 2π * f / fs (normalized frequency)
        float theta = juce::MathConstants<float>::twoPi * modeFreq / static_cast<float>(sampleRate);

        // r = e^(-1 / (decay_time * sample_rate)) (pole radius - controls decay)
        float r = std::exp(-1.0f / (decayTime * static_cast<float>(sampleRate)));

        // Clamp r to prevent instability (max 0.9999)
        r = juce::jlimit(0.0f, 0.9999f, r);

        // g = amplitude * (1 - r) (gain compensation)
        float g = modes[i].amplitude * (1.0f - r);

        // Biquad coefficients for resonator
        modes[i].b0 = g;
        modes[i].a1 = 2.0f * r * std::cos(theta);
        modes[i].a2 = -(r * r);
    }
}
