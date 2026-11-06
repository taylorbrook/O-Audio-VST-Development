/*
  ==============================================================================

    O-Granulator — render harness, Stage 2 / Phase 2.5a units + tempo rows
    (PLAN-2.5a P58). Sibling of engine_gates_24b.h; engine_gates_25ab.h holds
    the matrix / PERF rows.

      TT0  MockPlayHead v2 self-test (tempo map, loop, locate, pre-roll, time
           signature, block splitting at transport events)              (T2)
      U9a  kTempoDivBeats vs tempoDivisions (22) + the 20/19/22 prefixes (T3)
      TT1–TT5  TempoTracker: held anchor, R42 rules, trapezoid, R44 bars,
           pre-roll                                                     (T4)
      TS1–TS6  Tempo clock (FUNC-04a/b)                                  (T5)
      LS / EV / MC  LFO, envelope follower, macros                      (T6)
      U9b–g  matrix units                                               (T7)

  ==============================================================================
*/
#pragma once

#include <mutex>

namespace { namespace eg25
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg22::sp;
    constexpr double kFs = 48000.0;

    //==========================================================================
    // Transport plumbing (P47). Each processor gets its OWN MockPlayHead (jobs
    // render in parallel); `after` moves it to the next block start. The
    // transport's segment starts are added as no-op events, so every block-size
    // variant cuts its host blocks there (eg::render splits at events).
    //==========================================================================
    std::mutex headsLock;
    std::vector<std::shared_ptr<MockPlayHead>> heads;   // kept alive for the whole run

    void withTransport (Job& j, const Transport& tr)
    {
        for (auto t : tr.eventSamples())
            j.events.push_back ({ t, [] (Proc&) {} });
        auto pre = j.pre;
        j.pre = [tr, pre] (Proc& p)
        {
            if (pre) pre (p);
            auto h = tr.makeHead();
            {
                const std::lock_guard<std::mutex> g (headsLock);
                heads.push_back (h);
            }
            p.setPlayHead (h.get());
        };
        auto after = j.after;
        j.after = [after] (Proc& p, juce::int64 pos)
        {
            if (auto* h = dynamic_cast<MockPlayHead*> (p.getPlayHead()))
                h->timeInSamples = pos;
            if (after) after (p, pos);
        };
    }

    // Parallel renders WITH events: processors built / prepared on this thread,
    // the block loop (eg::render's, incl. the seeded variable schedule and the
    // event splits) on workers. Event lambdas touch only their own processor.
    Out setup (const Job& j) { return eg22::setupOut (j); }

    void loop (const Job& j, Out& o)
    {
        const int numCh = juce::jmax (j.inCh, j.outCh);
        o.L.assign ((size_t) j.len, 0.0f);
        o.R.assign ((size_t) j.len, 0.0f);
        juce::AudioBuffer<float> blk (numCh, 8192);
        juce::MidiBuffer midi;
        juce::Random vr (0xB10CC5EEDull);
        std::vector<eg::Event> ev = j.events;
        std::sort (ev.begin(), ev.end(), [] (const eg::Event& a, const eg::Event& b) { return a.at < b.at; });
        size_t ei = 0;
        int varStep = 0;
        juce::int64 pos = 0;
        while (pos < j.len)
        {
            while (ei < ev.size() && ev[ei].at <= pos) ev[ei++].fn (*o.p);
            int b = j.block;
            if (b == 0)
            {
                const int k = varStep++;
                b = k == 0 ? 1 : k == 1 ? 8192 : (k % 17 == 0 ? 8192 : (k % 11 == 0 ? 1 : 1 + vr.nextInt (2048)));
            }
            juce::int64 n = juce::jmin ((juce::int64) b, j.len - pos);
            if (ei < ev.size()) n = juce::jmin (n, ev[ei].at - pos);
            const int ni = (int) n;
            blk.setSize (numCh, ni, false, false, true);
            for (int ch = 0; ch < numCh; ++ch)
                for (int i = 0; i < ni; ++i)
                    blk.setSample (ch, i, ch < j.inCh ? j.stim (pos + i, ch) : 0.0f);
            o.p->processBlock (blk, midi);
            const float* l0 = blk.getReadPointer (0);
            const float* r0 = blk.getReadPointer (j.outCh > 1 ? 1 : 0);
            std::copy (l0, l0 + ni, o.L.begin() + pos);
            std::copy (r0, r0 + ni, o.R.begin() + pos);
            pos += n;
            if (j.after) j.after (*o.p, pos);
        }
    }

    std::vector<Out> renderAll (const std::vector<Job>& jobs)
    {
        std::vector<Out> outs;
        const size_t hw = juce::jmax (1u, std::thread::hardware_concurrency());
        for (size_t b0 = 0; b0 < jobs.size(); b0 += hw)
        {
            const size_t b1 = juce::jmin (jobs.size(), b0 + hw);
            const size_t first = outs.size();
            for (size_t i = b0; i < b1; ++i) outs.push_back (setup (jobs[i]));
            std::vector<std::thread> th;
            for (size_t i = b0; i < b1; ++i)
                th.emplace_back ([&jobs, &outs, i, b0, first] { loop (jobs[i], outs[first + i - b0]); });
            for (auto& t : th) t.join();
        }
        return outs;
    }

    juce::String blockName (int b) { return b == 0 ? juce::String ("var") : juce::String (b); }

    //==========================================================================
    // TT0 — mock v2 self-test (T2)
    //==========================================================================
    void mockRows()
    {
        std::printf ("\n-- TT0. MockPlayHead v2 self-test (P47)\n");
        const double fs = kFs;

        // Tempo step 120 → 90 at a split: ppq continuous, then 1.5 beats / s.
        {
            const juce::int64 t1 = 2 * 48000 + 123;
            Transport tr (fs, 120.0);
            tr.tempo (t1, 90.0);
            const double before = (double) t1 / fs * 2.0;     // the 120 bpm formula at t1
            const double at = tr.ppqAt (t1), later = tr.ppqAt (t1 + 48000);
            const bool ok = std::abs (at - before) <= 1e-12 && std::abs ((later - at) - 1.5) <= 1e-12
                            && std::abs (tr.ppqAt (t1 - 1) - (double) (t1 - 1) / fs * 2.0) <= 1e-12;
            row (ok, "TT0 tempo map: 120 -> 90 step, ppq continuous, integrated (1e-12)",
                 juce::String::formatted ("ppq(t1) %.15g vs %.15g, +1 s %.15g", at, before, later - at));
        }
        // Loop wrap, locate, negative start.
        {
            Transport tr (fs, 120.0);
            tr.loopWrap (96000, 1.0, 4.0).locate (150000, 17.25);
            auto h = tr.makeHead();
            h->timeInSamples = 96000;
            const auto pi = *h->getPosition();
            const auto lp = pi.getLoopPoints();
            const bool loopOk = tr.ppqAt (95999) > 3.99 && juce::exactlyEqual (tr.ppqAt (96000), 1.0) && pi.getIsLooping()
                                && lp.hasValue() && juce::exactlyEqual (lp->ppqStart, 1.0) && juce::exactlyEqual (lp->ppqEnd, 4.0);
            row (loopOk, "TT0 loop [1, 4) wraps to 1 at the event sample; isLooping + loop points",
                 juce::String::formatted ("ppq %.6f -> %.6f", tr.ppqAt (95999), tr.ppqAt (96000)));
            row (juce::exactlyEqual (tr.ppqAt (150000), 17.25) && std::abs (tr.ppqAt (150000 + 24000) - 18.25) <= 1e-12,
                 "TT0 locate jumps to 17.25 and runs on");

            Transport neg (fs, 120.0, -4.0);
            row (juce::exactlyEqual (neg.ppqAt (0), -4.0) && juce::exactlyEqual (neg.lastBarStartAt (0), -4.0)
                 && juce::exactlyEqual (neg.lastBarStartAt (24000), -4.0) && juce::exactlyEqual (neg.lastBarStartAt (96000), 0.0)
                 && juce::exactlyEqual (neg.ppqAt (96000), 0.0),
                 "TT0 negative start ppq -4: bars at -4 and 0");
        }
        // 3/4 and 7/8 ppqPositionOfLastBarStart across a time-sig change.
        {
            Transport tr (fs, 120.0, 0.0, true, 3, 4);
            tr.timeSig (144000, 7, 8);                  // ppq 6 (two 3/4 bars), a 7/8 bar starts there
            auto h = tr.makeHead();
            auto at = [&h] (juce::int64 t) { h->timeInSamples = t; return *h->getPosition(); };
            const auto a = at (108000), b = at (144000 + 4 * 24000 + 12000);   // ppq 4.5; ppq 10.5
            const bool ok = juce::exactlyEqual (*a.getPpqPositionOfLastBarStart(), 3.0) && a.getTimeSignature()->numerator == 3
                            && juce::exactlyEqual (*b.getPpqPositionOfLastBarStart(), 9.5) && b.getTimeSignature()->numerator == 7
                            && b.getTimeSignature()->denominator == 8;
            row (ok, "TT0 3/4 then 7/8 (bar 3.5 q): lastBarStart 3 at ppq 4.5, 9.5 at ppq 10.5",
                 juce::String::formatted ("%.6f / %.6f", *a.getPpqPositionOfLastBarStart(), *b.getPpqPositionOfLastBarStart()));
        }
        // The block splitter: every transport event sample is a block start in every variant.
        {
            Transport tr (fs, 120.0);
            tr.tempo (30001, 97.3).loopWrap (61234, 2.0, 6.0).locate (77777, -1.5).timeSig (90001, 3, 4).stop (95000).start (99999);
            const auto evs = tr.eventSamples();
            Tally t;
            std::vector<Job> jobs;
            const int blocks[] { 32, 441, 512, 4096, 0 };
            std::vector<std::shared_ptr<std::vector<juce::int64>>> starts;
            for (int b : blocks)
            {
                Job j;
                j.len = 120000;
                j.block = b;
                j.params = { { "mix", 0.0f } };
                auto s = std::make_shared<std::vector<juce::int64>> (1, 0);
                starts.push_back (s);
                j.after = [s] (Proc&, juce::int64 pos) { s->push_back (pos); };
                withTransport (j, tr);
                jobs.push_back (j);
            }
            renderAll (jobs);
            for (size_t k = 0; k < starts.size(); ++k)
                for (auto e : evs)
                    t.check (std::find (starts[k]->begin(), starts[k]->end(), e) != starts[k]->end(),
                             blockName (blocks[k]) + " misses " + juce::String (e));
            row (t.bad == 0, "TT0 block splitter: 6 events land on block starts for 32/441/512/4096/var",
                 t.detail ((int) (evs.size() * starts.size())));
        }
    }

    //==========================================================================
    // U9a — the division table (T3)
    //==========================================================================
    void divisionRows()
    {
        std::printf ("\n-- U9a. tempo division table vs the parameter strings\n");
        const auto all = GranulatorParamIds::tempoDivisions (22);
        Tally t;
        for (int i = 0; i < 22; ++i)
        {
            const auto s = all[i];
            double beats = 0.0; int bars = 0;
            if (s.contains ("bar"))
            {
                bars = s.upToFirstOccurrenceOf (" ", false, false).getIntValue();
                beats = 4.0 * bars;
            }
            else
            {
                const int den = s.fromFirstOccurrenceOf ("/", false, false).upToFirstOccurrenceOf (" ", false, false).getIntValue();
                beats = 4.0 / (double) den;
                if (s.endsWith (" T")) beats = beats * 2.0 / 3.0;
                if (s.endsWith (" D")) beats = beats * 1.5;
            }
            t.check (std::abs (ogran::kTempoDivBeats[i] - beats) <= 1e-15 * beats && ogran::kTempoDivBars[i] == bars,
                     s + juce::String::formatted (" %.17g vs %.17g", ogran::kTempoDivBeats[i], beats));
            if (bars > 0)
                t.check (juce::exactlyEqual (ogran::tempoDivBeats (i, 3.0), 3.0 * bars), s + " under 3/4");
        }
        bool prefixes = true;
        for (int i = 0; i < 22; ++i)
        {
            if (i < 20) prefixes = prefixes && GranulatorParamIds::sizeSyncs()[i] == all[i];
            if (i < 19) prefixes = prefixes && GranulatorParamIds::densitySyncs()[i] == all[i];
            prefixes = prefixes && GranulatorParamIds::lfoRateSyncs()[i] == all[i];
        }
        prefixes = prefixes && GranulatorParamIds::sizeSyncs().size() == 20 && GranulatorParamIds::densitySyncs().size() == 19
                   && GranulatorParamIds::lfoRateSyncs().size() == 22;
        row (t.bad == 0, "U9a kTempoDivBeats / kTempoDivBars == the 22 parsed strings; bars x barLen", t.detail (22 + 4));
        row (prefixes, "U9a size_sync / density_sync / rate_sync = first 20 / 19 / 22");
    }

    //==========================================================================
    // TT1–TT5 — TempoTracker units (T4). The tracker is driven directly from
    // the v2 mock, block by block, exactly as the processor does (one report
    // per host block, converted by the processor's rules).
    //==========================================================================
    ogran::TransportSnapshot snapOf (const MockPlayHead& h)
    {
        ogran::TransportSnapshot ts;
        if (const auto pos = h.getPosition())
        {
            const auto bpm = pos->getBpm();
            const auto ppq = pos->getPpqPosition();
            ts.valid   = bpm.hasValue() && ppq.hasValue();
            ts.playing = pos->getIsPlaying();
            ts.bpm     = bpm.orFallback (0.0);
            ts.ppq     = ppq.orFallback (0.0);
            if (const auto bar = pos->getPpqPositionOfLastBarStart()) { ts.hasLastBar = true; ts.lastBarPpq = *bar; }
            if (const auto sig = pos->getTimeSignature()) { ts.num = sig->numerator; ts.den = sig->denominator; }
        }
        return ts;
    }

    /** Host block starts as eg::render / loop() cut them (incl. the seeded variable
        schedule and the event splits). */
    std::vector<juce::int64> blockStarts (juce::int64 len, int block, const std::vector<juce::int64>& events)
    {
        std::vector<juce::int64> v;
        juce::Random vr (0xB10CC5EEDull);
        int varStep = 0;
        size_t ei = 0;
        juce::int64 pos = 0;
        while (pos < len)
        {
            while (ei < events.size() && events[ei] <= pos) ++ei;
            v.push_back (pos);
            int b = block;
            if (b == 0)
            {
                const int k = varStep++;
                b = k == 0 ? 1 : k == 1 ? 8192 : (k % 17 == 0 ? 8192 : (k % 11 == 0 ? 1 : 1 + vr.nextInt (2048)));
            }
            juce::int64 n = juce::jmin ((juce::int64) b, len - pos);
            if (ei < events.size()) n = juce::jmin (n, events[ei] - pos);
            pos += n;
        }
        return v;
    }

    struct TrackRun
    {
        std::vector<double> cellPpq;          // ppq at every absolute cell start
        std::vector<double> barOrigin, barLen; // anchor bar state at every cell start
        std::int64_t reanchors = 0, relocks = 0;
        std::uint64_t relockEpoch = 0;
        bool playingAtEnd = false;
    };

    TrackRun runTracker (const Transport& tr, juce::int64 len, int block, bool perBlockOrigin = false,
                         const std::function<ogran::TransportSnapshot (juce::int64, const ogran::TransportSnapshot&)>& edit = {},
                         const std::vector<juce::int64>& extraSplits = {})
    {
        TrackRun r;
        auto head = tr.makeHead();
        ogran::TempoTracker t;
        t.prepare (tr.fs);
        t.perBlockOrigin = perBlockOrigin;
        auto ev = tr.eventSamples();
        for (auto e : extraSplits) ev.push_back (e);
        std::sort (ev.begin(), ev.end());
        const auto starts = blockStarts (len, block, ev);
        for (size_t k = 0; k < starts.size(); ++k)
        {
            const juce::int64 h = starts[k], e = k + 1 < starts.size() ? starts[k + 1] : len;
            head->timeInSamples = h;
            auto s = snapOf (*head);
            if (edit) s = edit (h, s);
            t.update (s, h);
            for (juce::int64 c = ((h + 31) / 32) * 32; c < e; c += 32)
            {
                r.cellPpq.push_back (t.ppqAt (c));
                r.barOrigin.push_back (t.anchor().barOrigin);
                r.barLen.push_back (t.anchor().barLen);
            }
        }
        r.reanchors = t.getReanchors();
        r.relocks = t.getRelocks();
        r.relockEpoch = t.getRelockEpoch();
        r.playingAtEnd = t.isPlaying();
        return r;
    }

    int countDiffs (const std::vector<double>& a, const std::vector<double>& b)
    {
        int n = (int) std::abs ((long long) a.size() - (long long) b.size());
        for (size_t i = 0; i < a.size() && i < b.size(); ++i)
            if (std::memcmp (&a[i], &b[i], sizeof (double)) != 0) ++n;
        return n;
    }

    void trackerRows()
    {
        std::printf ("\n-- TT. TempoTracker (held anchor R39, R42 rules, R44 bars, pre-roll)\n");
        const int blocks[] { 441, 512, 4096, 0 };
        constexpr juce::int64 kCells = 20000, kLen = kCells * 32;

        // TT1 / TT1n — grid ppq bitwise across block sizes.
        {
            Tally t;
            double worstN = 1.0;
            for (double bpm : { 120.0, 133.0, 97.3 })
            {
                Transport tr (kFs, bpm);
                const auto ref = runTracker (tr, kLen, 32);
                for (int b : blocks)
                {
                    const auto o = runTracker (tr, kLen, b);
                    t.check (countDiffs (ref.cellPpq, o.cellPpq) == 0 && o.reanchors == 0,
                             juce::String (bpm) + " bpm / " + blockName (b) + juce::String::formatted (": %d diffs", countDiffs (ref.cellPpq, o.cellPpq)));
                    const auto n = runTracker (tr, kLen, b, true);
                    const auto nr = runTracker (tr, kLen, 32, true);
                    worstN = juce::jmin (worstN, (double) countDiffs (nr.cellPpq, n.cellPpq) / (double) n.cellPpq.size());
                }
            }
            row (t.bad == 0, "TT1 held anchor: 20 k grid ppq bitwise vs block 32 (120/133/97.3 bpm x 441/512/4096/var), 0 re-anchors",
                 t.detail (12));
            row (worstN >= 0.01, "TT1n perBlockOrigin: >= 1 % of grid ppq differ (R39: 30-52 %)",
                 juce::String::formatted ("min fraction %.3f", worstN));
        }

        // TT2 — R42 rules.
        {
            const juce::int64 s60 = 60 * 48000;
            const auto c = runTracker (Transport (kFs, 120.0), s60, 512);
            row (c.reanchors == 0 && c.relocks == 0, "TT2 constant tempo 60 s: 0 re-anchors",
                 juce::String::formatted ("%lld / %lld", (long long) c.reanchors, (long long) c.relocks));

            Tally t;
            for (int b : { 32, 441, 512, 4096, 0 })
            {
                Transport st (kFs, 120.0); st.tempo (100003, 97.3);
                const auto o = runTracker (st, 300000, b);
                t.check (o.reanchors == 1 && o.relocks == 0, "step " + blockName (b) + juce::String::formatted (" %lld/%lld", (long long) o.reanchors, (long long) o.relocks));
                Transport lp (kFs, 120.0); lp.loopWrap (100003, 2.0, 6.0);
                const auto l = runTracker (lp, 300000, b);
                t.check (l.reanchors == 1 && l.relocks == 1, "loop " + blockName (b) + juce::String::formatted (" %lld/%lld", (long long) l.reanchors, (long long) l.relocks));
                Transport lc (kFs, 120.0); lc.locate (100003, 33.5);
                const auto m = runTracker (lc, 300000, b);
                t.check (m.reanchors == 1 && m.relocks == 1, "locate " + blockName (b) + juce::String::formatted (" %lld/%lld", (long long) m.reanchors, (long long) m.relocks));
                Transport sp0 (kFs, 120.0, 0.0, false); sp0.start (100003);
                const auto s = runTracker (sp0, 300000, b);
                t.check (s.reanchors == 1 && s.relocks == 1 && s.playingAtEnd, "start " + blockName (b) + juce::String::formatted (" %lld/%lld", (long long) s.reanchors, (long long) s.relocks));
            }
            row (t.bad == 0, "TT2 tempo step: 1 re-anchor / 0 re-lock; loop, locate, transport start: 1 / 1 (5 block variants)", t.detail (20));

            // Stopped host parked at ppq 0 for 10 s: host ppq ignored, virtual ppq at host bpm.
            const auto p = runTracker (Transport (kFs, 120.0, 0.0, false), 10 * 48000, 512);
            const double end = p.cellPpq.back(), expect = (double) ((10 * 48000 - 1) / 32 * 32) / kFs * 2.0;
            row (std::abs (end - expect) <= 1e-9 && p.reanchors == 0 && ! p.playingAtEnd,
                 "TT2 stopped at ppq 0 for 10 s: virtual ppq runs at 120 bpm, no re-anchor", juce::String::formatted ("%.9f vs %.9f", end, expect));

            // No playhead / bpm <= 0 / no ppq → 120 bpm virtual (WR-09).
            Tally t2;
            for (int mode = 0; mode < 3; ++mode)
            {
                Transport tr (kFs, 90.0, 7.0, true);
                if (mode == 1) tr.noInfo (true, false);
                auto edit = [mode] (juce::int64, const ogran::TransportSnapshot& s)
                {
                    auto x = s;
                    if (mode == 0) x = ogran::TransportSnapshot {};   // no playhead
                    if (mode == 2) x.bpm = 0.0;                        // bpm <= 0
                    return x;
                };
                const auto o = runTracker (tr, 48000, 512, false, edit);
                const double q = o.cellPpq.back(), want = (double) ((48000 - 1) / 32 * 32) / kFs * 2.0;
                t2.check (std::abs (q - want) <= 1e-9 && ! o.playingAtEnd && o.reanchors == 0,
                          juce::String (mode) + juce::String::formatted (" ppq %.6f", q));
            }
            row (t2.bad == 0, "TT2 no playhead / no ppq / bpm 0: 120 bpm virtual from ppq 0, not playing", t2.detail (3));
        }

        // TT3 — trapezoid: a per-block linear ramp 60 → 180 bpm over 2 s, 4096 blocks.
        {
            const double fs = kFs, T = 2.0 * fs;
            auto bpmAt = [=] (double t) { return t < T ? 60.0 + 120.0 * t / T : 180.0; };
            auto ppqAt = [=] (double t)   // ∫ bpm/60/fs dt, exact
            {
                const double tt = juce::jmin (t, T);
                double q = (60.0 * tt + 60.0 * tt * tt / T) / 60.0 / fs;
                if (t > T) q += (t - T) * 180.0 / 60.0 / fs;
                return q;
            };
            auto edit = [=] (juce::int64 h, const ogran::TransportSnapshot& s)
            {
                auto x = s;
                x.bpm = bpmAt ((double) h);
                x.ppq = ppqAt ((double) h);
                x.hasLastBar = false;
                return x;
            };
            const auto o = runTracker (Transport (kFs, 60.0), (juce::int64) (3.0 * fs), 4096, false, edit);
            // INFO: the constant-old-pps predictor's worst miss on the same ramp.
            double worstStep = 0.0;
            for (juce::int64 h = 4096; h < (juce::int64) T; h += 4096)
            {
                const double pred = ppqAt ((double) (h - 4096)) + 4096.0 * bpmAt ((double) (h - 4096)) / 60.0 / fs;
                worstStep = juce::jmax (worstStep, std::abs (ppqAt ((double) h) - pred) / (bpmAt ((double) h) / 60.0 / fs));
            }
            row (o.relocks == 0 && o.reanchors > 0, "TT3 60 -> 180 bpm ramp / 2 s, 4096 blocks: 0 re-locks (trapezoid)",
                 juce::String::formatted ("re-anchors %lld, re-locks %lld", (long long) o.reanchors, (long long) o.relocks));
            std::printf ("  [INFO] TT3 constant-old-pps predictor worst miss %.1f samples (R42: 161) -> it would re-lock\n", worstStep);
        }

        // TT4 — R44 bars.
        {
            Tally t;
            auto check = [&t] (const TrackRun& r, size_t cell, double origin, double len, const juce::String& what)
            {
                t.check (cell < r.barOrigin.size() && std::abs (r.barOrigin[cell] - origin) <= 1e-12 && juce::exactlyEqual (r.barLen[cell], len),
                         what + juce::String::formatted (" origin %.6f len %.6f", cell < r.barOrigin.size() ? r.barOrigin[cell] : -1.0,
                                                         cell < r.barLen.size() ? r.barLen[cell] : -1.0));
            };
            check (runTracker (Transport (kFs, 120.0, 0.0, true, 4, 4), 96000, 512), 100, 0.0, 4.0, "4/4");
            check (runTracker (Transport (kFs, 120.0, 0.0, true, 3, 4), 96000, 512), 100, 0.0, 3.0, "3/4");
            check (runTracker (Transport (kFs, 120.0, 0.0, true, 7, 8), 96000, 512), 100, 0.0, 3.5, "7/8");
            // 3/4 → 7/8 at ppq 6 (sample 144000): origin floorMod (6, 3.5) = 2.5 from there on.
            Transport ch (kFs, 120.0, 0.0, true, 3, 4); ch.timeSig (144000, 7, 8);
            const auto r = runTracker (ch, 240000, 441);
            check (r, 144000 / 32 + 10, 2.5, 3.5, "3/4 -> 7/8 at ppq 6");
            check (r, 100, 0.0, 3.0, "before the change");
            // Locate into a 3/4 bar that starts at ppq 1.5 (pickup): lastBarStart 1.5 → origin 1.5.
            Transport pk (kFs, 120.0, 0.0, true, 3, 4); pk.timeSig (36000, 3, 4);   // ppq 1.5 → a bar starts there
            check (runTracker (pk, 96000, 512), 36000 / 32 + 5, 1.5, 3.0, "3/4 bar at ppq 1.5");
            // No time signature / no bar start → 4/4, origin 0.
            Transport ns (kFs, 120.0, 0.0, true, 3, 4); ns.noInfo (true, true, false);
            auto edit = [] (juce::int64, const ogran::TransportSnapshot& s) { auto x = s; x.hasLastBar = false; x.num = 4; x.den = 4; return x; };
            check (runTracker (ns, 96000, 512, false, edit), 100, 0.0, 4.0, "absent -> 4/4");
            row (t.bad == 0, "TT4 R44 bars: 4/4, 3/4, 7/8, 3/4 -> 7/8 at a split, pickup bar, 4/4 fallback", t.detail (7));
        }

        // TT5 — pre-roll from ppq −4: floor / ceil across 0, a 1/16 grid has no gap or double.
        {
            const auto r = runTracker (Transport (kFs, 120.0, -4.0), 4 * 48000, 441);
            std::int64_t prev = std::numeric_limits<std::int64_t>::min();
            int gaps = 0, back = 0;
            bool sawZero = false;
            for (double q : r.cellPpq)
            {
                const auto s = (std::int64_t) std::ceil (q / 0.25 - 1e-9);
                if (prev != std::numeric_limits<std::int64_t>::min())
                {
                    if (s > prev + 1) ++gaps;
                    if (s < prev) ++back;
                }
                sawZero = sawZero || s == 0;
                prev = s;
            }
            const bool fracOk = juce::exactlyEqual (ogran::fracPart (-3.75), 0.25) && juce::exactlyEqual (ogran::fracPart (-4.0), 0.0)
                                && juce::exactlyEqual (ogran::floorMod (-5.0, 4.0), 3.0);
            row (juce::exactlyEqual (r.cellPpq.front(), -4.0) && gaps == 0 && back == 0 && sawZero && fracOk,
                 "TT5 pre-roll ppq -4 -> 4: 1/16 slots step by <= 1 per cell across 0, frac / floorMod exact",
                 juce::String::formatted ("first %.3f, gaps %d, backwards %d", r.cellPpq.front(), gaps, back));
        }
    }

    //==========================================================================
    // TS — Tempo clock (T5, FUNC-04a/b). Onset instants come from the spawn log
    // (onsetSample − frac); the oracle is the mock's analytic tempo map.
    //==========================================================================
    using Rec = ogran::Engine::SpawnRecord;

    /** Stream 1 alone, Tempo, jitter 0, short grains (only the clock is judged). */
    Job tempoJob (const Transport& tr, int densSync, juce::int64 len, float jitter = 0.0f)
    {
        Job j;
        j.params = eg22::base ({ 1 });
        for (auto& kv : P { { "sync_mode", 1.0f }, { sp (1, "density_sync"), (float) densSync }, { sp (1, "jitter"), jitter * 100.0f },
                            { sp (1, "size_sync"), 1.0f }, { "grain_limit", 3.0f } })
            j.params.push_back (kv);
        j.len = len;
        j.block = 512;
        j.stim = eg::noiseStim (0.25f);
        withTransport (j, tr);
        return j;
    }

    /** Attach a spawn collector; returns the log the render fills. */
    std::shared_ptr<eg22::RecLog> collectInto (Job& j)
    {
        auto L = std::make_shared<eg22::RecLog>();
        auto prev = j.after;
        auto inner = eg22::collect (*L);
        j.after = [L, prev, inner] (Proc& p, juce::int64 pos) { if (prev) prev (p, pos); inner (p, pos); };
        return L;
    }

    std::vector<double> onsetTimes (const eg22::RecLog& L, int stream = 0)
    {
        std::vector<double> v;
        for (const auto& r : L.r)
            if (r.stream == stream) v.push_back ((double) r.onsetSample - r.frac);
        return v;
    }

    struct Expect { std::vector<double> t; int boundary = 0; std::vector<juce::int64> boundaryAt; };

    /** Analytic onset instants: per segment, the grid origin + m·div_eff from the
        segment's ppq0 (inclusive, 1e-9 div) to the next segment, mapped through the
        segment's tempo. A slot due inside the last sample before the next segment
        (ceil (t) == next.at) is a BOUNDARY slot (it would fire on the next segment's
        first sample against the new anchor) and is excluded from both sides. */
    Expect analyticGrid (const Transport& tr, int densSync, juce::int64 len)
    {
        Expect ex;
        for (size_t k = 0; k < tr.segs.size(); ++k)
        {
            const auto& s = tr.segs[k];
            if (! s.playing) continue;
            const juce::int64 end = k + 1 < tr.segs.size() ? tr.segs[k + 1].at : len;
            const double pps = s.bpm / 60.0 / tr.fs;
            const double barLen = MockPlayHead::barLen (s);
            const double origin = ogran::floorMod (s.barPpq, barLen);
            const double divBeats = ogran::tempoDivBeats (densSync, barLen);
            const double d0 = pps * tr.fs / divBeats;
            const double e = juce::jlimit (std::ceil (std::log2 (0.5 / d0)), std::floor (std::log2 (200.0 / d0)), 0.0);
            const double div = divBeats * std::exp2 (-e);
            const double qEnd = s.ppq0 + (double) (end - s.at) * pps;
            for (auto m = (std::int64_t) std::ceil ((s.ppq0 - origin) / div - 1e-9);; ++m)
            {
                const double g = origin + (double) m * div;
                if (! (g < qEnd)) break;
                const double t = (double) s.at + (g - s.ppq0) / pps;
                if (t >= (double) end) break;
                if ((juce::int64) std::ceil (t) >= end && end < len) { ++ex.boundary; ex.boundaryAt.push_back (end); continue; }
                ex.t.push_back (t);
            }
        }
        return ex;
    }

    /** Every expected instant matched by an engine onset within tol; every engine
        onset either matches or sits on a boundary sample. */
    bool matchGrid (const std::vector<double>& got, const Expect& ex, double tol, double& worst, int& missing, int& extra)
    {
        worst = 0.0; missing = 0; extra = 0;
        std::vector<bool> used (got.size(), false);
        for (double t : ex.t)
        {
            auto it = std::lower_bound (got.begin(), got.end(), t - 0.5);
            bool found = false;
            for (; it != got.end() && *it < t + 0.5; ++it)
            {
                const size_t i = (size_t) (it - got.begin());
                if (! used[i]) { used[i] = true; worst = juce::jmax (worst, std::abs (*it - t)); found = true; break; }
            }
            if (! found) ++missing;
        }
        for (size_t i = 0; i < got.size(); ++i)
            if (! used[i])
            {
                bool onBoundary = false;
                for (auto b : ex.boundaryAt) onBoundary = onBoundary || std::abs (got[i] - (double) b) < 1.0;
                if (! onBoundary) ++extra;
            }
        return missing == 0 && extra == 0 && worst <= tol;
    }

    void tempoClockRows()
    {
        std::printf ("\n-- TS. Tempo clock (FUNC-04a/b; R40, R41, R45)\n");
        constexpr juce::int64 kLen = 8 * 48000;

        // TS1 — 19 density divisions × 8 transport scenarios.
        {
            struct Sc { const char* name; Transport tr; };
            std::vector<Sc> sc;
            sc.push_back ({ "120", Transport (kFs, 120.0) });
            sc.push_back ({ "133", Transport (kFs, 133.0) });
            { Transport t (kFs, 120.0); t.tempo (100000, 97.3); sc.push_back ({ "step 120->97.3", t }); }
            { Transport t (kFs, 120.0); t.loopWrap (150016, 1.0, 6.25); sc.push_back ({ "loop", t }); }
            { Transport t (kFs, 133.0); t.locate (150016, 13.37); sc.push_back ({ "locate", t }); }
            sc.push_back ({ "pre-roll -4", Transport (kFs, 120.0, -4.0) });
            sc.push_back ({ "3/4", Transport (kFs, 120.0, 0.0, true, 3, 4) });
            sc.push_back ({ "7/8", Transport (kFs, 133.0, 0.0, true, 7, 8) });
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg22::RecLog>> logs;
            std::vector<std::pair<int, int>> ids;
            for (int si = 0; si < (int) sc.size(); ++si)
                for (int d = 0; d < 19; ++d)
                {
                    Job j = tempoJob (sc[(size_t) si].tr, d, kLen);
                    logs.push_back (collectInto (j));
                    jobs.push_back (j);
                    ids.push_back ({ si, d });
                }
            renderAll (jobs);
            Tally t;
            double worstAll = 0.0;
            int boundary = 0, onsets = 0;
            for (size_t k = 0; k < jobs.size(); ++k)
            {
                const auto& s = sc[(size_t) ids[k].first];
                const auto ex = analyticGrid (s.tr, ids[k].second, kLen);
                const auto got = onsetTimes (*logs[k]);
                double worst; int missing, extra;
                const bool ok = matchGrid (got, ex, 1e-6, worst, missing, extra) && ! logs[k]->overflow && ! got.empty();
                worstAll = juce::jmax (worstAll, worst);
                boundary += ex.boundary;
                onsets += (int) got.size();
                t.check (ok, juce::String (s.name) + " / " + GranulatorParamIds::densitySyncs()[ids[k].second]
                                 + juce::String::formatted (": missing %d extra %d worst %.2e (n %d / %d)", missing, extra, worst,
                                                            (int) got.size(), (int) ex.t.size()));
            }
            row (t.bad == 0, "TS1 Tempo onsets == analytic grid (<= 1e-6 sample, exact count): 19 divisions x 8 transports",
                 t.detail ((int) jobs.size()) + juce::String::formatted ("  worst %.2e, %d onsets, %d boundary slots excluded", worstAll, onsets, boundary));
        }

        // TS1n — ceilPerTick (the slot re-derived from ppq every tick, R40). 120 bpm / 1/16
        // puts every grid point on an INTEGER sample (6000), where the ε-ceil is exact;
        // the control runs 133 bpm / 1/16 T (fractional grid times) over 20 k cells.
        {
            const juce::int64 len = 20000 * 32;
            Transport tr (kFs, 133.0);
            Job a = tempoJob (tr, 6, len), b = a;
            auto La = collectInto (a);
            auto Lb = collectInto (b);
            auto pre = b.pre;
            b.pre = [pre] (Proc& p) { if (pre) pre (p); p.getEngineForTest().hooks.ceilPerTick = true; };
            renderAll ({ a, b });
            const auto ex = analyticGrid (tr, 6, len);
            double w1, w2; int m1, e1, m2, e2;
            const bool okA = matchGrid (onsetTimes (*La), ex, 1e-6, w1, m1, e1);
            matchGrid (onsetTimes (*Lb), ex, 1e-6, w2, m2, e2);
            row (okA && (m2 + e2) >= 1, "TS1n ceilPerTick (133 bpm, 1/16 T, 20 k cells): >= 1 dropped / doubled slot; tracked slot exact",
                 juce::String::formatted ("tracked missing %d extra %d; per-tick missing %d extra %d", m1, e1, m2, e2));
        }

        // TS2 — enable edge and Free → Tempo switch (R41): the first onset is the next slot >= now.
        {
            const juce::int64 edge = 100032;   // a tick, off the 1/16 grid (6000)
            Transport tr (kFs, 120.0);
            Job en = tempoJob (tr, 7, 3 * 48000);
            for (auto& kv : en.params) if (kv.first == sp (1, "enable")) kv.second = 0.0f;
            en.events.push_back ({ edge, [] (Proc& p) { eg::setP (p, sp (1, "enable"), 1.0f); } });
            auto Le = collectInto (en);

            Job sw = tempoJob (tr, 7, 3 * 48000, 0.5f);
            for (auto& kv : sw.params) if (kv.first == "sync_mode") kv.second = 0.0f;
            sw.events.push_back ({ edge, [] (Proc& p) { eg::setP (p, "sync_mode", 1.0f); } });
            auto Ls = collectInto (sw);
            Job fresh = tempoJob (tr, 7, 3 * 48000, 0.5f);
            auto Lf = collectInto (fresh);
            renderAll ({ en, sw, fresh });

            const auto te = onsetTimes (*Le);
            const bool enOk = ! te.empty() && std::abs (te.front() - 102000.0) <= 1e-6;
            const Rec* firstSw = nullptr;
            for (const auto& r : Ls->r) if (r.onsetSample >= edge) { firstSw = &r; break; }
            const double ts = firstSw != nullptr ? (double) firstSw->onsetSample - firstSw->frac : -1.0;
            // jitter 0.5 with u_0 = ½: the first Tempo onset sits ON its slot.
            const bool swOk = firstSw != nullptr && std::abs (ts - 102000.0) <= 1e-6 && firstSw->globalIndex > 0
                              && ! Lf->r.empty() && Lf->r.front().globalIndex == 0;
            row (enOk, "TS2 Tempo enable at an off-grid tick (100032): first onset at the next 1/16 slot (102000)",
                 te.empty() ? juce::String ("no onsets") : juce::String::formatted ("%.6f", te.front()));
            row (swOk, "TS2 Free -> Tempo switch: first Tempo onset on the next slot, epoch folded (no replay of a fresh start)",
                 juce::String::formatted ("t %.6f, globalIndex %llu (fresh %llu)", ts, firstSw ? (unsigned long long) firstSw->globalIndex : 0ull,
                                          Lf->r.empty() ? 0ull : (unsigned long long) Lf->r.front().globalIndex));
        }

        // TS3 — jitter 50 %, 1/16 at 120, 60 s: telescoping keeps the mean (Q2).
        {
            const juce::int64 len = 60 * 48000;
            Transport tr (kFs, 120.0);
            Job j = tempoJob (tr, 7, len, 0.5f);
            auto L = collectInto (j);
            renderAll ({ j });
            const auto t = onsetTimes (*L);
            const double I = 6000.0, J = 0.5;
            int off = 0;
            double worst = 0.0;
            for (size_t k = 0; k < t.size(); ++k)
            {
                const double d = std::abs (t[k] - (double) k * I);
                worst = juce::jmax (worst, d);
                if (d > J * I / 2.0 + 1e-6) ++off;
            }
            const double N = (double) t.size();
            const double mean = (t.back() - t.front()) / (N - 1.0);
            const bool countOk = (juce::int64) t.size() == len / 6000;
            row (off == 0 && countOk && std::abs (mean - I) <= J * I / (N - 1.0) + 1e-9,
                 "TS3 jitter 50 %: every onset within J*I/2 of its slot, exact count, mean within J*I/(N-1) (telescoping)",
                 juce::String::formatted ("n %d, worst |d| %.1f (bound %.0f), mean %.4f", (int) t.size(), worst, J * I / 2.0, mean));
        }

        // TS4 — effective values (R45).
        {
            Tally t;
            auto effective = [] (double bpm, int densSync, int sizeSync, int num = 4)
            {
                Transport tr (kFs, bpm, 0.0, true, num, 4);
                Job j = tempoJob (tr, densSync, 4096);
                for (auto& kv : j.params) if (kv.first == sp (1, "size_sync")) kv.second = (float) sizeSync;
                auto o = renderAll ({ j });
                const auto& e = o[0].eng();
                return std::make_tuple ((double) e.getGridParams().s[0].density, (double) e.getGridParams().s[0].sizeMs, e.getTempoDiv (0));
            };
            {
                auto [d, ms, div] = effective (60.0, 7, 18);   // 1 bar at 60 bpm = 4 s → 2000 ms
                t.check (juce::exactlyEqual (ms, 2000.0), juce::String::formatted ("1 bar @60 size %.3f", ms));
            }
            {
                auto [d, ms, div] = effective (300.0, 0, 7);   // 1/64 T @300: d0 = 120 g/s, e = 0
                t.check (std::abs (d - 120.0) <= 1e-4 && std::abs (div - 0.0625 * 2.0 / 3.0) <= 1e-15, juce::String::formatted ("1/64T @300 d %.6f", d));
            }
            {
                auto [d, ms, div] = effective (600.0, 0, 7);   // d0 = 240 → e = −1 → 120 g/s, div ×2
                t.check (std::abs (d - 120.0) <= 1e-4 && std::abs (div - 2.0 * 0.0625 * 2.0 / 3.0) <= 1e-15 && d <= 200.0,
                         juce::String::formatted ("1/64T @600 d %.6f div %.6f", d, div));
            }
            {
                auto [d, ms, div] = effective (20.0, 18, 7);   // 1 bar @20: d0 = 1/12 → e = 3 → 2/3 g/s
                t.check (std::abs (d - 2.0 / 3.0) <= 1e-6 && std::abs (div - 0.5) <= 1e-15 && d >= 0.5,
                         juce::String::formatted ("1 bar @20 d %.6f div %.6f", d, div));
            }
            {
                auto [d, ms, div] = effective (120.0, 18, 18, 3);   // 3/4: 1 bar = 3 quarters = 1.5 s
                t.check (std::abs (ms - 1500.0) <= 1e-3 && std::abs (div - 3.0) <= 1e-15, juce::String::formatted ("3/4 1 bar size %.3f div %.3f", ms, div));
            }
            row (t.bad == 0, "TS4 effective size clamp 2 s; density power-of-two in [0.5, 200] g/s; 3/4 bar", t.detail (5));

            // Quotas / coherence read the effective density: Tempo 1/64 T @120 (48 g/s) == Free 48 g/s.
            auto quotaOf = [] (bool tempo)
            {
                Transport tr (kFs, 120.0);
                Job j = tempoJob (tr, 0, 4096);
                // sync_mode is global: stream 2 runs its default 1/16 (8 g/s × 125 ms) in Tempo — the Free side matches it.
                for (auto& kv : P { { sp (2, "enable"), 1.0f }, { sp (2, "density"), tempo ? 150.0f : 8.0f }, { sp (2, "size"), tempo ? 400.0f : 125.0f },
                                    { sp (2, "density_sync"), 7.0f }, { sp (2, "size_sync"), 7.0f },
                                    { "grain_limit", 0.0f }, { sp (1, "size"), 125.0f }, { sp (1, "density"), tempo ? 10.0f : 48.0f },
                                    { sp (1, "size_sync"), 7.0f }, { "sync_mode", tempo ? 1.0f : 0.0f } })
                    j.params.push_back (kv);
                auto o = renderAll ({ j });
                return o[0].eng().getQuotas();
            };
            const auto qt = quotaOf (true), qf = quotaOf (false);
            row (std::abs (qt.q[0] - qf.q[0]) <= 1e-6 * qf.q[0] && std::abs (qt.q[1] - qf.q[1]) <= 1e-6 * qf.q[1] && qf.q[0] < 32.0,
                 "TS4 quotas read the effective density: Tempo 1/64 T @120 == Free 48 g/s (grain_limit 32)",
                 juce::String::formatted ("q %.6f / %.6f vs %.6f / %.6f", qt.q[0], qt.q[1], qf.q[0], qf.q[1]));
        }

        // TS5 — stopped, parked on a beat (ppq 0) for 5 s: the virtual tempo runs the grid.
        {
            Transport tr (kFs, 120.0, 0.0, false);
            Job j = tempoJob (tr, 7, 5 * 48000);
            auto L = collectInto (j);
            renderAll ({ j });
            const auto t = onsetTimes (*L);
            int bad = 0;
            for (size_t k = 0; k < t.size(); ++k)
                if (std::abs (t[k] - 6000.0 * (double) k) > 1e-6) ++bad;
            row (t.size() == 40 && bad == 0, "TS5 stopped at ppq 0 for 5 s: 40 onsets on the virtual 1/16 grid, no per-block burst",
                 juce::String::formatted ("n %d, off-grid %d", (int) t.size(), bad));
        }

        // TS6 — FUNC-04b Free density under the 2.5a tree, every LFO running unrouted.
        {
            Tally t;
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg22::RecLog>> logs;
            for (float dens : { 0.5f, 200.0f })
            {
                Job j;
                j.params = eg22::base ({ 1 });
                for (auto& kv : P { { sp (1, "density"), dens }, { sp (1, "jitter"), 0.0f }, { sp (1, "size"), 5.0f } })
                    j.params.push_back (kv);
                for (int n = 1; n <= 4; ++n)
                    for (auto& kv : P { { "lfo" + juce::String (n) + "_shape", (float) n }, { "lfo" + juce::String (n) + "_rate", 3.0f * (float) n } })
                        j.params.push_back (kv);
                j.len = 10 * 48000;
                logs.push_back (collectInto (j));
                withTransport (j, Transport (kFs, 120.0));
                jobs.push_back (j);
            }
            renderAll (jobs);
            const double want[2] { 5.0, 2000.0 };
            for (int k = 0; k < 2; ++k)
            {
                const double n = (double) onsetTimes (*logs[(size_t) k]).size();
                t.check (std::abs (n - want[k]) <= 0.02 * want[k] && ! logs[(size_t) k]->overflow,
                         juce::String::formatted ("%.1f g/s: %.0f onsets / 10 s", want[k] / 10.0, n));
            }
            row (t.bad == 0, "TS6 FUNC-04b Free density 0.5 / 200 g/s within 2 % over 10 s (LFOs running, unrouted)", t.detail (2));
        }
    }
}}

static void engineRows25a()
{
    eg25::mockRows();
    eg25::divisionRows();
    eg25::trackerRows();
    eg25::tempoClockRows();
}
