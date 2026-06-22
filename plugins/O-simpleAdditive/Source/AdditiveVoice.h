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
      • Phase 2.1 (THIS FILE): core voice — Frame A drawbars + amp ADSR only.
      • Phase 2.2: Frame A→B spectral morph + scan LFO + mod-env → scan.
      • Phase 2.3: spectral-decay tilt + bit-depth quantizer + viz snapshot.
    The refillTable()/active-spectrum pipeline below is deliberately staged so
    2.2/2.3 slot in at the marked extension points without reshaping the voice.

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL custom prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast (O-simpleFM FMVoice / O-Bassoon pattern). setSampleRate MUST
    precede the first setParameters.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>

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

        ampEnv.setSampleRate (sr);
        ampEnv.setParameters (ampParams);
        modEnv.setSampleRate (sr);
        modEnv.setParameters (modParams);

        // Scan pointer smoother (~20 ms) — see ARCHITECTURE.md §"Smoothing / zipper
        // avoidance". Post-sum smoothing keeps the morph zipper-free at control-rate
        // table refills (DSP-03).
        scanSmoothed.reset (sr, 0.02);

        // Start with a clean (silent) table so an un-triggered voice is benign.
        juce::FloatVectorOperations::clear (table, kTableSize);
        spectrumDirty = true;
    }

    //==========================================================================
    // Block param-push from the processor (once per block). Phase 2.2 adds the
    // wavetable dimension: Frame B target vector, the GLOBAL scan base (manual +
    // LFO, resolved once in the processor), the bipolar mod-env→scan depth, and
    // the mod-ADSR shape. Marks the table dirty only when the spectrum sources
    // (Frame A / Frame B) actually moved; scan motion is detected per-block in
    // renderNextBlock, so a fully static patch still refills once per note.
    void setParams (const float (&frameA)[kNumPartials],
                    const float (&frameB)[kNumPartials],
                    float scanBaseIn, float scanEnvAmountIn,
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

        ampParams = ap;
        ampEnv.setParameters (ampParams);
        modParams = mp;
        modEnv.setParameters (modParams);
    }

    //==========================================================================
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int /*pitchWheel*/) override
    {
        currentNote = midiNote;
        f0          = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        phaseInc    = (float) (f0 / sampleRate);

        velLevel = juce::jlimit (0.0f, 1.0f, velocity);

        phase = 0.0f;                 // restart the single-cycle read at zero phase
        computeKmax();                // per-note band-limit

        // Seed the scan pointer at the note's starting position (mod-env just
        // retriggered → value ≈ 0), with no smoother ramp from a stale value.
        lastModEnv = 0.0f;
        const float scan0 = juce::jlimit (0.0f, 1.0f, scanBase + lastModEnv * scanEnvAmount);
        scanSmoothed.setCurrentAndTargetValue (scan0);
        currentScan = scan0;

        refillTable (currentScan);    // first sample must be correct → fill now

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

        if (spectrumDirty)
            refillTable (currentScan);

        const int numCh = out.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const float s        = readTableLinear (phase);
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
    // Per-note Nyquist band-limit: highest alias-free harmonic count.
    void computeKmax() noexcept
    {
        const double nyquist = 0.5 * sampleRate;
        const int    k       = (int) std::floor (nyquist / juce::jmax (1.0, f0));
        Kmax = juce::jlimit (1, kNumPartials, k);
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
    // Fill the single-cycle table from the active spectrum. Phase 2.2: the active
    // spectrum is the per-partial morph between Frame A (drawbars) and Frame B
    // (preset), `active_k = lerp(A_k, B_k, scan)` — linear *spectral* interpolation,
    // phase-coherent and zipper-free (ARCHITECTURE.md §Morph, DSP-03). (Phase 2.3
    // inserts spectral-decay here too — also BEFORE the sum, so the table is always
    // exactly what is heard and, later, displayed.)
    void refillTable (float scan) noexcept
    {
        float band[kNumPartials];
        float sumA = 0.0f;

        for (int k = 0; k < kNumPartials; ++k)
        {
            // Morph (2.2): per-partial lerp A→B. --- 2.3 spectral-decay composes here ---
            const float activeK = frameASpectrum[k]
                                + scan * (frameBSpectrum[k] - frameASpectrum[k]);

            band[k] = activeK * nyquistGain (k + 1, Kmax);   // exact band-limit
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
                if (band[k] != 0.0f)
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
    int    currentNote = 69;
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

    alignas (16) float table[kTableSize] = { 0.0f };

    juce::ADSR ampEnv;
    juce::ADSR::Parameters ampParams { 0.005f, 0.3f, 0.8f, 0.1f };
    juce::ADSR modEnv;
    juce::ADSR::Parameters modParams { 0.005f, 0.3f, 0.8f, 0.1f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdditiveVoice)
};
