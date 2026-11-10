/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 3 / Phase 3.1 processor-surface
    gates (PLAN-3.1 PD11, T5). Processor only: the harness never compiles the
    editor. Included by main.cpp AFTER engine_gates_26b.h.

    Row IDs carry a U prefix: PLAN-3.1 PD11's PM1 / SR1 collide with existing
    rows (parity PM1, engine_gates_24 SR1), so the whole family is prefixed
    (UPM1 = PD11 PM1, ... UTL31 = PD11 TL31). OGI_ROWS groups: UPM, UWC, USR,
    UOV, UTL.

      UPM1   preset round trip (savePresetToFile / loadPresetFromFile, temp dir,
             fresh instance): 259 parameters, custom scale, 3 masks, seed equal;
             the 2 excluded stay the loader's own; scale revision moved
      UPM2   output_gain -12 dB survives a load; no output_gain / rec_arm key in
             the file                                   ctrl UPM2n exclusion off
      UPM2n  exclusion off -> output_gain lost (the predicate must FAIL)
      UPM3   a preset FILE carrying rec_arm On, loaded on an instance fed a live
             sidechain: recArms 0, rec_arm unchanged     ctrl UPM3n not excluded
      UPM3n  rec_arm not excluded -> recArms >= 1 (must FAIL; proves the
             stimulus arms and starts)
      UPM4   two reopens: preset load -> get state -> fresh set state -> change a
             mask -> get state -> fresh set state == the changed state; no
             CustomState child in any live tree
      UPM5   seed 0xFEDCBA9876543211 exact through a preset (a string)
      UPM5n  the seed written as a JSON number -> mismatch (must FAIL)
      UPM6   slots: file in slot 1 + factory stub in slot 3, save, clear, put
             buffers in slots 2 / 4, load -> both restored, slots 2 / 4 left
             untouched (PLAN-3.2 PD15, D45); no `embedded` key, < 64 KB
      UPM6b  a preset with no `slots` key leaves the loaded slots as they are
      UWC1   windowCurve (0, 0, 64) == closed-form Hann within 1e-6
      UWC2   skew +-100 and window 1: finite, in [0, 1], endpoints 0, differ
      USR1   slot revision moves on load, same-source reload, relink, reset,
             embed toggle, missing restore, preset apply, record commit; stable
             over 100 idle polls
      USR2   ovRev of the touched slot moves on every tape-changing event of
             USR1 and on a record commit (commitSync). The publish count is
             global, so untouched slots are NOT asserted to stay put.
      USR1n  fingerprint without its tape-identity term -> a same-source reload
             is missed (must FAIL)
      UOV1   overview of the tone stub: 656 peaks, max within 1 % of -12 dBFS,
             == an independent fold of the pristine; raised-cosine ends < 0.05,
             each end judged where its 512-frame bins sit inside the 10 ms ramp
             (tail at 48 k, head at 192 k; the 48 k head bin holds the whole
             ramp + 32 plateau frames, true peak ~0.248 — DEVIATION from PD11's
             "ends < 0.05", reported in SUMMARY); empty slot -> false + zeros
      UTL31  telemetryToVar at defaults: the exact PD8 key set, fb / rec null,
             no lo / hi, counts 4 numbers, rec.bus 1 / 0 by layout

    Every row prints "meas ... | thr ... | ctrl ...". A negative-control row
    (UPM2n / UPM3n / UPM5n / USR1n) runs the SAME predicate as its parent with
    the guard removed and PASSES only when that predicate FAILS ("ctrl FIRED");
    "ctrl NOT FIRED (vacuous)" fails the row.

    Sandbox: every file lives under one temp dir (deleted at exit); presets go
    through savePresetToFile / loadPresetFromFile only, so the manager's
    ~/Library/O-GranulatorInstrument/Presets tree is never resolved; recordings
    use the harness temp Recordings folder (make()).

  ==============================================================================
*/
#pragma once

namespace { namespace ug31
{
    using eg21::Session;
    using eg21::Proc;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::kSeed;
    using eg21::setP;
    using eg21::rawOf;
    using eg24b::want;
    using eg24b::section;
    using eg26a::ScSig;
    using eg26a::inject;
    using eg26a::rec;
    using eg26a::pr;
    using eg26a::recBase;
    using eg26a::stName;
    using eg26b::writeWav;
    using eg26b::settle;
    using eg26b::recordTake;
    using eg26b::stBase;
    using RS = ogi::RecState;
    using CS = juce::AudioChannelSet;
    using SlotInfo = ogi::SlotInfo;

    constexpr double kFs = 48000.0;

    juce::File gTmp31;

    //==========================================================================
    // Helpers
    juce::String hex (std::uint64_t v) { return "0x" + juce::String::toHexString ((juce::int64) v).paddedLeft ('0', 16); }

    bool savePreset (Proc& p, const juce::File& f) { return p.getPresetManager().savePresetToFile (f); }
    bool loadPreset (Proc& p, const juce::File& f) { return p.getPresetManager().loadPresetFromFile (f); }

    juce::var parseFile (const juce::File& f) { return juce::JSON::parse (f.loadFileAsString()); }

    bool writeJson (const juce::File& f, const juce::var& v)
    {
        return f.replaceWithText (juce::JSON::toString (v, true), false, false, "\n");
    }

    juce::StringArray keysOf (const juce::var& v)
    {
        juce::StringArray k;
        if (auto* o = v.getDynamicObject())
            for (const auto& prop : o->getProperties())
                k.add (prop.name.toString());
        return k;
    }

    bool isNumber (const juce::var& v) { return v.isInt() || v.isInt64() || v.isDouble(); }

    /** Every parameter to a seeded random value that is a fixed point of the
        parameter's own conversions (so a preset's saved getValue() re-applies
        to exactly the same getValue()). */
    void randomiseAll (Proc& p, juce::int64 seed)
    {
        juce::Random r (seed);
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                rp->setValueNotifyingHost (settledNorm (*rp, r.nextFloat()));
    }

    bool isExcludedId (const juce::String& id) { return id == "output_gain" || id == "rec_arm"; }

    struct ParamCmp
    {
        int compared = 0, differ = 0, nonDefault = 0;
        juce::String first;
    };

    /** Bitwise getValue() comparison of every parameter except output_gain /
        rec_arm; nonDefault counts how many of `a`'s differ from their default
        (the stimulus). */
    ParamCmp compareParams (Proc& a, Proc& b)
    {
        ParamCmp c;
        const auto& pa = a.getParameters();
        const auto& pb = b.getParameters();
        for (int i = 0; i < juce::jmin (pa.size(), pb.size()); ++i)
        {
            auto* ra = dynamic_cast<juce::RangedAudioParameter*> (pa[i]);
            auto* rb = dynamic_cast<juce::RangedAudioParameter*> (pb[i]);
            if (ra == nullptr || rb == nullptr || isExcludedId (ra->getParameterID()))
                continue;
            ++c.compared;
            const float va = ra->getValue(), vb = rb->getValue();
            if (std::memcmp (&va, &vb, sizeof (float)) != 0)
            {
                if (c.differ++ == 0)
                    c.first = ra->getParameterID() + " " + juce::String (va, 9) + " vs " + juce::String (vb, 9);
            }
            const float d = ra->getDefaultValue();
            c.nonDefault += std::memcmp (&va, &d, sizeof (float)) != 0 ? 1 : 0;
        }
        return c;
    }

    int masksDiffer (const Proc& a, const Proc& b)
    {
        int n = 0;
        for (int s = 1; s <= Ids::kStreams; ++s)
            for (int c = 1; c < Ids::kNumCollections; ++c)
                n += a.getDegreeMask (s, c) != b.getDegreeMask (s, c) ? 1 : 0;
        return n;
    }

    ogi::SavedSlot factorySlot (int index, const char* id)
    {
        ogi::SavedSlot x;
        x.index  = index;
        x.source = juce::String ("factory:") + id;
        x.name   = id;
        return x;
    }

    //==========================================================================
    // UPM1 — preset round trip
    void rowPM1()
    {
        constexpr std::uint64_t kSeedA = 0x0123456789ABCDEFull;
        const auto file = gTmp31.getChildFile ("pm1.json");

        auto a = make();
        randomiseAll (*a, (juce::int64) 0x31A10001);
        setParam (*a, "output_gain", -6.0f);
        setParam (*a, "rec_arm", 1.0f);
        a->setSeed (kSeedA);
        juce::String bad;
        const auto err = a->setCustomScale ("pm1 scale", "edo 19", bad);
        a->setDegreeMask (1, 3,  Proc::kAllOn & ~std::uint64_t (0x1));
        a->setDegreeMask (2, 17, Proc::kAllOn & ~std::uint64_t (0x2));
        a->setDegreeMask (4, 31, Proc::kAllOn & ~std::uint64_t (0x15));   // 31 = the shared custom scale
        const std::uint64_t m1 = a->getDegreeMask (1, 3), m2 = a->getDegreeMask (2, 17), m3 = a->getDegreeMask (4, 31);
        const bool stim = err.isEmpty() && m1 != Proc::kAllOn && m2 != Proc::kAllOn && m3 != Proc::kAllOn;
        const bool saved = savePreset (*a, file);

        auto b = make();
        b->setSeed (0x1111ull);
        const auto before = compareParams (*a, *b);
        const auto rev0 = b->getScaleStateRevision();
        const bool loaded = loadPreset (*b, file);
        const auto rev1 = b->getScaleStateRevision();
        const auto after = compareParams (*a, *b);

        const auto ca = a->getCustomScale(), cb = b->getCustomScale();
        const bool scale = cb.name == "pm1 scale" && cb.text == "edo 19" && cb.name == ca.name && cb.text == ca.text
                        && juce::exactlyEqual (cb.period, ca.period) && b->getCustomTable().count == 19;
        const int md = masksDiffer (*a, *b);
        const bool masks = md == 0 && b->getDegreeMask (1, 3) == m1 && b->getDegreeMask (2, 17) == m2 && b->getDegreeMask (4, 31) == m3;
        // The two excluded parameters keep the LOADER's values (defaults here), not the preset's.
        auto* og = param (*b, "output_gain");
        auto* ra = param (*b, "rec_arm");
        const bool exclKept = og != nullptr && ra != nullptr && og->getValue() == og->getDefaultValue() && ra->getValue() == ra->getDefaultValue()
                           && param (*a, "output_gain")->getValue() != og->getValue() && param (*a, "rec_arm")->getValue() != ra->getValue();

        row (stim && saved && loaded && after.compared == 259 && after.differ == 0 && before.differ > 200 && scale && masks
                 && b->getSeed() == kSeedA && rev1 != rev0 && exclKept,
             "UPM1 PD2 preset round trip (temp file, fresh instance): params, custom scale, 3 masks, seed",
             "meas " + juce::String (after.compared - after.differ) + "/" + juce::String (after.compared) + " params bitwise ("
                 + juce::String (after.nonDefault) + " non-default, " + juce::String (before.differ) + " differed before the load"
                 + (after.differ ? ", e.g. " + after.first : juce::String()) + "), scale " + (scale ? "edo 19 / 'pm1 scale'" : "LOST")
                 + ", masks " + juce::String (124 - md) + "/124 (3 non-default " + (masks ? "kept" : "LOST") + "), seed " + hex (b->getSeed())
                 + ", output_gain / rec_arm " + (exclKept ? "kept the loader's" : "OVERWRITTEN") + ", scaleRev " + juce::String ((int) rev0)
                 + " -> " + juce::String ((int) rev1) + " | thr 259/259, seed " + hex (kSeedA) + ", scaleRev moved | margin 0 | ctrl - (UPM2n / UPM5n)");
    }

    //==========================================================================
    // UPM2 / UPM2n — output_gain is the user's trim
    struct Pm2R { bool ok = false; float before = 0.0f, after = 0.0f; bool hasGain = false, hasArm = false; int keys = 0; };

    Pm2R runPM2 (bool exclusionOff, const juce::File& file)
    {
        Pm2R r;
        auto a = make();
        if (exclusionOff)
            a->getPresetManager().setExcludedParameters ({});
        setParam (*a, "output_gain", 3.0f);
        setParam (*a, "rec_arm", 1.0f);
        setParam (*a, "polyphony", 9.0f);
        r.ok = savePreset (*a, file);

        const auto params = parseFile (file)["parameters"];
        r.keys    = keysOf (params).size();
        r.hasGain = params.hasProperty ("output_gain");
        r.hasArm  = params.hasProperty ("rec_arm");

        auto b = make();
        if (exclusionOff)
            b->getPresetManager().setExcludedParameters ({});
        r.ok = setP (*b, "output_gain", -12.0f) && r.ok;
        r.before = rawOf (*b, "output_gain");
        r.ok = loadPreset (*b, file) && r.ok;
        r.after = rawOf (*b, "output_gain");
        // The load really applied: polyphony follows the preset.
        r.ok = r.ok && juce::exactlyEqual (rawOf (*b, "polyphony"), 9.0f);
        return r;
    }

    bool pm2Predicate (const Pm2R& r) { return juce::exactlyEqual (r.after, -12.0f) && juce::exactlyEqual (r.before, -12.0f); }

    void rowsPM2()
    {
        const auto r = runPM2 (false, gTmp31.getChildFile ("pm2.json"));
        row (r.ok && pm2Predicate (r) && ! r.hasGain && ! r.hasArm && r.keys == 259,
             "UPM2 PD2 output_gain -12 dB survives a preset load; no output_gain / rec_arm key in the file",
             "meas output_gain " + f2 (r.before, 2) + " -> " + f2 (r.after, 2) + " dB (preset saved at +3), file keys " + juce::String (r.keys)
                 + ", output_gain key " + (r.hasGain ? "PRESENT" : "absent") + ", rec_arm key " + (r.hasArm ? "PRESENT" : "absent")
                 + " | thr -12.00 exactly, 259 keys, both absent | margin 0 | ctrl UPM2n");

        const auto n = runPM2 (true, gTmp31.getChildFile ("pm2n.json"));
        const bool fired = n.ok && ! pm2Predicate (n) && n.hasGain;
        row (fired, "UPM2n control: setExcludedParameters ({}) -> output_gain lost (predicate must FAIL)",
             "meas output_gain " + f2 (n.before, 2) + " -> " + f2 (n.after, 2) + " dB, file keys " + juce::String (n.keys) + ", output_gain key "
                 + (n.hasGain ? "present" : "ABSENT") + " | thr predicate 'still -12.00' FAILS | "
                 + ctrl (fired, "UPM2n exclusion off -> predicate failed, output_gain " + f2 (n.after, 2) + " dB"));
    }

    //==========================================================================
    // UPM3 / UPM3n — a preset must never arm a capture
    struct Pm3R
    {
        bool ok = false, fileHasArm = false;
        std::uint64_t arms = 0, starts = 0;
        float armBefore = 0.0f, armAfter = 0.0f;
        RS state = RS::Idle;
        double scRms = 0.0;
    };

    Pm3R runPM3 (bool notExcluded, const juce::File& file)
    {
        Pm3R r;
        // The preset FILE carries rec_arm = 1 (written by an instance that only
        // excludes output_gain): the loader's exclusion is what is under test.
        {
            Session a (kFs, recBase (0, 1), kSeed, 512);   // Threshold trigger, -30 dB, 1 s
            a.p->getPresetManager().setExcludedParameters ({ "output_gain" });
            setP (*a.p, "rec_arm", 1.0f);
            r.ok = savePreset (*a.p, file);
            const auto v = parseFile (file)["parameters"]["rec_arm"];
            r.fileHasArm = isNumber (v) && static_cast<double> (v) == 1.0;
        }

        Session s (kFs, recBase (0, 1), kSeed, 512);
        if (notExcluded)
            s.p->getPresetManager().setExcludedParameters ({ "output_gain" });
        const ScSig g { 0.5f, 331 };   // seeded noise at 0.5: far above the -30 dB threshold
        inject (s, g);
        s.run (4800, 512);
        r.armBefore = rawOf (*s.p, "rec_arm");
        r.ok = loadPreset (*s.p, file) && r.ok && s.p->getSidechainChannelCount() == 2 && rec (s).hooks.recArms == 0;
        s.run (96000, 512);
        r.armAfter = rawOf (*s.p, "rec_arm");
        r.arms   = rec (s).hooks.recArms;
        r.starts = rec (s).hooks.recStarts;
        r.state  = pr (s).state;
        double e = 0.0;
        for (juce::int64 t = 4800; t < 4800 + 4800; ++t)
            e += (double) g.at (t, 0) * (double) g.at (t, 0);
        r.scRms = std::sqrt (e / 4800.0);
        return r;
    }

    bool pm3Predicate (const Pm3R& r) { return r.arms == 0 && juce::exactlyEqual (r.armAfter, r.armBefore); }

    void rowsPM3()
    {
        const auto r = runPM3 (false, gTmp31.getChildFile ("pm3.json"));
        const auto n = runPM3 (true, gTmp31.getChildFile ("pm3n.json"));
        const bool fired = n.ok && n.fileHasArm && ! pm3Predicate (n) && n.arms >= 1 && n.starts >= 1;

        row (r.ok && r.fileHasArm && pm3Predicate (r) && r.starts == 0 && r.state == RS::Idle && juce::exactlyEqual (r.armAfter, 0.0f),
             "UPM3 PD2 a preset carrying rec_arm On, loaded with a live sidechain: no arm, rec_arm unchanged",
             "meas recArms " + i64 (r.arms) + ", recStarts " + i64 (r.starts) + ", rec_arm " + f2 (r.armBefore, 0) + " -> " + f2 (r.armAfter, 0)
                 + ", state " + stName (r.state) + ", file rec_arm " + (r.fileHasArm ? "1" : "ABSENT") + ", sidechain RMS "
                 + f2 (juce::Decibels::gainToDecibels (r.scRms), 1) + " dBFS (threshold -30) | thr 0 / 0 / unchanged / Idle | margin 0 | ctrl UPM3n");

        row (fired, "UPM3n control: rec_arm not excluded -> the same load arms a capture (predicate must FAIL)",
             "meas recArms " + i64 (n.arms) + ", recStarts " + i64 (n.starts) + ", rec_arm " + f2 (n.armBefore, 0) + " -> " + f2 (n.armAfter, 0)
                 + ", state " + stName (n.state) + " | thr predicate 'recArms == 0, rec_arm unchanged' FAILS, arms >= 1, starts >= 1 | "
                 + ctrl (fired, "UPM3n exclusion { output_gain } only -> predicate failed, " + i64 (n.arms) + " arm(s), " + i64 (n.starts) + " start(s)"));
    }

    //==========================================================================
    // UPM4 — two reopens after a preset load
    void rowPM4()
    {
        const auto file = gTmp31.getChildFile ("pm4.json");
        juce::String bad;
        auto a = make();
        randomiseAll (*a, (juce::int64) 0x31A40001);
        setParam (*a, "rec_arm", 0.0f);
        a->setSeed (0x4444555566667777ull);
        a->setCustomScale ("pm4", "edo 7", bad);
        a->setDegreeMask (2, 5, Proc::kAllOn & ~std::uint64_t (0x4));
        const bool saved = savePreset (*a, file);

        auto b = make();
        const bool loaded = loadPreset (*b, file);
        const auto st1 = stateOf (*b);

        auto c = make();
        load (*c, st1);
        const std::uint64_t changed = Proc::kAllOn & ~std::uint64_t (0x9);
        c->setDegreeMask (3, 9, changed);
        const auto st2 = stateOf (*c);

        auto d = make();
        load (*d, st2);
        const auto st3 = stateOf (*d);

        const auto n1 = eg26b::norm (st1), n2 = eg26b::norm (st2), n3 = eg26b::norm (st3);
        const bool same = n2.isNotEmpty() && n3 == n2;
        const bool moved = n1 != n2;
        const bool values = d->getDegreeMask (3, 9) == c->getDegreeMask (3, 9) && d->getDegreeMask (3, 9) != Proc::kAllOn
                         && d->getDegreeMask (2, 5) == a->getDegreeMask (2, 5) && d->getCustomScale().text == "edo 7"
                         && d->getSeed() == 0x4444555566667777ull && bitwiseEqual (allValues (*b), allValues (*d));

        int liveChildren = 0, xmlChildren = 0;
        for (auto* q : { b.get(), c.get(), d.get() })
            liveChildren += q->parameters.state.getChildWithName ("CustomState").isValid() ? 1 : 0;
        for (const auto* m : { &st1, &st2, &st3 })
            if (const auto x = xmlOf (*m))
                xmlChildren += countChildren (*x, "CustomState");

        row (saved && loaded && same && moved && values && liveChildren == 0 && xmlChildren == 0,
             "UPM4 preset load -> state -> fresh restore -> change a mask -> state -> fresh restore == the changed state",
             juce::String ("meas reopen 2 state ") + (same ? "identical" : "DIFFERS") + " to the changed one (" + juce::String (n2.length())
                 + " chars), differs from reopen 1 " + (moved ? "yes" : "NO") + ", mask / scale / seed / 261 values " + (values ? "follow" : "LOST")
                 + ", CustomState children: live " + juce::String (liveChildren) + ", saved " + juce::String (xmlChildren)
                 + " | thr identical, 0, 0 | margin 0 | ctrl - (the changed state != the first)");
    }

    //==========================================================================
    // UPM5 / UPM5n — the seed is a string
    void rowsPM5()
    {
        constexpr std::uint64_t kSeed5 = 0xFEDCBA9876543211ull;
        constexpr std::uint64_t kLive  = 0x1111ull;
        const auto file  = gTmp31.getChildFile ("pm5.json");
        const auto fileN = gTmp31.getChildFile ("pm5n.json");
        const auto text  = juce::String ((juce::int64) kSeed5);

        auto a = make();
        a->setSeed (kSeed5);
        const bool saved = savePreset (*a, file);
        const auto sv = parseFile (file)["customState"]["seed"];
        const bool isString = sv.isString() && sv.toString() == text;

        auto b = make();
        b->setSeed (kLive);
        const bool loaded = loadPreset (*b, file);
        const auto got = b->getSeed();

        row (saved && loaded && isString && got == kSeed5,
             "UPM5 PD2 seed 0xFEDCBA9876543211 exact through a preset (int64 decimal string)",
             "meas seed " + hex (got) + ", file seed " + (sv.isString() ? "string \"" + sv.toString() + "\"" : juce::String ("NOT a string"))
                 + " | thr " + hex (kSeed5) + ", a string | margin 0 | ctrl UPM5n");

        // The control: the same file with the seed as a bare JSON number.
        const auto src = file.loadFileAsString();
        const auto edited = src.replace ("\"" + text + "\"", text);
        const bool rewritten = edited != src && fileN.replaceWithText (edited, false, false, "\n");
        const auto nv = parseFile (fileN)["customState"]["seed"];
        auto c = make();
        c->setSeed (kLive);
        const bool loadedN = loadPreset (*c, fileN);
        const auto gotN = c->getSeed();
        const bool fired = rewritten && loadedN && isNumber (nv) && gotN != kSeed5;
        row (fired, "UPM5n control: the seed written as a JSON number -> mismatch (predicate must FAIL)",
             "meas seed " + hex (gotN) + " (live was " + hex (kLive) + "), file seed " + (isNumber (nv) ? "a number" : "NOT a number")
                 + " | thr predicate 'seed == " + hex (kSeed5) + "' FAILS | "
                 + ctrl (fired, "UPM5n JSON-number seed -> predicate failed, seed " + hex (gotN)));
    }

    //==========================================================================
    // UPM6 / UPM6b — slot sources in a preset, never a blob
    void rowsPM6()
    {
        juce::String e;
        const auto wav    = writeWav (gTmp31.getChildFile ("pm6-file.wav"), noiseBuf (2, 24000, 0.3f, 361), kFs, 32);
        const auto file   = gTmp31.getChildFile ("pm6.json");
        const auto fileNs = gTmp31.getChildFile ("pm6b.json");

        Session s (kFs, stBase(), kSeed, 512);
        auto& ld = s.p->getLoader();
        const bool l0 = s.p->loadSlotSync (0, wav, e);
        {
            auto list = ld.getSavedSlots();
            list.push_back (factorySlot (2, "stub_tone"));
            ld.restoreSlots (list);
        }
        s.p->setSlotEmbed (0, true);   // the SESSION would carry a blob; the preset must not
        settle (s);
        const auto a0 = s.p->getSlotInfo (0), a2 = s.p->getSlotInfo (2);
        const bool sessionHasBlob = a0.hasBlob && eg26b::slotsText (stateOf (*s.p)).contains ("embedded=");
        const bool setup = l0 && a0.loaded && a0.source == "file:" + wav.getFullPathName() && a0.embed && a2.loaded
                        && a2.source == "factory:stub_tone" && a2.contentHash == ogi::kHashTone && sessionHasBlob;

        const bool saved = savePreset (*s.p, file);
        const auto text = file.loadFileAsString();
        const auto slotsVar = juce::JSON::parse (text)["customState"]["slots"];
        const int listed = slotsVar.isArray() ? slotsVar.size() : -1;

        // Clear all four.
        ld.restoreSlots ({});
        settle (s);
        bool cleared = true;
        for (int k = 0; k < 4; ++k)
            cleared = cleared && ! s.p->getSlotInfo (k).loaded && s.p->getSlotInfo (k).source.isEmpty() && s.eng().getCurrentTape (k) == nullptr;

        // PLAN-3.2 PD15 (D45): the lanes the preset has no source for hold a
        // tape of their own when it loads, and must come out untouched.
        const bool l1 = s.load (1, noiseBuf (2, 24000, 0.2f, 362), kFs, "lane 2");
        const bool l3 = s.load (3, noiseBuf (2, 24000, 0.2f, 363), kFs, "lane 4");
        s.run (2048, 256);
        const auto u1 = s.p->getSlotInfo (1), u3 = s.p->getSlotInfo (3);
        const auto* t1 = s.eng().getCurrentTape (1);
        const auto* t3 = s.eng().getCurrentTape (3);
        const bool unlistedLoaded = l1 && l3 && u1.loaded && u3.loaded && t1 != nullptr && t3 != nullptr;

        const bool loaded = loadPreset (*s.p, file);
        const bool idle = ld.waitForIdle (20000);   // the sync point (PD11)
        s.run (2048, 256);
        const auto b0 = s.p->getSlotInfo (0), b1 = s.p->getSlotInfo (1), b2 = s.p->getSlotInfo (2), b3 = s.p->getSlotInfo (3);
        const bool restored = b0.loaded && b0.source == a0.source && b0.hasHash && b0.contentHash == a0.contentHash && b0.embed
                           && b2.loaded && b2.source == "factory:stub_tone" && b2.contentHash == ogi::kHashTone
                           && s.eng().getCurrentTape (0) != nullptr && s.eng().getCurrentTape (2) != nullptr;
        const bool untouched = b1.loaded && b3.loaded && b1.generation == u1.generation && b3.generation == u3.generation
                            && b1.name == u1.name && b3.name == u3.name && b1.source == u1.source && b3.source == u3.source
                            && s.eng().getCurrentTape (1) == t1 && s.eng().getCurrentTape (3) == t3;
        const bool noBlob = ! text.contains ("embedded");
        const auto bytes = file.getSize();

        row (setup && saved && cleared && unlistedLoaded && loaded && idle && restored && untouched && noBlob && bytes < 65536 && listed == 2,
             "UPM6 PD1 slots: file in slot 1 + factory stub in slot 3 -> save, clear, load: both restored, 2 / 4 untouched (D45)",
             juce::String ("meas cleared ") + (cleared ? "4/4" : "NO") + ", restored slot 1 " + (b0.loaded ? "rung " + juce::String (b0.resolvedBy) : juce::String ("NO"))
                 + " (hash " + (b0.contentHash == a0.contentHash ? "equal" : "DIFFERS") + "), slot 3 "
                 + (b2.loaded ? "rung " + juce::String (b2.resolvedBy) : juce::String ("NO")) + ", slots 2 / 4 (a buffer each, not in the preset) "
                 + (untouched ? "same tape, same generation" : "CHANGED")
                 + ", preset " + juce::String (bytes) + " bytes, " + juce::String (listed) + " slots listed, `embedded` "
                 + (noBlob ? "absent" : "PRESENT") + " (session state carries a blob: " + (sessionHasBlob ? "yes" : "NO")
                 + ") | thr rungs 1 / 4, 2 / 4 untouched, < 65536 bytes, absent | ctrl - (session blob present; UPM6cn clears)");

        // UPM6b: the same preset without the `slots` key, on the loaded slots.
        auto v = juce::JSON::parse (text);
        bool removed = false;
        if (auto* cs = v["customState"].getDynamicObject())
        {
            removed = cs->hasProperty ("slots");
            cs->removeProperty ("slots");
            cs->setProperty ("seed", "424242");
        }
        const bool wrote = removed && writeJson (fileNs, v) && ! parseFile (fileNs)["customState"].hasProperty ("slots");
        const auto pub0 = s.p->getHandoff().getPublishedCount();
        const auto* t0 = s.eng().getCurrentTape (0);
        const auto* t2 = s.eng().getCurrentTape (2);
        const bool loadedNs = loadPreset (*s.p, fileNs);
        const bool idleNs = ld.waitForIdle (20000);
        s.run (2048, 256);
        const auto c0 = s.p->getSlotInfo (0), c2 = s.p->getSlotInfo (2);
        const bool kept = c0.loaded && c2.loaded && c0.generation == b0.generation && c2.generation == b2.generation
                       && c0.source == b0.source && c2.source == b2.source && s.eng().getCurrentTape (0) == t0 && t0 != nullptr
                       && s.eng().getCurrentTape (2) == t2 && t2 != nullptr && s.p->getHandoff().getPublishedCount() == pub0;
        const bool applied = s.p->getSeed() == 424242ull;
        row (wrote && loadedNs && idleNs && kept && applied,
             "UPM6b PD1 a preset with no `slots` key leaves the loaded slots as they are",
             juce::String ("meas slots 1 / 3 ") + (kept ? "same tape, same generation" : "CHANGED") + ", publishes "
                 + i64 (s.p->getHandoff().getPublishedCount() - pub0) + ", preset applied (seed 424242) " + (applied ? "yes" : "NO")
                 + " | thr same, 0 publishes, applied | ctrl - (UPM6: a `slots` array clears unlisted slots)");
    }

    //==========================================================================
    // UWC — the window figure
    struct CurveStats { bool finite = true; float lo = 1.0f, hi = 0.0f, first = 0.0f, last = 0.0f; double maxDiffToHann = 0.0; };

    CurveStats curveStats (const std::array<float, 64>& c, const std::array<float, 64>& ref)
    {
        CurveStats st;
        for (size_t i = 0; i < c.size(); ++i)
        {
            st.finite = st.finite && std::isfinite (c[i]);
            st.lo = std::min (st.lo, c[i]);
            st.hi = std::max (st.hi, c[i]);
            st.maxDiffToHann = std::max (st.maxDiffToHann, std::abs ((double) c[i] - (double) ref[i]));
        }
        st.first = c.front();
        st.last  = c.back();
        return st;
    }

    void rowsWC()
    {
        auto p = make();
        const auto& eng = p->getEngine();

        std::array<float, 64> hannCurve {};
        eng.windowCurve (0.0f, 0.0f, hannCurve.data(), 64);
        double worst = 0.0;
        int worstAt = 0;
        for (int i = 0; i < 64; ++i)
        {
            const double h = 0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * (double) i / 63.0);
            const double d = std::abs ((double) hannCurve[(size_t) i] - h);
            if (d > worst) { worst = d; worstAt = i; }
        }
        // The processor accessor reads the BASE s1_window / s1_window_skew (0 / 0 by default).
        std::array<float, 64> viaProc {};
        p->getWindowCurve (0, viaProc.data(), 64);
        const bool same = std::memcmp (viaProc.data(), hannCurve.data(), sizeof (float) * 64) == 0;

        row (worst <= 1.0e-6 && same, "UWC1 PD9 windowCurve (0, 0, 64) == closed-form Hann",
             "meas max |err| " + juce::String (worst, 9) + " at point " + juce::String (worstAt) + ", getWindowCurve (stream 1, defaults) "
                 + (same ? "bitwise equal" : "DIFFERS") + " | thr <= 1e-6 | margin " + juce::String (1.0e-6 - worst, 9) + " | ctrl - (UWC2 differs)");

        struct Case { const char* name; float window, skew; };
        const Case cases[] = { { "skew +100", 0.0f, 100.0f }, { "skew -100", 0.0f, -100.0f }, { "window 1", 1.0f, 0.0f } };
        bool ok = true;
        juce::String d;
        std::array<float, 64> prev = hannCurve;
        for (const auto& c : cases)
        {
            std::array<float, 64> cv {};
            eng.windowCurve (c.window, c.skew, cv.data(), 64);
            const auto st = curveStats (cv, hannCurve);
            const bool distinct = std::memcmp (cv.data(), prev.data(), sizeof (float) * 64) != 0;
            ok = ok && st.finite && st.lo >= 0.0f && st.hi <= 1.0f && std::abs (st.first) <= 1.0e-6f && std::abs (st.last) <= 1.0e-6f
                    && st.maxDiffToHann > 0.01 && distinct;
            d << (d.isEmpty() ? "" : "; ") << c.name << ": range [" << juce::String (st.lo, 6) << ", " << juce::String (st.hi, 6) << "], ends "
              << juce::String (st.first, 7) << " / " << juce::String (st.last, 7) << ", max diff to Hann " << juce::String (st.maxDiffToHann, 4)
              << (st.finite ? "" : " NON-FINITE");
            prev = cv;
        }
        row (ok, "UWC2 PD9 skew +-100 and window 1: finite, in [0, 1], endpoints 0, differ from Hann",
             "meas " + d + " | thr in [0, 1], |ends| <= 1e-6, diff > 0.01, the three distinct | ctrl - (UWC1)");
    }

    //==========================================================================
    // USR — the slot revision and the per-slot overview revision
    struct Step { juce::String name; bool tape = false; int slot = 0; bool slotMoved = false, ovMoved = false, stimOk = false; };

    void rowsSR()
    {
        juce::String e;
        const auto fa = writeWav (gTmp31.getChildFile ("sr-a.wav"), noiseBuf (2, 24000, 0.3f, 311), kFs, 32);
        const auto fb = writeWav (gTmp31.getChildFile ("sr-b.wav"), noiseBuf (2, 24000, 0.3f, 312), kFs, 32);
        const auto preset = gTmp31.getChildFile ("sr-preset.json");

        Session s (kFs, stBase(), kSeed, 512);
        auto& ld = s.p->getLoader();

        const auto first = s.p->getSlotStateRevision();
        std::uint32_t sr = first;
        std::array<std::uint32_t, 4> ov {};
        for (int k = 0; k < 4; ++k)
            ov[(size_t) k] = s.p->getOverviewRev (k);

        std::vector<Step> steps;
        auto step = [&] (const char* name, bool tape, int slot, const std::function<bool()>& fn)
        {
            Step st;
            st.name = name;
            st.tape = tape;
            st.slot = slot;
            st.stimOk = fn();
            const auto sr2 = s.p->getSlotStateRevision();
            st.slotMoved = sr2 != sr;
            st.ovMoved = s.p->getOverviewRev (slot) != ov[(size_t) slot];
            sr = sr2;
            for (int k = 0; k < 4; ++k)
                ov[(size_t) k] = s.p->getOverviewRev (k);
            steps.push_back (st);
        };

        step ("load", true, 0, [&]
        {
            const bool ok = s.p->loadSlotSync (0, fa, e);
            settle (s);
            return ok && s.p->getSlotInfo (0).loaded;
        });
        const bool presetSaved = savePreset (*s.p, preset);   // slot 1 = sr-a.wav
        step ("same-source reload", true, 0, [&]
        {
            const auto pub = s.p->getHandoff().getPublishedCount();
            const bool ok = s.p->loadSlotSync (0, fa, e);
            settle (s);
            return ok && s.p->getHandoff().getPublishedCount() > pub;
        });
        step ("relink", true, 0, [&]
        {
            s.p->requestRelink (0, fb);
            settle (s);
            const auto i = s.p->getSlotInfo (0);
            return i.loaded && i.source == "file:" + fb.getFullPathName();
        });
        step ("reset", true, 0, [&]
        {
            const auto pub = s.p->getHandoff().getPublishedCount();
            const auto gen = s.p->getSlotInfo (0).generation;
            s.p->requestReset (0);
            settle (s);
            // Reset republishes the tape WITHOUT a new generation.
            return s.p->getHandoff().getPublishedCount() > pub && s.p->getSlotInfo (0).generation == gen;
        });
        step ("embed toggle", false, 0, [&]
        {
            const bool was = s.p->getSlotInfo (0).embed;
            s.p->setSlotEmbed (0, ! was);
            ld.waitForIdle (20000);
            return s.p->getSlotInfo (0).embed != was;
        });
        step ("missing restore", true, 0, [&]
        {
            ogi::SavedSlot x;
            x.index  = 0;
            x.source = "file:/nonexistent-ogi-31/gone.wav";
            x.name   = "gone.wav";
            ld.restoreSlots ({ x });
            settle (s);
            const auto i = s.p->getSlotInfo (0);
            return i.missing && ! i.loaded;
        });
        step ("preset apply", true, 0, [&]
        {
            const bool ok = presetSaved && loadPreset (*s.p, preset);
            settle (s);
            const auto i = s.p->getSlotInfo (0);
            return ok && i.loaded && i.source == "file:" + fa.getFullPathName();
        });
        step ("record commit", true, 1, [&]
        {
            recordTake (s, 1, ScSig { 0.4f, 313 }, 1.0);
            const auto i = s.p->getSlotInfo (1);
            return i.loaded && i.source.startsWith ("rec:");
        });

        // 100 idle polls (audio running in the middle): nothing moves.
        int idleMoves = 0;
        const auto idleRev = s.p->getSlotStateRevision();
        for (int k = 0; k < 100; ++k)
        {
            if (k == 50)
                s.run (4800, 256);
            idleMoves += s.p->getSlotStateRevision() != idleRev ? 1 : 0;
        }

        int slotMoved = 0, tapeSteps = 0, ovMoved = 0, stim = 0;
        juce::String missedSlot, missedOv, badStim;
        for (const auto& st : steps)
        {
            slotMoved += st.slotMoved ? 1 : 0;
            stim += st.stimOk ? 1 : 0;
            if (! st.slotMoved) missedSlot << " " << st.name;
            if (! st.stimOk)    badStim << " " << st.name;
            if (st.tape)
            {
                ++tapeSteps;
                ovMoved += st.ovMoved ? 1 : 0;
                if (! st.ovMoved) missedOv << " " << st.name;
            }
        }
        const int n = (int) steps.size();
        const bool stimOk = stim == n;

        row (stimOk && first == 1 && slotMoved == n && idleMoves == 0,
             "USR1 PD3 slot revision moves on load / reload / relink / reset / embed / missing / preset / commit; idle stable",
             "meas moved " + juce::String (slotMoved) + "/" + juce::String (n) + (missedSlot.isNotEmpty() ? " (missed:" + missedSlot + ")" : juce::String())
                 + ", events real " + juce::String (stim) + "/" + juce::String (n) + (badStim.isNotEmpty() ? " (NOT real:" + badStim + ")" : juce::String())
                 + ", first call " + juce::String ((int) first) + ", final " + juce::String ((int) idleRev) + ", idle moves " + juce::String (idleMoves)
                 + "/100 | thr " + juce::String (n) + "/" + juce::String (n) + ", first 1, 0 idle moves | margin 0 | ctrl USR1n");

        row (stimOk && ovMoved == tapeSteps && tapeSteps == 7,
             "USR2 PD6 ovRev of the touched slot moves on every tape-changing event and on a record commit (commitSync)",
             "meas moved " + juce::String (ovMoved) + "/" + juce::String (tapeSteps) + (missedOv.isNotEmpty() ? " (missed:" + missedOv + ")" : juce::String())
                 + " (load, reload, relink, reset, missing, preset on slot 1; commit on slot 2), final ovRev "
                 + juce::String ((int) ov[0]) + " / " + juce::String ((int) ov[1]) + " / " + juce::String ((int) ov[2]) + " / " + juce::String ((int) ov[3])
                 + " | thr 7/7 (untouched slots not asserted: the publish count is global) | margin 0 | ctrl - (USR1n)");

        // USR1n: the fingerprint without its tape-identity term.
        auto reload = [&] (bool hook, std::uint32_t& r1, std::uint32_t& r2)
        {
            Session q (kFs, stBase(), kSeed, 512);
            q.p->testSlotRevNoGeneration = hook;
            juce::String err;
            bool ok = q.p->loadSlotSync (0, fa, err);
            settle (q);
            r1 = q.p->getSlotStateRevision();
            const auto pub = q.p->getHandoff().getPublishedCount();
            const auto gen = q.p->getSlotInfo (0).generation;
            ok = q.p->loadSlotSync (0, fa, err) && ok;
            settle (q);
            r2 = q.p->getSlotStateRevision();
            // The reload was real: a new tape was published under a new generation.
            return ok && q.p->getHandoff().getPublishedCount() > pub && q.p->getSlotInfo (0).generation != gen;
        };
        std::uint32_t a1 = 0, a2 = 0, n1 = 0, n2 = 0;
        const bool realA = reload (false, a1, a2);
        const bool realN = reload (true, n1, n2);
        const bool fired = realA && realN && a2 != a1 && n2 == n1;
        row (fired, "USR1n control: fingerprint without generation -> a same-source reload is missed (predicate must FAIL)",
             "meas shipping rev " + juce::String ((int) a1) + " -> " + juce::String ((int) a2) + ", testSlotRevNoGeneration rev "
                 + juce::String ((int) n1) + " -> " + juce::String ((int) n2) + ", reload republished " + (realA && realN ? "yes" : "NO")
                 + " | thr predicate 'revision moves on a same-source reload' FAILS with the hook | "
                 + ctrl (fired, "USR1n testSlotRevNoGeneration -> predicate failed, rev stayed " + juce::String ((int) n2)));
    }

    //==========================================================================
    // UOV1 — the pristine overview
    void rowOV1()
    {
        constexpr int kN = Proc::kOverviewPeaks;
        Session s (kFs, stBase(), kSeed, 512);
        auto& ld = s.p->getLoader();
        ld.restoreSlots ({ factorySlot (0, "stub_tone") });
        settle (s);
        const auto info = s.p->getSlotInfo (0);

        std::array<float, (size_t) kN> pk;
        std::array<std::uint8_t, (size_t) kN> dirty;
        pk.fill (9.0f);
        dirty.fill (9);
        std::uint32_t rev = 0;
        const bool ok = s.p->getSlotOverview (0, pk.data(), dirty.data(), kN, rev);

        const double want = std::pow (10.0, -12.0 / 20.0);
        double mx = 0.0;
        bool range = true, clean = true;
        for (int i = 0; i < kN; ++i)
        {
            mx = std::max (mx, (double) pk[(size_t) i]);
            range = range && std::isfinite (pk[(size_t) i]) && pk[(size_t) i] >= 0.0f && pk[(size_t) i] <= 1.0f;
            clean = clean && dirty[(size_t) i] == 0;
        }
        const double relErr = std::abs (mx - want) / want;

        // An independent fold of the pristine (the tape is the pristine at 48 k):
        // peak i = max |x| over the 512-frame bins [i * bins / n, (i + 1) * bins / n).
        double worstRef = -1.0;
        const auto pm = ld.getPristine (0);
        const int frames = info.frames;
        const auto bins = (std::int64_t) (ld.getOverview (0).size() / 2);
        if (pm != nullptr && pm->pcm.getNumSamples() == frames && bins > 0)
        {
            worstRef = 0.0;
            for (int i = 0; i < kN; ++i)
            {
                const std::int64_t b0 = (std::int64_t) i * bins / kN;
                const std::int64_t b1 = std::max (b0 + 1, std::min (bins, (std::int64_t) (i + 1) * bins / kN));
                float ref = 0.0f;
                for (std::int64_t f = b0 * 512; f < std::min ((std::int64_t) frames, b1 * 512); ++f)
                    for (int ch = 0; ch < pm->pcm.getNumChannels(); ++ch)
                        ref = std::max (ref, std::abs (pm->pcm.getSample (ch, (int) f)));
                worstRef = std::max (worstRef, std::abs ((double) ref - (double) pk[(size_t) i]));
            }
        }

        // An empty slot: false, zeros, the revision still set.
        std::array<float, (size_t) kN> ep;
        std::array<std::uint8_t, (size_t) kN> ed;
        ep.fill (9.0f);
        ed.fill (9);
        std::uint32_t erev = 0xffffffffu;
        const bool eok = s.p->getSlotOverview (1, ep.data(), ed.data(), kN, erev);
        bool zeros = true;
        for (int i = 0; i < kN; ++i)
            zeros = zeros && ep[(size_t) i] == 0.0f && ed[(size_t) i] == 0;
        const bool empty = ! eok && zeros && erev == s.p->getOverviewRev (1);

        // The raised-cosine ends. The loader's overview is one bin per 512 tape
        // frames and the stub's ramps are 10 ms, so an end peak reads < 0.05
        // only when its bins lie inside the ramp's foot:
        //   48 k  (ramp 480 frames): the head bin = the whole ramp + 32 plateau
        //         frames (true peak ~0.248, equal to the pristine fold); the
        //         tail peak = the last 128 frames -> judged here.
        //   192 k (ramp 1920 frames): the head peak = the first 512 frames ->
        //         judged here; the tail peak folds 2 bins (1024 frames, ~0.117).
        // Each end is judged in the session where its bins sit in the ramp; all
        // four figures are printed.
        const float e0 = pk[0], e1 = pk[(size_t) kN - 1];
        float h0 = 9.0f, h1 = 9.0f;
        double hmx = 0.0;
        int hframes = 0;
        bool hok = false;
        {
            Session h (192000.0, stBase(), kSeed, 512);
            h.p->getLoader().restoreSlots ({ factorySlot (0, "stub_tone") });
            settle (h);
            std::array<float, (size_t) kN> hp;
            hp.fill (9.0f);
            std::uint32_t hrev = 0;
            hok = h.p->getSlotOverview (0, hp.data(), nullptr, kN, hrev);
            h0 = hp[0];
            h1 = hp[(size_t) kN - 1];
            for (const float v : hp)
                hmx = std::max (hmx, (double) v);
            hframes = h.p->getSlotInfo (0).frames;
        }
        const bool ends = hok && h0 < 0.05f && e1 < 0.05f && std::abs (hmx - want) / want <= 0.01;

        row (info.loaded && ok && range && clean && relErr <= 0.01 && ends && worstRef >= 0.0 && worstRef <= 1.0e-4
                 && rev == s.p->getOverviewRev (0) && empty,
             "UOV1 PD5 overview of the tone stub: 656 peaks, max == -12 dBFS, raised-cosine ends; empty slot -> false + zeros",
             "meas max " + juce::String (mx, 6) + " (want " + juce::String (want, 6) + ", err " + juce::String (100.0 * relErr, 3)
                 + " %), worst |peak - pristine fold| " + juce::String (worstRef, 7) + ", " + juce::String (frames) + " frames / "
                 + juce::String ((int) bins) + " bins, dirty " + (clean ? "all 0" : "SET") + ", rev " + juce::String ((int) rev)
                 + "; ends head / tail at 48 k " + juce::String (e0, 4) + " / " + juce::String (e1, 4) + ", at 192 k ("
                 + juce::String (hframes) + " frames, max " + juce::String (hmx, 6) + ") " + juce::String (h0, 4) + " / " + juce::String (h1, 4)
                 + "; empty slot " + (empty ? "false + zeros" : "BAD")
                 + " | thr err <= 1 %, fold <= 1e-4, tail @ 48 k < 0.05, head @ 192 k < 0.05 (512-frame bins vs 10 ms ramps) | ctrl - (empty slot)");
    }

    //==========================================================================
    // UTL31 — the 3.1 telemetry shape (as the page receives it: through JSON)
    juce::String telemetryProblems (const juce::var& t, const juce::String& text, int wantBus)
    {
        juce::StringArray bad;
        const juce::StringArray topKeys { "scaleRev", "slotRev", "lanes", "voices", "newest", "menv", "grains", "counts", "limit", "out",
                                          "rec", "sources", "modNorm" };
        if (keysOf (t) != topKeys)
            bad.add ("top keys " + keysOf (t).joinIntoString (","));
        if (! (t["scaleRev"].isInt() && t["slotRev"].isInt()))
            bad.add ("revisions not ints");

        const auto lanes = t["lanes"];
        if (! lanes.isArray() || lanes.size() != 4)
            bad.add ("lanes not 4");
        else
            for (int i = 0; i < 4; ++i)
            {
                const auto l = lanes[i];
                if (keysOf (l) != juce::StringArray { "ovRev", "fb", "rec" })
                    bad.add ("lane " + juce::String (i) + " keys " + keysOf (l).joinIntoString (","));
                if (! l["ovRev"].isInt() || ! l["fb"].isVoid() || ! l["rec"].isVoid())
                    bad.add ("lane " + juce::String (i) + " ovRev / fb / rec");
                if (l.hasProperty ("lo") || l.hasProperty ("hi"))
                    bad.add ("lane " + juce::String (i) + " lo / hi");
            }

        auto emptyArray = [&] (const char* k)
        {
            if (! t[k].isArray() || t[k].size() != 0)
                bad.add (juce::String (k) + " not []");
        };
        emptyArray ("voices");
        emptyArray ("menv");
        emptyArray ("grains");
        emptyArray ("sources");
        if (! (t["newest"].isInt() && static_cast<int> (t["newest"]) == -1))
            bad.add ("newest");
        auto zeros = [&] (const juce::var& v, int n, const juce::String& k)
        {
            bool ok = v.isArray() && v.size() == n;
            for (int i = 0; ok && i < n; ++i)
                ok = isNumber (v[i]) && static_cast<double> (v[i]) == 0.0;
            if (! ok)
                bad.add (k + " not " + juce::String (n) + " zeros");
        };
        zeros (t["counts"], 4, "counts");
        zeros (t["out"], 2, "out");
        if (! (t["limit"].isInt() && static_cast<int> (t["limit"]) == 0))
            bad.add ("limit");

        const auto r = t["rec"];
        if (keysOf (r) != juce::StringArray { "bus", "signal", "run", "slot", "pos", "len", "capped", "peak" })
            bad.add ("rec keys " + keysOf (r).joinIntoString (","));
        if (! (r["bus"].isInt() && static_cast<int> (r["bus"]) == wantBus))
            bad.add ("rec.bus " + r["bus"].toString() + " (want " + juce::String (wantBus) + ")");
        if (! (r["signal"].isBool() && ! static_cast<bool> (r["signal"]) && r["capped"].isBool() && ! static_cast<bool> (r["capped"])))
            bad.add ("rec.signal / capped");
        for (const char* k : { "run", "slot", "pos", "len" })
            if (! (isNumber (r[k]) && static_cast<double> (r[k]) == 0.0))
                bad.add (juce::String ("rec.") + k);
        zeros (r["peak"], 2, "rec.peak");

        if (! (t["modNorm"].getDynamicObject() != nullptr && keysOf (t["modNorm"]).isEmpty()))
            bad.add ("modNorm not {}");
        for (const char* k : { "lfoPh", "frozen", "modRouted" })
            if (t.hasProperty (k))
                bad.add (juce::String (k) + " present");
        // In the text the page parses: the nulls are literal nulls.
        if (! text.contains ("\"fb\": null") || ! text.contains ("\"rec\": null"))
            bad.add ("fb / rec not literal null");
        return bad.joinIntoString ("; ");
    }

    void rowTL31()
    {
        struct L { const char* name; CS in; int channels, bus; };
        const L cases[] = { { "stereo SC", CS::stereo(), 2, 1 }, { "mono SC", CS::mono(), 1, 1 }, { "SC disabled", CS::disabled(), 0, 0 } };
        Tally t;
        juce::String d;
        int keyCount = 0;
        for (const auto& c : cases)
        {
            auto p = make();
            const bool okLayout = p->setBusesLayout (layout (c.in, CS::stereo()));
            p->prepareToPlay (kFs, 512);
            const auto text = juce::JSON::toString (GiUiJson::telemetryToVar (*p), false);
            const auto parsed = juce::JSON::parse (text);
            keyCount = keysOf (parsed).size();
            const auto problems = telemetryProblems (parsed, text, c.bus);
            t.check (okLayout && p->getSidechainChannelCount() == c.channels && problems.isEmpty(), juce::String (c.name) + ": " + problems);
            d << (d.isEmpty() ? "" : ", ") << c.name << " (" << p->getSidechainChannelCount() << " ch) -> bus " << parsed["rec"]["bus"].toString();
        }
        row (t.bad == 0, "UTL31 PD8 telemetryToVar at defaults: exact key set, fb / rec null, no lo / hi, counts x4, rec.bus by layout",
             "meas " + t.detail (3) + ", " + juce::String (keyCount) + " top-level keys, rec.bus: " + d
                 + " | thr 13 keys exact, bus 1 / 1 / 0 | margin 0 | ctrl - (the disabled layout reads 0)");
    }

    //==========================================================================
    void runAll()
    {
        gTmp31 = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ogi-ug31-" + juce::String (juce::Time::currentTimeMillis()));
        gTmp31.createDirectory();
        struct Guard { ~Guard() { gTmp31.deleteRecursively(); } } guard;

        // The preset manager logs every save / load with its absolute path
        // (juce::Logger -> stderr): silenced for these rows only.
        struct NullLogger final : juce::Logger { void logMessage (const juce::String&) override {} } nullLogger;
        struct LoggerScope
        {
            explicit LoggerScope (juce::Logger* l) : previous (juce::Logger::getCurrentLogger()) { juce::Logger::setCurrentLogger (l); }
            ~LoggerScope() { juce::Logger::setCurrentLogger (previous); }
            juce::Logger* previous;
        } loggerScope (&nullLogger);

        section ("3.1  processor surface: presets / window curve / revisions / overview / telemetry (PLAN-3.1 T5)");
        if (want ("UPM")) { rowPM1(); rowsPM2(); rowsPM3(); rowPM4(); rowsPM5(); rowsPM6(); }
        if (want ("UWC")) rowsWC();
        if (want ("USR")) rowsSR();
        if (want ("UOV")) rowOV1();
        if (want ("UTL")) rowTL31();
    }
} }
