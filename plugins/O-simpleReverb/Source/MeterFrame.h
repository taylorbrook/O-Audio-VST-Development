/*
   This file is part of O-simpleReverb, an Ouaricon Audio plugin.
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

    MeterFrame - the audio -> UI visualisation tap (v3.0.0).

    One frame is published per host block through TripleBuffer<MeterFrame>
    (PluginProcessor::meterFrames, public); the editor's 30 Hz Timer read()s
    it on the message thread. Every field is SINCE-THE-UI-LAST-READ: the audio
    thread accumulates across blocks until it sees the reader has consumed a
    frame (hasNewData() false) and only then starts again, so a 30 Hz reader
    never misses a burst at 64-sample host blocks.

      inDb       peak |x| of the input (the dry buffer, before everything),
                 in dBFS. Floor kFloorDb.
      wetDb      MEAN-SQUARE level of the wet signal after Low Cut and BEFORE
                 the WET gain, in dBFS: the tail is visible at any mix. The
                 live decay well plots it; the output meter shows the mix.
      outPeakL,  linear peak of the final output per side (R = L on a mono
      outPeakR   bus).
      spanMs     the audio time the fields above cover, so the page can place
                 a point in audio time when the timer ticks jitter.

    Not in the frame: the RT60 slope (baseT60 x DECAY, from getReverbModel and
    the relays: it must keep answering while the host idles the audio thread),
    the early-reflection taps and the tone curve (natives; they change only on
    a gesture and would defeat the hash gate here).

    Headless: <type_traits> only. No JUCE.

  ==============================================================================
*/

#pragma once

#include <type_traits>

namespace osr
{

struct MeterFrame
{
    static constexpr float kFloorDb = -120.0f;

    float inDb     = kFloorDb;
    float wetDb    = kFloorDb;
    float outPeakL = 0.0f, outPeakR = 0.0f;
    float spanMs   = 0.0f;
};

// The frame crosses threads by value through the triple buffer.
static_assert (std::is_trivially_copyable_v<MeterFrame>, "MeterFrame must be trivially copyable");

} // namespace osr
