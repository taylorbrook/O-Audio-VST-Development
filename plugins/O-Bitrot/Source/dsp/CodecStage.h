/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - CodecStage (Stage 2, Phase 2.5 — full implementation)

    Landline / cellphone chain (DSP-05):

        mono sum -> HPF 300 Hz (2x cascaded 2-pole = 4-pole Butterworth)
                 -> LPF 3400 Hz (4-pole)
                 -> 8 kHz fractional-hold latch (second FractionalHoldLatch
                    instance, rate = 8000/fs, jitterFactor 1 — the crude
                    resampling IS the aesthetic)
                 -> codec round trip (Mu-law | GSM 06.10 via vendored libgsm)
                 -> hold-upsample (the latch hold)
                 -> post-LPF ~3400 Hz (4-pole)
                 -> equal-power CODEC_MIX blend with the kCompLatency-aligned
                    pre-codec STEREO signal.

    LATENCY (RESEARCH stage-2 section 3): CodecStage owns ALL of the plugin's
    latency and presents exactly kCompLatency = ceil(0.020 * fs) samples in
    every state:
      * disabled:  the plain stereo integer delay ring (bit-exact — the
                   Phase 2.1 skeleton's exact code and indexing, so all
                   pre-2.5 renders are unchanged);
      * Mu-law:    the round trip is zero-latency, so the codec-path output
                   runs through its own mono kCompLatency ring;
      * GSM:       the 160-slot frame chain REPLACES the delay — a value
                   latched at grid tick t emerges at tick t + 160 (the
                   PREVIOUSLY-decoded frame plays while the next accumulates;
                   emitting the just-completed frame same-tick would make the
                   delay vary 0..159 grid samples across the frame). 160 grid
                   periods = 0.020 * fs host samples exactly at integer
                   0.020*fs rates; sub-sample alignment carries +/- one grid
                   period (~0.125 ms) by construction — accepted (RESEARCH).

    Filters: juce::dsp::IIR with ArrayCoefficients ONLY (RT-safe 6-raw form,
    pattern_arraycoefficients_rt_safe_iir), coefficients computed in prepare
    (fixed cutoffs), Butterworth cascade Qs 0.5412 / 1.3066.

    DETERMINISM: the codec path has NO RNG. The 8 kHz latch phase and both
    delay rings advance unconditionally every sample (pure functions of the
    sample count — never reset by enable/mode flips); mu-law is computed per
    grid crossing. Only the GSM encode/decode call is gated on audibility
    (enable gain > 0 and gsm mode weight > 0) — when gated, the output frame
    is zeroed, so (re-)engagement starts with <= 20 ms of silence covered by
    the 10 ms fades (documented; output frame is also primed with zeros in
    prepare). Mode switch mu-law <-> GSM crossfades ~10 ms between the two
    delay-aligned sub-paths; CODEC_ENABLE rides the EnableFade rails
    (bit-transparent when off).

    RT contract: gsm_create() allocates in prepare only; gsm_encode/decode
    are allocation-free fixed work per 160-slot frame on the audio thread;
    gsm_destroy() in releaseResources/destructor (double-destroy guarded).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <gsm.h>

#include "CrushStage.h"    // FractionalHoldLatch (shared primitive)
#include "Arbitration.h"   // EnableFade

class CodecStage
{
public:
    CodecStage() = default;
    ~CodecStage() { destroyHandles(); }

    // The only allocations (buffers + gsm_create). delaySamples = kCompLatency.
    void prepare (double sampleRate, int delaySamples,
                  bool initiallyEnabled, bool initiallyGsm, float initialMix01)
    {
        fs     = sampleRate;
        length = juce::jmax (1, delaySamples);

        delayBuffer.setSize (2, length);
        delayBuffer.clear();
        writeIndex = 0;

        muDelay.assign ((size_t) length, 0.0f);
        muWrite = 0;

        gridRate = juce::jmin (0.5, 8000.0 / fs);
        gridLatch.reset();
        muHeld  = 0.0f;
        gsmHeld = 0.0f;

        // Fixed-cutoff Butterworth cascades — coefficients computed HERE,
        // never on the audio path (ArrayCoefficients, 6-raw form).
        const juce::dsp::ProcessSpec spec { fs, 512u, 1u };
        using AC = juce::dsp::IIR::ArrayCoefficients<float>;

        auto setup = [&spec] (juce::dsp::IIR::Filter<float>& flt,
                              const std::array<float, 6>& coeffs)
        {
            flt.prepare (spec);
            *flt.coefficients = coeffs;
            flt.reset();
        };

        setup (hp1,   AC::makeHighPass (fs, 300.0f,  0.5412f));
        setup (hp2,   AC::makeHighPass (fs, 300.0f,  1.3066f));
        setup (lp1,   AC::makeLowPass  (fs, 3400.0f, 0.5412f));
        setup (lp2,   AC::makeLowPass  (fs, 3400.0f, 1.3066f));
        setup (post1, AC::makeLowPass  (fs, 3400.0f, 0.5412f));
        setup (post2, AC::makeLowPass  (fs, 3400.0f, 1.3066f));

        // GSM handles: one encoder + one decoder (states differ). Re-prepare
        // destroys and recreates.
        destroyHandles();
        encState = gsm_create();
        decState = gsm_create();

        frameSlot = 0;
        std::fill (std::begin (frameIn),     std::end (frameIn),     (gsm_signal) 0);
        std::fill (std::begin (prevDecoded), std::end (prevDecoded), (gsm_signal) 0);   // primed silent

        enableFade.prepare (fs, initiallyEnabled);
        modeFade.prepare (fs, initiallyGsm);
        mixSmooth.reset (fs, 0.02);
        mixSmooth.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, initialMix01));
    }

    // gsm_destroy is safe here (guarded); prepare recreates. processSample
    // guards on null handles (skips codec work, zero frames).
    void releaseHandles() noexcept { destroyHandles(); }

    int getDelaySamples() const noexcept { return length; }

    // Per-block snapshot.
    void setParams (bool enabled, bool gsmMode, float mix01) noexcept
    {
        enableFade.setEnabled (enabled);
        modeFade.setEnabled (gsmMode);
        mixSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, mix01));
    }

    // Per-sample stereo, in place. No RNG anywhere in this stage.
    void processSample (float& left, float& right) noexcept
    {
        // 1. Pre-codec stereo alignment delay — the Phase 2.1 skeleton's
        //    exact ring (read old, write new, advance): the disabled state
        //    stays bit-identical to all pre-2.5 renders.
        auto* dl = delayBuffer.getWritePointer (0);
        auto* dr = delayBuffer.getWritePointer (1);
        const float dryL = dl[writeIndex];
        const float dryR = dr[writeIndex];
        dl[writeIndex] = left;
        dr[writeIndex] = right;
        if (++writeIndex >= length)
            writeIndex = 0;

        // 2. Fades advance unconditionally (exact rails).
        const float g   = enableFade.next();
        const float w   = modeFade.next();     // 0 = mu-law, 1 = GSM
        const float mix = mixSmooth.getNextValue();

        // 3. Codec front end — unconditional (filters warm, latch phase a
        //    pure function of the sample count).
        const float mono = 0.5f * (left + right);
        float sig = hp1.processSample (mono);
        sig = hp2.processSample (sig);
        sig = lp1.processSample (sig);
        sig = lp2.processSample (sig);

        float latched = 0.0f, latchedR = 0.0f;
        const bool crossed = gridLatch.processSample (sig, sig, gridRate, 1.0,
                                                      latched, latchedR);

        if (crossed)
        {
            // Per-grid-tick codec work (~8 kHz, not per host sample).
            muHeld = muLawRoundTrip (latched);

            frameIn[frameSlot] = floatToGsm (latched);
            gsmHeld = gsmToFloat (prevDecoded[frameSlot]);   // previous frame plays

            if (++frameSlot >= kFrameLen)
            {
                frameSlot = 0;

                if (encState != nullptr && decState != nullptr && g > 0.0f && w > 0.0f)
                {
                    gsm_encode (encState, frameIn, frameBytes);
                    const int err = gsm_decode (decState, frameBytes, prevDecoded);
                    juce::ignoreUnused (err);
                    jassert (err == 0);          // cannot fail on our own frames
                }
                else
                {
                    // GSM side inaudible: keep the output frame silent so
                    // (re-)engagement is deterministic (<= 20 ms of silence,
                    // covered by the 10 ms fades).
                    std::fill (std::begin (prevDecoded), std::end (prevDecoded),
                               (gsm_signal) 0);
                }
            }
        }

        // 4. Delay-align the sub-paths, then mode-blend. Mu-law is
        //    zero-latency -> its own kCompLatency mono ring; GSM already
        //    carries the 160-grid structural delay.
        const float muAligned = muDelay[(size_t) muWrite];
        muDelay[(size_t) muWrite] = muHeld;
        if (++muWrite >= length)
            muWrite = 0;

        const float codecMono = muAligned * (1.0f - w) + gsmHeld * w;

        float post = post1.processSample (codecMono);
        post = post2.processSample (post);

        // 5. Equal-power CODEC_MIX blend with the aligned pre-codec stereo.
        const float theta = mix * juce::MathConstants<float>::halfPi;
        const float cw    = std::cos (theta);
        const float sw    = std::sin (theta);
        const float pL    = dryL * cw + post * sw;
        const float pR    = dryR * cw + post * sw;

        // 6. Enable rails (bit-transparent bypass at the 0.0 rail).
        if (g >= 1.0f)
        {
            left  = pL;
            right = pR;
        }
        else if (g > 0.0f)
        {
            left  = dryL * (1.0f - g) + pL * g;
            right = dryR * (1.0f - g) + pR * g;
        }
        else
        {
            left  = dryL;
            right = dryR;
        }
    }

private:
    static constexpr int kFrameLen = 160;   // GSM 06.10 frame, 8 kHz grid samples

    void destroyHandles() noexcept
    {
        if (encState != nullptr) { gsm_destroy (encState); encState = nullptr; }
        if (decState != nullptr) { gsm_destroy (decState); decState = nullptr; }
    }

    // Mu-law round trip: continuous formula + 8-bit mid-tread (deep-dive 1.4).
    static float muLawRoundTrip (float x) noexcept
    {
        constexpr float invLog256 = 0.180337f;              // 1 / log1p(255)
        constexpr float delta     = 2.0f / 256.0f;          // 8-bit mid-tread

        const float xc = juce::jlimit (-1.0f, 1.0f, x);
        const float y  = std::copysign (std::log1p (255.0f * std::abs (xc)) * invLog256, xc);
        const float yq = delta * std::floor (y / delta + 0.5f);

        return std::copysign ((std::pow (256.0f, std::abs (yq)) - 1.0f) * (1.0f / 255.0f), yq);
    }

    // Scaling per RESEARCH 4 (RSBrokenMedia-proven): 13-bit samples in the
    // upper bits. v*8 keeps the low 3 bits zero (the 0xFFF8 mask) without a
    // UB left-shift of a negative int.
    static gsm_signal floatToGsm (float x) noexcept
    {
        const int v = juce::jlimit (-4095, 4095,
                                    juce::roundToIntAccurate ((double) juce::jlimit (-1.0f, 1.0f, x) * 4096.0));
        return (gsm_signal) (v * 8);
    }

    static float gsmToFloat (gsm_signal s) noexcept
    {
        return (float) (s >> 3) / 4096.0f;
    }

    double fs     = 48000.0;
    int    length = 1;

    // Pre-codec stereo alignment ring (the skeleton's exact code).
    juce::AudioBuffer<float> delayBuffer;
    int writeIndex = 0;

    // Mu-law path alignment ring (mono).
    std::vector<float> muDelay;
    int muWrite = 0;

    // Front end
    juce::dsp::IIR::Filter<float> hp1, hp2, lp1, lp2, post1, post2;
    FractionalHoldLatch gridLatch;
    double gridRate = 8000.0 / 48000.0;

    // Codec state
    float      muHeld  = 0.0f;
    float      gsmHeld = 0.0f;
    gsm        encState = nullptr;
    gsm        decState = nullptr;
    gsm_signal frameIn[kFrameLen]     = {};
    gsm_signal prevDecoded[kFrameLen] = {};
    gsm_frame  frameBytes             = {};
    int        frameSlot              = 0;

    EnableFade enableFade;
    EnableFade modeFade;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmooth;
};
