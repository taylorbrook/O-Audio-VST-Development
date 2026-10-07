/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - Voice + Sound (Stage 2.2 - 2.4)
    Ouaricon Audio
    Developer: Taylor Brook

    One wavetable voice: double phase, strict mip level, shared linear read
    (wt::readSample), frame lerp (Interp On) or cycle-wrap latch (Off),
    mid-rise quantizer, amp ADSR, squared velocity, +/-2 st bend.

    Per-block inputs come from the processor through ONE BlockContext (D-H)
    whose pointer is set once in prepareToPlay and stays stable. Voices
    index its per-sample arrays at the in-buffer index startSample + i of the
    buffer they are handed (the processor hands every chunk its own view, so
    the arrays never need a chunk-origin offset). Voices never read APVTS.

    Position (2.3, RESEARCH 2.4 / 3.2):
      raw    = clamp01 (knob + depth * 0.5 * lfo + amount * modEnv)   (DSP-05)
      effPos = Interp On : 2 ms PositionSmoother(raw), seeded at the first
                           rendered sample after note-on (needsSeed); not
                           reseeded on a legato move.
               Interp Off: raw (the smoother tracks raw, so an Off -> On
                           toggle never glides from a stale value).
      Interp Off latches round(raw * (N - 1)) at note-on and at each wrap.
    Mod env: per voice, setSampleRate before setParameters, dirty-checked,
    noteOn at startNote and on a Mono RETRIGGER only (never on legato).

    Lifetime follows the AMP envelope only (FUNC-06): the voice ends even in
    the middle of a long mod-env release.

    Frozen-cycle crossfader (2.4, RESEARCH 4, QUAL-03):
      - triggers: bank pointer, mip level, interp, bandlimit (NOT Interp-Off
        frame steps, NOT bit depth). Checked at the top of EVERY render call
        (per block, after each pitch-wheel / MIDI sub-block, after each mono
        pitch change).
      - on a trigger the current output cycle is captured with wt::readSample
        from the OLD ReadCfg (this voice's own cfg.bank, the only bank the
        reaper protects for the next block) into one of two ping-pong
        float[2049] buffers (guard = s[0]); the output then crossfades
        from the frozen cycle (read at the running phase) to the live read
        under the new config over xfLenActive samples, raised-cosine:
            sample k after the trigger (k = 0 first):
            w = 0.5 - 0.5 * cos (pi * k / len)   (xfWeight; Stage 2.4 deviation
            from RESEARCH 4's linear w: QUAL-03 ratio tier, see SUMMARY),
            out = (1 - w) * q(frozen) + w * q(live),  q = bit quantizer.
        len = round(0.005 * fs) (test hook xfadeLenOverride; 0 = hard switch).
      - FOLD rule: a trigger during a running fade writes the currently heard
        mixed cycle (1 - w) * frozen + w * liveOld into the free buffer,
        swaps, and restarts. The outgoing buffer is never dropped.
      - Imported -> empty: live = 0, so it fades to silence; empty -> bank:
        the frozen cycle is silent, so it fades in.
      - D-K level hysteresis: moving UP a level (fewer harmonics) switches
        immediately; moving DOWN needs f < boundary * 2^(-1/24). Alias-safe
        (the held level always has fewer harmonics than the strict one) and
        stops crossfade storms from bend jitter. A note-on always takes the
        strict level (no history).

    Hard-stop tail (W1, QUAL-01): stopNote (..., false) from a voice steal,
    a Poly <-> Mono switch or all-sound-off ends the amp env in one sample.
    The voice's last output sample (folded with any running tail) is then
    added back as a 2 ms raised-cosine decay to 0, rendered at the top of
    every render call even while the voice is idle or playing the stealing
    note. A scalar only: no bank is read, so the reaper rules are untouched.

    Velocity ramp (W2, QUAL-01): a note that starts on a voice whose amp env
    is still active (a Mono retrigger from the release tail) ramps velGain
    linearly from its current value to the new one over 3 ms. A start from
    idle sets velGain instantly (G-VEL and the goldens are unchanged).

    Do NOT declare a method named isVoiceActive: that is the Synthesiser's
    allocation virtual; shadowing it would hijack voice stealing.

  ==============================================================================
*/

#pragma once

#include "TestHooks.h"   // first: the OSIW_TEST_HOOKS default (Stage 2 note 8)

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#if OSIW_TEST_HOOKS
 #include <vector>
#endif

#include "BitQuantizer.h"
#include "PositionSmoother.h"
#include "WavetableBank.h"
#include "WtRead.h"

//==============================================================================
// Filled by the processor before any voice renders in a block (D-H).
struct BlockContext
{
    const float* knobPos   = nullptr;   // [preparedBlock] 20 ms smoothed position knob
    const float* lfo       = nullptr;   // [preparedBlock] global LFO -1..1 (PositionLfo buffer)
    const float* lfoDepth  = nullptr;   // [preparedBlock] 20 ms smoothed lfo_depth  (D-I)
    const float* envAmount = nullptr;   // [preparedBlock] 20 ms smoothed env_amount (D-I)
    const WavetableBank* bank = nullptr;   // resolved ONCE per block; nullptr = silence (Imported empty)
    bool interp    = true;
    bool bandlimit = true;
    int  bitDepthIndex = 0;                // 0 = Full
};

//==============================================================================
struct WtSound final : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class WtVoice final : public juce::SynthesiserVoice
{
public:
    static constexpr float kVoiceGain = 0.5f;   // tuned at the listening checkpoint (Task 11)
    static constexpr double kXfadeSeconds = 0.005;              // frozen-cycle crossfade (ARCHITECTURE 5)
    static constexpr double kTailSeconds = 0.002;               // hard-stop tail decay (W1)
    static constexpr double kVelRampSeconds = 0.003;            // sounding-retrigger velocity ramp (W2)
    static constexpr double kLevelHysteresis = 1.0293022366434921;   // 2^(1/24): D-K downward margin

    // parameter-spec defaults for the mod env (menv_attack/decay/sustain/release).
    static juce::ADSR::Parameters defaultModEnvParams() noexcept { return { 0.5f, 1.0f, 0.0f, 0.5f }; }

    // Read configuration: a change of any field is a crossfade trigger in 2.4.
    struct ReadCfg
    {
        const WavetableBank* bank = nullptr;
        int  level     = 0;
        bool interp    = true;
        bool bandlimit = true;

        bool sameAs (const ReadCfg& o) const noexcept
        {
            return bank == o.bank && level == o.level && interp == o.interp && bandlimit == o.bandlimit;
        }
    };

    //==========================================================================
    // Pointer must stay valid for the voice's lifetime (a processor member).
    void setBlockContext (const BlockContext* c) noexcept { ctx = c; }

    // Non-virtual: SynthesiserVoice has no prepare hook, so the processor
    // dispatches here directly. setSampleRate BEFORE setParameters (jassert),
    // for BOTH envelopes.
    void prepareToPlay (double newSampleRate, int maxBlock, const juce::ADSR::Parameters& amp)
    {
        prepareToPlay (newSampleRate, maxBlock, amp, defaultModEnvParams());
    }

    void prepareToPlay (double newSampleRate, int maxBlock, const juce::ADSR::Parameters& amp,
                        const juce::ADSR::Parameters& menv)
    {
        setCurrentPlaybackSampleRate (newSampleRate);
        sr = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        ampEnv.setSampleRate (sr);
        ampEnv.setParameters (amp);
        ampParams  = amp;
        appliedAmp = amp;
        ampEnv.reset();

        modEnv.setSampleRate (sr);
        modEnv.setParameters (menv);
        menvParams  = menv;
        appliedMenv = menv;
        modEnv.reset();

        releasing = false;

        smoother.prepare (sr);
        smoother.seed (0.0f);
        needsSeed = true;
        effPos   = 0.0f;
        lastMenv = 0.0f;
        lastAmp  = 0.0f;

        phase = 0.0;
        cfgFresh = true;
        needsLatch = true;

        xfadeLen = juce::jmax (1, (int) std::lround (kXfadeSeconds * sr));
        xfActive = false;
        xfPos = 0;
        xfLenActive = xfadeLen;
        frozenActive = 0;
        for (int b = 0; b < 2; ++b)
        {
            std::fill (frozen[b], frozen[b] + WavetableBank::kStride, 0.0f);
            frozenSilent[b] = true;
        }

        tailLen  = juce::jmax (1, (int) std::lround (kTailSeconds * sr));
        tailVal  = 0.0f;
        tailPos  = 0;
        tailLeft = 0;
        lastOut  = 0.0f;

        velRampLen  = juce::jmax (1, (int) std::lround (kVelRampSeconds * sr));
        velRampLeft = 0;
        velStep     = 0.0f;

        updatePitch();
        clearCurrentNote();

       #if OSIW_TEST_HOOKS
        effTrace.assign ((size_t) juce::jmax (1, maxBlock), 0.0f);
       #else
        juce::ignoreUnused (maxBlock);
       #endif
    }

    // Once per block from the processor. Dirty-checked; never while releasing
    // (setParameters recomputes the release rate from SUSTAIN and with
    // sustain 0 resets the envelope mid-tail). Deferred params apply at note-on.
    // The same rule covers the mod env (it releases together with the amp env).
    void setBlockParams (const juce::ADSR::Parameters& amp) noexcept
    {
        ampParams = amp;
        if (! releasing && ! sameAdsr (amp, appliedAmp))
        {
            ampEnv.setParameters (amp);
            appliedAmp = amp;
        }
    }

    void setBlockParams (const juce::ADSR::Parameters& amp, const juce::ADSR::Parameters& menv) noexcept
    {
        setBlockParams (amp);
        menvParams = menv;
        if (! releasing && ! sameAdsr (menv, appliedMenv))
        {
            modEnv.setParameters (menv);
            appliedMenv = menv;
        }
    }

    //==========================================================================
    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<WtSound*> (s) != nullptr; }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override
    {
        // Synthesiser sets currentlyPlayingNote BEFORE startNote, so the env is
        // the only honest "was idle" signal. The Synthesiser hard-stops a busy
        // voice before startVoice, so this is normally true.
        startedFromIdle = ! ampEnv.isActive();

        noteHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        lastNote = midiNote;                    // display only (D-X)
        pitchWheelPos = currentPitchWheelPosition;      // SEED the wheel at note-on
        updatePitch();

        const float v = velocity >= 0.0f ? (velocity <= 1.0f ? velocity : 1.0f) : 0.0f;   // NaN -> 0
        setVelocityGain (v * v, ! startedFromIdle);     // squared velocity (CONTEXT); ramp only if sounding
        noteAge = ++sNoteCounter;

        phase      = 0.0;
        needsLatch = true;
        cfgFresh   = true;                              // adopt the config with no capture (2.4)
        needsSeed  = true;                              // smoother seeded at the first rendered sample

        applyDeferredParams();
        releasing = false;
        ampEnv.noteOn();

        // A fresh note gets a fresh mod contour (the previous note's mod env
        // may have been mid-release when the amp env ended the voice).
        modEnv.reset();
        modEnv.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            releasing = true;
            ampEnv.noteOff();
            modEnv.noteOff();
            if (! ampEnv.isActive())
                endVoice();
        }
        else
        {
            if (ampEnv.isActive())
                startTail();                            // W1: decay the last sample, never a step to 0
            ampEnv.reset();
            velRampLeft = 0;
            endVoice();
        }
        // Note-keyed state (noteHz, wheel, cfg) is NOT cleared here.
    }

    void pitchWheelMoved (int newValue) override
    {
        pitchWheelPos = newValue;
        updatePitch();          // level is re-selected at the top of the next render call
    }

    void controllerMoved (int, int) override {}

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (tailLeft > 0 && numSamples > 0 && out.getNumChannels() > 0)
            renderTail (out.getWritePointer (0), startSample, numSamples);   // idle or not (W1)

        if (! ampEnv.isActive())
        {
            if (getCurrentlyPlayingNote() >= 0)
                endVoice();
            return;
        }

        if (ctx == nullptr || numSamples <= 0 || out.getNumChannels() <= 0)
            return;

        refreshReadConfig();
        quant.setChoiceIndex (ctx->bitDepthIndex);

        const WavetableBank* b = cfg.bank;
        const int nF = b != nullptr ? b->numFrames : 0;
        float* dst = out.getWritePointer (0);           // mono into channel 0; processor copies

        const float* knob  = ctx->knobPos;
        const float* lfo   = ctx->lfo;
        const float* depth = ctx->lfoDepth;
        const float* amt   = ctx->envAmount;

        bool smooth = cfg.interp;                       // the 2 ms pole runs with Interp On only
       #if OSIW_TEST_HOOKS
        if (smootherBypass)
            smooth = false;
       #endif

        float eff  = effPos;
        float menv = lastMenv;
        float env  = lastAmp;
        for (int i = 0; i < numSamples; ++i)
        {
            const int j = startSample + i;
            env  = ampEnv.getNextSample();
            menv = modEnv.getNextSample();

            // DSP-05: clamp the SUM, then smooth (a convex filter of values
            // in [0, 1] stays in [0, 1]; the second clamp guards rounding).
            const float raw = wt::clamp01 (knob[j] + depth[j] * 0.5f * lfo[j] + amt[j] * menv);

            if (needsSeed)
            {
                smoother.seed (raw);                    // first rendered sample after note-on
                needsSeed = false;
            }

            if (smooth)
            {
                eff = wt::clamp01 (smoother.process (raw));
            }
            else
            {
                smoother.seed (raw);                    // track raw: no stale glide on Off -> On
                eff = raw;
            }

            if (needsLatch)
            {
                latchedFrame = wt::latchFrame (raw, nF);
                needsLatch = false;
            }

           #if OSIW_TEST_HOOKS
            if (j >= 0 && j < (int) effTrace.size())
                effTrace[(size_t) j] = eff;
           #endif

            float s = 0.0f;
            if (nF > 0)
                s = quant.apply (wt::readSample (*b, cfg.level, phase, cfg.interp, eff, latchedFrame));

            if (xfActive)                               // frozen-cycle crossfade (QUAL-03)
            {
                const float w = xfWeight (xfPos, xfLenActive);
                bool keepRate = true;
               #if OSIW_TEST_HOOKS
                keepRate = xfKeepRate;
               #endif
                const float fz = frozenSilent[frozenActive] ? 0.0f
                                                            : quant.apply (readFrozen (frozen[frozenActive], keepRate ? xfPhase : phase));
                s = (1.0f - w) * fz + w * s;
                xfPhase += xfInc;                       // W4: the outgoing cycle keeps the rate it was heard at
                if (xfPhase >= 1.0)
                    xfPhase -= 1.0;
                if (++xfPos >= xfLenActive)
                    xfActive = false;
            }

            if (velRampLeft > 0)                        // W2: sounding retrigger
            {
                velGain = --velRampLeft > 0 ? velGain + velStep : velTarget;
            }

            const float y = s * env * (velGain * kVoiceGain);
            dst[j] += y;
            lastOut = y;

            phase += inc;
            if (phase >= 1.0)
            {
                phase -= 1.0;
                if (! cfg.interp)
                    latchedFrame = wt::latchFrame (raw, nF);
            }

            if (! ampEnv.isActive())
                break;
        }

        renderedInc = inc;                              // W4: rate of the last rendered sample
        effPos    = eff;
        lastMenv  = menv;
        lastAmp   = env;
        lastPos   = eff;
        lastLevel = cfg.level;
        lastFrame = cfg.interp ? wt::latchFrame (eff, nF) : latchedFrame;

        if (! ampEnv.isActive())
        {
            lastOut = 0.0f;                             // ended by its own release: nothing to fade
            endVoice();
        }
    }

    //==========================================================================
    // Mono path (processor drives voice 0 directly; true legato, CONTEXT).
    // retrigger = the held-note stack was empty (voice idle or releasing):
    // noteOn() WITHOUT reset() so the attack ramps from the tail level, and
    // the phase is kept if the voice is still sounding. Without retrigger the
    // pitch changes only (envelope, phase and velocity continue).
    void noteOnDirect (int midiNote, float velocity, bool retrigger)
    {
        const bool wasSounding = ampEnv.isActive();
        noteHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        lastNote = midiNote;                    // display only (D-X)
        updatePitch();

        if (retrigger)
        {
            const float v = velocity >= 0.0f ? (velocity <= 1.0f ? velocity : 1.0f) : 0.0f;
            setVelocityGain (v * v, wasSounding);       // W2: ramp from the tail's gain, never a step
            if (! wasSounding)
            {
                phase      = 0.0;
                needsLatch = true;
                cfgFresh   = true;
                needsSeed  = true;                      // idle: seed at the first sample
                modEnv.reset();                         // idle: fresh mod contour
            }
            // Sounding (release tail): the smoother keeps tracking and both
            // envelopes re-attack from their CURRENT level (no reset, no click).
            startedFromIdle = ! wasSounding;
            noteAge = ++sNoteCounter;
            applyDeferredParams();
            releasing = false;
            ampEnv.noteOn();
            modEnv.noteOn();
        }
        // Legato move: pitch only. Amp env, mod env and smoother continue.
    }

    // Fallback to a still-held note: pitch only (no retrigger, velocity kept).
    void setPitchNote (int midiNote)
    {
        noteHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        lastNote = midiNote;                    // display only (D-X)
        updatePitch();
    }

    void noteOffDirect (bool allowTailOff) { stopNote (0.0f, allowTailOff); }

    //==========================================================================
    // Display getters (audio thread; the processor publishes the lead voice
    // through relaxed atomics).
    bool          isSounding() const noexcept   { return ampEnv.isActive(); }
    std::uint64_t getNoteAge() const noexcept   { return noteAge; }
    float         getLastPos() const noexcept   { return lastPos; }
    int           getLastLevel() const noexcept { return lastLevel; }
    int           getLastFrame() const noexcept { return lastFrame; }
    int           getLastNote() const noexcept  { return lastNote; }    // dispNote (D-X): -1 before any note
    double        getCurrentHz() const noexcept { return hz; }
    bool          wasStartedFromIdle() const noexcept { return startedFromIdle; }
    float         getLastModEnv() const noexcept { return lastMenv; }   // dispMenv
    float         getLastAmpEnv() const noexcept { return lastAmp; }    // dispAmp

   #if OSIW_TEST_HOOKS
    // Test hooks (console targets only).
    bool smootherBypass = false;                                         // negative control: no 2 ms pole
    float getEffPos() const noexcept { return effPos; }                  // effective position, last sample
    // effPos per in-buffer index of the last render(s) (stale where the
    // voice did not render). Sized maxBlock in prepareToPlay.
    const std::vector<float>& getEffPosTrace() const noexcept { return effTrace; }

    // 2.4 hooks. xfadeLenOverride >= 0 replaces the crossfade length
    // (0 = hard switch: the QUAL-03 negative control). forceLevel >= 0
    // replaces the mip level selection (QUAL-03 level-change exactness).
    int xfadeLenOverride = -1;
    int forceLevel = -1;
    bool isCrossfading() const noexcept { return xfActive; }
    int  getXfadeLen() const noexcept    { return xfadeLen; }
    int  getActiveLevel() const noexcept { return cfg.level; }

    // Gap-closure hooks (W1/W2 negative controls): false = the pre-fix
    // behaviour (hard stop to 0, instant velGain on a sounding retrigger).
    bool tailFadeEnabled = true;
    bool velRampEnabled  = true;
    bool isTailActive() const noexcept { return tailLeft > 0; }
    bool xfKeepRate = true;            // W4 negative control: false = frozen read at the live phase
   #endif

private:
    //==========================================================================
    static bool sameAdsr (const juce::ADSR::Parameters& a, const juce::ADSR::Parameters& b) noexcept
    {
        return juce::exactlyEqual (a.attack,  b.attack)
            && juce::exactlyEqual (a.decay,   b.decay)
            && juce::exactlyEqual (a.sustain, b.sustain)
            && juce::exactlyEqual (a.release, b.release);
    }

    void applyDeferredParams() noexcept
    {
        if (! sameAdsr (ampParams, appliedAmp))
        {
            ampEnv.setParameters (ampParams);
            appliedAmp = ampParams;
        }
        if (! sameAdsr (menvParams, appliedMenv))
        {
            modEnv.setParameters (menvParams);
            appliedMenv = menvParams;
        }
    }

    // W2. ramp = the voice is still sounding; otherwise (from idle) instant.
    void setVelocityGain (float target, bool ramp) noexcept
    {
       #if OSIW_TEST_HOOKS
        if (! velRampEnabled)
            ramp = false;
       #endif
        velTarget = target;
        if (ramp && velRampLen > 1 && ! juce::exactlyEqual (velGain, target))
        {
            velStep     = (target - velGain) / (float) velRampLen;
            velRampLeft = velRampLen;                   // velGain reaches target on the last step
        }
        else
        {
            velGain     = target;
            velRampLeft = 0;
        }
    }

    // W1. Called on a hard stop while the amp env is active: the heard value
    // (this voice's last output plus any tail still decaying) becomes the
    // start of a fresh tail.
    void startTail() noexcept
    {
       #if OSIW_TEST_HOOKS
        if (! tailFadeEnabled)
        {
            lastOut = 0.0f;
            return;
        }
       #endif
        const float running = tailLeft > 0 ? tailVal * tailWeight (tailPos, tailLen) : 0.0f;
        const float start = lastOut + running;
        lastOut = 0.0f;
        if (! std::isfinite (start) || start == 0.0f)
        {
            tailLeft = 0;
            return;
        }
        tailVal  = start;
        tailPos  = 0;
        tailLeft = tailLen;
    }

    void renderTail (float* dst, int startSample, int numSamples) noexcept
    {
        const int n = numSamples < tailLeft ? numSamples : tailLeft;
        for (int i = 0; i < n; ++i)
            dst[startSample + i] += tailVal * tailWeight (tailPos++, tailLen);
        tailLeft -= n;
    }

    // Raised-cosine decay 1 -> 0 over len samples (zero slope at both ends).
    static float tailWeight (int pos, int len) noexcept
    {
        return (float) (0.5 + 0.5 * std::cos (juce::MathConstants<double>::pi * (double) pos / (double) len));
    }

    // Voice lifetime ends with the AMP env only (FUNC-06). The mod env is
    // reset with it, even mid-release.
    void endVoice() noexcept
    {
        releasing = false;
        modEnv.reset();
        clearCurrentNote();
    }

    void updatePitch() noexcept
    {
        const double bendSemis = ((double) pitchWheelPos - 8192.0) / 8192.0 * 2.0;   // +/-2 st
        hz = noteHz * std::pow (2.0, bendSemis / 12.0);
        const double r = hz / sr;
        inc = (r >= 0.0) ? (r < 0.49 ? r : 0.49) : 0.0;  // f clamped below 0.5 fs (NaN -> 0)
    }

    // D-K: strict level on the way UP (immediate, alias safety); on the way
    // DOWN only once the pitch is a quarter-tone below the boundary. A fresh
    // note, a bandlimit toggle or band-limit Off take the strict level.
    int selectLevelWithHysteresis (bool bandlimit) const noexcept
    {
        const int strict = wt::selectLevel (hz, sr, bandlimit);
        if (cfgFresh || ! bandlimit || ! cfg.bandlimit || strict >= cfg.level)
            return strict;

        // f * 2^(1/24) still needs the current level -> stay. Otherwise take
        // that (still alias-safe: >= strict) level.
        const int down = wt::selectLevel (hz * kLevelHysteresis, sr, true);
        return down < cfg.level ? down : cfg.level;
    }

    // Recomputed at the top of EVERY render call (Synthesiser sub-blocks,
    // mono sub-ranges after a pitch change).
    void refreshReadConfig() noexcept
    {
        ReadCfg next;
        next.bank      = ctx->bank;
        next.level     = selectLevelWithHysteresis (ctx->bandlimit);
        next.interp    = ctx->interp;
        next.bandlimit = ctx->bandlimit;

       #if OSIW_TEST_HOOKS
        if (forceLevel >= 0)
            next.level = juce::jlimit (0, WavetableBank::kLevels - 1, forceLevel);
       #endif

        if (cfgFresh)
        {
            cfg = next;              // startNote: adopt with no capture
            cfgFresh = false;
            xfActive = false;        // a fresh note never inherits a fade
            return;
        }

        if (! next.sameAs (cfg))
        {
            checkConfig (cfg, next);

            // A stale latchedFrame from a larger bank would be out of range
            // (readSample clamps anyway); re-latch on bank or interp change.
            if (next.bank != cfg.bank || next.interp != cfg.interp)
                needsLatch = true;

            cfg = next;              // the old bank is never dereferenced again by this voice
        }
    }

    // Frozen-cycle trigger (RESEARCH 4). Captures the cycle heard right now
    // (old config; folded with the running fade, if any) and restarts the
    // fade. Capture ONLY from oldCfg.bank = this voice's config of the
    // previous render = the bank the reaper's audioHeldBank protects.
    void checkConfig (const ReadCfg& oldCfg, const ReadCfg& newCfg) noexcept
    {
        juce::ignoreUnused (newCfg);

        int len = xfadeLen;
       #if OSIW_TEST_HOOKS
        if (xfadeLenOverride >= 0)
            len = xfadeLenOverride;
       #endif
        if (len <= 0)
        {
            xfActive = false;        // hard switch (test-only negative control)
            return;
        }

        const WavetableBank* ob = oldCfg.bank;
       #if OSIW_TEST_HOOKS
        WavetableBank::testNoteDeref (ob);
       #endif
        const bool liveSilent = ob == nullptr || ob->numFrames <= 0;

        const bool  folding = xfActive;
        const float w       = folding ? xfWeight (xfPos, xfLenActive) : 1.0f;
        const float* cur    = frozen[frozenActive];
        const bool  curSilent = frozenSilent[frozenActive];
        float* dst          = frozen[1 - frozenActive];
        const float capPos  = effPos;                 // position read by the last rendered sample
        bool keepRate = true;
       #if OSIW_TEST_HOOKS
        keepRate = xfKeepRate;
       #endif
        // W4: a running frozen cycle is read at xfPhase, the live one at phase.
        // Fold it phase-aligned (offset 0 when the rates never differed: exact).
        const double curOffset = (folding && keepRate) ? xfPhase - phase : 0.0;

        for (int i = 0; i < WavetableBank::kTableSize; ++i)
        {
            const double ph = (double) i / (double) WavetableBank::kTableSize;
            const float liveOld = liveSilent ? 0.0f
                                             : wt::readSample (*ob, oldCfg.level, ph, oldCfg.interp, capPos, latchedFrame);
            float curVal = 0.0f;
            if (folding && ! curSilent)
            {
                double cp = ph + curOffset;
                cp -= std::floor (cp);
                curVal = juce::exactlyEqual (curOffset, 0.0) ? cur[i] : readFrozen (cur, cp);
            }
            dst[i] = folding ? (1.0f - w) * curVal + w * liveOld   // FOLD, never drop
                             : liveOld;
        }
        xfPhase = phase;
        xfInc   = keepRate ? renderedInc : inc;
        dst[WavetableBank::kTableSize] = dst[0];      // guard

        frozenSilent[1 - frozenActive] = liveSilent && (! folding || curSilent);
        frozenActive = 1 - frozenActive;
        xfActive     = true;
        xfPos        = 0;
        xfLenActive  = len;                           // running length: set only where the counter restarts
    }

    // Crossfade weight of the new config after pos of len samples: a
    // raised-cosine (equal-gain) ramp 0 -> 1. A linear ramp has a slope
    // corner at both ends, a second-difference kick of |A - B| / len that
    // clicks against smooth low notes (QUAL-03 ratio tier at A1, Imported ->
    // empty); the raised cosine starts and ends with zero slope.
    static float xfWeight (int pos, int len) noexcept
    {
        return (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * (double) pos / (double) len));
    }

    // The voice's read arithmetic on a captured 2049-sample cycle.
    static float readFrozen (const float* fr, double phase) noexcept
    {
        double ph = phase;
        if (! (ph >= 0.0) || ! (ph < 1.0))
            ph = 0.0;
        const double idx = ph * (double) WavetableBank::kTableSize;
        int i0 = (int) idx;
        i0 = i0 < WavetableBank::kTableSize - 1 ? i0 : WavetableBank::kTableSize - 1;
        const float t = (float) (idx - (double) i0);
        return wt::lerpCycle (fr, i0, t);
    }

    //==========================================================================
    const BlockContext* ctx = nullptr;
    double sr = 44100.0;

    juce::ADSR ampEnv;
    juce::ADSR::Parameters ampParams, appliedAmp;
    juce::ADSR modEnv;
    juce::ADSR::Parameters menvParams, appliedMenv;
    bool releasing = false;

    PositionSmoother smoother;
    bool  needsSeed = true;
    float effPos    = 0.0f;
    float lastMenv  = 0.0f;
    float lastAmp   = 0.0f;

    BitQuantizer quant;

    double noteHz = 440.0;
    double hz     = 440.0;
    double inc    = 0.0;
    double phase  = 0.0;
    int pitchWheelPos = 8192;   // centre
    float velGain = 0.0f;
    float velTarget = 0.0f;                 // W2 velocity ramp
    float velStep   = 0.0f;
    int   velRampLeft = 0;
    int   velRampLen  = 1;                  // round(0.003 * fs), set in prepareToPlay

    float lastOut  = 0.0f;                  // W1: this voice's last rendered output sample
    float tailVal  = 0.0f;
    int   tailPos  = 0;
    int   tailLeft = 0;
    int   tailLen  = 1;                     // round(0.002 * fs), set in prepareToPlay

    ReadCfg cfg;
    bool cfgFresh   = true;
    bool needsLatch = true;
    int  latchedFrame = 0;

    // Frozen-cycle crossfader (preallocated: part of the voice object).
    float frozen[2][WavetableBank::kStride] {};
    bool  frozenSilent[2] { true, true };   // captured from an empty bank: contributes exact 0
    int   frozenActive = 0;
    bool  xfActive     = false;
    int   xfPos        = 0;
    int   xfLenActive  = 1;
    int   xfadeLen     = 1;                 // round(0.005 * fs), set in prepareToPlay
    double xfPhase = 0.0;                   // W4: the frozen cycle's own phase ...
    double xfInc   = 0.0;                   // ... and rate (= the rate it was heard at)
    double renderedInc = 0.0;               // inc of the last rendered sample

    bool startedFromIdle = true;
    std::uint64_t noteAge = 0;
    float lastPos   = 0.0f;
    int   lastLevel = 0;
    int   lastFrame = 0;
    int   lastNote  = -1;                   // last startNote / noteOnDirect / setPitchNote note (D-X)

   #if OSIW_TEST_HOOKS
    std::vector<float> effTrace;
   #endif

    static inline std::atomic<std::uint64_t> sNoteCounter { 0 };
};
