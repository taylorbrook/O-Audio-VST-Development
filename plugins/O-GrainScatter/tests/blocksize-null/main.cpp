/*
  ==============================================================================

    O-GrainScatter — block-size null harness (v3.1.0)

    Gate: the same seeded state and stimulus rendered at 32-sample blocks and at
    a variable split (1-512, seeded) nulls against the 512-sample render to
    <= -120 dBFS peak, in every engine path:

      free     Free mode, every random control up, reverse, feedback
      sync     1/16 sync, Repeats 4, Probability 70, swing, Euclid 5/8,
               Stutter Gate on (dry at 0 dB, so the gate is in the output)
      spatial  Trajectory mode, Random trajectory, Doppler (control-rate SH
               updates on the absolute clock)
      freeze   Freeze engaged at a fixed sample (a block boundary in every split)
      pitch    +24 st transpose, 500 ms grains (deep pre-roll on the 12 s ring)

    Non-vacuity rows:
      - each reference render is non-silent;
      - a different seed does NOT null (the gate can see grain-level changes);
      - the seed survives getStateInformation -> setStateInformation;
      - scheduler unit rows: the v3.1.0 scheduler emits every repeat (count =
        triggers x repeats) at identical absolute times for 32/512/variable
        blocks, while the v3.0.5 scheduler (compiled from the backup) drops
        repeats at 512 — the defect this release fixes;
      - DelayBuffer read precision at 192 kHz with the write head past 2^21:
        a 1 kHz sine read at a fractional delay matches the analytic value.

    Exit 0 = every row PASS.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>

#ifdef GS_BASELINE_DSP_DIR
namespace old_v305
{
    #include GS_BASELINE_SCHEDULER_H
}
#endif

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    constexpr double kSampleRate  = 48000.0;
    constexpr int    kMaxBlock    = 512;
    constexpr int    kLength      = 6 * 48000;
    constexpr int    kChangePoint = 98304;   // 192 x 512 = 3072 x 32: a boundary in every split
    constexpr double kNullDb      = -120.0;

    int failures = 0;

    void row (bool ok, const char* name, const juce::String& detail)
    {
        std::printf ("  [%s] %-52s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    }

    double toDb (double x) { return x > 0.0 ? 20.0 * std::log10 (x) : -400.0; }

    using Params = std::map<juce::String, float>;   // id -> real value

    struct Scenario
    {
        const char* name;
        Params params;
        Params atChange;   // applied before the block starting at kChangePoint
    };

    // Tonal + noisy stereo stimulus, deterministic.
    juce::AudioBuffer<float> makeStimulus()
    {
        juce::AudioBuffer<float> b (2, kLength);
        juce::Random r (0x5EEDull);
        for (int i = 0; i < kLength; ++i)
        {
            const double t = i / kSampleRate;
            const double env = 0.5 + 0.5 * std::sin (2.0 * juce::MathConstants<double>::pi * 0.7 * t);
            const float n = (r.nextFloat() * 2.0f - 1.0f) * 0.05f;
            b.setSample (0, i, static_cast<float> (0.3 * env * std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * t)
                                                   + 0.1 * std::sin (2.0 * juce::MathConstants<double>::pi * 1337.0 * t)) + n);
            b.setSample (1, i, static_cast<float> (0.3 * env * std::sin (2.0 * juce::MathConstants<double>::pi * 330.0 * t)
                                                   + 0.1 * std::sin (2.0 * juce::MathConstants<double>::pi * 911.0 * t)) - n);
        }
        return b;
    }

    // Block splits. Every split has a boundary at kChangePoint.
    std::vector<int> fixedSplit (int n)
    {
        std::vector<int> s;
        for (int pos = 0; pos < kLength; pos += n)
            s.push_back (juce::jmin (n, kLength - pos));
        return s;
    }

    std::vector<int> variableSplit()
    {
        std::vector<int> s;
        juce::Random r (0xB10CCull);
        int pos = 0;
        while (pos < kLength)
        {
            int n = 1 + r.nextInt (kMaxBlock);
            if (pos < kChangePoint && pos + n > kChangePoint) n = kChangePoint - pos;
            n = juce::jmin (n, kLength - pos);
            s.push_back (n);
            pos += n;
        }
        return s;
    }

    void setParams (juce::AudioProcessor& p, const Params& params)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
            {
                auto it = params.find (rp->getParameterID());
                if (it != params.end())
                    rp->setValueNotifyingHost (rp->convertTo0to1 (it->second));
            }
    }

    std::unique_ptr<GrainScatterProcessor> makeInstance (const Params& params, uint64_t seed,
                                                         const juce::MemoryBlock* state = nullptr)
    {
        std::unique_ptr<GrainScatterProcessor> p (dynamic_cast<GrainScatterProcessor*> (createPluginFilter()));
        p->setPlayConfigDetails (2, 2, kSampleRate, kMaxBlock);
        if (state != nullptr)
            p->setStateInformation (state->getData(), static_cast<int> (state->getSize()));
        else
        {
            setParams (*p, params);
            p->setRngSeed (seed);
        }
        p->prepareToPlay (kSampleRate, kMaxBlock);
        p->reset();
        return p;
    }

    juce::AudioBuffer<float> render (GrainScatterProcessor& p, const Scenario& sc,
                                     const juce::AudioBuffer<float>& in, const std::vector<int>& split)
    {
        juce::AudioBuffer<float> out (2, kLength);
        juce::AudioBuffer<float> block (2, kMaxBlock);
        juce::MidiBuffer midi;
        int pos = 0;
        for (int n : split)
        {
            if (pos == kChangePoint && ! sc.atChange.empty())
                setParams (p, sc.atChange);
            block.setSize (2, n, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
                block.copyFrom (ch, 0, in, ch, pos, n);
            p.processBlock (block, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, block, ch, 0, n);
            pos += n;
        }
        return out;
    }

    double peakDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        double m = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kLength; ++i)
                m = juce::jmax (m, std::abs (static_cast<double> (a.getSample (ch, i)) - b.getSample (ch, i)));
        return m;
    }

    double rms (const juce::AudioBuffer<float>& a)
    {
        double s = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kLength; ++i)
                s += static_cast<double> (a.getSample (ch, i)) * a.getSample (ch, i);
        return std::sqrt (s / (2.0 * kLength));
    }

    // ── Scheduler unit rows ─────────────────────────────────────────────────
    template <typename Sched, typename Tracker, typename Run>
    std::vector<juce::int64> schedulerTimes (const std::vector<int>& split, Run run)
    {
        Sched s;
        Tracker t;
        s.prepare (kSampleRate);
        t.prepare (kSampleRate);
        std::vector<juce::int64> times;
        juce::int64 pos = 0;
        for (int n : split)
        {
            auto info = t.update (nullptr, n);
            run (s, info, n, pos, times);
            pos += n;
        }
        return times;
    }

    std::array<bool, 16> allOn()
    {
        std::array<bool, 16> a {};
        a.fill (true);
        return a;
    }

    void schedulerRows()
    {
        // 1/16 at 120 BPM, probability 100, Euclid off, Repeats 4: one trigger every
        // 6000 samples, each with 3 repeats.
        constexpr int kRepeats = 4;
        const auto pattern = allOn();

        auto runNew = [&] (GrainScheduler& s, const SyncInfo& info, int n, juce::int64 pos,
                           std::vector<juce::int64>& times)
        {
            std::vector<SpawnRequest> req;
            req.reserve (GrainScheduler::kMaxSpawnsPerBlock);
            s.processBlockSync (n, info, 3, 100.0f, kRepeats, pattern, 0, 0, 50.0f, false, req);
            for (auto& r : req) times.push_back (pos + r.sampleOffset);
        };

        std::vector<std::vector<juce::int64>> runs;
        for (auto& split : { fixedSplit (512), fixedSplit (32), variableSplit() })
        {
            auto t = schedulerTimes<GrainScheduler, TempoTracker> (split, runNew);
            std::sort (t.begin(), t.end());
            runs.push_back (t);
        }

        // Triggers every 6000 samples from sample 0 (ppq 0 counts as crossed); the
        // primary and each repeat count when they land inside the render.
        const juce::int64 interval = 6000;
        juce::int64 expected = 0;
        for (juce::int64 trig = 0; trig < kLength; trig += interval)
            for (int r = 0; r < kRepeats; ++r)
                if (trig + r * interval < kLength) ++expected;

        row (static_cast<juce::int64> (runs[0].size()) == expected, "scheduler: every repeat emitted (512)",
             juce::String (runs[0].size()) + " of " + juce::String (expected));
        row (runs[0] == runs[1] && runs[0] == runs[2], "scheduler: identical spawn times 32/512/variable",
             juce::String (runs[1].size()) + " / " + juce::String (runs[0].size()) + " / " + juce::String (runs[2].size()));

       #ifdef GS_BASELINE_DSP_DIR
        auto runOld = [&] (old_v305::GrainScheduler& s, const old_v305::SyncInfo& info, int n, juce::int64 pos,
                           std::vector<juce::int64>& times)
        {
            std::vector<old_v305::SpawnRequest> req;
            req.reserve (old_v305::GrainScheduler::kMaxSpawnsPerBlock);
            int gs = -1, ge = -1;
            s.processBlockSync (n, info, 3, 100.0f, kRepeats, pattern, 0, 0, 50.0f, false, req, gs, ge);
            for (auto& r : req) times.push_back (pos + r.sampleOffset);
        };
        auto old512 = schedulerTimes<old_v305::GrainScheduler, old_v305::TempoTracker> (fixedSplit (512), runOld);
        row (static_cast<juce::int64> (old512.size()) < expected, "negative control: v3.0.5 scheduler drops repeats",
             juce::String (old512.size()) + " of " + juce::String (expected) + " emitted at 512");
       #endif
    }

    // ── 12 s ring read precision ────────────────────────────────────────────
    void ringPrecisionRow()
    {
        constexpr double sr = 192000.0;
        constexpr double f  = 1000.0;
        DelayBuffer d;
        d.prepare (sr, 12.0f);
        // Fill past 2^21 so the write head sits high in the ring
        const int pushes = d.getBufferSize() - 10;
        for (int i = 0; i < pushes; ++i)
        {
            const double v = std::sin (2.0 * juce::MathConstants<double>::pi * f * i / sr);
            d.pushSample (static_cast<float> (v), static_cast<float> (v));
        }
        // Tap j = d.getWritePosition() - delay; the newest sample (index pushes-1) is delay 1.
        double worst = 0.0;
        for (int k = 0; k < 200; ++k)
        {
            const double delay = 1000.0 + k * 0.37 + 0.123;
            const double idx = (pushes - 1) - (delay - 1.0);
            const double ref = std::sin (2.0 * juce::MathConstants<double>::pi * f * idx / sr);
            worst = juce::jmax (worst, std::abs (static_cast<double> (d.readSample (0, delay)) - ref));
        }
        row (toDb (worst) <= -100.0, "ring read precision, 192 kHz, head > 2^21",
             juce::String (toDb (worst), 1) + " dB worst error (write head " + juce::String (d.getWritePosition()) + ")");
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    std::printf ("O-GrainScatter block-size null (gate <= %.0f dBFS)\n", kNullDb);

    const auto stimulus = makeStimulus();
    constexpr uint64_t kSeed = 0x0123456789ABCDEFull;

    const Params common {
        { "grain_size", 80.0f }, { "density", 70.0f }, { "pitch_random", 50.0f }, { "pan_random", 60.0f },
        { "reverse", 30.0f }, { "feedback", 30.0f }, { "dry_level", -60.0f }, { "wet_level", 0.0f },
        { "size_random", 50.0f }, { "amp_random", 50.0f }, { "spread", 50.0f }, { "scan_position", 20.0f }
    };

    auto with = [&] (Params extra) { Params p = common; for (auto& [k, v] : extra) p[k] = v; return p; };

    const std::vector<Scenario> scenarios {
        { "free",    with ({}), {} },
        { "sync",    with ({ { "sync_mode", 3.0f }, { "repeats", 4.0f }, { "probability", 70.0f },
                             { "euclidean_swing", 60.0f }, { "euclidean_steps", 8.0f }, { "euclidean_pulses", 5.0f },
                             { "stutter_gate", 1.0f }, { "dry_level", 0.0f } }), {} },
        { "spatial", with ({ { "spatial_mode", 2.0f }, { "trajectory", 3.0f }, { "doppler", 50.0f },
                             { "traj_speed", 200.0f } }), {} },
        { "freeze",  with ({}), { { "freeze", 1.0f } } },
        { "pitch",   with ({ { "pitch_transpose", 24.0f }, { "grain_size", 500.0f }, { "density", 40.0f } }), {} },
    };

    for (const auto& sc : scenarios)
    {
        auto ref = render (*makeInstance (sc.params, kSeed), sc, stimulus, fixedSplit (512));
        const double level = rms (ref);
        row (toDb (level) > -40.0, (juce::String (sc.name) + ": reference non-silent").toRawUTF8(),
             juce::String (toDb (level), 1) + " dBFS RMS");

        auto r32 = render (*makeInstance (sc.params, kSeed), sc, stimulus, fixedSplit (32));
        auto rv  = render (*makeInstance (sc.params, kSeed), sc, stimulus, variableSplit());
        const double d32 = toDb (peakDiff (ref, r32));
        const double dv  = toDb (peakDiff (ref, rv));
        row (d32 <= kNullDb, (juce::String (sc.name) + ": 32 vs 512").toRawUTF8(), juce::String (d32, 1) + " dBFS peak");
        row (dv  <= kNullDb, (juce::String (sc.name) + ": variable vs 512").toRawUTF8(), juce::String (dv, 1) + " dBFS peak");
    }

    // Non-vacuity: another seed must NOT null
    {
        const auto& sc = scenarios[0];
        auto a = render (*makeInstance (sc.params, kSeed), sc, stimulus, fixedSplit (512));
        auto b = render (*makeInstance (sc.params, kSeed + 1), sc, stimulus, fixedSplit (512));
        const double d = toDb (peakDiff (a, b));
        row (d > -40.0, "negative control: different seed differs", juce::String (d, 1) + " dBFS peak");
    }

    // Seed persists through state
    {
        const auto& sc = scenarios[1];
        auto src = makeInstance (sc.params, kSeed);
        juce::MemoryBlock state;
        src->getStateInformation (state);
        auto a = render (*src, sc, stimulus, fixedSplit (512));
        auto restored = makeInstance ({}, 0, &state);
        const bool seedOk = restored->getRngSeed() == kSeed;
        auto b = render (*restored, sc, stimulus, fixedSplit (32));
        const double d = toDb (peakDiff (a, b));
        row (seedOk && d <= kNullDb, "seed survives state round-trip (512 vs restored 32)",
             juce::String (d, 1) + " dBFS peak, seed " + (seedOk ? "match" : "MISMATCH"));
    }

    schedulerRows();
    ringPrecisionRow();

    std::printf ("%s\n", failures == 0 ? "ALL PASS" : "FAILURES");
    return failures == 0 ? 0 : 1;
}
