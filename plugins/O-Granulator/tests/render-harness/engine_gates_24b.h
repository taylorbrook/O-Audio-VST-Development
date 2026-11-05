/*
  ==============================================================================

    O-Granulator — render harness, sections N7 / FZ / RV / Z3 / Z4 / M4 / C4
    (Stage 2 / Phase 2.4, PLAN-2.4 T7–T12, P44). Sibling of engine_gates_24.h.

      N7  PERF-03 block-size null: the N6 all-FX recipe + feedback 80 %,
          diffusion 50 %, saturation 50 %, a fb_high_cut change at a shared
          boundary (T7); + freeze toggles incl. a re-engage inside the 10 ms
          release fade (T9); + reverb_mix 0 → 30 → 0 (T12, final). 441 / 512 /
          4096 / var vs 32, VALUE compare (R35). N7c different seed differs.

  ==============================================================================
*/
#pragma once

namespace { namespace eg24b
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg22::sp;
    constexpr double kFs = 48000.0;

    //==========================================================================
    // N7 — PERF-03 block-size null with feedback (T7 form; T9 / T12 widen it)
    //==========================================================================
    Job n7Base (int form)
    {
        Job base = eg23b::n6Base (true);
        for (auto& kv : P { { "feedback", 80.0f }, { "fb_diffusion", 50.0f }, { "fb_saturation", 50.0f } }) base.params.push_back (kv);
        base.events.push_back ({ 165001, [] (Proc& p) { eg::setP (p, "fb_high_cut", 3000.0f); } });
        if (form >= 1)
        {
            // Freeze toggles at three shared ticks (the third re-engages inside the 10 ms
            // release fade) with a frozen position automation.
            base.events.push_back ({ 64 * 1500,        [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
            base.events.push_back ({ 64 * 1700 + 5,    [] (Proc& p) { eg::setP (p, "s1_position", 60.0f); eg::setP (p, "s2_position", 10.0f); } });
            base.events.push_back ({ 64 * 2100,        [] (Proc& p) { eg::setP (p, "freeze", 0.0f); } });
            base.events.push_back ({ 64 * 2100 + 192,  [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });   // 6 cells into the 15-cell fade
            base.events.push_back ({ 64 * 2600 + 7,    [] (Proc& p) { eg::setP (p, "freeze", 0.0f); } });
        }
        if (form >= 2)
        {
            base.events.push_back ({ 60011, [] (Proc& p) { eg::setP (p, "reverb_mix", 30.0f); } });
            base.events.push_back ({ 240007, [] (Proc& p) { eg::setP (p, "reverb_mix", 0.0f); } });
        }
        return base;
    }

    void blockNullFbRows (int form)
    {
        const char* fn[3] { "feedback", "feedback + freeze", "feedback + freeze + reverb (final)" };
        std::printf ("\n-- N7. PERF-03 block-size null, N6 all-FX recipe + %s\n", fn[form]);
        Job base = n7Base (form);
        Job j32 = base; j32.block = 32;
        const auto ref = eg::render (j32);
        const auto& rd = ref.eng().diag;
        // Value compare (R35): the reverb's idle hand-off may flip −0 / +0 across splits.
        auto vdiff = [] (const Out& a, const Out& b, int& n, double& mx)
        {
            n = 0; mx = 0.0;
            for (size_t i = 0; i < a.L.size(); ++i)
            {
                const double dl = std::abs ((double) a.L[i] - (double) b.L[i]), dr = std::abs ((double) a.R[i] - (double) b.R[i]);
                if (dl > 0.0 || dr > 0.0 || std::isnan (a.L[i]) != std::isnan (b.L[i])) ++n;
                mx = juce::jmax (mx, dl, dr);
            }
        };
        Tally t;
        for (int b : { 441, 512, 4096, 0 })
        {
            Job jb = base; jb.block = b;
            const auto o = eg::render (jb);
            int n; double mx; vdiff (ref, o, n, mx);
            t.check (n == 0, (b == 0 ? juce::String ("var") : juce::String (b)) + juce::String::formatted (": %d diffs max %.3g", n, mx));
        }
        bool live = rd.spawns > 500 && rd.kills > 0 && rd.fbScrubs == 0;
        juce::String extra;
        if (form >= 1)
        {
            live = live && rd.freezeEngages == 2 && rd.freezeReleases == 2 && rd.forcedReleases > 0;
            extra << juce::String::formatted (", engages %lld releases %lld forced %lld reflections %lld", (long long) rd.freezeEngages,
                                              (long long) rd.freezeReleases, (long long) rd.forcedReleases, (long long) rd.headReflections);
        }
        row (t.bad == 0 && live, juce::String ("N7 ") + fn[form] + ": 441/512/4096/var vs 32 max|d| == 0 (value compare)",
             t.detail (4) + juce::String::formatted ("  [spawns %lld kills %lld scrubs %lld", (long long) rd.spawns, (long long) rd.kills,
                                                     (long long) rd.fbScrubs) + extra + "]" + (live ? "" : "  VACUOUS"));
        Job js = j32; js.seed = base.seed ^ 0x1ull;
        int n; double mx; vdiff (ref, eg::render (js), n, mx);
        row (n > 0, juce::String ("N7c ") + fn[form] + ": different-seed control differs", juce::String (n) + " diffs");
    }
} } // namespace eg24b (anonymous)

static void engineRows24b()
{
    std::printf ("\n== N7/FZ/RV/Z3/Z4/M4/C4. Stage 2 / Phase 2.4 rows (engine_gates_24b.h)\n");
    const auto t0 = std::chrono::steady_clock::now();
    eg24b::blockNullFbRows (0);
    std::printf ("  [INFO] 2.4b rows wall time %.1f s\n", std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
}
