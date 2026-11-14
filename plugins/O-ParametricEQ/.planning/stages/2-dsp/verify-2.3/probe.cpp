// Independent probe for O-ParametricEQ Phase 2.3 verify (written in the verify session).
// Drives the real processor through its public API and scores it against facts that do
// not come from the harnesses: the 20 ms ramp length, the shape of each trajectory
// (dB for gain, log for frequency and Q), the restart rule, the ramping path under every
// placement, the state-clearing paths during a ramp, oversize host blocks, re-prepare.
// It uses none of Source/dsp and none of the render harness.
#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

using Proc = OuariconParametricEQAudioProcessor;

static int nPass = 0, nFail = 0;
static const double kPi = juce::MathConstants<double>::pi;

static void row (bool ok, const char* name, const juce::String& detail)
{
    std::printf ("  [%s] %-62s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
    (ok ? nPass : nFail)++;
}

[[maybe_unused]] static void info (const char* name, const juce::String& detail)
{
    std::printf ("  [INFO] %-62s %s\n", name, detail.toRawUTF8());
}

static void setRaw (Proc& p, const juce::String& id, float raw)
{
    auto* prm = p.parameters.getParameter (id);
    if (prm == nullptr) { std::printf ("NO PARAM %s\n", id.toRawUTF8()); std::exit (2); }
    prm->setValueNotifyingHost (prm->convertTo0to1 (raw));
}

static juce::String bid (int band, const char* suffix)   // band 1 ... 24
{
    return "b" + juce::String (band).paddedLeft ('0', 2) + "_" + suffix;
}

// type: 0 Bell 1 LowShelf 2 HighShelf 3 LowCut 4 HighCut 5 Notch 6 BandPass 7 Tilt
// slope index: 0 = 6, 1 = 12, 2 = 18, 3 = 24, 4 = 36, 5 = 48, 6 = 72, 7 = 96 dB/oct
// placement: 0 Stereo 1 Left 2 Right 3 Mid 4 Side
static void setBand (Proc& p, int band, int type, float freq, float gain, float q, int slope, int placement)
{
    setRaw (p, bid (band, "type"), (float) type);
    setRaw (p, bid (band, "freq"), freq);
    setRaw (p, bid (band, "gain"), gain);
    setRaw (p, bid (band, "q"), q);
    setRaw (p, bid (band, "slope"), (float) slope);
    setRaw (p, bid (band, "placement"), (float) placement);
    setRaw (p, bid (band, "bypass"), 0.0f);
    setRaw (p, bid (band, "enabled"), 1.0f);
}

static std::unique_ptr<Proc> make (double fs, int channels, int block = 512)
{
    auto p = std::make_unique<Proc>();
    juce::AudioProcessor::BusesLayout layout;
    const auto set = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    layout.inputBuses.add (set);
    layout.outputBuses.add (set);
    if (! p->setBusesLayout (layout)) { std::printf ("layout refused\n"); std::exit (2); }
    p->setRateAndBufferSizeDetails (fs, block);
    p->prepareToPlay (fs, block);
    return p;
}

using Hook = std::function<void (int blockIndex, int startSample)>;

// Renders buf in place in host blocks of `block`; the hook runs before each block.
static void run (Proc& p, juce::AudioBuffer<float>& buf, int block, const Hook& hook = {}, int from = 0, int to = -1,
                 bool bypassedCall = false)
{
    juce::MidiBuffer midi;
    const int n = to < 0 ? buf.getNumSamples() : to;
    int index = 0;
    for (int start = from; start < n; start += block, ++index)
    {
        if (hook) hook (index, start);
        const int len = juce::jmin (block, n - start);
        juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), buf.getNumChannels(), start, len);
        if (bypassedCall) p.processBlockBypassed (view, midi); else p.processBlock (view, midi);
    }
}

static juce::AudioBuffer<float> sine (int channels, int n, double f, double fs, double amp = 0.25, double lr = 1.0)
{
    juce::AudioBuffer<float> b (channels, n);
    const double w = 2.0 * kPi * f / fs;
    for (int i = 0; i < n; ++i)
    {
        const float v = (float) (amp * std::sin (w * i));
        b.setSample (0, i, v);
        if (channels == 2) b.setSample (1, i, (float) (lr * v));
    }
    return b;
}

static juce::AudioBuffer<float> noise (int channels, int n, juce::int64 seed, bool sameLR = false)
{
    juce::Random r (seed);
    juce::AudioBuffer<float> b (channels, n);
    for (int c = 0; c < channels; ++c)
        for (int i = 0; i < n; ++i)
            b.setSample (c, i, 0.5f * (r.nextFloat() * 2.0f - 1.0f));
    if (sameLR && channels == 2) b.copyFrom (1, 0, b, 0, 0, n);
    return b;
}

static bool sameBits (const float* a, const float* b, int n)
{
    return std::memcmp (a, b, sizeof (float) * (size_t) n) == 0;
}

static int firstDiff (const float* a, const float* b, int n)
{
    for (int i = 0; i < n; ++i)
        if (std::memcmp (a + i, b + i, sizeof (float)) != 0) return i;
    return -1;
}

// Least-squares amplitude of a sinusoid of known frequency over [from, to).
static double fitAmp (const float* x, int from, int to, double f, double fs)
{
    double scc = 0, sss = 0, scs = 0, sxc = 0, sxs = 0;
    const double w = 2.0 * kPi * f / fs;
    for (int i = from; i < to; ++i)
    {
        const double c = std::cos (w * i), s = std::sin (w * i);
        scc += c * c; sss += s * s; scs += c * s;
        sxc += x[i] * c; sxs += x[i] * s;
    }
    const double det = scc * sss - scs * scs;
    const double a = (sxc * sss - sxs * scs) / det;
    const double b = (sxs * scc - sxc * scs) / det;
    return std::sqrt (a * a + b * b);
}

// Level in dB re `amp` of the tone at f, over one window of `periods` periods centred on sample c.
static double envDb (const float* x, int c, double f, double fs, double amp = 0.25, double periods = 1.0)
{
    const int half = (int) std::lround (0.5 * periods * fs / f);
    return 20.0 * std::log10 (std::max (fitAmp (x, c - half, c + half, f, fs), 1e-30) / amp);
}

static double ms (int samples, double fs) { return 1000.0 * samples / fs; }

// Largest |second difference| over [from, to).
static double maxD2 (const float* x, int from, int to)
{
    double m = 0.0;
    for (int i = std::max (from, 2); i < to; ++i)
        m = std::max (m, std::fabs ((double) x[i] - 2.0 * (double) x[i - 1] + (double) x[i - 2]));
    return m;
}

// Largest |first difference| over [from, to). A step in the output (a hard parameter change, a control-rate
// staircase) shows here; the slope change where a linear ramp starts or ends does not.
static double maxD1 (const float* x, int from, int to)
{
    double m = 0.0;
    for (int i = std::max (from, 1); i < to; ++i)
        m = std::max (m, std::fabs ((double) x[i] - (double) x[i - 1]));
    return m;
}

static double sineD1 (double a, double f, double fs) { return a * 2.0 * std::sin (kPi * f / fs); }

static double maxAbs (const float* x, int from, int to)
{
    double m = 0.0;
    for (int i = from; i < to; ++i) m = std::max (m, std::fabs ((double) x[i]));
    return m;
}

// The second difference of a steady sine of amplitude a at f.
static double sineD2 (double a, double f, double fs)
{
    const double s = 2.0 * std::sin (kPi * f / fs);
    return a * s * s;
}

static double steadyDb (Proc& p, double fs, double f, int channel = 0)
{
    const int n = (int) (fs * 2.0);
    auto buf = sine (2, n, f, fs);
    run (p, buf, 512);
    return 20.0 * std::log10 (fitAmp (buf.getReadPointer (channel), n - (int) (fs * 0.5), n, f, fs) / 0.25);
}

//==============================================================================
// 1. Ramp length and shape of a gain move, at three control grids
//==============================================================================
static void rowsGainRamp()
{
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        const int block = 512;
        const int edit = block * (int) std::lround (0.5 * fs / block);      // a block start: on a tick at every grid
        const int n = edit + (int) fs / 2;
        auto p = make (fs, 2, block);
        setBand (*p, 1, 0, 1000.0f, 0.0f, 1.0f, 1, 0);
        auto buf = sine (2, n, 1000.0, fs);
        run (*p, buf, block, [&] (int, int start) { if (start == edit) setRaw (*p, bid (1, "gain"), 12.0f); });
        const float* y = buf.getReadPointer (0);

        auto at = [&] (double msAfter) { return envDb (y, edit + (int) std::lround (msAfter * fs / 1000.0), 1000.0, fs); };
        // crossing times of 10 % and 90 % of the move, in dB
        double t10 = -1, t90 = -1;
        for (int i = edit - (int) (0.002 * fs); i < edit + (int) (0.040 * fs); ++i)
        {
            const double g = envDb (y, i, 1000.0, fs);
            if (t10 < 0 && g >= 1.2) t10 = ms (i - edit, fs);
            if (t90 < 0 && g >= 10.8) t90 = ms (i - edit, fs);
        }
        const double before = at (-3.0), mid = at (10.0), q1 = at (5.0), q3 = at (15.0), after = at (23.0);
        const bool ok = std::fabs (before) < 0.02 && std::fabs (after - 12.0) < 0.05
                     && std::fabs ((t90 - t10) - 16.0) < 1.5 && std::fabs (mid - 6.0) < 0.8
                     && std::fabs (q1 - 3.0) < 0.8 && std::fabs (q3 - 9.0) < 0.8;
        row (ok, "gain 0 -> +12 dB, bell on a 1 kHz tone: 20 ms, linear in dB",
             juce::String::formatted ("%.0f Hz: 10 %% - 90 %% in %.2f ms (16 expected); level at +5 / +10 / +15 ms %.2f / %.2f / %.2f dB "
                                      "(3 / 6 / 9; a ramp linear in amplitude reads 4.9 / 7.9 / 10.2); before %.3f, at +23 ms %.3f dB",
                                      fs, t90 - t10, q1, mid, q3, before, after));
    }
}

//==============================================================================
// 2. gain_scale takes the same ramp
//==============================================================================
static void rowGainScaleRamp()
{
    const double fs = 48000.0; const int block = 512, edit = 24064, n = edit + 24000;
    auto p = make (fs, 2, block);
    setBand (*p, 1, 0, 1000.0f, 12.0f, 1.0f, 1, 0);
    auto buf = sine (2, n, 1000.0, fs);
    run (*p, buf, block, [&] (int, int start) { if (start == edit) setRaw (*p, "gain_scale", 0.0f); });
    const float* y = buf.getReadPointer (0);
    const double before = envDb (y, edit - 144, 1000.0, fs), mid = envDb (y, edit + 480, 1000.0, fs), after = envDb (y, edit + 1104, 1000.0, fs);
    row (std::fabs (before - 12.0) < 0.02 && std::fabs (mid - 6.0) < 0.8 && std::fabs (after) < 0.05,
         "gain_scale 100 -> 0 on a +12 dB bell: the same 20 ms ramp",
         juce::String::formatted ("before %.3f dB, at +10 ms %.2f dB (6 expected), at +23 ms %.4f dB", before, mid, after));
}

//==============================================================================
// 3. Frequency moves in log f: a notch swept across a tone at the geometric mean
//==============================================================================
static void rowsFreqLog()
{
    const double fs = 48000.0; const int block = 512, edit = 24064, n = edit + 24000;
    for (int dir = 0; dir < 2; ++dir)
    {
        const float fa = dir == 0 ? 100.0f : 10000.0f, fb = dir == 0 ? 10000.0f : 100.0f;
        auto p = make (fs, 2, block);
        setBand (*p, 1, 5, fa, 0.0f, 1.0f, 1, 0);
        auto buf = sine (2, n, 1000.0, fs);
        run (*p, buf, block, [&] (int, int start) { if (start == edit) setRaw (*p, bid (1, "freq"), fb); });
        const float* y = buf.getReadPointer (0);
        double best = 1e9; int bestAt = 0;
        for (int i = edit - 96; i < edit + 1440; ++i)
        {
            const double g = envDb (y, i, 1000.0, fs);
            if (g < best) { best = g; bestAt = i; }
        }
        const double tMin = ms (bestAt - edit, fs);
        row (std::fabs (tMin - 10.0) < 2.0 && best < -6.0,
             dir == 0 ? "notch 100 Hz -> 10 kHz across a 1 kHz tone: deepest at mid-ramp" : "notch 10 kHz -> 100 Hz across a 1 kHz tone: deepest at mid-ramp",
             juce::String::formatted ("deepest (%.1f dB) at +%.2f ms; log f passes 1 kHz at 10 ms, linear f at %.1f ms", best, tMin,
                                      dir == 0 ? 1.8 : 18.2));
    }
}

//==============================================================================
// 4. Q moves in log Q
//==============================================================================
static void rowQLog()
{
    const double fs = 48000.0; const int block = 512, edit = 24064, n = edit + 24000;
    auto stat = [&] (float q) { auto p = make (fs, 2, block); setBand (*p, 1, 0, 1000.0f, 12.0f, q, 1, 0); return steadyDb (*p, fs, 1500.0); };
    const double sLo = stat (0.1f), sGeo = stat (1.0f), sLin = stat (5.05f), sHi = stat (10.0f);
    auto p = make (fs, 2, block);
    setBand (*p, 1, 0, 1000.0f, 12.0f, 0.1f, 1, 0);
    auto buf = sine (2, n, 1500.0, fs);
    run (*p, buf, block, [&] (int, int start) { if (start == edit) setRaw (*p, bid (1, "q"), 10.0f); });
    const double mid = envDb (buf.getReadPointer (0), edit + 480, 1500.0, fs, 0.25, 2.0);
    row (std::fabs (mid - sGeo) < 1.0 && std::fabs (mid - sGeo) < 0.5 * std::fabs (mid - sLin),
         "Q 0.1 -> 10 on a +12 dB bell, tone at 1.5 kHz: the mid-ramp level is that of Q 1",
         juce::String::formatted ("at +10 ms %.2f dB; static Q 0.1 / 1 / 5.05 / 10: %.2f / %.2f / %.2f / %.2f dB (log Q passes 1, linear Q 5.05)",
                                  mid, sLo, sGeo, sLin, sHi));
}

//==============================================================================
// 5. Restart rule: a new target mid-ramp restarts from the current value over the full length
//==============================================================================
static void rowRestart()
{
    const double fs = 48000.0; const int block = 480, edit = 24000, n = edit + 24000;      // 480 = 10 ms, 30 ticks
    auto p = make (fs, 2, block);
    setBand (*p, 1, 0, 1000.0f, 0.0f, 1.0f, 1, 0);
    auto buf = sine (2, n, 1000.0, fs);
    run (*p, buf, block, [&] (int, int start)
    {
        if (start == edit) setRaw (*p, bid (1, "gain"), 12.0f);
        if (start == edit + 480) setRaw (*p, bid (1, "gain"), 0.0f);
    });
    const float* y = buf.getReadPointer (0);
    auto at = [&] (double m) { return envDb (y, edit + (int) std::lround (m * 48.0), 1000.0, fs); };
    const double a10 = at (10.0), a20 = at (20.0), a28 = at (28.0), a33 = at (33.0);
    row (std::fabs (a10 - 6.0) < 0.8 && std::fabs (a20 - 3.0) < 0.8 && std::fabs (a28 - 0.6) < 0.5 && std::fabs (a33) < 0.05,
         "gain 0 -> +12 dB, back to 0 after 10 ms: 20 ms more from where it stood",
         juce::String::formatted ("level at +10 / +20 / +28 / +33 ms: %.2f / %.2f / %.2f / %.3f dB (6 / 3 / 0.6 / 0 expected)", a10, a20, a28, a33));
}

//==============================================================================
// 6. Click metric of my own: largest second difference against a steady sine
//==============================================================================
static void rowsClick()
{
    const double fs = 48000.0; const int block = 512, edit = 48128, n = edit + 48000;
    // (a) bell at the tone, gain -30 <-> +30 dB, 50 Hz
    for (int dir = 0; dir < 2; ++dir)
    {
        auto p = make (fs, 2, block);
        setBand (*p, 1, 0, 50.0f, dir == 0 ? -30.0f : 30.0f, 1.0f, 1, 0);
        auto buf = sine (2, n, 50.0, fs);
        run (*p, buf, block, [&] (int, int start) { if (start == edit) setRaw (*p, bid (1, "gain"), dir == 0 ? 30.0f : -30.0f); });
        const float* y = buf.getReadPointer (0);
        const double loud = 0.25 * std::pow (10.0, 30.0 / 20.0);
        const double r = maxD1 (y, edit - 240, edit + 4800) / sineD1 (loud, 50.0, fs);
        const double r2 = maxD2 (y, edit - 240, edit + 4800) / sineD2 (loud, 50.0, fs);
        row (r < 4.0, dir == 0 ? "bell on a 50 Hz tone, gain -30 -> +30 dB: no step in the output" : "bell on a 50 Hz tone, gain +30 -> -30 dB: no step in the output",
             juce::String::formatted ("largest first difference over the move = %.2fx that of the steady tone at +30 dB (limit of this probe 4x; "
                                      "a 1 dB staircase reads 18x); second difference %.1fx (the ramp's two corners)", r, r2));
    }
    // (b) control: a structural change at the same sample
    {
        auto p = make (fs, 2, block);
        setBand (*p, 1, 0, 50.0f, 30.0f, 1.0f, 1, 0);
        auto buf = sine (2, n, 50.0, fs);
        run (*p, buf, block, [&] (int, int start) { if (start == edit + 512) setRaw (*p, bid (1, "type"), 4.0f); });
        const double loud = 0.25 * std::pow (10.0, 30.0 / 20.0);
        const double r = maxD1 (buf.getReadPointer (0), edit - 240, edit + 4800) / sineD1 (loud, 50.0, fs);
        row (r > 50.0, "control: a type change instead (2.4 work) reads as a click", juce::String::formatted ("%.0fx", r));
    }
}

//==============================================================================
// 7. One-pole sections on the ramping path (6 and 18 dB/oct)
//==============================================================================
static void rowsOnePole()
{
    const double fs = 48000.0; const int block = 512, edit = 48128, n = edit + 48000;
    struct Case { const char* name; int type; int slope; float f0, g0, f1, g1; double tone; };
    const Case cases[] = {
        { "Low Shelf 6 dB/oct at 200 Hz, gain -18 -> +18 dB, 100 Hz tone", 1, 0, 200.0f, -18.0f, 200.0f, 18.0f, 100.0 },
        { "Low Shelf 6 dB/oct at 200 Hz, gain +18 -> -18 dB, 100 Hz tone", 1, 0, 200.0f, 18.0f, 200.0f, -18.0f, 100.0 },
        { "High Cut 6 dB/oct, 200 Hz -> 5 kHz, 1 kHz tone",                 4, 0, 200.0f, 0.0f, 5000.0f, 0.0f, 1000.0 },
        { "High Cut 18 dB/oct, 300 Hz -> 3 kHz, 1 kHz tone",                4, 2, 300.0f, 0.0f, 3000.0f, 0.0f, 1000.0 },
        { "Low Cut 18 dB/oct, 3 kHz -> 300 Hz, 1 kHz tone",                 3, 2, 3000.0f, 0.0f, 300.0f, 0.0f, 1000.0 },
        { "High Shelf 18 dB/oct at 500 Hz, gain 0 -> +18 dB, 1 kHz tone",   2, 2, 500.0f, 0.0f, 500.0f, 18.0f, 1000.0 },
    };
    for (const auto& c : cases)
    {
        auto p = make (fs, 2, block);
        setBand (*p, 1, c.type, c.f0, c.g0, 1.0f, c.slope, 0);
        auto buf = sine (2, n, c.tone, fs);
        run (*p, buf, block, [&] (int, int start)
        {
            if (start == edit) { setRaw (*p, bid (1, "freq"), c.f1); setRaw (*p, bid (1, "gain"), c.g1); }
        });
        const float* y = buf.getReadPointer (0);
        const double loud = std::max (fitAmp (y, edit - 9600, edit - 480, c.tone, fs), fitAmp (y, n - 9600, n, c.tone, fs));
        const double r = maxD1 (y, edit - 240, edit + 4800) / sineD1 (loud, c.tone, fs);
        const double r2 = maxD2 (y, edit - 240, edit + 4800) / sineD2 (loud, c.tone, fs);
        // the level must actually move, or the row is vacuous
        const double moved = std::fabs (envDb (y, n - 4800, c.tone, fs) - envDb (y, edit - 4800, c.tone, fs));
        row (r < 2.0 && moved > 6.0, c.name,
             juce::String::formatted ("largest first difference over the move = %.2fx the louder steady tone's (limit of this probe 2x); "
                                      "second difference %.1fx; the level moves %.1f dB", r, r2, moved));
    }
}

//==============================================================================
// 8. Several bands moving at once, automated at every host block (the summary's item for verify)
//==============================================================================
static double multiBandRatio (int block, bool withTypeFlips, double tone, double* peakOut = nullptr, double* d2Out = nullptr)
{
    const double fs = 48000.0; const int n = (int) (fs * 5.0);
    auto p = make (fs, 2, block);
    struct B { int type, slope; double f, g, q, rate; };
    const B bands[8] = {
        { 0, 1, 50.0, 0.0, 1.0, 1.3 }, { 1, 1, 80.0, 0.0, 1.0, 2.1 }, { 2, 1, 200.0, 0.0, 1.0, 0.7 }, { 7, 1, 400.0, 0.0, 1.0, 1.7 },
        { 0, 1, 100.0, 0.0, 2.0, 2.9 }, { 0, 1, 35.0, 0.0, 4.0, 1.1 }, { 1, 3, 60.0, 0.0, 1.0, 0.9 }, { 2, 0, 1000.0, 0.0, 1.0, 2.3 },
    };
    for (int b = 0; b < 8; ++b)
        setBand (*p, b + 1, bands[b].type, (float) bands[b].f, 0.0f, (float) bands[b].q, bands[b].slope, b % 5);
    auto buf = sine (2, n, tone, fs, 0.25, 0.6);
    run (*p, buf, block, [&] (int index, int start)
    {
        const double t = start / fs;
        for (int b = 0; b < 8; ++b)
        {
            const double ph = 2.0 * kPi * bands[b].rate * t + b;
            setRaw (*p, bid (b + 1, "freq"), (float) (bands[b].f * std::pow (2.0, std::sin (ph))));
            setRaw (*p, bid (b + 1, "gain"), (float) (12.0 * std::sin (1.37 * ph + 1.0)));
            setRaw (*p, bid (b + 1, "q"), (float) (bands[b].q * std::pow (2.0, std::sin (0.61 * ph + 2.0))));
        }
        setRaw (*p, "gain_scale", (float) (100.0 + 60.0 * std::sin (2.0 * kPi * 0.8 * t)));
        if (withTypeFlips && index > 0 && index % 97 == 0)
            setRaw (*p, bid (1 + (index / 97) % 8, "type"), (float) ((index / 97) % 3));
    });
    double worst = 0.0, peak = 0.0, worst2 = 0.0;
    const int from = (int) (0.3 * fs);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* y = buf.getReadPointer (ch);
        const double a = maxAbs (y, from, n);
        peak = std::max (peak, a);
        worst = std::max (worst, maxD1 (y, from, n) / sineD1 (a, tone, fs));
        worst2 = std::max (worst2, maxD2 (y, from, n) / sineD2 (a, tone, fs));
    }
    if (peakOut != nullptr) *peakOut = peak;
    if (d2Out != nullptr) *d2Out = worst2;
    return worst;
}

static void rowsMultiBand()
{
    for (int block : { 32, 64, 512, 1024 })
    {
        double peak = 0.0, d2 = 0.0;
        const double r = multiBandRatio (block, false, 50.0, &peak, &d2);
        row (r < 2.0, "8 bands (5 placements), f / gain / Q and gain scale automated per block, 50 Hz",
             juce::String::formatted ("block %d: largest first difference = %.2fx that of a steady tone at the output's peak (%.2f); limit of this probe 2x; "
                                      "second difference %.1fx", block, r, peak, d2));
    }
    const double c = multiBandRatio (64, true, 50.0);
    row (c > 20.0, "control: the same with a type change now and then reads as clicks", juce::String::formatted ("%.0fx", c));
}

//==============================================================================
// 9. The ramping path under every placement and on mono
//==============================================================================
static void rowsPlacement()
{
    const double fs = 48000.0; const int block = 512, n = 48000, edit = 8192;
    const auto in = noise (2, n, 1234, true);       // L = R
    auto render = [&] (int placement, int channels, bool withEdit, bool antiphase = false)
    {
        auto p = make (fs, channels, block);
        setBand (*p, 1, 0, 800.0f, -6.0f, 0.7f, 1, placement);
        juce::AudioBuffer<float> buf (channels, n);
        buf.copyFrom (0, 0, in, 0, 0, n);
        if (channels == 2) { buf.copyFrom (1, 0, in, 0, 0, n); if (antiphase) buf.applyGain (1, 0, n, -1.0f); }
        run (*p, buf, block, [&] (int, int start)
        {
            if (withEdit && start == edit)
            {
                setRaw (*p, bid (1, "freq"), 3000.0f); setRaw (*p, bid (1, "gain"), 18.0f); setRaw (*p, bid (1, "q"), 6.0f);
            }
        });
        return buf;
    };
    const auto st = render (0, 2, true), stNo = render (0, 2, false);
    const float* ref = st.getReadPointer (0);
    const int d0 = firstDiff (ref, stNo.getReadPointer (0), n);
    row (d0 >= edit && d0 <= edit + 16 && sameBits (ref, st.getReadPointer (1), n),
         "Stereo band, three parameters ramping, L = R in: L = R out, bit for bit",
         juce::String::formatted ("first sample that differs from the render without the edit: %d (edit at %d)", d0, edit));

    auto worstRel = [&] (const float* a, const float* b, int from, int to)
    {
        double w = 0.0, pk = 0.0;
        for (int i = from; i < to; ++i) { w = std::max (w, std::fabs ((double) a[i] - (double) b[i])); pk = std::max (pk, std::fabs ((double) b[i])); }
        return w / pk;
    };
    auto cmp = [&] (const float* a) { return sameBits (a, ref, n) ? juce::String ("bit for bit, the ramp included")
                                                                   : juce::String::formatted ("worst difference %.2g of the peak (not bit-identical)", worstRel (a, ref, 0, n)); };
    const auto le = render (1, 2, true), ri = render (2, 2, true), mono = render (0, 1, true), monoMid = render (3, 1, true);
    row (worstRel (le.getReadPointer (0), ref, 0, n) < 1e-6 && sameBits (le.getReadPointer (1), in.getReadPointer (0), n),
         "Left band through the same move: L equals the Stereo band's L, R is the input", cmp (le.getReadPointer (0)));
    row (worstRel (ri.getReadPointer (1), ref, 0, n) < 1e-6 && sameBits (ri.getReadPointer (0), in.getReadPointer (0), n),
         "Right band through the same move: R equals the Stereo band's L, L is the input", cmp (ri.getReadPointer (1)));
    row (worstRel (mono.getReadPointer (0), ref, 0, n) < 1e-6 && sameBits (monoMid.getReadPointer (0), mono.getReadPointer (0), n),
         "Mono layout (placement Stereo and Mid) through the same move: equals the Stereo band's L", cmp (mono.getReadPointer (0)));

    const auto mid = render (3, 2, true);
    const double em = std::max (worstRel (mid.getReadPointer (0), ref, 0, n), worstRel (mid.getReadPointer (1), ref, 0, n));
    const double emRamp = worstRel (mid.getReadPointer (0), ref, edit, edit + 1100);
    row (em < 1e-6, "Mid band, L = R in, through the same move: both channels equal the Stereo band's L",
         juce::String::formatted ("worst difference %.2g of the peak over the render, %.2g inside the ramp (float rounding is 6e-8)", em, emRamp));
    const auto side = render (4, 2, true, true);        // L = -R: all Side
    const float* sl = side.getReadPointer (0); const float* sr = side.getReadPointer (1);
    double es = worstRel (sl, ref, 0, n), anti = 0.0;
    for (int i = 0; i < n; ++i) anti = std::max (anti, std::fabs ((double) sl[i] + (double) sr[i]));
    row (es < 1e-6 && anti < 1e-6, "Side band, L = -R in, through the same move: L equals the Stereo band's L, R = -L",
         juce::String::formatted ("worst difference %.2g of the peak; largest |L + R| %.2g", es, anti));
}

//==============================================================================
// 10. Paths that clear the state during a ramp leave the band settled on its target
//==============================================================================
static void rowsClearDuringRamp()
{
    const double fs = 48000.0; const int block = 512, n = 16 * block;
    const auto in = noise (2, n, 777);
    const int tail = 8 * block;         // the last 8 blocks are compared

    // fresh instance at the target, fed the tail only
    auto fresh = [&]
    {
        auto p = make (fs, 2, block);
        setBand (*p, 1, 0, 300.0f, 18.0f, 3.0f, 1, 0);
        setBand (*p, 2, 2, 4000.0f, -9.0f, 1.0f, 1, 3);
        juce::AudioBuffer<float> buf (2, n - tail);
        for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, in, c, tail, n - tail);
        run (*p, buf, block);
        return buf;
    };
    const auto want = fresh();

    // mode 0: global bypass parameter, 1: processBlockBypassed, 2: band bypass on both bands, 3: no clearing (control)
    for (int mode = 0; mode < 4; ++mode)
    {
        auto p = make (fs, 2, block);
        setBand (*p, 1, 0, 2000.0f, -12.0f, 0.5f, 1, 0);
        setBand (*p, 2, 2, 500.0f, 9.0f, 1.0f, 1, 3);
        juce::AudioBuffer<float> buf;
        buf.makeCopyOf (in);
        juce::MidiBuffer midi;
        for (int index = 0; index < 16; ++index)
        {
            const int start = index * block;
            if (index == 4)         // the move starts; block 5 runs mid-ramp
            {
                setRaw (*p, bid (1, "freq"), 300.0f); setRaw (*p, bid (1, "gain"), 18.0f); setRaw (*p, bid (1, "q"), 3.0f);
                setRaw (*p, bid (2, "freq"), 4000.0f); setRaw (*p, bid (2, "gain"), -9.0f);
            }
            if (index == 5 && mode == 0) setRaw (*p, "bypass", 1.0f);
            if (index == 8 && mode == 0) setRaw (*p, "bypass", 0.0f);
            if (index == 5 && mode == 2) { setRaw (*p, bid (1, "bypass"), 1.0f); setRaw (*p, bid (2, "bypass"), 1.0f); }
            if (index == 8 && mode == 2) { setRaw (*p, bid (1, "bypass"), 0.0f); setRaw (*p, bid (2, "bypass"), 0.0f); }
            juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), 2, start, block);
            // mode 0 / 2: the edit of block 4 has been seen by block 4; the clearing starts at block 5 (a tick boundary,
            // 5.3 ms into the 20 ms ramp ... block 4 is 10.7 ms, so block 5 starts mid-ramp)
            if (mode == 1 && index >= 5 && index < 8) p->processBlockBypassed (view, midi); else p->processBlock (view, midi);
        }
        bool same = true; int diff = -1;
        for (int c = 0; c < 2; ++c)
        {
            const int d = firstDiff (buf.getReadPointer (c) + tail, want.getReadPointer (c), n - tail);
            if (d >= 0) { same = false; if (diff < 0) diff = d; }
        }
        static const char* names[4] = { "global bypass for 3 blocks, 10.7 ms into a 20 ms move: then settled on the target",
                                        "processBlockBypassed for 3 blocks, mid-move: then settled on the target",
                                        "both bands' own bypass for 3 blocks, mid-move: then settled on the target",
                                        "control: nothing clears the state (the move ends on its own): differs from a fresh instance" };
        if (mode < 3)
            row (same, names[mode], same ? "the 8 blocks after it equal a fresh instance at the target, bit for bit"
                                         : juce::String::formatted ("first difference at sample %d of the tail", diff));
        else
            row (! same, names[mode], same ? "did NOT differ" : juce::String::formatted ("first difference at sample %d of the tail", diff));
    }
}

//==============================================================================
// 11. Host blocks larger than the prepared size; prepared size not a multiple of K
//==============================================================================
static void rowsOversize()
{
    const double fs = 48000.0; const int n = 12 * 4096;
    const auto in = noise (2, n, 4242);
    auto render = [&] (int prepared, int block)
    {
        auto p = make (fs, 2, prepared);
        setBand (*p, 1, 0, 500.0f, 6.0f, 1.0f, 1, 0);
        setBand (*p, 2, 1, 120.0f, -6.0f, 1.0f, 3, 3);
        setBand (*p, 3, 4, 9000.0f, 0.0f, 0.8f, 2, 1);
        juce::AudioBuffer<float> buf;
        buf.makeCopyOf (in);
        run (*p, buf, block, [&] (int, int start)
        {
            if (start % 4096 != 0) return;
            const int e = start / 4096;
            juce::Random r (100 + e);
            setRaw (*p, bid (1, "freq"), 100.0f + 4000.0f * r.nextFloat()); setRaw (*p, bid (1, "gain"), -20.0f + 40.0f * r.nextFloat());
            setRaw (*p, bid (2, "q"), 0.3f + 3.0f * r.nextFloat());          setRaw (*p, bid (2, "gain"), -20.0f + 40.0f * r.nextFloat());
            setRaw (*p, bid (3, "freq"), 2000.0f + 9000.0f * r.nextFloat()); setRaw (*p, "gain_scale", 50.0f + 100.0f * r.nextFloat());
        });
        return buf;
    };
    const auto base = render (4096, 4096);
    auto eq = [&] (const juce::AudioBuffer<float>& b) { return sameBits (b.getReadPointer (0), base.getReadPointer (0), n) && sameBits (b.getReadPointer (1), base.getReadPointer (1), n); };
    const bool a = eq (render (512, 4096)), b = eq (render (100, 4096)), c = eq (render (7, 4096)), d = eq (render (512, 512)), e = eq (render (4096, 64));
    row (a && b && c && d && e, "events every 4096 samples: host block 4096 on prepared sizes 512 / 100 / 7, and blocks 512 / 64",
         juce::String::formatted ("bit-identical to prepared 4096, block 4096: %d %d %d %d %d (the chunk size does not move a tick)", a, b, c, d, e));
}

//==============================================================================
// 12. Re-prepare: the clock restarts
//==============================================================================
static void rowReprepare()
{
    const int block = 500, n = 20 * block;
    const auto in = noise (2, n, 99);
    auto tailRender = [&] (Proc& p)
    {
        juce::AudioBuffer<float> buf;
        buf.makeCopyOf (in);
        run (p, buf, block, [&] (int index, int)
        {
            if (index == 3) { setRaw (p, bid (1, "gain"), 15.0f); setRaw (p, bid (2, "freq"), 150.0f); }
            if (index == 7) { setRaw (p, bid (1, "q"), 8.0f); setRaw (p, "gain_scale", 150.0f); }
        });
        return buf;
    };
    auto setup = [] (Proc& p) { setBand (p, 1, 0, 700.0f, -3.0f, 1.0f, 1, 0); setBand (p, 2, 3, 60.0f, 0.0f, 1.0f, 3, 0); setRaw (p, "gain_scale", 100.0f); };

    auto a = make (44100.0, 2, 512);
    setup (*a);
    { auto warm = noise (2, 44100 + 7, 5); run (*a, warm, 512, [&] (int index, int) { if (index == 86) setRaw (*a, bid (1, "gain"), 20.0f); }); }
    setup (*a);     // back to the start values; the instance is mid-move when it is re-prepared
    a->setRateAndBufferSizeDetails (48000.0, block);
    a->prepareToPlay (48000.0, block);
    const auto got = tailRender (*a);

    auto b = make (48000.0, 2, block);
    setup (*b);
    const auto want = tailRender (*b);
    const int d = std::max (firstDiff (got.getReadPointer (0), want.getReadPointer (0), n), firstDiff (got.getReadPointer (1), want.getReadPointer (1), n));
    row (d < 0, "44.1 kHz for 44 107 samples, left mid-move, re-prepared at 48 kHz: moves after it",
         d < 0 ? "bit-identical to a fresh 48 kHz instance through two later moves (blocks of 500)" : juce::String::formatted ("first difference at %d", d));
}

//==============================================================================
// 13. Gain is inert on the four gainless types; settled after a move equals static
//==============================================================================
static void rowsInertAndSettled()
{
    const double fs = 48000.0; const int block = 512, n = 24000;
    const auto in = noise (2, n, 31);
    int same = 0;
    for (int type : { 3, 4, 5, 6 })
    {
        juce::AudioBuffer<float> out[2];
        for (int k = 0; k < 2; ++k)
        {
            auto p = make (fs, 2, block);
            setBand (*p, 1, type, 700.0f, 0.0f, 2.0f, 3, 0);
            out[k].makeCopyOf (in);
            run (*p, out[k], block, [&] (int index, int) { if (k == 1 && index == 9) { setRaw (*p, bid (1, "gain"), 24.0f); setRaw (*p, "gain_scale", 180.0f); } });
        }
        if (sameBits (out[0].getReadPointer (0), out[1].getReadPointer (0), n) && sameBits (out[0].getReadPointer (1), out[1].getReadPointer (1), n)) ++same;
    }
    row (same == 4, "gain and gain_scale edits on Low Cut, High Cut, Notch, Band Pass", juce::String::formatted ("output bit-identical to no edit: %d of 4", same));

    double worst = 0.0;
    struct C { int type, slope; float f0, g0, q0, f1, g1, q1; double tone; };
    for (const C& c : { C { 0, 1, 200.0f, -12.0f, 0.3f, 2000.0f, 9.0f, 5.0f, 1800.0 }, C { 1, 2, 5000.0f, 12.0f, 1.0f, 300.0f, -15.0f, 2.0f, 250.0 },
                        C { 2, 0, 300.0f, -6.0f, 1.0f, 3000.0f, 12.0f, 1.0f, 4000.0 }, C { 7, 3, 100.0f, 10.0f, 1.0f, 2500.0f, -10.0f, 0.7f, 800.0 },
                        C { 3, 4, 40.0f, 0.0f, 0.7f, 900.0f, 0.0f, 3.0f, 1000.0 } })
    {
        auto moved = make (fs, 2, block);
        setBand (*moved, 1, c.type, c.f0, c.g0, c.q0, c.slope, 0);
        { auto warm = noise (2, 9600, 3); run (*moved, warm, block); }
        setRaw (*moved, bid (1, "freq"), c.f1); setRaw (*moved, bid (1, "gain"), c.g1); setRaw (*moved, bid (1, "q"), c.q1);
        auto stat = make (fs, 2, block);
        setBand (*stat, 1, c.type, c.f1, c.g1, c.q1, c.slope, 0);
        worst = std::max (worst, std::fabs (steadyDb (*moved, fs, c.tone) - steadyDb (*stat, fs, c.tone)));
    }
    row (worst < 1e-4, "after a move of f, gain and Q (5 types): steady response equals a static instance's",
         juce::String::formatted ("worst difference %.2g dB", worst));
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    std::printf ("O-ParametricEQ verify 2.3 probe (independent of the harnesses)\n\n");
    rowsGainRamp();
    rowGainScaleRamp();
    rowsFreqLog();
    rowQLog();
    rowRestart();
    rowsClick();
    rowsOnePole();
    rowsMultiBand();
    rowsPlacement();
    rowsClearDuringRamp();
    rowsOversize();
    rowReprepare();
    rowsInertAndSettled();
    std::printf ("\n%d PASS, %d FAIL\n", nPass, nFail);
    return nFail == 0 ? 0 : 1;
}
