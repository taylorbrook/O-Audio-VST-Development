/*
   This file is part of O-SimpleReverb, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/
/*
    O-SimpleReverb render check (v1.11.0, extended v1.12.0 and v1.13.0)

    Offline console gate for the v1.11.0 and v1.12.0 review fixes. Build with
    -DOUARICON_BUILD_TESTS=ON, target O-SimpleReverb-render-check.

      1. Every factory preset recalls the TYPE its name says — in the DSP
         (roundToInt of the raw value), in the host (getIndex) and in the page
         (JS Math.round(norm * 5)).
      2. CHARACTER moves do not click: toggling it every 8 blocks keeps the
         wet output's max |second difference| near the held value (v1.10.0:
         332x..2154x).
      3. Ambient at 192 kHz renders finite, non-silent audio.
      4. The VU peak is HELD across blocks until read, then clears.
      5. (v1.12.0) Switches do not click, by HF burst: the peak |x| above
         3 kHz in the 30 ms after each toggle. LOW CUT on/off, LOW CUT freq
         jumps and CHARACTER Bright/centre crossings are gated in dB against
         a smoothed WET move; TYPE, whose types differ in steady HF, as a
         ratio over the steady HF of both types. v1.11.0: LOW CUT -1.1 dB,
         LPFREQ -23.5 dB, CHARACTER 10<->60 -46 dB (WET ref -60 dB); TYPE up
         to 5.4x.
      6. (v1.12.0) The six types sit within 1 dB of each other (K-weighted,
         BS.1770, pink noise, WET 100 / DRY 0). v1.11.0 spread: 7.7 dB.
      7. (v1.13.0) Flutter is pitch: a 1 kHz sine through FlutterDelay equals
         the ideal swept-delay sine, and the sweep swings the stated cents. Shimmer is an octave:
         a 1 kHz sine through OctaveUpShifter comes out at 2 kHz. In the
         plugin, Plate carries 2f content that Room does not.
      8. (v1.13.0) DECAY's top range works: Ambient at SIZE 100 rings longer
         at each of 1.25x / 1.6x / 2.0x. v1.12.0 clamped room size at 1.05x.
      9. (v1.14.0) Factory bank: 8 per type, one "Send" each (WET 100 /
         DRY 0), no stale file from a renamed preset, and every insert preset
         at or below +5 dB re input (K-weighted pink). v1.13.1: +0.4 .. +7.5.

    The factory bank is read from the INSTALLED Factory folder, which the
    processor rewrites only when its `.factory-version` sentinel differs. main()
    deletes the sentinel first, so the bank under test is this build's table.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <random>
#include <juce_dsp/juce_dsp.h>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

static int failures = 0;

static void check(bool ok, const juce::String& what)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    if (! ok) ++failures;
}

static std::unique_ptr<OSimpleReverbAudioProcessor> makeProcessor()
{
    std::unique_ptr<juce::AudioProcessor> base(createPluginFilter());
    return std::unique_ptr<OSimpleReverbAudioProcessor>(dynamic_cast<OSimpleReverbAudioProcessor*>(base.release()));
}

static void setParam(OSimpleReverbAudioProcessor& proc, const char* id, float value)
{
    auto* p = proc.parameters.getParameter(id);
    p->setValueNotifyingHost(p->convertTo0to1(value));
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

// ── v1.12.0 click metric ─────────────────────────────────────────────────────
// Toggles `id` between a and b every 24 blocks (256 @ 48 kHz) under a 300 Hz
// sine at 0.5, WET 100 / DRY 0. The output's left channel goes through a 4th-
// order Butterworth high-pass at 3 kHz; a click is a broadband burst there.
struct Burst { float db; float ratio; };  // db re 0.5; ratio over steady HF
static Burst hfBurst(const char* id, float a, float b, float lpOn = 0.0f)
{
    auto proc = makeProcessor();
    setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
    setParam(*proc, "LPON", lpOn); setParam(*proc, id, a);
    const int bs = 256, per = 24;
    juce::dsp::IIR::Filter<float> h1, h2;
    h1.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(48000.0, 3000.0f, 0.541f);
    h2.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(48000.0, 3000.0f, 1.307f);
    std::vector<float> hf;
    const auto out = renderSine(*proc, 48000.0, bs, per * 2 * 14, [&](int blk) {
        setParam(*proc, id, (blk / per) % 2 ? b : a);
    });
    for (float x : out) hf.push_back(std::abs(h2.processSample(h1.processSample(x))));
    auto peak = [&](size_t from, size_t to) {
        float m = 1.0e-7f;
        for (size_t i = from; i < to; ++i) m = std::max(m, hf[i]);
        return m;
    };
    const size_t W = 1440, T = (size_t) per * bs;
    float burst = 0.0f, worst = 0.0f;
    for (size_t t = 4 * T; t + T < hf.size(); t += T) {
        const float after = peak(t, t + W);
        burst = std::max(burst, after);
        worst = std::max(worst, after / std::max(peak(t - W, t), peak(t + T - W, t + T)));
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
// settle, with the processor's parameters as they stand.
static double loudnessDb(OSimpleReverbAudioProcessor& p)
{
    auto* proc = &p;
    const int bs = 512;
    proc->setPlayConfigDetails(2, 2, 48000.0, bs);
    proc->prepareToPlay(48000.0, bs);
    juce::AudioBuffer<float> buf(2, bs);
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
            buf.setSample(0, i, x); buf.setSample(1, i, x);
            const double k = kin(x);
            if (measure) sumIn += k * k;
        }
        proc->processBlock(buf, midi);
        for (int i = 0; i < bs; ++i) {
            const double l = kl(buf.getSample(0, i)), r = kr(buf.getSample(1, i));
            if (measure) sumOut += 0.5 * (l * l + r * r);
        }
    }
    return 10.0 * std::log10(sumOut / sumIn);
}

// Wet loudness of one type re its input (WET 100 / DRY 0).
static double typeLoudnessDb(int type)
{
    auto proc = makeProcessor();
    setParam(*proc, "TYPE", (float) type); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
    return loudnessDb(*proc);
}

// Goertzel power of x at f (Hz)
static double goertzel(const std::vector<float>& x, double f, double sr, size_t from = 0)
{
    const double w = juce::MathConstants<double>::twoPi * f / sr, c = 2.0 * std::cos(w);
    double s1 = 0, s2 = 0;
    for (size_t i = from; i < x.size(); ++i) { const double s0 = x[i] + c * s1 - s2; s2 = s1; s1 = s0; }
    return s1 * s1 + s2 * s2 - c * s1 * s2;
}

// A FlutterDelay's output against the analytic ideal sin(2 pi f0 (n - D(n)) / fs),
// where D(n) is the delay the unit was asked for. `err` proves it realises that
// trajectory; `cents` is the peak pitch deviation of the trajectory itself,
// from its largest per-sample slope (the ratio is 1 - D'(n)). A zero-crossing
// period count was tried first: it showed isolated one-period spikes
// (4.5 cents on a 3-cent Hall) that the ideal-sine comparison rules out.
struct FlutterResult { double err; float cents; };
static FlutterResult flutterAgainstIdeal(float rate, float cents, double sr, double f0 = 1000.0)
{
    FlutterDelay fl;
    fl.prepare(sr, 5.0f);
    fl.setModulation(rate, cents);
    const double depth = fl.getDepthSamples();
    const int n = static_cast<int>(sr * (1.0 / rate + 0.5));
    double lfo = 0.0, err = 0.0, prevD = -1.0, slope = 0.0;
    for (int i = 0; i < n; ++i) {
        const float l = static_cast<float>(std::sin(lfo));
        const float y = fl.process(static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * f0 * i / sr)), l);
        const double d = depth + 2.0 + depth * l;
        if (i > 1000)
            err = std::max(err, std::abs(y - std::sin(juce::MathConstants<double>::twoPi * f0 * (i - d) / sr)));
        if (prevD >= 0.0) slope = std::max(slope, std::abs(d - prevD));
        prevD = d;
        lfo += juce::MathConstants<double>::twoPi * rate / sr;
    }
    return { err, static_cast<float>(1200.0 * std::log2(1.0 + slope)) };
}

// Time (s) for Ambient's tail to fall 40 dB after 1 s of noise stops.
static double ambientTailSeconds(float decay)
{
    auto proc = makeProcessor();
    setParam(*proc, "TYPE", 5.0f); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
    setParam(*proc, "SIZE", 100.0f); setParam(*proc, "DECAY", decay);
    const int bs = 480;   // 10 ms @ 48 kHz
    proc->setPlayConfigDetails(2, 2, 48000.0, bs);
    proc->prepareToPlay(48000.0, bs);
    juce::AudioBuffer<float> buf(2, bs);
    juce::MidiBuffer midi;
    std::mt19937 rng(3);
    std::normal_distribution<float> nd(0.0f, 0.1f);
    double ref = -1.0;
    for (int blk = 0; blk < 100 + 3000; ++blk) {
        for (int i = 0; i < bs; ++i) {
            const float x = blk < 100 ? nd(rng) : 0.0f;
            buf.setSample(0, i, x); buf.setSample(1, i, x);
        }
        proc->processBlock(buf, midi);
        if (blk < 100) continue;
        double e = 0.0;
        for (int i = 0; i < bs; ++i) e += buf.getSample(0, i) * buf.getSample(0, i);
        if (blk == 105) ref = e;                         // 50 ms after the stop
        if (blk > 105 && e < ref * 1.0e-4) return (blk - 100) * 0.01;
    }
    return 30.0;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

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

    // ── 3. Ambient at 192 kHz ────────────────────────────────────────────────
    {
        auto proc = makeProcessor();
        setParam(*proc, "TYPE", 5.0f); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
        setParam(*proc, "DECAY", 2.0f); setParam(*proc, "SIZE", 100.0f);
        const auto out = renderSine(*proc, 192000.0, 512, 750, [](int) {});
        bool finite = true; float peak = 0.0f;
        for (float x : out) { finite = finite && std::isfinite(x); peak = std::max(peak, std::abs(x)); }
        check(finite && peak > 0.01f, "Ambient @ 192 kHz renders finite, non-silent (peak "
                                      + juce::String(peak, 4) + ")");
    }

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

    // ── 5. v1.12.0: switches do not click (HF burst) ───────────────────────────
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
        float worst = 0.0f;
        const int pairs[][2] = { { 1, 2 }, { 1, 3 }, { 2, 4 }, { 0, 5 }, { 1, 0 }, { 3, 4 } };
        for (const auto& pr : pairs)
            worst = std::max(worst, hfBurst("TYPE", (float) pr[0], (float) pr[1]).ratio);
        check(worst < 2.0f, "TYPE switch HF burst over steady HF " + juce::String(worst, 2) + "x < 2 (worst of 6 pairs)");
    }

    // ── 6. v1.12.0: types are level-matched ──────────────────────────────────
    {
        double lo = 1.0e9, hi = -1.0e9;
        for (int t = 0; t < 6; ++t) { const double l = typeLoudnessDb(t); lo = std::min(lo, l); hi = std::max(hi, l); }
        check(hi - lo <= 1.0, "type loudness spread " + juce::String(hi - lo, 2) + " dB <= 1 (K-weighted pink)");
    }

    // ── 7. v1.13.0: flutter is pitch, shimmer is an octave ────────────────────
    {
        struct F { const char* name; float rate, cents; };
        for (auto f : { F { "Spring", 4.5f, 6.0f }, F { "Hall", 0.15f, 3.0f }, F { "Ambient", 0.4f, 4.0f } }) {
            const auto r = flutterAgainstIdeal(f.rate, f.cents, 48000.0);
            check(r.err < 1.0e-4 && std::abs(r.cents - f.cents) < 0.25f,
                  juce::String(f.name) + " flutter: output = ideal swept delay (max err " + juce::String(r.err, 7)
                  + "), swing " + juce::String(r.cents, 2) + " cents (stated " + juce::String(f.cents, 0) + ")");
        }

        OctaveUpShifter oct;
        oct.prepare(48000.0);
        std::vector<float> y;
        for (int i = 0; i < 48000; ++i) y.push_back(oct.process(0.5f * (float) std::sin(juce::MathConstants<double>::twoPi * 1000.0 * i / 48000.0)));
        const double octDb = 10.0 * std::log10(goertzel(y, 2000.0, 48000.0, 4800) / goertzel(y, 1000.0, 48000.0, 4800));
        check(octDb > 20.0, "octave shifter: 2 kHz over 1 kHz " + juce::String(octDb, 1) + " dB > 20");

        auto twoF = [](int type) {
            auto proc = makeProcessor();
            setParam(*proc, "TYPE", (float) type); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
            const auto out = renderSine(*proc, 48000.0, 256, 750, [](int) {});
            return 10.0 * std::log10(goertzel(out, 600.0, 48000.0, 48000) / goertzel(out, 300.0, 48000.0, 48000));
        };
        const double plate = twoF(4), room = twoF(1);
        check(plate > room + 20.0, "Plate carries the octave: 600/300 Hz " + juce::String(plate, 1)
                                   + " dB vs Room " + juce::String(room, 1) + " dB (> +20)");
    }

    // ── 8. v1.13.0: DECAY's top range is live ─────────────────────────────────
    {
        const double t1 = ambientTailSeconds(1.25f), t2 = ambientTailSeconds(1.6f), t3 = ambientTailSeconds(2.0f);
        check(t2 > t1 * 1.1 && t3 > t2 * 1.1, "Ambient SIZE 100 tail -40 dB: 1.25x " + juce::String(t1, 2) + " s, 1.6x "
                                              + juce::String(t2, 2) + " s, 2.0x " + juce::String(t3, 2) + " s (each > +10 %)");
    }

    // ── 9. v1.14.0: factory bank shape and level ─────────────────────────────
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
        check(hi <= 5.0, "insert presets " + juce::String(lo, 1) + " (" + loName + ") .. "
                         + juce::String(hi, 1) + " dB (" + hiName + ") re input, ceiling +5.0");
    }

    std::printf("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
