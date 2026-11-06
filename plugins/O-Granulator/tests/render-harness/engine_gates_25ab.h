/*
  ==============================================================================

    O-Granulator — render harness, Stage 2 / Phase 2.5a matrix + PERF rows
    (PLAN-2.5a P58). Sibling of engine_gates_25a.h.

      MF1  ramped classes follow convertFrom0to1 (clamp (base01 + d·lfo)) (T8)
      MF2  a knob jump under a routed LFO keeps the 20 ms base ramp      (T8)
      MF3  spawn-latched classes: per-spawn lerped offset at the spawn    (T9)
      MF4  tick consumers (Free interval, quotas, g_out) read the cell value (T9)
      MF5  Tempo density x2^round(4·off) on the grid; size x2^(4·off)      (T9)
      TL1  telemetry == engine v01 / base01; no APVTS write              (T9)
      MF6 / MF7 / PH / ZQ                                                (T10)
      N8 / M5 / Z5 / C5                                                  (T11)

  ==============================================================================
*/
#pragma once

namespace { namespace eg25b
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg22::sp;
    using eg25::renderAll;
    constexpr double kFs = 48000.0;
    namespace D = ogran::ModDest;

    juce::String modId (int m, const char* s) { return "mod" + juce::String (m) + "_" + s; }

    /** Slot m: source → dest at depth (%). */
    P route (int m, int src, int dest, float depthPct)
    {
        return { { modId (m, "source"), (float) src }, { modId (m, "dest"), (float) dest }, { modId (m, "depth"), depthPct } };
    }
    P operator+ (P a, const P& b) { for (auto& kv : b) a.push_back (kv); return a; }

    /** One processor's per-cell source values (block 32: one cell per block). */
    struct Cells { std::vector<float> src[ogran::kModSources]; };
    std::shared_ptr<Cells> recordSources (Job& j)
    {
        auto c = std::make_shared<Cells>();
        auto prev = j.after;
        j.after = [c, prev] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const auto& e = p.getEngineForTest();
            for (int s = 1; s < ogran::kModSources; ++s) c->src[s].push_back (e.getSrcTelemetry (s));
        };
        return c;
    }

    /** {absolute sample → last traced value} of the processor's trace ring. */
    std::map<juce::int64, double> traceMap (const ogran::Engine& e)
    {
        std::map<juce::int64, double> m;
        const auto n = e.traceCount;
        const std::uint64_t from = n > (std::uint64_t) ogran::kTraceCapacity ? n - (std::uint64_t) ogran::kTraceCapacity : 0;
        for (std::uint64_t k = from; k < n; ++k)
        {
            const auto& t = e.trace[(size_t) (k % (std::uint64_t) ogran::kTraceCapacity)];
            m[t.at] = t.value;
        }
        return m;
    }
    std::vector<std::pair<juce::int64, double>> traceList (const ogran::Engine& e)
    {
        std::vector<std::pair<juce::int64, double>> v;
        const auto n = e.traceCount;
        const std::uint64_t from = n > (std::uint64_t) ogran::kTraceCapacity ? n - (std::uint64_t) ogran::kTraceCapacity : 0;
        for (std::uint64_t k = from; k < n; ++k)
        {
            const auto& t = e.trace[(size_t) (k % (std::uint64_t) ogran::kTraceCapacity)];
            v.push_back ({ t.at, t.value });
        }
        return v;
    }

    /** The independent expectation: the APVTS parameter's own range. */
    float applyParam (Proc& p, int dest, float raw, float off)
    {
        auto* rp = param (p, GranulatorParamIds::kModDestIds[dest]);
        const auto r = rp->getNormalisableRange();
        const float v01 = juce::jlimit (0.0f, 1.0f, r.convertTo0to1 (juce::jlimit (r.start, r.end, raw)) + off);
        return r.convertFrom0to1 (v01);
    }

    Job modJob (juce::int64 len, P extra, int traceDest)
    {
        Job j;
        j.params = eg22::base ({ 1 }) + P { { sp (1, "density"), 50.0f }, { sp (1, "size"), 60.0f }, { "lfo1_rate", 2.0f },
                                             { "lfo1_shape", 0.0f } } + extra;
        j.len = len;
        j.block = 32;
        j.stim = eg::noiseStim (0.2f);
        j.pre = [traceDest] (Proc& p) { p.getEngineForTest().hooks.traceDest.dest = traceDest; };
        return j;
    }

    //==========================================================================
    // MF1 / MF2 — ramped destinations (R47)
    //==========================================================================
    struct RampedCase { int dest; const char* id; float base; std::function<float (float raw)> toEngine; };

    std::vector<RampedCase> rampedCases()
    {
        using namespace ogran;
        auto pct = [] (float r) { return ModMap::pctFromRaw (r); };
        return {
            { D::InputGain,  "input_gain",  -6.0f,  [] (float r) { return dbToGain (r); } },
            { D::OutputGain, "output_gain", -3.0f,  [] (float r) { return dbToGain (r); } },
            { D::Mix,        "mix",         50.0f,  [] (float r) { return r / 100.0f; } },
            { D::stream (0, D::Level), "s1_level", -6.0f, [] (float r) { return levelDbToGain (r); } },
            { D::stream (0, D::Pan),   "s1_pan",   20.0f, [] (float r) { return r / 100.0f; } },
            { D::Feedback,   "feedback",    50.0f,  [] (float r) { return ModMap::fbKnobFromRaw (r); } },
            { D::FbSat,      "fb_saturation", 40.0f, [] (float r) { return ModMap::satDFromRaw (r); } },
            { D::FbDiff,     "fb_diffusion",  30.0f, pct },
            { D::FbLowCut,   "fb_low_cut",   200.0f, [] (float r) { return r; } },
            { D::FbHighCut,  "fb_high_cut", 5000.0f, [] (float r) { return r; } },
            { D::RevMix,     "reverb_mix",   40.0f, pct },
            { D::RevSize,    "reverb_size",  50.0f, pct },
            { D::RevDamp,    "reverb_damp",  60.0f, pct },
        };
    }

    void rampedRows()
    {
        std::printf ("\n-- MF1 / MF2. ramped destinations (R47: after the base ramp)\n");
        const auto cases = rampedCases();
        std::vector<Job> jobs;
        std::vector<std::shared_ptr<Cells>> cells;
        for (const auto& c : cases)
            for (float depth : { 50.0f, -50.0f })
            {
                Job j = modJob (48000, P { { c.id, c.base } } + route (1, 1, c.dest, depth), c.dest);
                cells.push_back (recordSources (j));
                jobs.push_back (j);
            }
        auto outs = renderAll (jobs);
        Tally t;
        double worst = 0.0;
        for (size_t k = 0; k < jobs.size(); ++k)
        {
            const auto& c = cases[k / 2];
            const float depth = outs[k].p->parameters.getRawParameterValue ("mod1_depth")->load() / 100.0f;   // the depth that landed
            const auto tm = traceMap (outs[k].eng());
            int bad = 0, n = 0;
            for (size_t cell = 1; cell < cells[k]->src[1].size(); ++cell)
            {
                const auto it = tm.find ((juce::int64) cell * 32);
                if (it == tm.end()) { ++bad; continue; }
                const float off = depth * cells[k]->src[1][cell];
                const float want = c.toEngine (applyParam (*outs[k].p, c.dest, c.base, off));
                const double rel = std::abs (it->second - (double) want) / juce::jmax (1e-6, std::abs ((double) want));
                worst = juce::jmax (worst, rel);
                if (rel > 1e-5) ++bad;
                ++n;
            }
            t.check (bad == 0 && n > 1000, juce::String (c.id) + (depth > 0 ? " +" : " -") + juce::String::formatted (": %d / %d off", bad, n));
        }
        row (t.bad == 0, "MF1 13 ramped dests x +-50 %: traced value == convertFrom0to1 (clamp (base01 + d*lfo)) (1e-5 rel)",
             t.detail ((int) jobs.size()) + juce::String::formatted ("  worst %.2e", worst));

        // MF2 — mix 20 → 80 % under a routed LFO: the base still ramps over 20 ms.
        {
            Job j = modJob (24000, P { { "mix", 20.0f } } + route (1, 1, D::Mix, 30.0f), D::Mix);
            j.events.push_back ({ 320 * 32, [] (Proc& p) { eg::setP (p, "mix", 80.0f); } });
            auto cs = recordSources (j);
            auto o = renderAll ({ j });
            const auto tm = traceMap (o[0].eng());
            ogran::ExactRamp base;
            base.reset (0.2f);
            int bad = 0;
            double firstStep = 0.0;
            for (int cell = 1; cell < 700; ++cell)
            {
                base.setTarget (cell >= 320 ? 0.8f : 0.2f, 30);
                base.tick();
                const float off = 0.3f * cs->src[1][(size_t) cell];
                const float want = applyParam (*o[0].p, D::Mix, base.current() * 100.0f, off) / 100.0f;
                const auto it = tm.find ((juce::int64) cell * 32);
                if (it == tm.end() || std::abs (it->second - want) > 1e-5 * juce::jmax (1e-3, (double) want)) ++bad;
                if (cell == 320 && it != tm.end()) firstStep = std::abs (it->second - tm.at (319 * 32));
            }
            row (bad == 0 && firstStep < 0.05, "MF2 mix 20 -> 80 % under a routed LFO: modulated = base ramp (30 cells) + offset, no 0.67 ms jump",
                 juce::String::formatted ("%d cells off, first-cell step %.4f (jump would be ~0.6)", bad, firstStep));
        }
    }

    //==========================================================================
    // MF3 — spawn-latched destinations (R48)
    //==========================================================================
    void latchedRows()
    {
        std::printf ("\n-- MF3. spawn-latched destinations (R48: offset lerped at the spawn sample)\n");
        struct C { int field; const char* name; float base; };
        const C cs[] {
            { D::Position, "position", 30.0f }, { D::Spray, "spray", 20.0f }, { D::Size, "size", 60.0f }, { D::SizeRand, "size_rand", 30.0f },
            { D::Density, "density", 50.0f }, { D::Jitter, "jitter", 30.0f }, { D::Pitch, "pitch", 3.0f }, { D::PitchRand, "pitch_rand", 2.0f },
            { D::ReverseProb, "reverse_prob", 40.0f }, { D::Window, "window", 0.4f }, { D::WindowSkew, "window_skew", 10.0f },
            { D::StereoSpread, "stereo_spread", 40.0f }, { D::GainRand, "gain_rand", 30.0f }, { D::FilterCutoff, "filter_cutoff", 2000.0f },
            { D::FilterRand, "filter_rand", 30.0f }, { D::FilterRes, "filter_res", 40.0f }, { D::Drive, "drive", 30.0f }, { D::Crush, "crush", 30.0f },
            { D::CrushProb, "crush_prob", 50.0f }, { D::RingFreq, "ring_freq", 300.0f }, { D::RingRand, "ring_rand", 30.0f },
            { D::RingMix, "ring_mix", 40.0f },
        };
        std::vector<Job> jobs;
        std::vector<std::shared_ptr<Cells>> cells;
        std::vector<int> dests;
        std::vector<float> bases, depths;
        auto add = [&] (int dest, const juce::String& id, float base, float depth)
        {
            Job j = modJob (36000, P { { id, base }, { sp (1, "filter_type"), 1.0f } } + route (1, 1, dest, depth), dest);
            cells.push_back (recordSources (j));
            jobs.push_back (j);
            dests.push_back (dest); bases.push_back (base); depths.push_back (depth / 100.0f);
        };
        for (const auto& c : cs)
            for (float depth : { 40.0f, -40.0f })
                add (D::stream (0, c.field), sp (1, c.name), c.base, depth);
        for (float depth : { 40.0f, -40.0f })
            add (D::BufferLength, "buffer_length", 8.0f, depth);
        auto outs = renderAll (jobs);

        Tally t;
        double worst = 0.0;
        int spawns = 0;
        for (size_t k = 0; k < jobs.size(); ++k)
        {
            const int d = dests[k];
            const auto tl = traceList (outs[k].eng());
            const auto& lfo = cells[k]->src[1];
            const float depth = outs[k].p->parameters.getRawParameterValue ("mod1_depth")->load() / 100.0f;   // the depth that landed
            int bad = 0;
            for (const auto& [at, v] : tl)
            {
                const size_t cell = (size_t) (at / 32);
                if (cell < 1 || cell >= lfo.size()) continue;
                const float p = depth * lfo[cell - 1], c = depth * lfo[cell];
                const float off = p + (c - p) * (float) ((double) (at % 32) / 32.0);
                const float landed = outs[k].p->parameters.getRawParameterValue (GranulatorParamIds::kModDestIds[d])->load();   // the raw that landed
                float raw = applyParam (*outs[k].p, d, landed, off);
                double want = raw;
                if (d != D::BufferLength)
                {
                    ogran::GridParams::Stream st;
                    ogran::ModMap::streamFieldFromRaw (st, (d - D::StreamBase) % D::StreamStride, raw);
                    want = ogran::ModMap::streamFieldValue (st, (d - D::StreamBase) % D::StreamStride);
                }
                const double rel = std::abs (v - want) / juce::jmax (1e-6, std::abs (want));
                worst = juce::jmax (worst, rel);
                if (rel > 1e-6)
                {
                    ++bad;
                }
                ++spawns;
            }
            t.check (bad == 0 && tl.size() >= 10, juce::String (GranulatorParamIds::kModDestIds[d]) + (depths[k] > 0 ? " +" : " -")
                                                      + juce::String::formatted (": %d / %d off", bad, (int) tl.size()));
        }
        row (t.bad == 0, "MF3 22 latched fields + buffer_length x +-40 %: per-spawn value == convertFrom0to1 (clamp (base01 + lerp (prev, cur, o/32))) (1e-6, landed raw + depth)",
             t.detail ((int) jobs.size()) + juce::String::formatted ("  %d spawns, worst %.2e", spawns, worst));
    }

    //==========================================================================
    // MF4 / MF5 — tick consumers; Tempo-mode modulation
    //==========================================================================
    void tickConsumerRows()
    {
        std::printf ("\n-- MF4 / MF5. tick consumers and Tempo-mode density / size modulation\n");
        // A constant source (macro 1 at 50 %) routed at depth d is equivalent to a knob at the
        // modulated value: the Free onset gaps, quotas and g_out targets must match that knob.
        Tally t;
        struct C { const char* name; int field; float base; float depth; };
        for (const C& c : { C { "density", D::Density, 20.0f, 30.0f }, C { "size", D::Size, 100.0f, 25.0f }, C { "spray", D::Spray, 10.0f, 40.0f } })
        {
            const int d = D::stream (0, c.field);
            P common = P { { sp (2, "enable"), 1.0f }, { sp (2, "density"), 80.0f }, { sp (2, "size"), 300.0f }, { "grain_limit", 0.0f },
                           { sp (1, "jitter"), 0.0f }, { "macro1", 50.0f } };
            Job a = modJob (48000, common + P { { sp (1, c.name), c.base } } + route (1, 8, d, c.depth), -1);
            auto proc = make();
            const float modRaw = applyParam (*proc, d, c.base, 0.5f * c.depth / 100.0f);
            Job b = modJob (48000, common + P { { sp (1, c.name), modRaw } }, -1);
            auto La = eg25::collectInto (a);
            auto Lb = eg25::collectInto (b);
            auto o = renderAll ({ a, b });
            const auto& ea = o[0].eng();
            const auto& eb = o[1].eng();
            const bool knobLanded = std::abs (eb.getGridParams().s[0].density - (c.field == D::Density ? modRaw : 50.0f)) < 1e-3f || c.field != D::Density;
            const double qa = ea.getQuotas().q[0], qb = eb.getQuotas().q[0];
            const double ga = ea.getGOutTarget (0), gb = eb.getGOutTarget (0);
            bool gapsOk = true;
            if (c.field == D::Density)
            {
                const auto ta = eg25::onsetTimes (*La), tb = eg25::onsetTimes (*Lb);
                for (size_t k = 5; k < 20 && k < ta.size() && k < tb.size(); ++k)
                    gapsOk = gapsOk && std::abs ((ta[k] - ta[k - 1]) - (tb[k] - tb[k - 1])) <= 1e-3 * (tb[k] - tb[k - 1]);
            }
            t.check (knobLanded && gapsOk && std::abs (qa - qb) <= 1e-4 * juce::jmax (1e-9, qb) && std::abs (ga - gb) <= 1e-4 * juce::jmax (1e-9, gb),
                     juce::String (c.name) + juce::String::formatted (": q %.6f/%.6f g_out %.6f/%.6f gaps %s", qa, qb, ga, gb, gapsOk ? "ok" : "differ"));
        }
        row (t.bad == 0, "MF4 tick consumers read the modulated cell value: Free gaps, quotas, g_out == a knob at the modulated value",
             t.detail (3));

        // MF5 — Tempo: +0.3 on density → ×2^round (1.2) = ×2 on the grid; size ×2^(4·0.25) = ×2.
        {
            Transport tr (kFs, 120.0);
            Job j = eg25::tempoJob (tr, 7, 4 * 48000);
            for (auto& kv : P { { "macro1", 30.0f }, { "macro2", 25.0f } } + route (1, 8, D::stream (0, D::Density), 100.0f)
                                 + route (2, 9, D::stream (0, D::Size), 100.0f) + P { { sp (1, "size_sync"), 7.0f } })
                j.params.push_back (kv);
            auto L = eg25::collectInto (j);
            auto o = renderAll ({ j });
            const auto got = eg25::onsetTimes (*L);
            int bad = 0;
            for (size_t k = 0; k < got.size(); ++k)
                if (std::abs (got[k] - 3000.0 * (double) k) > 1e-6) ++bad;
            const auto& e = o[0].eng();
            const double ms = e.getGridParams().s[0].sizeMs;
            row (bad == 0 && got.size() == 64 && std::abs (e.getTempoDiv (0) - 0.125) <= 1e-15 && std::abs (ms - 250.0) <= 1e-3,
                 "MF5 Tempo: density offset +0.3 -> x2 (1/16 -> 1/32 grid, on the grid <= 1e-6); size offset +0.25 -> x2 (125 -> 250 ms)",
                 juce::String::formatted ("%d onsets, %d off-grid, div %.4f, size %.3f ms", (int) got.size(), bad, e.getTempoDiv (0), ms));
        }
    }

    //==========================================================================
    // TL1 — telemetry; no APVTS writes
    //==========================================================================
    void telemetryRows()
    {
        std::printf ("\n-- TL1. telemetry (P55) and no APVTS writes\n");
        const int dl = D::stream (0, D::Level), dm = D::Mix, du = D::stream (1, D::Size);
        Job j = modJob (10 * 48000, P { { sp (1, "level"), -6.0f }, { "mix", 60.0f }, { sp (2, "size"), 300.0f } }
                                        + route (1, 1, dl, 40.0f) + route (2, 5, dm, -30.0f) + route (3, 8, D::Feedback, 20.0f)
                                        + P { { "macro1", 70.0f }, { "lfo1_rate", 5.0f } }, -1);
        j.block = 512;
        struct Snap { std::vector<float> before, after; };
        auto snap = std::make_shared<Snap>();
        auto mismatch = std::make_shared<int> (0);
        auto inRange = std::make_shared<bool> (true);
        auto pre = j.pre;
        j.pre = [snap, pre] (Proc& p) { if (pre) pre (p); for (auto* ap : p.getParameters()) snap->before.push_back (ap->getValue()); };
        j.after = [mismatch, inRange] (Proc& p, juce::int64)
        {
            const auto& e = p.getEngineForTest();
            const auto& M = e.getMatrix();
            for (int d : { dl, dm })
            {
                const float want = M.routed (d) ? M.v01 (d, M.rawNow (d), M.offset (d)) : M.range (d).convertTo0to1 (M.rawNow (d));
                if (! juce::exactlyEqual (e.getDestTelemetry (d), want)) ++*mismatch;
            }
            if (! juce::exactlyEqual (e.getDestTelemetry (du), M.range (du).convertTo0to1 (M.rawNow (du)))) ++*mismatch;
            for (int s = 1; s <= ogran::kSrcTelemetry; ++s)
            {
                const float v = e.getSrcTelemetry (s);
                *inRange = *inRange && std::isfinite (v) && v <= 1.0f && v >= (s <= 4 ? -1.0f : 0.0f);
            }
        };
        auto o = renderAll ({ j });
        for (auto* ap : o[0].p->getParameters()) snap->after.push_back (ap->getValue());
        bool unchanged = snap->before.size() == snap->after.size() && snap->before.size() == 220;
        for (size_t k = 0; unchanged && k < snap->before.size(); ++k)
            unchanged = std::memcmp (&snap->before[k], &snap->after[k], sizeof (float)) == 0;
        row (*mismatch == 0 && *inRange && unchanged && o[0].eng().diag.routedDests >= 3,
             "TL1 dest telemetry == v01 (routed) / base01 (unrouted); sources in range; 220 params bitwise unchanged over 10 s",
             juce::String::formatted ("%d mismatches, routed dests max %lld", *mismatch, (long long) o[0].eng().diag.routedDests));
    }

    //==========================================================================
    // Shared helpers for T10 / T11
    //==========================================================================
    /** The latched trace of a render vs convertFrom0to1 (clamp (base01 + lerp)). */
    void latchedCheck (const Out& o, const Cells& cells, int src, int d, int& bad, int& n, double& worst)
    {
        bad = 0; n = 0;
        const auto tl = traceList (o.eng());
        const auto& sv = cells.src[src];
        const float depth = o.p->parameters.getRawParameterValue ("mod1_depth")->load() / 100.0f;
        const float landed = o.p->parameters.getRawParameterValue (GranulatorParamIds::kModDestIds[d])->load();
        for (const auto& [at, v] : tl)
        {
            const size_t cell = (size_t) (at / 32);
            if (cell < 1 || cell >= sv.size()) continue;
            const float p = depth * sv[cell - 1], c = depth * sv[cell];
            const float off = p + (c - p) * (float) ((double) (at % 32) / 32.0);
            const float raw = applyParam (*o.p, d, landed, off);
            ogran::GridParams::Stream st;
            const int f = (d - D::StreamBase) % D::StreamStride;
            ogran::ModMap::streamFieldFromRaw (st, f, raw);
            const double want = ogran::ModMap::streamFieldValue (st, f);
            const double rel = std::abs (v - want) / juce::jmax (1e-6, std::abs (want));
            worst = juce::jmax (worst, rel);
            if (rel > 1e-6) ++bad;
            ++n;
        }
    }

    struct LfoObs { std::vector<double> pos; std::vector<int> mode; };
    std::shared_ptr<LfoObs> observeLfo (Job& j, int n)
    {
        auto ob = std::make_shared<LfoObs>();
        auto prev = j.after;
        j.after = [ob, prev, n] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const auto& l = p.getEngineForTest().getLfo (n);
            ob->pos.push_back (l.position());
            ob->mode.push_back (l.getMode() == ogran::Lfo::Mode::Direct ? 0 : 1);
        };
        return ob;
    }

    //==========================================================================
    // MF6 / MF7 — FUNC-06 coverage and route removal (T10)
    //==========================================================================
    void coverageRows()
    {
        std::printf ("\n-- MF6 / MF7. FUNC-06: every 2.5a source x class; route removal without a step\n");
        struct Cls { const char* name; int dest; float base; double baseEng, lo, hi; bool latched; };
        const int srcs[] { 1, 2, 3, 4, 5, 8, 9, 10, 11 };
        std::vector<Job> jobs;
        std::vector<std::shared_ptr<Cells>> cells;
        struct Meta { int src; Cls cls; float depth; };
        std::vector<Meta> meta;
        for (int src : srcs)
        {
            const int rateDest = D::LfoRate1 + (src >= 1 && src <= 4 ? src % 4 : 0);   // another LFO's rate
            const Cls classes[] {
                { "mix (ramped)", D::Mix, 50.0f, 0.5, 0.0, 1.0, false },
                { "s1_spray (latched)", D::stream (0, D::Spray), 30.0f, 0.3, 0.0, 1.0, true },
                { "s1_size (skewed)", D::stream (0, D::Size), 100.0f, 100.0, 5.0, 2000.0, true },
                { "reverb_size", D::RevSize, 50.0f, 0.5, 0.0, 1.0, false },
                { "lfo rate", rateDest, 1.0f, 1.0, 0.01, 20.0, false },
            };
            for (const auto& c : classes)
                for (float depth : { 100.0f, -100.0f })
                {
                    Job j = modJob (24000, P { { "macro1", 60.0f }, { "macro2", 60.0f }, { "macro3", 60.0f }, { "macro4", 60.0f },
                                               { "lfo2_rate", 3.0f }, { "lfo3_rate", 1.5f }, { "lfo4_rate", 4.0f }, { "lfo3_shape", 1.0f },
                                               { GranulatorParamIds::kModDestIds[c.dest], c.base } } + route (1, src, c.dest, depth), c.dest);
                    cells.push_back (recordSources (j));
                    jobs.push_back (j);
                    meta.push_back ({ src, c, depth });
                }
        }
        auto outs = renderAll (jobs);
        Tally t;
        for (size_t k = 0; k < jobs.size(); ++k)
        {
            const auto& m = meta[k];
            const auto tl = traceList (outs[k].eng());
            const auto& sv = cells[k]->src[m.src];
            int wrong = 0, out = 0, checked = 0;
            for (const auto& [at, v] : tl)
            {
                const size_t cell = (size_t) (at / 32);
                if (cell < 1 || cell >= sv.size()) continue;
                if (v < m.cls.lo - 1e-6 || v > m.cls.hi + 1e-6 || ! std::isfinite (v)) ++out;
                const float a = sv[cell], b = m.cls.latched ? sv[cell - 1] : sv[cell];
                if (std::abs (a) < 0.02f || std::abs (b) < 0.02f || (a > 0.0f) != (b > 0.0f)) continue;
                const double want = (m.depth > 0.0f ? 1.0 : -1.0) * (a > 0.0f ? 1.0 : -1.0);
                const double moved = v - m.cls.baseEng;
                if (! (moved * want > 0.0)) ++wrong;
                ++checked;
            }
            t.check (wrong == 0 && out == 0 && checked >= 5, juce::String::formatted ("src %d -> ", m.src) + m.cls.name + (m.depth > 0 ? " +" : " -")
                                                                 + juce::String::formatted (": %d wrong, %d out of range, %d checked", wrong, out, checked));
        }
        row (t.bad == 0, "MF6 9 sources (LFO 1-4, Env, Macro 1-4) x 5 classes x +-100 %: moves with sign(depth*src), stays in range",
             t.detail ((int) jobs.size()));

        // Pitch / Onset routed → identical to unrouted (2.5b).
        {
            Job a = modJob (24000, P {}, -1), b = modJob (24000, route (1, 6, D::Mix, 100.0f) + route (2, 7, D::stream (0, D::Pitch), -100.0f), -1);
            a.block = b.block = 512;
            auto o = renderAll ({ a, b });
            const auto dd = eg::diff (o[0], o[1]);
            row (dd.count == 0 && o[1].eng().diag.routedDests == 0, "MF6 Pitch / Onset routed at +-100 %: bitwise == unrouted (inert until 2.5b)",
                 juce::String::formatted ("%d diffs", dd.count));
        }

        // MF7 / MF7n — route removal (source → None, dest → None, depth → 0, dest swap).
        {
            const juce::int64 tR = 938 * 32;   // a tick
            const char* kinds[] { "source -> None", "dest -> None", "depth -> 0", "dest swap" };
            const int srcIds[] { 1, 5, 8 };
            std::vector<Job> mj;
            struct MM { int src, kind; bool noRes; };
            std::vector<MM> mm;
            for (int sid : srcIds)
                for (int kind = 0; kind < 4; ++kind)
                    for (bool noRes : { false, true })
                    {
                        if (noRes && ! (sid == 1 && kind == 0)) continue;
                        Job j = modJob (2 * 24000, P { { "mix", 50.0f }, { "macro1", 60.0f } } + route (1, sid, D::Mix, 60.0f), D::Mix);
                        j.events.push_back ({ tR, [kind] (Proc& p)
                        {
                            if (kind == 0) eg::setP (p, "mod1_source", 0.0f);
                            if (kind == 1) eg::setP (p, "mod1_dest", 0.0f);
                            if (kind == 2) eg::setP (p, "mod1_depth", 0.0f);
                            if (kind == 3) eg::setP (p, "mod1_dest", (float) D::stream (0, D::Pan));
                        } });
                        if (noRes)
                        {
                            auto pre = j.pre;
                            j.pre = [pre] (Proc& p) { pre (p); p.getEngineForTest().hooks.noResidual = true; };
                        }
                        mj.push_back (j);
                        mm.push_back ({ sid, kind, noRes });
                    }
            auto mouts = renderAll (mj);
            const double bound = 1.05 * (0.6 * 2.0 * eg::kPi * 2.0 / kFs) + 0.6 / (30.0 * 32.0) * 1.05;
            Tally t2;
            double nWorst = 0.0;
            for (size_t k = 0; k < mj.size(); ++k)
            {
                const auto tm = traceMap (mouts[k].eng());
                double worst = 0.0;
                bool rawAfter = true;
                for (juce::int64 c = tR / 32 - 200; c < tR / 32 + 200; ++c)
                {
                    const double a = tm.count ((c - 1) * 32) ? tm.at ((c - 1) * 32) : 0.0, b = tm.count (c * 32) ? tm.at (c * 32) : 0.0;
                    worst = juce::jmax (worst, std::abs (b - a) / 32.0);
                    if (c >= tR / 32 + 31) rawAfter = rawAfter && juce::exactlyEqual ((float) b, 0.5f);
                }
                if (mm[k].noRes) { nWorst = worst; continue; }
                t2.check (worst <= bound && rawAfter && ! mouts[k].eng().getMatrix().routed (D::Mix),
                          juce::String::formatted ("src %d ", mm[k].src) + kinds[mm[k].kind]
                              + juce::String::formatted (": max |dv|/sample %.3g (bound %.3g)%s", worst, bound, rawAfter ? "" : ", not raw after 20 ms"));
            }
            row (t2.bad == 0, "MF7 route removal (LFO / Env / Macro x 4 kinds): |dv| per sample <= P56 bound, then exactly the raw path",
                 t2.detail (12) + juce::String::formatted ("  bound %.3g", bound));
            row (nWorst > 2.0 * bound, "MF7n noResidual: source -> None steps past the bound", juce::String::formatted ("%.3g (bound %.3g)", nWorst, bound));

            // Secondary: step-click κ vs the same motion with the route held (440 Hz).
            Job held = modJob (2 * 24000, P { { "mix", 50.0f } } + route (1, 1, D::Mix, 60.0f), -1);
            held.stim = eg::sineStim (440.0, kFs, 0.5);
            Job rem = held;
            rem.events.push_back ({ tR, [] (Proc& p) { eg::setP (p, "mod1_source", 0.0f); } });
            auto ko = renderAll ({ held, rem });
            const double t0 = (double) tR / kFs - 0.03, t1 = (double) tR / kFs + 0.05;
            const double kh = eg24b::kappa (ko[0], t0, t1), kr = eg24b::kappa (ko[1], t0, t1);
            row (kr <= 2.0 * kh, "MF7 audio: removal step-click kappa <= 2.0x the held-route reference (440 Hz)",
                 juce::String::formatted ("%.2f vs %.2f (%.2fx)", kr, kh, kr / kh));
        }
    }

    //==========================================================================
    // PH1–PH3 — D15 / R50 phase under rate modulation (T10)
    //==========================================================================
    void phaseRows()
    {
        std::printf ("\n-- PH. LFO phase under rate modulation (D15 / R50)\n");
        const int r1 = D::LfoRate1;
        // PH1 Free: LFO2 (0.5 Hz) → LFO1 rate at ±100 %.
        {
            Tally t;
            double worstN = 0.0, maxInc = 0.0;
            for (float depth : { 100.0f, -100.0f })
                for (int force : { 0, 1 })
                {
                    Job j = modJob (3 * 48000, P { { "lfo1_rate", 1.0f }, { "lfo2_rate", 0.5f } } + route (1, 2, r1, depth), r1);
                    if (force)
                    {
                        auto pre = j.pre;
                        j.pre = [pre] (Proc& p) { pre (p); p.getEngineForTest().hooks.forceLfoMode = 1; };
                    }
                    auto ob = observeLfo (j, 0);
                    auto o = renderAll ({ j });
                    const auto tm = traceMap (o[0].eng());
                    double worst = 0.0;
                    // Tick n−1 advances the accumulator with the rate the matrix set at tick n−2.
                    for (size_t n = 3; n < ob->pos.size(); ++n)
                    {
                        const double rate = tm.at ((juce::int64) (n - 2) * 32);
                        const double want = 32.0 * rate / kFs;
                        maxInc = juce::jmax (maxInc, want);
                        const double d = ob->pos[n] - ob->pos[n - 1];
                        if (force) worstN = juce::jmax (worstN, std::abs (d));
                        else worst = juce::jmax (worst, std::abs (d - want));
                    }
                    if (! force) t.check (worst <= 1e-9, juce::String::formatted ("free %+.0f: max |dphi - inc| %.2e", depth, worst));
                }
            // Tempo: the route fades in at 1 s (Direct → Accum at the first modulated cell).
            {
                const juce::int64 tOn = 1500 * 32;
                Transport tr (kFs, 120.0);
                Job j = modJob (3 * 48000, P { { "sync_mode", 1.0f }, { "lfo1_rate_sync", 13.0f }, { "lfo2_rate", 0.5f } }
                                               + route (1, 2, r1, 0.0f), -1);
                eg25::withTransport (j, tr);
                j.events.push_back ({ tOn, [] (Proc& p) { eg::setP (p, "mod1_depth", 100.0f); } });
                auto cs = recordSources (j);
                auto ob = observeLfo (j, 0);
                auto o = renderAll ({ j });
                const double incD = 32.0 * (120.0 / 60.0 / kFs) / 1.0;
                ogran::ExactRamp dep;
                dep.reset (0.0f);
                const float landed = o[0].p->parameters.getRawParameterValue ("mod1_depth")->load() / 100.0f;
                double worst = 0.0;
                int directBefore = 0, accumAfter = 0;
                std::vector<float> offs (ob->pos.size(), 0.0f);
                for (size_t n = 0; n < ob->pos.size(); ++n)
                {
                    dep.setTarget ((juce::int64) n * 32 >= tOn ? landed : 0.0f, ogran::rampCellsFor (kFs, ogran::kModRampMs));
                    dep.tick();
                    offs[n] = dep.current() * cs->src[2][n];
                }
                for (size_t n = 2; n < ob->pos.size(); ++n)
                {
                    const double want = ob->mode[n] == 0 ? incD : incD * std::exp2 (4.0 * (double) offs[n - 2]);
                    if ((juce::int64) n * 32 < tOn) directBefore += ob->mode[n] == 0 ? 1 : 0;
                    else accumAfter += ob->mode[n] == 1 ? 1 : 0;
                    worst = juce::jmax (worst, std::abs ((ob->pos[n] - ob->pos[n - 1]) - want));
                }
                t.check (worst <= 1e-9 && directBefore == (int) (tOn / 32) - 2 && accumAfter > 0,
                         juce::String::formatted ("tempo: max |dphi - inc| %.2e, Direct %d cells before, Accum %d after", worst, directBefore, accumAfter));
            }
            row (t.bad == 0, "PH1 LFO2 -> LFO1 rate +-100 % (Free, Tempo): dphi per cell == the previous cell's modulated increment; Direct -> Accum continuous",
                 t.detail (3));
            row (worstN >= 10.0 * maxInc, "PH1n forceLfoMode Direct under the same modulation: phase jumps (>= 10x the max increment)",
                 juce::String::formatted ("max |dphi| %.3f vs inc %.2e", worstN, maxInc));
        }

        // PH2 — self and cross modulation: deterministic, finite, one-cell lag.
        {
            Tally t;
            for (int form = 0; form < 2; ++form)
            {
                P routes = form == 0 ? route (1, 1, r1, 80.0f) : route (1, 1, r1 + 1, 80.0f) + route (2, 2, r1, 80.0f);
                Job j = modJob (2 * 48000, P { { "lfo1_rate", 2.0f }, { "lfo2_rate", 3.0f } } + routes, r1);
                Job j2 = j;
                auto cs = recordSources (j);
                auto ob = observeLfo (j, 0);
                auto ob2 = observeLfo (j2, 0);
                auto o = renderAll ({ j, j2 });
                bool same = ob->pos.size() == ob2->pos.size(), finite = true;
                for (size_t n = 0; same && n < ob->pos.size(); ++n) same = std::memcmp (&ob->pos[n], &ob2->pos[n], sizeof (double)) == 0;
                const auto tm = traceMap (o[0].eng());
                const float depth = o[0].p->parameters.getRawParameterValue (form == 0 ? "mod1_depth" : "mod2_depth")->load() / 100.0f;
                const int srcOf1 = form == 0 ? 1 : 2;
                const float rate1 = o[0].p->parameters.getRawParameterValue ("lfo1_rate")->load();
                double worstLag = 0.0, worstMap = 0.0;
                for (size_t n = 3; n < ob->pos.size(); ++n)
                {
                    finite = finite && std::isfinite (ob->pos[n]);
                    const double rPrev = tm.at ((juce::int64) (n - 2) * 32);   // set at tick n−2, used by tick n−1's advance
                    worstLag = juce::jmax (worstLag, std::abs ((ob->pos[n] - ob->pos[n - 1]) - 32.0 * rPrev / kFs));
                    const float want = applyParam (*o[0].p, r1, rate1, depth * cs->src[srcOf1][n - 2]);
                    worstMap = juce::jmax (worstMap, std::abs (rPrev - (double) want) / (double) want);
                }
                t.check (same && finite && worstLag <= 1e-9 && worstMap <= 1e-6,
                         juce::String (form == 0 ? "LFO1 -> LFO1 rate" : "LFO1 <-> LFO2 rates")
                             + juce::String::formatted (": deterministic %s, lag err %.2e, rate map err %.2e", same ? "yes" : "NO", worstLag, worstMap));
            }
            row (t.bad == 0, "PH2 self / cross rate modulation: deterministic, finite, increment uses the PREVIOUS cell's matrix (one-cell lag)",
                 t.detail (2));
        }

        // PH3 — Accum → Direct only at a re-lock.
        {
            const juce::int64 tRem = 1500 * 32, tLoop = 3000 * 32;
            Transport tr (kFs, 120.0);
            tr.loopWrap (tLoop, 1.0, 9.0);
            Job j = modJob (5000 * 32, P { { "sync_mode", 1.0f }, { "lfo1_rate_sync", 11.0f }, { "lfo2_rate", 0.7f } } + route (1, 2, r1, 50.0f), -1);
            eg25::withTransport (j, tr);
            j.events.push_back ({ tRem, [] (Proc& p) { eg::setP (p, "mod1_depth", 0.0f); } });
            auto ob = observeLfo (j, 0);
            renderAll ({ j });
            int accumBetween = 0, between = 0, bad = 0, after = 0;
            const double pps = 120.0 / 60.0 / kFs, div = ogran::tempoDivBeats (11, 4.0);
            for (size_t n = 0; n < ob->pos.size(); ++n)
            {
                const juce::int64 t = (juce::int64) n * 32;
                if (t >= tRem + 40 * 32 && t < tLoop) { ++between; accumBetween += ob->mode[n]; }
                if (t >= tLoop)
                {
                    const double ppq = 1.0 + (double) (t - tLoop) * pps;
                    const double v = (ppq - 0.0) / div, c = std::floor (v), want = c + (v - c);
                    if (std::memcmp (&want, &ob->pos[n], sizeof (double)) != 0 || ob->mode[n] != 0) ++bad;
                    ++after;
                }
            }
            row (accumBetween == between && between > 1000 && bad == 0 && after > 1000,
                 "PH3 rate route removed: stays Accum until the loop re-lock; from the re-lock cell Direct, phase bitwise == the direct formula",
                 juce::String::formatted ("Accum %d / %d cells before the loop; %d / %d cells off after", accumBetween, between, bad, after));
        }
    }

    //==========================================================================
    // ZQ1 / ZQ2 — QUAL-03 zipper (T10, R57)
    //==========================================================================
    void zipperRows()
    {
        std::printf ("\n-- ZQ. QUAL-03: LFO sweeps of position / pitch / cutoff / mix\n");
        struct Dst { const char* id; int dest; float base; bool ramped; };
        const Dst dsts[] { { "mix", D::Mix, 50.0f, true }, { "s1_position", D::stream (0, D::Position), 40.0f, false },
                           { "s1_pitch", D::stream (0, D::Pitch), 0.0f, false }, { "s1_filter_cutoff", D::stream (0, D::FilterCutoff), 2000.0f, false } };
        std::vector<Job> jobs;
        std::vector<std::shared_ptr<Cells>> cells;
        struct M { int shape; double hz; float depth; Dst d; };
        std::vector<M> meta;
        for (int shape : { 0, 1 })
            for (double hz : { 0.1, 2.0, 20.0 })
                for (float depth : { 100.0f, -100.0f })
                    for (const auto& d : dsts)
                    {
                        Job j = modJob (36000, P { { d.id, d.base }, { sp (1, "filter_type"), 1.0f }, { "lfo1_shape", (float) shape },
                                                   { "lfo1_rate", (float) hz } } + route (1, 1, d.dest, depth), d.dest);
                        cells.push_back (recordSources (j));
                        jobs.push_back (j);
                        meta.push_back ({ shape, hz, depth, d });
                    }
        auto outs = renderAll (jobs);
        Tally tr, tl;
        double worstRatio = 0.0;
        for (size_t k = 0; k < jobs.size(); ++k)
        {
            const auto& m = meta[k];
            const juce::String name = juce::String (m.shape == 0 ? "sine " : "tri ") + juce::String (m.hz) + " Hz " + (m.depth > 0 ? "+" : "-") + m.d.id;
            if (m.d.ramped)
            {
                const auto tm = traceMap (outs[k].eng());
                const double rate = outs[k].p->parameters.getRawParameterValue ("lfo1_rate")->load();
                const double bound = 1.05 * std::abs (m.depth / 100.0) * (m.shape == 0 ? 2.0 * eg::kPi : 4.0) * rate / kFs;
                double worst = 0.0;
                for (juce::int64 c = 2; c < 36000 / 32; ++c)
                    worst = juce::jmax (worst, std::abs (tm.at (c * 32) - tm.at ((c - 1) * 32)) / 32.0);
                worstRatio = juce::jmax (worstRatio, worst / bound);
                tr.check (worst <= bound, name + juce::String::formatted (": %.3g vs %.3g", worst, bound));
            }
            else
            {
                int bad, n; double w = 0.0;
                latchedCheck (outs[k], *cells[k], 1, m.d.dest, bad, n, w);
                tl.check (bad == 0 && n >= 10, name + juce::String::formatted (": %d / %d grains off", bad, n));
            }
        }
        row (tr.bad == 0, "ZQ1 mix (ramped) sine / tri 0.1 / 2 / 20 Hz +-100 %: max |dv| per sample <= 1.05 x the analytic slope",
             tr.detail (12) + juce::String::formatted ("  worst %.3f of the bound", worstRatio));
        row (tl.bad == 0, "ZQ1 position / pitch / cutoff (latched): each grain == the source at its spawn sample (no within-grain step)",
             tl.detail (36));

        // ZQ2 / ZQ2n — the audio: 440 Hz, a 20 Hz sine at 100 % on mix vs a same-motion
        // reference built from separate dry and wet renders with a per-sample smooth mix;
        // the control steps mix per cell (no CellLerp). Latched dests vs their depth-0 render.
        {
            auto sine = eg::sineStim (440.0, kFs, 0.5);
            Job mod = modJob (72000, P { { "mix", 50.0f }, { "lfo1_rate", 20.0f } } + route (1, 1, D::Mix, 100.0f), -1);
            Job dry = modJob (72000, P { { "mix", 0.0f } }, -1), wet = modJob (72000, P { { "mix", 100.0f } }, -1);
            for (Job* j : { &mod, &dry, &wet }) j->stim = sine;
            auto cs = recordSources (mod);
            auto o = renderAll ({ mod, dry, wet });
            const double rate = o[0].p->parameters.getRawParameterValue ("lfo1_rate")->load();
            Out ref, step;
            ref.L.resize (o[0].L.size()); ref.R.resize (o[0].R.size());
            step.L.resize (o[0].L.size()); step.R.resize (o[0].R.size());
            for (size_t i = 0; i < o[0].L.size(); ++i)
            {
                // The engine's cell value at cell n is the LFO at 32·n, reached one cell later (CellLerp).
                const double ts = (double) i - 32.0;
                const double m = juce::jlimit (0.0, 1.0, 0.5 + std::sin (2.0 * eg::kPi * rate * juce::jmax (0.0, ts) / kFs));
                const size_t cell = i / 32;
                const double ms = juce::jlimit (0.0, 1.0, 0.5 + (double) cs->src[1][cell]);
                auto mixAt = [] (double mm, double d, double w) { return d * std::cos (0.5 * eg::kPi * mm) + w * std::sin (0.5 * eg::kPi * mm); };
                ref.L[i]  = (float) mixAt (m,  o[1].L[i], o[2].L[i]);
                ref.R[i]  = (float) mixAt (m,  o[1].R[i], o[2].R[i]);
                step.L[i] = (float) mixAt (ms, o[1].L[i], o[2].L[i]);
                step.R[i] = (float) mixAt (ms, o[1].R[i], o[2].R[i]);
            }
            const double km = eg24b::kappa (o[0], 0.25, 1.45), kr = eg24b::kappa (ref, 0.25, 1.45), ks = eg24b::kappa (step, 0.25, 1.45);
            row (km <= 2.0 * kr, "ZQ2 mix 20 Hz sine 100 % (440 Hz): kappa <= 2.0x the smooth same-motion reference",
                 juce::String::formatted ("%.2f vs %.2f (%.2fx)", km, kr, km / kr));
            row (ks > 2.0 * kr, "ZQ2n per-cell mix steps (no CellLerp) exceed 2.0x: the gate is not vacuous at this stimulus",
                 juce::String::formatted ("%.2f vs %.2f (%.2fx)", ks, kr, ks / kr));

            // Latched destinations: the same-motion reference is the same PER-GRAIN variety
            // without the matrix (pitch_rand 24 st, spray 100 %, filter_rand 100 %) — a
            // depth-0 render is not one (pitch spread alone raises curvature by f²).
            Tally t;
            // The spans match: position 40 % ± 100 % → [0, 1] vs spray 100 %; pitch 0 ± 100 % → ±24 st vs
            // pitch_rand 24; cutoff 2 kHz ± 20 % (normalised, skewed) → 312–7918 Hz vs filter_rand 100 % (±2 oct).
            struct LR { const Dst* d; P ref; float depth; };
            const LR refs[] { { &dsts[1], P { { sp (1, "spray"), 100.0f } }, 100.0f }, { &dsts[2], P { { sp (1, "pitch_rand"), 24.0f } }, 100.0f },
                              { &dsts[3], P { { sp (1, "filter_rand"), 100.0f } }, 20.0f } };
            std::vector<Job> lj;
            for (const auto& r : refs)
            {
                Job a = modJob (72000, P { { r.d->id, r.d->base }, { sp (1, "filter_type"), 1.0f }, { "lfo1_rate", 20.0f } } + route (1, 1, r.d->dest, r.depth), -1);
                Job b = modJob (72000, P { { r.d->id, r.d->base }, { sp (1, "filter_type"), 1.0f } } + r.ref, -1);
                a.stim = b.stim = sine;
                lj.push_back (a);
                lj.push_back (b);
            }
            auto lo = renderAll (lj);
            for (size_t li = 0; li < 3; ++li)
            {
                const double a = eg24b::kappa (lo[2 * li], 0.25, 1.45), b = eg24b::kappa (lo[2 * li + 1], 0.25, 1.45);
                t.check (a <= 2.0 * b, juce::String (refs[li].d->id) + juce::String::formatted (" %.3f vs %.3f", a, b));
            }
            row (t.bad == 0, "ZQ2 position / pitch / cutoff 20 Hz sine (440 Hz): kappa <= 2.0x the same per-grain span (spray / pitch_rand / filter_rand)",
                 t.detail (3));
        }
    }

    //==========================================================================
    // N8 / M5 / Z5 / C5 — PERF-03, PERF-01, QUAL-02, PERF-02 (T11)
    //==========================================================================
    /** The N7 final recipe (all FX, feedback 80 %, freeze toggles, reverb) + Tempo
        sync, every LFO shape, env, macro ramps, 16 live slots, route edits incl. a
        dest-stepping burst inside 20 ms, and a v2 transport with a tempo step, a
        loop, a locate and a 3/4 change. Every event lands on a SHARED boundary. */
    Job n8Base (bool m5Extras = false)
    {
        Job b = eg24b::n7Base (2);
        b.params = b.params + P {
            { "sync_mode", 1.0f },
            { sp (1, "density_sync"), 0.0f }, { sp (2, "density_sync"), 3.0f }, { sp (3, "density_sync"), 1.0f }, { sp (4, "density_sync"), 6.0f },
            { sp (1, "size_sync"), 7.0f }, { sp (2, "size_sync"), 10.0f }, { sp (3, "size_sync"), 5.0f }, { sp (4, "size_sync"), 13.0f },
            { "lfo1_shape", 0.0f }, { "lfo1_rate", 3.0f }, { "lfo2_shape", 1.0f }, { "lfo2_rate_sync", 13.0f },
            { "lfo3_shape", 4.0f }, { "lfo3_rate", 7.0f }, { "lfo4_shape", 5.0f }, { "lfo4_rate", 0.7f },
            { "env_attack", 5.0f }, { "env_release", 120.0f }, { "macro2", 40.0f } };
        b.params = b.params + route (1, 1, D::Mix, 30.0f) + route (2, 2, D::stream (0, D::Position), 40.0f) + route (3, 3, D::LfoRate1, 50.0f)
                 + route (4, 4, D::RevSize, 60.0f) + route (5, 5, D::stream (1, D::FilterCutoff), -40.0f) + route (6, 8, D::stream (2, D::Pitch), 30.0f)
                 + route (7, 1, D::stream (3, D::Size), 20.0f) + route (8, 2, D::stream (0, D::Density), 25.0f) + route (9, 3, D::FbSat, 40.0f)
                 + route (10, 4, D::stream (1, D::Pan), 50.0f) + route (11, 5, D::OutputGain, -20.0f) + route (12, 9, D::Feedback, 20.0f)
                 + route (13, 1, D::BufferLength, 30.0f) + route (14, 2, D::FbLowCut, 30.0f) + route (15, 3, D::stream (2, D::RingMix), 50.0f)
                 + route (16, 4, D::InputGain, 20.0f);
        auto ev = [&b] (juce::int64 at, std::function<void (Proc&)> f) { b.events.push_back ({ at, std::move (f) }); };
        ev (72011,  [] (Proc& p) { eg::setP (p, "macro1", 80.0f); });
        ev (96013,  [] (Proc& p) { eg::setP (p, "lfo1_shape", 3.0f); });
        ev (105601, [] (Proc& p) { eg::setP (p, "mod5_dest", (float) D::stream (2, D::FilterCutoff)); });
        ev (120007, [] (Proc& p) { eg::setP (p, "mod9_source", 8.0f); });
        ev (144019, [] (Proc& p) { eg::setP (p, "lfo2_shape", 2.0f); });
        for (int k = 0; k < 10; ++k)   // dest-stepping burst: 10 changes in 10 cells (< 20 ms)
            ev (152000 + 32 * k + 5, [k] (Proc& p) { eg::setP (p, "mod16_dest", (float) (D::stream (0, D::Level) + k)); });
        ev (200003, [] (Proc& p) { eg::setP (p, "mod1_depth", -60.0f); eg::setP (p, "mod7_source", 0.0f); });
        if (m5Extras)
        {
            ev (180001, [] (Proc& p) { eg::setP (p, "sync_mode", 0.0f); });
            ev (190003, [] (Proc& p) { eg::setP (p, "sync_mode", 1.0f); eg::setP (p, "lfo2_rate_sync", 9.0f); });
            ev (210011, [] (Proc& p) { eg::setP (p, "mod2_source", 3.0f); eg::setP (p, "mod3_dest", (float) D::stream (3, D::Spray)); });
        }
        Transport tr (kFs, 120.0);
        tr.tempo (48017, 97.3).loopWrap (96005, 2.0, 10.0).locate (168003, 21.5).timeSig (216011, 3, 4);
        if (m5Extras) tr.stop (240007).start (252001);
        eg25::withTransport (b, tr);
        return b;
    }

    void blockNullRows25()
    {
        std::printf ("\n-- N8. PERF-03 block-size null under modulation + tempo (value compare)\n");
        Job base = n8Base();
        auto vdiff = [] (const Out& a, const Out& b)
        {
            int n = 0;
            for (size_t i = 0; i < a.L.size() && i < b.L.size(); ++i)
                if ((double) a.L[i] != (double) b.L[i] || (double) a.R[i] != (double) b.R[i]) ++n;
            return n;
        };
        std::vector<Job> jobs;
        for (int b : { 32, 441, 512, 4096, 0 }) { Job j = base; j.block = b; jobs.push_back (j); }
        Job seed = base; seed.block = 32; seed.seed ^= 0x1ull; jobs.push_back (seed);
        for (int b : { 32, 441 })
        {
            Job j = base; j.block = b;
            auto pre = j.pre;
            j.pre = [pre] (Proc& p) { if (pre) pre (p); p.getEngineForTest().hooks.perBlockOrigin = true; };
            jobs.push_back (j);
        }
        auto o = renderAll (jobs);
        Tally t;
        const int blocks[] { 441, 512, 4096, 0 };
        for (int k = 1; k <= 4; ++k)
        {
            const int n = vdiff (o[0], o[(size_t) k]);
            t.check (n == 0, eg25::blockName (blocks[k - 1]) + juce::String::formatted (": %d diffs", n));
        }
        const auto& d = o[0].eng().diag;
        const bool live = d.tempoReanchors > 0 && d.tempoRelocks > 0 && d.routeChanges > 0 && d.routedDests > 10 && d.spawns > 500
                          && d.freezeEngages > 0 && d.reverbCells > 0 && d.tempoOnsets > 100;
        row (t.bad == 0 && live, "N8 PERF-03 under modulation + tempo: 441/512/4096/var vs 32 max|d| == 0 (value compare)",
             t.detail (4) + juce::String::formatted ("  [re-anchors %lld re-locks %lld route changes %lld routed max %lld tempo onsets %lld spawns %lld]",
                                                     (long long) d.tempoReanchors, (long long) d.tempoRelocks, (long long) d.routeChanges,
                                                     (long long) d.routedDests, (long long) d.tempoOnsets, (long long) d.spawns)
                 + (live ? "" : "  VACUOUS"));
        row (vdiff (o[0], o[5]) > 0, "N8c different-seed control differs", juce::String (vdiff (o[0], o[5])) + " diffs");
        row (vdiff (o[6], o[7]) > 0, "N8n perBlockOrigin (R39) breaks the null on this recipe (the anchor matters)",
             juce::String (vdiff (o[6], o[7])) + " diffs (32 vs 441)");
    }

    void allocRows25()
    {
        std::printf ("\n-- M5. PERF-01 allocations across matrix / transport edits\n");
       #if JUCE_MAC
        using namespace eg;
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocCount = 0;
        allocTrace = false;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const int liveness = allocCount;
        allocCount = 0;
        allocTrace = true;
        Job j = n8Base (true);
        j.len = 300000;
        j.block = 512;
        auto prevAfter = j.after;
        armDuringProcess = false;
        j.after = [prevAfter] (Proc& p, juce::int64 pos) { if (prevAfter) prevAfter (p, pos); armDuringProcess = true; };
        const auto o = render (j);
        armDuringProcess = false;
        malloc_logger = nullptr;
        allocTrace = false;
        const int n = allocCount;
        const auto& d = o.eng().diag;
        const bool live = d.routeChanges >= 14 && d.tempoRelocks >= 2 && d.freezeEngages > 0 && d.routedDests > 10;
        row (liveness == 1, "M5 liveness: a deliberate malloc on the render thread counts exactly 1", juce::String (liveness));
        row (liveness == 1 && n == 0 && live, "M5 0 allocations: route edits / swaps / burst, sync toggles, rate_sync, start / stop / loop / locate / tempo / 3/4",
             juce::String::formatted ("%d allocs; route changes %lld, re-locks %lld, re-anchors %lld", n, (long long) d.routeChanges,
                                      (long long) d.tempoRelocks, (long long) d.tempoReanchors) + (live ? "" : "  VACUOUS"));
       #else
        row (false, "M5 alloc gate is macOS-only");
       #endif
    }

    void stressRows25()
    {
        std::printf ("\n-- Z5. QUAL-02 matrix stress (16 slots +-100 %%, 20 Hz S&H / square, 0 dBFS env)\n");
        const double rates[3] { 48000.0, 44100.0, 192000.0 };
        struct ZR { bool finite = true; int sub = 0, scans = 0; double peak = 0.0, wetPeak = 0.0; long long scrubs = 0, nanKills = 0, envScrubs = 0, spawns = 0, routed = 0; };
        std::vector<ZR> rs (3);
        std::vector<Job> jobs; std::vector<double> cost;
        for (int i = 0; i < 3; ++i)
        {
            const double fs = rates[i];
            Job j; j.fs = fs; j.len = (juce::int64) (60.0 * fs); j.block = 512;
            j.params = eg22::base ({ 1, 2, 3, 4 }) + P { { "grain_limit", 3.0f }, { "mix", 100.0f }, { "feedback", 60.0f }, { "reverb_mix", 20.0f },
                                                       { "lfo1_shape", 4.0f }, { "lfo1_rate", 20.0f }, { "lfo2_shape", 3.0f }, { "lfo2_rate", 20.0f },
                                                       { "lfo3_shape", 4.0f }, { "lfo3_rate", 13.0f }, { "lfo4_shape", 3.0f }, { "lfo4_rate", 17.0f },
                                                       { "macro1", 100.0f } };
            for (int s = 1; s <= 4; ++s)
                j.params = j.params + P { { sp (s, "density"), 100.0f }, { sp (s, "size"), 300.0f }, { sp (s, "filter_type"), (float) (1 + s % 3) },
                                          { sp (s, "filter_res"), 70.0f }, { sp (s, "drive"), 40.0f }, { sp (s, "ring_mix"), 30.0f } };
            const int dests[16] { D::stream (0, D::Level), D::stream (1, D::Level), D::stream (2, D::Level), D::stream (3, D::Level),
                                  D::stream (0, D::Pitch), D::stream (1, D::Density), D::Feedback, D::FbSat, D::stream (0, D::FilterRes),
                                  D::stream (1, D::Drive), D::stream (2, D::RingMix), D::RevMix, D::OutputGain, D::Mix,
                                  D::stream (3, D::FilterRes), D::stream (2, D::Pitch) };
            const int srcs[16] { 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 8, 3, 4, 5 };
            for (int m = 0; m < 16; ++m)
                j.params = j.params + route (m + 1, srcs[m], dests[m], m % 2 == 0 ? 100.0f : -100.0f);
            j.stim = eg::noiseStim (1.0f);
            auto* r = &rs[(size_t) i];
            j.after = [r, len = j.len, fs] (Proc& p, juce::int64 pos)
            {
                const juce::int64 sect = (juce::int64) (20.0 * fs);
                if (pos % sect < 512 || pos >= len)
                {
                    const auto& e = p.getEngineForTest();
                    for (int k = 0; k < ogran::kPoolLive + ogran::kPoolFade; ++k)
                    {
                        ++r->scans;
                        if (eg23b::anySubnormal (e.getPool().getGrains()[(size_t) k])) ++r->sub;
                    }
                    auto sub = [] (float x) { return std::fpclassify (x) == FP_SUBNORMAL; };
                    const auto& c = e.getChainForTest();
                    for (int ch = 0; ch < 2; ++ch)
                        if (sub (c.hpIc1[ch]) || sub (c.hpIc2[ch]) || sub (c.lpIc1[ch]) || sub (c.lpIc2[ch])) ++r->sub;
                    if (sub (c.env) || sub (e.getEnv().state())) ++r->sub;
                    for (int s = 1; s < ogran::kModSources; ++s) if (sub (e.getSources().v[s])) ++r->sub;
                }
            };
            jobs.push_back (j);
            cost.push_back (fs);
        }
        eg22b::renderPool (jobs, cost, [&] (size_t i, Out& o)
        {
            auto& r = rs[i];
            for (size_t k = 0; k < o.L.size(); ++k)
            {
                if (! std::isfinite (o.L[k]) || ! std::isfinite (o.R[k])) r.finite = false;
                r.peak = juce::jmax (r.peak, (double) std::abs (o.L[k]), (double) std::abs (o.R[k]));
            }
            const auto& d = o.eng().diag;
            r.scrubs = d.fbScrubs; r.nanKills = d.nanKills; r.envScrubs = d.envScrubs; r.spawns = d.spawns; r.routed = d.routedDests;
            r.wetPeak = d.maxWetPeak;
        });
        bool ok = true;
        juce::String det;
        for (int i = 0; i < 3; ++i)
        {
            const auto& r = rs[(size_t) i];
            // output_gain (+12 dB) and mix are routed: out <= (|dry|·dg + A·wg)·og <= sqrt (1 + A²)·dbToGain (12) with 0 dBFS dry.
            const double outBound = std::sqrt (1.0 + (double) ogran::kKneeA * ogran::kKneeA) * (double) ogran::dbToGain (12.0f) * (1.0 + 1e-6);
            ok = ok && r.finite && r.sub == 0 && r.wetPeak <= (double) ogran::kKneeA && r.peak <= outBound && r.scrubs == 0 && r.nanKills == 0
                 && r.envScrubs == 0 && r.spawns > 1000 && r.routed >= 15;
            det << juce::String::formatted ("%g k: wet pk %.4f out pk %.4f (<= %.3f) sub %d/%d scrubs %lld nanKills %lld envScrubs %lld; ", rates[i] / 1000.0,
                                            r.wetPeak, r.peak, outBound, r.sub, r.scans, r.scrubs, r.nanKills, r.envScrubs);
        }
        row (ok, "Z5 60 s, 16 slots +-100 % @ 48/44.1/192 k: finite, no subnormal, wet (post-knee) <= float(A), out <= its gain bound, 0 scrubs / kills", det);
    }

    void cpuRows25()
    {
        std::printf ("\n-- C5. CPU with 16 slots + 4 LFOs + env (INFO; budget gate in 2.6)\n");
        P c4 { { "mix", 100.0f }, { "grain_limit", 3.0f }, { "feedback", 100.0f }, { "fb_diffusion", 100.0f }, { "reverb_mix", 50.0f } };
        for (int s = 1; s <= 4; ++s)
            c4 = c4 + P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 16.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "window"), 0.6f },
                          { sp (s, "scale"), 1.0f }, { sp (s, "pitch_rand"), 2.0f }, { sp (s, "filter_type"), 1.0f }, { sp (s, "filter_res"), 100.0f },
                          { sp (s, "filter_rand"), 100.0f }, { sp (s, "drive"), 100.0f }, { sp (s, "crush"), 100.0f }, { sp (s, "crush_prob"), 100.0f },
                          { sp (s, "ring_mix"), 100.0f }, { sp (s, "ring_rand"), 100.0f } };
        P c5 = c4 + P { { "lfo1_rate", 3.0f }, { "lfo2_shape", 5.0f }, { "lfo3_shape", 4.0f }, { "lfo3_rate", 9.0f }, { "lfo4_rate", 0.3f },
                        { "macro1", 50.0f } };
        const int dests[16] { D::Mix, D::stream (0, D::Position), D::stream (1, D::Pitch), D::stream (2, D::FilterCutoff), D::stream (3, D::Spray),
                              D::Feedback, D::FbSat, D::RevSize, D::stream (0, D::Level), D::stream (1, D::Pan), D::stream (2, D::Size),
                              D::stream (3, D::Density), D::InputGain, D::OutputGain, D::LfoRate1 + 1, D::stream (0, D::RingMix) };
        for (int m = 0; m < 16; ++m)
            c5 = c5 + route (m + 1, 1 + m % 5 + (m % 5 == 4 ? 0 : 0), dests[m], m % 2 == 0 ? 30.0f : -30.0f);
        int a4, a5;
        const double c = eg23b::timeConfig (c4, a4), c5v = eg23b::timeConfig (c5, a5);
        std::printf ("  [INFO] C4 re-run: %.2f %% of one core (live %d; 2.4 verify 17.75 %%)\n", c, a4);
        std::printf ("  [INFO] C5 = C4 + 16 slots + 4 LFOs + env: %.2f %% (live %d), %+.2f vs C4; headroom to 30 %%: %.2f %%%s\n", c5v, a5, c5v - c,
                     30.0 - c5v, c5v > 25.0 ? "  -> OVER 25 %: flag for 2.6" : "");
    }
}}

static void engineRows25ab()
{
    eg25b::rampedRows();
    eg25b::latchedRows();
    eg25b::tickConsumerRows();
    eg25b::telemetryRows();
    eg25b::coverageRows();
    eg25b::phaseRows();
    eg25b::zipperRows();
    eg25b::blockNullRows25();
    eg25b::allocRows25();
    eg25b::stressRows25();
    eg25b::cpuRows25();
}
