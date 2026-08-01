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

// 3rd-order (4-point) Lagrange interpolation.
// Given four equally-spaced samples ym1, y0, y1, y2 and a fractional
// position `frac` in [0,1) between y0 and y1, returns the interpolated value.
inline float lagrangeInterpolate (float ym1, float y0, float y1, float y2, float frac)
{
    float c0 = y0;
    float c1 = y1 - (1.0f / 3.0f) * ym1 - 0.5f * y0 - (1.0f / 6.0f) * y2;
    float c2 = 0.5f * (ym1 + y1) - y0;
    float c3 = (1.0f / 6.0f) * (y2 - ym1) + 0.5f * (y0 - y1);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
