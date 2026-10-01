/*
   This file is part of O-SimpleReverb, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/
/*
    O-SimpleReverb render check

    Offline console gate. Build with -DOUARICON_BUILD_TESTS=ON, target
    O-SimpleReverb-render-check.

      1. Every factory preset recalls the TYPE its name says — in the DSP
         (roundToInt of the raw value), in the host (getIndex) and in the page
         (JS Math.round(norm * 5)).
      2. CHARACTER moves do not click: toggling it every 8 blocks keeps the
         wet output's max |second difference| near the held value (v1.10.0:
         332x..2154x).
      3. All six types render finite, non-silent audio at 44.1, 96 and
         192 kHz, and a host block four times the prepared size renders the
         same samples as four prepared-size blocks.
      4. The VU peak is HELD across blocks until read, then clears.
      5. Switches do not click, by HF burst: the peak |x| above 3 kHz in the
         30 ms after each toggle. LOW CUT on/off, LOW CUT freq jumps,
         CHARACTER crossings, SIZE 0 <-> 100 and DECAY 0.5 <-> 2.0 are gated in
         dB against a smoothed WET move; TYPE, whose types differ in steady HF,
         as a ratio over the steady HF of both types. Rapid TYPE switching and
         a run through all 48 presets must not burst either, and must leave
         the plugin sounding as if it had never happened.
      6. The six types sit within 1 dB of each other (K-weighted, BS.1770,
         pink noise, WET 100 / DRY 0), in stereo and on a mono bus.
      7. The octave shifter is an octave: a 1 kHz sine comes out at 2 kHz.
      9. Factory bank: 8 per type, one "Send" each (WET 100 / DRY 0), no stale
         file from a renamed preset, and every insert preset at or below
         +5 dB re input (K-weighted pink).
     10. The measurer in measure.h reads a synthetic decay of known T60 (a
         tone within 1 %, noise within 2 % on the mean over seeds), and finds
         a known tap, correlation and echo density. The v1.14.0 state blob in
         tests/fixtures loads with the values its .tsv lists.
     11. The engine headers in Source/dsp, driven directly: the building
         blocks (allpass, Lagrange read, RT60-to-gain, mid-band shelf, glide),
         the early-reflection taps, and the FDN's decay time against its
         target at 44.1 / 48 / 96 kHz over DECAY x SIZE.
     12. The reverb through processBlock: RT60 = base x DECAY within 10 % and
         holding across SIZE; SIZE moving the first arrival; no two FDN types
         sharing resonances; discrete early arrivals, different in L and R;
         a decorrelated tail; the old tail ringing out on a TYPE change; a
         NaN recovered within one block; no allocation in processBlock.

    (8, "Ambient's tail grows with DECAY", is superseded by 12's RT60 table.)

    A gate that waits for an engine not yet written prints PEND. PEND is
    counted on its own line and is neither a pass nor a failure.

    The factory bank is read from the INSTALLED Factory folder, which the
    processor rewrites only when its `.factory-version` sentinel differs. main()
    deletes the sentinel first, so the bank under test is this build's table.

    Modes:
      (none)                  the gates above
      --mutants               each gate of 12 (and the SIZE, DECAY and TYPE
                              cases of 5) is run against a build with the gated
                              behaviour deliberately broken, and must FAIL
                              there. A gate that still passes has not been
                              shown to test anything.
      --levels                print each type's wet loudness (stereo, mono, and
                              against DECAY and SIZE), for setting wetTrimDb.
      --baseline              print the measurement table as Markdown: 6 types x
                              DECAY 0.5/1.0/2.0 at SIZE 50 and SIZE 0/100 at
                              DECAY 1.0. No gate runs. BASELINE.md in the
                              milestone directory is this mode's output at v1.14.0.
      --write-fixture         write tests/fixtures/state-v1.14.0.{bin,tsv}.
                              Refuses on any version but 1.14.0.

    Sections 1-10 and 12 go through the processor's public surface only, so
    this file also builds against v1.14.0's Source/ (the negative control):
    section 11 and the mutants need the engine headers and the test hooks,
    and are compiled out where those do not exist.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ModulationFx.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <juce_dsp/juce_dsp.h>

#include "measure.h"

#if __has_include("dsp/FdnEngine.h")
 #include "dsp/ReverbPrimitives.h"
 #include "dsp/FdnEngine.h"
 #include "dsp/EarlyReflections.h"
 #define OSR_ENGINES 1
#else
 #define OSR_ENGINES 0
#endif

#ifndef OSIMPLEREVERB_TEST_HOOKS
 #define OSIMPLEREVERB_TEST_HOOKS 0
#endif

#if JUCE_MAC
 #include <pthread.h>
// libmalloc's stack-logging hook (what MallocStackLogging / Instruments attach
// to). It sees malloc / calloc / realloc whoever calls them, so operator new
// and juce::HeapBlock are both covered.
extern "C"
{
    typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                    uintptr_t result, uint32_t numHotFramesToSkip);
    extern malloc_logger_t* malloc_logger;
}

// volatile: clang knows malloc() as a builtin that cannot touch globals, so it
// folds "armed = true; malloc(); armed = false" into "armed = false" and the
// hook never sees the flag up.
static volatile bool allocArmed = false;
static volatile int allocCount = 0;
static pthread_t audioThread;

static void countAllocation(uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
{
    // The hook is process-wide and JUCE's TimerThread mallocs whenever it
    // likes: only the thread that calls processBlock is the audio thread.
    if (allocArmed && (type & 2u) != 0 && pthread_equal(pthread_self(), audioThread))   // MALLOC_LOG_TYPE_ALLOCATE
        allocCount = allocCount + 1;
}
#endif

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

static int passes = 0, failures = 0, pending = 0;

static void check(bool ok, const juce::String& what)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    ++(ok ? passes : failures);
}

// A gate whose engine is not written yet.
static void pend(const juce::String& what)
{
    std::printf("PEND: %s\n", what.toRawUTF8());
    ++pending;
}

// What a gate found. Section 12's gates return one, so --mutants can run the
// same code against a broken build and require the opposite answer.
struct Verdict { bool ok; juce::String text; };
static void check(const Verdict& v) { check(v.ok, v.text); }

#if OSIMPLEREVERB_TEST_HOOKS
using Mutant = OSimpleReverbAudioProcessor::Mutant;
static Mutant activeMutant = Mutant::none;   // every processor made below is this build
#endif

static std::unique_ptr<OSimpleReverbAudioProcessor> makeProcessor()
{
    std::unique_ptr<juce::AudioProcessor> base(createPluginFilter());
    std::unique_ptr<OSimpleReverbAudioProcessor> proc(dynamic_cast<OSimpleReverbAudioProcessor*>(base.release()));
   #if OSIMPLEREVERB_TEST_HOOKS
    if (proc != nullptr) proc->testMutant = activeMutant;
   #endif
    return proc;
}

static void setParam(OSimpleReverbAudioProcessor& proc, const char* id, float value)
{
    auto* p = proc.parameters.getParameter(id);
    p->setValueNotifyingHost(p->convertTo0to1(value));
}

// Puts the processor on a stereo or a mono bus and prepares it.
static bool configure(OSimpleReverbAudioProcessor& proc, int channels, double sr, int blockSize)
{
    const auto set = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(set);
    layout.outputBuses.add(set);
    const bool ok = proc.setBusesLayout(layout);
    proc.setRateAndBufferSizeDetails(sr, blockSize);
    proc.prepareToPlay(sr, blockSize);
    return ok;
}

// Renders `blocks` blocks of a 300 Hz sine at 0.5, calling perBlock(b) first.
template <typename PerBlock>
static std::vector<float> renderSine(OSimpleReverbAudioProcessor& proc, double sr, int blockSize,
                                     int blocks, PerBlock perBlock)
{
    proc.setPlayConfigDetails(2, 2, sr, blockSize);
    proc.prepareToPlay(sr, blockSize);
    juce::AudioBuffer<float> buf(2, blockSize);
    juce::MidiBuffer midi;
    std::vector<float> out;
    double phase = 0.0;
    for (int b = 0; b < blocks; ++b) {
        perBlock(b);
        for (int i = 0; i < blockSize; ++i) {
            const float x = 0.5f * (float) std::sin(phase);
            phase += 2.0 * juce::MathConstants<double>::pi * 300.0 / sr;
            buf.setSample(0, i, x);
            buf.setSample(1, i, x);
        }
        proc.processBlock(buf, midi);
        for (int i = 0; i < blockSize; ++i)
            out.push_back(buf.getSample(0, i));
    }
    return out;
}

// ── click metric ──────────────────────────────────────────────────────────────
// A 300 Hz sine at 0.5 goes in; the output's left channel goes through a 4th-
// order Butterworth high-pass at 3 kHz. A click is a broadband burst there.
static std::vector<float> above(float hz, const std::vector<float>& x, double sr = 48000.0)
{
    juce::dsp::IIR::Filter<float> h1, h2;
    h1.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, hz, 0.541f);
    h2.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, hz, 1.307f);
    std::vector<float> hf;
    hf.reserve(x.size());
    for (float v : x) hf.push_back(std::abs(h2.processSample(h1.processSample(v))));
    return hf;
}

static float peakOf(const std::vector<float>& x, size_t from, size_t to)
{
    float m = 1.0e-7f;
    for (size_t i = from; i < to && i < x.size(); ++i) m = std::max(m, std::abs(x[i]));
    return m;
}

static double rmsOf(const std::vector<float>& x, size_t from, size_t to)
{
    double sum = 0.0;
    size_t n = 0;
    for (size_t i = from; i < to && i < x.size(); ++i, ++n) sum += (double) x[i] * x[i];
    return std::sqrt(sum / (double) std::max((size_t) 1, n));
}

// Toggles `id` between a and b every 24 blocks (256 @ 48 kHz), WET 100 / DRY 0.
struct Burst { float db; float ratio; };  // db re 0.5; ratio over steady HF
static Burst hfBurst(const char* id, float a, float b, float lpOn = 0.0f, int type = -1, float aboveHz = 3000.0f)
{
    auto proc = makeProcessor();
    setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
    if (type >= 0) setParam(*proc, "TYPE", (float) type);
    setParam(*proc, "LPON", lpOn); setParam(*proc, id, a);
    const int bs = 256, per = 24;
    const auto hf = above(aboveHz, renderSine(*proc, 48000.0, bs, per * 2 * 14, [&](int blk) {
        setParam(*proc, id, (blk / per) % 2 ? b : a);
    }));
    const size_t W = 1440, T = (size_t) per * bs;
    float burst = 0.0f, worst = 0.0f;
    for (size_t t = 4 * T; t + T < hf.size(); t += T) {
        const float after = peakOf(hf, t, t + W);
        burst = std::max(burst, after);
        worst = std::max(worst, after / std::max(peakOf(hf, t - W, t), peakOf(hf, t + T - W, t + T)));
    }
    return { juce::Decibels::gainToDecibels(burst / 0.5f), worst };
}

// K-weighting (BS.1770 @ 48 kHz): shelf then high-pass.
struct KWeight {
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0, u1 = 0, u2 = 0, z1 = 0, z2 = 0;
    double operator()(double x) {
        const double y = 1.53512485958697 * x - 2.69169618940638 * x1 + 1.19839281085285 * x2
                       + 1.69065929318241 * y1 - 0.73248077421585 * y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y;
        const double z = y - 2.0 * u1 + u2 + 1.99004745483398 * z1 - 0.99007225036621 * z2;
        u2 = u1; u1 = y; z2 = z1; z1 = z;
        return z;
    }
};

// Output loudness re input, K-weighted, pink noise (Kellet), 4 s after 1 s
// settle, with the processor's parameters as they stand. channels = 1 puts the
// processor on a mono bus.
static double loudnessDb(OSimpleReverbAudioProcessor& p, int channels = 2)
{
    auto* proc = &p;
    const int bs = 512;
    configure(*proc, channels, 48000.0, bs);
    juce::AudioBuffer<float> buf(channels, bs);
    juce::MidiBuffer midi;
    std::mt19937 rng(7);
    std::normal_distribution<float> nd(0.0f, 0.1f);
    float p1 = 0, p2 = 0, p3 = 0;
    KWeight kin, kl, kr;
    double sumIn = 0, sumOut = 0;
    for (int blk = 0; blk < 48000 * 5 / bs; ++blk) {
        const bool measure = blk * bs >= 48000;
        for (int i = 0; i < bs; ++i) {
            const float w = nd(rng);
            p1 = 0.99886f * p1 + w * 0.0555f; p2 = 0.99332f * p2 + w * 0.0750f; p3 = 0.969f * p3 + w * 0.1538f;
            const float x = p1 + p2 + p3 + w * 0.1848f;
            for (int ch = 0; ch < channels; ++ch) buf.setSample(ch, i, x);
            const double k = kin(x);
            if (measure) sumIn += k * k;
        }
        proc->processBlock(buf, midi);
        for (int i = 0; i < bs; ++i) {
            const double l = kl(buf.getSample(0, i)), r = channels > 1 ? kr(buf.getSample(1, i)) : l;
            if (measure) sumOut += 0.5 * (l * l + r * r);
        }
    }
    return 10.0 * std::log10(sumOut / sumIn);
}

// Wet loudness of one type re its input (WET 100 / DRY 0).
static double typeLoudnessDb(int type, int channels = 2, float decay = 1.0f, float size = 50.0f)
{
    auto proc = makeProcessor();
    setParam(*proc, "TYPE", (float) type); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
    setParam(*proc, "DECAY", decay); setParam(*proc, "SIZE", size);
    return loudnessDb(*proc, channels);
}

// Goertzel power of x at f (Hz)
static double goertzel(const std::vector<float>& x, double f, double sr, size_t from = 0)
{
    const double w = juce::MathConstants<double>::twoPi * f / sr, c = 2.0 * std::cos(w);
    double s1 = 0, s2 = 0;
    for (size_t i = from; i < x.size(); ++i) { const double s0 = x[i] + c * s1 - s2; s2 = s1; s1 = s0; }
    return s1 * s1 + s2 * s2 - c * s1 * s2;
}

// ── reverb-engine-rewrite: measurements, baseline, state fixture ─────────────

static const char* const kTypeNames[6] = { "Booth", "Room", "Hall", "Spring", "Plate", "Ambient" };

// v2.0.0's RT60 at DECAY 1.0x, in TYPE order (CONTEXT.md requirement 5). The
// target is base x DECAY at every SIZE.
static const double kBaseT60[6] = { 0.40, 1.1, 3.0, 2.5, 2.5, 7.0 };

static float paramValue(OSimpleReverbAudioProcessor& proc, const char* id)
{
    auto* p = proc.parameters.getParameter(id);
    return p->convertFrom0to1(p->getValue());
}

// Wet impulse response of one type at 48 kHz, block 512.
static measure::Stereo typeIr(int type, float decay, float size, double seconds, double sr = 48000.0)
{
    auto proc = makeProcessor();
    setParam(*proc, "TYPE", (float) type); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
    setParam(*proc, "DECAY", decay); setParam(*proc, "SIZE", size);
    return measure::impulseResponse(*proc, sr, 512, seconds);
}

// The same, rendered again at a longer length until the IR covers
// 1.35 x its own mid RT60 + 0.4 s (for a tail whose length is not known).
static measure::Stereo typeIrCovering(int type, float decay, float size, double sr = 48000.0)
{
    double seconds = 6.0;
    for (;;) {
        auto ir = typeIr(type, decay, size, seconds, sr);
        const double need = 1.35 * measure::midRt60(ir.l, sr) + 0.4;
        if (seconds >= need || seconds >= 45.0) return ir;
        seconds = std::min(45.0, std::max(need + 0.1, seconds * 1.5));
    }
}

// The window of an IR's tail used for stereo correlation and for the tail
// spectrum: it starts a quarter of the mid RT60 after the first arrival.
struct TailWindow { double t0, t1; };
static TailWindow correlationWindow(const measure::Vec& ir, double sr, double t60)
{
    const double on = (double) measure::onsetIndex(ir) / sr;
    return { on + 0.25 * t60, on + 0.75 * t60 };
}
static TailWindow spectrumWindow(const measure::Vec& ir, double sr, double t60)
{
    const double on = (double) measure::onsetIndex(ir) / sr;
    const double t0 = on + std::max(0.05, 0.25 * t60);
    return { t0, std::min((double) ir.size() / sr, t0 + juce::jlimit(0.25, 2.0, t60)) };
}

static juce::String tapList(const std::vector<measure::Tap>& taps, size_t most = 8)
{
    juce::String s;
    for (size_t i = 0; i < taps.size() && i < most; ++i)
        s << (i ? ", " : "") << juce::String(taps[i].ms, 2) << (taps[i].amp < 0 ? "-" : "+");
    if (taps.size() > most) s << ", ...";
    return s.isEmpty() ? juce::String("none") : s;
}

static int printBaseline()
{
    const double sr = 48000.0;
    std::printf("# Baseline: O-SimpleReverb %s\n\n", JucePlugin_VersionString);
    std::printf("Output of `O-SimpleReverb-render-check --baseline`. Measured through `processBlock` only\n"
                "(`tests/render-check/measure.h`): 48 kHz, block 512, WET 100 / DRY 0, CHARACTER 0, LOW CUT off,\n"
                "a unit impulse in both channels after 0.5 s of silence. Left channel unless a column says L / R.\n\n");

    struct Row { int type; float decay, size; };
    std::vector<Row> rows;
    for (int t = 0; t < 6; ++t)
        for (auto ds : { std::pair<float, float> { 0.5f, 50.0f }, { 1.0f, 50.0f }, { 2.0f, 50.0f }, { 1.0f, 0.0f }, { 1.0f, 100.0f } })
            rows.push_back({ t, ds.first, ds.second });

    std::vector<measure::Stereo> irs;
    std::vector<double> mids;
    for (const auto& r : rows) {
        irs.push_back(typeIrCovering(r.type, r.decay, r.size, sr));
        mids.push_back(measure::midRt60(irs.back().l, sr));
    }

    std::printf("## Decay time and stereo\n\n"
                "RT60 is T30 (Schroeder integral, line over -5..-35 dB, extrapolated to 60 dB). Mid = 354-1414 Hz,\n"
                "8 kHz = 5657-11314 Hz, 125 Hz = 88-177 Hz. Target = v2.0.0's base RT60 x DECAY, at every SIZE.\n"
                "L/R correlation is zero-lag, full band, from 0.25 to 0.75 x mid RT60 after the first arrival.\n\n"
                "| Type | DECAY | SIZE | Target (s) | Mid RT60 (s) | vs target | 8 kHz RT60 (s) | 125 Hz RT60 (s) | L/R correlation | IR length (s) |\n"
                "|---|---|---|---|---|---|---|---|---|---|\n");
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i]; const auto& ir = irs[i];
        const double target = kBaseT60[r.type] * r.decay;
        const auto w = correlationWindow(ir.l, sr, mids[i]);
        std::printf("| %s | %.1fx | %.0f | %.2f | %.3f | %+.0f %% | %.3f | %.3f | %+.3f | %.1f |\n",
                    kTypeNames[r.type], r.decay, r.size, target, mids[i], 100.0 * (mids[i] / target - 1.0),
                    measure::hfRt60(ir.l, sr), measure::lfRt60(ir.l, sr),
                    measure::correlation(ir.l, ir.r, (size_t) (w.t0 * sr), (size_t) (w.t1 * sr)), (double) ir.l.size() / sr);
    }

    std::printf("\n## Impulse response structure\n\n"
                "First arrival = first sample within 40 dB of the IR's peak. Early arrivals = local maxima within\n"
                "20 dB of the largest sample in the 60 ms after the first arrival (count L / R, and how many of L's\n"
                "have no R arrival within 0.1 ms). Echo density is Abel & Huang's (1.0 = Gaussian), 20 ms window,\n"
                "100 ms after the first arrival; mixing time is when it first reads 0.9. -60 dB = the decay curve's\n"
                "own crossing, no extrapolation.\n\n"
                "| Type | DECAY | SIZE | First arrival L / R (ms) | Early arrivals L / R | L without R | Echo density @ 100 ms | Mixing time (ms) | Peak (dBFS) | Energy (dB) | -60 dB at (s) |\n"
                "|---|---|---|---|---|---|---|---|---|---|---|\n");
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i]; const auto& ir = irs[i];
        const size_t onL = measure::onsetIndex(ir.l), onR = measure::onsetIndex(ir.r);
        const auto tl = measure::earlyTaps(ir.l, sr, onL), tr = measure::earlyTaps(ir.r, sr, onL);
        double peak = 0, energy = 0;
        for (double v : ir.l) { peak = std::max(peak, std::abs(v)); energy += v * v; }
        std::printf("| %s | %.1fx | %.0f | %.2f / %.2f | %d / %d | %d | %.2f | %.0f | %.1f | %.1f | %.2f |\n",
                    kTypeNames[r.type], r.decay, r.size, 1000.0 * (double) onL / sr, 1000.0 * (double) onR / sr,
                    (int) tl.size(), (int) tr.size(), measure::tapsWithoutPartner(tl, tr),
                    measure::echoDensityAt(ir.l, sr, onL + (size_t) (0.1 * sr)), measure::mixingTimeMs(ir.l, sr, onL),
                    20.0 * std::log10(peak + 1.0e-30), 10.0 * std::log10(energy + 1.0e-30), measure::decayTimeTo(ir.l, sr, 60.0));
    }

    std::printf("\n## Early arrivals at DECAY 1.0x, SIZE 50\n\n"
                "Times in ms after the left channel's first arrival, with the sign of the sample; first eight.\n\n"
                "| Type | L | R |\n|---|---|---|\n");
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].decay != 1.0f || rows[i].size != 50.0f) continue;
        const size_t on = measure::onsetIndex(irs[i].l);
        std::printf("| %s | %s | %s |\n", kTypeNames[rows[i].type],
                    tapList(measure::earlyTaps(irs[i].l, sr, on)).toRawUTF8(), tapList(measure::earlyTaps(irs[i].r, sr, on)).toRawUTF8());
    }

    std::printf("\n## Do two types share resonances?\n\n"
                "Pearson correlation of the tail's dB spectrum between types at DECAY 1.0x, SIZE 50: 200-2000 Hz in\n"
                "0.5 Hz steps, Hann window starting 0.25 x mid RT60 (at least 50 ms) after the first arrival and one\n"
                "mid RT60 long (0.25..2 s), a 100 Hz moving average removed so that an EQ tilt does not count.\n"
                "1.0 = the same resonant frequencies, 0 = unrelated.\n\n| |");
    std::vector<measure::Vec> spectra(6);
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].decay != 1.0f || rows[i].size != 50.0f) continue;
        const auto w = spectrumWindow(irs[i].l, sr, mids[i]);
        spectra[(size_t) rows[i].type] = measure::tailSpectrumDb(irs[i].l, sr, w.t0, w.t1);
    }
    for (int t = 1; t < 6; ++t) std::printf(" %s |", kTypeNames[t]);
    std::printf("\n|---|");
    for (int t = 1; t < 6; ++t) std::printf("---|");
    std::printf("\n");
    for (int a = 0; a < 5; ++a) {
        std::printf("| %s |", kTypeNames[a]);
        for (int b = 1; b < 6; ++b) {
            if (b <= a) std::printf(" |");
            else std::printf(" %+.2f |", measure::pearson(spectra[(size_t) a], spectra[(size_t) b]));
        }
        std::printf("\n");
    }

    std::printf("\n## When each band arrives (DECAY 1.0x, SIZE 50)\n\n"
                "First sample of a narrow band (f / 1.06 .. f x 1.06) within 20 dB of that band's largest in the\n"
                "160 ms after the first arrival, in ms, less the same reading of the band filter itself. A\n"
                "dispersive spring arrives later as frequency rises (a chirp); anything else reads about 0.\n\n"
                "| Type | 300 Hz | 600 Hz | 1 kHz | 2 kHz | 3 kHz | 3.8 kHz | 3 kHz - 1 kHz |\n|---|---|---|---|---|---|---|---|\n");
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].decay != 1.0f || rows[i].size != 50.0f) continue;
        const size_t on = measure::onsetIndex(irs[i].l);
        std::printf("| %s |", kTypeNames[rows[i].type]);
        double at1k = 0, at3k = 0;
        for (double f : { 300.0, 600.0, 1000.0, 2000.0, 3000.0, 3800.0 }) {
            const double ms = measure::bandArrivalMs(irs[i].l, sr, on, f);
            if (f == 1000.0) at1k = ms;
            if (f == 3000.0) at3k = ms;
            std::printf(" %.1f |", ms);
        }
        std::printf(" %+.1f |\n", at3k - at1k);
    }

    std::printf("\n## CPU\n\n"
                "Wall time inside `processBlock` per second of audio, white noise in, block 512, median of 5 runs of\n"
                "5 s, at each type's defaults (WET 25 / DRY 100). One core of the machine that ran this; a figure to\n"
                "compare on the same machine, never a pass/fail.\n\n"
                "| Type | 48 kHz | 96 kHz |\n|---|---|---|\n");
    for (int t = 0; t < 6; ++t) {
        std::printf("| %s |", kTypeNames[t]);
        for (double rate : { 48000.0, 96000.0 }) {
            auto proc = makeProcessor();
            setParam(*proc, "TYPE", (float) t);
            std::printf(" %.2f %% |", measure::cpuPercent(*proc, rate));
        }
        std::printf("\n");
    }
    return 0;
}

// The v1.14.0 state blob: every parameter away from its default, and the UI
// language away from English. The .tsv beside it is what the blob holds, as
// read back from the processor that wrote it.
static const struct { const char* id; float value; } kFixtureValues[8] = {
    { "TYPE", 4.0f }, { "CHARACTER", -37.5f }, { "WET", 62.5f }, { "DRY", 41.0f },
    { "DECAY", 1.37f }, { "SIZE", 83.0f }, { "LPFREQ", 137.0f }, { "LPON", 1.0f },
};

static juce::File fixtureDir()
{
    return juce::File(__FILE__).getParentDirectory().getSiblingFile("fixtures");
}

static int writeFixture()
{
    if (juce::String(JucePlugin_VersionString) != "1.14.0") {
        std::printf("refused: this build is %s; the fixture is a v1.14.0 state and is written once\n", JucePlugin_VersionString);
        return 1;
    }
    auto proc = makeProcessor();
    for (const auto& v : kFixtureValues) setParam(*proc, v.id, v.value);
    proc->uiLanguage.store(1);   // fr
    juce::MemoryBlock blob;
    proc->getStateInformation(blob);

    juce::String tsv("# O-SimpleReverb v1.14.0 state fixture: what state-v1.14.0.bin holds\n#id\tvalue\tnormalised\n");
    for (const auto& v : kFixtureValues)
        tsv << v.id << "\t" << juce::String(paramValue(*proc, v.id), 4) << "\t"
            << juce::String(proc->parameters.getParameter(v.id)->getValue(), 6) << "\n";
    tsv << "uiLanguage\t" << OSimpleReverbAudioProcessor::languageCode(proc->uiLanguage.load()) << "\t\n";

    const auto dir = fixtureDir();
    const bool ok = dir.createDirectory().wasOk()
                    && dir.getChildFile("state-v1.14.0.bin").replaceWithData(blob.getData(), blob.getSize())
                    && dir.getChildFile("state-v1.14.0.tsv").replaceWithText(tsv, false, false, "\n");
    std::printf("%s %s (%d bytes) and state-v1.14.0.tsv\n", ok ? "wrote" : "FAILED to write",
                dir.getChildFile("state-v1.14.0.bin").getFullPathName().toRawUTF8(), (int) blob.getSize());
    return ok ? 0 : 1;
}

// Margins of section 5's SIZE, DECAY, rapid-TYPE and preset-sweep cases (dB).
static constexpr float kSizeGlideMarginDb = 6.0f, kDecayGlideMarginDb = 6.0f, kRapidMarginDb = 6.0f, kSweepMarginDb = 12.0f;

// ── 12. the reverb through processBlock ──────────────────────────────────────
// Each gate is a function of the processor's public surface, so it reads the
// same on v1.14.0 (the negative control) and on any mutant.

static const int kFdnTypes[4] = { 0, 1, 2, 5 };   // Booth, Room, Hall, Ambient
static constexpr double kRt60Tolerance = 0.10;

static double typeRt60(int type, float decay, float size)
{
    const double target = kBaseT60[type] * decay;
    return measure::midRt60(typeIr(type, decay, size, 1.35 * target + 0.6).l, 48000.0);
}

// RT60 = base x DECAY at SIZE 50.
static Verdict rt60DecayGate(int type)
{
    juce::String list;
    double worst = 0.0;
    for (float decay : { 0.5f, 1.0f, 2.0f }) {
        const double target = kBaseT60[type] * decay, got = typeRt60(type, decay, 50.0f), err = got / target - 1.0;
        if (std::abs(err) > std::abs(worst)) worst = err;
        list << (list.isEmpty() ? "" : ", ") << juce::String(decay, 1) << "x " << juce::String(got, 3) << " s (" << juce::String(target, 2) << ")";
    }
    return { std::abs(worst) <= kRt60Tolerance,
             juce::String(kTypeNames[type]) + " RT60 = base x DECAY: " + list + "; worst " + juce::String(100.0 * worst, 1) + " % (within 10)" };
}

// The same RT60 at SIZE 0 and SIZE 100: SIZE is not a decay control.
static Verdict rt60SizeGate(int type)
{
    const double target = kBaseT60[type], at0 = typeRt60(type, 1.0f, 0.0f), at100 = typeRt60(type, 1.0f, 100.0f);
    const double worst = std::abs(at0 / target - 1.0) > std::abs(at100 / target - 1.0) ? at0 / target - 1.0 : at100 / target - 1.0;
    return { std::abs(worst) <= kRt60Tolerance,
             juce::String(kTypeNames[type]) + " RT60 holds across SIZE: " + juce::String(at0, 3) + " s at 0, " + juce::String(at100, 3)
             + " s at 100 (" + juce::String(target, 2) + "); worst " + juce::String(100.0 * worst, 1) + " % (within 10)" };
}

// SIZE moves the structure: the first arrival comes later in a larger space.
// Read on the first arrival, not on the mixing time - v1.14.0's mixing time
// moved with SIZE (Ambient 85 -> 278 ms) with no delay changing, because it
// follows the decay.
static Verdict sizeStructureGate(int type)
{
    const auto small = typeIr(type, 1.0f, 0.0f, 0.4), large = typeIr(type, 1.0f, 100.0f, 0.4);
    const double a = 1000.0 * (double) measure::onsetIndex(small.l) / 48000.0, b = 1000.0 * (double) measure::onsetIndex(large.l) / 48000.0;
    return { b - a >= 0.5, juce::String(kTypeNames[type]) + " first arrival moves with SIZE: " + juce::String(a, 2) + " ms at 0, "
                           + juce::String(b, 2) + " ms at 100 (" + juce::String(b - a, 2) + " ms later, >= 0.5)" };
}

// No two FDN types share their resonances.
static Verdict tailSpectrumGate()
{
    measure::Vec spectra[4];
    for (int k = 0; k < 4; ++k) {
        const auto ir = typeIrCovering(kFdnTypes[k], 1.0f, 50.0f);
        const auto w = spectrumWindow(ir.l, 48000.0, measure::midRt60(ir.l, 48000.0));
        spectra[k] = measure::tailSpectrumDb(ir.l, 48000.0, w.t0, w.t1);
    }
    double worst = 0.0;
    juce::String pair;
    for (int a = 0; a < 4; ++a)
        for (int b = a + 1; b < 4; ++b) {
            const double r = measure::pearson(spectra[a], spectra[b]);
            if (std::abs(r) >= std::abs(worst)) { worst = r; pair = juce::String(kTypeNames[kFdnTypes[a]]) + " / " + kTypeNames[kFdnTypes[b]]; }
        }
    return { std::abs(worst) < 0.3, "tail spectra of Booth, Room, Hall, Ambient are unrelated: worst pair " + pair + " "
                                    + juce::String(worst, 2) + " (|r| < 0.3; v1.14.0: 0.72..0.96)" };
}

// Discrete early arrivals at the output, different in L and R: a handful of
// arrivals stand above everything else in the 60 ms after the first one, and
// none of L's has an R arrival at the same time. v1.14.0 has about a hundred
// (its taps went into the tank, whose eight combs each repeated them), and so
// does the tank on its own.
struct EarlyCount { int l, r, alone; };
static EarlyCount earlyCount(int type, float decay)
{
    const auto ir = typeIr(type, decay, 50.0f, 0.4);
    const size_t on = measure::onsetIndex(ir.l);
    const auto tl = measure::earlyTaps(ir.l, 48000.0, on, 60.0, -12.0), tr = measure::earlyTaps(ir.r, 48000.0, on, 60.0, -12.0);
    return { (int) tl.size(), (int) tr.size(), measure::tapsWithoutPartner(tl, tr) };
}

static Verdict earlyTapsGate(int type)
{
    const auto atBase = earlyCount(type, 1.0f), atMin = earlyCount(type, 0.5f);
    auto discrete = [](const EarlyCount& c) { return c.l >= 3 && c.l <= 12 && c.r >= 3 && c.r <= 12 && c.alone == c.l; };
    return { discrete(atBase) && discrete(atMin),
             juce::String(kTypeNames[type]) + " early arrivals within 12 dB of the largest, L / R (L with no R partner): "
             + juce::String(atBase.l) + " / " + juce::String(atBase.r) + " (" + juce::String(atBase.alone) + ") at DECAY 1.0x, "
             + juce::String(atMin.l) + " / " + juce::String(atMin.r) + " (" + juce::String(atMin.alone) + ") at 0.5x (3..12 a side, all of L's alone)" };
}

// The late tail is decorrelated between L and R for a mono input.
static Verdict stereoGate()
{
    double worst = 0.0;
    juce::String where;
    for (int type : kFdnTypes) {
        const auto ir = typeIrCovering(type, 1.0f, 50.0f);
        const auto w = correlationWindow(ir.l, 48000.0, measure::midRt60(ir.l, 48000.0));
        const double r = measure::correlation(ir.l, ir.r, (size_t) (w.t0 * 48000.0), (size_t) (w.t1 * 48000.0));
        if (std::abs(r) >= std::abs(worst)) { worst = r; where = kTypeNames[type]; }
    }
    return { std::abs(worst) < 0.3, "late-tail L/R correlation, mono input: worst " + juce::String(worst, 3) + " (" + where + "), |r| < 0.3" };
}

// Noise into `from` for 1.5 s, then TYPE goes to `to` and the input stops.
// The old tail must still be there 200 ms later, and decay on ITS type's time.
static Verdict ringOutGate(int from = 2, int to = 0)
{
    const double sr = 48000.0;
    const int bs = 512, noiseBlocks = 141, tailBlocks = (int) ((1.35 * kBaseT60[from] + 0.6) * sr) / bs;
    auto proc = makeProcessor();
    setParam(*proc, "TYPE", (float) from); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
    configure(*proc, 2, sr, bs);
    juce::AudioBuffer<float> buf(2, bs);
    juce::MidiBuffer midi;
    std::mt19937 rng(21);
    std::normal_distribution<float> nd(0.0f, 0.1f);
    std::vector<float> out;
    for (int blk = 0; blk < noiseBlocks + tailBlocks; ++blk) {
        if (blk == noiseBlocks) setParam(*proc, "TYPE", (float) to);
        for (int i = 0; i < bs; ++i) {
            const float x = blk < noiseBlocks ? nd(rng) : 0.0f;
            buf.setSample(0, i, x); buf.setSample(1, i, x);
        }
        proc->processBlock(buf, midi);
        for (int i = 0; i < bs; ++i) out.push_back(buf.getSample(0, i));
    }
    const size_t at = (size_t) noiseBlocks * bs;
    const double before = rmsOf(out, at - 4800, at), later = rmsOf(out, at + 7200, at + 12000);
    const double stillDb = 20.0 * std::log10(later / before + 1.0e-30);
    measure::Vec tail(out.begin() + (long) at + 2400, out.end());
    const double t60 = measure::midRt60(tail, sr), err = t60 / kBaseT60[from] - 1.0;
    return { stillDb > -10.0 && std::abs(err) <= 0.15,
             juce::String(kTypeNames[from]) + " -> " + kTypeNames[to] + ": old tail 200 ms after the switch " + juce::String(stillDb, 1)
             + " dB re before (> -10), decays at " + juce::String(t60, 2) + " s (" + juce::String(kBaseT60[from], 1) + " s, its own type; within 15 %)" };
}

// SIZE and DECAY moves under signal do not click.
static Verdict glideGate(const char* id, float a, float b, float refDb, float marginDb)
{
    float worst = -200.0f;
    juce::String list;
    for (int type : { 1, 2 }) {
        const float db = hfBurst(id, a, b, 0.0f, type).db;
        worst = std::max(worst, db);
        list << (list.isEmpty() ? "" : ", ") << kTypeNames[type] << " " << juce::String(db, 1);
    }
    return { worst < refDb + marginDb, juce::String(id) + " " + juce::String(a, 1) + "<->" + juce::String(b, 1) + " HF burst " + list
                                       + " dB < ref + " + juce::String(marginDb, 0) };
}

// TYPE switched every `per` blocks round `cycle` for 1.5 s, then held on the
// first type. Two failures to catch: a click while it switches, and a slot
// left stuck or silent afterwards. burstDb is re 0.5; settleDb is the last
// quarter second's level against a render that never switched. The cycle
// starts on Booth, whose lines are not modulated: a modulated type's level
// under a steady sine depends on where its LFOs are, and a slot restarted
// during the switching is somewhere else in them (Room read -0.8 dB).
struct Switching { float burstDb; double settleDb; bool finite; };
static Switching rapidSwitching(const std::vector<int>& cycle, int per = 3)
{
    const int bs = 256, hold = 188, rapid = 282, after = 940;
    auto run = [&](bool switching) {
        auto proc = makeProcessor();
        setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f); setParam(*proc, "DECAY", 0.5f);
        setParam(*proc, "TYPE", (float) cycle[0]);
        return renderSine(*proc, 48000.0, bs, hold + rapid + after, [&](int blk) {
            const bool inRapid = switching && blk >= hold && blk < hold + rapid;
            setParam(*proc, "TYPE", (float) (inRapid ? cycle[(size_t) ((blk - hold) / per) % cycle.size()] : cycle[0]));
        });
    };
    const auto out = run(true), ref = run(false);
    bool finite = true;
    for (float v : out) finite = finite && std::isfinite(v);
    const auto hf = above(3000.0f, out);
    const size_t n = out.size(), q = 12000;
    return { juce::Decibels::gainToDecibels(peakOf(hf, (size_t) hold * bs, (size_t) (hold + rapid) * bs + 4800) / 0.5f),
             20.0 * std::log10(rmsOf(out, n - q, n) / rmsOf(ref, n - q, n) + 1.0e-30), finite };
}

static Verdict rapidSwitchGate(const std::vector<int>& cycle, float slowDb, float marginDb)
{
    const auto s = rapidSwitching(cycle);
    juce::String names;
    for (int t : cycle) names << (names.isEmpty() ? "" : "-") << kTypeNames[t];
    return { s.finite && s.burstDb < slowDb + marginDb && std::abs(s.settleDb) < 0.5,
             "TYPE " + names + " every 16 ms: HF burst " + juce::String(s.burstDb, 1) + " dB < slow switch " + juce::String(slowDb, 1)
             + " + " + juce::String(marginDb, 0) + "; afterwards " + juce::String(s.settleDb, 2) + " dB re never switched (|x| < 0.5)" };
}

// Every factory preset loaded in turn, 64 ms apart, under the sine; Booth's
// eight last, so the tails are short by the end. Then the last one is held.
// The sweep starts a second in: the sine's own start is a click (-41 dB above
// 8 kHz through the dry path) and has to ring out first. Above 8 kHz, not 3:
// consecutive presets of one type move SIZE, and the glide's pitch bend takes
// a 300 Hz tail up to 1.2 kHz, which a 3 kHz filter's skirt lets through.
static Verdict presetSweepGate(float refDb, float marginDb)
{
    const int bs = 256, lead = 188, per = 12, after = 1128;
    juce::StringArray order, booth;
    {
        auto proc = makeProcessor();
        for (const auto& name : proc->presetManager.getPresetList()) {
            if (! proc->presetManager.isFactoryPreset(name)) continue;
            (name.startsWith("Booth") ? booth : order).add(name);
        }
        order.addArray(booth);
    }
    if (order.isEmpty()) return { false, "preset sweep: no factory presets found" };
    auto run = [&](bool sweeping) {
        auto proc = makeProcessor();
        if (! sweeping) proc->presetManager.loadPreset(order[order.size() - 1]);
        return renderSine(*proc, 48000.0, bs, lead + per * order.size() + after, [&](int blk) {
            const int step = blk - lead;
            if (sweeping && step >= 0 && step % per == 0 && step / per < order.size()) proc->presetManager.loadPreset(order[step / per]);
        });
    };
    const auto out = run(true), ref = run(false);
    bool finite = true;
    for (float v : out) finite = finite && std::isfinite(v);
    const auto hf = above(8000.0f, out);
    const size_t n = out.size(), q = 12000;
    const size_t from = (size_t) lead * bs, to = (size_t) (lead + per * order.size()) * bs + 4800;
    const float burstDb = juce::Decibels::gainToDecibels(peakOf(hf, from, to) / 0.5f);
    std::printf("  (preset sweep: above 3 kHz %.1f dB, above 8 kHz %.1f dB)\n",
                juce::Decibels::gainToDecibels(peakOf(above(3000.0f, out), from, to) / 0.5f), burstDb);
    const double settleDb = 20.0 * std::log10(rmsOf(out, n - q, n) / rmsOf(ref, n - q, n) + 1.0e-30);
    return { finite && burstDb < refDb + marginDb && std::abs(settleDb) < 0.5,
             "all " + juce::String(order.size()) + " presets 64 ms apart: burst above 8 kHz " + juce::String(burstDb, 1) + " dB < WET ref there ("
             + juce::String(refDb, 1) + ") + "
             + juce::String(marginDb, 0) + "; afterwards " + juce::String(settleDb, 2) + " dB re the last preset held (|x| < 0.5)" };
}

// A host block four times the prepared size, with WET and TYPE moving, renders
// the same samples as four prepared-size blocks.
static Verdict oversizedBlockGate()
{
    const int bs = 256, big = 4 * bs, bigBlocks = 96;
    auto run = [&](int blockSize) {
        auto proc = makeProcessor();
        setParam(*proc, "WET", 20.0f); setParam(*proc, "DRY", 50.0f);
        configure(*proc, 2, 48000.0, bs);                      // prepared for 256 either way
        juce::AudioBuffer<float> buf(2, blockSize);
        juce::MidiBuffer midi;
        std::vector<float> out;
        double phase = 0.0;
        for (int b = 0; b < bigBlocks * big / blockSize; ++b) {
            const int step = b * blockSize / big;              // parameters move on the big-block grid
            if (b * blockSize % big == 0) {
                setParam(*proc, "WET", step % 2 ? 100.0f : 20.0f);
                if (step % 8 == 4) setParam(*proc, "TYPE", (float) ((step / 8) % 6));
            }
            for (int i = 0; i < blockSize; ++i) {
                const float x = 0.5f * (float) std::sin(phase);
                phase += 2.0 * juce::MathConstants<double>::pi * 300.0 / 48000.0;
                buf.setSample(0, i, x); buf.setSample(1, i, x);
            }
            proc->processBlock(buf, midi);
            for (int i = 0; i < blockSize; ++i) out.push_back(buf.getSample(0, i));
        }
        return out;
    };
    const auto small = run(bs), large = run(big);
    float diff = 0.0f, peak = 0.0f;
    bool finite = true;
    for (size_t i = 0; i < small.size(); ++i) {
        diff = std::max(diff, std::abs(small[i] - large[i]));
        peak = std::max(peak, std::abs(large[i]));
        finite = finite && std::isfinite(large[i]);
    }
    return { finite && peak > 0.01f && diff <= 1.0e-6f, "host blocks of 1024 on a processor prepared for 256 render what blocks of 256 do: max difference "
                                                        + juce::String(diff, 8) + " (<= 1e-6), peak " + juce::String(peak, 3) };
}

// No allocation inside processBlock: prepared-size blocks, odd sizes, an
// oversized block, with every control moving and TYPE going through a start,
// a ring-out, a reopen and a steal.
static Verdict allocGate()
{
   #if JUCE_MAC
    audioThread = pthread_self();
    malloc_logger = countAllocation;
    allocCount = 0;
    allocArmed = true;
    void* volatile probe = std::malloc(64);
    allocArmed = false;
    std::free(probe);
    const bool live = allocCount == 1;      // a dead hook must not read as a clean render
    allocCount = 0;

    const int bs = 256;
    auto proc = makeProcessor();
    setParam(*proc, "WET", 60.0f); setParam(*proc, "DRY", 80.0f);
    configure(*proc, 2, 48000.0, bs);
    juce::AudioBuffer<float> buffers[3] = { juce::AudioBuffer<float>(2, bs), juce::AudioBuffer<float>(2, 100), juce::AudioBuffer<float>(2, 4 * bs) };
    juce::MidiBuffer midi;
    std::mt19937 rng(5);
    std::normal_distribution<float> nd(0.0f, 0.1f);
    const int types[] = { 1, 2, 1, 5, 0, 3, 4, 2 };
    const int blocks = 600;
    for (int blk = 0; blk < blocks; ++blk) {
        if (blk % 40 == 0)  setParam(*proc, "TYPE", (float) types[(blk / 40) % 8]);
        if (blk % 23 == 0)  setParam(*proc, "SIZE", (float) ((blk * 37) % 101));
        if (blk % 31 == 0)  setParam(*proc, "DECAY", 0.5f + 1.5f * (float) ((blk * 13) % 10) / 9.0f);
        if (blk % 17 == 0)  setParam(*proc, "CHARACTER", (float) ((blk * 29) % 201) - 100.0f);
        if (blk % 53 == 0)  setParam(*proc, "LPON", (float) ((blk / 53) % 2));
        if (blk % 19 == 0)  setParam(*proc, "LPFREQ", 20.0f + (float) ((blk * 7) % 380));
        auto& buf = buffers[blk % 11 == 5 ? 2 : blk % 7 == 3 ? 1 : 0];
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < buf.getNumSamples(); ++i) buf.setSample(ch, i, nd(rng));
        allocArmed = true;
        proc->processBlock(buf, midi);
        allocArmed = false;
    }
    malloc_logger = nullptr;
    const int count = allocCount;
    return { live && count == 0, "no allocation in processBlock over " + juce::String(blocks) + " blocks (sizes 256 / 100 / 1024, every control moving): "
                                 + juce::String(count) + " allocation" + (count == 1 ? "" : "s") + (live ? "" : "; THE MALLOC HOOK IS NOT LIVE") };
   #else
    return { true, "no allocation in processBlock: not measured on this platform (macOS malloc_logger only)" };
   #endif
}

#if OSIMPLEREVERB_TEST_HOOKS
// A NaN in the playing slot's input: that block is finite (silent wet), and
// the reverb is back within the next 100 ms.
static Verdict nanGate()
{
    const int bs = 256, at = 200, blocks = 400;
    auto proc = makeProcessor();
    setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
    const auto out = renderSine(*proc, 48000.0, bs, blocks, [&](int blk) { if (blk == at) proc->testInjectNaN = true; });
    bool finite = true;
    for (float v : out) finite = finite && std::isfinite(v);
    const size_t t = (size_t) at * bs;
    const float before = peakOf(out, t - 4800, t), back = finite ? peakOf(out, t + 4800, t + 9600) : 0.0f;
    return { finite && back > 0.5f * before, "NaN into the playing slot: output " + juce::String(finite ? "stays finite" : "IS NOT FINITE")
                                             + ", wet peak " + juce::String(before, 3) + " before, " + juce::String(back, 3) + " 100-200 ms after (> half)" };
}
#endif

#if OSR_ENGINES
// ── 11. the engine headers, driven directly ──────────────────────────────────

static measure::Stereo fdnIr(const osr::FdnConfig& cfg, double fs, double sizeScale, double t60, double seconds)
{
    float maxLineMs = 1.0f, maxModMs = 0.0f;
    for (const auto& t : OSimpleReverbAudioProcessor::typePresets) {
        maxLineMs = std::max(maxLineMs, t.fdn.maxMs); maxModMs = std::max(maxModMs, t.fdn.modMs);
    }
    osr::FdnEngine engine;
    engine.prepare(fs, maxLineMs, maxModMs);
    engine.setType(cfg);
    engine.setSize(sizeScale);
    engine.setT60(t60);
    engine.reset();
    const size_t n = (size_t) (seconds * fs);
    measure::Stereo s { measure::Vec(n), measure::Vec(n) };
    for (size_t i = 0; i < n; ++i) {
        float l, r;
        const float x = i == 0 ? 1.0f : 0.0f;
        engine.process(x, x, l, r);
        s.l[i] = l; s.r[i] = r;
    }
    return s;
}

static void engineGates()
{
    juce::ScopedNoDenormals noDenormals;
    const auto& presets = OSimpleReverbAudioProcessor::typePresets;

    // -- building blocks --
    {
        double worstDb = 0.0;
        for (auto lg : { std::pair<int, float> { 229, 0.75f }, { 611, 0.625f }, { 43, 0.75f } }) {
            osr::Allpass ap;
            ap.prepare(1024);
            ap.set(lg.first, lg.second);
            measure::Vec ir(96000);
            for (size_t i = 0; i < ir.size(); ++i) ir[i] = ap.process(i == 0 ? 1.0f : 0.0f);
            for (double f : { 60.0, 317.0, 1000.0, 3170.0, 10000.0, 19000.0 })
                worstDb = std::max(worstDb, std::abs(10.0 * std::log10(measure::goertzel(ir, f, 48000.0, 0, ir.size()))));
        }
        check(worstDb < 0.01, "allpass magnitude is flat: worst " + juce::String(worstDb, 5) + " dB from 0 (3 lengths x 6 frequencies, < 0.01)");

        // Read first, push after: at(D) is D samples of delay; a fractional
        // read is the analytic sine at that delay.
        osr::RingDelay line;
        line.prepare(64);
        double errWhole = 0.0, errFrac = 0.0;
        const double w = 2.0 * measure::kPi * 1000.0 / 48000.0, d = 23.37;
        for (int n = 0; n < 4800; ++n) {
            const double whole = line.at(23), frac = line.readLagrange(d);
            line.push((float) std::sin(w * n));
            if (n > 100) {
                errWhole = std::max(errWhole, std::abs(whole - std::sin(w * (n - 23))));
                errFrac = std::max(errFrac, std::abs(frac - std::sin(w * (n - d))));
            }
        }
        check(errWhole < 1.0e-6 && errFrac < 1.0e-4, "ring delay: at(23) is 23 samples (max err " + juce::String(errWhole, 8)
                                                    + "), Lagrange read at 23.37 is the sine at that delay (max err " + juce::String(errFrac, 7) + " < 1e-4)");

        // RT60 -> gain: the algebra round-trips, and a loop built with that
        // gain decays in that time.
        const double g = osr::rt60ToGain(1000.0, 48000.0, 2.0), back = osr::gainToRt60(g, 1000.0, 48000.0);
        osr::RingDelay loop;
        loop.prepare(1000);
        measure::Vec comb((size_t) (3.1 * 48000.0));
        for (size_t i = 0; i < comb.size(); ++i) {
            const float y = loop.at(1000);
            loop.push((i == 0 ? 1.0f : 0.0f) + (float) g * y);
            comb[i] = y;
        }
        const double measured = measure::rt60(comb, 48000.0);
        check(std::abs(back - 2.0) < 1.0e-9 && std::abs(measured / 2.0 - 1.0) < 0.01,
              "rt60ToGain: round-trips to " + juce::String(back, 6) + " s, and a 1000-sample loop at that gain decays in "
              + juce::String(measured, 3) + " s (2.0, within 1 %)");

        // The shelf, solved at 1 kHz, has the target gain AT 1 kHz.
        double worstShelf = 0.0;
        for (double fs : { 44100.0, 48000.0, 96000.0 }) {
            osr::OnePole lp;
            lp.setCutoff(4000.0, fs);
            const double gHi = 0.70, gTarget = 0.90, gLo = osr::solveShelfLow(gHi, gTarget, osr::shelfReference(lp.a, 1000.0, fs));
            measure::Vec in((size_t) fs), out(in.size());
            for (size_t i = 0; i < in.size(); ++i) {
                const float x = (float) std::sin(2.0 * measure::kPi * 1000.0 * (double) i / fs);
                in[i] = x;
                out[i] = (float) gHi * x + (float) (gLo - gHi) * lp.lowPass(x);
            }
            const size_t from = in.size() / 2;
            worstShelf = std::max(worstShelf, std::abs(std::sqrt(measure::goertzel(out, 1000.0, fs, from, out.size())
                                                                 / measure::goertzel(in, 1000.0, fs, from, in.size())) / gTarget - 1.0));
        }
        check(worstShelf < 1.0e-3, "mid-band shelf: gain at 1 kHz is the target within " + juce::String(100.0 * worstShelf, 4) + " % (44.1 / 48 / 96 kHz, < 0.1)");

        // The glide: two poles of 125 ms, so 3 / e^2 of the way is left after
        // 250 ms at any rate; it starts with no velocity; it lands exactly.
        double worstGlide = 0.0, firstStep = 0.0;
        bool lands = true;
        for (double fs : { 44100.0, 48000.0, 96000.0 }) {
            osr::GlideLength len;
            len.set(1000.0); len.land(); len.set(2000.0);
            const double c = osr::slewCoefficient(0.5 * osr::kSizeGlideSeconds, fs);
            len.tick(c);
            firstStep = std::max(firstStep, (len.current - 1000.0) * fs);     // samples per second, first tick
            for (int i = 1; i < (int) (osr::kSizeGlideSeconds * fs); ++i) len.tick(c);
            worstGlide = std::max(worstGlide, std::abs((2000.0 - len.current) / 1000.0 - 3.0 * std::exp(-2.0)));
            for (int i = 0; i < (int) (5.0 * fs) && len.moving; ++i) len.tick(c);
            lands = lands && ! len.moving && len.current == 2000.0;
        }
        // A single pole would start at 1000 / 0.25 = 4000 samples per second.
        check(worstGlide < 0.005 && lands && firstStep < 40.0,
              "glide: 3/e^2 left after 250 ms at 44.1 / 48 / 96 kHz (worst off by " + juce::String(worstGlide, 5) + "), starts at "
              + juce::String(firstStep, 2) + " of 1000 samples per second (< 40), and lands on its target");
    }

    // -- early reflections --
    {
        const double fs = 48000.0;
        auto tapsOf = [&](double scale, bool glideThere, std::vector<int>& timesL, std::vector<int>& timesR) {
            osr::EarlyReflections er;
            er.prepare(fs, 57.5f);
            er.setType(23.0f);
            er.setSize(glideThere ? 1.0 : scale);
            er.reset();
            if (glideThere) {
                er.setSize(scale);
                for (int i = 0; i < (int) (4.0 * fs); ++i) { float l, r; er.process(0.0f, 0.0f, l, r); }
            }
            bool asStated = true;
            measure::Vec l(8192), r(8192);
            for (size_t i = 0; i < l.size(); ++i) {
                float a, b;
                er.process(i == 0 ? 1.0f : 0.0f, i == 0 ? 1.0f : 0.0f, a, b);
                l[i] = a; r[i] = b;
            }
            for (int side = 0; side < 2; ++side) {
                const auto& ir = side == 0 ? l : r;
                auto& times = side == 0 ? timesL : timesR;
                for (size_t i = 0; i < ir.size(); ++i) if (ir[i] != 0.0) times.push_back((int) i);
                asStated = asStated && times.size() == (size_t) osr::EarlyReflections::kTaps;
                for (int k = 0; k < osr::EarlyReflections::kTaps && asStated; ++k) {
                    const int want = (int) std::lround((side == 0 ? osr::EarlyReflections::kTimesL : osr::EarlyReflections::kTimesR)[(size_t) k] * 23.0 * scale * 0.001 * fs);
                    asStated = times[(size_t) k] == want && std::abs(ir[(size_t) want] - er.tapGain(side, k)) < 1.0e-6
                               && (ir[(size_t) want] > 0.0) == (k % 2 == 0);
                }
            }
            return asStated;
        };
        std::vector<int> l1, r1, l2, r2, l3, r3;
        const bool base = tapsOf(1.0, false, l1, r1), big = tapsOf(2.0, false, l2, r2), glided = tapsOf(2.0, true, l3, r3);
        bool differ = true;
        for (int a : l1) for (int b : r1) differ = differ && std::abs(a - b) > 4;     // 0.1 ms
        check(base && differ, "early reflections: 8 taps a side at the stated times and gains, alternating sign, no L tap within 0.1 ms of an R tap ("
                              + juce::String(l1.empty() ? 0.0 : l1.front() / 48.0, 2) + ".." + juce::String(l1.empty() ? 0.0 : l1.back() / 48.0, 2) + " ms at a 23 ms span)");
        check(big && glided && l2 == l3 && r2 == r3 && ! l2.empty() && ! l1.empty() && l2.back() == 2 * l1.back(),
              "early reflections: tap times follow SIZE (last tap " + juce::String(l1.empty() ? 0.0 : l1.back() / 48.0, 2) + " -> "
              + juce::String(l2.empty() ? 0.0 : l2.back() / 48.0, 2) + " ms at x2), and a glide lands on the same taps as a reset");
    }

    // -- FDN: decay time against target --
    double worstCorr = 0.0;
    int hfNotShorter = 0, points = 0;
    for (int type : kFdnTypes) {
        const auto& cfg = presets[type].fdn;
        double worst = 0.0;
        juce::String where;
        for (double fs : { 44100.0, 48000.0, 96000.0 })
            for (double decay : { 0.5, 1.0, 2.0 })
                for (double size : { 0.5, 1.0, 2.0 }) {
                    const double t60 = presets[type].baseT60 * decay;
                    const auto ir = fdnIr(cfg, fs, size, t60, 1.35 * t60 + 0.4);
                    const double mid = measure::midRt60(ir.l, fs), err = mid / t60 - 1.0;
                    if (std::abs(err) > std::abs(worst)) {
                        worst = err;
                        where = juce::String(fs / 1000.0, 1) + " kHz, DECAY " + juce::String(decay, 1) + "x, size x" + juce::String(size, 1);
                    }
                    if (measure::hfRt60(ir.l, fs) >= mid) ++hfNotShorter;
                    worstCorr = std::max(worstCorr, std::abs(measure::correlation(ir.l, ir.r, (size_t) (0.25 * t60 * fs), (size_t) (0.75 * t60 * fs))));
                    ++points;
                }
        check(std::abs(worst) <= kRt60Tolerance, juce::String("FDN ") + kTypeNames[type] + ": mid RT60 within 10 % of base x DECAY at 27 points "
                                                 "(44.1 / 48 / 96 kHz x DECAY 0.5 / 1 / 2 x size x0.5 / x1 / x2); worst "
                                                 + juce::String(100.0 * worst, 1) + " % at " + where);
    }
    check(hfNotShorter == 0, "FDN: the 8 kHz octave decays faster than the mid band at all " + juce::String(points) + " points ("
                             + juce::String(hfNotShorter) + " do not)");
    check(worstCorr < 0.3, "FDN: late-tail L/R correlation at all " + juce::String(points) + " points, worst |r| " + juce::String(worstCorr, 3) + " (< 0.3)");

    // -- FDN: the four delay sets do not share resonances --
    {
        measure::Vec spectra[4];
        for (int k = 0; k < 4; ++k)
            spectra[k] = measure::tailSpectrumDb(fdnIr(presets[kFdnTypes[k]].fdn, 48000.0, 1.0, 3.0, 2.6).l, 48000.0, 0.5, 2.5);
        double worst = 0.0;
        for (int a = 0; a < 4; ++a)
            for (int b = a + 1; b < 4; ++b) worst = std::max(worst, std::abs(measure::pearson(spectra[a], spectra[b])));
        check(worst < 0.3, "FDN: tail spectra of the four delay sets at one decay time, worst pair |r| " + juce::String(worst, 2) + " (< 0.3)");
    }

    // -- FDN: the worst case for stability and for capacity --
    {
        const double fs = 192000.0, t60 = presets[5].baseT60 * 2.0;
        const auto ir = fdnIr(presets[5].fdn, fs, 2.0, t60, 1.35 * t60 + 0.4);
        bool finite = true;
        for (size_t i = 0; i < ir.l.size(); ++i) finite = finite && std::isfinite(ir.l[i]) && std::isfinite(ir.r[i]);
        const double mid = finite ? measure::midRt60(ir.l, fs) : 0.0;
        check(finite && std::abs(mid / t60 - 1.0) <= kRt60Tolerance, "FDN Ambient at 192 kHz, DECAY 2.0x, size x2 (the longest lines there are): finite, mid RT60 "
                                                                     + juce::String(mid, 2) + " s (14.0, within 10 %)");
    }

    // -- FDN: the same input gives the same output --
    {
        auto render = [&]() {
            float maxLineMs = 1.0f, maxModMs = 0.0f;
            for (const auto& t : presets) { maxLineMs = std::max(maxLineMs, t.fdn.maxMs); maxModMs = std::max(maxModMs, t.fdn.modMs); }
            osr::FdnEngine engine;
            engine.prepare(48000.0, maxLineMs, maxModMs);
            engine.setType(presets[2].fdn);
            engine.setSize(1.0);
            engine.setT60(3.0);
            engine.reset();
            std::mt19937 rng(4);
            std::normal_distribution<float> nd(0.0f, 0.1f);
            std::vector<float> out;
            for (int i = 0; i < 96000; ++i) {
                float l, r;
                const float x = i < 24000 ? nd(rng) : 0.0f;
                if (i == 30000) engine.setSize(1.7);        // a glide and the modulation are both in the render
                engine.process(x, x, l, r);
                out.push_back(l); out.push_back(r);
            }
            return out;
        };
        check(render() == render(), "FDN: two renders of the same input, modulation and a SIZE glide included, are the same samples");
    }
}
#endif // OSR_ENGINES

#if OSIMPLEREVERB_TEST_HOOKS
// ── --mutants ────────────────────────────────────────────────────────────────
template <typename Gate>
static void mustFail(Mutant mutant, const char* broken, Gate gate)
{
    activeMutant = mutant;
    const Verdict v = gate();
    activeMutant = Mutant::none;
    check(! v.ok, juce::String("with ") + broken + ", this gate fails -> " + v.text);
}

static int runMutants()
{
    const float ref = hfBurst("WET", 20.0f, 100.0f).db;
    const float slow = hfBurst("TYPE", 1.0f, 0.0f).db;
    mustFail(Mutant::decayDead,   "DECAY not reaching the engine",        [] { return rt60DecayGate(1); });
    mustFail(Mutant::sizeIsDecay, "SIZE scaling the decay time",          [] { return rt60SizeGate(1); });
    mustFail(Mutant::sizeDead,    "SIZE not reaching the engine",         [] { return sizeStructureGate(1); });
    mustFail(Mutant::sameDelays,  "every type on Hall's delay set",       [] { return tailSpectrumGate(); });
    mustFail(Mutant::earlyMuted,  "early reflections muted",              [] { return earlyTapsGate(1); });
    mustFail(Mutant::monoTail,    "right = left",                         [] { return stereoGate(); });
    mustFail(Mutant::noRingOut,   "the old slot cleared on a TYPE change", [] { return ringOutGate(); });
    mustFail(Mutant::noNanGuard,  "no NaN guard",                         [] { return nanGate(); });
    mustFail(Mutant::noChunking,  "an oversized block processed whole",   [] { return allocGate(); });
    mustFail(Mutant::noChunking,  "an oversized block processed whole",   [] { return oversizedBlockGate(); });
    mustFail(Mutant::noGlide,     "SIZE landing at once",                 [ref] { return glideGate("SIZE", 0.0f, 100.0f, ref, kSizeGlideMarginDb); });
    mustFail(Mutant::noGlide,     "DECAY landing at once",                [ref] { return glideGate("DECAY", 0.5f, 2.0f, ref, kDecayGlideMarginDb); });
    mustFail(Mutant::hardSteal,   "a sounding slot taken back without a fade", [slow] { return rapidSwitchGate({ 0, 1, 2 }, slow, kRapidMarginDb); });
    mustFail(Mutant::hardSteal,   "a sounding slot taken back without a fade", [] { return presetSweepGate(hfBurst("WET", 20.0f, 100.0f, 0.0f, -1, 8000.0f).db, kSweepMarginDb); });
    std::printf("\n%d of %d mutants caught\n", passes, passes + failures);
    return failures == 0 ? 0 : 1;
}
#endif

// ── --levels ─────────────────────────────────────────────────────────────────
static int printLevels()
{
    std::printf("Wet loudness re input, K-weighted pink noise, WET 100 / DRY 0 (dB).\n\n"
                "| Type | stereo | mono | DECAY 0.5x | DECAY 2.0x | SIZE 0 | SIZE 100 |\n|---|---|---|---|---|---|---|\n");
    for (int t = 0; t < 6; ++t)
        std::printf("| %s | %+.2f | %+.2f | %+.2f | %+.2f | %+.2f | %+.2f |\n", kTypeNames[t], typeLoudnessDb(t), typeLoudnessDb(t, 1),
                    typeLoudnessDb(t, 2, 0.5f), typeLoudnessDb(t, 2, 2.0f), typeLoudnessDb(t, 2, 1.0f, 0.0f), typeLoudnessDb(t, 2, 1.0f, 100.0f));
    return 0;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::String mode(argc > 1 ? argv[1] : "");
    if (mode == "--baseline") return printBaseline();
    if (mode == "--write-fixture") return writeFixture();
    if (mode == "--levels") return printLevels();
   #if OSIMPLEREVERB_TEST_HOOKS
    if (mode == "--mutants") return runMutants();
   #endif
    if (mode.isNotEmpty()) { std::printf("unknown mode %s\n", mode.toRawUTF8()); return 2; }

    {
        auto proc = makeProcessor();   // locate the Factory folder, then force a rewrite
        auto dir = proc->presetManager.getFactoryPresetsDirectory();
        dir.getChildFile(".factory-version").deleteFile();
        dir.getChildFile("Spring - Dub Echo.json").replaceWithText("{}");   // stand-in for a v1.13.1 file
    }

    // ── 1. Factory presets recall their TYPE ─────────────────────────────────
    {
        auto proc = makeProcessor();
        if (proc == nullptr) { std::printf("FAIL: not an OSimpleReverbAudioProcessor\n"); return 1; }

        const juce::StringArray typeNames { "Booth", "Room", "Hall", "Spring", "Plate", "Ambient" };
        auto* typeParam = dynamic_cast<juce::AudioParameterChoice*>(proc->parameters.getParameter("TYPE"));
        auto* raw = proc->parameters.getRawParameterValue("TYPE");

        int factoryCount = 0, wrong = 0;
        for (const auto& name : proc->presetManager.getPresetList()) {
            if (! proc->presetManager.isFactoryPreset(name))
                continue;
            const int expected = typeNames.indexOf(name.upToFirstOccurrenceOf(" - ", false, false));
            if (expected < 0)
                continue;
            ++factoryCount;
            proc->presetManager.loadPreset(name);
            const int dsp  = juce::roundToInt(raw->load());
            const int host = typeParam->getIndex();
            const int page = (int) std::floor(static_cast<juce::AudioProcessorParameter*>(typeParam)->getValue() * 5.0f + 0.5f);  // JS Math.round
            if (dsp != expected || host != expected || page != expected) {
                ++wrong;
                std::printf("  %s: expected %d, dsp %d, host %d, page %d\n",
                            name.toRawUTF8(), expected, dsp, host, page);
            }
        }
        check(factoryCount == 48, "48 factory presets found (" + juce::String(factoryCount) + ")");
        check(wrong == 0, "every factory preset recalls its named TYPE in DSP, host and page ("
                          + juce::String(wrong) + " wrong)");
    }

    // ── 2. CHARACTER moves are click-free ────────────────────────────────────
    // Click metric: max |second difference| of the wet output (a step or a
    // filter-state blow-up spikes it; a 300 Hz sine barely registers). Toggling
    // CHARACTER every 8 blocks is compared against holding it. v1.10.0 measured
    // 2154x (-80<->-10), 762x (-80<->0) and 332x (10<->60).
    {
        auto maxD2 = [](float a, float b) {
            auto proc = makeProcessor();
            setParam(*proc, "TYPE", 1.0f); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
            setParam(*proc, "CHARACTER", a);
            const auto out = renderSine(*proc, 48000.0, 256, 600, [&](int blk) {
                setParam(*proc, "CHARACTER", (blk / 8) % 2 ? b : a);
            });
            float m = 0.0f;
            for (size_t i = 100 * 256; i < out.size(); ++i)
                m = std::max(m, std::abs(out[i] - 2.0f * out[i - 1] + out[i - 2]));
            return m;
        };
        const float held = juce::jmax(maxD2(-80.0f, -80.0f), maxD2(0.0f, 0.0f), maxD2(60.0f, 60.0f));
        struct Case { float a, b, limit; };
        for (auto c : { Case { -80.0f, -10.0f, 1.5f }, Case { -80.0f, 0.0f, 1.5f },
                        Case { -30.0f, 30.0f, 10.0f }, Case { 10.0f, 60.0f, 10.0f } }) {
            const float ratio = maxD2(c.a, c.b) / held;
            check(ratio < c.limit, "CHARACTER " + juce::String(c.a, 0) + "<->" + juce::String(c.b, 0)
                                   + " click ratio " + juce::String(ratio, 2) + " < " + juce::String(c.limit, 1));
        }
    }

    // ── 3. Every type at 44.1 / 96 / 192 kHz; an oversized host block ─────────
    for (double sr : { 44100.0, 96000.0, 192000.0 }) {
        bool finite = true;
        float quietest = 1.0e9f;
        for (int type = 0; type < 6; ++type) {
            auto proc = makeProcessor();
            setParam(*proc, "TYPE", (float) type); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
            setParam(*proc, "DECAY", 2.0f); setParam(*proc, "SIZE", 100.0f);
            const auto out = renderSine(*proc, sr, 512, (int) (2.0 * sr) / 512, [](int) {});
            float peak = 0.0f;
            for (float x : out) { finite = finite && std::isfinite(x); peak = std::max(peak, std::abs(x)); }
            quietest = std::min(quietest, peak);
        }
        check(finite && quietest > 0.01f, "all six types @ " + juce::String(sr / 1000.0, 1) + " kHz, DECAY 2.0x, SIZE 100: finite, non-silent (quietest peak "
                                          + juce::String(quietest, 4) + ")");
    }
    check(oversizedBlockGate());

    // ── 4. VU peak is held until read ────────────────────────────────────────
    {
        auto proc = makeProcessor();
        setParam(*proc, "WET", 0.0f); setParam(*proc, "DRY", 100.0f);
        proc->outputPeak.exchange(0.0f);
        renderSine(*proc, 48000.0, 256, 20, [](int) {});                   // loud blocks...
        juce::AudioBuffer<float> silence(2, 256); silence.clear();
        juce::MidiBuffer midi;
        for (int i = 0; i < 400; ++i) { silence.clear(); proc->processBlock(silence, midi); }  // ...then 2 s of silence
        const float held = proc->outputPeak.exchange(0.0f);
        const float after = proc->outputPeak.load();
        check(held > 0.45f, "peak from earlier loud blocks survives later silent blocks (" + juce::String(held, 3) + ")");
        check(after == 0.0f, "reading the peak clears it");
    }

    // ── 5. switches do not click (HF burst) ────────────────────────────────────
    {
        const float ref = hfBurst("WET", 20.0f, 100.0f).db;   // a smoothed 20 ms move
        std::printf("  (reference: WET 20<->100 burst %.1f dB)\n", ref);
        struct Case { const char* id; float a, b, lpOn, margin; };
        for (auto c : { Case { "LPON", 1.0f, 0.0f, 0.0f, 6.0f },
                        Case { "LPFREQ", 20.0f, 400.0f, 1.0f, 10.0f },
                        Case { "CHARACTER", 0.3f, 0.7f, 0.0f, 3.0f },
                        Case { "CHARACTER", 10.0f, 60.0f, 0.0f, 3.0f },
                        Case { "CHARACTER", -80.0f, 0.0f, 0.0f, 3.0f } }) {
            const float db = hfBurst(c.id, c.a, c.b, c.lpOn).db;
            check(db < ref + c.margin, juce::String(c.id) + " " + juce::String(c.a, 1) + "<->" + juce::String(c.b, 1)
                                       + " HF burst " + juce::String(db, 1) + " dB < ref + " + juce::String(c.margin, 0));
        }
        // SIZE glides and DECAY slews. The glide bends the tail's pitch, which
        // is why this is the HF-burst metric and not a second difference: a
        // bend raises the second difference by itself (4.3x at a 250 ms glide).
        check(glideGate("SIZE", 0.0f, 100.0f, ref, kSizeGlideMarginDb));
        check(glideGate("DECAY", 0.5f, 2.0f, ref, kDecayGlideMarginDb));

        float worst = 0.0f, slowDb = -200.0f;
        const int pairs[][2] = { { 1, 2 }, { 1, 3 }, { 2, 4 }, { 0, 5 }, { 1, 0 }, { 3, 4 } };
        for (const auto& pr : pairs) {
            const auto b = hfBurst("TYPE", (float) pr[0], (float) pr[1]);
            worst = std::max(worst, b.ratio);
            slowDb = std::max(slowDb, b.db);
        }
        check(worst < 2.0f, "TYPE switch HF burst over steady HF " + juce::String(worst, 2) + "x < 2 (worst of 6 pairs, "
                            + juce::String(slowDb, 1) + " dB)");
        // Two types alternate between the two slots (each reopens its own
        // ringing tail); three force the older slot to be taken back.
        check(rapidSwitchGate({ 0, 1 }, slowDb, kRapidMarginDb));
        check(rapidSwitchGate({ 0, 1, 2 }, slowDb, kRapidMarginDb));
        check(presetSweepGate(hfBurst("WET", 20.0f, 100.0f, 0.0f, -1, 8000.0f).db, kSweepMarginDb));
    }

    // ── 6. types are level-matched, in stereo and on a mono bus ───────────────
    for (int channels : { 2, 1 }) {
        double lo = 1.0e9, hi = -1.0e9;
        for (int t = 0; t < 6; ++t) { const double l = typeLoudnessDb(t, channels); lo = std::min(lo, l); hi = std::max(hi, l); }
        check(hi - lo <= 1.0, juce::String(channels == 2 ? "stereo" : "mono") + ": type loudness spread " + juce::String(hi - lo, 2)
                              + " dB <= 1 (K-weighted pink, " + juce::String(lo, 1) + ".." + juce::String(hi, 1) + " dB re input)");
    }

    // ── 7. the shifter is an octave ───────────────────────────────────────────
    {
        OctaveUpShifter oct;
        oct.prepare(48000.0);
        std::vector<float> y;
        for (int i = 0; i < 48000; ++i) y.push_back(oct.process(0.5f * (float) std::sin(juce::MathConstants<double>::twoPi * 1000.0 * i / 48000.0)));
        const double octDb = 10.0 * std::log10(goertzel(y, 2000.0, 48000.0, 4800) / goertzel(y, 1000.0, 48000.0, 4800));
        check(octDb > 20.0, "octave shifter: 2 kHz over 1 kHz " + juce::String(octDb, 1) + " dB > 20");
        pend("Plate shimmer blooms in the tail: 600/300 Hz in the tail exceeds the ratio at onset by >= 10 dB, Room shows none (Dattorro tank: stage 2)");
    }

    // ── 9. factory bank shape and level ───────────────────────────────────────
    // Each factory preset's output re its input (K-weighted pink noise), so a
    // step through the bank does not jump in level. Send presets go on an aux
    // bus with no dry beside them; they are listed but not held to the ceiling.
    {
        auto proc = makeProcessor();
        const juce::StringArray typeNames { "Booth", "Room", "Hall", "Spring", "Plate", "Ambient" };
        int perType[6] = {}, sends = 0, badSends = 0;
        double lo = 1.0e9, hi = -1.0e9;
        juce::String loName, hiName;
        for (const auto& name : proc->presetManager.getPresetList()) {
            if (! proc->presetManager.isFactoryPreset(name))
                continue;
            const int t = typeNames.indexOf(name.upToFirstOccurrenceOf(" - ", false, false));
            if (t >= 0) ++perType[t];
            auto fresh = makeProcessor();
            fresh->presetManager.loadPreset(name);
            const double db = loudnessDb(*fresh);
            auto v = [&](const char* id) { return fresh->parameters.getParameter(id)->convertFrom0to1(
                                                      fresh->parameters.getParameter(id)->getValue()); };
            std::printf("  %-30s %+5.1f dB   char %+5.0f wet %5.1f dry %5.1f decay %.2fx size %3.0f lowcut %s %3.0f Hz\n",
                        name.toRawUTF8(), db, v("CHARACTER"), v("WET"), v("DRY"), v("DECAY"), v("SIZE"),
                        v("LPON") >= 0.5f ? "on " : "off", v("LPFREQ"));
            if (name.endsWith(" - Send")) {
                ++sends;
                if (v("WET") != 100.0f || v("DRY") != 0.0f) ++badSends;
                continue;
            }
            if (db < lo) { lo = db; loName = name; }
            if (db > hi) { hi = db; hiName = name; }
        }
        bool eight = true;
        for (int n : perType) eight = eight && n == 8;
        check(eight, "8 factory presets per type");
        check(sends == 6 && badSends == 0, "one Send per type, each WET 100 / DRY 0 (" + juce::String(sends)
                                           + " sends, " + juce::String(badSends) + " wrong)");
        check(! proc->presetManager.getFactoryPresetsDirectory().getChildFile("Spring - Dub Echo.json").exists(),
              "renamed preset's stale file removed from the installed bank");
        pend("insert presets " + juce::String(lo, 1) + " (" + loName + ") .. " + juce::String(hi, 1) + " dB (" + hiName
             + ") re input, ceiling +5.0 (the bank is re-voiced for the new engines in stage 4)");
    }

    // ── 10. the measurer, and the v1.14.0 state ───────────────────────────────
    {
        // The RT60 gates are only as good as the measurer. A tone decaying
        // 60 dB in a known time has no statistics of its own and must read that
        // time. Decaying noise is what a reverb tail looks like: one
        // realisation scatters about the true value (the spread printed here
        // is the floor under any RT60 tolerance - about 3 % at 0.2 s, 1 % at
        // 3 s), so it is the MEAN over seeds that must sit on the target.
        for (double sr : { 44100.0, 48000.0, 96000.0 })
            for (double t60 : { 0.2, 0.4, 3.0, 14.0 }) {
                if (sr != 48000.0 && t60 != 3.0) continue;
                measure::Vec mid((size_t) ((1.35 * t60 + 0.4) * sr)), hf(mid.size());
                for (size_t i = 0; i < mid.size(); ++i) {
                    const double env = std::pow(10.0, -3.0 * (double) i / (sr * t60));
                    mid[i] = env * std::sin(2.0 * measure::kPi * 1000.0 * (double) i / sr);
                    hf[i]  = env * std::sin(2.0 * measure::kPi * 8000.0 * (double) i / sr);
                }
                const double toneMid = 100.0 * (measure::midRt60(mid, sr) / t60 - 1.0), toneHf = 100.0 * (measure::hfRt60(hf, sr) / t60 - 1.0);
                check(std::abs(toneMid) <= 1.0 && std::abs(toneHf) <= 1.0,
                      "measurer: decaying tone, T60 " + juce::String(t60, 1) + " s @ " + juce::String(sr / 1000.0, 1) + " kHz reads mid "
                      + juce::String(toneMid, 2) + " %, 8 kHz " + juce::String(toneHf, 2) + " % (within 1)");

                const int seeds = t60 > 5.0 ? 8 : 24;
                double sum = 0, sumSq = 0, sumHf = 0;
                for (int seed = 1; seed <= seeds; ++seed) {
                    const auto x = measure::decayingNoise(sr, t60, 1.35 * t60 + 0.4, (unsigned) seed);
                    const double e = 100.0 * (measure::midRt60(x, sr) / t60 - 1.0);
                    sum += e; sumSq += e * e; sumHf += 100.0 * (measure::hfRt60(x, sr) / t60 - 1.0);
                }
                const double mean = sum / seeds, sd = std::sqrt(std::max(0.0, sumSq / seeds - mean * mean)), meanHf = sumHf / seeds;
                check(std::abs(mean) <= 2.0 && std::abs(meanHf) <= 2.0,
                      "measurer: decaying noise, T60 " + juce::String(t60, 1) + " s @ " + juce::String(sr / 1000.0, 1) + " kHz reads mid "
                      + juce::String(mean, 2) + " %, 8 kHz " + juce::String(meanHf, 2) + " % (mean of " + juce::String(seeds)
                      + " seeds, within 2; one seed scatters " + juce::String(sd, 1) + " %)");
            }

        // Two independent decaying noises are uncorrelated and fully mixed; a
        // signal against itself correlates at 1. An impulse train is sparse.
        const auto a = measure::decayingNoise(48000.0, 3.0, 3.0, 11), b = measure::decayingNoise(48000.0, 3.0, 3.0, 12);
        const double rab = measure::correlation(a, b, 36000, 108000), raa = measure::correlation(a, a, 36000, 108000);
        const double dense = measure::echoDensityAt(a, 48000.0, 4800);
        measure::Vec sparse(48000, 0.0);
        for (size_t i = 0; i < sparse.size(); i += 480) sparse[i] = 1.0;
        const double thin = measure::echoDensityAt(sparse, 48000.0, 4800);
        check(std::abs(rab) < 0.05 && raa > 0.999, "measurer: correlation of independent noises " + juce::String(rab, 3)
                                                   + " (|r| < 0.05), of a signal with itself " + juce::String(raa, 3));
        check(dense > 0.9 && dense < 1.1 && thin < 0.1, "measurer: echo density of Gaussian noise " + juce::String(dense, 2)
                                                         + " (0.9..1.1), of a 10 ms impulse train " + juce::String(thin, 2) + " (< 0.1)");

        // A tap at a known time is found there, and a band's arrival is its delay.
        measure::Vec taps(48000, 0.0);
        taps[100] = 1.0; taps[100 + 480] = -0.5; taps[100 + 1200] = 0.25;
        const auto found = measure::earlyTaps(taps, 48000.0, measure::onsetIndex(taps));
        check(measure::onsetIndex(taps) == 100 && found.size() == 3 && std::abs(found[1].ms - 10.0) < 0.01 && found[1].amp < 0
              && std::abs(found[2].ms - 25.0) < 0.01,
              "measurer: taps at 0 / 10 / 25 ms found (" + tapList(found) + ")");
        const double arrive = measure::bandArrivalMs(taps, 48000.0, 0, 1000.0);
        check(std::abs(arrive - 100.0 / 48.0) < 0.05, "measurer: 1 kHz band of an impulse delayed 2.08 ms arrives at "
                                                       + juce::String(arrive, 2) + " ms");
        // A chirp: a 3 kHz burst 5 ms behind a 1 kHz burst reads about 5 ms later;
        // an impulse (every band at once) reads none.
        measure::Vec lo(9600, 0.0), hi(9600, 0.0), chirp(9600);
        lo[480] = 1.0; hi[720] = 1.0;
        lo = measure::band(lo, 48000.0, 700.0, 1400.0); hi = measure::band(hi, 48000.0, 2200.0, 4000.0);
        for (size_t i = 0; i < chirp.size(); ++i) chirp[i] = lo[i] + hi[i];
        const double lag = measure::bandArrivalMs(chirp, 48000.0, 0, 3000.0) - measure::bandArrivalMs(chirp, 48000.0, 0, 1000.0);
        const double none = measure::bandArrivalMs(taps, 48000.0, 0, 3000.0) - measure::bandArrivalMs(taps, 48000.0, 0, 1000.0);
        check(lag > 3.5 && lag < 6.5 && std::abs(none) < 0.5, "measurer: 3 kHz burst 5 ms behind a 1 kHz burst reads "
                                                               + juce::String(lag, 2) + " ms later (3.5..6.5); an impulse reads "
                                                               + juce::String(none, 2) + " ms (|x| < 0.5)");

        // The v1.14.0 state loads with the values it was written with.
        const auto dir = fixtureDir();
        const auto bin = dir.getChildFile("state-v1.14.0.bin");
        juce::StringArray lines;
        dir.getChildFile("state-v1.14.0.tsv").readLines(lines);
        juce::MemoryBlock blob;
        int checked = 0, wrong = 0;
        if (bin.loadFileAsData(blob) && blob.getSize() > 0) {
            auto proc = makeProcessor();
            proc->setStateInformation(blob.getData(), (int) blob.getSize());
            for (const auto& line : lines) {
                if (line.startsWithChar('#') || line.trim().isEmpty()) continue;
                const auto cols = juce::StringArray::fromTokens(line, "\t", "");
                const auto id = cols[0];
                ++checked;
                if (id == "uiLanguage") {
                    if (OSimpleReverbAudioProcessor::languageCode(proc->uiLanguage.load()) != cols[1]) ++wrong;
                } else if (proc->parameters.getParameter(id) == nullptr
                           || std::abs(paramValue(*proc, id.toRawUTF8()) - cols[1].getFloatValue()) > 1.0e-3f) {
                    ++wrong;
                    std::printf("  %s: fixture %s, loaded %s\n", id.toRawUTF8(), cols[1].toRawUTF8(),
                                proc->parameters.getParameter(id) ? juce::String(paramValue(*proc, id.toRawUTF8()), 4).toRawUTF8() : "no such parameter");
                }
            }
        }
        check(checked == 9 && wrong == 0, "v1.14.0 state blob loads with its 8 parameter values and UI language ("
                                          + juce::String(checked) + " checked, " + juce::String(wrong) + " wrong)");
    }

    // ── 11. the engine headers, driven directly ───────────────────────────────
   #if OSR_ENGINES
    engineGates();
   #endif

    // ── 12. the reverb through processBlock ───────────────────────────────────
    for (int type = 0; type < 6; ++type) {
        if (type == 3 || type == 4) {
            pend(juce::String(kTypeNames[type]) + " RT60 = base x DECAY, and holding across SIZE (its own engine: stage "
                 + (type == 4 ? "2" : "3") + ")");
            continue;
        }
        check(rt60DecayGate(type));
        check(rt60SizeGate(type));
        check(sizeStructureGate(type));
        check(earlyTapsGate(type));
    }
    check(tailSpectrumGate());
    check(stereoGate());
    check(ringOutGate());
   #if OSIMPLEREVERB_TEST_HOOKS
    check(nanGate());
   #endif
    check(allocGate());
    pend("Spring chirp: the first echo arrives >= 2 ms later at 3 kHz than at 1 kHz, repeating at the echo time (stage 3)");
    pend("Plate does not run away: DECAY 2.0x, SIZE 0 and 100, 10 s of noise, then a monotone decay (stage 2)");

    std::printf("\n%d PASS, %d FAIL, %d PEND\n%s\n", passes, failures, pending,
                failures > 0 ? "FAILED" : pending > 0 ? "ALL PASS (with gates pending)" : "ALL PASS");
    return failures == 0 ? 0 : 1;
}
