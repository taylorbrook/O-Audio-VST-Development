/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.5a engine gates,
    part a — the law arm (PLAN-2.5a T13, PD14; RESEARCH-2.5a §9). Included by
    main.cpp AFTER engine_gates_24b.h.

      PV1  per-voice independence: vel → size, key → cutoff, note random → pan,
           per-voice LFO → spray; latched len / cutoff == apply (raw, offV_k)   ctrl pvSharedSource → spread 0
      VL1  per-voice LFO saw → s1_level: each voice's first tick == shape (phase) ctrl pvLfoNoReset
      VL2  per-voice S&H + note random, slot offset 0 vs 7 → renders bitwise      ctrl slotKeyedSeed → differ
      VL3  64 notes with steals, per-voice S&H: per-serial values == a voice-free recompute
      ME1  Mod Env A 50 / D 200 / S 40 % / R 300 → closed form <= 1e-6
      ME2  A = R = 0 → the 2 ms floors (N = 96 @ 48 k), coefficients bitwise
      ME3  decay edited mid-decay → the segment is unchanged (bitwise)            ctrl envLiveTimes → differs
      ME4  retrigger() at level 0.5 continues from 0.5
      RG1  source range table (vel / key / slide / bend / pressure / wheel 14-bit / macro) exact
      NZ1  16 routes at ±100 from neutral controllers → bitwise == unrouted       ctrl macro 0.1 % → differs
      CP1  16 × Macro 1 +100 → s1_level clamps at +6 dB exactly; −100 → gain 0; no NaN
      NV1  newest-voice handover (vel 0.2 → 0.9 → output_gain): per cell <= 1.05 × bound + 1e-6   ctrl newestNoRamp >= 10×
      NV2  every voice ended → the value is held bitwise for 2 s
      NV3  poly 1, 8 notes @ 50 ms (each steals): bound, hull, newestWasStolen 0
      NV4  2nd handover 10 ms into the ramp: bound, no overshoot                   ctrl newestStack → overshoot
      SD1  each of the 17 sources → s1_level +60 moves it up
      SD2  per-voice sources → reverb_mix (G) move it via the newest voice
      DR1  Macro 1 → each of the 125 destinations: its consumer moves, nothing else routed (+ MX0 table self-check)
      MD1  MIDI script CC1 / 33, CC74, channel AT, poly AT, bend, CC121 → source values at the next tick

  ==============================================================================
*/
#pragma once

#include <set>

namespace { namespace eg25a
{
    using eg21::Session;
    using eg21::P;
    using eg21::with;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::kSeed;
    using eg21::setP;
    using eg21::Proc;
    using eg22::base4;
    using eg22::all4;
    using eg22::cat;
    using eg22::sn;
    using eg22::shipping;
    using eg22::setAt;
    using eg24b::want;
    using eg24b::section;

    constexpr double kFs = 48000.0;

    // Source / destination indices (mod-sources.tsv / GiParamIds::kModDestIds).
    namespace S { enum : int { None = 0, Lfo1 = 1, Lfo2, Lfo3, Lfo4, Env1, Env2, Vel, Key, Rand, Wheel, Press, Slide, Bend, Mac1, Mac2, Mac3, Mac4 }; }
    inline int dS (int stream1, int field) { return ogi::GiDest::stream (stream1 - 1, field); }
    using F = ogi::GiDest::Field;

    P route (int m, int src, int dest, float depth)
    {
        const juce::String b = "mod" + juce::String (m) + "_";
        return { { b + "source", (float) src }, { b + "dest", (float) dest }, { b + "depth", depth } };
    }

    /** Every mod / sync row at an explicit neutral value (the gates never rely
        on spec defaults): sync Free, LFOs Global sine, macros 0, slots None. */
    P modNeutral()
    {
        P p { { "sync_mode", 0.0f } };
        for (int n = 1; n <= 4; ++n)
        {
            const juce::String b = "lfo" + juce::String (n) + "_";
            p.push_back ({ b + "shape", 0.0f }); p.push_back ({ b + "rate", 1.0f }); p.push_back ({ b + "phase", 0.0f });
            p.push_back ({ b + "mode", 0.0f }); p.push_back ({ b + "rate_sync", 18.0f });
            p.push_back ({ "macro" + juce::String (n), 0.0f });
        }
        for (int m = 1; m <= 16; ++m)
            p = with (p, route (m, 0, 0, 0.0f));
        return p;
    }

    P baseMod (int streams = 1) { return cat ({ base4 (streams), modNeutral(), { { "polyphony", 16.0f }, { "grain_limit", 3.0f } } }); }

    void loadN (Session& s, int n, double seconds = 4.0, float amp = 0.2f)
    {
        for (int k = 0; k < n; ++k)
            s.load (k, noiseBuf (2, (int) (seconds * kFs), amp, 31 + k), kFs);
    }

    juce::NormalisableRange<float> rangeOf (Proc& p, const juce::String& id)
    {
        auto* f = dynamic_cast<juce::AudioParameterFloat*> (param (p, id));
        return f != nullptr ? f->getNormalisableRange() : juce::NormalisableRange<float> {};
    }

    float applyR (const juce::NormalisableRange<float>& r, float raw, float off)
    {
        return r.convertFrom0to1 (std::clamp (r.convertTo0to1 (std::clamp (raw, r.start, r.end)) + off, 0.0f, 1.0f));
    }

    //==========================================================================
    // PV1
    struct PvOut { std::map<std::uint64_t, std::vector<double>> len; std::map<std::uint64_t, std::vector<float>> fc; float sizeRaw = 0, cutRaw = 0, depS = 0.3f, depC = 0.3f; };

    PvOut pvRender (bool shared)
    {
        P p = cat ({ baseMod (1), { { sn (1, "size"), 100.0f }, { sn (1, "density"), 40.0f }, { sn (1, "filter_type"), 1.0f },
                                    { sn (1, "filter_cutoff"), 1000.0f }, { sn (1, "filter_rand"), 0.0f },
                                    { "lfo1_mode", 1.0f }, { "lfo1_rate", 2.0f } },
                     route (1, S::Vel, dS (1, F::Size), 30.0f), route (2, S::Key, dS (1, F::FilterCutoff), 30.0f),
                     route (3, S::Rand, dS (1, F::Pan), 50.0f), route (4, S::Lfo1, dS (1, F::Spray), 40.0f) });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.hk().pvSharedSource = shared;
        loadN (s, 1);
        s.noteOn (0, 48, 38);
        s.noteOn (4800, 60, 76);
        s.noteOn (9600, 72, 114);
        s.run (48000, 512);
        PvOut o;
        for (const auto& e : s.hk().logInOrder())
            if (e.stream == 0)
            {
                o.len[e.serial].push_back (e.len);
                o.fc[e.serial].push_back (e.fcHz);
            }
        o.sizeRaw = eg21::rawOf (*s.p, sn (1, "size"));
        o.depS = eg21::rawOf (*s.p, "mod1_depth") / 100.0f;   // the LANDED depth raw (gp: clamp (raw) / 100)
        o.depC = eg21::rawOf (*s.p, "mod2_depth") / 100.0f;
        o.cutRaw  = eg21::rawOf (*s.p, sn (1, "filter_cutoff"));
        return o;
    }

    void rowPV1()
    {
        auto probe = make();
        const auto rS = rangeOf (*probe, sn (1, "size")), rC = rangeOf (*probe, sn (1, "filter_cutoff"));
        const auto o = pvRender (false);
        const int notes[3] { 48, 60, 72 }, vels[3] { 38, 76, 114 };
        int bad = 0, n = 0, serials = 0;
        double worstLen = 0, worstFc = 0;
        std::set<double> distinct;
        for (int i = 0; i < 3; ++i)
        {
            const std::uint64_t ser = (std::uint64_t) (i + 1);
            const auto it = o.len.find (ser);
            if (it == o.len.end()) continue;
            ++serials;
            const float vel = (float) vels[i] / 127.0f;
            const float key = std::clamp ((float) (notes[i] - 60) / 64.0f, -1.0f, 1.0f);
            const float ms  = std::clamp (applyR (rS, o.sizeRaw, o.depS * vel), 5.0f, 2000.0f);
            const double lenX = std::clamp ((double) ms, ogi::kMinSizeMs, ogi::kMaxSizeMs) * 0.001 * kFs;
            const float fcX = std::clamp (applyR (rC, o.cutRaw, o.depC * key), 20.0f, 20000.0f);
            for (size_t j = 0; j < it->second.size(); ++j)
            {
                const double l = it->second[j];
                const double ulps = std::abs (l - lenX) / std::max (1e-300, std::nextafter (lenX, 1e300) - lenX);
                worstLen = std::max (worstLen, ulps);
                const double fr = std::abs ((double) o.fc.at (ser)[j] / (double) fcX - 1.0);
                worstFc = std::max (worstFc, fr);
                bad += (ulps <= 1.0 && fr <= 1.0e-6) ? 0 : 1;
                distinct.insert (l);
                ++n;
            }
        }
        const auto c = pvRender (true);
        std::set<double> distinctC;
        for (const auto& [ser, v] : c.len) for (double l : v) distinctC.insert (l);
        const bool spread = distinct.size() >= 3, fired = distinctC.size() == 1;
        row (bad == 0 && n > 30 && serials == 3 && spread && fired,
             "PV1 DSP-06 per-voice independence: vel -> size, key -> cutoff, note random -> pan, PV LFO -> spray",
             "meas " + juce::String (n) + " grains of 3 voices, max len err " + f2 (worstLen, 2) + " ulp, max cutoff rel err "
                 + juce::String (worstFc, 3) + ", " + juce::String ((int) distinct.size()) + " distinct lens | thr <= 1 ulp / 1e-6, spread > 0 | "
                 + ctrl (fired, "pvSharedSource -> " + juce::String ((int) distinctC.size()) + " distinct len (spread 0)"));
    }

    //==========================================================================
    // VL1 / VL2 / VL3
    void recordSrc (Session& s, int src, std::map<std::uint64_t, std::vector<float>>& out)
    {
        s.afterBlock = [&out, src] (Session& ss)
        {
            if (ss.pos % ogi::kCell != 0) return;
            for (int k = 0; k < ogi::kVoiceSlots; ++k)
            {
                const auto& v = ss.eng().getVoice (k);
                if (v.isFree()) continue;
                out[v.serial].push_back (ss.eng().getVoiceModForTest (k).src[src]);
            }
        };
    }

    void rowVL1()
    {
        auto run = [] (bool noReset)
        {
            P p = cat ({ baseMod (1), { { "lfo1_mode", 1.0f }, { "lfo1_shape", 2.0f }, { "lfo1_phase", 90.0f }, { "lfo1_rate", 3.0f } },
                         route (1, S::Lfo1, dS (1, F::Level), 50.0f) });
            Session s (kFs, p, kSeed, 32);
            shipping (s);
            s.hk().pvLfoNoReset = noReset;
            loadN (s, 1);
            std::map<std::uint64_t, std::vector<float>> rec;
            s.afterBlock = [&rec] (Session& ss)
            {
                for (int k = 0; k < ogi::kVoiceSlots; ++k)
                {
                    const auto& v = ss.eng().getVoice (k);
                    if (! v.isFree()) rec[v.serial].push_back (ss.eng().getVoiceModForTest (k).src[S::Lfo1]);
                }
            };
            s.noteOn (4800, 60);
            s.noteOn (4800 + 14880, 64);   // 0.31 s later
            s.run (48000, 32);
            return rec;
        };
        const auto r = run (false), c = run (true);
        auto first = [] (const std::map<std::uint64_t, std::vector<float>>& m, std::uint64_t ser) { auto it = m.find (ser); return it != m.end() && it->second.size() > 2 ? it->second[1] : 99.0f; };
        const float a = first (r, 1), b = first (r, 2), cb = first (c, 2);
        const bool ok = juce::exactlyEqual (a, -0.5f) && juce::exactlyEqual (b, -0.5f);
        const bool fired = ! juce::exactlyEqual (cb, -0.5f);
        row (ok && fired, "VL1 DSP-06 per-voice LFO phase reset: each voice's first tick == saw (lfo1_phase 90) = -0.5",
             "meas first-tick value voice 1 " + juce::String (a, 7) + ", voice 2 (0.31 s later) " + juce::String (b, 7)
                 + " | thr == -0.5 bitwise | " + ctrl (fired, "pvLfoNoReset -> voice 2 " + juce::String (cb, 7)));
    }

    std::vector<float> vl2Render (int offset, bool slotKey)
    {
        P p = cat ({ baseMod (1), { { "lfo1_mode", 1.0f }, { "lfo1_shape", 4.0f }, { "lfo1_rate", 7.0f }, { sn (1, "level"), -12.0f } },
                     route (1, S::Lfo1, dS (1, F::Level), 40.0f), route (2, S::Rand, dS (1, F::Pan), 60.0f),
                     route (3, S::Rand, dS (1, F::Size), 30.0f) });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.hk().setSlotOffset (offset);
        s.hk().slotKeyedSeed = slotKey;
        loadN (s, 1);
        for (int i = 0; i < 4; ++i) s.noteOn (2400 * i, 48 + 5 * i);
        s.run (36000, 512);
        return eg22::lr (s);
    }

    void rowVL2()
    {
        const int n = eg21::diffCount (vl2Render (0, false), vl2Render (7, false));
        const int c = eg21::diffCount (vl2Render (0, true), vl2Render (7, true));
        row (n == 0 && c > 0, "VL2 DSP-06 allocation-order invariance: per-voice S&H + note random, slot offset 0 vs 7",
             "meas " + juce::String (n) + " samples differ | thr 0 | " + ctrl (c > 0, "slotKeyedSeed -> " + juce::String (c) + " differ"));
    }

    void rowVL3()
    {
        P p = cat ({ baseMod (1), { { "polyphony", 4.0f }, { "amp_release", 20.0f }, { "lfo1_mode", 1.0f }, { "lfo1_shape", 4.0f }, { "lfo1_rate", 9.0f } },
                     route (1, S::Lfo1, dS (1, F::Size), 20.0f) });
        Session s (kFs, p, kSeed, 32);
        shipping (s);
        loadN (s, 1);
        for (int i = 0; i < 64; ++i)
        {
            s.noteOn (1500 * i, 40 + (i * 7) % 40);
            if (i % 3 == 0) s.noteOff (1500 * i + 4000, 40 + (i * 7) % 40);
        }
        std::map<std::uint64_t, std::vector<float>> rec;
        recordSrc (s, S::Lfo1, rec);
        s.run (1500 * 64 + 9600, 32);
        const float rate = eg21::rawOf (*s.p, "lfo1_rate");
        ogi::TempoTracker tr;
        tr.prepare (kFs);
        ogi::GiGridParams::Lfo lp;
        lp.shape = 4; lp.rateHz = rate; lp.rateSync = 18; lp.phaseDeg = 0.0f;
        int bad = 0, compared = 0;
        for (const auto& [ser, vals] : rec)
        {
            ogi::Lfo lfo;
            lfo.prepare (kFs, s.eng().getRampCellsModForTest(), 0.0f);
            const std::uint64_t sd = ogi::mix64 (kSeed ^ ogi::kVoiceLfoStream ^ ogi::mix64 (ser));
            // [0] the note-on seed (the first tick's value), then one per tick.
            for (size_t i = 0; i < vals.size(); ++i)
            {
                const float x = i == 0 ? ogi::Lfo::shape (4, 0.0, 0, sd, 0)
                                       : lfo.tick (0, lp, false, tr, 0, 0.0f, (double) rate, sd, 0);
                bad += juce::exactlyEqual (x, vals[i]) ? 0 : 1;
                ++compared;
            }
        }
        const auto steals = s.hk().voiceSteals;
        row (bad == 0 && rec.size() >= 60 && steals > 0 && compared > 1000,
             "VL3 DSP-06 per-voice S&H keyed (serial, cycle): 64 notes with steals == voice-free recompute",
             "meas " + juce::String (bad) + " / " + juce::String (compared) + " tick values differ over " + juce::String ((int) rec.size())
                 + " serials (steals " + i64 (steals) + ") | thr 0 | ctrl -- (VL2 slotKeyedSeed)");
    }

    //==========================================================================
    // ME1–ME4
    void rowME1()
    {
        P p = cat ({ baseMod (1), { { "amp_release", 2000.0f }, { "menv1_attack", 50.0f }, { "menv1_decay", 200.0f }, { "menv1_sustain", 40.0f }, { "menv1_release", 300.0f } },
                     route (1, S::Env1, dS (1, F::Size), 20.0f) });
        Session s (kFs, p, kSeed, 32);
        shipping (s);
        loadN (s, 1);
        std::vector<std::pair<juce::int64, double>> rec;
        s.afterBlock = [&rec] (Session& ss)
        {
            if (! ss.eng().getVoice (0).isFree())
                rec.push_back ({ ss.pos, ss.eng().getVoiceModForTest (0).env[0].getLevel() });
        };
        s.noteOn (0, 60);
        s.noteOff (24000, 60);
        s.run (48000, 32);
        const double A = eg21::rawOf (*s.p, "menv1_attack"), D = eg21::rawOf (*s.p, "menv1_decay");
        const double Sus = eg21::rawOf (*s.p, "menv1_sustain") / 100.0, R = eg21::rawOf (*s.p, "menv1_release");
        const double ca = std::exp (std::log (1.0 / 6.0) / (A * 0.001 * kFs));
        const double cd = std::exp (ogi::GiModEnv::kLog60 / (D * 0.001 * kFs));
        const double cr = std::exp (ogi::GiModEnv::kLog60 / (R * 0.001 * kFs));
        // Stage switch samples (closed form): attack ends at the first n with 1.2(1 − ca^n) >= 1.
        juce::int64 na = 1;
        while (1.2 * (1.0 - std::pow (ca, (double) na)) < 1.0) ++na;
        juce::int64 ns = na;
        while ((1.0 - Sus) * std::pow (cd, (double) (ns - na)) > 1.0e-3 * (1.0 - Sus)) ++ns;
        auto closed = [&] (juce::int64 n) -> double
        {
            if (n < 24000)
            {
                if (n < na) return 1.2 * (1.0 - std::pow (ca, (double) n));
                if (n < ns) return Sus + (1.0 - Sus) * std::pow (cd, (double) (n - na));
                return Sus;
            }
            const double l0 = 24000 < ns ? Sus + (1.0 - Sus) * std::pow (cd, (double) (24000 - na)) : Sus;
            const double l = l0 * std::pow (cr, (double) (n - 24000));
            return l < ogi::kEnvEndLevel ? 0.0 : l;   // the release ends at exactly 0 below −96 dB
        };
        double worst = 0.0;
        juce::int64 wAt = 0;
        int n = 0;
        for (const auto& [t, v] : rec)
        {
            if (std::abs (t - na) <= 2 || std::abs (t - ns) <= 2 || std::abs (t - 24000) <= 2) continue;
            const double e = std::abs (v - closed (t));
            if (e > worst) { worst = e; wAt = t; }
            ++n;
        }
        row (worst <= 1.0e-6 && n > 1000, "ME1 DSP-06 Mod Env 1 A 50 / D 200 / S 40 % / R 300 ms vs the closed form",
             "meas max |level - closed| " + juce::String (worst, 3) + " (" + juce::String::formatted ("%.3e", worst) + ")" + " over " + juce::String (n) + " ticks, worst at n " + i64 ((std::uint64_t) wAt) + " (attack ends n " + i64 ((std::uint64_t) na)
                 + ", sustain from n " + i64 ((std::uint64_t) ns) + ") | thr <= 1e-6 | ctrl -- (ME3)");
    }

    void rowME2()
    {
        ogi::GiModEnv e;
        ogi::ModEnvParams p;
        p.attackMs = 0.0f; p.decayMs = 0.0f; p.sustain = 0.5f; p.releaseMs = 0.0f;
        e.noteOn (p, kFs);
        const bool ca = juce::exactlyEqual (e.getCoef(), std::exp (std::log (1.0 / 6.0) / 96.0));
        int nA = 0;
        while (e.getStage() == ogi::GiModEnv::Stage::attack && nA < 1000) { e.advance (1, p, kFs); ++nA; }
        e.noteOff (p, kFs);
        const bool cr = juce::exactlyEqual (e.getCoef(), std::exp (ogi::GiModEnv::kLog60 / 96.0));
        row (ca && cr && nA >= 95 && nA <= 97, "ME2 DSP-06 Mod Env A = R = 0 -> the 2 ms floors (N = 96 @ 48 k)",
             "meas attack coef " + juce::String (ca ? "== exp(ln(1/6)/96)" : "DIFFERS") + ", release coef " + (cr ? "== exp(ln(1e-3)/96)" : "DIFFERS")
                 + ", attack reached 1 at n " + juce::String (nA) + " | thr bitwise, n in [95, 97]");
    }

    void rowME3()
    {
        auto run = [] (bool live)
        {
            ogi::GiModEnv a, b;
            ogi::ModEnvParams p, q;
            p.attackMs = 5.0f; p.decayMs = 400.0f; p.sustain = 0.3f; p.releaseMs = 300.0f;
            q = p; q.decayMs = 50.0f;
            a.noteOn (p, kFs); b.noteOn (p, kFs);
            a.advance (1000, p, kFs); b.advance (1000, p, kFs);   // inside the decay
            int diff = 0;
            for (int i = 0; i < 4000; ++i)
            {
                a.advance (1, p, kFs);
                b.advance (1, q, kFs, live);
                diff += juce::exactlyEqual (a.getLevel(), b.getLevel()) ? 0 : 1;
            }
            return std::make_pair (diff, a.getStage() == ogi::GiModEnv::Stage::decay);
        };
        const auto r = run (false), c = run (true);
        row (r.first == 0 && r.second && c.first > 0, "ME3 DSP-06 Mod Env decay edited mid-decay: the segment keeps its latched time",
             "meas " + juce::String (r.first) + " of 4000 samples differ (still in decay " + juce::String (r.second ? "yes" : "NO")
                 + ") | thr 0 | " + ctrl (c.first > 0, "envLiveTimes -> " + juce::String (c.first) + " differ"));
    }

    void rowME4()
    {
        ogi::GiModEnv e;
        ogi::ModEnvParams p;
        p.attackMs = 50.0f; p.decayMs = 100.0f; p.sustain = 0.2f; p.releaseMs = 100.0f;
        e.noteOn (p, kFs);
        e.advance (24000, p, kFs);
        e.setLevelForTest (0.5);
        e.retrigger (p, kFs);
        e.advance (1, p, kFs);
        const double l1 = e.getLevel();
        const double want = 1.2 + (0.5 - 1.2) * std::exp (std::log (1.0 / 6.0) / (50.0 * 0.001 * kFs));
        row (std::abs (l1 - want) <= 1.0e-12 && e.getStage() == ogi::GiModEnv::Stage::attack,
             "ME4 DSP-06 Mod Env retrigger() continues the attack from the current level (0.5)",
             "meas level after 1 sample " + juce::String (l1, 9) + " (expect " + juce::String (want, 9) + ") | thr <= 1e-12");
    }

    //==========================================================================
    // RG1 / MD1 — source values
    struct SrcProbe { juce::int64 at; std::function<void (Session&, Tally&)> check; };

    void rowRG1()
    {
        P p = cat ({ baseMod (1), { { "macro1", 100.0f } } });
        Session s (kFs, p, kSeed, 32);
        shipping (s);
        loadN (s, 1);
        Tally t;
        int checks = 0;
        auto voiceOf = [] (Session& ss, int note) { for (int k = 0; k < ogi::kVoiceSlots; ++k) if (! ss.eng().getVoice (k).isFree() && ss.eng().getVoice (k).note == note) return k; return -1; };
        auto eq = [&t, &checks] (float got, float want, const juce::String& what) { ++checks; t.check (juce::exactlyEqual (got, want), what + " " + juce::String (got, 9) + " != " + juce::String (want, 9)); };
        s.addMidi (0, juce::MidiMessage::noteOn (1, 0, (juce::uint8) 127));
        s.addMidi (0, juce::MidiMessage::noteOn (1, 60, (juce::uint8) 64));
        s.addMidi (0, juce::MidiMessage::noteOn (1, 124, (juce::uint8) 1));
        s.addMidi (0, juce::MidiMessage::noteOn (1, 127, (juce::uint8) 100));
        s.at (3200, [&] (Session& ss)
        {
            const int k0 = voiceOf (ss, 0), k60 = voiceOf (ss, 60), k124 = voiceOf (ss, 124), k127 = voiceOf (ss, 127);
            if (k0 < 0 || k60 < 0 || k124 < 0 || k127 < 0) { t.check (false, "voices"); return; }
            const auto& e = ss.eng();
            eq (e.getVoiceModForTest (k0).src[S::Vel], 127.0f / 127.0f, "vel 127");
            eq (e.getVoiceModForTest (k60).src[S::Vel], 64.0f / 127.0f, "vel 64");
            eq (e.getVoiceModForTest (k124).src[S::Vel], 1.0f / 127.0f, "vel 1");
            eq (e.getVoiceModForTest (k0).src[S::Key], -0.9375f, "key 0");
            eq (e.getVoiceModForTest (k60).src[S::Key], 0.0f, "key 60");
            eq (e.getVoiceModForTest (k124).src[S::Key], 1.0f, "key 124");
            eq (e.getVoiceModForTest (k127).src[S::Key], 1.0f, "key 127 (clamped)");
            eq (e.getSrcGForTest (S::Mac1), 1.0f, "macro 100 %");
            eq (e.getVoiceModForTest (k60).src[S::Slide], 0.0f, "slide default");
        });
        auto at = [&] (juce::int64 time, const juce::MidiMessage& m, int src, float want, const juce::String& what, bool global = false)
        {
            s.addMidi (time, m);
            s.at (time + 64, [=, &voiceOf, &eq] (Session& ss)
            {
                const int k = voiceOf (ss, 60);
                eq (global ? ss.eng().getSrcGForTest (src) : ss.eng().getVoiceModForTest (std::max (k, 0)).src[src], want, what);
            });
        };
        at (4800, juce::MidiMessage::controllerEvent (1, 74, 0), S::Slide, -1.0f, "slide cc74 0");
        at (6400, juce::MidiMessage::controllerEvent (1, 74, 64), S::Slide, 0.0f, "slide cc74 64");
        at (8000, juce::MidiMessage::controllerEvent (1, 74, 127), S::Slide, 63.0f / 64.0f, "slide cc74 127");
        at (9600, juce::MidiMessage::pitchWheel (1, 16383), S::Bend, 1.0f, "bend max");
        at (11200, juce::MidiMessage::pitchWheel (1, 0), S::Bend, -1.0f, "bend min");
        at (12800, juce::MidiMessage::pitchWheel (1, 8192), S::Bend, 0.0f, "bend centre");
        at (14400, juce::MidiMessage::channelPressureChange (1, 127), S::Press, 1.0f, "pressure 127");
        at (16000, juce::MidiMessage::aftertouchChange (1, 60, 64), S::Press, 64.0f / 127.0f, "poly AT 64");
        at (17600, juce::MidiMessage::controllerEvent (1, 1, 127), S::Wheel, (127.0f * 128.0f) / 16383.0f, "wheel msb 127", true);
        at (19200, juce::MidiMessage::controllerEvent (1, 33, 127), S::Wheel, 1.0f, "wheel 14-bit max", true);
        at (20800, juce::MidiMessage::controllerEvent (1, 1, 0), S::Wheel, 0.0f, "wheel msb 0 clears lsb", true);
        s.run (24000, 32);
        row (t.bad == 0 && checks >= 20, "RG1 DSP-06 source range table (vel / key / slide / bend / pressure / wheel 14-bit / macro) exact",
             "meas " + t.detail (checks) + " | thr bitwise");
    }

    void rowMD1()
    {
        P p = baseMod (1);
        Session s (kFs, p, kSeed, 32);
        shipping (s);
        loadN (s, 1);
        Tally t;
        int checks = 0;
        s.noteOn (0, 60);
        s.noteOn (0, 67);
        // events mid-cell (sample 4813): wheel is global → visible at the NEXT tick (4832), not at 4800.
        std::vector<float> wheelAt;
        // after the block [4800, 4832) the tick at 4800 preceded the event → 0; the tick at 4832 sees it.
        s.afterBlock = [&wheelAt] (Session& ss) { if (ss.pos == 4832 || ss.pos == 4864 || ss.pos == 4896) wheelAt.push_back (ss.eng().getSrcGForTest (S::Wheel)); };
        s.addMidi (4813, juce::MidiMessage::controllerEvent (1, 1, 100));
        s.addMidi (4813, juce::MidiMessage::controllerEvent (1, 33, 50));
        s.addMidi (6413, juce::MidiMessage::controllerEvent (2, 74, 96));
        s.addMidi (8013, juce::MidiMessage::channelPressureChange (3, 90));
        s.addMidi (9613, juce::MidiMessage::aftertouchChange (1, 67, 30));
        s.addMidi (11213, juce::MidiMessage::pitchWheel (1, 12288));
        s.addMidi (12813, juce::MidiMessage::controllerEvent (1, 121, 0));
        auto check = [&] (juce::int64 time, std::function<void (Session&)> f) { s.at (time, std::move (f)); };
        auto eq = [&t, &checks] (float got, float want, const juce::String& what) { ++checks; t.check (juce::exactlyEqual (got, want), what + " " + juce::String (got, 9) + " != " + juce::String (want, 9)); };
        auto k60 = [] (Session& ss) { for (int k = 0; k < ogi::kVoiceSlots; ++k) if (ss.eng().getVoice (k).note == 60) return k; return 0; };
        auto k67 = [] (Session& ss) { for (int k = 0; k < ogi::kVoiceSlots; ++k) if (ss.eng().getVoice (k).note == 67) return k; return 0; };
        check (6464, [&] (Session& ss) { eq (ss.eng().getVoiceModForTest (k60 (ss)).src[S::Slide], 0.5f, "cc74 96 (ch 2) -> every voice"); eq (ss.eng().getVoiceModForTest (k67 (ss)).src[S::Slide], 0.5f, "cc74 voice 2"); });
        check (8064, [&] (Session& ss) { eq (ss.eng().getVoiceModForTest (k60 (ss)).src[S::Press], 90.0f / 127.0f, "channel AT (ch 3)"); });
        check (9664, [&] (Session& ss) { eq (ss.eng().getVoiceModForTest (k67 (ss)).src[S::Press], 30.0f / 127.0f, "poly AT note 67");
                                          eq (ss.eng().getVoiceModForTest (k60 (ss)).src[S::Press], 90.0f / 127.0f, "poly AT leaves note 60"); });
        check (11264, [&] (Session& ss) { eq (ss.eng().getVoiceModForTest (k60 (ss)).src[S::Bend], 4096.0f / 8191.0f, "bend 12288"); });
        check (12864, [&] (Session& ss)
        {
            const auto& m = ss.eng().getVoiceModForTest (k60 (ss));
            eq (ss.eng().getSrcGForTest (S::Wheel), 0.0f, "CC121 wheel");
            eq (m.src[S::Press], 0.0f, "CC121 pressure");
            eq (m.src[S::Slide], 0.0f, "CC121 slide");
            eq (m.src[S::Bend], 0.0f, "CC121 bend");
        });
        s.run (14400, 32);
        const float w = (100.0f * 128.0f + 50.0f) / 16383.0f;
        const bool timing = wheelAt.size() == 3 && juce::exactlyEqual (wheelAt[0], 0.0f) && juce::exactlyEqual (wheelAt[1], w) && juce::exactlyEqual (wheelAt[2], w);
        row (t.bad == 0 && checks == 10 && timing, "MD1 DSP-06 MIDI script (CC1/33, CC74, channel AT, poly AT, bend, CC121) -> sources at the next tick",
             "meas " + t.detail (checks) + ", wheel after the ticks 4800 / 4832 / 4864 = " + (wheelAt.size() == 3 ? f2 (wheelAt[0], 6) + " / " + f2 (wheelAt[1], 6) + " / " + f2 (wheelAt[2], 6) : juce::String ("?"))
                 + " | thr bitwise, global sources at the next tick");
    }

    //==========================================================================
    // NZ1 / CP1
    std::vector<float> nzRender (bool routed, float macro)
    {
        P p = cat ({ baseMod (2), { { "macro1", macro }, { sn (1, "level"), -6.0f }, { "reverb_mix", 20.0f } } });
        if (routed)
        {
            const int dests[16] { dS (1, F::Level), dS (1, F::Size), dS (1, F::Pitch), 1, ogi::GiDest::RevMix, ogi::GiDest::Feedback,
                                  dS (1, F::Pan), dS (1, F::Density), dS (1, F::ScanRate), ogi::GiDest::LfoRate1, dS (1, F::LoopEnd),
                                  ogi::GiDest::FbLowCut, dS (1, F::FilterCutoff), dS (1, F::Spray), dS (1, F::Window), dS (2, F::Level) };
            const int srcs[6] { S::Mac1, S::Wheel, S::Press, S::Slide, S::Bend, S::Key };   // Mac1 → s1_level, s1_pan, s1_spray
            for (int m = 0; m < 16; ++m)
                p = with (p, route (m + 1, srcs[m % 6], dests[m], (m % 2) == 0 ? 100.0f : -100.0f));
        }
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        loadN (s, 2);
        s.noteOn (0, 60);
        s.noteOn (9600, 60);
        s.noteOff (36000, 60);
        s.run (48000, 512);
        return eg22::lr (s);
    }

    void rowNZ1()
    {
        const auto ref = nzRender (false, 0.0f);
        const int n = eg21::diffCount (ref, nzRender (true, 0.0f));
        const int c = eg21::diffCount (nzRender (false, 0.1f), nzRender (true, 0.1f));
        row (n == 0 && c > 0, "NZ1 DSP-06 16 routes at +/-100 from neutral controllers (wheel, pressure, slide, bend, key 60, macro 0) == unrouted",
             "meas " + juce::String (n) + " samples differ | thr 0 (bitwise) | " + ctrl (c > 0, "macro 1 = 0.1 % -> " + juce::String (c) + " differ"));
    }

    void rowCP1()
    {
        P p = cat ({ baseMod (1), { { "macro1", 100.0f } } });
        for (int m = 1; m <= 16; ++m) p = with (p, route (m, S::Mac1, dS (1, F::Level), 100.0f));
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        loadN (s, 1);
        std::vector<float> tr (4000, 0.0f);
        s.hk().traceDest = { dS (1, F::Level), -1, tr.data(), (std::int64_t) tr.size(), 0 };
        s.noteOn (0, 60);
        for (int m = 1; m <= 16; ++m) setAt (s, 24000, "mod" + juce::String (m) + "_depth", -100.0f);
        s.run (48000, 512);
        const auto n = (size_t) s.hk().traceDest.n;
        s.hk().traceDest = {};
        const float hi = n > 700 ? tr[700] : -1.0f, lo = n > 1400 ? tr[n - 1] : -1.0f;
        int nan = 0;
        for (float x : s.L) nan += std::isfinite (x) ? 0 : 1;
        const float want = ogi::levelDbToGain (6.0f);
        row (juce::exactlyEqual (hi, want) && juce::exactlyEqual (lo, 0.0f) && nan == 0,
             "CP1 DSP-06 16 x Macro 1 +100 -> s1_level clamps at +6 dB exactly; -100 -> gain 0; no NaN",
             "meas level gain " + juce::String (hi, 9) + " (want " + juce::String (want, 9) + "), then " + juce::String (lo, 9) + ", non-finite "
                 + juce::String (nan) + " | thr bitwise");
    }

    //==========================================================================
    // NV1–NV4 — the newest-voice rule (PD8), traced on the mono velocity
    struct NvTrace { std::vector<float> mono; std::vector<juce::int64> t; std::uint64_t handovers = 0, stolen = 0; int rampCells = 30; };

    NvTrace nvRender (const std::vector<std::tuple<juce::int64, int, int>>& notes, int poly, bool noRamp, bool stack, juce::int64 len,
                      const std::vector<std::pair<juce::int64, int>>& offs = {})
    {
        P p = cat ({ baseMod (1), { { "polyphony", (float) poly }, { "amp_release", 30.0f } }, route (1, S::Vel, 1, 50.0f) });
        Session s (kFs, p, kSeed, 32);
        shipping (s);
        s.hk().newestNoRamp = noRamp;
        s.hk().newestStack = stack;
        loadN (s, 1);
        NvTrace o;
        s.afterBlock = [&o] (Session& ss) { o.mono.push_back (ss.eng().getMonoForTest (S::Vel)); o.t.push_back (ss.pos); };
        for (const auto& [t, n, v] : notes) s.noteOn (t, n, v);
        for (const auto& [t, n] : offs) s.noteOff (t, n);
        s.run (len, 32);
        o.handovers = s.hk().handovers;
        o.stolen = s.hk().newestWasStolen;
        o.rampCells = s.eng().getRampCellsModForTest();
        return o;
    }

    /** Per cell |Δmono| / (|target − from| / rampCells) with from / target re-read at each handover. */
    double nvWorst (const NvTrace& o, const std::vector<std::pair<juce::int64, float>>& handoverTargets, double& hullLo, double& hullHi, bool& overshoot)
    {
        double worst = 0.0;
        float from = 0.0f, target = 0.0f;
        size_t h = 0;
        overshoot = false;
        hullLo = 1e9; hullHi = -1e9;
        for (size_t i = 1; i < o.mono.size(); ++i)
        {
            // the handover is seen at the first tick after the note-on
            while (h < handoverTargets.size() && o.t[i] > handoverTargets[h].first + ogi::kCell)
            {
                from = o.mono[i - 1];
                target = handoverTargets[h].second;
                ++h;
            }
            const double d = std::abs ((double) o.mono[i] - (double) o.mono[i - 1]);
            const double bound = std::abs ((double) target - (double) from) / (double) o.rampCells;
            if (d > 2e-6) worst = std::max (worst, (d - 1e-6) / std::max (bound, 1e-12));
            const double lo = std::min (from, target), hi = std::max (from, target);
            if ((double) o.mono[i] < lo - 1e-6 || (double) o.mono[i] > hi + 1e-6) overshoot = true;
            hullLo = std::min (hullLo, (double) o.mono[i]);
            hullHi = std::max (hullHi, (double) o.mono[i]);
        }
        return worst;
    }

    void rowNV()
    {
        // NV1: vel 25 (0.197) at 0.1 s, 114 (0.898) at 0.5 s.
        {
            const std::vector<std::tuple<juce::int64, int, int>> notes { { 4800, 60, 25 }, { 24000, 64, 114 } };
            const std::vector<std::pair<juce::int64, float>> hv { { 4800, 25.0f / 127.0f }, { 24000, 114.0f / 127.0f } };
            double lo, hi; bool ov, ovC;
            const auto o = nvRender (notes, 16, false, false, 48000);
            const double w = nvWorst (o, hv, lo, hi, ov);
            const auto c = nvRender (notes, 16, true, false, 48000);
            const double wc = nvWorst (c, hv, lo, hi, ovC);
            row (w <= 1.05 && o.handovers == 2 && ! ov && wc >= 10.0, "NV1 D4 newest-voice handover vel 0.2 -> 0.9 (-> output_gain): per cell <= 1.05 x bound",
                 "meas worst |dmono| / bound " + f2 (w, 3) + ", handovers " + i64 (o.handovers) + ", overshoot " + (ov ? "YES" : "no")
                     + " | thr <= 1.05 x (|target - from| / " + juce::String (o.rampCells) + ") + 1e-6 | " + ctrl (wc >= 10.0, "newestNoRamp -> " + f2 (wc, 1) + "x"));
        }
        // NV2: held when silent.
        {
            const auto o = nvRender ({ { 4800, 60, 90 } }, 16, false, false, 48000 * 3, { { 9600, 60 } });
            const size_t from = (size_t) (48000 / 32);   // 1 s: the voice has ended (release 30 ms)
            int moved = 0;
            for (size_t i = from + 1; i < o.mono.size(); ++i) moved += juce::exactlyEqual (o.mono[i], o.mono[from]) ? 0 : 1;
            const bool atVel = std::abs (o.mono[from] - 90.0f / 127.0f) < 1e-6f;
            row (moved == 0 && atVel && o.mono.size() >= from + 3000, "NV2 D4 every voice ended -> the newest value is held (bitwise, 2 s)",
                 "meas " + juce::String (moved) + " ticks moved, held " + f2 (o.mono[from], 6) + " (vel 90/127) | thr 0");
        }
        // NV3: poly 1, 8 notes @ 50 ms (each steals the newest).
        {
            std::vector<std::tuple<juce::int64, int, int>> notes;
            std::vector<std::pair<juce::int64, float>> hv;
            const int vels[8] { 20, 110, 45, 127, 5, 80, 60, 100 };
            for (int i = 0; i < 8; ++i) { notes.push_back ({ 4800 + 2400 * i, 50 + i, vels[i] }); hv.push_back ({ 4800 + 2400 * i, vels[i] / 127.0f }); }
            double lo, hi; bool ov;
            const auto o = nvRender (notes, 1, false, false, 48000);
            const double w = nvWorst (o, hv, lo, hi, ov);
            const bool hull = lo >= 0.0 && hi <= 1.0 + 1e-6;
            row (w <= 1.05 && ! ov && hull && o.stolen == 0 && o.handovers == 8, "NV3 D4 poly 1, 8 notes @ 50 ms (each steals): bound, hull, never a stolen spare",
                 "meas worst " + f2 (w, 3) + " x bound, overshoot " + (ov ? "YES" : "no") + ", range " + f2 (lo, 4) + ".." + f2 (hi, 4)
                     + ", handovers " + i64 (o.handovers) + ", newestWasStolen " + i64 (o.stolen) + " | thr <= 1.05, 0");
        }
        // NV4: second handover 10 ms into the first ramp.
        {
            const std::vector<std::tuple<juce::int64, int, int>> notes { { 4800, 60, 25 }, { 24000, 64, 114 }, { 24480, 67, 64 } };
            const std::vector<std::pair<juce::int64, float>> hv { { 4800, 25.0f / 127.0f }, { 24000, 114.0f / 127.0f }, { 24480, 64.0f / 127.0f } };
            double lo, hi; bool ov, ovC;
            const auto o = nvRender (notes, 16, false, false, 48000);
            const double w = nvWorst (o, hv, lo, hi, ov);
            const auto c = nvRender (notes, 16, false, true, 48000);
            const double wc = nvWorst (c, hv, lo, hi, ovC);
            const bool fired = ovC || wc > 2.0;
            row (w <= 1.05 && ! ov && o.handovers == 3 && fired, "NV4 D4 2nd handover 10 ms into the ramp: from the current blend, no overshoot",
                 "meas worst " + f2 (w, 3) + " x bound, overshoot " + (ov ? "YES" : "no") + " | thr <= 1.05 | "
                     + ctrl (fired, "newestStack -> " + f2 (wc, 1) + "x, overshoot " + (ovC ? "yes" : "no")));
        }
    }

    //==========================================================================
    // SD1 / SD2
    struct SdSetup { int src; P params; std::function<void (Session&)> midi; };

    std::vector<SdSetup> sdSetups()
    {
        std::vector<SdSetup> v;
        for (int n = 1; n <= 4; ++n)
            v.push_back ({ n, { { "lfo" + juce::String (n) + "_shape", 3.0f }, { "lfo" + juce::String (n) + "_rate", 0.3f } }, nullptr });
        v.push_back ({ S::Env1, { { "menv1_attack", 5.0f }, { "menv1_sustain", 100.0f } }, nullptr });
        v.push_back ({ S::Env2, { { "menv2_attack", 5.0f }, { "menv2_sustain", 100.0f } }, nullptr });
        v.push_back ({ S::Vel, {}, nullptr });
        v.push_back ({ S::Key, {}, nullptr });
        v.push_back ({ S::Rand, {}, nullptr });
        v.push_back ({ S::Wheel, {}, [] (Session& s) { s.addMidi (100, juce::MidiMessage::controllerEvent (1, 1, 100)); } });
        v.push_back ({ S::Press, {}, [] (Session& s) { s.addMidi (100, juce::MidiMessage::channelPressureChange (1, 100)); } });
        v.push_back ({ S::Slide, {}, [] (Session& s) { s.addMidi (100, juce::MidiMessage::controllerEvent (1, 74, 120)); } });
        v.push_back ({ S::Bend, {}, [] (Session& s) { s.addMidi (100, juce::MidiMessage::pitchWheel (1, 14000)); } });
        for (int n = 1; n <= 4; ++n)
            v.push_back ({ S::Mac1 + n - 1, { { "macro" + juce::String (n), 50.0f } }, nullptr });
        return v;
    }

    void rowSD()
    {
        const juce::String names[18] { "-", "LFO1", "LFO2", "LFO3", "LFO4", "Env1", "Env2", "Vel", "Key", "Rand", "Wheel", "Press", "Slide", "Bend", "Mac1", "Mac2", "Mac3", "Mac4" };
        auto probe = [] (const SdSetup& st, int dest, bool routed, bool pv)
        {
            P p = cat ({ baseMod (1), { { sn (1, "level"), -12.0f } }, st.params });
            if (routed) p = with (p, route (1, st.src, dest, 60.0f));
            Session s (kFs, p, kSeed, 512);
            shipping (s);
            loadN (s, 1);
            if (st.midi) st.midi (s);
            s.noteOn (200, 100, 100);
            std::vector<float> tr (2000, 0.0f);
            if (! pv && dest == dS (1, F::Level))
                s.hk().traceDest = { dest, -1, tr.data(), (std::int64_t) tr.size(), 0 };
            s.run (19200, 512);
            float v = 0.0f;
            if (dest == ogi::GiDest::RevMix) v = s.eng().getRevMixForTest();
            else if (pv) v = s.eng().getPairMixForTest (0) ? s.eng().getPairLevelForTest (0, 0) : ogi::levelDbToGain (-12.0f);
            else v = s.hk().traceDest.n > 0 ? tr[(size_t) s.hk().traceDest.n - 1] : 0.0f;
            s.hk().traceDest = {};
            return v;
        };
        int ok1 = 0, ok2 = 0, n2 = 0;
        juce::String bad1, bad2;
        const float base = ogi::levelDbToGain (-12.0f);
        for (const auto& st : sdSetups())
        {
            const bool pv = ogi::GiSrc::isPerVoice (st.src);
            const float v = probe (st, dS (1, F::Level), true, pv);
            if (v > base + 1.0e-3f) ++ok1; else bad1 << " " << names[st.src] << "=" << juce::String (v, 5);
            if (pv)
            {
                ++n2;
                const float r = probe (st, ogi::GiDest::RevMix, true, false);
                if (r > 1.0e-3f) ++ok2; else bad2 << " " << names[st.src] << "=" << juce::String (r, 5);
            }
        }
        row (ok1 == 17, "SD1 DSP-06 each of the 17 sources -> s1_level +60 moves it up (G: the stream substitute; per-voice: the pair gain)",
             "meas " + juce::String (ok1) + "/17 moved above " + f2 (base, 4) + bad1 + " | thr 17/17");
        row (ok2 == n2 && n2 == 8, "SD2 DSP-06 per-voice sources -> reverb_mix (G) move it through the newest voice",
             "meas " + juce::String (ok2) + "/" + juce::String (n2) + " moved" + bad2 + " | thr 8/8");
    }

    //==========================================================================
    // DR1 — Macro 1 → every destination (+ MX0 table self-check)
    void rowDR1()
    {
        // MX0: every mod-dests.tsv id is a float parameter and the dest choice names match the frozen order.
        auto probe = make();
        int mx0Bad = 0;
        auto* destChoice = dynamic_cast<juce::AudioParameterChoice*> (param (*probe, "mod1_dest"));
        const auto tsv = readTsv ("mod-dests.tsv");
        for (const auto& c : tsv)
        {
            const int idx = c[0].getIntValue();
            if (idx == 0) continue;
            auto* f = dynamic_cast<juce::AudioParameterFloat*> (param (*probe, c[1]));
            const bool nameOk = destChoice != nullptr && idx < destChoice->choices.size() && f != nullptr && destChoice->choices[idx] == f->getName (256);
            mx0Bad += (f != nullptr && juce::String (Ids::kModDestIds[idx]) == c[1] && nameOk) ? 0 : 1;
        }

        auto run = [] (int d, float depth)
        {
            P p = cat ({ baseMod (4), { { "macro1", 100.0f }, { "feedback", 30.0f }, { "reverb_mix", 20.0f } } });
            for (int n = 1; n <= 4; ++n) p = with (p, { { sn (n, "filter_type"), 1.0f } });
            if (d > 0) p = with (p, route (1, S::Mac1, d, depth));
            Session s (kFs, p, kSeed, 512);
            shipping (s);
            loadN (s, 4, 2.0);
            std::vector<float> tr (4000, 0.0f);
            s.hk().traceDest = { std::abs (d), -1, tr.data(), (std::int64_t) tr.size(), 0 };
            s.noteOn (0, 60);
            s.run (12000, 512);
            tr.resize ((size_t) s.hk().traceDest.n);
            s.hk().traceDest = {};
            return std::make_pair (tr, s.eng().getMatrixForTest().routedCount());
        };
        int moved = 0, single = 0;
        juce::String bad;
        for (int d = 1; d < ogi::kGiModDests; ++d)
        {
            const auto r = rangeOf (*probe, Ids::kModDestIds[d]);
            const float raw = eg21::rawOf (*probe, Ids::kModDestIds[d]);
            const float depth = r.convertTo0to1 (raw) > 0.5f ? -50.0f : 50.0f;
            const auto ref = run (-d, depth);   // unrouted, the same trace
            const auto mod = run (d, depth);
            const size_t n = std::min (ref.first.size(), mod.first.size());
            double dmax = 0.0;
            for (size_t i = 0; i < n; ++i) dmax = std::max (dmax, (double) std::abs (mod.first[i] - ref.first[i]));
            const bool mv = ! ref.first.empty() && ! mod.first.empty() && (dmax > 0.0 || ref.first.size() != mod.first.size());
            moved += mv ? 1 : 0;
            single += mod.second == 1 ? 1 : 0;
            if (! mv || mod.second != 1) bad << " " << Ids::kModDestIds[d] << "(n " << (int) n << ", routed " << mod.second << ")";
        }
        row (moved == 125 && single == 125 && mx0Bad == 0, "DR1 DSP-06 Macro 1 -> each of the 125 destinations: its consumer moves, only it routed (+ MX0)",
             "meas consumer moved " + juce::String (moved) + "/125, routedCount == 1 on " + juce::String (single) + "/125, MX0 table mismatches "
                 + juce::String (mx0Bad) + (bad.isEmpty() ? juce::String() : " |" + bad.substring (0, 300)) + " | thr 125 / 125 / 0");
    }

    void runAll()
    {
        std::printf ("\n== Phase 2.5a law arm (eg25a): per-voice sources, Mod Env, ranges, newest voice, destinations\n");
        if (want ("PV")) { section ("PV. per-voice independence"); rowPV1(); }
        if (want ("VL")) { section ("VL. per-voice LFO"); rowVL1(); rowVL2(); rowVL3(); }
        if (want ("ME")) { section ("ME. Mod Envs"); rowME1(); rowME2(); rowME3(); rowME4(); }
        if (want ("RG")) { section ("RG / MD. source values"); rowRG1(); rowMD1(); }
        if (want ("NZ")) { section ("NZ / CP. neutral + clamp"); rowNZ1(); rowCP1(); }
        if (want ("NV")) { section ("NV. newest-voice rule"); rowNV(); }
        if (want ("SD")) { section ("SD. every source"); rowSD(); }
        if (want ("DR")) { section ("DR. every destination"); rowDR1(); }
    }
}} // namespace ::eg25a
