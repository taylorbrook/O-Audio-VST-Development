/*
  ==============================================================================

    O-Granulator — render harness, Stage 2 / Phase 2.5b analysis rows
    (PLAN-2.5b P76). Sibling of engine_gates_25ab.h; engine_gates_25bb.h holds
    the f0-ring / follow / matrix / PERF rows.

      Stimulus helpers (P75) and the double references (detector, MPM)
      OD0–OD7  onset detector (FUNC-08, R60 / R61 / R74 / R75)        (T3, T4)
      OB1–OB5  onset bursts: Free re-phase, Tempo extra grain         (T3, T4)
      PS1–PS4  Onset / Pitch source laws, telemetry                   (T4, T6)
      MP0–MP4  decimator + MPM (DSP-08), R59 hop schedule             (T5, T6)
      OG1–OG3  D20 guard (R66 / R67)                                  (T6)

  ==============================================================================
*/
#pragma once

namespace { namespace eg25x
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg25::renderAll;
    using eg25b::route;
    using eg25b::operator+;
    using Stim = std::function<float (juce::int64, int)>;
    constexpr double kPi = eg::kPi;

    //==========================================================================
    // Stimulus helpers (P75). Every stimulus is position-deterministic and
    // identical on both channels unless stated (noise is per channel).
    //==========================================================================
    /** A precomputed mono buffer as a stimulus (both channels). */
    inline Stim fromBuffer (std::shared_ptr<std::vector<float>> b)
    {
        return [b] (juce::int64 i, int) { return i >= 0 && i < (juce::int64) b->size() ? (*b)[(size_t) i] : 0.0f; };
    }

    /** Seeded white noise in [−1, 1) × amp (deterministic per index). */
    inline double white (juce::int64 i, std::uint64_t seed)
    {
        std::uint64_t x = static_cast<std::uint64_t> (i) * 0x9E3779B97F4A7C15ull + seed * 0xD1B54A32D192ED03ull + 0x632BE59BD9B4E019ull;
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ull;
        x ^= x >> 27; x *= 0x94D049BB133111EBull;
        x ^= x >> 31;
        return 2.0 * (static_cast<double> (x >> 11) / 9007199254740992.0) - 1.0;
    }
    /** Gaussian-ish (sum of 4 uniforms, unit variance). */
    inline double gauss (juce::int64 i, std::uint64_t seed)
    {
        double s = 0.0;
        for (std::uint64_t k = 0; k < 4; ++k) s += white (4 * i + (juce::int64) k, seed);
        return s * std::sqrt (3.0 / 4.0);
    }

    /** Hann-windowed tone burst added into b: start (s), dur (ms), hz, amp. */
    inline void addBurst (std::vector<float>& b, double fs, double startS, double durMs, double hz, double amp, bool hann = true)
    {
        const auto n0 = (juce::int64) std::llround (startS * fs);
        const auto L  = (juce::int64) std::llround (durMs * 0.001 * fs);
        for (juce::int64 k = 0; k < L; ++k)
        {
            const auto i = n0 + k;
            if (i < 0 || i >= (juce::int64) b.size()) continue;
            const double w = hann ? 0.5 * (1.0 - std::cos (2.0 * kPi * (double) k / (double) L)) : 1.0;
            b[(size_t) i] += (float) (amp * w * std::sin (2.0 * kPi * hz * (double) k / fs));
        }
    }

    /** A burst train: count bursts every ioi s from first s. */
    inline std::shared_ptr<std::vector<float>> burstTrain (double fs, double lenS, double firstS, double ioiS, int count,
                                                    double durMs, double hz, double amp, bool hann = true)
    {
        auto b = std::make_shared<std::vector<float>> ((size_t) std::llround (lenS * fs), 0.0f);
        for (int k = 0; k < count; ++k)
            addBurst (*b, fs, firstS + ioiS * k, durMs, hz, amp, hann);
        return b;
    }

    /** Burst onset samples of a train (the stimulus truth). */
    inline std::vector<juce::int64> burstStarts (double fs, double firstS, double ioiS, int count)
    {
        std::vector<juce::int64> v;
        for (int k = 0; k < count; ++k) v.push_back ((juce::int64) std::llround ((firstS + ioiS * k) * fs));
        return v;
    }

    /** A piecewise-constant-frequency sine (phase-continuous at each step).
        segs: {startS, hz}, ascending; amp; optional attack (ms, raised cosine) at
        the first segment start and a hard stop at endS (< 0 = none). */
    struct Seg { double t, hz; };
    inline std::shared_ptr<std::vector<float>> steppedSine (double fs, double lenS, const std::vector<Seg>& segs, double amp,
                                                     double attackMs = 0.0, double endS = -1.0)
    {
        auto b = std::make_shared<std::vector<float>> ((size_t) std::llround (lenS * fs), 0.0f);
        double ph = 0.0;
        size_t si = 0;
        const double t0 = segs.empty() ? 0.0 : segs[0].t;
        for (size_t i = 0; i < b->size(); ++i)
        {
            const double t = (double) i / fs;
            while (si + 1 < segs.size() && t >= segs[si + 1].t) ++si;
            if (segs.empty() || t < t0 || (endS >= 0.0 && t >= endS)) { ph += 0.0; continue; }
            double env = 1.0;
            if (attackMs > 0.0)
            {
                const double x = (t - t0) / (attackMs * 0.001);
                env = x >= 1.0 ? 1.0 : std::pow (std::sin (0.5 * kPi * x), 2.0);
            }
            (*b)[i] = (float) (amp * env * std::sin (ph));
            ph += 2.0 * kPi * segs[si].hz / fs;
            if (ph > 2.0 * kPi) ph -= 2.0 * kPi;
        }
        return b;
    }

    inline double midiOf (double hz) { return 69.0 + 12.0 * std::log2 (hz / 440.0); }
    inline double dbAmp (double db)  { return std::pow (10.0, db / 20.0); }

    /** The analysis of a processor (test builds). */
    inline const ogran::Analysis& an (const Out& o) { return o.eng().getAnalysis(); }
    inline const ogran::Analysis& an (Proc& p)      { return p.getEngineForTest().getAnalysis(); }

    /** Every event of a render (the eventLog, drained after every block). */
    struct EvLog { std::vector<juce::int64> at; std::uint64_t seen = 0; bool overflow = false; };
    inline std::shared_ptr<EvLog> evInto (Job& j)
    {
        auto L = std::make_shared<EvLog>();
        auto prev = j.after;
        j.after = [L, prev] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const auto& a = an (p);
            if (a.eventCount - L->seen > (std::uint64_t) ogran::Analysis::kLogSize) L->overflow = true;
            for (std::uint64_t k = L->seen; k < a.eventCount; ++k) L->at.push_back (a.eventLog[(size_t) (k % ogran::Analysis::kLogSize)].at);
            L->seen = a.eventCount;
        };
        return L;
    }
    /** Every committed hop of a render (the hopLog). */
    struct HopLog { std::vector<ogran::Analysis::HopLogEntry> h; std::uint64_t seen = 0; bool overflow = false; };
    inline std::shared_ptr<HopLog> hopsInto (Job& j)
    {
        auto L = std::make_shared<HopLog>();
        auto prev = j.after;
        j.after = [L, prev] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const auto& a = an (p);
            if (a.hopCount - L->seen > (std::uint64_t) ogran::Analysis::kLogSize) L->overflow = true;
            for (std::uint64_t k = L->seen; k < a.hopCount; ++k) L->h.push_back (a.hopLog[(size_t) (k % ogran::Analysis::kLogSize)]);
            L->seen = a.hopCount;
        };
        return L;
    }
    using eg25::collectInto;
    using Rec = ogran::Engine::SpawnRecord;

    inline void info (const juce::String& s) { std::printf ("  [INFO] %s\n", s.toRawUTF8()); }

    /** An analysis job: s1 enabled at defaults, mix 100 %, the stimulus, the
        rate. Nothing consumes the analysis unless the params say so. */
    inline Job anaJob (double fs, double lenS, Stim stim, P extra = {})
    {
        Job j;
        j.fs = fs;
        j.len = (juce::int64) std::llround (lenS * fs);
        j.block = 512;
        j.prepBlock = 512;
        j.stim = std::move (stim);
        j.params = P { { "mix", 100.0f } } + extra;
        return j;
    }

    inline juce::String fsName (double fs) { return juce::String (fs / 1000.0, fs == 44100.0 || fs == 88200.0 || fs == 176400.0 ? 1 : 0) + "k"; }

} } // namespace eg25x

static void engineRows25b()
{
    std::printf ("\n== Stage 2 / 2.5b analysis rows (onset, bursts, sources, MPM, D20)\n");
}
