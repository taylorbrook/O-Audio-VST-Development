/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - section designer, shared types (Stage 2, Phase 2.1)

    Standard C++17 only. "Band settings in, up to eight section parameter sets
    out": this is the whole interface between the designer and its callers
    (the audio engine in Phase 2.2, the display model, the polish in 2.1b).

    Units: frequencies inside the designer are in rad/sample (w = 2 pi f / fs)
    and may exceed pi. Gains are linear unless a name says dB.

    Section order within a band: second-order sections in Butterworth index
    order (Q0 first), the first-order section last.

  ==============================================================================
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace eq
{

inline constexpr double pi = 3.14159265358979323846;

/** Band type. The value is the bNN_type choice index. */
enum class Type : int { bell, lowShelf, highShelf, lowCut, highCut, notch, bandPass, tilt };

inline constexpr int numTypes = 8;

/** Filter order N per bNN_slope choice index (6 dB/oct per order). */
inline constexpr int slopeOrder[8] = { 1, 2, 3, 4, 6, 8, 12, 16 };

inline constexpr int numSlopes = 8;
inline constexpr int maxSections = 8;

/** One band as the designer sees it. gainDbEff is already scaled by the
    gain scale and clamped to +-60 dB. slopeIndex is read only by the five
    slope types; q is not read at 6 dB/oct. */
struct BandSettings
{
    Type type;
    double freqHz;
    double gainDbEff;
    double q;
    int slopeIndex;
};

/** What a section is, which fixes how its analog prototype is evaluated.
      gain2     matched low-pass ratio: SVF + corrector one-pole (bell, shelf, tilt)
      gain1     first-order shelf section: one-pole only
      cutLp2    second-order low-pass of a High Cut: SVF only
      cutHp2    second-order high-pass of a Low Cut: SVF only
      cutLp1    first-order low-pass of a High Cut: one-pole only
      cutHp1    first-order high-pass of a Low Cut: one-pole only
      notch     matched low-pass ratio with the zeros on the unit circle
      bandPass  exact-peak matched band-pass: SVF only */
enum class SectionClass : std::uint8_t { gain2, gain1, cutLp2, cutHp2, cutLp1, cutHp1, notch, bandPass };

inline constexpr int numSectionClasses = 8;

/** An analog section, frequencies in rad/sample.

      gain2 / notch   Ha = c (wp^2 / wz^2) (s^2 + (wz/Qz) s + wz^2) / (s^2 + (wp/Qp) s + wp^2)
      gain1           Ha = c (wp / wz) (s + wz) / (s + wp)
      cuts, bandPass  only wp and Qp are read (first-order cuts: wp alone)

    c is the DC gain of the section. Qz is +infinity for a notch. Fields a
    class does not read hold defined values (0 or 1). */
struct AnalogSection
{
    double wz, Qz, wp, Qp, c;
};

/** The eight numbers of one realised section.

      SVF       H = (mL + mB s + mH s^2) / (1 + k s + s^2),  s = j tan (w/2) / g
      one-pole  H = (cL + cH s) / (1 + s),                   s = j tan (w/2) / G

    A half the section does not have holds the identity (all ones) and its
    flag is false. */
struct SectionParams
{
    double g, k, mL, mB, mH;      // SVF (unused when ! hasSvf)
    double G, cL, cH;             // one-pole (unused when ! hasOnePole)
    bool hasSvf, hasOnePole;
};

/** A designed band. Entries at and beyond numSections hold defined neutral
    values. trueProto is the boost-side analog section before the bell
    re-centre and before the notch / band-pass clamps; for Tilt the scalar
    10^(-g/40) is folded into the last section's c, as it is folded into that
    section's cL and cH. When inverted is true the realised band is the
    reciprocal of the boost that trueProto describes. */
struct BandDesign
{
    int numSections;
    std::array<SectionParams, maxSections> params;
    std::array<SectionClass,  maxSections> cls;
    std::array<AnalogSection, maxSections> trueProto;
    bool inverted;
};

} // namespace eq
