// O-ParametricEQ editor bench (PLAN-3.1 T11 / P10).
//
// The research relay bench (research-3.1/relay-bench) re-pointed at the REAL
// editor: Source/PluginEditor.cpp and the real index.html, with the timing
// script injected through the editor's OPEQ_EDITOR_BENCH hooks. The driver
// reaches the editor only through its public surface (the AudioProcessorEditor
// and its WebBrowserComponent child), the processor's public members and the
// page's DOM / JUCE frontend.
//
// The process re-execs once with CFFIXED_USER_HOME (the preset rows write
// files; JUCE resolves home through NSHomeDirectory, which ignores HOME). The
// presets directory is checked to be under it before the first write.

#include <JuceHeader.h>
#include <thread>
#include <crt_externs.h>
#include <mach-o/dyld.h>
#include <unistd.h>
#include <sys/stat.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BenchScript.h"

//==============================================================================
// Fake home: re-exec before JUCE starts (static constructor, runs before main)
__attribute__((constructor)) static void benchReexecWithFakeHome()
{
    if (std::getenv ("OPEQ_BENCH_HOME") != nullptr)
        return;

    const char* td = std::getenv ("TMPDIR");
    std::string home = std::string (td != nullptr ? td : "/tmp/") + "/opeq-editor-bench-home";
    mkdir (home.c_str(), 0755);

    char exe[4096];
    std::uint32_t exeLen = sizeof (exe);
    if (_NSGetExecutablePath (exe, &exeLen) != 0) { std::fprintf (stderr, "FATAL: executable path\n"); _exit (3); }

    setenv ("OPEQ_BENCH_HOME", home.c_str(), 1);
    setenv ("CFFIXED_USER_HOME", home.c_str(), 1);
    setenv ("HOME", home.c_str(), 1);
    std::fflush (stdout);
    execv (exe, *_NSGetArgv());
    std::fprintf (stderr, "FATAL: re-exec failed\n");
    _exit (3);
}

namespace
{
    double nowMs() { return juce::Time::getMillisecondCounterHiRes(); }

    struct Config
    {
        int cycles = 10;
        int reps = 10;
        int idleSec = 10;
        bool tests = true;
        bool sizeTest = true;
        juce::File out;
    };

    juce::var obj() { return juce::var (new juce::DynamicObject()); }

    struct BenchLogger : public juce::Logger
    {
        void logMessage (const juce::String& m) override
        {
            std::fprintf (stderr, "[log] %s\n", m.toRawUTF8());
            const juce::ScopedLock sl (lock);
            lines.add (m);
        }
        juce::CriticalSection lock;
        juce::StringArray lines;
    };

    struct GestureCounter : public juce::AudioProcessorParameter::Listener
    {
        explicit GestureCounter (int n) : begins ((size_t) n), ends ((size_t) n), values ((size_t) n) {}
        void parameterValueChanged (int i, float) override   { if (i >= 0 && (size_t) i < values.size()) ++values[(size_t) i]; }
        void parameterGestureChanged (int i, bool start) override
        {
            if (i < 0 || (size_t) i >= begins.size()) return;
            if (start) ++begins[(size_t) i]; else ++ends[(size_t) i];
        }
        void reset() { for (auto& b : begins) b = 0; for (auto& e : ends) e = 0; for (auto& v : values) v = 0; }
        std::vector<std::atomic<int>> begins, ends, values;
    };
}

class BenchWindow : public juce::DocumentWindow
{
public:
    BenchWindow() : DocumentWindow ("EditorBench", juce::Colours::black, DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setAlwaysOnTop (true);
    }
    void closeButtonPressed() override {}
};

using Editor = OuariconParametricEQAudioProcessorEditor;

class Driver : private juce::Timer
{
public:
    explicit Driver (Config c) : cfg (std::move (c))
    {
        juce::Logger::setCurrentLogger (&logger);

        proc = std::make_unique<OuariconParametricEQAudioProcessor>();
        proc->setRateAndBufferSizeDetails (48000.0, 512);
        proc->prepareToPlay (48000.0, 512);

        for (auto* p : proc->getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
                params.add (r);

        gestures = std::make_unique<GestureCounter> (params.size());

        // Fake-home check BEFORE any preset write (R5)
        const auto home = juce::File (juce::String::fromUTF8 (std::getenv ("OPEQ_BENCH_HOME")));
        const auto presetsDir = proc->presetManager.getPresetsDirectory();
        homeInfo = home.getFullPathName() + " | presets " + presetsDir.getFullPathName();
        if (! presetsDir.isAChildOf (home))
        {
            std::printf ("FATAL: presets directory %s is not under the bench home %s\n",
                         presetsDir.getFullPathName().toRawUTF8(), home.getFullPathName().toRawUTF8());
            std::fflush (stdout);
            std::_Exit (3);
        }
        home.getChildFile ("Library").getChildFile ("O-ParametricEQ").deleteRecursively();

        // Two alternating states: every one of the 196 parameters differs between A and B.
        for (int i = 0; i < params.size(); ++i)
        {
            auto* p = params[i];
            const auto id = p->getParameterID();
            float a, b;
            if (dynamic_cast<juce::AudioParameterBool*> (p) != nullptr)
            {
                a = id.contains ("bypass") ? 0.0f : 1.0f;   // A: everything enabled, nothing bypassed
                b = 1.0f - a;
            }
            else if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (p))
            {
                const int n = ch->choices.size();
                const int ia = (i * 3) % n, ib = (ia + 1) % n;
                a = p->convertTo0to1 ((float) ia);
                b = p->convertTo0to1 ((float) ib);
            }
            else
            {
                const float f = (float) std::fmod (i * 0.6180339887, 1.0);
                a = 0.25f + 0.5f * f;
                b = a + 0.2f;
            }
            stateA.push_back (a);
            stateB.push_back (b);
        }

        // Session blobs (setStateInformation path) and presets (presetManager path)
        applyDirect (stateA); proc->getStateInformation (blobA);
        presetOk = proc->presetManager.savePreset ("bench-A");
        applyDirect (stateB); proc->getStateInformation (blobB);
        presetOk = proc->presetManager.savePreset ("bench-B") && presetOk;
        curIsA = false;

        opeq::bench::hooks().userScript = kBenchScript;
        opeq::bench::hooks().onReport = [this] (const juce::var& v, double t) { onBench (v, t); };

        window = std::make_unique<BenchWindow>();
        window->setTopLeftPosition (80, 80);

        for (int cyc = 0; cyc < cfg.cycles; ++cyc)
            addCycle (cyc);

        if (cfg.sizeTest)
            addSizeTest();

        steps.push_back ([this] { finish(); return true; });
        startTimer (2);
    }

    ~Driver() override
    {
        stopTimer();
        window.reset();
        editor.reset();
        opeq::bench::hooks().onReport = nullptr;
        proc.reset();
        juce::Logger::setCurrentLogger (nullptr);
    }

private:
    using Step = std::function<bool()>;

    // ── helpers ─────────────────────────────────────────────────────────────
    static char kindOf (juce::RangedAudioParameter* p)
    {
        if (dynamic_cast<juce::AudioParameterBool*> (p) != nullptr)   return 't';
        if (dynamic_cast<juce::AudioParameterChoice*> (p) != nullptr) return 'c';
        return 's';
    }

    void applyDirect (const std::vector<float>& s)
    {
        for (int i = 0; i < params.size(); ++i)
            params[i]->setValueNotifyingHost (s[(size_t) i]);
    }

    juce::WebBrowserComponent* webView() const
    {
        if (editor == nullptr) return nullptr;
        for (auto* ch : editor->getChildren())
            if (auto* wv = dynamic_cast<juce::WebBrowserComponent*> (ch))
                return wv;
        return nullptr;
    }

    void js (const juce::String& code)
    {
        if (auto* wv = webView())
            wv->evaluateJavascript (code, nullptr);
    }

    static juce::String jsArgs (std::initializer_list<juce::var> args)
    {
        juce::StringArray parts;
        for (auto& a : args) parts.add (juce::JSON::toString (a, true));
        return parts.joinIntoString (",");
    }

    void bench (const juce::String& fn, std::initializer_list<juce::var> args)
    {
        js ("window.__bench." + fn + "(" + jsArgs (args) + ");");
    }

    /** The value the page should end up holding for parameter i at normalised t. */
    juce::var targetEntry (int i, float t)
    {
        auto* p = params[i];
        auto o = obj();
        auto* d = o.getDynamicObject();
        const char k = kindOf (p);
        d->setProperty ("id", p->getParameterID());
        d->setProperty ("k", juce::String::charToString (k));
        if (k == 's')      d->setProperty ("v", (double) p->convertFrom0to1 (t));
        else if (k == 't') d->setProperty ("v", t >= 0.5f);
        else               d->setProperty ("v", juce::roundToInt (p->convertFrom0to1 (t)));
        return o;
    }

    void onBench (const juce::var& v, double t)
    {
        const auto type = v["type"].toString();
        if (v.hasProperty ("id"))
            keyed[type + ":" + v["id"].toString()] = { t, v };
        else
            events[type] = { t, v };
    }

    Step waitFor (juce::String key, double timeoutMs, bool keyedEvent = false)
    {
        auto start = std::make_shared<double> (-1.0);
        return [this, key, timeoutMs, start, keyedEvent]
        {
            if (*start < 0) *start = nowMs();
            const bool seen = keyedEvent ? keyed.count (key) > 0 : events.count (key) > 0;
            if (seen) return true;
            if (nowMs() - *start > timeoutMs)
            {
                timeouts.add (key + " (cycle " + juce::String (cycleIndex) + ")");
                return true;
            }
            return false;
        };
    }

    Step sleep (double ms)
    {
        auto start = std::make_shared<double> (-1.0);
        return [start, ms] { if (*start < 0) *start = nowMs(); return nowMs() - *start >= ms; };
    }

    void setRow (const juce::String& row, const juce::var& v) { rows->setProperty (row, v); }
    juce::var getRow (const juce::String& row)
    {
        if (! rows->hasProperty (row)) rows->setProperty (row, obj());
        return rows->getProperty (row);
    }

    // Native call from the page; then `check` runs on the message thread with the report
    void addNative (const juce::String& tag, const juce::String& name, juce::Array<juce::var> args,
                    std::function<void (const juce::var&)> check)
    {
        steps.push_back ([this, tag, name, args]
        {
            keyed.erase ("native:" + tag);
            bench ("callNative", { tag, name, juce::var (args) });
            return true;
        });
        steps.push_back (waitFor ("native:" + tag, 3000.0, true));
        steps.push_back ([this, tag, check]
        {
            const auto key = "native:" + tag;
            check (keyed.count (key) ? keyed[key].second : juce::var());
            return true;
        });
    }

    void openEditor()
    {
        events.clear();
        keyed.clear();
        tOpen = nowMs();
        editor = std::make_unique<Editor> (*proc);
        tCtor = nowMs();
        openW = editor->getWidth(); openH = editor->getHeight();
        window->setContentNonOwned (editor.get(), true);
        window->setVisible (true);
        window->toFront (true);
        tShown = nowMs();
    }

    void closeEditor()
    {
        window->clearContentComponent();
        editor.reset();
    }

    // ── one open / test / close cycle ───────────────────────────────────────
    void addCycle (int c)
    {
        steps.push_back ([this, c]
        {
            cycleIndex = c;
            openEditor();
            return true;
        });
        steps.push_back (waitFor ("secondFrame", 15000.0));
        steps.push_back (waitFor ("pageReady", 3000.0));
        steps.push_back ([this]
        {
            auto r = obj();
            auto* o = r.getDynamicObject();
            o->setProperty ("cycle", cycleIndex);
            o->setProperty ("ctorMs", tCtor - tOpen);
            o->setProperty ("shownMs", tShown - tOpen);
            o->setProperty ("openWidth", openW);
            o->setProperty ("openHeight", openH);
            for (auto k : { "scriptStart", "dcl", "load", "allProps", "firstPaint", "secondFrame", "pageReady" })
                o->setProperty (juce::String (k) + "Ms", events.count (k) ? events[k].first - tOpen : -1.0);
            if (events.count ("allProps"))    o->setProperty ("allProps", events["allProps"].second);
            if (events.count ("dcl"))         o->setProperty ("dcl", events["dcl"].second);
            if (events.count ("secondFrame")) o->setProperty ("console", events["secondFrame"].second);
            if (events.count ("pageReady"))   o->setProperty ("pageReadyPnow", events["pageReady"].second["pnow"]);
            if (cycleIndex == 0) { firstOpenW = openW; firstOpenH = openH; }
            currentCycle = r;
            return true;
        });

        if (c == 0 && cfg.tests)
            addTests();

        steps.push_back ([this]
        {
            auto* o = currentCycle.getDynamicObject();
            const bool listenWasOn = proc->listenBand.load() >= 0;
            const double t0 = nowMs();
            closeEditor();
            o->setProperty ("closeMs", nowMs() - t0);
            o->setProperty ("cycleMs", nowMs() - tOpen);
            if (listenWasOn)
            {
                auto row = getRow ("E-listen");
                row.getDynamicObject()->setProperty ("listenBeforeClose", true);
                row.getDynamicObject()->setProperty ("listenAfterClose", proc->listenBand.load());
            }
            cycles.add (currentCycle);
            return true;
        });
        steps.push_back (sleep (400.0));
    }

    void addTests()
    {
        steps.push_back (sleep (300.0));

        // ── E-res (resources, MIME, fonts) ──────────────────────────────────
        steps.push_back ([this]
        {
            juce::Array<juce::var> expected;
            for (auto u : { "/", "/index.html", "/js/i18n.js", "/js/juce/index.js", "/js/juce/check_native_interop.js",
                            "/modules/preset-manager.js", "/modules/vu-meter.js", "/css/eb-garamond.css",
                            "/fonts/EBGaramond-Regular.woff2", "/fonts/EBGaramond-Italic.woff2", "/fonts/EBGaramond-Bold.woff2" })
                expected.add (juce::String (u));
            // The page's own requests are all done by now (boot + 300 ms): any
            // miss so far is the page's, not the bench fetches below.
            juce::StringArray notFound;
            { const juce::ScopedLock sl (logger.lock);
              for (auto& l : logger.lines) if (l.startsWith ("Resource not found")) notFound.add (l); }
            getRow ("E-res").getDynamicObject()->setProperty ("resourceNotFoundBeforeBenchFetch", juce::var (notFound));
            bench ("res", { "r0", juce::var (expected) });
            return true;
        });
        steps.push_back (waitFor ("res:r0", 8000.0, true));
        steps.push_back ([this] { bench ("fonts", { "f0" }); return true; });
        steps.push_back (waitFor ("fonts:f0", 8000.0, true));
        steps.push_back ([this]
        {
            auto row = getRow ("E-res");
            auto* d = row.getDynamicObject();
            if (keyed.count ("res:r0"))   d->setProperty ("fetch", keyed["res:r0"].second);
            if (keyed.count ("fonts:f0")) d->setProperty ("fonts", keyed["fonts:f0"].second);
            juce::StringArray notFound;
            { const juce::ScopedLock sl (logger.lock);
              for (auto& l : logger.lines) if (l.startsWith ("Resource not found")) notFound.add (l); }
            d->setProperty ("resourceNotFoundLogAll", juce::var (notFound));
            return true;
        });

        // ── frame period ────────────────────────────────────────────────────
        steps.push_back ([this] { bench ("raf", {}); return true; });
        steps.push_back (waitFor ("raf", 5000.0));
        steps.push_back ([this] { rafInfo = events.count ("raf") ? events["raf"].second : juce::var(); return true; });

        // ── E-idle: 10 s, nothing but the (empty) 30 Hz timer running ──────
        steps.push_back ([this]
        {
            bench ("disarm", {});
            events.erase ("idle");
            bench ("idleStart", {});
            return true;
        });
        steps.push_back (sleep (cfg.idleSec * 1000.0));
        steps.push_back ([this] { bench ("idleEnd", {}); return true; });
        steps.push_back (waitFor ("idle", 3000.0));
        steps.push_back ([this] { if (events.count ("idle")) setRow ("E-idle", events["idle"].second); return true; });

        // ── E-preset (a) - (d) + one automated parameter ────────────────────
        steps.push_back (sleep (300.0));
        const char* methods[] = { "single", "msgLoop", "setState", "offThread", "preset" };
        for (int rep = 0; rep < cfg.reps; ++rep)
            for (auto m : methods)
                addPresetTest (m, rep);
        steps.push_back ([this] { bench ("disarm", {}); return true; });

        addBindIn();
        addBindOut();
        addTypeAndDisabled();
        addUiTests();
        addListenTests();

        steps.push_back ([this] { bench ("console", {}); return true; });
        steps.push_back (waitFor ("console", 3000.0));
        steps.push_back ([this] { if (events.count ("console")) setRow ("consoleAfterTests", events["console"].second); return true; });
    }

    void addPresetTest (juce::String method, int rep)
    {
        const int id = nextTestId++;
        auto rec = std::make_shared<juce::var> (obj());

        steps.push_back ([this, method, id, rec, rep]
        {
            std::vector<float> target;
            for (auto* p : params) target.push_back (p->getValue());
            juce::String presetName;

            if (method == "single")
            {
                int idx = 0;
                for (int i = 0; i < params.size(); ++i)
                    if (params[i]->getParameterID() == "b05_freq") idx = i;
                target[(size_t) idx] = target[(size_t) idx] > 0.5f ? 0.3f : 0.7f;
            }
            else if (method == "preset")
            {
                const auto& s = curIsA ? stateB : stateA;
                presetName = curIsA ? "bench-B" : "bench-A";
                for (int i = 0; i < params.size(); ++i)
                {
                    const auto pid = params[i]->getParameterID();
                    if (pid != "output_gain" && pid != "bypass")   // excluded from presets (C2)
                        target[(size_t) i] = s[(size_t) i];
                }
            }
            else
                target = curIsA ? stateB : stateA;

            juce::Array<juce::var> targets;
            for (int i = 0; i < params.size(); ++i)
                if (std::abs (params[i]->getValue() - target[(size_t) i]) > 1.0e-6f)
                    targets.add (targetEntry (i, target[(size_t) i]));

            keyed.erase ("settled:" + juce::String (id));
            keyed.erase ("settledPaint:" + juce::String (id));
            auto arm = obj();
            arm.getDynamicObject()->setProperty ("id", id);
            arm.getDynamicObject()->setProperty ("targets", targets);
            bench ("arm", { arm });

            bool ok = true;
            const double t0 = nowMs();
            if (method == "single" || method == "msgLoop")
            {
                for (int i = 0; i < params.size(); ++i)
                    if (std::abs (params[i]->getValue() - target[(size_t) i]) > 1.0e-6f)
                        params[i]->setValueNotifyingHost (target[(size_t) i]);
            }
            else if (method == "setState")
            {
                auto& blob = curIsA ? blobB : blobA;
                proc->setStateInformation (blob.getData(), (int) blob.getSize());
            }
            else if (method == "offThread")
            {
                std::thread th ([this, target]
                {
                    for (int i = 0; i < params.size(); ++i)
                        if (std::abs (params[i]->getValue() - target[(size_t) i]) > 1.0e-6f)
                            params[i]->setValueNotifyingHost (target[(size_t) i]);
                });
                th.join();
            }
            else
                ok = proc->presetManager.loadPreset (presetName);
            const double t1 = nowMs();
            if (method != "single") curIsA = ! curIsA;

            // Host-side check: every parameter at its target (offThread: already set)
            int hostMismatch = 0;
            for (int i = 0; i < params.size(); ++i)
                if (std::abs (params[i]->getValue() - target[(size_t) i]) > 1.0e-5f) ++hostMismatch;

            auto* o = rec->getDynamicObject();
            o->setProperty ("method", method);
            o->setProperty ("rep", rep);
            o->setProperty ("id", id);
            o->setProperty ("expected", targets.size());
            o->setProperty ("applyMs", t1 - t0);
            o->setProperty ("applyOk", ok);
            o->setProperty ("hostMismatch", hostMismatch);
            o->setProperty ("t0", t0);
            return true;
        });
        steps.push_back (waitFor ("settledPaint:" + juce::String (id), 5000.0, true));
        steps.push_back ([this, id, rec]
        {
            auto* o = rec->getDynamicObject();
            const double t0 = o->getProperty ("t0");
            for (auto k : { "settled", "settledPaint" })
            {
                const auto key = juce::String (k) + ":" + juce::String (id);
                if (keyed.count (key))
                {
                    o->setProperty (juce::String (k) + "RecvMs", keyed[key].first - t0);
                    o->setProperty (k, keyed[key].second);
                }
            }
            presetResults.add (*rec);
            return true;
        });
        steps.push_back (sleep (150.0));
    }

    // ── E-bind-in: host -> page, all 196 ────────────────────────────────────
    void addBindIn()
    {
        steps.push_back ([this]
        {
            juce::Random rng (3101);
            for (auto* p : params)
            {
                const char k = kindOf (p);
                float v;
                if (k == 't') v = rng.nextBool() ? 1.0f : 0.0f;
                else if (k == 'c')
                {
                    auto* ch = dynamic_cast<juce::AudioParameterChoice*> (p);
                    v = p->convertTo0to1 ((float) rng.nextInt (ch->choices.size()));
                }
                else v = rng.nextFloat();
                p->setValueNotifyingHost (v);
            }

            juce::Array<juce::var> list;
            for (auto* p : params)
            {
                const char k = kindOf (p);
                auto o = obj();
                auto* d = o.getDynamicObject();
                d->setProperty ("id", p->getParameterID());
                d->setProperty ("k", juce::String::charToString (k));
                const float n = p->getValue();
                d->setProperty ("norm", (double) n);
                if (k == 's')
                {
                    d->setProperty ("scaled", (double) p->convertFrom0to1 (n));
                    d->setProperty ("textNum", p->getText (n, 64).getDoubleValue());
                    d->setProperty ("text", p->getText (n, 64));
                }
                else if (k == 't') d->setProperty ("v", n >= 0.5f);
                else               d->setProperty ("v", juce::roundToInt (p->convertFrom0to1 (n)));
                list.add (o);
            }
            bench ("bindIn", { "bi", juce::var (list) });
            return true;
        });
        steps.push_back (waitFor ("bindIn:bi", 5000.0, true));
        steps.push_back ([this]
        {
            if (keyed.count ("bindIn:bi")) setRow ("E-bind-in", keyed["bindIn:bi"].second);
            return true;
        });
    }

    // ── E-bind-out: page -> host, all 196, one gesture pair each ────────────
    /** Bind-out value check. Toggles / combos: exact. Sliders: compared in the
        scaled domain the page writes (WebSliderRelay carries scaled values; the
        page computes start + span * v^(1/skew) in double and the host stores
        that as a float AudioParameterFloat value). Tolerance: 4 float ulps of the
        value + 1e-6 x (value - start) / skew (the float normalised hop inside
        ParameterAttachment::setValueAsPartOfGesture). */
    struct OutErr { bool ok; double normErr, scaledErr, tol; };
    OutErr bindOutErr (int i, float target)
    {
        auto* p = params[i];
        const double normErr = std::abs ((double) p->getValue() - (double) target);
        if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (p))
        {
            const auto& r = f->range;
            const double page = r.start + (r.end - r.start) * std::pow ((double) target, 1.0 / (double) r.skew);
            const float pf = (float) page;
            const double ulp = (double) (std::nextafter (std::abs (pf), INFINITY) - std::abs (pf));
            const double tol = 4.0 * ulp + 1.0e-6 * std::abs (page - r.start) / (double) r.skew;
            const double err = std::abs ((double) f->get() - page);
            return { err <= tol, normErr, err, tol };
        }
        return { normErr < 1.0e-6, normErr, 0.0, 0.0 };
    }

    void addBindOut()
    {
        auto expect = std::make_shared<std::vector<float>>();
        auto tStart = std::make_shared<double> (0.0);
        steps.push_back ([this, expect, tStart]
        {
            juce::Random rng (3102);
            juce::Array<juce::var> list;
            expect->clear();
            for (auto* p : params)
            {
                const char k = kindOf (p);
                const float cur = p->getValue();
                auto o = obj();
                auto* d = o.getDynamicObject();
                d->setProperty ("id", p->getParameterID());
                d->setProperty ("k", juce::String::charToString (k));
                float target;
                if (k == 't') { target = cur >= 0.5f ? 0.0f : 1.0f; d->setProperty ("v", target >= 0.5f); }
                else if (k == 'c')
                {
                    auto* ch = dynamic_cast<juce::AudioParameterChoice*> (p);
                    const int n = ch->choices.size(), ci = ch->getIndex();
                    const int ni = (ci + 1 + rng.nextInt (n - 1)) % n;
                    target = p->convertTo0to1 ((float) ni);
                    d->setProperty ("v", ni);
                }
                else
                {
                    do { target = rng.nextFloat(); } while (std::abs (target - cur) < 0.05f);
                    d->setProperty ("v", (double) target);
                }
                expect->push_back (target);
                list.add (o);
            }
            gestures->reset();
            for (auto* p : params) p->addListener (gestures.get());
            *tStart = nowMs();
            bench ("bindOut", { "bo", juce::var (list) });
            return true;
        });
        steps.push_back (waitFor ("bindOut:bo", 5000.0, true));
        // Wait until every host value matches (or 3 s), then 200 ms for trailing gesture ends
        steps.push_back ([this, expect, tStart]
        {
            int bad = 0;
            for (int i = 0; i < params.size(); ++i)
                if (! bindOutErr (i, (*expect)[(size_t) i]).ok) ++bad;
            if (bad == 0) { bindOutSettleMs = nowMs() - *tStart; return true; }
            if (nowMs() - *tStart > 3000.0) { bindOutSettleMs = -1.0; return true; }
            return false;
        });
        steps.push_back (sleep (200.0));
        steps.push_back ([this, expect]
        {
            for (auto* p : params) p->removeListener (gestures.get());
            auto row = obj();
            auto* d = row.getDynamicObject();
            int bad = 0, gestureBad = 0, nb = 0, ne = 0, normOver1e6 = 0;
            double maxErr = 0, maxRatio = 0;
            juce::Array<juce::var> badList, normList;
            for (int i = 0; i < params.size(); ++i)
            {
                const auto r = bindOutErr (i, (*expect)[(size_t) i]);
                maxErr = juce::jmax (maxErr, r.normErr);
                if (r.tol > 0) maxRatio = juce::jmax (maxRatio, r.scaledErr / r.tol);
                if (r.normErr > 1.0e-6)
                {
                    ++normOver1e6;
                    normList.add (params[i]->getParameterID() + " target=" + juce::String ((*expect)[(size_t) i], 6)
                                  + " normErr=" + juce::String (r.normErr, 9) + " scaledErr=" + juce::String (r.scaledErr, 12)
                                  + " tol=" + juce::String (r.tol, 12));
                }
                const int b = gestures->begins[(size_t) i], e = gestures->ends[(size_t) i];
                nb += b; ne += e;
                const bool valBad = ! r.ok, gBad = (b != 1 || e != 1);
                if (valBad) ++bad;
                if (gBad) ++gestureBad;
                if ((valBad || gBad) && badList.size() < 20)
                    badList.add (params[i]->getParameterID() + " normErr=" + juce::String (r.normErr, 9)
                                 + " scaledErr=" + juce::String (r.scaledErr, 12) + " tol=" + juce::String (r.tol, 12)
                                 + " begin=" + juce::String (b) + " end=" + juce::String (e));
            }
            d->setProperty ("normErrOver1e6", normOver1e6);
            d->setProperty ("normErrOver1e6List", normList);
            d->setProperty ("maxScaledErrOverTol", maxRatio);
            d->setProperty ("n", params.size());
            d->setProperty ("valueBad", bad);
            d->setProperty ("gestureBad", gestureBad);
            d->setProperty ("beginTotal", nb);
            d->setProperty ("endTotal", ne);
            d->setProperty ("maxNormErr", maxErr);
            d->setProperty ("settleMs", bindOutSettleMs);
            d->setProperty ("bad", badList);
            setRow ("E-bind-out", row);
            return true;
        });
    }

    juce::RangedAudioParameter* param (const juce::String& id)
    {
        for (auto* p : params) if (p->getParameterID() == id) return p;
        return nullptr;
    }

    void setIndex (const juce::String& id, int index)
    {
        auto* p = param (id);
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
    }

    // ── E-type and E-disabled: band 1 enabled and selected in the page ──────
    void addTypeAndDisabled()
    {
        steps.push_back ([this]
        {
            param ("b01_enabled")->setValueNotifyingHost (1.0f);
            setIndex ("b01_type", 0);   // Bell
            setIndex ("b01_slope", 1);
            bench ("clickChip", { "c1", 1 });
            return true;
        });
        steps.push_back (waitFor ("chip:c1", 3000.0, true));
        steps.push_back ([this]
        {
            if (keyed.count ("chip:c1")) getRow ("E-disabled").getDynamicObject()->setProperty ("chip", keyed["chip:c1"].second);
            getRow ("E-disabled").getDynamicObject()->setProperty ("selectedBandAtomic", proc->uiSelectedBand.load());
            return true;
        });

        // E-type: typed entry on the selected band's frequency field.
        //   1000     -> 0.5 (checklist section 6)
        //   42.7     -> the exact skew mapping ((42.7 - 10) / 29990)^skew = 0.2500297 (the
        //               checklist's "42.7 -> 0.25" is 0.25 to three figures; 0.25 is 42.6809 Hz)
        //   42.68090 -> 0.25 (the 0.25 point, computed from the live range, typed to 5 decimals)
        int idx = 0;
        {
            auto* fp = dynamic_cast<juce::AudioParameterFloat*> (param ("b01_freq"));
            const auto& rg = fp->range;
            const double skew = rg.skew, span = rg.end - rg.start;
            const double hzAtQuarter = rg.start + span * std::pow (0.25, 1.0 / skew);
            struct T { juce::String text; double expect; double planExpect; };
            const T ts[] = { { "1000", std::pow ((1000.0 - rg.start) / span, skew), 0.5 },
                             { "42.7", std::pow ((42.7 - rg.start) / span, skew), 0.25 },
                             { juce::String (hzAtQuarter, 5), 0.25, 0.25 } };
            for (auto tt : ts)
            {
                const juce::String text = tt.text;
                const double expectNorm = tt.expect, planNorm = tt.planExpect;
                const juce::String tag = "t" + juce::String (idx++);
                steps.push_back ([this, text, tag]
                {
                    // Move b01_freq away first so the write is a change
                    param ("b01_freq")->setValueNotifyingHost (0.8f);
                    gestures->reset();
                    param ("b01_freq")->addListener (gestures.get());
                    bench ("typeEntry", { tag, text });
                    return true;
                });
                steps.push_back (waitFor ("typeEntry:" + tag, 3000.0, true));
                steps.push_back (sleep (200.0));
                steps.push_back ([this, text, tag, expectNorm, planNorm]
                {
                    auto* p = param ("b01_freq");
                    p->removeListener (gestures.get());
                    const int pi = p->getParameterIndex();
                    auto r = obj();
                    auto* d = r.getDynamicObject();
                    d->setProperty ("typed", text);
                    d->setProperty ("hostNorm", (double) p->getValue());
                    d->setProperty ("expectNorm", expectNorm);
                    d->setProperty ("normErr", std::abs ((double) p->getValue() - expectNorm));
                    d->setProperty ("planExpectNorm", planNorm);
                    d->setProperty ("normErrVsPlan", std::abs ((double) p->getValue() - planNorm));
                    d->setProperty ("hostHz", (double) p->convertFrom0to1 (p->getValue()));
                    d->setProperty ("hostText", p->getText (p->getValue(), 64));
                    d->setProperty ("gestureBegin", (int) gestures->begins[(size_t) pi]);
                    d->setProperty ("gestureEnd", (int) gestures->ends[(size_t) pi]);
                    if (keyed.count ("typeEntry:" + tag)) d->setProperty ("page", keyed["typeEntry:" + tag].second);
                    getRow ("E-type").getDynamicObject()->setProperty (text, r);
                    return true;
                });
            }
        }

        // E-disabled: 8 types x 8 slopes
        for (int t = 0; t < 8; ++t)
            for (int s = 0; s < 8; ++s)
            {
                const auto tag = "d" + juce::String (t) + "_" + juce::String (s);
                steps.push_back ([this, t, s, tag]
                {
                    setIndex ("b01_type", t);
                    setIndex ("b01_slope", s);
                    bench ("probeDisabled", { tag });
                    return true;
                });
                steps.push_back (waitFor ("disabled:" + tag, 3000.0, true));
                steps.push_back ([this, t, s, tag]
                {
                    const bool slopeTypes = (t == 1 || t == 2 || t == 3 || t == 4 || t == 7);
                    const bool expGainOff  = (t == 3 || t == 4 || t == 5 || t == 6);   // Low Cut, High Cut, Notch, Band Pass
                    const bool expSlopeOff = (t == 0 || t == 5 || t == 6);             // Bell, Notch, Band Pass
                    const bool expQOff     = slopeTypes && s == 0;                     // 6 dB/oct on slope types
                    auto r = obj();
                    auto* d = r.getDynamicObject();
                    bool pass = false;
                    if (keyed.count ("disabled:" + tag))
                    {
                        const auto v = keyed["disabled:" + tag].second;
                        pass = (bool) v["gainOff"] == expGainOff
                            && (bool) v["slopeDisabled"] == expSlopeOff
                            && (bool) v["slopeWrapOff"] == expSlopeOff
                            && (bool) v["qOff"] == expQOff
                            && v["gainAria"].toString() == (expGainOff ? "true" : "false")
                            && v["qAria"].toString() == (expQOff ? "true" : "false")
                            && ! (bool) v["freqOff"] && ! (bool) v["stripEmpty"]
                            && (int) v["typeIndex"] == t && (int) v["slopeIndex"] == s
                            && v["loz"].toString() == "01";
                        d->setProperty ("dom", v);
                    }
                    d->setProperty ("pass", pass);
                    auto row = getRow ("E-disabled");
                    row.getDynamicObject()->setProperty (tag, r);
                    disabledPass += pass ? 1 : 0;
                    row.getDynamicObject()->setProperty ("passCount", disabledPass);
                    return true;
                });
            }
        steps.push_back ([this] { setIndex ("b01_type", 0); setIndex ("b01_slope", 1); return true; });
    }

    // ── E-ui ─────────────────────────────────────────────────────────────────
    juce::var uiAtomics()
    {
        auto o = obj();
        auto* d = o.getDynamicObject();
        d->setProperty ("analyzerMode", proc->analyzerMode.load());
        d->setProperty ("displayRange", proc->uiDisplayRange.load());
        d->setProperty ("selectedBand", proc->uiSelectedBand.load());
        d->setProperty ("uiLanguage", proc->uiLanguage.load());
        d->setProperty ("editorWidth", proc->uiEditorWidth.load());
        d->setProperty ("editorHeight", proc->uiEditorHeight.load());
        return o;
    }

    void uiRecord (const juce::String& step, const juce::var& report, bool pass)
    {
        auto r = obj();
        r.getDynamicObject()->setProperty ("report", report);
        r.getDynamicObject()->setProperty ("atomics", uiAtomics());
        r.getDynamicObject()->setProperty ("pass", pass);
        getRow ("E-ui").getDynamicObject()->setProperty (step, r);
        uiChecks++; uiPass += pass ? 1 : 0;
        getRow ("E-ui").getDynamicObject()->setProperty ("passCount", juce::String (uiPass) + "/" + juce::String (uiChecks));
    }

    void addUiTests()
    {
        addNative ("ui0", "getUiState", {}, [this] (const juce::var& v)
        {
            const auto r = v["result"];
            uiRecord ("getUiState-initial", v, (int) r["analyzerMode"] == proc->analyzerMode.load()
                      && (int) r["displayRange"] == proc->uiDisplayRange.load()
                      && (int) r["selectedBand"] == proc->uiSelectedBand.load());
        });
        struct Set { const char* key; int value; bool ok; };
        const Set sets[] = { { "analyzerMode", 3, true }, { "displayRange", 30, true }, { "selectedBand", 7, true },
                             { "analyzerMode", 4, false }, { "analyzerMode", -1, false }, { "displayRange", 7, false },
                             { "displayRange", 0, false }, { "selectedBand", 25, false }, { "selectedBand", -1, false },
                             { "foo", 1, false } };
        int n = 0;
        for (auto s : sets)
        {
            const juce::String key = s.key; const int value = s.value; const bool ok = s.ok;
            const auto tag = "ui-set" + juce::String (n++);
            addNative (tag, "setUiState", { key, value }, [this, key, value, ok, tag] (const juce::var& v)
            {
                const bool result = (bool) v["result"];
                // After the valid sets: 3 / 30 / 7, and invalid sets leave them there
                bool pass = (result == ok);
                if (ok && key == "analyzerMode") pass = pass && proc->analyzerMode.load() == value;
                if (ok && key == "displayRange") pass = pass && proc->uiDisplayRange.load() == value;
                if (ok && key == "selectedBand") pass = pass && proc->uiSelectedBand.load() == value;
                if (! ok) pass = pass && proc->analyzerMode.load() == 3 && proc->uiDisplayRange.load() == 30
                                      && proc->uiSelectedBand.load() == 7;
                uiRecord (tag + " " + key + "=" + juce::String (value), v, pass);
            });
        }
        addNative ("ui1", "getUiState", {}, [this] (const juce::var& v)
        {
            const auto r = v["result"];
            uiRecord ("getUiState-after", v, (int) r["analyzerMode"] == 3 && (int) r["displayRange"] == 30
                                             && (int) r["selectedBand"] == 7);
        });
        steps.push_back ([this]
        {
            // Not host parameters
            juce::StringArray hits;
            for (auto* p : proc->getParameters())
            {
                const auto name = p->getName (64);
                const auto pid = dynamic_cast<juce::RangedAudioParameter*> (p) != nullptr
                                     ? dynamic_cast<juce::RangedAudioParameter*> (p)->getParameterID() : juce::String();
                for (auto k : { "analyzer", "display", "selected", "range" })
                    if (pid.containsIgnoreCase (k) || name.containsIgnoreCase (k)) hits.add (pid + "/" + name);
            }
            auto r = obj();
            r.getDynamicObject()->setProperty ("numParameters", proc->getParameters().size());
            r.getDynamicObject()->setProperty ("hits", juce::var (hits));
            uiRecord ("absentFromGetParameters", r, hits.isEmpty() && proc->getParameters().size() == 196);

            // Unchanged by a preset load (d)
            const auto before = juce::JSON::toString (uiAtomics(), true);
            const bool ok = proc->presetManager.loadPreset (curIsA ? "bench-B" : "bench-A");
            curIsA = ! curIsA;
            const auto after = juce::JSON::toString (uiAtomics(), true);
            auto r2 = obj();
            r2.getDynamicObject()->setProperty ("loadOk", ok);
            r2.getDynamicObject()->setProperty ("before", before);
            r2.getDynamicObject()->setProperty ("after", after);
            r2.getDynamicObject()->setProperty ("listenBand", proc->listenBand.load());
            uiRecord ("unchangedByPresetLoad", r2, ok && before == after);
            return true;
        });
        // Restore defaults for the rest of the cycles
        addNative ("ui-r0", "setUiState", { "analyzerMode", 2 }, [] (const juce::var&) {});
        addNative ("ui-r1", "setUiState", { "displayRange", 12 }, [] (const juce::var&) {});
        addNative ("ui-r2", "setUiState", { "selectedBand", 1 }, [] (const juce::var&) {});
    }

    // ── E-listen ─────────────────────────────────────────────────────────────
    void addListenTests()
    {
        struct L { int arg; int expect; };
        const L ls[] = { { 5, 4 }, { 0, -1 }, { 5, 4 }, { 25, -1 }, { 5, 4 }, { -3, -1 }, { 1, 0 }, { 24, 23 }, { 7, 6 } };
        int n = 0;
        for (auto l : ls)
        {
            const int arg = l.arg, expect = l.expect;
            const auto tag = "ls" + juce::String (n++);
            addNative (tag, "setListenBand", { arg }, [this, arg, expect, tag] (const juce::var& v)
            {
                auto r = obj();
                r.getDynamicObject()->setProperty ("arg", arg);
                r.getDynamicObject()->setProperty ("listenBand", proc->listenBand.load());
                r.getDynamicObject()->setProperty ("expect", expect);
                r.getDynamicObject()->setProperty ("report", v);
                r.getDynamicObject()->setProperty ("pass", proc->listenBand.load() == expect);
                getRow ("E-listen").getDynamicObject()->setProperty (tag, r);
            });
        }
        // The last call leaves listen on (6); the close step records listenAfterClose.
    }

    // ── E-size ───────────────────────────────────────────────────────────────
    void sizeRecord (const juce::String& step, int w, int h, int ew, int eh)
    {
        auto r = obj();
        r.getDynamicObject()->setProperty ("editor", juce::String (w) + "x" + juce::String (h));
        r.getDynamicObject()->setProperty ("expect", juce::String (ew) + "x" + juce::String (eh));
        r.getDynamicObject()->setProperty ("atomics", juce::String (proc->uiEditorWidth.load()) + "x" + juce::String (proc->uiEditorHeight.load()));
        r.getDynamicObject()->setProperty ("pass", w == ew && h == eh && proc->uiEditorWidth.load() == ew && proc->uiEditorHeight.load() == eh);
        getRow ("E-size").getDynamicObject()->setProperty (step, r);
    }

    void addSizeTest()
    {
        steps.push_back ([this]
        {
            cycleIndex = 1000;
            sizeRecord ("firstOpenCycle0", firstOpenW, firstOpenH, 960, 600);
            openEditor();
            return true;
        });
        steps.push_back (waitFor ("pageReady", 15000.0));
        steps.push_back ([this] { sizeRecord ("open", openW, openH, 960, 600); return true; });
        addNative ("sz0", "resizeEditor", { 700, 400 },   [this] (const juce::var&) { sizeRecord ("resize700x400", editor->getWidth(), editor->getHeight(), 800, 500); });
        addNative ("sz1", "resizeEditor", { 2000, 2000 }, [this] (const juce::var&) { sizeRecord ("resize2000x2000", editor->getWidth(), editor->getHeight(), 1600, 1000); });
        addNative ("sz2", "resizeEditor", { 1234, 777 },  [this] (const juce::var&) { sizeRecord ("resize1234x777", editor->getWidth(), editor->getHeight(), 1234, 777); });
        steps.push_back ([this] { closeEditor(); return true; });
        steps.push_back (sleep (300.0));
        steps.push_back ([this] { openEditor(); return true; });
        steps.push_back (waitFor ("pageReady", 15000.0));
        steps.push_back ([this] { sizeRecord ("reopenRestores", openW, openH, 1234, 777); return true; });
        addNative ("sz3", "resizeEditor", { 960, 600 }, [this] (const juce::var&) { sizeRecord ("backTo960x600", editor->getWidth(), editor->getHeight(), 960, 600); });
        steps.push_back ([this] { closeEditor(); return true; });
        steps.push_back (sleep (200.0));
    }

    void timerCallback() override
    {
        while (stepIndex < steps.size())
        {
            if (! steps[stepIndex]()) return;
            ++stepIndex;
        }
    }

    void finish()
    {
        stopTimer();
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty ("cycles", cycles);
        root->setProperty ("presets", presetResults);
        root->setProperty ("rows", juce::var (rows.get()));
        root->setProperty ("raf", rafInfo);
        root->setProperty ("timeouts", timeouts);
        root->setProperty ("home", homeInfo);
        root->setProperty ("presetsSaved", presetOk);
        root->setProperty ("listenBandAtEnd", proc->listenBand.load());
       #if JUCE_DEBUG
        root->setProperty ("build", "Debug");
       #else
        root->setProperty ("build", "Release");
       #endif
        const auto text = juce::JSON::toString (juce::var (root.get()));
        if (cfg.out != juce::File()) cfg.out.replaceWithText (text, false, false, "\n");
        std::printf ("BENCH DONE: %d cycles, %d preset tests, %d timeouts\n",
                     cycles.size(), presetResults.size(), timeouts.size());
        std::fflush (stdout);
        juce::MessageManager::callAsync ([] { juce::JUCEApplicationBase::quit(); });
    }

    Config cfg;
    BenchLogger logger;
    std::unique_ptr<OuariconParametricEQAudioProcessor> proc;
    std::unique_ptr<BenchWindow> window;
    std::unique_ptr<Editor> editor;
    juce::Array<juce::RangedAudioParameter*> params;
    std::unique_ptr<GestureCounter> gestures;
    std::vector<float> stateA, stateB;
    juce::MemoryBlock blobA, blobB;
    bool curIsA = false, presetOk = false;
    juce::String homeInfo;

    std::vector<Step> steps;
    size_t stepIndex = 0;
    int cycleIndex = 0, nextTestId = 1;
    double tOpen = 0, tCtor = 0, tShown = 0, bindOutSettleMs = 0;
    int openW = 0, openH = 0, firstOpenW = 0, firstOpenH = 0;
    int disabledPass = 0, uiChecks = 0, uiPass = 0;
    std::map<juce::String, std::pair<double, juce::var>> events, keyed;

    juce::var currentCycle;
    juce::Array<juce::var> cycles, presetResults;
    juce::DynamicObject::Ptr rows { new juce::DynamicObject() };
    juce::var rafInfo;
    juce::StringArray timeouts;
};

class EditorBenchApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "EditorBench"; }
    const juce::String getApplicationVersion() override { return "0.7.0"; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise (const juce::String&) override
    {
        Config c;
        const auto args = getCommandLineParameterArray();
        for (int i = 0; i < args.size(); ++i)
        {
            const auto a = args[i];
            const auto next = i + 1 < args.size() ? args[i + 1] : juce::String();
            if (a == "--cycles")      { c.cycles = next.getIntValue(); ++i; }
            else if (a == "--reps")   { c.reps = next.getIntValue(); ++i; }
            else if (a == "--idle")   { c.idleSec = next.getIntValue(); ++i; }
            else if (a == "--notests") c.tests = false;
            else if (a == "--nosize")  c.sizeTest = false;
            else if (a == "--out")    { c.out = juce::File (next); ++i; }
        }
        driver = std::make_unique<Driver> (c);
    }

    void shutdown() override { driver.reset(); }

private:
    std::unique_ptr<Driver> driver;
};

START_JUCE_APPLICATION (EditorBenchApp)
