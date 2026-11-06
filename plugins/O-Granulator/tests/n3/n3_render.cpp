/*
  ==============================================================================

    O-Granulator — N3 neutral-2.5b out-of-tree null driver
    (PLAN-2.5b T1 / P60, CONTEXT-2.5b gate)

    ONE source, compiled twice by tests/n3/run.sh: once against the verified
    2.5a tree (git archive 2cf30120) and once against the working tree. Both
    builds are processor consoles with OGRAN_TEST_HOOKS=1 (diag counters only —
    every hook stays at its default, i.e. off).

      n3_render <out.f32> <case> [variant]   render one case, raw float32 L/R
                                             interleaved; diag on stdout
      n3_render compare <a> <b>              max|Δ| and bitwise diff count

    The N2 recipe (2.3 FX on but non-peaking, 2.2 features as N0, 2.4 dials
    non-default but neutral, input_gain −12 dB → below the D9 knee, the 2.5a
    dials non-default but neutral) plus the 2.5b dials NON-DEFAULT but NEUTRAL:
      onset_threshold −24 dB; scale_root 7 (G); sN_onset_trigger Off ×4;
      pitch_follow_sN Off ×4; slot 12 Pitch → None (40 %), slot 13 Onset →
      None (−60 %). None of these is a route.
    Stimulus (both sides): the N2 hash noise × 0.1 (peak 0.05) + a 0.1 sine at
    227 Hz stepping phase-continuously to 301 Hz at 3 s + 2 ms 1 kHz Hann
    bursts at 0.35 every 250 ms from 0.5 s. Peak <= 0.5 (the N2 bound), 6 s.
    The detector fires and the MPM is confident in the neutral case, so the
    null proves that the analysis never reaches the audio path.
    A MockPlayHead (constant 120 bpm, 4/4, playing from ppq 0) on BOTH sides.
    Cases:
      free    each stream density 25 × 1000 ms (ΣN 100), grain_limit 128
      capped  each stream density 100 × 1000 ms (ΣN 400), grain_limit 32
    Variants:
      base    as above
      seed1   seed + 1                                        (N3c, new side only)
      THR     onset_threshold swept −60 → 0 dB per host block (N3p-THR; IDENTICAL)
      Z       slot 14 Pitch → s1_level, slot 15 Onset → mix, depth 0
                                                              (N3p-Z, R46; IDENTICAL)
      TRG     s1_onset_trigger On                             (N3n, flips at T3)
      ONS     slot 14 Onset → s1_level +1 %                   (N3n, flips at T4)
      PIT     slot 15 Pitch → s1_level +1 %                   (N3n, flips at T6)
      FOL     pitch_follow_s1 On (scale Off → D19 chromatic)  (N3n, flips at T8)
    The driver prints maxwet = max|out| over the render (mix 100 %, output 0 dB:
    the output IS the wet) and the analysis liveness counters
    onsets / hops / confident / f0writes through a C++20 `requires` probe on
    the engine Diag: the same source compiles on both sides at every task, and
    a field the tree does not have yet prints n/a.

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
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kFs = 48000.0;

    juce::RangedAudioParameter* param (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                if (rp->getParameterID() == id) return rp;
        return nullptr;
    }

    // Raw-value checksum (FNV-1a over the landed floats): both sides must land
    // on the SAME raw values (see tests/n2/n2_render.cpp).
    std::uint64_t rawSum = 0xcbf29ce484222325ull;
    int landed = 0;

    void hashRaw (float x)
    {
        std::uint32_t b; std::memcpy (&b, &x, 4);
        for (int k2 = 0; k2 < 4; ++k2) { rawSum ^= (b >> (8 * k2)) & 0xffu; rawSum *= 0x100000001b3ull; }
    }

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
                    hashRaw (raw->load());
                    ++landed;
                    return true;
                }
            }
            up = std::nextafter (up, 2.0f);
            dn = std::nextafter (dn, -1.0f);
        }
        rp->setValueNotifyingHost (n0);
        {
            const float x = raw->load();
            hashRaw (x);
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
            i.setTimeInSeconds ((double) pos / kFs);
            i.setBpm (120.0);
            i.setTimeSignature (TimeSignature { 4, 4 });
            const double ppq = (double) pos / kFs * 2.0;
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

    /** P60 composite: noise × 0.1 + 0.1 sine 227 → 301 Hz (phase-continuous at
        3 s) + 2 ms 1 kHz Hann bursts at 0.35 every 250 ms from 0.5 s. */
    float stimulus (juce::int64 i, int ch)
    {
        const double t = (double) i / kFs;
        const double tStep = 3.0;
        const double ph = t < tStep ? 2.0 * kPi * 227.0 * t
                                    : 2.0 * kPi * 227.0 * tStep + 2.0 * kPi * 301.0 * (t - tStep);
        double v = 0.1 * (double) hashNoise (i, ch) + 0.1 * std::sin (ph);
        if (t >= 0.5)
        {
            const double k  = std::floor ((t - 0.5) / 0.25);
            const double tb = t - (0.5 + 0.25 * k);
            if (tb >= 0.0 && tb < 0.002)
                v += 0.35 * 0.5 * (1.0 - std::cos (2.0 * kPi * tb / 0.002)) * std::sin (2.0 * kPi * 1000.0 * tb);
        }
        return (float) v;
    }

    // ── Analysis liveness (P60): C++20 requires-probes on the engine Diag ────
    // A field the tree under test does not have prints n/a (−1).
    template <typename D> long long probeOnsets (const D& d)    { if constexpr (requires { d.onsetEvents; })  return (long long) d.onsetEvents;  else return -1; }
    template <typename D> long long probeHops (const D& d)      { if constexpr (requires { d.mpmHops; })      return (long long) d.mpmHops;      else return -1; }
    template <typename D> long long probeConfident (const D& d) { if constexpr (requires { d.mpmConfident; }) return (long long) d.mpmConfident; else return -1; }
    template <typename D> long long probeF0Writes (const D& d)  { if constexpr (requires { d.f0Writes; })     return (long long) d.f0Writes;     else return -1; }
    juce::String na (long long v) { return v < 0 ? juce::String ("n/a") : juce::String (v); }

    int render (const char* outPath, const juce::String& which, const juce::String& variant)
    {
        const bool capped = which == "capped";
        if (! capped && which != "free") { std::fprintf (stderr, "unknown case %s\n", which.toRawUTF8()); return 2; }
        const juce::StringArray known { "base", "seed1", "THR", "Z", "TRG", "ONS", "PIT", "FOL" };
        if (! known.contains (variant))
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

        // 2.5a: non-default, neutral (no live route anywhere) — the N2 dials.
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

        auto route = [&ps] (int m, float src, float dst, float dep)
        {
            const std::string k = "mod" + std::to_string (m) + "_";
            ps.push_back ({ k + "source", src });
            ps.push_back ({ k + "dest", dst });
            ps.push_back ({ k + "depth", dep });
        };

        // 2.5b: non-default, neutral (P60). None is a route.
        ps.push_back ({ "onset_threshold", -24.0f });
        ps.push_back ({ "scale_root", 7.0f });   // G
        for (int s = 1; s <= 4; ++s)
        {
            ps.push_back ({ "s" + std::to_string (s) + "_onset_trigger", 0.0f });
            ps.push_back ({ "pitch_follow_s" + std::to_string (s), 0.0f });
        }
        route (12, 6.0f, 0.0f, 40.0f);    // Pitch → None
        route (13, 7.0f, 0.0f, -60.0f);   // Onset → None

        // Per-element controls (applied after the base, so they override it).
        if (variant == "Z")   { route (14, 6.0f, 5.0f, 0.0f); route (15, 7.0f, 2.0f, 0.0f); }   // Pitch → s1_level, Onset → mix, depth 0
        if (variant == "TRG") ps.push_back ({ "s1_onset_trigger", 1.0f });
        if (variant == "ONS") route (14, 7.0f, 5.0f, 1.0f);                                      // Onset → s1_level +1 %
        if (variant == "PIT") route (15, 6.0f, 5.0f, 1.0f);                                      // Pitch → s1_level +1 %
        if (variant == "FOL") ps.push_back ({ "pitch_follow_s1", 1.0f });

        int ok = 0;
        for (auto& kv : ps) ok += setP (*p, kv.first, kv.second) ? 1 : 0;
        p->setSeed (0x12A12A5EEDull + (variant == "seed1" ? 1ull : 0ull));
        ConstPlayHead ph;
        p->setPlayHead (&ph);
        p->prepareToPlay (kFs, 512);

        // N3p-THR: onset_threshold −60 → 0 dB, one value per host block (both sides).
        auto* thr = param (*p, "onset_threshold");

        const juce::int64 len = 6 * (juce::int64) kFs;
        std::vector<float> out ((size_t) (2 * len));
        juce::AudioBuffer<float> blk (2, 512);
        juce::MidiBuffer midi;
        double peakIn = 0.0;
        for (juce::int64 pos = 0; pos < len; pos += 512)
        {
            const int n = (int) juce::jmin ((juce::int64) 512, len - pos);
            ph.pos = pos;
            if (variant == "THR" && thr != nullptr)
                thr->setValueNotifyingHost (thr->convertTo0to1 ((float) (-60.0 + 60.0 * (double) pos / (double) len)));
            blk.setSize (2, n, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n; ++i)
                {
                    const float x = stimulus (pos + i, ch);
                    peakIn = juce::jmax (peakIn, (double) std::abs (x));
                    blk.setSample (ch, i, x);
                }
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
                     "wetdb %.2f maxwet %.6f peakin %.6f exact %d/%d landed %d raw %016llx live onsets %s hops %s confident %s f0writes %s\n",
                     which.toRawUTF8(), variant.toRawUTF8(), (long long) d.spawns, (long long) d.steals, (long long) d.kills,
                     (long long) d.maxAlive, (long long) d.planCalls[0], (long long) d.planCalls[1],
                     (long long) d.planCalls[2], (long long) d.planCalls[3], db, maxWet, peakIn, ok, (int) ps.size(), landed,
                     (unsigned long long) rawSum, na (probeOnsets (d)).toRawUTF8(), na (probeHops (d)).toRawUTF8(),
                     na (probeConfident (d)).toRawUTF8(), na (probeF0Writes (d)).toRawUTF8());

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
    std::fprintf (stderr, "usage: n3_render <out.f32> free|capped [base|seed1|THR|Z|TRG|ONS|PIT|FOL]  |  n3_render compare <a.f32> <b.f32>\n");
    return 2;
}
