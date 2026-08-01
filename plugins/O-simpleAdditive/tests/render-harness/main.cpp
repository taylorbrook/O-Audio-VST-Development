/*
   This file is part of O-simpleAdditive, an Ouaricon Audio plugin.
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

    O-simpleAdditive render-harness — Stage 2 DSP correctness gate
                                     + per-voice refill PROFILE (PERF probe).

    Two roles in one console app:

      A) Correctness gate (default): instantiates OSimpleAdditiveAudioProcessor,
         drives MIDI offline, and asserts the additive→wavetable invariants
           1. makes sound          — default params -> non-trivial RMS, finite.
           2. correct pitch        — pure-sine patch -> dominant bin at f0.
           3. morph moves spectrum — scanning A(sine)->B(saw) grows H2 energy.
           4. static = bit-identical— two renders of a static patch are equal
                                      (the once-per-note refill cadence guard).
           5. motion stays finite  — Morph-Pad LFO + spectral-decay, no NaN/Inf.

      B) Profile (--profile): the STEP-1 "is the refill actually hot?" probe for
         the wavetable-refill-cadence improvement. Renders a 16-voice held chord
         (dense Saw spectrum) under TWO regimes at several host block sizes and
         reports per-regime CPU (realtime %):
           • STATIC  — LFO depth 0, decay 0 -> refillTable() runs ONCE per note.
           • MOVING  — Morph-Pad LFO + spectral-decay -> refillTable() every block
                       per active voice (spectrumDirty re-armed each block).
         The MOVING-minus-STATIC delta IS the per-voice refill cost; printing it
         across block sizes exposes the amortization cliff (refill cost is fixed
         per refill, so small host blocks pay it far more often per sample).

    Exit 0 iff all correctness checks pass (profile mode always exits 0; it is a
    measurement, not a gate).

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"

#include <vector>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>

//==============================================================================
static void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float real)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (real));
}

// Single-bin DFT amplitude estimate at frequency f over [off, off+len).
static double binAmplitude (const std::vector<float>& x, int off, int len, double f, double fs)
{
    double re = 0.0, im = 0.0;
    for (int n = 0; n < len; ++n)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * f * n / fs;
        const double s = x[(size_t) (off + n)];
        re += s * std::cos (w);
        im -= s * std::sin (w);
    }
    return 2.0 * std::sqrt (re * re + im * im) / (double) len;
}

static double rms (const std::vector<float>& x, int off, int len)
{
    double acc = 0.0;
    for (int n = 0; n < len; ++n) { const double s = x[(size_t) (off + n)]; acc += s * s; }
    return std::sqrt (acc / (double) len);
}

static bool allFinite (const std::vector<float>& x)
{
    for (float s : x) if (! std::isfinite (s)) return false;
    return true;
}

//==============================================================================
// Reset every param to a known clean baseline (factory defaults of interest).
static void resetDefaults (juce::AudioProcessorValueTreeState& a)
{
    using namespace OSimpleAdditive::ParamIDs;
    for (int k = 0; k < 16; ++k)
        setParam (a, partialIds[k], (k == 0) ? 1.0f : 0.0f);   // pure sine
    setParam (a, frameBSource,  1.0f);   // Saw
    setParam (a, scanPosition,  0.0f);
    setParam (a, scanLfoRate,   0.5f);
    setParam (a, scanLfoDepth,  0.0f);
    setParam (a, scanEnvAmount, 0.0f);
    setParam (a, spectralDecay, 0.0f);
    setParam (a, bitDepth,      0.0f);   // Off
    setParam (a, velToDecay,    0.0f);
    setParam (a, ampAttack,  0.005f);  setParam (a, ampDecay,  0.3f);
    setParam (a, ampSustain, 0.8f);    setParam (a, ampRelease, 0.1f);
    setParam (a, modAttack,  0.005f);  setParam (a, modDecay,  0.3f);
    setParam (a, modSustain, 0.8f);    setParam (a, modRelease, 0.1f);
    setParam (a, outputLevel, 0.0f);
}

// Dense Frame A — all 16 drawbars lifted so the refill inner loop never hits the
// silent-partial skip: the true worst-case fill cost (16 partials × 2048 points).
static void denseSpectrum (juce::AudioProcessorValueTreeState& a)
{
    using namespace OSimpleAdditive::ParamIDs;
    for (int k = 0; k < 16; ++k)
        setParam (a, partialIds[k], 0.8f);
}

//==============================================================================
// Render `seconds` of mono output, holding a chord of `notes` for the whole
// render (note-on at sample 0, never released). Returns channel-0 samples.
static std::vector<float> renderChord (OSimpleAdditiveAudioProcessor& proc,
                                       const std::vector<int>& notes,
                                       double seconds, double fs, int block)
{
    const int total = (int) (seconds * fs);
    juce::AudioBuffer<float> buf (2, block);
    std::vector<float> out;
    out.reserve ((size_t) total);

    int pos = 0;
    bool sentOn = false;
    while (pos < total)
    {
        buf.clear();
        juce::MidiBuffer midi;
        if (! sentOn)
        {
            for (int n : notes)
                midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
            sentOn = true;
        }
        proc.processBlock (buf, midi);

        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
        pos += block;
    }
    return out;
}

// Timed render (no per-sample copy-out — minimise harness overhead in the timing
// path). Returns wall-clock seconds; accumulates a checksum so the optimiser
// cannot elide the render. `xrun`/`maxAbs` report stability.
static double timeChord (OSimpleAdditiveAudioProcessor& proc,
                         const std::vector<int>& notes,
                         double seconds, double fs, int block,
                         double& checksum, double& maxAbs, bool& finite)
{
    const int total = (int) (seconds * fs);
    juce::AudioBuffer<float> buf (2, block);

    juce::MidiBuffer onMidi;
    for (int n : notes)
        onMidi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);

    double sum = 0.0, mx = 0.0;
    bool fin = true;

    const auto t0 = std::chrono::high_resolution_clock::now();
    int pos = 0;
    bool sentOn = false;
    while (pos < total)
    {
        buf.clear();
        juce::MidiBuffer midi;
        if (! sentOn) { midi = onMidi; sentOn = true; }
        proc.processBlock (buf, midi);

        const float* d = buf.getReadPointer (0);
        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i)
        {
            const float s = d[i];
            sum += s;
            const float a = std::abs (s);
            if (a > mx) mx = a;
            if (! std::isfinite (s)) fin = false;
        }
        pos += block;
    }
    const auto t1 = std::chrono::high_resolution_clock::now();

    checksum = sum; maxAbs = mx; finite = fin;
    return std::chrono::duration<double> (t1 - t0).count();
}

//==============================================================================
static int runProfile (double fs)
{
    std::printf ("\n=== O-simpleAdditive REFILL PROFILE (STEP 1: is it hot?) ===\n");
    std::printf ("16-voice held chord, dense Saw spectrum (all 16 drawbars @ 0.8).\n");
    std::printf ("STATIC  = LFO depth 0, decay 0  -> refill once per note.\n");
    std::printf ("MOVING  = Morph-Pad LFO (3 Hz, depth 0.9) + spectral-decay 0.5\n");
    std::printf ("          -> refillTable() every block per active voice.\n");
    std::printf ("realtime%% = render_time / audio_time * 100  (per core; lower is better)\n\n");

    std::vector<int> chord;                       // 16 notes, C2..D#3 cluster (Kmax=16 all)
    for (int i = 0; i < 16; ++i) chord.push_back (36 + i);

    const double seconds = 4.0;
    const int blocks[] = { 64, 128, 256, 512 };

    std::printf ("  block |   STATIC rt%% |   MOVING rt%% |  refill cost (MOVING-STATIC)\n");
    std::printf ("  ------+--------------+--------------+----------------------------\n");

    for (int block : blocks)
    {
        // --- STATIC regime ---
        OSimpleAdditiveAudioProcessor procS;
        procS.setPlayConfigDetails (0, 2, fs, block);
        procS.prepareToPlay (fs, block);
        resetDefaults (procS.getAPVTS());
        denseSpectrum (procS.getAPVTS());          // dense, but no motion
        // warm-up (fill caches / first-note refill), discarded
        { double c, m; bool f; timeChord (procS, chord, 0.5, fs, block, c, m, f); }
        double cS, mS; bool fS;
        const double tS = timeChord (procS, chord, seconds, fs, block, cS, mS, fS);
        procS.releaseResources();

        // --- MOVING regime ---
        OSimpleAdditiveAudioProcessor procM;
        procM.setPlayConfigDetails (0, 2, fs, block);
        procM.prepareToPlay (fs, block);
        resetDefaults (procM.getAPVTS());
        denseSpectrum (procM.getAPVTS());
        setParam (procM.getAPVTS(), OSimpleAdditive::ParamIDs::scanLfoRate,  3.0f);
        setParam (procM.getAPVTS(), OSimpleAdditive::ParamIDs::scanLfoDepth, 0.9f);
        setParam (procM.getAPVTS(), OSimpleAdditive::ParamIDs::spectralDecay, 0.5f);
        { double c, m; bool f; timeChord (procM, chord, 0.5, fs, block, c, m, f); }
        double cM, mM; bool fM;
        const double tM = timeChord (procM, chord, seconds, fs, block, cM, mM, fM);
        procM.releaseResources();

        const double rtS = tS / seconds * 100.0;
        const double rtM = tM / seconds * 100.0;
        const double delta = rtM - rtS;
        const double factor = (tS > 0.0) ? tM / tS : 0.0;

        std::printf ("  %5d | %10.2f   | %10.2f   | +%6.2f pts  (%.2fx)  finite=%s\n",
                     block, rtS, rtM, delta, factor,
                     (fS && fM) ? "yes" : "NO");
    }

    std::printf ("\nReading: if MOVING rt%% climbs sharply as block shrinks while STATIC\n");
    std::printf ("stays flat, the per-block full-table refill is the hot cost and does\n");
    std::printf ("NOT amortize across small host blocks -> cap the refill cadence.\n");
    return 0;
}

//==============================================================================
// Golden set — a battery of STATIC patches (decay 0, LFO depth 0 -> refillTable()
// runs once per note). Each scenario renders from a FRESH processor so the result
// is fully deterministic; the concatenation is the bit-identical contract the
// refill-cadence improvement must NOT perturb. The MOVING/zipper behaviour is
// intentionally excluded — the cap is allowed to change WHEN refills land there.
static std::vector<float> buildGoldenSet (double fs)
{
    using namespace OSimpleAdditive::ParamIDs;
    const int block = 512;
    const double secs = 1.0;

    std::vector<float> all;

    auto scenario = [&] (const std::function<void (juce::AudioProcessorValueTreeState&)>& setup,
                         const std::vector<int>& notes)
    {
        OSimpleAdditiveAudioProcessor proc;
        proc.setPlayConfigDetails (0, 2, fs, block);
        proc.prepareToPlay (fs, block);
        resetDefaults (proc.getAPVTS());
        setup (proc.getAPVTS());
        auto y = renderChord (proc, notes, secs, fs, block);
        proc.releaseResources();
        all.insert (all.end(), y.begin(), y.end());
    };

    // S0: pure sine, single note.
    scenario ([] (auto&) {}, { 57 });
    // S1: dense Saw, single note.
    scenario ([] (auto& a) { denseSpectrum (a); }, { 57 });
    // S2: dense, static morph held at the A<->B midpoint (scanPosition 0.5).
    scenario ([] (auto& a) { denseSpectrum (a); setParam (a, scanPosition, 0.5f); }, { 60 });
    // S3: dense 4-note static chord (multi-voice path).
    scenario ([] (auto& a) { denseSpectrum (a); }, { 48, 55, 60, 64 });
    // S4: dense, Square Frame B, static morph at 0.3, bit-depth 8 (read-time path).
    scenario ([] (auto& a) { denseSpectrum (a);
                             setParam (a, frameBSource, 2.0f);   // Square
                             setParam (a, scanPosition, 0.3f);
                             setParam (a, bitDepth, 3.0f); },     // "8"
              { 52 });
    // S5: MOVING at a large host block (block 512 >= refill-cap interval) — the
    //     refill-cadence cap is a NO-OP here, so this must stay bit-identical
    //     across the change: it proves the FILL MATH is untouched and only the
    //     small-block cadence is bounded. LFO morph + spectral decay, two voices.
    scenario ([] (auto& a) { denseSpectrum (a);
                             setParam (a, scanLfoRate,  3.0f);
                             setParam (a, scanLfoDepth, 0.9f);
                             setParam (a, spectralDecay, 0.5f);
                             setParam (a, velToDecay,   0.4f); }, { 50, 57 });
    // S6: decay-only motion at block 512 (no LFO) — same large-block no-op proof
    //     for the spectral-decay refill path in isolation.
    scenario ([] (auto& a) { denseSpectrum (a);
                             setParam (a, spectralDecay, 0.7f); }, { 55 });

    return all;
}

static bool writeFloats (const juce::String& path, const std::vector<float>& v)
{
    juce::File f (path);
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (os.failedToOpen()) return false;
    const juce::int64 n = (juce::int64) v.size();
    os.writeInt64 (n);
    os.write (v.data(), v.size() * sizeof (float));
    return true;
}

static bool readFloats (const juce::String& path, std::vector<float>& out)
{
    juce::File f (path);
    juce::FileInputStream is (f);
    if (is.failedToOpen()) return false;
    const juce::int64 n = is.readInt64();
    out.resize ((size_t) n);
    is.read (out.data(), (int) (n * (juce::int64) sizeof (float)));
    return true;
}

//==============================================================================
static int runCorrectness (double fs)
{
    OSimpleAdditiveAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, fs, 512);
    proc.prepareToPlay (fs, 512);
    auto& apvts = proc.getAPVTS();

    const int   note = 57;                                   // A3
    const double f0  = juce::MidiMessage::getMidiNoteInHertz (note);
    const int   aOff = (int) (0.5 * fs);
    const int   aLen = (int) (0.5 * fs);

    int failures = 0;
    auto check = [&] (const char* name, bool ok, const juce::String& detail)
    {
        std::printf ("  [%s] %-24s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    };

    std::printf ("O-simpleAdditive render-harness — fs=%.0f, note=%d (%.1f Hz)\n", fs, note, f0);

    // 1 & 2: makes sound + correct pitch (pure sine = H1 only).
    {
        resetDefaults (apvts);   // pure sine, no motion
        auto y = renderChord (proc, { note }, 1.5, fs, 512);
        const double r    = rms (y, aOff, aLen);
        const double a_f0 = binAmplitude (y, aOff, aLen, f0, fs);
        const double a_2f = binAmplitude (y, aOff, aLen, 2.0 * f0, fs);
        check ("makes-sound", r > 0.01 && allFinite (y), juce::String ("rms=") + juce::String (r, 4));
        check ("pitch=fundamental", a_f0 > a_2f * 6.0,
               juce::String ("a_f0=") + juce::String (a_f0, 4) + " a_2f0=" + juce::String (a_2f, 4));
    }

    // 3: morph moves the spectrum — scanning sine(A)->saw(B) grows H2 energy.
    {
        resetDefaults (apvts);
        setParam (apvts, OSimpleAdditive::ParamIDs::scanPosition, 0.0f);   // pure A = sine
        auto y0 = renderChord (proc, { note }, 1.5, fs, 512);
        const double h2_at0 = binAmplitude (y0, aOff, aLen, 2.0 * f0, fs);

        setParam (apvts, OSimpleAdditive::ParamIDs::scanPosition, 1.0f);   // pure B = saw
        auto y1 = renderChord (proc, { note }, 1.5, fs, 512);
        const double h2_at1 = binAmplitude (y1, aOff, aLen, 2.0 * f0, fs);

        check ("morph-grows-H2", h2_at1 > h2_at0 * 10.0 + 0.005 && allFinite (y1),
               juce::String ("H2 scan0=") + juce::String (h2_at0, 5)
                 + " scan1=" + juce::String (h2_at1, 5));
    }

    // 4: a static patch is deterministic — two FRESH processor instances given
    //    the identical patch + MIDI render bit-for-bit identical output. This is
    //    the property the refill-cadence improvement must preserve for static
    //    patches (decay 0, LFO depth 0 -> once-per-note refill); the golden
    //    dump/check modes use the same set to compare across the change.
    {
        const auto a = buildGoldenSet (fs);
        const auto b = buildGoldenSet (fs);
        bool identical = (a.size() == b.size());
        if (identical)
            for (size_t i = 0; i < a.size(); ++i)
                if (! juce::exactlyEqual (a[i], b[i])) { identical = false; break; }
        check ("static-deterministic", identical && allFinite (a),
               juce::String ("n=") + juce::String ((int) a.size()));
    }

    // 5: continuous motion stays finite (Morph-Pad LFO + spectral-decay, 16 voices).
    {
        resetDefaults (apvts);
        denseSpectrum (apvts);
        setParam (apvts, OSimpleAdditive::ParamIDs::scanLfoRate,  3.0f);
        setParam (apvts, OSimpleAdditive::ParamIDs::scanLfoDepth, 0.9f);
        setParam (apvts, OSimpleAdditive::ParamIDs::spectralDecay, 0.6f);
        setParam (apvts, OSimpleAdditive::ParamIDs::velToDecay,   0.4f);
        std::vector<int> chord;
        for (int i = 0; i < 16; ++i) chord.push_back (36 + i);
        auto y = renderChord (proc, chord, 2.0, fs, 64);     // small block = worst refill cadence
        double mx = 0.0; for (float s : y) mx = juce::jmax (mx, (double) std::abs (s));
        check ("motion-finite", allFinite (y) && mx < 4.0,
               juce::String ("peak=") + juce::String (mx, 4));
    }

    proc.releaseResources();
    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures;
}

//==============================================================================
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // message manager for plugin internals
    const double fs = 44100.0;

    bool profile = false;
    juce::String dumpPath, checkPath;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp (argv[i], "--profile") == 0) profile = true;
        else if (std::strcmp (argv[i], "--dump-golden") == 0 && i + 1 < argc) dumpPath = argv[++i];
        else if (std::strcmp (argv[i], "--check-golden") == 0 && i + 1 < argc) checkPath = argv[++i];
    }

    // Golden capture: write the static-patch battery to a file (baseline snapshot).
    if (dumpPath.isNotEmpty())
    {
        const auto g = buildGoldenSet (fs);
        const bool ok = writeFloats (dumpPath, g);
        std::printf ("dump-golden: %s (%d samples) -> %s\n",
                     ok ? "OK" : "FAILED", (int) g.size(), dumpPath.toRawUTF8());
        return ok ? 0 : 1;
    }

    // Golden check: re-render the battery and compare bit-exactly to the baseline.
    if (checkPath.isNotEmpty())
    {
        std::vector<float> ref;
        if (! readFloats (checkPath, ref)) { std::printf ("check-golden: cannot read %s\n", checkPath.toRawUTF8()); return 1; }
        const auto now = buildGoldenSet (fs);
        bool match = (now.size() == ref.size());
        double maxDiff = 0.0; int firstBad = -1;
        if (match)
            for (size_t i = 0; i < now.size(); ++i)
            {
                const double d = std::abs ((double) now[i] - (double) ref[i]);
                if (d > maxDiff) maxDiff = d;
                if (firstBad < 0 && ! juce::exactlyEqual (now[i], ref[i])) firstBad = (int) i;
            }
        const bool bitIdentical = match && firstBad < 0;
        std::printf ("check-golden: %s  (n=%d, maxAbsDiff=%.3e, firstMismatch=%d)\n",
                     bitIdentical ? "BIT-IDENTICAL" : "DIFFERS",
                     (int) now.size(), maxDiff, firstBad);
        return bitIdentical ? 0 : 1;
    }

    const int failures = runCorrectness (fs);
    if (profile)
        runProfile (fs);

    return failures == 0 ? 0 : 1;
}
