/*
  ==============================================================================

    O-simpleGrain render-harness — Stage 2 DSP correctness gate.

    Instantiates OSimpleGrainAudioProcessor directly and verifies the granular
    engine's audible acceptance criteria without a DAW — converting most of the
    "manual-listen" ROADMAP checks into automated assertions:

      1. makes-sound        — default params + held MIDI note -> non-trivial RMS,
                              finite output, grains actually spawn.
      2. density->continuity— low density (overlap<1) leaves audible gaps; high
                              density (overlap>1) fuses into a continuous cloud
                              (gap fraction collapses). (FUNC-01 / DSP-02)
      3. pitch-tracks-MIDI  — an octave up (note 60 -> 72) scales the spectral
                              centroid ~2x (key-tracked resample). (FUNC-02)
      4. window-rect-clicks — a rectangular window injects more top-octave energy
                              (edge clicks) than Hann at identical settings.
                              (DSP-03)
      5. freeze-sustains    — Freeze on, held note -> finite, bounded, non-silent
                              frozen pad (playhead pinned). (FUNC-03 / QUAL-01)
      6. scatter-async      — scatter 0% (synchronous) is spectrally peakier than
                              scatter 100% (asynchronous -> flatter/noisier).
                              (DSP-05)
      7. stress-bounded     — 5-note chord, max density, max grain, full spray,
                              high scatter -> finite, peak bounded, and the global
                              active-grain count never exceeds 192. (PERF-01/02)
      8. uptranspose-stable — grainPitch +24 st (4x up) + high pitch spray on a
                              band-limited source stays finite + bounded (the
                              anti-aliasing stability half of DSP-08).
      9. velToDensity-depth — velToDensity is a graded depth, not a switch: depth
                              off -> grain count is velocity-independent; depth full
                              -> a hard note spawns a denser cloud than a soft one
                              (guards the 0..100 -> 0..1 scaling).

    Exit 0 iff all checks pass. Off by default; -DOUARICON_BUILD_TESTS=ON.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "PluginProcessor.h"

#include <vector>
#include <cmath>
#include <cstdio>

namespace PID = OSimpleGrain::ParamIDs;

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

// Short-window RMS-envelope continuity: fraction of `frameMs` frames whose RMS
// exceeds `ratio` x the mean frame RMS. A continuous overlap-add cloud -> most
// frames near the mean (-> ~1.0); sparse separated grains -> many near-silent
// frames between bursts (-> low). Amplitude-independent (normalized to the
// signal's own mean), unlike an absolute-threshold gap count.
static double continuityFraction (const std::vector<float>& x, int off, int len,
                                  double fs, double frameMs, double ratio)
{
    const int fn = juce::jmax (1, (int) (frameMs * 0.001 * fs));
    const int nFrames = len / fn;
    if (nFrames <= 0) return 0.0;

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
// searched in [fLo,fHi] Hz. Robust pitch estimate for a (quasi-)harmonic signal
// — the waveform repeats at its period regardless of which partial is loudest,
// so it tracks the true fundamental (and halves when the pitch doubles). Returns
// 0 if no lag clears `minCorr`.
static double autocorrPitchHz (const std::vector<float>& x, int off, int len,
                               double fs, double fLo, double fHi, double minCorr = 0.3)
{
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

//==============================================================================
// Magnitude spectrum of a Hann-windowed segment via juce::dsp::FFT (order 14 =
// 16384). Returns mag[0..N/2]; bin k -> frequency k*fs/N.
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

    // Frequency of the strongest magnitude bin within [loHz, hiHz].
    double dominantPeakHz (double loHz, double hiHz) const
    {
        double best = 0.0; int bestK = -1;
        for (size_t k = 1; k < mag.size(); ++k)
        {
            const double f = (double) k * binHz();
            if (f < loHz || f > hiHz) continue;
            if (mag[k] > best) { best = mag[k]; bestK = (int) k; }
        }
        return bestK < 0 ? 0.0 : (double) bestK * binHz();
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

    // Spectral flatness (geo mean / arith mean) over a band. 1.0 = white/noisy,
    // -> 0 = tonal/peaky.
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

// Best frequency-ratio shift aligning spectrum B to A, searched on a
// log-frequency grid over [f0,f1] within the ratio window [ratioLo,ratioHi].
// Comb-immune by construction: a grain-rate comb sits at identical absolute
// frequencies in both notes (a shift-of-1 / ratio-1 correlation peak), which
// the search window excludes — only the TRANSPOSED source content correlates
// inside [ratioLo,ratioHi]. Returns {bestRatio, normalizedCorrelation}.
struct ShiftResult { double ratio = 0.0; double corr = 0.0; };

static ShiftResult octaveShift (const Spectrum& A, const Spectrum& B,
                                double f0, double f1, int binsPerOct,
                                double ratioLo, double ratioHi)
{
    const int nOct = (int) std::ceil (std::log2 (f1 / f0));
    const int n = nOct * binsPerOct;

    auto logGrid = [&] (const Spectrum& s)
    {
        std::vector<double> L ((size_t) n, 0.0);
        for (int i = 0; i < n; ++i)
        {
            const double f = f0 * std::pow (2.0, (double) i / binsPerOct);
            const double kf = f / s.binHz();
            const int    k0 = (int) std::floor (kf);
            double m = 0.0;
            if (k0 >= 0 && k0 + 1 < (int) s.mag.size())
            {
                const double frac = kf - k0;
                m = (1.0 - frac) * s.mag[(size_t) k0] + frac * s.mag[(size_t) (k0 + 1)];
            }
            L[(size_t) i] = std::log (m + 1.0e-9);
        }
        // zero-mean for normalized correlation
        double mean = 0.0; for (double v : L) mean += v; mean /= (double) n;
        for (double& v : L) v -= mean;
        return L;
    };

    const auto La = logGrid (A);
    const auto Lb = logGrid (B);

    double na = 0.0, nb = 0.0;
    for (int i = 0; i < n; ++i) { na += La[(size_t) i] * La[(size_t) i]; nb += Lb[(size_t) i] * Lb[(size_t) i]; }
    const double denom = std::sqrt (na * nb) + 1.0e-12;

    const int sLo = (int) std::round (std::log2 (ratioLo) * binsPerOct);
    const int sHi = (int) std::round (std::log2 (ratioHi) * binsPerOct);

    ShiftResult best;
    for (int s = sLo; s <= sHi; ++s)
    {
        double acc = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const int j = i + s;                 // B is higher -> align B[i+s] with A[i]
            if (j < 0 || j >= n) continue;
            acc += La[(size_t) i] * Lb[(size_t) j];
        }
        const double c = acc / denom;
        if (c > best.corr) { best.corr = c; best.ratio = std::pow (2.0, (double) s / binsPerOct); }
    }
    return best;
}

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
// Render `seconds` of mono output (channel 0). All notes-on at sample 0. By default
// held to the end (no note-off) so the analysis window sits in steady sustain; pass
// `releaseAtSec >= 0` to send a note-off at that time (for envelope-release / ADSR-
// off grain-drain tests). `maxGrains` (out) receives the peak active-grain count.
static std::vector<float> render (OSimpleGrainAudioProcessor& proc,
                                  const std::vector<int>& notes, double seconds, double fs,
                                  int* maxGrains = nullptr, int velocity = 100,
                                  double releaseAtSec = -1.0)
{
    const int block = 512;
    const int total = (int) (seconds * fs);
    const int releaseSample = (releaseAtSec >= 0.0) ? (int) (releaseAtSec * fs) : -1;

    juce::AudioBuffer<float> buf (2, block);
    std::vector<float> out;
    out.reserve ((size_t) total);

    int pos = 0;
    bool sentOn = false, sentOff = false;
    int peak = 0;
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
        // Note-off the block the release point falls in (at the in-block offset).
        if (releaseSample >= 0 && ! sentOff && pos + block > releaseSample)
        {
            const int off = juce::jlimit (0, block - 1, releaseSample - pos);
            for (int nn : notes)
                midi.addEvent (juce::MidiMessage::noteOff (1, nn), off);
            sentOff = true;
        }

        proc.processBlock (buf, midi);
        peak = juce::jmax (peak, proc.getActiveGrainCount());

        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
        pos += block;
    }
    if (maxGrains != nullptr) *maxGrains = peak;
    return out;
}

// Pump the message loop so AsyncUpdater-driven source decode (sourceSample
// change) actually runs in this console app, then re-prepare is not needed
// (decode publishes via atomic swap). Returns after the dispatch settles.
static void pumpMessages (int ms = 120)
{
    if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
        mm->runDispatchLoopUntil (ms);
}

// Reset all 18 params to a clean, analysis-friendly baseline: factory-ish, but
// a fast amp envelope at full sustain so the [0.4s,0.75s] window is steady.
static void resetDefaults (juce::AudioProcessorValueTreeState& a)
{
    setParam (a, PID::sourceSample,  0.0f);   // fire
    setParam (a, PID::grainSize,    30.0f);
    setParam (a, PID::density,      40.0f);
    setParam (a, PID::position,     50.0f);
    setParam (a, PID::scan,          0.0f);
    setParam (a, PID::freeze,        0.0f);
    setParam (a, PID::windowShape,   4.0f);   // Hann
    setParam (a, PID::pitchSpray,    0.0f);
    setParam (a, PID::positionSpray, 0.0f);
    setParam (a, PID::scatter,       0.0f);
    setParam (a, PID::grainPitch,    0.0f);
    setParam (a, PID::panSpray,      0.0f);
    setParam (a, PID::velToDensity,  0.0f);
    setParam (a, PID::adsrEnabled,   1.0f);   // ADSR on = legacy/default behaviour
    setParam (a, PID::ampAttack,     0.01f);
    setParam (a, PID::ampDecay,      0.05f);
    setParam (a, PID::ampSustain,    1.0f);   // full sustain -> steady analysis
    setParam (a, PID::ampRelease,    0.1f);
    setParam (a, PID::outputLevel,   0.0f);
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // message manager for async decode

    const double fs = 44100.0;
    OSimpleGrainAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, fs, 512);
    proc.prepareToPlay (fs, 512);
    auto& apvts = proc.getAPVTS();

    const int root = OSimpleGrainAudioProcessor::kRootNote;   // 60 / C3
    const int aOff = (int) (0.40 * fs);                       // steady window [0.40s, 0.75s]
    const int aLen = (int) (0.35 * fs);

    int failures = 0;
    auto check = [&] (const char* name, bool ok, const juce::String& detail)
    {
        std::printf ("  [%s] %-22s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    };

    std::printf ("O-simpleGrain render-harness — fs=%.0f, root=%d (C3)\n", fs, root);

    // --- 1: makes sound + grains spawn ---------------------------------------
    // Wide position spray over the (sparse) fire source so grains reliably pick
    // up energy regardless of where the resting playhead sits.
    {
        resetDefaults (apvts);
        setParam (apvts, PID::density,      120.0f);
        setParam (apvts, PID::grainSize,     50.0f);
        setParam (apvts, PID::positionSpray, 70.0f);
        int peakGrains = 0;
        auto y = render (proc, { root }, 1.0, fs, &peakGrains);
        const double r = rms (y, aOff, aLen);
        check ("makes-sound", r > 0.005 && allFinite (y) && peakGrains > 0,
               juce::String ("rms=") + juce::String (r, 4)
                 + " peakGrains=" + juce::String (peakGrains));
    }

    // --- 2: density -> overlap continuity (envelope continuity rises) --------
    // Tonal "piano" source has a smooth sustain, so envelope continuity is not
    // confounded by source-amplitude bursts: a continuous cloud -> smooth high
    // envelope (~1.0); sparse separated grains -> bursty (low). Only density
    // differs between the two renders.
    {
        resetDefaults (apvts);
        setParam (apvts, PID::sourceSample,  3.0f);    // piano (tonal, smooth sustain)
        pumpMessages();
        setParam (apvts, PID::grainSize,     30.0f);
        setParam (apvts, PID::positionSpray, 20.0f);
        setParam (apvts, PID::density,        6.0f);   // overlap 0.18 -> gaps
        auto yLow = render (proc, { root }, 1.0, fs);

        setParam (apvts, PID::density,      160.0f);   // overlap 4.8 -> continuous
        auto yHigh = render (proc, { root }, 1.0, fs);

        // ratio 0.25 -> detect near-silent gaps (not half-mean dips): a
        // continuous cloud rarely drops near silence (-> high), sparse grains
        // leave many near-silent inter-grain frames (-> low).
        const double contLo = continuityFraction (yLow,  aOff, aLen, fs, 5.0, 0.25);
        const double contHi = continuityFraction (yHigh, aOff, aLen, fs, 5.0, 0.25);
        check ("density->continuity",
               contHi > 0.85 && contHi > contLo + 0.30 && allFinite (yHigh),
               juce::String ("contLowDensity=") + juce::String (contLo, 3)
                 + " contHighDensity=" + juce::String (contHi, 3));

        setParam (apvts, PID::sourceSample, 0.0f);     // restore fire
        pumpMessages();
    }

    // --- 3: pitch tracks MIDI (key-tracked resample doubles per octave) ------
    // SINGLE-GRAIN probe — the only fully comb-free measurement: at density 2 a
    // single 150 ms grain plays alone (next grain is 500 ms away), so there is
    // NO grain-rate comb. The grain reads the source at rate = 2^((note-root)/12),
    // so its spectral centroid scales directly with the read rate. Measure the
    // first grain's centroid at three octave-spaced notes; each octave ~doubles
    // it. Tonal piano source, synchronous, no spray.
    {
        resetDefaults (apvts);
        setParam (apvts, PID::sourceSample,  3.0f);   // piano
        pumpMessages();
        setParam (apvts, PID::density,        2.0f);   // one grain at a time
        setParam (apvts, PID::grainSize,    150.0f);
        setParam (apvts, PID::scatter,        0.0f);
        setParam (apvts, PID::position,      15.0f);

        // First grain spawns at note-on (sample 0). Detect its fundamental by
        // autocorrelation over the grain body (~10..135 ms) — comb-free. The
        // source piano is recorded at C3 (130.81 Hz), and root=note 60 plays it
        // at the recorded pitch, so each MIDI note N should sound at
        // 130.81 * 2^((N-60)/12). Bracket the autocorr search tightly per note
        // (±20%) so it locks the correct octave, then assert the measured f0 is
        // within ±7% of expected — verifying key-tracked resample AND the root.
        const int gOff = (int) (0.010 * fs);
        const int gLen = (int) (0.125 * fs);
        const double srcF0 = 130.81;   // piano.wav recorded fundamental (C3)
        auto noteF0 = [&] (int note)
        {
            const double exp = srcF0 * std::pow (2.0, (note - root) / 12.0);
            auto y = render (proc, { note }, 0.40, fs);
            const double meas = autocorrPitchHz (y, gOff, gLen, fs, exp * 0.8, exp * 1.25);
            return std::pair<double,double> { meas, exp };
        };
        const auto c2 = noteF0 (root - 12);   // expect ~65 Hz
        const auto c3 = noteF0 (root);        // expect ~131 Hz
        const auto c4 = noteF0 (root + 12);   // expect ~262 Hz
        auto near = [] (std::pair<double,double> m) { return m.first > 0 && std::abs (m.first - m.second) < 0.07 * m.second; };
        std::printf ("       (pitch probe: C2=%.1f/%.1f  C3=%.1f/%.1f  C4=%.1f/%.1f Hz meas/exp)\n",
                     c2.first, c2.second, c3.first, c3.second, c4.first, c4.second);
        check ("pitch-tracks-MIDI", near (c2) && near (c3) && near (c4),
               juce::String ("C2=") + juce::String (c2.first, 1)
                 + " C3=" + juce::String (c3.first, 1)
                 + " C4=" + juce::String (c4.first, 1) + " Hz (each within 7% of 2^((N-60)/12))");

        setParam (apvts, PID::sourceSample, 0.0f);   // restore fire
        pumpMessages();
    }

    // --- 4: rectangular window injects more top-octave energy than Hann ------
    // Use the band-limited "voice" source so >12 kHz is dominated by edge clicks.
    {
        resetDefaults (apvts);
        setParam (apvts, PID::sourceSample, 1.0f);   // voice (formant, rolls off high)
        pumpMessages();                              // run the async decode/hot-swap
        setParam (apvts, PID::grainSize, 18.0f);     // frequent grain edges
        setParam (apvts, PID::density,   70.0f);
        setParam (apvts, PID::windowShape, 0.0f);    // rectangular
        auto yRect = render (proc, { root }, 1.0, fs);

        setParam (apvts, PID::windowShape, 4.0f);    // Hann
        auto yHann = render (proc, { root }, 1.0, fs);

        auto sRect = analyze (yRect, aOff, fs);
        auto sHann = analyze (yHann, aOff, fs);
        const double hfRect = sRect.bandEnergy (12000.0, 21000.0);
        const double hfHann = sHann.bandEnergy (12000.0, 21000.0);
        const double ratio  = hfHann > 0 ? hfRect / hfHann : 0.0;
        check ("window-rect-clicks", hfRect > hfHann * 1.5 && allFinite (yRect),
               juce::String ("hfRect=") + juce::String (hfRect, 6)
                 + " hfHann=" + juce::String (hfHann, 6)
                 + " ratio=" + juce::String (ratio, 2));

        // restore default source for subsequent tests
        setParam (apvts, PID::sourceSample, 0.0f);
        pumpMessages();
    }

    // --- 5: freeze pins the playhead -> finite, bounded, sustaining pad ------
    {
        resetDefaults (apvts);
        setParam (apvts, PID::freeze,        1.0f);
        setParam (apvts, PID::density,      80.0f);
        setParam (apvts, PID::grainSize,    50.0f);
        setParam (apvts, PID::positionSpray, 20.0f);   // shimmer within the frozen region
        int peakGrains = 0;
        auto y = render (proc, { root }, 1.5, fs, &peakGrains);
        const double r  = rms (y, aOff, aLen);
        const double pk = peakAbs (y);
        check ("freeze-sustains", allFinite (y) && r > 0.005 && pk < 4.0 && peakGrains > 0,
               juce::String ("rms=") + juce::String (r, 4) + " peak=" + juce::String (pk, 3));
    }

    // --- 6: scatter 0% (sync) is peakier than scatter 100% (async/noisy) -----
    {
        resetDefaults (apvts);
        setParam (apvts, PID::grainSize,  6.0f);   // short -> clear grain-rate periodicity
        setParam (apvts, PID::density,  120.0f);
        setParam (apvts, PID::scatter,    0.0f);   // synchronous
        auto ySync  = render (proc, { root }, 1.0, fs);

        setParam (apvts, PID::scatter, 100.0f);    // asynchronous
        auto yAsync = render (proc, { root }, 1.0, fs);

        auto sSync  = analyze (ySync,  aOff, fs);
        auto sAsync = analyze (yAsync, aOff, fs);
        const double flatSync  = sSync .flatness (200.0, 8000.0);
        const double flatAsync = sAsync.flatness (200.0, 8000.0);
        check ("scatter-async-flatter", flatAsync > flatSync * 1.1 && allFinite (yAsync),
               juce::String ("flatSync=") + juce::String (flatSync, 4)
                 + " flatAsync=" + juce::String (flatAsync, 4));
    }

    // --- 7: stress — chord + max load stays finite/bounded, grains <= 192 -----
    {
        resetDefaults (apvts);
        setParam (apvts, PID::density,      200.0f);   // max
        setParam (apvts, PID::grainSize,    200.0f);   // max
        setParam (apvts, PID::positionSpray, 100.0f);
        setParam (apvts, PID::pitchSpray,    12.0f);
        setParam (apvts, PID::panSpray,     100.0f);
        setParam (apvts, PID::scatter,      100.0f);
        int peakGrains = 0;
        auto y = render (proc, { 48, 52, 55, 60, 64 }, 1.0, fs, &peakGrains);
        const double pk = peakAbs (y);
        const double r  = rms (y, aOff, aLen);
        check ("stress-bounded",
               allFinite (y) && pk < 4.0 && r > 0.001
                 && peakGrains > 0 && peakGrains <= OSimpleGrainAudioProcessor::kGlobalGrainCap,
               juce::String ("peak=") + juce::String (pk, 3)
                 + " rms=" + juce::String (r, 4)
                 + " peakGrains=" + juce::String (peakGrains)
                 + " (cap=" + juce::String (OSimpleGrainAudioProcessor::kGlobalGrainCap) + ")");
    }

    // --- 8: extreme up-transposition + high pitch spray stays bounded (DSP-08) -
    {
        resetDefaults (apvts);
        setParam (apvts, PID::sourceSample, 1.0f);   // voice (band-limited)
        pumpMessages();
        setParam (apvts, PID::grainPitch,  24.0f);   // +2 octaves
        setParam (apvts, PID::pitchSpray,  12.0f);
        setParam (apvts, PID::density,    120.0f);
        auto y = render (proc, { root + 12 }, 1.0, fs);   // note up too -> 8x source rate
        const double pk = peakAbs (y);
        const double r  = rms (y, aOff, aLen);
        check ("uptranspose-stable", allFinite (y) && pk < 4.0 && r > 0.0005,
               juce::String ("peak=") + juce::String (pk, 3) + " rms=" + juce::String (r, 4));
        setParam (apvts, PID::sourceSample, 0.0f);
        pumpMessages();
    }

    // --- 9: velToDensity depth makes the grain count track note velocity -------
    // Guards the 0..100 → 0..1 scaling of velToDensity: the param is exercised by
    // NO other gate (resetDefaults pins it to 0), and the scaling bug made it a
    // hard switch instead of a graded depth. With long, dense grains the live grain
    // count is directly countable. Depth OFF → count ~velocity-independent; depth
    // ON (full) → a hard note spawns a visibly denser cloud than a soft note.
    {
        auto peakAtVel = [&] (int vel)
        {
            proc.releaseResources();
            proc.prepareToPlay (fs, 512);   // clean voice + grain-count state per measurement
            int pk = 0;
            render (proc, { root }, 0.6, fs, &pk, vel);
            return pk;
        };

        resetDefaults (apvts);
        setParam (apvts, PID::density,    80.0f);
        setParam (apvts, PID::grainSize, 100.0f);   // overlap ~8 grains → countable

        setParam (apvts, PID::velToDensity, 0.0f);  // OFF → velocity must not move density
        const int offHi = peakAtVel (110);
        const int offLo = peakAtVel (25);

        setParam (apvts, PID::velToDensity, 100.0f); // full depth → graded by velocity
        const int onHi = peakAtVel (110);
        const int onLo = peakAtVel (25);

        const bool offFlat  = std::abs (offHi - offLo) <= 3;   // depth off → ~no velocity effect
        const bool onSpread = onHi >= onLo + 4 && onHi > offLo; // depth on → hard note denser
        check ("velToDensity-depth", offFlat && onSpread,
               juce::String ("off[hi=") + juce::String (offHi) + " lo=" + juce::String (offLo)
                 + "] on[hi=" + juce::String (onHi) + " lo=" + juce::String (onLo) + "]");
    }

    // --- 10: on-screen-keyboard bridge makes sound via the MidiMessageCollector
    // Guards bug #1 (v1.0.2): WebView-keyboard notes reach the synth ONLY through
    // handleUiMidi → midiCollector → processBlock. No other gate exercises that
    // path (render() injects MIDI straight into the host buffer), so a missing
    // collector merge / native fn shipped the keyboard silent in v1.0.1. Here we
    // inject via handleUiMidi and render with an EMPTY host MIDI buffer; the held
    // note must be drained by the collector and sustain into the analysis window.
    {
        proc.releaseResources();
        proc.prepareToPlay (fs, 512);
        resetDefaults (apvts);
        setParam (apvts, PID::density,      120.0f);
        setParam (apvts, PID::grainSize,     50.0f);
        setParam (apvts, PID::positionSpray, 70.0f);

        proc.handleUiMidi (root, true, 0.8f);   // key down via the UI bridge (no note-off → held)

        const int block = 512;
        const int total = (int) (1.0 * fs);
        juce::AudioBuffer<float> buf (2, block);
        std::vector<float> y; y.reserve ((size_t) total);
        int pos = 0, pk = 0;
        while (pos < total)
        {
            buf.clear();
            juce::MidiBuffer hostMidi;           // EMPTY — the collector is the only note source
            proc.processBlock (buf, hostMidi);
            pk = juce::jmax (pk, proc.getActiveGrainCount());
            const int n = juce::jmin (block, total - pos);
            for (int i = 0; i < n; ++i) y.push_back (buf.getSample (0, i));
            pos += block;
        }
        const double r = rms (y, aOff, aLen);
        check ("ui-midi-keyboard", r > 0.005 && allFinite (y) && pk > 0,
               juce::String ("rms=") + juce::String (r, 4) + " peakGrains=" + juce::String (pk));
    }

    // --- 11: ADSR toggle bypasses the amp envelope (v1.1.0) ------------------
    // With a long (2 s) attack the envelope is still ramping at 0.5 s, so an early
    // window is far quieter than a later one when the ADSR is ON. Turn the ADSR OFF
    // and the amp is a flat velocity-scaled gate from sample 0 — early ≈ later. The
    // ratio (early/late) collapses the cloud-warmup noise and isolates the envelope.
    {
        proc.releaseResources();
        proc.prepareToPlay (fs, 512);
        resetDefaults (apvts);
        setParam (apvts, PID::density,      120.0f);
        setParam (apvts, PID::grainSize,     50.0f);
        setParam (apvts, PID::positionSpray, 70.0f);
        setParam (apvts, PID::ampAttack,      2.0f);   // long attack — only audible if the ADSR is engaged

        const int eOff = (int) (0.06 * fs), eLen = (int) (0.06 * fs);   // [0.06,0.12] past cloud fill
        const int lOff = (int) (0.50 * fs), lLen = (int) (0.20 * fs);   // [0.50,0.70]

        setParam (apvts, PID::adsrEnabled, 1.0f);      // ON → attack still ramping → early << late
        auto yOn  = render (proc, { root }, 1.0, fs);
        const double onRatio = rms (yOn, eOff, eLen) / juce::jmax (1.0e-9, rms (yOn, lOff, lLen));

        setParam (apvts, PID::adsrEnabled, 0.0f);      // OFF → flat gate → early ≈ late
        auto yOff = render (proc, { root }, 1.0, fs);
        const double offRatio = rms (yOff, eOff, eLen) / juce::jmax (1.0e-9, rms (yOff, lOff, lLen));

        check ("adsr-toggle-bypass",
               onRatio < 0.3 && offRatio > 0.6 && offRatio > onRatio + 0.3 && allFinite (yOff),
               juce::String ("onEarly/Late=") + juce::String (onRatio, 3)
                 + " offEarly/Late=" + juce::String (offRatio, 3));
    }

    // --- 12: ADSR-off voice drains via grain windows after note-off (v1.1.0) -
    // With the ADSR off, note-off must stop spawning and let the active grains finish
    // their Window envelopes — so the cloud is loud while held, then falls silent
    // within ~one grain length and the voice frees (active grain count → 0). Guards
    // against a stuck voice (the off-mode lifetime no longer keys on ampEnv).
    {
        proc.releaseResources();
        proc.prepareToPlay (fs, 512);
        resetDefaults (apvts);
        setParam (apvts, PID::adsrEnabled,  0.0f);     // bypass
        setParam (apvts, PID::density,      120.0f);
        setParam (apvts, PID::grainSize,     50.0f);   // tail drains in ≤ ~50 ms
        setParam (apvts, PID::positionSpray, 70.0f);

        int peakG = 0;
        auto y = render (proc, { root }, 1.0, fs, &peakG, 100, 0.40);   // release at 0.40 s
        const double held = rms (y, (int) (0.20 * fs), (int) (0.15 * fs));  // [0.20,0.35] held → loud
        const double tail = rms (y, (int) (0.70 * fs), (int) (0.25 * fs));  // [0.70,0.95] drained → silent
        const int endGrains = proc.getActiveGrainCount();

        check ("adsr-off-drains",
               peakG > 0 && held > 0.005 && tail < 0.002 && endGrains == 0 && allFinite (y),
               juce::String ("peakGrains=") + juce::String (peakG)
                 + " heldRms=" + juce::String (held, 4) + " tailRms=" + juce::String (tail, 4)
                 + " endGrains=" + juce::String (endGrains));
    }

    // --- DIAG B: on-screen-keyboard (handleUiMidi) lifecycle: noteOn → noteOff →
    // retrigger. Does the collector path honour note-off and allow a fresh note?
    {
        auto renderN = [&](double secs) {
            const int block = 512; const int tot = (int) (secs * fs);
            juce::AudioBuffer<float> b (2, block); std::vector<float> o; o.reserve ((size_t) tot);
            int p = 0; while (p < tot) { b.clear(); juce::MidiBuffer empty;
                proc.processBlock (b, empty);
                const int n = juce::jmin (block, tot - p);
                for (int i = 0; i < n; ++i) o.push_back (b.getSample (0, i)); p += block; }
            return o; };

        proc.releaseResources(); proc.prepareToPlay (fs, 512);
        resetDefaults (apvts);
        setParam (apvts, PID::sourceSample, 3.0f); pumpMessages();   // piano: smooth sustained energy
        setParam (apvts, PID::positionSpray, 40.0f);

        proc.handleUiMidi (root, true, 0.8f);   auto a = renderN (0.6);   // held
        proc.handleUiMidi (root, false, 0.0f);  auto b = renderN (1.0);   // released → should go silent
        proc.handleUiMidi (root, true, 0.8f);   auto c = renderN (0.6);   // retrigger → should sound
        const double kHeld = rms (a, (int) (0.30 * fs), (int) (0.20 * fs));
        const double kTail = rms (b, (int) (0.70 * fs), (int) (0.25 * fs));
        const double kRtg  = rms (c, (int) (0.30 * fs), (int) (0.20 * fs));
        // Invariants (timing-robust — absolute level via the collector is non-
        // deterministic in this fast-render harness): note-off silences the voice,
        // and a held / re-triggered note is clearly louder than that silent tail.
        check ("kbd-lifecycle", kTail < 0.002 && kHeld > 0.0015 && kRtg > 0.0015
                                && kHeld > kTail * 4.0 && kRtg > kTail * 4.0,
               juce::String ("held=") + juce::String (kHeld, 4)
                 + " afterOff=" + juce::String (kTail, 4) + " retrig=" + juce::String (kRtg, 4));
    }

    // --- 14: polyphony + retrigger over time (no fade, no monophonic collapse) ---
    // Guards the user-reported "gradually quieter to silence on successive notes":
    // note 1 at 0, note 2 at 1.0 s (both held → must NOT be quieter than one note),
    // allNotesOff at 1.5 s, fresh note 3 at 2.0 s (must retrigger audibly).
    {
        proc.releaseResources();
        proc.prepareToPlay (fs, 512);
        resetDefaults (apvts);                         // ADSR on, default sustain 0.8
        setParam (apvts, PID::density,      120.0f);
        setParam (apvts, PID::grainSize,     50.0f);
        setParam (apvts, PID::positionSpray, 70.0f);

        const int block = 512;
        const int total = (int) (3.0 * fs);
        const int tNote2 = (int) (1.0 * fs);
        const int tRel   = (int) (1.5 * fs);
        const int tNote3 = (int) (2.0 * fs);
        juce::AudioBuffer<float> buf (2, block);
        std::vector<float> y; y.reserve ((size_t) total);
        int pos = 0; bool on1=false,on2=false,rel=false,on3=false;
        auto at = [&](juce::MidiBuffer& m, int when, bool& flag, juce::MidiMessage msg){
            if (! flag && pos + block > when) { m.addEvent (msg, juce::jlimit (0, block-1, when-pos)); flag = true; } };
        while (pos < total)
        {
            buf.clear();
            juce::MidiBuffer m;
            if (! on1) { m.addEvent (juce::MidiMessage::noteOn (1, root, (juce::uint8) 100), 0); on1 = true; }
            at (m, tNote2, on2, juce::MidiMessage::noteOn  (1, root + 7, (juce::uint8) 100));
            at (m, tRel,   rel, juce::MidiMessage::allNotesOff (1));
            at (m, tNote3, on3, juce::MidiMessage::noteOn  (1, root + 4, (juce::uint8) 100));
            proc.processBlock (buf, m);
            const int n = juce::jmin (block, total - pos);
            for (int i = 0; i < n; ++i) y.push_back (buf.getSample (0, i));
            pos += block;
        }
        const double t05 = rms (y, (int) (0.40 * fs), (int) (0.20 * fs));   // one note
        const double t12 = rms (y, (int) (1.20 * fs), (int) (0.20 * fs));   // two notes
        const double t23 = rms (y, (int) (2.30 * fs), (int) (0.20 * fs));   // fresh retrigger
        check ("poly-no-fade", t05 > 0.005 && t12 >= t05 * 0.8 && t23 > 0.005,
               juce::String ("t0.5=") + juce::String (t05, 4)
                 + " t1.2(2notes)=" + juce::String (t12, 4)
                 + " t2.3(retrig)=" + juce::String (t23, 4));
    }

    // --- 15: a single held note must NOT fade to silence over time (@48k) -------
    // Encodes the user-reported failure mode as a contract: factory defaults (Scan 0,
    // Position center, spray 0, ADSR on, sustain 0.8), one key held for 5 s at 48 kHz.
    // Late RMS must stay within a tight band of the early RMS — no global decay.
    {
        proc.releaseResources();
        proc.setPlayConfigDetails (0, 2, 48000.0, 512);
        proc.prepareToPlay (48000.0, 512);
        resetDefaults (apvts);
        setParam (apvts, PID::sourceSample, 3.0f); pumpMessages();   // piano (smooth sustained body)
        setParam (apvts, PID::positionSpray, 30.0f);                 // spread reads → stable steady-state level
        const double rate = 48000.0;
        const int block = 512; const int tot = (int) (5.0 * rate);
        juce::AudioBuffer<float> b (2, block); std::vector<float> o; o.reserve ((size_t) tot);
        int p = 0; bool on = false;
        while (p < tot) { b.clear(); juce::MidiBuffer m;
            if (! on) { m.addEvent (juce::MidiMessage::noteOn (1, root, (juce::uint8) 100), 0); on = true; }
            proc.processBlock (b, m);
            const int n = juce::jmin (block, tot - p);
            for (int i = 0; i < n; ++i) o.push_back (b.getSample (0, i)); p += block; }
        // Wide 1.5 s windows so the per-voice RNG (position spray) averages out — a
        // narrow window is noisy run-to-run; a sustained FADE would still show as
        // late << early across the wide average.
        const double early = rms (o, (int) (0.5 * rate), (int) (1.5 * rate));   // [0.5,2.0]
        const double late  = rms (o, (int) (3.0 * rate), (int) (1.5 * rate));   // [3.0,4.5]
        check ("held-no-fade@48k", early > 0.004 && late > early * 0.6,
               juce::String ("early=") + juce::String (early, 4) + " late=" + juce::String (late, 4));
    }

    proc.releaseResources();

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
