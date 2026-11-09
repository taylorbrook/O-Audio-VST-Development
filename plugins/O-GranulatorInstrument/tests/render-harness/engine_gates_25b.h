/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.5a engine gates,
    part b (PLAN-2.5a T14 / T15, PD14 / PD17 / PD18). Included by main.cpp
    AFTER engine_gates_25a.h (uses eg24b / eg25a helpers).

      RTa-g  route-edit trajectory (parent MX3 bound): 7 edit kinds x {global LFO,
             per-voice LFO, Mod Env} on 16 destinations; per cell |dv01| <= 1.05 x B
             + 1e-6, 0 cross-talk                               RTn noResidual / noDepthRamp > 2x
      REa-g  the same 7 kinds, audio local kappa vs held pre / post references <= 2.0x
                                                                 REn noDepthRamp at the LFO peak > 2.0x
      LM1    lfo1_mode Global <-> Per-voice with 4 voices: trajectory + kappa <= 2.0x
      LM1n   modeSwitchSnap > 2.0x
      TS1    Tempo onsets within 1 sample of the bar grid (44.1 / 48 / 96 k, 4 divs,
             3 tempi, bpm step + loop jump; bursts excluded and counted)
      TS1n   ceilPerTick @ 133 bpm 1/16 T drops slots
      TS2    cap 32, 16 voices: every onset on the base lattice, intervals I x 2^j
      TS2n   tempoContinuousThin -> off-grid onsets
      TS3    Free control: < 10 % of onsets within 1 sample of the grid
      TS4    global LFO Tempo value == shape (frac (ppq / div) + phase) bitwise, across a loop
      TS5    R45 effective density steps / size scale (bitwise)
      TS6    one burst onset at the note-on sample, then the grid   ctrl tempoGridOnly
      TT1    Tempo grid onsets bitwise at 32 / 441 / 512 / 4096 / var
      BN1    every source live, 16 routes + edits, Tempo step + loop: 5 block sizes, 0 diffs
      BN1n   seed + 1 differs
      FA5    0 audio-thread allocations across route edits, mode switches, tempo, steals, CC storms
      TB3    sources busy, unrouted -> renders-2.4 + legacy byte-identical
      TB4    neutral live routes on the recipes -> byte-identical
      AP1    the engine never writes APVTS (static; PLAN "ST1", renamed: eg22 owns ST1)                ctrl the processor's state path

    --cpu25a (PD18): C25w re-run + C25m (<= 40 %) + C25tm (<= 12 %) + C25t, witnesses.

  ==============================================================================
*/
#pragma once

namespace { namespace eg25b
{
    using eg21::Session;
    using eg21::P;
    using eg21::with;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::sineBuf;
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
    using eg24b::kappa;
    using eg24b::Out;
    using eg25a::route;
    using eg25a::baseMod;
    using eg25a::dS;
    using eg25a::loadN;
    namespace S = eg25a::S;
    using F = ogi::GiDest::Field;

    constexpr double kFs = 48000.0;

    //==========================================================================
    // The mock transport (4/4, bar origin 0; an optional bpm step and loop jump).
    struct Tp
    {
        double fs = 48000.0, bpm = 120.0, ppq0 = 0.0;
        juce::int64 stepAt = -1; double bpm2 = 120.0;
        juce::int64 loopAt = -1; double loopBack = 0.0;
        const juce::int64* pos = nullptr;

        double bpmAt (juce::int64 s) const { return (stepAt >= 0 && s >= stepAt) ? bpm2 : bpm; }
        double ppqAt (double s) const
        {
            double q = (stepAt >= 0 && s >= (double) stepAt) ? ppq0 + (double) stepAt * bpm / 60.0 / fs + (s - (double) stepAt) * bpm2 / 60.0 / fs
                                                             : ppq0 + s * bpm / 60.0 / fs;
            if (loopAt >= 0 && s >= (double) loopAt) q -= loopBack;
            return q;
        }
        void fill (juce::AudioPlayHead::PositionInfo& pi) const
        {
            const juce::int64 at = pos != nullptr ? *pos : 0;
            const double q = ppqAt ((double) at);
            pi.setBpm (bpmAt (at));
            pi.setPpqPosition (q);
            pi.setIsPlaying (true);
            pi.setPpqPositionOfLastBarStart (std::floor (q / 4.0) * 4.0);
            pi.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
        }
        /** Segment boundaries (block cuts so every block size sees the change at its sample). */
        void cut (Session& s) const
        {
            if (stepAt >= 0) s.at (stepAt, [] (Session&) {});
            if (loopAt >= 0) s.at (loopAt, [] (Session&) {});
        }
    };

    /** The playhead the processor sees (never copied). */
    struct TpHead final : public juce::AudioPlayHead
    {
        explicit TpHead (const Tp& tIn) : t (tIn) {}
        const Tp& t;
        juce::Optional<PositionInfo> getPosition() const override { PositionInfo pi; t.fill (pi); return pi; }
    };

    //==========================================================================
    // RT — the route-edit trajectory (parent MX3, engine_gates_26b.h:443-480)
    enum Kind { kAdd = 0, kSrcNone, kDestNone, kDepth0, kDepthStep, kDestSwap, kSrcSwap, kKinds };
    const char* kindName (int k) { static const char* n[] { "add", "src->None", "dest->None", "depth->0", "+60->-60", "dest swap", "src swap" }; return n[k]; }

    struct Cls { int src, src2; P params; const char* name; };
    std::vector<Cls> classes()
    {
        return { { S::Lfo1, S::Lfo2, { { "lfo1_rate", 2.0f }, { "lfo2_rate", 1.3f } }, "global LFO" },
                 { S::Lfo1, S::Lfo2, { { "lfo1_rate", 2.0f }, { "lfo2_rate", 1.3f }, { "lfo1_mode", 1.0f }, { "lfo2_mode", 1.0f } }, "per-voice LFO" },
                 { S::Env1, S::Env2, { { "menv1_attack", 300.0f }, { "menv1_sustain", 100.0f }, { "menv2_attack", 100.0f }, { "menv2_sustain", 60.0f } }, "Mod Env" } };
    }
    inline int groupDest (int m) { return dS (1, F::Position + m); }   // 16 stream-1 fields Position … FilterRes

    struct RtTrace { std::vector<std::array<float, ogi::kGiModDests>> dest; std::vector<std::array<float, 3>> src; int rampCells = 30; };

    RtTrace rtRender (const Cls& c, int kind, bool noRes, bool noRamp)
    {
        P p = cat ({ baseMod (1), c.params, { { sn (1, "density"), 40.0f } } });
        for (int m = 0; m < 16; ++m)
            p = with (p, route (m + 1, kind == kAdd ? 0 : c.src, groupDest (m), 60.0f));
        Session s (kFs, p, kSeed, 32);
        shipping (s);
        s.hk().noResidual = noRes;
        s.hk().noDepthRamp = noRamp;
        loadN (s, 1);
        s.noteOn (0, 60);
        const juce::int64 te = 30000;   // 0.625 s: the 2 Hz LFO peak
        s.at (te, [kind, c] (Session& ss)
        {
            for (int m = 0; m < 16; ++m)
            {
                const juce::String b = "mod" + juce::String (m + 1) + "_";
                switch (kind)
                {
                    case kAdd:       setP (*ss.p, b + "source", (float) c.src); break;
                    case kSrcNone:   setP (*ss.p, b + "source", 0.0f); break;
                    case kDestNone:  setP (*ss.p, b + "dest", 0.0f); break;
                    case kDepth0:    setP (*ss.p, b + "depth", 0.0f); break;
                    case kDepthStep: setP (*ss.p, b + "depth", -60.0f); break;
                    case kDestSwap:  setP (*ss.p, b + "dest", (float) groupDest ((m + 1) % 16)); break;
                    case kSrcSwap:   setP (*ss.p, b + "source", (float) c.src2); break;
                    default: break;
                }
            }
        });
        RtTrace tr;
        tr.rampCells = s.eng().getRampCellsModForTest();
        const bool pv = ogi::GiSrc::isPerVoice (c.src) || c.params.size() > 2;
        s.afterBlock = [&tr, pv, c] (Session& ss)
        {
            std::array<float, ogi::kGiModDests> d {};
            for (int i = 1; i < ogi::kGiModDests; ++i) d[(size_t) i] = ss.eng().getDestTelemetry (i);
            tr.dest.push_back (d);
            const auto& m = ss.eng().getVoiceModForTest (0);
            tr.src.push_back ({ pv ? m.src[c.src] : ss.eng().getSrcGForTest (c.src), pv ? m.src[c.src2] : ss.eng().getSrcGForTest (c.src2), 0.0f });
        };
        s.run (48000, 32);
        return tr;
    }

    struct RtVerdict { double worst = 0.0; int violations = 0, crossTalk = 0; };

    RtVerdict rtCheck (const RtTrace& tr, int kind)
    {
        RtVerdict v;
        const int eCell = 30000 / 32 + 1;   // the tick that latches the edit
        const size_t n = tr.dest.size();
        auto dsrc = [&] (int which, size_t t) { return t >= 1 && t < n ? std::abs ((double) tr.src[t][(size_t) which] - (double) tr.src[t - 1][(size_t) which]) : 0.0; };
        const double K = (kind == kDepthStep || kind == kDestSwap || kind == kSrcSwap) ? 1.2 : 0.6;
        for (int d = 1; d < ogi::kGiModDests; ++d)
        {
            bool inGroup = false;
            for (int m = 0; m < 16; ++m) inGroup = inGroup || d == groupDest (m);
            for (size_t t = 2; t < n; ++t)
            {
                const double dv = std::abs ((double) tr.dest[t][(size_t) d] - (double) tr.dest[t - 1][(size_t) d]);
                if (! inGroup) { if (dv != 0.0) ++v.crossTalk; continue; }
                const bool win = (int) t >= eCell - 1 && (int) t <= eCell + tr.rampCells + 2;
                double ms = 0.0;
                for (size_t u = t - 1; u <= t + 1; ++u)
                {
                    ms = std::max (ms, dsrc (0, u));
                    if (kind == kSrcSwap) ms = std::max (ms, dsrc (1, u));
                }
                const double slots = win && (kind == kDestSwap || kind == kSrcSwap) ? 2.0 : 1.0;
                const double B = slots * 0.6 * ms + (win ? K / (double) tr.rampCells : 0.0);
                if (dv > 1.05 * B + 1e-6) ++v.violations;
                if (dv > 2e-6) v.worst = std::max (v.worst, B > 1e-9 ? dv / B : 1e9);
            }
        }
        return v;
    }

    void rowsRT()
    {
        const auto cs = classes();
        for (int k = 0; k < kKinds; ++k)
        {
            int viol = 0, xt = 0;
            juce::String det;
            for (const auto& c : cs)
            {
                const auto v = rtCheck (rtRender (c, k, false, false), k);
                viol += v.violations; xt += v.crossTalk;
                det << c.name << " " << f2 (v.worst, 2) << "x (" << v.violations << "/" << v.crossTalk << "); ";
            }
            row (viol == 0 && xt == 0, juce::String ("RT") + (char) ('a' + k) + " QUAL-01 route-edit trajectory: " + kindName (k) + " x 3 source classes, 16 dests",
                 "meas worst |dv01| / B: " + det + "violations " + juce::String (viol) + ", cross-talk " + juce::String (xt)
                     + " | thr <= 1.05 x B + 1e-6, 0 cross-talk (parent MX3 bound)");
        }
        const auto c0 = cs[0];
        const auto a = rtCheck (rtRender (c0, kDestNone, true, false), kDestNone);
        const auto b = rtCheck (rtRender (c0, kAdd, false, true), kAdd);
        const bool fired = a.worst > 2.0 && b.worst > 2.0;
        row (fired, "RTn QUAL-01 trajectory controls: noResidual (dest->None), noDepthRamp (add) exceed 2x the bound",
             "meas noResidual worst " + f2 (a.worst, 1) + "x (" + juce::String (a.violations) + " violations), noDepthRamp " + f2 (b.worst, 1)
                 + "x (" + juce::String (b.violations) + ") | thr > 2x | " + ctrl (fired, "both controls exceed 2x"));
    }

    //==========================================================================
    // RE — route-edit audio kappa vs held references (local kappa, 2.4 S6)
    inline double gReHz = 110.0;   // RE material (S6 probe)
    Out reRender (const Cls& c, int kind, int phase /* 0 edit, 1 pre held, 2 post held */, bool noRamp)
    {
        // pre / post states of slot 1 (src, dest, depth)
        int srcPre = kind == kAdd ? 0 : c.src, destPre = dS (1, F::Level);
        float depPre = 20.0f;   // below the knee, no channel silenced (local kappa normalises per frame)
        int srcPost = srcPre, destPost = destPre;
        float depPost = depPre;
        switch (kind)
        {
            case kAdd:       srcPost = c.src; break;
            case kSrcNone:   srcPost = 0; break;
            case kDestNone:  destPost = 0; break;
            case kDepth0:    depPost = 0.0f; break;
            case kDepthStep: depPost = -20.0f; break;
            case kDestSwap:  destPost = dS (1, F::Pan); break;
            case kSrcSwap:   srcPost = c.src2; break;
            default: break;
        }
        const bool post = phase == 2;
        P p = cat ({ baseMod (1), c.params, { { sn (1, "level"), -12.0f }, { sn (1, "density"), 100.0f }, { sn (1, "size"), 100.0f } },
                     route (1, post ? srcPost : srcPre, post ? destPost : destPre, post ? depPost : depPre) });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.hk().noDepthRamp = noRamp;
        s.load (0, sineBuf (2, (int) (4.0 * kFs), gReHz, kFs, 0.1), kFs);
        s.noteOn (0, 48);
        if (phase == 0)
        {
            const juce::int64 te = 30000;
            P e = route (1, srcPost, destPost, depPost);
            s.at (te, [e] (Session& ss) { for (const auto& kv : e) setP (*ss.p, kv.first, kv.second); });
        }
        s.run (48000, 512);
        return { s.L, s.R };
    }

    /** S6 user decision 2026-11-09 (PD14 wording): the SAME-MOTION reference —
        no route; s1_level (and s1_pan for the dest swap) set at the edit sample to
        the post-edit value, so the stream's own 20 ms level ramp makes the same
        gain change. Mod Env source (constant at the edit): the matrix trajectory is
        exactly a 20 ms ramp between the same end points. */
    Out reSameMotion (int kind)
    {
        auto probe = make();
        const auto rl = eg25a::rangeOf (*probe, sn (1, "level")), rp = eg25a::rangeOf (*probe, sn (1, "pan"));
        const float e1 = 1.0f, e2 = 0.6f;   // Mod Env 1 / 2 at sustain by 0.625 s
        auto lev = [&rl] (float off) { return eg25a::applyR (rl, -12.0f, off); };
        float pre = lev (0.2f * e1), post = pre, panPost = 0.0f;
        switch (kind)
        {
            case kAdd:       pre = -12.0f; post = lev (0.2f * e1); break;
            case kSrcNone:
            case kDestNone:
            case kDepth0:    post = -12.0f; break;
            case kDepthStep: post = lev (-0.2f * e1); break;
            case kDestSwap:  post = -12.0f; panPost = eg25a::applyR (rp, 0.0f, 0.2f * e1); break;
            case kSrcSwap:   post = lev (0.2f * e2); break;
            default: break;
        }
        const auto c = classes()[2];
        P p = cat ({ baseMod (1), c.params, { { sn (1, "level"), pre }, { sn (1, "density"), 100.0f }, { sn (1, "size"), 100.0f } } });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.load (0, sineBuf (2, (int) (4.0 * kFs), gReHz, kFs, 0.1), kFs);
        s.noteOn (0, 48);
        setAt (s, 30000, sn (1, "level"), post);
        if (kind == kDestSwap) setAt (s, 30000, sn (1, "pan"), panPost);
        s.run (48000, 512);
        return { s.L, s.R };
    }

    void rowsRE()
    {
        const auto cs = classes();
        const double w0 = 30000.0 / kFs - 0.005, w1 = w0 + 0.045;
        for (int k = 0; k < kKinds; ++k)
        {
            const double ke = kappa (reRender (cs[2], k, 0, false), w0, w1);
            const double ks = kappa (reSameMotion (k), w0, w1);
            const double r = ks > 0.0 ? ke / ks : 1e9;
            juce::String info;
            for (int i = 0; i < 2; ++i)
            {
                const double ki = kappa (reRender (cs[(size_t) i], k, 0, false), w0, w1);
                const double kr = std::max (kappa (reRender (cs[(size_t) i], k, 1, false), w0, w1), kappa (reRender (cs[(size_t) i], k, 2, false), w0, w1));
                info << cs[(size_t) i].name << " " << f2 (kr > 0.0 ? ki / kr : 0.0, 2) << "x; ";
            }
            row (r <= 2.0, juce::String ("RE") + (char) ('a' + k) + " QUAL-01 route-edit click: " + kindName (k) + " on s1_level vs the same-motion 20 ms level ramp",
                 "meas kappa (Mod Env route edit) / kappa (s1_level param, same end points) " + f2 (r, 2) + "x | thr <= 2.0x | margin " + f2 (2.0 - r, 2)
                     + " | INFO vs held refs: " + info);
        }
        const double ke = kappa (reRender (cs[2], kAdd, 0, true), w0, w1);
        const double ks = kappa (reSameMotion (kAdd), w0, w1);
        const double r = ks > 0.0 ? ke / ks : 1e9;
        row (r > 2.0, "REn QUAL-01 click control: noDepthRamp add (Mod Env) vs the same-motion ramp exceeds 2.0x",
             "meas " + f2 (r, 2) + "x | thr > 2.0x | " + ctrl (r > 2.0, "noDepthRamp add -> " + f2 (r, 2) + "x"));
    }

    //==========================================================================
    // LM1 — lfo1_mode switch with voices
    struct LmOut { Out o; double worstTraj = 0.0; };
    LmOut lmRender (int modeMode /* 0 switch, 1 held global, 2 held per-voice */, bool snap)
    {
        P p = cat ({ baseMod (1), { { "lfo1_rate", 3.0f }, { "lfo1_mode", modeMode == 2 ? 1.0f : 0.0f }, { sn (1, "level"), -12.0f },
                                    { sn (1, "density"), 100.0f }, { sn (1, "size"), 100.0f } },
                     route (1, S::Lfo1, dS (1, F::Level), 50.0f) });
        Session s (kFs, p, kSeed, 32);
        shipping (s);
        s.hk().modeSwitchSnap = snap;
        s.load (0, sineBuf (2, (int) (4.0 * kFs), 110.0, kFs, 0.3), kFs);
        for (int i = 0; i < 4; ++i) s.noteOn (2400 * i, 48 + 4 * i);
        if (modeMode == 0)
        {
            setAt (s, 28000, "lfo1_mode", 1.0f);   // the global LFO at +1, the fresh per-voice LFOs at sin (0)
            setAt (s, 60000, "lfo1_mode", 0.0f);
        }
        LmOut r;
        std::vector<std::array<float, 4>> src;
        s.afterBlock = [&src] (Session& ss)
        {
            std::array<float, 4> a {};
            for (int k = 0; k < 4; ++k) a[(size_t) k] = ss.eng().getVoiceModForTest (k).src[S::Lfo1];
            src.push_back (a);
        };
        s.run (72000, 32);
        r.o = { s.L, s.R };
        const double slope = 2.0 * juce::MathConstants<double>::pi * 3.0 * 32.0 / kFs;   // max |d sine| per cell
        const double B = 2.0 * slope + 2.0 / (double) s.eng().getRampCellsModForTest();
        for (size_t t = 400; t < src.size(); ++t)   // past the note-ons
            for (int k = 0; k < 4; ++k)
            {
                const double d = std::abs ((double) src[t][(size_t) k] - (double) src[t - 1][(size_t) k]);
                r.worstTraj = std::max (r.worstTraj, d / B);
            }
        return r;
    }

    void rowsLM()
    {
        const auto sw = lmRender (0, false), g = lmRender (1, false), v = lmRender (2, false), sn2 = lmRender (0, true);
        double worst = 0.0, worstN = 0.0;
        for (double t : { 28000.0 / kFs, 60000.0 / kFs })
        {
            const double kr = std::max (kappa (g.o, t - 0.005, t + 0.04), kappa (v.o, t - 0.005, t + 0.04));
            worst = std::max (worst, kappa (sw.o, t - 0.005, t + 0.04) / std::max (kr, 1e-12));
            worstN = std::max (worstN, kappa (sn2.o, t - 0.005, t + 0.04) / std::max (kr, 1e-12));
        }
        row (worst <= 2.0 && sw.worstTraj <= 1.05, "LM1 QUAL-01 lfo1_mode Global <-> Per-voice with 4 voices (20 ms crossfade)",
             "meas kappa " + f2 (worst, 2) + "x held refs, per-voice source trajectory " + f2 (sw.worstTraj, 3) + " x (2 slope + 2 / rampCells) | thr <= 2.0x, <= 1.05");
        row (worstN > 2.0 || sn2.worstTraj > 1.05, "LM1n QUAL-01 mode-switch control: modeSwitchSnap exceeds 2.0x",
             "meas kappa " + f2 (worstN, 2) + "x, trajectory " + f2 (sn2.worstTraj, 2) + " | thr > 2.0x | " + ctrl (worstN > 2.0 || sn2.worstTraj > 1.05, "the snapped switch"));
    }

    //==========================================================================
    // TS / TT — Tempo
    struct TsLog { std::vector<std::pair<juce::int64, double>> grid; int bursts = 0; std::vector<ogi::GrainLogEntry> all; };

    TsLog tsRender (double fs, int div, Tp tp, double seconds, int block, P extra, std::function<void (Session&)> hk = {}, juce::int64 noteAt = 1000)
    {
        P p = cat ({ baseMod (1), { { "sync_mode", 1.0f }, { sn (1, "density_sync"), (float) div }, { sn (1, "size_sync"), 4.0f },
                                    { sn (1, "jitter"), 0.0f } }, extra });
        Session s (fs, p, kSeed, block > 0 ? block : 512);
        shipping (s);
        if (hk) hk (s);
        s.load (0, noiseBuf (2, (int) (4.0 * 48000.0), 0.2f, 7), 48000.0);
        tp.fs = fs;
        tp.pos = &s.pos;
        TpHead head (tp);
            s.p->setPlayHead (&head);
        tp.cut (s);
        s.noteOn (noteAt, 60);
        s.run ((juce::int64) (seconds * fs), block);
        s.p->setPlayHead (nullptr);
        TsLog o;
        o.all = s.hk().logInOrder();
        for (const auto& e : o.all)
        {
            if (e.stream != 0) continue;
            if ((e.counterIdx & ogi::kBurstBit) != 0) { ++o.bursts; continue; }
            o.grid.push_back ({ e.onsetSample, e.frac });
        }
        return o;
    }

    /** |onset − nearest grid slot| in samples (segments of the mock); boundary slots → −1. */
    double gridErr (const Tp& tp, double div, juce::int64 smp, double& slotOut)
    {
        const double fs = tp.fs;
        double s0 = 0.0, q0 = tp.ppq0, pps = tp.bpm / 60.0 / fs, s1 = 1e18;
        if (tp.stepAt >= 0 && (double) smp >= (double) tp.stepAt) { s0 = (double) tp.stepAt; q0 = tp.ppqAt (s0); pps = tp.bpm2 / 60.0 / fs; }
        else if (tp.stepAt >= 0) s1 = (double) tp.stepAt;
        if (tp.loopAt >= 0 && (double) smp >= (double) tp.loopAt) { s0 = std::max (s0, (double) tp.loopAt); q0 = tp.ppqAt (s0); s1 = 1e18; }
        else if (tp.loopAt >= 0) s1 = std::min (s1, (double) tp.loopAt);
        const double q = q0 + ((double) smp - s0) * pps;
        const double slot = std::round (q / div);
        slotOut = slot;
        const double tq = s0 + (slot * div - q0) / pps;
        if (tq < s0 + 1.0 || tq > s1 - 1.0) return -1.0;
        return std::abs ((double) smp - tq);
    }

    void rowsTS()
    {
        // TS1
        {
            int runs = 0, onsets = 0, bad = 0, boundary = 0, bursts = 0, missing = 0;
            double worst = 0.0;
            for (double fs : { 44100.0, 48000.0, 96000.0 })
                for (int div : { 7, 9, 14, 18 })
                    for (double bpm : { 120.0, 133.0, 97.3 })
                    {
                        Tp tp; tp.bpm = tp.bpm2 = bpm;
                        // R45: the effective division (a slow division is clamped to >= 0.5 g/s by 2^e)
                        const double d0 = (bpm / 60.0) / ogi::tempoDivBeats (div, 4.0);
                        const double eR45 = std::clamp (0.0, std::ceil (std::log2 (0.5 / d0)), std::floor (std::log2 (200.0 / d0)));
                        const double dq = ogi::tempoDivBeats (div, 4.0) * std::exp2 (-eR45);
                        const auto o = tsRender (fs, div, tp, div == 18 ? 8.0 : 3.0, 512, {});
                        tp.fs = fs;
                        ++runs; bursts += o.bursts;
                        double prevSlot = -1e18;
                        for (const auto& [smp, fr] : o.grid)
                        {
                            double slot = 0.0;
                            const double e = gridErr (tp, dq, smp, slot);
                            if (e < 0.0) { ++boundary; continue; }
                            ++onsets;
                            worst = std::max (worst, e);
                            bad += e <= 1.0 ? 0 : 1;
                            if (prevSlot > -1e17 && slot - prevSlot > 1.5) missing += (int) (slot - prevSlot - 1.0 + 0.5);
                            prevSlot = slot;
                        }
                    }
            // + bpm step and loop jump (48 k, 1/16)
            Tp tp; tp.bpm = 120.0; tp.stepAt = 72000; tp.bpm2 = 133.0; tp.loopAt = 144000; tp.loopBack = 4.0; tp.fs = 48000.0;
            const auto o = tsRender (48000.0, 7, tp, 5.0, 512, {});
            ++runs; bursts += o.bursts;
            for (const auto& [smp, fr] : o.grid)
            {
                double slot = 0.0;
                const double e = gridErr (tp, 0.25, smp, slot);
                if (e < 0.0) { ++boundary; continue; }
                ++onsets; worst = std::max (worst, e); bad += e <= 1.0 ? 0 : 1;
            }
            row (bad == 0 && onsets > 400 && missing == 0 && bursts == runs, "TS1 Tempo onsets within 1 sample of the bar grid (44.1 / 48 / 96 k, 4 divs, 3 tempi, step + loop)",
                 "meas " + juce::String (onsets) + " grid onsets over " + juce::String (runs) + " runs, max |onset - grid| " + f2 (worst, 4)
                     + " samples, missing slots " + juce::String (missing) + ", boundary excluded " + juce::String (boundary) + ", bursts " + juce::String (bursts)
                     + " (1 per run) | thr <= 1 sample, 0 missing");
        }
        // TS1n: ceilPerTick @ 133 bpm 1/16 T
        {
            auto miss = [] (bool ceil)
            {
                Tp tp; tp.bpm = tp.bpm2 = 133.0; tp.fs = 48000.0;
                const auto o = tsRender (48000.0, 6, tp, 4.0, 512, {}, [ceil] (Session& s) { s.hk().ceilPerTick = ceil; });
                int m = 0; double prev = -1e18;
                for (const auto& [smp, fr] : o.grid)
                {
                    double slot = 0.0;
                    if (gridErr (tp, ogi::tempoDivBeats (6, 4.0), smp, slot) < 0.0) continue;
                    if (prev > -1e17 && slot - prev > 1.5) m += (int) (slot - prev - 1.0 + 0.5);
                    if (prev > -1e17 && slot - prev < 0.5) ++m;   // a doubled slot
                    prev = slot;
                }
                return m;
            };
            const int m0 = miss (false), m1 = miss (true);
            row (m0 == 0 && m1 > 0, "TS1n Tempo slot control: ceilPerTick @ 133 bpm 1/16 T drops / doubles slots (P12)",
                 "meas held slot: " + juce::String (m0) + " slots dropped | thr 0 | " + ctrl (m1 > 0, "ceilPerTick -> " + juce::String (m1) + " dropped / doubled"));
        }
        // TS2 / TS2n: thinning on the division
        {
            auto run = [] (bool cont)
            {
                Tp tp; tp.bpm = tp.bpm2 = 120.0; tp.fs = 48000.0;
                P ex { { "grain_limit", 0.0f }, { sn (1, "size_sync"), 13.0f } };
                Session* sp = nullptr;
                juce::ignoreUnused (sp);
                P p = cat ({ baseMod (1), { { "sync_mode", 1.0f }, { sn (1, "density_sync"), 4.0f }, { sn (1, "jitter"), 0.0f } }, ex });
                Session s (48000.0, p, kSeed, 512);
                shipping (s);
                s.hk().tempoContinuousThin = cont;
                s.load (0, noiseBuf (2, (int) (4.0 * 48000.0), 0.2f, 7), 48000.0);
                tp.pos = &s.pos;
                TpHead head (tp);
            s.p->setPlayHead (&head);
                for (int k = 0; k < 16; ++k) s.noteOn (1000 + 37 * k, 40 + k);
                s.run (4 * 48000, 512);
                s.p->setPlayHead (nullptr);
                const double I = ogi::tempoDivBeats (4, 4.0) / (120.0 / 60.0 / 48000.0);
                int off = 0, n = 0, badI = 0;
                std::map<std::uint64_t, juce::int64> last;
                for (const auto& e : s.hk().logInOrder())
                {
                    if (e.stream != 0 || (e.counterIdx & ogi::kBurstBit) != 0 || e.onsetSample < 9600) continue;
                    double slot = 0.0;
                    const double err = gridErr (tp, ogi::tempoDivBeats (4, 4.0), e.onsetSample, slot);
                    ++n;
                    off += err >= 0.0 && err <= 1.0 ? 0 : 1;
                    auto it = last.find (e.serial);
                    if (it != last.end())
                    {
                        const double j = std::log2 ((double) (e.onsetSample - it->second) / I);
                        badI += std::abs (j - std::round (j)) < 1e-3 * 0 + std::log2 (1.0 + 2.0 / I) ? 0 : 1;
                    }
                    last[e.serial] = e.onsetSample;
                }
                return std::make_tuple (off, n, badI, s.hk().quotaRecomputes);
            };
            const auto [off, n, badI, qr] = run (false);
            const auto [offC, nC, badC, qrC] = run (true);
            juce::ignoreUnused (qr, qrC, nC, badC);
            row (off == 0 && badI == 0 && n > 100, "TS2 Tempo thinning (cap 32, 16 voices): every onset on the base lattice, intervals I x 2^j",
                 "meas " + juce::String (n) + " onsets, off-lattice " + juce::String (off) + ", non-power-of-two intervals " + juce::String (badI) + " | thr 0, 0");
            row (offC > 0, "TS2n Tempo thinning control: tempoContinuousThin -> off-grid onsets (P11)",
                 "meas " + juce::String (offC) + " off-lattice onsets | thr > 0 | " + ctrl (offC > 0, "continuous thinned period"));
        }
        // TS3: Free control
        {
            P p = cat ({ baseMod (1), { { sn (1, "density"), 8.0f }, { sn (1, "jitter"), 0.0f } } });
            Session s (48000.0, p, kSeed, 512);
            shipping (s);
            loadN (s, 1);
            Tp tp; tp.fs = 48000.0; tp.pos = &s.pos;
            TpHead head (tp);
            s.p->setPlayHead (&head);
            s.noteOn (1000, 60);
            s.run (4 * 48000, 512);
            s.p->setPlayHead (nullptr);
            int on = 0, n = 0;
            for (const auto& e : s.hk().logInOrder())
            {
                if (e.stream != 0) continue;
                double slot = 0.0;
                const double err = gridErr (tp, 0.25, e.onsetSample, slot);
                ++n;
                on += (err >= 0.0 && err <= 1.0) ? 1 : 0;
            }
            const double frac = n > 0 ? (double) on / n : 1.0;
            row (frac < 0.10 && n > 20, "TS3 Free-mode control: onsets do NOT lock to the grid (< 10 % within 1 sample)",
                 "meas " + juce::String (on) + "/" + juce::String (n) + " (" + f2 (100.0 * frac, 1) + " %) within 1 sample | thr < 10 %");
        }
        // TS4: global LFO Tempo value bitwise across a loop
        {
            P p = cat ({ baseMod (1), { { "sync_mode", 1.0f }, { "lfo1_shape", 2.0f }, { "lfo1_rate_sync", 13.0f }, { "lfo1_phase", 30.0f } } });
            Session s (48000.0, p, kSeed, 32);
            shipping (s);
            Tp tp; tp.fs = 48000.0; tp.loopAt = 96000; tp.loopBack = 3.0; tp.pos = &s.pos;
            TpHead head (tp);
            s.p->setPlayHead (&head);
            tp.cut (s);
            int bad = 0, n = 0;
            const float ph = eg21::rawOf (*s.p, "lfo1_phase");
            s.afterBlock = [&bad, &n, ph] (Session& ss)
            {
                const auto& tr = ss.eng().getTrackerForTest();
                const auto& A = tr.anchor();
                const double divB = ogi::tempoDivBeats (13, A.barLen);
                const double v = (tr.ppqAt (ss.pos - 32) - A.barOrigin) / divB;
                double c = std::floor (v), fr = v - c;
                if (! (fr < 1.0)) fr = 0.0;
                const double x0 = fr + (double) ph / 360.0;
                double x = x0 - std::floor (x0);
                if (! (x < 1.0)) x = 0.0;
                const float want = static_cast<float> (2.0 * x - 1.0);
                bad += juce::exactlyEqual (want, ss.eng().getSrcGForTest (S::Lfo1)) ? 0 : 1;
                ++n;
            };
            s.run (3 * 48000, 32);
            s.p->setPlayHead (nullptr);
            const auto relocks = s.eng().getTrackerForTest().getRelocks();
            row (bad == 0 && n > 4000 && relocks >= 1, "TS4 global LFO Tempo value == saw (frac ((ppq - origin) / div) + phase) bitwise, across a loop",
                 "meas " + juce::String (bad) + "/" + juce::String (n) + " ticks differ, re-locks " + i64 ((std::uint64_t) relocks) + " | thr 0");
        }
        // TS5: R45 effective values
        {
            int bad = 0, n = 0;
            juce::String det;
            for (float mac : { 0.0f, 25.0f, 50.0f, 100.0f })
            {
                P p = cat ({ baseMod (1), { { "sync_mode", 1.0f }, { "macro1", mac }, { sn (1, "density_sync"), 7.0f }, { sn (1, "size_sync"), 7.0f } },
                             route (1, S::Mac1, dS (1, F::Density), 50.0f), route (2, S::Mac1, dS (1, F::Size), 50.0f) });
                Session s (48000.0, p, kSeed, 512);
                shipping (s);
                s.run (4800, 512);
                const auto& g = s.eng().getGridParams().s[0];
                const float off = (eg21::rawOf (*s.p, "mod1_depth") / 100.0f) * (eg21::rawOf (*s.p, "macro1") / 100.0f);
                const float offS = (eg21::rawOf (*s.p, "mod2_depth") / 100.0f) * (eg21::rawOf (*s.p, "macro1") / 100.0f);
                const double d0 = (120.0 / 60.0) / 0.25;
                const double e = std::clamp (std::floor (4.0 * (double) off + 0.5), std::ceil (std::log2 (0.5 / d0)), std::floor (std::log2 (200.0 / d0)));
                const float wantD = static_cast<float> (d0 * std::exp2 (e));
                const float wantS = static_cast<float> (std::clamp (0.25 * 60000.0 / 120.0 * std::exp2 (4.0 * (double) offS), 5.0, 2000.0));
                bad += (juce::exactlyEqual (g.density, wantD) && juce::exactlyEqual (g.sizeMs, wantS)) ? 0 : 1;
                ++n;
                det << "m " << (int) mac << ": " << f2 (g.density, 2) << " g/s, " << f2 (g.sizeMs, 2) << " ms; ";
            }
            row (bad == 0, "TS5 Tempo R45 effective values: density d0 x 2^e (e = round (4 off)), size x 2^(4 off), bitwise",
                 "meas " + det + juce::String (bad) + "/" + juce::String (n) + " differ | thr 0");
        }
        // TS6: the note-on burst (PD1, USER)
        {
            auto first2 = [] (bool gridOnly)
            {
                Tp tp; tp.fs = 48000.0;
                const auto o = tsRender (48000.0, 7, tp, 1.0, 512, {}, [gridOnly] (Session& s) { s.hk().tempoGridOnly = gridOnly; }, 10000);
                std::vector<std::pair<juce::int64, bool>> v;
                for (const auto& e : o.all)
                    if (e.stream == 0 && v.size() < 2) v.push_back ({ e.onsetSample, (e.counterIdx & ogi::kBurstBit) != 0 });
                return std::make_pair (v, o.bursts);
            };
            const auto [a, nb] = first2 (false);
            const auto [c, nbc] = first2 (true);
            const bool ok = a.size() == 2 && a[0].first == 10000 && a[0].second && a[1].first == 12000 && ! a[1].second && nb == 1;
            const bool fired = ! c.empty() && c[0].first != 10000 && nbc == 0;
            row (ok && fired, "TS6 Tempo note-on: one burst onset at the note-on sample, then the grid (PD1, user 2026-11-08)",
                 "meas onsets " + (a.size() == 2 ? juce::String (a[0].first) + (a[0].second ? " (burst)" : "") + ", " + juce::String (a[1].first) : juce::String ("?"))
                     + ", bursts " + juce::String (nb) + " | thr 10000 burst, 12000 grid | " + ctrl (fired, "tempoGridOnly -> first onset " + (c.empty() ? juce::String ("-") : juce::String (c[0].first))));
        }
        // TT1: block-size invariance of the grid
        {
            Tp tp; tp.fs = 48000.0; tp.stepAt = 48000; tp.bpm2 = 97.3; tp.loopAt = 96000; tp.loopBack = 2.0;
            auto key = [&tp] (int block)
            {
                const auto o = tsRender (48000.0, 9, tp, 3.0, block, {});
                std::vector<std::pair<juce::int64, double>> v = o.grid;
                return v;
            };
            const auto ref = key (32);
            int diff = 0;
            for (int b : { 441, 512, 4096, 0 })
            {
                const auto x = key (b);
                diff += x.size() == ref.size() ? 0 : 1000;
                for (size_t i = 0; i < std::min (x.size(), ref.size()); ++i)
                    diff += (x[i].first == ref[i].first && juce::exactlyEqual (x[i].second, ref[i].second)) ? 0 : 1;
            }
            row (diff == 0 && ref.size() > 10, "TT1 Tempo grid onsets bitwise at 32 / 441 / 512 / 4096 / var (bpm step + loop)",
                 "meas " + juce::String (diff) + " onsets differ (" + juce::String ((int) ref.size()) + " per render) | thr 0");
        }
    }

    //==========================================================================
    // BN1 — every source live, 5 block sizes
    std::vector<float> bnRender (int block, std::uint64_t seed)
    {
        P p = cat ({ baseMod (2), { { "polyphony", 6.0f }, { "sync_mode", 1.0f }, { "lfo2_mode", 1.0f }, { "lfo2_shape", 4.0f }, { "lfo2_rate", 6.0f },
                                    { "lfo1_rate_sync", 13.0f }, { "menv1_attack", 40.0f }, { "menv1_sustain", 50.0f }, { "macro1", 30.0f },
                                    { sn (1, "jitter"), 30.0f }, { sn (2, "jitter"), 30.0f } },
                     route (1, S::Lfo1, dS (1, F::Level), 40.0f), route (2, S::Lfo2, dS (1, F::Size), 30.0f), route (3, S::Env1, dS (2, F::Pitch), 20.0f),
                     route (4, S::Vel, dS (1, F::Pan), 50.0f), route (5, S::Key, dS (2, F::FilterCutoff), 30.0f), route (6, S::Rand, dS (1, F::Spray), 40.0f),
                     route (7, S::Wheel, 1, 20.0f), route (8, S::Press, dS (2, F::Level), -30.0f), route (9, S::Slide, dS (1, F::Density), 30.0f),
                     route (10, S::Bend, ogi::GiDest::RevMix, 40.0f), route (11, S::Mac1, dS (2, F::Size), 40.0f), route (12, S::Vel, ogi::GiDest::Feedback, 30.0f),
                     route (13, S::Lfo2, ogi::GiDest::LfoRate1, 20.0f), route (14, S::Env1, dS (1, F::ScanRate), 30.0f), route (15, S::Mac2, dS (2, F::Pan), 50.0f),
                     route (16, S::Lfo1, dS (1, F::Density), 20.0f) });
        Session s (kFs, p, seed, block > 0 ? block : 512);
        shipping (s);
        loadN (s, 2);
        Tp tp; tp.fs = kFs; tp.stepAt = 48000; tp.bpm2 = 133.0; tp.loopAt = 96000; tp.loopBack = 4.0; tp.pos = &s.pos;
        TpHead head (tp);
            s.p->setPlayHead (&head);
        tp.cut (s);
        for (int i = 0; i < 12; ++i) { s.noteOn (3000 * i + 7, 48 + (i * 5) % 24, 40 + 7 * i); s.noteOff (3000 * i + 20000, 48 + (i * 5) % 24); }
        for (int i = 0; i < 30; ++i)
        {
            s.addMidi (2000 + 4000 * i, juce::MidiMessage::controllerEvent (1, 1, (i * 13) % 128));
            s.addMidi (2100 + 4000 * i, juce::MidiMessage::channelPressureChange (1, (i * 29) % 128));
            s.addMidi (2200 + 4000 * i, juce::MidiMessage::controllerEvent (1, 74, (i * 17) % 128));
            s.addMidi (2300 + 4000 * i, juce::MidiMessage::pitchWheel (1, (i * 997) % 16384));
        }
        setAt (s, 30000, "macro1", 80.0f);
        setAt (s, 60000, "mod4_dest", (float) dS (1, F::Position));
        setAt (s, 70000, "mod6_source", (float) S::Lfo1);
        setAt (s, 80000, "lfo1_mode", 1.0f);
        setAt (s, 110000, "mod1_depth", -40.0f);
        s.run (144000, block);
        s.p->setPlayHead (nullptr);
        return eg22::lr (s);
    }

    void rowBN1()
    {
        const auto ref = bnRender (32, kSeed);
        int worst = 0;
        juce::String det;
        for (int b : { 441, 512, 4096, 0 })
        {
            const int d = eg21::diffCount (ref, bnRender (b, kSeed));
            worst = std::max (worst, d);
            det << (b == 0 ? juce::String ("var") : juce::String (b)) << " " << d << "; ";
        }
        const int nSeed = eg21::diffCount (ref, bnRender (32, kSeed + 1));
        row (worst == 0, "BN1 PERF-03 block-size null, every source live, 16 routes + edits, Tempo step + loop (vs 32)",
             "meas differing samples " + det + "| thr 0");
        row (nSeed > 0, "BN1n PERF-03 block-size null control: seed + 1 differs",
             "meas " + juce::String (nSeed) + " differ | thr > 0 | " + ctrl (nSeed > 0, "seed + 1"));
    }

    //==========================================================================
    // FA5 — allocation gate
    void rowFA5()
    {
       #if JUCE_MAC
        using namespace eg21;
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocCount = 0;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const int liveness = allocCount;
        allocCount = 0;
        allocTrace = true;

        P p = cat ({ baseMod (4), { { "polyphony", 6.0f }, { "sync_mode", 1.0f }, { "amp_release", 50.0f } } });
        for (int m = 1; m <= 16; ++m)
            p = with (p, route (m, 1 + (m % 13), dS (1 + (m % 4), F::Position + (m % 20)), (m % 2) ? 40.0f : -40.0f));
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.record = false;
        loadN (s, 4, 3.0);
        Tp tp; tp.fs = kFs; tp.stepAt = 38400; tp.bpm2 = 97.0; tp.loopAt = 76800; tp.loopBack = 2.0; tp.pos = &s.pos;
        TpHead head (tp);
            s.p->setPlayHead (&head);
        tp.cut (s);
        s.arm = true;
        for (int i = 0; i < 24; ++i) s.noteOn (1000 + 3000 * i, 40 + (i * 7) % 30, 20 + i * 4);
        for (int i = 0; i < 200; ++i)
        {
            s.addMidi (500 + 400 * i, juce::MidiMessage::controllerEvent (1, 1 + (i % 2) * 73, (i * 31) % 128));
            s.addMidi (550 + 400 * i, juce::MidiMessage::channelPressureChange (1, (i * 11) % 128));
            s.addMidi (600 + 400 * i, juce::MidiMessage::pitchWheel (1, (i * 1237) % 16384));
        }
        for (int i = 0; i < 10; ++i) setAt (s, 4800 * (i + 1), "macro" + juce::String (1 + i % 4), (float) ((i * 37) % 100));
        setAt (s, 20000, "mod3_source", 0.0f);
        setAt (s, 25000, "mod3_source", (float) S::Lfo2);
        setAt (s, 30000, "mod5_dest", 0.0f);
        setAt (s, 35000, "mod5_dest", (float) dS (2, F::Level));
        setAt (s, 40000, "lfo1_mode", 1.0f);
        setAt (s, 50000, "lfo2_mode", 1.0f);
        setAt (s, 60000, "lfo1_mode", 0.0f);
        setAt (s, 70000, "sync_mode", 0.0f);
        setAt (s, 80000, "sync_mode", 1.0f);
        s.run (120000, 512);
        s.arm = false;
        s.p->setPlayHead (nullptr);
        malloc_logger = nullptr;
        allocTrace = false;
        const int n = allocCount;
        auto& h = s.hk();
        const bool exercised = h.handovers > 5 && h.voiceSteals > 0 && h.relocks > 0 && h.burstOnsets > 0 && h.quotaRecomputes > 100;
        row (liveness == 1 && n == 0 && exercised,
             "FA5 PERF-01 0 audio-thread allocations: route add / remove / swap, lfo mode switches, bpm step + loop, steals, 16 routes, CC storms, macros",
             "meas " + juce::String (n) + " allocs over " + i64 ((std::uint64_t) s.blocksDone) + " blocks (handovers " + i64 (h.handovers) + ", steals "
                 + i64 (h.voiceSteals) + ", relocks " + i64 (h.relocks) + ", bursts " + i64 (h.burstOnsets) + ", quota recomputes " + i64 (h.quotaRecomputes)
                 + ") | thr 0 | " + ctrl (liveness == 1, "liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (true, "FA5 PERF-01 allocation gate (macOS only)", "INFO skipped");
       #endif
    }

    //==========================================================================
    // TB3 / TB4 / ST1
    void rowsTB()
    {
        // TB3: sources busy, unrouted (no slot live).
        {
            rh::Overrides o;
            // Bend is not in the set: it moves pitch by design (voiceSemis), and the N1
            // recipe bends; bend-neutral routing is NZ1 / TB4, bend as a source SD1 / MD1.
            o.extra = { { "lfo1_mode", 1.0f }, { "lfo1_rate", 4.0f }, { "lfo2_shape", 4.0f }, { "lfo3_mode", 1.0f }, { "menv1_attack", 30.0f },
                        { "macro1", 40.0f } };
            o.events = { { 9600, { { "macro2", 70.0f } } }, { 19200, { { "macro1", 10.0f }, { "lfo2_mode", 1.0f } } }, { 38400, { { "lfo1_mode", 0.0f } } } };
            for (int i = 0; i < 20; ++i)
            {
                o.midi.push_back ({ 1000 + 3000 * i, juce::MidiMessage::controllerEvent (1, 1, (i * 13) % 128) });
                o.midi.push_back ({ 1100 + 3000 * i, juce::MidiMessage::controllerEvent (1, 74, (i * 7) % 128) });
                o.midi.push_back ({ 1200 + 3000 * i, juce::MidiMessage::channelPressureChange (1, (i * 29) % 128) });
                o.midi.push_back ({ 1300 + 3000 * i, juce::MidiMessage::aftertouchChange (1, 60, (i * 5) % 128) });
            }
            juce::StringArray ship, legacy;
            {
                rh::ScopedOverrides so (o);
                ship = rh::computeAll();
                legacy = rh::computeAllLegacy();
            }
            const int a = eg24::sameLines (eg24::toMap (ship), eg24::readTable ("renders-2.4.sha256"));
            const int b = eg24::sameLines (eg24::toMap (legacy), eg24::readTable ("renders-2.2.sha256"));
            row (a == 14 && b == 14, "TB3 true bypass with every source busy and no slot live (renders-2.4 + legacy)",
                 "meas shipping " + juce::String (a) + "/14, legacy " + juce::String (b) + "/14 identical | thr 14 / 14");
        }
        // TB4: neutral live routes on the recipes.
        {
            rh::Overrides o;
            const int srcs[5] { S::Wheel, S::Press, S::Slide, S::Mac4, S::Mac3 };   // N1 bends: Bend is not neutral there
            const int dests[16] { dS (1, F::Level), dS (1, F::Size), dS (1, F::Pitch), 1, ogi::GiDest::RevMix, ogi::GiDest::Feedback, dS (1, F::Pan),
                                  dS (1, F::Density), dS (1, F::ScanRate), ogi::GiDest::LfoRate1, dS (1, F::LoopEnd), ogi::GiDest::FbLowCut,
                                  dS (1, F::FilterCutoff), dS (1, F::Spray), dS (1, F::Window), dS (2, F::Level) };
            for (int m = 0; m < 16; ++m)
                o.extra = with (o.extra, route (m + 1, srcs[m % 5], dests[m], (m % 2) == 0 ? 100.0f : -100.0f));
            juce::StringArray ship;
            {
                rh::ScopedOverrides so (o);
                ship = rh::computeAll();
            }
            const int a = eg24::sameLines (eg24::toMap (ship), eg24::readTable ("renders-2.4.sha256"));
            row (a == 14, "TB4 neutral live routes (wheel, pressure, slide, macro 0 at +/-100) on the renders-2.4 recipes",
                 "meas " + juce::String (a) + "/14 identical | thr 14 (NZ1 control: macro 0.1 % differs)");
        }
    }

    void rowST1()
    {
        const juce::String pat = "setValueNotifyingHost|beginChangeGesture|setValue\\s*\\(|replaceState";
        const std::regex re (pat.toStdString());
        auto count = [&re] (const juce::File& f)
        {
            int n = 0;
            juce::StringArray lines;
            f.readLines (lines);
            for (auto l : lines)
            {
                const auto t = l.trimStart();
                if (t.startsWith ("//") || t.startsWith ("*") || t.startsWith ("/*")) continue;
                n += std::regex_search (l.toStdString(), re) ? 1 : 0;
            }
            return n;
        };
        const auto plug = gGoldenDir.getParentDirectory().getParentDirectory();
        int nDsp = 0, files = 0;
        for (const auto& f : plug.getChildFile ("Source/dsp").findChildFiles (juce::File::findFiles, true, "*.h;*.cpp"))
        {
            nDsp += count (f);
            ++files;
        }
        const int nProc = count (plug.getChildFile ("Source/PluginProcessor.cpp"));
        row (nDsp == 0 && files > 20 && nProc > 0, "AP1 PD16 the engine never writes APVTS (static scan of Source/dsp)",
             "meas " + juce::String (nDsp) + " writes in " + juce::String (files) + " files | thr 0 | " + ctrl (nProc > 0, "the same scan on PluginProcessor.cpp -> " + juce::String (nProc) + " (the state path)"));
    }

    //==========================================================================
    void runAll()
    {
        std::printf ("\n== Phase 2.5a engine gates (eg25b): route edits, mode switch, Tempo, invariance, bypass\n");
        if (want ("RT")) { section ("RT. route-edit trajectory"); rowsRT(); }
        if (want ("RE")) { section ("RE. route-edit clicks"); rowsRE(); }
        if (want ("LM")) { section ("LM. lfo mode switch"); rowsLM(); }
        if (want ("TS")) { section ("TS / TT. Tempo"); rowsTS(); }
        if (want ("BN")) { section ("BN. block-size null"); rowBN1(); }
        if (want ("FA5")) { section ("FA5. allocations"); rowFA5(); }
        if (want ("TB")) { section ("TB3 / TB4. true bypass"); rowsTB(); }
        if (want ("ST")) { section ("ST1. static"); rowST1(); }
    }

    //==========================================================================
    // --cpu25a (PD18)
    P c25mRoutes()
    {
        P p { { "lfo1_mode", 1.0f }, { "lfo2_mode", 1.0f }, { "lfo3_mode", 1.0f }, { "lfo4_mode", 1.0f },
              { "lfo1_rate", 1.3f }, { "lfo2_rate", 2.1f }, { "lfo3_rate", 3.7f }, { "lfo4_rate", 5.3f },
              { "menv1_attack", 5.0f }, { "menv1_decay", 300.0f }, { "menv1_sustain", 50.0f },
              { "menv2_attack", 5.0f }, { "menv2_decay", 300.0f }, { "menv2_sustain", 50.0f } };
        int m = 1;
        for (int s = 1; s <= 4; ++s) p = with (p, route (m++, S::Lfo1 + (s - 1), dS (s, F::Density), 30.0f));
        for (int s = 1; s <= 4; ++s) p = with (p, route (m++, s <= 2 ? S::Env1 : S::Env2, dS (s, F::Level), 30.0f));
        p = with (p, route (m++, S::Lfo2, dS (1, F::Pan), 30.0f));
        p = with (p, route (m++, S::Lfo3, dS (2, F::Pan), 30.0f));
        p = with (p, route (m++, S::Lfo1, dS (1, F::Pitch), 30.0f));
        p = with (p, route (m++, S::Lfo4, dS (3, F::Pitch), 30.0f));
        p = with (p, route (m++, S::Env1, dS (2, F::FilterCutoff), 30.0f));
        p = with (p, route (m++, S::Env2, dS (4, F::FilterCutoff), 30.0f));
        p = with (p, route (m++, S::Lfo3, dS (1, F::Spray), 30.0f));
        p = with (p, route (m++, S::Lfo2, dS (2, F::ScanRate), 30.0f));
        return p;
    }

    int cpu25a()
    {
        constexpr int kBlock = 128;
        const double refNs = cal::readRef (gGoldenDir);
        std::printf ("== --cpu25a (PLAN-2.5a T18, D28): ref_ns %.0f (tests/golden/cpu-calib-2.3.txt)\n", refNs);
        std::vector<int> n16;
        for (int k = 0; k < 16; ++k) n16.push_back (40 + k);

        const auto w = eg24::measure25 ([] { return eg24::c25Session (kFs, kBlock, true, eg23::worstFx(), eg24::c25Extra()); }, n16, kFs, kBlock, refNs, true);
        std::printf ("  INFO %s (C25w re-run, same session)\n", eg23::cpuLine ("C25w", w, refNs).toRawUTF8());
        std::fflush (stdout);

        auto mSession = [] { return eg24::c25Session (kFs, kBlock, true, eg23::worstFx(), cat ({ eg24::c25Extra(), c25mRoutes() })); };
        const auto m = eg24::measure25 (mSession, n16, kFs, kBlock, refNs, true);
        // Witnesses from an identical untimed run (deterministic stimulus).
        int pvMin = 0, pairMin = 0;
        double recompFrac = 0.0;
        {
            auto s = mSession();
            s->record = false;
            for (int n : n16) s->noteOn (0, n, 100);
            s->run (64, kBlock);
            s->hk().resetCounters();
            const auto t0 = s->hk().gridTicks;
            s->run ((juce::int64) (2.0 * kFs), kBlock);
            pvMin = s->hk().routedPvDestsMin;
            pairMin = s->hk().pairModeStreamsMin;
            recompFrac = (double) s->hk().quotaRecomputes / std::max<std::uint64_t> (1, s->hk().gridTicks - t0);
        }
        const bool mValid = ! m.slowed && ! m.contended && m.live >= 240.0 && pvMin >= 16 && recompFrac >= 0.9 && pairMin >= 4;
        const bool mPass = mValid && m.pct <= 40.0;
        row (mPass, "C25m D28 final PERF-02 worst case: C25w + 16 per-voice routes (4 PV LFOs + 2 Mod Envs, 16 voices) <= 40 %",
             "meas " + eg23::cpuLine ("C25m", m, refNs) + " | witnesses: routed PV dests min " + juce::String (pvMin) + " / tick, quota recomputes "
                 + f2 (100.0 * recompFrac, 1) + " % of ticks, pair-mode streams min " + juce::String (pairMin) + " | thr <= 40 %, live >= 240, >= 16, >= 90 %, 4"
                 + " | margin " + f2 (40.0 - m.pct, 2) + " points | delta vs C25w " + f2 (m.pct - w.pct, 2) + (mValid ? "" : " | NOVERDICT / INVALID = FAIL"));
        std::fflush (stdout);

        auto tBase = []
        {
            return cat ({ base4 (2), { { sn (1, "density"), 40.0f }, { sn (2, "density"), 40.0f }, { sn (1, "size"), 150.0f }, { sn (2, "size"), 150.0f },
                                       { sn (1, "spray"), 10.0f }, { sn (2, "spray"), 10.0f }, { sn (1, "jitter"), 20.0f }, { sn (2, "jitter"), 20.0f },
                                       { sn (1, "stereo_spread"), 30.0f }, { sn (2, "stereo_spread"), 30.0f },
                                       { "grain_limit", 3.0f }, { "polyphony", 8.0f },
                                       { sn (1, "filter_type"), 1.0f }, { sn (1, "filter_cutoff"), 1200.0f }, { sn (1, "filter_res"), 40.0f },
                                       { sn (1, "drive"), 30.0f } } });
        };
        auto tSession = [tBase] (bool routes)
        {
            P p = tBase();
            if (routes)
                p = cat ({ p, { { "lfo1_rate", 0.7f }, { "macro1", 40.0f }, { "menv1_attack", 50.0f }, { "menv1_sustain", 60.0f } },
                           route (1, S::Lfo1, dS (1, F::Position), 20.0f), route (2, S::Vel, dS (1, F::Level), 20.0f),
                           route (3, S::Env1, dS (1, F::Size), 5.0f) /* stimulus fix 2026-11-09: +20 put live at 113 (INVALID, PD18) */, route (4, S::Mac1, dS (1, F::FilterCutoff), 20.0f) });
            auto s = std::make_unique<Session> (kFs, p, kSeed, 128);
            shipping (*s);
            for (int k = 0; k < 4; ++k)
                s->load (k, noiseBuf (2, (int) (10.0 * kFs), 0.1f, 11 + k), kFs);
            s->record = false;
            return s;
        };
        const std::vector<int> n6 { 48, 52, 55, 59, 62, 64 };
        const auto tm = eg24::measure25 ([tSession] { return tSession (true); }, n6, kFs, kBlock, refNs, false);
        const bool tValid = ! tm.slowed && ! tm.contended && tm.live >= 60.0 && tm.live <= 90.0;
        const bool tPass = tValid && tm.pct <= 12.0;
        row (tPass, "C25tm D28 typical proxy + 4 routes (LFO -> position, vel -> level, Mod Env -> size, macro -> cutoff) <= 12 %",
             "meas " + eg23::cpuLine ("C25tm", tm, refNs) + " | thr <= 12 %, live in [60, 90] | margin " + f2 (12.0 - tm.pct, 2) + " points"
                 + (tValid ? "" : " | INVALID / NOVERDICT = FAIL"));
        const auto t = eg24::measure25 ([tSession] { return tSession (false); }, n6, kFs, kBlock, refNs, false);
        std::printf ("  INFO %s, delta C25tm - C25t %+.2f points\n", eg23::cpuLine ("C25t", t, refNs).toRawUTF8(), tm.pct - t.pct);
        std::printf ("  INFO delta C25m - C25w = %+.2f points (projection +2.5..5.5, C25m ~ 29-32 %%)\n", m.pct - w.pct);
        std::fflush (stdout);
        return (mPass ? 0 : 1) + (tPass ? 0 : 1);
    }
}} // namespace ::eg25b
