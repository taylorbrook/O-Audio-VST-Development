/*
  ==============================================================================

    O-GranulatorInstrument — DSP-01 parity: PARENT driver
    (PLAN-2.2 T3 / PD5; RESEARCH-2.2 §1–§3)

    Compiled OUT OF TREE by tests/parity/run.sh against O-Granulator at the
    fork pin 82406fce (git archive), as a processor console with
    OGRAN_TEST_HOOKS=1. Nothing here enters our CMake graph (P21).

      parent_render <outdir>

    Writes, per row of ParityRows.h (the single source of truth shared with our
    harness):
      <row>.f32         L/R interleaved float32, from T_e (null / stats) or 0 (GV)
      <row>.grains.tsv  one SpawnRecord per line (null / stats rows)
      manifest.tsv      row W0 A delta F T_f T_e landed total sizeRaw densityRaw level grains rmsDb
      activesets.tsv    SC1: parent ActiveSet members + [lo, hi) per case
      customtables.tsv  SC3: parent tableFromText via setCustomScale per text

    Self-checks (exit 1 on any failure — a check of the derivation, never a
    tuning step): every null-row grain has anchor == A and writeHead == W0;
    every parameter landed; every audio row's RMS >= −70 dBFS.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ParityRows.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    using Proc = GranulatorProcessor;

    juce::RangedAudioParameter* param (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                if (rp->getParameterID() == id) return rp;
        return nullptr;
    }

    int landed = 0, total = 0;

    /** Exact-raw setP (parent engine_gates.h:110-131 idiom, wider search). */
    bool setP (Proc& p, const juce::String& id, float v)
    {
        ++total;
        auto* rp = param (p, id);
        auto* raw = p.parameters.getRawParameterValue (id);
        if (rp == nullptr || raw == nullptr) { std::fprintf (stderr, "  !! unknown param %s\n", id.toRawUTF8()); return false; }
        const float n0 = rp->convertTo0to1 (v);
        float up = n0, dn = n0;
        for (int k = 0; k < 2048; ++k)
        {
            for (float c : { up, dn })
            {
                rp->setValueNotifyingHost (c);
                if (juce::exactlyEqual (raw->load(), v)) { ++landed; return true; }
            }
            up = std::nextafter (up, 2.0f);
            dn = std::nextafter (dn, -1.0f);
        }
        rp->setValueNotifyingHost (n0);
        const auto r = rp->getNormalisableRange();
        if (std::abs (raw->load() - v) <= 1.0e-5f * (r.end - r.start)) { ++landed; return true; }   // skewed: nearest
        std::fprintf (stderr, "  !! %s = %.9g landed at %.9g\n", id.toRawUTF8(), (double) v, (double) raw->load());
        return false;
    }

    float rawOf (Proc& p, const juce::String& id) { auto* r = p.parameters.getRawParameterValue (id); return r ? r->load() : 0.0f; }
    juce::String sid (int s, const std::string& suffix) { return "s" + juce::String (s) + "_" + juce::String (suffix); }

    bool writeF32 (const juce::File& f, const std::vector<float>& v)
    {
        f.deleteFile();
        juce::FileOutputStream os (f);
        return os.openedOk() && os.write (v.data(), v.size() * sizeof (float));
    }

    double rmsDb (const std::vector<float>& v)
    {
        double e = 0.0;
        for (float x : v) e += (double) x * x;
        return 10.0 * std::log10 (std::max (1.0e-300, e / std::max<size_t> (1, v.size())));
    }

    int failures = 0;
    void fail (const juce::String& m) { ++failures; std::printf ("  [FAIL] %s\n", m.toRawUTF8()); }

    //==========================================================================
    int renderRow (const parity::Row& row, const juce::File& out, juce::FileOutputStream& manifest)
    {
        landed = total = 0;
        std::unique_ptr<Proc> p (dynamic_cast<Proc*> (createPluginFilter()));
        auto& eng = p->getEngineForTest();
        const bool gv = row.kind == parity::Kind::gv;

        // Globals: instrument-equivalent chain (mix 100 %, output 0 dB, no FX / fb / reverb).
        setP (*p, "mix", 100.0f);
        setP (*p, "output_gain", 0.0f);
        setP (*p, "input_gain", 0.0f);
        setP (*p, "feedback", 0.0f);
        setP (*p, "reverb_mix", 0.0f);
        setP (*p, "freeze", 0.0f);
        if (! gv)
        {
            setP (*p, "grain_limit", 3.0f);           // 128
            setP (*p, "buffer_length", (float) parity::kBufLenS);
            for (int s = 1; s <= 4; ++s)
                setP (*p, sid (s, "enable"), 0.0f);    // enabled at T_e
            const int s0 = parity::isStatsKind (row.kind) ? 1 : row.stream;
            const int s1 = parity::isStatsKind (row.kind) ? row.streams : row.stream;
            for (int s = s0; s <= s1; ++s)
                for (const auto& kv : row.sp)
                    setP (*p, sid (s, kv.id), kv.v);
            eng.hooks.freezeNoMinSpray = true;
        }
        p->setSeed (row.seed);
        p->prepareToPlay (parity::kFs, 512);

        const int sRow = parity::isStatsKind (row.kind) ? 1 : row.stream;
        const float sizeRaw    = gv ? rawOf (*p, "s1_size") : rawOf (*p, sid (sRow, "size"));
        const float densityRaw = gv ? rawOf (*p, "s1_density") : rawOf (*p, sid (sRow, "density"));

        parity::Geometry g;
        if (! gv)
        {
            g = parity::solve (row, sizeRaw);
            if (! g.ok && parity::isNullKind (row.kind)) { fail (juce::String (row.id) + " geometry: " + g.why); return 1; }
        }

        const juce::int64 tStart = gv ? 0 : parity::kTe;
        const juce::int64 len    = (juce::int64) std::llround (row.seconds * parity::kFs);
        const juce::int64 tEnd   = tStart + len;
        std::vector<float> rec;
        rec.reserve ((size_t) (2 * len));

        juce::FileOutputStream* gl = nullptr;
        std::unique_ptr<juce::FileOutputStream> glOwner;
        if (! gv)
        {
            auto f = out.getChildFile (juce::String (row.id) + ".grains.tsv");
            f.deleteFile();
            glOwner = std::make_unique<juce::FileOutputStream> (f);
            gl = glOwner.get();
            gl->writeText ("#onsetSample\twriteHead\tstream\tdir\tlevel\tdelay\tanchor\tfrac\trate\tlen\tgainL\tgainR\twindowStep\ttiltT\tpitchCents\tglobalIndex\tfxActive\tsvfMode\tfcHz\tringHz\tcrushed\tdriveD\n",
                           false, false, "\n");
        }

        auto* freezeP = param (*p, "freeze");
        juce::AudioBuffer<float> blk (2, 512);
        juce::MidiBuffer midi;
        std::uint64_t drained = 0;
        int grains = 0, anchorBad = 0, headBad = 0;
        int fxBitsBad = 0, crushBad = 0, statBit = 0, crushedN = 0;   // 2.3 self-checks
        juce::int64 pos = 0;
        while (pos < tEnd)
        {
            if (! gv && pos == parity::kTf && freezeP != nullptr)
                freezeP->setValueNotifyingHost (1.0f);
            if (! gv && pos == parity::kTe)
            {
                const int s0 = parity::isStatsKind (row.kind) ? 1 : row.stream;
                const int s1 = parity::isStatsKind (row.kind) ? row.streams : row.stream;
                for (int s = s0; s <= s1; ++s)
                    param (*p, sid (s, "enable"))->setValueNotifyingHost (1.0f);
            }
            juce::int64 n = 512;
            n = std::min (n, tEnd - pos);
            if (! gv)
            {
                if (pos < parity::kTf) n = std::min (n, parity::kTf - pos);
                else if (pos < parity::kTe) n = std::min (n, parity::kTe - pos);
            }
            const int ni = (int) n;
            blk.setSize (2, ni, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
            {
                float* d = blk.getWritePointer (ch);
                for (int i = 0; i < ni; ++i)
                    d[i] = gv ? parity::gvNoise (pos + i, ch) : parity::parentInput (g, pos + i, ch, row.matPeak);
            }
            p->processBlock (blk, midi);

            if (pos + ni > tStart)
                for (int i = 0; i < ni; ++i)
                    if (pos + i >= tStart)
                    {
                        rec.push_back (blk.getSample (0, i));
                        rec.push_back (blk.getSample (1, i));
                    }

            // Drain the 4096-entry spawn log after EVERY block (P15).
            const auto& dg = eng.diag;
            if (dg.logCount - drained > (std::uint64_t) ogran::Engine::kSpawnLogSize)
                fail (juce::String (row.id) + " spawn log overran between drains");
            for (; drained < dg.logCount; ++drained)
            {
                const auto& r = dg.record (drained);
                ++grains;
                if (gl != nullptr)
                {
                    char line[800];
                    // 2.3 (PLAN-2.3 PD11): the FX columns are APPENDED (the 2.2 readers take lower bounds).
                    std::snprintf (line, sizeof line, "%lld\t%lld\t%d\t%d\t%d\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.9g\t%.9g\t%d\t%.9g\t%.9g\t%llu\t%u\t%d\t%.9g\t%.9g\t%d\t%.9g\n",
                                   (long long) r.onsetSample, (long long) r.writeHead, r.stream, r.dir, r.level, r.delay, r.anchor,
                                   r.frac, r.rate, r.len, (double) r.gainL, (double) r.gainR, r.windowStep, (double) r.tiltT,
                                   (double) r.pitchCents, (unsigned long long) r.globalIndex,
                                   (unsigned) r.fxActive, r.svfMode, (double) r.fcHz, (double) r.ringHz, r.crushed ? 1 : 0, (double) r.driveD);
                    gl->writeText (line, false, false, "\n");
                }
                if (row.kind == parity::Kind::fxNull)
                    fxBitsBad += r.fxActive == parity::expectedFxBits (row.sp) ? 0 : 1;
                if (row.kind == parity::Kind::fxStats)
                {
                    const bool fxRow = juce::String (row.id) == "PS7";
                    if (fxRow)
                        crushBad += (((r.fxActive & 4u) != 0) == r.crushed) ? 0 : 1;
                    statBit += (r.fxActive & (juce::String (row.id) == "PS5" ? 1u : juce::String (row.id) == "PS6" ? 8u : 4u)) != 0 ? 1 : 0;
                    crushedN += r.crushed ? 1 : 0;
                }
                if (parity::isNullKind (row.kind))
                {
                    anchorBad += juce::exactlyEqual (r.anchor, (double) g.A) ? 0 : 1;
                    headBad   += r.writeHead == parity::kW0 ? 0 : 1;
                }
            }
            pos += ni;
        }
        if (gl != nullptr) gl->flush();

        writeF32 (out.getChildFile (juce::String (row.id) + (gv ? ".f32" : ".f32")), rec);
        const double db = rmsDb (rec);
        double peak = 0.0;
        for (float x : rec) peak = std::max (peak, (double) std::abs (x));

        juce::String m;
        m << row.id << "\t" << (juce::int64) parity::kW0 << "\t" << (juce::int64) g.A << "\t" << (juce::int64) g.delta << "\t"
          << (juce::int64) parity::kF << "\t" << (juce::int64) parity::kTf << "\t" << (juce::int64) parity::kTe << "\t"
          << landed << "\t" << total << "\t" << juce::String (sizeRaw, 9) << "\t" << juce::String (densityRaw, 9) << "\t"
          << g.level << "\t" << grains << "\t" << juce::String (db, 3);
        // 2.3 (PD11): landed FX raws (stream sRow) + max |out|, appended.
        for (const auto& fid : parity::fxIds())
            m << "\t" << juce::String (gv ? 0.0f : rawOf (*p, sid (sRow, fid)), 9);
        m << "\t" << juce::String (peak, 9) << "\n";
        manifest.writeText (m, false, false, "\n");

        std::printf ("  %-5s grains %5d  landed %d/%d  rms %7.2f dBFS  delta %lld  A %lld  l %d%s\n", row.id.c_str(), grains,
                     landed, total, db, (long long) g.delta, (long long) g.A, g.level,
                     parity::isNullKind (row.kind) ? (juce::String ("  anchorBad ") + juce::String (anchorBad) + " headBad " + juce::String (headBad)).toRawUTF8() : "");

        if (landed != total) fail (juce::String (row.id) + " params landed " + juce::String (landed) + "/" + juce::String (total));
        if (db < -70.0)      fail (juce::String (row.id) + " reference RMS below -70 dBFS (vacuous)");
        if (grains == 0)     fail (juce::String (row.id) + " no grains");
        if (row.kind == parity::Kind::fxNull)
        {
            std::printf ("        fx: peak %.4f  fxActive bad %d (expect %u)\n", peak, fxBitsBad, (unsigned) parity::expectedFxBits (row.sp));
            if (peak > 0.45) fail (juce::String (row.id) + " INVALID: parent peak " + juce::String (peak, 4) + " > 0.45 (PD5)");
            if (fxBitsBad > 0) fail (juce::String (row.id) + " fxActive != expected on " + juce::String (fxBitsBad) + " grains");
        }
        if (row.kind == parity::Kind::fxStats && grains > 0)
        {
            const double bitFrac = (double) statBit / grains, ph = (double) crushedN / grains;
            std::printf ("        fx stats: stage bit on %.4f of grains, crushed p %.4f, crush bit != crushed %d\n", bitFrac, ph, crushBad);
            if (juce::String (row.id) != "PS7" && bitFrac < 0.99) fail (juce::String (row.id) + " stage bit on < 99 % of grains (P11)");
            if (juce::String (row.id) == "PS7" && (crushBad > 0 || ! (ph > 0.0 && ph < 1.0)))
                fail (juce::String (row.id) + " crush bit != crushed or p-hat not in (0, 1) (P11)");
        }
        if (anchorBad + headBad > 0)
            fail (juce::String (row.id) + " derivation self-check: anchor != A on " + juce::String (anchorBad)
                  + ", writeHead != W0 on " + juce::String (headBad));
        return 0;
    }

    //==========================================================================
    juce::String hexF (float v) { std::uint32_t b; std::memcpy (&b, &v, 4); return juce::String::toHexString ((int) b).paddedLeft ('0', 8); }
    juce::String hex64 (std::uint64_t v) { return juce::String::toHexString ((juce::int64) v).paddedLeft ('0', 16); }

    void scaleDumps (const juce::File& out)
    {
        // SC1: ActiveSet members + window per case.
        auto f = out.getChildFile ("activesets.tsv");
        f.deleteFile();
        juce::FileOutputStream os (f);
        os.writeText ("#case\tchoice\tmaskKind\tmask\tspanCents\tpitchSt\tlo\thi\tcount\ttruncated\tmembers(hex float)\n", false, false, "\n");
        int cases = 0;
        for (int c = 1; c <= 31; ++c)
        {
            for (int variant = 0; variant < (c == 31 ? 2 : 1); ++variant)
            {
                std::unique_ptr<Proc> p (dynamic_cast<Proc*> (createPluginFilter()));
                juce::String tag (c);
                if (c == 31)
                {
                    juce::String bad;
                    const auto err = p->setCustomScale ("sc", variant == 0 ? "edo 19" : "edo 64", bad);
                    if (err.isNotEmpty()) fail ("SC custom parse " + err);
                    tag << (variant == 0 ? "a" : "b");
                }
                const auto* table = p->getCollectionTable (c);
                if (table == nullptr) { fail ("SC table null for choice " + juce::String (c)); continue; }
                for (int mk = 0; mk < 3; ++mk)
                {
                    const std::uint64_t want = mk == 0 ? ~std::uint64_t (0) : (mk == 1 ? parity::kScFixedMask : parity::kScBit63);
                    p->setDegreeMask (1, c, want);
                    const std::uint64_t mask = p->getDegreeMask (1, c);
                    for (float span : parity::scSpans())
                        for (double pitch : parity::scPitches())
                        {
                            auto as = std::make_unique<ogran::ActiveSet>();
                            as->rebuildIfNeeded (c, mask, span, 0u, *table);
                            int lo = 0, hi = 0;
                            as->window (pitch, (double) span, lo, hi);
                            juce::String line;
                            line << tag << "_" << mk << "_" << (int) span << "_" << (int) pitch << "\t" << c << "\t" << mk << "\t"
                                 << hex64 (mask) << "\t" << hexF (span) << "\t" << juce::String (pitch, 1) << "\t" << lo << "\t" << hi
                                 << "\t" << as->size() << "\t" << (as->wasTruncated() ? 1 : 0) << "\t";
                            for (int i = 0; i < as->size(); ++i)
                                line << (i ? "," : "") << hexF (as->data()[i]);
                            os.writeText (line + "\n", false, false, "\n");
                            ++cases;
                        }
                }
            }
        }
        os.flush();
        std::printf ("  activesets.tsv: %d cases\n", cases);

        // SC3: custom tables (fresh processor per text: a failure keeps the 12-EDO default).
        auto g = out.getChildFile ("customtables.tsv");
        g.deleteFile();
        juce::FileOutputStream ts (g);
        ts.writeText ("#index\terror\tbadToken\tcount\tperiod(hex)\tcents(hex)\n", false, false, "\n");
        int k = 0;
        for (const auto& text : parity::scCustomTexts())
        {
            std::unique_ptr<Proc> p (dynamic_cast<Proc*> (createPluginFilter()));
            juce::String bad;
            const auto err = p->setCustomScale ("t", juce::String (text), bad);
            const auto t = p->getCustomTable();
            juce::String line;
            line << k++ << "\t" << (err.isEmpty() ? "-" : err) << "\t" << (bad.isEmpty() ? "-" : bad) << "\t" << t.count << "\t" << hexF (t.period) << "\t";
            for (int i = 0; i < t.count; ++i)
                line << (i ? "," : "") << hexF (t.cents[(size_t) i]);
            ts.writeText (line + "\n", false, false, "\n");
        }
        ts.flush();
        std::printf ("  customtables.tsv: %d texts\n", k);
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc != 2 && argc != 3)
    {
        std::fprintf (stderr, "usage: parent_render <outdir> [rowPrefix]\n");
        return 2;
    }
    const juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[1]));
    out.createDirectory();
    const juce::String prefix = argc == 3 ? juce::String::fromUTF8 (argv[2]) : juce::String();

    std::printf ("O-Granulator @ 82406fce parity reference -> %s\n", out.getFullPathName().toRawUTF8());
    auto mf = out.getChildFile ("manifest.tsv");
    mf.deleteFile();
    juce::FileOutputStream manifest (mf);
    manifest.writeText ("#row\tW0\tA\tdelta\tF\tT_f\tT_e\tlanded\ttotal\tsizeRaw\tdensityRaw\tlevel\tgrains\trmsDb\tfilterType\tcutoff\tfilterRand\tres\tdrive\tcrush\tcrushProb\tringFreq\tringRand\tringMix\tpeak\n", false, false, "\n");

    for (const auto& row : parity::rows())
    {
        if (! row.parentOf.empty()) continue;                     // ours-only control: reuses the parent render
        if (prefix.isNotEmpty() && ! juce::String (row.id).startsWith (prefix)) continue;
        renderRow (row, out, manifest);
    }
    manifest.flush();
    if (prefix.isEmpty() || prefix.startsWith ("SC"))
        scaleDumps (out);

    std::printf ("parent_render: %d self-check failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
