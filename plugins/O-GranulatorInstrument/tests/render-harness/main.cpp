/*
  ==============================================================================

    O-GranulatorInstrument: render harness (Stage 1 gate; Stage 2 extends it)

      O-GranulatorInstrument-render-harness [--golden-dir <dir>]
      prints "N PASS, M FAIL"; exit code = number of FAILs (capped at 255)

    Goldens come from the LOCKED spec (tests/golden/extract_spec.py ->
    params.tsv / mod-dests.tsv / mod-sources.tsv), never from the code.
    --golden-dir points the run at another golden set (negative controls,
    PLAN P10); the default is GOLDEN_DIR.

      G1  293 params, ID order == params.tsv (and allIdsInOrder())
      G2  per-ID type; totals 200 Float / 74 Choice / 19 Bool
      G3  version hint 1 everywhere; host names byte-equal to the golden
      G4  Float start / end / interval / skew == golden; 25 setSkewForCentre
          rows (== reference AND +-1e-4 of the spec skew); 14 raw-skew rows
          exact; 6 interval-1 rows (interval 1, get() == default exactly);
          43 negative-start rows interval 0; default normalised ==
          convertTo0to1 (spec) exactly, engineering default within 1e-6 of span
      G5  74 choice lists: strings, order, default index
      G6  Bools: s1_enable and the four sN_fb_enable On, the other 14 Off
      D1  kModDestIds == mod-dests.tsv (150, order)   D2 every dest != None is a Float
      D3  16 modM_dest lists == None + golden host names   D4 table version 2
      D5  the literal == the formula b = 2 + 36 (N - 1)
      SRC1 16 modM_source lists == mod-sources.tsv (18); kModSources (+ class) == golden
      B1  buses + MIDI flags; B3 latency 0 (44.1 k / 96 k), tail 0
      M1  MidiScript self-test (absolute sample times, never block-relative)
      R1  sidechain-only render -> output bitwise +0.0f and finite
          (R1v input-level vacuity guard, R1n comparator negative control)
      S1a S1b S2 S3 S4 S5  state codec
      H1  seed injection + loadSlotSync stubs link   H2 VST3 kAux (getPluginHasMainInput false)

  ==============================================================================
*/

// Phase 2.1: engine_gates_21.h pumps the message loop with CFRunLoopRunInMode.
// CoreFoundation must be seen BEFORE `using namespace juce` (MacTypes Point vs
// juce::Point ambiguity); its own later include is then a guarded no-op.
#if JUCE_MAC || defined (__APPLE__)
 #include <CoreFoundation/CoreFoundation.h>
#endif
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GiUiJson.h"   // PLAN-3.1 PD10: the encoders the editor uses (UTL31, --dump-ui-overrides)
#include "dsp/GiHash.h"
#include "dsp/GiFactory.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <vector>
#include <regex>
#include <cstdlib>
#include <string>
#include <pwd.h>
#include <unistd.h>
#include <mach-o/dyld.h>   // _NSGetExecutablePath (PLAN-4 PD15 re-exec)

#if defined (__clang__)
 #pragma clang diagnostic ignored "-Wfloat-equal"
#endif

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

#ifndef GOLDEN_DIR
 #error "GOLDEN_DIR must point at tests/golden"
#endif

namespace Ids = GiParamIds;

namespace
{
    int failures = 0, passes = 0;
    juce::File gGoldenDir { juce::String (GOLDEN_DIR) };

    void row (bool ok, const juce::String& name, const juce::String& detail = {})
    {
        std::printf ("  [%s] %-66s %s\n", ok ? "PASS" : "FAIL", name.toRawUTF8(), detail.toRawUTF8());
        std::fflush (stdout);
        if (ok) ++passes; else ++failures;
    }

    // Collects sub-failures of a many-item row; prints the first few.
    struct Tally
    {
        int bad = 0; juce::StringArray notes;
        void check (bool ok, const juce::String& what) { if (! ok) { ++bad; if (notes.size() < 4) notes.add (what); } }
        juce::String detail (int total) const { return juce::String (total - bad) + "/" + juce::String (total) + (bad ? "  e.g. " + notes.joinIntoString ("; ") : juce::String()); }
    };

    /** PLAN-4 PD15: from Stage 4 the processor constructor writes the factory
        bank under ~/Library/O-GranulatorInstrument/Presets. main() re-execs
        once with CFFIXED_USER_HOME = a per-run temp dir (JUCE resolves home
        through NSHomeDirectory(), which ignores HOME), so every processor
        sees the TEMP home. The REAL home is captured before the re-exec
        (OGI_REAL_HOME) for the guards that must look at the real folders:
        PD24, GD1, HM1 and the overrides hygiene check. */
    juce::File realHome()
    {
        static const juce::File f = []
        {
            if (const char* h = std::getenv ("OGI_REAL_HOME"))
                return juce::File (juce::String::fromUTF8 (h));
            return juce::File::getSpecialLocation (juce::File::userHomeDirectory);
        }();
        return f;
    }
    juce::File realMusic() { return realHome().getChildFile ("Music"); }   // JUCE mac userMusicDirectory = ~/Music
    juce::File realLibraryDir() { return realHome().getChildFile ("Library").getChildFile ("O-GranulatorInstrument"); }
    juce::File tempHome()
    {
        const char* h = std::getenv ("OGI_HARNESS_HOME");
        return h != nullptr ? juce::File (juce::String::fromUTF8 (h)) : juce::File();
    }

    /** PLAN-2.6 PD17 / P19: every processor's Recordings folder is a temp
        subdir with a NON-ASCII component (built through juce::File /
        String::fromUTF8, never String (const char*)); deleted at exit. The
        real ~/Music/Ouaricon/... folder is never resolved by the harness. */
    juce::File harnessRecordingsDir()
    {
        static const juce::File dir = []
        {
            auto base = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile ("ogi-harness-rec-" + juce::String (juce::Time::currentTimeMillis()));
            return base.getChildFile (juce::String::fromUTF8 ("R\xc3\xa9" "cordings \xe2\x9c\x93"));
        }();
        return dir;
    }

    /** PLAN-3.2 PD24: the Imports folder of every processor, a sibling of the
        Recordings temp dir (same temp tree, also a NON-ASCII component). */
    juce::File harnessImportsDir()
    {
        static const juce::File dir = harnessRecordingsDir().getParentDirectory()
                                          .getChildFile (juce::String::fromUTF8 ("Imp\xc3\xb4rts \xe2\x9c\x93"));
        return dir;
    }

    /** Removes the whole harness temp tree (Recordings + Imports) at exit. */
    struct RecDirGuard
    {
        ~RecDirGuard() { harnessRecordingsDir().getParentDirectory().deleteRecursively(); }
    };

    std::unique_ptr<GranulatorInstrumentProcessor> make()
    {
        std::unique_ptr<juce::AudioProcessor> raw (createPluginFilter());
        auto* p = dynamic_cast<GranulatorInstrumentProcessor*> (raw.get());
        jassert (p != nullptr);
        raw.release();
        p->getLoader().setRecordingsDirectory (harnessRecordingsDir());
        p->getLoader().setImportsDirectory (harnessImportsDir());   // PLAN-3.2 PD24
        return std::unique_ptr<GranulatorInstrumentProcessor> (p);
    }

    /** PLAN-3.2 PD24: the test-directory guard of EVERY harness mode.
        start() runs before any other processor work: a processor from make()
        must resolve its Recordings AND Imports folders to the harness temp
        tree, never under the real ~/Music/Ouaricon; it also snapshots the
        plugin's real folder. end() runs after the mode: the real folder must
        be as it was. Either failure fails the run (main()). */
    namespace pd24
    {
        juce::File realOuaricon() { return realMusic().getChildFile ("Ouaricon"); }   // PLAN-4 PD15: the REAL home
        juce::File realPluginDir() { return realOuaricon().getChildFile ("O-GranulatorInstrument"); }

        bool underReal (const juce::File& f)
        {
            const auto real = realOuaricon();
            return f == juce::File() || f == real || f.isAChildOf (real);
        }

        struct RealSnap
        {
            bool exists = false;
            int files = 0;
            juce::int64 newest = 0;
            bool operator== (const RealSnap& o) const { return exists == o.exists && files == o.files && newest == o.newest; }
        };

        RealSnap snapDir (const juce::File& dir)
        {
            RealSnap d;
            d.exists = dir.isDirectory();
            if (d.exists)
                for (const auto& e : juce::RangedDirectoryIterator (dir, true, "*", juce::File::findFilesAndDirectories))
                {
                    ++d.files;
                    d.newest = std::max (d.newest, e.getModificationTime().toMilliseconds());
                }
            return d;
        }
        RealSnap snapReal() { return snapDir (realPluginDir()); }
        RealSnap snapLib()  { return snapDir (realLibraryDir()); }   // PLAN-4 PD15: ~/Library/O-GranulatorInstrument (presets)

        RealSnap gStart, gLibStart;
        bool gRedirected = false;

        bool start()
        {
            juce::File recDir, impDir;
            {
                auto p = make();
                recDir = p->getLoader().getRecordingsDirectory();
                impDir = p->getLoader().getImportsDirectory();
            }
            gRedirected = ! underReal (recDir) && ! underReal (impDir) && recDir == harnessRecordingsDir() && impDir == harnessImportsDir()
                       && recDir != impDir;
            gStart = snapReal();
            gLibStart = snapLib();
            if (! gRedirected)
                std::printf ("PD24 GUARD FAILED: the Recordings / Imports folder of a harness processor is not redirected "
                             "(Recordings %s, Imports %s the real ~/Music/Ouaricon). Nothing was run.\n",
                             underReal (recDir) ? "UNDER" : "outside", underReal (impDir) ? "UNDER" : "outside");
            return gRedirected;
        }

        bool end()
        {
            const auto e = snapReal();
            const auto l = snapLib();
            if (! (l == gLibStart))
            {
                std::printf ("PD24 GUARD FAILED: the real ~/Library/O-GranulatorInstrument changed during the run "
                             "(start %s / %d entries, end %s / %d entries)\n",
                             gLibStart.exists ? "present" : "absent", gLibStart.files, l.exists ? "present" : "absent", l.files);
                return false;
            }
            if (e == gStart)
                return true;
            std::printf ("PD24 GUARD FAILED: the real ~/Music/Ouaricon/O-GranulatorInstrument changed during the run "
                         "(start %s / %d entries, end %s / %d entries)\n",
                         gStart.exists ? "present" : "absent", gStart.files, e.exists ? "present" : "absent", e.files);
            return false;
        }
    }

    bool dirGuardRedirected() { return pd24::gRedirected; }

    // ── Goldens ─────────────────────────────────────────────────────────────
    struct GoldenParam
    {
        int idx = 0;
        juce::String id, name, type;
        double lo = 0, hi = 0, interval = 0, def = 0, skew = 1, centre = 0;
        juce::StringArray choices;
    };

    std::vector<juce::StringArray> readTsv (const juce::String& file)
    {
        std::vector<juce::StringArray> rows;
        const auto f = gGoldenDir.getChildFile (file);
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

    // params.tsv: idx id name type start end interval default skew centre choices
    std::vector<GoldenParam> goldenParams()
    {
        std::vector<GoldenParam> g;
        for (auto& c : readTsv ("params.tsv"))
        {
            GoldenParam p;
            p.idx = c[0].getIntValue();
            p.id = c[1]; p.name = c[2]; p.type = c[3];
            p.lo = c[4].getDoubleValue(); p.hi = c[5].getDoubleValue(); p.interval = c[6].getDoubleValue();
            p.def = c[7].getDoubleValue(); p.skew = c[8].getDoubleValue(); p.centre = c[9].getDoubleValue();
            if (c[10].isNotEmpty()) p.choices.addTokens (c[10], "|", "");
            g.push_back (p);
        }
        return g;
    }

    // ── Synthetic playhead (Stage 2 tempo gates inherit it; parent verbatim) ─
    // Legacy mode (script empty): ppq = t/fs*bpm/60, a bpm change is a ppq JUMP.
    // v2 mode: a piecewise script of segments in HOST-SAMPLE time: tempo map,
    // time signature with ppqPositionOfLastBarStart, start / stop, loop wrap,
    // locate, negative start ppq.
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

    // ── MIDI script: ABSOLUTE sample times (never block-relative) ───────────
    struct MidiEv { juce::int64 t; juce::MidiMessage m; };

    struct MidiScript
    {
        std::vector<MidiEv> events;
        size_t cursor = 0;
        int late = 0;   // events found behind the block start: a scheduling bug

        MidiScript& add (juce::int64 t, const juce::MidiMessage& msg) { events.push_back ({ t, msg }); return *this; }
        MidiScript& chord (juce::int64 t, int ch, std::initializer_list<int> notes, float vel)
        {
            for (int note : notes) add (t, juce::MidiMessage::noteOn (ch, note, vel));
            return *this;
        }
        MidiScript& noteOff (juce::int64 t, int ch, int note)            { return add (t, juce::MidiMessage::noteOff (ch, note)); }
        MidiScript& bend (juce::int64 t, int ch, int value14)            { return add (t, juce::MidiMessage::pitchWheel (ch, value14)); }
        MidiScript& cc (juce::int64 t, int ch, int number, int value)    { return add (t, juce::MidiMessage::controllerEvent (ch, number, value)); }
        MidiScript& channelPressure (juce::int64 t, int ch, int value)   { return add (t, juce::MidiMessage::channelPressureChange (ch, value)); }
        MidiScript& polyAT (juce::int64 t, int ch, int note, int value)  { return add (t, juce::MidiMessage::aftertouchChange (ch, note, value)); }
        MidiScript& mpeMember (juce::int64 t, int ch, int note, float vel)
        {
            jassert (ch >= 2 && ch <= 16);   // MPE lower zone member channels
            return add (t, juce::MidiMessage::noteOn (ch, note, vel));
        }
        MidiScript& allNotesOff (juce::int64 t, int ch)                  { return add (t, juce::MidiMessage::allNotesOff (ch)); }

        void finalise()
        {
            std::stable_sort (events.begin(), events.end(), [] (const MidiEv& a, const MidiEv& b) { return a.t < b.t; });
            rewind();
        }
        void rewind() { cursor = 0; late = 0; }

        /** The events with blockStart <= t < blockStart + n, at offset t - blockStart. */
        void fill (juce::MidiBuffer& mb, juce::int64 blockStart, int n)
        {
            mb.clear();
            while (cursor < events.size() && events[cursor].t < blockStart) { ++late; ++cursor; }
            while (cursor < events.size() && events[cursor].t < blockStart + n)
            {
                mb.addEvent (events[cursor].m, static_cast<int> (events[cursor].t - blockStart));
                ++cursor;
            }
        }
    };

    // ── Parameter helpers (the HOST side: notifying-host writes are harness only)
    juce::RangedAudioParameter* param (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                if (rp->getParameterID() == id) return rp;
        return nullptr;
    }

    /** A normalised value n with T (F (n)) == n through the parameter's own
        conversions, so the value a host writes is exactly what a reopen yields. */
    float settledNorm (const juce::RangedAudioParameter& rp, float n)
    {
        for (int k = 0; k < 16; ++k)
        {
            const float nn = rp.convertTo0to1 (rp.convertFrom0to1 (n));
            if (nn == n) break;
            n = nn;
        }
        return n;
    }

    /** Engineering value written through the host-normalised path. */
    void setParam (juce::AudioProcessor& p, const juce::String& id, float value)
    {
        if (auto* rp = param (p, id))
            rp->setValueNotifyingHost (settledNorm (*rp, rp->convertTo0to1 (value)));
    }

    // Every parameter written with its default NORMALISED value, the way a host
    // or pluginval writes it.
    void setAllViaHostNormalised (juce::AudioProcessor& p)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                rp->setValueNotifyingHost (rp->getDefaultValue());
    }

    // Every parameter set to a seeded random normalised value through the host path.
    void setAllRandomViaHost (juce::AudioProcessor& p, juce::Random& r)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                rp->setValueNotifyingHost (r.nextFloat());
    }

    std::vector<float> allValues (juce::AudioProcessor& p)
    {
        std::vector<float> v;
        for (auto* ap : p.getParameters()) v.push_back (ap->getValue());
        return v;
    }

    bool bitwiseEqual (const std::vector<float>& a, const std::vector<float>& b)
    {
        return a.size() == b.size() && (a.empty() || std::memcmp (a.data(), b.data(), sizeof (float) * a.size()) == 0);
    }

    juce::AudioProcessor::BusesLayout layout (const juce::AudioChannelSet& in, const juce::AudioChannelSet& out)
    {
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (in);
        l.outputBuses.add (out);
        return l;
    }

    // ── State helpers ───────────────────────────────────────────────────────
    juce::MemoryBlock stateOf (juce::AudioProcessor& p) { juce::MemoryBlock m; p.getStateInformation (m); return m; }
    void load (juce::AudioProcessor& p, const juce::MemoryBlock& m) { p.setStateInformation (m.getData(), (int) m.getSize()); }
    std::unique_ptr<juce::XmlElement> xmlOf (const juce::MemoryBlock& m) { return juce::AudioProcessor::getXmlFromBinary (m.getData(), (int) m.getSize()); }
    juce::MemoryBlock blobOf (const juce::XmlElement& x) { juce::MemoryBlock m; juce::AudioProcessor::copyXmlToBinary (x, m); return m; }

    int countChildren (const juce::XmlElement& x, const juce::String& tag)
    {
        int n = 0;
        for (auto* c : x.getChildIterator()) n += c->hasTagName (tag) ? 1 : 0;
        return n;
    }

    // ── R1 comparator: every sample of every channel is +0.0f bitwise and finite
    bool bitwiseZeroAndFinite (const juce::AudioBuffer<float>& b, int n)
    {
        static const std::vector<float> zeros (8192, 0.0f);
        if (n > (int) zeros.size()) return false;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            const float* x = b.getReadPointer (ch);
            if (std::memcmp (x, zeros.data(), sizeof (float) * (size_t) n) != 0) return false;
            for (int i = 0; i < n; ++i)
                if (! std::isfinite (x[i])) return false;
        }
        return true;
    }
}

//==============================================================================
static void paramRows()
{
    std::printf ("\n-- G. parameters vs spec golden (%s)\n", gGoldenDir.getFullPathName().toRawUTF8());
    const auto golden = goldenParams();
    auto p = make();
    const auto& params = p->getParameters();

    std::vector<juce::RangedAudioParameter*> ranged;
    for (auto* ap : params) ranged.push_back (dynamic_cast<juce::RangedAudioParameter*> (ap));

    // G1
    const bool seq = golden.size() == 293 && params.size() == 293;
    Tally t1;
    for (size_t i = 0; seq && i < golden.size(); ++i)
        t1.check (ranged[i] != nullptr && ranged[i]->getParameterID() == golden[i].id, golden[i].id);
    row (seq && t1.bad == 0, "G1 293 params, ID order == params.tsv",
         t1.detail (293) + "  (golden " + juce::String ((int) golden.size()) + ", plugin " + juce::String (params.size()) + ")");
    if (! seq) return;

    const auto& ids = Ids::allIdsInOrder();
    Tally t1b;
    for (int i = 0; i < 293; ++i) t1b.check (ids[i] == golden[(size_t) i].id, golden[(size_t) i].id);
    row (ids.size() == 293 && t1b.bad == 0, "G1 allIdsInOrder() == params.tsv", t1b.detail (293));

    // G2 / G3
    int nf = 0, nc = 0, nb = 0;
    Tally t2, t3, tn;
    for (size_t i = 0; i < 293; ++i)
    {
        auto* rp = ranged[i];
        const auto& g = golden[i];
        const bool isF = dynamic_cast<juce::AudioParameterFloat*> (rp) != nullptr;
        const bool isC = dynamic_cast<juce::AudioParameterChoice*> (rp) != nullptr;
        const bool isB = dynamic_cast<juce::AudioParameterBool*> (rp) != nullptr;
        nf += isF ? 1 : 0; nc += isC ? 1 : 0; nb += isB ? 1 : 0;
        t2.check ((g.type == "Float" && isF) || (g.type == "Choice" && isC) || (g.type == "Bool" && isB), g.id + " type");
        t3.check (rp->getVersionHint() == 1, g.id);
        tn.check (rp->getName (256) == g.name, g.id + " '" + rp->getName (256) + "' != '" + g.name + "'");
    }
    row (t2.bad == 0, "G2 per-ID type", t2.detail (293));
    row (nf == 200 && nc == 74 && nb == 19, "G2 totals 200 Float / 74 Choice / 19 Bool",
         juce::String (nf) + " / " + juce::String (nc) + " / " + juce::String (nb));
    row (t3.bad == 0, "G3 version hint 1", t3.detail (293));
    row (tn.bad == 0, "G3 host names byte-equal to golden", tn.detail (293));

    // G4 / G5 / G6
    Tally t4r, t4c, t4k, t4i, t4n, t4d, t4e, t5, t6;
    int floats = 0, choices = 0, bools = 0, nCentre = 0, nRaw = 0, nStep = 0, nNeg = 0, nOn = 0;
    double worstEng = 0.0;
    for (size_t i = 0; i < 293; ++i)
    {
        const auto& g = golden[i];
        if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (ranged[i]))
        {
            ++floats;
            const auto& r = f->getNormalisableRange();
            const bool isCentre = g.centre > 0.0;
            const bool isRaw = ! isCentre && g.skew != 1.0;
            const bool isStep = g.interval == 1.0;
            const bool isNeg = g.lo < 0.0;

            t4r.check (r.start == (float) g.lo && r.end == (float) g.hi && r.interval == (float) g.interval
                       && (isCentre || r.skew == (float) g.skew) && ! r.symmetricSkew,
                       g.id + " range (skew " + juce::String (r.skew, 5) + ", interval " + juce::String (r.interval) + ")");
            if (isCentre)
            {
                ++nCentre;
                juce::NormalisableRange<float> ref ((float) g.lo, (float) g.hi, (float) g.interval, 1.0f);
                ref.setSkewForCentre ((float) g.centre);
                t4c.check (r.skew == ref.skew && std::abs ((double) r.skew - g.skew) <= 1e-4,
                           g.id + " skew " + juce::String (r.skew, 6) + " ref " + juce::String (ref.skew, 6));
            }
            if (isRaw)
            {
                ++nRaw;
                t4k.check (r.skew == (float) g.skew, g.id + " skew " + juce::String (r.skew, 6));
            }
            if (isStep)
            {
                ++nStep;
                t4i.check (r.interval == 1.0f && f->get() == (float) g.def, g.id + " get " + juce::String (f->get(), 6));
            }
            if (isNeg)
            {
                ++nNeg;
                t4n.check (r.interval == 0.0f, g.id + " interval " + juce::String (r.interval));
            }

            const float normSpec = r.convertTo0to1 ((float) g.def);
            t4d.check (ranged[i]->getDefaultValue() == normSpec, g.id + " default norm");
            const double eng = f->convertFrom0to1 (ranged[i]->getDefaultValue());
            const double span = juce::jmax (1.0, std::abs (g.hi - g.lo));
            worstEng = juce::jmax (worstEng, std::abs (eng - g.def) / span);
            t4e.check (std::abs (eng - g.def) <= 1e-6 * span && std::abs ((double) f->get() - g.def) <= 1e-6 * span,
                       g.id + " default eng " + juce::String (eng, 7));
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
            const bool on = g.def != 0.0;
            nOn += b->get() ? 1 : 0;
            // PLAN-R1 PD13: s1_enable and the four sN_fb_enable default On.
            t6.check (b->get() == on && (ranged[i]->getDefaultValue() > 0.5f) == on && on == (g.id == "s1_enable" || g.id.endsWith ("_fb_enable")), g.id);
        }
    }
    row (t4r.bad == 0, "G4 float start / end / interval / skew == golden", t4r.detail (floats));
    row (t4c.bad == 0 && nCentre == 25, "G4 setSkewForCentre rows (25): == reference, +-1e-4 of spec",
         t4c.detail (nCentre) + "  rows " + juce::String (nCentre));
    row (t4k.bad == 0 && nRaw == 14, "G4 raw-skew rows (14): skew exact", t4k.detail (nRaw) + "  rows " + juce::String (nRaw));
    row (t4i.bad == 0 && nStep == 6, "G4 interval-1 rows (6): interval 1, get() == default exactly",
         t4i.detail (nStep) + "  rows " + juce::String (nStep));
    row (t4n.bad == 0 && nNeg == 43, "G4 negative-start rows (43): interval 0", t4n.detail (nNeg) + "  rows " + juce::String (nNeg));
    row (t4d.bad == 0, "G4 float default normalised == convertTo0to1 (spec) exactly", t4d.detail (floats));
    row (t4e.bad == 0, "G4 float default engineering (tol 1e-6 of span)", t4e.detail (floats) + "  worst " + juce::String (worstEng, 9));
    row (t5.bad == 0 && choices == 74, "G5 74 choice lists: strings / order / default index", t5.detail (choices));
    row (t6.bad == 0 && bools == 19 && nOn == 5, "G6 bools: s1_enable + the four sN_fb_enable On, the other 14 Off", t6.detail (bools));

    // ── D: modulation destination table ─────────────────────────────────────
    std::printf ("\n-- D. modulation destination table\n");
    const auto dg = readTsv ("mod-dests.tsv");
    Tally d1, d2;
    const bool sized = dg.size() == 150 && Ids::kModDests == 150;
    for (int k = 0; sized && k < 150; ++k)
        d1.check (juce::String (Ids::kModDestIds[k]) == dg[(size_t) k][1] && dg[(size_t) k][0].getIntValue() == k,
                  juce::String (k) + " " + Ids::kModDestIds[k] + " vs " + dg[(size_t) k][1]);
    row (sized && d1.bad == 0 && juce::String (Ids::kModDestIds[0]) == "None", "D1 kModDestIds == mod-dests.tsv (150, order)",
         d1.detail (150) + "  (golden " + juce::String ((int) dg.size()) + ")");
    for (int k = 1; k < Ids::kModDests; ++k)
        d2.check (dynamic_cast<juce::AudioParameterFloat*> (param (*p, Ids::kModDestIds[k])) != nullptr, Ids::kModDestIds[k]);
    row (d2.bad == 0, "D2 every destination != None is a registered Float", d2.detail (149));

    Tally d3;
    juce::StringArray expect;
    for (auto& r : dg) expect.add (r[2]);
    for (int m = 1; m <= Ids::kModSlots; ++m)
    {
        auto* c = dynamic_cast<juce::AudioParameterChoice*> (param (*p, Ids::mod (m, "dest")));
        d3.check (c != nullptr && c->choices == expect, "mod" + juce::String (m) + "_dest");
    }
    row (d3.bad == 0 && expect.size() == 150 && expect[0] == "None" && Ids::modDestChoices() == expect,
         "D3 16 modM_dest lists == None + golden host names", d3.detail (16));
    row (Ids::kModDestTableVersion == 2, "D4 kModDestTableVersion == 2");

    // D5: the literal == the formula b = 2 + 36 (N - 1), from the 36 per-stream
    // suffixes (PLAN-R1 PD14: the 28 voice fields, then the stream's feedback and reverb).
    {
        static const char* const kStream36[] = {
            "level", "pan", "position", "spray", "size", "size_rand", "density", "jitter", "pitch", "pitch_rand",
            "reverse_prob", "window", "window_skew", "stereo_spread", "gain_rand", "filter_cutoff", "filter_rand",
            "filter_res", "drive", "crush", "crush_prob", "ring_freq", "ring_rand", "ring_mix",
            "scan_rate", "loop_start", "loop_end", "key_track",
            "feedback", "fb_low_cut", "fb_high_cut", "fb_saturation", "fb_diffusion",
            "reverb_mix", "reverb_size", "reverb_damp" };
        juce::StringArray formula { "None", "output_gain" };
        bool bases = true;
        for (int n = 1; n <= 4; ++n)
        {
            bases = bases && formula.size() == 2 + 36 * (n - 1);
            for (const char* s : kStream36) formula.add (Ids::stream (n, s));
        }
        for (int n = 1; n <= 4; ++n) formula.add (Ids::lfo (n, "rate"));
        Tally d5;
        for (int k = 0; k < juce::jmin (150, formula.size()); ++k)
            d5.check (formula[k] == Ids::kModDestIds[k], juce::String (k) + " " + formula[k] + " vs " + Ids::kModDestIds[k]);
        row (bases && formula.size() == 150 && d5.bad == 0, "D5 kModDestIds == formula b = 2 + 36 (N - 1)", d5.detail (150));
    }

    // ── SRC1: modulation sources ────────────────────────────────────────────
    std::printf ("\n-- SRC. modulation sources\n");
    {
        const auto sg = readTsv ("mod-sources.tsv");
        juce::StringArray names;
        for (auto& r : sg) names.add (r[1]);
        Tally s1, s2;
        const bool srcSized = sg.size() == 18;
        for (int k = 0; srcSized && k < 18; ++k)
            s1.check (sg[(size_t) k][0].getIntValue() == k
                      && juce::String (Ids::kModSources[k].name) == sg[(size_t) k][1]
                      && juce::String (Ids::sourceClassToken (Ids::kModSources[k].cls)) == sg[(size_t) k][2],
                      juce::String (k) + " " + Ids::kModSources[k].name);
        for (int m = 1; m <= Ids::kModSlots; ++m)
        {
            auto* c = dynamic_cast<juce::AudioParameterChoice*> (param (*p, Ids::mod (m, "source")));
            s2.check (c != nullptr && c->choices == names, "mod" + juce::String (m) + "_source");
        }
        row (srcSized && s1.bad == 0 && s2.bad == 0 && Ids::modSources() == names,
             "SRC1 16 modM_source lists + kModSources (class) == mod-sources.tsv (18)",
             s1.detail (18) + "; slots " + s2.detail (16) + "  (golden " + juce::String ((int) sg.size()) + ")");
    }
}

//==============================================================================
static void busRows()
{
    std::printf ("\n-- B. buses, MIDI, latency\n");
    using CS = juce::AudioChannelSet;
    auto p = make();

    const auto* inBus = p->getBus (true, 0);
    const auto* outBus = p->getBus (false, 0);
    const bool defaults = p->getBusCount (true) == 1 && p->getBusCount (false) == 1
                       && inBus != nullptr && outBus != nullptr
                       && inBus->getName() == "Sidechain" && inBus->getCurrentLayout() == CS::stereo()
                       && inBus->isEnabled() && inBus->isEnabledByDefault()
                       && outBus->getName() == "Output" && outBus->getCurrentLayout() == CS::stereo() && outBus->isEnabled();
    row (defaults, "B1 default buses: input 'Sidechain' stereo enabled; output 'Output' stereo");

    const bool acc = p->checkBusesLayoutSupported (layout (CS::disabled(), CS::stereo()))
                  && p->checkBusesLayoutSupported (layout (CS::mono(), CS::stereo()))
                  && p->checkBusesLayoutSupported (layout (CS::stereo(), CS::stereo()));
    row (acc, "B1 accepts input {disabled, mono, stereo} x output stereo");

    const bool rej = ! p->checkBusesLayoutSupported (layout (CS::stereo(), CS::mono()))
                  && ! p->checkBusesLayoutSupported (layout (CS::stereo(), CS::disabled()))
                  && ! p->checkBusesLayoutSupported (layout (CS::stereo(), CS::create5point1()))
                  && ! p->checkBusesLayoutSupported (layout (CS::create5point1(), CS::stereo()))
                  && ! p->checkBusesLayoutSupported (layout (CS::createLCR(), CS::stereo()));
    row (rej, "B1 rejects output mono / disabled / 5.1, input 5.1 / LCR");

    {
        struct L { const char* name; CS in; int expect; };
        const L cases[] = { { "stereo", CS::stereo(), 2 }, { "mono", CS::mono(), 1 }, { "disabled", CS::disabled(), 0 } };
        Tally tc;
        for (const auto& c : cases)
        {
            auto q = make();
            const bool ok = q->setBusesLayout (layout (c.in, CS::stereo()));
            q->prepareToPlay (48000.0, 512);
            tc.check (ok && q->getSidechainChannelCount() == c.expect,
                      juce::String (c.name) + " -> " + juce::String (q->getSidechainChannelCount()));
        }
        row (tc.bad == 0, "B1 getSidechainChannelCount() after layout + prepare = 2 / 1 / 0", tc.detail (3));
    }

    row (p->acceptsMidi() && ! p->producesMidi() && ! p->isMidiEffect(),
         "B1 acceptsMidi true, producesMidi false, isMidiEffect false");

    // B3
    {
        Tally tl;
        for (double sr : { 44100.0, 96000.0 })
        {
            auto q = make();
            q->prepareToPlay (sr, 512);
            tl.check (q->getLatencySamples() == 0, juce::String (sr) + " latency " + juce::String (q->getLatencySamples()));
            tl.check (q->getTailLengthSeconds() == 0.0, juce::String (sr) + " tail");
        }
        row (tl.bad == 0, "B3 latency 0 after prepare (44.1 k, 96 k); tail 0", tl.detail (4));
    }
}

//==============================================================================
static void midiRows()
{
    std::printf ("\n-- M. MIDI script self-test\n");
    const juce::int64 times[] = { 0, 511, 512, 513, 4095, 4096 };
    constexpr int kEvents = 6;
    const int blocks[] = { 1, 441, 512, 0 /* seeded variable */ };

    for (int b : blocks)
    {
        MidiScript s;
        for (int k = kEvents; --k >= 0;)   // added out of order: finalise() sorts
            s.add (times[k], juce::MidiMessage::noteOn (1, 60 + k, 0.5f));
        s.finalise();

        std::vector<int> count ((size_t) kEvents, 0);
        bool ok = true;
        juce::MidiBuffer mb;
        mb.ensureSize (1024);
        juce::Random vr ((juce::int64) 0x4D310001);
        const juce::int64 total = 4096 + 700;
        juce::int64 pos = 0;
        while (pos < total)
        {
            const int n = (int) juce::jmin ((juce::int64) (b == 0 ? 1 + vr.nextInt (512) : b), total - pos);
            s.fill (mb, pos, n);
            for (const auto meta : mb)
            {
                const int k = meta.getMessage().getNoteNumber() - 60;
                if (k < 0 || k >= kEvents) { ok = false; continue; }
                ++count[(size_t) k];
                ok = ok && meta.samplePosition >= 0 && meta.samplePosition < n && pos + meta.samplePosition == times[k];
            }
            pos += n;
        }
        bool once = true;
        for (int c : count) once = once && c == 1;
        row (ok && once && s.late == 0,
             "M1 events at 0/511/512/513/4095/4096 once, right block + offset, block "
                 + (b == 0 ? juce::String ("var") : juce::String (b)),
             "late " + juce::String (s.late));
    }
}

//==============================================================================
static MidiScript makeR1Script()
{
    MidiScript s;
    s.chord (0, 1, { 48, 52, 55, 60 }, 0.8f);
    int note = 64;
    for (juce::int64 edge : { (juce::int64) 32, (juce::int64) 441, (juce::int64) 512, (juce::int64) 4096 })
        for (juce::int64 d = -1; d <= 1; ++d)
            s.add (edge + d, juce::MidiMessage::noteOn (1, note++, 0.6f));
    s.bend (1000, 1, 12000).bend (2000, 1, 8192);
    s.cc (3000, 1, 74, 90).cc (5000, 1, 64, 127).cc (6000, 1, 1, 64);
    s.channelPressure (7000, 1, 100).polyAT (7500, 1, 60, 80);
    // MPE lower zone member notes (mpe_enable On) with per-note expression.
    s.mpeMember (8000, 2, 67, 0.7f).mpeMember (8001, 3, 71, 0.7f).mpeMember (8002, 16, 74, 0.7f);
    s.bend (9000, 2, 10000).channelPressure (9500, 3, 90).cc (10000, 16, 74, 100);
    s.cc (20000, 1, 64, 0);
    for (int k : { 48, 52, 55, 60 }) s.noteOff (30000, 1, k);
    for (int k = 64; k < note; ++k) s.noteOff (30001, 1, k);
    s.noteOff (31000, 2, 67).noteOff (31000, 3, 71).noteOff (31000, 16, 74);
    s.allNotesOff (40000, 1);
    s.finalise();
    return s;
}

static void renderRows()
{
    std::printf ("\n-- R. sidechain-only render -> output bitwise zero\n");
    using CS = juce::AudioChannelSet;
    constexpr double kSr = 48000.0;
    constexpr int kLen = 48000;   // 1 s per case
    constexpr int kMaxBlock = 4096;

    // Stimulus: seeded noise at 0.7 + a full-scale 997 Hz sine on every channel.
    juce::AudioBuffer<float> stim (2, kLen);
    for (int ch = 0; ch < 2; ++ch)
    {
        juce::Random r ((juce::int64) 0x5C000001 + ch);
        for (int i = 0; i < kLen; ++i)
        {
            const double sine = std::sin (2.0 * juce::MathConstants<double>::pi * 997.0 * (double) i / kSr);
            stim.setSample (ch, i, (r.nextFloat() * 2.0f - 1.0f) * 0.7f + (float) sine);
        }
    }

    auto script = makeR1Script();

    // R1n part 1: the comparator itself (positive + negative controls).
    bool comparatorOk = true;
    {
        juce::AudioBuffer<float> z (2, 64);
        z.clear();
        for (int i = 0; i < 64; ++i) { z.setSample (0, i, 0.0f); z.setSample (1, i, 0.0f); }
        comparatorOk = comparatorOk && bitwiseZeroAndFinite (z, 64);            // +0.0f passes
        z.setSample (1, 63, -0.0f);
        comparatorOk = comparatorOk && ! bitwiseZeroAndFinite (z, 64);          // -0.0f fails
    }

    struct L { const char* name; CS in; int scCh; };
    const L layouts[] = { { "stereo SC -> stereo", CS::stereo(), 2 },
                          { "mono SC -> stereo", CS::mono(), 1 },
                          { "SC disabled -> stereo (control)", CS::disabled(), 0 } };
    const int blocks[] = { 1, 32, 441, 512, 4096, 0 /* seeded variable */ };

    int caseIndex = 0;
    bool unprocessedAlwaysFails = true;
    for (const auto& lay : layouts)
    {
        Tally tv;
        for (int playheadMode : { 0, 1 })
            for (int paramMode : { 0, 1 })
            {
                Tally tb;
                for (int b : blocks)
                {
                    ++caseIndex;
                    MockPlayHead ph;
                    ph.sampleRate = kSr;
                    auto q = make();
                    if (playheadMode == 1) q->setPlayHead (&ph);
                    const bool okLayout = q->setBusesLayout (layout (lay.in, CS::stereo()));
                    if (paramMode == 0)
                        setAllViaHostNormalised (*q);
                    else
                    {
                        juce::Random pr ((juce::int64) 0xA1100000 + caseIndex);
                        setAllRandomViaHost (*q, pr);
                    }
                    setParam (*q, "mpe_enable", 1.0f);   // the script carries MPE member notes
                    // PLAN-2.6 PD17 (P9): rec_arm pinned Off — capture-on leak coverage is LK1.
                    setParam (*q, "rec_arm", 0.0f);
                    q->prepareToPlay (kSr, b == 0 ? 512 : b);
                    const bool okCount = q->getSidechainChannelCount() == lay.scCh;

                    juce::AudioBuffer<float> blk (2, kMaxBlock);
                    juce::MidiBuffer mb;
                    mb.ensureSize (4096);
                    script.rewind();
                    juce::Random vr ((juce::int64) 0xB10C0000 + caseIndex);
                    double sumSq[2] = { 0.0, 0.0 };
                    bool zeroOk = true, stimOk = true;
                    int pos = 0;
                    while (pos < kLen)
                    {
                        const int n = juce::jmin (b == 0 ? 1 + vr.nextInt (512) : b, kLen - pos);
                        blk.setSize (2, n, false, false, true);
                        for (int ch = 0; ch < 2; ++ch)
                        {
                            if (lay.scCh == 1 && ch == 1)
                                for (int i = 0; i < n; ++i) blk.setSample (ch, i, 12345.0f);   // host garbage past a mono SC
                            else
                                blk.copyFrom (ch, 0, stim, ch, pos, n);
                        }

                        // R1v: the stimulus is really in the buffer handed to processBlock.
                        stimOk = stimOk && ! blk.hasBeenCleared();
                        for (int ch = 0; ch < 2; ++ch)
                        {
                            const float* x = blk.getReadPointer (ch);
                            for (int i = 0; i < n; ++i) sumSq[ch] += (double) x[i] * (double) x[i];
                        }
                        // R1n part 2: the comparator FAILS on the unprocessed copy.
                        if (pos == 0)
                            unprocessedAlwaysFails = unprocessedAlwaysFails && ! bitwiseZeroAndFinite (blk, n);

                        script.fill (mb, pos, n);
                        q->processBlock (blk, mb);
                        ph.advance (n);
                        zeroOk = zeroOk && bitwiseZeroAndFinite (blk, n);
                        pos += n;
                    }
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        const double rmsDb = 10.0 * std::log10 (juce::jmax (1e-30, sumSq[ch] / kLen));
                        tv.check (stimOk && rmsDb > -20.0, juce::String ("ch ") + juce::String (ch) + " " + juce::String (rmsDb, 1) + " dBFS");
                    }
                    tb.check (okLayout && okCount && zeroOk && script.late == 0,
                              "block " + (b == 0 ? juce::String ("var") : juce::String (b))
                                  + (okLayout ? "" : " layout refused") + (okCount ? "" : " SC count")
                                  + (zeroOk ? "" : " non-zero output"));
                    q->setPlayHead (nullptr);
                }
                row (tb.bad == 0,
                     juce::String ("R1 ") + lay.name + (playheadMode ? " +playhead" : " no playhead")
                         + (paramMode ? " random params" : " defaults"),
                     tb.detail ((int) std::size (blocks)));
            }
        row (tv.bad == 0, juce::String ("R1v input RMS > -20 dBFS on every channel, not flagged clear: ") + lay.name,
             tv.detail (2 * 2 * 2 * (int) std::size (blocks)));
    }
    row (comparatorOk && unprocessedAlwaysFails, "R1n comparator: +0 passes, -0.0f fails, unprocessed copy fails");
}

//==============================================================================
static void stateRows()
{
    std::printf ("\n-- S. state codec\n");

    // S1a: seed > INT32_MAX (catches an int32 truncation), one non-default per class.
    {
        constexpr uint64_t kSeed = 0x0000000080000001ull;
        auto a = make();
        a->setSeed (kSeed);
        a->uiLanguage = 2;
        setParam (*a, "polyphony", 12.0f);
        setParam (*a, "amp_release", 3000.0f);
        setParam (*a, "glide", 250.0f);
        setParam (*a, "mod16_dest", 149.0f);
        setParam (*a, "s4_enable", 1.0f);
        setParam (*a, "mod1_depth", -37.0f);
        auto* poly = dynamic_cast<juce::AudioParameterFloat*> (param (*a, "polyphony"));
        auto* dest = dynamic_cast<juce::AudioParameterChoice*> (param (*a, "mod16_dest"));
        auto* s4 = dynamic_cast<juce::AudioParameterBool*> (param (*a, "s4_enable"));
        const bool applied = poly != nullptr && poly->get() == 12.0f && dest != nullptr && dest->getIndex() == 149
                          && s4 != nullptr && s4->get();
        const auto before = allValues (*a);
        auto b = make();
        load (*b, stateOf (*a));
        const auto after = allValues (*b);
        int diff = 0;
        for (size_t i = 0; i < juce::jmin (before.size(), after.size()); ++i)
            diff += std::memcmp (&before[i], &after[i], sizeof (float)) != 0 ? 1 : 0;
        row (applied && b->getSeed() == kSeed && bitwiseEqual (before, after) && before.size() == 293 && b->uiLanguage.load() == 2,
             "S1a round trip: seed 0x80000001, 293 values bitwise, uiLanguage 2",
             juce::String (diff) + " value(s) differ; seed " + juce::String::toHexString ((juce::int64) b->getSeed()));
    }

    // S1b: seed > 2^63 -> the XML attribute is the negative int64 string.
    {
        constexpr uint64_t kSeed = 0xF00DFACECAFEBEEFull;
        auto a = make();
        a->setSeed (kSeed);
        const auto blob = stateOf (*a);
        const auto x = xmlOf (blob);
        auto* eng = x != nullptr ? x->getChildByName ("Engine") : nullptr;
        const auto attr = eng != nullptr ? eng->getStringAttribute ("seed") : juce::String();
        auto b = make();
        load (*b, blob);
        row (attr == juce::String ((juce::int64) kSeed) && attr.startsWith ("-") && b->getSeed() == kSeed,
             "S1b seed 0xF00DFACECAFEBEEF: negative int64 string, reads back equal", attr);
    }

    // S2: XML shape.
    {
        auto a = make();
        a->uiLanguage = 2;
        const auto x = xmlOf (stateOf (*a));
        bool ok = x != nullptr && x->hasTagName ("OGranulatorInstrument");
        juce::String detail;
        if (ok)
        {
            auto* eng = x->getChildByName ("Engine");
            auto* slots = x->getChildByName ("Slots");
            ok = countChildren (*x, "Engine") == 1 && countChildren (*x, "Slots") == 1
              && eng != nullptr && slots != nullptr
              && eng->getStringAttribute ("stateVersion") == "1" && eng->getStringAttribute ("modDestTableVersion") == "2"
              && slots->getNumChildElements() == 0
              && ! eng->hasAttribute ("id") && ! slots->hasAttribute ("id")
              && x->getStringAttribute ("uiLanguage") == "zh-Hans"
              && countChildren (*x, "PARAM") == 293;
            detail = "Engine " + juce::String (countChildren (*x, "Engine")) + ", Slots " + juce::String (countChildren (*x, "Slots"))
                   + ", PARAM " + juce::String (countChildren (*x, "PARAM")) + ", uiLanguage " + x->getStringAttribute ("uiLanguage");
        }
        row (ok, "S2 XML: one <Engine v1/v2>, one empty <Slots>, no id, uiLanguage zh-Hans, 293 PARAM", detail);
    }

    // S3: A -> B -> change -> C (two reopens into the same instance).
    {
        constexpr uint64_t kSeedA = 0x1111222233334444ull, kSeedB = 0x9999AAAABBBBCCCCull;
        auto a = make();
        a->setSeed (kSeedA);
        setParam (*a, "s3_pitch", 7.5f);
        auto b = make();
        load (*b, stateOf (*a));
        setParam (*b, "s3_pitch", -3.25f);
        b->setSeed (kSeedB);
        b->uiLanguage = 1;
        const auto blob2 = stateOf (*b);
        auto c = make();
        load (*c, stateOf (*a));
        load (*c, blob2);
        load (*c, blob2);
        const auto x = xmlOf (stateOf (*c));
        const bool shape = x != nullptr && countChildren (*x, "Engine") == 1 && countChildren (*x, "Slots") == 1
                        && countChildren (*x, "PARAM") == 293;
        const bool liveClean = ! c->parameters.state.getChildWithName ("Engine").isValid()
                            && ! c->parameters.state.getChildWithName ("Slots").isValid();
        auto* pb = param (*b, "s3_pitch");
        auto* pc = param (*c, "s3_pitch");
        const bool follows = pb != nullptr && pc != nullptr && pb->getValue() == pc->getValue()
                          && std::abs (pc->convertFrom0to1 (pc->getValue()) + 3.25f) < 1e-4f
                          && c->getSeed() == kSeedB && c->uiLanguage.load() == 1 && bitwiseEqual (allValues (*b), allValues (*c));
        row (shape && liveClean && follows, "S3 A -> B -> change -> C: one of each child, values follow the change",
             juce::String (shape ? "" : "shape ") + (liveClean ? "" : "live tree carries custom children ") + (follows ? "" : "values"));
    }

    // S4: no <Engine> / uiLanguage -> seed and language unchanged; uiLanguage "1" -> fr.
    {
        auto a = make();
        a->setSeed (0x7777ull);
        a->uiLanguage = 2;
        auto x = xmlOf (stateOf (*a));
        x->removeChildElement (x->getChildByName ("Engine"), true);
        x->removeAttribute ("uiLanguage");
        auto b = make();
        b->setSeed (0x1234ull);
        b->uiLanguage = 1;
        load (*b, blobOf (*x));
        const bool kept = b->getSeed() == 0x1234ull && b->uiLanguage.load() == 1;

        x->setAttribute ("uiLanguage", "1");
        auto c = make();
        load (*c, blobOf (*x));
        row (kept && c->uiLanguage.load() == 1 && c->getSeed() != 0x7777ull,
             "S4 absent <Engine> / uiLanguage: seed + language kept; uiLanguage \"1\" -> fr");
    }

    // S5: a wrong root tag changes nothing; a garbage seed leaves the seed unchanged.
    {
        auto a = make();
        a->setSeed (0xABCDull);
        a->uiLanguage = 2;
        setParam (*a, "polyphony", 3.0f);
        setParam (*a, "output_gain", -12.0f);
        auto x = xmlOf (stateOf (*a));
        x->setTagName ("NotThisPlugin");

        auto b = make();
        b->setSeed (0x5555ull);
        b->uiLanguage = 1;
        setParam (*b, "glide", 100.0f);
        const auto before = allValues (*b);
        load (*b, blobOf (*x));
        const bool rootOk = b->getSeed() == 0x5555ull && b->uiLanguage.load() == 1 && bitwiseEqual (before, allValues (*b));

        bool seedOk = true;
        for (const char* bad : { "12ab", "" })
        {
            auto y = xmlOf (stateOf (*a));
            y->getChildByName ("Engine")->setAttribute ("seed", bad);
            auto c = make();
            c->setSeed (0x6666ull);
            load (*c, blobOf (*y));
            seedOk = seedOk && c->getSeed() == 0x6666ull;
        }
        row (rootOk && seedOk, "S5 wrong root tag: params / seed / language unchanged; seed \"12ab\" / \"\" ignored",
             juce::String (rootOk ? "" : "root ") + (seedOk ? "" : "seed"));
    }
}

//==============================================================================
static void hookRows()
{
    std::printf ("\n-- H. test hooks, VST3 extensions\n");
    {
        constexpr uint64_t kSeed = 0xDEADBEEF00C0FFEEull;
        auto q = make();
        q->setSeed (kSeed);
        q->prepareToPlay (48000.0, 512);
        juce::String e1, e2;
        const bool r1 = q->loadSlotSync (0, juce::File(), e1);
        juce::AudioBuffer<float> pcm (2, 16);
        pcm.clear();
        const bool r2 = q->loadSlotSync (0, pcm, 48000.0, "stub", e2);
        row (q->getSeed() == kSeed && ! r1 && ! r2 && e1.isNotEmpty() && e2.isNotEmpty(),
             "H1 setSeed before prepare survives; both loadSlotSync stubs return false + error", e1);
    }
    {
        auto q = make();
        auto* ext = q->getVST3ClientExtensions();
        row (ext != nullptr && ! ext->getPluginHasMainInput(), "H2 VST3 extensions present, getPluginHasMainInput() false (kAux)");
    }
}

//==============================================================================
// Stage 2 / Phase 2.1 engine gates (PLAN-2.1 T11). Included HERE, after the
// helpers above (row / Tally / make / param / allValues / stateOf) it uses.
#include "engine_gates_21.h"

// PLAN-2.2 T2: render hashes (prepareJob is the only later edit).
#include "render_hashes.h"

// PLAN-2.2 T9: parity gates vs O-Granulator @ 82406fce (--parity <dir> [--rows <prefix>]).
#include "parity_gates_22.h"

// PLAN-2.2 T9: engine gates (non-parity rows) + --cpu22.
#include "engine_gates_22.h"

// PLAN-2.3 T2: machine-speed witness (--calib; frozen reference tests/golden/cpu-calib-2.3.txt).
#include "cpu_calib.h"

// PLAN-2.3 T7: per-grain FX gates — ported unit rows (FU), engine rows (FN / FR /
// FV / FB / FZ / FA) + --cpu23, DSP-03 parity rows (PF / PFc / PS5-7, --parity).
#include "fx_units_23.h"
#include "parity_gates_23.h"
#include "engine_gates_23.h"

// PLAN-2.4 T3 / T12: working-copy feedback, Freeze, Reset, reverb + knee gates
// (TB / FL / CL / MS / RS / SR / FA4) + --cpu25.
#include "engine_gates_24.h"

// PLAN-2.4 T13: head clamp, click gates, reverb transitions (HC / CK / RV).
#include "engine_gates_24b.h"

// PLAN-2.4 T14: DSP-04 / DSP-05 parity rows (PB / PBc / PBd, PR / PRk / PRc, --parity).
#include "parity_gates_24.h"

// PLAN-2.5a T13 / T14: the law arm (PV / VL / ME / RG / NZ / CP / NV / SD / DR / MD) and
// edits, tempo, invariance, bypass (RT / RE / LM / TS / TT / BN / FA5 / TB3 / TB4 / ST1) + --cpu25a.
#include "engine_gates_25a.h"
#include "engine_gates_25b.h"

// PLAN-2.5b T11 / T12: voice modes, glide, D32 switch, sustain (VM / FB / GL / MS / SU);
// MPE, note expression, clicks, invariance, allocations, bypass (MP / MR / NE / CK5 /
// BS1 / AL1 / TB5 / TB6) + --cpu25b.
#include "engine_gates_25c.h"
#include "engine_gates_25d.h"

// PLAN-2.5a T7: DSP-06 matrix parity rows (PL / PL0t / PLs / PM / PE + controls, --parity).
#include "parity_gates_25a.h"

// PLAN-2.6 T12 / T13: Record-in capture / leaks / commit (eg26a); state / resolve /
// factory / hash / alloc / clicks / block size / telemetry / bypass (eg26b) + --cpu26.
#include "engine_gates_26a.h"
#include "engine_gates_26b.h"

// PLAN-3.1 T5 / PD11: the Stage 3.1 processor surface (presets, window curve,
// slot / overview revisions, overview, telemetry shape) + --ug31.
#include "ui_gates_31.h"

// PLAN-3.2 T6 / PD23: the Stage 3.2 processor surface (grain / voice telemetry,
// feedback + record heads, live overview + dirty, clear, import, save,
// encodedBytes) + --ug32.
#include "ui_gates_32.h"

// PLAN-3.3 T5 / PD21: the Stage 3.3 processor surface (16 display rows, steal
// ring + hold, menv / lfoPh / sources / modNorm / lo-hi, L / R meters, limit,
// take names) + --ug33.
#include "ui_gates_33.h"

// PLAN-4 PD5 / PD6: the curated factory-source generator (factory_gen.cpp,
// its own TU with -ffp-contract=off).
namespace factorygen
{
    int run (const juce::File& dir, bool force);
    int pin (const juce::File& dir);
    std::uint64_t fullPcmHash (const juce::AudioBuffer<float>& pcm);
}

// PLAN-4 (Stage 4 polish): auval drift set, factory presets / sources, D55
// rows + --pg4 / --cpu4.
#include "preset_gates_4.h"

// PLAN-R1 (Stage 4 R1): Record-in Arm auto-clear rows AC1-AC8 (PD31) + --egr1.
#include "engine_gates_r1.h"

// PLAN-R2 (Stage 4 R2): the level-hold makeup rows LH1-LH14 / LHB / LHF (PD11) + --egr2.
#include "engine_gates_r2.h"

//==============================================================================
// PLAN-2.6 T10 / PD16: the factory stub generator (--gen-factory <dir> [--force]).
// The same JUCE FLAC encoder, int path and quantiser as the plugin
// (SlotLoader::encodeFlac24, quality 5). Deterministic; licence-clean.
namespace { namespace genfactory
{
    ogi::SlotLoader::Pristine stubTone()
    {
        constexpr double fs = 48000.0;
        constexpr int n = 144000;               // 3.0 s
        constexpr int ramp = 480;               // 10 ms raised-cosine ends
        ogi::SlotLoader::Pristine p;
        p.rate = fs;
        p.name = "stub_tone";
        p.pcm.setSize (1, n);
        std::vector<double> x ((size_t) n);
        double pk = 0.0;
        for (int i = 0; i < n; ++i)
        {
            double v = 0.0;
            for (int k = 1; k <= 8; ++k)
                v += std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * k * (double) i / fs) / (double) k;
            x[(size_t) i] = v;
            pk = std::max (pk, std::abs (v));
        }
        const double g = std::pow (10.0, -12.0 / 20.0) / pk;   // −12 dBFS peak
        for (int i = 0; i < n; ++i)
        {
            double w = 1.0;
            if (i < ramp)          w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * (double) i / (double) ramp);
            else if (i >= n - ramp) w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * (double) (n - 1 - i) / (double) ramp);
            p.pcm.setSample (0, i, (float) (x[(size_t) i] * g * w));
        }
        return p;
    }

    ogi::SlotLoader::Pristine stubNoise()
    {
        constexpr double fs = 48000.0;
        constexpr int n = 144000;
        ogi::SlotLoader::Pristine p;
        p.rate = fs;
        p.name = "stub_noise";
        p.pcm.setSize (2, n);
        double acc = 0.0;
        std::vector<double> y (2 * (size_t) n);
        for (int c = 0; c < 2; ++c)
        {
            double z = 0.0;
            for (int i = 0; i < n; ++i)
            {
                z += 0.25 * ((double) eg21::hashNoise ((juce::int64) i, c, 1.0f) - z);   // one-pole LP
                y[(size_t) c * (size_t) n + (size_t) i] = z;
                acc += z * z;
            }
        }
        const double rms = std::sqrt (acc / (2.0 * n));
        const double g = std::pow (10.0, -18.0 / 20.0) / rms;   // −18 dBFS RMS
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
                p.pcm.setSample (c, i, (float) (y[(size_t) c * (size_t) n + (size_t) i] * g));
        return p;
    }

    int run (const juce::File& dir, bool force)
    {
        dir.createDirectory();
        int rc = 0;
        for (auto* make : { &stubTone, &stubNoise })
        {
            const auto p = make();
            const auto f = dir.getChildFile (p.name + ".flac");
            if (f.exists() && ! force)
            {
                std::printf ("refusing to overwrite %s (use --force)\n", f.getFullPathName().toRawUTF8());
                rc = 1;
                continue;
            }
            juce::MemoryBlock mb;
            if (! ogi::SlotLoader::encodeFlac24 (p, mb, 5))
            {
                std::printf ("encode failed: %s\n", p.name.toRawUTF8());
                rc = 1;
                continue;
            }
            f.replaceWithData (mb.getData(), mb.getSize());
            ogi::SlotLoader::Pristine d;
            juce::String err;
            const bool ok = ogi::SlotLoader::decodeMemoryFlac (mb.getData(), mb.getSize(), p.name, 48000.0, d, err);
            std::printf ("%s  %d bytes  decoded %s  contentHash %s  (%d ch, %d frames)\n", f.getFileName().toRawUTF8(),
                         (int) mb.getSize(), ok ? "ok" : err.toRawUTF8(),
                         ok ? ogi::gihash::toHex (ogi::gihash::contentHash (d.pcm)).toRawUTF8() : "-",
                         d.pcm.getNumChannels(), d.pcm.getNumSamples());
        }
        return rc;
    }
} }

//==============================================================================
// PLAN-3.1 PD12 / PLAN-3.2 PD25: `--dump-ui-overrides <path>` writes the generic
// UI stub's seed (tests/ui-stub/generic-overrides.json) from the processor
// itself: combos (choices + default index), every Float range, and the natives
// the page pulls, through the SAME encoders the editor uses (GiUiJson.h). Early
// exit; no gate runs. Deterministic: a pinned seed, a scripted clock (never a
// wall clock), a fixed block count, no path, LF line ends. scripts/ui-stub/generic-juce-stub.js
// answers a native with ONE value whatever its arguments, so getSlotOverview /
// getWindowCurve carry the slot 1 / stream 1 answer.
//
// Slot fixture: slot 1 the factory tone (really loaded; 150 blocks of a held
// note with feedback 80 % rewrite its tape, so its overview carries REAL live
// bins and `dirty` bytes and slots[0].dirty is the processor's own flag), slot
// 2 a trimmed `file:` source, slot 3 a missing recording, slot 4 empty with a
// failed-import `error` (2 - 4 are plain SlotInfo values through the pure
// encoder).
//
// Telemetry fixture (3.3 shape, GiUiJson::telemetryToVar (UiTelemetry); PLAN-3.3
// PD22): the revisions, the four ovRev and lane 1's feedback head are the REAL
// values of that run (buildUiTelemetry on the scripted clock). Everything else
// is written BY HAND into the struct before it is encoded, so the page fixture
// does not depend on the engine's grain / mod schedule: 16 display rows (row 0
// held + newest, row 1 released, row 2 stolen-held), menv for the three, 4 x 17
// lfoPh with LFO 1 per-voice, 17 sources with signs, modNorm for two routes
// (an LFO on lane 1's loop end = dest 28, Env 2 -> output_gain = dest 1),
// lo / hi on lane 1, out [0.5012, 0.1] (-6 / -20 dBFS), rec.peak [0.25, 0.05],
// limit 256; 3 / 2 / 2 / 3 grains on streams 1 - 4, one reversed, one with the
// clamp bit (flags 2); lane 2 recording: run 2, slot 2, pos 0.4. Grains sit on
// all four streams (the painter's stream indexing is what the stub page
// exercises) although slots 3 / 4 hold no tape; their playheads are -1.
//==============================================================================
static int dumpUiOverrides (const juce::File& out)
{
    auto p = make();
    p->setSeed (0x5EED3200CAFE0001ull);   // a fresh processor draws its seed: pinned, or the fed-back tape differs per run
    for (int n = 1; n <= 4; ++n)   // PLAN-R1 PD16: the former global on all four streams
        setParam (*p, Ids::stream (n, "feedback"), 80.0f);
    // PLAN-R2 (T4 deviation): the page fixture is a DATA stub for the UI gates,
    // not evidence of the loop's level. Its live overview bins come from 1.6 s
    // of a fed-back tape, so the level-hold makeup would move 1237 lines of the
    // committed file (tests/ui-stub is outside R2's fence). The fixture run
    // keeps the R1 write; the next UI-driven regeneration may drop this line
    // and re-pin.
    p->getEngine().hooks.makeupOff = true;
    p->prepareToPlay (48000.0, 512);

    ogi::SavedSlot tone;
    tone.index  = 0;
    tone.source = "factory:stub_tone";
    tone.name   = "stub_tone";
    p->getLoader().restoreSlots ({ tone });
    const bool idle = p->getLoader().waitForIdle (20000);

    // The real part of the fixture: a held note, feedback writing slot 1's
    // tape, on a scripted clock (ms = 1000 + samples / 48).
    using Tel = GranulatorInstrumentProcessor::UiTelemetry;
    constexpr int kBlocks = 150;
    double clock = 1000.0;
    {
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        for (int k = 0; k < kBlocks; ++k)
        {
            buf.clear();
            p->processBlock (buf, midi);
            midi.clear();
            clock += 1000.0 * 512.0 / 48000.0;
        }
    }
    Tel tel = p->buildUiTelemetry (clock);
    const bool realOk = ! tel.stale && tel.lanes[0].hasFb && p->getSlotDirty (0);

    auto* combos  = new juce::DynamicObject();
    auto* sliders = new juce::DynamicObject();
    int nc = 0, nf = 0;
    for (auto* ap : p->getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap);
        if (rp == nullptr)
            continue;
        const auto id = rp->getParameterID();
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (rp))
        {
            juce::Array<juce::var> choices;
            for (const auto& ch : c->choices)
                choices.add (ch);
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
    }

    // The infos AFTER the telemetry build (its slotRev is the state's revision).
    std::array<ogi::SlotInfo, ogi::kTapeSlots> infos;
    infos[0] = p->getSlotInfo (0);
    const bool toneOk = idle && infos[0].loaded && infos[0].source == "factory:stub_tone";
    {
        auto& i = infos[1];   // a 30 s trimmed file
        i.name = "Field Recording 03.wav";
        i.source = "file:/fixture/Field Recording 03.wav";   // never emitted (the page reads no path)
        i.loaded = true;
        i.trimmed = true;
        i.tapeRate = 48000.0;
        i.frames = 1440000;
        i.nativeRate = 44100.0;
        i.nativeFrames = 1323000;
        tel.lanes[1].ovRev |= 1u;   // loaded
    }
    {
        auto& i = infos[2];   // a recording whose file is gone (reference kept)
        i.name = "GI-Rec-fixture.wav";
        i.source = "rec:/fixture/GI-Rec-fixture.wav";
        i.missing = true;
        i.embed = true;
    }
    infos[3].lastError = "import failed (unreadable file): bad take.wav";   // an empty lane after a refused drop
    const std::array<bool, ogi::kTapeSlots> dirtyFlags { p->getSlotDirty (0), false, false, false };

    // The hand-written part of the telemetry (see the header note).
    for (auto& v : tel.voices)
        v = Tel::Voice {};
    for (auto& m : tel.menv)
        m = Tel::Menv {};
    // row 0: held, the newest voice (note 60, root 48 -> wantSt 12)
    tel.voices[0].state  = 1;
    tel.voices[0].note   = 60;
    tel.voices[0].amp    = 0.8f;
    tel.voices[0].scan   = { { 0.25f, 0.4f, -1.0f, -1.0f } };
    tel.voices[0].wantSt = 12.0f;
    // row 1: released (note 64, its release tail)
    tel.voices[1].state  = 2;
    tel.voices[1].note   = 64;
    tel.voices[1].amp    = 0.3f;
    tel.voices[1].scan   = { { 0.32f, 0.47f, -1.0f, -1.0f } };
    tel.voices[1].wantSt = 16.0f;
    // row 2: stolen, inside its hold (no playhead, amp fading)
    tel.voices[2].state  = 3;
    tel.voices[2].note   = 55;
    tel.voices[2].amp    = 0.05f;
    tel.newest = 0;
    tel.menv[0] = { 0.7f, 0.4f, 3, 2, 3 };   // { e1, e2, st1, st2, ampSt }: Env 1 sustain, Env 2 decay, amp sustain
    tel.menv[1] = { 0.2f, 0.1f, 4, 4, 4 };   // all releasing
    tel.menv[2] = { 0.05f, 0.0f, 4, 0, 4 };  // stolen: amp stage R
    for (auto& lr : tel.lfoPh)
        lr.fill (-1.0f);
    tel.lfoPh[0][0] = 0.125f;                // LFO 1 per-voice: global + rows 0 / 1
    tel.lfoPh[0][1] = 0.25f;
    tel.lfoPh[0][2] = 0.6f;
    tel.lfoPh[1][0] = 0.5f;                  // LFO 2-4 Global: the global phase only
    tel.lfoPh[2][0] = 0.75f;
    tel.lfoPh[3][0] = 0.0f;
    // sources 1-17: LFO 1-4, Env 1, Env 2, Velocity, Key, Note Random, Wheel,
    // Pressure, Slide, Bend, Macro 1-4 (Key 0 = note 60 newest).
    tel.sources = { { 0.5f, -0.3f, 0.8f, -0.9f, 0.7f, 0.4f, 0.8f, 0.0f, 0.37f, 0.2f, 0.15f, 0.3f, -0.25f, 0.6f, 0.0f, 1.0f, 0.45f } };
    tel.modRouted.fill (false);
    tel.modNorm.fill (0.0f);
    tel.modRouted[28] = true;                // LFO -> s1_loop_end (lane 1)
    tel.modNorm[28]   = 0.82f;
    tel.modRouted[1]  = true;                // Env 2 -> output_gain
    tel.modNorm[1]    = 0.64f;
    for (auto& lane : tel.lanes)
        lane.hasLoHi = false;
    tel.lanes[0].hasLoHi = true;             // lane 1: loop start unrouted (0), loop end modulated
    tel.lanes[0].lo = 0.0f;
    tel.lanes[0].hi = 0.82f;
    tel.outL = 0.5012f;                      // -6 dBFS
    tel.outR = 0.1f;                         // -20 dBFS
    tel.limit = 256;
    {
        // { stream, normPos, pan, env, gain, lengthSec, voice, flags }
        const Tel::Grain fixture[] = {
            { 1, 0.22f, -0.4f, 0.35f, 1.0f, 0.2f,  0, 0 }, { 1, 0.25f, 0.0f, 0.95f, 0.8f, 0.2f,  0, 0 }, { 1, 0.29f, 0.5f, 0.6f, 1.0f, 0.2f,  0, 1 },
            { 2, 0.38f, -0.7f, 0.5f, 0.9f, 0.12f, 1, 2 }, { 2, 0.43f, 0.3f, 0.85f, 1.0f, 0.12f, 0, 0 },
            { 3, 0.55f, 0.0f, 0.7f, 1.0f, 0.35f, 0, 0 },  { 3, 0.61f, 0.8f, 0.25f, 0.7f, 0.35f, 0, 0 },
            { 4, 0.72f, -1.0f, 0.9f, 1.0f, 0.06f, 0, 0 }, { 4, 0.78f, 0.2f, 0.45f, 0.85f, 0.06f, 0, 0 }, { 4, 0.83f, 1.0f, 0.15f, 1.0f, 0.06f, 0, 0 }
        };
        tel.grainCount = 0;
        for (const auto& g : fixture)
            tel.grains[static_cast<size_t> (tel.grainCount++)] = g;
        tel.counts = { { 3, 2, 2, 3 } };
    }
    tel.stale = false;
    tel.rec.bus = 1;
    tel.rec.signal = true;
    tel.rec.run = 2;
    tel.rec.slot = 2;
    tel.rec.pos = 0.4f;
    tel.rec.len = 4.0f;
    tel.rec.capped = false;
    tel.rec.peak = 0.25f;
    tel.rec.peakL = 0.25f;
    tel.rec.peakR = 0.05f;
    for (auto& lane : tel.lanes)
        lane.hasRec = false;
    tel.lanes[1].hasRec = true;
    tel.lanes[1].rec = 0.4f;

    auto* natives = new juce::DynamicObject();
    natives->setProperty ("getScaleState", GiUiJson::scaleStateToVar (*p));
    natives->setProperty ("getSlotState", GiUiJson::slotStateToVar (tel.slotRev, infos, dirtyFlags));
    natives->setProperty ("getTelemetry", GiUiJson::telemetryToVar (tel));
    natives->setProperty ("getParameterDefaults", GiUiJson::parameterDefaultsToVar (*p));
    natives->setProperty ("getWindowCurve", GiUiJson::windowCurveToVar (*p, 0));
    natives->setProperty ("getSlotOverview", GiUiJson::slotOverviewToVar (*p, 0));
    natives->setProperty ("getFactorySources", GiUiJson::factorySourcesToVar());   // 1.2.0: from the table (1.1.0 added the 8 by hand)
    natives->setProperty ("getUiLanguage", "en");

    auto* root = new juce::DynamicObject();
    root->setProperty ("_why", "Generated from the processor by the render harness (PLAN-3.1 PD12, PLAN-3.2 PD25, PLAN-3.3 PD22) - never hand-edit. "
                               "Regenerate (cwd plugins/O-GranulatorInstrument): O-GranulatorInstrument-render-harness "
                               "--dump-ui-overrides tests/ui-stub/generic-overrides.json");
    root->setProperty ("combos", juce::var (combos));
    root->setProperty ("sliders", juce::var (sliders));
    root->setProperty ("natives", juce::var (natives));
    const juce::var rootVar (root);   // owns the whole tree from here

    // JSON::toString breaks lines with juce::newLine (CRLF): LF only, here and on disk.
    const auto text = (juce::JSON::toString (rootVar, false) + "\n").replace ("\r\n", "\n");

    // Shape + hygiene: the counts, the fixture, and nothing machine-specific.
    const auto nat = rootVar["natives"];
    const int nd     = nat["getParameterDefaults"].getDynamicObject()->getProperties().size();
    const int curve  = nat["getWindowCurve"].size();
    const int peaks  = nat["getSlotOverview"]["peaks"].size();
    const bool revs  = static_cast<int> (nat["getTelemetry"]["slotRev"]) == static_cast<int> (nat["getSlotState"]["revision"])
                    && static_cast<int> (nat["getTelemetry"]["scaleRev"]) == static_cast<int> (nat["getScaleState"]["revision"])
                    && static_cast<int> (nat["getTelemetry"]["lanes"][0]["ovRev"]) == static_cast<int> (nat["getSlotOverview"]["rev"]);
    // The 3.3 fixture (PLAN-3.3 PD22 over the 3.2 PD25 one): 16 rows (held /
    // released / stolen), newest 0, menv / lfoPh / sources / modNorm / lo-hi /
    // out / rec.peak L R / limit, 10 grains over four streams (one clamp bit),
    // fb on lane 1, lane 2 recording at 0.4, live dirty on lane 1, one error.
    const auto telVar = nat["getTelemetry"];
    int held = 0, ticks = 0, errors = 0, clampBits = 0;
    std::array<int, 4> byState {};
    for (int k = 0; k < telVar["voices"].size(); ++k)
        ++byState[(size_t) juce::jlimit (0, 3, static_cast<int> (telVar["voices"][k][0]))];
    held = byState[1];
    for (int k = 0; k < telVar["grains"].size(); ++k)
        clampBits += (static_cast<int> (telVar["grains"][k][7]) & 2) != 0 ? 1 : 0;
    const auto mnKeys = telVar["modNorm"].getDynamicObject() != nullptr ? telVar["modNorm"].getDynamicObject()->getProperties().size() : -1;
    for (int k = 0; k < nat["getSlotOverview"]["dirty"].size(); ++k)
        ticks += static_cast<int> (nat["getSlotOverview"]["dirty"][k]) > 12 ? 1 : 0;   // above the page's 5 % tick threshold
    for (int k = 0; k < nat["getSlotState"]["slots"].size(); ++k)
        errors += nat["getSlotState"]["slots"][k]["error"].toString().isNotEmpty() ? 1 : 0;
    const bool fixture = realOk && held == 1 && byState[2] == 1 && byState[3] == 1 && telVar["voices"].size() == 16 && telVar["grains"].size() == 10
                      && static_cast<int> (telVar["newest"]) == 0 && telVar["menv"].size() == 16 && telVar["lfoPh"].size() == 4
                      && telVar["lfoPh"][0].size() == 17 && telVar["sources"].size() == 17 && mnKeys == 2 && clampBits == 1
                      && telVar["lanes"][0].hasProperty ("hi") && ! telVar["lanes"][1].hasProperty ("hi") && static_cast<int> (telVar["limit"]) == 256
                      && static_cast<double> (telVar["out"][0]) == 0.5012 && static_cast<double> (telVar["rec"]["peak"][1]) == 0.05
                      && static_cast<double> (telVar["lanes"][0]["fb"]) > 0.0 && telVar["lanes"][0]["rec"].isVoid()
                      && telVar["lanes"][1]["fb"].isVoid() && static_cast<double> (telVar["lanes"][1]["rec"]) == 0.4
                      && static_cast<int> (telVar["rec"]["run"]) == 2 && static_cast<int> (telVar["rec"]["slot"]) == 2
                      && static_cast<double> (telVar["rec"]["pos"]) == 0.4 && ticks > 0
                      && static_cast<bool> (nat["getSlotState"]["slots"][0]["dirty"]) && errors == 1;
    bool clean = ! text.contains ("\r");
    for (const auto& dir : { juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                             realHome(),   // PLAN-4 PD15: the process home is the temp one; the real one is checked too
                             juce::File::getSpecialLocation (juce::File::tempDirectory),
                             juce::File::getCurrentWorkingDirectory(),
                             harnessRecordingsDir().getParentDirectory() })
        clean = clean && ! text.contains (dir.getFullPathName());
    clean = clean && ! text.contains (harnessRecordingsDir().getParentDirectory().getFileName())                       // the temp-dir name
                  && ! text.contains (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFileName())     // the temp-home name
                  && ! text.contains (realHome().getFileName());                                                       // the user name

    const bool shape = toneOk && nc == 74 && nf == 200 && nd == 274 && curve == 64 && peaks == 656 && revs && fixture && clean;
    bool ok = false;
    if (shape)
    {
        out.getParentDirectory().createDirectory();
        ok = out.replaceWithText (text, false, false, "\n");
    }
    std::printf ("dump-ui-overrides: %d combos, %d sliders, %d defaults, %d curve points, %d peaks, revisions %s, tone %s, "
                 "3.3 fixture %s (%d held / %d released / %d stolen rows, %d grains, fb %.4f, %d dirty ticks, %d error), hygiene %s -> %s (%s)\n",
                 nc, nf, nd, curve, peaks, revs ? "match" : "MISMATCH", toneOk ? "loaded" : "NOT LOADED", fixture ? "ok" : "INCOMPLETE",
                 held, byState[2], byState[3], static_cast<int> (telVar["grains"].size()), static_cast<double> (telVar["lanes"][0]["fb"]), ticks, errors,
                 clean ? "clean" : "PATH / CR FOUND",
                 out.getFileName().toRawUTF8(), ok ? "ok" : (shape ? "WRITE FAILED" : "NOT WRITTEN"));
    return ok ? 0 : 1;
}

//==============================================================================
// PLAN-4 PD15 — HM1: the harness never writes the real home. Predicates:
//   (a) the process home is the per-run temp dir (the re-exec took), never the
//       real one — checked BEFORE any processor is built: without it no
//       processor is made at all (HM1n, `--no-home-redirect`, must not write
//       the real bank either);
//   (b) a processor's Factory and User preset dirs resolve under the temp home,
//       and a real user-preset save lands there (the stimulus: the same
//       userHomeDirectory resolution the factory-bank write uses);
//   (c) the real ~/Library/O-GranulatorInstrument and the real
//       ~/Music/Ouaricon/O-GranulatorInstrument are unchanged since main()
//       (both snapshots are taken from the REAL home: realHome() != the process
//       home is asserted, so the guards GD1 / PD24 / hygiene are not vacuous).
static void rowHM1()
{
    const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    const auto tmp = tempHome();
    const bool redirected = tmp != juce::File() && home == tmp && home != realHome() && tmp.isDirectory();
    const bool realDistinct = realHome() != home && realHome().isDirectory();
    bool dirsUnder = false, wroteTemp = false;
    if (redirected)
    {
        struct NullLogger final : juce::Logger { void logMessage (const juce::String&) override {} } nullLogger;
        struct LoggerScope
        {
            explicit LoggerScope (juce::Logger* l) : previous (juce::Logger::getCurrentLogger()) { juce::Logger::setCurrentLogger (l); }
            ~LoggerScope() { juce::Logger::setCurrentLogger (previous); }
            juce::Logger* previous;
        } loggerScope (&nullLogger);
        auto p = make();
        auto& pm = p->getPresetManager();
        dirsUnder = pm.getFactoryPresetsDirectory().isAChildOf (tmp) && pm.getUserPresetsDirectory().isAChildOf (tmp);
        const auto probe = pm.getUserPresetsDirectory().getChildFile ("HM1 probe.json");
        wroteTemp = pm.savePreset ("HM1 probe") && probe.existsAsFile();
        probe.deleteFile();
    }
    const auto lib = pd24::snapLib();
    const auto mus = pd24::snapReal();
    const bool realSame = lib == pd24::gLibStart && mus == pd24::gStart;
    row (redirected && realDistinct && dirsUnder && wroteTemp && realSame,
         "HM1 PD15 the harness never writes the real home (CFFIXED_USER_HOME temp re-exec)",
         juce::String ("meas redirected ") + (redirected ? "yes" : "NO") + ", real home distinct " + (realDistinct ? "yes" : "NO")
             + ", preset dirs under temp " + (dirsUnder ? "yes" : "no") + ", probe save in temp " + (wroteTemp ? "yes" : "no")
             + ", real ~/Library/O-GranulatorInstrument " + (lib.exists ? juce::String (lib.files) + " entries" : "absent")
             + (lib == pd24::gLibStart ? " (unchanged)" : " (CHANGED)") + ", real ~/Music/.../O-GranulatorInstrument "
             + (mus == pd24::gStart ? "unchanged" : "CHANGED") + " | thr all | ctrl HM1n --no-home-redirect");
}

// HM1n: the same binary re-run with --no-home-redirect (a child process; it
// builds no processor) -> its HM1 must FAIL.
static void rowHM1n()
{
    char exe[4096];
    std::uint32_t exeLen = sizeof (exe);
    juce::String out;
    int rc = -1;
    if (_NSGetExecutablePath (exe, &exeLen) == 0)
    {
        // The child must launch with the REAL environment (no CFFIXED_USER_HOME,
        // no OGI_HARNESS_HOME): unset around start() only, then restored. The
        // parent's own home was resolved at its launch and does not move.
        const std::string keepTmp = tempHome().getFullPathName().toStdString(), keepReal = realHome().getFullPathName().toStdString();
        unsetenv ("OGI_HARNESS_HOME");
        unsetenv ("OGI_REAL_HOME");
        unsetenv ("CFFIXED_USER_HOME");
        setenv ("HOME", keepReal.c_str(), 1);
        juce::ChildProcess c;
        const bool started = c.start (juce::StringArray { juce::String::fromUTF8 (exe), "--no-home-redirect" });
        setenv ("OGI_HARNESS_HOME", keepTmp.c_str(), 1);
        setenv ("OGI_REAL_HOME", keepReal.c_str(), 1);
        setenv ("CFFIXED_USER_HOME", keepTmp.c_str(), 1);
        setenv ("HOME", keepTmp.c_str(), 1);
        if (started)
        {
            out = c.readAllProcessOutput();
            c.waitForProcessToFinish (30000);
            rc = static_cast<int> (c.getExitCode());
        }
    }
    const bool fired = rc == 1 && out.contains ("[FAIL] HM1 ") && out.contains ("redirected NO");
    row (fired, "HM1n control: --no-home-redirect -> HM1 FAILS (predicate must FAIL; no processor built)",
         "meas child rc " + juce::String (rc) + " | thr HM1 FAIL | " + eg21::ctrl (fired, "HM1 predicate"));
}

static int runHarness (int argc, char** argv)
{
    for (int a = 1; a < argc; ++a)
    {
        if (std::strcmp (argv[a], "--gen-factory") == 0 && a + 1 < argc)   // PLAN-4 PD5: the 8 curated sources
        {
            const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
            const bool force = a + 1 < argc && std::strcmp (argv[a + 1], "--force") == 0;
            return factorygen::run (dir, force);
        }
        else if (std::strcmp (argv[a], "--pin-factory") == 0 && a + 1 < argc)   // 1.2.0: the two pins of every .flac in a folder, as JSON lines
            return factorygen::pin (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[a + 1])));
        else if (std::strcmp (argv[a], "--gen-factory-stubs") == 0 && a + 1 < argc)   // PLAN-2.6 PD16: the two harness fixtures
        {
            const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
            const bool force = a + 1 < argc && std::strcmp (argv[a + 1], "--force") == 0;
            return genfactory::run (dir, force);
        }
        else if (std::strcmp (argv[a], "--golden-dir") == 0 && a + 1 < argc)
            gGoldenDir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
        else if (std::strcmp (argv[a], "--makeup-off") == 0)   // PLAN-R2 PD8: applied in main() (before any processor); a modifier, not a mode
            continue;
        else if (std::strcmp (argv[a], "--parity") == 0 && a + 1 < argc)
        {
            const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
            juce::String prefix;
            if (a + 2 < argc && std::strcmp (argv[a + 1], "--rows") == 0)
            {
                prefix = juce::String::fromUTF8 (argv[a + 2]);
                a += 2;
            }
            const int rc = pg22::run (dir, prefix);
            if (rc != 0)
                return rc;
            if (prefix.isEmpty() || prefix.startsWith ("PF") || prefix == "PS" || prefix.startsWith ("PS5")
                || prefix.startsWith ("PS6") || prefix.startsWith ("PS7"))
                pg23::run (dir, prefix);
            if (prefix.isEmpty() || prefix.startsWith ("PB") || prefix.startsWith ("PR"))
                pg24::run (dir, prefix);
            if (prefix.isEmpty() || prefix.startsWith ("PL") || prefix.startsWith ("PM") || prefix.startsWith ("PE"))
                pg25a::run (dir, prefix);
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--cpu22") == 0)
        {
            eg22::cpu22();
            return 0;
        }
        else if (std::strcmp (argv[a], "--cpu23") == 0)
        {
            const int f = eg23::cpu23();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return f;
        }
        else if (std::strcmp (argv[a], "--cpu25") == 0)   // PLAN-2.4 PD17: run alone; rc = FAIL count
        {
            const int f = eg24::cpu25();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return f;
        }
        else if (std::strcmp (argv[a], "--eg25") == 0)    // dev: the Phase 2.5a engine gates only (eg25a + eg25b; OGI_ROWS filters)
        {
            eg25a::runAll();
            eg25b::runAll();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--eg25b") == 0)   // dev: the Phase 2.5b engine gates only (eg25c + eg25d; OGI_ROWS filters)
        {
            eg25c::runAll();
            eg25d::runAll();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--eg26") == 0)    // dev: the Phase 2.6 engine gates only (eg26a + eg26b; OGI_ROWS filters)
        {
            eg26b::guardStart();
            eg26a::runAll();
            eg26b::runAll();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--ug31") == 0)    // dev: the Phase 3.1 processor-surface gates only (OGI_ROWS filters)
        {
            ug31::runAll();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--ug32") == 0)    // dev: the Phase 3.2 processor-surface gates only (OGI_ROWS filters)
        {
            ug32::runAll();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--ug33") == 0)    // dev: the Phase 3.3 processor-surface gates only (OGI_ROWS filters)
        {
            ug33::runAll();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--dump-ui-overrides") == 0 && a + 1 < argc)   // PLAN-3.1 PD12 / PLAN-3.2 PD25: early exit, no gate runs
            return dumpUiOverrides (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a])));
        else if (std::strcmp (argv[a], "--cpu26") == 0)   // PLAN-2.6 T17: run alone; rc = FAIL count (C25m / C25tm), C26rec INFO
        {
            const int f = eg26b::cpu26();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return f;
        }
        else if (std::strcmp (argv[a], "--cpu25a") == 0)  // PLAN-2.5a PD18: run alone; rc = FAIL count
        {
            const int f = eg25b::cpu25a();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return f;
        }
        else if (std::strcmp (argv[a], "--cpu25b") == 0)  // PLAN-2.5b PD18: run alone; C25mpe INFO, rc = INVALID count
        {
            const int f = eg25d::cpu25b();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return f;
        }
        else if (std::strcmp (argv[a], "--eg24") == 0)    // dev: the Phase 2.4 engine gates only (eg24 + eg24b)
        {
            eg24::runAll();
            eg24b::runAll();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--pg4") == 0)     // dev: the Stage 4 gates only (OGI_ROWS filters)
        {
            pg4::runAll();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--egr1") == 0)    // dev: the R1 rows only (PLAN-R1 PD30 / PD31; OGI_ROWS filters)
        {
            egr1::runAll();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--egr2") == 0 || std::strcmp (argv[a], "--egr2-long") == 0)   // dev: the R2 rows only (PLAN-R2 PD11; OGI_ROWS filters; -long adds INFO LH11)
        {
            egr2::runAll (std::strcmp (argv[a], "--egr2-long") == 0);
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--cpu4") == 0)    // PLAN-4 PD16: run alone, in the background; rc = FAIL count
            return juce::jmin (pg4::cpu4(), 255);
        else if (std::strcmp (argv[a], "--dump-factory-presets") == 0 && a + 1 < argc)   // PLAN-4 PD16: early exit, no gate runs
        {
            const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
            const int n = pg4::dumpFactoryPresets (dir);
            std::printf ("dump-factory-presets: %d files -> %s\n", n, dir.getFullPathName().toRawUTF8());
            return n == 64 ? 0 : 1;
        }
        else if (std::strcmp (argv[a], "--dump-preset-renders") == 0 && a + 1 < argc)   // PLAN-R1 PD29: early exit, no gate runs
        {
            const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
            return pg4::dumpPresetRenders (dir);
        }
        else if (std::strcmp (argv[a], "--cmp-preset-renders") == 0 && a + 1 < argc)    // PLAN-R1 PD29: PT0 / PT1 / PT2; 77 = no reference
        {
            const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
            bool allBitwise = false, excludeShowcase = false, r2 = false;
            for (; a + 1 < argc; ++a)
            {
                if (std::strcmp (argv[a + 1], "--all-bitwise") == 0) allBitwise = true;
                else if (std::strcmp (argv[a + 1], "--exclude-showcase") == 0) excludeShowcase = true;
                else if (std::strcmp (argv[a + 1], "--r2") == 0) r2 = true;   // PLAN-R2 PD12: PT0r / PT3 / PT3n
                else break;
            }
            return pg4::cmpPresetRenders (dir, allBitwise, excludeShowcase, r2);
        }
        else if (std::strcmp (argv[a], "--preset-level-table") == 0 && a + 1 < argc)    // PLAN-R2 PD13: prints, not a row; 77 = no reference
        {
            const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
            return pg4::presetLevelTable (dir);
        }
        else if (std::strcmp (argv[a], "--hm1") == 0)     // dev: PLAN-4 PD15 HM1 + HM1n alone (the T11 pre-check)
        {
            rowHM1();
            rowHM1n();
            std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
            return juce::jmin (failures, 255);
        }
        else if (std::strcmp (argv[a], "--calib") == 0)
            return cal::runCalib();
        else if (std::strcmp (argv[a], "--dump-render-hashes") == 0 && a + 1 < argc)
            return rh::dump (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a])));
        else if (std::strcmp (argv[a], "--dump-render-hashes-legacy") == 0 && a + 1 < argc)   // PLAN-2.4 PD1
            return rh::dump (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a])), true);
        else
        {
            std::printf ("unknown argument: %s\nusage: %s [--golden-dir <dir>] [--dump-render-hashes <file>] [--dump-render-hashes-legacy <file>] [--parity <dir> [--rows <prefix>]] [--cpu22] [--cpu23] [--cpu25] [--cpu25a] [--cpu25b] [--cpu26] [--eg24] [--eg25] [--eg25b] [--eg26] [--ug31] [--ug32] [--ug33] [--dump-ui-overrides <file>] [--gen-factory <dir> [--force]] [--pin-factory <dir>] [--gen-factory-stubs <dir> [--force]] [--pg4] [--egr1] [--cpu4] [--dump-factory-presets <dir>] [--dump-preset-renders <dir>] [--cmp-preset-renders <dir> [--all-bitwise] [--exclude-showcase]] [--hm1] [--no-home-redirect] [--calib] [--makeup-off (first)] [--egr2] [--egr2-long] [--preset-level-table <dir>]\n", argv[a], argv[0]);
            return 255;
        }
    }

    std::printf ("O-GranulatorInstrument render harness (Stage 1 + Phase 2.1 + Phase 2.2 + Phase 2.3 + Phase 2.4 + Phase 2.5a + Phase 2.5b + Phase 2.6)\n");
    eg26b::guardStart();   // GD1: the real Recordings folder, snapshot before any row
    paramRows();
    busRows();
    midiRows();
    renderRows();
    stateRows();
    hookRows();
    eg21::runAll();
    eg22::runAll();
    fu23::runAll();
    eg23::runAll();
    eg24::runAll();
    eg24b::runAll();
    eg25a::runAll();
    eg25b::runAll();
    eg25c::runAll();
    eg25d::runAll();
    eg26a::runAll();
    eg26b::runAll();
    ug31::runAll();   // PLAN-3.1 PD11 (after every existing row: their order and output are unchanged)
    ug32::runAll();   // PLAN-3.2 PD23 (after ug31: the carried rows keep their order and output)
    ug33::runAll();   // PLAN-3.3 PD21 (after ug32: the carried rows keep their order)
    pg4::runAll();    // PLAN-4 (after ug33: the carried rows keep their order)
    egr1::runAll();   // PLAN-R1 PD30 (after pg4, before HM1: the carried order is unchanged)
    egr2::runAll();   // PLAN-R2 PD11 (after egr1, before HM1)
    rowHM1();         // PLAN-4 PD15 (last: it judges the whole run)
    rowHM1n();

    std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
    return juce::jmin (failures, 255);
}

//==============================================================================
int main (int argc, char** argv)
{
    // PLAN-4 PD15: re-exec once with a per-run temp home (the parent
    // O-Granulator main.cpp pattern). JUCE resolves home through
    // NSHomeDirectory(), which honours CFFIXED_USER_HOME only when it is set
    // at launch; HOME alone is ignored. The real home is captured first.
    bool noRedirect = false;
    for (int a = 1; a < argc; ++a)
        noRedirect = noRedirect || std::strcmp (argv[a], "--no-home-redirect") == 0;
    if (! noRedirect && std::getenv ("OGI_HARNESS_HOME") == nullptr)
    {
        const char* fixed = std::getenv ("CFFIXED_USER_HOME");
        const passwd* pw = getpwuid (getuid());
        const std::string real = fixed != nullptr ? fixed : (pw != nullptr && pw->pw_dir != nullptr ? pw->pw_dir : "");
        if (real.empty()) { std::printf ("FATAL: cannot resolve the real home\n"); return 3; }
        const char* td = std::getenv ("TMPDIR");
        std::string tmpl = std::string (td != nullptr ? td : "/tmp/") + "/ogi-harness-home-XXXXXX";
        if (mkdtemp (tmpl.data()) == nullptr) { std::printf ("FATAL: mkdtemp failed\n"); return 3; }
        char exe[4096];
        std::uint32_t exeLen = sizeof (exe);
        if (_NSGetExecutablePath (exe, &exeLen) != 0) { std::printf ("FATAL: executable path\n"); return 3; }
        setenv ("OGI_REAL_HOME", real.c_str(), 1);
        setenv ("OGI_HARNESS_HOME", tmpl.c_str(), 1);
        setenv ("CFFIXED_USER_HOME", tmpl.c_str(), 1);
        setenv ("HOME", tmpl.c_str(), 1);
        std::fflush (stdout);
        execv (exe, argv);
        std::printf ("FATAL: re-exec failed\n");
        return 3;
    }
    struct HomeCleanup { juce::File d; ~HomeCleanup() { if (d != juce::File()) d.deleteRecursively(); } } homeCleanup { tempHome() };

    // PLAN-R2 PD8: the harness-wide makeup-off switch (`--makeup-off` first, or
    // OGI_MAKEUP_OFF=1): every engine of this process runs the R1 write, so a
    // whole run can be compared with the R2 baseline (RB).
    {
        bool mkOff = std::getenv ("OGI_MAKEUP_OFF") != nullptr && std::strcmp (std::getenv ("OGI_MAKEUP_OFF"), "0") != 0;
        for (int a = 1; a < argc; ++a)
            mkOff = mkOff || std::strcmp (argv[a], "--makeup-off") == 0;
        ogi::EngineTestHooks::makeupOffAll = mkOff;
        if (mkOff)
            std::printf ("MAKEUP OFF (harness-wide, PLAN-R2 PD8): every engine runs the R1 write\n");
    }

    juce::ScopedJuceInitialiser_GUI init;
    if (noRedirect)
    {
        // HM1n: no re-exec. HM1's predicate (a) fails before any processor is
        // built, so nothing (no factory bank, no Recordings) is written.
        pd24::gStart = pd24::snapReal();
        pd24::gLibStart = pd24::snapLib();
        std::printf ("O-GranulatorInstrument render harness: HM1n (--no-home-redirect) -- HM1 must FAIL\n");
        rowHM1();
        std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
        return juce::jmin (failures, 255);
    }
    if (juce::File::getSpecialLocation (juce::File::userHomeDirectory) != tempHome())
    {
        std::printf ("FATAL: home redirect did not take (%s) -- refusing to run\n",
                     juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName().toRawUTF8());
        return 3;
    }
    RecDirGuard recDirGuard;   // PLAN-2.6 / PLAN-3.2 PD24: the harness Recordings + Imports temp tree is removed at exit

    // PD24: before any other processor work, in every mode.
    if (! pd24::start())
        return 254;
    int rc = runHarness (argc, argv);
    if (! pd24::end())
        rc = juce::jmax (rc, 1);
    return rc;
}
