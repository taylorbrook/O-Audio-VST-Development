// RESEARCH-2.6 scratch probes (hooks build): host non-finite input, decay-to-zero
// and the subnormal census with FTZ on (processBlock) vs off (Engine::process),
// incl. the reverb tank via the explicit-instantiation member-access idiom (no
// module change).
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <chrono>
#include <mach/mach.h>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
#include <pthread.h>
#include <execinfo.h>
extern "C" { typedef void (malloc_logger_t) (uint32_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t); extern malloc_logger_t* malloc_logger; }
static volatile bool gArmed = false; static volatile int gAllocs = 0; static pthread_t gThread;
static void countAlloc (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
{
    if (gArmed && (type & 2u) != 0 && pthread_equal (pthread_self(), gThread))
    {
        gAllocs = gAllocs + 1;
        if (gAllocs <= 2) { void* f[24]; backtrace_symbols_fd (f, backtrace (f, 24), 2); }
    }
}
using P = std::vector<std::pair<juce::String, float>>;
static P operator+ (P a, const P& b) { for (auto& kv : b) a.push_back (kv); return a; }
static juce::String sp (int s, const char* f) { return "s" + juce::String (s) + "_" + f; }
static constexpr double kPi = 3.14159265358979323846;

// ── private member access via explicit instantiation (access checks are waived
//    for names in an explicit instantiation; [temp.spec]/6) ─────────────────────
template <typename Tag> struct TagBase { friend auto get (Tag); };
template <typename Tag, auto M> struct Robber { friend auto get (Tag) { return M; } };
struct RevTag   : TagBase<RevTag>   {};  template struct Robber<RevTag,   &ogran::Engine::reverb>;
struct TankTag  : TagBase<TankTag>  {};  template struct Robber<TankTag,  &ReverbUnit::tankDelays>;
struct TFiltTag : TagBase<TFiltTag> {};  template struct Robber<TFiltTag, &ReverbUnit::tankFilters>;
struct DiffTag  : TagBase<DiffTag>  {};  template struct Robber<DiffTag,  &ReverbUnit::diffusionDelays>;
struct DStTag   : TagBase<DStTag>   {};  template struct Robber<DStTag,   &ReverbUnit::diffusionState>;
struct PreLTag  : TagBase<PreLTag>  {};  template struct Robber<PreLTag,  &ReverbUnit::preDelayL>;
struct FbDTag   : TagBase<FbDTag>   {};  template struct Robber<FbDTag,   &ogran::Engine::fbDelay>;
struct BufTag   : TagBase<BufTag>   {};  template struct Robber<BufTag,   &ReverbUnit::DelayLine::buffer>;
struct ZTag     : TagBase<ZTag>     {};  template struct Robber<ZTag,     &ReverbUnit::OnePole::z>;

static void setAll (juce::AudioProcessor& p, const P& ps)
{
    for (auto& kv : ps)
        for (auto* ap : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (ap))
                if (r->getParameterID() == kv.first) { r->setValueNotifyingHost (r->convertTo0to1 (kv.second)); break; }
}

static bool sub (float x)  { return std::fpclassify (x) == FP_SUBNORMAL; }
static bool subd (double x) { return std::fpclassify (x) == FP_SUBNORMAL; }

struct Census { int grains = 0, chain = 0, diff = 0, ring = 0, reverb = 0, analysis = 0, fbDelay = 0, nonFinRing = 0, nonFinRev = 0; };

static Census census (ogran::Engine& e, double fs, double ringBackS)
{
    Census c;
    for (int k = 0; k < ogran::kPoolLive + ogran::kPoolFade; ++k)
    {
        const auto& g = e.getPool().getGrains()[(size_t) k];
        if (sub (g.svfIc1[0]) || sub (g.svfIc1[1]) || sub (g.svfIc2[0]) || sub (g.svfIc2[1]) || sub (g.crushHeld[0]) || sub (g.crushHeld[1])
            || sub (g.ringRe) || sub (g.ringIm) || sub (g.killGain) || subd (g.readAbs)) ++c.grains;
    }
    const auto& ch = e.getChainForTest();
    for (int k = 0; k < 2; ++k) c.chain += sub (ch.hpIc1[k]) + sub (ch.hpIc2[k]) + sub (ch.lpIc1[k]) + sub (ch.lpIc2[k]);
    c.chain += sub (ch.env);
    for (int k = 0; k < 2; ++k) for (int s = 0; s < 4; ++s) for (int i = 0; i < ogran::kDiffuserMaxLen; ++i) c.diff += sub (ch.line[k][s][i]);
    const auto& ring = e.getRingForTest();
    for (int l = 0; l <= ogran::kMaxLevel; ++l)
    {
        const std::int64_t n = ring.getLevelCount (l);
        const std::int64_t back = std::min<std::int64_t> (n, (std::int64_t) (ringBackS * fs) >> l);
        for (std::int64_t j = n - back; j < n; ++j)
            for (int k = 0; k < 2; ++k) { const float x = ring.levelSample (l, k, j); c.ring += sub (x); c.nonFinRing += ! std::isfinite (x); }
    }
    auto& rev = e.*get (RevTag {});
    for (auto& d : rev.*get (TankTag {})) for (float x : d.*get (BufTag {})) { c.reverb += sub (x); c.nonFinRev += ! std::isfinite (x); }
    for (auto& st : rev.*get (DiffTag {})) for (auto& d : st) for (float x : d.*get (BufTag {})) { c.reverb += sub (x); c.nonFinRev += ! std::isfinite (x); }
    for (auto& st : rev.*get (DStTag {})) for (float x : st) c.reverb += sub (x);
    for (auto& f : rev.*get (TFiltTag {})) { const float z = f.*get (ZTag {}); c.reverb += sub (z); c.nonFinRev += ! std::isfinite (z); }
    for (float x : (rev.*get (PreLTag {})).*get (BufTag {})) c.reverb += sub (x);
    const auto& fbd = e.*get (FbDTag {});
    for (auto& row : fbd) for (float x : row) c.fbDelay += sub (x);
    const auto& a = e.getAnalysis();
    c.analysis += sub (a.getDetector().hpState()) + sub (a.getDetector().fastEnv()) + sub (a.getDetector().slowEnv());
    for (float x : a.getDecimator().history()) c.analysis += sub (x);
    for (float x : a.getWindow()) c.analysis += sub (x);
    return c;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0;
    const int block = 128;
    const bool decayOnly = argc > 1 && std::strcmp (argv[1], "--decay") == 0;
    if (argc > 1 && std::strcmp (argv[1], "--alloc") == 0)
    {
        P cfg { { "mix", 100.0f }, { "grain_limit", 0.0f }, { "feedback", 100.0f }, { "fb_diffusion", 100.0f }, { "reverb_mix", 100.0f },
                { "onset_threshold", -45.0f }, { "freeze", 1.0f } };
        for (int s = 1; s <= 4; ++s)
            cfg = cfg + P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 200.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "scale"), 2.0f },
                            { sp (s, "pitch_rand"), 12.0f }, { sp (s, "filter_type"), 1.0f }, { sp (s, "drive"), 50.0f }, { sp (s, "crush"), 10.0f },
                            { sp (s, "ring_mix"), 50.0f }, { sp (s, "onset_trigger"), 1.0f }, { "pitch_follow_s" + juce::String (s), 1.0f } };
        for (int m = 1; m <= 16; ++m)
            cfg = cfg + P { { "mod" + juce::String (m) + "_source", (float) (1 + (m - 1) % 11) }, { "mod" + juce::String (m) + "_dest", (float) (1 + 7 * m % 112) },
                            { "mod" + juce::String (m) + "_depth", 50.0f } };
        for (int variant = 0; variant < 2; ++variant)
        {
            auto p = std::unique_ptr<juce::AudioProcessor> (createPluginFilter());
            setAll (*p, cfg);
            p->prepareToPlay (fs, 512);
            juce::AudioBuffer<float> buf (2, 8192);
            juce::MidiBuffer midi;
            gThread = pthread_self();
            malloc_logger = countAlloc;
            gAllocs = 0;
            gArmed = true; void* volatile probe = std::malloc (64); gArmed = false; std::free (probe);
            const int live = gAllocs; gAllocs = 0;
            int firstBlock = -1;
            for (int k = 0; k < 400; ++k)
            {
                const int n = variant == 0 ? 512 : (k == 0 ? 8192 : 1 + (k * 7919) % 2048);
                buf.setSize (2, n, false, false, true);
                for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) buf.setSample (c, i, (float) (0.2 * std::sin (0.03 * (k * 512 + i))));
                gArmed = true;
                p->processBlock (buf, midi);
                gArmed = false;
                if (k == 0) firstBlock = gAllocs;
                if (k == 100) setAll (*p, P { { "freeze", 0.0f }, { "sync_mode", 1.0f }, { "mod3_source", 0.0f } });
            }
            malloc_logger = nullptr;
            std::printf ("alloc probe (%s, first block ARMED): liveness %d, first block %d allocs, total %d allocs over 400 blocks\n",
                         variant == 0 ? "512 blocks" : "first block 8192 after prepare 512, then variable", live, firstBlock, (int) gAllocs);
        }
        return 0;
    }
    if (argc > 2 && std::strcmp (argv[1], "--mem") == 0)
    {
        const double r = std::atof (argv[2]);
        auto rss = [] { mach_task_basic_info info; mach_msg_type_number_t n = MACH_TASK_BASIC_INFO_COUNT;
                        task_info (mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info, &n); return (double) info.resident_size / 1e6; };
        auto p = std::unique_ptr<juce::AudioProcessor> (createPluginFilter());
        const double r0 = rss();
        const auto t0 = std::chrono::steady_clock::now();
        p->prepareToPlay (r, 512);
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
        const double r1 = rss();
        const auto t1 = std::chrono::steady_clock::now();
        p->prepareToPlay (r, 512);
        const double ms2 = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t1).count();
        auto& e = dynamic_cast<GranulatorProcessor&> (*p).getEngineForTest();
        const auto& ring = e.getRingForTest();
        double bytes = 0;
        for (int l = 0; l <= ogran::kMaxLevel; ++l) bytes += 2.0 * 4.0 * (ring.getCapacity (l) + 16);
        std::printf ("fs %g: kRingSamples %d, ring bytes (levels 0-2, 2 ch, +16 guard) %.0f = %.2f MB; prepare #1 %.1f ms, RSS +%.1f MB; prepare #2 (same rate) %.1f ms\n",
                     r, ogran::kRingSamples (r), bytes, bytes / 1e6, ms, r1 - r0, ms2);
        return 0;
    }

    // Census self-test (the census must SEE a planted subnormal).
    {
        volatile float planted = 1.0e-40f;
        std::printf ("census self-test: fpclassify(1e-40f) subnormal = %d\n", (int) sub (planted));
    }

    // ── A/B: host non-finite input ─────────────────────────────────────────
    if (! decayOnly)
    for (int kind = 0; kind < 2; ++kind)
    {
        auto p = std::unique_ptr<juce::AudioProcessor> (createPluginFilter());
        setAll (*p, P { { "mix", 50.0f }, { "feedback", 50.0f }, { "reverb_mix", 30.0f }, { "s1_density", 50.0f }, { "s1_size", 200.0f },
                        { "s1_spray", 50.0f }, { "s1_position", 5.0f } });
        p->prepareToPlay (fs, block);
        auto& e = dynamic_cast<GranulatorProcessor&> (*p).getEngineForTest();
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        const juce::int64 at = (juce::int64) fs + 5, len = (juce::int64) (12.0 * fs);
        juce::int64 nonFinite = 0, lastBad = -1, firstBadAfter = -1;
        for (juce::int64 pos = 0; pos < len; pos += block)
        {
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < block; ++i)
                {
                    const juce::int64 n = pos + i;
                    float v = (float) (0.2 * std::sin (2.0 * kPi * 220.0 * (double) n / fs));
                    if (n == at && c == 0) v = kind == 0 ? std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity();
                    buf.setSample (c, i, v);
                }
            p->processBlock (buf, midi);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < block; ++i)
                    if (! std::isfinite (buf.getSample (c, i)))
                    {
                        ++nonFinite;
                        lastBad = pos + i;
                        if (pos + i > at && firstBadAfter < 0) firstBadAfter = pos + i;
                    }
        }
        const auto cs = census (e, fs, 41.0);
        std::printf ("host %s at %.3f s: non-finite output samples %lld (first after the input sample @ %.3f s, last @ %.3f s); ring non-finite %d, reverb non-finite %d; fbScrubs %lld nanKills %lld\n",
                     kind == 0 ? "NaN" : "+Inf", (double) at / fs, (long long) nonFinite, firstBadAfter / fs, lastBad / fs, cs.nonFinRing, cs.nonFinRev,
                     (long long) e.diag.fbScrubs, (long long) e.diag.nanKills);
    }

    // ── C: decay to zero after the input stops; FTZ on vs off ──────────────
    P maxAll { { "mix", 100.0f }, { "grain_limit", 3.0f }, { "buffer_length", 30.0f }, { "feedback", 100.0f }, { "fb_diffusion", 100.0f },
               { "fb_saturation", 100.0f }, { "reverb_mix", 100.0f }, { "reverb_size", 100.0f }, { "reverb_damp", 0.0f } };
    for (int s = 1; s <= 4; ++s)
        maxAll = maxAll + P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 200.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "spray"), 100.0f },
                              { sp (s, "position"), 0.0f }, { sp (s, "pitch"), s % 2 ? 12.0f : -12.0f }, { sp (s, "filter_type"), 1.0f },
                              { sp (s, "filter_res"), 100.0f }, { sp (s, "filter_cutoff"), 2000.0f }, { sp (s, "level"), 6.0f } };
    const double stimS = 10.0, totalS = decayOnly ? 400.0 : 130.0;
    for (int ftz = 1; ftz >= 0; --ftz)
    {
        auto p = std::unique_ptr<juce::AudioProcessor> (createPluginFilter());
        setAll (*p, maxAll);
        p->prepareToPlay (fs, block);
        auto& gpr = dynamic_cast<GranulatorProcessor&> (*p);
        auto& e = gpr.getEngineForTest();
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        double lastNonZero = -1.0;
        double firstSub[7]; for (double& f : firstSub) f = -1.0;
        int maxSub[7] {};
        const char* names[7] { "grains", "fb chain", "diffuser", "ring", "reverb", "analysis", "fbDelay" };
        double peakAfter = 0.0, seg = 0.0;
        for (juce::int64 pos = 0; pos < (juce::int64) (totalS * fs); pos += block)
        {
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < block; ++i)
                {
                    const juce::int64 n = pos + i;
                    buf.setSample (c, i, n < (juce::int64) (stimS * fs) ? (float) (0.5 * std::sin (2.0 * kPi * 220.0 * (double) n / fs)
                                                                                + 0.1 * std::sin (2.0 * kPi * 3311.0 * (double) n / fs)) : 0.0f);
                }
            if (ftz == 1)
                p->processBlock (buf, midi);
            else
            {
                const float* in[2] { buf.getReadPointer (0), buf.getReadPointer (1) };
                float* out[2] { buf.getWritePointer (0), buf.getWritePointer (1) };
                e.process (in, 2, out, 2, block, gpr.getSeed(), {});
            }
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < block; ++i)
                {
                    const float y = buf.getSample (c, i);
                    if (y != 0.0f) lastNonZero = (double) (pos + i) / fs;
                    if (pos + i >= (juce::int64) (stimS * fs)) peakAfter = std::max (peakAfter, (double) std::abs (y));
                    seg = std::max (seg, (double) std::abs (y));
                }
            if ((pos / block) % (juce::int64) (20.0 * fs / block) == 0 && pos > 0)
            {
                std::printf ("   FTZ %d t %4.0f s: peak |out| over the last 20 s %.3g (%.1f dBFS)\n", ftz, (double) pos / fs, seg, 20.0 * std::log10 (std::max (seg, 1e-300)));
                seg = 0.0;
            }
            if ((pos / block) % (juce::int64) (fs / block) == 0 && pos >= (juce::int64) (stimS * fs))
            {
                const auto cs = census (e, fs, 1.0);
                const int v[7] { cs.grains, cs.chain, cs.diff, cs.ring, cs.reverb, cs.analysis, cs.fbDelay };
                for (int k = 0; k < 7; ++k)
                {
                    if (v[k] > 0 && firstSub[k] < 0) firstSub[k] = (double) pos / fs;
                    maxSub[k] = std::max (maxSub[k], v[k]);
                }
            }
        }
        std::printf ("decay FTZ %s: stim %g s then silence to %g s; last non-zero output @ %.2f s; ", ftz ? "ON (processBlock)" : "OFF (Engine::process)", stimS, totalS, lastNonZero);
        for (int k = 0; k < 7; ++k)
            std::printf ("%s first sub @ %.0f s (max %d); ", names[k], firstSub[k], maxSub[k]);
        std::printf ("\n");
        std::fflush (stdout);
    }
    return 0;
}
