/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 4 (Polish) gates (PLAN-4
    PD9 / PD10 / PD12 / PD14 / PD16). Included by main.cpp AFTER ui_gates_33.h.
    `--pg4` runs this file alone (OGI_ROWS filters); the full set runs it after
    ug33.

    Row names (PLAN-4 T1 grep: 0 prior hits each; the plan's PR* / FA3 names
    collide with parity rows PR1-12 and the 2.3 alloc row FA3, so the preset
    rows are FP* and the source-pin row is FSP1 — SUMMARY deviation 1):

      AU1    the auval default round-trip drift set over all 261 parameters
             (setValue (default) -> getValue () != default on the REAL
             parameter objects) == { lfo1_rate ... lfo4_rate } exactly
      AU1n   a skew-0.3 range at a drifting default joins the set
      USV3   D55c: a take save whose WAV writer hangs before chunk 0 (a
             non-polling hold = a hung disk syscall): destroying the processor
             takes <= 500 ms; the completion runs once (`cancelled`); after the
             hold is released the detached writer exits once within 2 s, the
             ".part" is gone and the target was never written
      USV3n  the 3.2 in-worker write (testInlineWrite): the destructor waits
             for the hold (>= 3 s, released by a helper thread) -> USV3 FAILS

    Conventions as ui_gates_33.h: every row prints "meas ... | thr ... | ctrl
    ..."; an `n` row runs its parent's predicate on the broken input and PASSES
    only when that predicate FAILS ("ctrl FIRED").

  ==============================================================================
*/
#pragma once

#include <thread>

namespace { namespace pg4
{
    using eg21::ctrl;
    using eg24b::want;
    using eg24b::section;

    //==========================================================================
    // AU1 / AU1n — auval "Parameter did not retain default value" (PD9)
    //
    // The JUCE 8.0.15 AU wrapper reports the NORMALISED default and round-trips
    // it through setValue / getValue. A parameter drifts when
    // getValue (after setValue (d)) != d. The set is frozen at the four LFO
    // rates (skew 0.3, 1 ulp at 0.5 Hz; documented, not fixed: every fix moves
    // the LOCKED spec). The PD8 snap-only ranges must not add a fifth member.
    bool drifts (juce::RangedAudioParameter& p)
    {
        const float d = p.getDefaultValue();
        const float before = p.getValue();
        p.setValue (d);
        const float back = p.getValue();
        p.setValue (before);
        return ! juce::exactlyEqual (back, d);
    }

    juce::RangedAudioParameter* findParam (juce::AudioProcessor& proc, const juce::String& id)
    {
        for (auto* q : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (q))
                if (r->getParameterID() == id)
                    return r;
        return nullptr;
    }

    juce::StringArray driftSet (juce::AudioProcessor& proc)
    {
        juce::StringArray s;
        for (auto* q : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (q))
                if (drifts (*r))
                    s.add (r->getParameterID());
        return s;
    }

    void rowsAU1()
    {
        auto p = make();
        int counted = 0;
        for (auto* q : p->getParameters())
            if (dynamic_cast<juce::RangedAudioParameter*> (q) != nullptr)
                ++counted;
        const auto set = driftSet (*p);
        const juce::StringArray expected { "lfo1_rate", "lfo2_rate", "lfo3_rate", "lfo4_rate" };
        bool exact = set.size() == expected.size();
        for (const auto& id : expected)
            exact = exact && set.contains (id);

        // The six PD8 rows: exact 0 after the round-trip AND outside the set.
        int exactZero = 0;
        for (const auto* id : { "output_gain", "rec_gain", "s1_level", "s2_level", "s3_level", "s4_level" })
            if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (findParam (*p, id)))
            {
                auto& r = static_cast<juce::RangedAudioParameter&> (*f);
                r.setValue (r.getDefaultValue());
                if (juce::exactlyEqual (f->get(), 0.0f) && ! set.contains (id))
                    ++exactZero;
            }

        row (counted == 261 && exact && exactZero == 6, "AU1 PD9 auval default round-trip drift set == { lfo1..4_rate } (261 params)",
             "meas " + juce::String (counted) + " params, set { " + set.joinIntoString (", ") + " }, PD8 rows exact 0 "
                 + juce::String (exactZero) + " / 6 | thr set == 4 LFO rates, 6 / 6 | ctrl AU1n");

        // AU1n: the same predicate with a skew-0.3 range at a drifting default.
        juce::AudioParameterFloat tmp (juce::ParameterID { "au1n_tmp", 1 }, "AU1n",
                                       juce::NormalisableRange<float> (0.01f, 20.0f, 0.0f, 0.3f), 0.5f);
        auto withTmp = set;
        if (drifts (tmp))
            withTmp.add ("au1n_tmp");
        bool exactN = withTmp.size() == expected.size();
        for (const auto& id : expected)
            exactN = exactN && withTmp.contains (id);
        row (! exactN, "AU1n control: a skew-0.3 range at default 0.5 joins the set -> FAILS (predicate must FAIL)",
             "meas set " + juce::String (withTmp.size()) + " members | thr predicate FAILS | " + ctrl (! exactN, "AU1 predicate"));
    }

    //==========================================================================
    // USV3 / USV3n — bounded close on a hung disk (PD12)
    juce::File gTmp4;

    struct Sv3Result
    {
        bool take = false, entered = false, partDuring = false, partAfter = true, target = true;
        double dtorMs = -1.0, exitMs = -1.0;
        int completions = 0, exits = 0;
        juce::String error;
    };

    Sv3Result runSv3 (bool inlineWrite, int salt)
    {
        using Proc = eg21::Proc;
        Sv3Result r;
        const auto file = gTmp4.getChildFile (inlineWrite ? "usv3n.wav" : "usv3.wav");
        const auto part = file.getSiblingFile (file.getFileName() + ".part");
        juce::WaitableEvent hold;
        Proc::writerExits.store (0);
        Proc::testWriterEntered.store (0);
        Proc::testInlineWrite.store (inlineWrite);
        Proc::testHoldWrite.store (&hold);

        struct Box { std::atomic<int> n { 0 }; juce::String error; };
        auto box = std::make_shared<Box>();
        auto s = std::make_unique<eg21::Session> (48000.0, eg26b::stBase(), eg21::kSeed, 512);
        s->record = false;
        r.take = ug32::commitTake (*s, 0, 96000, 0.4f, salt);
        s->p->saveRecordingTo (0, file, [box] (const Proc::SaveResult& res)
        {
            box->error = res.error;
            box->n.fetch_add (1);
        });
        // Audio runs until the snapshot is copied and the writer reaches the hold.
        for (int waited = 0; Proc::testWriterEntered.load() == 0 && waited < 10000; waited += 2)
        {
            s->run (512, 512);
            juce::Thread::sleep (2);
        }
        r.entered = Proc::testWriterEntered.load() == 1;
        r.partDuring = part.existsAsFile();

        // The control's release comes from a helper thread after 3 s (the
        // destructor blocks the calling thread on the in-worker write).
        std::unique_ptr<std::thread> helper;
        if (inlineWrite)
            helper = std::make_unique<std::thread> ([&hold] { juce::Thread::sleep (3000); hold.signal(); });

        const double t0 = juce::Time::getMillisecondCounterHiRes();
        s.reset();   // the processor destructor (save worker joined first)
        r.dtorMs = juce::Time::getMillisecondCounterHiRes() - t0;
        r.completions = box->n.load();
        r.error = box->error;

        if (helper != nullptr)
            helper->join();
        else
            hold.signal();
        const double t1 = juce::Time::getMillisecondCounterHiRes();
        while (! inlineWrite && Proc::writerExits.load() == 0 && juce::Time::getMillisecondCounterHiRes() - t1 < 2000.0)
            juce::Thread::sleep (1);
        r.exitMs = juce::Time::getMillisecondCounterHiRes() - t1;
        juce::Thread::sleep (50);   // a second (wrong) exit would land here
        r.exits = Proc::writerExits.load();
        r.partAfter = part.exists();
        r.target = file.exists();

        Proc::testHoldWrite.store (nullptr);
        Proc::testInlineWrite.store (false);
        return r;
    }

    bool sv3Pred (const Sv3Result& r)
    {
        return r.take && r.entered && r.partDuring && r.dtorMs <= 500.0 && r.completions == 1 && r.error == "cancelled"
            && r.exits == 1 && r.exitMs < 2000.0 && ! r.partAfter && ! r.target;
    }

    void rowsSV3()
    {
        const auto yn = [] (bool b) { return juce::String (b ? "yes" : "no"); };
        const auto a = runSv3 (false, 401);
        row (sv3Pred (a), "USV3 PD12 D55c bounded close on a hung WAV write (detached writer, abandon, .part removed)",
             "meas take " + yn (a.take) + ", writer held " + yn (a.entered) + " (.part " + yn (a.partDuring) + "), dtor "
                 + juce::String (a.dtorMs, 1) + " ms, completions " + juce::String (a.completions) + " '" + a.error + "', writer exits "
                 + juce::String (a.exits) + " (+" + juce::String (a.exitMs, 1) + " ms after release), .part after " + yn (a.partAfter)
                 + ", target " + yn (a.target) + " | thr dtor <= 500 ms, 1 completion 'cancelled', 1 exit < 2 s, no .part | ctrl USV3n");

        const auto b = runSv3 (true, 402);
        const bool fired = ! sv3Pred (b) && b.entered && b.dtorMs >= 3000.0;
        row (fired, "USV3n control: the 3.2 in-worker write -> the destructor waits for the 3 s hold (predicate must FAIL)",
             "meas writer held " + yn (b.entered) + ", dtor " + juce::String (b.dtorMs, 1) + " ms, completions "
                 + juce::String (b.completions) + " '" + b.error + "' | thr dtor >= 3000 ms, USV3 FAILS | " + ctrl (fired, "USV3 predicate"));
    }

    //==========================================================================
    void runAll()
    {
        gTmp4 = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ogi-pg4-" + juce::String (juce::Time::currentTimeMillis()));
        gTmp4.createDirectory();
        struct Guard { ~Guard() { gTmp4.deleteRecursively(); } } guard;

        section ("4  polish: auval drift set, presets, factory sources, D55 (PLAN-4)");
        if (want ("AU1"))  rowsAU1();
        if (want ("USV3")) rowsSV3();
    }
} }
