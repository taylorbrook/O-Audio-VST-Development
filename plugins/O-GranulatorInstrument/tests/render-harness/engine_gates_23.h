/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.3 engine gates
    (PLAN-2.3 T7, the non-parity rows). Included by main.cpp AFTER
    engine_gates_22.h and fx_units_23.h (uses eg21 / eg22 helpers and
    rh::computeAll).

    Every row prints: ID, measured value, threshold, margin and its negative
    control ("ctrl FIRED: ..." / "ctrl NOT FIRED (vacuous): ..."); a row whose
    control does not fire FAILS.

      FN1  stage chunk invariance {1, 3, 17, 32} vs 32      ctrl per-call ring renorm, chunks 17
      FN2  engine block-size null, all FX                   ctrl seed + 1
      FN3  108 unread params bit-identical (readSet23)      ctrl s1_filter_cutoff / s4_ring_freq
      FR1  FX-off ≡ renders-2.2 (14 jobs)                   ctrl N1-default + s1_drive 1 %
      FV1a cutoff census (+ 44.1 k clamp)                   ctrl rand 50 % -> span < 3.6
      FV1b ring census (+ 44.1 k clamp)                     ctrl rand 50 % -> span < 3.6
      FV1c crush Bernoulli                                  ctrl p = 0.3 test on the 50 % log
      FV1d fxActive gate table + live-pool defaults         ctrl wrong table
      FV2  BP centroid vs latched fc (Spearman)             ctrl fxSharedLatch
      FV3  ring USB carrier vs latched ringHz               ctrl fxSharedLatch
      FV4  crush out-of-band split vs latched crushed       ctrl fxSharedLatch
      FB1  no bleed vs a fresh pool (poisonFreeSlots)       ctrl poison + fxNoReset
      FB2  slot reuse after a resonant grain                ctrl fxNoReset
      FB3  steal copy keeps FX latch + state                ctrl stealSnapPerturb
      FB4  FX state == Grain {} at spawn                    ctrl poison + fxNoReset
      FZ1  stress 44.1 / 48 / 96 k, finite, no subnormals   ctrl injectNan + nanKillOff; scanner denorm_min
      FZ2  NaN kill (13 cases) == forceRetire render        ctrl nanKillOff
      FA3  0 audio-thread allocations, all FX               ctrl liveness malloc(64)
    cpu23(): D19 / D20 verdicts with the PD2 machine-speed witness (--cpu23).

  ==============================================================================
*/
#pragma once

#include <complex>

namespace { namespace eg23
{
    using eg21::Session;
    using eg21::P;
    using eg21::Log;
    using eg21::with;
    using eg21::setP;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::sameF;
    using eg21::noiseBuf;
    using eg21::sineBuf;
    using eg21::diffCount;
    using eg21::kSeed;
    using eg22::base4;
    using eg22::all4;
    using eg22::cat;
    using eg22::sn;
    using eg22::shipping;
    using eg22::loadNoise4;
    using eg22::setAt;
    using eg22::lr;

    constexpr double kFs = 48000.0;
    constexpr double kPi = 3.14159265358979323846;
    using Proc = GranulatorInstrumentProcessor;

    void section (const char* t) { std::printf ("\n-- %s\n", t); }

    P s1p (std::initializer_list<std::pair<const char*, float>> kv)
    {
        P p;
        for (const auto& x : kv)
            p.push_back ({ sn (1, x.first), x.second });
        return p;
    }

    //==========================================================================
    // Spectra
    void fft (std::vector<std::complex<double>>& a)
    {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap (a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = -2.0 * kPi / (double) len;
            const std::complex<double> wl (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < n; i += len)
            {
                std::complex<double> w (1.0, 0.0);
                for (size_t k = 0; k < len / 2; ++k)
                {
                    const auto u = a[i + k], v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
    }

    /** Grains whose [onset, onset + len + 2) touches no other grain's span and lies inside the render. */
    Log isolated (const Log& recs, size_t total)
    {
        Log out;
        for (size_t k = 0; k < recs.size(); ++k)
        {
            const auto& r = recs[k];
            const juce::int64 a = r.onsetSample, b = a + (juce::int64) std::ceil (r.len) + 2;
            const bool iso = (k == 0 || recs[k - 1].onsetSample + (juce::int64) std::ceil (recs[k - 1].len) + 2 <= a)
                          && (k + 1 >= recs.size() || recs[k + 1].onsetSample >= b);
            if (iso && b < (juce::int64) total) out.push_back (r);
        }
        return out;
    }

    std::vector<double> grainSpectrum (const std::vector<float>& L, const ogi::GrainLogEntry& r, size_t N)
    {
        std::vector<std::complex<double>> x (N);
        const juce::int64 a = r.onsetSample, b = a + (juce::int64) std::ceil (r.len) + 2;
        for (juce::int64 i = a; i < b && (size_t) (i - a) < N && i < (juce::int64) L.size(); ++i)
            x[(size_t) (i - a)] = (double) L[(size_t) i];
        fft (x);
        std::vector<double> p (N / 2);
        for (size_t k = 0; k < N / 2; ++k) p[k] = std::norm (x[k]);
        return p;
    }

    double peakHz (const std::vector<double>& p, double fs, size_t N, double loHz, double hiHz)
    {
        const double bin = fs / (double) N;
        size_t kb = (size_t) (loHz / bin) + 1;
        for (size_t k = kb; k < (size_t) (hiHz / bin) && k + 1 < p.size(); ++k) if (p[k] > p[kb]) kb = k;
        const double y0 = std::log (p[kb - 1] + 1e-300), y1 = std::log (p[kb] + 1e-300), y2 = std::log (p[kb + 1] + 1e-300);
        const double den = y0 - 2.0 * y1 + y2;
        return ((double) kb + (std::abs (den) > 0 ? 0.5 * (y0 - y2) / den : 0.0)) * bin;
    }

    std::vector<double> ranks (const std::vector<double>& v)
    {
        std::vector<size_t> idx (v.size());
        for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
        std::sort (idx.begin(), idx.end(), [&v] (size_t a, size_t b) { return v[a] < v[b]; });
        std::vector<double> r (v.size());
        for (size_t i = 0; i < idx.size();)
        {
            size_t j = i;
            while (j + 1 < idx.size() && v[idx[j + 1]] == v[idx[i]]) ++j;
            for (size_t k = i; k <= j; ++k) r[idx[k]] = 0.5 * (double) (i + j);
            i = j + 1;
        }
        return r;
    }

    double spearman (const std::vector<double>& a, const std::vector<double>& b)
    {
        const auto ra = ranks (a), rb = ranks (b);
        const double n = (double) a.size();
        double ma = 0, mb = 0;
        for (size_t i = 0; i < a.size(); ++i) { ma += ra[i]; mb += rb[i]; }
        ma /= n; mb /= n;
        double sab = 0, saa = 0, sbb = 0;
        for (size_t i = 0; i < a.size(); ++i)
        {
            sab += (ra[i] - ma) * (rb[i] - mb);
            saa += (ra[i] - ma) * (ra[i] - ma);
            sbb += (rb[i] - mb) * (rb[i] - mb);
        }
        return sab / std::sqrt (std::max (1e-300, saa * sbb));
    }

    //==========================================================================
    // FN1 — stage chunk invariance (unit level)
    using ogi::Grain;

    ogi::GiGridParams::Stream fxStream (int type, float cutHz, float res, float drive, float crush, float ringHz, float ringMix)
    {
        ogi::GiGridParams::Stream sp;
        sp.filterType = type; sp.filterCutoffHz = cutHz; sp.filterRes = res; sp.drive = drive;
        sp.crush = crush; sp.crushProb = 1.0f; sp.ringFreqHz = ringHz; sp.ringMix = ringMix;
        return sp;
    }

    /** The FN1n control: ring rotation with the Newton renormalisation once per CALL. */
    void ringRenormPerCall (Grain& g, float* l, float* r, int n)
    {
        const float rr = g.ringRotRe, ri = g.ringRotIm, mix = g.ringMix, dry = 1.0f - mix;
        float re = g.ringRe, im = g.ringIm;
        for (int i = 0; i < n; ++i)
        {
            const float m = dry + mix * re;
            l[i] *= m;
            r[i] *= m;
            const float nr = re * rr - im * ri, ni = re * ri + im * rr;
            re = nr; im = ni;
        }
        const float s = 1.5f - 0.5f * (re * re + im * im);
        g.ringRe = re * s;
        g.ringIm = im * s;
    }

    /** Runs one stage over `x` (stereo, interleaved planes) in chunks of `chunk`. */
    std::vector<float> runStage (const ogi::GiGridParams::Stream& sp, double fs, int which, int chunk, bool perCallRing = false)
    {
        ogi::FxPlan p;
        ogi::planFx (p, sp, fs, [] (std::uint32_t) { return 0.5; });
        Grain g {};
        ogi::assignFxLatch (g, p);
        const int n = (int) (2.0 * fs);
        std::vector<float> L ((size_t) n), R ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            L[(size_t) i] = parity::hashNoise (i, 0, 0.5f, 0x51);
            R[(size_t) i] = parity::hashNoise (i, 1, 0.5f, 0x51);
        }
        juce::ScopedNoDenormals nd;
        for (int a = 0; a < n; a += chunk)
        {
            const int m = std::min (chunk, n - a);
            float* l = L.data() + a;
            float* r = R.data() + a;
            switch (which)
            {
                case 0: ogi::fx::svf (g, l, r, m); break;
                case 1: ogi::fx::drive (g, l, r, m); break;
                case 2: ogi::fx::crush (g, l, r, m); break;
                default: if (perCallRing) ringRenormPerCall (g, l, r, m); else ogi::fx::ring (g, l, r, m); break;
            }
        }
        L.insert (L.end(), R.begin(), R.end());
        return L;
    }

    void rowFN1()
    {
        struct Case { const char* name; ogi::GiGridParams::Stream sp; int which; };
        const double nyq = 0.45 * kFs;
        std::vector<Case> cases;
        for (int t = 1; t <= 3; ++t)
        {
            cases.push_back ({ t == 1 ? "LP res 100 @ 1 kHz" : t == 2 ? "BP res 100 @ 1 kHz" : "HP res 100 @ 1 kHz", fxStream (t, 1000.0f, 1.0f, 0, 0, 200, 0), 0 });
            cases.push_back ({ t == 1 ? "LP res 100 @ 0.45 fs" : t == 2 ? "BP res 100 @ 0.45 fs" : "HP res 100 @ 0.45 fs", fxStream (t, 20000.0f, 1.0f, 0, 0, 200, 0), 0 });
        }
        cases.push_back ({ "drive 100", fxStream (0, 20000, 0.2f, 1.0f, 0, 200, 0), 1 });
        cases.push_back ({ "crush 100", fxStream (0, 20000, 0.2f, 0, 1.0f, 200, 0), 2 });
        cases.push_back ({ "crush 10", fxStream (0, 20000, 0.2f, 0, 0.1f, 200, 0), 2 });
        cases.push_back ({ "ring 5 Hz", fxStream (0, 20000, 0.2f, 0, 0, 5.0f, 1.0f), 3 });
        cases.push_back ({ "ring 0.45 fs", fxStream (0, 20000, 0.2f, 0, 0, (float) nyq, 1.0f), 3 });
        Tally t;
        int total = 0;
        for (const auto& c : cases)
        {
            const auto ref = runStage (c.sp, kFs, c.which, 32);
            for (int ch : { 1, 3, 17 })
            {
                const int d = diffCount (ref, runStage (c.sp, kFs, c.which, ch));
                ++total;
                t.check (d == 0, juce::String (c.name) + " chunk " + juce::String (ch) + ": " + juce::String (d));
            }
        }
        const auto rRef = runStage (fxStream (0, 20000, 0.2f, 0, 0, 5.0f, 1.0f), kFs, 3, 32, true);
        const int dC = diffCount (rRef, runStage (fxStream (0, 20000, 0.2f, 0, 0, 5.0f, 1.0f), kFs, 3, 17, true));
        const bool fired = dC > 0;
        row (t.bad == 0 && fired, "FN1 per-stage chunk invariance (chunks 1 / 3 / 17 vs 32, 11 extremes)",
             "meas " + t.detail (total) + " bitwise | thr 0 differ | margin " + juce::String (-t.bad)
                 + " | " + ctrl (fired, "per-call ring renorm, chunk 17 vs 32 -> " + juce::String (dC) + " differ"));
    }

    //==========================================================================
    // FN2 / FN3 — engine determinism with every FX on
    P fxAll()
    {
        P p;
        const float types[4] { 1.0f, 2.0f, 3.0f, 0.0f };
        for (int n = 1; n <= 4; ++n)
            p = with (p, { { sn (n, "filter_type"), types[n - 1] }, { sn (n, "filter_cutoff"), 1500.0f }, { sn (n, "filter_res"), 80.0f },
                           { sn (n, "filter_rand"), 50.0f }, { sn (n, "drive"), 40.0f }, { sn (n, "crush"), 30.0f },
                           { sn (n, "crush_prob"), 50.0f }, { sn (n, "ring_freq"), 300.0f }, { sn (n, "ring_mix"), 50.0f },
                           { sn (n, "ring_rand"), 50.0f } });
        return p;
    }

    P fn2Params() { return cat ({ eg22::nb1Params(), fxAll() }); }

    std::vector<float> renderFN (int block, std::uint64_t seed, const std::function<void (Proc&)>& tweak, bool typeSwitch)
    {
        Session s (kFs, fn2Params(), seed, 512);
        shipping (s);
        if (tweak)
            tweak (*s.p);
        loadNoise4 (s, 3.0, 0.3f);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.noteOn (5003, 64);
        s.noteOn (10007, 67);
        s.noteOff (30011, 52);
        s.noteOff (44000, 48);
        if (typeSwitch)
            setAt (s, 20000, sn (2, "filter_type"), 3.0f);   // BP -> HP at a multiple of 32
        s.run (64000, block);
        return lr (s);
    }

    void rowFN2()
    {
        const auto ref = renderFN (32, kSeed, {}, true);
        double e = 0.0;
        for (float x : ref) e += (double) x * (double) x;
        const bool audible = std::sqrt (e / (double) ref.size()) > 1.0e-4;
        Tally t;
        juce::String d;
        int worst = 0;
        for (int b : { 441, 512, 4096, 8192, 0 })
        {
            const int n = diffCount (ref, renderFN (b, kSeed, {}, true));
            worst = std::max (worst, n);
            t.check (n == 0, (b == 0 ? juce::String ("var") : juce::String (b)) + " " + juce::String (n));
            d << " " << (b == 0 ? juce::String ("var") : juce::String (b)) << ":" << n;
        }
        const int nSeed = diffCount (ref, renderFN (32, kSeed + 1, {}, true));
        const bool fired = nSeed > 0;
        row (t.bad == 0 && audible && fired, "FN2 engine block-size null, 4 streams, every FX + filter_type switch",
             "meas differing samples vs 32:" + d + " | thr 0 | margin " + juce::String (-worst)
                 + " | " + ctrl (fired, "seed + 1 -> " + juce::String (nSeed) + " differ"));
    }

    void rowFN3()
    {
        const auto readSet = eg21::readSet23();
        int bound = 0;
        {
            auto probe = make();
            for (const auto& id : readSet)
                bound += param (*probe, id) != nullptr ? 1 : 0;
        }
        int touched = 0;
        auto unread = [&readSet, &touched] (Proc& p)
        {
            juce::Random r ((juce::int64) 0x4E230003);
            touched = 0;
            for (auto* ap : p.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                    if (! readSet.contains (rp->getParameterID()))
                    {
                        rp->setValueNotifyingHost (r.nextFloat());
                        ++touched;
                    }
        };
        const auto ref = renderFN (512, kSeed, {}, false);
        const int n = diffCount (ref, renderFN (512, kSeed, unread, false));
        const int nC = diffCount (ref, renderFN (512, kSeed, [] (Proc& p) { setP (p, sn (1, "filter_cutoff"), 3000.0f); }, false));
        const int nR = diffCount (ref, renderFN (512, kSeed, [] (Proc& p) { setP (p, sn (4, "ring_freq"), 900.0f); }, false));
        const bool fired = nC > 0 && nR > 0;
        row (n == 0 && bound == 153 && touched == 108 && fired, "FN3 unread params bit-identical, 4 streams, every FX on (readSet23)",
             "meas " + juce::String (n) + " samples differ with " + juce::String (touched) + " unread params randomised ("
                 + juce::String (bound) + "/153 read IDs bound) | thr 0 | margin " + juce::String (-n)
                 + " | " + ctrl (fired, "s1_filter_cutoff -> " + juce::String (nC) + ", s4_ring_freq -> " + juce::String (nR) + " differ"));
    }

    //==========================================================================
    // FR1 — FX off ≡ renders-2.2
    void rowFR1()
    {
        juce::StringArray gold;
        gold.addLines (gGoldenDir.getChildFile ("renders-2.2.sha256").loadFileAsString());
        gold.removeEmptyStrings();
        const auto now = rh::computeAll();
        int same = 0;
        for (const auto& l : now)
            same += gold.contains (l) ? 1 : 0;
        // Control: N1-default with s1_drive 1 % (the drive gate opens at 0.25 %).
        juce::String hC;
        {
            const P over { { "polyphony", 4.0f }, { "grain_limit", 1.0f }, { eg21::s1 ("density"), 100.0f }, { eg21::s1 ("size"), 300.0f },
                           { eg21::s1 ("size_rand"), 50.0f }, { eg21::s1 ("scan_rate"), 100.0f }, { eg21::s1 ("scan_mode"), 1.0f },
                           { "amp_attack", 5.0f }, { "amp_release", 200.0f }, { eg21::s1 ("drive"), 1.0f } };
            Session s (48000.0, rh::stim (over, false), kSeed, 512);
            rh::prepareJob (s, false);
            s.load (0, noiseBuf (2, 3 * 48000, 0.3f, 7), 48000.0);
            eg21::n1Script (s);
            s.run (96000, 512);
            hC = rh::sha (rh::take (s));
        }
        juce::String hN1;
        for (const auto& l : gold)
            if (l.startsWith ("N1-default ")) hN1 = l.fromLastOccurrenceOf (" ", false, false);
        const bool fired = hC.isNotEmpty() && hN1.isNotEmpty() && hC != hN1;
        row (gold.size() == 14 && now.size() == 14 && same == 14 && fired, "FR1 FX-off renders == renders-2.2.sha256 (14 jobs, true bypass)",
             "meas " + juce::String (same) + "/14 lines identical | thr 14 | margin " + juce::String (same - 14)
                 + " | " + ctrl (fired, "N1-default + s1_drive 1 % -> hash differs"));
    }

    //==========================================================================
    // FV1 — latched-value census (grain log)
    Log censusLog (double fs, double secs, const P& extra, float density = 100.0f, int streams = 1, float* baseCut = nullptr, float* baseRing = nullptr)
    {
        P p = cat ({ base4 (streams), { { "grain_limit", 3.0f } } });
        for (int n = 1; n <= streams; ++n)
            p = with (p, { { sn (n, "density"), density }, { sn (n, "size"), density > 100.0f ? 20.0f : 50.0f } });
        p = with (p, extra);
        Session s (fs, p, kSeed, 512);
        s.record = false;
        for (int k = 0; k < streams; ++k)
            s.load (k, noiseBuf (2, (int) (4.0 * fs), 0.2f, 21 + k), fs);
        s.noteOn (0, 48);
        s.run ((juce::int64) (secs * fs), 512);
        const auto& st = s.eng().getGridParams().s[0];
        if (baseCut != nullptr) *baseCut = st.filterCutoffHz;
        if (baseRing != nullptr) *baseRing = st.ringFreqHz;
        return s.hk().logInOrder();
    }

    void rowsFV1ab()
    {
        for (int which = 0; which < 2; ++which)
        {
            const char* rnd = which == 0 ? "filter_rand" : "ring_rand";
            const P on = which == 0 ? s1p ({ { "filter_type", 1.0f }, { "filter_cutoff", 1000.0f } })
                                    : s1p ({ { "ring_mix", 100.0f }, { "ring_freq", 200.0f } });
            auto val = [which] (const ogi::GrainLogEntry& r) { return which == 0 ? r.fcHz : r.ringHz; };
            auto span = [&] (float randPct, int& n, int& out, double& lo, double& hi)
            {
                float bc = 0, br = 0;
                const auto r1 = censusLog (kFs, 3.0, with (on, { { sn (1, rnd), randPct } }), 100.0f, 1, &bc, &br);
                const float base = which == 0 ? bc : br;
                lo = 1e9; hi = -1e9; out = 0; n = (int) r1.size();
                for (const auto& r : r1)
                {
                    const double x = std::log2 ((double) val (r) / (double) base);
                    lo = std::min (lo, x); hi = std::max (hi, x);
                    out += (x < -2.0 - 1e-6 || x > 2.0 + 1e-6) ? 1 : 0;
                }
            };
            int n1 = 0, out1 = 0, nC = 0, outC = 0;
            double lo1 = 0, hi1 = 0, loC = 0, hiC = 0;
            span (100.0f, n1, out1, lo1, hi1);
            span (50.0f, nC, outC, loC, hiC);
            float b0c = 0, b0r = 0;
            const auto r0 = censusLog (kFs, 1.0, with (on, { { sn (1, rnd), 0.0f } }), 100.0f, 1, &b0c, &b0r);
            const float base0 = which == 0 ? b0c : b0r;
            int exact = 0;
            for (const auto& r : r0) exact += sameF (val (r), base0) ? 1 : 0;
            const P pc = which == 0 ? s1p ({ { "filter_type", 1.0f }, { "filter_cutoff", 20000.0f }, { rnd, 100.0f } })
                                    : s1p ({ { "ring_mix", 100.0f }, { "ring_freq", 5000.0f }, { rnd, 100.0f } });
            const auto rc = which == 0 ? censusLog (44100.0, 2.0, pc) : censusLog (44100.0, 12.0, pc, 200.0f);
            const float clampV = (float) (ogi::kFxNyquist * 44100.0);
            int atClamp = 0, above = 0;
            for (const auto& r : rc)
            {
                const float v = val (r);
                above += v > clampV ? 1 : 0;
                atClamp += sameF (v, clampV) ? 1 : 0;
            }
            const bool fired = hiC - loC < 3.6;
            const bool ok = n1 >= 200 && out1 == 0 && hi1 - lo1 >= 3.6 && ! r0.empty() && exact == (int) r0.size() && atClamp > 0 && above == 0;
            row (ok && fired, which == 0 ? "FV1a cutoff census: +-2 oct, span >= 3.6; rand 0 bitwise base; 44.1 k clamp 0.45 fs exact"
                                         : "FV1b ring census: +-2 oct, span >= 3.6; rand 0 bitwise base; 44.1 k clamp 0.45 fs exact",
                 "meas " + juce::String (n1) + " grains, log2 span [" + f2 (lo1) + ", " + f2 (hi1) + "] = " + f2 (hi1 - lo1) + ", " + juce::String (out1)
                     + " out; rand 0 " + juce::String (exact) + "/" + juce::String ((int) r0.size()) + " == base; clamp " + juce::String (atClamp)
                     + " at " + f2 (clampV, 1) + ", " + juce::String (above) + " above (" + juce::String ((int) rc.size()) + " grains @ 44.1 k)"
                     + " | thr span >= 3.6, 0 out | margin " + f2 (hi1 - lo1 - 3.6) + " | " + ctrl (fired, "rand 50 % -> span " + f2 (hiC - loC)));
        }
    }

    void rowFV1c()
    {
        auto run = [] (float prob, int& n, int& crushed, int& bitSet)
        {
            P extra;
            for (int s = 1; s <= 4; ++s)
                extra = with (extra, { { sn (s, "crush"), 50.0f }, { sn (s, "crush_prob"), prob } });
            const auto lg = censusLog (kFs, 5.0, extra, 100.0f, 4);
            n = (int) lg.size(); crushed = 0; bitSet = 0;
            for (const auto& r : lg) { crushed += r.crushed ? 1 : 0; bitSet += (r.fxActive & ogi::kFxCrush) ? 1 : 0; }
        };
        int n0, c0, b0, n1, c1, b1, n5, c5, b5;
        run (0.0f, n0, c0, b0); run (100.0f, n1, c1, b1); run (50.0f, n5, c5, b5);
        const double z = ((double) c5 - 0.5 * n5) / std::sqrt (0.25 * n5);
        const double zc = ((double) c5 - 0.3 * n5) / std::sqrt (0.21 * n5);
        const bool fired = std::abs (zc) > 3.29;
        row (c0 == 0 && b0 == 0 && c1 == n1 && b1 == n1 && n5 >= 2000 && std::abs (z) <= 3.29 && b5 == c5 && fired,
             "FV1c crush Bernoulli @ crush 50 %: prob 0 none, 100 % all, 50 % |z| <= 3.29 (>= 2000 spawns)",
             "meas p0 " + juce::String (c0) + "/" + juce::String (n0) + " bits " + juce::String (b0) + "; p100 " + juce::String (c1) + "/" + juce::String (n1)
                 + "; p50 " + juce::String (c5) + "/" + juce::String (n5) + " z " + f2 (z, 2) + ", bit == crushed " + (b5 == c5 ? "yes" : "NO")
                 + " | thr |z| <= 3.29 | margin " + f2 (3.29 - std::abs (z), 2) + " | " + ctrl (fired, "50 % log vs p = 0.3 -> |z| " + f2 (std::abs (zc), 2)));
    }

    void rowFV1d()
    {
        struct Cfg { const char* name; P ps; std::uint32_t want; };
        const std::vector<Cfg> cfgs {
            { "filter Off rand 100", s1p ({ { "filter_type", 0.0f }, { "filter_rand", 100.0f } }), 0u },
            { "LP 20 kHz default", s1p ({ { "filter_type", 1.0f } }), ogi::kFxSvf },
            { "drive 0", s1p ({ { "drive", 0.0f } }), 0u },
            { "drive 0.2 %", s1p ({ { "drive", 0.2f } }), 0u },
            { "drive 0.3 %", s1p ({ { "drive", 0.3f } }), ogi::kFxDrive },
            { "crush 0 prob 37", s1p ({ { "crush", 0.0f }, { "crush_prob", 37.0f } }), 0u },
            { "crush 1 prob 0", s1p ({ { "crush", 1.0f }, { "crush_prob", 0.0f } }), 0u },
            { "crush 1 prob 100", s1p ({ { "crush", 1.0f }, { "crush_prob", 100.0f } }), ogi::kFxCrush },
            { "ring mix 0 rand 100", s1p ({ { "ring_mix", 0.0f }, { "ring_rand", 100.0f } }), 0u },
            { "ring mix 1", s1p ({ { "ring_mix", 1.0f } }), ogi::kFxRing },
        };
        auto check = [&cfgs] (bool wrongTable, int& grains, int& scanned)
        {
            Tally t;
            grains = scanned = 0;
            for (const auto& c0 : cfgs)
            {
                std::uint32_t want = c0.want;
                if (wrongTable && juce::String (c0.name) == "drive 0") want = ogi::kFxDrive;
                Session s (kFs, cat ({ base4 (1), { { sn (1, "density"), 40.0f }, { sn (1, "size"), 100.0f } }, c0.ps }));
                s.record = false;
                s.load (0, noiseBuf (2, (int) (2.0 * kFs), 0.2f), kFs);
                s.noteOn (0, 48);
                int badFields = 0;
                s.afterBlock = [&badFields, &scanned] (Session& ss)
                {
                    const auto& pool = ss.eng().getPool();
                    const Grain d {};
                    for (int k = 0; k < ogi::kPoolLive; ++k)
                    {
                        if (! pool.isActive (k)) continue;
                        const auto& g = pool.getGrains()[(size_t) k].g;
                        ++scanned;
                        if (! (g.fxActive & ogi::kFxSvf) && (g.svfMode != d.svfMode || ! sameF (g.svfK, d.svfK) || ! sameF (g.svfA1, d.svfA1)
                                                               || ! sameF (g.svfA2, d.svfA2) || ! sameF (g.svfA3, d.svfA3) || ! sameF (g.svfMake, d.svfMake))) ++badFields;
                        if (! (g.fxActive & ogi::kFxDrive) && (! sameF (g.driveD, d.driveD) || ! sameF (g.driveNorm, d.driveNorm))) ++badFields;
                        if (! (g.fxActive & ogi::kFxCrush) && (! sameF (g.crushHoldInc, d.crushHoldInc) || ! sameF (g.crushScale, d.crushScale))) ++badFields;
                        if (! (g.fxActive & ogi::kFxRing) && (! sameF (g.ringRotRe, d.ringRotRe) || ! sameF (g.ringRotIm, d.ringRotIm) || ! sameF (g.ringMix, d.ringMix))) ++badFields;
                    }
                };
                s.run ((juce::int64) kFs, 512);
                const auto lg = s.hk().logInOrder();
                int badBits = 0;
                for (const auto& r : lg) badBits += r.fxActive != want ? 1 : 0;
                grains += (int) lg.size();
                t.check (badBits == 0 && badFields == 0 && ! lg.empty(), juce::String (c0.name) + ": " + juce::String (badBits) + " bad bits, "
                                                                          + juce::String (badFields) + " bad fields");
            }
            return t;
        };
        int g = 0, sc = 0, gC = 0, scC = 0;
        const auto t = check (false, g, sc);
        const auto tc = check (true, gC, scC);
        const bool fired = tc.bad > 0;
        row (t.bad == 0 && fired, "FV1d fxActive gate table == expected; clear-bit stages keep Grain {} defaults (live pool scan)",
             "meas " + t.detail ((int) cfgs.size()) + " [" + juce::String (g) + " grains, " + juce::String (sc) + " live-grain scans] | thr 0 mismatches | margin "
                 + juce::String (-t.bad) + " | " + ctrl (fired, "checker fed drive 0 -> kFxDrive: " + juce::String (tc.bad) + " config(s) mismatch"));
    }

    //==========================================================================
    // FV2–FV4 — the renderer honours the per-grain latch (audio census)
    struct IsoOut { std::vector<float> L; Log log; };

    IsoOut isoRender (double secs, const P& extra, const juce::AudioBuffer<float>& src, bool shared)
    {
        Session s (kFs, cat ({ base4 (1), { { sn (1, "size"), 100.0f }, { sn (1, "density"), 5.0f }, { sn (1, "jitter"), 0.0f },
                                            { sn (1, "spray"), 10.0f } }, extra }));
        s.hk().fxSharedLatch = shared;
        s.load (0, src, kFs);
        s.noteOn (0, 48);
        s.run ((juce::int64) (secs * kFs), 512);
        return { s.L, s.hk().logInOrder() };
    }

    void rowFV2()
    {
        const auto src = noiseBuf (2, (int) (13.0 * kFs), 0.25f, 31);
        auto run = [&src] (bool shared, int& used, double& spanOct)
        {
            const auto o = isoRender (12.0, s1p ({ { "filter_type", 2.0f }, { "filter_res", 80.0f }, { "filter_cutoff", 2000.0f },
                                                   { "filter_rand", 100.0f } }), src, shared);
            constexpr size_t N = 1u << 15;
            std::vector<double> lf, lc;
            double cmin = 1e9, cmax = -1e9;
            for (const auto& r : isolated (o.log, o.L.size()))
            {
                if ((r.fxActive & ogi::kFxSvf) == 0) continue;
                const auto p = grainSpectrum (o.L, r, N);
                double w = 0.0, acc = 0.0;
                for (size_t k = 1; k < p.size(); ++k)
                {
                    const double f = (double) k * kFs / (double) N;
                    if (f < 50.0 || f > ogi::kFxNyquist * kFs) continue;
                    w += p[k]; acc += p[k] * std::log2 (f);
                }
                if (w <= 0.0) continue;
                lf.push_back (std::log2 ((double) r.fcHz));
                lc.push_back (acc / w);
                cmin = std::min (cmin, acc / w); cmax = std::max (cmax, acc / w);
            }
            used = (int) lf.size();
            spanOct = cmax - cmin;
            return lf.size() >= 3 ? spearman (lf, lc) : 0.0;
        };
        int n = 0, nn = 0;
        double span = 0, spann = 0;
        const double rho = run (false, n, span), rhon = run (true, nn, spann);
        const bool fired = nn >= 40 && std::abs (rhon) <= 0.5;
        row (n >= 40 && rho >= 0.9 && fired, "FV2 BP res 80 %, rand 100 %: Spearman (log2 fc, per-grain centroid) >= 0.9",
             "meas rho " + f2 (rho, 4) + " over " + juce::String (n) + " isolated grains, centroid span " + f2 (span, 2) + " oct | thr >= 0.9, n >= 40 | margin "
                 + f2 (rho - 0.9, 4) + " | " + ctrl (fired, "fxSharedLatch -> rho " + f2 (rhon, 4) + " over " + juce::String (nn) + " (|rho| <= 0.5)"));
    }

    void rowFV3()
    {
        const auto src = sineBuf (2, (int) (13.0 * kFs), 1000.0, kFs, 0.5);
        auto run = [&src] (bool shared, int& used, int& within)
        {
            const auto o = isoRender (12.0, s1p ({ { "ring_mix", 100.0f }, { "ring_freq", 200.0f }, { "ring_rand", 100.0f } }), src, shared);
            constexpr size_t N = 1u << 17;
            used = within = 0;
            for (const auto& r : isolated (o.log, o.L.size()))
            {
                if (r.onsetSample < (juce::int64) (0.3 * kFs) || (r.fxActive & ogi::kFxRing) == 0) continue;
                const auto p = grainSpectrum (o.L, r, N);
                const double fh = peakHz (p, kFs, N, 1025.0, 2000.0) - 1000.0;
                ++used;
                within += std::abs (fh - (double) r.ringHz) <= 2.0 ? 1 : 0;
            }
        };
        int u = 0, w = 0, un = 0, wn = 0;
        run (false, u, w);
        run (true, un, wn);
        const bool fired = un >= 40 && wn < 0.2 * un;
        row (u >= 40 && w >= 0.95 * u && fired, "FV3 ring 200 Hz rand 100 %: USB carrier within 2 Hz of latched ringHz >= 95 %",
             "meas " + juce::String (w) + "/" + juce::String (u) + " grains | thr >= 95 %, n >= 40 | margin " + f2 ((double) w / std::max (1, u) - 0.95, 3)
                 + " | " + ctrl (fired, "fxSharedLatch -> " + juce::String (wn) + "/" + juce::String (un) + " (< 20 %)"));
    }

    void rowFV4()
    {
        const auto src = sineBuf (2, (int) (17.0 * kFs), 440.0, kFs, 0.5);
        auto run = [&src] (bool shared, int& used, int& agree, int& nC, int& nU, double& sepLo, double& sepHi)
        {
            const auto o = isoRender (16.0, s1p ({ { "crush", 80.0f }, { "crush_prob", 50.0f } }), src, shared);
            constexpr size_t N = 1u << 14;
            used = agree = nC = nU = 0;
            sepLo = 1e9; sepHi = -1e9;
            for (const auto& r : isolated (o.log, o.L.size()))
            {
                if (r.onsetSample < (juce::int64) (0.3 * kFs)) continue;
                const auto p = grainSpectrum (o.L, r, N);
                double in = 0, outb = 0;
                for (size_t k = 1; k < p.size(); ++k)
                {
                    const double f = (double) k * kFs / (double) N;
                    (std::abs (f - 440.0) <= 50.0 ? in : outb) += p[k];
                }
                const double frac = 10.0 * std::log10 ((outb + 1e-300) / (in + outb + 1e-300));
                const bool cl = frac > -30.0;
                ++used;
                agree += cl == r.crushed ? 1 : 0;
                (r.crushed ? nC : nU)++;
                if (r.crushed) sepLo = std::min (sepLo, frac); else sepHi = std::max (sepHi, frac);
            }
        };
        int u, a, c, un_, un, an, cn, unn;
        double lo, hi, lo2, hi2;
        run (false, u, a, c, un_, lo, hi);
        run (true, un, an, cn, unn, lo2, hi2);
        const bool fired = un >= 40 && an <= 0.6 * un;
        row (u >= 40 && c >= 10 && un_ >= 10 && a >= 0.99 * u && fired, "FV4 crush 80 % prob 50 %: out-of-band split (-30 dB) agrees with latch >= 99 %",
             "meas " + juce::String (a) + "/" + juce::String (u) + " agree, crushed " + juce::String (c) + " / clean " + juce::String (un_)
                 + "; crushed min " + f2 (lo, 1) + " dB, clean max " + f2 (hi, 1) + " dB | thr >= 99 %, >= 10 per class | margin "
                 + f2 ((double) a / std::max (1, u) - 0.99, 3) + " | " + ctrl (fired, "fxSharedLatch -> " + juce::String (an) + "/" + juce::String (un) + " (<= 60 %)"));
    }

    //==========================================================================
    // FB1–FB4 — no bleed
    P fbParams (int cap, bool kills)
    {
        P p = cat ({ base4 (4), { { "polyphony", 8.0f }, { "grain_limit", (float) cap } } });
        const float types[4] { 1.0f, 2.0f, 3.0f, 0.0f };
        for (int n = 1; n <= 4; ++n)
            p = with (p, { { sn (n, "density"), kills ? 200.0f : 200.0f }, { sn (n, "size"), 300.0f },
                           { sn (n, "filter_type"), types[n - 1] }, { sn (n, "filter_res"), 90.0f }, { sn (n, "filter_cutoff"), 1500.0f },
                           { sn (n, "drive"), 50.0f }, { sn (n, "crush"), 60.0f }, { sn (n, "crush_prob"), 50.0f },
                           { sn (n, "ring_freq"), 400.0f }, { sn (n, "ring_mix"), 70.0f }, { sn (n, "ring_rand"), 50.0f },
                           { sn (n, "spray"), 20.0f }, { sn (n, "jitter"), 20.0f } });
        return p;
    }

    struct FbOut { std::vector<float> v; std::uint64_t spawns = 0, kills = 0, steals = 0, nonDefault = 0; };

    FbOut fbRender (bool poison, bool noReset, int cap, bool thinOff, bool capDrop = false)
    {
        Session s (kFs, fbParams (cap, true), kSeed, 512);
        shipping (s);
        s.hk().poisonFreeSlots = poison;
        s.hk().fxNoReset = noReset;
        s.hk().thinningOff = thinOff;
        loadNoise4 (s, 3.0, 0.3f);
        for (int n : { 48, 50, 52, 55, 57, 60, 62, 64 })
            s.noteOn (0, n);
        if (capDrop)
            setAt (s, 48000, "grain_limit", 0.0f);   // 256 (saturated: native steals) -> 32 (R3 kills)
        s.run ((juce::int64) (2.0 * kFs), 512);
        FbOut o;
        o.v = lr (s);
        o.spawns = s.hk().spawns;
        o.kills = s.hk().r3Kills + s.hk().capKills;
        o.steals = s.hk().nativeSteals;
        o.nonDefault = s.hk().fxStateNonDefaultAtSpawn;
        return o;
    }

    double maxAbsDiff (const std::vector<float>& a, const std::vector<float>& b)
    {
        if (a.size() != b.size()) return 1e30;
        double m = 0;
        for (size_t i = 0; i < a.size(); ++i) m = std::max (m, (double) std::abs (a[i] - b[i]));
        return m;
    }

    void rowFB1()
    {
        // thinningOff + cap 256 saturates the pool (native steals); the cap then
        // drops to 32 at 1 s (R3 kills): both paths in one render.
        const auto clean = fbRender (false, false, 3, true, true);
        const auto pois  = fbRender (true, false, 3, true, true);
        const auto ctl   = fbRender (true, true, 3, true, true);
        const double d = maxAbsDiff (clean.v, pois.v), dc = maxAbsDiff (clean.v, ctl.v);
        const bool fired = dc > 0.0;
        row (d == 0.0 && clean.spawns >= 1000 && clean.kills > 0 && clean.steals > 0 && fired,
             "FB1 no bleed: poisonFreeSlots render == clean render (4 streams, all FX, kills + steals)",
             "meas max|d| " + juce::String (d, 9) + " (spawns " + i64 (clean.spawns) + ", R3 kills " + i64 (clean.kills) + ", native steals "
                 + i64 (clean.steals) + ") | thr 0 | margin " + juce::String (-d) + " | " + ctrl (fired, "poison + fxNoReset -> max|d| " + juce::String (dc, 6)));
    }

    void rowFB2()
    {
        // A reads the sine region, B (the next grain, same slot) reads silence.
        const juce::int64 swap = (juce::int64) (0.3 * kFs), snap = (juce::int64) (0.4 * kFs);
        juce::AudioBuffer<float> src (2, (int) (3.0 * kFs));
        src.clear();
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < (int) (1.5 * kFs); ++i)
                src.setSample (c, i, (float) (0.5 * std::sin (2.0 * kPi * 1000.0 * (double) i / kFs)));
        auto run = [&] (bool noReset, bool& same, double& shadowIc, double& bPeak, juce::String& det)
        {
            Session s (kFs, cat ({ base4 (1), s1p ({ { "filter_type", 2.0f }, { "filter_res", 100.0f }, { "filter_cutoff", 1000.0f },
                                                     { "drive", 30.0f }, { "crush", 30.0f }, { "crush_prob", 100.0f },
                                                     { "ring_mix", 50.0f }, { "ring_freq", 200.0f },
                                                     { "density", 2.0f }, { "size", 200.0f }, { "position", 0.0f } }) }));
            s.hk().fxNoReset = noReset;
            s.load (0, src, kFs);
            s.noteOn (0, 48);
            setAt (s, swap, sn (1, "position"), 75.0f);
            ogi::Grain shadow[ogi::kPoolLive];
            bool snapped = false;
            s.afterBlock = [&] (Session& ss)
            {
                if (! snapped && ss.pos >= snap)
                {
                    for (int k = 0; k < ogi::kPoolLive; ++k) shadow[k] = ss.hk().fxShadow[k];
                    snapped = true;
                }
            };
            s.run ((juce::int64) (1.2 * kFs), 512);
            const auto lg = s.hk().logInOrder();
            same = false; shadowIc = 0.0; bPeak = -1.0;
            if (lg.size() < 2) { det = "fewer than 2 grains"; return; }
            const auto& A = lg[0];
            const auto& B = lg[1];
            same = A.poolSlot == B.poolSlot && A.onsetSample + (juce::int64) std::ceil (A.len) < swap && B.onsetSample > snap;
            const auto& sh = shadow[std::clamp (A.poolSlot, 0, ogi::kPoolLive - 1)];
            shadowIc = std::max ({ std::abs ((double) sh.svfIc1[0]), std::abs ((double) sh.svfIc2[0]), std::abs ((double) sh.svfIc1[1]) });
            bPeak = 0.0;
            for (juce::int64 i = B.onsetSample; i < B.onsetSample + (juce::int64) std::ceil (B.len) + 1 && i < (juce::int64) s.L.size(); ++i)
                bPeak = std::max ({ bPeak, (double) std::abs (s.L[(size_t) i]), (double) std::abs (s.R[(size_t) i]) });
            det = "A onset " + juce::String (A.onsetSample) + " slot " + juce::String (A.poolSlot) + ", B onset " + juce::String (B.onsetSample)
                  + " slot " + juce::String (B.poolSlot);
        };
        bool same = false, same2 = false;
        double ic = 0, ic2 = 0, pk = -1, pk2 = -1;
        juce::String d1, d2;
        run (false, same, ic, pk, d1);
        run (true, same2, ic2, pk2, d2);
        const bool fired = same2 && pk2 > 1.0e-6;
        row (same && ic > 1.0e-3 && pk == 0.0 && fired, "FB2 slot reuse after a resonant grain: B on silence == 0.0f exactly, same slot, A rang",
             "meas B peak " + juce::String (pk, 6) + ", A shadow |ic| " + f2 (ic, 4) + " (" + d1 + ") | thr B == 0, |ic| > 1e-3 | margin "
                 + juce::String (-pk) + " | " + ctrl (fired, "fxNoReset -> B peak " + juce::String (pk2, 6)));
    }

    struct FB3Out { std::uint64_t checked = 0, mism = 0, steals = 0; int moved = 0; };

    FB3Out fb3Render (bool perturb)
    {
        Session s (kFs, cat ({ base4 (1), { { "grain_limit", 3.0f } },
                               s1p ({ { "density", 200.0f }, { "size", 1000.0f }, { "jitter", 20.0f },
                                      { "filter_type", 1.0f }, { "filter_cutoff", 2000.0f }, { "filter_res", 70.0f }, { "filter_rand", 50.0f },
                                      { "drive", 40.0f }, { "crush", 30.0f }, { "crush_prob", 50.0f }, { "ring_mix", 50.0f }, { "ring_rand", 50.0f } }) }));
        s.hk().snapshotSteals = true;
        s.hk().stealSnapPerturb = perturb;
        s.hk().thinningOff = true;   // saturate the pool → native steals
        s.record = false;
        s.load (0, noiseBuf (2, (int) (4.0 * kFs), 0.2f), kFs);
        for (int k = 0; k < 4; ++k)
            s.noteOn (0, 48 + k);
        struct Seen { bool active = false; int age = 0; float ic = 0.0f; };
        Seen prev[ogi::kPoolFade] {};
        FB3Out o;
        s.afterBlock = [&] (Session& ss)
        {
            const auto& pool = ss.eng().getPool();
            for (int k = 0; k < ogi::kPoolFade; ++k)
            {
                const int sl = ogi::kPoolLive + k;
                const auto& g = pool.getGrains()[(size_t) sl].g;
                Seen now { pool.isActive (sl), g.age, g.svfIc1[0] };
                if (now.active && prev[k].active && now.age > prev[k].age && (g.fxActive & ogi::kFxSvf) != 0 && ! sameF (now.ic, prev[k].ic))
                    ++o.moved;
                prev[k] = now;
            }
        };
        s.run ((juce::int64) (3.0 * kFs), 512);
        o.checked = s.hk().stealStateChecked;
        o.mism = s.hk().stealStateMismatches;
        o.steals = s.hk().nativeSteals;
        return o;
    }

    void rowFB3()
    {
        const auto a = fb3Render (false);
        const auto c = fb3Render (true);
        const bool fired = c.checked > 0 && c.mism == c.checked;
        row (a.checked >= 50 && a.mism == 0 && a.moved > 0 && fired, "FB3 native steal: fade copy keeps FX latch + state bitwise; copy's SVF state moves on",
             "meas " + i64 (a.checked) + " checked, " + i64 (a.mism) + " mismatches (steals " + i64 (a.steals) + "); fade SVF state advanced "
                 + juce::String (a.moved) + " times | thr >= 50 checked, 0 mismatches | margin " + juce::String ((juce::int64) a.mism * -1)
                 + " | " + ctrl (fired, "stealSnapPerturb -> " + i64 (c.mism) + "/" + i64 (c.checked) + " mismatch"));
    }

    void rowFB4()
    {
        const auto k  = fbRender (false, false, 0, true);    // cap 32, thinningOff: R3 kills
        const auto st = fbRender (false, false, 3, true);    // cap 256 saturated: native steals
        const auto c  = fbRender (true, true, 3, true);
        const bool fired = c.nonDefault > 0;
        row (k.nonDefault == 0 && st.nonDefault == 0 && k.spawns >= 1000 && st.spawns >= 1000 && k.kills > 0 && st.steals > 0 && fired,
             "FB4 FX state == Grain {} at every spawn (kills render, steals render)",
             "meas non-default " + i64 (k.nonDefault) + " / " + i64 (st.nonDefault) + " (spawns " + i64 (k.spawns) + " / " + i64 (st.spawns)
                 + ", kills " + i64 (k.kills) + ", steals " + i64 (st.steals) + ") | thr 0 | margin 0 | "
                 + ctrl (fired, "poison + fxNoReset -> " + i64 (c.nonDefault) + " non-default"));
    }

    //==========================================================================
    // FZ1 / FZ2 — stress, NaN
    P stressParams()
    {
        P p = cat ({ base4 (4), { { "polyphony", 16.0f }, { "grain_limit", 3.0f } } });
        const float types[4] { 1.0f, 2.0f, 3.0f, 1.0f };
        for (int n = 1; n <= 4; ++n)
            p = with (p, { { sn (n, "density"), 100.0f }, { sn (n, "size"), 100.0f },
                           { sn (n, "filter_type"), types[n - 1] }, { sn (n, "filter_res"), 100.0f }, { sn (n, "filter_cutoff"), 1000.0f },
                           { sn (n, "drive"), 100.0f }, { sn (n, "crush"), 100.0f }, { sn (n, "crush_prob"), 100.0f },
                           { sn (n, "ring_freq"), 5000.0f }, { sn (n, "ring_mix"), 100.0f }, { sn (n, "ring_rand"), 100.0f },
                           { sn (n, "spray"), 30.0f } });
        p = with (p, { { sn (1, "pitch"), 24.0f }, { sn (2, "pitch"), -24.0f } });
        return p;
    }

    struct FzOut { std::uint64_t nonFinite = 0, subn = 0, kills = 0, spawns = 0; double peak = 0; bool hit = false; juce::int64 youngAt = -1; };

    /** injectAt >= 0: the control (injectNan into slot 0 + nanKillOff). youngAt
        returns the first cell >= 0.5 s that lands 64 samples into a slot-0 grain
        with >= 2400 samples left (an old grain can retire, or sit in a crush hold,
        before the NaN reaches the bus). */
    FzOut fzRender (double fs, juce::int64 injectAt)
    {
        const bool injectOff = injectAt >= 0;
        Session s (fs, stressParams(), kSeed, 512);
        shipping (s);
        s.hk().fxSubnormalScan = true;
        if (injectOff)
        {
            s.hk().nanKillOff = true;
            s.hk().injectNan = { 0, injectAt, 0, std::numeric_limits<float>::quiet_NaN(), false };
        }
        const int n = (int) (2.0 * fs);
        s.load (0, noiseBuf (2, n, 1.0f, 41), fs);
        s.load (1, noiseBuf (2, n, 1.0f, 42), fs);
        s.load (2, sineBuf (2, n, 1000.0, fs, 1.0), fs);
        s.load (3, sineBuf (2, n, 1000.0, fs, 1.0), fs);
        for (int k = 0; k < 16; ++k)
            s.noteOn (0, 84 + k);   // 36 st above the root: s1 +24 clamps at +48
        s.run ((juce::int64) (1.5 * fs), 512);
        FzOut o;
        for (size_t i = 0; i < s.L.size(); ++i)
        {
            const float l = s.L[i], r = s.R[i];
            if (! std::isfinite (l) || ! std::isfinite (r)) ++o.nonFinite;
            else o.peak = std::max ({ o.peak, (double) std::abs (l), (double) std::abs (r) });
        }
        o.subn = s.hk().fxSubnormals;
        o.kills = s.hk().nanKills;
        o.spawns = s.hk().spawns;
        o.hit = s.hk().injectNan.done;
        for (const auto& e : s.hk().logInOrder())
            if (e.poolSlot == 0 && e.onsetSample >= (juce::int64) (0.5 * fs) && e.len >= 4800.0)
            {
                o.youngAt = (e.onsetSample + 32 + 31) / 32 * 32 + 32;
                break;
            }
        return o;
    }

    void rowFZ1()
    {
        Tally t;
        juce::String d;
        for (double fs : { 44100.0, 48000.0, 96000.0 })
        {
            const auto o = fzRender (fs, -1);
            t.check (o.nonFinite == 0 && o.subn == 0 && o.kills == 0 && o.spawns > 500,
                     f2 (fs / 1000.0, 1) + " k: nonfinite " + i64 (o.nonFinite) + " subn " + i64 (o.subn) + " kills " + i64 (o.kills));
            d << " " << f2 (fs / 1000.0, 1) << " k: nonfinite " << i64 (o.nonFinite) << ", subnormal " << i64 (o.subn) << ", nanKills " << i64 (o.kills)
              << ", spawns " << i64 (o.spawns) << ", peak " << f2 (o.peak, 3) << ";";
        }
        const auto c = fzRender (48000.0, fzRender (48000.0, -1).youngAt);
        Grain dn {};
        dn.svfIc2[1] = std::numeric_limits<float>::denorm_min();
        const int scan = ogi::fxstate::subnormals (dn);
        const bool fired = c.nonFinite > 0 && scan == 1;
        row (t.bad == 0 && fired, "FZ1 stress 44.1 / 48 / 96 k (res 100, drive / crush / ring 100, +48 / -24 st, full scale): finite, 0 subnormal FX state",
             "meas" + d + " | thr 0 / 0 / 0 | margin 0 | " + ctrl (fired, "injectNan + nanKillOff (hit " + juce::String (c.hit ? "yes" : "NO") + ") -> " + i64 (c.nonFinite) + " non-finite samples; scanner fed denorm_min -> "
                                                              + juce::String (scan)));
    }

    struct FZ2Out { std::vector<float> v; std::uint64_t kills = 0, notRetired = 0; bool hit = false; };

    FZ2Out fz2Render (int slot, juce::int64 at, int field, float value, int mode, bool fade)
    {
        // mode 0: injectNan, 1: forceRetire, 2: injectNan + nanKillOff
        P p = fade ? cat ({ base4 (1), { { "grain_limit", 3.0f } },
                            s1p ({ { "density", 200.0f }, { "size", 1000.0f }, { "filter_type", 1.0f }, { "filter_cutoff", 1000.0f },
                                   { "filter_res", 50.0f } }) })
                   : cat ({ base4 (1), s1p ({ { "density", 5.0f }, { "size", 200.0f }, { "filter_type", 1.0f }, { "filter_cutoff", 1000.0f },
                                              { "filter_res", 50.0f } }) });
        Session s (kFs, p, kSeed, 512);
        if (fade)
            s.hk().thinningOff = true;
        if (mode == 1) s.hk().forceRetire = { slot, at, false };
        else           s.hk().injectNan = { slot, at, field, value, false };
        if (mode == 2) s.hk().nanKillOff = true;
        s.load (0, noiseBuf (2, (int) (3.0 * kFs), 0.2f), kFs);
        for (int k = 0; k < (fade ? 4 : 1); ++k)
            s.noteOn (0, 48 + k);
        s.run ((juce::int64) (fade ? 1.0 * kFs : 0.5 * kFs), 512);
        FZ2Out o;
        o.v = lr (s);
        o.kills = s.hk().nanKills;
        o.notRetired = s.hk().nanKillNotRetired;
        o.hit = mode == 1 ? s.hk().forceRetire.done : s.hk().injectNan.done;
        return o;
    }

    void rowFZ2()
    {
        Tally t;
        int total = 0;
        const float vals[3] { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() };
        const char* names[3] { "NaN", "+Inf", "-Inf" };
        const juce::int64 at = 3200;   // a multiple of 32, inside the first grain (slot 0, 0..9600)
        const auto ref = fz2Render (0, at, 0, 0.0f, 1, false);
        for (int v = 0; v < 3; ++v)
            for (int f = 0; f < 4; ++f)
            {
                const auto o = fz2Render (0, at, f, vals[v], 0, false);
                ++total;
                t.check (o.hit && o.kills == 1 && o.notRetired == 0 && diffCount (o.v, ref.v) == 0,
                         juce::String (names[v]) + " field " + juce::String (f) + ": kills " + i64 (o.kills) + ", differ " + juce::String (diffCount (o.v, ref.v)));
            }
        // Fade slot (256): a saturated pool keeps it occupied by a stolen SVF grain.
        const juce::int64 atF = 24000;
        const auto refF = fz2Render (ogi::kPoolLive, atF, 0, 0.0f, 1, true);
        const auto oF = fz2Render (ogi::kPoolLive, atF, 2, vals[0], 0, true);
        ++total;
        t.check (refF.hit && oF.hit && oF.kills == 1 && oF.notRetired == 0 && diffCount (oF.v, refF.v) == 0,
                 "fade slot 256: kills " + i64 (oF.kills) + ", differ " + juce::String (diffCount (oF.v, refF.v)));
        const auto c = fz2Render (0, at, 0, vals[0], 2, false);
        int nonFinite = 0;
        for (float x : c.v) nonFinite += std::isfinite (x) ? 0 : 1;
        const bool fired = nonFinite > 0 || diffCount (c.v, ref.v) != 0;
        row (t.bad == 0 && fired, "FZ2 NaN kill: NaN / +Inf / -Inf x 4 SVF fields + 1 fade slot -> 1 kill, retired, output == forceRetire render",
             "meas " + t.detail (total) + " | thr kills == 1, 0 not retired, 0 differ | margin " + juce::String (-t.bad)
                 + " | " + ctrl (fired, "nanKillOff -> " + juce::String (nonFinite) + " non-finite samples"));
    }

    //==========================================================================
    // FA3 — allocations, every FX on
    void rowFA3()
    {
       #if JUCE_MAC
        using namespace eg21;
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

        P fx;
        const float types[4] { 1.0f, 2.0f, 3.0f, 0.0f };
        for (int n = 1; n <= 4; ++n)
            fx = with (fx, { { sn (n, "filter_type"), types[n - 1] }, { sn (n, "filter_cutoff"), 1500.0f }, { sn (n, "filter_res"), 80.0f },
                             { sn (n, "filter_rand"), 50.0f }, { sn (n, "drive"), 50.0f }, { sn (n, "crush"), 40.0f },
                             { sn (n, "crush_prob"), 50.0f }, { sn (n, "ring_freq"), 300.0f }, { sn (n, "ring_mix"), 50.0f },
                             { sn (n, "ring_rand"), 50.0f } });
        Session s (kFs, cat ({ base4 (4), eg22::randomised(), all4 ("density", 100.0f), all4 ("size", 200.0f),
                               { { "polyphony", 8.0f }, { "amp_attack", 5.0f }, { "amp_release", 100.0f } }, fx }));
        shipping (s);
        s.record = false;
        loadNoise4 (s, 2.0, 0.3f);
        s.hk().injectNan = { 0, 36000, 1, std::numeric_limits<float>::quiet_NaN(), false };
        s.arm = true;
        s.noteOn (4800, 60);
        s.noteOff (9600, 60);
        for (int n = 40; n < 60; ++n)
            s.noteOn (14400, n);
        for (int n = 40; n < 60; ++n)
            s.noteOff (24000, n);
        for (int n = 48; n < 54; ++n)
            s.noteOn (28800, n);
        setAt (s, 33600, sn (1, "filter_type"), 2.0f);
        setAt (s, 38400, sn (2, "filter_type"), 0.0f);
        setAt (s, 40000, sn (4, "filter_type"), 3.0f);
        setAt (s, 43200, "grain_limit", 0.0f);
        s.run (57600, 512);
        s.arm = false;
        malloc_logger = nullptr;
        allocTrace = false;

        const int n = allocCount;
        auto& h = s.hk();
        const bool exercised = h.voiceSteals > 0 && (h.r3Kills + h.capKills) > 0 && h.nanKills == 1;
        row (liveness == 1 && n == 0 && exercised, "FA3 PERF-01 0 audio-thread allocations: every FX, type switches, steals, cap 256 -> 32, NaN kill",
             "meas " + juce::String (n) + " allocs over " + i64 ((std::uint64_t) s.blocksDone) + " blocks (voice steals " + i64 (h.voiceSteals)
                 + ", kills " + i64 (h.r3Kills + h.capKills) + ", nanKills " + i64 (h.nanKills) + ") | thr 0 | margin " + juce::String (-n)
                 + " | " + ctrl (liveness == 1, "liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (false, "FA3 alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    void runAll()
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        std::printf ("\n== Phase 2.3 engine gates (per-grain FX)\n");
        section ("FN / FR. determinism + bypass");
        rowFN1();
        rowFN2();
        rowFN3();
        rowFR1();
        section ("FV. latch census + the renderer honours the latch");
        rowsFV1ab();
        rowFV1c();
        rowFV1d();
        rowFV2();
        rowFV3();
        rowFV4();
        section ("FB. no bleed");
        rowFB1();
        rowFB2();
        rowFB3();
        rowFB4();
        section ("FZ / FA. stress, NaN, allocations");
        rowFZ1();
        rowFZ2();
        rowFA3();
        std::printf ("\n-- Phase 2.3 engine gates done (%.1f s)\n", (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
    }

    //==========================================================================
    // --cpu23: D19 / D20 (PLAN-2.3 PD1, PD2, PD13)
    struct CpuCfg { double pct = 0, mn = 0, mx = 0, duty = 0, live = 0, calB = 0, calA = 0; bool slowed = false, contended = false; };

    CpuCfg measureCfg (const std::function<std::unique_ptr<Session>()>& make, const std::vector<int>& notes, double fs, int block,
                       double refNs, int reps = 7)
    {
        CpuCfg c;
        auto s = make();
        {
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer mb;
            for (int n : notes)
                mb.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
            buf.clear();
            s->p->processBlock (buf, mb);
        }
        auto run = [&] (double seconds, bool timeIt, double& cpu, double& duty, double& live)
        {
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer mb;
            const int blocks = (int) (seconds * fs / block);
            cpu = 0; live = 0;
            const double w0 = eg22::wallNs(), t0 = eg22::threadNs();
            for (int b = 0; b < blocks; ++b)
            {
                buf.clear();
                mb.clear();
                const double a = timeIt ? eg22::threadNs() : 0.0;
                s->p->processBlock (buf, mb);
                if (timeIt) cpu += eg22::threadNs() - a;
                live += (double) s->hk().liveNow;
            }
            const double wall = eg22::wallNs() - w0, thr = eg22::threadNs() - t0;
            duty = wall > 0 ? thr / wall : 0;
            live /= std::max (1, blocks);
        };
        double cpu, duty, live;
        c.calB = cal::calibNs();
        run (5.0, false, cpu, duty, live);   // warm-up
        std::vector<double> pct;
        double minDuty = 1e9, lv = 0;
        for (int i = 0; i < reps; ++i)
        {
            run (10.0, true, cpu, duty, live);
            pct.push_back (100.0 * cpu / (10.0 * 1.0e9));
            minDuty = std::min (minDuty, duty);
            lv += live / reps;
        }
        c.calA = cal::calibNs();
        std::sort (pct.begin(), pct.end());
        c.pct = pct[pct.size() / 2]; c.mn = pct.front(); c.mx = pct.back();
        c.duty = minDuty; c.live = lv;
        c.contended = minDuty < 0.9;
        c.slowed = ! std::isfinite (refNs) || c.calB > 1.15 * refNs || c.calA / c.calB > 1.10;
        return c;
    }

    juce::String cpuLine (const char* name, const CpuCfg& c, double refNs)
    {
        return juce::String (name) + ": median " + f2 (c.pct, 2) + " % (min " + f2 (c.mn, 2) + ", max " + f2 (c.mx, 2) + "), live " + f2 (c.live, 1)
               + ", duty min " + f2 (c.duty, 3) + ", calib before " + juce::String ((juce::int64) c.calB) + " (" + f2 (c.calB / refNs, 3)
               + " x ref) after " + juce::String ((juce::int64) c.calA) + " (" + f2 (c.calA / c.calB, 3) + " x before)"
               + (c.slowed ? " [NOVERDICT - slowed core]" : "") + (c.contended ? " [NOVERDICT - contended]" : "");
    }

    std::unique_ptr<Session> c22Session (double fs, int block, bool streamsOn, const P& fx)
    {
        auto p = cat ({ base4 (streamsOn ? 4 : 0), all4 ("density", 200.0f), all4 ("size", 1000.0f),
                        all4 ("spray", 10.0f), all4 ("jitter", 20.0f), all4 ("stereo_spread", 30.0f),
                        { { "grain_limit", 3.0f }, { "polyphony", 16.0f } }, fx });
        auto s = std::make_unique<Session> (fs, p, kSeed, block);
        shipping (*s);
        for (int k = 0; k < 4; ++k)
            s->load (k, noiseBuf (2, (int) (10.0 * fs), 0.1f, 11 + k), fs);
        s->record = false;
        return s;
    }

    P worstFx()
    {
        P p;
        for (int n = 1; n <= 4; ++n)
            p = with (p, { { sn (n, "filter_type"), 2.0f }, { sn (n, "filter_cutoff"), 1000.0f }, { sn (n, "filter_res"), 100.0f },
                           { sn (n, "drive"), 100.0f }, { sn (n, "crush"), 100.0f }, { sn (n, "crush_prob"), 100.0f },
                           { sn (n, "ring_freq"), 200.0f }, { sn (n, "ring_mix"), 100.0f } });
        return p;
    }

    int cpu23()
    {
        constexpr int kBlock = 128;
        const double refNs = cal::readRef (gGoldenDir);
        double c22at22 = std::numeric_limits<double>::quiet_NaN(), c22at22CalB = std::numeric_limits<double>::quiet_NaN();
        {
            juce::StringArray lines;
            gGoldenDir.getChildFile ("cpu-calib-2.3.txt").readLines (lines);
            for (const auto& l : lines)
            {
                if (l.startsWith ("c22_22_pct ")) c22at22 = l.fromFirstOccurrenceOf (" ", false, false).getDoubleValue();
                if (l.startsWith ("c22_22_calib_before ")) c22at22CalB = l.fromFirstOccurrenceOf (" ", false, false).getDoubleValue();
            }
        }
        std::printf ("== --cpu23 (PLAN-2.3 T10): ref_ns %.0f (tests/golden/cpu-calib-2.3.txt)\n", refNs);
        std::vector<int> n16;
        for (int k = 0; k < 16; ++k) n16.push_back (40 + k);

        // bypass (streams disabled)
        const auto by = measureCfg ([] { return c22Session (kFs, kBlock, false, {}); }, n16, kFs, kBlock, refNs, 1);
        std::printf ("  INFO bypass (streams disabled): %s\n", cpuLine ("bypass", by, refNs).toRawUTF8());
        std::fflush (stdout);

        const auto r = measureCfg ([] { return c22Session (kFs, kBlock, true, {}); }, n16, kFs, kBlock, refNs);
        std::printf ("  INFO %s\n", cpuLine ("C22r (C22 recipe, FX off, same session)", r, refNs).toRawUTF8());
        std::fflush (stdout);

        const auto w = measureCfg ([] { return c22Session (kFs, kBlock, true, worstFx()); }, n16, kFs, kBlock, refNs);
        const bool wValid = ! w.slowed && ! w.contended && w.live >= 240.0;
        const bool wPass = wValid && w.pct <= 36.0;
        const double delta = w.pct - r.pct;
        row (wPass, "C23w D19 worst case: 16 notes x 4 streams, BP res 100 + drive 100 + crush 100 + ring 100, cap 256 <= 36 %",
             "meas " + cpuLine ("C23w", w, refNs) + " | thr <= 36 %, live >= 240, witness valid | margin " + f2 (36.0 - w.pct, 2)
                 + " points | delta vs C22r " + f2 (delta, 2) + " points = " + f2 (w.live > 0 ? delta / w.live : 0.0, 4) + " %/grain (parent 0.064)"
                 + (wValid ? "" : " | NOVERDICT = FAIL"));

        auto tSession = [] ()
        {
            auto p = cat ({ base4 (2), { { sn (1, "density"), 40.0f }, { sn (2, "density"), 40.0f }, { sn (1, "size"), 150.0f }, { sn (2, "size"), 150.0f },
                                         { sn (1, "spray"), 10.0f }, { sn (2, "spray"), 10.0f }, { sn (1, "jitter"), 20.0f }, { sn (2, "jitter"), 20.0f },
                                         { sn (1, "stereo_spread"), 30.0f }, { sn (2, "stereo_spread"), 30.0f },
                                         { "grain_limit", 3.0f }, { "polyphony", 8.0f },
                                         { sn (1, "filter_type"), 1.0f }, { sn (1, "filter_cutoff"), 1200.0f }, { sn (1, "filter_res"), 40.0f },
                                         { sn (1, "drive"), 30.0f } } });
            auto s = std::make_unique<Session> (kFs, p, kSeed, 128);
            shipping (*s);
            for (int k = 0; k < 4; ++k)
                s->load (k, noiseBuf (2, (int) (10.0 * kFs), 0.1f, 11 + k), kFs);
            s->record = false;
            return s;
        };
        const auto t = measureCfg (tSession, { 48, 52, 55, 59, 62, 64 }, kFs, kBlock, refNs);
        const bool tValid = ! t.slowed && ! t.contended && t.live >= 60.0 && t.live <= 90.0;
        const bool tPass = tValid && t.pct <= 12.0;
        row (tPass, "C23t D20 typical proxy (PD1): 6-note chord, s1 + s2 d 40 / 150 ms, s1 LP 1.2 kHz res 40 + drive 30 <= 12 %",
             "meas " + cpuLine ("C23t", t, refNs) + " | thr <= 12 %, live in [60, 90], witness valid | margin " + f2 (12.0 - t.pct, 2) + " points"
                 + (tValid ? "" : " | INVALID / NOVERDICT = FAIL"));

        // C23s INFO: restructure cost, each scaled to the reference machine speed.
        const double rs = r.pct * refNs / r.calB, ps = c22at22 * refNs / c22at22CalB;
        std::printf ("  INFO C23s C22r %.2f %% (scaled %.2f) vs C22@2.2 %.2f %% (scaled %.2f): %+.2f points%s\n", r.pct, rs, c22at22, ps, rs - ps,
                     rs - ps > 0.6 ? "  FLAG > +0.6 (restructure cost; lever 3 / P18 candidate)" : "");

        const auto w96 = measureCfg ([] { return c22Session (96000.0, 128, true, worstFx()); }, n16, 96000.0, 128, refNs);
        std::printf ("  INFO C23w96 %s\n", cpuLine ("C23w @ 96 k / 128", w96, refNs).toRawUTF8());
        std::fflush (stdout);
        return (wPass ? 0 : 1) + (tPass ? 0 : 1);
    }
}} // namespace ::eg23
