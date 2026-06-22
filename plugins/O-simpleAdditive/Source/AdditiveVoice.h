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

        // Start with a clean (silent) table so an un-triggered voice is benign.
        juce::FloatVectorOperations::clear (table, kTableSize);
        spectrumDirty = true;
    }

    //==========================================================================
    // Block param-push from the processor (once per block). Phase 2.1: Frame A
    // drawbars + amp ADSR only. Marks the table dirty only when the spectrum
    // actually moved, so a static patch refills once per note rather than once
    // per block.
    void setParams (const float (&frameA)[kNumPartials],
                    const juce::ADSR::Parameters& ap) noexcept
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
        }

        ampParams = ap;
        ampEnv.setParameters (ampParams);
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
        refillTable();                // first sample must be correct → fill now

        ampEnv.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();
        }
        else
        {
            clearCurrentNote();
            ampEnv.reset();
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

        // Control-rate table refill: at most once per block, only if the active
        // spectrum changed (drawbar move). Note-on already refilled.
        if (spectrumDirty)
            refillTable();

        const int numCh = out.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const float s        = readTableLinear (phase);
            const float ampVal   = ampEnv.getNextSample();
            const float sample   = s * ampVal * velLevel;

            for (int ch = 0; ch < numCh; ++ch)
                out.addSample (ch, startSample + i, sample);

            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;
        }

        // Voice lifetime keyed on the amp envelope only.
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
    // Fill the single-cycle table from the active spectrum. Phase 2.1: the
    // active spectrum IS the band-limited Frame A drawbars. (Phase 2.2 inserts
    // morph; 2.3 inserts spectral-decay — both act here, BEFORE the sum, so the
    // table is always exactly what is heard and, later, displayed.)
    void refillTable() noexcept
    {
        float band[kNumPartials];
        float sumA = 0.0f;

        for (int k = 0; k < kNumPartials; ++k)
        {
            // --- Phase 2.2 morph + 2.3 spectral-decay will compose into here ---
            const float activeK = frameASpectrum[k];

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

    // Active spectrum source (Phase 2.1: raw Frame A drawbars). Default to a
    // pure fundamental so a voice prepared before its first setParams is benign.
    float frameASpectrum[kNumPartials] = { 1.0f };   // H1=1, rest 0
    bool  spectrumDirty = true;

    alignas (16) float table[kTableSize] = { 0.0f };

    juce::ADSR ampEnv;
    juce::ADSR::Parameters ampParams { 0.005f, 0.3f, 0.8f, 0.1f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdditiveVoice)
};
