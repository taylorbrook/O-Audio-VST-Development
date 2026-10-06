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

    O-simpleWavetable - Voice + Sound (Stage 2.2)
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

    Lifetime follows the AMP envelope only (FUNC-06).

    Seams for later phases:
      - 2.3: BlockContext lfo / lfoDepth / envAmount already summed into the
        position (they point at zero buffers in 2.2); mod env + smoother.
      - 2.4: checkConfig() is called on every read-config change (bank
        pointer, level, interp, bandlimit). 2.2 = hard switch (empty body).

    Do NOT declare a method named isVoiceActive: that is the Synthesiser's
    allocation virtual; shadowing it would hijack voice stealing.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <cmath>
#include <cstdint>

#include "BitQuantizer.h"
#include "WavetableBank.h"
#include "WtRead.h"

//==============================================================================
// Filled by the processor before any voice renders in a block (D-H).
struct BlockContext
{
    const float* knobPos   = nullptr;   // [preparedBlock] 20 ms smoothed position knob
    const float* lfo       = nullptr;   // [preparedBlock] global LFO -1..1      (2.3; zeros in 2.2)
    const float* lfoDepth  = nullptr;   // [preparedBlock] smoothed lfo_depth    (2.3; zeros in 2.2)
    const float* envAmount = nullptr;   // [preparedBlock] smoothed env_amount   (2.3; zeros in 2.2)
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
    // dispatches here directly. setSampleRate BEFORE setParameters (jassert).
    void prepareToPlay (double newSampleRate, int /*maxBlock*/, const juce::ADSR::Parameters& amp)
    {
        setCurrentPlaybackSampleRate (newSampleRate);
        sr = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        ampEnv.setSampleRate (sr);
        ampEnv.setParameters (amp);
        ampParams  = amp;
        appliedAmp = amp;
        ampEnv.reset();
        releasing = false;

        phase = 0.0;
        cfgFresh = true;
        needsLatch = true;
        updatePitch();
        clearCurrentNote();
    }

    // Once per block from the processor. Dirty-checked; never while releasing
    // (setParameters recomputes the release rate from SUSTAIN and with
    // sustain 0 resets the envelope mid-tail). Deferred params apply at note-on.
    void setBlockParams (const juce::ADSR::Parameters& amp) noexcept
    {
        ampParams = amp;
        if (! releasing && ! sameAdsr (amp, appliedAmp))
        {
            ampEnv.setParameters (amp);
            appliedAmp = amp;
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
        pitchWheelPos = currentPitchWheelPosition;      // SEED the wheel at note-on
        updatePitch();

        const float v = velocity >= 0.0f ? (velocity <= 1.0f ? velocity : 1.0f) : 0.0f;   // NaN -> 0
        velGain = v * v;                                // squared velocity (CONTEXT)
        noteAge = ++sNoteCounter;

        phase      = 0.0;
        needsLatch = true;
        cfgFresh   = true;                              // adopt the config with no capture (2.4)

        applyDeferredParams();
        releasing = false;
        ampEnv.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            releasing = true;
            ampEnv.noteOff();
            if (! ampEnv.isActive())
            {
                releasing = false;
                clearCurrentNote();
            }
        }
        else
        {
            ampEnv.reset();
            releasing = false;
            clearCurrentNote();
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
        if (! ampEnv.isActive())
        {
            if (getCurrentlyPlayingNote() >= 0)
            {
                releasing = false;
                clearCurrentNote();
            }
            return;
        }

        if (ctx == nullptr || numSamples <= 0 || out.getNumChannels() <= 0)
            return;

        refreshReadConfig();
        quant.setChoiceIndex (ctx->bitDepthIndex);

        const WavetableBank* b = cfg.bank;
        const int nF = b != nullptr ? b->numFrames : 0;
        const float gain = velGain * kVoiceGain;
        float* dst = out.getWritePointer (0);           // mono into channel 0; processor copies

        const float* knob  = ctx->knobPos;
        const float* lfo   = ctx->lfo;
        const float* depth = ctx->lfoDepth;

        float pos = lastPos;
        for (int i = 0; i < numSamples; ++i)
        {
            const int j = startSample + i;
            const float env = ampEnv.getNextSample();

            // 2.2: knob (+ 0 from the zero LFO buffers). 2.3 adds amt * modEnv
            // and the per-voice smoother.
            pos = wt::clamp01 (knob[j] + depth[j] * 0.5f * lfo[j]);

            if (needsLatch)
            {
                latchedFrame = wt::latchFrame (pos, nF);
                needsLatch = false;
            }

            float s = 0.0f;
            if (nF > 0)
                s = quant.apply (wt::readSample (*b, cfg.level, phase, cfg.interp, pos, latchedFrame));

            dst[j] += s * env * gain;

            phase += inc;
            if (phase >= 1.0)
            {
                phase -= 1.0;
                if (! cfg.interp)
                    latchedFrame = wt::latchFrame (pos, nF);
            }

            if (! ampEnv.isActive())
                break;
        }

        lastPos   = pos;
        lastLevel = cfg.level;
        lastFrame = cfg.interp ? wt::latchFrame (pos, nF) : latchedFrame;

        if (! ampEnv.isActive())
        {
            releasing = false;
            clearCurrentNote();
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
        updatePitch();

        if (retrigger)
        {
            const float v = velocity >= 0.0f ? (velocity <= 1.0f ? velocity : 1.0f) : 0.0f;
            velGain = v * v;
            if (! wasSounding)
            {
                phase      = 0.0;
                needsLatch = true;
                cfgFresh   = true;
            }
            startedFromIdle = ! wasSounding;
            noteAge = ++sNoteCounter;
            applyDeferredParams();
            releasing = false;
            ampEnv.noteOn();
        }
    }

    // Fallback to a still-held note: pitch only (no retrigger, velocity kept).
    void setPitchNote (int midiNote)
    {
        noteHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
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
    double        getCurrentHz() const noexcept { return hz; }
    bool          wasStartedFromIdle() const noexcept { return startedFromIdle; }

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
    }

    void updatePitch() noexcept
    {
        const double bendSemis = ((double) pitchWheelPos - 8192.0) / 8192.0 * 2.0;   // +/-2 st
        hz = noteHz * std::pow (2.0, bendSemis / 12.0);
        const double r = hz / sr;
        inc = (r >= 0.0) ? (r < 0.49 ? r : 0.49) : 0.0;  // f clamped below 0.5 fs (NaN -> 0)
    }

    // Recomputed at the top of EVERY render call (Synthesiser sub-blocks,
    // mono sub-ranges after a pitch change).
    void refreshReadConfig() noexcept
    {
        ReadCfg next;
        next.bank      = ctx->bank;
        next.level     = wt::selectLevel (hz, sr, ctx->bandlimit);
        next.interp    = ctx->interp;
        next.bandlimit = ctx->bandlimit;

        if (cfgFresh)
        {
            cfg = next;              // startNote: adopt with no capture
            cfgFresh = false;
            return;
        }

        if (! next.sameAs (cfg))
        {
            checkConfig (cfg, next);

            // A stale latchedFrame from a larger bank would be out of range
            // (readSample clamps anyway); re-latch on bank or interp change.
            if (next.bank != cfg.bank || next.interp != cfg.interp)
                needsLatch = true;

            cfg = next;
        }
    }

    // 2.4 fills this (frozen-cycle 5 ms crossfade from the OLD config).
    // 2.2: hard switch.
    void checkConfig (const ReadCfg& oldCfg, const ReadCfg& newCfg) noexcept
    {
        juce::ignoreUnused (oldCfg, newCfg);
    }

    //==========================================================================
    const BlockContext* ctx = nullptr;
    double sr = 44100.0;

    juce::ADSR ampEnv;
    juce::ADSR::Parameters ampParams, appliedAmp;
    bool releasing = false;

    BitQuantizer quant;

    double noteHz = 440.0;
    double hz     = 440.0;
    double inc    = 0.0;
    double phase  = 0.0;
    int pitchWheelPos = 8192;   // centre
    float velGain = 0.0f;

    ReadCfg cfg;
    bool cfgFresh   = true;
    bool needsLatch = true;
    int  latchedFrame = 0;

    bool startedFromIdle = true;
    std::uint64_t noteAge = 0;
    float lastPos   = 0.0f;
    int   lastLevel = 0;
    int   lastFrame = 0;

    static inline std::atomic<std::uint64_t> sNoteCounter { 0 };
};
