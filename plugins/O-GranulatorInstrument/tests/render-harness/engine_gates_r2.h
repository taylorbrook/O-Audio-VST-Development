/*
  ==============================================================================
    O-GranulatorInstrument render harness — rework R2 rows (PLAN-R2 PD11).

    Hands-on finding F3: a lane under feedback faded lap by lap (the convex
    write with a return of gain <= 1). R2 adds a level-hold makeup in the record
    head (GiRecordHead.h) and a stop-writing latch (Engine::updateHeads). The
    rows below bound the lane level from BELOW; the bound rows from above (FL1,
    FL1s, FL1p, FL1sp, FL2 / FL2p, CL1) are unchanged.

    Lane level = RMS over the head's latched loop at tape level 0, in dB re its
    value at t = 0 (the pristine).

    Base stimulus S0: default patch, stream 1, 4 s stereo noise tape at 0.1
    peak (-24.8 dBFS RMS), chord 48 / 55 / 60 held, 60 s, readings at t = 0
    and at every lap (4 s) from 8 s.

      LH1   S0, Amount 100 %, Saturate 0 / 20 / 100, Post: -3 / +2 dB
      LH1n  makeupOff -> fails, and falls more than 20 dB (stimulus validity)
      LH1p  LH1 in Pre (Mix 50)                              ctrl LH1pn makeupOff
      LH2   S0, Amount 50 / 25 %                             ctrl LH2n makeupOff
      LH3   S0, Pre, Mix 100, Size 100, Saturate 20 / 100    ctrl LH3n makeupOff
      LH4   -55 dBFS noise tape, 60 s; a 10 s Freeze pulse; toggle Off / On:
            max reading <= +3 dB in every arm                ctrl LH4n mkAttackSlew
      LH5   zero tape stays bitwise; a 1e-5 tape never reads above +2 dB
      LH6   two-level tape, loop on the quiet half: -3 / +2 dB of that half
                                                             ctrl LH6n mkWholeTapeRef
      LH6u  unit: SlotTape::refMeanSq vs a direct sum        ctrl LH6un off-by-one bin
      LH7   55 Hz sine under Low Cut 2000 Hz still fades > 20 dB (D71)
                                                             ctrl LH7n mkCapOff
      LH8   note-off: default patch, Drones · Tape Halo, the pedal arm
                                                             ctrl LH8n mkLatchOff
      LH9   block 512 / 32 / 1 / variable + a repeat, Post and Pre: hashes equal
      LH10  the 13 presets, 60 s: return samples clamped <= 1 % ctrl LH10n mkPeakOff
      LH12  one non-finite return sample at 10 s             ctrl LH12n mkStatScrubOff
      LH13  S0 at 44.1 and 96 kHz; LH8's default arm at 96 kHz
      LH14  after the latch: a releasing voice does not re-open the head; a new
            note does                                        ctrl LH14n mkLatchOnCounted
      LHB   the 13 presets, 60 s: every feedback stream's lane held
                                                             ctrl LHBn makeupOff
      LHF   0 audio-thread allocations / frees: hold, Reset, Record-in commit
                                                             ctrl LHFn liveness

    INFO (not rows): LH8i (latch threshold 6 vs 3 dB), LHg (gain statistics per
    preset stream), LH11 (600 s drift on four presets; --egr2-long only).

    A negative-control row runs the SAME predicate as its parent on the broken
    input and PASSES only when that predicate FAILS ("ctrl FIRED").
  ==============================================================================
*/
#pragma once

namespace { namespace egr2
{
    using eg21::Session;
    using eg21::P;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::sineBuf;
    using eg21::kSeed;
    using eg22::shipping;
    using eg22::setAt;
    using eg24b::want;
    using eg24b::section;

    constexpr double kLo = -3.0, kHi = 2.0;   // the hold band, dB re t = 0

    double dbOf (double ms) { return 10.0 * std::log10 (std::max (ms, 1.0e-30)); }
    juce::String sg (double v, int dp = 1) { return (v >= 0.0 ? "+" : "") + juce::String (v, dp); }

    //==========================================================================
    struct Lane { double ms = 0.0, pk = 0.0; bool finite = true; };

    /** Mean square / peak of slot k's working copy over the head's latched loop. */
    Lane lane (Session& s, int k)
    {
        Lane o;
        const auto* t = s.eng().getCurrentTape (k);
        if (t == nullptr)
            return o;
        const auto& hd = s.eng().getHeadForTest (k);
        std::int64_t lo = hd.lo, hi = hd.hi;
        if (hi <= lo) { lo = 0; hi = t->frames; }
        double acc = 0.0;
        for (std::int64_t i = lo; i < hi; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const double v = t->tape.levelSample (0, ch, i);
                o.finite = o.finite && std::isfinite (v);
                acc += v * v;
                o.pk = std::max (o.pk, std::abs (v));
            }
        o.ms = acc / (2.0 * (double) (hi - lo));
        return o;
    }

    //==========================================================================
    /** One S0-family arm (stream 1 of the default patch). A negative value = the default. */
    struct Arm
    {
        float fb = 100.0f, sat = 20.0f;
        int   pre = 0;
        float mix = 0.0f, size = 50.0f;
        float lowCut = -1.0f, highCut = -1.0f, loopEnd = -1.0f, release = -1.0f;
        double fs = 48000.0;
        P extra {};
    };

    P armP (const Arm& a)
    {
        P p { { "s1_feedback", a.fb }, { "s1_fb_saturation", a.sat }, { "s1_reverb_pre", (float) a.pre },
              { "s1_reverb_mix", a.mix }, { "s1_reverb_size", a.size } };
        if (a.lowCut  >= 0.0f) p.push_back ({ "s1_fb_low_cut", a.lowCut });
        if (a.highCut >= 0.0f) p.push_back ({ "s1_fb_high_cut", a.highCut });
        if (a.loopEnd >= 0.0f) p.push_back ({ "s1_loop_end", a.loopEnd });
        if (a.release >= 0.0f) p.push_back ({ "amp_release", a.release });
        for (const auto& x : a.extra) p.push_back (x);
        return p;
    }

    using Hook = std::function<void (Session&)>;

    std::unique_ptr<Session> open (const Arm& a, const juce::AudioBuffer<float>& tape, const Hook& hook = {})
    {
        auto s = std::make_unique<Session> (a.fs, armP (a), kSeed, 512);
        shipping (*s);
        s->record = false;
        if (hook) hook (*s);
        s->load (0, tape, a.fs);
        return s;
    }

    juce::AudioBuffer<float> s0Tape (double fs = 48000.0, float amp = 0.1f) { return noiseBuf (2, (int) (4.0 * fs), amp, 5); }

    void chord (Session& s, juce::int64 offAt = -1)
    {
        for (int n : { 48, 55, 60 })
        {
            s.noteOn (64, n, 100);
            if (offAt >= 0)
                s.noteOff (offAt, n);
        }
    }

    struct Hold
    {
        std::vector<double> t, d;        // reading times (s) and lane dB re t = 0
        double l0 = 0.0, mn = 0.0, mx = 0.0, last = 0.0;
        std::uint64_t writes = 0, clamps = 0, scrubs = 0;
        ogi::MakeupStats st;
        bool finite = true;
        double gMeanDb() const { return st.cells ? 20.0 * st.gLogSum / (double) st.cells : 0.0; }
        double gMaxDb() const  { return 20.0 * std::log10 (std::max (st.gMax, 1.0e-9f)); }
        double capPct() const  { return st.cells ? 100.0 * (double) st.capCells / (double) st.cells : 0.0; }
        double clampPct() const { return writes ? 100.0 * (double) clamps / (2.0 * (double) writes) : 0.0; }
    };

    /** Runs s to `seconds`, reading slot's lane at t = 0 and from `first` every `step` s. */
    Hold hold (Session& s, int slot, double seconds, double first, double step, double fs = 48000.0)
    {
        Hold h;
        s.run (64, 32);
        h.l0 = dbOf (lane (s, slot).ms);
        h.mn = 1.0e9; h.mx = -1.0e9;
        for (double t = first; t <= seconds + 1.0e-9; t += step)
        {
            const juce::int64 tgt = (juce::int64) (t * fs);
            if (tgt > s.pos) s.run (tgt - s.pos, 512);
            const auto l = lane (s, slot);
            const double d = dbOf (l.ms) - h.l0;
            h.t.push_back (t); h.d.push_back (d);
            h.mn = std::min (h.mn, d); h.mx = std::max (h.mx, d);
            h.last = d;
            h.finite = h.finite && l.finite;
        }
        const juce::int64 end = (juce::int64) (seconds * fs);
        if (end > s.pos) s.run (end - s.pos, 512);
        if (std::getenv ("OGI_LH_SERIES") != nullptr)   // dev aid: every reading
        {
            std::printf ("    series (first %.1f s, step %.1f s):", first, step);
            for (double d : h.d) std::printf (" %+.1f", d);
            std::printf ("\n");
        }
        h.writes = s.hk().head[slot].writes;
        h.clamps = s.hk().head[slot].rClamps;
        h.scrubs = s.hk().head[slot].writeScrubs;
        h.st = s.hk().mk[slot];
        return h;
    }

    bool inBand (const Hold& h, double lo = kLo, double hi = kHi) { return h.finite && ! h.d.empty() && h.mn >= lo && h.mx <= hi; }
    /** LHB's form (PLAN-R2 PD11), used where single-lap dips are part of the
        stimulus (LH6: a 2 s loop, 27 readings; LH13: another rate, another noise
        realisation): the last reading inside the band, no reading above it,
        none more than 1 dB under it. T6 deviation, recorded in SUMMARY-R2. */
    constexpr double kDip = -4.0;
    bool inBandB (const Hold& h) { return h.finite && ! h.d.empty() && h.last >= kLo && h.last <= kHi && h.mx <= kHi && h.mn >= kDip; }
    /** The head wrote for most of the render and the makeup ran (or, under makeupOff, did not). */
    bool wrote (const Hold& h, double seconds, double fs = 48000.0) { return (double) h.writes > 0.8 * seconds * fs; }

    juce::String holdText (const Hold& h)
    {
        return "min " + sg (h.mn) + " max " + sg (h.mx) + " end " + sg (h.last) + " dB (g mean " + sg (h.gMeanDb()) + " dB, capped " + f2 (h.capPct(), 1) + " %)";
    }

    //==========================================================================
    // The band rows: LH1, LH1p, LH2, LH3, LH6, LH12, LH13
    struct BandArm
    {
        juce::String name;
        Arm arm;
        juce::AudioBuffer<float> tape;
        Hook subjectHook, controlHook;   // controlHook empty = no control for this arm
    };

    enum class Ctrl { none, fails, failsAndFalls20, failsHigh };

    void bandRows (const char* id, const juce::String& title, const juce::String& ctrlTitle, const char* ctrlName,
                   std::vector<BandArm> arms, Ctrl mode, double seconds = 60.0, double first = 8.0, double step = 4.0, bool formB = false)
    {
        const auto pred = [formB] (const Hold& x) { return formB ? inBandB (x) : inBand (x); };
        const juce::String thr = formB ? sg (kLo, 0) + " / " + sg (kHi, 0) + " dB at the end, no reading under " + sg (kDip, 0) + " or over " + sg (kHi, 0) + " dB"
                                       : sg (kLo, 0) + " / " + sg (kHi, 0) + " dB";
        bool stim = true, ok = true, fired = true;
        double worstLo = 1.0e9, worstHi = -1.0e9;
        juce::String d, dc;
        for (auto& a : arms)
        {
            auto s = open (a.arm, a.tape, a.subjectHook);
            chord (*s);
            const auto h = hold (*s, 0, seconds, first, step, a.arm.fs);
            stim = stim && wrote (h, seconds, a.arm.fs) && h.st.cells > 0;
            ok = ok && pred (h);
            worstLo = std::min (worstLo, formB ? std::min (h.mn + (kLo - kDip), h.last) : h.mn); worstHi = std::max (worstHi, h.mx);
            d << " " << a.name << ": " << holdText (h) << ";";
            if (mode != Ctrl::none)
            {
                auto c = open (a.arm, a.tape, [&a] (Session& ss) { if (a.subjectHook) a.subjectHook (ss); a.controlHook (ss); });
                chord (*c);
                const auto hc = hold (*c, 0, seconds, first, step, a.arm.fs);
                stim = stim && wrote (hc, seconds, a.arm.fs);
                bool f = ! pred (hc);
                if (mode == Ctrl::failsAndFalls20) f = f && hc.mn < -20.0;
                if (mode == Ctrl::failsHigh)       f = f && hc.mx > kHi;
                fired = fired && f;
                dc << " " << a.name << ": min " << sg (hc.mn) << " max " << sg (hc.mx) << " end " << sg (hc.last) << " dB;";
            }
        }
        const double margin = std::min (worstLo - kLo, kHi - worstHi);
        row (stim && ok, juce::String (id) + " " + title,
             "meas lane dB re t0, readings " + f2 (first, 0) + " s .. " + f2 (seconds, 0) + " s every " + f2 (step, 0) + " s:" + d + (stim ? "" : " INVALID (the head did not write)")
                 + " | thr " + thr + " | margin " + f2 (margin, 2) + " | ctrl " + (mode == Ctrl::none ? juce::String ("-") : juce::String (id) + "n"));
        if (mode != Ctrl::none)
            row (stim && fired, juce::String (id) + "n control: " + ctrlTitle + " (predicate must FAIL)",
                 "meas" + dc + " | thr predicate '" + thr + "' FAILS"
                     + (mode == Ctrl::failsAndFalls20 ? ", and min < -20 dB" : mode == Ctrl::failsHigh ? ", above " + sg (kHi, 0) + " dB" : juce::String())
                     + " | " + ctrl (stim && fired, juce::String (id) + "n " + ctrlName + " ->" + dc));
    }

    const Hook kMakeupOff = [] (Session& s) { s.hk().makeupOff = true; };

    void rowLH1()
    {
        std::vector<BandArm> arms;
        for (float sat : { 0.0f, 20.0f, 100.0f })
            arms.push_back ({ "sat " + f2 (sat, 0), Arm { 100.0f, sat }, s0Tape(), {}, kMakeupOff });
        bandRows ("LH1", "D70 / D75 lane level held: default patch, noise 0.1, chord held 60 s, Amount 100 %, Saturate 0 / 20 / 100, Post",
                  "makeupOff (the R1 write) -> the lane fades by more than 20 dB", "makeupOff", arms, Ctrl::failsAndFalls20);
    }

    void rowLH1p()
    {
        std::vector<BandArm> arms;
        for (float sat : { 0.0f, 20.0f, 100.0f })
            arms.push_back ({ "sat " + f2 (sat, 0), Arm { 100.0f, sat, 1, 50.0f }, s0Tape(), {}, kMakeupOff });
        bandRows ("LH1p", "D74 lane level held in Pre (Mix 50): Amount 100 %, Saturate 0 / 20 / 100", "makeupOff -> the lane fades", "makeupOff", arms, Ctrl::fails);
    }

    void rowLH2()
    {
        std::vector<BandArm> arms;
        for (float fb : { 50.0f, 25.0f })
            arms.push_back ({ "Amount " + f2 (fb, 0), Arm { fb, 20.0f }, s0Tape(), {}, kMakeupOff });
        bandRows ("LH2", "D73 lane level held below Amount 100 % (the kappa term): Amount 50 / 25 %, Saturate 20, Post", "makeupOff -> the lane fades", "makeupOff", arms, Ctrl::fails);
    }

    void rowLH3()
    {
        std::vector<BandArm> arms;
        for (float sat : { 20.0f, 100.0f })
            arms.push_back ({ "sat " + f2 (sat, 0), Arm { 100.0f, sat, 1, 100.0f, 100.0f }, s0Tape(), {}, kMakeupOff });
        bandRows ("LH3", "D74 Pre worst case (Mix 100, Size 100): Amount 100 %, Saturate 20 / 100", "makeupOff -> the lane fades", "makeupOff", arms, Ctrl::fails);
    }

    void rowLH6()
    {
        auto tape = noiseBuf (2, (int) (4.0 * 48000.0), 0.2f, 5);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < tape.getNumSamples() / 2; ++i)
                tape.getWritePointer (ch)[i] *= 0.1f;   // first half 20 dB down
        Arm a { 100.0f, 20.0f };
        a.loopEnd = 50.0f;
        std::vector<BandArm> arms { { "loop = the quiet half", a, tape, {}, [] (Session& s) { s.hk().mkWholeTapeRef = true; } } };
        bandRows ("LH6", "PD5 the reference follows the latched loop: two-level tape (first half 20 dB down), loop on the quiet half (2 s laps)",
                  "mkWholeTapeRef (the reference is the whole tape) -> the quiet half is lifted", "mkWholeTapeRef", arms, Ctrl::failsHigh, 60.0, 8.0, 2.0, true);
    }

    void rowLH12()
    {
        const Hook inj = [] (Session& s)
        {
            s.hk().injectReturn = { 0, 480000 + 7, std::numeric_limits<float>::quiet_NaN(), false };
        };
        // Subject and control by hand: the stimulus clause needs the injection's own counters.
        auto run = [&] (bool scrubOff)
        {
            auto s = open (Arm { 100.0f, 20.0f }, s0Tape(), [&] (Session& ss) { inj (ss); ss.hk().mkStatScrubOff = scrubOff; });
            chord (*s);
            auto h = hold (*s, 0, 60.0, 12.0, 4.0);
            const bool done = s->hk().injectReturn.done;
            return std::make_pair (h, done);
        };
        const auto [h, done] = run (false);
        const auto [hc, doneC] = run (true);
        const bool stim = done && doneC && h.scrubs >= 1 && hc.scrubs >= 1 && h.st.scrubCells == 1 && wrote (h, 60.0) && wrote (hc, 60.0);
        const bool ok = inBand (h);
        const bool fired = ! inBand (hc);
        row (stim && ok, "LH12 PD6 (correction 1) one non-finite return sample at 10 s: the makeup keeps holding (readings from 12 s), tape finite",
             "meas " + holdText (h) + ", tape finite " + (h.finite ? "yes" : "NO") + ", write scrubs " + i64 (h.scrubs) + ", makeup cells skipped " + i64 (h.st.scrubCells)
                 + (stim ? "" : " INVALID (the injection did not land)") + " | thr " + sg (kLo, 0) + " / " + sg (kHi, 0) + " dB | margin "
                 + f2 (std::min (h.mn - kLo, kHi - h.mx), 2) + " | ctrl LH12n");
        const juce::String dc = " min " + sg (hc.mn) + " max " + sg (hc.mx) + " end " + sg (hc.last) + " dB";
        row (stim && fired, "LH12n control: mkStatScrubOff -> the running means go non-finite, the gain sits at 1, the lane fades (predicate must FAIL)",
             "meas" + dc + " | thr predicate '" + sg (kLo, 0) + " / " + sg (kHi, 0) + " dB' FAILS | " + ctrl (stim && fired, "LH12n mkStatScrubOff ->" + dc));
    }

    //==========================================================================
    // LH4 — no overshoot on a quiet lane
    void rowLH4()
    {
        const float amp = 0.1f * (float) std::pow (10.0, (-55.0 + 24.77) / 20.0);   // -55 dBFS RMS
        const auto tape = noiseBuf (2, (int) (4.0 * 48000.0), amp, 5);
        struct A { const char* name; int ev; };
        const A arms[] = { { "held 60 s", 0 }, { "Freeze 20-30 s", 1 }, { "toggle Off 20 s / On 30 s", 2 } };
        auto run = [&] (int ev, bool slew)
        {
            auto s = open (Arm { 100.0f, 20.0f }, tape, [slew] (Session& ss) { ss.hk().mkAttackSlew = slew; });
            chord (*s);
            if (ev == 1) { eg24::setFreezeAt (*s, 20 * 48000, true); eg24::setFreezeAt (*s, 30 * 48000, false); }
            if (ev == 2) { setAt (*s, 20 * 48000, "s1_fb_enable", 0.0f); setAt (*s, 30 * 48000, "s1_fb_enable", 1.0f); }
            auto h = hold (*s, 0, 60.0, 0.5, 0.5);
            const auto parks = s->hk().head[0].parks;
            return std::make_pair (h, parks);
        };
        bool stim = true, ok = true, fired = false;
        double worst = -1.0e9;
        juce::String d, dc;
        for (const auto& a : arms)
        {
            const auto [h, parks] = run (a.ev, false);
            const auto [hc, parksC] = run (a.ev, true);
            stim = stim && h.writes > 0 && h.st.cells > 0 && hc.writes > 0 && (a.ev == 0 || (parks >= 1 && parksC >= 1));
            ok = ok && h.finite && h.mx <= 3.0;
            fired = fired || hc.mx > 3.0;
            worst = std::max (worst, h.mx);
            d << " " << a.name << ": max " << sg (h.mx) << " min " << sg (h.mn) << " end " << sg (h.last) << " dB (g max " << sg (h.gMaxDb()) << " dB, parks " << i64 (parks) << ");";
            dc << " " << a.name << ": max " << sg (hc.mx) << " dB;";
        }
        row (stim && ok, "LH4 PD6 no overshoot: -55 dBFS noise lane, Amount 100 %, 60 s; a 10 s Freeze pulse; the toggle Off / On (readings every 0.5 s)",
             "meas" + d + (stim ? "" : " INVALID") + " | thr max <= +3 dB | margin " + f2 (3.0 - worst, 2) + " | ctrl LH4n");
        row (stim && fired, "LH4n control: mkAttackSlew (the gain falls at the rise limit) -> overshoot above +3 dB (predicate must FAIL)",
             "meas" + dc + " | thr predicate 'max <= +3 dB' FAILS | " + ctrl (stim && fired, "LH4n mkAttackSlew ->" + dc));
    }

    //==========================================================================
    // LH5 — silence and the noise floor are never lifted
    void rowLH5()
    {
        auto z = open (Arm { 100.0f, 20.0f }, eg24::constBuf ((int) (4.0 * 48000.0), 0.0f));
        chord (*z);
        z->run (64, 32);
        const auto h0 = z->eng().getCurrentTape (0)->tape.hashForTest();
        z->run ((juce::int64) (20.0 * 48000.0), 512);
        const auto h1 = z->eng().getCurrentTape (0)->tape.hashForTest();
        const auto lz = lane (*z, 0);
        const auto wz = z->hk().head[0].writes;

        auto q = open (Arm { 100.0f, 20.0f }, noiseBuf (2, (int) (4.0 * 48000.0), 1.0e-5f, 5));
        chord (*q);
        const auto h = hold (*q, 0, 60.0, 1.0, 1.0);
        const bool stim = wz > 0 && wrote (h, 60.0);
        row (stim && h0 == h1 && lz.pk == 0.0 && h.finite && h.mx <= kHi,
             "LH5 PD6 silence is never lifted: a zero tape stays bitwise (20 s); a 1e-5 tape never reads above +2 dB (60 s, readings every 1 s)",
             "meas zero tape hash " + juce::String (h0 == h1 ? "equal" : "CHANGED") + ", peak " + juce::String (lz.pk, 9) + ", writes " + i64 (wz)
                 + "; 1e-5 tape (" + f2 (h.l0, 1) + " dBFS): max " + sg (h.mx) + " min " + sg (h.mn) + " end " + sg (h.last) + " dB, g max " + sg (h.gMaxDb()) + " dB"
                 + (stim ? "" : " INVALID") + " | thr bitwise, <= +2 dB | margin " + f2 (kHi - h.mx, 2) + " | ctrl - (bound row)");
    }

    //==========================================================================
    // LH6u — the reference table against a direct sum
    double directMs (const juce::AudioBuffer<float>& b, std::int64_t lo, std::int64_t hi)
    {
        double acc = 0.0;
        for (std::int64_t i = lo; i < hi; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const double v = b.getReadPointer (ch)[i];
                acc += v * v;
            }
        return hi > lo ? acc / (2.0 * (double) (hi - lo)) : 0.0;
    }

    /** refMeanSq with the first bin index shifted (the LH6un model of an off-by-one). */
    double refShifted (const ogi::SlotTape& t, std::int64_t lo, std::int64_t hi, int shift)
    {
        const std::int64_t bins = (std::int64_t) t.refCum.size() - 1;
        if (bins < 1) return 0.0;
        const std::int64_t b0 = std::clamp<std::int64_t> (lo / ogi::SlotTape::kRefBin + shift, 0, bins - 1);
        const std::int64_t b1 = std::clamp<std::int64_t> ((hi + ogi::SlotTape::kRefBin - 1) / ogi::SlotTape::kRefBin, b0 + 1, bins);
        const double n = (double) (std::min<std::int64_t> (b1 * ogi::SlotTape::kRefBin, t.refFrames) - lo / ogi::SlotTape::kRefBin * ogi::SlotTape::kRefBin);
        return n > 0.0 ? (t.refCum[(size_t) b1] - t.refCum[(size_t) b0]) / (2.0 * n) : 0.0;
    }

    void rowLH6u()
    {
        const int kB = ogi::SlotTape::kRefBin;
        const int lens[3] = { 65536, 48000 + 100, 100 };   // a multiple of 256; not a multiple; shorter than one bin
        int alignedBad = 0, alignedN = 0, coverBad = 0, coverN = 0, nearBad = 0, nearN = 0, shiftedBad = 0;
        double worstRel = 0.0, worstDb = 0.0;
        juce::Random rnd (0x1F2E3D);
        for (int k = 0; k < 3; ++k)
        {
            const int n = lens[k];
            auto buf = noiseBuf (2, n, 0.3f, 61 + k);
            // a level step so an off-by-one bin is visible: the first quarter 12 dB down
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n / 4; ++i)
                    buf.getWritePointer (ch)[i] *= 0.25f;
            ogi::SlotTape t;
            t.buildRef (buf.getReadPointer (0), buf.getReadPointer (1), n);
            t.frames = n;
            const int bins = (n + kB - 1) / kB;
            for (int r = 0; r < 40; ++r)
            {
                // (a) bin-aligned: [256·a, min (256·b, n))
                const int a = rnd.nextInt (bins), b = a + 1 + rnd.nextInt (bins - a);
                const std::int64_t lo = (std::int64_t) a * kB, hi = std::min<std::int64_t> ((std::int64_t) b * kB, n);
                const double want_ = directMs (buf, lo, hi), got = t.refMeanSq (lo, hi);
                const double rel = std::abs (got - want_) / std::max (want_, 1.0e-300);
                worstRel = std::max (worstRel, rel);
                ++alignedN; alignedBad += rel <= 1.0e-12 ? 0 : 1;
                const double sh = refShifted (t, lo, hi, 1);
                shiftedBad += (bins > 1 && a + 1 < b) ? (std::abs (sh - want_) / std::max (want_, 1.0e-300) <= 1.0e-12 ? 0 : 1) : 0;

                // (b) any region: equals the direct sum over the covering bins
                const std::int64_t l2 = rnd.nextInt (n), h2 = l2 + 1 + rnd.nextInt (n - (int) l2);
                const std::int64_t cl = l2 / kB * kB, chh = std::min<std::int64_t> ((h2 + kB - 1) / kB * kB, n);
                const double wc_ = directMs (buf, cl, chh), gc = t.refMeanSq (l2, h2);
                ++coverN; coverBad += std::abs (gc - wc_) / std::max (wc_, 1.0e-300) <= 1.0e-12 ? 0 : 1;

                // (c) on the stationary part (the last three quarters), regions of >= 1024 frames: within 0.5 dB of the region itself
                if (n >= 8192)
                {
                    const int q = n / 4 + kB;
                    const int span = n - q;
                    const std::int64_t l3 = q + rnd.nextInt (span - 1024), h3 = l3 + 1024 + rnd.nextInt (span - 1024 - (int) (l3 - q) + 1);
                    const double dbe = std::abs (dbOf (t.refMeanSq (l3, h3)) - dbOf (directMs (buf, l3, h3)));
                    worstDb = std::max (worstDb, dbe);
                    ++nearN; nearBad += dbe <= 0.5 ? 0 : 1;
                }
            }
        }
        ogi::SlotTape empty;
        const bool emptyZero = empty.refMeanSq (0, 1000) == 0.0;
        const bool ok = alignedBad == 0 && coverBad == 0 && nearBad == 0 && emptyZero;
        row (ok, "LH6u PD5 unit: SlotTape::refMeanSq vs a direct sum, 3 tapes (65536 / 48100 / 100 frames), 40 regions each",
             "meas bin-aligned " + juce::String (alignedN - alignedBad) + " / " + juce::String (alignedN) + " exact (worst rel " + juce::String (worstRel, 16) + "); any region == its covering bins "
                 + juce::String (coverN - coverBad) + " / " + juce::String (coverN) + "; regions >= 1024 frames on stationary noise " + juce::String (nearN - nearBad) + " / " + juce::String (nearN)
                 + " within 0.5 dB (worst " + f2 (worstDb, 3) + " dB); empty table -> " + (emptyZero ? "0" : "NON-ZERO")
                 + " | thr 1e-12 rel, 0.5 dB | ctrl LH6un");
        const bool fired = shiftedBad > 0;
        row (fired, "LH6un control: the first bin index off by one -> bin-aligned regions are no longer exact (predicate must FAIL)",
             "meas " + juce::String (shiftedBad) + " bin-aligned regions outside 1e-12 | thr predicate 'exact to 1e-12' FAILS | "
                 + ctrl (fired, "LH6un b0 + 1 -> " + juce::String (shiftedBad) + " regions wrong"));
    }

    //==========================================================================
    // LH7 — the cap: filtering to nothing still fades (D71)
    void rowLH7()
    {
        // T6 deviation (PLAN-R2 PD11 allowed a harder stimulus; the predicate and
        // the control are unchanged). The planned stimulus, a 55 Hz sine lane under
        // Low Cut 2000 Hz, does NOT fade: the residue that passes the filter
        // (grain edges, harmonics) is levelled back to the reference, the lane is
        // rewritten as pass-band content within a lap or two and then holds (INFO
        // LH7i below). A loss the lane cannot escape by changing its spectrum is
        // the stream level: it scales the tap on any material. At -30 dB the
        // return is about 54 dB under the lane, 14 dB more than the cap gives back.
        Arm b { 100.0f, 20.0f };
        b.extra = { { "s1_level", -30.0f } };
        auto run = [&] (const Arm& arm, const juce::AudioBuffer<float>& tape, bool capOff)
        {
            auto s = open (arm, tape, [capOff] (Session& ss) { ss.hk().mkCapOff = capOff; });
            chord (*s);
            return hold (*s, 0, 60.0, 4.0, 4.0);
        };
        const auto h = run (b, s0Tape(), false), hc = run (b, s0Tape(), true);
        const bool stim = wrote (h, 60.0) && wrote (hc, 60.0) && h.st.cells > 0 && h.capPct() > 90.0;
        const bool ok = h.finite && h.last < -20.0;
        const bool fired = ! (hc.last < -20.0);
        row (stim && ok, "LH7 D71 the cap: a lane whose return sits more than the cap under it still fades by more than 20 dB in 60 s (S0 at stream level -30 dB, Amount 100 %)",
             "meas end " + sg (h.last) + " dB (at 8 s " + sg (h.d.size() > 1 ? h.d[1] : 0.0) + " dB, g mean " + sg (h.gMeanDb()) + " dB, capped " + f2 (h.capPct(), 1) + " % of cells)"
                 + (stim ? "" : " INVALID (not cap-limited)") + " | thr end < -20 dB | margin " + f2 (-20.0 - h.last, 2) + " | ctrl LH7n");
        row (stim && fired, "LH7n control: mkCapOff -> the lane is held (fades less than 20 dB) (predicate must FAIL)",
             "meas end " + sg (hc.last) + " dB (min " + sg (hc.mn) + ", g mean " + sg (hc.gMeanDb()) + " dB, max " + sg (hc.gMaxDb()) + " dB) | thr predicate 'end < -20 dB' FAILS | "
                 + ctrl (stim && fired, "LH7n mkCapOff -> end " + sg (hc.last) + " dB"));

        // INFO LH7i: the planned filter stimulus, default grains / Saturate 20 and smooth grains / Saturate 0.
        Arm a { 100.0f, 20.0f };
        a.lowCut = 2000.0f;
        Arm a2 { 100.0f, 0.0f };
        a2.lowCut = 2000.0f;
        a2.extra = { { "s1_size", 500.0f }, { "s1_density", 20.0f } };
        const auto sine = sineBuf (2, (int) (4.0 * 48000.0), 55.0, 48000.0, 0.25);
        const auto i1 = run (a, sine, false), i2 = run (a2, sine, false);
        std::printf ("  INFO LH7i 55 Hz sine lane under Low Cut 2000 Hz, Amount 100 %%, 60 s: default grains, Saturate 20: at 4 s %+.1f, min %+.1f, end %+.1f dB (g mean %+.1f dB, capped %.1f %%);"
                     " size 500 ms, density 20, Saturate 0: at 4 s %+.1f, min %+.1f, end %+.1f dB (g mean %+.1f dB, capped %.1f %%) -- the lane is rewritten as the filter's residue and holds\n",
                     i1.d.empty() ? 0.0 : i1.d[0], i1.mn, i1.last, i1.gMeanDb(), i1.capPct(), i2.d.empty() ? 0.0 : i2.d[0], i2.mn, i2.last, i2.gMeanDb(), i2.capPct());
        std::fflush (stdout);
    }

    //==========================================================================
    // Presets (the 13 that carry a running feedback loop)
    int presetIndex (const char* shortName)
    {
        const auto& b = pg4::bank();
        for (int i = 0; i < (int) b.specs.size(); ++i)
            if (pg4::shortOf (b.specs[(size_t) i]) == juce::String::fromUTF8 (shortName))
                return i;
        return -1;
    }

    std::vector<int> fbStreams (const pg4::F::FactoryPresetSpec& sp)
    {
        std::vector<int> v;
        for (int n = 1; n <= 4; ++n)
            if (pg4::streamOn (sp, n) && pg4::sval (sp, n, "feedback", 0.0f) > 0.0f && pg4::sval (sp, n, "fb_enable", 1.0f) > 0.5f)
                v.push_back (n - 1);
        return v;
    }

    void presetChord (Session& s, juce::int64 offAt = -1)
    {
        s.noteOn (64, 48, 100);
        s.noteOn (64, 55, 100);
        s.noteOn (72000, 60, 90);
        if (offAt >= 0)
            for (int n : { 48, 55, 60 })
                s.noteOff (offAt, n);
    }

    const double kMarks[] = { 2.0, 5.0, 10.0, 20.0, 30.0, 45.0, 60.0 };

    struct PStream { int slot = 0; double l0 = 0.0, mn = 1.0e9, mx = -1.0e9, last = 0.0; std::uint64_t writes = 0, clamps = 0; ogi::MakeupStats st; bool finite = true; };
    struct PHold { juce::String name; std::vector<PStream> st; };

    PHold presetHold (int i, const Hook& hook)
    {
        const auto& sp = pg4::bank().specs[(size_t) i];
        PHold o;
        o.name = pg4::shortOf (sp);
        pg4::PSess ps;
        ps.apply (i);
        ps.s->record = false;
        if (hook) hook (*ps.s);
        for (int k : fbStreams (sp))
            { PStream q; q.slot = k; o.st.push_back (q); }
        presetChord (*ps.s);
        ps.s->run (64, 32);
        for (auto& q : o.st)
            q.l0 = dbOf (lane (*ps.s, q.slot).ms);
        for (double m : kMarks)
        {
            const juce::int64 tgt = (juce::int64) (m * 48000.0);
            if (tgt > ps.s->pos) ps.s->run (tgt - ps.s->pos, 512);
            for (auto& q : o.st)
            {
                const auto l = lane (*ps.s, q.slot);
                const double d = dbOf (l.ms) - q.l0;
                q.mn = std::min (q.mn, d); q.mx = std::max (q.mx, d); q.last = d;
                q.finite = q.finite && l.finite;
            }
        }
        for (auto& q : o.st)
        {
            q.writes = ps.s->hk().head[q.slot].writes;
            q.clamps = ps.s->hk().head[q.slot].rClamps;
            q.st = ps.s->hk().mk[q.slot];
        }
        return o;
    }

    std::vector<int> movedIdx()
    {
        std::vector<int> v;
        for (const char* n : pg4::kFeedbackMoved)
            v.push_back (presetIndex (n));
        return v;
    }

    std::vector<PHold> presetHolds (const Hook& hook)
    {
        const auto idx = movedIdx();
        std::vector<PHold> out (idx.size());
        pg4::parallelFor ((int) idx.size(), [&] (int k) { if (idx[(size_t) k] >= 0) out[(size_t) k] = presetHold (idx[(size_t) k], hook); });
        return out;
    }

    /** LHB / LH10 / LHg share three 60 s render sets of the 13 presets. */
    void rowsPresets()
    {
        const bool wantB = want ("LHB"), want10 = want ("LH10"), wantG = want ("LHg");
        if (! (wantB || want10 || wantG))
            return;
        const auto idx = movedIdx();
        bool found = true;
        for (int i : idx) found = found && i >= 0;
        const auto subj = found ? presetHolds ({}) : std::vector<PHold>();
        int streams = 0;
        for (const auto& p : subj) streams += (int) p.st.size();
        bool stim = found && streams >= 13;
        for (const auto& p : subj)
            for (const auto& q : p.st)
                stim = stim && (double) q.writes > 0.8 * 58.0 * 48000.0 && q.st.cells > 0;

        if (wantG)
            for (const auto& p : subj)
                for (const auto& q : p.st)
                    std::printf ("  INFO LHg %-14s s%d: makeup gain mean %+.1f dB, max %+.1f dB; capped %.1f %%, peak-limited %.1f %%, ceiling %.1f %% of cells; lane min %+.1f max %+.1f end %+.1f dB; clamped %.3f %%\n",
                                 p.name.toRawUTF8(), q.slot + 1, q.st.cells ? 20.0 * q.st.gLogSum / (double) q.st.cells : 0.0, 20.0 * std::log10 (std::max (q.st.gMax, 1.0e-9f)),
                                 q.st.cells ? 100.0 * (double) q.st.capCells / (double) q.st.cells : 0.0, q.st.cells ? 100.0 * (double) q.st.pkCells / (double) q.st.cells : 0.0,
                                 q.st.cells ? 100.0 * (double) q.st.ceilCells / (double) q.st.cells : 0.0, q.mn, q.mx, q.last,
                                 q.writes ? 100.0 * (double) q.clamps / (2.0 * (double) q.writes) : 0.0);

        if (wantB)
        {
            bool ok = true;
            double loEnd = 1.0e9, hiEnd = -1.0e9, loMark = 1.0e9;
            juce::String worst;
            for (const auto& p : subj)
                for (const auto& q : p.st)
                {
                    ok = ok && q.finite && q.last >= kLo && q.last <= kHi && q.mn >= -4.0;
                    if (q.mn < loMark) { loMark = q.mn; worst = p.name + " s" + juce::String (q.slot + 1); }
                    loEnd = std::min (loEnd, q.last); hiEnd = std::max (hiEnd, q.last);
                }
            const auto off = found ? presetHolds (kMakeupOff) : std::vector<PHold>();
            int down9 = 0, nOff = 0;
            double offLo = 1.0e9, offHi = -1.0e9;
            for (const auto& p : off)
                for (const auto& q : p.st)
                {
                    ++nOff;
                    down9 += q.last < -9.0 ? 1 : 0;
                    offLo = std::min (offLo, q.last); offHi = std::max (offHi, q.last);
                }
            row (stim && ok, "LHB D72 the 13 feedback presets, chord held 60 s: every feedback stream's lane within -3 / +2 dB at 60 s, never below -4 dB at the marks",
                 "meas " + juce::String (streams) + " streams on " + juce::String ((int) subj.size()) + " presets: at 60 s " + sg (loEnd) + " .. " + sg (hiEnd) + " dB; lowest mark " + sg (loMark) + " dB ("
                     + worst + "); marks 2 / 5 / 10 / 20 / 30 / 45 / 60 s" + (stim ? "" : " INVALID") + " | thr -3 / +2 dB at 60 s, >= -4 dB | margin "
                     + f2 (std::min ({ loEnd - kLo, kHi - hiEnd, loMark + 4.0 }), 2) + " | ctrl LHBn");
            const bool fired = nOff == streams && down9 >= 11;
            row (stim && fired, "LHBn control: makeupOff -> at least 11 of the feedback streams end more than 9 dB down (predicate must FAIL)",
                 "meas " + juce::String (down9) + " / " + juce::String (nOff) + " streams below -9 dB at 60 s (" + sg (offLo) + " .. " + sg (offHi) + " dB) | thr >= 11 | "
                     + ctrl (stim && fired, "LHBn makeupOff -> " + juce::String (down9) + " / " + juce::String (nOff) + " faded"));
        }

        if (want10)
        {
            double worstPct = 0.0;
            juce::String worst;
            for (const auto& p : subj)
                for (const auto& q : p.st)
                {
                    const double pct = q.writes ? 100.0 * (double) q.clamps / (2.0 * (double) q.writes) : 0.0;
                    if (pct >= worstPct) { worstPct = pct; worst = p.name + " s" + juce::String (q.slot + 1); }
                }
            const auto pkOff = found ? presetHolds ([] (Session& s) { s.hk().mkPeakOff = true; }) : std::vector<PHold>();
            double worstOff = 0.0;
            juce::String worstOffN;
            for (const auto& p : pkOff)
                for (const auto& q : p.st)
                {
                    const double pct = q.writes ? 100.0 * (double) q.clamps / (2.0 * (double) q.writes) : 0.0;
                    if (pct >= worstOff) { worstOff = pct; worstOffN = p.name + " s" + juce::String (q.slot + 1); }
                }
            row (stim && worstPct <= 1.0, "LH10 PD6 clamp pressure (the writer clamp hides an overdriving makeup): the 13 presets, 60 s, return samples clamped per feedback stream",
                 "meas worst " + f2 (worstPct, 3) + " % (" + worst + ") of the written samples, " + juce::String (streams) + " streams" + (stim ? "" : " INVALID")
                     + " | thr <= 1 % | margin " + f2 (1.0 - worstPct, 3) + " | ctrl LH10n");
            const bool fired = worstOff > 1.0;
            row (stim && fired, "LH10n control: mkPeakOff -> more than 1 % clamped on at least one stream (predicate must FAIL)",
                 "meas worst " + f2 (worstOff, 3) + " % (" + worstOffN + ") | thr predicate '<= 1 %' FAILS | " + ctrl (stim && fired, "LH10n mkPeakOff -> " + f2 (worstOff, 3) + " %"));
        }
    }

    //==========================================================================
    // LH8 / LH8i / LH14 — note-off: the stop-writing latch
    struct OffR
    {
        double atRelease = 0.0, after = 0.0;      // lane dB re t0 at the release and `wait` s later
        std::uint64_t wRelease = 0, wSoon = 0, wAfter = 0;
        bool latch = false, running = false, finite = true;
        double drop() const { return after - atRelease; }
    };

    /** preset < 0: the default patch arm `a`. Phrase 20 s, release, read `wait` s later. */
    OffR offRun (int preset, const Arm& a, bool pedal, const Hook& hook, double wait = 20.0)
    {
        OffR r;
        const double fs = preset < 0 ? a.fs : 48000.0;
        const juce::int64 off = (juce::int64) (20.0 * fs);
        std::unique_ptr<Session> own;
        std::unique_ptr<pg4::PSess> ps;
        Session* s = nullptr;
        int slot = 0;
        if (preset < 0)
        {
            own = open (a, s0Tape (a.fs), hook);
            s = own.get();
            chord (*s, off);
        }
        else
        {
            ps = std::make_unique<pg4::PSess>();
            ps->apply (preset);
            s = ps->s.get();
            s->record = false;
            if (hook) hook (*s);
            const auto fb = fbStreams (pg4::bank().specs[(size_t) preset]);
            slot = fb.empty() ? 0 : fb.front();
            presetChord (*s, off);
        }
        if (pedal)
            s->addMidi (off - 4800, juce::MidiMessage::controllerEvent (1, 64, 127));
        s->run (64, 32);
        const double l0 = dbOf (lane (*s, slot).ms);
        s->run (off - s->pos, 512);
        r.atRelease = dbOf (lane (*s, slot).ms) - l0;
        r.wRelease = s->hk().head[slot].writes;
        r.running = ! s->eng().getHeadForTest (slot).parked;
        s->run ((juce::int64) (1.0 * fs), 512);
        r.wSoon = s->hk().head[slot].writes;
        s->run ((juce::int64) ((wait - 1.0) * fs), 512);
        const auto l = lane (*s, slot);
        r.after = dbOf (l.ms) - l0;
        r.finite = l.finite;
        r.wAfter = s->hk().head[slot].writes;
        r.latch = s->eng().getMakeupLatchForTest (slot);
        return r;
    }

    void rowLH8()
    {
        const int halo = presetIndex ("Tape Halo");
        const Arm def { 100.0f, 20.0f };
        const Hook latchOff = [] (Session& s) { s.hk().mkLatchOff = true; };
        const auto a  = offRun (-1, def, false, {}),   ac = offRun (-1, def, false, latchOff);
        const auto b  = halo >= 0 ? offRun (halo, def, false, {}) : OffR(), bc = halo >= 0 ? offRun (halo, def, false, latchOff) : OffR();
        const auto p  = offRun (-1, def, true, {});
        const bool stim = halo >= 0 && a.running && b.running && p.running && a.wRelease > 0 && b.wRelease > 0 && a.latch && b.latch && ! ac.latch && ! bc.latch;
        const bool pedalOk = ! p.latch && (double) (p.wAfter - p.wSoon) > 0.9 * 19.0 * 48000.0 && p.after >= kLo && p.after <= kHi && p.finite;
        auto pred = [] (const OffR& x, const OffR& y) { return x.finite && y.finite && x.drop() >= -3.0 && y.drop() >= -3.0; };
        const bool ok = pred (a, b) && pedalOk;
        row (stim && ok, "LH8 PD9 note-off (20 s phrase, release, read 20 s later): the lane is left within -3 dB of its level at the release; under the pedal the head keeps writing",
             "meas default patch " + sg (a.drop()) + " dB (head wrote " + f2 ((double) (a.wAfter - a.wRelease) / 48000.0, 2) + " s after the release, latch set), Tape Halo " + sg (b.drop())
                 + " dB (" + f2 ((double) (b.wAfter - b.wRelease) / 48000.0, 2) + " s); pedal arm: lane " + sg (p.after) + " dB re t0, writes +" + f2 ((double) (p.wAfter - p.wSoon) / 48000.0, 1)
                 + " s over the 19 s, latch " + (p.latch ? "SET" : "clear") + (stim ? "" : " INVALID") + " | thr >= -3 dB; writes growing | margin "
                 + f2 (std::min (a.drop(), b.drop()) + 3.0, 2) + " | ctrl LH8n");
        const bool fired = ! pred (ac, bc);
        row (stim && fired, "LH8n control: mkLatchOff -> the dying return erases the lane after note-off (predicate must FAIL)",
             "meas default patch " + sg (ac.drop()) + " dB, Tape Halo " + sg (bc.drop()) + " dB | thr predicate '>= -3 dB' FAILS | "
                 + ctrl (stim && fired, "LH8n mkLatchOff -> " + sg (ac.drop()) + " / " + sg (bc.drop()) + " dB"));
    }

    void infoLH8i()
    {
        const int halo = presetIndex ("Tape Halo");
        Arm pre { 100.0f, 20.0f, 1, 100.0f, 100.0f };
        for (float db : { 6.0f, 3.0f })
        {
            const Hook h = [db] (Session& s) { s.hk().mkLatchDb = db; };
            const auto a = offRun (-1, Arm { 100.0f, 20.0f }, false, h);
            const auto b = halo >= 0 ? offRun (halo, Arm {}, false, h) : OffR();
            const auto c = offRun (-1, pre, false, h);
            std::printf ("  INFO LH8i latch threshold %.0f dB: lane 20 s after note-off re the release: default patch %+.1f dB, Tape Halo %+.1f dB, Pre Mix 100 / Size 100 %+.1f dB"
                         " (head wrote %.2f / %.2f / %.2f s after the release)\n", (double) db, a.drop(), b.drop(), c.drop(),
                         (double) (a.wAfter - a.wRelease) / 48000.0, (double) (b.wAfter - b.wRelease) / 48000.0, (double) (c.wAfter - c.wRelease) / 48000.0);
        }
        std::fflush (stdout);
    }

    struct L14 { bool stim = false; double latchMs = -1.0; std::uint64_t wA = 0; int voicesA0 = 0, voicesA1 = 0; double resumeMs = -1.0; bool latchAtB = false; };

    L14 lh14Run (bool onCounted, double latchAtMs)
    {
        L14 r;
        Arm a { 100.0f, 20.0f };
        a.release = 5000.0f;
        auto s = open (a, s0Tape(), [onCounted] (Session& ss) { ss.hk().mkLatchOnCounted = onCounted; });
        const juce::int64 off = 20 * 48000;
        chord (*s, off);
        s->run (off, 512);
        const bool running = ! s->eng().getHeadForTest (0).parked;
        // The latch sets some time into the release (the return falls 6 dB short for 50 ms).
        juce::int64 tLatch = -1;
        if (latchAtMs < 0.0)
        {
            for (int k = 0; k < 3 * 1500 && tLatch < 0; ++k)
            {
                s->run (32, 32);
                if (s->eng().getMakeupLatchForTest (0))
                    tLatch = s->pos;
            }
        }
        else
        {
            tLatch = off + (juce::int64) (latchAtMs * 48.0);
            s->run (tLatch - s->pos, 32);
        }
        if (tLatch < 0)
            return r;
        r.latchMs = (double) (tLatch - off) / 48.0;
        s->run (1920, 32);                                   // the 20 ms run ramp + park
        const auto w0 = s->hk().head[0].writes;
        r.voicesA0 = egr1::countedVoices (*s);
        s->run (48000, 512);                                 // (a) 1 s of the release
        r.voicesA1 = egr1::countedVoices (*s);
        r.wA = s->hk().head[0].writes - w0;
        // (b) a new note
        const juce::int64 tOn = s->pos + 17;
        s->noteOn (tOn, 62, 100);
        r.latchAtB = s->eng().getMakeupLatchForTest (0);
        const auto w1 = s->hk().head[0].writes;
        for (int k = 0; k < 200 && r.resumeMs < 0.0; ++k)
        {
            s->run (32, 32);
            if (s->hk().head[0].writes > w1)
                r.resumeMs = (double) (s->pos - tOn) / 48.0;
        }
        r.stim = running && r.voicesA0 >= 1 && r.voicesA1 >= 1;
        return r;
    }

    void rowLH14()
    {
        const auto r = lh14Run (false, -1.0);
        const auto c = lh14Run (true, r.latchMs);
        const bool stim = r.stim && c.stim && r.latchMs >= 0.0 && r.latchAtB;
        const double limitMs = 20.0 + 2.0 * 32.0 / 48.0;     // the 20 ms ramp + 1 cell (+ the 32-sample poll)
        const bool ok = r.wA == 0 && r.resumeMs >= 0.0 && r.resumeMs <= limitMs;
        row (stim && ok, "LH14 PD9 (pitfall 7) after the latch: (a) releasing voices (release 5 s) do not re-open the head; (b) a new note does, within the 20 ms ramp + 1 cell",
             "meas latch set " + f2 (r.latchMs, 1) + " ms after note-off; (a) writes in 1 s of release " + i64 (r.wA) + " (counted voices " + juce::String (r.voicesA0) + " -> " + juce::String (r.voicesA1)
                 + "); (b) writes resume " + f2 (r.resumeMs, 2) + " ms after the note-on" + (stim ? "" : " INVALID") + " | thr 0 writes; <= " + f2 (limitMs, 2) + " ms | margin "
                 + f2 (limitMs - r.resumeMs, 2) + " | ctrl LH14n");
        const bool fired = c.wA > 0;
        row (stim && fired, "LH14n control: mkLatchOnCounted (any counted voice clears the latch) -> the head writes during the release (predicate must FAIL)",
             "meas (a) writes in 1 s of release " + i64 (c.wA) + " | thr predicate '0 writes' FAILS | " + ctrl (stim && fired, "LH14n mkLatchOnCounted -> " + i64 (c.wA) + " frames written"));
    }

    //==========================================================================
    // LH9 — block-size invariance and determinism with the makeup on
    void rowLH9()
    {
        auto render = [] (int block, int pre)
        {
            P p { { "s1_feedback", 100.0f }, { "s1_fb_saturation", 20.0f }, { "s1_reverb_pre", (float) pre }, { "s1_reverb_mix", pre ? 50.0f : 0.0f },
                  { "s1_spray", 30.0f }, { "s1_scan_rate", 8.0f }, { "s1_scan_mode", 2.0f } };
            Session s (48000.0, p, kSeed, 512);
            shipping (s);
            s.load (0, noiseBuf (2, (int) (4.0 * 48000.0), 0.1f, 5), 48000.0);
            for (int note : { 48, 55, 60 }) s.noteOn (64, note, 100);
            s.run ((juce::int64) (12.0 * 48000.0), block);
            std::vector<float> v (s.L);
            v.insert (v.end(), s.R.begin(), s.R.end());
            const auto* t = s.eng().getCurrentTape (0);
            struct R { juce::String sha; std::uint64_t tape = 0, cells = 0; double g = 0.0; bool operator== (const R& o) const { return sha == o.sha && tape == o.tape; } };
            R r;
            r.sha = juce::SHA256 (v.data(), v.size() * sizeof (float)).toHexString().substring (0, 16);
            r.tape = t != nullptr ? t->tape.hashForTest() : 0;
            r.cells = s.hk().mk[0].cells;
            r.g = r.cells ? 20.0 * s.hk().mk[0].gLogSum / (double) r.cells : 0.0;
            return r;
        };
        bool ok = true, stim = true;
        juce::String d;
        for (int pre = 0; pre < 2; ++pre)
        {
            const auto a = render (512, pre), b = render (32, pre), c = render (1, pre), v = render (0, pre), again = render (512, pre);
            const bool eq = a == b && a == c && a == v && a == again;
            ok = ok && eq;
            stim = stim && a.cells > 10000 && a.g > 3.0;   // the makeup worked for most of the 12 s, above +3 dB on average
            d << (pre ? " Pre: " : " Post: ") << a.sha << " tape " << juce::String::toHexString ((juce::int64) a.tape) << (eq ? " == 32 / 1 / variable / repeat" : " DIFFERS")
              << " (mean makeup " << sg (a.g) << " dB over " << i64 (a.cells) << " cells);";
        }
        row (stim && ok, "LH9 PD6 makeup render (Amount 100 %, 12 s) at block 512 / 32 / 1 / seeded variable and a repeat: output and tape hashes equal, Post and Pre",
             "meas" + d + (stim ? "" : " INVALID (the makeup did not run)") + " | thr bitwise | ctrl - (LH1n: the makeup changes the render)");
    }

    //==========================================================================
    // LH13 — other sample rates
    void rowLH13()
    {
        bool stim = true, ok = true;
        juce::String d;
        double worst = 1.0e9;
        for (double fs : { 44100.0, 96000.0 })
        {
            Arm a { 100.0f, 20.0f };
            a.fs = fs;
            auto s = open (a, s0Tape (fs));
            chord (*s);
            const auto h = hold (*s, 0, 60.0, 8.0, 4.0, fs);
            stim = stim && wrote (h, 60.0, fs) && h.st.cells > 0;
            ok = ok && inBandB (h);
            worst = std::min ({ worst, h.mn - kDip, h.last - kLo, kHi - h.mx });
            d << " " << f2 (fs * 0.001, 1) << " kHz: " << holdText (h) << ";";
        }
        Arm a96 { 100.0f, 20.0f };
        a96.fs = 96000.0;
        const auto o = offRun (-1, a96, false, {});
        stim = stim && o.running && o.latch;
        ok = ok && o.finite && o.drop() >= -3.0;
        row (stim && ok, "LH13 PD7 every constant is in seconds: S0 (Saturate 20) held at 44.1 and 96 kHz (readings every lap from 8 s); LH8's default arm at 96 kHz",
             "meas" + d + " note-off at 96 kHz: " + sg (o.drop()) + " dB after 20 s (latch set)" + (stim ? "" : " INVALID") + " | thr " + sg (kLo, 0) + " / " + sg (kHi, 0)
                 + " dB at the end, no reading under " + sg (kDip, 0) + " or over " + sg (kHi, 0) + " dB; note-off >= -3 dB | margin " + f2 (std::min (worst, o.drop() + 3.0), 2) + " | ctrl - (LH1n, LH8n)");
    }

    //==========================================================================
    // LHF — no allocation, no free on the audio thread
   #if JUCE_MAC
    volatile int lhfAllocs = 0, lhfFrees = 0;
    void lhfLogger (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        if (eg21::allocArmed && pthread_equal (pthread_self(), eg21::audioThread))
        {
            if ((type & 2u) != 0) { const int v = lhfAllocs + 1; lhfAllocs = v; }   // MALLOC_LOG_TYPE_ALLOCATE
            if ((type & 4u) != 0) { const int v = lhfFrees + 1; lhfFrees = v; }    // MALLOC_LOG_TYPE_DEALLOCATE
        }
    }
   #endif

    void rowLHF()
    {
       #if JUCE_MAC
        using namespace eg21;
        audioThread = pthread_self();
        malloc_logger = lhfLogger;
        lhfAllocs = 0; lhfFrees = 0;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        std::free (probe);
        allocArmed = false;
        const int liveA = lhfAllocs, liveF = lhfFrees;
        lhfAllocs = 0; lhfFrees = 0;

        // Record-in into slot 1 (stream 1), Manual; stream 1 feeds back at Amount 100 %.
        Session s (48000.0, eg22::cat ({ egr1::recBase (0, 0), { { "s1_feedback", 100.0f }, { "s1_fb_saturation", 20.0f } } }), kSeed, 512);
        shipping (s);
        s.record = false;
        s.load (0, s0Tape(), 48000.0);
        const eg26a::ScSig g { 0.3f, 977 };
        eg26a::inject (s, g);
        chord (s);
        s.arm = true;
        s.run (20 * 48000, 512);                           // S0 for 20 s: the head writes with the makeup
        const auto w20 = s.hk().head[0].writes;
        const auto cells20 = s.hk().mk[0].cells;
        const auto taken0 = s.hk().tapesTaken;
        s.arm = false;
        s.p->resetSlotSync (0);                            // Reset (message-thread side, not armed)
        s.arm = true;
        s.run (48000, 512);                                // the take lands; the head resumes on the fresh copy
        const auto taken1 = s.hk().tapesTaken;
        const auto w21 = s.hk().head[0].writes;
        s.arm = false;
        const juce::int64 tArm = s.pos + 4807 - (s.pos % 32);
        eg26a::armAt (s, tArm);
        s.arm = true;
        s.run (tArm - s.pos + 48000 + 2000, 512);          // a 1 s Manual take
        const auto p1 = eg26a::pr (s);
        s.arm = false;
        const bool pumped = egr1::pumpTimer (s, false);    // the Timer commits (message thread)
        s.arm = true;
        s.run (2 * 48000, 512);                            // the new tape is taken; the head writes on it
        s.arm = false;
        malloc_logger = nullptr;
        const auto taken2 = s.hk().tapesTaken;
        const auto w2 = s.hk().head[0].writes;
        const auto commits = eg26a::rec (s).hooks.recCommits;

        const bool exercised = w20 > 15 * 48000 && cells20 > 0 && taken1 == taken0 + 1 && w21 > w20 && p1.state == ogi::RecState::Committing && pumped
                            && commits == 1 && taken2 == taken1 + 1 && w2 > w21;
        const int a = lhfAllocs, f = lhfFrees;
        row (liveA == 1 && liveF == 1 && exercised && a == 0 && f == 0,
             "LHF PERF-01 0 audio-thread allocations and 0 frees: S0 for 20 s with the makeup, Reset, a Record-in commit, the head writing throughout",
             "meas " + juce::String (a) + " allocs, " + juce::String (f) + " frees over " + i64 ((std::uint64_t) s.blocksDone) + " blocks (writes " + i64 (w20) + " -> " + i64 (w21) + " -> " + i64 (w2)
                 + ", makeup cells at 20 s " + i64 (cells20) + ", tapes taken " + i64 (taken2 - taken0) + ", commits " + i64 (commits) + ")" + (exercised ? "" : " INVALID")
                 + " | thr 0 / 0 | margin " + juce::String (-(a + f)) + " | ctrl LHFn");
        const bool fired = liveA == 1 && liveF == 1;
        row (fired, "LHFn control: one armed malloc (64) + free on this thread are counted (predicate '0 / 0' must FAIL)",
             "meas " + juce::String (liveA) + " alloc, " + juce::String (liveF) + " free counted | thr predicate '0 allocations, 0 frees' FAILS | "
                 + ctrl (fired, "LHFn liveness malloc(64) + free armed -> " + juce::String (liveA) + " / " + juce::String (liveF)));
       #else
        row (false, "LHF alloc gate is macOS-only");
        row (false, "LHFn alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    // LH11 (INFO, --egr2-long): 600 s drift on four presets
    void infoLH11()
    {
        const char* names[] = { "Iron Sky", "Tape Halo", "Fifth Bloom", "Dotted Echo" };
        std::vector<juce::String> lines (4);
        pg4::parallelFor (4, [&] (int k)
        {
            const int i = presetIndex (names[k]);
            if (i < 0) { lines[(size_t) k] = juce::String (names[k]) + ": not found"; return; }
            pg4::PSess ps;
            ps.apply (i);
            ps.s->record = false;
            const auto fb = fbStreams (pg4::bank().specs[(size_t) i]);
            const int slot = fb.empty() ? 0 : fb.front();
            presetChord (*ps.s);
            ps.s->run (64, 32);
            const double l0 = dbOf (lane (*ps.s, slot).ms);
            double mn = 1.0e9, mx = -1.0e9, last = 0.0;
            for (int q = 1; q <= 20; ++q)
            {
                ps.s->run ((juce::int64) (30.0 * 48000.0) - (q == 1 ? 64 : 0), 512);
                last = dbOf (lane (*ps.s, slot).ms) - l0;
                mn = std::min (mn, last); mx = std::max (mx, last);
            }
            lines[(size_t) k] = juce::String (names[k]) + " s" + juce::String (slot + 1) + ": min " + sg (mn) + " max " + sg (mx) + " at 600 s " + sg (last) + " dB";
        });
        for (const auto& l : lines)
            std::printf ("  INFO LH11 600 s held chord, lane dB re t0 every 30 s: %s\n", l.toRawUTF8());
        std::fflush (stdout);
    }

    //==========================================================================
    void runAll (bool longRows = false)
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        section ("R2  feedback holds the lane level: the makeup (PLAN-R2 PD11)");
        if (want ("LH1"))  rowLH1();
        if (want ("LH1p")) rowLH1p();
        if (want ("LH2"))  rowLH2();
        if (want ("LH3"))  rowLH3();
        if (want ("LH4"))  rowLH4();
        if (want ("LH5"))  rowLH5();
        if (want ("LH6"))  rowLH6();
        if (want ("LH6u")) rowLH6u();
        if (want ("LH7"))  rowLH7();
        if (want ("LH9"))  rowLH9();
        if (want ("LH12")) rowLH12();
        if (want ("LH13")) rowLH13();
        section ("R2  feedback holds the lane level: note-off, the stop-writing latch (PLAN-R2 PD9)");
        if (want ("LH8"))  rowLH8();
        if (want ("LH8i")) infoLH8i();
        if (want ("LH14")) rowLH14();
        section ("R2  feedback holds the lane level: the bank, allocation (PLAN-R2 PD11)");
        rowsPresets();   // LHB, LH10, LHg
        if (want ("LHF"))  rowLHF();
        if (longRows && want ("LH11")) infoLH11();
        std::printf ("\n-- R2 PD11 rows done (%.1f s)\n", (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
    }
} }
