// Independent probe for O-ParametricEQ Phase 2.2 verify (written in the verify session).
// Drives the real processor through its public API and scores against analog facts
// (peak gain, Butterworth magnitudes, asymptotes). It uses none of Source/dsp and
// none of the render harness.
#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

using Proc = OuariconParametricEQAudioProcessor;

static int nPass = 0, nFail = 0;

static void row (bool ok, const char* name, const juce::String& detail)
{
    std::printf ("  [%s] %-58s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
    (ok ? nPass : nFail)++;
}

static void info (const char* name, const juce::String& detail)
{
    std::printf ("  [INFO] %-58s %s\n", name, detail.toRawUTF8());
}

static void setRaw (Proc& p, const juce::String& id, float raw)
{
    auto* prm = p.parameters.getParameter (id);
    jassert (prm != nullptr);
    if (prm == nullptr) { std::printf ("NO PARAM %s\n", id.toRawUTF8()); std::exit (2); }
    prm->setValueNotifyingHost (prm->convertTo0to1 (raw));
}

static juce::String bid (int band, const char* suffix)   // band 1 ... 24
{
    return "b" + juce::String (band).paddedLeft ('0', 2) + "_" + suffix;
}

// type: 0 Bell 1 LowShelf 2 HighShelf 3 LowCut 4 HighCut 5 Notch 6 BandPass 7 Tilt
// slope index: 0 = 6, 1 = 12, 2 = 18, 3 = 24, 4 = 36, 5 = 48, 6 = 72, 7 = 96 dB/oct
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

static void run (Proc& p, juce::AudioBuffer<float>& buf, int block = 512)
{
    juce::MidiBuffer midi;
    const int n = buf.getNumSamples();
    for (int start = 0; start < n; start += block)
    {
        const int len = juce::jmin (block, n - start);
        juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), buf.getNumChannels(), start, len);
        p.processBlock (view, midi);
    }
}

// Least-squares amplitude of a sinusoid of known frequency over [from, to).
static double fitAmp (const float* x, int from, int to, double f, double fs)
{
    double scc = 0, sss = 0, scs = 0, sxc = 0, sxs = 0;
    const double w = 2.0 * juce::MathConstants<double>::pi * f / fs;
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

// Steady-state gain in dB per channel for a sine at f. lr: +1 = L = R, -1 = L = -R.
static void sineGain (Proc& p, double fs, double f, double out[2], int channels = 2, double lr = 1.0, double seconds = 3.0)
{
    const int n = (int) (fs * seconds);
    juce::AudioBuffer<float> in (channels, n), buf;
    const double w = 2.0 * juce::MathConstants<double>::pi * f / fs;
    for (int i = 0; i < n; ++i)
    {
        const float v = (float) (0.25 * std::sin (w * i));
        in.setSample (0, i, v);
        if (channels == 2) in.setSample (1, i, (float) (lr * v));
    }
    buf.makeCopyOf (in);
    run (p, buf);
    const int from = n - (int) (fs * 0.5);
    for (int c = 0; c < channels; ++c)
        out[c] = 20.0 * std::log10 (fitAmp (buf.getReadPointer (c), from, n, f, fs) / fitAmp (in.getReadPointer (c), from, n, f, fs));
}

static double g1 (Proc& p, double fs, double f)
{
    double g[2];
    sineGain (p, fs, f, g);
    return g[0];
}

static juce::AudioBuffer<float> noise (int channels, int n, juce::int64 seed)
{
    juce::Random r (seed);
    juce::AudioBuffer<float> b (channels, n);
    for (int c = 0; c < channels; ++c)
        for (int i = 0; i < n; ++i)
            b.setSample (c, i, 0.5f * (r.nextFloat() * 2.0f - 1.0f));
    return b;
}

static bool same (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, int ch)
{
    return std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), sizeof (float) * (size_t) a.getNumSamples()) == 0;
}

static double bw (double ratio, int order)   // Butterworth magnitude in dB at f / fc = ratio (low-pass form)
{
    return -10.0 * std::log10 (1.0 + std::pow (ratio, 2.0 * order));
}

static juce::String d (double v, int places = 4) { return juce::String (v, places); }

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0;

    std::printf ("O-ParametricEQ independent probe (verify 2.2), 48 kHz unless stated\n\n-- types against analog facts\n");

    {   // Bell
        auto p = make (fs, 2);
        setBand (*p, 1, 0, 1000.0f, 12.0f, 1.0f, 1, 0);
        double g[2]; sineGain (*p, fs, 1000.0, g);
        row (std::abs (g[0] - 12.0) < 0.01 && std::abs (g[1] - 12.0) < 0.01, "Bell 1 kHz +12 dB: gain at centre, L and R", d (g[0]) + " / " + d (g[1]) + " dB (12)");
        const double lo = g1 (*p, fs, 20.0), hi = g1 (*p, fs, 20000.0);
        row (std::abs (lo) < 0.05 && std::abs (hi) < 0.3, "Bell: far skirts return to 0 dB (20 Hz, 20 kHz)", d (lo) + " / " + d (hi) + " dB");
        row (p->getLatencySamples() == 0 && p->getTailLengthSeconds() == 2.0, "latency 0, tail 2.0", juce::String (p->getLatencySamples()) + " / " + d (p->getTailLengthSeconds(), 1));
    }
    {   // Low Shelf
        auto p = make (fs, 2);
        setBand (*p, 2, 1, 200.0f, 9.0f, 1.0f, 1, 0);
        const double dc = g1 (*p, fs, 10.0), hf = g1 (*p, fs, 10000.0), mid = g1 (*p, fs, 200.0);
        row (std::abs (dc - 9.0) < 0.1 && std::abs (hf) < 0.05, "Low Shelf 200 Hz +9 dB 12 dB/oct: 10 Hz -> +9, 10 kHz -> 0", d (dc) + " / " + d (hf) + " dB");
        info ("Low Shelf: gain at the corner", d (mid) + " dB (half gain = 4.5)");
    }
    {   // High Shelf
        auto p = make (fs, 2);
        setBand (*p, 3, 2, 2000.0f, -12.0f, 1.0f, 1, 0);
        const double lf = g1 (*p, fs, 20.0), hf = g1 (*p, fs, 20000.0);
        row (std::abs (lf) < 0.05 && std::abs (hf + 12.0) < 0.2, "High Shelf 2 kHz -12 dB 12 dB/oct: 20 Hz -> 0, 20 kHz -> -12", d (lf) + " / " + d (hf) + " dB");
    }
    {   // Low Cut, Butterworth at Q 1 (CONTEXT D9), every slope
        const int order[8] = { 1, 2, 3, 4, 6, 8, 12, 16 };
        int ok = 0; double worst = 0.0; juce::String list;
        for (int s = 0; s < 8; ++s)
        {
            auto p = make (fs, 2);
            setBand (*p, 4, 3, 1000.0f, 0.0f, 1.0f, s, 0);
            const double atFc = g1 (*p, fs, 1000.0), oct = g1 (*p, fs, 500.0);
            const double e1 = std::abs (atFc - bw (1.0, order[s])), e2 = std::abs (oct - bw (2.0, order[s]));
            worst = std::max (worst, std::max (e1, e2));
            if (e1 < 0.05 && e2 < 0.1) ++ok;
            list << d (oct, 2) << " ";
        }
        row (ok == 8, "Low Cut 1 kHz, 8 slopes: -3.01 dB at fc, Butterworth one octave down", juce::String (ok) + "/8, worst error " + d (worst) + " dB; 500 Hz: " + list);
    }
    {   // High Cut, every slope
        const int order[8] = { 1, 2, 3, 4, 6, 8, 12, 16 };
        int ok = 0; double worst = 0.0; juce::String list;
        for (int s = 0; s < 8; ++s)
        {
            auto p = make (fs, 2);
            setBand (*p, 5, 4, 1000.0f, 0.0f, 1.0f, s, 0);
            const double atFc = g1 (*p, fs, 1000.0), oct = g1 (*p, fs, 2000.0);
            const double e1 = std::abs (atFc - bw (1.0, order[s])), e2 = std::abs (oct - bw (2.0, order[s]));
            worst = std::max (worst, std::max (e1, e2));
            if (e1 < 0.05 && e2 < 0.1) ++ok;
            list << d (oct, 2) << " ";
        }
        row (ok == 8, "High Cut 1 kHz, 8 slopes: -3.01 dB at fc, Butterworth one octave up", juce::String (ok) + "/8, worst error " + d (worst) + " dB; 2 kHz: " + list);
    }
    {   // Notch
        auto p = make (fs, 2);
        setBand (*p, 6, 5, 1000.0f, 0.0f, 10.0f, 1, 0);
        const double c = g1 (*p, fs, 1000.0), lo = g1 (*p, fs, 100.0), hi = g1 (*p, fs, 10000.0);
        row (c < -60.0 && std::abs (lo) < 0.05 && std::abs (hi) < 0.05, "Notch 1 kHz Q 10: null at centre, 0 dB a decade away", d (c, 1) + " / " + d (lo) + " / " + d (hi) + " dB");
    }
    {   // Band Pass
        auto p = make (fs, 2);
        setBand (*p, 7, 6, 500.0f, 0.0f, 2.0f, 1, 0);
        const double c = g1 (*p, fs, 500.0), lo = g1 (*p, fs, 50.0), hi = g1 (*p, fs, 5000.0);
        // analog band-pass, unity peak: |H| = 1 / sqrt (1 + Q^2 (r - 1/r)^2)
        const double ref = -10.0 * std::log10 (1.0 + 4.0 * std::pow (10.0 - 0.1, 2.0));
        row (std::abs (c) < 0.01 && std::abs (lo - ref) < 0.05 && std::abs (hi - ref) < 0.1, "Band Pass 500 Hz Q 2: 0 dB at centre, analog skirt a decade away", d (c) + " / " + d (lo, 3) + " / " + d (hi, 3) + " dB (skirt " + d (ref, 3) + ")");
    }
    {   // Tilt (CONTEXT D8: -g/2 below, +g/2 above, 0 at the pivot)
        auto p = make (fs, 2);
        setBand (*p, 8, 7, 1000.0f, 6.0f, 1.0f, 1, 0);
        const double lo = g1 (*p, fs, 10.0), c = g1 (*p, fs, 1000.0), hi = g1 (*p, fs, 20000.0);
        row (std::abs (lo + 3.0) < 0.05 && std::abs (c) < 0.02 && std::abs (hi - 3.0) < 0.1, "Tilt 1 kHz +6 dB: -3 dB low, 0 dB at the pivot, +3 dB high", d (lo) + " / " + d (c) + " / " + d (hi) + " dB");
    }

    std::printf ("\n-- wiring: each band, gain scale, band bypass\n");
    {
        int ok = 0; double worst = 0.0;
        for (int b = 1; b <= 24; ++b)
        {
            auto p = make (fs, 2);
            const float gain = 0.5f * (float) b;
            setBand (*p, b, 0, 1000.0f, gain, 1.0f, 1, 0);
            const double e = std::abs (g1 (*p, fs, 1000.0) - (double) gain);
            worst = std::max (worst, e);
            if (e < 0.01) ++ok;
        }
        row (ok == 24, "band b alone as a bell of b/2 dB: centre gain = b/2 dB", juce::String (ok) + "/24, worst error " + d (worst, 6) + " dB");
    }
    {
        auto p = make (fs, 2);
        setBand (*p, 5, 0, 1000.0f, 12.0f, 1.0f, 1, 0);
        setRaw (*p, "gain_scale", 200.0f);  const double g200 = g1 (*p, fs, 1000.0);
        setRaw (*p, "gain_scale", 50.0f);   const double g50 = g1 (*p, fs, 1000.0);
        setRaw (*p, "gain_scale", 0.0f);    const double g0 = g1 (*p, fs, 1000.0), g0b = g1 (*p, fs, 300.0);
        row (std::abs (g200 - 24.0) < 0.01 && std::abs (g50 - 6.0) < 0.01 && std::abs (g0) < 1e-4 && std::abs (g0b) < 1e-4,
             "gain_scale 200 / 50 / 0 % on a +12 dB bell: +24 / +6 / 0 dB", d (g200) + " / " + d (g50) + " / " + d (g0, 7) + " dB");
    }
    {
        auto p = make (fs, 2);
        setBand (*p, 9, 0, 1000.0f, 12.0f, 1.0f, 1, 0);
        setRaw (*p, bid (9, "bypass"), 1.0f);
        auto in = noise (2, 48000, 11);
        juce::AudioBuffer<float> out; out.makeCopyOf (in);
        run (*p, out);
        const bool eq1 = same (in, out, 0) && same (in, out, 1);
        setRaw (*p, bid (9, "bypass"), 0.0f);
        out.makeCopyOf (in); run (*p, out);
        row (eq1 && ! same (in, out, 0), "bNN_bypass on an enabled band: output memcmp-equal to input; off: differs", eq1 ? "equal, then differs" : "NOT equal");
    }

    std::printf ("\n-- placement\n");
    {
        auto p = make (fs, 2);
        setBand (*p, 1, 0, 1000.0f, 12.0f, 1.0f, 1, 3);   // Mid
        double gm[2], gs[2];
        sineGain (*p, fs, 1000.0, gm, 2, 1.0);
        auto q = make (fs, 2);
        setBand (*q, 1, 0, 1000.0f, 12.0f, 1.0f, 1, 3);
        sineGain (*q, fs, 1000.0, gs, 2, -1.0);
        row (std::abs (gm[0] - 12.0) < 0.01 && std::abs (gm[1] - 12.0) < 0.01 && std::abs (gs[0]) < 1e-6 && std::abs (gs[1]) < 1e-6,
             "Mid bell +12 dB: L = R input gains 12 dB; L = -R input unchanged", d (gm[0]) + " / " + d (gm[1]) + " ; " + d (gs[0], 9) + " / " + d (gs[1], 9) + " dB");
    }
    {
        auto p = make (fs, 2);
        setBand (*p, 1, 0, 1000.0f, 12.0f, 1.0f, 1, 4);   // Side
        double gm[2], gs[2];
        sineGain (*p, fs, 1000.0, gs, 2, -1.0);
        auto q = make (fs, 2);
        setBand (*q, 1, 0, 1000.0f, 12.0f, 1.0f, 1, 4);
        sineGain (*q, fs, 1000.0, gm, 2, 1.0);
        row (std::abs (gs[0] - 12.0) < 0.01 && std::abs (gs[1] - 12.0) < 0.01 && std::abs (gm[0]) < 1e-6 && std::abs (gm[1]) < 1e-6,
             "Side bell +12 dB: L = -R input gains 12 dB; L = R input unchanged", d (gs[0]) + " / " + d (gs[1]) + " ; " + d (gm[0], 9) + " / " + d (gm[1], 9) + " dB");
    }
    {
        auto p = make (fs, 2);
        setBand (*p, 1, 0, 1000.0f, 12.0f, 1.0f, 1, 1);   // Left
        double g[2]; sineGain (*p, fs, 1000.0, g);
        auto q = make (fs, 2);
        setBand (*q, 1, 0, 1000.0f, 12.0f, 1.0f, 1, 2);   // Right
        double h[2]; sineGain (*q, fs, 1000.0, h);
        row (std::abs (g[0] - 12.0) < 0.01 && g[1] == 0.0 && std::abs (h[1] - 12.0) < 0.01 && h[0] == 0.0,
             "Left bell: L +12, R exactly 0 dB; Right bell: the converse", d (g[0]) + " / " + d (g[1]) + " ; " + d (h[0]) + " / " + d (h[1]) + " dB");
    }
    {
        auto p = make (fs, 1);
        setBand (*p, 1, 0, 1000.0f, 12.0f, 1.0f, 1, 4);   // Side on mono
        double g[2] = { 0, 0 }; sineGain (*p, fs, 1000.0, g, 1);
        row (std::abs (g[0] - 12.0) < 0.01, "mono layout, placement Side: the channel gains 12 dB", d (g[0]) + " dB");
    }

    std::printf ("\n-- instance life cycle\n");
    {
        // Same instance re-prepared at another rate vs a fresh instance at that rate.
        auto p = make (44100.0, 2);
        setBand (*p, 1, 1, 8000.0f, 9.0f, 2.0f, 3, 0);
        setBand (*p, 2, 4, 15000.0f, 0.0f, 1.0f, 5, 0);
        auto in = noise (2, 96000, 5);
        juce::AudioBuffer<float> a; a.makeCopyOf (in); run (*p, a);
        p->setRateAndBufferSizeDetails (96000.0, 512);
        p->prepareToPlay (96000.0, 512);
        a.makeCopyOf (in); run (*p, a);
        auto q = make (96000.0, 2);
        setBand (*q, 1, 1, 8000.0f, 9.0f, 2.0f, 3, 0);
        setBand (*q, 2, 4, 15000.0f, 0.0f, 1.0f, 5, 0);
        juce::AudioBuffer<float> b; b.makeCopyOf (in); run (*q, b);
        row (same (a, b, 0) && same (a, b, 1) && ! same (a, in, 0), "re-prepared 44.1 -> 96 kHz == a fresh instance at 96 kHz (memcmp)", same (a, b, 0) ? "equal" : "DIFFERENT");
    }
    {
        // State round trip: a restored instance renders the same bits.
        auto p = make (fs, 2);
        setBand (*p, 3, 0, 250.0f, -7.5f, 3.0f, 1, 3);
        setBand (*p, 11, 3, 60.0f, 0.0f, 1.3f, 4, 1);
        setBand (*p, 24, 7, 2500.0f, 4.0f, 1.0f, 2, 4);
        setRaw (*p, "gain_scale", 140.0f);
        juce::MemoryBlock blob; p->getStateInformation (blob);
        auto q = make (fs, 2);
        q->setStateInformation (blob.getData(), (int) blob.getSize());
        auto in = noise (2, 48000, 9);
        juce::AudioBuffer<float> a, b; a.makeCopyOf (in); b.makeCopyOf (in);
        run (*p, a); run (*q, b);
        row (same (a, b, 0) && same (a, b, 1) && ! same (a, in, 0) && ! same (a, in, 1), "state saved and restored into a fresh instance: same bits", same (a, b, 0) && same (a, b, 1) ? "equal" : "DIFFERENT");
    }
    {
        // Double-precision entry point: what does the plugin do when a host asks for double?
        auto p = make (fs, 2);
        info ("supportsDoublePrecisionProcessing()", p->supportsDoublePrecisionProcessing() ? "true" : "false (hosts use the float path)");
    }

    std::printf ("\n%d PASS, %d FAIL\n", nPass, nFail);
    return nFail == 0 ? 0 : 1;
}
