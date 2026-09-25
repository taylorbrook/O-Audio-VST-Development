/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    FormantVoice.cpp
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "FormantVoice.h"
#include "TuningEngine.h"

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
    pSpectralTilt  = apvts->getRawParameterValue ("spectralTilt");

    pConsonantLevel  = apvts->getRawParameterValue ("consonantLevel");
    pConsonantTone   = apvts->getRawParameterValue ("consonantTone");
    pSibilance       = apvts->getRawParameterValue ("sibilance");
    pConsonantVoicing = apvts->getRawParameterValue ("consonantVoicing");
    pAutoConsonant   = apvts->getRawParameterValue ("autoConsonant");
    pConsonantAttack = apvts->getRawParameterValue ("consonantAttack");
    pConsonantHold   = apvts->getRawParameterValue ("consonantHold");
    pConsonantDecay  = apvts->getRawParameterValue ("consonantDecay");
    pConsonantVOT    = apvts->getRawParameterValue ("consonantVOT");
    pConsonantTransition = apvts->getRawParameterValue ("consonantTransition");

    pAttack  = apvts->getRawParameterValue ("attack");
    pDecay   = apvts->getRawParameterValue ("decay");
    pSustain = apvts->getRawParameterValue ("sustain");
    pRelease = apvts->getRawParameterValue ("release");

    pFormantTopology = apvts->getRawParameterValue ("formantTopology");
    pFormantShift  = apvts->getRawParameterValue ("formantShift");
    pFormantSpread = apvts->getRawParameterValue ("formantSpread");
    pPitchGlide    = apvts->getRawParameterValue ("pitchGlide");
    pTransitionTime = apvts->getRawParameterValue ("transitionTime");
    pSourceFilterCoupling = apvts->getRawParameterValue ("sourceFilterCoupling");
    pSingersFormant = apvts->getRawParameterValue ("singersFormant");

    pNasalCoupling = apvts->getRawParameterValue ("nasalCoupling");
    pNasalPlace    = apvts->getRawParameterValue ("nasalPlace");

    pLyricsEnabled = apvts->getRawParameterValue ("lyricsEnabled");

    pPitchBendRange = apvts->getRawParameterValue ("tuning_pitchBendRange"); // WR-04

    pOutputGain   = apvts->getRawParameterValue ("outputGain");
    pStereoWidth  = apvts->getRawParameterValue ("stereoWidth");
}

void FormantVoice::setWavetable (const GlottalWavetable* wt)
{
    glottalSource.setWavetable (wt);
}

void FormantVoice::setTuningEngine (TuningEngine* te)
{
    tuningEnginePtr = te;
}

void FormantVoice::setLyricsEngine (LyricsEngine* le)
{
    lyricsEnginePtr = le;
}

void FormantVoice::prepare (double sampleRate)
{
    glottalSource.prepare (sampleRate);
    aspirationNoise.prepare (sampleRate);
    filterBank.prepare (sampleRate);
    cascadeBank.prepare (sampleRate);
    nasalPoleZero.prepare (sampleRate);
    vibratoLFO.prepare (sampleRate);
    pitchGlide.prepare (sampleRate);
    consonantEngine.prepare (sampleRate, voiceIdx);
    fricationBank.prepare (sampleRate);
    adsr.setSampleRate (sampleRate);
    declickCoeff = static_cast<float> (std::exp (-1.0 / (0.003 * sampleRate))); // WR-03: 3 ms tau
    declickL = declickR = lastOutL = lastOutR = 0.0f;
    rdSmoothed.reset (sampleRate, 0.020); // 20ms ramp for Rd modulation
    sourceFilterGain.reset (sampleRate, 0.010); // 10ms ramp for coupling gain
    sourceFilterGain.setCurrentAndTargetValue (1.0f);

    // Locus transition window (~50ms total, τ=15ms) — Kewley-Port 1982
    consonantTransitionMaxSamples = static_cast<int> (0.050 * sampleRate);
    consonantTauSamples = static_cast<float> (0.015 * sampleRate);
    consonantTransitionActive = false;
    consonantTransitionSamples = 0;
}

// F2 locus by place (Delattre-Liberman-Cooper 1955; Kewley-Port 1982)
// Labial 720 / Alveolar 1800 / Palatal 2200 / Velar back-V pinch → 1200
static float computeF2Locus (float place) noexcept
{
    if (place < 0.33f)
        return 720.0f + (place / 0.33f) * (1800.0f - 720.0f);
    if (place < 0.67f)
        return 1800.0f + ((place - 0.33f) / 0.34f) * (2200.0f - 1800.0f);
    return 2200.0f + ((place - 0.67f) / 0.33f) * (1200.0f - 2200.0f);
}

// F3 locus: Labial 2000 / Alveolar 2700 / Palatal 3000 / Velar 2200
static float computeF3Locus (float place) noexcept
{
    if (place < 0.33f)
        return 2000.0f + (place / 0.33f) * (2700.0f - 2000.0f);
    if (place < 0.67f)
        return 2700.0f + ((place - 0.33f) / 0.34f) * (3000.0f - 2700.0f);
    return 3000.0f + ((place - 0.67f) / 0.33f) * (2200.0f - 3000.0f);
}

void FormantVoice::noteStarted()
{
    // WR-07: a key the loaded .kbm leaves unmapped ('x', or outside its
    // first..last range) is silent per the Scala spec — the tuning table holds
    // 0 Hz for it. Release the voice without sounding.
    if (tuningEnginePtr != nullptr
        && ! tuningEnginePtr->isNoteMapped (currentlyPlayingNote.initialNote))
    {
        declickL = declickR = lastOutL = lastOutR = 0.0f;
        adsr.reset();
        voiceActive = false;
        clearCurrentNote();
        return;
    }

    // WR-03: voice stealing calls noteStarted() on a voice that is still
    // sounding (MPESynthesiser does not stop it first). Everything below
    // resets, so the old note would drop to 0 in one sample — hand its last
    // output to the declick tail instead.
    if (voiceActive)
        beginDeclickTail();

    voiceActive = true;
    sampleCounter = 0;
    releaseSampleCount = -1;

    // Reset DSP state for clean note onset
    glottalSource.reset();
    aspirationNoise.reset();
    filterBank.reset();
    cascadeBank.reset();
    nasalPoleZero.reset();
    vibratoLFO.reset();
    consonantEngine.reset();
    fricationBank.reset();

    // Snap formant SmoothedValues to current targets for click-free onset.
    // fricationBank.snapToTargets() is deferred to after the new syllable's
    // place is set (see consonant priming block below) so it doesn't snap to
    // the previous note's amplitudes.
    filterBank.snapToTargets();
    cascadeBank.snapToTargets();
    nasalPoleZero.snapToTargets();

    // MPE state reset
    mpeBreathOffset = 0.0f;
    mpeVowelYOffset = 0.0f;
    spectralTiltPrev = 0.0f;
    sourceFilterGain.setCurrentAndTargetValue (1.0f);
    sourceFilterJitterBoost = 0.0f;

    // Store velocity for consonant burst scaling
    noteVelocity = getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat();

    // Set initial breathiness target
    if (pBreathiness != nullptr)
        aspirationNoise.setBreathiness (pBreathiness->load());

    // Configure ADSR from current parameter values
    updateAdsrParameters (true);
    adsr.noteOn();

    // Vibrato onset delay
    float vibratoDelayMs = pVibratoDelay != nullptr ? pVibratoDelay->load() : 0.0f;
    vibratoLFO.noteOn (vibratoDelayMs);

    // Pitch glide setup — use tuning engine if available, else fall back to 12-TET
    int midiNote = currentlyPlayingNote.initialNote;
    tunedF0 = tuningEnginePtr != nullptr
        ? static_cast<float> (tuningEnginePtr->getFrequency (midiNote))
        : static_cast<float> (getCurrentlyPlayingNote().getFrequencyInHertz());
    float f0 = tunedF0;

    // VST3 Note Expression tuning delta (Dorico microtonal).
    // Apply BEFORE pitchGlide so the glottal source samples the correct fundamental
    // from sample 0 (Pattern 2 — no attack zipper). tunedF0 is referenced downstream
    // at lines 488, 616 (renderNextBlock) for spectral tilt and source-filter coupling
    // — all consumers see the tuned value. Helper consumes the slot via exchange(0.0)
    // so retriggered notes at the same pitch in a later block don't inherit a stale
    // offset. Cast through double at helper boundary (tunedF0 is float).
    if (pendingTuningSource != nullptr)
    {
        tunedF0 = static_cast<float> (Ouaricon::NoteExpression::applyPendingTuning (
            *pendingTuningSource, midiNote, static_cast<double> (tunedF0)));
        f0 = tunedF0;  // re-read after NE composition
    }

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

        // IN-01: renamed from midiNote to avoid shadowing the outer int midiNote
        // declared at note-on above; both hold the same note number.
        float midiNoteF = static_cast<float> (currentlyPlayingNote.initialNote);
        float pitchRdOffset = -0.3f * (midiNoteF - 60.0f) / 12.0f;
        float velRdOffset = -0.5f * noteVelocity;

        float initRd = juce::jlimit (0.3f, 2.7f,
            baseRd + modDepth * (pitchRdOffset + velRdOffset));
        rdSmoothed.setCurrentAndTargetValue (initRd);
        glottalSource.setRd (initRd);
    }

    // Lyrics engine: check if active and advance syllable
    lyricsActive = pLyricsEnabled != nullptr && pLyricsEnabled->load() >= 0.5f
                   && lyricsEnginePtr != nullptr && lyricsEnginePtr->getNumSyllables() > 0;

    if (lyricsActive)
        currentSyllable = lyricsEnginePtr->advanceAndGet();

    // Prime consonant engine + frication bank with THIS note's parameters
    // BEFORE triggerBurst. Fixes stale-cache bug where burst duration, VOT
    // trigger, and frication amplitudes used the previous syllable's values,
    // causing plosives to be inconsistently triggered (especially after
    // fricatives where manner=1 caused exp overflow in burst progress calc
    // and the voicing<0.5 && manner<0.3 VOT condition to fail).
    {
        float consonantTone    = lyricsActive ? currentSyllable.consonantTone
                                              : (pConsonantTone    != nullptr ? pConsonantTone->load()    : 0.5f);
        float sibilance        = lyricsActive ? currentSyllable.sibilance
                                              : (pSibilance        != nullptr ? pSibilance->load()        : 0.0f);
        float consonantVoicing = lyricsActive ? currentSyllable.consonantVoicing
                                              : (pConsonantVoicing != nullptr ? pConsonantVoicing->load() : 0.5f);
        double sr = getSampleRate();

        consonantEngine.updateCoefficients (consonantTone, sibilance, consonantVoicing, sr);

        // Manual envelope override when auto-consonant is off (lyrics always auto)
        bool autoConsonant = lyricsActive
            || (pAutoConsonant != nullptr && pAutoConsonant->load() >= 0.5f);
        if (! autoConsonant)
        {
            float consAtk   = pConsonantAttack != nullptr ? pConsonantAttack->load() : 20.0f;
            float consHold  = pConsonantHold   != nullptr ? pConsonantHold->load()   : 30.0f;
            float consDecay = pConsonantDecay  != nullptr ? pConsonantDecay->load()  : 40.0f;
            consonantEngine.setManualEnvelope (consAtk, consHold, consDecay, sr);
        }

        // VOT scale affects aspiration duration computed inside triggerBurst
        float votScale = pConsonantVOT != nullptr ? pConsonantVOT->load() : 0.5f;
        consonantEngine.setVOTScale (votScale);

        // Frication bank: set new target then snap so the 8ms plosive burst
        // is filtered by the correct place amplitudes from the very first sample
        fricationBank.setPlace (consonantTone);
        fricationBank.snapToTargets();
    }

    // Always trigger consonant envelope + burst at note onset (fresh coeffs)
    consonantEngine.triggerBurst (noteVelocity);

    // Locus-based F2/F3 transition — Delattre-Liberman-Cooper 1955.
    // Active only when a consonant is present at onset (level > 0.1).
    // Loci decay exponentially (τ=15ms) toward the vowel target over ~50ms.
    consonantTransitionSamples = 0;
    consonantTransitionActive  = false;
    {
        float consLevelAtOnset = lyricsActive
            ? currentSyllable.consonantLevel
            : (pConsonantLevel != nullptr ? pConsonantLevel->load() : 0.0f);

        if (consLevelAtOnset > 0.1f)
        {
            float placeAtOnset = lyricsActive
                ? currentSyllable.consonantTone
                : (pConsonantTone != nullptr ? pConsonantTone->load() : 0.5f);

            consonantF2Locus = computeF2Locus (placeAtOnset);
            consonantF3Locus = computeF3Locus (placeAtOnset);
            consonantTransitionActive = true;
        }
    }

    // Force immediate coefficient update on first sample
    sampleCounter = 0;
    snapFormantsOnNextUpdate = true;
}

void FormantVoice::updateAdsrParameters (bool force)
{
    if (pAttack == nullptr)
        return;

    const juce::ADSR::Parameters p { pAttack->load(), pDecay->load(),
                                     pSustain->load(), pRelease->load() };

    if (! force
        && p.attack == lastAdsrParams.attack && p.decay == lastAdsrParams.decay
        && p.sustain == lastAdsrParams.sustain && p.release == lastAdsrParams.release)
        return;

    adsr.setParameters (p);
    lastAdsrParams = p;
}

void FormantVoice::noteStopped (bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
        releaseSampleCount = 0;
    }
    else
    {
        // Hard stop: the voice goes inactive and MPESynthesiser stops rendering
        // it, so no declick tail could play — drop any pending one rather than
        // leak it into the next note-on.
        declickL = declickR = lastOutL = lastOutR = 0.0f;
        adsr.reset();
        voiceActive = false;
        wasActive = false;
        clearCurrentNote();
    }
}

void FormantVoice::beginDeclickTail() noexcept
{
    // Accumulate (a steal during a running tail keeps what is still decaying).
    declickL += lastOutL;
    declickR += lastOutR;
    lastOutL = lastOutR = 0.0f;
}

void FormantVoice::renderDeclickTail (float* outL, float* outR, int numSamples) noexcept
{
    if (declickL == 0.0f && declickR == 0.0f)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        outL[i] += declickL;
        if (outR != nullptr)
            outR[i] += declickR;
        declickL *= declickCoeff;
        declickR *= declickCoeff;
    }

    if (std::abs (declickL) < 1.0e-6f && std::abs (declickR) < 1.0e-6f)
        declickL = declickR = 0.0f;
}

void FormantVoice::notePressureChanged()
{
    // MPE pressure -> breathiness offset (additive above knob baseline)
    mpeBreathOffset = getCurrentlyPlayingNote().pressure.asUnsignedFloat();
}

void FormantVoice::notePitchbendChanged()
{
    // Nothing to do per-event: renderNextBlock reads the live bend each block via
    // getCurrentlyPlayingNote().pitchbend (× tuning_pitchBendRange, WR-04). MPESynthesiser re-splits the
    // buffer at every bend event, so the updated pitch is picked up on the next
    // renderNextBlock call for this voice.
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
    // WR-06: this voice's resonators (r up to ~0.9999) rely on FTZ/DAZ being
    // active to keep long decaying tails out of denormal range. The guarantee is
    // provided by juce::ScopedNoDenormals in PluginProcessor::processBlock, which
    // wraps every renderNextBlock call. Any future caller that drives this voice
    // directly (e.g. an offline render harness) MUST establish the same scope.
    // FormantBiquad::processSample also carries a cheap belt-and-suspenders flush.
    auto* outL = outputBuffer.getWritePointer (0, startSample);
    auto* outR = outputBuffer.getNumChannels() > 1
                     ? outputBuffer.getWritePointer (1, startSample)
                     : nullptr;

    // WR-03: a stolen note's last sample decays out here.
    renderDeclickTail (outL, outR, numSamples);

    if (! voiceActive)
        return;

    // CR-02: follow knob moves while the note is held; never during release
    // (a released note keeps the rate noteOff() computed; the new values apply
    // at the next note-on).
    if (releaseSampleCount < 0)
        updateAdsrParameters (false);

    // Nasal state (read once per block; lyrics override when active)
    float nasalCouplingVal, nasalPlaceVal;
    if (lyricsActive)
    {
        nasalCouplingVal = currentSyllable.nasalCoupling;
        nasalPlaceVal    = currentSyllable.nasalPlace;
    }
    else
    {
        nasalCouplingVal = pNasalCoupling != nullptr ? pNasalCoupling->load() : 0.0f;
        nasalPlaceVal    = pNasalPlace    != nullptr ? pNasalPlace->load()    : 0.5f;
    }
    nasalPoleZero.updateCoefficients (nasalCouplingVal, nasalPlaceVal, getSampleRate());

    // Nasal amplitude reduction: -3 dB at full coupling (gentle dip, not dramatic drop)
    float nasalAmpGain = juce::Decibels::decibelsToGain (-3.0f * nasalCouplingVal);

    // Breathiness: knob + MPE pressure offset
    float knobBreath = pBreathiness != nullptr ? pBreathiness->load() : 0.1f;
    float effectiveBreath = knobBreath + mpeBreathOffset * (1.0f - knobBreath);

    // Suppress aspiration during nasal murmurs (nasals are purely voiced)
    effectiveBreath *= (1.0f - nasalCouplingVal * 0.5f);

    // Envelope-aware breath modulation (aspirated onset + release breath burst)
    // is evaluated on the 32-sample update inside the sample loop — WR-14: a
    // once-per-block evaluation held the +4.5 dB onset boost for a whole
    // 2048-sample block, so the attack sounded different at every buffer size.
    const float baseBreath = effectiveBreath;
    const float srF = static_cast<float> (getSampleRate());
    auto breathEnvelopeMul = [this, srF]() noexcept
    {
        if (releaseSampleCount >= 0)
        {
            // Release phase: brief breath burst as vocal folds disengage (~40ms)
            const float tMs = static_cast<float> (releaseSampleCount) * 1000.0f / srF;
            if (tMs < 40.0f)
                return juce::Decibels::decibelsToGain (3.0f * std::exp (-tMs / 12.0f));
            return 1.0f;
        }
        // Attack phase: boost breath for aspirated vocal onset (~50ms)
        const float tMs = static_cast<float> (sampleCounter) * 1000.0f / srF;
        if (tMs < 50.0f)
            return juce::Decibels::decibelsToGain (4.5f * std::exp (-tMs / 15.0f));
        return 1.0f;
    };

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

    // Read consonant params once per block (lyrics override when active)
    float consonantLevel, consonantTone, sibilance, consonantVoicing;
    bool autoConsonant;
    if (lyricsActive)
    {
        consonantLevel   = currentSyllable.consonantLevel;
        consonantTone    = currentSyllable.consonantTone;
        sibilance        = currentSyllable.sibilance;
        consonantVoicing = currentSyllable.consonantVoicing;
        autoConsonant    = true;  // Lyrics engine manages consonant timing
    }
    else
    {
        consonantLevel   = pConsonantLevel   != nullptr ? pConsonantLevel->load()   : 0.0f;
        consonantTone    = pConsonantTone    != nullptr ? pConsonantTone->load()    : 0.5f;
        sibilance        = pSibilance        != nullptr ? pSibilance->load()        : 0.0f;
        consonantVoicing = pConsonantVoicing != nullptr ? pConsonantVoicing->load() : 0.5f;
        autoConsonant    = pAutoConsonant    != nullptr && pAutoConsonant->load() >= 0.5f;
    }

    // Update consonant filter coefficients (block-rate)
    consonantEngine.updateCoefficients (consonantTone, sibilance, consonantVoicing, getSampleRate());

    // Frication formant bank: place-dependent amplitudes (F3F/F4F/F6F/bypass)
    fricationBank.setPlace (consonantTone);

    // VOT scale: user knob 0-1 scales aspiration duration (default 0.5 = nominal)
    float votScale = pConsonantVOT != nullptr ? pConsonantVOT->load() : 0.5f;
    consonantEngine.setVOTScale (votScale);

    // Locus transition amount: user knob 0-1 scales F2/F3 pull at note onset
    consonantTransitionAmount = pConsonantTransition != nullptr
                                ? pConsonantTransition->load() : 0.5f;

    // When auto is off, override envelope timing with user knobs
    if (! autoConsonant)
    {
        float consAtk   = pConsonantAttack != nullptr ? pConsonantAttack->load() : 20.0f;
        float consHold  = pConsonantHold   != nullptr ? pConsonantHold->load()   : 30.0f;
        float consDecay = pConsonantDecay  != nullptr ? pConsonantDecay->load()  : 40.0f;
        consonantEngine.setManualEnvelope (consAtk, consHold, consDecay, getSampleRate());
    }

    // Formant topology: 0=Cascade, 1=Parallel (legacy), 2=Hybrid
    int topology = pFormantTopology != nullptr ? static_cast<int> (pFormantTopology->load()) : 0;

    // Formant transition time: per-formant SmoothedValue ramp durations
    float transitionTime = pTransitionTime != nullptr ? pTransitionTime->load() : 0.4f;
    filterBank.setTransitionTime (transitionTime);
    cascadeBank.setTransitionTime (transitionTime);

    // Spectral tilt: read once per block, compute one-pole alpha from f0
    float spectralTilt = pSpectralTilt != nullptr ? pSpectralTilt->load() : 0.0f;
    float tiltNorm = spectralTilt / 12.0f; // normalize to -1...+1
    float tiltF0 = tunedF0;
    float tiltCutoff = tiltF0 * 2.0f;
    float tiltAlpha = std::exp (-juce::MathConstants<float>::twoPi * tiltCutoff
                                / static_cast<float> (getSampleRate()));

    // Stereo width: compute pan gains from MIDI note (block-rate)
    float stereoWidth = pStereoWidth != nullptr ? pStereoWidth->load() : 0.5f;
    float noteNorm = currentlyPlayingNote.initialNote / 127.0f;
    float panPosition = juce::jlimit (-1.0f, 1.0f, (noteNorm - 0.5f) * stereoWidth * 2.0f);
    float panNorm = (panPosition + 1.0f) * 0.5f;
    static constexpr float halfPi = juce::MathConstants<float>::halfPi;
    float panLGain = std::cos (panNorm * halfPi);
    float panRGain = std::sin (panNorm * halfPi);

    // Velocity-to-amplitude: ~12 dB dynamic range (0.25 at vel=0, 1.0 at vel=1)
    float velocityGain = 0.25f + 0.75f * noteVelocity;

    // --- Live pitch bend: the note's wheel value × the Pitch Bend Range param ---
    // WR-04: the range was fixed at the legacy-mode ±2 st (getFrequencyInHertz
    // folds the bend in with MPEInstrument's own range); tuning_pitchBendRange
    // reached nothing. In legacy mode note.pitchbend carries the channel wheel.
    // tunedF0 already carries microtonal tuning + Dorico Note-Expression, so
    // folding the ratio into the glide target makes tuning, NE and bend stack
    // multiplicatively. MPESynthesiser splits sub-blocks at bend events, so the
    // bend is constant across one renderNextBlock — compute once here.
    // bendRatio == 1.0 when the wheel is centred.
    const float bendRangeSt = pPitchBendRange != nullptr ? pPitchBendRange->load() : 2.0f;
    const float bendRatio = std::pow (2.0f,
        getCurrentlyPlayingNote().pitchbend.asSignedFloat() * bendRangeSt / 12.0f);
    pitchGlide.setTarget (tunedF0 * bendRatio);

    for (int i = 0; i < numSamples; ++i)
    {
        // Block-rate formant coefficient update every 32 samples
        if ((sampleCounter % kCoeffUpdateInterval) == 0)
        {
            effectiveBreath = juce::jlimit (0.0f, 1.0f, baseBreath * breathEnvelopeMul());
            aspirationNoise.setBreathiness (effectiveBreath);

            float vowelX, vowelY;
            if (lyricsActive)
            {
                vowelX = currentSyllable.vowelX;
                vowelY = currentSyllable.vowelY;
            }
            else
            {
                vowelX = pVowelX != nullptr ? pVowelX->load() : 0.5f;
                vowelY = pVowelY != nullptr ? pVowelY->load() : 0.5f;
            }

            // Apply MPE timbre offset to vowelY
            vowelY = juce::jlimit (0.0f, 1.0f, vowelY + mpeVowelYOffset);

            float focus  = pVowelFocus != nullptr ? pVowelFocus->load() : 2.5f;
            float shift  = pFormantShift != nullptr ? pFormantShift->load() : 0.0f;
            float spread = pFormantSpread != nullptr ? pFormantSpread->load() : 1.0f;

            vowelMorpher.compute (vowelX, vowelY, focus,
                                  formantFreqs, formantBWs, formantGains);

            // F2/F3 locus bias — Delattre-Liberman-Cooper 1955; Kewley-Port 1982.
            // Exponential decay (τ=15ms) from place-specific loci toward the
            // vowel morpher target over ~50ms; scaled by user transitionAmount.
            // Runs before singersFormant / breathiness BW scaling so those
            // modulations ride on the post-biased frequencies naturally.
            if (consonantTransitionActive && consonantTransitionAmount > 0.0f)
            {
                if (consonantTransitionSamples < consonantTransitionMaxSamples)
                {
                    float locusWeight = std::exp (-static_cast<float> (consonantTransitionSamples)
                                                  / consonantTauSamples);
                    float w = locusWeight * consonantTransitionAmount;
                    formantFreqs[1] = w * consonantF2Locus + (1.0f - w) * formantFreqs[1];
                    formantFreqs[2] = w * consonantF3Locus + (1.0f - w) * formantFreqs[2];
                    consonantTransitionSamples += kCoeffUpdateInterval;
                }
                else
                {
                    consonantTransitionActive = false;
                }
            }

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

            // (3) Nasal damping — nasal cavity walls add loss, widening formant BWs up to 2x
            if (nasalCouplingVal > 0.0f)
            {
                float nasalBWScale = 1.0f + nasalCouplingVal * 0.6f;
                for (int fi = 0; fi < 5; ++fi)
                    formantBWs[fi] *= nasalBWScale;
            }

            // Singer's formant: cluster F3-F5 toward ~3 kHz (Sundberg)
            float singersFormant = pSingersFormant != nullptr ? pSingersFormant->load() : 0.0f;
            if (singersFormant > 0.0f)
            {
                static constexpr float clusterCenter = 3000.0f;
                static constexpr float clusterStrength[3] = { 0.7f, 0.8f, 0.6f }; // F3, F4, F5
                float gainBoost = juce::Decibels::decibelsToGain (4.0f * singersFormant);

                for (int fi = 2; fi < 5; ++fi)
                {
                    formantFreqs[fi] += singersFormant * (clusterCenter - formantFreqs[fi]) * clusterStrength[fi - 2];
                    formantBWs[fi] *= (1.0f - singersFormant * 0.4f);
                    formantGains[fi] *= gainBoost;
                }
            }

            // Always update parallel bank (used by all topologies)
            filterBank.updateCoefficients (formantFreqs, formantBWs, formantGains,
                                           shift, spread, getSampleRate());

            // Update cascade bank when needed (cascade or hybrid topology)
            if (topology != 1)
            {
                cascadeBank.setNumCascadeStages (topology == 0 ? 5 : 3);
                cascadeBank.updateCoefficients (formantFreqs, formantBWs,
                                                shift, spread, getSampleRate());
            }

            // CR-03: the note's first update lands on this note's formants
            // instead of gliding from the previous note's (or from 0 Hz on a
            // fresh voice) — the Transition ramp is for moves within a note.
            // Snap, then re-run so coefficients apply directly (a snapped
            // smoother no longer drives them from process()).
            if (snapFormantsOnNextUpdate)
            {
                snapFormantsOnNextUpdate = false;
                filterBank.snapToTargets();
                filterBank.updateCoefficients (formantFreqs, formantBWs, formantGains,
                                               shift, spread, getSampleRate());
                if (topology != 1)
                {
                    cascadeBank.snapToTargets();
                    cascadeBank.updateCoefficients (formantFreqs, formantBWs,
                                                    shift, spread, getSampleRate());
                }
            }

            // Source-filter coupling: harmonic reinforcement near formant peaks (Titze 2008)
            float coupling = pSourceFilterCoupling != nullptr ? pSourceFilterCoupling->load() : 0.3f;
            if (coupling > 0.0f)
            {
                float f0Est = tunedF0;
                float bestProximity = 0.0f;

                for (int h = 2; h <= 4; ++h)
                {
                    float harmFreq = f0Est * static_cast<float> (h);
                    for (int fi = 0; fi < 2; ++fi)
                    {
                        float bw = formantBWs[fi];
                        if (bw > 0.0f)
                        {
                            float dist = std::abs (harmFreq - formantFreqs[fi]);
                            if (dist < bw)
                                bestProximity = std::max (bestProximity, 1.0f - dist / bw);
                        }
                    }
                }

                float boostDb = 2.0f * bestProximity * coupling;
                sourceFilterGain.setTargetValue (juce::Decibels::decibelsToGain (boostDb));
                sourceFilterJitterBoost = 0.003f * bestProximity * coupling;
            }
            else
            {
                sourceFilterGain.setTargetValue (1.0f);
                sourceFilterJitterBoost = 0.0f;
            }
        }

        // --- Per-sample pitch: PitchGlide -> VibratoLFO -> final F0 ---
        float baseF0 = pitchGlide.getNextFrequency();
        float vibCents = vibratoLFO.getNextValue (vibratoRate, vibratoDepth);
        float jitterOffset = vibratoLFO.getJitterOffset() + sourceFilterJitterBoost;
        float finalF0 = baseF0 * std::pow (2.0f, vibCents / 1200.0f) * (1.0f + jitterOffset);
        glottalSource.setFrequency (finalF0);

        // Per-sample smoothed Rd update (20ms ramp avoids clicks)
        glottalSource.setRd (rdSmoothed.getNextValue());

        // Generate glottal pulse sample
        float glottal = glottalSource.getNextSample();

        // Pass glottal cycle phase for pitch-synchronous aspiration
        float glottalPhase = glottalSource.getPhase();
        aspirationNoise.setGlottalPhase (glottalPhase);
        // Klatt MOD: voiced-fricative noise gating at F0
        consonantEngine.setGlottalPhase (glottalPhase);

        // Mix with aspiration noise
        float source = aspirationNoise.process (glottal);

        // Consonant noise (shaped by place/manner filters + dedicated envelope)
        float consonantNoise = consonantEngine.getNextSample (consonantLevel);
        float onsetSuppression = consonantEngine.getOnsetSuppression();
        float continuousSuppression = consonantEngine.getContinuousSuppression();

        // Suppress glottal source: onset burst (plosive) + continuous (voiceless fricative)
        float totalSuppression = juce::jmin (1.0f, onsetSuppression + continuousSuppression);
        float voiceSource = source * (1.0f - 0.7f * totalSuppression);

        // ADSR envelope — applied to voiced source only
        float env = adsr.getNextSample();
        float voiceWithEnv = voiceSource * env;

        // Route through formant filters — topology determines signal path
        float sample;

        if (topology == 1) // Parallel (legacy): voice+consonant mixed, then parallel bank
        {
            float fullSource = voiceWithEnv + consonantNoise;
            float tiltLP = (1.0f - tiltAlpha) * fullSource + tiltAlpha * spectralTiltPrev;
            spectralTiltPrev = tiltLP;
            float tiltedSource = fullSource - tiltNorm * (fullSource - tiltLP);
            sample = filterBank.process (tiltedSource);
        }
        else // Cascade (0) or Hybrid (2): split voiced/consonant paths
        {
            // Spectral tilt on voiced path only (models glottal spectral slope)
            float tiltLP = (1.0f - tiltAlpha) * voiceWithEnv + tiltAlpha * spectralTiltPrev;
            spectralTiltPrev = tiltLP;
            float tiltedVoice = voiceWithEnv - tiltNorm * (voiceWithEnv - tiltLP);

            // Aspiration noise (VOT phase) routed through cascade bank so it is
            // shaped by the opening vocal tract toward the following vowel.
            float aspirationInject = consonantEngine.getAspirationNoise();

            // Voiced through cascade bank (correct relative formant amplitudes)
            float voicedFiltered = cascadeBank.process (tiltedVoice + aspirationInject);

            // Consonant through Klatt frication bank (parallel F3F/F4F/F6F + bypass)
            float consonantFiltered = fricationBank.process (consonantNoise);

            sample = voicedFiltered + consonantFiltered;
        }

        // Source-filter coupling: harmonic reinforcement gain (smoothed)
        sample *= sourceFilterGain.getNextValue();

        // Nasal pole-zero filtering (transparent when nasalCoupling = 0 via wet/dry mix)
        sample = nasalPoleZero.process (sample);
        sample *= nasalAmpGain;

        // Soft-clip to prevent extreme amplitudes from resonant filters
        sample = std::tanh (sample);

        // Velocity dynamics applied post-tanh so dynamic range isn't compressed
        sample *= velocityGain;

        // NaN/Inf guard (belt-and-suspenders after tanh)
        if (! std::isfinite (sample))
        {
            sample = 0.0f;
            // IN-03: also reset the excitation sources, not just the filters. If a
            // NaN originates upstream of the banks (glottal oscillator or aspiration
            // noise), resetting only the filter state leaves the source re-injecting
            // NaN on the next sample and the guard never clears. fricationBank feeds
            // the consonant path into `sample`, so reset it too.
            glottalSource.reset();
            aspirationNoise.reset();
            filterBank.reset();
            cascadeBank.reset();
            fricationBank.reset();
            nasalPoleZero.reset();
            consonantEngine.reset();
        }

        // Stereo width: pan by MIDI note (equal-power)
        lastOutL = sample * panLGain;
        lastOutR = sample * panRGain;
        outL[i] += lastOutL;
        if (outR != nullptr)
            outR[i] += lastOutR;

        ++sampleCounter;
        if (releaseSampleCount >= 0)
            ++releaseSampleCount;
    }

    // Check if voice has finished releasing
    if (! adsr.isActive())
    {
        voiceActive = false;
        lastOutL = lastOutR = declickL = declickR = 0.0f; // inactive voices aren't rendered
        clearCurrentNote();
    }
}
