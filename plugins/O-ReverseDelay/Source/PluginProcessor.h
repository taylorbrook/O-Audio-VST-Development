/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
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

#include <JuceHeader.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

// Set to 1 ONLY by tests/render-harness/CMakeLists.txt. Under the harness the
// per-instance RNG seed collapses back to v1.0's literal so every probe is
// reproducible; in a real build each instance seeds from its own hash so two
// tracks do not correlate. Defaulted here so the plugin build needs no flag.
#ifndef OUARICON_RENDER_HARNESS
 #define OUARICON_RENDER_HARNESS 0
#endif

#include "dsp/CaptureBuffer.h"
#include "dsp/GrainScheduler.h"
#include "dsp/ReverseGrain.h"
#include "dsp/WindowLut.h"

// Header-only, no WebView dependency — safe under the harness' JUCE_WEB_BROWSER=0.
#include "OuariconPresetManager.h"

// O-ReverseDelay — granular reverse delay (Stage 2 DSP, Phase 2.3 complete:
// reverse wet path + damped tanh-stable feedback loop + tempo sync + width).
// APVTS with 25 parameters: the 10 of research/ARCHITECTURE.md's immutable
// contract, plus v1.1.0's four grain randomisations (B3), v1.2.0's two window
// controls (B1), v1.3.0's overlap ceiling (B2), v1.4.0's Tukey taper,
// v1.6.0's three MOTION controls (B4 #1-#3) and v1.7.0's four SOURCE/DUCK/DRIFT
// controls (B4 #4-#6), all of which are ADDITIVE — every one defaults to the
// engine's no-op, so the contract's behaviour is the default behaviour.
// NOTE: this file (and PluginProcessor.cpp) must stay free of editor-only includes —
// the render harness compiles the processor without any editor sources.
class ReverseDelayProcessor : public juce::AudioProcessor
{
public:
    ReverseDelayProcessor();
    ~ReverseDelayProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    /** v1.0.1 (C): hosts calling reset() between transport passes previously left
        the capture ring, grain pool and filter states populated, so a stale reverse
        tail survived into the next pass. */
    void reset() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    //==========================================================================
    /** v1.9.0 — the hover-help language. 0 = en, 1 = fr.

        An INDEX rather than a string because std::atomic<juce::String> does not
        compile — juce::String is not trivially copyable — so the audio-safe
        form is an index behind the two-function codec below while the PERSISTED
        form stays a language code.

        Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
        automation lane, and a preset must not be able to change which language
        somebody reads their help in. It rides the APVTS state tree as a
        non-parameter property, which the JSON preset path never touches.

        Public so the editor's getUiLanguage/setUiLanguage native functions can
        reach it, matching how presetManager is exposed. */
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

    /** Stage 4: preset library access for the editor's 10 preset native functions
        and for the render harness' probe N factory audit. */
    OuariconPresetManager& getPresetManager() noexcept { return presetManager; }

    /** Live concurrent-grain count, read STRAIGHT off the pool.

        HARNESS ONLY. GrainPool::countActive() walks 32 `bool`s that the audio
        thread writes, so calling this from the message thread is a data race —
        benign in practice but formally UB, and v1.1.0's doc comment was too
        relaxed about it ("fine for a torn-value-tolerant UI meter"). It is not
        what the v1.3.0 UI meter uses; getGrainMeter() is. The render harness is
        single-threaded, so here it is exact and probe Y/AB keep using it. */
    int getActiveGrainCount() const noexcept { return grainPool.countActive(); }

    //==========================================================================
    // v1.3.0 (B2) — the grain meter.

    /** A snapshot of what the engine is actually doing, for the UI readout.

        `active` is the PEAK concurrency across the last processed block's passes
        (not the end-of-block instant, which at low density lands between spawns
        and reads low), and `overlap` is that block's effective overlap. Both are
        published by processBlock through relaxed atomics and read here — the
        message thread never touches pool or scheduler state. */
    struct GrainMeter
    {
        int   active  = 0;
        float overlap = 0.0f;
    };

    GrainMeter getGrainMeter() const noexcept
    {
        return { publishedActiveGrains.load (std::memory_order_relaxed),
                 publishedOverlap    .load (std::memory_order_relaxed) };
    }

    /** Cumulative spawn requests the scheduler's fixed array could not hold, and
        spawns GrainPool::obtain() refused for want of a free slot.

        These are two different events and only the second is a design choice:
        a refusal is v1.1.0 deliberately dropping one grain out of a wash rather
        than cutting a live envelope, while a DROP is the scheduler failing to
        report work it decided to do. v1.0–v1.2 did the latter silently
        (GrainScheduler.h's cap), which was safe only because the shipped ranges
        could not reach it — a guarantee nothing measured. Both counters exist so
        probe AB can assert the drop count is zero at the worst case the
        parameters allow, and report the refusal count instead of guessing it. */
    juce::uint32 getDroppedSpawnCount() const noexcept
    {
        return droppedSpawns.load (std::memory_order_relaxed);
    }

    juce::uint32 getRefusedSpawnCount() const noexcept
    {
        return refusedSpawns.load (std::memory_order_relaxed);
    }

    /** Zeroes both counters. Message-thread/harness only — they are cumulative
        across the processor's life, so a probe measuring one configuration must
        clear first or it inherits every earlier probe's total. */
    void resetSpawnCounters() noexcept
    {
        droppedSpawns.store (0, std::memory_order_relaxed);
        refusedSpawns.store (0, std::memory_order_relaxed);
    }

    /** v1.2.0 (B1): the window bank, exposed read-only so the render harness can
        PRINT the per-shape power duty cycles and normalisation constants it is
        asserting against. A level-match probe that derives its own expected
        numbers from the same header it is testing proves nothing; a probe that
        reports the constants alongside a measured RMS is auditable. */
    const WindowLut& getWindowLuts() const noexcept { return windowLuts; }

    //==========================================================================
    // delayTime range constants (v1.0.1 / A1).
    //
    // These are shared by createParameterLayout(), the tempo-sync clamp in
    // processBlock() and the user-preset migration — the v1.0.0 defect was a
    // literal 2000.0 in the sync clamp drifting from the parameter's own max,
    // so there is exactly one definition now.
    static constexpr float kDelayTimeMinMs         =   50.0f;
    static constexpr float kDelayTimeMaxMs         = 4000.0f;
    static constexpr float kDelayTimeSkewCentreMs  =  316.0f;

    /** v1.0.0's delayTime max. Preset JSON stores NORMALISED fractions, so a
        preset written against this max recalls a different number of ms once the
        max moves — see migrateUserPresets(). */
    static constexpr float kLegacyDelayTimeMaxMs   = 2000.0f;

    //==========================================================================
    // grainSize range + v1.1.0 randomisation constants.
    //
    // The grainSize endpoints are named because sizeRandom clamps each grain's
    // latched G back into them — a randomised grain must never be longer than a
    // grain the user could dial in by hand, or the ring requirement below stops
    // being true. Same single-definition discipline as kDelayTimeMaxMs (the
    // v1.0.0 A1 defect was a literal 2000.0 drifting from the parameter range).
    //
    // ── v1.5.0: max 500 -> 4000 ms ───────────────────────────────────────────
    // The knob now spans the SAME 50–4000 ms as delayTime, and deliberately
    // carries delayTime's skew centre too (316 ms): the two long-throw time
    // knobs sit next to each other on the panel, and a shared taper means a
    // given knob angle reads as roughly the same duration on both.
    //
    // Two things follow from this move and neither is optional:
    //   * kCaptureSeconds grows, because the ring requirement below is
    //     gD_max + 2·G_max and G_max just octupled. See kCaptureSeconds.
    //   * User presets must be rescaled, because preset JSON stores NORMALISED
    //     fractions and BOTH endpoints and skew moved. See migrateUserPresets()
    //     and the kLegacyGrainSize* constants.
    //
    // NOT affected, recorded here because each looks adjacent and is not:
    //   * GrainScheduler::kMaxSpawnsPerBlock — its bound is
    //     overlapMax·kDelayTimeMinMs/kGrainSizeMinMs, which keys off grainSize's
    //     MINIMUM. Unmoved, so the cap's 8x margin is untouched.
    //   * GrainPool's 32 slots — a grain lives G samples and the spawn interval
    //     is G/overlap, so concurrent grains ~= overlap regardless of G.
    //   * loopCountTrim — a function of overlap only.
    //   * ReverseGrain — latches an int G; it owns no buffer to resize.
    static constexpr float kGrainSizeMinMs         =   50.0f;
    static constexpr float kGrainSizeMaxMs         = 4000.0f;
    static constexpr float kGrainSizeSkewCentreMs  =  316.0f;

    /** v1.0.0–v1.4.0's grainSize range. Preset JSON stores NORMALISED fractions,
        so a preset written against these recalls a different number of ms once
        the range moves — and unlike the delayTime migration, the SKEW moved too,
        so this is doubly not a linear rescale. See migrateUserPresets(). */
    static constexpr float kLegacyGrainSizeMaxMs        = 500.0f;
    static constexpr float kLegacyGrainSizeSkewCentreMs = 158.0f;

    /** delayScatter's max, in ms. Also the ring's positive-scatter budget. */
    static constexpr float kDelayScatterMaxMs      = 500.0f;

    /** Largest fraction of unity that gainRandom may add to or remove from a
        grain's gain. Bounded well under 1.0 so a randomised grain can never be
        silent (mul 0) or double-level (mul 2) — both read as faults, not depth. */
    static constexpr float kMaxGainRandomDeviation = 0.75f;

    //==========================================================================
    // v1.2.0 (B1) — grain window tilt.
    //
    // grainTilt is 0..1 with 0.5 = symmetric; the ENGINE wants a peak position
    // inside the grain. The map is deliberately written as
    //     peakPos = 0.5 + (tilt - 0.5) · kTiltTravel
    // rather than the equivalent-looking `lo + tilt·(hi - lo)`, because only
    // this form returns EXACTLY 0.5f at tilt = 0.5f: the bracket is exactly zero
    // and 0.5f + 0.0f is 0.5f. The other form goes through a rounded multiply
    // and lands one ulp away, which would silently cost the bitwise-identity
    // guarantee that makes this a MINOR bump (probe Z1).
    //
    // 0.9 travel spans peak positions [0.05, 0.95], matching WindowLut's own
    // clamp — so the parameter reaches its endpoints exactly rather than being
    // clipped somewhere short of them.
    static constexpr float kTiltTravel = 0.9f;

    static float tiltToPeakPos (float tilt01) noexcept
    {
        return 0.5f + (tilt01 - 0.5f) * kTiltTravel;
    }

    //==========================================================================
    // v1.3.0 (B2) — the overlap ceiling.
    //
    // Through v1.2.0 the density knob mapped to a HARD-CODED span:
    //     overlap = 2 + (density/100)·6            -> 2 … 8
    // and grain count was therefore not controllable at all, only inferable from
    // density and grain size. v1.3.0 makes the span's top an explicit parameter:
    //     overlap = kOverlapMin + (density/100)·(ceiling − kOverlapMin)
    //
    // At the default ceiling of 8, (8 − 2) is EXACTLY 6.0f in float, so the
    // expression is the same three operations on the same values that v1.0.1
    // shipped — bitwise identical, not merely equivalent. That is what keeps
    // every existing session and preset sounding exactly as it did: density is
    // stored denormalised (a session at 60 % recalls 60 %), so a remap of the
    // knob's own span would have re-voiced all of them. Probe AC asserts it.
    //
    // Why 16 and not more: GrainPool holds 32 slots, so a sustained overlap of
    // 16 leaves 2x for delay-time transitions and the v1.1 randomisations'
    // transient peaks. 32 would leave none, and the pool would start refusing
    // spawns in STEADY state rather than only under transients.
    static constexpr float kOverlapMin       =  2.0f;
    static constexpr float kOverlapCeilingMin =  2.0f;
    static constexpr float kOverlapCeilingMax = 16.0f;

    /** v1.2.0's fixed ceiling, and this release's default. Named because the
        compatibility guarantee keys off it: every overlap the shipped plugin
        could reach is at or below this value. */
    static constexpr float kLegacyOverlapMax = 8.0f;

    // ── Why grainGain needs NO extra term for the raised ceiling ──────────────
    //
    // The review predicted one, and it is worth writing down why it was wrong,
    // because the reasoning is sound and the conclusion is not: "overlapping
    // grains read the same reversed material at nearby offsets, so summing is
    // partially coherent, the real level rise sits between sqrt(N) and N, and
    // 1/sqrt(overlap) is not exactly level-neutral — with the error growing with
    // the ceiling." On that basis a fitted correction above 8 was expected here.
    //
    // The grains do read the same material. They do not read it at the same TIME.
    // A grain spawned at s reads source (s − D − n) at its own sample n, so at a
    // fixed OUTPUT sample t, grain k reads source (2kH − D − t) where H is the
    // spawn interval: consecutive grains read points 2H apart, and every pair in
    // the sum is separated by a multiple of 2H. For broadband input those points
    // are decorrelated, so the output path sums incoherently after all and
    // 1/sqrt(N) is exactly the right law.
    //
    // Measured, not argued — render-harness probe AA sweeps overlap 2 → 16 and
    // holds the wet level inside 0.07 dB with no correction term at all, against
    // the ±1 dB budget probes D and Z2 use. (The first version of that probe DID
    // show a 2.5 dB spread, which is what a coherence error looks like. It was
    // the harness: its shared excitation generator has ±0.077 autocorrelation at
    // lags 600–2400 samples, which is precisely where the spawn interval sits,
    // so each overlap setting was summing a different amount of correlation in
    // the TEST SIGNAL. The +1.45 dB outlier at overlap 10 sat exactly on the
    // generator's correlation peak at lag 960 — that setting's interval. With a
    // white excitation the spread is 0.07 dB. See whiteNoiseAt in main.cpp.)
    //
    // Where partial coherence IS real is the FEEDBACK path — and there it is not
    // a subtlety, it is a clipping defect. See kLoopCountTrimExponent.

    //==========================================================================
    // v1.3.0 (B2) — the LOOP's overlap compensation.
    //
    // This is the correction the review asked for, on the path the review did not
    // name. The output needed none (above); the loop needs one badly.
    //
    // What recirculates is not broadband input but this engine's own wash:
    // self-similar material that overlapping grains read at nearby offsets, so
    // the loop tap sums closer to COHERENTLY. A coherent sum of N grains rises
    // like N, not sqrt(N), so grainGain's 1/sqrt(N) — correct for the output —
    // leaves the loop with sqrt(N) of excess gain per generation. That was
    // harmless while N stopped at 8 and the topology still lost ~7 dB per
    // generation. Doubling the ceiling spends the whole margin:
    //
    //     measured decay at feedback 100, density 100, before this trim
    //       ceiling  8:  −0.29 dB/s      (decaying — the shipped behaviour)
    //       ceiling 10:  +0.87 dB/s      (GROWING)
    //       ceiling 12:  +0.87 dB/s
    //       ceiling 14:  +0.67 dB/s
    //       ceiling 16:  +0.46 dB/s      -> 90 s render peaked at 1.28, CLIPPED
    //
    // Positive dB/s is self-oscillation. The tanh bounds the LOOP to ±1 per
    // sample, but the wet output is a near-coherent sum of 16 grains each reading
    // loop content at the limiter's ceiling, and 1/sqrt(16) does not bound that —
    // hence a peak of 1.28 where every other probe in the suite asserts < 1.0.
    // The non-monotonicity above is the tanh compressing harder at higher
    // overlap, which is a symptom of the runaway rather than a mitigation of it.
    //
    // The fix rides on the split v1.2.0 already built for exactly this class of
    // problem: output and feedback-tap gains are separate members on every grain,
    // so a correction can be applied to one path only. 0.5 is the fully-coherent
    // exponent — (N/8)^−0.5 turns the loop's power law into an amplitude law
    // relative to the legacy ceiling — and the measurement confirms it, so it is
    // used as derived rather than tuned to a number that merely happens to work.
    //
    // Anchored at kLegacyOverlapMax and EXACTLY 1.0f at or below it, which is
    // what keeps the shipped feedback character bit-identical: probe D, the
    // −4.3 dB/generation figure in the v1.0.0 CHANGELOG and all eight factory
    // presets are all measurements of overlap <= 8, and none of them may move.
    //
    // Verified by three probes that fail in different ways, which matters because
    // the defect satisfied two of the three checks that existed before it:
    //   AF (decay-count-fb60/fb100) — the rate across ceilings, and every rate
    //       asserted NEGATIVE. A "difference from ceiling 8" bound alone passes
    //       when both configurations are growing.
    //   AG (ceiling16-loop-bounded) — 90 s at the worst case, asserting a
    //       MONOTONE decay. A runaway that saturated below 1.0 would satisfy a
    //       peak check while still being a runaway.
    //   AA (level-flat-count)       — that fixing the loop did not disturb the
    //       output level the loop feeds.
    static constexpr float kLoopCountTrimExponent = 0.5f;

    static float loopCountTrim (float overlap) noexcept
    {
        if (overlap <= kLegacyOverlapMax)
            return 1.0f;

        return std::pow (overlap / kLegacyOverlapMax, -kLoopCountTrimExponent);
    }

    //==========================================================================
    // v1.6.0 (B4 #2) — the direction blend.
    //
    // `direction` is the PROBABILITY, in percent, that a grain is latched
    // forward at spawn. 0 = every grain reverse (the shipped engine), 100 =
    // every grain forward, and the interval between is a genuine mix of the two
    // read laws rather than a crossfade between two renders.
    //
    // Default 0 is the engine's exact no-op in the strong sense this plugin has
    // used since v1.1.0: the draw that decides a grain's direction is GATED on
    // direction > 0, so at the default the shared xorshift is not touched, the
    // pan sequence is unmoved, and `readAbs += step` with step == −1 is the same
    // integer operation `--readAbs` was. Probe AM asserts bit-equality against a
    // defaults render rather than trusting either half of that.
    //
    // See WindowLut::getForwardNorm for why the forward half needs an output
    // trim and the feedback tap does not.
    static constexpr float kDirectionMaxPct = 100.0f;

    //==========================================================================
    // v1.6.0 (B4 #3) — the regeneration makeup.
    //
    // D11 declined this at v1.0 as a HIDDEN constant, and that decision stands:
    // what ships here is the same gain as a user control defaulting to 0 dB, so
    // every existing session, preset and factory patch is bit-for-bit unchanged
    // and the only thing that has appeared is a knob (see
    // pattern_activating_dead_param_default_timbre). Fifth release running whose
    // new parameter's no-op is a specific number; this is the first where that
    // number is also the range MINIMUM, which makes it the easy one.
    //
    // What it buys: the topology loses ≈7.3 dB per generation at width 0 —
    // −4.3 dB of Hann-squared duty plus −3.0 dB of the pan-to-mono-sum round
    // trip (probe AG's note: width is also a decay control here, costing 6 dB at
    // width 100 rather than 3) — so "Near-Infinite" at feedback 100 cannot in
    // fact self-sustain. It decays, slowly. This makes true sustain reachable
    // without moving anybody's default.
    //
    // ── The cap is a MEASURED number, not a chosen one ───────────────────────
    //
    // Probe AO renders a ladder in 1 dB steps at feedback 100, width 0 (the
    // loop's worst case — see probe AG), and the ceiling is read off it:
    //
    //   shipped ceiling (grainCount 8, density 65, Hann) — peak / decay dB/s
    //     0 dB 0.262/−2.09   1 dB 0.275/−0.65   2 dB 0.445/+0.53
    //     4 dB 0.760/+0.26   6 dB 0.834/+0.08  12 dB 0.860/+0.03
    //
    // Two things fall out, and 6 dB is where they meet:
    //
    //   * SUSTAIN arrives at 2 dB here, and needs roughly 3 dB more at width 100
    //     (the pan-to-mono-sum round trip costs 6 dB there against 3 at width 0).
    //     6 dB clears every configuration's threshold with margin.
    //   * Past ~6 dB the control STOPS DOING ANYTHING. The tanh is already
    //     limiting, so the decay rate plateaus at +0.03 dB/s and the peak at
    //     0.86 — 12 dB buys 0.026 dB/s over 6 dB. A knob whose top half is inert
    //     is a worse control than a shorter one.
    //
    // ── What the cap does NOT guarantee, stated plainly ──────────────────────
    //
    // The tanh bounds the LOOP to ±1 per sample at every setting, which is the
    // safety property and the one the brief asked for. It does not bound the wet
    // OUTPUT. The output sums `overlap` grains reading self-similar limited
    // content, so once the loop is saturating the wet peak approaches
    //     sqrt(overlap) · mean · windowNorm
    // — 1.41 for Hann at overlap 8, 1.57 for Tukey, 2.2 for Tukey at overlap 16.
    // That is the same shortfall behind v1.3.0's 1.28 peak, which loopCountTrim
    // fixed by preventing self-oscillation rather than by bounding the sum.
    //
    // Enabling self-oscillation on purpose brings it back, and the measurements
    // say no useful cap avoids it:
    //     ceiling 8,  density 100, 6 dB — peak 0.99 (Hann) … 1.10 (Tukey)
    //     ceiling 16, density 100, 6 dB — peak 1.40 (Hann) … 1.55 (Tukey)
    //     ceiling 16, density 100, 1 dB — peak 0.98, and that corner is already
    //                                     at −0.18 dB/s with NO makeup, because
    //                                     kLoopCountTrimExponent was derived to
    //                                     make it just barely decay.
    // So a cap that held peak < 1.0 everywhere would be ~1 dB, which reaches
    // sustain nowhere. The choice is between a control that does nothing and a
    // documented one.
    //
    // The peak < 1.0 invariant the rest of the suite asserts therefore belongs
    // to the NON-SELF-OSCILLATING engine — which is regen 0 dB, i.e. the
    // default, every factory preset and every session written before v1.6.0.
    // Above it the plugin is doing what a self-oscillating delay does. What
    // probe AO still requires everywhere is that the output is finite, that it
    // CONVERGES rather than climbing, and that it stays under a hard 1.8 — a
    // runaway that had not yet reached the limiter would fail all three.
    //
    // Raising this constant means re-running that ladder, not re-reasoning about
    // it.
    static constexpr float kRegenMakeupMaxDb = 6.0f;

    /** dB -> linear, for the feedback tap. Exactly 1.0f at 0 dB — std::pow
        returns exactly 1.0 for a zero exponent — which is what makes the default
        a bitwise no-op rather than a value very close to one. */
    static float regenMakeupGain (float dB) noexcept
    {
        if (dB <= 0.0f)
            return 1.0f;

        return std::pow (10.0f, juce::jmin (dB, kRegenMakeupMaxDb) * 0.05f);
    }

    //==========================================================================
    // v1.7.0 (B4 #4) — ducking.
    //
    // The wet is attenuated by the DRY input's envelope, so the wash blooms in
    // the gaps rather than competing with the source. `duck` is the depth in
    // percent, 0 = the shipped engine.
    //
    // ── Why the follower runs per SAMPLE and not per block ───────────────────
    //
    // The obvious implementation is a block-rate one: take the block's RMS, run
    // a one-pole with coefficient exp(−N/(τ·fs)), ramp the resulting gain across
    // the block. It is cheaper and it is what most ambient delays do.
    //
    // It is wrong HERE, for a reason specific to this plugin: probes O, W2 and
    // AQ assert that a 512- and a 4096-sample render of the same input are
    // BIT-IDENTICAL, and three earlier releases spent real effort earning that
    // (the sub-block pass bound at v1.0.1, the split RNG streams at v1.1.0, the
    // draw-before-obtain ordering at v1.6.0). A block-rate envelope breaks it by
    // construction — and not subtly: at 4096 samples the attack resolves to
    // 85 ms against 10.7 ms at 512, so an offline bounce would duck audibly
    // later than the same session monitored. That is the class of defect
    // pattern_offline_render_asyncupdater_dynamics_gap describes, arrived at
    // from a different direction.
    //
    // Per-sample costs two mul-adds and one divide per sample and removes the
    // question. There is no block RMS and therefore no division by numSamples to
    // guard — the guard the block-rate design would have needed is not weakened
    // here, it is absent because the quantity is.
    //
    // ── The knee is a level, not a threshold parameter ───────────────────────
    //
    // gain = 1 − depth · env/(env + knee) is a smooth compressive map with no
    // discontinuity, no knee parameter to explain, and no division by zero
    // (env >= 0, knee > 0). At env = knee the duck is at half depth; the shape
    // then approaches full depth asymptotically, so a loud source never quite
    // mutes the wet and the tail stays audible under a lead rather than gating.
    //
    // −20 dBFS (0.1) is the half-depth point because that is roughly where a
    // mixed vocal or guitar sits: quieter material ducks proportionally rather
    // than being ignored, and hotter material does not slam the wet to silence.
    static constexpr float kDuckKnee       = 0.1f;

    /** Follower time constants, seconds. Attack fast enough that the duck lands
        with the transient rather than after it; release slow enough that the
        wash swells back over a phrase gap instead of pumping between syllables.
        Not exposed: a single-knob duck is what the review asked for, and two
        more time knobs on a page that is already at its third row would be a
        worse control surface than one well-chosen pair. */
    static constexpr float kDuckAttackSec  = 0.005f;
    static constexpr float kDuckReleaseSec = 0.250f;

    /** One-pole coefficient for a per-SAMPLE follower. Computed once per block
        (two std::exp) and applied per sample, which is what keeps the envelope
        a pure function of the input rather than of the host's buffer size. */
    static float duckCoeff (float tauSeconds, double sampleRate) noexcept
    {
        const float fs = static_cast<float> (sampleRate);

        if (tauSeconds <= 0.0f || fs <= 0.0f)
            return 0.0f;

        return std::exp (-1.0f / (tauSeconds * fs));
    }

    //==========================================================================
    // v1.7.0 (B4 #6) — delay-time drift.
    //
    // A slow sine on D, sampled AT SPAWN and latched with everything else, so it
    // is click-free by construction rather than by smoothing: a grain never
    // changes the delay it is reading from, the CLOUD's delay wanders.
    //
    // Multiplicative rather than additive in ms, so the wobble is proportional —
    // a 25 % swing reads the same at a 100 ms delay and at a 4 s one, which is
    // what tape does. Additive would be inaudible at the top of the range and
    // violent at the bottom.
    //
    // ── The maximum is bounded by the RING, not by taste ─────────────────────
    //
    // kCaptureSeconds must cover gD_max + 2·G_max, and drift extends gD_max the
    // same way delayScatter does. At 0.25:
    //     gD_max = 4000·1.25 + 500 = 5500 ms,  + 2·4000 = 13500 ms
    // against a 14.0 s ring — the same 0.5 s margin every earlier release held.
    // The static_assert below is what enforces that; raising this constant
    // without raising the ring stops the build rather than quietly producing
    // grains whose tails read overwritten material.
    static constexpr float kDriftMaxFraction = 0.25f;

    /** Drift LFO rate, Hz. The bottom is a ~50 s cycle — slower than any phrase,
        which is the "is this thing on" setting that turns out to be the useful
        one on long washes — and the top is fast enough to read as vibrato on the
        tail rather than as a slow tuning drift. */
    static constexpr float kDriftRateMinHz     = 0.02f;
    static constexpr float kDriftRateMaxHz     = 5.0f;
    static constexpr float kDriftRateCentreHz  = 0.30f;

    /** The drift multiplier for a grain spawned at absolute sample `absPos`.

        Exactly 1.0f when depth is 0 — the early-out, not a sine that happens to
        be near zero — so `D * 1.0f` is bitwise D and a v1.0–v1.6 session is
        untouched. Phase is derived from the ABSOLUTE sample position rather than
        from an accumulated per-block phase, which is what makes it block-size
        invariant: an accumulator advanced 8x more often at 512 than at 4096
        lands a few ulps apart, and probe AV asserts exact equality.

        The cost of that choice is that moving the RATE knob re-derives the phase
        rather than continuing it. On a latched-at-spawn parameter that is at
        worst one grain landing at a different delay — inaudible against Scatter,
        which does the same thing deliberately — and it buys an invariant the
        whole harness depends on. */
    static float driftMul (float depthNorm, float rateHz,
                           juce::int64 absPos, double sampleRate) noexcept
    {
        if (depthNorm <= 0.0f || sampleRate <= 0.0)
            return 1.0f;

        const double revs  = static_cast<double> (absPos) * static_cast<double> (rateHz)
                               / sampleRate;
        const double phase = revs * 2.0 * juce::MathConstants<double>::pi;

        return 1.0f + juce::jmin (1.0f, depthNorm) * kDriftMaxFraction
                        * static_cast<float> (std::sin (phase));
    }

    //==========================================================================
    // v1.8.0 (B4 #7, #8) — COLOUR: diffusion and loop drive.
    //
    // The two loop-character items the v1.0.0 review left, and the reason the
    // COLOUR panel shipped framed and empty at v1.7.0. Both act on the FEEDBACK
    // return and neither touches the output path, which is why they share a
    // panel: they change what the tail becomes, not what the first pass sounds
    // like.
    //
    // The loop chain, with both inserted:
    //     loop -> fbGain·regenMakeup -> HP -> LP -> DIFFUSION -> DRIVE -> guard
    //
    // The drive IS the tanh — it is not a stage in front of one. That is the
    // whole design and the next block says why.

    /** Allpass delay lengths, ms. Four Schroeder sections per channel.
        Mutually prime-ish so their echo patterns do not reinforce into a pitched
        comb, and short — ~48 ms total — because the job is smearing a grain's
        attack, not adding a room. Reverb-scale lengths (100 ms+) would read as a
        second space bolted onto the delay rather than as the delay's own tail
        blurring, which is what the review asked for.

        Fixed rather than exposed as a Size knob: the COLOUR panel is 190 px and
        holds two knob-cells (72 + 14 + 72 = 158), which Diffusion and Drive
        already spend. A third control would force the row-3 width contract open
        and, with it, a resize — the exact cost the reserve existed to avoid.
        Same call the window LUT's table constants get. */
    static constexpr std::array<float, 4> kDiffusionAllpassMs { 4.7f, 8.3f, 13.9f, 21.7f };

    /** Allpass coefficient. 0.7 is the usual Schroeder value and it is a VOICING
        constant, not a depth control — the Diffusion knob is a wet/dry mix over
        the whole chain (see below), not this coefficient scaled. Scaling `g`
        instead would have made diffusion 0 a chain of pure N-sample DELAYS
        rather than an identity, so the knob's first touch would splice ~48 ms of
        latency into the loop and click. */
    static constexpr float kDiffusionCoeff = 0.7f;

    /** Maximum drive ratio — the pre-gain into the tanh at Drive 100 %.
        8.0 is +18 dB. Mapped exponentially from the percentage so the knob is
        perceptually even rather than bunched at the bottom. */
    static constexpr float kDriveMaxRatio = 8.0f;

    /** Percent -> tanh pre-gain ratio. Exactly 1.0f at 0 %, which is what makes
        the default bitwise identical to v1.0–v1.7 (see driveShape below). */
    static float driveRatio (float pct) noexcept
    {
        if (pct <= 0.0f)
            return 1.0f;

        const float n = juce::jlimit (0.0f, 1.0f, pct * 0.01f);
        return std::pow (kDriveMaxRatio, n);
    }

    /** The loop's saturator: tanh(d·x)/d.

        ── Why this is not a second regenMakeup ─────────────────────────────────

        regenMakeup (v1.6.0, B4 #3) multiplies the loop gain AHEAD of the damping
        filters, so it raises level INTO a fixed ceiling. That couples two things:
        it lengthens the tail and it saturates, and past ~6 dB the tanh has taken
        over and the control stops doing either (the measured ladder is in
        kRegenMakeupMaxDb's note — +0.03 dB/s and 0.86 peak at 12 dB against
        +0.08 and 0.834 at 6). A second plain pre-gain here would be that same
        control with the same inert top half, which is the objection raised when
        this release was scoped.

        Dividing by d is what separates them. d/dx tanh(d·x) at x=0 is exactly d,
        so tanh(d·x)/d has SMALL-SIGNAL GAIN 1 for every d. Three consequences,
        and they are the argument for the knob:

          * The decay rate at low level is unchanged by drive. Drive cannot open
            a self-oscillation path that `feedback` alone would not, so it needs
            no equivalent of kRegenMakeupMaxDb's measured cap.
          * The loop is bounded to ±1/d, which is STRICTLY TIGHTER than the ±1
            this plugin has guaranteed since v1.0. Drive improves the safety
            property rather than spending it.
          * It does not plateau. Because level is compensated, raising drive
            keeps adding harmonic content instead of asymptoting into a limiter —
            loud repeats compress and dull while quiet ones stay linear, so the
            tail BLOOMS as it decays. That is the character regenMakeup cannot
            produce at any setting.

        So regenMakeup answers "how long", drive answers "what colour", and they
        are orthogonal rather than two spellings of one knob.

        At d = 1.0f this returns std::tanh(x) — the same call on the same value,
        divided by exactly 1.0f — so Drive 0 is bitwise the v1.7.3 loop. Probe BC
        asserts that against a defaults render rather than trusting it. */
    static float driveShape (float x, float d) noexcept
    {
        if (d <= 1.0f)
            return std::tanh (x);

        return std::tanh (d * x) / d;
    }

    /** One Schroeder allpass section: y[n] = -g·x[n] + x[n-N] + g·y[n-N].

        Magnitude-flat at every frequency, which is the load-bearing property
        here — see the diffusion mix note in processBlock for why that makes the
        block provably non-expansive inside a recirculating loop.

        RT-safe: `buf` is sized once in prepare() and never touched again on the
        audio thread. reset() zeroes state without reallocating, exactly like the
        IIR filters' reset(), so the non-finite guard can call it. */
    struct Allpass
    {
        void prepare (int maxDelaySamples)
        {
            buf.assign (static_cast<size_t> (juce::jmax (1, maxDelaySamples)), 0.0f);
            idx = 0;
        }

        void reset() noexcept
        {
            std::fill (buf.begin(), buf.end(), 0.0f);
            idx = 0;
        }

        /** `n` is the section's delay in samples, clamped to the allocated size
            so a sample-rate change can never index past the buffer. */
        float process (float x, int n) noexcept
        {
            const int size = static_cast<int> (buf.size());
            const int d    = juce::jlimit (1, size, n);

            // Read n back from the write position, wrapping.
            int readIdx = idx - d;
            if (readIdx < 0)
                readIdx += size;

            const float delayed = buf[static_cast<size_t> (readIdx)];
            const float y       = -kDiffusionCoeff * x + delayed;

            buf[static_cast<size_t> (idx)] = x + kDiffusionCoeff * y;

            if (++idx >= size)
                idx = 0;

            return y;
        }

        std::vector<float> buf;
        int idx = 0;
    };

    /** Capture ring length. Must cover the WORST-CASE latched read span,
        gD_max + 2·G_max, where v1.1's delayScatter and v1.7's delay drift both
        extend gD_max beyond the delayTime range:
            gD_max = kDelayTimeMaxMs·(1 + kDriftMaxFraction) + kDelayScatterMaxMs
                   = 4.0·1.25 + 0.5 = 5.5 s                          (v1.7.0)
            G_max  = kGrainSizeMaxMs                      = 4.0 s   (v1.5.0)
            -> 5.5 + 2·4.0 = 13.5 s required.

        v1.7.0 raised this 13.0 -> 14.0 s for drift's positive half, which costs
        ~0.4 MB at 48 kHz and ~1.5 MB at 192 kHz over v1.5.0's ring. Note that
        drift MULTIPLIES the delay while scatter ADDS to it, so the two compose
        as (D·drift + scatter) rather than summing — the parenthesisation in the
        static_assert below is load-bearing.

        Why 2·G and not G: a grain spawned at output sample s reads source
        (s − gD − n) at its own sample n, so its LAST read — at n = G − 1 — lands
        at (s − gD − G) while the write head has itself advanced to (s + G). The
        distance the ring must still hold at that instant is therefore
        gD + 2·G, not gD + G. Getting this wrong does not fault; the read simply
        wraps onto material the writer has already overwritten, and the tail of
        every long grain turns to garbage.

        v1.0.1's 5.5 s ring met the OLD requirement with ONE sample to spare,
        which is not headroom; 6.0 s restored a 0.5 s margin. 14.0 s keeps that
        same 0.5 s margin against the new 13.5 s requirement.

        Cost, allocated once in prepareToPlay() and never on the audio thread:
        14 s stereo float is ~5.4 MB at 48 kHz, ~10.8 MB at 96 kHz and ~21.5 MB
        at 192 kHz (was ~2.3 MB at 48 kHz). That is the price of a 4 s grain
        plus a quarter of drift, and it scales with the sample rate the host
        chose, not with anything the user can dial in. */
    static constexpr float kCaptureSeconds         = 14.0f;

    /** v1.7.2 (WR-03) — the damping filters' control rate, in samples.

        32 samples is ~0.67 ms at 48 kHz, i.e. ~30 updates across the 20 ms
        smoothing contract, which resolves a cutoff sweep finely enough that the
        biquad coefficient steps are inaudible inside the feedback loop.

        Fixed in SAMPLES on purpose, not derived from the host's block size: the
        point of the constant is that the update grid follows the sample rate and
        nothing else, so a render is identical at 512 and at 4096. It is consumed
        through coeffCountdown, which persists across blocks and passes — a grid
        anchored to a pass offset would shift whenever passLen is not a multiple
        of this (grainDelayFloor is 2205 at 44.1 kHz). */
    static constexpr int kCoeffUpdateSamples = 32;

    // The ring requirement, as a COMPILE-TIME assertion rather than a comment.
    //
    // Every prose description of this invariant above was already correct at
    // v1.4.0 and still did not stop the v1.5.0 grainSize raise from silently
    // invalidating it — the arithmetic lives in a comment, and comments do not
    // fail the build. This does. Any future move of kDelayTimeMaxMs,
    // kDelayScatterMaxMs, kGrainSizeMaxMs or kDriftMaxFraction that outgrows the
    // ring now stops the compiler instead of quietly producing grains whose
    // tails read overwritten material, which is a defect no auval/pluginval pass
    // would catch and which only sounds like "the long settings are a bit
    // crunchy".
    //
    // v1.7.0 added the drift term, and it is a MULTIPLIER on the delay rather
    // than another addend — drift scales D, scatter then offsets the result — so
    // it wraps kDelayTimeMaxMs alone and not the sum. Writing it as a fourth
    // additive term would over-state the requirement by 125 ms and, worse, would
    // stop describing what the spawn handler actually computes.
    //
    // v1.8.0's diffusion adds ~48 ms of group delay to the feedback return and
    // does NOT belong here, which is worth stating because the reflex is to add
    // it. This bound is on the worst-case latched READ span — how far BACK a
    // grain reaches from the write head — and the allpasses delay what is
    // WRITTEN to the ring, not where anything reads from. A grain's read offset
    // is (gD + n) computed at spawn from the parameters below; no allpass
    // appears in it. Diffusion changes the CONTENT arriving at the write head,
    // which this assert says nothing about and does not need to.
    static_assert (kCaptureSeconds * 1000.0f
                     >= kDelayTimeMaxMs * (1.0f + kDriftMaxFraction)
                          + kDelayScatterMaxMs + 2.0f * kGrainSizeMaxMs,
                   "kCaptureSeconds is too short for the worst-case latched read span "
                   "(gD_max + 2*G_max). Raise it before widening any of the four ranges.");

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** v1.0.1 (A1): rewrite pre-1.0.1 user presets in place so their delayTime
        recalls the ms value they were saved with, not the ms value their stored
        normalised fraction now maps to under the wider range. One-shot, guarded
        by a version sentinel exactly like initializeFactoryPresets(). */
    void migrateUserPresets();

    // MUST be declared after `parameters` — it binds a reference at construction,
    // and members initialise in declaration order regardless of access specifier.
    // Name is hardcoded (no OUARICON_DEV_SUFFIX) so dev and release builds share
    // one library at ~/Library/O-ReverseDelay/Presets/{Factory,User}/.
    OuariconPresetManager presetManager { parameters, "O-ReverseDelay" };

    //==========================================================================
    // DSP components (Stage 2). All allocation confined to prepareToPlay().
    CaptureBuffer  capture;             // 5.5 s stereo ring, input + feedback return

    // v1.2.0 (B1): five shapes + per-shape power-normalisation constants, all
    // built in this member's CONSTRUCTOR — i.e. before prepareToPlay, never on
    // the audio thread. Was `hannLut` (a single Hann table) through v1.1.0; the
    // rename is deliberate so no call site can keep assuming one shape.
    WindowLut      windowLuts { 2048 };
    GrainPool      grainPool;           // 32 preallocated reverse-grain slots
    GrainScheduler scheduler;           // free-countdown spawn scheduler

    std::array<SpawnRequest, GrainScheduler::kMaxSpawnsPerBlock> spawnRequests {};

    juce::AudioBuffer<float> wetScratch;   // OUTPUT wet — includes per-grain gainRandom
    juce::AudioBuffer<float> loopScratch;  // FEEDBACK-TAP wet — excludes gainRandom (v1.1.0)
    juce::AudioBuffer<float> fbScratch;    // feedback return: loop → fbGain → HP → LP → diffusion → drive → guard

    /** v1.8.0 (B4 #7) — the diffusion chain, four allpass sections per channel.

        Per channel and NOT shared: a shared chain would fold L and R through one
        state and collapse the stereo tail to mono the moment diffusion came up,
        which is the opposite of what a diffuser is for.

        Delay lengths are resolved to SAMPLES in prepareToPlay from
        kDiffusionAllpassMs, so a sample-rate change re-derives them rather than
        keeping a length that means a different time. */
    std::array<Allpass, 4> apL, apR;
    std::array<int, 4>     apDelaySamples { 1, 1, 1, 1 };

    // In-loop damping filters (2nd-order Butterworth): lowCut = HP, highCut = LP.
    // Coefficient updates use ArrayCoefficients assigned IN PLACE into the
    // existing Coefficients objects (*filter.coefficients = array) — never
    // Coefficients::makeXXX on the audio thread (heap-allocates), never memcpy
    // raw 6-arrays over the 5-value normalised storage.
    juce::dsp::IIR::Filter<float> hpL, hpR, lpL, lpR;

    // Cached-cutoff guards — gate ONLY the coefficient recompute; no
    // enabled/bypass flag exists (O-MultiBandCompressor v1.6.0 lesson).
    float lastLowCut  = -1.0f;
    float lastHighCut = -1.0f;

    /** v1.7.2 (WR-03) — samples remaining before the next damping-coefficient
        refresh.

        Deliberately a MEMBER and not a loop-local: it carries the remainder
        across block and pass boundaries, which is what makes the update grid
        continuous in stream time and therefore identical at every host block
        size. Reset to 0 in prepareToPlay()/reset() so the first block after a
        (re)start updates immediately. */
    int coeffCountdown = 0;

    /** Advances the two cutoff smoothers by `numSamples` and refreshes the
        damping filters' coefficients if either value actually moved.

        Called from step 5 of processBlock on the kCoeffUpdateSamples grid. RT-safe:
        ArrayCoefficients returns a stack std::array and operator= assigns the
        normalised values in place into the existing Coefficients — no allocation
        (pattern_arraycoefficients_rt_safe_iir). The exactlyEqual guards gate only
        the recompute, never an enabled flag
        (pattern_conditional_coeff_update_leaks_enabled_flag). */
    void updateDampingCoefficients (int numSamples) noexcept;

    // Smoothed (~20 ms): feedback, mix, lowCut, highCut.
    // NEVER smoothed (latched per grain): delayTime/D, grainSize, density, width.
    juce::SmoothedValue<float> feedbackSmoothed, mixSmoothed, lowCutSmoothed, highCutSmoothed;

    double currentSampleRate = 44100.0;

    // TWO RT-safe xorshift32 streams (never juce::Random::getSystemRandom on the
    // audio thread), split by WHEN they are consumed rather than by what they
    // feed — and that split is a correctness requirement, not tidiness.
    //
    //   grainRng  — consumed inside the spawn handler: scatter, size, gain, pan.
    //   jitterRng — consumed by GrainScheduler, inside its per-sample countdown.
    //
    // processBlock runs the engine in sub-passes bounded to D (the A2 fix), so
    // the number of spawns per pass depends on the HOST BLOCK SIZE. With one
    // shared stream the scheduler's jitter draws batch per pass and interleave
    // with the spawn handler's draws differently at 512 than at 4096 samples,
    // and the same session renders differently in an offline bounce than it
    // monitored. Two streams make each one's consumption a pure function of the
    // spawn INDEX, which is block-size invariant — render-harness probe W2
    // asserts 512-vs-4096 bit equality with all four randomisations on, and
    // caught exactly this.
    //
    // Both are seeded from instanceSeed (decorrelated by a different mixing
    // constant), so one seed still reproduces the whole engine.
    //
    // Every randomisation must still draw NOTHING when its amount is 0, or
    // turning one on would shift the others' sequences and change the shipped
    // v1.0 sound — probe T asserts that as bit-equality.
    //
    // panSign alternates so consecutive grains ping left/right rather than
    // clumping. Bias amount is a harness-tuned constant (probe K, D5).
    static constexpr float kPanBias = 0.5f;

    /** Per-instance seed, fixed for the lifetime of the processor.
        Fixed PER INSTANCE rather than per prepareToPlay: a single instance must
        stay reproducible across prepare/reset (probe O compares two renders of
        the same instance at different block sizes and requires bit equality),
        while two instances on two tracks must decorrelate — v1.0 gave every
        instance the same literal, so two tracks produced identical pan and
        (from v1.1) identical grain randomisation, and correlated audibly. */
    static juce::uint32 makeInstanceSeed (const void* self) noexcept;

    const juce::uint32 instanceSeed { makeInstanceSeed (this) };

    juce::uint32 rngState       { instanceSeed };
    juce::uint32 jitterRngState { deriveJitterSeed (instanceSeed) };
    float        panSign        = 1.0f;

    /** Second stream's seed. A different odd multiplier plus an xor so the two
        streams do not walk the same trajectory offset by a constant; guarded
        against zero, which xorshift32 absorbs permanently. */
    static juce::uint32 deriveJitterSeed (juce::uint32 s) noexcept
    {
        const juce::uint32 j = (s * 0x9E3779B9u) ^ 0x5BF03635u;
        return j != 0u ? j : 0xA5A5A5A5u;
    }

    static float xorshiftNext (juce::uint32& state) noexcept
    {
        juce::uint32 x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return static_cast<float>(x >> 8) * (1.0f / 16777216.0f);   // [0, 1)
    }

    float nextRand01()       noexcept { return xorshiftNext (rngState); }
    float nextJitterRand01() noexcept { return xorshiftNext (jitterRngState); }

    /** Bipolar random multiplier 1 + amount·u with u uniform on [−1, 1), so the
        mean is exactly 1 and the parameter changes SPREAD, not level.
        Returns 1.0f without touching the RNG when amount <= 0 — see the stream
        note above; this is the mechanism behind "all four at 0 is bit-identical
        to v1.0.1", not a coincidence of it. */
    float randomMul (float amount) noexcept
    {
        if (amount <= 0.0f)
            return 1.0f;

        return 1.0f + amount * (2.0f * nextRand01() - 1.0f);
    }

    // Cached APVTS atomics — read once per block on the audio thread.
    std::atomic<float>* pDelayTime    = nullptr;
    std::atomic<float>* pSyncMode     = nullptr;
    std::atomic<float>* pNoteDivision = nullptr;
    std::atomic<float>* pGrainSize    = nullptr;
    std::atomic<float>* pDensity      = nullptr;
    std::atomic<float>* pFeedback     = nullptr;
    std::atomic<float>* pLowCut       = nullptr;
    std::atomic<float>* pHighCut      = nullptr;
    std::atomic<float>* pWidth        = nullptr;
    std::atomic<float>* pMix          = nullptr;

    // v1.1.0 grain randomisation (B3). All four default to 0 — see
    // createParameterLayout() for why that is a hard requirement, not a taste.
    std::atomic<float>* pJitter       = nullptr;
    std::atomic<float>* pDelayScatter = nullptr;
    std::atomic<float>* pSizeRandom   = nullptr;
    std::atomic<float>* pGainRandom   = nullptr;

    // v1.2.0 grain window (B1). Both default to the SHIPPED window — grainTilt
    // to 0.5 (symmetric) and grainShape to 0 (Hann) — so unlike the v1.1 four,
    // the no-op default is not 0 for both. What matters is that it is the no-op:
    // see createParameterLayout().
    std::atomic<float>* pGrainTilt    = nullptr;
    std::atomic<float>* pGrainShape   = nullptr;

    // v1.3.0 grain count (B2). Defaults to 8 — v1.2.0's hard-coded ceiling —
    // which makes the overlap expression bitwise what it was. Third release in a
    // row where the no-op default is a specific number rather than zero.
    std::atomic<float>* pGrainCount   = nullptr;

    // v1.4.0 Tukey taper. Defaults to 0.5 — v1.2.0's frozen taper — so this is
    // the FOURTH release running whose new parameter's no-op is a specific
    // number rather than zero. 0.01 would ship a near-rectangular window.
    std::atomic<float>* pTukeyTaper   = nullptr;

    // v1.6.0 (B4 #1-#3) — MOTION panel. All three no-ops are the range minimum
    // for once: freeze off, direction 0 (all-reverse), regen 0 dB. That is the
    // v1.1.0 situation rather than the v1.2-v1.4 one, and it is worth saying so
    // explicitly, because "the new control's neutral value is zero" has been
    // WRONG for three consecutive releases here and a reader arriving from those
    // would reasonably expect a fourth trap.
    std::atomic<float>* pFreeze       = nullptr;
    std::atomic<float>* pDirection    = nullptr;
    std::atomic<float>* pRegenMakeup  = nullptr;

    // v1.7.0 (B4 #4-#6) — SOURCE / DUCK / DRIFT. Four more no-ops at the range
    // minimum, i.e. the v1.1.0 and v1.6.0 situation rather than the v1.2-v1.4
    // one. sourceMode's is index 0 (Mono Sum), which is the same kind of
    // guarantee grainShape's Hann-at-index-0 carries: the ORDER of the choice
    // list is load-bearing, because an absent key in a pre-v1.7.0 session or
    // preset resolves to index 0 and must land on the shipped behaviour.
    std::atomic<float>* pSourceMode   = nullptr;
    std::atomic<float>* pDuck         = nullptr;
    std::atomic<float>* pDriftRate    = nullptr;
    std::atomic<float>* pDriftDepth   = nullptr;

    // v1.8.0 (B4 #7-#8) — COLOUR. Both no-ops are the range minimum, so this is
    // the v1.1.0 / v1.6.0 / v1.7.0 situation again and not the v1.2-v1.4 trap.
    // Worth one line on WHY, because for `drive` the zero is doing more work
    // than it looks: 0 % maps through driveRatio() to a ratio of exactly 1.0f,
    // and driveShape() branches on `d <= 1.0f` to call plain std::tanh — the
    // same call on the same value the loop has made since v1.0. The no-op is an
    // early-out, not a division by a number very close to one.
    std::atomic<float>* pDiffusion    = nullptr;
    std::atomic<float>* pDrive        = nullptr;

    /** v1.7.0 (B4 #4) — the duck follower's state: a one-pole on |dry|, advanced
        per sample inside the mix loop (which is the one place the dry input is
        still readable — the wet is mixed into the output buffer in place).

        NOT a SmoothedValue: this is an envelope FOLLOWER with asymmetric attack
        and release, and a SmoothedValue would ramp linearly to a block-rate
        target, which is the block-size-dependent design this deliberately is
        not. Reset to 0 in prepare/reset so a fresh instance starts un-ducked. */
    float duckEnv = 0.0f;

    /** v1.6.0 (B4 #1) — the freeze crossfade, 0 = writing, 1 = held.
        Smoothed on the same ~20 ms as feedback/mix/cutoffs, and smoothed for a
        different reason than any of them: it is not the audible level that would
        step at the boundary but the ring's CONTENT, which splices from live
        capture to material a full ring lap old. See CaptureBuffer::pushBlended. */
    juce::SmoothedValue<float> freezeSmoothed;

    /** v1.6.0 (B4 #1) — how long the frozen loop is, latched when the hold
        begins and held until it ends.

        Latched to how much has ACTUALLY been captured, not to the ring length.
        The ring is kCaptureSeconds long and starts cleared, so a freeze in the
        first 13 s of a session would otherwise loop over material that was never
        written and the wash would fall silent mid-hold — which is not a corner
        case, it is what happens the first time anybody presses the button.
        Capped at bufferSize − 1 so the read can never land on the position about
        to be written. Probe AP is the standing guard.

        v1.7.2 (CR-02) adds the floor at the other end: the latch will not arm at
        all until at least one grain (G samples) has been captured. Below that the
        "hold" is a periodic tone at 1/loopSamples rather than a wash — 200
        captured samples is a 240 Hz buzz at 48 kHz — and at the limit
        (totalWritten == 0, which is every prepareToPlay and reset) it is a
        one-sample loop over a cleared ring, i.e. permanent silence. */
    int  freezeLoopSamples = 1;

    /** Whether a hold is currently ARMED — i.e. whether freezeLoopSamples above
        describes a real captured span.

        v1.7.2 (CR-02): this is deliberately NOT "the previous block's freeze
        target". It tracks whether the latch SUCCEEDED, so a Freeze engaged before
        the ring has a grain in it is deferred rather than lost: freezeEngaged
        stays false, the smoother's target stays 0, live material keeps being
        written, and the hold arms itself on the first block that has enough. The
        old unconditional `freezeEngaged = frozen` is what let a session saved
        with Freeze on reopen with a permanently silent wet path. */
    bool freezeEngaged = false;

    //==========================================================================
    // v1.3.0 (B2) — meter + spawn accounting, all published by processBlock and
    // read from the message thread. Relaxed ordering throughout: these are
    // display and diagnostic values with no happens-before relationship to any
    // audio state, and nothing in the audio path ever reads them back.
    std::atomic<int>          publishedActiveGrains { 0 };
    std::atomic<float>        publishedOverlap      { 0.0f };
    std::atomic<juce::uint32> droppedSpawns         { 0 };
    std::atomic<juce::uint32> refusedSpawns         { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverseDelayProcessor)
};
