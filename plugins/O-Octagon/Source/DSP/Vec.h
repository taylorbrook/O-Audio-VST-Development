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

#include <cmath>
#include <type_traits>

/**
    Minimal POD vector types shared by VenueModel and ConvexHull2D.

    Deliberately NOT juce::Point<float> (ARCHITECTURE §2 ConvexHull2D): juce_graphics must not be
    pulled into the RT path, and these types have to be trivially copyable so VenueSnapshot can be
    memcpy'd into a double-buffer slot without a constructor running on the message thread or a
    destructor running anywhere.

    This header is the shared root that lets VenueModel and ConvexHull2D stay independent of each
    other (PLAN-2.1 P12). Header-only — it is NOT added to target_sources.
*/
namespace oo
{

//==============================================================================
struct Vec2
{
    float x { 0.0f };
    float y { 0.0f };
};

struct Vec3
{
    float x { 0.0f };
    float y { 0.0f };
    float z { 0.0f };
};

static_assert (std::is_trivially_copyable_v<Vec2>, "Vec2 must be memcpy-safe for VenueSnapshot");
static_assert (std::is_trivially_copyable_v<Vec3>, "Vec3 must be memcpy-safe for VenueSnapshot");

//==============================================================================
// 2D helpers — the monotone chain, the inside test and the edge projection are written in terms of
// exactly these four. Free functions rather than members so the structs stay aggregate-initialisable
// ({ x, y }) and provably trivial.

inline constexpr Vec2 sub (Vec2 a, Vec2 b) noexcept { return { a.x - b.x, a.y - b.y }; }

/** z-component of the 3D cross product of two floor-plane vectors — a SIGNED AREA.
    Positive means b turns counter-clockwise from a. The units are m², which is why EPS_CROSS is an
    area tolerance and not a length one (ARCHITECTURE §3.1.2).
*/
inline constexpr float cross (Vec2 a, Vec2 b) noexcept { return a.x * b.y - a.y * b.x; }

inline constexpr float dot (Vec2 a, Vec2 b) noexcept { return a.x * b.x + a.y * b.y; }

inline constexpr float len2 (Vec2 a) noexcept { return a.x * a.x + a.y * a.y; }

inline float len (Vec2 a) noexcept { return std::sqrt (len2 (a)); }

//==============================================================================
// 3D helpers — used by VenueModel for the centroid and rigScale. The DBAP distance itself is a
// Phase 2.2 concern; only the venue-derived quantities need 3D at 2.1.

inline constexpr Vec3 sub (Vec3 a, Vec3 b) noexcept { return { a.x - b.x, a.y - b.y, a.z - b.z }; }

inline constexpr float len2 (Vec3 a) noexcept { return a.x * a.x + a.y * a.y + a.z * a.z; }

inline float len (Vec3 a) noexcept { return std::sqrt (len2 (a)); }

/** The (x, y) floor projection. The hull is 2D over this while DBAP distances stay 3D — the
    dimensional split of ARCHITECTURE §3.1.1, which is intentional and must not be "fixed".
*/
inline constexpr Vec2 floorOf (Vec3 a) noexcept { return { a.x, a.y }; }

} // namespace oo
