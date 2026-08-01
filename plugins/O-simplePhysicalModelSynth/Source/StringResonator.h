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

    O-simplePhysicalModelSynth - StringResonator (Karplus-Strong loop)

    Single-rail KS string: a Thiran fractional delay line in a feedback loop with
    a one-pole damping LPF. Pitch is set by the delay length, group-delay-compensated
    for the loop LPF; timbre by the LPF cutoff (Damping); sustain by the feedback
    gain g (Decay), hard-clamped < 1 for unconditional stability (QUAL-01).

    All exciters drive the loop through processAdditive(e): Pluck / Strike seed a
    decaying burst; Bow feeds a continuous friction-weighted noise drive (the
    CONTEXT-D2 fallback — memoryless STK friction does NOT self-oscillate in a
    SINGLE KS loop, only in the dual-rail Waveguide deferred to v1.1, so the bow
    sustains via band-limited friction noise whose coupling the Stribeck curve sets).

    Ported (RESEARCH §1, F1):
      - OnePoleLPF + 64-sample shadow-filter crossfade : O-Lyrica WaveguideString.h
      - Thiran DelayLine usage + sizing + ≥2-sample clamp : O-Bowed WaveguideString
      - Group-delay formula τ_lpf = fs/(2π·fc), N = fs/f0 − τ_lpf : O-Lyrica
      - DC blocker : authored new (none exists to port — F1).

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include "OnePoleLPF.h"

class StringResonator
{
public:
    //==========================================================================
    void prepare (double sr, int maxBlockSize) noexcept
    {
        sampleRate = sr;

        // Size for the ACTUAL tuning floor, not "lowest musical note": MIDI 0 is
        // 8.18 Hz and coarseTune −24 pushes anything below MIDI 38 under 20 Hz.
        // setFrequency clamps f0 ≥ 8 Hz, so sizing must cover fs/8 or setDelay
        // jasserts + silently clamps those notes to one fixed pitch (WR-03).
        const int maxDelay = static_cast<int> (sampleRate / 8.0) + 100;

        juce::dsp::ProcessSpec spec {
            sampleRate,
            static_cast<juce::uint32> (juce::jmax (1, maxBlockSize)),
            1
        };
        delay.prepare (spec);
        delay.setMaximumDelayInSamples (maxDelay);

        // Default loop cutoff so the first recompute has a valid τ_lpf.
        loopDamping.setCutoff (currentCutoff, sampleRate);
        loopDampingShadow = loopDamping;
        reset();
        recomputeDelay();
    }

    void reset() noexcept
    {
        delay.reset();
        loopDamping.reset();
        loopDampingShadow.reset();
        filterCrossfadeRemaining = 0;
        dcX1 = dcY1 = 0.0f;
    }

    //==========================================================================
    // Parameter setters — called at block start from the voice. Each only does
    // work when its input actually changes (cheap; avoids retriggering the
    // crossfade on a static knob).
    void setFrequency (float f0) noexcept
    {
        f0 = juce::jmax (8.0f, f0);
        if (! juce::approximatelyEqual (f0, frequency))
        {
            frequency = f0;
            recomputeDelay();
        }
    }

    // Damping 0–100% → loop-LPF cutoff, log-mapped (more damping = darker).
    // 12 kHz (damping=0) … 1.5 kHz (damping=100). Cutoff change also shifts τ_lpf,
    // so the tuned delay length is recomputed too (cutoff↔τ coupling, RESEARCH §1.3).
    void setDamping (float dampingPct) noexcept
    {
        const float d  = juce::jlimit (0.0f, 100.0f, dampingPct) / 100.0f;
        const float fc = 12000.0f * std::pow (1500.0f / 12000.0f, d);
        setLoopCutoff (fc);
    }

    // Decay 0–100% → feedback gain, hard-clamped < 1 (architecture lerp).
    void setDecay (float decayPct) noexcept
    {
        const float d = juce::jlimit (0.0f, 100.0f, decayPct) / 100.0f;
        feedback = juce::jlimit (0.80f, 0.999f, juce::jmap (d, 0.80f, 0.999f));
    }

    // Material macro override: drive cutoff + feedback directly (steel↔nylon).
    void setMaterialDamping (float fc, float g) noexcept
    {
        setLoopCutoff (fc);
        feedback = juce::jlimit (0.80f, 0.999f, g);
    }

    float getLoopCutoff() const noexcept { return currentCutoff; }
    float getFeedback()   const noexcept { return feedback; }

    //==========================================================================
    // All exciters: additive excitation e drives the loop. Pluck/Strike seed a
    // burst that decays to 0; Bow feeds continuous friction-noise (e ≠ 0 while held).
    float processAdditive (float e) noexcept
    {
        const float out = delay.popSample (0);
        const float s   = filterWithCrossfade (out);
        delay.pushSample (0, feedback * s + e);
        return dcBlock (s);
    }

private:
    //==========================================================================
    // Snapshot live→shadow, install new coeffs, restart the 64-sample fade — only
    // when the target cutoff actually moves (RESEARCH §1.6).
    void setLoopCutoff (float newCutoff) noexcept
    {
        newCutoff = juce::jlimit (1.0f, static_cast<float> (sampleRate * 0.49), newCutoff);
        if (! juce::approximatelyEqual (newCutoff, currentCutoff))
        {
            loopDampingShadow = loopDamping;                  // POD copy = coeffs + state
            loopDamping.setCutoff (newCutoff, sampleRate);
            filterCrossfadeRemaining = FILTER_CROSSFADE_LENGTH;
            currentCutoff = newCutoff;
            recomputeDelay();                                 // τ_lpf depends on fc
        }
    }

    inline float filterWithCrossfade (float x) noexcept
    {
        if (filterCrossfadeRemaining > 0)
        {
            const float t    = 1.0f - static_cast<float> (filterCrossfadeRemaining) / FILTER_CROSSFADE_LENGTH;
            const float yNew = loopDamping.processSample (x);
            const float yOld = loopDampingShadow.processSample (x);
            --filterCrossfadeRemaining;
            return yOld + (yNew - yOld) * t;
        }
        return loopDamping.processSample (x);
    }

    // N = fs/f0 − τ_lpf(f0). The loop resonance condition is total round-trip
    // PHASE delay = fs/f0 samples, so subtract the loop LPF's phase delay AT THE
    // FUNDAMENTAL (not the DC group-delay fs/(2π·fc) — that over-compensates at
    // the top octave and pushes high notes ~12 cents sharp). Thiran absorbs the
    // fractional remainder of N.
    void recomputeDelay() noexcept
    {
        const float fs = static_cast<float> (sampleRate);
        const float n  = fs / frequency - lpfPhaseDelaySamples (frequency);
        delay.setDelay (juce::jmax (2.0f, n));               // Thiran needs ≥ 2 samples
    }

    // Exact phase delay (samples) of the live loop LPF at frequency f0:
    //   H(e^jω) = (b0 + b1 e^−jω)/(1 + a1 e^−jω),  τφ = −∠H(ω)/ω.
    float lpfPhaseDelaySamples (float f0) const noexcept
    {
        const float w  = juce::MathConstants<float>::twoPi * f0 / static_cast<float> (sampleRate);
        const float cw = std::cos (w), sw = std::sin (w);
        const float nr =  loopDamping.b0 + loopDamping.b1 * cw;   // Re(numerator)
        const float ni = -loopDamping.b1 * sw;                    // Im(numerator)
        const float dr =  1.0f + loopDamping.a1 * cw;             // Re(denominator)
        const float di = -loopDamping.a1 * sw;                    // Im(denominator)
        const float phase = std::atan2 (ni, nr) - std::atan2 (di, dr);
        return -phase / juce::jmax (1.0e-6f, w);
    }

    inline float dcBlock (float x) noexcept
    {
        const float y = x - dcX1 + 0.995f * dcY1;
        dcX1 = x; dcY1 = y;
        return y;
    }

    //==========================================================================
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> delay { 4410 };

    OnePoleLPF loopDamping, loopDampingShadow;
    int  filterCrossfadeRemaining = 0;
    static constexpr int FILTER_CROSSFADE_LENGTH = 64;

    double sampleRate    = 44100.0;
    float  frequency     = 440.0f;
    float  currentCutoff = 6000.0f;   // re-set in prepare/setDamping
    float  feedback      = 0.99f;

    float  dcX1 = 0.0f, dcY1 = 0.0f;
};
