#!/usr/bin/env python3
"""Writes a scratch copy of the render harness main.cpp with an env-switched
G9 dump (OPEQ_G9_DUMP=<dir>): every XR-g9 job writes its packets, header, a
grid (192 log points + each active band's frequency) and the C++ unit-circle
closed form (composeFromPackets, LL / RR / MM / SS) as JSON, then exits.
The repo file is read only. usage: make_dump_main.py <out.cpp>"""
import sys
SRC = "/Users/taylorbrook/Dev/VST-development/plugins/O-ParametricEQ/tests/render-harness/main.cpp"
t = open(SRC).read()

def sub(old, new, count=1):
    global t
    assert t.count(old) == count, (old, t.count(old))
    t = t.replace(old, new)

sub("""        double jsonDb = 0.0;
        double radius = 0.0;
""", """        double jsonDb = 0.0;
        double radius = 0.0;
        std::string dump;   // G9B BENCH
""")

sub("""        // XR-g9j: the JSON round trip, composed again on the unit circle,
        // against the unserialised packets, within 80 dB of the largest
        // response maximum (PLAN-2.6 execute, Taylor's call 2026-11-15).
        if (out.jsonOk)""", """        if (std::getenv ("OPEQ_G9_DUMP") != nullptr)   // G9B BENCH
        {
            std::vector<double> grid (hz.begin(), hz.end());
            for (int b = 0; b < Processor::kNumBands; ++b)
                if (bands[(size_t) b].active)
                {
                    const double f = model->getBandInput (b).settings.freqHz;
                    if (f >= 10.0 && f <= 30000.0 && f < 0.5 * rate)
                        grid.push_back (f);
                }
            std::string s = "{\\"rate\\":" + std::to_string (rate) + ",\\"stereo\\":" + (stereo ? "true" : "false");
            s += ",\\"header\\":" + juce::JSON::toString (model->headerVar(), true).toStdString();
            s += ",\\"packets\\":" + juce::JSON::toString (model->allPackets(), true).toStdString();
            s += ",\\"grid\\":[";
            char buf[64];
            for (size_t k = 0; k < grid.size(); ++k) { std::snprintf (buf, sizeof (buf), "%s%.17g", k ? "," : "", grid[k]); s += buf; }
            s += "],\\"cpp\\":[";
            for (size_t k = 0; k < grid.size(); ++k)
            {
                const auto c4 = composeFromPackets (bands, std::polar (1.0, 2.0 * eq::pi * grid[k] / rate));
                s += k ? ",[" : "[";
                for (int q = 0; q < 4; ++q) { std::snprintf (buf, sizeof (buf), "%s%.17g,%.17g", q ? "," : "", c4[(size_t) q].real(), c4[(size_t) q].imag()); s += buf; }
                s += "]";
            }
            s += "]}";
            out.dump = s;
        }

        // XR-g9j: the JSON round trip, composed again on the unit circle,
        // against the unserialised packets, within 80 dB of the largest
        // response maximum (PLAN-2.6 execute, Taylor's call 2026-11-15).
        if (out.jsonOk)""")

sub("""        //======================================================================
        // XR-g9
        //======================================================================
        {""", """        if (const char* dumpDir = std::getenv ("OPEQ_G9_DUMP"))   // G9B BENCH
        {
            int n = 0;
            for (size_t j = 0; j < stereoOut.size(); ++j)
                juce::File (juce::String (dumpDir) + "/s" + juce::String ((int) j).paddedLeft ('0', 3) + ".json").replaceWithText (stereoOut[j].dump, false, false, "\\n"), ++n;
            for (size_t j = 0; j < monoOut.size(); ++j)
                juce::File (juce::String (dumpDir) + "/m" + juce::String ((int) j).paddedLeft ('0', 3) + ".json").replaceWithText (monoOut[j].dump, false, false, "\\n"), ++n;
            std::printf ("G9B dump: %d files\\n", n);
            std::fflush (stdout);
            std::_Exit (0);
        }

        //======================================================================
        // XR-g9
        //======================================================================
        {""")

# poll / payload cost bench (OPEQ_POLL_BENCH=1)
sub("""        //======================================================================
        // XR-dm: polling
        //======================================================================
        {""", """        if (std::getenv ("OPEQ_POLL_BENCH") != nullptr)   // G9B BENCH
        {
            static const char* fields[8] = { "enabled", "bypass", "type", "freq", "gain", "q", "slope", "placement" };
            for (const double rate : { 48000.0, 192000.0 })
            {
                auto p = make();
                setLayout (*p, 2);
                p->prepareToPlay (rate, 512);
                auto model = std::make_unique<DisplayModel> (*p);
                model->poll();
                Xorshift draw; draw.state = 0x504F4C4Cull;
                std::vector<double> full, one, idle, bytes;
                const auto payload = [&model] (const juce::Array<int>& changed)
                {
                    auto* o = new juce::DynamicObject();
                    o->setProperty ("sampleRate", model->getSampleRate());
                    juce::Array<juce::var> list;
                    for (const int b : changed) { auto pk = model->packetFor (b); pk.getDynamicObject()->setProperty ("band", b + 1); list.add (pk); }
                    o->setProperty ("bands", list);
                    return juce::JSON::toString (juce::var (o), true);
                };
                for (int it = 0; it < 400; ++it)
                {
                    G9Config c = g9Draw (draw);
                    for (int b = 0; b < Processor::kNumBands; ++b)
                    {
                        const auto& r = c.raw[(size_t) b];
                        const float v[8] = { 1.0f, 0.0f, r.type, r.freq, r.gain, r.q, r.slope, r.placement };
                        for (int f = 0; f < 8; ++f) setRaw (*p, bandId (b + 1, fields[f]), v[f]);
                    }
                    auto t0 = juce::Time::getHighResolutionTicks();
                    auto ch = model->poll();
                    auto s = payload (ch);
                    auto t1 = juce::Time::getHighResolutionTicks();
                    full.push_back (juce::Time::highResolutionTicksToSeconds (t1 - t0) * 1e6);
                    bytes.push_back ((double) s.length());
                    for (int k = 0; k < 20; ++k)
                    {
                        setRaw (*p, bandId (1 + (it % 24), "freq"), (float) (20.0 * std::pow (1000.0, draw.uniform())));
                        t0 = juce::Time::getHighResolutionTicks();
                        ch = model->poll();
                        s = payload (ch);
                        t1 = juce::Time::getHighResolutionTicks();
                        one.push_back (juce::Time::highResolutionTicksToSeconds (t1 - t0) * 1e6);
                    }
                    t0 = juce::Time::getHighResolutionTicks();
                    ch = model->poll();
                    t1 = juce::Time::getHighResolutionTicks();
                    idle.push_back (juce::Time::highResolutionTicksToSeconds (t1 - t0) * 1e6);
                }
                const auto stat = [] (std::vector<double> v) { std::sort (v.begin(), v.end()); return juce::String (v[v.size() / 2], 1) + " / " + juce::String (v[(size_t) (0.99 * (double) v.size())], 1) + " / " + juce::String (v.back(), 1); };
                std::printf ("POLL %.0f Hz  all 24 changed (poll + payload + JSON) us med/p99/max: %s   bytes med/p99/max: %s\\n", rate, stat (full).toRawUTF8(), stat (bytes).toRawUTF8());
                std::printf ("POLL %.0f Hz  one band freq (drag tick) us med/p99/max: %s\\n", rate, stat (one).toRawUTF8());
                std::printf ("POLL %.0f Hz  no change us med/p99/max: %s\\n", rate, stat (idle).toRawUTF8());
            }
            std::fflush (stdout);
            std::_Exit (0);
        }

        //======================================================================
        // XR-dm: polling
        //======================================================================
        {""")

# run displayRowsXr first when dumping
sub("""    // Stage 2, Phase 2.2 (PLAN-2.2 T7)
    responseRows();""", """    if (std::getenv ("OPEQ_G9_DUMP") != nullptr || std::getenv ("OPEQ_POLL_BENCH") != nullptr) { displayRowsXr(); return 0; }   // G9B BENCH

    // Stage 2, Phase 2.2 (PLAN-2.2 T7)
    responseRows();""")
open(sys.argv[1], "w").write(t)
print("ok")
