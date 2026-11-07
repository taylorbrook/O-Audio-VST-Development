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
        // Execution deviation (re-derived, recorded): C7's position 100 % + spray 100 % over a 30 s
        // buffer reads 15–30 s back — every Q-set render shorter than that is silent (κ 0/0).
        // position 0 + buffer 2 s keeps spray 100 % and every other C7 setting, and reads live content.
        // Second re-derivation (execution deviation, recorded): under the literal Q set every
        // κ negative control is vacuous (K3bn == K3b 0.947x, FZ9an 1.13x, EN1n 1.1x) — the set's
        // own curvature swamps a click: window 1 + skew +100 (instant grain attacks), ring mod at
        // 5 kHz ± rand, ±24 st pitch + rand 24, res 100 % at random cutoffs. Those four are set
        // neutral here (Hann, ring off, pitch 0, res / filter_rand 0); everything else of C7 stays.
        return c7Params (30.0f, 2.0f) + perStream ("crush_prob", 0.0f) + perStream ("position", 0.0f)
             + perStream ("window", 0.0f) + perStream ("window_skew", 0.0f) + perStream ("ring_mix", 0.0f)
             + perStream ("pitch", 0.0f) + perStream ("pitch_rand", 0.0f) + perStream ("filter_res", 0.0f) + perStream ("filter_rand", 0.0f)
             + P { { "reverb_mix", 0.0f }, { "buffer_length", 2.0f } };
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
    inline void envPinCounter (Job& j, int* pinned, std::vector<float>* trace = nullptr)
    {
        auto prev = j.after;
        j.after = [prev, pinned, trace] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const float v = p.getEngineForTest().getSources().v[ogran::ModSrc::Env];
            if (v >= 0.999f) ++*pinned;
            if (trace != nullptr) trace->push_back (v);
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
        std::vector<std::vector<float>> envTr;
        Tally tPin;   // PLAN-4 P147: Z7h3 is a row (pinned cells == reference; L+R / mono Env trace bitwise == reference)
        for (double fs : rates)
        {
            const juce::int64 at = (juce::int64) fs + 5;
            std::vector<Job> jobs;
            pins.assign (24, 0);
            envTr.assign (24, {});
            for (int m = 0; m < 3; ++m)
                for (int v = -1; v < 3; ++v)   // −1 = the reference (the sample(s) set to 0)
                {
                    Job j;
                    j.fs = fs; j.len = (juce::int64) (12.0 * fs); j.block = 128; j.prepBlock = 128;
                    j.inCh = m == 2 ? 1 : 2;
                    j.params = z7hBase();
                    j.stim = injected (sine220 (fs), { { at, m == 0 ? 1 : 3, v < 0 ? 0.0f : vals[v] } });
                    envPinCounter (j, &pins[(size_t) jobs.size()], &envTr[(size_t) jobs.size()]);
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
                    if (v == 2)
                    {
                        const size_t rf = (size_t) (m * 4);
                        const bool same = m == 0 || envTr[k] == envTr[rf];   // L-only: the pin drops R's half of that one sample too
                        tPin.check (pins[k] == pins[rf] && same,
                                    juce::String::formatted ("%g k %s: %d vs %d cells, trace %s", fs / 1000.0, mnm[m], pins[k], pins[rf], same ? "==" : "differs"));
                        if (m == 0)
                            envInfo << juce::String::formatted ("%g k: %d vs %d cells; ", fs / 1000.0, pins[k], pins[rf]);
                    }
                }
            }
        }
        for (int v = 0; v < 3; ++v)
            row (tv[v].bad == 0, juce::String ("Z7h") + juce::String (v + 1) + " host " + vnm[v]
                                     + " at fs + 5 (L / L+R / mono, 4 rates): finite after, bitwise == zero-sample ref after, census nonfinite 0, hostScrubs exact",
                 tv[v].detail (12));
        row (tPin.bad == 0, "Z7h3e Env source after a 1e30 host sample: pinned (>= 0.999) cells == zero-sample ref; L+R / mono Env trace bitwise (4 rates)",
             tPin.bad ? tPin.detail (12) : envInfo.trim());
        // Z7h3n — the PLAN-4 P147 control: without the env-input predicate the same 1e30 sample pins the Env source.
        {
            Job j;
            j.fs = 48000.0; j.len = (juce::int64) (12.0 * 48000.0); j.block = 128; j.prepBlock = 128;
            j.params = z7hBase();
            j.stim = injected (sine220 (48000.0), { { 48005, 3, 1.0e30f } });
            j.pre = [] (Proc& p) { p.getEngineForTest().hooks.envPinOff = true; };
            int pinned = 0;
            envPinCounter (j, &pinned);
            eg25::renderAll ({ j });
            row (pinned > 0, "Z7h3n control: envPinOff, the same 1e30 sample (L+R, 48 k) pins the Env source", juce::String::formatted ("%d cells >= 0.999", pinned));
        }

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
    // Z7a / Z7m / Z7f / Z7d — QUAL-02 (P90)
    //==========================================================================
    /** A bounded parallel pool WITH events (eg25::setup / eg25::loop): at most
        `maxPar` renders in flight (192 k: 110 MB ring each), `consume` frees each. */
    inline void renderPoolEv (const std::vector<Job>& jobs, size_t maxPar, const std::function<void (size_t, Out&)>& consume)
    {
        const size_t hw = juce::jmax ((size_t) 1, juce::jmin ((size_t) std::thread::hardware_concurrency(), maxPar));
        for (size_t b0 = 0; b0 < jobs.size(); b0 += hw)
        {
            const size_t b1 = juce::jmin (jobs.size(), b0 + hw);
            std::vector<Out> outs;
            for (size_t i = b0; i < b1; ++i) outs.push_back (eg25::setup (jobs[i]));
            std::vector<std::thread> th;
            for (size_t i = b0; i < b1; ++i)
                th.emplace_back ([&jobs, &outs, i, b0] { eg25::loop (jobs[i], outs[i - b0]); });
            for (auto& t : th) t.join();
            for (size_t i = b0; i < b1; ++i) consume (i, outs[i - b0]);
        }
    }

    /** The P90 common gate's per-render record. */
    struct StressRec
    {
        bool finite = true; double peakIn = 0.0, peakOut = 0.0, wetPeak = 0.0;
        int scans = 0; Census worst; long long subScans = 0, nonFinScans = 0;
        long long fb = 0, env = 0, ana = 0, host = 0, wet = 0, nanKills = 0, reach = 0;
        bool guards = true;
        long long spawns = 0, kills = 0, steals = 0, engages = 0, events = 0, hops = 0;
        long long reachClamp = 0;   // Z7m3: records whose delay exceeds the user span (the reach clamp)
        bool truncated = false;     // Z7m8: an ActiveSet truncated at kMaxActive
        juce::int64 lastSec = 0;    // census cadence (second index of the last scan)
    };
    /** Scans the census every 1 s of render time (block-aligned) and at the end. */
    inline void attachCensus (Job& j, StressRec* r)
    {
        auto prev = j.after;
        const juce::int64 sec = (juce::int64) j.fs, len = j.len;
        j.after = [prev, r, sec, len] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const auto& e = p.getEngineForTest();
            const bool end = pos >= len;
            const juce::int64 s = pos / sec;   // once per second boundary crossed, and at the end
            if (end || s != r->lastSec)
            {
                r->lastSec = s;
                const auto c = census (e, end ? -1.0 : 1.0);
                ++r->scans;
                if (c.subTotal() > 0) ++r->subScans;
                if (c.nonFinTotal() > 0) ++r->nonFinScans;
                r->worst.add (c);
            }
            for (int st = 0; st < ogran::kMaxStreams; ++st)
                r->truncated = r->truncated || e.getActiveSet (st).wasTruncated() || e.getAbsSet (st).wasTruncated();
        };
    }
    inline void finishRec (StressRec& r, const Out& o, const Stim& stim, double fs)
    {
        for (size_t k = 0; k < o.L.size(); ++k)
        {
            if (! std::isfinite (o.L[k]) || ! std::isfinite (o.R[k])) r.finite = false;
            r.peakOut = juce::jmax (r.peakOut, (double) std::abs (o.L[k]), (double) std::abs (o.R[k]));
            r.peakIn = juce::jmax (r.peakIn, (double) std::abs (stim ((juce::int64) k, 0)), (double) std::abs (stim ((juce::int64) k, 1)));
        }
        juce::ignoreUnused (fs);
        const auto& e = o.eng();
        const auto& d = e.diag;
        r.wetPeak = d.maxWetPeak;
        r.fb = d.fbScrubs; r.env = d.envScrubs; r.ana = d.analysisScrubs; r.host = d.hostScrubs; r.wet = d.wetScrubs; r.nanKills = d.nanKills;
        r.reach = d.reachViolations; r.guards = e.guardsIntact();
        r.spawns = d.spawns; r.kills = d.kills; r.steals = d.steals; r.engages = d.freezeEngages; r.events = d.onsetEvents; r.hops = d.mpmHops;
    }
    inline double outBound (double peakIn)
    {
        // out = (dry·dg + wet·wg)·og, dg² + wg² <= 1, |wet| <= A, og <= +12 dB (Z5 split gate, measured dry peak)
        return std::sqrt (peakIn * peakIn + (double) ogran::kKneeA * ogran::kKneeA) * (double) ogran::dbToGain (12.0f) * (1.0 + 1e-6);
    }
    inline bool commonGate (const StressRec& r, juce::String& why)
    {
        const bool ok = r.finite && r.wetPeak <= (double) ogran::kKneeA && r.peakOut <= outBound (r.peakIn) && r.subScans == 0 && r.nonFinScans == 0
                        && r.fb == 0 && r.env == 0 && r.ana == 0 && r.host == 0 && r.wet == 0 && r.nanKills == 0 && r.reach == 0 && r.guards && r.scans >= 2;
        why = juce::String::formatted ("fin %d wet %.4f out %.3f (<= %.3f) census %lld/%lld of %d scans [%s] scrubs fb %lld env %lld ana %lld host %lld wet %lld nanK %lld reach %lld guards %d",
                                       (int) r.finite, r.wetPeak, r.peakOut, outBound (r.peakIn), r.subScans, r.nonFinScans, r.scans, r.worst.worst().toRawUTF8(),
                                       r.fb, r.env, r.ana, r.host, r.wet, r.nanKills, r.reach, (int) r.guards);
        return ok;
    }
    inline Stim z7Stim (double fs, double stimS)
    {
        auto c = c7Stim (fs, 0.25f);
        const juce::int64 end = (juce::int64) (stimS * fs);
        return [c, end] (juce::int64 i, int ch) { return i < end ? c (i, ch) : 0.0f; };
    }

    void z7aRows()
    {
        std::printf ("\n-- Z7a. QUAL-02 max composite: C7 + input / output gain routes, flips every 10 s, freeze x6; 60 s + 30 s silence, 4 rates\n");
        const double rates[4] { 44100.0, 48000.0, 96000.0, 192000.0 };
        std::vector<Job> jobs;
        std::vector<StressRec> rs (4);
        std::vector<Stim> stims;
        for (int i = 0; i < 4; ++i)
        {
            const double fs = rates[i];
            Job j;
            j.fs = fs; j.len = (juce::int64) (90.0 * fs); j.block = 512; j.prepBlock = 512; j.seed = 0x27A0ull + (std::uint64_t) i;
            j.params = c7Params() + P { { "mod13_dest", (float) D::InputGain }, { "mod13_depth", 100.0f }, { "mod14_dest", (float) D::OutputGain }, { "mod14_depth", -100.0f } };
            j.stim = z7Stim (fs, 60.0);
            auto ev = [&j, fs] (double t, std::function<void (Proc&)> f) { j.events.push_back ({ (juce::int64) (t * fs) + 7, std::move (f) }); };
            ev (30.0, [] (Proc& p) { eg::setP (p, "mod13_depth", -100.0f); eg::setP (p, "mod14_depth", 100.0f); });
            for (int k = 1; k <= 8; ++k)
            {
                const bool odd = k % 2 == 1;
                ev (10.0 * k, [odd] (Proc& p)
                {
                    for (int s = 1; s <= 4; ++s) { eg::setP (p, sp (s, "pitch"), odd ? -24.0f : 24.0f); eg::setP (p, sp (s, "reverse_prob"), odd ? 0.0f : 100.0f); }
                    eg::setP (p, "fb_low_cut", odd ? 20.0f : 2000.0f);
                    eg::setP (p, "fb_high_cut", odd ? 1000.0f : 20000.0f);
                    eg::setP (p, "reverb_damp", odd ? 0.0f : 100.0f);
                });
            }
            const double fzT[6] { 5.0, 12.0, 25.0, 33.0, 47.0, 55.0 };
            for (int k = 0; k < 6; ++k) { const float v = k % 2 == 0 ? 1.0f : 0.0f; ev (fzT[k], [v] (Proc& p) { eg::setP (p, "freeze", v); }); }
            attachCensus (j, &rs[(size_t) i]);
            stims.push_back (j.stim);
            jobs.push_back (j);
        }
        renderPoolEv (jobs, 8, [&] (size_t i, Out& o) { finishRec (rs[i], o, stims[i], rates[i]); });
        Tally t;
        juce::String det;
        for (int i = 0; i < 4; ++i)
        {
            juce::String why;
            const auto& r = rs[(size_t) i];
            const bool live = r.engages >= 3 && r.events > 100 && r.spawns > 10000 && r.steals > 0;
            t.check (commonGate (r, why) && live, juce::String::formatted ("%g k: ", rates[i] / 1000.0) + why);
            det << juce::String::formatted ("%g k: wet %.3f out %.3f scans %d engages %lld events %lld steals %lld; ", rates[i] / 1000.0, r.wetPeak, r.peakOut, r.scans,
                                            r.engages, r.events, r.steals);
        }
        row (t.bad == 0, "Z7a 90 s max composite x 4 rates: finite, wet <= A, out <= bound, census 0 / 0 every 1 s, 0 scrubs / nanKills / reach, guards", t.bad ? t.detail (4) : det.trim());
    }

    /** Z7m9 (D25, execution deviation): spawns inside the freeze deferral with r < 1 and grains
        shorter than the lead (Z7f fuzz states 13 / 22 / 60): freeze on from block 0 and a mid-run
        engage, 48 / 192 k. Z7m9n: the pre-2.6 r·(headLead + 1) floor reproduces the violations. */
    void z7m9Rows()
    {
        std::printf ("\n-- Z7m9. freeze deferral with sub-unity-rate short grains (from block 0 and mid-run)\n");
        std::vector<Job> jobs;
        std::vector<StressRec> rs (8);
        std::vector<Stim> stims;
        std::vector<double> fss;
        for (int nc = 0; nc < 2; ++nc)
            for (int mid = 0; mid < 2; ++mid)
                for (double fs : { 48000.0, 192000.0 })
                {
                    Job j;
                    j.fs = fs; j.len = (juce::int64) (4.0 * fs); j.block = 512; j.prepBlock = 512; j.seed = 0x29A9ull;
                    j.params = eg22::base ({ 1, 2, 3, 4 }) + perStream ("size", 5.0f) + perStream ("density", 200.0f) + perStream ("pitch", -18.0f)
                             + perStream ("pitch_rand", 2.0f) + perStream ("position", 0.0f) + perStream ("spray", 0.0f) + P { { "freeze", mid ? 0.0f : 1.0f } };
                    j.stim = c7Stim (fs, 0.25f);
                    if (mid)
                        for (int k = 0; k < 6; ++k)
                        {
                            const float v = k % 2 == 0 ? 1.0f : 0.0f;
                            j.events.push_back ({ (juce::int64) ((1.0 + 0.4 * k) * fs) + 7, [v] (Proc& p) { eg::setP (p, "freeze", v); } });
                        }
                    const bool old = nc == 1;
                    j.pre = [old] (Proc& p) { p.getEngineForTest().hooks.deferFloorR = old; };
                    attachCensus (j, &rs[jobs.size()]);
                    stims.push_back (j.stim);
                    fss.push_back (fs);
                    jobs.push_back (j);
                }
        renderPoolEv (jobs, 8, [&] (size_t i, Out& o) { finishRec (rs[i], o, stims[i], fss[i]); });
        Tally t;
        juce::String det, ndet;
        long long ncStart = 1, ncMid = 0;
        for (size_t i = 0; i < 8; ++i)
        {
            const auto& r = rs[i];
            const juce::String tag = juce::String ((i % 4) / 2 ? "mid-run" : "from block 0") + ((i % 2) ? " 192 k" : " 48 k");
            if (i < 4)
            {
                juce::String why;
                t.check (commonGate (r, why) && r.engages >= 1 && r.spawns > 100, tag + ": " + why);
                det << tag << juce::String::formatted (": reach %lld engages %lld spawns %lld; ", r.reach, r.engages, r.spawns);
            }
            else
            {
                if ((i % 4) / 2 == 0) ncStart = juce::jmin (ncStart, r.reach); else ncMid += r.reach;
                ndet << tag << juce::String::formatted (": reach %lld; ", r.reach);
            }
        }
        row (t.bad == 0, "Z7m9 freeze deferral, 5 ms grains at -18 st x 4 streams (from block 0 + 3 mid-run engages), 48 / 192 k: the P90 common gate (reach 0)",
             t.bad ? t.detail (4) : det.trim());
        row (ncStart > 0 && ncMid > 0, "Z7m9n deferFloorR (the pre-2.6 r * (headLead + 1) floor): reach violations from block 0 (both rates) and mid-run", ndet.trim());
    }

    void z7mRows()
    {
        std::printf ("\n-- Z7m. QUAL-02 min-side hazards, 10 s + 10 s silence @ 48 k and 192 k (P90)\n");
        struct Item { const char* name; P params; std::function<void (Job&)> mod; int group; };
        auto c7 = c7Params();
        std::vector<Item> items;
        // 1. cutoff 20 + res 100, LP / BP / HP, + rand 100 at cutoff 20
        items.push_back ({ "Z7m1 filter cutoff 20 Hz res 100 % LP / BP / HP + rand 100 % (g ~ 3.3e-4 @ 192 k)",
                           c7 + perStream ("filter_cutoff", 20.0f) + perStream ("filter_res", 100.0f) + perStream ("filter_rand", 0.0f)
                              + P { { "s1_filter_type", 1.0f }, { "s2_filter_type", 2.0f }, { "s3_filter_type", 3.0f }, { "s4_filter_type", 1.0f }, { "s4_filter_rand", 100.0f } }, {}, 1 });
        items.push_back ({ "Z7m2 ring 20 Hz + ring_rand 100 % (carrier clamps at 20)", c7 + perStream ("ring_freq", 20.0f) + perStream ("ring_rand", 100.0f), {}, 2 });
        items.push_back ({ "Z7m3 buffer 1 s, size 2 s, +24 st, reverse 100 %, live then frozen: reach clamp taken, 0 violations",
                           c7 + P { { "buffer_length", 1.0f } } + perStream ("size", 2000.0f) + perStream ("pitch", 24.0f) + perStream ("reverse_prob", 100.0f)
                              + perStream ("pitch_rand", 0.0f),
                           [] (Job& j) { j.events.push_back ({ (juce::int64) (5.0 * j.fs) + 3, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } }); }, 3 });
        items.push_back ({ "Z7m4a size 5 ms x 200 g/s x 4 (spawn-bound)", c7 + perStream ("size", 5.0f) + perStream ("size_rand", 0.0f), {}, 4 });
        items.push_back ({ "Z7m4b grain_limit 32 under demand 400 (kills)", c7 + perStream ("size", 500.0f) + perStream ("size_rand", 0.0f) + P { { "grain_limit", 0.0f } }, {}, 4 });
        items.push_back ({ "Z7m4c grain_limit 128 under demand 400 (steals)", c7 + perStream ("size", 500.0f) + perStream ("size_rand", 0.0f) + P { { "grain_limit", 3.0f } }, {}, 4 });
        items.push_back ({ "Z7m5 input_gain -24 dB on a -60 dBFS input; s_level -60 + gain_rand 100 % (-84 dB grains)",
                           c7 + P { { "input_gain", -24.0f } } + perStream ("level", -60.0f) + perStream ("gain_rand", 100.0f), [] (Job& j)
                           {
                               auto base = j.stim;
                               j.stim = [base] (juce::int64 i, int ch) { return 0.001f * base (i, ch); };
                           }, 5 });
        items.push_back ({ "Z7m6 env attack 1 ms / release 10 ms, LFO 0.01 Hz, onset threshold -60 (fires on the floor)",
                           c7 + P { { "env_attack", 1.0f }, { "env_release", 10.0f }, { "onset_threshold", -60.0f }, { "lfo1_rate", 0.01f }, { "lfo2_rate", 0.01f },
                                    { "lfo3_rate", 0.01f }, { "lfo4_rate", 0.01f } }, {}, 6 });
        items.push_back ({ "Z7m7a drive 0.26 % (d = 16 drive^2 just above kDriveGuard 1e-4), crush 1 %", c7 + perStream ("drive", 0.26f) + perStream ("crush", 1.0f), {}, 7 });
        items.push_back ({ "Z7m7b drive 0.24 % (just below the guard), crush 1 %", c7 + perStream ("drive", 0.24f) + perStream ("crush", 1.0f), {}, 7 });
        items.push_back ({ "Z7m8 Custom 'edo 64 1.001', scale + follow on x 4 (ActiveSet / AbsSet truncate at kMaxActive)",
                           c7 + perStream ("scale", (float) eg25y::scaleChoice ("Custom")), [] (Job& j)
                           {
                               auto pre = j.pre;
                               j.pre = [pre] (Proc& p) { if (pre) pre (p); juce::String bad; p.setCustomScale ("", "edo 64 1.001", bad); };
                           }, 8 });
        std::vector<Job> jobs;
        std::vector<StressRec> rs (items.size() * 2);
        std::vector<Stim> stims;
        std::vector<std::shared_ptr<eg22::RecLog>> logs;
        std::vector<double> fss;
        for (size_t it = 0; it < items.size(); ++it)
            for (double fs : { 48000.0, 192000.0 })
            {
                Job j;
                j.fs = fs; j.len = (juce::int64) (20.0 * fs); j.block = 512; j.prepBlock = 512; j.seed = 0x27B0ull + it;
                j.params = items[it].params;
                j.stim = z7Stim (fs, 10.0);
                if (items[it].mod) items[it].mod (j);
                auto L = std::make_shared<eg22::RecLog>();
                if (it == 2) j.after = eg22::collect (*L);
                logs.push_back (L);
                attachCensus (j, &rs[jobs.size()]);
                stims.push_back (j.stim);
                fss.push_back (fs);
                jobs.push_back (j);
            }
        renderPoolEv (jobs, 8, [&] (size_t i, Out& o)
        {
            finishRec (rs[i], o, stims[i], fss[i]);
            const double span = 1.0 * fss[i];   // Z7m3: buffer_length 1 s
            for (const auto& r : logs[i]->r) rs[i].reachClamp += r.delay > span ? 1 : 0;
        });
        const char* rowName[9] { "", "Z7m1 filter cutoff 20 Hz, res 100 %: LP / BP / HP, + rand 100 % at 20 Hz",
                                 "Z7m2 ring 20 Hz + ring_rand 100 % (carrier clamps at 20 Hz)",
                                 "Z7m3 buffer 1 s, size 2 s, +24 st, reverse 100 %, live then frozen: reach clamp taken, 0 violations",
                                 "Z7m4 size 5 ms x 200 g/s x 4 (spawn-bound); demand 400 at cap 32 (kills) and 128 (steals)",
                                 "Z7m5 input_gain -24 dB on a -60 dBFS input; s_level -60 + gain_rand 100 % (-84 dB grains)",
                                 "Z7m6 env 1 / 10 ms, LFO 0.01 Hz, onset threshold -60 (fires on the floor)",
                                 "Z7m7 drive 0.26 % / 0.24 % (d = 16 drive^2 either side of kDriveGuard 1e-4), crush 1 %",
                                 "Z7m8 Custom 'edo 64 1.001', scale + follow x 4: ActiveSet truncates at kMaxActive" };
        for (int g = 1; g <= 8; ++g)
        {
            Tally t;
            juce::String det;
            int total = 0;
            for (size_t it = 0; it < items.size(); ++it)
            {
                if (items[it].group != g) continue;
                for (int k = 0; k < 2; ++k)
                {
                    const auto& r = rs[it * 2 + (size_t) k];
                    juce::String why;
                    bool live = r.spawns > 100;
                    if (g == 3) live = live && r.reachClamp > 0 && r.engages >= 1;
                    if (it == 4) live = live && r.kills > 0;
                    if (it == 5) live = live && r.steals > 0;
                    if (g == 8) live = live && r.truncated;
                    const juce::String tag = juce::String (items[it].name).upToFirstOccurrenceOf (" ", false, false) + (k ? " 192 k" : " 48 k");
                    ++total;
                    t.check (commonGate (r, why) && live, tag + ": " + why + juce::String::formatted (" [spawns %lld kills %lld steals %lld clamp %lld trunc %d]",
                                                                                                    r.spawns, r.kills, r.steals, r.reachClamp, (int) r.truncated));
                    det << tag << juce::String::formatted (": wet %.3f out %.3f scans %d spawns %lld kills %lld steals %lld clamp %lld trunc %d; ",
                                                           r.wetPeak, r.peakOut, r.scans, r.spawns, r.kills, r.steals, r.reachClamp, (int) r.truncated);
                }
            }
            row (t.bad == 0, rowName[g], t.bad ? t.detail (total) : det.trim());
        }
    }

    /** Seeded uniform normalised value for parameter p of state k (draw r). */
    inline float fuzzU (std::uint64_t seed, int k, int r, int p)
    {
        std::uint64_t x = seed * 0x9E3779B97F4A7C15ull ^ ((std::uint64_t) k << 40) ^ ((std::uint64_t) r << 20) ^ (std::uint64_t) p;
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ull; x ^= x >> 27; x *= 0x94D049BB133111EBull; x ^= x >> 31;
        return (float) ((double) (x >> 40) / (double) (1ull << 24));
    }

    void z7fRows()
    {
        std::printf ("\n-- Z7f. QUAL-02 seeded full-parameter fuzz: 64 states x 2 s (8 jumps of 32 parameters each) per rate\n");
        const double rates[4] { 44100.0, 48000.0, 96000.0, 192000.0 };
        for (double fs : rates)
        {
            std::vector<Job> jobs;
            std::vector<StressRec> rs (64);
            std::vector<Stim> stims;
            for (int k = 0; k < 64; ++k)
            {
                Job j;
                j.fs = fs; j.len = (juce::int64) (2.0 * fs); j.block = 512; j.prepBlock = 512; j.seed = 0xF022ull + (std::uint64_t) k;
                j.stim = c7Stim (fs, 0.25f);
                j.pre = [k] (Proc& p)
                {
                    int i = 0;
                    for (auto* ap : p.getParameters()) ap->setValueNotifyingHost (fuzzU (0xF0221ull, k, 0, i++));
                };
                for (int r = 1; r <= 8; ++r)
                    j.events.push_back ({ (juce::int64) ((double) r * 0.22 * fs) + 13, [k, r] (Proc& p)
                    {
                        auto& ps = p.getParameters();
                        for (int q = 0; q < 32; ++q)
                        {
                            const int idx = (int) (fuzzU (0xF0222ull, k, r, q) * (float) ps.size()) % ps.size();
                            ps[idx]->setValueNotifyingHost (fuzzU (0xF0223ull, k, r, q));
                        }
                    } });
                attachCensus (j, &rs[(size_t) k]);
                stims.push_back (j.stim);
                jobs.push_back (j);
            }
            renderPoolEv (jobs, fs > 100000.0 ? 8 : 16, [&] (size_t i, Out& o) { finishRec (rs[i], o, stims[i], fs); });
            Tally t;
            long long spawns = 0, engages = 0;
            double wet = 0.0, out = 0.0;
            for (int k = 0; k < 64; ++k)
            {
                juce::String why;
                t.check (commonGate (rs[(size_t) k], why), juce::String::formatted ("state %d: ", k) + why);
                spawns += rs[(size_t) k].spawns; engages += rs[(size_t) k].engages;
                wet = juce::jmax (wet, rs[(size_t) k].wetPeak); out = juce::jmax (out, rs[(size_t) k].peakOut);
            }
            row (t.bad == 0 && spawns > 1000, juce::String::formatted ("Z7f fuzz @ %g k: 64 seeded states x 8 jumps: the P90 common gate", fs / 1000.0),
                 t.detail (64) + juce::String::formatted ("  [spawns %lld, freeze engages %lld, max wet %.3f, max out %.3f]", spawns, engages, wet, out));
        }
    }

    /** Z7d / Z7dn (--long): decay to 340 s, FTZ on (processBlock) vs off (Engine::process direct). */
    void z7dRows()
    {
        if (! gLong) return;
        std::printf ("\n-- Z7d. QUAL-02 decay to 340 s after 10 s of input (R81 config), FTZ on vs off (--long)\n");
        P cfg { { "mix", 100.0f }, { "grain_limit", 3.0f }, { "buffer_length", 30.0f }, { "feedback", 100.0f }, { "fb_diffusion", 100.0f },
                { "fb_saturation", 100.0f }, { "reverb_mix", 100.0f }, { "reverb_size", 100.0f }, { "reverb_damp", 0.0f } };
        for (int s = 1; s <= 4; ++s)
            cfg = cfg + P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 200.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "spray"), 100.0f },
                            { sp (s, "position"), 0.0f }, { sp (s, "pitch"), s % 2 ? 12.0f : -12.0f }, { sp (s, "filter_type"), 1.0f },
                            { sp (s, "filter_res"), 100.0f }, { sp (s, "filter_cutoff"), 2000.0f }, { sp (s, "level"), 6.0f } };
        struct Arm { double lastNonZero = -1.0; int scans = 0; long long subScans = 0, nonFinScans = 0; double firstSub = -1.0; Census worst; bool probe = false; double secs = 0.0; };
        Arm arms[2];
        auto run = [&] (int ftz, Arm& a)
        {
            const auto t0 = std::chrono::steady_clock::now();
            const double fs = 48000.0;
            const int block = 128;
            auto p = make();
            eg::setAll (*p, cfg);
            p->prepareToPlay (fs, block);
            a.probe = ftz == 1 ? true : ftzOffOnThisThread();
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer midi;
            const juce::int64 stimEnd = (juce::int64) (10.0 * fs), end = (juce::int64) (340.0 * fs);
            for (juce::int64 pos = 0; pos < end; pos += block)
            {
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < block; ++i)
                    {
                        const juce::int64 n = pos + i;
                        buf.setSample (c, i, n < stimEnd ? (float) (0.5 * std::sin (2.0 * eg::kPi * 220.0 * (double) n / fs) + 0.1 * std::sin (2.0 * eg::kPi * 3311.0 * (double) n / fs)) : 0.0f);
                    }
                if (ftz == 1) p->processBlock (buf, midi);
                else processDirect (*p, buf);
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < block; ++i)
                        if (buf.getSample (c, i) != 0.0f) a.lastNonZero = (double) (pos + i) / fs;
                if ((pos + block) % (juce::int64) fs < block)
                {
                    const auto cs = census (p->getEngineForTest(), 1.0);
                    ++a.scans;
                    if (cs.subTotal() > 0) { ++a.subScans; if (a.firstSub < 0.0) a.firstSub = (double) (pos + block) / fs; }
                    if (cs.nonFinTotal() > 0) ++a.nonFinScans;
                    a.worst.add (cs);
                }
            }
            a.secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        };
        std::thread on ([&] { run (1, arms[0]); }), off ([&] { run (0, arms[1]); });
        on.join(); off.join();
        const auto& a = arms[0];
        const auto& b = arms[1];
        row (a.lastNonZero >= 0.0 && a.lastNonZero < 329.0 && a.subScans == 0 && a.nonFinScans == 0 && a.scans >= 330,
             "Z7d FTZ on (processBlock): exact-zero output for >= 1 s before 330 s, census 0 / 0 at every 1 s scan",
             juce::String::formatted ("last non-zero @ %.2f s (R81: 295.63 s); %d scans, subnormal scans %lld, non-finite %lld (%.0f s wall)", a.lastNonZero, a.scans, a.subScans,
                                      a.nonFinScans, a.secs));
        row (b.probe && b.firstSub >= 0.0 && b.firstSub <= 300.0,
             "Z7dn FTZ off (Engine::process direct): the census sees subnormals by 300 s (the non-vacuity control)",
             juce::String::formatted ("FTZ-off probe %d; first subnormal scan @ %.0f s (R93: ring 246 s, reverb 262 s); [%s]; last non-zero @ %.2f s", (int) b.probe, b.firstSub,
                                      b.worst.worst().toRawUTF8(), b.lastNonZero));
    }

    //==========================================================================
    // HR1–HR3 — 96 / 192 k smoke (P93; R101, R102)
    //==========================================================================
    /** The QB1 swell carrier on 220 Hz (no 10 kHz bursts: R101 decimator band at D = 16). */
    inline Stim swell220 (double fs, double lenS)
    {
        auto x = std::make_shared<std::vector<float>> ((size_t) (lenS * fs));
        const double up = std::pow (10.0, 18.0 / 20.0);
        for (size_t i = 0; i < x->size(); ++i)
        {
            const double t = (double) i / fs, tc = t - 0.125 * std::floor (t / 0.125);
            double e = 1.0;
            if (tc < 0.001) e = 1.0 + (up - 1.0) * 0.5 * (1.0 - std::cos (eg::kPi * tc / 0.001));
            else if (tc < 0.021) e = up;
            else if (tc < 0.051) e = 1.0 + (up - 1.0) * 0.5 * (1.0 + std::cos (eg::kPi * (tc - 0.021) / 0.030));
            (*x)[i] = (float) (0.03 * e * std::sin (2.0 * eg::kPi * 220.0 * t));
        }
        return eg25x::fromBuffer (x);
    }

    void highRateRows()
    {
        std::printf ("\n-- HR. 96 / 192 k smoke: analysis load-independence, coefficient clamps, ring bytes (P93)\n");
        // HR1 — the hop log under the full C7 config == the same input with every stream disabled.
        {
            Tally t;
            juce::String det;
            for (double fs : { 96000.0, 192000.0 })
            {
                auto mk = [&] (bool streams)
                {
                    Job j;
                    j.fs = fs; j.len = (juce::int64) (6.0 * fs); j.block = 512; j.prepBlock = 512; j.seed = 0x4A11ull;
                    j.params = c7Params() + P { { "onset_threshold", -50.0f } } + (streams ? P {} : perStream ("enable", 0.0f));
                    j.stim = swell220 (fs, 6.0);
                    return j;
                };
                Job a = mk (true), b = mk (false);
                auto ha = eg25x::hopsInto (a), hb = eg25x::hopsInto (b);
                auto o = eg25::renderAll ({ a, b });
                bool same = ha->h.size() == hb->h.size() && ! ha->overflow && ! hb->overflow;
                long long conf = 0;
                for (size_t k = 0; same && k < ha->h.size(); ++k)
                {
                    const auto& x = ha->h[k];
                    const auto& y = hb->h[k];
                    same = x.aHop == y.aHop && x.commitAt == y.commitAt && std::memcmp (&x.rawNote, &y.rawNote, 4) == 0 && std::memcmp (&x.clarity, &y.clarity, 8) == 0
                           && x.confident == y.confident && std::memcmp (&x.decidedNote, &y.decidedNote, 4) == 0 && x.ringResult == y.ringResult;
                    conf += x.confident ? 1 : 0;
                }
                const double cf = ha->h.empty() ? 0.0 : (double) conf / (double) ha->h.size();
                const auto ev = o[0].eng().diag.onsetEvents;
                t.check (same && cf >= 0.90 && ev > 0 && o[0].eng().diag.spawns > 1000,
                         juce::String::formatted ("%g k: hops %d vs %d %s, confident %.1f %%, events %lld", fs / 1000.0, (int) ha->h.size(), (int) hb->h.size(),
                                                  same ? "bitwise" : "DIFFER", 100.0 * cf, (long long) ev));
                det << juce::String::formatted ("%g k: %d hops bitwise, confident %.1f %%, events %lld, spawns %lld; ", fs / 1000.0, (int) ha->h.size(), 100.0 * cf,
                                                (long long) ev, (long long) o[0].eng().diag.spawns);
            }
            row (t.bad == 0, "HR1 96 / 192 k: hop log under the full C7 load == streams disabled (hop, commit, raw / decided note, clarity, confident); confident >= 90 %",
                 t.bad ? t.detail (2) : det.trim());
        }
        // HR2 — fcHz / ringHz <= 0.45 fs at 4 rates; the 44.1 k bind; fb LP coefficients.
        {
            const double rates[4] { 44100.0, 48000.0, 96000.0, 192000.0 };
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg22::RecLog>> logs;
            for (double fs : rates)
            {
                Job j;
                j.fs = fs; j.len = (juce::int64) (4.0 * fs); j.block = 512; j.prepBlock = 512; j.seed = 0x4A12ull;
                j.params = eg22::base ({ 1, 2, 3, 4 }) + perStream ("density", 100.0f) + perStream ("size", 100.0f) + perStream ("filter_type", 1.0f)
                         + perStream ("filter_cutoff", 20000.0f) + perStream ("filter_rand", 100.0f) + perStream ("ring_freq", 5000.0f) + perStream ("ring_rand", 100.0f)
                         + perStream ("ring_mix", 100.0f) + P { { "fb_high_cut", 20000.0f }, { "feedback", 50.0f } };
                j.stim = c7Stim (fs);
                auto L = std::make_shared<eg22::RecLog>();
                j.after = eg22::collect (*L);
                logs.push_back (L);
                jobs.push_back (j);
            }
            auto o = eg25::renderAll (jobs);
            Tally t;
            juce::String det;
            for (size_t k = 0; k < 4; ++k)
            {
                const double fs = rates[k];
                const float lim = (float) (0.45 * fs);
                float fcMax = 0.0f, ringMax = 0.0f;
                int over = 0, n = 0;
                for (const auto& r : logs[k]->r)
                {
                    fcMax = juce::jmax (fcMax, r.fcHz); ringMax = juce::jmax (ringMax, r.ringHz);
                    over += (r.fcHz > lim || r.ringHz > lim) ? 1 : 0;
                    ++n;
                }
                const auto& ch = o[k].eng().getChainForTest();
                // The chain's cutoff comes through exp2 (log2 ramp) in float (a few ulps off 20 000):
                // recover the coefficient's effective cutoff fc = atan (a2 / a1) · fs / π (TPT: g = a2 / a1)
                // and require it at min (20 k, 0.45 fs) within 1e-4 relative — the clamp, not the ulps.
                const double wantFc = std::min (20000.0, 0.45 * fs);
                const double gotFc = std::atan ((double) ch.lp.a2 / (double) ch.lp.a1) * fs / eg::kPi;
                const bool lpOk = std::abs (gotFc - wantFc) <= 1e-4 * wantFc;
                const bool bind = k != 0 || juce::exactlyEqual (fcMax, lim);
                t.check (over == 0 && n > 100 && lpOk && bind && ! logs[k]->overflow,
                         juce::String::formatted ("%g k: %d over of %d, fc max %.1f (lim %.1f), ring max %.1f, fb LP fc %.3f (want %.1f)", fs / 1000.0, over, n, fcMax, lim, ringMax, gotFc, wantFc));
                det << juce::String::formatted ("%g k: fc max %.1f / ring max %.1f (lim %.1f) over %d of %d, fb LP fc %.3f; ", fs / 1000.0, fcMax, ringMax, lim, over, n, gotFc);
            }
            row (t.bad == 0, "HR2 every spawn's fcHz and ringHz <= 0.45 fs at 44.1 / 48 / 96 / 192 k; 44.1 k fc max == 19845 (binds); fb LP effective fc == min (20 k, 0.45 fs)",
                 t.bad ? t.detail (4) : det.trim());
        }
        // HR3 — ring bytes.
        {
            const double rates[4] { 44100.0, 48000.0, 96000.0, 192000.0 };
            const long long want[4] { 25315320LL, 27553920LL, 55105920LL, 110209920LL };
            Tally t;
            juce::String det;
            for (int k = 0; k < 4; ++k)
            {
                auto p = make();
                const auto t0 = std::chrono::steady_clock::now();
                p->prepareToPlay (rates[k], 512);
                const double ms1 = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
                const auto t1 = std::chrono::steady_clock::now();
                p->prepareToPlay (rates[k], 512);
                const double ms2 = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t1).count();
                const auto& ring = p->getEngineForTest().getRingForTest();
                long long bytes = 0;
                for (int l = 0; l <= ogran::kMaxLevel; ++l) bytes += 8LL * (ring.getCapacity (l) + 16);
                const bool rs = ogran::kRingSamples (rates[k]) == (int) std::ceil (41.0 * rates[k]);
                t.check (bytes == want[k] && rs, juce::String::formatted ("%g k: %lld (want %lld), kRingSamples %d", rates[k] / 1000.0, bytes, want[k], ogran::kRingSamples (rates[k])));
                det << juce::String::formatted ("%g k: %lld bytes, prepare %.1f / %.1f ms; ", rates[k] / 1000.0, bytes, ms1, ms2);
            }
            row (t.bad == 0, "HR3 ring bytes 8 * sum ((ceil (41 fs) >> l) + 64 + 16) == 25 315 320 / 27 553 920 / 55 105 920 / 110 209 920; kRingSamples == ceil (41 fs)",
                 t.bad ? t.detail (4) : det.trim());
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
    if (sect ("tailRows")) eg26::tailRows();
    if (sect ("censusRows")) eg26::censusRows();
    if (sect ("hostInputRows")) eg26::hostInputRows();
    if (sect ("z7aRows")) eg26::z7aRows();
    if (sect ("z7mRows")) eg26::z7mRows();
    if (sect ("z7m9Rows")) eg26::z7m9Rows();
    if (sect ("z7fRows")) eg26::z7fRows();
    if (sect ("z7dRows")) eg26::z7dRows();
    if (sect ("highRateRows")) eg26::highRateRows();
}
