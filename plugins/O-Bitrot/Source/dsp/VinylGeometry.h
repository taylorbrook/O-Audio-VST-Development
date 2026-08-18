/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - VinylGeometry (v1.7.0)

    The ONE definition of what VINYL_RPM means in samples. Three consumers now
    need it and they must agree EXACTLY, not approximately:

      * VinylTransport — the revolution quantum every groove jump is a multiple
        of, backward and (since v1.7.0) forward.
      * VinylWarp      — the warp LFO's period. Brief item 27b's claim is that
        "a warped locked groove wobbles identically every pass", and that is
        only true if the LFO period is the SAME INTEGER as the jump distance.
        Deriving the LFO from the un-rounded seconds instead would leave a
        sub-sample phase residue per pass (0.077 samples at 78 RPM / 48 kHz) —
        small, but it makes the identity approximate when it can be exact.
      * VinylBed       — the once-per-revolution rumble AM, which beats at the
        platter rate.

    Three copies of `rpmIndex == 1 ? 45.0 : 100.0/3.0` is exactly the shape
    that drifts silently the next time a speed is added
    (pattern_test_fixture_mirrors_drift_silently), so there is one table here
    and no copies anywhere else.

    Index 2 (78 RPM) was APPENDED to the VINYL_RPM choice in v1.7.0. Appending
    repoints the normalised fraction saved in old presets, which is why
    PluginProcessor installs a preset-manager v1.0.6 migration hook; the choice
    INDEX stored in APVTS session state is unaffected. An out-of-range index
    falls back to 33 1/3 rather than reading off the end.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace VinylGeometry
{
    /** Revolutions per second for a VINYL_RPM choice index. */
    inline double revsPerSecond (int rpmIndex) noexcept
    {
        if (rpmIndex == 1) return 45.0 / 60.0;         // 45    => 0.75 Hz
        if (rpmIndex == 2) return 78.0 / 60.0;         // 78    => 1.3 Hz
        return (100.0 / 3.0) / 60.0;                   // 33 1/3 => 0.5556 Hz
    }

    /** Seconds per revolution: 1.8 @ 33 1/3, 1.333 @ 45, 0.769 @ 78. */
    inline double revolutionSeconds (int rpmIndex) noexcept
    {
        return 1.0 / revsPerSecond (rpmIndex);
    }

    /** The revolution quantum in WHOLE samples — the integer both the jump
        distance and the warp LFO period are built from. */
    inline int revolutionSamples (double sampleRate, int rpmIndex) noexcept
    {
        return juce::jmax (1, juce::roundToIntAccurate (sampleRate
                                                        * revolutionSeconds (rpmIndex)));
    }
}
