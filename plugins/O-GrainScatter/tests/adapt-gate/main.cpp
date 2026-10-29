/*
  ==============================================================================

    O-GrainScatter — Grain Adapt gate (v3.5.0, extrema-density grain length)

    Three binaries from this one driver:

      O-GrainScatter-adapt-gate-v340 <dir>    compiled against v3.4.0 (commit 144a73f3,
                                              extracted with git archive at configure
                                              time): renders the reference scenarios and
                                              dumps them, plus a v3.4.0 session, to <dir>.
      O-GrainScatter-adapt-gate <dir>         this build (GS_TEST_SPAWN_LOG): every row.
      O-GrainScatter-adapt-gate-fixedk        negative control (GS_TEST_ADAPT_FIXED_K:
                                              CONTEXT's K = GS * M / len, no K0, no c
                                              solve): the mean-length row must FAIL.

    Rows
      U1  ExtremaMap unit: extremum count M and K0 on the prototype's stimuli match
          research-scripts/proto_output.txt within 1 % (silence: M = 0).
      A   Adapt 0 bit-exact against v3.4.0 on 8 paths; live paths at Adapt 100 too
          (Adapt only touches frozen grains); a v3.4.0 session loaded into an instance
          whose Adapt was moved first renders bit-exact and reads Adapt 0.
      L   sparse/dense snapshot (1 s 55 Hz sine | 1 s noise, equal RMS): mean adapted
          length over the sine half / the noise half >= 3.0 at Adapt 100, >= 1.8 at 50;
          Adapt 0 control = 1.00.
      M   mean adapted length (before Size Random) within 5 % of Grain Size, >= 2000
          spawns, GS 50 / 100 / 200, sparse/dense + bass+snare; forward and reverse.
      P   Grain Size 50 -> 100 doubles every unclamped grain's length (2.00 +- 0.01);
          every logged adapted length equals the map's own lookup at its start.
      F   fallbacks render bit-exact against Adapt 0: silent snapshot, -80 dB noise,
          a loop under 0.2 s, and every grain spawned before the analysis is ready.
      D   block-size null at Adapt 100 in freeze (engage, Adapt automated, re-engage
          mid-analysis, spatial): 32 / variable vs 512 <= -120 dBFS; the first adapted
          spawn lands on the same sample in every split, at engage + ceil(W / 32).
      R   0 allocations in processBlock across engage, analysis, re-engage, release and
          Adapt 0 <-> 100 (liveness: one deliberate new counts 1).
      V   level (16 seeds x 8 s): sparse/dense at Density 100, GS 200, Scan 30, Adapt 50 vs 0 within 0.5 dB
          (old-vs-old seed control <= 0.2 dB, length ratio >= 2 in the same renders);
          dense noise cloud Adapt 100 vs 0 within 0.5 dB; V3 reports Adapt 100 and the
          Scan 0 read-head-floor case (no verdict, a known limitation).
      C   analysis cost per 32-sample block at 48 / 96 kHz (2 s noise): report; FAIL
          if any block's median step exceeds 50 us.

    Exit 0 = every row PASS (fixedk: exit 0 = the negative control FAILS as it must).

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <new>
#include <random>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

// ── allocation counter (R rows): every operator new on the armed thread counts ──
namespace alloccheck
{
    thread_local bool armed = false;
    std::atomic<long> count { 0 };
    inline void note() { if (armed) count.fetch_add (1, std::memory_order_relaxed); }
}

void* operator new (std::size_t size)
{
    alloccheck::note();
    if (void* p = std::malloc (size == 0 ? 1 : size))
        return p;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t size) { return ::operator new (size); }
void* operator new (std::size_t size, std::align_val_t alignment)
{
    alloccheck::note();
    std::size_t a = static_cast<std::size_t> (alignment);
    if (a < sizeof (void*)) a = sizeof (void*);
    void* p = nullptr;
    if (posix_memalign (&p, a, size == 0 ? a : size) == 0 && p != nullptr)
        return p;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t size, std::align_val_t alignment) { return ::operator new (size, alignment); }
void operator delete (void* p) noexcept                                  { std::free (p); }
void operator delete[] (void* p) noexcept                                { std::free (p); }
void operator delete (void* p, std::size_t) noexcept                     { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept                   { std::free (p); }
void operator delete (void* p, std::align_val_t) noexcept                { std::free (p); }
void operator delete[] (void* p, std::align_val_t) noexcept              { std::free (p); }
void operator delete (void* p, std::size_t, std::align_val_t) noexcept   { std::free (p); }
void operator delete[] (void* p, std::size_t, std::align_val_t) noexcept { std::free (p); }

namespace
{
    constexpr double kSampleRate  = 48000.0;
    constexpr int    kMaxBlock    = 512;
    constexpr int    kLength      = 6 * 48000;
    constexpr int    kChangePoint = 98304;   // a block boundary in every split (2.048 s)
    constexpr uint64_t kSeed      = 0x0D15EA5Eull;

    // Freeze at kChangePoint captures the full 2 s: length 96001, xfade 480, loop 95521,
    // snapshot position 0 = absolute sample kChangePoint - 506 - 96001.
    constexpr int kSnapLength = 96001;
    constexpr int kXfade      = 480;
    constexpr int kLoop       = kSnapLength - kXfade;

    int failures = 0;

    void row (bool ok, const char* name, const juce::String& detail)
    {
        std::printf ("  [%s] %-62s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        std::fflush (stdout);
        if (! ok) ++failures;
    }

    double toDb (double x) { return x > 0.0 ? 20.0 * std::log10 (x) : -400.0; }

    using Params = std::map<juce::String, float>;
    struct Event { int pos; Params params; };

    struct Scenario
    {
        const char* name;
        Params params;
        std::vector<Event> events;
    };

    // ── stimuli ─────────────────────────────────────────────────────────────
    enum class Stim { Music, SparseDense, BassSnare, Noise, Noise80, SilenceThenMusic };

    // One period of a periodic stimulus (period kLoop, so the freeze loop is exactly one
    // period and the seam crossfade blends identical audio).
    std::vector<float> periodic (Stim kind)
    {
        std::vector<float> p (static_cast<size_t> (kLoop));
        std::mt19937 g (0x5EED);
        std::uniform_real_distribution<float> u (-1.0f, 1.0f);
        const double tp = 2.0 * juce::MathConstants<double>::pi;
        for (int k = 0; k < kLoop; ++k)
        {
            const double t = k / kSampleRate;
            double v = 0.0;
            if (kind == Stim::SparseDense)
                v = k < kLoop / 2 ? 0.5 * std::sin (tp * 55.0 * t) : 0.612 * u (g);   // equal RMS 0.354
            else
            {
                const double ph = std::fmod (t, 0.5);
                v = 0.5 * std::exp (-ph * 3.0) * std::sin (tp * 60.0 * t);
                const double ps = std::fmod (t + 0.25, 0.5);
                v += 0.4 * std::exp (-ps / 0.06) * u (g);
            }
            p[static_cast<size_t> (k)] = static_cast<float> (v);
        }
        return p;
    }

    juce::AudioBuffer<float> makeStimulus (Stim kind, int length = kLength)
    {
        juce::AudioBuffer<float> b (2, length);
        juce::Random r (0x5EEDull);
        std::vector<float> per;
        if (kind == Stim::SparseDense || kind == Stim::BassSnare)
            per = periodic (kind);
        for (int i = 0; i < length; ++i)
        {
            const double t = i / kSampleRate;
            float l = 0.0f, rr = 0.0f;
            if (! per.empty())
                l = rr = per[static_cast<size_t> (i % kLoop)];
            else if (kind == Stim::Noise)
            {
                l  = (r.nextFloat() * 2.0f - 1.0f) * 0.3f;
                rr = (r.nextFloat() * 2.0f - 1.0f) * 0.3f;
            }
            else if (kind == Stim::Noise80)
            {
                l  = (r.nextFloat() * 2.0f - 1.0f) * 1.0e-4f;
                rr = (r.nextFloat() * 2.0f - 1.0f) * 1.0e-4f;
            }
            else if (kind == Stim::SilenceThenMusic && i < kChangePoint + 4800)
                l = rr = 0.0f;
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

    std::vector<int> fixedSplit (int n, int length = kLength)
    {
        std::vector<int> s;
        for (int pos = 0; pos < length; pos += n)
            s.push_back (juce::jmin (n, length - pos));
        return s;
    }

    // Random block sizes 1..512, cut at every event position.
    std::vector<int> variableSplit (const std::vector<Event>& events, int length = kLength)
    {
        std::vector<int> s;
        juce::Random r (0xB10CCull);
        int pos = 0;
        while (pos < length)
        {
            int n = 1 + r.nextInt (kMaxBlock);
            for (const auto& e : events)
                if (pos < e.pos && pos + n > e.pos) n = e.pos - pos;
            n = juce::jmin (n, length - pos);
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

    std::unique_ptr<GrainScatterProcessor> makeInstance (const Params& params, uint64_t seed = kSeed,
                                                         const juce::MemoryBlock* state = nullptr,
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
            p->setRngSeed (seed);
        }
        p->prepareToPlay (kSampleRate, kMaxBlock);
        p->reset();
        return p;
    }

   #ifdef GS_TEST_SPAWN_LOG
    using SpawnEntry = GrainScatterProcessor::SpawnLogEntry;
   #else
    struct SpawnEntry {};
   #endif

    struct Hooks
    {
        std::vector<SpawnEntry>* log = nullptr;   // drained after every block
        bool countAllocs = false;                 // R rows: arm around processBlock (not the first)
    };

    juce::AudioBuffer<float> render (GrainScatterProcessor& p, const std::vector<Event>& events,
                                     const juce::AudioBuffer<float>& in, const std::vector<int>& split,
                                     Hooks hooks = {})
    {
        const int length = in.getNumSamples();
        juce::AudioBuffer<float> out (2, length);
        juce::AudioBuffer<float> block (2, kMaxBlock);
        juce::MidiBuffer midi;
        midi.ensureSize (256);
        int pos = 0;
        bool first = true;
       #ifdef GS_TEST_SPAWN_LOG
        uint64_t logged = p.getSpawnLogCount();
       #endif
        for (int n : split)
        {
            for (const auto& e : events)
                if (e.pos == pos)
                    setParams (p, e.params);
            block.setSize (2, n, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
                block.copyFrom (ch, 0, in, ch, pos, n);
            if (hooks.countAllocs && ! first) alloccheck::armed = true;
            p.processBlock (block, midi);
            alloccheck::armed = false;
            first = false;
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, block, ch, 0, n);
           #ifdef GS_TEST_SPAWN_LOG
            if (hooks.log != nullptr)
                for (; logged < p.getSpawnLogCount(); ++logged)
                    hooks.log->push_back (p.getSpawnLogEntry (logged));
           #endif
            pos += n;
        }
        return out;
    }

    double peakDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, int from = 0, int to = -1)
    {
        if (to < 0) to = juce::jmin (a.getNumSamples(), b.getNumSamples());
        double m = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = from; i < to; ++i)
                m = juce::jmax (m, std::abs (static_cast<double> (a.getSample (ch, i)) - b.getSample (ch, i)));
        return m;
    }

    double meanSquare (const juce::AudioBuffer<float>& a, int from = 0, int to = -1)
    {
        if (to < 0) to = a.getNumSamples();
        double s = 0.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = from; i < to; ++i)
                s += static_cast<double> (a.getSample (ch, i)) * a.getSample (ch, i);
        return s / (2.0 * (to - from));
    }

    double rms (const juce::AudioBuffer<float>& a, int from = 0) { return std::sqrt (meanSquare (a, from)); }

    Params wetOnly (Params p) { p["dry_level"] = -60.0f; return p; }

    // ── reference scenarios (v3.4.0 parameters only) ────────────────────────
    constexpr int kChange2 = 2 * kChangePoint;
    constexpr int kChange3 = 2 * kChangePoint + 49152;

    std::vector<Scenario> referenceScenarios()
    {
        const std::vector<Event> freezeAt { { kChangePoint, { { "freeze", 1.0f } } } };
        return {
            { "free", { { "density", 70.0f }, { "grain_size", 80.0f }, { "spread", 40.0f }, { "pitch_random", 60.0f },
                        { "pan_random", 80.0f }, { "reverse", 30.0f }, { "feedback", 35.0f }, { "size_random", 50.0f },
                        { "amp_random", 40.0f }, { "scan_position", 20.0f } }, {} },
            { "sync", { { "sync_mode", 3.0f }, { "repeats", 4.0f }, { "probability", 70.0f }, { "euclidean_swing", 62.0f },
                        { "euclidean_pulses", 5.0f }, { "euclidean_steps", 8.0f }, { "stutter_gate", 1.0f },
                        { "grain_size", 60.0f }, { "pitch_random", 30.0f } }, {} },
            { "spatial", { { "spatial_mode", 2.0f }, { "trajectory", 3.0f }, { "doppler", 60.0f }, { "density", 60.0f },
                           { "spread", 30.0f }, { "pitch_random", 20.0f } }, {} },
            { "freeze", { { "density", 60.0f }, { "spread", 20.0f }, { "grain_size", 120.0f } }, freezeAt },
            { "freeze-rev-size-tilt", { { "density", 70.0f }, { "spread", 40.0f }, { "grain_size", 150.0f }, { "reverse", 50.0f },
                                        { "size_random", 50.0f }, { "shape_tilt", -80.0f } }, freezeAt },
            { "freeze-pitch24", { { "density", 50.0f }, { "grain_size", 200.0f }, { "pitch_transpose", 24.0f },
                                  { "spread", 30.0f } }, freezeAt },
            { "freeze-spatial", { { "spatial_mode", 1.0f }, { "density", 60.0f }, { "grain_size", 100.0f }, { "spread", 30.0f } },
                                freezeAt },
            { "freeze-sync-reengage", { { "sync_mode", 3.0f }, { "repeats", 3.0f }, { "grain_size", 90.0f }, { "reverse", 30.0f } },
                                      { { kChangePoint, { { "freeze", 1.0f } } }, { kChange2, { { "freeze", 0.0f } } },
                                        { kChange3, { { "freeze", 1.0f } } } } },
        };
    }

    [[maybe_unused]] bool writeBuffer (const juce::File& f, const juce::AudioBuffer<float>& b)
    {
        juce::MemoryBlock mb;
        for (int ch = 0; ch < 2; ++ch)
            mb.append (b.getReadPointer (ch), sizeof (float) * static_cast<size_t> (b.getNumSamples()));
        return f.replaceWithData (mb.getData(), mb.getSize());
    }

    [[maybe_unused]] bool readBuffer (const juce::File& f, juce::AudioBuffer<float>& b)
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

#if defined (GS_ADAPT_V340)
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
        if (juce::String (sc.name) == "freeze")
        {
            juce::MemoryBlock state;
            p->getStateInformation (state);
            dir.getChildFile ("state_freeze.bin").replaceWithData (state.getData(), state.getSize());
        }
        const auto out = render (*p, sc.events, in, fixedSplit (kMaxBlock));
        if (! writeBuffer (dir.getChildFile (juce::String (sc.name) + ".f32"), out))
            return 1;
        std::printf ("  v3.4.0 reference %-22s rms %.2f dBFS\n", sc.name, toDb (rms (out)));
    }
    return 0;
}
#else
// ════════════════════════════════════════════════════════════════════════════
#include "dsp/ExtremaMap.h"

namespace
{
    // A snapshot-shaped fake ring over two vectors (absolute index = base + vector index).
    struct FakeRing
    {
        const std::vector<float>* L;
        const std::vector<float>* R;
        std::int64_t base;
        void copyLevel (int, int ch, std::int64_t first, int n, float* dst) const noexcept
        {
            const auto& v = ch != 0 ? *R : *L;
            std::memcpy (dst, v.data() + (first - base), sizeof (float) * static_cast<size_t> (n));
        }
    };

    // The prototype's stimuli (research-scripts/extrema_proto.cpp makeStim), 2 s + 1.
    void protoStim (int kind, double fs, std::vector<float>& l, std::vector<float>& r)
    {
        const int N = static_cast<int> (fs * 2.0) + 1;
        l.assign (static_cast<size_t> (N), 0.0f);
        r.assign (static_cast<size_t> (N), 0.0f);
        std::mt19937 g (0x5EED);
        std::uniform_real_distribution<float> u (-1.0f, 1.0f);
        const double tp = 6.283185307179586;
        for (int i = 0; i < N; ++i)
        {
            float a = 0.0f, b = 0.0f;
            if (kind == 0) a = b = static_cast<float> (0.5 * std::sin (tp * 55.0 * i / fs));
            else if (kind == 1) { a = 0.3f * u (g); b = 0.3f * u (g); }
            l[static_cast<size_t> (i)] = a;
            r[static_cast<size_t> (i)] = b;
        }
    }

    double nowUs()
    {
        using namespace std::chrono;
        return duration<double, std::micro> (steady_clock::now().time_since_epoch()).count();
    }

    // Spawn-log statistics over adapted frozen grains.
    struct LenStats
    {
        int n = 0, frozen = 0, frozenFixed = 0;
        double sum = 0.0, sumA = 0.0, sumB = 0.0;
        int nA = 0, nB = 0;
        juce::int64 firstAdapted = -1, lastFixedFrozen = -1;
    };

    // Stimulus phase of a loop coordinate: snapshot position b = xfade + j is absolute
    // sample origin + b, origin = engage - captureEnd - length; the input is periodic in kLoop.
    double stimPhase (double loopPos, int engageAbs)
    {
        const double origin = engageAbs - FreezeManager::kCaptureEndDelay - kSnapLength;
        return std::fmod (origin + kXfade + loopPos, static_cast<double> (kLoop));
    }

    LenStats lengthStats (const std::vector<SpawnEntry>& log, int engageAbs = kChangePoint)
    {
        LenStats s;
        for (const auto& e : log)
        {
            if (! e.frozen) continue;
            ++s.frozen;
            if (! e.adapted)
            {
                ++s.frozenFixed;
                s.lastFixedFrozen = juce::jmax (s.lastFixedFrozen, e.absSample);
                continue;
            }
            if (s.firstAdapted < 0) s.firstAdapted = e.absSample;
            ++s.n;
            s.sum += e.preRandomLength;
            if (stimPhase (e.loopPos, engageAbs) < kLoop / 2) { s.sumA += e.preRandomLength; ++s.nA; }
            else                                               { s.sumB += e.preRandomLength; ++s.nB; }
        }
        return s;
    }

    double halfRatio (const LenStats& s)
    {
        return (s.nA > 0 && s.nB > 0) ? (s.sumA / s.nA) / (s.sumB / s.nB) : 0.0;
    }

    // Adapt-on scenario on a periodic stimulus, frozen at kChangePoint.
    Params adaptParams (float gsMs, float adapt, float reverse = 0.0f)
    {
        return wetOnly ({ { "density", 100.0f }, { "grain_size", gsMs }, { "spread", 50.0f }, { "scan_position", 25.0f },
                          { "grain_adapt", adapt }, { "reverse", reverse } });
    }

    const std::vector<Event> kFreezeAt { { kChangePoint, { { "freeze", 1.0f } } } };

    struct Rendered
    {
        juce::AudioBuffer<float> out;
        std::vector<SpawnEntry> log;
        std::unique_ptr<GrainScatterProcessor> proc;
    };

    Rendered renderLogged (const Params& params, const juce::AudioBuffer<float>& in, const std::vector<Event>& events,
                           const std::vector<int>& split, uint64_t seed = kSeed)
    {
        Rendered r;
        r.proc = makeInstance (params, seed);
        r.log.reserve (8192);
        Hooks h; h.log = &r.log;
        r.out = render (*r.proc, events, in, split, h);
        return r;
    }

    // Hash-mixed seeds (never seed + i: neighbouring seeds share no structure this way).
    uint64_t seedAt (int set, int i) { return CounterRng::mix (CounterRng::mix (0xA0A9u + static_cast<uint64_t> (set)) ^ static_cast<uint64_t> (i)); }

   #ifdef GS_TEST_ADAPT_FIXED_K
    int runFixedK()
    {
        std::printf ("\n-- N1. negative control: fixed K = GS x M / len, no K0, no c solve (must FAIL M1)\n");
        const int len = 26 * 48000;
        const auto sd = makeStimulus (Stim::SparseDense, len);
        const auto r = renderLogged (adaptParams (100.0f, 100.0f), sd, kFreezeAt, fixedSplit (kMaxBlock, len));
        const auto s = lengthStats (r.log);
        const double gs = 100.0 * 0.001 * kSampleRate;
        const double err = s.n > 0 ? (s.sum / s.n) / gs - 1.0 : 0.0;
        const bool failsAsItMust = s.n >= 2000 && std::abs (err) > 0.05;
        std::printf ("  [%s] %-62s %s\n", failsAsItMust ? "PASS" : "FAIL",
                     "N1 fixed-K mean length misses GS 100 ms by > 5 % (M1 FAILS)",
                     (juce::String (s.n) + " spawns, mean " + juce::String (s.sum / juce::jmax (1, s.n) / 48.0, 1)
                      + " ms (" + juce::String (err * 100.0, 1) + " %)").toRawUTF8());
        return failsAsItMust ? 0 : 1;
    }
   #endif
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
   #ifdef GS_TEST_ADAPT_FIXED_K
    juce::ignoreUnused (argc, argv);
    return runFixedK();
   #else
    if (argc < 2) { std::printf ("usage: %s <dir with the v3.4.0 dump>\n", argv[0]); return 2; }
    juce::File dir (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    const double fs = kSampleRate;

    // ── U1: ExtremaMap vs the research prototype ────────────────────────────
    std::printf ("\n-- U1. ExtremaMap unit vs research-scripts/proto_output.txt\n");
    {
        struct Want { const char* name; int kind; int M; double K0; };
        // silence: kind 2 (no branch in protoStim writes it)
        for (const Want w : { Want { "sine 55 Hz", 0, 220, 11.0 }, Want { "white noise", 1, 47030, 2366.4 }, Want { "silence", 2, 0, 0.0 } })
        {
            std::vector<float> l, r;
            protoStim (w.kind, fs, l, r);
            const int N = static_cast<int> (l.size());
            const int xf = juce::jmin (static_cast<int> (fs * 0.010), N / 4);
            FakeRing ring { &l, &r, 1000 };
            ExtremaMap em;
            em.prepare (fs, N);
            em.begin (1000, N, xf, N - xf, 0);
            em.step (ring, std::int64_t (1) << 30);
            const int M = em.getNumExtrema();
            const bool okM = w.M == 0 ? M == 0 : std::abs (M - w.M) <= 0.01 * w.M;
            const bool okK = w.M == 0 ? ! em.isDone() : (em.isDone() && std::abs (em.getK0() - w.K0) <= 0.01 * w.K0);
            row (okM && okK, (juce::String ("U1 ") + w.name + ": M and K0 within 1 % of the prototype").toRawUTF8(),
                 "M " + juce::String (M) + " (proto " + juce::String (w.M) + "), K0 " + juce::String (em.getK0(), 1)
                 + " (proto " + juce::String (w.K0, 1) + ")" + (em.isDone() ? "" : ", fallback"));
        }
    }

    const auto music = makeStimulus (Stim::Music);

    // ── A: Adapt 0 bit-exact vs v3.4.0; live paths at Adapt 100 too ─────────
    std::printf ("\n-- A. Adapt 0 bit-exact against v3.4.0 (144a73f3); live grains untouched at Adapt 100\n");
    {
        std::vector<std::pair<Scenario, juce::String>> runs;   // scenario, reference name
        for (const auto& sc0 : referenceScenarios())
        {
            Scenario sc = sc0;
            sc.params["grain_adapt"] = 0.0f;
            runs.push_back ({ sc, sc0.name });
        }
        for (const char* live : { "free", "sync", "spatial" })
            for (const auto& sc0 : referenceScenarios())
                if (juce::String (sc0.name) == live)
                {
                    Scenario sc = sc0;
                    sc.params["grain_adapt"] = 100.0f;
                    runs.push_back ({ sc, juce::String (sc0.name) + "@adapt100" });
                }

        for (const auto& [sc, label] : runs)
        {
            const juce::String refName = label.upToFirstOccurrenceOf ("@", false, false);
            juce::AudioBuffer<float> ref;
            if (! readBuffer (dir.getChildFile (refName + ".f32"), ref))
            {
                row (false, ("A " + label + " reference present").toRawUTF8(), "run -v340 first");
                continue;
            }
            auto p = makeInstance (sc.params);
            const auto out = render (*p, sc.events, music, fixedSplit (kMaxBlock));
            const double d = peakDiff (out, ref);
            row (d == 0.0 && rms (ref) > 1e-4, ("A " + label + " == v3.4.0 bit for bit").toRawUTF8(),
                 "peak diff " + juce::String (d, 10) + " (ref rms " + juce::String (toDb (rms (ref)), 1) + " dBFS)");
        }

        juce::MemoryBlock state;
        juce::AudioBuffer<float> ref;
        if (dir.getChildFile ("state_freeze.bin").loadFileAsData (state) && readBuffer (dir.getChildFile ("freeze.f32"), ref))
        {
            Scenario sc;
            for (const auto& s : referenceScenarios())
                if (juce::String (s.name) == "freeze") sc = s;
            auto p = makeInstance ({}, kSeed, &state, { { "grain_adapt", 50.0f } });
            const float adaptAfter = p->parameters.getRawParameterValue ("grain_adapt")->load();
            const auto out = render (*p, sc.events, music, fixedSplit (kMaxBlock));
            const double d = peakDiff (out, ref);
            row (d == 0.0 && adaptAfter == 0.0f, "A v3.4.0 freeze session into an Adapt 50 instance == v3.4.0",
                 "adapt " + juce::String (adaptAfter) + ", peak diff " + juce::String (d, 10));
        }
        else
            row (false, "A v3.4.0 session state present", "run -v340 first");
    }

    const int longLen = 26 * 48000;   // >= 2000 adapted spawns at 100 g/s
    const auto sparseDense = makeStimulus (Stim::SparseDense, longLen);
    const auto bassSnare = makeStimulus (Stim::BassSnare, longLen);
    const auto longSplit = fixedSplit (kMaxBlock, longLen);

    // ── L: content-following lengths ─────────────────────────────────────────
    std::printf ("\n-- L. sparse/dense snapshot: sine-half / noise-half mean length (GS 100 ms)\n");
    {
        const auto sd12 = makeStimulus (Stim::SparseDense, 12 * 48000);
        const auto split12 = fixedSplit (kMaxBlock, 12 * 48000);
        for (auto [adapt, bound] : { std::pair<float, double> { 100.0f, 3.0 }, { 50.0f, 1.8 } })
        {
            const auto r = renderLogged (adaptParams (100.0f, adapt), sd12, kFreezeAt, split12);
            const auto s = lengthStats (r.log);
            const double q = halfRatio (s);
            row (q >= bound && s.nA > 100 && s.nB > 100,
                 (juce::String ("L Adapt ") + juce::String (adapt, 0) + " %: ratio >= " + juce::String (bound, 1)).toRawUTF8(),
                 juce::String (q, 2) + " (" + juce::String (s.nA) + " / " + juce::String (s.nB) + " spawns)");
        }
        {
            const auto r = renderLogged (adaptParams (100.0f, 0.0f), sd12, kFreezeAt, split12);
            // Adapt 0: nothing is adapted, so treat every frozen spawn as fixed length.
            double sA = 0, sB = 0; int nA = 0, nB = 0;
            for (const auto& e : r.log)
                if (e.frozen)
                    (stimPhase (e.loopPos, kChangePoint) < kLoop / 2 ? (sA += e.grainLengthSamples, ++nA)
                                                                     : (sB += e.grainLengthSamples, ++nB));
            const double q = (nA > 0 && nB > 0) ? (sA / nA) / (sB / nB) : 0.0;
            row (std::abs (q - 1.0) <= 0.02 && nA > 100 && nB > 100, "L Adapt 0 control: ratio 1.00 +- 0.02",
                 juce::String (q, 4) + " (" + juce::String (nA) + " / " + juce::String (nB) + ")");
        }
    }

    // ── M: Grain Size is the mean ────────────────────────────────────────────
    std::printf ("\n-- M. mean adapted length (before Size Random) vs Grain Size, Adapt 100\n");
    std::unique_ptr<GrainScatterProcessor> bassSnare100;   // kept frozen for P
    std::vector<SpawnEntry> bassSnare100Log;
    for (float reverse : { 0.0f, 100.0f })
        for (auto [stimName, stim] : { std::pair<const char*, const juce::AudioBuffer<float>*> { "sparse/dense", &sparseDense },
                                       { "bass+snare", &bassSnare } })
            for (float gs : { 50.0f, 100.0f, 200.0f })
            {
                auto r = renderLogged (adaptParams (gs, 100.0f, reverse), *stim, kFreezeAt, longSplit);
                const auto s = lengthStats (r.log);
                const double gsS = static_cast<int> (gs * fs / 1000.0);
                const double err = s.n > 0 ? (s.sum / s.n) / gsS - 1.0 : 1.0;
                row (s.n >= 2000 && std::abs (err) <= 0.05,
                     (juce::String (reverse > 0 ? "M2 reverse " : "M1 ") + stimName + " GS " + juce::String (gs, 0) + " ms: mean within 5 %").toRawUTF8(),
                     juce::String (s.sum / juce::jmax (1, s.n) / 48.0, 2) + " ms (" + juce::String (err * 100.0, 2) + " %, "
                     + juce::String (s.n) + " spawns)");
                if (reverse == 0.0f && gs == 100.0f && juce::String (stimName) == "bass+snare")
                {
                    bassSnare100 = std::move (r.proc);
                    bassSnare100Log = r.log;
                }
            }

    // ── P: proportionality + the spawn path uses the map ─────────────────────
    std::printf ("\n-- P. Grain Size proportionality (bass+snare, Adapt 100)\n");
    if (bassSnare100 != nullptr)
    {
        const auto& fm = bassSnare100->getFreezeManager();
        const auto& map = fm.getExtremaMap();
        const double lo = fm.adaptMinLength(), hi = fm.adaptMaxLength();
        const double g50 = static_cast<int> (50.0 * fs / 1000.0), g100 = static_cast<int> (100.0 * fs / 1000.0);
        const double c50 = fm.solveAdaptC (g50, 1.0), c100 = fm.solveAdaptC (g100, 1.0);
        double rmin = 1e9, rmax = 0.0;
        int unclamped = 0, mismatches = 0, adaptedN = 0;
        for (const auto& e : bassSnare100Log)
        {
            if (! e.adapted) continue;
            ++adaptedN;
            const double l50 = map.lengthAt (e.loopPos, e.reverse, c50, g50, 1.0, lo, hi);
            const double l100 = map.lengthAt (e.loopPos, e.reverse, c100, g100, 1.0, lo, hi);
            if (juce::jmax (1, static_cast<int> (l100)) != e.grainLengthSamples) ++mismatches;
            if (l50 > lo && l50 < hi && l100 > lo && l100 < hi)
            {
                ++unclamped;
                rmin = juce::jmin (rmin, l100 / l50);
                rmax = juce::jmax (rmax, l100 / l50);
            }
        }
        row (unclamped > 1000 && std::abs (rmin - 2.0) <= 0.01 && std::abs (rmax - 2.0) <= 0.01,
             "P1 GS 50 -> 100: unclamped grains scale 2.00 +- 0.01",
             "ratio " + juce::String (rmin, 6) + " .. " + juce::String (rmax, 6) + " over " + juce::String (unclamped) + " grains");
        row (adaptedN > 1000 && mismatches == 0, "P1 every logged adapted length == the map's lookup at its start",
             juce::String (mismatches) + " mismatches in " + juce::String (adaptedN));
    }
    else
        row (false, "P1 bass+snare render present", "");

    // ── F: fallbacks ─────────────────────────────────────────────────────────
    std::printf ("\n-- F. fallbacks: fixed length, bit-exact against Adapt 0\n");
    {
        struct FCase { const char* name; Stim stim; std::vector<Event> events; };
        const std::vector<FCase> cases {
            { "silent snapshot", Stim::SilenceThenMusic, kFreezeAt },
            { "-80 dB noise", Stim::Noise80, kFreezeAt },
            { "loop < 0.2 s (freeze at 85 ms)", Stim::Music, { { 4096, { { "freeze", 1.0f } } } } },
        };
        for (const auto& c : cases)
        {
            const auto in = makeStimulus (c.stim);
            const auto a = renderLogged (adaptParams (100.0f, 100.0f), in, c.events, fixedSplit (kMaxBlock));
            const auto b = renderLogged (adaptParams (100.0f, 0.0f), in, c.events, fixedSplit (kMaxBlock));
            const auto s = lengthStats (a.log);
            const double d = peakDiff (a.out, b.out);
            const bool audible = c.stim == Stim::SilenceThenMusic || rms (b.out) > 1e-7;
            row (d == 0.0 && s.n == 0 && s.frozen > 100 && audible, (juce::String ("F1 ") + c.name).toRawUTF8(),
                 "peak diff " + juce::String (d, 10) + ", adapted " + juce::String (s.n) + " of " + juce::String (s.frozen)
                 + " frozen spawns, rms " + juce::String (toDb (rms (b.out)), 1) + " dBFS");
        }
        {
            const auto a = renderLogged (adaptParams (100.0f, 100.0f), music, kFreezeAt, fixedSplit (kMaxBlock));
            const auto b = renderLogged (adaptParams (100.0f, 0.0f), music, kFreezeAt, fixedSplit (kMaxBlock));
            const auto readyAbs = static_cast<int> (a.proc->getFreezeManager().getExtremaMap().getReadyAbs());
            const auto s = lengthStats (a.log);
            const double dBefore = peakDiff (a.out, b.out, 0, readyAbs);
            const double dAfter = peakDiff (a.out, b.out, readyAbs);
            row (dBefore == 0.0 && s.frozenFixed > 0 && s.lastFixedFrozen < readyAbs && s.firstAdapted >= readyAbs && dAfter > 1e-4,
                 "F1 spawns before readyAbs: fixed length, output identical until then",
                 "readyAbs " + juce::String (readyAbs) + ": diff before " + juce::String (dBefore, 10) + ", after "
                 + juce::String (toDb (dAfter), 1) + " dBFS; " + juce::String (s.frozenFixed) + " fixed frozen spawns, last at "
                 + juce::String (s.lastFixedFrozen) + ", first adapted at " + juce::String (s.firstAdapted));
        }
    }

    // ── D: block-size determinism ────────────────────────────────────────────
    std::printf ("\n-- D. block-size null at Adapt 100 in freeze\n");
    {
        const std::vector<Scenario> scs {
            { "engage", { { "density", 80.0f }, { "grain_adapt", 100.0f }, { "reverse", 30.0f }, { "size_random", 30.0f },
                          { "shape_tilt", -50.0f }, { "spread", 50.0f }, { "pitch_random", 30.0f }, { "feedback", 20.0f } },
              kFreezeAt },
            { "adapt 0 -> 100 + size", { { "density", 80.0f }, { "grain_size", 150.0f }, { "spread", 40.0f } },
              { { kChangePoint, { { "freeze", 1.0f } } }, { kChange2, { { "grain_adapt", 100.0f }, { "grain_size", 60.0f } } } } },
            { "re-engage mid-analysis", { { "density", 90.0f }, { "grain_adapt", 100.0f }, { "reverse", 50.0f } },
              { { kChangePoint, { { "freeze", 1.0f } } }, { kChangePoint + 1024, { { "freeze", 0.0f } } },
                { kChangePoint + 2048, { { "freeze", 1.0f } } } } },
            { "spatial trajectory", { { "spatial_mode", 2.0f }, { "trajectory", 1.0f }, { "doppler", 40.0f }, { "density", 70.0f },
                                      { "grain_adapt", 100.0f } }, kFreezeAt },
        };
        for (const auto& sc : scs)
        {
            const auto r512 = renderLogged (sc.params, music, sc.events, fixedSplit (512));
            const auto r32  = renderLogged (sc.params, music, sc.events, fixedSplit (32));
            const auto rvar = renderLogged (sc.params, music, sc.events, variableSplit (sc.events));
            const double d32 = toDb (peakDiff (r512.out, r32.out)), dvar = toDb (peakDiff (r512.out, rvar.out));
            const auto s512 = lengthStats (r512.log), s32 = lengthStats (r32.log), svar = lengthStats (rvar.log);
            row (d32 <= -120.0 && dvar <= -120.0 && s512.n > 50,
                 (juce::String ("D1 ") + sc.name + ": 32 / variable null the 512 render").toRawUTF8(),
                 juce::String (d32, 1) + " / " + juce::String (dvar, 1) + " dBFS (" + juce::String (s512.n) + " adapted spawns)");

            if (juce::String (sc.name) == "engage" || juce::String (sc.name) == "re-engage mid-analysis")
            {
                const int engage = juce::String (sc.name) == "engage" ? kChangePoint : kChangePoint + 2048;
                const int W = juce::jmin (ExtremaMap::kWarmup, kLoop - 1) + kLoop + ExtremaMap::kCalibSteps * ExtremaMap::kCalibUnits;
                const juce::int64 want = engage + (W + ExtremaMap::kRate - 1) / ExtremaMap::kRate;
                const auto readyAbs = r512.proc->getFreezeManager().getExtremaMap().getReadyAbs();
                bool allAdaptedAfter = true;
                for (const auto& e : r512.log)
                    if (e.frozen && e.absSample >= readyAbs && ! e.adapted) allAdaptedAfter = false;
                row (s512.firstAdapted == s32.firstAdapted && s512.firstAdapted == svar.firstAdapted
                     && readyAbs == want && s512.firstAdapted >= readyAbs && allAdaptedAfter,
                     (juce::String ("D2 ") + sc.name + ": first adapted spawn identical, readyAbs = engage + ceil(W / 32)").toRawUTF8(),
                     "first " + juce::String (s512.firstAdapted) + " / " + juce::String (s32.firstAdapted) + " / "
                     + juce::String (svar.firstAdapted) + "; readyAbs " + juce::String (readyAbs) + " (want " + juce::String (want)
                     + " = " + juce::String ((want - engage) / 48.0, 1) + " ms)");
            }
        }
    }

    // ── R: allocation-free ───────────────────────────────────────────────────
    std::printf ("\n-- R. allocations in processBlock\n");
    {
        alloccheck::count = 0;
        alloccheck::armed = true;
        {   // a direct operator new call (not a new-expression), which may not be elided
            void* volatile probe = ::operator new (16);
            ::operator delete (probe);
        }
        alloccheck::armed = false;
        const long live = alloccheck::count.load();

        const std::vector<Event> events {
            { kChangePoint, { { "freeze", 1.0f } } },
            { kChangePoint + 1024, { { "freeze", 0.0f } } },
            { kChangePoint + 2048, { { "freeze", 1.0f } } },          // re-engage mid-analysis
            { kChange2, { { "grain_adapt", 0.0f } } },
            { kChange2 + 4096, { { "grain_adapt", 100.0f }, { "grain_size", 300.0f } } },
            { kChange3, { { "freeze", 0.0f } } },
            { kChange3 + 8192, { { "freeze", 1.0f }, { "spatial_mode", 1.0f } } },
        };
        long total = 0;
        for (int bs : { 32, 512 })
        {
            auto p = makeInstance ({ { "density", 100.0f }, { "grain_adapt", 100.0f }, { "reverse", 30.0f }, { "size_random", 20.0f } });
            alloccheck::count = 0;
            Hooks h; h.countAllocs = true;
            render (*p, events, music, bs == 32 ? fixedSplit (32) : variableSplit (events), h);
            total += alloccheck::count.load();
        }
        row (live == 1 && total == 0, "R1 0 allocations in processBlock (32 + variable blocks)",
             juce::String (total) + " allocations; liveness: a deliberate new counted " + juce::String (live));
    }

    // ── V: level ─────────────────────────────────────────────────────────────
    std::printf ("\n-- V. level flat with varying lengths (16 hash-mixed seeds x 8 s)\n");
    {
        constexpr int kSeeds = 16;
        const int len = 16 * 48000;
        const auto sd = makeStimulus (Stim::SparseDense, len);
        const auto noise = makeStimulus (Stim::Noise, len);
        const auto split = fixedSplit (kMaxBlock, len);
        // Scan 30 %: no grain reaches the read-head floor (the spawn guard), so every grain
        // pair is incoherent, the premise of an equal-RMS level row. At Scan 0 about half the
        // frozen grains are pinned to the floor and read in step; V3 reports that case.
        auto level = [&] (const juce::AudioBuffer<float>& in, float gs, float adapt, int set, double* ratio, float scan = 30.0f)
        {
            double ms = 0.0, q = 0.0;
            for (int i = 0; i < kSeeds; ++i)
            {
                Params p = wetOnly ({ { "density", 100.0f }, { "grain_size", gs }, { "spread", 100.0f }, { "grain_adapt", adapt },
                                      { "scan_position", scan } });
                const auto r = renderLogged (p, in, kFreezeAt, split, seedAt (set, i));
                const int ready = kChangePoint + 3690;
                const int from = ready + 24000, to = from + 8 * 48000;
                ms += meanSquare (r.out, from, to);
                q += halfRatio (lengthStats (r.log));
            }
            if (ratio != nullptr) *ratio = q / kSeeds;
            return 10.0 * std::log10 (ms / kSeeds);
        };
        double ratio = 0.0;
        const double a50 = level (sd, 200.0f, 50.0f, 0, &ratio);
        const double a0 = level (sd, 200.0f, 0.0f, 0, nullptr);
        const double a0b = level (sd, 200.0f, 0.0f, 1, nullptr);
        row (std::abs (a50 - a0) <= 0.5 && std::abs (a0b - a0) <= 0.2 && ratio >= 2.0,
             "V1 sparse/dense GS 200, Scan 30, Adapt 50 vs 0 within 0.5 dB",
             juce::String (a50 - a0, 3) + " dB (old-vs-old " + juce::String (a0b - a0, 3) + " dB; length ratio "
             + juce::String (ratio, 2) + "; level " + juce::String (a0, 1) + " dBFS)");
        {
            const double a100 = level (sd, 200.0f, 100.0f, 0, nullptr);
            const double f50 = level (sd, 200.0f, 50.0f, 0, nullptr, 0.0f);
            const double f100 = level (sd, 200.0f, 100.0f, 0, nullptr, 0.0f);
            const double f0 = level (sd, 200.0f, 0.0f, 0, nullptr, 0.0f);
            // Report only (known, CHANGELOG 3.5.0): the compensation's coherence term is one
            // rho for the whole cloud, but the floor-pinned cluster is coherent over the sine
            // half and decorrelates over the noise half (the +-6 ct detune), and Adapt moves
            // how many of its grains overlap in each half.
            std::printf ("  [INFO] %-62s %s\n", "V3 report: Adapt 50 / 100 vs 0 at Scan 30 and at Scan 0 (floor)",
                         (juce::String ("Scan 30: ") + juce::String (a50 - a0, 2) + " / " + juce::String (a100 - a0, 2)
                          + " dB; Scan 0: " + juce::String (f50 - f0, 2) + " / " + juce::String (f100 - f0, 2) + " dB").toRawUTF8());
        }
        const double n100 = level (noise, 100.0f, 100.0f, 0, nullptr);
        const double n0 = level (noise, 100.0f, 0.0f, 0, nullptr);
        row (std::abs (n100 - n0) <= 0.5, "V2 dense noise cloud, Adapt 100 vs 0 within 0.5 dB",
             juce::String (n100 - n0, 3) + " dB (level " + juce::String (n0, 1) + " dBFS)");
    }

    // ── C: analysis cost ─────────────────────────────────────────────────────
    std::printf ("\n-- C. analysis step cost per 32-sample block (2 s white noise)\n");
    for (double rate : { 48000.0, 96000.0 })
    {
        std::vector<float> l, r;
        protoStim (1, rate, l, r);
        const int N = static_cast<int> (l.size());
        const int xf = juce::jmin (static_cast<int> (rate * 0.010), N / 4);
        FakeRing ring { &l, &r, 1000 };
        ExtremaMap em;
        em.prepare (rate, N);
        constexpr int kReps = 9;
        std::vector<std::vector<double>> per;   // [block][rep]
        double total = 0.0;
        for (int rep = 0; rep < kReps; ++rep)
        {
            em.begin (1000, N, xf, N - xf, 0);
            std::int64_t end = 0;
            int b = 0;
            const double t0 = nowUs();
            while (em.isAnalysing())
            {
                end += 32;
                const double s0 = nowUs();
                em.step (ring, end);
                const double s1 = nowUs();
                if (static_cast<int> (per.size()) <= b) per.emplace_back();
                per[static_cast<size_t> (b++)].push_back (s1 - s0);
            }
            total += nowUs() - t0;
        }
        double worst = 0.0;
        for (auto& v : per)
        {
            std::nth_element (v.begin(), v.begin() + static_cast<long> (v.size() / 2), v.end());
            worst = juce::jmax (worst, v[v.size() / 2]);
        }
        row (worst <= 50.0, (juce::String ("C1 ") + juce::String (rate / 1000.0, 0) + " kHz: worst block median <= 50 us").toRawUTF8(),
             juce::String (worst, 1) + " us; total " + juce::String (total / kReps, 0) + " us over " + juce::String (per.size())
             + " blocks (" + juce::String (per.size() * 32.0 / rate * 1000.0, 1) + " ms)");
    }

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
   #endif
}
#endif
