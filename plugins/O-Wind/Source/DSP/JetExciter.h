/*
  ==============================================================================

    JetExciter.h
    O-Wind - Jet Excitation Model (Breath + Embouchure + Noise + Vibrato)
    Ouaricon Audio
    Developer: Taylor Brook

    Converts breath pressure into jet velocity via simplified Bernoulli model.
    Generates turbulence noise scaled by jet velocity squared.
    Provides pressure vibrato LFO.
    Manages breath attack/release envelopes.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <cmath>

class JetExciter
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;

        // Prepare Strouhal bandpass noise filter (2nd-order IIR)
        juce::dsp::ProcessSpec spec { sampleRate, 512, 1 };
        noiseBandpass.prepare (spec);
        updateNoiseBandpass (3000.0f);

        // Reset envelope state
        breathEnvelope = 0.0f;
        envelopeTarget = 0.0f;
        envelopeIncrement = 0.0f;

        // Reset vibrato
        vibratoPhase = 0.0f;
    }

    void reset()
    {
        noiseBandpass.reset();
        breathEnvelope = 0.0f;
        envelopeTarget = 0.0f;
        envelopeIncrement = 0.0f;
        vibratoPhase = 0.0f;
        chiffLevel = 0.0f;
    }

    // Called once per block from voice
    void setBreathPressure (float pressure) { breathPressureParam = pressure; }
    void setBreathNoise (float noise)       { breathNoiseParam = noise; }
    void setJetReflection (float refl)      { jetReflectionParam = refl; }
    void setVibratoRate (float rateHz)      { vibratoRate = rateHz; }
    void setVibratoDepth (float depth)      { vibratoDepth = depth; }
    void setJetAmplification (float mu)      { jetAmplification = mu; }
    void setAttackChiff (float chiff)         { attackChiffParam = chiff; }

    // Start note: begin attack envelope ramp + chiff transient
    void startNote (float velocity)
    {
        // Velocity 127 = 5ms attack, velocity 1 = 30ms attack
        float attackMs = juce::jmap (velocity, 0.0f, 1.0f, 30.0f, 5.0f);
        float attackSamples = static_cast<float> (sampleRate * attackMs * 0.001);
        envelopeTarget = 1.0f;
        envelopeIncrement = (envelopeTarget - breathEnvelope)
                            / std::max (1.0f, attackSamples);
        releasing = false;

        // Chiff transient: noise boost during first 20-40ms of attack
        // Boost = 3-6x steady-state noise, scaled by attackChiff * velocity
        // Higher velocity = shorter chiff (20ms), lower = longer (40ms)
        float chiffMs = juce::jmap (velocity, 0.0f, 1.0f, 40.0f, 20.0f);
        float chiffSamples = static_cast<float> (sampleRate * chiffMs * 0.001);

        // Peak boost: 3x at low chiff, 6x at full chiff, scaled by velocity
        float peakBoost = juce::jmap (attackChiffParam, 0.0f, 1.0f, 3.0f, 6.0f);
        chiffLevel = peakBoost * velocity;

        // Exponential decay coefficient: reach ~5% of peak by chiffMs
        // exp(-3/tau * chiffSamples) ≈ 0.05 → tau = chiffSamples / 3
        chiffDecayCoeff = std::exp (-3.0f / std::max (1.0f, chiffSamples));
    }

    // Stop note: begin release envelope ramp
    void stopNote()
    {
        float releaseSamples = static_cast<float> (sampleRate * 0.050);  // 50ms release
        envelopeTarget = 0.0f;
        envelopeIncrement = (envelopeTarget - breathEnvelope)
                            / std::max (1.0f, releaseSamples);
        releasing = true;
    }

    bool isReleasing() const { return releasing; }
    float getEnvelopeLevel() const { return breathEnvelope; }

    // Per-sample processing: returns excitation signal to feed into jet delay
    float processSample (float boreFeedback)
    {
        // Update breath envelope
        if (std::abs (breathEnvelope - envelopeTarget) > 1.0e-6f)
            breathEnvelope += envelopeIncrement;
        else
            breathEnvelope = envelopeTarget;

        breathEnvelope = juce::jlimit (0.0f, 1.0f, breathEnvelope);

        // Apply vibrato LFO to breath pressure
        float vibratoMod = 1.0f;
        if (vibratoDepth > 0.0f && sampleRate > 0.0)
        {
            vibratoMod = 1.0f + vibratoDepth * std::sin (vibratoPhase);
            float phaseInc = static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                                  * vibratoRate / sampleRate);
            vibratoPhase += phaseInc;
            if (vibratoPhase > juce::MathConstants<float>::twoPi)
                vibratoPhase -= juce::MathConstants<float>::twoPi;
        }

        // Breath pressure with envelope and vibrato
        float effectivePressure = breathPressureParam * breathEnvelope * vibratoMod;
        effectivePressure = juce::jlimit (0.0f, 1.0f, effectivePressure);

        // Bernoulli: jet velocity ~ pressure^1.5
        float jetVelocity = std::pow (effectivePressure, 1.5f);

        // Strouhal turbulence noise: bandpass centered on f_s = 0.2 * v / d
        float turbulence = 0.0f;
        if (breathNoiseParam > 0.0f)
        {
            float whiteNoise = noiseRng.nextFloat() * 2.0f - 1.0f;
            float filteredNoise = noiseBandpass.processSample (whiteNoise);
            float noiseGain = breathNoiseParam * jetVelocity * jetVelocity;

            // Chiff transient: multiply noise by decaying boost during attack
            if (chiffLevel > 0.01f)
            {
                noiseGain *= (1.0f + chiffLevel);
                chiffLevel *= chiffDecayCoeff;
            }

            turbulence = filteredNoise * noiseGain;
        }

        // Store jet velocity for Verge (1995) nonlinearity model
        lastJetVelocity = jetVelocity;

        // Jet deflection: bore feedback + turbulence (NO DC from jetVelocity)
        // Breath pressure controls saturation amplitude in the nonlinearity,
        // not DC bias — keeps loop gain independent of breath pressure.
        float excitation = boreFeedback * jetReflectionParam + turbulence;

        // Jet spatial amplification (mu factor) per Verge (1995):
        // jet instability grows exponentially from lip to labium.
        // This is the physical energy source — replaces the old bore feedbackGain hack.
        return excitation * jetAmplification;
    }

    float getLastJetVelocity() const { return lastJetVelocity; }

    void setJetDiameter (float diameterMeters)  { jetDiameter = diameterMeters; }

    // Update Strouhal bandpass: center frequency scales with breath pressure
    // f_s = 0.2 * jetVelocity_physical / jetDiameter
    // jetVelocity_physical ~ pressure * maxJetSpeed (≈40 m/s for concert flute)
    void updateStrouhalBandpass (float breathPressure)
    {
        if (sampleRate <= 0.0 || jetDiameter <= 0.0f) return;

        float jetVelPhysical = breathPressure * maxJetSpeed;
        float strouhalFreq = 0.2f * jetVelPhysical / jetDiameter;
        strouhalFreq = juce::jlimit (500.0f, static_cast<float> (sampleRate * 0.45), strouhalFreq);
        updateNoiseBandpass (strouhalFreq);
    }

private:
    void updateNoiseBandpass (float centerHz)
    {
        if (sampleRate > 0.0)
        {
            float clamped = juce::jlimit (200.0f, static_cast<float> (sampleRate * 0.45), centerHz);
            float q = 1.5f;  // moderate bandwidth for natural turbulence
            *noiseBandpass.coefficients = juce::dsp::IIR::Coefficients<float> (
                juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sampleRate, clamped, q));
        }
    }

    double sampleRate = 44100.0;

    // Parameters (set once per block)
    float breathPressureParam = 0.5f;
    float breathNoiseParam = 0.15f;
    float jetReflectionParam = 0.5f;
    float vibratoRate = 5.0f;
    float vibratoDepth = 0.0f;
    float jetAmplification = 25.0f;  // mu factor: jet spatial amplification (Verge 1995)
    float jetDiameter = 0.010f;      // jet opening diameter in meters
    static constexpr float maxJetSpeed = 40.0f;  // m/s, typical concert flute

    // Envelope state
    float breathEnvelope = 0.0f;
    float envelopeTarget = 0.0f;
    float envelopeIncrement = 0.0f;
    bool releasing = false;

    // Chiff transient state (attack noise burst)
    float attackChiffParam = 0.5f;  // APVTS parameter: 0-1
    float chiffLevel = 0.0f;       // current boost multiplier (decays per sample)
    float chiffDecayCoeff = 0.0f;  // exponential decay coefficient

    // Jet velocity for Verge (1995) model (read by voice for nonlinearity)
    float lastJetVelocity = 0.0f;

    // Vibrato LFO
    float vibratoPhase = 0.0f;

    // Noise
    juce::Random noiseRng;
    juce::dsp::IIR::Filter<float> noiseBandpass;
};
