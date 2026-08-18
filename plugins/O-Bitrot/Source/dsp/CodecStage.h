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
                 -> fast feed-forward AGC (v1.8.0)
                 -> post-LPF ~3400 Hz (4-pole)
                 -> equal-power CODEC_MIX blend with the kCompLatency-aligned
                    pre-codec STEREO signal.

    v1.8.0 (improvement brief items 16, 29, 7) — three changes, all confined
    to the codec sub-path:

      * ITEM 29, segmented mu-law. muLawRoundTrip was the continuous
        log1p/pow curve with a uniform companded-domain quantizer. Real G.711
        quantizes on 8 piecewise-linear chords of 16 steps; this is the ITU-T
        G.711 / Sun reference encoder (14-bit domain, bias 33) against a
        256-entry decode LUT. Silence still maps to EXACTLY 0.0f; full scale
        lands on 32124/32768 (-0.17 dB), which is the real codec's clip point
        rather than a normalisation choice. Pure lookup: RNG-free,
        zero-latency, so the mu-path alignment ring is untouched.

      * ITEM 16, codec AGC. The research chain is "mu-law OR GSM -> fast ~4:1
        comp -> optional noise bed", and the pumping is a large part of why a
        phone sounds like a phone: without it quiet sources stay quiet instead
        of being dragged up into the codec noise. A per-sample one-pole
        feed-forward compressor (attack 1 ms, release 50 ms, threshold
        -20 dBFS, 4:1, fixed +10 dB makeup) sits after the mode blend and
        before post1. CODEC_AGC scales the whole gain in the dB domain, so
        depth 0 is exp(0) = 1.0f and the multiply is bit-transparent.

      * ITEM 7, codec-domain frame loss. The chain order is Packet -> Codec,
        so PCM was lost BEFORE encoding; real cellular loss drops ENCODED
        frames and the decoder conceals by repeating the previous frame's
        parameters with attenuation — the metallic warble everyone knows as
        bad reception. When GSM and PACKET are both enabled, a lost frame now
        re-feeds the decoder the previous frame's BYTES (the encoder still
        runs: it lives at the far end of the link, and loss happens in
        transit), attenuating a further 3 dB per consecutive repeat and
        muting at 16 frames — GSM 06.11's own mute point.

        COUPLING: the 20 ms grids nominally match (packet grid 0.020*fs;
        160 GSM frame slots at 8 kHz) but the frame boundary rides the 8 kHz
        FRACTIONAL latch crossings and is phase-offset from the packet grid,
        so a shared counter would drift. The loss state is instead looked up
        PER FRAME: the processor hands each sample the packet's current loss
        flag and CodecStage consumes whichever value arrives on the sample
        that closes a frame. The PCM-domain stage keeps running underneath
        for the codec-off / VoIP case; during a lost frame the GSM decoder
        output does not depend on its input at all, so the codec repeat masks
        the PCM concealment for that frame rather than stacking onto it.

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

    DETERMINISM: the codec path has NO RNG — v1.8.0 included. The AGC is a
    pure function of the sub-path signal and its envelope advances every
    sample regardless of CODEC_AGC; the frame-loss decision is a pure function
    of the packet stage's GE chain, which is itself a pure function of (seed,
    params, sample count). Both are per-sample, so block-size invariance
    (QUAL-02) holds. The 8 kHz latch phase and both delay rings advance
    unconditionally every sample (pure functions of the sample count — never
    reset by enable/mode flips); mu-law is computed per grid crossing. Only
    the GSM encode/decode call is gated on audibility
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

#include <array>
#include <cstddef>

#include "CrushStage.h"    // FractionalHoldLatch (shared primitive)
#include "Arbitration.h"   // EnableFade

namespace g711
{
//==========================================================================
// ITEM 29 — G.711 mu-law, SEGMENTED (ITU-T G.711, Sun/CCITT reference).
//
// Eight piecewise-linear chords of 16 steps, not the continuous log curve
// v1.7.0 and earlier used. Encode works in the 14-bit domain with bias 33
// and clip 8159; decode is a 256-entry table built at compile time from
// the 16-bit-domain bias 132, so the audio thread does a shift-search and
// one array read — no pow, no log, no static-init guard.
//
// Landmarks worth stating because they are NOT normalisation choices:
//   * code 0xFF decodes to exactly 0 -> digital silence stays silent;
//   * full scale encodes to 0x80 and decodes to 32124/32768 = -0.17 dBFS,
//     which is where a real landline clips.
//==========================================================================

// Upper bound of each chord in the biased 14-bit domain.
constexpr int kMuSegEnd[8] = { 0x3F, 0x7F, 0xFF, 0x1FF,
                               0x3FF, 0x7FF, 0xFFF, 0x1FFF };

constexpr int muSegment (int val) noexcept
{
    for (int i = 0; i < 8; ++i)
        if (val <= kMuSegEnd[i])
            return i;

    return 8;
}

constexpr unsigned char linearToMuLaw (int pcm16) noexcept
{
    int v = pcm16 >> 2;                                 // 16-bit -> 14-bit
    int mask;

    if (v < 0) { v = -v; mask = 0x7F; }
    else       {         mask = 0xFF; }

    if (v > 8159)                                       // CLIP
        v = 8159;

    v += 33;                                            // BIAS >> 2

    const int seg = muSegment (v);

    if (seg >= 8)
        return (unsigned char) (0x7F ^ mask);

    return (unsigned char) (((seg << 4) | ((v >> (seg + 1)) & 0x0F)) ^ mask);
}

constexpr std::array<float, 256> makeMuDecodeTable() noexcept
{
    std::array<float, 256> t {};

    for (int i = 0; i < 256; ++i)
    {
        const int u = (~i) & 0xFF;
        int       v = (((u & 0x0F) << 3) + 132) << ((u & 0x70) >> 4);

        v = (u & 0x80) != 0 ? (132 - v) : (v - 132);
        t[(std::size_t) i] = (float) v * (1.0f / 32768.0f);
    }

    return t;
}

inline constexpr std::array<float, 256> kMuDecode = makeMuDecodeTable();
}   // namespace g711

class CodecStage
{
public:
    CodecStage() = default;
    ~CodecStage() { destroyHandles(); }

    // The only allocations (buffers + gsm_create). delaySamples = kCompLatency.
    void prepare (double sampleRate, int delaySamples,
                  bool initiallyEnabled, bool initiallyGsm, float initialMix01,
                  float initialAgc01)
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
        std::fill (std::begin (frameIn),        std::end (frameIn),        (gsm_signal) 0);
        std::fill (std::begin (prevDecoded),    std::end (prevDecoded),    (gsm_signal) 0);   // primed silent
        std::fill (std::begin (prevFrameBytes), std::end (prevFrameBytes), (unsigned char) 0);

        havePrevFrame = false;
        lostFrameRun  = 0;
        plcPrev       = 1.0f;
        plcTarget     = 1.0f;

        // AGC one-pole coefficients (item 16). y = x + a * (y - x), so a is
        // the per-sample retention factor of the time constant.
        agcAtt = (float) std::exp (-1.0 / (kAgcAttackSec  * fs));
        agcRel = (float) std::exp (-1.0 / (kAgcReleaseSec * fs));

        // Primed at the UNITY-GAIN level, not at zero. An envelope starting
        // from silence sits at full makeup, so the codec's very first signal
        // — which arrives ~20 ms in, past the enable fade — met +10 dB and
        // overshot to about +3.7 dBFS for the ~1 ms the attack takes to pull
        // down. Starting at unity means a cold start is flat and the AGC
        // RELEASES up into its makeup over 50 ms if the material is quiet.
        // (Re-entry after a PLC mute still overshoots: that one is the
        // compressor doing its job on a real 60 dB step, and it is part of
        // how a line coming back sounds.)
        agcEnv = kAgcUnityEnv;

        enableFade.prepare (fs, initiallyEnabled);
        modeFade.prepare (fs, initiallyGsm);
        mixSmooth.reset (fs, 0.02);
        mixSmooth.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, initialMix01));
        agcSmooth.reset (fs, 0.02);
        agcSmooth.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, initialAgc01));
    }

    // gsm_destroy is safe here (guarded); prepare recreates. processSample
    // guards on null handles (skips codec work, zero frames).
    void releaseHandles() noexcept { destroyHandles(); }

    int getDelaySamples() const noexcept { return length; }

    // Per-block snapshot.
    void setParams (bool enabled, bool gsmMode, float mix01, float agc01) noexcept
    {
        enableFade.setEnabled (enabled);
        modeFade.setEnabled (gsmMode);
        mixSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, mix01));
        agcSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, agc01));
    }

    // Per-sample stereo, in place. No RNG anywhere in this stage.
    //
    // packetLost is the packet stage's CURRENT loss flag for THIS sample,
    // already ANDed with PACKET_ENABLE by the caller. It is consumed only on
    // the sample that closes a GSM frame — the frame-indexed lookup that
    // couples the two 20 ms grids without a shared counter (item 7).
    void processSample (float& left, float& right, bool packetLost) noexcept
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

            // The PLC gain glides across the frame it applies to. A -3 dB
            // STEP at every 20 ms boundary would click; 160 grid periods of
            // linear ramp is the same 20 ms the fades everywhere else use.
            const float pg = plcPrev + (plcTarget - plcPrev)
                                           * ((float) frameSlot * (1.0f / (float) kFrameLen));
            gsmHeld = gsmToFloat (prevDecoded[frameSlot]) * pg;   // previous frame plays

            if (++frameSlot >= kFrameLen)
            {
                frameSlot = 0;
                plcPrev   = plcTarget;

                if (encState != nullptr && decState != nullptr && g > 0.0f && w > 0.0f)
                {
                    // The encoder is at the FAR end of the link and always
                    // runs on the source frame — loss happens in transit, so
                    // encoder state must keep tracking the programme even
                    // through a burst.
                    gsm_encode (encState, frameIn, frameBytes);

                    if (packetLost)
                    {
                        // ITEM 7. The frame never arrived. GSM 06.11
                        // substitution-and-muting: decode the last frame that
                        // DID arrive, again. The decoder's own LTP history
                        // still evolves on each repeat, which is where the
                        // metallic warble comes from — it is not a buffer
                        // being looped.
                        ++lostFrameRun;

                        if (havePrevFrame && lostFrameRun < kPlcMuteFrames)
                        {
                            const int err = gsm_decode (decState, prevFrameBytes, prevDecoded);
                            juce::ignoreUnused (err);
                            jassert (err == 0);  // prevFrameBytes is our own encoder's output

                            // Repeat 1 plays at unity (it IS the last good
                            // frame); every further repeat drops 3 dB.
                            plcTarget = lostFrameRun <= 1 ? 1.0f
                                                          : plcTarget * kPlcAttenPerRepeat;
                        }
                        else
                        {
                            // Nothing to repeat yet, or past the 320 ms mute
                            // point — the spec mutes rather than warbling on
                            // forever.
                            std::fill (std::begin (prevDecoded), std::end (prevDecoded),
                                       (gsm_signal) 0);
                            plcTarget = 0.0f;
                        }
                    }
                    else
                    {
                        const int err = gsm_decode (decState, frameBytes, prevDecoded);
                        juce::ignoreUnused (err);
                        jassert (err == 0);      // cannot fail on our own frames

                        std::copy (std::begin (frameBytes), std::end (frameBytes),
                                   std::begin (prevFrameBytes));
                        havePrevFrame = true;
                        lostFrameRun  = 0;
                        plcTarget     = 1.0f;
                    }
                }
                else
                {
                    // GSM side inaudible: keep the output frame silent so
                    // (re-)engagement is deterministic (<= 20 ms of silence,
                    // covered by the 10 ms fades). The PLC state resets with
                    // it — a stale retained frame must not surface 30 seconds
                    // later on the first loss after re-engagement.
                    std::fill (std::begin (prevDecoded), std::end (prevDecoded),
                               (gsm_signal) 0);
                    havePrevFrame = false;
                    lostFrameRun  = 0;
                    plcPrev       = 1.0f;
                    plcTarget     = 1.0f;
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

        // 4b. ITEM 16 — feed-forward AGC on the codec sub-path. The envelope
        //     runs unconditionally (codecMono is computed every sample), so
        //     CODEC_AGC changes only the gain applied, never the state: the
        //     determinism convention holds and the knob is free of history.
        //
        //     Gain is assembled in dB so CODEC_AGC can scale it linearly and
        //     land on EXACTLY exp(0) = 1.0f at depth 0 — a bit-transparent
        //     multiply, not an approximately-unity one. log10 is evaluated
        //     only above threshold.
        const float depth = agcSmooth.getNextValue();
        const float rect  = std::abs (codecMono);
        agcEnv = rect + (rect > agcEnv ? agcAtt : agcRel) * (agcEnv - rect);

        float agcDb = kAgcMakeupDb;
        if (agcEnv > kAgcThreshLin)
            agcDb += (kAgcThreshDb - 20.0f * std::log10 (agcEnv)) * kAgcSlope;

        const float driven = codecMono * std::exp (agcDb * depth * kLn10Over20);

        float post = post1.processSample (driven);
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

    static float muLawRoundTrip (float x) noexcept
    {
        const int pcm = juce::jlimit (-32768, 32767,
                                      juce::roundToIntAccurate ((double) juce::jlimit (-1.0f, 1.0f, x)
                                                                * 32768.0));

        return g711::kMuDecode[(std::size_t) g711::linearToMuLaw (pcm)];
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

    // ITEM 7 — codec-domain frame loss. prevFrameBytes is the last frame that
    // ARRIVED (never overwritten by a lost one), so a burst re-decodes the
    // same 33 bytes repeatedly while the decoder's state drifts.
    static constexpr float kPlcAttenPerRepeat = 0.70794578f;   // -3 dB
    static constexpr int   kPlcMuteFrames     = 16;            // GSM 06.11, 320 ms

    gsm_frame prevFrameBytes = {};
    bool      havePrevFrame  = false;
    int       lostFrameRun   = 0;
    float     plcPrev        = 1.0f;
    float     plcTarget      = 1.0f;

    // ITEM 16 — codec AGC.
    static constexpr double kAgcAttackSec  = 0.001;
    static constexpr double kAgcReleaseSec = 0.050;
    static constexpr float  kAgcThreshDb   = -20.0f;
    static constexpr float  kAgcThreshLin  = 0.1f;             // 10^(-20/20)
    static constexpr float  kAgcSlope      = 0.75f;            // 1 - 1/4  (4:1)
    static constexpr float  kAgcMakeupDb   = 10.0f;            // unity near -6.7 dBFS
    static constexpr float  kAgcUnityEnv   = 0.46415888f;      // 10^((-20 + 10/0.75)/20)
    static constexpr float  kLn10Over20    = 0.11512925f;

    float agcAtt = 0.0f;
    float agcRel = 0.0f;
    float agcEnv = 0.0f;

    EnableFade enableFade;
    EnableFade modeFade;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> agcSmooth;
};
