/*
  ==============================================================================

    O-GrainScatter — pitch-collections null gate (v3.6.0)

    v3.6.0 moved namespace PitchCollections out of Source/dsp/PitchEngine.h into
    the shared module modules/tuning/pitch-collections (1.0.0). The move must be
    audio-, state- and parse-neutral. Two binaries from this one driver:

      O-GrainScatter-pitch-null-v351 <dir>   compiled against v3.5.1 (commit
                                             0e1117b6, extracted with git archive at
                                             configure time): renders every job and
                                             dumps it, plus the parse corpus, to <dir>.
      O-GrainScatter-pitch-null <dir>        this build: re-renders and compares
                                             each dump byte for byte.

    Jobs (2 s, 48 kHz stereo, seeded noise + 220 Hz tone, wet only)
      R1  every collection Id 0..31 (Free and Custom included) x 3 seeds,
          Random mode, Pitch Rnd 70, Range 24 (seed 3 at a seeded variable split)
      R2  Ladder Up / Ladder Down / Pendulum x direction Up / Down x {Major, 53-EDO,
          Bohlen-Pierce, 7-Limit Just, Harmonics 8-16, Custom} x root C / G
      R3  degree masks (alternating, single degree, all zero) on {Chromatic,
          19-EDO, 53-EDO, Custom}
      R4  custom text corpus (default 12-EDO, ratios, cents, folding, negative,
          64 degrees, 3/1 period) + 3 .scl bodies, then rendered on Custom
      P   parse corpus: error | count | period hex | cents hex for the module's
          v3.5.1 corpus (text + Scala) through PitchCollections::tableFromText /
          tableFromScala — the 3-arg / 5-arg API both trees have

    Verdicts (this build)
      every R* dump byte-identical; parse.txt byte-identical;
      POSITIVE CONTROL: parse-edo.txt ("edo 19", "19-edo", "EDO 13 3/1") MUST differ
      (v3.5.1: badPitch; v3.6.0: parsed);
      seed liveness: the 3 seeds of each R1 row render pairwise different;
      signal: every render finite with RMS > -90 dBFS.

    Exit 0 = every row PASS. C++ only — no UI, no binary data.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    #include "../../../../modules/tuning/pitch-collections/tests/corpus.inc"

    constexpr double kSampleRate = 48000.0;
    constexpr int    kMaxBlock   = 512;
    constexpr int    kLength     = 2 * 48000;

    int failures = 0, passes = 0;

    [[maybe_unused]] void row (bool ok, const juce::String& name, const juce::String& detail = {})
    {
        if (! ok || detail.isNotEmpty())
            std::printf ("  [%s] %-60s %s\n", ok ? "PASS" : "FAIL", name.toRawUTF8(), detail.toRawUTF8());
        if (ok) ++passes; else ++failures;
    }

    [[maybe_unused]] double toDb (double x) { return x > 0.0 ? 20.0 * std::log10 (x) : -400.0; }

    using Params = std::map<juce::String, float>;

    struct Job
    {
        juce::String name;
        Params params;
        uint64_t seed = 0xC0FFEEull;
        bool variableSplit = false;
        std::function<void (GrainScatterProcessor&)> setup;   // masks / custom table
    };

    juce::AudioBuffer<float> stimulus()
    {
        juce::AudioBuffer<float> b (2, kLength);
        juce::Random r (0x5EEDull);
        for (int i = 0; i < kLength; ++i)
        {
            const double t = i / kSampleRate;
            const float tone = static_cast<float> (0.3 * std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * t));
            b.setSample (0, i, tone + (r.nextFloat() * 2.0f - 1.0f) * 0.05f);
            b.setSample (1, i, tone + (r.nextFloat() * 2.0f - 1.0f) * 0.05f);
        }
        return b;
    }

    std::vector<int> split (bool variable)
    {
        std::vector<int> s;
        juce::Random r (0xB10CCull);
        for (int pos = 0; pos < kLength;)
        {
            const int n = juce::jmin (variable ? 1 + r.nextInt (kMaxBlock) : kMaxBlock, kLength - pos);
            s.push_back (n);
            pos += n;
        }
        return s;
    }

    void setParams (juce::AudioProcessor& p, const Params& params)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
            {
                auto it = params.find (rp->getParameterID());
                if (it != params.end())
                    rp->setValueNotifyingHost (rp->convertTo0to1 (it->second));
            }
    }

    juce::MemoryBlock render (const Job& job, const juce::AudioBuffer<float>& in, double& rmsOut, bool& finite)
    {
        std::unique_ptr<GrainScatterProcessor> p (dynamic_cast<GrainScatterProcessor*> (createPluginFilter()));
        p->setPlayConfigDetails (2, 2, kSampleRate, kMaxBlock);
        setParams (*p, job.params);
        if (job.setup)
            job.setup (*p);
        p->setRngSeed (job.seed);
        p->prepareToPlay (kSampleRate, kMaxBlock);
        p->reset();

        juce::AudioBuffer<float> out (2, kLength), block (2, kMaxBlock);
        juce::MidiBuffer midi;
        int pos = 0;
        for (int n : split (job.variableSplit))
        {
            block.setSize (2, n, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
                block.copyFrom (ch, 0, in, ch, pos, n);
            p->processBlock (block, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, block, ch, 0, n);
            pos += n;
        }

        double s = 0.0;
        finite = true;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kLength; ++i)
            {
                const float v = out.getSample (ch, i);
                finite = finite && std::isfinite (v);
                s += static_cast<double> (v) * v;
            }
        rmsOut = std::sqrt (s / (2.0 * kLength));

        juce::MemoryBlock mb;
        for (int ch = 0; ch < 2; ++ch)
            mb.append (out.getReadPointer (ch), sizeof (float) * static_cast<size_t> (kLength));
        return mb;
    }

    Params base()
    {
        return { { "dry_level", -60.0f }, { "density", 70.0f }, { "grain_size", 80.0f },
                 { "pitch_mode", 0.0f }, { "pitch_random", 70.0f }, { "pitch_range", 24.0f } };
    }

    // Ids (v3.5.1 enum, frozen): Chromatic 0, Major 1, Free 5, Edo19 19, Edo53 23,
    // JustSevenLimit 25, Harmonics8to16 26, BohlenPierce 27, Custom 31.
    std::vector<Job> jobs()
    {
        std::vector<Job> v;
        for (int id = 0; id < 32; ++id)
            for (int s = 1; s <= 3; ++s)
            {
                Job j { "R1-id" + juce::String (id) + "-seed" + juce::String (s), base(), 0xC0FFEEull, false, {} };
                j.params["scale"] = (float) id;
                j.seed = 0x1000ull * (uint64_t) s + 7;
                j.variableSplit = (s == 3);
                v.push_back (j);
            }

        for (int mode = 1; mode <= 3; ++mode)
            for (int dir : { 1, 2 })
                for (int id : { 1, 23, 27, 25, 26, 31 })
                    for (int root : { 0, 7 })
                    {
                        Job j { "R2-m" + juce::String (mode) + "-d" + juce::String (dir) + "-id" + juce::String (id)
                                    + "-r" + juce::String (root), base(), 0xC0FFEEull, false, {} };
                        j.params["pitch_mode"] = (float) mode;
                        j.params["pitch_direction"] = (float) dir;
                        j.params["scale"] = (float) id;
                        j.params["root_note"] = (float) root;
                        v.push_back (j);
                    }

        const std::pair<const char*, juce::String> masks[] = {
            { "alt",    juce::String::repeatedString ("10", 32) },
            { "single", "1" + juce::String::repeatedString ("0", 63) },
            { "zero",   juce::String::repeatedString ("0", 64) },
        };
        for (int id : { 0, 19, 23, 31 })
            for (auto& [mName, bits] : masks)
            {
                Job j { "R3-id" + juce::String (id) + "-" + mName, base(), 0xC0FFEEull, false, {} };
                j.params["scale"] = (float) id;
                const int c = id;
                const juce::String b = bits;
                j.setup = [c, b] (GrainScatterProcessor& p) { p.setPitchMaskString (c, b); };
                v.push_back (j);
            }

        juce::String sixtyFour;
        for (int k = 1; k <= 63; ++k) sixtyFour << juce::String (k * 18) << " ";
        sixtyFour << "1200";
        const juce::String texts[] = { "100 200 300 400 500 600 700 800 900 1000 1100 1200", "5/4 3/2 2/1",
                                       "386.3 701.955 1200", "100 1300 2400", "-100 1200", sixtyFour, "3/2 7/4 3/1" };
        for (int i = 0; i < (int) std::size (texts); ++i)
        {
            Job j { "R4-text" + juce::String (i), base(), 0xC0FFEEull, false, {} };
            j.params["scale"] = 31.0f;
            const juce::String t = texts[i];
            j.setup = [t] (GrainScatterProcessor& p) { p.setCustomPitchesFromText (t, "t"); };
            v.push_back (j);
        }
        const juce::String scls[] = {
            "! a.scl\n!\nThirds\n 3\n! comment\n 5/4\n 3/2\n 2\n",
            "Desc line\n2\n386.3 major third\n1200.0\n",
            "! bp\nBohlen\n 4\n 146.30423\n 7/3\n 1463.0423\n 3/1\n",
        };
        for (int i = 0; i < (int) std::size (scls); ++i)
        {
            Job j { "R4-scl" + juce::String (i), base(), 0xC0FFEEull, false, {} };
            j.params["scale"] = 31.0f;
            const juce::String s = scls[i];
            j.setup = [s] (GrainScatterProcessor& p) { p.setCustomPitchesFromScala (s, "f.scl"); };
            v.push_back (j);
        }
        return v;
    }

    juce::String hexBits (float f) { uint32_t b; std::memcpy (&b, &f, 4); return juce::String::toHexString ((juce::int64) b).paddedLeft ('0', 8); }

    juce::String tableLine (bool ok, const juce::String& err, const PitchCollections::Table& t)
    {
        juce::String s = juce::String (ok ? "ok" : "fail") + " | " + err + " | ";
        if (ok)
        {
            s << t.count << " | " << hexBits (t.period) << " |";
            for (int i = 0; i < t.count; ++i) s << " " << hexBits (t.cents[(size_t) i]);
        }
        return s + "\n";
    }

    bool isEdoPositive (const juce::String& s) { return s == "edo 19" || s == "19-edo"; }

    juce::String parseDump (bool edoOnly)
    {
        juce::String out;
        if (edoOnly)
        {
            for (auto* s : { "edo 19", "19-edo", "EDO 13 3/1" })
            {
                PitchCollections::Table t; juce::String e;
                out << s << " => " << tableLine (PitchCollections::tableFromText (s, t, e), e, t);
            }
            return out;
        }
        for (auto* c : kTextCorpus)
        {
            const auto s = juce::String (juce::CharPointer_UTF8 (c));
            if (isEdoPositive (s)) continue;
            PitchCollections::Table t; juce::String e;
            out << s.replace ("\n", "\\n") << " => " << tableLine (PitchCollections::tableFromText (s, t, e), e, t);
        }
        for (auto* c : kScalaCorpus)
        {
            PitchCollections::Table t; juce::String e, asText, desc;
            const bool ok = PitchCollections::tableFromScala (juce::String (juce::CharPointer_UTF8 (c)), t, asText, desc, e);
            out << "scl => " << asText << " / " << desc << " / " << tableLine (ok, e, t);
        }
        return out;
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2) { std::printf ("usage: %s <dir>\n", argv[0]); return 2; }
    juce::File dir (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    const auto in = stimulus();
    const auto all = jobs();

#ifdef GS_PITCHNULL_V351
    dir.createDirectory();
    int n = 0;
    for (auto& j : all)
    {
        double r = 0.0; bool fin = true;
        const auto mb = render (j, in, r, fin);
        if (! dir.getChildFile (j.name + ".f32").replaceWithData (mb.getData(), mb.getSize())) return 1;
        ++n;
    }
    dir.getChildFile ("parse.txt").replaceWithText (parseDump (false), false, false, "\n");
    dir.getChildFile ("parse-edo.txt").replaceWithText (parseDump (true), false, false, "\n");
    std::printf ("v3.5.1 reference: %d renders + parse.txt + parse-edo.txt -> %s\n", n, dir.getFullPathName().toRawUTF8());
    return 0;
#else
    std::printf ("\n-- R1-R4: renders byte-identical to v3.5.1 (0e1117b6)\n");
    std::map<juce::String, juce::MemoryBlock> r1;
    int identical = 0;
    double minRms = 1.0;
    for (auto& j : all)
    {
        double r = 0.0; bool fin = true;
        const auto mb = render (j, in, r, fin);
        minRms = juce::jmin (minRms, r);
        juce::MemoryBlock ref;
        const bool have = dir.getChildFile (j.name + ".f32").loadFileAsData (ref);
        const bool same = have && ref == mb;
        identical += same ? 1 : 0;
        row (same, j.name + " == v3.5.1", have ? juce::String() : "reference missing (run -v351 first)");
        row (fin && toDb (r) > -90.0, j.name + " finite, RMS > -90 dBFS", fin && toDb (r) > -90.0 ? juce::String() : juce::String (toDb (r), 1) + " dBFS");
        if (j.name.startsWith ("R1-"))
            r1[j.name] = mb;
    }
    std::printf ("  %d / %d renders byte-identical; quietest render %.1f dBFS\n", identical, (int) all.size(), toDb (minRms));

    std::printf ("\n-- seed liveness (R1: 3 seeds pairwise different per Id)\n");
    int live = 0;
    for (int id = 0; id < 32; ++id)
    {
        const auto k = [id] (int s) { return "R1-id" + juce::String (id) + "-seed" + juce::String (s); };
        const bool ok = ! (r1[k (1)] == r1[k (2)]) && ! (r1[k (1)] == r1[k (3)]) && ! (r1[k (2)] == r1[k (3)]);
        live += ok ? 1 : 0;
        row (ok, "R1 id " + juce::String (id) + " seeds pairwise distinct");
    }
    std::printf ("  %d / 32 Ids seed-live\n", live);

    std::printf ("\n-- P: parse corpus\n");
    const auto parseNew = parseDump (false), edoNew = parseDump (true);
    const auto parseOld = dir.getChildFile ("parse.txt").loadFileAsString();
    const auto edoOld   = dir.getChildFile ("parse-edo.txt").loadFileAsString();
    row (parseOld.isNotEmpty() && parseOld == parseNew, "parse.txt byte-identical (" + juce::String (juce::StringArray::fromLines (parseNew).size() - 1) + " lines)");
    row (edoOld.isNotEmpty() && edoOld != edoNew, "POSITIVE CONTROL parse-edo.txt differs (edo now parses)");
    std::printf ("     v3.5.1: %s     v3.6.0: %s", edoOld.replace ("\n", "\n             ").toRawUTF8(), edoNew.replace ("\n", "\n             ").toRawUTF8());
    row (! edoNew.contains ("fail"), "v3.6.0 parses every edo control input");

    std::printf ("\n%d PASS, %d FAIL -- %s\n", passes, failures, failures == 0 ? "ALL PASS" : "FAILURES");
    return failures == 0 ? 0 : 1;
#endif
}
