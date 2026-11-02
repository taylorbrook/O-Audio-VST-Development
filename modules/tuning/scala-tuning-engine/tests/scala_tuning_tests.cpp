/*
   This file is part of the Ouaricon Audio scala-tuning-engine module.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/
/*
    scala-tuning-engine tests. Exit 0 iff every check passes. Each check names
    the review finding it pins, from the fork it was upstreamed from in v3.3.0.
    Build: see tests/CMakeLists.txt.
*/

#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
#include <cmath>
#include <cstdio>
#include <thread>

static int failures = 0, checks = 0;

static void check(const char* name, bool ok)
{
    ++checks;
    if (! ok) ++failures;
    std::printf("%s  %s\n", ok ? "PASS" : "FAIL", name);
}

static bool near(double a, double b, double tol = 1e-6) { return std::abs(a - b) <= tol; }

static juce::File tempWith(const juce::String& text, const char* ext)
{
    auto f = juce::File::createTempFile(ext);
    f.replaceWithText(text);
    return f;
}

static bool loadScl(TuningEngine& e, const juce::String& text)
{
    auto f = tempWith(text, ".scl");
    const bool ok = e.loadScalaFile(f);
    f.deleteFile();
    return ok;
}

static bool loadKbm(TuningEngine& e, const juce::String& text)
{
    auto f = tempWith(text, ".kbm");
    const bool ok = e.loadKBMFile(f);
    f.deleteFile();
    return ok;
}

// size, first, last, middle, refNote, refHz, octaveDegree, then the map.
static juce::String kbm(int size, int middle, int refNote, double refHz, int octDeg, const juce::String& map)
{
    return juce::String(size) + "\n0\n127\n" + juce::String(middle) + "\n" + juce::String(refNote) + "\n"
         + juce::String(refHz, 6) + "\n" + juce::String(octDeg) + "\n" + map;
}

static juce::String identityMap(int n)
{
    juce::String s;
    for (int i = 0; i < n; ++i) s += juce::String(i) + "\n";
    return s;
}

static std::vector<double> twelveTet()
{
    return { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200 };
}

static void testScalaParser()
{
    {
        TuningEngine e;
        const bool ok = loadScl(e, "neg\n2\n-50.0\n1200.0\n");
        const auto iv = e.getIntervals();
        check("WR-08/WR-11 negative cents are a legal pitch",
              ok && iv.size() == 3 && near(iv[1], -50.0) && near(iv[2], 1200.0));
    }
    {
        TuningEngine e;
        const bool ok = loadScl(e, "! comment\n\n2\n3/2\n2/1\n");
        const auto iv = e.getIntervals();
        check("WR-08/WR-11 blank description is consumed, count stays the count",
              ok && iv.size() == 3 && near(iv[1], 701.955, 1e-3) && near(iv[2], 1200.0));
    }
    {
        TuningEngine e;
        const bool ok = loadScl(e, "name\n2\n3/2 perfect fifth\n1200.0 octave\n");
        const auto iv = e.getIntervals();
        check("WR-08 text after the pitch token is a comment",
              ok && iv.size() == 3 && near(iv[1], 701.955, 1e-3));
    }
    {
        TuningEngine e;
        const auto before = e.getIntervals();
        const bool rejected = ! loadScl(e, "name\n3\n100.0\nnot-a-pitch\n1200.0\n")
                           && ! loadScl(e, "name\n3\n100.0\n1200.0\n")
                           && ! loadScl(e, "name\n2\n3/2/5\n1200.0\n")
                           && ! loadScl(e, "name\n2\n" + juce::String::repeatedString("9", 400) + "/"
                                           + juce::String::repeatedString("9", 400) + "\n1200.0\n");
        check("WR-08 malformed, short, double-slash and inf/inf files are rejected",
              rejected && e.getIntervals() == before);
    }
    {
        TuningEngine e;
        check("IN-08 zero degree count is rejected", ! loadScl(e, "name\n0\n1200.0\n"));
        check("WR-11 absurd degree count is rejected", ! loadScl(e, "name\n5000\n1200.0\n"));
    }
    {
        TuningEngine e;
        check("WR-08 module-generated .scl round-trips",
              [&] { e.setCustomIntervals({ 0, 133.5, 702, 1200 }, "rt");
                    const auto text = e.generateScalaFileContent();
                    TuningEngine f;
                    return loadScl(f, text) && f.getIntervals() == e.getIntervals(); }());
    }
}

static void testKbm(bool skipHanging)
{
    {
        TuningEngine e;
        e.setCustomIntervals(twelveTet(), "t");
        const bool ok = loadKbm(e, kbm(12, 60, 60, 261.63, 12, identityMap(12)));
        check("CR-07 60 @ 261.63 plays middle C at 261.63, not the 400 Hz clamp",
              ok && near(e.getFrequency(60), 261.63) && near(e.getFrequency(72), 523.26));
        check("CR-07 loading a .kbm leaves the user's A4 alone", near(e.getMasterTune(), 440.0));

        e.setMasterTune(450.0);
        check("CR-07 master tune does not move notes the file references",
              near(e.getFrequency(60), 261.63));
        check("CR-07 generated .kbm carries the file's reference frequency",
              e.generateKBMFileContent().contains("261.630000"));
    }
    {
        // The O-Bowed v1.9.4 call order: load, hold A4, pin the reference.
        TuningEngine e;
        e.setCustomIntervals(twelveTet(), "t");
        loadKbm(e, kbm(12, 60, 60, 261.63, 12, identityMap(12)));
        check("3.2.0 getKbmReferenceFrequency returns the file's value",
              near(e.getKbmReferenceFrequency(), 261.63) && e.getKbmReferenceNote() == 60);
        e.setMasterTune(440.0);
        e.setKbmReferenceFrequency(261.6256);
        check("3.2.0 setKbmReferenceFrequency overrides the file", near(e.getFrequency(60), 261.6256));
        e.setKbmReferenceFrequency(0.0);
        check("3.2.0 override 0 falls back to the master tune", near(e.getFrequency(60), 440.0));
    }
    {
        TuningEngine e;
        e.setCustomIntervals(twelveTet(), "t");
        loadKbm(e, kbm(12, 60, 60, 300.0, 12, identityMap(12)));
        // Without CR-08 the 2-billion claim allocates until the OS gives up.
        const bool rejected = (skipHanging || ! loadKbm(e, kbm(2000000000, 60, 60, 500.0, 12, identityMap(12))))
                           && ! loadKbm(e, kbm(-1, 60, 60, 500.0, 12, identityMap(12)))
                           && ! loadKbm(e, kbm(129, 60, 60, 500.0, 12, identityMap(12)));
        check("CR-08/WR-12 out-of-range map size is rejected and keeps the old map",
              rejected && near(e.getFrequency(60), 300.0));
        check("CR-08 map size 128 still loads",
              loadKbm(e, kbm(128, 60, 60, 300.0, 12, identityMap(128))));
    }
    {
        TuningEngine e;
        e.setCustomIntervals(twelveTet(), "t");
        loadKbm(e, kbm(12, 60, 60, 261.63, 7, identityMap(12)));
        check("IN-11 formal octave degree sets the pattern jump (degree 7 = 700c)",
              near(e.getFrequency(61), 261.63 * std::pow(2.0, 100.0 / 1200.0))
           && near(e.getFrequency(72), 261.63 * std::pow(2.0, 700.0 / 1200.0)));
    }
    {
        TuningEngine e;
        e.setCustomIntervals(twelveTet(), "t");
        check("CR-08 absurd octave degree loads without blowing up",
              loadKbm(e, kbm(12, 60, 60, 261.63, 2000000000, identityMap(12)))
           && std::isfinite(e.getFrequency(127)) && e.getFrequency(127) >= 1.0
           && e.getFrequency(127) <= 100000.0 && e.getFrequency(0) >= 1.0);
    }
    {
        TuningEngine e;
        e.setCustomIntervals(twelveTet(), "t");
        loadKbm(e, kbm(3, 60, 60, 261.63, 12, "0\n12\n14\n"));
        check("IN-09 degree == scale size is the next period's unison, not the period entry",
              near(e.getFrequency(61), 523.26));
        check("IN-09 degree past the scale size wraps instead of clamping",
              near(e.getFrequency(62), 261.63 * std::pow(2.0, 1400.0 / 1200.0)));
    }
}

static void testEngineMisc()
{
    {
        TuningEngine e;
        const std::vector<double> eleven { 0, 109, 218, 327, 436, 545, 655, 764, 873, 982, 1091, 1200 };
        e.setCustomIntervals(eleven, "11");
        e.setSingleInterval(3, 333.0);
        const auto iv = e.getIntervals();
        check("WR-14 editing a 12-entry (11-degree) scale keeps it",
              iv.size() == 12 && near(iv[3], 333.0) && near(iv[4], 436.0));
    }
    {
        // CR-05: rebuild locks once. A second lock in the path would hang here,
        // and readers must get through while setters rebuild.
        TuningEngine e;
        e.setCustomIntervals(twelveTet(), "t");
        loadKbm(e, kbm(12, 60, 60, 261.63, 12, identityMap(12)));
        std::atomic<bool> stop { false };
        std::thread reader([&] { while (! stop.load()) { (void) e.getIntervals(); (void) e.isNoteMapped(60); } });
        for (int i = 0; i < 400; ++i)
        {
            e.setMasterTune(430.0 + (i % 20));
            e.setOctaveStretch(1.0f + 0.002f * static_cast<float>(i % 5));
            e.setSingleInterval(1, 100.0 + (i % 3));
        }
        stop.store(true);
        reader.join();
        check("CR-05 rebuild under one lock: no deadlock against readers", true);
    }
    {
        TuningEngine e;
        bool same = true;
        for (int n = 0; n < 128; ++n)
            same = same && near(e.getFrequency(n), 440.0 * std::pow(2.0, (n - 69) / 12.0), 1e-9);
        check("default 12-TET table unchanged", same);
    }
}

static void testGenerator(bool skipHanging)
{
    // Without CR-09 these calls never return, so the negative control skips them.
    if (! skipHanging)
    {
        const auto noHang = ScaleGenerator::generateRank2(1e13, 1e13, 7);
        check("CR-09 huge generator and period stay bounded",
              noHang.size() >= 2 && near(noHang.back(), 2400.0));

        const auto nonFinite = ScaleGenerator::generateRank2(std::nan(""), std::numeric_limits<double>::infinity(), 7);
        bool finite = ! nonFinite.empty();
        for (auto c : nonFinite) finite = finite && std::isfinite(c);
        check("CR-09 non-finite inputs fall back to finite defaults",
              finite && near(nonFinite.back(), 1200.0));
    }

    const auto narrow = ScaleGenerator::generateRank2(700.0, 50.0, 5);
    bool inRange = true;
    for (size_t i = 0; i + 1 < narrow.size(); ++i)
        inRange = inRange && narrow[i] >= 0.0 && narrow[i] < 100.0;
    // Period 50 corrects to 100, so the generator clamps to 99. Clamped against
    // the raw period it was 49, and degree 99 never appeared.
    check("CR-09 generator clamps against the corrected period",
          near(narrow.back(), 100.0) && inRange
              && std::find_if(narrow.begin(), narrow.end(), [](double c) { return near(c, 99.0); }) != narrow.end());

    const auto edo = ScaleGenerator::generateEDO(12);
    check("generateEDO(12) unchanged", edo.size() == 13 && near(edo[7], 700.0) && near(edo[12], 1200.0));
}

static void testEmbedded()
{
    const auto* p = EmbeddedTunings::getTuningById("just/partch43");
    bool sorted = p != nullptr;
    if (p != nullptr)
        for (size_t i = 1; i < p->intervals.size(); ++i)
            sorted = sorted && p->intervals[i] > p->intervals[i - 1];
    check("IN-17 Partch 43 has 43 ascending degrees", p != nullptr && p->intervals.size() == 43 && sorted);
    const auto has = [p](double c) { return p != nullptr && std::find(p->intervals.begin(), p->intervals.end(), c) != p->intervals.end(); };
    check("IN-17 11/10 and 20/11 present", has(165.0) && has(1035.0));
}

static void testExporter()
{
    check("IN-07 ET deviation on a degenerate scale is 0, not inf",
          TuningExporter::calculateETDeviation(100.0, 1, 0, 1200.0) == 0.0);

    auto lineCount = [](const std::vector<double>& iv, double period)
    {
        const auto svg = TuningExporter::generatePitchCircleSVG(iv, period, "s");
        int n = 0;
        for (int at = svg.indexOf("<line"); at >= 0; at = svg.indexOf(at + 1, "<line")) ++n;
        return n;
    };
    auto noPeriod = twelveTet();
    noPeriod.pop_back();
    check("IN-11 closing period adds no duplicate node or spoke",
          lineCount(twelveTet(), 1200.0) == lineCount(noPeriod, 1200.0));

    TuningEngine e;
    e.setCustomIntervals({ 0, 78.0, 156.0, 234.0, 312.0, 390.0, 468.0, 546.0, 624.0, 702.0 }, "sub");
    const auto html = TuningExporter::toHTML(e);
    check("WR-15 sub-octave period exports as itself", html.contains("702.0 cents"));
    check("WR-15 equal sub-octave steps show no ET deviation",
          ! html.contains("deviation sharp") && ! html.contains("deviation flat"));

    TuningEngine x;
    loadScl(x, "<script>alert(1)</script>\n1\n1200.0\n");
    const auto escaped = TuningExporter::toHTML(x);
    check("WR-20 .scl description is escaped in the export",
          ! escaped.contains("<script>alert") && escaped.contains("&lt;script&gt;"));
}

int main(int argc, char**)
{
    const bool skipHanging = argc > 1; // any argument: negative control on an old tree
    testScalaParser();
    testKbm(skipHanging);
    testEngineMisc();
    testGenerator(skipHanging);
    testEmbedded();
    testExporter();
    std::printf("\n%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
