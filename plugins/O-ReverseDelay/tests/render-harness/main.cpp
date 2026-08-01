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

    v1.7.0 probes (B4 #4 ducking, #5 stereo source, #6 delay drift):
      AR. duck-gap-bloom    — the wet's own level already tracks the input, so a
                              direct "quieter while the dry plays" test would
                              measure the DELAY and pass on a dead knob. Renders
                              the same gated excitation at duck 0 and duck 100
                              and compares window by window, which cancels the
                              wet's dynamics exactly. Plus the no-op, as a
                              rendered fact: two duck-0 renders with a
                              full-depth render between them must be
                              BIT-IDENTICAL.
      AS. duck-depth-monotone— the knob does something proportional across its
                              travel, not just at the ends.
      AT. stereo-source     — three assertions. IMAGE: a hard-left source comes
                              back left in Stereo and centred in Mono Sum.
                              EQUIVALENCE: with a CORRELATED source the two
                              modes are BIT-IDENTICAL, because 0.5·(L+R) is
                              exactly L when L==R — which proves the mode
                              changes what is READ rather than adding a gain.
                              COLLAPSE: at width 0 a Stereo render is still
                              dual-mono (wetL bitwise wetR).
                              Needs renderEffectStereo — every probe before this
                              release drove L and R with the SAME signal, which
                              a stereo-source probe cannot use.
      AU. mono-fold         — the 0.70710677f fold, re-derived rather than
                              assumed: mono OUT is only reachable with mono IN,
                              where the capture ring holds L == R and the two
                              source modes coincide by construction. Asserts
                              that (bitwise) and that the fold is still unity
                              against the stereo width-0 wet.
      AV. drift-ring-clamp  — probe AH's assertion reached from a new direction.
                              Drift MULTIPLIES the latched delay by up to 1.25,
                              so it extends the worst-case read span; an
                              over-reaching read does not fault, it wraps onto
                              overwritten material. D and G both at 4000 ms,
                              scatter at max, drift at full depth: the early
                              window must be silent.
      AW. drift-live/no-op  — depth 0 must be bit-identical across two very
                              different RATES (driftMul returns before touching
                              std::sin), and depth 100 must measurably move the
                              render.
      AX. v170-blocksize    — probe AQ's property with this release's four
                              controls engaged, on a DECORRELATED stereo
                              excitation. This is the line a block-rate duck
                              envelope or a per-block drift phase accumulator
                              fails, and the only line either would fail.
      AY. nonfinite-sticky  — a 10 ms NaN/inf burst must not poison the duck
                              follower for the life of the instance. Measured
                              against the duck-0 path rather than an absolute.

    v1.8.0 (B4 #7-#8) — COLOUR:

      AZ. v180-defaults     — diffusion 0 / drive 0 renders BITWISE the v1.7.3
                              engine, with a same-parameters repeat as the
                              determinism control, and both controls proven
                              audible at 60 % so the probe cannot be satisfied
                              by a pair of dead knobs.
      BA. diffusion-nonexp  — the safety claim made INSTEAD of a measured cap.
                              An allpass is magnitude-flat, so the mixed block
                              is non-expansive at every setting and cannot open
                              a self-oscillation path `feedback` would not.
                              Asserted at feedback 100 against the undiffused
                              engine's own late/early growth, not an absolute —
                              diffusion IS allowed to move energy around in
                              time, just not to add any.
      BB. drive-decay       — the property separating drive from regenMakeup.
                              tanh(d·x)/d has small-signal gain 1 at every d, so
                              the LATE-tail decay rate must not move across the
                              whole knob. regenMakeup fails this by construction
                              at any non-zero setting, which is why it needed a
                              measured cap and this does not. The EARLY tail is
                              expected to differ — that difference is the
                              feature.
      BC. v180-blocksize    — probes AQ/AX again with COLOUR engaged. The
                              allpass ring index is per-sample state carried
                              across block boundaries, which is the exact shape
                              of the v1.0.1 delay-read-latch bug.

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
#include <limits>
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
// v1.7.0 (B4 #5): the same loop with INDEPENDENT L and R fills.
//
// Every probe before this release drove both input channels with the same
// signal, which was fine while the grain engine read CaptureBuffer::monoSum and
// the source's stereo image was discarded before a grain ever saw it. It is
// exactly what a stereo-source probe cannot use: with L == R the two source
// modes read identical material by construction, so a mono excitation would
// make probe AT pass against an engine that ignored the mode entirely.
template <typename FillL, typename FillR>
static StereoRender renderEffectStereo (ReverseDelayProcessor& proc, double seconds, double fs,
                                        int block, FillL&& fillL, FillR&& fillR)
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
        for (int i = 0; i < block; ++i)
        {
            buf.setSample (0, i, fillL (pos + i));
            buf.setSample (1, i, fillR (pos + i));
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

//==============================================================================
// v1.7.0: MONO in / MONO out, for the mono-fold half of probe AT.
//
// A separate loop rather than a flag on renderEffect: the buffer is genuinely
// 1-channel, which is what makes processBlock take its `numOutputChannels <= 1`
// branch — the one place the 0.70710677f fold constant is used. Driving a
// 2-channel buffer and reading channel 0 would exercise the stereo path and
// prove nothing about the fold.
template <typename FillFn>
static std::vector<float> renderEffectMono (ReverseDelayProcessor& proc, double seconds,
                                            double fs, int block, FillFn&& fill)
{
    const int total = (int) (seconds * fs);
    juce::AudioBuffer<float> buf (1, block);
    juce::MidiBuffer midi;

    std::vector<float> out;
    out.reserve ((size_t) total);

    int pos = 0;
    while (pos < total)
    {
        for (int i = 0; i < block; ++i)
            buf.setSample (0, i, fill (pos + i));

        proc.processBlock (buf, midi);

        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i)
            out.push_back (buf.getSample (0, i));

        pos += block;
    }
    return out;
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

    // v1.6.0 (B4 #1-#3): the MOTION panel, reset here for the fifth release
    // running and for exactly the reason the four above it are — probes AM-AQ
    // sweep all three, and every probe that follows them calls setBaseline().
    //
    // These are the FIRST additions to this function whose neutral value really
    // is zero, which makes them the easy case and therefore the one to be
    // careful about copying from: grainTilt (0.5), grainCount (8) and tukeyTaper
    // (0.5) are all still here because their no-op is NOT the range minimum.
    //
    // freeze matters most of the three. A probe that left it engaged would leave
    // every later probe rendering against a ring nothing is writing to, so the
    // wash would simply stop after one ring lap — which reads as a catastrophic
    // DSP regression and is harness state leaking forward, the exact failure the
    // v1.2.0 comment above describes.
    setParam (a, "freeze",         0.0f);
    setParam (a, "direction",      0.0f);
    setParam (a, "regenMakeup",    0.0f);

    // v1.7.0 (B4 #4-#6): SOURCE / DUCK / DRIFT, reset here for the sixth release
    // running and for the reason every block above it gives — probes AR-AV sweep
    // all four, and every probe that follows them calls setBaseline().
    //
    // driftRate is the one to be careful with, and it is a NEW disguise of the
    // grainTilt/grainCount/tukeyTaper trap rather than a repeat of it: its
    // neutral value is the parameter's own DEFAULT (0.30 Hz), not the range
    // minimum, because what makes it inert is driftDepth being 0. Writing 0.0f
    // here would be clamped up to kDriftRateMinHz — so it would not fail, it
    // would just leave every later probe running a slower LFO than the plugin
    // ships, at a setting where the LFO does nothing anyway. Silent until
    // somebody raises depth.
    //
    // duck matters most of the four for leakage, in the way freeze does: a probe
    // that left it engaged would leave every later probe measuring a wet path
    // whose level tracks the excitation's envelope, which reads as a level or
    // decay regression rather than as harness state leaking forward.
    setParam (a, "sourceMode",     0.0f);   // Mono Sum
    setParam (a, "duck",           0.0f);
    setParam (a, "driftRate",      ReverseDelayProcessor::kDriftRateCentreHz);
    setParam (a, "driftDepth",     0.0f);
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
    setParam (a, "freeze",         0.0f);   // v1.6.0 defaults — all three no-ops
    setParam (a, "direction",      0.0f);
    setParam (a, "regenMakeup",    0.0f);
    setParam (a, "sourceMode",     0.0f);   // v1.7.0 defaults — Mono Sum,
    setParam (a, "duck",           0.0f);   // duck off, drift off, and the rate
    setParam (a, "driftRate",      ReverseDelayProcessor::kDriftRateCentreHz);
    setParam (a, "driftDepth",     0.0f);   // at its DEFAULT, not its minimum
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
            // v1.7.0 (B4 #4-#6). Three of the four take the LATCHED-CONTENT
            // (loose) tier and one does not, and the split is the point:
            //
            //   sourceMode / driftRate / driftDepth all change what a NEW grain
            //   reads — which channel, and from how far back — while in-flight
            //   grains finish on what they latched. That is the same legitimate
            //   read-reseating grainSize and delayScatter already get the loose
            //   tier for, and a source or delay that was NOT latched would show
            //   up here as a click.
            //
            //   duck does not touch the grains at all: it is a smooth gain on
            //   the wet, downstream of everything, so it gets the SMOOTH tier.
            //   That is the assertion — a duck implemented as a per-block gain
            //   step rather than a per-sample envelope would fail this line and
            //   pass every other probe in the suite.
            { "sourceMode",    0.0f,     1.0f, true,  true,  false,   0.0f },
            { "duck",          0.0f,   100.0f, false, false, false,   0.0f },
            { "driftRate",     0.02f,    5.0f, false, true,  true,    0.0f },
            { "driftDepth",    0.0f,   100.0f, false, true,  true,    0.0f },
            // v1.8.0 (B4 #7-#8). Both take the SMOOTH tier, and neither is a
            // latched-content parameter — they act on the feedback RETURN, which
            // is a per-sample path with no grain state in it at all, so there is
            // nothing to latch and no legitimate read-reseating to excuse.
            //
            // That makes these two lines a real assertion rather than a
            // formality. Neither control is smoothed in the processor, and both
            // are allowed to be for a specific reason (see the block-rate reads
            // in processBlock): diffusion crossfades between two unity-MAGNITUDE
            // paths so a step moves phase and not level, and drive scales the
            // argument of a function whose small-signal gain is 1 at every
            // setting so a step leaves quiet material where it was. If either
            // claim is wrong the sweep steps the loop's level and this tier
            // catches it — which is exactly the check a SmoothedValue would have
            // hidden.
            { "diffusion",     0.0f,   100.0f, false, false, false,   0.0f },
            { "drive",         0.0f,   100.0f, false, false, false,   0.0f },
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
    // v1.7.2 (WR-02): this probe used to reproduce the header's derivation
    //
    //   spawns/pass = passLen / interval, passLen <= kDelayTimeMinMs·fs,
    //   interval = G/overlap >= (kGrainSizeMinMs·fs)/overlapMax
    //   -> 16 · 50/50 = 16 nominal, against a cap of 128
    //
    // verbatim, and then pinned delayTime to kDelayTimeMinMs — which is the ONE
    // configuration in which the false premise on line 2 happens to be true.
    // `passLen <= kDelayTimeMinMs·fs` only holds when delayScatter > 0; with
    // scatter at 0 (the shipped default) passBound is D, so passLen is governed by
    // the HOST BLOCK SIZE. Pinning delayTime to its minimum forces
    // D == minDelaySamples and hides exactly that.
    // See GrainScheduler::kMaxSpawnsPerBlock for the corrected bound.
    //
    // The `min` case below is kept as-is (it is a legitimate configuration and the
    // historical regression guard); `longdelay` is the case the old derivation
    // could not see, and it is the real worst case: delayTime at its MAXIMUM so
    // passLen == numSamples, grainSize at its minimum and ceiling 16 so the
    // interval is as short as the parameters allow, scatter explicitly at 0 so the
    // wide pass bound is selected, and 16384 included because several hosts bounce
    // at that block size.
    //
    // Run at multiple block sizes because passLen is derived from the host block
    // size: 512 gives one pass per block, 4096 gives several, and only the
    // second exercises a pass that fills from a mid-block starting countdown.
    {
        struct SpawnCapCase { const char* name; float delayMs; std::vector<int> blocks; };

        const SpawnCapCase spawnCapCases[] = {
            { "min",       ReverseDelayProcessor::kDelayTimeMinMs, { 512, 4096 } },
            { "longdelay", ReverseDelayProcessor::kDelayTimeMaxMs, { 512, 4096, 16384 } },
        };

        for (const auto& scc : spawnCapCases)
        for (const int blk : scc.blocks)
        {
            setBaseline (apvts);
            clearRandomisation();
            clearWindow();
            setParam (apvts, "grainCount",   16.0f);
            setParam (apvts, "grainSize",    ReverseDelayProcessor::kGrainSizeMinMs);
            setParam (apvts, "delayTime",    scc.delayMs);
            setParam (apvts, "density",      100.0f);
            setParam (apvts, "jitter",       100.0f);
            setParam (apvts, "feedback",      60.0f);
            setParam (apvts, "width",        100.0f);
            setParam (apvts, "mix",          100.0f);
            // Explicit: 0 is what selects the WIDE pass bound, which is the whole
            // point of the longdelay case. clearRandomisation() already zeroes it;
            // stated here so a future edit to that helper cannot quietly turn this
            // back into the scatter-bounded configuration.
            setParam (apvts, "delayScatter",   0.0f);

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
            check ((juce::String ("spawncap-headroom-") + scc.name + "-" + juce::String (blk)).toRawUTF8(),
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

    //==========================================================================
    // v1.6.0 probes (AM–AQ) — the MOTION panel (B4 #1-#3)
    //==========================================================================

    // --- Probe AM: the forward read law is a delay tap, exactly --------------
    //
    // The assertion this release most needs, because the read-law change is one
    // token and its consequences are not.
    //
    // A grain latches readAbs = s − gD at spawn sample s and steps ±1 while the
    // write head advances +1, so at output time t (index n = t − s) it reads
    //     reverse:  2s − gD − t     — depends on s, so grains decorrelate
    //     forward:       t − gD     — does NOT
    // i.e. at direction 100 the wet path is the INPUT DELAYED BY gD and nothing
    // else. That is asserted here by correlating the wet output against a
    // synthesised delayed copy of the excitation, which tests three things at
    // once and would fail differently for each:
    //
    //   * the direction blend reaches the engine at all (a dead parameter gives
    //     the reverse render, correlation ~0),
    //   * the delay is gD and not gD ± G or 2·gD (a mis-latched anchor),
    //   * no forward grain reads capture that has not been written this pass.
    //     That last one is the review's "forward grains may need their own
    //     headroom clamp" question, answered in audio: an unwritten read returns
    //     either silence or a full ring lap of stale material, and either
    //     destroys the correlation. (It cannot happen — at pass-relative index k
    //     a forward grain reads passStartAbs − gD + k and A2's bound already
    //     gives k < passLen <= grainDelayFloor <= gD — but "cannot happen" was
    //     also true of the v1.0.0 spawn cap that nothing measured.)
    //
    // The direction-0 control is what gives the probe teeth: the same
    // correlation against the same reference must be near ZERO for the reverse
    // engine, or the test would pass on any delay-like output.
    {
        auto renderDir = [&] (float dir)
        {
            setBaseline (apvts);
            setParam (apvts, "syncMode",  0.0f);     // Free — D must be the knob
            setParam (apvts, "delayTime", 500.0f);
            setParam (apvts, "grainSize", 200.0f);
            setParam (apvts, "density",     0.0f);   // overlap 2, the cleanest sum
            setParam (apvts, "feedback",    0.0f);   // no recirculation to muddy it
            setParam (apvts, "width",       0.0f);   // centred, so L is the whole wet
            setParam (apvts, "mix",       100.0f);   // wet only
            setParam (apvts, "direction",   dir);
            clearRandomisation();
            clearWindow();
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 3.0, fs, block,
                                 [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });
        };

        const int Dsamp = juce::jmax (1, (int) (500.0f * 0.001 * fs));

        auto fwd = renderDir (100.0f);
        auto rev = renderDir (0.0f);

        // The excitation, delayed by exactly D. Built from the same
        // position-deterministic generator the render used, so this is the
        // engine's own input and not an approximation of it.
        std::vector<float> delayed (fwd.L.size(), 0.0f);
        for (size_t i = (size_t) Dsamp; i < delayed.size(); ++i)
            delayed[i] = (float) (kRandA * whiteNoiseAt ((int) i - Dsamp));

        const int winOff = (int) (1.2 * fs);
        const int winLen = (int) (1.5 * fs);

        const double corrFwd = corrRange (fwd.L, delayed, winOff, winLen);
        const double corrRev = corrRange (rev.L, delayed, winOff, winLen);

        const double pk = juce::jmax (peakAbs (fwd.L), peakAbs (fwd.R));

        check ("direction-forward-is-delay",
               corrFwd > 0.95 && std::abs (corrRev) < 0.10
                 && allFinite (fwd.L) && allFinite (fwd.R) && pk < 1.0,
               juce::String ("corr(wet, in delayed by D) fwd=") + juce::String (corrFwd, 4)
                 + " (>0.95) rev=" + juce::String (corrRev, 4) + " (|.|<0.10)"
                 + " D=" + juce::String (Dsamp) + " peak=" + juce::String (pk, 4));
    }

    // --- Probe AN: direction is level-flat, and loop-neutral -----------------
    //
    // Two measurements, because a control that changes the read law can be heard
    // as a volume knob and as a feedback knob independently — which is precisely
    // how v1.2.0 (shape), v1.3.0 (overlap) and v1.4.0 (taper) each went wrong.
    //
    // (a) OUTPUT LEVEL. Uncompensated, direction 100 is +7.3 dB at overlap 8:
    //     the forward set adds in amplitude (N·m) where the reverse set adds in
    //     power (sqrt(N·q)). WindowLut::getForwardNorm cancels that exactly at
    //     the endpoints. It does NOT cancel it in the middle, and that is
    //     derived rather than overlooked — with each grain independently forward
    //     with probability p, the expected wet power works out to
    //         q · [ p² + p(1−p)·q/(N·m²) + (1−p) ]
    //     which is q at p = 0 and at p = 1 and dips to ~0.82·q at p = 0.5, i.e.
    //     a ~0.9 dB sag mid-travel. That is inside the ±1 dB budget probes D and
    //     Z2 use, and closing it would take a second p-dependent factor fitted
    //     to a curve rather than derived from a summing law — which this file's
    //     rule says not to do. Bounded loosely and REPORTED, so the sag is a
    //     printed number rather than a surprise.
    //
    // (b) LOOP. The feedback tap takes NO direction trim, on the argument that
    //     getLoopNorm already models the loop as a coherent sum so forward and
    //     reverse grains contribute identically through it. That argument is
    //     load-bearing and cheap to be wrong about — forward grains are
    //     EXACTLY coherent where reverse ones are only approximately so — so the
    //     decay rate is measured across the blend at feedback 100 and every rate
    //     must still be NEGATIVE, the assertion that caught v1.3.0's runaway.
    //
    // Uses whiteNoiseAt for probe AA's reason: this varies what the grains read
    // relative to one another, so a coloured excitation would fold its own
    // correlation structure into the answer.
    {
        const float kDirs[] = { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f };

        // (a) — wet level across the blend, scatter OFF.
        {
            double loRms = 1.0e30, hiRms = 0.0;
            juce::String detail;
            bool ok = true;

            for (const float d : kDirs)
            {
                setBaseline (apvts);
                setParam (apvts, "density",   60.0f);
                setParam (apvts, "feedback",   0.0f);   // output path only
                setParam (apvts, "width",      0.0f);
                setParam (apvts, "mix",      100.0f);
                setParam (apvts, "direction",     d);
                clearRandomisation();
                clearWindow();
                proc.prepareToPlay (fs, block);

                auto y = renderEffect (proc, 3.0, fs, block,
                                       [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });

                const double r = rms (y.L, (int) (1.0 * fs), (int) (2.0 * fs));
                loRms = juce::jmin (loRms, r);
                hiRms = juce::jmax (hiRms, r);

                detail << juce::String ((int) d) << "%=" << juce::String (r, 5) << " ";
                ok = ok && allFinite (y.L) && allFinite (y.R)
                        && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0;
            }

            const double spreadDb = (loRms > 0.0) ? 20.0 * std::log10 (hiRms / loRms) : 99.0;

            // 1.5 dB, not 1.0: the ~0.9 dB mid-travel sag above is a property of
            // mixing a coherent set with an incoherent one, and a 1.0 dB bound
            // would be asserting that away. Uncompensated this reads 7.3 dB, so
            // the bound is still 5x below the defect it exists to catch.
            check ("level-flat-direction", ok && spreadDb < 1.5,
                   juce::String ("rms ") + detail
                     + "spread=" + juce::String (spreadDb, 3)
                     + " dB (<1.50; uncompensated is ~7.3)");
        }

        // (b) — the loop, at feedback 100.
        {
            const int exciteLen = (int) (2.0 * fs);

            double zeroDecay = 0.0, worstDelta = 0.0, worstPeak = 0.0;
            int    worstDir = 0;
            juce::String detail;
            bool   ok = true;

            for (const float d : kDirs)
            {
                setBaseline (apvts);
                setParam (apvts, "density",   60.0f);
                setParam (apvts, "feedback", 100.0f);
                setParam (apvts, "width",      0.0f);   // worst case for the loop (probe AG)
                setParam (apvts, "mix",      100.0f);
                setParam (apvts, "direction",     d);
                clearRandomisation();
                clearWindow();
                proc.prepareToPlay (fs, block);

                juce::Random rng ((juce::int64) 0x0feedbac);
                auto fill = [&] (int t)
                {
                    return t < exciteLen ? (float) (kRandA * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
                };

                auto y = renderEffect (proc, 30.0, fs, block, fill);

                const double w1 = rms (y.L, (int)  (5.0 * fs), (int) (5.0 * fs));
                const double w2 = rms (y.L, (int) (20.0 * fs), (int) (5.0 * fs));
                const double decay = (w1 > 0.0 && w2 > 0.0)
                                       ? 20.0 * std::log10 (w2 / w1) / 15.0 : 0.0;

                if (juce::exactlyEqual (d, 0.0f))
                    zeroDecay = decay;
                else if (std::abs (decay - zeroDecay) > worstDelta)
                {
                    worstDelta = std::abs (decay - zeroDecay);
                    worstDir   = (int) d;
                }

                const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));
                worstPeak = juce::jmax (worstPeak, pk);
                detail << juce::String ((int) d) << "%=" << juce::String (decay, 3) << " ";

                ok = ok && allFinite (y.L) && allFinite (y.R) && pk < 1.0 && decay < 0.0;
            }

            // ── The measured result, and why the bound is 1.20 and not 0.25 ──
            //
            // Measured: 0%=-2.49  25%=-2.79  50%=-2.91  75%=-3.58  100%=-3.21
            // dB/s. Every rate NEGATIVE — no direction setting self-oscillates,
            // which is the assertion that matters and the one that caught
            // v1.3.0's runaway — but the tail does get shorter as forward grains
            // blend in, by up to 1.09 dB/s.
            //
            // That is not a missing trim, it is the topology. At direction 100
            // the loop IS a plain feedback delay: N grains reading one sample,
            // summing to N·m·g·loopTrim = 1.18 before the width-0 mono-sum's
            // 0.7071, i.e. 0.836 per 500 ms generation = -3.1 dB/s, which is
            // what the 100 % column reads. The reverse loop is a smear across
            // 2·interval-separated reads and recirculates a little more. Two
            // genuinely different feedback structures cannot have the same decay
            // rate, and normalising them to each other would mean making the
            // forward delay quieter than a delay should be.
            //
            // The dip at 75 % is the same mechanism as the output-level sag in
            // (a): a MIX of the two sets is mutually decorrelated, so the loop
            // amplitude is sqrt(fwd² + rev²) — less than either pure case.
            //
            // 1.20 dB/s is the bound decay-shape-fb100 already uses for window
            // shape, so this control is held to the tolerance the suite has
            // already accepted for a character control. It is 6x below the
            // 7.3 dB/generation the OUTPUT trim removes.
            check ("decay-direction-fb100", ok && worstDelta < 1.20,
                   juce::String ("dB/s ") + detail
                     + "worst-vs-0=" + juce::String (worstDelta, 3)
                     + " (dir " + juce::String (worstDir) + ", <1.20)"
                     + " peak=" + juce::String (worstPeak, 4));
        }
    }

    // --- Probe AO: regen makeup at the ceiling stays bounded ------------------
    //
    // The probe that SETS ReverseDelayProcessor::kRegenMakeupMaxDb rather than
    // merely checking it. The constant is a measured stability bound: makeup is
    // a gain inside a recirculating path, so past some value the loop stops
    // decaying and drives into the tanh — which is the point of the control —
    // and the question the cap answers is how far past that the WET OUTPUT
    // stays under 1.0.
    //
    // The tanh does not answer it. It bounds the LOOP to ±1 per sample, but the
    // output is a sum of `overlap` grains each reading loop content, and
    // 1/sqrt(overlap) against a near-coherent sum is exactly the shortfall that
    // produced v1.3.0's 1.28 peak. So this renders everything the parameters can
    // stack at once — overlap 16, feedback 100, width 0 (the loop's worst case,
    // see probe AG), makeup at maximum, both direction extremes, all five window
    // shapes — for 90 s, which is long enough that a per-generation gain even
    // slightly above unity has to show itself.
    //
    // What is asserted is deliberately NOT probe AG's monotone decay: at max
    // makeup the loop is SUPPOSED to sustain or grow into the limiter, and
    // demanding a decay would be asserting the feature away. What must hold is
    // that it CONVERGES — the last window no louder than the worst earlier one
    // by more than a hair — and stays finite.
    //
    // The peak is REPORTED, not bounded at 1.0, and that is the honest form of
    // the result rather than a weakened assertion. The tanh bounds the loop at
    // every setting; it does not bound the wet path's sqrt(overlap)·mean·
    // windowNorm sum of limited content — the v1.3.0 mechanism, made reachable
    // again by deliberately allowing self-oscillation. Measured at 6 dB it runs
    // 0.99 (Hann, ceiling 8) to 1.55 (Tukey, ceiling 16), and no cap that
    // reaches sustain anywhere avoids it (see kRegenMakeupMaxDb). The peak < 1.0
    // invariant belongs to the non-self-oscillating engine, which is regen 0 dB
    // — the default, every factory preset and every pre-v1.6.0 session.
    //
    // What is still required everywhere: finite, CONVERGENT, and under a hard
    // 1.8. A genuine runaway — the v1.3.0 defect — fails all three, so this is
    // not a check that has been turned off, it is one that has been aimed at
    // what actually distinguishes saturation from instability.
    {
        const int exciteLen = (int) (2.0 * fs);

        // Ceiling on the reported peak. Well above the 1.55 the parameters can
        // reach and well below anything a climbing loop would produce, so it is
        // a runaway tripwire rather than a level statement.
        constexpr double kHardPeakBound = 1.8;

        struct MaxCase { const char* name; float count, density; };

        const MaxCase kMaxCases[] = {
            { "ceil8",  ReverseDelayProcessor::kLegacyOverlapMax,  100.0f },
            { "ceil16", ReverseDelayProcessor::kOverlapCeilingMax, 100.0f },
        };

        for (const auto& mc : kMaxCases)
        for (const float dir : { 0.0f, 100.0f })
        {
            double worstPeak = 0.0, worstGrowth = -1.0e30;
            int    worstShape = -1;
            juce::String detail;
            bool   ok = true;

            for (const int sh : { WindowLut::hann, WindowLut::tukey, WindowLut::gaussian,
                                  WindowLut::triangular, WindowLut::expoDecay })
            {
                setBaseline (apvts);
                setParam (apvts, "grainCount",  mc.count);
                setParam (apvts, "density",     mc.density);
                setParam (apvts, "feedback",  100.0f);
                setParam (apvts, "width",       0.0f);
                setParam (apvts, "mix",       100.0f);
                setParam (apvts, "direction",    dir);
                setParam (apvts, "grainShape", (float) sh);
                setParam (apvts, "regenMakeup",
                          ReverseDelayProcessor::kRegenMakeupMaxDb);
                clearRandomisation();
                proc.prepareToPlay (fs, block);

                juce::Random rng ((juce::int64) 0x0feedbac);
                auto fill = [&] (int t)
                {
                    return t < exciteLen ? (float) (kRandA * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
                };

                auto y = renderEffect (proc, 90.0, fs, block, fill);

                const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));
                worstPeak = juce::jmax (worstPeak, pk);

                // Convergence: the tail window against the loudest earlier one.
                // A saturating self-oscillator settles (growth <= 0); a runaway
                // that has not yet reached the limiter is still climbing.
                double earlyMax = 0.0;
                for (int s = 5; s <= 45; s += 10)
                    earlyMax = juce::jmax (earlyMax, rms (y.L, (int) (s * fs), (int) (2.0 * fs)));

                const double lateRms = rms (y.L, (int) (85.0 * fs), (int) (2.0 * fs));
                const double growth   = (earlyMax > 0.0 && lateRms > 0.0)
                                          ? 20.0 * std::log10 (lateRms / earlyMax) : 0.0;

                if (growth > worstGrowth) { worstGrowth = growth; worstShape = sh; }

                detail << juce::String (sh) << ":pk" << juce::String (pk, 3)
                       << "/g" << juce::String (growth, 2) << " ";

                ok = ok && allFinite (y.L) && allFinite (y.R) && pk < kHardPeakBound;
            }

            // Growth bound is +0.5 dB over 40 s, not 0: the trajectory is a wash
            // measured in 2 s windows and it wanders. What it must not do is
            // climb.
            check ((juce::String ("regen-max-") + mc.name + "-dir"
                      + juce::String ((int) dir)).toRawUTF8(),
                   ok && worstGrowth < 0.5,
                   juce::String ("shape:peak/growth ") + detail
                     + "| worstPeak=" + juce::String (worstPeak, 4)
                     + " (<" + juce::String (kHardPeakBound, 1)
                     + " runaway tripwire; the peak<1.0 invariant is the 0 dB engine"
                       " — see kRegenMakeupMaxDb)"
                     + " worstGrowth=" + juce::String (worstGrowth, 3)
                     + " dB (shape " + juce::String (worstShape) + ", <0.5)"
                     + " @" + juce::String (ReverseDelayProcessor::kRegenMakeupMaxDb, 1) + " dB");
        }

        // ── The ladder that SETS the cap ────────────────────────────────────
        //
        // Peak and decay against makeup, in 1 dB steps up to the ceiling, at the
        // worst configuration the parameters allow. This is the measurement
        // kRegenMakeupMaxDb is read off, and it is kept in the suite rather than
        // done once and deleted so that raising the ceiling later means re-running
        // it instead of re-reasoning about it.
        //
        // Two numbers per rung, and they answer different questions:
        //   peak  — is the OUTPUT bounded? The tanh bounds the loop to ±1, but the
        //           wet path sums `overlap` grains reading loop content and
        //           1/sqrt(overlap) does not bound a near-coherent sum. This is
        //           the v1.3.0 shortfall, and it is what the cap is against.
        //   decay — has the loop reached sustain? Crossing zero is the whole
        //           point of the control, so the rung where it does is reported.
        //
        // Reported for EVERY rung, not just the last, so the shape of the
        // approach is visible: peak climbs slowly while the loop is still
        // decaying and then jumps once it starts driving the limiter.
        // Two configurations, because they answer the two halves of "how high
        // may the cap go":
        //
        //   SHIPPED — grainCount 8 (the default ceiling), density 65, Hann:
        //     the "Near-Infinite" shape the review's motivation names. The rung
        //     where THIS reaches sustain is the smallest cap that does the job.
        //
        //   CORNER  — grainCount 16, density 100, Tukey: everything the
        //     parameters can stack. Its peak is the safety number.
        //
        // Both at feedback 100, width 0 (probe AG: width 0 is the loop's worst
        // case, not width 100).
        struct RegenCase { const char* name; float count, density; int shape; };

        const RegenCase kRegenCases[] = {
            { "shipped", ReverseDelayProcessor::kLegacyOverlapMax,  65.0f, WindowLut::hann  },
            { "corner",  ReverseDelayProcessor::kOverlapCeilingMax, 100.0f, WindowLut::tukey },
        };

        for (const auto& rc : kRegenCases)
        {
            juce::String ladder;
            double worstPeak = 0.0;
            float  sustainAt = -1.0f;
            bool   finite = true;

            for (float mk = 0.0f; mk <= ReverseDelayProcessor::kRegenMakeupMaxDb + 0.01f; mk += 1.0f)
            {
                setBaseline (apvts);
                setParam (apvts, "grainCount",  rc.count);
                setParam (apvts, "density",     rc.density);
                setParam (apvts, "feedback",  100.0f);
                setParam (apvts, "width",       0.0f);
                setParam (apvts, "mix",       100.0f);
                setParam (apvts, "grainShape", (float) rc.shape);
                setParam (apvts, "regenMakeup",  mk);
                clearRandomisation();
                proc.prepareToPlay (fs, block);

                juce::Random rng ((juce::int64) 0x0feedbac);
                auto fill = [&] (int t)
                {
                    return t < exciteLen ? (float) (kRandA * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
                };

                auto y = renderEffect (proc, 40.0, fs, block, fill);

                const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));
                const double w1 = rms (y.L, (int)  (5.0 * fs), (int) (5.0 * fs));
                const double w2 = rms (y.L, (int) (30.0 * fs), (int) (5.0 * fs));
                const double decay = (w1 > 0.0 && w2 > 0.0)
                                       ? 20.0 * std::log10 (w2 / w1) / 25.0 : 0.0;

                worstPeak = juce::jmax (worstPeak, pk);
                if (sustainAt < 0.0f && decay >= 0.0)
                    sustainAt = mk;

                ladder << juce::String (mk, 0) << ":" << juce::String (pk, 3)
                       << "/" << juce::String (decay, 2) << " ";

                finite = finite && allFinite (y.L) && allFinite (y.R);
            }

            // Finiteness is the hard assertion; peak and the rung at which
            // sustain arrives are REPORTED, and the report is the point — this is
            // the measurement kRegenMakeupMaxDb is read off, printed rather than
            // asserted so that a future change to the loop shows up as a moved
            // ladder rather than as a bound that still happens to pass.
            check ((juce::String ("regen-cap-ladder-") + rc.name).toRawUTF8(), finite,
                   juce::String ("dB:peak/decay ") + ladder
                     + "| worstPeak=" + juce::String (worstPeak, 4)
                     + " sustainAt=" + juce::String (sustainAt, 0) + "dB"
                     + " | ceiling=" + juce::String (ReverseDelayProcessor::kRegenMakeupMaxDb, 1) + "dB");
        }

        // And the control must actually REACH sustain — the failure class every
        // "must stay bounded" assertion above would pass, and the whole reason
        // the review asked for this parameter. Measured at the configuration the
        // motivation names: feedback 100 with the shipped ceiling, i.e. what
        // "Near-Infinite" is. At 0 dB it decays (that is the shipped behaviour
        // and the presets depend on it); at the ceiling it must not.
        {
            juce::String detail;
            double atZero = 0.0, atMax = 0.0;
            bool ok = true;

            for (const float mk : { 0.0f, ReverseDelayProcessor::kRegenMakeupMaxDb })
            {
                setBaseline (apvts);
                setParam (apvts, "density",    65.0f);   // Near-Infinite's own
                setParam (apvts, "feedback",  100.0f);
                setParam (apvts, "width",       0.0f);
                setParam (apvts, "mix",       100.0f);
                setParam (apvts, "regenMakeup",  mk);
                clearRandomisation();
                clearWindow();
                proc.prepareToPlay (fs, block);

                juce::Random rng ((juce::int64) 0x0feedbac);
                auto fill = [&] (int t)
                {
                    return t < exciteLen ? (float) (kRandA * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
                };

                auto y = renderEffect (proc, 30.0, fs, block, fill);

                const double w1 = rms (y.L, (int)  (5.0 * fs), (int) (5.0 * fs));
                const double w2 = rms (y.L, (int) (20.0 * fs), (int) (5.0 * fs));
                const double decay = (w1 > 0.0 && w2 > 0.0)
                                       ? 20.0 * std::log10 (w2 / w1) / 15.0 : 0.0;

                if (juce::exactlyEqual (mk, 0.0f)) atZero = decay; else atMax = decay;
                detail << juce::String (mk, 1) << "dB=" << juce::String (decay, 3) << " ";

                ok = ok && allFinite (y.L) && allFinite (y.R)
                        && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0;
            }

            check ("regen-reaches-sustain",
                   ok && atZero < 0.0 && atMax >= 0.0,
                   juce::String ("decay dB/s ") + detail
                     + "(0 dB must DECAY — the shipped sound — and the ceiling "
                       "must reach sustain, which is what B4 #3 is for)");
        }
    }

    // --- Probe AP: a 60 s frozen render sustains and stays bounded ------------
    //
    // Freeze is the one control here whose whole claim is about what happens a
    // long time after the input stops, so it is rendered for a long time after
    // the input stops.
    //
    // 3 s of excitation, freeze engaged at 3 s, silence from there to 63 s. What
    // must hold, and what each assertion is guarding against:
    //
    //   * The wash is STILL THERE at 60 s. A freeze that skipped pushSample
    //     entirely — the obvious one-line implementation — passes this, but a
    //     freeze that wrote the feedback return into the held ring does not: it
    //     overwrites the material with a decaying copy of itself and fades out
    //     at whatever `feedback` happens to be.
    //   * It has not GROWN. Nothing is recirculating while frozen, so the level
    //     is set by held material and must not climb.
    //   * It is not a BUZZ. Freezing the write head instead of advancing it
    //     makes every grain latch the same readAbs, and the output collapses to
    //     a periodic burst at the spawn interval — which is loud, sustained, and
    //     would pass both assertions above. Caught by comparing the frozen
    //     wash's crest factor against the live wash's: a periodic pulse train
    //     has a much higher peak-to-RMS than a cloud.
    //   * Neither transition CLICKS. Probe C's first-difference detector at both
    //     the freeze and the release edge.
    {
        const int freezeAt  = (int) (3.0 * fs);
        const int releaseAt = (int) (63.0 * fs);

        setBaseline (apvts);
        setParam (apvts, "delayTime", 500.0f);
        setParam (apvts, "grainSize", 200.0f);
        setParam (apvts, "density",    60.0f);
        setParam (apvts, "feedback",   40.0f);
        setParam (apvts, "width",      60.0f);
        setParam (apvts, "mix",       100.0f);   // wet only — dry would mask a dead wash
        clearRandomisation();
        clearWindow();
        proc.prepareToPlay (fs, block);

        // Reference crest factor: the same engine, never frozen, over its live
        // wash. Measured rather than assumed so the buzz bound below is relative
        // to this plugin's own output and not to a number picked from the air.
        auto live = renderEffect (proc, 6.0, fs, block,
                                  [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });
        const double liveRms   = rms (live.L, (int) (3.0 * fs), (int) (2.0 * fs));
        const double liveCrest = liveRms > 0.0 ? peakAbs (live.L) / liveRms : 0.0;

        setBaseline (apvts);
        setParam (apvts, "delayTime", 500.0f);
        setParam (apvts, "grainSize", 200.0f);
        setParam (apvts, "density",    60.0f);
        setParam (apvts, "feedback",   40.0f);
        setParam (apvts, "width",      60.0f);
        setParam (apvts, "mix",       100.0f);
        clearRandomisation();
        clearWindow();
        proc.prepareToPlay (fs, block);

        // Freeze is driven from the per-block callback, i.e. through the same
        // setValueNotifyingHost path a DAW would use — not by poking the atomic.
        auto y = renderEffect (proc, 66.0, fs, block,
                               [&] (int t) { return t < freezeAt
                                                      ? (float) (kRandA * whiteNoiseAt (t))
                                                      : 0.0f; },
                               [&] (int pos, int)
                               {
                                   if (pos >= releaseAt)      setParam (apvts, "freeze", 0.0f);
                                   else if (pos >= freezeAt)   setParam (apvts, "freeze", 1.0f);
                               });

        const double rms10 = rms (y.L, (int) (10.0 * fs), (int) (2.0 * fs));
        const double rms30 = rms (y.L, (int) (30.0 * fs), (int) (2.0 * fs));
        const double rms60 = rms (y.L, (int) (60.0 * fs), (int) (2.0 * fs));

        const double sustainDb = (rms10 > 0.0 && rms60 > 0.0)
                                   ? 20.0 * std::log10 (rms60 / rms10) : -99.0;

        const double frozenPeak  = peakAbs (std::vector<float> (
                                       y.L.begin() + (size_t) (10.0 * fs),
                                       y.L.begin() + (size_t) (60.0 * fs)));
        const double frozenCrest = rms30 > 0.0 ? frozenPeak / rms30 : 0.0;

        const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));

        // ±6 dB over 50 s of holding. Not tighter: the ring is a
        // kCaptureSeconds loop of material whose own level varies, so the 2 s
        // windows at 10 s and 60 s land on different parts of it. What is being
        // asserted is that it neither dies nor runs away — a freeze that wrote
        // the feedback return back in reads −30 dB or worse here.
        const bool sustains = rms60 > 1.0e-4 && std::abs (sustainDb) < 6.0;

        check ("freeze-60s-sustain",
               sustains && pk < 1.0 && allFinite (y.L) && allFinite (y.R),
               juce::String ("rms 10s=") + juce::String (rms10, 6)
                 + " 30s=" + juce::String (rms30, 6)
                 + " 60s=" + juce::String (rms60, 6)
                 + " drift=" + juce::String (sustainDb, 2) + " dB (|.|<6)"
                 + " peak=" + juce::String (pk, 4));

        // 2x the live wash's crest is generous — a same-readAbs buzz measures
        // several times it, and a genuine frozen cloud measures about the same.
        check ("freeze-is-a-wash-not-a-buzz",
               frozenCrest > 0.0 && frozenCrest < liveCrest * 2.0,
               juce::String ("crest frozen=") + juce::String (frozenCrest, 3)
                 + " live=" + juce::String (liveCrest, 3)
                 + " (needs < " + juce::String (liveCrest * 2.0, 3)
                 + "; a stopped write head reads far above this)");

        // Click check at both edges, ±150 ms around each, against probe C's
        // loose tier. The wash itself has structure, so the reference is the
        // largest step the SAME render makes while nothing is being switched.
        auto maxStepIn = [&] (int from, int len)
        {
            double m = 0.0;
            const int lo = juce::jmax (1, from);
            const int hi = juce::jmin ((int) y.L.size(), from + len);
            for (int i = lo; i < hi; ++i)
                m = juce::jmax (m, (double) std::abs (y.L[(size_t) i] - y.L[(size_t) (i - 1)]));
            return m;
        };

        const int   guard    = (int) (0.15 * fs);
        const double stepOn  = maxStepIn (freezeAt  - guard, 2 * guard);
        const double stepOff = maxStepIn (releaseAt - guard, 2 * guard);
        const double stepRef = maxStepIn ((int) (20.0 * fs), (int) (5.0 * fs));

        check ("freeze-transition-clickfree",
               stepOn < juce::jmax (stepRef * 2.0, 0.02)
                 && stepOff < juce::jmax (stepRef * 2.0, 0.02),
               juce::String ("maxStep on=") + juce::String (stepOn, 6)
                 + " off=" + juce::String (stepOff, 6)
                 + " steady-ref=" + juce::String (stepRef, 6)
                 + " (bound " + juce::String (juce::jmax (stepRef * 2.0, 0.02), 6) + ")");
    }

    // --- Probe AQ: all three are block-size invariant ------------------------
    //
    // Probe W2's property, re-asserted with the v1.6.0 controls engaged, because
    // each of them touches exactly the machinery W2 exists to protect:
    //
    //   * direction draws from the SHARED grain xorshift inside the spawn
    //     handler, and the number of spawns per pass depends on the host block
    //     size. Drawing in the wrong place — after obtain() rather than before —
    //     makes consumption depend on pool occupancy and the render stops
    //     matching between 512 and 4096 (the exact defect W2 caught at v1.1.0).
    //   * freeze advances the capture head from a per-pass loop.
    //   * regenMakeup multiplies a per-sample smoothed gain.
    //
    // Everything on at once, including all four v1.1 randomisations, so the
    // streams are being consumed as hard as the parameters allow.
    {
        auto renderAtBlock = [&] (int blk)
        {
            setBaseline (apvts);
            setParam (apvts, "density",      60.0f);
            setParam (apvts, "feedback",     60.0f);
            setParam (apvts, "width",        60.0f);
            setParam (apvts, "mix",         100.0f);
            setParam (apvts, "jitter",       50.0f);
            setParam (apvts, "delayScatter", 80.0f);
            setParam (apvts, "sizeRandom",   40.0f);
            setParam (apvts, "gainRandom",   30.0f);
            setParam (apvts, "direction",    50.0f);
            setParam (apvts, "regenMakeup",
                      ReverseDelayProcessor::kRegenMakeupMaxDb * 0.5f);
            proc.setPlayConfigDetails (2, 2, fs, blk);
            proc.prepareToPlay (fs, blk);
            return renderEffect (proc, 3.0, fs, blk,
                                 [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });
        };

        auto small = renderAtBlock (512);
        auto large = renderAtBlock (4096);

        const double d = juce::jmax (maxAbsDiff (small.L, large.L),
                                     maxAbsDiff (small.R, large.R));

        // Restore the harness-wide play config, as probe O does.
        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);

        check ("motion-blocksize-invariance", d == 0.0,
               juce::String ("max|512-4096| = ") + juce::String (d, 12)
                 + " with direction/regen/all four randomisations engaged"
                 + " rms512=" + juce::String (rms (small.L, (int) (1.0 * fs), (int) (1.5 * fs)), 6));
    }

    // ═══════════════════════════════════════════════════════════════════════
    // v1.7.0 probes (B4 #4-#6: ducking, stereo source, delay drift)
    // ═══════════════════════════════════════════════════════════════════════

    // --- Probe AR: ducking responds to the DRY envelope ----------------------
    //
    // The measurement problem this probe has to solve first: the wet's own level
    // already tracks the input, because the input is what the wet is made of. A
    // naive "wet is quieter while the dry is loud" test would therefore measure
    // the delay, not the duck, and would pass on an engine where `duck` did
    // nothing at all.
    //
    // Solved by rendering the SAME excitation twice - once at duck 0 and once at
    // duck 100 - and comparing the two renders window by window. The ratio
    // cancels the wet's own dynamics exactly, so what is left is the duck gain
    // and nothing else.
    //
    // Excitation is a 1 s loud / 1 s quiet square-gated noise: quiet is 0.02x
    // rather than silence so the wet is genuinely present in both windows and
    // the ratio is well conditioned in each. Windows are placed clear of the
    // 5 ms attack and 250 ms release so neither is measured mid-transition.
    {
        auto gatedNoise = [&] (int t)
        {
            const double sec  = (double) t / fs;
            const bool   loud = ((int) sec) % 2 == 1;      // [1,2), [3,4), ...
            return (float) ((loud ? 1.0 : 0.02) * kRandA * whiteNoiseAt (t));
        };

        auto renderAtDuck = [&] (float duckPct)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",  0.0f);
            setParam (apvts, "delayTime", 500.0f);
            setParam (apvts, "feedback",   40.0f);
            setParam (apvts, "mix",       100.0f);         // wet only
            setParam (apvts, "duck",      duckPct);
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 6.0, fs, block, gatedNoise);
        };

        auto ref     = renderAtDuck (0.0f);
        auto ducked  = renderAtDuck (100.0f);
        auto refTwo  = renderAtDuck (0.0f);

        // Loud window: inside [3,4) s, clear of the 5 ms attack.
        // Quiet window: inside [4,5) s, 700 ms after the gate fell - nearly
        // three release time constants, so the envelope has released.
        const int loudOff  = (int) (3.5 * fs), loudLen  = (int) (0.4 * fs);
        const int quietOff = (int) (4.7 * fs), quietLen = (int) (0.25 * fs);

        const double loudRef  = rms (ref.L,    loudOff,  loudLen);
        const double loudDuck = rms (ducked.L, loudOff,  loudLen);
        const double qtRef    = rms (ref.L,    quietOff, quietLen);
        const double qtDuck   = rms (ducked.L, quietOff, quietLen);

        const double ratioLoud  = loudRef > 0.0 ? loudDuck / loudRef : -1.0;
        const double ratioQuiet = qtRef   > 0.0 ? qtDuck   / qtRef   : -1.0;

        // The duck must bite hard while the dry is loud and be nearly released
        // while it is quiet. Both bounds are loose against the arithmetic the
        // header predicts (~0.44 loud, ~0.9 quiet) and tight against a dead
        // control, which would put BOTH ratios at exactly 1.
        check ("duck-gap-bloom",
               ratioLoud > 0.0 && ratioQuiet > 0.0
                 && ratioLoud < 0.70 && ratioQuiet > 0.82
                 && ratioLoud < ratioQuiet * 0.85,
               juce::String ("wet vs un-ducked: loud=") + juce::String (ratioLoud, 4)
                 + " (" + juce::String (20.0 * std::log10 (juce::jmax (1.0e-9, ratioLoud)), 2) + " dB)"
                 + " quiet=" + juce::String (ratioQuiet, 4)
                 + " (" + juce::String (20.0 * std::log10 (juce::jmax (1.0e-9, ratioQuiet)), 2) + " dB)"
                 + " - needs loud<0.70, quiet>0.82");

        // The no-op, as a rendered fact rather than as an argument about
        // `1 - 0.u`. The full-depth render between the two duck-0 renders is the
        // point: the follower runs unconditionally and carries state across
        // blocks, so anything of it that leaked into the output path at depth 0
        // - a stale gain, a mis-scoped branch - would show up as a difference
        // here and nowhere else in the suite.
        const double d = juce::jmax (maxAbsDiff (ref.L, refTwo.L),
                                     maxAbsDiff (ref.R, refTwo.R));

        check ("duck-zero-is-noop", d == 0.0,
               juce::String ("max|duck0 - duck0| = ") + juce::String (d, 12)
                 + " across an intervening duck-100 render");
    }

    // --- Probe AS: the duck is monotone in depth -----------------------------
    //
    // Probe AR proves the duck responds to the ENVELOPE; this proves the KNOB
    // does something proportional across its travel, which is the other half of
    // "not a dead control" and the half a two-point test misses. Continuous
    // noise, so the envelope is steady and the only variable is depth.
    {
        double prev = 1.0e9;
        bool   monotone = true;
        double firstRms = 0.0, lastRms = 0.0;

        for (float pct : { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f })
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",  0.0f);
            setParam (apvts, "delayTime", 500.0f);
            setParam (apvts, "feedback",   40.0f);
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "duck",      pct);
            proc.prepareToPlay (fs, block);

            auto y = renderEffect (proc, 4.0, fs, block,
                                   [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });

            const double r = rms (y.L, (int) (1.0 * fs), (int) (2.5 * fs));

            if (pct == 0.0f)   firstRms = r;
            if (pct == 100.0f) lastRms  = r;

            if (r > prev) monotone = false;
            prev = r;
        }

        const double totalDb = (firstRms > 0.0 && lastRms > 0.0)
                                 ? 20.0 * std::log10 (lastRms / firstRms) : 0.0;

        // At least 3 dB of travel end-to-end. The header's arithmetic predicts
        // ~7 dB at this excitation level; 3 is the bound that separates a live
        // control from a nearly-dead one without pinning the knee constant.
        check ("duck-depth-monotone",
               monotone && totalDb < -3.0,
               juce::String ("wet rms 0% -> 100% = ") + juce::String (totalDb, 2)
                 + " dB, monotone=" + (monotone ? "yes" : "NO")
                 + " (needs monotone and < -3 dB)");
    }

    // --- Probe AT: the stereo source preserves the image ---------------------
    //
    // Three assertions, and the middle one is the one that would be missing from
    // an obvious version of this probe.
    //
    //   1. IMAGE - a hard-left source must come back on the left in Stereo mode
    //      and be centred in Mono Sum mode. Measured as the L/R energy
    //      asymmetry of the wet, at width 100 where the two modes differ most.
    //   2. EQUIVALENCE - with a CORRELATED input the two modes must be
    //      BIT-IDENTICAL, because monoSum of L == R is L exactly and the two
    //      code paths then read the same numbers. This is what proves the mode
    //      changes what is READ rather than adding a gain or a decorrelator, and
    //      it is exact rather than within a tolerance.
    //   3. COLLAPSE - at width 0 a Stereo-mode render must still produce a
    //      dual-mono wet (wetL bitwise wetR), which is the "collapses sensibly"
    //      requirement. Grains alternate L and R sources there while all panning
    //      centre, so the pair stays identical and the mono fold below stays
    //      unity.
    {
        // Hard-left: L carries noise, R is silent. Two independent generators
        // would also work; silence is the sharper test because a mode that
        // ignored the channel would produce a symmetric wet from asymmetric
        // input, which is precisely the v1.6.0 behaviour.
        auto fillL   = [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); };
        auto fillNil = [ ] (int)   { return 0.0f; };

        auto renderPanned = [&] (float sourceMode, float width)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",   0.0f);
            setParam (apvts, "delayTime", 500.0f);
            setParam (apvts, "feedback",    0.0f);   // one generation - image, not wash
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "width",     width);
            setParam (apvts, "sourceMode", sourceMode);
            proc.prepareToPlay (fs, block);
            return renderEffectStereo (proc, 4.0, fs, block, fillL, fillNil);
        };

        auto monoW100   = renderPanned (0.0f, 100.0f);
        auto stereoW100 = renderPanned (1.0f, 100.0f);

        auto asym = [&] (const StereoRender& y)
        {
            const double l = rms (y.L, (int) (1.0 * fs), (int) (2.5 * fs));
            const double r = rms (y.R, (int) (1.0 * fs), (int) (2.5 * fs));
            return (l + r) > 0.0 ? std::abs (l - r) / (l + r) : 0.0;
        };

        const double asymMono   = asym (monoW100);
        const double asymStereo = asym (stereoW100);

        // Mono Sum discards the image, so its wet is near-symmetric whatever the
        // source did; Stereo keeps it, so a hard-panned source produces a
        // strongly asymmetric wet. 3x separation is far below what the mechanism
        // predicts (half the grains read silence) and far above the pan
        // sequence's own residual imbalance.
        check ("stereo-source-image",
               asymStereo > 0.30 && asymMono < 0.15 && asymStereo > asymMono * 3.0,
               juce::String ("L/R asymmetry - monoSum=") + juce::String (asymMono, 4)
                 + " stereo=" + juce::String (asymStereo, 4)
                 + " (hard-left source, width 100; needs stereo>0.30, mono<0.15)");

        // (2) Correlated input: the two modes must agree BITWISE.
        //
        // FEEDBACK MUST BE 0 here, and finding out why was worth the probe on
        // its own. The identity is about the CAPTURE RING, not about the input:
        // monoSum is 0.5*(L+R) and equals L exactly only while the ring's two
        // channels hold the same numbers. The ring is written with `input +
        // feedback return`, and the feedback return is the WIDTH-PANNED wet - so
        // the moment anything recirculates at width > 0, a perfectly correlated
        // INPUT stops implying a correlated RING and the two read laws
        // legitimately diverge. Measured at feedback 40 / width 60 that
        // divergence is 0.0154: not a rounding error, and not a defect.
        //
        // Width stays at 60 so the pan path is still exercised - it is the
        // recirculation that has to go, not the panning.
        auto renderCorrelated = [&] (float sourceMode)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",   0.0f);
            setParam (apvts, "delayTime", 500.0f);
            setParam (apvts, "feedback",    0.0f);   // see above - NOT optional
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "width",      60.0f);
            setParam (apvts, "sourceMode", sourceMode);
            proc.prepareToPlay (fs, block);
            return renderEffectStereo (proc, 3.0, fs, block, fillL, fillL);
        };

        auto corrMono   = renderCorrelated (0.0f);
        auto corrStereo = renderCorrelated (1.0f);

        const double dCorr = juce::jmax (maxAbsDiff (corrMono.L, corrStereo.L),
                                         maxAbsDiff (corrMono.R, corrStereo.R));

        check ("stereo-source-correlated-identity", dCorr == 0.0,
               juce::String ("max|monoSum - stereo| = ") + juce::String (dCorr, 12)
                 + " on a correlated source at feedback 0 - 0.5*(L+R) is exactly"
                 + " L when L==R, so the two read laws coincide sample-for-sample");

        // (3) Width 0 collapse: the wet pair must stay dual-mono in Stereo mode.
        //
        // NOT asserted as bit-equality, and that correction belongs to v1.0.0
        // rather than to this release. At width 0 the pan is exactly 0.5, so the
        // gains are cos(pi/4) and sin(pi/4) - mathematically equal, and one ulp
        // apart as the library computes them. The shipped width-0 wet has
        // therefore always been dual-mono to within a pan-gain ulp rather than
        // bitwise, which is why probe K has always measured it with a tolerance.
        // Measured here at 1.1e-8 against a ~0.07 signal, i.e. about -136 dB.
        //
        // What IS asserted is the meaningful form of "collapses sensibly":
        // Stereo at width 0 must be no more asymmetric than Mono Sum is at
        // width 0. A mode that picked its channel from the pan POSITION rather
        // than from panSign would send every grain to one channel here and miss
        // this by orders of magnitude - which is the whole reason srcCh follows
        // panSign.
        auto stereoW0 = renderPanned (1.0f, 0.0f);
        auto monoW0   = renderPanned (0.0f, 0.0f);

        const double asymStereoW0 = asym (stereoW0);
        const double asymMonoW0   = asym (monoW0);
        const double dW0          = maxAbsDiff (stereoW0.L, stereoW0.R);
        const double pkW0         = peakAbs (stereoW0.L);

        check ("stereo-source-width0-collapse",
               asymStereoW0 < 1.0e-4
                 && asymStereoW0 < juce::jmax (asymMonoW0 * 10.0, 1.0e-6)
                 && pkW0 > 0.0 && dW0 < pkW0 * 1.0e-5,
               juce::String ("width-0 L/R asymmetry stereo=") + juce::String (asymStereoW0, 9)
                 + " monoSum=" + juce::String (asymMonoW0, 9)
                 + " | max|wetL - wetR| = " + juce::String (dW0, 12)
                 + " vs peak " + juce::String (pkW0, 6)
                 + " (pan-gain ulp, not a mode artefact - see probe K)");
    }

    // --- Probe AU: the mono-output fold, re-derived for a stereo source -------
    //
    // The v1.7.0 review asked whether the 0.70710677f in processBlock's mono
    // branch still holds once grains can read one channel rather than the sum.
    // The answer is that the constant is about the PAN geometry and not about
    // what is read, and the mono path makes that concrete in a way worth
    // asserting: mono OUT is only reachable with mono IN (the bus layout rejects
    // stereo->mono outright), and with a mono input the capture ring holds
    // L == R, so the two source modes read identical material by construction.
    //
    // So two things are measured rather than argued:
    //   * the two modes render BITWISE identically down the mono path, i.e. the
    //     constant cannot depend on the mode because the mode cannot reach it;
    //   * the fold is still UNITY - the mono-out wet level matches the
    //     stereo-out width-0 wet level, which is what "0.7071.(L+R) -> unity for
    //     the centred dual-mono wet" means as a measurement.
    {
        auto fill = [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); };

        auto renderMonoAt = [&] (float sourceMode)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",   0.0f);
            setParam (apvts, "delayTime", 500.0f);
            setParam (apvts, "feedback",   40.0f);
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "width",       0.0f);
            setParam (apvts, "sourceMode", sourceMode);
            proc.setPlayConfigDetails (1, 1, fs, block);
            proc.prepareToPlay (fs, block);
            return renderEffectMono (proc, 3.0, fs, block, fill);
        };

        auto monoOutSum    = renderMonoAt (0.0f);
        auto monoOutStereo = renderMonoAt (1.0f);

        const double dMode = maxAbsDiff (monoOutSum, monoOutStereo);

        // Stereo reference at the same settings, for the fold's unity check.
        setBaseline (apvts);
            clearRandomisation();
        setParam (apvts, "syncMode",   0.0f);
        setParam (apvts, "delayTime", 500.0f);
        setParam (apvts, "feedback",   40.0f);
        setParam (apvts, "mix",       100.0f);
        setParam (apvts, "width",       0.0f);
        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);
        auto stereoRef = renderEffect (proc, 3.0, fs, block, fill);

        // The reference is the stereo render's TOTAL POWER, sqrt(rmsL^2 + rmsR^2),
        // and not one of its channels. That correction is the measurement this
        // probe exists to make, because getting it wrong looks exactly like a
        // broken constant: the first version of this probe compared against
        // rms(L) alone and reported +3.010 dB, which is not the fold being wrong
        // but the reference being 3 dB low by construction. At width 0 each
        // channel carries 0.7071 of the grain sum, so one channel is always
        // 3 dB under the pair.
        //
        // Total power is the right reference precisely because it is
        // fold-INDEPENDENT: it is the same number whatever constant the mono
        // branch uses, so the comparison tests the constant instead of
        // restating it. A power-preserving fold lands on it exactly; the two
        // other plausible constants do not (a bare sum would read +3.01 dB,
        // a 0.5 average -3.01 dB).
        const int    win     = (int) (1.5 * fs);
        const double monoRms = rms (monoOutSum, (int) (1.0 * fs), win);
        const double stL     = rms (stereoRef.L, (int) (1.0 * fs), win);
        const double stR     = rms (stereoRef.R, (int) (1.0 * fs), win);
        const double stPower = std::sqrt (stL * stL + stR * stR);
        const double foldDb  = (monoRms > 0.0 && stPower > 0.0)
                                 ? 20.0 * std::log10 (monoRms / stPower) : -99.0;

        // Restore the harness-wide play config before anything else runs.
        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);

        check ("mono-fold-source-invariant",
               dMode == 0.0 && std::abs (foldDb) < 0.5
                 && allFinite (monoOutSum) && allFinite (monoOutStereo),
               juce::String ("max|monoSum - stereo| = ") + juce::String (dMode, 12)
                 + " (mono out is only reachable with mono in, where L==R)"
                 + " | mono out vs stereo TOTAL POWER = " + juce::String (foldDb, 3)
                 + " dB (needs |.|<0.5 - a bare sum reads +3.01, a 0.5 average -3.01)");
    }

    // --- Probe AV: delay drift stays inside the ring -------------------------
    //
    // The defect this exists to catch is probe AH's, reached from a new
    // direction: drift MULTIPLIES the latched delay by up to 1.25, so it extends
    // the worst-case read span the same way delayScatter does - and an
    // over-reaching read does not fault, does not produce a NaN and does not
    // click. It wraps onto material the writer has already overwritten, and the
    // only symptom is that the long settings sound wrong.
    //
    // Same configuration and same assertion as probe AH, with drift at FULL
    // depth on top: D = 4000 ms, G = 4000 ms, one burst at the start, silence
    // after. A grain reads [s - gD - G, s - gD], so with gD in [3000, 5000] ms
    // the burst can only reach the output after ~3 s and the early window must
    // be silent. Under a ring that did not cover the drifted span the burst
    // leaks straight into it.
    //
    // Deliberately NOT asserted by reading maxLatchedDelay back: that constant
    // is what the engine believes, and a probe that checks the engine's own
    // belief proves nothing. The arrival TIME is the observable.
    {
        setBaseline (apvts);
            clearRandomisation();
        setParam (apvts, "syncMode",   0.0f);
        setParam (apvts, "delayTime",  ReverseDelayProcessor::kDelayTimeMaxMs);
        setParam (apvts, "grainSize",  ReverseDelayProcessor::kGrainSizeMaxMs);
        setParam (apvts, "density",     0.0f);        // overlap 2
        setParam (apvts, "feedback",    0.0f);
        setParam (apvts, "mix",       100.0f);
        setParam (apvts, "width",       0.0f);
        setParam (apvts, "delayScatter",
                  ReverseDelayProcessor::kDelayScatterMaxMs);   // both extenders at once
        setParam (apvts, "driftRate",   0.15f);       // ~6.7 s cycle: a full swing inside the render
        setParam (apvts, "driftDepth", 100.0f);
        proc.prepareToPlay (fs, block);

        const int    burstLen = (int) (1.0 * fs);
        juce::Random rng ((juce::int64) 0x5eed77);
        auto fill = [&] (int t)
        {
            return t < burstLen ? (float) (0.5 * (rng.nextDouble() * 2.0 - 1.0)) : 0.0f;
        };

        auto y = renderEffect (proc, 12.0, fs, block, fill);

        // Early window [0.5 s, 2.5 s). The shortest reachable delay here is
        // 4000.0.75 - 500 = 2500 ms, so nothing may arrive before then.
        const double earlyRms = juce::jmax (rms (y.L, (int) (0.5 * fs), (int) (2.0 * fs)),
                                            rms (y.R, (int) (0.5 * fs), (int) (2.0 * fs)));

        // Arrival window [4 s, 10 s): wide, because drift moves where the burst
        // lands. It must be somewhere in here or the probe would also pass on an
        // engine that output nothing.
        const double arrivedRms = juce::jmax (rms (y.L, (int) (4.0 * fs), (int) (6.0 * fs)),
                                              rms (y.R, (int) (4.0 * fs), (int) (6.0 * fs)));

        const double pk = juce::jmax (peakAbs (y.L), peakAbs (y.R));

        check ("drift-ring-clamp",
               earlyRms < arrivedRms * 1.0e-3 && arrivedRms > 1.0e-4 && pk < 1.0
                 && allFinite (y.L) && allFinite (y.R),
               juce::String ("earlyRms[0.5..2.5s]=") + juce::String (earlyRms, 9)
                 + " arrivedRms[4..10s]=" + juce::String (arrivedRms, 7)
                 + " ratio=" + juce::String (arrivedRms > 0.0 ? earlyRms / arrivedRms : -1.0, 9)
                 + " peak=" + juce::String (pk, 4)
                 + " | required span = " + juce::String (
                       ReverseDelayProcessor::kDelayTimeMaxMs
                         * (1.0f + ReverseDelayProcessor::kDriftMaxFraction)
                         + ReverseDelayProcessor::kDelayScatterMaxMs
                         + 2.0f * ReverseDelayProcessor::kGrainSizeMaxMs, 0)
                 + " ms, ring = " + juce::String (
                       ReverseDelayProcessor::kCaptureSeconds * 1000.0f, 0) + " ms");
    }

    // --- Probe AW: drift is live at depth, and exactly nothing at zero -------
    //
    // The mirror of probe T, for the one control in this release whose no-op is
    // reached by an early RETURN rather than by arithmetic that happens to be
    // neutral: driftMul() bails before evaluating std::sin at depth 0. That is
    // stronger than "the sine is near zero" and is asserted as such - a
    // defaults render must be bit-identical to one where drift is explicitly
    // zeroed at a non-default RATE, which a sine-that-rounds-to-zero would fail.
    {
        auto renderDrift = [&] (float depth, float rate)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",   0.0f);
            setParam (apvts, "delayTime", 500.0f);
            setParam (apvts, "feedback",   40.0f);
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "driftRate",  rate);
            setParam (apvts, "driftDepth", depth);
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 4.0, fs, block,
                                 [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });
        };

        auto off      = renderDrift (0.0f,   ReverseDelayProcessor::kDriftRateCentreHz);
        auto offFast  = renderDrift (0.0f,   4.0f);    // same no-op, wildly different rate
        auto on       = renderDrift (100.0f, 1.0f);

        const double dOff = juce::jmax (maxAbsDiff (off.L, offFast.L),
                                        maxAbsDiff (off.R, offFast.R));
        const double dOn  = diffRms (off.L, on.L, (int) (1.0 * fs), (int) (2.5 * fs));
        const double base = rms (off.L, (int) (1.0 * fs), (int) (2.5 * fs));

        check ("drift-zero-is-noop", dOff == 0.0,
               juce::String ("max|rate 0.30 - rate 4.0| at depth 0 = ")
                 + juce::String (dOff, 12)
                 + " (driftMul returns before touching std::sin)");

        // 10 % of the wet's own RMS is a low bar deliberately: what is being
        // caught is a control wired to a parameter and reaching no engine, which
        // measures exactly 0.
        check ("drift-is-live",
               base > 0.0 && dOn > base * 0.10 && allFinite (on.L) && allFinite (on.R),
               juce::String ("rms|depth100 - depth0| = ") + juce::String (dOn, 6)
                 + " against wet rms " + juce::String (base, 6)
                 + " (needs > 10 %)");
    }

    // --- Probe AX: v1.7.0's controls are block-size invariant ----------------
    //
    // Probe AQ's property, re-asserted with this release's four controls
    // engaged, and it is the reason two of them are built the way they are:
    //
    //   * duck runs its follower per SAMPLE with a coefficient computed once per
    //     block. The cheaper block-rate design - block RMS, one-pole on
    //     exp(-N/(τ.fs)), gain ramped across the block - fails THIS LINE by
    //     construction, and only this line: it passes every level, response and
    //     click probe above. It would also mean an offline bounce at 4096
    //     ducking 75 ms later than the same session monitored at 512.
    //   * driftMul derives its LFO phase from the grain's absolute spawn
    //     position rather than from a per-block accumulator, for the same
    //     reason: an accumulator advanced eight times as often lands a few ulps
    //     apart, and this assertion is exact.
    //
    // Everything on at once, including the v1.1 randomisations and the v1.6
    // motion controls, so every stream is being consumed as hard as the
    // parameters allow.
    {
        auto renderAtBlock = [&] (int blk)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "density",      60.0f);
            setParam (apvts, "feedback",     60.0f);
            setParam (apvts, "width",        60.0f);
            setParam (apvts, "mix",         100.0f);
            setParam (apvts, "jitter",       50.0f);
            setParam (apvts, "delayScatter", 80.0f);
            setParam (apvts, "sizeRandom",   40.0f);
            setParam (apvts, "gainRandom",   30.0f);
            setParam (apvts, "direction",    50.0f);
            setParam (apvts, "sourceMode",    1.0f);   // Stereo
            setParam (apvts, "duck",         80.0f);
            setParam (apvts, "driftRate",     1.5f);
            setParam (apvts, "driftDepth",   70.0f);
            proc.setPlayConfigDetails (2, 2, fs, blk);
            proc.prepareToPlay (fs, blk);
            return renderEffectStereo (proc, 3.0, fs, blk,
                                       [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); },
                                       [&] (int t) { return (float) (kRandA * whiteNoiseAt (t + 7919)); });
        };

        auto small = renderAtBlock (512);
        auto large = renderAtBlock (4096);

        const double d = juce::jmax (maxAbsDiff (small.L, large.L),
                                     maxAbsDiff (small.R, large.R));

        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);

        check ("v170-blocksize-invariance", d == 0.0,
               juce::String ("max|512-4096| = ") + juce::String (d, 12)
                 + " with duck/drift/stereo-source and every earlier"
                 + " randomisation engaged, on a DECORRELATED stereo excitation"
                 + " rms512=" + juce::String (rms (small.L, (int) (1.0 * fs), (int) (1.5 * fs)), 6));
    }

    // --- Probe AY: a non-finite input must not stick -------------------------
    //
    // The duck follower is the plugin's first piece of PERSISTENT audio state
    // that a pathological input can poison permanently, and that is a property
    // of the follower rather than of ducking: duckEnv is updated as
    // `rect + c*(duckEnv - rect)`, and once it holds a NaN every later update
    // reproduces one, for the life of the instance. The engine's other state
    // recovers on its own schedule - the capture ring ages a bad sample out
    // after one lap, and the feedback loop's isfinite guard resets the filters
    // within the pass - so this would be the one place a single bad sample is
    // forever.
    //
    // Measured against the SHIPPED engine rather than against an absolute:
    // duck 0 is the v1.0-v1.6 path, so whatever recovery it manages is the
    // standard duck 80 has to meet. A probe that only asserted "duck 80
    // recovers" could be satisfied by an engine that never went bad, and one
    // that asserted "the output is finite throughout" would fail on the ring
    // lap, which is correct behaviour.
    //
    // Render is 30 s so the 14 s ring has fully lapped past the burst before the
    // measurement window opens.
    {
        auto burstFill = [&] (int t)
        {
            // 10 ms of non-finite input at 1 s: both flavours, since inf reaches
            // NaN by a different route (inf/(inf+knee) is NaN, not 1).
            const int burstStart = (int) (1.0 * fs);
            const int burstEnd   = burstStart + (int) (0.010 * fs);

            if (t >= burstStart && t < burstEnd)
                return ((t - burstStart) % 2 == 0)
                         ? std::numeric_limits<float>::quiet_NaN()
                         : std::numeric_limits<float>::infinity();

            return (float) (kRandA * whiteNoiseAt (t));
        };

        auto renderBurst = [&] (float duckPct)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",  0.0f);
            setParam (apvts, "delayTime", 500.0f);
            setParam (apvts, "feedback",   40.0f);
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "duck",      duckPct);
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 30.0, fs, block, burstFill);
        };

        auto shipped = renderBurst (0.0f);    // the v1.0-v1.6 path
        auto ducked  = renderBurst (80.0f);

        const int tailOff = (int) (20.0 * fs), tailLen = (int) (10.0 * fs);

        auto tailOk = [&] (const StereoRender& y)
        {
            for (int i = tailOff; i < tailOff + tailLen; ++i)
                if (! std::isfinite (y.L[(size_t) i]) || ! std::isfinite (y.R[(size_t) i]))
                    return false;
            return true;
        };

        const bool   shippedOk  = tailOk (shipped);
        const bool   duckedOk   = tailOk (ducked);
        const double shippedRms = rms (shipped.L, tailOff, tailLen);
        const double duckedRms  = rms (ducked.L,  tailOff, tailLen);

        check ("nonfinite-input-does-not-stick",
               shippedOk && duckedOk && shippedRms > 1.0e-5 && duckedRms > 1.0e-5,
               juce::String ("tail[20..30s] finite: duck0=") + (shippedOk ? "yes" : "NO")
                 + " duck80=" + (duckedOk ? "yes" : "NO")
                 + " | rms duck0=" + juce::String (shippedRms, 7)
                 + " duck80=" + juce::String (duckedRms, 7)
                 + " (a 10 ms NaN/inf burst at 1 s; the 14 s ring has lapped by 20 s)");
    }

    // --- Probe AZ: v1.8.0's defaults are bitwise the v1.7.3 engine -----------
    //
    // The guarantee every release since v1.1.0 has made, asserted the only way
    // it can be: diffusion and drive at 0 must produce a render that is EXACTLY
    // equal — not close — to one made with the code paths they add stepped over.
    //
    // Both zeros are early-outs rather than arithmetic that happens to cancel,
    // which is what makes exact equality the right bar instead of a tolerance:
    //   * diffusion 0 -> `l += 0.0f * (dl - l)`, and 0.0f * finite is +0.0f, so
    //     l is unchanged bit-for-bit. (The chain still RUNS; this asserts that
    //     running it is inaudible, which is the actual claim.)
    //   * drive 0 -> driveRatio returns exactly 1.0f -> driveShape branches on
    //     `d <= 1.0f` and calls std::tanh(x). Not tanh(1.0f*x)/1.0f, which would
    //     also be exact here but would be relying on it.
    //
    // Rendered with feedback high and mix 100 so the loop — the only path either
    // control touches — dominates the output. A defaults render at mix 0 would
    // pass this probe against an engine where both controls were wired backwards.
    {
        auto renderColour = [&] (float diffusion, float drive)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",  0.0f);
            setParam (apvts, "delayTime", 400.0f);
            setParam (apvts, "feedback",   75.0f);
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "diffusion", diffusion);
            setParam (apvts, "drive",     drive);
            proc.prepareToPlay (fs, block);
            return renderEffect (proc, 6.0, fs, block,
                                 [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });
        };

        auto shipped = renderColour (0.0f, 0.0f);
        auto again   = renderColour (0.0f, 0.0f);

        // Same parameters twice is the control: if THIS differs the harness is
        // not deterministic and the comparison below means nothing.
        const double repeatD = maxAbsDiff (shipped.L, again.L);

        auto diffused = renderColour (60.0f,  0.0f);
        auto driven   = renderColour ( 0.0f, 60.0f);

        const double dDiff  = maxAbsDiff (shipped.L, diffused.L);
        const double dDrive = maxAbsDiff (shipped.L, driven.L);

        check ("v180-defaults-bit-identical",
               repeatD == 0.0 && dDiff > 1.0e-4 && dDrive > 1.0e-4,
               juce::String ("repeat|0,0 vs 0,0| = ") + juce::String (repeatD, 12)
                 + " (must be 0) | diffusion 60 moves " + juce::String (dDiff, 7)
                 + " | drive 60 moves " + juce::String (dDrive, 7)
                 + " (both must be audible, else the controls are not wired)");
    }

    // --- Probe BA: diffusion cannot raise the LINEAR loop gain ---------------
    //
    // The safety claim v1.8.0 makes INSTEAD of a measured cap, and the reason
    // diffusion needs no equivalent of kRegenMakeupMaxDb. An allpass has unit
    // magnitude at every frequency, so the mixed block's response is
    // |(1-m) + m.e^{j.phi}| <= (1-m) + m = 1 for every m and every phi. The
    // block is non-expansive, therefore it cannot open a self-oscillation path
    // that `feedback` alone would not.
    //
    // ── Per GENERATION, not per second — and that is the whole probe ─────────
    //
    // Two earlier versions of this probe failed, and both failed because they
    // measured decay in dB/SECOND. The diffused tail genuinely decays slower by
    // that measure — 4.81 dB/s against 5.30 at feedback 85 — and it does so
    // even at -52 dBFS, where the tanh is within ulps of the identity and no
    // saturation story can explain it.
    //
    // The cause is not gain, it is TIME. The allpass chain adds up to 48.6 ms of
    // group delay to the feedback return, so at a 400 ms delay each generation
    // takes ~12 % longer. Fewer generations per second is a slower decay per
    // second with the loss per generation unchanged — and self-oscillation
    // depends on the gain a signal accumulates per TRIP AROUND THE LOOP, not
    // per second. A loop losing 2.1 dB per pass never runs away however long a
    // pass takes.
    //
    // The measured ratio confirms it rather than merely being consistent with
    // it: 4.8053/5.2989 = 0.9068 against the pure period prediction
    // 400/(400+48.6) = 0.8917. The small residual is group delay being
    // frequency-dependent and below the full sum at the top of the band.
    //
    // So the assertion is on dB per generation, with the diffused loop CREDITED
    // the maximum group delay the chain can physically produce (the sum of the
    // four section lengths — a bound, not a fitted value). Crediting the
    // maximum is what makes this conservative in the right direction: if
    // diffusion were adding real energy, no amount of period credit up to the
    // physical ceiling could account for it and this line fails. Rewriting the
    // mix as something with gain — scaling the coefficient, dropping the dry
    // path, chaining the sections in series without the crossfade — moves it
    // immediately.
    //
    // Measured at -52 dBFS so the tanh cannot contribute either way. The
    // feedback-100 corner is checked separately below for the property that is
    // actually load-bearing there: it must CONVERGE and stay bounded.
    {
        auto tailDecayDb = [&] (float diffusion, float feedback, double amp)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",  0.0f);
            setParam (apvts, "delayTime", 400.0f);
            setParam (apvts, "feedback",  feedback);
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "diffusion", diffusion);
            proc.prepareToPlay (fs, block);

            auto y = renderEffect (proc, 20.0, fs, block, [&] (int t)
            {
                return t < (int) (2.0 * fs) ? (float) (amp * whiteNoiseAt (t)) : 0.0f;
            });

            const int aOff = (int) (5.0 * fs), bOff = (int) (15.0 * fs);
            const int win  = (int) (3.0 * fs);
            const double ra = juce::jmax (1.0e-30, rms (y.L, aOff, win));
            const double rb = juce::jmax (1.0e-30, rms (y.L, bOff, win));

            double peak = 0.0;
            for (size_t i = 0; i < y.L.size(); ++i)
                peak = juce::jmax (peak, (double) std::abs (y.L[i]));

            const bool finite = std::all_of (y.L.begin(), y.L.end(),
                                             [] (float v) { return std::isfinite (v); });

            struct R { double decayDb, peak; bool finite; };
            return R { 20.0 * std::log10 (rb / ra) / 10.0, peak, finite };
        };

        // (a) LINEAR regime — the non-expansiveness assertion proper.
        // -52 dBFS excitation: the loop peaks around 1e-3, where tanh(x) and x
        // agree to ~1e-7 relative, so the saturator cannot be the thing moving
        // the decay rate.
        const double quiet   = std::pow (10.0, -52.0 / 20.0);
        const double delaySec = 0.400;

        const auto linPlain = tailDecayDb (  0.0f, 85.0f, quiet);
        const auto linDiff  = tailDecayDb (100.0f, 85.0f, quiet);

        // The maximum group delay the chain can produce: the sum of its four
        // section lengths. Read from the plugin's own constant rather than
        // re-typed, so a voicing change to the lengths cannot leave this probe
        // crediting a period the engine no longer has
        // (pattern_test_fixture_mirrors_drift_silently).
        double gdMaxSec = 0.0;
        for (float ms : ReverseDelayProcessor::kDiffusionAllpassMs)
            gdMaxSec += (double) ms * 0.001;

        // dB lost per trip around the loop. Decay is negative, so "more
        // negative" is more loss; the diffused loop must lose AT LEAST as much
        // per generation as the plain one even after being credited the largest
        // period the chain can physically add.
        const double plainPerGen = linPlain.decayDb * delaySec;
        const double diffPerGen  = linDiff.decayDb  * (delaySec + gdMaxSec);

        // For the report line: how much of the dB/s slowdown the period alone
        // predicts, against what was measured.
        const double measuredRatio  = linDiff.decayDb / linPlain.decayDb;
        const double predictedRatio = delaySec / (delaySec + gdMaxSec);

        // (b) feedback 100 — convergence and boundedness, the property that is
        // actually load-bearing at the corner.
        const auto hotPlain = tailDecayDb (  0.0f, 100.0f, kRandA);
        const auto hotDiff  = tailDecayDb (100.0f, 100.0f, kRandA);

        const bool converges = hotPlain.decayDb < 0.0 && hotDiff.decayDb < 0.0;
        const bool bounded   = hotPlain.peak < 1.8 && hotDiff.peak < 1.8
                                 && hotPlain.finite && hotDiff.finite;

        check ("diffusion-is-non-expansive",
               linPlain.finite && linDiff.finite
                 && diffPerGen <= plainPerGen && converges && bounded,
               juce::String ("LINEAR (-52 dBFS, fb 85) dB per GENERATION:"
                             " diffusion 0 = ")
                 + juce::String (plainPerGen, 4)
                 + ", diffusion 100 = " + juce::String (diffPerGen, 4)
                 + " (crediting the full " + juce::String (gdMaxSec * 1000.0, 1)
                 + " ms allpass group delay; diffused must lose at least as much)"
                 + " | dB/s ratio measured " + juce::String (measuredRatio, 4)
                 + " vs period prediction " + juce::String (predictedRatio, 4)
                 + " || fb 100: decay 0 = " + juce::String (hotPlain.decayDb, 4)
                 + ", 100 = " + juce::String (hotDiff.decayDb, 4)
                 + " dB/s (both must be < 0) peak = "
                 + juce::String (hotDiff.peak, 4));
    }

    // --- Probe BB: drive does not move the decay rate ------------------------
    //
    // The property that separates drive from regenMakeup, asserted rather than
    // asserted-in-a-comment. tanh(d.x)/d has small-signal gain exactly 1 for
    // every d, so the loop's LOW-LEVEL decay — which is what sets how long a
    // tail lasts — must be unchanged across the whole knob. regenMakeup fails
    // this line by construction at any non-zero setting, which is precisely why
    // it needed a measured cap and this does not.
    //
    // Measured in the LATE tail, where the loop content has decayed into the
    // small-signal region the claim is about. The early tail is expected to
    // differ — that is the saturation, i.e. the feature.
    {
        auto decayDbPerSec = [&] (float drivePct)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "syncMode",  0.0f);
            setParam (apvts, "delayTime", 400.0f);
            setParam (apvts, "feedback",   85.0f);
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "drive",     drivePct);
            proc.prepareToPlay (fs, block);

            auto y = renderEffect (proc, 25.0, fs, block, [&] (int t)
            {
                return t < (int) (1.0 * fs) ? (float) (kRandA * whiteNoiseAt (t)) : 0.0f;
            });

            // Two windows deep in the tail, 8 s apart.
            const int aOff = (int) (12.0 * fs), bOff = (int) (20.0 * fs);
            const int win  = (int) (2.0 * fs);
            const double ra = juce::jmax (1.0e-12, rms (y.L, aOff, win));
            const double rb = juce::jmax (1.0e-12, rms (y.L, bOff, win));
            return 20.0 * std::log10 (rb / ra) / 8.0;
        };

        const double d0   = decayDbPerSec (0.0f);
        const double d50  = decayDbPerSec (50.0f);
        const double d100 = decayDbPerSec (100.0f);

        const double spread = juce::jmax (std::abs (d50 - d0), std::abs (d100 - d0));

        check ("drive-preserves-decay-rate",
               spread < 0.25,
               juce::String ("decay dB/s: drive 0 = ") + juce::String (d0, 4)
                 + ", 50 = " + juce::String (d50, 4)
                 + ", 100 = " + juce::String (d100, 4)
                 + " | spread = " + juce::String (spread, 4)
                 + " dB/s (must stay under 0.25 — the small-signal gain of"
                 + " tanh(d.x)/d is 1 at every d)");
    }

    // --- Probe BC: v1.8.0's controls are block-size invariant ----------------
    //
    // Probes AQ and AX again, with COLOUR engaged. Neither new control has any
    // block-rate state — the allpass chain advances per sample and the drive is
    // a memoryless function — so this should hold trivially. It is asserted
    // anyway because "should hold trivially" is what was said about the delay
    // read latch (v1.0.1) and the RNG streams (v1.1.0), and both were wrong.
    //
    // The allpass chain is the specific risk: its ring index is per-sample state
    // carried across block boundaries, which is exactly the shape of the bug
    // pattern_grain_read_before_capture_write_blocksize describes.
    {
        auto renderAtBlock = [&] (int blk)
        {
            setBaseline (apvts);
            clearRandomisation();
            setParam (apvts, "density",      60.0f);
            setParam (apvts, "feedback",     70.0f);
            setParam (apvts, "mix",         100.0f);
            setParam (apvts, "jitter",       50.0f);
            setParam (apvts, "delayScatter", 80.0f);
            setParam (apvts, "direction",    50.0f);
            setParam (apvts, "duck",         80.0f);
            setParam (apvts, "driftDepth",   70.0f);
            setParam (apvts, "diffusion",    85.0f);
            setParam (apvts, "drive",        75.0f);
            proc.setPlayConfigDetails (2, 2, fs, blk);
            proc.prepareToPlay (fs, blk);
            return renderEffectStereo (proc, 4.0, fs, blk,
                                       [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); },
                                       [&] (int t) { return (float) (kRandA * whiteNoiseAt (t + 7919)); });
        };

        auto small = renderAtBlock (512);
        auto large = renderAtBlock (4096);

        const double d = juce::jmax (maxAbsDiff (small.L, large.L),
                                     maxAbsDiff (small.R, large.R));

        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);

        check ("v180-blocksize-invariance", d == 0.0,
               juce::String ("max|512-4096| = ") + juce::String (d, 12)
                 + " with diffusion 85 / drive 75 and every earlier control"
                 + " engaged, rms512="
                 + juce::String (rms (small.L, (int) (1.0 * fs), (int) (1.5 * fs)), 6));
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
            // v1.6.0 (B4): the MOTION trio, all pinned at 0. Unlike `tilt` two
            // lines up, zero really IS the neutral value for all three — which
            // makes this the one column block in this struct where the obvious
            // reflex is correct, and worth saying so right next to the one where
            // it is not.
            float  freeze, direction, regen;
            // v1.7.0 (B4 #4-#6): SOURCE / DUCK / DRIFT. Three zeros and one that
            // is NOT — driftRate is pinned at its DEFAULT 0.30 Hz, because a 0
            // written here would be clamped up to kDriftRateMinHz by the
            // parameter's own range and the comparison would fail against a
            // table that reads as if it were correct. Same class of trap as
            // `tilt` four lines up, in a shape that is new: there the wrong
            // value is out of range in spirit, here it is out of range in fact.
            float  source, duck, driftRate, driftDepth;
            // v1.8.0 (B4 #7-#8): COLOUR, both pinned at 0. Back to the v1.6.0
            // situation — the reflex is correct here — but these two columns are
            // load-bearing for a reason the MOTION trio's were not. Diffusion and
            // Drive are the first parameters this plugin has added that would
            // genuinely IMPROVE the factory presets, so the pressure is not to
            // write the wrong neutral, it is to write a deliberate non-neutral
            // and re-voice eight shipped sounds. These zeros are what fails if
            // someone does (pattern_activating_dead_param_default_timbre).
            float  diffusion, drive;
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
            { "Reverse Bloom",    0, 6,  500, 200, 53.3f,  40, 100,  8000, 60, 40, 0.5f, 0, 0, 0, 0, 0, 0, 0.30f, 0, 0, 0, 10.0 },
            { "Guitar Swell",     0, 6,  700, 300, 47.5f,  45, 120,  6500, 55, 55, 0.5f, 0, 0, 0, 0, 0, 0, 0.30f, 0, 0, 0, 10.0 },
            { "Vocal Halo",       0, 6,  380, 180, 65.0f,  30, 300,  7000, 70, 25, 0.5f, 0, 0, 0, 0, 0, 0, 0.30f, 0, 0, 0, 10.0 },
            { "Slow Wash",        0, 6, 1400, 450, 18.3f,  65,  80,  5000, 85, 50, 0.5f, 0, 0, 0, 0, 0, 0, 0.30f, 0, 0, 0, 10.0 },
            { "Tight Smear",      0, 6,  180,  70, 88.3f,  35, 150, 11000, 35, 45, 0.5f, 0, 0, 0, 0, 0, 0, 0.30f, 0, 0, 0, 10.0 },
            { "Dark Cavern",      0, 6,  850, 320, 59.2f,  70, 220,  1800, 75, 55, 0.5f, 0, 0, 0, 0, 0, 0, 0.30f, 0, 0, 0, 10.0 },
            { "Near-Infinite",    0, 6,  900, 350, 65.0f, 100, 180,  2500, 80, 50, 0.5f, 0, 0, 0, 0, 0, 0, 0.30f, 0, 0, 0, 30.0 },
            { "Rhythmic Reverse", 1, 4,  500, 120, 76.7f,  50, 140,  9000, 50, 45, 0.5f, 0, 0, 0, 0, 0, 0, 0.30f, 0, 0, 0, 10.0 },
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
            // v1.6.0 (B4). freeze is a BOOL, so its raw value is 0.0 or 1.0 and
            // kTolChoice is the right tolerance. It is also the one of the three
            // that the render check below could NOT catch: a preset shipping
            // frozen still produces a wash — it is simply held — so washRms and
            // peak both look healthy and only this comparison reports it.
            cmp ("freeze",       e.freeze,    kTolChoice);
            cmp ("direction",    e.direction, kTolPct);
            cmp ("regenMakeup",  e.regen,     kTolPct);
            // v1.7.0 (B4 #4-#6). sourceMode is a CHOICE, so kTolChoice; the
            // other three are floats. driftRate's tolerance is its own 0.01 Hz
            // step, tight enough that the clamped-to-minimum failure mode
            // (0.30 -> 0.02) misses by 28 steps rather than sneaking through.
            cmp ("sourceMode",   e.source,     kTolChoice);
            cmp ("duck",         e.duck,       kTolPct);
            cmp ("driftRate",    e.driftRate,  0.01f);
            cmp ("driftDepth",   e.driftDepth, kTolPct);
            // v1.8.0 (B4 #7-#8). Both linear 0-100 % floats on a 0.1 step, so
            // kTolPct is one step — any deliberate re-voicing of a shipped preset
            // fails this by at least 100x rather than drifting under the bar.
            cmp ("diffusion",    e.diffusion,  kTolPct);
            cmp ("drive",        e.drive,      kTolPct);

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

    // --- Probe BA: drift with scatter OFF is block-size invariant (CR-01) -----
    //
    // The gap probes AV, AW and AX left between them, and the whole finding is in
    // which branch of PluginProcessor's pass bound each of them selected:
    //
    //   * AV (drift-ring-clamp)        sets delayScatter = kDelayScatterMaxMs
    //   * AX (v170-blocksize-invariance) sets delayScatter = 80 ms
    //     -> both take `scatterSamples > 0`, i.e. the CONSERVATIVE grainDelayFloor
    //        bound, which is precisely the branch that was already safe.
    //   * AW (drift-is-live)           runs scatter-free, but at block = 512 with
    //     delayTime = 500 ms, where passLen is two orders of magnitude below the
    //     smallest reachable gD.
    //
    // So no probe varied the block size with drift ON and scatter OFF, and that is
    // the one combination in which v1.7.0's driftMul could pull a latched delay
    // (down to 0.75·D) below a pass bound still widened to D. A grain spawning
    // late in such a pass latched readAbs AHEAD of the capture write head — which
    // step 6 has not written yet — and read a full 14 s ring lap of stale material.
    //
    // The settings are chosen so passLen actually straddles gD's negative
    // excursion rather than merely being legal: at 48 kHz, delayTime 95 ms gives
    // D = 4560, gD ranges over [3420, 5700] at depth 100, and a 4096-sample block
    // makes passLen = 4096 > 3420. direction 60 matters because a forward grain
    // reads t − gD at EVERY sample of its life, so the corruption is the whole
    // grain rather than just its head.
    //
    // Asserted EXACT, like every other invariance probe here: this is arithmetic
    // on the same inputs, not a tolerance question.
    {
        auto renderAtBlock = [&] (int blk)
        {
            setBaseline (apvts);
            clearRandomisation();
            clearWindow();
            setParam (apvts, "delayTime",     95.0f);   // D = 4560 at 48 kHz
            setParam (apvts, "grainSize",    120.0f);
            setParam (apvts, "density",       70.0f);
            setParam (apvts, "feedback",      50.0f);
            setParam (apvts, "width",         60.0f);
            setParam (apvts, "mix",          100.0f);
            setParam (apvts, "direction",     60.0f);
            setParam (apvts, "driftRate",      2.0f);
            setParam (apvts, "driftDepth",   100.0f);   // gD in [0.75·D, 1.25·D]
            setParam (apvts, "delayScatter",   0.0f);   // THE branch under test
            proc.setPlayConfigDetails (2, 2, fs, blk);
            proc.prepareToPlay (fs, blk);
            return renderEffect (proc, 3.0, fs, blk,
                                 [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });
        };

        auto small = renderAtBlock (512);
        auto large = renderAtBlock (4096);

        const double d = juce::jmax (maxAbsDiff (small.L, large.L),
                                     maxAbsDiff (small.R, large.R));
        const double r = rms (small.L, (int) (1.0 * fs), (int) (1.5 * fs));

        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);

        check ("drift-noscatter-blocksize-invariance",
               d == 0.0 && r > 1.0e-4
                 && allFinite (small.L) && allFinite (large.L),
               juce::String ("max|512-4096| = ") + juce::String (d, 12)
                 + " at driftDepth 100 / delayScatter 0 / delayTime 95 ms"
                 + " (the pass-bound branch AV and AX never take)"
                 + " rms512=" + juce::String (r, 6));
    }

    // --- Probe BB: Freeze restored from a session must not be silent (CR-02) --
    //
    // Probe AP engages the hold 3 s after load, which was the right timing for the
    // failure it was built to catch (advancing the write head without writing goes
    // silent) and is the one timing at which this failure cannot appear: it needs
    // capture.getTotalWritten() to be SMALL when the latch fires.
    //
    // A session saved with Freeze engaged is ordinary use — it is a performance
    // control — and the ring is not part of that saved state. prepareToPlay
    // allocates and clears it, so the parameter says "frozen" while the buffer
    // holds nothing. v1.7.1 latched jlimit(1, .., 0) == 1 on the first block,
    // pushLooped(1) copied the previous (zero) slot, every later sample copied the
    // zero it had just written, and freezeEngaged was already true so the latch
    // never re-armed: the wet path was exactly zero for the life of the session.
    // The dry path passed through, which is why it presents as "the plugin stopped
    // working" rather than as a freeze.
    //
    // This is the ONLY ordering that reproduces it: set freeze BEFORE
    // prepareToPlay, exactly as setStateInformation-then-prepare does on session
    // load. Rendering wet-only (mix 100) is what makes the assertion meaningful —
    // any dry leak would mask a dead wash.
    {
        setBaseline (apvts);
        clearRandomisation();
        clearWindow();
        setParam (apvts, "delayTime", 500.0f);
        setParam (apvts, "grainSize", 200.0f);
        setParam (apvts, "density",    60.0f);
        setParam (apvts, "feedback",   40.0f);
        setParam (apvts, "width",      60.0f);
        setParam (apvts, "mix",       100.0f);   // wet only — dry would mask silence
        setParam (apvts, "freeze",      1.0f);   // BEFORE prepare: the session-load order

        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);

        // Freeze stays engaged throughout — no callback touches it. The hold must
        // arm itself once a grain of material exists.
        auto y = renderEffect (proc, 5.0, fs, block,
                               [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });

        const double rEarly = rms (y.L, (int) (0.5 * fs), (int) (1.0 * fs));
        const double rLate  = rms (y.L, (int) (3.5 * fs), (int) (1.0 * fs));
        const double pk     = juce::jmax (peakAbs (y.L), peakAbs (y.R));

        // v1.7.1 measured exactly 0.0 for both windows. The bound is a real wash
        // level rather than "> 0" so a single leaked sample cannot satisfy it.
        check ("freeze-restored-from-session-is-not-silent",
               rLate > 1.0e-3 && rEarly > 1.0e-4 && pk < 1.0
                 && allFinite (y.L) && allFinite (y.R),
               juce::String ("wet rms early[0.5..1.5s]=") + juce::String (rEarly, 6)
                 + " late[3.5..4.5s]=" + juce::String (rLate, 6)
                 + " peak=" + juce::String (pk, 4)
                 + " (freeze=1 set BEFORE prepareToPlay; v1.7.1 rendered 0.000000)");
    }

    // --- Probe BC: a hold can never latch into a TONE (CR-02, second arm) -----
    //
    // The sibling of BB, reached from the other end. Engaging Freeze a few hundred
    // samples after load — well before a grain has been captured — used to latch a
    // loop that short: at block 512 the hold became a 512-sample loop, i.e. a
    // 94 Hz pitched tone at 48 kHz rather than the wash the control claims.
    // v1.7.2 refuses to arm below one grain, so the hold waits and then holds real
    // material.
    //
    // NOT measured by crest factor, which is what probe AP uses and what this
    // probe was first written with. Crest catches a PULSE TRAIN (AP's stopped-write-
    // head failure reads far above the live wash) but a short loop is a periodic
    // WAVEFORM, whose crest is if anything lower than a cloud's: the first draft of
    // this probe read crest 3.62 against a live 3.20 on the very code it was meant
    // to reject, and passed.
    //
    // The discriminator that actually separates them is LAGGED AUTOCORRELATION. A
    // ring looping with period L makes every grain read material that repeats at L,
    // so the output correlates near-1 at lag L. The assertion is therefore the
    // general form of the invariant — no strong periodicity at ANY lag shorter than
    // one grain — rather than a test against the one lag the old code happened to
    // pick, which would be pinned to the harness's block size.
    {
        // Normalised autocorrelation of x at `lag`, over [off, off+len).
        auto autocorrAt = [] (const std::vector<float>& x, int lag, int off, int len)
        {
            const int hi = juce::jmin ((int) x.size() - lag, off + len);
            if (hi - off < 2) return 0.0;

            double num = 0.0, ea = 0.0, eb = 0.0;
            for (int i = off; i < hi; ++i)
            {
                const double a = (double) x[(size_t) i];
                const double b = (double) x[(size_t) (i + lag)];
                num += a * b;
                ea  += a * a;
                eb  += b * b;
            }
            const double den = std::sqrt (ea * eb);
            return den > 1.0e-20 ? num / den : 0.0;
        };

        // Freeze at 256 samples — inside the first block at every tested size, and
        // two orders of magnitude below G.
        //
        // The excitation runs for the WHOLE render, unlike probe AP's 3-s burst.
        // That is not incidental: the fix defers the latch until a grain of
        // material exists, so the ring must still be receiving live input across
        // that window. Cutting the input at freezeAt starves the deferred latch —
        // it then captures ~5 ms of noise and 195 ms of silence, and the resulting
        // sparse output reads crest 48 against a live 3.2, which measures the
        // probe's excitation and not the plugin. Once armed the hold ignores the
        // input anyway, so leaving it on cannot flatter the result.
        const int freezeAt = 256;

        setBaseline (apvts);
        clearRandomisation();
        clearWindow();
        setParam (apvts, "delayTime", 500.0f);
        setParam (apvts, "grainSize", 200.0f);
        setParam (apvts, "density",    60.0f);
        setParam (apvts, "feedback",   40.0f);
        setParam (apvts, "width",      60.0f);
        setParam (apvts, "mix",       100.0f);
        setParam (apvts, "freeze",     0.0f);
        proc.prepareToPlay (fs, block);

        auto y = renderEffect (proc, 8.0, fs, block,
                               [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); },
                               [&] (int pos, int)
                               {
                                   if (pos >= freezeAt) setParam (apvts, "freeze", 1.0f);
                               });

        const double rHold = rms (y.L, (int) (5.0 * fs), (int) (2.0 * fs));

        // Scan every lag a degenerate latch could have produced: from 64 samples up
        // to one grain. The spawn interval (G/overlap = 1714 samples here) lives
        // inside this range and is legitimate structure, so the bound has to sit
        // above whatever the correctly-armed hold reads there — measured, not
        // guessed, and reported either way so a future shift is visible.
        const int    gSamples = (int) (0.200 * fs);
        const int    winOff   = (int) (5.0 * fs);
        const int    winLen   = (int) (2.0 * fs);
        double worstCorr = 0.0;
        int    worstLag  = 0;

        for (int lag = 64; lag < gSamples; lag += 16)
        {
            const double c = std::abs (autocorrAt (y.L, lag, winOff, winLen));
            if (c > worstCorr) { worstCorr = c; worstLag = lag; }
        }

        check ("freeze-early-engage-is-not-a-tone",
               rHold > 1.0e-4 && worstCorr < 0.80
                 && allFinite (y.L) && allFinite (y.R),
               juce::String ("hold rms[5..7s]=") + juce::String (rHold, 6)
                 + " worst |autocorr| = " + juce::String (worstCorr, 4)
                 + " @lag " + juce::String (worstLag)
                 + " over lags 64..G (needs < 0.80)"
                 + " | freeze engaged at " + juce::String (freezeAt)
                 + " samples, G=" + juce::String (gSamples));
    }

    // --- Probe BD: a SWEPT cutoff's block-size divergence is bounded (WR-03) --
    //
    // The gap in probes O, W2, AQ, AX and BA: every one of them sets its parameters
    // before prepareToPlay and never moves them, and setCurrentAndTargetValue
    // starts the smoothers AT target. So all five assert bit-identity for STATIC
    // parameters, while NOTES.md and the v1.7.0 CHANGELOG stated the invariant
    // flatly. This probe is the automated case they never covered.
    //
    // Bit-identity is UNREACHABLE here, and NOT because of the smoother: the
    // processor reads pHighCut->load() once per processBlock, which is all a host
    // without sample-accurate automation offers. A 512-sample block samples the
    // automation curve 8x more finely than a 4096-sample one, so the two renders
    // are asked to follow genuinely different target sequences. Asserting == 0
    // would be asserting something false.
    //
    // ── What this probe established about the WR-03 fix, measured both ways ────
    //
    // The fix (32-sample coefficient grid) was landed on the argument that the old
    // skip(numSamples) update defeated the documented 20 ms smoothing contract at
    // large buffers — a 20 ms ramp is 960 samples at 48 kHz, so a 4096-sample block
    // skipped the whole ramp in ONE step and held one coefficient set for the block.
    // That mechanism is real and the fix removes it.
    //
    // Its measurable consequence, however, is much smaller than that reasoning
    // suggests, and this probe is the record of it. Rendered both ways:
    //
    //   512-vs-4096 divergence:  6.7 % of wet RMS with the grid, 7.2 % with the
    //                            block-rate update. The divergence is dominated by
    //                            the per-block PARAMETER sampling above, not by the
    //                            coefficient grid, so the fix barely moves it.
    //   maxAbsStep vs parked:    0.98x with the block-rate update — i.e. probe C's
    //                            click detector cannot see the coefficient jump
    //                            either, because broadband noise through the grain
    //                            engine dominates the first difference.
    //
    // So the fix is a fidelity-to-contract correction, not an audible-artefact fix,
    // and the NOTES.md invariance claim needed correcting regardless of it. Both
    // numbers are reported rather than asserted tightly; the bound below is a
    // regression guard against a future change making the divergence wild, not a
    // discriminator between the two implementations.
    {
        auto renderSweep = [&] (int blk)
        {
            setBaseline (apvts);
            clearRandomisation();
            clearWindow();
            setParam (apvts, "delayTime", 300.0f);
            setParam (apvts, "grainSize", 150.0f);
            setParam (apvts, "density",    60.0f);
            setParam (apvts, "feedback",   50.0f);
            setParam (apvts, "width",      50.0f);
            setParam (apvts, "mix",       100.0f);
            setParam (apvts, "lowCut",     20.0f);
            setParam (apvts, "highCut", 18000.0f);
            proc.setPlayConfigDetails (2, 2, fs, blk);
            proc.prepareToPlay (fs, blk);

            // A fast sweep — 18 kHz down to 800 Hz over 2 s — because a slow one
            // hides a coarse update grid. Driven through setParam, i.e. the same
            // path a DAW's automation lane uses.
            return renderEffect (proc, 3.0, fs, blk,
                                 [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); },
                                 [&] (int pos, int)
                                 {
                                     const double u = juce::jlimit (0.0, 1.0,
                                                        (double) pos / (2.0 * fs));
                                     setParam (apvts, "highCut",
                                               (float) (18000.0 - u * 17200.0));
                                 });
        };

        auto small = renderSweep (512);
        auto large = renderSweep (4096);

        const double d  = juce::jmax (maxAbsDiff (small.L, large.L),
                                      maxAbsDiff (small.R, large.R));
        const double r  = rms (small.L, (int) (0.5 * fs), (int) (2.0 * fs));
        const double rel = r > 0.0 ? d / r : 99.0;

        // Largest sample-to-sample step in the 4096-sample render, against the same
        // engine's step with the cutoff PARKED. Reported as a click guard. It does
        // not discriminate the two coefficient implementations (the block-rate one
        // reads 0.98x here) — see the note above; it is kept because a future change
        // that genuinely does splice a coefficient jump would show up in it.
        setBaseline (apvts);
        clearRandomisation();
        clearWindow();
        setParam (apvts, "delayTime", 300.0f);
        setParam (apvts, "grainSize", 150.0f);
        setParam (apvts, "density",    60.0f);
        setParam (apvts, "feedback",   50.0f);
        setParam (apvts, "width",      50.0f);
        setParam (apvts, "mix",       100.0f);
        setParam (apvts, "lowCut",     20.0f);
        setParam (apvts, "highCut",  9000.0f);   // parked mid-sweep
        proc.setPlayConfigDetails (2, 2, fs, 4096);
        proc.prepareToPlay (fs, 4096);
        auto steady = renderEffect (proc, 3.0, fs, 4096,
                                    [&] (int t) { return (float) (kRandA * whiteNoiseAt (t)); });

        const double stepSwept = maxAbsStep (large.L);
        const double stepRef   = maxAbsStep (steady.L);
        const double stepRatio = stepRef > 0.0 ? stepSwept / stepRef : 99.0;

        proc.setPlayConfigDetails (2, 2, fs, block);
        proc.prepareToPlay (fs, block);

        check ("cutoff-sweep-bounded-and-clickfree",
               rel < 0.40 && stepRatio < 2.0 && r > 1.0e-4
                 && allFinite (small.L) && allFinite (large.L)
                 && allFinite (steady.L),
               juce::String ("512-vs-4096 divergence = ") + juce::String (d, 6)
                 + " = " + juce::String (rel * 100.0, 1) + " % of wet rms "
                 + juce::String (r, 6) + " (bound 40 %; inherent ~7 % from"
                 + " per-block parameter delivery, NOT the coefficient grid)"
                 + " | maxStep swept=" + juce::String (stepSwept, 6)
                 + " parked-ref=" + juce::String (stepRef, 6)
                 + " ratio=" + juce::String (stepRatio, 3) + "x (bound 2.0x)");
    }

    std::printf ("%s (%d failure%s)\n",
                 failures == 0 ? "ALL PROBES PASSED" : "PROBES FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
