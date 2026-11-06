/*
  ==============================================================================

    O-Granulator — N2 neutral-2.5a out-of-tree null driver
    (PLAN-2.5a T1 / P46, CONTEXT-2.5a gate 1)

    ONE source, compiled twice by tests/n2/run.sh: once against the verified
    2.4 tree (git archive 42d30b01) and once against the working tree. Both
    builds are processor consoles with OGRAN_TEST_HOOKS=1 (diag counters only —
    every hook stays at its default, i.e. off).

      n2_render <out.f32> <case> [variant]   render one case, raw float32 L/R
                                             interleaved; diag on stdout
      n2_render compare <a> <b>              max|Δ| and bitwise diff count

    The N1 recipe (2.3 FX on but non-peaking, 2.2 features as N0, 2.4 dials
    non-default but neutral, input_gain −12 dB → below the D9 knee) plus the
    2.5a dials NON-DEFAULT but NEUTRAL (none is a route):
      sync_mode Free; sN_size_sync / sN_density_sync non-default (inert in Free);
      LFO 1–4 non-default shape / rate / rate_sync / phase; env attack /
      release / gain non-default; macros 1–4 at 25 / 50 / 75 / 100 %;
      slots 1–4 source set, dest None; slots 5–8 source None, dest set;
      slots 9–16 None / None; every depth non-zero.
    A MockPlayHead (constant 120 bpm, 4/4, playing from ppq 0) is attached on
    BOTH sides (the 2.4 tree never reads it).
    Cases:
      free    each stream density 25 × 1000 ms (ΣN 100), grain_limit 128
      capped  each stream density 100 × 1000 ms (ΣN 400), grain_limit 32
    Variants:
      base    as above
      seed1   seed + 1                                   (N2c, new side only)
      P       slot 1 LFO1 → s1_level depth 0, slot 2 Env → mix depth 0
              (N2p, R46: offset exactly 0 → raw path; IDENTICAL always)
      TMP     sync_mode Tempo                            (T5)
      LFO     slot 9 LFO1 → s1_level depth 1 %           (T8)
      MAC     slot 10 Macro 1 (50 %) → mix depth 1 %     (T8)
      ENV     slot 11 Env → s1_pitch depth 1 %           (T9, spawn-latched)
    The driver prints maxwet = max|out| over the render: at mix 100 % and
    output 0 dB the output IS the wet (dg = 0, wg = og = 1).

    Parameters are set BY ID through the host-normalised path (the harness
    setP search: land exactly on the raw value).

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

    /** Constant 120 bpm, 4/4, playing from ppq 0 (the legacy formula). */
    struct ConstPlayHead final : juce::AudioPlayHead
    {
        juce::int64 pos = 0;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo i;
            i.setTimeInSamples (pos);
            i.setTimeInSeconds ((double) pos / 48000.0);
            i.setBpm (120.0);
            i.setTimeSignature (TimeSignature { 4, 4 });
            const double ppq = (double) pos / 48000.0 * 2.0;
            i.setPpqPosition (ppq);
            i.setPpqPositionOfLastBarStart (std::floor (ppq / 4.0) * 4.0);
            i.setIsPlaying (true);
            return i;
        }
    };

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
        if (variant != "base" && variant != "seed1" && variant != "P" && variant != "TMP" && variant != "LFO"
            && variant != "MAC" && variant != "ENV")
        {
            std::fprintf (stderr, "unknown variant %s\n", variant.toRawUTF8());
            return 2;
        }

        std::unique_ptr<GranulatorProcessor> p (dynamic_cast<GranulatorProcessor*> (createPluginFilter()));
        std::vector<std::pair<std::string, float>> ps {
            { "mix", 100.0f },
            { "input_gain", -12.0f },   // PLAN-2.4 T1: both sides, keeps max|wet| below the D9 knee (T - 1 dB)
            { "grain_limit", capped ? 0.0f /* 32 */ : 3.0f /* 128 */ },
        };
        for (int s = 1; s <= 4; ++s)
        {
            const std::string k = "s" + std::to_string (s) + "_";
            ps.push_back ({ k + "enable", 1.0f });
            ps.push_back ({ k + "density", capped ? 100.0f : 25.0f });
            ps.push_back ({ k + "size", 1000.0f });
        }
        // 2.3 FX on, non-peaking; 2.2 features as N0.
        for (const auto& kv : std::vector<std::pair<std::string, float>> {
                 { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 1500.0f }, { "s1_filter_res", 20.0f }, { "s1_filter_rand", 50.0f },
                 { "s2_filter_type", 2.0f }, { "s2_filter_cutoff", 2000.0f }, { "s2_filter_res", 80.0f },
                 { "s3_drive", 30.0f }, { "s3_crush", 40.0f }, { "s3_crush_prob", 50.0f },
                 { "s4_ring_freq", 300.0f }, { "s4_ring_mix", 50.0f }, { "s4_ring_rand", 50.0f },
                 { "s1_spray", 10.0f }, { "s1_jitter", 20.0f }, { "s1_window", 0.0f }, { "s1_stereo_spread", 0.0f },
                 { "s2_window", 0.6f }, { "s2_window_skew", -40.0f }, { "s2_stereo_spread", 70.0f },
                 { "s3_scale", 1.0f }, { "s3_pitch_rand", 3.0f },
                 { "s4_reverse_prob", 50.0f }, { "s4_gain_rand", 50.0f },
                 // 2.4: non-default, neutral.
                 { "feedback", 0.0f }, { "fb_low_cut", 400.0f }, { "fb_high_cut", 3000.0f },
                 { "fb_saturation", 80.0f }, { "fb_diffusion", 70.0f },
                 { "freeze", 0.0f }, { "reverb_mix", 0.0f }, { "reverb_size", 90.0f }, { "reverb_damp", 10.0f } })
            ps.push_back (kv);

        // 2.5a: non-default, neutral (no live route anywhere).
        ps.push_back ({ "sync_mode", 0.0f });
        {
            const float sizeSync[4] { 10.0f, 3.0f, 15.0f, 19.0f }, densSync[4] { 4.0f, 9.0f, 12.0f, 18.0f };
            const float shape[4] { 1.0f, 2.0f, 4.0f, 5.0f }, rate[4] { 2.0f, 0.3f, 7.0f, 0.05f };
            const float rateSync[4] { 3.0f, 10.0f, 15.0f, 21.0f }, phase[4] { 45.0f, 90.0f, 180.0f, 270.0f };
            for (int n = 1; n <= 4; ++n)
            {
                const std::string s = "s" + std::to_string (n) + "_", l = "lfo" + std::to_string (n) + "_";
                ps.push_back ({ s + "size_sync", sizeSync[n - 1] });
                ps.push_back ({ s + "density_sync", densSync[n - 1] });
                ps.push_back ({ l + "shape", shape[n - 1] });
                ps.push_back ({ l + "rate", rate[n - 1] });
                ps.push_back ({ l + "rate_sync", rateSync[n - 1] });
                ps.push_back ({ l + "phase", phase[n - 1] });
                ps.push_back ({ "macro" + std::to_string (n), 25.0f * (float) n });
            }
            ps.push_back ({ "env_attack", 30.0f });
            ps.push_back ({ "env_release", 500.0f });
            ps.push_back ({ "env_gain", 6.0f });
            // Slots 1–4: source, dest None. 5–8: source None, dest. 9–16: None / None. Depth ≠ 0 everywhere.
            const float src14[4] { 1.0f, 5.0f, 8.0f, 4.0f }, dep14[4] { 50.0f, -30.0f, 70.0f, -100.0f };
            const float dst58[4] { 2.0f, 13.0f, 101.0f, 106.0f }, dep58[4] { 40.0f, -60.0f, 20.0f, 90.0f };
            for (int m = 1; m <= 16; ++m)
            {
                const std::string k = "mod" + std::to_string (m) + "_";
                const float src = m <= 4 ? src14[m - 1] : 0.0f;
                const float dst = (m >= 5 && m <= 8) ? dst58[m - 5] : 0.0f;
                const float dep = m <= 4 ? dep14[m - 1] : (m <= 8 ? dep58[m - 5] : (m % 2 == 0 ? 1.0f : -1.0f) * 10.0f * (float) (m - 8));
                ps.push_back ({ k + "source", src });
                ps.push_back ({ k + "dest", dst });
                ps.push_back ({ k + "depth", dep });
            }
        }

        // Per-element controls (applied after the base, so they override it).
        auto route = [&ps] (int m, float src, float dst, float dep)
        {
            const std::string k = "mod" + std::to_string (m) + "_";
            ps.push_back ({ k + "source", src });
            ps.push_back ({ k + "dest", dst });
            ps.push_back ({ k + "depth", dep });
        };
        if (variant == "P")   { route (1, 1.0f, 5.0f, 0.0f); route (2, 5.0f, 2.0f, 0.0f); }   // LFO1 → s1_level, Env → mix, depth 0
        if (variant == "TMP") ps.push_back ({ "sync_mode", 1.0f });
        if (variant == "LFO") route (9, 1.0f, 5.0f, 1.0f);                                      // LFO1 → s1_level 1 %
        if (variant == "MAC") { ps.push_back ({ "macro1", 50.0f }); route (10, 8.0f, 2.0f, 1.0f); }   // Macro 1 → mix 1 %
        if (variant == "ENV") route (11, 5.0f, 13.0f, 1.0f);                                    // Env → s1_pitch 1 %

        int ok = 0;
        for (auto& kv : ps) ok += setP (*p, kv.first, kv.second) ? 1 : 0;
        p->setSeed (0x12A12A5EEDull + (variant == "seed1" ? 1ull : 0ull));
        ConstPlayHead ph;
        p->setPlayHead (&ph);
        p->prepareToPlay (48000.0, 512);

        const juce::int64 len = 6 * 48000;
        std::vector<float> out ((size_t) (2 * len));
        juce::AudioBuffer<float> blk (2, 512);
        juce::MidiBuffer midi;
        for (juce::int64 pos = 0; pos < len; pos += 512)
        {
            const int n = (int) juce::jmin ((juce::int64) 512, len - pos);
            ph.pos = pos;
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
        p->setPlayHead (nullptr);
        double maxWet = 0.0;
        for (float v : out) maxWet = juce::jmax (maxWet, (double) std::abs (v));
        double e = 0.0;
        for (size_t i = (size_t) 2 * 48000; i < out.size(); ++i) e += (double) out[i] * out[i];
        const double db = 10.0 * std::log10 (juce::jmax (1e-300, e / (double) (out.size() - 2 * 48000)));

        const auto& d = p->getEngineForTest().diag;
        std::printf ("diag %s/%s spawns %lld steals %lld kills %lld maxAlive %lld plan1 %lld plan2 %lld plan3 %lld plan4 %lld "
                     "wetdb %.2f maxwet %.6f exact %d/%d landed %d raw %016llx\n",
                     which.toRawUTF8(), variant.toRawUTF8(), (long long) d.spawns, (long long) d.steals, (long long) d.kills,
                     (long long) d.maxAlive, (long long) d.planCalls[0], (long long) d.planCalls[1],
                     (long long) d.planCalls[2], (long long) d.planCalls[3], db, maxWet, ok, (int) ps.size(), landed, (unsigned long long) rawSum);

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
    std::fprintf (stderr, "usage: n2_render <out.f32> free|capped [base|seed1|P|TMP|LFO|MAC|ENV]  |  n2_render compare <a.f32> <b.f32>\n");
    return 2;
}
