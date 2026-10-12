/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    MeterFrame — the audio → UI visualisation tap (ARCHITECTURE §Visualization
    Tap). One frame is published per host block through
    TripleBuffer<MeterFrame> (PluginProcessor::meterFrames, public); the
    Stage-3 editor Timer (30 Hz) read()s it on the message thread.

    STAGE-3 READER CONTRACT (RESEARCH-2.1 R7) — fields are plain float / int,
    the struct is trivially copyable (static_assert below), and:

      shGain[16]    SIGNED, program-independent effective coefficient vector
                    c_eff[acn] = Y_acn(az_eff, el_eff) · orderGain[band] — the
                    lobe values the channel meters animate (polarity colour =
                    sign). From Phase 2.2 this is the POST-ROTATION vector
                    R·c_eff; in 2.1 the rotation is the identity.
      shPeak[16]    per-channel |bus| peak, PEAK-SINCE-THE-UI-LAST-READ: the
                    audio thread accumulates across blocks until it sees the
                    reader has consumed a frame (hasNewData() false) and only
                    then resets. A 30 Hz reader therefore never misses a
                    transient at 64-sample host blocks (96 % of frames are
                    never read). It is already a HOLD value — JS applies decay
                    only, never a second max().
      azEffDeg,     post-orbit, smoothed source direction (pre-rotation),
      elEffDeg      [−180, 180) / [−90, 90]. The UI puck follows this.
      azRotDeg,     the rotated direction (where the listener hears it) for
      elRotDeg      the polar-plot centre and compass. == azEff / elEff until
                    Phase 2.2.
      orderActive   (order + 1)² = 4 / 9 / 16 — the FUNC-03 meter-count
                    criterion ("orders light 4 / 9 / 16 channels").
      decoderMix    1 = Binaural, 0 = Stereo, ramps between over 20 ms.
      outPeakL/R    post-output-gain peaks, same peak-since-last-read rule
                    (accumulated by the processor).
      latencySamples constant kPartition = 64 (read, never recomputed).

    Headless: <type_traits> only. No JUCE.

  ==============================================================================
*/

#pragma once

#include <type_traits>

namespace OSimpleAmbisonics::dsp
{

struct MeterFrame
{
    float shGain[16] {};
    float shPeak[16] {};
    float azEffDeg = 0.0f, elEffDeg = 0.0f;
    float azRotDeg = 0.0f, elRotDeg = 0.0f;
    int   orderActive = 4;
    float decoderMix  = 1.0f;
    float outPeakL = 0.0f, outPeakR = 0.0f;
    int   latencySamples = 64;
};

// The frame crosses threads by value through the triple buffer.
static_assert (std::is_trivially_copyable_v<MeterFrame>, "MeterFrame must be trivially copyable");

} // namespace OSimpleAmbisonics::dsp
