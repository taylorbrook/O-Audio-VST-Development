/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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

    O-simpleSubtractive render-harness — Stage 2 DSP correctness gate.

    Instantiates OSimpleSubtractiveAudioProcessor directly and verifies the
    subtractive engine without a DAW:

      1.  makes-sound        — default params, MIDI note → non-trivial RMS, finite.
      2.  pitch=fundamental  — Saw plays the fundamental; passes the open filter.
      3.  sine-vs-saw        — Sine has far less HF content than Saw (filter has
                               "nothing to remove" from a sine).
      4.  filter-modes       — LP keeps lows / cuts highs; HP the opposite; BP
                               peaks at cutoff; Notch dips at cutoff.
      5.  filter-slopes      — 24 dB rolls off steeper than 12 dB than 6 dB.
      6.  filter-env         — bipolar filterEnvAmount opens (+) and closes (−)
                               the cutoff; amp env is unaffected.
      7.  key-track          — keyTrack=100% raises the effective cutoff with pitch.
      8.  aa-highpitch       — Saw at C7: inter-harmonic alias energy ≪ harmonics.
      9.  self-osc           — max resonance → strong bounded tone at cutoff.
      10. self-osc-in-tune   — keyTrack=100%: self-osc pitch tracks the note (×2).
      11. curve-vs-measured  — closed-form magnitude matches the measured filter.
      12. voice-modes        — Poly = both notes; Mono = last-note priority.
      13. legato-vs-mono     — Legato suppresses the amp retrigger; Mono retriggers.
      14. glide              — Mono glide ramps pitch between notes; no NaN.

    Exit 0 iff all checks pass.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"

#include <vector>
#include <cmath>
#include <cstdio>

using namespace OSimpleSubtractive::ParamIDs;

//==============================================================================
static void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float real)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (real));
}

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

static double peakAbs (const std::vector<float>& x)
{
    double p = 0.0;
    for (float s : x) p = juce::jmax (p, (double) std::abs (s));
    return p;
}

static bool allFinite (const std::vector<float>& x)
{
    for (float s : x) if (! std::isfinite (s)) return false;
    return true;
}

// Argmax of binAmplitude over a log-scanned frequency range — the dominant pitch.
static double peakFreq (const std::vector<float>& x, int off, int len,
                        double fLo, double fHi, double fs)
{
    double best = fLo, bestA = -1.0;
    const int steps = 400;
    for (int i = 0; i <= steps; ++i)
    {
        const double f = fLo * std::pow (fHi / fLo, (double) i / (double) steps);
        const double a = binAmplitude (x, off, len, f, fs);
        if (a > bestA) { bestA = a; best = f; }
    }
    return best;
}

// Sum of |X(f)| over a set of frequencies.
static double bandEnergy (const std::vector<float>& x, int off, int len,
                          const std::vector<double>& freqs, double fs)
{
    double acc = 0.0;
    for (double f : freqs) if (f < 0.45 * fs) acc += binAmplitude (x, off, len, f, fs);
    return acc;
}

//==============================================================================
struct Event { double timeSec; bool noteOn; int note; float vel; };

// Render `seconds` of mono output driving the given MIDI events.
static std::vector<float> renderEvents (OSimpleSubtractiveAudioProcessor& proc,
                                        const std::vector<Event>& events,
                                        double seconds, double fs)
{
    const int block = 512;
    const int total = (int) (seconds * fs);

    juce::AudioBuffer<float> buf (2, block);
    std::vector<float> out;
    out.reserve ((size_t) total);

    size_t ev = 0;
    int pos = 0;
    while (pos < total)
    {
        buf.clear();
        juce::MidiBuffer midi;
        while (ev < events.size() && (int) (events[ev].timeSec * fs) < pos + block)
        {
            const int sp = juce::jmax (0, (int) (events[ev].timeSec * fs) - pos);
            const auto& e = events[ev];
            midi.addEvent (e.noteOn ? juce::MidiMessage::noteOn  (1, e.note, (juce::uint8) juce::jlimit (1, 127, (int) (e.vel * 127.0f)))
                                    : juce::MidiMessage::noteOff (1, e.note), sp);
            ++ev;
        }

        proc.processBlock (buf, midi);

        const int n = juce::jmin (block, total - pos);
        for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
        pos += block;
    }
    return out;
}

// Hold one note from t=0, release at `releaseFrac` of the duration.
static std::vector<float> renderHold (OSimpleSubtractiveAudioProcessor& proc,
                                      int note, double seconds, double fs, double releaseFrac = 0.85)
{
    return renderEvents (proc, { { 0.0, true, note, 0.8f },
                                 { seconds * releaseFrac, false, note, 0.0f } }, seconds, fs);
}

// Reset all 20 params to factory defaults so each scenario starts clean.
static void resetDefaults (juce::AudioProcessorValueTreeState& a)
{
    setParam (a, oscWave, 0.0f);          // Saw
    setParam (a, subLevel, 0.0f);         setParam (a, noiseLevel, 0.0f);
    setParam (a, filterType, 0.0f);       // LP
    setParam (a, filterSlope, 2.0f);      // 24 dB
    setParam (a, cutoff, 2000.0f);        setParam (a, resonance, 0.10f);
    setParam (a, filterEnvAmount, 0.5f);  setParam (a, keyTrack, 0.0f);
    setParam (a, filterAttack, 0.005f);   setParam (a, filterDecay, 0.30f);
    setParam (a, filterSustain, 0.40f);   setParam (a, filterRelease, 0.20f);
    setParam (a, ampAttack, 0.005f);      setParam (a, ampDecay, 0.30f);
    setParam (a, ampSustain, 0.80f);      setParam (a, ampRelease, 0.10f);
    setParam (a, voiceMode, 0.0f);        // Poly
    setParam (a, glide, 0.0f);            setParam (a, outputLevel, 0.0f);
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const double fs = 44100.0;
    OSimpleSubtractiveAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, fs, 512);
    proc.prepareToPlay (fs, 512);
    auto& apvts = proc.getAPVTS();

    const int    note = 69;                                       // A4
    const double f0   = juce::MidiMessage::getMidiNoteInHertz (note);  // 440
    const int    aOff = (int) (0.5 * fs);                         // steady-state window [0.5s, 1.0s]
    const int    aLen = (int) (0.5 * fs);

    int failures = 0;
    auto check = [&] (const char* name, bool ok, const juce::String& detail)
    {
        std::printf ("  [%s] %-20s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    };

    std::printf ("O-simpleSubtractive render-harness — fs=%.0f, note=%d (%.1f Hz)\n", fs, note, f0);

    // --- 1 & 2: makes sound + plays the fundamental (Saw, open filter) --------
    {
        resetDefaults (apvts);
        setParam (apvts, cutoff, 8000.0f);   // open so the source passes
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, ampSustain, 1.0f);
        auto y = renderHold (proc, note, 1.5, fs);

        const double r    = rms (y, aOff, aLen);
        const double a440 = binAmplitude (y, aOff, aLen, f0, fs);
        const double a220 = binAmplitude (y, aOff, aLen, f0 * 0.5, fs);

        check ("makes-sound", r > 0.02 && allFinite (y), juce::String ("rms=") + juce::String (r, 4));
        check ("pitch=fundamental", a440 > a220 * 4.0 && a440 > 0.01,
               juce::String ("a440=") + juce::String (a440, 4) + " a220=" + juce::String (a220, 4));
    }

    // --- 3: sine has far less HF than saw (filter has nothing to remove) -------
    {
        const std::vector<double> harm { f0 * 2, f0 * 3, f0 * 4, f0 * 5, f0 * 6 };
        resetDefaults (apvts);
        setParam (apvts, cutoff, 12000.0f);
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, ampSustain, 1.0f);

        setParam (apvts, oscWave, 0.0f);  // Saw
        auto ySaw = renderHold (proc, note, 1.5, fs);
        const double sawHF = bandEnergy (ySaw, aOff, aLen, harm, fs);

        setParam (apvts, oscWave, 3.0f);  // Sine
        auto ySin = renderHold (proc, note, 1.5, fs);
        const double sinHF = bandEnergy (ySin, aOff, aLen, harm, fs);

        check ("sine-vs-saw", sawHF > sinHF * 8.0 && allFinite (ySin),
               juce::String ("sawHF=") + juce::String (sawHF, 4) + " sinHF=" + juce::String (sinHF, 5));
    }

    // --- 4: filter modes — LP vs HP low/high balance; BP & Notch at cutoff -----
    {
        const double fc = 600.0;
        const std::vector<double> lows  { f0 };                       // 440 (below fc)
        const std::vector<double> highs { f0 * 3, f0 * 4, f0 * 5 };   // 1320+ (above fc)

        resetDefaults (apvts);
        setParam (apvts, filterSlope, 1.0f);   // 12 dB
        setParam (apvts, cutoff, (float) fc);
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, ampSustain, 1.0f);

        setParam (apvts, filterType, 0.0f);    // LP
        auto yLP = renderHold (proc, note, 1.5, fs);
        const double lpLow = bandEnergy (yLP, aOff, aLen, lows, fs);
        const double lpHigh = bandEnergy (yLP, aOff, aLen, highs, fs);

        setParam (apvts, filterType, 1.0f);    // HP
        auto yHP = renderHold (proc, note, 1.5, fs);
        const double hpLow = bandEnergy (yHP, aOff, aLen, lows, fs);
        const double hpHigh = bandEnergy (yHP, aOff, aLen, highs, fs);

        check ("mode-LP", lpLow > lpHigh * 2.0 && allFinite (yLP),
               juce::String ("low=") + juce::String (lpLow, 4) + " high=" + juce::String (lpHigh, 4));
        check ("mode-HP", hpHigh > lpHigh && hpLow < lpLow && allFinite (yHP),
               juce::String ("hpLow=") + juce::String (hpLow, 4) + " hpHigh=" + juce::String (hpHigh, 4));

        // BP peaks AT cutoff and attenuates both sides; Notch dips at cutoff.
        // Steep 24 dB skirts + real resonance so the 3rd harmonic at cutoff beats
        // the (3× stronger) fundamental below and a higher harmonic above.
        setParam (apvts, filterSlope, 2.0f);   // 24 dB
        setParam (apvts, resonance, 0.6f);
        setParam (apvts, filterType, 2.0f);    // BP
        setParam (apvts, cutoff, (float) (f0 * 3.0)); // 1320, on the 3rd harmonic
        auto yBP = renderHold (proc, note, 1.5, fs);
        const double bpAtFc = binAmplitude (yBP, aOff, aLen, f0 * 3.0, fs);
        const double bpLow  = binAmplitude (yBP, aOff, aLen, f0,       fs);  // 440 (below)
        const double bpHigh = binAmplitude (yBP, aOff, aLen, f0 * 6.0, fs);  // 2640 (above)
        check ("mode-BP", bpAtFc > bpLow * 1.5 && bpAtFc > bpHigh * 1.5 && allFinite (yBP),
               juce::String ("atFc=") + juce::String (bpAtFc, 4) + " low=" + juce::String (bpLow, 4)
                 + " high=" + juce::String (bpHigh, 4));

        setParam (apvts, filterType, 3.0f);    // Notch (cutoff still 1320)
        auto yNo = renderHold (proc, note, 1.5, fs);
        const double noAtFc = binAmplitude (yNo, aOff, aLen, f0 * 3.0, fs);
        const double noFar  = binAmplitude (yNo, aOff, aLen, f0, fs);
        check ("mode-Notch", noAtFc < noFar * 0.6 && allFinite (yNo),
               juce::String ("atFc=") + juce::String (noAtFc, 4) + " far=" + juce::String (noFar, 4));
    }

    // --- 5: slopes — stopband attenuation 24 < 12 < 6 (an octave above cutoff) -
    {
        const double fc   = 500.0;
        const double fStop = f0 * 4.0;   // 1760 Hz, ~1.8 oct above fc
        resetDefaults (apvts);
        setParam (apvts, filterType, 0.0f);    // LP
        setParam (apvts, cutoff, (float) fc);
        setParam (apvts, resonance, 0.0f);
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, ampSustain, 1.0f);

        setParam (apvts, filterSlope, 0.0f);   // 6 dB
        auto y6 = renderHold (proc, note, 1.5, fs);
        const double s6 = binAmplitude (y6, aOff, aLen, fStop, fs);

        setParam (apvts, filterSlope, 1.0f);   // 12 dB
        auto y12 = renderHold (proc, note, 1.5, fs);
        const double s12 = binAmplitude (y12, aOff, aLen, fStop, fs);

        setParam (apvts, filterSlope, 2.0f);   // 24 dB
        auto y24 = renderHold (proc, note, 1.5, fs);
        const double s24 = binAmplitude (y24, aOff, aLen, fStop, fs);

        check ("slopes-steepen", s6 > s12 * 1.2 && s12 > s24 * 1.2 && allFinite (y24),
               juce::String ("s6=") + juce::String (s6, 5) + " s12=" + juce::String (s12, 5)
                 + " s24=" + juce::String (s24, 5));
    }

    // --- 6: filter env bipolar — +amount brighter than −amount; amp unaffected --
    {
        const std::vector<double> highs { f0 * 4, f0 * 6, f0 * 8 };
        resetDefaults (apvts);
        setParam (apvts, cutoff, 500.0f);
        setParam (apvts, filterDecay, 1.0f);  setParam (apvts, filterSustain, 1.0f);
        setParam (apvts, ampSustain, 1.0f);

        setParam (apvts, filterEnvAmount, 0.8f);    // open up
        auto yPos = renderHold (proc, note, 1.5, fs);
        const double hiPos = bandEnergy (yPos, aOff, aLen, highs, fs);
        const double rPos  = rms (yPos, aOff, aLen);

        setParam (apvts, filterEnvAmount, -0.8f);   // close down
        auto yNeg = renderHold (proc, note, 1.5, fs);
        const double hiNeg = bandEnergy (yNeg, aOff, aLen, highs, fs);

        check ("filter-env-bipolar", hiPos > hiNeg * 3.0 && allFinite (yPos) && rPos > 0.01,
               juce::String ("hiPos=") + juce::String (hiPos, 4) + " hiNeg=" + juce::String (hiNeg, 5));
    }

    // --- 7: key-track raises the effective cutoff with pitch -------------------
    {
        resetDefaults (apvts);
        setParam (apvts, cutoff, 500.0f);
        setParam (apvts, filterEnvAmount, 0.0f);     // isolate keyTrack
        setParam (apvts, keyTrack, 1.0f);
        setParam (apvts, ampSustain, 1.0f);

        (void) renderHold (proc, 36, 1.0, fs);       // C2
        const double cLow = proc.getDisplayCutoffHz();
        (void) renderHold (proc, 84, 1.0, fs);       // C6 (4 octaves up)
        const double cHigh = proc.getDisplayCutoffHz();

        check ("key-track", cHigh > cLow * 6.0,
               juce::String ("cutoffC2=") + juce::String (cLow, 1) + " cutoffC6=" + juce::String (cHigh, 1));
    }

    // --- 8: aliasing — Saw at C7, inter-harmonic alias ≪ harmonic energy -------
    {
        const int    hiNote = 96;                                          // C7
        const double hf0    = juce::MidiMessage::getMidiNoteInHertz (hiNote); // ~2093
        resetDefaults (apvts);
        setParam (apvts, oscWave, 0.0f);     // Saw (worst case)
        setParam (apvts, cutoff, 18000.0f);  // wide open — expose any aliasing
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, resonance, 0.0f);
        setParam (apvts, ampSustain, 1.0f);
        auto y = renderHold (proc, hiNote, 1.5, fs);

        double harm = 0.0;
        for (int k = 1; k <= 6 && k * hf0 < 0.45 * fs; ++k)
            harm += binAmplitude (y, aOff, aLen, k * hf0, fs);
        double alias = 0.0;
        for (double m : { 0.5, 1.37, 2.4, 3.3, 5.7 })
            if (m * hf0 < 0.45 * fs)
                alias += binAmplitude (y, aOff, aLen, m * hf0, fs);

        check ("aa-highpitch", allFinite (y) && peakAbs (y) < 4.0 && alias < harm * 0.25 + 1.0e-4,
               juce::String ("harm=") + juce::String (harm, 4) + " alias=" + juce::String (alias, 5)
                 + " ratio=" + juce::String (harm > 0 ? alias / harm : 0.0, 4));
    }

    // --- 9: self-oscillation — max resonance → strong bounded tone at cutoff ----
    {
        const double fc = 800.0;
        resetDefaults (apvts);
        setParam (apvts, oscWave, 3.0f);     // Sine far from cutoff (clean reference)
        setParam (apvts, filterType, 0.0f);  // LP
        setParam (apvts, filterSlope, 1.0f); // 12 dB
        setParam (apvts, cutoff, (float) fc);
        setParam (apvts, resonance, 1.0f);   // max → self-osc
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, ampSustain, 1.0f);
        auto y = renderHold (proc, 45, 2.0, fs);   // low note (110 Hz), well below fc

        // Energy near the cutoff can ONLY come from self-oscillation — the source
        // is a 110 Hz sine. The limit cycle sits a few % below fc (negative-k
        // shift), so find the peak in a band around fc (excluding the 110 input).
        const double pf      = peakFreq (y, aOff, aLen, fc * 0.7, fc * 1.2, fs);
        const double atPeak  = binAmplitude (y, aOff, aLen, pf, fs);
        const double lateRms = rms (y, (int) (1.5 * fs), (int) (0.4 * fs));
        check ("self-osc", allFinite (y) && peakAbs (y) < 4.0 && atPeak > 0.1 && lateRms > 0.05
                 && pf > fc * 0.8,
               juce::String ("selfOscHz=") + juce::String (pf, 1) + " amp=" + juce::String (atPeak, 4)
                 + " lateRMS=" + juce::String (lateRms, 4) + " peak=" + juce::String (peakAbs (y), 3));
    }

    // --- 10: self-osc in tune — keyTrack=100% → self-osc pitch tracks note (×2) -
    // Cutoff base (600 Hz) sits ABOVE the played note so the self-osc tone is at a
    // different frequency than the input sine — the measured peak is the self-osc,
    // not the input passing through.
    {
        const double cutBase = 600.0;
        resetDefaults (apvts);
        setParam (apvts, oscWave, 3.0f);
        setParam (apvts, filterType, 0.0f);
        setParam (apvts, filterSlope, 1.0f);
        setParam (apvts, cutoff, (float) cutBase);   // note 60 → cutoff 600; note 72 → 1200
        setParam (apvts, resonance, 1.0f);
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, keyTrack, 1.0f);
        setParam (apvts, ampSustain, 1.0f);

        const double expLo = cutBase;          // note 60
        const double expHi = cutBase * 2.0;    // note 72 (one octave up)
        auto yLo = renderHold (proc, 60, 2.0, fs);
        const double pLo = peakFreq (yLo, aOff, aLen, expLo * 0.8, expLo * 1.25, fs);
        auto yHi = renderHold (proc, 72, 2.0, fs);
        const double pHi = peakFreq (yHi, aOff, aLen, expHi * 0.8, expHi * 1.25, fs);

        const double ratio = pLo > 0 ? pHi / pLo : 0.0;
        const bool inTune = std::abs (pLo - expLo) / expLo < 0.08
                         && std::abs (pHi - expHi) / expHi < 0.08
                         && ratio > 1.85 && ratio < 2.15;
        check ("self-osc-in-tune", allFinite (yHi) && inTune,
               juce::String ("fLo=") + juce::String (pLo, 1) + " (exp 600) fHi=" + juce::String (pHi, 1)
                 + " (exp 1200) ratio=" + juce::String (ratio, 3));
    }

    // --- 11: closed-form magnitude curve matches the measured filter (LP 12) ---
    {
        const double fc = 1000.0;
        resetDefaults (apvts);
        setParam (apvts, oscWave, 3.0f);     // Sine probe tone
        setParam (apvts, filterType, 0.0f);  // LP
        setParam (apvts, filterSlope, 1.0f); // 12 dB
        setParam (apvts, cutoff, (float) fc);
        setParam (apvts, resonance, 0.10f);  // mild → linear (closed-form valid)
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, keyTrack, 0.0f);
        setParam (apvts, ampSustain, 1.0f);

        // k the audio thread uses for resonance=0.10 (mirror SubVoice::resonanceToK).
        const double k = juce::jmax (0.0, 2.0 * std::pow (1.0 - 0.10, 0.6));

        // Probe at notes spanning the cutoff; normalise both curves to the lowest.
        const int probes[] = { 45, 57, 69, 81, 93 };  // ~110, 220, 440, 880, 1760 Hz
        double measDb[5], predDb[5];
        for (int i = 0; i < 5; ++i)
        {
            const double pf = juce::MidiMessage::getMidiNoteInHertz (probes[i]);
            auto y = renderHold (proc, probes[i], 1.5, fs);
            const double amp = binAmplitude (y, aOff, aLen, pf, fs);
            measDb[i] = juce::Decibels::gainToDecibels (amp + 1.0e-9, -120.0);
            predDb[i] = SubFilterCurve::magnitudeDb (pf, fc, k, 0, 1, fs);
        }
        double maxErr = 0.0;
        juce::String row;
        for (int i = 0; i < 5; ++i)
        {
            const double m = measDb[i] - measDb[0];   // relative to lowest probe (≈ passband)
            const double p = predDb[i] - predDb[0];
            maxErr = juce::jmax (maxErr, std::abs (m - p));
            row += juce::String (m - p, 1) + " ";
        }
        check ("curve-vs-measured", maxErr < 4.0,
               juce::String ("maxErrDb=") + juce::String (maxErr, 2) + " (Δ: " + row + ")");
    }

    // --- 12: voice modes — Poly = both notes; Mono = last-note priority --------
    {
        const double fA = juce::MidiMessage::getMidiNoteInHertz (60);   // C4 261.6
        const double fB = juce::MidiMessage::getMidiNoteInHertz (64);   // E4 329.6

        resetDefaults (apvts);
        setParam (apvts, cutoff, 6000.0f);
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, ampSustain, 1.0f);

        // Poly: both notes held → both present.
        setParam (apvts, voiceMode, 0.0f);
        auto yPoly = renderEvents (proc, { {0.0,true,60,0.8f}, {0.0,true,64,0.8f},
                                           {1.3,false,60,0}, {1.3,false,64,0} }, 1.4, fs);
        const double pa = binAmplitude (yPoly, aOff, aLen, fA, fs);
        const double pb = binAmplitude (yPoly, aOff, aLen, fB, fs);
        check ("poly-both", pa > 0.01 && pb > 0.01 && allFinite (yPoly),
               juce::String ("A=") + juce::String (pa, 4) + " B=" + juce::String (pb, 4));

        // Mono: both held, last-note (E4) priority → A4 weak.
        setParam (apvts, voiceMode, 1.0f);
        auto yMono = renderEvents (proc, { {0.0,true,60,0.8f}, {0.05,true,64,0.8f},
                                           {1.3,false,64,0}, {1.3,false,60,0} }, 1.4, fs);
        const double ma = binAmplitude (yMono, aOff, aLen, fA, fs);
        const double mb = binAmplitude (yMono, aOff, aLen, fB, fs);
        check ("mono-last-note", mb > ma * 4.0 && allFinite (yMono),
               juce::String ("A=") + juce::String (ma, 4) + " B=" + juce::String (mb, 4));
    }

    // --- 13: legato suppresses the amp retrigger; mono retriggers --------------
    {
        // Slow amp attack; play note1, let it reach sustain, then note2 while held.
        // Measure RMS in a short window right after note2: legato stays loud
        // (no re-attack), mono drops (re-attacks from 0).
        auto seq = std::vector<Event> { {0.0,true,60,0.8f}, {0.8,true,64,0.8f}, {1.4,false,64,0}, {1.4,false,60,0} };
        const int w2Off = (int) (0.82 * fs);   // just after the 2nd note-on
        const int w2Len = (int) (0.04 * fs);

        resetDefaults (apvts);
        setParam (apvts, cutoff, 6000.0f);
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, ampAttack, 0.25f);    // slow attack exposes a retrigger
        setParam (apvts, ampSustain, 1.0f);

        setParam (apvts, voiceMode, 1.0f);     // Mono (retrigger)
        auto yMono = renderEvents (proc, seq, 1.6, fs);
        const double monoR = rms (yMono, w2Off, w2Len);

        setParam (apvts, voiceMode, 2.0f);     // Legato (no retrigger)
        auto yLeg = renderEvents (proc, seq, 1.6, fs);
        const double legR = rms (yLeg, w2Off, w2Len);

        check ("legato-vs-mono", legR > monoR * 1.8 && allFinite (yLeg) && allFinite (yMono),
               juce::String ("legatoRMS=") + juce::String (legR, 4) + " monoRMS=" + juce::String (monoR, 4));
    }

    // --- 14: glide ramps pitch between mono notes ------------------------------
    {
        resetDefaults (apvts);
        setParam (apvts, cutoff, 6000.0f);
        setParam (apvts, filterEnvAmount, 0.0f);
        setParam (apvts, ampSustain, 1.0f);
        setParam (apvts, voiceMode, 1.0f);     // Mono
        setParam (apvts, glide, 0.5f);         // 0.5 s portamento

        // A4 (440) then A5 (880) 0.1 s later; measure pitch early in the glide.
        auto y = renderEvents (proc, { {0.0,true,69,0.8f}, {0.1,true,81,0.8f}, {1.4,false,81,0} }, 1.5, fs);
        const int gOff = (int) (0.16 * fs);    // ~60 ms into the 0.5 s glide
        const int gLen = (int) (0.03 * fs);
        const double pf = peakFreq (y, gOff, gLen, 400.0, 950.0, fs);

        check ("glide", allFinite (y) && pf > 470.0 && pf < 850.0,
               juce::String ("midGlideHz=") + juce::String (pf, 1) + " (between 440 and 880)");
    }

    // --- 15: no click on note-off (v1.2.5 regression probe) --------------------
    // Amp sustain 0 + slow decay, note-off while the tail still rings. The
    // per-block ADSR param push used to zero the release rate (recalculateRates
    // derives it from sustain), hard-resetting the envelope one block after
    // note-off: tail truncated to silence = click (O-simpleFM v1.2.5 pattern).
    // Static low cutoff (filterEnvAmount 0) keeps the waveform near-sine; the
    // tail-vs-pre RMS ratio is the discriminator.
    {
        resetDefaults (apvts);
        setParam (apvts, cutoff, 300.0f);          // near-sine output on A3
        setParam (apvts, filterEnvAmount, 0.0f);   // static cutoff
        setParam (apvts, ampDecay, 3.0f);          // slow decay
        setParam (apvts, ampSustain, 0.0f);        // the killer setting
        setParam (apvts, ampRelease, 0.3f);
        auto y = renderHold (proc, 57, 1.0, fs, 0.85);   // A3 (220 Hz), note-off at 0.85 s

        const double preRms  = rms (y, (int) (0.70 * fs), (int) (0.14 * fs));
        const double tailRms = rms (y, (int) (0.90 * fs), (int) (0.08 * fs));

        check ("noteoff-click",
               preRms > 0.01 && tailRms > 0.2 * preRms && allFinite (y),
               juce::String ("preRms=") + juce::String (preRms, 4)
                 + " tailRms=" + juce::String (tailRms, 4));
    }

    proc.releaseResources();

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
