#pragma once
// O-GranulatorInstrument — render hashes (PLAN-2.2 T2 / P9 / PD10).
//
// Seven jobs, each rendered twice:
//   <job>-pinned   stimulus + pins22() (every 2.2 control at its neutral value)
//   <job>-default  stimulus only (the spec defaults spray 10 / jitter 20 / spread 30 apply)
//
// The stimulus is FROZEN at T2 (written against the unmodified 2.1 engine).
// prepareJob() is the ONLY extension point that later phases may edit
// (PLAN-2.2 T9: pinned -> coherenceOff + voiceGainDbOverride -6 dB). The local
// base21() is a frozen copy of eg21::base() at 06d7be7b so later edits to
// eg21::base() (T9 adds pins22) do not leak into the -default jobs.
//
// Hash = SHA-256 over the float32 L bytes then the R bytes (V3: the packed
// 2.1 GrainLogEntry identity fields serial onsetIndex onsetSample frac len
// startPos rate level). Lines "<job> <hex>", sorted.
//
// Included by main.cpp AFTER engine_gates_21.h (uses eg21::Session & helpers).

#include <juce_cryptography/juce_cryptography.h>   // linked into consoles (ParamDump.cmake), not in JuceHeader

namespace { namespace rh
{
    using eg21::Session;
    using eg21::P;
    using eg21::with;
    using eg21::s1;

    /** Frozen copy of eg21::base() at 06d7be7b (2.1 read set at a gate baseline). */
    inline P base21()
    {
        return { { "polyphony", 16.0f }, { "grain_limit", 3.0f }, { "pitch_bend_range", 2.0f }, { "output_gain", 0.0f },
                 { "amp_attack", 0.0f }, { "amp_decay", 0.0f }, { "amp_sustain", 100.0f }, { "amp_release", 50.0f },
                 { "vel_to_amp", 0.0f },
                 { s1 ("enable"), 1.0f }, { s1 ("mute"), 0.0f }, { s1 ("level"), 0.0f }, { s1 ("pan"), 0.0f },
                 { s1 ("position"), 0.0f }, { s1 ("scan_rate"), 0.0f }, { s1 ("scan_mode"), 1.0f },
                 { s1 ("loop_start"), 0.0f }, { s1 ("loop_end"), 100.0f }, { s1 ("root_note"), 48.0f },
                 { s1 ("root_fine"), 0.0f }, { s1 ("key_track"), 100.0f }, { s1 ("size"), 100.0f },
                 { s1 ("size_rand"), 0.0f }, { s1 ("density"), 10.0f }, { s1 ("pitch"), 0.0f } };
    }

    /** Every 2.2 control at its neutral value (s2-s4 disabled). */
    inline P pins22()
    {
        P p;
        for (const char* x : { "spray", "jitter", "stereo_spread", "gain_rand", "pitch_rand", "reverse_prob",
                               "window", "window_skew", "scale", "solo" })
            p.push_back ({ s1 (x), 0.0f });
        for (int n = 2; n <= 4; ++n)
            p.push_back ({ juce::String (Ids::stream (n, "enable")), 0.0f });
        return p;
    }

    /** PLAN-2.4 PD1 / T3: job overrides applied by prepareJob. `legacy` renders
        under the 2.1–2.3 wet safety (legacyWetTanh, the attribution proof);
        `extra` is applied after the pins; `events` are parameter sets at
        absolute sample positions (multiples of 32, event-split blocks: TB1). */
    struct Overrides
    {
        bool legacy = false;
        bool wetOff = false;   // TB2 identity half: wetSafetyOff (identity) instead of the knee
        P extra;
        std::vector<std::pair<juce::int64, P>> events;
        std::vector<std::pair<juce::int64, juce::MidiMessage>> midi;   // PLAN-2.5a TB3: CC / AT / bend added to every job
    };
    inline Overrides overrides;

    /** RAII: install overrides for a scope, restore the previous ones after. */
    struct ScopedOverrides
    {
        explicit ScopedOverrides (Overrides o) : saved (overrides) { overrides = std::move (o); }
        ~ScopedOverrides() { overrides = saved; }
        Overrides saved;
    };

    /** THE single extension point (PLAN-2.2 PD10; PLAN-2.4 T3 adds the overrides). */
    inline void prepareJob (Session& s, bool pinned)
    {
        if (pinned) { s.hk().coherenceOff = true; s.hk().voiceGainDbOverride = -6.0; }
        if (! overrides.extra.empty())
            eg21::setAll (*s.p, overrides.extra);
        for (const auto& ev : overrides.events)
        {
            const P ps = ev.second;
            s.at (ev.first, [ps] (Session& ss) { eg21::setAll (*ss.p, ps); });
        }
        for (const auto& m : overrides.midi)
            s.addMidi (m.first, m.second);
        s.hk().legacyWetTanh = overrides.legacy;
        s.hk().wetSafetyOff  = overrides.wetOff;
    }

    inline P stim (P over, bool pinned)
    {
        P b = with (base21(), over);
        return pinned ? with (b, pins22()) : b;
    }

    struct JobOut { std::vector<float> L, R; std::vector<char> logBytes; };

    template <typename T>
    void putBytes (std::vector<char>& v, const T& x)
    {
        const char* c = reinterpret_cast<const char*> (&x);
        v.insert (v.end(), c, c + sizeof (T));
    }

    juce::String sha (const JobOut& o)
    {
        juce::MemoryBlock mb;
        if (! o.logBytes.empty())
            mb.append (o.logBytes.data(), o.logBytes.size());
        else
        {
            mb.append (o.L.data(), o.L.size() * sizeof (float));
            mb.append (o.R.data(), o.R.size() * sizeof (float));
        }
        return juce::SHA256 (mb).toHexString();
    }

    JobOut take (Session& s) { return { s.L, s.R, {} }; }

    //==========================================================================
    // Jobs (stimuli copied from the eg21 rows named in PLAN-2.2 T2)

    JobOut jobN1 (bool pinned, std::uint64_t seed)
    {
        const P over { { "polyphony", 4.0f }, { "grain_limit", 1.0f }, { s1 ("density"), 100.0f }, { s1 ("size"), 300.0f },
                       { s1 ("size_rand"), 50.0f }, { s1 ("scan_rate"), 100.0f }, { s1 ("scan_mode"), 1.0f },
                       { "amp_attack", 5.0f }, { "amp_release", 200.0f } };
        Session s (48000.0, stim (over, pinned), seed, 512);
        prepareJob (s, pinned);
        s.load (0, eg21::noiseBuf (2, 3 * 48000, 0.3f, 7), 48000.0);
        eg21::n1Script (s);
        s.run (96000, 512);
        return take (s);
    }

    JobOut jobV3 (bool pinned)
    {
        constexpr double fs = 48000.0;
        const P over { { "polyphony", 4.0f }, { s1 ("density"), 50.0f }, { s1 ("size"), 100.0f },
                       { s1 ("size_rand"), 0.0f }, { "amp_attack", 20.0f }, { "amp_release", 300.0f },
                       { s1 ("position"), 20.0f } };
        Session s (fs, stim (over, pinned), eg21::kSeed);
        prepareJob (s, pinned);
        s.load (0, eg21::sineBuf (1, (int) (4.0 * fs), 1000.0, fs, 0.3), fs);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.noteOn (24000, 64);
        s.noteOn (36000, 67);
        s.run (48000, 512);
        JobOut o;
        for (const auto& e : s.hk().logInOrder())
        {
            putBytes (o.logBytes, e.serial);
            putBytes (o.logBytes, e.onsetIndex);
            putBytes (o.logBytes, e.onsetSample);
            putBytes (o.logBytes, e.frac);
            putBytes (o.logBytes, e.len);
            putBytes (o.logBytes, e.startPos);
            putBytes (o.logBytes, e.rate);
            putBytes (o.logBytes, e.level);
        }
        return o;
    }

    JobOut jobQ4 (bool pinned)
    {
        constexpr double fs = 48000.0;
        Session s (fs, stim ({ { "grain_limit", 1.0f }, { s1 ("density"), 100.0f }, { s1 ("size"), 200.0f },
                               { s1 ("position"), 50.0f } }, pinned));
        prepareJob (s, pinned);
        s.load (0, eg21::noiseBuf (2, (int) (3.0 * fs), 0.05f, 4), fs);
        for (int k = 0; k < 16; ++k)
            s.noteOn (97 * k, 48);
        s.run ((juce::int64) (1.6 * fs), 512);
        return take (s);
    }

    JobOut jobE1 (bool pinned)
    {
        constexpr double fs = 48000.0;
        Session s (fs, stim ({ { "amp_attack", 10.0f }, { "amp_decay", 50.0f }, { "amp_sustain", 50.0f }, { "amp_release", 100.0f },
                               { "vel_to_amp", 100.0f }, { s1 ("density"), 100.0f }, { s1 ("size"), 50.0f },
                               { s1 ("position"), 20.0f } }, pinned));
        prepareJob (s, pinned);
        s.load (0, eg21::sineBuf (1, (int) (2.0 * fs), 1000.0, fs, 0.1), fs);
        s.noteOn (0, 48, 32);                       // velocity 0.25
        s.noteOff ((juce::int64) (0.3 * fs), 48);
        s.run ((juce::int64) (0.5 * fs), 512);
        return take (s);
    }

    JobOut jobH1 (bool pinned)
    {
        constexpr double fs = 48000.0;
        Session s (fs, stim ({ { s1 ("density"), 200.0f }, { s1 ("size"), 50.0f }, { s1 ("position"), 20.0f } }, pinned));
        prepareJob (s, pinned);
        const auto srcA = eg21::sineBuf (1, (int) (3.0 * fs), 1000.0, fs, 0.2);
        const auto srcB = eg21::sineBuf (1, (int) (3.0 * fs), 1000.0, fs, 0.2, 0.5 * eg21::kPi);
        s.load (0, srcA, fs, "A");
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.at (24000, [&srcB] (Session& ss) { ss.load (0, srcB, 48000.0, "B"); });
        s.at (48000, [&srcA] (Session& ss) { ss.load (0, srcA, 48000.0, "A2"); });
        s.run (72000, 32);
        return take (s);
    }

    JobOut jobP2 (bool pinned)
    {
        constexpr double fs = 48000.0;
        Session s (fs, stim ({ { s1 ("size"), 2000.0f }, { s1 ("density"), 0.5f }, { s1 ("position"), 25.0f } }, pinned));
        prepareJob (s, pinned);
        s.load (0, eg21::sineBuf (1, (int) (6.0 * fs), 440.0, fs, 0.5), fs);
        s.noteOn (0, 60);                           // root + 12
        s.run ((juce::int64) (2.05 * fs), 512);
        return take (s);
    }

    JobOut jobCH (bool pinned)
    {
        constexpr double fs = 48000.0;
        Session s (fs, stim ({}, pinned));
        prepareJob (s, pinned);
        s.load (0, eg21::sineBuf (1, (int) (3.0 * fs), 440.0, fs, 0.3), fs);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.run ((juce::int64) (1.0 * fs), 512);
        return take (s);
    }

    /** "<job> <hex>" lines, sorted. */
    juce::StringArray computeAll()
    {
        juce::StringArray lines;
        for (bool pinned : { true, false })
        {
            const juce::String suf = pinned ? "-pinned" : "-default";
            lines.add ("N1" + suf + " " + sha (jobN1 (pinned, eg21::kSeed)));
            lines.add ("V3" + suf + " " + sha (jobV3 (pinned)));
            lines.add ("Q4" + suf + " " + sha (jobQ4 (pinned)));
            lines.add ("E1" + suf + " " + sha (jobE1 (pinned)));
            lines.add ("H1" + suf + " " + sha (jobH1 (pinned)));
            lines.add ("P2" + suf + " " + sha (jobP2 (pinned)));
            lines.add ("CH" + suf + " " + sha (jobCH (pinned)));
        }
        lines.sort (false);
        return lines;
    }

    /** computeAll() under legacyWetTanh (PLAN-2.4 PD1: renders-2.1 / 2.2 attribution). */
    juce::StringArray computeAllLegacy()
    {
        Overrides o = overrides;
        o.legacy = true;
        ScopedOverrides so (o);
        return computeAll();
    }

    /** --dump-render-hashes <file> (shipping) / --dump-render-hashes-legacy <file>
        (legacyWetTanh): writes the table; returns failures (control + pinned==default INFO). */
    int dump (const juce::File& out, bool legacy = false)
    {
        Overrides o = overrides;
        o.legacy = legacy;
        ScopedOverrides so (o);
        const auto lines = computeAll();
        out.replaceWithText (lines.joinIntoString ("\n") + "\n", false, false, "\n");

        // Control: N1 with seed + 1 must hash differently.
        const bool ctrlFired = sha (jobN1 (true, eg21::kSeed + 1)) != sha (jobN1 (true, eg21::kSeed));
        int pinnedEqDefault = 0;
        for (const auto& l : lines)
            if (l.containsWholeWord ("-pinned") || l.contains ("-pinned "))
            {
                const auto job = l.upToFirstOccurrenceOf ("-", false, false);
                const auto h = l.fromLastOccurrenceOf (" ", false, false);
                for (const auto& m : lines)
                    if (m.startsWith (job + "-default ") && m.fromLastOccurrenceOf (" ", false, false) == h)
                        ++pinnedEqDefault;
            }
        std::printf ("render hashes: %d lines -> %s\n", lines.size(), out.getFullPathName().toRawUTF8());
        std::printf ("INFO pinned == default: %d / 7\n", pinnedEqDefault);
        std::printf ("%s\n", eg21::ctrl (ctrlFired, "N1 seed + 1 -> different hash").toRawUTF8());
        return ctrlFired ? 0 : 1;
    }
}} // namespace ::rh
