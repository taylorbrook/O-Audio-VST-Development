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

    O-Bitrot - RotStage (v1.10.0, improvement brief item 8)

    The literal bitrot the plugin is named for, and the seventh clocked family.
    Research §3.5 lists four corrupt-file mechanisms; the engine already had the
    fourth (buffer shuffling, via the read-head machinery) and none of the other
    three. This class is those three:

      FLIP    — a rate-limited window over which individual samples are XORed in
                the 16-bit domain and clipped. One flipped bit is one impulse:
                low bits are a faint granular crackle, high bits are full-scale
                spikes. This is the sound of a PCM file with bad blocks.
      STICK   — the decoder hangs and one sample is held for 10-80 ms. A short
                DC plateau, which is exactly what a stalled playback pointer
                emits.
      GARBLE  — a wrong-decode stretch: the decoder is reading at the wrong byte
                offset, so it emits garbage AT THE PROGRAMME'S OWN LEVEL for
                30-300 ms. The envelope match is the whole artifact — noise at a
                fixed level reads as a fault in the plugin, noise that swells
                and ducks with the music reads as a corrupt file.

    CLASS: pure OVERLAY (v1.9.0 arbitration vocabulary). Rot changes no head
    position and no transport rate — it is gain/artifact domain applied to the
    rendered wet pair — so it never contends for ownership of a tick and never
    touches the `arbitration` stream. It fires whenever its own roll succeeds,
    layering under a tape bend, a CD loop or a locked groove.

    CHAIN POSITION: after the transport render, the artifact bus and the media
    beds; UPSTREAM of the packet stage. Same argument the beds are placed by —
    rot lives on the stored medium, so a lost packet has to conceal it along
    with the programme, and by the time the file was written the hiss and the
    pops were part of the programme too.

    DRAW DISCIPLINE (all on the single `rot` stream; see RngBank for why one
    stream is enough here):
      * tick, family enabled ....... 1 gate draw (taken in Arbitration)
      * tick, roll succeeded ....... 2 kind draws + 1 duration draw
      * per sample, FLIP window .... 1 schedule draw, +2 more on a flip
      * per sample, GARBLE stretch . 2 draws (decorrelated channels)
      * per sample, STICK hold ..... 0 draws
      * per sample, idle ........... 0 draws
    Every one of those instants is a pure function of the absolute sample index,
    which is what makes the single stream block-size invariant (probe R4).

    TRANSPARENCY: while idle the only work done is the envelope follower, which
    writes to its own state and never to the signal, so the all-off FUNC-02 null
    stays bit-exact even with the ROT knobs turned up (probe R5). Disabling the
    family mid-event lets the running event FINISH — it is bounded at 300 ms and
    ends on its own fade, exactly as the CD conceal/mute rungs and the tape
    dropout do, rather than cutting to a step.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class RotStage
{
public:
    enum class Kind { Flip, Stick, Garble };

    // ── Event durations ────────────────────────────────────────────────────
    static constexpr double kFlipMinSec   = 0.020;
    static constexpr double kFlipMaxSec   = 0.200;
    static constexpr double kStickMinSec  = 0.010;   // brief item 8: 10-80 ms
    static constexpr double kStickMaxSec  = 0.080;
    static constexpr double kGarbleMinSec = 0.030;
    static constexpr double kGarbleMaxSec = 0.300;

    // Edge blend for the two REPLACEMENT kinds. 1.5 ms is short enough that a
    // sticky hold still reads as a hang rather than a fade, and long enough to
    // keep the entry step out of tweeter territory. HARD_EDGES sets it to 0 and
    // the entry becomes a true step — the same bypass the splice fades take.
    static constexpr double kEdgeSec      = 0.0015;

    // Flip schedule. DEPTH sweeps the rate exponentially between these, so the
    // knob's bottom is an occasional tick and its top is a dense digital hash.
    static constexpr double kFlipRateMinHz = 25.0;
    static constexpr double kFlipRateSpan  = 160.0;  // 25 Hz -> 4 kHz

    // The RATE LIMIT of "rate-limited window": whatever DEPTH and the sample
    // rate conspire to ask for, at most one sample in four is ever touched.
    // Without it a low sample rate would turn the top of the knob into solid
    // white noise, which is the garble kind's job, not this one's.
    static constexpr float  kFlipProbCap   = 0.25f;

    // Bit range XORed in the 16-bit domain, INCLUDING bit 15 — the sign.
    //
    // Excluding the sign bit was the first cut of this and it was wrong twice
    // over. Physically, a corrupt block does not know which bit is the sign;
    // every bit in the word is equally likely to rot. Numerically, excluding it
    // made the brief's "post-clip" unreachable: a single flip of bit <= 14 on a
    // word already inside +/-32767 can never leave +/-32768, so there was
    // nothing for a clip to do and the probe that claimed to gate it passed
    // with the clip deleted. Bit 15 is what takes the XOR result to +/-65535,
    // i.e. +/-2.0 FS, and that is what the clip is FOR (probe R1 asserts the
    // bound and fails without it).
    static constexpr int    kFlipBitMin    = 3;
    static constexpr int    kFlipBitMax    = 15;

    // Envelope follower for the garble kind. Fast enough to catch a transient,
    // slow enough that the noise does not chatter with the waveform.
    static constexpr double kEnvAttackSec  = 0.005;
    static constexpr double kEnvReleaseSec = 0.060;

    void prepare (double sampleRate) noexcept
    {
        fs = sampleRate;

        attCoef = 1.0f - std::exp (static_cast<float> (-1.0 / (kEnvAttackSec  * fs)));
        relCoef = 1.0f - std::exp (static_cast<float> (-1.0 / (kEnvReleaseSec * fs)));

        reset();
    }

    void reset() noexcept
    {
        eventT    = 0;
        eventDur  = 0;
        edgeLen   = 0;
        kind      = Kind::Flip;
        needLatch = false;
        heldL     = 0.0f;
        heldR     = 0.0f;
        flipProb  = 0.0f;
        flipBits  = kFlipBitMin;
        envSq     = 0.0f;
    }

    bool isActive() const noexcept { return eventT < eventDur; }

    /** Tick-time install. ALWAYS consumes EXACTLY ONE draw from the `rot`
        stream — including when an event is already running and the install is
        discarded — so the stream's pattern never depends on whether a previous
        event happened to still be in flight (the CDSkip/TapeDropout "the draw
        is already consumed" convention).

        One draw covers every kind because duration is the only randomised
        parameter all three share; the flip window's density comes from DEPTH,
        and the other two have no severity axis at all.

        A retrigger while active is DISCARDED rather than restarted: restarting
        a sticky hold would re-latch mid-plateau and step the DC, and restarting
        a garble would re-open its entry fade from full noise. */
    void trigger (juce::Random& rotStream, Kind k, double depth01, bool hardEdges) noexcept
    {
        const double r = rotStream.nextDouble();

        if (isActive())
            return;

        kind = k;

        const double minSec = k == Kind::Flip  ? kFlipMinSec
                            : k == Kind::Stick ? kStickMinSec
                                               : kGarbleMinSec;
        const double maxSec = k == Kind::Flip  ? kFlipMaxSec
                            : k == Kind::Stick ? kStickMaxSec
                                               : kGarbleMaxSec;

        eventDur = juce::jmax (1, static_cast<int> ((minSec + (maxSec - minSec) * r) * fs));
        eventT   = 0;

        // Edges are a property of the EVENT, so HARD_EDGES is latched here
        // rather than read per sample: a splice mode toggled mid-hold must not
        // strand a plateau with no way back down.
        edgeLen = hardEdges ? 0
                            : juce::jmin (static_cast<int> (kEdgeSec * fs), eventDur / 2);

        if (k == Kind::Flip)
        {
            const double d    = juce::jlimit (0.0, 1.0, depth01);
            const double rate = kFlipRateMinHz * std::pow (kFlipRateSpan, d);

            flipProb = juce::jmin (kFlipProbCap, static_cast<float> (rate / fs));

            // DEPTH also opens the bit field upward: at 0 only the bottom bits
            // are reachable (a faint granular crackle), and only at 1 does the
            // field reach the sign bit, where one flip in thirteen is a
            // polarity inversion that clips to full scale.
            flipBits = kFlipBitMin
                       + static_cast<int> (std::lround (d * (kFlipBitMax - kFlipBitMin)));
        }
        else if (k == Kind::Stick)
        {
            // The held value cannot be taken here — the tick is processed
            // BEFORE this sample's wet pair exists. Latch on the first
            // processSample of the event instead.
            needLatch = true;
        }
    }

    /** Per-sample, on the rendered wet pair. Exact no-op on the signal while
        idle: the follower writes only to its own state, so the FUNC-02 null
        holds with the ROT knobs at any setting (probe R5). */
    void processSample (juce::Random& rotStream, float& left, float& right) noexcept
    {
        // Runs EVERY sample, event or not, on the INCOMING (pre-rot) signal.
        // Two reasons it is not gated: a garble stretch has to match the level
        // the programme had when the stretch STARTED, so the follower must
        // already be tracking; and during a stretch the incoming pair is still
        // the live programme, so the match keeps following the music under the
        // noise instead of freezing at the onset level.
        const float sq = 0.5f * (left * left + right * right);
        envSq += (sq - envSq) * (sq > envSq ? attCoef : relCoef);

        if (! isActive())
            return;

        switch (kind)
        {
            case Kind::Flip:
            {
                // Rate-limited schedule: exactly one draw per sample so the
                // sequence does not depend on how many flips happened to land.
                if (rotStream.nextFloat() < flipProb)
                {
                    const int  bit   = rotStream.nextInt (flipBits + 1);
                    const bool onRight = rotStream.nextBool();

                    // ONE channel per flip. A corrupt block damages the bytes
                    // it covers, and in interleaved PCM that is one sample of
                    // one channel — which is also why the crackle wanders
                    // across the image instead of sitting in the middle.
                    float& target = onRight ? right : left;
                    target = flipSample (target, bit);
                }
                break;
            }

            case Kind::Stick:
            {
                if (needLatch)
                {
                    heldL     = left;
                    heldR     = right;
                    needLatch = false;
                }

                // Equal-GAIN blend between the live pair and the held sample.
                // Correct here precisely because the two are CORRELATED at the
                // entry instant — the held value IS the signal at t=0, so the
                // sum of the gains is what has to stay at 1
                // (pattern_hann_pair_is_equal_gain_not_equal_power).
                const float g = edgeBlend();
                left  += (heldL - left)  * g;
                right += (heldR - right) * g;
                break;
            }

            case Kind::Garble:
            {
                // Uniform noise in [-A, A] has RMS A/sqrt(3), so scaling by
                // sqrt(3) * rms puts the garbage at the programme's own RMS.
                // That is the "envelope-matched" claim, and probe R3 measures
                // it rather than taking it on faith.
                const float amp = 1.7320508f * std::sqrt (envSq);
                const float nL  = (rotStream.nextFloat() * 2.0f - 1.0f) * amp;
                const float nR  = (rotStream.nextFloat() * 2.0f - 1.0f) * amp;

                // Equal-POWER crossfade, unlike the sticky blend above: noise
                // and programme are UNCORRELATED, so their powers add and it is
                // sqrt(w) / sqrt(1-w) that holds the level flat across the
                // edge. An equal-gain pair would dip ~3 dB in the middle of
                // both fades.
                const float w  = edgeBlend();
                const float wn = std::sqrt (w);
                const float wd = std::sqrt (1.0f - w);

                left  = left  * wd + nL * wn;
                right = right * wd + nR * wn;
                break;
            }
        }

        ++eventT;
    }

private:
    /** 0 -> 1 -> 0 with flat-topped edges of edgeLen samples. edgeLen == 0
        (HARD_EDGES) makes it the constant 1: a true step in and a true step
        out. */
    float edgeBlend() const noexcept
    {
        if (edgeLen <= 0)
            return 1.0f;

        if (eventT < edgeLen)
            return static_cast<float> (eventT) / static_cast<float> (edgeLen);

        const int fromEnd = eventDur - eventT;
        if (fromEnd <= edgeLen)
            return static_cast<float> (fromEnd) / static_cast<float> (edgeLen);

        return 1.0f;
    }

    /** XOR one bit in the 16-bit two's-complement domain — the file as it sits
        on disk — then clip.

        The input clamp is not defensive padding: the conceit is that this
        signal LIVES in a 16-bit file, and a 16-bit file cannot hold 1.3, so a
        sample that hot is already saturated by the time a block goes bad.

        The output clip is the brief's "post-clip", and it earns its place only
        because bit 15 is in the field: flipping the sign moves the word by
        32768, which from anywhere inside +/-32767 reaches +/-65535 — 2.0 FS.
        A real decoder would WRAP instead, turning every over into a second
        polarity inversion; clipping keeps the artifact a spike whose ceiling is
        full scale, which is the impulse this kind is modelling. */
    static float flipSample (float x, int bit) noexcept
    {
        auto word = static_cast<juce::int32> (juce::jlimit (-1.0f, 1.0f, x) * 32767.0f);
        word ^= (1 << bit);
        return juce::jlimit (-1.0f, 1.0f, static_cast<float> (word) * (1.0f / 32767.0f));
    }

    double fs = 48000.0;

    float attCoef = 0.0f;
    float relCoef = 0.0f;
    float envSq   = 0.0f;

    Kind kind     = Kind::Flip;
    int  eventT   = 0;
    int  eventDur = 0;
    int  edgeLen  = 0;

    // Flip
    float flipProb = 0.0f;
    int   flipBits = kFlipBitMin;

    // Stick
    bool  needLatch = false;
    float heldL     = 0.0f;
    float heldR     = 0.0f;
};
