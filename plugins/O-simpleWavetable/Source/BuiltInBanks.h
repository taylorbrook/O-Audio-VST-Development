/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - BuiltInBanks (Stage 2.1)
    Ouaricon Audio
    Developer: Taylor Brook

    The five built-in banks, shared by every plugin instance through a
    juce::SharedResourcePointer<BuiltInBanks> processor member (constructed
    on the message thread, never in processBlock). Immutable after the ctor.

    The ctor MUST NOT construct a SharedResourcePointer<BuiltInBanks>: JUCE
    builds the object inside its SpinLock, so that would deadlock.

    buildMillis is logged by the bank-check driver; never gated (no
    wall-clock verdicts).

  ==============================================================================
*/

#pragma once

#include <array>

#include "WavetableBank.h"

struct BuiltInBanks
{
    static constexpr int kCount  = 5;
    static constexpr int kFrames = 32;

    BuiltInBanks();

    // index == bank param 0..4 (0 Sine->Saw, 1 Sine->Square, 2 Pulse Width,
    // 3 Formant, 4 Drive). nullptr outside 0..4 (5 = Imported lives elsewhere).
    const WavetableBank* get (int index) const noexcept
    {
        return (index >= 0 && index < kCount) ? &banks[(size_t) index] : nullptr;
    }

    std::array<WavetableBank, kCount> banks;
    double buildMillis = 0.0;

    JUCE_DECLARE_NON_COPYABLE (BuiltInBanks)
};
