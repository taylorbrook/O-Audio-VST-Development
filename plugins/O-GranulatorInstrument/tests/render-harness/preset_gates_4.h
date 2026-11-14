/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 4 (Polish) gates (PLAN-4
    PD9 / PD10 / PD12 / PD14 / PD16). Included by main.cpp AFTER ui_gates_33.h.
    `--pg4` runs this file alone (OGI_ROWS filters); the full set runs it after
    ug33.

    Row names (PLAN-4 T1 grep: 0 prior hits each; the plan's PR* / FA3 names
    collide with parity rows PR1-12 and the 2.3 alloc row FA3, so the preset
    rows are FP* and the source-pin row is FSP1 — SUMMARY deviation 1):

      AU1    the auval default round-trip drift set over all 293 parameters
             (setValue (default) -> getValue () != default on the REAL
             parameter objects) == { lfo1_rate ... lfo4_rate } exactly
      AU1n   a skew-0.3 range at a drifting default joins the set
      FSP1   the curated factory sources (8 until 1.2.0, 41 since): each
             decodes to the manifest's channels / 48 kHz / frames;
             contentHash == kHash*, full-PCM hash == kFull*; id, name,
             category and root == the manifest's; each FLAC <= 3 145 728 B,
             total <= 25 165 824 B; the 1.0.0 set also against hand-written
             literal pins, shapes and names; the 2 stubs after the curated
             entries, only under OGI_TEST_HOOKS
      FSP1n  one pin perturbed (kFullDust ^ 1) -> FSP1 FAILS
      FR1 / FR1n / FR1r / FR1e / FR2, FLV1 / FLV1n, FSM1 / FSM1n, FPT1 /
             FPT1n: 1.2.0, the factory sources expansion (root write, level
             spread, seam, pitch); described where they are defined
      FP0    the bank table: 64 presets, 8 per category, unique names / seeds,
             "<Cat> · <Name>" <= 24 chars, no / \ :, validateSpec clean (no
             output_gain / rec_arm, ids, ranges, seed, factory ids, enabled
             stream -> listed slot), per-category feature coverage (R1: the
             feedback / reverb rules read per stream — a range rule holds when
             at least one ENABLED stream meets it)
      FP1    apply from fresh (no disk): engineering read-back == table
             (absent == default, the PD8 six exact 0), output_gain -7 dB kept,
             seed / custom scale, listed slots loaded / resolvedBy 4 / hash ==
             pin, fsTouches 0
      FP1n   s1_size authored as a LINEAR fraction -> read-back FAILS
      FP1n2  a file: slot -> fsTouches > 0 (FP1's no-disk predicate FAILS)
      FP2    20 s per preset (fixed note script, 120 bpm playhead): finite,
             peak <= 4.0, RMS 2-20 s > -60 dBFS; bank loudness spread <= 9 dB
      FP3    Rhythmic: every stream-0 grain onset on its density_sync grid
             (<= 1 sample) at 120 and 93 bpm
      FP4    load after dirty: 291 random params + custom scale + 3 masks +
             seed + foreign file: lanes (Live: the fixed preload) -> apply B ==
             fresh B, 10 s bitwise, 320 ordered pairs A = B + {1,8,13,32,63};
             the 64 fresh renders pairwise distinct
      FP4b   load after play (A 5 s at feedback 80 % on all four streams, notes off, apply B):
             parameters + seed / custom / masks + listed slots == fresh B;
             listed slots re-resolved (generation moved), not dirty
      FP5    the real path under the temp HOME: the constructor writes 64
             files + .factory-version + .bank-hash; loadPreset read-back ==
             table; save as user -> reload on a fresh instance == factory
             apply (params, state, 10 s bitwise); stamp arm: an edited file +
             a wrong stamp + an orphan "Old · Name.json" -> the next
             constructor restores the file, deletes the orphan, rewrites the
             stamp; a User/ file survives
      PG1    save guard: savePresetToFileGuarded refuses a target in
             Presets/Factory (file unchanged), accepts one elsewhere
      PG1n   testNoFactoryGuard -> the factory file is overwritten
      FP6    no click on a switch while playing: 8 pairs, max |d2| over
             [0.9, 1.3] s <= 2.0 x the larger held reference; FP6n
             swapKillHard -> FAILS
      UWF1   D55a: a take whose WAV write fails (hooks.failWav) -> the page's
             slot JSON carries wavFailed == true and slotRev moved; the next
             good take on that lane -> false (slotRev moved again)
      UWF1n  the field omitted (GiUiJson::testOmitWavFailed) -> the page reads
             no wavFailed -> UWF1 FAILS
      USV3   D55c: a take save whose WAV writer hangs before chunk 0 (a
             non-polling hold = a hung disk syscall): destroying the processor
             takes <= 500 ms; the completion runs once (`cancelled`); after the
             hold is released the detached writer exits once within 2 s, the
             ".part" is gone and the target was never written
      USV3n  the 3.2 in-worker write (testInlineWrite): the destructor waits
             for the hold (>= 3 s, released by a helper thread) -> USV3 FAILS

    Conventions as ui_gates_33.h: every row prints "meas ... | thr ... | ctrl
    ..."; an `n` row runs its parent's predicate on the broken input and PASSES
    only when that predicate FAILS ("ctrl FIRED").

  ==============================================================================
*/
#pragma once

#include <array>
#include <thread>
#include <map>
#include <set>
#include "FactoryPresets.h"

namespace { namespace pg4
{
    using eg21::ctrl;
    using eg24b::want;
    using eg24b::section;

    juce::File gTmp4;   // the Stage 4 temp tree (runAll creates / deletes it)

    //==========================================================================
    // AU1 / AU1n — auval "Parameter did not retain default value" (PD9)
    //
    // The JUCE 8.0.15 AU wrapper reports the NORMALISED default and round-trips
    // it through setValue / getValue. A parameter drifts when
    // getValue (after setValue (d)) != d. The set is frozen at the four LFO
    // rates (skew 0.3, 1 ulp at 0.5 Hz; documented, not fixed: every fix moves
    // the LOCKED spec). The PD8 snap-only ranges must not add a fifth member.
    bool drifts (juce::RangedAudioParameter& p)
    {
        const float d = p.getDefaultValue();
        const float before = p.getValue();
        p.setValue (d);
        const float back = p.getValue();
        p.setValue (before);
        return ! juce::exactlyEqual (back, d);
    }

    juce::RangedAudioParameter* findParam (juce::AudioProcessor& proc, const juce::String& id)
    {
        for (auto* q : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (q))
                if (r->getParameterID() == id)
                    return r;
        return nullptr;
    }

    juce::StringArray driftSet (juce::AudioProcessor& proc)
    {
        juce::StringArray s;
        for (auto* q : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (q))
                if (drifts (*r))
                    s.add (r->getParameterID());
        return s;
    }

    void rowsAU1()
    {
        auto p = make();
        int counted = 0;
        for (auto* q : p->getParameters())
            if (dynamic_cast<juce::RangedAudioParameter*> (q) != nullptr)
                ++counted;
        const auto set = driftSet (*p);
        const juce::StringArray expected { "lfo1_rate", "lfo2_rate", "lfo3_rate", "lfo4_rate" };
        bool exact = set.size() == expected.size();
        for (const auto& id : expected)
            exact = exact && set.contains (id);

        // The six PD8 rows: exact 0 after the round-trip AND outside the set.
        int exactZero = 0;
        for (const auto* id : { "output_gain", "rec_gain", "s1_level", "s2_level", "s3_level", "s4_level" })
            if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (findParam (*p, id)))
            {
                auto& r = static_cast<juce::RangedAudioParameter&> (*f);
                r.setValue (r.getDefaultValue());
                if (juce::exactlyEqual (f->get(), 0.0f) && ! set.contains (id))
                    ++exactZero;
            }

        row (counted == 293 && exact && exactZero == 6, "AU1 PD9 auval default round-trip drift set == { lfo1..4_rate } (293 params)",
             "meas " + juce::String (counted) + " params, set { " + set.joinIntoString (", ") + " }, PD8 rows exact 0 "
                 + juce::String (exactZero) + " / 6 | thr set == 4 LFO rates, 6 / 6 | ctrl AU1n");

        // AU1n: the same predicate with a skew-0.3 range at a drifting default.
        juce::AudioParameterFloat tmp (juce::ParameterID { "au1n_tmp", 1 }, "AU1n",
                                       juce::NormalisableRange<float> (0.01f, 20.0f, 0.0f, 0.3f), 0.5f);
        auto withTmp = set;
        if (drifts (tmp))
            withTmp.add ("au1n_tmp");
        bool exactN = withTmp.size() == expected.size();
        for (const auto& id : expected)
            exactN = exactN && withTmp.contains (id);
        row (! exactN, "AU1n control: a skew-0.3 range at default 0.5 joins the set -> FAILS (predicate must FAIL)",
             "meas set " + juce::String (withTmp.size()) + " members | thr predicate FAILS | " + ctrl (! exactN, "AU1 predicate"));
    }

    //==========================================================================
    // FSP1 / FSP1n — the curated factory sources (PD6; 1.2.0: every source)
    //
    // Two independent expectations: kSources is generated from the manifest
    // (tools/factory_table.py), kOld8 is hand-written and never regenerated.
    // The 1.0.0 set must match BOTH, so a manifest edit or a regenerated
    // table cannot move one of its pins, its shape, its index or its name.
    struct SourceSpec { const char* id; const char* name; const char* category; int channels; int frames; int rootNote; double rootCents; bool seam; bool synthetic; int refPartial; };
    constexpr SourceSpec kSources[] = {
       #include "factory_specs.inc"
    };
    constexpr int kSourceCount = (int) (sizeof (kSources) / sizeof (kSources[0]));

    struct OldPin { const char* id; int channels; int frames; std::uint64_t hash, full; };
    constexpr OldPin kOld8[] = {
        { "harmonic",  1, 384000, 0x8d3f53401dd512abull, 0xc66d8c372e1a6466ull },
        { "pluck",     1, 144000, 0xced194ec27e461b4ull, 0xd852d9429eaded57ull },
        { "bell",      1, 288000, 0xf0195e2384e5b366ull, 0x8d3f987172d73125ull },
        { "vowel",     1, 384000, 0x0e61dc5699763d74ull, 0xd7725f3426607bf1ull },
        { "noise_bed", 2, 288000, 0x3ab6faaf08e4f577ull, 0x3859a39daead5356ull },
        { "dust",      2, 288000, 0x77aae6be696b9d35ull, 0x139ab02c9dab4494ull },
        { "drone",     2, 576000, 0x428c82363a9af2c5ull, 0xa89ce6f4c53fa4b5ull },
        { "pulse",     1, 192000, 0x09cc60d1e810229cull, 0x1ee8522dcdb667bbull },
    };
    constexpr std::int64_t kFactoryBudget = 25165824;   // 24 MiB (1.2.0; was 8 MiB)
    constexpr std::int64_t kFactoryEach   = 3145728;    // 3 MiB

    struct Fsp1 { int good = 0; int old8 = 0; std::int64_t total = 0, largest = 0; juce::String detail; };

    Fsp1 runFsp1 (std::uint64_t perturbDustFull)
    {
        Fsp1 r;
        for (int k = 0; k < std::min (kSourceCount, ogi::kCuratedFactoryCount); ++k)
        {
            const auto e = ogi::factoryEntry (k);
            const auto& spec = kSources[k];
            ogi::FactoryEntry f;
            const bool found = e.id != nullptr && juce::String (e.id) == spec.id && ogi::findFactory (juce::String (spec.id), f) && f.data == e.data;
            const bool meta = found && e.name != nullptr && juce::String::fromUTF8 (e.name) == juce::String::fromUTF8 (spec.name)
                           && e.category != nullptr && juce::String (e.category) == spec.category
                           && e.rootNote == spec.rootNote && std::abs ((double) e.rootFineCt - spec.rootCents) < 1.0e-4;
            ogi::SlotLoader::Pristine p;
            juce::String err;
            const bool dec = found && ogi::SlotLoader::decodeMemoryFlac (e.data, e.size, e.id, 48000.0, p, err);
            const bool shape = dec && p.pcm.getNumChannels() == spec.channels && p.pcm.getNumSamples() == spec.frames
                            && juce::exactlyEqual (p.rate, 48000.0);
            const auto h = dec ? ogi::gihash::contentHash (p.pcm) : 0;
            const auto full = dec ? factorygen::fullPcmHash (p.pcm) : 0;
            const auto pinFull = juce::String (spec.id) == "dust" ? (e.full ^ perturbDustFull) : e.full;
            const bool pins = h == e.hash && e.hash != 0 && full == pinFull && pinFull != 0;
            const bool size = e.size > 0 && (std::int64_t) e.size <= kFactoryEach;
            bool frozen = true;
            if (k < 8)
            {
                const auto& o = kOld8[k];
                frozen = found && juce::String (e.id) == o.id && shape && p.pcm.getNumChannels() == o.channels && p.pcm.getNumSamples() == o.frames
                      && h == o.hash && full == o.full && e.hash == o.hash && e.full == o.full
                      && juce::String::fromUTF8 (e.name) == ogi::factoryDisplayName (o.id)
                      && e.rootNote == 48 && juce::exactlyEqual (e.rootFineCt, 0.0f);
                if (frozen)
                    ++r.old8;
            }
            r.total += (std::int64_t) e.size;
            r.largest = std::max (r.largest, (std::int64_t) e.size);
            if (found && meta && shape && pins && size && frozen)
                ++r.good;
            else
                r.detail << " " << spec.id << (found ? "" : " not-found") << (meta ? "" : " meta") << (shape ? "" : " shape") << (pins ? "" : " PIN")
                         << (size ? "" : " size") << (frozen ? "" : " OLD8");
        }
        return r;
    }

    void rowsFSP1()
    {
        const auto a = runFsp1 (0);
        const int N = kSourceCount;
        const int curated = ogi::factoryCount() - (OGI_HAS_TEST_FACTORY ? 2 : 0);
        ogi::FactoryEntry stub;
        const bool stubsTail = OGI_HAS_TEST_FACTORY && ogi::findFactory ("stub_tone", stub) && ogi::factoryEntry (N).data == stub.data
                            && juce::String (ogi::factoryEntry (N + 1).id) == "stub_noise";
        const bool ok = a.good == N && a.old8 == 8 && a.total <= kFactoryBudget && a.largest <= kFactoryEach && curated == N && ogi::kCuratedFactoryCount == N && stubsTail;
        row (ok, "FSP1 PD6 " + juce::String (N) + " curated factory sources: shape, kHash + full-PCM pins, ids, names, roots, size budget; the 1.0.0 set against its literals",
             "meas " + juce::String (a.good) + " / " + juce::String (N) + " good" + a.detail + ", 1.0.0 set " + juce::String (a.old8) + " / 8 on its literal pins and names, total "
                 + juce::String (a.total) + " B, largest " + juce::String (a.largest) + " B, curated " + juce::String (curated) + " + stubs at "
                 + juce::String (N) + " / " + juce::String (N + 1) + " " + (stubsTail ? "yes" : "no")
                 + " | thr " + juce::String (N) + " / " + juce::String (N) + ", 8 / 8, <= " + juce::String (kFactoryBudget) + ", <= " + juce::String (kFactoryEach) + " | ctrl FSP1n");
        const auto b = runFsp1 (1);
        const bool fired = b.good == N - 1;
        row (fired, "FSP1n control: kFullDust ^ 1 -> FSP1 FAILS (predicate must FAIL)",
             "meas " + juce::String (b.good) + " / " + juce::String (N) + " good" + b.detail + " | thr " + juce::String (N - 1) + " / " + juce::String (N)
                 + " | " + ctrl (fired, "FSP1 predicate"));
    }

    //==========================================================================
    // FP — the factory bank (PD1 / PD2 / PD14)
    namespace F = ogi::factory;
    using Def = OuariconPresetManager::FactoryPresetDef;
    constexpr double kFsP = 48000.0;

    struct Bank
    {
        std::vector<F::FactoryPresetSpec> specs;
        std::vector<Def> defs;
    };

    const Bank& bank()
    {
        static const Bank b = []
        {
            Bank x;
            x.specs = F::factoryPresetSpecs();
            // PLAN-R1 PD27: the table readers below see the keys a preset really
            // carries ("s*_x" -> the four "sN_x"); makeFactoryPresets expands the
            // authored table itself, and FP0 validates the authored form too.
            for (auto& s : x.specs)
                F::expandSpecValues (s.values);
            auto p = make();
            x.defs = F::makeFactoryPresets (p->parameters);
            return x;
        }();
        return b;
    }

    juce::String catOf (const F::FactoryPresetSpec& s)
    {
        return juce::String::fromUTF8 (s.name).upToFirstOccurrenceOf (juce::String::fromUTF8 (" \xc2\xb7 "), false, false);
    }
    juce::String shortOf (const F::FactoryPresetSpec& s)
    {
        return juce::String::fromUTF8 (s.name).fromFirstOccurrenceOf (juce::String::fromUTF8 (" \xc2\xb7 "), false, false);
    }

    bool has (const F::FactoryPresetSpec& s, const char* id)
    {
        for (const auto& [k, v] : s.values)
            if (juce::String (k) == id)
                return true;
        return false;
    }
    /** The LAST pair wins (PLAN-R1 PD27: a literal "sN_x" after a wildcard overrides it). */
    float val (const F::FactoryPresetSpec& s, const char* id, float def)
    {
        float out = def;
        for (const auto& [k, v] : s.values)
            if (juce::String (k) == id)
                out = v;
        return out;
    }
    struct Route { int src = 0; juce::String dest; float depth = 0.0f; };
    std::vector<Route> routesOf (const F::FactoryPresetSpec& s)
    {
        std::vector<Route> r;
        for (int m = 1; m <= 16; ++m)
        {
            const auto pre = "mod" + juce::String (m) + "_";
            const int src = (int) val (s, (pre + "source").toRawUTF8(), 0.0f);
            const int dst = (int) val (s, (pre + "dest").toRawUTF8(), 0.0f);
            if (src > 0 && dst > 0)
                r.push_back ({ src, juce::String (GiParamIds::kModDestIds[dst]), val (s, (pre + "depth").toRawUTF8(), 0.0f) });
        }
        return r;
    }
    bool streamOn (const F::FactoryPresetSpec& s, int n)
    {
        return val (s, ("s" + juce::String (n) + "_enable").toRawUTF8(), n == 1 ? 1.0f : 0.0f) > 0.5f;
    }
    /** A per-stream row of the preset (sN_<suffix>). */
    float sval (const F::FactoryPresetSpec& s, int n, const char* suffix, float def)
    {
        return val (s, ("s" + juce::String (n) + "_" + suffix).toRawUTF8(), def);
    }
    /** PLAN-R1 PD16 (FP0): a feedback / reverb rule holds when at least one
        ENABLED stream meets it. */
    bool anyOnStream (const F::FactoryPresetSpec& s, const std::function<bool (int)>& rule)
    {
        for (int n = 1; n <= 4; ++n)
            if (streamOn (s, n) && rule (n))
                return true;
        return false;
    }

    /** Runs fn (i) for i in [0, n) on a few threads (each job owns its processors). */
    void parallelFor (int n, const std::function<void (int)>& fn)
    {
        const int T = std::max (1, std::min (n, (int) std::thread::hardware_concurrency() - 2));
        std::atomic<int> next { 0 };
        std::vector<std::thread> th;
        for (int t = 0; t < T; ++t)
            th.emplace_back ([&] { for (int i = next++; i < n; i = next++) fn (i); });
        for (auto& t : th)
            t.join();
    }

    //==========================================================================
    // FL1 — 1.1.0: a built-in source picked for a lane (loadFactorySource)
    //==========================================================================
    void rowFL1()
    {
        eg21::Session s (kFsP, eg21::P {}, eg21::kSeed, 512);
        auto& p = *s.p;
        const auto e0 = p.loadFactorySource (0, "drone");
        p.getLoader().waitForIdle (20000);
        const auto gen0 = p.getSlotInfo (0).generation;
        const auto gen3 = p.getSlotInfo (3).generation;

        const auto e2 = p.loadFactorySource (2, "noise_bed");
        p.getLoader().waitForIdle (20000);
        const auto i0 = p.getSlotInfo (0), i2 = p.getSlotInfo (2);
        const bool loaded = e0.isEmpty() && e2.isEmpty()
                         && i0.loaded && i0.source == "factory:drone" && i0.name == "Drone" && i0.contentHash == ogi::kHashDrone
                         && i2.loaded && i2.source == "factory:noise_bed" && i2.name == "Noise Bed"
                         && i2.hasHash && i2.contentHash == ogi::kHashNoiseBed && i2.resolvedBy == 4 && ! i2.embed && ! i2.missing;
        const bool others = i0.generation == gen0 && ! p.getSlotInfo (1).loaded && p.getSlotInfo (1).source.isEmpty();

        const auto eU = p.loadFactorySource (3, "no_such_source");
        const auto eB = p.loadFactorySource (4, "drone");
        p.getLoader().waitForIdle (20000);
        const auto i3 = p.getSlotInfo (3);
        const bool refused = eU == "unknown" && eB == "badSlot" && i3.generation == gen3 && ! i3.loaded && i3.source.isEmpty();

        bool saved = false;
        for (const auto& x : p.getLoader().getSavedSlots())
            saved = saved || (x.index == 2 && x.source == "factory:noise_bed" && x.name == "Noise Bed" && ! x.embed && x.blob.isEmpty());

        row (loaded && others && refused && saved,
             "FL1 1.1.0 loadFactorySource: lane 3 <- noise_bed (name, hash, rung 4, never embedded), other lanes untouched, unknown id / bad slot refused, saved as factory:<id>",
             "meas loaded " + juce::String (loaded ? "yes" : "no") + " (\"" + i2.name + "\", rung " + juce::String (i2.resolvedBy)
                 + "), others untouched " + (others ? "yes" : "no") + ", refusals \"" + eU + "\" / \"" + eB + "\" " + (refused ? "ok" : "BAD")
                 + ", saved " + (saved ? "yes" : "no") + " | thr all four");
    }

    //==========================================================================
    // 1.2.0 — the factory sources expansion (improve factory-sources-expansion)
    //
    //   FR1    every source picked through loadFactorySource from a non-default
    //          root: the lane's stream reads the source's root (note exactly,
    //          cents within 0.001), the other three streams keep theirs
    //   FR1n   testNoFactoryRootWrite -> FR1 FAILS
    //   FR1r   unknown id / bad slot / a take recording into the lane: refused,
    //          both parameters as they were
    //   FR1e   a pick whose root is already there: zero gestures (and the pick
    //          before it: two, so the listener is live)
    //   FR2    a session restore, a preset load, a dropped file and Clear leave
    //          the root alone (the source is contrabassoon, G#0 -19 ct, the
    //          root 55 / +12: a write would show)
    //   FLV1   every source: peak <= -1 dBFS (+ one 16-bit step: the 1.0.0
    //          noise sources were rounded to that grid AFTER the cap), loud
    //          frame (90th-percentile 400 ms frame of the channel mean) in
    //          [-33, -18] dBFS;  FLV1n  one source + 12 dB -> FAILS
    //   FSM1   the seam sources read circularly: centre 100 ms of the fold
    //          within +-1.5 dB of the mean power of the two segments it mixed
    //          (tests/fixtures/factory-seam), wrap step <= the 99.9th
    //          percentile sample step;  FSM1n  the linear-gain fold -> FAILS
    //   FPT1   crystal / guitar_overtones / guitar_drift / contrabassoon picked,
    //          C3 played, one long grain: the reference partial within +-10 ct
    //          of k x 130.81 Hz;  FPT1n  the root left at C3 -> all four FAIL
    //==========================================================================
    using eg21::setP;
    using eg21::rawOf;
    using eg22::sn;

    juce::String f3 (double v) { return juce::String (v, 3); }

    struct Fr1 { int good = 0, offC3 = 0; juce::String detail; };

    Fr1 runFr1 (bool noWrite)
    {
        Fr1 r;
        eg21::Session s (kFsP, eg21::P {}, eg21::kSeed, 512);
        auto& p = *s.p;
        p.testNoFactoryRootWrite = noWrite;
        for (int k = 0; k < kSourceCount; ++k)
        {
            const auto& spec = kSources[k];
            const int lane = k % 4;
            float note0[4], fine0[4];
            for (int n = 1; n <= 4; ++n)   // sentinels no source carries
            {
                setP (p, sn (n, "root_note"), (float) (60 + n));
                setP (p, sn (n, "root_fine"), 30.0f + (float) n);
                note0[n - 1] = rawOf (p, sn (n, "root_note"));
                fine0[n - 1] = rawOf (p, sn (n, "root_fine"));
            }
            const auto err = p.loadFactorySource (lane, spec.id);
            p.getLoader().waitForIdle (20000);
            const float note = rawOf (p, sn (lane + 1, "root_note")), fine = rawOf (p, sn (lane + 1, "root_fine"));
            bool others = true;
            for (int n = 1; n <= 4; ++n)
                if (n != lane + 1)
                    others = others && eg21::sameF (rawOf (p, sn (n, "root_note")), note0[n - 1]) && eg21::sameF (rawOf (p, sn (n, "root_fine")), fine0[n - 1]);
            const auto info = p.getSlotInfo (lane);
            const bool loaded = err.isEmpty() && info.loaded && info.source == "factory:" + juce::String (spec.id) && info.name == juce::String::fromUTF8 (spec.name);
            const bool root = juce::exactlyEqual (note, (float) spec.rootNote) && std::abs ((double) fine - spec.rootCents) <= 0.001;
            if (spec.rootNote != 48 || std::abs (spec.rootCents) > 0.0)
                ++r.offC3;
            if (loaded && root && others)
                ++r.good;
            else if (r.detail.length() < 400)
                r.detail << " " << spec.id << (loaded ? "" : " not-loaded") << (root ? "" : " root " + juce::String (note, 0) + " / " + f3 (fine))
                         << (others ? "" : " OTHER-STREAM");
        }
        return r;
    }

    void rowsFR1()
    {
        const int N = kSourceCount;
        const auto a = runFr1 (false);
        row (a.good == N && a.offC3 >= 5,
             "FR1 1.2.0 every factory pick writes the lane's stream root (note exact, cents within 0.001), from sentinel roots; the other streams keep theirs",
             "meas " + juce::String (a.good) + " / " + juce::String (N) + " good" + a.detail + ", " + juce::String (a.offC3)
                 + " sources off C3 / 0 ct | thr " + juce::String (N) + " / " + juce::String (N) + ", >= 5 off C3 | ctrl FR1n");
        const auto b = runFr1 (true);
        row (b.good == 0, "FR1n control: testNoFactoryRootWrite (the lane loads, the write is skipped) -> FR1 FAILS (predicate must FAIL)",
             "meas " + juce::String (b.good) + " / " + juce::String (N) + " good | thr 0 | " + ctrl (b.good == 0, "FR1 predicate, every source"));
    }

    void rowFR1r()
    {
        using RS = ogi::RecState;
        const auto sentinels = [] (GranulatorInstrumentProcessor& p)
        {
            for (int n = 1; n <= 4; ++n)
            {
                setP (p, sn (n, "root_note"), (float) (60 + n));
                setP (p, sn (n, "root_fine"), 30.0f + (float) n);
            }
        };
        const auto snapshot = [] (GranulatorInstrumentProcessor& p)
        {
            std::vector<float> v;
            for (int n = 1; n <= 4; ++n)
            {
                v.push_back (rawOf (p, sn (n, "root_note")));
                v.push_back (rawOf (p, sn (n, "root_fine")));
            }
            return v;
        };
        const auto same = [] (const std::vector<float>& a, const std::vector<float>& b)
        {
            bool ok = a.size() == b.size();
            for (size_t i = 0; ok && i < a.size(); ++i)
                ok = eg21::sameF (a[i], b[i]);
            return ok;
        };

        eg21::Session s (kFsP, eg21::P {}, eg21::kSeed, 512);
        sentinels (*s.p);
        const auto before = snapshot (*s.p);
        const auto eU = s.p->loadFactorySource (1, "no_such_source");
        const auto eB = s.p->loadFactorySource (4, "contrabassoon");
        const auto eN = s.p->loadFactorySource (-1, "contrabassoon");
        s.p->getLoader().waitForIdle (20000);
        const bool keptA = same (before, snapshot (*s.p));

        // A take recording into lane 2 (the ug32 CL1 (b) stimulus).
        eg21::Session r (kFsP, eg26a::recBase (1, 0, { { "rec_length", 0.6f } }), eg21::kSeed, 512);
        r.load (1, eg21::noiseBuf (2, 24000, 0.2f, 263), kFsP);
        eg26a::inject (r, eg26a::ScSig { 0.5f, 265 });
        r.record = false;
        sentinels (*r.p);
        r.run (4096, 512);
        eg26a::armAt (r, r.pos);
        r.run (9600, 512);
        const auto st = eg26a::pr (r).state;
        const auto beforeR = snapshot (*r.p);
        const auto eR = r.p->loadFactorySource (1, "contrabassoon");
        const bool keptR = same (beforeR, snapshot (*r.p));
        // ... and the same pick on another lane is not refused, and writes.
        const auto eO = r.p->loadFactorySource (2, "contrabassoon");
        const bool wroteO = juce::exactlyEqual (rawOf (*r.p, sn (3, "root_note")), 20.0f) && std::abs (rawOf (*r.p, sn (3, "root_fine")) + 19.0f) <= 0.001f;
        r.run (28800, 512);
        r.p->commitSync();
        r.p->getLoader().waitForIdle (20000);
        r.run (2048, 256);

        const bool ok = eU == "unknown" && eB == "badSlot" && eN == "badSlot" && keptA && st == RS::Recording && eR == "recording" && keptR && eO.isEmpty() && wroteO;
        row (ok, "FR1r 1.2.0 a refused pick (unknown id, bad slot, a take recording into the lane) leaves both root parameters as they were",
             "meas refusals \"" + eU + "\" / \"" + eB + "\" / \"" + eN + "\" roots kept " + (keptA ? "yes" : "NO") + "; " + eg26a::stName (st) + " -> \"" + eR
                 + "\" roots kept " + (keptR ? "yes" : "NO") + " | thr all | " + ctrl (eO.isEmpty() && wroteO, "the same pick on another lane writes 20 / -19"));
    }

    struct GestureCount : juce::AudioProcessorParameter::Listener
    {
        int begins = 0, ends = 0, values = 0;
        void parameterValueChanged (int, float) override { ++values; }
        void parameterGestureChanged (int, bool starting) override { ++(starting ? begins : ends); }
    };

    void rowFR1e()
    {
        eg21::Session s (kFsP, eg21::P {}, eg21::kSeed, 512);
        auto& p = *s.p;
        auto* note = p.parameters.getParameter (sn (2, "root_note"));
        auto* fine = p.parameters.getParameter (sn (2, "root_fine"));
        GestureCount g;
        note->addListener (&g);
        fine->addListener (&g);
        const auto pick = [&] (const char* id)
        {
            const GestureCount before = g;
            p.loadFactorySource (1, id);
            p.getLoader().waitForIdle (20000);
            return std::array<int, 3> { g.begins - before.begins, g.ends - before.ends, g.values - before.values };
        };
        const auto c3   = pick ("drone");           // a fresh lane is at 48 / 0 and so is drone: nothing to write
        const auto low  = pick ("contrabassoon");   // 20 / -19: two parameters move
        const auto same = pick ("contrabassoon");   // already there
        const auto half = pick ("sub");             // 24 / 0: both move again (the fine back to 0)
        const auto one  = pick ("drone");           // 48 / 0: only the note moves
        note->removeListener (&g);
        fine->removeListener (&g);
        const bool ok = c3 == std::array<int, 3> { 0, 0, 0 } && same == std::array<int, 3> { 0, 0, 0 }
                     && low == std::array<int, 3> { 2, 2, 2 } && half == std::array<int, 3> { 2, 2, 2 } && one == std::array<int, 3> { 1, 1, 1 };
        const auto t = [] (const std::array<int, 3>& a) { return juce::String (a[0]) + " / " + juce::String (a[1]) + " / " + juce::String (a[2]); };
        row (ok, "FR1e 1.2.0 a pick whose root is already on the stream makes no gesture; one that moves a parameter makes exactly one gesture for it",
             "meas begins / ends / values: drone on a fresh lane " + t (c3) + ", contrabassoon " + t (low) + ", contrabassoon again " + t (same)
                 + ", sub " + t (half) + ", drone " + t (one) + " | thr 0 / 0 / 0, 2 / 2 / 2, 0 / 0 / 0, 2 / 2 / 2, 1 / 1 / 1 | "
                 + ctrl (low[0] == 2, "the listener counts the gestures of a pick that writes"));
    }

    void rowFR2()
    {
        const auto rootIs = [] (GranulatorInstrumentProcessor& p, float n, float f)
        {
            return juce::exactlyEqual (rawOf (p, sn (1, "root_note")), n) && std::abs (rawOf (p, sn (1, "root_fine")) - f) <= 0.001f;
        };
        const auto holds = [] (GranulatorInstrumentProcessor& p)
        {
            const auto i = p.getSlotInfo (0);
            return i.loaded && i.source == "factory:contrabassoon";
        };

        // A: the pick writes 20 / -19 (the stimulus), then the user moves the root to 55 / +12.
        eg21::Session a (kFsP, eg21::P {}, eg21::kSeed, 512);
        a.p->loadFactorySource (0, "contrabassoon");
        a.p->getLoader().waitForIdle (20000);
        const bool wrote = rootIs (*a.p, 20.0f, -19.0f) && holds (*a.p);
        setP (*a.p, sn (1, "root_note"), 55.0f);
        setP (*a.p, sn (1, "root_fine"), 12.0f);
        const auto blob = eg26b::save (a);
        const auto file = gTmp4.getChildFile ("fr2-preset.json");
        const bool saved = a.p->getPresetManager().savePresetToFile (file);

        eg21::Session b (kFsP, eg21::P {}, eg21::kSeed, 512);   // session restore
        eg26b::load (b, blob);
        eg26b::settle (b);
        const bool session = holds (*b.p) && rootIs (*b.p, 55.0f, 12.0f);

        eg21::Session c (kFsP, eg21::P {}, eg21::kSeed, 512);   // preset load
        const bool loadedPreset = saved && c.p->getPresetManager().loadPresetFromFile (file);
        eg26b::settle (c);
        const bool preset = loadedPreset && holds (*c.p) && rootIs (*c.p, 55.0f, 12.0f);

        // A dropped file, then Clear, on the lane that holds the factory source.
        const auto wav = eg26b::writeWav (gTmp4.getChildFile ("fr2-drop.wav"), eg21::noiseBuf (1, 24000, 0.2f, 1201), kFsP, 24);
        const bool queued = c.p->importDroppedFile (0, wav, "fr2-drop.wav");
        eg26b::settle (c);
        const auto ci = c.p->getSlotInfo (0);
        const bool drop = queued && ci.loaded && ! ci.source.startsWith ("factory:") && rootIs (*c.p, 55.0f, 12.0f);
        const auto ce = c.p->clearSlot (0);
        eg26b::settle (c);
        const bool clear = ce.isEmpty() && ! c.p->getSlotInfo (0).loaded && rootIs (*c.p, 55.0f, 12.0f);

        row (wrote && session && preset && drop && clear,
             "FR2 1.2.0 a session restore, a preset load, a dropped file and Clear leave the root alone (contrabassoon on the lane, root moved to 55 / +12)",
             juce::String ("meas session ") + (session ? "55 / +12" : "MOVED") + ", preset " + (preset ? "55 / +12" : "MOVED") + ", drop " + (drop ? "55 / +12" : "MOVED")
                 + ", clear " + (clear ? "55 / +12" : "MOVED") + " | thr all four | " + ctrl (wrote, "the pick itself wrote 20 / -19 on this lane"));
    }

    //--------------------------------------------------------------------------
    // FLV1 / FLV1n — the level spread
    bool decodeEntry (const char* id, ogi::SlotLoader::Pristine& p)
    {
        ogi::FactoryEntry e;
        juce::String err;
        return ogi::findFactory (id, e) && ogi::SlotLoader::decodeMemoryFlac (e.data, e.size, e.id, 48000.0, p, err);
    }

    struct Level { double peakDb = 0.0, loudDb = 0.0; };

    /** Peak over every channel; the loud frame = the 90th percentile (linear
        interpolation, as numpy) of the level in dB of the 400 ms frames of the
        channel mean. The measure of tools/factory_ingest.py. */
    Level levelOf (const juce::AudioBuffer<float>& pcm, double scale)
    {
        Level r;
        const int C = pcm.getNumChannels(), n = pcm.getNumSamples(), hop = 19200, F = n / hop;
        double pk = 0.0;
        for (int c = 0; c < C; ++c)
            for (int i = 0; i < n; ++i)
                pk = std::max (pk, std::abs ((double) pcm.getSample (c, i)) * scale);
        r.peakDb = 20.0 * std::log10 (std::max (pk, 1.0e-12));
        std::vector<double> f;
        for (int k = 0; k < F; ++k)
        {
            double acc = 0.0;
            for (int i = 0; i < hop; ++i)
            {
                double m = 0.0;
                for (int c = 0; c < C; ++c)
                    m += (double) pcm.getSample (c, k * hop + i);
                m *= scale / (double) C;
                acc += m * m;
            }
            f.push_back (10.0 * std::log10 (acc / (double) hop + 1.0e-12));
        }
        std::sort (f.begin(), f.end());
        if (f.empty())
            return r;
        const double pos = 0.9 * (double) (f.size() - 1);
        const size_t lo = (size_t) pos;
        r.loudDb = lo + 1 < f.size() ? f[lo] + (f[lo + 1] - f[lo]) * (pos - (double) lo) : f[lo];
        return r;
    }

    constexpr double kPeakMaxDb = -1.0 + 0.0003;   // -1 dBFS + one 16-bit step at that level (0.00030 dB)
    bool levelOk (const Level& l) { return l.peakDb <= kPeakMaxDb && l.loudDb >= -33.0 && l.loudDb <= -18.0; }

    void rowsFLV1()
    {
        int good = 0;
        double peakMax = -200.0, loudMin = 0.0, loudMax = -200.0;
        juce::String detail, loudest, quietest;
        for (int k = 0; k < kSourceCount; ++k)
        {
            ogi::SlotLoader::Pristine p;
            if (! decodeEntry (kSources[k].id, p))
            {
                detail << " " << kSources[k].id << " not-decoded";
                continue;
            }
            const auto l = levelOf (p.pcm, 1.0);
            peakMax = std::max (peakMax, l.peakDb);
            if (l.loudDb < loudMin) { loudMin = l.loudDb; quietest = kSources[k].id; }
            if (l.loudDb > loudMax) { loudMax = l.loudDb; loudest = kSources[k].id; }
            if (levelOk (l))
                ++good;
            else
                detail << " " << kSources[k].id << " peak " << f3 (l.peakDb) << " loud " << juce::String (l.loudDb, 2);
        }
        row (good == kSourceCount, "FLV1 1.2.0 every factory source: peak <= -1 dBFS (+ one 16-bit step), loud frame in [-33, -18] dBFS",
             "meas " + juce::String (good) + " / " + juce::String (kSourceCount) + " good" + detail + ", peak max " + juce::String (peakMax, 4) + " dBFS, loud frame "
                 + juce::String (loudMin, 2) + " (" + quietest + ") ... " + juce::String (loudMax, 2) + " (" + loudest + ") | thr peak <= " + juce::String (kPeakMaxDb, 4)
                 + ", loud in [-33, -18] | margin floor " + juce::String (loudMin + 33.0, 2) + " dB, ceiling " + juce::String (-18.0 - loudMax, 2) + " dB | ctrl FLV1n");

        ogi::SlotLoader::Pristine h;
        const bool dec = decodeEntry ("harmonic", h);
        const auto l0 = dec ? levelOf (h.pcm, 1.0) : Level {};
        const auto l12 = dec ? levelOf (h.pcm, 3.9810717055349722) : Level {};
        const bool fired = dec && levelOk (l0) && ! levelOk (l12);
        row (fired, "FLV1n control: harmonic scaled + 12 dB -> FLV1 FAILS (predicate must FAIL)",
             "meas as shipped peak " + f3 (l0.peakDb) + " loud " + juce::String (l0.loudDb, 2) + " (passes), + 12 dB peak " + f3 (l12.peakDb) + " loud "
                 + juce::String (l12.loudDb, 2) + " | thr predicate FAILS | " + ctrl (fired, "FLV1 predicate"));
    }

    //--------------------------------------------------------------------------
    // FSM1 / FSM1n — the folded seam
    struct Seam { bool valid = false; double foldDb = 0.0, linDb = 0.0, stepRatio = 0.0; };

    double meanPowerMono (const juce::AudioBuffer<float>& b, int from, int count)
    {
        double acc = 0.0;
        const int C = b.getNumChannels();
        for (int i = 0; i < count; ++i)
        {
            double m = 0.0;
            for (int c = 0; c < C; ++c)
                m += (double) b.getSample (c, from + i);
            m /= (double) C;
            acc += m * m;
        }
        return acc / (double) count;
    }

    Seam seamOf (const SourceSpec& spec)
    {
        Seam r;
        ogi::SlotLoader::Pristine y, fx;
        juce::MemoryBlock mb;
        juce::String err;
        const auto f = gGoldenDir.getSiblingFile ("fixtures").getChildFile ("factory-seam").getChildFile (juce::String (spec.id) + ".flac");
        if (! decodeEntry (spec.id, y) || ! f.loadFileAsData (mb) || ! ogi::SlotLoader::decodeMemoryFlac (mb.getData(), mb.getSize(), spec.id, 48000.0, fx, err))
            return r;
        const int C = y.pcm.getNumChannels(), N = y.pcm.getNumSamples();
        if (fx.pcm.getNumChannels() != C || fx.pcm.getNumSamples() != 14400 || N < 96000)
            return r;
        // The fold is the first 0.5 s of the shipped file; its centre 100 ms is [9600, 14400).
        const double ref = 0.5 * (meanPowerMono (fx.pcm, 0, 4800) + meanPowerMono (fx.pcm, 4800, 4800));
        if (! (ref > 1.0e-10))
            return r;
        r.foldDb = 10.0 * std::log10 (meanPowerMono (y.pcm, 9600, 4800) / ref);
        r.linDb  = 10.0 * std::log10 (meanPowerMono (fx.pcm, 9600, 4800) / ref);
        // The wrap: last sample -> first sample, against the body's own sample steps.
        std::vector<float> steps;
        steps.reserve ((size_t) C * (size_t) (N - 1));
        double wrap = 0.0;
        for (int c = 0; c < C; ++c)
        {
            const float* d = y.pcm.getReadPointer (c);
            for (int i = 1; i < N; ++i)
                steps.push_back (std::abs (d[i] - d[i - 1]));
            wrap = std::max (wrap, (double) std::abs (d[0] - d[N - 1]));
        }
        std::sort (steps.begin(), steps.end());
        const double pos = 0.999 * (double) (steps.size() - 1);
        const size_t lo = (size_t) pos;
        const double p999 = (double) steps[lo] + ((double) steps[std::min (lo + 1, steps.size() - 1)] - (double) steps[lo]) * (pos - (double) lo);
        r.stepRatio = p999 > 0.0 ? wrap / p999 : 1.0e9;
        r.valid = true;
        return r;
    }

    void rowsFSM1()
    {
        constexpr double tol = 1.5;
        int seams = 0, good = 0, fired = 0;
        double worstFold = 0.0, nearestLin = 1.0e9, worstStep = 0.0;
        juce::String detail, ctl;
        for (int k = 0; k < kSourceCount; ++k)
        {
            if (! kSources[k].seam)
                continue;
            ++seams;
            const auto s = seamOf (kSources[k]);
            const bool ok = s.valid && std::abs (s.foldDb) <= tol && s.stepRatio <= 1.0;
            good += ok ? 1 : 0;
            fired += (s.valid && std::abs (s.linDb) > tol) ? 1 : 0;
            if (s.valid)
            {
                worstFold = std::abs (s.foldDb) > std::abs (worstFold) ? s.foldDb : worstFold;
                nearestLin = std::min (nearestLin, std::abs (s.linDb));
                worstStep = std::max (worstStep, s.stepRatio);
            }
            detail << " " << kSources[k].id << " " << juce::String (s.foldDb, 2) << " dB / " << juce::String (s.stepRatio, 2) << (s.valid ? "" : " INVALID");
            ctl << " " << kSources[k].id << " " << juce::String (s.linDb, 2);
        }
        row (seams == 7 && good == seams, "FSM1 1.2.0 the 7 seam sources wrap cleanly: fold centre within +-1.5 dB of the mean power of the two segments it mixed, wrap step <= the 99.9th-percentile sample step",
             "meas " + juce::String (good) + " / " + juce::String (seams) + " good (fold centre / wrap step ratio):" + detail + " | thr |dB| <= 1.5, ratio <= 1 | margin "
                 + juce::String (tol - std::abs (worstFold), 2) + " dB, step " + juce::String (1.0 - worstStep, 2) + " | ctrl FSM1n");
        row (seams == 7 && fired == seams, "FSM1n control: the same folds made with linear gains (fixture, not shipped) -> the level clause FAILS on all 7 (predicate must FAIL)",
             "meas fold centre:" + ctl + " dB | thr |dB| > 1.5 on 7 / 7 | margin " + juce::String (nearestLin - tol, 2) + " dB | " + ctrl (fired == seams, "FSM1 level clause"));
    }

    //--------------------------------------------------------------------------
    // FPT1 / FPT1n — a pick plays at the key's pitch
    struct Est { double hz = 0.0, strength = 0.0; bool edge = true; };

    /** The strongest sinusoid within +-span cents of f0 in x [a, b): a
        Hann-windowed transform on a 1-cent grid, parabolic refinement.
        strength = its amplitude over the RMS of the window. */
    Est partialNear (const std::vector<float>& x, juce::int64 a, juce::int64 b, double f0, double fs, int span)
    {
        Est r;
        const int n = (int) (b - a);
        if (n < 1024 || b > (juce::int64) x.size())
            return r;
        std::vector<double> w ((size_t) n), mag ((size_t) (2 * span + 1));
        double ws = 0.0, sq = 0.0;
        for (int i = 0; i < n; ++i)
        {
            w[(size_t) i] = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * ((double) i + 0.5) / (double) n);
            ws += w[(size_t) i];
            sq += (double) x[(size_t) (a + i)] * (double) x[(size_t) (a + i)];
        }
        for (int c = -span; c <= span; ++c)
        {
            const double om = juce::MathConstants<double>::twoPi * f0 * std::exp2 ((double) c / 1200.0) / fs;
            const double cs = std::cos (om), sn_ = std::sin (om);
            double re = 0.0, im = 0.0, pr = 1.0, pi = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const double v = w[(size_t) i] * (double) x[(size_t) (a + i)];
                re += v * pr;
                im += v * pi;
                const double t = pr * cs - pi * sn_;
                pi = pr * sn_ + pi * cs;
                pr = t;
            }
            mag[(size_t) (c + span)] = 2.0 * std::sqrt (re * re + im * im) / ws;
        }
        const int j = (int) (std::max_element (mag.begin(), mag.end()) - mag.begin());
        r.edge = j == 0 || j == 2 * span;
        double d = 0.0;
        if (! r.edge)
        {
            const double y0 = std::log (mag[(size_t) j - 1] + 1.0e-30), y1 = std::log (mag[(size_t) j] + 1.0e-30), y2 = std::log (mag[(size_t) j + 1] + 1.0e-30);
            const double den = y0 - 2.0 * y1 + y2;
            d = std::abs (den) > 1.0e-12 ? 0.5 * (y0 - y2) / den : 0.0;
        }
        r.hz = f0 * std::exp2 (((double) (j - span) + d) / 1200.0);
        const double rms = std::sqrt (sq / (double) n);
        r.strength = rms > 0.0 ? mag[(size_t) j] / rms : 0.0;
        return r;
    }

    /** The period by normalised autocorrelation, lags within +-span cents of
        fs / f0, parabolic refinement. strength = the correlation at the peak. */
    Est periodNear (const std::vector<float>& x, juce::int64 a, juce::int64 b, double f0, double fs, int span)
    {
        Est r;
        const int lagLo = (int) std::floor (fs / (f0 * std::exp2 ((double) span / 1200.0))), lagHi = (int) std::ceil (fs / (f0 * std::exp2 (-(double) span / 1200.0)));
        const int n = (int) (b - a) - lagHi;
        if (n < 1024 || b > (juce::int64) x.size())
            return r;
        std::vector<double> c ((size_t) (lagHi - lagLo + 1));
        for (int lag = lagLo; lag <= lagHi; ++lag)
        {
            double xy = 0.0, xx = 0.0, yy = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const double u = x[(size_t) (a + i)], v = x[(size_t) (a + i + lag)];
                xy += u * v; xx += u * u; yy += v * v;
            }
            c[(size_t) (lag - lagLo)] = xx > 0.0 && yy > 0.0 ? xy / std::sqrt (xx * yy) : 0.0;
        }
        const int j = (int) (std::max_element (c.begin(), c.end()) - c.begin());
        r.edge = j == 0 || j == lagHi - lagLo;
        double d = 0.0;
        if (! r.edge)
        {
            const double den = c[(size_t) j - 1] - 2.0 * c[(size_t) j] + c[(size_t) j + 1];
            d = std::abs (den) > 1.0e-12 ? 0.5 * (c[(size_t) j - 1] - c[(size_t) j + 1]) / den : 0.0;
        }
        r.hz = fs / ((double) (lagLo + j) + d);
        r.strength = c[(size_t) j];
        return r;
    }

    struct PitchCase { const char* id; bool acf; float sizeMs; float positionPct; };
    constexpr PitchCase kPitchCases[] = {
        { "crystal",          false, 2000.0f, 20.0f },
        { "guitar_overtones", false, 2000.0f, 20.0f },   // one harmonic note, C5 -32 ct (k 1)
        { "guitar_drift",     false, 2000.0f, 20.0f },
        { "contrabassoon",    true,   600.0f, 20.0f },
    };

    struct PitchRes { bool valid = false, ok = false; double cents = 0.0, strength = 0.0, rms = 0.0; int k = 0; float note = 0.0f, fine = 0.0f; };

    PitchRes runPitch (const PitchCase& pc, bool noWrite)
    {
        PitchRes r;
        const SourceSpec* spec = nullptr;
        for (const auto& s : kSources)
            if (juce::String (s.id) == pc.id)
                spec = &s;
        if (spec == nullptr || spec->refPartial < 1)
            return r;
        r.k = spec->refPartial;
        eg21::Session s (kFsP, eg21::with (eg21::base(), { { eg21::s1 ("size"), pc.sizeMs }, { eg21::s1 ("density"), 0.5f }, { eg21::s1 ("position"), pc.positionPct } }));
        s.p->testNoFactoryRootWrite = noWrite;
        if (! s.p->loadFactorySource (0, pc.id).isEmpty())
            return r;
        eg26b::settle (s);
        r.note = rawOf (*s.p, sn (1, "root_note"));
        r.fine = rawOf (*s.p, sn (1, "root_fine"));
        const juce::int64 t0 = s.pos;
        s.noteOn (t0, 48);
        const double sec = (double) pc.sizeMs / 1000.0;
        s.run ((juce::int64) ((sec + 0.05) * kFsP), 512);
        // The central half of the one grain.
        const juce::int64 a = t0 + (juce::int64) (0.25 * sec * kFsP), b = t0 + (juce::int64) (0.75 * sec * kFsP);
        const double f0 = 130.81278265029931 * (double) r.k;
        const auto e = pc.acf ? periodNear (s.L, a, b, f0, kFsP, 100) : partialNear (s.L, a, b, f0, kFsP, 100);
        double sq = 0.0;
        for (juce::int64 i = a; i < b && i < (juce::int64) s.L.size(); ++i)
            sq += (double) s.L[(size_t) i] * (double) s.L[(size_t) i];
        r.rms = std::sqrt (sq / (double) std::max<juce::int64> (1, b - a));
        r.valid = r.rms > 1.0e-4 && s.p->getSlotInfo (0).loaded;
        r.cents = e.hz > 0.0 ? 1200.0 * std::log2 (e.hz / f0) : 999.0;
        r.strength = e.strength;
        // A partial that IS the reference: inside the window (not its edge), within 10 ct, and a real
        // component of the sound (>= 0.15 of the RMS; the autocorrelation: >= 0.5).
        r.ok = r.valid && ! e.edge && std::abs (r.cents) <= 10.0 && e.strength >= (pc.acf ? 0.5 : 0.15);
        return r;
    }

    void rowsFPT1()
    {
        int good = 0, fired = 0, valid = 0;
        double worst = 0.0;
        juce::String detail, ctl;
        for (const auto& pc : kPitchCases)
        {
            const auto a = runPitch (pc, false);
            const auto n = runPitch (pc, true);
            good += a.ok ? 1 : 0;
            valid += (a.valid && n.valid) ? 1 : 0;
            fired += (n.valid && ! n.ok) ? 1 : 0;
            worst = std::max (worst, std::abs (a.cents));
            detail << " " << pc.id << " (root " << juce::String (a.note, 0) << " / " << juce::String (a.fine, 0) << ", k " << a.k << (pc.acf ? ", period" : "") << ") "
                   << juce::String (a.cents, 1) << " ct, strength " << juce::String (a.strength, 2) << (a.valid ? "" : " INVALID") << ";";
            ctl << " " << pc.id << " " << juce::String (n.cents, 1) << " ct / " << juce::String (n.strength, 2) << (n.valid ? "" : " INVALID") << ";";
        }
        row (good == 4 && valid == 4, "FPT1 1.2.0 the four off-C3 samples picked through loadFactorySource and played at C3: the reference partial within +-10 ct of k x 130.81 Hz",
             "meas " + juce::String (good) + " / 4:" + detail + " | thr |ct| <= 10, strength >= 0.15 (period: >= 0.5) | margin " + juce::String (10.0 - worst, 1) + " ct | ctrl FPT1n");
        row (fired == 4 && valid == 4, "FPT1n control: the root left at C3 (testNoFactoryRootWrite) -> FPT1 FAILS on all four (predicate must FAIL)",
             "meas cents / strength in the same window:" + ctl + " | thr 4 / 4 fail | " + ctrl (fired == 4, "FPT1 predicate"));
    }

    /** The fixed lane preload of the Live templates (D45: they keep the user's lanes). */
    void preloadLanes (eg21::Session& s)
    {
        std::vector<ogi::SavedSlot> v;
        const char* ids[4] = { "harmonic", "vowel", "pluck", "drone" };
        for (int i = 0; i < 4; ++i)
        {
            ogi::SavedSlot x;
            x.index = i;
            x.source = juce::String ("factory:") + ids[i];
            x.name = ids[i];
            v.push_back (x);
        }
        s.p->getLoader().restoreSlots (v, false);
        s.p->getLoader().waitForIdle (20000);
    }

    /** A rendering session at 120 bpm (the playhead outlives the session). */
    struct PSess
    {
        eg25b::Tp tp;
        std::unique_ptr<eg25b::TpHead> head;
        std::unique_ptr<eg21::Session> s;

        explicit PSess (double bpm = 120.0)
        {
            s = std::make_unique<eg21::Session> (kFsP, eg21::P {}, eg21::kSeed, 512);
            eg22::shipping (*s);
            tp.fs = kFsP;
            tp.bpm = tp.bpm2 = bpm;
            tp.pos = &s->pos;
            head = std::make_unique<eg25b::TpHead> (tp);
            s->p->setPlayHead (head.get());
        }
        ~PSess() { if (s != nullptr) s->p->setPlayHead (nullptr); }

        void apply (int i)
        {
            const auto& b = bank();
            if (b.specs[(size_t) i].live)
                preloadLanes (*s);
            s->p->getPresetManager().applyPresetData (F::presetJsonOf (b.defs[(size_t) i]));
            s->p->getLoader().waitForIdle (20000);
        }
    };

    /** The fixed note script (PD14 FP2): a dyad, two later notes, release, a
        second phrase; every note off by 16 s. */
    void script (eg21::Session& s)
    {
        const auto t = [] (double sec) { return (juce::int64) (sec * kFsP); };
        s.noteOn (t (0.0) + 64, 48, 100);
        s.noteOn (t (0.0) + 64, 55, 100);
        s.noteOn (t (1.5), 60, 90);
        s.noteOn (t (3.0), 64, 70);
        for (int n : { 48, 55, 60, 64 })
            s.noteOff (t (8.0), n);
        s.noteOn (t (10.0), 52, 110);
        s.noteOn (t (10.0), 59, 110);
        s.noteOn (t (12.0), 67, 80);
        for (int n : { 52, 59, 67 })
            s.noteOff (t (16.0), n);
    }

    double rmsDbOf (const std::vector<float>& L, const std::vector<float>& R, double t0, double t1)
    {
        const size_t a = (size_t) (t0 * kFsP), b = std::min ({ (size_t) (t1 * kFsP), L.size(), R.size() });
        double acc = 0.0;
        for (size_t k = a; k < b; ++k)
            acc += (double) L[k] * L[k] + (double) R[k] * R[k];
        const double ms = b > a ? acc / (2.0 * (double) (b - a)) : 0.0;
        return 10.0 * std::log10 (std::max (ms, 1e-30));
    }

    /** Gated loudness (the spread of FP2): RMS over the 50 ms frames of
        [t0, t1) whose own level is above -70 dBFS — a percussive preset is
        judged on what sounds, not on the silence after its decay. */
    double gatedDbOf (const std::vector<float>& L, const std::vector<float>& R, double t0, double t1, double& activeSec)
    {
        double acc = 0.0; size_t n = 0; int frames = 0;
        for (double t = t0; t + 0.05 <= t1; t += 0.05)
        {
            const double f = rmsDbOf (L, R, t, t + 0.05);
            if (f <= -70.0) continue;
            ++frames;
            const size_t a = (size_t) (t * kFsP), b = std::min ((size_t) ((t + 0.05) * kFsP), L.size());
            for (size_t k = a; k < b; ++k) acc += (double) L[k] * L[k] + (double) R[k] * R[k];
            n += 2 * (b - a);
        }
        activeSec = 0.05 * frames;
        return n > 0 ? 10.0 * std::log10 (std::max (acc / (double) n, 1e-30)) : -300.0;
    }

    bool sameBits (const std::vector<float>& a, const std::vector<float>& b)
    {
        return a.size() == b.size() && (a.empty() || std::memcmp (a.data(), b.data(), a.size() * sizeof (float)) == 0);
    }

    //--------------------------------------------------------------------------
    // FP0
    void rowFP0()
    {
        const auto& b = bank();
        auto p = make();
        Tally t;
        std::set<juce::String> names;
        std::set<std::int64_t> seeds;
        std::map<juce::String, int> perCat;
        for (const auto& s : b.specs)
        {
            const auto n = juce::String::fromUTF8 (s.name);
            t.check (names.insert (n).second, "dup name " + n);
            t.check (seeds.insert (s.seed).second, "dup seed " + n);
            t.check (n.length() <= 24 && ! n.containsAnyOf ("/\\:") && n.contains (juce::String::fromUTF8 (" \xc2\xb7 ")), "name " + n);
            const auto v = F::validateSpec (s, p->parameters);
            t.check (v.isEmpty(), v);
            ++perCat[catOf (s)];
        }
        // PLAN-R1 PD27: the AUTHORED table (wildcards unexpanded) validates too;
        // not counted in `checks` (a failure still fails the row).
        for (const auto& s : F::factoryPresetSpecs())
        {
            const auto v = F::validateSpec (s, p->parameters);
            if (v.isNotEmpty())
                t.check (false, "authored: " + v);
        }
        const juce::StringArray cats { "Drones", "Expressive", "Keys", "Live", "Motion", "Pads", "Rhythmic", "Textures" };
        for (const auto& c : cats)
            t.check (perCat[c] == 8, c + " count " + juce::String (perCat[c]));
        t.check (b.specs.size() == 64 && perCat.size() == 8, "64 / 8 categories");

        // Coverage (RESEARCH-4 A§Q1 table).
        int padsCustom = 0, padsPenta = 0, padsJust = 0, drFreeze = 0, drShFilter = 0, rhLfo = 0, mpe = 0, mpeBend = 0, monoGlide = 0, playCtl = 0;
        int txBp = 0, txGain = 0, txRev = 0, txCrush = 0, txDrive = 0;
        bool moFree = false, moNeg = false, moLoop = false, moRing = false, moCrush = false;
        std::set<int> moMacros;
        std::set<int> liveTrig, liveTarget;
        bool liveSync = false, liveGain = false, liveFb = false, liveFreeze = false, liveInit = false;
        for (const auto& s : b.specs)
        {
            const auto c = catOf (s), n = shortOf (s);
            const auto rs = routesOf (s);
            const auto routed = [&rs] (std::function<bool (const Route&)> f) { return std::any_of (rs.begin(), rs.end(), f); };
            if (c == "Keys")
            {
                bool noPitch = true;
                for (int k = 1; k <= 4; ++k)
                    noPitch = noPitch && val (s, ("s" + juce::String (k) + "_pitch_rand").toRawUTF8(), 0.0f) == 0.0f;
                noPitch = noPitch && ! routed ([] (const Route& r) { return r.src >= 1 && r.src <= 4 && (r.dest.endsWith ("_pitch") || r.dest.endsWith ("_pitch_rand")); });
                t.check (noPitch && val (s, "voice_mode", 0) == 0 && val (s, "s1_key_track", 100) == 100 && val (s, "amp_attack", 20) <= 5
                             && val (s, "amp_release", 1500) <= 2000 && has (s, "vel_to_amp") && val (s, "s1_window", 0) >= 0.5f
                             && val (s, "s1_spray", 10) <= 10,
                         "Keys " + n);
            }
            if (c == "Pads")
            {
                int on = 0; bool interval = false;
                for (int k = 1; k <= 4; ++k)
                    if (streamOn (s, k))
                    {
                        ++on;
                        const float pt = val (s, ("s" + juce::String (k) + "_pitch").toRawUTF8(), 0.0f);
                        interval = interval || pt == 7.0f || pt == 12.0f || pt == -12.0f;
                    }
                const bool rvOk = anyOnStream (s, [&s] (int k) { const float rv = sval (s, k, "reverb_mix", 0.0f); return rv >= 30 && rv <= 60; });
                t.check (on >= 2 && interval && val (s, "amp_attack", 20) >= 700 && val (s, "amp_release", 1500) >= 2500 && rvOk
                             && routed ([] (const Route& r) { return r.src >= 1 && r.src <= 4 && r.dest.endsWith ("_position"); }),
                         "Pads " + n);
                padsCustom += s.customJson.contains ("\"custom\"") && s.customJson.contains ("\"masks\"") ? 1 : 0;
                padsPenta += val (s, "s1_scale", 0) == 4 || val (s, "s2_scale", 0) == 4 ? 1 : 0;
                padsJust += val (s, "s1_scale", 0) == 24 || val (s, "s2_scale", 0) == 24 ? 1 : 0;
            }
            if (c == "Textures")
            {
                t.check (val (s, "s1_density", 10) >= 70 && val (s, "s1_spray", 10) == 100, "Textures " + n);
                txBp += val (s, "s1_filter_type", 0) == 2 && val (s, "s1_filter_rand", 0) > 0 ? 1 : 0;
                txGain += val (s, "s1_gain_rand", 0) > 0 ? 1 : 0;
                txRev += val (s, "s1_reverse_prob", 0) > 0 ? 1 : 0;
                txCrush += val (s, "s1_crush", 0) > 0 && val (s, "s1_crush_prob", 100) < 100 ? 1 : 0;
                txDrive += val (s, "s1_drive", 0) > 0 ? 1 : 0;
            }
            if (c == "Drones")
            {
                const bool fbOk = anyOnStream (s, [&s] (int k) { const float fb = sval (s, k, "feedback", 0); return fb >= 30 && fb <= 80 && sval (s, k, "fb_diffusion", 0) > 0; });
                t.check (val (s, "s1_key_track", 100) <= 50 && val (s, "s1_scan_mode", 1) == 2 && fbOk
                             && val (s, "amp_release", 1500) >= 5000,
                         "Drones " + n);
                drFreeze += val (s, "freeze", 0) == 1 ? 1 : 0;
                for (int l = 1; l <= 4; ++l)
                    if (val (s, ("lfo" + juce::String (l) + "_shape").toRawUTF8(), 0) == 4)
                        drShFilter += routed ([l] (const Route& r) { return r.src == l && r.dest.endsWith ("_filter_cutoff"); }) ? 1 : 0;
            }
            if (c == "Rhythmic")
            {
                bool grid = val (s, "sync_mode", 0) == 1;
                for (int k = 1; k <= 4; ++k)
                    if (streamOn (s, k))
                    {
                        const auto pre = "s" + juce::String (k) + "_";
                        grid = grid && val (s, (pre + "jitter").toRawUTF8(), 20) == 0 && val (s, (pre + "scan_mode").toRawUTF8(), 1) == 1
                                    && has (s, (pre + "density_sync").toRawUTF8());
                    }
                t.check (grid, "Rhythmic " + n);
                rhLfo += routed ([&s] (const Route& r) { return r.src >= 1 && r.src <= 4 && has (s, ("lfo" + juce::String (r.src) + "_rate_sync").toRawUTF8()); }) ? 1 : 0;
            }
            if (c == "Motion")
            {
                bool pv = false;
                for (const auto& r : rs)
                    pv = pv || (r.src >= 1 && r.src <= 4 && val (s, ("lfo" + juce::String (r.src) + "_mode").toRawUTF8(), 0) == 1);
                t.check (rs.size() >= 6 && pv, "Motion " + n + " routes " + juce::String ((int) rs.size()));
                for (int k = 1; k <= 4; ++k)
                {
                    const auto pre = "s" + juce::String (k) + "_";
                    if (! streamOn (s, k)) continue;
                    moFree = moFree || val (s, (pre + "scan_mode").toRawUTF8(), 1) == 0;
                    moNeg = moNeg || val (s, (pre + "scan_rate").toRawUTF8(), 0) < 0;
                    moLoop = moLoop || val (s, (pre + "loop_end").toRawUTF8(), 100) - val (s, (pre + "loop_start").toRawUTF8(), 0) <= 20;
                    moRing = moRing || val (s, (pre + "ring_mix").toRawUTF8(), 0) > 0;
                    moCrush = moCrush || val (s, (pre + "crush").toRawUTF8(), 0) > 0;
                }
                for (const auto& r : rs)
                    if (r.src >= 14)
                        moMacros.insert (r.src);
            }
            if (c == "Expressive")
            {
                if (val (s, "mpe_enable", 0) == 1)
                {
                    ++mpe;
                    t.check (routed ([] (const Route& r) { return r.src == 11 || r.src == 12; }), "MPE routes " + n);
                    mpeBend += routed ([] (const Route& r) { return r.src == 13; }) ? 1 : 0;
                }
                else if (val (s, "voice_mode", 0) >= 1 && val (s, "glide", 0) > 0)
                    ++monoGlide;
                else if (routed ([] (const Route& r) { return r.src >= 7 && r.src <= 10; }))
                    ++playCtl;
            }
            if (c == "Live")
            {
                t.check (s.live && s.slots.empty(), "Live has slots " + n);
                liveTrig.insert ((int) val (s, "rec_trigger", 0));
                liveTarget.insert ((int) val (s, "rec_target", 0));
                liveSync = liveSync || (val (s, "sync_mode", 0) == 1 && has (s, "rec_length_sync"));
                liveGain = liveGain || val (s, "rec_gain", 0) != 0;
                liveFb = liveFb || anyOnStream (s, [&s] (int k) { return sval (s, k, "feedback", 0) > 0; });
                liveFreeze = liveFreeze || val (s, "freeze", 0) == 1;
                liveInit = liveInit || (n == "Init" && s.values.empty());
            }
            else
                t.check (! s.live && ! s.slots.empty(), "not Live but no slots " + n);
        }
        t.check (padsCustom == 2 && padsPenta >= 1 && padsJust >= 1, "Pads custom / scales");
        t.check (txBp >= 1 && txGain >= 1 && txRev >= 1 && txCrush >= 1 && txDrive >= 1, "Textures features");
        t.check (drFreeze >= 1 && drShFilter >= 1, "Drones freeze / S&H filter");
        t.check (rhLfo >= 1, "Rhythmic synced LFO");
        t.check (moFree && moNeg && moLoop && moRing && moCrush && moMacros.size() == 4, "Motion features");
        t.check (mpe == 4 && mpeBend >= 1 && monoGlide == 2 && playCtl == 2, "Expressive split");
        t.check (liveTrig.size() == 3 && liveTarget.size() == 4 && liveSync && liveGain && liveFb && liveFreeze && liveInit, "Live features");
        const int checks = 64 * 4 + 9 + 64 + 7;
        row (t.bad == 0, "FP0 PD1 / PD2 bank table: 64 = 8 x 8, names, seeds, validateSpec, per-category coverage",
             "meas " + t.detail (checks) + " | thr 0 bad | ctrl FP1n / FP1n2");
    }

    //--------------------------------------------------------------------------
    // FP1 / FP1n / FP1n2 — apply from fresh
    struct Fp1 { int bad = 0; juce::String why; std::uint64_t touches = 0; };

    Fp1 fp1One (int i, const juce::var& json, const F::FactoryPresetSpec& s, std::function<bool (GranulatorInstrumentProcessor&)> load = {})
    {
        Fp1 r;
        PSess ps;
        auto& p = *ps.s->p;
        auto* og = p.parameters.getParameter ("output_gain");
        og->setValueNotifyingHost (og->convertTo0to1 (-7.0f));
        p.getLoader().hooks.resetCounters();
        const bool loaded = load ? load (p) : p.getPresetManager().applyPresetData (json);
        p.getLoader().waitForIdle (20000);
        if (! loaded) { r.bad = 1; r.why = shortOf (s) + ": load failed"; return r; }
        r.touches = p.getLoader().hooks.fsTouches.load();
        const auto fail = [&r, &s] (const juce::String& w) { if (r.bad++ == 0) r.why = shortOf (s) + ": " + w; };
        for (auto* q : p.getParameters())
        {
            auto* rp = dynamic_cast<juce::RangedAudioParameter*> (q);
            if (rp == nullptr) continue;
            const auto id = rp->getParameterID();
            if (id == "output_gain" || id == "rec_arm") continue;
            const float got = rp->convertFrom0to1 (rp->getValue());
            const bool listed = has (s, id.toRawUTF8());
            const float want = listed ? val (s, id.toRawUTF8(), 0.0f) : rp->convertFrom0to1 (rp->getDefaultValue());
            const auto& rg = rp->getNormalisableRange();
            const float tol = rg.interval > 0.0f ? 0.5f * rg.interval : 1.0e-4f * (rg.end - rg.start);
            const bool exact6 = ! listed && GiParamIds::isExactZeroDbRow (id);
            if (exact6 ? ! juce::exactlyEqual (got, 0.0f) : std::abs (got - want) > tol)
                fail (id + " " + juce::String (got, 6) + " != " + juce::String (want, 6));
        }
        const float og2 = og->convertFrom0to1 (og->getValue());
        if (std::abs (og2 + 7.0f) > 1.0e-4f) fail ("output_gain " + juce::String (og2));
        if ((std::int64_t) p.getSeed() != s.seed) fail ("seed");
        if (s.customJson.contains ("\"text\""))
        {
            const auto txt = juce::JSON::parse (s.customJson)["custom"]["text"].toString();
            if (p.getCustomScale().text != txt) fail ("custom scale " + p.getCustomScale().text);
        }
        for (const auto& sl : s.slots)
        {
            const auto info = p.getSlotInfo (sl.index);
            ogi::FactoryEntry e;
            ogi::findFactory (juce::String (sl.source).substring (8), e);
            if (! info.loaded || info.resolvedBy != 4 || info.contentHash != e.hash || info.source != juce::String (sl.source)
                || info.name != juce::String::fromUTF8 (sl.name))
                fail ("slot " + juce::String (sl.index) + " loaded " + juce::String ((int) info.loaded) + " by " + juce::String (info.resolvedBy));
        }
        if (r.touches != 0) fail ("fsTouches " + juce::String ((juce::int64) r.touches));
        juce::ignoreUnused (i);
        return r;
    }

    void rowsFP1()
    {
        const auto& b = bank();
        const int N = (int) b.specs.size();
        std::vector<Fp1> rs ((size_t) N);
        parallelFor (N, [&] (int i) { rs[(size_t) i] = fp1One (i, F::presetJsonOf (b.defs[(size_t) i]), b.specs[(size_t) i]); });
        int bad = 0; juce::String why;
        for (const auto& r : rs) { bad += r.bad > 0 ? 1 : 0; if (why.isEmpty() && r.bad > 0) why = r.why; }
        row (bad == 0 && N == 64, "FP1 PD14 apply from fresh: read-back == table, absent == default (PD8 six exact), output_gain kept, slots factory / pin, fsTouches 0",
             "meas " + juce::String (N - bad) + " / " + juce::String (N) + " presets clean" + (why.isEmpty() ? juce::String() : ", first: " + why)
                 + " | thr 64 / 64 | ctrl FP1n / FP1n2");

        // FP1n: Keys · Grain Piano with s1_size written as a linear fraction (skew 0.3 ignored).
        int k = 0;
        for (int i = 0; i < N; ++i) if (shortOf (b.specs[(size_t) i]) == "Grain Piano") k = i;
        auto bad1 = juce::JSON::parse (juce::JSON::toString (F::presetJsonOf (b.defs[(size_t) k])));
        const float lin = (val (b.specs[(size_t) k], "s1_size", 100.0f) - 5.0f) / (2000.0f - 5.0f);
        bad1["parameters"].getDynamicObject()->setProperty ("s1_size", lin);
        const auto n1 = fp1One (k, bad1, b.specs[(size_t) k]);
        row (n1.bad > 0 && n1.why.contains ("s1_size"), "FP1n control: s1_size authored as a linear fraction -> read-back FAILS (predicate must FAIL)",
             "meas " + n1.why + " | thr FP1 FAILS on s1_size | " + ctrl (n1.bad > 0 && n1.why.contains ("s1_size"), "FP1 predicate"));

        // FP1n2: the same preset with its slot pointing at a file: source.
        const auto wav = gTmp4.getChildFile ("fp1n2.wav");
        eg26b::writeWav (wav, eg21::noiseBuf (1, 48000, 0.2f, 77), 48000.0, 24);
        // A DEEP copy: a juce::var copy shares its DynamicObject, and editing the
        // bank's own customState would corrupt every later row (and the stamp).
        auto bad2 = juce::JSON::parse (juce::JSON::toString (F::presetJsonOf (b.defs[(size_t) k])));
        auto cs = bad2["customState"];
        cs["slots"][0].getDynamicObject()->setProperty ("source", "file:" + wav.getFullPathName());
        const auto n2 = fp1One (k, bad2, b.specs[(size_t) k]);
        row (n2.touches > 0 && n2.bad > 0, "FP1n2 control: a file: slot -> fsTouches > 0 (predicate must FAIL)",
             "meas fsTouches " + juce::String ((juce::int64) n2.touches) + " | thr > 0 | " + ctrl (n2.touches > 0 && n2.bad > 0, "FP1 no-disk predicate"));
    }

    //--------------------------------------------------------------------------
    // FP2 / FP3 — audible, bounded; Rhythmic on the grid
    void rowsFP2()
    {
        const auto& b = bank();
        const int N = (int) b.specs.size();
        struct R { bool finite = true; double peak = 0.0, rms = -300.0, gated = -300.0, active = 0.0; };
        std::vector<R> rs ((size_t) N);
        parallelFor (N, [&] (int i)
        {
            PSess ps;
            ps.apply (i);
            script (*ps.s);
            ps.s->run ((juce::int64) (20.0 * kFsP), 512);
            auto& r = rs[(size_t) i];
            for (size_t k = 0; k < ps.s->L.size(); ++k)
            {
                if (! std::isfinite (ps.s->L[k]) || ! std::isfinite (ps.s->R[k])) r.finite = false;
                r.peak = std::max ({ r.peak, (double) std::abs (ps.s->L[k]), (double) std::abs (ps.s->R[k]) });
            }
            r.rms = rmsDbOf (ps.s->L, ps.s->R, 2.0, 20.0);
            r.gated = gatedDbOf (ps.s->L, ps.s->R, 2.0, 20.0, r.active);
        });
        bool ok = true; double lo = 1e9, hi = -1e9, pk = 0.0, minRms = 1e9; juce::String tab, loN, hiN, minN;
        for (int i = 0; i < N; ++i)
        {
            const auto& r = rs[(size_t) i];
            ok = ok && r.finite && r.peak <= 4.0 && r.rms > -60.0;
            if (r.gated < lo) { lo = r.gated; loN = shortOf (b.specs[(size_t) i]); }
            if (r.gated > hi) { hi = r.gated; hiN = shortOf (b.specs[(size_t) i]); }
            if (r.rms < minRms) { minRms = r.rms; minN = shortOf (b.specs[(size_t) i]); }
            pk = std::max (pk, r.peak);
            tab << shortOf (b.specs[(size_t) i]).removeCharacters (" ") << juce::String::formatted (" %.1f/%.1f/%.1fs;", r.gated, r.rms, r.active);
        }
        row (ok && hi - lo <= 9.0, "FP2 PD14 20 s per preset (note script, 120 bpm): finite, peak <= 4.0, RMS 2-20 s > -60 dBFS, gated spread <= 9 dB",
             juce::String::formatted ("meas gated %.1f (", lo) + loN + juce::String::formatted (") .. %.1f (", hi) + hiN
                 + juce::String::formatted ("), spread %.2f dB; lowest plain RMS %.1f (", hi - lo, minRms) + minN
                 + juce::String::formatted ("); max peak %.3f | thr > -60, <= 9 dB, <= 4.0 | ctrl -", pk));
        std::printf ("  INFO FP2 gated / plain RMS dBFS / active s (2-20 s): %s\n", tab.toRawUTF8());
    }

    void rowFP3()
    {
        const auto& b = bank();
        int presets = 0, onsets = 0, bad = 0, empty = 0;
        double worst = 0.0;
        for (int i = 0; i < (int) b.specs.size(); ++i)
        {
            const auto& sp = b.specs[(size_t) i];
            if (catOf (sp) != "Rhythmic") continue;
            ++presets;
            for (double bpm : { 120.0, 93.0 })
            {
                PSess ps (bpm);
                ps.apply (i);
                ps.s->noteOn (1000, 48, 100);
                ps.s->run ((juce::int64) (4.0 * kFsP), 512);
                const int div = (int) val (sp, "s1_density_sync", 7.0f);
                const double d0 = (bpm / 60.0) / ogi::tempoDivBeats (div, 4.0);
                const double eR45 = std::clamp (0.0, std::ceil (std::log2 (0.5 / d0)), std::floor (std::log2 (200.0 / d0)));
                const double dq = ogi::tempoDivBeats (div, 4.0) * std::exp2 (-eR45);
                int here = 0;
                for (const auto& e : ps.s->hk().logInOrder())
                {
                    if (e.stream != 0 || (e.counterIdx & ogi::kBurstBit) != 0) continue;
                    double slot = 0.0;
                    const double err = eg25b::gridErr (ps.tp, dq, e.onsetSample, slot);
                    if (err < 0.0) continue;
                    ++onsets; ++here;
                    worst = std::max (worst, err);
                    bad += err <= 1.0 ? 0 : 1;
                }
                empty += here == 0 ? 1 : 0;
            }
        }
        row (presets == 8 && bad == 0 && empty == 0 && onsets > 0, "FP3 PD14 Rhythmic presets lock to the host grid at 120 and 93 bpm (stream 1 onsets <= 1 sample)",
             "meas " + juce::String (presets) + " presets x 2 tempi, " + juce::String (onsets) + " onsets, " + juce::String (bad) + " off-grid, worst "
                 + juce::String (worst, 3) + " smp, empty runs " + juce::String (empty) + " | thr 0 off-grid, 0 empty | ctrl - (TS1n carries the grid control)");
    }

    //--------------------------------------------------------------------------
    // FP4 / FP4b — load after dirty / after play
    std::unique_ptr<PSess> fp4Session (int a, int bIdx, bool dirty, int salt)
    {
        const auto& b = bank();
        auto ps = std::make_unique<PSess>();
        auto& p = *ps->s->p;
        if (dirty)
        {
            juce::Random rnd ((juce::int64) (0x5EED0000 + salt));
            for (auto* q : p.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (q))
                    if (rp->getParameterID() != "output_gain" && rp->getParameterID() != "rec_arm")
                        rp->setValueNotifyingHost (rnd.nextFloat());
            juce::String bad;
            p.setCustomScale ("dirty", "edo 23", bad);
            p.setDegreeMask (1, 4, 0x15ull);
            p.setDegreeMask (2, 31, 0x1F0ull);
            p.setDegreeMask (3, 24, 0x3ull);
            p.setSeed (0xD1A7D1A7ull + (std::uint64_t) salt);
            const auto& sb = b.specs[(size_t) bIdx];
            if (sb.live)
                preloadLanes (*ps->s);
            else
            {
                bool listed[4] = { false, false, false, false };
                for (const auto& sl : sb.slots) listed[sl.index] = true;
                std::vector<ogi::SavedSlot> v;
                for (int k = 0; k < 4; ++k)
                    if (! listed[k])
                    {
                        ogi::SavedSlot x;
                        x.index = k;
                        x.source = "file:" + gTmp4.getChildFile ("fp4-foreign-" + juce::String (k) + ".wav").getFullPathName();
                        x.name = "foreign";
                        v.push_back (x);
                    }
                p.getLoader().restoreSlots (v, false);
                p.getLoader().waitForIdle (20000);
            }
            juce::ignoreUnused (a);
        }
        else if (b.specs[(size_t) bIdx].live)
            preloadLanes (*ps->s);
        p.getPresetManager().applyPresetData (F::presetJsonOf (b.defs[(size_t) bIdx]));
        p.getLoader().waitForIdle (20000);
        return ps;
    }

    void rowsFP4()
    {
        const auto& b = bank();
        const int N = (int) b.specs.size();
        for (int k = 0; k < 4; ++k)
            eg26b::writeWav (gTmp4.getChildFile ("fp4-foreign-" + juce::String (k) + ".wav"), eg21::noiseBuf (2, 96000, 0.3f, 900 + k), 48000.0, 24);
        constexpr juce::int64 len = (juce::int64) (10.0 * kFsP);
        const auto render = [] (PSess& ps)
        {
            ps.s->noteOn (64, 48, 100);
            ps.s->noteOn (64, 55, 90);
            ps.s->noteOn ((juce::int64) (2.0 * kFsP), 62, 100);
            ps.s->noteOff ((juce::int64) (6.0 * kFsP), 48);
            ps.s->noteOff ((juce::int64) (6.0 * kFsP), 55);
            ps.s->noteOff ((juce::int64) (6.0 * kFsP), 62);
            ps.s->run (len, 512);
        };
        std::vector<std::vector<float>> fL ((size_t) N), fR ((size_t) N);
        parallelFor (N, [&] (int i)
        {
            auto ps = fp4Session (-1, i, false, 0);
            render (*ps);
            fL[(size_t) i] = ps->s->L;
            fR[(size_t) i] = ps->s->R;
        });
        std::vector<std::pair<int, int>> ab;
        for (int k : { 1, 8, 13, 32, 63 })
            for (int bi = 0; bi < N; ++bi)
                ab.push_back ({ (bi + k) % N, bi });
        std::atomic<int> bad { 0 };
        std::vector<juce::String> whyV (ab.size());
        parallelFor ((int) ab.size(), [&] (int j)
        {
            const auto [a, bi] = ab[(size_t) j];
            auto ps = fp4Session (a, bi, true, j);
            render (*ps);
            if (! sameBits (ps->s->L, fL[(size_t) bi]) || ! sameBits (ps->s->R, fR[(size_t) bi]))
            {
                ++bad;
                whyV[(size_t) j] = shortOf (b.specs[(size_t) a]) + " -> " + shortOf (b.specs[(size_t) bi]);
            }
        });
        juce::String why;
        for (const auto& w : whyV) if (why.isEmpty() && w.isNotEmpty()) why = w;
        int same = 0;
        for (int x = 0; x < N; ++x)
            for (int y = x + 1; y < N; ++y)
                same += sameBits (fL[(size_t) x], fL[(size_t) y]) && sameBits (fR[(size_t) x], fR[(size_t) y]) ? 1 : 0;
        const int nPairs = (int) ab.size();
        row (bad.load() == 0 && same == 0 && nPairs == 320,
             "FP4 PD14 load after dirty (291 random params, scale, masks, seed, foreign lanes) == fresh B, 10 s bitwise, 320 pairs",
             "meas " + juce::String (nPairs - bad.load()) + " / " + juce::String (nPairs) + " identical, " + juce::String (same)
                 + " equal fresh pairs" + (why.isEmpty() ? juce::String() : ", first diff " + why) + " | thr 320 / 320, 0 equal | ctrl non-vacuity (64 distinct)");
    }

    juce::var stateSansSlots (GranulatorInstrumentProcessor& p)
    {
        auto cs = p.presetCustomState();
        if (auto* o = cs.getDynamicObject())
            o->removeProperty ("slots");
        return cs;
    }

    void rowFP4b()
    {
        const auto& b = bank();
        const int N = (int) b.specs.size();
        std::atomic<int> bad { 0 };
        std::vector<juce::String> whyV ((size_t) N);
        parallelFor (N, [&] (int bi)
        {
            const int a = (bi + 1) % N;
            const auto& sb = b.specs[(size_t) bi];
            PSess ps;
            auto& p = *ps.s->p;
            ps.apply (a);
            for (int n = 1; n <= 4; ++n)   // PLAN-R1 PD16: all four sN_feedback
            {
                auto* fb = p.parameters.getParameter (GiParamIds::stream (n, "feedback"));
                fb->setValueNotifyingHost (fb->convertTo0to1 (80.0f));
            }
            ps.s->noteOn (64, 48, 100);
            ps.s->noteOn (64, 55, 100);
            ps.s->noteOff ((juce::int64) (5.0 * kFsP) - 4096, 48);
            ps.s->noteOff ((juce::int64) (5.0 * kFsP) - 4096, 55);
            ps.s->run ((juce::int64) (5.0 * kFsP), 512);
            std::uint64_t gen[4];
            for (int k = 0; k < 4; ++k) gen[k] = p.getSlotInfo (k).generation;
            ps.apply (bi);
            ps.s->run (2048, 256);   // the engine takes the new tapes within a few blocks (the settle () length)

            PSess fr;
            fr.apply (bi);
            juce::String w;
            for (auto* q : p.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (q))
                {
                    if (rp->getParameterID() == "output_gain" || rp->getParameterID() == "rec_arm") continue;
                    auto* other = fr.s->p->parameters.getParameter (rp->getParameterID());
                    if (! juce::exactlyEqual (rp->getValue(), other->getValue()) && w.isEmpty())
                        w = rp->getParameterID();
                }
            if (w.isEmpty() && juce::JSON::toString (stateSansSlots (p)) != juce::JSON::toString (stateSansSlots (*fr.s->p)))
                w = "seed / custom / masks";
            for (const auto& sl : sb.slots)
            {
                const auto i1 = p.getSlotInfo (sl.index), i2 = fr.s->p->getSlotInfo (sl.index);
                // The engine now plays the tape of the re-resolve (a new SlotTape
                // built from the pristine decode at that generation), not A's
                // feedback-dirtied one.
                const auto* tape = ps.s->eng().getCurrentTape (sl.index);
                const bool fresh = tape != nullptr && tape->generation == i1.generation;
                if (w.isEmpty() && (i1.source != i2.source || i1.name != i2.name || ! i1.loaded || i1.generation == gen[sl.index] || ! fresh))
                    w = "slot " + juce::String (sl.index) + (fresh ? "" : " old tape still playing") + " (tape gen "
                        + juce::String ((juce::int64) (tape != nullptr ? tape->generation : 0)) + ", info gen " + juce::String ((juce::int64) i1.generation)
                        + ", before " + juce::String ((juce::int64) gen[sl.index]) + ")";
            }
            if (w.isNotEmpty()) { ++bad; whyV[(size_t) bi] = shortOf (b.specs[(size_t) a]) + " -> " + shortOf (sb) + ": " + w; std::printf ("    FP4b %s\n", whyV[(size_t) bi].toRawUTF8()); }
        });
        juce::String why;
        for (const auto& w : whyV) if (why.isEmpty() && w.isNotEmpty()) why = w;
        row (bad.load() == 0, "FP4b PD14 load after play (A at feedback 80 %, notes off): params + seed / scale / masks + listed slots == fresh B, re-resolved (engine plays the new generation)",
             "meas " + juce::String (N - bad.load()) + " / " + juce::String (N) + " pairs (A = B + 1)" + (why.isEmpty() ? juce::String() : ", first " + why)
                 + " | thr 64 / 64 | ctrl - (not bitwise: reverb tail and global LFO phase carry over by design)");
    }

    //--------------------------------------------------------------------------
    // FP5 / PG1 — the real factory path (temp HOME only)
    bool underTempHome()
    {
        const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
        return tempHome() != juce::File() && home == tempHome() && home != realHome();
    }

    void rowFP5()
    {
        if (! underTempHome())
        {
            row (false, "FP5 real path: HOME is not the harness temp home -- refusing to touch a preset root", "meas home not redirected | thr temp | ctrl -");
            return;
        }
        const auto& b = bank();
        const int N = (int) b.specs.size();
        juce::File factory, user;
        {
            auto p0 = make();
            factory = p0->getPresetManager().getFactoryPresetsDirectory();
            user = p0->getPresetManager().getUserPresetsDirectory();
        }
        factory.deleteRecursively();
        auto p = make();   // the constructor writes the bank
        int named = 0;
        for (const auto& s : b.specs)
            named += factory.getChildFile (juce::String::fromUTF8 (s.name) + ".json").existsAsFile() ? 1 : 0;
        const int onDisk = factory.getNumberOfChildFiles (juce::File::findFiles, "*.json");
        const auto stamp = F::bankHash (b.defs);
        const bool sentinels = factory.getChildFile (".factory-version").loadFileAsString().trim() == JucePlugin_VersionString
                            && factory.getChildFile (".bank-hash").loadFileAsString().trim() == stamp;

        // loadPreset read-back (the FP1 predicate on the real load path).
        std::vector<Fp1> rs ((size_t) N);
        parallelFor (N, [&] (int i)
        {
            const auto name = juce::String::fromUTF8 (b.specs[(size_t) i].name);
            rs[(size_t) i] = fp1One (i, {}, b.specs[(size_t) i], [name] (GranulatorInstrumentProcessor& q) { return q.getPresetManager().loadPreset (name); });
        });
        int bad = 0; juce::String why;
        for (const auto& r : rs) { bad += r.bad > 0 ? 1 : 0; if (why.isEmpty() && r.bad > 0) why = r.why; }

        // Save as user -> reload on a fresh instance == factory apply.
        int k = 0;
        for (int i = 0; i < N; ++i) if (shortOf (b.specs[(size_t) i]) == "Tape Halo") k = i;
        bool sameState = false, sameAudio = false, saved = false, reloaded = false;
        juce::String inexactNote;
        {
            PSess a;
            a.apply (k);
            saved = a.s->p->getPresetManager().savePreset ("FP5 user copy");
        }
        {
            PSess u, c;
            reloaded = u.s->p->getPresetManager().loadPreset ("FP5 user copy");
            u.s->p->getLoader().waitForIdle (20000);
            c.apply (k);
            bool params = true;
            juce::StringArray inexact;
            for (auto* q : u.s->p->getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (q))
                    if (rp->getParameterID() != "output_gain" && rp->getParameterID() != "rec_arm")
                    {
                        // A saved value is getValue () = to01 (from01 (n)): the four
                        // LFO rates drift 1 ulp there (the AU1 set), nothing else may.
                        const float vu = rp->getValue(), vc = c.s->p->parameters.getParameter (rp->getParameterID())->getValue();
                        if (! juce::exactlyEqual (vu, vc))
                            inexact.add (rp->getParameterID());
                        params = params && std::abs (vu - vc) <= 1.0e-6f;
                    }
            const auto su = juce::JSON::toString (stateSansSlots (*u.s->p), true), sc = juce::JSON::toString (stateSansSlots (*c.s->p), true);
            const juce::StringArray au1 { "lfo1_rate", "lfo2_rate", "lfo3_rate", "lfo4_rate" };
            bool onlyAu1 = true;
            for (const auto& id : inexact) onlyAu1 = onlyAu1 && au1.contains (id);
            inexactNote = inexact.joinIntoString (", ");
            sameState = params && onlyAu1 && su == sc && u.s->p->getSlotInfo (0).source == c.s->p->getSlotInfo (0).source;
            for (auto* x : { &u, &c })
            {
                x->s->noteOn (64, 48, 100);
                x->s->noteOn (64, 55, 100);
                x->s->run ((juce::int64) (10.0 * kFsP), 512);
            }
            sameAudio = sameBits (u.s->L, c.s->L) && sameBits (u.s->R, c.s->R);
        }

        // Stamp arm.
        const auto bellFile = factory.getChildFile (juce::String::fromUTF8 ("Keys \xc2\xb7 Bell.json"));
        const auto original = bellFile.loadFileAsString();
        bellFile.replaceWithText ("{\"parameters\":{},\"edited\":true}");
        const auto orphan = factory.getChildFile (juce::String::fromUTF8 ("Old \xc2\xb7 Name.json"));
        orphan.replaceWithText ("{}");
        factory.getChildFile (".bank-hash").replaceWithText ("0000000000000000");
        const auto keep = user.getChildFile ("FP5 keep.json");
        keep.replaceWithText ("{\"parameters\":{}}");
        { auto again = make(); }
        const bool restored = bellFile.loadFileAsString() == original;
        const bool orphanGone = ! orphan.exists();
        const bool stampBack = factory.getChildFile (".bank-hash").loadFileAsString().trim() == stamp;
        const bool userKept = keep.existsAsFile() && user.getChildFile ("FP5 user copy.json").existsAsFile();
        keep.deleteFile();
        p->getPresetManager().deletePreset ("FP5 user copy");

        const auto yn = [] (bool x) { return juce::String (x ? "yes" : "NO"); };
        row (named == N && onDisk == N && sentinels && bad == 0 && saved && reloaded && sameState && sameAudio && restored && orphanGone && stampBack && userKept,
             "FP5 PD3 / PD14 real path (temp HOME): bank 64 + sentinel + stamp; loadPreset read-back; user copy == factory; stamp arm",
             "meas " + juce::String (named) + " / " + juce::String (N) + " named (" + juce::String (onDisk) + " .json), sentinels " + yn (sentinels)
                 + ", read-back bad " + juce::String (bad) + (why.isEmpty() ? juce::String() : " (" + why + ")") + ", user copy saved " + yn (saved)
                 + " / reloaded " + yn (reloaded) + " / state " + yn (sameState) + " (not bit-exact: " + (inexactNote.isEmpty() ? juce::String ("none") : inexactNote)
                 + ", 1 ulp = the AU1 set) / 10 s bitwise " + yn (sameAudio) + "; stamp arm: file restored "
                 + yn (restored) + ", orphan deleted " + yn (orphanGone) + ", stamp rewritten " + yn (stampBack) + ", User/ kept " + yn (userKept)
                 + " | thr all | ctrl - (stamp arm = the stale-bank stimulus)");
    }

    void rowsPG1()
    {
        if (! underTempHome())
        {
            row (false, "PG1 save guard: HOME is not the harness temp home -- refusing", "meas home not redirected | thr temp | ctrl -");
            return;
        }
        auto p = make();
        const auto factory = p->getPresetManager().getFactoryPresetsDirectory();
        const auto target = factory.getChildFile (juce::String::fromUTF8 ("Keys \xc2\xb7 Bell.json"));
        const auto before = target.loadFileAsString();
        const bool refused = ! p->savePresetToFileGuarded (target);
        const bool kept = target.loadFileAsString() == before && before.isNotEmpty();
        const bool nested = ! p->savePresetToFileGuarded (factory.getChildFile ("sub").getChildFile ("x.json"));
        const auto elsewhere = gTmp4.getChildFile ("pg1-elsewhere.json");
        const bool outside = p->savePresetToFileGuarded (elsewhere) && elsewhere.existsAsFile();
        row (refused && kept && nested && outside, "PG1 PD4 save guard: a target in / under Presets/Factory is refused, the file unchanged; elsewhere saves",
             juce::String ("meas factory refused ") + (refused ? "yes" : "NO") + ", file unchanged " + (kept ? "yes" : "NO") + ", nested refused "
                 + (nested ? "yes" : "NO") + ", outside saved " + (outside ? "yes" : "NO") + " | thr all | ctrl PG1n");
        p->testNoFactoryGuard = true;
        const bool wrote = p->savePresetToFileGuarded (target);
        const bool overwritten = target.loadFileAsString() != before;
        p->testNoFactoryGuard = false;
        factory.getChildFile (".bank-hash").deleteFile();   // the next constructor restores the bank
        { auto again = make(); }
        const bool fired = wrote && overwritten;
        row (fired, "PG1n control: guard off -> the factory file is overwritten (predicate must FAIL)",
             juce::String ("meas overwritten ") + (overwritten ? "yes" : "no") + ", restored after " + (target.loadFileAsString() == before ? "yes" : "NO")
                 + " | thr overwritten | " + ctrl (fired, "PG1 predicate"));
    }

    //--------------------------------------------------------------------------
    // FP6 / FP6n — switching presets while a chord sounds
    eg24b::Out fp6Render (int a, int bIdx, int mode, bool hard)
    {
        // mode 0: A, then B applied at 1.0 s; 1: A only; 2: B only.
        PSess ps;
        ps.s->hk().swapKillHard = hard;
        ps.apply (mode == 2 ? bIdx : a);
        ps.s->noteOn (64, 48, 100);
        ps.s->noteOn (64, 55, 100);
        if (mode == 0)
            ps.s->at ((juce::int64) kFsP, [bIdx] (eg21::Session& ss)
            {
                const auto& bb = bank();
                if (bb.specs[(size_t) bIdx].live) return;
                ss.p->getPresetManager().applyPresetData (F::presetJsonOf (bb.defs[(size_t) bIdx]));
                ss.p->getLoader().waitForIdle (20000);
            });
        ps.s->run ((juce::int64) (2.0 * kFsP), 256);
        return { ps.s->L, ps.s->R };
    }

    void rowsFP6()
    {
        const auto& b = bank();
        const auto idx = [&b] (const char* shortName)
        {
            for (int i = 0; i < (int) b.specs.size(); ++i)
                if (shortOf (b.specs[(size_t) i]) == juce::String::fromUTF8 (shortName)) return i;
            jassertfalse;
            return 0;
        };
        // Same source / other params; source change; Poly -> Mono; MPE on -> off; reverb 60 -> 0;
        // Tempo -> Free; feedback 80 -> 0; grain_limit 128 -> 256. Targets SUSTAIN under the held
        // chord: a percussive target's B-only reference has decayed by 0.9 s, so its window says
        // nothing about a click (switching onto Keys while holding = a SESSION-4 row).
        const std::pair<int, int> pairs[] = {
            { idx ("Organ Loft"), idx ("Tape Halo") }, { idx ("Bell"), idx ("Soft Rhodes") }, { idx ("Velvet Strings"), idx ("Mono Lead") },
            { idx ("MPE Breath"), idx ("Wheel Morph") }, { idx ("Slow Tide"), idx ("Glide Bass") }, { idx ("Gated Pad"), idx ("Warm Choir") },
            { idx ("Tape Halo"), idx ("Velvet Strings") }, { idx ("Warm Choir"), idx ("Orbit Lines") } };
        const auto d2 = [] (const eg24b::Out& o) { return std::max (eg24b::maxCurv (o.L, (size_t) (0.9 * kFsP), (size_t) (1.3 * kFsP)),
                                                                    eg24b::maxCurv (o.R, (size_t) (0.9 * kFsP), (size_t) (1.3 * kFsP))); };
        double worst = 0.0, worstH = 0.0;
        juce::String tab;
        for (const auto& [a, bi] : pairs)
        {
            const auto ra = fp6Render (a, bi, 1, false), rb = fp6Render (a, bi, 2, false);
            const auto sub = fp6Render (a, bi, 0, false), hard = fp6Render (a, bi, 0, true);
            const double ref = std::max ({ d2 (ra), d2 (rb), 1e-9 });
            const double r = d2 (sub) / ref, rh = d2 (hard) / ref;
            worst = std::max (worst, r);
            worstH = std::max (worstH, rh);
            tab << shortOf (b.specs[(size_t) a]).removeCharacters (" ") << ">" << shortOf (b.specs[(size_t) bi]).removeCharacters (" ")
                << juce::String::formatted (" %.2f/%.1f; ", r, rh);
        }
        row (worst <= 2.0, "FP6 PD14 switch presets while a chord sounds (8 pairs): max |d2| [0.9, 1.3] s <= 2.0 x the larger held reference",
             "meas worst " + juce::String (worst, 3) + "x | thr <= 2.0 | ctrl FP6n");
        row (worstH > 2.0, "FP6n control: swapKillHard (1-sample cut of the old grains) -> exceeds 2.0x (predicate must FAIL)",
             "meas worst " + juce::String (worstH, 2) + "x | thr > 2.0 | " + ctrl (worstH > 2.0, "FP6 predicate"));
        std::printf ("  INFO FP6 ratio subject / hard per pair: %s\n", tab.toRawUTF8());
    }

    //==========================================================================
    // UWF1 / UWF1n — wavFailed reaches the page's slot JSON (PD10)
    struct Wf1 { bool take1 = false, take2 = false, flag1 = false, flag2 = true, rev1 = false, rev2 = false, has1 = false; };

    Wf1 runWf1 (bool omit, int salt)
    {
        Wf1 r;
        GiUiJson::testOmitWavFailed = omit;
        eg21::Session s (48000.0, eg26b::stBase(), eg21::kSeed, 512);
        s.record = false;
        auto& ld = s.p->getLoader();
        const auto rev0 = s.p->getSlotStateRevision();
        ld.hooks.failWav.store (true);
        r.take1 = ug32::commitTake (s, 0, 4800, 0.3f, salt);
        ld.hooks.failWav.store (false);
        const auto rev1 = s.p->getSlotStateRevision();
        const auto j1 = GiUiJson::slotStateToVar (*s.p)["slots"][0];
        r.has1 = j1.hasProperty ("wavFailed");
        r.flag1 = r.has1 && static_cast<bool> (j1["wavFailed"]);
        r.rev1 = rev1 != rev0;
        r.take2 = ug32::commitTake (s, 0, 4800, 0.3f, salt + 1);
        const auto rev2 = s.p->getSlotStateRevision();
        const auto j2 = GiUiJson::slotStateToVar (*s.p)["slots"][0];
        r.flag2 = j2.hasProperty ("wavFailed") ? static_cast<bool> (j2["wavFailed"]) : true;
        r.rev2 = rev2 != rev1;
        GiUiJson::testOmitWavFailed = false;
        return r;
    }

    bool wf1Pred (const Wf1& r) { return r.take1 && r.take2 && r.flag1 && r.rev1 && ! r.flag2 && r.rev2; }

    void rowsWF1()
    {
        const auto yn = [] (bool b) { return juce::String (b ? "yes" : "no"); };
        const auto a = runWf1 (false, 511);
        row (wf1Pred (a), "UWF1 PD10 D55a a failed take WAV -> slot JSON wavFailed true (slotRev moved); next good take -> false",
             "meas takes " + yn (a.take1) + " / " + yn (a.take2) + ", failed take wavFailed " + yn (a.flag1) + " (rev moved " + yn (a.rev1)
                 + "), good take wavFailed " + yn (a.flag2) + " (rev moved " + yn (a.rev2) + ") | thr true then false, rev moves | ctrl UWF1n");
        const auto b = runWf1 (true, 513);
        const bool fired = ! wf1Pred (b) && b.take1 && ! b.has1;
        row (fired, "UWF1n control: wavFailed omitted from the slot JSON -> the page reads nothing (predicate must FAIL)",
             "meas field present " + yn (b.has1) + " | thr UWF1 FAILS | " + ctrl (fired, "UWF1 predicate"));
    }

    //==========================================================================
    // USV3 / USV3n — bounded close on a hung disk (PD12)

    struct Sv3Result
    {
        bool take = false, entered = false, partDuring = false, partAfter = true, target = true;
        double dtorMs = -1.0, exitMs = -1.0;
        int completions = 0, exits = 0;
        juce::String error;
    };

    Sv3Result runSv3 (bool inlineWrite, int salt)
    {
        using Proc = eg21::Proc;
        Sv3Result r;
        const auto file = gTmp4.getChildFile (inlineWrite ? "usv3n.wav" : "usv3.wav");
        const auto part = file.getSiblingFile (file.getFileName() + ".part");
        juce::WaitableEvent hold;
        Proc::writerExits.store (0);
        Proc::testWriterEntered.store (0);
        Proc::testInlineWrite.store (inlineWrite);
        Proc::testHoldWrite.store (&hold);

        struct Box { std::atomic<int> n { 0 }; juce::String error; };
        auto box = std::make_shared<Box>();
        auto s = std::make_unique<eg21::Session> (48000.0, eg26b::stBase(), eg21::kSeed, 512);
        s->record = false;
        r.take = ug32::commitTake (*s, 0, 96000, 0.4f, salt);
        s->p->saveRecordingTo (0, file, [box] (const Proc::SaveResult& res)
        {
            box->error = res.error;
            box->n.fetch_add (1);
        });
        // Audio runs until the snapshot is copied and the writer reaches the hold.
        for (int waited = 0; Proc::testWriterEntered.load() == 0 && waited < 10000; waited += 2)
        {
            s->run (512, 512);
            juce::Thread::sleep (2);
        }
        r.entered = Proc::testWriterEntered.load() == 1;
        r.partDuring = part.existsAsFile();

        // The control's release comes from a helper thread after 3 s (the
        // destructor blocks the calling thread on the in-worker write).
        std::unique_ptr<std::thread> helper;
        if (inlineWrite)
            helper = std::make_unique<std::thread> ([&hold] { juce::Thread::sleep (3000); hold.signal(); });

        const double t0 = juce::Time::getMillisecondCounterHiRes();
        s.reset();   // the processor destructor (save worker joined first)
        r.dtorMs = juce::Time::getMillisecondCounterHiRes() - t0;
        r.completions = box->n.load();
        r.error = box->error;

        if (helper != nullptr)
            helper->join();
        else
            hold.signal();
        const double t1 = juce::Time::getMillisecondCounterHiRes();
        while (! inlineWrite && Proc::writerExits.load() == 0 && juce::Time::getMillisecondCounterHiRes() - t1 < 2000.0)
            juce::Thread::sleep (1);
        r.exitMs = juce::Time::getMillisecondCounterHiRes() - t1;
        juce::Thread::sleep (50);   // a second (wrong) exit would land here
        r.exits = Proc::writerExits.load();
        r.partAfter = part.exists();
        r.target = file.exists();

        Proc::testHoldWrite.store (nullptr);
        Proc::testInlineWrite.store (false);
        return r;
    }

    bool sv3Pred (const Sv3Result& r)
    {
        return r.take && r.entered && r.partDuring && r.dtorMs <= 500.0 && r.completions == 1 && r.error == "cancelled"
            && r.exits == 1 && r.exitMs < 2000.0 && ! r.partAfter && ! r.target;
    }

    void rowsSV3()
    {
        const auto yn = [] (bool b) { return juce::String (b ? "yes" : "no"); };
        const auto a = runSv3 (false, 401);
        row (sv3Pred (a), "USV3 PD12 D55c bounded close on a hung WAV write (detached writer, abandon, .part removed)",
             "meas take " + yn (a.take) + ", writer held " + yn (a.entered) + " (.part " + yn (a.partDuring) + "), dtor "
                 + juce::String (a.dtorMs, 1) + " ms, completions " + juce::String (a.completions) + " '" + a.error + "', writer exits "
                 + juce::String (a.exits) + " (+" + juce::String (a.exitMs, 1) + " ms after release), .part after " + yn (a.partAfter)
                 + ", target " + yn (a.target) + " | thr dtor <= 500 ms, 1 completion 'cancelled', 1 exit < 2 s, no .part | ctrl USV3n");

        const auto b = runSv3 (true, 402);
        const bool fired = ! sv3Pred (b) && b.entered && b.dtorMs >= 3000.0;
        row (fired, "USV3n control: the 3.2 in-worker write -> the destructor waits for the 3 s hold (predicate must FAIL)",
             "meas writer held " + yn (b.entered) + ", dtor " + juce::String (b.dtorMs, 1) + " ms, completions "
                 + juce::String (b.completions) + " '" + b.error + "' | thr dtor >= 3000 ms, USV3 FAILS | " + ctrl (fired, "USV3 predicate"));
    }

    //==========================================================================
    /** PD16: `--dump-factory-presets <dir>` — the 64 preset files exactly as
        the manager writes them (parameters, customState, version, plugin,
        factory), from makeFactoryPresets (). Never calls
        initializeFactoryPresets. Returns the number written. */
    int dumpFactoryPresets (const juce::File& dir)
    {
        dir.createDirectory();
        const auto& b = bank();
        int n = 0;
        for (const auto& d : b.defs)
        {
            auto* o = new juce::DynamicObject();
            auto* ps = new juce::DynamicObject();
            for (const auto& [id, v] : d.parameters)
                ps->setProperty (id, v);
            o->setProperty ("parameters", juce::var (ps));
            o->setProperty ("customState", d.customState);
            o->setProperty ("version", JucePlugin_VersionString);
            o->setProperty ("plugin", "O-GranulatorInstrument");
            o->setProperty ("factory", true);
            const auto f = dir.getChildFile (d.name + ".json");
            n += f.replaceWithText (juce::JSON::toString (juce::var (o), true), false, false, "\n") ? 1 : 0;
        }
        return n;
    }

    //==========================================================================
    // PLAN-R1 PD29: `--dump-preset-renders <dir>` / `--cmp-preset-renders <dir>
    // [--all-bitwise] [--exclude-showcase]`. Outside the full set: they need a
    // reference directory (rework R1's B0, kept under build/). Per preset: the
    // FP2 session (shipping law, 120 bpm, Live templates on the stand-in
    // lanes) and note script, a fixed 10 s render; the raw file is L then R,
    // float32 little-endian. `preset-renders.sha256` lists them in bank order
    // (shasum format, LF). A missing directory or file = exit 77 (not run).
    constexpr double kPtSeconds = 10.0;

    /** PD28: the 10 re-voiced presets leave the comparison by name (T23 on). */
    constexpr const char* kShowcase[] = {
        "Drones \xc2\xb7 Deep Well", "Drones \xc2\xb7 Organ Loft", "Drones \xc2\xb7 Night Engine", "Expressive \xc2\xb7 Bend Choir",
        "Live \xc2\xb7 Four Lanes", "Motion \xc2\xb7 Phase Garden", "Pads \xc2\xb7 Fifth Bloom", "Pads \xc2\xb7 Slow Tide",
        "Rhythmic \xc2\xb7 Triplet Rain", "Textures \xc2\xb7 Broken Radio" };

    bool isShowcase (const F::FactoryPresetSpec& s)
    {
        for (const char* n : kShowcase)
            if (std::strcmp (s.name, n) == 0)
                return true;
        return false;
    }

    /** 1.3.0 (preset-bank-expansion P1 / P2): the 8 approved re-voices of the
        1.2.0 bank leave the comparison by name (`--exclude-revoiced13`). */
    constexpr const char* kRevoiced13[] = {
        "Keys \xc2\xb7 Kalimba", "Drones \xc2\xb7 Organ Loft", "Pads \xc2\xb7 Warm Choir", "Pads \xc2\xb7 Velvet Strings",
        "Textures \xc2\xb7 Ember Field", "Drones \xc2\xb7 Hollow Wind", "Textures \xc2\xb7 Rain Glass", "Keys \xc2\xb7 Glass Mallet" };

    bool isRevoiced13 (const F::FactoryPresetSpec& s)
    {
        for (const char* n : kRevoiced13)
            if (std::strcmp (s.name, n) == 0)
                return true;
        return false;
    }

    /** PLAN-R2 PD12: the 13 presets whose render moves with the level-hold
        makeup (a feedback Amount above 0 on a head that runs; Drones · Frozen
        Field has Freeze on and stays bitwise). Short names. */
    constexpr const char* kFeedbackMoved[] = {
        "Iron Sky", "Deep Well", "Organ Loft", "Hollow Wind", "Low Orbit", "Tape Halo", "Night Engine",
        "Feedback Loop", "Four Lanes", "Fifth Bloom", "Slow Tide", "Dotted Echo", "Broken Radio" };

    bool isFeedbackMoved (const F::FactoryPresetSpec& s)
    {
        const auto n = shortOf (s);
        for (const char* m : kFeedbackMoved)
            if (n == juce::String::fromUTF8 (m))
                return true;
        return false;
    }

    juce::String ptFileOf (const F::FactoryPresetSpec& s)
    {
        return catOf (s).replaceCharacter (' ', '_') + "__" + shortOf (s).replaceCharacter (' ', '_') + ".f32";
    }

    int ptStreams (const F::FactoryPresetSpec& s)
    {
        int n = 0;
        for (int k = 1; k <= 4; ++k)
            n += streamOn (s, k) ? 1 : 0;
        return n;
    }

    /** L then R, 10 s at 48 kHz. */
    std::vector<float> ptRender (int i, int dropTank = -1, const std::function<void (GranulatorInstrumentProcessor&)>& tweak = {})
    {
        PSess ps;
        ps.apply (i);
        if (tweak) tweak (*ps.s->p);          // PT2n2 control
        ps.s->hk().dropTankTerm = dropTank;   // PT2n control (-1 = off)
        script (*ps.s);
        ps.s->run ((juce::int64) (kPtSeconds * kFsP), 512);
        std::vector<float> v (ps.s->L);
        v.insert (v.end(), ps.s->R.begin(), ps.s->R.end());
        return v;
    }

    int dumpPresetRenders (const juce::File& dir)
    {
        dir.createDirectory();
        const auto& b = bank();
        const int N = (int) b.specs.size();
        std::vector<juce::String> sha ((size_t) N);
        std::vector<int> ok ((size_t) N, 0);
        parallelFor (N, [&] (int i)
        {
            const auto v = ptRender (i);
            const auto f = dir.getChildFile (ptFileOf (b.specs[(size_t) i]));
            ok[(size_t) i] = v.size() == (size_t) (2.0 * kPtSeconds * kFsP) && f.replaceWithData (v.data(), v.size() * sizeof (float)) ? 1 : 0;
            sha[(size_t) i] = juce::SHA256 (v.data(), v.size() * sizeof (float)).toHexString();
        });
        juce::String list;
        int n = 0, per[5] = { 0, 0, 0, 0, 0 };
        for (int i = 0; i < N; ++i)
        {
            const auto& s = b.specs[(size_t) i];
            list << sha[(size_t) i] << "  " << ptFileOf (s) << "\n";
            n += ok[(size_t) i];
            ++per[juce::jlimit (0, 4, ptStreams (s))];
            std::printf ("  %s  streams %d  %s\n", sha[(size_t) i].toRawUTF8(), ptStreams (s), s.name);
        }
        const bool wrote = dir.getChildFile ("preset-renders.sha256").replaceWithText (list, false, false, "\n");
        std::printf ("dump-preset-renders: %d files -> %s; enabled streams 1 / 2 / 3 / 4 = %d / %d / %d / %d (0: %d)\n",
                     n, dir.getFullPathName().toRawUTF8(), per[1], per[2], per[3], per[4], per[0]);
        return n == N && wrote ? 0 : 1;
    }

    int cmpPresetRenders (const juce::File& dir, bool allBitwise, bool excludeShowcase, bool r2 = false, bool excludeRevoiced13 = false)
    {
        if (! dir.isDirectory())
        {
            std::printf ("cmp-preset-renders: reference directory missing: %s (exit 77 = not run)\n", dir.getFullPathName().toRawUTF8());
            return 77;
        }
        const auto& b = bank();
        const int N = (int) b.specs.size();
        struct R { bool have = false, skip = false; juce::int64 differ = 0; double nullDb = -400.0, g = 1.0; int streams = 0; };
        std::vector<R> rs ((size_t) N);
        parallelFor (N, [&] (int i)
        {
            const auto& s = b.specs[(size_t) i];
            auto& r = rs[(size_t) i];
            r.streams = ptStreams (s);
            if (excludeShowcase && isShowcase (s)) { r.skip = true; return; }
            if (excludeRevoiced13 && isRevoiced13 (s)) { r.skip = true; return; }
            juce::MemoryBlock mb;
            const auto f = dir.getChildFile (ptFileOf (s));
            if (! f.existsAsFile() || ! f.loadFileAsData (mb)) return;
            const auto v = ptRender (i);
            if (mb.getSize() != v.size() * sizeof (float)) return;
            r.have = true;
            const auto* ref = static_cast<const float*> (mb.getData());
            double rr = 0.0, xr = 0.0, dd = 0.0;
            for (size_t k = 0; k < v.size(); ++k)
            {
                if (std::memcmp (&v[k], &ref[k], sizeof (float)) != 0) ++r.differ;
                const double x = v[k], y = ref[k];
                rr += y * y; xr += x * y; dd += (x - y) * (x - y);
            }
            r.g = rr > 0.0 ? xr / rr : 1.0;
            r.nullDb = dd > 0.0 && rr > 0.0 ? 10.0 * std::log10 (dd / rr) : -400.0;
        });
        int missing = 0, skipped = 0, single = 0, singleOk = 0, multi = 0, multiOk = 0, bit = 0, counted = 0;
        double worstNull = -400.0, worstG = 0.0;
        juce::String firstBad, excluded;
        for (int i = 0; i < N; ++i)
        {
            const auto& s = b.specs[(size_t) i];
            const auto& r = rs[(size_t) i];
            if (r.skip) { ++skipped; excluded << shortOf (s) << "; "; continue; }
            if (! r.have) { ++missing; continue; }
            ++counted;
            std::printf ("  PT %-34s streams %d  differ %9lld  null %8.2f dB  g-1 %+.3e\n", s.name, r.streams, (long long) r.differ, r.nullDb, r.g - 1.0);
            bit += r.differ == 0 ? 1 : 0;
            bool ok = true;
            if (r.streams <= 1)
            {
                ++single;
                ok = r.differ == 0;
                singleOk += ok ? 1 : 0;
            }
            else
            {
                ++multi;
                ok = r.nullDb <= -110.0 && std::abs (r.g - 1.0) <= 1e-5 && r.differ > 0;
                multiOk += ok ? 1 : 0;
                worstNull = std::max (worstNull, r.nullDb);
                worstG = std::max (worstG, std::abs (r.g - 1.0));
            }
            if ((allBitwise ? r.differ != 0 : ! ok) && firstBad.isEmpty())
                firstBad = juce::String::fromUTF8 (s.name) + juce::String::formatted (" (streams %d, differ %lld, null %.2f dB)", r.streams, (long long) r.differ, r.nullDb);
        }
        if (missing > 0)
        {
            std::printf ("cmp-preset-renders: %d reference file(s) missing or of the wrong size in %s (exit 77 = not run)\n", missing, dir.getFullPathName().toRawUTF8());
            return 77;
        }
        if (excludeShowcase)
            std::printf ("  INFO PT excluded (showcase, PD28): %d: %s\n", skipped, excluded.toRawUTF8());
        if (excludeRevoiced13)
            std::printf ("  INFO PT excluded (1.3.0 re-voices, P1): %d: %s\n", skipped, excluded.toRawUTF8());
        const juce::String why = firstBad.isEmpty() ? juce::String() : " first: " + firstBad;
        if (r2)
        {
            // PLAN-R2 PD12: PT0r (the other 51 bitwise), PT3 (each of the 13 differs),
            // PT3n (the same compare under makeupOff: the 13 are bitwise, PT3's predicate fails).
            int moved = 0, movedDiff = 0, rest = 0, restBit = 0;
            juce::String badRest, badMoved;
            std::vector<int> mi;
            for (int i = 0; i < N; ++i)
            {
                const auto& s = b.specs[(size_t) i];
                const auto& r = rs[(size_t) i];
                if (isFeedbackMoved (s))
                {
                    ++moved; mi.push_back (i);
                    if (r.differ > 0) ++movedDiff; else if (badMoved.isEmpty()) badMoved = shortOf (s);
                }
                else
                {
                    ++rest;
                    if (r.differ == 0) ++restBit; else if (badRest.isEmpty()) badRest = shortOf (s) + juce::String::formatted (" (differ %lld)", (long long) r.differ);
                }
            }
            // T10 deviation (recorded): after the D72 trims a makeup-off render of a
            // trimmed preset is no longer the baseline, so "the 13 are bitwise under
            // makeupOff" cannot be the control. PT3 judges the makeup itself: each of
            // the 13 differs from the baseline AND from its own makeupOff render.
            // PT3n runs the same on-vs-off compare on 13 presets outside the set:
            // all bitwise, so the predicate fails. The members whose makeupOff render
            // IS the baseline (the untrimmed ones) are counted as INFO.
            const auto offTweak = [] (GranulatorInstrumentProcessor& q) { q.getEngine().hooks.makeupOff = true; };
            const auto countDiff = [] (const std::vector<float>& a, const float* bptr, size_t n)
            {
                juce::int64 d = 0;
                for (size_t q = 0; q < n; ++q)
                    d += std::memcmp (&a[q], &bptr[q], sizeof (float)) != 0 ? 1 : 0;
                return d;
            };
            std::vector<int> ci;   // the control set: the first 13 presets outside the feedback set
            for (int i = 0; i < N && ci.size() < mi.size(); ++i)
                if (! isFeedbackMoved (b.specs[(size_t) i]))
                    ci.push_back (i);
            std::vector<juce::int64> onOff (mi.size(), -1), offRef (mi.size(), -1), ctlOnOff (ci.size(), -1);
            parallelFor ((int) (mi.size() + ci.size()), [&] (int k)
            {
                const bool isCtl = k >= (int) mi.size();
                const int i = isCtl ? ci[(size_t) k - mi.size()] : mi[(size_t) k];
                const auto on = ptRender (i), off = ptRender (i, -1, offTweak);
                if (on.size() != off.size()) return;
                if (isCtl) { ctlOnOff[(size_t) k - mi.size()] = countDiff (on, off.data(), on.size()); return; }
                onOff[(size_t) k] = countDiff (on, off.data(), on.size());
                juce::MemoryBlock mb;
                if (dir.getChildFile (ptFileOf (b.specs[(size_t) i])).loadFileAsData (mb) && mb.getSize() == off.size() * sizeof (float))
                    offRef[(size_t) k] = countDiff (off, static_cast<const float*> (mb.getData()), off.size());
            });
            int actOn = 0, offIsRef = 0, ctlBit = 0;
            juce::String badAct;
            for (size_t k = 0; k < mi.size(); ++k)
            {
                if (onOff[k] > 0) ++actOn; else if (badAct.isEmpty()) badAct = shortOf (b.specs[(size_t) mi[k]]);
                offIsRef += offRef[k] == 0 ? 1 : 0;
            }
            for (auto d : ctlOnOff) ctlBit += d == 0 ? 1 : 0;
            const int nMoved = (int) (sizeof (kFeedbackMoved) / sizeof (kFeedbackMoved[0]));
            row (moved == nMoved && rest == N - nMoved && restBit == rest, "PT0r every preset outside the feedback set renders bitwise as the R2 baseline (10 s, FP2 script)",
                 "meas " + juce::String (restBit) + " / " + juce::String (rest) + " bitwise" + (badRest.isEmpty() ? juce::String() : " first: " + badRest) + " | thr all | ctrl -");
            row (moved == nMoved && movedDiff == moved && actOn == moved, "PT3 each of the 13 feedback presets differs from the R2 baseline and from its own makeupOff render (the makeup acts on it)",
                 "meas " + juce::String (movedDiff) + " / " + juce::String (moved) + " differ from the baseline, " + juce::String (actOn) + " / " + juce::String (moved) + " from makeupOff"
                     + (badMoved.isEmpty() ? juce::String() : " first bitwise vs baseline: " + badMoved) + (badAct.isEmpty() ? juce::String() : " first bitwise vs makeupOff: " + badAct)
                     + " | thr all, differ > 0 | ctrl PT3n");
            std::printf ("  INFO PT3 under makeupOff %d of the %d render bitwise as the baseline (the presets with no level trim)\n", offIsRef, moved);
            const bool fired = ! ci.empty() && ctlBit == (int) ci.size();
            row (fired, "PT3n control: the same on-vs-makeupOff compare on 13 presets outside the feedback set -> bitwise (the PT3 predicate must FAIL)",
                 "meas " + juce::String (ctlBit) + " / " + juce::String ((int) ci.size()) + " bitwise | thr predicate 'differ > 0' FAILS on all | " + ctrl (fired, "PT3 predicate on presets the makeup does not reach"));
        }
        else if (allBitwise)
            row (bit == counted, "PT0 every preset render bitwise equal to the reference (10 s, FP2 script)",
                 "meas " + juce::String (bit) + " / " + juce::String (counted) + " bitwise" + why + " | thr all | ctrl -");
        else
        {
            row (singleOk == single, "PT1 single-stream presets bitwise equal to the reference",
                 "meas " + juce::String (singleOk) + " / " + juce::String (single) + (singleOk == single ? juce::String() : why) + " | thr all, differ == 0 | ctrl -");
            row (multiOk == multi, "PT2 multi-stream presets null <= -110 dB, |g - 1| <= 1e-5, differ > 0",
                 "meas " + juce::String (multiOk) + " / " + juce::String (multi)
                     + juce::String::formatted (", worst null %.2f dB, worst |g - 1| %.2e", worstNull, worstG) + (multiOk == multi ? juce::String() : why)
                     + " | thr <= -110 dB, <= 1e-5, > 0 | ctrl PT2n, PT2n2");
            // PT2n: stream 2's tank term dropped from the sum on the first multi-stream preset
            // that moved -> the PT2 predicate must FAIL (a build that lost a tank is not within -110 dB).
            int k = -1;
            for (int i = 0; i < N && k < 0; ++i)
                if (! rs[(size_t) i].skip && rs[(size_t) i].streams >= 2 && rs[(size_t) i].differ > 0 && streamOn (b.specs[(size_t) i], 2))
                    k = i;
            double nd = -400.0;
            if (k >= 0)
            {
                juce::MemoryBlock mb;
                dir.getChildFile (ptFileOf (b.specs[(size_t) k])).loadFileAsData (mb);
                const auto v = ptRender (k, 1);
                const auto* ref = static_cast<const float*> (mb.getData());
                double rr = 0.0, dd = 0.0;
                for (size_t q = 0; q < v.size(); ++q) { const double x = v[q], y = ref[q]; rr += y * y; dd += (x - y) * (x - y); }
                nd = dd > 0.0 && rr > 0.0 ? 10.0 * std::log10 (dd / rr) : -400.0;
            }
            const bool fired = k >= 0 && nd > -110.0;
            row (fired, "PT2n control: dropTankTerm on stream 2 -> the PT2 null FAILS (predicate must FAIL)",
                 "meas " + (k >= 0 ? juce::String::fromUTF8 (b.specs[(size_t) k].name) + juce::String::formatted (" null %.2f dB", nd) : juce::String ("no multi-stream preset moved"))
                     + " | thr > -110 dB | " + ctrl (fired, "PT2 predicate"));

            // PT2n2 (PLAN-R1 PD29, from T20): s2_reverb_size + 1 on a two-stream preset
            // whose stream 2 has reverb -> worse than -60 dB (the -110 dB tolerance is
            // not vacuous: one step of one stream's own knob is far outside it).
            int k2 = -1;
            for (int i = 0; i < N && k2 < 0; ++i)
                if (! rs[(size_t) i].skip && rs[(size_t) i].streams == 2 && streamOn (b.specs[(size_t) i], 2)
                    && sval (b.specs[(size_t) i], 2, "reverb_mix", 0.0f) > 0.0f)
                    k2 = i;
            double nd2 = -400.0;
            float size0 = 0.0f, size1 = 0.0f;
            if (k2 >= 0)
            {
                juce::MemoryBlock mb;
                dir.getChildFile (ptFileOf (b.specs[(size_t) k2])).loadFileAsData (mb);
                size0 = sval (b.specs[(size_t) k2], 2, "reverb_size", 50.0f);
                size1 = size0 <= 99.0f ? size0 + 1.0f : size0 - 1.0f;
                const auto v = ptRender (k2, -1, [size1] (GranulatorInstrumentProcessor& q)
                {
                    auto* rp = q.parameters.getParameter ("s2_reverb_size");
                    rp->setValueNotifyingHost (rp->convertTo0to1 (size1));
                });
                const auto* ref = static_cast<const float*> (mb.getData());
                double rr = 0.0, dd = 0.0;
                for (size_t q = 0; q < v.size(); ++q) { const double x = v[q], y = ref[q]; rr += y * y; dd += (x - y) * (x - y); }
                nd2 = dd > 0.0 && rr > 0.0 ? 10.0 * std::log10 (dd / rr) : -400.0;
            }
            const bool fired2 = k2 >= 0 && nd2 > -60.0;
            row (fired2, "PT2n2 control: s2_reverb_size + 1 on a two-stream preset -> worse than -60 dB (the PT2 predicate must FAIL)",
                 "meas " + (k2 >= 0 ? juce::String::fromUTF8 (b.specs[(size_t) k2].name) + juce::String::formatted (" s2_reverb_size %.0f -> %.0f, null %.2f dB", (double) size0, (double) size1, nd2)
                                    : juce::String ("no two-stream preset with reverb on stream 2"))
                     + " | thr > -60 dB | " + ctrl (fired2, "PT2 predicate"));
        }
        std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
        return juce::jmin (failures, 255);
    }

    //==========================================================================
    // PLAN-R2 PD13: `--preset-level-table <refDir>` (D72, sign-off 4). Per
    // preset, for the reference raw and the current render: RMS over 0-10 s and
    // 6-10 s, and the magnitude-weighted spectral centroid of the mono sum over
    // 2-10 s (one FFT of the whole span, zero-padded to 2^19). It prints; it is
    // not a row. Outlier: more than 3 dB louder over 0-10 s, or the centroid
    // moved by more than 0.5 octave. 77 = no reference.
    double ptRmsDb (const float* v, size_t n, double t0, double t1)
    {
        const size_t a = (size_t) (t0 * kFsP), b = std::min ((size_t) (t1 * kFsP), n);
        double acc = 0.0;
        for (size_t k = a; k < b; ++k)
            acc += (double) v[k] * v[k] + (double) v[n + k] * v[n + k];
        return 10.0 * std::log10 (std::max (b > a ? acc / (2.0 * (double) (b - a)) : 0.0, 1e-30));
    }

    /** power = false: magnitude-weighted (PD13); true: power-weighted (the
        weighting RESEARCH-R2's preset table used; T9 found it by reproducing
        that table). */
    double ptCentroidHz (const float* v, size_t n, double t0, double t1, bool power = false)
    {
        constexpr int kOrder = 19;
        const size_t a = (size_t) (t0 * kFsP), b = std::min ({ (size_t) (t1 * kFsP), n, a + ((size_t) 1 << kOrder) });
        juce::dsp::FFT fft (kOrder);
        std::vector<float> x ((size_t) 2 << kOrder, 0.0f);
        for (size_t k = a; k < b; ++k)
            x[k - a] = 0.5f * (v[k] + v[n + k]);
        fft.performFrequencyOnlyForwardTransform (x.data());
        double num = 0.0, den = 0.0;
        const size_t half = (size_t) 1 << (kOrder - 1);
        for (size_t k = 1; k < half; ++k)
        {
            const double m = power ? (double) x[k] * x[k] : (double) x[k];
            num += m * ((double) k * kFsP / (double) ((size_t) 1 << kOrder));
            den += m;
        }
        return den > 0.0 ? num / den : 0.0;
    }

    int presetLevelTable (const juce::File& dir)
    {
        if (! dir.isDirectory())
        {
            std::printf ("preset-level-table: reference directory missing: %s (exit 77 = not run)\n", dir.getFullPathName().toRawUTF8());
            return 77;
        }
        const auto& b = bank();
        const int N = (int) b.specs.size();
        struct R { bool have = false; double r0 = 0, r6 = 0, rc = 0, c0 = 0, c6 = 0, cc = 0, rp = 0, cp = 0; juce::int64 differ = 0; };
        std::vector<R> rs ((size_t) N);
        parallelFor (N, [&] (int i)
        {
            auto& r = rs[(size_t) i];
            juce::MemoryBlock mb;
            const auto f = dir.getChildFile (ptFileOf (b.specs[(size_t) i]));
            if (! f.existsAsFile() || ! f.loadFileAsData (mb)) return;
            const auto v = ptRender (i);
            if (mb.getSize() != v.size() * sizeof (float)) return;
            const auto* ref = static_cast<const float*> (mb.getData());
            const size_t n = v.size() / 2;
            for (size_t k = 0; k < v.size(); ++k)
                r.differ += std::memcmp (&v[k], &ref[k], sizeof (float)) != 0 ? 1 : 0;
            r.r0 = ptRmsDb (ref, n, 0.0, 10.0);      r.c0 = ptRmsDb (v.data(), n, 0.0, 10.0);
            r.r6 = ptRmsDb (ref, n, 6.0, 10.0);      r.c6 = ptRmsDb (v.data(), n, 6.0, 10.0);
            r.rc = ptCentroidHz (ref, n, 2.0, 10.0); r.cc = ptCentroidHz (v.data(), n, 2.0, 10.0);
            r.rp = ptCentroidHz (ref, n, 2.0, 10.0, true); r.cp = ptCentroidHz (v.data(), n, 2.0, 10.0, true);
            r.have = true;
        });
        int missing = 0, moved = 0, level = 0, cent = 0;
        std::printf ("preset-level-table vs %s\n", dir.getFullPathName().toRawUTF8());
        std::printf ("  %-34s %9s %9s %7s | %9s %9s %7s | %8s %8s %7s | %8s %8s %7s | %s\n", "preset", "ref 0-10", "cur 0-10", "d dB", "ref 6-10", "cur 6-10", "d dB",
                     "mag Hz", "cur", "oct", "pow Hz", "cur", "oct", "outlier");
        for (int i = 0; i < N; ++i)
        {
            const auto& r = rs[(size_t) i];
            if (! r.have) { ++missing; continue; }
            const double d0 = r.c0 - r.r0, d6 = r.c6 - r.r6;
            const double oct = r.rc > 0.0 && r.cc > 0.0 ? std::log2 (r.cc / r.rc) : 0.0;
            const double octP = r.rp > 0.0 && r.cp > 0.0 ? std::log2 (r.cp / r.rp) : 0.0;
            const bool lv = d0 > 3.0, ce = std::abs (oct) > 0.5 || std::abs (octP) > 0.5;
            moved += r.differ > 0 ? 1 : 0; level += lv ? 1 : 0; cent += ce ? 1 : 0;
            std::printf ("  LT %-31s %9.2f %9.2f %+7.2f | %9.2f %9.2f %+7.2f | %8.0f %8.0f %+7.2f | %8.0f %8.0f %+7.2f | %s%s%s\n", b.specs[(size_t) i].name, r.r0, r.c0, d0, r.r6, r.c6, d6, r.rc, r.cc, oct,
                         r.rp, r.cp, octP,
                         lv ? "LEVEL " : "", ce ? "CENTROID " : "", r.differ == 0 ? "(bitwise)" : "");
        }
        if (missing > 0)
        {
            std::printf ("preset-level-table: %d reference file(s) missing or of the wrong size (exit 77 = not run)\n", missing);
            return 77;
        }
        std::printf ("preset-level-table: %d presets, %d differ from the reference, %d level outliers (> +3.0 dB over 0-10 s), %d centroid outliers (> 0.5 octave, magnitude- or power-weighted)\n", N, moved, level, cent);
        return 0;
    }

    //==========================================================================
    // --cpu4 (PD16): the PERF-02 verdict on the real bank. Run ALONE, in the
    // background. First the --cpu26 rows (C25m <= 40, C25tm <= 12: the frozen
    // proxy regression + the session factor), then per preset: a fresh
    // session in the shipping law, loadPresetFromFile (the in-repo JSON from
    // makeFactoryPresets, written to a temp dir; initializeFactoryPresets is
    // never called), waitForIdle + 2048 samples, fsTouches 0 and the listed
    // slots loaded; then T (the C25tm chord {48,52,55,59,62,64}) and W (16
    // held notes 40-55) through eg24::measure25 UNCHANGED. Gate per preset:
    // T <= 12 %, W <= 40 % on the median. The sweep is measure25 at reps 5
    // (its warm-up and run lengths are fixed at 5 s / 10 s); a borderline
    // result (T > 10.5, W > 36) or NOVERDICT re-runs at reps 7, up to twice;
    // still invalid -> FAIL. Live templates get the factory stand-in lanes
    // (preloadLanes); every preset runs against an advancing 120-bpm playhead.

    /** A 120-bpm playhead that advances one block per processBlock (measure25
        drives processBlock directly, so Session::pos never moves). */
    struct AdvHead final : juce::AudioPlayHead
    {
        explicit AdvHead (int b) : block (b) {}
        int block;
        mutable juce::int64 at = 0;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo pi;
            const double q = (double) at * 120.0 / 60.0 / kFsP;
            pi.setBpm (120.0);
            pi.setPpqPosition (q);
            pi.setIsPlaying (true);
            pi.setPpqPositionOfLastBarStart (std::floor (q / 4.0) * 4.0);
            pi.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
            at += block;
            return pi;
        }
    };

    int cpu4()
    {
        const int f26 = eg26b::cpu26();
        std::printf ("  INFO --cpu4 session factor = this run's C25tm / 6.91 (the 3.3-verify value); C25m / C25tm rows above\n");

        constexpr int kBlock = 128;
        const double refNs = cal::readRef (gGoldenDir);
        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ogi-cpu4-" + juce::String (juce::Time::currentTimeMillis()));
        struct Guard { juce::File d; ~Guard() { d.deleteRecursively(); } } guard { dir };
        dumpFactoryPresets (dir);
        const auto& b = bank();
        const int N = (int) b.specs.size();
        const std::vector<int> chordT { 48, 52, 55, 59, 62, 64 };
        std::vector<int> heldW;
        for (int k = 0; k < 16; ++k) heldW.push_back (40 + k);

        struct Res { double t = 0, w = 0, liveT = 0, liveW = 0; bool ok = false, valid = false, loadOk = false; int runs = 0; juce::String note; };
        std::vector<Res> rs ((size_t) N);
        std::vector<std::unique_ptr<AdvHead>> heads;
        int fails = 0;
        for (int i = 0; i < N; ++i)
        {
            const auto& sp = b.specs[(size_t) i];
            const auto file = dir.getChildFile (juce::String::fromUTF8 (sp.name) + ".json");
            bool loadOk = true;
            auto mk = [&]
            {
                auto s = std::make_unique<eg21::Session> (kFsP, eg21::P {}, eg21::kSeed, kBlock);
                eg22::shipping (*s);
                heads.push_back (std::make_unique<AdvHead> (kBlock));
                s->p->setPlayHead (heads.back().get());
                if (sp.live)
                    preloadLanes (*s);
                s->p->getLoader().hooks.resetCounters();
                const bool ok = s->p->getPresetManager().loadPresetFromFile (file);
                s->p->getLoader().waitForIdle (20000);
                s->run (2048, kBlock);
                bool slots = true;
                for (const auto& sl : sp.slots)
                    slots = slots && s->p->getSlotInfo (sl.index).loaded;
                loadOk = loadOk && ok && slots && s->p->getLoader().hooks.fsTouches.load() == 0;
                return s;
            };
            auto& r = rs[(size_t) i];
            eg24::Cpu25 t, w;
            for (int reps : { 5, 7, 7 })
            {
                t = eg24::measure25 (mk, chordT, kFsP, kBlock, refNs, false, 0, reps);
                w = eg24::measure25 (mk, heldW, kFsP, kBlock, refNs, false, 0, reps);
                ++r.runs;
                const bool valid = ! t.slowed && ! t.contended && ! w.slowed && ! w.contended;
                const bool border = t.pct > 10.5 || w.pct > 36.0;
                if (valid && ! (border && reps == 5))
                {
                    r.valid = true;
                    break;
                }
            }
            r.t = t.pct; r.w = w.pct; r.liveT = t.live; r.liveW = w.live; r.loadOk = loadOk;
            r.ok = r.valid && loadOk && r.t <= 12.0 && r.w <= 40.0;
            fails += r.ok ? 0 : 1;
            row (r.ok, "CP4 " + shortOf (sp) + " (" + catOf (sp) + ")",
                 juce::String::formatted ("meas T %.2f %% (live %.0f) W %.2f %% (live %.0f)%s%s, runs %d", r.t, r.liveT, r.w, r.liveW,
                                          r.valid ? "" : " NOVERDICT", loadOk ? "" : " LOAD / fsTouches FAILED", r.runs)
                     + (sp.live ? " [stand-in lanes]" : "") + " | thr T <= 12, W <= 40 | ctrl -");
        }
        std::vector<double> ts;
        for (const auto& r : rs) ts.push_back (r.t);
        std::vector<int> order ((size_t) N);
        for (int i = 0; i < N; ++i) order[(size_t) i] = i;
        std::sort (order.begin(), order.end(), [&rs] (int x, int y) { return rs[(size_t) x].t > rs[(size_t) y].t; });
        auto sorted = ts;
        std::sort (sorted.begin(), sorted.end());
        double maxW = 0;
        for (const auto& r : rs) maxW = std::max (maxW, r.w);
        juce::String top;
        for (int k = 0; k < 5; ++k)
            top << shortOf (b.specs[(size_t) order[(size_t) k]]) << juce::String::formatted (" %.2f; ", rs[(size_t) order[(size_t) k]].t);
        std::printf ("  INFO CP4 bank: T median %.2f %%, p90 %.2f %%, max %.2f %%; W max %.2f %%; top-5 T: %s\n",
                     sorted[(size_t) N / 2], sorted[(size_t) (N * 9) / 10], sorted.back(), maxW, top.toRawUTF8());
        std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
        return f26 + fails;
    }

    //==========================================================================
    void runAll()
    {
        gTmp4 = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ogi-pg4-" + juce::String (juce::Time::currentTimeMillis()));
        gTmp4.createDirectory();
        struct Guard { ~Guard() { gTmp4.deleteRecursively(); } } guard;

        section ("4  polish: auval drift set, presets, factory sources, D55 (PLAN-4)");
        if (want ("AU1"))  rowsAU1();
        if (want ("FSP1")) rowsFSP1();
        if (want ("FL1"))  rowFL1();
        if (want ("FR1"))  rowsFR1();
        if (want ("FR1r")) rowFR1r();
        if (want ("FR1e")) rowFR1e();
        if (want ("FR2"))  rowFR2();
        if (want ("FLV1")) rowsFLV1();
        if (want ("FSM1")) rowsFSM1();
        if (want ("FPT1")) rowsFPT1();
        if (want ("FP0"))  rowFP0();
        if (want ("FP1"))  rowsFP1();
        if (want ("FP2"))  rowsFP2();
        if (want ("FP3"))  rowFP3();
        if (want ("FP4"))  rowsFP4();
        if (want ("FP4b")) rowFP4b();
        if (want ("FP5"))  rowFP5();
        if (want ("PG1"))  rowsPG1();
        if (want ("FP6"))  rowsFP6();
        if (want ("UWF1")) rowsWF1();
        if (want ("USV3")) rowsSV3();
    }
} }
