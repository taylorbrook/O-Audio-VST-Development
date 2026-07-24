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

    Phase 2.3 scaffolds included now (PLAN Task 1): MockPlayHead (sync
    probes I/J), continuityFraction.

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
    setParam (a, "syncMode",       0.0f);   // Free — sync engine lands in Phase 2.3
    setParam (a, "noteDivision",   6.0f);   // 1/4 (inert in Phase 2.1)
    setParam (a, "grainSize",    200.0f);
    setParam (a, "density",       60.0f);
    setParam (a, "feedback",       0.0f);
    setParam (a, "lowCut",       100.0f);
    setParam (a, "highCut",     8000.0f);
    setParam (a, "width",          0.0f);
    setParam (a, "mix",          100.0f);
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

    std::printf ("O-ReverseDelay render-harness — Phase 2.1 probes, fs=%.0f block=%d\n", fs, block);

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

    std::printf ("%s (%d failure%s)\n",
                 failures == 0 ? "ALL PROBES PASSED" : "PROBES FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
