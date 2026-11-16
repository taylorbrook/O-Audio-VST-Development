    // ════════════════════════════════════════════════════════════════════════
    // research-3.3 draw-bench rows (included inside Driver, after Rows32.inc).
    // Run with: --drawbench --cycles 1 --notests --nosize
    // ════════════════════════════════════════════════════════════════════════

    void drawCase (const juce::String& name, juce::DynamicObject::Ptr opts)
    {
        call ("drawCase", { juce::var (opts.get()) }, "drawCase", [this, name] (const juce::var& r)
        {
            put ("draw33", name, r);
            const auto d = r["draw"];
            std::printf ("%-28s n %3d  median %6.2f  p99 %6.2f  max %6.2f ms  (sections %d, %dx%d dpr %g, cache hit %d miss %d)\n",
                         name.toRawUTF8(), (int) d["n"], (double) d["median"], (double) d["p99"], (double) d["max"],
                         (int) r["sections"], (int) r["geom"]["w"], (int) r["geom"]["h"], (double) r["geom"]["dpr"],
                         (int) r["cacheStats"]["hit"], (int) r["cacheStats"]["miss"]);
            std::fflush (stdout);
        }, 60000.0);
        steps.push_back (sleep (200.0));
    }

    static juce::DynamicObject::Ptr O (std::initializer_list<std::pair<const char*, juce::var>> kv)
    {
        juce::DynamicObject::Ptr o (new juce::DynamicObject());
        for (auto& p : kv) o->setProperty (p.first, p.second);
        return o;
    }

    void heavySetup()
    {
        now ([this]
        {
            neutral();
            for (int b = 1; b <= 24; ++b)
            {
                bandSet (b, 1 + (b % 4), 30.0 * std::pow (600.0, (b - 1) / 23.0), (b % 2 ? 4.0 : -4.0), 1.0, b % 5);
                setIndex (bp (b, "slope"), 7);
            }
        });
        steps.push_back (sleep (500.0));
    }

    void lightSetup()
    {
        now ([this]
        {
            neutral();
            const double fr[] = { 120.0, 600.0, 2500.0, 8000.0 };
            const double gn[] = { 3.0, -4.0, 2.5, -2.0 };
            for (int b = 1; b <= 4; ++b) bandSet (b, 0, fr[b - 1], gn[b - 1], 1.2);
        });
        steps.push_back (sleep (500.0));
    }

    void addDrawBench()
    {
        const bool cacheBuild = cfg.cacheBuild;
        const int N = cfg.drawN;
        steps.push_back (sleep (300.0));
        heavySetup();
        addNative ("draw-size", "resizeEditor", { 1600, 1000 }, [] (const juce::var&) {});
        steps.push_back (sleep (800.0));
        readGeom();

        // ── heavy (E-draw config, 192 sections) ──
        drawCase ("heavy/off",            O ({ { "n", N }, { "analyzer", 0 } }));
        drawCase ("heavy/post-nodata",    O ({ { "n", N }, { "analyzer", 2 } }));     // 3.2's E-draw state
        drawCase ("heavy/post",           O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" } }));
        drawCase ("heavy/prepost",        O ({ { "n", N }, { "analyzer", 3 }, { "inject", "both" } }));
        drawCase ("heavy/post-every2",    O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "injectEvery", 2 }, { "framesPerDraw", 1 } }));
        drawCase ("heavy/post-1f",        O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "framesPerDraw", 1 } }));
        if (cacheBuild)
        {
            drawCase ("heavy/off+cache",      O ({ { "n", N }, { "analyzer", 0 }, { "cache", true } }));
            drawCase ("heavy/post+cache",     O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "cache", true } }));
            drawCase ("heavy/prepost+cache",  O ({ { "n", N }, { "analyzer", 3 }, { "inject", "both" }, { "cache", true } }));
            drawCase ("heavy/post-1f+cache",  O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "framesPerDraw", 1 }, { "cache", true } }));
            drawCase ("heavy/post+pkdrag12",       O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "framesPerDraw", 1 }, { "dragBand", 12 } }));
            drawCase ("heavy/post+pkdrag12+cache", O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "framesPerDraw", 1 }, { "dragBand", 12 }, { "cache", true } }));
        }

        if (cacheBuild)
        {
            // phase breakdown (mean ms per draw per phase; 1 ms timer resolution, so means only)
            drawCase ("heavy/off+phases",          O ({ { "n", N }, { "analyzer", 0 }, { "phases", true } }));
            drawCase ("heavy/post+phases",         O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "phases", true } }));
            drawCase ("heavy/prepost+phases",      O ({ { "n", N }, { "analyzer", 3 }, { "inject", "both" }, { "phases", true } }));
            drawCase ("heavy/post+cache+phases",   O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "cache", true }, { "phases", true } }));
            drawCase ("heavy/post+cache+gradc",    O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "cache", true }, { "gradCache", true } }));
            drawCase ("heavy/post+cache+gradc+ph", O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "cache", true }, { "gradCache", true }, { "phases", true } }));
        }

        // ── real pointer drags on band 12 (relays -> C++ -> eqSections) ──
        for (int pass = 0; pass < (cacheBuild ? 2 : 1); ++pass)
        {
            const bool cache = pass == 1;
            const juce::String sfx = cache ? "+cache" : "";
            steps.push_back ([this, cache, sfx]
            {
                keyed.erase ("dragCase:dragF" + sfx);
                const double x0 = fxP (getS (bp (12, "freq"))), y0 = gyP (getS (bp (12, "gain")));
                js ("window.__bench.dragCase('dragF" + sfx + "'," + juce::String (x0) + "," + juce::String (y0) + ","
                    + juce::String (x0 + 160) + "," + juce::String (y0) + ",150,{cache:" + (cache ? "true" : "false") + "});");
                return true;
            });
            steps.push_back (waitFor ("dragCase:dragF" + sfx, 30000.0, true));
            steps.push_back ([this, sfx]
            {
                const auto r = keyed.count ("dragCase:dragF" + sfx) ? keyed["dragCase:dragF" + sfx].second : juce::var();
                put ("draw33", "heavy/post+freqDrag12" + sfx, r);
                const auto d = r["draw"];
                std::printf ("%-28s n %3d  median %6.2f  p99 %6.2f  max %6.2f ms  (eqSections %d, cache hit %d miss %d)\n",
                             ("heavy/post+freqDrag12" + sfx).toRawUTF8(), (int) d["n"], (double) d["median"], (double) d["p99"], (double) d["max"],
                             (int) r["eqSectionsDuringDrag"], (int) r["cacheStats"]["hit"], (int) r["cacheStats"]["miss"]);
                std::fflush (stdout);
                return true;
            });
            steps.push_back (sleep (400.0));
            steps.push_back ([this, cache, sfx]
            {
                keyed.erase ("dragCase:dragG" + sfx);
                const double x0 = fxP (getS (bp (12, "freq"))), y0 = gyP (getS (bp (12, "gain")));
                js ("window.__bench.dragCase('dragG" + sfx + "'," + juce::String (x0) + "," + juce::String (y0) + ","
                    + juce::String (x0) + "," + juce::String (y0 - 120) + ",150,{cache:" + (cache ? "true" : "false") + "});");
                return true;
            });
            steps.push_back (waitFor ("dragCase:dragG" + sfx, 30000.0, true));
            steps.push_back ([this, sfx]
            {
                const auto r = keyed.count ("dragCase:dragG" + sfx) ? keyed["dragCase:dragG" + sfx].second : juce::var();
                put ("draw33", "heavy/post+gainDrag12" + sfx, r);
                const auto d = r["draw"];
                std::printf ("%-28s n %3d  median %6.2f  p99 %6.2f  max %6.2f ms  (eqSections %d, cache hit %d miss %d)\n",
                             ("heavy/post+gainDrag12" + sfx).toRawUTF8(), (int) d["n"], (double) d["median"], (double) d["p99"], (double) d["max"],
                             (int) r["eqSectionsDuringDrag"], (int) r["cacheStats"]["hit"], (int) r["cacheStats"]["miss"]);
                std::fflush (stdout);
                return true;
            });
            steps.push_back (sleep (400.0));
            heavySetup();    // restore band 12 for the next pass
        }

        // ── listen (halo + wash) ──
        call ("setListen", { 12 }, "done", [this] (const juce::var& r) { put ("draw33", "listenSet", r); });
        drawCase ("heavy/post+listen",        O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" } }));
        if (cacheBuild)
            drawCase ("heavy/post+listen+cache", O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "cache", true } }));
        call ("clearListen", {}, "done", [this] (const juce::var& r) { put ("draw33", "listenCleared", r); });

        // ── light: 4 bells, Post ──
        lightSetup();
        drawCase ("light/off",            O ({ { "n", N }, { "analyzer", 0 } }));
        drawCase ("light/post",           O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" } }));
        drawCase ("light/prepost",        O ({ { "n", N }, { "analyzer", 3 }, { "inject", "both" } }));
        if (cacheBuild)
        {
            drawCase ("light/post+cache",  O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "cache", true } }));
            drawCase ("light/post+phases", O ({ { "n", N }, { "analyzer", 2 }, { "inject", "post" }, { "phases", true } }));
        }

        // ── heavy/off again at the end (drift check) ──
        heavySetup();
        drawCase ("heavy/off-end",        O ({ { "n", N }, { "analyzer", 0 } }));

        now ([this] { neutral(); });
        addNative ("draw-size-back", "resizeEditor", { 960, 600 }, [] (const juce::var&) {});
        steps.push_back (sleep (300.0));
    }
