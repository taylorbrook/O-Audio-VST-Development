/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.1 engine gates
    (PLAN-2.1 T11). Included by main.cpp AFTER its helpers (row / Tally / make /
    param / allValues / stateOf) and run from main() after the 55 Stage 1 rows.

    Every row prints: ID, measured value, threshold, margin, and its negative
    control ("ctrl FIRED: ..." / "ctrl NOT FIRED (vacuous): ..."). A row whose
    control does not fire FAILS. Rows marked "ctrl —" have none by plan.

      Q0  waterFillN<4> == waterFill bitwise (10 k vectors)    ctrl noSort differs
      Q1a sum live <= cap every tick (caps 32..256)             ctrl — (Q1b)
      Q1b 0 steady-state R3 kills                               ctrl thinningOff
      Q2  no starvation (A fills cap 64, then B)                ctrl overQuotaOff
      Q3  cap 256 -> 32 converges                               ctrl thinningOff + d25Off
      Q4  per-voice RMS vs output law, 1/4/16 voices            ctrl old-vs-old 0 dB +
                                                                     thinningOff off-law
      T1  MipTape writeRun == MipRing<4>::push bitwise          ctrl input 1e-3 bump
      T2  MipTape readStereo == MipSnapshot bitwise             ctrl input 1e-3 bump
          (the 1-double-ulp halfband hook is reported as INFO on both)
      K1  resampler DC 44.1<->96 k                              ctrl normalise = false
      K2  resampler pass ripple / stop band                     ctrl beta = 2
      K3  equal rates bit-exact                                 ctrl —
      L1  trim 30 s / 2.88 M frames                             ctrl 10 s file untrimmed
      L2  mono file -> L == R                                   ctrl stereo file L != R
      P1  latched rate == exp2(st/12)·tapeRate/fs               ctrl keyTrack 100 differs
      P2  audio pitch +-1 ct (one long grain, zero crossings)   ctrl fine +5 ct
      P3  96 k session, 44.1 k source alias <= -60 dB           ctrl 2.5 k @ +48 present
      P4  +-48 st clamp, level 4; R2 last-1k-frame reads        ctrl —
      S1  scan 100 % / 0 %, Retrigger / Free-run / Ping-pong    ctrl Retrigger vs Free-run
      S2  loop region sorted, 10 ms minimum, no param writes    ctrl —
      V1  4-note chord = 4 rates                                ctrl —
      V2  steal click (poly 4, 6 distinct pitches)              ctrl 0 ms steal ramp
      V3  steal invariance (slot permutation)                   ctrl seed change
      V4  polyphony 12 -> 4 live                                ctrl — (0 ms INFO-fired)
      M1  MIDI ties / zero-length / 600 events                  ctrl —
      E1  ADSR stage times, velocity law, click                 ctrl —
      H1  load while sounding                                   ctrl —
      H2  host idle: pending <= 1, superseded freed             ctrl testNoEntryCounter
      H3  retire vs fade spare (288-slot scan)                  ctrl retireScan256
      H4  SR change 48 -> 96 k mid-note                         ctrl —
      H5  prepare / setState / load stress                      ctrl —
      N1  block-size null 32/441/512/4096/8192/var              ctrl different seed
      N2  unread params bit-identical                           ctrl s1_density
      A1  0 audio-thread allocations (malloc_logger)            ctrl liveness probe

    Synthetic WAVs live in a temp dir under the system temp directory and are
    deleted at exit. No checked-in audio. Sources are position-deterministic.

  ==============================================================================
*/
#pragma once

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "dsp/forked/Quotas.h"
#include "dsp/MipTape.h"
#include "dsp/KaiserResampler.h"

#if JUCE_MAC
 #ifndef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
  #define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0   // no `check` / `verify` macros (Tally::check)
 #endif
 #include <CoreFoundation/CoreFoundation.h>   // CFRunLoopRunInMode: pump the message loop (JUCE_MODAL_LOOPS_PERMITTED = 0)
 #include <execinfo.h>
 #include <pthread.h>
extern "C"
{
    typedef void (ogi21_malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                          uintptr_t result, uint32_t numHotFramesToSkip);
    extern ogi21_malloc_logger_t* malloc_logger;
}
#endif

namespace { namespace eg21
{
    using Proc  = GranulatorInstrumentProcessor;
    using P     = std::vector<std::pair<juce::String, float>>;
    using Log   = std::vector<ogi::GrainLogEntry>;
    using Tape4 = ogi::MipTape<4>;
    using Ring4 = ouaricon::grain::MipRing<4>;
    using Snap4 = ouaricon::grain::MipSnapshot<4>;

    constexpr double        kPi         = 3.14159265358979323846;
    constexpr std::uint64_t kSeed       = 0x5EED2100CAFE0001ull;
    constexpr int           kMaxBlock   = 8192;
    constexpr double        kClickRatio = 2.0;    // click metric: second-difference peak ratio vs its baseline

    //==========================================================================
    // Allocation hook (copied from O-Granulator engine_gates.h at 82406fce,
    // renamed; memory: malloc_logger two traps — volatile flag + counter,
    // scoped to the audio thread = the harness thread inside processBlock).
   #if JUCE_MAC
    volatile bool allocArmed = false;
    volatile int  allocCount = 0;
    volatile bool allocTrace = false;
    pthread_t     audioThread;

    void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        if (allocArmed && (type & 2u) != 0 && pthread_equal (pthread_self(), audioThread))   // MALLOC_LOG_TYPE_ALLOCATE
        {
            allocCount = allocCount + 1;
            if (allocTrace && allocCount == 1)
            {
                void* frames[32];
                backtrace_symbols_fd (frames, backtrace (frames, 32), 2);   // does not malloc
            }
        }
    }
   #endif

    //==========================================================================
    // Small helpers
    juce::String f2 (double v, int dp = 3) { return juce::String (v, dp); }
    juce::String i64 (std::uint64_t v)     { return juce::String ((juce::int64) v); }

    juce::String ctrl (bool fired, const juce::String& what)
    {
        return juce::String (fired ? "ctrl FIRED: " : "ctrl NOT FIRED (vacuous): ") + what;
    }

    double dB (double ratio) { return 20.0 * std::log10 (std::max (1.0e-300, ratio)); }
    bool sameD (double a, double b) { return std::memcmp (&a, &b, sizeof a) == 0; }
    bool sameF (float a, float b)   { return std::memcmp (&a, &b, sizeof a) == 0; }
    juce::String s1 (const char* x) { return juce::String (Ids::stream (1, x)); }
    juce::int64 ceilCell (juce::int64 t) { return ((t + 31) / 32) * 32; }

    /** Runs the message loop for ms milliseconds (loader callAsync / reaper Timer). */
    void pump (int ms)
    {
       #if JUCE_MODAL_LOOPS_PERMITTED
        juce::MessageManager::getInstance()->runDispatchLoopUntil (ms);
       #elif JUCE_MAC
        const double end = juce::Time::getMillisecondCounterHiRes() + (double) ms;
        do
        {
            CFRunLoopRunInMode (kCFRunLoopDefaultMode, 0.001, false);
        }
        while (juce::Time::getMillisecondCounterHiRes() < end);
       #else
        juce::Thread::sleep (ms);
       #endif
    }

    //==========================================================================
    // Parameters. setP lands EXACTLY on v when some normalised value maps there
    // (search a few ulps around convertTo0to1 (v)); otherwise the nearest. Rows
    // that need the exact engine value read it back with rawOf().
    bool setP (Proc& p, const juce::String& id, float v)
    {
        auto* rp  = param (p, id);
        auto* raw = p.parameters.getRawParameterValue (id);
        if (rp == nullptr || raw == nullptr)
        {
            std::printf ("    !! unknown param %s\n", id.toRawUTF8());
            return false;
        }
        const float n0 = rp->convertTo0to1 (v);
        float up = n0, dn = n0;
        for (int k = 0; k < 256; ++k)
        {
            for (float c : { up, dn })
            {
                rp->setValueNotifyingHost (c);
                if (juce::exactlyEqual (raw->load(), v))
                    return true;
            }
            up = std::nextafter (up, 2.0f);
            dn = std::nextafter (dn, -1.0f);
        }
        rp->setValueNotifyingHost (n0);
        return false;
    }

    float rawOf (Proc& p, const juce::String& id)
    {
        auto* r = p.parameters.getRawParameterValue (id);
        return r != nullptr ? r->load() : 0.0f;
    }

    void setAll (Proc& p, const P& ps)
    {
        for (const auto& kv : ps)
            setP (p, kv.first, kv.second);
    }

    P with (P b, const P& over)
    {
        for (const auto& kv : over)
        {
            bool found = false;
            for (auto& x : b)
                if (x.first == kv.first) { x.second = kv.second; found = true; }
            if (! found)
                b.push_back (kv);
        }
        return b;
    }

    /** Every 2.2 control at its neutral value: the s1 randomisation / window /
        scale / solo rows at 0 and s2-s4 disabled (PLAN-2.2 T9, R§12.2). */
    P pins22()
    {
        P p;
        for (const char* x : { "spray", "jitter", "stereo_spread", "gain_rand", "pitch_rand", "reverse_prob",
                               "window", "window_skew", "scale", "solo" })
            p.push_back ({ s1 (x), 0.0f });
        for (int n = 2; n <= 4; ++n)
            p.push_back ({ juce::String (Ids::stream (n, "enable")), 0.0f });
        return p;
    }

    /** Every parameter the 2.1 engine reads, at a known gate baseline, plus
        pins22() (2.2: the deterministic 2.1 rows never rely on spec defaults). */
    P base()
    {
        P b { { "polyphony", 16.0f }, { "grain_limit", 3.0f }, { "pitch_bend_range", 2.0f }, { "output_gain", 0.0f },
              { "amp_attack", 0.0f }, { "amp_decay", 0.0f }, { "amp_sustain", 100.0f }, { "amp_release", 50.0f },
              { "vel_to_amp", 0.0f },
              { s1 ("enable"), 1.0f }, { s1 ("mute"), 0.0f }, { s1 ("level"), 0.0f }, { s1 ("pan"), 0.0f },
              { s1 ("position"), 0.0f }, { s1 ("scan_rate"), 0.0f }, { s1 ("scan_mode"), 1.0f },
              { s1 ("loop_start"), 0.0f }, { s1 ("loop_end"), 100.0f }, { s1 ("root_note"), 48.0f },
              { s1 ("root_fine"), 0.0f }, { s1 ("key_track"), 100.0f }, { s1 ("size"), 100.0f },
              { s1 ("size_rand"), 0.0f }, { s1 ("density"), 10.0f }, { s1 ("pitch"), 0.0f } };
        for (const auto& kv : pins22())
            b.push_back (kv);
        return b;
    }

    /** The 113 IDs the 2.2 engine binds (ControlGrid.h GiParamPointers::bind):
        9 globals + 26 rows × 4 streams. Everything else (148 IDs: the per-grain
        FX rows, sN_size_sync / sN_density_sync, feedback, freeze, reverb, mod,
        voice mode, MPE, record) is unread in 2.2. */
    juce::StringArray readSet22()
    {
        juce::StringArray a { "polyphony", "pitch_bend_range", "grain_limit", "output_gain", "amp_attack", "amp_decay",
                              "amp_sustain", "amp_release", "vel_to_amp" };
        for (int n = 1; n <= 4; ++n)
            for (const char* x : { "enable", "mute", "solo", "level", "pan", "position", "spray", "scan_rate", "scan_mode",
                                   "loop_start", "loop_end", "root_note", "root_fine", "key_track", "size", "size_rand",
                                   "density", "jitter", "pitch", "pitch_rand", "scale", "reverse_prob", "window",
                                   "window_skew", "stereo_spread", "gain_rand" })
                a.add (Ids::stream (n, x));
        return a;
    }

    /** PLAN-2.3 PD10: readSet22() + the 10 per-grain FX rows × 4 streams = 153
        bound IDs (unread 108: sN_size_sync / sN_density_sync, feedback, freeze,
        reverb, mod, voice mode, MPE, record). */
    juce::StringArray readSet23()
    {
        auto a = readSet22();
        for (int n = 1; n <= 4; ++n)
            for (const char* x : { "filter_type", "filter_cutoff", "filter_rand", "filter_res", "drive", "crush",
                                   "crush_prob", "ring_freq", "ring_rand", "ring_mix" })
                a.add (Ids::stream (n, x));
        return a;
    }

    //==========================================================================
    // Position-deterministic sources.
    float hashNoise (juce::int64 i, int ch, float amp)
    {
        std::uint64_t x = static_cast<std::uint64_t> (i) * 0x9E3779B97F4A7C15ull + (ch != 0 ? 0xD1B54A32D192ED03ull : 0x2545F4914F6CDD1Dull);
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ull;
        x ^= x >> 27; x *= 0x94D049BB133111EBull;
        x ^= x >> 31;
        const double u = static_cast<double> (x >> 40) / static_cast<double> (1ull << 24);   // [0, 1)
        return static_cast<float> ((2.0 * u - 1.0) * amp);
    }

    juce::AudioBuffer<float> noiseBuf (int numCh, int n, float amp, int salt = 0)
    {
        juce::AudioBuffer<float> b (numCh, n);
        for (int c = 0; c < numCh; ++c)
        {
            float* d = b.getWritePointer (c);
            for (int i = 0; i < n; ++i)
                d[i] = hashNoise ((juce::int64) i + (juce::int64) salt * 1000003, c, amp);
        }
        return b;
    }

    juce::AudioBuffer<float> sineBuf (int numCh, int n, double hz, double fs, double amp, double phase = 0.0)
    {
        juce::AudioBuffer<float> b (numCh, n);
        for (int c = 0; c < numCh; ++c)
        {
            float* d = b.getWritePointer (c);
            for (int i = 0; i < n; ++i)
                d[i] = static_cast<float> (amp * std::sin (2.0 * kPi * hz * (double) i / fs + phase));
        }
        return b;
    }

    juce::File gTmp;

    /** 32-bit float WAV into the temp dir (deleted at exit). */
    juce::File writeWav (const juce::String& name, const juce::AudioBuffer<float>& b, double rate)
    {
        auto f = gTmp.getChildFile (name);
        f.deleteFile();
        auto fos = std::make_unique<juce::FileOutputStream> (f);
        if (! fos->openedOk())
            return {};
        std::unique_ptr<juce::OutputStream> os (std::move (fos));
        juce::WavAudioFormat wav;
        const auto opts = juce::AudioFormatWriterOptions{}
                              .withSampleRate (rate)
                              .withNumChannels (b.getNumChannels())
                              .withBitsPerSample (32)
                              .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
        auto w = wav.createWriterFor (os, opts);
        if (w == nullptr)
            return {};
        const bool ok = w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        w.reset();
        return ok ? f : juce::File();
    }

    //==========================================================================
    // Session: one processor, an absolute harness clock, MIDI at absolute
    // sample times, events between blocks (blocks are CUT at event times).
    struct Session;
    struct Ev     { juce::int64 at; std::function<void (Session&)> fn; };
    struct MidiAt { juce::int64 t;  juce::MidiMessage m; };

    struct Session
    {
        std::unique_ptr<Proc> p;
        juce::int64 pos = 0;
        std::vector<float> L, R;
        std::vector<MidiAt> midi;
        size_t mcur = 0;
        int late = 0;
        std::vector<Ev> events;
        size_t ecur = 0;
        std::function<void (Session&)> afterBlock;
        bool arm = false;        // A1: arm the allocation hook around processBlock (from the 2nd block)
        bool record = true;
        juce::int64 blocksDone = 0;
        juce::AudioBuffer<float> blk { 2, kMaxBlock };
        juce::MidiBuffer mb;
        juce::Random vr { (juce::int64) 0x0B10C2100 };
        int varStep = 0;

        Session (double fs, const P& params, std::uint64_t seed = kSeed, int prepBlock = 512)
            : p (make())
        {
            setAll (*p, params);
            p->setSeed (seed);
            p->getEngine().hooks.coherenceOff = true;   // PLAN-2.2 T9 / R§12.2: the 2.1 law (c := 0)
            p->prepareToPlay (fs, prepBlock);
            mb.ensureSize (32768);
        }

        ogi::Engine&          eng() { return p->getEngine(); }
        ogi::EngineTestHooks& hk()  { return p->getEngine().hooks; }

        bool load (int slot, const juce::AudioBuffer<float>& pcm, double rate, const juce::String& name = "src")
        {
            juce::String e;
            const bool ok = p->loadSlotSync (slot, pcm, rate, name, e);
            if (! ok)
                std::printf ("    !! loadSlotSync failed: %s\n", e.toRawUTF8());
            return ok;
        }

        void addMidi (juce::int64 t, const juce::MidiMessage& m)
        {
            auto it = std::upper_bound (midi.begin() + (std::ptrdiff_t) mcur, midi.end(), t,
                                        [] (juce::int64 v, const MidiAt& e) { return v < e.t; });
            midi.insert (it, MidiAt { t, m });
        }
        void noteOn (juce::int64 t, int note, int vel = 100) { addMidi (t, juce::MidiMessage::noteOn (1, note, (juce::uint8) vel)); }
        void noteOff (juce::int64 t, int note)               { addMidi (t, juce::MidiMessage::noteOff (1, note)); }

        void at (juce::int64 t, std::function<void (Session&)> fn)
        {
            auto it = std::upper_bound (events.begin() + (std::ptrdiff_t) ecur, events.end(), t,
                                        [] (juce::int64 v, const Ev& e) { return v < e.at; });
            events.insert (it, Ev { t, std::move (fn) });
        }

        int nextVar()
        {
            const int k = varStep++;
            return k == 0 ? 1 : k == 1 ? kMaxBlock : (k % 17 == 0 ? kMaxBlock : (k % 11 == 0 ? 1 : 1 + vr.nextInt (2048)));
        }

        /** Renders n samples in blocks of `block` (0 = seeded variable incl. 1 and 8192). */
        void run (juce::int64 n, int block)
        {
            const juce::int64 end = pos + n;
            while (pos < end)
            {
                while (ecur < events.size() && events[ecur].at <= pos)
                {
                    auto fn = events[ecur].fn;
                    ++ecur;
                    fn (*this);
                }

                juce::int64 b = block <= 0 ? (juce::int64) nextVar() : (juce::int64) block;
                b = std::min (b, end - pos);
                if (ecur < events.size())
                    b = std::min (b, events[ecur].at - pos);
                b = std::clamp<juce::int64> (b, 1, kMaxBlock);
                const int ni = (int) b;

                blk.setSize (2, ni, false, false, true);
                blk.clear();
                mb.clear();
                while (mcur < midi.size() && midi[mcur].t < pos + ni)
                {
                    if (midi[mcur].t >= pos)
                        mb.addEvent (midi[mcur].m, (int) (midi[mcur].t - pos));
                    else
                        ++late;
                    ++mcur;
                }

               #if JUCE_MAC
                if (arm && blocksDone > 0)
                    allocArmed = true;
               #endif
                p->processBlock (blk, mb);
               #if JUCE_MAC
                allocArmed = false;
               #endif

                if (record)
                {
                    const float* l = blk.getReadPointer (0);
                    const float* r = blk.getReadPointer (1);
                    L.insert (L.end(), l, l + ni);
                    R.insert (R.end(), r, r + ni);
                }
                pos += ni;
                ++blocksDone;
                if (afterBlock)
                    afterBlock (*this);
            }
        }
    };

    //==========================================================================
    // Measurements
    double d2Peak (const std::vector<float>& L, const std::vector<float>& R, juce::int64 a, juce::int64 b)
    {
        const juce::int64 n = (juce::int64) std::min (L.size(), R.size());
        a = std::max<juce::int64> (a, 1);
        b = std::min<juce::int64> (b, n - 1);
        double m = 0.0;
        for (juce::int64 i = a; i < b; ++i)
            for (const auto* x : { &L, &R })
            {
                const auto& v = *x;
                const double d = (double) v[(size_t) (i + 1)] - 2.0 * (double) v[(size_t) i] + (double) v[(size_t) (i - 1)];
                m = std::max (m, std::abs (d));
            }
        return m;
    }

    double rmsSt (const std::vector<float>& L, const std::vector<float>& R, juce::int64 a, juce::int64 b)
    {
        const juce::int64 n = (juce::int64) std::min (L.size(), R.size());
        a = std::max<juce::int64> (a, 0);
        b = std::min<juce::int64> (b, n);
        if (b <= a)
            return 0.0;
        double acc = 0.0;
        for (juce::int64 i = a; i < b; ++i)
            acc += (double) L[(size_t) i] * (double) L[(size_t) i] + (double) R[(size_t) i] * (double) R[(size_t) i];
        return std::sqrt (acc / (2.0 * (double) (b - a)));
    }

    /** Single-frequency amplitude: 2 |sum w x e^-iwn| / sum w (hann = analysis window). */
    double toneAmp (const std::vector<float>& x, juce::int64 a, juce::int64 n, double hz, double fs, bool hann)
    {
        double re = 0.0, im = 0.0, ws = 0.0;
        const double w0 = 2.0 * kPi * hz / fs;
        for (juce::int64 i = 0; i < n; ++i)
        {
            const double w = hann ? 0.5 - 0.5 * std::cos (2.0 * kPi * (double) i / (double) (n - 1)) : 1.0;
            const double v = w * (double) x[(size_t) (a + i)];
            re += v * std::cos (w0 * (double) i);
            im -= v * std::sin (w0 * (double) i);
            ws += w;
        }
        return ws > 0.0 ? 2.0 * std::sqrt (re * re + im * im) / ws : 0.0;
    }

    /** Positive-going zero-crossing frequency over [a, b) (linear interpolation). */
    double zcFreq (const std::vector<float>& x, juce::int64 a, juce::int64 b, double fs)
    {
        double first = -1.0, last = -1.0;
        int n = 0;
        for (juce::int64 i = std::max<juce::int64> (a, 1); i < b && i < (juce::int64) x.size(); ++i)
        {
            const double x0 = x[(size_t) (i - 1)], x1 = x[(size_t) i];
            if (x0 < 0.0 && x1 >= 0.0)
            {
                const double t = (double) (i - 1) + x0 / (x0 - x1);
                if (n == 0) first = t;
                last = t;
                ++n;
            }
        }
        return n >= 2 ? (double) (n - 1) * fs / (last - first) : 0.0;
    }

    int countState (ogi::EngineTestHooks& h, bool countedOnly)
    {
        int n = 0;
        for (int k = 0; k < ogi::kVoiceSlots; ++k)
            n += countedOnly ? (h.voiceState[k] == 1 ? 1 : 0) : (h.voiceState[k] != 0 ? 1 : 0);
        return n;
    }

    //==========================================================================
    // Q — grain budget on the voice axis (D12)
    void rowQ0()
    {
        juce::Random r ((juce::int64) 0x51000001);
        constexpr int kN = 10000;
        int mism = 0, ctrlDiff = 0, tieDiff = 0, capped = 0, tieVectors = 0;
        for (int t = 0; t < kN; ++t)
        {
            double d[ogi::kMaxStreams];
            for (auto& x : d)
            {
                const int mode = r.nextInt (4);
                x = mode == 0 ? 0.0 : (mode == 1 ? 12.5 * (double) r.nextInt (8) : 400.0 * r.nextDouble());
            }
            bool tie = false;
            for (int i = 0; i < ogi::kMaxStreams; ++i)
                for (int j = i + 1; j < ogi::kMaxStreams; ++j)
                    tie = tie || sameD (d[i], d[j]);
            tieVectors += tie ? 1 : 0;

            const int cap = 1 + r.nextInt (400);
            const ogi::Quotas a = ogi::waterFill (d, cap);
            auto same = [&a] (const auto& q)
            {
                bool s = sameD (a.lambda, q.lambda) && a.capped == q.capped;
                for (int k = 0; k < ogi::kMaxStreams; ++k)
                    s = s && sameD (a.q[k], q.q[k]);
                return s;
            };
            capped   += a.capped ? 1 : 0;
            mism     += same (ogi::waterFillN<ogi::kMaxStreams> (d, cap)) ? 0 : 1;
            ctrlDiff += same (ogi::waterFillN<ogi::kMaxStreams> (d, cap, ogi::WaterFillTest::noSort)) ? 0 : 1;
            tieDiff  += same (ogi::waterFillN<ogi::kMaxStreams> (d, cap, ogi::WaterFillTest::tiesDescending)) ? 0 : 1;
        }
        const bool fired = ctrlDiff > 0;
        row (mism == 0 && capped > 1000 && tieVectors > 1000 && fired,
             "Q0 waterFillN<4> == waterFill bitwise (q, lambda, capped)",
             "meas " + juce::String (mism) + "/" + juce::String (kN) + " differ | thr 0 | margin " + juce::String (-mism)
                 + " | capped " + juce::String (capped) + ", tie vectors " + juce::String (tieVectors)
                 + " | " + ctrl (fired, "noSort -> " + juce::String (ctrlDiff) + " differ")
                 + " | INFO tiesDescending -> " + juce::String (tieDiff) + " differ (result-invariant by construction)");
    }

    struct CapRun { int cap = 0, liveMaxAll = 0; std::uint64_t r3Steady = 0, spawnsSteady = 0; };

    CapRun runCap (int capIdx, bool thinOff)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "grain_limit", (float) capIdx }, { s1 ("density"), 200.0f }, { s1 ("size"), 2000.0f } }));
        s.hk().thinningOff = thinOff;
        s.record = false;
        s.load (0, noiseBuf (2, (int) (5.0 * fs), 0.25f), fs);
        for (int k = 0; k < 16; ++k)
            s.noteOn (0, 48);
        s.run ((juce::int64) (2.0 * fs) + 64, 512);   // transition window: L_max + 2 cells
        CapRun c;
        c.cap = 32 << capIdx;
        c.liveMaxAll = s.hk().liveMax;
        s.hk().resetCounters();
        s.run ((juce::int64) (2.5 * fs), 512);         // steady state: >= L_max + the longest period (cap 32: T ~ 1 s)
        c.liveMaxAll   = std::max (c.liveMaxAll, s.hk().liveMax);
        c.r3Steady     = s.hk().r3Kills;
        c.spawnsSteady = s.hk().spawns;
        return c;
    }

    void rowsQ1()
    {
        Tally ta, tb;
        juce::String da, db;
        int marginA = INT_MAX;
        for (int ci = 0; ci < 4; ++ci)
        {
            const auto c = runCap (ci, false);
            ta.check (c.liveMaxAll <= c.cap && c.liveMaxAll >= c.cap / 2,
                      "cap " + juce::String (c.cap) + " liveMax " + juce::String (c.liveMaxAll));
            tb.check (c.r3Steady == 0 && c.spawnsSteady > 0, "cap " + juce::String (c.cap) + " r3 " + i64 (c.r3Steady));
            marginA = std::min (marginA, c.cap - c.liveMaxAll);
            da << "  " << c.cap << ":" << c.liveMaxAll;
            db << "  " << c.cap << ":" << i64 (c.r3Steady) << "/" << i64 (c.spawnsSteady);
        }
        const auto k = runCap (1, true);
        const bool fired = k.r3Steady > 0;
        row (ta.bad == 0, "Q1a sum live <= cap at every tick (16 v, d 200, 2000 ms)",
             "meas liveMax" + da + " | thr <= cap (>= cap/2 non-vacuous) | margin " + juce::String (marginA)
                 + " | " + ta.detail (4) + " | ctrl -- (Q1b)");
        row (tb.bad == 0 && fired, "Q1b 0 steady-state R3 kills (after L_max + 2 cells)",
             "meas r3/spawns" + db + " | thr 0 | " + tb.detail (4)
                 + " | " + ctrl (fired, "thinningOff cap 64 -> r3 " + i64 (k.r3Steady)));
    }

    struct Q2R { int fillA = 0, liveA = 0, liveB = 0, mA = 0, mB = 0; };

    Q2R runQ2 (bool overQuotaOff, bool thinOff = false)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "grain_limit", 1.0f }, { s1 ("density"), 200.0f }, { s1 ("size"), 2000.0f } }));
        s.hk().overQuotaOff = overQuotaOff;
        s.hk().periodThinOff = thinOff;   // demand outruns the (finite) quota
        s.record = false;
        s.load (0, noiseBuf (2, (int) (6.0 * fs), 0.25f), fs);
        const juce::int64 tB = (juce::int64) (3.0 * fs);
        Q2R q;
        s.noteOn (0, 48);
        s.noteOn (tB, 50);
        s.at (tB, [&q] (Session& ss) { q.fillA = ss.hk().liveVoice[0]; });
        const juce::int64 tCheck = ceilCell (tB + (juce::int64) (2.0 * fs) + 32);   // L_max + 1 cell
        s.run (tCheck, 512);
        s.run (32, 32);                                                            // hooks = the tick at tCheck
        q.liveA = s.hk().liveVoice[0];
        q.liveB = s.hk().liveVoice[1];
        q.mA = s.hk().quotaM[0];
        q.mB = s.hk().quotaM[1];
        return q;
    }

    void rowQ2()
    {
        // Thinned (shipping) path: B reaches m_B within L_max + 1 cell.
        const auto q = runQ2 (false);
        // Under integer thinning A's excess drains at exactly B's spawn rate, so
        // the over-quota term cannot change the thinned outcome (a control there
        // is vacuous). The anti-starvation mechanism is proven where demand
        // outruns the quota: spawn period unthinned (periodThinOff), quotas finite.
        const auto u = runQ2 (false, true);
        const auto k = runQ2 (true, true);
        const bool okT = q.fillA >= 60 && q.mB > 0 && q.liveB >= q.mB && q.liveA <= q.mA;
        const bool okU = u.mB > 0 && u.liveB >= u.mB && u.liveA <= u.mA + 1;   // +1: the spawning voice may sit one over until its next acquire
        const bool fired = k.liveB < k.mB;
        row (okT && okU && fired,
             "Q2 no starvation: A fills cap 64 for 3 s, B on; at L_max + 1 cell",
             "meas thinned A fill " + juce::String (q.fillA) + ", B " + juce::String (q.liveB) + " >= m_B " + juce::String (q.mB)
                 + ", A " + juce::String (q.liveA) + " <= m_A " + juce::String (q.mA)
                 + "; unthinned B " + juce::String (u.liveB) + " >= " + juce::String (u.mB) + ", A " + juce::String (u.liveA)
                 + " <= " + juce::String (u.mA) + "+1 | margin B " + juce::String (std::min (q.liveB - q.mB, u.liveB - u.mB))
                 + " | " + ctrl (fired, "unthinned + overQuotaOff -> B " + juce::String (k.liveB) + " vs m_B " + juce::String (k.mB)));
    }

    struct Q3R { int before = 0, after = 0; };

    Q3R runQ3 (bool thinOff, bool d25Off)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "grain_limit", 3.0f }, { s1 ("density"), 200.0f }, { s1 ("size"), 2000.0f } }));
        s.hk().thinningOff = thinOff;
        s.hk().d25Off = d25Off;
        s.record = false;
        s.load (0, noiseBuf (2, (int) (6.0 * fs), 0.25f), fs);
        for (int k = 0; k < 8; ++k)
            s.noteOn (0, 48);
        const juce::int64 tc = thinOff ? (juce::int64) (0.5 * fs) : (juce::int64) (2.1 * fs);
        Q3R q;
        s.at (tc, [&q] (Session& ss) { q.before = ss.hk().liveNow; setP (*ss.p, "grain_limit", 0.0f); });
        const juce::int64 tCheck = thinOff ? tc + (juce::int64) (0.5 * fs)
                                           : ceilCell (tc + (juce::int64) (2.0 * fs) + (juce::int64) (0.005 * fs) + 32);
        s.run (tCheck, 512);
        s.run (32, 32);
        q.after = s.hk().liveNow;
        return q;
    }

    void rowQ3()
    {
        const auto a = runQ3 (false, false);   // thinned path: L_max + 5 ms + 1 cell
        const auto b = runQ3 (true, false);    // unthinned: D25 must actively converge within 0.5 s
        const auto k = runQ3 (true, true);     // control: no D25 → alive stays > 32
        const bool fired = k.after > 32;
        row (a.before > 32 && a.after <= 32 && b.before > 32 && b.after <= 32 && fired,
             "Q3 grain_limit 256 -> 32 live, 8 notes over-demand",
             "meas thinned " + juce::String (a.before) + " -> " + juce::String (a.after) + " @ L_max+5ms+1cell; unthinned+D25 "
                 + juce::String (b.before) + " -> " + juce::String (b.after) + " @ 0.5 s | thr <= 32 | margin "
                 + juce::String (32 - std::max (a.after, b.after))
                 + " | " + ctrl (fired, "thinningOff + d25Off -> " + juce::String (k.after) + " @ 0.5 s"));
    }

    struct Q4R { double perVoice = 0.0, neff = 0.0, law = 0.0; };

    Q4R runQ4 (int voices, int capIdx, float density, bool thinOff, ogi::WindowMorphBank& bank)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "grain_limit", (float) capIdx }, { s1 ("density"), density }, { s1 ("size"), 200.0f },
                                       { s1 ("position"), 50.0f } }));
        s.hk().thinningOff = thinOff;
        s.load (0, noiseBuf (2, (int) (3.0 * fs), 0.05f, 4), fs);
        for (int k = 0; k < voices; ++k)
            s.noteOn (97 * k, 48);   // staggered onsets: incoherent voices
        s.run ((juce::int64) (1.6 * fs), 512);
        Q4R q;
        const double total = rmsSt (s.L, s.R, (juce::int64) (0.5 * fs), (juce::int64) (1.5 * fs));
        q.perVoice = total / std::sqrt ((double) voices);
        const double lenBar = std::clamp ((double) rawOf (*s.p, s1 ("size")), 5.0, 2000.0) * 0.001 * fs;
        q.neff = ogi::effectiveCount (lenBar, s.hk().periodT[0]);
        q.law  = (double) ogi::GiRenderer::voiceLawGain (q.neff, bank);
        return q;
    }

    void rowQ4()
    {
        ogi::WindowMorphBank bank;
        const auto ref = runQ4 (1, 3, 100.0f, false, bank);   // uncapped reference, N = 20
        auto expectFor = [&ref] (const Q4R& q)
        {
            return ref.perVoice * (q.law * std::sqrt (q.neff)) / (ref.law * std::sqrt (ref.neff));
        };
        Tally t;
        juce::String d;
        double worst = 0.0;
        for (int v : { 1, 4, 16 })
        {
            const auto q = runQ4 (v, 0, 200.0f, false, bank);
            const double err = dB (q.perVoice / expectFor (q));
            worst = std::max (worst, std::abs (err));
            t.check (std::abs (err) <= 0.5, juce::String (v) + "v err " + f2 (err));
            d << "  " << v << "v Neff " << f2 (q.neff, 2) << " err " << f2 (err) << " dB";
        }
        const auto a = runQ4 (16, 0, 200.0f, false, bank);
        const auto b = runQ4 (16, 0, 200.0f, false, bank);
        const double oldOld = dB (b.perVoice / a.perVoice);
        const auto k = runQ4 (16, 0, 200.0f, true, bank);
        const double kErr = dB (k.perVoice / expectFor (k));
        const bool fired = std::abs (kErr) > 0.5;
        row (t.bad == 0 && sameD (oldOld, 0.0) && ref.neff > 1.0 && fired,
             "Q4 per-voice RMS vs output law under thinning (noise, cap 32)",
             "meas" + d + " | thr +-0.5 dB | margin " + f2 (0.5 - worst) + " dB | old-vs-old " + f2 (oldOld, 6)
                 + " dB (== 0) | " + ctrl (fired, "thinningOff 16 v -> " + f2 (kErr) + " dB off-law"));
    }

    //==========================================================================
    // T — MipTape vs MipRing / MipSnapshot (+ R2)
    struct MipFix
    {
        static constexpr int F = 48000;
        std::vector<float> L, R;
        Ring4 ring;
        Snap4 snap;

        MipFix() : L ((size_t) F), R ((size_t) F)
        {
            for (int i = 0; i < F; ++i)
            {
                L[(size_t) i] = hashNoise (i, 0, 0.8f);
                R[(size_t) i] = hashNoise (i, 1, 0.8f);
            }
            const int tail = Tape4::tailFrames();
            ring.prepare (F + tail + 4096);   // no wrap: absolute index == storage index
            for (int i = 0; i < F; ++i)
                ring.push (L[(size_t) i], R[(size_t) i]);
            for (int i = 0; i < tail; ++i)
                ring.push (0.0f, 0.0f);
            snap.prepare (F);
            snap.captureFrom (ring, tail, F);   // origin 0, settled to F
        }

        static void build (Tape4& t, const std::vector<float>& l, const std::vector<float>& r, juce::int64 seed)
        {
            t.prepare (F);
            juce::Random rr (seed);
            int pos = 0;
            while (pos < F)
            {
                const int n = std::min (1 + rr.nextInt (4096), F - pos);
                t.writeRun (pos, n, l.data() + pos, r.data() + pos);
                pos += n;
            }
            t.flushTail();
        }
    };

    struct LevelCmp { int compared = 0, diff = 0; int perLevel[5] {}; };

    LevelCmp cmpLevels (const Tape4& t, const Ring4& ring)
    {
        LevelCmp c;
        for (int l = 0; l <= 4; ++l)
            for (juce::int64 j = 0; (juce::int64) (1 << l) * j + Ring4::delayOf (l) + (1 << l) < MipFix::F; ++j)   // settled indices
                for (int ch = 0; ch < 2; ++ch)
                {
                    ++c.compared;
                    if (! sameF (ring.levelSample (l, ch, j), t.levelSample (l, ch, j)))
                    {
                        ++c.diff;
                        ++c.perLevel[l];
                    }
                }
        return c;
    }

    struct ReadCmp { int n = 0, diff = 0, clamped = 0; };

    ReadCmp cmpReads (const Tape4& t, const Snap4& snap, juce::Random& rr, int count, int levelLo, int levelHi,
                      double posLo, double posHi, bool checkClamp)
    {
        ReadCmp c;
        for (int i = 0; i < count; ++i)
        {
            const int level = levelLo + rr.nextInt (levelHi - levelLo + 1);
            const double pos = std::min (posLo + rr.nextDouble() * (posHi - posLo), (double) MipFix::F - 1.0e-6);
            const int set = ouaricon::grain::SincBank::setFor (1.0 + rr.nextDouble());
            float a = 0.0f, b = 0.0f, x = 0.0f, y = 0.0f;
            t.readStereo (level, pos, set, a, b);
            snap.readStereo (level, pos, set, x, y);
            ++c.n;
            if (! sameF (a, x) || ! sameF (b, y))
                ++c.diff;
            if (checkClamp)
            {
                const double bI = std::floor (pos);
                const auto lp = Ring4::levelPos (level, (std::int64_t) bI, pos - bI);
                const std::int64_t st = lp.index - ouaricon::grain::blr::kTapsBack;
                if (st < -(std::int64_t) Tape4::kPre || st + Tape4::kTaps > (std::int64_t) t.levelSize (level))
                    ++c.clamped;
            }
        }
        return c;
    }

    void rowsT (MipFix& fx)
    {
        Tape4 t;
        MipFix::build (t, fx.L, fx.R, 0x71000001);
        const auto c = cmpLevels (t, fx.ring);

        // INFO: the engine's 1-double-ulp halfband-tap hook (likely below float resolution).
        ogi::test::mipTapePerturbTap = 5;
        Tape4 th;
        MipFix::build (th, fx.L, fx.R, 0x71000001);
        ogi::test::mipTapePerturbTap = -1;
        const auto ch = cmpLevels (th, fx.ring);

        // Firing control: one level-0 input sample bumped by 1e-3 (the comparator sees every level).
        auto Lp = fx.L;
        Lp[(size_t) (MipFix::F / 2)] += 1.0e-3f;
        Tape4 tp;
        MipFix::build (tp, Lp, fx.R, 0x71000001);
        const auto cp = cmpLevels (tp, fx.ring);
        bool everyLevel = true;
        juce::String per;
        for (int l = 0; l <= 4; ++l)
        {
            everyLevel = everyLevel && cp.perLevel[l] > 0;
            per << " L" << l << ":" << cp.perLevel[l];
        }
        const bool lvl4Live = ! sameF (t.levelSample (4, 0, 1000), 0.0f) || ! sameF (t.levelSample (4, 1, 1000), 0.0f);
        row (c.diff == 0 && c.compared > 100000 && lvl4Live && everyLevel,
             "T1 MipTape writeRun (runs 1..4096) == MipRing<4>::push bitwise",
             "meas " + juce::String (c.diff) + " / " + juce::String (c.compared) + " settled samples differ | thr 0 | margin "
                 + juce::String (-c.diff) + " | " + ctrl (everyLevel, "input +1e-3 ->" + per)
                 + " | INFO 1-ulp tap hook -> " + juce::String (ch.diff) + " differ");

        juce::Random r1 ((juce::int64) 0x72000002);
        const auto rd = cmpReads (t, fx.snap, r1, 1000, 0, 4, 0.0, (double) MipFix::F, false);
        juce::Random r2 ((juce::int64) 0x72000003);
        const auto rp = cmpReads (tp, fx.snap, r2, 64, 0, 4, (double) (MipFix::F / 2) - 8.0, (double) (MipFix::F / 2) + 8.0, false);
        juce::Random r3 ((juce::int64) 0x72000002);
        const auto rh = cmpReads (th, fx.snap, r3, 1000, 0, 4, 0.0, (double) MipFix::F, false);
        const bool fired = rp.diff > 0;
        row (rd.diff == 0 && fired, "T2 MipTape readStereo == MipSnapshot (1 k random level/pos/set)",
             "meas " + juce::String (rd.diff) + " / " + juce::String (rd.n) + " differ | thr 0 | margin " + juce::String (-rd.diff)
                 + " | " + ctrl (fired, "input +1e-3 -> " + juce::String (rp.diff) + "/64 reads near F/2 differ")
                 + " | INFO 1-ulp tap hook -> " + juce::String (rh.diff) + " differ");
    }

    //==========================================================================
    // K — Kaiser resampler
    std::vector<float> resampleV (const std::vector<float>& in, double fi, double fo, const ogi::kaiser::Options& o = {})
    {
        std::vector<float> out;
        ogi::kaiser::resample (in.data(), (std::int64_t) in.size(), fi, fo, out, {}, o);
        return out;
    }

    juce::int64 interiorGuard (double fi, double fo, const ogi::kaiser::Options& o = {})
    {
        const int K = ogi::kaiser::tapsPerSide (fi, fo, o);
        return (juce::int64) std::ceil ((double) (K + 2) * fo / fi) + 2;
    }

    double dcErr (double fi, double fo, bool normalise)
    {
        std::vector<float> in ((size_t) fi, 0.5f);
        ogi::kaiser::Options o;
        o.normalise = normalise;
        const auto out = resampleV (in, fi, fo, o);
        const juce::int64 g = interiorGuard (fi, fo, o);
        double m = 0.0;
        for (juce::int64 j = g; j < (juce::int64) out.size() - g; ++j)
            m = std::max (m, std::abs ((double) out[(size_t) j] - 0.5));
        return m;
    }

    /** Gain (dB) of a 0.5-amplitude tone fin at fi, measured at fmeas in the fo output. */
    double toneGainDb (double fi, double fo, double fin, double fmeas, const ogi::kaiser::Options& o = {})
    {
        std::vector<float> in ((size_t) fi);
        for (size_t i = 0; i < in.size(); ++i)
            in[i] = (float) (0.5 * std::sin (2.0 * kPi * fin * (double) i / fi));
        const auto out = resampleV (in, fi, fo, o);
        const juce::int64 g = interiorGuard (fi, fo, o);
        const juce::int64 n = (juce::int64) out.size() - 2 * g;
        std::vector<float> ov (out);
        return dB (toneAmp (ov, g, n, fmeas, fo, true) / 0.5);
    }

    double aliasOf (double f, double fs) { return std::abs (f - std::round (f / fs) * fs); }

    void rowsK()
    {
        {
            const double up = dcErr (44100.0, 96000.0, true), dn = dcErr (96000.0, 44100.0, true);
            const double kup = dcErr (44100.0, 96000.0, false), kdn = dcErr (96000.0, 44100.0, false);
            const double worst = std::max (up, dn);
            const bool fired = std::max (kup, kdn) > 1.0e-6;
            row (worst <= 1.0e-6 && fired, "K1 resampler DC: constant 0.5, 44.1->96 k and 96->44.1 k (interior)",
                 "meas max|out-0.5| " + juce::String (up, 9) + " / " + juce::String (dn, 9) + " | thr 1e-6 | margin "
                     + juce::String (1.0e-6 - worst, 9)
                     + " | " + ctrl (fired, "unnormalised -> " + juce::String (kup, 9) + " / " + juce::String (kdn, 9)));
        }
        {
            const double edge = 0.9 * 44100.0 * 0.5;   // 0.9 · min / 2 = 19845 Hz
            Tally tp;
            double worstPass = 0.0;
            for (double f : { 100.0, 1000.0, 5000.0, 10000.0, 15000.0, 18000.0, edge })
                for (int dir = 0; dir < 2; ++dir)
                {
                    const double fi = dir == 0 ? 44100.0 : 96000.0, fo = dir == 0 ? 96000.0 : 44100.0;
                    const double g = toneGainDb (fi, fo, f, f);
                    worstPass = std::max (worstPass, std::abs (g));
                    tp.check (std::abs (g) <= 0.01, juce::String (dir == 0 ? "up " : "dn ") + f2 (f, 0) + " Hz " + f2 (g, 4) + " dB");
                }
            double worstStop = -400.0, worstStopK = -400.0;
            Tally ts;
            ogi::kaiser::Options b2;
            b2.beta = 2.0;
            for (double f : { 30000.0, 36000.0, 42000.0, 47000.0 })   // 96 -> 44.1: out-of-band input
            {
                const double a = aliasOf (f, 44100.0);
                const double g = toneGainDb (96000.0, 44100.0, f, a);
                worstStop = std::max (worstStop, g);
                worstStopK = std::max (worstStopK, toneGainDb (96000.0, 44100.0, f, a, b2));
                ts.check (g <= -90.0, "dn " + f2 (f, 0) + " -> " + f2 (a, 0) + " Hz " + f2 (g, 1) + " dB");
            }
            for (double f : { 1000.0, 5000.0, 10000.0, 15000.0 })      // 44.1 -> 96: images at 44.1 k − f
            {
                const double im = 44100.0 - f;
                const double g = toneGainDb (44100.0, 96000.0, f, im);
                worstStop = std::max (worstStop, g);
                worstStopK = std::max (worstStopK, toneGainDb (44100.0, 96000.0, f, im, b2));
                ts.check (g <= -90.0, "up image " + f2 (im, 0) + " Hz " + f2 (g, 1) + " dB");
            }
            const bool fired = worstStopK > -90.0;
            row (tp.bad == 0 && ts.bad == 0 && fired, "K2 resampler pass <= 0.01 dB to 0.9 min/2; stop >= 90 dB",
                 "meas pass worst " + f2 (worstPass, 4) + " dB, stop worst " + f2 (worstStop, 1) + " dB | thr 0.01 / -90 | margin "
                     + f2 (0.01 - worstPass, 4) + " / " + f2 (-90.0 - worstStop, 1) + " | pass " + tp.detail (14)
                     + " | stop " + ts.detail (8) + " | " + ctrl (fired, "beta 2 -> stop worst " + f2 (worstStopK, 1) + " dB"));
        }
        {
            std::vector<float> in (48000);
            for (size_t i = 0; i < in.size(); ++i)
                in[i] = hashNoise ((juce::int64) i, 0, 0.7f);
            const auto out = resampleV (in, 48000.0, 48000.0);
            const bool same = out.size() == in.size() && std::memcmp (out.data(), in.data(), sizeof (float) * in.size()) == 0;
            row (same, "K3 resampler equal rates 48 -> 48 k: bit-exact copy",
                 "meas " + juce::String (same ? "identical" : "DIFFERS") + " (" + juce::String ((int) out.size()) + " frames) | thr bitwise | ctrl --");
        }
    }

    //==========================================================================
    // L — loader trim / mono upmix
    void processSilent (Proc& p, int n)
    {
        juce::AudioBuffer<float> b (2, n);
        b.clear();
        juce::MidiBuffer m;
        p.processBlock (b, m);
    }

    void rowsL()
    {
        // L1 (a) 40 s @ 44.1 k into a 44.1 k session (sync load, real job body).
        bool okA = false, trimA = false;
        int framesA = 0, nativeA = 0;
        {
            auto p = make();
            p->prepareToPlay (44100.0, 512);
            const auto pcm = noiseBuf (1, 40 * 44100, 0.3f);
            juce::String e;
            okA = p->loadSlotSync (0, pcm, 44100.0, "long40", e);
            const auto si = p->getSlotInfo (0);
            trimA = si.trimmed;
            framesA = si.frames;
            nativeA = si.nativeFrames;
        }
        // L1 (b) 20 s @ 192 k (session 48 k): the 2.88 M native-frame cap binds at 15 s.
        bool okB = false, trimB = false;
        int framesB = 0;
        {
            ogi::SlotLoader::Pristine pr;
            juce::String e;
            const auto pcm = noiseBuf (1, 20 * 192000, 0.3f);
            okB = ogi::SlotLoader::pristineFromPcm (pcm, 192000.0, "hi192", 48000.0, pr, e);
            trimB = pr.trimmed;
            framesB = pr.pcm.getNumSamples();
        }
        const bool caps = ogi::SlotLoader::nativeFrameCap (44100.0, 44100.0) == 1323000
                       && ogi::SlotLoader::nativeFrameCap (192000.0, 192000.0) == 2880000
                       && ogi::SlotLoader::nativeFrameCap (192000.0, 48000.0) == 2880000;
        // Control: a 10 s WAV through the decode path is not trimmed.
        bool okC = false, trimC = true;
        int nativeC = 0;
        {
            auto p = make();
            p->prepareToPlay (44100.0, 512);
            const auto f = writeWav ("l1-10s.wav", noiseBuf (1, 10 * 44100, 0.3f, 2), 44100.0);
            juce::String e;
            okC = f.existsAsFile() && p->loadSlotSync (0, f, e);
            const auto si = p->getSlotInfo (0);
            trimC = si.trimmed;
            nativeC = si.nativeFrames;
        }
        const bool fired = okC && ! trimC && nativeC == 441000;
        row (okA && trimA && nativeA == 1323000 && framesA == 1323000 && okB && trimB && framesB == 2880000 && caps && fired,
             "L1 trim: 40 s @ 44.1 k -> 30 s; 20 s @ 192 k -> 15 s (2.88 M cap)",
             "meas 44.1k native " + juce::String (nativeA) + " tape " + juce::String (framesA) + (trimA ? " trimmed" : " NOT trimmed")
                 + "; 192k " + juce::String (framesB) + (trimB ? " trimmed" : " NOT trimmed") + "; caps " + (caps ? "ok" : "BAD")
                 + " | thr 1323000 / 2880000 | " + ctrl (fired, "10 s WAV -> " + juce::String (nativeC) + (trimC ? " trimmed" : " untrimmed")));

        // L2 mono → L == R at tape build (every level).
        auto lrDiff = [] (const juce::File& f, int& compared) -> int
        {
            auto p = make();
            p->prepareToPlay (48000.0, 512);
            juce::String e;
            compared = 0;
            if (! f.existsAsFile() || ! p->loadSlotSync (0, f, e))
                return -1;
            processSilent (*p, 64);   // the grid tick takes the tape
            const ogi::SlotTape* t = p->getEngine().getCurrentTape (0);
            if (t == nullptr)
                return -1;
            int d = 0;
            for (int l = 0; l <= 4; ++l)
                for (int j = 0; j < t->tape.levelSize (l); ++j)
                {
                    ++compared;
                    d += sameF (t->tape.levelSample (l, 0, j), t->tape.levelSample (l, 1, j)) ? 0 : 1;
                }
            return d;
        };
        int nMono = 0, nStereo = 0;
        const int dMono = lrDiff (writeWav ("l2-mono.wav", noiseBuf (1, 24000, 0.5f, 3), 48000.0), nMono);
        const int dStereo = lrDiff (writeWav ("l2-stereo.wav", noiseBuf (2, 24000, 0.5f, 3), 48000.0), nStereo);
        const bool fired2 = dStereo > 0;
        row (dMono == 0 && nMono > 24000 && fired2, "L2 mono WAV -> tape L == R bitwise on every level",
             "meas " + juce::String (dMono) + " / " + juce::String (nMono) + " differ | thr 0 | margin " + juce::String (-dMono)
                 + " | " + ctrl (fired2, "stereo WAV -> " + juce::String (dStereo) + " / " + juce::String (nStereo) + " differ"));
    }

    //==========================================================================
    // P — pitch
    double expectRate (Proc& p, int note, double tapeRate, double fs)
    {
        const float root = std::clamp (rawOf (p, s1 ("root_note")), 0.0f, 127.0f);
        const float fine = std::clamp (rawOf (p, s1 ("root_fine")), -100.0f, 100.0f);
        const float kt   = std::clamp (rawOf (p, s1 ("key_track")), 0.0f, 100.0f) / 100.0f;
        const float ps   = std::clamp (rawOf (p, s1 ("pitch")), -24.0f, 24.0f);
        const double key = (double) note - (double) root - (double) fine / 100.0;
        const double st  = std::clamp ((double) kt * key + (double) ps, -48.0, 48.0);
        return std::exp2 (st / 12.0) * tapeRate / fs;
    }

    void rowP1()
    {
        Session s (48000.0, with (base(), { { s1 ("root_fine"), -30.0f } }));
        s.load (0, noiseBuf (2, 48000, 0.2f), 48000.0);
        s.run (64, 32);                     // tape taken at 48 k
        s.p->prepareToPlay (96000.0, 512);  // fs 96 k; the tape stays 48 k (rebuild not pumped)
        const int notes[3] = { 48, 60, 36 };
        for (int n : notes)
            s.noteOn (s.pos, n);
        s.run (64, 32);
        const double tr = s.hk().tapeRate[0];
        double worstA = 0.0;
        double rates[3] {};
        for (int k = 0; k < 3; ++k)
        {
            rates[k] = s.hk().voiceRate[k];
            const double e = expectRate (*s.p, s.hk().voiceNote[k], tr, 96000.0);
            worstA = std::max (worstA, std::abs (rates[k] - e) / e);
        }
        const bool distinct = ! sameD (rates[0], rates[1]) && ! sameD (rates[1], rates[2]) && ! sameD (rates[0], rates[2]);

        setP (*s.p, s1 ("key_track"), 0.0f);
        setP (*s.p, s1 ("pitch"), 3.5f);
        s.run (64, 32);
        double worstB = 0.0;
        bool equalB = true;
        for (int k = 0; k < 3; ++k)
        {
            const double r = s.hk().voiceRate[k];
            const double e = expectRate (*s.p, s.hk().voiceNote[k], tr, 96000.0);
            worstB = std::max (worstB, std::abs (r - e) / e);
            equalB = equalB && sameD (r, s.hk().voiceRate[0]);
        }
        const double worst = std::max (worstA, worstB);
        row (sameD (tr, 48000.0) && worst <= 1.0e-12 && equalB && distinct,
             "P1 latched rate == exp2(st/12)·tapeRate/fs (48 k tape, 96 k fs)",
             "meas rel err kt100 " + juce::String (worstA, 15) + ", kt0 " + juce::String (worstB, 15) + " (kt0 equal "
                 + (equalB ? "yes" : "NO") + ") | thr 1e-12 | margin " + juce::String (1.0e-12 - worst, 15)
                 + " | " + ctrl (distinct, "keyTrack 100 -> " + juce::String (rates[0], 6) + " / " + juce::String (rates[1], 6)
                                           + " / " + juce::String (rates[2], 6)));
    }

    double runP2 (int note, float fineCt)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { s1 ("size"), 2000.0f }, { s1 ("density"), 0.5f }, { s1 ("position"), 25.0f },
                                       { s1 ("root_fine"), fineCt } }));
        s.load (0, sineBuf (1, (int) (6.0 * fs), 440.0, fs, 0.5), fs);
        s.noteOn (0, note);
        s.run ((juce::int64) (2.05 * fs), 512);
        return zcFreq (s.L, (juce::int64) (0.5 * fs), (juce::int64) (1.5 * fs), fs);   // central 50 % of the 2 s grain
    }

    void rowP2()
    {
        const double f0 = runP2 (48, 0.0f), f12 = runP2 (60, 0.0f), fk = runP2 (48, 5.0f);
        const double c0 = 1200.0 * std::log2 (f0 / 440.0);
        const double c12 = 1200.0 * std::log2 (f12 / 880.0);
        const double ck = 1200.0 * std::log2 (fk / 440.0);
        const double worst = std::max (std::abs (c0), std::abs (c12));
        const bool fired = std::abs (ck) >= 4.0;
        row (f0 > 0.0 && f12 > 0.0 && worst <= 1.0 && fired, "P2 FUNC-03 audio pitch, one 2000 ms grain, root and root+12",
             "meas " + f2 (c0, 4) + " / " + f2 (c12, 4) + " ct | thr +-1 ct | margin " + f2 (1.0 - worst, 4)
                 + " | " + ctrl (fired, "fine +5 ct -> " + f2 (ck, 3) + " ct (|.| >= 4)"));
    }

    struct P3R { double refAmp = 0.0, testAmp = 0.0; double db() const { return dB (testAmp / std::max (1.0e-300, refAmp)); } };

    P3R runP3 (double srcHz, int semis, double measHz)
    {
        constexpr double fs = 96000.0;
        Session s (fs, with (base(), { { s1 ("size"), 200.0f }, { s1 ("density"), 0.5f }, { s1 ("position"), 10.0f } }));
        s.load (0, sineBuf (1, 4 * 44100, srcHz, 44100.0, 0.5), 44100.0);   // resampled to the 96 k tape
        const juce::int64 len = (juce::int64) (0.2 * fs);
        s.noteOn (0, 48);                          // reference grain at +0
        s.noteOn ((juce::int64) (0.5 * fs), 48 + semis);   // test grain
        s.run ((juce::int64) (0.5 * fs) + len + 960, 512);
        P3R r;
        r.refAmp  = toneAmp (s.L, 0, len, srcHz, fs, false);   // the grain's own Hann is the window
        r.testAmp = toneAmp (s.L, (juce::int64) (0.5 * fs), len, measHz, fs, false);
        return r;
    }

    void rowP3()
    {
        const auto a48 = runP3 (4000.0, 48, 32000.0);
        const auto a24 = runP3 (15000.0, 24, 36000.0);
        const auto pos = runP3 (2500.0, 48, 40000.0);
        const double worst = std::max (a48.db(), a24.db());
        const bool fired = pos.db() >= -20.0;
        row (worst <= -60.0 && fired, "P3 DSP-07 96 k session, 44.1 k source: aliases <= -60 dB",
             "meas +48 st 4 k -> 32 k bin " + f2 (a48.db(), 1) + " dB, +24 st 15 k -> 36 k bin " + f2 (a24.db(), 1)
                 + " dB | thr -60 dB | margin " + f2 (-60.0 - worst, 1)
                 + " | " + ctrl (fired, "2.5 k @ +48 -> 40 k present " + f2 (pos.db(), 1) + " dB (>= -20)"));
    }

    void rowP4 (MipFix& fx)
    {
        Session s (48000.0, with (base(), { { s1 ("loop_start"), 95.8333f }, { s1 ("loop_end"), 100.0f },
                                            { s1 ("size"), 20.0f }, { s1 ("density"), 50.0f } }));
        s.load (0, noiseBuf (2, 96000, 0.3f, 5), 48000.0);
        s.noteOn (0, 108);   // root + 60 → +48 clamp
        s.run (9600, 32);
        const int lvl = s.hk().voiceLevel[0];
        const double rate = s.hk().voiceRate[0];
        const auto log = s.hk().logInOrder();
        const int F = s.p->getSlotInfo (0).frames;
        bool allL4 = ! log.empty(), nearEnd = ! log.empty();
        for (const auto& e : log)
        {
            allL4 = allL4 && e.level == 4 && sameD (e.rate, 16.0);
            nearEnd = nearEnd && e.startPos >= (double) F - 4100.0;
        }
        bool finite = true;
        float peak = 0.0f;
        for (size_t i = 0; i < s.L.size(); ++i)
        {
            finite = finite && std::isfinite (s.L[i]) && std::isfinite (s.R[i]);
            peak = std::max (peak, std::max (std::abs (s.L[i]), std::abs (s.R[i])));
        }
        // R2: synthetic tape, level-4 reads in the last 1 k frames == MipSnapshot, start never clamped.
        Tape4 t;
        MipFix::build (t, fx.L, fx.R, 0x74000004);
        juce::Random rr ((juce::int64) 0x74000005);
        const auto r2 = cmpReads (t, fx.snap, rr, 1000, 4, 4, (double) MipFix::F - 1000.0, (double) MipFix::F, true);
        row (lvl == 4 && std::abs (rate - 16.0) <= 1.0e-12 && allL4 && nearEnd && finite && peak > 1.0e-4f && peak <= 2.0f
                 && r2.diff == 0 && r2.clamped == 0,
             "P4 +-48 st clamp (root+60): level 4, rate 16; R2 last-1k-frame reads",
             "meas level " + juce::String (lvl) + ", rate " + juce::String (rate, 12) + ", " + juce::String ((int) log.size())
                 + " grains all L4 " + (allL4 ? "yes" : "NO") + ", in last 4.1 k frames " + (nearEnd ? "yes" : "NO")
                 + ", peak " + f2 (peak, 4) + " | R2 " + juce::String (r2.diff) + " differ, " + juce::String (r2.clamped)
                 + " clamped / 1000 | thr level 4, rate 16 +-1e-12, 0, 0 | ctrl --");
    }

    //==========================================================================
    // S — scan / loop region
    struct S1R { Log log; double lo = 0.0, hi = 0.0; double reach = 0.0; };

    S1R runS1 (int mode, float scanPct, float loopEndPct, juce::int64 t2, int note2, juce::int64 len)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { s1 ("scan_mode"), (float) mode }, { s1 ("scan_rate"), scanPct }, { s1 ("size"), 10.0f },
                                       { s1 ("density"), 100.0f }, { s1 ("loop_end"), loopEndPct } }));
        s.load (0, noiseBuf (2, (int) (4.0 * fs), 0.2f, 6), fs);
        s.noteOn (0, 48);
        if (t2 > 0)
            s.noteOn (t2, note2);
        s.run (len, 512);
        S1R r;
        r.log = s.hk().logInOrder();
        r.lo = s.hk().regionLo;
        r.hi = s.hk().regionHi;
        r.reach = std::clamp ((double) rawOf (*s.p, s1 ("size")), 5.0, 2000.0) * 0.001 * fs;   // rate 1 at the root
        return r;
    }

    void rowS1()
    {
        const juce::int64 t2 = 33600;
        const auto rt = runS1 (1, 100.0f, 100.0f, t2, 50, 72000);
        const auto r0 = runS1 (1, 0.0f, 100.0f, 0, 0, 24000);
        const auto fr = runS1 (0, 100.0f, 100.0f, t2, 50, 72000);
        const auto pp = runS1 (2, 100.0f, 25.0f, 0, 0, 76800);

        double worstRt = 0.0, worstFr = 0.0, worstPp = 0.0;
        int nRt1 = 0, nRt2 = 0, nFr2 = 0, nPp = 0;
        double rt2First = -1.0, fr2First = -1.0;
        for (const auto& e : rt.log)
        {
            const double expect = e.serial == 1 ? (double) e.onsetSample : (double) (e.onsetSample - t2);
            worstRt = std::max (worstRt, std::abs (e.startPos - expect));
            (e.serial == 1 ? nRt1 : nRt2)++;
            if (e.serial == 2 && rt2First < 0.0) rt2First = e.startPos;
        }
        bool hold = ! r0.log.empty();
        for (const auto& e : r0.log)
            hold = hold && sameD (e.startPos, 0.0);
        for (const auto& e : fr.log)
        {
            worstFr = std::max (worstFr, std::abs (e.startPos - (double) e.onsetSample));
            if (e.serial == 2) { ++nFr2; if (fr2First < 0.0) fr2First = e.startPos; }
        }
        const double W = pp.hi - pp.lo, bnd = W - pp.reach;
        bool reflected = false;
        double prev = -1.0;
        for (const auto& e : pp.log)
        {
            const double u = (double) e.onsetSample;
            const double m = u - 2.0 * W * std::floor (u / (2.0 * W));
            const double c = m <= W ? m : 2.0 * W - m;
            const double expect = pp.lo + (c <= bnd ? c : 2.0 * bnd - c);
            worstPp = std::max (worstPp, std::abs (e.startPos - expect));
            if (prev >= 0.0 && e.startPos < prev) reflected = true;
            prev = e.startPos;
            ++nPp;
        }
        const double worst = std::max (worstRt, std::max (worstFr, worstPp));
        const bool fired = rt2First >= 0.0 && fr2First >= 0.0 && ! sameD (rt2First, fr2First);
        row (worst <= 1.0e-6 && nRt1 > 50 && nRt2 > 20 && nFr2 > 20 && nPp > 50 && hold && reflected
                 && sameD (rt.lo, 0.0) && std::abs (rt2First) <= 1.0e-6 && std::abs (fr2First - (double) t2) <= 1.0e-6 && fired,
             "S1 FUNC-04 scan: 100 % = 1 s/s, 0 % holds, Retrigger / Free-run / Ping-pong",
             "meas |startPos - expect| max " + juce::String (worst, 9) + " frames (rt " + juce::String (worstRt, 9) + ", fr "
                 + juce::String (worstFr, 9) + ", pp " + juce::String (worstPp, 9) + "), hold " + (hold ? "yes" : "NO")
                 + ", pp reflects " + (reflected ? "yes" : "NO") + " | thr 1e-6 | margin " + juce::String (1.0e-6 - worst, 9)
                 + " | " + ctrl (fired, "2nd note start: Retrigger " + f2 (rt2First, 3) + " vs Free-run " + f2 (fr2First, 3)));
    }

    void rowS2()
    {
        Session s (48000.0, base());
        s.load (0, noiseBuf (2, 96000, 0.2f), 48000.0);
        const double F = (double) s.p->getSlotInfo (0).frames;

        auto regionAfter = [&s] (float a, float b, std::vector<float>& before) -> std::pair<double, double>
        {
            setP (*s.p, s1 ("loop_start"), a);
            setP (*s.p, s1 ("loop_end"), b);
            before = allValues (*s.p);
            s.run (64, 32);
            return { s.hk().regionLo, s.hk().regionHi };
        };
        Tally t;
        std::vector<float> v1, v2, v3;
        const auto rA = regionAfter (70.0f, 30.0f, v1);
        const bool wA = bitwiseEqual (v1, allValues (*s.p));
        const double lsA = (double) (std::clamp (rawOf (*s.p, s1 ("loop_start")), 0.0f, 100.0f) / 100.0f);
        const double leA = (double) (std::clamp (rawOf (*s.p, s1 ("loop_end")), 0.0f, 100.0f) / 100.0f);
        t.check (std::abs (rA.first - leA * F) <= 1.0e-9 && std::abs (rA.second - lsA * F) <= 1.0e-9,
                 "sorted " + f2 (rA.first, 2) + ".." + f2 (rA.second, 2));
        const auto rB = regionAfter (50.0f, 50.01f, v2);
        const bool wB = bitwiseEqual (v2, allValues (*s.p));
        t.check (std::abs ((rB.second - rB.first) - 480.0) <= 1.0e-6, "min width " + f2 (rB.second - rB.first, 6));
        const auto rC = regionAfter (99.99f, 100.0f, v3);
        const bool wC = bitwiseEqual (v3, allValues (*s.p));
        t.check (sameD (rC.second, F) && std::abs (rC.first - (F - 480.0)) <= 1.0e-6, "end clamp " + f2 (rC.first, 3) + ".." + f2 (rC.second, 3));
        const bool noWrites = wA && wB && wC;
        row (t.bad == 0 && noWrites, "S2 loop region: sorted, 10 ms minimum, end clamp, no param writes",
             "meas [" + f2 (rA.first, 1) + ", " + f2 (rA.second, 1) + "), width(10ms) " + f2 (rB.second - rB.first, 6) + ", end ["
                 + f2 (rC.first, 1) + ", " + f2 (rC.second, 1) + "), params unchanged " + (noWrites ? "yes" : "NO")
                 + " | thr exact / 480 frames | " + t.detail (3) + " | ctrl --");
    }

    //==========================================================================
    // V — voices
    void rowV1()
    {
        Session s (48000.0, with (base(), { { s1 ("density"), 50.0f } }));
        s.load (0, noiseBuf (2, 96000, 0.2f), 48000.0);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.run (9600, 512);
        auto& h = s.hk();
        const int counted = countState (h, true);
        std::map<std::uint64_t, double> rateOf;
        std::set<double> rates;
        double worst = 0.0;
        for (int k = 0; k < ogi::kVoiceSlots; ++k)
            if (h.voiceState[k] == 1)
            {
                const double e = std::exp2 ((double) (h.voiceNote[k] - 48) / 12.0);
                worst = std::max (worst, std::abs (h.voiceRate[k] - e) / e);
                rateOf[h.voiceSerial[k]] = h.voiceRate[k];
                rates.insert (h.voiceRate[k]);
            }
        std::map<std::uint64_t, int> grains;
        bool logOk = true;
        for (const auto& e : h.logInOrder())
        {
            ++grains[e.serial];
            logOk = logOk && rateOf.count (e.serial) == 1 && sameD (e.rate, rateOf[e.serial]);
        }
        bool each = grains.size() == 4;
        for (const auto& g : grains)
            each = each && g.second >= 2;
        row (counted == 4 && rates.size() == 4 && worst <= 1.0e-12 && logOk && each,
             "V1 FUNC-01 4-note chord = 4 clouds at 4 rates",
             "meas " + juce::String (counted) + " voices, " + juce::String ((int) rates.size()) + " distinct rates, rel err "
                 + juce::String (worst, 15) + ", log rates match " + (logOk ? "yes" : "NO") + " | thr 4 / 1e-12 | ctrl --");
    }

    struct StealRun
    {
        std::vector<float> L, R;
        Log log;
        std::uint64_t steals = 0, stolenCount = 0;
        std::uint64_t stolen[2] {};
    };

    StealRun runSteal (int poly, bool rampZero, int slotOffset, std::uint64_t seed, float sizeRand)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "polyphony", (float) poly }, { s1 ("density"), 50.0f }, { s1 ("size"), 100.0f },
                                       { s1 ("size_rand"), sizeRand }, { "amp_attack", 20.0f }, { "amp_release", 300.0f },
                                       { s1 ("position"), 20.0f } }), seed);
        s.hk().stealRampZero = rampZero;
        if (slotOffset > 0)
            s.hk().setSlotOffset (slotOffset);
        s.load (0, sineBuf (1, (int) (4.0 * fs), 1000.0, fs, 0.3), fs);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.noteOn (24000, 64);   // 5th distinct pitch: steals serial 2 (52; 48 low / 60 top protected)
        s.noteOn (36000, 67);   // 6th: steals serial 3 (55; 48 low / 64 top protected)
        s.run (48000, 512);
        StealRun r;
        r.L = s.L;
        r.R = s.R;
        r.log = s.hk().logInOrder();
        r.steals = s.hk().voiceSteals;
        r.stolenCount = s.hk().stolenCount;
        r.stolen[0] = s.hk().stolenSerials[0];
        r.stolen[1] = s.hk().stolenSerials[1];
        return r;
    }

    double stealClick (const StealRun& a, const StealRun& ref)
    {
        double worst = 0.0;
        for (juce::int64 ts : { (juce::int64) 24000, (juce::int64) 36000 })
        {
            const double pa = d2Peak (a.L, a.R, ts - 32, ts + 240 + 64);
            const double pb = d2Peak (ref.L, ref.R, ts - 32, ts + 240 + 64);
            worst = std::max (worst, pa / std::max (1.0e-12, pb));
        }
        return worst;
    }

    void rowsV23()
    {
        const auto A = runSteal (4, false, 0, kSeed, 0.0f);
        const auto B = runSteal (16, false, 0, kSeed, 0.0f);   // no-steal baseline
        const auto C = runSteal (4, true, 0, kSeed, 0.0f);     // 0 ms steal ramp
        const double ratio = stealClick (A, B), ratioC = stealClick (C, B);
        const bool overflow = A.steals == 2 && A.stolenCount == 2 && A.stolen[0] == 2 && A.stolen[1] == 3 && B.steals == 0;
        const bool fired = ratioC > kClickRatio;
        row (overflow && ratio <= kClickRatio && fired, "V2 steal click: poly 4, 6 distinct pitches, 1 kHz tonal source",
             "meas steals " + i64 (A.steals) + " (serials " + i64 (A.stolen[0]) + ", " + i64 (A.stolen[1]) + "; baseline "
                 + i64 (B.steals) + "), d2 peak ratio vs no-steal " + f2 (ratio, 3) + " | thr <= " + f2 (kClickRatio, 1)
                 + " | margin " + f2 (kClickRatio - ratio, 3) + " | " + ctrl (fired, "0 ms ramp -> ratio " + f2 (ratioC, 2)));

        auto cmpLogs = [] (const Log& a, const Log& b, int& voiceDiff, bool& sameKeys) -> int
        {
            std::map<std::pair<std::uint64_t, std::uint64_t>, ogi::GrainLogEntry> ma, mb;
            for (const auto& e : a) ma[{ e.serial, e.onsetIndex }] = e;
            for (const auto& e : b) mb[{ e.serial, e.onsetIndex }] = e;
            sameKeys = ma.size() == mb.size();
            voiceDiff = 0;
            int diff = 0;
            for (const auto& kv : ma)
            {
                const auto it = mb.find (kv.first);
                if (it == mb.end()) { sameKeys = false; ++diff; continue; }
                const auto& e = kv.second;
                const auto& f = it->second;
                const bool same = e.onsetSample == f.onsetSample && sameD (e.frac, f.frac) && sameD (e.len, f.len)
                               && sameD (e.startPos, f.startPos) && sameD (e.rate, f.rate) && e.level == f.level;
                diff += same ? 0 : 1;
                voiceDiff += e.voice != f.voice ? 1 : 0;
            }
            return diff;
        };
        const auto a = runSteal (4, false, 0, kSeed, 50.0f);
        const auto b = runSteal (4, false, 7, kSeed, 50.0f);       // forced different slot assignment
        const auto c = runSteal (4, false, 0, kSeed + 1, 50.0f);   // different seed
        int vd = 0, vdC = 0;
        bool keys = false, keysC = false;
        const int d = cmpLogs (a.log, b.log, vd, keys);
        const int dC = cmpLogs (a.log, c.log, vdC, keysC);
        const bool fired3 = dC > 0;
        row (d == 0 && keys && vd > 0 && a.steals == 2 && b.steals == 2 && (int) a.log.size() > 50 && fired3,
             "V3 steal invariance: slot permutation -> per-serial grain logs identical",
             "meas " + juce::String (d) + " / " + juce::String ((int) a.log.size()) + " grains differ ("
                 + juce::String (vd) + " on another voice slot, steals " + i64 (a.steals) + "/" + i64 (b.steals)
                 + ") | thr 0 | margin " + juce::String (-d) + " | " + ctrl (fired3, "seed+1 -> " + juce::String (dC) + " differ"));
    }

    struct V4R { std::uint64_t forced = 0; int sounding = -1, counted = -1; double ratio = 0.0; };

    V4R runV4 (bool rampZero)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "polyphony", 12.0f }, { s1 ("density"), 50.0f }, { s1 ("size"), 100.0f },
                                       { "amp_attack", 20.0f }, { "amp_release", 300.0f }, { s1 ("position"), 20.0f } }));
        s.hk().stealRampZero = rampZero;
        s.load (0, sineBuf (1, (int) (4.0 * fs), 1000.0, fs, 0.3), fs);
        for (int n = 48; n < 60; ++n)
            s.noteOn (0, n);
        const juce::int64 tp = 24000;
        s.at (tp, [] (Session& ss) { setP (*ss.p, "polyphony", 4.0f); });
        s.run (tp, 512);
        s.run (320, 32);   // 5 ms ramp + 2 cells
        V4R r;
        r.sounding = countState (s.hk(), false);
        r.counted = countState (s.hk(), true);
        r.forced = s.hk().polyForceReleases;
        s.run (9280, 512);
        r.ratio = d2Peak (s.L, s.R, tp - 32, tp + 240 + 64) / std::max (1.0e-12, d2Peak (s.L, s.R, tp - 9600, tp - 480));
        return r;
    }

    void rowV4()
    {
        const auto r = runV4 (false);
        const auto k = runV4 (true);
        const bool infoFired = k.ratio > kClickRatio;
        row (r.forced == 8 && r.sounding == 4 && r.counted == 4 && r.ratio <= kClickRatio,
             "V4 polyphony 12 -> 4 live: 8 released over 5 ms, no click",
             "meas forced " + i64 (r.forced) + ", sounding " + juce::String (r.sounding) + " / counted " + juce::String (r.counted)
                 + " after 5 ms + 2 cells, d2 ratio vs steady " + f2 (r.ratio, 3) + " | thr 8 / 4 / <= " + f2 (kClickRatio, 1)
                 + " | margin " + f2 (kClickRatio - r.ratio, 3) + " | ctrl -- (INFO 0 ms ramp ratio " + f2 (k.ratio, 2)
                 + (infoFired ? ", fires)" : ", does NOT fire)"));
    }

    //==========================================================================
    // M — MIDI ties
    void rowM1()
    {
        constexpr double fs = 48000.0;
        Tally t;
        juce::String d;
        // (a) zero-length note: on(60) then off(60) at the same offset.
        {
            Session s (fs, with (base(), { { s1 ("density"), 50.0f }, { s1 ("size"), 50.0f } }));
            s.load (0, noiseBuf (2, 48000, 0.2f), fs);
            s.addMidi (100, juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100));
            s.addMidi (100, juce::MidiMessage::noteOff (1, 60));
            s.noteOn (1000, 62);
            s.noteOff (2000, 62);
            s.run (14400, 512);
            bool second = false;
            for (const auto& e : s.hk().logInOrder())
                second = second || (e.serial == 2 && e.onsetSample == 1000);
            const int stuck = countState (s.hk(), false);
            t.check (second && stuck == 0 && s.late == 0, "zero-length: serial 2 at 1000 " + juce::String (second ? "yes" : "NO")
                                                          + ", stuck " + juce::String (stuck));
            d << "zero-length stuck " << stuck;
        }
        // (b) off(60) then on(60) at one offset while 60 sounds → retrigger.
        {
            Session s (fs, with (base(), { { s1 ("density"), 50.0f }, { s1 ("size"), 50.0f } }));
            s.load (0, noiseBuf (2, 48000, 0.2f), fs);
            s.noteOn (0, 60);
            s.addMidi (4800, juce::MidiMessage::noteOff (1, 60));
            s.addMidi (4800, juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100));
            s.noteOff (9600, 60);
            int retrig = 0;
            s.at (7200, [&retrig] (Session& ss)
            {
                for (int k = 0; k < ogi::kVoiceSlots; ++k)
                    retrig += (ss.hk().voiceSerial[k] == 2 && ss.hk().voiceState[k] == 1 && ss.hk().voiceNote[k] == 60) ? 1 : 0;
            });
            s.run (24000, 512);
            bool at4800 = false;
            for (const auto& e : s.hk().logInOrder())
                at4800 = at4800 || (e.serial == 2 && e.onsetSample == 4800);
            const int stuck = countState (s.hk(), false);
            t.check (retrig == 1 && at4800 && stuck == 0, "off->on retrigger " + juce::String (retrig) + ", stuck " + juce::String (stuck));
            d << ", retrigger " << retrig << " stuck " << stuck;
        }
        // (c) 600 events (300 on/off pairs) inside one 4096 block.
        {
            Session s (fs, with (base(), { { s1 ("density"), 50.0f }, { s1 ("size"), 20.0f }, { "amp_release", 20.0f } }));
            s.load (0, noiseBuf (2, 48000, 0.2f), fs);
            for (int i = 0; i < 300; ++i)
            {
                s.noteOn (12 * i, 30 + (i % 60));
                s.noteOff (12 * i + 6, 30 + (i % 60));
            }
            s.run (4096, 4096);
            s.run (24000, 512);
            std::set<std::uint64_t> serials;
            for (const auto& e : s.hk().logInOrder())
                serials.insert (e.serial);
            int applied = 0;
            for (std::uint64_t k = 1; k <= 300; ++k)
                applied += serials.count (k) != 0 ? 1 : 0;
            const int stuck = countState (s.hk(), false);
            t.check (applied == 300 && stuck == 0 && s.late == 0, "600 events: " + juce::String (applied) + "/300 started, stuck " + juce::String (stuck));
            d << ", 600 events " << applied << "/300 started, stuck " << stuck;
        }
        row (t.bad == 0, "M1 MIDI: same-offset on->off, off->on, 600 events in one block",
             "meas " + d + " | thr 0 stuck, all applied | " + t.detail (3) + " | ctrl --");
    }

    //==========================================================================
    // E — amp envelope
    struct EnvSet { float a, d, s, r; };

    struct EnvRun
    {
        double na = 0.0, nd = 0.0, nr = 0.0, sus = 1.0;
        juce::int64 eA = 0, eD = 0, eEnd = 0, hold = 0;
        juce::int64 attackEnd = -1, decayEnd = -1, endAt = -1;
        double clickA = 0.0, clickR = 0.0;
    };

    EnvRun runEnv (const EnvSet& es)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "amp_attack", es.a }, { "amp_decay", es.d }, { "amp_sustain", es.s }, { "amp_release", es.r },
                                       { s1 ("density"), 100.0f }, { s1 ("size"), 50.0f }, { s1 ("position"), 20.0f } }));
        s.load (0, sineBuf (1, (int) (6.0 * fs), 1000.0, fs, 0.2), fs);
        EnvRun r;
        auto samples = [&s] (const char* id) { return std::max ((double) rawOf (*s.p, id), 2.0) * 0.001 * 48000.0; };
        r.na = samples ("amp_attack");
        r.nd = samples ("amp_decay");
        r.nr = samples ("amp_release");
        r.sus = std::clamp ((double) (std::clamp (rawOf (*s.p, "amp_sustain"), 0.0f, 100.0f) / 100.0f), 0.0, 1.0);
        r.eA = (juce::int64) std::ceil (r.na) - 1;
        r.eD = r.sus < 1.0 ? r.eA + (juce::int64) std::ceil (r.nd) : r.eA;
        r.hold = ceilCell (r.eD + (juce::int64) (0.1 * fs));
        const double nRel = r.nr * std::log (std::max (r.sus, 1.0e-9) / ogi::kEnvEndLevel) / std::log (1000.0);
        r.eEnd = r.hold + (juce::int64) std::ceil (nRel) - 1;
        s.noteOn (0, 48, 127);
        s.noteOff (r.hold, 48);
        s.afterBlock = [&r] (Session& ss)
        {
            const auto& v = ss.eng().getVoice (0);
            const auto st = v.env.getStage();
            if (r.attackEnd < 0 && st != ogi::AmpEnv::Stage::attack) r.attackEnd = ss.pos;
            if (r.decayEnd < 0 && st == ogi::AmpEnv::Stage::sustain) r.decayEnd = ss.pos;
            if (r.endAt < 0 && ss.pos > r.hold && v.isFree()) r.endAt = ss.pos;
        };
        s.run (r.eEnd + (juce::int64) (0.05 * fs), 32);
        const double steady = std::max (1.0e-12, d2Peak (s.L, s.R, r.hold - (juce::int64) (0.05 * fs), r.hold - 64));
        // The attack peaks at level 1.0 while `steady` is measured at the sustain
        // level: compare at equal level (a 1 kHz sine's d2 scales with amplitude).
        r.clickA = d2Peak (s.L, s.R, 0, r.eA + 64) / (steady / std::max (r.sus, 1.0e-3));
        r.clickR = d2Peak (s.L, s.R, r.hold - 32, r.hold + (juce::int64) (0.02 * fs)) / steady;
        return r;
    }

    double runVel (int velByte, float velToAmp)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "amp_attack", 10.0f }, { "amp_decay", 50.0f }, { "amp_sustain", 50.0f }, { "amp_release", 100.0f },
                                       { "vel_to_amp", velToAmp }, { s1 ("density"), 100.0f }, { s1 ("size"), 50.0f },
                                       { s1 ("position"), 20.0f } }));
        s.load (0, sineBuf (1, (int) (2.0 * fs), 1000.0, fs, 0.1), fs);
        s.noteOn (0, 48, velByte);
        s.run ((juce::int64) (0.4 * fs), 512);
        return rmsSt (s.L, s.R, (juce::int64) (0.2 * fs), (juce::int64) (0.35 * fs));
    }

    void rowE1()
    {
        const EnvSet sets[4] = { { 0.0f, 0.0f, 100.0f, 0.0f }, { 10.0f, 50.0f, 50.0f, 100.0f },
                                 { 100.0f, 200.0f, 25.0f, 300.0f }, { 300.0f, 500.0f, 70.0f, 500.0f } };
        auto inCell = [] (juce::int64 expected, juce::int64 blockEnd) { return blockEnd >= 0 && expected >= blockEnd - 33 && expected <= blockEnd; };
        Tally tt, tc;
        juce::String d;
        double worstClick = 0.0;
        for (int i = 0; i < 4; ++i)
        {
            const auto r = runEnv (sets[i]);
            const bool okA = inCell (r.eA, r.attackEnd);
            const bool okD = r.sus >= 1.0 || inCell (r.eD, r.decayEnd);
            const bool okR = inCell (r.eEnd, r.endAt);
            tt.check (okA && okD && okR, "set " + juce::String (i) + " A " + juce::String (r.eA) + "@" + juce::String (r.attackEnd)
                                         + " D " + juce::String (r.eD) + "@" + juce::String (r.decayEnd)
                                         + " end " + juce::String (r.eEnd) + "@" + juce::String (r.endAt));
            if (i > 0)   // set 0 = the 2 ms floors (a 2 ms exponential release is a deliberate fast edge)
            {
                worstClick = std::max (worstClick, std::max (r.clickA, r.clickR));
                tc.check (r.clickA <= kClickRatio && r.clickR <= kClickRatio, "set " + juce::String (i) + " click " + f2 (r.clickA, 2) + "/" + f2 (r.clickR, 2));
            }
            d << " s" << i << " A" << (r.attackEnd - 1 - r.eA) << " D" << (r.sus >= 1.0 ? 0 : r.decayEnd - 1 - r.eD) << " R" << (r.endAt - 1 - r.eEnd);
        }
        // Velocity law: sustain RMS ratio vs g_v = (1 − d) + d·v².
        Tally tv;
        double worstV = 0.0;
        const double ref = runVel (127, 0.0f);
        for (int vb : { 127, 32 })
            for (float dd : { 0.0f, 50.0f, 100.0f })
            {
                const double rms = runVel (vb, dd);
                const double v = (double) vb / 127.0, dv = (double) dd / 100.0;
                const double gv = (1.0 - dv) + dv * v * v;
                const double err = dB (rms / ref) - dB (gv);
                worstV = std::max (worstV, std::abs (err));
                tv.check (std::abs (err) <= 0.05, "vel " + juce::String (vb) + " d " + f2 (dd, 0) + " err " + f2 (err, 4));
            }
        row (tt.bad == 0 && tc.bad == 0 && tv.bad == 0, "E1 DSP-02 ADSR: stage times, vel_to_amp law, click-free",
             "meas offsets (block end - 1 - expected, samples)" + d + "; vel worst " + f2 (worstV, 4) + " dB; click worst "
                 + f2 (worstClick, 3) + " | thr one cell (32) / +-0.05 dB / <= " + f2 (kClickRatio, 1) + " | margin vel "
                 + f2 (0.05 - worstV, 4) + " | times " + tt.detail (4) + " | vel " + tv.detail (6) + " | click " + tc.detail (3) + " | ctrl --");
    }

    //==========================================================================
    // H — slot handoff
    bool waitIdle (Proc& p, int timeoutMs)
    {
        const auto t0 = juce::Time::getMillisecondCounter();
        for (;;)
        {
            bool busy = false;
            for (int s = 0; s < ogi::kTapeSlots; ++s)
                busy = busy || p.getSlotInfo (s).loading;
            if (! busy)
                return true;
            if ((int) (juce::Time::getMillisecondCounter() - t0) > timeoutMs)
                return false;
            pump (1);
        }
    }

    void rowH1()
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { s1 ("density"), 200.0f }, { s1 ("size"), 50.0f }, { s1 ("position"), 20.0f } }));
        const auto srcA = sineBuf (1, (int) (3.0 * fs), 1000.0, fs, 0.2);
        const auto srcB = sineBuf (1, (int) (3.0 * fs), 1000.0, fs, 0.2, 0.5 * kPi);
        s.load (0, srcA, fs, "A");
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        const juce::int64 ts[2] = { 24000, 48000 };   // slot 1 (index 0) loaded twice while sounding
        s.at (ts[0], [&srcB] (Session& ss) { ss.load (0, srcB, 48000.0, "B"); });
        s.at (ts[1], [&srcA] (Session& ss) { ss.load (0, srcA, 48000.0, "A2"); });
        std::vector<juce::int64> retireAt;
        std::uint64_t lastRetired = 0;
        s.afterBlock = [&retireAt, &lastRetired] (Session& ss)
        {
            if (ss.hk().retiredByEngine > lastRetired)
            {
                lastRetired = ss.hk().retiredByEngine;
                retireAt.push_back (ss.pos);
            }
        };
        s.run (72000, 32);

        Tally t;
        double worstDip = 0.0, worstRec = -400.0, worstClick = 0.0;
        juce::int64 worstLat = 0;
        for (int i = 0; i < 2; ++i)
        {
            const juce::int64 t0 = ts[i];
            const double steady = rmsSt (s.L, s.R, t0 - 9600, t0 - 480);
            double minR = 1.0e30;
            for (int k = 0; k < 60; ++k)
                minR = std::min (minR, rmsSt (s.L, s.R, t0 + 48 * k, t0 + 48 * k + 480));
            const double dip = dB (minR / steady);
            const double rec = dB (rmsSt (s.L, s.R, t0 + 2400 + 480, t0 + 2400 + 1440) / steady);
            const double click = d2Peak (s.L, s.R, t0 - 32, t0 + 480) / std::max (1.0e-12, d2Peak (s.L, s.R, t0 - 9600, t0 - 480));
            worstDip = std::min (worstDip, dip);
            worstRec = std::max (worstRec, -rec);
            worstClick = std::max (worstClick, click);
            const juce::int64 lat = i < (int) retireAt.size() ? retireAt[(size_t) i] - t0 : -1;
            worstLat = std::max (worstLat, lat);
            t.check (dip >= -24.0 && rec >= -3.0 && click <= kClickRatio && lat >= 0 && lat <= 240 + 96,
                     "swap " + juce::String (i) + " dip " + f2 (dip, 1) + " rec " + f2 (rec, 2) + " click " + f2 (click, 2)
                         + " retire " + juce::String (lat));
        }
        const float ks = s.eng().getSpawner().getKillStep();
        const bool fade5 = std::abs ((double) ks - 1.0 / 240.0) <= 1.0e-9;
        auto& h = s.hk();
        row (t.bad == 0 && h.tapesTaken == 3 && h.swapKills > 0 && retireAt.size() == 2 && fade5,
             "H1 FUNC-02 load slot 1 twice while 4 notes sound",
             "meas min 10 ms RMS " + f2 (worstDip, 1) + " dB, recovery worst " + f2 (-worstRec, 2) + " dB, d2 ratio " + f2 (worstClick, 3)
                 + ", retire latency " + juce::String (worstLat) + " smp, kills " + i64 (h.swapKills) + ", killStep 1/" + f2 (1.0 / (double) ks, 2)
                 + " | thr dip >= -24, rec >= -3, d2 <= " + f2 (kClickRatio, 1) + ", retire <= 336 | " + t.detail (2) + " | ctrl --");
    }

    void rowH2()
    {
        const int base0 = ogi::SlotTape::liveCount.load();
        const auto f = writeWav ("h2.wav", noiseBuf (1, 4800, 0.2f, 8), 48000.0);
        int maxPending = 0;
        std::uint64_t published = 0, freed = 0, entries = 0;
        int deferred = -1, live = -1;
        bool idleOk = true;
        {
            auto p = make();
            p->prepareToPlay (48000.0, 512);
            auto& h = p->getHandoff();
            for (int i = 0; i < 50; ++i)
            {
                p->requestSlotLoad (0, f);
                idleOk = waitIdle (*p, 3000) && idleOk;
                maxPending = std::max (maxPending, h.pendingDepth (0));
            }
            for (int i = 0; i < 50; ++i)
            {
                p->requestSlotLoad (0, f);
                maxPending = std::max (maxPending, h.pendingDepth (0));
            }
            idleOk = waitIdle (*p, 5000) && idleOk;
            pump (300);   // reaper Timer ticks
            maxPending = std::max (maxPending, h.pendingDepth (0));
            published = h.getPublishedCount();
            freed = h.getSupersededFreed();
            deferred = h.deferredCount();
            entries = h.getBlockEntries();
            live = ogi::SlotTape::liveCount.load() - base0;
        }
        int deferredK = -1;
        {
            auto q = make();
            q->prepareToPlay (48000.0, 512);
            q->getHandoff().testNoEntryCounter.store (true);
            for (int i = 0; i < 20; ++i)
            {
                q->requestSlotLoad (0, f);
                waitIdle (*q, 3000);
            }
            pump (300);
            deferredK = q->getHandoff().deferredCount();
            q->getHandoff().testNoEntryCounter.store (false);
            q->getHandoff().sweep();
        }
        const bool fired = deferredK >= 10;
        row (idleOk && maxPending <= 1 && deferred == 0 && live == 1 && published >= 50 && freed + 1 == published && entries == 0 && fired,
             "H2 host idle: 100 requestSlotLoad, 0 processBlock, loop pumped",
             "meas pending max " + juce::String (maxPending) + ", published " + i64 (published) + ", superseded freed " + i64 (freed)
                 + ", deferred " + juce::String (deferred) + ", live tapes " + juce::String (live) + " | thr <= 1, freed = published - 1, 0, 1"
                 + " | " + ctrl (fired, "no entry counter -> " + juce::String (deferredK) + " deferred (leak)"));
    }

    struct H3R { bool retired = false, violation = false, sawFadeOnly = false; std::uint64_t steals = 0, early = 0; juce::int64 lat = -1; };

    H3R runH3 (bool scan256)
    {
        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "grain_limit", 3.0f }, { s1 ("density"), 200.0f }, { s1 ("size"), 2000.0f } }));
        s.hk().thinningOff = true;   // saturate the pool → native steals into the 32 fade spares
        s.hk().retireScan256 = scan256;
        s.eng().testSetPoolFadeMs (20.0);   // fade spares outlive the 5 ms swap kills
        s.record = false;
        s.load (0, noiseBuf (2, (int) (3.0 * fs), 0.2f), fs);
        const auto src2 = noiseBuf (2, (int) (3.0 * fs), 0.2f, 9);
        for (int k = 0; k < 16; ++k)
            s.noteOn (0, 48);
        const juce::int64 tsw = 24000;
        const ogi::SlotTape* old = nullptr;
        std::uint64_t retired0 = 0;
        H3R r;
        s.at (tsw, [&] (Session& ss)
        {
            old = ss.eng().getCurrentTape (0);
            retired0 = ss.hk().retiredByEngine;
            ss.load (0, src2, 48000.0, "swap");
        });
        s.afterBlock = [&] (Session& ss)
        {
            if (old == nullptr || r.retired)
                return;
            const auto& pool = ss.eng().getPool();
            const auto* g = pool.getGrains().data();
            bool live = false, fade = false;
            for (int i = 0; i < ogi::GiPool::kNumSlots; ++i)
                if (pool.isActive (i) && (g[i].tape == old || g[i].xTape == old))
                    (i < ogi::kPoolLive ? live : fade) = true;
            if (fade && ! live)
                r.sawFadeOnly = true;
            if (ss.hk().retiredByEngine > retired0)
            {
                r.retired = true;
                r.violation = live || fade;   // references only decrease after the swap
                r.lat = ss.pos - tsw;
            }
        };
        s.run (48000, 32);
        r.steals = s.hk().nativeSteals;
        r.early = s.hk().earlyRetires;
        return r;
    }

    void rowH3()
    {
        const auto r = runH3 (false);
        const auto k = runH3 (true);
        const bool fired = k.early > 0;
        row (r.retired && ! r.violation && r.steals > 0 && r.sawFadeOnly && fired,
             "H3 retire vs fade spare: tape retired only once all 288 slots clear",
             "meas retired " + juce::String (r.retired ? "yes" : "NO") + " after " + juce::String (r.lat) + " smp, held at retire "
                 + (r.violation ? "YES" : "no") + ", native steals " + i64 (r.steals) + ", fade-only hold seen "
                 + (r.sawFadeOnly ? "yes" : "NO (stimulus vacuous)") + " | thr 0 references at retire"
                 + " | " + ctrl (fired, "retireScan256 -> " + i64 (k.early) + " early retires"));
    }

    void rowH4()
    {
        constexpr double fs2 = 96000.0;
        Session s (48000.0, with (base(), { { s1 ("density"), 50.0f }, { s1 ("size"), 100.0f }, { s1 ("position"), 20.0f },
                                            { "amp_attack", 20.0f } }));
        s.load (0, sineBuf (1, 3 * 48000, 440.0, 48000.0, 0.3), 48000.0);
        s.run (480, 32);                    // tape taken at 48 k
        s.p->prepareToPlay (fs2, 512);      // engine 96 k; rebuild queued via callAsync (not pumped yet)
        const juce::int64 t0 = s.pos;
        s.noteOn (t0, 48);                  // prepareToPlay cleared the voices: the note starts now
        juce::int64 tX = -1;
        int xBlocks = 0;
        double worstRate = 0.0;
        s.afterBlock = [&] (Session& ss)
        {
            const auto& h = ss.hk();
            if (h.voiceState[0] == 1 && h.tapeRate[0] > 0.0)
                worstRate = std::max (worstRate, std::abs (h.voiceRate[0] * fs2 / h.tapeRate[0] - 1.0));
            if (h.xfadesStarted > 0 && tX < 0)
                tX = ss.pos - 32;
            if (h.xfadeActive[0])
                ++xBlocks;
            if (ss.pos - t0 > (juce::int64) (0.15 * fs2) && tX < 0)
                pump (1);
        };
        while (tX < 0 && s.pos - t0 < (juce::int64) (20.0 * fs2))
            s.run (3200, 32);
        s.run ((juce::int64) (0.2 * fs2), 32);
        const int xLen = xBlocks * 32;
        const double click = tX > 0 ? d2Peak (s.L, s.R, tX - 64, tX + 1920 + 64) / std::max (1.0e-12, d2Peak (s.L, s.R, tX - 9600, tX - 480))
                                    : 1.0e9;
        row (tX > 0 && worstRate <= 1.0e-12 && std::abs (xLen - 1920) <= 32 && click <= kClickRatio && sameD (s.hk().tapeRate[0], fs2),
             "H4 SR change 48 -> 96 k mid-note: pitch, 20 ms crossfade, no click",
             "meas rebuild at " + juce::String (tX - t0) + " smp, pitch err max " + juce::String (worstRate, 15) + ", crossfade "
                 + juce::String (xLen) + " smp, d2 ratio " + f2 (click, 3) + " | thr 1e-12 / 1920 +-32 / <= " + f2 (kClickRatio, 1)
                 + " | margin d2 " + f2 (kClickRatio - click, 3) + " | ctrl --");
    }

    void rowH5()
    {
        const int base0 = ogi::SlotTape::liveCount.load();
        bool consistent = true, settled = false;
        int loaded = 0;
        juce::String why;
        {
            auto p = make();
            setP (*p, "polyphony", 5.0f);
            const auto blob = stateOf (*p);
            const juce::File files[3] = { writeWav ("h5-441.wav", noiseBuf (1, 2205, 0.2f, 10), 44100.0),
                                          writeWav ("h5-48.wav",  noiseBuf (2, 2400, 0.2f, 11), 48000.0),
                                          writeWav ("h5-96.wav",  noiseBuf (1, 4800, 0.2f, 12), 96000.0) };
            const double rates[3] = { 44100.0, 48000.0, 96000.0 };
            juce::Random r ((juce::int64) 0x45000005);
            p->prepareToPlay (48000.0, 512);
            juce::AudioBuffer<float> b (2, 256);
            juce::MidiBuffer mb;
            for (int i = 0; i < 200; ++i)
            {
                switch (r.nextInt (5))
                {
                    case 0:  p->prepareToPlay (rates[r.nextInt (3)], 512); break;
                    case 1:  p->setStateInformation (blob.getData(), (int) blob.getSize()); break;
                    case 2:  p->requestSlotLoad (r.nextInt (4), files[r.nextInt (3)]); break;
                    case 3:
                    {
                        b.clear();
                        mb.clear();
                        mb.addEvent (juce::MidiMessage::noteOn (1, 40 + r.nextInt (40), (juce::uint8) 100), 0);
                        if (r.nextBool()) mb.addEvent (juce::MidiMessage::allNotesOff (1), 200);
                        p->processBlock (b, mb);
                        break;
                    }
                    default: pump (1); break;
                }
            }
            waitIdle (*p, 5000);
            const double sr = p->getLoader().getSessionRate();
            for (int k = 0; k < 500 && ! settled; ++k)   // let SR rebuilds land
            {
                pump (10);
                b.clear();
                mb.clear();
                p->processBlock (b, mb);
                settled = true;
                for (int sl = 0; sl < ogi::kTapeSlots; ++sl)
                {
                    const auto si = p->getSlotInfo (sl);
                    if (si.loaded && (si.loading || ! sameD (si.tapeRate, sr)))
                        settled = false;
                }
            }
            for (int sl = 0; sl < ogi::kTapeSlots; ++sl)
            {
                const auto si = p->getSlotInfo (sl);
                if (! si.loaded)
                    continue;
                ++loaded;
                const bool ok = si.frames > 0 && si.name.isNotEmpty() && si.nativeFrames > 0 && ! si.loading && si.lastError.isEmpty()
                             && (sameD (si.nativeRate, 44100.0) || sameD (si.nativeRate, 48000.0) || sameD (si.nativeRate, 96000.0))
                             && sameD (si.tapeRate, sr);
                if (! ok)
                    why << " slot" << sl << " frames " << si.frames << " rate " << si.tapeRate << " err " << si.lastError;
                consistent = consistent && ok;
            }
        }
        const int leaked = ogi::SlotTape::liveCount.load() - base0;
        row (consistent && settled && loaded >= 1 && leaked == 0, "H5 200 interleaved prepare / setState / load / process calls",
             "meas " + juce::String (loaded) + " slots loaded, SlotInfo consistent " + (consistent ? "yes" : "NO" + why)
                 + ", rebuilds settled " + (settled ? "yes" : "NO") + ", leaked tapes " + juce::String (leaked)
                 + " | thr consistent, 0 leaked | ctrl --");
    }

    //==========================================================================
    // N — determinism
    P n1Params()
    {
        return with (base(), { { "polyphony", 4.0f }, { "grain_limit", 1.0f }, { s1 ("density"), 100.0f }, { s1 ("size"), 300.0f },
                               { s1 ("size_rand"), 50.0f }, { s1 ("scan_rate"), 100.0f }, { s1 ("scan_mode"), 1.0f },
                               { "amp_attack", 5.0f }, { "amp_release", 200.0f } });
    }

    void n1Script (Session& s)
    {
        for (int n : { 48, 52, 55 })
            s.noteOn (0, n);
        s.noteOn (1000, 60);
        s.noteOn (5003, 64);   // 5th note at polyphony 4: a steal
        s.addMidi (7001, juce::MidiMessage::pitchWheel (1, 10000));
        s.noteOn (10007, 67);
        s.noteOff (30011, 52);
        s.addMidi (40000, juce::MidiMessage::pitchWheel (1, 8192));
        s.noteOn (44101, 70);
        s.noteOff (60000, 48);
        s.noteOff (60000, 55);
        s.noteOn (70001, 50);
        s.addMidi (85000, juce::MidiMessage::allNotesOff (1));
    }

    std::vector<float> renderN (int block, std::uint64_t seed, const std::function<void (Proc&)>& tweak, std::uint64_t* r3 = nullptr)
    {
        Session s (48000.0, n1Params(), seed, 512);
        if (tweak)
            tweak (*s.p);
        s.load (0, noiseBuf (2, 3 * 48000, 0.3f, 7), 48000.0);
        n1Script (s);
        s.run (96000, block);
        if (r3 != nullptr)
            *r3 = s.hk().r3Kills + s.hk().capKills;
        std::vector<float> out (s.L);
        out.insert (out.end(), s.R.begin(), s.R.end());
        return out;
    }

    int diffCount (const std::vector<float>& a, const std::vector<float>& b)
    {
        if (a.size() != b.size())
            return INT_MAX;
        int n = 0;
        for (size_t i = 0; i < a.size(); ++i)
            n += sameF (a[i], b[i]) ? 0 : 1;
        return n;
    }

    void rowsN()
    {
        std::uint64_t kills = 0;
        const auto ref = renderN (32, kSeed, {}, &kills);
        double e = 0.0;
        for (float x : ref) e += (double) x * (double) x;
        const bool audible = std::sqrt (e / (double) ref.size()) > 1.0e-4;
        Tally t;
        juce::String d;
        int worst = 0;
        for (int b : { 441, 512, 4096, 8192, 0 })
        {
            const int n = diffCount (ref, renderN (b, kSeed, {}));
            worst = std::max (worst, n);
            t.check (n == 0, (b == 0 ? juce::String ("var") : juce::String (b)) + " " + juce::String (n));
            d << " " << (b == 0 ? juce::String ("var") : juce::String (b)) << ":" << n;
        }
        const int nSeed = diffCount (ref, renderN (32, kSeed + 0x1234, {}));
        const bool fired = nSeed > 0;
        row (t.bad == 0 && audible && fired, "N1 PERF-03 block-size null vs 32 (size_rand 50 %, steals, cap 64)",
             "meas differing samples" + d + " (8192 = large-block probe, prepared 512; kills " + i64 (kills) + ") | thr 0 | margin "
                 + juce::String (-worst) + " | " + ctrl (fired, "seed change -> " + juce::String (nSeed) + " differ"));

        // 2.2: the unread set is everything outside readSet22() (the 2.1 set
        // plus every row 2.2 binds); s2-s4 stay disabled by base().
        // 2.3 (PLAN-2.3 PD10): readSet23() — the FX rows are bound now.
        const auto readSet = readSet23();
        int bound = 0;
        {
            auto probe = make();
            for (const auto& id : readSet)
                bound += param (*probe, id) != nullptr ? 1 : 0;
        }
        int touched = 0;
        auto unread = [&readSet, &touched] (Proc& p)
        {
            juce::Random r ((juce::int64) 0x4E200002);
            touched = 0;
            for (auto* ap : p.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                    if (! readSet.contains (rp->getParameterID()))
                    {
                        rp->setValueNotifyingHost (r.nextFloat());
                        ++touched;
                    }
        };
        const auto base512 = renderN (512, kSeed, {});
        const int nUnread = diffCount (base512, renderN (512, kSeed, unread));
        const int nDensity = diffCount (base512, renderN (512, kSeed, [] (Proc& p) { setP (p, s1 ("density"), 120.0f); }));
        const bool fired2 = nDensity > 0;
        row (nUnread == 0 && bound == 153 && touched == 108 && fired2, "N2 unread params (sync, fb, reverb, mod, ...) bit-identical",
             "meas " + juce::String (nUnread) + " samples differ with " + juce::String (touched) + " unread params randomised ("
                 + juce::String (bound) + "/153 read IDs bound, readSet23) | thr 0 | margin " + juce::String (-nUnread)
                 + " | " + ctrl (fired2, "s1_density 120 -> " + juce::String (nDensity) + " differ"));
    }

    //==========================================================================
    // A1 — audio-thread allocations
    void rowA1()
    {
       #if JUCE_MAC
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocCount = 0;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const int liveness = allocCount;
        allocCount = 0;
        allocTrace = true;

        constexpr double fs = 48000.0;
        Session s (fs, with (base(), { { "polyphony", 8.0f }, { s1 ("density"), 100.0f }, { s1 ("size"), 200.0f },
                                       { s1 ("size_rand"), 50.0f }, { "amp_attack", 5.0f }, { "amp_release", 100.0f } }));
        s.record = false;
        s.load (0, noiseBuf (2, 2 * 48000, 0.3f), fs);
        const auto src2 = noiseBuf (2, 2 * 48000, 0.3f, 13);
        s.arm = true;
        s.noteOn (4800, 60);                       // note on / off
        s.noteOff (9600, 60);
        for (int n = 40; n < 60; ++n)              // 20-note chord @ poly 8: steals
            s.noteOn (14400, n);
        for (int n = 40; n < 60; ++n)
            s.noteOff (24000, n);
        s.noteOn (28800, 48);                      // held
        s.at (33600, [&src2] (Session& ss) { ss.load (0, src2, 48000.0, "swap"); });   // swap mid-note
        for (int n = 50; n < 58; ++n)
            s.noteOn (38400, n);
        s.at (43200, [] (Session& ss) { setP (*ss.p, "grain_limit", 0.0f); });       // cap 256 -> 32 live
        s.run (57600, 512);

        s.p->prepareToPlay (96000.0, 512);         // SR change, then notes sound through the rebuild
        s.noteOn (s.pos, 48);
        s.noteOn (s.pos, 55);
        s.afterBlock = [] (Session&) { pump (1); };
        const juce::int64 tStart = s.pos;
        while (s.hk().xfadesStarted == 0 && s.pos - tStart < (juce::int64) (20.0 * 96000.0))
            s.run (3200, 32);
        s.afterBlock = nullptr;
        s.run ((juce::int64) (0.2 * 96000.0), 512);
        s.arm = false;
        malloc_logger = nullptr;
        allocTrace = false;

        const int n = allocCount;
        auto& h = s.hk();
        const bool exercised = h.voiceSteals > 0 && h.tapesTaken >= 3 && h.xfadesStarted >= 1 && (h.r3Kills + h.capKills) > 0;
        row (liveness == 1 && n == 0 && exercised, "A1 PERF-01 0 audio-thread allocations (malloc_logger)",
             "meas " + juce::String (n) + " allocs over " + i64 ((std::uint64_t) s.blocksDone) + " blocks (steals " + i64 (h.voiceSteals)
                 + ", tapes taken " + i64 (h.tapesTaken) + ", xfades " + i64 (h.xfadesStarted) + ", r3+cap kills "
                 + i64 (h.r3Kills + h.capKills) + ") | thr 0 | margin " + juce::String (-n)
                 + " | " + ctrl (liveness == 1, "liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (false, "A1 alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    void runAll()
    {
        struct TmpGuard
        {
            TmpGuard()
            {
                gTmp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                           .getChildFile ("ogi-gates-21-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64()));
                gTmp.createDirectory();
            }
            ~TmpGuard() { gTmp.deleteRecursively(); }
        } guard;

        const double t0 = juce::Time::getMillisecondCounterHiRes();
        auto section = [t0] (const char* title)
        {
            std::printf ("\n-- %s  (t = %.1f s)\n", title, (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
            std::fflush (stdout);
        };

        std::printf ("\n== Stage 2 / Phase 2.1 engine gates (engine_gates_21.h)\n");
        section ("Q. grain budget (D12)");
        rowQ0();
        rowsQ1();
        rowQ2();
        rowQ3();
        rowQ4();

        MipFix fx;
        section ("T. MipTape");
        rowsT (fx);
        section ("K. resampler");
        rowsK();
        section ("L. loader");
        rowsL();
        section ("P. pitch");
        rowP1();
        rowP2();
        rowP3();
        rowP4 (fx);
        section ("S. scan / loop region");
        rowS1();
        rowS2();
        section ("V. voices");
        rowV1();
        rowsV23();
        rowV4();
        section ("M. MIDI");
        rowM1();
        section ("E. amp envelope");
        rowE1();
        section ("H. slot handoff");
        rowH1();
        rowH2();
        rowH3();
        rowH4();
        rowH5();
        section ("N. determinism");
        rowsN();
        section ("A. allocations");
        rowA1();
        std::printf ("\n-- Phase 2.1 gates done (%.1f s)\n", (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
    }
}} // namespace ::eg21
