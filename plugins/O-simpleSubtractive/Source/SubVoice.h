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

    O-simpleSubtractive - Subtractive Voice (osc bank → multimode SVF → VCA)

    One polyphonic voice: an OscillatorBank source (PolyBLEP osc + sub + noise)
    runs through a multimode ZDF state-variable filter (LP/HP/BP/Notch at
    6/12/24 dB, pushable to clean tanh self-oscillation) and a VCA. Two fully
    independent juce::ADSR envelopes shape it: the FILTER env modulates cutoff
    (bipolar, in octaves) and the AMP env drives the VCA and governs voice
    lifetime.

    Per sample:  src → fcEff(cutoff·keyTrack·filterEnv·lfo) → SVF → mode select → VCA

    v1.7.0 added the one global LFO (SubLfo.h — pitch, cutoff and pulse width
    destinations), the second oscillator, the pulse width and velocity → filter
    depth. All of it rides one SubVoiceParams struct pushed once per block; the
    defaults of every new field are neutral, so a pre-1.7.0 session renders the
    same signal it did.

    Two drive paths share this voice:
      - Poly  : juce::Synthesiser calls startNote()/stopNote() (instant pitch).
      - Mono/Legato : the processor's MonoController drives a single voice via
        noteOnDirect()/noteOffDirect()/setPitchTargetNote() with glide.

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast (O-Bassoon / O-simpleFM pattern).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "OscillatorBank.h"
#include "SvfZDF.h"
#include "SubLfo.h"
#include <cmath>

//==============================================================================
// Everything a voice reads from the APVTS, pushed by the processor ONCE per
// block (voices never read parameters). Defaults are the parameter defaults.
struct SubVoiceParams
{
    // Sources
    int   wave        = 0;       // osc 1: Saw / Square / Triangle / Sine
    float pulseWidth  = 0.5f;    // Square duty, (0,1); 0.5 = the pre-1.7.0 square
    int   osc2Wave    = 0;
    float osc2Ratio   = 1.0f;    // 2^((coarse + fine/100)/12)
    float osc2Level   = 0.0f;
    float subLevel    = 0.0f;
    float noiseLevel  = 0.0f;

    // Filter
    int   filterType      = 0;
    int   slope           = 2;
    float cutoffHz        = 2000.0f;
    float resonance       = 0.10f;
    float filterEnvAmount = 0.5f;   // bipolar, octaves/kEnvOctaves
    float keyTrack        = 0.0f;
    float velToFilter     = 0.0f;   // 0 = env depth ignores velocity; 1 = depth ∝ velocity

    // Envelopes + glide
    juce::ADSR::Parameters filt { 0.005f, 0.3f, 0.4f, 0.2f };
    juce::ADSR::Parameters amp  { 0.005f, 0.3f, 0.8f, 0.1f };
    float glideSeconds = 0.0f;

    // The global LFO: block state + three depths
    SubLfo::BlockState lfo;
    float lfoToCutoff     = 0.0f;   // 0..1 → ±(depth · kLfoCutoffOctaves) octaves
    float lfoToPitchSemis = 0.0f;   // ± semitones at full LFO swing
    float lfoToPw         = 0.0f;   // 0..1 → ±(depth · kLfoPwSwing) of duty
};

//==============================================================================
class SubSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class SubVoice : public juce::SynthesiserVoice
{
public:
    // Filter mode order matches the APVTS `filterType` choice list.
    enum FilterType { LP = 0, HP = 1, BP = 2, Notch = 3 };
    // Slope order matches the APVTS `filterSlope` choice list (6/12/24 dB).
    enum Slope { Slope6 = 0, Slope12 = 1, Slope24 = 2 };

    static constexpr double kEnvOctaves       = 7.0;   // ±100% filterEnvAmount ⇒ ±7 octaves
    static constexpr double kLfoCutoffOctaves = 3.0;   // lfoToCutoff = 1 ⇒ ±3 octaves of swing
    static constexpr float  kLfoPwSwing       = 0.45f; // lfoToPw = 1 ⇒ duty swings ±0.45 (clamped 0.05..0.95)
    static constexpr float  kPwMin            = 0.05f;
    static constexpr float  kPwMax            = 0.95f;

    SubVoice() = default;
    ~SubVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SubSound*> (s) != nullptr;
    }

    //==========================================================================
    // Non-virtual JUCE-8 prepare. setSampleRate MUST precede setParameters.
    void prepareToPlay (double sr, int /*maxBlockSize*/)
    {
        setCurrentPlaybackSampleRate (sr);
        sampleRate = sr;

        filterEnv.setSampleRate (sr);
        ampEnv.setSampleRate (sr);
        filterEnv.setParameters (filterParams);
        ampEnv.setParameters (ampParams);
        appliedFilterParams = filterParams;
        appliedAmpParams    = ampParams;
        releasing = false;

        osc.setSampleRate (sr);
        freqSmoothed.reset (sr, 0.0);   // glide time re-armed per block via setGlideTime
        excState = 0x2545F491u;
    }

    //==========================================================================
    // Block param-push from the processor (once per block). Voices never read APVTS.
    void setParams (const SubVoiceParams& p) noexcept
    {
        wave            = p.wave;
        pulseWidth      = juce::jlimit (kPwMin, kPwMax, p.pulseWidth);
        osc2Wave        = p.osc2Wave;
        osc2Ratio       = juce::jmax (1.0e-3f, p.osc2Ratio);
        osc2Level       = juce::jlimit (0.0f, 1.0f, p.osc2Level);
        subLevel        = juce::jlimit (0.0f, 1.0f, p.subLevel);
        noiseLevel      = juce::jlimit (0.0f, 1.0f, p.noiseLevel);
        filterType      = p.filterType;
        slope           = p.slope;
        cutoffHz        = p.cutoffHz;
        resonance       = juce::jlimit (0.0f, 1.0f, p.resonance);
        filterEnvAmount = juce::jlimit (-1.0f, 1.0f, p.filterEnvAmount);
        keyTrack        = juce::jlimit (0.0f, 1.0f, p.keyTrack);
        velToFilter     = juce::jlimit (0.0f, 1.0f, p.velToFilter);

        lfo             = p.lfo;
        lfoToCutoff     = juce::jlimit (0.0f, 1.0f, p.lfoToCutoff);
        lfoToPitchSemis = juce::jmax (0.0f, p.lfoToPitchSemis);
        lfoToPw         = juce::jlimit (0.0f, 1.0f, p.lfoToPw);

        // Push envelope params only when they actually CHANGED, and never while
        // the voice is releasing. juce::ADSR::setParameters() recalculates
        // releaseRate from the SUSTAIN level, clobbering the envelopeVal-based
        // rate noteOff() computed — and with sustain == 0 the zeroed rate makes
        // recalculateRates() reset() the envelope mid-tail: instant cut = click
        // on every note-off. Changes made mid-release apply at the next noteOn.
        // (O-simpleFM v1.2.5 pattern; guards BOTH envelopes — a killed filter
        // env is a cutoff jump, a killed amp env is the audible click.)
        filterParams = p.filt;
        ampParams    = p.amp;
        if (! releasing)
        {
            if (! sameAdsr (p.filt, appliedFilterParams)) { filterEnv.setParameters (p.filt); appliedFilterParams = p.filt; }
            if (! sameAdsr (p.amp, appliedAmpParams))     { ampEnv.setParameters (p.amp);     appliedAmpParams    = p.amp; }
        }

        setGlideTime (p.glideSeconds);
    }

    void setGlideTime (float seconds) noexcept
    {
        // Multiplicative smoother; 0 s ⇒ effectively instant. Re-arm only on change.
        const double t = juce::jmax (0.0, (double) seconds);
        if (std::abs (t - glideTimeSeconds) > 1.0e-9)
        {
            glideTimeSeconds = t;
            freqSmoothed.reset (sampleRate, juce::jmax (1.0e-4, t));
            freqSmoothed.setCurrentAndTargetValue (juce::jmax (1.0f, freqSmoothed.getTargetValue()));
        }
    }

    //==========================================================================
    // Poly path (juce::Synthesiser drives these). Instant pitch (no glide).
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int pitchWheel) override
    {
        currentNote   = midiNote;
        pitchWheelPos = pitchWheel;
        velLevel      = juce::jlimit (0.0f, 1.0f, velocity);
        updateBend();

        // New allocation: reset source phase + filter state (never mid-note).
        osc.reset();
        onePole.reset(); svf1.reset(); svf2.reset();

        freqSmoothed.setCurrentAndTargetValue (noteHz (currentNote));

        // Apply any envelope params deferred while the previous tail released.
        releasing = false;
        applyDeferredEnvParams();

        filterEnv.noteOn();
        ampEnv.noteOn();
        active = true;
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            releasing = true;   // freeze envelope rates until the tail ends
            filterEnv.noteOff();
            ampEnv.noteOff();
        }
        else
        {
            clearCurrentNote();
            releasing = false;
            filterEnv.reset();
            ampEnv.reset();
            active = false;
        }
    }

    void pitchWheelMoved (int v) override { pitchWheelPos = v; updateBend(); }
    void controllerMoved (int, int) override {}

    //==========================================================================
    // Mono/Legato path (processor's MonoController drives a single voice).
    // retrigger=true  → Mono   (re-strike both envelopes)
    // retrigger=false → Legato (slur: keep envelopes running while held)
    void noteOnDirect (int midiNote, float velocity, bool retrigger) noexcept
    {
        const bool wasActive = active;
        currentNote = midiNote;
        updateBend();

        if (retrigger || ! wasActive)
            velLevel = juce::jlimit (0.0f, 1.0f, velocity);

        const float target = noteHz (currentNote);
        if (! wasActive)
        {
            // First note of the phrase: no glide source — snap.
            osc.reset(); onePole.reset(); svf1.reset(); svf2.reset();
            freqSmoothed.setCurrentAndTargetValue (target);
        }
        else
        {
            // Continuous pitch — glide from the current frequency, never reset phase/state.
            freqSmoothed.setTargetValue (target);
        }

        if (retrigger || ! wasActive)
        {
            // Mono re-strike: juce::ADSR::noteOn() ramps from the CURRENT level (it
            // does not zero), so an in-sustain retrigger would be inaudible. Reset
            // to 0 first so the attack genuinely re-plucks. (First note of a phrase
            // is already idle at 0 — reset is harmless.)
            releasing = false;
            applyDeferredEnvParams();
            filterEnv.reset();
            ampEnv.reset();
            filterEnv.noteOn();
            ampEnv.noteOn();
        }
        active = true;
    }

    void setPitchTargetNote (int midiNote) noexcept
    {
        currentNote = midiNote;
        updateBend();
        freqSmoothed.setTargetValue (noteHz (currentNote));
    }

    void noteOffDirect (bool allowTailOff) noexcept
    {
        if (allowTailOff)
        {
            releasing = true;   // freeze envelope rates until the tail ends
            filterEnv.noteOff();
            ampEnv.noteOff();
        }
        else
        {
            clearCurrentNote();
            releasing = false;
            filterEnv.reset();
            ampEnv.reset();
            active = false;
        }
    }

    bool isSounding() const noexcept { return ampEnv.isActive(); }

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! ampEnv.isActive())
            return;

        const int numCh = out.getNumChannels();

        // Velocity → filter: scale the envelope depth by (1 − v + v · velocity),
        // so 0 leaves the depth alone and 1 makes it proportional to velocity.
        const double envDepth = (double) filterEnvAmount
                              * (1.0 - (double) velToFilter + (double) velToFilter * (double) velLevel);
        const double keyTrackOct = (double) keyTrack * ((double) currentNote - 60.0) / 12.0;

        for (int i = 0; i < numSamples; ++i)
        {
            const float eF = filterEnv.getNextSample();   // [0,1]
            const float eA = ampEnv.getNextSample();      // [0,1]

            // The global LFO at this sample (stateless; see SubLfo.h). Evaluated
            // only if any destination is live — a 0-depth LFO costs nothing.
            const bool  lfoLive = (lfoToCutoff > 0.0f || lfoToPitchSemis > 0.0f || lfoToPw > 0.0f);
            const float l       = lfoLive ? lfo.at (startSample + i) : 0.0f;

            const float baseHz = freqSmoothed.getNextValue();
            float fOsc = baseHz * bendFactor;
            if (lfoToPitchSemis > 0.0f)
                fOsc *= std::exp2 (l * lfoToPitchSemis * (1.0f / 12.0f));

            float pw = pulseWidth;
            if (lfoToPw > 0.0f)
                pw = juce::jlimit (kPwMin, kPwMax, pw + l * lfoToPw * kLfoPwSwing);

            const float src = osc.next (wave, fOsc, pw, osc2Wave, osc2Ratio, osc2Level, subLevel, noiseLevel);

            // --- Effective cutoff: base · 2^(key-track + bipolar filter env + LFO), in octaves ---
            const double octaves = keyTrackOct
                                 + envDepth * (double) eF * kEnvOctaves
                                 + (double) l * (double) lfoToCutoff * kLfoCutoffOctaves;
            double fcEff = (double) cutoffHz * std::exp2 (octaves);
            fcEff = juce::jlimit (20.0, juce::jmin (20000.0, 0.45 * sampleRate), fcEff);

            const double g = std::tan (juce::MathConstants<double>::pi * fcEff / sampleRate);
            const double k = resonanceToK (resonance);

            // The tanh self-osc bounding is engaged ONLY in the strong-resonance /
            // self-osc zone (top of the knob). Below that the filter is LINEAR, so
            // the closed-form magnitude curve matches the running filter exactly.
            const bool nl = (k < kNonlinearThresh);

            // --- Self-oscillation seed: as k→0, inject a tiny excitation so the
            //     resonator starts from silence; tanh in the SVF bounds it to a
            //     clean sine. Negligible (and filtered out) away from self-osc. ---
            const double selfOscAmt = juce::jlimit (0.0, 1.0, (kExciteThresh - k) / kExciteThresh);
            const double x = (double) src + selfOscAmt * 1.0e-3 * excNoise();

            double out1 = filterSample (x, g, k, nl);

            // Resonance make-up trim so sweeps don't jump in loudness.
            out1 *= 1.0 / (1.0 + 0.5 * (double) resonance);

            if (! std::isfinite (out1)) out1 = 0.0;

            const float sample = (float) out1 * eA * velLevel;
            for (int ch = 0; ch < numCh; ++ch)
                out.addSample (ch, startSample + i, sample);

            lastFcEff = fcEff;
            lastK     = k;
            lastEf    = eF;
            lastEa    = eA;
        }

        // Voice lifetime keyed on the AMP envelope only (a long filter-env release
        // must never keep a silent voice alive).
        if (! ampEnv.isActive())
        {
            clearCurrentNote();
            active = false;
            releasing = false;
        }
    }

    //==========================================================================
    // Lead-voice display reporting (read by the processor → atomics → UI/curve).
    double getLastCutoffHz()  const noexcept { return lastFcEff; }
    double getLastK()         const noexcept { return lastK; }
    float  getFilterEnvVal()  const noexcept { return lastEf; }
    float  getAmpEnvVal()     const noexcept { return lastEa; }

private:
    //==========================================================================
    static bool sameAdsr (const juce::ADSR::Parameters& a, const juce::ADSR::Parameters& b) noexcept
    {
        return a.attack  == b.attack  && a.decay   == b.decay
            && a.sustain == b.sustain && a.release == b.release;
    }

    // Apply envelope params deferred while a release tail was in flight.
    void applyDeferredEnvParams() noexcept
    {
        if (! sameAdsr (filterParams, appliedFilterParams)) { filterEnv.setParameters (filterParams); appliedFilterParams = filterParams; }
        if (! sameAdsr (ampParams, appliedAmpParams))       { ampEnv.setParameters (ampParams);       appliedAmpParams    = ampParams; }
    }

    //==========================================================================
    static float noteHz (int note) noexcept
    {
        return (float) juce::MidiMessage::getMidiNoteInHertz (note);
    }

    void updateBend() noexcept
    {
        const double bendSemis = ((double) pitchWheelPos - 8192.0) / 8192.0 * 2.0; // ±2 st
        bendFactor = (float) std::pow (2.0, bendSemis / 12.0);
    }

    // resonance 0..1 → k = 1/Q. Tapered: res=0 ⇒ k≈2 (Q=0.5, no peak);
    // Butterworth ≈ res 0.3. A LOCALIZED bias (res^8) drives k slightly NEGATIVE
    // at the very top so the resonator self-oscillates — loop gain > 1, bounded by
    // the SVF tanh into a clean limit-cycle sine. The bias is negligible below
    // res≈0.9 (res^8 ≈ 0), so the magnitude curve stays exact in the normal range.
    static double resonanceToK (float res) noexcept
    {
        const double r = juce::jlimit (0.0, 1.0, (double) res);
        return 2.0 * std::pow (1.0 - r, 0.6) - 0.18 * std::pow (r, 8.0);
    }

    // Mode-select from one SVF stage's outputs. `in` is the signal fed to THIS
    // stage (needed for the derived notch = in − k·bp).
    static double selectMode (int type, double in, double lp, double bp, double hp, double k) noexcept
    {
        switch (type)
        {
            case LP:    return lp;
            case HP:    return hp;
            case BP:    return bp;
            case Notch: return in - k * bp;
            default:    return lp;
        }
    }

    // Filter the source through the selected slope + mode. Recomputed per sample.
    // `nl` engages the tanh self-osc bounding (strong-resonance zone only).
    double filterSample (double x, double g, double k, bool nl) noexcept
    {
        if (slope == Slope6)
        {
            double lp, hp;
            onePole.process (x, g, lp, hp);
            if (filterType == LP) return lp;
            if (filterType == HP) return hp;
            // A 1-pole has no resonant BP/Notch — degrade gracefully to a single
            // 12 dB SVF stage so those modes still function at the 6 dB setting.
            double l, b, h; svf1.process (x, g, k, nl, l, b, h);
            return selectMode (filterType, x, l, b, h, k);
        }

        if (slope == Slope12)
        {
            double l, b, h; svf1.process (x, g, k, nl, l, b, h);
            return selectMode (filterType, x, l, b, h, k);
        }

        // 24 dB: two stages cascaded at the same cutoff; resonance on stage 1 only.
        double l1, b1, h1; svf1.process (x, g, k, nl, l1, b1, h1);
        const double sel1 = selectMode (filterType, x, l1, b1, h1, k);

        const double kStage2 = juce::MathConstants<double>::sqrt2; // Butterworth, no extra peak
        double l2, b2, h2; svf2.process (sel1, g, kStage2, false, l2, b2, h2);
        return selectMode (filterType, sel1, l2, b2, h2, kStage2);
    }

    // Tiny deterministic noise for the self-oscillation seed (xorshift, [-1,1]).
    float excNoise() noexcept
    {
        excState ^= excState << 13; excState ^= excState >> 17; excState ^= excState << 5;
        return (float) ((int32_t) excState) * (1.0f / 2147483648.0f);
    }

    //==========================================================================
    static constexpr double kNonlinearThresh = 0.60; // k below this ⇒ tanh bounding (top of res knob)
    static constexpr double kExciteThresh    = 0.12; // k below this ⇒ self-osc seed

    double sampleRate = 44100.0;

    OscillatorBank osc;
    OnePoleTPT     onePole;
    SvfZDF         svf1, svf2;

    juce::ADSR filterEnv, ampEnv;
    juce::ADSR::Parameters filterParams { 0.005f, 0.3f, 0.4f, 0.2f };
    juce::ADSR::Parameters ampParams    { 0.005f, 0.3f, 0.8f, 0.1f };
    juce::ADSR::Parameters appliedFilterParams = filterParams;   // last values pushed to the live envelopes
    juce::ADSR::Parameters appliedAmpParams    = ampParams;
    bool releasing = false;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> freqSmoothed { 440.0f };
    double glideTimeSeconds = 0.0;

    // Cached block params.
    int   wave = 0, osc2Wave = 0, filterType = LP, slope = Slope24;
    float pulseWidth = 0.5f, osc2Ratio = 1.0f, osc2Level = 0.0f;
    float subLevel = 0.0f, noiseLevel = 0.0f;
    float cutoffHz = 2000.0f, resonance = 0.1f, filterEnvAmount = 0.5f, keyTrack = 0.0f, velToFilter = 0.0f;
    SubLfo::BlockState lfo;
    float lfoToCutoff = 0.0f, lfoToPitchSemis = 0.0f, lfoToPw = 0.0f;

    int   currentNote = 69, pitchWheelPos = 8192;
    float velLevel = 1.0f, bendFactor = 1.0f;
    bool  active = false;

    // Lead-voice display state.
    double lastFcEff = 2000.0, lastK = 1.4;
    float  lastEf = 0.0f, lastEa = 0.0f;

    uint32_t excState = 0x2545F491u;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubVoice)
};
