#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <cmath>

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
// APVTS with 17 parameters: the 10 of research/ARCHITECTURE.md's immutable
// contract, plus v1.1.0's four grain randomisations (B3), v1.2.0's two window
// controls (B1) and v1.3.0's overlap ceiling (B2), all of which are ADDITIVE —
// every one defaults to the engine's no-op, so the contract's behaviour is the
// default behaviour.
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
    static constexpr float kGrainSizeMinMs         =  50.0f;
    static constexpr float kGrainSizeMaxMs         = 500.0f;

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

    /** Capture ring length. Must cover the WORST-CASE latched read span,
        gD_max + 2·G_max, where v1.1's delayScatter extends gD_max beyond the
        delayTime range:
            gD_max = kDelayTimeMaxMs + kDelayScatterMaxMs = 4.0 + 0.5 = 4.5 s
            G_max  = kGrainSizeMaxMs                      = 0.5 s
            -> 4.5 + 2·0.5 = 5.5 s required.
        v1.0.1's 5.5 s ring met that requirement with ONE sample to spare, which
        is not headroom. 6.0 s restores a 0.5 s margin for ~192 KB stereo at
        48 kHz. */
    static constexpr float kCaptureSeconds         = 6.0f;

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
    juce::AudioBuffer<float> fbScratch;    // feedback return: loop → fbGain → HP → LP → tanh → guard

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
