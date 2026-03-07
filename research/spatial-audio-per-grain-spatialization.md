---
title: "Spatial Audio Techniques for Per-Grain Spatialization in Granular Synthesis"
created: 2026-02-08
domain: spatial-audio
type: research
keywords:
  - per-grain-spatialization
  - granular-synthesis
  - ambisonics
  - binaural
  - spatial-audio
---
# Spatial Audio Techniques for Per-Grain Spatialization in Granular Synthesis

**Researched:** 2026-02-07
**Domain:** Spatial audio DSP, per-grain spatialization, VBAP, ambisonics, binaural/HRTF, distance modeling
**Target:** JUCE C++ audio plugin development
**Confidence:** HIGH (mathematical foundations, open-source implementations) / MEDIUM (CPU benchmarks, scaling estimates)

---

## Table of Contents

1. [VBAP (Vector Base Amplitude Panning)](#1-vbap-vector-base-amplitude-panning)
2. [Ambisonics Encoding Per Grain](#2-ambisonics-encoding-per-grain)
3. [Binaural/HRTF Per Grain](#3-binauralhrtf-per-grain)
4. [Distance Attenuation Models](#4-distance-attenuation-models)
5. [Grain Trajectory/Movement](#5-grain-trajectorymovement)
6. [Open-Source Spatial Audio Libraries](#6-open-source-spatial-audio-libraries)
7. [Performance Considerations](#7-performance-considerations)

---

## 1. VBAP (Vector Base Amplitude Panning)

**Reference:** Pulkki, V. (1997). "Virtual Sound Source Positioning Using Vector Base Amplitude Panning." *JAES*, 45(6), 456-466.

### 1.1 Core Algorithm: 2D Case

In 2D VBAP, loudspeakers are arranged on a circle. The virtual source direction is expressed as a linear combination of the two nearest loudspeaker direction vectors.

**Direction vectors** (unit-length, pointing toward each loudspeaker):

```
l_i = [cos(theta_i), sin(theta_i)]^T    for each loudspeaker i
```

**Source direction vector:**

```
p = [cos(theta_s), sin(theta_s)]^T      where theta_s = source azimuth
```

**The fundamental VBAP equation (2D):**

```
p^T = g * L_12

where:
  g   = [g_1, g_2]                       (gain vector for the two active speakers)
  L_12 = [l_1, l_2]^T                    (2x2 matrix of speaker direction vectors as rows)
```

**Solving for gains via matrix inverse:**

```
g = p^T * L_12^(-1)
```

For a 2x2 matrix, the inverse is computed analytically:

```
L_12 = | cos(theta_1)  sin(theta_1) |
       | cos(theta_2)  sin(theta_2) |

L_12^(-1) = (1/det) * |  sin(theta_2)  -sin(theta_1) |
                       | -cos(theta_2)   cos(theta_1) |

where det = cos(theta_1)*sin(theta_2) - sin(theta_1)*cos(theta_2)
          = sin(theta_2 - theta_1)
```

**Normalization (energy-preserving):**

```
g_normalized_i = g_i / sqrt(g_1^2 + g_2^2)
```

This ensures constant power: `sum(g_i^2) = 1`.

### 1.2 Core Algorithm: 3D Case

In 3D, loudspeakers occupy positions on a sphere. The convex hull of the speaker positions forms a triangulation. Each triangle defines a "speaker triplet."

**3D direction vectors:**

```
l_i = [cos(theta_i)*cos(phi_i), sin(theta_i)*cos(phi_i), sin(phi_i)]^T

where theta_i = azimuth, phi_i = elevation
```

**Source vector:**

```
p = [cos(theta_s)*cos(phi_s), sin(theta_s)*cos(phi_s), sin(phi_s)]^T
```

**3D VBAP equation:**

```
p^T = g * L_123

where:
  g     = [g_1, g_2, g_3]               (gain vector for three speakers)
  L_123 = [l_1, l_2, l_3]^T             (3x3 matrix, speaker vectors as rows)
```

**Solving:**

```
g = p^T * L_123^(-1)
```

The 3x3 inverse is precomputed for each speaker triplet using the standard cofactor/adjugate method. This precomputation means real-time panning requires only a matrix-vector multiply (9 multiplies + 6 adds per triplet).

### 1.3 Speaker Triplet Selection

**Algorithm:**

1. Compute the convex hull of all loudspeaker positions on the unit sphere (Delaunay triangulation on the sphere).
2. Each face of the convex hull defines a valid speaker triplet.
3. Optionally filter out triplets with excessive angular aperture (e.g., >120 degrees between any two speakers in the triplet).
4. Precompute and store `L_123^(-1)` for every valid triplet.

**Active triplet selection at runtime:**

For a given source direction `p`, test all triplets: compute `g = p^T * L_123^(-1)`. The correct triplet is the one where ALL three gains are non-negative (`g_1 >= 0, g_2 >= 0, g_3 >= 0`). Exactly one triplet will satisfy this condition for any valid source direction.

**Optimization:** Build a lookup table (gain table) indexed by quantized azimuth/elevation. The SAF framework uses resolution-based indexing:

```
index = ((azimuth_deg + 180) / az_res) * num_el_steps + (elevation_deg + 90) / el_res
```

### 1.4 C++ Implementation for Per-Grain VBAP

```cpp
struct VBAPConfig {
    int numSpeakers;
    int numTriplets;
    // Each triplet: 3 speaker indices
    std::vector<std::array<int, 3>> triplets;
    // Precomputed inverse matrices (3x3 per triplet, stored row-major)
    std::vector<std::array<float, 9>> invMatrices;
};

// Precompute once during setup
void precomputeVBAP(VBAPConfig& config,
                    const std::vector<float>& azimuths,   // speaker azimuths in radians
                    const std::vector<float>& elevations)  // speaker elevations in radians
{
    // 1. Build unit direction vectors for all speakers
    // 2. Compute convex hull -> triangulation
    // 3. For each triplet, build L_123 and compute L_123^(-1)
    // 4. Store in config.invMatrices

    for (int t = 0; t < config.numTriplets; ++t)
    {
        auto [i, j, k] = config.triplets[t];
        // Build 3x3 matrix L
        float L[9] = {
            std::cos(azimuths[i]) * std::cos(elevations[i]),
            std::sin(azimuths[i]) * std::cos(elevations[i]),
            std::sin(elevations[i]),
            std::cos(azimuths[j]) * std::cos(elevations[j]),
            std::sin(azimuths[j]) * std::cos(elevations[j]),
            std::sin(elevations[j]),
            std::cos(azimuths[k]) * std::cos(elevations[k]),
            std::sin(azimuths[k]) * std::cos(elevations[k]),
            std::sin(elevations[k])
        };
        // Compute inverse via cofactors (standard 3x3 inverse)
        float det = L[0]*(L[4]*L[8]-L[5]*L[7])
                  - L[1]*(L[3]*L[8]-L[5]*L[6])
                  + L[2]*(L[3]*L[7]-L[4]*L[6]);
        float invDet = 1.0f / det;

        config.invMatrices[t] = {
            (L[4]*L[8]-L[5]*L[7]) * invDet,
            (L[2]*L[7]-L[1]*L[8]) * invDet,
            (L[1]*L[5]-L[2]*L[4]) * invDet,
            (L[5]*L[6]-L[3]*L[8]) * invDet,
            (L[0]*L[8]-L[2]*L[6]) * invDet,
            (L[2]*L[3]-L[0]*L[5]) * invDet,
            (L[3]*L[7]-L[4]*L[6]) * invDet,
            (L[1]*L[6]-L[0]*L[7]) * invDet,
            (L[0]*L[4]-L[1]*L[3]) * invDet
        };
    }
}

// Called per grain, per audio block
void computeVBAPGains(const VBAPConfig& config,
                      float azimuth, float elevation,
                      std::vector<float>& gains)  // output: one gain per speaker
{
    float px = std::cos(azimuth) * std::cos(elevation);
    float py = std::sin(azimuth) * std::cos(elevation);
    float pz = std::sin(elevation);

    gains.assign(config.numSpeakers, 0.0f);

    for (int t = 0; t < config.numTriplets; ++t)
    {
        const auto& inv = config.invMatrices[t];
        float g1 = px * inv[0] + py * inv[1] + pz * inv[2];
        float g2 = px * inv[3] + py * inv[4] + pz * inv[5];
        float g3 = px * inv[6] + py * inv[7] + pz * inv[8];

        if (g1 >= -0.001f && g2 >= -0.001f && g3 >= -0.001f)
        {
            // Found the active triplet -- normalize
            g1 = std::max(0.0f, g1);
            g2 = std::max(0.0f, g2);
            g3 = std::max(0.0f, g3);
            float norm = std::sqrt(g1*g1 + g2*g2 + g3*g3);
            if (norm > 0.0f)
            {
                auto [i, j, k] = config.triplets[t];
                gains[i] = g1 / norm;
                gains[j] = g2 / norm;
                gains[k] = g3 / norm;
            }
            return;
        }
    }
}
```

**Cost per grain:** ~9 multiplies + 6 adds per triplet tested. Average case tests ~3-5 triplets before finding the active one. With a gain table lookup: O(1).

**Sources:**
- [Aalto University VBAP Library](http://research.spa.aalto.fi/projects/vbap-lib/vbap.html)
- [polarch/Vector-Base-Amplitude-Panning (GitHub)](https://github.com/polarch/Vector-Base-Amplitude-Panning)
- [SAF VBAP Module](https://leomccormack.github.io/Spatial_Audio_Framework/group___v_b_a_p.html)
- [CCRMA Stanford - Amplitude Panning](https://ccrma.stanford.edu/workshops/gaffta2010/spatialsound/topics/amplitude_panning/)
- [pierreguillot/vbap (GitHub)](https://github.com/pierreguillot/vbap)

---

## 2. Ambisonics Encoding Per Grain

### 2.1 Fundamentals

Ambisonics represents a 3D sound field using spherical harmonic basis functions. To "encode" a mono grain at position (azimuth `a`, elevation `e`) into ambisonics means multiplying the grain signal by the spherical harmonic coefficients for that direction.

**Channel count:** `N = (order + 1)^2`
- 1st order: 4 channels (W, Y, Z, X)
- 2nd order: 9 channels
- 3rd order: 16 channels

**Standard format (AmbiX):**
- Channel ordering: ACN (Ambisonic Channel Numbering): `n = l*(l+1) + m`
- Normalization: SN3D (Schmidt semi-normalization)

### 2.2 Encoding Equations: All Orders Through 3rd (ACN/SN3D)

Given a mono grain signal `S` at azimuth `a` (0=front, pi/2=left) and elevation `e` (0=horizon, pi/2=zenith):

**Order 0 (1 channel):**

```
ACN 0 (W):  S * 1
```

**Order 1 (3 channels, cumulative 4):**

```
ACN 1 (Y):  S * sin(a) * cos(e)
ACN 2 (Z):  S * sin(e)
ACN 3 (X):  S * cos(a) * cos(e)
```

**Order 2 (5 channels, cumulative 9):**

```
ACN 4 (V):  S * sqrt(3)/2 * sin(2a) * cos^2(e)
ACN 5 (T):  S * sqrt(3)/2 * sin(a) * sin(2e)
ACN 6 (R):  S * (1/2) * (3*sin^2(e) - 1)
ACN 7 (S):  S * sqrt(3)/2 * cos(a) * sin(2e)
ACN 8 (U):  S * sqrt(3)/2 * cos(2a) * cos^2(e)
```

**Order 3 (7 channels, cumulative 16):**

```
ACN 9  (Q):  S * sqrt(5/8) * sin(3a) * cos^3(e)
ACN 10 (O):  S * sqrt(15)/2 * sin(2a) * cos^2(e) * sin(e)
ACN 11 (M):  S * sqrt(3/8) * sin(a) * cos(e) * (5*sin^2(e) - 1)
ACN 12 (K):  S * (1/2) * sin(e) * (5*sin^2(e) - 3)
ACN 13 (L):  S * sqrt(3/8) * cos(a) * cos(e) * (5*sin^2(e) - 1)
ACN 14 (N):  S * sqrt(15)/2 * cos(2a) * cos^2(e) * sin(e)
ACN 15 (P):  S * sqrt(5/8) * cos(3a) * cos^3(e)
```

### 2.3 Spherical Harmonic Basis Functions (General Form)

The real-valued spherical harmonic of degree `l` and order `m` with SN3D normalization:

```
Y_l^m(a, e) = N_l^|m| * P_l^|m|(sin(e)) * { sin(|m|*a)  if m < 0
                                             { 1           if m = 0
                                             { cos(m*a)    if m > 0

where:
  N_l^|m| = sqrt( (2 - delta_m0) * (l - |m|)! / (l + |m|)! )   (SN3D normalization)
  P_l^|m| = associated Legendre polynomial
  delta_m0 = Kronecker delta (1 if m=0, else 0)
```

**Conversion between normalizations:**

```
N3D_to_SN3D: multiply by 1/sqrt(2*l + 1)
  Order 0: factor = 1
  Order 1: factor = 1/sqrt(3) = 0.5774
  Order 2: factor = 1/sqrt(5) = 0.4472
  Order 3: factor = 1/sqrt(7) = 0.3780
```

### 2.4 C++ Implementation: Per-Grain Ambisonics Encoder

```cpp
// Precompute trig values once per grain position update
struct AmbiEncodeCoeffs {
    float cosE, sinE, cosA, sinA;
    float cos2A, sin2A, cos3A, sin3A;
    float cosE2, sinE2, cosE3;

    void compute(float azimuth, float elevation) {
        cosE = std::cos(elevation);  sinE = std::sin(elevation);
        cosA = std::cos(azimuth);    sinA = std::sin(azimuth);
        cos2A = std::cos(2.0f * azimuth);
        sin2A = std::sin(2.0f * azimuth);
        cos3A = std::cos(3.0f * azimuth);
        sin3A = std::sin(3.0f * azimuth);
        cosE2 = cosE * cosE;
        sinE2 = sinE * sinE;
        cosE3 = cosE2 * cosE;
    }
};

// Constants (precomputed at compile time)
static constexpr float kSqrt3_2  = 0.86602540378f;   // sqrt(3)/2
static constexpr float kSqrt15_2 = 1.93649167310f;   // sqrt(15)/2
static constexpr float kSqrt5_8  = 0.79056941504f;   // sqrt(5/8)
static constexpr float kSqrt3_8  = 0.61237243570f;   // sqrt(3/8)

// Encode one sample into HOA3 (16 channels)
// coeffs must be precomputed for the grain's current position
inline void encodeGrainSample(float sample,
                              const AmbiEncodeCoeffs& c,
                              float* __restrict ambiOut)  // 16-element array, ACCUMULATE
{
    // Order 0
    ambiOut[0]  += sample;

    // Order 1
    ambiOut[1]  += sample * c.sinA * c.cosE;
    ambiOut[2]  += sample * c.sinE;
    ambiOut[3]  += sample * c.cosA * c.cosE;

    // Order 2
    ambiOut[4]  += sample * kSqrt3_2 * c.sin2A * c.cosE2;
    ambiOut[5]  += sample * kSqrt3_2 * c.sinA * c.sinE * c.cosE;
    ambiOut[6]  += sample * 0.5f * (3.0f * c.sinE2 - 1.0f);
    ambiOut[7]  += sample * kSqrt3_2 * c.cosA * c.sinE * c.cosE;
    ambiOut[8]  += sample * kSqrt3_2 * c.cos2A * c.cosE2;

    // Order 3
    ambiOut[9]  += sample * kSqrt5_8 * c.sin3A * c.cosE3;
    ambiOut[10] += sample * kSqrt15_2 * c.sin2A * c.cosE2 * c.sinE;
    ambiOut[11] += sample * kSqrt3_8 * c.sinA * c.cosE * (5.0f * c.sinE2 - 1.0f);
    ambiOut[12] += sample * 0.5f * c.sinE * (5.0f * c.sinE2 - 3.0f);
    ambiOut[13] += sample * kSqrt3_8 * c.cosA * c.cosE * (5.0f * c.sinE2 - 1.0f);
    ambiOut[14] += sample * kSqrt15_2 * c.cos2A * c.cosE2 * c.sinE;
    ambiOut[15] += sample * kSqrt5_8 * c.cos3A * c.cosE3;
}
```

### 2.5 Optimization: Precompute Coefficients Per Grain

Since grain positions change slowly relative to sample rate (position updates per grain, not per sample), precompute the full coefficient array once per grain onset or per grain position update:

```cpp
struct GrainSpatialState {
    std::array<float, 16> ambiCoeffs;  // spherical harmonic weights
    float distanceGain;                 // distance attenuation

    void updatePosition(float azimuth, float elevation, float distance) {
        AmbiEncodeCoeffs c;
        c.compute(azimuth, elevation);

        ambiCoeffs[0]  = 1.0f;
        ambiCoeffs[1]  = c.sinA * c.cosE;
        ambiCoeffs[2]  = c.sinE;
        ambiCoeffs[3]  = c.cosA * c.cosE;
        ambiCoeffs[4]  = kSqrt3_2 * c.sin2A * c.cosE2;
        // ... (remaining channels as above)
        ambiCoeffs[15] = kSqrt5_8 * c.cos3A * c.cosE3;

        distanceGain = computeDistanceGain(distance);
    }
};

// In processBlock -- per sample, per grain:
// Just multiply sample by precomputed coefficients (16 multiplies + 16 adds)
for (int ch = 0; ch < 16; ++ch)
    ambiBuffer[ch][sampleIdx] += grainSample * grain.spatial.ambiCoeffs[ch]
                                             * grain.spatial.distanceGain;
```

**Cost per grain per sample (HOA3):** 16 multiplies + 16 additions = 32 FLOPs.
**Cost for 64 grains per sample (HOA3):** 64 * 32 = 2048 FLOPs/sample.
At 48kHz: ~98 MFLOPS. This is trivial for modern CPUs (single-core GFLOPS > 50).

### 2.6 Why Ambisonics Is Ideal for Per-Grain Spatialization

1. **Additive by nature:** All grains encode into the same ambisonics bus by simple addition. No per-grain decoder needed.
2. **Format-agnostic:** Decode to any speaker layout or binaural at the output stage, not per grain.
3. **Rotation-friendly:** Rotate the entire sound field with a single matrix multiply on the ambisonics bus.
4. **Scalable resolution:** Use 1st order for cheap real-time, 3rd order for quality, same encoding code.

**Sources:**
- [Blue Ripple Sound - B-Format Technical Notes](https://www.blueripplesound.com/notes/bformat)
- [Angelo Farina - ACN-N3D Formulas](https://www.angelofarina.it/Aurora/HOA_ACN_N3D_formulas.htm)
- [Angelo Farina - Explicit AmbiX Formulas](https://www.angelofarina.it/Aurora/HOA_explicit_formulas.htm)
- [Wikipedia - Ambisonic Data Exchange Formats](https://en.wikipedia.org/wiki/Ambisonic_data_exchange_formats)
- [RingBuffer - Understanding Ambisonics](https://ringbuffer.org/spatial_audio/ambisonics/understanding-ambisonics/)
- [polarch/Higher-Order-Ambisonics (GitHub)](https://github.com/polarch/Higher-Order-Ambisonics)

---

## 3. Binaural/HRTF Per Grain

### 3.1 The Problem

Applying a unique HRTF per grain means convolving each grain's mono signal with a pair of head-related impulse responses (left ear, right ear) corresponding to the grain's spatial position. A typical HRIR is 128-512 samples long (2.7-10.7ms at 48kHz).

For 64 concurrent grains, that is 128 separate convolutions (64 left + 64 right) per audio block.

### 3.2 Direct Convolution Cost Analysis

**HRIR length:** 128 samples (typical minimum for reasonable quality)
**Audio block size:** 256 samples
**Per grain, per ear:** 128 * 256 = 32,768 multiply-accumulate operations
**Per grain (stereo):** 65,536 MACs
**64 grains:** 4,194,304 MACs per block
**At 48kHz, 256 sample blocks:** 187.5 blocks/second
**Total:** ~786 MFLOPS

This is within reach of a single CPU core but leaves little headroom. FFT-based convolution is strongly preferred.

### 3.3 FFT-Based (Overlap-Save) Convolution

**Overlap-Save method for HRTF:**

1. Maintain a circular input buffer of length `K` per grain (where `K >= B + N - 1`, `B` = block size, `N` = HRIR length).
2. Each block: append `B` new samples, compute FFT of the full `K`-sample window.
3. Multiply in frequency domain with pre-FFT'd HRIR.
4. Inverse FFT, discard first `N-1` samples (aliased region), output remaining `B` samples.

**Optimal FFT size:** Next power of 2 >= `B + N - 1`. For B=256, N=128: K=512 (already power of 2).

```cpp
struct GrainHRTFState {
    // FFT-domain HRIR (precomputed for current direction)
    std::array<std::complex<float>, FFT_SIZE/2 + 1> hrirFreqLeft;
    std::array<std::complex<float>, FFT_SIZE/2 + 1> hrirFreqRight;

    // Input history buffer (overlap region)
    std::array<float, HRIR_LENGTH - 1> inputHistory;

    void loadHRIR(float azimuth, float elevation, const HRTFDatabase& db) {
        // 1. Find nearest HRIR pair (or interpolate 3 nearest via barycentric)
        // 2. FFT the HRIR pair
        // 3. Store in hrirFreqLeft, hrirFreqRight
    }
};

// Per-grain convolution in processBlock
void convolveGrain(const float* grainSamples, int blockSize,
                   const GrainHRTFState& hrtf,
                   float* leftOut, float* rightOut)
{
    // 1. Prepend inputHistory to grainSamples -> inputBuffer[K]
    // 2. FFT(inputBuffer) -> inputFreq
    // 3. leftFreq = inputFreq * hrtf.hrirFreqLeft  (complex multiply)
    // 4. rightFreq = inputFreq * hrtf.hrirFreqRight
    // 5. IFFT(leftFreq) -> leftTime, take last blockSize samples
    // 6. IFFT(rightFreq) -> rightTime, take last blockSize samples
    // 7. Update inputHistory with last (N-1) samples of grainSamples
}
```

**FFT convolution cost per grain:**
- 2 forward FFTs (input, done once) + 2 complex multiplies + 2 inverse FFTs
- FFT of size 512: ~512 * log2(512) * 5 = ~23,040 FLOPs
- Total per grain: ~1 FFT + 2 complex multiplies + 2 IFFTs = ~80,000 FLOPs
- 64 grains: ~5.1 MFLOPS per block

**Optimization: shared input FFT.** If multiple grains share the same input (unlikely in granular synthesis since each grain reads different buffer positions), the input FFT could be shared. In practice, each grain has a unique signal, so each needs its own FFT.

### 3.4 Partitioned Convolution for Longer HRIRs

For HRIRs longer than the audio block size, use non-uniformly partitioned convolution:

```
Partition 0: [  0..255  ] -> FFT size 512, processed immediately (zero latency)
Partition 1: [ 256..767 ] -> FFT size 1024, processed every 2 blocks
Partition 2: [768..2815 ] -> FFT size 4096, processed every 8 blocks
```

**Reference implementation:** [HiFi-LoFi/FFTConvolver (GitHub)](https://github.com/HiFi-LoFi/FFTConvolver) provides:
- `FFTConvolver`: uniform partitioned convolution
- `TwoStageFFTConvolver`: non-uniform (head + tail) partitioning for zero-latency operation
- Pure C++, self-contained FFT, optional SSE via `FFTCONVOLVER_USE_SSE`

### 3.5 HRTF Interpolation

Real-time position changes require interpolating between measured HRTF directions. Three approaches:

**Nearest-neighbor:** Snap to closest measured direction. Causes audible jumps during movement.

**Bilinear interpolation:** Interpolate between 4 nearest HRIRs. Smooth but can cause comb filtering.

**Barycentric interpolation (recommended):** Find the 3 nearest HRIRs forming a triangle containing the source direction. Interpolate using barycentric weights. This is what the 3DTI Toolkit uses.

```cpp
// Barycentric HRIR interpolation
void interpolateHRIR(float azimuth, float elevation,
                     const HRTFDatabase& db,
                     float* hrirLeft, float* hrirRight, int hrirLen)
{
    // Find 3 nearest measurement directions forming triangle around (az, el)
    auto [idx0, idx1, idx2, w0, w1, w2] = db.findTriangle(azimuth, elevation);

    for (int i = 0; i < hrirLen; ++i) {
        hrirLeft[i]  = w0 * db.left[idx0][i]  + w1 * db.left[idx1][i]
                     + w2 * db.left[idx2][i];
        hrirRight[i] = w0 * db.right[idx0][i] + w1 * db.right[idx1][i]
                     + w2 * db.right[idx2][i];
    }
}
```

### 3.6 HRTF Dataset Sources

| Dataset | Subjects | Directions | Sample Rate | HRIR Length | Format | License |
|---------|----------|------------|-------------|-------------|--------|---------|
| **MIT KEMAR** | 1 (dummy head) | 710 | 44.1 kHz | 512 samples | WAV, SOFA | Public domain |
| **CIPIC** | 43 humans + KEMAR | 1250 per subject | 44.1 kHz | 200 samples | MAT, SOFA | Academic |
| **LISTEN (IRCAM)** | 51 humans | 187 per subject | 44.1 kHz | 512 samples | WAV, SOFA | Research |
| **SADIE II** | 20 humans + KU100 | 8802 per subject | 96 kHz | 256-512 samples | SOFA | CC BY 4.0 |
| **ARI (Austrian Academy)** | 200+ humans | 1550 per subject | 48 kHz | 256 samples | SOFA | Open |

**SOFA format** (AES69-2022): The standard interchange format. HDF5-based. C++ readers:
- `libmysofa` (C, MIT license) - lightweight SOFA reader
- SAF includes SOFA reader (`saf_sofa_reader` optional module, GPLv2)
- 3DTI Toolkit includes SOFA reader

### 3.7 Practical Strategy: Ambisonics-to-Binaural Hybrid

**The per-grain HRTF approach is expensive.** A more practical approach:

1. Encode each grain into ambisonics (cheap: 16 multiplies per sample per grain for HOA3).
2. Sum all grains into a single ambisonics bus (just addition).
3. Decode the ambisonics bus to binaural ONCE using a single HRTF convolution per ambisonics channel.

**Cost comparison for 64 grains at HOA3:**

| Approach | Per-Sample Cost | Notes |
|----------|----------------|-------|
| Per-grain HRTF (direct) | 64 grains * 2 ears * 128 taps = 16,384 MACs | Highest quality, highest cost |
| Per-grain HRTF (FFT) | ~5.1 MFLOPS/block (amortized) | Needs 128 FFT instances |
| **Ambi encode + bus decode** | 64 * 32 + 16 * 2 * 128 = **6,144 MACs** | 3x cheaper than direct HRTF |

The ambisonics-to-binaural hybrid is roughly **2.5x cheaper** and only requires 32 HRTF convolutions (16 channels * 2 ears) regardless of grain count.

**Sources:**
- [SOFA Convention](https://www.sofaconventions.org/mediawiki/index.php/SOFA_(Spatially_Oriented_Format_for_Acoustics))
- [SOFA Files Database](https://www.sofaconventions.org/mediawiki/index.php/Files)
- [3DTI Toolkit (GitHub)](https://github.com/3DTune-In/3dti_AudioToolkit)
- [Frank Wefers - Partitioned Convolution Algorithms (PDF)](https://publications.rwth-aachen.de/record/466561/files/466561.pdf)
- [HiFi-LoFi/FFTConvolver (GitHub)](https://github.com/HiFi-LoFi/FFTConvolver)
- [WolfSound - Fast Convolution](https://thewolfsound.com/fast-convolution-fft-based-overlap-add-overlap-save-partitioned/)
- [Superpowered - 3D Audio HRTF Processing](https://superpowered.com/3d-spatialized-audio-hrtf-processing)
- [SADIE II Database](https://www.york.ac.uk/sadie-project/database_old.html)
- [Partitioned Convolution (GitHub)](https://github.com/michaelkrzyzaniak/Partitioned-Convolution)

---

## 4. Distance Attenuation Models

### 4.1 Inverse Distance Law (Amplitude)

Sound pressure decreases linearly with distance from a point source:

```
gain(d) = d_ref / d     (for d >= d_ref)

where:
  d     = distance from source to listener
  d_ref = reference distance (typically 1.0 meter)
```

In decibels: -6 dB per doubling of distance.

**Clamped version (avoid division by zero and infinite gain):**

```cpp
float inverseDistanceGain(float distance, float refDist = 1.0f,
                          float minDist = 0.1f, float maxDist = 100.0f)
{
    float d = std::clamp(distance, minDist, maxDist);
    return refDist / d;
}
```

### 4.2 Inverse Distance Squared (Energy/Power)

For energy-based calculations (more physically accurate for spherical spreading):

```
gain(d) = (d_ref / d)^2

or equivalently in amplitude: gain(d) = d_ref / d  (already amplitude-domain)
```

The distinction matters for summing: amplitude gains sum directly, power gains require sqrt after summing squares.

### 4.3 Rolloff Factor (Adjustable Curve)

Games and VR engines commonly use a rolloff exponent for artistic control:

```
gain(d) = (d_ref / d)^rolloff

where rolloff typically ranges from 0.5 (gentle) to 2.0 (aggressive)
```

```cpp
float adjustableDistanceGain(float distance, float refDist,
                             float rolloff, float minDist, float maxDist)
{
    float d = std::clamp(distance, minDist, maxDist);
    return std::pow(refDist / d, rolloff);
}
```

### 4.4 Atmospheric Absorption (Frequency-Dependent)

Air absorbs high frequencies more than low frequencies. The effect is modeled as a low-pass filter whose cutoff decreases with distance.

**Physical model:**

```
Total attenuation (dB) = L_geometric + alpha * d

where:
  L_geometric = 20*log10(d_ref/d)       (inverse distance in dB)
  alpha = absorption coefficient (dB/m)  (frequency-dependent)
```

**Absorption coefficients (typical, 20C, 50% humidity):**

| Frequency | alpha (dB/m) |
|-----------|-------------|
| 125 Hz | 0.0003 |
| 250 Hz | 0.001 |
| 500 Hz | 0.002 |
| 1 kHz | 0.004 |
| 2 kHz | 0.010 |
| 4 kHz | 0.028 |
| 8 kHz | 0.090 |
| 16 kHz | 0.290 |

**Simple implementation: one-pole low-pass filter whose cutoff tracks distance.**

A one-pole LPF is a reasonable real-time approximation. The cutoff frequency decreases as distance increases:

```cpp
class DistanceFilter {
    float y1 = 0.0f;  // previous output (state)

    // Map distance to cutoff frequency
    float distanceToCutoff(float distance) const {
        // At 1m: 20kHz (transparent). At 100m: ~1kHz. At 1000m: ~200Hz.
        // Approximation: cutoff = 3.0 / (alpha * distance) mapped to frequency
        // Simpler artistic model:
        float maxCutoff = 20000.0f;
        float minCutoff = 200.0f;
        float t = std::clamp((distance - 1.0f) / 99.0f, 0.0f, 1.0f);
        // Logarithmic mapping
        return maxCutoff * std::pow(minCutoff / maxCutoff, t);
    }

public:
    float process(float sample, float distance, float sampleRate) {
        float cutoff = distanceToCutoff(distance);
        float omega = 2.0f * juce::MathConstants<float>::pi * cutoff / sampleRate;
        float alpha = omega / (omega + 1.0f);  // one-pole coefficient
        y1 = alpha * sample + (1.0f - alpha) * y1;
        return y1;
    }
};
```

**More accurate: biquad shelving filter** with frequency-dependent rolloff, updated when grain distance changes:

```cpp
// Update shelving filter based on distance
void updateDistanceFilter(juce::dsp::IIR::Coefficients<float>& coeffs,
                          float distance, float sampleRate)
{
    // Attenuation of high frequencies in dB (approximation)
    float hfAttenuation = std::min(distance * 0.1f, 24.0f);  // up to -24dB at HF

    // Use a high-shelf filter at 2kHz with negative gain
    float shelfFreq = 2000.0f;
    float shelfGainDB = -hfAttenuation;
    *coeffs = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, shelfFreq, 0.707f, juce::Decibels::decibelsToGain(shelfGainDB));
}
```

### 4.5 Proximity Effect (Near-Field Bass Boost)

When a source is very close (< 1 meter), low frequencies are exaggerated due to the near-field wavefront curvature. This is modeled as a low-frequency shelving boost:

```
Bass boost (dB) = max(0, G_max * (1 - d/d_threshold))

where:
  G_max = maximum boost in dB (typically 6-12 dB)
  d_threshold = distance below which proximity effect kicks in (typically 0.5-1.0m)
```

```cpp
class ProximityEffect {
    juce::dsp::IIR::Filter<float> shelfFilter;

public:
    void updateForDistance(float distance, float sampleRate) {
        float threshold = 1.0f;  // meters
        if (distance < threshold) {
            float t = 1.0f - distance / threshold;  // 0..1 as distance -> 0
            float boostDB = t * 10.0f;  // up to +10dB at distance=0
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, 200.0f, 0.707f,
                juce::Decibels::decibelsToGain(boostDB));
            *shelfFilter.coefficients = *coeffs;
        } else {
            // Flat response (no boost)
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, 200.0f, 0.707f, 1.0f);
            *shelfFilter.coefficients = *coeffs;
        }
    }
};
```

### 4.6 Complete Per-Grain Distance Model

```cpp
struct GrainDistanceModel {
    float amplitudeGain;     // inverse distance
    float lpfCoefficient;    // atmospheric absorption (one-pole)
    float bassBoostDB;       // proximity effect

    void update(float distance) {
        // Inverse distance gain (clamped)
        float d = std::max(distance, 0.1f);
        amplitudeGain = std::min(1.0f / d, 10.0f);  // cap at +20dB

        // Atmospheric absorption: one-pole cutoff
        float cutoff = 20000.0f * std::exp(-0.03f * d);
        cutoff = std::max(cutoff, 200.0f);
        // Convert to coefficient: alpha = 2*pi*fc / (2*pi*fc + sr)
        // (computed in process with actual sample rate)

        // Proximity effect
        if (d < 1.0f)
            bassBoostDB = (1.0f - d) * 10.0f;
        else
            bassBoostDB = 0.0f;
    }
};
```

**Sources:**
- [Approximating Atmospheric Absorption With a Simple Filter](https://computingandrecording.wordpress.com/2017/07/05/approximating-atmospheric-absorption-with-a-simple-filter/)
- [Engineering Toolbox - Outdoor Sound Propagation](https://www.engineeringtoolbox.com/outdoor-propagation-sound-d_64.html)
- [Audiokinetic - Distance Modeling](https://www.audiokinetic.com/en/blog/a-wwise-approach-to-spatial-audio-part-1/)
- [Unreal Engine - Sound Attenuation](https://docs.unrealengine.com/4.27/en-US/WorkingWithAudio/DistanceModelAttenuation)

---

## 5. Grain Trajectory/Movement

### 5.1 Static Position Assignment

Simplest approach: assign each grain a fixed position at onset. The position does not change during the grain's lifetime.

```cpp
struct GrainPosition {
    float azimuth;    // radians, 0=front
    float elevation;  // radians, 0=horizon
    float distance;   // meters
};

// Assign position at grain onset
GrainPosition assignPosition(float centerAz, float centerEl, float spread) {
    // Random offset within spherical cap
    float azOffset = (rng.nextFloat() - 0.5f) * 2.0f * spread;
    float elOffset = (rng.nextFloat() - 0.5f) * 2.0f * spread * 0.5f;
    return { centerAz + azOffset, centerEl + elOffset, 1.0f };
}
```

### 5.2 Linear Trajectory

Grain moves from start position to end position over its lifetime:

```cpp
struct LinearTrajectory {
    float startAz, startEl, startDist;
    float endAz, endEl, endDist;

    GrainPosition positionAt(float t) const {  // t in [0, 1]
        return {
            startAz   + t * (endAz   - startAz),
            startEl   + t * (endEl   - startEl),
            startDist + t * (endDist - startDist)
        };
    }
};
```

### 5.3 Orbital/Circular Trajectory

Grain orbits around a center point. Good for creating spinning/swirling textures.

```cpp
struct OrbitalTrajectory {
    float centerAz, centerEl;
    float radius;        // angular radius in radians
    float angularSpeed;  // radians per second
    float startPhase;    // random initial angle

    GrainPosition positionAt(float grainAge, float dt) const {
        float angle = startPhase + angularSpeed * grainAge;
        return {
            centerAz + radius * std::cos(angle),
            centerEl + radius * std::sin(angle),
            1.0f  // constant distance, or modulate with sin for spiral
        };
    }
};
```

### 5.4 Spiral Trajectory

Combines orbital motion with outward/inward radial drift:

```cpp
struct SpiralTrajectory {
    float centerAz, centerEl;
    float startRadius, endRadius;  // angular radius
    float angularSpeed;            // radians per second
    float startPhase;
    float startDist, endDist;      // radial distance from listener

    GrainPosition positionAt(float t) const {  // t in [0, 1]
        float angle = startPhase + angularSpeed * t;
        float radius = startRadius + t * (endRadius - startRadius);
        float dist = startDist + t * (endDist - startDist);
        return {
            centerAz + radius * std::cos(angle),
            centerEl + radius * std::sin(angle),
            dist
        };
    }
};
```

### 5.5 Random Walk (Brownian Motion)

Each position update adds a small random displacement. Creates organic, drifting spatial textures.

```cpp
struct RandomWalkTrajectory {
    float az, el, dist;
    float stepSize;      // maximum angular step per update
    float distStepSize;  // maximum distance step per update

    void step(juce::Random& rng) {
        az  += (rng.nextFloat() - 0.5f) * 2.0f * stepSize;
        el  += (rng.nextFloat() - 0.5f) * 2.0f * stepSize;
        dist += (rng.nextFloat() - 0.5f) * 2.0f * distStepSize;

        // Clamp
        el = std::clamp(el, -juce::MathConstants<float>::halfPi,
                             juce::MathConstants<float>::halfPi);
        dist = std::max(dist, 0.1f);

        // Wrap azimuth
        if (az > juce::MathConstants<float>::pi)
            az -= juce::MathConstants<float>::twoPi;
        if (az < -juce::MathConstants<float>::pi)
            az += juce::MathConstants<float>::twoPi;
    }

    GrainPosition position() const { return { az, el, dist }; }
};
```

### 5.6 Smoothing Position Updates

Avoid zipper noise when updating grain positions by smoothing the spatial coefficients:

```cpp
struct SmoothedSpatialState {
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> azimuth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> elevation;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> distance;

    void prepare(float sampleRate) {
        // Smooth over ~5ms to avoid spatial artifacts
        float smoothTime = 0.005f;
        azimuth.reset(sampleRate, smoothTime);
        elevation.reset(sampleRate, smoothTime);
        distance.reset(sampleRate, smoothTime);
    }

    void setTarget(float az, float el, float dist) {
        azimuth.setTargetValue(az);
        elevation.setTargetValue(el);
        distance.setTargetValue(dist);
    }

    // Call per-sample to get smoothed position
    GrainPosition getNext() {
        return { azimuth.getNextValue(),
                 elevation.getNextValue(),
                 distance.getNextValue() };
    }
};
```

**Alternative for ambisonics:** Instead of smoothing position and recomputing coefficients per sample, smooth the ambisonics coefficients directly. This is cheaper since it avoids per-sample trig:

```cpp
struct SmoothedAmbiCoeffs {
    std::array<juce::SmoothedValue<float>, 16> coeffs;

    void prepare(float sampleRate) {
        for (auto& c : coeffs)
            c.reset(sampleRate, 0.005f);
    }

    void setTarget(float azimuth, float elevation) {
        // Compute target coefficients
        AmbiEncodeCoeffs ac;
        ac.compute(azimuth, elevation);
        coeffs[0].setTargetValue(1.0f);
        coeffs[1].setTargetValue(ac.sinA * ac.cosE);
        coeffs[2].setTargetValue(ac.sinE);
        coeffs[3].setTargetValue(ac.cosA * ac.cosE);
        // ... remaining channels
    }

    // Per-sample: multiply by next smoothed value
    float getCoeff(int ch) { return coeffs[ch].getNextValue(); }
};
```

### 5.7 IEM GranularEncoder: Reference Implementation

The IEM GranularEncoder plugin (part of the IEM Plugin Suite) implements per-grain ambisonic spatialization with:

- **3D spherical cap distribution:** Grains distributed around a center direction within a controllable "Size" parameter defining angular spread.
- **2D circular distribution:** Grains spread in azimuth only on a fixed elevation plane.
- **Built-in randomization modulators** for grain timing, length, position.
- **Open source** (JUCE-based, GPLv3).

This is the closest existing reference implementation to per-grain spatial granular synthesis.

**Sources:**
- [IEM GranularEncoder Guide](https://plugins.iem.at/docs/granularencoder/)
- [IEM Plugin Suite (GitHub)](https://github.com/tu-studio/IEMPluginSuite)
- [mc.spindrift~ (Michael Norris)](https://www.michaelnorris.info/software/spindrift)

---

## 6. Open-Source Spatial Audio Libraries Compatible with C++/JUCE

### 6.1 Spatial Audio Framework (SAF) / SPARTA

**Repository:** [github.com/leomccormack/Spatial_Audio_Framework](https://github.com/leomccormack/Spatial_Audio_Framework)
**License:** ISC (core), GPLv2 (optional modules with SOFA reader)
**Language:** C/C++

**Modules relevant to per-grain spatialization:**

| Module | Content | Per-Grain Use |
|--------|---------|---------------|
| `saf_vbap` | VBAP gain computation, gain tables, speaker triplet finding | Compute per-grain VBAP gains |
| `saf_hoa` | Ambisonics encoding/decoding up to 10th order | Encode each grain into ambisonics |
| `saf_sh` | Spherical harmonic functions, rotation matrices | Compute encoding coefficients |
| `saf_hrir` | HRTF processing, ITD estimation, interpolation | Binaural decoding |
| `saf_utilities` | FFT wrappers, matrix math, SIMD | Optimized math operations |

**JUCE integration:** SAF is pure C with C++ compatibility. The SPARTA plugins (19 VST/LV2 plugins) are built on JUCE + SAF, proving the integration works. Link SAF as a static library in CMake alongside JUCE.

**Key SAF VBAP functions:**

```c
// Generate 3D VBAP gain table for a grid of directions
void generateVBAPgainTable3D(float* ls_dirs_deg, int numLS, int az_res_deg,
                             int el_res_deg, int omitLargeTriangles,
                             int enableDummies, float spread,
                             float** gtable, int* nTriangles, int* nDirs);

// Compute gains for specific source directions
void vbap3D(float* src_dirs_deg, int numSrcs, int numLS,
            int* triangles, int nTriangles, float* invMtx,
            float spread, float** GainMtx);

// Find speaker triplets via convex hull
void findLsTriplets(float* ls_dirs_deg, int numLS,
                    int** triplets, int* nTriangles);
```

**Assessment: BEST CHOICE for per-grain spatialization.** Comprehensive, proven with JUCE, well-documented, covers VBAP + ambisonics + binaural.

### 6.2 libspatialaudio

**Repository:** [github.com/videolabs/libspatialaudio](https://github.com/videolabs/libspatialaudio)
**License:** LGPLv2.1+ (or commercial)
**Language:** C++

**Features:**
- Unified `Renderer` class for HOA, object-based, speaker, and binaural output
- Ambisonics encoding/decoding up to 3rd order (ACN/SN3D)
- Binauralization with built-in MIT HRTF or custom SOFA files
- Object-based rendering with dynamic 3D positioning

**JUCE integration:** Standard C++ library. Can be linked via CMake. The `Renderer` class takes audio buffers and metadata, returning spatialized output. However, the library is designed for renderer-level (bus-level) processing, not per-grain operation. You would need to use the lower-level ambisonics encoding functions.

**Assessment:** Good for bus-level ambisonics decoding to binaural/speakers. Not ideal for per-grain encoding (its API is higher-level). The LGPL license allows dynamic linking in commercial plugins.

### 6.3 IEM Plugin Suite

**Repository:** [github.com/tu-studio/IEMPluginSuite](https://github.com/tu-studio/IEMPluginSuite)
**License:** GPLv3
**Language:** C++ (JUCE-based)

**Directly relevant:** The **GranularEncoder** plugin implements per-grain ambisonic encoding. This is the most directly applicable reference for a spatial granular synth. The source code shows exactly how to:
- Schedule grains with spatial positions
- Distribute grains within a spherical cap
- Encode each grain into ambisonics
- Sum into a shared ambisonics bus

**Other useful plugins in the suite:**
- `StereoEncoder`: Basic mono/stereo to ambisonics encoder
- `BinauralDecoder`: Fast convolution ambisonics-to-binaural decoder
- `RoomEncoder`: Room simulation with per-source positioning

**Assessment: BEST REFERENCE for per-grain ambisonic granular synthesis.** Direct JUCE implementation. GPLv3 license means you can study but not copy directly into a commercial plugin without also being GPLv3.

### 6.4 3DTI Toolkit (3D Tune-In)

**Repository:** [github.com/3DTune-In/3dti_AudioToolkit](https://github.com/3DTune-In/3dti_AudioToolkit)
**License:** GPLv3
**Language:** C++

**Features:**
- Real-time binaural spatializer with per-source HRTF convolution
- HRIR barycentric interpolation (3 nearest HRIRs)
- Uniformly partitioned overlap-save convolution
- SOFA and custom binary HRTF format support
- Distance attenuation and frequency-dependent air absorption
- Near-field simulation (proximity effect with ILD)
- Acoustic parallax (independent L/R ear HRIR selection)

**JUCE integration:** Has an official JUCE-based VST plugin project (`3dti_AudioToolkit_VST_Plugins`). JUCE is included as a submodule.

**Per-grain applicability:** The toolkit renders each source independently via HRTF convolution. Each grain would be a "source." However, the overhead of creating/destroying sources per grain may need optimization. Better used for a moderate number of concurrent grains (8-16 with per-grain HRTF) or as a binaural decoder at the bus level.

**Note:** The developers recommend transitioning to the newer **BRT (Binaural Rendering Toolbox)** for new projects.

### 6.5 SoundScape Renderer (SSR)

**Repository:** [github.com/SoundScapeRenderer/ssr](https://github.com/SoundScapeRenderer/ssr)
**License:** GPLv3
**Language:** C++ (uses STL heavily)

**JUCE integration:** A prototype JUCE plugin exists: [JuceSSRPlugin](https://github.com/SoundScapeRenderer/JuceSSRPlugin). However, it is a 32-bit Windows-only prototype with only basic binaural synthesis supported. Not production-ready.

**Algorithms:** Wave Field Synthesis, Higher-Order Ambisonics, binaural rendering. The SSR is designed as a standalone renderer, not as an embeddable per-grain library.

**Assessment: Not recommended for per-grain use.** Architecture is designed for scene-level rendering. JUCE integration is a limited prototype.

### 6.6 Comparison Matrix

| Library | License | VBAP | Ambisonics | Binaural/HRTF | SOFA | JUCE Proven | Per-Grain Suitable |
|---------|---------|------|-----------|--------------|------|------------|-------------------|
| **SAF/SPARTA** | ISC/GPLv2 | Yes (full) | Up to 10th | Yes | Optional | Yes (19 plugins) | **Yes** |
| **libspatialaudio** | LGPLv2.1 | No | Up to 3rd | Yes (MIT HRTF) | Yes | No (but easy) | Partial |
| **IEM Suite** | GPLv3 | No | Up to 7th | Yes | No | Yes | **Yes (GranularEncoder)** |
| **3DTI Toolkit** | GPLv3 | No | Via binaural | Yes (per-source) | Yes | Yes (VST plugin) | Moderate |
| **SSR** | GPLv3 | No | Yes | Yes | No | Prototype only | No |

**Recommendation:** Use **SAF** for low-level spatial DSP functions (VBAP gains, SH encoding, HRTF convolution). Study the **IEM GranularEncoder** source for architectural patterns. If binaural-only output is acceptable, the **3DTI Toolkit** provides the most complete per-source binaural pipeline.

**Sources:**
- [SPARTA Site](https://leomccormack.github.io/sparta-site/)
- [SAF GitHub](https://github.com/leomccormack/Spatial_Audio_Framework)
- [libspatialaudio GitHub](https://github.com/videolabs/libspatialaudio)
- [IEM Plugin Suite](https://plugins.iem.at/)
- [IEM Plugin Suite GitHub](https://github.com/tu-studio/IEMPluginSuite)
- [3DTI Toolkit GitHub](https://github.com/3DTune-In/3dti_AudioToolkit)
- [3DTI Toolkit Paper (PLOS One)](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0211899)
- [SSR - SoundScape Renderer](http://spatialaudio.net/ssr/)
- [JuceSSRPlugin (GitHub)](https://github.com/SoundScapeRenderer/JuceSSRPlugin)
- [JUCE Forum - Spatial Audio Support](https://forum.juce.com/t/juce-spatial-audio-support/30509)

---

## 7. Performance Considerations

### 7.1 Per-Grain vs. Bus-Level Spatialization

| Approach | Description | Cost Model | Quality |
|----------|------------|------------|---------|
| **Per-grain spatial** | Each grain individually positioned | O(grains * channels) per sample | Highest. Each grain is a distinct spatial point. |
| **Submix grouping** | Group grains by spatial region, spatialize groups | O(groups * channels) per sample | Good. Spatial blur within groups. |
| **Bus-level panning** | All grains summed mono, then spatialized once | O(channels) per sample | Lowest. All grains at same position. |

### 7.2 CPU Cost Breakdown: Ambisonics Encoding (Per Grain)

Assuming HOA3 (16 channels), 64 concurrent grains, 48kHz, 256-sample blocks:

**Per grain, per sample:**
- Ambisonics encoding: 16 multiplies + 16 additions = 32 FLOPs
- Distance attenuation (gain multiply): 1 FLOP
- Atmospheric absorption (one-pole filter): 2 FLOPs
- **Total per grain per sample: ~35 FLOPs**

**Per block (256 samples, 64 grains):**
- 35 * 256 * 64 = 573,440 FLOPs per block
- At 187.5 blocks/second = **107 MFLOPS**

**Modern CPU single-core capacity:** ~50-100 GFLOPS (with SIMD). This is **0.1-0.2%** of a single core. Extremely affordable.

### 7.3 CPU Cost Breakdown: VBAP (Per Grain)

Assuming 7.1.4 (12 speakers), with precomputed gain table:

**Per grain, per sample:**
- Gain table lookup: ~5 FLOPs (interpolation)
- Apply gains to 3 active speakers: 3 multiplies + 3 additions = 6 FLOPs
- **Total: ~11 FLOPs per grain per sample**

**64 grains, per block:** 11 * 256 * 64 = 180,224 FLOPs = **34 MFLOPS**. Even cheaper than ambisonics.

### 7.4 CPU Cost Breakdown: Per-Grain HRTF (Binaural)

**Direct convolution (128-tap HRIR):**

Per grain: 128 MACs * 2 ears = 256 MACs/sample
64 grains: 16,384 MACs/sample
At 48kHz: **787 MFLOPS**

This is about 0.8-1.6% of a CPU core. Feasible but tight when combined with granular synthesis itself.

**FFT-based convolution (512-point FFT, 256-sample blocks):**

Per grain: 2 FFTs + 2 complex multiplies + 2 IFFTs = ~80,000 FLOPs/block
64 grains: 5.12 MFLOPS/block
At 187.5 blocks/s: **960 MFLOPS** (slightly more than direct due to FFT overhead for short HRIRs)

For short HRIRs (128 samples), direct convolution is competitive with FFT. FFT wins for HRIRs > 256 samples.

### 7.5 Recommended Architecture: Tiered Approach

```
                    ┌────────────────────────┐
                    │   Grain Pool (64 max)   │
                    └──────────┬─────────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
         Per-grain        Per-grain         Per-grain
         envelope         pitch/warp       ambi encode
         (cheap)          (moderate)        (cheap)
              │                │                │
              └────────────────┼────────────────┘
                               │
                    ┌──────────▼─────────────┐
                    │  HOA3 Ambisonics Bus   │
                    │     (16 channels)      │
                    └──────────┬─────────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
         Binaural         Speaker Array      Atmos Bed
         Decoder          Decoder (VBAP)     Decoder
         (16 HRTFs)       (16 -> N spkrs)    (16 -> 12ch)
              │                │                │
         Stereo out       N-ch out          7.1.4 out
```

**Why this architecture:**

1. **Per-grain ambisonics encoding is cheap** (~35 FLOPs/sample/grain).
2. **Bus-level decoding is done ONCE** regardless of grain count.
3. **Binaural decoding cost is fixed**: 16 channels * 2 ears * convolution = 32 convolutions total (not 128 for per-grain HRTF).
4. **Scales linearly with grain count** for the cheap part (encoding), constant for the expensive part (decoding).
5. **Output format flexibility**: Same grain engine drives stereo, surround, ambisonics, or Atmos output.

### 7.6 When to Use Per-Grain vs. Submix

**Use per-grain spatialization when:**
- Each grain needs a distinct spatial identity (the whole point of spatial granular)
- The output is ambisonics (encoding is very cheap)
- Grain count is <= 128 (beyond this, even cheap per-grain processing adds up)
- Grain positions change during grain lifetime (trajectories)

**Use submix/grouped spatialization when:**
- Grain count exceeds 128-256
- Grains are clustered in spatial regions
- Output is binaural with per-source HRTF (too expensive per grain)
- CPU budget is very tight (e.g., running alongside heavy reverb, spectral processing)

**Typical submix approach:** Divide the sphere into 8-16 spatial zones. Assign each grain to the nearest zone. Spatialize zone submixes instead of individual grains. This reduces 128 spatial operations to 8-16, at the cost of spatial resolution within each zone.

### 7.7 Memory Considerations

| Resource | Per-Grain Memory | 64 Grains |
|----------|-----------------|-----------|
| Ambi coefficients (HOA3) | 64 bytes (16 floats) | 4 KB |
| VBAP gains (12 speakers) | 48 bytes | 3 KB |
| HRTF state (FFT convolver) | ~8 KB (FFT buffers) | 512 KB |
| Position state | 12 bytes | 768 bytes |
| Distance filter state | 8 bytes | 512 bytes |
| Grain audio buffer | 2-4 KB (typical grain) | 128-256 KB |

**Total per-grain spatial overhead:** ~8 KB without HRTF, ~16 KB with per-grain HRTF.
**64 grains:** ~0.5 MB without HRTF, ~1 MB with. Negligible on modern systems.

### 7.8 SIMD Optimization Opportunities

The ambisonics encoding inner loop (16 channel multiply-accumulate per grain per sample) is highly SIMD-friendly:

```cpp
// Process 4 ambisonics channels at once with SSE/NEON
void encodeGrainSIMD(float sample, const float* coeffs, float* ambiOut, int numChannels) {
    #if JUCE_USE_SSE_INTRINSICS
    __m128 s = _mm_set1_ps(sample);
    for (int ch = 0; ch < numChannels; ch += 4) {
        __m128 c = _mm_load_ps(&coeffs[ch]);
        __m128 out = _mm_load_ps(&ambiOut[ch]);
        out = _mm_add_ps(out, _mm_mul_ps(s, c));
        _mm_store_ps(&ambiOut[ch], out);
    }
    #endif
}
```

For HOA3 (16 channels): 4 SIMD iterations instead of 16 scalar operations = ~4x speedup on the encoding path.

**Sources:**
- [EmissionControl2 - Per-Grain Processing Architecture (MIT Press)](https://direct.mit.edu/comj/article-abstract/45/3/20/113899/Architecture-for-Real-Time-Granular-Synthesis-With)
- [EmissionControl2 (GitHub)](https://github.com/EmissionControl2/EmissionControl2)
- [AMD TrueAudio Next - GPU HRTF Processing](https://gpuopen.com/download/TAN_whitepaper_2016.pdf)
- [IEM GranularEncoder](https://plugins.iem.at/docs/granularencoder/)

---

## Appendix A: Quick Reference - Encoding Cost Per Output Format

| Output Format | Channels | Per-Grain Encode Cost | Bus Decode Cost | Total (64 grains) |
|--------------|----------|----------------------|-----------------|-------------------|
| Stereo (pan) | 2 | 2 FLOP/sample | None | 128 FLOP/sample |
| FOA (Ambi 1st) | 4 | 8 FLOP/sample | 0 or binaural | 512 + decode |
| HOA2 (Ambi 2nd) | 9 | 18 FLOP/sample | 0 or binaural | 1,152 + decode |
| HOA3 (Ambi 3rd) | 16 | 32 FLOP/sample | 0 or binaural | 2,048 + decode |
| VBAP (7.1.4) | 12 | ~11 FLOP/sample | None | 704 FLOP/sample |
| Per-grain HRTF | 2 | 256 FLOP/sample | None | 16,384 FLOP/sample |
| HOA3 + binaural decode | 16 -> 2 | 32 FLOP/sample | ~4,096 FLOP/sample | 2,048 + 4,096 = 6,144 |

## Appendix B: Coordinate System Convention

All formulas in this document use the following convention (matching AmbiX/JUCE):

```
Azimuth (a):    0 = front, +pi/2 = left, -pi/2 = right, +/-pi = back
Elevation (e):  0 = horizon, +pi/2 = zenith (up), -pi/2 = nadir (down)

Cartesian mapping:
  x = cos(a) * cos(e)   (front-back axis, positive = front)
  y = sin(a) * cos(e)   (left-right axis, positive = left)
  z = sin(e)             (up-down axis, positive = up)
```
