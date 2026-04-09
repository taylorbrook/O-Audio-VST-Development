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
    tankDelayA2.setSize (scaledDelay (kDelayA2));

    // Tank half B
    tankDiffusionB1.setSize (scaledDelay (kDecayDiff1B));
    delayB1len = scaledDelay (kDelayB1);
    tankDelayB1.setSize (delayB1len + 64);
    tankDampB.clear();
    tankDiffusionB2.setSize (scaledDelay (kDecayDiff2B));
    tankDelayB2.setSize (scaledDelay (kDelayB2));

    // Output tap positions (from Dattorro paper, scaled)
    tapA1pos = scaledDelay (266);
    tapA2pos = scaledDelay (2974);
    tapA3pos = scaledDelay (1913);
    tapB1pos = scaledDelay (353);
    tapB2pos = scaledDelay (3627);
    tapB3pos = scaledDelay (1228);
    tapA1neg = scaledDelay (1990);
    tapB1neg = scaledDelay (187);

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

    lfoPhase = 0.0f;
}

void ReverbProcessor::setSize (float size)
{
    // Map 0-1 to decay coefficient 0.0 - 0.98
    decayCoeff = size * 0.98f;
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

    // Decay diffusion coefficients
    static constexpr float kDecDiff1 = 0.7f;

    // Cross-feedback state from previous sample
    float tankFeedbackA = 0.0f;
    float tankFeedbackB = 0.0f;

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
        float lfoB = std::sin ((lfoPhase + 0.25f) * 6.2831853f) * modDepth;  // 90-degree offset
        lfoPhase += lfoIncrement;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

        // ─── Tank Half A ───
        float tankInputA = diffused + tankFeedbackB * decayCoeff;

        // Decay diffuser 1
        float decDiff1A = tankDiffusionA1.process (tankInputA, -kDecDiff1);

        // Modulated delay 1
        tankDelayA1.write (decDiff1A);
        float delayedA1 = tankDelayA1.readInterpolated (static_cast<float> (delayA1len) + lfoA);

        // Damping
        float dampedA = tankDampA.process (delayedA1);

        // Apply decay
        dampedA *= decayCoeff;

        // Decay diffuser 2
        float decDiff2A = tankDiffusionA2.process (dampedA, kDecDiff1);

        // Delay 2
        tankDelayA2.write (decDiff2A);
        float delayedA2 = tankDelayA2.read (scaledDelay (kDelayA2));

        tankFeedbackA = delayedA2;

        // ─── Tank Half B ───
        float tankInputB = diffused + tankFeedbackA * decayCoeff;

        float decDiff1B = tankDiffusionB1.process (tankInputB, -kDecDiff1);

        tankDelayB1.write (decDiff1B);
        float delayedB1 = tankDelayB1.readInterpolated (static_cast<float> (delayB1len) + lfoB);

        float dampedB = tankDampB.process (delayedB1);
        dampedB *= decayCoeff;

        float decDiff2B = tankDiffusionB2.process (dampedB, kDecDiff1);

        tankDelayB2.write (decDiff2B);
        float delayedB2 = tankDelayB2.read (scaledDelay (kDelayB2));

        tankFeedbackB = delayedB2;

        // ─── Output taps (multi-tap stereo from both tank halves) ───
        // Taps read at fixed offsets behind each delay's write head

        float outL = tankDelayA1.read (tapA1pos)
                   + tankDelayA1.read (tapA2pos)
                   - tankDelayB2.read (tapA3pos)
                   + tankDelayA2.read (tapA1neg)
                   - tankDelayB1.read (tapB1neg);

        float outR = tankDelayB1.read (tapB1pos)
                   + tankDelayB1.read (tapB2pos)
                   - tankDelayA2.read (tapB3pos)
                   + tankDelayB2.read (tapA1neg)
                   - tankDelayA1.read (tapA1neg);

        // Scale output
        leftData[i]  = outL * 0.6f;
        rightData[i] = outR * 0.6f;
    }

    dryWetMixer.mixWetSamples (block);
}
