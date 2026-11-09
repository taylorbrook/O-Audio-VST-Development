/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.5a parity gates
    vs O-Granulator @ 82406fce (PLAN-2.5a T7, PD13). Included by main.cpp
    AFTER parity_gates_24.h (reuses pg22 / pg24 helpers in the same TU).

      PL0      no route, every source busy: null <= -130 dB (sets g_PL)
      PL0t     per-tick s1_level trace on the PL1 stimulus <= 1e-7 abs (the
               phase precondition: our prepare mirrors 82406fce :90-101, P4)
      PL1-15   global LFO rows (6 shapes, LFO 2-4, phase edits, Tempo x 3,
               LFO -> lfo1_rate, LFO -> output_gain)
      PLs1     LFO 1 -> s1_size: the spawn offsetAt latch — per-grain latched len
               == the parent grains.tsv (<= 1e-6 rel, every spawn, onsets equal);
               audio null INFO (S1 user decision 2026-11-09: the parent ramps
               g_out 20 ms, ours steps the law per cell; size moves the law)
      PM1-4    macro 1-4 -> s1_level, 0 -> 70 -> 20 (20 ms ramp)
      PE1-6    slot ramps and the residual shadow: depth flip, depth from 0,
               source swap, dest swap, source -> None, 4 stacked slots
               every non-control row: null <= -90 dB, |g / g_PL - 1| <= 1e-4
      PLc1-3   ours-only controls: lfo1_phase + 1 deg; seed + 1 (S&H); bpm + 0.1
      PMc1     macroRampZero;  PEc1 noDepthRamp;  PEc2 noResidual;  PEc3 depth -59
               each control must FAIL the -90 dB gate (or the band)

    Routes are mapped by NAME on each side (P2). Our side: PN01 geometry, the
    note-on at T_e (silent pre-roll so the LFO accumulators share the
    parent's absolute time), blocks cut at T_f / T_e like the parent driver.

  ==============================================================================
*/
#pragma once

namespace { namespace pg25a
{
    using eg21::Session;
    using eg21::P;
    using eg21::f2;
    using eg21::ctrl;
    using pg22::prow;
    using pg22::sid;
    using eg21::Proc;
    using eg21::setP;

    /** T12 lands the Tempo stream path: the Tempo rows (PL11-13, PLc3) run then. */
    constexpr bool kTempoStreamsReady = false;

    struct MockPlayHead final : public juce::AudioPlayHead
    {
        parity::Transport t;
        const juce::int64* pos = nullptr;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo pi;
            const juce::int64 at = pos != nullptr ? *pos : 0;
            const double ppq = parity::tpPpqAt (t, at, parity::kFs);
            pi.setBpm (parity::tpBpmAt (t, at));
            pi.setPpqPosition (ppq);
            pi.setIsPlaying (true);
            pi.setPpqPositionOfLastBarStart (std::floor (ppq / 4.0) * 4.0);
            pi.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
            return pi;
        }
    };

    int choiceIndex (Proc& p, const juce::String& id, const std::string& name)
    {
        auto* c = dynamic_cast<juce::AudioParameterChoice*> (param (p, id));
        return c != nullptr ? c->choices.indexOf (juce::String::fromUTF8 (name.c_str())) : -1;
    }

    struct Ours
    {
        std::vector<float> L, R;
        std::vector<double> trace;
        std::vector<ogi::GrainLogEntry> log;
        std::map<juce::String, float> raws;
        juce::String landed;
        bool namesOk = true;
    };

    /** Route params by name → our indices (into P, or applied live). */
    bool routeParams (Proc& probe, int slot, const std::string& src, const std::string& dest, bool setDepth, float depth,
                      P& out, juce::String& landed)
    {
        const juce::String b = "mod" + juce::String (slot) + "_";
        bool ok = true;
        if (! src.empty())
        {
            const int i = choiceIndex (probe, b + "source", src);
            ok = ok && i >= 0;
            out.push_back ({ b + "source", (float) i });
            landed << " " << b << "source=" << i;
        }
        if (! dest.empty())
        {
            const int i = choiceIndex (probe, b + "dest", dest);
            ok = ok && i >= 0;
            out.push_back ({ b + "dest", (float) i });
            landed << " " << b << "dest=" << i;
        }
        if (setDepth)
            out.push_back ({ b + "depth", depth });
        return ok;
    }

    Ours renderOurs (const parity::Row& row)
    {
        Ours o;
        auto probe = make();

        P params = pg22::globalsNull();
        for (auto& kv : pg22::streamParams (row.stream, parity::over (row.sp, row.oursOnly)))
            params.push_back (kv);
        for (const auto& kv : parity::over (row.gl, row.oursGl))
            params.push_back ({ juce::String (kv.id), kv.v });
        params.push_back ({ "freeze", 0.0f });
        for (const auto& rt : row.routes)
            o.namesOk = routeParams (*probe, rt.slot, rt.src, rt.dest, true, rt.depth, params, o.landed) && o.namesOk;

        Session ses (parity::kFs, params, row.seed, row.blockSize);
        ses.hk().coherenceOff = false;   // shipping law
        for (const auto& h : row.oursHooks)
        {
            if (h == "macroRampZero") ses.hk().macroRampZero = true;
            if (h == "noDepthRamp")   ses.hk().noDepthRamp = true;
            if (h == "noResidual")    ses.hk().noResidual = true;
            if (h == "tempoGridOnly") ses.hk().tempoGridOnly = true;
        }
        ses.hk().tempoGridOnly = true;   // the parent has no note-on burst (PD1)
        ses.load (row.stream - 1, pg23::materialBuf (row.matPeak), parity::kFs, "mat");

        // Blocks cut at T_f and T_e exactly like the parent driver.
        ses.at (parity::kTf, [] (Session&) {});
        ses.at (parity::kTe, [] (Session&) {});
        for (const auto& ev : row.events)
            eg22::setAt (ses, ev.at, juce::String (ev.id), ev.value);
        for (const auto& ev : row.oursEvents)
            eg22::setAt (ses, ev.at, juce::String (ev.id), ev.value);
        auto applyEdit = [&o, &probe, &ses] (const parity::RouteEdit& ed)
        {
            P live;
            o.namesOk = routeParams (*probe, ed.slot, ed.src, ed.dest, ed.setDepth, ed.depth, live, o.landed) && o.namesOk;
            ses.at (ed.at, [live] (Session& ss) { for (const auto& kv : live) setP (*ss.p, kv.first, kv.second); });
        };
        for (const auto& ed : row.edits)     applyEdit (ed);
        for (const auto& ed : row.oursEdits) applyEdit (ed);

        MockPlayHead mock;
        mock.t = row.tp;
        mock.pos = &ses.pos;
        if (row.tp.on)
            ses.p->setPlayHead (&mock);

        const juce::int64 len = (juce::int64) std::llround (row.seconds * parity::kFs);
        const juce::int64 end = parity::kTe + len;
        std::vector<float> tbuf;
        if (! row.traceDest.empty())
        {
            const int idx = choiceIndex (*probe, "mod1_dest", row.traceDest);
            o.namesOk = o.namesOk && idx >= 0;
            tbuf.assign ((size_t) (end / ogi::kCell + 64), 0.0f);
            ses.hk().traceDest = { idx, -1, tbuf.data(), (std::int64_t) tbuf.size(), 0 };
        }
        ses.noteOn (parity::kTe, parity::kOurRoot, 127);
        ses.run (end, row.blockSize);
        ses.p->setPlayHead (nullptr);
        if (! row.traceDest.empty())
        {
            for (std::int64_t i = 0; i < ses.hk().traceDest.n; ++i)
                o.trace.push_back ((double) tbuf[(size_t) i]);
            ses.hk().traceDest = {};
        }
        o.L.assign (ses.L.begin() + parity::kTe, ses.L.end());
        o.R.assign (ses.R.begin() + parity::kTe, ses.R.end());
        o.log = ses.hk().logInOrder();
        for (const auto& kv : row.gl)
            o.raws[juce::String (kv.id)] = eg21::rawOf (*ses.p, juce::String (kv.id));
        return o;
    }

    std::map<juce::String, float> readModRaw (const juce::String& rowId, juce::String& landedOut)
    {
        std::map<juce::String, float> m;
        juce::StringArray lines;
        pg22::gDir.getChildFile ("modraw_" + rowId + ".tsv").readLines (lines);
        for (const auto& l : lines)
        {
            if (l.startsWith ("#routes")) { landedOut = l.fromFirstOccurrenceOf ("#routes", false, false); continue; }
            juce::StringArray t;
            t.addTokens (l, "\t", "");
            if (t.size() >= 2) m[t[0]] = std::strtof (t[1].toRawUTF8(), nullptr);
        }
        return m;
    }

    std::vector<double> readF64 (const juce::String& name)
    {
        juce::MemoryBlock mb;
        if (! pg22::gDir.getChildFile (name).loadFileAsData (mb))
            return {};
        std::vector<double> v (mb.getSize() / sizeof (double));
        std::memcpy (v.data(), mb.getData(), v.size() * sizeof (double));
        return v;
    }

    bool rawsMatch (const std::map<juce::String, float>& par, const Ours& o, juce::String& why)
    {
        for (const auto& [id, v] : o.raws)
        {
            const auto it = par.find (id);
            if (it == par.end() || ! juce::exactlyEqual (it->second, v))
            {
                why = id + " ours " + juce::String (v, 9) + " parent " + (it == par.end() ? juce::String ("-") : juce::String (it->second, 9));
                return false;
            }
        }
        return true;
    }

    void run (const juce::File& dir, const juce::String& prefix)
    {
        pg22::gDir = dir;
        std::printf ("\n== Phase 2.5a parity gates (PL / PM / PE matrix nulls) vs O-Granulator @ 82406fce\n");
        const auto rs = parity::rows();
        const size_t n0 = 256;
        auto want = [&prefix] (const juce::String& id) { return prefix.isEmpty() || id.startsWith (prefix); };

        // PL0 first: g_PL.
        double gPL = 0.0;
        bool pl0Ok = false;
        {
            const auto* row = pg24::rowById (rs, "PL0");
            const auto par = pg22::readF32 ("PL0.f32");
            if (row == nullptr || par.empty())
                prow (false, "PL0 parent render present", "missing PL0.f32");
            else
            {
                const auto o = renderOurs (*row);
                const auto nl = pg24::nullIL (par, pg24::interleave (o.L, o.R), n0);
                juce::String pLanded, why;
                const bool raws = rawsMatch (readModRaw ("PL0", pLanded), o, why);
                gPL = nl.gain;
                pl0Ok = nl.resDb <= -130.0 && nl.refDb >= -70.0 && raws;
                if (want ("PL0"))
                    prow (pl0Ok, "PL0 DSP-06 parity precondition: no route, every source busy, vs parent @82406fce",
                          "meas residual " + f2 (nl.resDb, 1) + " dB (g_PL " + juce::String (gPL, 7) + ", ref " + f2 (nl.refDb, 1)
                              + " dBFS, n " + juce::String ((juce::int64) nl.n) + ", landed raws " + (raws ? "same" : "DIFFER: " + why)
                              + ") | thr <= -130 dB | margin " + f2 (-130.0 - nl.resDb, 1) + " | ctrl -- (gates PL / PM / PE)"
                              + (pl0Ok ? "" : " | S1: PL0 precondition failed"));
            }
        }

        // PL0t: the per-tick s1_level trace (phase alignment, P4).
        bool pl0tOk = false;
        {
            const auto* row = pg24::rowById (rs, "PL0t");
            const auto par = readF64 ("trace_PL0t.f64");
            if (row == nullptr || par.empty())
                prow (false, "PL0t parent trace present", "missing trace_PL0t.f64");
            else
            {
                const auto o = renderOurs (*row);
                const size_t n = std::min (par.size(), o.trace.size());
                double worst = 0.0;
                size_t at = 0;
                for (size_t i = 0; i < n; ++i)
                {
                    const double d = std::abs (par[i] - o.trace[i]);
                    if (d > worst) { worst = d; at = i; }
                }
                double span = 0.0, mn = 1e9, mx = -1e9;
                for (size_t i = 0; i < n; ++i) { mn = std::min (mn, par[i]); mx = std::max (mx, par[i]); }
                span = mx - mn;
                pl0tOk = n > 1000 && par.size() == o.trace.size() && worst <= 1.0e-7 && span > 0.1;
                if (want ("PL0t"))
                    prow (pl0tOk, "PL0t DSP-06 per-tick s1_level trace == parent (LFO 1 sine 3.7 Hz, depth 50; P4 phase precondition)",
                          "meas max |ours - parent| " + juce::String (worst, 3) + " at tick " + juce::String ((juce::int64) at) + " of "
                              + juce::String ((juce::int64) n) + " (ours " + juce::String ((juce::int64) o.trace.size()) + ", parent "
                              + juce::String ((juce::int64) par.size()) + " ticks; trace span " + f2 (span, 4) + ") | thr <= 1e-7 abs | "
                              + ctrl (span > 0.1, "the trace moves (span > 0.1 gain)") + (pl0tOk ? "" : " | S1: PL0t precondition failed"));
            }
        }

        std::printf ("  PL / PM / PE band table (g / g_PL - 1):\n");
        for (const auto& row : rs)
        {
            if (row.kind != parity::Kind::modNull || row.id == "PL0" || row.id == "PL0t") continue;
            const juce::String id (row.id);
            if (! want (id)) continue;
            if (row.deferred && ! kTempoStreamsReady)
            {
                std::printf ("  [INFO] %s deferred to T12 (sync_mode Tempo puts the parent streams on the R41 grid)\n", row.id.c_str());
                continue;
            }
            const bool isCtrl = id.startsWith ("PLc") || id.startsWith ("PMc") || id.startsWith ("PEc");
            const juce::String parId = row.parentOf.empty() ? id : juce::String (row.parentOf);
            const auto par = pg22::readF32 (parId + ".f32");
            if (par.empty())
            {
                prow (false, id + " parent render present", "missing " + parId + ".f32");
                continue;
            }
            const auto o = renderOurs (row);
            const auto nl = pg24::nullIL (par, pg24::interleave (o.L, o.R), n0);
            const double band = gPL != 0.0 ? nl.gain / gPL - 1.0 : 1e9;
            if (id == "PLs1")
            {
                // S1 re-specification (user 2026-11-09): the spawn latch, grain by grain.
                auto latch = [] (const std::vector<pg22::PGrain>& pgr, const std::vector<ogi::GrainLogEntry>& lg,
                                 double& worst, int& nCmp, int& onsetBad, double& lo, double& hi)
                {
                    worst = 0.0; nCmp = 0; onsetBad = 0; lo = 1e300; hi = -1e300;
                    std::vector<const ogi::GrainLogEntry*> mine;
                    for (const auto& e : lg)
                        if (e.stream == 0 && e.onsetSample >= parity::kTe) mine.push_back (&e);
                    const size_t n = std::min (pgr.size(), mine.size());
                    for (size_t i = 0; i < n; ++i)
                    {
                        onsetBad += pgr[i].onsetSample == mine[i]->onsetSample ? 0 : 1;
                        worst = std::max (worst, std::abs (mine[i]->len / pgr[i].len - 1.0));
                        lo = std::min (lo, pgr[i].len); hi = std::max (hi, pgr[i].len);
                        ++nCmp;
                    }
                    if (pgr.size() != mine.size()) onsetBad += 1000;
                };
                const auto pgr = pg22::readGrains (parId);
                double worst = 0, lo = 0, hi = 0; int nCmp = 0, bad = 0;
                latch (pgr, o.log, worst, nCmp, bad, lo, hi);
                parity::Row c = row;
                c.oursGl = { { "lfo1_phase", 1.0f } };
                const auto oc = renderOurs (c);
                double worstC = 0, loC = 0, hiC = 0; int nC = 0, badC = 0;
                latch (pgr, oc.log, worstC, nC, badC, loC, hiC);
                const bool moves = hi / std::max (lo, 1.0) > 1.01;
                const bool fired = worstC > 1.0e-6;
                juce::String pL2, why2;
                const bool rawsOk = rawsMatch (readModRaw (parId, pL2), o, why2);
                const bool ok = pl0Ok && pl0tOk && o.namesOk && rawsOk && nCmp >= 8 && bad == 0 && worst <= 1.0e-6 && moves && fired;
                std::printf ("    PLs1  latch: %d grains, max |len/len_par - 1| %.3e, len %.1f..%.1f samples; audio null INFO %.1f dB (g/g_PL-1 %+.3e)\n",
                             nCmp, worst, lo, hi, nl.resDb, band);
                prow (ok, "PLs1 DSP-06 spawn offsetAt latch: per-grain s1 len == parent grains.tsv (LFO 1 -> s1_size 10 %)",
                      "meas " + juce::String (nCmp) + " grains, max rel err " + juce::String (worst, 3) + ", onsets " + (bad == 0 ? "equal" : "DIFFER")
                          + ", len " + f2 (lo, 1) + ".." + f2 (hi, 1) + " (INFO audio null " + f2 (nl.resDb, 1)
                          + " dB: parent g_out 20 ms ramp vs our per-cell law) | thr <= 1e-6 rel | "
                          + ctrl (fired, "our lfo1_phase + 1 deg -> max rel err " + juce::String (worstC, 3)));
                continue;
            }
            juce::String pLanded, why;
            const auto pr = readModRaw (parId, pLanded);
            const bool raws = isCtrl || rawsMatch (pr, o, why);
            std::printf ("    %-5s g %.9f  g/g_PL-1 %+.3e  residual %.1f dB  ours%s | parent%s\n", row.id.c_str(), nl.gain, band, nl.resDb,
                         o.landed.toRawUTF8(), pLanded.toRawUTF8());
            const juce::String meas = "residual " + f2 (nl.resDb, 1) + " dB (g/g_PL - 1 " + juce::String (band, 3) + ", ref "
                                      + f2 (nl.refDb, 1) + " dBFS, n " + juce::String ((juce::int64) nl.n) + ")";
            if (! isCtrl)
            {
                const bool ok = pl0Ok && pl0tOk && o.namesOk && raws && nl.resDb <= -90.0 && std::abs (band) <= 1.0e-4 && nl.refDb >= -70.0;
                prow (ok, id + " DSP-06 matrix null vs parent @82406fce",
                      "meas " + meas + " | names " + (o.namesOk ? "mapped" : "UNMAPPED") + ", landed raws " + (raws ? "same" : "DIFFER: " + why)
                          + " | thr <= -90 dB, |band| <= 1e-4 | margin " + f2 (-90.0 - nl.resDb, 1) + " dB"
                          + (pl0Ok && pl0tOk ? "" : " | BLOCKED (PL0 / PL0t)"));
            }
            else
            {
                const bool fired = nl.resDb > -90.0 || std::abs (band) > 1.0e-4;
                const juce::String what = id == "PLc1" ? "PL1 + our lfo1_phase 1 deg"
                                        : id == "PLc2" ? "PL5 (S&H) with our seed + 1"
                                        : id == "PLc3" ? "PL11 with our mock bpm + 0.1"
                                        : id == "PMc1" ? "PM1 + macroRampZero"
                                        : id == "PEc1" ? "PE1 + noDepthRamp"
                                        : id == "PEc2" ? "PE3 + noResidual" : "PE1 with our depth -59";
                prow (fired, id + " DSP-06 matrix null control", "meas " + meas + " | " + ctrl (fired, what));
            }
        }
    }
}} // namespace ::pg25a
