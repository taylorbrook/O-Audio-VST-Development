/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>

// ============================================================================
// GiMpe — MPE lower-zone state (PLAN-2.5b T9, PD7 / PD8, D31).
//
// ch 1 = master, ch 2 … memberCount + 1 = members. Per-channel bend / pressure
// / slide are written on EVERY event (MPE on or off) and read only with
// mpe_enable on, so the 2.5a non-MPE path stays untouched (RESEARCH-2.5b
// Summary 10). RPN parsing is juce::MidiRPNDetector (fixed per-channel
// state, noexcept, no allocation): NRPN interleave, RPN null and 14-bit
// data entry are handled by the detector; this policy layer applies
//   RPN 0 on a member channel → memberRange = semis + cents/100, [0, 96]
//                               (zone-wide; cents kept);
//   RPN 0 on ch 1             → ignored (the master range is pitch_bend_range);
//   RPN 6 on ch 1 (MCM)       → memberCount = MSB (0–15), memberRange = 48;
//   anything else             → ignored (upper zone = v1.1).
// MCM 0 ≠ MPE off: the parameter is the switch.
// ============================================================================
namespace ogi
{
    struct GiMpe
    {
        static constexpr double kDefaultMemberRange = 48.0;

        float  bend[17] {};       // −1..1, index = MIDI channel 1..16
        float  pressure[17] {};   // 0..1
        float  slide[17] {};      // −1..1 (CC74 centred at 64)
        double memberRange = kDefaultMemberRange;
        int    memberCount = 15;
        juce::MidiRPNDetector rpn;

        void reset() noexcept
        {
            std::fill (std::begin (bend), std::end (bend), 0.0f);
            std::fill (std::begin (pressure), std::end (pressure), 0.0f);
            std::fill (std::begin (slide), std::end (slide), 0.0f);
            memberRange = kDefaultMemberRange;
            memberCount = 15;
            rpn.reset();
        }

        bool isMember (int channel) const noexcept
        {
            return channel >= 2 && channel <= memberCount + 1;
        }

        static bool isRpnCc (int cc) noexcept
        {
            return cc == 6 || cc == 38 || (cc >= 98 && cc <= 101);
        }

        enum class Rpn { none, memberRange, mcm };

        /** Feeds one CC 6 / 38 / 98–101. Returns what changed (policy above). */
        Rpn feed (int channel, int cc, int value) noexcept
        {
            const auto msg = rpn.tryParse (channel, cc, value);
            if (! msg.has_value() || msg->isNRPN)
                return Rpn::none;
            const int msb = msg->is14BitValue ? (msg->value >> 7) : msg->value;
            const int lsb = msg->is14BitValue ? (msg->value & 0x7F) : 0;
            if (msg->parameterNumber == 0 && msg->channel >= 2)
            {
                memberRange = std::clamp (static_cast<double> (msb) + static_cast<double> (lsb) / 100.0, 0.0, 96.0);
                return Rpn::memberRange;
            }
            if (msg->parameterNumber == 6 && msg->channel == 1)
            {
                memberCount = std::clamp (msb, 0, 15);
                memberRange = kDefaultMemberRange;
                return Rpn::mcm;
            }
            return Rpn::none;
        }
    };
} // namespace ogi
