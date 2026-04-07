/*
  ==============================================================================

    FormantVoice.cpp
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "FormantVoice.h"

FormantVoice::FormantVoice (int voiceIndex)
    : aspirationNoise (voiceIndex * 31 + 17),
      voiceIdx (voiceIndex)
{
}

void FormantVoice::setAPVTS (juce::AudioProcessorValueTreeState* apvts)
{
    parameters = apvts;

    // Cache all 21 parameter pointers for real-time access
    pVowelX        = apvts->getRawParameterValue ("vowelX");
    pVowelY        = apvts->getRawParameterValue ("vowelY");
    pVowelFocus    = apvts->getRawParameterValue ("vowelFocus");

    pGlottalRd     = apvts->getRawParameterValue ("glottalRd");
    pBreathiness   = apvts->getRawParameterValue ("breathiness");
    pVibratoRate   = apvts->getRawParameterValue ("vibratoRate");
    pVibratoDepth  = apvts->getRawParameterValue ("vibratoDepth");
    pVibratoDelay  = apvts->getRawParameterValue ("vibratoDelay");
    pJitter        = apvts->getRawParameterValue ("jitter");
    pShimmer       = apvts->getRawParameterValue ("shimmer");
    pRdModDepth    = apvts->getRawParameterValue ("rdModDepth");

    pConsonantLevel  = apvts->getRawParameterValue ("consonantLevel");
    pConsonantTone   = apvts->getRawParameterValue ("consonantTone");
    pSibilance       = apvts->getRawParameterValue ("sibilance");
    pAutoConsonant   = apvts->getRawParameterValue ("autoConsonant");
    pConsonantAttack = apvts->getRawParameterValue ("consonantAttack");
    pConsonantHold   = apvts->getRawParameterValue ("consonantHold");
    pConsonantDecay  = apvts->getRawParameterValue ("consonantDecay");

    pAttack  = apvts->getRawParameterValue ("attack");
    pDecay   = apvts->getRawParameterValue ("decay");
    pSustain = apvts->getRawParameterValue ("sustain");
    pRelease = apvts->getRawParameterValue ("release");

    pFormantShift  = apvts->getRawParameterValue ("formantShift");
    pFormantSpread = apvts->getRawParameterValue ("formantSpread");
    pPitchGlide    = apvts->getRawParameterValue ("pitchGlide");

    pOutputGain   = apvts->getRawParameterValue ("outputGain");
    pStereoWidth  = apvts->getRawParameterValue ("stereoWidth");
}

void FormantVoice::setWavetable (const GlottalWavetable* wt)
{
    glottalSource.setWavetable (wt);
}

void FormantVoice::prepare (double sampleRate)
{
    glottalSource.prepare (sampleRate);
    aspirationNoise.prepare (sampleRate);
    filterBank.prepare (sampleRate);
    vibratoLFO.prepare (sampleRate);
    pitchGlide.prepare (sampleRate);
    consonantEngine.prepare (sampleRate, voiceIdx);
    adsr.setSampleRate (sampleRate);
    rdSmoothed.reset (sampleRate, 0.020); // 20ms ramp for Rd modulation
}

void FormantVoice::noteStarted()
{
    voiceActive = true;
    sampleCounter = 0;

    // Reset DSP state for clean note onset
    glottalSource.reset();
    aspirationNoise.reset();
    filterBank.reset();
    vibratoLFO.reset();
    consonantEngine.reset();

    // MPE state reset
    mpeBreathOffset = 0.0f;
    mpeVowelYOffset = 0.0f;

    // Store velocity for consonant burst scaling
    noteVelocity = getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat();

    // Set initial breathiness target
    if (pBreathiness != nullptr)
        aspirationNoise.setBreathiness (pBreathiness->load());

    // Configure ADSR from current parameter values
    if (pAttack != nullptr)
    {
        adsr.setParameters ({
            pAttack->load(),
            pDecay->load(),
            pSustain->load(),
            pRelease->load()
        });
    }
    adsr.noteOn();

    // Vibrato onset delay
    float vibratoDelayMs = pVibratoDelay != nullptr ? pVibratoDelay->load() : 0.0f;
    vibratoLFO.noteOn (vibratoDelayMs);

    // Pitch glide setup
    float f0 = static_cast<float> (getCurrentlyPlayingNote().getFrequencyInHertz());
    float glideMs = pPitchGlide != nullptr ? pPitchGlide->load() : 0.0f;
    pitchGlide.setTime (glideMs);

    if (glideMs > 0.0f && wasActive)
        pitchGlide.setTarget (f0);  // Glide from old pitch
    else
        pitchGlide.snapTo (f0);     // Jump immediately

    wasActive = true;

    // Seed RNG per-voice for uncorrelated jitter/shimmer patterns
    glottalSource.setSeed (static_cast<uint32_t> (voiceIdx * 7919 + 2463534242u));

    // Set initial Rd with modulation applied at note onset
    {
        float baseRd = pGlottalRd != nullptr ? pGlottalRd->load() : 1.0f;
        float modDepth = pRdModDepth != nullptr ? pRdModDepth->load() : 0.5f;

        float midiNote = static_cast<float> (currentlyPlayingNote.initialNote);
        float pitchRdOffset = -0.3f * (midiNote - 60.0f) / 12.0f;
        float velRdOffset = -0.5f * noteVelocity;

        float initRd = juce::jlimit (0.3f, 2.7f,
            baseRd + modDepth * (pitchRdOffset + velRdOffset));
        rdSmoothed.setCurrentAndTargetValue (initRd);
        glottalSource.setRd (initRd);
    }

    // Always trigger consonant envelope + burst at note onset
    consonantEngine.triggerBurst (noteVelocity);

    // Force immediate coefficient update on first sample
    sampleCounter = 0;
}

void FormantVoice::noteStopped (bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        voiceActive = false;
        wasActive = false;
        clearCurrentNote();
    }
}

void FormantVoice::notePressureChanged()
{
    // MPE pressure -> breathiness offset (additive above knob baseline)
    mpeBreathOffset = getCurrentlyPlayingNote().pressure.asUnsignedFloat();
}

void FormantVoice::notePitchbendChanged()
{
    // Pitchbend handled per-sample via getCurrentlyPlayingNote().getFrequencyInHertz()
}

void FormantVoice::noteTimbreChanged()
{
    // MPE timbre/slide -> vowel Y offset (centered at 0.5 unsigned)
    mpeVowelYOffset = getCurrentlyPlayingNote().timbre.asUnsignedFloat() - 0.5f;
}

void FormantVoice::noteKeyStateChanged()
{
    // Sustain/sostenuto handled by MPESynthesiser
}

void FormantVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                     int startSample, int numSamples)
{
    if (! voiceActive)
        return;

    // Update ADSR parameters (block-rate, safe per JUCE docs)
    if (pAttack != nullptr)
    {
        adsr.setParameters ({
            pAttack->load(),
            pDecay->load(),
            pSustain->load(),
            pRelease->load()
        });
    }

    // Breathiness: knob + MPE pressure offset
    float knobBreath = pBreathiness != nullptr ? pBreathiness->load() : 0.1f;
    float effectiveBreath = knobBreath + mpeBreathOffset * (1.0f - knobBreath);
    aspirationNoise.setBreathiness (effectiveBreath);

    // Dynamic Rd modulation: pitch + velocity + expression
    {
        float baseRd = pGlottalRd != nullptr ? pGlottalRd->load() : 1.0f;
        float modDepth = pRdModDepth != nullptr ? pRdModDepth->load() : 0.5f;

        // (1) Pitch tracking: -0.3 Rd per octave above middle C
        float midiNote = static_cast<float> (currentlyPlayingNote.initialNote);
        float pitchRdOffset = -0.3f * (midiNote - 60.0f) / 12.0f;

        // (2) Velocity: higher velocity = lower Rd (more effort)
        float velRdOffset = -0.5f * noteVelocity;

        // (3) Expression: MPE pressure 0-1 -> +/-0.4 Rd offset
        float exprRdOffset = (mpeBreathOffset - 0.5f) * 0.8f;

        float effectiveRd = juce::jlimit (0.3f, 2.7f,
            baseRd + modDepth * (pitchRdOffset + velRdOffset + exprRdOffset));
        rdSmoothed.setTargetValue (effectiveRd);
    }

    // Read jitter/shimmer once per block
    float jitter  = pJitter  != nullptr ? pJitter->load()  : 0.15f;
    float shimmer = pShimmer != nullptr ? pShimmer->load() : 0.1f;
    glottalSource.setJitterShimmer (jitter, shimmer, noteVelocity);

    // Read vibrato params once per block
    float vibratoRate  = pVibratoRate  != nullptr ? pVibratoRate->load()  : 5.0f;
    float vibratoDepth = pVibratoDepth != nullptr ? pVibratoDepth->load() : 0.0f;

    // Read consonant params once per block
    float consonantLevel = pConsonantLevel != nullptr ? pConsonantLevel->load() : 0.0f;
    float consonantTone  = pConsonantTone  != nullptr ? pConsonantTone->load()  : 0.5f;
    float sibilance      = pSibilance      != nullptr ? pSibilance->load()      : 0.0f;
    bool  autoConsonant  = pAutoConsonant  != nullptr && pAutoConsonant->load() >= 0.5f;

    // Update consonant filter coefficients (block-rate)
    consonantEngine.updateCoefficients (consonantTone, sibilance, getSampleRate());

    // When auto is off, override envelope timing with user knobs
    if (! autoConsonant)
    {
        float consAtk   = pConsonantAttack != nullptr ? pConsonantAttack->load() : 20.0f;
        float consHold  = pConsonantHold   != nullptr ? pConsonantHold->load()   : 30.0f;
        float consDecay = pConsonantDecay  != nullptr ? pConsonantDecay->load()  : 40.0f;
        consonantEngine.setManualEnvelope (consAtk, consHold, consDecay, getSampleRate());
    }

    // Stereo width: compute pan gains from MIDI note (block-rate)
    float stereoWidth = pStereoWidth != nullptr ? pStereoWidth->load() : 0.5f;
    float noteNorm = currentlyPlayingNote.initialNote / 127.0f;
    float panPosition = juce::jlimit (-1.0f, 1.0f, (noteNorm - 0.5f) * stereoWidth * 2.0f);
    float panNorm = (panPosition + 1.0f) * 0.5f;
    static constexpr float halfPi = juce::MathConstants<float>::halfPi;
    float panLGain = std::cos (panNorm * halfPi);
    float panRGain = std::sin (panNorm * halfPi);

    auto* outL = outputBuffer.getWritePointer (0, startSample);
    auto* outR = outputBuffer.getNumChannels() > 1
                     ? outputBuffer.getWritePointer (1, startSample)
                     : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // Block-rate formant coefficient update every 32 samples
        if ((sampleCounter % kCoeffUpdateInterval) == 0)
        {
            float vowelX = pVowelX != nullptr ? pVowelX->load() : 0.5f;
            float vowelY = pVowelY != nullptr ? pVowelY->load() : 0.5f;

            // Apply MPE timbre offset to vowelY
            vowelY = juce::jlimit (0.0f, 1.0f, vowelY + mpeVowelYOffset);

            float focus  = pVowelFocus != nullptr ? pVowelFocus->load() : 2.5f;
            float shift  = pFormantShift != nullptr ? pFormantShift->load() : 0.0f;
            float spread = pFormantSpread != nullptr ? pFormantSpread->load() : 1.0f;

            vowelMorpher.compute (vowelX, vowelY, focus,
                                  formantFreqs, formantBWs, formantGains);

            // Dynamic bandwidth variation based on vowel openness and breathiness
            // (1) Openness — F1 as proxy: higher F1 = more open = wider B1
            float opennessFactor = 0.4f * (formantFreqs[0] - 400.0f) / 800.0f;

            formantBWs[0] = juce::jlimit (40.0f, 200.0f,
                formantBWs[0] * (1.0f + opennessFactor));

            // B2-B5: 30% of B1's openness scaling
            float minorOpenness = opennessFactor * 0.3f;
            for (int fi = 1; fi < 5; ++fi)
                formantBWs[fi] *= (1.0f + minorOpenness);

            // (2) Breathiness coupling — wider bandwidths for breathier voice
            float breathBWScale = 1.0f + effectiveBreath * 0.5f;
            for (int fi = 0; fi < 5; ++fi)
                formantBWs[fi] *= breathBWScale;

            filterBank.updateCoefficients (formantFreqs, formantBWs, formantGains,
                                           shift, spread, getSampleRate());
        }

        // --- Per-sample pitch: PitchGlide -> VibratoLFO -> final F0 ---
        float baseF0 = pitchGlide.getNextFrequency();
        float vibCents = vibratoLFO.getNextValue (vibratoRate, vibratoDepth);
        float jitter = vibratoLFO.getJitterOffset();
        float finalF0 = baseF0 * std::pow (2.0f, vibCents / 1200.0f) * (1.0f + jitter);
        glottalSource.setFrequency (finalF0);

        // Per-sample smoothed Rd update (20ms ramp avoids clicks)
        glottalSource.setRd (rdSmoothed.getNextValue());

        // Generate glottal pulse sample
        float glottal = glottalSource.getNextSample();

        // Mix with aspiration noise
        float source = aspirationNoise.process (glottal);

        // Consonant noise (shaped by place/manner filters + dedicated envelope)
        float consonantNoise = consonantEngine.getNextSample (consonantLevel);
        float onsetSuppression = consonantEngine.getOnsetSuppression();

        // During plosive onset, suppress glottal source so noise dominates
        float voiceSource = source * (1.0f - 0.7f * onsetSuppression);

        // ADSR envelope — applied to voiced source only
        float env = adsr.getNextSample();
        float voiceWithEnv = voiceSource * env;

        // Route through formant filters (vocal tract resonance for both)
        float fullSource = voiceWithEnv + consonantNoise;
        float mixed = filterBank.process (fullSource);

        float sample = mixed;

        // Soft-clip to prevent extreme amplitudes from resonant filters
        sample = std::tanh (sample);

        // NaN/Inf guard (belt-and-suspenders after tanh)
        if (! std::isfinite (sample))
        {
            sample = 0.0f;
            filterBank.reset();
            consonantEngine.reset();
        }

        // Stereo width: pan by MIDI note (equal-power)
        outL[i] += sample * panLGain;
        if (outR != nullptr)
            outR[i] += sample * panRGain;

        ++sampleCounter;
    }

    // Check if voice has finished releasing
    if (! adsr.isActive())
    {
        voiceActive = false;
        clearCurrentNote();
    }
}
