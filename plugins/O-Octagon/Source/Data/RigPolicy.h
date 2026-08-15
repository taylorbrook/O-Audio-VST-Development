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
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace oo
{

//==============================================================================
/**
    THE SAFE-MODE PARTITION, IN ONE PLACE (PLAN-4.1 P91).

    Extracted from prepareToPlay() at Phase 4.1 so a probe can present sets that
    isBusesLayoutSupported() rejects — which is the only way to test the FORM of this predicate
    rather than its behaviour on the five layouts prepareToPlay() can be reached with.

    ── What it says, stated the way COMPAT-04 criterion 3 states it ──────────────────────────────

    A set is a REAL RIG iff it is one of exactly three containers: 7.1, 7.1-SDDS, 5.1.2. Those are
    the three 8-channel formats Logic exposes (`critical_logic_only_named_surround_formats`), and
    they are the three isBusesLayoutSupported() admits as full-rig output.

    ANY OTHER SET IS SAFE, AND THE BANNER IS RAISED. That includes mono and stereo, which is the
    everyday case — and it also includes any FOURTH 8-or-more-channel container a future JUCE or a
    future host might present. On such a set the plugin does not know where the speakers are, so it
    raises the banner and folds. That is the design, and it is why this is written as the complement
    of the three real containers rather than as `== mono() || == stereo()`.

    The two spellings agree on every layout that exists today and disagree exactly where it matters:

    | Set                    | complement form (SHIPPED) | `== mono \|\| == stereo` (REJECTED) |
    |------------------------|---------------------------|-------------------------------------|
    | mono, stereo           | SAFE — banner up          | SAFE — banner up                    |
    | 7.1, 7.1-SDDS, 5.1.2   | REAL — banner down        | REAL — banner down                  |
    | a fourth 8-ch set      | **SAFE — banner up**      | **REAL — banner DOWN, silently**    |

    The rejected spelling would silently STOP raising the banner on a container nobody had mapped.
    Probe CO asserts this table; probe BM asserts the wiring from prepareToPlay(); NC1 proves BM
    alone cannot tell the two spellings apart, which is what makes the pairing non-vacuous.

    ── Why a header, and why juce_audio_basics only ──────────────────────────────────────────────

    AudioChannelSet lives in juce_audio_basics, so this is includable from the geometry unit target,
    whose link line is deliberately narrow (no juce_audio_processors, no juce_gui_*) and whose
    narrowness is a structural property gate 11 protects. Header-only also means no CMakeLists edit
    on either test target.

    THE THREE-CONTAINER LITERAL BELOW MUST APPEAR EXACTLY ONCE IN Source/. Two copies is how a
    predicate drifts away from its call site with every probe still green.
*/
namespace rig
{
    inline bool isRealRig (const juce::AudioChannelSet& s) noexcept
    {
        return s == juce::AudioChannelSet::create7point1()        // primary
            || s == juce::AudioChannelSet::create7point1SDDS()    // Logic/Emagic order
            || s == juce::AudioChannelSet::create5point1point2(); // third 8-ch container
    }
}

} // namespace oo
