/*
  ==============================================================================

    BassoonVoice.cpp
    Modal Synthesis Bassoon - Synthesiser Voice (Phase 2.1 first audio)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.3: Per-Note Expression — Envelope, Breath, Vibrato, Output Gain.
    Architectural pivot: continuous breath-driven sustain via NoiseExciter
    replaces struck-modal-only sustain. Phase 2.2 rev-3 strike() retained at
    startNote for attack transient; Phase 2.1 impulse Exciter member retained
    (D6-rev-3) but NOT called from renderNextBlock — Phase 2.4 re-wires for
    attack-character morph.

    Phase 2.1 carry-forward: 16-mode parallel pole-only resonator bank,
    raw-14-bit pitch-bend ±2 semitones, equal L+R per-sample voice write.
    Phase 2.2 carry-forward: bassoon partial table + formant Gaussian, tone
    SmoothedValue dispatch, 1/8 headroom scaler.

  ==============================================================================
*/

#include "BassoonVoice.h"

bool BassoonVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<BassoonSound*> (sound) != nullptr;
}

void BassoonVoice::prepareToPlay (double sampleRate, int /*maxBlockSize*/)
{
    setCurrentPlaybackSampleRate (sampleRate);   // JUCE bookkeeping
    modeBank.prepare (sampleRate);
    exciter.prepare  (sampleRate);                // D6-rev-3 retention

    // CRITICAL: setSampleRate MUST be called BEFORE setParameters
    // (JUCE 8 ADSR contract — juce_ADSR.h:115-119; recalculateRates uses
    // the stored sample rate, defaulted to 44100.0 if setSampleRate not called).
    adsr.setSampleRate (sampleRate);
    adsr.setParameters (juce::ADSR::Parameters { 0.010f, 0.0f, 1.0f, 0.200f });

    // Phase 2.3 systems
    vibrato.prepare (sampleRate);
    noiseExciter.prepare (sampleRate, voiceIndex);
    breathSmoother.reset (sampleRate, 0.020);   // 20 ms ramp (CONTEXT-rev-3 line 521)
    breathSmoother.setCurrentAndTargetValue (0.7f);
}

void BassoonVoice::startNote (int midiNoteNumber, float velocity,
                              juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    pitchWheelValue       = currentPitchWheelPosition;
    pitchBendSemitones    = ((static_cast<float> (pitchWheelValue) - 8192.0f) / 8192.0f)
                            * PITCH_BEND_RANGE_SEMITONES;

    // Phase 2.4 (DSP-06 + TuningEngine): compose chain TuningEngine → applyPendingTuning.
    // O-Lyrica precedent — HarpSynthVoice.cpp:113-147. Bit-identical to Phase 2.3
    // baseline at default 12-TET A=440 (octaveStretch=1.0f cast preserves the value).
    double f_double = (tuningEngine != nullptr)
        ? tuningEngine->getFrequency (midiNoteNumber)
        : juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    if (pendingTuningSource != nullptr)
        f_double = Ouaricon::NoteExpression::applyPendingTuning (
                       *pendingTuningSource, midiNoteNumber, f_double);

    currentFrequencyBase = static_cast<float> (f_double);

    const float fBent = currentFrequencyBase * std::pow (2.0f, pitchBendSemitones / 12.0f);
    modeBank.setFundamental (fBent);
    modeBank.strike();           // rev-3: inject modal sustain energy (state init)

    // Phase 2.4 (DSP-05): re-engage Exciter via attack-character morph + velocity bias.
    // attackChar snapshot at note-on; mid-onset automation does NOT affect in-flight onset.
    const float attackChar = parameters->getRawParameterValue ("attack_character")->load();
    exciter.startOnset (attackChar, velocity);

    // Phase 2.3: ADSR APVTS reads at note-on (one-shot)
    const float attackMs  = parameters->getRawParameterValue ("attack_time")->load();
    const float releaseMs = parameters->getRawParameterValue ("release_time")->load();
    adsr.setParameters ({ attackMs / 1000.0f, 0.0f, 1.0f, releaseMs / 1000.0f });
    adsr.noteOn();

    // Phase 2.3: reset Phase 2.3 systems
    vibrato.reset();
    noiseExciter.reset();
    breathSmoother.setCurrentAndTargetValue (velocity);  // velocity-as-initial-UI-breath
    lastUiBreath = velocity;

    // Phase 2.3: CC2 state reset on every note-on
    cc2EverActive      = false;
    lastCC2SampleCount = 0;

    // Phase 2.3: shadow init — force first setExpression dispatch
    lastAppliedAttackMs   = -1.0f;
    lastAppliedReleaseMs  = -1.0f;
    lastAppliedVibRate    = -1.0f;
    lastAppliedVibDepth   = -1.0f;
    lastAppliedVibOnsetMs = -1.0f;
    lastDispatchedFrequency = 0.0f;

    // rev-4: force first vibratoMult recompute on first per-sample iteration
    lastVibratoCents  = 1.0e9f;
    cachedVibratoMult = 1.0f;
}

void BassoonVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
        modeBank.reset();
        exciter.reset();
        currentFrequencyBase = 0.0f;
    }
}

void BassoonVoice::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelValue    = newPitchWheelValue;
    pitchBendSemitones = ((static_cast<float> (pitchWheelValue) - 8192.0f) / 8192.0f)
                         * PITCH_BEND_RANGE_SEMITONES;

    // Guard: only recompute coefficients if a note is currently sounding.
    // Pitch-wheel events arriving before the first note-on must not mutate
    // the mode bank with f0 = 0.
    if (currentFrequencyBase > 0.0f)
    {
        const float fBent = currentFrequencyBase * std::pow (2.0f, pitchBendSemitones / 12.0f);
        modeBank.setFundamental (fBent);
    }
}

void BassoonVoice::controllerMoved (int controllerNumber, int newControllerValue)
{
    if (controllerNumber == 2)  // CC2: MIDI breath controller
    {
        const float cc2Normalised = juce::jlimit (0.0f, 1.0f,
                                                   static_cast<float> (newControllerValue) / 127.0f);
        cc2EverActive       = true;
        lastCC2SampleCount  = currentSampleCount;
        // Multiplicative compose: breath_voice = ui_breath × cc2_normalised
        breathSmoother.setTargetValue (lastUiBreath * cc2Normalised);
    }
    // CC1 / aftertouch deferred to v1.1 per Stage 0 D4.
}

void BassoonVoice::setTone (float tone01) noexcept
{
    // Phase 2.2: throttle gate is at the processor dispatch site, not here.
    modeBank.setTone (tone01);
    modeBank.applyToneChange();
}

void BassoonVoice::setExpression (float attackMs, float releaseMs,
                                   float vibRateHz, float vibDepthCents, float vibOnsetMs,
                                   float uiBreath) noexcept
{
    constexpr float EPS = 0.001f;

    // ADSR — re-shape only when changed
    if (std::abs (attackMs  - lastAppliedAttackMs)  > EPS
     || std::abs (releaseMs - lastAppliedReleaseMs) > EPS)
    {
        adsr.setParameters ({ attackMs / 1000.0f, 0.0f, 1.0f, releaseMs / 1000.0f });
        lastAppliedAttackMs  = attackMs;
        lastAppliedReleaseMs = releaseMs;
    }

    if (std::abs (vibRateHz     - lastAppliedVibRate)    > EPS) { vibrato.setRateHz (vibRateHz);     lastAppliedVibRate    = vibRateHz; }
    if (std::abs (vibDepthCents - lastAppliedVibDepth)   > EPS) { vibrato.setDepthCents (vibDepthCents); lastAppliedVibDepth = vibDepthCents; }
    if (std::abs (vibOnsetMs    - lastAppliedVibOnsetMs) > EPS) { vibrato.setOnsetMs (vibOnsetMs);   lastAppliedVibOnsetMs = vibOnsetMs; }

    // Breath UI shadow (always update — small cost; CC2-takeover gate decides effective target)
    lastUiBreath = uiBreath;

    // CC2-takeover gate: only apply UI breath if CC2 idle for >500 ms
    const juce::int64 cc2WindowSamples = static_cast<juce::int64> (0.500 * getSampleRate());
    const bool cc2RecentlyActive = cc2EverActive
                                && (currentSampleCount - lastCC2SampleCount) < cc2WindowSamples;
    if (! cc2RecentlyActive)
        breathSmoother.setTargetValue (uiBreath);
    // else: CC2 is the active source; controllerMoved sets the smoother target
}

void BassoonVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                    int startSample, int numSamples)
{
    if (! adsr.isActive())
        return;

    // pitchBend is block-rate (pitchWheelMoved fires at MIDI cadence); compute once.
    const float pbMult = std::pow (2.0f, pitchBendSemitones / 12.0f);

    // Per-sample render: vibrato → f_final dispatch (throttled) → continuous noise
    // → modeBank → ADSR → output.
    //
    // rev-4 fix: vibrato.getCurrentCents() advances LFO phase by ONE step per call,
    // so it MUST run per-sample. Previously called once per block, which collapsed
    // the 5 Hz LFO to (5 / blockSize) Hz ≈ 0.02 Hz — effectively DC, stuck at the
    // random initial phase set in Vibrato::reset(). Throttle |Δf|>0.1 Hz keeps
    // modeBank coefficient recompute to <1 kHz worst-case at max excursion
    // derivative (5 Hz × 100 cents → ~120 Hz/s).
    //
    // Phase 2.1 impulse Exciter call dropped (D6-rev-3 — file/member retained
    // for Phase 2.4 attack-character morph re-introduction).
    // Voices SUM into the buffer — host's processBlock clears it BEFORE
    // synthesiser.renderNextBlock (PluginProcessor.cpp).
    for (int i = 0; i < numSamples; ++i)
    {
        // rev-4 fix: vibrato phase MUST advance per sample (LFO is per-sample);
        // the std::pow recompute is throttled by |Δc| > 0.5 c (sub-cent
        // resolution, well below audible threshold). Item 10 8-voice CPU
        // dropped from ~25 % → target <20 % by collapsing pow rate from
        // 48 kHz/voice → ~3 kHz/voice at max LFO derivative.
        const float vibratoCents = vibrato.getCurrentCents();
        if (std::abs (vibratoCents - lastVibratoCents) > 0.5f)
        {
            cachedVibratoMult = std::pow (2.0f, vibratoCents / 1200.0f);
            lastVibratoCents  = vibratoCents;
        }
        const float f_final = currentFrequencyBase * cachedVibratoMult * pbMult;

        if (std::abs (f_final - lastDispatchedFrequency) > 0.1f)
        {
            modeBank.setFundamental (f_final);
            lastDispatchedFrequency = f_final;
        }

        // Phase 2.4 (DSP-05): additive Exciter contribution during onset window.
        // exciter.getNextSample() auto-zeros after onset window, so only NoiseExciter
        // sustains thereafter (OQ#8-rev-4 additive composition).
        const float breath        = breathSmoother.getNextValue();
        const float noiseSample   = noiseExciter.getNextSample (breath);
        const float exciterSample = exciter.getNextSample();
        const float excitation    = noiseSample + exciterSample;
        const float voice         = modeBank.processSample (excitation);
        const float env           = adsr.getNextSample();
        const float sample        = voice * env;

        outputBuffer.addSample (0, startSample + i, sample);
        outputBuffer.addSample (1, startSample + i, sample);
    }

    // Phase 2.3: advance sample-count for CC2-takeover state machine
    currentSampleCount += numSamples;

    // ADSR-idle exit (Phase 2.1 invariant preserved — moved to post-loop
    // because per-sample early-return would skip currentSampleCount advance).
    if (! adsr.isActive())
    {
        clearCurrentNote();
        modeBank.reset();
        noiseExciter.reset();
        currentFrequencyBase = 0.0f;
    }
}
