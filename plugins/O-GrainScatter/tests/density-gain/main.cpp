/*
  ==============================================================================

    O-GrainScatter — density compensation + freeze jitter gate (v3.2.0)

    Built three times: against Source/ (target O-GrainScatter-density-gain), against
    the v3.1.0 backup (O-GrainScatter-density-gain-baseline, GS_DENSITY_BASELINE),
    which prints the same metric table as the negative control, and (v3.3.0) against
    Source/ with GS_TEST_DENSITY_COMP_OFF (O-GrainScatter-density-gain-compoff,
    which also defines GS_DENSITY_BASELINE): the sparse row's reference. Each
    reference target dumps its sparse render; the current build reads that dump.

      moments    windowMoments() matches a numeric integral of computeEnvelope()
                 for all six shapes (rel. error <= 0.5 %)
      incoh      noise, Spread 100: wet level within +-1.5 dB of -3 dB (the
                 centre-pan law) for every density with N >= 4, including
                 Scan settings that pin part of the cloud to the read floor
      coh        same, Spread 0 / no pitch / no reverse (grains read the same
                 audio in step): within +-1.5 dB for N >= 4
      thd        0.5-peak sine, N = 20 incoherent: H3/H1 <= -60 dB (the tanh
                 made density a distortion control)
      sparse     N << 1, quiet sine: nulls against the same build with density
                 compensation forced off to <= -70 dB (gain is exactly 1 below one
                 grain). v3.2.0 nulled against v3.1.0; v3.3.0's band-limited reader
                 changes every grain's samples (pitch_random 50 on this row), so the
                 reference moved to the build's own compensation-off render (plan
                 decision 5). Threshold unchanged.
      freeze     frozen noise, Spread 0: output correlation one capture loop
                 apart <= 0.5 (v3.1.0: 0.29, coherent OLA of the loop); frozen level within
                 +-1.5 dB of the live incoherent level
      onset      cold start into N = 20: worst 20 ms level over the first 1 s
                 <= +6 dB over steady state (the 100 ms smoother lags the cloud)

    Usage: density-gain [reference-dump-dir]. Exit 0 = every row PASS.
      O-GrainScatter-density-gain-compoff <dir> && O-GrainScatter-density-gain <dir>

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlock      = 512;
    constexpr double kPi         = juce::MathConstants<double>::pi;

    int failures = 0;

    void row (bool ok, const juce::String& name, const juce::String& detail)
    {
       #ifdef GS_DENSITY_BASELINE
        juce::ignoreUnused (ok);
        std::printf ("  [ -- ] %-34s %s\n", name.toRawUTF8(), detail.toRawUTF8());
       #else
        std::printf ("  [%s] %-34s %s\n", ok ? "PASS" : "FAIL", name.toRawUTF8(), detail.toRawUTF8());
        if (! ok) ++failures;
       #endif
    }

    double toDb (double x) { return x > 0.0 ? 20.0 * std::log10 (x) : -400.0; }

    using Params = std::map<juce::String, float>;

    // Everything random off, dry muted, wet 0 dB, Free mode, Hann.
    Params base()
    {
        return { { "dry_level", -60.0f }, { "wet_level", 0.0f }, { "feedback", 0.0f },
                 { "sync_mode", 0.0f },   { "probability", 100.0f },
                 { "pitch_random", 0.0f }, { "pitch_transpose", 0.0f }, { "pitch_fine", 0.0f },
                 { "pan_random", 0.0f },  { "reverse", 0.0f }, { "size_random", 0.0f },
                 { "amp_random", 0.0f },  { "scan_position", 10.0f }, { "grain_shape", 0.0f },
                 { "spatial_mode", 0.0f }, { "freeze", 0.0f }, { "spread", 100.0f } };
    }

    void setParams (juce::AudioProcessor& p, const Params& params)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
            {
                auto it = params.find (rp->getParameterID());
                if (it != params.end())
                    rp->setValueNotifyingHost (rp->convertTo0to1 (it->second));
            }
    }

    std::unique_ptr<GrainScatterProcessor> makeInstance (const Params& params)
    {
        std::unique_ptr<GrainScatterProcessor> p (dynamic_cast<GrainScatterProcessor*> (createPluginFilter()));
        p->setPlayConfigDetails (2, 2, kSampleRate, kBlock);
        setParams (*p, params);
        p->setRngSeed (0xD3A51717ull);
        p->prepareToPlay (kSampleRate, kBlock);
        p->reset();
        return p;
    }

    // Renders `in`; `atSample` (if >= 0) applies `change` before the block starting there.
    juce::AudioBuffer<float> render (GrainScatterProcessor& p, const juce::AudioBuffer<float>& in,
                                     int atSample = -1, const Params& change = {})
    {
        const int len = in.getNumSamples();
        juce::AudioBuffer<float> out (2, len), block (2, kBlock);
        juce::MidiBuffer midi;
        for (int pos = 0; pos < len; pos += kBlock)
        {
            if (pos == atSample) setParams (p, change);
            const int n = juce::jmin (kBlock, len - pos);
            block.setSize (2, n, false, false, true);
            for (int ch = 0; ch < 2; ++ch) block.copyFrom (ch, 0, in, ch, pos, n);
            p.processBlock (block, midi);
            for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, block, ch, 0, n);
        }
        return out;
    }

    juce::AudioBuffer<float> noise (int len, float rmsLevel)
    {
        juce::AudioBuffer<float> b (2, len);
        juce::Random r (0x5EEDull);
        const float a = rmsLevel * std::sqrt (3.0f);   // uniform: rms = a / sqrt 3
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < len; ++i)
                b.setSample (ch, i, (r.nextFloat() * 2.0f - 1.0f) * a);
        return b;
    }

    juce::AudioBuffer<float> sine (int len, double hz, float peak)
    {
        juce::AudioBuffer<float> b (2, len);
        for (int i = 0; i < len; ++i)
        {
            const float v = peak * static_cast<float> (std::sin (2.0 * kPi * hz * i / kSampleRate));
            b.setSample (0, i, v);
            b.setSample (1, i, v);
        }
        return b;
    }

    double rms (const juce::AudioBuffer<float>& a, int from, int to)
    {
        double s = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = from; i < to; ++i)
                s += static_cast<double> (a.getSample (ch, i)) * a.getSample (ch, i);
        return std::sqrt (s / (2.0 * (to - from)));
    }

    // Hann-windowed single-bin DFT magnitude (channel 0).
    double toneMag (const juce::AudioBuffer<float>& a, int from, int to, double hz)
    {
        double re = 0.0, im = 0.0;
        const int n = to - from;
        for (int i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (2.0 * kPi * i / (n - 1));
            const double x = w * a.getSample (0, from + i);
            const double ph = 2.0 * kPi * hz * (from + i) / kSampleRate;
            re += x * std::cos (ph);
            im -= x * std::sin (ph);
        }
        return std::hypot (re, im);
    }

    // Interval of the Free scheduler (GrainScheduler::processBlockFree).
    double intervalMs (float density) { return 1000.0 * std::pow (0.01, density / 100.0); }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
   #if defined (GS_TEST_DENSITY_COMP_OFF)
    std::printf ("O-GrainScatter density gate — REFERENCE v3.3.0, density compensation OFF (no verdicts)\n\n");
   #elif defined (GS_DENSITY_BASELINE)
    std::printf ("O-GrainScatter density gate — BASELINE v3.1.0 (negative control, no verdicts)\n\n");
   #else
    std::printf ("O-GrainScatter density gate — v3.3.0\n\n");
   #endif

    const int kSteadyFrom = static_cast<int> (2.0 * kSampleRate);
    const int kLen        = static_cast<int> (5.0 * kSampleRate);
    const double inRms    = 0.1;
    const double panLaw   = std::cos (kPi / 4.0);   // centre pan, equal power

   #ifndef GS_DENSITY_BASELINE
    // ── moments ────────────────────────────────────────────────────────────
    {
        const char* names[] = { "Hann", "Triangle", "Trapezoid", "Tukey", "Blackman", "ExpDecay" };
        for (int shape = 0; shape < GrainShape::NumShapes; ++shape)
        {
            constexpr int n = 1 << 20;
            double m1 = 0.0, m2 = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const double w = GrainPool::computeEnvelope (static_cast<float> ((i + 0.5) / n), shape);
                m1 += w; m2 += w * w;
            }
            m1 /= n; m2 /= n;
            const auto mom = GrainPool::windowMoments (shape);
            const double e1 = std::abs (mom.mu1 / m1 - 1.0), e2 = std::abs (mom.mu2 / m2 - 1.0);
            row (e1 <= 0.005 && e2 <= 0.005, juce::String ("moments ") + names[shape],
                 juce::String::formatted ("mu1 %.4f (num %.4f)  mu2 %.4f (num %.4f)", mom.mu1, m1, mom.mu2, m2));
        }
    }
   #endif

    // ── level vs density ───────────────────────────────────────────────────
    const auto in = noise (kLen, static_cast<float> (inRms));
    auto levelRows = [&] (const char* tag, float spread, float sizeMs, std::initializer_list<float> densities,
                          double minN, float scan = 10.0f)
    {
        double lo = 1e9, hi = -1e9;
        for (float d : densities)
        {
            auto ps = base(); ps["spread"] = spread; ps["grain_size"] = sizeMs; ps["density"] = d;
            ps["scan_position"] = scan;
            auto p = makeInstance (ps);
            const auto out = render (*p, in);
            const double rel = toDb (rms (out, kSteadyFrom, kLen) / (inRms * panLaw));
            const double n = sizeMs / intervalMs (d);
            const bool gated = n >= minN;
            if (gated) { lo = juce::jmin (lo, rel); hi = juce::jmax (hi, rel); }
            row (! gated || std::abs (rel) <= 1.5,
                 juce::String::formatted ("%s %3.0f ms d=%3.0f N=%5.1f scan %2.0f", tag, sizeMs, d, n, scan),
                 juce::String::formatted ("%+6.2f dB re centre-pan input%s", rel, gated ? "" : "  (not gated)"));
        }
        std::printf ("         %s gated spread: %.2f dB\n", tag, hi - lo);
    };
    // N*mu2 >= 1.5 -> N >= 4 for Hann
    levelRows ("incoh", 100.0f, 200.0f, { 30.0f, 50.0f, 65.0f, 80.0f, 90.0f, 100.0f }, 4.0);
    levelRows ("incoh", 100.0f, 500.0f, { 100.0f }, 4.0);           // ~30 % of grains pinned to the read floor
    levelRows ("incoh", 100.0f, 500.0f, { 100.0f }, 4.0, 60.0f);    // none pinned
    levelRows ("incoh", 100.0f, 200.0f, { 100.0f }, 4.0, 0.0f);     // half pinned
    levelRows ("coh",     0.0f, 200.0f, { 50.0f, 65.0f, 80.0f, 100.0f }, 4.0);

    // ── thd ────────────────────────────────────────────────────────────────
    {
        const auto s = sine (kLen, 440.0, 0.5f);
        auto ps = base(); ps["grain_size"] = 200.0f; ps["density"] = 100.0f;
        auto p = makeInstance (ps);
        const auto out = render (*p, s);
        const double h1 = toneMag (out, kSteadyFrom, kLen, 440.0);
        const double h3 = toneMag (out, kSteadyFrom, kLen, 1320.0);
        row (toDb (h3 / h1) <= -60.0, "thd 440 Hz 0.5 pk N=20",
             juce::String::formatted ("H3/H1 %.1f dB", toDb (h3 / h1)));
    }

    // ── sparse null vs v3.1.0 ──────────────────────────────────────────────
    {
        const auto s = sine (kLen, 330.0, 0.02f);
        auto ps = base(); ps["grain_size"] = 50.0f; ps["density"] = 10.0f;   // ~1.6 g/s, N ~ 0.08
        ps["spread"] = 50.0f; ps["pitch_random"] = 50.0f; ps["pan_random"] = 60.0f; ps["reverse"] = 30.0f;
        auto p = makeInstance (ps);
        const auto out = render (*p, s);
        const juce::File dir = argc > 1 ? juce::File (argv[1]) : juce::File();
        const juce::File dump = dir.getChildFile ("sparse.f32");
       #ifdef GS_DENSITY_BASELINE
        if (argc > 1)
        {
            dir.createDirectory();
            juce::FileOutputStream os (dump);
            os.setPosition (0); os.truncate();
            for (int ch = 0; ch < 2; ++ch)
                os.write (out.getReadPointer (ch), sizeof (float) * static_cast<size_t> (kLen));
            std::printf ("  sparse render dumped to %s\n", dump.getFullPathName().toRawUTF8());
        }
       #else
        juce::MemoryBlock mb;
        if (argc > 1 && dump.loadFileAsData (mb) && mb.getSize() == sizeof (float) * 2 * static_cast<size_t> (kLen))
        {
            const auto* ref = static_cast<const float*> (mb.getData());
            double peak = 0.0, diff = 0.0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < kLen; ++i)
                {
                    const double a = out.getSample (ch, i), b = ref[ch * kLen + i];
                    peak = juce::jmax (peak, std::abs (b));
                    diff = juce::jmax (diff, std::abs (a - b));
                }
            row (peak > 1e-3 && toDb (diff / peak) <= -70.0, "sparse N~0.08 null vs comp-off",
                 juce::String::formatted ("residual %.1f dB re peak (ref peak %.4f)", toDb (diff / peak), peak));
        }
        else
            row (false, "sparse N~0.08 null vs comp-off", "no reference dump (run the -compoff target first)");
       #endif
    }

    // ── freeze ─────────────────────────────────────────────────────────────
    {
        const int len = static_cast<int> (9.0 * kSampleRate);
        const int freezeAt = 3 * 48000 / kBlock * kBlock;
        const auto nin = noise (len, static_cast<float> (inRms));
        auto ps = base(); ps["grain_size"] = 200.0f; ps["density"] = 90.0f; ps["spread"] = 0.0f;
        auto p = makeInstance (ps);
        const auto out = render (*p, nin, freezeAt, { { "freeze", 1.0f } });

        // The capture loop: 2 s + 1 samples, less the 10 ms seam crossfade.
        const int loop = static_cast<int> (kSampleRate * 2.0) + 1 - static_cast<int> (kSampleRate * 0.010);
        const int from = freezeAt + static_cast<int> (1.0 * kSampleRate);
        const int span = static_cast<int> (2.0 * kSampleRate);
        double best = 0.0;
        for (int lag = loop - 8; lag <= loop + 8; ++lag)
        {
            double xy = 0.0, xx = 0.0, yy = 0.0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = from; i < from + span; ++i)
                {
                    const double x = out.getSample (ch, i), y = out.getSample (ch, i + lag);
                    xy += x * y; xx += x * x; yy += y * y;
                }
            best = juce::jmax (best, xy / std::sqrt (xx * yy + 1e-30));
        }
        row (best <= 0.5, "freeze loop repetition (Spread 0)",
             juce::String::formatted ("max corr one loop apart %.3f", best));

        const double rel = toDb (rms (out, from, from + span) / (inRms * panLaw));
        row (std::abs (rel) <= 1.5, "freeze level N=12.6",
             juce::String::formatted ("%+6.2f dB re centre-pan input", rel));
    }

    // ── onset overshoot ────────────────────────────────────────────────────
    {
        auto ps = base(); ps["grain_size"] = 200.0f; ps["density"] = 100.0f;
        auto p = makeInstance (ps);
        const auto out = render (*p, in);
        const double steady = rms (out, kSteadyFrom, kLen);
        const int win = static_cast<int> (0.020 * kSampleRate);
        double worst = 0.0;
        for (int i = 0; i + win <= static_cast<int> (kSampleRate); i += win / 2)
            worst = juce::jmax (worst, rms (out, i, i + win));
        row (toDb (worst / steady) <= 6.0, "onset overshoot N=20",
             juce::String::formatted ("worst 20 ms %+.2f dB re steady", toDb (worst / steady)));
    }

   #ifdef GS_DENSITY_BASELINE
    std::printf ("\nbaseline done\n");
    return 0;
   #else
    std::printf ("\n%s (%d failing)\n", failures == 0 ? "ALL PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
   #endif
}
