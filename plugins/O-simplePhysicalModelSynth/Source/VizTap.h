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

    O-simplePhysicalModelSynth - VizTap (real-time-safe visualization taps)

    PERF-01: the audio thread is COPY-ONLY into pre-allocated lock-free structures —
    NO allocation, NO FFT, NO locks. Stage 3's editor Timer (message thread) reads
    these and computes the scope / spectrum / loop diagram / modal stems.

    Three taps, all written by the audio thread, read by the message thread:
      - waveform   : 8192-sample lock-free ring of the master output (scope + FFT).
      - loopEnergy : KS circulating-energy scalar (the dampening-loop diagram, UI-02).
      - modal stems: 8 (f_k, amp_k) pairs — the exact values driving the bank (UI-05).

    VizRing ported verbatim (O-simpleFM FmVizAnalyzer.h:30-58). The loop-energy scalar
    and stem arrays are authored here (FmVizAnalyzer has only a frameCount atomic);
    they use the same store(relaxed)/load(relaxed) idiom.

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>

//==============================================================================
// Lock-free overwrite ring. Power-of-two size + bitmask wrap. Fully allocated at
// construction — nothing to prepare.
class VizRing
{
public:
    static constexpr int kSize = 8192;
    static_assert ((kSize & (kSize - 1)) == 0, "VizRing size must be power of two");

    void write (const float* data, int n) noexcept             // AUDIO THREAD
    {
        int w = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            buffer[(size_t) w].store (data[i], std::memory_order_relaxed);
            w = (w + 1) & (kSize - 1);
        }
        writePos.store (w, std::memory_order_release);
    }

    void readLatest (float* dest, int n) const noexcept        // MESSAGE THREAD
    {
        const int w = writePos.load (std::memory_order_acquire);
        int start = (w - n) & (kSize - 1);
        for (int i = 0; i < n; ++i)
            dest[i] = buffer[(size_t) ((start + i) & (kSize - 1))].load (std::memory_order_relaxed);
    }

private:
    std::array<std::atomic<float>, (size_t) kSize> buffer {};
    std::atomic<int> writePos { 0 };
};

//==============================================================================
class VizTap
{
public:
    static constexpr int kStems = 8;

    VizRing waveform;

    //--- audio thread (lead voice + processor) --------------------------------
    void publishLoopEnergy (float e) noexcept
    {
        loopEnergy.store (e, std::memory_order_relaxed);
    }

    void publishStems (const std::array<float, kStems>& freqs,
                       const std::array<float, kStems>& amps) noexcept
    {
        for (int k = 0; k < kStems; ++k)
        {
            stemFreq[(size_t) k].store (freqs[(size_t) k], std::memory_order_relaxed);
            stemAmp[(size_t) k].store  (amps[(size_t) k],  std::memory_order_relaxed);
        }
    }

    //--- message thread (Stage-3 editor Timer) --------------------------------
    float getLoopEnergy() const noexcept { return loopEnergy.load (std::memory_order_relaxed); }
    float getStemFreq (int k) const noexcept { return stemFreq[(size_t) k].load (std::memory_order_relaxed); }
    float getStemAmp  (int k) const noexcept { return stemAmp[(size_t) k].load (std::memory_order_relaxed); }

private:
    std::atomic<float> loopEnergy { 0.0f };
    std::array<std::atomic<float>, (size_t) kStems> stemFreq {};
    std::array<std::atomic<float>, (size_t) kStems> stemAmp {};
};
