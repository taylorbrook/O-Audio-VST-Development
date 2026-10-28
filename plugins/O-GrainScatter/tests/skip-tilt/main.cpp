/*
  ==============================================================================

    O-GrainScatter — Grain Skip + Shape Tilt gate (v3.4.0)

    Two binaries from this one driver:

      O-GrainScatter-skip-tilt-v330 <dir>   compiled against v3.3.0 (commit d920bedd,
                                            extracted with git archive at configure time):
                                            renders the reference scenarios and dumps
                                            them, plus a v3.3.0 session state, to <dir>.
      O-GrainScatter-skip-tilt <dir>        this build: every verdict row.

    Rows
      A  bit-exact defaults: free / sync (repeats, swing, Euclid, stutter gate) /
         spatial (trajectory, Doppler) / freeze / +24 st pitch / Trapezoid / Exp Decay
         at Skip 0 + Tilt 0 null EXACTLY (peak diff == 0) against v3.3.0; a v3.3.0
         session loaded into an instance whose Skip / Tilt were moved first renders
         bit-exact too (the migration writes the defaults).
      S  Grain Skip: 100 % silences the wet path and spawns nothing; 25 / 50 % skip
         that share of grains (binomial bound); with Sync Repeats 4 the skipped share
         is ~50 % (> 25 %, so repeats are skipped one by one, not only the primary)
         and the scheduled total is unchanged; 50 % differs audibly from 0 %.
      D  determinism: Skip 50 + Tilt -100 / +60 at 32-sample and variable blocks null
         the 512-sample render to <= -120 dBFS (free, sync, spatial).
      T  Shape Tilt: peak position mapping; envelope step <= 0.05 per sample at
         +-100 % on every shape with a zero onset (10 ms .. 1 s grains) and a negative
         control with the 2 ms ramp floor removed (step ~1); window moments vs
         numeric integration (all six shapes, peak 0.02 .. 0.98); rendered DC-grain
         step at +-100 % and through a mid-grain -100 -> +100 jump; dense-cloud level
         at +-100 % within 0.5 dB of 0; Exp Decay follows its moments within 1 dB.

    Exit 0 = every row PASS.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    constexpr double kSampleRate  = 48000.0;
    constexpr int    kMaxBlock    = 512;
    constexpr int    kLength      = 6 * 48000;
    constexpr int    kChangePoint = 98304;   // a block boundary in every split
    constexpr uint64_t kSeed      = 0x0D15EA5Eull;

    int failures = 0;

    void row (bool ok, const char* name, const juce::String& detail)
    {
        std::printf ("  [%s] %-58s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    }

    double toDb (double x) { return x > 0.0 ? 20.0 * std::log10 (x) : -400.0; }

    using Params = std::map<juce::String, float>;

    struct Scenario
    {
        const char* name;
        Params params;
        Params atChange;
    };

    enum class Stim { Music, Dc, Noise };

    juce::AudioBuffer<float> makeStimulus (Stim kind)
    {
        juce::AudioBuffer<float> b (2, kLength);
        juce::Random r (0x5EEDull);
        for (int i = 0; i < kLength; ++i)
        {
            const double t = i / kSampleRate;
            float l = 0.0f, rr = 0.0f;
            if (kind == Stim::Dc)
                l = rr = 0.5f;
            else if (kind == Stim::Noise)
            {
                l  = (r.nextFloat() * 2.0f - 1.0f) * 0.3f;
                rr = (r.nextFloat() * 2.0f - 1.0f) * 0.3f;
            }
            else
            {
                const double env = 0.5 + 0.5 * std::sin (2.0 * juce::MathConstants<double>::pi * 0.7 * t);
                const float n = (r.nextFloat() * 2.0f - 1.0f) * 0.05f;
                l  = static_cast<float> (0.3 * env * std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * t)
                                         + 0.1 * std::sin (2.0 * juce::MathConstants<double>::pi * 1337.0 * t)) + n;
                rr = static_cast<float> (0.3 * env * std::sin (2.0 * juce::MathConstants<double>::pi * 330.0 * t)
                                         + 0.1 * std::sin (2.0 * juce::MathConstants<double>::pi * 911.0 * t)) - n;
            }
            b.setSample (0, i, l);
            b.setSample (1, i, rr);
        }
        return b;
    }

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

    std::unique_ptr<GrainScatterProcessor> makeInstance (const Params& params, const juce::MemoryBlock* state = nullptr,
                                                         const Params& before = {})
    {
        std::unique_ptr<GrainScatterProcessor> p (dynamic_cast<GrainScatterProcessor*> (createPluginFilter()));
        p->setPlayConfigDetails (2, 2, kSampleRate, kMaxBlock);
        setParams (*p, before);
        if (state != nullptr)
            p->setStateInformation (state->getData(), static_cast<int> (state->getSize()));
        else
        {
            setParams (*p, params);
            p->setRngSeed (kSeed);
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

    double rms (const juce::AudioBuffer<float>& a, int from = 0)
    {
        double s = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = from; i < kLength; ++i)
                s += static_cast<double> (a.getSample (ch, i)) * a.getSample (ch, i);
        return std::sqrt (s / (2.0 * (kLength - from)));
    }

    // ── Reference scenarios: v3.3.0 parameters only ─────────────────────────
    Params wetOnly (Params p) { p["dry_level"] = -60.0f; return p; }

    std::vector<Scenario> referenceScenarios()
    {
        return {
            { "free", { { "density", 70.0f }, { "grain_size", 80.0f }, { "spread", 40.0f }, { "pitch_random", 60.0f },
                        { "pan_random", 80.0f }, { "reverse", 30.0f }, { "feedback", 35.0f }, { "size_random", 50.0f },
                        { "amp_random", 40.0f }, { "scan_position", 20.0f } }, {} },
            { "sync", { { "sync_mode", 3.0f }, { "repeats", 4.0f }, { "probability", 70.0f }, { "euclidean_swing", 62.0f },
                        { "euclidean_pulses", 5.0f }, { "euclidean_steps", 8.0f }, { "stutter_gate", 1.0f },
                        { "grain_size", 60.0f }, { "pitch_random", 30.0f } }, {} },
            { "spatial", { { "spatial_mode", 2.0f }, { "trajectory", 3.0f }, { "doppler", 60.0f }, { "density", 60.0f },
                           { "spread", 30.0f }, { "pitch_random", 20.0f } }, {} },
            { "freeze", { { "density", 60.0f }, { "spread", 20.0f }, { "grain_size", 120.0f } }, { { "freeze", 1.0f } } },
            { "pitch", { { "pitch_transpose", 24.0f }, { "grain_size", 500.0f }, { "density", 50.0f } }, {} },
            { "trapezoid", { { "grain_shape", 2.0f }, { "density", 80.0f }, { "spread", 60.0f } }, {} },
            { "expdecay", { { "grain_shape", 5.0f }, { "density", 80.0f }, { "spread", 60.0f }, { "reverse", 50.0f } }, {} },
        };
    }

    [[maybe_unused]] bool writeBuffer (const juce::File& f, const juce::AudioBuffer<float>& b)
    {
        juce::MemoryBlock mb;
        for (int ch = 0; ch < 2; ++ch)
            mb.append (b.getReadPointer (ch), sizeof (float) * static_cast<size_t> (kLength));
        return f.replaceWithData (mb.getData(), mb.getSize());
    }

    bool readBuffer (const juce::File& f, juce::AudioBuffer<float>& b)
    {
        juce::MemoryBlock mb;
        if (! f.loadFileAsData (mb) || mb.getSize() != 2 * sizeof (float) * static_cast<size_t> (kLength))
            return false;
        b.setSize (2, kLength);
        const auto* src = static_cast<const float*> (mb.getData());
        for (int ch = 0; ch < 2; ++ch)
            std::memcpy (b.getWritePointer (ch), src + ch * kLength, sizeof (float) * static_cast<size_t> (kLength));
        return true;
    }
}

#ifdef GS_SKIPTILT_V330
// ════════════════════════════════════════════════════════════════════════════
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2) { std::printf ("usage: %s <dir>\n", argv[0]); return 2; }
    juce::File dir (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    dir.createDirectory();

    const auto in = makeStimulus (Stim::Music);
    for (const auto& sc : referenceScenarios())
    {
        auto p = makeInstance (sc.params);
        if (juce::String (sc.name) == "free")
        {
            juce::MemoryBlock state;
            p->getStateInformation (state);
            dir.getChildFile ("state_free.bin").replaceWithData (state.getData(), state.getSize());
        }
        const auto out = render (*p, sc, in, fixedSplit (kMaxBlock));
        if (! writeBuffer (dir.getChildFile (juce::String (sc.name) + ".f32"), out))
            return 1;
        std::printf ("  v3.3.0 reference %-10s rms %.2f dBFS\n", sc.name, toDb (rms (out)));
    }
    return 0;
}
#else
// ════════════════════════════════════════════════════════════════════════════
namespace
{
    // Envelope of one grain as the pool computes it, per sample (0 before / after).
    std::vector<float> grainEnvelope (int shape, float tilt, int len, double minRamp)
    {
        GrainVoice v;
        const float c = GrainPool::tiltPeakFor (tilt, len, minRamp);
        v.tilted = c != 0.5f;
        v.tiltPeak = c;
        v.tiltAtkScale = v.tilted ? 0.5f / c : 1.0f;
        v.tiltRelScale = v.tilted ? 0.5f / (1.0f - c) : 1.0f;
        std::vector<float> e (static_cast<size_t> (len) + 2, 0.0f);
        for (int k = 0; k < len; ++k)
        {
            const float phase = 1.0f - static_cast<float> (len - k) / static_cast<float> (len);
            e[static_cast<size_t> (k) + 1] = GrainPool::computeEnvelope (GrainPool::tiltPhase (phase, v), shape);
        }
        return e;
    }

    double maxStep (const std::vector<float>& e)
    {
        double m = 0.0;
        for (size_t k = 1; k < e.size(); ++k)
            m = juce::jmax (m, std::abs (static_cast<double> (e[k]) - e[k - 1]));
        return m;
    }

    // Largest per-sample step of the wet output relative to its peak, from `from`.
    double renderStepRatio (const juce::AudioBuffer<float>& b, int from, double& peakOut)
    {
        double step = 0.0, peak = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = from; i < kLength; ++i)
            {
                peak = juce::jmax (peak, std::abs (static_cast<double> (b.getSample (ch, i))));
                step = juce::jmax (step, std::abs (static_cast<double> (b.getSample (ch, i)) - b.getSample (ch, i - 1)));
            }
        peakOut = peak;
        return peak > 0.0 ? step / peak : 1e9;
    }

    const char* shapeName (int s)
    {
        static const char* n[] = { "Hann", "Triangle", "Trapezoid", "Tukey", "Blackman", "ExpDecay" };
        return n[s];
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2) { std::printf ("usage: %s <dir with the v3.3.0 dump>\n", argv[0]); return 2; }
    juce::File dir (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));

    const auto music = makeStimulus (Stim::Music);
    const double minRamp = GrainPool::kTiltMinRampMs * 0.001 * kSampleRate;   // 96 samples

    // ── A: bit-exact defaults vs v3.3.0 ─────────────────────────────────────
    std::printf ("\n-- A. defaults (Skip 0, Tilt 0) bit-exact against v3.3.0 (d920bedd)\n");
    for (const auto& sc0 : referenceScenarios())
    {
        Scenario sc = sc0;
        sc.params["grain_skip"] = 0.0f;
        sc.params["shape_tilt"] = 0.0f;
        juce::AudioBuffer<float> ref;
        if (! readBuffer (dir.getChildFile (juce::String (sc.name) + ".f32"), ref))
        {
            row (false, (juce::String ("A ") + sc.name + " reference present").toRawUTF8(), "run -v330 first");
            continue;
        }
        auto p = makeInstance (sc.params);
        const auto out = render (*p, sc, music, fixedSplit (kMaxBlock));
        const double d = peakDiff (out, ref);
        row (d == 0.0 && rms (ref) > 1e-4, (juce::String ("A ") + sc.name + " == v3.3.0 bit for bit").toRawUTF8(),
             "peak diff " + juce::String (d, 10) + " (ref rms " + juce::String (toDb (rms (ref)), 1) + " dBFS)");
    }
    {
        juce::MemoryBlock state;
        juce::AudioBuffer<float> ref;
        const bool ok = dir.getChildFile ("state_free.bin").loadFileAsData (state)
                        && readBuffer (dir.getChildFile ("free.f32"), ref);
        if (ok)
        {
            const auto sc = referenceScenarios()[0];
            auto p = makeInstance ({}, &state, { { "grain_skip", 50.0f }, { "shape_tilt", 80.0f } });
            const float skipAfter = p->parameters.getRawParameterValue ("grain_skip")->load();
            const float tiltAfter = p->parameters.getRawParameterValue ("shape_tilt")->load();
            const auto out = render (*p, sc, music, fixedSplit (kMaxBlock));
            const double d = peakDiff (out, ref);
            row (d == 0.0 && skipAfter == 0.0f && tiltAfter == 0.0f, "A v3.3.0 session into a moved instance == v3.3.0",
                 "skip " + juce::String (skipAfter) + " tilt " + juce::String (tiltAfter) + ", peak diff " + juce::String (d, 10));
        }
        else
            row (false, "A v3.3.0 session state present", "run -v330 first");
    }

    // ── S: Grain Skip ───────────────────────────────────────────────────────
    std::printf ("\n-- S. Grain Skip\n");
    {
        const Params freeBase = wetOnly ({ { "density", 100.0f }, { "grain_size", 40.0f }, { "spread", 30.0f } });
        const Params syncBase = wetOnly ({ { "sync_mode", 4.0f }, { "repeats", 4.0f }, { "grain_size", 30.0f } });
        const Params spatBase = wetOnly ({ { "spatial_mode", 1.0f }, { "density", 100.0f }, { "grain_size", 40.0f } });

        for (auto [name, base] : { std::pair<const char*, Params> { "free", freeBase }, { "sync", syncBase }, { "spatial", spatBase } })
        {
            Params p100 = base; p100["grain_skip"] = 100.0f;
            auto inst = makeInstance (p100);
            const auto out = render (*inst, { name, p100, {} }, music, fixedSplit (kMaxBlock));
            double peak = 0.0;
            for (int ch = 0; ch < 2; ++ch)
                peak = juce::jmax (peak, static_cast<double> (out.getMagnitude (ch, 0, kLength)));
            row (peak == 0.0 && inst->getGrainsSpawned() == 0 && inst->getGrainsSkipped() > 100,
                 (juce::String ("S ") + name + " Skip 100 %: silent wet, nothing spawned").toRawUTF8(),
                 "peak " + juce::String (peak) + ", spawned " + juce::String ((juce::int64) inst->getGrainsSpawned())
                 + ", skipped " + juce::String ((juce::int64) inst->getGrainsSkipped()));
        }

        for (float pct : { 25.0f, 50.0f })
        {
            Params p = freeBase; p["grain_skip"] = pct;
            auto inst = makeInstance (p);
            render (*inst, { "free", p, {} }, music, fixedSplit (kMaxBlock));
            const double n = static_cast<double> (inst->getGrainsSpawned() + inst->getGrainsSkipped());
            const double share = inst->getGrainsSkipped() / n;
            const double sd = std::sqrt (pct / 100.0 * (1.0 - pct / 100.0) / n);
            row (std::abs (share - pct / 100.0) <= 4.0 * sd,
                 (juce::String ("S free Skip ") + juce::String (pct, 0) + " %: skipped share within 4 sd").toRawUTF8(),
                 juce::String (share * 100.0, 2) + " % of " + juce::String (n, 0) + " (sd " + juce::String (sd * 100.0, 2) + " %)");
        }

        {
            Params p0 = syncBase;
            Params p50 = syncBase; p50["grain_skip"] = 50.0f;
            auto i0 = makeInstance (p0);
            auto i50 = makeInstance (p50);
            render (*i0, { "sync", p0, {} }, music, fixedSplit (kMaxBlock));
            const auto o50 = render (*i50, { "sync", p50, {} }, music, fixedSplit (kMaxBlock));
            const auto total0 = i0->getGrainsSpawned() + i0->getGrainsSkipped();
            const auto total50 = i50->getGrainsSpawned() + i50->getGrainsSkipped();
            const double share = static_cast<double> (i50->getGrainsSkipped()) / static_cast<double> (total50);
            const double sd = std::sqrt (0.25 / static_cast<double> (total50));
            row (total0 == total50 && i0->getGrainsSkipped() == 0 && std::abs (share - 0.5) <= 4.0 * sd && share > 0.25 + 4.0 * sd,
                 "S sync Repeats 4, Skip 50 %: repeats skipped one by one",
                 "scheduled " + juce::String ((juce::int64) total0) + " / " + juce::String ((juce::int64) total50)
                 + ", skipped " + juce::String (share * 100.0, 2) + " % (primaries are 25 %)");

            auto i0b = makeInstance (p0);
            const auto o0 = render (*i0b, { "sync", p0, {} }, music, fixedSplit (kMaxBlock));
            const double d = peakDiff (o0, o50);
            row (toDb (d) > -40.0, "S Skip 50 % is audible against Skip 0 % (non-vacuity)",
                 "peak diff " + juce::String (toDb (d), 1) + " dBFS");
        }
    }

    // ── D: determinism across block sizes with Skip + Tilt ──────────────────
    std::printf ("\n-- D. block-size null, Skip 50 %% + Tilt\n");
    {
        const std::vector<Scenario> scs = {
            { "free", { { "density", 90.0f }, { "grain_skip", 50.0f }, { "shape_tilt", -100.0f }, { "spread", 40.0f },
                        { "pitch_random", 50.0f }, { "feedback", 30.0f }, { "size_random", 40.0f }, { "reverse", 30.0f } },
                      { { "shape_tilt", 60.0f } } },
            { "sync", { { "sync_mode", 3.0f }, { "repeats", 4.0f }, { "probability", 80.0f }, { "stutter_gate", 1.0f },
                        { "grain_skip", 50.0f }, { "shape_tilt", 60.0f }, { "euclidean_swing", 60.0f } },
                      { { "shape_tilt", -100.0f } } },
            { "spatial", { { "spatial_mode", 2.0f }, { "trajectory", 3.0f }, { "doppler", 50.0f }, { "density", 80.0f },
                           { "grain_skip", 50.0f }, { "shape_tilt", -100.0f } }, {} },
        };
        for (const auto& sc : scs)
        {
            auto a = makeInstance (sc.params);
            auto b = makeInstance (sc.params);
            auto c = makeInstance (sc.params);
            const auto r512 = render (*a, sc, music, fixedSplit (512));
            const auto r32  = render (*b, sc, music, fixedSplit (32));
            const auto rvar = render (*c, sc, music, variableSplit());
            const double d32 = toDb (peakDiff (r512, r32)), dvar = toDb (peakDiff (r512, rvar));
            row (d32 <= -120.0 && dvar <= -120.0 && rms (r512) > 1e-4,
                 (juce::String ("D ") + sc.name + " 32 / variable null the 512 render").toRawUTF8(),
                 juce::String (d32, 1) + " / " + juce::String (dvar, 1) + " dBFS (rms " + juce::String (toDb (rms (r512)), 1) + ")");
        }
    }

    // ── T: Shape Tilt ───────────────────────────────────────────────────────
    std::printf ("\n-- T. Shape Tilt\n");
    {
        const float c1 = GrainPool::tiltPeakFor (-1.0f, 4800, minRamp);
        const float c2 = GrainPool::tiltPeakFor (1.0f, 4800, minRamp);
        const float c0 = GrainPool::tiltPeakFor (0.0f, 4800, minRamp);
        const float c3 = GrainPool::tiltPeakFor (-1.0f, 480, minRamp);
        const float c4 = GrainPool::tiltPeakFor (1.0f, 100, minRamp);
        const float c5 = GrainPool::tiltPeakFor (0.5f, 4800, minRamp);
        row (std::abs (c1 - 0.02f) < 1e-6f && std::abs (c2 - 0.98f) < 1e-6f && c0 == 0.5f
             && std::abs (c3 - 0.2f) < 1e-6f && c4 == 0.5f && std::abs (c5 - 0.74f) < 1e-6f,
             "T peak mapping (-1 pluck / 0 symmetric / +1 swell, 2 ms floor)",
             "100 ms: " + juce::String (c1, 4) + " / " + juce::String (c0, 4) + " / " + juce::String (c2, 4)
             + "; 10 ms -1: " + juce::String (c3, 4) + "; 2 ms grain: " + juce::String (c4, 4) + "; +0.5: " + juce::String (c5, 4));

        // Envelope steps (shapes with a zero onset; Exp Decay starts at 1 by design).
        double worst = 0.0, worstSym = 0.0;
        juce::String worstAt;
        int peakMiss = 0;
        for (int shape = 0; shape < 5; ++shape)
            for (int len : { 480, 4800, 48000 })
            {
                worstSym = juce::jmax (worstSym, maxStep (grainEnvelope (shape, 0.0f, len, minRamp)));
                for (float tilt : { -1.0f, 1.0f })
                {
                    const auto e = grainEnvelope (shape, tilt, len, minRamp);
                    const double s = maxStep (e);
                    if (s > worst) { worst = s; worstAt = juce::String (shapeName (shape)) + " " + juce::String (len) + " tilt " + juce::String (tilt, 0); }
                    if (shape == 0 || shape == 4)   // single-peak windows: argmax at c * len
                    {
                        const auto it = std::max_element (e.begin(), e.end());
                        const double k = static_cast<double> (std::distance (e.begin(), it) - 1);
                        const double want = GrainPool::tiltPeakFor (tilt, len, minRamp) * len;
                        // Tolerance: 2 samples or 0.1 % of the grain. A 1 s window is flat to
                        // float precision for ~8 samples either side of its peak.
                        if (std::abs (k - want) > juce::jmax (2.0, 1e-3 * len))
                        {
                            ++peakMiss;
                            std::printf ("     peak miss: %s len %d tilt %+.0f at %.0f, want %.1f\n", shapeName (shape), len, tilt, k, want);
                        }
                    }
                }
            }
        row (worst <= 0.05, "T envelope step <= 0.05 at +-100 % (5 shapes, 10 ms .. 1 s)",
             "worst " + juce::String (worst, 4) + " (" + worstAt + "); symmetric worst " + juce::String (worstSym, 4));
        row (peakMiss == 0, "T envelope peak lands at tiltPeak x length (Hann, Blackman)", juce::String (peakMiss) + " misses");

        double ctl = 0.0;
        for (int shape = 0; shape < 5; ++shape)
            ctl = juce::jmax (ctl, maxStep (grainEnvelope (shape, -1.0f, 4800, 0.0)));
        row (ctl > 0.5, "T negative control: no ramp floor -> the pluck edge is a step", "step " + juce::String (ctl, 4));

        // Moments vs numeric integration of the warped window.
        double worstRel = 0.0;
        for (int shape = 0; shape < 6; ++shape)
            for (float c : { 0.02f, 0.2f, 0.35f, 0.5f, 0.65f, 0.8f, 0.98f })
            {
                GrainVoice v;
                v.tilted = c != 0.5f; v.tiltPeak = c;
                v.tiltAtkScale = 0.5f / c; v.tiltRelScale = 0.5f / (1.0f - c);
                constexpr int N = 200000;
                double m1 = 0.0, m2 = 0.0;
                for (int k = 0; k < N; ++k)
                {
                    const float ph = (static_cast<float> (k) + 0.5f) / N;
                    const double w = GrainPool::computeEnvelope (GrainPool::tiltPhase (ph, v), shape);
                    m1 += w; m2 += w * w;
                }
                m1 /= N; m2 /= N;
                const auto mo = GrainPool::windowMoments (shape, c);
                worstRel = juce::jmax (worstRel, std::abs (mo.mu1 - m1) / m1, std::abs (mo.mu2 - m2) / m2);
            }
        row (worstRel <= 2e-3, "T windowMoments(shape, tilt) match numeric integration", "worst rel err " + juce::String (worstRel, 6));

        // Rendered DC grains: sparse, wet only, centred pan.
        const auto dc = makeStimulus (Stim::Dc);
        double worstR = 0.0, worstR0 = 0.0;
        juce::String worstRAt;
        for (int shape = 0; shape < 5; ++shape)
            for (float size : { 10.0f, 200.0f })
                for (float tilt : { -100.0f, 0.0f, 100.0f })
                {
                    Params p = wetOnly ({ { "grain_shape", static_cast<float> (shape) }, { "grain_size", size },
                                          { "density", 1.0f }, { "shape_tilt", tilt } });
                    auto inst = makeInstance (p);
                    const auto out = render (*inst, { "dc", p, {} }, dc, fixedSplit (kMaxBlock));
                    double peak = 0.0;
                    const double r = renderStepRatio (out, 4800, peak);
                    if (tilt == 0.0f) worstR0 = juce::jmax (worstR0, r);
                    else if (r > worstR) { worstR = r; worstRAt = juce::String (shapeName (shape)) + " " + juce::String (size, 0) + " ms " + juce::String (tilt, 0); }
                }
        row (worstR <= 0.05, "T rendered DC grains: step / peak <= 0.05 at +-100 %",
             "worst " + juce::String (worstR, 4) + " (" + worstRAt + "); tilt 0 worst " + juce::String (worstR0, 4));

        {
            Params p = wetOnly ({ { "grain_size", 500.0f }, { "density", 30.0f }, { "shape_tilt", -100.0f } });
            Scenario sc { "jump", p, { { "shape_tilt", 100.0f } } };
            auto inst = makeInstance (p);
            const auto out = render (*inst, sc, dc, fixedSplit (kMaxBlock));
            double peak = 0.0;
            const double r = renderStepRatio (out, 4800, peak);
            row (r <= 0.05, "T tilt jump -100 -> +100 mid-grain: no step (latched per grain)",
                 "step / peak " + juce::String (r, 4));
        }

        // Dense-cloud level against tilt 0.
        const auto noise = makeStimulus (Stim::Noise);
        double worstDb = 0.0, worstExp = 0.0;
        for (int shape : { 0, 2, 5 })
        {
            double ref = 0.0;
            for (float tilt : { 0.0f, -100.0f, 100.0f })
            {
                Params p = wetOnly ({ { "grain_shape", static_cast<float> (shape) }, { "grain_size", 100.0f },
                                      { "density", 100.0f }, { "spread", 100.0f }, { "scan_position", 30.0f },
                                      { "shape_tilt", tilt } });
                auto inst = makeInstance (p);
                const double level = rms (render (*inst, { "lvl", p, {} }, noise, fixedSplit (kMaxBlock)), 48000);
                if (tilt == 0.0f) ref = level;
                else if (shape == 5)
                {
                    // Exp Decay is not symmetric: the warp changes its energy (the moments
                    // say how much). The compensation scales N*mu2 down to 1 and never
                    // boosts, so the level must follow min(1, N*mu2) with N = 10 live
                    // grains (100 g/s x 100 ms) — that is what "sane" means here.
                    const float c = GrainPool::tiltPeakFor (tilt / 100.0f, 4800, minRamp);
                    constexpr double N = 10.0;
                    const double predicted = 10.0 * std::log10 (juce::jmin (1.0, N * GrainPool::windowMoments (5, c).mu2)
                                                                / juce::jmin (1.0, N * GrainPool::windowMoments (5).mu2));
                    worstExp = juce::jmax (worstExp, std::abs (toDb (level / ref) - predicted));
                    std::printf ("     Exp Decay tilt %+.0f: %+.2f dB (moments predict %+.2f dB)\n",
                                 tilt, toDb (level / ref), predicted);
                }
                else worstDb = juce::jmax (worstDb, std::abs (toDb (level / ref)));
            }
        }
        row (worstDb <= 0.5 && worstExp <= 1.0, "T dense-cloud level at +-100 % vs 0 (Hann, Trapezoid <= 0.5 dB; Exp Decay = moments +-1 dB)",
             "worst " + juce::String (worstDb, 3) + " dB; Exp Decay off prediction by " + juce::String (worstExp, 3) + " dB");
    }

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
#endif
