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

    pConsonantLevel = apvts->getRawParameterValue ("consonantLevel");
    pConsonantTone  = apvts->getRawParameterValue ("consonantTone");
    pSibilance      = apvts->getRawParameterValue ("sibilance");
    pAutoConsonant  = apvts->getRawParameterValue ("autoConsonant");

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

    // Set initial Rd
    if (pGlottalRd != nullptr)
        glottalSource.setRd (pGlottalRd->load());

    // Auto-consonant plosive burst
    bool autoConsonant = pAutoConsonant != nullptr && pAutoConsonant->load() >= 0.5f;
    if (autoConsonant)
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

    // Read Rd once per block
    float rd = pGlottalRd != nullptr ? pGlottalRd->load() : 1.0f;
    glottalSource.setRd (rd);

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

            filterBank.updateCoefficients (formantFreqs, formantBWs, formantGains,
                                           shift, spread, getSampleRate());
        }

        // --- Per-sample pitch: PitchGlide -> VibratoLFO -> final F0 ---
        float baseF0 = pitchGlide.getNextFrequency();
        float vibCents = vibratoLFO.getNextValue (vibratoRate, vibratoDepth);
        float jitter = vibratoLFO.getJitterOffset();
        float finalF0 = baseF0 * std::pow (2.0f, vibCents / 1200.0f) * (1.0f + jitter);
        glottalSource.setFrequency (finalF0);

        // Generate glottal pulse sample
        float glottal = glottalSource.getNextSample();

        // Mix with aspiration noise
        float source = aspirationNoise.process (glottal);

        // Consonant noise (shaped by tone/sibilance filters)
        float consonantNoise = consonantEngine.getNextSample (consonantLevel, autoConsonant);
        float onsetSuppression = consonantEngine.getOnsetSuppression();

        // During plosive onset, suppress glottal source so noise dominates
        float voiceSource = source * (1.0f - 0.7f * onsetSuppression);

        // Route consonant noise through formant filters alongside voice source
        float fullSource = voiceSource + consonantNoise;

        // Formant filtering shapes both voice and consonant through the vocal tract
        float mixed = filterBank.process (fullSource);

        // Apply ADSR envelope
        float env = adsr.getNextSample();
        float sample = mixed * env;

        // Final NaN/Inf guard
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
