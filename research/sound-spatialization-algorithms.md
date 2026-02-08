# Sound Spatialization Algorithms for Audio Plugin Development

**Research Date:** 2026-02-07
**Purpose:** Comprehensive technical overview of spatialization techniques for VST/AU plugin development

---

## Table of Contents

1. [Stereo Spatialization](#1-stereo-spatialization)
2. [Binaural / HRTF-based Spatialization](#2-binaural--hrtf-based-spatialization)
3. [Ambisonics](#3-ambisonics)
4. [VBAP (Vector Base Amplitude Panning)](#4-vbap-vector-base-amplitude-panning)
5. [Distance Modeling](#5-distance-modeling)
6. [Room Simulation](#6-room-simulation)
7. [Frameworks and Libraries](#7-frameworks-and-libraries)
8. [Technique Comparison Matrix](#8-technique-comparison-matrix)

---

## 1. Stereo Spatialization

**Complexity:** Simple | **CPU Cost:** Negligible | **Plugin Suitability:** Excellent

### 1.1 Panning Laws

#### Linear Panning

The simplest approach. Channel gains are inversely correlated and sum to 1.

```
L(theta) = (pi/2 - theta) * (2/pi)
R(theta) = theta * (2/pi)
```

Where `theta` is in `[0, pi/2]`, left speaker at 0, right at pi/2.

**Problem:** Creates a "hole-in-the-middle" effect -- perceived loudness drops by ~3 dB at center because amplitude sums to 1 but power does not remain constant.

**Use case:** Rarely used in practice; included for completeness.

#### Equal-Power (Constant Power) Panning

The standard approach for stereo panning. Uses trigonometric functions to maintain consistent power across all positions.

```
L(theta) = cos(theta)
R(theta) = sin(theta)
```

**Why this works:** Power is proportional to squared amplitude, and `cos^2 + sin^2 = 1`. At center position, each channel has gain 0.707 (-3 dB), producing 0 dB total power at all positions.

This is the "-3 dB center" pan law and is the most widely recommended default.

#### -4.5 dB Compromise Pan Law

A geometric mean of linear and constant power methods:

```
L(theta) = sqrt( (pi/2 - theta) * (2/pi) * cos(theta) )
R(theta) = sqrt( theta * (2/pi) * sin(theta) )
```

Center gain is approximately 0.59 (-4.5 dB per channel). Some DAWs use this as a compromise.

#### -6 dB Center Pan Law

Pure linear panning where center is -6 dB per channel. Used in some broadcast contexts.

**Recommendation for plugin development:** Implement constant-power (sin/cos) as the default. Optionally expose pan law selection (-3 dB, -4.5 dB, -6 dB) as a preference.

### 1.2 Stereo Widening Techniques

#### Mid/Side (M/S) Processing

Decomposes a stereo signal into mid (sum) and side (difference) components:

```
M = (L + R) / 2      // mono-compatible center content
S = (L - R) / 2      // stereo difference (width information)

// Reconstruction:
L = M + S
R = M - S
```

**Widening:** Boost the S component relative to M. Narrowing: reduce S.

```
L_out = M + (S * width_factor)
R_out = M - (S * width_factor)
```

Where `width_factor`:
- 0.0 = mono
- 1.0 = original stereo width
- >1.0 = widened (caution: mono compatibility degrades)

**Complexity:** Simple | **CPU Cost:** Negligible (addition/subtraction per sample)

#### Haas Effect (Precedence Effect)

Exploits psychoacoustic phenomenon: sounds arriving within ~1-40 ms of each other are perceived as a single event, with localization determined by the first-arriving signal.

**Implementation:**
1. Duplicate the signal
2. Pan one copy left, one right
3. Delay one copy by 1-35 ms (typically 10-30 ms)
4. The ear receiving the earlier signal perceives the source as originating from that side

```
L_out = input
R_out = delay(input, delay_ms)   // 10-30 ms delay, source perceived left
```

**Key parameters:**
- < 1 ms: Comb filtering artifacts dominate
- 1-5 ms: ITD-like panning with some coloration
- 5-35 ms: Clear spatial widening, still fused
- > 40 ms: Perceived as echo/discrete reflection

**Complexity:** Simple | **CPU Cost:** Negligible (delay line)
**Warning:** Poor mono compatibility -- summing L+R causes comb filtering.

#### Decorrelation-Based Widening

Reduces the correlation between L and R channels to increase perceived width without Haas-effect coloration.

**Methods:**
1. **Allpass cascade:** Pass one channel through a series of allpass filters with different delays. Preserves magnitude spectrum but randomizes phase.
2. **Sparse noise convolution:** Convolve one channel with a short sparse noise sequence.
3. **Velvet noise:** A more recent approach using optimized sparse impulse sequences.

```
// Typical implementation:
L_out = mix(L, L, dry_wet)
R_out = mix(R, allpass_cascade(R), dry_wet)
```

**Complexity:** Simple-Moderate | **CPU Cost:** Low (allpass chain is ~5-10 biquads)
**Advantage:** Better mono compatibility than Haas. Phase-only decorrelation preserves tone.

### 1.3 ITD and ILD

These are the two primary binaural cues the human auditory system uses for sound localization.

#### Interaural Time Difference (ITD)

The difference in arrival time between left and right ears.

- Maximum ITD ~ 0.63 ms (sound from 90 degrees, head diameter ~17 cm)
- Dominant localization cue below ~1500 Hz
- Implementation: per-sample fractional delay line

```
ITD_samples = (head_radius / speed_of_sound) * sin(azimuth) * sample_rate
// head_radius ~ 0.0875 m, speed_of_sound ~ 343 m/s
// At 48 kHz: max ITD ~ 12.2 samples
```

#### Interaural Level Difference (ILD)

The difference in sound level between ears caused by head shadowing.

- Dominant localization cue above ~1500 Hz
- Frequency-dependent: higher frequencies are shadowed more
- Simple model: apply low-pass filter to the far ear, boost near ear

```
// Simplified ILD model:
near_ear_gain = 1.0 + ILD_factor * sin(azimuth)
far_ear_gain  = 1.0 - ILD_factor * sin(azimuth)
// Plus low-pass on far ear, cutoff decreasing with azimuth offset
```

**Combined ITD + ILD:** A simple binaural panner without full HRTF. Lower quality than HRTF but very cheap.

**Complexity:** Simple-Moderate | **CPU Cost:** Low

---

## 2. Binaural / HRTF-based Spatialization

**Complexity:** Complex | **CPU Cost:** Moderate-High | **Plugin Suitability:** Good (headphone output only)

### 2.1 How HRTF Works

A Head-Related Transfer Function describes how sound is filtered by the listener's head, torso, and pinnae (outer ears) before reaching the eardrums. It encodes all the spectral cues that allow humans to perceive 3D sound direction, including elevation and front/back disambiguation.

An HRTF is a pair of complex frequency-domain transfer functions, one per ear: `H_L(f, azimuth, elevation)` and `H_R(f, azimuth, elevation)`.

The time-domain equivalent is the HRIR (Head-Related Impulse Response). Spatializing a mono source at a given direction means convolving it with the appropriate left and right HRIRs:

```
L_out = input * HRIR_L(azimuth, elevation)
R_out = input * HRIR_R(azimuth, elevation)
```

Where `*` denotes convolution.

### 2.2 HRIR Convolution Approach

Typical HRIR lengths: 128-512 samples (at 44.1/48 kHz).

**Time-domain convolution:**
- Direct FIR filtering
- CPU cost: `2 * N` multiply-accumulate operations per sample (N = HRIR length)
- For 256-tap HRIR: 512 MACs/sample = ~24.6 million MACs/second at 48 kHz
- Feasible for small numbers of sources

**Frequency-domain convolution (overlap-add or overlap-save):**
- FFT the input block, multiply by FFT of HRIR, IFFT
- More efficient for longer filters
- Adds latency equal to the FFT block size

**Partitioned convolution:**
- Splits the HRIR into smaller segments, each processed via FFT
- First partition can be as small as the audio buffer size (minimizing latency)
- Subsequent partitions use progressively larger FFT sizes
- Achieves near-zero added latency with FFT efficiency
- The standard technique for real-time HRTF processing

### 2.3 Available HRTF Datasets

| Dataset | Subjects | Measurements | Format | Notes |
|---------|----------|-------------|--------|-------|
| **MIT KEMAR** | 1 (dummy head) | 710 positions | WAV | Classic reference set from 1994. Compact, 128-tap HRIRs. Good for prototyping. |
| **CIPIC** | 45 | 1250 per subject | Custom/SOFA | UC Davis, includes anthropometric data. 200-tap HRIRs. Widely used in research. |
| **LISTEN** (IRCAM) | 51 | 187 per subject | SOFA | Measured with individual subjects. |
| **SONICOM** | 120+ | High density | SOFA | Modern 2023 dataset, excellent spatial resolution. |
| **ARI** (Austrian Academy) | 200+ | Variable | SOFA | Large collection, some with individual measurements. |
| **Bernschutz HRTF** | 1 (Neumann KU100) | 2702 positions | SOFA | Very high spatial resolution. |

**SOFA (Spatially Oriented Format for Acoustics):**
- AES69 standard (AES69-2022)
- HDF5-based container format
- Stores HRIRs/HRTFs with associated spatial metadata
- Industry standard; all modern datasets provide SOFA files
- C/C++ libraries: libsofa, libmysofa (lightweight, BSD-licensed, used by FFmpeg)

### 2.4 Efficient Real-Time HRTF Processing

#### Minimum-Phase Decomposition

Decompose each HRIR into a minimum-phase component and a pure delay:

```
HRIR = minimum_phase_HRIR * delay(ITD)
```

**Benefits:**
- Minimum-phase HRIRs are shorter (energy concentrated at start), allowing shorter FIR filters (~64-128 taps instead of 256-512)
- ITD is handled separately with a simple fractional delay line
- Reduces convolution cost by ~50%

**Process:**
1. Take magnitude spectrum of HRIR
2. Apply Hilbert transform to log-magnitude to obtain minimum-phase
3. Extract ITD from the group delay difference between original and minimum-phase

#### Partitioned Convolution

For HRIRs that remain long (e.g., BRIRs including room reflections):

- Split filter into K partitions of size B (buffer size)
- First partition: time-domain convolution (zero added latency)
- Remaining partitions: frequency-domain convolution with increasing block sizes
- "Non-uniform" partitioned convolution optimizes block size scheduling

#### Sparse HRIR Representation

Recent research (2015+) shows HRIRs can be approximated with sparse representations:

- Truncate HRIRs to essential energy content
- Use only significant taps (sparse FIR)
- 60-80% reduction in computation with minimal perceptual degradation

### 2.5 HRTF Interpolation

Measured HRTFs exist at discrete directions. For arbitrary source positions, interpolation is required.

**Methods (in order of increasing quality):**
1. **Nearest-neighbor:** Snap to closest measured direction. Audible jumps for moving sources.
2. **Bilinear interpolation:** Interpolate between 4 nearest measurements in azimuth/elevation grid. Adequate quality.
3. **Spherical harmonic interpolation:** Decompose HRTF onto spherical harmonic basis, reconstruct at any direction. Best quality, higher compute cost.
4. **VBAP-style triangulation:** Barycentric interpolation within triangulated measurement grid. Good balance of quality and cost.

**Best practice:** Magnitude interpolation + phase/ITD interpolation separately (since phase interpolation can cause destructive interference).

### 2.6 CPU Cost Considerations

| Configuration | Per-Source Cost (48 kHz) | Notes |
|--------------|------------------------|-------|
| Direct HRIR convolution, 256-tap | ~25 MMAC/s | Simple but expensive per source |
| Minimum-phase HRIR, 128-tap + ITD | ~12 MMAC/s | Recommended approach |
| Partitioned convolution, 512-tap | ~8-15 MMAC/s | Efficient for longer filters |
| Sparse HRIR, ~64 taps effective | ~6 MMAC/s | Best efficiency |

**Practical limits on a modern CPU core (~3 GHz):**
- Direct convolution: ~20-40 simultaneous sources
- Optimized (minimum-phase + sparse): ~80-150 sources
- With SIMD (SSE/AVX): 2-8x improvement

---

## 3. Ambisonics

**Complexity:** Moderate-Complex | **CPU Cost:** Moderate-High (scales with order) | **Plugin Suitability:** Good (encoding/decoding plugins, binaural output)

### 3.1 Fundamentals

Ambisonics represents a sound field as a set of **spherical harmonic** coefficients, independent of any particular loudspeaker layout. It is a two-stage process:

1. **Encoding:** Sound sources are encoded into an ambisonic "scene" (B-format)
2. **Decoding:** The B-format scene is decoded for a specific reproduction system (speakers or binaural)

This separation is the key advantage: encode once, decode to any output format.

### 3.2 Spherical Harmonics Basics

Spherical harmonics `Y_n^m(theta, phi)` are orthogonal basis functions defined on the surface of a sphere, analogous to Fourier series but in 3D angular space.

- `n` = order (0, 1, 2, ...)
- `m` = degree (-n to +n)
- Order 0: omnidirectional (1 component)
- Order 1: three figure-8 patterns (3 components)
- Order n: adds 2n+1 new components

**Channel count formulas:**
- 3D (full sphere): `(N+1)^2` channels for order N
- 2D (horizontal only): `2N+1` channels for order N

| Order | 3D Channels | 2D Channels | Spatial Resolution |
|-------|------------|------------|-------------------|
| 0 | 1 | 1 | Omnidirectional (no direction) |
| 1 (FOA) | 4 | 3 | ~60 degrees beam width |
| 2 | 9 | 5 | ~45 degrees |
| 3 | 16 | 7 | ~30 degrees |
| 4 | 25 | 9 | ~22 degrees |
| 5 | 36 | 11 | ~18 degrees |
| 7 | 64 | 15 | ~13 degrees |

### 3.3 B-Format Encoding

#### First-Order Ambisonics (FOA) - B-Format

The classic 4-channel format using ACN channel ordering (Ambisonics Channel Number):

```
Channel 0 (W): Y_0^0  = 1                           // omnidirectional (pressure)
Channel 1 (Y): Y_1^-1 = sin(theta) * sin(phi)       // left-right (Y axis)
Channel 2 (Z): Y_1^0  = cos(theta)                   // up-down (Z axis)  [3D only]
Channel 3 (X): Y_1^1  = sin(theta) * cos(phi)        // front-back (X axis)
```

Where `theta` = inclination (0 = top), `phi` = azimuth.

**Encoding a mono source at direction (azimuth, elevation):**

```
W = signal * 1.0                         // or with W-weighting: * 1/sqrt(2)
Y = signal * sin(azimuth) * cos(elevation)
Z = signal * sin(elevation)
X = signal * cos(azimuth) * cos(elevation)
```

**Normalization conventions:**
- **SN3D** (Schmidt semi-normalized): Most common modern standard. Used by AmbiX format.
- **N3D** (full 3D normalization): SN3D * sqrt(2n+1). Used by some academic tools.
- **FuMa** (Furse-Malham): Legacy format. W channel has 1/sqrt(2) scaling. Channel order: WXYZ.

**Modern standard:** AmbiX = ACN channel ordering + SN3D normalization.

#### Higher-Order Encoding

For order N, encoding a source at direction `(az, el)`:

```
for each channel c with order n, degree m:
    B[c] = signal * Y_n^m(az, el) * SN3D_normalization(n, m)
```

The spherical harmonic values can be precomputed for each source direction and only recomputed when the source moves.

### 3.4 Ambisonics Decoding

#### Loudspeaker Decoding

Given `M` loudspeakers at known positions and `C = (N+1)^2` ambisonic channels:

```
speaker_feeds = D * B
```

Where `D` is an `M x C` decoder matrix and `B` is the `C x 1` ambisonic signal vector.

**Decoder types:**
- **Basic (pseudoinverse):** `D = Y^T * (Y * Y^T)^-1` where Y is the re-encoding matrix. Simple but uneven energy distribution.
- **Max-rE:** Applies frequency-dependent weighting to maximize the energy vector. Reduces side lobes. Standard for high-frequency content.
- **AllRAD (All-Round Ambisonic Decoding):** Projects onto a virtual dense t-design layout, then VBAP-maps to actual speakers. Current standard for irregular layouts.

**Dual-band decoding:** Use basic decoder below ~400-700 Hz (preserves amplitude/velocity), max-rE above (preserves energy). This is the recommended approach.

#### Binaural Decoding

Two main approaches:

1. **Virtual loudspeaker method:**
   - Define K virtual loudspeaker positions on a sphere
   - Decode ambisonics to virtual speakers (standard matrix decode)
   - Convolve each virtual speaker feed with its corresponding HRTF pair
   - Sum all contributions per ear
   - CPU cost: K convolutions (K typically 26-50 for adequate quality)

2. **Direct spherical harmonic HRTF method:**
   - Decompose HRTF dataset into spherical harmonic coefficients
   - Directly convolve ambisonic channels with SH-domain HRTF filters
   - Only `(N+1)^2` convolutions needed (4 for FOA, 9 for 2nd order, etc.)
   - More efficient but requires SH-decomposed HRTFs

3. **MagLS (Magnitude Least Squares):**
   - Modern approach: optimizes magnitude match while allowing phase freedom
   - Reduces artifacts from SH truncation at high frequencies
   - Used by SPARTA and IEM plugin suites
   - Currently considered the state-of-the-art binaural decoder

### 3.5 CPU Cost Analysis

| Order | Channels | Binaural Decode (virtual speakers, 26 HRTF pairs) | Binaural Decode (SH-domain) |
|-------|----------|---------------------------------------------------|----------------------------|
| 1 (FOA) | 4 | 26 HRTF convolutions | 4 HRTF convolutions |
| 2 | 9 | 26 HRTF convolutions | 9 HRTF convolutions |
| 3 | 16 | 50 HRTF convolutions | 16 HRTF convolutions |
| 5 | 36 | 50+ HRTF convolutions | 36 HRTF convolutions |
| 7 | 64 | 86+ HRTF convolutions | 64 HRTF convolutions |

**Encoding cost per source:** Negligible (just multiply by SH coefficients per sample).

**Practical notes:**
- FOA (1st order) is very affordable -- 4-channel bus + binaural decode is ~4 HRTF convolutions
- 3rd order is the sweet spot for quality vs. cost in plugin use
- 5th+ order mainly useful for loudspeaker reproduction in studios
- For a plugin producing binaural output, FOA or 3rd order is recommended

---

## 4. VBAP (Vector Base Amplitude Panning)

**Complexity:** Moderate | **CPU Cost:** Low | **Plugin Suitability:** Good (multi-speaker output)

### 4.1 Algorithm Overview

VBAP, created by Ville Pulkki (1997), positions virtual sound sources by distributing amplitude across the minimal set of surrounding loudspeakers.

**Core principle:** For a desired source direction vector `p`, find the loudspeaker subset whose direction vectors span a region containing `p`, then solve for gain factors.

#### 2D Case (Pair-wise Panning)

Given two adjacent speakers with direction vectors `l1` and `l2`:

```
p = g1 * l1 + g2 * l2

// Matrix form:
[g1, g2] = p^T * L^(-1)

where L = [l1, l2]^T  (2x2 matrix of speaker direction vectors)
```

Then normalize for constant power: `g_norm = g / sqrt(g1^2 + g2^2)`

#### 3D Case (Triplet-wise Panning)

For 3D layouts, speakers are triangulated using convex hull (Delaunay triangulation on the sphere). For each triangle of three speakers with direction vectors `l1, l2, l3`:

```
[g1, g2, g3] = p^T * L^(-1)

where L = [l1, l2, l3]^T  (3x3 matrix)
```

Source belongs to this triangle if all gains are non-negative.

### 4.2 Implementation Steps

1. **Preprocessing (done once per speaker layout):**
   - Convert speaker positions to unit direction vectors
   - For 2D: sort speakers by angle, form adjacent pairs
   - For 3D: compute convex hull triangulation
   - Precompute inverse matrices `L^(-1)` for all pairs/triplets

2. **Per-source, per-block (real-time):**
   - Convert source direction to unit vector
   - Find active pair/triplet (test against precomputed triangulation)
   - Compute gains: `g = p^T * L_inv`
   - Normalize: `g = g / ||g||`
   - Apply gains to source signal, route to speakers
   - Smooth gain changes between blocks to avoid clicks

### 4.3 Spread Control

Basic VBAP creates point-like sources (only 2-3 speakers active). To create wider sources:

- Generate auxiliary source directions around the main direction
- Compute VBAP gains for each auxiliary direction
- Sum and normalize all gain vectors
- "Spread" parameter controls the angular spread of auxiliaries (0-360 degrees)

### 4.4 Comparison with Ambisonics

| Property | VBAP | Ambisonics |
|----------|------|-----------|
| **Localization precision** | Excellent (sharp imaging) | Good at high orders, blurry at low orders |
| **Sweet spot** | Required (speaker-dependent) | Required (but more forgiving) |
| **Moving sources** | May have width fluctuations | Smooth, consistent |
| **Speaker layout flexibility** | Any layout (triangulated) | Prefers regular layouts |
| **Channel count** | = number of speakers | = (N+1)^2, independent of speakers |
| **Scene rotation** | Requires recomputation | Trivial matrix multiply |
| **CPU cost** | Very low per source | Higher (especially decoding) |
| **Diffuse/wide sources** | Spread parameter (hack-ish) | Natural with decorrelation |
| **Scalability** | Straightforward | Order determines quality ceiling |

**When to use VBAP:** Known speaker layout, point-like sources, low CPU budget.
**When to use Ambisonics:** Scene rotation needed, format-agnostic workflow, binaural output, diffuse sound fields.

---

## 5. Distance Modeling

**Complexity:** Simple-Moderate | **CPU Cost:** Low | **Plugin Suitability:** Excellent

### 5.1 Distance Attenuation

#### Inverse Square Law (Physical Model)

Sound pressure decreases proportionally to 1/distance:

```
gain = reference_distance / distance          // for distance > reference_distance
gain = 1.0                                    // for distance <= reference_distance
```

In dB: every doubling of distance = -6 dB.

#### Inverse Distance (Linear Rolloff in dB)

A perceptually smoother alternative:

```
gain = reference_distance / max(distance, reference_distance)
```

Many implementations clamp at a maximum distance to avoid silence:

```
gain = 1.0 - (distance - min_dist) / (max_dist - min_dist)
gain = clamp(gain, 0.0, 1.0)
```

#### Logarithmic Rolloff

```
gain_dB = -20 * rolloff_factor * log10(distance / reference_distance)
```

Where `rolloff_factor`:
- 1.0 = inverse distance (-6 dB/doubling, free-field)
- 0.5 = half rolloff (-3 dB/doubling, semi-reverberant)
- 2.0 = double rolloff (-12 dB/doubling, exaggerated)

**Recommendation:** Expose rolloff_factor as a parameter. Default 1.0 for realistic behavior.

### 5.2 Air Absorption (Frequency-Dependent Attenuation)

High frequencies are attenuated more over distance due to molecular absorption.

**Approximate absorption coefficients (dB per 100 meters, 20C, 50% humidity):**

| Frequency | Absorption |
|-----------|-----------|
| 250 Hz | ~0.01 dB |
| 1 kHz | ~0.1 dB |
| 2 kHz | ~0.25 dB |
| 4 kHz | ~0.8 dB |
| 8 kHz | ~2.5 dB |
| 16 kHz | ~8.0 dB |

**Implementation:** Apply a low-pass filter whose cutoff frequency decreases with distance.

```
// Simple model: single-pole low-pass
cutoff_freq = 20000.0 / (1.0 + air_absorption_factor * distance)

// Or: parametric EQ shelving filter
high_shelf_gain_dB = -absorption_coefficient * distance / 100.0
```

**Better model:** Use a 2-3 band shelving EQ, each band with its own distance-dependent attenuation curve. This models the frequency-dependent nature more accurately.

**Complexity:** Simple | **CPU Cost:** Negligible (1 biquad filter per source)

### 5.3 Early Reflections Simulation

See Section 6 for full details. In the context of distance modeling:

- Close sources: high direct-to-reflected ratio (dry)
- Far sources: low direct-to-reflected ratio (wet/reverberant)
- The wet/dry balance is itself a powerful distance cue

```
dry_gain = 1.0 / distance
wet_gain = reverb_amount * (1.0 - dry_gain)
```

### 5.4 Doppler Effect

The pitch shift caused by relative motion between source and listener.

```
frequency_ratio = speed_of_sound / (speed_of_sound + radial_velocity)
```

Where `radial_velocity` is positive when source moves away, negative when approaching.

**Implementation:**
- Track the source distance over time
- Compute radial velocity: `v = (distance_current - distance_previous) / dt`
- Apply pitch shift via variable-rate delay line (interpolated read pointer)

```
// Delay line approach:
delay_samples = distance / speed_of_sound * sample_rate
// Change in delay_samples over time creates pitch shift naturally
write_pos += 1
read_pos += 1.0 - (radial_velocity / speed_of_sound)
output = interpolated_read(delay_line, read_pos)
```

**Key considerations:**
- Use cubic or sinc interpolation for the variable delay to avoid aliasing
- Smooth velocity changes to prevent clicks
- Scale effect: typical musical Doppler is subtle (few cents pitch shift)
- Often exaggerated in games, kept subtle in music production

**Complexity:** Moderate | **CPU Cost:** Low (one interpolated delay line per source)

---

## 6. Room Simulation

**Complexity:** Moderate-Complex | **CPU Cost:** Moderate-High | **Plugin Suitability:** Good

### 6.1 Reverb as Spatial Cue

Reverberation is one of the strongest distance cues:

- **Direct-to-reverberant ratio (DRR):** Decreases with distance. A mono source sent to a reverb with increasing wet mix sounds farther away.
- **Initial Time Delay Gap (ITDG):** Time between direct sound and first reflection. Smaller rooms or closer walls = shorter ITDG.
- **Reverb time (RT60):** Encodes room size. Longer RT60 = larger perceived space.
- **Spectral content of reverb:** Darker reverb = more absorptive materials, softer room.

### 6.2 Image Source Method (ISM)

A geometric technique for computing early reflections by "mirroring" the sound source across each room boundary.

**Algorithm:**
1. Define room geometry (rectangular room = 6 walls)
2. For each wall, create a mirror image of the source
3. For each image source, check visibility from listener position
4. Compute path length and apply:
   - Distance attenuation (1/r)
   - Wall absorption (material-dependent frequency-dependent loss)
   - Delay = path_length / speed_of_sound
5. For higher-order reflections, recursively mirror image sources
6. Apply spatialization (HRTF/panning) to each reflection based on its arrival direction

**Reflection order and computational cost:**

| Order | Reflections (rectangular room) | CPU Cost |
|-------|-------------------------------|----------|
| 1 | 6 | Very low |
| 2 | 30 | Low |
| 3 | ~150 | Moderate |
| 4 | ~750 | High |
| 5 | ~3750 | Very high |

**For real-time plugin use:** Orders 1-3 are practical (6-150 reflections). Beyond order 3, switch to statistical reverb (FDN).

**Implementation structure:**
```
// Per source:
direct_signal = attenuate(source, direct_distance) * HRTF(direct_direction)

for each reflection in image_sources:
    reflected = attenuate(source, reflection.distance) * reflection.wall_absorption
    reflected = delay(reflected, reflection.delay_samples)
    reflected = HRTF(reflected, reflection.direction)  // or panning
    output += reflected

// Add late reverb:
output += FDN_reverb(source, room_params)
```

### 6.3 Practical Distance Perception Pipeline

Combining all distance cues for a complete spatialization chain:

```
Input: mono_source, source_position, listener_position

1. Compute distance = ||source_position - listener_position||
2. Compute direction = normalize(source_position - listener_position)

3. Apply distance attenuation:     gain = ref_dist / distance
4. Apply air absorption:           low_pass(source, cutoff_from_distance)
5. Apply Doppler (if moving):      variable_delay(source, radial_velocity)
6. Apply spatialization:           HRTF(source, direction) or panning
7. Apply early reflections:        ISM reflections (orders 1-2)
8. Mix with reverb:                wet/dry based on distance
   - Close: mostly dry
   - Far: mostly wet

Output: L_ear, R_ear (binaural) or speaker_feeds
```

---

## 7. Frameworks and Libraries

### 7.1 Spatial Audio Framework (SAF)

- **URL:** https://github.com/leomccormack/Spatial_Audio_Framework
- **Language:** C/C++
- **License:** ISC (permissive)
- **Modules:** HOA, VBAP, HRIR/HRTF, reverb, spherical harmonics, utilities
- **Performance:** Uses Intel MKL, Apple Accelerate, OpenBLAS, SIMD intrinsics
- **Plugin examples:** SPARTA suite (VST/AU/LV2 plugins built with JUCE + SAF)
- **Relevance:** The most comprehensive open-source spatial audio library. Ideal for JUCE plugin integration.

### 7.2 SPARTA Plugin Suite

- **URL:** https://leomccormack.github.io/sparta-site/
- **Built with:** JUCE + SAF
- **Plugins include:** Ambisonic encoder, decoder, binaural renderer, VBAP panner, room simulator, rotator, and more
- **Relevance:** Reference implementations for all major spatialization techniques as JUCE plugins.

### 7.3 libmysofa

- **URL:** https://github.com/hoene/libmysofa
- **Language:** C
- **License:** BSD-3
- **Purpose:** Lightweight SOFA file reader for loading HRTF datasets
- **Used by:** FFmpeg, PipeWire, and many audio applications
- **Relevance:** Essential for loading standardized HRTF data in plugins.

### 7.4 3D Tune-In Toolkit

- **URL:** https://github.com/3DTune-In/3dti_AudioToolkit
- **Language:** C++
- **License:** GPL-3.0
- **Features:** HRTF-based binaural rendering, hearing aid simulation, reverb
- **Relevance:** Well-documented binaural rendering library.

### 7.5 IEM Plugin Suite

- **URL:** https://plugins.iem.at/
- **Built with:** JUCE
- **Plugins:** Ambisonics encoder, decoder, binaural decoder, room encoder, and more
- **Relevance:** High-quality open-source ambisonics plugins. Good reference for JUCE implementation patterns.

---

## 8. Technique Comparison Matrix

### Overall Comparison

| Technique | Complexity | CPU/Source | Output | Elevation | Best For |
|-----------|-----------|-----------|--------|-----------|----------|
| Stereo panning (equal power) | Simple | Negligible | Stereo | No | Basic L/R positioning |
| M/S widening | Simple | Negligible | Stereo | No | Stereo width control |
| Haas effect | Simple | Negligible | Stereo | No | Stereo widening (caution: mono compat) |
| Decorrelation widening | Simple | Low | Stereo | No | Mono-compatible stereo widening |
| ITD + ILD (simple binaural) | Simple-Mod | Low | Binaural | Limited | Quick binaural panning |
| HRTF binaural | Complex | Moderate | Binaural | Yes | Headphone 3D audio |
| FOA Ambisonics (1st order) | Moderate | Low-Mod | Any | Yes | Scene-based workflows, VR |
| HOA Ambisonics (3rd order) | Complex | Moderate | Any | Yes | High-quality spatial scenes |
| VBAP (2D) | Moderate | Low | Multi-speaker | No | Multi-speaker installations |
| VBAP (3D) | Moderate | Low | Multi-speaker | Yes | Immersive speaker setups |
| Distance modeling | Simple-Mod | Low | Any | N/A | Depth/distance perception |
| ISM early reflections | Moderate | Moderate | Any | Yes | Room simulation |

### Plugin Development Recommendations

**For a stereo spatialization plugin (simplest entry point):**
- Equal-power panning + M/S widening + Haas effect + decorrelation
- Add ITD/ILD for basic binaural mode
- Include distance attenuation + air absorption
- Complexity: Simple-Moderate
- Single stereo output

**For a binaural 3D panner plugin:**
- HRTF convolution with minimum-phase decomposition
- Ship with MIT KEMAR or a permissive HRTF set; allow SOFA file loading
- HRTF interpolation (bilinear minimum)
- Distance modeling (attenuation + air absorption + reverb wet/dry)
- Optional Doppler
- Complexity: Moderate-Complex
- Headphone-only output

**For an ambisonics suite:**
- Separate encoder and decoder plugins
- Encoder: mono/stereo to FOA or HOA
- Decoder: FOA/HOA to binaural (MagLS) or loudspeaker (AllRAD)
- Scene rotator plugin (trivial matrix multiply in SH domain)
- Complexity: Complex
- Consider using SAF library

**For a comprehensive spatial audio plugin:**
- Combine HRTF binaural + VBAP multi-speaker output
- Distance modeling pipeline (attenuation + air absorption + early reflections + reverb blend)
- Ambisonics as intermediate format (encode, manipulate, decode)
- Doppler for moving sources
- Complexity: Complex
- This is essentially building a spatial audio engine

---

## References and Sources

- [SPARTA - Spatial Audio Real-Time Applications](https://leomccormack.github.io/sparta-site/)
- [Spatial Audio Framework (SAF) on GitHub](https://github.com/leomccormack/Spatial_Audio_Framework)
- [CMU Loudness Concepts & Panning Laws](https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/)
- [SPAT Revolution Panning Algorithms](https://doc.flux.audio/spat-revolution/Spatialisation_Technology_Panning_Algorithms.html)
- [SOFA Conventions (AES69)](https://www.sofaconventions.org/mediawiki/index.php/SOFA_(Spatially_Oriented_Format_for_Acoustics))
- [MIT KEMAR HRTF Dataset](https://sound.media.mit.edu/resources/KEMAR.html)
- [CIPIC HRTF Database](https://github.com/amini-allight/cipic-hrtf-database)
- [SONICOM HRTF Dataset](https://www.sonicom.eu/tools-and-resources/hrtf-dataset/)
- [VBAP Library (Aalto University)](http://research.spa.aalto.fi/projects/vbap-lib/vbap.html)
- [VBAP on GitHub (polarch)](https://github.com/polarch/Vector-Base-Amplitude-Panning)
- [Higher-Order Ambisonics Library (polarch)](https://github.com/polarch/Higher-Order-Ambisonics)
- [HOLOPHONIX Ambisonics Guide](https://holophonix.xyz/documentation/docs/guides/hoa-guide/)
- [Blue Ripple Sound B-Format Notes](https://www.blueripplesound.com/notes/bformat)
- [Ambisonics on Wikipedia](https://en.wikipedia.org/wiki/Ambisonics)
- [Partitioned Convolution for Real-Time Auralization (Wefers, RWTH Aachen)](https://publications.rwth-aachen.de/record/466561/files/466561.pdf)
- [FFTConvolver (HiFi-LoFi)](https://github.com/HiFi-LoFi/FFTConvolver)
- [libmysofa](https://github.com/hoene/libmysofa)
- [3D Tune-In Toolkit](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0211899)
- [IEM Plugin Suite](https://plugins.iem.at/)
- [Audiokinetic Image Source Approach to Dynamic Early Reflections](https://www.audiokinetic.com/en/blog/image-source-approach-to-dynamic-early-reflections/)
- [Stereo Widening with Decorrelation (Stanford CCRMA)](https://ccrma.stanford.edu/~orchi/Documents/DAFx24_paper_92.pdf)
- [Anaglyph Binaural Encoder](https://anaglyph.dalembert.upmc.fr/)
- [Implementing Distance Simulation with Sound (Vanderbilt)](https://wp0.vanderbilt.edu/youngscientistjournal/article/implementing-an-audio-processing-system-to-simulate-realistic-distance-with-sound)
- [Panning Law (Wikipedia)](https://en.wikipedia.org/wiki/Panning_law)
- [Skippy Studio - The Law of Panning](https://skippystudio.nl/2024/12/the-law-of-panning-it-is-not-as-simple-as-it-looks/)
- [VBAP Definition (Audio Drama Production)](https://audiodramaproduction.com/audio-formats-and-codecs-glossary/vector-based-amplitude-panning-vbap/)
