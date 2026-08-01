/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
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

    O-ReverseDelay - CaptureBuffer

    Stereo circular capture ring, written by input + damped feedback return.
    Adapted from O-GrainScatter's DelayBuffer.h with two contract changes
    (Stage-2 PLAN Task 2 / RESEARCH §3.1):

      * Lagrange interpolation DROPPED — v1.0 grain reads are integer-stepped
        (reverse speed is exactly 1.0, no pitch change).
      * Absolute-position bookkeeping ADDED — a monotonically increasing
        int64 `totalWritten` runs alongside the ring. A grain latches
        `readStartAbs = totalWritten − D` at spawn and reads
        `capture[(readStartAbs − n) mod size]`. No per-sample drift math,
        no wrap ambiguity; the D+2n read-offset growth falls out for free
        (write head +1 per sample while readAbs steps −1).

    clear() is alloc-free (never setSize outside prepare) — O-GrainScatter WR fix.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CaptureBuffer
{
public:
    void prepare (double sampleRate, float maxSeconds = 3.5f)
    {
        bufferSize = static_cast<int> (sampleRate * maxSeconds) + 1;
        buffer.setSize (2, bufferSize);
        buffer.clear();
        writePosition = 0;
        totalWritten = 0;
    }

    // Alloc-free zeroing for reset paths — must NOT call setSize (that would
    // reallocate on the audio thread).
    void clear() noexcept
    {
        buffer.clear();
        writePosition = 0;
        totalWritten = 0;
    }

    void pushSample (float L, float R) noexcept
    {
        buffer.setSample (0, writePosition, L);
        buffer.setSample (1, writePosition, R);
        writePosition = (writePosition + 1) % bufferSize;
        ++totalWritten;
    }

    // ── v1.6.0 (B4 #1): Freeze ────────────────────────────────────────────────
    //
    // While frozen the ring keeps advancing and keeps being written — but what is
    // written is a COPY of the sample `loopSamples` back, so the ring becomes an
    // endless repetition of that span. The grain engine is told nothing and
    // changes nothing: it goes on reading a ring that goes on being written, and
    // the material it finds there simply repeats.
    //
    // ── Two implementations this is deliberately NOT ─────────────────────────
    //
    //  1. STOP THE WRITE HEAD. The one-line version, and it does not work.
    //     Grains latch `readAbs = totalWritten + offset − gD` at spawn, so with
    //     totalWritten frozen every grain spawned during the hold latches the
    //     SAME readAbs and replays the same G samples:
    //         y(t) = Σ_k w(t − kH) · S(t − kH)
    //     which is strictly periodic at the spawn interval H — a ~28 Hz buzz at
    //     the shipped 200 ms grain and overlap 5.6, not a wash.
    //
    //  2. ADVANCE THE HEAD WITHOUT WRITING. Fixes the buzz — successive grains
    //     latch progressively later positions and sweep forward through held
    //     material — and then fails a different way: the head sweeps over the
    //     whole `kCaptureSeconds` ring, including however much of it has never
    //     been written. Freeze within the first 13 s of loading the plugin and
    //     the wash falls silent as soon as the read passes the last captured
    //     sample. Probe AP caught exactly this: rms 10s/30s/60s all 0.000000.
    //
    // Copying back fixes both, and it fixes the second one by construction
    // rather than by hoping the buffer is full: the caller latches loopSamples
    // to how much has ACTUALLY been captured at the moment the hold begins (see
    // freezeLoopSamples in PluginProcessor), so the loop is exactly as long as
    // there is material for and never longer.
    //
    // Stable by inspection — the write is a unity-gain copy with no filter and
    // no summing, so a hold cannot grow, decay or drift however long it runs.
    // The feedback return is not written at all while frozen, which is what
    // makes "infinite" mean infinite rather than "decays at whatever rate the
    // feedback knob happens to be set to".
    //
    // The read cannot collide with the write: the source index is
    // (totalWritten − loopSamples) mod bufferSize with 1 <= loopSamples <=
    // bufferSize − 1, so it is never the position about to be written. The ring
    // invariant is untouched — grains still reach at most gD + 2·G back, which
    // is what the static_assert in PluginProcessor.h covers.
    void pushLooped (int loopSamples) noexcept
    {
        const juce::int64 src = totalWritten - juce::jmax (1, loopSamples);
        pushSample (readAbs (0, src), readAbs (1, src));
    }

    // Crossfaded write, for the freeze transition only — and it is the LOOP SEAM
    // as much as the transition.
    //
    // `holdWeight` runs 0 (writing live) to 1 (holding) over ~20 ms. During that
    // ramp the ring receives a blend of the incoming signal and the loop's own
    // start, so the point where the frozen material joins back onto itself is a
    // crossfade rather than a splice — which is the one place a looper clicks,
    // and it clicks once per repeat rather than once per gesture.
    //
    // A gain ramp on the written value would not do this: writing `input · 0`
    // erases the ring instead of holding it, which is the opposite of a freeze.
    // The blend has to be against the material being looped.
    //
    // Both endpoints short-circuit to the exact paths they stand for, so
    // `freeze` off is bitwise pushSample and costs the shipped render nothing.
    void pushCrossfaded (float L, float R, int loopSamples, float holdWeight) noexcept
    {
        if (holdWeight <= 0.0f) { pushSample (L, R);        return; }
        if (holdWeight >= 1.0f) { pushLooped (loopSamples); return; }

        const juce::int64 src = totalWritten - juce::jmax (1, loopSamples);
        const float       k   = 1.0f - holdWeight;

        pushSample (readAbs (0, src) * holdWeight + L * k,
                    readAbs (1, src) * holdWeight + R * k);
    }

    // Integer read at an absolute (monotonic) sample index. Double-mod handles
    // negative indices (pre-history reads return the cleared buffer's zeros).
    //
    // v1.7.0: this is the STEREO SOURCE read (B4 #5). Through v1.6.0 it existed
    // and was called by nothing but pushLooped/pushCrossfaded — the grain engine
    // always went through monoSum, so the source's stereo image was discarded
    // before a grain ever saw it and `width` could only pan mono copies of it.
    // A grain in Stereo mode now latches a channel at spawn and reads it here.
    float readAbs (int ch, juce::int64 absIndex) const noexcept
    {
        const int idx = static_cast<int> (((absIndex % bufferSize) + bufferSize) % bufferSize);
        return buffer.getSample (ch, idx);
    }

    // Mono-sum grain source (Stage-2 CONTEXT decision D4): the grain engine
    // reads 0.5·(L+R); equal-power pan then places the mono grain.
    float monoSum (juce::int64 absIndex) const noexcept
    {
        const int idx = static_cast<int> (((absIndex % bufferSize) + bufferSize) % bufferSize);
        return 0.5f * (buffer.getSample (0, idx) + buffer.getSample (1, idx));
    }

    juce::int64 getTotalWritten() const noexcept { return totalWritten; }
    int getBufferSize() const noexcept           { return bufferSize; }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize = 0;
    int writePosition = 0;
    juce::int64 totalWritten = 0;
};
