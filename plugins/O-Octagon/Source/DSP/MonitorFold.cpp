/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#include "MonitorFold.h"

#include <cmath>

namespace oo
{

namespace
{
    constexpr float kPi     = 3.14159265358979f;
    constexpr float kHalfPi = kPi * 0.5f;

    /// The snapshot is already sanitised by publishSnapshot(), which is the single funnel for
    /// everything the audio thread reads about the room (P29 / H5). This is the belt to that
    /// braces: the fold derives NEW quantities — a reciprocal, an arc sine, a normalised ratio —
    /// and a value that was merely finite going in can still leave a division non-finite.
    inline float sane (float v, float fallback) noexcept
    {
        return std::isfinite (v) ? v : fallback;
    }
}

//==============================================================================
float MonitorFold::woodworthSeconds (float azimuthRad) noexcept
{
    // Odd in θ, so fold to a magnitude and restore the sign at the end.
    const float sign = azimuthRad < 0.0f ? -1.0f : 1.0f;
    const float a    = std::fabs (juce::jlimit (-kPi, kPi, azimuthRad));

    // f(θ) = θ + sin θ up to π/2, then (π − θ) + sin θ. Both give π/2 + 1 at the seam, so the
    // curve has no corner there, and f(π) = 0 — a source directly behind has no inter-aural delay,
    // which a naive clamp at π/2 would get wrong by the full 0.66 ms.
    const float f = a <= kHalfPi ? (a + std::sin (a))
                                 : ((kPi - a) + std::sin (a));

    return sign * (kHeadRadiusM / plane::kSpeedOfSoundMps) * f;
}

//==============================================================================
void MonitorFold::prepare (double sampleRateToUse, const VenueSnapshot& snapshot)
{
    sampleRate = sampleRateToUse;

    const int lineSamples = juce::jmax (4, (int) std::ceil (kLineSeconds * sampleRateToUse));

    // The ceiling the jlimit in fold() rails to. Held as a member beside the allocation it
    // describes so the two cannot disagree — setDelay() jasserts in Debug and silently jlimits in
    // Release into a delay quietly shorter than the geometry asked for (the alignDelay note).
    maxDelaySamples = (float) lineSamples - 1.0f;

    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = sampleRateToUse;
    spec.maximumBlockSize = 512;      // unused by these processors; required by the struct
    spec.numChannels      = 1;

    for (auto& line : itd)
    {
        line.setMaximumDelayInSamples (lineSamples);
        line.prepare (spec);
        line.reset();
    }

    // MANDATORY, not advisable — see the header. An unprepared FirstOrderTPTFilter decays from 2.0.
    for (int i = 0; i < kNumSpeakers; ++i)
    {
        const auto k = (std::size_t) i;
        shadowL[k].prepare (spec);
        shadowR[k].prepare (spec);
        shadowL[k].reset();
        shadowR[k].reset();
    }

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        const auto k = (std::size_t) i;
        gainL[k].reset  (sampleRateToUse, kFadeSeconds);
        gainR[k].reset  (sampleRateToUse, kFadeSeconds);
        delayL[k].reset (sampleRateToUse, kFadeSeconds);
        delayR[k].reset (sampleRateToUse, kFadeSeconds);
    }

    mix.reset (sampleRateToUse, kFadeSeconds);

    // prepare() is a state reset: the monitor cannot survive a sample-rate change armed, because
    // every ramp below is about to be teleported to a target derived at the NEW rate and a
    // half-faded crossfade would resume against coefficients from the old one.
    engaged = false;
    mix.setCurrentAndTargetValue (0.0f);

    // Force the derivation — a snapshot whose generation happens to equal the stale
    // lastGeometryGeneration would otherwise leave every ramp at zero and the fold silent.
    haveGeometry           = false;
    lastGeometryGeneration = 0;

    updateGeometry (snapshot);
    teleportRamps();
}

//==============================================================================
void MonitorFold::teleportRamps() noexcept
{
    for (int i = 0; i < kNumSpeakers; ++i)
    {
        const auto k = (std::size_t) i;
        gainL[k].setCurrentAndTargetValue  (gainL[k].getTargetValue());
        gainR[k].setCurrentAndTargetValue  (gainR[k].getTargetValue());
        delayL[k].setCurrentAndTargetValue (delayL[k].getTargetValue());
        delayR[k].setCurrentAndTargetValue (delayR[k].getTargetValue());
    }
}

//==============================================================================
void MonitorFold::updateGeometry (const VenueSnapshot& snapshot) noexcept
{
    // The ROOM moved, or this is the first derivation. The source did not — the source reaches
    // this class through the eight lane values fold() reads, never through a parameter.
    if (haveGeometry && snapshot.generation == lastGeometryGeneration)
        return;

    lastGeometryGeneration = snapshot.generation;
    haveGeometry           = true;

    // ── THE LISTENER ─────────────────────────────────────────────────────────────────────────
    //
    // The audience centroid, at ear height on the RAKED plane — plane::earHeight() is the same
    // function the hull attenuation and the air filter already reference, so the monitor cannot
    // disagree with the rest of the plugin about how high a listener's ears are.
    //
    // NO NEW VENUE FIELD. The venue schema stays at 2. A stored listener position was considered
    // and rejected: it is another 3 values to migrate, another row in the venue UI, and the
    // centroid is the point the DBAP solve is already implicitly referenced to.
    const float lx = sane (snapshot.centroid.x, 0.0f);
    const float ly = sane (snapshot.centroid.y, 0.0f);
    const float lz = sane (plane::earHeight (snapshot.rakeFront, snapshot.rakeRear,
                                             snapshot.bbMinY, snapshot.bbMaxY, ly),
                           0.0f);

    // Reference radius: the mean speaker distance. Dividing by it makes the fold's overall level
    // independent of hall size — a 40 m arena and a 6 m studio both put their average speaker at
    // unity — which is what stops the monitor from being inaudible in one venue and clipping in
    // the next.
    float sumR = 0.0f;

    std::array<float, kNumSpeakers> radius {};
    std::array<float, kNumSpeakers> azimuth {};
    std::array<float, kNumSpeakers> elevation {};

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        const auto k = (std::size_t) i;

        const float dx = sane (snapshot.spk[k].x, 0.0f) - lx;
        const float dy = sane (snapshot.spk[k].y, 0.0f) - ly;
        const float dz = sane (snapshot.spk[k].z, 0.0f) - lz;

        const float r = juce::jmax (kMinRadiusM, std::sqrt (dx * dx + dy * dy + dz * dz));

        radius[k] = r;
        sumR     += r;

        // AZIMUTH IS MEASURED FROM THE STAGE, WHICH IS −Y.
        //
        // plane::earHeight() interpolates rakeFront at bbMinY to rakeRear at bbMaxY, so low Y is
        // the front of the hall and the listener faces −Y. atan2(dx, −dy) therefore gives 0 at the
        // stage, +π/2 to the listener's right and ±π directly behind. Getting this backwards
        // mirrors the entire fold left-for-right and is inaudible without a reference — which is
        // why it is written down here rather than left to the reader of the atan2.
        azimuth[k]   = std::atan2 (dx, -dy);
        elevation[k] = std::asin (juce::jlimit (-1.0f, 1.0f, dz / r));
    }

    const float refR = juce::jmax (kMinRadiusM, sumR / (float) kNumSpeakers);

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        const auto k = (std::size_t) i;

        const float az = azimuth[k];
        const float el = elevation[k];

        const float cosEl = std::cos (el);

        // ── PAN: constant power on the LATERAL component ─────────────────────────────────────
        //
        // lat = sin(az)·cos(el) ∈ [−1, +1]: +1 hard right, −1 hard left, 0 for anything on the
        // median plane — which correctly includes a speaker directly BEHIND. That front/back
        // collapse is the documented limitation of an ITD/ILD model, not an error here.
        const float lat = juce::jlimit (-1.0f, 1.0f, std::sin (az) * cosEl);

        // COMPRESSED BY kPanDepth BEFORE THE PAN — see the header. Without this the far ear is
        // exactly 0 at lat = +/-1 and the ITD and head shadow below become unreachable code for
        // every lateral source.
        const float pan = (lat * kPanDepth + 1.0f) * 0.5f;   // 0 = hard left, 1 = hard right

        const float gL = std::cos (pan * kHalfPi);
        const float gR = std::sin (pan * kHalfPi);

        // ── DISTANCE ─────────────────────────────────────────────────────────────────────────
        const float dist = juce::jlimit (kGainFloor, kGainCeil, refR / radius[k]);

        gainL[k].setTargetValue (sane (gL * dist * kFoldTrim, 0.0f));
        gainR[k].setTargetValue (sane (gR * dist * kFoldTrim, 0.0f));

        // ── ITD ──────────────────────────────────────────────────────────────────────────────
        //
        // Scaled by cos(elevation): a speaker directly overhead is equidistant from both ears
        // whatever its azimuth, and the cone-of-confusion collapse is the correct behaviour.
        const float itdSec = woodworthSeconds (az) * cosEl;
        const float itdSmp = sane (itdSec * (float) sampleRate, 0.0f);

        // Positive itd means the source is to the RIGHT, so the LEFT ear is the far one and gets
        // the delay. Both ears are railed non-negative, then railed again to the allocated ceiling
        // — setDelay() jasserts in Debug on anything outside [0, max].
        const float dL = juce::jlimit (0.0f, maxDelaySamples, juce::jmax (0.0f,  itdSmp));
        const float dR = juce::jlimit (0.0f, maxDelaySamples, juce::jmax (0.0f, -itdSmp));

        delayL[k].setTargetValue (dL);
        delayR[k].setTargetValue (dR);

        // ── HEAD SHADOW ──────────────────────────────────────────────────────────────────────
        //
        // shadowAmount is 0 for the near ear and 1 for the far one, interpolated geometrically
        // between an open 18 kHz and a shadowed 2.5 kHz — geometric rather than linear because
        // cutoff is perceived logarithmically, so a linear sweep spends most of its travel in the
        // top octave where it is least audible.
        const float shadowAmountL = juce::jlimit (0.0f, 1.0f, pan);
        const float shadowAmountR = 1.0f - shadowAmountL;

        // The pinna, approximated: a source behind is darker than the same source in front. It is
        // the ONLY front/back cue this model has, and it is a hint rather than a resolution.
        const float rear       = juce::jmax (0.0f, -std::cos (az) * cosEl);
        const float rearFactor = 1.0f + (kRearDarkenRatio - 1.0f) * rear;

        const float ratio = kShadowCutoffHz / kOpenCutoffHz;

        // Nyquist is a HARD ceiling for a TPT one-pole: prewarping tan(π·fc/fs) blows up as fc
        // approaches fs/2. 18 kHz already exceeds it at 44.1 kHz, so this clamp is load-bearing at
        // the most common rate rather than a defensive flourish.
        const float nyqCap = (float) (sampleRate * 0.45);

        const float fcL = juce::jlimit (200.0f, nyqCap,
                                        kOpenCutoffHz * std::pow (ratio, shadowAmountL) * rearFactor);
        const float fcR = juce::jlimit (200.0f, nyqCap,
                                        kOpenCutoffHz * std::pow (ratio, shadowAmountR) * rearFactor);

        shadowL[k].setCutoffFrequency (sane (fcL, 1000.0f));
        shadowR[k].setCutoffFrequency (sane (fcR, 1000.0f));
    }
}

//==============================================================================
void MonitorFold::setEngaged (bool shouldBeEngaged) noexcept
{
    if (shouldBeEngaged == engaged)
        return;

    engaged = shouldBeEngaged;

    // The false -> true edge clears the lines and the filters. A network that last ran before the
    // operator took the headphones off is holding audio from a different musical moment, and a
    // bypass that lasted minutes has no continuity worth preserving — the decorrEngaged argument,
    // and the alignDelay one before it.
    //
    // Safe to do HERE and not one chunk later because the crossfade starts from 0 on this edge:
    // the first samples out of a just-cleared line are multiplied by a mix that has not yet left
    // zero, so the clear is inaudible by construction.
    if (engaged)
    {
        for (auto& line : itd)
            line.reset();

        for (int i = 0; i < kNumSpeakers; ++i)
        {
            shadowL[(std::size_t) i].reset();
            shadowR[(std::size_t) i].reset();
        }
    }

    mix.setTargetValue (engaged ? 1.0f : 0.0f);
}

//==============================================================================
void MonitorFold::fold (float* const* out, int slotL, int slotR, int start, int count) noexcept
{
    // The caller resolved these on the message thread and published them. An unresolved pair is
    // {-1,-1} and the monitor is refused upstream — this is the backstop, not the gate.
    if (out == nullptr || slotL < 0 || slotR < 0 || slotL >= kNumSpeakers || slotR >= kNumSpeakers
        || slotL == slotR)
        return;

    const int last = start + count;

    // Tracked for the per-chunk NaN guard. The LAST value, never a running max: juce::jmax is
    // `a < b ? b : a` and `worst < NaN` is false, so a max SILENTLY DISCARDS the NaN it exists to
    // catch. Poisoning here is sticky by construction for the shadow filters exactly as it is for
    // the air filter — s = y + v re-derives s from a value that is already non-finite.
    float lastMonL = 0.0f, lastMonR = 0.0f;

    for (int n = start; n < last; ++n)
    {
        // ── READ ALL EIGHT BEFORE WRITING ANY ────────────────────────────────────────────────
        //
        // out[slotL] and out[slotR] are among the eight, so this is the H7 aliasing rule in its
        // sharpest form yet: writing the monitor pair before the other six have been read would
        // fold the pair's own output back into the sum. The plausible "optimisation" that writes
        // as it goes is wrong for every speaker whose slot is below slotR.
        float y[kNumSpeakers];

        for (int i = 0; i < kNumSpeakers; ++i)
            y[i] = out[i][n];

        const float m = mix.getNextValue();

        float sumL = 0.0f, sumR = 0.0f;

        for (int i = 0; i < kNumSpeakers; ++i)
        {
            const auto k = (std::size_t) i;

            // ADVANCED UNCONDITIONALLY, all four, for the reason the seventeen and the eight
            // alignment ramps are: fold() is skipped entirely between engagements, and prepare()
            // is the only place a ramp is ever teleported. Branching on `m` here would freeze a
            // ramp mid-fade and resume it from a stale currentValue on the next engage.
            const float aL = gainL[k].getNextValue();
            const float aR = gainR[k].getNextValue();
            const float dL = delayL[k].getNextValue();
            const float dR = delayR[k].getNextValue();

            itd[k].pushSample (0, y[i]);

            // ONE LINE, TWO READS. The near ear reads WITHOUT advancing readPos; the far ear's
            // read advances it. Reversing the order would advance the pointer under the second
            // read and shift that ear by a sample — a 20 µs error, inaudible as a delay and
            // audible as a comb.
            const float nearSmp = itd[k].popSample (0, juce::jlimit (0.0f, maxDelaySamples, dL), false);
            const float farSmp  = itd[k].popSample (0, juce::jlimit (0.0f, maxDelaySamples, dR), true);

            sumL += aL * shadowL[k].processSample (0, nearSmp);
            sumR += aR * shadowR[k].processSample (0, farSmp);
        }

        lastMonL = sumL;
        lastMonR = sumR;

        // ── WRITE: two folded, six silent, all crossfaded ────────────────────────────────────
        //
        // LERP, NOT (1−m)·dry + m·wet. At m == 0 `y + 0·(f − y)` IS y — the v1.5.0 lane reached by
        // arithmetic that cannot round away from it — so a settled-but-not-yet-disengaged chunk is
        // bit-transparent rather than transparent to within an ulp. The complementary form's first
        // product is y·1.0f, which is also exact but only while the compiler keeps the multiply,
        // and -ffast-math is free not to. (decorrMix's argument, reused verbatim.)
        for (int i = 0; i < kNumSpeakers; ++i)
        {
            const float target = i == slotL ? sumL
                               : i == slotR ? sumR
                                            : 0.0f;

            out[i][n] = y[i] + m * (target - y[i]);
        }

        instr::countMonitorSample();
    }

    // Both chains are cleared when EITHER poisons: they are a matched pair whose entire purpose is
    // to differ from each other, and restarting one against filters the other has been running for
    // minutes would leave the pair correlated in exactly the way the fold exists to avoid. The
    // delay lines go with them — a ring holding a non-finite sample re-poisons on its next read,
    // and popSample's interpolation multiplies it by a fraction that may be 0.0f, which makes the
    // result NaN rather than clean.
    if (! std::isfinite (lastMonL) || ! std::isfinite (lastMonR))
    {
        for (int i = 0; i < kNumSpeakers; ++i)
        {
            shadowL[(std::size_t) i].reset();
            shadowR[(std::size_t) i].reset();
            itd[(std::size_t) i].reset();
        }
    }
}

} // namespace oo
