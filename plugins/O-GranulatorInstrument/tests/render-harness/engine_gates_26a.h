/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.6 engine gates,
    part a (PLAN-2.6 T12): Record-in capture, triggers, one-shot arm, length,
    presence, sidechain leaks, commit paths. Included by main.cpp AFTER
    engine_gates_25d.h. The sidechain is injected into the in-place host buffer
    through Session::beforeBlock. Every row prints its control.

      RC1   Manual x slots 0-3, stereo noise SC, rec_gain +6 dB: tape level 0 ==
            sc x g bitwise over [start, start + frames)    ctrl RC1n startOffset
      RC2   Threshold x slots 0-3 (pre-roll included)
      RC3   Next Note x slots 0-3 (from the note sample)
      RC4   a recorded slot plays a 4-note chord at 4 rates; mono SC -> 1-ch pristine
      TH1   crossing at arm + 200 ms -> start == crossing - P   ctrl TH1n prerollOff
      TH2   crossing at arm + 3 ms -> start == the arm sample (clip)
      TH3   Manual == arm sample; Next Note == note sample; pre-roll counts toward length
      OS1   arm held On after a commit stays Idle 2 s       ctrl OS1n armLevelTrigger
      OS2   Off -> On again: a second capture lands
      OS3   Off while Armed: Idle, Cancelled, slot unchanged
      OS4   Off while Recording at 0.5 s commits / 0.49 s discards (TooShort)
      OS5   an arm edge during Committing is held -> Armed at the block start after the copy-out
      LN1   Free, rec_length 1.25 s -> llround (1.25 fs) frames
      LN2   Tempo + playing, 2 bars @ 120 bpm 4/4, tempo change mid-take ignored
      LN3   Tempo + stopped -> seconds
      LN4   cap at 44.1 / 96 / 192 k, rec_length 30 s       ctrl LN4n capOff -> overrun
      MN1   Next Note: Mono retrigger counts; Legato move + D30 fallback do not
                                                            ctrl MN1n nnAnyNote
      QD1   silent SC: Manual / Next Note discard NoSignal, Threshold stays Armed;
            slot tape + hash unchanged                      ctrl QD1n -89 dBFS commits
      QD2   bus disabled: arm inert (SidechainUnavailable); enabling later does nothing
      QD3   -89 / -91 dBFS takes commit / discard
      LK1   loud SC x {stereo, mono + garbage, disabled} x {Idle, Armed, Recording,
            Committing}: output == the SC-silent render bitwise; no notes -> +0.0f
                                                            ctrl LK1n scNoClear
      LK2   processBlockBypassed with loud SC, 3 layouts -> +0.0f
                                                            ctrl LK2n baseBypass (the pre-2.6 leak)
      CM1   commit via commitSync and via the Timer + pump: tape, WAV in the
            (non-ASCII) Recordings dir, source rec:<path>
      CM2   setNonRealtime (true) + pump -> the commit lands
      CM3   host idle mid-commit / mid-swap, 50 relinks + 50 restores pumped:
            pending <= 1, tapes bounded, resume drains     ctrl CM3n testNoEntryCounter
      CM4   prepare mid-Recording (44.1 -> 96 k) commits at 44.1 k; Armed -> Interrupted;
            Committing survives
      CM5   bypassed blocks: Armed -> Bypassed; Recording >= 0.5 s commits
      CM6   WAV write fails: still publishes, wavFailed, blob carries the take, reason WavFailed

  ==============================================================================
*/
#pragma once

namespace { namespace eg26a
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
    using eg21::sameF;
    using eg21::diffCount;
    using eg21::hashNoise;
    using eg22::cat;
    using eg22::sn;
    using eg24b::want;
    using eg24b::section;
    using eg25a::baseMod;
    using eg25c::runTo;
    using RS = ogi::RecState;
    using RR = ogi::RecReason;
    using CS = juce::AudioChannelSet;

    constexpr double kFs = 48000.0;

    //==========================================================================
    // Helpers
    ogi::GiRecorder&   rec (Session& s) { return s.eng().getRecorder(); }
    ogi::RecorderProbe pr (Session& s)  { return rec (s).probe(); }

    const char* stName (RS st)
    {
        switch (st) { case RS::Idle: return "Idle"; case RS::Armed: return "Armed"; case RS::Recording: return "Recording"; case RS::Committing: return "Committing"; }
        return "?";
    }
    const char* rsName (RR r)
    {
        switch (r)
        {
            case RR::None: return "None"; case RR::SidechainUnavailable: return "SidechainUnavailable"; case RR::NoSignal: return "NoSignal";
            case RR::TooShort: return "TooShort"; case RR::Cancelled: return "Cancelled"; case RR::Interrupted: return "Interrupted";
            case RR::Bypassed: return "Bypassed"; case RR::Committed: return "Committed"; case RR::WavFailed: return "WavFailed";
        }
        return "?";
    }

    /** The injected sidechain: seeded noise (+ an optional sine) inside [from, to). */
    struct ScSig
    {
        float amp = 0.5f;
        int   salt = 0;
        juce::int64 from = 0, to = std::numeric_limits<juce::int64>::max();
        double sineHz = 0.0;
        float  sineAmp = 0.0f;
        float  dc = 0.0f;           // a constant (QD3 levels)

        float at (juce::int64 t, int ch) const
        {
            if (t < from || t >= to)
                return 0.0f;
            float v = amp > 0.0f ? hashNoise (t + (juce::int64) salt * 1000003, ch, amp) : 0.0f;
            if (sineAmp > 0.0f)
                v += (float) ((double) sineAmp * std::sin (2.0 * juce::MathConstants<double>::pi * sineHz * (double) t / kFs));
            return v + dc;
        }
    };

    /** beforeBlock: the host buffer carries g; garbageCh (0 / 1) gets 12345.0f. */
    void inject (Session& s, const ScSig& g, int garbageCh = -1, std::function<void (Session&, juce::int64, int)> also = {})
    {
        s.beforeBlock = [g, garbageCh, also] (Session& ss, juce::int64 b0, int n)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                float* d = ss.blk.getWritePointer (ch);
                for (int i = 0; i < n; ++i)
                    d[i] = ch == garbageCh ? 12345.0f : g.at (b0 + i, ch);
            }
            if (also)
                also (ss, b0, n);
        };
    }

    void setLayout (Session& s, const CS& in)
    {
        s.p->setBusesLayout (layout (in, CS::stereo()));
        s.p->prepareToPlay (s.eng().getSampleRate(), 512);
    }

    /** The record baseline: one stream, no modulation, gain 0 dB, 1 s Manual. */
    P recBase (int slot, int trigger, const P& over = {})
    {
        return with (cat ({ baseMod (1), { { "rec_target", (float) slot }, { "rec_trigger", (float) trigger },
                                          { "rec_gain", 0.0f }, { "rec_length", 1.0f }, { "rec_threshold", -30.0f },
                                          { "rec_arm", 0.0f }, { "sync_mode", 0.0f } } }),
                     over);
    }

    void armAt (Session& s, juce::int64 t, bool on = true)
    {
        s.at (t, [on] (Session& ss) { setP (*ss.p, "rec_arm", on ? 1.0f : 0.0f); });
    }

    /** The take's expected sample: sc · g (float), mono → ch 0. */
    float xAt (const ScSig& g, juce::int64 t, int ch, float gain) { return g.at (t, ch) * gain; }

    /** Mismatches of the slot's level-0 tape against sc · g over [start, start + frames).
        −1: no tape; −2: frame count differs. */
    int tapeDiff (Session& s, int slot, const ScSig& g, float gain, juce::int64 start, int frames, int channels)
    {
        const ogi::SlotTape* tp = s.eng().getCurrentTape (slot);
        if (tp == nullptr)
            return -1;
        if (tp->frames != frames)
            return -2;
        int bad = 0;
        for (int i = 0; i < frames; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const float want = xAt (g, start + i, channels > 1 ? ch : 0, gain);
                bad += sameF (tp->tape.levelSample (0, ch, i), want) ? 0 : 1;
            }
        return bad;
    }

    /** commitSync + a few blocks so the engine takes the tape. */
    bool land (Session& s)
    {
        const bool ok = s.p->commitSync();
        s.run (256, 64);
        return ok;
    }

    /** The first sample t >= from with max |x| > thr (the recorder's strict test). */
    juce::int64 crossing (const ScSig& g, juce::int64 from, float gain, float thrDb, int channels = 2)
    {
        const float thr = juce::Decibels::decibelsToGain (thrDb);
        for (juce::int64 t = from; t < from + (juce::int64) (10 * kFs); ++t)
        {
            const float a = std::max (std::abs (xAt (g, t, 0, gain)), channels > 1 ? std::abs (xAt (g, t, 1, gain)) : 0.0f);
            if (a > thr)
                return t;
        }
        return -1;
    }

    struct TakeResult
    {
        ogi::RecorderProbe probe;   // at the stop (before the commit)
        bool committed = false;
        int  diff = 0;
        int  frames = 0;
        float gain = 1.0f;          // the recorder's law on the RAW rec_gain (dbToGain, snapDb)
    };

    /** One take: trigger 0 Manual / 1 Threshold / 2 Next Note on `slot`. */
    TakeResult takeOnce (int slot, int trigger, const ScSig& g, float gainDb, juce::int64 armT, juce::int64 noteT,
                         const std::function<void (Session&)>& tweak = {}, double lenSec = 1.0)
    {
        Session s (kFs, recBase (slot, trigger, { { "rec_gain", gainDb }, { "rec_length", (float) lenSec } }), kSeed, 512);
        if (tweak)
            tweak (s);
        inject (s, g);
        armAt (s, armT);
        if (noteT >= 0)
            s.noteOn (noteT, 60);
        s.run (armT + (juce::int64) (lenSec * kFs) + 30000, 512);
        TakeResult r;
        r.gain = ogi::dbToGain (eg21::rawOf (*s.p, "rec_gain"));
        r.probe = pr (s);
        r.committed = land (s);
        r.frames = r.probe.frames;
        r.diff = r.committed ? tapeDiff (s, slot, g, r.gain, r.probe.start, r.probe.frames, 2) : -3;
        return r;
    }

    //==========================================================================
    // RC — capture by every trigger into every slot (FUNC-07)
    void rowsRC()
    {
        const juce::int64 A = 4807;   // off the 32-grid

        for (int trig = 0; trig < 3; ++trig)
        {
            Tally t;
            juce::String d;
            for (int slot = 0; slot < 4; ++slot)
            {
                ScSig g { 0.5f, 3 + slot };
                if (trig == 1)
                    g.from = A + 9600;   // silent until 200 ms after the arm
                const juce::int64 noteT = trig == 2 ? A + 1000 : -1;
                const auto r = takeOnce (slot, trig, g, 6.0f, A, noteT);
                juce::int64 wantStart = A;
                if (trig == 1)
                    wantStart = crossing (g, A, r.gain, -30.0f) - 480;
                if (trig == 2)
                    wantStart = noteT;
                t.check (r.committed && r.diff == 0 && r.probe.start == wantStart && r.frames == 48000 && std::abs (r.gain - 1.99526f) < 1.0e-4f,
                         "slot " + juce::String (slot) + " diff " + juce::String (r.diff) + " start " + i64 ((std::uint64_t) r.probe.start)
                             + " want " + i64 ((std::uint64_t) wantStart) + " frames " + juce::String (r.frames));
                d << " s" << slot << ":" << r.diff << "@" << (juce::int64) r.probe.start;
                if (slot == 0)
                    d << " (g " << juce::String (r.gain, 6) << ")";
            }
            const char* ids[3] = { "RC1", "RC2", "RC3" };
            const char* names[3] = { "Manual", "Threshold (pre-roll incl.)", "Next Note" };
            if (trig == 0)
            {
                ScSig g { 0.5f, 3 };
                const auto rn = takeOnce (0, 0, g, 6.0f, A, -1, [] (Session& s) { rec (s).hooks.startOffset = true; });
                // The control's tape compared against the UNSHIFTED reference.
                const bool fired = rn.committed && rn.probe.start == A + 1;
                row (t.bad == 0 && fired, juce::String (ids[trig]) + " FUNC-07 " + names[trig] + " x slots 0-3: tape == sc x g bitwise (+6 dB)",
                     "meas " + t.detail (4) + " | diffs" + d + " | thr 0 diffs, 48000 frames, start == arm | margin 0 | "
                         + ctrl (fired, "RC1n startOffset -> start " + i64 ((std::uint64_t) rn.probe.start) + " (arm " + i64 ((std::uint64_t) A) + ")"));
            }
            else
            {
                row (t.bad == 0, juce::String (ids[trig]) + " FUNC-07 " + names[trig] + " x slots 0-3: tape == sc x g bitwise (+6 dB)",
                     "meas " + t.detail (4) + " | diffs" + d + " | thr 0 diffs, 48000 frames | ctrl - (RC1n)");
            }
        }
    }

    void rowRC4()
    {
        // (a) the recorded slot plays a 4-note chord at 4 distinct rates.
        const juce::int64 A = 4807;
        Session s (kFs, recBase (0, 0, { { sn (1, "density"), 100.0f }, { sn (1, "size"), 40.0f } }), kSeed, 512);
        ScSig g { 0.5f, 11 };
        inject (s, g);
        armAt (s, A);
        s.run (A + 48000 + 4000, 512);
        const bool committed = land (s);
        s.hk().resetCounters();
        const juce::int64 t0 = s.pos + 64;
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (t0, n);
        s.run (24000, 256);
        std::set<double> rates;
        int grains = 0;
        for (const auto& e : s.hk().logInOrder())
            if (e.stream == 0 && e.onsetSample >= t0)
            {
                rates.insert (e.rate);
                ++grains;
            }

        // (b) a mono sidechain → a 1-channel pristine; tape L == R == x.
        Session m (kFs, recBase (1, 0), kSeed, 512);
        setLayout (m, CS::mono());
        ScSig gm { 0.5f, 12 };
        inject (m, gm, 1);   // ch 1 = host garbage past a mono SC
        armAt (m, A);
        m.run (A + 48000 + 4000, 512);
        const bool mc = land (m);
        const auto pm = m.p->getLoader().getPristine (1);
        const int chans = pm != nullptr ? pm->pcm.getNumChannels() : 0;
        const int md = mc ? tapeDiff (m, 1, gm, 1.0f, pr (m).start, pr (m).frames, 1) : -3;

        row (committed && (int) rates.size() == 4 && mc && chans == 1 && md == 0,
             "RC4 recorded slot plays a 4-note chord (4 rates); mono SC -> 1-ch pristine",
             "meas " + juce::String ((int) rates.size()) + " distinct rates over " + juce::String (grains) + " grains; mono pristine "
                 + juce::String (chans) + " ch, tape L == R == x diffs " + juce::String (md) + " | thr 4 rates, 1 ch, 0 diffs | ctrl -");
    }

    //==========================================================================
    // TH — Threshold pre-roll / clip; Manual / Next Note exactness (D35)
    void rowsTH()
    {
        const juce::int64 A = 9613;
        const float thrDb = -30.0f;
        {
            ScSig g { 0.5f, 21 };
            g.from = A + 9600;
            const auto r  = takeOnce (2, 1, g, 0.0f, A, -1);
            const auto rn = takeOnce (2, 1, g, 0.0f, A, -1, [] (Session& s) { rec (s).hooks.prerollOff = true; });
            const juce::int64 c = crossing (g, A, 1.0f, thrDb);
            const bool fired = rn.probe.start == c;
            row (r.probe.start == c - 480 && r.diff == 0 && fired, "TH1 D35 Threshold start == crossing - 10 ms (P = 480 @ 48 k)",
                 "meas start " + i64 ((std::uint64_t) r.probe.start) + ", crossing " + i64 ((std::uint64_t) c) + ", pre-roll "
                     + i64 ((std::uint64_t) (c - r.probe.start)) + ", diffs " + juce::String (r.diff) + " | thr == crossing - 480 | margin 0 | "
                     + ctrl (fired, "TH1n prerollOff -> start == crossing (" + i64 ((std::uint64_t) rn.probe.start) + ")"));
        }
        {
            ScSig g { 0.5f, 22 };
            g.from = A + 144;   // 3 ms
            const auto r = takeOnce (2, 1, g, 0.0f, A, -1);
            const juce::int64 c = crossing (g, A, 1.0f, thrDb);
            row (r.probe.start == A && r.diff == 0 && c - A < 480, "TH2 D35 crossing at arm + 3 ms -> start clipped at the arm sample",
                 "meas start " + i64 ((std::uint64_t) r.probe.start) + " (arm " + i64 ((std::uint64_t) A) + ", crossing +" + i64 ((std::uint64_t) (c - A))
                     + "), diffs " + juce::String (r.diff) + " | thr == arm | ctrl - (TH1n)");
        }
        {
            ScSig g { 0.5f, 23 };
            const auto rm = takeOnce (3, 0, g, 0.0f, A, -1);
            const auto rn = takeOnce (3, 2, g, 0.0f, A, A + 3333);
            ScSig gt { 0.5f, 24 };
            gt.from = A + 9600;
            const auto rt = takeOnce (3, 1, gt, 0.0f, A, -1);
            const juce::int64 c = crossing (gt, A, 1.0f, thrDb);
            const bool ok = rm.probe.start == A && rn.probe.start == A + 3333 && rt.frames == 48000
                         && rt.probe.stop == rt.probe.start + 48000 && rt.probe.start == c - 480;
            row (ok, "TH3 Manual == arm sample; Next Note == note sample; pre-roll counts toward length",
                 "meas Manual +" + i64 ((std::uint64_t) (rm.probe.start - A)) + ", Next Note " + i64 ((std::uint64_t) rn.probe.start) + " (note "
                     + i64 ((std::uint64_t) (A + 3333)) + "), Threshold frames " + juce::String (rt.frames) + " stop - start "
                     + i64 ((std::uint64_t) (rt.probe.stop - rt.probe.start)) + " | thr +0, == note, 48000 | ctrl -");
        }
    }

    //==========================================================================
    // OS — one-shot arm (D34)
    void rowsOS()
    {
        const juce::int64 A = 4807;
        auto oneShot = [&] (bool levelHook, std::uint64_t& commits, std::uint64_t& arms, RS& st)
        {
            Session s (kFs, recBase (0, 0), kSeed, 512);
            rec (s).hooks.armLevelTrigger = levelHook;
            inject (s, ScSig { 0.5f, 31 });
            armAt (s, A);
            s.run (A + 48000 + 2000, 512);
            land (s);
            s.run ((juce::int64) (2 * kFs), 512);   // arm still On
            commits = rec (s).hooks.recCommits;
            arms = rec (s).hooks.recArms;
            st = pr (s).state;
        };
        std::uint64_t c1 = 0, a1 = 0, cn = 0, an = 0;
        RS s1 = RS::Idle, sn_ = RS::Idle;
        oneShot (false, c1, a1, s1);
        oneShot (true, cn, an, sn_);
        const bool fired = an > 1;
        row (c1 == 1 && a1 == 1 && s1 == RS::Idle && fired, "OS1 D34 one-shot: arm held On after a commit stays Idle (2 s; engine level, no pump)",
             "meas commits " + i64 (c1) + ", arms " + i64 (a1) + ", state " + stName (s1) + " | thr 1 / 1 / Idle | "
                 + ctrl (fired, "OS1n armLevelTrigger -> " + i64 (an) + " arms, state " + stName (sn_)));

        // OS2: Off → On again captures a second take (another seed region).
        {
            Session s (kFs, recBase (0, 0), kSeed, 512);
            ScSig g { 0.5f, 32 };
            inject (s, g);
            armAt (s, A);
            s.run (A + 48000 + 2000, 512);
            land (s);
            const auto* first = s.eng().getCurrentTape (0);
            const juce::int64 B = s.pos + 1000;
            armAt (s, B, false);
            armAt (s, B + 777);
            s.run (B + 777 + 48000 + 2000 - s.pos, 512);
            const auto p2 = pr (s);
            const bool ok2 = land (s);
            const int d2 = tapeDiff (s, 0, g, 1.0f, p2.start, p2.frames, 2);
            row (ok2 && d2 == 0 && p2.start == B + 777 && s.eng().getCurrentTape (0) != first && rec (s).hooks.recCommits == 2,
                 "OS2 D34 Off -> On again: a second capture lands",
                 "meas commits " + i64 (rec (s).hooks.recCommits) + ", second start " + i64 ((std::uint64_t) p2.start) + " (edge "
                     + i64 ((std::uint64_t) (B + 777)) + "), diffs " + juce::String (d2) + " | thr 2 commits, 0 diffs | ctrl -");
        }

        // OS3: Off while Armed (Threshold on a silent SC) → Idle, Cancelled, slot unchanged.
        {
            Session s (kFs, recBase (0, 1), kSeed, 512);
            s.load (0, noiseBuf (2, 48000, 0.2f, 5), kFs);
            s.run (512, 512);
            const auto* before = s.eng().getCurrentTape (0);
            inject (s, ScSig { 0.0f, 0 });
            armAt (s, s.pos + 1000);
            armAt (s, s.pos + 9000, false);
            s.run (20000, 512);
            const auto p = pr (s);
            row (p.state == RS::Idle && p.reason == RR::Cancelled && s.eng().getCurrentTape (0) == before && rec (s).hooks.recCancels == 1,
                 "OS3 D34 Off while Armed: Idle, Cancelled, slot unchanged",
                 juce::String ("meas state ") + stName (p.state) + ", reason " + rsName (p.reason) + ", tape same "
                     + (s.eng().getCurrentTape (0) == before ? "yes" : "no") + " | thr Idle / Cancelled / same | ctrl -");
        }

        // OS4: Off while Recording at 0.5 s commits; at 0.49 s discards (TooShort).
        {
            auto stopAfter = [&] (juce::int64 len, RR& reason, std::uint64_t& commits, int& frames)
            {
                Session s (kFs, recBase (1, 0, { { "rec_length", 10.0f } }), kSeed, 512);
                inject (s, ScSig { 0.5f, 33 });
                armAt (s, A);
                armAt (s, A + len, false);
                s.run (A + len + 2000, 512);
                reason = pr (s).reason;
                commits = rec (s).hooks.recCommits;
                frames = pr (s).frames;
            };
            RR r1 = RR::None, r2 = RR::None;
            std::uint64_t c1x = 0, c2x = 0;
            int f1 = 0, f2x = 0;
            stopAfter (24000, r1, c1x, f1);
            stopAfter (23520, r2, c2x, f2x);
            row (r1 == RR::Committed && c1x == 1 && f1 == 24000 && r2 == RR::TooShort && c2x == 0,
                 "OS4 D34 Off while Recording: 0.5 s commits, 0.49 s discards (TooShort)",
                 juce::String ("meas 0.50 s ") + rsName (r1) + " (" + juce::String (f1) + " frames), 0.49 s " + rsName (r2) + " (" + juce::String (f2x)
                     + ") | thr Committed / TooShort, min 24000 | ctrl -");
        }

        // OS5: an arm edge during Committing is held → Armed at the first block start after the copy-out.
        {
            Session s (kFs, recBase (0, 0, { { "rec_length", 0.6f } }), kSeed, 512);
            inject (s, ScSig { 0.5f, 34 });
            armAt (s, A);
            s.run (A + 28800 + 1000, 512);
            const auto pc = pr (s);
            armAt (s, s.pos + 100, false);
            armAt (s, s.pos + 200);
            s.run (1000, 512);
            const auto ph = pr (s);   // still Committing, held
            const bool copied = s.p->commitSync();
            const juce::int64 next = s.pos;
            s.run (512, 512);
            const auto pa = pr (s);
            row (pc.state == RS::Committing && ph.state == RS::Committing && ph.heldArm && copied
                     && (pa.state == RS::Recording || pa.state == RS::Armed) && pa.armAt == next && rec (s).hooks.heldArms == 1,
                 "OS5 arm edge during Committing is held -> Armed at the next block start",
                 juce::String ("meas ") + stName (pc.state) + " -> held " + (ph.heldArm ? "yes" : "no") + " -> " + stName (pa.state) + " armAt "
                     + i64 ((std::uint64_t) pa.armAt) + " (block " + i64 ((std::uint64_t) next) + ") | thr held, armAt == block start | ctrl -");
        }
    }

    //==========================================================================
    // LN — length (seconds / bars / cap)
    void rowsLN()
    {
        const juce::int64 A = 4807;
        {
            const auto r = takeOnce (0, 0, ScSig { 0.5f, 41 }, 0.0f, A, -1, {}, 1.25);
            row (r.frames == 60000 && r.committed && r.diff == 0, "LN1 Free rec_length 1.25 s -> llround (1.25 fs) frames",
                 "meas " + juce::String (r.frames) + " frames, diffs " + juce::String (r.diff) + " | thr 60000 | margin 0 | ctrl -");
        }
        // LN2 / LN3: Tempo, 2 bars @ 120 bpm 4/4 = 4 s, a tempo change mid-take ignored; stopped → seconds.
        auto tempoTake = [&] (bool playing, int& frames, juce::int64& start)
        {
            Session s (kFs, recBase (0, 0, { { "sync_mode", 1.0f }, { "rec_length_sync", 1.0f }, { "rec_length", 1.0f } }), kSeed, 512);
            auto ph = std::make_shared<MockPlayHead>();
            ph->sampleRate = kFs;
            MockPlayHead::Seg a;
            a.at = 0; a.ppq0 = 0.0; a.bpm = 120.0; a.playing = playing;
            ph->script.push_back (a);
            MockPlayHead::Seg b = a;
            b.at = A + 96000;
            b.ppq0 = playing ? ph->ppqAt (b.at) : 0.0;
            b.bpm = 90.0;
            ph->script.push_back (b);
            s.p->setPlayHead (ph.get());
            inject (s, ScSig { 0.5f, 42 }, -1, [ph] (Session&, juce::int64 b0, int) { ph->timeInSamples = b0; });
            armAt (s, A);
            s.run (A + (juce::int64) (5 * kFs), 512);
            frames = pr (s).frames;
            start = pr (s).start;
            s.p->setPlayHead (nullptr);
        };
        int fp = 0, fs = 0;
        juce::int64 sp = 0, ss = 0;
        tempoTake (true, fp, sp);
        tempoTake (false, fs, ss);
        row (fp == 192000, "LN2 Tempo + playing: 2 bars @ 120 bpm 4/4, tempo change mid-take ignored",
             "meas " + juce::String (fp) + " frames (bar latched at the start; 90 bpm from +2 s) | thr 192000 | margin 0 | ctrl LN3 stopped");
        row (fs == 48000, "LN3 Tempo + stopped -> seconds", "meas " + juce::String (fs) + " frames | thr 48000 (rec_length 1 s) | ctrl -");

        // LN4: the cap at 44.1 / 96 / 192 k with rec_length 30 s.
        Tally t;
        juce::String d;
        const double rates[3] = { 44100.0, 96000.0, 192000.0 };
        const int want[3] = { 1323000, 2880000, 2880000 };
        for (int k = 0; k < 3; ++k)
        {
            Session s (rates[k], recBase (0, 0, { { "rec_length", 30.0f } }), kSeed, 4096);
            inject (s, ScSig { 0.5f, 43 });
            armAt (s, 1000);
            s.run (1000 + (juce::int64) want[k] + 8192, 4096);
            const auto p = pr (s);
            t.check (p.frames == want[k] && p.state == RS::Committing, juce::String (rates[k] / 1000.0, 1) + " k: " + juce::String (p.frames));
            d << " " << juce::String (rates[k] / 1000.0, 1) << "k:" << p.frames;
        }
        std::uint64_t overruns = 0;
        {
            Session s (192000.0, recBase (0, 0, { { "rec_length", 30.0f } }), kSeed, 4096);
            rec (s).hooks.capOff = true;
            inject (s, ScSig { 0.5f, 43 });
            armAt (s, 1000);
            s.run (1000 + 2880000 + 8192, 4096);
            overruns = rec (s).hooks.overruns;
        }
        const bool fired = overruns > 0;
        row (t.bad == 0 && fired, "LN4 cap F_max (fs) at 44.1 / 96 / 192 k (rec_length 30 s)",
             "meas" + d + " | thr 1323000 / 2880000 / 2880000 | margin 0 | " + ctrl (fired, "LN4n capOff -> " + i64 (overruns) + " overrun at 192 k"));
    }

    //==========================================================================
    // MN — Next Note in Mono / Legato
    void rowMN1()
    {
        const juce::int64 A = 4807;
        auto nn = [&] (int mode, bool anyNote) -> juce::int64
        {
            Session s (kFs, recBase (0, 2, { { "voice_mode", (float) mode }, { "rec_length", 0.6f } }), kSeed, 512);
            rec (s).hooks.nnAnyNote = anyNote;
            inject (s, ScSig { 0.5f, 51 });
            s.noteOn (100, 60);           // held before the arm
            armAt (s, A);
            s.noteOn (A + 1000, 64);      // Mono: retrigger (mints); Legato: pitch move (same serial)
            s.noteOff (A + 2000, 64);     // D30 fallback to 60 (no mint)
            s.noteOff (A + 3000, 60);
            s.noteOn (A + 4000, 67);      // a fresh note (mints in every mode)
            s.run (A + 4000 + 40000, 512);
            return pr (s).start;
        };
        const juce::int64 mono = nn (1, false), legato = nn (2, false), legAny = nn (2, true);
        const bool fired = legAny == A + 1000;
        row (mono == A + 1000 && legato == A + 4000 && fired, "MN1 Next Note: Mono retrigger counts; Legato move + D30 fallback do not",
             "meas Mono start +" + i64 ((std::uint64_t) (mono - A)) + ", Legato start +" + i64 ((std::uint64_t) (legato - A))
                 + " | thr +1000 / +4000 | " + ctrl (fired, "MN1n nnAnyNote -> Legato start +" + i64 ((std::uint64_t) (legAny - A))));
    }

    //==========================================================================
    // QD — presence / availability (QUAL-02)
    void rowsQD()
    {
        const juce::int64 A = 4807;
        {
            Tally t;
            juce::String d;
            for (int trig = 0; trig < 3; ++trig)
            {
                Session s (kFs, recBase (0, trig), kSeed, 512);
                s.load (0, noiseBuf (2, 48000, 0.2f, 6), kFs);
                s.run (512, 512);
                const auto* before = s.eng().getCurrentTape (0);
                std::uint64_t h0 = 0, h1 = 0;
                s.p->getLoader().getPristineHash (0, h0);
                inject (s, ScSig { 0.0f, 0 });
                armAt (s, A);
                if (trig == 2)
                    s.noteOn (A + 500, 60);
                s.run (A + 60000, 512);
                s.p->commitSync();
                s.run (512, 512);
                s.p->getLoader().getPristineHash (0, h1);
                const auto p = pr (s);
                const bool ok = s.eng().getCurrentTape (0) == before && h0 == h1
                             && (trig == 1 ? p.state == RS::Armed : (p.state == RS::Idle && p.reason == RR::NoSignal));
                t.check (ok, juce::String ("trig ") + juce::String (trig) + " " + stName (p.state) + "/" + rsName (p.reason));
                d << " " << trig << ":" << stName (p.state) << "/" << rsName (p.reason);
            }
            const auto rn = takeOnce (0, 0, ScSig { 0.0f, 0, 0, std::numeric_limits<juce::int64>::max(), 0.0, 0.0f,
                                                    juce::Decibels::decibelsToGain (-89.0f) }, 0.0f, A, -1);
            const bool fired = rn.committed;
            row (t.bad == 0 && fired, "QD1 QUAL-02 silent SC: discard NoSignal (Threshold stays Armed), slot bitwise unchanged",
                 "meas" + d + " | thr tape + hash unchanged | " + ctrl (fired, "QD1n -89 dBFS take commits"));
        }
        {
            Session s (kFs, recBase (0, 0), kSeed, 512);
            setLayout (s, CS::disabled());
            inject (s, ScSig { 0.5f, 61 });
            armAt (s, A);
            s.run (A + 30000, 512);
            const auto p1 = pr (s);
            setLayout (s, CS::stereo());   // the bus comes back with rec_arm still On
            s.run (60000, 512);
            const auto p2 = pr (s);
            row (p1.state == RS::Idle && p1.reason == RR::SidechainUnavailable && p2.state == RS::Idle && rec (s).hooks.recArms == 0,
                 "QD2 QUAL-02 no bus: arm inert (SidechainUnavailable); enabling later does nothing",
                 juce::String ("meas disabled ") + stName (p1.state) + "/" + rsName (p1.reason) + ", re-enabled " + stName (p2.state) + ", arms "
                     + i64 (rec (s).hooks.recArms) + " | thr Idle / SidechainUnavailable / Idle / 0 | ctrl -");
        }
        {
            auto lvl = [&] (float db)
            {
                ScSig g { 0.0f, 0 };
                g.sineHz = 997.0;
                g.sineAmp = juce::Decibels::decibelsToGain (db);
                return takeOnce (0, 0, g, 0.0f, A, -1);
            };
            const auto a = lvl (-89.0f), b = lvl (-91.0f);
            row (a.committed && ! b.committed && b.probe.reason == RR::NoSignal, "QD3 D6 presence: -89 dBFS commits, -91 dBFS discards",
                 juce::String ("meas -89: ") + (a.committed ? "committed" : "discarded") + " (peak " + f2 (juce::Decibels::gainToDecibels (a.probe.peak), 2)
                     + " dB), -91: " + rsName (b.probe.reason) + " | thr -90 dBFS | ctrl -");
        }
    }

    //==========================================================================
    // LK — the sidechain never reaches the output (QUAL-02)
    struct LkCase { const char* name; CS in; int garbageCh; };

    std::vector<float> lkRender (const LkCase& lay, int state, bool loud, bool notes, bool noClear)
    {
        // state 0 Idle, 1 Armed (Next Note, no note after the arm), 2 Recording (Manual 30 s),
        // 3 Committing (Manual 0.5 s, never pumped).
        const int trig = state == 1 ? 2 : 0;
        const float len = state == 3 ? 0.5f : 30.0f;
        Session s (kFs, recBase (0, trig, { { "rec_length", len }, { sn (1, "density"), 60.0f }, { sn (1, "size"), 40.0f } }), kSeed, 512);
        if (lay.in != CS::stereo())
            setLayout (s, lay.in);
        s.hk().scNoClear = noClear;
        s.load (0, noiseBuf (2, 96000, 0.3f, 7), kFs);
        ScSig g { loud ? 0.7f : 0.0f, 71 };
        if (loud)
        {
            g.sineHz = 997.0;
            g.sineAmp = 1.0f;
        }
        inject (s, g, lay.garbageCh);
        if (notes)
            for (int n : { 48, 55, 60 })
                s.noteOn (64, n);
        if (state > 0)
            armAt (s, 4807);
        s.run (60000, 441);
        std::vector<float> out (s.L);
        out.insert (out.end(), s.R.begin(), s.R.end());
        return out;
    }

    bool allPosZero (const std::vector<float>& x)
    {
        for (float v : x)
        {
            std::uint32_t b = 0;
            std::memcpy (&b, &v, 4);
            if (b != 0)
                return false;
        }
        return true;
    }

    void rowsLK()
    {
        const LkCase lays[3] = { { "stereo", CS::stereo(), -1 }, { "mono + garbage ch 1", CS::mono(), 1 }, { "disabled", CS::disabled(), -1 } };
        Tally t;
        juce::String d;
        int worst = 0;
        for (const auto& lay : lays)
            for (int st = 0; st < 4; ++st)
            {
                const int n = diffCount (lkRender (lay, st, false, true, false), lkRender (lay, st, true, true, false));
                const bool z = allPosZero (lkRender (lay, st, true, false, false));
                worst = std::max (worst, n);
                t.check (n == 0 && z, juce::String (lay.name) + " state " + juce::String (st) + ": " + juce::String (n) + " differ, +0 " + (z ? "yes" : "no"));
            }
        d << " 12 cases, worst " << worst;
        const int nN = diffCount (lkRender (lays[0], 2, false, true, true), lkRender (lays[0], 2, true, true, true));
        const bool fired = nN > 0;
        row (t.bad == 0 && fired, "LK1 QUAL-02 loud SC x 3 layouts x Idle / Armed / Recording / Committing: output == SC-silent bitwise",
             "meas " + t.detail (12) + " |" + d + " | thr 0 diffs, no notes == +0.0f | " + ctrl (fired, "LK1n scNoClear -> " + juce::String (nN) + " samples leak"));

        // LK2: processBlockBypassed.
        auto bypassed = [] (const LkCase& lay, bool baseHook)
        {
            auto p = make();
            if (lay.in != CS::stereo())
                p->setBusesLayout (layout (lay.in, CS::stereo()));
            p->getEngine().hooks.baseBypass = baseHook;
            p->prepareToPlay (kFs, 512);
            juce::AudioBuffer<float> b (2, 512);
            juce::MidiBuffer mb;
            ScSig g { 0.7f, 72 };
            g.sineHz = 997.0;
            g.sineAmp = 1.0f;
            bool zero = true;
            for (int k = 0; k < 20; ++k)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                        b.setSample (ch, i, ch == lay.garbageCh ? 12345.0f : g.at (k * 512 + i, ch));
                p->processBlockBypassed (b, mb);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                    {
                        const float v = b.getSample (ch, i);
                        std::uint32_t bits = 0;
                        std::memcpy (&bits, &v, 4);
                        zero = zero && bits == 0;
                    }
            }
            return zero;
        };
        Tally t2;
        for (const auto& lay : lays)
            t2.check (bypassed (lay, false), lay.name);
        const bool leaks = ! bypassed (lays[0], true);
        row (t2.bad == 0 && leaks, "LK2 QUAL-02 processBlockBypassed with a loud SC, 3 layouts -> +0.0f (RS1 fix)",
             "meas " + t2.detail (3) + " | thr bitwise +0.0f | " + ctrl (leaks, "LK2n baseBypass (JUCE base, the pre-2.6 behaviour) -> the SC leaks"));
    }

    //==========================================================================
    // CM — the commit paths
    void rowsCM()
    {
        const juce::int64 A = 4807;
        const auto recDir = harnessRecordingsDir();

        // CM1: commitSync, and the real Timer + pump.
        {
            bool ok[2] = { false, false };
            juce::String src[2];
            bool wav[2] = { false, false }, nonAscii[2] = { false, false };
            for (int path = 0; path < 2; ++path)
            {
                Session s (kFs, recBase (2, 0), kSeed, 512);
                ScSig g { 0.5f, 81 + path };
                inject (s, g);
                armAt (s, A);
                s.run (A + 48000 + 2000, 512);
                const auto p = pr (s);
                if (path == 0)
                    s.p->commitSync();
                else
                {
                    for (int k = 0; k < 200 && rec (s).commitPending(); ++k)
                        eg21::pump (5);
                    s.p->getLoader().waitForIdle (5000);
                }
                s.run (512, 64);
                const auto info = s.p->getSlotInfo (2);
                src[path] = info.source;
                const juce::File f (info.source.fromFirstOccurrenceOf ("rec:", false, false));
                wav[path] = f.existsAsFile() && f.isAChildOf (recDir);
                nonAscii[path] = info.source.contains (juce::String::fromUTF8 ("R\xc3\xa9" "cordings \xe2\x9c\x93"));
                ok[path] = tapeDiff (s, 2, g, 1.0f, p.start, p.frames, 2) == 0 && info.source.startsWith ("rec:");
            }
            row (ok[0] && ok[1] && wav[0] && wav[1] && nonAscii[0] && nonAscii[1],
                 "CM1 commit via commitSync and via the Timer + pump: tape, WAV, source rec:<path> (D66: Arm Off after the pump)",
                 juce::String ("meas sync ") + (ok[0] ? "tape ok" : "tape BAD") + ", timer " + (ok[1] ? "tape ok" : "tape BAD") + ", WAVs in the override dir "
                     + (wav[0] && wav[1] ? "yes" : "no") + ", non-ASCII path round-trips " + (nonAscii[0] && nonAscii[1] ? "yes" : "no") + " | e.g. "
                     + src[0].fromLastOccurrenceOf ("/", false, false) + " | ctrl -");
        }
        // CM2: offline (non-realtime) + pump.
        {
            Session s (kFs, recBase (3, 0), kSeed, 512);
            s.p->setNonRealtime (true);
            ScSig g { 0.5f, 83 };
            inject (s, g);
            armAt (s, A);
            s.run (A + 48000 + 2000, 512);
            const auto p = pr (s);
            for (int k = 0; k < 200 && rec (s).commitPending(); ++k)
                eg21::pump (5);
            s.p->getLoader().waitForIdle (5000);
            s.run (512, 64);
            const int dd = tapeDiff (s, 3, g, 1.0f, p.start, p.frames, 2);
            row (dd == 0, "CM2 setNonRealtime (true) + pump: the commit lands", "meas tape diffs " + juce::String (dd) + " | thr 0 | ctrl -");
        }
        // CM3: host idle mid-commit and mid-swap; 50 relinks + 50 restores pumped.
        auto idle = [&] (bool noEntry, int& pendMax, int& deferredEnd, int& liveMax, bool& drained, bool& landed)
        {
            Session s (kFs, recBase (0, 0, { { "rec_length", 0.6f } }), kSeed, 512);
            s.p->getHandoff().testNoEntryCounter.store (noEntry);
            ScSig g { 0.5f, 84 };
            inject (s, g);
            armAt (s, A);
            s.run (A + 28800 + 1000, 512);       // Committing; the audio thread now idles
            for (int k = 0; k < 100 && rec (s).commitPending(); ++k)
                eg21::pump (5);
            s.p->getLoader().waitForIdle (5000);   // published: mid-swap (pending)
            const auto wavFile = juce::File (s.p->getSlotInfo (0).source.fromFirstOccurrenceOf ("rec:", false, false));
            juce::MemoryBlock st;
            s.p->getStateInformation (st);
            pendMax = 0;
            liveMax = 0;
            for (int k = 0; k < 50; ++k)
            {
                s.p->requestRelink (0, wavFile);
                s.p->getLoader().waitForIdle (5000);
                eg21::pump (2);
                pendMax = std::max (pendMax, s.p->getHandoff().pendingDepth (0));
                liveMax = std::max (liveMax, ogi::SlotTape::liveCount.load());
                s.p->setStateInformation (st.getData(), (int) st.getSize());
                s.p->getLoader().waitForIdle (5000);
                eg21::pump (2);
                pendMax = std::max (pendMax, s.p->getHandoff().pendingDepth (0));
                liveMax = std::max (liveMax, ogi::SlotTape::liveCount.load());
            }
            deferredEnd = s.p->getHandoff().deferredCount();
            // Resume: the take lands; the retire ring drains.
            s.run (48000, 512);
            eg21::pump (250);
            landed = s.eng().getCurrentTape (0) != nullptr;
            drained = s.eng().getRetiringCount() == 0;
        };
        int pm = 0, de = 0, lm = 0, pmN = 0, deN = 0, lmN = 0;
        bool dr = false, la = false, drN = false, laN = false;
        idle (false, pm, de, lm, dr, la);
        idle (true, pmN, deN, lmN, drN, laN);
        const bool fired = deN > de + 20;
        row (pm <= 1 && de == 0 && lm <= 8 && dr && la && fired, "CM3 PERF-03 host idle mid-commit / mid-swap, 50 relinks + 50 restores",
             "meas pending max " + juce::String (pm) + ", deferred after " + juce::String (de) + ", live tapes max " + juce::String (lm)
                 + ", resume: landed " + (la ? "yes" : "no") + ", retiring drained " + (dr ? "yes" : "no") + " | thr <= 1, 0, bounded | "
                 + ctrl (fired, "CM3n testNoEntryCounter -> deferred " + juce::String (deN)));

        // CM4: prepare mid-Recording (44.1 → 96 k); Armed → Interrupted; Committing survives.
        {
            Session s (44100.0, recBase (1, 0, { { "rec_length", 10.0f } }), kSeed, 512);
            ScSig g { 0.5f, 85 };
            inject (s, g);
            armAt (s, A);
            s.run (A + 44100, 512);   // 1 s of a 10 s take
            const auto before = pr (s);
            s.p->prepareToPlay (96000.0, 512);
            const auto after = pr (s);
            const bool c = land (s);
            const auto pm4 = s.p->getLoader().getPristine (1);
            const auto* tp = s.eng().getCurrentTape (1);
            const bool rec44 = pm4 != nullptr && pm4->rate == 44100.0 && pm4->pcm.getNumSamples() == before.frames;
            const bool tape96 = tp != nullptr && tp->rate == 96000.0;

            Session s2 (kFs, recBase (1, 1), kSeed, 512);
            inject (s2, ScSig { 0.0f, 0 });
            armAt (s2, A);
            s2.run (A + 2000, 512);
            s2.p->prepareToPlay (kFs, 512);
            const auto pi = pr (s2);

            Session s3 (kFs, recBase (1, 0, { { "rec_length", 0.6f } }), kSeed, 512);
            inject (s3, ScSig { 0.5f, 86 });
            armAt (s3, A);
            s3.run (A + 30000, 512);
            s3.p->prepareToPlay (kFs, 512);
            const bool survives = pr (s3).state == RS::Committing && land (s3) && s3.eng().getCurrentTape (1) != nullptr;

            row (before.state == RS::Recording && after.state == RS::Committing && c && rec44 && tape96
                     && pi.state == RS::Idle && pi.reason == RR::Interrupted && survives,
                 "CM4 prepare mid-Recording commits at the old rate; Armed -> Interrupted; Committing survives",
                 juce::String ("meas ") + stName (before.state) + " -> " + stName (after.state) + ", pristine " + (rec44 ? "44.1 k" : "BAD") + " ("
                     + juce::String (before.frames) + " frames), tape " + (tape96 ? "96 k" : "BAD") + "; Armed -> " + rsName (pi.reason) + "; Committing "
                     + (survives ? "survives" : "LOST") + " | ctrl -");
        }
        // CM5: bypassed blocks while Armed / Recording.
        {
            auto bypassAt = [&] (int trig, juce::int64 after, RR& reason, std::uint64_t& commits)
            {
                Session s (kFs, recBase (0, trig, { { "rec_length", 10.0f } }), kSeed, 512);
                inject (s, ScSig { trig == 1 ? 0.0f : 0.5f, 87 });
                armAt (s, A);
                s.run (A + after, 512);
                juce::AudioBuffer<float> b (2, 512);
                juce::MidiBuffer mb;
                b.clear();
                s.p->processBlockBypassed (b, mb);
                reason = pr (s).reason;
                commits = rec (s).hooks.recCommits;
            };
            RR ra = RR::None, rr = RR::None;
            std::uint64_t ca = 0, cr = 0;
            bypassAt (1, 4000, ra, ca);
            bypassAt (0, 30000, rr, cr);
            row (ra == RR::Bypassed && ca == 0 && rr == RR::Committed && cr == 1, "CM5 PD10 bypassed blocks: Armed -> Bypassed; Recording >= 0.5 s commits",
                 juce::String ("meas Armed -> ") + rsName (ra) + ", Recording 0.625 s -> " + rsName (rr) + " | thr Bypassed / Committed | ctrl -");
        }
        // CM6: the WAV write fails.
        {
            Session s (kFs, recBase (0, 0), kSeed, 512);
            s.p->getLoader().hooks.failWav.store (true);
            ScSig g { 0.5f, 88 };
            inject (s, g);
            armAt (s, A);
            s.run (A + 48000 + 2000, 512);
            const auto p = pr (s);
            const bool c = land (s);
            const auto info = s.p->getSlotInfo (0);
            const int dd = tapeDiff (s, 0, g, 1.0f, p.start, p.frames, 2);
            const bool fileMissing = ! juce::File (info.source.fromFirstOccurrenceOf ("rec:", false, false)).existsAsFile();
            const auto reason = pr (s).reason;
            row (c && dd == 0 && info.wavFailed && info.hasBlob && fileMissing && info.source.startsWith ("rec:") && reason == RR::WavFailed,
                 "CM6 WAV write fails: still publishes, flagged, the blob carries the take",
                 juce::String ("meas tape diffs ") + juce::String (dd) + ", wavFailed " + (info.wavFailed ? "yes" : "no") + ", blob "
                     + (info.hasBlob ? "yes" : "no") + ", no file " + (fileMissing ? "yes" : "no") + ", reason " + rsName (reason) + " | ctrl -");
        }
    }

    //==========================================================================
    void runAll()
    {
        section ("2.6a  Record-in capture / triggers / one-shot / length / presence / leaks / commit (PLAN-2.6 T12)");
        if (want ("RC"))  { rowsRC(); rowRC4(); }
        if (want ("TH"))  rowsTH();
        if (want ("OS"))  rowsOS();
        if (want ("LN"))  rowsLN();
        if (want ("MN"))  rowMN1();
        if (want ("QD"))  rowsQD();
        if (want ("LK"))  rowsLK();
        if (want ("CM"))  rowsCM();
    }
} }
