/*
  ==============================================================================

    O-Granulator — N0 neutral-FX out-of-tree null driver
    (PLAN-2.3 T1 / P17, RESEARCH-2.3 §7)

    ONE source, compiled twice by tests/n0/run.sh: once against the verified
    2.2 tree (git archive c10d0f2c) and once against the working tree. Both
    builds are processor consoles with OGRAN_TEST_HOOKS=1 (diag counters only —
    every hook stays at its default, i.e. off).

      n0_render <out.f32> <case> [variant]   render one case, raw float32 L/R
                                             interleaved; diag on stdout
      n0_render compare <a> <b>              max|Δ| and bitwise diff count

    Every stream enabled, every FX dial NON-DEFAULT but NEUTRAL:
      filter_type Off with cutoff 1000, res 90, rand 100;  drive 0;
      crush 0 with crush_prob 37;  ring_mix 0 with ring_freq 3000, ring_rand 100.
    2.2 features on (Hann AND bank window paths):
      s1 spray 10 %, jitter 20 %, window 0, spread 0
      s2 window 0.6, skew −40, spread 70
      s3 scale 1 (Chromatic), pitch_rand 3
      s4 reverse_prob 50, gain_rand 50
    Cases:
      free    each stream density 25 × 1000 ms (ΣN 100), grain_limit 128
      capped  each stream density 100 × 1000 ms (ΣN 400), grain_limit 32
    Variants (new side only; compared with the old side's free render):
      base    as above
      seed1   seed + 1                              (N0c, must differ)
      F       s1 LP, cutoff 20000 (→ 0.45·fs), res 0, rand 0   (N0n-F)
      D       s1 drive 1 %                          (N0n-D)
      C       s1 crush 1 %                          (N0n-C)
      R       s1 ring_mix 1 %                       (N0n-R)

    Parameters are set BY ID through the host-normalised path (the harness
    setP search: land exactly on the raw value). All 220 IDs exist in both
    trees (Stage 1).

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
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

    // Raw-value checksum (FNV-1a over the landed floats): both sides must land
    // on the SAME raw values. A skewed range cannot reach every value exactly
    // through the normalised path (density 25 → 24.9999962), so "landed" is
    // |raw − v| <= 1e-5 · span, and identity is proved by the checksum.
    std::uint64_t rawSum = 0xcbf29ce484222325ull;
    int landed = 0;

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
                if (juce::exactlyEqual (raw->load(), v))
                {
                    std::uint32_t b; const float x = raw->load(); std::memcpy (&b, &x, 4);
                    for (int k2 = 0; k2 < 4; ++k2) { rawSum ^= (b >> (8 * k2)) & 0xffu; rawSum *= 0x100000001b3ull; }
                    ++landed;
                    return true;
                }
            }
            up = std::nextafter (up, 2.0f);
            dn = std::nextafter (dn, -1.0f);
        }
        rp->setValueNotifyingHost (n0);
        {
            std::uint32_t b; const float x = raw->load(); std::memcpy (&b, &x, 4);
            for (int k2 = 0; k2 < 4; ++k2) { rawSum ^= (b >> (8 * k2)) & 0xffu; rawSum *= 0x100000001b3ull; }
            const auto r = rp->getNormalisableRange();
            if (std::abs (x - v) <= 1.0e-5f * (r.end - r.start)) ++landed;
        }
        std::fprintf (stderr, "  inexact %s = %.9g (raw %.9g)\n", id.toRawUTF8(), (double) v, (double) raw->load());
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

    int render (const char* outPath, const juce::String& which, const juce::String& variant)
    {
        const bool capped = which == "capped";
        if (! capped && which != "free") { std::fprintf (stderr, "unknown case %s\n", which.toRawUTF8()); return 2; }
        if (variant != "base" && variant != "seed1" && variant != "F" && variant != "D" && variant != "C" && variant != "R")
        {
            std::fprintf (stderr, "unknown variant %s\n", variant.toRawUTF8());
            return 2;
        }

        std::unique_ptr<GranulatorProcessor> p (dynamic_cast<GranulatorProcessor*> (createPluginFilter()));
        std::vector<std::pair<std::string, float>> ps {
            { "mix", 100.0f },
            { "grain_limit", capped ? 0.0f /* 32 */ : 3.0f /* 128 */ },
        };
        for (int s = 1; s <= 4; ++s)
        {
            const std::string k = "s" + std::to_string (s) + "_";
            ps.push_back ({ k + "enable", 1.0f });
            ps.push_back ({ k + "density", capped ? 100.0f : 25.0f });
            ps.push_back ({ k + "size", 1000.0f });
            // FX dials: non-default, neutral.
            ps.push_back ({ k + "filter_type", 0.0f });
            ps.push_back ({ k + "filter_cutoff", 1000.0f });
            ps.push_back ({ k + "filter_res", 90.0f });
            ps.push_back ({ k + "filter_rand", 100.0f });
            ps.push_back ({ k + "drive", 0.0f });
            ps.push_back ({ k + "crush", 0.0f });
            ps.push_back ({ k + "crush_prob", 37.0f });
            ps.push_back ({ k + "ring_mix", 0.0f });
            ps.push_back ({ k + "ring_freq", 3000.0f });
            ps.push_back ({ k + "ring_rand", 100.0f });
        }
        // 2.2 features (Hann and bank paths both covered).
        for (const auto& kv : std::vector<std::pair<std::string, float>> {
                 { "s1_spray", 10.0f }, { "s1_jitter", 20.0f }, { "s1_window", 0.0f }, { "s1_stereo_spread", 0.0f },
                 { "s2_window", 0.6f }, { "s2_window_skew", -40.0f }, { "s2_stereo_spread", 70.0f },
                 { "s3_scale", 1.0f }, { "s3_pitch_rand", 3.0f },
                 { "s4_reverse_prob", 50.0f }, { "s4_gain_rand", 50.0f } })
            ps.push_back (kv);

        // Per-stage controls (s1 only).
        if (variant == "F")
            for (const auto& kv : std::vector<std::pair<std::string, float>> {
                     { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 20000.0f }, { "s1_filter_res", 0.0f }, { "s1_filter_rand", 0.0f } })
                ps.push_back (kv);
        if (variant == "D") ps.push_back ({ "s1_drive", 1.0f });
        if (variant == "C") ps.push_back ({ "s1_crush", 1.0f });
        if (variant == "R") ps.push_back ({ "s1_ring_mix", 1.0f });

        int ok = 0;
        for (auto& kv : ps) ok += setP (*p, kv.first, kv.second) ? 1 : 0;
        p->setSeed (0x12A12A5EEDull + (variant == "seed1" ? 1ull : 0ull));
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

        const auto& d = p->getEngineForTest().diag;
        std::printf ("diag %s/%s spawns %lld steals %lld kills %lld maxAlive %lld plan1 %lld plan2 %lld plan3 %lld plan4 %lld "
                     "wetdb %.2f exact %d/%d landed %d raw %016llx\n",
                     which.toRawUTF8(), variant.toRawUTF8(), (long long) d.spawns, (long long) d.steals, (long long) d.kills,
                     (long long) d.maxAlive, (long long) d.planCalls[0], (long long) d.planCalls[1],
                     (long long) d.planCalls[2], (long long) d.planCalls[3], db, ok, (int) ps.size(), landed, (unsigned long long) rawSum);

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
        return (bits == 0 && peak > 1e-3) ? 0 : 1;
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc == 4 && std::strcmp (argv[1], "compare") == 0) return compare (argv[2], argv[3]);
    if (argc == 3) return render (argv[1], argv[2], "base");
    if (argc == 4) return render (argv[1], argv[2], argv[3]);
    std::fprintf (stderr, "usage: n0_render <out.f32> free|capped [base|seed1|F|D|C|R]  |  n0_render compare <a.f32> <b.f32>\n");
    return 2;
}
