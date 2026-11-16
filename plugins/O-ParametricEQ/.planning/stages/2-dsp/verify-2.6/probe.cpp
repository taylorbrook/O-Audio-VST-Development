// Independent probe for O-ParametricEQ Phase 2.6 verify (listen, taps, analyzer through the processor).
// Rows N1, N1e, E1, X1, B2 were added after the first mutation pass, aimed at its survivors.
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

//------------------------------------------------------------------------------
// T1: listen timeline at block size 1 against the re-stated state machine
struct Seg { int start; eq::ListenMove kind; int len; int target; bool latched; };

static void rowT1()
{
    struct Cfg { double fs; int ch; };
    int cases = 0, bad = 0; juce::String detail;
    for (Cfg c : { Cfg { 48000.0, 2 }, Cfg { 96000.0, 2 }, Cfg { 44100.0, 1 }, Cfg { 192000.0, 2 } })
    {
        const int K = gridK (c.fs), R = lenN (0.030, c.fs), N = lenN (0.020, c.fs), L = R + N;
        // script
        const int s1 = 1001;                 const int t1 = ceilK (s1, K);
        const int s2 = t1 + 500;             const int e1 = t1 + L;          // latched change to 7
        const int s3 = e1 + 700;             const int e2 = e1 + L;          // latched release
        const int s4 = e2 + 100;             const int e3 = e2 + N;          // latched engage to 5
        const int s5 = e3 + L + 301;                                          // band 5 freq edit: ramp, no move
        const int s6 = s5 + 403;                                              // band 5 gain edit: nothing
        const int s7 = s6 + 405;             const int t7 = ceilK (s7, K);   // band 5 type family: change, target 5
        const int s8 = t7 + 37;              const int e7 = t7 + L;          // latched change to 9
        const int n = e7 + L + 500;
        std::vector<Seg> segs = {
            { t1, eq::ListenMove::engage,  L, 3,  false },
            { e1, eq::ListenMove::change,  L, 7,  true  },
            { e2, eq::ListenMove::release, N, -1, true  },
            { e3, eq::ListenMove::engage,  L, 5,  true  },
            { t7, eq::ListenMove::change,  L, 5,  false },
            { e7, eq::ListenMove::change,  L, 9,  true  } };
        auto targetAfterNone = [&] (int i) { // target when no move holds sample i
            if (i < t1) return -1;
            if (i < e7 + L) { for (int k = (int) segs.size() - 1; k >= 0; --k) if (i >= segs[(size_t) k].start) return segs[(size_t) k].target; }
            return 9;
        };
        auto expect = [&] (int i, eq::ListenMove& mv, int& pos, int& tg)
        {
            for (size_t k = 0; k < segs.size(); ++k)
            {
                const Seg& s = segs[k];
                if (i >= s.start && i < s.start + s.len)
                {
                    if (i < s.start + s.len - 1) { mv = s.kind; pos = i - s.start + 1; tg = s.target; return; }
                    if (k + 1 < segs.size() && segs[k + 1].latched && segs[k + 1].start == i + 1)
                    { mv = segs[k + 1].kind; pos = 0; tg = segs[k + 1].target; return; }
                    mv = eq::ListenMove::none; pos = 0; tg = s.target; return;
                }
            }
            mv = eq::ListenMove::none; pos = 0; tg = targetAfterNone (i);
        };
        Buf in = noise (c.ch, n, 21);
        int mism = 0, firstBad = -1; int rampSeen = 0, fadeSeen = 0;
        unsigned long moves = 0;
        Opt o; o.block = 1;
        o.after = [&] (Proc& p, int start, int)
        {
            const auto st = p.getListenState();
            rampSeen = st.rampLength; fadeSeen = st.fadeLength;
            eq::ListenMove mv; int pos, tg; expect (start, mv, pos, tg);
            if (st.move != mv || st.movePosition != pos || st.target != tg)
            { if (firstBad < 0) firstBad = start; ++mism; }
        };
        auto setup = all ({ bandI (3, 0, 1000.0f, 6.0f, 2.0f), bandI (7, 1, 200.0f, -4.0f), bandI (5, 0, 3000.0f, 3.0f, 4.0f), bandI (9, 4, 6000.0f, 0.0f, 1.0f, 3) });
        std::unique_ptr<Proc> own;
        render (c.fs, in, setup, { { s1, lis (3) }, { s2, lis (7) }, { s3, lis (-1) }, { s4, lis (5) },
                                   { s5, set (bidI (5, "freq"), 2500.0f) }, { s6, set (bidI (5, "gain"), -9.0f) },
                                   { s7, set (bidI (5, "type"), 2.0f) }, { s8, lis (9) } }, o, &own);
        ++cases;
        const bool ok = mism == 0 && rampSeen == R && fadeSeen == N;
        if (! ok) ++bad;
        detail << f (c.fs / 1000.0, 1) << "k/" << c.ch << "ch " << (ok ? "ok" : "BAD") << " (R " << rampSeen << "/" << R << ", N " << fadeSeen << "/" << N
               << ", mismatched samples " << mism << (firstBad >= 0 ? " first " + juce::String (firstBad) : juce::String()) << ")  ";
        juce::ignoreUnused (moves);
    }
    row (bad == 0, "T1", "listen timeline (block 1) = re-stated P6 state machine", detail);
}

//------------------------------------------------------------------------------
// O1: onset and return are exact: no band running, unity stage
static void rowO1()
{
    struct Cfg { double fs; int ch; int block; bool rnd; };
    int bad = 0; juce::String detail;
    for (Cfg c : { Cfg { 48000.0, 2, 64, false }, Cfg { 48000.0, 2, 0, true }, Cfg { 96000.0, 1, 512, false }, Cfg { 44100.0, 2, 0, true }, Cfg { 192000.0, 2, 7, false } })
    {
        const int K = gridK (c.fs), R = lenN (0.030, c.fs), N = lenN (0.020, c.fs);
        const int s1 = 1001, t1 = ceilK (s1, K);
        const int s2 = t1 + R + N + 3001, t2 = ceilK (s2, K);
        const int n = t2 + N + 4000;
        Buf in = noise (c.ch, n, 31);
        Opt o; o.block = c.block; o.randomBlocks = c.rnd; o.maxBlock = 1024; o.seed = 77;
        // band 3 configured but disabled (listen does not need it running)
        Buf out = render (c.fs, in, all ({ bandI (3, 0, 1000.0f, 6.0f, 2.0f), set (bidI (3, "enabled"), 0.0f) }),
                          { { s1, lis (3) }, { s2, lis (-1) } }, o);
        int first = -1, last = -1;
        countDiff (out, in, 0, n, &first, &last);
        const bool ok = first == t1 + R && last == t2 + N - 1;
        if (! ok) ++bad;
        detail << f (c.fs / 1000.0, 1) << "k/" << c.ch << "ch/" << (c.rnd ? juce::String ("rnd") : juce::String (c.block))
               << " first diff " << first << " (want " << (t1 + R) << "), last " << last << " (want " << (t2 + N - 1) << ")  ";
    }
    row (bad == 0, "O1", "listen onset at tick + R, return exact from release end", detail);
}

//------------------------------------------------------------------------------
// I1: listen output independent of everything but the listened band's f / Q /
// type family / placement (settled at open, unity output gain)
static void rowI1()
{
    struct Cfg { double fs; int ch; };
    const int types[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    const float freqs[] = { 1000.0f, 300.0f, 3000.0f, 80.0f, 8000.0f, 1500.0f, 700.0f, 2000.0f };
    int cases = 0, bad = 0, ctl = 0, ctlFired = 0, ctlQ = 0, ctlQFired = 0; juce::String firstBad;
    juce::Random rr (5);
    for (Cfg c : { Cfg { 44100.0, 2 }, Cfg { 44100.0, 1 }, Cfg { 96000.0, 2 } })
        for (int t = 0; t < 8; ++t)
            for (int pl = 0; pl < (c.ch == 2 ? 5 : 1); ++pl)
            {
                if (c.fs > 50000.0 && pl != t % 5) continue;
                const int k = 4, n = lenN (0.25, c.fs);
                Buf in = noise (c.ch, n, 40 + t);
                auto lb = bandI (k, types[t], freqs[t], 6.0f, 2.5f, 4, pl);
                Opt o; o.block = 128;
                const Fn L = lis (k);
                Buf base = render (c.fs, in, all ({ lb, L }), {}, o);
                auto others = [&] (Proc& p) {
                    juce::Random r (c.fs + t * 7 + pl);
                    for (int b = 0; b < 24; ++b) if (b != k)
                        bandI (b, r.nextInt (8), 20.0f * std::pow (1000.0f, r.nextFloat()), -18.0f + 36.0f * r.nextFloat(), 0.1f + 8.0f * r.nextFloat(), r.nextInt (8), r.nextInt (5)) (p);
                };
                std::vector<std::pair<const char*, Buf>> vars;
                vars.push_back ({ "23 other bands", render (c.fs, in, all ({ lb, others, L }), {}, o) });
                vars.push_back ({ "disabled", render (c.fs, in, all ({ lb, set (bidI (k, "enabled"), 0.0f), L }), {}, o) });
                vars.push_back ({ "bypassed", render (c.fs, in, all ({ lb, set (bidI (k, "bypass"), 1.0f), L }), {}, o) });
                vars.push_back ({ "gain 0 dB", render (c.fs, in, all ({ lb, set (bidI (k, "gain"), 0.0f), L }), {}, o) });
                vars.push_back ({ "gain scale 0", render (c.fs, in, all ({ lb, set ("gain_scale", 0.0f), L }), {}, o) });
                vars.push_back ({ "auto gain on + others", render (c.fs, in, all ({ lb, others, set ("auto_gain", 1.0f), L }), {}, o) });
                {
                    std::vector<Ev> ev = { { n / 5 + 3, set (bidI (k, "gain"), -12.0f) }, { n / 3 + 11, set (bidI (k, "slope"), 7.0f) },
                                           { n / 2 + 5, set (bidI (k, "gain"), 30.0f) } };
                    if (types[t] != 0 && types[t] != 5 && types[t] != 6) ev.push_back ({ 2 * n / 3 + 1, set (bidI (k, "q"), 0.3f) }); // Q inert on shelves, cuts, tilt
                    vars.push_back ({ "gain / slope (/ inert Q) edits", render (c.fs, in, all ({ lb, L }), ev, o) });
                }
                {
                    Opt r = o; r.randomBlocks = true; r.maxBlock = 2048; r.seed = 9 + t;
                    vars.push_back ({ "random blocks", render (c.fs, in, all ({ lb, L }), {}, r) });
                }
                for (auto& v : vars)
                {
                    ++cases;
                    if (! bitEq (v.second, base, 0, n)) { ++bad; if (firstBad.isEmpty()) firstBad = juce::String (v.first) + " type " + juce::String (t) + " pl " + juce::String (pl) + " " + f (c.fs, 0); }
                }
                // controls: a frequency edit of the listened band must change the output;
                // a Q edit too where Q enters the map (Bell, Notch, Band Pass)
                ++ctl;
                if (! bitEq (render (c.fs, in, all ({ lb, L }), { { n / 2, set (bidI (k, "freq"), freqs[t] * 1.3f) } }, o), base, 0, n)) ++ctlFired;
                if (types[t] == 0 || types[t] == 5 || types[t] == 6)
                {
                    ++ctlQ;
                    if (! bitEq (render (c.fs, in, all ({ lb, L }), { { n / 2, set (bidI (k, "q"), 0.7f) } }, o), base, 0, n)) ++ctlQFired;
                }
            }
    row (bad == 0 && ctlFired == ctl && ctlQFired == ctlQ, "I1", "listen = F(dry) only: memcmp-equal across 8 variants",
         juce::String (cases - bad) + " / " + juce::String (cases) + " equal" + (bad ? "  first bad " + firstBad : juce::String())
         + "; controls: freq edit " + juce::String (ctlFired) + " / " + juce::String (ctl) + ", Q edit " + juce::String (ctlQFired) + " / " + juce::String (ctlQ) + " differ");
}

//------------------------------------------------------------------------------
// I2: output gain scales the listen output (C3: listen before output gain)
static void rowI2()
{
    double worst = 0; int cases = 0;
    for (double fs : { 48000.0, 96000.0 })
        for (int ch : { 1, 2 })
        {
            const int n = lenN (0.2, fs);
            Buf in = noise (ch, n, 50);
            auto lb = bandI (2, 6, 900.0f, 0.0f, 3.0f);
            Opt o; o.block = 256;
            Buf a = render (fs, in, all ({ lb, lis (2) }), {}, o);
            Buf b = render (fs, in, all ({ lb, lis (2), set ("output_gain", 6.0f), set ("auto_gain", 1.0f) }), {}, o);
            const double g = std::pow (10.0, 6.0 / 20.0);
            double pk = peakOf (a, 0, n), m = 0;
            for (int c = 0; c < ch; ++c) for (int i = 0; i < n; ++i) m = std::max (m, std::fabs (b.getSample (c, i) - g * a.getSample (c, i)));
            worst = std::max (worst, m / (g * pk)); ++cases;
        }
    row (worst < 1.0e-6, "I2", "settled listen x output +6 dB (auto gain on) = 1.995 x listen", juce::String (cases) + " cases, worst " + db (worst) + " re peak");
}

//------------------------------------------------------------------------------
// R1: out-of-range listenBand reads as off
static void rowR1()
{
    int bad = 0, cases = 0;
    for (int ch : { 1, 2 })
    {
        const double fs = 48000.0; const int n = lenN (0.4, fs);
        Buf in = noise (ch, n, 60);
        auto setup = all ({ bandI (0, 0, 500.0f, 6.0f), bandI (11, 3, 60.0f, 0.0f, 1.0f, 5), bandI (23, 2, 5000.0f, -4.0f) });
        Opt o; o.randomBlocks = true; o.maxBlock = 1000; o.seed = 3;
        Buf base = render (fs, in, setup, {}, o);
        Buf out = render (fs, in, setup, { { 777, lis (24) }, { 5000, lis (1000) }, { 9001, lis (-2) }, { 13000, lis (INT_MIN) }, { 15555, lis (INT_MAX) }, { 17000, lis (-1) } }, o);
        ++cases; if (! bitEq (out, base, 0, n)) ++bad;
    }
    row (bad == 0, "R1", "listenBand 24, 1000, -2, INT_MIN, INT_MAX read as off", juce::String (cases - bad) + " / " + juce::String (cases) + " memcmp-equal to no listen");
}

//------------------------------------------------------------------------------
// B1: block identity with listen, band, gain and bypass events (both routes)
struct BEv { int at; int kind; int a; float v; };

static Buf renderB (double fs, const Buf& in, const std::vector<BEv>& evs, int mode, int seed, int prepared)
{
    // mode 0 random (max 4096), 1 fixed block = seed, 2 one block per span between events
    const int n = in.getNumSamples(), ch = in.getNumChannels();
    auto p = make (fs, ch, prepared);
    all ({ bandI (1, 0, 800.0f, 6.0f, 3.0f), bandI (4, 1, 150.0f, -6.0f), bandI (8, 4, 9000.0f, 0.0f, 1.0f, 6, 3), bandI (13, 6, 2000.0f, 0.0f, 8.0f, 1, 1),
           bandI (17, 7, 1200.0f, 5.0f), bandI (22, 3, 40.0f, 0.0f, 1.0f, 7, 4) }) (*p);
    Buf buf; buf.makeCopyOf (in);
    juce::MidiBuffer midi; juce::Random r (seed);
    bool routeBypassed = false, bypassOn = false; int bypassCount = 0;
    size_t e = 0;
    const int pool[] = { 1, 4, 8, 13, 17, 22 };
    for (int start = 0; start < n;)
    {
        while (e < evs.size() && evs[e].at == start)
        {
            const BEv& v = evs[e++];
            switch (v.kind)
            {
                case 0: p->listenBand.store (v.a); break;
                case 1: setRaw (*p, bidI (pool[v.a % 6], "freq"), v.v); break;
                case 2: setRaw (*p, bidI (pool[v.a % 6], "q"), v.v); break;
                case 3: setRaw (*p, bidI (pool[v.a % 6], "type"), (float) ((int) v.v % 8)); break;
                case 4: setRaw (*p, bidI (pool[v.a % 6], "placement"), (float) ((int) v.v % 5)); break;
                case 5: setRaw (*p, bidI (pool[v.a % 6], "enabled"), v.v > 0.5f ? 1.0f : 0.0f); break;
                case 6: setRaw (*p, "output_gain", v.v); break;
                case 7: setRaw (*p, "auto_gain", v.v > 0.5f ? 1.0f : 0.0f); break;
                case 8:
                    bypassOn = ! bypassOn;
                    if (bypassOn) routeBypassed = (bypassCount++ % 2) == 1;
                    setRaw (*p, "bypass", bypassOn && ! routeBypassed ? 1.0f : 0.0f);
                    break;
                default: break;
            }
        }
        int len = mode == 0 ? 1 + r.nextInt (4096) : (mode == 1 ? seed : n);
        len = std::min ({ len, n - start, prepared });
        if (e < evs.size() && evs[e].at < start + len) len = evs[e].at - start;
        Buf view (buf.getArrayOfWritePointers(), ch, start, len);
        if (bypassOn && routeBypassed) p->processBlockBypassed (view, midi); else p->processBlock (view, midi);
        start += len;
    }
    return buf;
}

static void rowB1()
{
    struct Cfg { double fs; int ch; };
    int bad = 0, cases = 0; juce::String detail;
    for (Cfg c : { Cfg { 44100.0, 2 }, Cfg { 96000.0, 1 }, Cfg { 192000.0, 2 }, Cfg { 48000.0, 2 } })
    {
        const int n = lenN (2.0, c.fs);
        Buf in = noise (c.ch, n, 70 + (int) c.fs);
        std::vector<BEv> evs;
        juce::Random r ((juce::int64) c.fs * 3 + c.ch);
        for (int at = 500 + r.nextInt (2000); at < n - 100; at += 100 + r.nextInt (lenN (0.06, c.fs)))
        {
            BEv v { at, 0, 0, 0.0f };
            const double u = r.nextDouble();
            if (u < 0.40)      { v.kind = 0; v.a = r.nextDouble() < 0.2 ? -1 : (r.nextDouble() < 0.6 ? (int) std::vector<int> { 1, 4, 8, 13, 17, 22 }[(size_t) r.nextInt (6)] : r.nextInt (24)); }
            else if (u < 0.55) { v.kind = 1; v.a = r.nextInt (6); v.v = 30.0f * std::pow (600.0f, r.nextFloat()); }
            else if (u < 0.65) { v.kind = 2; v.a = r.nextInt (6); v.v = 0.2f + 10.0f * r.nextFloat(); }
            else if (u < 0.72) { v.kind = 3; v.a = r.nextInt (6); v.v = (float) r.nextInt (8); }
            else if (u < 0.78) { v.kind = 4; v.a = r.nextInt (6); v.v = (float) r.nextInt (5); }
            else if (u < 0.83) { v.kind = 5; v.a = r.nextInt (6); v.v = r.nextFloat(); }
            else if (u < 0.89) { v.kind = 6; v.v = -12.0f + 24.0f * r.nextFloat(); }
            else if (u < 0.94) { v.kind = 7; v.v = r.nextFloat(); }
            else               { v.kind = 8; }
            evs.push_back (v);
        }
        const Buf ref = renderB (c.fs, in, evs, 0, 1, 4096);
        std::vector<std::pair<juce::String, Buf>> alts;
        alts.push_back ({ "rnd2", renderB (c.fs, in, evs, 0, 2, 4096) });
        alts.push_back ({ "blk1", renderB (c.fs, in, evs, 1, 1, 4096) });
        alts.push_back ({ "blk7", renderB (c.fs, in, evs, 1, 7, 4096) });
        alts.push_back ({ "blk4096", renderB (c.fs, in, evs, 1, 4096, 4096) });
        alts.push_back ({ "span", renderB (c.fs, in, evs, 2, 0, n) });
        for (auto& a : alts)
        {
            ++cases; int first = -1;
            if (countDiff (a.second, ref, 0, n, &first) != 0) { ++bad; detail << a.first << "@" << f (c.fs, 0) << " first " << first << "  "; }
        }
        detail << f (c.fs / 1000.0, 1) << "k/" << c.ch << "ch " << (int) evs.size() << " events  ";
    }
    row (bad == 0, "B1", "block identity: listen + bands + gains + bypass both routes", juce::String (cases - bad) + " / " + juce::String (cases) + " bit-identical  " + detail);
}

//------------------------------------------------------------------------------
// A1: the analyzer through the processor: mono layout, mono sum, output gain
static double analyzerPeak (double fs, const Buf& in, bool post, std::function<void (Proc&)> setup = nullptr)
{
    const int ch = in.getNumChannels(), n = in.getNumSamples();
    auto p = make (fs, ch, 512);
    if (setup) setup (*p);
    p->analyzerMode.store (3);
    SpectrumAnalyzer an (*p);
    Buf buf; buf.makeCopyOf (in);
    juce::MidiBuffer midi;
    const int every = lenN (0.033, fs); int since = 0;
    for (int start = 0; start < n; start += 512)
    {
        const int len = std::min (512, n - start);
        Buf view (buf.getArrayOfWritePointers(), ch, start, len);
        p->processBlock (view, midi);
        since += len;
        if (since >= every) { an.process (since / fs); since = 0; }
    }
    an.process (0.0);
    const float* pts = post ? an.getPost() : an.getPre();
    double m = -1000.0; for (int i = 0; i < 256; ++i) m = std::max (m, (double) pts[i]);
    return m;
}

static void rowA1()
{
    juce::String detail; bool ok = true;
    const double amp = std::pow (10.0, -12.0 / 20.0);
    for (double fs : { 48000.0, 96000.0, 192000.0 })
    {
        const int n = lenN (1.0, fs);
        const double mono = analyzerPeak (fs, sine (1, n, fs, 1000.0, amp), true);
        const double onlyL = analyzerPeak (fs, sine (2, n, fs, 1000.0, amp, 1.0, true), true);
        const double anti = analyzerPeak (fs, sine (2, n, fs, 1000.0, amp, -1.0), true);
        const double pre = analyzerPeak (fs, sine (1, n, fs, 1000.0, amp), false, [] (Proc& p) { setRaw (p, "output_gain", 6.0f); });
        const double post = analyzerPeak (fs, sine (1, n, fs, 1000.0, amp), true, [] (Proc& p) { setRaw (p, "output_gain", 6.0f); });
        const bool k = std::fabs (mono + 12.0) <= 0.5 && std::fabs (onlyL + 18.02) <= 0.5 && anti <= -100.0
                    && std::fabs (pre + 12.0) <= 0.5 && std::fabs ((post - pre) - 6.0) <= 0.5;
        ok = ok && k;
        detail << f (fs / 1000.0, 0) << "k: mono " << f (mono, 2) << ", L only " << f (onlyL, 2) << ", L = -R " << f (anti, 1)
               << ", +6 dB post - pre " << f (post - pre, 2) << "  ";
    }
    row (ok, "A1", "analyzer: mono layout -12, L-only -18.02, L = -R floor, post - pre", detail);
}

//------------------------------------------------------------------------------
// G1: tap gating through processBlockBypassed and when fully bypassed
static void rowG1()
{
    const double fs = 48000.0; const int n = 48000;
    Buf in = noise (2, n, 80);
    int bad = 0; juce::String d;
    for (int mode = 0; mode < 4; ++mode)
        for (int route = 0; route < 2; ++route)
        {
            auto p = make (fs, 2, 512);
            p->analyzerMode.store (mode);
            p->setAnalyzerConsumer (true);
            Buf buf; buf.makeCopyOf (in); juce::MidiBuffer midi;
            for (int s = 0; s < n; s += 512)
            {
                Buf view (buf.getArrayOfWritePointers(), 2, s, std::min (512, n - s));
                if (route == 1) p->processBlockBypassed (view, midi); else p->processBlock (view, midi);
            }
            const auto pre = p->getPreTap().getWriteIndex() + p->getPreTap().getDropCount();
            const auto post = p->getPostTap().getWriteIndex() + p->getPostTap().getDropCount();
            const unsigned wantPre = (mode & 1) ? (unsigned) n : 0u, wantPost = (mode & 2) ? (unsigned) n : 0u;
            if (pre != wantPre || post != wantPost) { ++bad; d << "mode " << mode << " route " << route << " pre " << (int) pre << " post " << (int) post << "  "; }
            // content: the newest 32768 of the mono sum (pre) in the ring, read back
            if ((mode & 1) && route == 1)
            {
                std::vector<float> got (32768); std::uint32_t g, dr;
                const int k = p->getPreTap().drain (got.data(), 32768, g, dr);
                int mism = 0;
                for (int i = 0; i < k; ++i)
                {
                    const float want = 0.5f * (in.getSample (0, i) + in.getSample (1, i));
                    if (std::memcmp (&got[(size_t) i], &want, 4) != 0) ++mism;
                }
                if (k != 32768 || mism) { ++bad; d << "content k " << k << " mismatches " << mism << "  "; }
            }
        }
    row (bad == 0, "G1", "taps: mode bits on both entry points; ring = oldest-fitting mono sum", bad ? d : juce::String ("8 / 8 counts exact; 32768 drained samples bit-equal"));
}


//------------------------------------------------------------------------------
// N1: a non-finite input sample while listening does not stick (PLAN-2.6 P10 step 8:
// listen.clearState). Listen settled from the first block (blocks aligned at 0), and no listen;
// the output is finite from the first chunk that starts after the NaN.
// N1e (INFO): a NaN inside an engage's input ramp (the incoming voice not yet heard) with no band
// running: where non-finite output appears after the NaN's own chunk.
static void rowN1()
{
    int bad = 0, cases = 0; juce::String d;
    for (int ch : { 1, 2 })
        for (int mode : { 0, 1 })    // 0 listen settled on band 3, 1 no listen
            for (int block : { 64, 512 })
            {
                const double fs = 48000.0; const int n = 48000, sn = 20000 + 37;
                Buf in = noise (ch, n, 90 + ch);
                for (int c = 0; c < ch; ++c) in.setSample (c, sn, std::numeric_limits<float>::quiet_NaN());
                Opt o; o.block = block;
                Buf out = render (fs, in, all ({ bandI (3, 0, 1000.0f, 6.0f, 2.0f), lis (mode == 0 ? 3 : -1) }), {}, o);
                const int from = ((sn / block) + 1) * block;
                int nonFinite = 0;
                for (int c = 0; c < ch; ++c) for (int i = from; i < n; ++i) if (! std::isfinite (out.getSample (c, i))) ++nonFinite;
                ++cases;
                if (nonFinite) { ++bad; d << ch << "ch " << (mode ? "no listen" : "listen") << " blk " << block << ": " << nonFinite << " non-finite after " << from << "  "; }
            }
    row (bad == 0, "N1", "NaN input, listen settled / off: finite from the next chunk", juce::String (cases - bad) + " / " + juce::String (cases) + "  " + d);

    juce::String e;
    for (int block : { 64, 512 })
        for (int band3 : { 0, 1 })
        {
            const double fs = 48000.0; const int n = 9000, K = gridK (fs), R = lenN (0.030, fs);
            const int t1 = ceilK (1001, K), sn = t1 + 700;
            Buf in = noise (1, n, 91);
            in.setSample (0, sn, std::numeric_limits<float>::quiet_NaN());
            Opt o; o.block = block;
            Buf out = render (fs, in, all ({ bandI (3, 0, 1000.0f, 6.0f, 2.0f), set (bidI (3, "enabled"), band3 ? 1.0f : 0.0f) }), { { 1001, lis (3) } }, o);
            const int chunkEnd = 1001 + ((sn - 1001) / block + 1) * block;   // blocks restart at the event
            int lo = -1, hi = -1;
            for (int i = chunkEnd; i < n; ++i) if (! std::isfinite (out.getSample (0, i))) { if (lo < 0) lo = i; hi = i; }
            e << "blk " << block << (band3 ? " band on" : " band off") << ": NaN@" << sn << ", its chunk ends " << chunkEnd - 1
              << ", later non-finite " << (lo < 0 ? juce::String ("none") : "[" + juce::String (lo) + ".." + juce::String (hi) + "]") << " (t1 + R = " << t1 + R << ")  ";
        }
    info ("N1e", "NaN inside an engage's input ramp", e);
}

//------------------------------------------------------------------------------
// E1: an f / Q edit of the listened band during an engage / change reaches the incoming voice
// on its tick (P6 ramp row, applyPending): the output differs from the unedited render from
// move sample R (the first sample the incoming voice is heard), not from the move's end.
static void rowE1()
{
    int bad = 0, cases = 0; juce::String d;
    for (double fs : { 48000.0, 96000.0 })
        for (int kind : { 0, 1 })   // 0 engage band 3; 1 change 3 -> 7, edit band 7
            for (int what : { 0, 1 })   // 0 freq, 1 Q
            {
                const int K = gridK (fs), R = lenN (0.030, fs), N = lenN (0.020, fs), L = R + N;
                const int s1 = 1001, t1 = ceilK (s1, K);
                const int s2 = t1 + L + 500, t2 = ceilK (s2, K);
                const int moveStart = kind == 0 ? t1 : t2, band = kind == 0 ? 3 : 7;
                const int se = moveStart + R / 3;                      // inside the incoming voice's ramp
                const int n = moveStart + L + 4000;
                Buf in = noise (2, n, 95);
                auto setup = all ({ bandI (3, 0, 1000.0f, 6.0f, 2.0f), set (bidI (3, "enabled"), 0.0f), bandI (7, 6, 3000.0f, 0.0f, 4.0f), set (bidI (7, "enabled"), 0.0f) });
                std::vector<Ev> base = { { s1, lis (3) } };
                if (kind == 1) base.push_back ({ s2, lis (7) });
                std::vector<Ev> edited = base;
                edited.push_back ({ se, what == 0 ? set (bidI (band, "freq"), band == 3 ? 1500.0f : 4500.0f) : set (bidI (band, "q"), 0.5f) });
                Opt o; o.block = 64;
                Buf a = render (fs, in, setup, base, o), b = render (fs, in, setup, edited, o);
                int first = -1; countDiff (a, b, 0, n, &first);
                ++cases;
                const bool ok = first >= moveStart + R && first < moveStart + R + K;
                if (! ok) ++bad;
                d << f (fs / 1000.0, 0) << "k " << (kind ? "change" : "engage") << (what ? " Q" : " f") << " first " << first << " (want " << moveStart + R << ")  ";
            }
    row (bad == 0, "E1", "f / Q edit during engage / change reaches the incoming voice", d);
}

//------------------------------------------------------------------------------
// X1: a latched un-bypass with listen engaged, no band running, stage at unity fades back in
// (PLAN-2.6 P10 step 7: the jump is taken only with listen idle). The bypass parameter goes on at
// b1 and off again inside the fade; the fade back starts at r = b1 + N from the dry signal.
static void rowX1()
{
    int bad = 0; juce::String d;
    for (double fs : { 48000.0, 96000.0 })
        for (int ch : { 1, 2 })
            for (int route : { 0, 1 })
            {
                const int N = lenN (0.020, fs), b1 = 6400, b2 = b1 + 128, n = b1 + 6 * N;
                Buf in = noise (ch, n, 101);
                bool viaB = false;
                Opt o; o.block = 64;
                if (route == 1) o.viaBypassed = [&] (int start) { return start >= b1 && start < b2; };
                std::vector<Ev> ev = { { 0, lis (3) } };
                if (route == 0) { ev.push_back ({ b1, set ("bypass", 1.0f) }); ev.push_back ({ b2, set ("bypass", 0.0f) }); }
                juce::ignoreUnused (viaB);
                Buf out = render (fs, in, all ({ bandI (3, 6, 1000.0f, 0.0f, 2.0f), set (bidI (3, "enabled"), 0.0f) }), ev, o);
                const int r = b1 + N;
                auto meanE = [&] (int a, int z) { double m = 0; int k = 0; for (int c = 0; c < ch; ++c) for (int i = a; i < z; ++i) { m += std::fabs ((double) out.getSample (c, i) - in.getSample (c, i)); ++k; } return m / k; };
                const double ratio = meanE (r - 4, r + 12) / meanE (r + 2 * N, r + 2 * N + 512);
                if (! (ratio < 0.05)) ++bad;
                d << f (fs / 1000.0, 0) << "k/" << ch << "ch/" << (route ? "bypassed" : "param") << " " << f (ratio, 4) << "  ";
            }
    row (bad == 0, "X1", "latched un-bypass, listen engaged, no band: fades (|y - x| near r / settled < 0.05)", d);
}

//------------------------------------------------------------------------------
// B2: block identity with both gains away from unity (auto gain on with a +12 dB shelf, output
// +5 dB): the fused (ag og) and split (ag, listen, og) forms round differently, so a form switch
// inside a segment would depend on the block size (PLAN-2.6 R1). Listen events at off-grid times.
static Buf renderB2 (double fs, const Buf& in, const std::vector<Ev>& ev, int mode, int seed)
{
    Opt o;
    if (mode == 0) { o.randomBlocks = true; o.maxBlock = 4096; o.seed = seed; }
    else o.block = seed;
    return render (fs, in, all ({ bandI (0, 1, 200.0f, 12.0f), bandI (3, 0, 1000.0f, 6.0f, 2.0f), set ("auto_gain", 1.0f), set ("output_gain", 5.0f) }), ev, o);
}
static void rowB2()
{
    int bad = 0, cases = 0; juce::String d;
    for (double fs : { 44100.0, 96000.0 })
        for (int ch : { 1, 2 })
        {
            const int n = lenN (1.5, fs);
            Buf in = noise (ch, n, 111);
            std::vector<Ev> ev;
            juce::Random r ((juce::int64) fs + ch);
            for (int at = 777; at < n - 100; at += 300 + r.nextInt (lenN (0.08, fs)))
            {
                const double u = r.nextDouble();
                ev.push_back ({ at, u < 0.3 ? lis (-1) : (u < 0.6 ? lis (3) : (u < 0.8 ? lis (0) : set (bidI (3, "freq"), 300.0f + 3000.0f * r.nextFloat()))) });
            }
            const Buf ref = renderB2 (fs, in, ev, 0, 1);
            for (int alt : { -2, 1, 13, 4096 })
            {
                const Buf b = alt < 0 ? renderB2 (fs, in, ev, 0, -alt) : renderB2 (fs, in, ev, 1, alt);
                int first = -1; ++cases;
                if (countDiff (b, ref, 0, n, &first)) { ++bad; d << f (fs / 1000.0, 1) << "k/" << ch << "ch alt " << alt << " first " << first << "  "; }
            }
        }
    row (bad == 0, "B2", "block identity with auto gain and output gain away from unity", juce::String (cases - bad) + " / " + juce::String (cases) + " bit-identical  " + d);
}
//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    std::printf ("=== O-ParametricEQ verify-2.6 probe ===\n");
    rowT1();
    rowO1();
    rowI1();
    rowI2();
    rowR1();
    rowB1();
    rowA1();
    rowG1();
    rowN1();
    rowE1();
    rowX1();
    rowB2();
    std::printf ("\n%d PASS, %d FAIL\n", nPass, nFail);
    return nFail == 0 ? 0 : 1;
}
