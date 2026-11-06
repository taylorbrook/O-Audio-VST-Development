/*
  ==============================================================================

    O-Granulator — render harness, Stage 2 / Phase 2.6 hardening rows
    (PLAN-2.6 P94). Sibling: engine_gates_26b.h.

      TA1          D21 infinite tail, latency 0                          (T1)
      Z7s          census self-test + reach (R93, R94, P83)              (T2)
      Z7h1–5, Z7hn, Z7hr   R96 host non-finite input (ring + wet guard)  (T3)
      Z7a / Z7m1–8 / Z7f   QUAL-02 extremes, min-side hazards, fuzz      (T8)
      Z7d / Z7dn   decay to 340 s, FTZ on / off (`--long`)               (T8)
      HR1–HR3      96 / 192 k smoke (R101, R102)                         (T10)

  ==============================================================================
*/
#pragma once

//==============================================================================
// P82 — reverb-unit 1.2.0 privates through the explicit-instantiation member-
// access idiom (ref-2.6/probe.cpp:32-44): access checks are waived for names in
// an explicit instantiation ([temp.spec]). Harness-side only; NO module change.
// A module rename breaks this harness build, never the plugin (accepted, PLAN-2.6).
//==============================================================================
namespace { namespace eg26
{
    template <typename Tag> struct TagBase { friend auto get (Tag); };
    template <typename Tag, auto M> struct Robber { friend auto get (Tag) { return M; } };
    struct TankTag  : TagBase<TankTag>  {};
    struct TFiltTag : TagBase<TFiltTag> {};
    struct DiffTag  : TagBase<DiffTag>  {};
    struct DStTag   : TagBase<DStTag>   {};
    struct PreLTag  : TagBase<PreLTag>  {};
    struct PreRTag  : TagBase<PreRTag>  {};
    struct BufTag   : TagBase<BufTag>   {};
    struct ZTag     : TagBase<ZTag>     {};
} }
template struct eg26::Robber<eg26::TankTag,  &ReverbUnit::tankDelays>;
template struct eg26::Robber<eg26::TFiltTag, &ReverbUnit::tankFilters>;
template struct eg26::Robber<eg26::DiffTag,  &ReverbUnit::diffusionDelays>;
template struct eg26::Robber<eg26::DStTag,   &ReverbUnit::diffusionState>;
template struct eg26::Robber<eg26::PreLTag,  &ReverbUnit::preDelayL>;
template struct eg26::Robber<eg26::PreRTag,  &ReverbUnit::preDelayR>;
template struct eg26::Robber<eg26::BufTag,   &ReverbUnit::DelayLine::buffer>;
template struct eg26::Robber<eg26::ZTag,     &ReverbUnit::OnePole::z>;

namespace { namespace eg26
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;

    inline void info (const juce::String& s) { std::printf ("  [INFO] %s\n", s.toRawUTF8()); }

    //==========================================================================
    // Builders (P84, P86): the C7 config, the R84 stimulus, the Q set.
    //==========================================================================
    using Stim = std::function<float (juce::int64, int)>;
    namespace D = ogran::ModDest;
    using eg25b::route;
    using eg25b::operator+;
    inline juce::String sp (int s, const char* f) { return eg22::sp (s, f); }
    inline P perStream (const char* f, float v) { P r; for (int s = 1; s <= 4; ++s) r.push_back ({ sp (s, f), v }); return r; }

    /** The 16 C7 routes (ref-2.6/cpu.cpp:170-172, route 13 → s1_level per R84). */
    constexpr int kC7Dests[16] { D::stream (0, D::Size), D::stream (1, D::Pitch), D::stream (2, D::FilterCutoff), D::stream (3, D::Drive),
                                 D::Mix, D::Feedback, D::FbSat, D::RevSize,
                                 D::stream (0, D::Level), D::stream (1, D::Pan), D::stream (2, D::Density), D::stream (3, D::Position),
                                 D::stream (0, D::Level), D::OutputGain, D::LfoRate1 + 1, D::stream (0, D::RingMix) };
    constexpr int kC7Srcs[16] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 1, 2, 5, 6, 7 };

    /** D22 as amended by sign-offs 2 and 3: everything maxed, crush 1 %, no input_gain route. */
    inline P c7Params (float routeDepth = 100.0f, float lfoHz = 20.0f)
    {
        P p { { "mix", 100.0f }, { "grain_limit", 3.0f }, { "buffer_length", 30.0f }, { "input_gain", 0.0f },
              { "feedback", 100.0f }, { "fb_diffusion", 100.0f }, { "fb_saturation", 100.0f }, { "fb_low_cut", 2000.0f }, { "fb_high_cut", 20000.0f },
              { "reverb_mix", 100.0f }, { "reverb_size", 100.0f }, { "reverb_damp", 100.0f }, { "onset_threshold", -45.0f }, { "sync_mode", 0.0f } };
        const int major = eg25y::scaleChoice ("Major");
        for (int s = 1; s <= 4; ++s)
            p = p + P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 200.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "spray"), 100.0f },
                        { sp (s, "position"), 100.0f }, { sp (s, "size_rand"), 100.0f }, { sp (s, "jitter"), 100.0f }, { sp (s, "pitch"), 24.0f },
                        { sp (s, "pitch_rand"), 24.0f }, { sp (s, "reverse_prob"), 100.0f }, { sp (s, "window"), 1.0f }, { sp (s, "window_skew"), 100.0f },
                        { sp (s, "stereo_spread"), 100.0f }, { sp (s, "gain_rand"), 100.0f }, { sp (s, "filter_type"), 1.0f },
                        { sp (s, "filter_cutoff"), 20000.0f }, { sp (s, "filter_rand"), 100.0f }, { sp (s, "filter_res"), 100.0f },
                        { sp (s, "drive"), 100.0f }, { sp (s, "crush"), 1.0f }, { sp (s, "crush_prob"), 100.0f },
                        { sp (s, "ring_freq"), 5000.0f }, { sp (s, "ring_rand"), 100.0f }, { sp (s, "ring_mix"), 100.0f },
                        { sp (s, "onset_trigger"), 1.0f }, { "pitch_follow_s" + juce::String (s), 1.0f }, { sp (s, "scale"), (float) major },
                        { sp (s, "level"), 6.0f } };
        for (int n = 1; n <= 4; ++n)
            p = p + P { { "lfo" + juce::String (n) + "_rate", lfoHz }, { "lfo" + juce::String (n) + "_shape", (float) (n + 1) },
                        { "macro" + juce::String (n), 100.0f } };
        for (int m = 0; m < 16; ++m)
            p = p + route (m + 1, kC7Srcs[m], kC7Dests[m], m % 2 ? -routeDepth : routeDepth);
        return p;
    }

    /** R84: 220 Hz sine 0.2 + 3 ms Hann-gated 10 kHz bursts at 0.8 every 52 ms, both
        channels; a precomputed 10 s buffer, looped (ref-2.6/cpu.cpp makeStim). */
    inline std::shared_ptr<std::vector<float>> c7Buffer (double fs)
    {
        auto x = std::make_shared<std::vector<float>> ((size_t) (10.0 * fs));
        for (size_t i = 0; i < x->size(); ++i)
        {
            const double t = (double) i / fs, tc = std::fmod (t, 0.052);
            double v = 0.2 * std::sin (2.0 * eg::kPi * 220.0 * t);
            if (tc < 0.003) v += 0.8 * std::sin (eg::kPi * tc / 0.003) * std::sin (2.0 * eg::kPi * 10000.0 * t);
            (*x)[i] = (float) v;
        }
        return x;
    }
    inline Stim c7Stim (double fs, float noiseAmp = 0.0f)
    {
        auto b = c7Buffer (fs);
        return [b, noiseAmp] (juce::int64 i, int ch)
        {
            const float v = (*b)[(size_t) (i % (juce::int64) b->size())];
            return noiseAmp > 0.0f ? v + eg::hashNoise (i, ch, noiseAmp) : v;
        };
    }

    /** P86 Q set: the C7 config with crush off, reverb off, LFOs 2 Hz, route depths ±30 %. */
    inline P qSet()
    {
        return c7Params (30.0f, 2.0f) + perStream ("crush_prob", 0.0f) + P { { "reverb_mix", 0.0f } };
    }

    //==========================================================================
    // P82 FTZ-off render path (R93): Engine::process directly, outside
    // processBlock's ScopedNoDenormals. The caller asserts the probe first.
    //==========================================================================
    inline bool ftzOffOnThisThread()
    {
        volatile float a = 1.0e-30f, b = 1.0e-10f;
        const float c = a * b;
        return std::fpclassify (c) == FP_SUBNORMAL;
    }
    inline void processDirect (Proc& p, juce::AudioBuffer<float>& buf)
    {
        const float* in[2] { buf.getReadPointer (0), buf.getReadPointer (buf.getNumChannels() > 1 ? 1 : 0) };
        float* out[2] { buf.getWritePointer (0), buf.getNumChannels() > 1 ? buf.getWritePointer (1) : nullptr };
        p.getEngineForTest().process (in, 2, out, buf.getNumChannels() > 1 ? 2 : 1, buf.getNumSamples(), p.getSeed(), {});
    }

    //==========================================================================
    // P83 census (R93, R94): subnormal + non-finite count per persistent state.
    // Runs between blocks (outside ScopedNoDenormals: classification unflushed).
    //==========================================================================
    struct Census
    {
        enum K { Grains, Chain, Diffuser, Ring, FbDelay, Tank, TankFilt, Diffusion, DiffState, PreL, PreR,
                 Analysis, Env, Lfo, Residual, Sources, NumK };
        static const char* name (int k)
        {
            static const char* n[NumK] { "grains", "fb chain", "diffuser", "ring", "fbDelay", "tank", "tank filt", "diffusion",
                                         "diff state", "predelay L", "predelay R", "analysis", "env", "lfo", "residual", "sources" };
            return n[k];
        }
        long long sub[NumK] {}, nonFin[NumK] {}, scanned[NumK] {};
        long long subTotal() const    { long long t = 0; for (auto x : sub) t += x; return t; }
        long long nonFinTotal() const { long long t = 0; for (auto x : nonFin) t += x; return t; }
        void add (const Census& c) { for (int k = 0; k < NumK; ++k) { sub[k] += c.sub[k]; nonFin[k] += c.nonFin[k]; } }
        juce::String worst() const
        {
            juce::String s;
            for (int k = 0; k < NumK; ++k)
                if (sub[k] || nonFin[k]) s << name (k) << " " << sub[k] << "/" << nonFin[k] << " ";
            return s.isEmpty() ? juce::String ("0/0") : s.trim();
        }
    };

    /** Subnormal only: states whose NaN is a by-design sentinel (D20 guard medians: NaN = unconfident hop). */
    template <typename T> inline void tallySub (Census& c, int k, T x)
    {
        ++c.scanned[k];
        if (std::fpclassify (x) == FP_SUBNORMAL) ++c.sub[k];
    }
    template <typename T> inline void tally (Census& c, int k, T x)
    {
        ++c.scanned[k];
        if (std::fpclassify (x) == FP_SUBNORMAL) ++c.sub[k];
        if (! std::isfinite (x)) ++c.nonFin[k];
    }

    /** ringBackS < 0: every captured sample of every level (the render-end scan). */
    inline Census census (const ogran::Engine& e, double ringBackS = 1.0)
    {
        Census c;
        using K = Census::K;
        for (int k = 0; k < ogran::kPoolLive + ogran::kPoolFade; ++k)
        {
            const auto& g = e.getPool().getGrains()[(size_t) k];
            ++c.scanned[K::Grains];
            if (eg23b::anySubnormal (g)) ++c.sub[K::Grains];
            bool bad = false;
            for (int ch = 0; ch < 2; ++ch)
                bad = bad || ! std::isfinite (g.svfIc1[ch]) || ! std::isfinite (g.svfIc2[ch]) || ! std::isfinite (g.crushHeld[ch]);
            bad = bad || ! std::isfinite (g.ringRe) || ! std::isfinite (g.ringIm) || ! std::isfinite (g.killGain) || ! std::isfinite (g.readAbs);
            if (bad) ++c.nonFin[K::Grains];
        }
        const auto& ch = e.getChainForTest();
        for (int k = 0; k < 2; ++k)
        {
            tally (c, K::Chain, ch.hpIc1[k]); tally (c, K::Chain, ch.hpIc2[k]);
            tally (c, K::Chain, ch.lpIc1[k]); tally (c, K::Chain, ch.lpIc2[k]);
        }
        tally (c, K::Chain, ch.env);
        for (int k = 0; k < 2; ++k)
            for (int s = 0; s < 4; ++s)
                for (int i = 0; i < ogran::kDiffuserMaxLen; ++i) tally (c, K::Diffuser, ch.line[k][s][i]);
        const auto& ring = e.getRingForTest();
        for (int l = 0; l <= ogran::kMaxLevel; ++l)
        {
            const std::int64_t n = ring.getLevelCount (l);
            const std::int64_t have = std::min<std::int64_t> (n, (std::int64_t) ring.getCapacity (l));
            const std::int64_t back = ringBackS < 0.0 ? have : std::min<std::int64_t> (have, (std::int64_t) (ringBackS * e.getSampleRate()) >> l);
            for (std::int64_t j = n - back; j < n; ++j)
                for (int k = 0; k < 2; ++k) tally (c, K::Ring, ring.levelSample (l, k, j));
        }
        for (const auto& row : e.getFbDelayForTest())
            for (float x : row) tally (c, K::FbDelay, x);
        const ReverbUnit& rev = e.getReverbForTest();
        for (const auto& d : rev.*get (TankTag {}))
            for (float x : d.*get (BufTag {})) tally (c, K::Tank, x);
        for (const auto& f : rev.*get (TFiltTag {})) tally (c, K::TankFilt, f.*get (ZTag {}));
        for (const auto& st : rev.*get (DiffTag {}))
            for (const auto& d : st)
                for (float x : d.*get (BufTag {})) tally (c, K::Diffusion, x);
        for (const auto& st : rev.*get (DStTag {}))
            for (float x : st) tally (c, K::DiffState, x);
        for (float x : (rev.*get (PreLTag {})).*get (BufTag {})) tally (c, K::PreL, x);
        for (float x : (rev.*get (PreRTag {})).*get (BufTag {})) tally (c, K::PreR, x);
        const auto& a = e.getAnalysis();
        const auto& det = a.getDetector();
        tally (c, K::Analysis, det.hpState()); tally (c, K::Analysis, det.fastEnv()); tally (c, K::Analysis, det.slowEnv());
        for (float x : a.getDecimator().history()) tally (c, K::Analysis, x);
        for (float x : a.getWindow()) tally (c, K::Analysis, x);
        for (int k = 0; k <= a.getMpm().getTauMax() + 1; ++k) tally (c, K::Analysis, a.getMpm().nsdfData()[k]);
        for (float x : a.getGuard().medians()) tallySub (c, K::Analysis, x);
        tally (c, K::Analysis, a.getGuard().accepted());
        tally (c, K::Analysis, a.getTracker().source());
        tally (c, K::Env, e.getEnv().state());
        for (int n = 0; n < ogran::kLfos; ++n) tally (c, K::Lfo, e.getLfo (n).value());
        for (int d = 1; d < ogran::kModDests; ++d) tally (c, K::Residual, e.getMatrix().residualValue (d));
        for (int k = 1; k < ogran::kModSources; ++k) tally (c, K::Sources, e.getSources().v[k]);
        return c;
    }

    //==========================================================================
    // Z7s — census self-test + reach (T2)
    //==========================================================================
    void censusRows()
    {
        std::printf ("\n-- Z7s. census self-test and reach (P83)\n");
        volatile float planted = 1.0e-40f;
        const bool a = std::fpclassify ((float) planted) == FP_SUBNORMAL;
        Census pc;
        float arr[16] {};
        for (int i = 0; i < 16; ++i) arr[i] = 0.25f * (float) i;
        arr[5] = planted;
        arr[11] = std::numeric_limits<float>::quiet_NaN();
        for (float x : arr) tally (pc, Census::Ring, x);
        const bool b = pc.sub[Census::Ring] == 1 && pc.nonFin[Census::Ring] == 1 && pc.scanned[Census::Ring] == 16;

        Job j;
        j.fs = 48000.0; j.len = 96000; j.block = 128; j.prepBlock = 128;
        j.params = c7Params();
        j.stim = c7Stim (j.fs);
        j.seed = 0xC2C2ull;
        const auto o = eg::render (j);
        const auto& e = o.eng();
        const auto c = census (e, 1.0);
        const auto& ring = e.getRingForTest();
        long long ringWant = 0;
        for (int l = 0; l <= ogran::kMaxLevel; ++l)
            ringWant += 2 * std::min<long long> ({ (long long) ring.getLevelCount (l), (long long) ring.getCapacity (l), (long long) 48000 >> l });
        using K = Census::K;
        Tally t;
        auto want = [&] (int k, long long n) { t.check (c.scanned[k] == n, juce::String (Census::name (k)) + juce::String::formatted (" %lld != %lld", c.scanned[k], n)); };
        want (K::Grains, ogran::kPoolLive + ogran::kPoolFade);
        want (K::Chain, 9);
        want (K::Diffuser, 2LL * 4 * ogran::kDiffuserMaxLen);
        want (K::Ring, ringWant);
        want (K::FbDelay, 2LL * ogran::kFbDelay);
        want (K::TankFilt, 8);
        want (K::DiffState, 32);
        want (K::Env, 1);
        want (K::Lfo, ogran::kLfos);
        want (K::Residual, ogran::kModDests - 1);
        want (K::Sources, ogran::kModSources - 1);
        for (int k : { (int) K::Tank, (int) K::Diffusion, (int) K::PreL, (int) K::PreR, (int) K::Analysis })
            t.check (c.scanned[k] > 0, juce::String (Census::name (k)) + " scanned 0");
        const bool zero = c.subTotal() == 0 && c.nonFinTotal() == 0;
        const bool live = e.diag.spawns > 0 && e.diag.reverbCells > 0;
        juce::String sizes;
        for (int k = 0; k < Census::NumK; ++k) sizes << Census::name (k) << " " << c.scanned[k] << ", ";
        row (a && b && t.bad == 0 && zero && live, "Z7s census: planted 1e-40f / NaN seen 1 / 1; 2 s of C7 @ 48 k: every state scanned at its declared size, 0 subnormal / 0 non-finite",
             juce::String::formatted ("probe %d, planted %lld/%lld; ", (int) a, pc.sub[Census::Ring], pc.nonFin[Census::Ring]) + t.detail (16) + "; " + c.worst()
                 + juce::String::formatted ("; spawns %lld reverbCells %lld", (long long) e.diag.spawns, (long long) e.diag.reverbCells));
        info ("Z7s sizes: " + sizes.dropLastCharacters (2));
    }

    //==========================================================================
    // Z7h — R96 host non-finite input (T3)
    //==========================================================================
    /** Injections: {absolute sample, channel mask (1 L, 2 R, 3 both), value}. */
    struct Inj { juce::int64 at; int mask; float v; };
    inline Stim injected (Stim base, std::vector<Inj> inj)
    {
        return [base, inj] (juce::int64 i, int ch)
        {
            for (const auto& x : inj)
                if (x.at == i && (x.mask & (1 << ch)) != 0) return x.v;
            return base (i, ch);
        };
    }
    inline Stim sine220 (double fs) { return [fs] (juce::int64 i, int) { return (float) (0.2 * std::sin (2.0 * eg::kPi * 220.0 * (double) i / fs)); }; }

    /** ref-2.6/probe.cpp:176-177: s1 50 g/s × 200 ms, spray 50, position 5, mix 50, fb 50, reverb 30; no route / trigger / follow. */
    inline P z7hBase()
    {
        return eg22::base ({ 1 }) + P { { "mix", 50.0f }, { "feedback", 50.0f }, { "reverb_mix", 30.0f }, { "s1_density", 50.0f }, { "s1_size", 200.0f },
                                        { "s1_spray", 50.0f }, { "s1_position", 5.0f } };
    }

    struct HostRes
    {
        bool finiteAfter = true; juce::int64 firstBad = -1; int nonFiniteOut = 0;
        bool lastBlockBad = false;
        Census end;
        long long hostScrubs = 0, wetScrubs = 0, fbScrubs = 0, nanKills = 0, envScrubs = 0;
        int envPinned = 0;   // cells with the Env source >= 0.999 (Z7h3 INFO)
    };
    inline HostRes hostResult (const Out& o, const std::vector<juce::int64>& ats)
    {
        HostRes r;
        auto isAt = [&ats] (juce::int64 i) { for (auto a : ats) if (a == i) return true; return false; };
        const juce::int64 first = ats.empty() ? 0 : *std::min_element (ats.begin(), ats.end());
        for (size_t i = 0; i < o.L.size(); ++i)
        {
            const bool bad = ! std::isfinite (o.L[i]) || ! std::isfinite (o.R[i]);
            if (! bad) continue;
            ++r.nonFiniteOut;
            if ((juce::int64) i > first && ! isAt ((juce::int64) i)) { r.finiteAfter = false; if (r.firstBad < 0) r.firstBad = (juce::int64) i; }
            if (i + 512 >= o.L.size()) r.lastBlockBad = true;
        }
        r.end = census (o.eng(), -1.0);
        const auto& d = o.eng().diag;
        r.hostScrubs = d.hostScrubs; r.wetScrubs = d.wetScrubs; r.fbScrubs = d.fbScrubs; r.nanKills = d.nanKills; r.envScrubs = d.envScrubs;
        return r;
    }
    inline long long auditedNonFinite (const Census& c)
    {
        using K = Census::K;
        return c.nonFin[K::Ring] + c.nonFin[K::Tank] + c.nonFin[K::TankFilt] + c.nonFin[K::Diffusion] + c.nonFin[K::DiffState]
             + c.nonFin[K::PreL] + c.nonFin[K::PreR] + c.nonFin[K::Chain] + c.nonFin[K::Diffuser] + c.nonFin[K::FbDelay];
    }
    /** Samples > at that differ (bitwise) from the reference. */
    inline int diffAfter (const Out& a, const Out& b, juce::int64 at)
    {
        int n = 0;
        for (size_t i = (size_t) at + 1; i < a.L.size() && i < b.L.size(); ++i)
            if (std::memcmp (&a.L[i], &b.L[i], 4) != 0 || std::memcmp (&a.R[i], &b.R[i], 4) != 0) ++n;
        return n;
    }
    inline void envPinCounter (Job& j, int* pinned)
    {
        auto prev = j.after;
        j.after = [prev, pinned] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            if (p.getEngineForTest().getSources().v[ogran::ModSrc::Env] >= 0.999f) ++*pinned;
        };
    }

    void hostInputRows()
    {
        std::printf ("\n-- Z7h. R96 host non-finite / absurd input: ring-write guard + wet guard\n");
        const double rates[4] { 44100.0, 48000.0, 96000.0, 192000.0 };
        const float vals[3] { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 1.0e30f };
        const char* vnm[3] { "NaN", "+Inf", "1e30" };
        const char* mnm[3] { "L", "L+R", "mono" };
        Tally tv[3];
        juce::String envInfo;
        std::vector<int> pins;
        for (double fs : rates)
        {
            const juce::int64 at = (juce::int64) fs + 5;
            std::vector<Job> jobs;
            pins.assign (24, 0);
            for (int m = 0; m < 3; ++m)
                for (int v = -1; v < 3; ++v)   // −1 = the reference (the sample(s) set to 0)
                {
                    Job j;
                    j.fs = fs; j.len = (juce::int64) (12.0 * fs); j.block = 128; j.prepBlock = 128;
                    j.inCh = m == 2 ? 1 : 2;
                    j.params = z7hBase();
                    j.stim = injected (sine220 (fs), { { at, m == 0 ? 1 : 3, v < 0 ? 0.0f : vals[v] } });
                    envPinCounter (j, &pins[(size_t) jobs.size()]);
                    jobs.push_back (j);
                }
            auto o = eg25::renderAll (jobs);
            for (int m = 0; m < 3; ++m)
            {
                const auto& ref = o[(size_t) (m * 4)];
                const int want = m == 0 ? 1 : 2;
                for (int v = 0; v < 3; ++v)
                {
                    const size_t k = (size_t) (m * 4 + 1 + v);
                    const auto r = hostResult (o[k], { at });
                    const int dd = diffAfter (o[k], ref, at);
                    const long long nf = auditedNonFinite (r.end);
                    tv[v].check (r.finiteAfter && dd == 0 && nf == 0 && r.hostScrubs == want && r.wetScrubs == 0 && r.fbScrubs == 0 && r.nanKills == 0,
                                 juce::String::formatted ("%g k %s: firstBad %lld, %d diffs after, census nonfinite %lld, host %lld (want %d) wet %lld fb %lld nanKills %lld",
                                                          fs / 1000.0, mnm[m], (long long) r.firstBad, dd, nf, r.hostScrubs, want, r.wetScrubs, r.fbScrubs, r.nanKills));
                    if (v == 2 && m == 0)
                        envInfo << juce::String::formatted ("%g k: %d vs %d cells; ", fs / 1000.0, pins[k], pins[(size_t) (m * 4)]);
                }
            }
        }
        for (int v = 0; v < 3; ++v)
            row (tv[v].bad == 0, juce::String ("Z7h") + juce::String (v + 1) + " host " + vnm[v]
                                     + " at fs + 5 (L / L+R / mono, 4 rates): finite after, bitwise == zero-sample ref after, census nonfinite 0, hostScrubs exact",
                 tv[v].detail (12));
        info ("Z7h3 Env source pinned at >= 0.999 (blocks of 128) after a 1e30 host sample vs the zero-sample reference (P81, decision at verify): " + envInfo.trim());

        // Z7h4 — inside the release crossfade.
        {
            const double fs = 48000.0;
            const juce::int64 rel = 4 * 48000, at = rel + 100;
            auto mk = [&] (float v, bool* xf)
            {
                Job j;
                j.fs = fs; j.len = (juce::int64) (8.0 * fs); j.block = 128; j.prepBlock = 128;
                j.params = z7hBase();
                j.stim = injected (sine220 (fs), { { at, 1, v } });
                j.events.push_back ({ 2 * 48000, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
                j.events.push_back ({ rel, [] (Proc& p) { eg::setP (p, "freeze", 0.0f); } });
                j.events.push_back ({ at, [] (Proc&) {} });   // a block boundary at the injection
                j.after = [xf, atv = at] (Proc& p, juce::int64 pos) { if (pos == atv) *xf = p.getEngineForTest().getFreezeForTest().xfading; };
                return j;
            };
            bool xs = false, xr = false;
            auto o = eg25::renderAll ({ mk (std::numeric_limits<float>::quiet_NaN(), &xs), mk (0.0f, &xr) });
            const auto r = hostResult (o[0], { at });
            const int dd = diffAfter (o[0], o[1], at);
            const long long nf = auditedNonFinite (r.end);
            row (xs && r.finiteAfter && dd == 0 && nf == 0 && r.hostScrubs == 1 && r.wetScrubs == 0 && r.fbScrubs == 0 && r.nanKills == 0
                     && o[0].eng().diag.freezeReleases >= 1,
                 "Z7h4 host NaN at the release tick + 100 (inside the 10 ms content crossfade): the Z7h1 gates",
                 juce::String::formatted ("xfading %d, firstBad %lld, %d diffs, census nonfinite %lld, host %lld wet %lld fb %lld nanKills %lld, releases %lld",
                                          (int) xs, (long long) r.firstBad, dd, nf, r.hostScrubs, r.wetScrubs, r.fbScrubs, r.nanKills,
                                          (long long) o[0].eng().diag.freezeReleases));
        }

        // Z7h5 — full feature (C7) at 4 rates: NaN L @ 1 s, +Inf R @ 3 s, 1e30 both @ 5 s.
        {
            std::vector<Job> jobs;
            std::vector<std::vector<juce::int64>> ats;
            for (double fs : rates)
            {
                Job j;
                j.fs = fs; j.len = (juce::int64) (7.0 * fs); j.block = 128; j.prepBlock = 128;
                j.params = c7Params();
                j.seed = 0xC2C2ull;
                const juce::int64 a1 = (juce::int64) fs, a3 = 3 * (juce::int64) fs, a5 = 5 * (juce::int64) fs;
                j.stim = injected (c7Stim (fs), { { a1, 1, std::numeric_limits<float>::quiet_NaN() }, { a3, 2, std::numeric_limits<float>::infinity() },
                                                  { a5, 3, 1.0e30f } });
                jobs.push_back (j);
                ats.push_back ({ a1, a3, a5 });
            }
            auto o = eg25::renderAll (jobs);
            Tally t;
            for (size_t k = 0; k < 4; ++k)
            {
                const auto r = hostResult (o[k], ats[k]);
                const long long nf = auditedNonFinite (r.end);
                // finite from each at + 1: the injected samples themselves reach the output on the
                // dry path (NaN · dg is NaN even at mix 100 % — the host's own sample, out of scope, P81)
                t.check (r.finiteAfter && r.nonFiniteOut <= 3 && nf == 0 && r.hostScrubs == 4,
                         juce::String::formatted ("%g k: firstBad %lld, nonfinite out %d (the injected samples), census nonfinite %lld, host %lld (want 4)",
                                                  rates[k] / 1000.0, (long long) r.firstBad, r.nonFiniteOut, nf, r.hostScrubs));
            }
            row (t.bad == 0, "Z7h5 C7 full feature, 4 rates: NaN L @ 1 s, +Inf R @ 3 s, 1e30 both @ 5 s: finite from each at + 1, census nonfinite 0, hostScrubs 4", t.detail (4));
        }

        // Z7hn — both guards off reproduces R96; Z7hr — ring guard off, wet guard on.
        {
            const double fs = 48000.0;
            const juce::int64 at = 48000 + 5;
            auto mk = [&] (bool wetOff)
            {
                Job j;
                j.fs = fs; j.len = (juce::int64) (12.0 * fs); j.block = 128; j.prepBlock = 128;
                j.params = z7hBase();
                j.stim = injected (sine220 (fs), { { at, 1, std::numeric_limits<float>::quiet_NaN() } });
                j.pre = [wetOff] (Proc& p) { p.getEngineForTest().hooks.hostGuardOff = true; p.getEngineForTest().hooks.wetGuardOff = wetOff; };
                return j;
            };
            auto o = eg25::renderAll ({ mk (true), mk (false) });
            const auto n = hostResult (o[0], { at }), r = hostResult (o[1], { at });
            using K = Census::K;
            row (n.lastBlockBad && n.end.nonFin[K::Tank] > 0 && n.end.nonFin[K::Ring] > 0,
                 "Z7hn both guards off: a host NaN on L reproduces R96 (non-finite output to the render end, tank and ring poisoned)",
                 juce::String::formatted ("non-finite out %d, last block bad %d, ring %lld tank %lld (R96 probe: 1 030 727 / tank 32 656)",
                                          n.nonFiniteOut, (int) n.lastBlockBad, n.end.nonFin[K::Ring], n.end.nonFin[K::Tank]));
            row (r.finiteAfter && r.end.nonFin[K::Ring] > 0 && r.wetScrubs > 0 && r.end.nonFin[K::Tank] == 0,
                 "Z7hr ring guard off, wet guard on: output finite after, ring poisoned, wetScrubs > 0, tank clean (the wet guard's reach)",
                 juce::String::formatted ("firstBad %lld, ring %lld, wetScrubs %lld, tank %lld, fbScrubs %lld, nanKills %lld",
                                          (long long) r.firstBad, r.end.nonFin[K::Ring], r.wetScrubs, r.end.nonFin[K::Tank], r.fbScrubs, r.nanKills));
        }
    }

    //==========================================================================
    // TA1 — D21 infinite tail (T1)
    //==========================================================================
    void tailRows()
    {
        std::printf ("\n-- TA. D21 tail length + latency\n");
        auto p = make();
        const double t0 = p->getTailLengthSeconds();
        p->prepareToPlay (48000.0, 512);
        const double t1 = p->getTailLengthSeconds();
        auto inf = [] (double t) { return std::isinf (t) && t > 0.0; };
        row (inf (t0) && inf (t1) && p->getLatencySamples() == 0, "TA1 getTailLengthSeconds() == +inf before and after prepareToPlay (48 k / 512); latency 0",
             juce::String::formatted ("tail %g / %g, latency %d", t0, t1, p->getLatencySamples()));
    }
} } // namespace eg26

static void engineRows26()
{
    std::printf ("\n== Stage 2 / 2.6 hardening rows (tail, census, host input, stress, high rate)\n");
    eg26::tailRows();
    eg26::censusRows();
    eg26::hostInputRows();
}
