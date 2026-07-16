/*
  ==============================================================================

    O-simpleAdditive - Additive Voice (band-limited single-cycle wavetable)

    One polyphonic additive voice. The 16-partial amplitude vector (Frame A
    drawbars) is summed into a per-note band-limited single-cycle table at
    control rate; the table is read back by phase at audio rate and shaped by an
    independent amp ADSR (which also governs voice lifetime). This is the
    CCRMA additive→wavetable bridge: one stored period IS the summed-partials
    waveshape (ARCHITECTURE.md §"Additive → wavetable bridge").

    Band-limiting is EXACT and free: partials with harmonic number k > Kmax
    (= floor(0.5·fs/f0)) are simply never written into the table, with a
    raised-cosine taper on the top 2 surviving harmonics to avoid boundary
    clicks. No oversampling → zero latency.

    Stage 2 build-out:
      • Phase 2.1: core voice — Frame A drawbars + amp ADSR only.
      • Phase 2.2: Frame A→B spectral morph + scan LFO + mod-env → scan.
      • Phase 2.3 (THIS FILE): spectral-decay tilt (per-partial exp(-rate·k·tau)
        composed into refillTable BEFORE band-limit) + bit-depth quantizer
        (read-time, post-table pre-amp) + active-spectrum snapshot (16 morphed +
        decayed amplitudes published to the editor's drawbar display) + per-voice
        noteAge (primary-voice selection for the snapshot).
    The refillTable()/active-spectrum pipeline was deliberately staged so 2.2/2.3
    slot in at the marked extension points without reshaping the voice.

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL custom prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast (O-simpleFM FMVoice / O-Bassoon pattern). setSampleRate MUST
    precede the first setParameters.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace OSimpleAdditive
{
    // Band-clean sine via a shared 1024-point lookup table (same primitive and
    // SNR rationale as O-simpleFM's Operator.h fastSine). Function-local static
    // (C++11 thread-safe init); std::sin is used ONLY for the one-time fill.
    //
    // CRITICAL: phase MUST be floor-modulo wrapped into [0, 2*pi) before the
    // lookup. Table fills evaluate sin at k·θ for k up to 16, i.e. up to ~32*pi;
    // LookupTableTransform CLAMPS out-of-range inputs rather than wrapping, so
    // without the wrap high harmonics would flat-line.
    inline float fastSine (float phase) noexcept
    {
        struct SineTable
        {
            juce::dsp::LookupTableTransform<float> t;
            SineTable()
            {
                t.initialise ([] (float x) { return std::sin (x); },
                              0.0f, juce::MathConstants<float>::twoPi, 1024);
            }
        };
        static const SineTable table;

        if (! std::isfinite (phase)) return 0.0f;          // floor(NaN)=NaN -> LUT UB
        constexpr float twoPi = juce::MathConstants<float>::twoPi;
        phase -= twoPi * std::floor (phase / twoPi);       // MANDATORY wrap (LUT clamps)
        return table.t (phase);
    }

    // Frame B target spectra — the four teaching presets selected by `frameBSource`
    // (ARCHITECTURE.md §"Frame B preset shapes"). Raw amplitudes (NOT normalized);
    // refillTable() applies the shared headroom divide, so the morph endpoints stay
    // balanced against Frame A.
    //   0 Sine   : H1 only                — "the atom of additive"
    //   1 Saw    : A_k = 1/k              — the 1/n series fills the ramp
    //   2 Square : odd-only 1/k          — hollow / woody
    //   3 Odd    : odd-only 1/m (flatter) — a brighter hollow variant vs Square
    enum FrameBSource { kSine = 0, kSaw, kSquare, kOdd };

    inline void fillFrameB (int source, float (&out)[16]) noexcept
    {
        juce::FloatVectorOperations::clear (out, 16);
        switch (source)
        {
            case kSaw:
                for (int k = 1; k <= 16; ++k) out[k - 1] = 1.0f / (float) k;
                break;
            case kSquare:                                   // odds only, 1/k roll-off
                for (int k = 1; k <= 16; k += 2) out[k - 1] = 1.0f / (float) k;
                break;
            case kOdd:                                      // odds only, flatter 1/m roll-off
                for (int k = 1, m = 1; k <= 16; k += 2, ++m) out[k - 1] = 1.0f / (float) m;
                break;
            case kSine:
            default:
                out[0] = 1.0f;
                break;
        }
    }
}

//==============================================================================
class AdditiveSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class AdditiveVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int kNumPartials = 16;
    static constexpr int kTableSize   = 2048;   // single cycle; power-of-two → mask wrap

    AdditiveVoice() = default;
    ~AdditiveVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<AdditiveSound*> (sound) != nullptr;
    }

    //==========================================================================
    // Non-virtual JUCE-8 prepare. setSampleRate MUST precede setParameters.
    void prepareToPlay (double sr, int /*maxBlockSize*/)
    {
        setCurrentPlaybackSampleRate (sr);
        sampleRate = sr;

        // Touch the shared sine LUT so its one-time magic-static construction
        // (guard mutex + LookupTableTransform heap fill) happens HERE, on the
        // message/host thread — never inside the first note-on's render call.
        (void) OSimpleAdditive::fastSine (0.0f);

        ampEnv.setSampleRate (sr);
        ampEnv.setParameters (ampParams);
        modEnv.setSampleRate (sr);
        modEnv.setParameters (modParams);

        // Scan pointer smoother (~20 ms) — see ARCHITECTURE.md §"Smoothing / zipper
        // avoidance". Post-sum smoothing keeps the morph zipper-free at control-rate
        // table refills (DSP-03).
        scanSmoothed.reset (sr, 0.02);

        // Resolve the refill-cadence cap (PERF): minimum samples between
        // motion-driven table rebuilds (~5 ms control rate). At least 1.
        minRefillSamples   = juce::jmax (1, (int) std::lround (sr * (double) kRefillIntervalSeconds));
        samplesSinceRefill = minRefillSamples;   // allow the first motion refill immediately

        // Start with a clean (silent) table so an un-triggered voice is benign.
        juce::FloatVectorOperations::clear (table, kTableSize);
        spectrumDirty = true;
    }

    //==========================================================================
    // Block param-push from the processor (once per block). Phase 2.2 added the
    // wavetable dimension (Frame B vector, the GLOBAL scan base, bipolar mod-env
    // depth, mod-ADSR shape). Phase 2.3 adds the spectral-decay sources
    // (`spectralDecay` knob + `velToDecay` amount, combined with this voice's
    // velocity into an effective per-partial decay `rate` inside renderNextBlock)
    // and the resolved bit-depth quantizer count (`bitDepthBits`, 0 = Off).
    // Marks the table dirty only when the spectrum SOURCES (Frame A / Frame B)
    // actually moved; scan motion AND decay motion are detected per-block in
    // renderNextBlock, so a fully static patch still refills once per note.
    void setParams (const float (&frameA)[kNumPartials],
                    const float (&frameB)[kNumPartials],
                    float scanBaseIn, float scanEnvAmountIn,
                    float spectralDecayIn, float velToDecayIn, int bitDepthBitsIn,
                    const juce::ADSR::Parameters& ap,
                    const juce::ADSR::Parameters& mp) noexcept
    {
        for (int k = 0; k < kNumPartials; ++k)
        {
            // Exact compare (intentional dirty-check): juce::exactlyEqual keeps
            // the bit-exact semantics without tripping -Wfloat-equal.
            if (! juce::exactlyEqual (frameA[k], frameASpectrum[k]))
            {
                frameASpectrum[k] = frameA[k];
                spectrumDirty = true;
            }
            if (! juce::exactlyEqual (frameB[k], frameBSpectrum[k]))
            {
                frameBSpectrum[k] = frameB[k];
                spectrumDirty = true;
            }
        }

        scanBase      = scanBaseIn;          // global: scanPosition + lfo·depth (pre-clamp)
        scanEnvAmount = scanEnvAmountIn;      // bipolar mod-env → scan depth (−1..+1)

        // Decay sources. A change in either source moves the *effective* rate, so
        // the next block recomputes it (and re-dirties the table while tau<1).
        // Exact compares avoid -Wfloat-equal; harmless if the recompute is a no-op.
        spectralDecay = spectralDecayIn;     // 0..1 knob
        velToDecay    = velToDecayIn;        // 0..1 opt-in velocity→decay amount

        bitDepthBits  = bitDepthBitsIn;       // 0 = Off (passthrough), else bit count

        ampParams = ap;
        ampEnv.setParameters (ampParams);
        modParams = mp;
        modEnv.setParameters (modParams);
    }

    //==========================================================================
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int /*pitchWheel*/) override
    {
        f0       = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        phaseInc    = (float) (f0 / sampleRate);

        velLevel = juce::jlimit (0.0f, 1.0f, velocity);

        phase = 0.0f;                 // restart the single-cycle read at zero phase
        tau   = 0.0f;                 // restart the spectral-decay ramp at note-on (2.3)
        computeKmax();                // per-note band-limit

        // Monotonic note-age stamp for primary-voice selection (the editor draws
        // the active-spectrum snapshot of the NEWEST sounding voice). One shared
        // atomic counter across all voices → strictly increasing per note-on.
        noteAge = ++sNoteCounter;

        // Seed the scan pointer at the note's starting position (mod-env just
        // retriggered → value ≈ 0), with no smoother ramp from a stale value.
        lastModEnv = 0.0f;
        const float scan0 = juce::jlimit (0.0f, 1.0f, scanBase + lastModEnv * scanEnvAmount);
        scanSmoothed.setCurrentAndTargetValue (scan0);
        currentScan = scan0;

        refillTable (currentScan);    // first sample must be correct → fill now
        samplesSinceRefill = 0;       // restart the refill-cadence window at note-on

        ampEnv.noteOn();
        modEnv.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();
            modEnv.noteOff();
        }
        else
        {
            clearCurrentNote();
            ampEnv.reset();
            modEnv.reset();
        }
    }

    // v1.0: no pitch bend (pitch is fixed per note — ARCHITECTURE.md §MIDI Handling).
    void pitchWheelMoved (int) override {}
    void controllerMoved  (int, int) override {}

    //==========================================================================
    // Active-spectrum accessors (2.3) — read by the processor on the AUDIO thread
    // (same thread as renderNextBlock), which then publishes the chosen voice's
    // 16 amplitudes into an atomic snapshot for the message-thread drawbar display.
    // No atomics needed voice→processor (same thread); the snapshot crosses the
    // thread boundary, not these getters.
    //   getActiveSpectrum : post-morph, post-decay, post-band-limit, pre-norm
    //                       amplitudes — the exact bars the engine is summing
    //                       (partials above Nyquist read 0, so the live-glow on
    //                       high notes shows only what actually sounds).
    //   isAmpActive       : amp-env activity (the voice's audible lifetime). NB:
    //                       deliberately NOT named isVoiceActive — that is a
    //                       juce::SynthesiserVoice virtual used internally for
    //                       voice-stealing; shadowing it would hijack allocation.
    //   getNoteAge        : monotonic note-on stamp → newest sounding voice wins.
    const float* getActiveSpectrum() const noexcept { return activeSpectrum; }
    bool isAmpActive() const noexcept                { return ampEnv.isActive(); }
    std::uint64_t getNoteAge() const noexcept        { return noteAge; }

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! ampEnv.isActive())
            return;

        // --- Control-rate scan resolve (ARCHITECTURE.md §"Scan pointer is the sum
        // of three sources"). scanBase already folds in the global manual position
        // and the global LFO; here we add this voice's mod-env, clamp, and smooth.
        // The mod-env value is taken from the END of the previous block (advanced in
        // the audio loop below) — a sub-block latency, harmless under the 20 ms
        // smoother. Refill only when the smoothed scan actually moved, so a static
        // patch keeps the once-per-note refill cadence.
        const float scanTarget = juce::jlimit (0.0f, 1.0f, scanBase + lastModEnv * scanEnvAmount);
        scanSmoothed.setTargetValue (scanTarget);
        const float scanNow = scanSmoothed.skip (numSamples);   // end-of-block value

        if (std::abs (scanNow - currentScan) > kScanRefillEps)
        {
            currentScan   = scanNow;
            spectrumDirty = true;
        }

        // --- Spectral-decay ramp (2.3 — ARCHITECTURE.md §"Spectral-Decay Macro").
        // tau ramps 0→1 from note-on over a fixed musical time at CONTROL rate
        // (once per block). The per-partial multiplier D_k = exp(-rate·k·tau) is
        // applied inside refillTable; here we advance tau and compute the effective
        // rate so we can decide whether the active spectrum is still moving.
        currentDecayRate = effectiveDecayRate();

        // A change in the effective rate itself (Spectral Decay / Vel→Decay moved)
        // must re-dirty the table even when tau has saturated at 1 (the branch
        // below stops running then) and when the rate drops to 0 mid-note (the
        // table must refill once more to RESTORE the undecayed spectrum). tau==0
        // ⇒ D_k = 1 regardless of rate, so no refill is needed yet.
        if (! juce::exactlyEqual (currentDecayRate, lastRenderedDecayRate))
        {
            lastRenderedDecayRate = currentDecayRate;
            if (tau > 0.0f)
                spectrumDirty = true;
        }

        if (currentDecayRate > 0.0f && tau < 1.0f)
        {
            tau = juce::jmin (1.0f, tau + (float) numSamples / (kTauRampSeconds * (float) sampleRate));
            // While decaying with a non-zero rate the active spectrum changes every
            // block → the table MUST refill every block. When rate==0 (default:
            // spectralDecay=0 && velToDecay=0) this branch never runs, so the
            // Phase-2.2 once-per-note / scan-driven refill cadence is preserved.
            spectrumDirty = true;
        }

        // Cadence-capped refill (PERF — see kRefillIntervalSeconds). The table
        // rebuild is a fixed cost per call regardless of numSamples, so on small
        // host blocks a per-block rebuild during continuous motion does not
        // amortize. Bound motion-driven refills to the control-rate interval: a
        // refill only fires once at least minRefillSamples have elapsed since the
        // last one. Static patches never set spectrumDirty here (they refill once
        // per note in startNote), so their output stays bit-identical; large host
        // blocks (numSamples ≥ minRefillSamples) still refill every block. The
        // counter saturates at the interval so a long static sustain cannot
        // overflow it.
        if (samplesSinceRefill < minRefillSamples)
            samplesSinceRefill += numSamples;

        if (spectrumDirty && samplesSinceRefill >= minRefillSamples)
        {
            refillTable (currentScan);     // clears spectrumDirty
            samplesSinceRefill = 0;
        }

        const int numCh = out.getNumChannels();

        // Bit-depth quantizer level, precomputed once per block (NOT per sample).
        // bitDepthBits == 0 → Off (passthrough). Else mid-tread: q = round(s·L)/L
        // with L = 2^(bits-1) — the staircase IS the lesson (no dither). DSP-05.
        const bool  quantOn = (bitDepthBits > 0);
        const float qLevel  = quantOn ? std::exp2 ((float) (bitDepthBits - 1)) : 1.0f;
        const float qInv    = quantOn ? 1.0f / qLevel : 1.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float s = readTableLinear (phase);
            if (quantOn)
                s = std::round (s * qLevel) * qInv;          // bit-depth grit (post-table, pre-amp)

            const float ampVal   = ampEnv.getNextSample();
            lastModEnv           = modEnv.getNextSample();   // advance mod-env in time
            const float sample   = s * ampVal * velLevel;

            for (int ch = 0; ch < numCh; ++ch)
                out.addSample (ch, startSample + i, sample);

            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;
        }

        // Voice lifetime keyed on the amp envelope only — a long mod-env release
        // must NOT keep a silent voice alive (ARCHITECTURE.md §Voice lifetime).
        if (! ampEnv.isActive())
            clearCurrentNote();
    }

private:
    //==========================================================================
    // Per-note Nyquist band-limit: highest alias-free harmonic count. Kmax may
    // legitimately be 0 (f0 at/above Nyquist, e.g. MIDI 127 at very low sample
    // rates): nyquistGain's k > Kmax check then zeroes EVERY partial, so
    // refillTable produces a silent table rather than writing an aliased
    // fundamental. Never force it up to 1.
    void computeKmax() noexcept
    {
        const double nyquist = 0.5 * sampleRate;
        const int    k       = (int) std::floor (nyquist / juce::jmax (1.0, f0));
        Kmax = juce::jlimit (0, kNumPartials, k);
    }

    // Raised-cosine band-limit gain for harmonic k (1-based) given Kmax.
    //   k > Kmax        → 0      (above Nyquist: never written)
    //   k == 1          → 1      (fundamental always passes when below Nyquist)
    //   top 2 surviving → cosine taper (Kmax-1 ≈ 0.5, Kmax ≈ 0) → no boundary click
    static float nyquistGain (int k, int Kmax) noexcept
    {
        if (k > Kmax) return 0.0f;
        if (k == 1)   return 1.0f;

        const int taperStart = Kmax - 2;     // harmonics above this are tapered
        if (k <= taperStart) return 1.0f;

        const float x = (float) (k - taperStart);   // 1 → 0.5, 2 → 0.0
        return 0.5f * (1.0f + std::cos (juce::MathConstants<float>::pi * x * 0.5f));
    }

    //==========================================================================
    // Effective per-partial decay rate for this block (2.3 — ARCHITECTURE.md
    // §"Spectral-Decay Macro"). rate = (spectralDecay + velLevel·velToDecay)·
    // kDecayRateMax, clamped ≥ 0. At full decay (tau=1) the steepest harmonic
    // (k=15, the 16th) reaches D_15 = exp(-kDecayRateMax·15) ≈ exp(-5.25) ≈ 0.005
    // (≈ −46 dB) — a strong, clearly audible/visible darkening. Default sources
    // (spectralDecay=0 && velToDecay=0) → rate=0 → D_k=1 ∀k (no-regression).
    float effectiveDecayRate() const noexcept
    {
        const float r = (spectralDecay + velLevel * velToDecay) * kDecayRateMax;
        return juce::jmax (0.0f, r);
    }

    //==========================================================================
    // Fill the single-cycle table from the active spectrum. Phase 2.2: the active
    // spectrum is the per-partial morph between Frame A (drawbars) and Frame B
    // (preset), `active_k = lerp(A_k, B_k, scan)` — linear *spectral* interpolation,
    // phase-coherent and zipper-free (ARCHITECTURE.md §Morph, DSP-03). Phase 2.3
    // multiplies in the spectral-decay tilt `D_k = exp(-rate·k·tau)` here too —
    // also BEFORE band-limit + sum, so the table is always exactly what is heard.
    // The drawbar display reads the same band-limited amplitudes (snapshotted into
    // activeSpectrum[] after the band-limit) so the live-glow matches the sound;
    // QUAL-02).
    void refillTable (float scan) noexcept
    {
        float band[kNumPartials];
        float sumA = 0.0f;

        const float rate = currentDecayRate;   // effective decay rate for this block

        for (int k = 0; k < kNumPartials; ++k)
        {
            // Morph (2.2): per-partial lerp A→B.
            const float morphedK = frameASpectrum[k]
                                 + scan * (frameBSpectrum[k] - frameASpectrum[k]);

            // --- 2.3 spectral-decay composes here ---
            // D_k = exp(-rate·k·tau), k 0-based: k=0 (fundamental) → D=1 (never
            // decays); higher k decays faster. rate==0 → D_k=1 ∀k → bit-identical
            // to Phase 2.2 (no-regression guarantee). std::exp(0)==1 exactly.
            const float decayK = (rate > 0.0f)
                               ? std::exp (-rate * (float) k * tau)
                               : 1.0f;
            const float activeK = morphedK * decayK;

            band[k] = activeK * nyquistGain (k + 1, Kmax);   // exact band-limit

            // Snapshot the post-morph, post-decay, POST-band-limit (pre-norm)
            // amplitude — the exact bar the engine is summing, which the drawbar
            // live-glow reads via getActiveSpectrum(). Folding in the band-limit
            // gain means a partial above Nyquist on a high note glows at 0, so the
            // green glow only ever shows what is actually sounding (QUAL-02).
            activeSpectrum[k] = band[k];

            sumA   += band[k];
        }

        // Headroom: |Σ band[k]·sin| ≤ Σ band[k], so dividing by max(1, Σ) bounds
        // the table to [-1, 1] → 16 maxed drawbars cannot clip. A single partial
        // (Σ ≤ 1) is left full-scale.
        const float norm = 1.0f / juce::jmax (1.0f, sumA);

        constexpr float twoPi = juce::MathConstants<float>::twoPi;
        for (int i = 0; i < kTableSize; ++i)
        {
            const float theta = twoPi * (float) i / (float) kTableSize;
            float acc = 0.0f;
            for (int k = 0; k < kNumPartials; ++k)
                if (! juce::exactlyEqual (band[k], 0.0f))   // skip silent partials (-Wfloat-equal safe)
                    acc += band[k] * OSimpleAdditive::fastSine (theta * (float) (k + 1));

            table[i] = acc * norm;
        }

        spectrumDirty = false;
    }

    // Linear-interpolated single-cycle read. phase01 ∈ [0,1).
    float readTableLinear (float phase01) const noexcept
    {
        const float fp   = phase01 * (float) kTableSize;
        int         i0   = (int) fp;
        const float frac = fp - (float) i0;
        i0 &= (kTableSize - 1);                      // power-of-two wrap
        const int i1 = (i0 + 1) & (kTableSize - 1);
        return table[i0] + frac * (table[i1] - table[i0]);
    }

    //==========================================================================
    double sampleRate = 44100.0;
    double f0         = 440.0;
    float  phase      = 0.0f;     // normalized single-cycle phase [0,1)
    float  phaseInc   = 0.0f;
    int    Kmax        = kNumPartials;
    float  velLevel    = 1.0f;

    // Morph endpoints. Frame A = the live drawbars; Frame B = the selected preset
    // (resolved in the processor, pushed each block). Default to a pure fundamental
    // so a voice prepared before its first setParams is benign.
    float frameASpectrum[kNumPartials] = { 1.0f };   // H1=1, rest 0
    float frameBSpectrum[kNumPartials] = { 1.0f };   // H1=1, rest 0 (≡ Sine until pushed)
    bool  spectrumDirty = true;

    // Scan pointer state. scanBase = global (manual + LFO); the per-voice mod-env
    // adds scanEnvAmount·modEnv on top; the sum is clamped + smoothed → currentScan.
    float scanBase      = 0.0f;
    float scanEnvAmount = 0.0f;
    float currentScan   = 0.0f;       // last scan the table was filled at
    float lastModEnv    = 0.0f;       // mod-env value at end of previous block
    juce::SmoothedValue<float> scanSmoothed { 0.0f };

    // Below this smoothed-scan delta we skip the refill — bounds static-patch work
    // to once per note while staying well under the morph's audible step threshold.
    static constexpr float kScanRefillEps = 1.0e-4f;

    //--- Refill-cadence cap (PERF) -------------------------------------------
    // refillTable() rebuilds the whole single-cycle table — a FIXED ~2048×16
    // sine-sum cost per call, INDEPENDENT of the host block size. During
    // continuous motion (scan/LFO/mod-env/spectral-decay) the table is marked
    // dirty every block, so on small host blocks the rebuild fires far more often
    // per second than on large ones and the cost does not amortize (measured
    // ~37 % of a core for a 16-voice Morph-Pad chord at 64-sample blocks, vs
    // ~4.7 % at 512). We bound MOTION-driven refills to a control-rate interval
    // (~5 ms): a minimum number of samples must elapse between rebuilds.
    //   • Static patches are untouched — they never set spectrumDirty in
    //     renderNextBlock (the refill stays once-per-note in startNote), so the
    //     gate below is never reached and their output is BIT-IDENTICAL.
    //   • Large host blocks (numSamples ≥ minRefillSamples) still refill every
    //     block exactly as before — the cap is a no-op there.
    //   • 5 ms is far finer than the refill cadence a ≥512-sample host already
    //     uses (≈11.6 ms @ 44.1 kHz), so the 20 ms scan smoother's zipper-free
    //     guarantee is preserved by construction (the per-refill scan step stays
    //     below the already-validated large-block case).
    static constexpr float kRefillIntervalSeconds = 0.005f;
    int minRefillSamples   = 1;     // resolved from the sample rate in prepareToPlay
    int samplesSinceRefill = 0;     // samples elapsed since the last refillTable()

    //--- Spectral-decay state (2.3) ------------------------------------------
    // tau: internal 0→1 ramp from note-on, advanced at control rate. The decay
    // tilt grows over the note even on a sustained note (the "spectrum darkens
    // over time" motion, DSP-04). kTauRampSeconds ≈ 2 s gives a musical sweep.
    // kDecayRateMax scales the knob: at 100% decay, tau=1, the 16th harmonic
    // drops to ≈ −46 dB (see effectiveDecayRate).
    static constexpr float kTauRampSeconds = 2.0f;
    static constexpr float kDecayRateMax   = 0.35f;
    float tau              = 0.0f;     // 0→1 over kTauRampSeconds from note-on
    float spectralDecay    = 0.0f;     // 0..1 knob (pushed each block)
    float velToDecay       = 0.0f;     // 0..1 opt-in velocity→decay amount
    float currentDecayRate = 0.0f;     // effective rate for the current block
    float lastRenderedDecayRate = 0.0f; // rate at the last dirty-check → knob moves re-dirty the table

    //--- Bit-depth quantizer (2.3) ------------------------------------------
    int bitDepthBits = 0;             // 0 = Off (passthrough); else bit count {12,10,8,6,4,2}

    //--- Active-spectrum snapshot (2.3) -------------------------------------
    // The post-morph, post-decay, post-band-limit, pre-norm 16 amplitudes — the
    // exact bars the engine is summing. Filled each refillTable(); read by the
    // processor (audio thread) which publishes the primary voice's copy to the
    // editor. Seeded to a pure fundamental so a freshly prepared voice is benign.
    float activeSpectrum[kNumPartials] = { 1.0f };

    //--- Primary-voice selection (2.3) --------------------------------------
    // noteAge: per-voice monotonic stamp set in startNote from the shared counter.
    // The processor picks the ACTIVE voice with the largest noteAge as the one
    // whose active spectrum drives the drawbar display (newest sounding note wins).
    std::uint64_t noteAge = 0;
    static inline std::atomic<std::uint64_t> sNoteCounter { 0 };

    alignas (16) float table[kTableSize] = { 0.0f };

    juce::ADSR ampEnv;
    juce::ADSR::Parameters ampParams { 0.005f, 0.3f, 0.8f, 0.1f };
    juce::ADSR modEnv;
    juce::ADSR::Parameters modParams { 0.005f, 0.3f, 0.8f, 0.1f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdditiveVoice)
};
