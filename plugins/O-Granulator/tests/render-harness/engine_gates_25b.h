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

    /** The P60 composite as scaled at T3 (tests/n3 header): hash noise × 0.05
        (per channel) + 0.05 sine 227 → 301 Hz (phase-continuous at 3 s) + 5 ms
        1 kHz hard-gated bursts at 0.4 every 250 ms from 0.5 s. */
    inline Stim compositeStim (double fs)
    {
        return [fs] (juce::int64 i, int ch)
        {
            const double t = (double) i / fs, tStep = 3.0;
            const double ph = t < tStep ? 2.0 * kPi * 227.0 * t : 2.0 * kPi * 227.0 * tStep + 2.0 * kPi * 301.0 * (t - tStep);
            double v = 0.05 * (double) eg::hashNoise (i, ch) + 0.05 * std::sin (ph);
            if (t >= 0.5)
            {
                const double k = std::floor ((t - 0.5) / 0.25), tb = t - (0.5 + 0.25 * k);
                if (tb >= 0.0 && tb < 0.005)
                    v += 0.4 * std::sin (2.0 * kPi * 1000.0 * tb);
            }
            return (float) v;
        };
    }

    /** The neutral in-tree recipe (OD0 / OD6): 4 streams, input −12 dB,
        threshold −24 dB, triggers / follow off, no Pitch / Onset route. */
    inline P neutralRecipe()
    {
        P p = eg22::base ({ 1, 2, 3, 4 }) + P { { "input_gain", -12.0f }, { "onset_threshold", -24.0f } };
        for (int s = 1; s <= 4; ++s)
            p = p + P { { eg22::sp (s, "density"), 25.0f }, { eg22::sp (s, "size"), 200.0f } };
        return p;
    }

    //==========================================================================
    // The detector reference (the engine's statements; dbCompare → the R75
    // double dB-domain conditions, OD4b).
    //==========================================================================
    struct RefDet { std::vector<juce::int64> ev; int nearThr = 0; };
    inline RefDet refDetect (const std::vector<float>& x, double fs, double thrDb, bool dbCompare)
    {
        ogran::OnsetDetector d;
        d.prepare (fs);
        const float G = d.hpG(), cFa = d.coeffFastAtt(), cFr = d.coeffFastRel(), cS = d.coeffSlow();
        const int refrLen = d.refractoryLength();
        const float thrLin = (float) std::pow (10.0, thrDb / 20.0);
        float sState = 0.0f, fast = 0.0f, slow = 0.0f;
        bool armed = true;
        int refr = 0;
        RefDet r;
        for (size_t n = 0; n < x.size(); ++n)
        {
            const float a = x[n];
            const float v  = (a - sState) * G;
            const float lp = v + sState;
            sState = lp + v;
            const float hp = a - lp;
            const float xx = std::abs (hp);
            fast += (xx > fast ? cFa : cFr) * (xx - fast);
            slow += (xx > slow ? cS : cS) * (xx - slow);
            bool rearm, cond;
            if (dbCompare)
            {
                const double fdb = 20.0 * std::log10 (std::max (1.0e-30, (double) fast));
                const double sdb = 20.0 * std::log10 (std::max (1.0e-30, (double) slow));
                rearm = fdb < sdb + 3.0;
                cond  = fdb > thrDb && fdb > sdb + 6.0;
                if (std::abs (fdb - thrDb) < 1.0e-6 || std::abs (fdb - sdb - 6.0) < 1.0e-6 || std::abs (fdb - sdb - 3.0) < 1.0e-6)
                    ++r.nearThr;
            }
            else
            {
                rearm = fast < ogran::OnsetDetector::kRearmF * slow;
                cond  = fast > thrLin && fast > ogran::OnsetDetector::kRatioF * slow;
            }
            if (! armed && rearm) armed = true;
            if (refr > 0) --refr;
            if (armed && refr == 0 && cond)
            {
                r.ev.push_back ((juce::int64) n);
                refr = refrLen;
                armed = false;
            }
        }
        return r;
    }

    inline std::vector<float> sampled (const Stim& st, juce::int64 len)
    {
        std::vector<float> v ((size_t) len);
        for (juce::int64 i = 0; i < len; ++i) v[(size_t) i] = st (i, 0);
        return v;
    }

    //==========================================================================
    // OD0–OD6 — onset detector (T3)
    //==========================================================================
    void detectorRows()
    {
        std::printf ("\n-- OD. onset detector (FUNC-08; R60 edge + re-arm, R61 bursts, R74 tap, R75 linear)\n");
        const double rates[] { 44100.0, 48000.0, 96000.0, 192000.0 };

        // OD0 / OD0n — in-tree isolation.
        {
            Job on = anaJob (48000.0, 10.0, compositeStim (48000.0), neutralRecipe());
            Job off = on, trg = on;
            off.pre = [] (Proc& p) { p.getEngineForTest().hooks.analysisOff = true; };
            trg.params = trg.params + P { { "s1_onset_trigger", 1.0f } };
            auto o = renderAll ({ on, off, trg });
            const auto d = eg::diff (o[0], o[1]), dn = eg::diff (o[0], o[2]);
            const auto ev = o[0].eng().diag.onsetEvents, evOff = o[1].eng().diag.onsetEvents;
            row (d.count == 0 && ev > 0 && evOff == 0, "OD0 neutral render: analysisOff == analysis on, bitwise (events fire, nothing consumes them)",
                 juce::String::formatted ("%d diffs; events %lld (off %lld), hops %lld", d.count, (long long) ev, (long long) evOff,
                                          (long long) o[0].eng().diag.mpmHops));
            row (dn.count > 0, "OD0n same render with s1_onset_trigger On: differs", juce::String::formatted ("%d diffs", dn.count));
        }

        // Stimuli per rate (440 Hz sin² 5 ms attack, 200 Hz hard-gated; −12 dBFS from 0.1 s).
        struct Case { juce::String name; std::shared_ptr<std::vector<float>> x; double fs; juce::int64 n0; };
        std::vector<Case> tones, bursts, impulses, pairs;
        for (double fs : rates)
        {
            const auto n0 = (juce::int64) std::llround (0.1 * fs);
            tones.push_back ({ "440 Hz " + fsName (fs), steppedSine (fs, 1.0, { { 0.1, 440.0 } }, 0.25, 5.0), fs, n0 });
            tones.push_back ({ "200 Hz " + fsName (fs), steppedSine (fs, 1.0, { { 0.1, 200.0 } }, 0.25, 0.0), fs, n0 });
            auto b = std::make_shared<std::vector<float>> ((size_t) fs, 0.0f);
            addBurst (*b, fs, 0.1, 1.0, 2000.0, 0.5, false);
            bursts.push_back ({ "1 ms 2 kHz " + fsName (fs), b, fs, n0 });
            auto im = std::make_shared<std::vector<float>> ((size_t) fs, 0.0f);
            (*im)[(size_t) n0] = 1.0f;
            impulses.push_back ({ "impulse " + fsName (fs), im, fs, n0 });
            for (double gap : { 0.030, 0.045, 0.055 })
            {
                auto pr = std::make_shared<std::vector<float>> ((size_t) fs, 0.0f);
                addBurst (*pr, fs, 0.1, 1.0, 2000.0, 0.5, false);
                addBurst (*pr, fs, 0.1 + gap, 1.0, 2000.0, 0.5, false);
                pairs.push_back ({ juce::String (juce::roundToInt (gap * 1000.0)) + " ms pair " + fsName (fs), pr, fs, n0 });
            }
        }
        auto runCases = [] (const std::vector<Case>& cs, bool level)
        {
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<EvLog>> logs;
            for (const auto& c : cs)
            {
                Job j = anaJob (c.fs, 1.0, fromBuffer (c.x));
                if (level) j.pre = [] (Proc& p) { p.getEngineForTest().hooks.levelTrigger = true; };
                logs.push_back (evInto (j));
                jobs.push_back (j);
            }
            renderAll (jobs);
            return logs;
        };
        const auto tEv = runCases (tones, false), tLv = runCases (tones, true);
        const auto bEv = runCases (bursts, false), iEv = runCases (impulses, false), pEv = runCases (pairs, false);

        // OD1 / OD1n
        {
            Tally t, tn;
            juce::String lags;
            for (size_t k = 0; k < tones.size(); ++k)
            {
                const auto& e = tEv[k]->at;
                t.check (e.size() == 1, tones[k].name + juce::String::formatted (": %d events", (int) e.size()));
                if (! e.empty() && k < 2)
                    lags << juce::String ((double) (e[0] - tones[k].n0) / tones[k].fs * 1000.0, 2) << " ms ";
                tn.check (tLv[k]->at.size() == 3, tones[k].name + juce::String::formatted (": level %d", (int) tLv[k]->at.size()));
            }
            juce::String lv;
            for (auto x : tLv[0]->at) lv << juce::String ((double) (x - tones[0].n0) / tones[0].fs * 1000.0, 1) << " ";
            row (t.bad == 0, "OD1 sustained 440 Hz (5 ms attack) + gated 200 Hz, -12 dBFS, 4 rates: exactly 1 event each",
                 t.detail ((int) tones.size()) + "  lag @44.1k " + lags.trim());
            row (tn.bad == 0, "OD1n levelTrigger (ARCH s8 form): 3 events each (re-fires)", tn.detail ((int) tones.size()) + "  440 @44.1k at ms " + lv.trim());
        }
        // OD2 / OD2i
        {
            Tally t;
            juce::String im;
            for (size_t k = 0; k < bursts.size(); ++k)
            {
                t.check (bEv[k]->at.size() == 1, bursts[k].name + juce::String::formatted (": %d", (int) bEv[k]->at.size()));
                im << fsName (impulses[k].fs) << " " << (int) iEv[k]->at.size() << "  ";
            }
            row (t.bad == 0, "OD2 1 ms 2 kHz burst at -6 dBFS detected at 44.1 / 48 / 96 / 192 k", t.detail ((int) bursts.size()));
            info ("OD2i 0 dBFS single-sample impulse events per rate (R61: 1 / 1 / 0 / 0 expected): " + im.trim());
        }
        // OD3 — refractory over every OD stimulus.
        {
            int pairs2 = 0, edgeFires = 0, worstGap = 1 << 30;
            bool ok = true;
            auto scan = [&] (const std::vector<Case>& cs, const std::vector<std::shared_ptr<EvLog>>& L)
            {
                for (size_t k = 0; k < cs.size(); ++k)
                {
                    const int refr = (int) std::lround (0.05 * cs[k].fs);
                    for (size_t i = 1; i < L[k]->at.size(); ++i)
                    {
                        const auto gap = L[k]->at[i] - L[k]->at[i - 1];
                        worstGap = juce::jmin (worstGap, (int) gap);
                        ok = ok && gap >= refr;
                        if (gap == refr) ++edgeFires;
                    }
                }
            };
            scan (tones, tEv); scan (bursts, bEv); scan (impulses, iEv); scan (pairs, pEv);
            Tally t55;
            for (size_t k = 0; k < pairs.size(); ++k)
            {
                const bool is55 = pairs[k].name.startsWith ("55");
                if (is55) { ++pairs2; t55.check (pEv[k]->at.size() == 2, pairs[k].name + juce::String::formatted (": %d", (int) pEv[k]->at.size())); }
            }
            juce::String in;
            for (size_t k = 0; k < pairs.size(); ++k)
                if (! pairs[k].name.startsWith ("55")) in << pairs[k].name << " " << (int) pEv[k]->at.size() << "; ";
            row (ok && t55.bad == 0, "OD3 refractory: no two events closer than round (0.05 fs); 55 ms pair -> 2 events",
                 t55.detail (pairs2) + juce::String::formatted ("  min gap %d samples, fires at exactly e1 + refr: %d", worstGap, edgeFires));
            info ("OD3 pairs inside the refractory (events): " + in.trim());
        }

        // OD4 — threshold −30: −35 dBFS train 0 events, −25 dBFS train every burst; tick latch.
        {
            const double fs = 48000.0;
            const auto starts = burstStarts (fs, 0.2, 0.25, 20);
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<EvLog>> logs;
            for (double db : { -35.0, -25.0 })
            {
                // 5 ms hard-gated bursts (execution deviation: the plan's 2 ms Hann at −25 dBFS peaks the
                // fast envelope at ≈ −30 dB — a stimulus on the threshold, not above it).
                Job j = anaJob (fs, 5.5, fromBuffer (burstTrain (fs, 5.5, 0.2, 0.25, 20, 5.0, 1000.0, dbAmp (db), false)), P { { "onset_threshold", -30.0f } });
                logs.push_back (evInto (j));
                jobs.push_back (j);
            }
            // Tick latch: threshold 0 dB until sample tSet (not on a tick), then −60 dB, inside a burst.
            const juce::int64 B = 48000, tSet = B + 50;   // 48050 % 32 = 18
            {
                auto x = std::make_shared<std::vector<float>> ((size_t) (2.0 * fs), 0.0f);
                addBurst (*x, fs, (double) B / fs, 2.0, 1000.0, 0.5);
                Job j = anaJob (fs, 2.0, fromBuffer (x), P { { "onset_threshold", 0.0f } });
                j.events.push_back ({ tSet, [] (Proc& p) { eg::setP (p, "onset_threshold", -60.0f); } });
                logs.push_back (evInto (j));
                jobs.push_back (j);
            }
            renderAll (jobs);
            int hits = 0;
            for (auto st : starts)
                for (auto e : logs[1]->at)
                    if (e >= st && e - st <= (juce::int64) (0.005 * fs)) { ++hits; break; }
            const juce::int64 nextTick = ((tSet + 31) / 32) * 32;
            const bool latch = logs[2]->at.size() == 1 && logs[2]->at[0] == nextTick;
            row (logs[0]->at.empty() && hits == 20 && (int) logs[1]->at.size() == 20,
                 "OD4 threshold -30 dB: -35 dBFS train 0 events, -25 dBFS train 20/20 bursts",
                 juce::String::formatted ("-35: %d events; -25: %d events, %d hits", (int) logs[0]->at.size(), (int) logs[1]->at.size(), hits));
            row (latch, "OD4 a threshold change takes effect at the next tick (set at 48050 -> event at 48064)",
                 logs[2]->at.empty() ? juce::String ("no event") : juce::String::formatted ("event at %lld", (long long) logs[2]->at[0]));
        }
        // OD4b — R75: linear compare == double dB reference, sample for sample (drum loop, 10 s).
        {
            const double fs = 48000.0;
            auto x = std::make_shared<std::vector<float>> ((size_t) (10.0 * fs), 0.0f);
            int k = 0;
            for (double st = 0.1; st < 9.8; st += 0.125, ++k)
            {
                const auto n = (juce::int64) (st * fs), L = (juce::int64) (0.15 * fs);
                const double amp = k % 2 == 0 ? 0.5 : 0.12;
                for (juce::int64 m = 0; m < L && n + m < (juce::int64) x->size(); ++m)
                    (*x)[(size_t) (n + m)] += (float) (amp * std::exp (-(double) m / (0.03 * fs))
                                                       * (0.5 * gauss (n + m, 5) + std::sin (2.0 * kPi * 180.0 * (double) m / fs)));
            }
            Job j = anaJob (fs, 10.0, fromBuffer (x));
            auto L = evInto (j);
            renderAll ({ j });
            const auto ref = refDetect (*x, fs, -30.0, true);
            row (L->at == ref.ev && ! ref.ev.empty(), "OD4b drum loop 10 s: linear-compare events == double dB-domain reference, sample for sample",
                 juce::String::formatted ("%d engine / %d reference events (%d hits of %d), %d samples within 1e-6 dB of a threshold",
                                          (int) L->at.size(), (int) ref.ev.size(), (int) ref.ev.size(), k, ref.nearThr));
        }

        // OD5 — tap point (R74).
        {
            const double fs = 48000.0;
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<EvLog>> logs;
            // (a) feedback 100 %, input removed at 2 s.
            {
                auto x = burstTrain (fs, 6.0, 0.2, 0.25, 7, 2.0, 1000.0, 0.5);
                Job j = anaJob (fs, 6.0, fromBuffer (x), P { { "feedback", 100.0f }, { "s1_density", 50.0f }, { "s1_size", 200.0f },
                                                             { "s1_spray", 30.0f } });
                logs.push_back (evInto (j));
                jobs.push_back (j);
            }
            // (b) −20 dBFS train at threshold −30, input_gain 0 / −24 dB. (c) freeze on.
            for (float ig : { 0.0f, -24.0f })
            {
                Job j = anaJob (fs, 5.5, fromBuffer (burstTrain (fs, 5.5, 0.2, 0.25, 20, 2.0, 1000.0, 0.1)),
                                P { { "onset_threshold", -30.0f }, { "input_gain", ig } });
                logs.push_back (evInto (j));
                jobs.push_back (j);
            }
            {
                Job j = anaJob (fs, 5.5, fromBuffer (burstTrain (fs, 5.5, 0.2, 0.25, 20, 2.0, 1000.0, 0.5)), P { { "freeze", 1.0f } });
                logs.push_back (evInto (j));
                jobs.push_back (j);
            }
            auto o = renderAll (jobs);
            int late = 0, early = 0;
            for (auto e : logs[0]->at) (e > (juce::int64) (1.9 * fs) ? late : early)++;
            const double ringDb = eg::rmsDb (o[0].L, (size_t) (2.5 * fs), (size_t) (6.0 * fs));
            row (late == 0 && early > 0 && ringDb > -80.0, "OD5 feedback 100 %, input removed at 2 s: 0 events after (the return is not tapped)",
                 juce::String::formatted ("events before %d, after %d; loop output %.1f dBFS", early, late, ringDb));
            row ((int) logs[1]->at.size() == 20 && logs[2]->at.empty(), "OD5 -20 dBFS train, threshold -30: input_gain 0 -> 20 events, -24 dB -> 0",
                 juce::String::formatted ("%d / %d", (int) logs[1]->at.size(), (int) logs[2]->at.size()));
            row ((int) logs[3]->at.size() == 20 && o[3].eng().diag.freezeEngages > 0, "OD5 freeze on: the detector still fires on the live input (20/20)",
                 juce::String::formatted ("%d events, engages %lld", (int) logs[3]->at.size(), (long long) o[3].eng().diag.freezeEngages));
        }

        // OD6 — NaN at the tap.
        {
            Job a = anaJob (48000.0, 4.0, compositeStim (48000.0), neutralRecipe());
            Job b = a;
            auto La = evInto (a), Lb = evInto (b);
            const juce::int64 at = (juce::int64) (1.3 * 48000.0);
            b.pre = [] (Proc& p) { p.getEngineForTest().hooks.injectAnalysis = { at, std::numeric_limits<float>::quiet_NaN(), false }; };
            auto o = renderAll ({ a, b });
            const auto d = eg::diff (o[0], o[1]);
            const auto& det = an (o[1]).getDetector();
            bool after = false;
            for (auto e : Lb->at) after = after || e > at;
            row (d.count == 0 && o[1].eng().diag.analysisScrubs == 1 && o[0].eng().diag.analysisScrubs == 0 && La->at == Lb->at && after
                     && std::isfinite (det.fastEnv()) && std::isfinite (det.slowEnv()) && std::isfinite (det.hpState()),
                 "OD6 NaN at the tap: 1 scrub, no event from it, finite state, next burst fires; audio bitwise",
                 juce::String::formatted ("scrubs %lld, events %d vs %d, %d diffs", (long long) o[1].eng().diag.analysisScrubs,
                                          (int) Lb->at.size(), (int) La->at.size(), d.count));
        }
    }

    //==========================================================================
    // OB1 — Free re-phase (D14 / R73) (T3)
    //==========================================================================
    void freeBurstRows()
    {
        std::printf ("\n-- OB1. onset bursts, Free: re-phase at the event (D14 / R73)\n");
        const double fs = 48000.0;
        const double dens = 8.0;   // execution deviation: plan 4 g/s — the 180 ms IOI is shorter than the 250 ms interval
        auto train = burstTrain (fs, 20.0, 0.3, 0.18, 108, 2.0, 1000.0, 0.5);
        P prm = eg22::base ({ 1, 2, 3 }) + P { { "s4_enable", 0.0f }, { "s1_onset_trigger", 1.0f }, { "s2_onset_trigger", 1.0f },
                                             { "s3_onset_trigger", 0.0f }, { "s4_onset_trigger", 1.0f } };
        for (int s = 1; s <= 4; ++s)
            prm = prm + P { { eg22::sp (s, "density"), (float) dens }, { eg22::sp (s, "jitter"), 0.0f } };
        Job j = anaJob (fs, 20.0, fromBuffer (train), prm);
        Job off = j;
        off.params = off.params + P { { "s1_onset_trigger", 0.0f }, { "s2_onset_trigger", 0.0f }, { "s4_onset_trigger", 0.0f } };
        auto L = collectInto (j);
        auto E = evInto (j);
        auto o = renderAll ({ j, off });
        const auto& recs = L->r;
        const juce::int64 I = (juce::int64) std::llround (fs / dens);
        int atEvent = 0, wrongFrac = 0, wrongIdx = 0, nextOk = 0, nextBad = 0, s3bad = 0, bursts = 0;
        for (int s = 0; s < 3; ++s)
        {
            std::vector<const Rec*> rs;
            for (const auto& r : recs) if (r.stream == s) rs.push_back (&r);
            std::set<std::uint64_t> idx;
            for (auto* r : rs) idx.insert (r->globalIndex);
            if (idx.size() != rs.size()) ++wrongIdx;
            for (size_t k = 0; k < rs.size(); ++k)
            {
                const auto* r = rs[k];
                if (s == 2)
                {
                    if (r->burst || (k > 0 && r->onsetSample - rs[k - 1]->onsetSample != I)) ++s3bad;
                    continue;
                }
                if (! r->burst) continue;
                ++bursts;
                if (r->frac != 0.0) ++wrongFrac;
                if (k > 0 && r->globalIndex != rs[k - 1]->globalIndex + 1) ++wrongIdx;
                if (k + 1 < rs.size())
                {
                    if (rs[k + 1]->onsetSample == r->onsetSample + I && ! rs[k + 1]->burst) ++nextOk;
                    else if (! rs[k + 1]->burst || rs[k + 1]->onsetSample - r->onsetSample > I) ++nextBad;
                }
            }
            for (auto e : E->at)
            {
                bool f = false;
                for (auto* r : rs) f = f || (r->onsetSample == e && r->burst);
                if (s < 2 && f) ++atEvent;
            }
        }
        const auto& d = o[0].eng().diag, &dOff = o[1].eng().diag;
        const int want = 2 * (int) E->at.size();
        row (! L->overflow && atEvent == want && bursts == want && wrongFrac == 0 && ! E->at.empty(),
             "OB1 s1, s2 (trigger on) spawn at every reported event sample (delta 0), frac 0",
             juce::String::formatted ("%d / %d at the event, %d events, frac != 0: %d", atEvent, want, (int) E->at.size(), wrongFrac));
        row (wrongIdx == 0, "OB1 the re-phase grain is index 0 of the new epoch (prev + 1); globalIndex unique per stream",
             juce::String::formatted ("%d violations", wrongIdx));
        row (nextBad == 0 && nextOk > 0, "OB1 the next grid onset sits at event + fs/density exactly",
             juce::String::formatted ("%d ok, %d bad (I = %lld)", nextOk, nextBad, (long long) I));
        row (s3bad == 0 && d.planCalls[3] == 0 && d.schedCalls[3] == dOff.schedCalls[3] && d.planCalls[3] == dOff.planCalls[3]
                 && d.planCalls[2] == dOff.planCalls[2],
             "OB1 s3 (trigger off) keeps its grid, no burst; s4 (disabled, trigger on) draws nothing",
             juce::String::formatted ("s3 bad %d; s4 plan %lld sched %lld (off %lld / %lld); freeBursts %lld", s3bad, (long long) d.planCalls[3],
                                      (long long) d.schedCalls[3], (long long) dOff.planCalls[3], (long long) dOff.schedCalls[3], (long long) d.freeBursts));
    }

    //==========================================================================
    // OB2–OB5 — Tempo extra grain (D18), coincidence, frozen, edges (T4)
    //==========================================================================
    /** Detection lag (samples) of a 2 ms 1 kHz Hann burst at amp, from silence. */
    inline juce::int64 burstLag (double fs, double amp)
    {
        auto x = std::make_shared<std::vector<float>> ((size_t) fs, 0.0f);
        const juce::int64 B = (juce::int64) (0.5 * fs);
        addBurst (*x, fs, (double) B / fs, 2.0, 1000.0, amp);
        Job j = anaJob (fs, 1.0, fromBuffer (x));
        auto L = evInto (j);
        renderAll ({ j });
        return L->at.size() == 1 ? L->at[0] - B : -1;
    }
    /** A burst whose event lands exactly on sample G (lag from burstLag). */
    inline void addBurstEventAt (std::vector<float>& b, double fs, juce::int64 G, juce::int64 lag, double amp)
    {
        addBurst (b, fs, (double) (G - lag) / fs, 2.0, 1000.0, amp);
    }

    inline bool samePlan (const Rec& a, const Rec& b)
    {
        auto eq = [] (const void* x, const void* y, size_t n) { return std::memcmp (x, y, n) == 0; };
        return a.onsetSample == b.onsetSample && eq (&a.frac, &b.frac, 8) && a.globalIndex == b.globalIndex && a.stream == b.stream
            && a.dir == b.dir && a.level == b.level && a.set == b.set && eq (&a.delay, &b.delay, 8) && eq (&a.rate, &b.rate, 8)
            && eq (&a.len, &b.len, 8) && eq (&a.gainL, &b.gainL, 4) && eq (&a.gainR, &b.gainR, 4) && a.windowStep == b.windowStep
            && eq (&a.pitchCents, &b.pitchCents, 4) && a.fxActive == b.fxActive && a.writeHead == b.writeHead;
    }

    void tempoBurstRows()
    {
        std::printf ("\n-- OB2-OB5. onset bursts, Tempo extra grain (D18), coincidence, frozen, edges\n");
        const double fs = 48000.0;

        // OB2 — Tempo 1/16 @ 120, jitter 30 %; s1, s2 trigger on, s3 off, s4 disabled (trigger on).
        {
            auto train = burstTrain (fs, 20.0, 0.3, 0.18, 108, 2.0, 1000.0, 0.5);
            auto job = [&] (bool trig)
            {
                Job j;
                j.params = eg22::base ({ 1, 2, 3 }) + P { { "sync_mode", 1.0f }, { "grain_limit", 3.0f } };
                for (int s = 1; s <= 4; ++s)
                    j.params = j.params + P { { eg22::sp (s, "density_sync"), 7.0f }, { eg22::sp (s, "jitter"), 30.0f },
                                              { eg22::sp (s, "onset_trigger"), trig && s != 3 ? 1.0f : 0.0f } };
                j.len = (juce::int64) (20.0 * fs);
                j.block = 512;
                j.stim = fromBuffer (train);
                eg25::withTransport (j, Transport (fs, 120.0));
                return j;
            };
            Job on = job (true), off = job (false);
            auto Lon = collectInto (on), Loff = collectInto (off);
            auto E = evInto (on);
            struct Trace { std::vector<std::int64_t> slot[4]; std::vector<std::uint64_t> epoch[4]; };
            auto Ton = std::make_shared<Trace>(), Toff = std::make_shared<Trace>();
            auto tr = [] (Job& j, std::shared_ptr<Trace> T)
            {
                auto prev = j.after;
                j.after = [prev, T] (Proc& p, juce::int64 pos)
                {
                    if (prev) prev (p, pos);
                    for (int s = 0; s < 4; ++s)
                    {
                        T->slot[s].push_back (p.getEngineForTest().getSlotNext (s));
                        T->epoch[s].push_back (p.getEngineForTest().getEpoch (s));
                    }
                };
            };
            tr (on, Ton);
            tr (off, Toff);
            renderAll ({ on, off });
            std::vector<Rec> gridOn;
            int burstBad = 0, bursts = 0, other = 0;
            std::uint64_t nextN[4] {};
            for (const auto& r : Lon->r)
            {
                if (! r.burst) { gridOn.push_back (r); continue; }
                ++bursts;
                if (r.stream >= 2) { ++other; continue; }
                const bool atEv = std::find (E->at.begin(), E->at.end(), r.onsetSample) != E->at.end();
                if (! atEv || r.frac != 0.0 || r.globalIndex != (ogran::kBurstBit | nextN[r.stream])) ++burstBad;
                ++nextN[r.stream];
            }
            int gridBad = 0;
            if (gridOn.size() != Loff->r.size()) gridBad = 1 << 20;
            for (size_t k = 0; k < gridOn.size() && k < Loff->r.size(); ++k)
                if (! samePlan (gridOn[k], Loff->r[k])) ++gridBad;
            bool traces = true;
            for (int s = 0; s < 4; ++s) traces = traces && Ton->slot[s] == Toff->slot[s] && Ton->epoch[s] == Toff->epoch[s];
            row (gridBad == 0 && ! Lon->overflow && ! gridOn.empty(), "OB2 Tempo 1/16 @ 120, jitter 30 %: every grid record bitwise == trigger-off render",
                 juce::String::formatted ("%d grid records, %d mismatches", (int) gridOn.size(), gridBad));
            row (burstBad == 0 && bursts == 2 * (int) E->at.size() && other == 0 && ! E->at.empty(),
                 "OB2 bursts at the events, frac 0, globalIndex 2^63 | n (n = 0, 1, 2 ... per stream); s3 / s4 none",
                 juce::String::formatted ("%d bursts / %d events x 2, bad %d, s3/s4 %d", bursts, (int) E->at.size(), burstBad, other));
            row (traces, "OB2 getSlotNext() / getEpoch() traces == trigger-off render (D18: grid untouched)",
                 juce::String::formatted ("%d blocks", (int) Ton->slot[0].size()));
        }

        // OB2b — the VERIFICATION-2.5a item 6 watch: TS1 with s1's trigger on over a burst train.
        {
            struct Sc { const char* name; Transport tr; };
            std::vector<Sc> sc;
            sc.push_back ({ "120", Transport (fs, 120.0) });
            sc.push_back ({ "133", Transport (fs, 133.0) });
            { Transport t (fs, 120.0); t.tempo (100000, 97.3); sc.push_back ({ "step 120->97.3", t }); }
            { Transport t (fs, 120.0); t.loopWrap (150016, 1.0, 6.25); sc.push_back ({ "loop", t }); }
            { Transport t (fs, 133.0); t.locate (150016, 13.37); sc.push_back ({ "locate", t }); }
            sc.push_back ({ "pre-roll -4", Transport (fs, 120.0, -4.0) });
            sc.push_back ({ "3/4", Transport (fs, 120.0, 0.0, true, 3, 4) });
            sc.push_back ({ "7/8", Transport (fs, 133.0, 0.0, true, 7, 8) });
            const int divs[] { 7, 9, 14, 18 };   // 1/16, 1/8 T, 1/4 D, 1 bar
            const juce::int64 kLen = 8 * 48000;
            auto train = burstTrain (fs, 8.0, 0.21, 0.18, 44, 2.0, 1000.0, 0.5);
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg22::RecLog>> logs;
            std::vector<std::pair<int, int>> ids;
            for (int si = 0; si < (int) sc.size(); ++si)
                for (int d : divs)
                {
                    Job j = eg25::tempoJob (sc[(size_t) si].tr, d, kLen);
                    j.stim = fromBuffer (train);
                    j.params = j.params + P { { "s1_onset_trigger", 1.0f } };
                    logs.push_back (collectInto (j));
                    jobs.push_back (j);
                    ids.push_back ({ si, d });
                }
            renderAll (jobs);
            Tally t;
            double worstAll = 0.0;
            int onsets = 0, bursts = 0;
            for (size_t k = 0; k < jobs.size(); ++k)
            {
                const auto ex = eg25::analyticGrid (sc[(size_t) ids[k].first].tr, ids[k].second, kLen);
                std::vector<double> got;
                for (const auto& r : logs[k]->r)
                {
                    if (r.stream != 0) continue;
                    if (r.burst) { ++bursts; continue; }
                    got.push_back ((double) r.onsetSample - r.frac);
                }
                double worst; int missing, extra;
                const bool ok = eg25::matchGrid (got, ex, 1e-6, worst, missing, extra) && ! logs[k]->overflow && ! got.empty();
                worstAll = juce::jmax (worstAll, worst);
                onsets += (int) got.size();
                t.check (ok, juce::String (sc[(size_t) ids[k].first].name) + " / " + GranulatorParamIds::densitySyncs()[ids[k].second]
                                 + juce::String::formatted (": missing %d extra %d worst %.2e", missing, extra, worst));
            }
            row (t.bad == 0 && bursts > 0, "OB2b item 6 watch: TS1 with s1 trigger on over bursts, 4 divisions x 8 transports: grid == analytic",
                 t.detail ((int) jobs.size()) + juce::String::formatted ("  worst %.2e, %d grid onsets, %d bursts", worstAll, onsets, bursts));
        }

        const juce::int64 lag = burstLag (fs, 0.5);
        // OB3 — coincidence: the event on a grid onset sample.
        {
            const juce::int64 G = 6000 * 20;   // a 1/16 grid point at 120 bpm, and a Free onset at 8 g/s
            auto x = std::make_shared<std::vector<float>> ((size_t) (3.0 * fs), 0.0f);
            addBurstEventAt (*x, fs, G, lag, 0.5);
            Job tj;
            tj.params = eg22::base ({ 1 }) + P { { "sync_mode", 1.0f }, { "s1_density_sync", 7.0f }, { "s1_jitter", 0.0f }, { "s1_onset_trigger", 1.0f } };
            tj.len = (juce::int64) (3.0 * fs);
            tj.stim = fromBuffer (x);
            eg25::withTransport (tj, Transport (fs, 120.0));
            Job fj = anaJob (fs, 3.0, fromBuffer (x), eg22::base ({ 1 }) + P { { "s1_density", 8.0f }, { "s1_jitter", 0.0f }, { "s1_onset_trigger", 1.0f } });
            auto Lt = collectInto (tj), Lf = collectInto (fj);
            auto Et = evInto (tj);
            renderAll ({ tj, fj });
            std::vector<const Rec*> atT, atF;
            for (const auto& r : Lt->r) if (r.onsetSample == G) atT.push_back (&r);
            for (const auto& r : Lf->r) if (r.onsetSample == G) atF.push_back (&r);
            const bool evOk = Et->at.size() == 1 && Et->at[0] == G;
            row (evOk && atT.size() == 2 && ! atT[0]->burst && atT[1]->burst, "OB3 event on a Tempo grid sample: 2 grains, grid first, burst second",
                 juce::String::formatted ("event %lld (G %lld), %d grains at G", Et->at.empty() ? -1LL : (long long) Et->at[0], (long long) G, (int) atT.size()));
            row (evOk && atF.size() == 1 && atF[0]->burst, "OB3 event on a Free grid sample: exactly 1 grain (the re-phase; process (i) stops before i)",
                 juce::String::formatted ("%d grains at G", (int) atF.size()));
        }

        // OB4 — frozen: a live burst while frozen spawns with frozen geometry.
        {
            auto x = steppedSine (fs, 3.0, { { 0.0, 233.0 } }, 0.2);
            addBurst (*x, fs, 2.0, 2.0, 1000.0, 0.5);
            Job j = anaJob (fs, 3.0, fromBuffer (x), eg22::base ({ 1 }) + P { { "s1_density", 2.0f }, { "s1_onset_trigger", 1.0f } });
            j.events.push_back ({ 48000, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
            auto L = collectInto (j);
            auto E = evInto (j);
            auto o = renderAll ({ j });
            const auto& fz = o[0].eng().getFreezeForTest();
            int found = 0, ok = 0;
            for (auto e : E->at)
                if (e > 60000)
                    for (const auto& r : L->r)
                        if (r.onsetSample == e && r.burst)
                        {
                            ++found;
                            if (r.writeHead == fz.W0 && r.delay <= (double) fz.heldCaptured + 1e-9 && r.delay > 0.0) ++ok;
                        }
            row (found >= 1 && ok == found, "OB4 frozen + live burst: a grain at the event with frozen geometry (writeHead == W0, delay in heldSpan)",
                 juce::String::formatted ("%d bursts while frozen, %d ok; W0 %lld", found, ok, (long long) fz.W0));
        }

        // OB5 — edges.
        {
            // muted stream fires
            Job m = anaJob (fs, 3.0, fromBuffer (burstTrain (fs, 3.0, 0.3, 0.5, 5, 2.0, 1000.0, 0.5)),
                            eg22::base ({ 1 }) + P { { "s1_mute", 1.0f }, { "s1_onset_trigger", 1.0f }, { "s1_density", 2.0f } });
            auto Lm = collectInto (m);
            // enable edge at tick E + event 10 samples later (same piece); edge + event on the edge sample.
            const juce::int64 Ed = 1600 * 32;
            auto x1 = std::make_shared<std::vector<float>> ((size_t) (3.0 * fs), 0.0f);
            addBurstEventAt (*x1, fs, Ed + 10, lag, 0.5);
            auto x0 = std::make_shared<std::vector<float>> ((size_t) (3.0 * fs), 0.0f);
            addBurstEventAt (*x0, fs, Ed, lag, 0.5);
            auto edgeJob = [&] (std::shared_ptr<std::vector<float>> x, bool trig)
            {
                Job j = anaJob (fs, 3.0, fromBuffer (x), eg22::base ({ 1 }) + P { { "s1_density", 2.0f }, { "s1_jitter", 0.0f },
                                                                                 { "s1_onset_trigger", trig ? 1.0f : 0.0f } });
                j.events.push_back ({ 32 * 300, [] (Proc& p) { eg::setP (p, "s1_enable", 0.0f); } });
                j.events.push_back ({ Ed, [] (Proc& p) { eg::setP (p, "s1_enable", 1.0f); } });
                return j;
            };
            Job e1 = edgeJob (x1, true), e0 = edgeJob (x0, true), e0off = edgeJob (x0, false);
            auto L1 = collectInto (e1), L0 = collectInto (e0), L0off = collectInto (e0off);
            auto o = renderAll ({ m, e1, e0, e0off });
            int mutedBursts = 0;
            for (const auto& r : Lm->r) mutedBursts += r.burst ? 1 : 0;
            row (mutedBursts == 5, "OB5 a muted stream fires (5 bursts)", juce::String::formatted ("%d", mutedBursts));
            std::vector<const Rec*> in1;
            for (const auto& r : L1->r) if (r.onsetSample >= Ed && r.onsetSample < Ed + 32) in1.push_back (&r);
            row (in1.size() == 2 && in1[0]->onsetSample == Ed && ! in1[0]->burst && in1[1]->onsetSample == Ed + 10 && in1[1]->burst
                     && in1[1]->globalIndex == in1[0]->globalIndex + 1,
                 "OB5 enable edge + event in the same piece: 2 grains (reset onset at the piece start, re-phase at the event)",
                 juce::String::formatted ("%d grains in the piece", (int) in1.size()));
            std::vector<const Rec*> in0, in0off;
            for (const auto& r : L0->r) if (r.onsetSample >= Ed && r.onsetSample < Ed + 32) in0.push_back (&r);
            for (const auto& r : L0off->r) if (r.onsetSample >= Ed && r.onsetSample < Ed + 32) in0off.push_back (&r);
            row (in0.size() == 1 && in0off.size() == 1 && in0[0]->onsetSample == Ed && in0[0]->globalIndex == in0off[0]->globalIndex,
                 "OB5 event at i = 0 right after an edge: 1 grain, epoch unchanged (no replay)",
                 juce::String::formatted ("%d grains; index %llu vs trigger-off %llu", (int) in0.size(),
                                          in0.empty() ? 0ull : (unsigned long long) in0[0]->globalIndex,
                                          in0off.empty() ? 0ull : (unsigned long long) in0off[0]->globalIndex));
            std::int64_t mx = 0;
            for (auto& oo : o) mx = juce::jmax (mx, oo.eng().diag.maxEventsPerPiece);
            row (mx == 1, "OB5 maxEventsPerPiece == 1", juce::String (mx));
        }
    }

    //==========================================================================
    // PS1 — Onset source law (R75, P64) (T4)
    //==========================================================================
    void onsetSourceRows()
    {
        std::printf ("\n-- PS1. Onset source: exp (-(T - e)/(0.15 fs)), 0 before, exactly 0 beyond 16 tau\n");
        const double fs = 48000.0;
        auto x = burstTrain (fs, 70.0, 0.5, 0.4, 6, 2.0, 1000.0, 0.5);   // events up to ~2.5 s, then ~67 s of silence
        Job j = anaJob (fs, 70.0, fromBuffer (x));
        j.block = 32;
        auto E = evInto (j);
        auto cells = eg25b::recordSources (j);
        renderAll ({ j });
        const auto& v = cells->src[ogran::ModSrc::Onset];
        int bad = 0, before = 0, retrig = 0, zeros = 0, sub = 0;
        float worstRetrig = 1.0f;
        size_t ei = 0;
        juce::int64 last = -1;
        for (size_t c = 0; c < v.size(); ++c)
        {
            const juce::int64 T = (juce::int64) c * 32;
            bool fresh = false;
            while (ei < E->at.size() && E->at[ei] < T) { last = E->at[ei++]; fresh = true; }
            float want = 0.0f;
            if (last >= 0)
            {
                const double dt = (double) (T - last);
                want = dt > 16.0 * 0.15 * fs ? 0.0f : (float) std::exp (-dt / (0.15 * fs));
            }
            else ++before;
            if (std::memcmp (&want, &v[c], 4) != 0) ++bad;
            if (fresh) { ++retrig; worstRetrig = juce::jmin (worstRetrig, v[c]); }
            if (last >= 0 && T - last > (juce::int64) (16.0 * 0.15 * fs) && v[c] == 0.0f) ++zeros;
            if (v[c] != 0.0f && std::fpclassify (v[c]) == FP_SUBNORMAL) ++sub;
        }
        row (bad == 0 && before > 0 && retrig == (int) E->at.size() && worstRetrig > 0.99f && E->at.size() == 6,
             "PS1 Onset source == (float) exp (-(T - e)/(0.15 fs)) at every tick, bitwise; 0 before the first event; ~1 after each",
             juce::String::formatted ("%d ticks, %d mismatches, %d before, %d re-triggers (min %.4f)", (int) v.size(), bad, before, retrig, (double) worstRetrig));
        row (zeros > 0 && sub == 0, "PS1 exactly 0 beyond 16 tau; ~67 s of silence: no subnormal source value",
             juce::String::formatted ("%d zero ticks, %d subnormal", zeros, sub));
    }

    //==========================================================================
    // OD7 — FUNC-08 click-train hit rate (T4)
    //==========================================================================
    void clickTrainRows()
    {
        std::printf ("\n-- OD7. FUNC-08 click train: 2 ms 1 kHz Hann bursts, IOI 60 / 125 / 250 ms, 30 s, 4 rates\n");
        struct C { double fs, ioi, db; };
        std::vector<C> cs;
        for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
            for (double ioi : { 0.060, 0.125, 0.250 })
                for (double db : { -12.0, -36.0 })
                    cs.push_back ({ fs, ioi, db });
        std::vector<Job> jobs;
        std::vector<std::shared_ptr<EvLog>> logs;
        for (const auto& c : cs)
        {
            const int n = (int) std::floor ((29.8 - 0.1) / c.ioi);
            Job j = anaJob (c.fs, 30.0, fromBuffer (burstTrain (c.fs, 30.0, 0.1, c.ioi, n, 2.0, 1000.0, dbAmp (c.db))), P { { "onset_threshold", -30.0f } });
            logs.push_back (evInto (j));
            jobs.push_back (j);
        }
        renderAll (jobs);
        Tally t, tq;
        int total = 0;
        for (size_t k = 0; k < cs.size(); ++k)
        {
            const auto& c = cs[k];
            const int n = (int) std::floor ((29.8 - 0.1) / c.ioi);
            const auto starts = burstStarts (c.fs, 0.1, c.ioi, n);
            const auto& ev = logs[k]->at;
            if (c.db < -20.0) { tq.check (ev.empty(), fsName (c.fs) + juce::String::formatted (" %d ms: %d events", juce::roundToInt (c.ioi * 1000), (int) ev.size())); continue; }
            int hits = 0, doubles = 0;
            size_t e = 0;
            for (auto st : starts)
            {
                int inWin = 0;
                while (e < ev.size() && ev[e] < st) ++e;
                for (size_t q = e; q < ev.size() && ev[q] - st <= (juce::int64) (0.005 * c.fs); ++q) ++inWin;
                hits += inWin >= 1 ? 1 : 0;
                doubles += inWin > 1 ? 1 : 0;
            }
            total += n;
            t.check (hits == n && doubles == 0 && (int) ev.size() == n, fsName (c.fs) + juce::String::formatted (" %d ms: %d/%d hits, %d doubles, %d events",
                                                                                                      juce::roundToInt (c.ioi * 1000), hits, n, doubles, (int) ev.size()));
        }
        row (t.bad == 0, "OD7 -12 dBFS click train, threshold -30: events == bursts (within 5 ms), 0 double fires", t.detail (12) + juce::String::formatted ("  %d bursts", total));
        row (tq.bad == 0, "OD7 the same trains at -36 dBFS: 0 events", tq.detail (12));
    }

    //==========================================================================
    // MP0–MP4 — decimator + MPM (DSP-08), R59 hop schedule (T5)
    //==========================================================================
    /** The independent double reference of the Mpm pipeline (direct autocorrelation). */
    struct RefMpm { double tau = 0.0, clarity = 0.0, level = 0.0, keyMargin = 1.0; bool peak = false, confident = false; std::vector<double> r, n; };
    inline RefMpm refMpm (const float* w, double fsd)
    {
        constexpr int W = ogran::kMpmWindow;
        RefMpm res;
        double mean = 0.0, sq = 0.0;
        for (int i = 0; i < W; ++i) { mean += (double) w[i]; sq += (double) w[i] * (double) w[i]; }
        mean /= W;
        res.level = sq / W;
        std::vector<double> x ((size_t) W);
        for (int i = 0; i < W; ++i) x[(size_t) i] = (double) w[i] - mean;
        const int tauMin = (int) std::floor (fsd / 1500.0), tauMax = juce::jmin (W - 2, (int) std::ceil (fsd / 40.0) + 1), H = tauMax + 1;
        res.r.assign ((size_t) H + 1, 0.0);
        res.n.assign ((size_t) H + 1, 0.0);
        for (int t = 0; t <= H; ++t)
        {
            double r = 0.0, m = 0.0;
            for (int i = 0; i + t < W; ++i) { r += x[(size_t) i] * x[(size_t) (i + t)]; m += x[(size_t) i] * x[(size_t) i] + x[(size_t) (i + t)] * x[(size_t) (i + t)]; }
            res.r[(size_t) t] = r;
            res.n[(size_t) t] = m > 0.0 ? 2.0 * r / m : 0.0;
        }
        const auto& n = res.n;
        int t = 1;
        while (t < H && n[(size_t) t] > 0.0) ++t;
        std::vector<int> peaks;
        double mx = 0.0;
        while (t < tauMax)
        {
            while (t < H && n[(size_t) t] <= 0.0) ++t;
            int best = t;
            while (t < H && n[(size_t) t] > 0.0) { if (n[(size_t) t] > n[(size_t) best]) best = t; ++t; }
            if (best >= tauMin && best <= tauMax && best > 0 && best < H && n[(size_t) best] > 0.0 && n[(size_t) best + 1] <= n[(size_t) best])
            { peaks.push_back (best); mx = juce::jmax (mx, n[(size_t) best]); }
        }
        for (int p : peaks)
            res.keyMargin = juce::jmin (res.keyMargin, std::abs (n[(size_t) p] - 0.9 * mx));   // distance of a key max to the k = 0.9 cut
        for (int p : peaks)
            if (n[(size_t) p] >= 0.9 * mx)
            {
                const double a = n[(size_t) p - 1], b = n[(size_t) p], c = n[(size_t) p + 1], den = a - 2.0 * b + c;
                const double d = den != 0.0 ? 0.5 * (a - c) / den : 0.0;
                res.tau = p + d;
                res.clarity = b - 0.25 * (a - c) * d;
                res.peak = true;
                break;
            }
        res.confident = res.peak && res.clarity >= 0.8 && res.level >= 1.0e-5;
        return res;
    }

    inline double cents (double note, double hz) { return 100.0 * (note - midiOf (hz)); }

    void mpmRows()
    {
        std::printf ("\n-- MP. decimator + MPM (DSP-08; R68), R59 hop schedule\n");

        // MP0 — the FFT NSDF vs the double direct reference, 100 windows × 2 rates.
        {
            int windows = 0, rBad = 0, tauBad = 0, clBad = 0, decBad = 0, nearThr = 0;
            double worstR = 0.0, worstTau = 0.0, worstCl = 0.0;
            for (double fsd : { 11025.0, 12000.0 })
            {
                auto mpm = std::make_unique<ogran::Mpm>();
                mpm->prepare (fsd);
                std::vector<float> w ((size_t) ogran::kMpmWindow);
                for (int k = 0; k < 50; ++k)
                {
                    const double f0 = 40.0 * std::pow (1500.0 / 40.0, (double) (k % 10) / 9.0) * (1.0 + 0.013 * (k / 10));
                    const int kind = k / 10;   // 0 sine, 1 harmonic tone, 2 noise, 3 sine + noise, 4 two tones
                    for (int i = 0; i < ogran::kMpmWindow; ++i)
                    {
                        const double t = (double) i / fsd;
                        double v = 0.0;
                        if (kind == 0) v = 0.3 * std::sin (2.0 * kPi * f0 * t + 0.3 * k);
                        if (kind == 1) for (int h = 1; h <= 5; ++h) if (f0 * h < 0.45 * fsd) v += 0.3 / h * std::sin (2.0 * kPi * f0 * h * t + 0.7 * h);
                        if (kind == 2) v = 0.2 * gauss (i + 10000 * k, 77);
                        if (kind == 3) v = 0.3 * std::sin (2.0 * kPi * f0 * t) + 0.05 * gauss (i + 10000 * k, 78);
                        if (kind == 4) v = 0.2 * std::sin (2.0 * kPi * f0 * t) + 0.15 * std::sin (2.0 * kPi * f0 * 1.5 * t + 1.0);
                        w[(size_t) i] = (float) v;
                    }
                    const auto e = mpm->analyse (w.data());
                    const auto r = refMpm (w.data(), fsd);
                    ++windows;
                    const float* er = mpm->autocorr();
                    double dr = 0.0;
                    for (int t = 0; t <= mpm->getTauMax(); ++t) dr = juce::jmax (dr, std::abs ((double) er[t] - r.r[(size_t) t]));
                    const double rel = dr / juce::jmax (1e-30, r.r[0]);
                    worstR = juce::jmax (worstR, rel);
                    if (rel > 1e-4) ++rBad;
                    const bool near = std::abs (r.clarity - 0.8) < 1e-4 || std::abs (r.level - 1e-5) < 1e-9 || r.keyMargin < 1e-4;
                    if (near) { ++nearThr; continue; }
                    if (e.peak != r.peak) { ++tauBad; continue; }
                    if (r.peak)
                    {
                        worstTau = juce::jmax (worstTau, std::abs (e.tau - r.tau));
                        worstCl = juce::jmax (worstCl, std::abs (e.clarity - r.clarity));
                        if (std::abs (e.tau - r.tau) > 1e-3) ++tauBad;
                        if (std::abs (e.clarity - r.clarity) > 1e-4) ++clBad;
                    }
                    if (e.confident != r.confident) ++decBad;
                }
            }
            row (rBad == 0 && tauBad == 0 && clBad == 0 && decBad == 0,
                 "MP0 FFT NSDF vs double direct autocorrelation, 100 windows at fs_d 11025 / 12000: r, tau, clarity, decisions",
                 juce::String::formatted ("%d windows: max|dr|/r0 %.2e, tau %.2e, clarity %.2e; bad r %d tau %d cl %d dec %d; %d near a threshold",
                                          windows, worstR, worstTau, worstCl, rBad, tauBad, clBad, decBad, nearThr));
        }

        // MP1 / MP1i / MP1h — accuracy.
        {
            struct C { double fs, hz; int kind; };   // kind 0 sine, 1 sawtooth (partials to 20 kHz)
            std::vector<C> cs;
            for (double fs : { 44100.0, 48000.0, 96000.0 })
                for (double hz : { 55.0, 82.41, 110.0, 220.0, 440.0, 700.0, 1000.0, 40.0, 1500.0 })
                    cs.push_back ({ fs, hz, 0 });
            for (double fs : { 176400.0, 192000.0 })
            {
                cs.push_back ({ fs, 220.0, 0 });
                cs.push_back ({ fs, 220.0, 1 });
                cs.push_back ({ fs, 1000.0, 0 });
            }
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<HopLog>> logs;
            for (const auto& c : cs)
            {
                std::shared_ptr<std::vector<float>> x;
                if (c.kind == 0) x = steppedSine (c.fs, 0.6, { { 0.0, c.hz } }, 0.25);
                else
                {
                    x = std::make_shared<std::vector<float>> ((size_t) (0.6 * c.fs), 0.0f);
                    for (size_t i = 0; i < x->size(); ++i)
                    {
                        double v = 0.0;
                        for (int h = 1; c.hz * h < 20000.0; ++h) v += std::sin (2.0 * kPi * c.hz * h * (double) i / c.fs) / h;
                        (*x)[i] = (float) (0.25 * 2.0 / kPi * v);
                    }
                }
                Job j = anaJob (c.fs, 0.6, fromBuffer (x));
                logs.push_back (hopsInto (j));
                jobs.push_back (j);
            }
            renderAll (jobs);
            Tally t;
            double worst = 0.0;
            juce::String edges, high;
            for (size_t k = 0; k < cs.size(); ++k)
            {
                const auto& c = cs[k];
                int conf = 0, n = 0;
                double w = 0.0;
                for (const auto& h : logs[k]->h)
                {
                    if (h.index < 1) continue;
                    ++n;
                    if (h.confident) { ++conf; w = juce::jmax (w, std::abs (cents (h.rawNote, c.hz))); }
                }
                const bool gated = c.fs <= 96000.0 && c.hz >= 55.0 && c.hz <= 1000.0;
                if (gated)
                {
                    worst = juce::jmax (worst, w);
                    t.check (conf == n && n > 20 && w <= 10.0, fsName (c.fs) + juce::String::formatted (" %.2f Hz: %d/%d confident, worst %.2f c", c.hz, conf, n, w));
                }
                else if (c.fs <= 96000.0)
                    edges << fsName (c.fs) << " " << juce::String (c.hz, 0) << " Hz " << conf << "/" << n << " " << juce::String (w, 2) << " c; ";
                else
                    high << fsName (c.fs) << " " << (c.kind ? "saw " : "sine ") << juce::String (c.hz, 0) << " Hz " << conf << "/" << n << " confident, worst "
                         << juce::String (w, 2) << " c; ";
            }
            row (t.bad == 0, "MP1 DSP-08 sines -12 dBFS 55-1000 Hz x 44.1 / 48 / 96 k: every hop confident, worst <= 10 c",
                 t.detail (21) + juce::String::formatted ("  worst %.2f c (expected <= 2.8; > 3 investigated)", worst));
            info ("MP1i 40 / 1500 Hz edges: " + edges.trim());
            info ("MP1h 176.4 / 192 k (64-tap decimator, P65 watch): " + high.trim());
        }

        // MP2 — clarity / gate.
        {
            const double fs = 48000.0;
            struct C { const char* name; std::function<float (juce::int64)> f; bool wantConf; };
            auto brown = std::make_shared<std::vector<float>> ((size_t) fs);
            {
                double y = 0.0, acc2 = 0.0;
                for (size_t i = 0; i < brown->size(); ++i) { y = 0.999 * y + gauss ((juce::int64) i, 91); (*brown)[i] = (float) y; acc2 += y * y; }
                const double g = 0.1 / std::sqrt (acc2 / (double) brown->size());
                for (auto& v : *brown) v = (float) (v * g);
            }
            std::vector<C> cs {
                { "silence", [] (juce::int64) { return 0.0f; }, false },
                { "white -20 dBFS", [] (juce::int64 i) { return (float) (0.1 * gauss (i, 11)); }, false },
                { "white -60 dBFS", [] (juce::int64 i) { return (float) (0.001 * gauss (i, 12)); }, false },
                { "brown -20 dBFS", [brown] (juce::int64 i) { return (*brown)[(size_t) i]; }, false },
                { "sine -55 dBFS", [fs] (juce::int64 i) { return (float) (dbAmp (-55.0) * std::sin (2.0 * kPi * 220.0 * (double) i / fs)); }, false },
                { "sine -45 dBFS", [fs] (juce::int64 i) { return (float) (dbAmp (-45.0) * std::sin (2.0 * kPi * 220.0 * (double) i / fs)); }, true },
            };
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<HopLog>> logs;
            for (const auto& c : cs)
            {
                auto f = c.f;
                Job j = anaJob (fs, 1.0, [f] (juce::int64 i, int) { return f (i); });
                logs.push_back (hopsInto (j));
                jobs.push_back (j);
            }
            renderAll (jobs);
            Tally t;
            juce::String d;
            for (size_t k = 0; k < cs.size(); ++k)
            {
                int conf = 0, n = 0;
                double mc = 0.0;
                for (const auto& h : logs[k]->h) { if (h.index < 1) continue; ++n; conf += h.confident ? 1 : 0; mc = juce::jmax (mc, h.clarity); }
                t.check (cs[k].wantConf ? conf == n && n > 0 : conf == 0, juce::String (cs[k].name) + juce::String::formatted (": %d/%d", conf, n));
                d << cs[k].name << " " << conf << "/" << n << " (max clarity " << juce::String (mc, 3) << "); ";
            }
            row (t.bad == 0, "MP2 0 confident on silence, white -20 / -60, brown -20, sine -55 dBFS; sine -45 dBFS confident", t.detail ((int) cs.size()));
            info ("MP2 " + d.trim());
        }

        // MP4 — hop schedule (absolute; identical at blocks 32 / 441 / 4096); centres.
        {
            const double fs = 48000.0;
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<HopLog>> logs;
            for (int b : { 32, 441, 4096 })
            {
                Job j = anaJob (fs, 3.0, compositeStim (fs));
                j.block = b;
                logs.push_back (hopsInto (j));
                jobs.push_back (j);
            }
            renderAll (jobs);
            const auto dm = ogran::AnalysisDims::compute (fs);
            int bad = 0, same = 0;
            for (const auto& h : logs[0]->h)
            {
                const juce::int64 aWant = (1023 + 128 * h.index) * dm.D;
                const juce::int64 tick = (h.aHop / 32 + 1) * 32;
                if (h.aHop != aWant || h.commitAt != tick) ++bad;
                if (h.decided >= 0 && h.decidedCentre != (double) ((1023 + 128 * h.decided) * dm.D) - dm.lagCentre) ++bad;
            }
            auto eqLog = [] (const HopLog& a, const HopLog& b)
            {
                if (a.h.size() != b.h.size()) return false;
                for (size_t k = 0; k < a.h.size(); ++k)
                    if (a.h[k].index != b.h[k].index || a.h[k].aHop != b.h[k].aHop || a.h[k].commitAt != b.h[k].commitAt
                        || std::memcmp (&a.h[k].rawNote, &b.h[k].rawNote, 4) != 0 || std::memcmp (&a.h[k].decidedNote, &b.h[k].decidedNote, 4) != 0
                        || a.h[k].confident != b.h[k].confident || a.h[k].decided != b.h[k].decided)
                        return false;
                return true;
            };
            same = (eqLog (*logs[0], *logs[1]) ? 1 : 0) + (eqLog (*logs[0], *logs[2]) ? 1 : 0);
            row (bad == 0 && same == 2 && logs[0]->h.size() > 200, "MP4 hops at m.D for (m + 1) % 128 == 0, m >= 1023; committed at the next tick; centres a_hop - (511.5 D + 31.5)",
                 juce::String::formatted ("%d hops, %d bad; hopLog identical at 441 / 4096 vs 32: %d/2", (int) logs[0]->h.size(), bad, same));
        }
    }

} } // namespace eg25x

static void engineRows25b()
{
    std::printf ("\n== Stage 2 / 2.5b analysis rows (onset, bursts, sources, MPM, D20)\n");
    eg25x::detectorRows();
    eg25x::freeBurstRows();
    eg25x::tempoBurstRows();
    eg25x::onsetSourceRows();
    eg25x::clickTrainRows();
    eg25x::mpmRows();
}
