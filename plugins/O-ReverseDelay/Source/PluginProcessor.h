#pragma once

#include <JuceHeader.h>

#include <atomic>

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
// APVTS with 16 parameters: the 10 of research/ARCHITECTURE.md's immutable
// contract, plus v1.1.0's four grain randomisations (B3) and v1.2.0's two
// window controls (B1), all of which are ADDITIVE — every one defaults to the
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

    /** Stage 4: preset library access for the editor's 10 preset native functions
        and for the render harness' probe N factory audit. */
    OuariconPresetManager& getPresetManager() noexcept { return presetManager; }

    /** Live concurrent-grain count. GrainPool::countActive() existed from Stage 2
        and was called by nothing; v1.1.0 exposes it so render-harness probe Y can
        report the peak concurrency the randomisations actually reach instead of
        asserting a number nobody measured.

        Reads audio-thread state without synchronisation: fine for the harness
        (single-threaded) and for a torn-value-tolerant UI meter, NOT a basis for
        any audio-path decision. */
    int getActiveGrainCount() const noexcept { return grainPool.countActive(); }

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverseDelayProcessor)
};
