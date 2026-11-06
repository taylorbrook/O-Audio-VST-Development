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

    //==========================================================================
    // LS — LFO units (T6). The Lfo class is driven cell by cell from a
    // TempoTracker fed by the v2 mock, block by block (the engine's order).
    //==========================================================================
    struct LfoRun
    {
        std::vector<double> pos;     // cycle + x of each cell's value
        std::vector<float>  val;
        std::vector<int>    mode;    // 0 Direct, 1 Accum
        std::vector<double> ppq;     // tracker ppq at each tick
        std::vector<double> origin;  // anchor bar origin at each tick
        std::vector<double> barLen;
    };

    using LfoPer = std::function<void (juce::int64 cell, ogran::GridParams::Lfo&, float& rateOff, double& rateHz)>;

    LfoRun runLfo (const Transport& tr, juce::int64 cells, ogran::GridParams::Lfo p, bool tempo, int index = 0,
                   const LfoPer& per = {}, int block = 512, int forceMode = 0, std::uint64_t seed = 0x5EEDull)
    {
        LfoRun r;
        auto head = tr.makeHead();
        ogran::TempoTracker t;
        t.prepare (tr.fs);
        ogran::Lfo lfo;
        lfo.prepare (tr.fs, ogran::rampCellsFor (tr.fs, ogran::kModRampMs), p.phaseDeg);
        const juce::int64 len = cells * 32;
        const auto starts = blockStarts (len, block, tr.eventSamples());
        float off = 0.0f;
        double rate = p.rateHz;
        for (size_t k = 0; k < starts.size(); ++k)
        {
            const juce::int64 h = starts[k], e = k + 1 < starts.size() ? starts[k + 1] : len;
            head->timeInSamples = h;
            t.update (snapOf (*head), h);
            for (juce::int64 c = ((h + 31) / 32) * 32; c < e; c += 32)
            {
                if (per) per (c / 32, p, off, rate);
                else rate = p.rateHz;
                r.val.push_back (lfo.tick (index, p, tempo, t, c, off, rate, seed, forceMode));
                r.pos.push_back (lfo.position());
                r.mode.push_back (lfo.getMode() == ogran::Lfo::Mode::Direct ? 0 : 1);
                r.ppq.push_back (t.ppqAt (c));
                r.origin.push_back (t.anchor().barOrigin);
                r.barLen.push_back (t.anchor().barLen);
            }
        }
        return r;
    }

    void lfoRows()
    {
        std::printf ("\n-- LS. LFO shapes / phase (R50, R51; D15 as amended)\n");
        using L = ogran::Lfo;

        // LS1 — shapes on a 1/1024 grid vs the R51 formulas; determinism; smooth continuity.
        {
            Tally t;
            const std::uint64_t seed = 0xABCDull;
            auto h = [] (std::uint64_t sd, int n, std::int64_t c)
            {
                return (double) ouaricon::grain::CounterRng::Draw (sd, ogran::kLfoStream + (std::uint64_t) n, (std::uint64_t) c).nextFloat (0);
            };
            bool range = true;
            for (int s = 0; s < 6; ++s)
                for (std::int64_t c : { (std::int64_t) -3, (std::int64_t) 0, (std::int64_t) 7 })
                    for (int i = 0; i < 1024; ++i)
                    {
                        const double x = i / 1024.0;
                        float want = 0.0f;
                        switch (s)
                        {
                            case 0: want = (float) std::sin (2.0 * ogran::kFxPi * x); break;
                            case 1: want = (float) (1.0 - 4.0 * std::abs (x - 0.5)); break;
                            case 2: want = (float) (2.0 * x - 1.0); break;
                            case 3: want = x < 0.5 ? 1.0f : -1.0f; break;
                            case 4: want = (float) (2.0 * h (seed, 2, c) - 1.0); break;
                            default:
                            {
                                const double a = h (seed, 2, c), b = h (seed, 2, c + 1);
                                want = (float) (2.0 * (a + (b - a) * 0.5 * (1.0 - std::cos (ogran::kFxPi * x))) - 1.0);
                            }
                        }
                        const float got = L::shape (s, x, c, seed, 2);
                        t.check (std::memcmp (&got, &want, 4) == 0, juce::String::formatted ("shape %d c %lld x %.4f", s, (long long) c, x));
                        range = range && got >= -1.0f && got <= 1.0f;
                    }
            bool det = true, differs = false;
            for (std::int64_t c = 0; c < 64; ++c)
            {
                det = det && juce::exactlyEqual (L::shape (4, 0.3, c, seed, 1), L::shape (4, 0.3, c, seed, 1));
                differs = differs || ! juce::exactlyEqual (L::shape (4, 0.3, c, seed, 1), L::shape (4, 0.3, c, seed + 1, 1));
            }
            row (t.bad == 0 && range && det && differs, "LS1 six shapes bitwise == R51 on a 1/1024 grid (c -3, 0, 7); [-1, 1]; S&H seeded, seed+1 differs",
                 t.detail (6 * 3 * 1024));

            // Smooth random continuity at 5 Hz over 10 s (Free accumulator).
            ogran::GridParams::Lfo p; p.shape = 5; p.rateHz = 5.0f;
            const auto r = runLfo (Transport (kFs, 120.0), 15000, p, false, 1, {}, 512, 0, seed);
            const double inc = 32.0 * 5.0 / kFs;
            double worstRatio = 0.0;
            for (size_t k = 1; k < r.val.size(); ++k)
            {
                const auto c0 = (std::int64_t) std::floor (r.pos[k - 1]);
                double dh = 0.0;
                for (std::int64_t c = c0 - 1; c <= c0 + 1; ++c) dh = juce::jmax (dh, std::abs (h (seed, 1, c + 1) - h (seed, 1, c)));
                const double bound = ogran::kFxPi * dh * inc * (1.0 + 1e-3) + 1e-6;
                worstRatio = juce::jmax (worstRatio, std::abs ((double) r.val[k] - r.val[k - 1]) / bound);
            }
            row (worstRatio <= 1.0, "LS1 smooth random continuous across cycles: |dv| per cell <= pi*|dh|*inc",
                 juce::String::formatted ("worst / bound %.3f", worstRatio));
        }

        // LS2 / LS2n — Free accumulator under rate automation 0.5 → 0.51 Hz at 60 s.
        {
            ogran::GridParams::Lfo p; p.rateHz = 0.5f;
            const juce::int64 at = 60 * 48000 / 32, cells = 70 * 48000 / 32;
            auto per = [] (juce::int64 cell, ogran::GridParams::Lfo& q, float&, double& rate) { q.rateHz = cell >= at ? 0.51f : 0.5f; rate = q.rateHz; };
            const auto a = runLfo (Transport (kFs, 120.0), cells, p, false, 0, per);
            const auto n = runLfo (Transport (kFs, 120.0), cells, p, false, 0, per, 512, 1);
            double worst = 0.0, worstN = 0.0;
            for (size_t k = 1; k < a.pos.size(); ++k)
            {
                worst  = juce::jmax (worst, a.pos[k] - a.pos[k - 1]);
                worstN = juce::jmax (worstN, std::abs (n.pos[k] - n.pos[k - 1]));
            }
            const double bound = 32.0 * (double) 0.51f / kFs * (1.0 + 1e-9);
            bool allAccum = true;
            for (int m : a.mode) allAccum = allAccum && m == 1;
            row (worst <= bound && allAccum, "LS2 Free accumulator: rate 0.5 -> 0.51 Hz at 60 s, |dphi| per cell <= increment (no jump)",
                 juce::String::formatted ("max %.3e (bound %.3e)", worst, bound));
            row (worstN >= 0.5, "LS2n Free direct frac(n*inc) (forceLfoMode Direct): jumps >= 0.5 cycle at the automation (R50)",
                 juce::String::formatted ("max |dphi| %.3f cycle", worstN));
        }

        // LS3 — Tempo direct lock over 64 bars; re-lock after a loop and a locate.
        {
            Tally t;
            for (double bpm : { 120.0, 133.0 })
                for (int sync : { 18, 6, 11 })   // 1 bar, 1/16 T, 1/8 D
                {
                    ogran::GridParams::Lfo p; p.rateSync = sync;
                    const auto cells = (juce::int64) std::ceil (64.0 * 4.0 / (bpm / 60.0) * kFs / 32.0);
                    Transport tr (kFs, bpm);
                    const auto r = runLfo (tr, cells, p, true);
                    const double pps = bpm / 60.0 / kFs, div = ogran::tempoDivBeats (sync, 4.0);
                    int bad = 0;
                    for (size_t k = 0; k < r.pos.size(); ++k)
                    {
                        const double v = (0.0 + (double) ((juce::int64) k * 32 - 0) * pps - 0.0) / div;
                        const double c = std::floor (v);
                        const double want = c + (v - c);
                        if (std::memcmp (&want, &r.pos[k], sizeof (double)) != 0 || r.mode[k] != 0) ++bad;
                    }
                    t.check (bad == 0, juce::String (bpm) + " bpm / " + GranulatorParamIds::lfoRateSyncs()[sync] + juce::String::formatted (": %d cells off", bad));
                }
            row (t.bad == 0, "LS3 Tempo direct: phase bitwise == frac((ppq - barOrigin)/div) every cell, 64 bars (120/133 x 1 bar, 1/16 T, 1/8 D)",
                 t.detail (6));

            Tally t2;
            for (int kind = 0; kind < 2; ++kind)
            {
                Transport tr (kFs, 120.0);
                if (kind == 0) tr.loopWrap (100000, 2.0, 10.0); else tr.locate (100000, 37.3);
                ogran::GridParams::Lfo p; p.rateSync = 11;
                const auto r = runLfo (tr, 6000, p, true);
                int bad = 0;
                for (size_t k = 0; k < r.pos.size(); ++k)
                {
                    const double v = (r.ppq[k] - r.origin[k]) / ogran::tempoDivBeats (11, r.barLen[k]);
                    const double c = std::floor (v), want = c + (v - c);
                    if (std::memcmp (&want, &r.pos[k], sizeof (double)) != 0) ++bad;
                }
                t2.check (bad == 0, juce::String (kind == 0 ? "loop" : "locate") + juce::String::formatted (" %d off", bad));
            }
            row (t2.bad == 0, "LS3 after a loop wrap and a locate: re-locked to the direct formula from the re-lock cell", t2.detail (2));
        }

        // LS4 — lfo_phase 359° → 1° moves +2° over 20 ms (shortest arc).
        {
            ogran::GridParams::Lfo p; p.phaseDeg = 359.0f; p.rateHz = 0.01f;
            auto per = [] (juce::int64 cell, ogran::GridParams::Lfo& q, float&, double& rate) { q.phaseDeg = cell >= 100 ? 1.0f : 359.0f; rate = q.rateHz; };
            Transport tr (kFs, 120.0);
            auto head = tr.makeHead();
            ogran::TempoTracker tt; tt.prepare (kFs); tt.update (snapOf (*head), 0);
            ogran::Lfo lfo; lfo.prepare (kFs, ogran::rampCellsFor (kFs, ogran::kModRampMs), p.phaseDeg);
            std::vector<float> ph;
            float off = 0.0f; double rate = 0.01;
            for (juce::int64 c = 0; c < 200; ++c)
            {
                per (c, p, off, rate);
                lfo.tick (0, p, false, tt, c * 32, off, rate, 1);
                ph.push_back (lfo.getPhaseDeg());
            }
            bool mono = true;
            for (size_t k = 101; k < ph.size(); ++k) mono = mono && ph[k] >= ph[k - 1];
            const float end = ph.back();
            row (mono && std::abs (end - 361.0f) <= 1e-4f && juce::exactlyEqual (ph[99], 359.0f) && std::abs (ph[115] - 360.0f) < 1.0f,
                 "LS4 lfo_phase 359 -> 1 deg: +2 deg over 20 ms (shortest arc, monotone)",
                 juce::String::formatted ("%.4f -> %.4f (mid %.4f)", ph[99], end, ph[115]));
        }

        // LS5 — pre-roll: negative S&H cycles deterministic across block schedules, distinct from positive ones.
        {
            ogran::GridParams::Lfo p; p.shape = 4; p.rateSync = 13;   // 1/4 = 1 beat
            Transport tr (kFs, 120.0, -8.0);
            const auto a = runLfo (tr, 16 * 24000 / 32, p, true, 3, {}, 512);
            const auto b = runLfo (tr, 16 * 24000 / 32, p, true, 3, {}, 441);
            bool same = a.val.size() == b.val.size();
            for (size_t k = 0; same && k < a.val.size(); ++k) same = juce::exactlyEqual (a.val[k], b.val[k]);
            std::vector<float> neg, pos;
            for (size_t k = 0; k < a.val.size(); ++k)
            {
                const auto c = (std::int64_t) std::floor (a.pos[k]);
                if (c < 0 && (neg.empty() || ! juce::exactlyEqual (neg.back(), a.val[k]))) neg.push_back (a.val[k]);
                if (c >= 0 && (pos.empty() || ! juce::exactlyEqual (pos.back(), a.val[k]))) pos.push_back (a.val[k]);
            }
            bool distinct = neg.size() >= 7 && pos.size() >= 7;
            for (size_t k = 0; distinct && k < 7; ++k) distinct = ! juce::exactlyEqual (neg[k], pos[k]);
            row (same && distinct, "LS5 pre-roll S&H: negative cycles deterministic (512 vs 441 blocks) and distinct from cycles >= 0",
                 juce::String::formatted ("%d / %d steps", (int) neg.size(), (int) pos.size()));
        }
    }

    //==========================================================================
    // EV / MC — envelope follower and macros (engine renders, block 32: one
    // cell per block, read after each block)
    //==========================================================================
    struct CellTrace { std::vector<float> envState, envSrc, macro1; bool srcInRange = true; };

    Job srcJob (juce::int64 len, std::function<float (juce::int64, int)> stim, P extra = {})
    {
        Job j;
        j.params = eg22::base ({ 1 });
        for (auto& kv : P { { "mix", 0.0f }, { sp (1, "density"), 5.0f }, { sp (1, "size"), 50.0f } }) j.params.push_back (kv);
        for (auto& kv : extra) j.params.push_back (kv);
        j.len = len;
        j.block = 32;
        j.stim = std::move (stim);
        return j;
    }
    std::shared_ptr<CellTrace> traceCells (Job& j)
    {
        auto tr = std::make_shared<CellTrace>();
        auto prev = j.after;
        j.after = [tr, prev] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const auto& e = p.getEngineForTest();
            tr->envState.push_back (e.getEnv().state());
            tr->envSrc.push_back (e.getSrcTelemetry (ogran::ModSrc::Env));
            tr->macro1.push_back (e.getSrcTelemetry (ogran::ModSrc::Macro1));
            for (int s = 1; s <= ogran::kSrcTelemetry; ++s)
            {
                const float v = e.getSrcTelemetry (s);
                const bool bip = s >= ogran::ModSrc::Lfo1 && s <= ogran::ModSrc::Lfo4;
                tr->srcInRange = tr->srcInRange && std::isfinite (v) && v <= 1.0f && v >= (bip ? -1.0f : 0.0f);
            }
        };
        return tr;
    }

    void envRows()
    {
        std::printf ("\n-- EV / MC. envelope follower (R52) and macros\n");
        auto dc = [] (float a, double t0, double t1)
        {
            return [a, t0, t1] (juce::int64 i, int) { const double t = (double) i / kFs; return t >= t0 && t < t1 ? a : 0.0f; };
        };

        // EV1 — the follower law on a −20 dBFS DC step (1 s on, 2 s off).
        {
            Job j = srcJob (3 * 48000, dc (0.1f, 0.0, 1.0), { { "env_attack", 10.0f }, { "env_release", 200.0f } });
            auto tr = traceCells (j);
            Job g = srcJob (2 * 48000, dc (0.01f, 0.0, 2.0), { { "env_gain", 24.0f } });
            auto tg = traceCells (g);
            auto outs = renderAll ({ j, g });
            const auto& env = outs[0].eng().getEnv();
            // Bitwise float reference of the one-pole (input gain 0 dB = 1.0f, env gain 1).
            float e = 0.0f; int bad = 0;
            const float aA = env.attackCoeff(), aR = env.releaseCoeff();
            for (juce::int64 i = 0; i < 3 * 48000; ++i)
            {
                const float s = 0.5f * (dc (0.1f, 0.0, 1.0) (i, 0) + dc (0.1f, 0.0, 1.0) (i, 1)) * 1.0f;
                const float x = std::abs (s * 1.0f);
                e += (x > e ? aA : aR) * (x - e);
                if ((i + 1) % 32 == 0 && ! juce::exactlyEqual (e, tr->envState[(size_t) ((i + 1) / 32 - 1)])) ++bad;
            }
            const double at63 = tr->envState[14] / 0.1;                 // after 480 samples = τ
            const double expect63 = 1.0 - std::exp (-1.0);
            const float settled = tr->envSrc[1499];                      // the tick at 1 s − 1 cell
            const float silent = tr->envSrc.back();
            row (bad == 0 && std::abs (at63 - expect63) <= 1e-4 && std::abs (settled - 2.0f / 3.0f) <= 1e-4f && juce::exactlyEqual (silent, 0.0f)
                 && tr->srcInRange,
                 "EV1 follower: state bitwise == the one-pole, 63 % at tau (10 ms), -20 dBFS -> 0.667, silence -> exactly 0",
                 juce::String::formatted ("%d off, e(tau)/A %.6f (%.6f), src %.6f, silence %g", bad, at63, expect63, settled, silent));
            row (std::abs (tg->envSrc.back() - (44.0f / 60.0f)) <= 1e-3f && tg->srcInRange,
                 "EV1 env_gain +24 dB: -40 dBFS DC reads (-16 + 60)/60", juce::String::formatted ("%.5f", tg->envSrc.back()));
        }

        // EV2 — tap point: feedback return not tapped; input_gain applies; runs while frozen.
        {
            // Gated noise (DC would be removed by the loop's 80 Hz HP: nothing would recirculate).
            auto gatedNoise = [] (juce::int64 i, int ch) { return i < 48000 ? eg::hashNoise (i, ch, 0.3f) : 0.0f; };
            Job fb = srcJob (13 * 4800 * 4, gatedNoise, { { "feedback", 100.0f }, { "mix", 100.0f }, { sp (1, "density"), 40.0f },
                                                       { sp (1, "size"), 200.0f } });
            auto tfb = traceCells (fb);
            Job lo = srcJob (48000, dc (0.1f, 0.0, 1.0), { { "input_gain", -24.0f } });
            auto tlo = traceCells (lo);
            Job hi = srcJob (48000, dc (0.1f, 0.0, 1.0));
            auto thi = traceCells (hi);
            Job fz = srcJob (2 * 48000, dc (0.1f, 1.0, 2.0));
            fz.events.push_back ({ 24000, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
            auto tfz = traceCells (fz);
            auto outs = renderAll ({ fb, lo, hi, fz });
            double wetTail = 0.0;
            for (size_t i = 2 * 48000; i < outs[0].L.size(); ++i) wetTail = juce::jmax (wetTail, (double) std::abs (outs[0].L[i]));
            row (juce::exactlyEqual (tfb->envSrc.back(), 0.0f) && wetTail > 1e-3,
                 "EV2 feedback 100 %, input removed: env -> exactly 0 while the loop still rings (return not tapped)",
                 juce::String::formatted ("env %g, wet tail peak %.4f", tfb->envSrc.back(), wetTail));
            const float d = thi->envSrc.back() - tlo->envSrc.back();
            row (std::abs (d - 0.4f) <= 1e-3f, "EV2 input_gain -24 dB lowers the env source by 24/60", juce::String::formatted ("%.5f", d));
            row (std::abs (tfz->envSrc.back() - 2.0f / 3.0f) <= 1e-3f && outs[3].eng().diag.freezeEngages == 1,
                 "EV2 freeze on: the follower still tracks the input", juce::String::formatted ("%.5f", tfz->envSrc.back()));
        }

        // EV3 — NaN at the tap: one scrub, 0 at the next tick, audio bitwise unaffected.
        {
            Job a = srcJob (2 * 48000, eg::noiseStim (0.3f), { { "mix", 100.0f } });
            Job b = a;
            auto ta = traceCells (a);
            auto tb = traceCells (b);
            auto pre = b.pre;
            b.pre = [pre] (Proc& p) { if (pre) pre (p); p.getEngineForTest().hooks.injectEnvNanAt = 48000 + 7; };
            auto outs = renderAll ({ a, b });
            const auto d = eg::diff (outs[0], outs[1]);
            const size_t k = 48000 / 32;   // the block holding the injection is cell 1500; the next tick reads 0
            row (outs[1].eng().diag.envScrubs == 1 && outs[0].eng().diag.envScrubs == 0 && juce::exactlyEqual (tb->envSrc[k + 1], 0.0f)
                 && ta->envSrc[k + 1] > 0.1f && d.count == 0,
                 "EV3 NaN at the follower: 1 scrub, source 0 at the next tick, audio bitwise unaffected",
                 juce::String::formatted ("scrubs %lld, src %g (clean %g), audio diffs %d", (long long) outs[1].eng().diag.envScrubs,
                                          tb->envSrc[k + 1], ta->envSrc[k + 1], d.count));
        }

        // MC1 — macro1 0 → 100 % ramps linearly over 20 ms (30 cells), unipolar.
        {
            Job j = srcJob (48000, eg::noiseStim (0.1f));
            j.events.push_back ({ 320 * 32, [] (Proc& p) { eg::setP (p, "macro1", 100.0f); } });
            auto t = traceCells (j);
            renderAll ({ j });
            bool lin = juce::exactlyEqual (t->macro1[319], 0.0f);
            for (int c = 0; c < 30; ++c)
                lin = lin && std::abs (t->macro1[(size_t) (320 + c)] - (float) (c + 1) / 30.0f) <= 1e-6f;
            lin = lin && juce::exactlyEqual (t->macro1[349], 1.0f) && juce::exactlyEqual (t->macro1.back(), 1.0f) && t->srcInRange;
            row (lin, "MC1 macro 0 -> 100 %: linear over 30 cells (20 ms), exactly 1 on arrival; sources in range",
                 juce::String::formatted ("%.6f %.6f %.6f", t->macro1[320], t->macro1[334], t->macro1[349]));
        }
    }

    //==========================================================================
    // U9b–g — matrix units (T7), on a copy of a processor's matrix (its ranges
    // and raw pointers were built by the processor from the parameter objects).
    //==========================================================================
    void matrixUnitRows()
    {
        std::printf ("\n-- U9b-g. modulation matrix (R46, R49, R55)\n");
        namespace D = ogran::ModDest;
        auto proc = make();
        const auto& eng = proc->getEngineForTest();
        const ogran::ModMatrix& M = eng.getMatrix();

        // U9b — the engine's destination layout == kModDestIds (the Stage 1 golden pins the ids).
        {
            Tally t;
            static const char* fields[] { "level", "pan", "position", "spray", "size", "size_rand", "density", "jitter", "pitch",
                                          "pitch_rand", "reverse_prob", "window", "window_skew", "stereo_spread", "gain_rand",
                                          "filter_cutoff", "filter_rand", "filter_res", "drive", "crush", "crush_prob", "ring_freq",
                                          "ring_rand", "ring_mix" };
            for (int s = 0; s < 4; ++s)
                for (int f = 0; f < D::NumFields; ++f)
                    t.check (juce::String (GranulatorParamIds::kModDestIds[D::stream (s, f)]) == sp (s + 1, fields[f]), sp (s + 1, fields[f]));
            const std::pair<int, const char*> g[] { { D::InputGain, "input_gain" }, { D::Mix, "mix" }, { D::OutputGain, "output_gain" },
                                                    { D::BufferLength, "buffer_length" }, { D::Feedback, "feedback" }, { D::FbLowCut, "fb_low_cut" },
                                                    { D::FbHighCut, "fb_high_cut" }, { D::FbSat, "fb_saturation" }, { D::FbDiff, "fb_diffusion" },
                                                    { D::RevMix, "reverb_mix" }, { D::RevSize, "reverb_size" }, { D::RevDamp, "reverb_damp" },
                                                    { D::LfoRate1, "lfo1_rate" }, { D::LfoRate1 + 3, "lfo4_rate" } };
            for (auto& kv : g) t.check (juce::String (GranulatorParamIds::kModDestIds[kv.first]) == kv.second, kv.second);
            bool allFloat = true;
            for (int d = 1; d < ogran::kModDests; ++d)
                allFloat = allFloat && dynamic_cast<juce::AudioParameterFloat*> (param (*proc, GranulatorParamIds::kModDestIds[d])) != nullptr;
            row (t.bad == 0 && allFloat, "U9b engine destination layout == kModDestIds (golden-pinned); every dest != None is Float",
                 t.detail (96 + 14));
        }

        // U9c — range parity: matrix base01 / convertFrom0to1 == the APVTS parameter, bitwise, 64 values per dest.
        {
            Tally t;
            for (int d = 1; d < ogran::kModDests; ++d)
            {
                auto* rp = param (*proc, GranulatorParamIds::kModDestIds[d]);
                const auto& r = M.range (d);
                for (int k = 0; k < 64; ++k)
                {
                    const float v = r.start + (r.end - r.start) * (float) k / 63.0f, n = (float) k / 63.0f;
                    const float a = r.convertTo0to1 (v), b = rp->convertTo0to1 (v);
                    const float c = r.convertFrom0to1 (n), e = rp->convertFrom0to1 (n);
                    t.check (std::memcmp (&a, &b, 4) == 0 && std::memcmp (&c, &e, 4) == 0, juce::String (GranulatorParamIds::kModDestIds[d]));
                }
            }
            row (t.bad == 0, "U9c range parity: matrix convertTo0to1 / convertFrom0to1 == APVTS bitwise (112 dests x 64)", t.detail (112 * 64));
        }

        auto gpWith = [] (std::initializer_list<std::tuple<int, int, int, float>> slots)
        {
            ogran::GridParams gp;
            for (auto& [m, src, dst, dep] : slots) { gp.mod[m].src = src; gp.mod[m].dest = dst; gp.mod[m].depth = dep; }
            return gp;
        };
        ogran::SourceValues one;
        for (auto& v : one.v) v = 1.0f;
        one.v[0] = 0.0f;
        const int lvl = D::stream (0, D::Level);

        // U9d — sum before the single clamp.
        {
            ogran::ModMatrix m = M;
            m.prepare (30, gpWith ({ { 0, 8, lvl, 0.8f }, { 1, 9, lvl, -0.8f } }));
            m.tick (gpWith ({ { 0, 8, lvl, 0.8f }, { 1, 9, lvl, -0.8f } }), one);
            const bool cancel = ! m.routed (lvl) && juce::exactlyEqual (m.offset (lvl), 0.0f);
            ogran::ModMatrix m2 = M;
            m2.prepare (30, gpWith ({ { 0, 8, lvl, 0.8f }, { 1, 9, lvl, 0.8f } }));
            m2.tick (gpWith ({ { 0, 8, lvl, 0.8f }, { 1, 9, lvl, 0.8f } }), one);
            const float raw = m2.apply (lvl, -12.0f, m2.offset (lvl));
            row (cancel && m2.routed (lvl) && juce::exactlyEqual (raw, 6.0f),
                 "U9d +80 % and -80 % on one dest -> offset 0 -> unrouted; +80 % + 80 % -> clamped at the range end",
                 juce::String::formatted ("off %g / %g, raw %g", m.offset (lvl), m2.offset (lvl), raw));
        }

        // U9e / U9en — residual (R49): a source change mid-ramp, then a dest stepping burst.
        {
            auto run = [&] (bool noRes, double& worstStep, bool& settled, int& changes)
            {
                ogran::ModMatrix m = M;
                m.noResidual = noRes;
                ogran::SourceValues src {};
                src.v[8] = 0.5f; src.v[9] = 0.2f;
                auto gp = gpWith ({ { 0, 8, lvl, 1.0f } });
                m.prepare (30, gp);
                std::vector<int> touched { lvl };
                std::array<float, ogran::kModDests> last {};
                worstStep = 0.0; changes = 0;
                for (int c = 0; c < 200; ++c)
                {
                    if (c == 40) gp.mod[0].src = 9;                                       // source change
                    if (c >= 100 && c < 110) { gp.mod[0].dest = D::stream (1, D::Level) + (c - 100); touched.push_back (gp.mod[0].dest); }
                    changes += m.tick (gp, src);
                    for (int d : touched)
                    {
                        if (c > 0)   // cell 0: the slot snapped at prepare (no fade at load)
                            worstStep = juce::jmax (worstStep, (double) std::abs (m.offset (d) - last[(size_t) d]));
                        last[(size_t) d] = m.offset (d);
                    }
                }
                settled = true;
                for (int d : touched)
                    if (d != gp.mod[0].dest) settled = settled && ! m.routed (d) && juce::exactlyEqual (m.residualValue (d), 0.0f);
            };
            double w, wn; bool s, sn; int ch, chn;
            run (false, w, s, ch);
            run (true, wn, sn, chn);
            const double bound = (0.5 + 0.2) / 30.0 + 1e-6;   // residual slope + the fade-in slope, worst sum
            row (w <= bound && s && ch == 11, "U9e residual: source change + 10 dest changes in 20 ms value-continuous; every residual exactly 0 after 20 ms",
                 juce::String::formatted ("max |d off| %.5f (bound %.5f), changes %d", w, bound, ch));
            row (wn >= 10.0 * bound - 1e-9 || wn >= 0.2 - 1e-6, "U9en noResidual: a route change steps the offset (>= 10x the slope bound)",
                 juce::String::formatted ("max |d off| %.4f", wn));
        }

        // U9f — engine <-> raw maps round-trip (R47 table).
        {
            namespace Mp = ogran::ModMap;
            Tally t;
            double worst = 0.0;
            auto chk = [&] (const char* what, float x, float y)
            {
                const double rel = std::abs ((double) y - x) / juce::jmax (1e-30, (double) std::abs (x));
                worst = juce::jmax (worst, rel);
                t.check (rel <= 8.0 * 1.1920929e-7, juce::String (what) + juce::String::formatted (" %.9g -> %.9g", x, y));
            };
            for (int k = 1; k <= 1000; ++k)
            {
                const float u = (float) k / 1000.0f;
                const float gIn = ogran::dbToGain (-24.0f + 36.0f * u);
                chk ("gain", gIn, ogran::dbToGain (Mp::gainToRaw (gIn)));
                const float gl = ogran::levelDbToGain (-59.0f + 65.0f * u);
                chk ("level", gl, ogran::levelDbToGain (Mp::levelToRaw (gl)));
                chk ("mix", u, (u * 100.0f) / 100.0f);
                chk ("pan", 2.0f * u - 1.0f + 1e-3f, ((2.0f * u - 1.0f + 1e-3f) * 100.0f) / 100.0f);
                const float k95 = ogran::kFbKnob * u;
                chk ("feedback", k95, Mp::fbKnobFromRaw (Mp::fbKnobToRaw (k95)));
                const float d = 1.0f + ogran::kSatSpan * u;
                chk ("saturation", d, Mp::satDFromRaw (Mp::satDToRaw (d)));
                const float lh = std::log2 (20.0f + 19980.0f * u);
                chk ("log2 Hz", lh, std::log2 (std::exp2 (lh)));
            }
            row (t.bad == 0, "U9f engine <-> raw maps round-trip (gain, level, mix, pan, fb x0.95, sat, diffusion, log2 Hz) <= 8 ulp",
                 t.detail (7000) + juce::String::formatted ("  worst %.2e rel", worst));
        }

        // U9g — the R46 control: a full round trip changes raw values on skewed ranges.
        {
            int changed = 0, levelChanged = 0, total = 0;
            for (int d = 1; d < ogran::kModDests; ++d)
            {
                const auto& r = M.range (d);
                for (int k = 0; k < 64; ++k)
                {
                    const float v = r.start + (r.end - r.start) * ((float) k + 0.37f) / 64.0f;
                    const float y = M.apply (d, v, 0.0f);
                    ++total;
                    if (! juce::exactlyEqual (y, v)) { ++changed; if (d == lvl) ++levelChanged; }
                }
            }
            row (changed > 0 && levelChanged > 0, "U9g full round trip (offset 0 through the matrix) changes raw values; the sparse form never runs it",
                 juce::String::formatted ("%d / %d values moved (s1_level %d / 64)", changed, total, levelChanged));
        }
    }
}}

static void engineRows25a()
{
    eg25::mockRows();
    eg25::divisionRows();
    eg25::trackerRows();
    eg25::tempoClockRows();
    eg25::lfoRows();
    eg25::envRows();
    eg25::matrixUnitRows();
}
