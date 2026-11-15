// Independent probe for O-ParametricEQ Phase 2.4 verify (structural crossfades, soft bypass, sleep).
// Drives the real processor through its public API only (APVTS parameters, prepareToPlay, processBlock,
// processBlockBypassed, get/setStateInformation). Every reference is built from separate fresh instances
// of the same processor plus formulas re-stated here (N = round (0.020 fs), w(n) = 0.5 - 0.5 cos (pi (n+1) / (N+1))).
// It includes nothing from Source/dsp directly and none of the harness code.
#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

using Proc = OuariconParametricEQAudioProcessor;
using Buf  = juce::AudioBuffer<float>;
using Fn   = std::function<void (Proc&)>;

static int nPass = 0, nFail = 0;
static const double kPi = juce::MathConstants<double>::pi;

static void row (bool ok, const char* id, const char* name, const juce::String& detail)
{
    std::printf ("[%s] %-6s %-58s %s\n", ok ? "PASS" : "FAIL", id, name, detail.toRawUTF8());
    std::fflush (stdout);
    (ok ? nPass : nFail)++;
}
static void info (const char* id, const char* name, const juce::String& detail)
{
    std::printf ("[INFO] %-6s %-58s %s\n", id, name, detail.toRawUTF8());
    std::fflush (stdout);
}

//==============================================================================
// Parameters
//==============================================================================
static void setRaw (Proc& p, const juce::String& id, float raw)
{
    auto* prm = p.parameters.getParameter (id);
    if (prm == nullptr) { std::printf ("NO PARAM %s\n", id.toRawUTF8()); std::exit (2); }
    prm->setValueNotifyingHost (prm->convertTo0to1 (raw));
}
static juce::String bid (int band, const char* suffix) { return "b" + juce::String (band).paddedLeft ('0', 2) + "_" + suffix; }

// type: 0 Bell 1 LowShelf 2 HighShelf 3 LowCut 4 HighCut 5 Notch 6 BandPass 7 Tilt
// slope: 0 = 6, 1 = 12, 2 = 18, 3 = 24, 4 = 36, 5 = 48, 6 = 72, 7 = 96 dB/oct ; placement: 0 St 1 L 2 R 3 Mid 4 Side
static Fn band (int b, int type, float f, float g, float q = 1.0f, int slope = 1, int place = 0)
{
    return [=] (Proc& p)
    {
        setRaw (p, bid (b, "type"), (float) type); setRaw (p, bid (b, "freq"), f); setRaw (p, bid (b, "gain"), g);
        setRaw (p, bid (b, "q"), q); setRaw (p, bid (b, "slope"), (float) slope); setRaw (p, bid (b, "placement"), (float) place);
        setRaw (p, bid (b, "bypass"), 0.0f); setRaw (p, bid (b, "enabled"), 1.0f);
    };
}
static Fn set (const juce::String& id, float v) { return [=] (Proc& p) { setRaw (p, id, v); }; }
static Fn all (std::vector<Fn> fs) { return [=] (Proc& p) { for (auto& f : fs) if (f) f (p); }; }
static Fn none() { return [] (Proc&) {}; }

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
// Rendering
//==============================================================================
struct Ev { int at; Fn f; };
struct Opt
{
    int block = 64;
    int prepared = 0;           // 0: = block
    int from = 0;               // fresh instance fed input[from ...); output[0, from) = 0
    std::function<bool (int start)> viaBypassed;
};

// Events whose position falls in [start, start + block) are applied at that block's start (host semantics).
static Buf render (double fs, const Buf& in, const Fn& setup, const std::vector<Ev>& evs = {}, Opt o = {}, Proc** keep = nullptr,
                   std::unique_ptr<Proc>* own = nullptr)
{
    const int n = in.getNumSamples(), ch = in.getNumChannels();
    auto p = make (fs, ch, o.prepared > 0 ? o.prepared : o.block);
    if (setup) setup (*p);
    Buf buf (ch, n); buf.clear();
    for (int c = 0; c < ch; ++c) buf.copyFrom (c, o.from, in, c, o.from, n - o.from);
    juce::MidiBuffer midi;
    for (int start = o.from; start < n; start += o.block)
    {
        const int len = juce::jmin (o.block, n - start);
        for (const auto& e : evs) if (e.at >= start && e.at < start + len) e.f (*p);
        Buf view (buf.getArrayOfWritePointers(), ch, start, len);
        if (o.viaBypassed && o.viaBypassed (start)) p->processBlockBypassed (view, midi); else p->processBlock (view, midi);
    }
    if (keep) *keep = p.get();
    if (own) *own = std::move (p);
    return buf;
}

static Buf noise2 (int n, juce::int64 seed, double amp = 0.5)
{
    juce::Random r (seed); Buf b (2, n);
    for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) b.setSample (c, i, (float) (amp * (2.0 * r.nextDouble() - 1.0)));
    return b;
}
static Buf tone2 (int n, double f, double fs, double aL = 0.5, double aR = 0.3, double ph = 0.7)
{
    Buf b (2, n); const double w = 2.0 * kPi * f / fs;
    for (int i = 0; i < n; ++i) { b.setSample (0, i, (float) (aL * std::sin (w * i))); b.setSample (1, i, (float) (aR * std::sin (w * i + ph))); }
    return b;
}
static Buf denorm2 (int n, juce::int64 seed)       // every sample a nonzero subnormal float
{
    juce::Random r (seed); Buf b (2, n);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < n; ++i)
        {
            float v = std::ldexp ((float) (0.25 + 0.75 * r.nextDouble()), -128);
            b.setSample (c, i, r.nextBool() ? v : -v);
        }
    return b;
}

//==============================================================================
// Measures
//==============================================================================
static int fadeN (double fs) { return juce::jmax (1, (int) std::lround (0.020 * fs)); }
static double wRC (int n, int N) { return 0.5 - 0.5 * std::cos (kPi * (n + 1) / (N + 1)); }
static double wLin (int n, int N) { return (double) (n + 1) / (N + 1); }

static bool bitEq (const Buf& a, const Buf& b, int from, int to)
{
    if (to <= from) return true;
    for (int c = 0; c < a.getNumChannels(); ++c)
        if (std::memcmp (a.getReadPointer (c) + from, b.getReadPointer (c) + from, sizeof (float) * (size_t) (to - from)) != 0) return false;
    return true;
}
// One past the last index in [from, to) where a and b differ (both channels); `from` if none.
static int equalFrom (const Buf& a, const Buf& b, int from, int to)
{
    int last = from - 1;
    for (int c = 0; c < a.getNumChannels(); ++c)
        for (int i = to - 1; i > last; --i)
            if (std::memcmp (a.getReadPointer (c) + i, b.getReadPointer (c) + i, sizeof (float)) != 0) { last = i; break; }
    return last + 1;
}
static double peakOf (const Buf& a, int from, int to)
{
    double m = 0; for (int c = 0; c < a.getNumChannels(); ++c) for (int i = from; i < to; ++i) m = std::max (m, std::fabs ((double) a.getSample (c, i))); return m;
}
static double maxDiff (const Buf& a, const Buf& b, int from, int to)
{
    double m = 0; for (int c = 0; c < a.getNumChannels(); ++c) for (int i = from; i < to; ++i) m = std::max (m, std::fabs ((double) a.getSample (c, i) - (double) b.getSample (c, i))); return m;
}
// max |P - (A + w (B - A))| over [e, e + N) / peak of A, B there
static double blendErr (const Buf& P, const Buf& A, const Buf& B, int e, int N, bool linear = false, int bOffset = 0)
{
    double m = 0, pk = 1e-30;
    for (int c = 0; c < P.getNumChannels(); ++c)
        for (int k = 0; k < N; ++k)
        {
            const int i = e + k;
            const double a = A.getSample (c, i), b = B.getSample (c, i + bOffset);
            const double w = linear ? wLin (k, N) : wRC (k, N);
            m = std::max (m, std::fabs ((double) P.getSample (c, i) - (a + w * (b - a))));
            pk = std::max (pk, std::max (std::fabs (a), std::fabs (b)));
        }
    return m / pk;
}
static double maxD2 (const Buf& a, int from, int to)
{
    double m = 0;
    for (int c = 0; c < a.getNumChannels(); ++c)
    {
        const float* x = a.getReadPointer (c);
        for (int i = std::max (from, 2); i < to; ++i) m = std::max (m, std::fabs ((double) x[i] - 2.0 * x[i - 1] + (double) x[i - 2]));
    }
    return m;
}
static Buf splice (const Buf& A, const Buf& B, int e)
{
    Buf s; s.makeCopyOf (A);
    for (int c = 0; c < A.getNumChannels(); ++c) s.copyFrom (c, e, B, c, e, A.getNumSamples() - e);
    return s;
}
static int countNonFinite (const Buf& a, int from, int to, int* last = nullptr)
{
    int k = 0; if (last) *last = -1;
    for (int c = 0; c < a.getNumChannels(); ++c) for (int i = from; i < to; ++i) if (! std::isfinite (a.getSample (c, i))) { ++k; if (last) *last = std::max (*last, i); }
    return k;
}
static int roundTo (double x, int m) { return m * (int) std::lround (x / m); }
static juce::String f (const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
static juce::String f (const char* fmt, ...)
{
    char buf[1024]; va_list ap; va_start (ap, fmt); std::vsnprintf (buf, sizeof buf, fmt, ap); va_end (ap); return juce::String (buf);
}

// Float rounding: P is one rounding of a double blend; A, B one rounding each. Worst ~1.5 ulp of the peak
// (1.8e-7). Limit 1e-6 of the peak (about 8 ulps).
static const double kBlendTol = 1e-6;
static double gLinMin = 1e9;

//==============================================================================
// 1 + 2. Crossfade timing and shape
//==============================================================================
struct FadeResult { bool pre, post; int eqFrom; double rc, lin; };

static FadeResult fadeCase (double fs, int block, const Buf& in, int e, const Fn& setupOld, const Fn& edit, const Fn& setupNew)
{
    const int N = fadeN (fs), n = in.getNumSamples();
    const Buf A = render (fs, in, setupOld, {}, { block });
    Opt ob { block }; ob.from = e;
    const Buf B = render (fs, in, setupNew, {}, ob);
    const Buf P = render (fs, in, setupOld, { { e, edit } }, { block });
    FadeResult r;
    r.pre = bitEq (P, A, 0, e);
    r.post = bitEq (P, B, e + N, n);
    r.eqFrom = equalFrom (P, B, e, n) - e;
    r.rc = blendErr (P, A, B, e, N);
    r.lin = blendErr (P, A, B, e, N, true);
    gLinMin = std::min (gLinMin, r.lin);
    return r;
}
static void fadeRow (const char* id, const char* name, double fs, const FadeResult& r)
{
    const int N = fadeN (fs);
    row (r.pre && r.post && r.rc <= kBlendTol && r.eqFrom <= N && r.eqFrom > N / 2, id, name,
         f ("%.1f kHz N=%d: pre-edit==old %s; ==fresh-new from e+N %s (bit-equal from e+%d = %.3f ms); raised-cos blend err %.2e of peak (lim %.0e); linear %.2e",
            fs / 1000, N, r.pre ? "bit" : "NO", r.post ? "bit" : "NO", r.eqFrom, 1000.0 * r.eqFrom / fs, r.rc, kBlendTol, r.lin));
}

static void rowsFade()
{
    std::printf ("\n-- 1. Crossfade timing and shape (type change, steady 1 kHz tone, 4 rates) --\n");
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        const int block = 64, e = roundTo (0.3 * fs, block), n = e + (int) (0.15 * fs);
        const auto in = tone2 (n, 1000.0, fs);
        auto r = fadeCase (fs, block, in, e, band (1, 0, 1000, 12), set (bid (1, "type"), 2), band (1, 2, 1000, 12));
        fadeRow ("F1", "Bell+12 -> High Shelf+12 @1k, 1 kHz tone", fs, r);
    }
    {
        const double fs = 48000; const int block = 64, e = roundTo (0.3 * fs, block), n = e + 9600;
        const auto in = noise2 (n, 11);
        fadeRow ("F1n", "Bell+12 -> High Shelf+12 @1k, white noise", fs,
                 fadeCase (fs, block, in, e, band (1, 0, 1000, 12), set (bid (1, "type"), 2), band (1, 2, 1000, 12)));
    }

    std::printf ("\n-- 2. Same for slope, placement, enable, band bypass (48 kHz, noise, L != R) --\n");
    const double fs = 48000; const int block = 64, e = roundTo (0.3 * fs, block), n = e + 9600;
    const auto in = noise2 (n, 22);
    struct C { const char* id; const char* name; Fn o, ed, nw; };
    const Fn lc24 = band (1, 3, 200, 0, 0.7f, 3), lc48 = band (1, 3, 200, 0, 0.7f, 5);
    const Fn bell = band (1, 0, 800, 9, 0.7f);
    const C cases[] = {
        { "F2s",  "Low Cut 200 Hz slope 24 -> 48 dB/oct",            lc24, set (bid (1, "slope"), 5), lc48 },
        { "F2pm", "Bell+9 @800 placement Stereo -> Mid",             bell, set (bid (1, "placement"), 3), band (1, 0, 800, 9, 0.7f, 1, 3) },
        { "F2pl", "Bell+9 @800 placement Stereo -> Left",            bell, set (bid (1, "placement"), 1), band (1, 0, 800, 9, 0.7f, 1, 1) },
        { "F2e1", "Bell+9 @800 enable off -> on",                    all ({ bell, set (bid (1, "enabled"), 0) }), set (bid (1, "enabled"), 1), bell },
        { "F2e0", "Bell+9 @800 enable on -> off",                    bell, set (bid (1, "enabled"), 0), all ({ bell, set (bid (1, "enabled"), 0) }) },
        { "F2b1", "Bell+9 @800 band bypass off -> on",               bell, set (bid (1, "bypass"), 1), all ({ bell, set (bid (1, "bypass"), 1) }) },
        { "F2b0", "Bell+9 @800 band bypass on -> off",               all ({ bell, set (bid (1, "bypass"), 1) }), set (bid (1, "bypass"), 0), bell },
    };
    for (const auto& c : cases) fadeRow (c.id, c.name, fs, fadeCase (fs, block, in, e, c.o, c.ed, c.nw));

    row (gLinMin > 100 * kBlendTol, "F2n", "control: linear weights (n+1)/(N+1) do not match",
         f ("smallest linear-blend error over the %d cases above %.2e of peak (must be > %.0e)", 12, gLinMin, 100 * kBlendTol));
}

//==============================================================================
// 3. No click across a structural step
//==============================================================================
static bool gHardOk = true; static double gHardMin = 1e9;
static void clickCase (const char* id, const char* name, double tone, const Fn& o, const Fn& ed, const Fn& nw, int block = 512, bool gate = true)
{
    const double fs = 48000; const int e = roundTo (0.75 * fs, block), n = e + 24000, N = fadeN (fs);
    const auto in = tone2 (n, tone, fs);
    const Buf A = render (fs, in, o, {}, { block });
    const Buf Bf = render (fs, in, nw, {}, { block });
    Opt ob { block }; ob.from = e;
    const Buf B = render (fs, in, nw, {}, ob);
    const Buf P = render (fs, in, o, { { e, ed } }, { block });
    const int w0 = e - 4800, w1 = e + 4800;
    const double held = std::max (maxD2 (A, w0, w1), maxD2 (Bf, w0, w1));
    const double r = maxD2 (P, w0, w1) / held, hard = maxD2 (splice (A, B, e), w0, w1) / held;
    const double bl = blendErr (P, A, B, e, N);
    // level view: peaks of the held renders and of the probe render over the window
    const double pkA = peakOf (A, w0, w1), pkB = peakOf (Bf, w0, w1), pkP = peakOf (P, e, w1), pkFresh = peakOf (B, e, e + N);
    const juce::String d = f ("max 2nd diff over e +-100 ms = %.3fx the louder held render (lim 2.0x); hard splice %.1fx; blend err %.1e; ==fresh from e+N %s"
                              " | peaks: held old %.3f new %.3f, render after e %.3f, incoming-from-rest %.3f",
                              r, hard, bl, bitEq (P, B, e + N, n) ? "yes" : "NO", pkA, pkB, pkP, pkFresh);
    if (gate)
    {
        gHardMin = std::min (gHardMin, hard); gHardOk = gHardOk && hard >= 10.0;
        row (r <= 2.0 && countNonFinite (P, 0, n) == 0 && bitEq (P, B, e + N, n), id, name, d);
    }
    else info (id, name, d);
}
static void rowsClick()
{
    std::printf ("\n-- 3. No click (48 kHz, block 512, L 0.5 / R 0.3 +0.7 rad) --\n");
    clickCase ("C1", "50 Hz tone: Low Cut 100 Hz 48 dB/oct enable off -> on", 50.0,
               all ({ band (1, 3, 100, 0, 0.7f, 5), set (bid (1, "enabled"), 0) }), set (bid (1, "enabled"), 1), band (1, 3, 100, 0, 0.7f, 5));
    clickCase ("C2", "50 Hz tone: Low Cut 60 Hz slope 48 -> 96 dB/oct", 50.0,
               band (1, 3, 60, 0, 0.7f, 5), set (bid (1, "slope"), 7), band (1, 3, 60, 0, 0.7f, 7));
    clickCase ("C3", "50 Hz tone: Bell +18 @50 -> Low Shelf -18 @50", 50.0,
               band (1, 0, 50, 18), all ({ set (bid (1, "type"), 1), set (bid (1, "gain"), -18) }), band (1, 1, 50, -18));
    clickCase ("C4", "1 kHz tone: Bell +18 @1k -> Notch @1k", 1000.0,
               band (1, 0, 1000, 18), set (bid (1, "type"), 5), band (1, 5, 1000, 18));
    clickCase ("C5", "1 kHz tone: Bell +18 @1k -> High Shelf +18 @2k", 1000.0,
               band (1, 0, 1000, 18), all ({ set (bid (1, "type"), 2), set (bid (1, "freq"), 2000) }), band (1, 2, 2000, 18));
    clickCase ("C6", "1 kHz tone: High Cut 2k 24 dB/oct placement Stereo -> Left", 1000.0,
               band (1, 4, 2000, 0, 0.7f, 3), set (bid (1, "placement"), 1), band (1, 4, 2000, 0, 0.7f, 3, 1));
    row (gHardOk, "C0n", "control: hard splice (old | fresh new at e) reads >= 10x", f ("smallest hard reading %.1fx", gHardMin));

    // Diagnosis of C2: where the 50 Hz tone sits against a cut filter's corner, and the slope pair.
    struct D { float fc; int s0, s1; int block; };
    for (const D& dd : { D { 30, 5, 7, 512 }, D { 40, 5, 7, 512 }, D { 50, 5, 7, 512 }, D { 60, 5, 7, 32 }, D { 80, 5, 7, 512 }, D { 120, 5, 7, 512 },
                         D { 60, 7, 5, 512 }, D { 60, 3, 5, 512 }, D { 60, 1, 3, 512 } })
    {
        const char* sl[8] = { "6", "12", "18", "24", "36", "48", "72", "96" };
        const juce::String nm = f ("50 Hz tone: Low Cut %.0f Hz %s -> %s dB/oct, block %d", dd.fc, sl[dd.s0], sl[dd.s1], dd.block);
        clickCase ("C2d", nm.toRawUTF8(), 50.0, band (1, 3, dd.fc, 0, 0.7f, dd.s0), set (bid (1, "slope"), (float) dd.s1), band (1, 3, dd.fc, 0, 0.7f, dd.s1),
                   dd.block, false);
    }
    clickCase ("C2d", "50 Hz tone: Low Cut 60 Hz 96 dB/oct enable off -> on", 50.0,
               all ({ band (1, 3, 60, 0, 0.7f, 7), set (bid (1, "enabled"), 0) }), set (bid (1, "enabled"), 1), band (1, 3, 60, 0, 0.7f, 7), 512, false);
}

//==============================================================================
// 4. Latch
//==============================================================================
static void rowsLatch()
{
    std::printf ("\n-- 4. Latch (48 kHz, block 32, N = 960; edits at e, e+320, e+480, e+640) --\n");
    const double fs = 48000; const int block = 32, N = fadeN (fs), e = roundTo (0.3 * fs, block), n = e + 2 * N + 9600;
    const auto in = noise2 (n, 44);
    const Fn bell = band (1, 0, 1000, 12), hs = band (1, 2, 1000, 12), ls = band (1, 1, 1000, 12), nt = band (1, 5, 1000, 12), bp = band (1, 6, 1000, 12, 2.0f);
    auto fresh = [&] (const Fn& s, int from) { Opt o { block }; o.from = from; return render (fs, in, s, {}, o); };
    const Buf A = render (fs, in, bell, {}, { block });
    const Buf HSe = fresh (hs, e);

    // (a) one latched edit
    {
        const Buf P = render (fs, in, bell, { { e, set (bid (1, "type"), 2) }, { e + 320, set (bid (1, "type"), 1) } }, { block });
        const Buf LS = fresh (ls, e + N), LSearly = fresh (ls, e + 320);
        const double s1 = blendErr (P, A, HSe, e, N), s2 = blendErr (P, HSe, LS, e + N, N);
        const bool post = bitEq (P, LS, e + 2 * N, n);
        row (s1 <= kBlendTol && s2 <= kBlendTol && post && ! bitEq (P, LSearly, e + 320 + N, n), "L1",
             "Bell->HS at e, ->LS at e+320: LS fade waits for e+N",
             f ("fade 1 err %.1e; fade 2 [e+N, e+2N) vs LS-from-rest at e+N err %.1e; ==LS fresh from e+2N %s; unlatched model (LS from e+320) matches: %s",
                s1, s2, post ? "bit" : "NO", bitEq (P, LSearly, e + 320 + N, n) ? "yes(!)" : "no"));
    }
    // (b) latest wins
    {
        const Buf P = render (fs, in, bell, { { e, set (bid (1, "type"), 2) }, { e + 320, set (bid (1, "type"), 1) },
                                              { e + 480, set (bid (1, "type"), 5) },
                                              { e + 640, all ({ set (bid (1, "type"), 6), set (bid (1, "q"), 2.0f) }) } }, { block });
        const Buf BP = fresh (bp, e + N);
        const double s2 = blendErr (P, HSe, BP, e + N, N);
        const bool post = bitEq (P, BP, e + 2 * N, n);
        const bool tail = bitEq (P, BP, e + 2 * N, n);
        row (blendErr (P, A, HSe, e, N) <= kBlendTol && s2 <= kBlendTol && post && tail, "L2", "three edits in one fade (LS, Notch, BandPass): only the last",
             f ("fade 2 vs HS -> BandPass-from-rest at e+N err %.1e; ==BandPass fresh from e+2N %s (no third fade)", s2, post ? "bit" : "NO"));
        (void) nt;
    }
    // (c) B -> C -> B inside the fade: nothing after
    {
        const Buf P = render (fs, in, bell, { { e, set (bid (1, "type"), 2) }, { e + 320, set (bid (1, "type"), 1) }, { e + 480, set (bid (1, "type"), 2) } }, { block });
        const bool post = bitEq (P, HSe, e + N, n);
        row (post && blendErr (P, A, HSe, e, N) <= kBlendTol, "L3", "Bell->HS, mid-fade ->LS then ->HS again: no second fade",
             f ("==HS fresh-from-e from e+N: %s (first diff after e: %d)", post ? "bit" : "NO", equalFrom (P, HSe, e, n) - e));
    }
    // (d) A -> B -> A: request back to the outgoing settings mid-fade
    {
        const Buf P = render (fs, in, bell, { { e, set (bid (1, "type"), 2) }, { e + 320, set (bid (1, "type"), 0) } }, { block });
        const Buf BellN = fresh (bell, e + N);
        const double s2 = blendErr (P, HSe, BellN, e + N, N);
        const bool post = bitEq (P, BellN, e + 2 * N, n), stayedHS = bitEq (P, HSe, e + N, n), backToA = bitEq (P, A, e + 2 * N, n);
        row (s2 <= kBlendTol && post, "L4", "Bell->HS at e, back to Bell at e+320 (A->B->A)",
             f ("observed: HS fade completes, then a 2nd fade to Bell FROM REST at e+N (err %.1e), ==Bell-from-rest(e+N) from e+2N %s; "
                "stays HS %s; rejoins the original Bell render %s",
                s2, post ? "bit" : "NO", stayedHS ? "yes" : "no", backToA ? "yes" : "no"));
    }
    // (e) continuous edit during a fade: report
    {
        const Buf P = render (fs, in, bell, { { e, set (bid (1, "type"), 2) }, { e + 320, set (bid (1, "gain"), 6) } }, { block });
        const Buf HS6 = render (fs, in, band (1, 2, 1000, 6), {}, { block });
        const int from = e + N + 1920;
        const double d = maxDiff (P, HS6, from, n) / peakOf (HS6, from, n);
        const double dMid = blendErr (P, A, HSe, e, N);
        info ("L5", "Bell->HS+12 at e, gain ->+6 at e+320 (continuous, mid-fade)",
              f ("fade 1 vs unmodified HS+12 blend err %.1e from e (diverges after e+320 => gain went to the incoming slot at once: %s); "
                 "from e+N+40 ms vs static HS+6 %.1e of peak", dMid, dMid > kBlendTol ? "yes" : "no", d));
        row (d < 1e-5, "L5", "continuous edit mid-fade arrives at the static design", f ("rel diff %.2e (lim 1e-5, IIR state decays)", d));
    }
}

//==============================================================================
// 5 + 6. Global bypass
//==============================================================================
static Buf withDenormals (Buf b)
{
    for (int c = 0; c < 2; ++c) for (int i = 997 + c; i < b.getNumSamples(); i += 997) b.setSample (c, i, std::ldexp (1.0f, -130));
    return b;
}
static void rowsBypass()
{
    std::printf ("\n-- 5. Global bypass (48 kHz, block 32; input noise with subnormal samples sprinkled) --\n");
    const double fs = 48000; const int block = 32, N = fadeN (fs), e = roundTo (0.3 * fs, block), u = e + 4800, n = u + 2 * N + 9600;
    const auto in = withDenormals (noise2 (n, 55));
    const Fn s = all ({ band (1, 0, 1000, 12, 0.7f), band (2, 1, 150, -6), band (3, 3, 60, 0, 0.7f, 4, 3) });
    auto fresh = [&] (const Fn& st, int from) { Opt o { block }; o.from = from; return render (fs, in, st, {}, o); };
    const Buf A = render (fs, in, s, {}, { block });
    const Buf Fu = fresh (s, u);

    for (int route = 0; route < 2; ++route)
    {
        Opt o { block };
        std::vector<Ev> ev;
        if (route == 0) ev = { { e, set ("bypass", 1) }, { u, set ("bypass", 0) } };
        else o.viaBypassed = [&] (int st) { return st >= e && st < u; };
        const Buf P = render (fs, in, s, ev, o);
        const double toDry = blendErr (P, A, in, e, N), back = blendErr (P, in, Fu, u, N);
        const bool pre = bitEq (P, A, 0, e), dry = bitEq (P, in, e + N, u), post = bitEq (P, Fu, u + N, n);
        const int dryFrom = equalFrom (P, in, e, u) - e;
        row (pre && dry && toDry <= kBlendTol && back <= kBlendTol && post, route == 0 ? "B1" : "B2",
             route == 0 ? "bypass param: 20 ms to dry, dry bit-exact, back to fresh" : "processBlockBypassed: same path",
             f ("to-dry blend err %.1e; ==input (subnormals intact) from e+%d (N=%d) through u: %s; un-bypass blend vs fresh-from-u err %.1e; ==fresh from u+N %s",
                toDry, dryFrom, N, dry ? "bit" : "NO", back, post ? "bit" : "NO"));
    }
    // latched toggle: bypass at e, un-bypass at e+320 (during the fade to dry)
    {
        const Buf P = render (fs, in, s, { { e, set ("bypass", 1) }, { e + 320, set ("bypass", 0) } }, { block });
        const Buf FeN = fresh (s, e + N);
        const double s1 = blendErr (P, A, in, e, N), s2 = blendErr (P, in, FeN, e + N, N);
        const bool post = bitEq (P, FeN, e + 2 * N, n);
        // reversing model: w climbs to e+320 then retraces
        double rev = 0, pk = 1e-30;
        for (int c = 0; c < 2; ++c)
            for (int k = 320; k < N; ++k)
            {
                const int kk = 320 - (k - 320) - 1;
                const double w = kk >= 0 ? wRC (kk, N) : 0.0, a = A.getSample (c, e + k), d = in.getSample (c, e + k);
                rev = std::max (rev, std::fabs (P.getSample (c, e + k) - (a + w * (d - a)))); pk = std::max (pk, std::fabs (a));
            }
        row (s1 <= kBlendTol && s2 <= kBlendTol && post, "B3", "un-bypass at e+320 during the fade to dry: latched, no reversal",
             f ("fade to dry completes (err %.1e); fade back starts at e+N from rest (err %.1e); ==fresh-from-(e+N) from e+2N %s", s1, s2, post ? "bit" : "NO"));
        row (rev / pk > 100 * kBlendTol, "B3n", "control: a reversing envelope does not match", f ("reversing-model error %.2e of peak", rev / pk));
    }
    // latched toggle the other way: un-bypass at u, bypass again at u+320
    {
        const Buf P = render (fs, in, s, { { e, set ("bypass", 1) }, { u, set ("bypass", 0) }, { u + 320, set ("bypass", 1) } }, { block });
        const double s1 = blendErr (P, in, Fu, u, N), s2 = blendErr (P, Fu, in, u + N, N);
        const bool post = bitEq (P, in, u + 2 * N, n);
        row (s1 <= kBlendTol && s2 <= kBlendTol && post, "B4", "re-bypass at u+320 during the fade back: latched",
             f ("fade back completes (err %.1e); fade to dry from u+N (err %.1e); ==input from u+2N %s", s1, s2, post ? "bit" : "NO"));
    }
    // opens bypassed
    {
        const Buf P = render (fs, in, all ({ s, set ("bypass", 1) }), {}, { block });
        row (bitEq (P, in, 0, n), "B5", "session opens bypassed (bands on): output == input from sample 0",
             f ("first differing sample %d", equalFrom (P, in, 0, n) == 0 ? -1 : equalFrom (P, in, 0, n) - 1));
    }

    std::printf ("\n-- 6. Changed while bypassed: in effect after un-bypass (vs fresh instance started at u) --\n");
    struct V { const char* id; const char* name; int at; Fn ed; Fn nw; };
    const Fn snew = all ({ band (1, 2, 3000, -6, 0.7f), band (2, 1, 150, -6), band (3, 3, 60, 0, 0.7f, 4, 3) });
    const Fn scont = all ({ band (1, 0, 2500, -9, 2.0f), band (2, 1, 150, -6), band (3, 3, 60, 0, 0.7f, 4, 3) });
    const V vs[] = {
        { "B6s", "structural+continuous edit while bypassed", e + N + 640, all ({ set (bid (1, "type"), 2), set (bid (1, "freq"), 3000), set (bid (1, "gain"), -6) }), snew },
        { "B6c", "continuous-only edit while bypassed", e + N + 640, all ({ set (bid (1, "freq"), 2500), set (bid (1, "gain"), -9), set (bid (1, "q"), 2.0f) }), scont },
        { "B6f", "continuous edit during the fade to dry (e+320)", e + 320, all ({ set (bid (1, "freq"), 2500), set (bid (1, "gain"), -9), set (bid (1, "q"), 2.0f) }), scont },
        { "B6r", "continuous edit 5 ms before bypass (ramp in flight)", e - 224, all ({ set (bid (1, "freq"), 2500), set (bid (1, "gain"), -9), set (bid (1, "q"), 2.0f) }), scont },
    };
    for (const auto& v : vs)
    {
        const Buf Fn_ = fresh (v.nw, u);
        bool ok = true; juce::String d;
        for (int route = 0; route < 2; ++route)
        {
            Opt o { block };
            std::vector<Ev> ev { { v.at, v.ed } };
            if (route == 0) { ev.push_back ({ e, set ("bypass", 1) }); ev.push_back ({ u, set ("bypass", 0) }); }
            else o.viaBypassed = [&] (int st) { return st >= e && st < u; };
            const Buf P = render (fs, in, s, ev, o);
            const double b = blendErr (P, in, Fn_, u, N);
            const bool post = bitEq (P, Fn_, u + N, n);
            ok = ok && post && b <= kBlendTol;
            d << (route == 0 ? "param: " : "  pBB: ") << f ("blend err %.1e, ==fresh from u+N %s", b, post ? "bit" : "NO");
        }
        row (ok, v.id, v.name, d);
    }
}

//==============================================================================
// 7. Sleep. Observation: processAudio runs under ScopedNoDenormals, so a lane converted to double loses its
// subnormal samples (FTZ flushes them); a lane no running band touches is not converted and keeps them.
// Subnormal input therefore shows exactly when the band leaves the running list.
//==============================================================================
static void rowsSleep()
{
    std::printf ("\n-- 7. Sleep / wake (48 kHz, block 32) --\n");
    const double fs = 48000; const int block = 32, n = (int) (0.6 * fs);
    const auto dn = denorm2 (n, 77);
    {
        const Buf P = render (fs, dn, band (1, 0, 1000, 1.0f), {}, { block });
        int zeros = 0; for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) zeros += P.getSample (c, i) == 0.0f;
        row (zeros == 2 * n, "S0n", "control: awake +1 dB bell flushes subnormal input to 0", f ("%d of %d output samples exactly 0", zeros, 2 * n));
    }
    {
        const Buf P = render (fs, dn, band (1, 0, 1000, 0.0f), {}, { block });
        const int x = equalFrom (P, dn, 0, n);
        row (x == 0, "S1", "Bell at exactly 0 dB from the start: untouched", f ("output == input from sample %d (%.2f ms)", x, 1000.0 * x / fs));
    }
    {
        const int e = roundTo (0.1 * fs, block);
        const Buf P = render (fs, dn, band (1, 0, 1000, 6.0f), { { e, set (bid (1, "gain"), 0.0f) } }, { block });
        const int x = equalFrom (P, dn, 0, n) - e;
        row (x > 0 && x <= (int) (0.1 * fs), "S2", "Bell +6 -> 0 dB: sleeps (input untouched) within 100 ms",
             f ("untouched from edit + %.2f ms (ramp 20 + sleep 50 + fade 20 = 90 nominal; lim 100)", 1000.0 * x / fs));
    }
    {
        const int e = roundTo (0.1 * fs, block);
        const Buf P = render (fs, dn, band (1, 0, 1000, 12.0f), { { e, set ("gain_scale", 0.0f) } }, { block });
        const int x = equalFrom (P, dn, 0, n) - e;
        row (x > 0 && x <= (int) (0.1 * fs), "S3", "Bell +12, gain_scale 100 -> 0 %: sleeps within 100 ms", f ("untouched from edit + %.2f ms", 1000.0 * x / fs));
    }
    {
        const Buf P = render (fs, dn, band (1, 0, 1000, 0.0001f), {}, { block });
        const Buf T = render (fs, tone2 (n, 1000, fs), band (1, 0, 1000, 0.0001f), {}, { block });
        const auto t = tone2 (n, 1000, fs);
        int same = 0, diffs = 0;
        for (int c = 0; c < 2; ++c) for (int i = n / 2; i < n; ++i) { same += P.getSample (c, i) == dn.getSample (c, i); diffs += T.getSample (c, i) != t.getSample (c, i); }
        row (same == 0 && diffs > 0, "S4", "Bell at 0.0001 dB never sleeps, never identity",
             f ("last 300 ms: subnormal samples passed %d (0 = band still runs); 1 kHz tone samples differing from input %d of %d", same, diffs, n));
    }
    {
        // gain 0 dB with gain moving 0 -> 0.0001: wakes, and a later return to 0 sleeps again
        const int e = roundTo (0.2 * fs, block);
        const Buf P = render (fs, dn, band (1, 0, 1000, 0.0f), { { e, set (bid (1, "gain"), 0.0001f) } }, { block });
        int passedAfter = 0; for (int c = 0; c < 2; ++c) for (int i = e + 960; i < n; ++i) passedAfter += P.getSample (c, i) == dn.getSample (c, i);
        row (bitEq (P, dn, 0, e) && passedAfter == 0, "S5", "asleep 0 dB, gain -> 0.0001 dB: wakes", f ("untouched before edit %s; subnormals passed after edit+20 ms: %d",
             bitEq (P, dn, 0, e) ? "yes" : "NO", passedAfter));
    }
    // wake with a real gain: no click, arrives at the static design
    {
        const int e = roundTo (0.3 * fs, block), m = e + (int) (0.3 * fs);
        const auto t = tone2 (m, 1000, fs);
        const Buf P = render (fs, t, band (1, 0, 1000, 0.0f), { { e, set (bid (1, "gain"), 12.0f) } }, { block });
        const Buf S = render (fs, t, band (1, 0, 1000, 12.0f), {}, { block });
        const double held = std::max (maxD2 (S, e - 4800, e + 4800), maxD2 (t, e - 4800, e + 4800));
        const double r = maxD2 (P, e - 4800, e + 4800) / held;
        const int from = e + (int) (0.06 * fs);
        const double d = maxDiff (P, S, from, m) / peakOf (S, from, m);
        row (r <= 2.0, "S6", "asleep 0 dB bell, gain -> +12 on a 1 kHz tone: no click", f ("max 2nd diff %.3fx the louder held (lim 2.0x)", r));
        row (d <= 1e-5 && bitEq (P, t, 0, e), "S7", "... and arrives at the static +12 dB design",
             f ("from edit+60 ms: max |P - static| %.2e of peak (lim 1e-5); before edit == input %s", d, bitEq (P, t, 0, e) ? "bit" : "NO"));
    }
}

//==============================================================================
// 8. Block-size independence
//==============================================================================
struct ScriptEv { int at; juce::String id; float v; };
static std::vector<ScriptEv> script (int n, int S, juce::int64 seed)
{
    std::vector<ScriptEv> v; juce::Random r (seed);
    bool byp = false; bool en[5] = { false, true, true, true, true }, bb[5] = {};
    for (int at = S; at < n; at += S)
    {
        const int count = 1 + r.nextInt (2);
        for (int k = 0; k < count; ++k)
        {
            const int b = 1 + r.nextInt (4), kind = r.nextInt (10);
            switch (kind)
            {
                case 0: case 1: v.push_back ({ at, bid (b, "type"), (float) r.nextInt (8) }); break;
                case 2: v.push_back ({ at, bid (b, "slope"), (float) r.nextInt (8) }); break;
                case 3: v.push_back ({ at, bid (b, "placement"), (float) r.nextInt (5) }); break;
                case 4: en[b] = ! en[b]; v.push_back ({ at, bid (b, "enabled"), en[b] ? 1.0f : 0.0f }); break;
                case 5: bb[b] = ! bb[b]; v.push_back ({ at, bid (b, "bypass"), bb[b] ? 1.0f : 0.0f }); break;
                case 6: v.push_back ({ at, bid (b, "freq"), (float) (30.0 * std::pow (500.0, r.nextDouble())) }); break;
                case 7: v.push_back ({ at, bid (b, "gain"), r.nextBool() ? 0.0f : (float) (-15.0 + 30.0 * r.nextDouble()) }); break;
                case 8: v.push_back ({ at, bid (b, "q"), (float) (0.3 + 4.0 * r.nextDouble()) }); break;
                case 9: if (r.nextInt (3) == 0) { byp = ! byp; v.push_back ({ at, "bypass", byp ? 1.0f : 0.0f }); } break;
                default: break;
            }
        }
    }
    return v;
}
static Buf scriptRender (double fs, const Buf& in, const std::vector<ScriptEv>& sc, int block, int prepared)
{
    std::vector<Ev> ev;
    for (const auto& s : sc) { const auto id = s.id; const float v = s.v; ev.push_back ({ s.at, [id, v] (Proc& p) { setRaw (p, id, v); } }); }
    const Fn setup = all ({ band (1, 0, 500, 6), band (2, 3, 80, 0, 0.7f, 3), band (3, 2, 4000, -4, 0.7f, 1, 3), band (4, 7, 1000, 3, 1, 1, 1) });
    Opt o { block }; o.prepared = prepared;
    return render (fs, in, setup, ev, o);
}
static void rowsBlocks()
{
    std::printf ("\n-- 8. Block-size independence (events on multiples of 2048 / 4096) --\n");
    for (double fs : { 48000.0, 192000.0 })
    {
        const int n = 2048 * (int) (fs * 0.8 / 2048);
        const auto in = noise2 (n, 88);
        const auto sc = script (n, 2048, 1000 + (int) fs);
        const Buf ref = scriptRender (fs, in, sc, 2048, 2048);
        juce::String d; bool ok = true;
        for (auto bp : { std::pair<int, int> { 1, 1 }, { 32, 32 }, { 64, 64 }, { 512, 512 }, { 2048, 512 }, { 1, 2048 } })
        {
            const Buf b = scriptRender (fs, in, sc, bp.first, bp.second);
            const bool eq = bitEq (b, ref, 0, n);
            ok = ok && eq && countNonFinite (b, 0, n) == 0;
            d << f ("%d/%d:%s ", bp.first, bp.second, eq ? "=" : "DIFF");
        }
        row (ok, fs < 1e5 ? "K1" : "K2", fs < 1e5 ? "48 kHz, 4 bands, scripted structural+bypass events" : "192 kHz (N=3840 > 2048: latches occur)",
             f ("%zu events; host/prepared vs 2048/2048: %s", sc.size(), d.toRawUTF8()));
    }
    {
        const double fs = 48000; const int n = 4096 * 20;
        const auto in = noise2 (n, 89);
        const auto sc = script (n, 4096, 77);
        const Buf ref = scriptRender (fs, in, sc, 512, 512);
        const bool a = bitEq (scriptRender (fs, in, sc, 4096, 512), ref, 0, n), b = bitEq (scriptRender (fs, in, sc, 4096, 100), ref, 0, n),
                   c = bitEq (scriptRender (fs, in, sc, 64, 4096), ref, 0, n);
        row (a && b && c, "K3", "host block > prepared (4096 on 512 / 100), events every 4096", f ("vs 512/512: 4096/512 %d, 4096/100 %d, 64/4096 %d", a, b, c));
    }
}

//==============================================================================
// 9. Re-prepare and state mid-fade
//==============================================================================
static void rowsPrepareState()
{
    std::printf ("\n-- 9. prepareToPlay / state mid-fade (48 kHz, block 32) --\n");
    const double fs = 48000; const int block = 32, N = fadeN (fs), e = roundTo (0.3 * fs, block), n = e + 3 * N + 9600;
    const auto in = noise2 (n, 99);
    const Fn bell = band (1, 0, 1000, 12), hs = band (1, 2, 1000, 12);
    auto fresh = [&] (const Fn& s, int from) { Opt o { block }; o.from = from; return render (fs, in, s, {}, o); };
    {
        const Buf P = render (fs, in, bell, { { e, set (bid (1, "type"), 2) }, { e + 320, [] (Proc& p) { p.setRateAndBufferSizeDetails (48000, 32); p.prepareToPlay (48000, 32); } } }, { block });
        const Buf H = fresh (hs, e + 320);
        const bool eq = bitEq (P, H, e + 320, n);
        row (eq && countNonFinite (P, 0, n) == 0, "R1", "prepareToPlay at e+320 mid-fade: settles on the new type, no fade",
             f ("==fresh HS instance from e+320 %s (first diff at e+320+%d)", eq ? "bit" : "NO", equalFrom (P, H, e + 320, n) - e - 320));
    }
    {
        // re-prepare at 96 kHz mid-fade, then a type edit: equal to a fresh 96 kHz instance given the same edit
        const int r0 = e + 320, e2 = r0 + 4800;
        const Buf P = render (fs, in, bell, { { e, set (bid (1, "type"), 2) }, { r0, [] (Proc& p) { p.setRateAndBufferSizeDetails (96000, 32); p.prepareToPlay (96000, 32); } },
                                              { e2, set (bid (1, "type"), 1) } }, { block });
        Opt o { block }; o.from = r0;
        const Buf H = render (96000, in, hs, { { e2, set (bid (1, "type"), 1) } }, o);
        const bool eq = bitEq (P, H, r0, n);
        row (eq, "R2", "re-prepared at 96 kHz mid-fade, then HS->LS: equals a fresh 96 kHz instance", f ("bit-equal from re-prepare %s", eq ? "yes" : "NO"));
    }
    {
        juce::MemoryBlock blob;
        render (fs, in, bell, { { e, set (bid (1, "type"), 2) }, { e + 320, [&blob] (Proc& p) { p.getStateInformation (blob); } } }, { block });
        const Buf want = render (fs, in, hs, {}, { block });
        const Buf got = render (fs, in, [&blob] (Proc& p) { p.setStateInformation (blob.getData(), (int) blob.getSize()); }, {}, { block });
        row (bitEq (got, want, 0, n), "R3", "state saved mid-fade holds the requested (new) settings", f ("restored instance == static HS instance: %s", bitEq (got, want, 0, n) ? "bit" : "NO"));
    }
    {
        juce::MemoryBlock blob;
        { auto q = make (fs, 2, block); band (1, 1, 300, -6) (*q); q->getStateInformation (blob); }
        const Buf P = render (fs, in, bell, { { e, set (bid (1, "type"), 2) }, { e + 320, [&blob] (Proc& p) { p.setStateInformation (blob.getData(), (int) blob.getSize()); } } }, { block });
        const Buf HSe = fresh (hs, e), LS = fresh (band (1, 1, 300, -6), e + N);
        const double s1 = blendErr (P, render (fs, in, bell, {}, { block }), HSe, e, N), s2 = blendErr (P, HSe, LS, e + N, N);
        const bool post = bitEq (P, LS, e + 2 * N, n);
        row (countNonFinite (P, 0, n) == 0 && s1 <= kBlendTol && s2 <= kBlendTol && post, "R4", "setStateInformation (LS -6 @300) mid-fade: latched like an edit",
             f ("fade 1 err %.1e; fade 2 at e+N to LS-from-rest err %.1e; ==LS fresh(e+N) from e+2N %s", s1, s2, post ? "bit" : "NO"));
    }
}

//==============================================================================
// 10. Non-finite input during a fade
//==============================================================================
static void rowsNaN()
{
    std::printf ("\n-- 10. NaN / inf during a fade (48 kHz, host block 64, prepared 512) --\n");
    const double fs = 48000; const int block = 64, N = fadeN (fs), e = roundTo (0.3 * fs, block), n = e + N + (int) (0.4 * fs);
    for (int k = 0; k < 2; ++k)
    {
        auto in = noise2 (n, 111);
        const int at = e + 100;
        in.setSample (k, at, k == 0 ? std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity());
        Opt o { block }; o.prepared = 512;
        const Buf P = render (fs, in, band (1, 0, 1000, 12), { { e, set (bid (1, "type"), 2) } }, o);
        auto clean = noise2 (n, 111);
        Opt of { block }; of.prepared = 512; of.from = e;
        const Buf H = render (fs, clean, band (1, 2, 1000, 12), {}, of);
        int last = -1; const int bad = countNonFinite (P, 0, n, &last);
        const int blockEnd = (at / block + 1) * block;
        const int from = e + N + (int) (0.2 * fs);
        const double d = maxDiff (P, H, from, n) / peakOf (H, from, n);
        row (last < blockEnd && d < 1e-5, k == 0 ? "N1" : "N2", k == 0 ? "NaN in L at e+100 during a type fade" : "+inf in R at e+100 during a type fade",
             f ("%d non-finite outputs, last at e+%d (its host block ends at e+%d); 200 ms after the fade vs clean HS run %.1e of peak (lim 1e-5)",
                bad, last - e, blockEnd - e, d));
    }
}

//==============================================================================
// 11. 24 bands changing type at once
//==============================================================================
static void rows24()
{
    std::printf ("\n-- 11. 24 bands change type at once (48 kHz, block 512) --\n");
    const double fs = 48000; const int block = 512, N = fadeN (fs);
    std::vector<Fn> o, ed, nw;
    float frs[24], gs[24]; int ts[24];
    for (int k = 0; k < 24; ++k)
    {
        const float fr = (float) (40.0 * std::pow (300.0, k / 23.0)), g = (k % 2 == 0) ? 6.0f : -6.0f;
        const int t = k % 4 == 0 ? 1 : k % 4 == 1 ? 2 : k % 4 == 2 ? 7 : 5;
        frs[k] = fr; gs[k] = g; ts[k] = t;
        o.push_back (band (k + 1, 0, fr, g, 2.0f));
        ed.push_back (set (bid (k + 1, "type"), (float) t));
        nw.push_back (band (k + 1, t, fr, g, 2.0f));
    }
    // Oracle: the bands run in series and each fades on its own, so the right reference is a series chain of 24
    // single-band instances, each given its own type edit at e (float between stages: tolerance 1e-5 of peak,
    // i.e. 24 stages x a few ulps with +-6 dB gains). A single fresh instance is NOT the reference: in the plugin the
    // incoming slot of band k starts from rest fed band k-1's crossfaded output.
    auto chain = [&] (const Buf& in, int e, int block)
    {
        Buf x; x.makeCopyOf (in);
        for (int k = 0; k < 24; ++k) x = render (fs, x, band (1, 0, frs[k], gs[k], 2.0f), { { e, set (bid (1, "type"), (float) ts[k]) } }, { block });
        return x;
    };
    const double kChainTol = 1e-5;
    double hardMin = 1e9;
    for (double tone : { 50.0, 1000.0 })
    {
        const int e = roundTo (0.75 * fs, block), n = e + 24000;
        const auto in = tone2 (n, tone, fs);
        const Buf A = render (fs, in, all (o), {}, { block });
        const Buf Bf = render (fs, in, all (nw), {}, { block });
        Opt ob { block }; ob.from = e;
        const Buf B = render (fs, in, all (nw), {}, ob);
        const Buf P = render (fs, in, all (o), { { e, all (ed) } }, { block });
        const int w0 = e - 4800, w1 = e + 4800;
        const double held = std::max (maxD2 (A, w0, w1), maxD2 (Bf, w0, w1));
        const double r = maxD2 (P, w0, w1) / held, hard = maxD2 (splice (A, B, e), w0, w1) / held;
        hardMin = std::min (hardMin, hard);
        const Buf C = chain (in, e, block);
        const double pk = peakOf (P, 0, n), cAll = maxDiff (P, C, 0, n) / pk, cFade = maxDiff (P, C, e, e + N) / pk;
        // control: the same chain with every stage's edit one block late must NOT match
        const Buf Cl = chain (in, e + block, block);
        const double cLate = maxDiff (P, Cl, e, n) / pk;
        const int eqFrom = equalFrom (P, B, e, n) - e;
        row (r <= 2.0 && countNonFinite (P, 0, n) == 0 && cAll <= kChainTol && cLate > 100 * kChainTol, tone < 100 ? "M1" : "M2",
             tone < 100 ? "24 bands Bell -> LS/HS/Tilt/Notch, 50 Hz tone" : "24 bands Bell -> LS/HS/Tilt/Notch, 1 kHz tone",
             f ("max 2nd diff %.3fx held (lim 2.0x); vs serial chain of 24 single-band fades: %.1e of peak overall, %.1e in the fade "
                "(lim %.0e; chain edited one block late %.1e); == one fresh instance only from e+%.1f ms (from-rest tails of the series chain decay)",
                r, cAll, cFade, kChainTol, cLate, 1000.0 * eqFrom / fs));
    }
    row (hardMin >= 10.0, "M0n", "control: hard splice of the 24-band change", f ("smallest %.1fx (>= 10x)", hardMin));
}

//==============================================================================
// 12. Mid / Side structural fade
//==============================================================================
static void rowsMS()
{
    std::printf ("\n-- 12. Mid / Side structural fade (48 kHz, block 64) --\n");
    const double fs = 48000; const int block = 64, N = fadeN (fs), e = roundTo (0.3 * fs, block), n = e + N + 9600;
    for (int side = 0; side < 2; ++side)
    {
        auto in = noise2 (n, 121);
        in.copyFrom (1, 0, in, 0, 0, n);
        if (side) in.applyGain (1, 0, n, -1.0f);
        const Fn ed = all ({ set (bid (1, "type"), 2), set (bid (1, "enabled"), 1) });
        const Buf M = render (fs, in, band (1, 0, 800, 9, 0.7f, 1, side ? 4 : 3), { { e, ed } }, { block });
        auto ref = noise2 (n, 121); ref.copyFrom (1, 0, ref, 0, 0, n);
        const Buf S = render (fs, ref, band (1, 0, 800, 9, 0.7f, 1, 0), { { e, ed } }, { block });
        double d = 0, anti = 0;
        for (int i = 0; i < n; ++i)
        {
            d = std::max (d, std::fabs ((double) M.getSample (0, i) - S.getSample (0, i)));
            anti = std::max (anti, std::fabs ((double) M.getSample (1, i) - (side ? -1.0 : 1.0) * M.getSample (0, i)));
        }
        double dF = 0; for (int i = e; i < e + N; ++i) dF = std::max (dF, std::fabs ((double) M.getSample (0, i) - S.getSample (0, i)));
        const double pk = peakOf (S, 0, n), dFade = dF / pk;
        row (d / pk <= kBlendTol && anti / pk <= kBlendTol, side ? "MS2" : "MS1",
             side ? "Side band Bell->HS on L=-R: L equals Stereo band's, R=-L" : "Mid band Bell->HS on L=R: both equal the Stereo band's",
             f ("worst diff %.1e of peak (in the fade %.1e); channel symmetry err %.1e (lim %.0e)", d / pk, dFade, anti / pk, kBlendTol));
    }
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    std::printf ("O-ParametricEQ verify 2.4 probe (independent of the harnesses). N = round (0.020 fs); w(n) = 0.5 - 0.5 cos (pi (n+1)/(N+1))\n");
    rowsFade();
    rowsClick();
    rowsLatch();
    rowsBypass();
    rowsSleep();
    rowsBlocks();
    rowsPrepareState();
    rowsNaN();
    rows24();
    rowsMS();
    std::printf ("\n%d PASS, %d FAIL\n", nPass, nFail);
    return nFail == 0 ? 0 : 1;
}
