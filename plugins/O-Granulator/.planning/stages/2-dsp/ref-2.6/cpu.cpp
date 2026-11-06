// RESEARCH-2.6 scratch: CPU argmax sweep, prepare time / memory at 3 rates.
// Built twice: OGRAN_TEST_HOOKS=1 (liveness counters) and =0 (the plugin's code).
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <vector>
#include <algorithm>
#include <mach/mach.h>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
using P = std::vector<std::pair<juce::String, float>>;
static P operator+ (P a, const P& b) { for (auto& kv : b) a.push_back (kv); return a; }
static juce::String sp (int s, const char* f) { return "s" + juce::String (s) + "_" + f; }
static constexpr double kPi = 3.14159265358979323846;

static void setAll (juce::AudioProcessor& p, const P& ps)
{
    for (auto& kv : ps)
    {
        juce::RangedAudioParameter* rp = nullptr;
        for (auto* ap : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (ap))
                if (r->getParameterID() == kv.first) { rp = r; break; }
        if (rp == nullptr) { std::printf ("!! unknown %s\n", kv.first.toRawUTF8()); continue; }
        rp->setValueNotifyingHost (rp->convertTo0to1 (kv.second));
    }
}

static double threadCpuS()
{
    timespec ts; clock_gettime (CLOCK_THREAD_CPUTIME_ID, &ts);
    return (double) ts.tv_sec + 1e-9 * (double) ts.tv_nsec;
}
static double rssMB()
{
    mach_task_basic_info info; mach_msg_type_number_t n = MACH_TASK_BASIC_INFO_COUNT;
    task_info (mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info, &n);
    return (double) info.resident_size / 1048576.0;
}

// Stimulus: 220 Hz harmonic tone (−14 dBFS-ish) with +12 dB 5 ms swells every
// `ioi` s (same pitch: keeps the MPM periodic, fires the detector near its 50 ms
// refractory), optional noise floor.
static std::vector<float> makeStim (double fs, double lenS, double ioi, double noise)
{
    // 220 Hz harmonic tone (MPM-confident) + 3 ms 10 kHz bursts every `ioi` s:
    // the bursts drive the onset detector near its 50 ms refractory and sit
    // above the decimator band (0.4 fs/D = 4.8 kHz at 48 k), so the MPM sees
    // the tone only.
    std::vector<float> x ((size_t) (lenS * fs));
    juce::Random r (7);
    for (size_t i = 0; i < x.size(); ++i)
    {
        const double t = (double) i / fs, tc = std::fmod (t, ioi);
        const double w = 2.0 * kPi * 220.0 * t;
        double v = 0.2 * std::sin (w);
        if (tc < 0.003) v += 0.8 * std::sin (kPi * tc / 0.003) * std::sin (2.0 * kPi * 10000.0 * t);
        x[i] = (float) (v + noise * (2.0 * r.nextDouble() - 1.0));
    }
    return x;
}

struct Res { double p50us = 0, p99us = 0, maxus = 0; double medWall, minWall, maxWall, medThr, medWallTotal; double prepMs; double rssDelta;
             long long spawns, steals, events, hops, confident, hits, bursts; int alive, maxAlive; };

static Res timeConfig (const P& ps, const std::vector<float>& stim, double fs, int block, int runs, double runS, double warmS)
{
    Res res {};
    const double rss0 = rssMB();
    auto p = std::unique_ptr<juce::AudioProcessor> (createPluginFilter());
    setAll (*p, ps);
    auto* gp = dynamic_cast<GranulatorProcessor*> (p.get());
    gp->setSeed (0xC2C2ull);
    const auto tp0 = std::chrono::steady_clock::now();
    p->prepareToPlay (fs, block);
    res.prepMs = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - tp0).count();
    res.rssDelta = rssMB() - rss0;
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    juce::int64 pos = 0;
    const juce::int64 N = (juce::int64) stim.size();
    std::vector<double>* blockUs = nullptr;
    std::vector<double> lastUs;
    auto runFor = [&] (double secs, double& wallSum, double& wallTotal, double& thr)
    {
        const juce::int64 end = pos + (juce::int64) (secs * fs);
        wallSum = 0.0;
        const double c0 = threadCpuS();
        const auto t0 = std::chrono::steady_clock::now();
        for (; pos < end; pos += block)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                float* d = buf.getWritePointer (ch);
                for (int i = 0; i < block; ++i) d[i] = stim[(size_t) ((pos + i) % N)];
            }
            const auto b0 = std::chrono::steady_clock::now();
            p->processBlock (buf, midi);
            const double us = std::chrono::duration<double, std::micro> (std::chrono::steady_clock::now() - b0).count();
            wallSum += us * 1e-6;
            if (blockUs != nullptr) blockUs->push_back (us);
        }
        wallTotal = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        thr = threadCpuS() - c0;
    };
    double a, b, c;
    runFor (warmS, a, b, c);
   #if OGRAN_TEST_HOOKS
    auto& e = gp->getEngineForTest();
    const auto d0 = e.diag;
   #endif
    std::vector<double> w, wt, th;
    for (int r = 0; r < runs; ++r)
    {
        lastUs.clear(); blockUs = (r == runs - 1) ? &lastUs : nullptr;
        runFor (runS, a, b, c);
        w.push_back (a / runS * 100.0); wt.push_back (b / runS * 100.0); th.push_back (c / runS * 100.0);
    }
    auto med = [] (std::vector<double> v) { std::sort (v.begin(), v.end()); return v[v.size() / 2]; };
    res.medWall = med (w); res.medWallTotal = med (wt); res.medThr = med (th);
    if (! lastUs.empty()) { std::sort (lastUs.begin(), lastUs.end()); res.p50us = lastUs[lastUs.size() / 2]; res.p99us = lastUs[(size_t) (0.99 * (double) lastUs.size())]; res.maxus = lastUs.back(); }
    res.minWall = *std::min_element (w.begin(), w.end()); res.maxWall = *std::max_element (w.begin(), w.end());
   #if OGRAN_TEST_HOOKS
    const auto& d = e.diag;
    const double T = runs * runS;
    res.spawns = (long long) ((d.spawns - d0.spawns) / T); res.steals = (long long) ((d.steals - d0.steals) / T);
    res.events = (long long) ((d.onsetEvents - d0.onsetEvents) / T); res.hops = d.mpmHops - d0.mpmHops;
    res.confident = d.mpmConfident - d0.mpmConfident; res.hits = (long long) ((d.followHits - d0.followHits) / T);
    res.bursts = (long long) ((d.freeBursts + d.tempoBursts - d0.freeBursts - d0.tempoBursts) / T);
    for (int s = 0; s < ogran::kPoolLive + ogran::kPoolFade; ++s) res.alive += e.getPool().isActive (s) ? 1 : 0;
    res.maxAlive = (int) d.maxAlive;
   #endif
    return res;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const bool quick = argc > 1 && std::strcmp (argv[1], "--quick") == 0;
    const bool rates = argc > 1 && std::strcmp (argv[1], "--rates") == 0;
    const bool spread = argc > 1 && std::strcmp (argv[1], "--spread") == 0;
    const double fs = 48000.0;
    const auto stim = makeStim (fs, 10.0, 0.052, 0.0);

    // ── D22 literal: everything at its max ──────────────────────────────────
    P maxAll { { "mix", 100.0f }, { "grain_limit", 3.0f }, { "buffer_length", 30.0f }, { "feedback", 100.0f }, { "fb_diffusion", 100.0f },
               { "fb_saturation", 100.0f }, { "fb_low_cut", 2000.0f }, { "fb_high_cut", 20000.0f },
               { "reverb_mix", 100.0f }, { "reverb_size", 100.0f }, { "reverb_damp", 100.0f }, { "onset_threshold", -45.0f },
               { "input_gain", 0.0f } };
    for (int s = 1; s <= 4; ++s)
        maxAll = maxAll + P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 200.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "spray"), 100.0f },
                              { sp (s, "position"), 100.0f }, { sp (s, "size_rand"), 100.0f }, { sp (s, "jitter"), 100.0f }, { sp (s, "pitch"), 24.0f },
                              { sp (s, "pitch_rand"), 24.0f }, { sp (s, "reverse_prob"), 100.0f }, { sp (s, "window"), 1.0f }, { sp (s, "window_skew"), 100.0f },
                              { sp (s, "stereo_spread"), 100.0f }, { sp (s, "gain_rand"), 100.0f }, { sp (s, "filter_type"), 1.0f },
                              { sp (s, "filter_cutoff"), 20000.0f }, { sp (s, "filter_rand"), 100.0f }, { sp (s, "filter_res"), 100.0f },
                              { sp (s, "drive"), 100.0f }, { sp (s, "crush"), 100.0f }, { sp (s, "crush_prob"), 100.0f },
                              { sp (s, "ring_freq"), 5000.0f }, { sp (s, "ring_rand"), 100.0f }, { sp (s, "ring_mix"), 100.0f },
                              { sp (s, "onset_trigger"), 1.0f }, { "pitch_follow_s" + juce::String (s), 1.0f }, { sp (s, "scale"), 2.0f },
                              { sp (s, "level"), 6.0f } };
    for (int n = 1; n <= 4; ++n)
        maxAll = maxAll + P { { "lfo" + juce::String (n) + "_rate", 20.0f }, { "lfo" + juce::String (n) + "_shape", (float) (n + 1) },
                              { "macro" + juce::String (n), 100.0f } };
    // 16 routes, every source class (LFO1-4, Env, Pitch, Onset, Macro1-4), full depth,
    // mixed classes: spawn-latched stream fields, ramped, tick dests.
    const int dests[16] { 5 + 0 * 24 + 4, 5 + 1 * 24 + 8, 5 + 2 * 24 + 15, 5 + 3 * 24 + 17, 2, 101, 104, 107,
                          5 + 0 * 24 + 0, 5 + 1 * 24 + 1, 5 + 2 * 24 + 6, 5 + 3 * 24 + 2, 1, 3, 110, 5 + 0 * 24 + 23 };
    const int srcs[16] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 1, 2, 5, 6, 7 };
    for (int m = 0; m < 16; ++m)
        maxAll = maxAll + P { { "mod" + juce::String (m + 1) + "_source", (float) srcs[m] }, { "mod" + juce::String (m + 1) + "_dest", (float) dests[m] },
                              { "mod" + juce::String (m + 1) + "_depth", m % 2 ? -100.0f : 100.0f } };

    // C6-like reference (2.5b harness C6 config, input differs).
    P c6 { { "mix", 100.0f }, { "grain_limit", 3.0f }, { "feedback", 100.0f }, { "fb_diffusion", 100.0f }, { "reverb_mix", 50.0f } };
    for (int s = 1; s <= 4; ++s)
        c6 = c6 + P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 16.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "window"), 0.6f },
                      { sp (s, "scale"), 1.0f }, { sp (s, "pitch_rand"), 2.0f }, { sp (s, "filter_type"), 1.0f }, { sp (s, "filter_res"), 100.0f },
                      { sp (s, "filter_rand"), 100.0f }, { sp (s, "drive"), 100.0f }, { sp (s, "crush"), 100.0f }, { sp (s, "crush_prob"), 100.0f },
                      { sp (s, "ring_mix"), 100.0f }, { sp (s, "ring_rand"), 100.0f }, { sp (s, "onset_trigger"), 1.0f },
                      { "pitch_follow_s" + juce::String (s), 1.0f } };

    auto perStream = [] (const char* f, float v) { P r; for (int s = 1; s <= 4; ++s) r.push_back ({ sp (s, f), v }); return r; };
    P noFx = perStream ("filter_type", 0.0f) + perStream ("drive", 0.0f) + perStream ("crush", 0.0f) + perStream ("ring_mix", 0.0f);

    std::vector<std::pair<const char*, P>> cfgs;
    if (spread)
    {
        cfgs = { { "MAX (D22 literal)", maxAll }, { "MAX crush 1%", maxAll + perStream ("crush", 1.0f) },
                 { "MAX crush 1% route13->s1_level", [&] { P r = maxAll + perStream ("crush", 1.0f); r.push_back ({ "mod13_dest", 5.0f }); return r; }() } };
    }
    else if (! rates)
    {
        cfgs = {
            { "bypass (streams off)", P { { "mix", 100.0f } } + perStream ("enable", 0.0f) },
            { "C6-like", c6 },
            { "MAX (D22 literal)", maxAll },
            { "MAX crush 1%", maxAll + perStream ("crush", 1.0f) },
            { "MAX crush 1% window 0 skew 0", maxAll + perStream ("crush", 1.0f) + perStream ("window", 0.0f) + perStream ("window_skew", 0.0f) },
            { "MAX window 0 skew 0", maxAll + perStream ("window", 0.0f) + perStream ("window_skew", 0.0f) },
            { "MAX density 16 (fewer steals)", maxAll + perStream ("density", 16.0f) },
            { "MAX size 5 (spawn-bound)", maxAll + perStream ("size", 5.0f) },
            { "MAX pitch 0 rand 0", maxAll + perStream ("pitch", 0.0f) + perStream ("pitch_rand", 0.0f) },
            { "MAX reverse 0", maxAll + perStream ("reverse_prob", 0.0f) },
            { "MAX scale Off", maxAll + perStream ("scale", 0.0f) },
            { "MAX no FX", maxAll + noFx },
            { "MAX FX svf only", maxAll + noFx + perStream ("filter_type", 1.0f) },
            { "MAX FX drive only", maxAll + noFx + perStream ("drive", 100.0f) },
            { "MAX FX crush 100 only", maxAll + noFx + perStream ("crush", 100.0f) },
            { "MAX FX crush 1 only", maxAll + noFx + perStream ("crush", 1.0f) },
            { "MAX FX ring only", maxAll + noFx + perStream ("ring_mix", 100.0f) },
            { "MAX fb 0 reverb 0", maxAll + P { { "feedback", 0.0f }, { "fb_diffusion", 0.0f }, { "fb_saturation", 0.0f }, { "reverb_mix", 0.0f } } },
            { "MAX no routes", [&] { P r = maxAll; for (int m = 1; m <= 16; ++m) r.push_back ({ "mod" + juce::String (m) + "_source", 0.0f }); return r; }() },
            { "MAX triggers off follow off", maxAll + perStream ("onset_trigger", 0.0f)
                  + P { { "pitch_follow_s1", 0.0f }, { "pitch_follow_s2", 0.0f }, { "pitch_follow_s3", 0.0f }, { "pitch_follow_s4", 0.0f } } },
            { "MAX Tempo 1/64T", maxAll + P { { "sync_mode", 1.0f } } + perStream ("density_sync", 0.0f) + perStream ("size_sync", 19.0f) },
        };
        if (quick) cfgs.resize (3);
    }
    else
    {
        cfgs = { { "MAX crush 1%", maxAll + perStream ("crush", 1.0f) } };
    }

    if (argc > 1 && std::strcmp (argv[1], "--blocks") == 0)
    {
        P cfg = maxAll + perStream ("crush", 1.0f);
        for (int b : { 32, 64, 128, 256, 512, 1024 })
        {
            const auto x = timeConfig (cfg, stim, fs, b, 5, 10.0, 5.0);
            std::printf ("MAX crush 1%% block %5d: wall %6.2f %% (min %.2f max %.2f, N=5 x 10 s) thread %6.2f %%  p99 %.0f us max %.0f us (budget %.0f us)\n",
                         b, x.medWall, x.minWall, x.maxWall, x.medThr, x.p99us, x.maxus, b / fs * 1e6);
            std::fflush (stdout);
        }
        return 0;
    }
    std::printf ("hooks=%d  fs=%g block=128\n", (int) OGRAN_TEST_HOOKS, fs);
    const int runs = spread ? 9 : 5;
    const double runS = spread ? 10.0 : 5.0;
    for (auto& c : cfgs)
    {
        if (rates)
        {
            for (double r : { 44100.0, 48000.0, 96000.0, 192000.0 })
            {
                const auto st = makeStim (r, 10.0, 0.052, 0.0);
                const auto x = timeConfig (c.second, st, r, 128, 3, 5.0, 4.5);
                std::printf ("%-36s fs %6.0f  wall %6.2f %% (min %.2f max %.2f) thread %6.2f %%  prepare %.1f ms  rss +%.1f MB  alive %d events/s %lld conf %lld/%lld steals/s %lld  p99 %.0f us max %.0f us (budget %.0f us)\n",
                             c.first, r, x.medWall, x.minWall, x.maxWall, x.medThr, x.prepMs, x.rssDelta, x.alive, x.events, x.confident, x.hops, x.steals, x.p99us, x.maxus, 128.0 / r * 1e6);
                std::fflush (stdout);
            }
            continue;
        }
        const auto x = timeConfig (c.second, stim, fs, 128, runs, runS, 4.5);
        std::printf ("%-36s blocks p50 %.1f us p99 %.1f us max %.1f us (budget %.1f us)\n", c.first, x.p50us, x.p99us, x.maxus, 128.0 / fs * 1e6);
        std::printf ("%-36s wall %6.2f %% (min %.2f max %.2f, N=%d x %.0f s) thread %6.2f %% loop %6.2f %% | alive %d maxAlive %d spawns/s %lld steals/s %lld events/s %lld conf %lld/%lld hits/s %lld bursts/s %lld\n",
                     c.first, x.medWall, x.minWall, x.maxWall, runs, runS, x.medThr, x.medWallTotal, x.alive, x.maxAlive, x.spawns, x.steals, x.events,
                     x.confident, x.hops, x.hits, x.bursts);
        std::fflush (stdout);
    }
    return 0;
}
