# Ambisonics Encoding Deep-Dive: Full HOA3 Pipeline for Per-Grain Spatial Granular Synthesis

**Researched:** 2026-02-08
**JUCE Version:** 8.0.4
**Target:** Per-grain HOA3 (3rd-order, 16-channel) ambisonics encoding in a JUCE C++ audio plugin
**Format:** AmbiX (ACN channel ordering, SN3D normalization)
**Confidence:** HIGH (established mathematical foundations, verified against multiple references)

---

## Table of Contents

1. [Coordinate System](#1-coordinate-system)
2. [Spherical Harmonic Mathematics](#2-spherical-harmonic-mathematics)
3. [Optimized C++ Implementation](#3-optimized-c-implementation)
4. [Per-Grain Encoding Architecture](#4-per-grain-encoding-architecture)
5. [Ambisonics Bus Accumulation](#5-ambisonics-bus-accumulation)
6. [Normalization Comparison: SN3D vs N3D vs FuMa](#6-normalization-comparison-sn3d-vs-n3d-vs-fuma)

---

## 1. Coordinate System

### 1.1 AmbiX Convention

All formulas in this document use the **AmbiX** coordinate convention, which is the standard used by JUCE, VST3, AU, and AAX for ambisonics interchange.

```
Azimuth (a):    0 = front, +pi/2 = left, -pi/2 = right, +/-pi = back
Elevation (e):  0 = horizon, +pi/2 = zenith (up), -pi/2 = nadir (down)

Cartesian mapping (unit sphere):
  x = cos(a) * cos(e)     (front-back axis, positive = front)
  y = sin(a) * cos(e)     (left-right axis, positive = left)
  z = sin(e)               (up-down axis, positive = up)
```

The position is parameterized as `(a, e)` where `a` is measured counter-clockwise from front in the horizontal plane, and `e` is measured upward from the horizontal plane.

### 1.2 Cartesian-to-Spherical Conversion

```
a = atan2(y, x)
e = atan2(z, sqrt(x^2 + y^2))
  = asin(z)                       (when on unit sphere: x^2 + y^2 + z^2 = 1)
```

### 1.3 Common Pitfalls

**Sign conventions differ between FuMa and AmbiX.** In FuMa (legacy B-format), the convention is `X = front/back`, `Y = left/right`, `Z = up/down` with a different azimuth sign. In AmbiX (ACN ordering), ACN 1 = Y (left/right), ACN 2 = Z (up/down), ACN 3 = X (front/back). When converting legacy FuMa content, both channel reordering AND potential sign flips must be applied.

**Latitude vs colatitude.** Some physics references use colatitude `theta` measured from the north pole (0 = up, pi = down). AmbiX uses elevation from the equator. The relationship is `e = pi/2 - theta`. When adapting formulas from physics texts, substitute `cos(theta) = sin(e)` and `sin(theta) = cos(e)`.

---

## 2. Spherical Harmonic Mathematics

### 2.1 General Formula

The real-valued spherical harmonic of degree `n` (order) and index `m` with SN3D normalization is:

```
Y_n^m(a, e) = N_n^|m| * P_n^|m|(sin(e)) * T_m(a)
```

where:

- `N_n^|m|` is the **SN3D normalization factor**
- `P_n^|m|` is the **associated Legendre polynomial** (without Condon-Shortley phase)
- `T_m(a)` is the **azimuthal trigonometric function**

### 2.2 SN3D Normalization Factor

```
N_n^|m| = sqrt( (2 - delta_{m,0}) * (n - |m|)! / (n + |m|)! )

where delta_{m,0} = { 1 if m = 0
                    { 0 if m != 0
```

The factor `(2 - delta_{m,0})` equals 2 for all `m != 0` and 1 for `m = 0`. This accounts for the fact that non-zero-order azimuthal functions (sin and cos) have a maximum of 1, while the m=0 case has no azimuthal modulation.

### 2.3 Azimuthal Function

```
T_m(a) = { sin(|m| * a)    if m < 0
         { 1                if m = 0
         { cos(m * a)       if m > 0
```

### 2.4 ACN Channel Ordering

The Ambisonic Channel Number maps each (n, m) pair to a linear index:

```
ACN = n^2 + n + m = n * (n + 1) + m
```

For orders 0 through 3, this produces 16 channels:

| ACN | n | m  | Letter | Description          |
|-----|---|----|--------|----------------------|
| 0   | 0 | 0  | W      | Omnidirectional      |
| 1   | 1 | -1 | Y      | Left-right (Y axis)  |
| 2   | 1 | 0  | Z      | Up-down (Z axis)     |
| 3   | 1 | +1 | X      | Front-back (X axis)  |
| 4   | 2 | -2 | V      | 2nd order diagonal   |
| 5   | 2 | -1 | T      | 2nd order lateral    |
| 6   | 2 | 0  | R      | 2nd order axial      |
| 7   | 2 | +1 | S      | 2nd order lateral    |
| 8   | 2 | +2 | U      | 2nd order diagonal   |
| 9   | 3 | -3 | Q      | 3rd order            |
| 10  | 3 | -2 | O      | 3rd order            |
| 11  | 3 | -1 | M      | 3rd order            |
| 12  | 3 | 0  | K      | 3rd order axial      |
| 13  | 3 | +1 | L      | 3rd order            |
| 14  | 3 | +2 | N      | 3rd order            |
| 15  | 3 | +3 | P      | 3rd order            |

### 2.5 Associated Legendre Polynomials (Orders 0-3)

The associated Legendre polynomials `P_n^m(x)` without the Condon-Shortley phase, evaluated at `x = sin(e)`:

```
Order 0:
  P_0^0(sin(e)) = 1

Order 1:
  P_1^0(sin(e)) = sin(e)
  P_1^1(sin(e)) = cos(e)

Order 2:
  P_2^0(sin(e)) = (3*sin^2(e) - 1) / 2
  P_2^1(sin(e)) = 3 * sin(e) * cos(e)
  P_2^2(sin(e)) = 3 * cos^2(e)

Order 3:
  P_3^0(sin(e)) = sin(e) * (5*sin^2(e) - 3) / 2
  P_3^1(sin(e)) = (3/2) * cos(e) * (5*sin^2(e) - 1)
  P_3^2(sin(e)) = 15 * sin(e) * cos^2(e)
  P_3^3(sin(e)) = 15 * cos^3(e)
```

### 2.6 SN3D Normalization Factors Table (Orders 0-3)

Computing `N_n^|m| = sqrt( (2 - delta_{m,0}) * (n - |m|)! / (n + |m|)! )` for each (n, m):

| ACN | (n, m)  | Factorial ratio (n-\|m\|)!/(n+\|m\|)! | (2-delta) | N_n^\|m\| | Decimal     |
|-----|---------|---------------------------------------|-----------|-----------|-------------|
| 0   | (0, 0)  | 0!/0! = 1                             | 1         | 1         | 1.0000      |
| 1   | (1, -1) | 0!/2! = 1/2                           | 2         | 1         | 1.0000      |
| 2   | (1, 0)  | 1!/1! = 1                             | 1         | 1         | 1.0000      |
| 3   | (1, +1) | 0!/2! = 1/2                           | 2         | 1         | 1.0000      |
| 4   | (2, -2) | 0!/4! = 1/24                          | 2         | 1/sqrt(12) | 0.28868   |
| 5   | (2, -1) | 1!/3! = 1/6                           | 2         | 1/sqrt(3) | 0.57735    |
| 6   | (2, 0)  | 2!/2! = 1                             | 1         | 1         | 1.0000      |
| 7   | (2, +1) | 1!/3! = 1/6                           | 2         | 1/sqrt(3) | 0.57735    |
| 8   | (2, +2) | 0!/4! = 1/24                          | 2         | 1/sqrt(12) | 0.28868   |
| 9   | (3, -3) | 0!/6! = 1/720                         | 2         | 1/sqrt(360) | 0.05270  |
| 10  | (3, -2) | 1!/5! = 1/120                         | 2         | 1/sqrt(60) | 0.12910   |
| 11  | (3, -1) | 2!/4! = 1/12                          | 2         | 1/sqrt(6) | 0.40825    |
| 12  | (3, 0)  | 3!/3! = 1                             | 1         | 1         | 1.0000      |
| 13  | (3, +1) | 2!/4! = 1/12                          | 2         | 1/sqrt(6) | 0.40825    |
| 14  | (3, +2) | 1!/5! = 1/120                         | 2         | 1/sqrt(60) | 0.12910   |
| 15  | (3, +3) | 0!/6! = 1/720                         | 2         | 1/sqrt(360) | 0.05270  |

### 2.7 Complete Derivation: Y_n^m(a, e) for Each ACN Channel

Each encoding coefficient is `Y_n^m(a, e) = N_n^|m| * P_n^|m|(sin(e)) * T_m(a)`. Below is the full product for every channel.

#### Order 0 (1 channel)

```
ACN 0: Y_0^0(a, e) = 1 * 1 * 1 = 1
```

The omnidirectional component. A source at any direction contributes equally to ACN 0.

#### Order 1 (3 channels, cumulative 4)

```
ACN 1: Y_1^-1(a, e) = 1 * cos(e) * sin(a)
                     = sin(a) * cos(e)

ACN 2: Y_1^0(a, e)  = 1 * sin(e) * 1
                     = sin(e)

ACN 3: Y_1^+1(a, e) = 1 * cos(e) * cos(a)
                     = cos(a) * cos(e)
```

Note: All order-1 SN3D normalization factors equal 1, so the first-order encoding is simply the Cartesian unit vector components: `(y, z, x)` in ACN order.

#### Order 2 (5 channels, cumulative 9)

```
ACN 4: Y_2^-2(a, e) = (1/sqrt(12)) * 3*cos^2(e) * sin(2a)
                     = (3/sqrt(12)) * sin(2a) * cos^2(e)
                     = (sqrt(3)/2) * sin(2a) * cos^2(e)

ACN 5: Y_2^-1(a, e) = (1/sqrt(3)) * 3*sin(e)*cos(e) * sin(a)
                     = sqrt(3) * sin(a) * sin(e) * cos(e)
                     = (sqrt(3)/2) * sin(a) * sin(2e)

ACN 6: Y_2^0(a, e)  = 1 * (3*sin^2(e) - 1)/2 * 1
                     = (3*sin^2(e) - 1) / 2

ACN 7: Y_2^+1(a, e) = (1/sqrt(3)) * 3*sin(e)*cos(e) * cos(a)
                     = sqrt(3) * cos(a) * sin(e) * cos(e)
                     = (sqrt(3)/2) * cos(a) * sin(2e)

ACN 8: Y_2^+2(a, e) = (1/sqrt(12)) * 3*cos^2(e) * cos(2a)
                     = (sqrt(3)/2) * cos(2a) * cos^2(e)
```

#### Order 3 (7 channels, cumulative 16)

```
ACN 9:  Y_3^-3(a, e) = (1/sqrt(360)) * 15*cos^3(e) * sin(3a)
                      = (15/sqrt(360)) * sin(3a) * cos^3(e)
                      = sqrt(5/8) * sin(3a) * cos^3(e)

       Proof: (15)^2 / 360 = 225/360 = 5/8, so 15/sqrt(360) = sqrt(225/360) = sqrt(5/8)

ACN 10: Y_3^-2(a, e) = (1/sqrt(60)) * 15*sin(e)*cos^2(e) * sin(2a)
                      = (15/sqrt(60)) * sin(2a) * sin(e) * cos^2(e)
                      = (sqrt(15)/2) * sin(2a) * sin(e) * cos^2(e)

       Proof: (15)^2 / 60 = 225/60 = 15/4, so 15/sqrt(60) = sqrt(15/4) = sqrt(15)/2

ACN 11: Y_3^-1(a, e) = (1/sqrt(6)) * (3/2)*cos(e)*(5*sin^2(e)-1) * sin(a)
                      = (3/(2*sqrt(6))) * sin(a) * cos(e) * (5*sin^2(e) - 1)
                      = sqrt(3/8) * sin(a) * cos(e) * (5*sin^2(e) - 1)

       Proof: 9/(4*6) = 9/24 = 3/8, so 3/(2*sqrt(6)) = sqrt(9/(4*6)) = sqrt(3/8)

ACN 12: Y_3^0(a, e)  = 1 * sin(e)*(5*sin^2(e)-3)/2 * 1
                      = sin(e) * (5*sin^2(e) - 3) / 2

ACN 13: Y_3^+1(a, e) = (1/sqrt(6)) * (3/2)*cos(e)*(5*sin^2(e)-1) * cos(a)
                      = sqrt(3/8) * cos(a) * cos(e) * (5*sin^2(e) - 1)

ACN 14: Y_3^+2(a, e) = (1/sqrt(60)) * 15*sin(e)*cos^2(e) * cos(2a)
                      = (sqrt(15)/2) * cos(2a) * sin(e) * cos^2(e)

ACN 15: Y_3^+3(a, e) = (1/sqrt(360)) * 15*cos^3(e) * cos(3a)
                      = sqrt(5/8) * cos(3a) * cos^3(e)
```

### 2.8 Summary: All 16 SN3D Encoding Coefficients

For a mono source signal `S` at direction `(a, e)`, the 16-channel AmbiX encoding is:

```
encoded[ACN] = S * Y_n^m(a, e)
```

| ACN | Encoding coefficient Y_n^m(a, e)                               | Compact constant    |
|-----|----------------------------------------------------------------|---------------------|
| 0   | 1                                                              |                     |
| 1   | sin(a) * cos(e)                                                |                     |
| 2   | sin(e)                                                         |                     |
| 3   | cos(a) * cos(e)                                                |                     |
| 4   | sqrt(3)/2 * sin(2a) * cos^2(e)                                 | 0.86603             |
| 5   | sqrt(3)/2 * sin(a) * sin(2e)                                   | 0.86603             |
| 6   | (3*sin^2(e) - 1) / 2                                          |                     |
| 7   | sqrt(3)/2 * cos(a) * sin(2e)                                   | 0.86603             |
| 8   | sqrt(3)/2 * cos(2a) * cos^2(e)                                 | 0.86603             |
| 9   | sqrt(5/8) * sin(3a) * cos^3(e)                                 | 0.79057             |
| 10  | sqrt(15)/2 * sin(2a) * sin(e) * cos^2(e)                       | 1.93649             |
| 11  | sqrt(3/8) * sin(a) * cos(e) * (5*sin^2(e) - 1)                | 0.61237             |
| 12  | sin(e) * (5*sin^2(e) - 3) / 2                                 |                     |
| 13  | sqrt(3/8) * cos(a) * cos(e) * (5*sin^2(e) - 1)                | 0.61237             |
| 14  | sqrt(15)/2 * cos(2a) * sin(e) * cos^2(e)                       | 1.93649             |
| 15  | sqrt(5/8) * cos(3a) * cos^3(e)                                 | 0.79057             |

---

## 3. Optimized C++ Implementation

### 3.1 Compile-Time Constants

```cpp
// SN3D normalization constants (precomputed at compile time)
static constexpr float kSqrt3_2   = 0.86602540378f;    // sqrt(3)/2
static constexpr float kSqrt3     = 1.73205080757f;    // sqrt(3)
static constexpr float kSqrt15_2  = 1.93649167310f;    // sqrt(15)/2
static constexpr float kSqrt5_8   = 0.79056941504f;    // sqrt(5/8)
static constexpr float kSqrt3_8   = 0.61237243570f;    // sqrt(3/8)
```

### 3.2 Precomputed Trigonometric Values

The key optimization insight: all 16 encoding coefficients use only `sin(a)`, `cos(a)`, `sin(e)`, `cos(e)`, and their powers. Higher-order azimuthal terms (`sin(2a)`, `cos(2a)`, `sin(3a)`, `cos(3a)`) are derived from the base terms via trigonometric identities, avoiding additional calls to `std::sin`/`std::cos`.

**Trigonometric identities used:**

```
sin(2a) = 2 * sin(a) * cos(a)
cos(2a) = cos^2(a) - sin^2(a)        (or equivalently: 2*cos^2(a) - 1)
sin(3a) = 3*sin(a)*cos^2(a) - sin^3(a) = sin(a)*(3*cos^2(a) - sin^2(a))
cos(3a) = cos^3(a) - 3*cos(a)*sin^2(a) = cos(a)*(cos^2(a) - 3*sin^2(a))
sin(2e) = 2 * sin(e) * cos(e)
```

This reduces the entire coefficient computation to exactly **2 transcendental function calls**: `sinf(a)` and `sinf(e)` (with `cos` derived from `sin` via `cos(x) = sqrt(1 - sin^2(x))` or computed alongside via `sincosf` where available).

```cpp
struct AmbiTrigCache
{
    float sinA, cosA;
    float sinE, cosE;
    float sin2A, cos2A;
    float sin3A, cos3A;
    float cosE2, cosE3;    // cos^2(e), cos^3(e)
    float sinE2;            // sin^2(e)
    float sin2E;            // sin(2e)

    void compute (float azimuth, float elevation)
    {
        // Only 2 trig calls needed (or use sincosf for each pair)
        sinA = std::sin (azimuth);
        cosA = std::cos (azimuth);
        sinE = std::sin (elevation);
        cosE = std::cos (elevation);

        // Double-angle via identity (zero additional trig calls)
        sin2A = 2.0f * sinA * cosA;
        cos2A = cosA * cosA - sinA * sinA;

        // Triple-angle via identity (zero additional trig calls)
        float sinA2 = sinA * sinA;
        float cosA2 = cosA * cosA;
        sin3A = sinA * (3.0f * cosA2 - sinA2);
        cos3A = cosA * (cosA2 - 3.0f * sinA2);

        // Powers of elevation trig
        cosE2 = cosE * cosE;
        cosE3 = cosE2 * cosE;
        sinE2 = sinE * sinE;
        sin2E = 2.0f * sinE * cosE;
    }
};
```

### 3.3 FOA Encoding Function (4 Channels)

For applications requiring only first-order ambisonics (4 channels):

```cpp
// Compute FOA (1st-order) encoding coefficients for direction (azimuth, elevation).
// Writes 4 coefficients to outCoeffs[0..3].
inline void computeFOACoeffs (float azimuth, float elevation,
                              float* __restrict outCoeffs)
{
    const float sinA = std::sin (azimuth);
    const float cosA = std::cos (azimuth);
    const float sinE = std::sin (elevation);
    const float cosE = std::cos (elevation);

    outCoeffs[0] = 1.0f;               // ACN 0: W
    outCoeffs[1] = sinA * cosE;         // ACN 1: Y
    outCoeffs[2] = sinE;                // ACN 2: Z
    outCoeffs[3] = cosA * cosE;         // ACN 3: X
}

// Encode a single sample into FOA and ACCUMULATE into output buffer.
inline void encodeSampleFOA (float sample,
                             const float* __restrict coeffs,
                             float* __restrict ambiOut)
{
    ambiOut[0] += sample * coeffs[0];
    ambiOut[1] += sample * coeffs[1];
    ambiOut[2] += sample * coeffs[2];
    ambiOut[3] += sample * coeffs[3];
}
```

### 3.4 HOA3 Encoding Function (16 Channels)

```cpp
static constexpr int kHOA3Channels = 16;

// Compute all 16 SN3D encoding coefficients for direction (azimuth, elevation).
// Uses trig identity optimization to avoid redundant transcendental calls.
inline void computeHOA3Coeffs (float azimuth, float elevation,
                               float* __restrict outCoeffs)
{
    AmbiTrigCache c;
    c.compute (azimuth, elevation);

    // Order 0
    outCoeffs[0]  = 1.0f;

    // Order 1
    outCoeffs[1]  = c.sinA * c.cosE;
    outCoeffs[2]  = c.sinE;
    outCoeffs[3]  = c.cosA * c.cosE;

    // Order 2
    outCoeffs[4]  = kSqrt3_2 * c.sin2A * c.cosE2;
    outCoeffs[5]  = kSqrt3_2 * c.sinA * c.sin2E;
    outCoeffs[6]  = 0.5f * (3.0f * c.sinE2 - 1.0f);
    outCoeffs[7]  = kSqrt3_2 * c.cosA * c.sin2E;
    outCoeffs[8]  = kSqrt3_2 * c.cos2A * c.cosE2;

    // Order 3
    outCoeffs[9]  = kSqrt5_8 * c.sin3A * c.cosE3;
    outCoeffs[10] = kSqrt15_2 * c.sin2A * c.sinE * c.cosE2;
    outCoeffs[11] = kSqrt3_8 * c.sinA * c.cosE * (5.0f * c.sinE2 - 1.0f);
    outCoeffs[12] = 0.5f * c.sinE * (5.0f * c.sinE2 - 3.0f);
    outCoeffs[13] = kSqrt3_8 * c.cosA * c.cosE * (5.0f * c.sinE2 - 1.0f);
    outCoeffs[14] = kSqrt15_2 * c.cos2A * c.sinE * c.cosE2;
    outCoeffs[15] = kSqrt5_8 * c.cos3A * c.cosE3;
}

// Encode a single sample into HOA3 and ACCUMULATE into output buffer.
// coeffs: precomputed via computeHOA3Coeffs() for the grain's position.
// ambiOut: pointer to 16-channel interleaved output (one sample position).
inline void encodeSampleHOA3 (float sample,
                              const float* __restrict coeffs,
                              float* __restrict ambiOut)
{
    for (int ch = 0; ch < kHOA3Channels; ++ch)
        ambiOut[ch] += sample * coeffs[ch];
}
```

### 3.5 SIMD Vectorization: Encoding 4 Channels at Once

The encoding inner loop (16 multiply-accumulates per sample per grain) is highly SIMD-friendly because it processes independent channels with the same scalar sample value.

#### SSE Implementation (x86_64)

```cpp
#include <immintrin.h>  // SSE/AVX intrinsics

// Encode one sample into HOA3 using SSE: 4 channels per iteration.
// coeffs and ambiOut MUST be 16-byte aligned.
inline void encodeSampleHOA3_SSE (float sample,
                                  const float* __restrict coeffs,
                                  float* __restrict ambiOut)
{
    __m128 s = _mm_set1_ps (sample);  // broadcast sample to all 4 lanes

    // Process 4 channels per iteration, 4 iterations for 16 channels
    for (int ch = 0; ch < 16; ch += 4)
    {
        __m128 c   = _mm_load_ps (&coeffs[ch]);
        __m128 out = _mm_load_ps (&ambiOut[ch]);
        out = _mm_add_ps (out, _mm_mul_ps (s, c));  // out += sample * coeffs
        _mm_store_ps (&ambiOut[ch], out);
    }
}
```

#### NEON Implementation (ARM/Apple Silicon)

```cpp
#include <arm_neon.h>

// Encode one sample into HOA3 using NEON: 4 channels per iteration.
inline void encodeSampleHOA3_NEON (float sample,
                                   const float* __restrict coeffs,
                                   float* __restrict ambiOut)
{
    float32x4_t s = vdupq_n_f32 (sample);

    for (int ch = 0; ch < 16; ch += 4)
    {
        float32x4_t c   = vld1q_f32 (&coeffs[ch]);
        float32x4_t out = vld1q_f32 (&ambiOut[ch]);
        out = vmlaq_f32 (out, s, c);   // fused multiply-accumulate: out += s * c
        vst1q_f32 (&ambiOut[ch], out);
    }
}
```

#### Platform-Agnostic Wrapper

```cpp
inline void encodeSampleHOA3_SIMD (float sample,
                                    const float* __restrict coeffs,
                                    float* __restrict ambiOut)
{
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    encodeSampleHOA3_NEON (sample, coeffs, ambiOut);
#elif defined(__SSE2__)
    encodeSampleHOA3_SSE (sample, coeffs, ambiOut);
#else
    encodeSampleHOA3 (sample, coeffs, ambiOut);  // scalar fallback
#endif
}
```

**Performance:** 16 scalar multiply-accumulates become 4 SIMD iterations, each processing 4 channels. NEON's `vmlaq_f32` is a fused multiply-accumulate (FMA), reducing both latency and instruction count. On Apple Silicon (M1/M2/M3), this completes in approximately 4 cycles per grain per sample.

### 3.6 Batch Encoding: Processing a Block of Samples for One Grain

When a grain's position does not change within an audio block (the common case for static grains or grains whose position updates at block boundaries), the SH coefficients are constant across all samples. This enables a highly efficient batch encoding pattern.

```cpp
// Encode an entire block of mono grain audio into the 16-channel ambisonics bus.
// grainAudio: mono samples for this grain (blockSize floats)
// coeffs:     16 SH coefficients for this grain's position (constant for this block)
// ambiBus:    16-channel output buffer, each channel has blockSize samples
// blockSize:  number of samples to process
void encodeGrainBlock (const float* __restrict grainAudio,
                       const float* __restrict coeffs,
                       juce::AudioBuffer<float>& ambiBus,
                       int blockSize)
{
    for (int ch = 0; ch < kHOA3Channels; ++ch)
    {
        float* __restrict dest = ambiBus.getWritePointer (ch);
        const float coeff = coeffs[ch];

        // This inner loop is trivially auto-vectorizable by the compiler
        for (int s = 0; s < blockSize; ++s)
            dest[s] += grainAudio[s] * coeff;
    }
}
```

**Why this layout is fast:** The inner loop (over samples) accesses contiguous memory in both `grainAudio` and `dest`, which maximizes cache line utilization and enables compiler auto-vectorization. The coefficient is hoisted out of the inner loop as a scalar broadcast.

**Alternative: JUCE FloatVectorOperations**

JUCE provides SIMD-optimized vector math that dispatches to the best available instruction set:

```cpp
void encodeGrainBlock_JUCE (const float* __restrict grainAudio,
                            const float* __restrict coeffs,
                            juce::AudioBuffer<float>& ambiBus,
                            int blockSize)
{
    for (int ch = 0; ch < kHOA3Channels; ++ch)
    {
        // addWithMultiply: dest[i] += src[i] * multiplier
        juce::FloatVectorOperations::addWithMultiply (
            ambiBus.getWritePointer (ch),    // destination (accumulate)
            grainAudio,                       // source samples
            coeffs[ch],                       // scalar multiplier (SH coefficient)
            blockSize);
    }
}
```

`FloatVectorOperations::addWithMultiply` is SIMD-optimized internally and handles alignment. This is the recommended approach for JUCE plugins as it avoids platform-specific intrinsic code while delivering near-optimal performance.

### 3.7 Smoothing Strategy: Coefficient Interpolation

When a grain's position changes (due to trajectory movement or user parameter changes), directly snapping to new SH coefficients causes audible clicks ("zipper noise"). The correct strategy is to **smooth the SH coefficients directly** rather than smoothing the angles and recomputing trig per sample.

**Why smooth coefficients, not angles:**
- Smoothing angles would require `sin`/`cos` evaluation at every sample to recompute coefficients. At 48 kHz with 64 grains, that is 128 trig calls per sample = 6.1 million trig calls per second.
- Smoothing the 16 coefficients requires only 16 linear interpolations per sample per grain = 16 multiplies + 16 adds. This is identical in cost to the encoding itself.
- Linear interpolation of SH coefficients produces a perceptually smooth spatial transition. The path on the sphere is not a geodesic (it is a chord), but for small angular changes per block, the difference is inaudible.

```cpp
struct SmoothedSHCoeffs
{
    static constexpr int kNumChannels = 16;

    // Current and target coefficient arrays
    alignas(16) float current[kNumChannels] {};
    alignas(16) float target[kNumChannels] {};
    float smoothingCoeff = 0.0f;  // one-pole coefficient (0..1)

    void prepare (float sampleRate, float smoothTimeMs = 5.0f)
    {
        // One-pole smoothing: coefficient = 1 - e^(-1 / (tau * sr))
        float tau = smoothTimeMs * 0.001f;
        smoothingCoeff = 1.0f - std::exp (-1.0f / (tau * sampleRate));
    }

    void setTarget (float azimuth, float elevation)
    {
        computeHOA3Coeffs (azimuth, elevation, target);
    }

    // Call once per sample to advance smoothing.
    // After calling, 'current' contains the smoothed coefficients.
    void advance()
    {
        for (int ch = 0; ch < kNumChannels; ++ch)
            current[ch] += smoothingCoeff * (target[ch] - current[ch]);
    }

    // Snap immediately (use at grain onset)
    void snapToTarget()
    {
        std::copy (std::begin (target), std::end (target), std::begin (current));
    }

    // Check if smoothing is complete (within tolerance)
    bool isSmoothing() const
    {
        for (int ch = 0; ch < kNumChannels; ++ch)
            if (std::abs (target[ch] - current[ch]) > 1e-6f)
                return true;
        return false;
    }
};
```

**Usage pattern:**

```cpp
// When grain position updates (e.g., trajectory tick):
grain.shCoeffs.setTarget (newAzimuth, newElevation);

// In processBlock, if this grain is smoothing:
if (grain.shCoeffs.isSmoothing())
{
    // Per-sample encoding with smoothed coefficients
    for (int s = 0; s < blockSize; ++s)
    {
        grain.shCoeffs.advance();
        encodeSampleHOA3_SIMD (grainSample[s], grain.shCoeffs.current, ambiBusSample);
    }
}
else
{
    // Block encoding with constant coefficients (fast path)
    encodeGrainBlock_JUCE (grainAudio, grain.shCoeffs.current, ambiBus, blockSize);
}
```

**Smoothing time recommendations:**
- 5 ms: good default, prevents clicks without audible spatial blur
- 1-2 ms: for fast trajectories (orbital, spinning) where latency matters
- 10-20 ms: for very slow, smooth parameter sweeps from the UI

---

## 4. Per-Grain Encoding Architecture

### 4.1 Integration with a 64-Voice Grain Pool

This section describes how to integrate HOA3 encoding with a grain pool similar to O-GrainScatter's `GrainPool` (64 voices, round-robin allocation, Hann window envelope).

#### Extended GrainVoice Structure

```cpp
struct SpatialGrainVoice
{
    // --- Existing grain state (from GrainPool) ---
    bool active = false;
    float readPosition = 0.0f;
    float playbackRate = 1.0f;
    int samplesRemaining = 0;
    int grainLengthSamples = 0;
    bool reverse = false;
    bool readFromFrozen = false;
    float positionOffset = 0.0f;

    // --- Spatial state (new) ---
    alignas(16) float shCoeffs[16] {};        // Current SH encoding coefficients
    alignas(16) float shCoeffsTarget[16] {};   // Target coefficients (for smoothing)
    float distanceGain = 1.0f;                 // Distance attenuation multiplier

    // Position in spherical coordinates
    float azimuth = 0.0f;
    float elevation = 0.0f;
    float distance = 1.0f;
};
```

### 4.2 Memory Layout

Each grain stores its 16 SH coefficients inline:

```
Per-grain spatial overhead:
  SH coefficients (current):   16 * 4 bytes =  64 bytes (aligned to 16 for SIMD)
  SH coefficients (target):    16 * 4 bytes =  64 bytes (aligned to 16 for SIMD)
  Distance gain:                1 * 4 bytes =   4 bytes
  Position (az, el, dist):      3 * 4 bytes =  12 bytes
  -----------------------------------------------
  Total spatial state per grain:              144 bytes

64-grain pool spatial overhead:
  144 * 64 = 9,216 bytes = 9 KB
```

This fits comfortably in L1 cache (typically 32-64 KB per core). During the encoding inner loop, only `shCoeffs[16]` (64 bytes = one cache line) is accessed per grain per sample, ensuring excellent cache locality.

### 4.3 Grain Spawning with Spatial Parameters

```cpp
struct SpatialGrainParams
{
    // Audio parameters
    float positionInSamples = 0.0f;
    float playbackRate = 1.0f;
    int grainLengthSamples = 4410;
    bool reverse = false;
    bool readFromFrozen = false;

    // Spatial parameters
    float azimuth = 0.0f;        // radians, 0 = front
    float elevation = 0.0f;      // radians, 0 = horizon
    float distance = 1.0f;       // meters
};

void spawnGrain (const SpatialGrainParams& params)
{
    // ... (existing round-robin voice allocation) ...

    auto& v = voices[targetVoice];
    v.active = true;
    v.positionOffset = params.positionInSamples;
    v.playbackRate = params.playbackRate;
    v.grainLengthSamples = params.grainLengthSamples;
    v.samplesRemaining = params.grainLengthSamples;
    v.reverse = params.reverse;
    v.readFromFrozen = params.readFromFrozen;
    v.readPosition = params.reverse
        ? static_cast<float> (params.grainLengthSamples - 1) : 0.0f;

    // Spatial initialization: snap coefficients (no smoothing at onset)
    v.azimuth = params.azimuth;
    v.elevation = params.elevation;
    v.distance = params.distance;
    v.distanceGain = 1.0f / std::max (params.distance, 0.1f);
    computeHOA3Coeffs (params.azimuth, params.elevation, v.shCoeffs);
    std::copy (v.shCoeffs, v.shCoeffs + 16, v.shCoeffsTarget);
}
```

### 4.4 ProcessBlock Flow: Complete Inner Loop

The processBlock architecture follows this flow:

```
1. Zero the 16-channel ambisonics bus
2. For each active grain:
   a. Read grain audio samples (mono) from source buffer
   b. Apply grain envelope (Hann window)
   c. Apply distance attenuation
   d. Encode into ambisonics bus via precomputed SH coefficients
3. Output the 16-channel bus
```

#### Full Implementation

```cpp
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    const int blockSize = buffer.getNumSamples();

    // Step 1: Zero the ambisonics bus
    ambiBus.clear (0, blockSize);

    // Scratch buffer for one grain's mono audio (stack-allocated for small blocks)
    alignas(16) float grainMono[2048];
    jassert (blockSize <= 2048);

    // Step 2: Process each active grain
    for (auto& grain : grainPool.getVoices())
    {
        if (! grain.active)
            continue;

        // Step 2a: Read grain audio samples into mono scratch buffer
        // Step 2b: Apply envelope simultaneously
        const int samplesToProcess = std::min (blockSize, grain.samplesRemaining);

        for (int s = 0; s < samplesToProcess; ++s)
        {
            // Hann envelope
            float phase = 1.0f - static_cast<float> (grain.samplesRemaining - s)
                                / static_cast<float> (grain.grainLengthSamples);
            float envelope = 0.5f * (1.0f - std::cos (
                juce::MathConstants<float>::twoPi * phase));

            // Read from source (delay buffer or frozen buffer)
            float sample = readGrainSample (grain, s);  // your existing read logic

            // Step 2c: Apply distance attenuation and envelope
            grainMono[s] = sample * envelope * grain.distanceGain;

            // Advance read position
            grain.readPosition += grain.reverse ? -grain.playbackRate : grain.playbackRate;
        }

        // Zero remaining samples if grain ends mid-block
        for (int s = samplesToProcess; s < blockSize; ++s)
            grainMono[s] = 0.0f;

        // Step 2d: Encode mono grain audio into 16-channel ambisonics bus
        // Uses the batch encoding path (constant coefficients for this block)
        encodeGrainBlock_JUCE (grainMono, grain.shCoeffs, ambiBus, blockSize);

        // Update grain state
        grain.samplesRemaining -= samplesToProcess;
        if (grain.samplesRemaining <= 0)
            grain.active = false;
    }

    // Step 3: Copy ambisonics bus to output buffer
    const int outputChannels = buffer.getNumChannels();
    const int channelsToCopy = std::min (outputChannels, kHOA3Channels);

    for (int ch = 0; ch < channelsToCopy; ++ch)
        buffer.copyFrom (ch, 0, ambiBus, ch, 0, blockSize);

    // Clear any extra output channels beyond what we produce
    for (int ch = channelsToCopy; ch < outputChannels; ++ch)
        buffer.clear (ch, 0, blockSize);
}
```

### 4.5 Moving Grains: Per-Sample Coefficient Interpolation Path

For grains with trajectories (positions that change during the block), the encoding switches to a per-sample path with coefficient smoothing:

```cpp
void processGrainWithTrajectory (SpatialGrainVoice& grain,
                                 juce::AudioBuffer<float>& ambiBus,
                                 int blockSize)
{
    // Update target position (e.g., from trajectory tick)
    grain.azimuth += grain.azimuthRate * blockSize;  // angular velocity
    grain.elevation += grain.elevationRate * blockSize;
    computeHOA3Coeffs (grain.azimuth, grain.elevation, grain.shCoeffsTarget);

    // One-pole smoothing coefficient (precomputed in prepareToPlay)
    const float smoothCoeff = smoothingCoefficient;

    for (int s = 0; s < blockSize; ++s)
    {
        // Advance coefficient smoothing
        for (int ch = 0; ch < kHOA3Channels; ++ch)
            grain.shCoeffs[ch] += smoothCoeff * (grain.shCoeffsTarget[ch] - grain.shCoeffs[ch]);

        // Read and envelope the grain sample
        float sample = readAndEnvelopeGrainSample (grain, s);

        // Encode into ambisonics bus (per-sample)
        for (int ch = 0; ch < kHOA3Channels; ++ch)
            ambiBus.getWritePointer (ch)[s] += sample * grain.shCoeffs[ch];
    }
}
```

### 4.6 Performance Analysis

**Per-grain, per-sample cost (HOA3 batch encoding):**

| Operation                        | FLOPs per sample |
|----------------------------------|------------------|
| Read sample (interpolated)       | ~4               |
| Envelope (Hann)                  | ~5               |
| Distance gain multiply           | 1                |
| Ambisonics encoding (16 ch)      | 16 mul + 16 add = 32 |
| **Total per grain per sample**   | **~42**          |

**Aggregate cost for 64 grains, 48 kHz:**

```
42 FLOPs * 48,000 samples/sec * 64 grains = 129 MFLOPS
```

A single modern CPU core delivers 20-100 GFLOPS with SIMD. The encoding consumes approximately 0.1-0.6% of a core -- negligible. The real bottleneck in a granular synth is typically the grain read path (random access into delay buffer) and the envelope computation, not the spatial encoding.

---

## 5. Ambisonics Bus Accumulation

### 5.1 Buffer Management

#### Allocation in prepareToPlay

```cpp
class SpatialGranularProcessor : public juce::AudioProcessor
{
    juce::AudioBuffer<float> ambiBus;    // 16-channel HOA3 accumulation buffer
    float smoothingCoefficient = 0.0f;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        // Allocate 16-channel ambisonics bus
        ambiBus.setSize (kHOA3Channels, samplesPerBlock, false, true, false);
        // Args: numChannels, numSamples, keepExistingContent, clearExtraSpace, avoidReallocating

        // Precompute smoothing coefficient (5ms default)
        float smoothTimeMs = 5.0f;
        float tau = smoothTimeMs * 0.001f;
        smoothingCoefficient = 1.0f - std::exp (-1.0f / (tau * static_cast<float> (sampleRate)));
    }
};
```

### 5.2 Thread Safety

The ambisonics bus is accessed exclusively on the audio thread during `processBlock`. No locking is needed because:

1. The bus is allocated in `prepareToPlay` (which is guaranteed to be called before `processBlock` and not concurrently with it).
2. All grain processing occurs within a single `processBlock` call.
3. The bus is zero-initialized at the start of each block and written only by the current thread.

**Parameter changes from the UI thread** (azimuth, elevation, distance) should be communicated via atomic stores or a lock-free FIFO. The audio thread reads these atomically and updates grain positions. The SH coefficient recomputation happens on the audio thread.

```cpp
// Thread-safe parameter communication:
std::atomic<float> paramAzimuth { 0.0f };
std::atomic<float> paramElevation { 0.0f };

// In processBlock (audio thread):
float az = paramAzimuth.load (std::memory_order_relaxed);
float el = paramElevation.load (std::memory_order_relaxed);
```

### 5.3 Zero-Init Per Block, Accumulate, Output

The processBlock pattern is:

```cpp
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
{
    const int blockSize = buffer.getNumSamples();

    // 1. Zero the accumulation buffer
    ambiBus.clear (0, blockSize);

    // 2. Accumulate all grains
    for (auto& grain : grainPool.getVoices())
    {
        if (! grain.active) continue;
        processGrain (grain, ambiBus, blockSize);
    }

    // 3. (Optional) Apply bus-level processing
    applyBusProcessing (ambiBus, blockSize);

    // 4. Output to DAW buffer
    const int outCh = std::min (buffer.getNumChannels(), kHOA3Channels);
    for (int ch = 0; ch < outCh; ++ch)
        buffer.copyFrom (ch, 0, ambiBus, ch, 0, blockSize);
    for (int ch = outCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, blockSize);
}
```

### 5.4 DC Offset Prevention

DC offset can accumulate in ambisonics channels, especially ACN 0 (omnidirectional), when many grains overlap and their envelopes do not perfectly cancel. A simple per-channel DC blocker prevents drift:

```cpp
class DCBlocker
{
public:
    void reset() { x1 = 0.0f; y1 = 0.0f; }

    // Process one sample. R = 1 - (2*pi*fc/sr), fc ~ 5-20 Hz.
    // R = 0.9999 at 48kHz gives fc ~ 0.76 Hz (subsonic).
    float process (float x)
    {
        float y = x - x1 + R * y1;
        x1 = x;
        y1 = y;
        return y;
    }

private:
    static constexpr float R = 0.9999f;
    float x1 = 0.0f;
    float y1 = 0.0f;
};

// One DC blocker per ambisonics channel
std::array<DCBlocker, kHOA3Channels> dcBlockers;

void applyDCBlocking (juce::AudioBuffer<float>& bus, int blockSize)
{
    for (int ch = 0; ch < kHOA3Channels; ++ch)
    {
        float* data = bus.getWritePointer (ch);
        for (int s = 0; s < blockSize; ++s)
            data[s] = dcBlockers[ch].process (data[s]);
    }
}
```

### 5.5 Soft-Clipping for the Ambisonics Bus

When many grains simultaneously overlap at similar positions, the ambisonics bus can clip. However, **hard clipping on ambisonics channels destroys the spatial encoding** because the relative levels between channels carry directional information. A spatially-aware approach is essential.

**Option A: Per-channel soft-clip (simple but spatially imprecise)**

```cpp
// tanh soft-clipper: transparent below ~0.5, progressively limits peaks
inline float softClip (float x)
{
    return std::tanh (x);
}

void applySoftClipping (juce::AudioBuffer<float>& bus, int blockSize)
{
    for (int ch = 0; ch < kHOA3Channels; ++ch)
    {
        float* data = bus.getWritePointer (ch);
        for (int s = 0; s < blockSize; ++s)
            data[s] = softClip (data[s]);
    }
}
```

**Option B: Omnidirectional gain reduction (spatially transparent)**

The preferred approach for ambisonics is to compute the peak level from ACN 0 (which represents the overall energy) and apply a single gain reduction uniformly to all 16 channels. This preserves spatial encoding:

```cpp
void applyOmniLimiter (juce::AudioBuffer<float>& bus, int blockSize)
{
    const float* omni = bus.getReadPointer (0);  // ACN 0 = omnidirectional
    float* channels[kHOA3Channels];
    for (int ch = 0; ch < kHOA3Channels; ++ch)
        channels[ch] = bus.getWritePointer (ch);

    static float envelope = 0.0f;
    const float attack  = 0.001f;   // ~instant attack
    const float release = 0.1f;     // 100ms release
    const float threshold = 0.9f;   // start limiting at -0.9 dBFS on omni channel

    for (int s = 0; s < blockSize; ++s)
    {
        float absOmni = std::abs (omni[s]);

        // Envelope follower
        if (absOmni > envelope)
            envelope = attack * absOmni + (1.0f - attack) * envelope;
        else
            envelope = release * absOmni + (1.0f - release) * envelope;

        // Compute gain reduction
        float gain = 1.0f;
        if (envelope > threshold)
            gain = threshold / envelope;

        // Apply uniform gain to all channels (preserves spatial balance)
        for (int ch = 0; ch < kHOA3Channels; ++ch)
            channels[ch][s] *= gain;
    }
}
```

---

## 6. Normalization Comparison: SN3D vs N3D vs FuMa

### 6.1 Overview

Three normalization conventions exist for ambisonics. Understanding the differences is critical when interfacing with external tools, DAWs, or legacy content.

| Property          | SN3D (AmbiX)        | N3D                  | FuMa (maxN)                |
|-------------------|---------------------|----------------------|----------------------------|
| Channel ordering  | ACN                 | ACN                  | FuMa (W,X,Y,Z,R,S,T,U,V,...) |
| Standard          | AmbiX (de facto)    | MPEG-H              | Furse-Malham (legacy)      |
| Max level (any ch)| 1.0 (W channel)     | Grows with order     | 1.0 (per channel)          |
| JUCE support      | Native              | Convert from SN3D    | Convert from SN3D          |
| Max order         | Unlimited           | Unlimited            | 3rd order only             |

### 6.2 SN3D to N3D Conversion

The relationship is a simple per-order scaling:

```
N3D_n^m = sqrt(2n + 1) * SN3D_n^m
```

| Order (n) | Scale factor sqrt(2n+1) | Decimal    |
|-----------|------------------------|------------|
| 0         | sqrt(1) = 1            | 1.0000     |
| 1         | sqrt(3)                | 1.7321     |
| 2         | sqrt(5)                | 2.2361     |
| 3         | sqrt(7)                | 2.6458     |

**C++ conversion:**

```cpp
// Convert 16-channel SN3D buffer to N3D in-place
void sn3dToN3D (float* coeffs)
{
    // Order 0: no change (factor = 1)
    // Order 1: multiply by sqrt(3)
    for (int ch = 1; ch <= 3; ++ch)
        coeffs[ch] *= 1.73205080757f;
    // Order 2: multiply by sqrt(5)
    for (int ch = 4; ch <= 8; ++ch)
        coeffs[ch] *= 2.23606797750f;
    // Order 3: multiply by sqrt(7)
    for (int ch = 9; ch <= 15; ++ch)
        coeffs[ch] *= 2.64575131106f;
}

// Convert N3D to SN3D in-place
void n3dToSN3D (float* coeffs)
{
    for (int ch = 1; ch <= 3; ++ch)
        coeffs[ch] *= 0.57735026919f;    // 1/sqrt(3)
    for (int ch = 4; ch <= 8; ++ch)
        coeffs[ch] *= 0.44721359550f;    // 1/sqrt(5)
    for (int ch = 9; ch <= 15; ++ch)
        coeffs[ch] *= 0.37796447301f;    // 1/sqrt(7)
}
```

### 6.3 FuMa to ACN/SN3D Conversion

FuMa uses a different channel ordering AND different normalization. The conversion requires both reordering and rescaling.

#### Channel Reordering

| FuMa index | FuMa name | ACN index | ACN (n,m) |
|------------|-----------|-----------|-----------|
| 0          | W         | 0         | (0, 0)    |
| 1          | X         | 3         | (1, +1)   |
| 2          | Y         | 1         | (1, -1)   |
| 3          | Z         | 2         | (1, 0)    |
| 4          | R         | 6         | (2, 0)    |
| 5          | S         | 7         | (2, +1)   |
| 6          | T         | 5         | (2, -1)   |
| 7          | U         | 8         | (2, +2)   |
| 8          | V         | 4         | (2, -2)   |
| 9          | K         | 12        | (3, 0)    |
| 10         | L         | 13        | (3, +1)   |
| 11         | M         | 11        | (3, -1)   |
| 12         | N         | 14        | (3, +2)   |
| 13         | O         | 10        | (3, -2)   |
| 14         | P         | 15        | (3, +3)   |
| 15         | Q         | 9         | (3, -3)   |

#### Normalization Scale Factors (FuMa -> SN3D)

To convert a FuMa-normalized signal to SN3D, multiply the FuMa channel value by the factor below AFTER reordering to ACN:

| ACN | FuMa ch | FuMa -> SN3D factor | Decimal     | Notes                        |
|-----|---------|---------------------|-------------|------------------------------|
| 0   | W       | sqrt(2)             | 1.41421     | FuMa W = 1/sqrt(2) of SN3D  |
| 1   | Y       | 1                   | 1.00000     |                              |
| 2   | Z       | 1                   | 1.00000     |                              |
| 3   | X       | 1                   | 1.00000     |                              |
| 4   | V       | 2/sqrt(3)           | 1.15470     |                              |
| 5   | T       | 2/sqrt(3)           | 1.15470     |                              |
| 6   | R       | 1                   | 1.00000     |                              |
| 7   | S       | 2/sqrt(3)           | 1.15470     |                              |
| 8   | U       | 2/sqrt(3)           | 1.15470     |                              |
| 9   | Q       | 8/(3*sqrt(5))       | 1.19257     |                              |
| 10  | O       | 2*sqrt(2/5)         | 1.26491     | = sqrt(8/5)                  |
| 11  | M       | sqrt(32/45)         | 0.84327     | = 4*sqrt(2)/(3*sqrt(5))      |
| 12  | K       | 1                   | 1.00000     |                              |
| 13  | L       | sqrt(32/45)         | 0.84327     |                              |
| 14  | N       | 2*sqrt(2/5)         | 1.26491     |                              |
| 15  | P       | 8/(3*sqrt(5))       | 1.19257     |                              |

**Note:** The 3rd-order FuMa conversion factors are less standardized across implementations. Some sources use slightly different factors depending on which "maxN" variant is applied. When interfacing with FuMa content, always verify against the specific encoder/decoder documentation.

#### C++ Conversion Function

```cpp
// Convert 16-channel FuMa (maxN normalization, FuMa ordering) to ACN/SN3D in-place.
// Input: fumaChannels[0..15] in FuMa order (W,X,Y,Z,R,S,T,U,V,K,L,M,N,O,P,Q)
// Output: acnChannels[0..15] in ACN order with SN3D normalization
void fumaToAmbiX (const float* fumaChannels, float* acnChannels)
{
    // Reorder and rescale simultaneously
    acnChannels[0]  = fumaChannels[0]  * 1.41421356f;   // W -> ACN 0
    acnChannels[1]  = fumaChannels[2]  * 1.00000000f;   // Y -> ACN 1
    acnChannels[2]  = fumaChannels[3]  * 1.00000000f;   // Z -> ACN 2
    acnChannels[3]  = fumaChannels[1]  * 1.00000000f;   // X -> ACN 3
    acnChannels[4]  = fumaChannels[8]  * 1.15470054f;   // V -> ACN 4
    acnChannels[5]  = fumaChannels[6]  * 1.15470054f;   // T -> ACN 5
    acnChannels[6]  = fumaChannels[4]  * 1.00000000f;   // R -> ACN 6
    acnChannels[7]  = fumaChannels[5]  * 1.15470054f;   // S -> ACN 7
    acnChannels[8]  = fumaChannels[7]  * 1.15470054f;   // U -> ACN 8
    acnChannels[9]  = fumaChannels[15] * 1.19257284f;   // Q -> ACN 9
    acnChannels[10] = fumaChannels[13] * 1.26491106f;   // O -> ACN 10
    acnChannels[11] = fumaChannels[11] * 0.84327404f;   // M -> ACN 11
    acnChannels[12] = fumaChannels[9]  * 1.00000000f;   // K -> ACN 12
    acnChannels[13] = fumaChannels[10] * 0.84327404f;   // L -> ACN 13
    acnChannels[14] = fumaChannels[12] * 1.26491106f;   // N -> ACN 14
    acnChannels[15] = fumaChannels[14] * 1.19257284f;   // P -> ACN 15
}
```

### 6.4 Practical Recommendations

1. **Always use AmbiX (ACN/SN3D) internally.** This is the standard for JUCE, VST3, AU, and modern DAWs.

2. **Only convert to/from FuMa at I/O boundaries** (loading legacy files, interfacing with old plugins). Never mix formats internally.

3. **N3D is mainly relevant for MPEG-H workflows.** If not targeting broadcast, SN3D is sufficient.

4. **SN3D peak levels never exceed 1.0** for any single point source at any direction. This is a safety property that simplifies gain staging. N3D channels can exceed 1.0 at higher orders (up to sqrt(2n+1) times louder than SN3D).

---

## Appendix A: Quick Reference Card

### Encoding a mono source at (azimuth a, elevation e) into HOA3 AmbiX:

```
ACN 0:  1
ACN 1:  sin(a) * cos(e)
ACN 2:  sin(e)
ACN 3:  cos(a) * cos(e)
ACN 4:  0.86603 * sin(2a) * cos^2(e)
ACN 5:  0.86603 * sin(a) * sin(2e)
ACN 6:  0.5 * (3*sin^2(e) - 1)
ACN 7:  0.86603 * cos(a) * sin(2e)
ACN 8:  0.86603 * cos(2a) * cos^2(e)
ACN 9:  0.79057 * sin(3a) * cos^3(e)
ACN 10: 1.93649 * sin(2a) * sin(e) * cos^2(e)
ACN 11: 0.61237 * sin(a) * cos(e) * (5*sin^2(e) - 1)
ACN 12: 0.5 * sin(e) * (5*sin^2(e) - 3)
ACN 13: 0.61237 * cos(a) * cos(e) * (5*sin^2(e) - 1)
ACN 14: 1.93649 * cos(2a) * sin(e) * cos^2(e)
ACN 15: 0.79057 * cos(3a) * cos^3(e)
```

### Trig identity cheat sheet (compute sin/cos of a and e only once):

```
sin(2a) = 2 * sin(a) * cos(a)
cos(2a) = cos(a)^2 - sin(a)^2
sin(3a) = sin(a) * (3*cos(a)^2 - sin(a)^2)
cos(3a) = cos(a) * (cos(a)^2 - 3*sin(a)^2)
sin(2e) = 2 * sin(e) * cos(e)
```

### JUCE bus setup:

```cpp
// In constructor:
BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                 .withOutput ("Output", juce::AudioChannelSet::ambisonic (3), true)

// In isBusesLayoutSupported:
int order = mainOutput.getAmbisonicOrder();
return order >= 1 && order <= 3;
```

---

## Appendix B: Verification Test Vectors

These test vectors can be used to validate an implementation. Each row shows the expected 16 SH coefficients for a source at a specific direction.

### Source at front (a=0, e=0):

```
ACN 0:  1.0000    ACN 1:  0.0000    ACN 2:  0.0000    ACN 3:  1.0000
ACN 4:  0.0000    ACN 5:  0.0000    ACN 6: -0.5000    ACN 7:  0.0000
ACN 8:  0.8660    ACN 9:  0.0000    ACN 10: 0.0000    ACN 11: 0.0000
ACN 12: 0.0000    ACN 13:-0.6124    ACN 14: 0.0000    ACN 15: 0.7906
```

Explanation: At front (a=0, e=0): sin(a)=0, cos(a)=1, sin(e)=0, cos(e)=1.
- ACN 3 = cos(0)*cos(0) = 1 (X channel, front)
- ACN 6 = (3*0 - 1)/2 = -0.5
- ACN 8 = sqrt(3)/2 * cos(0) * 1 = 0.8660
- ACN 12 = 0 * (0 - 3)/2 = 0
- ACN 13 = sqrt(3/8) * 1 * 1 * (0 - 1) = -0.6124
- ACN 15 = sqrt(5/8) * 1 * 1 = 0.7906

### Source at left (a=pi/2, e=0):

```
ACN 0:  1.0000    ACN 1:  1.0000    ACN 2:  0.0000    ACN 3:  0.0000
ACN 4:  0.0000    ACN 5:  0.0000    ACN 6: -0.5000    ACN 7:  0.0000
ACN 8: -0.8660    ACN 9: -0.7906    ACN 10: 0.0000    ACN 11:-0.6124
ACN 12: 0.0000    ACN 13: 0.0000    ACN 14: 0.0000    ACN 15: 0.0000
```

Explanation: At left (a=pi/2, e=0): sin(a)=1, cos(a)=0, sin(e)=0, cos(e)=1.
sin(3a)=sin(3*pi/2)=-1, so ACN 9 = sqrt(5/8)*(-1)*1 = -0.7906.
ACN 11 = sqrt(3/8)*1*1*(0-1) = -0.6124 (the 5*sin^2(e)-1 term equals -1 at horizon).

### Source directly above (a=0, e=pi/2):

```
ACN 0:  1.0000    ACN 1:  0.0000    ACN 2:  1.0000    ACN 3:  0.0000
ACN 4:  0.0000    ACN 5:  0.0000    ACN 6:  1.0000    ACN 7:  0.0000
ACN 8:  0.0000    ACN 9:  0.0000    ACN 10: 0.0000    ACN 11: 0.0000
ACN 12: 1.0000    ACN 13: 0.0000    ACN 14: 0.0000    ACN 15: 0.0000
```

Explanation: At zenith (a=0, e=pi/2): sin(e)=1, cos(e)=0. All terms with cos(e) vanish.
- ACN 2 = sin(pi/2) = 1
- ACN 6 = (3*1 - 1)/2 = 1
- ACN 12 = 1 * (5*1 - 3)/2 = 1

### Source at 45 degrees azimuth, 30 degrees elevation:

```
a = pi/4 = 0.7854 rad,  e = pi/6 = 0.5236 rad
sin(a) = 0.70711,  cos(a) = 0.70711
sin(e) = 0.50000,  cos(e) = 0.86603

ACN 0:  1.00000   ACN 1:  0.61237   ACN 2:  0.50000   ACN 3:  0.61237
ACN 4:  0.64952   ACN 5:  0.53033   ACN 6: -0.12500   ACN 7:  0.53033
ACN 8:  0.00000   ACN 9:  0.36310   ACN 10: 0.72618   ACN 11: 0.09375
ACN 12:-0.43750   ACN 13: 0.09375   ACN 14: 0.00000   ACN 15:-0.36310
```

Derivation of selected values:
- ACN 4 = sqrt(3)/2 * sin(pi/2) * cos^2(pi/6) = 0.86603 * 1.0 * 0.75 = 0.64952
- ACN 5 = sqrt(3)/2 * sin(pi/4) * sin(pi/3) = 0.86603 * 0.70711 * 0.86603 = 0.53033
- ACN 8 = sqrt(3)/2 * cos(pi/2) * cos^2(pi/6) = 0.86603 * 0.0 * 0.75 = 0.0
- ACN 10 = sqrt(15)/2 * sin(pi/2) * 0.5 * 0.75 = 1.93649 * 0.375 = 0.72618
- ACN 12 = 0.5 * 0.5 * (5*0.25 - 3) = 0.25 * (-1.75) = -0.43750
- ACN 15 = sqrt(5/8) * cos(3*pi/4) * cos^3(pi/6) = 0.79057 * (-0.70711) * 0.64952 = -0.36310

---

## Sources

### Mathematical References
- Angelo Farina - ACN-N3D Formulas for High Order Ambisonics: https://www.angelofarina.it/Aurora/HOA_ACN_N3D_formulas.htm
- Angelo Farina - Explicit AmbiX Formulas: http://pcfarina.eng.unipr.it/Aurora/HOA_explicit_formulas.htm
- Wikipedia - Ambisonic Data Exchange Formats: https://en.wikipedia.org/wiki/Ambisonic_data_exchange_formats
- Wikipedia - Associated Legendre Polynomials: https://en.wikipedia.org/wiki/Associated_Legendre_polynomials
- Wikipedia - Spherical Harmonics: https://en.wikipedia.org/wiki/Spherical_harmonics
- Blue Ripple Sound - B-Format Technical Notes (SN3D): https://www.blueripplesound.com/notes/bformat
- RingBuffer - Understanding Ambisonics: https://ringbuffer.org/spatial_audio/ambisonics/understanding-ambisonics/
- Christian Nachbar et al. - "AmbiX: A Suggested Ambisonics Format" (Ambisonics Symposium 2011): https://ambisonics.iem.at/proceedings-of-the-ambisonics-symposium-2011/ambix-a-suggested-ambisonics-format

### Implementation References
- Google Spherical Harmonics Library: https://github.com/google/spherical-harmonics
- IEM Plugin Suite (GranularEncoder, StereoEncoder): https://github.com/tu-studio/IEMPluginSuite
- Spatial Audio Framework (SAF/SPARTA): https://github.com/leomccormack/Spatial_Audio_Framework
- polarch/Higher-Order-Ambisonics: https://github.com/polarch/Higher-Order-Ambisonics
- sse2neon (SSE to NEON translator): https://github.com/DLTcollab/sse2neon

### JUCE References
- JUCE AudioChannelSet: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/buffers/juce_AudioChannelSet.h`
- JUCE AudioProcessor: `/Users/taylorbrook/JUCE/modules/juce_audio_processors/processors/juce_AudioProcessor.h`
- Prior research: `/Users/taylorbrook/Dev/VST-development/research/juce8-multichannel-spatial-audio.md`
- Prior research: `/Users/taylorbrook/Dev/VST-development/research/spatial-audio-per-grain-spatialization.md`
- Reference grain pool: `/Users/taylorbrook/Dev/VST-development/plugins/O-GrainScatter/Source/dsp/GrainPool.h`

### Normalization References
- SuperCollider HOA Tutorial - Converting SN3D, N3D, FuMa: https://depts.washington.edu/dxscdoc/Help/Tutorials/Exercise_02_HOA_converting_SN3D_N3D_FuMa.html
- IEM Plugin Suite Compatibility (format conversion): https://plugins.iem.at/docs/compatibility/
- Ambisonics for Beginners (FH St. Polten, 2020): https://audiodesign.fhstp.ac.at/wp-content/uploads/2020/07/AmbisonicsTutorial_Beginners_200701.pdf
