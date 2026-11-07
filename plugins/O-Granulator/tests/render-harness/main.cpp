/*
  ==============================================================================

    O-Granulator — render harness (Stage 1 gate; Stage 2 extends it)

      O-Granulator-render-harness        exit 0 = every row PASS

    Goldens come from the LOCKED spec (tests/golden/extract_spec.py →
    params.tsv / mod-dests.tsv), never from the code.

      G1  220 params, ID sequence == spec "Full ID list"
      G2  per-ID type; totals 140 Float / 59 Choice / 21 Bool
      G3  version hint 1 everywhere; host names == golden
      G4  Float: start / end exact, interval 0, skew (±1e-4 on setSkewForCentre
          rows), no custom mapping; default normalised == convertTo0to1 (spec)
          EXACTLY, engineering default within tolerance (skewed defaults are not
          bit-exact through APVTS)
      G5  Choice strings, order, default index
      G6  Bool defaults (s1_enable On, s2–s4 Off)
      D1  kModDestIds == golden (113, order)   D2 every dest ≠ None is a Float
      D3  modM_dest choices == None + golden dest names   D4 table version 1
      B1  layouts: mono→mono, mono→stereo, stereo→stereo accepted; stereo→mono,
          disabled, 5.1 rejected
      B2  (rescoped 2.1, P8) mix 0 % memcmp-exact, every param set through the
          host normalised path: 3 layouts × blocks {1, 32, 441, 512, 4096,
          seeded variable}; mono→stereo L == R == in; with a playing
          MockPlayHead and with none. Defaults (mix 50 %): wet present.
      B3  latency 0 after prepare
      S1  state round trip: seed 0xF00DFACECAFEBEEF (> 2^63), high-bit masks,
          custom "edo 19" / "edo 64", uiLanguage 2, a parameter
      S2  absent non-param state → defaults (seed kept)
      S3  two reopens with a change between: no stale / duplicate children
      S4  no Mask children at defaults
      P1  scale choice → Id map; names cross-check against the module
      E*  Stage 2 / 2.1 engine rows — see engine_gates.h
      F, Q, D  Stage 2 / 2.2 rows (FUNC-03, DSP-01, DSP-04) — see engine_gates_22.h
      E0+, W, K, X, N5, M2, C2  Stage 2 / 2.2 law + QUAL/mixer/PERF rows — see engine_gates_22b.h
      U, V, H  Stage 2 / 2.3 FX units, per-grain, no-bleed rows — see engine_gates_23.h
      U5–U8, FB  Stage 2 / 2.4 knee / D10 / chain / tap-law units, feedback rows — engine_gates_24.h
      N7, FZ, RV, Z3, Z4, M4, C4  Stage 2 / 2.4 PERF / freeze / reverb / stress rows — engine_gates_24b.h
      TT, U9, TS, LS, EV, MC  Stage 2 / 2.5a tempo + source units / rows — engine_gates_25a.h
      MF, PH, ZQ, TL, N8, M5, Z5, C5  Stage 2 / 2.5a matrix / QUAL / PERF rows — engine_gates_25ab.h
      OD, OB, PS, MP, OG  Stage 2 / 2.5b onset / burst / source / MPM / D20 rows — engine_gates_25b.h
      FR, AQ, MX, QB, N9, M6, Z6, C6  Stage 2 / 2.5b f0 ring / follow / PERF rows — engine_gates_25bb.h

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ScaleStateJson.h"   // Stage 3 (P106 / P107): the page's JSON payloads, shared with the editor

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#include <complex>
#include <limits>

#if defined (__clang__)
 #pragma clang diagnostic ignored "-Wfloat-equal"
#endif

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

#ifndef GOLDEN_DIR
 #error "GOLDEN_DIR must point at tests/golden"
#endif

namespace
{
    int failures = 0, passes = 0;

    void row (bool ok, const juce::String& name, const juce::String& detail = {})
    {
        std::printf ("  [%s] %-62s %s\n", ok ? "PASS" : "FAIL", name.toRawUTF8(), detail.toRawUTF8());
        if (ok) ++passes; else ++failures;
    }

    // Collects sub-failures of a many-item row; prints the first few.
    struct Tally
    {
        int bad = 0; juce::StringArray notes;
        void check (bool ok, const juce::String& what) { if (! ok) { ++bad; if (notes.size() < 4) notes.add (what); } }
        juce::String detail (int total) const { return juce::String (total - bad) + "/" + juce::String (total) + (bad ? "  e.g. " + notes.joinIntoString ("; ") : juce::String()); }
    };

    // PLAN-2.4 P33 attribution runs: `--legacy-24` (both), `--legacy-24=knee`
    // (D9 bypass only) or `--legacy-24=svf` (D10 bypass only) default the hooks
    // on EVERY processor the harness makes.
    bool gLegacyKnee = false, gLegacySvf = false;

    // PLAN-2.6 P94: `--long` adds Z7d / Z7dn and the C7 INFO extras; C7 sets
    // gNoVerdict when fewer than 7 uncontended runs exist (rc 2, never a FAIL).
    bool gLong = false, gNoVerdict = false;
    bool gNoC7 = false;   // `--no-c7`: iteration aid only (skips C7 / C7L timing); acceptance runs never pass it
    juce::String gSect;   // `--sect=a,b`: iteration aid only — run just these 2.6 sections (with --only26)
    bool sect (const char* name) { return gSect.isEmpty() || juce::StringArray::fromTokens (gSect, ",", "").contains (name); }

    std::unique_ptr<GranulatorProcessor> make()
    {
        auto p = std::unique_ptr<GranulatorProcessor> (dynamic_cast<GranulatorProcessor*> (createPluginFilter()));
        p->getEngineForTest().hooks.legacyKnee    = gLegacyKnee;
        p->getEngineForTest().hooks.legacySvfMake = gLegacySvf;
        return p;
    }

    // ── Goldens ─────────────────────────────────────────────────────────────
    struct GoldenParam { int idx; juce::String id, name, type; double start, end, def, skew, centre; juce::StringArray choices; };

    std::vector<juce::StringArray> readTsv (const juce::String& file)
    {
        std::vector<juce::StringArray> rows;
        const auto f = juce::File (GOLDEN_DIR).getChildFile (file);
        juce::StringArray lines;
        lines.addLines (f.loadFileAsString());
        bool header = true;
        for (auto& l : lines)
        {
            if (l.startsWith ("#") || l.isEmpty()) continue;
            if (header) { header = false; continue; }
            juce::StringArray cols;
            cols.addTokens (l, "\t", "");
            rows.push_back (cols);
        }
        return rows;
    }

    std::vector<GoldenParam> goldenParams()
    {
        std::vector<GoldenParam> g;
        for (auto& c : readTsv ("params.tsv"))
        {
            GoldenParam p { c[0].getIntValue(), c[1], c[2], c[3], c[4].getDoubleValue(), c[5].getDoubleValue(),
                            c[6].getDoubleValue(), c[7].getDoubleValue(), c[8].getDoubleValue(), {} };
            if (c[9].isNotEmpty()) p.choices.addTokens (c[9], "|", "");
            g.push_back (p);
        }
        return g;
    }

    // ── Synthetic playhead (Stage 2 tempo gates inherit it) ──────────────────
    // Legacy mode (script empty — B1/B2 and every carried row): ppq = t/fs·bpm/60,
    // a bpm change is a ppq JUMP. v2 mode (PLAN-2.5a P47, RESEARCH-2.5a R56): a
    // piecewise script of segments in HOST-SAMPLE time — tempo map (ppq
    // integrated across steps), time signature with ppqPositionOfLastBarStart,
    // start / stop, loop wrap, locate, negative start ppq. Hosts report a
    // position only at block start, so v2 renders cut host blocks at every
    // segment start (Transport::eventSamples → eg::withTransport).
    struct MockPlayHead : juce::AudioPlayHead
    {
        double bpm = 120.0, sampleRate = 48000.0;
        bool playing = true, hasBpm = true;
        juce::int64 timeInSamples = 0;

        struct Seg
        {
            juce::int64 at = 0;          // first host sample of the segment
            double ppq0 = 0.0;           // ppq at `at`
            double bpm  = 120.0;
            bool   playing = true;
            int    num = 4, den = 4;
            double barPpq = 0.0;         // a bar start in this segment's bar frame
            bool   looping = false;
            double loopA = 0.0, loopB = 0.0;
            bool   hasBpm = true, hasPpq = true, hasSig = true;
        };
        std::vector<Seg> script;         // v2 iff non-empty (sorted by `at`, script[0].at == 0)

        const Seg& segAt (juce::int64 t) const
        {
            size_t k = 0;
            while (k + 1 < script.size() && script[k + 1].at <= t) ++k;
            return script[k];
        }
        static double barLen (const Seg& s) { return 4.0 * (double) s.num / (double) s.den; }
        double ppqAt (juce::int64 t) const
        {
            const auto& s = segAt (t);
            return s.playing ? s.ppq0 + (double) (t - s.at) / sampleRate * s.bpm / 60.0 : s.ppq0;
        }
        double lastBarStartAt (juce::int64 t) const
        {
            const auto& s = segAt (t);
            const double L = barLen (s), q = ppqAt (t);
            return s.barPpq + std::floor ((q - s.barPpq) / L) * L;
        }

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            if (! script.empty())
            {
                const auto& s = segAt (timeInSamples);
                if (s.hasBpm) info.setBpm (s.bpm);
                info.setIsPlaying (s.playing);
                if (s.hasSig) info.setTimeSignature (TimeSignature { s.num, s.den });
                info.setTimeInSamples (timeInSamples);
                if (s.hasPpq)
                {
                    info.setPpqPosition (ppqAt (timeInSamples));
                    info.setPpqPositionOfLastBarStart (lastBarStartAt (timeInSamples));
                }
                info.setIsLooping (s.looping);
                if (s.looping) info.setLoopPoints (LoopPoints { s.loopA, s.loopB });
                return info;
            }
            if (hasBpm) info.setBpm (bpm);
            info.setIsPlaying (playing);
            info.setTimeSignature (TimeSignature { 4, 4 });
            info.setTimeInSamples (timeInSamples);
            info.setPpqPosition (static_cast<double> (timeInSamples) / sampleRate * bpm / 60.0);
            return info;
        }
        void advance (int n) { if (playing) timeInSamples += n; }
    };

    /** v2 script builder. Every modifier takes the host sample it lands on
        (strictly increasing) and derives the new segment from the state there. */
    struct Transport
    {
        double fs = 48000.0;
        std::vector<MockPlayHead::Seg> segs;

        explicit Transport (double sampleRate = 48000.0, double bpm = 120.0, double startPpq = 0.0, bool playing = true,
                            int num = 4, int den = 4)
            : fs (sampleRate)
        {
            MockPlayHead::Seg s;
            s.ppq0 = startPpq; s.bpm = bpm; s.playing = playing; s.num = num; s.den = den;
            s.barPpq = 0.0;
            segs.push_back (s);
        }

        double ppqAt (juce::int64 t) const
        {
            MockPlayHead m; m.sampleRate = fs; m.script = segs;
            return m.ppqAt (t);
        }
        double lastBarStartAt (juce::int64 t) const
        {
            MockPlayHead m; m.sampleRate = fs; m.script = segs;
            return m.lastBarStartAt (t);
        }
        const MockPlayHead::Seg& segAt (juce::int64 t) const
        {
            size_t k = 0;
            while (k + 1 < segs.size() && segs[k + 1].at <= t) ++k;
            return segs[k];
        }

        MockPlayHead::Seg& next (juce::int64 at)
        {
            jassert (at > segs.back().at);
            MockPlayHead::Seg s = segs.back();
            s.ppq0 = ppqAt (at);
            s.at = at;
            segs.push_back (s);
            return segs.back();
        }
        Transport& tempo (juce::int64 at, double bpm)            { next (at).bpm = bpm; return *this; }
        Transport& stop (juce::int64 at)                         { next (at).playing = false; return *this; }
        Transport& start (juce::int64 at)                        { next (at).playing = true; return *this; }
        Transport& locate (juce::int64 at, double ppq)           { next (at).ppq0 = ppq; return *this; }
        /** The time signature changes at `at`; a bar starts there. */
        Transport& timeSig (juce::int64 at, int num, int den)
        {
            auto& s = next (at);
            s.num = num; s.den = den; s.barPpq = s.ppq0;
            return *this;
        }
        /** Loop [a, b) engaged from the start; the wrap to a happens at host sample `at`. */
        Transport& loopWrap (juce::int64 at, double a, double b)
        {
            for (auto& s : segs) { s.looping = true; s.loopA = a; s.loopB = b; }
            auto& s = next (at);
            s.ppq0 = a;
            return *this;
        }
        /** Every segment loses bpm / ppq / time signature (the WR-09 / no-ppq fallback). */
        Transport& noInfo (bool bpm, bool ppq, bool sig = true)
        {
            for (auto& s : segs) { s.hasBpm = bpm; s.hasPpq = ppq; s.hasSig = sig; }
            return *this;
        }

        std::vector<juce::int64> eventSamples() const
        {
            std::vector<juce::int64> v;
            for (size_t k = 1; k < segs.size(); ++k) v.push_back (segs[k].at);
            return v;
        }
        std::shared_ptr<MockPlayHead> makeHead() const
        {
            auto h = std::make_shared<MockPlayHead>();
            h->sampleRate = fs;
            h->script = segs;
            return h;
        }
    };

    juce::RangedAudioParameter* param (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                if (rp->getParameterID() == id) return rp;
        return nullptr;
    }

    void setParam (juce::AudioProcessor& p, const juce::String& id, float value)
    {
        if (auto* rp = param (p, id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));   // harness only (the host side)
    }

    // Every parameter written with its default NORMALISED value, the way a host
    // or pluginval writes it (R2: skewed / non-dyadic ranges land a hair off).
    void setAllViaHostNormalised (juce::AudioProcessor& p)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                rp->setValueNotifyingHost (rp->getDefaultValue());
    }
}

#include "engine_gates.h"   // Stage 2 section E (engine rows)
#include "engine_gates_22.h" // Stage 2 / 2.2 sections F, Q, D (PLAN-2.2 P16)
#include "engine_gates_22b.h" // Stage 2 / 2.2 sections E0+, W, K, X, N5, M2, C2 (T14 / T16)
#include "engine_gates_23.h"  // Stage 2 / 2.3 sections U, V, H (PLAN-2.3 P29)
#include "engine_gates_23b.h" // Stage 2 / 2.3 sections N6, Z, K2, M3, C3 (PLAN-2.3 P29)
#include "engine_gates_24.h"  // Stage 2 / 2.4 sections U5–U8, FB (PLAN-2.4 P44)
#include "engine_gates_24b.h" // Stage 2 / 2.4 sections N7, FZ, RV, Z3/Z4, M4, C4 (PLAN-2.4 P44)
#include "engine_gates_25a.h" // Stage 2 / 2.5a sections TT, U9, TS, LS, EV, MC (PLAN-2.5a P58)
#include "engine_gates_25ab.h" // Stage 2 / 2.5a sections MF, PH, ZQ, TL, N8, M5, Z5, C5 (PLAN-2.5a P58)
#include "engine_gates_25b.h"  // Stage 2 / 2.5b sections OD, OB, PS, MP, OG (PLAN-2.5b P76)
#include "engine_gates_25bb.h" // Stage 2 / 2.5b sections FR, AQ, MX, QB, N9, M6, Z6, C6 (PLAN-2.5b P76)
#include "engine_gates_26.h"   // Stage 2 / 2.6 sections TA, Z7, HR (PLAN-2.6 P94)
#include "engine_gates_26b.h"  // Stage 2 / 2.6 sections C7, GL, MX3, MF8, EN1, FZ9, K3, QB3, M7, N10, O2 (PLAN-2.6 P94)

//==============================================================================
static void paramRows()
{
    std::printf ("\n-- G. parameters vs spec golden\n");
    const auto golden = goldenParams();
    auto p = make();
    const auto& params = p->getParameters();

    std::vector<juce::RangedAudioParameter*> ranged;
    for (auto* ap : params) ranged.push_back (dynamic_cast<juce::RangedAudioParameter*> (ap));

    // G1
    bool seq = golden.size() == 220 && params.size() == 220;
    Tally t1;
    for (size_t i = 0; seq && i < golden.size(); ++i)
        t1.check (ranged[i] != nullptr && ranged[i]->getParameterID() == golden[i].id, golden[i].id);
    row (seq && t1.bad == 0, "G1 220 params, ID order == spec", t1.detail (220) + "  (golden " + juce::String ((int) golden.size()) + ", plugin " + juce::String (params.size()) + ")");
    if (! seq) return;

    // ID list also equals GranulatorParamIds::allIdsInOrder()
    const auto& ids = GranulatorParamIds::allIdsInOrder();
    Tally t1b;
    for (int i = 0; i < 220; ++i) t1b.check (ids[i] == golden[(size_t) i].id, golden[(size_t) i].id);
    row (ids.size() == 220 && t1b.bad == 0, "G1 allIdsInOrder() == spec", t1b.detail (220));

    // G2 / G3
    int nf = 0, nc = 0, nb = 0;
    Tally t2, t3, tn;
    for (size_t i = 0; i < 220; ++i)
    {
        auto* rp = ranged[i];
        const auto& g = golden[i];
        const bool isF = dynamic_cast<juce::AudioParameterFloat*> (rp) != nullptr;
        const bool isC = dynamic_cast<juce::AudioParameterChoice*> (rp) != nullptr;
        const bool isB = dynamic_cast<juce::AudioParameterBool*> (rp) != nullptr;
        nf += isF; nc += isC; nb += isB;
        t2.check ((g.type == "Float" && isF) || (g.type == "Choice" && isC) || (g.type == "Bool" && isB), g.id + " type");
        t3.check (rp->getVersionHint() == 1, g.id);
        tn.check (rp->getName (128) == g.name, g.id + " '" + rp->getName (128) + "' != '" + g.name + "'");
    }
    row (t2.bad == 0, "G2 per-ID type", t2.detail (220));
    row (nf == 140 && nc == 59 && nb == 21, "G2 totals 140 Float / 59 Choice / 21 Bool",
         juce::String (nf) + " / " + juce::String (nc) + " / " + juce::String (nb));
    row (t3.bad == 0, "G3 version hint 1", t3.detail (220));
    row (tn.bad == 0, "G3 host names == golden", tn.detail (220));

    // G4 / G5 / G6
    Tally t4r, t4d, t4e, t5, t6;
    int floats = 0, choices = 0, bools = 0;
    double worstEng = 0.0;
    for (size_t i = 0; i < 220; ++i)
    {
        const auto& g = golden[i];
        if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (ranged[i]))
        {
            ++floats;
            const auto& r = f->getNormalisableRange();
            const bool skewOk = g.centre > 0.0 ? std::abs (r.skew - g.skew) <= 1e-4 : r.skew == (float) g.skew;
            t4r.check (r.start == (float) g.start && r.end == (float) g.end && r.interval == 0.0f && skewOk
                       && ! r.symmetricSkew, g.id + " range (skew " + juce::String (r.skew, 5) + ")");
            const float normSpec = r.convertTo0to1 ((float) g.def);
            t4d.check (ranged[i]->getDefaultValue() == normSpec, g.id + " default norm");
            const double eng = f->convertFrom0to1 (ranged[i]->getDefaultValue());
            const double tol = 1e-6 * juce::jmax (1.0, std::abs (g.end - g.start));
            worstEng = juce::jmax (worstEng, std::abs (eng - g.def) / juce::jmax (1.0, std::abs (g.end - g.start)));
            t4e.check (std::abs (eng - g.def) <= tol && std::abs (f->get() - g.def) <= tol, g.id + " default eng " + juce::String (eng, 7));
        }
        else if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (ranged[i]))
        {
            ++choices;
            t5.check (c->choices == g.choices && c->getIndex() == (int) g.def
                      && ranged[i]->getDefaultValue() == c->convertTo0to1 ((float) g.def),
                      g.id + " (" + juce::String (c->choices.size()) + " vs " + juce::String (g.choices.size()) + ")");
        }
        else if (auto* b = dynamic_cast<juce::AudioParameterBool*> (ranged[i]))
        {
            ++bools;
            t6.check (b->get() == (g.def != 0.0) && (ranged[i]->getDefaultValue() > 0.5f) == (g.def != 0.0), g.id);
        }
    }
    row (t4r.bad == 0, "G4 float start / end / interval 0 / skew", t4r.detail (floats));
    row (t4d.bad == 0, "G4 float default normalised == convertTo0to1 (spec)", t4d.detail (floats));
    row (t4e.bad == 0, "G4 float default engineering (tol 1e-6 of span)", t4e.detail (floats) + "  worst " + juce::String (worstEng, 9));
    row (t5.bad == 0, "G5 choice strings / order / default", t5.detail (choices));
    row (t6.bad == 0, "G6 bool defaults (s1_enable On)", t6.detail (bools));
    row (dynamic_cast<juce::AudioParameterBool*> (param (*p, "s1_enable"))->get()
         && ! dynamic_cast<juce::AudioParameterBool*> (param (*p, "s4_enable"))->get(), "G6 s1_enable On, s4_enable Off");

    // D
    std::printf ("\n-- D. modulation destination table\n");
    const auto dg = readTsv ("mod-dests.tsv");
    Tally d1, d2;
    bool sized = dg.size() == 113 && GranulatorParamIds::kModDests == 113;
    for (int k = 0; sized && k < 113; ++k)
        d1.check (juce::String (GranulatorParamIds::kModDestIds[k]) == dg[(size_t) k][1], juce::String (k) + " " + GranulatorParamIds::kModDestIds[k]);
    row (sized && d1.bad == 0 && juce::String (GranulatorParamIds::kModDestIds[0]) == "None", "D1 kModDestIds == golden (113, order)", d1.detail (113));
    for (int k = 1; sized && k < 113; ++k)
        d2.check (dynamic_cast<juce::AudioParameterFloat*> (param (*p, GranulatorParamIds::kModDestIds[k])) != nullptr, GranulatorParamIds::kModDestIds[k]);
    row (d2.bad == 0, "D2 every destination is a registered Float", d2.detail (112));
    Tally d3;
    juce::StringArray expect;
    for (auto& r : dg) expect.add (r[2]);
    for (int m = 1; m <= GranulatorParamIds::kModSlots; ++m)
    {
        auto* c = dynamic_cast<juce::AudioParameterChoice*> (param (*p, GranulatorParamIds::mod (m, "dest")));
        d3.check (c != nullptr && c->choices == expect, "mod" + juce::String (m) + "_dest");
    }
    row (d3.bad == 0 && expect[0] == "None", "D3 modM_dest choices == None + golden host names", d3.detail (16));
    row (GranulatorParamIds::kModDestTableVersion == 1, "D4 modDestTableVersion == 1");
}

//==============================================================================
static void busRows()
{
    std::printf ("\n-- B. buses, pass-through, latency\n");
    using CS = juce::AudioChannelSet;
    auto p = make();
    auto layout = [] (CS in, CS out) { juce::AudioProcessor::BusesLayout l; l.inputBuses.add (in); l.outputBuses.add (out); return l; };
    const bool acc = p->checkBusesLayoutSupported (layout (CS::mono(), CS::mono()))
                  && p->checkBusesLayoutSupported (layout (CS::mono(), CS::stereo()))
                  && p->checkBusesLayoutSupported (layout (CS::stereo(), CS::stereo()));
    const bool rej = ! p->checkBusesLayoutSupported (layout (CS::stereo(), CS::mono()))
                  && ! p->checkBusesLayoutSupported (layout (CS::disabled(), CS::stereo()))
                  && ! p->checkBusesLayoutSupported (layout (CS::stereo(), CS::disabled()))
                  && ! p->checkBusesLayoutSupported (layout (CS::create5point1(), CS::create5point1()))
                  && ! p->checkBusesLayoutSupported (layout (CS::stereo(), CS::create5point1()));
    row (acc, "B1 accepts mono->mono, mono->stereo, stereo->stereo");
    row (rej, "B1 rejects stereo->mono, disabled buses, 5.1");
    row (p->getMainBusNumInputChannels() == 2 && p->getMainBusNumOutputChannels() == 2, "B1 default layout stereo->stereo");

    struct L { const char* name; CS in, out; };
    const L layouts[] = { { "mono->mono", CS::mono(), CS::mono() }, { "mono->stereo", CS::mono(), CS::stereo() },
                          { "stereo->stereo", CS::stereo(), CS::stereo() } };
    const int blocks[] = { 1, 32, 441, 512, 4096, 0 /* seeded variable */ };
    constexpr int kLen = 48000;
    constexpr double kSr = 48000.0;

    juce::AudioBuffer<float> input (2, kLen);
    juce::Random r (0x6EA1ull);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kLen; ++i)
            input.setSample (ch, i, (r.nextFloat() * 2.0f - 1.0f) * 0.7f + (ch ? 1e-30f : 0.0f));   // a denormal-ish offset on R

    // P8 (deliberate gate change, Stage 2 / 2.1): mix defaults to 50 %, so the
    // Stage 1 "defaults" bitwise rows are REPLACED by "defaults: wet present"
    // below. The mix 0 % rows set EVERY parameter through the host normalised
    // path (setValueNotifyingHost (default normalised)) — that exercises R2: the
    // 0 dB output_gain default round-trips to 7.15e-7 dB.
    for (auto& L : layouts)
        for (bool mix0 : { true })
            for (int playheadMode : { 0, 1 })
            {
                Tally tb;
                bool latencyOk = true;
                for (int b : blocks)
                {
                    auto q = make();
                    MockPlayHead ph;
                    if (playheadMode == 1) q->setPlayHead (&ph);
                    const bool okLayout = q->setBusesLayout (layout (L.in, L.out));
                    setAllViaHostNormalised (*q);
                    if (mix0) setParam (*q, "mix", 0.0f);
                    q->prepareToPlay (kSr, b == 0 ? 512 : b);
                    latencyOk = latencyOk && q->getLatencySamples() == 0;
                    const int inCh = L.in.size(), outCh = L.out.size(), numCh = juce::jmax (inCh, outCh);
                    juce::AudioBuffer<float> out (numCh, kLen), blk (numCh, 4096);
                    juce::MidiBuffer midi;
                    juce::Random vr (0xB10CCull);
                    int pos = 0;
                    while (pos < kLen)
                    {
                        const int n = juce::jmin (b == 0 ? 1 + vr.nextInt (512) : b, kLen - pos);
                        blk.setSize (numCh, n, false, false, true);
                        for (int ch = 0; ch < numCh; ++ch)
                        {
                            if (ch < inCh) blk.copyFrom (ch, 0, input, ch, pos, n);
                            else for (int i = 0; i < n; ++i) blk.setSample (ch, i, 12345.0f);   // garbage in a non-input channel
                        }
                        q->processBlock (blk, midi);
                        ph.advance (n);
                        for (int ch = 0; ch < numCh; ++ch) out.copyFrom (ch, pos, blk, ch, 0, n);
                        pos += n;
                    }
                    const auto bytes = sizeof (float) * (size_t) kLen;
                    bool same = okLayout;
                    if (inCh == 2) same = same && std::memcmp (out.getReadPointer (0), input.getReadPointer (0), bytes) == 0
                                               && std::memcmp (out.getReadPointer (1), input.getReadPointer (1), bytes) == 0;
                    else
                    {
                        same = same && std::memcmp (out.getReadPointer (0), input.getReadPointer (0), bytes) == 0;
                        if (outCh == 2) same = same && std::memcmp (out.getReadPointer (1), input.getReadPointer (0), bytes) == 0;
                    }
                    tb.check (same, "block " + (b == 0 ? juce::String ("var") : juce::String (b)));
                    q->setPlayHead (nullptr);
                }
                row (tb.bad == 0, juce::String ("B2 ") + L.name + (mix0 ? " mix 0 % (host-normalised)" : " defaults") + (playheadMode ? " +playhead" : " no playhead")
                         + " bitwise", tb.detail ((int) std::size (blocks)));
                if (! latencyOk) row (false, "B3 latency 0", L.name);
            }
    row (true, "B3 latency 0 after prepare (all layouts / blocks)");

    // B2 defaults: wet present (replaces the Stage 1 defaults pass-through rows).
    for (auto& L : layouts)
    {
        auto q = make();
        q->setSeed (0x5EED'B2B2'0000'0001ull);   // PLAN-2.4 P43: deterministic (VERIFICATION-2.3 deviation 4)
        q->setBusesLayout (layout (L.in, L.out));
        q->prepareToPlay (kSr, 512);
        const int inCh = L.in.size(), outCh = L.out.size(), numCh = juce::jmax (inCh, outCh);
        juce::AudioBuffer<float> blk (numCh, 512);
        juce::MidiBuffer midi;
        double accSq = 0.0; int nAcc = 0;
        for (int pos = 0; pos < kLen * 2; pos += 512)
        {
            for (int ch = 0; ch < numCh; ++ch)
                for (int i = 0; i < 512; ++i)
                    blk.setSample (ch, i, ch < inCh ? input.getSample (ch, (pos + i) % kLen) : 0.0f);
            q->processBlock (blk, midi);
            if (pos >= kLen)   // after 1 s
                for (int ch = 0; ch < outCh; ++ch)
                    for (int i = 0; i < 512; ++i)
                    {
                        const double d = (double) blk.getSample (ch, i) - (double) input.getSample (ch < inCh ? ch : 0, (pos + i) % kLen);
                        accSq += d * d; ++nAcc;
                    }
        }
        const double db = 10.0 * std::log10 (juce::jmax (1e-30, accSq / juce::jmax (1, nAcc)));
        row (db > -40.0, juce::String ("B2 ") + L.name + " defaults: wet present (RMS(out - dry) > -40 dBFS after 1 s)",
             juce::String (db, 1) + " dBFS");
    }

    // No-bpm playhead row (Stage 2 tempo code must survive it); shell passes through regardless.
    {
        auto q = make();
        MockPlayHead ph; ph.hasBpm = false; ph.playing = false;
        q->setPlayHead (&ph);
        setParam (*q, "mix", 0.0f);
        q->prepareToPlay (kSr, 512);
        juce::AudioBuffer<float> blk (2, 512);
        for (int ch = 0; ch < 2; ++ch) blk.copyFrom (ch, 0, input, ch, 0, 512);
        juce::MidiBuffer midi;
        q->processBlock (blk, midi);
        row (std::memcmp (blk.getReadPointer (0), input.getReadPointer (0), 512 * sizeof (float)) == 0, "B2 no-bpm stopped playhead, mix 0 %: passes through");
        q->setPlayHead (nullptr);
    }
}

//==============================================================================
static juce::MemoryBlock stateOf (juce::AudioProcessor& p) { juce::MemoryBlock m; p.getStateInformation (m); return m; }
static void load (juce::AudioProcessor& p, const juce::MemoryBlock& m) { p.setStateInformation (m.getData(), (int) m.getSize()); }
static std::unique_ptr<juce::XmlElement> xmlOf (const juce::MemoryBlock& m) { return juce::AudioProcessor::getXmlFromBinary (m.getData(), (int) m.getSize()); }

static bool sameNonParamState (GranulatorProcessor& a, GranulatorProcessor& b, juce::String& why)
{
    if (a.getSeed() != b.getSeed()) { why = "seed"; return false; }
    if (a.uiLanguage.load() != b.uiLanguage.load()) { why = "uiLanguage"; return false; }
    const auto ca = a.getCustomScale(), cb = b.getCustomScale();
    if (ca.text != cb.text || ca.name != cb.name) { why = "custom text/name"; return false; }
    const auto ta = a.getCustomTable(), tb = b.getCustomTable();
    if (ta.count != tb.count || std::memcmp (&ta.period, &tb.period, 4) != 0
        || std::memcmp (ta.cents.data(), tb.cents.data(), sizeof (float) * (size_t) ta.count) != 0) { why = "custom table"; return false; }
    for (int s = 1; s <= 4; ++s)
        for (int c = 1; c < 32; ++c)
            if (a.getDegreeMask (s, c) != b.getDegreeMask (s, c)) { why = "mask s" + juce::String (s) + " c" + juce::String (c); return false; }
    return true;
}

static int countChildren (const juce::XmlElement& x, const juce::String& tag)
{
    int n = 0;
    for (auto* c : x.getChildIterator()) n += c->hasTagName (tag) ? 1 : 0;
    return n;
}

#include "engine_gates_31.h"  // Stage 3 / 3.1 sections PM, WC (PLAN-3.1 P104); TL31 → TL32 (3.2)
#include "engine_gates_32.h"  // Stage 3 / 3.2 sections TB, WV, GD, CN, BD, M8, TL32, FB1 / PL1 (PLAN-3.2 P120)
#include "engine_gates_33.h"  // Stage 3 / 3.3 sections UX1 (PLAN-3.3 P128, P133)

static void stateRows()
{
    std::printf ("\n-- S. state codec\n");
    constexpr uint64_t kSeed = 0xF00DFACECAFEBEEFull;

    // S1a: custom edo 19, high-bit mask on 53-EDO (choice 23), uiLanguage 2, a param.
    {
        auto a = make();
        a->setSeed (kSeed);
        a->uiLanguage = 2;
        juce::String bad;
        const auto err = a->setCustomScale ("Nineteen", "edo 19", bad);
        a->setDegreeMask (2, 23, 0x8000000000000001ull);
        a->setDegreeMask (4, 1, 0x0000000000000AAAull);
        setParam (*a, "s3_pitch", 7.5f);
        const auto blob = stateOf (*a);
        auto b = make();
        load (*b, blob);
        juce::String why;
        const bool same = err.isEmpty() && sameNonParamState (*a, *b, why);
        row (same && b->getSeed() == kSeed, "S1 round trip: seed > 2^63, edo 19, masks, uiLanguage 2", same ? juce::String() : why);
        row (b->getDegreeMask (2, 23) == GranulatorProcessor::canonicalMask (0x8000000000000001ull, 53)
             && b->getCustomTable().count == 19, "S1 mask canonical + custom table count 19");
        auto* pp = param (*b, "s3_pitch");
        row (pp != nullptr && std::abs (pp->convertFrom0to1 (pp->getValue()) - 7.5f) < 1e-4f, "S1 parameter restored");
        const auto x = xmlOf (blob);
        row (x != nullptr && x->getStringAttribute ("uiLanguage") == "zh-Hans"
             && x->getChildByName ("Engine")->getStringAttribute ("seed") == juce::String ((juce::int64) kSeed),
             "S1 XML: uiLanguage code, seed as int64 string", x ? x->getChildByName ("Engine")->getStringAttribute ("seed") : juce::String());
        bool noId = true;
        for (auto* tag : { "Engine", "CustomScale", "DegreeMasks" })
            noId = noId && ! x->getChildByName (tag)->hasAttribute ("id");
        row (noId, "S1 custom children carry no 'id' attribute");
    }
    // S1b: 64-degree custom keeps a raw 0x8000000000000001 mask (no canonical bits to add).
    {
        auto a = make();
        juce::String bad;
        a->setCustomScale ("", "edo 64", bad);
        a->setDegreeMask (1, 31, 0x8000000000000001ull);
        a->uiLanguage = 1;
        auto b = make();
        load (*b, stateOf (*a));
        row (b->getDegreeMask (1, 31) == 0x8000000000000001ull && b->getCustomTable().count == 64 && b->uiLanguage.load() == 1,
             "S1 edo 64 + mask 0x8000000000000001 exact, uiLanguage fr");
    }
    // S1c: uiLanguage stored as an int (spec form) reads too.
    {
        auto a = make();
        auto x = xmlOf (stateOf (*a));
        x->setAttribute ("uiLanguage", "2");
        juce::MemoryBlock m;
        juce::AudioProcessor::copyXmlToBinary (*x, m);
        auto b = make();
        load (*b, m);
        row (b->uiLanguage.load() == 2, "S1 uiLanguage read tolerantly (int form)");
    }
    // S2: absent non-param state → defaults; seed kept.
    {
        auto a = make();
        auto x = xmlOf (stateOf (*a));
        for (auto* tag : { "Engine", "CustomScale", "DegreeMasks" })
            x->removeChildElement (x->getChildByName (tag), true);
        x->removeAttribute ("uiLanguage");
        juce::MemoryBlock m;
        juce::AudioProcessor::copyXmlToBinary (*x, m);
        auto b = make();
        b->setSeed (0x1234ull);
        b->uiLanguage = 1;
        juce::String bad;
        b->setCustomScale ("x", "5/4 3/2 2/1", bad);
        b->setDegreeMask (3, 19, 0x5ull);
        load (*b, m);
        row (b->getSeed() == 0x1234ull && b->uiLanguage.load() == 1 && b->getCustomScale().text == GranulatorProcessor::defaultCustomText()
             && b->getCustomScale().name.isEmpty() && b->getDegreeMask (3, 19) == GranulatorProcessor::kAllOn,
             "S2 absent state: custom 12-EDO, masks all-on, seed + language kept");
    }
    // S3: two reopens, a change between.
    {
        auto a = make();
        a->setSeed (kSeed);
        juce::String bad;
        a->setCustomScale ("Pelog-ish", "120 270 540 670 785 950 1200", bad);
        a->setDegreeMask (1, 19, 0x7ull);
        auto b = make();
        load (*b, stateOf (*a));
        b->setDegreeMask (2, 5, 0x3ull);
        const auto blob2 = stateOf (*b);
        auto c = make();
        load (*c, blob2);
        load (*c, blob2);   // reopen twice into the same instance
        const auto x = xmlOf (stateOf (*c));
        juce::String why;
        row (sameNonParamState (*b, *c, why) && c->getDegreeMask (1, 19) == GranulatorProcessor::canonicalMask (0x7ull, 19)
             && c->getDegreeMask (2, 5) == GranulatorProcessor::canonicalMask (0x3ull, 6), "S3 second reopen keeps the change", why);
        row (countChildren (*x, "Engine") == 1 && countChildren (*x, "CustomScale") == 1 && countChildren (*x, "DegreeMasks") == 1
             && countChildren (*x, "PARAM") == 220, "S3 exactly one of each child after reopens (220 PARAM)",
             juce::String (countChildren (*x, "Engine")) + "/" + juce::String (countChildren (*x, "CustomScale")) + "/"
             + juce::String (countChildren (*x, "DegreeMasks")) + " PARAM " + juce::String (countChildren (*x, "PARAM")));
    }
    // S4: defaults write no Mask children.
    {
        auto a = make();
        const auto x = xmlOf (stateOf (*a));
        auto* dm = x->getChildByName ("DegreeMasks");
        row (dm != nullptr && dm->getNumChildElements() == 0, "S4 no Mask children at defaults");
    }
    // Custom edits reset the Custom mask of every stream.
    {
        auto a = make();
        juce::String bad;
        a->setCustomScale ("", "edo 12", bad);
        for (int s = 1; s <= 4; ++s) a->setDegreeMask (s, 31, 0x1ull);
        a->setCustomScale ("", "edo 7", bad);
        bool allOn = true;
        for (int s = 1; s <= 4; ++s) allOn = allOn && a->getDegreeMask (s, 31) == GranulatorProcessor::kAllOn;
        juce::String bad2;
        const auto err = a->setCustomScale ("", "edo 99", bad2);
        const auto err2 = a->setCustomScale ("", "100 2x0 1200", bad2);
        row (allOn && err == "badEdo" && err2 == "badPitch" && bad2 == "2x0" && a->getCustomScale().text == "edo 7",
             "S custom: new table resets Custom masks; errors keep the old scale");
    }
}

//==============================================================================
static void scaleRows()
{
    std::printf ("\n-- P. scale map\n");
    const auto& names = GranulatorParamIds::scaleNames();
    Tally t;
    for (int c = 1; c < 32; ++c)
        t.check (names[c] == PitchCollections::names()[GranulatorParamIds::scaleChoiceToId (c)], names[c]);
    row (names.size() == 32 && names[0] == "Off" && t.bad == 0, "P1 scale choice c -> Id: names match the module", t.detail (31));
    auto p = make();
    bool tables = p->getCollectionTable (0) == nullptr && p->getCollectionTable (32) == nullptr;
    for (int c = 1; c < 31; ++c)
        tables = tables && p->getCollectionTable (c) == &PitchCollections::builtIns()[(size_t) GranulatorParamIds::scaleChoiceToId (c)];
    row (tables && p->getCollectionTable (31) != nullptr && p->getCollectionTable (31)->count == 12, "P1 getCollectionTable: Off null, built-ins, custom 12-EDO");
}

//==============================================================================
// PLAN-3.1 P106 / P107: `--dump-ui-overrides <path>` writes the generic UI
// stub's seed (tests/ui-stub/generic-overrides.json) from the processor itself
// — combos (choices + default index), every Float range (start / end / def /
// skew / interval), and the natives the page pulls, through the SAME encoders
// the editor uses (ScaleStateJson.h). Early exit; no gate runs. Deterministic:
// nothing seeded, nothing timed, LF line endings.
//==============================================================================
static int dumpUiOverrides (const juce::File& out)
{
    auto p = make();

    auto* combos  = new juce::DynamicObject();
    auto* sliders = new juce::DynamicObject();
    auto* defs    = new juce::DynamicObject();
    int nc = 0, nf = 0;
    for (auto* ap : p->getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap);
        if (rp == nullptr) continue;
        const auto id = rp->getParameterID();
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (rp))
        {
            juce::Array<juce::var> choices;
            for (const auto& ch : c->choices) choices.add (ch);
            auto* o = new juce::DynamicObject();
            o->setProperty ("choices", choices);
            o->setProperty ("def", juce::roundToInt (c->convertFrom0to1 (rp->getDefaultValue())));
            combos->setProperty (id, juce::var (o));
            ++nc;
        }
        else if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (rp))
        {
            const auto& r = f->getNormalisableRange();
            auto* o = new juce::DynamicObject();
            o->setProperty ("start", r.start);
            o->setProperty ("end", r.end);
            o->setProperty ("def", f->convertFrom0to1 (rp->getDefaultValue()));
            o->setProperty ("skew", r.skew);
            o->setProperty ("interval", r.interval);
            o->setProperty ("label", f->getLabel());
            sliders->setProperty (id, juce::var (o));
            ++nf;
        }
        // getParameterDefaults: every Float AND every Choice (choice = index),
        // exactly as the editor's native answers (PluginEditor.cpp).
        if (dynamic_cast<juce::AudioParameterBool*> (rp) == nullptr)
            defs->setProperty (id, rp->convertFrom0to1 (rp->getDefaultValue()));
    }

    // getTelemetry: the widest realistic frame (RESEARCH-3.1 §7) — every 3.2 /
    // 3.3 key present so the label gates measure the longest readouts.
    GranulatorProcessor::Telemetry t;
    p->readTelemetry (t);
    t.hasBuffer = t.hasCounts = t.hasMeters = t.hasOnset = t.hasLoopGain = true;
    t.frozen = false;
    t.spanSec = 30.0f;                                   // buffer_length max
    t.waveCount = GranulatorProcessor::Telemetry::kWaveBins;
    for (int i = 0; i < t.waveCount; ++i)
        t.wave[(size_t) i] = (float) (0.5 + 0.45 * std::sin (0.11 * i) * std::cos (0.017 * i));
    t.grainCount = GranulatorProcessor::Telemetry::kMaxGrains;
    for (int i = 0; i < t.grainCount; ++i)
    {
        auto& g = t.grains[(size_t) i];
        g.stream = (uint8_t) (1 + i % 4);
        g.delaySec = 0.2f * (float) i;
        g.pitchSt = (float) ((i % 49) - 24);
        g.pan = (float) ((i % 21) - 10) / 10.0f;
        g.env = 0.5f;
        g.gain = 0.8f;
        g.lengthSec = 0.1f;
    }
    for (auto& b : t.bands) b = { 0.5f, 1.0f };          // 3.2 (D30): the widest band, every stream
    t.counts = { 128, 128, 128, 128 };
    t.limit = 128;
    t.outPeakL = t.outPeakR = t.inPeak = 0.9f;
    t.onsetPulse = 1.0f;
    t.onsetFired = true;
    t.lfoPhase = { 0.25f, 0.5f, 0.75f, 0.9f };
    t.pitchMidi = 61.0f; t.pitchCents = -49.0f; t.pitchClarity = 1.0f;
    t.loopGainDb = -0.4f;

    std::array<float, 64> curve {};
    p->getWindowCurve (1, curve.data(), (int) curve.size());
    juce::Array<juce::var> pts;
    for (auto v : curve) pts.add (v);

    auto* natives = new juce::DynamicObject();
    natives->setProperty ("getScaleState", ogran::ui::buildScaleState (*p));
    natives->setProperty ("getTelemetry", ogran::ui::telemetryToVar (t));
    natives->setProperty ("getParameterDefaults", juce::var (defs));
    natives->setProperty ("getWindowCurve", pts);

    auto* root = new juce::DynamicObject();
    root->setProperty ("_why", "Generated from the processor by the render harness (PLAN-3.1 P106) - never hand-edit. "
                               "Regenerate: O-Granulator-render-harness --dump-ui-overrides "
                               "plugins/O-Granulator/tests/ui-stub/generic-overrides.json");
    root->setProperty ("combos", juce::var (combos));
    root->setProperty ("sliders", juce::var (sliders));
    root->setProperty ("natives", juce::var (natives));

    const juce::var rootVar (root);   // owns the whole tree from here (DynamicObject is ref-counted)
    const int nd = defs->getProperties().size();
    out.getParentDirectory().createDirectory();
    const bool ok = out.replaceWithText (juce::JSON::toString (rootVar, false) + "\n", false, false, "\n");
    std::printf ("dump-ui-overrides: %d combos, %d sliders, %d defaults -> %s (%s)\n", nc, nf, nd,
                 out.getFullPathName().toRawUTF8(), ok ? "ok" : "WRITE FAILED");
    return ok && nc == 59 && nf == 140 && nd == 199 ? 0 : 1;
}

//==============================================================================
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    for (int a = 1; a + 1 < argc; ++a)
        if (std::strcmp (argv[a], "--dump-ui-overrides") == 0)
            return dumpUiOverrides (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (argv[a + 1])));
    for (int a = 1; a < argc; ++a)
    {
        if (std::strcmp (argv[a], "--legacy-24") == 0)       { gLegacyKnee = gLegacySvf = true; }
        if (std::strcmp (argv[a], "--legacy-24=knee") == 0)  { gLegacyKnee = true; }
        if (std::strcmp (argv[a], "--legacy-24=svf") == 0)   { gLegacySvf = true; }
        if (std::strcmp (argv[a], "--long") == 0)            { gLong = true; }
        if (std::strcmp (argv[a], "--no-c7") == 0)           { gNoC7 = true; }
        if (std::strncmp (argv[a], "--sect=", 7) == 0)       { gSect = juce::String (argv[a] + 7); }
    }
    bool only26 = false;
    for (int a = 1; a < argc; ++a)
        only26 = only26 || std::strcmp (argv[a], "--only26") == 0;
    auto rc = [] { return failures != 0 ? 1 : (gNoVerdict ? 2 : 0); };
    if (only26)
    {
        engineRows26();
        engineRows26b();
        std::printf ("\n%d PASS, %d FAIL -- %s (2.6 sections only%s)\n", passes, failures,
                     failures == 0 ? (gNoVerdict ? "NO VERDICT (C7 contended)" : "ALL PASS") : "FAILURES", gLong ? ", --long" : "");
        return rc();
    }
    // Iteration aid (3.1 / 3.2 / 3.3): `--only3N` runs that phase's sections alone.
    if (argc > 1 && std::strcmp (argv[1], "--only33") == 0)
    {
        engineRows33();
        std::printf ("\n%d PASS, %d FAIL -- %s (3.3 sections only)\n", passes, failures, failures == 0 ? "ALL PASS" : "FAILURES");
        return failures == 0 ? 0 : 1;
    }
    if (argc > 1 && std::strcmp (argv[1], "--only32") == 0)
    {
        engineRows32();
        std::printf ("\n%d PASS, %d FAIL -- %s (3.2 sections only)\n", passes, failures, failures == 0 ? "ALL PASS" : "FAILURES");
        return failures == 0 ? 0 : 1;
    }
    if (argc > 1 && std::strcmp (argv[1], "--only31") == 0)
    {
        engineRows31();
        std::printf ("\n%d PASS, %d FAIL -- %s (3.1 sections only)\n", passes, failures, failures == 0 ? "ALL PASS" : "FAILURES");
        return failures == 0 ? 0 : 1;
    }
    if (gLegacyKnee || gLegacySvf)
        std::printf ("[legacy-24] knee bypass %s, svfMake forced 1 %s\n", gLegacyKnee ? "ON" : "off", gLegacySvf ? "ON" : "off");
    if (argc > 1 && std::strcmp (argv[1], "--only25b") == 0)
    {
        engineRows25b();
        engineRows25bb();
        std::printf ("\n%d PASS, %d FAIL -- %s (2.5b sections only)\n", passes, failures, failures == 0 ? "ALL PASS" : "FAILURES");
        return failures == 0 ? 0 : 1;
    }
    if (argc > 1 && std::strcmp (argv[1], "--only25a") == 0)
    {
        engineRows25a();
        engineRows25ab();
        std::printf ("\n%d PASS, %d FAIL -- %s (2.5a sections only)\n", passes, failures, failures == 0 ? "ALL PASS" : "FAILURES");
        return failures == 0 ? 0 : 1;
    }
    if (argc > 1 && std::strcmp (argv[1], "--only24") == 0)
    {
        engineRows24();
        engineRows24b();
        std::printf ("\n%d PASS, %d FAIL -- %s (2.4 sections only)\n", passes, failures, failures == 0 ? "ALL PASS" : "FAILURES");
        return failures == 0 ? 0 : 1;
    }
    // Iteration aid (2.3): `--only23` runs the 2.3 sections alone. The gate run
    // (no arguments) always runs every section.
    if (argc > 1 && std::strcmp (argv[1], "--only23") == 0)
    {
        engineRows23();
        engineRows23b();
        std::printf ("\n%d PASS, %d FAIL -- %s (2.3 sections only)\n", passes, failures, failures == 0 ? "ALL PASS" : "FAILURES");
        return failures == 0 ? 0 : 1;
    }
    std::printf ("O-Granulator render harness (Stage 1 + Stage 2 / 2.1–2.6 + Stage 3 / 3.1–3.3%s)  goldens: %s\n", gLong ? ", --long" : "", GOLDEN_DIR);
    paramRows();
    busRows();
    stateRows();
    scaleRows();
    engineRows();
    engineRows22();
    engineRows22b();
    engineRows23();
    engineRows23b();
    engineRows24();
    engineRows24b();
    engineRows25a();
    engineRows25ab();
    engineRows25b();
    engineRows25bb();
    engineRows26();
    engineRows26b();
    engineRows31();
    engineRows32();
    engineRows33();
    std::printf ("\n%d PASS, %d FAIL -- %s\n", passes, failures,
                 failures == 0 ? (gNoVerdict ? "NO VERDICT (C7 contended)" : "ALL PASS") : "FAILURES");
    return rc();
}
