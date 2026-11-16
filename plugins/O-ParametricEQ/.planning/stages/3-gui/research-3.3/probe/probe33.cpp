// RESEARCH-3.3 probe (scratch): analyzer tick cost, tap cost on the audio thread, Off gating,
// meter reads under host block sizes, the listen NaN fix (N1e as a gated row).
// Helpers below are copied from stages/2-dsp/verify-2.6/probe.cpp.
// Drives the real processor through its public API only (APVTS parameters, listenBand, analyzerMode,
// setAnalyzerConsumer, prepareToPlay, processBlock, processBlockBypassed, getListenState, the tap read
// side, SpectrumAnalyzer). Includes no Source/dsp header directly and none of the harness code.
// Helpers (render, noise, bitEq, countDiff, gridK, lenN) are copied from verify-2.5/probe.cpp.
#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

using Proc = OuariconParametricEQAudioProcessor;
using Buf  = juce::AudioBuffer<float>;
using Fn   = std::function<void (Proc&)>;

static int nPass = 0, nFail = 0;
static const double kPi = 3.14159265358979323846;

static void row (bool ok, const char* id, const char* name, const juce::String& detail)
{
    std::printf ("[%s] %-7s %-56s %s\n", ok ? "PASS" : "FAIL", id, name, detail.toRawUTF8());
    std::fflush (stdout);
    (ok ? nPass : nFail)++;
}
static void info (const char* id, const char* name, const juce::String& detail)
{
    std::printf ("[INFO] %-7s %-56s %s\n", id, name, detail.toRawUTF8());
    std::fflush (stdout);
}
static juce::String f (double v, int d = 3) { return juce::String (v, d); }
static juce::String db (double ratio) { return ratio <= 0 ? juce::String ("-inf dB") : juce::String (20.0 * std::log10 (ratio), 1) + " dB"; }

//==============================================================================
static void setRaw (Proc& p, const juce::String& id, float raw)
{
    auto* prm = p.parameters.getParameter (id);
    if (prm == nullptr) { std::printf ("NO PARAM %s\n", id.toRawUTF8()); std::exit (2); }
    prm->setValueNotifyingHost (prm->convertTo0to1 (raw));
}
static double rawOf (Proc& p, const char* id) { return (double) p.parameters.getRawParameterValue (id)->load(); }
static juce::String bid (int band, const char* s) { return "b" + juce::String (band).paddedLeft ('0', 2) + "_" + s; }

// type: 0 Bell 1 LowShelf 2 HighShelf 3 LowCut 4 HighCut 5 Notch 6 BandPass 7 Tilt
// slope idx: 0 6, 1 12, 2 18, 3 24, 4 36, 5 48, 6 72, 7 96 dB/oct ; placement: 0 St 1 L 2 R 3 Mid 4 Side
static Fn band (int b, int type, float fr, float g, float q = 1.0f, int slope = 1, int place = 0)
{
    return [=] (Proc& p)
    {
        setRaw (p, bid (b, "type"), (float) type); setRaw (p, bid (b, "freq"), fr); setRaw (p, bid (b, "gain"), g);
        setRaw (p, bid (b, "q"), q); setRaw (p, bid (b, "slope"), (float) slope); setRaw (p, bid (b, "placement"), (float) place);
        setRaw (p, bid (b, "bypass"), 0.0f); setRaw (p, bid (b, "enabled"), 1.0f);
    };
}
static Fn set (const juce::String& id, float v) { return [=] (Proc& p) { setRaw (p, id, v); }; }
static Fn all (std::vector<Fn> fs) { return [=] (Proc& p) { for (auto& x : fs) if (x) x (p); }; }

static std::unique_ptr<Proc> make (double fs, int channels, int prepared)
{
    auto p = std::make_unique<Proc>();
    juce::AudioProcessor::BusesLayout layout;
    const auto s = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    layout.inputBuses.add (s); layout.outputBuses.add (s);
    if (! p->setBusesLayout (layout)) { std::printf ("layout refused\n"); std::exit (2); }
    p->setRateAndBufferSizeDetails (fs, prepared);
    p->prepareToPlay (fs, prepared);
    return p;
}

//==============================================================================
struct Ev { int at; Fn f; };
struct Opt
{
    int block = 64;
    bool randomBlocks = false;
    juce::int64 seed = 1;
    int maxBlock = 2048;
    bool nonRealtime = false;
    std::function<bool (int start)> viaBypassed;
    std::function<void (Proc&, int start, int len)> after;   // called after each block
};

// Blocks are cut at every event position, so an event always lands on a block start.
static Buf render (double fs, const Buf& in, const Fn& setup, const std::vector<Ev>& evs = {}, Opt o = {}, std::unique_ptr<Proc>* own = nullptr)
{
    const int n = in.getNumSamples(), ch = in.getNumChannels();
    auto p = make (fs, ch, o.randomBlocks ? o.maxBlock : o.block);
    if (o.nonRealtime) p->setNonRealtime (true);
    if (setup) setup (*p);
    Buf buf; buf.makeCopyOf (in);
    juce::MidiBuffer midi;
    juce::Random r (o.seed);
    for (int start = 0; start < n;)
    {
        int len = o.randomBlocks ? 1 + r.nextInt (o.maxBlock) : o.block;
        len = std::min (len, n - start);
        for (const auto& e : evs) if (e.at > start && e.at < start + len) len = e.at - start;
        for (const auto& e : evs) if (e.at == start) e.f (*p);
        Buf view (buf.getArrayOfWritePointers(), ch, start, len);
        if (o.viaBypassed && o.viaBypassed (start)) p->processBlockBypassed (view, midi); else p->processBlock (view, midi);
        if (o.after) o.after (*p, start, len);
        start += len;
    }
    if (own) *own = std::move (p);
    return buf;
}

static Buf noise (int ch, int n, juce::int64 seed, double amp = 0.5)
{
    juce::Random r (seed); Buf b (ch, n);
    for (int c = 0; c < ch; ++c) for (int i = 0; i < n; ++i) b.setSample (c, i, (float) (amp * (2.0 * r.nextDouble() - 1.0)));
    return b;
}
static void denormTail (Buf& b, int from, juce::int64 seed)
{
    juce::Random r (seed);
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = from; i < b.getNumSamples(); ++i)
        {
            float v = std::ldexp ((float) (0.25 + 0.75 * r.nextDouble()), -128);
            b.setSample (c, i, r.nextBool() ? v : -v);
        }
}

static bool bitEq (const Buf& a, const Buf& b, int from, int to)
{
    for (int c = 0; c < a.getNumChannels(); ++c)
        if (to > from && std::memcmp (a.getReadPointer (c) + from, b.getReadPointer (c) + from, sizeof (float) * (size_t) (to - from)) != 0) return false;
    return true;
}
static int countDiff (const Buf& a, const Buf& b, int from, int to, int* first = nullptr, int* last = nullptr)
{
    int k = 0; if (first) *first = -1; if (last) *last = -1;
    for (int c = 0; c < a.getNumChannels(); ++c)
        for (int i = from; i < to; ++i)
            if (std::memcmp (a.getReadPointer (c) + i, b.getReadPointer (c) + i, sizeof (float)) != 0)
            {
                ++k;
                if (first && (*first < 0 || i < *first)) *first = i;
                if (last) *last = std::max (*last, i);
            }
    return k;
}
static double peakOf (const Buf& a, int from, int to, int c0 = 0, int c1 = -1)
{
    if (c1 < 0) c1 = a.getNumChannels() - 1;
    double m = 0; for (int c = c0; c <= c1; ++c) for (int i = from; i < to; ++i) m = std::max (m, std::fabs ((double) a.getSample (c, i))); return m;
}
// max |a - ref| / peak(ref), over channels [c0, c1]
static double relErr (const Buf& a, const std::vector<std::vector<double>>& ref, int from, int to, int c0, int c1)
{
    double m = 0, pk = 1e-30;
    for (int c = c0; c <= c1; ++c)
        for (int i = from; i < to; ++i)
        {
            m = std::max (m, std::fabs ((double) a.getSample (c, i) - ref[(size_t) c][(size_t) i]));
            pk = std::max (pk, std::fabs (ref[(size_t) c][(size_t) i]));
        }
    return m / pk;
}

static int gridK (double fs) { return fs <= 48000.0 ? 16 : (fs <= 96000.0 ? 32 : 64); }
static int lenN (double sec, double fs) { return std::max (1, (int) std::lround (sec * fs)); }
//==============================================================================
// Phase 2.6 probe rows. Oracles re-stated from PLAN-2.6 P4 - P6, P10, P11 and
// PLAN-2.4 / 2.5 (grid K, fade weight, tick clock from 0 at prepare):
//   R  = round (0.030 fs)          listen input ramp
//   N  = round (0.020 fs)          fade (preroll 0 in v1)
//   engage / change = R + N samples, release = N; requests taken at the
//   first control tick at or after the block start that read them; a request
//   during a move waits for its end (latest wins); a weight > 0 first at move
//   sample R (engage), so the EQ path is untouched before it.
//   Listen output = og x F(dry) by placement (P5): independent of every other
//   band, of the listened band's gain / slope / enable / bypass, of gain
//   scale and of auto gain (C3).
//==============================================================================
#include "analysis/SpectrumAnalyzer.h"
#include <climits>
#include <limits>

static int ceilK (int s, int K) { return ((s + K - 1) / K) * K; }
static Fn lis (int k) { return [=] (Proc& p) { p.listenBand.store (k); }; }
// band numbers in parameter IDs are 1-based; listen indices are 0-based
static Fn bandI (int idx, int type, float fr, float g, float q = 1.0f, int slope = 1, int place = 0)
{
    return band (idx + 1, type, fr, g, q, slope, place);
}
static juce::String bidI (int idx, const char* s) { return bid (idx + 1, s); }

static Buf sine (int ch, int n, double fs, double hz, double amp, double sgnR = 1.0, bool onlyL = false)
{
    Buf b (ch, n);
    for (int i = 0; i < n; ++i)
    {
        const double v = amp * std::sin (2.0 * kPi * hz * i / fs);
        b.setSample (0, i, (float) v);
        if (ch > 1) b.setSample (1, i, onlyL ? 0.0f : (float) (sgnR * v));
    }
    return b;
}


#include <chrono>
using Clock = std::chrono::steady_clock;
static double usSince (Clock::time_point t0) { return std::chrono::duration<double, std::micro> (Clock::now() - t0).count(); }
static juce::String stats (std::vector<double> v, int d = 1)
{
    if (v.empty()) return "n 0";
    std::sort (v.begin(), v.end());
    auto q = [&] (double p) { return v[(size_t) std::min ((double) v.size() - 1, std::floor (p * (double) v.size()))]; };
    return "med " + f (q (0.5), d) + " p99 " + f (q (0.99), d) + " max " + f (v.back(), d) + " (n " + juce::String ((int) v.size()) + ")";
}
static Fn busy24()
{
    std::vector<Fn> fs;
    for (int b = 1; b <= 24; ++b)
        fs.push_back (band (b, 1 + (b % 4), (float) (30.0 * std::pow (600.0, (b - 1) / 23.0)), (b % 2 ? 4.0f : -4.0f), 1.0f, 7, b % 5));
    return all (fs);
}
static Fn light4()
{
    return all ({ band (1, 0, 120.0f, 3.0f), band (2, 0, 800.0f, -4.0f, 2.0f), band (3, 0, 3000.0f, 2.0f), band (4, 2, 9000.0f, 3.0f) });
}

// AC1: SpectrumAnalyzer::process cost per 60 Hz tick, and the event payload (256 points as JSON).
static void rowAC1()
{
    juce::String d;
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int mode : { 2, 3 })
        {
            auto p = make (fs, 2, 512);
            light4() (*p);
            p->analyzerMode.store (mode);
            SpectrumAnalyzer an (*p);
            const int n = lenN (6.0, fs);
            Buf in = noise (2, n, 7);
            juce::MidiBuffer midi;
            const double tick = fs / 60.0; double next = tick;
            std::vector<double> t;
            for (int start = 0; start < n; start += 512)
            {
                const int len = std::min (512, n - start);
                Buf view (in.getArrayOfWritePointers(), 2, start, len);
                p->processBlock (view, midi);
                if (start + len >= next) { auto t0 = Clock::now(); an.process (1.0 / 60.0); const double us = usSince (t0); if (start > fs) t.push_back (us); next += tick; }
            }
            d << f (fs / 1000.0, 1) << "k mode " << mode << " N " << an.getFftSize() << ": " << stats (t) << " us\n        ";
        }
    // payload: 256 floats -> var array -> JSON (full precision vs 0.1 dB ints)
    {
        auto p = make (48000.0, 2, 512); p->analyzerMode.store (2);
        SpectrumAnalyzer an (*p);
        Buf in = noise (2, 48000, 9); juce::MidiBuffer midi;
        for (int s = 0; s < 48000; s += 512) { Buf v (in.getArrayOfWritePointers(), 2, s, std::min (512, 48000 - s)); p->processBlock (v, midi); }
        an.process (0.02);
        std::vector<double> tFull, tTenth; size_t szFull = 0, szTenth = 0;
        for (int r = 0; r < 400; ++r)
        {
            auto t0 = Clock::now();
            juce::Array<juce::var> a; a.ensureStorageAllocated (256);
            for (int i = 0; i < 256; ++i) a.add ((double) an.getPost()[i]);
            auto js = juce::JSON::toString (juce::var (a), true);
            tFull.push_back (usSince (t0)); szFull = (size_t) js.getNumBytesAsUTF8();
            t0 = Clock::now();
            juce::Array<juce::var> b; b.ensureStorageAllocated (256);
            for (int i = 0; i < 256; ++i) b.add ((int) std::lround (std::max (-120.0f, an.getPost()[i]) * 10.0f));
            auto js2 = juce::JSON::toString (juce::var (b), true);
            tTenth.push_back (usSince (t0)); szTenth = (size_t) js2.getNumBytesAsUTF8();
        }
        d << "payload full double: " << stats (tFull) << " us, " << (int) szFull << " B; tenths int (floor -120): " << stats (tTenth) << " us, " << (int) szTenth << " B";
    }
    info ("AC1", "analyzer tick cost (process, 60 Hz) + payload", d);
}

// AT1: audio-thread time per block, analyzer mode 0 / 2 / 3 with a consumer registered (drained at 60 Hz
// between blocks on the same thread), no consumer as the reference.
static void rowAT1()
{
    juce::String d;
    for (int cfg : { 0, 1 })
        for (double fs : { 48000.0, 192000.0 })
        {
            d << (cfg ? "24 x 96 dB/oct " : "4 bells ") << f (fs / 1000.0, 1) << "k blk 512: ";
            for (int mode : { -1, 0, 2, 3 })
            {
                auto p = make (fs, 2, 512);
                (cfg ? busy24() : light4()) (*p);
                std::unique_ptr<SpectrumAnalyzer> an;
                if (mode >= 0) { p->analyzerMode.store (mode); an = std::make_unique<SpectrumAnalyzer> (*p); }
                const int n = lenN (4.0, fs);
                Buf in = noise (2, n, 11); juce::MidiBuffer midi;
                std::vector<double> t; const double tick = fs / 60.0; double next = tick;
                for (int rep = 0; rep < 3; ++rep)
                    for (int start = 0; start + 512 <= n; start += 512)
                    {
                        Buf view (in.getArrayOfWritePointers(), 2, start, 512);
                        auto t0 = Clock::now(); p->processBlock (view, midi); const double us = usSince (t0);
                        if (rep > 0) t.push_back (us);
                        if (an && start + 512 >= next) { an->process (1.0 / 60.0); next += tick; }
                    }
                if (an && mode == 0) d << "[drained " << (int) an->getSamplesDrained (0) << "/" << (int) an->getSamplesDrained (1) << "] ";
                std::sort (t.begin(), t.end());
                d << (mode < 0 ? "no consumer " : ("mode " + juce::String (mode) + " ")) << f (t[t.size() / 2], 2) << " / p99 " << f (t[(size_t) (t.size() * 0.99)], 2) << " us; ";
            }
            d << "\n        ";
        }
    info ("AT1", "processBlock time vs analyzer mode (tap cost)", d);
}

// AO1: Off stops the audio-thread writes; a mode switch back on resumes them.
static void rowAO1()
{
    auto p = make (48000.0, 2, 512); light4() (*p);
    p->analyzerMode.store (0);
    SpectrumAnalyzer an (*p);
    Buf in = noise (2, 48000, 3); juce::MidiBuffer midi;
    for (int s = 0; s < 48000; s += 512) { Buf v (in.getArrayOfWritePointers(), 2, s, std::min (512, 48000 - s)); p->processBlock (v, midi); if (s % 4096 == 0) an.process (0.016); }
    an.process (0.016);
    const auto off0 = an.getSamplesDrained (0), off1 = an.getSamplesDrained (1);
    p->analyzerMode.store (2);
    for (int s = 0; s < 48000; s += 512) { Buf v (in.getArrayOfWritePointers(), 2, s, std::min (512, 48000 - s)); p->processBlock (v, midi); if (s % 4096 == 0) an.process (0.016); }
    an.process (0.016);
    row (off0 == 0 && off1 == 0 && an.getSamplesDrained (1) > 40000 && an.getSamplesDrained (0) == 0, "AO1", "Off: no tap writes; Post on resumes post only",
         "off drained " + juce::String ((int) off0) + "/" + juce::String ((int) off1) + ", post on: pre " + juce::String ((int) an.getSamplesDrained (0)) + " post " + juce::String ((int) an.getSamplesDrained (1)));
}

// M1: output meter reads under host block sizes. Model: blocks complete in real time (block b ends at
// (b+1) B / fs); the editor reads readAndResetOutputPeak at 60 Hz ticks. -6 dBFS 1 kHz sine, stereo.
// Reported: worst tick reading of the raw read, and with a 3-tick (50 ms) max-hold over the reads.
static void rowM1()
{
    juce::String d; bool okHold = true;
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int B : { 64, 512, 1024, 2048, 4096 })
        {
            auto p = make (fs, 2, B);
            const int n = lenN (3.0, fs);
            Buf in = sine (2, n, fs, 1000.0, std::pow (10.0, -6.0 / 20.0));
            juce::MidiBuffer midi;
            int blk = 0; double rawMin = 1e9, rawMax = -1e9, holdMin = 1e9; int zeros = 0, ticks = 0;
            std::vector<float> hist;
            for (int j = 1; j / 60.0 < 3.0; ++j)
            {
                const double tt = j / 60.0;
                while (blk < n / B && (blk + 1) * (double) B / fs <= tt)
                {
                    Buf view (in.getArrayOfWritePointers(), 2, blk * B, B); p->processBlock (view, midi); ++blk;
                }
                const float pk = std::max (p->readAndResetOutputPeak (0), p->readAndResetOutputPeak (1));
                hist.push_back (pk);
                if (tt < 0.25) continue;
                ++ticks;
                const double raw = pk > 0 ? 20.0 * std::log10 (pk) : -200.0;
                if (pk <= 0) ++zeros;
                rawMin = std::min (rawMin, raw); rawMax = std::max (rawMax, raw);
                float h = 0; for (size_t k = hist.size() >= 3 ? hist.size() - 3 : 0; k < hist.size(); ++k) h = std::max (h, hist[k]);
                const double hd = h > 0 ? 20.0 * std::log10 (h) : -200.0;
                holdMin = std::min (holdMin, hd);
            }
            const bool ok = std::fabs (holdMin + 6.0) <= 0.5;
            if (B * 1000.0 / fs <= 50.0) okHold = okHold && ok;
            if (zeros || ! ok || B == 2048)
                d << f (fs / 1000.0, 1) << "k B " << B << " (" << f (B * 1000.0 / fs, 1) << " ms): raw " << f (rawMin, 2) << " .. " << f (rawMax, 2) << " dB, empty ticks " << zeros << "/" << ticks << "; 3-tick hold min " << f (holdMin, 2) << "\n        ";
        }
    row (okHold, "M1", "-6 dBFS sine: 3-tick hold within 0.5 dB for blocks <= 50 ms", d);
}

// N1e as a gated row: a NaN inside an engage's input ramp with no band running is caught at its own chunk.
static void rowN1e()
{
    juce::String e; int bad = 0, cases = 0;
    for (int ch : { 1, 2 })
    for (int block : { 64, 512 })
        for (int band3 : { 0, 1 })
            for (int kind : { 0, 1 })   // 0 engage from off, 1 change 5 -> 3
            {
                const double fs = 48000.0; const int n = 9000, K = gridK (fs), R = lenN (0.030, fs);
                const int t1 = ceilK (1001, K), sn = t1 + 700;
                Buf in = noise (ch, n, 91);
                for (int c = 0; c < ch; ++c) in.setSample (c, sn, std::numeric_limits<float>::quiet_NaN());
                Opt o; o.block = block;
                Buf out = render (fs, in, all ({ bandI (3, 0, 1000.0f, 6.0f, 2.0f), bandI (5, 0, 4000.0f, 3.0f, 1.0f), set (bidI (5, "enabled"), 0.0f),
                                                 set (bidI (3, "enabled"), band3 ? 1.0f : 0.0f), lis (kind ? 5 : -1) }),
                                  { { 1001, lis (3) } }, o);
                const int chunkEnd = 1001 + ((sn - 1001) / block + 1) * block;
                int lo = -1, hi = -1;
                for (int c = 0; c < ch; ++c) for (int i = chunkEnd; i < n; ++i) if (! std::isfinite (out.getSample (c, i))) { if (lo < 0 || i < lo) lo = i; hi = std::max (hi, i); }
                ++cases; if (lo >= 0) ++bad;
                if (lo >= 0 || block == 512)
                    e << ch << "ch blk " << block << (band3 ? " on" : " off") << (kind ? " change" : " engage") << ": later non-finite "
                      << (lo < 0 ? juce::String ("none") : "[" + juce::String (lo) + ".." + juce::String (hi) + "]") << " (t1+R " << t1 + R << ")  ";
            }
    row (bad == 0, "N1e", "NaN in listen's input ramp: caught at its own chunk", juce::String (cases - bad) + " / " + juce::String (cases) + "  " + e);
}

// N1x: listen-off and listen-on renders without a NaN are bit-identical to the unfixed build: printed
// as a hash so the two builds can be compared.
static void rowHash()
{
    juce::MemoryBlock mem;
    for (double fs : { 44100.0, 96000.0 })
        for (int ch : { 1, 2 })
            for (int L : { -1, 3 })
            {
                Buf in = noise (ch, lenN (1.0, fs), 5 + ch);
                std::vector<Ev> ev { { 3001, lis (L) }, { lenN (0.5, fs), lis (L < 0 ? 3 : 0) } };
                Opt o; o.randomBlocks = true; o.maxBlock = 1500; o.seed = 4;
                Buf out = render (fs, in, all ({ light4(), set ("auto_gain", 1.0f) }), ev, o);
                for (int c = 0; c < ch; ++c) mem.append (out.getReadPointer (c), sizeof (float) * (size_t) out.getNumSamples());
            }
    info ("HASH", "listen renders without NaN (compare across builds)", juce::String::toHexString ((juce::int64) mem.toBase64Encoding().hashCode64()) + " (" + juce::String ((int) mem.getSize()) + " B)");
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const juce::String only = argc > 1 ? juce::String (argv[1]) : juce::String();
    std::printf ("=== O-ParametricEQ research-3.3 probe ===\n");
    if (only.isEmpty() || only == "n") { rowN1e(); rowHash(); }
    if (only.isEmpty() || only == "a") { rowAO1(); rowAC1(); rowAT1(); }
    if (only.isEmpty() || only == "m") rowM1();
    std::printf ("\n%d PASS, %d FAIL\n", nPass, nFail);
    return nFail == 0 ? 0 : 1;
}
