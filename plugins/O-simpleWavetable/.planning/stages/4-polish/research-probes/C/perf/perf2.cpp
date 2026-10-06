// Scratch PERF-02 probe (NOT in repo). Release -O3, no OSIW_TEST_HOOKS: the timed code == shipped processor code.
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <time.h>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <sys/resource.h>
using Proc = OSimpleWavetableAudioProcessor;
namespace ids = OSimpleWavetable::ParamIDs;
static void setP (Proc& p, const char* id, float v) { auto* r = p.getAPVTS().getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); }
static double tcpu() { timespec t; clock_gettime (CLOCK_THREAD_CPUTIME_ID, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static double wall() { timespec t; clock_gettime (CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
struct R { double med, p99, mx, wallMed, rms; };
static R run (double fs, int bs, int voices, bool worst, int storm /*0 none, 1 bandlimit toggle per block, 2 bank cycle per block*/)
{
    Proc p;
    setP (p, ids::bank, 0.0f); setP (p, ids::position, 0.5f);
    if (worst) { setP (p, ids::interp, 1.0f); setP (p, ids::bandlimit, 1.0f); setP (p, ids::bitDepth, 14.0f);
                 setP (p, ids::lfoShape, 2.0f); setP (p, ids::lfoDepth, 1.0f); setP (p, ids::lfoRate, 5.0f);
                 setP (p, ids::envAmount, 1.0f); setP (p, ids::menvSustain, 0.5f); setP (p, ids::ampSustain, 1.0f); }
    p.setPlayConfigDetails (0, 2, fs, bs); p.prepareToPlay (fs, bs);
    juce::AudioBuffer<float> buf (2, bs); juce::MidiBuffer midi; midi.ensureSize (4096);
    // warm-up 1 s (notes on in block 0), unmeasured
    const int warm = (int) (fs / bs), meas = (int) (4.0 * fs / bs);
    std::vector<double> t; t.reserve ((size_t) meas); std::vector<double> w; w.reserve ((size_t) meas);
    double ss = 0; long n = 0;
    for (int b = 0; b < warm + meas; ++b)
    {
        midi.clear();
        if (b == 0) for (int v = 0; v < voices; ++v) midi.addEvent (juce::MidiMessage::noteOn (1, 84 + v, (juce::uint8) 100), 0);
        if (storm == 1) setP (p, ids::bandlimit, (float) (b & 1));
        if (storm == 2) setP (p, ids::bank, (float) (b % 5));
        const double c0 = tcpu(), w0 = wall();
        p.processBlock (buf, midi);
        const double c1 = tcpu(), w1 = wall();
        if (b >= warm) { t.push_back ((c1 - c0) * fs / bs); w.push_back ((w1 - w0) * fs / bs);
                         const float* l = buf.getReadPointer (0); for (int i = 0; i < bs; ++i) { ss += (double) l[i] * l[i]; ++n; } }
    }
    std::sort (t.begin(), t.end()); std::sort (w.begin(), w.end());
    return { t[t.size() / 2], t[(size_t) (0.99 * (double) t.size())], t.back(), w[w.size() / 2], std::sqrt (ss / (double) n) };
}
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    rusage r0; getrusage (RUSAGE_SELF, &r0); const double wall0 = wall();
    std::printf ("16 voices C6..D#7, thread-CPU per block / block budget (median | p99 | max), wall median\n");
    for (int storm : { 0, 1 })
        for (double fs : { 96000.0 })
            for (int bs : { 16, 32, 128, 1024, 4096 })
            {
                auto a = run (fs, bs, 16, true, storm);
                std::printf ("%-9s fs %6.0f bs %4d: med %6.3f%% | p99 %6.3f%% | max %6.3f%% | wall med %6.3f%% | rms %.3f\n",
                             storm == 0 ? "steady" : (storm == 1 ? "bl-storm" : "bank-storm"), fs, bs,
                             100 * a.med, 100 * a.p99, 100 * a.mx, 100 * a.wallMed, a.rms);
            }
    auto one = run (96000.0, 512, 1, true, 0); auto six = run (96000.0, 512, 16, true, 0);
    std::printf ("scaling 96k/512: 1 voice %.3f%%, 16 voices %.3f%% (ratio %.1f)\n", 100 * one.med, 100 * six.med, six.med / one.med);
    rusage r1; getrusage (RUSAGE_SELF, &r1);
    const double user = (r1.ru_utime.tv_sec - r0.ru_utime.tv_sec) + 1e-6 * (r1.ru_utime.tv_usec - r0.ru_utime.tv_usec);
    std::printf ("duty witness: user %.2f s / wall %.2f s = %.0f%%\n", user, wall() - wall0, 100 * user / (wall() - wall0));
}
