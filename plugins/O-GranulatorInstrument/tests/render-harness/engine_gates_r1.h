/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 4 / R1 rows (PLAN-R1 PD30 /
    PD31, task T13). Included by main.cpp AFTER preset_gates_4.h; run after
    pg4::runAll() and before rowHM1(). Flag: --egr1 (OGI_ROWS filters).

    Record-in Arm auto-clear (CONTEXT-R1 D66, PD18 / PD19). The write-back runs
    from the processor's 20 ms message-thread Timer, which fires in the harness
    ONLY while the message loop is pumped (eg21::pump): commitSync / land () do
    not pump, so every row here commits through the Timer.

      AC1   Manual take with signal, run to commit, pump 100 ms; an
            AudioProcessorListener records what a host sees: Arm 0, exactly one
            gesture begin / change (0) / gesture end, tape committed
      AC1n  testNoArmAutoClear -> Arm stays 1 (the predicate must FAIL)
      AC2   five discards (silent take; bypass while Armed; prepareToPlay while
            Armed; arm with the sidechain bus disabled; Arm On with audio
            stopped, then prepareToPlay): Arm 0 after the pump, slot unchanged
      AC2n  skipEndGen on each arm's own branch -> that arm stays 1 (must FAIL)
      AC3   Threshold below threshold / Next Note with no notes, 2 s of blocks
            with pumps between: Arm 1, Armed, zero host notifications; then the
            trigger fires and AC1 holds
      AC3n  testClearWheneverNotRecording -> cleared while waiting (must FAIL)
      AC4   after AC1, ONE setValueNotifyingHost (1): a second take lands
            (recCommits 2)                                  ctrl - (AC1n)
      AC5   an On edge during Committing, before the pump: held (heldArms 1),
            then armed (recArms 2); Arm still 1 while it records
      AC5n  testNoGenGuard -> the held arm is cancelled (must FAIL)
      AC6   AC1's write arrives on the message thread at APVTS-listener depth 0
      AC6n  testClearFromListener + testRunArmAutoClear() from inside the
            harness's parameterChanged -> depth 1 (must FAIL)
      AC7   (a) a blob with rec_arm 1 restored, prepareToPlay, pump: Arm 0,
            recArms unchanged; (b) save, restore, save: byte-identical
            + INFO: a restore while blocks are running (today's edge, OS2)
      AC7n  skipEndGen = 7 (StaleOn) -> Arm stays 1 (must FAIL)
      AC8   a factory preset applied while waiting Armed (Threshold): Arm 1,
            Armed, the latched trigger / slot unchanged     ctrl - (UPM3n)

    "APVTS-listener depth": the number of this header's own
    AudioProcessorValueTreeState::Listener::parameterChanged frames on the
    stack when the host-visible change of rec_arm to 0 is delivered (the Spy
    listens on rec_arm and rec_gain and counts its own entries / exits).

    A negative-control row runs the SAME predicate as its parent on the broken
    input and PASSES only when that predicate FAILS ("ctrl FIRED").

  ==============================================================================
*/
#pragma once

namespace { namespace egr1
{
    using eg21::Session;
    using eg21::Proc;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::kSeed;
    using eg21::setP;
    using eg21::setAll;
    using eg21::rawOf;
    using eg21::pump;
    using eg24b::want;
    using eg24b::section;
    using eg26a::ScSig;
    using eg26a::inject;
    using eg26a::rec;
    using eg26a::pr;
    using eg26a::recBase;
    using eg26a::armAt;
    using eg26a::setLayout;
    using eg26a::tapeDiff;
    using eg26a::crossing;
    using eg26a::stName;
    using eg26a::rsName;
    using RS = ogi::RecState;
    using RR = ogi::RecReason;
    using RT = ogi::RecTrigger;
    using CS = juce::AudioChannelSet;

    constexpr double      kFs = 48000.0;
    constexpr juce::int64 kA  = 4807;   // the arm sample, off the 32-grid

    //==========================================================================
    /** What a host sees of rec_arm, in order: "C1" / "C0" a value change,
        "B" / "E" a gesture begin / end. Also this header's APVTS listener (see
        "depth" above). */
    struct Spy final : juce::AudioProcessorListener,
                       juce::AudioProcessorValueTreeState::Listener
    {
        explicit Spy (Proc& proc) : p (proc), idx (param (proc, "rec_arm")->getParameterIndex())
        {
            p.addListener (this);
            p.parameters.addParameterListener ("rec_arm", this);
            p.parameters.addParameterListener ("rec_gain", this);
        }
        ~Spy() override
        {
            p.parameters.removeParameterListener ("rec_gain", this);
            p.parameters.removeParameterListener ("rec_arm", this);
            p.removeListener (this);
        }

        void audioProcessorParameterChanged (juce::AudioProcessor*, int i, float v) override
        {
            if (i != idx)
                return;
            add (v > 0.5f ? "C1" : "C0");
            if (v > 0.5f)
                ++ones;
            else
            {
                ++zeros;
                offDepth = depth;
                offOnMsg = juce::MessageManager::getInstance()->isThisTheMessageThread();
            }
        }
        void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override {}
        void audioProcessorParameterChangeGestureBegin (juce::AudioProcessor*, int i) override { if (i == idx) { add ("B"); ++begins; } }
        void audioProcessorParameterChangeGestureEnd (juce::AudioProcessor*, int i) override   { if (i == idx) { add ("E"); ++ends; } }

        void parameterChanged (const juce::String& id, float v) override
        {
            const int at = depth;
            ++depth;
            if (id == "rec_arm" && ! (v > 0.5f))
                apvtsOffDepth = at;
            if (id == "rec_gain" && fireFromListener)
                p.testRunArmAutoClear();   // AC6n: the write issued from a parameterChanged
            --depth;
        }

        void add (const char* t) { if (seq.isNotEmpty()) seq << " "; seq << t; }

        Proc& p;
        const int idx;
        juce::String seq;
        int begins = 0, ends = 0, zeros = 0, ones = 0;
        int depth = 0;                 // this listener's parameterChanged frames on the stack
        int offDepth = -1;             // `depth` when the host-visible change to 0 arrived
        int apvtsOffDepth = -1;        // `depth` at the entry of parameterChanged (rec_arm, 0)
        bool offOnMsg = false;
        bool fireFromListener = false;
    };

    float armOf (Session& s) { return rawOf (*s.p, "rec_arm"); }

    /** The Timer takes the commit (pollRecorder) and runs the auto-clear in the
        same callback; then 100 ms more of the loop (PD31) and a few blocks so
        the audio thread sees the tape and the Off. */
    bool pumpTimer (Session& s, bool blocksAfter = true)
    {
        for (int k = 0; k < 200 && rec (s).commitPending(); ++k)
            pump (5);
        const bool taken = ! rec (s).commitPending();
        s.p->getLoader().waitForIdle (5000);
        pump (100);
        if (blocksAfter)
            s.run (512, 64);
        return taken;
    }

    std::uint64_t hashOf (Session& s, int slot)
    {
        std::uint64_t h = 0;
        s.p->getLoader().getPristineHash (slot, h);
        return h;
    }

    juce::String a0 (float v) { return f2 (v, 0); }

    //==========================================================================
    // AC1 / AC1n / AC4 / AC6 / AC6n — one Manual take through the Timer
    struct TakeR
    {
        bool  stim = false;                 // Committing with Arm 1 before the pump; the Timer took the commit
        float armBefore = 0.0f, armAfter = 0.0f;
        int   diff = -9;
        std::uint64_t commits = 0, clears = 0;
        juce::String seq;
        int   begins = 0, ends = 0, zeros = 0;
        int   offDepth = -1, apvtsOffDepth = -1;
        bool  offOnMsg = false;
        // AC6n (mode 2): after the pump, before the listener write
        float armAfterPump = 0.0f;
        std::uint64_t clearsAfterPump = 0;
        // AC4 (mode 0), or the same single write with the clear off (mode 1)
        bool  stim2 = false;
        float arm2Before = 0.0f, arm2After = 0.0f;
        int   diff2 = -9;
        std::uint64_t commits2 = 0, clears2 = 0, arms2 = 0;
        juce::int64 start2 = -1, edge2 = -1;
        juce::String seq2;
        bool  newTape = false;
    };

    /** mode 0: as shipped (+ the AC4 second take). 1: testNoArmAutoClear.
        2: testClearFromListener, the write issued from Spy::parameterChanged. */
    TakeR runTake (int mode)
    {
        TakeR r;
        Session s (kFs, recBase (2, 0), kSeed, 512);
        s.record = false;
        s.p->testNoArmAutoClear    = mode == 1;
        s.p->testClearFromListener = mode == 2;
        const ScSig g { 0.5f, 901 };
        inject (s, g);
        Spy spy (*s.p);
        armAt (s, kA);
        s.run (kA + 48000 + 2000, 512);
        const auto p1 = pr (s);
        r.armBefore = armOf (s);
        const bool taken = pumpTimer (s);
        r.stim = p1.state == RS::Committing && juce::exactlyEqual (r.armBefore, 1.0f) && taken && p1.start == kA;

        if (mode == 2)
        {
            r.armAfterPump = armOf (s);
            r.clearsAfterPump = s.p->recAutoClears;
            spy.fireFromListener = true;
            setP (*s.p, "rec_gain", 1.0f);   // any host edit: the Spy's parameterChanged runs the check on its own stack
            spy.fireFromListener = false;
            s.run (512, 64);
        }

        r.armAfter = armOf (s);
        r.diff     = tapeDiff (s, 2, g, 1.0f, p1.start, p1.frames, 2);
        r.commits  = rec (s).hooks.recCommits;
        r.clears   = s.p->recAutoClears;
        r.seq      = spy.seq;
        r.begins = spy.begins; r.ends = spy.ends; r.zeros = spy.zeros;
        r.offDepth = spy.offDepth; r.apvtsOffDepth = spy.apvtsOffDepth; r.offOnMsg = spy.offOnMsg;

        if (mode != 2)
        {
            // AC4: ONE host write of 1, nothing else.
            const auto* first = s.eng().getCurrentTape (2);
            r.arm2Before = armOf (s);
            r.edge2 = s.pos;
            param (*s.p, "rec_arm")->setValueNotifyingHost (1.0f);
            s.run (48000 + 2000, 512);
            const auto p2 = pr (s);
            const bool taken2 = pumpTimer (s);
            r.start2   = p2.start;
            r.commits2 = rec (s).hooks.recCommits;
            r.arms2    = rec (s).hooks.recArms;
            r.clears2  = s.p->recAutoClears;
            r.arm2After = armOf (s);
            r.seq2     = spy.seq;
            r.newTape  = s.eng().getCurrentTape (2) != first;
            r.diff2    = r.commits2 == 2 ? tapeDiff (s, 2, g, 1.0f, p2.start, p2.frames, 2) : -3;
            r.stim2    = taken2;
        }
        return r;
    }

    bool ac1Predicate (const TakeR& r)
    {
        return juce::exactlyEqual (r.armAfter, 0.0f) && r.seq == "C1 B C0 E" && r.begins == 1 && r.zeros == 1 && r.ends == 1
            && r.diff == 0 && r.commits == 1;
    }
    bool ac4Predicate (const TakeR& r) { return r.commits2 == 2 && r.diff2 == 0 && r.newTape && r.start2 == r.edge2; }
    bool ac6Predicate (const TakeR& r) { return r.zeros == 1 && r.offOnMsg && r.offDepth == 0 && r.apvtsOffDepth == 0; }

    void rowsTake()
    {
        const auto r = runTake (0);
        const auto n = runTake (1);
        const auto l = runTake (2);

        if (want ("AC1"))
        {
            const bool fired = n.stim && ! ac1Predicate (n) && juce::exactlyEqual (n.armAfter, 1.0f) && n.clears == 0 && n.diff == 0 && n.commits == 1;
            row (r.stim && ac1Predicate (r) && r.clears == 1,
                 "AC1 D66 Manual take, Timer commit, pump 100 ms: Arm 0, one gesture begin / change (0) / end, tape committed",
                 "meas rec_arm " + a0 (r.armBefore) + " -> " + a0 (r.armAfter) + ", host sees [" + r.seq + "] (begins " + juce::String (r.begins)
                     + ", zeros " + juce::String (r.zeros) + ", ends " + juce::String (r.ends) + "), tape diffs " + juce::String (r.diff) + ", commits "
                     + i64 (r.commits) + ", auto-clears " + i64 (r.clears) + " | thr 0, [C1 B C0 E], 0 diffs, 1, 1 | margin 0 | ctrl AC1n");
            row (fired, "AC1n control: testNoArmAutoClear -> Arm stays 1 (predicate must FAIL)",
                 "meas rec_arm " + a0 (n.armBefore) + " -> " + a0 (n.armAfter) + ", host sees [" + n.seq + "], tape diffs " + juce::String (n.diff)
                     + ", commits " + i64 (n.commits) + ", auto-clears " + i64 (n.clears) + " | thr predicate 'Arm 0, [C1 B C0 E], tape committed' FAILS on Arm | "
                     + ctrl (fired, "AC1n the write skipped -> rec_arm " + a0 (n.armAfter) + " after the same take and pump (take committed, "
                                        + juce::String (n.diff) + " diffs)"));
        }
        if (want ("AC4"))
        {
            // AC1n is AC4's control: with the clear off the same single write is not an edge.
            const bool fired = n.stim && juce::exactlyEqual (n.arm2Before, 1.0f) && ! ac4Predicate (n) && n.commits2 == 1 && n.arms2 == 1;
            row (r.stim && ac1Predicate (r) && juce::exactlyEqual (r.arm2Before, 0.0f) && r.stim2 && ac4Predicate (r) && r.arms2 == 2
                     && juce::exactlyEqual (r.arm2After, 0.0f) && r.clears2 == 2 && r.seq2 == "C1 B C0 E C1 B C0 E" && fired,
                 "AC4 D66 after AC1, one setValueNotifyingHost (1): a second take lands",
                 "meas rec_arm " + a0 (r.arm2Before) + " before the write, commits " + i64 (r.commits2) + ", arms " + i64 (r.arms2) + ", second start "
                     + juce::String (r.start2) + " (write at " + juce::String (r.edge2) + "), tape diffs " + juce::String (r.diff2) + ", new tape "
                     + (r.newTape ? "yes" : "no") + ", rec_arm after " + a0 (r.arm2After) + ", auto-clears " + i64 (r.clears2) + ", host sees [" + r.seq2
                     + "] | thr 2 commits, 0 diffs | margin 0 | "
                     + ctrl (fired, "AC1n (clear off): rec_arm " + a0 (n.arm2Before) + " before the same write -> no edge, commits " + i64 (n.commits2)
                                        + ", arms " + i64 (n.arms2)));
        }
        if (want ("AC6"))
        {
            const bool stimL = l.stim && juce::exactlyEqual (l.armAfterPump, 1.0f) && l.clearsAfterPump == 0;   // the Timer skipped the check
            const bool fired = stimL && ! ac6Predicate (l) && l.zeros == 1 && l.offDepth == 1 && l.apvtsOffDepth == 1
                            && juce::exactlyEqual (l.armAfter, 0.0f) && l.clears == 1;
            row (r.stim && ac1Predicate (r) && ac6Predicate (r),
                 "AC6 D66 the auto-clear write arrives on the message thread at APVTS-listener depth 0",
                 juce::String ("meas change to 0: message thread ") + (r.offOnMsg ? "yes" : "NO") + ", depth " + juce::String (r.offDepth)
                     + " at the host callback, " + juce::String (r.apvtsOffDepth) + " at parameterChanged (rec_arm) entry, " + juce::String (r.zeros)
                     + " change(s) | thr message thread, depth 0 | margin 0 | ctrl AC6n");
            row (fired, "AC6n control: the write issued from parameterChanged -> depth 1 (predicate must FAIL)",
                 "meas Timer check skipped: rec_arm " + a0 (l.armAfterPump) + ", auto-clears " + i64 (l.clearsAfterPump) + " after the pump; then "
                     + "testRunArmAutoClear() inside parameterChanged (rec_gain): rec_arm " + a0 (l.armAfter) + ", host sees [" + l.seq + "], depth "
                     + juce::String (l.offDepth) + " / " + juce::String (l.apvtsOffDepth) + ", message thread " + (l.offOnMsg ? "yes" : "no")
                     + " | thr predicate 'message thread, depth 0' FAILS on depth | "
                     + ctrl (fired, "AC6n testClearFromListener -> the change to 0 arrives at depth " + juce::String (l.offDepth)));
        }
    }

    //==========================================================================
    // AC2 / AC2n — the five discards
    struct DiscR
    {
        bool  stim = false;      // the ending was reached with Arm still 1
        float armBefore = 0.0f, armAfter = 0.0f;
        bool  slotSame = false;
        RS    state = RS::Idle;
        RR    reason = RR::None;
        std::uint64_t ends = 0, stale = 0, arms = 0, starts = 0, commits = 0, clears = 0;
        juce::String seq;
    };

    const char* const kDiscName[5] = { "silent take", "bypass while Armed", "prepare while Armed", "no sidechain bus", "stale On + prepare" };
    const int         kDiscEnd[5]  = { 2, 5, 4, 6, 7 };   // the RecEnding of each arm (NoSignal, Bypassed, Interrupted, SidechainUnavailable, StaleOn)

    /** arm 0…4 = PD31's five discards, in its order. skip = RecorderTestHooks::skipEndGen. */
    DiscR runDiscard (int arm, int skip)
    {
        DiscR r;
        const int trig = (arm == 1 || arm == 2) ? 1 : 0;   // waiting Armed needs Threshold
        Session s (kFs, recBase (1, trig), kSeed, 512);
        s.record = false;
        if (arm == 3)
            setLayout (s, CS::disabled());   // prepares with Arm Off: no ending here
        s.load (1, noiseBuf (2, 48000, 0.2f, 6), kFs);
        s.run (512, 512);
        const std::uint64_t h0 = hashOf (s, 1);
        rec (s).hooks.resetCounters();
        rec (s).hooks.skipEndGen = skip;
        inject (s, ScSig { (arm == 3 || arm == 4) ? 0.5f : 0.0f, 911 + arm });
        Spy spy (*s.p);

        RR wantReason = RR::None;
        switch (arm)
        {
            case 0:   // a silent Manual take runs its full length, then NoSignal
                armAt (s, s.pos + kA);
                s.run (kA + 60000, 512);
                wantReason = RR::NoSignal;
                break;
            case 1:   // bypassed block while Armed (Threshold, silent)
            {
                armAt (s, s.pos + kA);
                s.run (kA + 4000, 512);
                const bool armed = pr (s).state == RS::Armed;
                juce::AudioBuffer<float> b (2, 512);
                juce::MidiBuffer mb;
                b.clear();
                s.p->processBlockBypassed (b, mb);
                wantReason = armed ? RR::Bypassed : RR::None;
                break;
            }
            case 2:   // prepareToPlay while Armed (Threshold, silent)
            {
                armAt (s, s.pos + kA);
                s.run (kA + 4000, 512);
                const bool armed = pr (s).state == RS::Armed;
                s.p->prepareToPlay (kFs, 512);
                wantReason = armed ? RR::Interrupted : RR::None;
                break;
            }
            case 3:   // the bus is disabled: the arm is inert
                armAt (s, s.pos + kA);
                s.run (kA + 30000, 512);
                wantReason = RR::SidechainUnavailable;
                break;
            default:  // Arm set On with audio stopped, then prepareToPlay (the edge is swallowed)
                s.run (4800, 512);
                setP (*s.p, "rec_arm", 1.0f);
                s.p->prepareToPlay (kFs, 512);
                break;
        }

        const auto p0 = pr (s);
        r.armBefore = armOf (s);
        r.ends  = rec (s).hooks.armEnds;
        r.stale = rec (s).hooks.staleArms;
        r.stim  = juce::exactlyEqual (r.armBefore, 1.0f) && p0.state == RS::Idle && r.ends == (skip == kDiscEnd[arm] ? 0u : 1u)
               && (arm == 4 ? (r.stale == 1 && rec (s).hooks.recArms == 0) : (p0.reason == wantReason && wantReason != RR::None && r.stale == 0));

        // No prepareToPlay from here to the Arm read (a prepare after a skipped
        // ending is itself a StaleOn ending).
        pump (100);
        s.run (arm == 4 ? 48000 : 512, 64);   // arm 4: a second of loud sidechain — nothing may start
        pump (40);

        const auto p1 = pr (s);
        r.armAfter = armOf (s);
        r.state  = p1.state;
        r.reason = p0.reason;
        r.arms    = rec (s).hooks.recArms;
        r.starts  = rec (s).hooks.recStarts;
        r.commits = rec (s).hooks.recCommits;
        r.clears  = s.p->recAutoClears;
        r.seq     = spy.seq;
        r.slotSame = hashOf (s, 1) == h0 && h0 != 0 && r.commits == 0 && ! rec (s).commitPending();
        return r;
    }

    bool ac2Predicate (const DiscR& r) { return juce::exactlyEqual (r.armAfter, 0.0f) && r.slotSame; }

    void rowsDiscard()
    {
        Tally t, tn;
        juce::String d, dn;
        bool firedAll = true;
        for (int arm = 0; arm < 5; ++arm)
        {
            const auto r = runDiscard (arm, 0);
            const bool ok = r.stim && ac2Predicate (r) && r.clears == 1 && r.seq == "C1 B C0 E" && r.state == RS::Idle
                         && (arm == 4 ? (r.arms == 0 && r.starts == 0) : true);
            t.check (ok, juce::String (kDiscName[arm]) + ": stim " + (r.stim ? "ok" : "NOT FIRED") + ", rec_arm " + a0 (r.armBefore) + " -> " + a0 (r.armAfter)
                             + ", [" + r.seq + "], slot " + (r.slotSame ? "same" : "CHANGED"));
            d << " | " << kDiscName[arm] << " (" << (arm == 4 ? juce::String ("staleArms ") + i64 (r.stale) + ", recArms " + i64 (r.arms)
                                                               : juce::String (rsName (r.reason)))
              << "): " << a0 (r.armBefore) << " -> " << a0 (r.armAfter) << " [" << r.seq << "] slot " << (r.slotSame ? "same" : "CHANGED");

            const auto n = runDiscard (arm, kDiscEnd[arm]);
            const bool fired = n.stim && ! ac2Predicate (n) && juce::exactlyEqual (n.armAfter, 1.0f) && n.clears == 0 && n.slotSame && n.seq == "C1";
            firedAll = firedAll && fired;
            tn.check (fired, juce::String (kDiscName[arm]) + ": stim " + (n.stim ? "ok" : "NOT FIRED") + ", rec_arm " + a0 (n.armAfter) + ", clears " + i64 (n.clears));
            dn << " | skipEndGen " << kDiscEnd[arm] << " (" << kDiscName[arm] << "): rec_arm " << a0 (n.armBefore) << " -> " << a0 (n.armAfter)
               << ", endGen writes " << i64 (n.ends) << ", auto-clears " << i64 (n.clears);
        }
        row (t.bad == 0, "AC2 D66 five discards: Arm 0 after the pump in each, slot unchanged",
             "meas " + t.detail (5) + d + " | thr rec_arm 0, [C1 B C0 E], pristine hash same, 0 commits | margin 0 | ctrl AC2n");
        row (firedAll, "AC2n control: the endGen write removed from each arm's branch -> that arm stays 1 (predicate must FAIL)",
             "meas " + tn.detail (5) + dn + " | thr predicate 'Arm 0, slot unchanged' FAILS on Arm in all five | "
                 + ctrl (firedAll, "AC2n skipEndGen 2 / 5 / 4 / 6 / 7 -> rec_arm stays 1 in " + juce::String (5 - tn.bad) + " / 5 arms (no prepareToPlay after the skipped ending)"));
    }

    //==========================================================================
    // AC3 / AC3n — a waiting arm is never cleared
    struct WaitR
    {
        bool  stimWait = false;     // Armed entered, >= 2 s of blocks, the Timer ran
        float armWait = 0.0f;
        RS    stateWait = RS::Idle;
        juce::String seqWait;
        std::uint64_t clearsWait = 0, cancels = 0;
        // after the trigger
        bool  stimTake = false;
        float armAfter = 0.0f;
        int   diff = -9;
        juce::int64 start = -1, wantStart = -1;
        juce::String seq;
        std::uint64_t commits = 0, clears = 0;
    };

    /** trig 1: Threshold, the sidechain silent until T. trig 2: Next Note, a
        loud sidechain and no note until T. T = arm + 100 000 samples. */
    WaitR runWait (int trig, bool clearWhenNotRecording)
    {
        WaitR r;
        Session s (kFs, recBase (2, trig), kSeed, 512);
        s.record = false;
        s.p->testClearWheneverNotRecording = clearWhenNotRecording;
        const juce::int64 T = kA + 100000;
        ScSig g { 0.5f, 921 + trig };
        if (trig == 1)
            g.from = T;
        inject (s, g);
        Spy spy (*s.p);
        armAt (s, kA);
        s.run (kA + 1000, 512);
        const bool armed = pr (s).state == RS::Armed && rec (s).hooks.recArms == 1;
        const juce::int64 w0 = s.pos;
        for (int k = 0; k < 10; ++k)
        {
            s.run (9600, 512);
            pump (30);
        }
        r.armWait    = armOf (s);
        r.stateWait  = pr (s).state;
        r.seqWait    = spy.seq;
        r.clearsWait = s.p->recAutoClears;
        r.cancels    = rec (s).hooks.recCancels;
        r.stimWait   = armed && s.pos - w0 == 96000 && s.pos < T;
        if (clearWhenNotRecording)
            return r;

        if (trig == 2)
            s.noteOn (T, 60);
        s.run (T + 48000 + 3000 - s.pos, 512);
        const auto p = pr (s);
        const float armPre = armOf (s);
        const bool taken = pumpTimer (s);
        r.wantStart = trig == 1 ? crossing (g, kA, 1.0f, -30.0f) - 480 : T;
        r.start    = p.start;
        r.stimTake = p.state == RS::Committing && juce::exactlyEqual (armPre, 1.0f) && taken && p.start == r.wantStart && p.start >= T - 480;
        r.armAfter = armOf (s);
        r.diff     = tapeDiff (s, 2, g, 1.0f, p.start, p.frames, 2);
        r.seq      = spy.seq;
        r.commits  = rec (s).hooks.recCommits;
        r.clears   = s.p->recAutoClears;
        return r;
    }

    bool ac3Predicate (const WaitR& r)
    {
        return juce::exactlyEqual (r.armWait, 1.0f) && r.stateWait == RS::Armed && r.seqWait == "C1";   // "C1" is the harness's own arm: zero notifications after it
    }

    void rowsWait()
    {
        const char* names[3] = { "", "Threshold", "Next Note" };
        Tally t, tn;
        juce::String d, dn;
        bool firedAll = true;
        for (int trig = 1; trig <= 2; ++trig)
        {
            const auto r = runWait (trig, false);
            const bool ok = r.stimWait && ac3Predicate (r) && r.clearsWait == 0
                         && r.stimTake && juce::exactlyEqual (r.armAfter, 0.0f) && r.seq == "C1 B C0 E" && r.diff == 0 && r.commits == 1 && r.clears == 1;
            t.check (ok, juce::String (names[trig]) + ": wait stim " + (r.stimWait ? "ok" : "NOT FIRED") + ", rec_arm " + a0 (r.armWait) + " " + stName (r.stateWait)
                             + " [" + r.seqWait + "]; take stim " + (r.stimTake ? "ok" : "NOT FIRED") + ", rec_arm " + a0 (r.armAfter) + " [" + r.seq + "] diffs "
                             + juce::String (r.diff));
            d << " | " << names[trig] << ": 2 s waiting (10 x 9600 samples + 30 ms pump) rec_arm " << a0 (r.armWait) << ", " << stName (r.stateWait)
              << ", host sees [" << r.seqWait << "], auto-clears " << i64 (r.clearsWait) << "; trigger -> start " << r.start << " (want " << r.wantStart
              << "), rec_arm " << a0 (r.armAfter) << ", [" << r.seq << "], tape diffs " << r.diff;

            const auto n = runWait (trig, true);
            const bool fired = n.stimWait && ! ac3Predicate (n) && juce::exactlyEqual (n.armWait, 0.0f) && n.clearsWait >= 1 && n.stateWait == RS::Idle;
            firedAll = firedAll && fired;
            tn.check (fired, juce::String (names[trig]) + ": stim " + (n.stimWait ? "ok" : "NOT FIRED") + ", rec_arm " + a0 (n.armWait) + " " + stName (n.stateWait));
            dn << " | " << names[trig] << ": rec_arm " << a0 (n.armWait) << ", " << stName (n.stateWait) << ", host sees [" << n.seqWait << "], auto-clears "
               << i64 (n.clearsWait) << ", cancels " << i64 (n.cancels);
        }
        row (t.bad == 0, "AC3 D66 a waiting arm (Threshold below threshold, Next Note with no notes) is not cleared for 2 s; then the trigger fires and AC1 holds",
             "meas " + t.detail (2) + d + " | thr rec_arm 1, Armed, [C1] while waiting; then 0, [C1 B C0 E], 0 diffs | margin 0 | ctrl AC3n");
        row (firedAll, "AC3n control: clear whenever state != Recording -> cleared while waiting (predicate must FAIL)",
             "meas " + tn.detail (2) + dn + " | thr predicate 'Arm 1, Armed, zero notifications' FAILS in both | "
                 + ctrl (firedAll, "AC3n testClearWheneverNotRecording -> the waiting arm is cleared (" + juce::String (2 - tn.bad) + " / 2 triggers)"));
    }

    //==========================================================================
    // AC5 / AC5n — an arm held during Committing is not cancelled by the previous take's clear
    struct HeldR
    {
        bool  stim = false;          // Committing; Off -> On between; held with armGen 2, endGen 1, before any pump
        std::uint32_t armGen = 0, endGen = 0;
        std::uint64_t held = 0;
        float armAfterPump = 0.0f, armWhileRec = 0.0f;
        std::uint64_t clearsAfterPump = 0, armsRec = 0, cancels = 0;
        RS    stateRec = RS::Idle;
        juce::String seqRec;
        // the held take's own ending (as shipped only)
        std::uint64_t commits = 0, clears = 0;
        float armEnd = 0.0f;
        int   diff = -9;
        juce::String seq;
    };

    HeldR runHeld (bool noGenGuard)
    {
        HeldR r;
        Session s (kFs, recBase (2, 0, { { "rec_length", 0.6f } }), kSeed, 512);
        s.record = false;
        s.p->testNoGenGuard = noGenGuard;
        const ScSig g { 0.5f, 931 };
        inject (s, g);
        Spy spy (*s.p);
        armAt (s, kA);
        s.run (kA + 28800 + 1000, 512);
        const auto pc = pr (s);
        armAt (s, s.pos + 100, false);
        armAt (s, s.pos + 200);
        s.run (1000, 512);
        const auto ph = pr (s);   // still Committing: nothing has pumped
        r.armGen = rec (s).getArmGen();
        r.endGen = rec (s).getEndGen();
        r.held   = rec (s).hooks.heldArms;
        r.stim   = pc.state == RS::Committing && ph.state == RS::Committing && ph.heldArm && r.held == 1 && r.armGen == 2 && r.endGen == 1
                && juce::exactlyEqual (armOf (s), 1.0f) && s.p->recAutoClears == 0 && rec (s).hooks.recArms == 1;

        const bool taken = pumpTimer (s, false);   // the first take's commit + the check
        r.stim = r.stim && taken;
        r.armAfterPump    = armOf (s);
        r.clearsAfterPump = s.p->recAutoClears;

        s.run (4800, 512);    // the copy-out is seen: the held arm goes Armed -> Recording (Manual)
        pump (60);
        const auto pa = pr (s);
        r.armWhileRec = armOf (s);
        r.stateRec    = pa.state;
        r.armsRec     = rec (s).hooks.recArms;
        r.cancels     = rec (s).hooks.recCancels;
        r.seqRec      = spy.seq;
        if (noGenGuard)
            return r;

        s.run (28800, 512);   // the held take runs out
        const auto p2 = pr (s);
        const bool taken2 = pumpTimer (s);
        r.commits = rec (s).hooks.recCommits;
        r.clears  = s.p->recAutoClears;
        r.armEnd  = armOf (s);
        r.seq     = spy.seq;
        r.diff    = (taken2 && p2.state == RS::Committing) ? tapeDiff (s, 2, g, 1.0f, p2.start, p2.frames, 2) : -3;
        return r;
    }

    bool ac5Predicate (const HeldR& r)
    {
        return r.held == 1 && r.armsRec == 2 && juce::exactlyEqual (r.armAfterPump, 1.0f) && juce::exactlyEqual (r.armWhileRec, 1.0f)
            && r.stateRec == RS::Recording;
    }

    void rowsHeld()
    {
        const auto r = runHeld (false);
        const auto n = runHeld (true);
        const bool fired = n.stim && ! ac5Predicate (n) && juce::exactlyEqual (n.armAfterPump, 0.0f) && n.clearsAfterPump == 1 && n.stateRec != RS::Recording;

        row (r.stim && ac5Predicate (r) && r.clearsAfterPump == 0 && r.seqRec == "C1 C0 C1"
                 && r.commits == 2 && r.clears == 1 && juce::exactlyEqual (r.armEnd, 0.0f) && r.diff == 0 && r.seq == "C1 C0 C1 B C0 E",
             "AC5 D66 an On edge during Committing, before the pump: the new take arms; Arm still 1 while it records",
             "meas armGen " + juce::String ((int) r.armGen) + " / endGen " + juce::String ((int) r.endGen) + " at the pump, heldArms " + i64 (r.held)
                 + "; after the pump rec_arm " + a0 (r.armAfterPump) + ", auto-clears " + i64 (r.clearsAfterPump) + "; then recArms " + i64 (r.armsRec) + ", "
                 + stName (r.stateRec) + ", rec_arm " + a0 (r.armWhileRec) + ", host sees [" + r.seqRec + "] (the harness's own Off / On); its own ending: commits "
                 + i64 (r.commits) + ", rec_arm " + a0 (r.armEnd) + ", auto-clears " + i64 (r.clears) + ", tape diffs " + juce::String (r.diff) + ", [" + r.seq
                 + "] | thr heldArms 1, recArms 2, Arm 1 while Recording | margin 0 | ctrl AC5n");
        row (fired, "AC5n control: the endGen == armGen guard off -> the held arm is cancelled (predicate must FAIL)",
             "meas armGen " + juce::String ((int) n.armGen) + " / endGen " + juce::String ((int) n.endGen) + " at the pump, heldArms " + i64 (n.held)
                 + "; after the pump rec_arm " + a0 (n.armAfterPump) + ", auto-clears " + i64 (n.clearsAfterPump) + "; then recArms " + i64 (n.armsRec) + ", "
                 + stName (n.stateRec) + ", rec_arm " + a0 (n.armWhileRec) + ", host sees [" + n.seqRec + "]"
                 + " | thr predicate 'recArms 2, Arm 1 while Recording' FAILS | "
                 + ctrl (fired, "AC5n testNoGenGuard -> take 1's clear lands on arm 2: rec_arm " + a0 (n.armAfterPump) + ", recArms " + i64 (n.armsRec)
                                    + ", " + stName (n.stateRec)));
    }

    //==========================================================================
    // AC7 / AC7n — saved state (PD20: the state functions are untouched)
    juce::MemoryBlock armedBlob()
    {
        auto a = make();
        setAll (*a, recBase (2, 0));
        setP (*a, "rec_arm", 1.0f);
        return stateOf (*a);
    }

    struct RestoreR
    {
        bool  stim = false;       // the restore put rec_arm at 1 with audio stopped; prepare reached the StaleOn branch
        float armRestored = 0.0f, armAfter = 0.0f;
        std::uint64_t stale = 0, arms0 = 0, arms = 0, starts = 0, clears = 0, ends = 0;
        int   begins = 0, zeros = 0, gestureEnds = 0;
        RS    state = RS::Idle;
    };

    RestoreR runRestore (int skip)
    {
        RestoreR r;
        const auto blob = armedBlob();
        Session s (kFs, recBase (2, 0), kSeed, 512);
        s.record = false;
        inject (s, ScSig { 0.5f, 941 });   // a loud sidechain: an arm that could start would
        s.run (4800, 512);
        rec (s).hooks.resetCounters();
        rec (s).hooks.skipEndGen = skip;
        r.arms0 = rec (s).hooks.recArms;
        const float arm0 = armOf (s);
        Spy spy (*s.p);

        s.p->setStateInformation (blob.getData(), (int) blob.getSize());   // audio stopped
        s.p->getLoader().waitForIdle (5000);
        r.armRestored = armOf (s);
        s.p->prepareToPlay (kFs, 512);
        r.stale = rec (s).hooks.staleArms;
        r.ends  = rec (s).hooks.armEnds;
        r.stim  = juce::exactlyEqual (arm0, 0.0f) && juce::exactlyEqual (r.armRestored, 1.0f) && r.stale == 1 && r.ends == (skip == 7 ? 0u : 1u);

        pump (100);
        r.armAfter = armOf (s);
        s.run (48000, 512);
        pump (40);
        r.arms   = rec (s).hooks.recArms;
        r.starts = rec (s).hooks.recStarts;
        r.clears = s.p->recAutoClears;
        r.state  = pr (s).state;
        r.begins = spy.begins; r.zeros = spy.zeros; r.gestureEnds = spy.ends;
        return r;
    }

    bool ac7Predicate (const RestoreR& r) { return juce::exactlyEqual (r.armAfter, 0.0f) && r.arms == r.arms0; }

    struct RoundR
    {
        bool same = false;
        float arm0 = 0.0f, arm1 = 0.0f;
        size_t size = 0;
        std::uint64_t clears = 0;
        int diffElems = 0;          // top-level XML children that differ between the two blobs
        bool paramsSame = false;    // every <PARAM> identical (rec_arm included)
        juce::String diffNote;
    };

    /** (b) save, restore onto the same instance, save: nothing changed between. */
    RoundR runRound (bool armOn)
    {
        RoundR r;
        Session s (kFs, recBase (2, 1), kSeed, 512);
        s.record = false;
        s.run (4800, 512);
        if (armOn)
            setP (*s.p, "rec_arm", 1.0f);   // audio stopped: no edge, no ending
        r.arm0 = armOf (s);
        const auto m1 = stateOf (*s.p);
        s.p->setStateInformation (m1.getData(), (int) m1.getSize());
        s.p->getLoader().waitForIdle (5000);
        pump (60);                          // the Timer runs: nothing ended, nothing to clear
        const auto m2 = stateOf (*s.p);
        r.arm1 = armOf (s);
        r.size = m1.getSize();
        r.same = m1 == m2;
        // Where two blobs differ (evidence for the row; the verdict is the byte compare).
        const auto x1 = xmlOf (m1), x2 = xmlOf (m2);
        if (x1 != nullptr && x2 != nullptr)
        {
            juce::StringArray notes;
            int n = 0;
            bool paramsSame = x1->getNumChildElements() == x2->getNumChildElements();
            auto* c2 = x2->getFirstChildElement();
            for (auto* c1 = x1->getFirstChildElement(); c1 != nullptr && c2 != nullptr; c1 = c1->getNextElement(), c2 = c2->getNextElement())
            {
                if (c1->isEquivalentTo (c2, false))
                    continue;
                ++n;
                paramsSame = paramsSame && ! c1->hasTagName ("PARAM");
                for (int k = 0; k < c1->getNumAttributes() && notes.size() < 3; ++k)
                    if (c1->getAttributeValue (k) != c2->getStringAttribute (c1->getAttributeName (k)))
                        notes.add ("<" + c1->getTagName() + " " + c1->getAttributeName (k) + "> " + c1->getAttributeValue (k) + " -> "
                                       + c2->getStringAttribute (c1->getAttributeName (k)));
                if (notes.isEmpty())
                    notes.add ("<" + c1->getTagName() + "> (children)");
            }
            r.diffElems = n;
            r.paramsSame = paramsSame;
            r.diffNote = notes.joinIntoString ("; ");
        }
        r.clears = s.p->recAutoClears;
        return r;
    }

    juce::String roundText (const RoundR& b)
    {
        if (b.same)
            return "identical";
        return "DIFFER in " + juce::String (b.diffElems) + " element(s): " + b.diffNote + "; all <PARAM> rows (rec_arm included) "
             + (b.paramsSame ? "identical" : "DIFFER");
    }

    void rowsState()
    {
        const auto r = runRestore (0);
        const auto n = runRestore (7);
        const auto b1 = runRound (true);
        const auto b0 = runRound (false);
        const bool fired = n.stim && ! ac7Predicate (n) && juce::exactlyEqual (n.armAfter, 1.0f) && n.clears == 0 && n.arms == n.arms0;
        // Amended at T13 (recorded in SUMMARY-R1): a byte compare cannot pass on this
        // plugin, R1 or not — every restore bumps the <CustomScale version> runtime
        // counter (PluginProcessor.cpp customVersion.fetch_add) and the save writes
        // it. (b) is therefore: identical, or the ONLY differing element is that
        // counter and every <PARAM> row (rec_arm included) is identical.
        const auto roundOk = [] (const RoundR& b)
        {
            return b.same || (b.paramsSame && b.diffElems == 1 && b.diffNote.startsWith ("<CustomScale version>"));
        };
        const bool okB = roundOk (b1) && juce::exactlyEqual (b1.arm0, 1.0f) && juce::exactlyEqual (b1.arm1, 1.0f) && b1.clears == 0 && b1.size > 0
                      && roundOk (b0) && juce::exactlyEqual (b0.arm0, 0.0f) && juce::exactlyEqual (b0.arm1, 0.0f) && b0.size > 0;

        row (r.stim && ac7Predicate (r) && r.starts == 0 && r.clears == 1 && r.state == RS::Idle && r.begins == 1 && r.zeros == 1 && r.gestureEnds == 1 && okB,
             "AC7 D66 (a) a blob with rec_arm 1 restored, prepareToPlay, pump: Arm 0, recArms unchanged; (b) save, restore, save: every PARAM identical (only the CustomScale version counter may move)",
             "meas (a) rec_arm restored " + a0 (r.armRestored) + ", staleArms " + i64 (r.stale) + ", after the pump " + a0 (r.armAfter) + " (begin / 0 / end "
                 + juce::String (r.begins) + " / " + juce::String (r.zeros) + " / " + juce::String (r.gestureEnds) + "), recArms " + i64 (r.arms0) + " -> "
                 + i64 (r.arms) + " and recStarts " + i64 (r.starts) + " over 1 s of loud sidechain, " + stName (r.state) + "; (b) Arm On: "
                 + roundText (b1) + " (" + juce::String ((juce::int64) b1.size) + " bytes, rec_arm " + a0 (b1.arm0) + " -> " + a0 (b1.arm1)
                 + ", Timer pumped 60 ms between), Arm Off: " + roundText (b0) + " (" + juce::String ((juce::int64) b0.size)
                 + " bytes) | thr 0, unchanged, identical | margin 0 | ctrl AC7n");
        row (fired, "AC7n control: the StaleOn (row-11) ending removed -> Arm stays 1 (predicate must FAIL)",
             "meas rec_arm restored " + a0 (n.armRestored) + ", staleArms " + i64 (n.stale) + " (branch reached), endGen writes " + i64 (n.ends)
                 + ", after the pump " + a0 (n.armAfter) + ", auto-clears " + i64 (n.clears) + ", recArms " + i64 (n.arms)
                 + " | thr predicate 'Arm 0, recArms unchanged' FAILS on Arm | "
                 + ctrl (fired, "AC7n skipEndGen 7 -> rec_arm " + a0 (n.armAfter) + " after the same restore, prepare and pump"));

        // INFO (PD31, OS2): a restore while blocks are running is an ordinary
        // On edge today — the take starts. Not a row.
        {
            const auto blob = armedBlob();
            Session s (kFs, recBase (2, 0), kSeed, 512);
            s.record = false;
            inject (s, ScSig { 0.5f, 942 });
            s.run (4800, 512);
            rec (s).hooks.resetCounters();
            s.p->setStateInformation (blob.getData(), (int) blob.getSize());   // between two blocks, no prepare
            s.p->getLoader().waitForIdle (5000);
            const float armR = armOf (s);
            s.run (4800, 512);
            const auto p = pr (s);
            std::printf ("  INFO AC7 a restore of rec_arm 1 while blocks are running (no prepareToPlay; OS2, today's edge behaviour): rec_arm %s, recArms %s, recStarts %s, state %s after 100 ms\n",
                         a0 (armR).toRawUTF8(), i64 (rec (s).hooks.recArms).toRawUTF8(), i64 (rec (s).hooks.recStarts).toRawUTF8(), stName (p.state));
            std::fflush (stdout);
        }
    }

    //==========================================================================
    // AC8 — a preset applied while waiting Armed
    void rowAC8()
    {
        // A factory preset that MOVES the record rows: rec_trigger 2 (Next Note), rec_target 1.
        const auto& b = pg4::bank();
        int idx = -1;
        for (size_t i = 0; i < b.specs.size() && idx < 0; ++i)
        {
            bool trig2 = false, target1 = false;
            for (const auto& [k, v] : b.specs[i].values)
            {
                trig2   = trig2   || (juce::String (k) == "rec_trigger" && juce::exactlyEqual (v, 2.0f));
                target1 = target1 || (juce::String (k) == "rec_target"  && juce::exactlyEqual (v, 1.0f));
            }
            if (trig2 && target1)
                idx = (int) i;
        }

        Session s (kFs, recBase (2, 1), kSeed, 512);   // Threshold, slot index 2
        s.record = false;
        const juce::int64 T = kA + 48000;
        ScSig g { 0.5f, 951 };
        g.from = T;                                    // silent while waiting
        inject (s, g);
        Spy spy (*s.p);
        armAt (s, kA);
        s.run (kA + 9600, 512);
        const auto p0 = pr (s);
        const float trig0 = rawOf (*s.p, "rec_trigger"), target0 = rawOf (*s.p, "rec_target");

        bool applied = false;
        if (idx >= 0)
        {
            applied = s.p->getPresetManager().applyPresetData (ogi::factory::presetJsonOf (b.defs[(size_t) idx]));
            s.p->getLoader().waitForIdle (20000);
        }
        const float trig1 = rawOf (*s.p, "rec_trigger"), target1 = rawOf (*s.p, "rec_target");
        for (int k = 0; k < 5; ++k)
        {
            s.run (4800, 512);
            pump (30);
        }
        const auto p1 = pr (s);
        const float arm1 = armOf (s);
        const juce::String seq1 = spy.seq;
        const bool stim = idx >= 0 && applied && p0.state == RS::Armed && p0.trigger == RT::Threshold && p0.slot == 2
                       && juce::exactlyEqual (trig0, 1.0f) && juce::exactlyEqual (target0, 2.0f)
                       && juce::exactlyEqual (trig1, 2.0f) && juce::exactlyEqual (target1, 1.0f) && s.pos < T;

        // The latch still decides: no note is played; the level crossing starts the take, on the latched slot.
        s.run (T + 9600 - s.pos, 512);
        const auto p2 = pr (s);
        const std::uint64_t starts = rec (s).hooks.recStarts, arms = rec (s).hooks.recArms;

        const juce::String name = idx >= 0 ? juce::String::fromUTF8 (b.specs[(size_t) idx].name) : juce::String ("(none found)");
        row (stim && juce::exactlyEqual (arm1, 1.0f) && p1.state == RS::Armed && p1.trigger == RT::Threshold && p1.slot == 2 && seq1 == "C1"
                 && s.p->recAutoClears == 0 && p2.state == RS::Recording && p2.slot == 2 && starts == 1 && arms == 1,
             "AC8 D66 a factory preset applied while waiting Armed (Threshold): Arm 1, Armed, latched trigger unchanged",
             "meas preset \"" + name + "\" moved rec_trigger " + a0 (trig0) + " -> " + a0 (trig1) + ", rec_target " + a0 (target0) + " -> " + a0 (target1)
                 + "; 0.5 s later (5 pumps) rec_arm " + a0 (arm1) + ", " + stName (p1.state) + ", latched trigger " + juce::String ((int) p1.trigger)
                 + " (Threshold = 1), latched slot " + juce::String (p1.slot) + ", host sees [" + seq1 + "], auto-clears " + i64 (s.p->recAutoClears)
                 + "; then the level crossing (no note): " + stName (p2.state) + " on slot " + juce::String (p2.slot) + ", recStarts " + i64 (starts)
                 + " | thr 1, Armed, 1 / 2 unchanged | margin 0 | ctrl - (UPM3n)");
    }

    //==========================================================================
    //==========================================================================
    // PLAN-R1 PD30 (task T24): the per-stream feedback + reverb rows.
    //
    //   Pre = sN_reverb_pre 1 (the loop tank, "tank B", sits on the tap before
    //   the chain); tank A = the audible tank of a stream. Every row checks
    //   that its stimulus fired BEFORE its result (a row whose stimulus did
    //   not fire prints INVALID and FAILS). Every `n` row runs its parent's
    //   predicate on a deliberately broken input and PASSES only when that
    //   predicate FAILS. Every per-stream row has an arm on a stream >= 2.
    //
    //   FL1p   FL1 rows + coherent tap, Pre x 4, 60 s: write bound, finite  FL1pn  scanners
    //   FL1sp  the same at sat 0, 0.5-peak tapes: written <= 0.7f exactly  FL1spn rClampOff (no injectReturn)
    //   FL2p   FL2 geometry in Pre: 16 voices <= 1 voice + 1 dB            FL2pn  sumTap
    //   FL3p   Freeze while the Pre tail rings: hashes / writes frozen     FL3pn  the hash at the edge
    //   FL4p   Pre, Mix 100, feedback 0: tapes == pristine                 FL4pn  feedback 0.1 %
    //   FL4t   sN_fb_enable Off from load (Post + Pre): == pristine        FL4tn  toggle On
    //   FL4m   toggle Off + Macro 100 % -> sN_feedback: 0 writes           FL4mn  toggle On
    //   FL5p   Pre / toggle switches on s2, s4: tapes 1, 3 untouched       FL5pn  the switches on s1
    //   TG1    NaN on a voice bus into a live loop tank: scrubbed          TG1n   tankGuardOff
    //   RS2p   Reset while the head runs on the Pre tail: == pristine      RS2pn  preHoldOff
    //   MU1    mute a Pre stream: its head stops writing                   MU1n   gateBIgnoreAudible
    //   IN1    one stream's 10 new parameters move nothing else            IN1n   the other stream's
    //   SI1    reverb sits before the stream sum                           SI1n   tankOnSum
    //   TK1    tank A idles after its tail; re-entry is a cold start       TK1n   tankGateSticky
    //   MZ1    N1 through the per-stream path == renders-2.4; signed zeros MZ1n   addZeroTerm
    //   FA4p   0 audio-thread allocations across the R1 events             FA4pn  an armed malloc
    //   CK7    feedback toggle Off / On                                    CK7n   fbOnHardStop
    //   CK8    Pre <-> Post on the return probe (+ INFO CK8o, the output)  CK8n   preHardCut
    //   CK9    mute with the reverb on                                     CK9n   revTailOff
    //   DC1    destination class == mod-dests.tsv                          DC1n   destClassLegacy
    //   MC1    per-voice source -> class-G stream destination              MC1n   destClassLegacy
    //   UHD1   the frozen head is per lane                                 UHD1n  a stream-1-only rule
    //   INFO   FL1lit (PD30's literal FL1p / FL1sp stimulus), FL8p (FL8 in Pre),
    //          PreTail (seconds the head writes after note-off), CK8o
    using eg21::P;
    using eg21::with;
    using eg21::sameF;
    using eg21::sineBuf;
    using eg22::base4;
    using eg22::all4;
    using eg22::cat;
    using eg22::sn;
    using eg22::shipping;
    using eg22::setAt;
    using eg22::setAt4;
    using eg22::setP4;
    using eg24::Hashes;
    using eg24::tapeHashes;
    using eg24::Classify;
    using eg24::classify;
    using eg24::FlOut;

    constexpr float kLive = 1.0e-5f;   // the engine's run / tail threshold (−100 dBFS)

    /** base4 with exactly stream n enabled (n = 1 is base4 (1)). */
    P onlyStream (int n)
    {
        P p = base4 (4);
        for (int k = 1; k <= 4; ++k)
            p = with (p, { { sn (k, "enable"), k == n ? 1.0f : 0.0f } });
        return p;
    }

    /** Row values on stream n. */
    P snp (int n, std::initializer_list<std::pair<const char*, float>> kv)
    {
        P p;
        for (const auto& x : kv)
            p.push_back ({ sn (n, x.first), x.second });
        return p;
    }

    P pre4() { return all4 ("reverb_pre", 1.0f); }

    /** FL1p / FL1sp stimulus (T24 deviation, measured): with FL1's own grain
        rows a Pre loop on noise tapes DECAYS: 16 voices x ~100 overlapping
        grains read unrelated tape positions, the mean tap is ~ -30 dB under the
        tape, and the chain input peaks at 0.086 (FL1p) / never reaches the
        writer clamp (FL1sp: rClamps 0) — both rows' validity clauses fail and
        FL1sp's control cannot fire. PD30: "if it does not fire, raise the
        stimulus". Raised by making the cloud coherent: Key Track 0, Pitch 0,
        Scan Rate 100 % (every grain of every voice reads the same tape frame at
        the same time, so the tap is the tape at full level). Everything else
        is FL1's: 16 voices x 4 streams, fb 100 %, Mix 100, Size 100, density
        200, size 500 ms, level +6 dB, the resonant filters, the tapes. */
    P coherentTap() { return cat ({ all4 ("key_track", 0.0f), all4 ("pitch", 0.0f), all4 ("scan_rate", 100.0f) }); }

    juce::String yn (bool b) { return b ? "yes" : "NO"; }
    juce::String u4 (const std::uint64_t (&v)[4])
    {
        return i64 (v[0]) + "/" + i64 (v[1]) + "/" + i64 (v[2]) + "/" + i64 (v[3]);
    }

    int countedVoices (Session& s)
    {
        int n = 0;
        for (int k = 0; k < ogi::kVoiceSlots; ++k)
            n += s.hk().voiceState[k] == 1 ? 1 : 0;
        return n;
    }

    std::uint64_t settlesAll (Session& s)
    {
        std::uint64_t n = 0;
        for (const auto& c : s.hk().head) n += c.settles;
        return n;
    }
    std::uint64_t parksAll (Session& s)
    {
        std::uint64_t n = 0;
        for (const auto& c : s.hk().head) n += c.parks;
        return n;
    }

    //==========================================================================
    // The FL1 render (eg24::flRender) with the R1 readouts: gate-B cells, the
    // tap guard, the chain-input probe of one slot, the latched toggles.
    struct PreOut
    {
        FlOut fl;
        std::uint64_t bCells[4] {}, aCells[4] {}, guard = 0, settles = 0, parks = 0;
        double probePk = 0.0;          // max |chain input| of probeSlot (after tank B)
        bool pre[4] {}, fbOn[4] {};
        float traceMax = 0.0f;
        std::int64_t traceN = 0;
    };

    PreOut preRender (const P& params, double seconds, float tapeAmp, int probeSlot = -1,
                      const std::function<void (Session&)>& tweak = {}, int traceDest = -1)
    {
        Session s (kFs, params, kSeed, 512);
        shipping (s);
        if (tweak) tweak (s);
        const juce::int64 n = (juce::int64) (seconds * kFs);
        std::vector<float> tap, tr;
        if (probeSlot >= 0)
        {
            tap.assign ((size_t) (2 * n), 0.0f);
            s.hk().tapProbe = { tap.data(), n, probeSlot };
        }
        if (traceDest >= 0)
        {
            tr.assign ((size_t) (n / 32 + 64), 0.0f);
            s.hk().traceDest = { traceDest, -1, tr.data(), (std::int64_t) tr.size(), 0 };
        }
        for (int k = 0; k < 4; ++k)
            s.load (k, noiseBuf (2, (int) (4.0 * kFs), tapeAmp, 21 + k), kFs);
        for (int note = 40; note < 56; ++note)
            s.noteOn (0, note);
        PreOut r;
        auto& o = r.fl;
        s.run (32, 32);   // the tick takes the tapes; the notes land after it (no write yet)
        for (int k = 0; k < 4; ++k)
            if (const auto* t = s.eng().getCurrentTape (k))
                o.pristinePk[(size_t) k] = t->tape.levelPeak (0);
        s.run (n - 32, 512);
        auto& h = s.hk();
        for (int k = 0; k < 4; ++k)
        {
            const auto* t = s.eng().getCurrentTape (k);
            float fin = 0.0f;
            if (t != nullptr)
            {
                fin = t->tape.levelPeak (0);
                for (int l = 0; l < 5; ++l) o.levelPk[k][l] = t->tape.levelPeak (l);
                t->tape.scanForTest (o.tape.nonFinite, o.tape.subnormal);
            }
            o.written[(size_t) k] = std::max (h.head[k].maxWritten, fin);
            o.bound[(size_t) k] = std::max (o.pristinePk[(size_t) k], ogi::kAgcCeil);
            o.writes[k] = h.head[k].writes;
            o.rClamps += h.head[k].rClamps;
            o.writeScrubs += h.head[k].writeScrubs;
            eg24::chainScan (s.eng().getChainForTest (k), o.chain);
            r.bCells[k] = h.tankBRunCells[k];
            r.aCells[k] = h.tankRunCells[k];
            r.pre[k]  = s.eng().getReverbPreForTest (k);
            r.fbOn[k] = s.eng().getFbOnForTest (k);
        }
        for (size_t i = 0; i < s.L.size(); ++i)
        {
            classify (s.L[i], o.out); classify (s.R[i], o.out);
            o.outPk = std::max ({ o.outPk, (double) std::abs (s.L[i]), (double) std::abs (s.R[i]) });
        }
        o.forced = h.forcedCrossings;
        o.pre = h.preKneePeak;
        o.knee = h.kneePeak;
        o.hash = tapeHashes (s);
        r.guard = h.tankGuardScrubs;
        r.settles = settlesAll (s);
        r.parks = parksAll (s);
        for (float x : tap)
            if (std::abs (x) > r.probePk) r.probePk = std::abs (x);   // NaN never wins
        r.traceN = h.traceDest.n;
        for (std::int64_t i = 0; i < r.traceN; ++i)
            r.traceMax = std::max (r.traceMax, tr[(size_t) i]);
        h.tapProbe = {};
        h.traceDest = {};
        return r;
    }

    bool all4b (const bool (&v)[4], bool want_) { return v[0] == want_ && v[1] == want_ && v[2] == want_ && v[3] == want_; }
    bool allPos (const std::uint64_t (&v)[4]) { return v[0] > 0 && v[1] > 0 && v[2] > 0 && v[3] > 0; }
    std::uint64_t sum4 (const std::uint64_t (&v)[4]) { return v[0] + v[1] + v[2] + v[3]; }

    std::array<std::uint64_t, 4> pristine4()   // the FL tapes (noise 1.0, seeds 21..24)
    {
        std::array<std::uint64_t, 4> h {};
        for (int k = 0; k < 4; ++k)
            h[(size_t) k] = eg24::pristineHash (noiseBuf (2, (int) (4.0 * kFs), 1.0f, 21 + k), kFs, kFs);
        return h;
    }
    int sameAs (const Hashes& a, const std::array<std::uint64_t, 4>& b)
    {
        int n = 0;
        for (size_t k = 0; k < 4; ++k) n += a[k] == b[k] ? 1 : 0;
        return n;
    }

    //==========================================================================
    // FL1p / FL1pn
    bool fl1Predicate (const FlOut& o)
    {
        return eg24::boundViolations (o.written, o.bound) == 0 && o.out.nonFinite == 0 && o.out.subnormal == 0
            && o.tape.nonFinite == 0 && o.tape.subnormal == 0 && o.chain.nonFinite == 0 && o.chain.subnormal == 0;
    }

    void rowFL1p()
    {
        constexpr int kProbe = 2;   // stream 3's chain input
        const auto r = preRender (cat ({ eg24::flParams (20.0f), pre4(), coherentTap() }), 60.0, 1.0f, kProbe);
        const auto& o = r.fl;
        const int viol = eg24::boundViolations (o.written, o.bound);
        const bool stim = r.probePk > (double) ogi::kAgcCeil && all4b (r.pre, true) && all4b (r.fbOn, true) && allPos (r.bCells) && allPos (o.writes);
        row (stim && fl1Predicate (o),
             "FL1p PD8 60 s stress in Pre x 4 (FL1 rows, coherent tap; fb 100 %, Mix 100, Size 100): tape peak <= max (pristine, 0.7), all finite",
             "meas bound violations " + juce::String (viol) + " |" + eg24::peaksInfo (o) + " | out non-finite " + i64 (o.out.nonFinite)
                 + " subnormal " + i64 (o.out.subnormal) + ", tape (all levels) " + i64 (o.tape.nonFinite) + "/" + i64 (o.tape.subnormal)
                 + ", chains " + i64 (o.chain.nonFinite) + "/" + i64 (o.chain.subnormal) + ", writes " + u4 (o.writes)
                 + " | stimulus: chain-input peak s3 " + f2 (r.probePk, 4) + " (> 0.7), gate-B cells " + u4 (r.bCells)
                 + (stim ? "" : ", INVALID") + " | INFO rClamps " + i64 (o.rClamps) + ", tap-guard scrubs " + i64 (r.guard)
                 + ", writeScrubs " + i64 (o.writeScrubs) + " | thr 0 | ctrl FL1pn");

        // The same predicate on three broken inputs (FL1's scanners).
        FlOut b1 = o, b2 = o, b3 = o;
        b1.written[(size_t) kProbe] = std::nextafter (b1.bound[(size_t) kProbe], 2.0f);
        classify (std::numeric_limits<float>::denorm_min(), b2.out);
        classify (std::numeric_limits<float>::quiet_NaN(), b3.tape);
        const bool f1 = ! fl1Predicate (b1), f2_ = ! fl1Predicate (b2), f3 = ! fl1Predicate (b3);
        const bool fired = stim && fl1Predicate (o) && f1 && f2_ && f3;
        row (fired, "FL1pn control: bound + 1 ulp, a denorm_min, a NaN fed to FL1p's predicate (predicate must FAIL)",
             juce::String ("meas predicate on s3 written = bound + 1 ulp: ") + (f1 ? "fails" : "PASSES") + "; + denorm_min in the output: "
                 + (f2_ ? "fails" : "PASSES") + "; + NaN in a tape: " + (f3 ? "fails" : "PASSES") + " | thr 3 / 3 fail | "
                 + ctrl (fired, "FL1pn the scanners / bound checker see 1 ulp, 1 subnormal, 1 non-finite"));
    }

    //==========================================================================
    // FL1sp / FL1spn
    bool fl1sPredicate (const FlOut& o)
    {
        bool exact = true;
        for (auto b : o.bound) exact = exact && sameF (b, ogi::kAgcCeil);
        return exact && eg24::boundViolations (o.written, o.bound) == 0 && o.tape.nonFinite == 0;
    }

    void rowFL1sp()
    {
        const P p = cat ({ eg24::flParams (0.0f), pre4(), coherentTap() });
        const auto r = preRender (p, 60.0, 0.5f);
        const auto& o = r.fl;
        const bool stim = o.rClamps > 0 && all4b (r.pre, true) && allPos (r.bCells) && allPos (o.writes);
        row (stim && fl1sPredicate (o),
             "FL1sp PD8 Pre, Mix 100, Size 100, sat 0, 0.5-peak tapes, coherent tap, 60 s: written <= 0.7f exactly on 4 slots",
             "meas bound violations " + juce::String (eg24::boundViolations (o.written, o.bound)) + " |" + eg24::peaksInfo (o)
                 + " | stimulus: rClamps " + i64 (o.rClamps) + " (> 0), gate-B cells " + u4 (r.bCells) + (stim ? "" : ", INVALID")
                 + " | thr 0, bound 0.7f | ctrl FL1spn");

        const auto c = preRender (p, 10.0, 0.5f, -1, [] (Session& s) { s.hk().rClampOff = true; });   // no injectReturn
        float worst = 0.0f;
        for (auto w : c.fl.written) worst = std::max (worst, w);
        const bool fired = stim && ! fl1sPredicate (c.fl) && worst > ogi::kAgcCeil && c.fl.rClamps == 0;
        row (fired, "FL1spn control: rClampOff, 10 s, no injectReturn -> written > 0.7 (predicate must FAIL)",
             "meas written" + eg24::peaksInfo (c.fl) + ", rClamps " + i64 (c.fl.rClamps) + " | thr predicate 'written <= 0.7f' FAILS | "
                 + ctrl (fired, "FL1spn the loop's own return, unclamped -> written " + f2 (worst, 4)));
    }

    /** INFO: PD30's literal FL1p / FL1sp stimulus (FL1's grain rows, no coherent tap), 15 s. */
    void infoFL1lit()
    {
        const auto a = preRender (cat ({ eg24::flParams (20.0f), pre4() }), 15.0, 1.0f, 2);
        const auto b = preRender (cat ({ eg24::flParams (0.0f), pre4() }), 15.0, 0.5f, 2);
        std::printf ("  INFO FL1lit PD30's literal stimulus (FL1 rows as they are, Pre x 4, 15 s): sat 20, 1.0 tapes: chain-input peak s3 %.4f, tape peak after 15 s %.4f; "
                     "sat 0, 0.5 tapes: chain-input peak s3 %.4f, rClamps %llu, tape peak after 15 s %.4f (the loop decays: below both rows' stimulus clauses, hence the coherent tap)\n",
                     a.probePk, (double) a.fl.levelPk[2][0], b.probePk, (unsigned long long) b.fl.rClamps, (double) b.fl.levelPk[2][0]);
        std::fflush (stdout);
    }

    //==========================================================================
    // FL2p / FL2pn — the runaway guard in Pre (FL2 geometry on stream n)
    struct Fl2R { double db = -400.0; std::uint64_t bCells = 0; };

    Fl2R fl2pTapRms (int n, int voices, bool sumTap)
    {
        P p = cat ({ onlyStream (n), { { "polyphony", 16.0f }, { "grain_limit", 3.0f } },
                     snp (n, { { "feedback", 100.0f }, { "reverb_pre", 1.0f }, { "reverb_mix", 100.0f }, { "reverb_size", 50.0f },
                               { "density", 200.0f }, { "size", 500.0f }, { "key_track", 0.0f }, { "level", 6.0f },
                               { "filter_type", 1.0f }, { "filter_cutoff", 1000.0f }, { "filter_res", 100.0f },
                               { "spray", 30.0f }, { "jitter", 50.0f } }) });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.hk().sumTap = sumTap;
        s.record = false;
        s.load (n - 1, noiseBuf (2, (int) (4.0 * kFs), 0.05f, 31), kFs);
        const juce::int64 N = (juce::int64) (20.0 * kFs);
        std::vector<float> tap ((size_t) (2 * N), 0.0f);
        s.hk().tapProbe = { tap.data(), N, n - 1 };
        for (int k = 0; k < voices; ++k)
            s.noteOn (0, 40 + k);
        s.run (N, 512);
        s.hk().tapProbe = {};
        double e = 0;
        const size_t a = (size_t) (10.0 * kFs);
        for (size_t i = 2 * a; i < tap.size(); ++i) e += (double) tap[i] * tap[i];
        return { 10.0 * std::log10 (std::max (1e-300, e / (double) (tap.size() - 2 * a))), s.hk().tankBRunCells[n - 1] };
    }

    void rowFL2p()
    {
        bool ok = true, stim = true, fired = true;
        juce::String d, dc;
        for (int n : { 1, 3 })
        {
            const auto r1 = fl2pTapRms (n, 1, false), r16 = fl2pTapRms (n, 16, false);
            const auto c1 = fl2pTapRms (n, 1, true),  c16 = fl2pTapRms (n, 16, true);
            const bool st = r1.bCells > 0 && r16.bCells > 0 && r1.db > -100.0 && c1.bCells > 0 && c16.bCells > 0;
            stim = stim && st;
            ok = ok && r16.db <= r1.db + 1.0;                       // the predicate
            fired = fired && ! (c16.db <= c1.db + 1.0) && c16.db - c1.db >= 6.0;
            d << " s" << n << ": 1 voice " << f2 (r1.db, 2) << " dB, 16 voices " << f2 (r16.db, 2) << " dB (delta " << f2 (r16.db - r1.db, 2)
              << ", gate-B cells " << i64 (r1.bCells) << " / " << i64 (r16.bCells) << ");";
            dc << " s" << n << " delta " << f2 (c16.db - c1.db, 2) << " dB;";
        }
        row (stim && ok, "FL2p PD8 runaway guard in Pre (Mix 100, Size 50, tape noise 0.05): chain-input RMS 16 voices <= 1 voice + 1 dB",
             "meas chain input (after the loop tank)" + d + (stim ? "" : " INVALID") + " | thr <= +1 dB | ctrl FL2pn");
        row (stim && fired, "FL2pn control: sumTap -> 16 voices >= 1 voice + 6 dB (predicate must FAIL)",
             "meas" + dc + " | thr predicate '<= +1 dB' FAILS, delta >= +6 dB | " + ctrl (stim && fired, "FL2pn sumTap ->" + dc));
    }

    //==========================================================================
    // FL3p / FL3pn — Freeze while the Pre tail rings (notes off, heads still run)
    void rowFL3p()
    {
        Session s (kFs, cat ({ eg24::loopParams (100.0f), pre4(), all4 ("reverb_mix", 60.0f), all4 ("reverb_size", 100.0f) }), kSeed, 512);
        shipping (s);
        s.record = false;
        eg24::loadLoop4 (s, 3.0, 0.5f);
        for (int n : { 48, 50, 52, 53, 55, 57, 59, 60 })
        {
            s.noteOn (0, n);
            s.noteOff (216000, n);            // 4.5 s
        }
        eg24::setFreezeAt (s, 240000, true);  // 5.0 s: 0.5 s into the tail
        s.run (240000, 512);
        const auto hEdge = tapeHashes (s);
        const auto wEdge = eg24::writesAll (s);
        bool running = true, gateB = true;
        float retMin = 1.0e9f;
        for (int k = 0; k < 4; ++k)
        {
            const auto& hd = s.eng().getHeadForTest (k);
            running = running && ! hd.parked;
            gateB = gateB && s.eng().getTankGateBForTest (k);
            retMin = std::min (retMin, hd.returnPk);
        }
        const bool noVoice = countedVoices (s) == 0;
        s.run (4800, 512);                    // past the 20 ms ramp, the park and its settle
        const auto hA = tapeHashes (s);
        const auto wA = eg24::writesAll (s);
        const auto parks = parksAll (s);
        s.run ((juce::int64) (10.0 * kFs), 512);
        const auto hB = tapeHashes (s);
        const auto wB = eg24::writesAll (s);
        const bool stim = running && gateB && retMin > kLive && noVoice && wEdge > 0;
        const bool pass = hA == hB && wA == wB && parks >= 4 && wA > 0;
        row (stim && pass, "FL3p PD8 Freeze while the Pre tail rings (notes off 0.5 s earlier): hashes after ramp + settle == 10 s later",
             "meas hashes " + juce::String (hA == hB ? "equal" : "DIFFER") + ", writes " + i64 (wA) + " -> " + i64 (wB) + ", parks " + i64 (parks)
                 + " | stimulus at the edge: 4 heads running " + yn (running) + ", gate B x 4 " + yn (gateB) + ", min return peak " + juce::String (retMin, 5)
                 + " (> 1e-5), voices 0 " + yn (noVoice) + (stim ? "" : ", INVALID") + " | thr bitwise | ctrl FL3pn");
        const bool fired = stim && ! (hEdge == hB && wEdge == wB);
        row (fired, "FL3pn control: the hash at the Freeze edge (before the ramp) vs 10 s later (predicate must FAIL)",
             "meas edge hashes " + juce::String (hEdge == hB ? "equal" : "differ") + ", writes " + i64 (wEdge) + " -> " + i64 (wB)
                 + " | thr predicate 'hashes equal, writes frozen' FAILS | " + ctrl (fired, "FL3pn the head wrote " + i64 (wB - wEdge) + " frames during the 20 ms ramp"));
    }

    //==========================================================================
    // FL4p / FL4pn — Pre, Mix 100, feedback 0
    void rowFL4p()
    {
        const auto pr4 = pristine4();
        const auto r = preRender (cat ({ eg24::flParams (20.0f, 0.0f), pre4() }), 60.0, 1.0f);
        const int same = sameAs (r.fl.hash, pr4);
        const auto w = sum4 (r.fl.writes);
        const bool stim = all4b (r.pre, true) && all4b (r.fbOn, true) && r.fl.outPk > 0.1 && allPos (r.aCells);
        row (stim && same == 4 && w == 0, "FL4p PD9 Pre, Mix 100, feedback 0, 60 s: every tape hash == its pristine build, 0 writes",
             "meas " + juce::String (same) + "/4 hashes == pristine, writes " + i64 (w) + ", gate-B cells " + u4 (r.bCells)
                 + " | stimulus: Pre x 4 " + yn (all4b (r.pre, true)) + ", output peak " + f2 (r.fl.outPk, 3) + ", tank-A cells " + u4 (r.aCells)
                 + (stim ? "" : ", INVALID") + " | thr 4/4, 0 | ctrl FL4pn");
        const auto c = preRender (cat ({ eg24::flParams (20.0f, 0.1f), pre4() }), 5.0, 1.0f);
        const int cSame = sameAs (c.fl.hash, pr4);
        const auto cw = sum4 (c.fl.writes);
        const bool fired = ! (cSame == 4 && cw == 0) && cSame == 0;
        row (fired, "FL4pn control: feedback 0.1 % (5 s) -> tapes differ (predicate must FAIL)",
             "meas " + juce::String (cSame) + "/4 hashes == pristine, writes " + i64 (cw) + ", gate-B cells " + u4 (c.bCells)
                 + " | thr predicate '4/4 == pristine, 0 writes' FAILS | " + ctrl (fired, "FL4pn feedback 0.1 % -> " + juce::String (4 - cSame) + "/4 differ"));
    }

    //==========================================================================
    // FL4t / FL4tn — the toggle Off from load
    void rowFL4t()
    {
        const auto pr4 = pristine4();
        bool ok = true, stim = true, fired = true;
        juce::String d, dc;
        for (int pre = 0; pre < 2; ++pre)
        {
            const P base = cat ({ eg24::flParams (20.0f, 100.0f), all4 ("reverb_pre", (float) pre) });
            const auto r = preRender (cat ({ base, all4 ("fb_enable", 0.0f) }), 20.0, 1.0f);
            const int same = sameAs (r.fl.hash, pr4);
            const auto w = sum4 (r.fl.writes);
            const bool st = all4b (r.fbOn, false) && all4b (r.pre, pre == 1) && r.fl.outPk > 0.1;
            stim = stim && st;
            ok = ok && same == 4 && w == 0 && r.settles == 0;
            d << (pre ? " Pre: " : " Post: ") << same << "/4 == pristine, writes " << i64 (w) << ", settles " << i64 (r.settles) << ", parks " << i64 (r.parks)
              << ", gate-B cells " << u4 (r.bCells) << " (toggle Off x 4 " << yn (all4b (r.fbOn, false)) << ", output peak " << f2 (r.fl.outPk, 3) << ");";
            const auto c = preRender (cat ({ base, all4 ("fb_enable", 1.0f) }), 5.0, 1.0f);
            const int cSame = sameAs (c.fl.hash, pr4);
            const auto cw = sum4 (c.fl.writes);
            fired = fired && ! (cSame == 4 && cw == 0 && c.settles == 0) && cSame == 0 && cw > 0;
            dc << (pre ? " Pre: " : " Post: ") << (4 - cSame) << "/4 differ, writes " << i64 (cw) << ";";
        }
        row (stim && ok, "FL4t PD9 feedback 100 %, sN_fb_enable Off x 4 from load, 20 s (Post and Pre): tapes == pristine, 0 writes, 0 settles",
             "meas" + d + (stim ? "" : " INVALID") + " | thr 4/4, 0, 0 | ctrl FL4tn");
        row (stim && fired, "FL4tn control: the toggle On (5 s) -> tapes differ (predicate must FAIL)",
             "meas" + dc + " | thr predicate '4/4 == pristine, 0 writes' FAILS | " + ctrl (stim && fired, "FL4tn toggle On ->" + dc));
    }

    //==========================================================================
    // FL4m / FL4mn — a routed Amount cannot lift the toggle
    void rowFL4m()
    {
        using eg25a::route;
        const int dFb1 = ogi::GiDest::stream (0, ogi::GiDest::Feedback), dFb3 = ogi::GiDest::stream (2, ogi::GiDest::Feedback);
        const P base = cat ({ eg24::flParams (20.0f, 0.0f), eg25a::modNeutral(), { { "macro1", 100.0f } },
                              route (1, eg25a::S::Mac1, dFb1, 100.0f), route (2, eg25a::S::Mac1, dFb3, 100.0f) });
        bool ok = true, stim = true, fired = true;
        juce::String d, dc;
        for (int arm = 0; arm < 2; ++arm)   // the trace reads s1's Amount, then s3's
        {
            const int dest = arm == 0 ? dFb1 : dFb3;
            const auto r = preRender (cat ({ base, all4 ("fb_enable", 0.0f) }), 10.0, 1.0f, -1, {}, dest);
            const bool st = r.traceN > 100 && r.traceMax > 0.5f && all4b (r.fbOn, false) && r.fl.outPk > 0.1;
            stim = stim && st;
            ok = ok && sum4 (r.fl.writes) == 0;
            d << " s" << (arm == 0 ? 1 : 3) << " routed Amount max " << f2 (r.traceMax, 3) << " (" << juce::String (r.traceN) << " ticks), writes " << u4 (r.fl.writes) << ";";
            const auto c = preRender (cat ({ base, all4 ("fb_enable", 1.0f) }), 10.0, 1.0f, -1, {}, dest);
            const bool f = c.traceMax > 0.5f && ! (sum4 (c.fl.writes) == 0) && c.fl.writes[0] > 0 && c.fl.writes[2] > 0
                        && c.fl.writes[1] == 0 && c.fl.writes[3] == 0;
            fired = fired && f;
            dc << " writes " << u4 (c.fl.writes) << " (routed Amount max " << f2 (c.traceMax, 3) << ");";
        }
        row (stim && ok, "FL4m PD9 toggle Off x 4 + Macro 1 at 100 % routed at depth 100 % onto s1_feedback and s3_feedback (base 0): 0 writes",
             "meas" + d + (stim ? "" : " INVALID") + " | thr 0 writes | ctrl FL4mn");
        row (stim && fired, "FL4mn control: the same routes with the toggle On -> s1 and s3 write, s2 and s4 do not (predicate must FAIL)",
             "meas" + dc + " | thr predicate '0 writes' FAILS on s1 and s3 | " + ctrl (stim && fired, "FL4mn toggle On ->" + dc));
    }

    //==========================================================================
    // FL5p / FL5pn — Pre <-> Post and toggle switches on other streams
    struct Fl5R
    {
        Hashes hash {};
        std::array<float, 4> pristinePk {}, written {}, bound {};
        Classify out, tape, chain;
        std::uint64_t writes[4] {}, bCells[4] {};
    };

    /** mode 0: no switch. 1: the switches on s2 and s4. 2: the same switches on s1. */
    Fl5R fl5pRender (int mode)
    {
        Session s (kFs, cat ({ eg24::loopParams (100.0f), all4 ("reverb_mix", 60.0f),
                               { { sn (2, "reverb_pre"), 1.0f }, { sn (4, "reverb_pre"), 1.0f } } }), kSeed, 512);
        shipping (s);
        eg24::loadLoop4 (s, 3.0, 0.5f);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        Fl5R r;
        s.run (32, 32);
        for (int k = 0; k < 4; ++k)
            if (const auto* t = s.eng().getCurrentTape (k))
                r.pristinePk[(size_t) k] = t->tape.levelPeak (0);
        const int a = mode == 2 ? 1 : 2, b = mode == 2 ? 1 : 4;
        if (mode != 0)
        {
            const float preA = mode == 2 ? 0.0f : 1.0f;   // s1 starts Post, s2 / s4 start Pre
            setAt (s, 96000,  sn (a, "reverb_pre"), 1.0f - preA);
            setAt (s, 120000, sn (b, "fb_enable"), 0.0f);
            setAt (s, 144000, sn (a, "reverb_pre"), preA);
            setAt (s, 168000, sn (b, "fb_enable"), 1.0f);
            setAt (s, 192000, sn (b, "reverb_pre"), 1.0f - preA);
            setAt (s, 216000, sn (a, "fb_enable"), 0.0f);
            setAt (s, 240000, sn (a, "fb_enable"), 1.0f);
            setAt (s, 264000, sn (b, "reverb_pre"), preA);
        }
        s.run ((juce::int64) (8.0 * kFs) - 32, 512);
        auto& h = s.hk();
        for (int k = 0; k < 4; ++k)
        {
            float fin = 0.0f;
            if (const auto* t = s.eng().getCurrentTape (k))
            {
                fin = t->tape.levelPeak (0);
                t->tape.scanForTest (r.tape.nonFinite, r.tape.subnormal);
            }
            r.written[(size_t) k] = std::max (h.head[k].maxWritten, fin);
            r.bound[(size_t) k] = std::max (r.pristinePk[(size_t) k], ogi::kAgcCeil);
            r.writes[k] = h.head[k].writes;
            r.bCells[k] = h.tankBRunCells[k];
            eg24::chainScan (s.eng().getChainForTest (k), r.chain);
        }
        for (size_t i = 0; i < s.L.size(); ++i) { classify (s.L[i], r.out); classify (s.R[i], r.out); }
        r.hash = tapeHashes (s);
        return r;
    }

    bool fl5pPredicate (const Fl5R& r, const Fl5R& ref)
    {
        return eg24::boundViolations (r.written, r.bound) == 0 && r.out.nonFinite == 0 && r.tape.nonFinite == 0 && r.chain.nonFinite == 0
            && r.hash[0] == ref.hash[0] && r.hash[2] == ref.hash[2];
    }

    void rowFL5p()
    {
        const auto ref = fl5pRender (0), r = fl5pRender (1), c = fl5pRender (2);
        const bool stim = allPos (ref.writes) && allPos (r.writes) && r.bCells[1] > 0 && r.bCells[3] > 0
                       && r.hash[1] != ref.hash[1] && r.hash[3] != ref.hash[3]          // the switches changed their own tapes
                       && r.writes[1] < ref.writes[1] && r.writes[3] < ref.writes[3];   // the toggles parked their heads
        row (stim && fl5pPredicate (r, ref),
             "FL5p PD8 / PD9 Pre on s2, s4 (fb 100 %, Mix 60), Pre <-> Post and toggle switches on them mid-render: bound holds, tapes 1 and 3 bitwise == no-switch",
             "meas bound violations " + juce::String (eg24::boundViolations (r.written, r.bound)) + ", non-finite out / tape / chain " + i64 (r.out.nonFinite) + "/"
                 + i64 (r.tape.nonFinite) + "/" + i64 (r.chain.nonFinite) + ", tape 1 " + (r.hash[0] == ref.hash[0] ? "==" : "!=") + " no-switch, tape 3 "
                 + (r.hash[2] == ref.hash[2] ? "==" : "!=") + " no-switch | stimulus: tapes 2 / 4 " + (r.hash[1] != ref.hash[1] ? "differ" : "SAME") + " / "
                 + (r.hash[3] != ref.hash[3] ? "differ" : "SAME") + ", writes " + u4 (r.writes) + " (no-switch " + u4 (ref.writes) + "), gate-B cells " + u4 (r.bCells)
                 + (stim ? "" : ", INVALID") + " | thr 0 / 0 / bitwise | ctrl FL5pn");
        const bool fired = stim && ! fl5pPredicate (c, ref) && c.hash[0] != ref.hash[0] && c.bCells[0] > 0;
        row (fired, "FL5pn control: the same switches on s1 -> tape 1 differs (predicate must FAIL)",
             "meas tape 1 " + juce::String (c.hash[0] != ref.hash[0] ? "differs" : "SAME") + ", tape 3 " + (c.hash[2] == ref.hash[2] ? "==" : "!=")
                 + " no-switch, writes " + u4 (c.writes) + ", gate-B cells " + u4 (c.bCells) + " | thr predicate 'tapes 1 and 3 == no-switch' FAILS on tape 1 | "
                 + ctrl (fired, "FL5pn s1 switched -> tape 1 differs"));
    }

    //==========================================================================
    // TG1 / TG1n — a NaN on a voice bus must not enter the loop tank
    struct TgR
    {
        bool hit = false, gateB = false;
        std::uint64_t guard = 0, lateNonFinite = 0, probeNonFinite = 0, outNonFinite = 0, wetScrubs = 0;
        std::int64_t chainScrubs = 0;
        Classify tape, chain;
    };

    TgR tg1Run (int n, bool guardOff)
    {
        const juce::int64 N = 4 * 48000, kAt = 48000;
        Session s (kFs, cat ({ onlyStream (n), { { "polyphony", 16.0f } },
                               snp (n, { { "feedback", 100.0f }, { "reverb_pre", 1.0f }, { "reverb_mix", 60.0f }, { "reverb_size", 50.0f },
                                         { "density", 50.0f }, { "size", 200.0f }, { "filter_type", 1.0f }, { "filter_cutoff", 1000.0f } }) }), kSeed, 512);
        shipping (s);
        s.hk().nanKillOff = true;
        s.hk().tankGuardOff = guardOff;
        s.hk().injectNan = { 0, kAt, 0, std::numeric_limits<float>::quiet_NaN(), false };
        s.load (n - 1, noiseBuf (2, (int) (3.0 * kFs), 0.3f, 51 + n), kFs);
        std::vector<float> tap ((size_t) (2 * N), 0.0f);
        s.hk().tapProbe = { tap.data(), N, n - 1 };
        for (int note : { 48, 52, 55, 60 })
            s.noteOn (0, note);
        s.run (kAt, 512);
        TgR r;
        r.gateB = s.eng().getTankGateBForTest (n - 1);
        s.run (N - kAt, 512);
        s.hk().tapProbe = {};
        r.hit = s.hk().injectNan.done;
        r.guard = s.hk().tankGuardScrubs;
        r.wetScrubs = s.hk().wetScrubs;
        r.chainScrubs = s.eng().getChainForTest (n - 1).scrubs;
        for (size_t i = 0; i < tap.size(); ++i)
            if (! std::isfinite (tap[i]))
            {
                ++r.probeNonFinite;
                if (i >= (size_t) (2 * (N - 48000))) ++r.lateNonFinite;   // the last second: 2 s after the injection
            }
        for (size_t i = (size_t) kAt; i < s.L.size(); ++i)
            r.outNonFinite += (std::isfinite (s.L[i]) && std::isfinite (s.R[i])) ? 0 : 1;
        if (const auto* t = s.eng().getCurrentTape (n - 1))
            t->tape.scanForTest (r.tape.nonFinite, r.tape.subnormal);
        eg24::chainScan (s.eng().getChainForTest (n - 1), r.chain);
        return r;
    }

    bool tg1Predicate (const TgR& r)
    {
        return r.guard > 0 && r.probeNonFinite == 0 && r.tape.nonFinite == 0 && r.chain.nonFinite == 0 && r.outNonFinite == 0;
    }

    void rowTG1()
    {
        bool ok = true, stim = true, fired = true;
        juce::String d, dc;
        for (int n : { 1, 3 })
        {
            const auto r = tg1Run (n, false), c = tg1Run (n, true);
            stim = stim && r.hit && r.gateB && c.hit && c.gateB;
            ok = ok && tg1Predicate (r);
            fired = fired && ! tg1Predicate (c) && c.lateNonFinite > 0 && c.guard == 0;
            d << " s" << n << ": tap-guard scrubs " << i64 (r.guard) << ", chain input non-finite " << i64 (r.probeNonFinite) << ", tape " << i64 (r.tape.nonFinite)
              << ", chain state " << i64 (r.chain.nonFinite) << ", output after the injection " << i64 (r.outNonFinite) << " (injected " << yn (r.hit)
              << ", gate B on " << yn (r.gateB) << ", chain scrubs " << juce::String (r.chainScrubs) << ", tank-A guard scrubs " << i64 (r.wetScrubs) << ");";
            dc << " s" << n << ": chain input non-finite " << i64 (c.probeNonFinite) << " (" << i64 (c.lateNonFinite) << " in the last second, 2 s after), chain scrubs "
               << juce::String (c.chainScrubs) << ", tap-guard scrubs " << i64 (c.guard) << ";";
        }
        row (stim && ok, "TG1 PD8 Pre, a NaN on one voice bus (nanKillOff): scrubbed before the loop tank; tape, chain, later output finite",
             "meas" + d + (stim ? "" : " INVALID") + " | thr > 0 scrubs, 0 non-finite | ctrl TG1n");
        row (stim && fired, "TG1n control: tankGuardOff -> the loop tank stays non-finite (predicate must FAIL)",
             "meas" + dc + " | thr predicate 'guard > 0, chain input finite' FAILS | " + ctrl (stim && fired, "TG1n no guard -> the tank is poisoned for good"));
    }

    //==========================================================================
    // RS2p / RS2pn — Reset while the head still runs on the Pre tail
    struct RsR
    {
        bool stim = false, took = false, running = false, gateB = false, written = false;
        float retPk = 0.0f;
        int voices = -1;
        juce::int64 take = -1;
        Hashes pr {}, a {}, b {}, refA {}, refB {};
        std::uint64_t wTake = 0, wA = 0, wB = 0;
    };

    RsR rs2pRun (int slot, bool holdOff)
    {
        const P p = cat ({ eg24::loopParams (100.0f), pre4(), all4 ("reverb_mix", 60.0f), all4 ("reverb_size", 100.0f) });
        constexpr juce::int64 kOff = 144000, kReset = 158400;   // notes off at 3 s, Reset 0.3 s later
        RsR r;
        auto start = [&] (Session& s)
        {
            shipping (s);
            s.record = false;
            std::vector<juce::AudioBuffer<float>> keep;
            eg24::loadLoop4 (s, 3.0, 0.5f, &keep);
            for (int k = 0; k < 4; ++k) r.pr[(size_t) k] = eg24::pristineHash (keep[(size_t) k], kFs, kFs);
            for (int n : { 48, 52, 55, 60 })
            {
                s.noteOn (0, n);
                s.noteOff (kOff, n);
            }
            s.run (kReset, 512);
        };

        Session s (kFs, p, kSeed, 512);
        s.hk().preHoldOff = holdOff;
        start (s);
        const auto& hd = s.eng().getHeadForTest (slot);
        r.running = ! hd.parked;
        r.retPk   = hd.returnPk;
        r.gateB   = s.eng().getTankGateBForTest (slot);
        r.voices  = countedVoices (s);
        r.written = tapeHashes (s)[(size_t) slot] != r.pr[(size_t) slot];
        const auto taken0 = s.hk().tapesTaken;
        s.p->resetSlotSync (slot);
        int guard = 0;
        while (s.hk().tapesTaken == taken0 && guard++ < 600)   // the head parks (20 ms), then the tick takes the tape
            s.run (32, 32);
        r.took = s.hk().tapesTaken == taken0 + 1;
        r.take = s.pos;
        r.wTake = s.hk().head[slot].writes;
        s.run (9600, 512);                 // 200 ms after the take
        r.a = tapeHashes (s);
        r.wA = s.hk().head[slot].writes;
        s.run (96000, 512);                // 2 s later
        r.b = tapeHashes (s);
        r.wB = s.hk().head[slot].writes;

        // The same render with no Reset, at the same sample positions: what
        // the other three slots hold (their heads still run on their tails).
        Session q (kFs, p, kSeed, 512);
        q.hk().preHoldOff = holdOff;
        start (q);
        q.run (r.take - kReset, 32);
        q.run (9600, 512);
        r.refA = tapeHashes (q);
        q.run (96000, 512);
        r.refB = tapeHashes (q);

        r.stim = r.took && r.running && r.retPk > kLive && r.gateB && r.voices == 0 && r.written;
        return r;
    }

    bool rs2pOthers (const RsR& r, int slot)
    {
        bool same = true;
        for (int k = 0; k < 4; ++k)
            if (k != slot)
                same = same && r.a[(size_t) k] == r.refA[(size_t) k] && r.b[(size_t) k] == r.refB[(size_t) k];
        return same;
    }
    bool rs2pPredicate (const RsR& r, int slot)
    {
        return r.a[(size_t) slot] == r.pr[(size_t) slot] && r.b[(size_t) slot] == r.pr[(size_t) slot] && rs2pOthers (r, slot);
    }

    void rowRS2p()
    {
        bool ok = true, stim = true, fired = true;
        juce::String d, dc;
        for (int slot : { 0, 2 })
        {
            const auto r = rs2pRun (slot, false), c = rs2pRun (slot, true);
            stim = stim && r.stim && c.stim;
            ok = ok && rs2pPredicate (r, slot);
            fired = fired && ! rs2pPredicate (c, slot) && (c.a[(size_t) slot] != c.pr[(size_t) slot] || c.b[(size_t) slot] != c.pr[(size_t) slot]);
            d << " s" << (slot + 1) << ": 200 ms after the take " << (r.a[(size_t) slot] == r.pr[(size_t) slot] ? "== pristine" : "!= pristine") << ", 2 s later "
              << (r.b[(size_t) slot] == r.pr[(size_t) slot] ? "== pristine" : "!= pristine") << ", the other three " << (rs2pOthers (r, slot) ? "== the no-Reset render" : "CHANGED")
              << ", writes after the take " << i64 (r.wA - r.wTake) << " / " << i64 (r.wB - r.wTake) << " (stimulus at the Reset: head running " << yn (r.running)
              << ", return peak " << juce::String (r.retPk, 5) << ", gate B " << yn (r.gateB) << ", voices " << r.voices << ", tape written " << yn (r.written)
              << ", take +" << juce::String (r.take - 158400) << " samples" << (r.stim ? "" : ", INVALID") << ");";
            dc << " s" << (slot + 1) << ": " << (c.a[(size_t) slot] == c.pr[(size_t) slot] ? "== pristine" : "!= pristine") << " / "
               << (c.b[(size_t) slot] == c.pr[(size_t) slot] ? "== pristine" : "!= pristine") << ", writes after the take " << i64 (c.wA - c.wTake) << " / " << i64 (c.wB - c.wTake) << ";";
        }
        row (stim && ok, "RS2p PD8 Pre (Mix 60, Size 100), notes off, Reset one slot while its head runs on the tail: == pristine 200 ms after the take and 2 s later",
             "meas" + d + " | thr bitwise | ctrl RS2pn");
        row (stim && fired, "RS2pn control: preHoldOff -> the old tail is written into the fresh copy (predicate must FAIL)",
             "meas" + dc + " | thr predicate '== pristine' FAILS | " + ctrl (stim && fired, "RS2pn no Reset hold ->" + dc));
    }

    //==========================================================================
    // MU1 / MU1n — a muted Pre stream stops writing
    struct MuR
    {
        bool stim = false;
        std::uint64_t w0[4] {}, wA[4] {}, wB[4] {};
        double parkMs = -1.0;   // INFO: when the muted head parked, ms after the mute
    };

    MuR mu1Run (int n, bool ignAud)
    {
        Session s (kFs, cat ({ eg24::loopParams (100.0f), pre4(), all4 ("reverb_mix", 60.0f), all4 ("reverb_size", 100.0f) }), kSeed, 512);
        shipping (s);
        s.record = false;
        s.hk().gateBIgnoreAudible = ignAud;
        eg24::loadLoop4 (s, 3.0, 0.5f);
        for (int note : { 48, 52, 55, 60 })
            s.noteOn (0, note);
        constexpr juce::int64 kMute = 96000;   // 2 s, mid-phrase (the notes stay held)
        s.run (kMute, 512);
        MuR r;
        bool live = true;
        for (int k = 0; k < 4; ++k)
        {
            r.w0[k] = s.hk().head[k].writes;
            live = live && ! s.eng().getHeadForTest (k).parked && s.eng().getTankGateBForTest (k) && r.w0[k] > 0;
        }
        setP (*s.p, sn (n, "mute"), 1.0f);
        auto step = [&] (juce::int64 samples)
        {
            for (juce::int64 done = 0; done < samples; done += 480)
            {
                s.run (480, 480);
                if (r.parkMs < 0.0 && s.eng().getHeadForTest (n - 1).parked)
                    r.parkMs = 1000.0 * (double) (s.pos - kMute) / kFs;
            }
        };
        step (24000);                          // t + 0.5 s
        for (int k = 0; k < 4; ++k) r.wA[k] = s.hk().head[k].writes;
        step (96000);                          // 2 s more
        for (int k = 0; k < 4; ++k) r.wB[k] = s.hk().head[k].writes;
        r.stim = live && countedVoices (s) == 4;
        return r;
    }

    bool mu1Predicate (const MuR& r, int n)
    {
        bool others = true;
        for (int k = 0; k < 4; ++k)
            if (k != n - 1)
                others = others && r.wB[k] > r.wA[k];
        return r.wB[n - 1] == r.wA[n - 1] && others;
    }

    void rowMU1()
    {
        bool ok = true, stim = true, fired = true;
        juce::String d, dc;
        for (int n : { 1, 3 })
        {
            const auto r = mu1Run (n, false), c = mu1Run (n, true);
            stim = stim && r.stim && c.stim;
            ok = ok && mu1Predicate (r, n);
            fired = fired && ! mu1Predicate (c, n) && c.wB[n - 1] > c.wA[n - 1];
            d << " mute s" << n << ": its writes from t + 0.5 s " << i64 (r.wA[n - 1]) << " -> " << i64 (r.wB[n - 1]) << " (+" << i64 (r.wB[n - 1] - r.wA[n - 1])
              << "), the others + " << i64 (r.wB[n % 4] - r.wA[n % 4]) << " each over 2 s; head parked " << (r.parkMs < 0.0 ? juce::String ("never") : f2 (r.parkMs, 0) + " ms")
              << " after the mute" << (r.stim ? "" : " INVALID") << ";";
            dc << " mute s" << n << ": its writes from t + 0.5 s +" << i64 (c.wB[n - 1] - c.wA[n - 1]) << " (parked "
               << (c.parkMs < 0.0 ? juce::String ("never") : f2 (c.parkMs, 0) + " ms") << ");";
        }
        row (stim && ok, "MU1 PD8 Pre, fb 100 %, Mix 60: mute one stream mid-phrase -> its head's writes are frozen from t + 0.5 s; the others keep writing",
             "meas" + d + " | thr + 0 frames | ctrl MU1n");
        row (stim && fired, "MU1n control: gateBIgnoreAudible -> the loop tank keeps the muted head writing (predicate must FAIL)",
             "meas" + dc + " | thr predicate 'writes frozen from t + 0.5 s' FAILS | " + ctrl (stim && fired, "MU1n gate B without its audible term ->" + dc));
    }

    //==========================================================================
    // IN1 / IN1n — one stream's ten new parameters move nothing of another stream
    constexpr const char* kNew10[10] { "fb_enable", "feedback", "fb_low_cut", "fb_high_cut", "fb_saturation", "fb_diffusion",
                                       "reverb_pre", "reverb_mix", "reverb_size", "reverb_damp" };
    constexpr float kIn1[4][10] {   // four streams, all different
        { 1.0f, 60.0f,  50.0f,  8000.0f, 10.0f, 20.0f, 1.0f, 30.0f, 40.0f, 30.0f },
        { 1.0f, 70.0f, 100.0f,  6000.0f, 20.0f, 40.0f, 0.0f, 40.0f, 60.0f, 45.0f },
        { 1.0f, 80.0f, 200.0f,  4000.0f, 30.0f, 60.0f, 1.0f, 50.0f, 80.0f, 70.0f },
        { 1.0f, 90.0f,  80.0f, 10000.0f, 40.0f, 80.0f, 0.0f, 60.0f, 30.0f, 20.0f } };

    float in1Moved (int stream0, int k)
    {
        const float v = kIn1[stream0][k];
        switch (k)
        {
            case 0:  return 0.0f;            // toggle Off
            case 1:  return v * 0.5f;
            case 2:  return v * 2.0f;
            case 3:  return v * 0.5f;
            case 4:  return v + 30.0f;
            case 6:  return 1.0f - v;        // Pre <-> Post
            case 7:  return v + 25.0f;
            default: return 100.0f - v;      // diffusion, size, damp
        }
    }

    struct In1Out { Hashes hash {}; std::vector<float> out; std::uint64_t writes[4] {}; double pk = 0.0; };

    /** change: the stream (1..4) whose parameters move from 2 s on (0 = none);
        solo: the stream soloed from load (0 = none); only: one parameter index,
        or -1 for all ten, 0.25 s apart (the toggle goes Off, then On again). */
    In1Out in1Render (int change, int solo, int only = -1)
    {
        P p = cat ({ eg24::loopParams (0.0f) });
        for (int n = 1; n <= 4; ++n)
            for (int k = 0; k < 10; ++k)
                p = with (p, { { sn (n, kNew10[k]), kIn1[n - 1][k] } });
        if (solo > 0)
            p = with (p, { { sn (solo, "solo"), 1.0f } });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        eg24::loadLoop4 (s, 3.0, 0.3f);
        for (int note : { 48, 52, 55, 60 })
            s.noteOn (0, note);
        if (change > 0)
        {
            juce::int64 t = 96000;
            for (int k = 0; k < 10; ++k)
            {
                if (only >= 0 && k != only)
                    continue;
                setAt (s, t, sn (change, kNew10[k]), in1Moved (change - 1, k));
                if (k == 0 && only < 0)
                    setAt (s, t + 6000, sn (change, kNew10[0]), 1.0f);
                t += 12000;
            }
        }
        s.run ((juce::int64) (5.5 * kFs), 512);
        In1Out o;
        o.hash = tapeHashes (s);
        o.out = eg22::lr (s);
        for (int k = 0; k < 4; ++k) o.writes[k] = s.hk().head[k].writes;
        for (float x : o.out) o.pk = std::max (o.pk, (double) std::abs (x));
        return o;
    }

    void rowIN1()
    {
        const auto ref = in1Render (0, 0);
        bool ok = true, stim = true, fired = true;
        juce::String d, dc;
        for (int arm = 0; arm < 2; ++arm)
        {
            const int c = arm == 0 ? 1 : 3, o = arm == 0 ? 2 : 4;   // change s1 (s3); the "other stream" is s2 (s4)
            const auto chg = in1Render (c, 0);
            const auto sRef = in1Render (0, o), sChg = in1Render (c, o);
            int tapesSame = 0;
            for (int k = 0; k < 4; ++k)
                if (k != c - 1)
                    tapesSame += chg.hash[(size_t) k] == ref.hash[(size_t) k] ? 1 : 0;
            const int soloDiff = eg21::diffCount (sRef.out, sChg.out);
            const bool own = chg.hash[(size_t) (c - 1)] != ref.hash[(size_t) (c - 1)];
            const bool st = own && allPos (ref.writes) && sRef.pk > 1.0e-3 && sRef.writes[o - 1] > 0;
            stim = stim && st;
            ok = ok && tapesSame == 3 && soloDiff == 0;                       // the predicate
            d << " change s" << c << " (10 parameters): the other three tapes " << tapesSame << "/3 == no-change, the s" << o << "-solo render " << soloDiff
              << " samples differ (stimulus: tape " << c << " " << (own ? "differs" : "SAME") << ", solo peak " << f2 (sRef.pk, 4) << (st ? "" : ", INVALID") << ");";

            // Control: each of the OTHER stream's ten parameters, one at a time.
            int live = 0;
            juce::String dead;
            for (int k = 0; k < 10; ++k)
            {
                const auto t = in1Render (o, 0, k);
                bool differs = t.hash[(size_t) (o - 1)] != ref.hash[(size_t) (o - 1)];   // "the other three tapes" fails
                if (! differs)
                    differs = eg21::diffCount (sRef.out, in1Render (o, o, k).out) != 0;   // "the solo render" fails
                live += differs ? 1 : 0;
                if (! differs) dead << " " << kNew10[k];
            }
            fired = fired && live == 10;
            dc << " s" << o << ": " << live << "/10 parameters move its tape or its solo render" << (dead.isEmpty() ? juce::String() : " (not:" + dead + ")") << ";";
        }
        row (stim && ok, "IN1 D60 four streams with different feedback + reverb values: one stream's 10 new parameters change no other tape and no other stream's solo render",
             "meas" + d + " | thr 3/3 bitwise, 0 samples | ctrl IN1n");
        row (stim && fired, "IN1n control: the other stream's own 10 parameters, one at a time (predicate must FAIL for each)",
             "meas" + dc + " | thr 10 / 10 fail 'tape and solo render unchanged' | " + ctrl (stim && fired, "IN1n" + dc));
    }

    //==========================================================================
    // SI1 / SI1n — the reverb sits before the stream sum
    struct SiOut { std::vector<float> L, R; std::uint64_t cells[4] {}; };

    /** Streams a and b enabled; a has Mix 50, b Mix 0. mode 0: both sound;
        1: b muted (a's wet render); 2: a muted (b's dry render). */
    SiOut siRender (int a, int b, int mode, bool onSum)
    {
        P p = base4 (4);
        for (int k = 1; k <= 4; ++k)
            p = with (p, { { sn (k, "enable"), (k == a || k == b) ? 1.0f : 0.0f } });
        p = cat ({ p, { { "polyphony", 8.0f } }, all4 ("density", 30.0f), all4 ("size", 150.0f), all4 ("level", -12.0f),
                   { { sn (a, "reverb_mix"), 50.0f }, { sn (a, "reverb_size"), 50.0f } } });
        if (mode == 1) p = with (p, { { sn (b, "mute"), 1.0f } });
        if (mode == 2) p = with (p, { { sn (a, "mute"), 1.0f } });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.hk().tankOnSum = onSum;
        s.load (a - 1, noiseBuf (2, (int) (3.0 * kFs), 0.2f, 71), kFs);
        s.load (b - 1, noiseBuf (2, (int) (3.0 * kFs), 0.2f, 72), kFs);
        for (int note : { 48, 55, 60 })
        {
            s.noteOn (0, note);
            s.noteOff (72000, note);
        }
        s.run (144000, 512);
        SiOut o { s.L, s.R, {} };
        for (int k = 0; k < 4; ++k) o.cells[k] = s.hk().tankRunCells[k];
        return o;
    }

    double siNullDb (const SiOut& sum, const SiOut& x, const SiOut& y)
    {
        double e = 0.0;
        const size_t n = std::min ({ sum.L.size(), x.L.size(), y.L.size() });
        for (size_t i = 0; i < n; ++i)
        {
            const double dl = (double) sum.L[i] - ((double) x.L[i] + (double) y.L[i]);
            const double dr = (double) sum.R[i] - ((double) x.R[i] + (double) y.R[i]);
            e += dl * dl + dr * dr;
        }
        e /= 2.0 * (double) std::max<size_t> (1, n);
        return e > 0.0 ? 10.0 * std::log10 (e) : -400.0;
    }
    double siRmsDb (const SiOut& x)
    {
        SiOut z;
        z.L.assign (x.L.size(), 0.0f);
        z.R.assign (x.R.size(), 0.0f);
        return siNullDb (x, z, z);
    }

    void rowSI1()
    {
        bool ok = true, stim = true, fired = true;
        juce::String d, dc;
        for (int arm = 0; arm < 2; ++arm)
        {
            const int a = arm == 0 ? 1 : 3, b = arm == 0 ? 2 : 4;
            const auto both = siRender (a, b, 0, false), wet = siRender (a, b, 1, false), dry = siRender (a, b, 2, false);
            const auto c = siRender (a, b, 0, true);
            const double null = siNullDb (both, wet, dry), cNull = siNullDb (c, wet, dry);
            const double lvWet = siRmsDb (wet), lvDry = siRmsDb (dry);
            const bool st = both.cells[a - 1] > 0 && both.cells[b - 1] == 0 && wet.cells[a - 1] > 0 && dry.cells[a - 1] == 0 && dry.cells[b - 1] == 0
                         && lvWet > -60.0 && lvDry > -60.0;
            stim = stim && st;
            ok = ok && null <= -120.0;
            fired = fired && ! (cNull <= -120.0);
            d << " s" << a << " Mix 50 + s" << b << " Mix 0: both - (s" << a << " wet + s" << b << " dry) = " << f2 (null, 1) << " dBFS (renders " << f2 (lvWet, 1) << " / "
              << f2 (lvDry, 1) << " dBFS RMS, tank-A cells s" << a << " " << i64 (both.cells[a - 1]) << ", s" << b << " " << i64 (both.cells[b - 1]) << (st ? "" : ", INVALID") << ");";
            dc << " s" << a << " + s" << b << ": " << f2 (cNull, 1) << " dBFS;";
        }
        row (stim && ok, "SI1 PD7 two streams, one with Reverb Mix 50: the output == the dry stream's render + the wet stream's render (reverb before the stream sum)",
             "meas" + d + " | thr null <= -120 dBFS | ctrl SI1n");
        row (stim && fired, "SI1n control: tankOnSum (one tank on the stream sum) -> the dry stream gets reverb too (predicate must FAIL)",
             "meas null" + dc + " | thr predicate 'null <= -120 dBFS' FAILS | " + ctrl (stim && fired, "SI1n tank on the sum ->" + dc));
    }

    //==========================================================================
    // TK1 / TK1n — tank A idles after its tail and re-enters cold
    struct TkR
    {
        bool stim = false, gateAtEnd = false;
        std::uint64_t cellsSound = 0, cellsQuiet = 0, calls = 0, ticks = 0;
        juce::int64 fall = -1;
        std::vector<float> tail;        // the output from the un-mute on (L then R)
        double tailPk = 0.0;
    };

    /** The stream sounds (notes held), is muted at 1 s, un-muted at 4 s.
        neverRan: Mix 0 until 3.5 s (the tank never ran before the re-entry). */
    TkR tkRun (int n, bool sticky, bool neverRan)
    {
        constexpr juce::int64 kMute = 48000, kMix = 168000, kBack = 192000, kEnd = 264000;
        Session s (kFs, cat ({ onlyStream (n), snp (n, { { "density", 50.0f }, { "size", 100.0f }, { "reverb_mix", neverRan ? 0.0f : 50.0f },
                                                         { "reverb_size", 0.0f } }) }), kSeed, 512);
        shipping (s);
        s.hk().tankGateSticky = sticky;
        s.load (n - 1, noiseBuf (2, (int) (3.0 * kFs), 0.3f, 61 + n), kFs);
        for (int note : { 48, 60 })
            s.noteOn (0, note);
        TkR r;
        s.run (kMute, 512);
        r.cellsSound = s.hk().tankRunCells[n - 1];
        setP (*s.p, sn (n, "mute"), 1.0f);
        while (s.pos < kMix - 50048 - 512)   // until the tank's own output peak has fallen under the threshold
        {
            s.run (512, 512);
            if (s.eng().getTailPkForTest (n - 1) <= kLive) { r.fall = s.pos; break; }
        }
        s.run (2048, 512);
        const auto c0 = s.hk().tankRunCells[n - 1];
        s.run (48000, 512);
        r.cellsQuiet = s.hk().tankRunCells[n - 1] - c0;
        r.gateAtEnd = s.eng().getTankGateForTest (n - 1);
        s.run (kMix - s.pos, 512);
        if (neverRan)
            setP (*s.p, sn (n, "reverb_mix"), 50.0f);
        s.run (kBack - s.pos, 512);
        setP (*s.p, sn (n, "mute"), 0.0f);
        s.run (kEnd - s.pos, 512);
        r.calls = s.hk().tankProcessCalls;
        r.ticks = s.hk().gridTicks;
        r.tail.assign (s.L.begin() + (std::ptrdiff_t) kBack, s.L.end());
        r.tail.insert (r.tail.end(), s.R.begin() + (std::ptrdiff_t) kBack, s.R.end());
        for (float x : r.tail) r.tailPk = std::max (r.tailPk, (double) std::abs (x));
        r.stim = r.fall > kMute && r.tailPk > 1.0e-3 && (neverRan ? r.cellsSound == 0 : r.cellsSound > 0);
        return r;
    }

    bool tk1Predicate (const TkR& r, const TkR& cold)
    {
        return r.cellsSound > 0 && r.cellsQuiet == 0 && ! r.gateAtEnd && eg21::diffCount (r.tail, cold.tail) == 0 && r.calls == 4 * r.ticks;
    }

    void rowTK1()
    {
        bool ok = true, stim = true, fired = true;
        juce::String d, dc;
        for (int n : { 1, 3 })
        {
            const auto r = tkRun (n, false, false), cold = tkRun (n, false, true), c = tkRun (n, true, false);
            const int wetDiff = eg21::diffCount (r.tail, tkRun (n, false, false).tail);   // determinism of the reference itself
            stim = stim && r.stim && cold.stim && c.stim && wetDiff == 0 && cold.cellsQuiet == 0;
            ok = ok && tk1Predicate (r, cold);
            fired = fired && ! tk1Predicate (c, cold) && c.cellsQuiet > 0;
            d << " s" << n << ": tank-A cells while it sounds " << i64 (r.cellsSound) << ", from 2048 samples after the tail fell (" << f2 ((double) (r.fall - 48000) / kFs, 3)
              << " s after the mute) over 1 s: " << i64 (r.cellsQuiet) << ", re-entry (1.5 s) vs a tank that never ran: " << eg21::diffCount (r.tail, cold.tail)
              << " samples differ (peak " << f2 (r.tailPk, 4) << "), tank process calls " << i64 (r.calls) << " = 4 x " << i64 (r.ticks) << " cells "
              << yn (r.calls == 4 * r.ticks) << (r.stim && cold.stim ? "" : " INVALID") << ";";
            dc << " s" << n << ": cells in the quiet second " << i64 (c.cellsQuiet) << ", re-entry differs on " << eg21::diffCount (c.tail, cold.tail) << " samples;";
        }
        row (stim && ok, "TK1 PD7 a stream sounds, is muted, sounds again: tank A runs while it sounds, idles after its tail, re-enters as a cold tank; all four tanks called every piece",
             "meas" + d + " | thr > 0, 0, bitwise, 4 x | ctrl TK1n");
        row (stim && fired, "TK1n control: tankGateSticky -> the run counter never returns to 0 (predicate must FAIL)",
             "meas" + dc + " | thr predicate '0 cells after the tail' FAILS | " + ctrl (stim && fired, "TK1n sticky gate ->" + dc));
    }

    //==========================================================================
    // MZ1 / MZ1n — render job N1 through the per-stream path
    //
    // T24 finding (measured): none of the 14 golden jobs has a -0 sample in its
    // output, so an inaudible stream's +0 term (PD10's "-0 + 0 = +0") cannot
    // move N1's hash — the planned control cannot fire on N1 alone. The row
    // therefore carries a second clause on which the rule IS observable: N1
    // with s1 panned hard left. The right channel is then accR * lv * 0 = a
    // SIGNED zero, and its sign must be the sign of the same sample rendered
    // at pan -99 (where the right gain is small but not 0).
    struct MzR { juce::String sha; std::uint64_t calls = 0, ticks = 0, cells = 0; std::vector<float> R; };

    MzR mzJob (bool zeroTerm, float pan = 0.0f)   // rh::jobN1 (false, kSeed) + the hook, the counters, s1_pan
    {
        const P over { { "polyphony", 4.0f }, { "grain_limit", 1.0f }, { eg21::s1 ("density"), 100.0f }, { eg21::s1 ("size"), 300.0f },
                       { eg21::s1 ("size_rand"), 50.0f }, { eg21::s1 ("scan_rate"), 100.0f }, { eg21::s1 ("scan_mode"), 1.0f },
                       { "amp_attack", 5.0f }, { "amp_release", 200.0f } };
        Session s (48000.0, pan != 0.0f ? with (rh::stim (over, false), { { eg21::s1 ("pan"), pan } }) : rh::stim (over, false), kSeed, 512);
        rh::prepareJob (s, false);
        s.hk().addZeroTerm = zeroTerm;
        s.load (0, noiseBuf (2, 3 * 48000, 0.3f, 7), 48000.0);
        eg21::n1Script (s);
        s.run (96000, 512);
        MzR r;
        r.sha = rh::sha (rh::take (s));
        r.calls = s.hk().tankProcessCalls;
        r.ticks = s.hk().gridTicks;
        for (auto c : s.hk().tankRunCells) r.cells += c;
        r.R = s.R;
        return r;
    }

    struct MzSign { std::uint64_t negZero = 0, nonZero = 0, tested = 0, mismatch = 0; };

    /** hard: the right channel at pan -100 (all zeros); ref: at pan -99. */
    MzSign mzSigns (const std::vector<float>& hard, const std::vector<float>& ref)
    {
        MzSign m;
        for (size_t i = 0; i < hard.size() && i < ref.size(); ++i)
        {
            if (hard[i] != 0.0f) { ++m.nonZero; continue; }
            m.negZero += std::signbit (hard[i]) ? 1 : 0;
            if (ref[i] == 0.0f) continue;
            ++m.tested;
            m.mismatch += std::signbit (hard[i]) == std::signbit (ref[i]) ? 0 : 1;
        }
        return m;
    }

    void rowMZ1()
    {
        const auto gold = eg24::readTable ("renders-2.4.sha256");
        const auto it = gold.find ("N1-default");
        const juce::String g = it != gold.end() ? it->second : juce::String();
        const auto r = mzJob (false), c = mzJob (true);
        const auto ref = mzJob (false, -99.0f);
        const auto m  = mzSigns (mzJob (false, -100.0f).R, ref.R);
        const auto mc = mzSigns (mzJob (true,  -100.0f).R, ref.R);
        const bool same = rh::sha (rh::jobN1 (false, kSeed)) == r.sha;                 // this header's copy is the job
        const bool stim = g.isNotEmpty() && same && r.calls > 0 && r.calls == 4 * r.ticks && r.cells == 0
                       && m.nonZero == 0 && m.tested > 10000 && m.negZero > 1000 && mc.nonZero == 0 && mc.tested == m.tested;
        auto predicate = [&g] (const MzR& job, const MzSign& sg) { return job.sha == g && sg.mismatch == 0; };
        row (stim && predicate (r, m), "MZ1 PD10 render job N1 with the per-stream path live (four tanks called, none on): hash == its renders-2.4 line; a hard-panned N1 keeps its signed zeros",
             "meas hash " + juce::String (r.sha == g ? "==" : "!=") + " renders-2.4 N1-default (" + r.sha.substring (0, 12) + "...), tank process calls " + i64 (r.calls)
                 + " = 4 x " + i64 (r.ticks) + " cells, tank-A cells " + i64 (r.cells) + ", copy == rh::jobN1 " + yn (same) + "; N1 with s1_pan -100: right channel "
                 + i64 (m.nonZero) + " non-zero samples, " + i64 (m.negZero) + " are -0, sign != the pan -99 render on " + i64 (m.mismatch) + " of " + i64 (m.tested)
                 + (stim ? "" : ", INVALID") + " | thr bitwise, 0 sign mismatches | ctrl MZ1n");
        const bool fired = stim && ! predicate (c, mc) && mc.mismatch > 0 && mc.negZero == 0;
        row (fired, "MZ1n control: addZeroTerm (an inaudible stream's +0 joins the sum: -0 + 0 = +0) -> the signed zeros are lost (predicate must FAIL)",
             "meas N1 hash " + juce::String (c.sha == g ? "== renders-2.4 (N1 has no -0 sample: the hash clause cannot see the term)" : "!= renders-2.4")
                 + "; N1 with s1_pan -100: " + i64 (mc.negZero) + " -0 samples left, sign != the pan -99 render on " + i64 (mc.mismatch) + " of " + i64 (mc.tested)
                 + " | thr predicate 'hash == golden and 0 sign mismatches' FAILS on the signs | "
                 + ctrl (fired, "MZ1n +0 term -> " + i64 (mc.mismatch) + " signed zeros flipped"));
    }

    //==========================================================================
    // FA4p / FA4pn — 0 audio-thread allocations across the R1 events
    void rowFA4p()
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

        Session s (kFs, cat ({ base4 (4), eg22::randomised(), all4 ("density", 100.0f), all4 ("size", 200.0f), all4 ("loop_end", 10.0f),
                               { { "polyphony", 8.0f }, { "amp_attack", 5.0f }, { "amp_release", 100.0f } },
                               all4 ("feedback", 100.0f), all4 ("reverb_mix", 50.0f), all4 ("fb_enable", 1.0f), pre4() }));
        shipping (s);
        s.record = false;
        eg24::loadLoop4 (s, 3.0, 0.3f);
        int risesA[4] {}, risesB[4] {}, quadA = 0, quadB = 0;
        bool wasA[4] {}, wasB[4] {};
        s.afterBlock = [&] (Session& ss)    // not armed here (the hook brackets processBlock only)
        {
            int ra = 0, rb = 0;
            for (int k = 0; k < 4; ++k)
            {
                const bool a = ss.eng().getTankGateForTest (k), b = ss.eng().getTankGateBForTest (k);
                if (a && ! wasA[k]) { ++risesA[k]; ++ra; }
                if (b && ! wasB[k]) { ++risesB[k]; ++rb; }
                wasA[k] = a;
                wasB[k] = b;
            }
            quadA += ra == 4 ? 1 : 0;
            quadB += rb == 4 ? 1 : 0;
        };
        s.arm = true;
        s.noteOn (4800, 60);
        for (int n = 40; n < 60; ++n) s.noteOn (14400, n);
        for (int n = 40; n < 60; ++n) s.noteOff (24000, n);
        for (int n = 48; n < 54; ++n) s.noteOn (28800, n);
        setAt4 (s, 19200, "reverb_pre", 0.0f);     // Pre Off x 4 ...
        setAt4 (s, 24000, "reverb_pre", 1.0f);     // ... On x 4 after the 50 ms fade: four loop tanks cleared in one cell
        setAt4 (s, 33600, "fb_enable", 0.0f);      // toggle Off x 4 (20 ms run ramp, park)
        setAt4 (s, 38400, "fb_enable", 1.0f);
        setAt4 (s, 43200, "reverb_mix", 0.0f);     // all eight tanks off ...
        setAt4 (s, 48000, "reverb_mix", 50.0f);    // ... and re-enabled in one cell
        s.at (57600, [] (Session& ss) { ss.p->resetSlotSync (-1); });   // Reset all: the 60 ms hold, four loop tanks restart
        setAt (s, 67200, sn (2, "reverb_pre"), 0.0f);
        setAt (s, 67200, sn (4, "fb_enable"), 0.0f);
        setAt (s, 72000, sn (2, "reverb_pre"), 1.0f);
        setAt (s, 72000, sn (4, "fb_enable"), 1.0f);
        for (int n = 48; n < 54; ++n) s.noteOff (86400, n);
        s.noteOff (86400, 60);
        s.run (120000, 512);
        s.arm = false;
        s.afterBlock = nullptr;
        malloc_logger = nullptr;
        allocTrace = false;

        const int n = allocCount;
        auto& h = s.hk();
        std::uint64_t parks = 0, writes = 0;
        for (const auto& c : h.head) { parks += c.parks; writes += c.writes; }
        bool rises = true;
        for (int k = 0; k < 4; ++k) rises = rises && risesA[k] >= 2 && risesB[k] >= 4;
        const bool exercised = rises && quadA >= 1 && quadB >= 3 && parks >= 8 && writes > 0 && allPos (h.tankBRunCells) && allPos (h.tankRunCells)
                            && h.tapesTaken >= 8;
        row (liveness == 1 && exercised && n == 0,
             "FA4p PERF-01 0 audio-thread allocations: Pre Off / On x 4, toggle Off / On x 4, all eight tanks re-enabled in one cell, Reset all in Pre",
             "meas " + juce::String (n) + " allocs over " + i64 ((std::uint64_t) s.blocksDone) + " blocks (loop-tank starts per stream " + juce::String (risesB[0]) + "/"
                 + juce::String (risesB[1]) + "/" + juce::String (risesB[2]) + "/" + juce::String (risesB[3]) + ", four in one block " + juce::String (quadB)
                 + " times; tank-A starts " + juce::String (risesA[0]) + "/" + juce::String (risesA[1]) + "/" + juce::String (risesA[2]) + "/" + juce::String (risesA[3])
                 + ", four in one block " + juce::String (quadA) + "; parks " + i64 (parks) + ", writes " + i64 (writes) + ", tapes taken " + i64 (h.tapesTaken) + ")"
                 + (exercised ? "" : " INVALID") + " | thr 0 | margin " + juce::String (-n) + " | ctrl FA4pn");
        const bool fired = liveness == 1 && ! (liveness == 0);
        row (fired, "FA4pn control: one armed malloc (64) on this thread is counted (predicate '0 allocations' must FAIL)",
             "meas " + juce::String (liveness) + " alloc counted | thr predicate '0 allocations' FAILS | " + ctrl (fired, "FA4pn liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (false, "FA4p alloc gate is macOS-only");
        row (false, "FA4pn alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    // CK7 / CK8 / CK9 — click gates on stream n (eg24b::ckRender's geometry:
    // 440 Hz sine tape, position 50 %, spray 100 %; local 5 ms-frame kappa)
    struct CkR
    {
        eg24b::Out out, ret;            // the output; the return probe of the stream's chain
        std::uint64_t aCells = 0, bCells = 0, parks = 0;
        double capPct = 0.0, laneDb = 0.0;   // PLAN-R2 PD10 validity (CK7): cap-limited makeup cells; the lane re pristine at the end
    };

    /** companion: stream n + 1 is enabled too, on a SILENT tape (an audible
        stream that adds nothing): the stream sum then never falls back to
        "stream 1's term is the output" when stream n leaves it (CK9). */
    CkR ckRenderN (int n, const P& extra, double seconds, const std::function<void (Session&)>& setup, bool probeRet = false, bool companion = false)
    {
        P p = cat ({ onlyStream (n), companion ? P { { sn (n + 1, "enable"), 1.0f } } : P {},
                     { { "polyphony", 16.0f }, { "grain_limit", 3.0f } },
                     snp (n, { { "density", 20.0f }, { "size", 200.0f }, { "jitter", 20.0f }, { "spray", 100.0f },
                               { "position", 50.0f }, { "loop_end", 66.0f } }),
                     extra });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.load (n - 1, sineBuf (2, (int) (3.0 * kFs), 440.0, kFs, 0.5), kFs);
        if (companion)
            s.load (n, eg24::constBuf ((int) (3.0 * kFs), 0.0f), kFs);
        const juce::int64 N = (juce::int64) (seconds * kFs);
        std::vector<float> ret;
        if (probeRet)
        {
            ret.assign ((size_t) (2 * N), 0.0f);
            s.hk().retProbe = { ret.data(), N, n - 1 };
        }
        for (int note : { 48, 55, 60 })
            s.noteOn (0, note);
        if (setup) setup (s);
        auto laneMs = [&s, n]
        {
            const auto* t = s.eng().getCurrentTape (n - 1);
            const auto& hd = s.eng().getHeadForTest (n - 1);
            double acc = 0.0;
            for (std::int64_t i = hd.lo; t != nullptr && i < hd.hi; ++i)
                for (int ch = 0; ch < 2; ++ch) { const double v = t->tape.levelSample (0, ch, i); acc += v * v; }
            return hd.hi > hd.lo ? acc / (2.0 * (double) (hd.hi - hd.lo)) : 0.0;
        };
        s.run (64, 32);
        const double ms0 = laneMs();
        s.run (N - 64, 512);
        s.hk().retProbe = {};
        CkR r;
        {
            const auto& st = s.hk().mk[n - 1];
            r.capPct = st.cells ? 100.0 * (double) st.capCells / (double) st.cells : 0.0;
            r.laneDb = 10.0 * std::log10 (std::max (laneMs(), 1e-30) / std::max (ms0, 1e-30));
        }
        r.out = { s.L, s.R };
        if (probeRet)
        {
            r.ret.L.resize ((size_t) N);
            r.ret.R.resize ((size_t) N);
            for (size_t i = 0; i < (size_t) N; ++i) { r.ret.L[i] = ret[2 * i]; r.ret.R[i] = ret[2 * i + 1]; }
        }
        r.aCells = s.hk().tankRunCells[n - 1];
        r.bCells = s.hk().tankBRunCells[n - 1];
        r.parks  = s.hk().head[n - 1].parks;
        return r;
    }

    double rmsIn (const eg24b::Out& o, double t0, double t1)
    {
        double e = 0.0;
        const size_t a = (size_t) (t0 * kFs), b = std::min (o.L.size(), (size_t) (t1 * kFs));
        for (size_t i = a; i < b; ++i) e += (double) o.L[i] * o.L[i] + (double) o.R[i] * o.R[i];
        return b > a ? std::sqrt (e / (2.0 * (double) (b - a))) : 0.0;
    }

    struct CkArm { double ratio = 0.0, hard = 0.0, ks = 0.0, kc = 0.0; bool stim = false; };

    void ckRows (const char* id, const juce::String& title, const juce::String& ctrlTitle, const char* hardName, const CkArm (&arm)[2], double t0, double t1,
                 const juce::String& stimNote)
    {
        const bool stim = arm[0].stim && arm[1].stim;
        const bool ok = arm[0].ratio <= eg24b::kRatio && arm[1].ratio <= eg24b::kRatio;                 // the predicate
        const bool fired = ! (arm[0].hard <= eg24b::kRatio) && ! (arm[1].hard <= eg24b::kRatio);
        juce::String d, dc;
        for (int k = 0; k < 2; ++k)
        {
            d << " s" << (k == 0 ? 1 : 3) << " kappa ratio " << f2 (arm[k].ratio, 3) << " (subject " << f2 (arm[k].ks, 4) << ", static " << f2 (arm[k].kc, 4) << ");";
            dc << " s" << (k == 0 ? 1 : 3) << " " << f2 (arm[k].hard, 3) << "x;";
        }
        row (stim && ok, juce::String (id) + " " + title,
             "meas" + d + " window " + f2 (t0, 2) + "-" + f2 (t1, 2) + " s | stimulus: " + stimNote + (stim ? "" : ", INVALID") + " | thr <= " + f2 (eg24b::kRatio, 1)
                 + "x | margin " + f2 (eg24b::kRatio - std::max (arm[0].ratio, arm[1].ratio), 3) + " | ctrl " + id + "n");
        row (stim && fired, juce::String (id) + "n control: " + ctrlTitle + " (predicate must FAIL)",
             "meas" + dc + " | thr predicate '<= " + f2 (eg24b::kRatio, 1) + "x' FAILS | " + ctrl (stim && fired, juce::String (id) + "n " + hardName + " ->" + dc));
    }

    void rowCK7()
    {
        CkArm arm[2];
        std::uint64_t parks[2] {};
        double capPct[2] {}, laneDb[2] {};
        for (int k = 0; k < 2; ++k)
        {
            const int n = k == 0 ? 1 : 3;
            const P fb = cat ({ eg24b::ckChain(), all4 ("feedback", 100.0f) });
            auto toggle = [n] (Session& s)
            {
                setAt (s, 48000, sn (n, "fb_enable"), 0.0f);   // Off at 1 s: the head stops mid-lap
                setAt (s, 72000, sn (n, "fb_enable"), 1.0f);   // On at 1.5 s: it resumes where it parked
            };
            const auto subj = ckRenderN (n, fb, 2.5, toggle);
            const auto stat = ckRenderN (n, fb, 2.5, {});
            const auto hard = ckRenderN (n, fb, 2.5, [&] (Session& s) { s.hk().fbOnHardStop = true; toggle (s); });
            const double kc = eg24b::kappa (stat.out, 1.0, 2.5);
            arm[k].ks = eg24b::kappa (subj.out, 1.0, 2.5);
            arm[k].kc = kc;
            arm[k].ratio = arm[k].ks / std::max (1e-30, kc);
            arm[k].hard = eg24b::kappa (hard.out, 1.0, 2.5) / std::max (1e-30, kc);
            arm[k].stim = kc > 0.0 && subj.parks > stat.parks && hard.parks > stat.parks
                       && stat.capPct > 90.0 && stat.laneDb <= -20.0;   // PLAN-R2 PD10: cap-limited, the lane erased
            parks[k] = subj.parks;
            capPct[k] = stat.capPct; laneDb[k] = stat.laneDb;
        }
        ckRows ("CK7", "QUAL-01 / PD9 feedback toggle Off at 1 s, On at 1.5 s (fb 100 %, empty chain passband, 20 ms run ramp) vs the same render untouched",
                "fbOnHardStop (the run ramp steps at the toggle edge)", "fbOnHardStop", arm, 1.0, 2.5,
                "the toggled head parked " + i64 (parks[0]) + " / " + i64 (parks[1]) + " times; static render: makeup cap-limited " + f2 (capPct[0], 1) + " / " + f2 (capPct[1], 1)
                    + " % of cells, lane " + f2 (laneDb[0], 1) + " / " + f2 (laneDb[1], 1) + " dB re pristine (stream level " + f2 (eg24b::kCkLevelDb, 0) + " dB, PLAN-R2 PD10)");
    }

    void rowCK8()
    {
        CkArm arm[2];
        juce::String info, note;
        for (int k = 0; k < 2; ++k)
        {
            const int n = k == 0 ? 1 : 3;
            const P pre = snp (n, { { "feedback", 100.0f }, { "reverb_pre", 1.0f }, { "reverb_mix", 60.0f }, { "reverb_size", 50.0f } });
            auto sw = [n] (Session& s)
            {
                setAt (s, 48000, sn (n, "reverb_pre"), 0.0f);   // Pre -> Post at 1 s (the tank fades out over 50 ms)
                setAt (s, 84000, sn (n, "reverb_pre"), 1.0f);   // Post -> Pre at 1.75 s (a cold tank fades in)
            };
            const auto subj = ckRenderN (n, pre, 3.0, sw, true);
            const auto stat = ckRenderN (n, pre, 3.0, {}, true);     // static Pre: the same tank history up to the switch
            const auto hard = ckRenderN (n, pre, 3.0, [&] (Session& s) { s.hk().preHardCut = true; sw (s); }, true);
            const double kc = eg24b::kappa (stat.ret, 1.0, 3.0);
            arm[k].ks = eg24b::kappa (subj.ret, 1.0, 3.0);
            arm[k].kc = kc;
            arm[k].ratio = arm[k].ks / std::max (1e-30, kc);
            arm[k].hard = eg24b::kappa (hard.ret, 1.0, 3.0) / std::max (1e-30, kc);
            const double lv = rmsIn (stat.ret, 1.0, 3.0);
            arm[k].stim = kc > 0.0 && lv > 1.0e-3 && subj.bCells > 0 && subj.bCells < stat.bCells;
            note << (k ? ", " : "") << "s" << n << " return RMS " << f2 (20.0 * std::log10 (std::max (1e-30, lv)), 1) << " dBFS, gate-B cells " << i64 (subj.bCells)
                 << " (static " << i64 (stat.bCells) << ")";
            const double oc = eg24b::kappa (stat.out, 1.0, 3.0);
            info << " s" << n << " " << f2 (eg24b::kappa (subj.out, 1.0, 3.0) / std::max (1e-30, oc), 3) << "x (preHardCut "
                 << f2 (eg24b::kappa (hard.out, 1.0, 3.0) / std::max (1e-30, oc), 3) << "x);";
        }
        ckRows ("CK8", "QUAL-01 / PD8 Pre -> Post at 1 s, Post -> Pre at 1.75 s (sine tape, fb 100 %, Mix 60, Size 50), on the chain's return, vs static Pre",
                "preHardCut (the loop tank is reset at the Pre -> Post edge)", "preHardCut", arm, 1.0, 3.0, note);
        std::printf ("  INFO CK8o the same Pre <-> Post switch on the OUTPUT: kappa ratio vs static Pre%s (not a gate: the output only hears the tape through later grains)\n",
                     info.toRawUTF8());
    }

    void rowCK9()
    {
        CkArm arm[2];
        juce::String note;
        for (int k = 0; k < 2; ++k)
        {
            const int n = k == 0 ? 1 : 3;
            const P rev = snp (n, { { "reverb_mix", 50.0f }, { "reverb_size", 50.0f } });
            // At a tick, 1 s in — and one cell (32 samples) off the 5 ms frame grid. The cut the control makes lands 10 ms after
            // the mute (when the audibility ramp settles). On a frame boundary it would sit between a full frame and a silent,
            // skipped one, where local kappa cannot see it; one cell inside a frame, that frame's RMS is the step itself
            // (measured: control 10.8x / 15.6x here, 2.3x / 7.1x at +96, the subject <= 1.17x at every offset tried).
            auto mute = [n] (Session& s) { setAt (s, 48032, sn (n, "mute"), 1.0f); };
            // A second, silent stream stays audible: with stream n alone, a muted stream 1 would still reach the output through
            // the "no stream joins" fallback and the control's cut would become the module's own 50 ms fade.
            const auto subj = ckRenderN (n, rev, 2.5, mute, false, true);
            const auto stat = ckRenderN (n, rev, 2.5, {}, false, true);
            const auto hard = ckRenderN (n, rev, 2.5, [&] (Session& s) { s.hk().revTailOff = true; mute (s); }, false, true);
            const double kc = eg24b::kappa (stat.out, 1.0, 2.5);
            arm[k].ks = eg24b::kappa (subj.out, 1.0, 2.5);
            arm[k].kc = kc;
            arm[k].ratio = arm[k].ks / std::max (1e-30, kc);
            arm[k].hard = eg24b::kappa (hard.out, 1.0, 2.5) / std::max (1e-30, kc);
            const double tail = rmsIn (subj.out, 1.1, 1.3), cut = rmsIn (hard.out, 1.1, 1.3);
            arm[k].stim = kc > 0.0 && tail > 1.0e-4 && subj.aCells > 0 && cut < tail;
            note << (k ? ", " : "") << "s" << n << " tail 100-300 ms after the mute " << f2 (20.0 * std::log10 (std::max (1e-30, tail)), 1) << " dBFS (revTailOff "
                 << f2 (20.0 * std::log10 (std::max (1e-30, cut)), 1) << ")";
        }
        ckRows ("CK9", "QUAL-01 / PD7 mute at a tick with the reverb on (Mix 50, Size 50, sine tape): the tail rings out, no click at the mute edge, vs unmuted",
                "revTailOff (the tank is cut when the stream stops being live)", "revTailOff", arm, 1.0, 2.5, note);
    }

    //==========================================================================
    // DC1 / DC1n — the destination class
    void rowDC1()
    {
        const auto dg = readTsv ("mod-dests.tsv");
        auto count = [&dg] (bool legacy, int& engineBad, int& tableBad, int& nV)
        {
            Session s (kFs, eg25a::baseMod (1), kSeed, 512);
            s.hk().destClassLegacy = legacy;
            engineBad = tableBad = nV = 0;
            for (int d = 0; d < (int) dg.size() && d < ogi::kGiModDests; ++d)
            {
                const bool v = dg[(size_t) d].size() > 3 && dg[(size_t) d][3] == "V";
                nV += v ? 1 : 0;
                tableBad  += ogi::GiDest::isVoiceClass (d) == v ? 0 : 1;
                engineBad += s.eng().destVoiceNowForTest (d) == v ? 0 : 1;
            }
        };
        int eBad = 0, tBad = 0, nV = 0, ceBad = 0, ctBad = 0, cV = 0;
        count (false, eBad, tBad, nV);
        count (true, ceBad, ctBad, cV);
        const bool stim = dg.size() == 150 && ogi::kGiModDests == 150 && nV == 112;
        row (stim && eBad == 0 && tBad == 0, "DC1 PD14 all 150 destinations: isVoiceClass (d) and the engine's per-voice rule == the class column of mod-dests.tsv (V)",
             "meas mismatches: GiDest::isVoiceClass " + juce::String (tBad) + ", Engine::destVoiceNow (LFOs Global) " + juce::String (eBad) + " of " + juce::String ((int) dg.size())
                 + " (class V rows " + juce::String (nV) + (stim ? "" : ", INVALID") + ") | thr 0 / 0 | ctrl DC1n");
        const bool fired = stim && ! (ceBad == 0) && ceBad == 32;
        row (fired, "DC1n control: destClassLegacy (class by position) -> 32 mismatches (predicate must FAIL)",
             "meas Engine::destVoiceNow mismatches " + juce::String (ceBad) + " (the 4 x 8 feedback / reverb destinations) | thr predicate '0 mismatches' FAILS, 32 | "
                 + ctrl (fired, "DC1n every stream destination class V -> " + juce::String (ceBad) + " mismatches"));
    }

    //==========================================================================
    // MC1 / MC1n — a per-voice source on a class-G stream destination follows
    // the newest voice (D4)
    struct McR
    {
        float v1 = 0, v2 = 0, v3 = 0;      // s2 reverb mix after notes at velocity 20, 127, 20
        float eA = 0, eB = 0, eC = 0;      // s3 feedback: before the 2nd note, the minimum after it, 0.45 s later
        float others = 0;                  // max reverb mix of s1, s3, s4 (never routed)
        std::int64_t ticks = 0;
        int voices = 0;
    };

    McR mc1Run (bool legacy)
    {
        using eg25a::route;
        const int dMix2 = eg25a::dS (2, ogi::GiDest::RevMix), dFb3 = eg25a::dS (3, ogi::GiDest::Feedback);
        Session s (kFs, cat ({ eg25a::baseMod (4), all4 ("density", 30.0f),
                               { { sn (2, "reverb_mix"), 20.0f }, { sn (3, "feedback"), 10.0f },
                                 { "menv1_attack", 300.0f }, { "menv1_decay", 0.0f }, { "menv1_sustain", 100.0f }, { "menv1_release", 500.0f } },
                               route (1, eg25a::S::Vel, dMix2, 50.0f), route (2, eg25a::S::Env1, dFb3, 80.0f) }), kSeed, 512);
        shipping (s);
        s.record = false;
        s.hk().destClassLegacy = legacy;
        eg25a::loadN (s, 4);
        std::vector<float> tr (4000, 0.0f);
        s.hk().traceDest = { dFb3, -1, tr.data(), (std::int64_t) tr.size(), 0 };
        s.noteOn (4800, 60, 20);
        s.noteOn (28800, 64, 127);
        s.noteOn (52800, 67, 20);
        McR r;
        s.run (26400, 512);
        r.v1 = s.eng().getRevMixForTest (1);
        s.run (24000, 512);
        r.v2 = s.eng().getRevMixForTest (1);
        s.run (24000, 512);
        r.v3 = s.eng().getRevMixForTest (1);
        r.others = std::max ({ s.eng().getRevMixForTest (0), s.eng().getRevMixForTest (2), s.eng().getRevMixForTest (3) });
        r.ticks = s.hk().traceDest.n;
        s.hk().traceDest = {};
        r.voices = countedVoices (s);
        auto at = [&] (double sec) { return tr[(size_t) std::min<std::int64_t> (std::max<std::int64_t> (0, r.ticks - 1), (std::int64_t) (sec * kFs / 32.0))]; };
        r.eA = at (0.59);
        r.eB = 10.0f;
        for (double t = 0.6; t < 0.75; t += 0.001) r.eB = std::min (r.eB, at (t));
        r.eC = at (1.05);
        return r;
    }

    bool mc1Predicate (const McR& r)
    {
        const bool vel = std::abs (r.v1 - 0.2f) > 0.02f && r.v2 - r.v1 > 0.1f && r.v2 - r.v3 > 0.1f;
        const bool env = r.eA - 0.1f > 0.3f && r.eA - r.eB > 0.3f && r.eC - r.eB > 0.3f;
        return vel && env;
    }

    void rowMC1()
    {
        const auto r = mc1Run (false), c = mc1Run (true);
        const bool stim = r.ticks > 2000 && r.voices == 3 && c.voices == 3 && r.others == 0.0f;
        row (stim && mc1Predicate (r), "MC1 PD14 / D4 Velocity -> s2_reverb_mix and Mod Env 1 -> s3_feedback: the stream's engine value moves and follows the newest voice",
             "meas s2 reverb mix (base 0.20) after notes at velocity 20 / 127 / 20: " + f2 (r.v1, 4) + " / " + f2 (r.v2, 4) + " / " + f2 (r.v3, 4)
                 + "; s3 feedback (base 0.10, attack 300 ms): " + f2 (r.eA, 4) + " before the 2nd note, min " + f2 (r.eB, 4) + " within 150 ms after it, "
                 + f2 (r.eC, 4) + " 0.45 s later; unrouted streams' mix " + f2 (r.others, 4) + (stim ? "" : ", INVALID")
                 + " | thr moves > 0.02, follows by > 0.1 (velocity) / > 0.3 (envelope) | ctrl MC1n");
        const bool flat = sameF (c.v1, c.v2) && sameF (c.v2, c.v3) && sameF (c.eA, c.eB) && sameF (c.eB, c.eC);
        const bool fired = stim && ! mc1Predicate (c) && flat;
        row (fired, "MC1n control: destClassLegacy -> the routes modulate nothing (predicate must FAIL)",
             "meas s2 reverb mix " + f2 (c.v1, 4) + " / " + f2 (c.v2, 4) + " / " + f2 (c.v3, 4) + "; s3 feedback " + f2 (c.eA, 4) + " / " + f2 (c.eB, 4) + " / "
                 + f2 (c.eC, 4) + " | thr predicate 'moves and follows' FAILS, no movement | " + ctrl (fired, "MC1n class by position -> both values sit at their base"));
    }

    //==========================================================================
    // UHD1 / UHD1n — the frozen (dashed) head is per lane
    void rowUHD1()
    {
        Session s (kFs, cat ({ eg25a::baseMod (4), all4 ("density", 30.0f),
                               { { sn (1, "feedback"), 0.0f }, { sn (2, "feedback"), 60.0f }, { sn (3, "feedback"), 60.0f }, { sn (4, "feedback"), 0.0f },
                                 { sn (3, "fb_enable"), 0.0f } } }), kSeed, 512);
        shipping (s);
        s.record = false;
        eg22::loadNoise4 (s, 2.0, 0.3f);
        for (int n : { 48, 60 })
            s.noteOn (64, n);
        auto& tel = s.eng().getTelemetry();
        auto parkedBit = [&tel] (int k) { return (tel.swapState[(size_t) k].load() & 4u) != 0; };
        s.run (48000, 512);
        const auto t0 = s.p->buildUiTelemetry (ug32::nowOf (s));
        const bool before = ! t0.lanes[0].hasFb && t0.lanes[1].hasFb && ! t0.lanes[2].hasFb && ! t0.lanes[3].hasFb && ! parkedBit (1);
        setP (*s.p, "freeze", 1.0f);
        s.run (24000, 512);
        int shown[4] {}, legacy[4] {}, parked[4] {};
        const bool frozen = rawOf (*s.p, "freeze") > 0.5f;
        const bool s1Rule = frozen && rawOf (*s.p, sn (1, "feedback")) > 0.0f && rawOf (*s.p, sn (1, "fb_enable")) > 0.5f;   // the pre-R1 rule: one global pair
        for (int k = 0; k < 20; ++k)
        {
            s.run (512, 512);
            const auto t = s.p->buildUiTelemetry (ug32::nowOf (s));
            for (int l = 0; l < 4; ++l)
            {
                const bool pk = parkedBit (l);
                shown[l]  += t.lanes[(size_t) l].hasFb ? 1 : 0;
                parked[l] += pk ? 1 : 0;
                legacy[l] += (! pk || s1Rule) ? 1 : 0;
            }
        }
        auto lane2Only = [] (const int (&v)[4]) { return v[1] == 20 && v[0] == 0 && v[2] == 0 && v[3] == 0; };
        const bool stim = before && frozen && parked[0] == 20 && parked[1] == 20 && parked[2] == 20 && parked[3] == 20
                       && rawOf (*s.p, sn (3, "feedback")) > 0.0f && rawOf (*s.p, sn (3, "fb_enable")) < 0.5f;
        auto v4 = [] (const int (&v)[4]) { return juce::String (v[0]) + "/" + juce::String (v[1]) + "/" + juce::String (v[2]) + "/" + juce::String (v[3]); };
        row (stim && lane2Only (shown),
             "UHD1 PD23 Freeze on, s1_feedback 0, s2_feedback 60, s3_feedback 60 with s3_fb_enable Off, s4_feedback 0: the parked (dashed) head is sent on lane 2 only",
             "meas frozen polls with a head, lanes 1-4: " + v4 (shown) + " of 20 (heads parked " + v4 (parked) + "; before the Freeze only lane 2 ran: " + yn (before) + ")"
                 + (stim ? "" : ", INVALID") + " | thr 0 / 20 / 0 / 0 | ctrl UHD1n");
        const bool fired = stim && ! lane2Only (legacy);
        row (fired, "UHD1n control: a rule that reads stream 1's feedback for every lane -> no lane shows a head (predicate must FAIL)",
             "meas frozen polls with a head under the stream-1 rule, lanes 1-4: " + v4 (legacy) + " of 20 | thr predicate 'lane 2 only' FAILS | "
                 + ctrl (fired, "UHD1n stream-1-only rule -> lane 2 shown on " + juce::String (legacy[1]) + "/20 polls"));
    }

    //==========================================================================
    // INFO lines (not rows): FL8p, PreTail
    void infoFL8p()
    {
        double worstTap = 0.0, worstTape = 0.0;
        for (float dn : { 10.0f, 75.0f, 200.0f })
            for (float sz : { 20.0f, 500.0f })
                for (float w : { 0.0f, 0.5f })
                {
                    P p = cat ({ base4 (1), { { "polyphony", 16.0f }, { "grain_limit", 3.0f } },
                                 snp (1, { { "density", dn }, { "size", sz }, { "window", w }, { "level", 6.0f }, { "feedback", 100.0f },
                                           { "reverb_pre", 1.0f }, { "reverb_mix", 100.0f }, { "reverb_size", 100.0f } }) });
                    Session s (kFs, p, kSeed, 512);
                    shipping (s);
                    s.record = false;
                    s.load (0, eg24::constBuf ((int) (6.0 * kFs), 0.5f), kFs);
                    const juce::int64 n = (juce::int64) (4.0 * kFs);
                    std::vector<float> tap ((size_t) (2 * n), 0.0f);
                    s.hk().tapProbe = { tap.data(), n, 0 };
                    s.noteOn (0, 48);
                    s.run (n, 512);
                    s.hk().tapProbe = {};
                    for (float x : tap) worstTap = std::max (worstTap, (double) std::abs (x));
                    worstTape = std::max (worstTape, (double) s.hk().head[0].maxWritten);
                }
        std::printf ("  INFO FL8p FL8 in Pre (DC 0.5 tape, fb 100 %%, Mix 100, Size 100, 12 density x size x window configs): worst chain-input peak %.4f "
                     "(FL8's Post bound is 0.5; the loop tank has gain), worst written tape peak %.4f (bound 0.7, FL1p / FL1sp)\n", worstTap, worstTape);
        std::fflush (stdout);
    }

    void infoPreTail()
    {
        Session s (kFs, cat ({ base4 (1), { { "polyphony", 16.0f } },
                               snp (1, { { "density", 50.0f }, { "size", 200.0f }, { "feedback", 100.0f }, { "reverb_pre", 1.0f },
                                         { "reverb_mix", 60.0f }, { "reverb_size", 100.0f } }) }), kSeed, 512);
        shipping (s);
        s.record = false;
        s.load (0, noiseBuf (2, (int) (3.0 * kFs), 0.5f, 51), kFs);
        constexpr juce::int64 kOff = 144000;
        for (int n : { 48, 52, 55, 60 })
        {
            s.noteOn (0, n);
            s.noteOff (kOff, n);
        }
        s.run (kOff, 512);
        double parkedAt = -1.0;
        float pk10 = -1.0f;
        while (s.pos < kOff + (juce::int64) (120.0 * kFs))
        {
            s.run (4800, 480);
            if (parkedAt < 0.0 && s.eng().getHeadForTest (0).parked)
                parkedAt = (double) (s.pos - kOff) / kFs;
            if (pk10 < 0.0f && s.pos >= kOff + 480000)
                pk10 = s.eng().getHeadForTest (0).returnPk;
            if (parkedAt >= 0.0 && pk10 >= 0.0f)
                break;
        }
        if (parkedAt >= 0.0)
            std::printf ("  INFO PreTail Pre, Size 100, Mix 60, fb 100 %%: the head keeps writing %.1f s after note-off (0.1 s steps); return peak 10 s after note-off %.6f\n",
                         parkedAt, (double) pk10);
        else
            std::printf ("  INFO PreTail Pre, Size 100, Mix 60, fb 100 %%: the head is still writing 120 s after note-off; return peak 10 s after note-off %.6f\n", (double) pk10);
        std::fflush (stdout);
    }

    //==========================================================================
    void runAll()
    {
        section ("R1  Record-in Arm auto-clear (PLAN-R1 PD31, D66)");
        if (want ("AC1") || want ("AC4") || want ("AC6")) rowsTake();
        if (want ("AC2")) rowsDiscard();
        if (want ("AC3")) rowsWait();
        if (want ("AC5")) rowsHeld();
        if (want ("AC7")) rowsState();
        if (want ("AC8")) rowAC8();

        const double t0 = juce::Time::getMillisecondCounterHiRes();
        section ("R1  per-stream feedback + reverb: the loop in Pre, the toggle (PLAN-R1 PD30)");
        if (want ("FL1p"))  rowFL1p();
        if (want ("FL1sp")) rowFL1sp();
        if (want ("FL1lit")) infoFL1lit();
        if (want ("FL2p"))  rowFL2p();
        if (want ("FL3p"))  rowFL3p();
        if (want ("FL4p"))  rowFL4p();
        if (want ("FL4t"))  rowFL4t();
        if (want ("FL4m"))  rowFL4m();
        if (want ("FL5p"))  rowFL5p();
        if (want ("FL8p"))  infoFL8p();
        if (want ("PreTail")) infoPreTail();
        if (want ("TG1"))   rowTG1();
        if (want ("RS2p"))  rowRS2p();
        if (want ("MU1"))   rowMU1();
        section ("R1  per-stream feedback + reverb: independence, the audible tanks, bit-identity (PLAN-R1 PD30)");
        if (want ("IN1"))   rowIN1();
        if (want ("SI1"))   rowSI1();
        if (want ("TK1"))   rowTK1();
        if (want ("MZ1"))   rowMZ1();
        if (want ("FA4p"))  rowFA4p();
        section ("R1  per-stream feedback + reverb: clicks (PLAN-R1 PD30; local 5 ms-frame kappa)");
        if (want ("CK7"))   rowCK7();
        if (want ("CK8"))   rowCK8();
        if (want ("CK9"))   rowCK9();
        section ("R1  destination class, the frozen head (PLAN-R1 PD30)");
        if (want ("DC1"))   rowDC1();
        if (want ("MC1"))   rowMC1();
        if (want ("UHD1"))  rowUHD1();
        std::printf ("\n-- R1 PD30 rows done (%.1f s)\n", (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
    }
} }
