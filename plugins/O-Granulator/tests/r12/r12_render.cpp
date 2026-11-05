/*
  ==============================================================================

    O-Granulator — R12 2.1-equivalence out-of-tree null driver
    (PLAN-2.2 T16 / RESEARCH-2.2 §7 R12)

    ONE source, compiled twice by tests/r12/run.sh: once against the verified
    2.1 tree (git archive 6508c635) and once against the working tree. Both
    builds are processor consoles (ouaricon_add_processor_console) with the
    same flags and OGRAN_TEST_HOOKS=1 (for the steal / kill counters only —
    every hook stays at its default, i.e. off).

      r12_render <out.f32> <case>      render one case, raw float32 L/R
                                       interleaved; prints diag on stderr
      r12_render compare <a> <b>       max|Δ| and bitwise diff count

    Cases (s1 only, spray 10 % → c = 0, window 0, skew 0, spread 0,
    gain_rand 0, scale Off, jitter 20 %, static params, seed fixed, mix 100 %):
      free    N 100 (density 100 × 1000 ms), grain_limit 128   (steal-free)
      capped  N 400 (density 200 × 2000 ms), grain_limit 32    (R3 kills)

    Parameters are set BY ID through the host-normalised path (the harness
    setP search: land exactly on the raw value). An ID absent on one side is
    skipped and reported — every ID used here exists in both trees (the 220
    parameters are Stage 1), the 2.2-only ones are simply inert in 2.1.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    juce::RangedAudioParameter* param (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                if (rp->getParameterID() == id) return rp;
        return nullptr;
    }

    // Same search as the render harness eg::setP: land EXACTLY on v through the
    // normalised path (skewed ranges: a few ulps around convertTo0to1 (v)).
    bool setP (GranulatorProcessor& p, const juce::String& id, float v)
    {
        auto* rp = param (p, id);
        if (rp == nullptr) { std::fprintf (stderr, "  skip %s (absent)\n", id.toRawUTF8()); return false; }
        auto* raw = p.parameters.getRawParameterValue (id);
        const float n0 = rp->convertTo0to1 (v);
        float up = n0, dn = n0;
        for (int k = 0; k < 2048; ++k)
        {
            for (float c : { up, dn })
            {
                rp->setValueNotifyingHost (c);
                if (juce::exactlyEqual (raw->load(), v)) return true;
            }
            up = std::nextafter (up, 2.0f);
            dn = std::nextafter (dn, -1.0f);
        }
        rp->setValueNotifyingHost (n0);
        std::fprintf (stderr, "  inexact %s = %g (raw %g)\n", id.toRawUTF8(), (double) v, (double) raw->load());
        return false;
    }

    float hashNoise (juce::int64 i, int ch)
    {
        std::uint64_t x = static_cast<std::uint64_t> (i) * 0x9E3779B97F4A7C15ull + (ch != 0 ? 0xD1B54A32D192ED03ull : 0x2545F4914F6CDD1Dull);
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ull;
        x ^= x >> 27; x *= 0x94D049BB133111EBull;
        x ^= x >> 31;
        const double u = static_cast<double> (x >> 40) / static_cast<double> (1ull << 24);
        return static_cast<float> ((2.0 * u - 1.0) * 0.5);
    }

    int render (const char* outPath, const juce::String& which)
    {
        const bool capped = which == "capped";
        if (! capped && which != "free") { std::fprintf (stderr, "unknown case %s\n", which.toRawUTF8()); return 2; }

        std::unique_ptr<GranulatorProcessor> p (dynamic_cast<GranulatorProcessor*> (createPluginFilter()));
        const std::vector<std::pair<const char*, float>> ps {
            { "mix", 100.0f },
            { "s1_enable", 1.0f }, { "s2_enable", 0.0f }, { "s3_enable", 0.0f }, { "s4_enable", 0.0f },
            { "s1_spray", 10.0f }, { "s1_jitter", 20.0f },
            { "s1_window", 0.0f }, { "s1_window_skew", 0.0f },
            { "s1_stereo_spread", 0.0f }, { "s1_gain_rand", 0.0f }, { "s1_scale", 0.0f },
            { "s1_density", capped ? 200.0f : 100.0f }, { "s1_size", capped ? 2000.0f : 1000.0f },
            { "grain_limit", capped ? 0.0f /* 32 */ : 3.0f /* 128 */ },
        };
        int ok = 0;
        for (auto& kv : ps) ok += setP (*p, kv.first, kv.second) ? 1 : 0;
        p->setSeed (0x12A12A5EEDull);
        p->prepareToPlay (48000.0, 512);

        const juce::int64 len = 6 * 48000;
        std::vector<float> out ((size_t) (2 * len));
        juce::AudioBuffer<float> blk (2, 512);
        juce::MidiBuffer midi;
        for (juce::int64 pos = 0; pos < len; pos += 512)
        {
            const int n = (int) juce::jmin ((juce::int64) 512, len - pos);
            blk.setSize (2, n, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n; ++i) blk.setSample (ch, i, hashNoise (pos + i, ch));
            p->processBlock (blk, midi);
            for (int i = 0; i < n; ++i)
            {
                out[(size_t) (2 * (pos + i))]     = blk.getSample (0, i);
                out[(size_t) (2 * (pos + i) + 1)] = blk.getSample (1, i);
            }
        }
        double e = 0.0;
        for (size_t i = (size_t) 2 * 48000; i < out.size(); ++i) e += (double) out[i] * out[i];
        const double db = 10.0 * std::log10 (juce::jmax (1e-300, e / (double) (out.size() - 2 * 48000)));

       #if OGRAN_TEST_HOOKS
        const auto& d = p->getEngineForTest().diag;
        std::fprintf (stderr, "  %s: %d/%d params exact; spawns %lld steals %lld kills %lld maxAlive %lld; wet %.2f dBFS\n",
                      which.toRawUTF8(), ok, (int) ps.size(), (long long) d.spawns, (long long) d.steals,
                      (long long) d.kills, (long long) d.maxAlive, db);
        std::printf ("diag %s spawns %lld steals %lld kills %lld maxAlive %lld wetdb %.2f exact %d/%d\n", which.toRawUTF8(),
                     (long long) d.spawns, (long long) d.steals, (long long) d.kills, (long long) d.maxAlive, db, ok, (int) ps.size());
       #else
        std::printf ("diag %s wetdb %.2f exact %d/%d\n", which.toRawUTF8(), db, ok, (int) ps.size());
       #endif

        juce::File f (juce::File::getCurrentWorkingDirectory().getChildFile (outPath));
        f.deleteFile();
        juce::FileOutputStream os (f);
        if (! os.openedOk()) return 3;
        os.write (out.data(), out.size() * sizeof (float));
        return 0;
    }

    int compare (const char* a, const char* b)
    {
        juce::MemoryBlock ma, mb;
        const auto cwd = juce::File::getCurrentWorkingDirectory();
        if (! cwd.getChildFile (a).loadFileAsData (ma) || ! cwd.getChildFile (b).loadFileAsData (mb)) return 3;
        if (ma.getSize() != mb.getSize() || ma.getSize() == 0) { std::printf ("size mismatch %zu vs %zu\n", ma.getSize(), mb.getSize()); return 1; }
        const auto* x = static_cast<const float*> (ma.getData());
        const auto* y = static_cast<const float*> (mb.getData());
        const size_t n = ma.getSize() / sizeof (float);
        double mx = 0.0, peak = 0.0;
        size_t bits = 0, first = n;
        for (size_t i = 0; i < n; ++i)
        {
            const double dv = std::abs ((double) x[i] - (double) y[i]);
            if (dv > mx) mx = dv;
            peak = juce::jmax (peak, std::abs ((double) x[i]));
            if (std::memcmp (x + i, y + i, sizeof (float)) != 0) { ++bits; if (first == n) first = i; }
        }
        std::printf ("compare: %zu samples, max|d| %.9g, bitwise diffs %zu (first frame %lld), peak %.4f\n", n, mx, bits,
                     first == n ? -1LL : (long long) (first / 2), peak);
        return (mx == 0.0 && peak > 1e-3) ? 0 : 1;
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc == 4 && std::strcmp (argv[1], "compare") == 0) return compare (argv[2], argv[3]);
    if (argc == 3) return render (argv[1], argv[2]);
    std::fprintf (stderr, "usage: r12_render <out.f32> free|capped  |  r12_render compare <a.f32> <b.f32>\n");
    return 2;
}
