/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    O-simpleGrain render-harness — v2.0.0 Pulsar-mode checks (Wave A).

      P1 bh-moments       the internal Blackman-Harris table: mean 0.35875 and
                          mean-square 0.25796, each within 1e-4.
      P2 sine-no-source   a GrainVoice in Pulsar / Sine with a NaN-POISONED
                          source buffer renders finite, audible output (Sine never
                          reads the source), and with no source at all still
                          sounds. Control: the same voice in Sample mode reads the
                          poison (non-finite) — the poison is reachable.
      P3 content-switch   Sine -> Sample switched mid-note at a block boundary
                          that lands INSIDE a pulsaret: every output sample equals
                          the Sine-only render up to the first onset after the
                          switch and the Sample-only render from it on (content is
                          latched per pulsaret, so no pulsaret changes content
                          mid-window), and the switch region's worst second
                          difference is no larger than the larger of the two
                          same-motion steady renders' (ratio <= 1). A build that
                          reads the content live fails both.
      P4 viz-decimation   8 voices at the top of the keyboard, 512-sample blocks:
                          GrainEvents per block <= 8 x ceil(512 / 480) and never
                          reach kMaxEvents (so none is dropped); a voice rendered
                          with and without a viz frame attached is bit-identical.
      P5 alloc + CPU      every Pulsar feature on, 8 voices: 0 heap allocations
                          inside processBlock (libmalloc malloc_logger, audio thread
                          only, with a liveness probe); CPU of 8 voices at the top
                          of the keyboard, Sample content, vs Cloud, printed.

      P6 mode-switch      (Wave C) Cloud -> Pulsar and Pulsar -> Cloud switched
                          mid-note at a block boundary inside a pulsaret, with the
                          Cloud overlap at 4 so the two kinds' gains differ by 6 dB
                          (Cloud normGain 0.5, pulsarets 1). Verdict vs the
                          SAME-MOTION model R = old-mode tail + new-mode note
                          started at the switch, each rendered on its own fresh
                          processor (the tail by a note-off at the switch, ADSR off:
                          no new grains, in-flight ones window out) and each at its
                          OWN bus gain: identical before the switch, max|S - R|
                          <= -100 dB re peak after it, and worst second difference
                          S / R <= 1.01 around it. A build that cuts the other
                          kind's grains, or ramps one shared output gain across the
                          switch (pre-fix 2.0.0: the draining Cloud tail swelled by
                          1 / normGain), fails.
                          (The vs-static ratio is printed as INFO only: a mode
                          switch legitimately overlaps two signals.)

      P7 fm-crossing      GrainVoice::fmCrossingDelta (the sub-sample onset under
                          rate FM) vs a 200-step bisection of the same phase
                          integral, 200 000 seeded cases over modulator steps
                          Δm 0.005..1, depth 0.5 / 0.95, ratio 0.25..8, f_p <= 0.1 fs:
                          worst |δ - δref| <= 1e-7 sample. Control (in-harness):
                          the pre-fix plain 3-step Newton must fail the same bound.
                          (An integer-ratio spectrum cannot see a solver error: every
                          onset lands at the same modulator phase, so the error is
                          one constant shift, not jitter.)

    usage: O-simpleGrain-render-test [--pulsar [P2,P6]]   (--pulsar: only these checks)

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"
#include "GrainVoice.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>

#if JUCE_MAC
 #include <execinfo.h>
 #include <pthread.h>
extern "C"
{
    typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                    uintptr_t result, uint32_t numHotFramesToSkip);
    extern malloc_logger_t* malloc_logger;
}
#endif

namespace PID = OSimpleGrain::ParamIDs;

namespace
{
#if JUCE_MAC
// volatile: clang treats malloc() as a builtin that cannot touch globals and would
// fold "armed = true; malloc(); armed = false" (memory: malloc_logger gate traps).
volatile bool allocArmed = false;
volatile int  allocCount = 0;
pthread_t     audioThread;

void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
{
    // Process-wide hook; JUCE's timer thread mallocs at will — count the audio thread only.
    if (allocArmed && (type & 2u) != 0 && pthread_equal (pthread_self(), audioThread))
    {
        allocArmed = false;            // the first one is the finding: say where it came from
        allocCount = allocCount + 1;
        void* frames[32];
        backtrace_symbols_fd (frames, backtrace (frames, 32), 2);   // does not malloc
    }
}
#endif

void setP (juce::AudioProcessorValueTreeState& a, const char* id, float real)
{
    if (auto* p = a.getParameter (id))
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (real));
    else
        std::printf ("  !! unknown param id '%s'\n", id);
}

void resetAll (juce::AudioProcessorValueTreeState& a)
{
    for (auto* p : a.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    setP (a, PID::ampSustain, 1.0f);
    setP (a, PID::outputLevel, 0.0f);
}

void pump (int ms = 150)
{
    if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
        mm->runDispatchLoopUntil (ms);
}

double curvature (const std::vector<float>& x, int from, int to)
{
    double c = 0.0;
    for (int n = juce::jmax (1, from); n < juce::jmin ((int) x.size() - 1, to); ++n)
        c = juce::jmax (c, (double) std::abs (x[(size_t) n + 1] - 2.0f * x[(size_t) n] + x[(size_t) n - 1]));
    return c;
}

// Render channel 0 through the processor; notes on at sample 0, held.
// `onBlock(b)` runs before block b (parameter moves at block boundaries).
template <typename Fn>
std::vector<float> renderProc (OSimpleGrainAudioProcessor& proc, const std::vector<int>& notes,
                               int total, int block, Fn&& onBlock, int* maxEvents = nullptr)
{
    juce::AudioBuffer<float> buf (2, block);
    std::vector<float> out;
    out.reserve ((size_t) total);
    int peakEv = 0;
    for (int pos = 0, b = 0; pos < total; pos += block, ++b)
    {
        onBlock (b);
        buf.clear();
        juce::MidiBuffer midi;
        if (b == 0)
            for (int nn : notes)
                midi.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 100), 0);
        proc.processBlock (buf, midi);
        if (maxEvents != nullptr)
        {
            auto& tb = proc.getGrainCloudBuffer();
            if (tb.hasNewData())
                peakEv = juce::jmax (peakEv, tb.read().count);
        }
        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
    }
    if (maxEvents != nullptr) *maxEvents = peakEv;
    return out;
}

std::vector<float> renderProc (OSimpleGrainAudioProcessor& proc, const std::vector<int>& notes, int total, int block)
{
    return renderProc (proc, notes, total, block, [] (int) {});
}

// A bare GrainVoice (no processor): Pulsar params, a given source, a resting playhead.
struct VoiceRig
{
    WindowLuts luts { OSimpleGrainAudioProcessor::kWindowLutSize };
    GrainVoice voice;
    juce::uint64 seed = 0x1234567887654321ull, counter = 0;
    std::vector<double> track;
    GrainSound sound;

    VoiceRig (double fs, int block, const GrainVoiceParams& p)
    {
        voice.setWindowLuts (&luts);
        voice.setRngSource (&seed, &counter);
        voice.prepareToPlay (fs, block);
        voice.setParams (p);
        track.assign ((size_t) block, 100.0);
        voice.setPlayheadTrack (track.data(), block);
    }

    std::vector<float> run (int note, int blocks, int block, GrainCloudFrame* frame = nullptr, int* maxEv = nullptr)
    {
        juce::AudioBuffer<float> buf (2, block);
        std::vector<float> out;
        voice.startNote (note, 0.8f, &sound, 0);
        int peak = 0;
        for (int b = 0; b < blocks; ++b)
        {
            buf.clear();
            if (frame != nullptr) frame->count = 0;
            voice.setGrainCloudFrame (frame);
            voice.renderNextBlock (buf, 0, block);
            if (frame != nullptr) peak = juce::jmax (peak, frame->count);
            for (int i = 0; i < block; ++i) out.push_back (buf.getSample (0, i));
        }
        if (maxEv != nullptr) *maxEv = peak;
        return out;
    }
};

bool finiteAll (const std::vector<float>& x)
{
    for (float s : x) if (! std::isfinite (s)) return false;
    return true;
}

double rmsOf (const std::vector<float>& x, size_t from = 0)
{
    double a = 0.0;
    for (size_t i = from; i < x.size(); ++i) a += (double) x[i] * x[i];
    return std::sqrt (a / (double) juce::jmax ((size_t) 1, x.size() - from));
}
} // namespace

//==============================================================================
int runPulsarChecks (const juce::String& only);
int runPulsarChecks (const juce::String& only)
{
    int failures = 0;
    // only: comma list of check ids ("P2,P6"); empty = all.
    auto want = [&] (const char* id) { return only.isEmpty() || juce::StringArray::fromTokens (only, ",", "").contains (id); };
    auto check = [&] (const char* name, bool ok, const juce::String& detail)
    {
        std::printf ("  [%s] %-22s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    };
    std::printf ("\nPulsar mode (v2.0.0)\n");

    // --- P1 ---------------------------------------------------------------------
    if (want ("P1"))
    {
        WindowLuts luts (OSimpleGrainAudioProcessor::kWindowLutSize);
        const double m1 = luts.bhMean(), m2 = luts.bhMeanSquare();
        check ("P1 bh-moments", std::abs (m1 - 0.35875) <= 1e-4 && std::abs (m2 - 0.25796) <= 1e-4,
               "mean " + juce::String (m1, 6) + " (0.35875), mean-square " + juce::String (m2, 6)
                 + " (0.25796), BH power gain " + juce::String (luts.bhPowerGain(), 4)
                 + " / sine " + juce::String (luts.bhPowerGain (true), 4));
    }

    // --- P2 ---------------------------------------------------------------------
    if (want ("P2"))
    {
        const double fs = 48000.0;
        const int block = 256;
        GrainVoiceParams p;
        p.grainMode = 1; p.pulsaretSource = 0; p.formant = 4.0f; p.adsrEnabled = false;
        std::vector<float> poison (48000, std::numeric_limits<float>::quiet_NaN());

        VoiceRig a (fs, block, p);
        a.voice.setSource (poison.data(), nullptr, (int) poison.size());
        const auto ya = a.run (57, 100, block);

        VoiceRig b (fs, block, p);
        b.voice.setSource (nullptr, nullptr, 0);
        const auto yb = b.run (57, 100, block);

        GrainVoiceParams ps = p; ps.pulsaretSource = 1;
        VoiceRig c (fs, block, ps);
        c.voice.setSource (poison.data(), nullptr, (int) poison.size());
        const auto yc = c.run (57, 100, block);

        const double ra = rmsOf (ya), rb = rmsOf (yb);
        // audibility floor 1e-3 (-60 dBFS): the Sine content carries the -24.1 dB trim
        check ("P2 sine-no-source", finiteAll (ya) && ra > 1.0e-3 && finiteAll (yb) && rb > 1.0e-3 && ! finiteAll (yc),
               "Sine over a NaN source: finite " + juce::String (finiteAll (ya) ? "yes" : "NO")
                 + ", RMS " + juce::String (ra, 4) + "; no source: RMS " + juce::String (rb, 4)
                 + "; control Sample over the NaN source: " + juce::String (finiteAll (yc) ? "finite (POISON UNREACHED)" : "non-finite (reads it)"));
    }

    // --- P3 ---------------------------------------------------------------------
    if (want ("P3"))
    {
        const double fs = 48000.0;
        const int block = 64;
        // Formant 1.25x at 220 Hz: a pulsaret covers 80 % of each period, so a switch at a
        // block boundary almost surely lands inside one (checked below). A FRESH processor
        // per render, so no render inherits another's draining voice.
        // switch at 19264: 64 samples into the pulsaret that starts at 88 x fs / 220 = 19200
        // (a boundary ON an onset would leave no pulsaret in flight and prove nothing).
        const int total = (int) (0.6 * fs), switchBlock = 301;
        const int sw = switchBlock * block;
        auto run = [&] (float content, bool doSwitch)
        {
            OSimpleGrainAudioProcessor proc;
            proc.setPlayConfigDetails (0, 2, fs, block);
            proc.prepareToPlay (fs, block);
            auto& ap = proc.getAPVTS();
            resetAll (ap);
            setP (ap, PID::grainMode, 1.0f);
            setP (ap, PID::formant, 1.25f);
            setP (ap, PID::pulsaretSource, content);
            setP (ap, PID::adsrEnabled, 0.0f);
            return renderProc (proc, { 57 }, total, block, [&] (int b)
            {
                if (doSwitch && b == switchBlock) setP (ap, PID::pulsaretSource, 1.0f);
            });
        };
        const auto ySine   = run (0.0f, false);
        const auto ySample = run (1.0f, false);
        const auto ySw     = run (0.0f, true);

        // The first onset at or after the switch: pulses every fs / 220 samples from 0.
        const double period = fs / 220.0;
        const int nextOnset = (int) std::ceil (std::ceil ((double) sw / period) * period);
        const int prevOnset = (int) std::ceil (std::floor ((double) sw / period) * period);
        const bool inside = (sw - prevOnset) > 2 && (sw - prevOnset) < (int) (0.8 * period) - 2;   // inside a pulsaret
        bool before = true, after = true;
        for (int n = 0; n < total; ++n)
        {
            if (n < nextOnset - 1 && ! juce::exactlyEqual (ySw[(size_t) n], ySine[(size_t) n]))   before = false;
            if (n > nextOnset + 1 && ! juce::exactlyEqual (ySw[(size_t) n], ySample[(size_t) n])) after = false;
        }
        const int w0 = sw - 2 * (int) period, w1 = sw + 3 * (int) period;
        const double cSw = curvature (ySw, w0, w1);
        const double cRef = juce::jmax (curvature (ySine, w0, w1), curvature (ySample, w0, w1));
        const double ratio = cSw / juce::jmax (1.0e-12, cRef);
        const double lvSine = rmsOf (ySine, (size_t) (0.2 * fs)), lvSample = rmsOf (ySample, (size_t) (0.2 * fs));
        std::printf ("  [INFO] P3 level step          Sine -> Sample steady RMS %.5f -> %.5f (%+.1f dB, fire, Formant 1.25x, MIDI 57)\n",
                     lvSine, lvSample, 20.0 * std::log10 (lvSample / juce::jmax (1.0e-12, lvSine)));
        check ("P3 content-switch", inside && before && after && ratio <= 1.0 + 1.0e-6,
               "switch at " + juce::String (sw) + " inside a pulsaret: " + juce::String (inside ? "yes" : "NO")
                 + "; = Sine render before the next onset: " + juce::String (before ? "yes" : "NO")
                 + "; = Sample render after: " + juce::String (after ? "yes" : "NO")
                 + "; curvature ratio vs same-motion steady renders " + juce::String (ratio, 4) + " (<= 1)");
    }

    // --- P4 ---------------------------------------------------------------------
    if (want ("P4"))
    {
        const double fs = 48000.0;
        const int block = 512;
        OSimpleGrainAudioProcessor proc;
        proc.setPlayConfigDetails (0, 2, fs, block);
        proc.prepareToPlay (fs, block);
        auto& ap = proc.getAPVTS();
        resetAll (ap);
        setP (ap, PID::grainMode, 1.0f);
        setP (ap, PID::formant, 32.0f);
        int maxEv = 0;
        renderProc (proc, { 101, 102, 103, 104, 105, 106, 107, 108 }, (int) (1.0 * fs), block, [] (int) {}, &maxEv);
        const int bound = 8 * ((block + 479) / 480);

        GrainVoiceParams p;
        p.grainMode = 1; p.formant = 6.0f; p.panSpray = 50.0f; p.pitchSpray = 2.0f; p.fmDepth = 0.5f; p.fmRatio = 1.37f;
        VoiceRig a (fs, block, p), b (fs, block, p);
        GrainCloudFrame frame;
        int vEv = 0;
        const auto ya = a.run (90, 100, block, &frame, &vEv);
        const auto yb = b.run (90, 100, block, nullptr);
        check ("P4 viz-decimation", maxEv <= bound && maxEv < GrainCloudFrame::kMaxEvents && maxEv > 0
                                      && vEv > 0 && ya == yb,
               "8 voices MIDI 101-108 x 32: max " + juce::String (maxEv) + " events / block (bound "
                 + juce::String (bound) + ", kMaxEvents " + juce::String (GrainCloudFrame::kMaxEvents)
                 + "); one voice frame attached vs detached: " + juce::String (ya == yb ? "bit-identical" : "DIFFERS")
                 + " (" + juce::String (vEv) + " events/block attached)");
    }

    // --- P5 ---------------------------------------------------------------------
    if (want ("P5"))
    {
        const double fs = 48000.0;
        const int block = 512;
        OSimpleGrainAudioProcessor proc;
        proc.setPlayConfigDetails (0, 2, fs, block);
        proc.prepareToPlay (fs, block);
        auto& ap = proc.getAPVTS();
        const std::vector<int> chord { 101, 102, 103, 104, 105, 106, 107, 108 };

        auto pulsarAll = [&]
        {
            resetAll (ap);
            setP (ap, PID::sourceSample, 3.0f);     // piano
            pump();
            setP (ap, PID::grainMode, 1.0f);
            setP (ap, PID::pulsaretSource, 1.0f);   // Sample
            setP (ap, PID::formant, 32.0f);
            setP (ap, PID::pulseMask, 2.0f);        // index 2 = mask 3
            setP (ap, PID::grainSkip, 30.0f);
            setP (ap, PID::fmDepth, 100.0f);
            setP (ap, PID::fmRatio, 1.5f);
            setP (ap, PID::glisson, 12.0f);
            setP (ap, PID::pitchSpray, 6.0f);
            setP (ap, PID::positionSpray, 40.0f);
            setP (ap, PID::panSpray, 80.0f);
            setP (ap, PID::scan, 50.0f);
        };

       #if JUCE_MAC
        pulsarAll();
        renderProc (proc, chord, block * 4, block);       // warm-up (first-block posts) ...
        {
            juce::AudioBuffer<float> buf (2, block);          // ... then release it fully, so the
            juce::MidiBuffer offs;                            // measured note-ons find free voices
            for (int nn : chord) offs.addEvent (juce::MidiMessage::noteOff (1, nn), 0);
            proc.processBlock (buf, offs);
            for (int b = 0; b < (int) fs / block; ++b) { buf.clear(); juce::MidiBuffer m; proc.processBlock (buf, m); }
        }
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const bool live = (allocCount == 1);
        allocCount = 0;
        {
            juce::AudioBuffer<float> buf (2, block);
            const int blocks = (int) (3.0 * fs) / block;
            for (int b = 0; b < blocks; ++b)
            {
                buf.clear();
                juce::MidiBuffer midi;
                if (b == 0)
                    for (int nn : chord) midi.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 100), 0);
                // Release, let every voice finish (ADSR release 0.4 s), then re-strike: the
                // note-off / note-on paths run without a voice steal. (A STEAL allocates
                // inside juce::Synthesiser::findVoiceToSteal in either mode — pre-existing,
                // measured separately below as INFO.)
                if (b == blocks / 3)
                    for (int nn : chord) midi.addEvent (juce::MidiMessage::noteOff (1, nn), 0);
                if (b == 2 * blocks / 3)
                    for (int nn : chord) midi.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 90), 0);
                allocArmed = true;
                proc.processBlock (buf, midi);
                allocArmed = false;
            }
        }
        const int pulsarAllocs = allocCount;
        check ("P5 alloc", live && pulsarAllocs == 0,
               "malloc hook live: " + juce::String (live ? "yes" : "NO") + "; heap allocations in processBlock over 3 s, "
                 "8 voices, every Pulsar feature on, release + re-strike: " + juce::String (pulsarAllocs));

        // INFO: a voice STEAL (9th note while 8 voices sound) — the same in Cloud and Pulsar.
        auto stealAllocs = [&] (float mode)
        {
            OSimpleGrainAudioProcessor pr;
            pr.setPlayConfigDetails (0, 2, fs, block);
            pr.prepareToPlay (fs, block);
            setP (pr.getAPVTS(), PID::grainMode, mode);
            juce::AudioBuffer<float> buf (2, block);
            for (int b = 0; b < 20; ++b)
            {
                buf.clear();
                juce::MidiBuffer midi;
                if (b == 2)
                    for (int nn : chord) midi.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 100), 0);
                if (b == 10)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
                allocCount = 0;
                allocArmed = (b >= 10);
                pr.processBlock (buf, midi);
                allocArmed = false;
                if (b == 10) return (int) allocCount;
            }
            return -1;
        };
        std::fprintf (stderr, "(the steal traces below are the INFO probe)\n");
        const int stealCloud = stealAllocs (0.0f), stealPulsar = stealAllocs (1.0f);
        malloc_logger = nullptr;
        std::printf ("  [INFO] P5 steal-alloc         a 9th note steals a voice: >= 1 allocation in "
                     "juce::Synthesiser::findVoiceToSteal — Cloud %d, Pulsar %d (pre-existing, mode-independent)\n",
                     stealCloud, stealPulsar);
       #endif

        // CPU: 10 s of 8 voices, Sample content at the top of the keyboard, vs Cloud.
        auto timeIt = [&] (double seconds)
        {
            juce::AudioBuffer<float> buf (2, block);
            const int blocks = (int) (seconds * fs) / block;
            const auto t0 = std::chrono::steady_clock::now();
            for (int b = 0; b < blocks; ++b)
            {
                buf.clear();
                juce::MidiBuffer midi;
                if (b == 0)
                    for (int nn : chord) midi.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 100), 0);
                proc.processBlock (buf, midi);
            }
            const double el = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            return 100.0 * el / seconds;
        };
        auto release = [&]
        {
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer midi;
            for (int nn : chord) midi.addEvent (juce::MidiMessage::noteOff (1, nn), 0);
            proc.processBlock (buf, midi);
            for (int b = 0; b < 400; ++b) { buf.clear(); juce::MidiBuffer m; proc.processBlock (buf, m); }
        };
        pulsarAll();
        setP (ap, PID::pulseMask, 0.0f); setP (ap, PID::grainSkip, 0.0f);     // every pulse sounds
        release();
        const double cpuPulsar = timeIt (10.0);
        release();
        setP (ap, PID::fmDepth, 0.0f); setP (ap, PID::glisson, 0.0f);
        release();
        const double cpuPulsarPlain = timeIt (10.0);
        release();
        resetAll (ap);
        setP (ap, PID::sourceSample, 3.0f);
        pump();
        const double cpuCloud = timeIt (10.0);
        release();
        setP (ap, PID::density, 200.0f); setP (ap, PID::grainSize, 500.0f); setP (ap, PID::grainPitch, 24.0f);
        const double cpuCloudMax = timeIt (10.0);
        release();
        std::printf ("  [INFO] P5 cpu                 8 voices MIDI 101-108, 48 kHz, block 512, %% of real time: "
                     "Pulsar Sample x32 FM+glisson %.2f %%, Pulsar Sample x32 plain %.2f %%, "
                     "Cloud defaults %.2f %%, Cloud 500 ms x 200 g/s +24 st %.2f %%\n",
                     cpuPulsar, cpuPulsarPlain, cpuCloud, cpuCloudMax);
    }

    // --- P6 ---------------------------------------------------------------------
    if (want ("P6"))
    {
        const double fs = 48000.0;
        const int block = 64;
        const int total = (int) (0.6 * fs), switchBlock = 301;   // as P3: 64 samples into a pulsaret
        const int sw = switchBlock * block;
        const double period = fs / 220.0;
        float normGain = 0.0f;
        // A FRESH processor per render. mode0 from block 0, mode1 from switchBlock
        // (< 0: no switch); MIDI 57 on at onBlock, off at offBlock (< 0: held).
        auto run = [&] (float mode0, float mode1, int onBlock, int offBlock)
        {
            OSimpleGrainAudioProcessor proc;
            proc.setPlayConfigDetails (0, 2, fs, block);
            proc.prepareToPlay (fs, block);
            auto& ap = proc.getAPVTS();
            resetAll (ap);
            setP (ap, PID::adsrEnabled, 0.0f);
            setP (ap, PID::grainSize, 100.0f);    // overlap 4 -> Cloud normGain 0.5:
            setP (ap, PID::density, 40.0f);       // the switch moves the output gain 6 dB
            setP (ap, PID::formant, 1.25f);       // 80 % duty
            setP (ap, PID::grainMode, mode0);
            // Cloud normGain as the processor forms it (Hann, no tilt, no skip, under the cap).
            const float rawOverlap = (ap.getRawParameterValue (PID::grainSize)->load() * 0.001f)
                                   * ap.getRawParameterValue (PID::density)->load();
            normGain = 1.0f / std::sqrt (juce::jmax (1.0f, rawOverlap * 1.0f));
            juce::AudioBuffer<float> buf (2, block);
            std::vector<float> out;
            out.reserve ((size_t) total);
            for (int b = 0; b * block < total; ++b)
            {
                if (mode1 >= 0.0f && b == switchBlock) setP (ap, PID::grainMode, mode1);
                buf.clear();
                juce::MidiBuffer midi;
                if (b == onBlock)  midi.addEvent (juce::MidiMessage::noteOn (1, 57, (juce::uint8) 100), 0);
                if (b == offBlock) midi.addEvent (juce::MidiMessage::noteOff (1, 57), 0);
                proc.processBlock (buf, midi);
                for (int i = 0; i < block; ++i) out.push_back (buf.getSample (0, i));
            }
            return out;
        };
        const int shapeDefault = (int) OSimpleGrainAudioProcessor().getAPVTS().getRawParameterValue (PID::windowShape)->load();

        auto direction = [&] (float from, float to, const char* name)
        {
            const auto S    = run (from, to, 0, -1);
            const auto tail = run (from, -1.0f, 0, switchBlock);   // old mode, note off at the switch
            const auto late = run (to, -1.0f, switchBlock, -1);    // new mode, note on at the switch
            const auto stA  = run (from, -1.0f, 0, -1);            // static renders (INFO)
            const auto stB  = run (to, -1.0f, 0, -1);
            const float gFrom = (from == 0.0f) ? normGain : 1.0f, gTo = (to == 0.0f) ? normGain : 1.0f;

            std::vector<float> R ((size_t) total);
            bool premise = true;
            double peak = 0.0, resid = 0.0, tailRms = 0.0, lateRms = 0.0;
            for (int n = 0; n < total; ++n)
            {
                if (n < sw)
                {
                    R[(size_t) n] = tail[(size_t) n];
                    if (! juce::exactlyEqual (S[(size_t) n], tail[(size_t) n])) premise = false;
                    continue;
                }
                R[(size_t) n] = tail[(size_t) n] + late[(size_t) n];   // each kind on its own bus gain
                peak  = juce::jmax (peak, (double) std::abs (S[(size_t) n]));
                resid = juce::jmax (resid, (double) std::abs (S[(size_t) n] - R[(size_t) n]));
                if (n < sw + 3 * (int) period)
                {
                    tailRms += (double) tail[(size_t) n] * tail[(size_t) n];
                    lateRms += (double) late[(size_t) n] * late[(size_t) n];
                }
            }
            tailRms = std::sqrt (tailRms / (3 * (int) period));
            lateRms = std::sqrt (lateRms / (3 * (int) period));
            const double residDb = 20.0 * std::log10 (juce::jmax (1.0e-30, resid / juce::jmax (1.0e-30, peak)));
            const int w0 = sw - 2 * (int) period, w1 = sw + 3 * (int) period;
            const double cS = curvature (S, w0, w1), cR = curvature (R, w0, w1);
            const double ratio = cS / juce::jmax (1.0e-12, cR);
            const double cStatic = juce::jmax (curvature (stA, w0, w1), curvature (stB, w0, w1));
            const double stepDb = 20.0 * std::log10 ((double) gTo / gFrom);
            const bool live = tailRms > 1.0e-4 && lateRms > 1.0e-4 && std::abs (stepDb) >= 3.0;
            std::printf ("  [INFO] P6 %-15s vs static (larger of the two steady renders) curvature ratio %.3f "
                         "(not gated: the switch overlaps two signals)\n", name, cS / juce::jmax (1.0e-12, cStatic));
            check ((juce::String ("P6 ") + name).toRawUTF8(), premise && live && residDb <= -100.0 && ratio <= 1.01,
                   "switch at " + juce::String (sw) + "; = old-mode render before it: " + juce::String (premise ? "yes" : "NO")
                     + "; after it max|S - R| " + juce::String (residDb, 1) + " dB re peak (<= -100)"
                     + "; curvature S/R " + juce::String (ratio, 4) + " (<= 1.01)"
                     + "; liveness: tail RMS " + juce::String (tailRms, 5) + ", new-mode RMS " + juce::String (lateRms, 5)
                     + ", Cloud vs Pulsar bus gain " + juce::String (stepDb, 2) + " dB (|diff| >= 3)");
        };
        direction (0.0f, 1.0f, "Cloud->Pulsar");
        direction (1.0f, 0.0f, "Pulsar->Cloud");
        std::printf ("  [INFO] P6 setup           window shape %d (Hann = %d), Cloud normGain %.6f\n",
                     shapeDefault, (int) WindowLuts::kShapeHann, (double) normGain);
    }

    // --- P7 ---------------------------------------------------------------------
    if (want ("P7"))
    {
        constexpr double twoPi = juce::MathConstants<double>::twoPi;
        constexpr double pi    = juce::MathConstants<double>::pi;
        // The pre-fix 2.0.0 solve: 3 plain Newton steps, clamped (the control).
        auto plainNewton = [] (double before, double inc0, double inc, double fmD, double phm0, double step)
        {
            const double c0 = std::cos (twoPi * phm0);
            const double k  = fmD / (twoPi * step);
            double s = juce::jlimit (0.0, 1.0, (1.0 - before) / inc);
            for (int it = 0; it < 3; ++it)
            {
                const double a = twoPi * (phm0 + step * s);
                const double f = before + inc0 * (s + k * (c0 - std::cos (a))) - 1.0;
                s = juce::jlimit (0.0, 1.0, s - f / (inc0 * (1.0 + fmD * std::sin (a))));
            }
            return juce::jlimit (0.0, 0.999999, 1.0 - s);
        };
        juce::Random rng (0x5eed07);
        constexpr double ratios[] = { 0.25, 0.5, 1.5, 3.0, 8.0 };
        double worstNew = 0.0, worstOld = 0.0, worstOldLowDm = 0.0;
        int cases = 0;
        for (int n = 0; n < 200000; ++n)
        {
            const double step  = 0.005 + 0.995 * rng.nextDouble();
            const double fmD   = (n & 1) ? 0.95 : 0.5;
            const double inc0  = step / ratios[rng.nextInt (5)];
            if (inc0 > 0.1)
                continue;
            const double phm0  = rng.nextDouble();
            const double sinc  = std::sin (pi * step) / (pi * step);
            const double inc   = inc0 * (1.0 + fmD * sinc * std::sin (twoPi * (phm0 + 0.5 * step)));
            const double before = juce::jmax (0.0, 1.0 - inc) + rng.nextDouble() * juce::jmin (1.0, inc);
            if (before >= 1.0 || before + inc < 1.0)
                continue;
            const double c0 = std::cos (twoPi * phm0), k = fmD / (twoPi * step);
            double lo = 0.0, hi = 1.0;
            for (int it = 0; it < 200; ++it)
            {
                const double m = 0.5 * (lo + hi);
                (before + inc0 * (m + k * (c0 - std::cos (twoPi * (phm0 + step * m)))) - 1.0 < 0.0 ? lo : hi) = m;
            }
            const double ref = juce::jlimit (0.0, 0.999999, 1.0 - lo);
            const double eNew = std::abs (GrainVoice::fmCrossingDelta (before, inc0, inc, fmD, phm0, step) - ref);
            const double eOld = std::abs (plainNewton (before, inc0, inc, fmD, phm0, step) - ref);
            worstNew = juce::jmax (worstNew, eNew);
            worstOld = juce::jmax (worstOld, eOld);
            if (step <= 0.1)
                worstOldLowDm = juce::jmax (worstOldLowDm, eOld);
            ++cases;
        }
        check ("P7 fm-crossing", worstNew <= 1.0e-7 && cases > 50000,
               juce::String (cases) + " cases: worst |delta - bisection| " + juce::String (worstNew, 12)
                 + " sample (<= 1e-7)");
        check ("P7 control", worstOld > 1.0e-7,
               "pre-fix 3-step Newton worst " + juce::String (worstOld, 6) + " sample (must exceed 1e-7; "
                 + "at dm <= 0.1 it was " + juce::String (worstOldLowDm, 9) + ")");
    }

    std::printf ("Pulsar checks: %s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures;
}
