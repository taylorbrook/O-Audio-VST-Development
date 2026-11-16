#include <JuceHeader.h>
#include <thread>
#include "BenchEditor.h"

namespace
{
    double nowMs() { return juce::Time::getMillisecondCounterHiRes(); }

    struct Config
    {
        bool fallback = false;
        int cycles = 10;
        int reps = 10;
        int idleSec = 5;
        bool tests = true;
        juce::File out;
    };
}

class BenchWindow : public juce::DocumentWindow
{
public:
    BenchWindow() : DocumentWindow ("RelayBench", juce::Colours::black, DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setAlwaysOnTop (true);
    }
    void closeButtonPressed() override {}
};

class Driver : private juce::Timer
{
public:
    explicit Driver (Config c) : cfg (std::move (c))
    {
        proc = std::make_unique<OuariconParametricEQAudioProcessor>();
        proc->setRateAndBufferSizeDetails (48000.0, 512);
        proc->prepareToPlay (48000.0, 512);

        for (auto* p : proc->getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
                params.add (r);

        // Two alternating "presets": every one of the 196 parameters differs between A and B.
        for (int i = 0; i < params.size(); ++i)
        {
            auto* p = params[i];
            const auto id = p->getParameterID();
            const int steps = p->getNumSteps();
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
            juce::ignoreUnused (steps);
            stateA.push_back (a);
            stateB.push_back (b);
            bound.push_back (! cfg.fallback || ! id.startsWithChar ('b') || id.startsWith ("b01_")
                             || id == "bypass");
        }

        // Session blobs for the setStateInformation path
        applyDirect (stateA); proc->getStateInformation (blobA);
        applyDirect (stateB); proc->getStateInformation (blobB);
        curIsA = false;

        window = std::make_unique<BenchWindow>();
        window->setTopLeftPosition (80, 80);

        for (int c = 0; c < cfg.cycles; ++c)
            addCycle (c);

        steps.push_back ([this] { finish(); return true; });
        startTimer (2);
    }

    ~Driver() override
    {
        stopTimer();
        editor.reset();
        window.reset();
        proc.reset();
    }

private:
    using Step = std::function<bool()>;

    // ── helpers ─────────────────────────────────────────────────────────────
    void applyDirect (const std::vector<float>& s)
    {
        for (int i = 0; i < params.size(); ++i)
            params[i]->setValueNotifyingHost (s[(size_t) i]);
    }

    int countExpected (const std::vector<float>& s) const
    {
        int n = 0;
        for (int i = 0; i < params.size(); ++i)
            if (bound[(size_t) i] && std::abs (params[i]->getValue() - s[(size_t) i]) > 1.0e-6f)
                ++n;
        return n;
    }

    void emit (const juce::String& id, const juce::var& v = {})
    {
        if (editor != nullptr && editor->getWebView() != nullptr)
            editor->getWebView()->emitEventIfBrowserIsVisible (id, v);
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

    static juce::var obj() { return juce::var (new juce::DynamicObject()); }

    // ── one open / test / close cycle ───────────────────────────────────────
    void addCycle (int c)
    {
        steps.push_back ([this, c]
        {
            cycleIndex = c;
            events.clear();
            keyed.clear();
            tOpen = nowMs();
            editor = std::make_unique<BenchEditor> (*proc, cfg.fallback,
                                                    [this] (const juce::var& v, double t) { onBench (v, t); });
            tCtor = nowMs();
            window->setContentNonOwned (editor.get(), true);
            window->setVisible (true);
            window->toFront (true);
            tShown = nowMs();
            return true;
        });
        steps.push_back (waitFor ("secondFrame", 15000.0));
        steps.push_back ([this]
        {
            auto r = obj();
            auto* o = r.getDynamicObject();
            o->setProperty ("cycle", cycleIndex);
            o->setProperty ("ctorMs", tCtor - tOpen);
            o->setProperty ("shownMs", tShown - tOpen);
            auto& tm = editor->timings;
            o->setProperty ("relaysMs", tm.relaysMs);
            o->setProperty ("optionsMs", tm.optionsMs);
            o->setProperty ("webViewCtorMs", tm.webViewMs);
            o->setProperty ("attachMs", tm.attachMs);
            o->setProperty ("numRelays", tm.numRelays);
            o->setProperty ("numEventListeners", tm.numEventListeners);
            o->setProperty ("estUserScriptBytes", (juce::int64) tm.estUserScriptBytes);
            o->setProperty ("estEventIdScriptBytes", (juce::int64) tm.estEventIdScriptBytes);
            o->setProperty ("estInitDataBytes", (juce::int64) tm.estInitDataBytes);
            for (auto k : { "scriptStart", "dcl", "load", "allProps", "firstPaint", "secondFrame" })
                o->setProperty (juce::String (k) + "Ms", events.count (k) ? events[k].first - tOpen : -1.0);
            if (events.count ("allProps"))  o->setProperty ("allProps", events["allProps"].second);
            if (events.count ("dcl"))       o->setProperty ("dcl", events["dcl"].second);
            if (events.count ("secondFrame")) o->setProperty ("console", events["secondFrame"].second);
            currentCycle = r;
            return true;
        });

        if (c == 0 && cfg.tests)
            addTests();

        steps.push_back ([this]
        {
            auto* o = currentCycle.getDynamicObject();
            juce::DynamicObject::Ptr nc = new juce::DynamicObject();
            for (auto& [k, v] : editor->nativeCalls) nc->setProperty (k, v);
            o->setProperty ("nativeCalls", juce::var (nc.get()));
            o->setProperty ("missingResources", editor->missingResources);
            o->setProperty ("resourceRequests", editor->resourceRequests);

            const double t0 = nowMs();
            window->clearContentComponent();
            editor.reset();
            o->setProperty ("closeMs", nowMs() - t0);
            o->setProperty ("cycleMs", nowMs() - tOpen);
            cycles.add (currentCycle);
            return true;
        });
        steps.push_back (sleep (400.0));
    }

    void addTests()
    {
        steps.push_back (sleep (300.0));
        steps.push_back ([this] { emit ("__bench_raf"); return true; });
        steps.push_back (waitFor ("raf", 5000.0));
        steps.push_back ([this] { rafInfo = events.count ("raf") ? events["raf"].second : juce::var(); return true; });
        steps.push_back (sleep (300.0));

        const char* methods[] = { "single", "msgLoop", "setState", "offThread" };
        for (int rep = 0; rep < cfg.reps; ++rep)
            for (auto m : methods)
                addPresetTest (m, rep);

        // Idle: no streaming timer (relays only), then with the 30 Hz editor timer
        for (int streaming = 0; streaming < 2; ++streaming)
        {
            steps.push_back ([this, streaming]
            {
                emit ("__bench_disarm");
                editor->setStreaming (streaming == 1);
                events.erase ("idle");
                emit ("__bench_idle_start");
                return true;
            });
            steps.push_back (sleep (cfg.idleSec * 1000.0));
            steps.push_back ([this] { emit ("__bench_idle_end"); return true; });
            steps.push_back (waitFor ("idle", 3000.0));
            steps.push_back ([this, streaming]
            {
                if (events.count ("idle"))
                    idleResults->setProperty (streaming ? "with30HzTimer" : "relaysOnly", events["idle"].second);
                return true;
            });
        }
    }

    void addPresetTest (juce::String method, int rep)
    {
        const int id = nextTestId++;
        auto rec = std::make_shared<juce::var> (obj());

        steps.push_back ([this, method, id, rec, rep]
        {
            std::vector<float> target;
            const std::vector<float>* blobTarget = nullptr;
            if (method == "single")
            {
                for (auto* p : params) target.push_back (p->getValue());
                int idx = 0;
                for (int i = 0; i < params.size(); ++i)
                    if (params[i]->getParameterID() == (cfg.fallback ? "b01_freq" : "b05_freq")) idx = i;
                target[(size_t) idx] = target[(size_t) idx] > 0.5f ? 0.3f : 0.7f;
            }
            else
            {
                blobTarget = curIsA ? &stateB : &stateA;
                target = *blobTarget;
            }
            const int expected = countExpected (target);

            auto arm = obj();
            arm.getDynamicObject()->setProperty ("id", id);
            arm.getDynamicObject()->setProperty ("expected", expected);
            emit ("__bench_arm", arm);

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
            else
            {
                std::thread th ([this, target]
                {
                    for (int i = 0; i < params.size(); ++i)
                        if (std::abs (params[i]->getValue() - target[(size_t) i]) > 1.0e-6f)
                            params[i]->setValueNotifyingHost (target[(size_t) i]);
                });
                th.join();
            }
            const double t1 = nowMs();
            if (blobTarget != nullptr) curIsA = ! curIsA;

            auto* o = rec->getDynamicObject();
            o->setProperty ("method", method);
            o->setProperty ("rep", rep);
            o->setProperty ("id", id);
            o->setProperty ("expected", expected);
            o->setProperty ("applyMs", t1 - t0);
            o->setProperty ("t0", t0);
            return true;
        });
        steps.push_back (waitFor ("settledPaint:" + juce::String (id), 5000.0, true));
        if (method != "single")
            steps.push_back (waitFor ("curvePaint:" + juce::String (id), 400.0, true));
        steps.push_back ([this, id, rec]
        {
            auto* o = rec->getDynamicObject();
            const double t0 = o->getProperty ("t0");
            for (auto k : { "settled", "settledPaint", "curvePaint" })
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
        root->setProperty ("mode", cfg.fallback ? "fallback" : "full");
        root->setProperty ("cycles", cycles);
        root->setProperty ("presets", presetResults);
        root->setProperty ("idle", juce::var (idleResults.get()));
        root->setProperty ("raf", rafInfo);
        root->setProperty ("timeouts", timeouts);
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
    std::unique_ptr<OuariconParametricEQAudioProcessor> proc;
    std::unique_ptr<BenchWindow> window;
    std::unique_ptr<BenchEditor> editor;
    juce::Array<juce::RangedAudioParameter*> params;
    std::vector<float> stateA, stateB;
    std::vector<bool> bound;
    juce::MemoryBlock blobA, blobB;
    bool curIsA = false;

    std::vector<Step> steps;
    size_t stepIndex = 0;
    int cycleIndex = 0, nextTestId = 1;
    double tOpen = 0, tCtor = 0, tShown = 0;
    std::map<juce::String, std::pair<double, juce::var>> events, keyed;

    juce::var currentCycle;
    juce::Array<juce::var> cycles, presetResults;
    juce::DynamicObject::Ptr idleResults { new juce::DynamicObject() };
    juce::var rafInfo;
    juce::StringArray timeouts;
};

class RelayBenchApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "RelayBench"; }
    const juce::String getApplicationVersion() override { return "0.6.0"; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise (const juce::String&) override
    {
        Config c;
        const auto args = getCommandLineParameterArray();
        for (int i = 0; i < args.size(); ++i)
        {
            const auto a = args[i];
            const auto next = i + 1 < args.size() ? args[i + 1] : juce::String();
            if (a == "--fallback") c.fallback = true;
            else if (a == "--cycles") { c.cycles = next.getIntValue(); ++i; }
            else if (a == "--reps")   { c.reps = next.getIntValue(); ++i; }
            else if (a == "--idle")   { c.idleSec = next.getIntValue(); ++i; }
            else if (a == "--notests") c.tests = false;
            else if (a == "--out")    { c.out = juce::File (next); ++i; }
        }
        driver = std::make_unique<Driver> (c);
    }

    void shutdown() override { driver.reset(); }

private:
    std::unique_ptr<Driver> driver;
};

START_JUCE_APPLICATION (RelayBenchApp)
