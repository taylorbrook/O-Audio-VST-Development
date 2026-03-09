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
