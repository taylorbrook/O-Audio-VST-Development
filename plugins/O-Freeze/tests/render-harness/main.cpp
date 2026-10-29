/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-Freeze render harness (v2.9.0, milestone spectral-diffusion-freeze).
    Console target O-Freeze-render-test (OUARICON_BUILD_TESTS=ON). Constructs the
    processor directly (no host, no editor) and runs:

    --fft-identity   juce::dsp::FFT order 13 real-only forward -> inverse round
                     trip; prints the scale factor the inverse applies (the
                     spectral OLA normalisation 1/1.5 assumes it is 1).
    --probe          scripted Spectral clock / capture checks: the analysis hop
                     counter runs only while the write head moves (stops during
                     the freeze and the WR-03 tail hold); rolling and fallback
                     captures both fire.
    --alloc          heap allocations inside processBlock with Spectral engaged:
                     rolling-path engage, fallback-path engage, Granular <->
                     Spectral switches while frozen, release. Ported from
                     plugins/O-Bells/tests/render-harness (libmalloc malloc_logger
                     hook, processBlock thread only), proven live first by a
                     deliberate malloc.
    --timing [--rate=48000] [--block=32] [--seconds=20]
                     worst processBlock time with Spectral frozen (steady blocks,
                     engage block reported separately) vs the block period.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#if JUCE_MAC
 #include <execinfo.h>
 #include <pthread.h>
#endif

#if JUCE_MAC
extern "C"
{
    typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                    uintptr_t result, uint32_t numHotFramesToSkip);
    extern malloc_logger_t* malloc_logger;
}
#endif

namespace
{
#if JUCE_MAC
// volatile: clang folds "armed = true; malloc(); armed = false" otherwise (O-Bells note)
volatile bool allocArmed = false;
volatile int allocCount = 0;
bool allocTrace = false;
pthread_t audioThread;

void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
{
    if (allocArmed && (type & 2u) != 0 && pthread_equal (pthread_self(), audioThread))   // MALLOC_LOG_TYPE_ALLOCATE
    {
        allocArmed = false;
        allocCount = allocCount + 1;
        if (allocTrace)
        {
            void* frames[32];
            backtrace_symbols_fd (frames, backtrace (frames, 32), 2);
        }
    }
}
#endif

int fail (const juce::String& m) { std::cout << "FAIL " << m << std::endl; return 1; }

void setParam (OFreezeAudioProcessor& p, const char* id, float plain)
{
    auto* par = p.getAPVTS().getParameter (id);
    par->setValueNotifyingHost (par->convertTo0to1 (plain));
}

struct Rig
{
    OFreezeAudioProcessor proc;
    juce::AudioBuffer<float> block;
    juce::MidiBuffer midi;
    std::mt19937 rng { 7 };
    std::normal_distribution<float> noise { 0.0f, 0.1f };
    double fs;
    int blockSize;
    long long t = 0;
    bool armAlloc = false;
    double lastBlockSeconds = 0.0;

    Rig (double rate, int bs) : fs (rate), blockSize (bs)
    {
        proc.rngSeed.store (0x1234567890abcdefull);
        proc.setPlayConfigDetails (2, 2, fs, blockSize);
        proc.prepareToPlay (fs, blockSize);
        block.setSize (2, blockSize);
    }

    // chord + noise, the gate stimulus family
    void fill()
    {
        for (int i = 0; i < blockSize; ++i, ++t)
        {
            const double tt = static_cast<double> (t) / fs;
            float c = 0.0f;
            for (double f : { 220.0, 277.18, 329.63, 466.16 })
                c += 0.12f * static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * f * tt));
            block.setSample (0, i, c + noise (rng) * 0.5f);
            block.setSample (1, i, c + noise (rng) * 0.5f);
        }
    }

    bool run (double seconds)
    {
        const long long n = static_cast<long long> (seconds * fs / blockSize);
        for (long long b = 0; b < n; ++b)
        {
            fill();
           #if JUCE_MAC
            allocArmed = armAlloc;
           #endif
            const auto t0 = std::chrono::steady_clock::now();
            proc.processBlock (block, midi);
            lastBlockSeconds = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
           #if JUCE_MAC
            allocArmed = false;
            if (allocCount > 0)
                return false;
           #endif
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                    if (! std::isfinite (block.getSample (ch, i)))
                        return false;
        }
        return true;
    }
};

int fftIdentity()
{
    juce::dsp::FFT fft (13);
    const int n = fft.getSize();
    std::vector<float> x (static_cast<size_t> (2 * n), 0.0f), ref (static_cast<size_t> (n));
    std::mt19937 rng (3);
    std::uniform_real_distribution<float> u (-1.0f, 1.0f);
    for (int i = 0; i < n; ++i)
        x[static_cast<size_t> (i)] = ref[static_cast<size_t> (i)] = u (rng);

    fft.performRealOnlyForwardTransform (x.data(), true);
    // the synthesis path writes only the first N/2 + 1 complex values; mimic it
    std::fill (x.begin() + 2 * (n / 2 + 1), x.end(), 0.0f);
    fft.performRealOnlyInverseTransform (x.data());

    double num = 0.0, den = 0.0;
    for (int i = 0; i < n; ++i)
    {
        num += static_cast<double> (x[static_cast<size_t> (i)]) * ref[static_cast<size_t> (i)];
        den += static_cast<double> (ref[static_cast<size_t> (i)]) * ref[static_cast<size_t> (i)];
    }
    const double scale = num / den;
    double err = 0.0;
    for (int i = 0; i < n; ++i)
        err = std::max (err, std::abs (static_cast<double> (x[static_cast<size_t> (i)]) - scale * ref[static_cast<size_t> (i)]));

    std::cout << "fft-identity: inverse(forward(x)) = " << scale << " * x, max residual " << err << std::endl;
    const bool ok = std::abs (scale - 1.0) < 1.0e-5 && err < 1.0e-4;
    std::cout << (ok ? "PASS" : "FAIL") << " fft-identity (scale 1, the 1/1.5 OLA normalisation holds)" << std::endl;
    return ok ? 0 : 1;
}

int probe()
{
    int fails = 0;
    auto check = [&fails] (bool ok, const juce::String& what)
    {
        std::cout << (ok ? "PASS " : "FAIL ") << what << std::endl;
        fails += ok ? 0 : 1;
    };

    Rig r (48000.0, 64);
    const auto& sp = r.proc.getSpectralForTests();
    setParam (r.proc, "ENGINE", 1.0f);
    setParam (r.proc, "AVG_FRAMES", 8.0f);

    // 1. rolling analysis on the write-head clock: 1 s = 48000 writes = 23 hops
    r.run (1.0);
    const int roll0 = sp.getRollingFrames();
    check (roll0 == 48000 / SpectralFreeze::kHop, "rolling hops after 1 s = " + juce::String (roll0) + " (expect 23)");

    // 2. engage with >= N rolling frames: rolling capture, no fallback
    setParam (r.proc, "FREEZE", 1.0f);
    r.run (1.0);
    check (sp.getCaptures() == 1 && sp.getFallbackCaptures() == 0,
           "engage after 1 s: captures " + juce::String (sp.getCaptures()) + ", fallback " + juce::String (sp.getFallbackCaptures()) + " (rolling capture)");
    check (sp.getRollingFrames() == roll0, "analysis hop counter frozen with the write head: " + juce::String (sp.getRollingFrames()));
    const auto synthHops = sp.getHopCounter();
    check (synthHops >= 4 + 23, "synth hops advanced while frozen: " + juce::String ((juce::int64) synthHops));

    // 3. release: write head held through the 100 ms tail (WR-03), then resumes
    setParam (r.proc, "FREEZE", 0.0f);
    r.run (0.064);
    check (sp.getRollingFrames() == roll0, "no analysis during the release tail hold: " + juce::String (sp.getRollingFrames()));
    r.run (1.0);
    check (sp.getRollingFrames() > roll0, "analysis resumes after the tail: " + juce::String (sp.getRollingFrames()));
    check (! sp.isRunning(), "spectral synthesis stopped after the tail");

    // 4. engine switched just before engaging (< N * H): fallback capture
    setParam (r.proc, "ENGINE", 0.0f);
    r.run (0.5);
    setParam (r.proc, "ENGINE", 1.0f);
    r.run (0.02);   // 960 samples < 8 * 2048
    setParam (r.proc, "FREEZE", 1.0f);
    r.run (0.2);
    check (sp.getCaptures() == 2 && sp.getFallbackCaptures() == 1,
           "engage 20 ms after an engine switch: fallback capture (captures " + juce::String (sp.getCaptures())
           + ", fallback " + juce::String (sp.getFallbackCaptures()) + ")");
    setParam (r.proc, "FREEZE", 0.0f);
    r.run (0.5);

    // 5. Granular freeze, then switch to Spectral mid-freeze: fallback from the held ring
    setParam (r.proc, "ENGINE", 0.0f);
    r.run (0.5);
    setParam (r.proc, "FREEZE", 1.0f);
    r.run (0.3);
    setParam (r.proc, "ENGINE", 1.0f);
    r.run (0.3);
    check (sp.getCaptures() == 3 && sp.getFallbackCaptures() == 2 && sp.isRunning(),
           "mid-freeze Granular -> Spectral: fallback capture, synthesis running");
    // and back: spectral stops once the 50 ms engine fade completes
    setParam (r.proc, "ENGINE", 0.0f);
    r.run (0.2);
    check (! sp.isRunning(), "mid-freeze Spectral -> Granular: spectral stops after the engine fade");
    setParam (r.proc, "FREEZE", 0.0f);
    r.run (1.5);

    std::cout << "PROBE " << (fails == 0 ? "PASS" : "FAIL") << std::endl;
    return fails == 0 ? 0 : 1;
}

int allocCheck()
{
   #if JUCE_MAC
    audioThread = pthread_self();
    malloc_logger = countAllocation;
    allocArmed = true;
    void* volatile p = std::malloc (64);
    allocArmed = false;
    std::free (p);
    if (allocCount != 1)
        return fail ("--alloc: the malloc hook is not live (a deliberate malloc counted " + juce::String (allocCount) + ")");
    std::cout << "PASS alloc hook live (deliberate malloc counted 1)" << std::endl;
    allocCount = 0;
    allocTrace = true;

    for (const double rate : { 48000.0, 192000.0 })
    {
        Rig r (rate, 32);
        const auto& sp = r.proc.getSpectralForTests();
        setParam (r.proc, "ENGINE", 1.0f);
        setParam (r.proc, "DIFFUSION", 40.0f);
        setParam (r.proc, "DRIFT", 60.0f);
        setParam (r.proc, "DETUNE", 20.0f);
        setParam (r.proc, "AVG_FRAMES", 16.0f);
        r.run (1.0);
        r.armAlloc = true;
        bool ok = true;
        // rolling-path engage, frozen, release
        setParam (r.proc, "FREEZE", 1.0f);   ok = ok && r.run (1.0);
        setParam (r.proc, "FREEZE", 0.0f);   ok = ok && r.run (0.5);
        // fallback-path engage (engine switched just before)
        r.armAlloc = false;
        setParam (r.proc, "ENGINE", 0.0f);   r.run (0.2);
        r.armAlloc = true;
        setParam (r.proc, "ENGINE", 1.0f);   ok = ok && r.run (0.01);
        setParam (r.proc, "FREEZE", 1.0f);   ok = ok && r.run (0.5);
        // both switch directions while frozen, LFO on
        setParam (r.proc, "LFO_DEPTH", 80.0f);
        setParam (r.proc, "ENGINE", 0.0f);   ok = ok && r.run (0.5);
        setParam (r.proc, "ENGINE", 1.0f);   ok = ok && r.run (0.5);
        setParam (r.proc, "FREEZE", 0.0f);   ok = ok && r.run (0.5);
        r.armAlloc = false;

        const bool pass = ok && allocCount == 0 && sp.getFallbackCaptures() >= 2;
        std::cout << (pass ? "PASS " : "FAIL ") << "alloc @" << rate << " Hz block 32: " << allocCount
                  << " allocations in processBlock (captures " << sp.getCaptures() << ", fallback "
                  << sp.getFallbackCaptures() << ")" << std::endl;
        if (! pass)
            return 1;
    }
    std::cout << "ALLOC PASS" << std::endl;
    return 0;
   #else
    return fail ("--alloc is macOS-only");
   #endif
}

int timing (double rate, int bs, double seconds)
{
    Rig r (rate, bs);
    setParam (r.proc, "ENGINE", 1.0f);
    setParam (r.proc, "DIFFUSION", 50.0f);
    setParam (r.proc, "DRIFT", 50.0f);
    setParam (r.proc, "DETUNE", 10.0f);
    setParam (r.proc, "LFO_DEPTH", 50.0f);
    r.run (1.0);

    setParam (r.proc, "FREEZE", 1.0f);
    r.run (static_cast<double> (bs) / rate);   // the engage block
    const double engage = r.lastBlockSeconds;

    const long long n = static_cast<long long> (seconds * rate / bs);
    double worst = 0.0, sum = 0.0;
    std::vector<double> times;
    times.reserve (static_cast<size_t> (n));
    for (long long b = 0; b < n; ++b)
    {
        r.run (static_cast<double> (bs) / rate);
        times.push_back (r.lastBlockSeconds);
        worst = std::max (worst, r.lastBlockSeconds);
        sum += r.lastBlockSeconds;
    }
    std::sort (times.begin(), times.end());

    // engage blocks: 20 release / re-engage cycles (rolling capture + 4 primed frames)
    std::vector<double> engages;
    for (int i = 0; i < 20; ++i)
    {
        setParam (r.proc, "FREEZE", 0.0f);
        r.run (0.4);
        setParam (r.proc, "FREEZE", 1.0f);
        r.run (static_cast<double> (bs) / rate);
        engages.push_back (r.lastBlockSeconds);
    }
    std::sort (engages.begin(), engages.end());
    const double period = static_cast<double> (bs) / rate;
    const double p999 = times[static_cast<size_t> (0.999 * static_cast<double> (times.size() - 1))];
    const bool ok = worst < 0.5 * period;
    std::cout << "timing @" << rate << " Hz block " << bs << ", " << seconds << " s frozen: worst "
              << worst * 1.0e6 << " us, p99.9 " << p999 * 1.0e6 << " us, mean " << sum / static_cast<double> (n) * 1.0e6
              << " us; first engage block " << engage * 1.0e6 << " us, re-engage median "
              << engages[engages.size() / 2] * 1.0e6 << " us / max " << engages.back() * 1.0e6 << " us; budget (50 % of " << period * 1.0e6 << " us) "
              << 0.5 * period * 1.0e6 << " us" << std::endl;
    std::cout << (ok ? "PASS" : "FAIL") << " worst steady block < 50 % of the block period" << std::endl;
    return ok ? 0 : 1;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    juce::ArgumentList args (argc, argv);

    if (args.containsOption ("--fft-identity")) return fftIdentity();
    if (args.containsOption ("--probe"))        return probe();
    if (args.containsOption ("--alloc"))        return allocCheck();
    if (args.containsOption ("--timing"))
    {
        const double rate = args.containsOption ("--rate") ? args.getValueForOption ("--rate").getDoubleValue() : 48000.0;
        const int bs = args.containsOption ("--block") ? args.getValueForOption ("--block").getIntValue() : 32;
        const double secs = args.containsOption ("--seconds") ? args.getValueForOption ("--seconds").getDoubleValue() : 20.0;
        return timing (rate, bs, secs);
    }
    return fail ("usage: --fft-identity | --probe | --alloc | --timing [--rate=48000] [--block=32] [--seconds=20]");
}
