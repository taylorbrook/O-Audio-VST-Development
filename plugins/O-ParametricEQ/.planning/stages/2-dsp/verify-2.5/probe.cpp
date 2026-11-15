// Independent probe for O-ParametricEQ Phase 2.5 verify (output stage, auto gain, seeding, idle, meter).
// Drives the real processor through its public API only (APVTS parameters, prepareToPlay, processBlock,
// processBlockBypassed, readAndResetOutputPeak, setNonRealtime). Includes nothing from Source/dsp directly
// and none of the harness code. References are re-stated here from PLAN-2.5 P2 / P3 / P5 / P6:
//   smoother  four one-pole stages, a = 1 - exp (-13.0622 / N), snap when all four within 1e-9 of T
//   N         round (0.020 fs) output gain, round (0.100 fs) auto gain
//   ticks     every K samples of the clock (K = 16 / 32 / 64), output target applied at the next tick
//   refresh   tick index multiple of R = max (1, round (1024 / 48000 fs / K)); auto_gain takes effect there
//   estimate  measured independently: |H|^2 of the plugin's own impulse response on the 512-midpoint grid
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

//==============================================================================
// Re-stated stage
//==============================================================================
static int gridK (double fs) { return fs <= 48000.0 ? 16 : (fs <= 96000.0 ? 32 : 64); }
static int lenN (double sec, double fs) { return std::max (1, (int) std::lround (sec * fs)); }
static int refreshR (double fs) { return std::max (1, (int) std::lround (1024.0 / 48000.0 * fs / gridK (fs))); }
static double ogTarget (double rawDb) { double d = std::isfinite (rawDb) ? rawDb : 0.0; d = std::clamp (d, -36.0, 36.0); return std::pow (10.0, d / 20.0); }
static double wRC (int k, int N) { return 0.5 - 0.5 * std::cos (kPi * (k + 1) / (N + 1)); }

struct Sm
{
    double y[4] { 1, 1, 1, 1 }, T = 1, a = 1; bool settled = true;
    void prepare (int N) { a = 1.0 - std::exp (-13.0622 / (double) N); }
    void snap (double t) { T = t; y[0] = y[1] = y[2] = y[3] = t; settled = true; }
    void setTarget (double t) { T = t; settled = (y[0] == t && y[1] == t && y[2] == t && y[3] == t); }
    double next()
    {
        if (settled) return T;
        y[0] += a * (T - y[0]); y[1] += a * (y[0] - y[1]); y[2] += a * (y[1] - y[2]); y[3] += a * (y[2] - y[3]);
        if (std::fabs (y[0] - T) <= 1e-9 && std::fabs (y[1] - T) <= 1e-9 && std::fabs (y[2] - T) <= 1e-9 && std::fabs (y[3] - T) <= 1e-9)
        { y[0] = y[1] = y[2] = y[3] = T; settled = true; }
        return y[3];
    }
};

// Per-sample gain of the re-stated stage. ogRaw: (sample, raw dB) changes; agWant: (sample, on) changes.
// agValue: the target while on. tickShift / rOverride: controls (shift the output tick, change R).
struct GainPlan
{
    std::vector<std::pair<int, double>> og;     // sorted, og[0].first == 0
    std::vector<std::pair<int, bool>> ag;       // sorted, ag[0].first == 0
    double agValue = 1.0;
    int ogTickShift = 0, agTickShift = 0, rOverride = 0, ogLenOverride = 0;
};
static std::vector<double> gainTrack (double fs, int n, const GainPlan& gp)
{
    const int K = gridK (fs), R = gp.rOverride > 0 ? gp.rOverride : refreshR (fs);
    Sm og, ag; og.prepare (gp.ogLenOverride > 0 ? gp.ogLenOverride : lenN (0.020, fs)); ag.prepare (lenN (0.100, fs));
    size_t io = 0, ia = 0;
    double pending = ogTarget (gp.og[0].second);
    bool wanted = gp.ag[0].second, on = wanted;
    og.snap (pending); ag.snap (on ? gp.agValue : 1.0);
    std::vector<double> g ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        while (io < gp.og.size() && gp.og[io].first <= i) pending = ogTarget (gp.og[io++].second);
        while (ia < gp.ag.size() && gp.ag[ia].first <= i) wanted = gp.ag[ia++].second;
        if ((i - gp.ogTickShift) >= 0 && (i - gp.ogTickShift) % K == 0) og.setTarget (pending);
        const int ia2 = i - gp.agTickShift;
        if (ia2 >= 0 && ia2 % K == 0 && (ia2 / K) % R == 0) { on = wanted; ag.setTarget (on ? gp.agValue : 1.0); }
        const double gA = ag.next();
        g[(size_t) i] = gA * og.next();
    }
    return g;
}

//==============================================================================
// Independent estimate from the plugin's own impulse response
//==============================================================================
struct AgGrid { int M; bool inclusive; double fTopCap; double topFrac; };
static std::vector<double> gridFreqs (double fs, AgGrid g)
{
    const double fTop = std::min (g.fTopCap, g.topFrac * fs);
    std::vector<double> fr ((size_t) g.M);
    for (int i = 0; i < g.M; ++i)
    {
        const double u = g.inclusive ? (double) i / (g.M - 1) : (i + 0.5) / g.M;
        fr[(size_t) i] = std::exp (std::log (40.0) + (std::log (fTop) - std::log (40.0)) * u);
    }
    return fr;
}
static std::vector<double> mag2At (const std::vector<double>& h, const std::vector<double>& fr, double fs)
{
    std::vector<double> out (fr.size());
    for (size_t k = 0; k < fr.size(); ++k)
    {
        const double w = 2.0 * kPi * fr[k] / fs;
        const std::complex<double> rot (std::cos (w), -std::sin (w));
        std::complex<double> z (1.0, 0.0), acc (0.0, 0.0);
        for (size_t i = 0; i < h.size(); ++i)
        {
            acc += h[i] * z; z *= rot;
            if ((i & 1023) == 1023) { const double ph = -w * (double) (i + 1); z = { std::cos (ph), std::sin (ph) }; }
        }
        out[k] = std::norm (acc);
    }
    return out;
}
struct IR { std::vector<double> L, R; };
static IR impulseResponse (double fs, int ch, const Fn& setup, int len)
{
    Buf imp (ch, len); imp.clear();
    for (int c = 0; c < ch; ++c) imp.setSample (c, 0, 1.0f);
    Opt o; o.block = 512;
    Buf out = render (fs, imp, all ({ setup, set ("auto_gain", 0.0f), set ("output_gain", 0.0f) }), {}, o);
    IR r; r.L.resize ((size_t) len); r.R.resize ((size_t) len);
    for (int i = 0; i < len; ++i) { r.L[(size_t) i] = out.getSample (0, i); r.R[(size_t) i] = out.getSample (ch - 1, i); }
    return r;
}
static double agDbFrom (const std::vector<double>& PL, const std::vector<double>& PR, bool clampIt = true)
{
    double s = 0; for (size_t i = 0; i < PL.size(); ++i) s += 0.5 * (PL[i] + PR[i]);
    const double mean = s / (double) PL.size();
    double d = -10.0 * std::log10 (mean);
    return clampIt ? std::clamp (d, -18.0, 12.0) : d;
}
// The realised auto gain: least squares ratio of (auto on) to (auto off), after both have settled
static double realisedAg (double fs, int ch, const Fn& setup)
{
    const int n = (int) (0.6 * fs);
    Buf in = noise (ch, n, 77);
    Opt o; o.block = 256;
    Buf on  = render (fs, in, all ({ setup, set ("auto_gain", 1.0f) }), {}, o);
    Buf off = render (fs, in, all ({ setup, set ("auto_gain", 0.0f) }), {}, o);
    double num = 0, den = 0;
    for (int c = 0; c < ch; ++c) for (int i = n / 2; i < n; ++i) { num += (double) on.getSample (c, i) * off.getSample (c, i); den += (double) off.getSample (c, i) * off.getSample (c, i); }
    return num / den;
}

//==============================================================================
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const bool quick = argc > 1 && std::strcmp (argv[1], "--quick") == 0;
    const double rates[] = { 22050.0, 32000.0, 44100.0, 48000.0, 96000.0, 192000.0 };

    std::printf ("=== O-ParametricEQ verify-2.5 probe ===\n");

    //--------------------------------------------------------------------------
    // T1: output gain step pairs, no band: bit-exact against the re-stated stage
    {
        const float dbs[] = { -36.0f, -12.0f, 0.0f, 12.0f, 36.0f };
        int cases = 0, bad = 0; juce::String worst;
        for (double fs : rates)
            for (int block : { 1, 7, 64, 512 })
            {
                if (block == 1 && fs > 48000.0) continue;
                const int n = lenN (0.15, fs), e = lenN (0.020, fs) + 5;   // edit off the tick grid
                Buf in = noise (2, n, 11);
                for (float a : dbs) for (float b : dbs)
                {
                    if (a == b) continue;
                    double rawA = 0, rawB = 0;
                    Opt o; o.block = block;
                    Buf out = render (fs, in, [&] (Proc& p) { setRaw (p, "output_gain", a); rawA = rawOf (p, "output_gain"); },
                                      { { e, [&] (Proc& p) { setRaw (p, "output_gain", b); rawB = rawOf (p, "output_gain"); } } }, o);
                    GainPlan gp; gp.og = { { 0, rawA }, { e, rawB } }; gp.ag = { { 0, false } };
                    auto g = gainTrack (fs, n, gp);
                    Buf ref (2, n);
                    for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) ref.setSample (c, i, (float) ((double) in.getSample (c, i) * g[(size_t) i]));
                    ++cases;
                    if (! bitEq (out, ref, 0, n)) { ++bad; if (worst.isEmpty()) worst = f (fs, 0) + " blk " + juce::String (block) + " " + f (a, 0) + "->" + f (b, 0); }
                }
            }
        row (bad == 0, "T1", "output gain steps, no band: bit-exact to re-stated P2/P5", juce::String (cases - bad) + " / " + juce::String (cases) + (bad ? "  first bad " + worst : ""));
    }

    // T1n: controls on one case: output tick one tick late; 20 ms length off by one; must differ
    {
        const double fs = 48000.0; const int n = lenN (0.15, fs), e = 965;
        Buf in = noise (2, n, 12); double rawA = 0, rawB = 0;
        Buf out = render (fs, in, [&] (Proc& p) { setRaw (p, "output_gain", 0.0f); rawA = rawOf (p, "output_gain"); },
                          { { e, [&] (Proc& p) { setRaw (p, "output_gain", 12.0f); rawB = rawOf (p, "output_gain"); } } });
        int fired = 0;
        for (int k = 0; k < 3; ++k)
        {
            // k 0: applied one tick late; 1: N + 1; 2: applied at the edit sample instead of the next tick
            GainPlan gp; gp.og = { { 0, rawA }, { k == 0 ? e + gridK (fs) : e, rawB } }; gp.ag = { { 0, false } };
            if (k == 1) gp.ogLenOverride = lenN (0.020, fs) + 1;
            if (k == 2) gp.ogTickShift = e % gridK (fs);
            auto g = gainTrack (fs, n, gp);
            Buf ref (2, n);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) ref.setSample (c, i, (float) ((double) in.getSample (c, i) * g[(size_t) i]));
            fired += bitEq (out, ref, 0, n) ? 0 : 1;
        }
        row (fired == 3, "T1n", "controls: one tick late, N + 1, at the edit sample", juce::String (fired) + " / 3 differ");
    }

    //--------------------------------------------------------------------------
    // T3: random output-gain automation, random blocks 1..2048, 10 s per rate, mono and stereo: bit-exact
    {
        int cases = 0, bad = 0, events = 0; juce::String detail;
        for (double fs : rates)
            for (int ch : { 1, 2 })
            {
                const int n = (int) ((quick ? 2.0 : 10.0) * fs);
                Buf in = noise (ch, n, 21 + ch);
                juce::Random r (1000 + (juce::int64) fs + ch);
                std::vector<int> at; for (int i = 1; i < n; i += 1 + r.nextInt ((int) (0.03 * fs))) at.push_back (i);
                std::vector<float> val; for (size_t k = 0; k < at.size(); ++k)
                {
                    const double u = r.nextDouble();
                    val.push_back (u < 0.05f ? -36.0f : u < 0.10 ? 36.0f : u < 0.25 ? 0.0f : (float) (-36.0 + 72.0 * r.nextDouble()));
                }
                std::vector<std::pair<int, double>> log { { 0, 0.0 } };
                std::vector<Ev> evs;
                for (size_t k = 0; k < at.size(); ++k)
                    evs.push_back ({ at[k], [&, k] (Proc& p) { setRaw (p, "output_gain", val[k]); log.push_back ({ at[k], rawOf (p, "output_gain") }); } });
                Opt o; o.randomBlocks = true; o.seed = 5 + ch;
                Buf out = render (fs, in, nullptr, evs, o);
                GainPlan gp; gp.og = log; gp.ag = { { 0, false } };
                auto g = gainTrack (fs, n, gp);
                Buf ref (ch, n);
                for (int c = 0; c < ch; ++c) for (int i = 0; i < n; ++i) ref.setSample (c, i, (float) ((double) in.getSample (c, i) * g[(size_t) i]));
                ++cases; events += (int) at.size();
                int first = -1; const int d = countDiff (out, ref, 0, n, &first);
                if (d) { ++bad; detail << " [" << f (fs, 0) << " ch" << ch << ": " << d << " diff from " << first << "]"; }
            }
        row (bad == 0, "T3", "random output-gain automation, random blocks: bit-exact", juce::String (cases - bad) + " / " + juce::String (cases) + " renders, " + juce::String (events) + " events" + detail);
    }

    //--------------------------------------------------------------------------
    // E1: the estimator against the plugin's own response on the 512-midpoint grid
    struct ECase { const char* name; double fs; int ch; Fn setup; int irLen; };
    std::vector<ECase> ec = {
        { "G12 LS 200 +12 12dB Q 1 (harness form) 48k", 48000, 2, band (1, 1, 200, 12, 1.0f, 1), 1 << 15 },
        { "G12 LS 200 +12 12dB Q 0.7071 48k", 48000, 2, band (1, 1, 200, 12, 0.7071f, 1), 1 << 15 },
        { "G12 LS 200 +12 12dB 44.1k", 44100, 2, band (1, 1, 200, 12, 0.7071f, 1), 1 << 15 },
        { "G12 LS 200 +12 12dB 96k", 96000, 2, band (1, 1, 200, 12, 0.7071f, 1), 1 << 16 },
        { "G12 LS 200 +12 12dB 192k", 192000, 2, band (1, 1, 200, 12, 0.7071f, 1), 1 << 17 },
        { "G12 LS 200 +12 12dB 22.05k", 22050, 2, band (1, 1, 200, 12, 0.7071f, 1), 1 << 14 },
        { "Bell 1k +6 Q1", 48000, 2, band (1, 0, 1000, 6, 1.0f), 1 << 15 },
        { "Bell 3k -9 Q4", 48000, 2, band (1, 0, 3000, -9, 4.0f), 1 << 15 },
        { "HS 8k +6 12dB", 48000, 2, band (1, 2, 8000, 6, 0.7071f, 1), 1 << 14 },
        { "HS 6k +12 12dB 22.05k (fTop 9.9k)", 22050, 2, band (1, 2, 6000, 12, 0.7071f, 1), 1 << 14 },
        { "LowCut 100 24dB", 48000, 2, band (1, 3, 100, 0, 0.7071f, 3), 1 << 16 },
        { "HighCut 5k 48dB", 48000, 2, band (1, 4, 5000, 0, 0.7071f, 5), 1 << 15 },
        { "Left Bell 1k +12 Q1 (stereo lanes)", 48000, 2, band (1, 0, 1000, 12, 1.0f, 1, 1), 1 << 15 },
        { "Right LS 300 +9", 48000, 2, band (1, 1, 300, 9, 0.7071f, 1, 2), 1 << 15 },
        { "Mid Bell 1k +12 Q1", 48000, 2, band (1, 0, 1000, 12, 1.0f, 1, 3), 1 << 15 },
        { "mono, Side Bell 1k +12 Q1", 48000, 1, band (1, 0, 1000, 12, 1.0f, 1, 4), 1 << 15 },
        { "two bands: Bell 1k +6, LS 100 -6", 48000, 2, all ({ band (1, 0, 1000, 6, 1.0f), band (2, 1, 100, -6, 0.7071f, 1) }), 1 << 16 },
        { "Tilt 1k +6", 48000, 2, band (1, 7, 1000, 6, 0.7071f, 1), 1 << 15 },
        { "Notch 2k Q2 + BP 500 Q1 (L / R)", 48000, 2, all ({ band (1, 5, 2000, 0, 2.0f, 1, 1), band (2, 6, 500, 0, 1.0f, 1, 2) }), 1 << 16 },
    };
    {
        double worst = 0; int bad = 0; juce::String lines;
        for (auto& c : ec)
        {
            const IR ir = impulseResponse (c.fs, c.ch, c.setup, c.irLen);
            const auto fr = gridFreqs (c.fs, { 512, false, 16000.0, 0.45 });
            const auto PL = mag2At (ir.L, fr, c.fs), PR = mag2At (ir.R, fr, c.fs);
            const double truth = agDbFrom (PL, PR);
            const double got = 20.0 * std::log10 (realisedAg (c.fs, c.ch, c.setup));
            const double d = std::fabs (got - truth);
            worst = std::max (worst, d); if (d > 0.001) ++bad;
            // alternatives that the formula rules out
            const auto fI = gridFreqs (c.fs, { 512, true, 16000.0, 0.45 });
            const auto f64 = gridFreqs (c.fs, { 64, false, 16000.0, 0.45 });
            const auto f16 = gridFreqs (c.fs, { 512, false, 16000.0, 0.5 });
            const double incl = agDbFrom (mag2At (ir.L, fI, c.fs), mag2At (ir.R, fI, c.fs));
            const double g64  = agDbFrom (mag2At (ir.L, f64, c.fs), mag2At (ir.R, f64, c.fs));
            const double top5 = agDbFrom (mag2At (ir.L, f16, c.fs), mag2At (ir.R, f16, c.fs));
            info ("E1", c.name, "plugin " + f (got, 4) + " dB, IR truth " + f (truth, 4) + " (diff " + f (d, 5) + "); inclusive " + f (incl, 4)
                                    + ", 64 pts " + f (g64, 4) + ", fTop 0.5 fs " + f (top5, 4));
        }
        row (bad == 0, "E1", "auto gain = IR-measured 512-midpoint pink mean", juce::String ((int) ec.size() - bad) + " / " + juce::String ((int) ec.size()) + " within 0.001 dB, worst " + f (worst, 5) + " dB");
    }

    // E2: Side band on stereo contributes nothing: auto gain on == off, bit for bit
    {
        const double fs = 48000; const int n = (int) (0.5 * fs);
        Buf in = noise (2, n, 31);
        Buf a = render (fs, in, all ({ band (1, 0, 1000, 12, 1.0f, 1, 4), set ("auto_gain", 1.0f) }));
        Buf b = render (fs, in, all ({ band (1, 0, 1000, 12, 1.0f, 1, 4), set ("auto_gain", 0.0f) }));
        Buf c = render (fs, in, all ({ band (1, 0, 1000, 12, 1.0f, 1, 3), set ("auto_gain", 1.0f) }));
        Buf d = render (fs, in, all ({ band (1, 0, 1000, 12, 1.0f, 1, 3), set ("auto_gain", 0.0f) }));
        row (bitEq (a, b, 0, n) && ! bitEq (c, d, 0, n), "E2", "Side band: auto on == off (Mid control differs)", juce::String (bitEq (a, b, 0, n) ? "equal" : "DIFFER") + "; control " + (bitEq (c, d, 0, n) ? "EQUAL" : "differs"));
    }

    // E3: clamps
    {
        const double fs = 48000;
        const double hi = realisedAg (fs, 2, all ({ band (1, 3, 8000, 0, 0.7071f, 7), band (2, 4, 9000, 0, 0.7071f, 7) }));
        const double lo = realisedAg (fs, 2, all ({ band (1, 0, 1000, 30, 0.1f), band (2, 0, 3000, 30, 0.1f) }));
        const double eHi = std::fabs (hi / std::pow (10.0, 12.0 / 20.0) - 1.0), eLo = std::fabs (lo / std::pow (10.0, -18.0 / 20.0) - 1.0);
        row (eHi < 1e-5 && eLo < 1e-5, "E3", "clamps: +12 dB (narrow pass), -18 dB (two +30 dB bells)",
             f (20 * std::log10 (hi), 5) + " / " + f (20 * std::log10 (lo), 5) + " dB");
    }

    //--------------------------------------------------------------------------
    // T2: auto-gain toggle trajectory and refresh schedule. Left-only band: the R lane is the input times the
    // gain, so R out = float (in_R * g) with g re-stated (target measured in a seeded render).
    {
        int cases = 0, bad = 0, fired = 0, controls = 0; double worst = 0; juce::String detail;
        for (double fs : rates)
            for (int block : { 32, 512 })
            {
                const Fn setup = band (1, 0, 1000, 12, 1.0f, 1, 1);
                const int n = lenN (0.9, fs);
                Buf in = noise (2, n, 41);
                // the target, from a session seeded with auto gain on: R_out / R_in
                double T = 0;
                {
                    Buf s = render (fs, in, all ({ setup, set ("auto_gain", 1.0f) }));
                    std::vector<double> rr; for (int i = 0; i < n; i += 97) if (std::fabs (in.getSample (1, i)) > 0.1) rr.push_back ((double) s.getSample (1, i) / in.getSample (1, i));
                    std::sort (rr.begin(), rr.end()); T = rr[rr.size() / 2];
                }
                const int e1 = lenN (0.031, fs) + 3, e2 = lenN (0.45, fs) + 7;
                Opt o; o.block = block;
                Buf out = render (fs, in, all ({ setup, set ("auto_gain", 0.0f) }),
                                  { { e1, set ("auto_gain", 1.0f) }, { e2, set ("auto_gain", 0.0f) } }, o);
                auto refFor = [&] (GainPlan gp) {
                    gp.og = { { 0, 0.0 } }; gp.ag = { { 0, false }, { e1, true }, { e2, false } }; gp.agValue = T;
                    auto g = gainTrack (fs, n, gp);
                    std::vector<std::vector<double>> ref (2, std::vector<double> ((size_t) n));
                    for (int i = 0; i < n; ++i) ref[1][(size_t) i] = (double) (float) ((double) in.getSample (1, i) * g[(size_t) i]);
                    return relErr (out, ref, 0, n, 1, 1);
                };
                const double err = refFor ({});
                ++cases; worst = std::max (worst, err);
                if (err > 1e-6) { ++bad; detail << " [" << f (fs, 0) << "/" << block << " " << db (err) << "]"; }
                GainPlan c1; c1.agTickShift = gridK (fs);
                GainPlan c2; c2.rOverride = refreshR (fs) - 1;
                controls += 2; fired += (refFor (c1) > 1e-6 ? 1 : 0) + (refFor (c2) > 1e-6 ? 1 : 0);
            }
        row (bad == 0, "T2", "auto-gain toggle: refresh tick, 100 ms smoother (R lane)", juce::String (cases - bad) + " / " + juce::String (cases) + ", worst " + db (worst) + " re peak" + detail);
        row (fired == controls, "T2n", "controls: refresh one tick late, R - 1", juce::String (fired) + " / " + juce::String (controls) + " differ");
    }

    //--------------------------------------------------------------------------
    // S1: seeded at session open
    {
        int bad = 0; juce::String d;
        for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
        {
            const int n = lenN (0.3, fs);
            Buf in = noise (2, n, 51);
            // (a) no band, +12 dB: from sample 0 float (in * 10^(raw/20))
            double raw = 0;
            Buf a = render (fs, in, [&] (Proc& p) { setRaw (p, "output_gain", 12.0f); raw = rawOf (p, "output_gain"); });
            Buf ra (2, n); const double g = ogTarget (raw);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) ra.setSample (c, i, (float) ((double) in.getSample (c, i) * g));
            if (! bitEq (a, ra, 0, n)) { ++bad; d << " (a)" << f (fs, 0); }
            // (b) Left +12 bell, auto on, +12 dB: R lane is a constant from sample 0
            Buf b = render (fs, in, all ({ band (1, 0, 1000, 12, 1.0f, 1, 1), set ("auto_gain", 1.0f), set ("output_gain", 12.0f) }));
            std::vector<double> rr; for (int i = n / 2; i < n; i += 13) if (std::fabs (in.getSample (1, i)) > 0.1) rr.push_back ((double) b.getSample (1, i) / in.getSample (1, i));
            std::sort (rr.begin(), rr.end()); const double G = rr[rr.size() / 2];
            std::vector<std::vector<double>> ref (2, std::vector<double> ((size_t) n));
            for (int i = 0; i < n; ++i) ref[1][(size_t) i] = (double) in.getSample (1, i) * G;
            const double err = relErr (b, ref, 0, lenN (0.050, fs), 1, 1);
            if (err > 1e-6) { ++bad; d << " (b)" << f (fs, 0) << " " << db (err); }
        }
        row (bad == 0, "S1", "seeded at open: +12 dB output, auto gain: constant from 0", bad == 0 ? juce::String ("8 / 8") : d);
    }

    // S2: un-bypass after the output gain changed while fully bypassed, no band; by parameter and by processBlockBypassed
    // S3: latched un-bypass mid-block (block 512): gain edit during the fade to bypass, restart seeded at e + N
    for (int route = 0; route < 2; ++route)
    {
        const double fs = 48000; const int N = lenN (0.020, fs), n = lenN (0.5, fs);
        Buf in = noise (2, n, 61);
        // S2
        {
            const int b0 = 4800, ch = 9600, u = 14400;
            double r6 = 0, r12 = 0;
            std::vector<Ev> evs = { { ch, [&] (Proc& p) { setRaw (p, "output_gain", 12.0f); r12 = rawOf (p, "output_gain"); } } };
            Opt o; o.block = 64;
            if (route == 0) { evs.push_back ({ b0, set ("bypass", 1.0f) }); evs.push_back ({ u, set ("bypass", 0.0f) }); }
            else o.viaBypassed = [=] (int s) { return s >= b0 && s < u; };
            Buf out = render (fs, in, [&] (Proc& p) { setRaw (p, "output_gain", 6.0f); r6 = rawOf (p, "output_gain"); }, evs, o);
            const double g6 = ogTarget (r6), g12 = ogTarget (r12);
            double err = 0, pk = 1e-30;
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < n; ++i)
                {
                    const double x = in.getSample (c, i);
                    double want;
                    if (i < b0) want = x * g6;
                    else if (i < b0 + N) { const double a = x * g6; want = a + wRC (i - b0, N) * (x - a); }
                    else if (i < u) want = x;
                    else if (i < u + N) want = x + wRC (i - u, N) * (x * g12 - x);
                    else want = (double) (float) (x * g12);
                    err = std::max (err, std::fabs (out.getSample (c, i) - want)); pk = std::max (pk, std::fabs (want));
                }
            Buf tail (2, n); for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) tail.setSample (c, i, (float) ((double) in.getSample (c, i) * g12));
            const bool exact = bitEq (out, tail, u + N, n);
            row (err / pk < 1e-6 && exact, route ? "S2b" : "S2", route ? "un-bypass via processBlockBypassed, gain edited while bypassed" : "un-bypass by parameter, gain edited while bypassed",
                 "blend err " + db (err / pk) + "; from u + N bit-exact: " + (exact ? "yes" : "NO"));
        }
        // S3
        {
            const int e = 5120, edit = e + 512;     // edit inside the fade to bypass (N = 960); restart at e + N mid-block
            double r6 = 0, r12 = 0;
            std::vector<Ev> evs = { { edit, [&] (Proc& p) { setRaw (p, "output_gain", 12.0f); r12 = rawOf (p, "output_gain"); } } };
            Opt o; o.block = 512;
            if (route == 0) { evs.push_back ({ e, set ("bypass", 1.0f) }); evs.push_back ({ edit, set ("bypass", 0.0f) }); }
            else o.viaBypassed = [=] (int s) { return s >= e && s < edit; };
            Buf out = render (fs, in, [&] (Proc& p) { setRaw (p, "output_gain", 6.0f); r6 = rawOf (p, "output_gain"); }, evs, o);
            GainPlan gp; gp.og = { { 0, r6 }, { edit, r12 } }; gp.ag = { { 0, false } };
            auto g = gainTrack (fs, n, gp);
            const double g12 = ogTarget (r12);
            auto score = [&] (int late, bool rampedB) {
                double err = 0, pk = 1e-30;
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < e + 3 * N; ++i)
                    {
                        const double x = in.getSample (c, i), A = x * g[(size_t) i];
                        const int r0 = e + N + late;
                        double want;
                        if (i < e) want = A;
                        else if (i < e + N) want = A + wRC (i - e, N) * (x - A);
                        else if (i < r0) want = x;
                        else { const double B = rampedB ? A : x * g12; want = i < r0 + N ? x + wRC (i - r0, N) * (B - x) : B; }
                        err = std::max (err, std::fabs (out.getSample (c, i) - want)); pk = std::max (pk, std::fabs (want));
                    }
                return err / pk;
            };
            const double err = score (0, false), cLate = score (1, false), cRamp = score (0, true);
            row (err < 1e-6 && cLate > 1e-6 && cRamp > 1e-6, route ? "S3b" : "S3", route ? "latched un-bypass mid-block, processBlockBypassed route" : "latched un-bypass mid-block (512), gain edit in the fade",
                 "err " + db (err) + "; controls: fade 1 sample late " + db (cLate) + ", unseeded ramp " + db (cRamp));
        }
    }

    //--------------------------------------------------------------------------
    // I1: back to idle through auto gain: a +6 dB bell taken to 0 dB with auto gain on. The band sleeps,
    // the estimate goes to 1.0, the smoother snaps; from then the output is the input bit for bit,
    // a subnormal tail included. Mono and stereo, 44.1 / 48 / 96 kHz.
    {
        int bad = 0; juce::String d;
        for (double fs : { 44100.0, 48000.0, 96000.0 })
            for (int ch : { 1, 2 })
            {
                const int n = lenN (1.0, fs), e = lenN (0.2, fs);
                Buf in = noise (ch, n, 71); denormTail (in, lenN (0.8, fs), 72);
                Buf out = render (fs, in, all ({ band (1, 0, 1000, 6, 1.0f), set ("auto_gain", 1.0f) }), { { e, set (bid (1, "gain"), 0.0f) } });
                int last = -1; countDiff (out, in, 0, n, nullptr, &last);
                const double ms = (last + 1 - e) * 1000.0 / fs;
                if (last >= lenN (0.8, fs) || ms > 400.0) ++bad;
                d << f (fs / 1000, 1) << "k/" << ch << "ch " << f (ms, 1) << " ms  ";
            }
        row (bad == 0, "I1", "auto gain returns to idle: input bit-exact after (ms)", d);
    }

    // I2: G4 (a) with auto gain on, no band; and 24 bands with gain_scale 0 and auto on: memcmp from sample 0
    {
        int cases = 0, bad = 0;
        for (double fs : { 44100.0, 48000.0, 192000.0 })
            for (int ch : { 1, 2 })
                for (int block : { 1, 16, 64, 512, 4096, -1 })
                {
                    if (block == 1 && fs > 48000) continue;
                    const int n = lenN (0.4, fs);
                    Buf in = noise (ch, n, 81); denormTail (in, n / 2, 82);
                    Opt o; if (block < 0) { o.randomBlocks = true; o.maxBlock = 4096; } else o.block = block;
                    Buf a = render (fs, in, set ("auto_gain", 1.0f), {}, o);
                    std::vector<Fn> bs; for (int b = 1; b <= 24; ++b) bs.push_back (band (b, b % 3, 100.0f * b, 12.0f, 2.0f));
                    bs.push_back (set ("gain_scale", 0.0f)); bs.push_back (set ("auto_gain", 1.0f));
                    Buf b = render (fs, in, all (bs), {}, o);
                    cases += 2; bad += (bitEq (a, in, 0, n) ? 0 : 1) + (bitEq (b, in, 0, n) ? 0 : 1);
                }
        row (bad == 0, "I2", "G4 (a) auto on; 24 bands at gain scale 0, auto on", juce::String (cases - bad) + " / " + juce::String (cases) + " memcmp equal");
    }

    // I3 (INFO): toggling auto gain with nothing to compensate on a subnormal input
    {
        const double fs = 48000; const int n = lenN (0.3, fs);
        Buf in (2, n); denormTail (in, 0, 91);
        Buf out = render (fs, in, nullptr, { { 4803, set ("auto_gain", 1.0f) }, { 9600, set ("auto_gain", 0.0f) } });
        int first = -1, last = -1; const int k = countDiff (out, in, 0, n, &first, &last);
        info ("I3", "auto-gain toggles, no band, subnormal input", juce::String (k) + " samples differ" + (k ? " in [" + juce::String (first) + ", " + juce::String (last) + "]" : juce::String()));
    }

    //--------------------------------------------------------------------------
    // B1: block identity with gain, auto-gain and band events (refreshes skipped in idle blocks), and offline
    {
        int bad = 0; juce::String d;
        for (double fs : { 44100.0, 48000.0, 96000.0 })
        {
            const int n = (int) ((quick ? 2.0 : 6.0) * fs);
            Buf in = noise (2, n, 101);
            juce::Random r (7 + (juce::int64) fs);
            std::vector<Ev> evs;
            for (int at = 2048; at < n; at += 2048 * (1 + r.nextInt (3)))
            {
                const int what = r.nextInt (6);
                const float v = (float) r.nextDouble();
                switch (what)
                {
                    case 0: evs.push_back ({ at, set ("output_gain", v < 0.4f ? 0.0f : -24.0f + 48.0f * v) }); break;
                    case 1: evs.push_back ({ at, set ("auto_gain", v < 0.5f ? 0.0f : 1.0f) }); break;
                    case 2: evs.push_back ({ at, set (bid (1, "enabled"), v < 0.5f ? 0.0f : 1.0f) }); break;
                    case 3: evs.push_back ({ at, set (bid (1, "gain"), v < 0.3f ? 0.0f : -12.0f + 24.0f * v) }); break;
                    case 4: evs.push_back ({ at, set (bid (2, "freq"), 50.0f + 2000.0f * v) }); break;
                    default: evs.push_back ({ at, set (bid (2, "enabled"), v < 0.5f ? 0.0f : 1.0f) }); break;
                }
            }
            const Fn setup = all ({ band (1, 0, 1000, 6, 1.0f), band (2, 1, 200, -6, 0.7071f, 1, 1), set (bid (2, "enabled"), 0.0f), set ("auto_gain", 1.0f) });
            Opt o32; o32.block = 32;
            Buf ref = render (fs, in, setup, evs, o32);
            std::vector<std::pair<const char*, Opt>> vs;
            { Opt o; o.block = 2048; vs.push_back ({ "2048", o }); }
            { Opt o; o.block = 64; o.nonRealtime = true; vs.push_back ({ "64 offline", o }); }
            { Opt o; o.randomBlocks = true; o.maxBlock = 2048; o.seed = 3; vs.push_back ({ "random", o }); }
            if (fs <= 48000) { Opt o; o.block = 1; vs.push_back ({ "1", o }); }
            for (auto& v : vs)
                if (! bitEq (render (fs, in, setup, evs, v.second), ref, 0, n)) { ++bad; d << " " << f (fs, 0) << "/" << v.first; }
            const bool live = ! bitEq (render (fs, in, setup, {}, o32), ref, 0, n);
            if (! live) { ++bad; d << " " << f (fs, 0) << " events had no effect"; }
            d << " " << f (fs / 1000, 1) << "k:" << (int) evs.size() << "ev";
        }
        row (bad == 0, "B1", "block identity with gain / auto / band events; offline", bad ? "FAIL" + d : "all bit-identical;" + d);
    }

    //--------------------------------------------------------------------------
    // M1: meter: max-hold between reads, reset on read, both routes, mono channel 1 reads 0
    {
        const double fs = 48000; int ok = 0, total = 0; juce::String d;
        for (int route = 0; route < 2; ++route)
        {
            Buf in (2, 32 * 50); in.clear(); in.setSample (1, 32 * 17 + 5, 2.0f); in.setSample (0, 32 * 30, -0.25f);
            std::unique_ptr<Proc> p;
            Opt o; o.block = 32; if (route) o.viaBypassed = [] (int) { return true; };
            render (fs, in, nullptr, {}, o, &p);
            const float l = p->readAndResetOutputPeak (0), r = p->readAndResetOutputPeak (1), r2 = p->readAndResetOutputPeak (1);
            total += 3; ok += (l == 0.25f) + (r == 2.0f) + (r2 == 0.0f);
            d << (route ? " bypassed: " : "processed: ") << l << " / " << r << " / " << r2;
        }
        {
            Buf in (1, 64); in.clear(); in.setSample (0, 3, 0.5f);
            std::unique_ptr<Proc> p; render (fs, in, nullptr, {}, {}, &p);
            const float m0 = p->readAndResetOutputPeak (0), m1 = p->readAndResetOutputPeak (1), m9 = p->readAndResetOutputPeak (9);
            total += 3; ok += (m0 == 0.5f) + (m1 == 0.0f) + (m9 == 0.0f);
            d << "; mono " << m0 << " / " << m1 << " / " << m9;
        }
        row (ok == total, "M1", "meter max-hold, reset, routes, mono", d);
    }

    std::printf ("\n%d PASS, %d FAIL\n", nPass, nFail);
    return nFail == 0 ? 0 : 1;
}
