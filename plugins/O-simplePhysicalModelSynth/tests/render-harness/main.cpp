/*
   This file is part of O-simplePhysicalModelSynth, an Ouaricon Audio plugin.
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

    O-simplePhysicalModelSynth render-harness — Stage-2 DSP correctness gate.

    Offline console app. Instantiates OSimplePhysicalModelSynthAudioProcessor
    directly (no DAW), drives MIDI notes, and asserts DSP acceptance criteria.

    Compiled under JUCE_WEB_BROWSER=0 with NO PluginEditor.cpp and NO *_UIResources
    target (the Stage-1 seam) — do NOT regress the CMake toward a WebView build.

    Pitch is probed by normalized AUTOCORRELATION (not a DFT — the KS loop comb
    fools spectral probes; O-simpleGrain lesson) with PARABOLIC sub-sample peak
    interpolation (integer-lag autocorr can't resolve ±5 cents at the top octave,
    where one period is only ~21 samples ≈ ±57 cents/lag).

    Phase gates (re-run at the end of each phase before the next begins):
      2.1  makes-sound, finite, tuning ±5¢ at C1/C3/C5/C7.
      2.2  exciter character + Bow bounded at max Force + max Decay, no DC.   (added 2.2)
      2.3  modal cross-driving + inharmonicity.                              (added 2.3)

    Exit 0 iff all checks pass.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"
#include "ModalResonator.h"

#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <functional>

//==============================================================================
// Acceptance-test helpers
//==============================================================================

static bool allFinite (const std::vector<float>& x)
{
    for (float v : x) if (! std::isfinite (v)) return false;
    return true;
}

static double rms (const std::vector<float>& x, int off, int len)
{
    if (len <= 0) return 0.0;
    double s = 0.0;
    for (int i = 0; i < len; ++i) { const double v = x[(size_t) (off + i)]; s += v * v; }
    return std::sqrt (s / (double) len);
}

static double peakAbs (const std::vector<float>& x)
{
    double p = 0.0;
    for (float v : x) p = juce::jmax (p, (double) std::abs (v));
    return p;
}

static double meanVal (const std::vector<float>& x, int off, int len)
{
    if (len <= 0) return 0.0;
    double s = 0.0;
    for (int i = 0; i < len; ++i) s += x[(size_t) (off + i)];
    return s / (double) len;
}

// Fundamental via normalized autocorrelation over [off,off+len), searched in
// [fLo,fHi] Hz, refined to sub-sample precision by parabolic interpolation of the
// correlation peak. Robust against the KS comb (tracks the waveform's repetition
// period regardless of which partial is loudest). Returns 0 if no lag clears minCorr.
static double autocorrPitchHz (const std::vector<float>& x, int off, int len,
                               double fs, double fLo, double fHi, double minCorr = 0.3)
{
    const int lagMin = juce::jmax (2, (int) (fs / fHi));
    const int lagMax = juce::jmin ((int) (fs / fLo), len - 2);
    if (lagMax <= lagMin + 1) return 0.0;

    double e0 = 0.0;
    for (int n = 0; n < len; ++n) { const double s = x[(size_t) (off + n)]; e0 += s * s; }
    if (e0 < 1.0e-9) return 0.0;

    std::vector<double> corr ((size_t) (lagMax + 2), 0.0);
    double bestCorr = 0.0; int bestLag = -1;
    for (int lag = lagMin; lag <= lagMax; ++lag)
    {
        double acc = 0.0, eL = 0.0;
        for (int n = 0; n + lag < len; ++n)
        {
            const double a = x[(size_t) (off + n)];
            const double b = x[(size_t) (off + n + lag)];
            acc += a * b;
            eL  += b * b;
        }
        const double c = acc / (std::sqrt (e0 * eL) + 1.0e-12);
        corr[(size_t) lag] = c;
        if (c > bestCorr) { bestCorr = c; bestLag = lag; }
    }
    if (bestLag < 0 || bestCorr < minCorr) return 0.0;

    double refined = (double) bestLag;
    if (bestLag > lagMin && bestLag < lagMax)
    {
        const double cm = corr[(size_t) (bestLag - 1)];
        const double c0 = corr[(size_t) bestLag];
        const double cp = corr[(size_t) (bestLag + 1)];
        const double denom = cm - 2.0 * c0 + cp;
        if (std::abs (denom) > 1.0e-12)
        {
            const double delta = 0.5 * (cm - cp) / denom;
            if (std::abs (delta) < 1.0) refined = (double) bestLag + delta;
        }
    }
    return fs / refined;
}

static float midiToHz (int note) { return 440.0f * std::pow (2.0f, (note - 69) / 12.0f); }

//==============================================================================
// A fresh, isolated render of one held note. Constructs its own processor so no
// state bleeds between cases; `setup` sets parameters before the note is struck.
//==============================================================================
struct Render
{
    std::vector<float> mono;
    double fs;
};

static Render renderNote (int note, int vel, double fs, int block, int numSamples,
                          const std::function<void (juce::AudioProcessorValueTreeState&)>& setup,
                          int noteOffAt = -1)
{
    OSimplePhysicalModelSynthAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, fs, block);
    proc.prepareToPlay (fs, block);
    if (setup) setup (proc.getAPVTS());

    Render r; r.fs = fs; r.mono.reserve ((size_t) numSamples);
    juce::AudioBuffer<float> buf (2, block);

    int rendered = 0;
    bool noteSent = false, offSent = false;
    while (rendered < numSamples)
    {
        buf.clear();
        juce::MidiBuffer midi;
        if (! noteSent) { midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) vel), 0); noteSent = true; }
        if (! offSent && noteOffAt >= 0 && rendered >= noteOffAt)
        { midi.addEvent (juce::MidiMessage::noteOff (1, note), 0); offSent = true; }

        proc.processBlock (buf, midi);

        const int n = juce::jmin (block, numSamples - rendered);
        for (int i = 0; i < n; ++i)
        {
            const float l = buf.getSample (0, i);
            const float rr = buf.getNumChannels() > 1 ? buf.getSample (1, i) : l;
            r.mono.push_back (0.5f * (l + rr));
        }
        rendered += n;
    }
    proc.releaseResources();
    return r;
}

static void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float v)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;  // message manager for plugin internals

    const double fs    = 44100.0;
    const int    block = 512;

    int failures = 0;
    auto check = [&] (const char* name, bool ok, const juce::String& detail)
    {
        std::printf ("  [%s] %-26s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
        if (! ok) ++failures;
    };

    std::printf ("O-simplePhysicalModelSynth render-harness (Stage 2.3) — fs=%.0f\n", fs);

    //--------------------------------------------------------------------------
    // 1. makes-sound + finite — default params, Pluck → String, A4.
    //--------------------------------------------------------------------------
    {
        auto r = renderNote (69, 100, fs, block, (int) (0.4 * fs), nullptr);
        const int off = (int) (0.012 * fs);                // just past the 10 ms burst
        const int len = (int) (0.06 * fs);
        const double e = rms (r.mono, off, len);
        check ("makes-sound", e > 0.02, juce::String ("rms=") + juce::String (e, 5));
        check ("output-finite", allFinite (r.mono), "no NaN/Inf");
        check ("no-blowup", peakAbs (r.mono) < 4.0, juce::String ("peak=") + juce::String (peakAbs (r.mono), 4));
    }

    //--------------------------------------------------------------------------
    // 2. Tuning ±5 cents at C1/C3/C5/C7 — controlled: low damping (fundamentals
    //    pass the loop LPF) + high decay (all notes ring long enough to probe).
    //--------------------------------------------------------------------------
    {
        auto tuningSetup = [] (juce::AudioProcessorValueTreeState& a)
        {
            setParam (a, ParamIDs::excitationType, 0.0f);   // Pluck
            setParam (a, ParamIDs::resonatorType,  0.0f);   // String
            setParam (a, ParamIDs::damping, 5.0f);          // cutoff ≈ 10.8 kHz — all fundamentals pass
            setParam (a, ParamIDs::decay,  92.0f);          // long ring
            setParam (a, ParamIDs::excitationColor, 80.0f);
            setParam (a, ParamIDs::excitationPosition, 25.0f);
        };

        const int notes[] = { 24, 48, 72, 96 };             // C1, C3, C5, C7
        const char* names[] = { "tuning-C1", "tuning-C3", "tuning-C5", "tuning-C7" };

        for (int k = 0; k < 4; ++k)
        {
            const int   note   = notes[k];
            const float f0     = midiToHz (note);
            const double period = fs / f0;

            auto r = renderNote (note, 110, fs, block, (int) (0.5 * fs), tuningSetup);

            const int off = (int) (0.035 * fs);             // after the 10 ms burst + settle
            const int len = juce::jlimit (2048, 16384, (int) (8.0 * period));
            const double meas  = autocorrPitchHz (r.mono, off, len, fs, f0 * 0.8, f0 * 1.25);
            const double cents = (meas > 0.0) ? 1200.0 * std::log2 (meas / f0) : 9999.0;

            check (names[k], meas > 0.0 && std::abs (cents) <= 5.0,
                   juce::String (f0, 1) + " Hz → " + juce::String (meas, 2) + " Hz, "
                   + juce::String (cents, 2) + " cents");
        }
    }

    //--------------------------------------------------------------------------
    // 2.2  Exciter character + stability (Strike, Bow, Material/velocity, no-DC).
    //--------------------------------------------------------------------------
    {
        // Bow → String, held throughout (no note-off) — must SUSTAIN, not decay.
        auto bowSetup = [] (juce::AudioProcessorValueTreeState& a)
        {
            setParam (a, ParamIDs::excitationType, 2.0f);   // Bow
            setParam (a, ParamIDs::resonatorType,  0.0f);   // String
            setParam (a, ParamIDs::damping, 30.0f);
            setParam (a, ParamIDs::decay,   60.0f);
            setParam (a, ParamIDs::bowForce, 60.0f);
        };
        auto rb = renderNote (57, 100, fs, block, (int) (0.5 * fs), bowSetup);   // A3
        const int win = (int) (0.05 * fs);
        const double eEarly = rms (rb.mono, (int) (0.06 * fs), win);
        const double eLate  = rms (rb.mono, (int) (0.40 * fs), win);
        check ("bow-makes-sound", eLate > 0.01, juce::String ("rmsLate=") + juce::String (eLate, 5));
        check ("bow-sustains", eEarly > 0.005 && eLate > 0.5 * eEarly,
               juce::String ("early=") + juce::String (eEarly, 4) + " late=" + juce::String (eLate, 4));

        // Bow at MAX Force + MAX Decay + min damping — must stay finite + bounded (QUAL-01).
        auto bowMax = [] (juce::AudioProcessorValueTreeState& a)
        {
            setParam (a, ParamIDs::excitationType, 2.0f);
            setParam (a, ParamIDs::resonatorType,  0.0f);
            setParam (a, ParamIDs::damping, 0.0f);
            setParam (a, ParamIDs::decay,  100.0f);
            setParam (a, ParamIDs::bowForce, 100.0f);
        };
        auto rbm = renderNote (57, 127, fs, block, (int) (0.6 * fs), bowMax);
        check ("bow-bounded-max", allFinite (rbm.mono) && peakAbs (rbm.mono) < 4.0,
               juce::String ("peak=") + juce::String (peakAbs (rbm.mono), 4));

        // decay-shortens-with-Decay: low Decay rings less late than high Decay (Pluck).
        auto pluckDecay = [] (float d)
        {
            return [d] (juce::AudioProcessorValueTreeState& a)
            {
                setParam (a, ParamIDs::excitationType, 0.0f);
                setParam (a, ParamIDs::resonatorType,  0.0f);
                setParam (a, ParamIDs::damping, 30.0f);
                setParam (a, ParamIDs::decay, d);
            };
        };
        auto rShort = renderNote (60, 100, fs, block, (int) (0.5 * fs), pluckDecay (15.0f));
        auto rLong  = renderNote (60, 100, fs, block, (int) (0.5 * fs), pluckDecay (95.0f));
        const double eShort = rms (rShort.mono, (int) (0.30 * fs), win);
        const double eLong  = rms (rLong.mono,  (int) (0.30 * fs), win);
        check ("decay-shortens", eShort < eLong,
               juce::String ("short=") + juce::String (eShort, 5) + " long=" + juce::String (eLong, 5));

        // no-DC post-DC-blocker (Strike — the all-positive raised-cosine is the worst case).
        auto strikeSetup = [] (juce::AudioProcessorValueTreeState& a)
        {
            setParam (a, ParamIDs::excitationType, 1.0f);   // Strike
            setParam (a, ParamIDs::resonatorType,  0.0f);
            setParam (a, ParamIDs::damping, 40.0f);
            setParam (a, ParamIDs::decay,   60.0f);
        };
        auto rs = renderNote (60, 100, fs, block, (int) (0.4 * fs), strikeSetup);
        const double dc = meanVal (rs.mono, (int) (0.02 * fs), (int) (0.20 * fs));
        check ("no-DC", std::abs (dc) < 0.005, juce::String ("mean=") + juce::String (dc, 6));
        check ("strike-makes-sound", rms (rs.mono, (int) (0.015 * fs), (int) (0.05 * fs)) > 0.02,
               juce::String ("rms=") + juce::String (rms (rs.mono, (int) (0.015 * fs), (int) (0.05 * fs)), 5));

        // Strike top-octave alias audit: a high struck note must stay finite + bounded.
        auto rhi = renderNote (96, 110, fs, block, (int) (0.2 * fs), strikeSetup);   // C7
        check ("strike-hi-clean", allFinite (rhi.mono) && peakAbs (rhi.mono) < 4.0,
               juce::String ("peak=") + juce::String (peakAbs (rhi.mono), 4));
    }

    //--------------------------------------------------------------------------
    // 2.3  Modal resonator + cross-driving + inharmonicity.
    //--------------------------------------------------------------------------
    {
        // Cross-driving: each exciter must drive the Modal bank to audible sound.
        auto modalSetup = [] (int excType)
        {
            return [excType] (juce::AudioProcessorValueTreeState& a)
            {
                setParam (a, ParamIDs::excitationType, (float) excType);
                setParam (a, ParamIDs::resonatorType,  1.0f);   // Modal
                setParam (a, ParamIDs::decay,         70.0f);
                setParam (a, ParamIDs::inharmonicity, 30.0f);
                setParam (a, ParamIDs::modeBrightness, 50.0f);
                setParam (a, ParamIDs::bowForce,      60.0f);
            };
        };
        const char* exNames[] = { "modal-pluck", "modal-strike", "modal-bow" };
        for (int ex = 0; ex < 3; ++ex)
        {
            auto m = renderNote (60, 110, fs, block, (int) (0.4 * fs), modalSetup (ex));
            const int probeOff = (ex == 2) ? (int) (0.30 * fs) : (int) (0.02 * fs);   // bow: probe late (sustains)
            const double e = rms (m.mono, probeOff, (int) (0.08 * fs));
            check (exNames[ex], e > 0.02 && allFinite (m.mono) && peakAbs (m.mono) < 4.0,
                   juce::String ("rms=") + juce::String (e, 5) + " peak=" + juce::String (peakAbs (m.mono), 3));
        }

        // Inharmonicity stretches the upper partials (unit-level, deterministic):
        // 0% ≈ harmonic (f_k = k·f0), high% ≈ bell (top mode pushed sharp).
        ModalResonator mr;
        mr.prepare (fs);
        mr.setParams (220.0f, 70.0f, 0.0f, 50.0f);
        const float harmonicTop = mr.getModeFreqs()[ModalResonator::NUM_MODES - 1];
        mr.setParams (220.0f, 70.0f, 100.0f, 50.0f);
        const float stretchedTop = mr.getModeFreqs()[ModalResonator::NUM_MODES - 1];
        check ("modal-fundamental-harmonic", std::abs (harmonicTop - 220.0f * 8.0f) < 5.0f,
               juce::String ("topMode@0%=") + juce::String (harmonicTop, 1) + " Hz (≈1760)");
        check ("inharmonicity-stretches", stretchedTop > harmonicTop * 1.2f,
               juce::String ("0%=") + juce::String (harmonicTop, 1)
               + " → 100%=" + juce::String (stretchedTop, 1) + " Hz");

        // Modal rings + decays per Decay (unit-level): higher Decay → more late energy.
        auto modalRing = [&] (float decayPct)
        {
            ModalResonator m;
            m.prepare (fs);
            m.setParams (220.0f, decayPct, 0.0f, 50.0f);
            std::vector<float> y;
            const int n = (int) (0.5 * fs);
            y.reserve ((size_t) n);
            y.push_back (m.processSample (0.05f));              // unit-ish impulse
            for (int i = 1; i < n; ++i) y.push_back (m.processSample (0.0f));
            return y;
        };
        auto rgShort = modalRing (20.0f);
        auto rgLong  = modalRing (90.0f);
        const double lateShort = rms (rgShort, (int) (0.30 * fs), (int) (0.05 * fs));
        const double lateLong  = rms (rgLong,  (int) (0.30 * fs), (int) (0.05 * fs));
        check ("modal-rings", rms (rgLong, (int) (0.005 * fs), (int) (0.05 * fs)) > 1.0e-4 && allFinite (rgLong),
               juce::String ("earlyRms=") + juce::String (rms (rgLong, (int) (0.005 * fs), (int) (0.05 * fs)), 6));
        check ("modal-decay-tracks-Decay", lateLong > lateShort,
               juce::String ("short=") + juce::String (lateShort, 6) + " long=" + juce::String (lateLong, 6));
    }

    //--------------------------------------------------------------------------
    // 3. State round-trip smoke (kept from Stage 1).
    //--------------------------------------------------------------------------
    {
        OSimplePhysicalModelSynthAudioProcessor proc;
        proc.setPlayConfigDetails (0, 2, fs, block);
        proc.prepareToPlay (fs, block);
        juce::MemoryBlock state;
        proc.getStateInformation (state);
        proc.setStateInformation (state.getData(), (int) state.getSize());
        check ("state-roundtrip", state.getSize() > 0,
               juce::String ("stateBytes=") + juce::String ((int) state.getSize()));
        proc.releaseResources();
    }

    std::printf ("\n%s — %d failure(s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
