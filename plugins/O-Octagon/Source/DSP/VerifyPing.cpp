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
#include "VerifyPing.h"

#include <cmath>

namespace oo
{

namespace
{
    constexpr float kPi = 3.14159265358979323846f;

    /** 20 ms raised cosine, t in [0, 1]. Reaches exactly 0 and exactly 1 at the ends, which is what
        makes a state reset unnecessary at either boundary. */
    inline float raisedCosine (float t) noexcept
    {
        return 0.5f - 0.5f * std::cos (kPi * juce::jlimit (0.0f, 1.0f, t));
    }
} // namespace

//==============================================================================
void VerifyPing::prepare (double sampleRateToUse)
{
    sampleRate = sampleRateToUse > 0.0 ? sampleRateToUse : 48000.0;

    // SAMPLES, from the prepared rate. Never a transcribed 614400.
    const auto toSamples = [this] (double seconds)
    {
        return juce::jmax (1, static_cast<int> (std::lround (seconds * sampleRate)));
    };

    onSamples    = toSamples (kOnSeconds);
    gapSamples   = toSamples (kGapSeconds);
    latchSamples = toSamples (kLatchSeconds);
    fadeSamples  = toSamples (kFadeSeconds);

    // The fades live INSIDE the 1.2 s on-segment, so 8 x (on + gap) is exactly 12.8 s regardless of
    // the fade length. A fade added on top would make probe BS's arithmetic wrong by 320 ms.
    fadeSamples = juce::jmin (fadeSamples, onSamples / 2);

    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = 1;      // processSample only; the block size is irrelevant to a TPT
    spec.numChannels      = 1;

    // prepare() on a FirstOrderTPTFilter is MANDATORY, not advisable: the default `s1` is the
    // initializer-list vector { 2 } — size 1, holding 2.0f — so an unprepared filter decays from 2.0
    // on its first outputs (RESEARCH-2.3 H6, the same trap GainStage documents).
    hp.prepare (spec);
    lp.prepare (spec);

    hp.setType (juce::dsp::FirstOrderTPTFilterType::highpass);
    hp.setCutoffFrequency (200.0f);

    lp.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
    lp.setCutoffFrequency (juce::jmin (8000.0f, static_cast<float> (0.45 * sampleRate)));

    // This class's own state, initialised HERE and nowhere else — the same discipline
    // GainStage::prepare() states for the seventeen smoothers. Nothing below ever calls reset().
    b0 = b1 = b2 = 0.0f;

    phase        = Phase::idle;
    phaseCounter = 0;
    runCounter   = 0;
    speaker      = 0;
    mode         = 0;

    command.store (kCmdNone, std::memory_order_release);
    activeFlag.store (false, std::memory_order_release);
    publish();
}

//==============================================================================
void VerifyPing::start (int speakerOrAuto)
{
    if (speakerOrAuto != kAuto && (speakerOrAuto < 1 || speakerOrAuto > kNumSpeakers))
        return;

    // Order matters: the command must be visible before the audio thread is allowed in, or the
    // first overwrite() would consume kCmdNone and render nothing while isActive() reads true.
    command.store (speakerOrAuto, std::memory_order_release);
    activeFlag.store (true, std::memory_order_release);
}

void VerifyPing::stop()
{
    // GRACEFUL. activeFlag stays true so the audio thread is still called and can run the release;
    // it clears the flag itself once the envelope has reached zero.
    command.store (kCmdStop, std::memory_order_release);
}

void VerifyPing::abort()
{
    // ATOMICS ONLY. This is what makes the call safe from the message thread (editor destructor)
    // AND from the audio thread (bypass, and the mapped -> not-mapped flip) without a lock and
    // without touching a single audio-thread-private member. The stale phase state left behind is
    // irrelevant: beginRun() re-initialises all of it.
    command.store (kCmdAbort, std::memory_order_release);
    activeFlag.store (false, std::memory_order_release);

    modeOut.store (0, std::memory_order_release);
    speakerOut.store (0, std::memory_order_release);
    elapsedMsOut.store (0, std::memory_order_release);
    remainingMsOut.store (0, std::memory_order_release);
}

//==============================================================================
VerifyPing::State VerifyPing::getState() const noexcept
{
    State s;
    s.active      = activeFlag.load (std::memory_order_acquire);
    s.mode        = modeOut.load (std::memory_order_acquire);
    s.speaker     = speakerOut.load (std::memory_order_acquire);
    s.elapsedMs   = elapsedMsOut.load (std::memory_order_acquire);
    s.remainingMs = remainingMsOut.load (std::memory_order_acquire);
    return s;
}

void VerifyPing::publish() noexcept
{
    const bool live = phase != Phase::idle;

    modeOut.store (live ? mode : 0, std::memory_order_release);

    // The speaker is 0 during a gap, so a UI that lights the returned index goes dark between
    // steps rather than holding the previous one lit through 400 ms of silence.
    speakerOut.store (phase == Phase::gap || ! live ? 0 : speaker, std::memory_order_release);

    const double msPerSample = sampleRate > 0.0 ? 1000.0 / sampleRate : 0.0;
    const int total = mode == 2 ? juce::jmax (1, kNumSpeakers * (onSamples + gapSamples)) : latchSamples;

    elapsedMsOut.store (static_cast<int> (runCounter * msPerSample), std::memory_order_release);
    remainingMsOut.store (live ? static_cast<int> (juce::jmax (0, total - runCounter) * msPerSample) : 0,
                          std::memory_order_release);
}

//==============================================================================
void VerifyPing::beginRun (int target) noexcept
{
    mode         = target == kAuto ? 2 : 1;
    speaker      = target == kAuto ? 1 : target;
    phase        = Phase::fadeIn;
    phaseCounter = 0;
    runCounter   = 0;
}

//==============================================================================
float VerifyPing::nextSample() noexcept
{
    // White, U(-1, 1). nextFloat() is [0, 1).
    const float w = rng.nextFloat() * 2.0f - 1.0f;

    // The fixed-coefficient pinking network (§OQ2). Its output RMS is a PROPERTY OF THESE
    // COEFFICIENTS, which is why kPinkNormScalar was measured rather than chosen.
    b0 = 0.99765f * b0 + w * 0.0990460f;
    b1 = 0.96300f * b1 + w * 0.2965164f;
    b2 = 0.57000f * b2 + w * 1.0526913f;

    const float pink = b0 + b1 + b2 + w * 0.1848f;

    // 200 Hz HP then 8 kHz LP: a band that is unmistakably present in a hall and unmistakably not
    // the programme material, and that no PA subwoofer crossover can steal.
    const float shaped = lp.processSample (0, hp.processSample (0, pink));

    return shaped * kPinkNormScalar;
}

//==============================================================================
void VerifyPing::overwrite (float* const* out, int numSpeakers, int start, int count) noexcept
{
    // ── Consume the pending command. exchange() so a command posted between two blocks is seen
    //    exactly once, without the message thread ever reading audio-thread state. ───────────────
    const int cmd = command.exchange (kCmdNone, std::memory_order_acq_rel);

    if (cmd == kCmdAbort)
    {
        phase = Phase::idle;
        activeFlag.store (false, std::memory_order_release);
        publish();
        return;                                     // NOTHING is written — normal audio stands
    }

    if (cmd == kCmdStop)
    {
        if (phase != Phase::idle && phase != Phase::fadeOut)
        {
            phase        = Phase::fadeOut;
            phaseCounter = 0;
        }
    }
    else if (cmd == kAuto || (cmd >= 1 && cmd <= kNumSpeakers))
    {
        beginRun (cmd);
    }

    if (phase == Phase::idle)
    {
        activeFlag.store (false, std::memory_order_release);
        publish();
        return;
    }

    const int lanes = juce::jmin (numSpeakers, kNumSpeakers);
    const int last  = start + count;

    // HOISTED. decibelsToGain is a pow() and this is a constant for the life of the class; per
    // sample it would be the most expensive thing in the routine by an order of magnitude.
    const float ceiling = juce::Decibels::decibelsToGain (kPeakCeilDb);

    for (int n = start; n < last; ++n)
    {
        float env = 0.0f;

        switch (phase)
        {
            case Phase::fadeIn:
                env = raisedCosine (static_cast<float> (phaseCounter) / static_cast<float> (fadeSamples));
                break;
            case Phase::sustain:
                env = 1.0f;
                break;
            case Phase::fadeOut:
                env = raisedCosine (1.0f - static_cast<float> (phaseCounter) / static_cast<float> (fadeSamples));
                break;
            case Phase::gap:
            case Phase::idle:
            default:
                env = 0.0f;
                break;
        }

        // The generator runs even through the gap. Stopping and restarting it would put a
        // deterministic transient at every step boundary, which is precisely what the envelope
        // exists to remove.
        const float s = nextSample() * env;

        // HARD CEILING, not a limiter. Measured crest is 4.21, so at -20 dBFS RMS the steady-state
        // peak sits near -7.5 dBFS and this clamps essentially nothing — it exists so that a future
        // change to the network cannot quietly raise what leaves the PA.
        const float y = juce::jlimit (-ceiling, ceiling, s);

        // ── EXACTLY ONE LANE SOUNDS; THE OTHER SEVEN ARE EXACT ZERO ─────────────────────────────
        // Not "attenuated" — zero. Probe BQ asserts the seven against 0.0f rather than against a
        // threshold, on a NON-IDENTITY map, which is what makes it a test of speakerToBuffer rather
        // than of whatever out[i] happened to be.
        for (int i = 0; i < lanes; ++i)
            out[i][n] = (i + 1) == speaker ? y : 0.0f;

        ++phaseCounter;
        ++runCounter;

        switch (phase)
        {
            case Phase::fadeIn:
                if (phaseCounter >= fadeSamples) { phase = Phase::sustain; phaseCounter = 0; }
                break;

            case Phase::sustain:
                // The fades live INSIDE the on-segment: fadeIn + sustain + fadeOut == onSamples.
                if (mode == 2 && phaseCounter >= onSamples - 2 * fadeSamples)
                {
                    phase = Phase::fadeOut;
                    phaseCounter = 0;
                }
                else if (mode == 1 && runCounter >= latchSamples - fadeSamples)
                {
                    // THE 120 s SELF-STOP (D11, ROADMAP orphan 1). Sample-counted, so probe BT can
                    // measure it offline; a juce::Timer could not be measured at all.
                    phase = Phase::fadeOut;
                    phaseCounter = 0;
                }
                break;

            case Phase::fadeOut:
                if (phaseCounter >= fadeSamples)
                {
                    // EVERY on-segment is followed by its gap, INCLUDING speaker 8's — that is what
                    // makes the cycle exactly 8 x (on + gap) rather than 8 x on + 7 x gap, which
                    // would come to 12.4 s and fail probe BS by one gap.
                    phase        = mode == 2 ? Phase::gap : Phase::idle;
                    phaseCounter = 0;
                }
                break;

            case Phase::gap:
                if (phaseCounter >= gapSamples)
                {
                    if (speaker >= kNumSpeakers)
                    {
                        phase = Phase::idle;     // 1 -> 8 complete, at exactly 12.8 s
                        phaseCounter = 0;
                    }
                    else
                    {
                        ++speaker;
                        phase = Phase::fadeIn;
                        phaseCounter = 0;
                    }
                }
                break;

            case Phase::idle:
            default:
                break;
        }

        if (phase == Phase::idle)
        {
            // Zero the remainder of the chunk so a run that ends mid-block leaves silence rather
            // than the previous sample's value smeared across the tail.
            for (int m = n + 1; m < last; ++m)
                for (int i = 0; i < lanes; ++i)
                    out[i][m] = 0.0f;

            break;
        }
    }

    if (phase == Phase::idle)
        activeFlag.store (false, std::memory_order_release);

    publish();
}

} // namespace oo
