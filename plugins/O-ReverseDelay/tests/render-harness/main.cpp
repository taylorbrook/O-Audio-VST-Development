/*
  ==============================================================================

    O-ReverseDelay render-harness — Stage 2 DSP correctness gate (D5).

    Instantiates ReverseDelayProcessor directly and drives it as an EFFECT
    (input buffers filled per block, empty MidiBuffer — no MIDI, no DAW).
    Every acceptance criterion is a hard pass/fail assertion; exit 0 iff all
    probes pass. Off by default; enable with -DOUARICON_BUILD_TESTS=ON.

    Phase 2.1 probes:
      0. silence-pass       — Stage scaffold gate: silence in -> silence out,
                              finite, no spurious wet energy.
      A. reversed-ramp      — (FUNC-01 direction) single grain of a rising
                              linear ramp plays FALLING: de-windowed slope is
                              negative AND non-trivial in magnitude (catches
                              the frozen-read D+n bug), and the de-windowed
                              output correlates with the reversed source
                              region, anti-correlates with the forward one.
      B. impulse-bloom      — (FUNC-01 bloom) an impulse re-emerges as a
                              reverse swell: envelope peak inside the bloom
                              support, pre-peak energy ramps up, no hard
                              leading edge (the anti-"chunked block" check).
      C. click-detector     — (DSP-01) first-difference bound on a smooth
                              220 Hz sine at defaults and during a density
                              0->100->0 sweep; allFinite on every render.
      D. density-flatness   — (DSP-01) wet RMS flat within ±1 dB across
                              density {0,25,50,75,100}; freezes the
                              1/sqrt(overlap) compensation constant.
      E. single-generation  — (FUNC-03 precondition) feedback=0 -> all wet
                              energy confined to the first bloom window
                              (no echo at T+2D), < -80 dB relative outside.

    Phase 2.2 probes:
      F. damping-generations— (FUNC-03) feedback=60 + tight damping: gen 2
                              exists, loses energy, spectral centroid falls
                              (LP highCut) and the 20-150 Hz fraction falls
                              (HP lowCut). NO direction assertions past gen 1
                              (alternating-direction regens are intended).
      G. stability-60s      — (DSP-03) feedback=100, default damping, 2 s
                              excitation then silence: every sample finite,
                              peak < 1.0, last-10 s bounded, wash persists.
      H. cutoff-sweeps      — (QUAL-01 partial) lowCut and highCut ramped
                              full-range during playback: no clicks/zipper
                              via the probe-C first-difference detector.

    Phase 2.3 probes:
      I. sync-spacing       — (FUNC-02) MockPlayHead @120 BPM, Sync 1/4 ->
                              first-echo latency 0.5 s ±1 block; free-mode
                              variants at 150/500/1200 ms. The raw onset is
                              quantised to the grain spawn grid (jitter up to
                              2*interval > 1 block at legal grain sizes), so
                              the tight latency check uses the ENERGY CENTROID
                              of the hann^2-symmetric bloom (= T+D+G for any
                              grid phase); the onset keeps a grid-bounded
                              sanity window.
      J. no-playhead        — (COMPAT-02) setPlayHead never called (fresh
                              instance), and bonus getBpm()==nullopt case:
                              Sync mode falls back to delayTime, no crash,
                              no silence.
      K. width              — (FUNC-04) width=0 -> centered dual-mono (L/R
                              RMS match + corr ~1); width=100 -> wet-tail
                              corr < 0.9 + frame-level |L-R| energy well
                              above the width-0 run.
      L. mono->stereo       — (D4 open item) 1-in/2-out: output L == R,
                              wet level matches the stereo width-0 run
                              within 0.5 dB.
      M. all-param sweep    — (QUAL-01) each of the 10 params ramped
                              full-range (triangle over the render), one at
                              a time, others at defaults; probe-C click
                              detector in two tiers (latched content params
                              legitimately re-seat grain reads) + allFinite
                              + peak bound; plus a Sync<->Free mode switch
                              mid-playback.

    v1.0.1 probes (the three defects from the 2026-07-24 review):
      O. blocksize-invariance— (A2) 512- vs 4096-sample render of the same
                              position-deterministic noise at delayTime=50
                              must agree sample-for-sample. Pre-fix the
                              4096 run reads a full ring lap of stale
                              capture. THE HARNESS NEVER VARIED BLOCK SIZE
                              before v1.0.1, which is why A2 shipped.
      P. delaytime-range    — (A1) 1/1 at 60 BPM must render a 4000 ms
                              bloom (v1.0.0 clamped it to 2000); the range
                              max is reachable and the skew centre still
                              sits at 316 ms; and an APVTS session round
                              trip recalls 1400 ms literally — session
                              state is DENORMALISED and must NOT be
                              rescaled when the range grows.
      Q. density-0-continuity—(A3) constant input, density 0: the wet
                              overlap-add envelope must be flat. At
                              v1.0.0's overlap=1 it fell to zero at every
                              grain boundary (100 %-depth 5 Hz tremolo).
      R. preset-migration   — (A1) a synthesised v1.0.0 preset (normalised
                              delayTime under the old 2000 ms range) must
                              recall its original ms after the constructor's
                              one-shot migration.
      S. decay-fb100        — (A3) loop decay at feedback=100 measured at
                              the overlap-matched and the new density, so
                              the remap's effect on the loop duty cycle is
                              a printed number rather than an assumption.

    v1.1.0 probes (B3 grain randomisation + grain-pool refusal):
      T. random-live        — each of the four randomisations measurably
                              changes the render (no dead controls), AND two
                              independent all-zero renders are BIT-IDENTICAL:
                              every randomisation must draw nothing from the
                              shared xorshift when off, or the pan sequence
                              shifts and v1.0 sessions change sound. Also
                              pins the new per-instance seed as per-INSTANCE
                              rather than per-prepare, which probe O needs.
      U. level-flat         — all four are symmetric about nominal (and
                              gainRandom is power-normalised), so wet RMS
                              stays within ±1 dB across {0, 50, 100 %} for
                              each. Catches a "character" knob that is
                              really a loudness knob.
      V. jitter-breaks-grid — the B3 #1 claim, measured: at density 0 the
                              overlap-add of a REGULAR grid is exactly flat
                              (probe Q), so flatness reads grid regularity
                              directly. Asserts both ends — flat at 0,
                              broken at 100 — so a dead jitter FAILS.
      W. scatter-ring       — W1: the ring's worst case, gD+2G = 5.5 s
                              against the 6.0 s ring (scatter can push a
                              LATCHED delay past the delayTime max).
                              W2: 512- vs 4096-sample bit equality with all
                              four randomisations ON — negative scatter
                              shortens a grain's delay, so A2's pass bound
                              needs the latched-value clamp to hold.
      X. gainrandom-loop    — loop decay at feedback=100 with gainRandom 0
                              vs 100 must match: the wet/loop buffer split
                              is what keeps the randomised gain downstream
                              of the feedback tap. Tap the wrong buffer and
                              the decay RATE becomes stochastic.
      Y. pool-pressure      — grainSize swept under max randomisation: no
                              clicks, no NaN, bounded peak. v1.0 stole the
                              oldest slot here and cut a live Hann envelope
                              to zero; v1.1 refuses the spawn. Peak grain
                              concurrency is reported, not asserted.

    v1.2.0 probes (B1 grain window shape + tilt):
      Z1. window-identity   — the compatibility guarantee, as MECHANISM: the
                              tilt warp at 0.5 is the BITWISE identity over a
                              4097-point phase sweep, both power-normalisation
                              constants are exactly 1.0f at (Hann, 0.5), and
                              getTiltNorm is exactly 1.0f at EVERY tilt for all
                              four symmetric shapes (the warp's power
                              invariance). Plus a rendered default-vs-explicit
                              equality, and a printed table of every shape's
                              power duty cycle. The cross-VERSION half of the
                              claim cannot live in one binary: the v1.1.0
                              harness was rebuilt from commit 8fa3646 and its
                              63 result lines diffed byte-for-byte against
                              this build's (see CHANGELOG).
      Z2. level-flat-shape  — wet RMS within ±1 dB across all five shapes at a
                              fixed density. Uncompensated, Tukey's 1.83x duty
                              would put it +2.6 dB — a timbre control read as
                              a volume control.
      Z3. level-flat-tilt   — same ±1 dB across tilt {0,.25,.5,.75,1}, run for
                              Hann (tests the WARP, whose tiltNorm is exactly
                              1.0f) and for Expo-Decay (the only asymmetric
                              shape, i.e. the only one where the tilt
                              normalisation arithmetic actually runs).
      Z4. decay-fb100-shape — loop decay dB/s at feedback=100 for each shape.
                              grainGain sits BEFORE the feedback tap, so the
                              window's duty sets per-generation loss; without
                              normalisation "shape" silently becomes "tail
                              length". Every shape's number is printed.
      Z5. window-live       — the mirror of probe T: every other Z probe is a
                              "must not change", which a control wired to
                              nothing satisfies perfectly. Each non-default
                              shape and BOTH tilt extremes must measurably
                              move the render.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

//==============================================================================
// Small helpers (lifted from the O-simpleGrain harness)

static void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float real)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (real));
    else
        std::printf ("  !! unknown param id '%s'\n", id);
}

// Read back the actual (skew/step-snapped) engineering value so the harness
// derives D/G with EXACTLY the same rounding as the processor.
static float paramValue (juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    return apvts.getRawParameterValue (id)->load();
}

static double rms (const std::vector<float>& x, int off, int len)
{
    if (len <= 0 || off < 0 || off + len > (int) x.size()) return 0.0;
    double acc = 0.0;
    for (int n = 0; n < len; ++n) { const double s = x[(size_t) (off + n)]; acc += s * s; }
    return std::sqrt (acc / (double) len);
}

static double peakAbs (const std::vector<float>& x)
{
    double p = 0.0;
    for (float s : x) p = juce::jmax (p, (double) std::abs (s));
    return p;
}

static bool allFinite (const std::vector<float>& x)
{
    for (float s : x) if (! std::isfinite (s)) return false;
    return true;
}

static double maxAbsStep (const std::vector<float>& x)
{
    double m = 0.0;
    for (size_t i = 1; i < x.size(); ++i)
        m = juce::jmax (m, (double) std::abs (x[i] - x[i - 1]));
    return m;
}

// Energy-weighted centroid (sample index) of x over [off, off+len); -1 if empty.
static double energyCentroid (const std::vector<float>& x, int off, int len)
{
    const int lo = juce::jmax (0, off);
    const int hi = juce::jmin ((int) x.size(), off + len);
    double num = 0.0, den = 0.0;
    for (int i = lo; i < hi; ++i)
    {
        const double e = (double) x[(size_t) i] * (double) x[(size_t) i];
        num += e * (double) i;
        den += e;
    }
    return den > 0.0 ? num / den : -1.0;
}

// First index with |x[i]| > thresh, or -1.
static int firstAbove (const std::vector<float>& x, double thresh)
{
    for (size_t i = 0; i < x.size(); ++i)
        if ((double) std::abs (x[i]) > thresh)
            return (int) i;
    return -1;
}

// Mean-removed Pearson correlation between channels a/b over [off, off+len).
static double corrRange (const std::vector<float>& a, const std::vector<float>& b,
                         int off, int len)
{
    const int lo = juce::jmax (0, off);
    const int hi = juce::jmin ((int) juce::jmin (a.size(), b.size()), off + len);
    if (hi - lo < 2) return 0.0;

    const double n = (double) (hi - lo);
    double ma = 0.0, mb = 0.0;
    for (int i = lo; i < hi; ++i) { ma += a[(size_t) i]; mb += b[(size_t) i]; }
    ma /= n; mb /= n;

    double num = 0.0, ea = 0.0, eb = 0.0;
    for (int i = lo; i < hi; ++i)
    {
        const double da = (double) a[(size_t) i] - ma;
        const double db = (double) b[(size_t) i] - mb;
        num += da * db;
        ea  += da * da;
        eb  += db * db;
    }
    const double den = std::sqrt (ea * eb);
    return den > 1.0e-12 ? num / den : 0.0;
}

// RMS of the channel difference (frame-level |L-R| energy) over [off, off+len).
static double diffRms (const std::vector<float>& a, const std::vector<float>& b,
                       int off, int len)
{
    const int lo = juce::jmax (0, off);
    const int hi = juce::jmin ((int) juce::jmin (a.size(), b.size()), off + len);
    if (hi <= lo) return 0.0;

    double acc = 0.0;
    for (int i = lo; i < hi; ++i)
    {
        const double d = (double) a[(size_t) i] - (double) b[(size_t) i];
        acc += d * d;
    }
    return std::sqrt (acc / (double) (hi - lo));
}

static double maxAbsDiff (const std::vector<float>& a, const std::vector<float>& b)
{
    const size_t n = juce::jmin (a.size(), b.size());
    double m = 0.0;
    for (size_t i = 0; i < n; ++i)
        m = juce::jmax (m, (double) std::abs (a[i] - b[i]));
    return m;
}

// Short-window RMS-envelope continuity (scaffold — used from Phase 2.2 on).
[[maybe_unused]] static double continuityFraction (const std::vector<float>& x, int off, int len,
                                                   double fs, double frameMs, double ratio)
{
    const int fn = juce::jmax (1, (int) (frameMs * 0.001 * fs));
    const int nFrames = len / fn;
    if (nFrames <= 0) return 0.0;

    std::vector<double> env ((size_t) nFrames, 0.0);
    double mean = 0.0;
    for (int f = 0; f < nFrames; ++f)
    {
        env[(size_t) f] = rms (x, off + f * fn, fn);
        mean += env[(size_t) f];
    }
    mean /= (double) nFrames;
    if (mean <= 0.0) return 0.0;

    int above = 0;
    for (double e : env) if (e > ratio * mean) ++above;
    return (double) above / (double) nFrames;
}

// Least-squares slope of y against x.
struct FitResult { double slope = 0.0; };

static FitResult linearFit (const std::vector<double>& x, const std::vector<double>& y)
{
    FitResult r;
    const size_t n = juce::jmin (x.size(), y.size());
    if (n < 2) return r;

    double mx = 0.0, my = 0.0;
    for (size_t i = 0; i < n; ++i) { mx += x[i]; my += y[i]; }
    mx /= (double) n; my /= (double) n;

    double num = 0.0, den = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        num += (x[i] - mx) * (y[i] - my);
        den += (x[i] - mx) * (x[i] - mx);
    }
    r.slope = den > 0.0 ? num / den : 0.0;
    return r;
}

// Mean-removed Pearson correlation.
static double pearson (const std::vector<double>& a, const std::vector<double>& b)
{
    const size_t n = juce::jmin (a.size(), b.size());
    if (n < 2) return 0.0;

    double ma = 0.0, mb = 0.0;
    for (size_t i = 0; i < n; ++i) { ma += a[i]; mb += b[i]; }
    ma /= (double) n; mb /= (double) n;

    double num = 0.0, ea = 0.0, eb = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double da = a[i] - ma, db = b[i] - mb;
        num += da * db;
        ea  += da * da;
        eb  += db * db;
    }
    const double den = std::sqrt (ea * eb);
    return den > 1.0e-12 ? num / den : 0.0;
}

//==============================================================================
// Magnitude spectrum of a Hann-windowed segment (scaffold for Phase 2.2's
// damping-generation probe F).
struct Spectrum
{
    std::vector<float> mag;     // size N/2+1
    double             fs = 0;
    int                fftSize = 0;

    double binHz() const { return fs / (double) fftSize; }

    double centroid (double loHz, double hiHz) const
    {
        double num = 0.0, den = 0.0;
        for (size_t k = 0; k < mag.size(); ++k)
        {
            const double f = (double) k * binHz();
            if (f < loHz || f > hiHz) continue;
            num += f * mag[k];
            den += mag[k];
        }
        return den > 0 ? num / den : 0.0;
    }

    double bandEnergy (double loHz, double hiHz) const
    {
        double e = 0.0;
        for (size_t k = 0; k < mag.size(); ++k)
        {
            const double f = (double) k * binHz();
            if (f >= loHz && f <= hiHz) e += (double) mag[k] * mag[k];
        }
        return e;
    }
};

[[maybe_unused]] static Spectrum analyze (const std::vector<float>& x, int off, double fs, int order = 14)
{
    const int N = 1 << order;
    juce::dsp::FFT fft (order);

    std::vector<float> buf ((size_t) (2 * N), 0.0f);
    juce::dsp::WindowingFunction<float> win ((size_t) N,
                                             juce::dsp::WindowingFunction<float>::hann);
    std::vector<float> seg ((size_t) N, 0.0f);
    for (int n = 0; n < N; ++n)
    {
        const int idx = off + n;
        seg[(size_t) n] = (idx >= 0 && idx < (int) x.size()) ? x[(size_t) idx] : 0.0f;
    }
    win.multiplyWithWindowingTable (seg.data(), (size_t) N);
    for (int n = 0; n < N; ++n) buf[(size_t) n] = seg[(size_t) n];

    fft.performRealOnlyForwardTransform (buf.data());

    Spectrum s;
    s.fs = fs; s.fftSize = N;
    s.mag.resize ((size_t) (N / 2 + 1));
    for (int k = 0; k <= N / 2; ++k)
    {
        const float re = buf[(size_t) (2 * k)];
        const float im = buf[(size_t) (2 * k + 1)];
        s.mag[(size_t) k] = std::sqrt (re * re + im * im);
    }
    return s;
}

//==============================================================================
// Mock playhead (scaffold — Phase 2.3 sync probes I/J; RESEARCH §1).
// setBpm({}) simulates a host with no tempo; never calling setPlayHead at all
// is the COMPAT-02 no-playhead case.
struct MockPlayHead : juce::AudioPlayHead
{
    juce::Optional<double> bpm { 120.0 };
    bool playing = true;

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo pi;
        pi.setBpm (bpm);
        pi.setIsPlaying (playing);
        return pi;
    }
};

//==============================================================================
// Effect-driving render loop: fill the stereo input per block (same signal on
// L and R), process with an empty MidiBuffer, capture both output channels.
struct StereoRender
{
    std::vector<float> L, R;
};

template <typename FillFn, typename BlockFn>
static StereoRender renderEffect (ReverseDelayProcessor& proc, double seconds, double fs,
                                  int block, FillFn&& fill, BlockFn&& perBlock)
{
    const int total = (int) (seconds * fs);
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;

    StereoRender out;
    out.L.reserve ((size_t) total);
    out.R.reserve ((size_t) total);

    int pos = 0;
    while (pos < total)
    {
        perBlock (pos, total);

        for (int i = 0; i < block; ++i)
        {
            const float v = fill (pos + i);
            buf.setSample (0, i, v);
            buf.setSample (1, i, v);
        }

        proc.processBlock (buf, midi);

        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i)
        {
            out.L.push_back (buf.getSample (0, i));
            out.R.push_back (buf.getSample (1, i));
        }
        pos += block;
    }
    return out;
}

template <typename FillFn>
static StereoRender renderEffect (ReverseDelayProcessor& proc, double seconds, double fs,
                                  int block, FillFn&& fill)
{
    return renderEffect (proc, seconds, fs, block, std::forward<FillFn> (fill),
                         [] (int, int) {});
}

//==============================================================================
// Probe baseline: wet-only, no feedback, no width, free 500 ms / 200 ms grain.
// Individual probes override what they need.
static void setBaseline (juce::AudioProcessorValueTreeState& a)
{
    setParam (a, "delayTime",    500.0f);
    setParam (a, "syncMode",       0.0f);   // Free — Sync exercised by probes I/J/M
    setParam (a, "noteDivision",   6.0f);   // 1/4 (inert in Phase 2.1)
    setParam (a, "grainSize",    200.0f);
    setParam (a, "density",       60.0f);
    setParam (a, "feedback",       0.0f);
    setParam (a, "lowCut",       100.0f);
    setParam (a, "highCut",     8000.0f);
    setParam (a, "width",          0.0f);
    setParam (a, "mix",          100.0f);

    // v1.2.0 (B1): the SHIPPED window, so every probe written before this
    // release measures exactly what it always measured.
    //
    // These belong here rather than in a clearWindow() the way v1.1's four
    // randomisations live in clearRandomisation(), and that is a correction
    // rather than a style choice. Probe M sweeps grainTilt and grainShape and
    // leaves them wherever its triangle ended; probes P, Q and V run afterwards,
    // call setBaseline(), and would silently inherit a tilted or non-Hann
    // window. All three DID fail that way on the first run of this release —
    // probe Q's constant-overlap-add flatness fell from 1.0000 to 0.3718 — and
    // every one of them looked like a DSP regression rather than harness state
    // leaking forward. Resetting at the source makes the leak impossible
    // instead of making it every future probe's job to remember.
    setParam (a, "grainTilt",      0.5f);   // symmetric — NOT 0
    setParam (a, "grainShape",     0.0f);   // Hann

    // v1.3.0 (B2): the SHIPPED ceiling, for the same reason and by the same
    // hard-won argument as the two window params above — probe AA sweeps
    // grainCount and every probe that runs after it calls setBaseline().
    // 8 is v1.2.0's hard-coded value, so pre-v1.3.0 probes measure what they
    // always measured. NOT kOverlapCeilingMax, and not 2.
    setParam (a, "grainCount",     8.0f);

    // v1.4.0: the SHIPPED taper. Same argument as grainTilt/grainShape/grainCount
    // above, and the same trap in a fourth disguise — the no-op is 0.5, not the
    // range minimum. Probes AH-AK sweep it and every probe after them calls
    // setBaseline().
    setParam (a, "tukeyTaper",     0.5f);
}

// Plugin defaults for the QUAL-01 all-parameter sweep (probe M): every value
// matches createParameterLayout()'s defaults (incl. syncMode = Sync).
static void setDefaults (juce::AudioProcessorValueTreeState& a)
{
    setParam (a, "delayTime",    500.0f);
    setParam (a, "syncMode",       1.0f);   // Sync (plugin default)
    setParam (a, "noteDivision",   6.0f);   // 1/4
    setParam (a, "grainSize",    200.0f);
    setParam (a, "density",       60.0f);
    setParam (a, "feedback",      40.0f);
    setParam (a, "lowCut",       100.0f);
    setParam (a, "highCut",     8000.0f);
    setParam (a, "width",         60.0f);
    setParam (a, "mix",           35.0f);
    setParam (a, "grainTilt",      0.5f);   // v1.2.0 default — see setBaseline
    setParam (a, "grainShape",     0.0f);
    setParam (a, "grainCount",     8.0f);   // v1.3.0 default
    setParam (a, "tukeyTaper",     0.5f);   // v1.4.0 default
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const double fs    = 48000.0;
    const int    block = 512;

    ReverseDelayProcessor proc;
    proc.setPlayConfigDetails (2, 2, fs, block);
    proc.prepareToPlay (fs, block);
    auto& apvts = proc.parameters;

    int failures = 0;
    auto check = [&] (const char* name, bool ok, const juce::String& detail)
    {
        std::printf ("  [%s] %-22s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    };

    // Derive D/G exactly as the processor does, from the snapped param values.
    auto currentD = [&] { return juce::jmax (1, (int) (paramValue (apvts, "delayTime") * 0.001 * fs)); };
    auto currentG = [&] { return juce::jmax (2, (int) (paramValue (apvts, "grainSize") * 0.001 * fs)); };

    std::printf ("O-ReverseDelay render-harness — probes 0, A–M (Stage 2), O–S (v1.0.1) "
                 "+ N (Stage 4), fs=%.0f block=%d\n", fs, block);

    // --- Probe 0: silence pass (scaffold gate) -------------------------------
    {
        setBaseline (apvts);
        proc.prepareToPlay (fs, block);
        auto y = renderEffect (proc, 0.5, fs, block, [] (int) { return 0.0f; });
        const double p = juce::jmax (peakAbs (y.L), peakAbs (y.R));
        check ("silence-pass", allFinite (y.L) && allFinite (y.R) && p < 1.0e-6,
               juce::String ("peak=") + juce::String (p, 9));
    }

    // --- Probe A: single-grain reversed ramp (FUNC-01 direction) -------------
    // v1.0.1: density=0 no longer isolates a grain. The A3 remap floors overlap
    // at 2, so the hop is G/2 and two grains are always in flight — de-windowing
    // the raw output would mix both. Isolation now comes from the SOURCE instead:
    // the input is a ramp BURST exactly G long, positioned so that only the grain
    // spawning at `start` reads inside it.
    //
    // With spawns every G/2 and reads stepping −1 from (spawn − D):
    //   grain @ start        reads (rampStart, rampStart+G]  -> inside the burst
    //   grain @ start − G/2  reads [rampStart−G/2, rampStart] -> silence
    //   grain @ start + G/2  reads (rampEnd, rampEnd+G/2]     -> silence
    // so y[start+n] carries exactly one grain for the whole n in [0, G).
    // This is a stronger isolation than v1.0.0's density trick — it holds at any
    // overlap rather than depending on the hop equalling G.
    {
        setBaseline (apvts);
        setParam (apvts, "density", 0.0f);
        proc.prepareToPlay (fs, block);

        const int D = currentD();
        const int G = currentG();

        // +2 (not +1) so rampStart = start − D − G stays positive.
        const int k         = (int) std::ceil ((double) D / (double) G) + 2;
        const int start     = k * G;
        const int rampStart = start - D - G;

        auto ramp = [rampStart, G] (int t) noexcept
        {
            return (t >= rampStart && t < rampStart + G)
                     ? (float) (t - rampStart) / (float) G
                     : 0.0f;
        };

        auto y = renderEffect (proc, 1.3, fs, block, [&] (int t) { return ramp (t); });

        // De-windowed values over the hann > 0.1 guard region.
        std::vector<double> xs, vs, srcRev, srcFwd;
        for (int n = 0; n < G; ++n)
        {
            const double h = 0.5 * (1.0 - std::cos (juce::MathConstants<double>::twoPi
                                                    * (double) n / (double) G));
            if (h <= 0.1) continue;
            const int idx = start + n;
            if (idx >= (int) y.L.size()) break;

            xs.push_back ((double) n);
            vs.push_back ((double) y.L[(size_t) idx] / h);
            srcRev.push_back ((double) ramp (start - D - n));               // reverse-read hypothesis
            srcFwd.push_back ((double) ramp (start - D - (G - 1) + n));     // forward-read hypothesis
        }

        const auto fit = linearFit (xs, vs);

        // pan gain (width=0 -> 1/sqrt(2)) * grainGain (1/sqrt(overlap), overlap=2
        // at density 0) / burst span. The 1/sqrt(overlap) factor is new in v1.0.1
        // — at v1.0.0's overlap=1 it was unity.
        const double expectedSlope = 0.70710677 / std::sqrt (2.0) / (double) G;
        const double corrRev       = pearson (vs, srcRev);
        const double corrFwd       = pearson (vs, srcFwd);
        const double segRms        = rms (y.L, start + G / 4, G / 2);

        // Slope must be negative (reversed) AND non-trivial (catches the frozen
        // D+n read bug whose de-windowed values are near-constant -> slope ~ 0).
        const bool ok = fit.slope < 0.0
                     && std::abs (fit.slope) > 0.3 * expectedSlope
                     && corrRev > 0.9
                     && corrFwd < 0.5
                     && corrRev > 5.0 * juce::jmax (corrFwd, 0.0)
                     && segRms > 1.0e-3
                     && allFinite (y.L) && allFinite (y.R);

        check ("reversed-ramp", ok,
               juce::String ("slope=") + juce::String (fit.slope, 9)
                 + " (expected~" + juce::String (-expectedSlope, 9) + ")"
                 + " corrRev=" + juce::String (corrRev, 3)
                 + " corrFwd=" + juce::String (corrFwd, 3)
                 + " segRms=" + juce::String (segRms, 4));
    }

    // --- Probe B: impulse reverse bloom (FUNC-01) ----------------------------
    // density=100 for a dense copy grid (each overlapping grain re-emits the
    // impulse once; the copies trace the bloom envelope).
    {
        setBaseline (apvts);
        setParam (apvts, "density", 100.0f);
        proc.prepareToPlay (fs, block);

        const int D = currentD();
        const int G = currentG();
        const int T = (int) fs;   // impulse at 1.0 s

        auto y = renderEffect (proc, 3.0, fs, block,
                               [T] (int t) { return t == T ? 1.0f : 0.0f; });

        const int frame = (int) (0.005 * fs);   // 5 ms frames
        const int nFrames = (int) y.L.size() / frame;

        int    peakFrame = 0;
        double peakEnv   = 0.0;
        for (int f = 0; f < nFrames; ++f)
        {
            const double e = rms (y.L, f * frame, frame);
            if (e > peakEnv) { peakEnv = e; peakFrame = f; }
        }
        const int peakPos = peakFrame * frame;

        // Impulse copies land at output = T + D + 2n, n in [0,G): the bloom
        // support is [T+D, T+D+2G] with the envelope peak near T+D+G.
        const int bloomStart = T + D;
        const bool peakInWindow = peakPos + frame / 2 >= T + D - G
                               && peakPos + frame / 2 <= T + D + 2 * G + frame;

        bool ramping = false, leadingSoft = false;
        double firstHalf = 0.0, secondHalf = 0.0, envFirst = 0.0;
        const int preLen = peakPos - bloomStart;
        if (preLen >= 4 * frame)
        {
            const int half = preLen / 2;
            firstHalf  = rms (y.L, bloomStart, half);
            secondHalf = rms (y.L, bloomStart + half, preLen - half);
            ramping    = secondHalf > 2.0 * firstHalf;

            envFirst    = rms (y.L, bloomStart, frame);
            leadingSoft = envFirst < 0.25 * peakEnv;   // no hard leading edge
        }

        const bool ok = peakEnv > 0.0
                     && peakInWindow
                     && ramping
                     && leadingSoft
                     && allFinite (y.L) && allFinite (y.R);

        check ("impulse-bloom", ok,
               juce::String ("peakPos=") + juce::String (peakPos)
                 + " (bloom " + juce::String (T + D - G) + ".." + juce::String (T + D + 2 * G) + ")"
                 + " preHalves=" + juce::String (firstHalf, 5) + "/" + juce::String (secondHalf, 5)
                 + " envFirst=" + juce::String (envFirst, 5)
                 + " peakEnv=" + juce::String (peakEnv, 5));
    }

    // --- Probe C: click detector (DSP-01) ------------------------------------
    // 220 Hz sine @ -12 dBFS. Legit step bound: A*2*pi*f/fs, factored up for
    // the equal-power dry+wet coherent sum (<= sqrt(2)) and window-envelope
    // slope; +10^(-60/20) margin per DSP-01. kStepFactor frozen per D5.
    {
        const double A  = std::pow (10.0, -12.0 / 20.0);
        const double f0 = 220.0;
        const double sineStep    = A * juce::MathConstants<double>::twoPi * f0 / fs;
        const double kStepFactor = 1.75;
        const double thresh      = kStepFactor * sineStep + std::pow (10.0, -60.0 / 20.0);

        auto sine = [=] (int t) noexcept
        {
            return (float) (A * std::sin (juce::MathConstants<double>::twoPi * f0 * (double) t / fs));
        };

        // Run 1: plugin defaults (feedback stubbed to 0 in Phase 2.1).
        setBaseline (apvts);
        setParam (apvts, "mix",      35.0f);
        setParam (apvts, "feedback", 40.0f);
        setParam (apvts, "density",  60.0f);
        setParam (apvts, "width",    60.0f);
        proc.prepareToPlay (fs, block);

        auto y1 = renderEffect (proc, 4.0, fs, block, sine);
        const double step1 = juce::jmax (maxAbsStep (y1.L), maxAbsStep (y1.R));

        check ("clicks-defaults",
               step1 < thresh && allFinite (y1.L) && allFinite (y1.R),
               juce::String ("maxStep=") + juce::String (step1, 6)
                 + " thresh=" + juce::String (thresh, 6));

        // Run 2: density 0 -> 100 -> 0 automation sweep during playback.
        setBaseline (apvts);
        setParam (apvts, "mix",      35.0f);
        setParam (apvts, "feedback", 40.0f);
        setParam (apvts, "width",    60.0f);
        setParam (apvts, "density",   0.0f);
        proc.prepareToPlay (fs, block);

        auto sweep = [&] (int pos, int total)
        {
            const double t = (double) pos / (double) total;
            const float  d = (float) ((t < 0.5 ? 2.0 * t : 2.0 - 2.0 * t) * 100.0);
            setParam (apvts, "density", d);
        };

        auto y2 = renderEffect (proc, 6.0, fs, block, sine, sweep);
        const double step2 = juce::jmax (maxAbsStep (y2.L), maxAbsStep (y2.R));

        check ("clicks-density-sweep",
               step2 < thresh && allFinite (y2.L) && allFinite (y2.R),
               juce::String ("maxStep=") + juce::String (step2, 6)
                 + " thresh=" + juce::String (thresh, 6));
    }

    // --- Probe D: density loudness flatness (DSP-01, tunes/freezes comp) -----
    // Broadband deterministic noise (identical sequence per render): grain
    // contributions at different capture offsets are uncorrelated, so powers
    // add and RMS isolates the 1/sqrt(overlap) compensation. (A pure sine is
    // unusable here: the fixed 2*interval read spacing phase-aligns copies at
    // harmonically-related densities and inflates RMS coherently.)
    {
        const double A = std::pow (10.0, -12.0 / 20.0);
        const float densities[] = { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f };
        double levels[5] = {};
        bool finiteAll = true;

        for (int i = 0; i < 5; ++i)
        {
            setBaseline (apvts);
            setParam (apvts, "density", densities[i]);
            proc.prepareToPlay (fs, block);

            juce::Random rng (0x5eed1234);   // same sequence every render
            auto noise = [&] (int) { return (float) (A * (rng.nextDouble() * 2.0 - 1.0)); };

            auto y = renderEffect (proc, 3.5, fs, block, noise);
            levels[i] = rms (y.L, (int) (1.5 * fs), (int) (2.0 * fs));
            finiteAll = finiteAll && allFinite (y.L) && allFinite (y.R);
        }

        double lo = levels[0], hi = levels[0];
        for (double l : levels) { lo = juce::jmin (lo, l); hi = juce::jmax (hi, l); }

        const double ratio    = lo > 0.0 ? hi / lo : 1.0e9;
        const double maxRatio = std::pow (10.0, 1.0 / 20.0);   // ±1 dB window

        check ("density-flatness",
               lo > 1.0e-4 && ratio <= maxRatio && finiteAll,
               juce::String ("rms={") + juce::String (levels[0], 5) + ", "
                 + juce::String (levels[1], 5) + ", " + juce::String (levels[2], 5) + ", "
                 + juce::String (levels[3], 5) + ", " + juce::String (levels[4], 5) + "}"
                 + " spread=" + juce::String (20.0 * std::log10 (ratio > 0 ? ratio : 1.0), 3) + " dB");
    }

    // --- Probe E: feedback=0 -> exactly one generation (FUNC-03 pre) ---------
    {
        setBaseline (apvts);
        setParam (apvts, "density", 100.0f);
        proc.prepareToPlay (fs, block);

        const int D = currentD();
        const int G = currentG();
        const int T = (int) fs;
        const int margin = (int) (0.05 * fs);   // 50 ms smoothing margin

        auto y = renderEffect (proc, 4.5, fs, block,
                               [T] (int t) { return t == T ? 1.0f : 0.0f; });

        const int winLo = T + D - G;
        const int winHi = T + D + 2 * G + margin;

        double eIn = 0.0, eOut = 0.0;
        for (int i = 0; i < (int) y.L.size(); ++i)
        {
            const double e = (double) y.L[(size_t) i] * y.L[(size_t) i]
                           + (double) y.R[(size_t) i] * y.R[(size_t) i];
            if (i >= winLo && i <= winHi) eIn += e;
            else                          eOut += e;
        }

        // -80 dB relative: energy ratio < 1e-8 (no echo at T+2D).
        const bool ok = eIn > 0.0
                     && eOut <= 1.0e-8 * eIn
                     && allFinite (y.L) && allFinite (y.R);

        check ("single-generation", ok,
               juce::String ("eIn=") + juce::String (eIn, 8)
                 + " eOut=" + juce::String (eOut, 12)
                 + " (win " + juce::String (winLo) + ".." + juce::String (winHi) + ")");
    }

    // --- Probe F: damping loss per generation (FUNC-03) ----------------------
    // Impulse through the closed loop with tight damping. Gen 1 is the raw
    // (undamped) bloom starting T+D; gen 2 is one loop pass later at T+2D and
    // has been through fbGain -> HP(200) -> LP(4000) -> tanh exactly once.
    {
        setBaseline (apvts);
        setParam (apvts, "density",  100.0f);
        setParam (apvts, "feedback",  60.0f);
        setParam (apvts, "lowCut",   200.0f);
        setParam (apvts, "highCut", 4000.0f);
        proc.prepareToPlay (fs, block);

        const int D = currentD();
        const int T = (int) fs;

        auto y = renderEffect (proc, 3.5, fs, block,
                               [T] (int t) { return t == T ? 1.0f : 0.0f; });

        // FFT windows (2^14 = 16384 ~ 341 ms) at the generation onsets. With
        // D = 24000 and gen-k support [T+kD, T+kD+2kG], the gen-1 window ends
        // before gen 2 starts and the gen-2 window ends before gen 3 starts.
        const auto s1 = analyze (y.L, T + D,     fs);
        const auto s2 = analyze (y.L, T + 2 * D, fs);

        const double e1 = s1.bandEnergy (20.0, 20000.0);
        const double e2 = s2.bandEnergy (20.0, 20000.0);
        const double c1 = s1.centroid   (20.0, 20000.0);
        const double c2 = s2.centroid   (20.0, 20000.0);
        const double lowFrac1 = e1 > 0.0 ? s1.bandEnergy (20.0, 150.0) / e1 : 0.0;
        const double lowFrac2 = e2 > 0.0 ? s2.bandEnergy (20.0, 150.0) / e2 : 0.0;

        // Two generations suffice; NO direction assertions past generation 1
        // (alternating-direction regenerations are intended by ARCHITECTURE).
        const bool ok = e1 > 1.0e-9
                     && e2 > 1.0e-12         // the loop actually regenerates
                     && e2 < e1              // loss per generation (fb 0.6 + damping)
                     && c2 < c1              // HF loss: centroid falls through LP(4000)
                     && lowFrac2 < lowFrac1  // LF loss: 20-150 Hz fraction falls through HP(200)
                     && allFinite (y.L) && allFinite (y.R);

        check ("damping-generations", ok,
               juce::String ("centroid=") + juce::String (c1, 1) + "->" + juce::String (c2, 1)
                 + " lowFrac=" + juce::String (lowFrac1, 5) + "->" + juce::String (lowFrac2, 5)
                 + " energy=" + juce::String (e1, 6) + "->" + juce::String (e2, 6));
    }

    // --- Probe G: 60 s stability at feedback=100 (DSP-03) --------------------
    // 2 s broadband excitation, then silence — the loop self-sustains through
    // default damping. tanh bounds the loop; output must stay below ceiling
    // with zero NaN/Inf for the full render.
    {
        setBaseline (apvts);
        setParam (apvts, "density",   60.0f);
        setParam (apvts, "feedback", 100.0f);
        setParam (apvts, "lowCut",   100.0f);   // default damping
        setParam (apvts, "highCut", 8000.0f);
        proc.prepareToPlay (fs, block);

        const double A = std::pow (10.0, -12.0 / 20.0);
        juce::Random rng ((juce::int64) 0x0feedbac);
        const int exciteLen = (int) (2.0 * fs);
        auto fill = [&] (int t)
        {
            return t < exciteLen ? (float) (A * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
        };

        auto y = renderEffect (proc, 60.0, fs, block, fill);

        const double peakAll = juce::jmax (peakAbs (y.L), peakAbs (y.R));

        double tailPeak = 0.0;
        for (int i = (int) (50.0 * fs); i < (int) y.L.size(); ++i)
            tailPeak = juce::jmax (tailPeak,
                                   (double) std::abs (y.L[(size_t) i]),
                                   (double) std::abs (y.R[(size_t) i]));

        // Loop carried energy well past the excitation (feedback not silently dead).
        const double washRms = rms (y.L, (int) (5.0 * fs), (int) (5.0 * fs));

        const bool ok = allFinite (y.L) && allFinite (y.R)
                     && peakAll < 1.0
                     && tailPeak < 1.0
                     && washRms > 1.0e-7;

        check ("stability-60s", ok,
               juce::String ("peak=") + juce::String (peakAll, 4)
                 + " tailPeak=" + juce::String (tailPeak, 6)
                 + " washRms[5..10s]=" + juce::String (washRms, 7));
    }

    // --- Probe H: cutoff sweeps during playback (QUAL-01 partial) ------------
    // 220 Hz sine with the loop engaged (feedback=60); lowCut then highCut
    // ramped full-range and back (log-mapped) while rendering. Legit-step
    // factor is larger than probe C's: feedback regeneration raises the
    // steady-state sine amplitude by the loop-convergence factor (< 2.5x at
    // fb=60) — a real click/zipper is still an order of magnitude above.
    {
        const double A  = std::pow (10.0, -12.0 / 20.0);
        const double f0 = 220.0;
        const double sineStep     = A * juce::MathConstants<double>::twoPi * f0 / fs;
        const double kStepFactorH = 2.5;
        const double thresh       = kStepFactorH * sineStep + std::pow (10.0, -60.0 / 20.0);

        auto sine = [=] (int t) noexcept
        {
            return (float) (A * std::sin (juce::MathConstants<double>::twoPi * f0 * (double) t / fs));
        };

        auto setupLoop = [&]
        {
            setBaseline (apvts);
            setParam (apvts, "mix",      35.0f);
            setParam (apvts, "feedback", 60.0f);
            setParam (apvts, "density",  60.0f);
            setParam (apvts, "width",    60.0f);
            proc.prepareToPlay (fs, block);
        };

        // Run 1: lowCut 20 -> 2000 -> 20 Hz (log-mapped triangle).
        setupLoop();
        auto sweepLow = [&] (int pos, int total)
        {
            const double t = (double) pos / (double) total;
            const double v = t < 0.5 ? 2.0 * t : 2.0 - 2.0 * t;      // 0..1..0
            setParam (apvts, "lowCut", (float) (20.0 * std::pow (2000.0 / 20.0, v)));
        };
        auto y1 = renderEffect (proc, 4.0, fs, block, sine, sweepLow);
        const double step1 = juce::jmax (maxAbsStep (y1.L), maxAbsStep (y1.R));

        check ("sweep-lowcut",
               step1 < thresh && allFinite (y1.L) && allFinite (y1.R),
               juce::String ("maxStep=") + juce::String (step1, 6)
                 + " thresh=" + juce::String (thresh, 6));

        // Run 2: highCut 20000 -> 500 -> 20000 Hz (log-mapped triangle).
        setupLoop();
        auto sweepHigh = [&] (int pos, int total)
        {
            const double t = (double) pos / (double) total;
            const double v = t < 0.5 ? 2.0 * t : 2.0 - 2.0 * t;
            setParam (apvts, "highCut", (float) (20000.0 * std::pow (500.0 / 20000.0, v)));
        };
        auto y2 = renderEffect (proc, 4.0, fs, block, sine, sweepHigh);
        const double step2 = juce::jmax (maxAbsStep (y2.L), maxAbsStep (y2.R));

        check ("sweep-highcut",
               step2 < thresh && allFinite (y2.L) && allFinite (y2.R),
               juce::String ("maxStep=") + juce::String (step2, 6)
                 + " thresh=" + juce::String (thresh, 6));
    }

    //==========================================================================
    // Phase 2.3 probes (I–M)
    //==========================================================================

    // Shared bloom-latency measurement for probes I/J. Impulse copies land at
    // output T+D+2n (n in [0,G)) weighted hann(n/G), but ONLY at grain-grid
    // positions (spawns every `interval` samples -> copies every 2*interval).
    // The raw first-onset therefore jitters by up to 2*interval (2400 smp at
    // G=200 ms / density=100) — larger than the ±1-block acceptance. The
    // ENERGY CENTROID of the single-generation bloom, however, sits at T+D+G
    // (hann^2 is symmetric about n=G/2 and the grid sum is phase-invariant to
    // within a few samples), so latency := centroid − T − G carries the tight
    // FUNC-02 assertion while the onset keeps a grid-bounded sanity window.
    struct BloomMeasure
    {
        double latency = -1.0;   // centroid-derived first-echo latency (samples)
        int    onset   = -1;     // first sample with |wet| > 1e-5
        double preRms  = 1.0;    // RMS before the expected bloom (decoy detector)
        bool   finite  = false;
    };

    auto measureBloom = [&] (ReverseDelayProcessor& p, double seconds, int T, int G, int Dexp)
    {
        auto y = renderEffect (p, seconds, fs, block,
                               [T] (int t) { return t == T ? 1.0f : 0.0f; });
        BloomMeasure m;
        m.finite = allFinite (y.L) && allFinite (y.R);
        const double c = energyCentroid (y.L, 0, (int) y.L.size());
        m.latency = c >= 0.0 ? c - (double) T - (double) G : -1.0;
        m.onset   = firstAbove (y.L, 1.0e-5);
        m.preRms  = rms (y.L, 0, T + Dexp - block);
        return m;
    };

    auto bloomOk = [&] (const BloomMeasure& m, int T, int Dexp, int interval)
    {
        return m.finite
            && std::abs (m.latency - (double) Dexp) <= (double) block
            && m.onset >= T + Dexp - block
            && m.onset <= T + Dexp + 2 * interval + block
            && m.preRms < 1.0e-6;
    };

    auto bloomDetail = [] (const BloomMeasure& m, int T, int Dexp)
    {
        return juce::String ("latency=") + juce::String (m.latency, 1)
             + " (expected " + juce::String (Dexp) + ")"
             + " onset-T=" + juce::String (m.onset - T)
             + " preRms=" + juce::String (m.preRms, 9);
    };

    // --- Probe I: tempo-sync spacing (FUNC-02) -------------------------------
    {
        // Sync case: 120 BPM, 1/4 -> D = 0.5 s. delayTime is set to a 150 ms
        // DECOY: a silent fallback-to-free bug lands the bloom at 150 ms and
        // fails both the latency assertion and the pre-bloom silence check.
        MockPlayHead mph;                        // bpm defaults to 120
        proc.setPlayHead (&mph);

        setBaseline (apvts);
        setParam (apvts, "syncMode",     1.0f);
        setParam (apvts, "noteDivision", 6.0f);  // 1/4
        setParam (apvts, "delayTime",  150.0f);  // decoy
        setParam (apvts, "density",    100.0f);  // overlap = 8 -> dense copy grid
        proc.prepareToPlay (fs, block);

        const int T        = (int) (0.5 * fs);
        const int G        = currentG();
        const int Dexp     = (int) (0.5 * fs);   // 1/4 @ 120 BPM
        const int interval = juce::jmax (1, (int) ((float) G / 8.0f));

        const auto m = measureBloom (proc, 2.0, T, G, Dexp);
        proc.setPlayHead (nullptr);

        check ("sync-quarter-120", bloomOk (m, T, Dexp, interval), bloomDetail (m, T, Dexp));

        // Free-mode variant: continuous control at 150 / 500 / 1200 ms.
        for (const float dt : { 150.0f, 500.0f, 1200.0f })
        {
            setBaseline (apvts);
            setParam (apvts, "delayTime", dt);
            setParam (apvts, "density", 100.0f);
            proc.prepareToPlay (fs, block);

            const int Gf  = currentG();
            const int Df  = currentD();
            const int ivf = juce::jmax (1, (int) ((float) Gf / 8.0f));

            const auto mf = measureBloom (proc, 1.6 + dt * 0.001, T, Gf, Df);

            check ((juce::String ("free-") + juce::String ((int) dt) + "ms").toRawUTF8(),
                   bloomOk (mf, T, Df, ivf), bloomDetail (mf, T, Df));
        }
    }

    // --- Probe J: no-playhead fallback (COMPAT-02) ---------------------------
    {
        // Main case: setPlayHead is NEVER called on this fresh instance. Sync
        // mode must fall back to the free delayTime (350 ms — distinct from
        // the 500 ms a 120 BPM 1/4 would produce), no crash, no silence.
        ReverseDelayProcessor procJ;
        procJ.setPlayConfigDetails (2, 2, fs, block);
        auto& aj = procJ.parameters;

        setBaseline (aj);
        setParam (aj, "syncMode",     1.0f);
        setParam (aj, "noteDivision", 6.0f);
        setParam (aj, "delayTime",  350.0f);
        setParam (aj, "density",    100.0f);
        procJ.prepareToPlay (fs, block);

        const int T   = (int) (0.5 * fs);
        const int Gj  = juce::jmax (2, (int) (paramValue (aj, "grainSize") * 0.001 * fs));
        const int Dj  = juce::jmax (1, (int) (paramValue (aj, "delayTime") * 0.001 * fs));
        const int ivj = juce::jmax (1, (int) ((float) Gj / 8.0f));

        const auto mj = measureBloom (procJ, 1.8, T, Gj, Dj);
        check ("no-playhead-fallback", bloomOk (mj, T, Dj, ivj), bloomDetail (mj, T, Dj));

        // Bonus case: a playhead IS installed but reports no tempo
        // (getBpm() == nullopt) -> same fallback path.
        MockPlayHead mphNoBpm;
        mphNoBpm.bpm = {};
        proc.setPlayHead (&mphNoBpm);

        setBaseline (apvts);
        setParam (apvts, "syncMode",     1.0f);
        setParam (apvts, "noteDivision", 6.0f);
        setParam (apvts, "delayTime",  350.0f);
        setParam (apvts, "density",    100.0f);
        proc.prepareToPlay (fs, block);

        const int Gb  = currentG();
        const int Db  = currentD();
        const int ivb = juce::jmax (1, (int) ((float) Gb / 8.0f));

        const auto mb = measureBloom (proc, 1.8, T, Gb, Db);
        proc.setPlayHead (nullptr);

        check ("null-bpm-fallback", bloomOk (mb, T, Db, ivb), bloomDetail (mb, T, Db));
    }

    // --- Probe K: width spread (FUNC-04) -------------------------------------
    // Wet-only (mix=100, feedback=0) deterministic noise, density=100 for
    // grain statistics (~160 grains inside the measured 2 s tail). The same
    // noise seed feeds every run here and probe L's mono pass.
    double refWidth0WetRms = -1.0;
    {
        const double A   = std::pow (10.0, -12.0 / 20.0);
        const int    off = (int) (1.5 * fs);
        const int    len = (int) (2.0 * fs);

        auto renderNoise = [&] (float width)
        {
            setBaseline (apvts);
            setParam (apvts, "density", 100.0f);
            setParam (apvts, "width",   width);
            proc.prepareToPlay (fs, block);

            juce::Random rng (0x0077aa11);
            return renderEffect (proc, 4.0, fs, block,
                                 [&] (int) { return (float) (A * (rng.nextDouble() * 2.0 - 1.0)); });
        };

        // width = 0: centered dual-mono.
        auto y0 = renderNoise (0.0f);
        const double l0 = rms (y0.L, off, len);
        const double r0 = rms (y0.R, off, len);
        const double w0 = 0.5 * (l0 + r0);
        const double c0 = corrRange (y0.L, y0.R, off, len);
        const double s0 = diffRms  (y0.L, y0.R, off, len);
        refWidth0WetRms = w0;

        check ("width-0-centered",
               w0 > 1.0e-4
                 && std::abs (l0 - r0) / juce::jmax (w0, 1.0e-12) < 0.01
                 && c0 > 0.99
                 && allFinite (y0.L) && allFinite (y0.R),
               juce::String ("rmsL=") + juce::String (l0, 5) + " rmsR=" + juce::String (r0, 5)
                 + " corr=" + juce::String (c0, 4) + " sideRms=" + juce::String (s0, 9));

        // width = 100: decorrelated spread; frame-level |L-R| energy must sit
        // well above the width-0 run (x10) AND be a real fraction of the wet.
        auto y1 = renderNoise (100.0f);
        const double l1 = rms (y1.L, off, len);
        const double r1 = rms (y1.R, off, len);
        const double w1 = 0.5 * (l1 + r1);
        const double c1 = corrRange (y1.L, y1.R, off, len);
        const double s1 = diffRms  (y1.L, y1.R, off, len);

        check ("width-100-spread",
               w1 > 1.0e-4
                 && c1 < 0.9
                 && s1 > 10.0 * (s0 + 1.0e-9)
                 && s1 > 0.1 * w1
                 && allFinite (y1.L) && allFinite (y1.R),
               juce::String ("corr=") + juce::String (c1, 4)
                 + " sideRms=" + juce::String (s1, 6)
                 + " (width0 side=" + juce::String (s0, 9) + ")"
                 + " wetRms=" + juce::String (w1, 5));
    }

    // --- Probe L: mono->stereo identity (D4 open item) -----------------------
    {
        const double A   = std::pow (10.0, -12.0 / 20.0);
        const int    off = (int) (1.5 * fs);
        const int    len = (int) (2.0 * fs);

        ReverseDelayProcessor procL;
        procL.setPlayConfigDetails (1, 2, fs, block);   // mono in -> stereo out
        auto& al = procL.parameters;

        // Wet-only run with identical settings + noise seed to probe K's
        // width-0 stereo run: the 0.5*(L+R) mono-sum must degrade to identity
        // (no ±6 dB surprise), and width=0 keeps output L == R exactly.
        setBaseline (al);
        setParam (al, "density", 100.0f);
        setParam (al, "width",     0.0f);
        procL.prepareToPlay (fs, block);

        juce::Random rng (0x0077aa11);
        auto y = renderEffect (procL, 4.0, fs, block,
                               [&] (int) { return (float) (A * (rng.nextDouble() * 2.0 - 1.0)); });

        const double wm      = 0.5 * (rms (y.L, off, len) + rms (y.R, off, len));
        const double maxDiff = maxAbsDiff (y.L, y.R);
        const double levelDb = (refWidth0WetRms > 0.0 && wm > 0.0)
                                 ? 20.0 * std::log10 (wm / refWidth0WetRms) : 99.0;

        check ("mono-in-identity",
               maxDiff < 1.0e-6 && allFinite (y.L) && allFinite (y.R),
               juce::String ("max|L-R|=") + juce::String (maxDiff, 9));

        check ("mono-in-wet-level",
               refWidth0WetRms > 0.0 && std::abs (levelDb) <= 0.5,
               juce::String ("delta=") + juce::String (levelDb, 4) + " dB"
                 + " (mono=" + juce::String (wm, 5)
                 + " stereo=" + juce::String (refWidth0WetRms, 5) + ")");

        // Dry-path duplication at default mix: mono dry must be duplicated to
        // the right channel too (out L == R with dry present).
        setBaseline (al);
        setParam (al, "density", 100.0f);
        setParam (al, "width",     0.0f);
        setParam (al, "mix",      35.0f);
        procL.prepareToPlay (fs, block);

        juce::Random rng2 (0x0077aa11);
        auto y2 = renderEffect (procL, 2.0, fs, block,
                                [&] (int) { return (float) (A * (rng2.nextDouble() * 2.0 - 1.0)); });
        const double maxDiff2 = maxAbsDiff (y2.L, y2.R);

        check ("mono-in-dry-dup",
               maxDiff2 < 1.0e-6 && allFinite (y2.L) && allFinite (y2.R),
               juce::String ("max|L-R|=") + juce::String (maxDiff2, 9));
    }

    // --- Probe M: all-parameter sweep (QUAL-01) ------------------------------
    // Each of the 10 params ramped full-range (triangle lo->hi->lo over the
    // 3 s render), one at a time, others at plugin defaults; probe-C click
    // detector + allFinite + peak bound on every render. A 120 BPM playhead
    // is installed so the sync-dependent params (syncMode/noteDivision) sweep
    // a LIVE D; the delayTime sweep forces Free mode so it is live too.
    //
    // Two detector tiers (D5 tunables — frozen once green):
    //  * kStepFactorSmooth (3.0x sine step): smoothed / pan-only params —
    //    lowCut, highCut, width, mix. Same regime as probes C (1.75x) and
    //    H (2.5x), with headroom for the mix->100 wet boost.
    //  * kStepFactorLoose (8.0x sine step): LATCHED content params
    //    (delayTime, grainSize, density, syncMode, noteDivision) whose sweeps
    //    legitimately re-seat grain read positions, plus feedback (fb->100
    //    raises the loop's steady-state amplitude and with it the legitimate
    //    per-sample step). 8x still sits ~4x below a genuine discontinuity of
    //    the -12 dBFS test signal (~35x sine step) — real clicks are caught.
    {
        MockPlayHead mph;   // 120 BPM
        proc.setPlayHead (&mph);

        const double A  = std::pow (10.0, -12.0 / 20.0);
        const double f0 = 220.0;
        const double sineStep          = A * juce::MathConstants<double>::twoPi * f0 / fs;
        const double kStepFactorSmooth = 3.0;
        const double kStepFactorLoose  = 8.0;
        const double margin            = std::pow (10.0, -60.0 / 20.0);

        auto sine = [=] (int t) noexcept
        {
            return (float) (A * std::sin (juce::MathConstants<double>::twoPi * f0 * (double) t / fs));
        };

        struct SweepSpec
        {
            const char* id;
            float lo, hi;
            bool  choice;      // round the swept value to an integer index
            bool  loose;       // latched-content tier (kStepFactorLoose)
            bool  forceFree;   // syncMode -> Free so the sweep is live
            float dtOverride;  // > 0: delayTime override (free D != sync D)
        };

        const SweepSpec specs[] = {
            { "delayTime",    50.0f,  4000.0f, false, true,  true,    0.0f },   // v1.0.1: max 2000 -> 4000
            { "syncMode",      0.0f,     1.0f, true,  true,  false, 250.0f },
            { "noteDivision",  0.0f,    12.0f, true,  true,  false,   0.0f },
            { "grainSize",    50.0f,  4000.0f, false, true,  false,   0.0f },   // v1.5.0: max 500 -> 4000
            { "density",       0.0f,   100.0f, false, true,  false,   0.0f },
            { "feedback",      0.0f,   100.0f, false, true,  false,   0.0f },
            { "lowCut",       20.0f,  2000.0f, false, false, false,   0.0f },
            { "highCut",     500.0f, 20000.0f, false, false, false,   0.0f },
            { "width",         0.0f,   100.0f, false, false, false,   0.0f },
            { "mix",           0.0f,   100.0f, false, false, false,   0.0f },
            // v1.2.0 (B1). Both are LATCHED CONTENT parameters — a sweep
            // re-shapes the envelope of every NEW grain while in-flight grains
            // finish on the window they were spawned with, which is the same
            // legitimate read-reseating grainSize/density already get the loose
            // tier for. A window that was NOT latched would show up here as a
            // click, because two window shapes disagree at every phase and
            // switching mid-grain steps the envelope.
            { "grainTilt",     0.0f,     1.0f, false, true,  false,   0.0f },
            { "grainShape",    0.0f,     4.0f, true,  true,  false,   0.0f },
        };

        for (const auto& sp : specs)
        {
            setDefaults (apvts);
            if (sp.forceFree)         setParam (apvts, "syncMode",  0.0f);
            if (sp.dtOverride > 0.0f) setParam (apvts, "delayTime", sp.dtOverride);
            setParam (apvts, sp.id, sp.lo);
            proc.prepareToPlay (fs, block);

            auto sweep = [&] (int pos, int total)
            {
                const double t   = (double) pos / (double) total;
                const double tri = t < 0.5 ? 2.0 * t : 2.0 - 2.0 * t;
                float v = sp.lo + (float) tri * (sp.hi - sp.lo);
                if (sp.choice) v = std::round (v);
                setParam (apvts, sp.id, v);
            };

            auto y = renderEffect (proc, 3.0, fs, block, sine, sweep);

            const double step   = juce::jmax (maxAbsStep (y.L), maxAbsStep (y.R));
            const double pk     = juce::jmax (peakAbs (y.L), peakAbs (y.R));
            const double thresh = (sp.loose ? kStepFactorLoose : kStepFactorSmooth) * sineStep + margin;

            check ((juce::String ("sweep-") + sp.id).toRawUTF8(),
                   step < thresh && pk < 1.0 && allFinite (y.L) && allFinite (y.R),
                   juce::String ("maxStep=") + juce::String (step, 6)
                     + " thresh=" + juce::String (thresh, 6)
                     + " peak=" + juce::String (pk, 4)
                     + (sp.loose ? " [loose]" : ""));
        }

        // Sync <-> Free switch mid-playback: free D (250 ms) != sync D
        // (500 ms), toggled every 0.5 s. Only next-spawn D may change —
        // in-flight grains finish on latched values (click-free mechanism).
        setDefaults (apvts);
        setParam (apvts, "delayTime", 250.0f);
        proc.prepareToPlay (fs, block);

        auto toggle = [&] (int pos, int)
        {
            const int half = (int) (0.5 * fs);
            setParam (apvts, "syncMode", ((pos / half) & 1) != 0 ? 0.0f : 1.0f);
        };

        auto y = renderEffect (proc, 4.0, fs, block, sine, toggle);
        proc.setPlayHead (nullptr);

        const double step   = juce::jmax (maxAbsStep (y.L), maxAbsStep (y.R));
        const double pk     = juce::jmax (peakAbs (y.L), peakAbs (y.R));
        const double thresh = kStepFactorLoose * sineStep + margin;

        check ("mode-switch",
               step < thresh && pk < 1.0 && allFinite (y.L) && allFinite (y.R),
               juce::String ("maxStep=") + juce::String (step, 6)
                 + " thresh=" + juce::String (thresh, 6)
                 + " peak=" + juce::String (pk, 4));
    }

    //==========================================================================
    // v1.0.1 probes (O–S) — the three shipped defects from the 2026-07-24 review
    //==========================================================================

    // --- Probe O: block-size invariance (A2) ---------------------------------
    // A grain spawned at block offset i latches readAbs = blockStart + i − D and
    // is rendered BEFORE this block's capture write, so every read is
    // already-written only while i < D. v1.0.0 ran the whole host block as one
    // pass, so at blockSize >= D the late grains read a full ring lap of stale
    // audio. D bottoms out at 2400 samples (delayTime 50 ms @ 48 kHz), which puts
    // every 4096-sample buffer squarely in the broken region — and the harness
    // had never varied block size, which is why this shipped.
    //
    // The fix bounds each engine pass to D samples, which makes the engine
    // block-size INVARIANT: identical spawn positions (the scheduler countdown is
    // continuous), identical latched state, identical per-sample arithmetic. So
    // the assertion is a direct equality between a 512-sample render and a
    // 4096-sample one. Pre-fix the two diverge grossly; post-fix they agree to
    // float noise.
    //
    // The excitation must be POSITION-deterministic, not a sequential RNG:
    // renderEffect fills whole blocks, so a sequential generator would be
    // consumed a different number of times at a different block size and the two
    // runs would not share an input signal at all.
    {
        const double A = std::pow (10.0, -12.0 / 20.0);

        auto noiseAt = [] (int t) noexcept
        {
            juce::uint32 x = (juce::uint32) t * 2654435761u + 0x9e3779b9u;
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            return (double) (x >> 8) * (1.0 / 16777216.0) * 2.0 - 1.0;
        };

        auto renderAtBlock = [&] (int blk)
        {
            setBaseline (apvts);
            setParam (apvts, "delayTime",  50.0f);   // D = 2400 < 4096
            setParam (apvts, "density",    60.0f);
            setParam (apvts, "feedback",   40.0f);   // loop engaged — the stale read feeds back
            setParam (apvts, "width",      60.0f);
            proc.setPlayConfigDetails (2, 2, fs, blk);
            proc.prepareToPlay (fs, blk);

            return renderEffect (proc, 2.0, fs, blk,
                                 [&] (int t) { return (float) (A * noiseAt (t)); });
        };

        auto ySmall = renderAtBlock (512);
        auto yLarge = renderAtBlock (4096);

        // Restore the harness-wide play config for every probe that follows.
        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);

        const double dL      = maxAbsDiff (ySmall.L, yLarge.L);
        const double dR      = maxAbsDiff (ySmall.R, yLarge.R);
        const double refRms  = rms (ySmall.L, (int) (1.0 * fs), (int) (0.9 * fs));
        const double bigRms  = rms (yLarge.L, (int) (1.0 * fs), (int) (0.9 * fs));

        check ("blocksize-invariance",
               dL < 1.0e-6 && dR < 1.0e-6
                 && refRms > 1.0e-4 && bigRms > 1.0e-4
                 && allFinite (yLarge.L) && allFinite (yLarge.R),
               juce::String ("max|512-4096|=") + juce::String (juce::jmax (dL, dR), 9)
                 + " rms512=" + juce::String (refRms, 6)
                 + " rms4096=" + juce::String (bigRms, 6));
    }

    // --- Probe P: delayTime range + state recall (A1) ------------------------
    // P1 is the actual A1 acceptance test: 1/1 at 60 BPM is 4 beats = 4000 ms.
    // v1.0.0 clamped that to 2000 ms, so the UI named 1/1 while the engine played
    // 1/2 — and the same collapse hit 1/2D below 90 BPM and 1/2 below 60 BPM,
    // i.e. across the whole tempo band this plugin targets.
    {
        MockPlayHead mph;
        mph.bpm = 60.0;
        proc.setPlayHead (&mph);

        setBaseline (apvts);
        setParam (apvts, "syncMode",      1.0f);
        setParam (apvts, "noteDivision", 12.0f);   // 1/1
        setParam (apvts, "delayTime",   150.0f);   // decoy, as probe I
        setParam (apvts, "density",     100.0f);
        proc.prepareToPlay (fs, block);

        const int T        = (int) (0.5 * fs);
        const int G        = currentG();
        const int Dexp     = (int) (4.0 * fs);      // 4 beats @ 60 BPM
        const int interval = juce::jmax (1, (int) ((float) G / 8.0f));

        const auto m = measureBloom (proc, 6.0, T, G, Dexp);
        proc.setPlayHead (nullptr);

        check ("sync-whole-60bpm", bloomOk (m, T, Dexp, interval), bloomDetail (m, T, Dexp));

        // P2: the widened range is reachable at all, and the skew centre is
        // unchanged (316 ms must still sit at the knob's midpoint).
        setParam (apvts, "delayTime", 4000.0f);
        const float atMax = paramValue (apvts, "delayTime");

        auto* dtParam = apvts.getParameter ("delayTime");
        const float midMs = dtParam->getNormalisableRange().convertFrom0to1 (0.5f);

        check ("delaytime-range",
               std::abs (atMax - 4000.0f) < 0.5f && std::abs (midMs - 316.0f) < 1.0f,
               juce::String ("max=") + juce::String (atMax, 2)
                 + " midpoint=" + juce::String (midMs, 2) + " ms (expected 316)");

        // P3: APVTS session state stores DENORMALISED values, so a v1.0.0 session
        // recalls its literal ms under the widened range with no migration. This
        // asserts that directly — and would catch a well-meant "migration" that
        // rescaled the session tree as if it held normalised fractions.
        setParam (apvts, "delayTime", 1400.0f);

        juce::MemoryBlock stateBlob;
        proc.getStateInformation (stateBlob);

        setParam (apvts, "delayTime", 200.0f);
        proc.setStateInformation (stateBlob.getData(), (int) stateBlob.getSize());

        const float recalled = paramValue (apvts, "delayTime");

        check ("state-recall-1400ms",
               std::abs (recalled - 1400.0f) < 0.5f,
               juce::String ("recalled=") + juce::String (recalled, 3) + " ms (saved 1400)");
    }

    // --- Probe Q: density floor removes the gated-pulse region (A3) ----------
    // At v1.0.0's overlap = 1 the hop equalled the grain length, so Hann grains
    // abutted and the wet output fell to ZERO at every boundary — a 100 %-depth
    // 5 Hz tremolo at grainSize 200 ms, sold to the user as "less dense".
    //
    // A constant input makes the modulation directly measurable: with feedback=0
    // and width=0 the wet signal IS the overlap-add envelope, no interference and
    // no noise. Hann is constant-overlap-add at hop G/2, so at the new floor
    // (overlap 2) the envelope is flat; at the old floor it is a full-depth
    // ripple. min/max over a steady window separates them by two orders.
    {
        setBaseline (apvts);
        setParam (apvts, "density", 0.0f);
        proc.prepareToPlay (fs, block);

        auto y = renderEffect (proc, 2.0, fs, block, [] (int) { return 0.25f; });

        const int lo = (int) (1.0 * fs);
        const int hi = (int) (1.8 * fs);

        double envLo = 1.0e30, envHi = 0.0;
        for (int i = lo; i < hi && i < (int) y.L.size(); ++i)
        {
            const double v = std::abs ((double) y.L[(size_t) i]);
            envLo = juce::jmin (envLo, v);
            envHi = juce::jmax (envHi, v);
        }

        const double ratio = envHi > 0.0 ? envLo / envHi : 0.0;

        check ("density-0-continuity",
               ratio > 0.9 && envHi > 1.0e-3 && allFinite (y.L) && allFinite (y.R),
               juce::String ("min/max=") + juce::String (ratio, 4)
                 + " (v1.0.0 ~0.00, COLA = 1.00)"
                 + " env=" + juce::String (envLo, 5) + ".." + juce::String (envHi, 5));
    }

    // --- Probe R: v1.0.0 user-preset migration (A1) --------------------------
    // Preset JSON stores NORMALISED fractions (unlike the APVTS session tree),
    // so widening delayTime's max silently re-points every saved fraction at a
    // different ms. 500 ms saved under the 2000 ms range reads back as ~1240 ms
    // under the 4000 ms one. migrateUserPresets() rewrites those files in place.
    //
    // The probe synthesises a genuine v1.0.0-format preset, clears the migration
    // sentinel, and constructs a FRESH processor so the constructor's migration
    // actually runs — then asserts recall in milliseconds.
    {
        auto& pm       = proc.getPresetManager();
        auto  userDir  = pm.getUserPresetsDirectory();
        auto  sentinel = pm.getPresetsDirectory().getChildFile (".user-migration-version");

        const juce::String probeName = "ZZ Harness Migration Probe";
        auto probeFile = userDir.getChildFile (probeName + ".json");

        // v1.0.0's delayTime range, reconstructed exactly.
        juce::NormalisableRange<float> legacyRange { 50.0f, 2000.0f, 0.01f };
        legacyRange.setSkewForCentre (316.0f);

        const float savedMs    = 1400.0f;
        const float legacyNorm = legacyRange.convertTo0to1 (savedMs);

        auto* params = new juce::DynamicObject();
        params->setProperty ("delayTime", legacyNorm);

        auto* root = new juce::DynamicObject();
        root->setProperty ("parameters", juce::var (params));
        root->setProperty ("version",    "1.0.0");
        root->setProperty ("plugin",     "O-ReverseDelay");

        userDir.createDirectory();
        probeFile.replaceWithText (juce::JSON::toString (juce::var (root), true));
        sentinel.deleteFile();

        // What an UNMIGRATED v1.0.0 preset would recall under the new range —
        // printed so the probe visibly has teeth rather than passing vacuously.
        const float unmigratedMs =
            apvts.getParameter ("delayTime")->getNormalisableRange().convertFrom0to1 (legacyNorm);

        ReverseDelayProcessor procR;            // ctor runs migrateUserPresets()
        procR.setPlayConfigDetails (2, 2, fs, block);
        const bool loaded = procR.getPresetManager().loadPreset (probeName);
        const float recalled = paramValue (procR.parameters, "delayTime");

        probeFile.deleteFile();

        check ("preset-migration",
               loaded && std::abs (recalled - savedMs) < 0.5f,
               juce::String ("loaded=") + (loaded ? "1" : "0")
                 + " recalled=" + juce::String (recalled, 2) + " ms (saved " + juce::String (savedMs, 0)
                 + ", unmigrated would give " + juce::String (unmigratedMs, 1) + ")");
    }

    // --- Probe S: feedback decay at 100 % (A3 duty-cycle re-measure) ---------
    // The A3 remap changes overlap at a given density, and overlap sets both the
    // spawn hop and grainGain = 1/sqrt(overlap) — i.e. the loop's duty cycle. The
    // review flagged this as needing a before/after measurement.
    //
    // Both numbers come from THIS binary, which is what makes the comparison
    // clean: density 53.3 reproduces v1.0.0's overlap at density 60 (1+0.6·7 =
    // 5.2 = 2+0.533·6), and density 60 is the v1.0.1 mapping's own 5.6. The delta
    // between the two printed decay rates IS the remap's effect on the loop.
    // (Every factory preset is re-authored to the overlap-matched density, so no
    // shipped preset moves.)
    {
        const double A = std::pow (10.0, -12.0 / 20.0);
        const int exciteLen = (int) (2.0 * fs);

        for (const float d : { 53.3f, 60.0f })
        {
            setBaseline (apvts);
            setParam (apvts, "density",     d);
            setParam (apvts, "feedback", 100.0f);
            setParam (apvts, "lowCut",    100.0f);   // default damping
            setParam (apvts, "highCut",  8000.0f);
            proc.prepareToPlay (fs, block);

            juce::Random rng ((juce::int64) 0x0feedbac);
            auto fill = [&] (int t)
            {
                return t < exciteLen ? (float) (A * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
            };

            auto y = renderEffect (proc, 30.0, fs, block, fill);

            const double w1 = rms (y.L, (int) ( 5.0 * fs), (int) (5.0 * fs));
            const double w2 = rms (y.L, (int) (20.0 * fs), (int) (5.0 * fs));
            const double decayDbPerSec = (w1 > 0.0 && w2 > 0.0)
                                           ? 20.0 * std::log10 (w2 / w1) / 15.0
                                           : 0.0;
            const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));

            check ((juce::String ("decay-fb100-d") + juce::String (d, 1)).toRawUTF8(),
                   allFinite (y.L) && allFinite (y.R) && pk < 1.0 && w1 > 1.0e-7,
                   juce::String ("overlap=") + juce::String (2.0f + d * 0.06f, 2)
                     + " rms[5-10s]=" + juce::String (w1, 7)
                     + " rms[20-25s]=" + juce::String (w2, 7)
                     + " decay=" + juce::String (decayDbPerSec, 3) + " dB/s"
                     + " peak=" + juce::String (pk, 4));
        }
    }

    //==========================================================================
    // v1.1.0 probes (T–Y) — grain randomisation (B3) + grain-pool refusal
    //==========================================================================

    // Position-deterministic broadband excitation shared by T/U/W/Y. A
    // sequential juce::Random would be consumed a different number of times at
    // a different block size, so two runs would not even share an input signal
    // (probe O's lesson) — and probes T and W2 both compare renders directly.
    const double kRandA = std::pow (10.0, -12.0 / 20.0);

    auto randNoiseAt = [] (int t) noexcept
    {
        juce::uint32 x = (juce::uint32) t * 2654435761u + 0x9e3779b9u;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        return (double) (x >> 8) * (1.0 / 16777216.0) * 2.0 - 1.0;
    };

    auto randNoiseFill = [&] (int t) { return (float) (kRandA * randNoiseAt (t)); };

    // ── v1.3.0: a WHITE position-deterministic excitation ────────────────────
    //
    // randNoiseAt above is one xorshift round over a multiplied counter, and that
    // is not enough mixing to be white. Measured autocorrelation reaches ±0.077
    // at lags in the 600–2400 sample range — which happens to be exactly where
    // the grain spawn interval lives at these grain sizes.
    //
    // That does not matter for the probes that use it. T, U, W and Y compare two
    // renders to each other, so any fixed colouration cancels; Z2 and Z3 compare
    // levels across window shapes at a FIXED interval, so the same lag structure
    // applies to every shape and cancels there too.
    //
    // It matters enormously for probe AA, which is the first probe to compare
    // levels while VARYING the spawn interval. Adjacent grains read the source
    // 2·interval apart, so with a coloured excitation each overlap setting sees a
    // different amount of correlation between the grains it is summing, and the
    // measured level moves for reasons that have nothing to do with the gain law.
    // Measured that way, the ladder spread was 2.5 dB and NON-MONOTONIC in
    // overlap: the +1.45 dB outlier at overlap 10 sat exactly on the generator's
    // +0.077 correlation peak at lag 960, which is that setting's interval. It
    // reads exactly like the partially-coherent summing the review predicted,
    // and it is the test signal.
    //
    // A murmur3 finaliser over the same counter fixes it: max|acf| over lags
    // 1..20000 drops from 0.077 to 0.0024, against an estimator noise floor of
    // 0.0007. Still a pure function of the sample INDEX — the property probe O
    // and W2 depend on — so it is block-size invariant like its predecessor.
    //
    // Added ALONGSIDE randNoiseAt rather than replacing it, deliberately: every
    // pre-v1.3.0 probe's reported numbers stay exactly what they were, so this
    // release's result lines remain diffable against v1.2.0's.
    auto whiteNoiseAt = [] (int t) noexcept
    {
        juce::uint32 x = (juce::uint32) t * 0x9E3779B1u + 0x85EBCA6Bu;
        x ^= x >> 16; x *= 0x85EBCA6Bu;
        x ^= x >> 13; x *= 0xC2B2AE35u;
        x ^= x >> 16;
        return (double) (x >> 8) * (1.0 / 16777216.0) * 2.0 - 1.0;
    };

    // All four v1.1 controls off. Every probe below starts from here, so a probe
    // that forgets to zero one of them cannot silently inherit it from the last.
    auto clearRandomisation = [&]
    {
        setParam (apvts, "jitter",       0.0f);
        setParam (apvts, "delayScatter", 0.0f);
        setParam (apvts, "sizeRandom",   0.0f);
        setParam (apvts, "gainRandom",   0.0f);
    };

    // --- Probe T: each randomisation is LIVE, and zero is exactly zero -------
    // Two failure modes, both of which ship green through build/auval/pluginval:
    //
    //   1. A randomisation that is wired to a parameter but never reaches the
    //      engine — the DSP analogue of an unregistered native function. The
    //      knob moves, the sound does not, and nothing reports it.
    //   2. A randomisation that draws from the shared xorshift even when it is
    //      OFF. That would advance the stream one step per spawn, shifting every
    //      subsequent pan value, and would change the shipped v1.0 sound for
    //      every existing session the moment v1.1 is installed. It is invisible
    //      to anything except a direct render comparison.
    //
    // (2) is asserted as bit-equality of two independent defaults renders, which
    // also proves the new per-instance seed is fixed for the instance and not
    // re-rolled per prepareToPlay — probe O's 512-vs-4096 equality depends on
    // exactly that and would otherwise fail for an unrelated-looking reason.
    {
        auto renderWith = [&] (const char* id, float value)
        {
            setBaseline (apvts);
            setParam (apvts, "density",  60.0f);
            setParam (apvts, "feedback", 40.0f);   // loop engaged
            setParam (apvts, "width",    60.0f);   // pan draws active — the stream under test
            clearRandomisation();
            if (id != nullptr)
                setParam (apvts, id, value);
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 2.0, fs, block, randNoiseFill);
        };

        auto base  = renderWith (nullptr, 0.0f);
        auto base2 = renderWith (nullptr, 0.0f);

        const double repeatDiff = juce::jmax (maxAbsDiff (base.L, base2.L),
                                              maxAbsDiff (base.R, base2.R));
        const double baseRef = juce::jmax (peakAbs (base.L), peakAbs (base.R));

        check ("random-zero-determinism",
               repeatDiff == 0.0 && baseRef > 1.0e-3,
               juce::String ("max|run1-run2|=") + juce::String (repeatDiff, 9)
                 + " peak=" + juce::String (baseRef, 5));

        struct RandSpec { const char* id; float on; };
        const RandSpec kRand[] = {
            { "jitter",       75.0f },
            { "delayScatter",  60.0f },   // ms
            { "sizeRandom",   75.0f },
            { "gainRandom",   75.0f },
        };

        for (const auto& r : kRand)
        {
            auto y = renderWith (r.id, r.on);
            const double d  = juce::jmax (maxAbsDiff (base.L, y.L), maxAbsDiff (base.R, y.R));
            const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));

            // 2 % of the reference peak: far above float noise, far below the
            // change any of these genuinely makes. A dead control reads 0.
            check ((juce::String ("random-live-") + r.id).toRawUTF8(),
                   d > 0.02 * baseRef && pk < 1.0 && allFinite (y.L) && allFinite (y.R),
                   juce::String ("max|base-on|=") + juce::String (d, 6)
                     + " (needs >" + juce::String (0.02 * baseRef, 6) + ")"
                     + " peak=" + juce::String (pk, 4));
        }
    }

    // --- Probe U: randomisation changes SPREAD, not LEVEL --------------------
    // Every one of these is symmetric about its nominal value on purpose:
    // jitter keeps the mean spawn interval, scatter keeps the mean delay,
    // sizeRandom keeps the mean grain length, and gainRandom is explicitly
    // power-normalised by 1/sqrt(1 + dev²/3). If any of that is wrong the knob
    // silently becomes a loudness control — which is how a "character" control
    // ends up being used as a mixer move and then blamed for the mix.
    //
    // Same ±1 dB budget probe D holds density to, measured the same way.
    {
        struct LevelSpec { const char* id; float maxVal; };
        const LevelSpec kLevels[] = {
            { "jitter",       100.0f },
            { "delayScatter", 500.0f },
            { "sizeRandom",   100.0f },
            { "gainRandom",   100.0f },
        };

        for (const auto& sp : kLevels)
        {
            double lo = 1.0e30, hi = 0.0;
            juce::String detail;

            for (const float frac : { 0.0f, 0.5f, 1.0f })
            {
                setBaseline (apvts);          // feedback 0, width 0, mix 100 — wet only
                clearRandomisation();
                setParam (apvts, sp.id, frac * sp.maxVal);
                proc.prepareToPlay (fs, block);

                auto y = renderEffect (proc, 3.0, fs, block, randNoiseFill);

                // Start after the first bloom has fully established (D + 2G at
                // the baseline is 900 ms) so the measurement window is steady.
                const double r = rms (y.L, (int) (1.2 * fs), (int) (1.6 * fs));
                lo = juce::jmin (lo, r);
                hi = juce::jmax (hi, r);
                detail += juce::String (r, 5) + " ";
            }

            const double spreadDb = (lo > 0.0) ? 20.0 * std::log10 (hi / lo) : 99.0;

            check ((juce::String ("level-flat-") + sp.id).toRawUTF8(),
                   spreadDb < 1.0 && lo > 1.0e-4,
                   juce::String ("rms{0,50,100%}=") + detail
                     + "spread=" + juce::String (spreadDb, 3) + " dB");
        }
    }

    // --- Probe V: jitter actually breaks the spawn grid ----------------------
    // The musical claim behind B3 #1 is that strictly periodic spawning combs
    // the wash. Probe Q already established the measurement: at density 0 the
    // overlap is exactly 2, Hann is constant-overlap-add at hop G/2, and with a
    // CONSTANT input and feedback 0 the wet signal IS the overlap-add envelope —
    // so a perfectly regular grid renders perfectly flat (min/max = 1.0000).
    //
    // That makes flatness a direct readout of grid regularity, with no spectrum
    // and no peak-picking: jitter must visibly destroy it. Asserting BOTH ends
    // is what keeps this honest — a jitter that did nothing would leave the
    // ratio at 1.0, and the probe would fail rather than pass quietly.
    {
        auto envRatio = [&] (float jitterPct)
        {
            setBaseline (apvts);
            setParam (apvts, "density", 0.0f);
            clearRandomisation();
            setParam (apvts, "jitter", jitterPct);
            proc.prepareToPlay (fs, block);

            auto y = renderEffect (proc, 2.0, fs, block, [] (int) { return 0.25f; });

            const int lo = (int) (1.0 * fs);
            const int hi = (int) (1.8 * fs);

            double eLo = 1.0e30, eHi = 0.0;
            for (int i = lo; i < hi && i < (int) y.L.size(); ++i)
            {
                const double v = std::abs ((double) y.L[(size_t) i]);
                eLo = juce::jmin (eLo, v);
                eHi = juce::jmax (eHi, v);
            }
            return eHi > 0.0 ? eLo / eHi : 0.0;
        };

        const double flat     = envRatio (0.0f);
        const double jittered = envRatio (100.0f);

        check ("jitter-breaks-grid",
               flat > 0.9 && jittered < 0.6,
               juce::String ("min/max: jitter0=") + juce::String (flat, 4)
                 + " (COLA = 1.00) jitter100=" + juce::String (jittered, 4)
                 + " (needs < 0.60)");
    }

    // --- Probe W: delayScatter ring safety + block-size invariance -----------
    // W1 is the ring's worst case, which is the whole reason this landed after
    // v1.0.1: a grain's LATCHED delay can now exceed the delayTime maximum by
    // the full scatter range, so the read span is gD + 2G = 4.5 + 1.0 = 5.5 s
    // against a 6.0 s ring. Under-size the ring and grains read a full lap of
    // stale capture — the A2 failure mode, arriving by a different route and
    // just as inaudible to auval.
    //
    // W2 is the one that catches a broken pass bound. The A2 fix bounds each
    // engine pass to D so that spawn offset i is always < the grain's delay;
    // NEGATIVE scatter shortens that delay, so the latched value is clamped to
    // passLen. Forget that clamp and a 4096-sample block at delayTime 50 ms
    // diverges from a 512-sample one exactly as v1.0.0 did — with all four
    // randomisations on, which is the configuration nobody renders twice.
    {
        // W1 — max delay, max grain, max scatter, dense, heavy feedback.
        // v1.5.0: "max grain" now means 4000 ms, which also makes this the
        // block-size-invariance probe that exercises the widest possible latched
        // read span (gD_max + 2·G_max) against the ring.
        setBaseline (apvts);
        setParam (apvts, "delayTime",   4000.0f);
        setParam (apvts, "grainSize",  ReverseDelayProcessor::kGrainSizeMaxMs);
        setParam (apvts, "density",      100.0f);
        setParam (apvts, "feedback",      80.0f);
        setParam (apvts, "width",        100.0f);
        clearRandomisation();
        setParam (apvts, "delayScatter", 500.0f);
        setParam (apvts, "sizeRandom",   100.0f);
        setParam (apvts, "jitter",       100.0f);
        setParam (apvts, "gainRandom",   100.0f);
        proc.prepareToPlay (fs, block);

        auto yRing = renderEffect (proc, 12.0, fs, block,
                                   [&] (int t) { return t < (int) (4.0 * fs) ? randNoiseFill (t) : 0.0f; });

        const double ringPk  = juce::jmax (peakAbs (yRing.L), peakAbs (yRing.R));
        const double ringRms = rms (yRing.L, (int) (6.0 * fs), (int) (4.0 * fs));

        check ("scatter-ring-worst-case",
               allFinite (yRing.L) && allFinite (yRing.R) && ringPk < 1.0 && ringRms > 1.0e-6,
               juce::String ("peak=") + juce::String (ringPk, 4)
                 + " washRms[6-10s]=" + juce::String (ringRms, 7)
                 + " (D+scatter=4.5s, G=0.5s, span=5.5s vs 6.0s ring)");

        // W2 — the A2 pass bound must survive negative scatter.
        auto renderRandomAtBlock = [&] (int blk)
        {
            setBaseline (apvts);
            setParam (apvts, "delayTime", 50.0f);    // D = 2400 < 4096
            setParam (apvts, "density",   60.0f);
            setParam (apvts, "feedback",  40.0f);
            setParam (apvts, "width",     60.0f);
            clearRandomisation();
            setParam (apvts, "jitter",       50.0f);
            setParam (apvts, "delayScatter", 40.0f);   // ±1920 samples against D = 2400
            setParam (apvts, "sizeRandom",   50.0f);
            setParam (apvts, "gainRandom",   50.0f);
            proc.setPlayConfigDetails (2, 2, fs, blk);
            proc.prepareToPlay (fs, blk);
            return renderEffect (proc, 2.0, fs, blk, randNoiseFill);
        };

        auto rSmall = renderRandomAtBlock (512);
        auto rLarge = renderRandomAtBlock (4096);

        proc.setPlayConfigDetails (2, 2, fs, block);   // restore for later probes
        proc.prepareToPlay (fs, block);

        const double rd   = juce::jmax (maxAbsDiff (rSmall.L, rLarge.L),
                                        maxAbsDiff (rSmall.R, rLarge.R));
        const double rRef = rms (rSmall.L, (int) (1.0 * fs), (int) (0.9 * fs));

        check ("scatter-blocksize-invariance",
               rd < 1.0e-6 && rRef > 1.0e-4
                 && allFinite (rLarge.L) && allFinite (rLarge.R),
               juce::String ("max|512-4096|=") + juce::String (rd, 9)
                 + " rms512=" + juce::String (rRef, 6));
    }

    // --- Probe X: gainRandom does NOT change the loop decay rate -------------
    // This is the single assertion behind "applied after the feedback tap".
    // The engine accumulates two wet buffers for this: step 5 taps the one
    // WITHOUT per-grain random gain, step 7 outputs the one with it. Tap the
    // wrong buffer and a random gain recirculates, compounding every
    // generation — at feedback 100 the decay rate itself becomes stochastic,
    // so "shimmer" silently becomes "how long the tail lasts".
    //
    // Measured exactly as probe S measures the A3 remap: same excitation, same
    // windows, same dB/s. Both numbers are printed, so the comparison is a
    // reported delta rather than a claim.
    {
        const int exciteLen = (int) (2.0 * fs);
        double decay[2] = { 0.0, 0.0 };
        double washRms[2] = { 0.0, 0.0 };
        bool   ok = true;
        int    slot = 0;

        for (const float gr : { 0.0f, 100.0f })
        {
            setBaseline (apvts);
            setParam (apvts, "density",     60.0f);
            setParam (apvts, "feedback",   100.0f);
            setParam (apvts, "lowCut",     100.0f);
            setParam (apvts, "highCut",   8000.0f);
            clearRandomisation();
            setParam (apvts, "gainRandom",     gr);
            proc.prepareToPlay (fs, block);

            juce::Random rng ((juce::int64) 0x0feedbac);
            auto fill = [&] (int t)
            {
                return t < exciteLen ? (float) (kRandA * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
            };

            auto y = renderEffect (proc, 30.0, fs, block, fill);

            const double w1 = rms (y.L, (int) ( 5.0 * fs), (int) (5.0 * fs));
            const double w2 = rms (y.L, (int) (20.0 * fs), (int) (5.0 * fs));

            decay[slot]   = (w1 > 0.0 && w2 > 0.0) ? 20.0 * std::log10 (w2 / w1) / 15.0 : 0.0;
            washRms[slot] = w1;
            ok = ok && allFinite (y.L) && allFinite (y.R)
                    && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0 && w1 > 1.0e-7;
            ++slot;
        }

        const double deltaDecay = std::abs (decay[1] - decay[0]);

        // 0.25 dB/s against a ~2.5 dB/s decay: a randomisation that leaked into
        // the loop moves this by whole dB/s, not fractions.
        check ("gainrandom-loop-neutral",
               ok && deltaDecay < 0.25,
               juce::String ("decay gr0=") + juce::String (decay[0], 3)
                 + " gr100=" + juce::String (decay[1], 3)
                 + " dB/s delta=" + juce::String (deltaDecay, 4)
                 + " rms[5-10s]=" + juce::String (washRms[0], 7)
                 + "/" + juce::String (washRms[1], 7));
    }

    // --- Probe Y: grain-pool pressure is click-free (refuse, never steal) ----
    // v1.0 stole the oldest slot on exhaustion, cutting a live Hann envelope to
    // zero in one sample. v1.1 refuses the spawn instead. The pool is 32 slots
    // against a max sustained overlap of 8, so exhaustion needs the transient
    // peaks the randomisations create — jitter can shorten an interval to 0.1x
    // nominal, and a grainSize sweep leaves long grains in flight while short
    // ones spawn underneath them.
    //
    // The observed peak concurrency is REPORTED rather than asserted, because
    // whether 32 is reached depends on the RNG sequence and a threshold here
    // would be a coin flip that fails on someone else's machine one day. What is
    // asserted is the property that matters at any concurrency: no clicks, no
    // NaN, bounded peak. Probe C's first-difference detector, loose tier —
    // grainSize is a latched-content parameter and legitimately re-seats reads.
    {
        const double sineHz   = 220.0;
        const double sineStep = 2.0 * juce::MathConstants<double>::pi * sineHz / fs;
        const double margin   = 0.004;
        const double thresh   = 6.0 * sineStep + margin;   // kStepFactorLoose

        setBaseline (apvts);
        setParam (apvts, "grainSize", ReverseDelayProcessor::kGrainSizeMaxMs);
        setParam (apvts, "density",   100.0f);   // overlap 8
        setParam (apvts, "feedback",   60.0f);
        setParam (apvts, "width",     100.0f);
        setParam (apvts, "mix",       100.0f);
        clearRandomisation();
        setParam (apvts, "jitter",       100.0f);
        setParam (apvts, "delayScatter", 250.0f);
        setParam (apvts, "sizeRandom",   100.0f);
        setParam (apvts, "gainRandom",   100.0f);
        proc.prepareToPlay (fs, block);

        int peakActive = 0;

        // Sweep grainSize max -> min -> max so long grains are always in flight
        // while short ones spawn beneath them — the review's stated steal trigger.
        auto pressure = [&] (int pos, int total)
        {
            peakActive = juce::jmax (peakActive, proc.getActiveGrainCount());
            const double t   = (double) pos / (double) total;
            const double tri = t < 0.5 ? 2.0 * t : 2.0 - 2.0 * t;
            setParam (apvts, "grainSize",
                      ReverseDelayProcessor::kGrainSizeMaxMs
                        - (float) tri * (ReverseDelayProcessor::kGrainSizeMaxMs
                                           - ReverseDelayProcessor::kGrainSizeMinMs));
        };

        auto y = renderEffect (proc, 6.0, fs, block,
                               [&] (int t) { return (float) (0.25 * std::sin (sineStep * t)); },
                               pressure);

        const double step = juce::jmax (maxAbsStep (y.L), maxAbsStep (y.R));
        const double pk   = juce::jmax (peakAbs (y.L), peakAbs (y.R));

        check ("pool-pressure-clickfree",
               step < thresh && pk < 1.0 && allFinite (y.L) && allFinite (y.R),
               juce::String ("maxStep=") + juce::String (step, 6)
                 + " thresh=" + juce::String (thresh, 6)
                 + " peak=" + juce::String (pk, 4)
                 + " peakGrains=" + juce::String (peakActive) + "/32");
    }

    //==========================================================================
    // v1.2.0 probes (Z1–Z5) — B1 grain window shape + tilt
    //==========================================================================

    // Shape order must match WindowLut::Shape and the grainShape StringArray.
    struct ShapeSpec { int index; const char* name; };
    const ShapeSpec kShapes[] = {
        { 0, "Hann"       },
        { 1, "Tukey"      },
        { 2, "Gaussian"   },
        { 3, "Triangular" },
        { 4, "Expo-Decay" },
    };

    auto clearWindow = [&]
    {
        setParam (apvts, "grainShape", 0.0f);   // Hann
        setParam (apvts, "grainTilt",  0.5f);   // symmetric
        setParam (apvts, "tukeyTaper", 0.5f);   // v1.4.0 — the shipped taper
    };

    // --- Probe Z1: the default window is the SHIPPED window, exactly ---------
    // The whole compatibility claim of this release rests on two mechanisms
    // being EXACT rather than approximately right, so both are asserted as
    // exact-equality rather than within a tolerance:
    //
    //   1. The tilt phase warp at t = 0.5 must be the BITWISE identity. It is
    //      built from min/max and two coefficients that are exactly 1.0f, and
    //      0.5 + (p - 0.5) round-trips exactly for p in [0.5, 1] (Sterbenz).
    //      A "near identity" here — say a lerp that lands one ulp off — would
    //      pass every level and decay probe below and still change the render
    //      of every existing session, which is the failure this release cannot
    //      have.
    //   2. Both power-normalisation constants must be exactly 1.0f at
    //      (Hann, 0.5), so folding them into grainGain is a no-op multiply.
    //
    // The cross-version half of this claim cannot live inside one binary: the
    // v1.1.0 harness was rebuilt from commit 8fa3646 and its 63 probe result
    // lines diffed against this build's — byte-for-byte identical. That is
    // recorded in the CHANGELOG; what runs here is the mechanism behind it.
    {
        const auto& luts = proc.getWindowLuts();

        // (a) warp identity at the default tilt, over a dense phase sweep.
        const auto tilt = WindowLut::makeTilt (
            ReverseDelayProcessor::tiltToPeakPos (0.5f));

        bool  warpExact = (tilt.t == 0.5f && tilt.a == 1.0f && tilt.b == 1.0f);
        int   warpFails = 0;
        float worstP = 0.0f;

        for (int i = 0; i <= 4096; ++i)
        {
            const float p = (float) i / 4096.0f;

            // juce::exactlyEqual, not `!=`: bitwise equality is the ASSERTION
            // here, not an accident of it, and a bare != on two variables trips
            // -Wfloat-equal (which juce_recommended_warning_flags enables). The
            // whole probe exists because "close enough" is not enough.
            if (! juce::exactlyEqual (tilt.warp (p), p))
                { ++warpFails; if (warpFails == 1) worstP = p; }
        }

        warpExact = warpExact && warpFails == 0;

        check ("window-warp-identity", warpExact,
               juce::String ("t=") + juce::String (tilt.t, 6)
                 + " a=" + juce::String (tilt.a, 6) + " b=" + juce::String (tilt.b, 6)
                 + " mismatches=" + juce::String (warpFails) + "/4097"
                 + (warpFails ? juce::String (" first@p=") + juce::String (worstP, 6)
                              : juce::String()));

        // (b) both norms exactly 1.0f at the shipped window, and the tilt norm
        //     exactly 1.0f at ANY tilt for every symmetric shape (the warp's
        //     power invariance — the reason tilt needs no compensation).
        const bool hannNormExact = (luts.getShapeNorm (0) == 1.0f)
                                && (luts.getTiltNorm (0, 0.5f) == 1.0f);

        int symmetricFails = 0;
        for (int s = 0; s <= 3; ++s)                      // Hann..Triangular are symmetric
            for (const float tv : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                if (luts.getTiltNorm (s, ReverseDelayProcessor::tiltToPeakPos (tv)) != 1.0f)
                    ++symmetricFails;

        check ("window-norm-identity", hannNormExact && symmetricFails == 0,
               juce::String ("hann shapeNorm=") + juce::String (luts.getShapeNorm (0), 9)
                 + " tiltNorm=" + juce::String (luts.getTiltNorm (0, 0.5f), 9)
                 + " symmetric-tilt non-unity: " + juce::String (symmetricFails) + "/20");

        // (c) the rendered form of the same claim: explicitly selecting the
        //     default window must produce the same samples as never touching it.
        auto renderWindow = [&] (bool touch, float shapeIdx, float tiltVal)
        {
            setBaseline (apvts);
            setParam (apvts, "density",  60.0f);
            setParam (apvts, "feedback", 40.0f);
            setParam (apvts, "width",    60.0f);
            clearRandomisation();
            clearWindow();
            if (touch)
            {
                setParam (apvts, "grainShape", shapeIdx);
                setParam (apvts, "grainTilt",  tiltVal);
            }
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 2.0, fs, block, randNoiseFill);
        };

        auto untouched = renderWindow (false, 0.0f, 0.5f);
        auto explicitly = renderWindow (true, 0.0f, 0.5f);

        const double wd = juce::jmax (maxAbsDiff (untouched.L, explicitly.L),
                                      maxAbsDiff (untouched.R, explicitly.R));
        const double wRef = juce::jmax (peakAbs (untouched.L), peakAbs (untouched.R));

        check ("window-default-identity", wd == 0.0 && wRef > 1.0e-3,
               juce::String ("max|default-explicit|=") + juce::String (wd, 9)
                 + " peak=" + juce::String (wRef, 5));

        // (d) report the power duty cycles the normalisation is built on, so
        //     the constants Z2/Z4 depend on are printed numbers, not a claim.
        juce::String duty;
        for (const auto& sh : kShapes)
            duty += juce::String (sh.name) + "=" + juce::String (luts.getMeanSquare (sh.index), 4)
                  + "/" + juce::String (luts.getMean (sh.index), 4)
                  + "/" + juce::String (luts.getShapeNorm (sh.index), 4)
                  + "/" + juce::String (luts.getLoopNorm (sh.index, 0.5f), 4) + " ";

        check ("window-duty-report", true,
               juce::String ("meanSq/mean/shapeNorm/loopNorm ") + duty);

        // (e) the loop trim must ALSO be exactly 1.0f at the shipped window, or
        //     the feedback tap's latched gains stop being bitwise the pan values
        //     and (c) above would already have failed — asserted separately so a
        //     failure names the constant rather than just the render.
        check ("window-loopnorm-identity",
               juce::exactlyEqual (luts.getLoopNorm (0, 0.5f), 1.0f),
               juce::String ("hann loopNorm=") + juce::String (luts.getLoopNorm (0, 0.5f), 9));
    }

    // --- Probe Z2: shape changes TIMBRE, not level ---------------------------
    // grainGain = 1/sqrt(overlap) assumed Hann's power duty cycle. Tukey's mean
    // square is 0.6875 against Hann's 0.375, so an uncompensated shape switch
    // would raise the wet level by 2.6 dB — the control would be read as a
    // volume knob and blamed for the mix, exactly as probe U guards the four
    // randomisations. Same ±1 dB budget, same measurement as probes D and U.
    {
        double lo = 1.0e30, hi = 0.0;
        juce::String detail;
        bool ok = true;

        for (const auto& sh : kShapes)
        {
            setBaseline (apvts);          // feedback 0, width 0, mix 100 — wet only
            clearRandomisation();
            clearWindow();
            setParam (apvts, "grainShape", (float) sh.index);
            proc.prepareToPlay (fs, block);

            auto y = renderEffect (proc, 3.0, fs, block, randNoiseFill);

            const double r = rms (y.L, (int) (1.2 * fs), (int) (1.6 * fs));
            lo = juce::jmin (lo, r);
            hi = juce::jmax (hi, r);
            detail += juce::String (sh.name) + "=" + juce::String (r, 5) + " ";
            ok = ok && allFinite (y.L) && allFinite (y.R)
                    && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0;
        }

        const double spreadDb = (lo > 0.0) ? 20.0 * std::log10 (hi / lo) : 99.0;

        check ("level-flat-shape", ok && spreadDb < 1.0 && lo > 1.0e-4,
               juce::String ("rms ") + detail
                 + "spread=" + juce::String (spreadDb, 3) + " dB");
    }

    // --- Probe Z3: tilt changes SHAPE, not level -----------------------------
    // Run for Hann AND for Expo-Decay, because the two exercise different code:
    // for a symmetric window the warp is power-preserving by construction and
    // getTiltNorm() returns exactly 1.0f (probe Z1b), so Hann tests the WARP;
    // Expo-Decay is asymmetric, its two half-powers differ, and it is the only
    // shape where the tilt normalisation actually computes something. Testing
    // only Hann would leave that arithmetic entirely unexercised.
    {
        for (const auto& sh : { kShapes[0], kShapes[4] })
        {
            double lo = 1.0e30, hi = 0.0;
            juce::String detail;
            bool ok = true;

            for (const float tv : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
            {
                setBaseline (apvts);
                clearRandomisation();
                clearWindow();
                setParam (apvts, "grainShape", (float) sh.index);
                setParam (apvts, "grainTilt",  tv);
                proc.prepareToPlay (fs, block);

                auto y = renderEffect (proc, 3.0, fs, block, randNoiseFill);

                const double r = rms (y.L, (int) (1.2 * fs), (int) (1.6 * fs));
                lo = juce::jmin (lo, r);
                hi = juce::jmax (hi, r);
                detail += juce::String (r, 5) + " ";
                ok = ok && allFinite (y.L) && allFinite (y.R)
                        && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0;
            }

            const double spreadDb = (lo > 0.0) ? 20.0 * std::log10 (hi / lo) : 99.0;

            check ((juce::String ("level-flat-tilt-") + sh.name).toRawUTF8(),
                   ok && spreadDb < 1.0 && lo > 1.0e-4,
                   juce::String ("rms{0,.25,.5,.75,1}=") + detail
                     + "spread=" + juce::String (spreadDb, 3) + " dB");
        }
    }

    // --- Probe Z4: shape does not change the FEEDBACK DECAY RATE -------------
    // The second half of the load-bearing warning. grainGain is applied BEFORE
    // the feedback tap, so the window's power duty cycle sets how much energy
    // survives each generation — the v1.0.0 CHANGELOG attributes the loop's
    // −4.3 dB/generation to the Hann² duty specifically. Normalise the level but
    // not the loop and "window shape" silently becomes "tail length"; a user
    // auditioning shapes at feedback 100 would hear the decay move and read it
    // as the shape being louder or quieter.
    //
    // Measured exactly as probes S and X measure theirs: same excitation, same
    // windows, same dB/s. Every shape's number is printed, so the comparison is
    // a reported table and not an assertion nobody can audit.
    // Two feedback settings, because they answer different questions and the
    // first run of this release needed both to tell them apart:
    //
    //   fb = 60  — the loop runs well below the tanh's knee, so this is the
    //              LINEAR per-generation gain. It is the number the loop
    //              normalisation is responsible for, and the one held tight.
    //   fb = 100 — the loop is into tanh compression, where a window's crest
    //              factor changes how hard it limits. No linear constant can
    //              equalise that, so this is held to a looser band and its whole
    //              table is printed.
    //
    // The two settings need DIFFERENT measurement windows, and getting that
    // wrong is its own trap: at fb 60 the loop loses ~8.8 dB/s, so probes S/X's
    // [5-10 s] vs [20-25 s] pair spans 15 s of decay — about 133 dB — and the
    // later window is reading the denormal floor rather than the tail. Measured
    // that way, four of the five shapes still agreed to 0.06 dB/s while
    // Expo-Decay read 1.5 dB/s off, which looks exactly like a normalisation
    // failure and is not one. The windows below are placed where each setting's
    // tail is genuinely alive.
    {
        const int exciteLen = (int) (2.0 * fs);

        for (const float fb : { 60.0f, 100.0f })
        {
            const bool  fast = fb < 80.0f;
            const double w1Start = fast ?  3.0 :  5.0;
            const double w2Start = fast ?  6.0 : 20.0;
            const double winLen  = fast ?  2.0 :  5.0;
            const double gapSec  = w2Start - w1Start;

            double hannDecay = 0.0;
            double worstDelta = 0.0;        // over all five shapes
            double worstSmooth = 0.0;       // excluding the peaky Expo-Decay
            const char* worstName = "-";
            juce::String detail;
            bool ok = true;

            for (const auto& sh : kShapes)
            {
                setBaseline (apvts);
                setParam (apvts, "density",     60.0f);
                setParam (apvts, "feedback",       fb);
                setParam (apvts, "lowCut",     100.0f);
                setParam (apvts, "highCut",   8000.0f);
                clearRandomisation();
                clearWindow();
                setParam (apvts, "grainShape", (float) sh.index);
                proc.prepareToPlay (fs, block);

                juce::Random rng ((juce::int64) 0x0feedbac);
                auto fill = [&] (int t)
                {
                    return t < exciteLen ? (float) (kRandA * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
                };

                auto y = renderEffect (proc, 30.0, fs, block, fill);

                const double w1 = rms (y.L, (int) (w1Start * fs), (int) (winLen * fs));
                const double w2 = rms (y.L, (int) (w2Start * fs), (int) (winLen * fs));
                const double decayDbPerSec = (w1 > 0.0 && w2 > 0.0)
                                               ? 20.0 * std::log10 (w2 / w1) / gapSec : 0.0;

                if (sh.index == 0)
                {
                    hannDecay = decayDbPerSec;
                }
                else
                {
                    const double delta = std::abs (decayDbPerSec - hannDecay);

                    if (delta > worstDelta) { worstDelta = delta; worstName = sh.name; }

                    // Expo-Decay is excluded from the tight bound, not from the
                    // probe: it is the only window with a crest factor far from
                    // the others (3.10 against 1.21-1.79), and crest factor is
                    // what a tanh responds to. Its number is still measured,
                    // still printed, and still bounded — just separately.
                    if (sh.index != 4 && delta > worstSmooth)
                        worstSmooth = delta;
                }

                detail += juce::String (sh.name) + "=" + juce::String (decayDbPerSec, 3) + " ";

                ok = ok && allFinite (y.L) && allFinite (y.R)
                        && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0 && w1 > 1.0e-7;
            }

            // ── bounds, set FROM MEASUREMENT ────────────────────────────────
            // The same probe was run against a build with getLoopNorm() forced
            // to 1.0f — i.e. against power-only normalisation, which is what
            // this release's first attempt shipped — to establish what each
            // bound has to catch. All numbers dB/s, worst delta vs Hann:
            //
            //                        fb 60        fb 100
            //   power-only, all      6.216        4.400      <- must FAIL
            //   power-only, smooth   1.318        1.330      <- must FAIL
            //   + loop trim, all     1.848        0.175
            //   + loop trim, smooth  0.042        0.042
            //
            // Hence two bounds rather than one loose one. The four low-crest
            // shapes are held to 0.35 — 8x tighter than they measure and 3.8x
            // below the power-only failure — so a regression in the loop
            // normalisation is caught even though it would leave the overall
            // number inside any bound wide enough for Expo-Decay.
            //
            // Expo-Decay's own bound is looser because its residual is NOT a
            // normalisation error and no linear constant removes it: its crest
            // factor is 3.10 against Hann's 1.63, so at equal loop energy its
            // peaks hit the loop's tanh far harder and it genuinely loses more
            // per generation. That is a real property of a peaky window in a
            // saturating loop, and it is largest at fb 60 — mid-knee, where a
            // limiter's incremental gain is most level-dependent — rather than
            // at fb 100, where everything is deep enough into limiting for the
            // differences to wash out.
            const double smoothBound = 0.35;
            const double allBound    = fast ? 2.50 : 1.20;

            check ((juce::String ("decay-shape-fb") + juce::String ((int) fb)).toRawUTF8(),
                   ok && worstSmooth < smoothBound && worstDelta < allBound,
                   juce::String ("dB/s ") + detail
                     + "worst-vs-Hann=" + juce::String (worstDelta, 3)
                     + " (" + worstName + ", <" + juce::String (allBound, 2) + ")"
                     + " low-crest worst=" + juce::String (worstSmooth, 3)
                     + " (<" + juce::String (smoothBound, 2) + ")"
                     + " win " + juce::String (w1Start, 0) + "s/" + juce::String (w2Start, 0) + "s");
        }
    }

    // --- Probe Z5: both window controls are LIVE -----------------------------
    // The mirror of probe T. Every guard above is a "must not change" — level
    // flat, decay flat, defaults bit-identical — and a control wired to nothing
    // at all would satisfy every one of them perfectly. This is the assertion
    // that fails when the window becomes a dead knob: the same failure class as
    // an unregistered native function, and just as invisible to auval.
    {
        auto renderWindowAt = [&] (float shapeIdx, float tiltVal)
        {
            setBaseline (apvts);
            setParam (apvts, "density",  60.0f);
            setParam (apvts, "feedback", 40.0f);
            setParam (apvts, "width",    60.0f);
            clearRandomisation();
            clearWindow();
            setParam (apvts, "grainShape", shapeIdx);
            setParam (apvts, "grainTilt",  tiltVal);
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 2.0, fs, block, randNoiseFill);
        };

        auto base = renderWindowAt (0.0f, 0.5f);
        const double baseRef = juce::jmax (peakAbs (base.L), peakAbs (base.R));

        // Each non-default shape, at the default tilt.
        for (const auto& sh : kShapes)
        {
            if (sh.index == 0) continue;

            auto y = renderWindowAt ((float) sh.index, 0.5f);
            const double d  = juce::jmax (maxAbsDiff (base.L, y.L), maxAbsDiff (base.R, y.R));
            const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));

            check ((juce::String ("window-live-") + sh.name).toRawUTF8(),
                   d > 0.02 * baseRef && pk < 1.0 && allFinite (y.L) && allFinite (y.R),
                   juce::String ("max|hann-shape|=") + juce::String (d, 6)
                     + " (needs >" + juce::String (0.02 * baseRef, 6) + ")"
                     + " peak=" + juce::String (pk, 4));
        }

        // Both tilt extremes, on Hann. Asserted separately from each other
        // because a warp that collapsed to the identity in one direction only
        // would still pass a single-ended check.
        for (const float tv : { 0.0f, 1.0f })
        {
            auto y = renderWindowAt (0.0f, tv);
            const double d  = juce::jmax (maxAbsDiff (base.L, y.L), maxAbsDiff (base.R, y.R));
            const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));

            check ((juce::String ("window-live-tilt") + juce::String ((int) (tv * 100.0f))).toRawUTF8(),
                   d > 0.02 * baseRef && pk < 1.0 && allFinite (y.L) && allFinite (y.R),
                   juce::String ("max|centre-tilt|=") + juce::String (d, 6)
                     + " (needs >" + juce::String (0.02 * baseRef, 6) + ")"
                     + " peak=" + juce::String (pk, 4));
        }
    }

    //==========================================================================
    // v1.3.0 probes (AA–AE) — B2 overlap ceiling / grain count
    //==========================================================================

    // Every probe below drives overlap by setting density to 100 and reading the
    // ceiling as the overlap, which is exact: at density 100 the map collapses to
    // `min + 1.0·(ceiling − min)` == ceiling. Asking for an arbitrary overlap
    // would mean inverting the map onto density's 0.1 % grid and landing near it
    // rather than on it, and probe AC's identity assertions are exact.
    // --- Probe AA: level is flat across the RAISED overlap range -------------
    // The acceptance test for the ceiling raise, and the probe that settled
    // whether grainGain needed a new term. It did not.
    //
    // The review predicted one: grainGain's 1/sqrt(overlap) assumes incoherent
    // summing, overlapping grains read the same reversed material at nearby
    // offsets, so the compensation should under-correct by a margin growing with
    // N — tolerable while N topped out at 8, but doubling the ceiling doubles the
    // range over which it accumulates. Sound reasoning; wrong conclusion. At a
    // fixed OUTPUT sample the grains in the sum read source points that are
    // multiples of 2·interval apart, which is decorrelated for broadband input,
    // so the output path sums incoherently and 1/sqrt(N) is exactly right.
    // Measured here at 0.07 dB across 2..16 with no correction term.
    //
    // Measured at density 100 so overlap == ceiling exactly, wet-only, no
    // feedback: the loop would fold a level error back on itself and confuse
    // "the gain law is wrong" with "the decay rate moved" (which is probe AF's
    // job). The whole ladder is PRINTED in dB relative to the legacy ceiling of
    // 8, because the useful output of this probe is the table, not the verdict —
    // a future release that re-tunes grainGain reads its effect straight off it.
    //
    // ⚠ USES whiteNoiseAt, NOT randNoiseFill, and that is load-bearing. This is
    // the first probe in the suite to compare LEVELS while varying the spawn
    // INTERVAL, which is what makes it sensitive to colouration in the test
    // signal — see whiteNoiseAt's note. With the shared generator this probe
    // reported a 2.5 dB non-monotonic spread that looked exactly like the
    // predicted coherence error and was entirely the excitation.
    {
        constexpr float kLadder[] = { 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f };
        constexpr int   kN        = (int) (sizeof (kLadder) / sizeof (kLadder[0]));
        constexpr int   kReps     = 3;            // independent noise realisations

        // A single 0.4 s window on one noise realisation is NOT a good enough
        // estimator here, and the first run of this probe proved it: it produced
        // a 2.5 dB spread that was not monotonic in overlap (10 read +1.43 dB
        // while 16 read −1.10 dB), which no gain law can explain and which would
        // have been "fitted" by an exponent chasing noise.
        //
        // The variance is structural rather than sloppy. At overlap N the output
        // is a sum of N windowed grains, and which parts of the noise land
        // constructively depends on where the spawn grid falls against that
        // particular realisation — so each ceiling gets a different draw from the
        // same distribution. The cure is more data, in both directions: a 2.4 s
        // window instead of 0.4, and three independent realisations averaged in
        // POWER (not in dB, which would bias the mean low). ~18x the samples,
        // ~4x less standard error.
        double r[kN] {};
        bool   ok = true;

        struct Diag { int interval = 0; int refused = 0; int peakActive = 0; };
        Diag diag[kN] {};

        for (int i = 0; i < kN; ++i)
        {
            double power = 0.0;

            for (int rep = 0; rep < kReps; ++rep)
            {
                setBaseline (apvts);             // feedback 0, width 0, mix 100 — wet only
                clearRandomisation();
                clearWindow();
                setParam (apvts, "grainCount", kLadder[i]);
                setParam (apvts, "density",    100.0f);   // overlap == ceiling
                proc.prepareToPlay (fs, block);

                // Decorrelated realisations from the same position-deterministic
                // generator: offsetting t re-phases the whole sequence, and the
                // offsets are far larger than any grain or delay length so no two
                // renders share source material.
                const int tOffset = rep * 500000;
                auto fill = [&] (int t) { return (float) (kRandA * whiteNoiseAt (t + tOffset)); };

                proc.resetSpawnCounters();
                int peakActive = 0;
                auto watch = [&] (int, int) { peakActive = juce::jmax (peakActive, proc.getActiveGrainCount()); };

                auto y = renderEffect (proc, 4.0, fs, block, fill, watch);

                const double rr = rms (y.L, (int) (1.2 * fs), (int) (2.4 * fs));
                power += rr * rr;

                diag[i].interval   = juce::jmax (1, (int) ((double) currentG() / (double) kLadder[i]));
                diag[i].refused    = juce::jmax (diag[i].refused, (int) proc.getRefusedSpawnCount());
                diag[i].peakActive = juce::jmax (diag[i].peakActive, peakActive);

                ok = ok && allFinite (y.L) && allFinite (y.R)
                        && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0;
            }

            r[i] = std::sqrt (power / (double) kReps);
        }

        // Reference is the legacy ceiling: what v1.2.0 could already reach is the
        // level the new territory has to match, not some new global average.
        const double ref = r[3];   // kLadder[3] == kLegacyOverlapMax
        juce::String ladder;

        // The spawn interval is printed alongside each rung because it is what the
        // level actually keys off, and because it is what made the harness bug
        // legible: the outlier rung's interval matched the excitation's
        // correlation peak exactly.
        for (int i = 0; i < kN; ++i)
            ladder << juce::String ((int) kLadder[i]) << "="
                   << juce::String (ref > 0.0 ? 20.0 * std::log10 (r[i] / ref) : -99.0, 2)
                   << "dB/iv" << juce::String (diag[i].interval) << " ";

        // Asserted over the FULL ladder, not just the new territory above 8. The
        // first draft held only 8..16 on the assumption that the shipped span
        // carried a drift that must not be "fixed" retroactively; the measurement
        // says there is no drift to preserve, so the whole range gets the same
        // ±1 dB budget probes D and Z2 use. That also makes this probe a guard on
        // the legacy span rather than only on the extension.
        //
        // Refusals must be zero for the measurement to mean anything: a refused
        // spawn lowers the ACTUAL overlap while grainGain still divides by the
        // nominal one, which reads as a level error that is really a pool error.
        double lo = 1.0e30, hi = 0.0;
        int    refusedTotal = 0;

        for (int i = 0; i < kN; ++i)
        {
            lo = juce::jmin (lo, r[i]);
            hi = juce::jmax (hi, r[i]);
            refusedTotal += diag[i].refused;
        }

        const double spreadDb = lo > 0.0 ? 20.0 * std::log10 (hi / lo) : 99.0;

        check ("level-flat-count", ok && spreadDb < 1.0 && lo > 1.0e-4 && refusedTotal == 0,
               juce::String ("rel-to-8: ") + ladder
                 + "| spread(2..16)=" + juce::String (spreadDb, 3) + " dB"
                 + " refused=" + juce::String (refusedTotal));
    }

    // --- Probe AB: the spawn cap is never reached ----------------------------
    // GrainScheduler's fixed request array silently discarded anything past 32
    // through v1.2.0. Unreachable at the shipped ranges — but "unreachable" was
    // an argument, not a measurement, and the ceiling raise moves the very
    // quantity the argument depended on.
    //
    // Worst case the parameters allow, derived rather than guessed:
    //   spawns/pass = passLen / interval, passLen <= kDelayTimeMinMs·fs,
    //   interval = G/overlap >= (kGrainSizeMinMs·fs)/overlapMax
    //   -> 16 · 50/50 = 16 nominal, against a cap of 128.
    // So: ceiling 16, grainSize AND delayTime both at their 50 ms minimum, and
    // jitter at 100 % to add the bursts that a nominal count cannot show.
    //
    // Run at TWO block sizes because passLen is derived from the host block
    // size: 512 gives one pass per block, 4096 gives several, and only the
    // second exercises a pass that fills from a mid-block starting countdown.
    {
        for (const int blk : { 512, 4096 })
        {
            setBaseline (apvts);
            clearRandomisation();
            clearWindow();
            setParam (apvts, "grainCount",   16.0f);
            setParam (apvts, "grainSize",    ReverseDelayProcessor::kGrainSizeMinMs);
            setParam (apvts, "delayTime",    ReverseDelayProcessor::kDelayTimeMinMs);
            setParam (apvts, "density",      100.0f);
            setParam (apvts, "jitter",       100.0f);
            setParam (apvts, "feedback",      60.0f);
            setParam (apvts, "width",        100.0f);
            setParam (apvts, "mix",          100.0f);

            proc.setPlayConfigDetails (2, 2, fs, blk);
            proc.prepareToPlay (fs, blk);
            proc.resetSpawnCounters();          // cumulative — clear or inherit

            int peakActive = 0;
            auto watch = [&] (int, int) { peakActive = juce::jmax (peakActive, proc.getActiveGrainCount()); };

            auto y = renderEffect (proc, 4.0, fs, blk, randNoiseFill, watch);

            const auto dropped = proc.getDroppedSpawnCount();
            const auto refused = proc.getRefusedSpawnCount();

            // Drops are asserted zero; REFUSALS are only reported. A refusal is
            // v1.1.0 working as designed (drop one grain rather than cut a live
            // envelope) and at ceiling 16 against 32 slots it is reachable under
            // full jitter — probe AD is what proves it stays inaudible.
            check ((juce::String ("spawncap-headroom-") + juce::String (blk)).toRawUTF8(),
                   dropped == 0 && allFinite (y.L) && allFinite (y.R)
                     && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0,
                   juce::String ("dropped=") + juce::String ((int) dropped)
                     + " (cap " + juce::String (GrainScheduler::kMaxSpawnsPerBlock) + ")"
                     + " refused=" + juce::String ((int) refused)
                     + " peakGrains=" + juce::String (peakActive) + "/32");
        }

        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);
    }

    // --- Probe AC: the default ceiling is v1.2.0, BITWISE --------------------
    // The guarantee that makes this a MINOR bump. Density is stored denormalised
    // in session state, so if the ceiling had been folded into the density knob's
    // own span every saved session would have got denser with no migration
    // available (critical_apvts_denormalised_vs_preset_normalised). Making it a
    // separate parameter avoids that only if its default reproduces the old map
    // EXACTLY — `min + d·(ceiling−min)` with ceiling 8 must be the same three
    // float operations as v1.0.1's `2 + d·6`, not merely the same value to
    // within an ulp.
    //
    // Asserted as a full-render bit comparison at several densities — including
    // 0 and 100, the two endpoints where the map's arithmetic degenerates — and
    // as exact float equality rather than a tolerance, because a difference of
    // one ulp is inaudible and would still mean the identity claim is false.
    //
    // Because grainGain ended up needing no new term (probe AA), this identity
    // in fact holds at EVERY ceiling and not only at the default: nothing in the
    // gain path changed at all this release. The probe still pins the default,
    // which is the property the compatibility claim rests on.
    {
        double worstDiff = 0.0;
        bool   ok = true;

        for (const float d : { 0.0f, 37.5f, 60.0f, 100.0f })
        {
            auto renderAtCeiling = [&] (bool setCeiling)
            {
                setBaseline (apvts);            // leaves grainCount at 8 …
                clearRandomisation();
                clearWindow();
                if (setCeiling)                 // … and this sets it to 8 again
                    setParam (apvts, "grainCount", ReverseDelayProcessor::kLegacyOverlapMax);
                setParam (apvts, "density",  d);
                setParam (apvts, "feedback", 40.0f);
                setParam (apvts, "width",    60.0f);
                proc.prepareToPlay (fs, block);
                return renderEffect (proc, 2.0, fs, block, randNoiseFill);
            };

            auto a = renderAtCeiling (false);
            auto b = renderAtCeiling (true);

            const double diff = juce::jmax (maxAbsDiff (a.L, b.L), maxAbsDiff (a.R, b.R));
            worstDiff = juce::jmax (worstDiff, diff);
            ok = ok && diff == 0.0 && rms (a.L, (int) (0.8 * fs), (int) (1.0 * fs)) > 1.0e-5;
        }

        check ("count-default-identity", ok,
               juce::String ("max|explicit8 - default|=") + juce::String (worstDiff, 12)
                 + " over density {0, 37.5, 60, 100}");
    }

    // --- Probe AD: the raised ceiling is still click-free --------------------
    // Probe Y at ceiling 16. v1.1.0 replaced steal-oldest with refuse-on-
    // exhaustion precisely because a stolen slot's envelope jumps from mid-window
    // to zero in one sample; the review flagged that raising the ceiling makes
    // exhaustion "much more likely", which is the condition that mechanism was
    // built for and has never actually been run at.
    //
    // Same detector, same loose tier, same grainSize sweep as probe Y — so a
    // regression here reads directly against that probe's numbers. Refusals are
    // reported: a non-zero count with a passing step detector is the affirmative
    // evidence that refuse-not-steal holds at 16, and is a stronger result than
    // zero refusals would be, because zero refusals would mean the mechanism was
    // never exercised.
    {
        const double sineHz   = 220.0;
        const double sineStep = 2.0 * juce::MathConstants<double>::pi * sineHz / fs;
        const double thresh   = 6.0 * sineStep + 0.004;   // kStepFactorLoose, as probe Y

        setBaseline (apvts);
        setParam (apvts, "grainCount", 16.0f);
        setParam (apvts, "grainSize",  ReverseDelayProcessor::kGrainSizeMaxMs);
        setParam (apvts, "density",    100.0f);   // overlap 16
        setParam (apvts, "feedback",    60.0f);
        setParam (apvts, "width",      100.0f);
        setParam (apvts, "mix",        100.0f);
        clearRandomisation();
        clearWindow();
        setParam (apvts, "jitter",       100.0f);
        setParam (apvts, "delayScatter", 250.0f);
        setParam (apvts, "sizeRandom",   100.0f);
        setParam (apvts, "gainRandom",   100.0f);
        proc.prepareToPlay (fs, block);
        proc.resetSpawnCounters();

        int peakActive = 0;

        auto pressure = [&] (int pos, int total)
        {
            peakActive = juce::jmax (peakActive, proc.getActiveGrainCount());
            const double t   = (double) pos / (double) total;
            const double tri = t < 0.5 ? 2.0 * t : 2.0 - 2.0 * t;
            setParam (apvts, "grainSize",
                      ReverseDelayProcessor::kGrainSizeMaxMs
                        - (float) tri * (ReverseDelayProcessor::kGrainSizeMaxMs
                                           - ReverseDelayProcessor::kGrainSizeMinMs));
        };

        auto y = renderEffect (proc, 6.0, fs, block,
                               [&] (int t) { return (float) (0.25 * std::sin (sineStep * t)); },
                               pressure);

        const double step = juce::jmax (maxAbsStep (y.L), maxAbsStep (y.R));
        const double pk   = juce::jmax (peakAbs (y.L), peakAbs (y.R));

        check ("ceiling16-pool-clickfree",
               step < thresh && pk < 1.0 && allFinite (y.L) && allFinite (y.R)
                 && proc.getDroppedSpawnCount() == 0,
               juce::String ("maxStep=") + juce::String (step, 6)
                 + " thresh=" + juce::String (thresh, 6)
                 + " peak=" + juce::String (pk, 4)
                 + " peakGrains=" + juce::String (peakActive) + "/32"
                 + " refused=" + juce::String ((int) proc.getRefusedSpawnCount())
                 + " dropped=" + juce::String ((int) proc.getDroppedSpawnCount()));
    }

    // --- Probe AE: the ceiling is a LIVE control, and the meter reports it ---
    // The mirror of probes T and Z5. Every assertion above is a "must not
    // change" — bit-identical at the default, level flat above it, no drops, no
    // clicks — and a grainCount wired to nothing whatsoever would satisfy all of
    // them perfectly. This is the probe that fails when it is a dead knob.
    //
    // The UI readout is checked in the same place because it has the same failure
    // mode and it is the reason B2 asked for the ceiling to be legible at all:
    // countActive() shipped in Stage 2 and was called by NOTHING until v1.1's
    // probe Y, which is precisely how a control ends up dead
    // (pattern_webview_native_fn_bridge_gap — an unwired readout passes build,
    // auval and pluginval identically to a wired one).
    {
        auto renderAtCount = [&] (float ceiling, float density)
        {
            setBaseline (apvts);
            clearRandomisation();
            clearWindow();
            setParam (apvts, "grainCount", ceiling);
            setParam (apvts, "density",    density);
            setParam (apvts, "feedback",   40.0f);
            setParam (apvts, "width",      60.0f);
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 2.0, fs, block, randNoiseFill);
        };

        auto base = renderAtCount (8.0f, 60.0f);
        const double baseRef = juce::jmax (peakAbs (base.L), peakAbs (base.R));

        for (const float c : { 2.0f, 12.0f, 16.0f })
        {
            auto y = renderAtCount (c, 60.0f);
            const double d = juce::jmax (maxAbsDiff (base.L, y.L), maxAbsDiff (base.R, y.R));

            check ((juce::String ("count-live-") + juce::String ((int) c)).toRawUTF8(),
                   d > 0.02 * baseRef && allFinite (y.L) && allFinite (y.R)
                     && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0,
                   juce::String ("max|ceil8 - ceil") + juce::String ((int) c) + "|="
                     + juce::String (d, 6) + " (needs >" + juce::String (0.02 * baseRef, 6) + ")");
        }

        // Meter: the published snapshot must be non-zero while a wash is running
        // and must track the ceiling. Asserted as an ORDERING (16 reports more
        // concurrent grains than 2) rather than against absolute counts, which
        // depend on where the block boundary lands relative to the spawn grid.
        renderAtCount (2.0f, 100.0f);
        const auto meterLo = proc.getGrainMeter();

        renderAtCount (16.0f, 100.0f);
        const auto meterHi = proc.getGrainMeter();

        check ("count-meter-live",
               meterLo.active > 0 && meterHi.active > meterLo.active
                 && std::abs (meterLo.overlap -  2.0f) < 0.01f
                 && std::abs (meterHi.overlap - 16.0f) < 0.01f,
               juce::String ("ceil2: active=") + juce::String (meterLo.active)
                 + " overlap=" + juce::String (meterLo.overlap, 3)
                 + " | ceil16: active=" + juce::String (meterHi.active)
                 + " overlap=" + juce::String (meterHi.overlap, 3));
    }

    // --- Probe AF: the ceiling does not move the FEEDBACK DECAY RATE ---------
    // Required by the review's own verification notes: "any change to grainGain,
    // window shape, or overlap RANGE alters the feedback loop's per-generation
    // loss — re-measure the decay rate at feedback = 100 before and after."
    //
    // And it is the one place the partial-coherence argument probe AA disproved
    // for the output path does genuinely apply. What recirculates is not
    // broadband input but this engine's own wash: self-similar material that
    // overlapping grains read at nearby offsets, summing closer to coherently.
    // v1.2.0 met that with a separate AMPLITUDE-normalised loop trim
    // (WindowLut::getLoopNorm) while the output keeps a POWER normalisation —
    // and that trim is a function of window shape and tilt, NOT of overlap, so
    // whether it still holds when the overlap range doubles is exactly the open
    // question. Nothing before this probe answers it.
    //
    // Measured the way probes S, X and Z4 measure theirs — same excitation, same
    // dB/s, and Z4's fb=100 window placement ([5-10 s] vs [20-25 s], where that
    // setting's tail is genuinely alive rather than sitting on the denormal
    // floor). Density is held at 100 so overlap == ceiling.
    //
    // Both feedback tiers, with Z4's window placement for each — fb 60 is the
    // loop's LINEAR per-generation gain, which is what loopCountTrim is directly
    // responsible for, and fb 100 is where it runs into the tanh. Both are held
    // to probe X's 0.25 dB/s: the trim's derived exponent leaves 0.02 dB/s of
    // residual across the whole ceiling range, so a tight bound costs nothing and
    // a loose one would let the defect back in unnoticed.
    {
        const int exciteLen = (int) (2.0 * fs);
        constexpr float kCeilings[] = { 8.0f, 10.0f, 12.0f, 14.0f, 16.0f };

        for (const float fb : { 60.0f, 100.0f })
        {
            const bool   fast    = fb < 80.0f;
            const double w1Start = fast ? 3.0 :  5.0;
            const double w2Start = fast ? 6.0 : 20.0;
            const double winLen  = fast ? 2.0 :  5.0;
            const double gapSec  = w2Start - w1Start;

            double legacyDecay = 0.0, worstDelta = 0.0, worstPeak = 0.0;
            int    worstCeil = 0;
            juce::String detail;
            bool   ok = true;

            for (const float c : kCeilings)
            {
                setBaseline (apvts);
                setParam (apvts, "grainCount", c);
                setParam (apvts, "density",   100.0f);   // overlap == ceiling
                setParam (apvts, "feedback",       fb);
                setParam (apvts, "lowCut",    100.0f);
                setParam (apvts, "highCut",  8000.0f);
                clearRandomisation();
                clearWindow();
                proc.prepareToPlay (fs, block);

                juce::Random rng ((juce::int64) 0x0feedbac);
                auto fill = [&] (int t)
                {
                    return t < exciteLen ? (float) (kRandA * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
                };

                auto y = renderEffect (proc, 30.0, fs, block, fill);

                const double w1 = rms (y.L, (int) (w1Start * fs), (int) (winLen * fs));
                const double w2 = rms (y.L, (int) (w2Start * fs), (int) (winLen * fs));
                const double decay = (w1 > 0.0 && w2 > 0.0)
                                       ? 20.0 * std::log10 (w2 / w1) / gapSec : 0.0;

                if (juce::exactlyEqual (c, ReverseDelayProcessor::kLegacyOverlapMax))
                {
                    legacyDecay = decay;
                }
                else if (std::abs (decay - legacyDecay) > worstDelta)
                {
                    worstDelta = std::abs (decay - legacyDecay);
                    worstCeil  = (int) c;
                }

                const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));
                worstPeak = juce::jmax (worstPeak, pk);
                detail << juce::String ((int) c) << "=" << juce::String (decay, 3) << " ";

                // Every decay must be NEGATIVE. This is the assertion that would
                // have caught the pre-trim defect on its own: at ceiling 10-16 the
                // rate went positive, i.e. the loop self-oscillated, and a
                // "difference from ceiling 8" bound alone can be satisfied by two
                // configurations that are both growing.
                ok = ok && allFinite (y.L) && allFinite (y.R) && pk < 1.0 && decay < 0.0;
            }

            check ((juce::String ("decay-count-fb") + juce::String ((int) fb)).toRawUTF8(),
                   ok && worstDelta < 0.25,
                   juce::String ("dB/s ") + detail
                     + "worst-vs-8=" + juce::String (worstDelta, 3)
                     + " (ceil " + juce::String (worstCeil) + ", <0.25)"
                     + " peak=" + juce::String (worstPeak, 4));
        }
    }

    // --- Probe AG: the worst case the ceiling allows stays bounded ------------
    // The probe that would have failed before loopCountTrim existed, and the one
    // worth keeping for that reason. Everything the ceiling raise can stack in one
    // configuration: overlap 16, feedback 100, 90 seconds — long enough that a
    // per-generation gain even slightly above unity has to show itself.
    //
    // Measured before the trim: RMS climbed 0.25 -> 1.07 and was still rising at
    // 85 s, with a peak of 1.28. That is a clipped output, not a loud one, and it
    // is the reason this probe asserts a MONOTONE DECAY across the whole
    // trajectory rather than only a final peak — a runaway that happened to
    // saturate below 1.0 would pass a peak check while still being a runaway.
    //
    // Probe M's stability-60s covers this shape at the DEFAULT ceiling; nothing
    // covered it at the raised one, which is exactly how the defect got in.
    {
        const int exciteLen = (int) (2.0 * fs);

        setBaseline (apvts);
        setParam (apvts, "grainCount", ReverseDelayProcessor::kOverlapCeilingMax);
        setParam (apvts, "density",   100.0f);
        setParam (apvts, "feedback",  100.0f);
        setParam (apvts, "mix",       100.0f);
        clearRandomisation();
        clearWindow();
        proc.prepareToPlay (fs, block);

        // width stays at setBaseline's 0, and that IS the worst case for the loop
        // — which is the opposite of the intuition that "everything at maximum"
        // is the hardest test. The first draft of this probe set width to 100 and
        // measured a tail that died in 20 s where probe AF's rate predicts 15 s
        // per 4 dB; the two disagreed by ~50 dB.
        //
        // The reason is pre-existing topology, not a v1.3.0 change: width scales
        // the per-grain pan gains, and those same gains feed the FEEDBACK TAP.
        // At width 0 every grain is centred, so loopL == loopR and the grains'
        // mono sum on read-back carries a factor of 0.7071. At width 100 the
        // alternating hard pan sends consecutive grains to opposite channels, so
        // the mono sum carries 0.5 instead — 3 dB less loop gain per generation,
        // compounding every pass. Width is therefore also a decay control in this
        // engine. Worth knowing; out of scope here.

        juce::Random rng ((juce::int64) 0x0feedbac);
        auto fill = [&] (int t)
        {
            return t < exciteLen ? (float) (kRandA * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
        };

        auto y = renderEffect (proc, 90.0, fs, block, fill);

        // Sampled every 10 s from 5 s (after the excitation stops and the wash
        // establishes) to 85 s. Each window must be quieter than the one before.
        juce::String traj;
        bool   monotone = true;
        double prev     = 1.0e30;

        for (int s = 5; s < 90; s += 10)
        {
            const double r = rms (y.L, (int) (s * fs), (int) (2.0 * fs));
            traj << juce::String (s) << "s=" << juce::String (r, 5) << " ";
            monotone = monotone && r < prev;
            prev = r;
        }

        const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));

        check ("ceiling16-loop-bounded",
               monotone && pk < 1.0 && allFinite (y.L) && allFinite (y.R),
               juce::String ("peak=") + juce::String (pk, 4)
                 + " monotone=" + (monotone ? "1" : "0") + " " + traj);
    }

    //==========================================================================
    // v1.4.0 probes (AH–AL) — continuous Tukey taper α
    //==========================================================================

    // --- Probe AH: the Hann remap really is the Tukey window -----------------
    // The claim v1.4.0's render rests on: Tukey's taper is a Hann half, so
    // Tukey(φ) = Hann(min(φ,1−φ)/taperEnd clamped, scaled) exactly. If that is
    // wrong the shape is wrong at every α and nothing else here would say so —
    // every level and decay probe below would happily normalise the wrong window.
    //
    // Measured against WindowLut's STORED Tukey table, which is built from
    // std::cos directly and is therefore an independent reference rather than the
    // remap compared to itself. Only the α = 0.5 table exists to compare against
    // (that is the one v1.2.0 shipped), so that is where the shape is verified;
    // the closed-form duty check in probe AI is what covers the other α.
    //
    // The bound is the LUT's own linear-interpolation error, not zero, and this
    // probe is where that number is on the record: reading a 2048-point table at
    // an arbitrary phase is not the same operation as evaluating cos there. 3e-6
    // is ~5x the predicted 5.9e-7 — tight enough to catch a real shape error,
    // loose enough not to fail on a compiler's fused multiply-add.
    {
        const auto& luts = proc.getWindowLuts();
        const int   n    = 4096;

        double worst = 0.0, worstPhi = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const float phi = static_cast<float> (i) / static_cast<float> (n - 1);

            const auto  taper   = WindowLut::makeTaper (WindowLut::tukey, 0.5f);
            const float remap   = luts.readShaped (luts.getTable (WindowLut::hann), taper, phi);
            const float stored  = luts.readAt     (luts.getTable (WindowLut::tukey), phi);
            const double d      = std::abs ((double) remap - (double) stored);

            if (d > worst) { worst = d; worstPhi = phi; }
        }

        check ("taper-remap-is-tukey", worst < 3.0e-6,
               juce::String ("max|hannRemap - storedTukey|=") + juce::String (worst, 10)
                 + " at phi=" + juce::String (worstPhi, 4)
                 + " (bound 3e-6; LUT lerp error, ~" + juce::String (20.0 * std::log10 (worst), 1)
                 + " dB)");

        // And at α = 1.0 the remap lands on the table's own points, so it must
        // reproduce HANN essentially exactly — a much stronger statement, and the
        // one that proves the phase scaling has no off-by-one.
        double worstHann = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const float phi   = static_cast<float> (i) / static_cast<float> (n - 1);
            const auto  taper = WindowLut::makeTaper (WindowLut::tukey, 1.0f);
            const float remap = luts.readShaped (luts.getTable (WindowLut::hann), taper, phi);
            const float hannV = luts.readAt     (luts.getTable (WindowLut::hann), phi);
            worstHann = juce::jmax (worstHann, std::abs ((double) remap - (double) hannV));
        }

        check ("taper-alpha1-is-hann", worstHann < 1.0e-6,
               juce::String ("max|tukey(a=1) - hann|=") + juce::String (worstHann, 12));
    }

    // --- Probe AI: the closed-form duty cycles are correct -------------------
    // WindowLut integrates its normalisation constants from the real window and
    // never from a closed form, because a hand-derived constant that drifts from
    // the window is the silent error the whole normalisation exists to prevent.
    // The closed forms are still worth having — they are how a reader checks the
    // grid is sane — so they are ASSERTED here rather than trusted anywhere:
    //
    //     meanSq(α) = 1 − 0.625·α        mean(α) = 1 − 0.5·α
    //
    // Derived from the taper being a Hann half: over a taper the mean is 0.5 and
    // the mean square 0.375, the flat part contributes 1, and the taper occupies
    // exactly α of the window.
    //
    // Tolerance 1.5e-3 absolute: the grid integrates a 2048-point SAMPLED window
    // whose endpoints are included, so it carries a small discrete-sum bias
    // against the continuous integral (~5e-4, and largest at small α). A tighter
    // bound would be asserting the sampling scheme, not the formula.
    {
        const auto& luts = proc.getWindowLuts();

        double worstMs = 0.0, worstM = 0.0;
        int    worstMsIdx = 0;
        juce::String table;

        for (int k = 0; k < WindowLut::kNumTaperSteps; ++k)
        {
            const float a  = WindowLut::taperAlphaAt (k);
            const double ms = luts.getMeanSquare (WindowLut::tukey, a);
            const double m  = luts.getMean       (WindowLut::tukey, a);

            const double dMs = std::abs (ms - (1.0 - 0.625 * a));
            const double dM  = std::abs (m  - (1.0 - 0.500 * a));

            if (dMs > worstMs) { worstMs = dMs; worstMsIdx = k; }
            worstM = juce::jmax (worstM, dM);

            // Print the ends and the shipped default — the whole 100-row grid
            // would drown the log, and these are the rows a reader checks.
            if (k == 0 || k == 49 || k == WindowLut::kNumTaperSteps - 1)
                table << "a=" << juce::String (a, 2)
                      << " ms=" << juce::String (ms, 5)
                      << " m="  << juce::String (m, 5) << " | ";
        }

        check ("taper-duty-closed-form", worstMs < 1.5e-3 && worstM < 1.5e-3,
               table + "worst dMeanSq=" + juce::String (worstMs, 6)
                 + " (a=" + juce::String (WindowLut::taperAlphaAt (worstMsIdx), 2) + ")"
                 + " dMean=" + juce::String (worstM, 6));

        // The default α must reproduce v1.3.0's constants BITWISE — that is the
        // whole reason the parameter's step was chosen to make 0.5 a grid point.
        // 0.6875 is the number the WindowLut header has documented since v1.2.0.
        check ("taper-default-grid-exact",
               juce::exactlyEqual (luts.getMeanSquare (WindowLut::tukey, 0.5f),
                                   luts.getMeanSquare (WindowLut::tukey))
                 && juce::exactlyEqual (luts.getShapeNorm (WindowLut::tukey, 0.5f),
                                        luts.getShapeNorm (WindowLut::tukey)),
               juce::String ("meanSq(0.5)=") + juce::String (luts.getMeanSquare (WindowLut::tukey, 0.5f), 9)
                 + " shapeNorm(0.5)=" + juce::String (luts.getShapeNorm (WindowLut::tukey, 0.5f), 9));

        // Tilt must stay power-invariant for Tukey at EVERY α, not just at the
        // default — Tukey is symmetric for all α, so this is exactly 1.0f, and
        // asserting it exactly is what stops it rotting into 1.0f ± 5e-8.
        bool tiltExact = true;
        for (int k = 0; k < WindowLut::kNumTaperSteps; ++k)
            for (const float tv : { 0.05f, 0.3f, 0.5f, 0.7f, 0.95f })
                tiltExact = tiltExact
                         && juce::exactlyEqual (luts.getTiltNorm (WindowLut::tukey, tv,
                                                                  WindowLut::taperAlphaAt (k)), 1.0f);

        check ("taper-tilt-power-invariant", tiltExact,
               juce::String ("getTiltNorm == 1.0f exactly over all ")
                 + juce::String (WindowLut::kNumTaperSteps) + " alpha x 5 tilts");
    }

    // --- Probe AJ: α changes TIMBRE, not level ------------------------------
    // The taper knob's acceptance test, and the mirror of probes D, U, Z2 and AA.
    // α moves Tukey's power duty from 0.994 to 0.375 — a 4.2 dB swing — so
    // without the α-aware shapeNorm this knob would be a volume control. Same
    // ±1 dB budget and the same measurement as every other level probe.
    //
    // Uses whiteNoiseAt for the reason probe AA does: α changes the window's
    // effective LENGTH, which shifts where overlapping grains read relative to
    // each other, so a coloured excitation would make this measure the test
    // signal again.
    {
        constexpr float kAlphas[] = { 0.01f, 0.1f, 0.25f, 0.5f, 0.75f, 1.0f };
        constexpr int   kNa       = (int) (sizeof (kAlphas) / sizeof (kAlphas[0]));

        double r[kNa] {};
        bool   ok = true;

        for (int i = 0; i < kNa; ++i)
        {
            setBaseline (apvts);              // feedback 0, width 0, mix 100 — wet only
            clearRandomisation();
            clearWindow();
            setParam (apvts, "grainShape", (float) WindowLut::tukey);
            setParam (apvts, "tukeyTaper", kAlphas[i]);
            proc.prepareToPlay (fs, block);

            auto y = renderEffect (proc, 4.0, fs, block,
                                   [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });

            r[i] = rms (y.L, (int) (1.2 * fs), (int) (2.4 * fs));
            ok = ok && allFinite (y.L) && allFinite (y.R)
                    && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0;
        }

        double lo = 1.0e30, hi = 0.0;
        juce::String detail;

        for (int i = 0; i < kNa; ++i)
        {
            lo = juce::jmin (lo, r[i]);
            hi = juce::jmax (hi, r[i]);
            detail << juce::String (kAlphas[i], 2) << "=" << juce::String (r[i], 5) << " ";
        }

        const double spreadDb = lo > 0.0 ? 20.0 * std::log10 (hi / lo) : 99.0;

        check ("level-flat-taper", ok && spreadDb < 1.0 && lo > 1.0e-4,
               juce::String ("rms ") + detail
                 + "spread=" + juce::String (spreadDb, 3) + " dB");
    }

    // --- Probe AK: α does not move the FEEDBACK DECAY RATE ------------------
    // The half that probe AJ cannot see, and the third time this engine has had
    // to answer it separately: α moves the AMPLITUDE duty 0.995 -> 0.500 (6.0 dB)
    // against the power duty's 4.2 dB, so an α-aware output norm alone leaves
    // ~1.8 dB per generation in the loop. Sold as "taper", heard as "tail
    // length" — exactly what v1.2.0 fixed for shape and v1.3.0 for overlap.
    //
    // Both feedback tiers, Z4's window placement, and every rate asserted
    // NEGATIVE as well as close to the default's — the lesson from v1.3.0, where
    // a difference-only bound would have passed two growing configurations.
    {
        const int exciteLen = (int) (2.0 * fs);
        constexpr float kAlphas[] = { 0.01f, 0.25f, 0.5f, 0.75f, 1.0f };

        for (const float fb : { 60.0f, 100.0f })
        {
            const bool   fast    = fb < 80.0f;
            const double w1Start = fast ? 3.0 :  5.0;
            const double w2Start = fast ? 6.0 : 20.0;
            const double winLen  = fast ? 2.0 :  5.0;
            const double gapSec  = w2Start - w1Start;

            // TWO PASSES, and that is a correctness fix rather than tidiness.
            // Probes Z4 and AF compare each measurement against the reference
            // configuration's as they go, which works only because their
            // reference (Hann / ceiling 8) happens to be FIRST in their sweep.
            // Here the reference α = 0.5 sits third, so a single pass compared
            // α = 0.01 and 0.25 against a still-zero reference and reported a
            // whole decay rate as the delta — a 9.7 dB/s "failure" that was the
            // probe, not the engine. Collect first, compare after.
            double decays[(int) (sizeof (kAlphas) / sizeof (kAlphas[0]))] {};
            double worstPeak = 0.0;
            juce::String detail;
            bool   ok = true;
            int    idx = 0;

            for (const float a : kAlphas)
            {
                setBaseline (apvts);
                clearRandomisation();
                clearWindow();
                setParam (apvts, "grainShape", (float) WindowLut::tukey);
                setParam (apvts, "tukeyTaper", a);
                setParam (apvts, "density",    60.0f);
                setParam (apvts, "feedback",      fb);
                proc.prepareToPlay (fs, block);

                juce::Random rng ((juce::int64) 0x0feedbac);
                auto fill = [&] (int t)
                {
                    return t < exciteLen ? (float) (kRandA * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
                };

                auto y = renderEffect (proc, 30.0, fs, block, fill);

                const double w1 = rms (y.L, (int) (w1Start * fs), (int) (winLen * fs));
                const double w2 = rms (y.L, (int) (w2Start * fs), (int) (winLen * fs));
                const double decay = (w1 > 0.0 && w2 > 0.0)
                                       ? 20.0 * std::log10 (w2 / w1) / gapSec : 0.0;

                decays[idx++] = decay;

                const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));
                worstPeak = juce::jmax (worstPeak, pk);
                detail << juce::String (a, 2) << "=" << juce::String (decay, 3) << " ";

                ok = ok && allFinite (y.L) && allFinite (y.R) && pk < 1.0 && decay < 0.0;
            }

            // Reference is the shipped α, found by value rather than by position.
            double refDecay = 0.0;
            for (int i = 0; i < idx; ++i)
                if (juce::exactlyEqual (kAlphas[i], WindowLut::kTukeyTaperDefault))
                    refDecay = decays[i];

            // Two tiers, exactly as probe Z4 splits Expo-Decay out from the four
            // smooth windows, and for the same physical reason.
            //
            // α = 0.01 is a very nearly RECTANGULAR window: crest factor ~1.0
            // against Hann's 1.63, and a flat-topped window overlapping at hop
            // G/overlap sums to something much closer to a constant. Both change
            // how the loop's tanh is driven, and neither is removable by a linear
            // amplitude-duty constant — the same statement the WindowLut header
            // already makes about Expo-Decay. It is measured, printed and bounded
            // separately rather than excused.
            double worstAll = 0.0, worstSmooth = 0.0;
            float  worstAlpha = 0.0f;

            for (int i = 0; i < idx; ++i)
            {
                const double d = std::abs (decays[i] - refDecay);

                if (d > worstAll) { worstAll = d; worstAlpha = kAlphas[i]; }

                if (kAlphas[i] >= 0.1f)
                    worstSmooth = juce::jmax (worstSmooth, d);
            }

            const double allBound = fast ? 0.40 : 1.10;

            check ((juce::String ("decay-taper-fb") + juce::String ((int) fb)).toRawUTF8(),
                   ok && worstAll < allBound && worstSmooth < 0.25,
                   juce::String ("dB/s ") + detail
                     + "worst-vs-0.5=" + juce::String (worstAll, 3)
                     + " (a=" + juce::String (worstAlpha, 2)
                     + ", <" + juce::String (allBound, 2) + ")"
                     + " a>=0.1 worst=" + juce::String (worstSmooth, 3) + " (<0.25)"
                     + " peak=" + juce::String (worstPeak, 4));
        }
    }

    // --- Probe AL: the taper is a LIVE control, inert off Tukey --------------
    // Two assertions that fail in opposite directions, and both matter:
    //   (a) on Tukey, moving α must change the render — else it is a dead knob,
    //       the failure class every "must not change" probe above would pass.
    //   (b) on any OTHER shape, moving α must change NOTHING, bit-for-bit. The
    //       taper is Tukey-only, and a leak would mean four shapes silently
    //       re-voiced by a control that does not apply to them.
    {
        auto renderTaper = [&] (int shape, float alpha)
        {
            setBaseline (apvts);
            clearRandomisation();
            clearWindow();
            setParam (apvts, "grainShape", (float) shape);
            setParam (apvts, "tukeyTaper", alpha);
            setParam (apvts, "feedback",   40.0f);
            setParam (apvts, "width",      60.0f);
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 2.0, fs, block, randNoiseFill);
        };

        auto base = renderTaper (WindowLut::tukey, 0.5f);
        const double baseRef = juce::jmax (peakAbs (base.L), peakAbs (base.R));

        for (const float a : { 0.01f, 0.25f, 1.0f })
        {
            auto y = renderTaper (WindowLut::tukey, a);
            const double d = juce::jmax (maxAbsDiff (base.L, y.L), maxAbsDiff (base.R, y.R));

            check ((juce::String ("taper-live-") + juce::String ((int) (a * 100.0f))).toRawUTF8(),
                   d > 0.02 * baseRef && allFinite (y.L) && allFinite (y.R)
                     && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0,
                   juce::String ("max|a0.5 - a") + juce::String (a, 2) + "|="
                     + juce::String (d, 6) + " (needs >" + juce::String (0.02 * baseRef, 6) + ")");
        }

        // (b) — every non-Tukey shape, α at both extremes, asserted BIT-identical.
        double worstLeak = 0.0;
        juce::String leakDetail;

        for (const int sh : { WindowLut::hann, WindowLut::gaussian,
                              WindowLut::triangular, WindowLut::expoDecay })
        {
            auto ref = renderTaper (sh, 0.5f);

            for (const float a : { 0.01f, 1.0f })
            {
                auto y = renderTaper (sh, a);
                const double d = juce::jmax (maxAbsDiff (ref.L, y.L), maxAbsDiff (ref.R, y.R));
                worstLeak = juce::jmax (worstLeak, d);
            }

            leakDetail << juce::String (sh) << ":" << juce::String (worstLeak, 3) << " ";
        }

        check ("taper-inert-off-tukey", worstLeak == 0.0,
               juce::String ("max|alpha leak| over shapes 0,2,3,4 = ")
                 + juce::String (worstLeak, 12));

        // ── What α = 0.01 actually does to the grain edges ──────────────────
        // The range's low end is a near-rectangular window, and this engine has
        // form here: the WindowLut header records that the Gaussian's 2 % end
        // pedestal was REMOVED because a step at every grain boundary, overlapping
        // several deep inside a re-reversing feedback loop, was audible. α = 0.01
        // is a deliberate 0->1 transition across 0.5 % of the grain, which is the
        // same kind of edge by choice rather than by accident.
        //
        // So it is measured rather than assumed, at the WORST case for it: the
        // 50 ms grain minimum, where 0.5 % of the grain is ~12 samples at 48 kHz
        // (0.25 ms). Probe C's first-difference detector, loose tier — the same
        // instrument, so the number is comparable to every other click probe.
        //
        // Reported at both extremes, and bounded only loosely: a fast edge is the
        // POINT of the low end, and a probe that demanded α = 0.01 be as smooth as
        // α = 0.5 would be asserting the feature away. What must hold is that it
        // stays a fast edge and does not become a discontinuity or a NaN.
        {
            const double sineHz   = 220.0;
            const double sineStep = 2.0 * juce::MathConstants<double>::pi * sineHz / fs;

            juce::String edges;
            bool ok = true;

            for (const float a : { 0.01f, 0.5f, 1.0f })
            {
                setBaseline (apvts);
                clearRandomisation();
                clearWindow();
                setParam (apvts, "grainShape", (float) WindowLut::tukey);
                setParam (apvts, "tukeyTaper", a);
                setParam (apvts, "grainSize",  ReverseDelayProcessor::kGrainSizeMinMs);
                setParam (apvts, "density",    60.0f);
                setParam (apvts, "mix",       100.0f);
                proc.prepareToPlay (fs, block);

                auto y = renderEffect (proc, 3.0, fs, block,
                                       [&] (int t) { return (float) (0.25 * std::sin (sineStep * t)); });

                const double step = juce::jmax (maxAbsStep (y.L), maxAbsStep (y.R));
                const double pk   = juce::jmax (peakAbs (y.L), peakAbs (y.R));

                edges << "a=" << juce::String (a, 2) << " step=" << juce::String (step, 5) << " ";
                ok = ok && allFinite (y.L) && allFinite (y.R) && pk < 1.0 && step < 0.25;
            }

            check ("taper-edge-report", ok,
                   edges + "(bounded <0.25; a=0.01 is INTENTIONALLY a fast edge "
                           "— 0.5 % of a 50 ms grain, ~0.25 ms)");
        }
    }

    // --- Probe N: factory-preset audit (Stage 4, D16 / C1) -------------------
    // MUST run last: it mutates the APVTS through the real preset manager and
    // leaves the plugin on the final preset's values, so no probe may follow it.
    //
    // The audit deliberately goes through the SHIPPING OuariconPresetManager
    // ::loadPreset() rather than a re-typed normalised table. That is the only
    // thing that actually proves the round trip
    //     engineering units -> convertTo0to1 -> JSON -> convertFrom0to1
    // survives the four skewed params. A table authored as raw normalised
    // fractions recalls 10-30x wrong on those and is otherwise silent — it
    // passes build, auval and pluginval and only ever sounds "like the wrong
    // preset" (pattern_factory_preset_normalized_ignores_skew).
    //
    // NOTE the dev-loop trap: initializeFactoryPresets() early-returns while
    // Factory/.factory-version already holds JucePlugin_VersionString. At a
    // frozen 1.0.0 every edit to the processor's factory table after the first
    // run is a SILENT no-op. After changing that table:
    //     rm -rf ~/Library/O-ReverseDelay/Presets/Factory
    // and re-run this harness (constructing the processor re-seeds the dir).
    {
        struct FactoryExpect
        {
            const char* name;
            float  sync, div, delay, grain, dens, fb, lo, hi, width, mix;
            // v1.2.0 (B1): every preset must recall the SHIPPED window. tilt is
            // 0.5 and not 0 — the one column here whose neutral value is not
            // zero, and the one a "new key, write 0" reflex would silently
            // hard-tilt all eight presets with.
            float  tilt, shape;
            double seconds;
        };

        // Mirrors the table in ReverseDelayProcessor's constructor, in
        // ENGINEERING units. Near-Infinite renders 30 s rather than 10: at
        // feedback=100 it is the preset-driven DSP-03 stability statement.
        //
        // v1.0.1: the density column is re-authored to (7·d_old − 100)/6 so each
        // preset holds the overlap it shipped with under the A3 remap. If these
        // ever fail, the FIRST thing to check is that the version bump actually
        // re-seeded ~/Library/O-ReverseDelay/Presets/Factory (see the note below).
        const FactoryExpect kFactoryExpect[] = {
            { "Reverse Bloom",    0, 6,  500, 200, 53.3f,  40, 100,  8000, 60, 40, 0.5f, 0, 10.0 },
            { "Guitar Swell",     0, 6,  700, 300, 47.5f,  45, 120,  6500, 55, 55, 0.5f, 0, 10.0 },
            { "Vocal Halo",       0, 6,  380, 180, 65.0f,  30, 300,  7000, 70, 25, 0.5f, 0, 10.0 },
            { "Slow Wash",        0, 6, 1400, 450, 18.3f,  65,  80,  5000, 85, 50, 0.5f, 0, 10.0 },
            { "Tight Smear",      0, 6,  180,  70, 88.3f,  35, 150, 11000, 35, 45, 0.5f, 0, 10.0 },
            { "Dark Cavern",      0, 6,  850, 320, 59.2f,  70, 220,  1800, 75, 55, 0.5f, 0, 10.0 },
            { "Near-Infinite",    0, 6,  900, 350, 65.0f, 100, 180,  2500, 80, 50, 0.5f, 0, 30.0 },
            { "Rhythmic Reverse", 1, 4,  500, 120, 76.7f,  50, 140,  9000, 50, 45, 0.5f, 0, 10.0 },
        };

        // Per-param tolerances, set FROM MEASUREMENT rather than assumed: the
        // first run of this probe printed worst=0.0000 for all ten params of all
        // eight presets, i.e. the engineering-unit round trip is bit-exact once
        // the 0.01 step snapping is applied. These values are therefore ~4 orders
        // of magnitude above the observed error and ~4 orders BELOW the error a
        // genuine skew bug produces (10-30x, i.e. thousands of Hz on highCut).
        // The "worst= ... % of tol" field in each line reports the headroom.
        const float kTolMs     = 0.5f;    // delayTime, grainSize
        const float kTolPct    = 0.1f;    // density, feedback, width, mix (linear)
        const float kTolLoHz   = 0.5f;    // lowCut   (20-2000, skew centre 200)
        const float kTolHiHz   = 0.5f;    // highCut  (500-20000, skew centre 3162)
        const float kTolChoice = 0.01f;   // syncMode, noteDivision (exact index)

        // 120 BPM playhead for the whole probe. Inert for the seven Free presets;
        // Rhythmic Reverse (syncMode=Sync, 1/8D) genuinely exercises tempo sync
        // rather than the COMPAT-02 no-BPM fallback — asserting the case we chose.
        MockPlayHead mph;
        proc.setPlayHead (&mph);

        const double A = std::pow (10.0, -12.0 / 20.0);
        const int exciteLen = (int) (2.0 * fs);

        for (const auto& e : kFactoryExpect)
        {
            const bool loaded = proc.getPresetManager().loadPreset (e.name);
            proc.prepareToPlay (fs, block);

            // (a) skew round-trip — the C1 assertion.
            // Every param is compared unconditionally (no && short-circuit) so
            // the printed "worst" always reflects all twelve, not just those
            // before the first failure. "worst" is the param with the largest delta
            // RELATIVE to its own tolerance, i.e. the one closest to failing.
            float       worst      = 0.0f;   // raw delta of that param
            float       worstRatio = 0.0f;   // delta / tolerance
            const char* worstId    = "-";
            bool        inRange    = true;

            auto cmp = [&] (const char* id, float expected, float tol)
            {
                const float d     = std::abs (paramValue (apvts, id) - expected);
                const float ratio = d / tol;
                if (ratio > worstRatio) { worstRatio = ratio; worst = d; worstId = id; }
                if (d > tol) inRange = false;
            };

            cmp ("syncMode",     e.sync,  kTolChoice);
            cmp ("noteDivision", e.div,   kTolChoice);
            cmp ("delayTime",    e.delay, kTolMs);
            cmp ("grainSize",    e.grain, kTolMs);
            cmp ("density",      e.dens,  kTolPct);
            cmp ("feedback",     e.fb,    kTolPct);
            cmp ("lowCut",       e.lo,    kTolLoHz);
            cmp ("highCut",      e.hi,    kTolHiHz);
            cmp ("width",        e.width, kTolPct);
            cmp ("mix",          e.mix,   kTolPct);
            // v1.2.0 (B1). grainTilt's 0.001 step resolves 0.5 exactly, so the
            // 0.002 tolerance is two steps — tight enough that a preset written
            // as 0 (the wrong "neutral") fails by 250x.
            cmp ("grainTilt",    e.tilt,  0.002f);
            cmp ("grainShape",   e.shape, kTolChoice);

            const bool values = loaded && inRange;

            // (b) render — finite, bounded, and audibly alive.
            // 2 s broadband burst then silence, as probe G. The wash window
            // starts right after the burst so low-feedback presets (Vocal Halo
            // at fb=30 decays in ~2 s) are measured where they actually sound,
            // not after they have legitimately died away.
            juce::Random rng ((juce::int64) 0x0feedbac);
            auto fill = [&] (int t)
            {
                return t < exciteLen ? (float) (A * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
            };

            auto y = renderEffect (proc, e.seconds, fs, block, fill);

            const double pk      = juce::jmax (peakAbs (y.L), peakAbs (y.R));
            const double washRms = rms (y.L, (int) (2.0 * fs), (int) (2.0 * fs));

            const bool audio = allFinite (y.L) && allFinite (y.R)
                            && pk < 1.0
                            && washRms > 1.0e-5;

            check ((juce::String ("preset-") + e.name).toRawUTF8(),
                   values && audio,
                   juce::String ("loaded=") + (loaded ? "1" : "0")
                     + " worst=" + juce::String (worst, 4) + "(" + worstId
                     + " " + juce::String (worstRatio * 100.0f, 1) + "% of tol)"
                     + " peak=" + juce::String (pk, 4)
                     + " washRms[2..4s]=" + juce::String (washRms, 7)
                     + " " + juce::String (e.seconds, 0) + "s");
        }

        proc.setPlayHead (nullptr);
    }

    // --- Probe AH: ring coverage at the v1.5.0 maximum grain ------------------
    //
    // The defect this exists to catch: kGrainSizeMaxMs moved 500 -> 4000 ms, and
    // the capture ring must cover gD_max + 2·G_max. If the ring is too short the
    // engine does NOT fault — the tail of each long grain simply wraps onto
    // material the writer has already overwritten. There is no NaN, no
    // discontinuity at the wrap (the ring is contiguous), and every existing
    // probe still passes. It just plays back the wrong audio.
    //
    // A finiteness/peak check cannot see that, so this probe tests ARRIVAL TIME
    // instead, which the wrap destroys in a way that is trivially measurable.
    //
    // Configuration: D = 4000 ms, G = 4000 ms, mix 100 %, feedback 0, overlap 2,
    // no randomisation, no window tilt. A grain spawned at output sample s reads
    // source over [s − D − G, s − D] = [s − 8 s, s − 4 s]. Excite with a 1 s
    // burst at the very start and hold silence after, so the ONLY source energy
    // lives in [0, 1 s). The read window therefore overlaps the burst only while
    // 4 s < s < 9 s, and the wet output must be silent before ~4 s.
    //
    // Under a too-short ring the read at (s − 8 s) wraps forward by the ring
    // length — with v1.4.0's 6 s ring it would land at (s − 2 s), i.e. RECENT
    // material — and the burst leaks into the early window. So "early window is
    // silent" is exactly the assertion that fails on an undersized ring and
    // passes on a correct one. The static_assert in PluginProcessor.h guards the
    // same invariant at compile time; this one proves it end-to-end in audio.
    {
        setBaseline (apvts);
        setParam (apvts, "syncMode",  0.0f);        // Free — D must be the knob
        setParam (apvts, "delayTime", ReverseDelayProcessor::kDelayTimeMaxMs);
        setParam (apvts, "grainSize", ReverseDelayProcessor::kGrainSizeMaxMs);
        setParam (apvts, "density",   0.0f);        // overlap 2
        setParam (apvts, "feedback",  0.0f);        // no recirculation to muddy timing
        setParam (apvts, "mix",     100.0f);        // wet only
        setParam (apvts, "width",     0.0f);
        clearRandomisation();
        clearWindow();
        proc.prepareToPlay (fs, block);

        const int    burstLen = (int) (1.0 * fs);
        juce::Random rng ((juce::int64) 0x5eed12);
        auto fill = [&] (int t)
        {
            return t < burstLen ? (float) (0.5 * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
        };

        auto y = renderEffect (proc, 11.0, fs, block, fill);

        // Early window [0.5 s, 3.5 s): strictly before the 4 s delay can deliver
        // anything. Starts at 0.5 s to skip the direct-path settling of the very
        // first block regardless of mix.
        const double earlyRms = juce::jmax (rms (y.L, (int) (0.5 * fs), (int) (3.0 * fs)),
                                            rms (y.R, (int) (0.5 * fs), (int) (3.0 * fs)));

        // Arrival window [5 s, 9 s): the reversed burst must actually be here,
        // or the probe would also pass on an engine that output nothing at all.
        const double arrivedRms = juce::jmax (rms (y.L, (int) (5.0 * fs), (int) (4.0 * fs)),
                                              rms (y.R, (int) (5.0 * fs), (int) (4.0 * fs)));

        const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));

        // 60 dB of separation between "nothing has arrived" and "the grain is
        // sounding" — a wrap leaks the burst itself, which is far above this.
        const bool silentEarly = earlyRms < arrivedRms * 1.0e-3;

        check ("ring-cover-maxgrain",
               silentEarly && arrivedRms > 1.0e-4 && pk < 1.0
                 && allFinite (y.L) && allFinite (y.R),
               juce::String ("earlyRms[0.5..3.5s]=") + juce::String (earlyRms, 9)
                 + " arrivedRms[5..9s]=" + juce::String (arrivedRms, 7)
                 + " ratio=" + juce::String (arrivedRms > 0.0 ? earlyRms / arrivedRms : -1.0, 9)
                 + " peak=" + juce::String (pk, 4)
                 + " ring=" + juce::String (ReverseDelayProcessor::kCaptureSeconds, 1) + "s");
    }

    // --- Probe AI: grainSize user-preset migration transform -------------------
    //
    // Guards migrateUserPresets()'s v1.5.0 arm. Deliberately tests the TRANSFORM
    // rather than the file rewrite: the rewrite operates on
    // ~/Library/O-ReverseDelay/Presets/User, and a harness that writes fixtures
    // there would corrupt the developer's real preset library and burn the
    // one-shot version sentinel. The filesystem plumbing is shared with the
    // delayTime arm that has shipped since v1.0.1; the arithmetic is what is new,
    // and the arithmetic is what silently recalls the wrong grain size.
    //
    // Asserts two things, and the second matters as much as the first:
    //   1. Round trip — a stored fraction written against the OLD curve, pushed
    //      through the migration, recalls the SAME milliseconds under the new one.
    //   2. The migration is NECESSARY — the same fraction read directly under the
    //      new curve is materially wrong. Without this, a migration that silently
    //      did nothing would still pass (1).
    {
        juce::NormalisableRange<float> legacy { ReverseDelayProcessor::kGrainSizeMinMs,
                                                ReverseDelayProcessor::kLegacyGrainSizeMaxMs, 0.01f };
        legacy.setSkewForCentre (ReverseDelayProcessor::kLegacyGrainSizeSkewCentreMs);

        const auto& current = apvts.getParameter ("grainSize")->getNormalisableRange();

        // The eight factory grain sizes plus both legacy endpoints.
        const float cases[] = { 50.0f, 70.0f, 120.0f, 180.0f, 200.0f,
                                300.0f, 320.0f, 350.0f, 450.0f, 500.0f };

        double worstErr   = 0.0;   // migrated recall error, ms
        double leastDrift = 1.0e9; // smallest un-migrated error, ms
        float  worstMs = 0.0f, driftMs = 0.0f, driftGot = 0.0f;

        for (float ms : cases)
        {
            const float stored   = legacy.convertTo0to1 (ms);           // what v1.4.0 wrote
            const float migrated = current.convertTo0to1 (
                                       legacy.convertFrom0to1 (stored)); // what v1.5.0 rewrites it to

            const double err = std::abs ((double) current.convertFrom0to1 (migrated) - (double) ms);
            if (err > worstErr) { worstErr = err; worstMs = ms; }

            // Same fraction, read straight off the NEW curve — the bug.
            //
            // Skip the range MINIMUM: 50 ms is a fixed point of both curves
            // (fraction 0.0 maps to `start` at any skew), so it is the one value
            // that legitimately survives without migration and would drag this
            // bound to zero while proving nothing.
            if (ms > ReverseDelayProcessor::kGrainSizeMinMs)
            {
                const double naive = (double) current.convertFrom0to1 (stored);
                const double drift = std::abs (naive - (double) ms);
                if (drift < leastDrift) { leastDrift = drift; driftMs = ms; driftGot = (float) naive; }
            }
        }

        // 0.01 ms is the parameter's own step, so anything at or under it is
        // quantisation rather than a transform error.
        //
        // The drift bound is 1 ms — two orders of magnitude above that step, and
        // deliberately not tighter: both curves are pinned at 50 ms, so drift
        // grows with the value (70 ms recalls ~61 ms, a 13 % error; the old
        // 500 ms endpoint recalls 4000 ms). The SMALLEST interior drift is the
        // honest thing to bound, and it sits at the bottom of the range.
        check ("grainsize-preset-migration",
               worstErr <= 0.01 && leastDrift > 1.0,
               juce::String ("worstRecallErr=") + juce::String (worstErr, 6)
                 + "ms @" + juce::String (worstMs, 0) + "ms"
                 + " | unmigrated drift >= " + juce::String (leastDrift, 1)
                 + "ms (e.g. " + juce::String (driftMs, 0) + " -> "
                 + juce::String (driftGot, 1) + "ms)");
    }

    std::printf ("%s (%d failure%s)\n",
                 failures == 0 ? "ALL PROBES PASSED" : "PROBES FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
