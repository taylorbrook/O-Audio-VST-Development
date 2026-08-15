/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

namespace oo
{

/**
    FUNC-04 — the verify ping. "Is speaker 5 actually speaker 5?", answered by ear.

    ── IT IS A POST-WRITE OVERWRITE, NOT A TERM IN THE GAIN PATH (§7.2 / §OQ2) ────────────────────
    It sits at the END of GainStage::renderChunk()'s REAL arm, after the sample loop and after the
    NaN guard, indexed through the SAME `out[]` pointers — which are `snapshot.speakerToBuffer`.
    That placement is the whole point: bypassing DBAP, the weights, the hull trim, the air filter and
    `outputGain` means a ping that comes out of the wrong speaker has EXACTLY ONE possible cause.
    Every other channel is hard-zeroed, so "exactly one lane sounds" is a property of the routine
    rather than of the material.

    ── NO reset() ANYWHERE, AND THAT PRESERVES P23/P30's ONE-RESET-SITE-EVER ─────────────────────
    Two reasons, and neither is an oversight:

      * GainStage's per-sample loop advances all 17 smoothers UNCONDITIONALLY, so overwriting the
        output leaves every one of them running. On ping stop the DBAP signal resumes from smoothers
        that never froze — there is nothing to teleport back to.
      * The 20 ms raised-cosine envelope below already owns the discontinuity at both ends. This is
        Phase 2.3's H1 argument in a different place: an envelope that reaches zero makes a state
        reset unnecessary, and a reset is the thing that would INTRODUCE a click, by discarding a
        partly-faded tail.

    `prepare()` is this class's single initialisation site, exactly as GainStage::prepare() is its.

    ── BOTH CLOCKS ARE COUNTED IN SAMPLES (P60) ──────────────────────────────────────────────────
    1.2 s on, 0.4 s gap, and the 120 s latch — all derived from the prepared sample rate. Three
    reasons and the third is decisive: sample counting is deterministic, it is block-size invariant
    because chunks tile a block exactly, and IT IS THE ONLY FORM PROBES BS AND BT CAN MEASURE
    OFFLINE. A juce::Timer is unmeasurable in a render harness.

    ── THREADING ─────────────────────────────────────────────────────────────────────────────────
    Commands in and state out through ATOMICS ONLY; no juce::String ever crosses a thread (P43). The
    cycle itself is owned by the AUDIO thread, which is what makes D14 true — the editor renders
    `getState().speaker` and never re-derives the step. A drifted setInterval on the page would name
    speaker 5 while 6 sounds, during the one procedure whose entire purpose is the opposite.

    ── NOT PART OF THE BIT-IDENTITY CONTRACT (§F9) ───────────────────────────────────────────────
    A UI action, never automated, and every QUAL-03 / bit-identity probe runs with it off. The
    `juce::Random` below is MEMBER-OWNED rather than `getSystemRandom()`, which is free and removes
    the question entirely (pattern_rng_stream_interleave_blocksize).
*/
class VerifyPing
{
public:
    //==============================================================================
    static constexpr int kNumSpeakers = 8;

    /** `start()` targets: 1..kNumSpeakers for a latched single speaker, or this for the 1->8 cycle. */
    static constexpr int kAuto = -1;

    // ── §OQ2's timings. Seconds here, SAMPLES at prepare(). ──────────────────────────────────────
    static constexpr double kOnSeconds    = 1.2;
    static constexpr double kGapSeconds   = 0.4;
    static constexpr double kLatchSeconds = 120.0;
    static constexpr double kFadeSeconds  = 0.020;

    /** The whole auto cycle: 8 x (1.2 + 0.4) = 12.8 s. Named so probe BS can assert against THIS
        symbol multiplied by the prepared rate rather than against a transcribed 614400
        (pattern_test_fixture_mirrors_drift_silently). */
    static constexpr double kAutoCycleSeconds = kNumSpeakers * (kOnSeconds + kGapSeconds);

    // ── Level (§OQ2) ─────────────────────────────────────────────────────────────────────────────
    static constexpr float kTargetRmsDb = -20.0f;
    static constexpr float kPeakCeilDb  = -6.0f;

    /** RMS-normalising scalar for the pink network + 200 Hz HP + 8 kHz LP chain.

        CALIBRATED BY MEASUREMENT AT EXECUTE, NOT GUESSED. 4 x 10^5 samples of the exact chain, at
        44.1 / 48 / 96 kHz, gave a chain RMS of 1.09608 / 1.09697 / 1.09935 — a 0.3 % spread, i.e.
        0.02 dB, which is why ONE rate-independent constant is honest here. 0.1 / 1.09697 = 0.09116.

        Measured crest factor is 4.21 at 48 kHz, so the steady-state peak lands near -7.5 dBFS and
        the -6 dBFS clamp below is a CEILING rather than a limiter that is always working.

        PROBE BR ASSERTS THE RESULTING RMS, NOT THIS CONSTANT — a probe written against the constant
        would agree with any value it was given and measure nothing.
    */
    static constexpr float kPinkNormScalar = 0.09116f;

    //==============================================================================
    /** What the editor polls. POD ints and bools — no juce::String crosses a thread. */
    struct State
    {
        bool active      { false };
        int  mode        { 0 };   ///< 0 off, 1 manual/latched, 2 auto cycle
        int  speaker     { 0 };   ///< 1-based; 0 when nothing is sounding
        int  elapsedMs   { 0 };
        int  remainingMs { 0 };   ///< to the 120 s latch (manual) or to cycle end (auto)
    };

    //==============================================================================
    VerifyPing() = default;

    /** THE SINGLE INITIALISATION SITE. Message thread, from prepareToPlay(). */
    void prepare (double sampleRateToUse);

    //==============================================================================
    // ── Commands. Message thread, except abort() which is callable from either. ──

    /** @param speakerOrAuto  1..8, or kAuto. Out-of-range values are ignored. */
    void start (int speakerOrAuto);

    /** GRACEFUL: the audio thread runs the 20 ms release, then goes idle. */
    void stop();

    /** IMMEDIATE, and callable from EITHER thread because it touches only atomics.

        Three callers, all of them D11 stops that cannot wait for a fade: the editor destructor, the
        `processBlockBypassed()` override, and the audio thread when `mapped` goes false mid-ping. A
        bypassed plugin still emitting noise is a genuinely confusing thing to debug on a stage —
        the first instinct is to bypass, and if that does not silence it the diagnosis goes somewhere
        wrong.
    */
    void abort();

    //==============================================================================
    /** The processor's guard before calling overwrite(). Becomes true on start() IMMEDIATELY, so the
        audio thread is called and can consume the command. */
    bool isActive() const noexcept { return activeFlag.load (std::memory_order_acquire); }

    /** Message thread. */
    State getState() const noexcept;

    //==============================================================================
    /** AUDIO THREAD. Writes the target lane and HARD-ZEROES the other seven.

        @param out           out[i] is ALREADY speaker (i+1)'s buffer — the caller has resolved it
                             through snapshot.speakerToBuffer, which is what makes this test the map
        @param numSpeakers   always kNumSpeakers; passed so the loop bound is the caller's
        @param start,count   the chunk, exactly as renderChunk() received it
    */
    void overwrite (float* const* out, int numSpeakers, int start, int count) noexcept;

private:
    //==============================================================================
    enum Command { kCmdNone = 0, kCmdStop = -2, kCmdAbort = -3 };
    enum class Phase { idle, fadeIn, sustain, fadeOut, gap };

    float nextSample() noexcept;
    void  beginRun (int target) noexcept;
    void  publish() noexcept;

    //==============================================================================
    // ── Cross-thread surface ────────────────────────────────────────────────────────────────────
    std::atomic<int>  command     { kCmdNone };
    std::atomic<bool> activeFlag  { false };
    std::atomic<int>  modeOut     { 0 };
    std::atomic<int>  speakerOut  { 0 };
    std::atomic<int>  elapsedMsOut   { 0 };
    std::atomic<int>  remainingMsOut { 0 };

    //==============================================================================
    // ── Audio-thread private. Never touched from the message thread; abort() deliberately does
    //    NOT reach in here, which is what makes it race-free from either side. ───────────────────
    double sampleRate { 0.0 };

    int onSamples    { 0 };
    int gapSamples   { 0 };
    int latchSamples { 0 };
    int fadeSamples  { 0 };

    Phase phase        { Phase::idle };
    int   phaseCounter { 0 };
    int   speaker      { 0 };
    int   mode         { 0 };
    int   runCounter   { 0 };   ///< samples since this run started — the 120 s / 12.8 s clock

    // ── The signal (§OQ2) ───────────────────────────────────────────────────────────────────────
    juce::Random rng;                                  // MEMBER-OWNED, never getSystemRandom()
    float b0 { 0.0f }, b1 { 0.0f }, b2 { 0.0f };       // fixed-coefficient pinking network

    juce::dsp::FirstOrderTPTFilter<float> hp, lp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VerifyPing)
};

} // namespace oo
