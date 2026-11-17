    // ════════════════════════════════════════════════════════════════════════
    // PLAN-3.3 T11 rows (included inside Driver, after Rows32.inc). The real
    // editor and page. Rows that need audio run a bench thread (Feeder) that
    // calls processBlock in real time; every other row runs with the
    // transport stopped, as in 3.1 / 3.2.
    // ════════════════════════════════════════════════════════════════════════

    struct Feeder
    {
        OuariconParametricEQAudioProcessor* proc = nullptr;
        std::thread th;
        std::atomic<bool> running { false };
        std::atomic<int> signal { 0 };          // 0 silence, 1 noise, 2 sine
        std::atomic<double> amp { 0.5 }, hz { 1000.0 };
        std::atomic<bool> record { false };
        double rate = 48000.0;
        int block = 512, channels = 2;
        std::mutex lock;
        std::vector<double> times;              // processBlock µs while `record`
        std::atomic<int> rtPolicyResult { -1 }; // thread_policy_set's kern_return_t (0 = KERN_SUCCESS)

        void start (OuariconParametricEQAudioProcessor& p, double fs, int blk, int ch)
        {
            stop();
            proc = &p; rate = fs; block = blk; channels = ch;
            running = true;
            th = std::thread ([this] { loop(); });
        }

        void stop()
        {
            running = false;
            if (th.joinable()) th.join();
        }

        std::vector<double> take() { const std::lock_guard<std::mutex> g (lock); auto t = times; times.clear(); return t; }

        void loop()
        {
            // User-interactive QoS (a performance core). No Mach time-constraint
            // policy: this thread spins between blocks (below), and a declared
            // computation it overruns gets it demoted (execute --diag-audio).
            rtPolicyResult = pthread_set_qos_class_self_np (QOS_CLASS_USER_INTERACTIVE, 0);
            juce::AudioBuffer<float> buf (channels, block);
            juce::MidiBuffer midi;
            juce::Random rnd (7);
            double phase = 0.0;
            const auto t0 = std::chrono::steady_clock::now();
            long k = 0;

            while (running)
            {
                const int s = signal.load();
                const double a = amp.load(), f = hz.load();

                for (int i = 0; i < block; ++i)
                {
                    const double v = s == 2 ? a * std::sin (phase) : 0.0;
                    if (s == 2) { phase += 2.0 * juce::MathConstants<double>::pi * f / rate; if (phase > 2.0 * juce::MathConstants<double>::pi) phase -= 2.0 * juce::MathConstants<double>::pi; }
                    for (int c = 0; c < channels; ++c)
                        buf.setSample (c, i, (float) (s == 1 ? a * (2.0 * rnd.nextDouble() - 1.0) : v));
                }

                const auto b0 = std::chrono::steady_clock::now();
                proc->processBlock (buf, midi);
                const double us = std::chrono::duration<double, std::micro> (std::chrono::steady_clock::now() - b0).count();

                if (record) { const std::lock_guard<std::mutex> g (lock); times.push_back (us); }

                ++k;
                // Spin to the next block's deadline (no sleep): a core that idles
                // between blocks starts each one cold and reads 1.4 - 2 ms instead
                // of ~0.3 ms, and how often it idles depends on the message
                // thread's own load, i.e. on the editor (execute --diag-audio).
                // A spinning feeder holds its core, so E-audio sees only the
                // editor's interference.
                const auto deadline = t0 + std::chrono::duration_cast<std::chrono::steady_clock::duration> (
                                               std::chrono::duration<double> ((double) k * block / rate));
                while (running && std::chrono::steady_clock::now() < deadline)
                    asm volatile ("yield");
            }
        }
    };

    Feeder feeder;

    void feedStart (double fs, int blk, int ch) { feeder.start (*proc, fs, blk, ch); }
    void feedNoise (double a)                   { feeder.amp = a; feeder.signal = 1; }
    void feedSine (double f, double db)         { feeder.hz = f; feeder.amp = std::pow (10.0, db / 20.0); feeder.signal = 2; }
    void feedSilence()                          { feeder.signal = 0; }

    /** Stops the feeder, re-prepares the processor (layout, rate, block). */
    void reprep (double fs, int blk, int ch)
    {
        feeder.stop();
        const auto set = ch == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (set); l.outputBuses.add (set);
        proc->releaseResources();
        proc->setBusesLayout (l);
        proc->setRateAndBufferSizeDetails (fs, blk);
        proc->prepareToPlay (fs, blk);
    }

    // ── C++-side analyzer / outputLevel emits (bench hooks) ─────────────────
    struct AnEmit { double t; int tap; juce::Array<int> v; };
    std::vector<AnEmit> anLog;
    std::vector<std::pair<double, juce::Array<int>>> levelLog;
    bool anRecord = false, levelRecord = false;
    int anCount[2] { 0, 0 }, levelCount = 0;

    void onAnalyzerEmit (int tap, const juce::Array<int>& v)
    {
        ++anCount[tap & 1];
        if (anRecord) anLog.push_back ({ nowMs(), tap & 1, v });
    }

    void onOutputLevel (const juce::Array<int>& v)
    {
        ++levelCount;
        if (levelRecord) levelLog.push_back ({ nowMs(), v });
    }

    static double spanStart (int p) { return 10.0 * std::pow (3000.0, p / 256.0); }
    static double spanCentre (int p) { return 10.0 * std::pow (3000.0, (p + 0.5) / 256.0); }

    void clickMode (int m) { call ("clickEl", { "#seg-analyzer button[data-v=\"" + juce::String (m) + "\"]" }, "done"); }

    static juce::var statsVar (std::vector<double> v)
    {
        auto o = juce::var (new juce::DynamicObject());
        std::sort (v.begin(), v.end());
        const auto q = [&v] (double p) { return v.empty() ? -1.0 : v[(size_t) std::min ((double) v.size() - 1, std::floor (p * (double) v.size()))]; };
        o.getDynamicObject()->setProperty ("n", (int) v.size());
        o.getDynamicObject()->setProperty ("median", q (0.5));
        o.getDynamicObject()->setProperty ("p99", q (0.99));
        o.getDynamicObject()->setProperty ("max", v.empty() ? -1.0 : v.back());
        return o;
    }

    // ════════════════════════════════════════════════════════════════════════
    void addTests33()
    {
        now ([this] { neutral(); feeder.stop(); });
        addNative ("ls-reset", "setListenBand", { 0 }, [] (const juce::var&) {});
        steps.push_back (sleep (200.0));
        readGeom();

        addAnalyzerRows();
        addMeterRows();
        addIdleDecayRow();
        addFillsRow();
        addListenRows33();
        addCreateSeededRow();
        addGbypassRow();
        addSumviewRow();
        addFpsAudioRows();

        now ([this] { feeder.stop(); neutral(); });
        clickMode (2);
        addNative ("ls-reset2", "setListenBand", { 0 }, [] (const juce::var&) {});
        steps.push_back (sleep (200.0));
    }

    // ── E-an-modes, E-an-stale, E-an-nyq, E-an-peak (UI-03) ─────────────────
    void addAnalyzerRows()
    {
        now ([this] { neutral(); feedStart (48000.0, 512, 2); feedNoise (0.5); });
        steps.push_back (sleep (600.0));

        for (const int m : { 0, 1, 2, 3 })
        {
            clickMode (m);
            steps.push_back (sleep (400.0));
            auto w0 = std::make_shared<std::array<std::uint32_t, 2>>();
            now ([this, w0]
            {
                anLog.clear(); anRecord = true;
                (*w0)[0] = proc->getPreTap().getWriteIndex(); (*w0)[1] = proc->getPostTap().getWriteIndex();
            });
            steps.push_back (sleep (1000.0));
            now ([this, m, w0]
            {
                anRecord = false;
                int n[2] { 0, 0 };
                for (auto& e : anLog) ++n[e.tap];
                const std::uint32_t d0 = proc->getPreTap().getWriteIndex() - (*w0)[0], d1 = proc->getPostTap().getWriteIndex() - (*w0)[1];
                const bool pre = (m & 1) != 0, post = (m & 2) != 0;
                auto d = obj(); auto* o = d.getDynamicObject();
                o->setProperty ("preEmits", n[0]); o->setProperty ("postEmits", n[1]);
                o->setProperty ("preWritten", (int) d0); o->setProperty ("postWritten", (int) d1);
                o->setProperty ("processorMode", proc->analyzerMode.load());
                const bool pass = (n[0] > 0) == pre && (n[1] > 0) == post && (pre ? d0 > 0 : d0 == 0) && (post ? d1 > 0 : d1 == 0)
                               && proc->analyzerMode.load() == m;
                verdict ("E-an-modes", "mode " + juce::String (m) + ": taps emitted and written", pass, d);
            });
        }

        // A mode change that adds a tap emits it within one tick (from Pre to Pre+Post).
        clickMode (1);
        steps.push_back (sleep (400.0));
        auto t0 = std::make_shared<double> (0.0);
        now ([this, t0] { anLog.clear(); anRecord = true; *t0 = nowMs(); proc->analyzerMode.store (3); });
        steps.push_back (sleep (300.0));
        now ([this, t0]
        {
            anRecord = false;
            double first = -1.0;
            for (auto& e : anLog) if (e.tap == 1) { first = e.t - *t0; break; }
            verdict ("E-an-modes", "a mode change adding Post emits it within one tick", first >= 0.0 && first <= 25.0,
                     juce::var ("first Post emit " + juce::String (first, 1) + " ms after the change (gate 25 ms)"));
        });
        clickMode (3);   // the page's segment back in step with the processor

        // E-an-stale: Post with quiet noise, Off, 3.5 s silence, Post: no frame of the old spectrum.
        auto noiseTop = std::make_shared<int> (-2000);
        clickMode (2);
        now ([this] { feedNoise (0.05); anLog.clear(); anRecord = true; });
        steps.push_back (sleep (1000.0));
        now ([this, noiseTop]
        {
            anRecord = false;
            for (auto& e : anLog) if (e.tap == 1) for (auto v : e.v) if (v != opeq::kAnalyzerNoData) *noiseTop = juce::jmax (*noiseTop, v);
        });
        clickMode (0);
        now ([this] { feedSilence(); });
        steps.push_back (sleep (3500.0));
        now ([this] { anLog.clear(); anRecord = true; });
        clickMode (2);
        steps.push_back (sleep (600.0));
        now ([this, noiseTop]
        {
            anRecord = false;
            int frames = 0, firstTop = -2000, allTop = -2000;
            for (auto& e : anLog)
            {
                if (e.tap != 1) continue;
                int top = -2000;
                for (auto v : e.v) if (v != opeq::kAnalyzerNoData) top = juce::jmax (top, v);
                if (frames == 0) firstTop = top;
                allTop = juce::jmax (allTop, top);
                ++frames;
            }
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("noiseTopTenths", *noiseTop); o->setProperty ("postFramesAfterSwitch", frames);
            o->setProperty ("firstFrameTop", firstTop); o->setProperty ("allFramesTop", allTop);
            verdict ("E-an-stale", "first Post frame after Off -> Post is <= -1000 everywhere (and every frame in 0.6 s)",
                     *noiseTop > -900 && frames >= 1 && firstTop <= -1000 && allTop <= -1000, d);
        });

        // E-an-nyq: 48 kHz, Post with noise: null from the first span >= 24 kHz; no ink right of fx(24 kHz).
        clickMode (0);
        now ([this] { feedNoise (0.5); });
        steps.push_back (sleep (400.0));
        readGeom();
        call ("anBase", {}, "done");
        clickMode (2);
        steps.push_back (sleep (800.0));
        now ([this] { anLog.clear(); anRecord = true; });
        steps.push_back (sleep (200.0));
        auto xLimit = std::make_shared<double> (0.0);
        now ([this, xLimit] { anRecord = false; *xLimit = fxP (24000.0) + 2.0; });
        steps.push_back ([this, xLimit]
        {
            keyed.erase ("anDiff:nyq");
            js ("window.__bench.anDiff('nyq'," + juce::String (*xLimit) + ");");
            return true;
        });
        steps.push_back (waitFor ("anDiff:nyq", 5000.0, true));
        now ([this, xLimit]
        {
            const auto rep = keyed.count ("anDiff:nyq") ? keyed["anDiff:nyq"].second : juce::var();
            juce::Array<int> last;
            for (auto& e : anLog) if (e.tap == 1) last = e.v;
            int mismatched = 0, nulls = 0, firstNull = -1;
            for (int p = 0; p < last.size(); ++p)
            {
                const bool isNull = last[p] == opeq::kAnalyzerNoData, want = spanStart (p) >= 24000.0;
                nulls += isNull ? 1 : 0;
                if (isNull && firstNull < 0) firstNull = p;
                mismatched += isNull != want ? 1 : 0;
            }
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("points", last.size()); o->setProperty ("nulls", nulls); o->setProperty ("firstNull", firstNull);
            o->setProperty ("mismatched", mismatched); o->setProperty ("diff", rep); o->setProperty ("xLimit", *xLimit);
            verdict ("E-an-nyq", "null exactly from the first span >= 24 kHz; no analyzer ink right of fx(24 kHz)",
                     last.size() == 256 && mismatched == 0 && nulls > 0 && (bool) rep["baseOk"] && (int) rep["n"] > 200 && (int) rep["right"] == 0, d);
        });

        // E-an-peak: a sine at the centre of span 240 (18.5 kHz), -6 dBFS, Post, at 1600 x 1000: the
        // trace's highest pixel within 1 px of fx(f). (A span-centre frequency: the drawn peak can
        // sit anywhere in its span, +-0.5 span = +-2.8 px at this width; at the span centre the page's
        // (i + 0.5) / n is exact and the reverted i / (n - 1) is 2.4 px off.) 1 kHz is recorded.
        addNative ("an-size", "resizeEditor", { 1600, 1000 }, [] (const juce::var&) {});
        steps.push_back (sleep (500.0));
        readGeom();
        for (const double f : { spanCentre (240), 1000.0 })
        {
            const bool gated = f > 2000.0;
            clickMode (0);
            now ([this, f] { feedSine (f, -6.0); });
            steps.push_back (sleep (400.0));
            call ("anBase", {}, "done");
            clickMode (2);
            steps.push_back (sleep (1000.0));
            const auto tag = juce::String (gated ? "peak" : "peak1k");
            steps.push_back ([this, tag]
            {
                keyed.erase ("anDiff:" + tag);
                js ("window.__bench.anDiff('" + tag + "',1e9);");
                return true;
            });
            steps.push_back (waitFor ("anDiff:" + tag, 5000.0, true));
            now ([this, f, gated, tag]
            {
                const auto rep = keyed.count ("anDiff:" + tag) ? keyed["anDiff:" + tag].second : juce::var();
                geom.plotW = (double) rep["plotW"];
                const double want = fxP (f), got = rep["topX"], off = got - want;
                auto d = obj(); auto* o = d.getDynamicObject();
                o->setProperty ("f", f); o->setProperty ("fx", want); o->setProperty ("topX", got); o->setProperty ("offsetPx", off);
                o->setProperty ("topY", rep["topY"]); o->setProperty ("plotW", rep["plotW"]);
                if (gated) verdict ("E-an-peak", "span-centre sine: highest trace pixel within 1 px of fx(f)", (double) rep["topY"] > 0 && std::abs (off) <= 1.0, d);
                else       put ("E-an-peak", "1 kHz (recorded)", d);
            });
        }
        addNative ("an-size-back", "resizeEditor", { 960, 600 }, [] (const juce::var&) {});
        steps.push_back (sleep (300.0));
        readGeom();
    }

    // ── E-meter (UI-06), E-meter-rest ───────────────────────────────────────
    void addMeterRows()
    {
        now ([this] { feedSine (1000.0, -6.0); });
        steps.push_back (sleep (500.0));
        call ("meterTops", {}, "meterTops", [this] (const juce::var& r)
        {
            // The peak line sits at round (my(-6)), 1.5 px tall; the level fill starts at my(-6)
            // and shows from just under the line: level top within [m6 - 1, m6 + 2.5].
            const double m6 = r["m6"], t0 = r["tops"][0], t1 = r["tops"][1], l0 = r["levels"][0], l1 = r["levels"][1];
            verdict ("E-meter", "48 k / 512: both bar tops at my(-6) +- 1 px after 0.5 s (peak line and level fill)",
                     std::abs (t0 - m6) <= 1.0 && std::abs (t1 - m6) <= 1.0 && l0 >= m6 - 1.0 && l0 <= m6 + 2.5 && l1 >= m6 - 1.0 && l1 <= m6 + 2.5, r);
        });
        call ("meterClears", { 300 }, "meterClears", [this] (const juce::var& r) { put ("E-meter-rest", "framesWhilePlaying", r); });

        // 44.1 k, prepared and fed in blocks of 2048 (empty ticks without the hold)
        now ([this] { reprep (44100.0, 2048, 2); feedStart (44100.0, 2048, 2); feedSine (1000.0, -6.0); });
        steps.push_back (sleep (1000.0));
        call ("meterTrack", { 2000 }, "meterTrack", [this] (const juce::var& r)
        {
            // The LEVEL fill's top, not the peak line (which holds for 1 s and hides a
            // sawtooth: execute mutant run, PeakHold depth 1 survived the peak-line probe).
            const double r0 = (double) r["lmax0"] - (double) r["lmin0"], r1 = (double) r["lmax1"] - (double) r["lmin1"], m6 = r["m6"];
            verdict ("E-meter", "44.1 k / 2048: level-fill top range <= 1 px over 2 s (no sawtooth), near my(-6); peak line range <= 1 px",
                     (int) r["n"] > 60 && (double) r["lmin0"] > 0 && r0 <= 1.0 && r1 <= 1.0 && (double) r["lmin0"] >= m6 - 1.0 && (double) r["lmax0"] <= m6 + 2.5
                         && (double) r["max0"] - (double) r["min0"] <= 1.0, r);
        }, 10000.0);

        // Mono: two equal bars
        now ([this] { reprep (48000.0, 512, 1); feedStart (48000.0, 512, 1); feedSine (1000.0, -6.0); });
        steps.push_back (sleep (800.0));
        call ("meterTops", {}, "meterTops", [this] (const juce::var& r)
        {
            const double m6 = r["m6"], t0 = r["tops"][0], t1 = r["tops"][1];
            verdict ("E-meter", "mono: two equal bars at my(-6) +- 1 px", t0 == t1 && (double) r["levels"][0] == (double) r["levels"][1] && std::abs (t0 - m6) <= 1.0, r);
        });
        now ([this] { reprep (48000.0, 512, 2); feedStart (48000.0, 512, 2); feedSine (1000.0, -6.0); });
        steps.push_back (sleep (500.0));

        // E-meter-rest: silence; the meter's loop stops once the peak reaches -60 (3.7 s from -6)
        now ([this] { feedSilence(); });
        steps.push_back (sleep (4500.0));
        call ("meterClears", { 2000 }, "meterClears", [this] (const juce::var& r)
        {
            const auto playing = row ("E-meter-rest")->getProperty ("framesWhilePlaying");
            verdict ("E-meter-rest", "no meter frames for 2 s at rest (frames while playing > 5)", (int) r["n"] == 0 && (int) playing["n"] > 5, r);
        }, 6000.0);
    }

    // ── E-idle extended: after audio stops and the displays decay ───────────
    void addIdleDecayRow()
    {
        auto at = std::make_shared<std::array<int, 4>>();
        now ([this] { feeder.stop(); });
        steps.push_back (sleep (8000.0));   // analyzer from -6 dB to the -120 dB clamp at 20 dB/s, the meter to rest
        now ([this, at]
        {
            bench ("idleStart", {});
            events.erase ("idle");
            *at = { (int) emits.size(), anCount[0] + anCount[1], levelCount, 0 };
        });
        steps.push_back (sleep (cfg.idleSec * 1000.0));
        now ([this] { bench ("idleEnd", {}); });
        steps.push_back (waitFor ("idle", 3000.0));
        now ([this, at]
        {
            const auto ev = events.count ("idle") ? events["idle"].second : juce::var();
            int jsAll = 0, posts = 0;
            if (auto* o = ev["evals"].getDynamicObject()) for (auto& nv : o->getProperties()) jsAll += (int) nv.value;
            if (auto* o = ev["posts"].getDynamicObject()) for (auto& nv : o->getProperties()) posts += (int) nv.value;
            auto d = obj(); auto* o = d.getDynamicObject();
            const int eq = (int) emits.size() - (*at)[0], an = anCount[0] + anCount[1] - (*at)[1], lv = levelCount - (*at)[2];
            o->setProperty ("eqSections", eq); o->setProperty ("analyzer", an); o->setProperty ("outputLevel", lv);
            o->setProperty ("jsEvents", jsAll); o->setProperty ("jsPosts", posts); o->setProperty ("idleSec", cfg.idleSec);
            verdict ("E-idle33", "after audio and decay: 0 relay / eqSections / analyzer / outputLevel messages",
                     events.count ("idle") > 0 && eq == 0 && an == 0 && lv == 0 && jsAll == 0 && posts == 0, d);
        });
    }

    // ── E-fills (UI-07) ─────────────────────────────────────────────────────
    static double hueOf (double r, double g, double b)
    {
        const double mx = juce::jmax (r, g, b), mn = juce::jmin (r, g, b), c = mx - mn;
        if (c <= 0) return -1.0;
        double h = mx == r ? std::fmod ((g - b) / c, 6.0) : mx == g ? (b - r) / c + 2.0 : (r - g) / c + 4.0;
        h *= 60.0;
        return h < 0 ? h + 360.0 : h;
    }
    static double hueDist (double a, double b) { const double d = std::abs (a - b); return juce::jmin (d, 360.0 - d); }

    void addFillsRow()
    {
        struct B { int b; double f, g; juce::uint32 pig; };
        // Frequencies off the grid's lines (the log grid draws every integer multiple of a decade).
        static const B bands[] = { { 1, 150.0, 9.0, 0x8F6212 }, { 2, 1500.0, -9.0, 0xA23B2E }, { 3, 6500.0, 9.0, 0x2C7265 } };
        clickMode (0);
        now ([this] { neutral(); for (auto& x : bands) bandSet (x.b, 0, x.f, x.g, 2.0); });
        steps.push_back (sleep (300.0));
        readGeom();
        auto pts = std::make_shared<juce::Array<juce::var>>();
        const auto fill = [this, pts]
        {
            pts->clear();
            for (auto& x : bands) { juce::Array<juce::var> p { fxP (x.f), gyP (x.g / 2.0) }; pts->add (juce::var (p)); }
        };
        const auto probeStep = [this, pts] (const juce::String& tag)
        {
            steps.push_back ([this, pts, tag]
            {
                keyed.erase ("pixels:" + tag);
                js ("window.__bench.pixels('" + tag + "'," + juce::JSON::toString (juce::var (*pts), true) + ");");
                return true;
            });
            steps.push_back (waitFor ("pixels:" + tag, 4000.0, true));
        };
        now (fill);
        probeStep ("fills");
        now ([this]
        {
            const auto rep = keyed.count ("pixels:fills") ? keyed["pixels:fills"].second : juce::var();
            for (int i = 0; i < 3; ++i)
            {
                const auto px = rep["pts"][i]["rgba"];
                const double h = hueOf (px[0], px[1], px[2]), want = hueOf ((bands[i].pig >> 16) & 255, (bands[i].pig >> 8) & 255, bands[i].pig & 255);
                auto d = obj(); auto* o = d.getDynamicObject();
                o->setProperty ("rgba", px); o->setProperty ("hue", h); o->setProperty ("pigmentHue", want);
                verdict ("E-fills", "band " + juce::String (bands[i].b) + " wash hue within 15 deg of its pigment",
                         (double) px[3] > 15.0 && h >= 0 && hueDist (h, want) <= 15.0, d);
            }
            setB (bp (2, "bypass"), true);
        });
        steps.push_back (sleep (300.0));
        probeStep ("fillsByp");
        now ([this]
        {
            const auto rep = keyed.count ("pixels:fillsByp") ? keyed["pixels:fillsByp"].second : juce::var();
            const auto px = rep["pts"][1]["rgba"];
            verdict ("E-fills", "band 2 bypassed: that pixel is the background (alpha < 6)", (double) px[3] < 6.0, juce::var (rep["pts"][1]));
            setB (bp (2, "bypass"), false);
        });
    }

    // ── E-listen33 (FUNC-08): the page's drag / latch state machine ─────────
    std::pair<double, double> nodeXY (int b)
    {
        const double x = fxP (juce::jlimit (10.0, 30000.0, getS (bp (b, "freq"))));
        const double y = gyP (getS (bp (b, "gain")) * getS ("gain_scale") / 100.0);
        return { x, y };
    }

    int nativeListenCount = 0;
    void readNatives() { call ("natives", {}, "natives", [this] (const juce::var& v) { nativeListenCount = (int) v["n"]["setListenBand"]; }); }

    /** Drag band b 60 px right (10 moves, 16 ms), read listenBand after the 3rd move, end. */
    void listenDrag (int b, const juce::String& end, std::function<void (int during)> onPause)
    {
        const auto tag = "ld" + juce::String (jsTag++);
        auto xy = std::make_shared<std::pair<double, double>>();
        now ([this, b, xy] { *xy = nodeXY (b); });
        steps.push_back ([this, tag, xy, end]
        {
            keyed.erase ("paused:" + tag); keyed.erase ("done:" + tag);
            js ("window.__bench.dragPauseEnd('" + tag + "'," + juce::String (xy->first) + "," + juce::String (xy->second) + ","
                + juce::String (xy->first + 60.0) + "," + juce::String (xy->second) + ",10,16,'" + end + "');");
            return true;
        });
        steps.push_back (waitFor ("paused:" + tag, 4000.0, true));
        steps.push_back (sleep (120.0));
        now ([this, onPause] { onPause (proc->listenBand.load()); js ("window.__bench.resume();"); });
        steps.push_back (waitFor ("done:" + tag, 4000.0, true));
        steps.push_back (sleep (150.0));
    }

    /** Selects band b and presses strip Listen only if it is not already pressed
        (idempotent: a check that leaves the latch on must not make the next latch
        toggle it off; execute mutant run, the hook-only-in-delete mutant). */
    void latch (int b)
    {
        call ("clickChip", { b }, "chip");
        call ("latchOn", {}, "done");
        steps.push_back (sleep (150.0));
    }

    void expectListen (const juce::String& check, int want, int delayMs = 300)
    {
        steps.push_back (sleep (delayMs));
        now ([this, check, want]
        {
            const int got = proc->listenBand.load();
            verdict ("E-listen33", check, got == want, juce::var ("listenBand " + juce::String (got) + " (want " + juce::String (want) + ")"));
        });
    }

    void addListenRows33()
    {
        now ([this]
        {
            neutral();
            bandSet (3, 0, 300.0, 6.0, 1.0); bandSet (5, 0, 3000.0, -6.0, 1.0);
            // Presets for (e) / (f): band 3 disabled; band 3 enabled at another frequency.
            setB (bp (3, "enabled"), false);
            proc->presetManager.savePreset ("bench-l-off");
            setB (bp (3, "enabled"), true); setS (bp (3, "freq"), 420.0);
            proc->presetManager.savePreset ("bench-l-on");
            setS (bp (3, "freq"), 300.0);
        });
        steps.push_back (sleep (300.0));
        readGeom();

        // (a) drag past the threshold -> the band; release -> off
        listenDrag (3, "up", [this] (int during) { verdict ("E-listen33", "(a) drag band 3: listenBand 2 during the drag", during == 2, juce::var (during)); });
        expectListen ("(a) release: -1", -1, 0);

        // (b) a click without movement never sets listen
        readNatives();
        auto n0 = std::make_shared<int> (0);
        now ([this, n0] { *n0 = nativeListenCount; });
        auto xy = std::make_shared<std::pair<double, double>>();
        now ([this, xy] { *xy = nodeXY (5); });
        steps.push_back ([this, xy]
        {
            keyed.erase ("done:clk5");
            js ("window.__bench.click('clk5'," + juce::String (xy->first) + "," + juce::String (xy->second) + ");");
            return true;
        });
        steps.push_back (waitFor ("done:clk5", 4000.0, true));
        steps.push_back (sleep (200.0));
        readNatives();
        now ([this, n0]
        {
            verdict ("E-listen33", "(b) click on node 5 without movement: 0 setListenBand writes, listen off",
                     nativeListenCount == *n0 && proc->listenBand.load() == -1 && proc->uiSelectedBand.load() == 5,
                     juce::var ("writes " + juce::String (nativeListenCount - *n0) + ", listenBand " + juce::String (proc->listenBand.load())));
        });

        // (c) latch 3, drag 5 -> 4 during; release -> back to 2
        latch (3);
        expectListen ("(c) latch band 3: 2", 2, 0);
        listenDrag (5, "up", [this] (int during) { verdict ("E-listen33", "(c) latched 3, drag band 5: 4 during", during == 4, juce::var (during)); });
        expectListen ("(c) release returns to the latch: 2", 2, 0);

        // (d) host automation disables the latched band -> off
        now ([this] { setB (bp (3, "enabled"), false); });
        expectListen ("(d) host disables band 3: -1", -1);
        now ([this] { setB (bp (3, "enabled"), true); });
        steps.push_back (sleep (200.0));

        // (e) a preset load that disables the latched band -> off
        latch (3);
        expectListen ("(e) re-latched 3: 2", 2, 0);
        now ([this] { proc->presetManager.loadPreset ("bench-l-off"); });
        expectListen ("(e) preset load disabling band 3: -1", -1, 400);
        now ([this] { setB (bp (3, "enabled"), true); });
        steps.push_back (sleep (200.0));

        // (f) a preset load keeping it enabled -> unchanged
        latch (3);
        now ([this] { proc->presetManager.loadPreset ("bench-l-on"); });
        expectListen ("(f) preset load keeping band 3 enabled: 2", 2, 400);

        // (g) bypass does not end listen
        now ([this] { setB (bp (3, "bypass"), true); });
        expectListen ("(g) bypass band 3: 2", 2);
        now ([this] { setB (bp (3, "bypass"), false); });

        // (h) Delete on the selected (latched) band -> off
        call ("clickChip", { 3 }, "chip");
        call ("key", { "Delete" }, "done");
        expectListen ("(h) Delete key on band 3: -1", -1, 400);
        now ([this] { setB (bp (3, "enabled"), true); });
        steps.push_back (sleep (200.0));

        // (i) strip Listen twice
        latch (3);
        expectListen ("(i) strip Listen: 2", 2, 0);
        call ("clickEl", { "#btn-listen" }, "done");
        expectListen ("(i) strip Listen again: -1", -1, 150);

        // (j) cancel and lost capture end a drag listen
        for (const auto* end : { "cancel", "lost" })
        {
            const juce::String e (end);
            listenDrag (3, e, [this, e] (int during) { verdict ("E-listen33", "(j) " + e + ": band 3 during", during == 2, juce::var (during)); });
            expectListen ("(j) " + e + " ends it: -1", -1, 0);
        }
    }

    // ── E-create-seeded (VERIFICATION-3.2 Issue 2: V15, V20) ────────────────
    void addCreateSeededRow()
    {
        now ([this]
        {
            neutral();
            setIndex (bp (1, "type"), 2); setIndex (bp (1, "placement"), 4); setB (bp (1, "bypass"), true);
            setB (bp (1, "enabled"), false);
        });
        steps.push_back (sleep (250.0));
        readGeom();
        auto pt = std::make_shared<std::pair<double, double>>();
        now ([this, pt] { *pt = freeSpot (0.45, 0.35); });
        steps.push_back ([this, pt]
        {
            keyed.erase ("done:seeded");
            js ("window.__bench.click('seeded'," + juce::String (pt->first) + "," + juce::String (pt->second) + ");");
            return true;
        });
        steps.push_back (waitFor ("done:seeded", 4000.0, true));
        steps.push_back (sleep (250.0));
        now ([this]
        {
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("enabled", getB (bp (1, "enabled"))); o->setProperty ("type", getI (bp (1, "type")));
            o->setProperty ("placement", getI (bp (1, "placement"))); o->setProperty ("bypass", getB (bp (1, "bypass")));
            verdict ("E-create-seeded", "band 1 seeded High Shelf / Side / bypassed / disabled -> Bell / Stereo / not bypassed",
                     getB (bp (1, "enabled")) && getI (bp (1, "type")) == 0 && getI (bp (1, "placement")) == 0 && ! getB (bp (1, "bypass")), d);
        });
    }

    // ── E-gbypass (VERIFICATION-3.2 Issue 4, V7) ────────────────────────────
    void addGbypassRow()
    {
        now ([this] { neutral(); bandSet (2, 0, 500.0, 9.0, 1.0); bandSet (6, 0, 4000.0, -9.0, 1.0); });
        steps.push_back (sleep (300.0));
        readGeom();
        auto xs = std::make_shared<juce::Array<juce::var>>();
        auto offFlat = std::make_shared<int> (0);
        const auto probeTo = [this, xs] (const juce::String& tag)
        {
            now ([this, xs] { *xs = probeXs (9); });
            steps.push_back ([this, xs, tag]
            {
                keyed.erase ("probe:" + tag);
                js ("window.__bench.curveProbe('" + tag + "'," + juce::JSON::toString (juce::var (*xs), true) + ");");
                return true;
            });
            steps.push_back (waitFor ("probe:" + tag, 5000.0, true));
        };
        probeTo ("gb-off");
        now ([this, offFlat]
        {
            const auto rep = keyed["probe:gb-off"].second;
            const double y0 = gyP (0.0);
            if (auto* cols = rep["cols"].getArray())
                for (auto& c : *cols)
                    if (auto* runs = c["runs"].getArray())
                        for (auto& r : *runs)
                            if (std::abs (0.5 * ((double) r[0] + (double) r[1]) - y0) > 3.0) { ++*offFlat; break; }
            setB ("bypass", true);
        });
        steps.push_back (sleep (300.0));
        probeTo ("gb-on");
        now ([this, offFlat]
        {
            const auto rep = keyed["probe:gb-on"].second;
            const double y0 = gyP (0.0);
            int valid = 0, ok = 0;
            if (auto* cols = rep["cols"].getArray())
                for (auto& c : *cols)
                {
                    auto* runs = c["runs"].getArray();
                    if (runs == nullptr || runs->isEmpty()) continue;   // a dash gap
                    ++valid;
                    bool all = true;
                    for (auto& r : *runs) all = all && std::abs (0.5 * ((double) r[0] + (double) r[1]) - y0) <= 1.0;
                    ok += all ? 1 : 0;
                }
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("columnsOffZeroBeforeBypass", *offFlat); o->setProperty ("valid", valid); o->setProperty ("onZero", ok);
            o->setProperty ("gy0", y0); o->setProperty ("probe", rep);
            verdict ("E-gbypass", "global bypass: every ink run on gy(0) +- 1 px (the curve was not flat before)", *offFlat >= 1 && valid >= 3 && ok == valid, d);
            setB ("bypass", false);
        });
    }

    // ── E-sumview (VERIFICATION-3.2 Issue 4, V8) ────────────────────────────
    void addSumviewRow()
    {
        now ([this] { neutral(); bandSet (4, 0, 400.0, 9.0, 1.0, 3); bandSet (7, 0, 4000.0, -9.0, 1.0, 4); });
        steps.push_back (sleep (200.0));
        call ("clickChip", { 7 }, "chip");
        steps.push_back (sleep (300.0));
        readGeom();
        probe ("E-sumview", "Side band selected: ink matches the SS composition", nullptr, [this] (const juce::var& res)
        {
            int differ = 0;
            if (auto* cols = res["cols"].getArray())
                for (auto& c : *cols)
                {
                    if (c.hasProperty ("skip")) continue;
                    const double f = c["f"];
                    if (std::abs (gyP (refDb (f, 3)) - gyP (refDb (f, 2))) > 6.0) ++differ;
                }
            verdict ("E-sumview", "vacuity: SS and MM differ by > 6 px at >= 1 probed column; view is SS",
                     differ >= 1 && (int) res["view"] == 3, juce::var ("columns differing " + juce::String (differ)));
        });
    }

    // ── E-fps, E-rate (ROADMAP 3.3 criterion 5; VERIFICATION-3.2 Issue 1), E-audio ─
    void busy24()
    {
        neutral();
        for (int b = 1; b <= 24; ++b)
        {
            bandSet (b, 1 + (b % 4), 30.0 * std::pow (600.0, (b - 1) / 23.0), (b % 2 ? 4.0 : -4.0), 1.0, b % 5);
            setIndex (bp (b, "slope"), 7);
        }
    }

    /** Host automation of band 12 along the scripted drag's path (the page's
        dragFor: x0 + 12 + 160 sin (pi t), y0 + 40 sin (1.3 pi t)) at 60 Hz for
        `ms`: the engine sees the same parameter motion with the editor closed
        or idle as during the page drag. */
    Step automate (double ms)
    {
        auto st = std::make_shared<std::array<double, 4>> (std::array<double, 4> { -1.0, 0.0, 0.0, 0.0 });   // t0, last, x0, y0
        return [this, st, ms]
        {
            auto& a = *st;
            const double t = nowMs();
            if (a[0] < 0) { a[0] = t; a[1] = -1e9; const auto xy = nodeXY (12); a[2] = xy.first; a[3] = xy.second; }
            if (t - a[0] >= ms) return true;
            if (t - a[1] >= 1000.0 / 60.0)
            {
                a[1] = t;
                const double ph = (t - a[0]) / 1000.0 * juce::MathConstants<double>::pi;
                setS (bp (12, "freq"), xfP (a[2] + 12.0 + 160.0 * std::sin (ph)));
                setS (bp (12, "gain"), ygP (a[3] + 40.0 * std::sin (ph * 1.3)) / (getS ("gain_scale") / 100.0));
            }
            return false;
        };
    }

    juce::var audioRun (int run)
    {
        auto t = feeder.take(); feeder.record = false;
        auto s = statsVar (t);
        s.getDynamicObject()->setProperty ("rtPolicy", feeder.rtPolicyResult.load());
        put ("E-audio", "run" + juce::String (run), s);
        return s;
    }

    void scriptedDrag (const juce::String& tag, double ms, std::function<void (const juce::var&)> then)
    {
        auto xy = std::make_shared<std::pair<double, double>>();
        now ([this, xy] { *xy = nodeXY (12); });
        steps.push_back ([this, xy, tag, ms]
        {
            keyed.erase ("dragFor:" + tag);
            js ("window.__bench.dragFor('" + tag + "'," + juce::String (xy->first) + "," + juce::String (xy->second) + ",160,40," + juce::String (ms) + ",60);");
            return true;
        });
        steps.push_back (waitFor ("dragFor:" + tag, ms + 8000.0, true));
        now ([this, tag, then] { then (keyed.count ("dragFor:" + tag) ? keyed["dragFor:" + tag].second : juce::var()); });
    }

    void addFpsAudioRows()
    {
        now ([this] { busy24(); feedStart (48000.0, 512, 2); feedNoise (0.5); });
        clickMode (3);
        addNative ("fps-size", "resizeEditor", { 1600, 1000 }, [] (const juce::var&) {});
        steps.push_back (sleep (800.0));
        readGeom();

        // E-fps + E-rate: a scripted 5 s node drag at 60 Hz, Pre+Post with noise, 192 sections
        auto e0 = std::make_shared<size_t> (0);
        auto t0 = std::make_shared<double> (0.0);
        now ([this, e0, t0] { *e0 = emits.size(); *t0 = nowMs(); });
        scriptedDrag ("fps", 5000.0, [this, e0, t0] (const juce::var& r)
        {
            std::vector<double> iv;
            for (size_t i = *e0 + 1; i < emits.size(); ++i)
                if (emits[i - 1].t >= *t0 + 200.0 && emits[i].t <= *t0 + 4800.0) iv.push_back (emits[i].t - emits[i - 1].t);
            const auto s = statsVar (iv);
            verdict ("E-fps", "rAF intervals during the drag: median <= 17.5 ms, p99 <= 25 ms (draw() recorded)",
                     (int) r["n"] > 200 && (double) r["median"] <= 17.5 && (double) r["p99"] <= 25.0, r);
            verdict ("E-rate", "median interval between eqSections emits during the drag <= 20 ms", (int) s["n"] > 100 && (double) s["median"] <= 20.0, s);
        });

        // E-audio: processBlock time, 24 x 96 dB/oct, 48 k / 512, real time, 10 s each:
        // (1) editor closed, (2) open, Off, no drag, (3) open, Pre+Post, scripted 60 fps drag.
        // Runs 1 and 2 move band 12 by host automation along the drag's path at 60 Hz
        // (execute deviation: a moving band redesigns every block, so a static run 1
        // would charge the engine's own motion cost to the editor).
        for (int attempt = 0; attempt < 2; ++attempt)
        {
            const auto gate = [this, attempt] (Step s) -> Step { return [this, attempt, s] { return (attempt == 0 || audioRetry) ? s() : true; }; };
            const auto pfx = juce::String (attempt == 0 ? "" : "retry ");

            steps.push_back (gate ([this] { closeEditor(); feeder.take(); feeder.record = true; return true; }));
            steps.push_back (gate (automate (10000.0)));
            steps.push_back (gate ([this, attempt] { audioRun (attempt * 10 + 1); openEditor(); return true; }));
            steps.push_back (gate (waitFor ("pageReady", 15000.0)));
            steps.push_back (gate (sleep (800.0)));
            steps.push_back (gate ([this] { keyed.erase ("geom:aud"); js ("window.__bench.geom('aud');"); return true; }));
            steps.push_back (gate (waitFor ("geom:aud", 4000.0, true)));
            steps.push_back (gate ([this] { if (keyed.count ("geom:aud")) geom = geomFrom (keyed["geom:aud"].second); return true; }));
            steps.push_back (gate ([this] { js ("document.querySelector('#seg-analyzer button[data-v=\"0\"]').click();"); return true; }));
            steps.push_back (gate (sleep (300.0)));
            steps.push_back (gate ([this] { feeder.take(); feeder.record = true; return true; }));
            steps.push_back (gate (automate (10000.0)));
            steps.push_back (gate ([this, attempt] { audioRun (attempt * 10 + 2); return true; }));
            steps.push_back (gate ([this] { js ("document.querySelector('#seg-analyzer button[data-v=\"3\"]').click();"); return true; }));
            steps.push_back (gate (sleep (300.0)));
            steps.push_back (gate ([this] { feeder.take(); feeder.record = true; return true; }));
            {
                auto xy = std::make_shared<std::pair<double, double>>();
                const auto tag = "aud" + juce::String (attempt);
                steps.push_back (gate ([this, xy] { *xy = nodeXY (12); return true; }));
                steps.push_back (gate ([this, xy, tag]
                {
                    keyed.erase ("dragFor:" + tag);
                    js ("window.__bench.dragFor('" + tag + "'," + juce::String (xy->first) + "," + juce::String (xy->second) + ",160,40,10000,60);");
                    return true;
                }));
                steps.push_back (gate (waitFor ("dragFor:" + tag, 20000.0, true)));
            }
            steps.push_back (gate ([this, attempt, pfx]
            {
                const auto r3 = audioRun (attempt * 10 + 3);
                const auto r1 = row ("E-audio")->getProperty ("run" + juce::String (attempt * 10 + 1));
                const double m1 = r1["median"], m3 = r3["median"], p1 = r1["p99"], p3 = r3["p99"];
                const bool pass = (int) r3["n"] > 500 && (int) r1["n"] > 500 && m3 <= 1.10 * m1 && p3 <= 1.25 * p1;
                auto d = obj(); auto* o = d.getDynamicObject();
                o->setProperty ("median closed / open+drag us", juce::String (m1, 1) + " / " + juce::String (m3, 1));
                o->setProperty ("p99 closed / open+drag us", juce::String (p1, 1) + " / " + juce::String (p3, 1));
                if (attempt == 0)
                {
                    audioRetry = ! pass;
                    put ("E-audio", "attempt1", d);
                    if (pass) verdict ("E-audio", "run 3 median within 10 % of run 1, p99 within 25 %", true, d);
                }
                else
                {
                    put ("E-audio", "attempt2", d);
                    verdict ("E-audio", "run 3 median within 10 % of run 1, p99 within 25 % (re-run after a first fail)", pass, d);
                }
                return true;
            }));
        }

        now ([this] { feeder.stop(); });
        addNative ("fps-size-back", "resizeEditor", { 960, 600 }, [] (const juce::var&) {});
        steps.push_back (sleep (300.0));
        readGeom();
    }

    bool audioRetry = false;

    // --diag-audio: processBlock time with the editor open / closed, band 12 static / automated.
    void addDiagAudio()
    {
        now ([this] { busy24(); feedStart (48000.0, 512, 2); feedNoise (0.5); });
        clickMode (0);
        steps.push_back (sleep (800.0));
        readGeom();
        const auto rec = [this] (const juce::String& name, bool automated)
        {
            now ([this] { feeder.take(); feeder.record = true; });
            if (automated) steps.push_back (automate (6000.0)); else steps.push_back (sleep (6000.0));
            now ([this, name] { auto t = feeder.take(); feeder.record = false; auto v = statsVar (t); v.getDynamicObject()->setProperty ("rt", feeder.rtPolicyResult.load()); put ("diag", name, v); });
        };
        rec ("open static", false);
        rec ("open automated", true);
        now ([this] { closeEditor(); });
        rec ("closed static", false);
        rec ("closed automated", true);
        now ([this] { openEditor(); });
        steps.push_back (waitFor ("pageReady", 15000.0));
        rec ("reopened automated", true);
        now ([this] { feeder.stop(); });
    }
