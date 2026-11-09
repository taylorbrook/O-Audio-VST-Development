/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.2 engine gates
    (PLAN-2.2 T9, the non-parity rows). Included by main.cpp AFTER
    render_hashes.h (uses eg21::Session / helpers and rh::computeAll).

    Every row prints: ID, measured value, threshold, margin and its negative
    control ("ctrl FIRED: ..." / "ctrl NOT FIRED (vacuous): ..."); a row whose
    control does not fire FAILS.

      RB1  7 -pinned hashes == renders-2.1.sha256          ctrl -default jobs differ
      W0a  window 0 == 2.1 (P2 + N1 pinned)                 ctrl window step 1 -> P2 differs
      W0b  P2 + forceBankPath                               (is the control: hash != 2.1)
      W0c  law at window 0 == 2.1 voiceLawGain (unit+engine) ctrl step 1 law differs
      QS1  live <= cap every tick, 80 pairs, 0 R3 kills     ctrl thinningOff
      QS2  stream-axis starvation                           ctrl periodThinOff + overQuotaOff
      QS3  voice-axis starvation, 4 streams                 ctrl as Q2
      QS4  cap lowered live, 4 streams                      ctrl thinningOff + d25Off
      QS5  asymmetric fill (400, 1, 1, 1) cap 64            ctrl noSort
      QS6  waterFillN<80> == <20>                           ctrl s2 demand 1e-9
      QS7  mute vs disable                                  ctrl muted predicate on the disabled render
      LF1  level flat over density x size (PD11)            ctrl lawOff
      CO1  coherence (b), noise                             ctrl coherenceVerbatim
      CO2  coherence (b), coherent scan                     ctrl coherenceOff
      MX1  enable / mute / solo click (same-motion ref)     ctrl mixerRampZero
      V2s  steal click, 4 streams                           ctrl stealRampZero
      FS1  FUNC-01/02 stream arm                            ctrl s3 disabled
      FS2  release continues per stream (DSP-02)            ctrl release 0
      SC2  masks per stream / collection (FUNC-09)          ctrl mask on s1 instead
      ST1-ST6 scale state codec / publish (D17, PD3, PD4)   ctrls per row
      A2   0 audio-thread allocations, 4 streams + scale    ctrl liveness malloc(64)
      NB1  block-size null, all randomisation               ctrl seed + 1
      V3s  slot invariance, 4 streams                       ctrl seed + 1
      N2s  108 unread params bit-identical, 4 streams       ctrl s1_density / s4_window (readSet23, PLAN-2.3 PD10)
    cpu22(): PLAN-2.2 T13 C22 recipe, INFO only (--cpu22).

  ==============================================================================
*/
#pragma once

#include <time.h>   // clock_gettime (CLOCK_THREAD_CPUTIME_ID)
#include <map>
#include <set>
#include <tuple>

namespace { namespace eg22
{
    using eg21::Session;
    using eg21::P;
    using eg21::Log;
    using eg21::with;
    using eg21::setP;
    using eg21::rawOf;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::dB;
    using eg21::sameD;
    using eg21::sameF;
    using eg21::noiseBuf;
    using eg21::sineBuf;
    using eg21::d2Peak;
    using eg21::rmsSt;
    using eg21::ceilCell;
    using eg21::diffCount;
    using eg21::kSeed;
    using eg21::kClickRatio;

    constexpr double kFs = 48000.0;
    using Proc = GranulatorInstrumentProcessor;

    juce::String sn (int n, const char* x) { return Ids::stream (n, x); }

    /** The pinned stream baseline (eg21::base()'s s1 rows + pins22) for stream n. */
    P streamRows (int n, bool on)
    {
        P p { { sn (n, "enable"), on ? 1.0f : 0.0f }, { sn (n, "mute"), 0.0f }, { sn (n, "solo"), 0.0f },
              { sn (n, "level"), 0.0f }, { sn (n, "pan"), 0.0f }, { sn (n, "position"), 0.0f },
              { sn (n, "scan_rate"), 0.0f }, { sn (n, "scan_mode"), 1.0f }, { sn (n, "loop_start"), 0.0f },
              { sn (n, "loop_end"), 100.0f }, { sn (n, "root_note"), 48.0f }, { sn (n, "root_fine"), 0.0f },
              { sn (n, "key_track"), 100.0f }, { sn (n, "size"), 100.0f }, { sn (n, "size_rand"), 0.0f },
              { sn (n, "density"), 10.0f }, { sn (n, "pitch"), 0.0f } };
        for (const char* x : { "spray", "jitter", "stereo_spread", "gain_rand", "pitch_rand", "reverse_prob",
                               "window", "window_skew", "scale" })
            p.push_back ({ sn (n, x), 0.0f });
        return p;
    }

    /** eg21::base() (pinned) with all 4 streams pinned; streams 1..nOn enabled. */
    P base4 (int nOn = 4)
    {
        P b = eg21::base();
        for (int n = 1; n <= 4; ++n)
            b = with (b, streamRows (n, n <= nOn));
        return b;
    }

    /** One row value on every stream. */
    P all4 (const char* x, float v)
    {
        P p;
        for (int n = 1; n <= 4; ++n)
            p.push_back ({ sn (n, x), v });
        return p;
    }

    P cat (std::initializer_list<P> ps)
    {
        P out;
        for (const auto& p : ps)
            out = with (out, p);
        return out;
    }

    /** Shipping law: coherence on (eg21's Session pins c := 0). */
    void shipping (Session& s) { s.hk().coherenceOff = false; }

    void loadNoise4 (Session& s, double seconds, float amp)
    {
        for (int k = 0; k < 4; ++k)
            s.load (k, noiseBuf (2, (int) (seconds * kFs), amp, 11 + k), kFs);
    }

    void loadSine4 (Session& s, double seconds, const double (&hz)[4], double amp)
    {
        for (int k = 0; k < 4; ++k)
            s.load (k, sineBuf (1, (int) (seconds * kFs), hz[k], kFs, amp), kFs);
    }

    void setAt (Session& s, juce::int64 t, const juce::String& id, float v)
    {
        s.at (t, [id, v] (Session& ss) { setP (*ss.p, id, v); });
    }

    int countLog (const Log& log, int stream, juce::int64 a, juce::int64 b)
    {
        int n = 0;
        for (const auto& e : log)
            n += (e.stream == stream && e.onsetSample >= a && e.onsetSample < b) ? 1 : 0;
        return n;
    }

    std::vector<float> lr (const Session& s)
    {
        std::vector<float> v (s.L);
        v.insert (v.end(), s.R.begin(), s.R.end());
        return v;
    }

    int liveChildren (Proc& p)
    {
        int n = 0;
        for (int i = 0; i < p.parameters.state.getNumChildren(); ++i)
        {
            const auto t = p.parameters.state.getChild (i).getType().toString();
            n += (t == "Engine" || t == "CustomScale" || t == "DegreeMasks") ? 1 : 0;
        }
        return n;
    }

    //==========================================================================
    // RB1 / W0 — re-baseline proof and window 0 == 2.1
    juce::String p2Hash (bool forceBank, float window)
    {
        P p = rh::stim ({ { eg21::s1 ("size"), 2000.0f }, { eg21::s1 ("density"), 0.5f }, { eg21::s1 ("position"), 25.0f } }, true);
        p = with (p, { { eg21::s1 ("window"), window } });
        Session s (kFs, p);
        s.hk().coherenceOff = true;
        s.hk().voiceGainDbOverride = -6.0;
        s.hk().forceBankPath = forceBank;
        s.hk().legacyWetTanh = true;   // PLAN-2.4 PD1: W0a / W0b compare against renders-2.1 (the RB1 attribution)
        s.load (0, sineBuf (1, (int) (6.0 * kFs), 440.0, kFs, 0.5), kFs);
        s.noteOn (0, 60);
        s.run ((juce::int64) (2.05 * kFs), 512);
        return rh::sha (rh::take (s));
    }

    void rowsRB()
    {
        std::map<juce::String, juce::String> gold, now;
        {
            juce::StringArray g;
            g.addLines (gGoldenDir.getChildFile ("renders-2.1.sha256").loadFileAsString());
            for (const auto& l : g)
                if (l.trim().isNotEmpty())
                    gold[l.upToFirstOccurrenceOf (" ", false, false)] = l.fromLastOccurrenceOf (" ", false, false).trim();
        }
        for (const auto& l : rh::computeAllLegacy())   // PLAN-2.4 PD1: hashes under legacyWetTanh
            now[l.upToFirstOccurrenceOf (" ", false, false)] = l.fromLastOccurrenceOf (" ", false, false).trim();

        int pinnedOk = 0, pinnedN = 0, defDiff = 0, defN = 0;
        juce::String bad;
        for (const auto& kv : now)
        {
            const bool same = gold.count (kv.first) != 0 && gold[kv.first] == kv.second;
            if (kv.first.endsWith ("-pinned"))
            {
                ++pinnedN;
                pinnedOk += same ? 1 : 0;
                if (! same) bad << " " << kv.first;
            }
            else
            {
                ++defN;
                defDiff += same ? 0 : 1;
            }
        }
        const bool fired = defDiff >= 1;
        row (gold.size() == 14 && pinnedN == 7 && pinnedOk == 7 && fired, "RB1 re-baseline proof: 7 -pinned hashes == renders-2.1.sha256 (legacyWetTanh, PLAN-2.4 PD1)",
             "meas " + juce::String (pinnedOk) + "/7 pinned equal" + (bad.isEmpty() ? juce::String() : " (differ:" + bad + ")")
                 + " | thr 7/7 | margin " + juce::String (pinnedOk - 7)
                 + " | " + ctrl (fired, "-default jobs: " + juce::String (defDiff) + "/" + juce::String (defN) + " differ from 2.1"));

        // W0a: window 0 renders == 2.1 (P2 long grain + N1). Control: window step 1.
        const bool p2 = now["P2-pinned"] == gold["P2-pinned"], n1 = now["N1-pinned"] == gold["N1-pinned"];
        const auto replica = p2Hash (false, 0.0f);
        const auto step1   = p2Hash (false, 1.0f / 64.0f);
        const bool repOk = replica == gold["P2-pinned"];
        const bool firedA = step1 != gold["P2-pinned"];
        row (p2 && n1 && repOk && firedA, "W0a window 0 / skew 0 render == 2.1 Hann (P2 + N1 pinned)",
             juce::String ("meas P2 ") + (p2 ? "==" : "!=") + " 2.1, N1 " + (n1 ? "==" : "!=") + " 2.1, P2 replica "
                 + (repOk ? "==" : "!=") + " 2.1 | thr bitwise | margin 0"
                 + " | " + ctrl (firedA, "window step 1 -> P2 hash " + juce::String (firedA ? "differs" : "SAME")));

        // W0b: the fast path is under test — the bank read at step 0 is NOT the 2.1 render.
        const auto bank = p2Hash (true, 0.0f);
        const bool firedB = bank != gold["P2-pinned"];
        row (repOk && firedB, "W0b P2 + forceBankPath (P5: the closed-form path is what renders)",
             "meas forceBankPath hash " + juce::String (firedB ? "!=" : "==") + " 2.1 | thr != | "
                 + ctrl (firedB, "bank path at step 0 is not bitwise Hann"));
    }

    void rowW0c()
    {
        ogi::WindowMorphBank bank;
        int diff = 0, ctrlDiff = 0, engineSel = 0;
        ogi::WindowSel eng;
        eng.step = ogi::WindowMorphBank::stepFor (0.0f);
        eng.tilt = ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * 0.0f);
        engineSel = (eng.hann() && eng.step == 0) ? 1 : 0;
        ogi::WindowSel w1 = eng;
        w1.step = 1;
        for (int i = 0; i < 1000; ++i)
        {
            const double n = 400.0 * (double) i / 999.0;
            const float a = ogi::outputGainFromPower (ogi::streamPower (n, n, eng, 0.0, 0.0, bank));
            const float b = ogi::GiRenderer::voiceLawGain (n, bank);
            const float c = ogi::outputGainFromPower (ogi::streamPower (n, n, w1, 0.0, 0.0, bank));
            diff += sameF (a, b) ? 0 : 1;
            ctrlDiff += sameF (c, b) ? 0 : 1;
        }

        // Engine: every counted voice's window-0 lawTarget == voiceLawGain (N_eff) at every tick.
        Session s (kFs, with (eg21::n1Params(), {}), kSeed, 512);
        s.load (0, noiseBuf (2, 3 * 48000, 0.3f, 7), kFs);
        eg21::n1Script (s);
        s.record = false;
        int ticks = 0, engDiff = 0;
        const double lenBar = 300.0 * 0.001 * kFs;
        s.afterBlock = [&] (Session& ss)
        {
            auto& h = ss.hk();
            for (int k = 0; k < ogi::kVoiceSlots; ++k)
            {
                if (h.voiceState[k] != 1)
                    continue;
                ++ticks;
                const float want = ogi::GiRenderer::voiceLawGain (ogi::effectiveCount (lenBar, h.periodTVS[k][0]), ss.eng().getMorphBank());
                engDiff += sameF (want, h.lawTargetVS[k][0]) ? 0 : 1;
            }
        };
        s.run (24000, 32);
        const bool fired = ctrlDiff > 0;
        row (engineSel == 1 && diff == 0 && engDiff == 0 && ticks > 1000 && fired,
             "W0c law at window 0 == 2.1 voiceLawGain (1000 n in [0, 400] + engine)",
             "meas unit " + juce::String (diff) + "/1000 differ, engine " + juce::String (engDiff) + "/" + juce::String (ticks)
                 + " voice-ticks differ | thr 0 | margin " + juce::String (-(diff + engDiff))
                 + " | " + ctrl (fired, "window step 1 -> " + juce::String (ctrlDiff) + "/1000 differ"));
    }

    //==========================================================================
    // QS — 80-pair quotas, thinning, stealing
    struct QS1R { int cap = 0, liveMax = 0; std::uint64_t r3 = 0, spawns = 0; };

    QS1R runQS1 (int capIdx, float jitterPct, bool thinOff)
    {
        Session s (kFs, cat ({ base4 (4), { { "grain_limit", (float) capIdx } }, all4 ("density", 200.0f), all4 ("size", 2000.0f),
                               all4 ("jitter", jitterPct) }));
        s.hk().thinningOff = thinOff;
        s.record = false;
        loadNoise4 (s, 5.0, 0.25f);
        for (int k = 0; k < 16; ++k)
            s.noteOn (0, 48);
        s.run ((juce::int64) (2.0 * kFs) + 64, 512);   // transition: L_max + 2 cells
        QS1R r;
        r.cap = 32 << capIdx;
        r.liveMax = s.hk().liveMax;
        s.hk().resetCounters();
        s.run ((juce::int64) (2.5 * kFs), 512);         // steady state, 2.5 s window
        r.liveMax = std::max (r.liveMax, s.hk().liveMax);
        r.r3 = s.hk().r3Kills;
        r.spawns = s.hk().spawns;
        return r;
    }

    void rowQS1()
    {
        Tally live, kills;
        juce::String dl, dk, info;
        int marginLive = INT_MAX;
        for (int ci = 3; ci >= 0; --ci)
            for (float j : { 0.0f, 20.0f })
            {
                const auto r = runQS1 (ci, j, false);
                const juce::String tag = juce::String (r.cap) + "/J" + juce::String ((int) j);
                live.check (r.liveMax <= r.cap && r.liveMax >= r.cap / 2, tag + " liveMax " + juce::String (r.liveMax));
                marginLive = std::min (marginLive, r.cap - r.liveMax);
                dl << " " << tag << ":" << r.liveMax;
                const bool gated = r.cap >= 128 || (r.cap == 64 && j == 0.0f);
                if (gated)
                {
                    kills.check (r.r3 == 0 && r.spawns > 0, tag + " r3 " + i64 (r.r3));
                    dk << " " << tag << ":" << i64 (r.r3) << "/" << i64 (r.spawns);
                }
                else
                {
                    info << " " << tag << ": " << f2 ((double) r.r3 / 2.5, 1) << " kills/s (" << i64 (r.r3) << "/" << i64 (r.spawns) << " spawns)";
                }
            }
        const auto k = runQS1 (2, 0.0f, true);
        const bool fired = k.r3 > 0;
        row (live.bad == 0 && kills.bad == 0 && fired, "QS1 16 notes x 4 streams: live <= cap every tick; 0 steady R3 kills",
             "meas liveMax" + dl + " | r3/spawns" + dk + " | thr live <= cap, r3 0 (caps 256/128, 64 J0) | margin live "
                 + juce::String (marginLive) + " | " + live.detail (8) + " | " + kills.detail (5)
                 + " | " + ctrl (fired, "thinningOff cap 128 -> r3 " + i64 (k.r3)));
        std::printf ("  INFO QS1 R3 kill rate (pairs > cap or jittered m = 1, PD7 / P10):%s\n", info.toRawUTF8());
    }

    /** The water-fill of the demands the engine actually latched (raw parameter
        values: skewed density / size rows cannot land exactly on 10 / 100, so a
        nominal N = 1 is 1.0000002). Pairs (voice 0, stream s); other voices 0. */
    ogi::QuotasN<ogi::kPairs> refFill (Session& s, int cap, const bool (&on)[4])
    {
        double d[ogi::kPairs] {};
        for (int st = 0; st < 4; ++st)
            if (on[st])
                d[st] = ogi::streamDemand ((double) rawOf (*s.p, sn (st + 1, "density")), (double) rawOf (*s.p, sn (st + 1, "size")), kFs);
        return ogi::waterFillN (d, cap);
    }

    struct QS2R { double q0 = 0.0, q1 = 0.0; int s1At = 0, s2Max = 0; bool bitwise = false; };

    QS2R runQS2 (float s2Density, bool control)
    {
        Session s (kFs, cat ({ base4 (1), { { "grain_limit", 1.0f }, { sn (1, "density"), 200.0f }, { sn (1, "size"), 2000.0f },
                                            { sn (2, "density"), s2Density }, { sn (2, "size"), 100.0f } } }));
        s.hk().periodThinOff = control;
        s.hk().overQuotaOff  = control;
        s.record = false;
        s.load (0, noiseBuf (2, (int) (6.0 * kFs), 0.25f, 1), kFs);
        s.load (1, noiseBuf (2, (int) (6.0 * kFs), 0.25f, 2), kFs);
        s.noteOn (0, 48);
        const juce::int64 tB = (juce::int64) (3.0 * kFs);
        setAt (s, tB, sn (2, "enable"), 1.0f);
        const juce::int64 tCheck = ceilCell (tB + (juce::int64) (2.0 * kFs) + 32);   // L_max + 1 cell
        s.run (tB, 512);
        QS2R r;
        s.afterBlock = [&r] (Session& ss) { r.s2Max = std::max (r.s2Max, ss.hk().liveVS[0][1]); };
        s.run (tCheck - tB, 32);
        s.afterBlock = nullptr;
        s.run (32, 32);
        r.s1At = s.hk().liveVS[0][0];
        r.q0 = s.hk().quotaQVS[0][0];
        r.q1 = s.hk().quotaQVS[0][1];
        const bool on[4] { true, true, false, false };
        const auto ref = refFill (s, 64, on);
        r.bitwise = sameD (r.q0, ref.q[0]) && sameD (r.q1, ref.q[1]);
        return r;
    }

    void rowQS2()
    {
        const auto a = runQS2 (10.0f, false);   // plan stimulus: s2 N = 1 → q = (63, 1)
        const auto b = runQS2 (40.0f, false);   // s2 N = 4 → q = (60, 4)
        const auto k = runQS2 (40.0f, true);    // control: periodThinOff + overQuotaOff
        auto near = [] (double x, double v) { return std::abs (x - v) <= 1.0e-4; };
        const bool okA = a.bitwise && near (a.q0, 63.0) && near (a.q1, 1.0) && a.s2Max >= 1 && a.s1At <= 63;
        const bool okB = b.bitwise && near (b.q0, 60.0) && near (b.q1, 4.0) && b.s2Max >= 4 && b.s1At <= 60;
        const bool fired = k.s2Max < 4;
        row (okA && okB && fired, "QS2 stream-axis starvation: s1 fills cap 64, then s2 on",
             "meas N1: q (" + f2 (a.q0, 1) + ", " + f2 (a.q1, 1) + "), s2 live max " + juce::String (a.s2Max) + " >= 1, s1 "
                 + juce::String (a.s1At) + " <= 63; N4: q (" + f2 (b.q0, 1) + ", " + f2 (b.q1, 1) + "), s2 max " + juce::String (b.s2Max)
                 + " >= 4, s1 " + juce::String (b.s1At) + " <= 60; q bitwise == waterFillN (latched demands) "
                 + (a.bitwise && b.bitwise ? "yes" : "NO") + " | thr q exact, live >= m within L_max + 1 cell | margin "
                 + juce::String (std::min (a.s2Max - 1, b.s2Max - 4))
                 + " | " + ctrl (fired, "periodThinOff + overQuotaOff (N4) -> s2 max " + juce::String (k.s2Max) + " < 4"));
    }

    struct QS3R { int fillA = 0; int liveA[4] {}, liveB[4] {}, mA[4] {}, mB[4] {}; };

    QS3R runQS3 (bool overQuotaOff, bool thinOff)
    {
        Session s (kFs, cat ({ base4 (4), { { "grain_limit", 1.0f } }, all4 ("density", 200.0f), all4 ("size", 2000.0f) }));
        s.hk().overQuotaOff = overQuotaOff;
        s.hk().periodThinOff = thinOff;
        s.record = false;
        loadNoise4 (s, 6.0, 0.25f);
        const juce::int64 tB = (juce::int64) (3.0 * kFs);
        QS3R q;
        s.noteOn (0, 48);
        s.noteOn (tB, 50);
        s.at (tB, [&q] (Session& ss) { q.fillA = ss.hk().liveVoice[0]; });
        const juce::int64 tCheck = ceilCell (tB + (juce::int64) (2.0 * kFs) + 32);
        s.run (tCheck, 512);
        s.run (32, 32);
        for (int st = 0; st < 4; ++st)
        {
            q.liveA[st] = s.hk().liveVS[0][st];
            q.liveB[st] = s.hk().liveVS[1][st];
            q.mA[st] = s.hk().quotaMVS[0][st];
            q.mB[st] = s.hk().quotaMVS[1][st];
        }
        return q;
    }

    void rowQS3()
    {
        const auto t = runQS3 (false, false);
        const auto u = runQS3 (false, true);
        const auto k = runQS3 (true, true);
        bool okT = t.fillA >= 60, okU = true;
        int starved = 0, margin = INT_MAX;
        juce::String d;
        for (int st = 0; st < 4; ++st)
        {
            okT = okT && t.mB[st] > 0 && t.liveB[st] >= t.mB[st] && t.liveA[st] <= t.mA[st];
            okU = okU && u.mB[st] > 0 && u.liveB[st] >= u.mB[st] && u.liveA[st] <= u.mA[st] + 1;
            starved += k.liveB[st] < k.mB[st] ? 1 : 0;
            margin = std::min (margin, std::min (t.liveB[st] - t.mB[st], u.liveB[st] - u.mB[st]));
            d << " s" << (st + 1) << " B " << t.liveB[st] << ">=" << t.mB[st] << " A " << t.liveA[st] << "<=" << t.mA[st];
        }
        const bool fired = starved > 0;
        row (okT && okU && fired, "QS3 voice-axis starvation, 4 streams (Q2 script, cap 64)",
             "meas A fill " + juce::String (t.fillA) + ";" + d + " (unthinned " + (okU ? "ok" : "FAIL") + ") | thr B >= m_B,s, A <= m_A,s"
                 + " | margin " + juce::String (margin)
                 + " | " + ctrl (fired, "unthinned + overQuotaOff -> " + juce::String (starved) + "/4 streams B < m_B (s1 B "
                                        + juce::String (k.liveB[0]) + " vs " + juce::String (k.mB[0]) + ")"));
    }

    struct QS4R { int before = 0, after = 0; };

    QS4R runQS4 (bool thinOff, bool d25Off)
    {
        Session s (kFs, cat ({ base4 (4), { { "grain_limit", 3.0f } }, all4 ("density", 200.0f), all4 ("size", 2000.0f) }));
        s.hk().thinningOff = thinOff;
        s.hk().d25Off = d25Off;
        s.record = false;
        loadNoise4 (s, 6.0, 0.25f);
        for (int k = 0; k < 8; ++k)
            s.noteOn (0, 48);
        const juce::int64 tc = thinOff ? (juce::int64) (0.5 * kFs) : (juce::int64) (2.1 * kFs);
        QS4R q;
        s.at (tc, [&q] (Session& ss) { q.before = ss.hk().liveNow; setP (*ss.p, "grain_limit", 0.0f); });
        const juce::int64 tCheck = thinOff ? tc + (juce::int64) (0.5 * kFs)
                                           : ceilCell (tc + (juce::int64) (2.0 * kFs) + (juce::int64) (0.005 * kFs) + 32);
        s.run (tCheck, 512);
        s.run (32, 32);
        q.after = s.hk().liveNow;
        return q;
    }

    void rowQS4()
    {
        const auto a = runQS4 (false, false);
        const auto b = runQS4 (true, false);
        const auto k = runQS4 (true, true);
        const bool fired = k.after > 32;
        row (a.before > 32 && a.after <= 32 && b.before > 32 && b.after <= 32 && fired,
             "QS4 grain_limit 256 -> 32 live, 8 notes x 4 streams over-demand",
             "meas thinned " + juce::String (a.before) + " -> " + juce::String (a.after) + " @ L_max+5ms+1cell; unthinned+D25 "
                 + juce::String (b.before) + " -> " + juce::String (b.after) + " @ 0.5 s | thr <= 32 | margin "
                 + juce::String (32 - std::max (a.after, b.after))
                 + " | " + ctrl (fired, "thinningOff + d25Off -> " + juce::String (k.after) + " @ 0.5 s"));
    }

    void rowQS5()
    {
        double d[ogi::kPairs] {};
        d[0] = 400.0; d[1] = 1.0; d[2] = 1.0; d[3] = 1.0;
        const auto q = ogi::waterFillN (d, 64);
        const auto n = ogi::waterFillN (d, 64, ogi::WaterFillTest::noSort);
        const bool unit = q.q[0] == 61.0 && q.q[1] == 1.0 && q.q[2] == 1.0 && q.q[3] == 1.0 && q.capped;

        // Engine: 1 voice, s1 N 400, s2-s4 N 1, cap 64.
        Session s (kFs, cat ({ base4 (4), { { "grain_limit", 1.0f }, { sn (1, "density"), 200.0f }, { sn (1, "size"), 2000.0f } } }));
        s.record = false;
        loadNoise4 (s, 2.0, 0.25f);
        s.noteOn (0, 48);
        s.run (4096, 512);
        const auto& h = s.hk();
        const bool on[4] { true, true, true, true };
        const auto ref = refFill (s, 64, on);
        bool engine = true;
        for (int st = 0; st < 4; ++st)
            engine = engine && sameD (h.quotaQVS[0][st], ref.q[st]) && std::abs (h.quotaQVS[0][st] - (st == 0 ? 61.0 : 1.0)) <= 1.0e-4;
        const bool fired = ! (n.q[0] == q.q[0] && n.q[1] == q.q[1]);
        row (unit && engine && fired, "QS5 asymmetric fill: demands (400, 1, 1, 1), cap 64 -> q = (61, 1, 1, 1)",
             "meas unit (" + f2 (q.q[0], 3) + ", " + f2 (q.q[1], 3) + ", " + f2 (q.q[2], 3) + ", " + f2 (q.q[3], 3) + "), engine ("
                 + f2 (h.quotaQVS[0][0], 3) + ", " + f2 (h.quotaQVS[0][1], 3) + ", " + f2 (h.quotaQVS[0][2], 3) + ", "
                 + f2 (h.quotaQVS[0][3], 3) + ") bitwise == waterFillN (latched demands) " + (engine ? "yes" : "NO")
                 + " | thr exact | margin 0"
                 + " | " + ctrl (fired, "noSort -> (" + f2 (n.q[0], 3) + ", " + f2 (n.q[1], 3) + ", ...)"));
    }

    void rowQS6()
    {
        juce::Random r ((juce::int64) 0x51060001);
        constexpr int kN = 10000;
        int mism = 0, ctrlDiff = 0, capped = 0;
        for (int t = 0; t < kN; ++t)
        {
            double v[ogi::kVoiceSlots];
            for (auto& x : v)
            {
                const int mode = r.nextInt (4);
                x = mode == 0 ? 0.0 : (mode == 1 ? 12.5 * (double) r.nextInt (8) : 400.0 * r.nextDouble());
            }
            const int cap = 32 << r.nextInt (4);
            double w[ogi::kPairs] {};
            for (int k = 0; k < ogi::kVoiceSlots; ++k)
                w[k * ogi::kGiStreams] = v[k];
            const auto a = ogi::waterFillN (v, cap);
            const auto b = ogi::waterFillN (w, cap);
            auto same = [&a] (const ogi::QuotasN<ogi::kPairs>& q)
            {
                bool s = sameD (a.lambda, q.lambda) && a.capped == q.capped;
                for (int k = 0; k < ogi::kVoiceSlots; ++k)
                    s = s && sameD (a.q[k], q.q[k * ogi::kGiStreams]) && a.m[k] == q.m[k * ogi::kGiStreams];
                return s;
            };
            capped += a.capped ? 1 : 0;
            mism += same (b) ? 0 : 1;
            w[1] = 1.0e-9;   // one s2 demand
            ctrlDiff += same (ogi::waterFillN (w, cap)) ? 0 : 1;
        }
        const bool fired = ctrlDiff > 0;
        row (mism == 0 && capped > 1000 && fired, "QS6 waterFillN<80> == waterFillN<20> (s2-s4 zero), q m lambda capped",
             "meas " + juce::String (mism) + "/" + juce::String (kN) + " differ (" + juce::String (capped) + " capped) | thr 0 | margin "
                 + juce::String (-mism) + " | " + ctrl (fired, "one s2 demand 1e-9 -> " + juce::String (ctrlDiff) + " differ"));
    }

    struct QS7R
    {
        double qBefore[4] {}, qDuring[4] {};
        int spawnsDuring = 0, spawnsAfterDisable = 0;
        double peakMuted = 0.0, rmsPost = 0.0;
        std::vector<float> L, R;
    };

    /** mode 0 = reference, 1 = mute at tM / unmute at tU, 2 = disable at tM. */
    QS7R runQS7 (int mode)
    {
        constexpr juce::int64 tM = 24000, tU = 48000;
        Session s (kFs, cat ({ base4 (0), { { "grain_limit", 1.0f }, { sn (2, "enable"), 1.0f }, { sn (2, "density"), 100.0f },
                                            { sn (2, "size"), 200.0f }, { sn (2, "position"), 30.0f } } }));
        shipping (s);
        s.load (1, noiseBuf (2, (int) (3.0 * kFs), 0.1f, 5), kFs);
        for (int n : { 48, 50, 52, 55 })
            s.noteOn (0, n);
        QS7R r;
        s.at (tM, [&r] (Session& ss) { for (int k = 0; k < 4; ++k) r.qBefore[k] = ss.hk().quotaQVS[k][1]; });
        if (mode == 1) { setAt (s, tM, sn (2, "mute"), 1.0f); setAt (s, tU, sn (2, "mute"), 0.0f); }
        if (mode == 2) setAt (s, tM, sn (2, "enable"), 0.0f);
        s.at (tU, [&r] (Session& ss) { for (int k = 0; k < 4; ++k) r.qDuring[k] = ss.hk().quotaQVS[k][1]; });
        s.run (57600, 512);
        const auto log = s.hk().logInOrder();
        r.spawnsDuring = countLog (log, 1, tM + 32, tU);
        r.spawnsAfterDisable = countLog (log, 1, tM + 32, 57600);
        for (juce::int64 i = tM + 480 + 64; i < tU; ++i)
            r.peakMuted = std::max (r.peakMuted, (double) std::max (std::abs (s.L[(size_t) i]), std::abs (s.R[(size_t) i])));
        r.rmsPost = rmsSt (s.L, s.R, tU + 480, tU + 512);
        r.L = s.L;
        r.R = s.R;
        return r;
    }

    void rowQS7()
    {
        const auto ref = runQS7 (0), mu = runQS7 (1), di = runQS7 (2);
        const double refPost = ref.rmsPost;
        auto mutedOk = [refPost] (const QS7R& r, juce::String& why)
        {
            bool qSame = true;
            for (int k = 0; k < 4; ++k)
                qSame = qSame && sameD (r.qBefore[k], r.qDuring[k]) && r.qBefore[k] > 0.0;
            const double post = std::abs (dB (r.rmsPost / std::max (1.0e-30, refPost)));
            why = juce::String ("q unchanged ") + (qSame ? "yes" : "NO") + ", spawns " + juce::String (r.spawnsDuring)
                + ", muted peak " + juce::String (r.peakMuted, 9) + ", unmute cell " + f2 (post, 3) + " dB vs ref";
            return qSame && r.spawnsDuring > 0 && r.peakMuted == 0.0 && post <= 1.0;
        };
        juce::String wm, wc;
        const bool m = mutedOk (mu, wm);
        bool dq = true;
        for (int k = 0; k < 4; ++k)
            dq = dq && di.qDuring[k] == 0.0;
        const bool d = dq && di.spawnsAfterDisable == 0;
        const bool fired = ! mutedOk (di, wc);
        row (m && d && fired && refPost > 1.0e-4, "QS7 mute keeps quota + spawns (silent after 10 ms); disable drops both",
             "meas muted: " + wm + "; disabled: q 0 " + (dq ? "yes" : "NO") + ", spawns after tick " + juce::String (di.spawnsAfterDisable)
                 + " | thr bitwise q, 0 out, <= 1 dB, 0 spawns | margin 0 | " + ctrl (fired, "muted predicate on the disabled render fails (" + wc + ")"));
    }

    //==========================================================================
    // LF1 / CO — output law and coherence
    double lfCell (float density, float size, int seed, bool lawOff, double& nEff)
    {
        Session s (kFs, cat ({ base4 (1), { { "grain_limit", 0.0f }, { sn (1, "density"), density }, { sn (1, "size"), size },
                                            { sn (1, "spray"), 10.0f }, { sn (1, "position"), 50.0f } } }),
                   kSeed + (std::uint64_t) seed);
        shipping (s);
        s.hk().lawOff = lawOff;
        s.load (0, noiseBuf (2, (int) (8.0 * kFs), 0.05f, 21), kFs);
        s.noteOn (0, 48, 127);
        s.run ((juce::int64) (4.0 * kFs), 512);
        nEff = ogi::effectiveCount (std::clamp ((double) size, 5.0, 2000.0) * 0.001 * kFs, s.hk().periodTVS[0][0]);
        return rmsSt (s.L, s.R, (juce::int64) (2.0 * kFs), (juce::int64) (4.0 * kFs));
    }

    double lfMean (float density, float size, bool lawOff, double& nEff)
    {
        double p = 0.0;
        for (int seed = 0; seed < 4; ++seed)
        {
            const double r = lfCell (density, size, seed, lawOff, nEff);
            p += r * r;
        }
        return std::sqrt (p / 4.0);
    }

    void rowLF1()
    {
        double nRef = 0.0;
        const double ref = lfMean (10.0f, 100.0f, false, nRef);
        double tmp = 0.0;
        const double oldOld = dB (lfCell (10.0f, 100.0f, 0, false, tmp) / lfCell (10.0f, 100.0f, 0, false, tmp));
        juce::String d, info;
        double worst = 0.0;
        int gated = 0;
        for (float den : { 1.0f, 10.0f, 50.0f, 200.0f })
            for (float sz : { 100.0f, 500.0f, 2000.0f })
            {
                double ne = 0.0;
                const double v = dB (lfMean (den, sz, false, ne) / ref);
                const juce::String tag = "d" + juce::String ((int) den) + "/" + juce::String ((int) sz);
                if (ne >= 1.0 - 1.0e-9)
                {
                    ++gated;
                    worst = std::max (worst, std::abs (v));
                    d << " " << tag << ":" << f2 (v, 2) << "(N" << f2 (ne, 1) << ")";
                }
                else
                {
                    info << " " << tag << ":" << f2 (v, 2) << "(N" << f2 (ne, 2) << ")";
                }
            }
        // Control: lawOff -> the spread over the extreme gated cells exceeds 1.5 dB.
        double n1 = 0.0, n2 = 0.0;
        const double off1 = lfMean (10.0f, 100.0f, true, n1), off2 = lfMean (200.0f, 2000.0f, true, n2);
        const double ctrlSpread = std::abs (dB (off2 / off1));
        const bool fired = ctrlSpread > 1.5;
        row (gated == 10 && worst <= 1.5 && oldOld == 0.0 && fired, "LF1 level flat over density x size (cap 32, 4 seeds, N_eff >= 1)",
             "meas dB vs d10/100:" + d + "; old-vs-old " + f2 (oldOld, 3) + " dB | thr +-1.5 dB | margin " + f2 (1.5 - worst, 3)
                 + " | " + ctrl (fired, "lawOff -> d200/2000 vs d10/100 " + f2 (ctrlSpread, 2) + " dB"));
        std::printf ("  INFO LF1 sparse cells (N_eff < 1, law floors at 1):%s\n", info.toRawUTF8());
    }

    double coRms (bool sine, float density, float size, int hookMode)
    {
        P p = cat ({ base4 (1), { { "grain_limit", 3.0f }, { sn (1, "density"), density }, { sn (1, "size"), size } } });
        if (sine)
            p = with (p, { { sn (1, "scan_rate"), 100.0f }, { sn (1, "scan_mode"), 1.0f }, { sn (1, "position"), 0.0f } });
        else
            p = with (p, { { sn (1, "position"), 50.0f } });
        Session s (kFs, p);
        shipping (s);
        s.hk().coherenceVerbatim = hookMode == 1;
        s.hk().coherenceOff      = hookMode == 2;
        if (sine)
            s.load (0, sineBuf (1, (int) (20.0 * kFs), 440.0, kFs, 0.1), kFs);
        else
            s.load (0, noiseBuf (2, (int) (8.0 * kFs), 0.05f, 31), kFs);
        s.noteOn (0, 48, 127);
        s.run ((juce::int64) (3.0 * kFs), 512);
        return rmsSt (s.L, s.R, (juce::int64) (1.2 * kFs), (juce::int64) (2.8 * kFs));
    }

    void rowsCO()
    {
        {
            const double d   = dB (coRms (false, 200.0f, 1000.0f, 0) / coRms (false, 10.0f, 100.0f, 0));
            const double dc  = dB (coRms (false, 200.0f, 1000.0f, 1) / coRms (false, 10.0f, 100.0f, 1));
            const bool fired = std::abs (dc) > 1.5;
            row (std::abs (d) <= 1.5 && fired, "CO1 coherence (b), noise, spray 0, scan 0: N 200 vs N 1",
                 "meas " + f2 (d, 3) + " dB | thr +-1.5 dB | margin " + f2 (1.5 - std::abs (d), 3)
                     + " | " + ctrl (fired, "coherenceVerbatim -> " + f2 (dc, 2) + " dB"));
        }
        {
            const double d   = dB (coRms (true, 200.0f, 1000.0f, 0) / coRms (true, 10.0f, 100.0f, 0));
            const double dc  = dB (coRms (true, 200.0f, 1000.0f, 2) / coRms (true, 10.0f, 100.0f, 2));
            const bool fired = std::abs (dc) > 1.5;
            row (std::abs (d) <= 1.5 && fired, "CO2 coherence (b), sine, scan 100 % at root (coherent): N 200 vs N 1",
                 "meas " + f2 (d, 3) + " dB | thr +-1.5 dB | margin " + f2 (1.5 - std::abs (d), 3)
                     + " | " + ctrl (fired, "coherenceOff -> " + f2 (dc, 2) + " dB"));
        }
    }

    //==========================================================================
    // MX1 — enable / mute / solo clicks vs a same-motion reference
    struct MxEdge { juce::int64 t; int stream; const char* row; float v; };

    void rowMX1()
    {
        const double hz[4] { 440.0, 440.0, 440.0, 440.0 };
        // spray 30 %: without it every grain reads the same sine segment and the
        // grains of a stream phase-cancel (T = 8.8 periods) -> a near-silent,
        // vacuous render. Draws key on (stream, serial), so the isolated renders
        // still add up to the full one.
        const P common = cat ({ all4 ("density", 50.0f), all4 ("size", 200.0f), all4 ("position", 10.0f), all4 ("spray", 30.0f) });
        const std::vector<MxEdge> edges { { 9600, 3, "mute", 1.0f }, { 19200, 3, "mute", 0.0f }, { 28800, 4, "solo", 1.0f },
                                          { 38400, 4, "solo", 0.0f }, { 48000, 2, "enable", 0.0f }, { 52800, 2, "enable", 1.0f } };
        constexpr juce::int64 kLen = 62400;

        auto render = [&] (int only, bool toggles, bool zero, std::vector<float>& L, std::vector<float>& R)
        {
            Session s (kFs, with (only == 0 ? base4 (4) : with (base4 (0), { { sn (only, "enable"), 1.0f } }), common));
            shipping (s);
            s.hk().mixerRampZero = zero;
            loadSine4 (s, 2.0, hz, 0.05);
            s.noteOn (0, 48, 127);
            if (toggles)
                for (const auto& e : edges)
                    setAt (s, e.t, sn (e.stream, e.row), e.v);
            s.run (kLen, 512);
            L = s.L;
            R = s.R;
        };

        std::vector<float> UL[4], UR[4], TL, TR, CL, CR;
        for (int n = 1; n <= 4; ++n)
            render (n, false, false, UL[n - 1], UR[n - 1]);
        render (0, true, false, TL, TR);
        render (0, true, true, CL, CR);

        // Harness-side audibility trajectories: the exact 10 ms ExactRamp per stream.
        std::vector<float> aud[4];
        {
            bool en[4] { true, true, true, true }, mu[4] {}, so[4] {};
            ogi::ExactRamp ramp[4];
            const int cells = ogi::rampCellsFor (kFs, ogi::kMixerRampMs);
            for (int s = 0; s < 4; ++s)
            {
                ramp[s].reset (1.0f);
                aud[s].assign ((size_t) kLen, 0.0f);
            }
            size_t ei = 0;
            for (juce::int64 t = 0; t < kLen; t += 32)
            {
                while (ei < edges.size() && edges[ei].t <= t)
                {
                    const auto& e = edges[ei++];
                    bool* f = std::strcmp (e.row, "mute") == 0 ? mu : (std::strcmp (e.row, "solo") == 0 ? so : en);
                    f[e.stream - 1] = e.v > 0.5f;
                }
                bool anySolo = false;
                for (int s = 0; s < 4; ++s) anySolo = anySolo || (en[s] && so[s]);
                for (int s = 0; s < 4; ++s)
                {
                    ramp[s].setTarget ((en[s] && ! mu[s] && (! anySolo || so[s])) ? 1.0f : 0.0f, cells);
                    ramp[s].tick();
                    for (int i = 0; i < 32; ++i)
                        aud[s][(size_t) (t + i)] = ramp[s].value (i);
                }
            }
        }
        std::vector<float> refL ((size_t) kLen), refR ((size_t) kLen), sumL ((size_t) kLen), sumR ((size_t) kLen);
        for (juce::int64 i = 0; i < kLen; ++i)
            for (int s = 0; s < 4; ++s)
            {
                refL[(size_t) i] += aud[s][(size_t) i] * UL[s][(size_t) i];
                refR[(size_t) i] += aud[s][(size_t) i] * UR[s][(size_t) i];
                sumL[(size_t) i] += UL[s][(size_t) i];
                sumR[(size_t) i] += UR[s][(size_t) i];
            }

        double worst = 0.0, worstCtrl = 0.0;
        juce::String d;
        for (size_t k = 0; k < edges.size(); ++k)
        {
            const auto t = edges[k].t;
            const bool onEdge = std::strcmp (edges[k].row, "enable") == 0 && edges[k].v > 0.5f;
            const auto& rl = onEdge ? sumL : refL;
            const auto& rr = onEdge ? sumR : refR;
            const double base = std::max (1.0e-12, d2Peak (rl, rr, t - 32, t + 480 + 64));
            const double r  = d2Peak (TL, TR, t - 32, t + 480 + 64) / base;
            const double rc = d2Peak (CL, CR, t - 32, t + 480 + 64) / base;
            worst = std::max (worst, r);
            if (! onEdge)
                worstCtrl = std::max (worstCtrl, rc);
            d << " s" << edges[k].stream << edges[k].row << (edges[k].v > 0.5f ? "+" : "-") << ":" << f2 (r, 2);
        }
        const double level = rmsSt (sumL, sumR, 2400, 9600);
        const bool fired = worstCtrl > kClickRatio;
        row (worst <= kClickRatio && level > 1.0e-3 && fired, "MX1 QUAL-01 enable / mute / solo toggles, d2 peak vs same-motion ref",
             "meas d2 ratio" + d + " | thr <= " + f2 (kClickRatio, 1) + " | margin " + f2 (kClickRatio - worst, 3)
                 + " | " + ctrl (fired, "mixerRampZero -> worst " + f2 (worstCtrl, 2)));
    }

    //==========================================================================
    // V2s / FS1 / FS2 — voices, streams, release
    struct StealRun4 { std::vector<float> L, R; std::uint64_t steals = 0, stolenCount = 0, kills = 0; std::uint64_t stolen[2] {}; };

    StealRun4 runSteal4 (int poly, int capIdx, bool rampZero)
    {
        const double hz[4] { 1000.0, 1000.0, 1000.0, 1000.0 };
        Session s (kFs, cat ({ base4 (4), { { "polyphony", (float) poly }, { "grain_limit", (float) capIdx }, { "amp_attack", 20.0f },
                                            { "amp_release", 300.0f } },
                               all4 ("density", 50.0f), all4 ("size", 100.0f), all4 ("position", 20.0f),
                               all4 ("spray", 30.0f) }));   // spray: no grain phase cancellation (see MX1)
        shipping (s);
        s.hk().stealRampZero = rampZero;
        loadSine4 (s, 4.0, hz, 0.3);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.noteOn (24000, 64);
        s.noteOn (36000, 67);
        s.run (48000, 512);
        StealRun4 r;
        r.L = s.L;
        r.R = s.R;
        r.steals = s.hk().voiceSteals;
        r.stolenCount = s.hk().stolenCount;
        r.stolen[0] = s.hk().stolenSerials[0];
        r.stolen[1] = s.hk().stolenSerials[1];
        r.kills = s.hk().r3Kills + s.hk().capKills + s.hk().nativeSteals;
        return r;
    }

    void rowV2s()
    {
        const auto A = runSteal4 (4, 1, false);
        const auto B = runSteal4 (16, 3, false);
        const auto C = runSteal4 (4, 1, true);
        auto click = [] (const StealRun4& a, const StealRun4& ref)
        {
            double w = 0.0;
            for (juce::int64 ts : { (juce::int64) 24000, (juce::int64) 36000 })
                w = std::max (w, d2Peak (a.L, a.R, ts - 32, ts + 240 + 64) / std::max (1.0e-12, d2Peak (ref.L, ref.R, ts - 32, ts + 240 + 64)));
            return w;
        };
        const double ratio = click (A, B), ratioC = click (C, B);
        const bool overflow = A.steals == 2 && A.stolenCount == 2 && A.stolen[0] == 2 && A.stolen[1] == 3 && B.steals == 0 && A.kills > 0;
        const bool fired = ratioC > kClickRatio;
        row (overflow && ratio <= kClickRatio && fired, "V2s steal click, 4 streams: poly 4, 6 distinct pitches, cap 64",
             "meas steals " + i64 (A.steals) + " (serials " + i64 (A.stolen[0]) + ", " + i64 (A.stolen[1]) + "), grain kills + steals "
                 + i64 (A.kills) + ", d2 ratio vs no-steal (poly 16, cap 256) " + f2 (ratio, 3) + " | thr <= " + f2 (kClickRatio, 1)
                 + " | margin " + f2 (kClickRatio - ratio, 3) + " | " + ctrl (fired, "stealRampZero -> " + f2 (ratioC, 2)));
    }

    void rowFS1()
    {
        const double hz[4] { 220.0, 330.0, 440.0, 550.0 };
        const float roots[4] { 48.0f, 50.0f, 52.0f, 55.0f };
        const int notes[4] { 60, 64, 67, 72 };
        auto params = [&] (int disabled, int muted)
        {
            P p = cat ({ base4 (4), all4 ("density", 50.0f), all4 ("size", 100.0f), all4 ("position", 10.0f) });
            for (int n = 1; n <= 4; ++n)
                p = with (p, { { sn (n, "root_note"), roots[n - 1] } });
            if (disabled > 0) p = with (p, { { sn (disabled, "enable"), 0.0f } });
            if (muted > 0)    p = with (p, { { sn (muted, "mute"), 1.0f } });
            return p;
        };
        struct Out { std::vector<float> L, R; Log log; double tapeRate[4] {}; bool slotOk = true; };
        auto render = [&] (int disabled, int muted)
        {
            Session s (kFs, params (disabled, muted));
            shipping (s);
            loadSine4 (s, 3.0, hz, 0.05);
            for (int n : notes)
                s.noteOn (0, n, 127);
            s.run ((juce::int64) (1.0 * kFs), 512);
            Out o;
            o.L = s.L;
            o.R = s.R;
            o.log = s.hk().logInOrder();
            for (int k = 0; k < 4; ++k)
                o.tapeRate[k] = s.hk().tapeRate[k];
            const auto& pool = s.eng().getPool();
            const auto* g = pool.getGrains().data();
            for (int i = 0; i < ogi::GiPool::kNumSlots; ++i)
                if (pool.isActive (i))
                    o.slotOk = o.slotOk && g[i].slot == g[i].g.stream;
            return o;
        };
        const auto full = render (0, 0);

        // 16 (serial, stream) rates.
        std::map<std::pair<std::uint64_t, int>, double> rate;
        bool logSlot = true;
        for (const auto& e : full.log)
        {
            rate.emplace (std::make_pair (e.serial, e.stream), e.rate);
            logSlot = logSlot && e.stream >= 0 && e.stream < 4;
        }
        double worst = 0.0;
        int found = 0;
        for (int v = 0; v < 4; ++v)
            for (int st = 0; st < 4; ++st)
            {
                const auto it = rate.find ({ (std::uint64_t) (v + 1), st });
                if (it == rate.end()) { worst = 1.0; continue; }
                ++found;
                const double semis = std::clamp ((double) notes[v] - (double) roots[st], -48.0, 48.0);
                const double want = std::exp2 (semis / 12.0) * full.tapeRate[st] / kFs;
                worst = std::max (worst, std::abs (it->second / want - 1.0));
            }

        // Per-stream contribution = full − (that stream muted from the start).
        double minDb = 1.0e9;
        juce::String d;
        for (int st = 1; st <= 4; ++st)
        {
            const auto m = render (0, st);
            std::vector<float> dl (full.L.size()), dr (full.R.size());
            for (size_t i = 0; i < dl.size(); ++i) { dl[i] = full.L[i] - m.L[i]; dr[i] = full.R[i] - m.R[i]; }
            const double v = dB (rmsSt (dl, dr, 4800, (juce::int64) dl.size()));
            minDb = std::min (minDb, v);
            d << " s" << st << ":" << f2 (v, 1);
        }

        // Control: s3 disabled -> 0 stream-2 grains and s3 silent (disabled vs disabled + muted bitwise).
        const auto c  = render (3, 0);
        const auto cm = render (3, 3);
        const int s3Grains = countLog (c.log, 2, 0, 1 << 30);
        const bool silent = diffCount (c.L, cm.L) == 0 && diffCount (c.R, cm.R) == 0;
        const bool fired = s3Grains == 0 && silent;
        row (found == 16 && worst <= 1.0e-12 && full.slotOk && logSlot && minDb >= -60.0 && fired,
             "FS1 FUNC-01/02 stream arm: 4 slots, roots 48/50/52/55, 4-note chord",
             "meas " + juce::String (found) + "/16 (serial, stream) rates, rel err " + juce::String (worst, 15) + ", grain slot == stream "
                 + (full.slotOk ? "yes" : "NO") + ", stream dB" + d + " | thr 1e-12, >= -60 dB | margin " + f2 (minDb + 60.0, 1)
                 + " | " + ctrl (fired, "s3 disabled -> " + juce::String (s3Grains) + " stream-2 grains, s3 " + (silent ? "silent" : "AUDIBLE")));
    }

    void rowFS2()
    {
        auto run = [] (float releaseMs, int (&per)[4])
        {
            Session s (kFs, cat ({ base4 (4), { { "amp_release", releaseMs } }, all4 ("density", 50.0f), all4 ("size", 100.0f) }));
            shipping (s);
            loadNoise4 (s, 3.0, 0.1f);
            s.noteOn (0, 48);
            const juce::int64 off = 24000;
            s.noteOff (off, 48);
            s.record = false;
            s.run (48000, 512);
            const auto log = s.hk().logInOrder();
            for (int st = 0; st < 4; ++st)
                per[st] = countLog (log, st, off + 480, off + 19200);
        };
        int a[4] {}, k[4] {};
        run (500.0f, a);
        run (0.0f, k);
        int minA = INT_MAX, sumK = 0;
        for (int st = 0; st < 4; ++st) { minA = std::min (minA, a[st]); sumK += k[st]; }
        const bool fired = sumK == 0;
        row (minA > 0 && fired, "FS2 DSP-02 release continues in every stream (release 500 ms)",
             "meas spawns in [off+10, off+400] ms per stream " + juce::String (a[0]) + "/" + juce::String (a[1]) + "/" + juce::String (a[2])
                 + "/" + juce::String (a[3]) + " | thr > 0 | margin " + juce::String (minA)
                 + " | " + ctrl (fired, "release 0 -> " + juce::String (sumK) + " spawns in the window"));
    }

    //==========================================================================
    // SC2 — degree masks per stream and per collection (FUNC-09)
    void rowSC2()
    {
        constexpr int kMajor = 2, kMinor = 3;   // sN_scale choices (0 Off, 1 Chromatic, 2 Major, 3 Minor)
        const auto& major = PitchCollections::builtIns()[(size_t) Ids::scaleChoiceToId (kMajor)];
        std::uint64_t maskBits = 0;
        std::vector<double> masked;
        for (int dg = 0; dg < major.count; ++dg)
        {
            const double c = (double) major.cents[(size_t) dg];
            if (c == 0.0 || c == 400.0 || c == 700.0)
                maskBits |= std::uint64_t (1) << dg;
        }
        const double span = 1200.0, p2 = 7.0;
        const double lo = std::max (-span, -2400.0 - 100.0 * p2), hi = std::min (span, 2400.0 - 100.0 * p2);
        for (int k = -3; k <= 3; ++k)
            for (int dg = 0; dg < major.count; ++dg)
                if ((maskBits >> dg & 1u) != 0)
                {
                    const double c = (double) major.cents[(size_t) dg] + k * (double) major.period;
                    if (c >= lo && c <= hi) masked.push_back (c);
                }

        struct R { int s2 = 0, s2Bad = 0, s1 = 0, s1Unmasked = 0; std::uint64_t minorMask = 0; };
        auto run = [&] (int maskStream)
        {
            Session s (kFs, cat ({ base4 (2), all4 ("density", 100.0f), all4 ("size", 50.0f), all4 ("scale", (float) kMajor),
                                   all4 ("pitch_rand", 12.0f), { { sn (2, "pitch"), (float) p2 } } }));
            shipping (s);
            s.p->setDegreeMask (maskStream, kMajor, maskBits);
            s.load (0, noiseBuf (2, (int) (3.0 * kFs), 0.1f, 41), kFs);
            s.load (1, noiseBuf (2, (int) (3.0 * kFs), 0.1f, 42), kFs);
            s.noteOn (0, 48);
            s.record = false;
            s.run (48000, 512);
            R r;
            for (const auto& e : s.hk().logInOrder())
            {
                const double c = (double) e.pitchCents;
                if (e.stream == 1)
                {
                    ++r.s2;
                    bool in = false;
                    for (double m : masked) in = in || std::abs (c - m) < 1.0e-3;
                    r.s2Bad += in ? 0 : 1;
                }
                else if (e.stream == 0)
                {
                    ++r.s1;
                    const double m = std::fmod (std::fmod (c, 1200.0) + 1200.0, 1200.0);
                    bool unmasked = false;
                    for (double u : { 200.0, 500.0, 900.0, 1100.0 }) unmasked = unmasked || std::abs (m - u) < 1.0e-3;
                    r.s1Unmasked += unmasked ? 1 : 0;
                }
            }
            r.minorMask = s.p->getDegreeMask (2, kMinor);
            return r;
        };
        const auto a = run (2);
        const auto k = run (1);
        const bool fired = k.s2Bad > 0;
        row (a.s2 > 50 && a.s2Bad == 0 && a.s1Unmasked > 0 && a.minorMask == Proc::kAllOn && fired,
             "SC2 FUNC-09 degree mask (s2, Major) = {0, 400, 700} c, s2 pitch +7",
             "meas s2 " + juce::String (a.s2Bad) + "/" + juce::String (a.s2) + " picks outside the masked window, s1 unmasked picks "
                 + juce::String (a.s1Unmasked) + "/" + juce::String (a.s1) + ", (s2, Minor) " + (a.minorMask == Proc::kAllOn ? "all-on" : "CHANGED")
                 + " | thr 0 outside | margin " + juce::String (-a.s2Bad)
                 + " | " + ctrl (fired, "mask on s1 instead -> s2 " + juce::String (k.s2Bad) + " picks outside"));
    }

    //==========================================================================
    // ST — scale state (D17, PD3, PD4)
    bool st1Control()
    {
        const std::uint64_t m = 0x8000000000000001ull;
        const auto viaDecimal = (std::uint64_t) juce::String ((juce::uint64) m).getLargeIntValue();
        return viaDecimal != m;
    }

    void rowsST()
    {
        juce::String bad;
        // ST1: 4 x 31 random canonical masks + a 19-EDO custom scale.
        {
            auto a = make();
            a->setCustomScale ("nineteen", "edo 19", bad);
            juce::Random r ((juce::int64) 0x5701);
            std::uint64_t want[5][32] {};
            for (int s = 1; s <= 4; ++s)
                for (int c = 1; c < 32; ++c)
                {
                    a->setDegreeMask (s, c, (std::uint64_t) r.nextInt64());
                    want[s][c] = a->getDegreeMask (s, c);
                }
            const auto ca = a->getCustomScale();
            const auto blob = stateOf (*a);
            auto b = make();
            const auto v0 = b->getCustomScale().version;
            load (*b, blob);
            int diff = 0, bit63 = 0;
            for (int s = 1; s <= 4; ++s)
                for (int c = 1; c < 32; ++c)
                {
                    diff += b->getDegreeMask (s, c) == want[s][c] ? 0 : 1;
                    bit63 += (want[s][c] >> 63) != 0 ? 1 : 0;
                }
            const auto cb = b->getCustomScale();
            const bool custom = cb.name == ca.name && cb.text == ca.text && sameF (cb.period, ca.period) && cb.version > v0
                             && b->getCustomTable().count == 19;
            const bool fired = st1Control();
            row (diff == 0 && custom && fired, "ST1 D17 round trip: 4 x 31 random canonical masks + 19-EDO custom",
                 "meas " + juce::String (diff) + "/124 masks differ (" + juce::String (bit63) + " with bit 63), custom name/text/period "
                     + (custom ? "exact, version advanced " + juce::String ((int) v0) + " -> " + juce::String ((int) cb.version) : juce::String ("WRONG"))
                     + " | thr exact | margin " + juce::String (-diff)
                     + " | " + ctrl (fired, "bit-63 mask via decimal string + getLargeIntValue clamps (P17)"));
        }
        // ST2: a 64-degree custom with only bit 63 set + a partial 12-EDO mask.
        {
            auto a = make();
            a->setCustomScale ("sixtyfour", "edo 64", bad);
            const std::uint64_t only63 = std::uint64_t (1) << 63;
            a->setDegreeMask (1, 31, only63);
            a->setDegreeMask (2, 1, 0x5);
            const auto m1 = a->getDegreeMask (1, 31), m2 = a->getDegreeMask (2, 1);
            auto b = make();
            load (*b, stateOf (*a));
            const bool ok = m1 == only63 && b->getDegreeMask (1, 31) == only63 && b->getDegreeMask (2, 1) == m2
                         && m2 == Proc::canonicalMask (0x5, 12) && b->getCustomTable().count == 64;
            const bool fired = st1Control();
            row (ok, "ST2 bit 63: 64-degree custom, only bit 63 set; partial 12-EDO mask",
                 "meas custom mask " + juce::String::toHexString ((juce::int64) b->getDegreeMask (1, 31)) + ", 12-EDO "
                     + juce::String::toHexString ((juce::int64) b->getDegreeMask (2, 1)) + " | thr exact | margin 0 | "
                     + ctrl (fired, "(ST1 control) decimal + getLargeIntValue clamps bit 63"));
        }
        // ST3: absent CustomScale / DegreeMasks / Engine -> defaults, seed kept.
        {
            auto runSt3 = [&bad] (bool noReset, juce::String& text, bool& allOn, std::uint64_t& seed)
            {
                auto a = make();
                a->setSeed (0x1234ABCDull);
                a->setCustomScale ("x", "edo 19", bad);
                a->setDegreeMask (3, 2, 0x5);
                const auto blob1 = stateOf (*a);
                auto x = xmlOf (blob1);
                for (const char* tag : { "CustomScale", "DegreeMasks", "Engine" })
                    while (auto* c = x->getChildByName (tag))
                        x->removeChildElement (c, true);
                const auto blob2 = blobOf (*x);
                auto b = make();
                b->testNoStateReset = noReset;
                load (*b, blob1);
                load (*b, blob2);
                text = b->getCustomScale().text;
                allOn = true;
                for (int s = 1; s <= 4; ++s)
                    for (int c = 1; c < 32; ++c)
                        allOn = allOn && b->getDegreeMask (s, c) == Proc::kAllOn;
                seed = b->getSeed();
            };
            juce::String t, tc;
            bool on = false, onC = false;
            std::uint64_t sd = 0, sdC = 0;
            runSt3 (false, t, on, sd);
            runSt3 (true, tc, onC, sdC);
            const bool ok = t == Proc::defaultCustomText() && on && sd == 0x1234ABCDull;
            const bool fired = tc != Proc::defaultCustomText() || ! onC;
            row (ok && fired, "ST3 absent CustomScale / DegreeMasks -> 12-EDO, all-on, seed kept",
                 "meas text '" + t + "', masks all-on " + (on ? "yes" : "NO") + ", seed " + juce::String::toHexString ((juce::int64) sd)
                     + " | thr defaults | margin 0 | " + ctrl (fired, "testNoStateReset -> '" + tc + "', all-on " + (onC ? "yes" : "no")));
        }
        // ST4: two reopens.
        {
            auto runSt4 = [&bad] (bool noStrip, juce::String& textB, juce::String& textC, int& kids)
            {
                auto a = make();
                a->setCustomScale ("a", "edo 19", bad);
                a->setDegreeMask (1, 2, 0x5);
                const auto blob1 = stateOf (*a);
                auto b = make();
                b->testNoStateStrip = noStrip;
                load (*b, blob1);
                b->setCustomScale ("b", "edo 22", bad);
                b->setDegreeMask (1, 2, 0x9);
                const auto blob2 = stateOf (*b);
                load (*b, blob2);
                auto c = make();
                c->testNoStateStrip = noStrip;
                load (*c, blob2);
                textB = b->getCustomScale().text;
                textC = c->getCustomScale().text;
                kids = liveChildren (*b) + liveChildren (*c);
                return b->getDegreeMask (1, 2) == Proc::canonicalMask (0x9, 7) && c->getDegreeMask (1, 2) == Proc::canonicalMask (0x9, 7);
            };
            juce::String b, c, bc, cc;
            int k = 0, kc = 0;
            const bool masks = runSt4 (false, b, c, k);
            runSt4 (true, bc, cc, kc);
            const bool ok = masks && b == "edo 22" && c == "edo 22" && k == 0;
            const bool fired = cc != "edo 22" || bc != "edo 22" || kc > 0;
            row (ok && fired, "ST4 two reopens: second state wins, 0 custom children in the live tree",
                 "meas reopen B '" + b + "', fresh C '" + c + "', masks " + (masks ? "ok" : "WRONG") + ", live children " + juce::String (k)
                     + " | thr second wins, 0 | margin 0 | "
                     + ctrl (fired, "testNoStateStrip -> B '" + bc + "', C '" + cc + "', children " + juce::String (kc)));
        }
        // ST5: mask bits must be exactly 16 hex digits.
        {
            auto a = make();
            auto x = xmlOf (stateOf (*a));
            auto* dm = x->getChildByName ("DegreeMasks");
            if (dm == nullptr)
                dm = x->createNewChildElement ("DegreeMasks");
            auto add = [dm] (int s, int c, const char* bits)
            {
                auto* m = dm->createNewChildElement ("Mask");
                m->setAttribute ("stream", s);
                m->setAttribute ("collection", c);
                m->setAttribute ("bits", bits);
            };
            add (1, 2, "000000000000005");      // 15
            add (1, 3, "00000000000000005");    // 17
            add (1, 4, "000000000000000g");     // 16, not hex
            add (1, 5, "0000000000000005");     // valid (control)
            auto b = make();
            load (*b, blobOf (*x));
            const bool ignored = b->getDegreeMask (1, 2) == Proc::kAllOn && b->getDegreeMask (1, 3) == Proc::kAllOn
                              && b->getDegreeMask (1, 4) == Proc::kAllOn;
            const auto whole = PitchCollections::builtIns()[(size_t) Ids::scaleChoiceToId (5)].count;
            const bool fired = b->getDegreeMask (1, 5) == Proc::canonicalMask (0x5, whole);
            row (ignored && fired, "ST5 mask bits: 15 / 17 chars and a non-hex digit are ignored",
                 juce::String ("meas 3 malformed -> all-on ") + (ignored ? "yes" : "NO") + " | thr ignored | margin 0 | "
                     + ctrl (fired, "valid 16-hex -> applied " + juce::String::toHexString ((juce::int64) b->getDegreeMask (1, 5))));
        }
        // ST6: the engine's try-lock never blocks (PD3).
        {
            Session s (kFs, base4 (1));
            s.load (0, noiseBuf (2, (int) (2.0 * kFs), 0.1f), kFs);
            s.noteOn (0, 48);
            s.run (4096, 512);
            const auto& sc = s.eng().getScaleState();
            const auto vBefore = sc.customCopyVersion();
            s.p->setCustomScale ("h", "edo 19", bad);
            const auto vWant = s.p->getCustomScale().version;
            const auto fails0 = s.hk().customTryFails;
            std::uint64_t failsHeld = 0;
            std::uint32_t vHeld = 0;
            int countHeld = 0;
            juce::int64 heldBlocks = 0;
            {
                const juce::SpinLock::ScopedLockType hold (s.p->getCustomLockForTest());
                const auto b0 = s.blocksDone;
                s.run (1024, 512);   // two blocks with the lock held by the harness
                heldBlocks = s.blocksDone - b0;
                failsHeld = s.hk().customTryFails - fails0;
                vHeld = sc.customCopyVersion();
                countHeld = sc.customCopy().count;
            }
            s.run (32, 32);   // the next tick after the release
            const bool held = heldBlocks == 2 && failsHeld >= 1 && vHeld == vBefore && countHeld == 12;
            const bool released = sc.customCopyVersion() == vWant && sc.customCopy().count == 19;
            row (held && released, "ST6 PD3 try-lock never blocks: lock held 2 blocks, copied at the next tick",
                 "meas held: " + juce::String ((int) heldBlocks) + " blocks returned, tryFails " + i64 (failsHeld) + ", copy v"
                     + juce::String ((int) vHeld) + " (" + juce::String (countHeld) + " deg) | released: v"
                     + juce::String ((int) sc.customCopyVersion()) + " (" + juce::String (sc.customCopy().count) + " deg)"
                     + " | thr fails >= 1, unchanged, then copied | margin " + i64 (failsHeld)
                     + " | " + ctrl (released && vHeld != vWant, "the held phase is the released phase's control"));
        }
    }

    //==========================================================================
    // A2 — allocations with 4 streams, randomisation and scale swaps
    P randomised()
    {
        return cat ({ all4 ("spray", 30.0f), all4 ("jitter", 50.0f), all4 ("size_rand", 50.0f), all4 ("stereo_spread", 70.0f),
                      all4 ("gain_rand", 50.0f), all4 ("pitch_rand", 7.0f), all4 ("reverse_prob", 40.0f), all4 ("window", 0.6f),
                      all4 ("window_skew", -40.0f) });
    }

    void rowA2()
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

        juce::String bad;
        Session s (kFs, cat ({ base4 (4), randomised(), all4 ("density", 100.0f), all4 ("size", 200.0f),
                               { { "polyphony", 8.0f }, { "amp_attack", 5.0f }, { "amp_release", 100.0f },
                                 { sn (2, "scale"), 2.0f }, { sn (3, "scale"), 31.0f } } }));
        shipping (s);
        s.record = false;
        loadNoise4 (s, 2.0, 0.3f);
        const auto src2 = noiseBuf (2, 2 * 48000, 0.3f, 13);
        s.arm = true;
        s.noteOn (4800, 60);
        s.noteOff (9600, 60);
        for (int n = 40; n < 60; ++n)
            s.noteOn (14400, n);
        for (int n = 40; n < 60; ++n)
            s.noteOff (24000, n);
        for (int n = 48; n < 54; ++n)                 // 6 notes sound through the swaps
            s.noteOn (28800, n);
        s.at (33600, [&src2] (Session& ss) { ss.load (0, src2, 48000.0, "swap"); });
        s.at (36000, [&bad] (Session& ss) { ss.p->setCustomScale ("x", "edo 19", bad); });
        s.at (37600, [] (Session& ss) { ss.p->setDegreeMask (2, 2, 0x15); });
        s.at (39200, [] (Session& ss) { setP (*ss.p, Ids::stream (2, "enable"), 0.0f); });
        s.at (40000, [] (Session& ss) { setP (*ss.p, Ids::stream (3, "mute"), 1.0f); });
        s.at (40800, [] (Session& ss) { setP (*ss.p, Ids::stream (4, "solo"), 1.0f); });
        s.at (43200, [] (Session& ss) { setP (*ss.p, "grain_limit", 0.0f); });
        s.at (45600, [] (Session& ss) { setP (*ss.p, Ids::stream (2, "enable"), 1.0f); setP (*ss.p, Ids::stream (4, "solo"), 0.0f); });
        s.at (46400, [&bad] (Session& ss) { ss.p->setCustomScale ("y", "edo 22", bad); ss.p->setDegreeMask (3, 31, 0x5); });
        s.run (57600, 512);
        s.arm = false;
        malloc_logger = nullptr;
        allocTrace = false;

        const int n = allocCount;
        auto& h = s.hk();
        const bool exercised = h.voiceSteals > 0 && h.tapesTaken >= 5 && (h.r3Kills + h.capKills) > 0
                            && h.customCopies > 0 && h.activeSetRebuilds > 0;
        row (liveness == 1 && n == 0 && exercised, "A2 PERF-01 0 audio-thread allocations: 4 streams, randomisation, scale swaps",
             "meas " + juce::String (n) + " allocs over " + i64 ((std::uint64_t) s.blocksDone) + " blocks (steals " + i64 (h.voiceSteals)
                 + ", tapes " + i64 (h.tapesTaken) + ", kills " + i64 (h.r3Kills + h.capKills) + ", customCopies " + i64 (h.customCopies)
                 + ", activeSetRebuilds " + i64 (h.activeSetRebuilds) + ") | thr 0 | margin " + juce::String (-n)
                 + " | " + ctrl (liveness == 1, "liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (false, "A2 alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    // NB1 / V3s / N2s — determinism with every 2.2 draw on
    P nb1Params()
    {
        return cat ({ base4 (4), randomised(), all4 ("density", 100.0f), all4 ("size", 300.0f),
                      { { "polyphony", 4.0f }, { "grain_limit", 1.0f }, { sn (3, "scale"), 2.0f },
                        { "amp_attack", 5.0f }, { "amp_release", 200.0f } } });
    }

    std::vector<float> renderNB (int block, std::uint64_t seed, const std::function<void (Proc&)>& tweak, const P& params, bool toggles)
    {
        Session s (kFs, params, seed, 512);
        shipping (s);
        if (tweak)
            tweak (*s.p);
        loadNoise4 (s, 3.0, 0.3f);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.noteOn (5003, 64);
        s.noteOn (10007, 67);   // 6 notes at polyphony 4: steals
        s.noteOff (30011, 52);
        s.noteOff (44000, 48);
        if (toggles)
        {
            setAt (s, 16000, sn (2, "enable"), 0.0f);
            setAt (s, 24000, sn (3, "mute"), 1.0f);
            s.at (28800, [] (Session& ss) { ss.p->setDegreeMask (3, 2, 0x15); });
            setAt (s, 32000, sn (2, "enable"), 1.0f);
            setAt (s, 40000, sn (3, "mute"), 0.0f);
        }
        s.run (64000, block);
        return lr (s);
    }

    void rowNB1()
    {
        const auto P0 = nb1Params();
        const auto ref = renderNB (32, kSeed, {}, P0, true);
        double e = 0.0;
        for (float x : ref) e += (double) x * (double) x;
        const bool audible = std::sqrt (e / (double) ref.size()) > 1.0e-4;
        Tally t;
        juce::String d;
        int worst = 0;
        for (int b : { 441, 512, 4096, 8192, 0 })
        {
            const int n = diffCount (ref, renderNB (b, kSeed, {}, P0, true));
            worst = std::max (worst, n);
            t.check (n == 0, (b == 0 ? juce::String ("var") : juce::String (b)) + " " + juce::String (n));
            d << " " << (b == 0 ? juce::String ("var") : juce::String (b)) << ":" << n;
        }
        const int nSeed = diffCount (ref, renderNB (32, kSeed + 1, {}, P0, true));
        const bool fired = nSeed > 0;
        row (t.bad == 0 && audible && fired, "NB1 block-size null, 4 streams, all 2.2 randomisation + toggles + mask swap",
             "meas differing samples vs 32:" + d + " | thr 0 | margin " + juce::String (-worst)
                 + " | " + ctrl (fired, "seed + 1 -> " + juce::String (nSeed) + " differ"));
    }

    Log runV3s (int slotOffset, std::uint64_t seed, std::uint64_t& steals)
    {
        const double hz[4] { 1000.0, 1000.0, 1000.0, 1000.0 };
        Session s (kFs, cat ({ base4 (4), randomised(), all4 ("density", 50.0f), all4 ("size", 100.0f),
                               { { "polyphony", 4.0f }, { "amp_attack", 20.0f }, { "amp_release", 300.0f }, { sn (3, "scale"), 2.0f } } }),
                   seed);
        shipping (s);
        if (slotOffset > 0)
            s.hk().setSlotOffset (slotOffset);
        loadSine4 (s, 4.0, hz, 0.3);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.noteOn (24000, 64);
        s.noteOn (36000, 67);
        s.record = false;
        s.run (48000, 512);
        steals = s.hk().voiceSteals;
        return s.hk().logInOrder();
    }

    void rowV3s()
    {
        auto cmp = [] (const Log& a, const Log& b, int& voiceDiff, bool& keys)
        {
            using K = std::tuple<std::uint64_t, int, std::uint64_t>;
            std::map<K, ogi::GrainLogEntry> ma, mb;
            for (const auto& e : a) ma[K { e.serial, e.stream, e.onsetIndex }] = e;
            for (const auto& e : b) mb[K { e.serial, e.stream, e.onsetIndex }] = e;
            keys = ma.size() == mb.size();
            voiceDiff = 0;
            int diff = 0;
            for (const auto& kv : ma)
            {
                const auto it = mb.find (kv.first);
                if (it == mb.end()) { keys = false; ++diff; continue; }
                const auto& e = kv.second;
                const auto& f = it->second;
                const bool same = e.onsetSample == f.onsetSample && sameD (e.frac, f.frac) && sameD (e.len, f.len)
                               && sameD (e.startPos, f.startPos) && sameD (e.rate, f.rate) && e.level == f.level && e.dir == f.dir
                               && sameF (e.gainL, f.gainL) && sameF (e.gainR, f.gainR) && e.windowStep == f.windowStep
                               && sameF (e.tiltT, f.tiltT) && sameF (e.pitchCents, f.pitchCents) && e.counterIdx == f.counterIdx;
                diff += same ? 0 : 1;
                voiceDiff += e.voice != f.voice ? 1 : 0;
            }
            return diff;
        };
        std::uint64_t sa = 0, sb = 0, sc = 0;
        const auto a = runV3s (0, kSeed, sa);
        const auto b = runV3s (7, kSeed, sb);
        const auto c = runV3s (0, kSeed + 1, sc);
        int vd = 0, vdC = 0;
        bool keys = false, keysC = false;
        const int d = cmp (a, b, vd, keys);
        const int dC = cmp (a, c, vdC, keysC);
        std::set<int> streams;
        for (const auto& e : a) streams.insert (e.stream);
        const bool fired = dC > 0;
        row (d == 0 && keys && vd > 0 && sa == 2 && sb == 2 && streams.size() == 4 && (int) a.size() > 200 && fired,
             "V3s slot invariance, 4 streams + randomisation: per-(serial, stream) logs identical",
             "meas " + juce::String (d) + " / " + juce::String ((int) a.size()) + " grains differ (" + juce::String (vd)
                 + " on another voice slot, steals " + i64 (sa) + "/" + i64 (sb) + ") | thr 0 | margin " + juce::String (-d)
                 + " | " + ctrl (fired, "seed + 1 -> " + juce::String (dC) + " differ"));
    }

    void rowN2s()
    {
        const auto readSet = eg21::readSet23();   // PLAN-2.3 PD10 (was readSet22: 113 / 148)
        int bound = 0;
        {
            auto probe = make();
            for (const auto& id : readSet)
                bound += param (*probe, id) != nullptr ? 1 : 0;
        }
        const P P0 = cat ({ base4 (4), all4 ("density", 50.0f), all4 ("size", 200.0f), all4 ("spray", 20.0f), all4 ("jitter", 30.0f),
                            all4 ("stereo_spread", 50.0f), { { "polyphony", 4.0f } } });
        int touched = 0;
        auto unread = [&readSet, &touched] (Proc& p)
        {
            juce::Random r ((juce::int64) 0x4E220002);
            touched = 0;
            for (auto* ap : p.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                    if (! readSet.contains (rp->getParameterID()))
                    {
                        rp->setValueNotifyingHost (r.nextFloat());
                        ++touched;
                    }
        };
        const auto ref = renderNB (512, kSeed, {}, P0, false);
        const int n = diffCount (ref, renderNB (512, kSeed, unread, P0, false));
        const int nD = diffCount (ref, renderNB (512, kSeed, [] (Proc& p) { setP (p, Ids::stream (1, "density"), 120.0f); }, P0, false));
        const int nW = diffCount (ref, renderNB (512, kSeed, [] (Proc& p) { setP (p, Ids::stream (4, "window"), 0.5f); }, P0, false));
        const bool fired = nD > 0 && nW > 0;
        row (n == 0 && bound == 153 && touched == 108 && fired, "N2s unread params bit-identical, 4 streams on (readSet23)",
             "meas " + juce::String (n) + " samples differ with " + juce::String (touched) + " unread params randomised ("
                 + juce::String (bound) + "/153 read IDs bound) | thr 0 | margin " + juce::String (-n)
                 + " | " + ctrl (fired, "s1_density -> " + juce::String (nD) + ", s4_window -> " + juce::String (nW) + " differ"));
    }

    //==========================================================================
    void runAll()
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        auto section = [t0] (const char* title)
        {
            std::printf ("\n-- %s  (t = %.1f s)\n", title, (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
            std::fflush (stdout);
        };
        std::printf ("\n== Stage 2 / Phase 2.2 engine gates (engine_gates_22.h)\n");
        section ("RB / W0. re-baseline + window 0");
        rowsRB();
        rowW0c();
        section ("QS. 80-pair quotas");
        rowQS1();
        rowQS2();
        rowQS3();
        rowQS4();
        rowQS5();
        rowQS6();
        rowQS7();
        section ("LF / CO. output law + coherence");
        rowLF1();
        rowsCO();
        section ("MX / V / FS. mixer, steals, streams");
        rowMX1();
        rowV2s();
        rowFS1();
        rowFS2();
        section ("SC / ST. scale state");
        rowSC2();
        rowsST();
        section ("A. allocations");
        rowA2();
        section ("N. determinism");
        rowNB1();
        rowV3s();
        rowN2s();
        std::printf ("\n-- Phase 2.2 engine gates done (%.1f s)\n", (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
    }

    //==========================================================================
    // C22 — CPU INFO (PLAN-2.2 T13, R§14). No verdict.
    double threadNs()
    {
        timespec ts {};
        clock_gettime (CLOCK_THREAD_CPUTIME_ID, &ts);
        return (double) ts.tv_sec * 1.0e9 + (double) ts.tv_nsec;
    }

    double wallNs()
    {
        timespec ts {};
        clock_gettime (CLOCK_MONOTONIC, &ts);
        return (double) ts.tv_sec * 1.0e9 + (double) ts.tv_nsec;
    }

    struct CpuRun { double pct = 0.0, duty = 0.0, meanLive = 0.0; };

    void cpu22()
    {
        static constexpr int kBlock = 128;
        auto session = [] (bool streamsOn)
        {
            auto p = cat ({ base4 (streamsOn ? 4 : 0), all4 ("density", 200.0f), all4 ("size", 1000.0f),
                            all4 ("spray", 10.0f), all4 ("jitter", 20.0f), all4 ("stereo_spread", 30.0f),
                            { { "grain_limit", 3.0f }, { "polyphony", 16.0f } } });
            auto s = std::make_unique<Session> (kFs, p, kSeed, kBlock);
            shipping (*s);
            loadNoise4 (*s, 10.0, 0.1f);
            s->record = false;
            return s;
        };
        auto measure = [] (Session& s, double seconds, bool timeIt)
        {
            juce::AudioBuffer<float> buf (2, kBlock);
            juce::MidiBuffer mb;
            const int blocks = (int) (seconds * kFs / kBlock);
            double cpu = 0.0, live = 0.0;
            const double w0 = wallNs(), c0 = threadNs();
            for (int b = 0; b < blocks; ++b)
            {
                buf.clear();
                mb.clear();
                const double a = timeIt ? threadNs() : 0.0;
                s.p->processBlock (buf, mb);
                if (timeIt)
                    cpu += threadNs() - a;
                live += (double) s.hk().liveNow;
            }
            CpuRun r;
            const double wall = wallNs() - w0, thr = threadNs() - c0;
            r.pct = 100.0 * cpu / (seconds * 1.0e9);
            r.duty = wall > 0.0 ? thr / wall : 0.0;
            r.meanLive = live / (double) blocks;
            return r;
        };

        auto s = session (true);
        {
            juce::AudioBuffer<float> buf (2, kBlock);
            juce::MidiBuffer mb;
            for (int k = 0; k < 16; ++k)
                mb.addEvent (juce::MidiMessage::noteOn (1, 40 + k, (juce::uint8) 100), 0);   // 16 distinct pitches, root 48 +-12
            buf.clear();
            s->p->processBlock (buf, mb);
        }
        measure (*s, 5.0, false);   // warm-up
        std::vector<double> pct;
        double minDuty = 1.0e9, live = 0.0;
        for (int i = 0; i < 7; ++i)
        {
            const auto r = measure (*s, 10.0, true);
            pct.push_back (r.pct);
            minDuty = std::min (minDuty, r.duty);
            live += r.meanLive / 7.0;
        }
        std::sort (pct.begin(), pct.end());

        auto by = session (false);
        {
            juce::AudioBuffer<float> buf (2, kBlock);
            juce::MidiBuffer mb;
            for (int k = 0; k < 16; ++k)
                mb.addEvent (juce::MidiMessage::noteOn (1, 40 + k, (juce::uint8) 100), 0);
            buf.clear();
            by->p->processBlock (buf, mb);
        }
        measure (*by, 1.0, false);
        const auto b = measure (*by, 10.0, true);

        const bool contended = minDuty < 0.9;
        std::printf ("  INFO C22 48 k / %d, 16 notes x 4 streams x 4 slots, d 200, size 1000, cap 256, no FX: median %.2f %% "
                     "(min %.2f, max %.2f) of real time over 7 x 10 s; mean live %.1f (saturated >= 240: %s); duty witness min %.3f%s; "
                     "bypass (streams disabled) %.3f %% -> ratio %.1f x%s\n",
                     kBlock, pct[3], pct.front(), pct.back(), live, live >= 240.0 ? "yes" : "NO", minDuty,
                     contended ? " (contended - no verdict)" : "", b.pct, b.pct > 0.0 ? pct[3] / b.pct : 0.0,
                     pct[3] > 28.0 ? " | FLAG > 28 % (ARCHITECTURE grain-only projection)" : "");
        std::fflush (stdout);
    }
}} // namespace ::eg22
