/*
  ==============================================================================

    O-simpleSampler render-harness — Stage 2 DSP correctness gate.

    Instantiates OSimpleSamplerAudioProcessor directly and verifies the sampler
    engine's audible acceptance criteria without a DAW — converting the
    "manual-listen" ROADMAP checks into automated assertions. Human DAW testing is
    deferred to post-GUI, so THIS harness is the load-bearing Stage-2 gate.

      1. makes-sound             — defaults + held note 48 -> non-trivial RMS, finite.
      2. repitch-tuning          — pitchMode=Repitch: autocorr f0(note 48) ~= 131 Hz;
                                   note 60 ~= 2x note 48; note 36 ~= 0.5x note 48
                                   (octave scaling, key-tracked resample). (FUNC-02)
      3. stretch-pitch-tracks    — pitchMode=Stretch: autocorr f0(note 60) ~= 2x
                                   f0(note 48) — pitch tracks the key in Stretch too.
      4. stretch-time-INDEPENDENCE — one-shot full region. Repitch: dur(36) ~= 2x
                                   dur(48) (read rate 0.5x => twice as long; pitch+
                                   time COUPLED). Stretch: dur(36) ~= dur(48) (timePos
                                   1x regardless of pitch; duration PRESERVED). The
                                   ratio contrast is the pitch/time-independence proof.
      5. loop-seam-continuity    — loopMode=forward held past the loop end, crossfade
                                   0/10/100 ms -> continuity high, no large seam jump;
                                   repeated for ping-pong. (DSP loop seam)
      6. region-end-declick      — one-shot ending mid-body: the max |sample delta| in
                                   the final ramp is bounded vs a hard cut (raised-
                                   cosine ramp, not a hard edge). (2.1 declick warning)
      7. vintage-clean-at-zero   — vintage=0 vs vintage=100 differ (macro wired) and
                                   vintage=100 is spectrally FLATTER/noisier while
                                   vintage=0 stays tonal/clean. (DSP-04) NOTE: the
                                   amplitude grid is NOT observable on the output —
                                   the resonant filter sits AFTER Vintage and smears
                                   the 8-bit grid into continuous floats — so spectral
                                   flatness + an aligned-difference probe replace the
                                   "smallest-delta grid" check (see report).
      8. aa/uptranspose-stable   — note 84 (+3 oct, 8x source rate) in BOTH Repitch and
                                   Stretch -> finite, peak bounded (AA stability).
      9. stress-bounded          — 5-note chord, Stretch, vintage high, filter low +
                                   high resonance, loop on -> finite, peak bounded, no
                                   NaN; tail silent after note-offs (no stuck voice).

    A fresh processor's prepareToPlay seeds the piano root to 48 (probed f0 ~=
    131 Hz), so the tests key off note 48 = recorded pitch. ALL pitch checks use
    autocorrPitchHz (a time-domain period probe) — the Stretch grain comb confounds
    spectral bins (project memory).

    Exit 0 iff all checks pass. Off by default; -DOUARICON_BUILD_TESTS=ON.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "PluginProcessor.h"

#include <vector>
#include <utility>
#include <cmath>
#include <cstdio>

namespace PID = OSimpleSampler::ParamIDs;
using juce::String;

//==============================================================================
static void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float real)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (real));
    else
        std::printf ("  !! unknown param id '%s'\n", id);
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

static double peakAbsWin (const std::vector<float>& x, int off, int len)
{
    double p = 0.0;
    const int lo = juce::jmax (0, off);
    const int hi = juce::jmin ((int) x.size(), off + len);
    for (int i = lo; i < hi; ++i) p = juce::jmax (p, (double) std::abs (x[(size_t) i]));
    return p;
}

static bool allFinite (const std::vector<float>& x)
{
    for (float s : x) if (! std::isfinite (s)) return false;
    return true;
}

// Max |x[i] - x[i-1]| over [off, off+len). A click / hard cut shows up as a big
// single-sample jump; band-limited content keeps consecutive deltas tiny.
static double maxAbsDeltaWin (const std::vector<float>& x, int off, int len)
{
    double m = 0.0;
    const int lo = juce::jmax (1, off);
    const int hi = juce::jmin ((int) x.size(), off + len);
    for (int i = lo; i < hi; ++i)
        m = juce::jmax (m, (double) std::abs (x[(size_t) i] - x[(size_t) (i - 1)]));
    return m;
}

// Short-window RMS-envelope continuity: fraction of `frameMs` frames whose RMS
// exceeds `ratio` x the mean frame RMS. A continuous (looping) note -> most frames
// near the mean (-> ~1.0); a seam DROPOUT leaves near-silent frames (-> low).
// Amplitude-independent. Ported verbatim from O-simpleGrain.
static double continuityFraction (const std::vector<float>& x, int off, int len,
                                  double fs, double frameMs, double ratio)
{
    const int fn = juce::jmax (1, (int) (frameMs * 0.001 * fs));
    const int nFrames = len / fn;
    if (nFrames <= 0 || off < 0 || off + len > (int) x.size()) return 0.0;

    std::vector<double> env ((size_t) nFrames, 0.0);
    double mean = 0.0;
    for (int f = 0; f < nFrames; ++f)
    {
        double acc = 0.0;
        for (int n = 0; n < fn; ++n) { const double s = x[(size_t) (off + f * fn + n)]; acc += s * s; }
        env[(size_t) f] = std::sqrt (acc / (double) fn);
        mean += env[(size_t) f];
    }
    mean /= (double) nFrames;
    if (mean <= 0.0) return 0.0;

    int above = 0;
    for (double e : env) if (e > ratio * mean) ++above;
    return (double) above / (double) nFrames;
}

// Fundamental frequency via normalized autocorrelation over [off,off+len),
// searched in [fLo,fHi] Hz. The waveform repeats at its period regardless of which
// partial is loudest, so it tracks the true fundamental (and halves when the pitch
// doubles) — and, crucially, it is immune to the Stretch grain-rate comb that
// confounds spectral bins (project memory). Ported verbatim from O-simpleGrain.
static double autocorrPitchHz (const std::vector<float>& x, int off, int len,
                               double fs, double fLo, double fHi, double minCorr = 0.3)
{
    if (off < 0 || off + len > (int) x.size()) return 0.0;
    const int lagMin = juce::jmax (1, (int) (fs / fHi));
    const int lagMax = (int) (fs / fLo);
    double e0 = 0.0;
    for (int n = 0; n < len; ++n) { const double s = x[(size_t) (off + n)]; e0 += s * s; }
    if (e0 < 1.0e-9) return 0.0;

    double bestCorr = 0.0; int bestLag = -1;
    for (int lag = lagMin; lag <= lagMax; ++lag)
    {
        double acc = 0.0, eL = 0.0;
        for (int n = 0; n + lag < len; ++n)
        {
            const double a = x[(size_t) (off + n)];
            const double b = x[(size_t) (off + n + lag)];
            acc += a * b;
            eL  += b * b;
        }
        const double c = acc / (std::sqrt (e0 * eL) + 1.0e-12);
        if (c > bestCorr) { bestCorr = c; bestLag = lag; }
    }
    return (bestLag > 0 && bestCorr >= minCorr) ? fs / (double) bestLag : 0.0;
}

// Active duration (seconds): time from sample 0 until the short-frame RMS envelope
// last exceeds a fraction of its own peak. Because Repitch scales the WHOLE time
// axis (incl. the natural decay) by 1/rate while Stretch holds it at 1x, this
// relative-threshold measure yields proportional durations regardless of whether
// the one-shot terminates at the region end or the source decays first.
static double activeDurationSec (const std::vector<float>& x, double fs,
                                 double relThresh = 0.08, double absFloor = 1.0e-4)
{
    const int fn = juce::jmax (1, (int) (0.005 * fs));
    const int nFrames = (int) x.size() / fn;
    if (nFrames <= 0) return 0.0;

    std::vector<double> env ((size_t) nFrames, 0.0);
    double peak = 0.0;
    for (int f = 0; f < nFrames; ++f)
    {
        double a = 0.0;
        for (int n = 0; n < fn; ++n) { const double s = x[(size_t) (f * fn + n)]; a += s * s; }
        env[(size_t) f] = std::sqrt (a / (double) fn);
        peak = juce::jmax (peak, env[(size_t) f]);
    }
    if (peak <= absFloor) return 0.0;

    const double th = juce::jmax (absFloor, relThresh * peak);
    int last = -1;
    for (int f = 0; f < nFrames; ++f) if (env[(size_t) f] > th) last = f;
    return (double) (last + 1) * (double) fn / fs;
}

static int lastNonSilent (const std::vector<float>& x, double thresh = 1.0e-4)
{
    for (int i = (int) x.size() - 1; i >= 0; --i)
        if (std::abs (x[(size_t) i]) > thresh) return i;
    return -1;
}

//==============================================================================
// Magnitude spectrum of a Hann-windowed segment via juce::dsp::FFT (order 14 =
// 16384). Ported (trimmed) from O-simpleGrain.
struct Spectrum
{
    std::vector<float> mag;     // size N/2+1
    double             fs = 0;
    int                fftSize = 0;

    double binHz() const { return fs / (double) fftSize; }

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

    // Spectral flatness (geo mean / arith mean) over a band. 1.0 = white/noisy,
    // -> 0 = tonal/peaky. Quantization noise + S&H aliasing fill the inter-harmonic
    // valleys -> flatness rises.
    double flatness (double loHz, double hiHz) const
    {
        double logSum = 0.0, sum = 0.0; int n = 0;
        for (size_t k = 0; k < mag.size(); ++k)
        {
            const double f = (double) k * binHz();
            if (f < loHz || f > hiHz) continue;
            const double m = (double) mag[k] + 1.0e-12;
            logSum += std::log (m);
            sum    += m;
            ++n;
        }
        if (n == 0 || sum <= 0) return 0.0;
        const double geo = std::exp (logSum / n);
        const double ar  = sum / n;
        return geo / ar;
    }
};

static Spectrum analyze (const std::vector<float>& x, int off, double fs, int order = 14)
{
    const int N = 1 << order;                     // default 16384
    juce::dsp::FFT fft (order);

    std::vector<float> buf ((size_t) (2 * N), 0.0f);   // interleaved-real for FFT
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
// Pump the message loop so AsyncUpdater-driven work (sourceSample decode + zero-
// cross snap) actually runs in this console app.
static void pumpMessages (int ms = 60)
{
    if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
        mm->runDispatchLoopUntil (ms);
}

// Render `seconds` of mono output (channel 0). Each render first does a clean
// releaseResources()/prepareToPlay() so voices, source and zero-cross snaps are
// fresh and DETERMINISTIC (the sampler has no per-voice RNG — identical params
// give bit-identical output, which the vintage aligned-difference probe relies on).
// All notes-on at sample 0; held to the end unless `releaseAtSec >= 0`.
static std::vector<float> render (OSimpleSamplerAudioProcessor& proc,
                                  const std::vector<int>& notes, double seconds, double fs,
                                  int velocity = 100, double releaseAtSec = -1.0)
{
    proc.releaseResources();
    proc.prepareToPlay (fs, 512);   // clean voices + reload source + fresh snaps
    pumpMessages (40);              // flush any AsyncUpdater queued by param changes

    const int block = 512;
    const int total = (int) (seconds * fs);
    const int releaseSample = (releaseAtSec >= 0.0) ? (int) (releaseAtSec * fs) : -1;

    juce::AudioBuffer<float> buf (2, block);
    std::vector<float> out;
    out.reserve ((size_t) total);

    int pos = 0;
    bool sentOn = false, sentOff = false;
    while (pos < total)
    {
        buf.clear();
        juce::MidiBuffer midi;
        if (! sentOn)
        {
            for (int nn : notes)
                midi.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) velocity), 0);
            sentOn = true;
        }
        if (releaseSample >= 0 && ! sentOff && pos + block > releaseSample)
        {
            const int off = juce::jlimit (0, block - 1, releaseSample - pos);
            for (int nn : notes)
                midi.addEvent (juce::MidiMessage::noteOff (1, nn), off);
            sentOff = true;
        }

        proc.processBlock (buf, midi);

        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
        pos += block;
    }
    return out;
}

// Reset all 21 params to a clean, analysis-friendly baseline: factory-ish, full
// sustain so a held window is steady, Repitch one-shot, no loop/vintage/filter.
// rootKey is pinned to 48 (the piano recorded root) so pitch tests are
// deterministic regardless of the seed-on-prepare timing.
static void resetDefaults (juce::AudioProcessorValueTreeState& a)
{
    setParam (a, PID::sourceSample,        0.0f);   // piano
    setParam (a, PID::regionStart,         0.0f);
    setParam (a, PID::regionEnd,         100.0f);
    setParam (a, PID::loopMode,            0.0f);   // off (one-shot)
    setParam (a, PID::loopStart,           0.0f);
    setParam (a, PID::loopEnd,           100.0f);
    setParam (a, PID::loopCrossfade,      10.0f);
    setParam (a, PID::reverse,             0.0f);
    setParam (a, PID::rootKey,            48.0f);   // piano recorded root (deterministic)
    setParam (a, PID::pitchMode,           0.0f);   // Repitch
    setParam (a, PID::tune,                0.0f);
    setParam (a, PID::fine,                0.0f);
    setParam (a, PID::vintage,             0.0f);   // bit-clean bypass
    setParam (a, PID::filterCutoff,    20000.0f);   // open
    setParam (a, PID::filterResonance,     0.0f);
    setParam (a, PID::ampAttack,         0.005f);
    setParam (a, PID::ampDecay,            0.3f);
    setParam (a, PID::ampSustain,          1.0f);   // full sustain (stored 0..1)
    setParam (a, PID::ampRelease,          0.2f);
    setParam (a, PID::velToAmp,           50.0f);
    setParam (a, PID::outputLevel,         0.0f);
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // message manager for async decode

    const double fs = 44100.0;
    OSimpleSamplerAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, fs, 512);
    proc.prepareToPlay (fs, 512);               // seeds the piano root to 48
    auto& apvts = proc.getAPVTS();

    const int rootPiano = 48;                   // piano recorded root (probed f0 ~131 Hz)

    int failures = 0;
    auto check = [&] (const char* name, bool ok, const String& detail)
    {
        std::printf ("  [%s] %-26s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    };

    std::printf ("O-simpleSampler render-harness — fs=%.0f, pianoRoot=%d (f0~131 Hz)\n", fs, rootPiano);

    // --- 1: makes sound -------------------------------------------------------
    {
        resetDefaults (apvts);
        auto y = render (proc, { rootPiano }, 1.0, fs);
        const int o = (int) (0.05 * fs), l = (int) (0.25 * fs);
        const double r = rms (y, o, l);
        check ("makes-sound", r > 0.005 && allFinite (y),
               String ("rms=") + String (r, 4));
    }

    // --- 2: Repitch tuning — f0 at root ~131 Hz, octave scaling ---------------
    {
        resetDefaults (apvts);
        setParam (apvts, PID::pitchMode, 0.0f);          // Repitch
        const int o = (int) (0.06 * fs), l = (int) (0.25 * fs);
        auto y48 = render (proc, { 48 }, 0.6, fs);
        auto y60 = render (proc, { 60 }, 0.6, fs);
        auto y36 = render (proc, { 36 }, 0.6, fs);
        const double f48 = autocorrPitchHz (y48, o, l, fs, 100.0, 175.0);
        const double f60 = autocorrPitchHz (y60, o, l, fs, 200.0, 350.0);
        const double f36 = autocorrPitchHz (y36, o, l, fs,  50.0,  90.0);
        const bool   root = f48 > 0.0 && std::abs (f48 - 131.0) < 0.08 * 131.0;
        const bool   up   = f48 > 0.0 && f60 > 0.0 && (f60 / f48) > 1.85 && (f60 / f48) < 2.15;
        const bool   down = f48 > 0.0 && f36 > 0.0 && (f36 / f48) > 0.43 && (f36 / f48) < 0.57;
        check ("repitch-tuning", root && up && down,
               String ("f48=") + String (f48, 1) + " f60=" + String (f60, 1)
                 + " f36=" + String (f36, 1)
                 + " (f60/f48=" + String (f48 > 0 ? f60 / f48 : 0.0, 3)
                 + " f36/f48=" + String (f48 > 0 ? f36 / f48 : 0.0, 3) + ")");
    }

    // --- 3: Stretch — pitch tracks the key (f0 doubles per octave) ------------
    {
        resetDefaults (apvts);
        setParam (apvts, PID::pitchMode, 1.0f);          // Stretch
        const int o = (int) (0.08 * fs), l = (int) (0.25 * fs);
        auto y48 = render (proc, { 48 }, 0.6, fs);
        auto y60 = render (proc, { 60 }, 0.6, fs);
        const double f48 = autocorrPitchHz (y48, o, l, fs, 100.0, 175.0);
        const double f60 = autocorrPitchHz (y60, o, l, fs, 200.0, 350.0);
        const bool   tracks = f48 > 0.0 && f60 > 0.0 && (f60 / f48) > 1.8 && (f60 / f48) < 2.2;
        check ("stretch-pitch-tracks", tracks,
               String ("f48=") + String (f48, 1) + " f60=" + String (f60, 1)
                 + " ratio=" + String (f48 > 0 ? f60 / f48 : 0.0, 3));
    }

    // --- 4: Stretch time-INDEPENDENCE (the headline) -------------------------
    // One-shot, full region. Repitch couples pitch+time (dur(36) ~= 2x dur(48));
    // Stretch preserves duration (dur(36) ~= dur(48)). The ratio contrast proves
    // pitch/time independence. Windows are sized adaptively to the source length so
    // even the slowest case (Repitch note 36 = 2x source) terminates in-window.
    {
        resetDefaults (apvts);
        setParam (apvts, PID::loopMode,    0.0f);        // one-shot
        setParam (apvts, PID::regionStart, 0.0f);
        setParam (apvts, PID::regionEnd, 100.0f);

        setParam (apvts, PID::pitchMode, 1.0f);          // probe source length in Stretch (1x)
        auto probe = render (proc, { 48 }, 16.0, fs);
        const double dSrc = activeDurationSec (probe, fs);
        const double win  = juce::jlimit (3.0, 28.0, 2.6 * dSrc + 1.0);

        setParam (apvts, PID::pitchMode, 0.0f);          // Repitch
        const double rp48 = activeDurationSec (render (proc, { 48 }, win, fs), fs);
        const double rp36 = activeDurationSec (render (proc, { 36 }, win, fs), fs);

        setParam (apvts, PID::pitchMode, 1.0f);          // Stretch
        const double st48 = activeDurationSec (render (proc, { 48 }, win, fs), fs);
        const double st36 = activeDurationSec (render (proc, { 36 }, win, fs), fs);

        const double rpRatio = (rp48 > 0.0) ? rp36 / rp48 : 0.0;
        const double stRatio = (st48 > 0.0) ? st36 / st48 : 0.0;
        check ("stretch-time-independence",
               rp48 > 0.05 && st48 > 0.05
                 && rpRatio > 1.6 && stRatio < 1.3 && rpRatio > stRatio * 1.4,
               String ("dSrc=") + String (dSrc, 2) + "s win=" + String (win, 1) + "s | Repitch "
                 + String (rp36, 2) + "/" + String (rp48, 2) + "=" + String (rpRatio, 2)
                 + " | Stretch " + String (st36, 2) + "/" + String (st48, 2) + "=" + String (stRatio, 2));
    }

    // --- 5: loop-seam continuity (forward + ping-pong, crossfade 0/10/100 ms) -
    {
        auto runLoop = [&] (int loopModeVal, float xfadeMs) -> std::pair<double, double>
        {
            resetDefaults (apvts);
            setParam (apvts, PID::pitchMode,     0.0f);          // Repitch
            setParam (apvts, PID::loopMode,      (float) loopModeVal);
            setParam (apvts, PID::loopStart,    20.0f);          // short body loop -> several wraps
            setParam (apvts, PID::loopEnd,      45.0f);
            setParam (apvts, PID::loopCrossfade, xfadeMs);
            auto y = render (proc, { 48 }, 2.5, fs);
            const int o = (int) (0.4 * fs), l = (int) (1.8 * fs);
            return { continuityFraction (y, o, l, fs, 5.0, 0.25), maxAbsDeltaWin (y, o, l) };
        };

        bool fwdOk = true; double fwdContMin = 1.0, fwdMdMax = 0.0;
        for (float xf : { 0.0f, 10.0f, 100.0f })
        {
            auto r = runLoop (1, xf);
            fwdContMin = juce::jmin (fwdContMin, r.first);
            fwdMdMax   = juce::jmax (fwdMdMax,  r.second);
            if (! (r.first > 0.85 && r.second < 0.5)) fwdOk = false;
        }
        auto pp = runLoop (2, 10.0f);
        const bool ppOk = pp.first > 0.85 && pp.second < 0.5;
        check ("loop-seam-continuity", fwdOk && ppOk,
               String ("fwd contMin=") + String (fwdContMin, 3) + " seamMaxDelta=" + String (fwdMdMax, 3)
                 + " | pingpong cont=" + String (pp.first, 3) + " seamMaxDelta=" + String (pp.second, 3));
    }

    // --- 6: region-end declick (raised-cosine ramp, not a hard cut) -----------
    // End the region mid-body (30 %) so content is still loud at the region end —
    // a hard cut there would jump by ~the content level; the raised-cosine ramp
    // keeps the final-window |delta| far below that.
    {
        resetDefaults (apvts);
        setParam (apvts, PID::pitchMode,   0.0f);        // Repitch one-shot
        setParam (apvts, PID::loopMode,    0.0f);
        setParam (apvts, PID::regionStart, 0.0f);
        setParam (apvts, PID::regionEnd,  30.0f);
        auto y = render (proc, { 48 }, 6.0, fs);
        const int L       = lastNonSilent (y, 1.0e-4);
        const int rampWin = (int) (0.006 * fs);
        bool   ok = false; double md = 0.0, contentLvl = 0.0;
        if (L > 3 * rampWin)
        {
            md         = maxAbsDeltaWin (y, L - rampWin, rampWin + 1);
            contentLvl = peakAbsWin (y, L - 2 * rampWin, rampWin);   // loud content just before ramp
            ok = allFinite (y) && contentLvl > 0.02 && md < 0.5 * contentLvl;
        }
        check ("region-end-declick", ok,
               String ("L=") + String (L) + " endMaxDelta=" + String (md, 4)
                 + " contentLevel=" + String (contentLvl, 4)
                 + " (declick: endMaxDelta < 0.5*contentLevel; a hard cut ~= contentLevel)");
    }

    // --- 7: Vintage clean-at-zero (DSP-04) -----------------------------------
    // The amplitude grid is NOT observable on the output (the resonant filter sits
    // AFTER Vintage and smears the 8-bit grid into continuous floats). Instead:
    //   (a) vintage=100 differs substantially from vintage=0 (aligned diff) — the
    //       macro is wired AND vintage=0 is the genuine bypass end; and
    //   (b) vintage=100 is spectrally FLATTER/noisier (quant noise + S&H aliasing
    //       fill the inter-harmonic valleys) while vintage=0 stays tonal/clean.
    {
        resetDefaults (apvts);
        setParam (apvts, PID::pitchMode, 0.0f);
        const int o = (int) (0.10 * fs), l = (int) (0.40 * fs);

        setParam (apvts, PID::vintage,   0.0f);
        auto yClean = render (proc, { 48 }, 1.0, fs);
        setParam (apvts, PID::vintage, 100.0f);
        auto yCrush = render (proc, { 48 }, 1.0, fs);

        double diffSq = 0.0, refSq = 0.0;
        for (int i = o; i < o + l && i < (int) yClean.size() && i < (int) yCrush.size(); ++i)
        {
            const double d = (double) yCrush[(size_t) i] - (double) yClean[(size_t) i];
            const double r = (double) yClean[(size_t) i];
            diffSq += d * d; refSq += r * r;
        }
        const double relDiff = std::sqrt (diffSq / juce::jmax (1.0e-12, refSq));

        const auto sClean = analyze (yClean, o, fs);
        const auto sCrush = analyze (yCrush, o, fs);
        const double flatClean = sClean.flatness (1000.0, 10000.0);
        const double flatCrush = sCrush.flatness (1000.0, 10000.0);

        check ("vintage-clean-at-zero",
               allFinite (yClean) && allFinite (yCrush)
                 && relDiff > 0.05 && flatCrush > flatClean * 1.2,
               String ("relDiff(v100 vs v0)=") + String (relDiff, 3)
                 + " flatClean=" + String (flatClean, 4) + " flatCrush=" + String (flatCrush, 4)
                 + " (grid smeared by post-Vintage filter -> flatness is the observable)");
    }

    // --- 8: anti-alias / up-transpose stability (note 84 = +3 oct, 8x rate) ---
    {
        const int o = (int) (0.02 * fs), l = (int) (0.08 * fs);

        resetDefaults (apvts);
        setParam (apvts, PID::pitchMode, 0.0f);          // Repitch
        auto yR = render (proc, { 84 }, 1.0, fs);

        resetDefaults (apvts);
        setParam (apvts, PID::pitchMode, 1.0f);          // Stretch
        auto yS = render (proc, { 84 }, 1.0, fs);

        const double pkR = peakAbs (yR), pkS = peakAbs (yS);
        const double rR  = rms (yR, o, l), rS = rms (yS, o, l);
        check ("aa-uptranspose-stable",
               allFinite (yR) && allFinite (yS) && pkR < 2.0 && pkS < 2.0
                 && rR > 0.0003 && rS > 0.0003,
               String ("Repitch peak=") + String (pkR, 3) + " rms=" + String (rR, 4)
                 + " | Stretch peak=" + String (pkS, 3) + " rms=" + String (rS, 4));
    }

    // --- 9: stress — chord + Stretch + vintage + low cutoff/high res + loop ---
    // Bounded (no NaN/blowup) and the tail goes silent after the note-offs (proxy
    // for "no stuck voice" — there is no public active-voice accessor).
    {
        resetDefaults (apvts);
        setParam (apvts, PID::pitchMode,        1.0f);   // Stretch
        setParam (apvts, PID::vintage,         80.0f);
        setParam (apvts, PID::filterCutoff,   300.0f);   // low
        setParam (apvts, PID::filterResonance, 85.0f);   // high Q
        setParam (apvts, PID::loopMode,         1.0f);   // forward loop
        setParam (apvts, PID::loopStart,       20.0f);
        setParam (apvts, PID::loopEnd,         55.0f);
        auto y = render (proc, { 36, 43, 48, 55, 60 }, 1.6, fs, 100, 0.7);  // note-off @0.7s
        const double pk   = peakAbs (y);
        const double held = rms (y, (int) (0.20 * fs), (int) (0.30 * fs));
        const double tail = rms (y, (int) (1.20 * fs), (int) (0.40 * fs));
        check ("stress-bounded",
               allFinite (y) && pk < 20.0 && held > 0.001 && tail < 0.02,
               String ("peak=") + String (pk, 3) + " heldRms=" + String (held, 4)
                 + " tailRms=" + String (tail, 4) + " (bounded, no NaN, tail silent after note-offs)");
    }

    proc.releaseResources();

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
