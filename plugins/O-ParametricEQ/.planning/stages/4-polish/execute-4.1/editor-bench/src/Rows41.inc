    // ════════════════════════════════════════════════════════════════════════
    // PLAN-4.1 T26 rows (included inside Driver, after Rows32.inc / Rows33.inc).
    // RESEARCH-4 Q6 / Q1 definitions:
    //   E-del-listen     latch b, Delete b: setListenBand(0) within one frame (< 20 ms)
    //   E-listen33-aria  latch off, drag band 3: #btn-listen aria-pressed "false" every frame
    //                    (PLAN names it E-listen33; renamed so it does not merge into the
    //                    3.3 row of that name)
    //   E-tipfade        point tip 11.5 px through its fade; .tip-point gone after; a
    //                    re-show mid-fade keeps it
    //   E-tip-lang       language switch while the tip shows: re-measured, inside the window
    //   E-tip-v13        hover tip over the point tip: no data-i18n "bands-full"
    //   E-preset-listen  latch b03; Kick Tighten keeps it; De-Mud ends it
    // ════════════════════════════════════════════════════════════════════════

    static constexpr double kFrameMs41 = 20.0;     // "within one frame" (60 Hz = 16.7 ms, + slack)

    /** Watches processor listenBand from its own thread (0.1 ms) and stamps, on the
        driver's clock, the first moment it reads -1 (the native's arrival). */
    struct ListenOffPoll
    {
        std::atomic<bool> on { false };
        std::atomic<double> tOff { -1.0 };
        std::thread th;
        void start (OuariconParametricEQAudioProcessor& p)
        {
            stop();
            tOff = -1.0; on = true;
            th = std::thread ([this, &p]
            {
                while (on.load())
                {
                    if (tOff.load() < 0 && p.listenBand.load() == -1) tOff = nowMs();
                    std::this_thread::sleep_for (std::chrono::microseconds (100));
                }
            });
        }
        void stop() { on = false; if (th.joinable()) th.join(); }
        ~ListenOffPoll() { stop(); }
    };
    ListenOffPoll listenOffPoll;

    static double fsPx (const juce::var& s) { return s.toString().upToFirstOccurrenceOf ("px", false, false).getDoubleValue(); }

    void addTests41()
    {
        now ([this] { neutral(); feeder.stop(); });
        addNative ("ls41-reset", "setListenBand", { 0 }, [] (const juce::var&) {});
        steps.push_back (sleep (200.0));
        readGeom();

        addDelListenRow();
        addListenAriaRow();
        addTipRows41();
        addPresetListenRow();

        now ([this] { neutral(); });
        addNative ("ls41-reset2", "setListenBand", { 0 }, [] (const juce::var&) {});
        steps.push_back (sleep (300.0));
    }

    // ── E-del-listen ─────────────────────────────────────────────────────────
    void addDelListenRow()
    {
        now ([this]
        {
            neutral();
            bandSet (3, 0, 300.0, 6.0, 1.0); bandSet (9, 0, 1500.0, -4.0, 1.0); bandSet (17, 0, 6000.0, 3.0, 1.0);
        });
        steps.push_back (sleep (300.0));
        auto worst = std::make_shared<std::pair<double, double>> (0.0, 0.0);   // page post, C++ arrival
        for (int b : { 3, 9, 17 })
        {
            latch (b);
            steps.push_back (sleep (100.0));
            auto pre = std::make_shared<int> (-99);
            now ([this, b, pre] { *pre = proc->listenBand.load(); listenOffPoll.start (*proc); });
            auto tag = std::make_shared<juce::String>();
            *tag = call ("delListen", {}, "delListen");
            steps.push_back (sleep (150.0));
            now ([this, b, pre, tag, worst]
            {
                listenOffPoll.stop();
                const auto key = "delListen:" + *tag, startKey = "delStart:" + *tag;
                const auto v = keyed.count (key) ? keyed[key].second : juce::var();
                const double tStart = keyed.count (startKey) ? keyed[startKey].first : -1.0;
                const double tOff = listenOffPoll.tOff.load();
                const double arrival = (tStart >= 0 && tOff >= 0) ? tOff - tStart : -1.0;
                const double dtOff = v.isVoid() ? -1.0 : (double) v["dtOff"];
                const bool enabledAfter = getB (bp (b, "enabled"));
                const int after = proc->listenBand.load();
                const bool pass = *pre == b - 1 && dtOff >= 0 && dtOff < kFrameMs41 && arrival >= 0 && arrival < kFrameMs41
                               && after == -1 && ! enabledAfter;
                worst->first = juce::jmax (worst->first, dtOff); worst->second = juce::jmax (worst->second, arrival);
                auto d = obj(); auto* o = d.getDynamicObject();
                o->setProperty ("listenBefore", *pre);
                o->setProperty ("pagePostMs", dtOff);
                o->setProperty ("nativeArrivalMs", arrival);
                o->setProperty ("listenAfter", after);
                o->setProperty ("enabledAfter", enabledAfter);
                o->setProperty ("posts", v["posts"]);
                verdict ("E-del-listen", "band " + juce::String (b) + ": latched, Delete -> setListenBand(0) < "
                         + juce::String (kFrameMs41, 0) + " ms (page post and C++ arrival)", pass, d);
            });
        }
        now ([this, worst]
        {
            put ("E-del-listen", "maxPagePostMs", worst->first);
            put ("E-del-listen", "maxNativeArrivalMs", worst->second);
        });
    }

    // ── E-listen33-aria (VERIFICATION-3.3 Issue 3) ──────────────────────────
    void addListenAriaRow()
    {
        now ([this] { neutral(); bandSet (3, 0, 300.0, 6.0, 1.0); bandSet (5, 0, 3000.0, -6.0, 1.0); });
        addNative ("ls41-aria", "setListenBand", { 0 }, [] (const juce::var&) {});
        steps.push_back (sleep (300.0));
        readGeom();
        auto a0 = std::make_shared<juce::String>();
        auto during = std::make_shared<int> (-99);
        auto selDuring = std::make_shared<int> (-99);
        auto pre = std::make_shared<int> (-99);
        // Band 3 in the strip with the latch off before the drag. (Without this the
        // previous row's last Delete leaves the strip EMPTY, and the empty strip is
        // never re-rendered, so #btn-listen keeps the stale "true" of the band it
        // last showed latched until a band is selected again. Hidden: the empty
        // strip's cells are visibility:hidden, so it is not an exposed state.)
        call ("clickChip", { 3 }, "chip");
        steps.push_back (sleep (100.0));
        now ([this, pre] { *pre = proc->listenBand.load(); });
        call ("ariaStart", {}, "done", [a0] (const juce::var& v) { *a0 = v["a0"].toString(); });
        listenDrag (3, "up", [this, during, selDuring] (int d) { *during = d; *selDuring = proc->uiSelectedBand.load(); });
        call ("ariaStop", {}, "aria", [this, a0, during, selDuring, pre] (const juce::var& v)
        {
            const int n = v["n"], nTrue = v["nTrue"], nFalse = v["nFalse"];
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("frames", n); o->setProperty ("framesTrue", nTrue); o->setProperty ("framesFalse", nFalse);
            o->setProperty ("distinct", v["distinct"]); o->setProperty ("ariaBefore", *a0);
            o->setProperty ("listenBefore", *pre); o->setProperty ("listenDuringDrag", *during);
            o->setProperty ("selectedDuringDrag", *selDuring);
            // Non-vacuous: the drag really listened to band 3 (listenBand 2) with band 3
            // in the strip, over >= 10 frames; the latch was off before it.
            verdict ("E-listen33-aria", "latch off, drag band 3 past the threshold: aria-pressed \"false\" at every frame",
                     *pre == -1 && *a0 == "false" && *during == 2 && *selDuring == 3 && n >= 10 && nTrue == 0 && nFalse == n, d);
        });
        steps.push_back (sleep (150.0));
        now ([this]
        {
            verdict ("E-listen33-aria", "release: listen off", proc->listenBand.load() == -1, juce::var (proc->listenBand.load()));
        });
    }

    // ── E-tipfade, E-tip-lang, E-tip-v13 (VERIFICATION-3.2 Issues 3 / 5) ────
    void addTipRows41()
    {
        now ([this]
        {
            neutral();
            for (int b = 1; b <= 24; ++b)
                bandSet (b, 0, 30.0 * std::pow (600.0, (b - 1) / 23.0), 0.0, 1.0);
        });
        steps.push_back (sleep (300.0));
        readGeom();
        auto labels = std::make_shared<juce::var>();
        call ("labels", {}, "labels", [labels] (const juce::var& v) { *labels = v["entry"]; });
        auto pt = std::make_shared<std::pair<double, double>>();
        auto ptR = std::make_shared<std::pair<double, double>>();
        now ([this, pt, ptR] { *pt = freeSpot (0.5, 0.2); *ptR = freeSpot (0.985, 0.2); });

        const auto tipCall = [this] (const juce::String& fn, std::shared_ptr<std::pair<double, double>> p,
                                     juce::Array<juce::var> extra, const juce::String& report,
                                     std::function<void (const juce::var&)> then)
        {
            const auto tag = "t41-" + juce::String (jsTag++);
            steps.push_back ([this, fn, p, extra, report, tag]
            {
                keyed.erase (report + ":" + tag);
                juce::StringArray parts { juce::JSON::toString (tag), juce::String (p->first), juce::String (p->second) };
                for (auto& a : extra) parts.add (juce::JSON::toString (a, true));
                js ("window.__bench." + fn + "(" + parts.joinIntoString (",") + ");");
                return true;
            });
            steps.push_back (waitFor (report + ":" + tag, 8000.0, true));
            steps.push_back ([this, report, tag, then] { then (keyed.count (report + ":" + tag) ? keyed[report + ":" + tag].second : juce::var()); return true; });
        };

        // E-tipfade (a) - (c): show, dismiss, every frame through the fade
        tipCall ("tipFade", pt, { juce::var ("fade"), juce::var (0) }, "tipFade", [this] (const juce::var& v)
        {
            const auto shown = v["shown"], after = v["afterDismiss"], later = v["later"];
            double fsMin = 1e9, fsMax = -1e9; int fading = 0, sampled = 0, wrong = 0;
            juce::StringArray fsDuring;
            if (auto* a = v["samples"].getArray())
                for (auto& s : *a)
                {
                    const double op = s["op"];
                    if (op <= 0.001) continue;                  // faded out: the class may go now
                    ++sampled;
                    if (op < 0.999) ++fading;
                    const double fs = fsPx (s["fs"]);
                    fsMin = juce::jmin (fsMin, fs); fsMax = juce::jmax (fsMax, fs);
                    if (std::abs (fs - 11.5) > 1e-6) ++wrong;
                    fsDuring.addIfNotAlreadyThere (s["fs"].toString());
                }
            const double fsDismiss = fsPx (after["fs"]);
            fsMin = juce::jmin (fsMin, fsDismiss); fsMax = juce::jmax (fsMax, fsDismiss);
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("shown", shown); o->setProperty ("afterDismiss", after); o->setProperty ("later", later);
            o->setProperty ("framesWithOpacity", sampled); o->setProperty ("framesMidFade", fading);
            o->setProperty ("fontSizeMinPx", fsMin); o->setProperty ("fontSizeMaxPx", fsMax);
            o->setProperty ("fontSizesSeen", juce::var (fsDuring));
            put ("E-tipfade", "fontSizeMinPx", fsMin); put ("E-tipfade", "fontSizeMaxPx", fsMax);
            put ("E-tipfade", "framesMidFade", fading);
            verdict ("E-tipfade", "(a) shown: .tip-point, 11.5px, bands-full",
                     (bool) shown["point"] && (bool) shown["visible"] && shown["fs"].toString() == "11.5px" && shown["i18n"].toString() == "bands-full", shown);
            // Non-vacuous: >= 2 frames caught mid-fade (0 < opacity < 1), plus the
            // synchronous read right after hideTooltip.
            verdict ("E-tipfade", "(b) 11.5px at every frame of the fade (and at the dismiss itself)",
                     v["mode"].toString() == "fade" && wrong == 0 && fading >= 2 && std::abs (fsDismiss - 11.5) < 1e-6 && (bool) after["point"], d);
            // The class change is visible in size (body 12px), so (b) can fail.
            verdict ("E-tipfade", "(c) after the fade: .tip-point and the label binding gone, box at the hover size",
                     ! (bool) later["point"] && later["i18n"].isVoid() && ! (bool) later["visible"]
                     && std::abs (fsPx (later["fs"]) - 11.5) > 0.2, later);
        });
        // (d) re-show 40 ms into the fade: the pending removal is cancelled
        tipCall ("tipFade", pt, { juce::var ("reshow"), juce::var (40) }, "tipFade", [this] (const juce::var& v)
        {
            const auto mid = v["reshown"]["mid"], later = v["later"];
            const double midOp = mid["op"];
            verdict ("E-tipfade", "(d) re-show mid-fade: .tip-point kept past the fallback timer, 11.5px, bands-full",
                     v["mode"].toString() == "reshow" && (bool) mid["point"] && midOp > 0.0 && midOp < 1.0
                     && (bool) later["point"] && (bool) later["visible"] && later["fs"].toString() == "11.5px"
                     && later["i18n"].toString() == "bands-full", v);
            put ("E-tipfade", "reshowMidOpacity", midOp);
        });
        // (e) no transition (the reduced-motion path): the fallback timer removes it
        tipCall ("tipFade", pt, { juce::var ("notrans"), juce::var (0) }, "tipFade", [this] (const juce::var& v)
        {
            const auto later = v["later"];
            verdict ("E-tipfade", "(e) transition none (reduced-motion path): .tip-point gone by the fade + 50 ms timer",
                     v["mode"].toString() == "notrans" && (double) v["afterDismiss"]["op"] == 0.0   // the transition really was off
                     && (bool) v["afterDismiss"]["point"] && ! (bool) later["point"] && later["i18n"].isVoid(), v);
        });

        // E-tip-lang: centre and right edge, fr and zh-Hans
        for (auto* which : { "centre", "right edge" })
            for (auto* lang : { "fr", "zh-Hans" })
            {
                const juce::String w (which), l (lang);
                tipCall ("tipLang", w == "centre" ? pt : ptR, { juce::var (l) }, "tipLang", [this, w, l, labels] (const juce::var& v)
                {
                    const auto b = v["before"], a = v["after"];
                    const auto want = (*labels)[juce::Identifier (l)]["t"].toString();
                    const double left = a["l"], wd = a["w"], vw = a["vw"], top = a["t"], bot = a["b"], vh = a["vh"];
                    const bool inside = left >= 0 && left + wd <= vw + 0.01 && top >= 0 && bot <= vh + 0.01;
                    const bool remeasured = std::abs ((double) a["w"] - (double) b["w"]) > 0.5;
                    auto d = obj(); auto* o = d.getDynamicObject();
                    o->setProperty ("before", b); o->setProperty ("after", a); o->setProperty ("want", want);
                    o->setProperty ("widthEn", b["w"]); o->setProperty ("widthAfter", a["w"]);
                    o->setProperty ("rightEdge", left + wd); o->setProperty ("innerWidth", vw);
                    verdict ("E-tip-lang", w + ", en -> " + l + ": text re-localized, width re-measured, inside the window",
                             (bool) b["point"] && (bool) b["visible"] && (bool) a["point"] && a["lang"].toString() == l
                             && want.isNotEmpty() && a["text"].toString() == want && remeasured && inside, d);
                });
            }

        // E-tip-v13: hover a control while the point tip is up
        tipCall ("tipV13", pt, { juce::var ("#toggle-auto_gain") }, "tipV13", [this, labels] (const juce::var& v)
        {
            const auto p = v["point"], h = v["hover"], r = v["reapplied"], want = v["want"];
            const auto fullEn = (*labels)["en"]["t"].toString();
            const auto body = want["body"].toString();
            const bool pre = v["tipsOn"].toString() == "true" && (bool) v["found"] && (bool) p["point"] && (bool) p["visible"]
                          && p["i18n"].toString() == "bands-full";
            const bool hoverOk = (bool) h["visible"] && ! (bool) h["point"] && h["i18n"].toString() != "bands-full"
                              && body.isNotEmpty() && h["text"].toString().contains (body) && ! h["text"].toString().contains (fullEn);
            const bool reapplyOk = r["i18n"].toString() != "bands-full" && r["text"].toString() == h["text"].toString();
            verdict ("E-tip-v13", "hover tip over the point tip: no data-i18n bands-full, its own text, survives a re-apply",
                     pre && hoverOk && reapplyOk, v);
        });
        now ([this] { neutral(); });
        steps.push_back (sleep (200.0));
    }

    // ── E-preset-listen (RESEARCH-4 Q1): real factory loads ─────────────────
    void addPresetListenRow()
    {
        now ([this] { neutral(); bandSet (3, 0, 380.0, -5.0, 1.8); });
        steps.push_back (sleep (300.0));
        latch (3);
        now ([this]
        {
            verdict ("E-preset-listen", "(pre) latched b03: listenBand 2", proc->listenBand.load() == 2, juce::var (proc->listenBand.load()));
        });
        auto ok = std::make_shared<bool> (false);
        auto passes = std::make_shared<int> (0);
        now ([this, ok] { listenAll(); *ok = proc->presetManager.loadPreset ("Kick Tighten"); });
        steps.push_back (sleep (400.0));
        now ([this, ok, passes]
        {
            *passes = gv (bp (3, "enabled"));
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("loadOk", *ok); o->setProperty ("listenBand", proc->listenBand.load());
            o->setProperty ("b03Enabled", getB (bp (3, "enabled"))); o->setProperty ("b03EnabledValueChanges", *passes);
            o->setProperty ("b03Freq", getS (bp (3, "freq"))); o->setProperty ("presetName", proc->presetManager.getCurrentPresetName());
            verdict ("E-preset-listen", "Kick Tighten (b03 stays on): listen holds 400 ms after the load (> 80 ms settle)",
                     *ok && proc->listenBand.load() == 2 && getB (bp (3, "enabled")) && std::abs (getS (bp (3, "freq")) - 380.0) < 0.5, d);
        });
        steps.push_back (sleep (600.0));
        now ([this]
        {
            verdict ("E-preset-listen", "Kick Tighten: still holding at 1 s", proc->listenBand.load() == 2, juce::var (proc->listenBand.load()));
            unlistenAll();
        });
        now ([this, ok] { *ok = proc->presetManager.loadPreset ("De-Mud"); });
        steps.push_back (sleep (400.0));
        now ([this, ok]
        {
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("loadOk", *ok); o->setProperty ("listenBand", proc->listenBand.load());
            o->setProperty ("b03Enabled", getB (bp (3, "enabled")));
            o->setProperty ("presetName", proc->presetManager.getCurrentPresetName());
            verdict ("E-preset-listen", "De-Mud (b03 off): listen ends",
                     *ok && proc->listenBand.load() == -1 && ! getB (bp (3, "enabled")), d);
        });
    }
