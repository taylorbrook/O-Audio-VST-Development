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
    void runAll()
    {
        section ("R1  Record-in Arm auto-clear (PLAN-R1 PD31, D66)");
        if (want ("AC1") || want ("AC4") || want ("AC6")) rowsTake();
        if (want ("AC2")) rowsDiscard();
        if (want ("AC3")) rowsWait();
        if (want ("AC5")) rowsHeld();
        if (want ("AC7")) rowsState();
        if (want ("AC8")) rowAC8();
    }
} }
