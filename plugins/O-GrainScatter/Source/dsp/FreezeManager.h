/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>
#include "DelayBuffer.h"
#include "ExtremaMap.h"
#include <array>
#include <cmath>

// v3.0.4 freeze. A held snapshot of the delay line, played by grains as if the
// write head were still running. Each slot keeps a virtual write head that keeps
// advancing while freeze is held; a grain latches base = head - delay at spawn and
// reads base + readPosition, wrapped round the captured loop. Frozen grains then
// stream through the held audio exactly as live grains stream through the delay
// line, so Scan and Spread work and the output is the captured loop repeating.
// (v3.0.3 started every frozen grain at snapshot index 0, so each grain replayed
// the same samples: a buzz at the spawn rate.)
//
// Two slots: a grain latches its slot at spawn and finishes on it after release,
// so release needs no timer and nothing switches source mid-grain. The grain
// envelopes are the crossfade in both directions.
//
// v3.3.0: a slot is a grain-engine MipSnapshot — every octave level of the ring
// over the captured span — read through the same band-limited sinc reader as the
// live ring, so frozen pitched-up grains don't alias either. The capture ends
// margin(3) = 506 samples (~10 ms) before the write head, where every level is
// settled; at engage the newest ~10 ms are not part of the loop.
//
// v3.5.0: one ExtremaMap analyses the held capture (not per slot: only grains of the
// current slot spawn, and a grain latches its length at spawn). Engage starts it,
// release / clear / prepare cancel it; it runs a little each block (analysisStep).
class FreezeManager
{
public:
    static constexpr int kNumSlots = 2;
    using Snapshot = ouaricon::grain::MipSnapshot<DelayBuffer::kMaxLevel>;
    static constexpr int kCaptureEndDelay = DelayBuffer::margin (DelayBuffer::kMaxLevel);

    void prepare (double newSampleRate, int maxLengthSamples)
    {
        for (auto& s : slots)
        {
            s.snap.prepare (maxLengthSamples);
            s.length = s.loopLength = s.xfade = 0;
            s.head = 0.0;
        }
        maxLength = maxLengthSamples;
        maxXfade = juce::jmax (1, static_cast<int> (newSampleRate * 0.010));  // 10 ms seam crossfade
        sampleRate = newSampleRate;
        extrema.prepare (newSampleRate, maxLengthSamples);
        active = false;
        currentSlot = 0;
    }

    // Alloc-free state reset for reset() — zeroes the (already-sized) snapshots and
    // clears freeze state. CR-02: keeps reset() off the allocator.
    void clear()
    {
        for (auto& s : slots)
        {
            s.snap.clear();
            s.length = s.loopLength = s.xfade = 0;
            s.head = 0.0;
        }
        extrema.clear();
        active = false;
        currentSlot = 0;
    }

    // Capture the most recent settled audio into `slot`. The loop length is latched
    // here to what the delay line has actually been written with (less the capture
    // end offset), capped at the slot size, so a freeze pressed right after load
    // doesn't loop through never-written silence.
    // engageAbs: the absolute sample clock at engage (the analysis quota counts from it).
    void engage (const DelayBuffer& delayBuf, int slot, juce::int64 engageAbs)
    {
        auto& s = slots[static_cast<size_t> (slot)];
        const int len = juce::jlimit (0, maxLength, delayBuf.getSamplesWritten() - kCaptureEndDelay);
        s.snap.captureFrom (delayBuf.getRing(), kCaptureEndDelay, len);
        s.length     = len;
        s.xfade      = juce::jmin (maxXfade, len / 4);
        s.loopLength = len - s.xfade;
        s.head       = static_cast<double> (len);   // one past the newest captured sample
        currentSlot  = slot;
        active       = true;
        extrema.begin (s.snap.getOrigin(), s.length, s.xfade, s.loopLength, engageAbs);
    }

    // New grains go back to the live delay line. Grains already on the snapshot keep it.
    void release()
    {
        active = false;
        extrema.cancel();
    }

    // ── v3.5.0 Grain Adapt ──────────────────────────────────────────────────
    // Once per block, after engage / release: analysis work up to blockEndAbs's quota.
    void analysisStep (const DelayBuffer& delayBuf, juce::int64 blockEndAbs) noexcept
    {
        extrema.step (delayBuf.getRing(), blockEndAbs);
    }

    bool adaptValid() const noexcept { return active && extrema.isDone(); }
    bool adaptReady (juce::int64 abs) const noexcept { return active && extrema.ready (abs); }
    std::uint32_t adaptGeneration() const noexcept { return extrema.getGeneration(); }
    const ExtremaMap& getExtremaMap() const noexcept { return extrema; }

    double adaptMinLength() const noexcept { return kAdaptMinMs * 0.001 * sampleRate; }
    double adaptMaxLength() const noexcept { return kAdaptMaxMs * 0.001 * sampleRate; }

    // c for (grain size, adapt): the clamped mean length over the snapshot = gs.
    double solveAdaptC (double gs, double a) const noexcept
    {
        return extrema.solveC (gs, a, adaptMinLength(), adaptMaxLength());
    }

    // Adapted grain length (output samples) for a grain anchored at snapshot position
    // `anchor`: forward grains measure the extrema ahead of it, reverse grains behind.
    double adaptiveLength (double anchor, bool reverse, double c, double gs, double a) const noexcept
    {
       #ifdef GS_TEST_ADAPT_FIXED_K
        juce::ignoreUnused (c);
        return extrema.lengthAtFixedK (extrema.loopCoord (anchor), reverse, gs, a, adaptMinLength(), adaptMaxLength());
       #else
        return extrema.lengthAt (extrema.loopCoord (anchor), reverse, c, gs, a, adaptMinLength(), adaptMaxLength());
       #endif
    }

    double loopCoord (double anchor) const noexcept { return extrema.loopCoord (anchor); }

    bool isActive() const { return active; }
    int getCurrentSlot() const { return currentSlot; }

    // Per sample, after the delay-line push: the held slot's virtual head moves on.
    // Kept bounded by stepping back one loop length (reads wrap modulo the loop anyway).
    void advance()
    {
        if (! active) return;
        auto& s = slots[static_cast<size_t> (currentSlot)];
        s.head += 1.0;
        if (s.loopLength > 0 && s.head >= static_cast<double> (s.length + s.loopLength))
            s.head -= static_cast<double> (s.loopLength);
    }

    // Snapshot position a grain spawned now at `delaySamples` starts from.
    double getSpawnBase (double delaySamples) const
    {
        return slots[static_cast<size_t> (currentSlot)].head - delaySamples;
    }

    // The captured audio is a loop over [xfade, length). Its last xfade samples are
    // blended equal-power into the xfade samples before the loop start, so the seam
    // where the newest captured sample meets the oldest is not a splice. The reader
    // is linear, so blending two band-limited reads equals reading the blend.
    void readStereo (int slot, int level, double position, int sincSet, float& outL, float& outR) const
    {
        const auto& s = slots[static_cast<size_t> (slot)];
        outL = outR = 0.0f;
        if (s.loopLength <= 0) return;

        Seam seam;
        locate (s, position, seam);
        s.snap.readStereo (level, seam.b, sincSet, outL, outR);
        if (seam.gIn > 0.0f || seam.gOut < 1.0f)
        {
            float l2, r2;
            s.snap.readStereo (level, seam.b - static_cast<double> (s.loopLength), sincSet, l2, r2);
            outL = seam.gOut * outL + seam.gIn * l2;
            outR = seam.gOut * outR + seam.gIn * r2;
        }
    }

    // (L + R) / 2, for the spatial path.
    float readMono (int slot, int level, double position, int sincSet) const
    {
        const auto& s = slots[static_cast<size_t> (slot)];
        if (s.loopLength <= 0) return 0.0f;

        Seam seam;
        locate (s, position, seam);
        float m = s.snap.readMono (level, seam.b, sincSet);
        if (seam.gIn > 0.0f || seam.gOut < 1.0f)
            m = seam.gOut * m + seam.gIn * s.snap.readMono (level, seam.b - static_cast<double> (s.loopLength), sincSet);
        return m;
    }

private:
    struct Slot
    {
        Snapshot snap;
        int length = 0;       // captured samples
        int loopLength = 0;   // length - xfade
        int xfade = 0;        // seam crossfade samples
        double head = 0.0;    // virtual write head (snapshot coordinates)
    };

    struct Seam { double b = 0.0; float gOut = 1.0f, gIn = 0.0f; };

    static void locate (const Slot& s, double position, Seam& out)
    {
        const double x  = static_cast<double> (s.xfade);
        const double lp = static_cast<double> (s.loopLength);
        double m = std::fmod (position - x, lp);
        if (m < 0.0) m += lp;
        out.b = x + m;   // [xfade, length)
        const double seamStart = static_cast<double> (s.length) - x;
        if (s.xfade > 0 && out.b >= seamStart)
        {
            const float fade = static_cast<float> ((out.b - seamStart) / x);
            out.gOut = std::cos (fade * juce::MathConstants<float>::halfPi);
            out.gIn  = std::sin (fade * juce::MathConstants<float>::halfPi);
        }
    }

    static constexpr double kAdaptMinMs = 10.0;    // the Grain Size range
    static constexpr double kAdaptMaxMs = 500.0;

    std::array<Slot, kNumSlots> slots;
    ExtremaMap extrema;
    double sampleRate = 44100.0;
    int maxLength = 0;
    int maxXfade = 441;
    int currentSlot = 0;
    bool active = false;
};
