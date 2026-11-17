    // ════════════════════════════════════════════════════════════════════════
    // PLAN-3.2 T10 rows (included inside Driver). The real editor and page; the
    // bench never calls processBlock, so every row runs with the transport
    // stopped by construction.
    // ════════════════════════════════════════════════════════════════════════

    static constexpr double kMl = 38, kMr = 14, kMt = 12, kMb = 22;   // PLOT_MARGIN (index.html)
    static constexpr double kTickMs = 1000.0 / 60.0;

    struct Geom { double plotW = 0, plotH = 0, range = 12, w = 0, h = 0, vw = 0, vh = 0; };
    Geom geom;

    double fxP (double f) const { return kMl + (std::log (f) - std::log (10.0)) / std::log (3000.0) * geom.plotW; }
    double xfP (double x) const { return std::exp (std::log (10.0) + juce::jlimit (0.0, 1.0, (x - kMl) / geom.plotW) * std::log (3000.0)); }
    double gyP (double db) const { return kMt + (geom.range - db) / (2.0 * geom.range) * geom.plotH; }
    double ygP (double y) const { return geom.range - juce::jlimit (0.0, 1.0, (y - kMt) / geom.plotH) * 2.0 * geom.range; }

    static Geom geomFrom (const juce::var& g)
    {
        Geom o;
        o.plotW = g["plotW"]; o.plotH = g["plotH"]; o.w = g["w"]; o.h = g["h"]; o.vw = g["vw"]; o.vh = g["vh"];
        o.range = g["range"].isVoid() ? 12.0 : (double) g["range"];
        return o;
    }

    // ── host-side helpers ───────────────────────────────────────────────────
    void setS (const juce::String& id, double v) { auto* p = param (id); p->setValueNotifyingHost (p->convertTo0to1 ((float) v)); }
    double getS (const juce::String& id)         { auto* p = param (id); return p->convertFrom0to1 (p->getValue()); }
    void setB (const juce::String& id, bool v)   { param (id)->setValueNotifyingHost (v ? 1.0f : 0.0f); }
    bool getB (const juce::String& id)           { return param (id)->getValue() >= 0.5f; }
    int  getI (const juce::String& id)           { auto* p = param (id); return juce::roundToInt (p->convertFrom0to1 (p->getValue())); }
    static juce::String bp (int b, const char* k) { return opeq::bandPrefix (b - 1) + k; }

    void bandSet (int b, int type, double hz, double db, double q, int placement = 0)
    {
        setB (bp (b, "enabled"), true); setB (bp (b, "bypass"), false);
        setIndex (bp (b, "type"), type); setIndex (bp (b, "placement"), placement);
        setS (bp (b, "freq"), hz); setS (bp (b, "gain"), db); setS (bp (b, "q"), q);
    }

    /** Every parameter to its default, every band off, gain scale 100 %. */
    void neutral()
    {
        for (auto* p : params) p->setValueNotifyingHost (p->getDefaultValue());
        for (int b = 1; b <= 24; ++b) setB (bp (b, "enabled"), false);
        setS ("gain_scale", 100.0); setB ("bypass", false);
    }

    void listenAll()   { gestures->reset(); for (auto* p : params) p->addListener (gestures.get()); }
    void unlistenAll() { for (auto* p : params) p->removeListener (gestures.get()); }
    int  gb (const juce::String& id) { return gestures->begins[(size_t) param (id)->getParameterIndex()]; }
    int  ge (const juce::String& id) { return gestures->ends[(size_t) param (id)->getParameterIndex()]; }
    int  gv (const juce::String& id) { return gestures->values[(size_t) param (id)->getParameterIndex()]; }
    int  gesturesOutside (const juce::StringArray& ids)
    {
        int n = 0;
        for (auto* p : params)
            if (! ids.contains (p->getParameterID()))
                n += gestures->begins[(size_t) p->getParameterIndex()] + gestures->ends[(size_t) p->getParameterIndex()];
        return n;
    }

    // ── JS call: push the call, wait for its keyed report, run `then` ───────
    int jsTag = 0;
    juce::String call (const juce::String& fn, juce::Array<juce::var> args, const juce::String& reportType,
                       std::function<void (const juce::var&)> then = {}, double timeoutMs = 8000.0)
    {
        const auto tag = "j" + juce::String (jsTag++);
        juce::Array<juce::var> full { tag };
        full.addArray (args);
        steps.push_back ([this, fn, full, reportType, tag]
        {
            keyed.erase (reportType + ":" + tag);
            juce::StringArray parts;
            for (auto& a : full) parts.add (juce::JSON::toString (a, true));
            js ("window.__bench." + fn + "(" + parts.joinIntoString (",") + ");");
            return true;
        });
        steps.push_back (waitFor (reportType + ":" + tag, timeoutMs, true));
        steps.push_back ([this, reportType, tag, then]
        {
            const auto key = reportType + ":" + tag;
            if (then) then (keyed.count (key) ? keyed[key].second : juce::var());
            return true;
        });
        return tag;
    }
    void now (std::function<void()> f) { steps.push_back ([f] { f(); return true; }); }

    void readGeom() { call ("geom", {}, "geom", [this] (const juce::var& g) { geom = geomFrom (g); }); }

    juce::DynamicObject* row (const juce::String& name) { return getRow (name).getDynamicObject(); }
    void put (const juce::String& r, const juce::String& k, const juce::var& v) { row (r)->setProperty (k, v); }
    void verdict (const juce::String& r, const juce::String& check, bool pass, const juce::var& detail = {})
    {
        auto o = obj();
        o.getDynamicObject()->setProperty ("pass", pass);
        if (! detail.isVoid()) o.getDynamicObject()->setProperty ("detail", detail);
        row (r)->setProperty (check, o);
        if (! pass) failedChecks.add (r + " / " + check);
    }

    // ── the reference curve: an independent C++ closed form over a bench-
    // owned DisplayModel's packets (the processor's own designs) ────────────
    std::unique_ptr<DisplayModel> refModel;

    static std::complex<double> bandResponse (const juce::var& pk, double w)
    {
        using C = std::complex<double>;
        C h (1.0, 0.0);
        const double t0 = std::tan (w / 2.0);
        if (auto* s = pk["sections"].getArray())
            for (auto& sec : *s)
            {
                const double g = sec[0], k = sec[1], mL = sec[2], mB = sec[3], mH = sec[4];
                const C x (0.0, t0 / g);
                h *= (mL + mB * x + mH * x * x) / (1.0 + k * x + x * x);
            }
        if (auto* o = pk["onePoles"].getArray())
            for (auto& op : *o)
            {
                const double G = op[0], cL = op[1], cH = op[2];
                const C x (0.0, t0 / G);
                h *= (cL + cH * x) / (1.0 + x);
            }
        return h;
    }

    /** dB of the summed curve the page should draw at f. placementOverride:
        relay-style placement per band (1-based) to compute the WRONG curve. */
    double refDb (double f, int view, const std::map<int, int>* placementOverride = nullptr)
    {
        using C = std::complex<double>;
        if (getB ("bypass")) return 0.0;
        const auto packets = refModel->allPackets();
        const double fs = refModel->getSampleRate();
        const double w = 2.0 * juce::MathConstants<double>::pi * f / fs;
        C a (1, 0), b (0, 0), c (0, 0), d (1, 0);
        for (int i = 0; i < packets.size(); ++i)
        {
            const auto pk = packets[i];
            if (! (bool) pk["active"]) continue;
            const C F = bandResponse (pk, w), hh = (F - 1.0) / 2.0;
            int pl = (int) pk["placement"];
            if (placementOverride != nullptr && placementOverride->count (i + 1)) pl = placementOverride->at (i + 1);
            C p, q, r, s;
            switch (pl)
            {
                case 1:  p = F; q = 0; r = 0; s = 1; break;
                case 2:  p = 1; q = 0; r = 0; s = F; break;
                case 3:  p = 1.0 + hh; q = hh; r = hh; s = 1.0 + hh; break;
                case 4:  p = 1.0 + hh; q = -hh; r = -hh; s = 1.0 + hh; break;
                default: p = F; q = 0; r = 0; s = F;
            }
            const C na = p * a + q * c, nb = p * b + q * d, nc = r * a + s * c, nd = r * b + s * d;
            a = na; b = nb; c = nc; d = nd;
        }
        C z = a;
        if (view == 1) z = d;
        else if (view == 2) z = (a + b + c + d) / 2.0;
        else if (view == 3) z = (a - b - c + d) / 2.0;
        const double m2 = std::norm (z);
        return m2 > 1e-20 ? 10.0 * std::log10 (m2) : -200.0;
    }

    int refView()
    {
        const int s = proc->uiSelectedBand.load();
        if (s < 1 || s > 24 || ! getB (bp (s, "enabled"))) return 0;
        const int pl = (int) refModel->allPackets()[s - 1]["placement"];
        static const int map[] = { 0, 0, 1, 2, 3 };
        return map[juce::jlimit (0, 4, pl)];
    }

    /** Probe columns: `n` x positions spread over the plot, none within 18 px
        of an enabled node. */
    juce::Array<juce::var> probeXs (int n = 9)
    {
        juce::Array<juce::var> xs;
        for (int i = 0; i < n * 4 && xs.size() < n; ++i)
        {
            const double x = std::round (kMl + 10 + (geom.plotW - 20) * (0.03 + 0.94 * ((i * 0.618034) - std::floor (i * 0.618034))));
            bool clear = true;
            for (int b = 1; b <= 24; ++b)
                if (getB (bp (b, "enabled")) && std::abs (fxP (juce::jlimit (10.0, 30000.0, getS (bp (b, "freq")))) - x) < 18.0) clear = false;
            if (clear) xs.add (x);
        }
        return xs;
    }

    /** Checks a probe report against the reference curve. Returns the detail
        object; `pass` = >= 3 columns in the plot and every one of them inside
        an ink run (+-2.5 px). */
    juce::var checkProbe (const juce::var& rep, const std::map<int, int>* wrong = nullptr)
    {
        geom = geomFrom (rep["geom"]);
        refModel->poll();
        const int view = refView();
        int valid = 0, ok = 0, wrongDiffers = 0;
        juce::Array<juce::var> cols;
        if (auto* arr = rep["cols"].getArray())
            for (auto& col : *arr)
            {
                const double x = col["x"], f = xfP (x);
                const double y = gyP (refDb (f, view));
                auto o = obj(); auto* d = o.getDynamicObject();
                d->setProperty ("x", x); d->setProperty ("f", f); d->setProperty ("expY", y);
                d->setProperty ("runs", col["runs"]);
                if (y < kMt + 3 || y > kMt + geom.plotH - 3) { d->setProperty ("skip", true); cols.add (o); continue; }
                if (getB ("bypass") && col["runs"].size() == 0) { d->setProperty ("skip", "dash gap"); cols.add (o); continue; }
                ++valid;
                bool hit = false;
                if (auto* runs = col["runs"].getArray())
                    for (auto& r : *runs)
                        if (y >= (double) r[0] - 2.5 && y <= (double) r[1] + 2.5) hit = true;
                d->setProperty ("hit", hit);
                ok += hit ? 1 : 0;
                if (wrong != nullptr)
                {
                    const double yw = gyP (juce::jlimit (-geom.range * 3, geom.range * 3, refDb (f, view, wrong)));
                    d->setProperty ("wrongY", yw);
                    if (std::abs (yw - y) > 6.0) ++wrongDiffers;
                }
                cols.add (o);
            }
        auto o = obj(); auto* d = o.getDynamicObject();
        d->setProperty ("valid", valid); d->setProperty ("hits", ok); d->setProperty ("view", view);
        d->setProperty ("pass", valid >= 3 && ok == valid);
        if (wrong != nullptr) d->setProperty ("wrongDiffers", wrongDiffers);
        d->setProperty ("cols", cols);
        return o;
    }

    void probe (const juce::String& r, const juce::String& check, const std::map<int, int>* wrong = nullptr,
                std::function<void (const juce::var&)> extra = {})
    {
        auto xs = std::make_shared<juce::Array<juce::var>>();
        now ([this, xs] { *xs = probeXs(); });
        // The xs are filled when the step runs; the call reads them then.
        const auto tag = "j" + juce::String (jsTag++);
        steps.push_back ([this, xs, tag]
        {
            keyed.erase ("probe:" + tag);
            js ("window.__bench.curveProbe(" + juce::JSON::toString (tag) + "," + juce::JSON::toString (juce::var (*xs), true) + ");");
            return true;
        });
        steps.push_back (waitFor ("probe:" + tag, 5000.0, true));
        std::shared_ptr<std::map<int, int>> wrongCopy;
        if (wrong != nullptr) wrongCopy = std::make_shared<std::map<int, int>> (*wrong);
        steps.push_back ([this, r, check, tag, wrongCopy, extra]
        {
            const auto rep = keyed.count ("probe:" + tag) ? keyed["probe:" + tag].second : juce::var();
            const auto res = checkProbe (rep, wrongCopy.get());
            bool pass = (bool) res["pass"];
            if (wrongCopy) pass = pass && (int) res["wrongDiffers"] >= 1;
            verdict (r, check, pass, res);
            if (extra) extra (res);
            return true;
        });
    }

    // ── C++-side eqSections emissions (bench hook) ──────────────────────────
    struct Emit { double t; juce::Array<int> bands; };
    std::vector<Emit> emits;            // since the editor was opened
    std::vector<std::pair<int, double>> eqPaints;   // (JS eq idx, C++ receipt ms)
    std::vector<double> lagFreqChanges; // C++ ms of the lag band's freq changes
    int lagParamIndex = -1;

    juce::Array<juce::var> bandsVar (const juce::Array<int>& b) { juce::Array<juce::var> a; for (int x : b) a.add (x); return a; }

    // ════════════════════════════════════════════════════════════════════════
    void addTests32()
    {
        steps.push_back (sleep (200.0));
        addReloadRow();
        readGeom();

        addCreateAndFull();
        addDragRows();
        addWheelRows();
        addDblClickRows();
        addDeleteRows();
        addEndsRows();
        addStoppedRow();
        addMonoRow();
        addLagRow();
        addDrawRow();

        now ([this] { neutral(); });
        steps.push_back (sleep (200.0));
    }

    // ── E-eq-reload: the page reloads inside a live editor (nothing changed in
    // the processor). Only requestEqSections' full-push flag can repaint it:
    // poll() reports no change. ─────────────────────────────────────────────
    void addReloadRow()
    {
        auto at = std::make_shared<size_t> (0);
        // The reloaded page counts its eqSections from 0 again: restart the C++
        // list with it, so the E-lag pairing (k-th send <-> k-th arrival) holds.
        now ([this, at] { emits.clear(); *at = 0; events.erase ("pageReady"); js ("location.reload();"); });
        steps.push_back (waitFor ("pageReady", 15000.0));
        steps.push_back (sleep (250.0));
        steps.push_back ([this]
        {
            keyed.erase ("eqSince:reload");
            js ("window.__bench.eqSince('reload',0);");
            return true;
        });
        steps.push_back (waitFor ("eqSince:reload", 4000.0, true));
        now ([this, at]
        {
            const auto list = keyed.count ("eqSince:reload") ? keyed["eqSince:reload"].second["list"] : juce::var();
            const int n = list.size() > 0 ? (int) list[0]["n"] : 0;
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("eventsOnNewPage", list.size()); o->setProperty ("firstN", n);
            o->setProperty ("cppEmitsSinceReload", (int) (emits.size() - *at));
            verdict ("E-eq-reload", "a reloaded page gets all 24 bands", n == 24, d);
        });
    }

    // ── E-create (UI-01) and E-full (U5) ────────────────────────────────────
    std::pair<double, double> freeSpot (double fx0, double fy0)
    {
        // plot-local point near (fx0, fy0) of the plot box, >= 24 px from every enabled node
        for (int k = 0; k < 400; ++k)
        {
            const double x = kMl + geom.plotW * juce::jlimit (0.02, 0.98, fx0 + 0.013 * (k % 20) * ((k / 20) % 2 ? -1 : 1));
            const double y = kMt + geom.plotH * juce::jlimit (0.03, 0.97, fy0 + 0.011 * (k / 20) * ((k / 40) % 2 ? -1 : 1));
            bool clear = true;
            for (int b = 1; b <= 24; ++b)
            {
                if (! getB (bp (b, "enabled"))) continue;
                const int t = getI (bp (b, "type"));
                const bool hasGain = t == 0 || t == 1 || t == 2 || t == 7;
                const double nx = fxP (juce::jlimit (10.0, 30000.0, getS (bp (b, "freq"))));
                const double ny = hasGain ? juce::jlimit (kMt, kMt + geom.plotH, gyP (getS (bp (b, "gain")) * getS ("gain_scale") / 100.0)) : gyP (0);
                if (std::hypot (nx - x, ny - y) < 24.0) clear = false;
            }
            if (clear) return { x, y };
        }
        return { kMl + geom.plotW * fx0, kMt + geom.plotH * fy0 };
    }

    void addCreateAndFull()
    {
        auto pt = std::make_shared<std::pair<double, double>>();
        now ([this] { neutral(); setIndex (bp (1, "slope"), 3); setS (bp (1, "q"), 7.0); });
        steps.push_back (sleep (200.0));
        readGeom();
        now ([this, pt] { *pt = { kMl + 0.40 * geom.plotW, kMt + 0.30 * geom.plotH }; listenAll(); });
        steps.push_back ([this, pt]
        {
            keyed.erase ("done:create1");
            js ("window.__bench.click('create1'," + juce::String (pt->first) + "," + juce::String (pt->second) + ");");
            return true;
        });
        steps.push_back (waitFor ("done:create1", 4000.0, true));
        steps.push_back (sleep (200.0));
        now ([this, pt]
        {
            unlistenAll();
            const double expF = xfP (pt->first), expG = std::round (ygP (pt->second) * 10.0) / 10.0;
            const double qDef = param (bp (1, "q"))->convertFrom0to1 (param (bp (1, "q"))->getDefaultValue());
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("expF", expF); o->setProperty ("hostF", getS (bp (1, "freq")));
            o->setProperty ("expG", expG); o->setProperty ("hostG", getS (bp (1, "gain")));
            o->setProperty ("qDef", qDef); o->setProperty ("hostQ", getS (bp (1, "q")));
            o->setProperty ("type", getI (bp (1, "type"))); o->setProperty ("placement", getI (bp (1, "placement")));
            o->setProperty ("bypass", getB (bp (1, "bypass"))); o->setProperty ("enabled", getB (bp (1, "enabled")));
            o->setProperty ("slope", getI (bp (1, "slope")));
            juce::StringArray band1;
            for (auto k : { "enabled", "bypass", "type", "freq", "gain", "q", "slope", "placement" }) band1.add (bp (1, k));
            o->setProperty ("gestures freq/gain/q", juce::String (gb (bp (1, "freq"))) + "/" + juce::String (ge (bp (1, "freq"))) + " "
                                                   + juce::String (gb (bp (1, "gain"))) + "/" + juce::String (ge (bp (1, "gain"))) + " "
                                                   + juce::String (gb (bp (1, "q"))) + "/" + juce::String (ge (bp (1, "q"))));
            o->setProperty ("gesturesOutsideBand1", gesturesOutside (band1));
            const bool pass = getB (bp (1, "enabled")) && getI (bp (1, "type")) == 0 && getI (bp (1, "placement")) == 0
                           && ! getB (bp (1, "bypass")) && getI (bp (1, "slope")) == 3
                           && std::abs (getS (bp (1, "freq")) / expF - 1.0) < 1e-4
                           && std::abs (getS (bp (1, "gain")) - expG) < 1e-3
                           && std::abs (getS (bp (1, "q")) / qDef - 1.0) < 1e-5
                           && gb (bp (1, "freq")) == 1 && ge (bp (1, "freq")) == 1
                           && gb (bp (1, "gain")) == 1 && ge (bp (1, "gain")) == 1
                           && gb (bp (1, "q")) == 1 && ge (bp (1, "q")) == 1
                           && gesturesOutside (band1) == 0;
            verdict ("E-create", "click creates band 1 there", pass, d);

            for (int b = 2; b <= 23; ++b) setB (bp (b, "enabled"), true);
        });
        steps.push_back (sleep (200.0));
        now ([this, pt] { *pt = freeSpot (0.62, 0.70); });
        steps.push_back ([this, pt]
        {
            keyed.erase ("done:create24");
            js ("window.__bench.click('create24'," + juce::String (pt->first) + "," + juce::String (pt->second) + ");");
            return true;
        });
        steps.push_back (waitFor ("done:create24", 4000.0, true));
        steps.push_back (sleep (200.0));
        now ([this]
        {
            verdict ("E-create", "with 1 - 23 enabled the click creates band 24", getB (bp (24, "enabled")) && getI (bp (24, "type")) == 0,
                     juce::var ("b24 enabled " + juce::String ((int) getB (bp (24, "enabled")))));
        });

        // E-full: all 24 in use
        auto labels = std::make_shared<juce::var>();
        call ("labels", {}, "labels", [labels] (const juce::var& v) { *labels = v["entry"]; });
        const auto fullClick = [this, pt] (const juce::String& name, double fx0, double fy0, const juce::String& lang)
        {
            now ([this, pt, fx0, fy0] { *pt = freeSpot (fx0, fy0); listenAll(); });
            steps.push_back ([this, pt, name]
            {
                keyed.erase ("done:" + name);
                js ("window.__bench.click('" + name + "'," + juce::String (pt->first) + "," + juce::String (pt->second) + ");");
                return true;
            });
            steps.push_back (waitFor ("done:" + name, 4000.0, true));
            call ("tip", {}, "tip", [this, pt, name, lang] (const juce::var& t)
            {
                unlistenAll();
                int g = 0, v = 0;
                for (size_t i = 0; i < gestures->begins.size(); ++i) { g += gestures->begins[i] + gestures->ends[i]; v += gestures->values[i]; }
                const auto r = t["rect"];
                const double cx = geom.vw > 0 ? 0 : 0; juce::ignoreUnused (cx);
                const bool inside = (double) r["l"] >= 0 && (double) r["t"] >= 0
                                 && (double) r["r"] <= (double) t["vw"] && (double) r["b"] <= (double) t["vh"];
                auto d = obj(); auto* o = d.getDynamicObject();
                o->setProperty ("tip", t); o->setProperty ("gestures", g); o->setProperty ("valueChanges", v);
                o->setProperty ("click", juce::String (pt->first, 1) + "," + juce::String (pt->second, 1));
                o->setProperty ("inside", inside);
                verdict ("E-full", name, (bool) t["visible"] && g == 0 && v == 0 && inside
                                         && t["text"].toString().isNotEmpty() && t["i18n"].toString() == "bands-full", d);
            });
        };
        now ([this] { setB (bp (24, "enabled"), true); });
        steps.push_back (sleep (150.0));
        fullClick ("en: refused click shows the tip", 0.5, 0.5, "en");
        call ("tip", {}, "tip", [this, labels] (const juce::var& t)
        {
            const auto en = (*labels)["en"]["t"].toString();
            verdict ("E-full", "en text is LABELS bands-full en", t["text"].toString() == en && en == "All 24 bands are in use",
                     juce::var (t["text"].toString() + " | " + en));
        });
        call ("pressElsewhere", {}, "done");
        call ("tip", {}, "tip", [this] (const juce::var& t)
        {
            verdict ("E-full", "hidden by the next pointerdown", ! (bool) t["visible"], t);
        });
        fullClick ("bottom-right edge: tip stays inside the window", 0.98, 0.95, "en");
        call ("pressElsewhere", {}, "done");
        // hover help off
        call ("clickEl", { "#tips-toggle" }, "done");
        fullClick ("hover help off: tip still shows", 0.3, 0.6, "en");
        call ("tip", {}, "tip", [this] (const juce::var& t)
        {
            verdict ("E-full", "hover help was off for that click", t["tipsOn"].toString() == "false", juce::var (t["tipsOn"].toString()));
        });
        call ("pressElsewhere", {}, "done");
        call ("clickEl", { "#tips-toggle" }, "done");
        for (auto lang : { "fr", "zh-Hans" })
        {
            const juce::String l (lang);
            call ("lang", { l }, "done");
            fullClick (l + ": refused click shows the tip", 0.45, 0.4, l);
            call ("tip", {}, "tip", [this, labels, l] (const juce::var& t)
            {
                const auto want = (*labels)[juce::Identifier (l)]["t"].toString();
                verdict ("E-full", l + " text is that language's entry", want.isNotEmpty() && t["text"].toString() == want,
                         juce::var (t["text"].toString() + " | " + want));
            });
            call ("pressElsewhere", {}, "done");
        }
        call ("lang", { "en" }, "done");
    }

    // ── E-drag ──────────────────────────────────────────────────────────────
    void dragRow (const juce::String& name, int type, double scalePct, const juce::String& end,
                  int expFreqPairs, int expGainPairs, bool expGainMoves, bool pauseAndChangeType = false)
    {
        auto xy = std::make_shared<std::array<double, 4>>();
        auto g0 = std::make_shared<double> (0.0);
        now ([this, type, scalePct]
        {
            neutral(); bandSet (2, type, 1000.0, 6.0, 1.0); setS ("gain_scale", scalePct);
        });
        steps.push_back (sleep (150.0));
        call ("clickChip", { 2 }, "chip");
        readGeom();
        now ([this, xy, g0, type, scalePct]
        {
            const bool hasGain = type == 0 || type == 1 || type == 2 || type == 7;
            const double x0 = fxP (1000.0), y0 = hasGain ? gyP (6.0 * scalePct / 100.0) : gyP (0.0);
            *xy = { x0, y0, x0 + 80.0, y0 - 40.0 };
            *g0 = getS (bp (2, "gain"));
            listenAll();
        });
        const auto tag = "drag" + juce::String (jsTag++);
        steps.push_back ([this, xy, tag, end, pauseAndChangeType]
        {
            keyed.erase ("done:" + tag); keyed.erase ("paused:" + tag);
            const auto a = juce::String ((*xy)[0]) + "," + juce::String ((*xy)[1]) + "," + juce::String ((*xy)[2]) + "," + juce::String ((*xy)[3]);
            if (pauseAndChangeType) js ("window.__bench.dragPause('" + tag + "'," + a + ",10,16);");
            else                    js ("window.__bench.drag('" + tag + "'," + a + ",10,16,'" + end + "');");
            return true;
        });
        if (pauseAndChangeType)
        {
            steps.push_back (waitFor ("paused:" + tag, 4000.0, true));
            now ([this] { setIndex (bp (2, "type"), 3); });   // automation: Bell -> Low Cut mid-drag
            steps.push_back (sleep (60.0));
            now ([this] { js ("window.__bench.resume();"); });
        }
        steps.push_back (waitFor ("done:" + tag, 5000.0, true));
        steps.push_back (sleep (200.0));
        now ([this, xy, g0, name, expFreqPairs, expGainPairs, expGainMoves, scalePct, pauseAndChangeType]
        {
            unlistenAll();
            const auto f = bp (2, "freq"), g = bp (2, "gain");
            const double expF = xfP ((*xy)[2]);
            const double expG = scalePct > 1 ? ygP ((*xy)[3]) / (scalePct / 100.0) : *g0;
            juce::StringArray mine { f, g };
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("freq b/e", juce::String (gb (f)) + "/" + juce::String (ge (f)));
            o->setProperty ("gain b/e", juce::String (gb (g)) + "/" + juce::String (ge (g)));
            o->setProperty ("gainValueChanges", gv (g));
            o->setProperty ("outside", gesturesOutside (mine));
            o->setProperty ("expF", expF); o->setProperty ("hostF", getS (f));
            o->setProperty ("g0", *g0); o->setProperty ("expG", expG); o->setProperty ("hostG", getS (g));
            bool pass = gb (f) == expFreqPairs && ge (f) == expFreqPairs && gb (g) == expGainPairs && ge (g) == expGainPairs
                     && gesturesOutside (mine) == 0;
            if (! pauseAndChangeType)
            {
                pass = pass && std::abs (getS (f) / expF - 1.0) < 2e-4;
                pass = pass && (expGainMoves ? std::abs (getS (g) - expG) < 2e-3 : (gv (g) == 0 && getS (g) == *g0));
            }
            verdict ("E-drag", name, pass, d);
        });
    }

    void addDragRows()
    {
        dragRow ("Bell: one pair on freq and on gain, values follow", 0, 100.0, "up", 1, 1, true);
        dragRow ("Low Cut: one pair on freq, none on gain", 3, 100.0, "up", 1, 0, false);
        dragRow ("Bell at gain scale 0 %: none on gain", 0, 0.0, "up", 1, 0, false);
        dragRow ("Bell, pointercancel closes both pairs", 0, 100.0, "cancel", 1, 1, true);
        dragRow ("Bell, lostpointercapture closes both pairs", 0, 100.0, "lost", 1, 1, true);
        dragRow ("Bell -> Low Cut by automation mid-drag: pairs stay balanced", 0, 100.0, "up", 1, 1, true, true);
    }

    // ── E-wheel ─────────────────────────────────────────────────────────────
    void wheelRow (const juce::String& name, bool shift, double expLn)
    {
        auto xy = std::make_shared<std::pair<double, double>>();
        now ([this] { neutral(); bandSet (2, 0, 1000.0, 6.0, 1.0); });
        steps.push_back (sleep (150.0));
        readGeom();
        now ([this, xy] { *xy = { fxP (1000.0), gyP (6.0) }; listenAll(); });
        const auto tag = "wh" + juce::String (jsTag++);
        steps.push_back ([this, xy, tag, shift]
        {
            keyed.erase ("done:" + tag);
            js ("window.__bench.wheel('" + tag + "'," + juce::String (xy->first) + "," + juce::String (xy->second) + ",10,-100,"
                + (shift ? "true" : "false") + ",20);");
            return true;
        });
        steps.push_back (waitFor ("done:" + tag, 4000.0, true));
        steps.push_back (sleep (450.0));   // the 250 ms burst timer closes the gesture
        now ([this, name, expLn]
        {
            unlistenAll();
            const auto q = bp (2, "q");
            const double ln = std::log (getS (q) / 1.0);
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("q b/e", juce::String (gb (q)) + "/" + juce::String (ge (q)));
            o->setProperty ("lnRatio", ln); o->setProperty ("expLn", expLn); o->setProperty ("outside", gesturesOutside ({ q }));
            verdict ("E-wheel", name, gb (q) == 1 && ge (q) == 1 && std::abs (ln - expLn) < 2e-3 && gesturesOutside ({ q }) == 0, d);
        });
    }

    void addWheelRows()
    {
        wheelRow ("10 events, one burst: one pair on q, log domain", false, 10 * 100 * 0.0025);
        wheelRow ("Shift: finer", true, 10 * 100 * 0.0006);
    }

    // ── E-dblclick (VERIFICATION-3.1 Issue 1; E15) ──────────────────────────
    void dblRow (const juce::String& name, const juce::String& id, std::function<void()> setup, const juce::String& sel, bool onNode)
    {
        auto xy = std::make_shared<std::pair<double, double>>();
        now ([this, setup] { setup(); });
        steps.push_back (sleep (150.0));
        if (! sel.contains ("knob")) call ("clickChip", { 2 }, "chip");
        steps.push_back (sleep (500.0));   // past CREATE_DBLCLICK_MS of any earlier create
        readGeom();
        now ([this, xy] { *xy = { fxP (getS (bp (2, "freq"))), gyP (getS (bp (2, "gain"))) }; listenAll(); });
        const auto tag = "db" + juce::String (jsTag++);
        steps.push_back ([this, xy, tag, sel, onNode]
        {
            keyed.erase ("done:" + tag);
            if (onNode) js ("window.__bench.dblPlot('" + tag + "'," + juce::String (xy->first) + "," + juce::String (xy->second) + ");");
            else        js ("window.__bench.dblEl('" + tag + "','" + sel + "');");
            return true;
        });
        steps.push_back (waitFor ("done:" + tag, 4000.0, true));
        steps.push_back (sleep (200.0));
        now ([this, name, id]
        {
            unlistenAll();
            auto* p = param (id);
            const double defN = p->getDefaultValue(), hostN = p->getValue();
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("defaultScaled", p->convertFrom0to1 ((float) defN)); o->setProperty ("hostScaled", getS (id));
            o->setProperty ("normErr", std::abs (hostN - defN)); o->setProperty ("exact", hostN == (float) defN);
            o->setProperty ("b/e", juce::String (gb (id)) + "/" + juce::String (ge (id)));
            verdict ("E-dblclick", name, std::abs (hostN - defN) <= 1e-6 && gb (id) == 1 && ge (id) == 1, d);
        });
    }

    void addDblClickRows()
    {
        const auto band2 = [this] { neutral(); bandSet (2, 0, 4000.0, 7.0, 3.0); };
        dblRow ("node: gain to its default", bp (2, "gain"), band2, "", true);
        dblRow ("strip freq field", bp (2, "freq"), band2, "#field-freq", false);
        dblRow ("strip gain field", bp (2, "gain"), band2, "#field-gain", false);
        dblRow ("strip Q field", bp (2, "q"), band2, "#field-q", false);
        dblRow ("output gain knob", "output_gain", [this] { neutral(); setS ("output_gain", 5.0); }, "#knob-output_gain", false);
        dblRow ("gain scale knob", "gain_scale", [this] { neutral(); setS ("gain_scale", 150.0); }, "#knob-gain_scale", false);

        // A node created within 450 ms is not reset by the double-click that made it.
        auto pt = std::make_shared<std::pair<double, double>>();
        now ([this] { neutral(); });
        steps.push_back (sleep (150.0));
        readGeom();
        now ([this, pt] { *pt = { kMl + 0.5 * geom.plotW, gyP (6.0) }; });
        steps.push_back ([this, pt]
        {
            keyed.erase ("done:dbcreate");
            js ("window.__bench.dblPlot('dbcreate'," + juce::String (pt->first) + "," + juce::String (pt->second) + ");");
            return true;
        });
        steps.push_back (waitFor ("done:dbcreate", 4000.0, true));
        steps.push_back (sleep (200.0));
        now ([this, pt]
        {
            const double expG = std::round (ygP (pt->second) * 10.0) / 10.0;
            verdict ("E-dblclick", "a node created within 450 ms keeps its gain", getB (bp (1, "enabled")) && std::abs (getS (bp (1, "gain")) - expG) < 1e-3 && std::abs (expG) > 1.0,
                     juce::var ("gain " + juce::String (getS (bp (1, "gain")), 3) + " expected " + juce::String (expG, 3)));
        });
    }

    // ── E-delete ────────────────────────────────────────────────────────────
    void addDeleteRows()
    {
        now ([this] { neutral(); bandSet (3, 0, 200.0, 4.0, 1.0); bandSet (5, 0, 1000.0, -4.0, 1.0); bandSet (8, 0, 6000.0, 3.0, 1.0); });
        steps.push_back (sleep (150.0));
        call ("clickChip", { 5 }, "chip");
        now ([this] { listenAll(); });
        call ("key", { "Delete" }, "done");
        steps.push_back (sleep (200.0));
        now ([this]
        {
            unlistenAll();
            const auto e = bp (5, "enabled");
            verdict ("E-delete", "Delete key: band 5 off, one gesture, band 3 selected",
                     ! getB (e) && gb (e) == 1 && ge (e) == 1 && proc->uiSelectedBand.load() == 3 && gesturesOutside ({ e }) == 0,
                     juce::var ("enabled " + juce::String ((int) getB (e)) + " b/e " + juce::String (gb (e)) + "/" + juce::String (ge (e))
                                + " selected " + juce::String (proc->uiSelectedBand.load())));
        });
        call ("clickChip", { 8 }, "chip");
        now ([this] { listenAll(); });
        call ("clickEl", { "#btn-delete" }, "done");
        steps.push_back (sleep (200.0));
        now ([this]
        {
            unlistenAll();
            const auto e = bp (8, "enabled");
            verdict ("E-delete", "strip button: band 8 off, one gesture, band 3 selected",
                     ! getB (e) && gb (e) == 1 && ge (e) == 1 && proc->uiSelectedBand.load() == 3 && gesturesOutside ({ e }) == 0,
                     juce::var ("enabled " + juce::String ((int) getB (e)) + " b/e " + juce::String (gb (e)) + "/" + juce::String (ge (e))
                                + " selected " + juce::String (proc->uiSelectedBand.load())));
        });
    }

    // ── E-ends (UI-04, DSP-06): typed entry lands EXACTLY on the range ends ─
    void addEndsRows()
    {
        now ([this] { neutral(); bandSet (1, 0, 1000.0, 0.0, 1.0); });
        steps.push_back (sleep (150.0));
        call ("clickChip", { 1 }, "chip");
        struct E { const char* field; const char* key; const char* text; bool end; double mid; };
        const E es[] = {
            { "field-freq", "freq", "10", false, 1000 }, { "field-freq", "freq", "30k", true, 1000 }, { "field-freq", "freq", "30000", true, 1000 },
            { "field-freq", "freq", "5", false, 1000 },  { "field-freq", "freq", "99k", true, 1000 }, { "field-freq", "freq", "0", false, 1000 },
            { "field-freq", "freq", "1e9", true, 1000 }, { "field-freq", "freq", "-99", false, 1000 },
            { "field-gain", "gain", "-30", false, 3 },   { "field-gain", "gain", "30", true, 3 },     { "field-gain", "gain", "-99", false, 3 },
            { "field-gain", "gain", "99k", true, 3 },    { "field-gain", "gain", "1e9", true, 3 },    { "field-gain", "gain", "-1e9", false, 3 },
            { "field-q", "q", "0.025", false, 1 },       { "field-q", "q", "40", true, 1 },           { "field-q", "q", "0", false, 1 },
            { "field-q", "q", "99k", true, 1 },          { "field-q", "q", "1e9", true, 1 },          { "field-q", "q", "-99", false, 1 },
        };
        for (auto e : es)
        {
            const juce::String field (e.field), id = bp (1, e.key), text (e.text);
            const bool end = e.end; const double mid = e.mid;
            now ([this, id, mid] { setS (id, mid); });
            steps.push_back (sleep (60.0));
            call ("typeField", { field, text }, "typed", [this, id, text, end, field] (const juce::var& rep)
            {
                auto* f = dynamic_cast<juce::AudioParameterFloat*> (param (id));
                const float want = end ? f->range.end : f->range.start;
                auto d = obj(); auto* o = d.getDynamicObject();
                o->setProperty ("host", (double) f->get()); o->setProperty ("want", (double) want);
                o->setProperty ("diff", (double) f->get() - (double) want); o->setProperty ("page", rep);
                verdict ("E-ends", field + " \"" + text + "\" -> " + (end ? "end" : "start"), f->get() == want, d);
            });
        }
    }

    // ── E-stopped: host edit, no processBlock, curve follows within 2 ticks ─
    void addStoppedRow()
    {
        auto t0 = std::make_shared<double> (0.0);
        auto firstAt = std::make_shared<size_t> (0);
        auto before = std::make_shared<juce::var>();
        now ([this] { neutral(); bandSet (3, 0, 1000.0, 9.0, 1.0); });
        steps.push_back (sleep (150.0));
        call ("clickChip", { 3 }, "chip");
        steps.push_back (sleep (150.0));
        readGeom();
        probe ("E-stopped", "curve before the edit matches the engine", nullptr, [before] (const juce::var& r) { *before = r; });
        now ([this, t0, firstAt]
        {
            *firstAt = emits.size();
            *t0 = nowMs();
            setS (bp (3, "freq"), 3000.0);       // a host edit; the bench never calls processBlock
        });
        steps.push_back (sleep (150.0));
        now ([this, t0, firstAt]
        {
            double at = -1; int nth = 0;
            for (size_t i = *firstAt; i < emits.size(); ++i)
            {
                ++nth;
                if (emits[i].bands.contains (2)) { at = emits[i].t - *t0; break; }
            }
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("msToEmit", at); o->setProperty ("nthEmitAfterEdit", nth);
            o->setProperty ("processBlockCalls", 0);
            verdict ("E-stopped", "eqSections for band 3 within 2 ticks", at >= 0 && at <= 2.0 * kTickMs + 4.0 && nth == 1, d);
        });
        probe ("E-stopped", "curve after the edit matches the engine", nullptr, [this, before] (const juce::var& after)
        {
            int moved = 0;
            if (auto* a = after["cols"].getArray())
                if (auto* b = (*before)["cols"].getArray())
                    for (int i = 0; i < juce::jmin (a->size(), b->size()); ++i)
                        if (std::abs ((double) (*a)[i]["expY"] - (double) (*b)[i]["expY"]) > 3.0) ++moved;
            verdict ("E-stopped", "the painted curve changed (columns whose expected y moved > 3 px)", moved >= 1, juce::var (moved));
        });
    }

    // ── E-mono: packets carry Stereo; painted = Stereo-placement curve ──────
    void setLayoutCh (int ch)
    {
        const auto set = ch == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (set); l.outputBuses.add (set);
        proc->releaseResources();
        proc->setBusesLayout (l);
        proc->prepareToPlay (48000.0, 512);
    }

    void addMonoRow()
    {
        auto mark = std::make_shared<int> (0);
        now ([this] { neutral(); bandSet (4, 0, 300.0, 12.0, 1.0, 1); bandSet (5, 0, 3000.0, -12.0, 1.0, 2); });
        steps.push_back (sleep (150.0));
        call ("clickChip", { 4 }, "chip");
        call ("eqMark", {}, "eqMark", [mark] (const juce::var& v) { *mark = (int) v["n"]; });
        now ([this] { setLayoutCh (1); });
        steps.push_back (sleep (250.0));
        readGeom();
        steps.push_back ([this, mark]
        {
            keyed.erase ("eqSince:mono");
            js ("window.__bench.eqSince('mono'," + juce::String (*mark) + ");");
            return true;
        });
        steps.push_back (waitFor ("eqSince:mono", 4000.0, true));
        now ([this]
        {
            const auto list = keyed.count ("eqSince:mono") ? keyed["eqSince:mono"].second["list"] : juce::var();
            int p4 = -1, p5 = -1; bool stereoFlag = true, sawFull = false;
            if (auto* a = list.getArray())
                for (auto& e : *a)
                {
                    stereoFlag = (bool) e["stereo"];
                    if ((int) e["n"] == 24) sawFull = true;
                    if (auto* bs = e["bands"].getArray())
                        for (int i = 0; i < bs->size(); ++i)
                        {
                            if ((int) (*bs)[i] == 4) p4 = (int) e["placement"][i];
                            if ((int) (*bs)[i] == 5) p5 = (int) e["placement"][i];
                        }
                }
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("placement b04", p4); o->setProperty ("placement b05", p5); o->setProperty ("stereoHeader", stereoFlag);
            o->setProperty ("all24AfterLayoutChange", sawFull);
            o->setProperty ("relay b04", getI (bp (4, "placement"))); o->setProperty ("relay b05", getI (bp (5, "placement")));
            verdict ("E-mono", "packets carry Stereo on mono (relays still Left / Right)",
                     p4 == 0 && p5 == 0 && ! stereoFlag && sawFull && getI (bp (4, "placement")) == 1 && getI (bp (5, "placement")) == 2, d);
        });
        const std::map<int, int> relayStyle { { 4, 1 }, { 5, 2 } };
        probe ("E-mono", "painted curve = the Stereo-placement curve, and differs from the relay-placement curve", &relayStyle);
        now ([this] { setLayoutCh (2); });
        steps.push_back (sleep (250.0));
    }

    // ── E-lag: relay write -> eqSections -> painted, at 60 Hz pointer rate ──
    void addLagRow()
    {
        now ([this] { neutral(); bandSet (2, 0, 300.0, 6.0, 1.0); });
        steps.push_back (sleep (150.0));
        call ("clickChip", { 2 }, "chip");
        readGeom();
        now ([this]
        {
            listenAll();
            lagFreqChanges.clear(); eqPaints.clear();
            lagParamIndex = param (bp (2, "freq"))->getParameterIndex();
            js ("window.__bench.paintReports(true);");
        });
        steps.push_back (sleep (100.0));
        steps.push_back ([this]
        {
            keyed.erase ("done:lag");
            const double x0 = fxP (300.0), y0 = gyP (6.0);
            js ("window.__bench.drag('lag'," + juce::String (x0) + "," + juce::String (y0) + "," + juce::String (x0 + 260) + ","
                + juce::String (y0) + ",120,16.7,'up');");
            return true;
        });
        steps.push_back (waitFor ("done:lag", 10000.0, true));
        steps.push_back (sleep (300.0));
        now ([this]
        {
            js ("window.__bench.paintReports(false);");
            unlistenAll();
            const int lagIdx = lagParamIndex;
            lagParamIndex = -1;
            std::vector<double> lags;
            std::map<int, double> paintAt;
            for (auto& pr : eqPaints) paintAt[pr.first] = pr.second;
            for (double tc : lagFreqChanges)
            {
                for (size_t k = 0; k < emits.size(); ++k)
                    if (emits[k].t >= tc && emits[k].bands.contains (1))
                    {
                        if (paintAt.count ((int) k)) lags.push_back (paintAt[(int) k] - tc);
                        break;
                    }
            }
            std::sort (lags.begin(), lags.end());
            auto d = obj(); auto* o = d.getDynamicObject();
            o->setProperty ("freqChanges", (int) lagFreqChanges.size()); o->setProperty ("matched", (int) lags.size());
            o->setProperty ("paramIndex", lagIdx);
            const double med = lags.empty() ? -1 : lags[lags.size() / 2];
            const double p99 = lags.empty() ? -1 : lags[juce::jmin (lags.size() - 1, (size_t) (0.99 * (double) lags.size()))];
            o->setProperty ("medianMs", med); o->setProperty ("p99Ms", p99); o->setProperty ("maxMs", lags.empty() ? -1 : lags.back());
            o->setProperty ("boundMs", 2.0 * kTickMs + 16.7);
            verdict ("E-lag", "p99 node-to-curve lag <= 2 ticks + 1 frame", lags.size() >= 60 && p99 >= 0 && p99 <= 2.0 * kTickMs + 16.7, d);
        });
    }

    // ── E-draw: draw() time on a heavy 24-band draw at 1600 x 1000 ──────────
    void addDrawRow()
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
        addNative ("draw-size", "resizeEditor", { 1600, 1000 }, [] (const juce::var&) {});
        steps.push_back (sleep (500.0));
        call ("drawTiming", { 120 }, "drawTiming", [this] (const juce::var& r)
        {
            put ("E-draw", "result", r);
            verdict ("E-draw", "recorded (not gated): draw() median / p99 ms", (int) r["n"] >= 60, juce::var (juce::String ((double) r["median"], 2) + " / "
                     + juce::String ((double) r["p99"], 2) + " ms over " + juce::String ((int) r["n"]) + " draws, " + juce::String ((int) r["sections"])
                     + " sections, " + juce::String ((int) r["geom"]["w"]) + " x " + juce::String ((int) r["geom"]["h"])));
        }, 20000.0);
        addNative ("draw-size-back", "resizeEditor", { 960, 600 }, [] (const juce::var&) {});
        steps.push_back (sleep (300.0));
    }
