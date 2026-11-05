/*
  ==============================================================================

    O-Granulator — render harness, sections N6 / Z / K2 / M3 / C3 (Stage 2 /
    Phase 2.3, PLAN-2.3 T4, T11, T12). Sibling of engine_gates_23.h.

      N6  PERF-03 block-size null: the N5 everything-on recipe (4 streams,
          cap 32, toggles + custom swap at shared boundaries) PLUS per-grain FX
          on every stream (T4 SVF form: LP / BP / HP / LP, 1.5 kHz, res 60 %,
          filter_rand 50 %; s1 LP → HP at a shared boundary). 441 / 512 / 4096
          / var vs 32 max|Δ| == 0. N6c different seed differs.

  ==============================================================================
*/
#pragma once

namespace { namespace eg23b
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg22::sp;
    constexpr double kFs = 48000.0;

    //==========================================================================
    // N6 — block-size null with FX (the N5 base + FX)
    //==========================================================================
    Job n6Base (bool allFx)
    {
        Job base; base.len = (juce::int64) (6.0 * kFs); base.prepBlock = 512;
        base.params = { { "mix", 70.0f }, { "buffer_length", 4.0f }, { "grain_limit", 0.0f } };
        const float dens[4] { 100, 60, 40, 30 }, size[4] { 500, 300, 800, 200 }, scale[4] { 3, 31, 0, 24 }, prand[4] { 12, 7, 5, 9 };
        const float ftype[4] { 1, 2, 3, 1 };
        for (int s = 1; s <= 4; ++s)
        {
            for (auto& kv : P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), dens[s - 1] }, { sp (s, "size"), size[s - 1] },
                                { sp (s, "size_rand"), 50.0f }, { sp (s, "spray"), 50.0f }, { sp (s, "jitter"), 50.0f },
                                { sp (s, "position"), 30.0f }, { sp (s, "pitch_rand"), prand[s - 1] }, { sp (s, "reverse_prob"), 50.0f },
                                { sp (s, "window"), 0.6f }, { sp (s, "window_skew"), -40.0f }, { sp (s, "stereo_spread"), 70.0f },
                                { sp (s, "gain_rand"), 50.0f }, { sp (s, "scale"), scale[s - 1] }, { sp (s, "pan"), -60.0f + 40.0f * s },
                                { sp (s, "filter_type"), allFx && s == 4 ? 0.0f : ftype[s - 1] }, { sp (s, "filter_cutoff"), 1500.0f },
                                { sp (s, "filter_res"), 60.0f }, { sp (s, "filter_rand"), 50.0f } })
                base.params.push_back (kv);
            if (allFx)
                for (auto& kv : P { { sp (s, "drive"), 50.0f }, { sp (s, "crush"), 40.0f }, { sp (s, "crush_prob"), 50.0f },
                                    { sp (s, "ring_freq"), 300.0f }, { sp (s, "ring_rand"), 50.0f }, { sp (s, "ring_mix"), 50.0f } })
                    base.params.push_back (kv);
        }
        base.params.push_back ({ "s3_pitch", 5.0f });
        base.pre = [] (Proc& p)
        {
            juce::String bad; p.setCustomScale ("n5a", "edo 7", bad);
            p.setDegreeMask (1, 3, 0b10110101ull);
            p.setDegreeMask (4, 24, 0b1011011ull);
        };
        auto ev = [&base] (juce::int64 at, std::function<void (Proc&)> f) { base.events.push_back ({ at, std::move (f) }); };
        ev (100003, [] (Proc& p) { eg::setP (p, "s1_pitch", 5.0f); eg::setP (p, "mix", 40.0f); eg::setP (p, "s1_level", -4.0f); eg::setP (p, "s2_pan", 40.0f); });
        ev (120001, [] (Proc& p) { eg::setP (p, "s2_mute", 1.0f); });
        ev (140009, [] (Proc& p) { eg::setP (p, "s3_solo", 1.0f); });
        ev (150011, [] (Proc& p) { eg::setP (p, "s4_enable", 0.0f); });
        ev (160007, [] (Proc& p) { eg::setP (p, "s3_solo", 0.0f); });
        ev (170017, [] (Proc& p) { eg::setP (p, "s2_mute", 0.0f); });
        ev (190013, [] (Proc& p) { eg::setP (p, "s4_enable", 1.0f); });
        ev (200003, [] (Proc& p) { juce::String bad; p.setCustomScale ("n5b", "edo 5", bad); });
        ev (210001, [] (Proc& p) { eg::setP (p, "s1_window", 0.2f); eg::setP (p, "s1_window_skew", 30.0f); eg::setP (p, "s1_scale", 9.0f); });
        ev (230003, [] (Proc& p) { p.setDegreeMask (1, 9, 0b1101ull); eg::setP (p, "s2_pitch_rand", 3.0f); });
        // FX changes at shared boundaries.
        ev (130007, [] (Proc& p) { eg::setP (p, "s1_filter_type", 3.0f); });   // s1 LP → HP
        if (allFx)
        {
            ev (180001, [] (Proc& p) { eg::setP (p, "s4_filter_type", 2.0f); });   // s4 Off → BP
            ev (220009, [] (Proc& p) { eg::setP (p, "s2_ring_freq", 900.0f); });
        }
        return base;
    }

    void blockNullFxRows (bool allFx)
    {
        std::printf ("\n-- N6. PERF-03 block-size null, N5 recipe + per-grain FX (%s)\n", allFx ? "all four stages" : "SVF form");
        Job base = n6Base (allFx);
        Job j32 = base; j32.block = 32;
        eg22::RecLog L32; j32.after = eg22::collect (L32);
        const auto ref = eg::render (j32);
        const auto& rd = ref.eng().diag;
        std::uint32_t bitsSeen = 0; int crushed = 0, uncrushedCrushOn = 0, modes[4] {};
        for (auto& r : L32.r)
        {
            bitsSeen |= r.fxActive;
            modes[juce::jlimit (0, 3, r.svfMode)]++;
            if (r.crushed) ++crushed; else ++uncrushedCrushOn;
        }
        const std::uint32_t want = allFx ? (ogran::kFxSvf | ogran::kFxDrive | ogran::kFxCrush | ogran::kFxRing) : ogran::kFxSvf;
        const bool live = rd.kills > 0 && rd.spawns > 500 && (bitsSeen & want) == want && modes[1] > 0 && modes[2] > 0 && modes[3] > 0
                          && (! allFx || (crushed > 0 && uncrushedCrushOn > 0)) && ! L32.overflow;
        Tally t;
        for (int b : { 441, 512, 4096, 0 })
        {
            Job jb = base; jb.block = b;
            const auto o = eg::render (jb);
            const auto d = eg::diff (ref, o);
            const bool ok = d.count == 0 && o.eng().diag.spawns == rd.spawns && o.eng().diag.reachViolations == 0;
            t.check (ok, (b == 0 ? juce::String ("var") : juce::String (b)) + juce::String::formatted (": %d diffs max %.3g @%lld", d.count, d.maxAbs, (long long) d.first));
        }
        row (t.bad == 0 && live && rd.reachViolations == 0 && rd.nanKills == 0,
             juce::String ("N6 4 streams, everything on + FX (") + (allFx ? "all FX" : "SVF") + "): 441/512/4096/var vs 32 max|d| == 0",
             t.detail (4) + juce::String::formatted ("  [spawns %lld kills %lld, fx bits 0x%x, LP/BP/HP %d/%d/%d, crushed %d/%d]",
                                                     (long long) rd.spawns, (long long) rd.kills, (unsigned) bitsSeen, modes[1], modes[2], modes[3],
                                                     crushed, crushed + uncrushedCrushOn)
                 + (live ? "" : "  VACUOUS"));
        Job js = j32; js.seed = base.seed ^ 0x1ull; js.after = {};
        const auto d = eg::diff (ref, eg::render (js));
        row (d.count > 0, "N6c different-seed control differs", juce::String (d.count) + " diffs");
    }
} } // namespace eg23b (anonymous)

static void engineRows23b()
{
    std::printf ("\n== N6/Z/K2/M3/C3. Stage 2 / Phase 2.3 PERF + QUAL rows (engine_gates_23b.h)\n");
    const auto t0 = std::chrono::steady_clock::now();
    eg23b::blockNullFxRows (false);
    std::printf ("  [INFO] 2.3b rows wall time %.1f s\n", std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
}
