/*
  ==============================================================================

    ReverbProcessor.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    Dattorro plate reverb implementation.

  ==============================================================================
*/

#include "ReverbProcessor.h"

void ReverbProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);

    // Pre-delay
    preDelayL.prepare (spec);
    preDelayR.prepare (spec);

    // Dry/wet mixer
    dryWetMixer.prepare (spec);

    // Input diffusion allpasses (scaled from 29761 Hz reference)
    inputDiffusion[0].setSize (scaledDelay (kInputDiff1));
    inputDiffusion[1].setSize (scaledDelay (kInputDiff2));
    inputDiffusion[2].setSize (scaledDelay (kInputDiff3));
    inputDiffusion[3].setSize (scaledDelay (kInputDiff4));

    // Tank half A
    tankDiffusionA1.setSize (scaledDelay (kDecayDiff1A));
    delayA1len = scaledDelay (kDelayA1);
    tankDelayA1.setSize (delayA1len + 64);  // Extra room for modulation excursion
    tankDampA.clear();
    tankDiffusionA2.setSize (scaledDelay (kDecayDiff2A));
    delayA2len = scaledDelay (kDelayA2);
    tankDelayA2.setSize (delayA2len);

    // Tank half B
    tankDiffusionB1.setSize (scaledDelay (kDecayDiff1B));
    delayB1len = scaledDelay (kDelayB1);
    tankDelayB1.setSize (delayB1len + 64);
    tankDampB.clear();
    tankDiffusionB2.setSize (scaledDelay (kDecayDiff2B));
    delayB2len = scaledDelay (kDelayB2);
    tankDelayB2.setSize (delayB2len);

    // Output tap positions (delay-line-only, all within buffer bounds)
    tapA1_266  = scaledDelay (266);   // A1 max: 4453 ✓
    tapA1_2974 = scaledDelay (2974);
    tapA1_2111 = scaledDelay (2111);
    tapA2_1913 = scaledDelay (1913);  // A2 max: 3720 ✓
    tapA2_1990 = scaledDelay (1990);
    tapA2_335  = scaledDelay (335);
    tapB1_353  = scaledDelay (353);   // B1 max: 4217 ✓
    tapB1_3627 = scaledDelay (3627);
    tapB1_1990 = scaledDelay (1990);
    tapB2_187  = scaledDelay (187);   // B2 max: 3163 ✓
    tapB2_1228 = scaledDelay (1228);
    tapB2_2111 = scaledDelay (2111);

    // Cross-feedback state
    tankFeedbackA = 0.0f;
    tankFeedbackB = 0.0f;

    // LFO
    lfoPhase = 0.0f;
    lfoIncrement = modRateHz / currentSampleRate;
}

void ReverbProcessor::reset()
{
    preDelayL.reset();
    preDelayR.reset();
    dryWetMixer.reset();

    for (auto& ap : inputDiffusion)
        ap.clear();

    tankDiffusionA1.clear();
    tankDelayA1.clear();
    tankDampA.clear();
    tankDiffusionA2.clear();
    tankDelayA2.clear();

    tankDiffusionB1.clear();
    tankDelayB1.clear();
    tankDampB.clear();
    tankDiffusionB2.clear();
    tankDelayB2.clear();

    tankFeedbackA = 0.0f;
    tankFeedbackB = 0.0f;

    lfoPhase = 0.0f;
}

void ReverbProcessor::setSize (float size)
{
    // Map 0-1 to decay coefficient 0.0 - 0.98
    decayCoeff = size * 0.98f;

    // Dattorro: decayDiffusion2 = decay^2 * 0.5 + 0.15
    decDiff2Coeff = decayCoeff * decayCoeff * 0.5f + 0.15f;
}

void ReverbProcessor::setDamping (float damp)
{
    // Map 0-1 to one-pole coefficient (higher = more damping = darker)
    dampingCoeff = damp;
    tankDampA.setCoefficient (dampingCoeff);
    tankDampB.setCoefficient (dampingCoeff);
}

void ReverbProcessor::setPredelay (float ms)
{
    preDelaySamples = ms * 0.001f * currentSampleRate;
}

void ReverbProcessor::setMix (float mix)
{
    dryWetMixer.setWetMixProportion (mix);
}

void ReverbProcessor::setModDepth (float depth)
{
    // Map 0-1 to 0-16 samples of modulation excursion
    modDepth = depth * 16.0f;
}

void ReverbProcessor::setModRate (float rateHz)
{
    modRateHz = rateHz;
    lfoIncrement = modRateHz / currentSampleRate;
}

void ReverbProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    dryWetMixer.pushDrySamples (block);

    auto numSamples = block.getNumSamples();
    auto* leftData  = block.getChannelPointer (0);
    auto* rightData = block.getNumChannels() > 1 ? block.getChannelPointer (1) : leftData;

    // Input diffusion coefficients (from Dattorro paper)
    static constexpr float kInDiff1 = 0.75f;
    static constexpr float kInDiff2 = 0.625f;

    // Decay diffusion 1 coefficient (fixed per Dattorro)
    static constexpr float kDecDiff1 = 0.7f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        // ─── Pre-delay ───
        float inputL = leftData[i];
        float inputR = rightData[i];

        if (preDelaySamples > 0.0f)
        {
            preDelayL.pushSample (0, inputL);
            preDelayR.pushSample (0, inputR);
            inputL = preDelayL.popSample (0, preDelaySamples);
            inputR = preDelayR.popSample (0, preDelaySamples);
        }

        // Sum to mono for reverb input
        float input = (inputL + inputR) * 0.5f;

        // ─── Input diffusion (4 series allpasses) ───
        float diffused = inputDiffusion[0].process (input,    kInDiff1);
        diffused       = inputDiffusion[1].process (diffused, kInDiff1);
        diffused       = inputDiffusion[2].process (diffused, kInDiff2);
        diffused       = inputDiffusion[3].process (diffused, kInDiff2);

        // ─── LFO for tank modulation ───
        float lfoA = std::sin (lfoPhase * 6.2831853f) * modDepth;
        float lfoB = std::sin ((lfoPhase + 0.25f) * 6.2831853f) * modDepth;
        lfoPhase += lfoIncrement;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

        // ─── Tank Half A ───
        float tankInputA = diffused + tankFeedbackB * decayCoeff;

        // Decay diffuser 1 (negative coefficient per Dattorro)
        float decDiff1A = tankDiffusionA1.process (tankInputA, -kDecDiff1);

        // Modulated delay 1
        tankDelayA1.write (decDiff1A);
        float delayedA1 = tankDelayA1.readInterpolated (static_cast<float> (delayA1len) + lfoA);

        // Damping + decay
        float dampedA = tankDampA.process (delayedA1) * decayCoeff;

        // Decay diffuser 2 (decay-dependent coefficient per Dattorro)
        float decDiff2A = tankDiffusionA2.process (dampedA, decDiff2Coeff);

        // Delay 2 (precomputed length)
        tankDelayA2.write (decDiff2A);
        tankFeedbackA = tankDelayA2.read (delayA2len);

        // ─── Tank Half B ───
        float tankInputB = diffused + tankFeedbackA * decayCoeff;

        float decDiff1B = tankDiffusionB1.process (tankInputB, -kDecDiff1);

        tankDelayB1.write (decDiff1B);
        float delayedB1 = tankDelayB1.readInterpolated (static_cast<float> (delayB1len) + lfoB);

        float dampedB = tankDampB.process (delayedB1) * decayCoeff;

        float decDiff2B = tankDiffusionB2.process (dampedB, decDiff2Coeff);

        tankDelayB2.write (decDiff2B);
        tankFeedbackB = tankDelayB2.read (delayB2len);

        // ─── Output taps (delay-line-only, all within buffer bounds) ───
        float outL = tankDelayA1.read (tapA1_266)
                   + tankDelayA1.read (tapA1_2974)
                   - tankDelayA2.read (tapA2_1913)
                   + tankDelayA2.read (tapA2_1990)
                   - tankDelayB1.read (tapB1_1990)
                   - tankDelayB2.read (tapB2_187);

        float outR = tankDelayB1.read (tapB1_353)
                   + tankDelayB1.read (tapB1_3627)
                   - tankDelayB2.read (tapB2_1228)
                   + tankDelayB2.read (tapB2_2111)
                   - tankDelayA1.read (tapA1_2111)
                   - tankDelayA2.read (tapA2_335);

        // Scale output (6 taps summed)
        leftData[i]  = outL * 0.4f;
        rightData[i] = outR * 0.4f;
    }

    dryWetMixer.mixWetSamples (block);
}
