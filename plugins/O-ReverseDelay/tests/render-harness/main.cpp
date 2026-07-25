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

    std::printf ("O-ReverseDelay render-harness — probes 0, A–M (Stage 2) + N (Stage 4), "
                 "fs=%.0f block=%d\n", fs, block);

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
    // density=0 -> back-to-back grains, deterministic spawns at multiples of G.
    {
        setBaseline (apvts);
        setParam (apvts, "density", 0.0f);
        proc.prepareToPlay (fs, block);

        const int D = currentD();
        const int G = currentG();
        const int rampLen = (int) fs;   // 1 s linear ramp 0 -> 1, then silence

        auto ramp = [rampLen] (int t) noexcept
        {
            return (t >= 0 && t < rampLen) ? (float) t / (float) rampLen : 0.0f;
        };

        auto y = renderEffect (proc, 1.3, fs, block, [&] (int t) { return ramp (t); });

        // First grain whose read region [kG-D-G, kG-D] is fully inside the ramp.
        const int k     = (int) std::ceil ((double) D / (double) G) + 1;
        const int start = k * G;

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

        const auto   fit           = linearFit (xs, vs);
        const double expectedSlope = 0.70710677 / (double) rampLen;   // pan gain / ramp span
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
            { "delayTime",    50.0f,  2000.0f, false, true,  true,    0.0f },
            { "syncMode",      0.0f,     1.0f, true,  true,  false, 250.0f },
            { "noteDivision",  0.0f,    12.0f, true,  true,  false,   0.0f },
            { "grainSize",    50.0f,   500.0f, false, true,  false,   0.0f },
            { "density",       0.0f,   100.0f, false, true,  false,   0.0f },
            { "feedback",      0.0f,   100.0f, false, true,  false,   0.0f },
            { "lowCut",       20.0f,  2000.0f, false, false, false,   0.0f },
            { "highCut",     500.0f, 20000.0f, false, false, false,   0.0f },
            { "width",         0.0f,   100.0f, false, false, false,   0.0f },
            { "mix",           0.0f,   100.0f, false, false, false,   0.0f },
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
            double seconds;
        };

        // Mirrors the table in ReverseDelayProcessor's constructor, in
        // ENGINEERING units. Near-Infinite renders 30 s rather than 10: at
        // feedback=100 it is the preset-driven DSP-03 stability statement.
        const FactoryExpect kFactoryExpect[] = {
            { "Reverse Bloom",    0, 6,  500, 200, 60,  40, 100,  8000, 60, 40, 10.0 },
            { "Guitar Swell",     0, 6,  700, 300, 55,  45, 120,  6500, 55, 55, 10.0 },
            { "Vocal Halo",       0, 6,  380, 180, 70,  30, 300,  7000, 70, 25, 10.0 },
            { "Slow Wash",        0, 6, 1400, 450, 30,  65,  80,  5000, 85, 50, 10.0 },
            { "Tight Smear",      0, 6,  180,  70, 90,  35, 150, 11000, 35, 45, 10.0 },
            { "Dark Cavern",      0, 6,  850, 320, 65,  70, 220,  1800, 75, 55, 10.0 },
            { "Near-Infinite",    0, 6,  900, 350, 70, 100, 180,  2500, 80, 50, 30.0 },
            { "Rhythmic Reverse", 1, 4,  500, 120, 80,  50, 140,  9000, 50, 45, 10.0 },
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
            // the printed "worst" always reflects all ten, not just those before
            // the first failure. "worst" is the param with the largest delta
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

    std::printf ("%s (%d failure%s)\n",
                 failures == 0 ? "ALL PROBES PASSED" : "PROBES FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
