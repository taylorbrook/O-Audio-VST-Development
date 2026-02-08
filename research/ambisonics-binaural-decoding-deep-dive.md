# Ambisonics-to-Binaural Decoding: Complete Technical Deep Dive

**Research Date:** 2026-02-08
**Target:** JUCE C++ plugin implementing binaural output for HOA3 spatial granular synthesis (O-GrainScatter)
**Ambisonics Order:** 3rd order (HOA3) = 16 channels (ACN/SN3D)
**Output:** Stereo headphone (binaural)
**Confidence:** HIGH (peer-reviewed algorithms, production-proven open-source implementations)

---

## Table of Contents

1. [Virtual Speaker Decoding (Baseline Approach)](#1-virtual-speaker-decoding-baseline-approach)
2. [MagLS (Magnitude Least Squares) Decoding](#2-magls-magnitude-least-squares-decoding)
3. [HRTF Data Sources](#3-hrtf-data-sources)
4. [Convolution Implementation](#4-convolution-implementation)
5. [Head Rotation Support](#5-head-rotation-support)
6. [Practical Implementation in JUCE](#6-practical-implementation-in-juce)
7. [Alternatives to Full Convolution](#7-alternatives-to-full-convolution)
8. [References](#8-references)

---

## 1. Virtual Speaker Decoding (Baseline Approach)

### 1.1 Concept

Virtual speaker decoding is the most intuitive approach to rendering ambisonics over headphones. The idea is straightforward:

1. Place N virtual loudspeakers at known positions on a sphere around the listener.
2. Decode the ambisonics signal into N speaker feed signals using a decoding matrix.
3. Convolve each speaker feed with the HRTF (Head-Related Impulse Response) corresponding to that speaker's direction.
4. Sum the convolved signals to produce left-ear and right-ear outputs.

```
HOA3 (16 channels)
    |
    v
Decoding Matrix D  [N_speakers x 16]
    |
    v
N virtual speaker feeds  (one mono signal per virtual speaker)
    |
    v  (per speaker)
Convolve with HRIR_left(speaker_direction) and HRIR_right(speaker_direction)
    |
    v
Sum all left-ear signals -> Left output
Sum all right-ear signals -> Right output
```

### 1.2 Decoding Matrix Computation

The decoding matrix D maps (N+1)^2 ambisonics channels to N speaker feeds. It is computed from the **pseudoinverse** of the spherical harmonics (SH) matrix evaluated at the speaker positions.

**Step 1: Construct the SH matrix Y**

Evaluate the real spherical harmonics (ACN ordering, SN3D normalization) at each speaker position:

```
Y = [ y(Omega_1), y(Omega_2), ..., y(Omega_L) ]

where:
  y(Omega_i) = [ Y_0^0(Omega_i), Y_1^-1(Omega_i), Y_1^0(Omega_i), Y_1^1(Omega_i), ..., Y_N^N(Omega_i) ]^T

Y is a matrix of size [(N+1)^2 x L]
  (N+1)^2 = number of SH channels (16 for HOA3)
  L = number of speaker positions
```

**Step 2: Compute the pseudoinverse decoding matrix**

For a well-conditioned speaker layout (e.g., Lebedev grid), the basic decoding matrix is:

```
D = Y^T * (Y * Y^T)^(-1)

D has dimensions [L x (N+1)^2]

Equivalently, using the Moore-Penrose pseudoinverse:
  D = pinv(Y)^T = (Y^+)^T
```

When the speaker positions form a t-design or Lebedev grid with appropriate integration weights w_i, the decoding simplifies to a weighted re-encoding transpose:

```
D_i,n = w_i * Y_n(Omega_i)

where w_i is the quadrature weight for speaker i
```

For a uniform distribution (equal weights), this reduces to:

```
D = (4*pi / L) * Y^T
```

**Step 3: Decode ambisonics to speaker feeds**

```
s(t) = D * a(t)

where:
  a(t) = [a_0(t), a_1(t), ..., a_15(t)]^T  (16 HOA3 channels)
  s(t) = [s_1(t), s_2(t), ..., s_L(t)]^T    (L speaker feeds)
```

### 1.3 Speaker Count Requirements

The minimum number of speakers depends on the ambisonics order N:

| Order N | Min Speakers (N+1)^2 | Recommended 2*(N+1)^2 | Practical Choice |
|---------|---------------------|----------------------|------------------|
| 1 | 4 | 8 | 6 (octahedron) or 8 (cube) |
| 2 | 9 | 18 | 14 (Lebedev) |
| 3 | 16 | 32 | 26 (Lebedev) or 50 (Lebedev) |
| 4 | 25 | 50 | 50 (Lebedev) |
| 5 | 36 | 72 | 86 (Lebedev) |

For HOA3 specifically:
- **Minimum:** 16 speakers -- the decoding matrix Y is square, barely invertible, highly sensitive to position errors
- **Recommended:** 26-50 speakers on a Lebedev grid -- overdetermined system, well-conditioned pseudoinverse, smooth spatial reconstruction

### 1.4 Lebedev Grids for Ambisonics

Lebedev quadrature grids are optimal for ambisonics because they provide exact integration of spherical harmonics up to a certain degree. The key Lebedev grids for ambisonics are:

| Lebedev Grid Nodes | Max SH Degree | Suitable for Ambisonics Order |
|---------------------|---------------|-------------------------------|
| 6 | 3 | 1 |
| 14 | 5 | 2 |
| 26 | 7 | 3 |
| 50 | 11 | 5 |
| 86 | 15 | 7 |

For HOA3, the **26-node Lebedev grid** provides exact integration up to SH degree 7, which comfortably covers 3rd-order spherical harmonics (degree 3). Using the 50-node grid gives extra headroom and better conditioning.

### 1.5 Binaural Rendering from Virtual Speaker Feeds

Once the L speaker feeds are computed, binaural output is:

```
p_left(t)  = sum_{i=1}^{L}  s_i(t) * h_left(Omega_i, t)
p_right(t) = sum_{i=1}^{L}  s_i(t) * h_right(Omega_i, t)

where:
  h_left(Omega_i, t) = left-ear HRIR for direction Omega_i
  h_right(Omega_i, t) = right-ear HRIR for direction Omega_i
  * denotes convolution
```

In the frequency domain (per FFT bin k):

```
P_left(k)  = sum_{i=1}^{L}  S_i(k) * H_left(Omega_i, k)
P_right(k) = sum_{i=1}^{L}  S_i(k) * H_right(Omega_i, k)
```

### 1.6 Precomputing Binaural Decoding Filters

Rather than decoding to speakers then convolving, we can precompute combined binaural decoding filters that go directly from ambisonics channels to left/right ear signals:

```
For each SH channel n (0 to 15) and each ear (left, right):

  h_bin_left_n(t)  = sum_{i=1}^{L}  D_{i,n} * h_left(Omega_i, t)
  h_bin_right_n(t) = sum_{i=1}^{L}  D_{i,n} * h_right(Omega_i, t)

In the frequency domain:
  H_bin_left_n(k)  = sum_{i=1}^{L}  D_{i,n} * H_left(Omega_i, k)
  H_bin_right_n(k) = sum_{i=1}^{L}  D_{i,n} * H_right(Omega_i, k)
```

This gives us 16 left-ear filters and 16 right-ear filters (32 total filters for HOA3). At runtime, the binaural output is then:

```
P_left(k)  = sum_{n=0}^{15}  A_n(k) * H_bin_left_n(k)
P_right(k) = sum_{n=0}^{15}  A_n(k) * H_bin_right_n(k)
```

This is computationally identical to 32 convolutions (16 channels x 2 ears), but the filters are precomputed once during initialization.

### 1.7 Limitations of Virtual Speaker Decoding

The virtual speaker approach has well-documented shortcomings:

1. **Spatial aliasing above the aliasing frequency:** The limited SH order cannot reproduce fine spatial structure at high frequencies. Virtual speaker decoding treats all frequencies identically, leading to spectral coloration and timbral artifacts above the aliasing frequency.

2. **Comb filtering from phase errors:** At high frequencies, the SH representation introduces phase errors. When these phase-erroneous signals are convolved with HRTFs (which have their own complex phase structure), the result is constructive and destructive interference -- audible as spectral coloration.

3. **Energy loss at high frequencies:** The truncated SH representation loses energy at high frequencies, leading to a dull-sounding binaural output.

These limitations motivate the MagLS approach described next.

---

## 2. MagLS (Magnitude Least Squares) Decoding

### 2.1 Why MagLS is Superior for Binaural Rendering

MagLS, introduced by Schoerkhuber, Zotter, and Hoeldrich (2018), exploits a key psychoacoustic principle: **the human auditory system is insensitive to interaural phase differences above approximately 1.5-2 kHz.** Below this frequency, interaural time differences (ITDs) carry spatial information. Above it, interaural level differences (ILDs) dominate spatial perception.

Standard least-squares decoding tries to match both magnitude and phase of the HRTF across all frequencies. This is achievable below the aliasing frequency but fails above it, producing both magnitude and phase errors. MagLS takes a smarter approach:

- **Below the aliasing frequency f_a:** Use standard least-squares (match both magnitude and phase)
- **Above f_a:** Match only the magnitude (discard phase), using the freed degrees of freedom to achieve a much better magnitude fit

The result is dramatically reduced spectral coloration and more natural-sounding binaural output compared to virtual speaker decoding, especially for low ambisonics orders (1st through 5th) where the aliasing frequency is in the critical hearing range.

### 2.2 The Aliasing Frequency

The aliasing frequency marks the boundary above which a given ambisonics order cannot accurately reproduce the spatial sound field on the surface of a sphere the size of the human head:

```
f_a = (N * c) / (2 * pi * r)

where:
  N = ambisonics order
  c = speed of sound = 343 m/s
  r = head radius = 0.0875 m (average human)
```

For specific orders:

| Order N | Aliasing Frequency f_a |
|---------|----------------------|
| 1 | ~625 Hz |
| 2 | ~1250 Hz |
| 3 | ~1875 Hz |
| 4 | ~2500 Hz |
| 5 | ~3125 Hz |
| 7 | ~4375 Hz |

**For HOA3: f_a = 1875 Hz.** This means that above 1875 Hz, the 3rd-order SH representation cannot capture the fine spatial structure of the HRTF. MagLS mitigates this by only matching the magnitude spectrum above this frequency.

In practice, some implementations use a fixed cutoff of f_c = 3 kHz rather than the order-dependent aliasing frequency. Schoerkhuber et al. found that phase errors above 3 kHz are generally imperceptible, regardless of ambisonics order. However, using f_a = N*c/(2*pi*r) is more principled and adapts automatically to the ambisonics order.

### 2.3 The MagLS Algorithm

The MagLS algorithm operates in the frequency domain, computing binaural decoding filters bin by bin. It uses an iterative procedure that processes frequency bins from low to high.

**Notation:**
- H(Omega, f) = HRTF at direction Omega, frequency f (complex-valued, for one ear)
- h(f) = [H(Omega_1, f), H(Omega_2, f), ..., H(Omega_Q, f)]^T = HRTF vector over Q measurement directions
- Y = [(N+1)^2 x Q] SH matrix evaluated at HRTF measurement directions
- h_nm(f) = SH-domain HRTF coefficients at frequency f

**Step 1: SH decomposition of the HRTF (least-squares)**

Below f_a, compute the SH coefficients of the HRTF via least-squares:

```
h_nm(f) = Y^+ * h(f)     for f <= f_a

where Y^+ = (Y^H * Y)^(-1) * Y^H  is the pseudoinverse of Y
```

This matches both magnitude and phase of the HRTF as closely as possible with (N+1)^2 SH coefficients.

**Step 2: Magnitude-only matching above f_a (iterative)**

Above f_a, the algorithm uses the previous frequency bin's phase to reconstruct phase, while optimizing only for magnitude:

```
For each frequency bin f_k where f_k > f_a, iterating from f_a upward:

  1. Take the magnitude of the measured HRTF:
     m(f_k) = |h(f_k)|

  2. Reconstruct phase from the SH-domain result of the previous bin:
     phi(f_k) = angle( Y^H * h_nm(f_{k-1}) )

  3. Construct a modified HRTF vector with measured magnitude but reconstructed phase:
     h_mod(f_k) = m(f_k) .* exp(j * phi(f_k))

  4. Compute SH coefficients for this modified HRTF:
     h_nm(f_k) = Y^+ * h_mod(f_k)
```

This is repeated independently for the left and right ears.

**Step 3: Smooth transition**

To avoid artifacts at the crossover frequency, a smooth transition (half-octave fade) blends between the LS and MagLS solutions around f_a:

```
h_nm_final(f) = alpha(f) * h_nm_LS(f) + (1 - alpha(f)) * h_nm_MagLS(f)

where alpha(f) is a crossfade function:
  alpha(f) = 1.0                         for f < f_a / sqrt(2)
  alpha(f) = cos^2(pi/2 * log2(f/f_a))   for f_a/sqrt(2) <= f <= f_a*sqrt(2)
  alpha(f) = 0.0                         for f > f_a * sqrt(2)
```

### 2.4 Complete MagLS Pseudocode

```
FUNCTION computeMagLSFilters(hrtf_left, hrtf_right, speaker_dirs, order, sampleRate):
    // hrtf_left[Q][NFFT]:  HRTF spectra for left ear, Q measurement directions
    // hrtf_right[Q][NFFT]: HRTF spectra for right ear, Q measurement directions
    // speaker_dirs[Q][2]:  (azimuth, elevation) of each HRTF measurement direction
    // order: ambisonics order (3 for HOA3)
    // sampleRate: audio sample rate

    N_SH = (order + 1)^2   // 16 for HOA3
    N_FFT = FFT size of HRTF data
    N_bins = N_FFT / 2 + 1

    // Step 1: Build SH matrix at HRTF measurement directions
    Y = zeros(N_SH, Q)
    FOR each direction q in 0..Q-1:
        Y[:, q] = realSphericalHarmonics(order, speaker_dirs[q])
    END

    // Step 2: Compute pseudoinverse of Y
    Y_pinv = pseudoinverse(Y)   // size [Q x N_SH]

    // Step 3: Compute aliasing frequency
    f_a = order * 343.0 / (2.0 * PI * 0.0875)
    bin_a = round(f_a / sampleRate * N_FFT)

    // Step 4: Initialize output filter arrays
    H_decode_left  = zeros(N_SH, N_bins)   // complex
    H_decode_right = zeros(N_SH, N_bins)   // complex

    // Step 5: Below aliasing frequency -- standard least-squares
    FOR k = 0 TO bin_a:
        h_left_k  = hrtf_left[:, k]    // Q complex values
        h_right_k = hrtf_right[:, k]

        H_decode_left[:, k]  = Y_pinv * h_left_k    // error: these are wrong way
        // Actually: h_nm = Y^+ * h  where Y^+ is [N_SH x Q], h is [Q x 1]
        // So h_nm is [N_SH x 1]
        H_decode_left[:, k]  = pinv(Y) * h_left_k
        H_decode_right[:, k] = pinv(Y) * h_right_k
    END

    // Step 6: Above aliasing frequency -- magnitude-only matching (iterative)
    FOR k = bin_a + 1 TO N_bins - 1:
        // Left ear
        mag_left    = abs(hrtf_left[:, k])              // [Q x 1] magnitudes
        phase_left  = angle(transpose(Y) * H_decode_left[:, k-1])  // [Q x 1] reconstructed phase
        h_mod_left  = mag_left .* exp(j * phase_left)   // [Q x 1] modified HRTF
        H_decode_left[:, k] = pinv(Y) * h_mod_left

        // Right ear
        mag_right   = abs(hrtf_right[:, k])
        phase_right = angle(transpose(Y) * H_decode_right[:, k-1])
        h_mod_right = mag_right .* exp(j * phase_right)
        H_decode_right[:, k] = pinv(Y) * h_mod_right
    END

    // Step 7: Apply smooth crossover transition (half-octave)
    // (optional refinement -- blend LS and MagLS around f_a)

    // Step 8: Convert to time domain
    FOR n = 0 TO N_SH - 1:
        filter_left[n]  = IFFT(H_decode_left[n, :])
        filter_right[n] = IFFT(H_decode_right[n, :])
    END

    RETURN filter_left[N_SH], filter_right[N_SH]
END
```

### 2.5 Phase Continuation Strategies

The iterative phase reconstruction in step 6 above is the simplest approach but can suffer from high-frequency phase dispersion (the phase drifts unpredictably at very high frequencies). Three mitigation strategies exist:

1. **Average group delay continuation (`hf_cont='avg'`):** After MagLS processing, estimate the average group delay from the low-frequency region and apply it uniformly above f_a. This prevents phase drift and preserves a natural-sounding transient response.

2. **Per-direction delay continuation (`hf_cont='angle'`):** Estimate the group delay independently for each measurement direction, preserving angle-dependent timing that contributes to ILD cues. More complex but potentially more accurate.

3. **Manual delay specification (`hf_delay`):** Directly specify the high-frequency group delay in samples for each ear. Useful for fine-tuning with specific HRTF datasets.

The `spaudiopy` library implements all three strategies in its `magls_bin()` function.

### 2.6 BiMagLS (Bilateral MagLS)

BiMagLS extends MagLS by processing the left and right ears independently using bilateral ambisonics -- a representation where separate SH expansions describe the sound field at each ear position. This provides better ILD reproduction at the cost of doubling the SH channel count. For a JUCE plugin, standard MagLS is recommended as the primary approach, with BiMagLS as an optional high-quality mode.

---

## 3. HRTF Data Sources

### 3.1 The SOFA File Format

SOFA (Spatially Oriented Format for Acoustics), standardized as AES69-2015 (reaffirmed AES69-2022), is the de facto standard for exchanging HRTF data. It is built on the NetCDF4/HDF5 container format.

**What a SOFA file contains:**

```
SOFA File Structure (SimpleFreeFieldHRIR convention):
|
|-- Global Attributes
|   |-- SOFAConventions = "SimpleFreeFieldHRIR"
|   |-- DataType = "FIR"
|   |-- RoomType = "free field"
|   |-- ListenerShortName = "Subject_003"
|   |-- DatabaseName = "CIPIC"
|
|-- Dimensions
|   |-- M = number of measurement positions (e.g., 1250)
|   |-- R = number of receivers (2 for binaural: left ear, right ear)
|   |-- E = number of emitters (1 for single source)
|   |-- N = number of data samples per measurement (HRIR length, e.g., 200)
|   |-- C = coordinate dimensions (3: x, y, z or az, el, r)
|   |-- I = singleton dimension (1)
|
|-- Variables
|   |-- SourcePosition [M x C]:  (azimuth, elevation, radius) for each measurement
|   |-- ListenerPosition [I x C]: listener position (usually origin)
|   |-- ReceiverPosition [R x C]: ear positions relative to head center
|   |-- Data.IR [M x R x N]: the actual HRIR data (M positions, 2 ears, N taps)
|   |-- Data.SamplingRate [I]: sample rate in Hz (e.g., 48000)
|   |-- Data.Delay [M x R]: onset delay in samples for each measurement
```

**Key fields for binaural decoding:**
- `SourcePosition [M x 3]`: The directions (azimuth, elevation, distance) at which HRIRs were measured
- `Data.IR [M x 2 x N]`: The actual impulse responses -- M measurements, 2 ears, N samples long
- `Data.SamplingRate`: Sample rate of the HRIR data
- `Data.Delay [M x 2]`: Per-measurement onset delays (important for ITD handling)

### 3.2 Loading SOFA Files in C++

**libmysofa** is the standard lightweight C library for loading SOFA files. It has minimal dependencies and is suitable for plugin use.

```cpp
#include <mysofa.h>

// Loading an HRTF from a SOFA file
int filterLength = 0;
int err = MYSOFA_OK;
struct MYSOFA_EASY* hrtf = mysofa_open("hrtf.sofa", 48000.0f, &filterLength, &err);

if (err != MYSOFA_OK || hrtf == nullptr) {
    // Handle error
    return;
}

// Retrieve HRIR for a specific direction
float x = 1.0f, y = 0.0f, z = 0.0f;  // Cartesian: front
float irLeft[filterLength];
float irRight[filterLength];
float delayLeft, delayRight;

mysofa_getfilter_float(hrtf, x, y, z,
                       irLeft, irRight,
                       &delayLeft, &delayRight);

// irLeft and irRight now contain the HRIR for the nearest measured direction
// delayLeft and delayRight contain the interaural delay in samples

// Cleanup
mysofa_close(hrtf);
```

**Integration with CMake:**
```cmake
# Option 1: System-installed libmysofa
find_package(mysofa REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE mysofa::mysofa)

# Option 2: Bundled as subdirectory
add_subdirectory(libs/libmysofa)
target_link_libraries(${PROJECT_NAME} PRIVATE mysofa)
```

### 3.3 Recommended Free HRTF Datasets

| Dataset | Subjects | Directions | HRIR Length | Sample Rate | Format | License | Best For |
|---------|----------|------------|-------------|-------------|--------|---------|----------|
| **SADIE II** | 20 | 2114-8802 | 256-512 | 96 kHz | SOFA, WAV | CC-BY | Ambisonics decoding (includes Lebedev grid measurements) |
| **TH Cologne (Bernschutz KU100)** | 1 (KU100) | 2354, 2702 | 256 | 48 kHz | SOFA | Free | High-resolution reference, MagLS filter computation |
| **CIPIC** | 45 | 1250 | 200 | 44.1 kHz | SOFA, MAT | Public domain | Individualized HRTF research, large subject pool |
| **MIT KEMAR** | 1 (KEMAR) | 710 | 512 | 44.1 kHz | SOFA, WAV | Free | Reference dummy head, widely cited |
| **LISTEN (IRCAM)** | 51 | 187 | 512 | 44.1 kHz | SOFA | Free | Good subject diversity |
| **HUTUBS** | 96 | 440 | 256 | 44.1 kHz | SOFA | CC-BY | Large subject pool, 3D scans included |
| **ARI (Austrian Academy)** | 200+ | 1550 | 256 | 48 kHz | SOFA | Free | Largest subject pool |

**Recommendations for this project:**

1. **Primary default HRTF: TH Cologne KU100 (Bernschutz)** -- 2702-direction Lebedev grid measurement of the Neumann KU100 dummy head at 48 kHz. The high spatial resolution is ideal for computing MagLS decoding filters. Available at [Zenodo](https://zenodo.org/records/3928297).

2. **SADIE II** -- Provides HRIRs measured specifically at Lebedev grid positions suitable for ambisonics decoding (6, 14, 26, 50, 86 node grids). Available at [University of York](https://www.york.ac.uk/sadie-project/database.html).

3. **Bundled fallback: MIT KEMAR** -- Small file size, public domain, suitable as a built-in default when no user HRTF is loaded.

### 3.4 HRTF Spatial Resolution Requirements

For computing MagLS (or standard LS) binaural decoding filters, the HRTF dataset must have sufficient spatial resolution to accurately represent the SH expansion up to the desired order. The minimum requirements are:

```
Minimum HRTF measurement directions >= (N+1)^2

For HOA3: >= 16 directions (absolute minimum)
Recommended: >= 50 directions (well-overdetermined SH decomposition)
Ideal: >= 200 directions (smooth interpolation, low condition number)
```

The TH Cologne dataset (2702 directions) and SADIE II (2114+ directions) far exceed these requirements, allowing accurate SH decomposition up to very high orders.

For the CIPIC dataset (1250 directions), the irregular measurement grid means that the SH decomposition may have a higher condition number. Using Tikhonov regularization or spherical cap-aware sampling is recommended:

```
h_nm = (Y^H * Y + lambda * I)^(-1) * Y^H * h

where lambda is a regularization parameter (typical: 1e-4 to 1e-2)
```

### 3.5 Precomputing Binaural Decoding Filters from HRTF + Ambisonics Order

The complete preprocessing pipeline to generate runtime-ready binaural decoding filters:

```
INPUT:
  - SOFA file with Q measured HRIRs at directions (az_q, el_q)
  - Target ambisonics order N (e.g., 3)
  - Target sample rate fs
  - Target filter length L_filt

PROCESS:

  1. Load HRTF data from SOFA file
     hrir_left[Q][N_hrir], hrir_right[Q][N_hrir] = load_sofa("hrtf.sofa")
     Resample to target fs if necessary

  2. Zero-pad HRIRs to FFT length
     N_FFT = next_power_of_2(max(N_hrir, L_filt) * 2)
     HRTF_left[Q][N_FFT]  = FFT(zero_pad(hrir_left, N_FFT))
     HRTF_right[Q][N_FFT] = FFT(zero_pad(hrir_right, N_FFT))

  3. Build SH matrix at HRTF measurement positions
     Y[(N+1)^2][Q] = spherical_harmonics(N, az[:], el[:])

  4. Run MagLS algorithm (Section 2.4)
     H_decode_left[N_SH][N_bins], H_decode_right[N_SH][N_bins] =
         computeMagLSFilters(HRTF_left, HRTF_right, dirs, N, fs)

  5. Convert to time domain and truncate
     FOR n = 0 TO N_SH - 1:
         decode_ir_left[n]  = IFFT(H_decode_left[n])[:L_filt]
         decode_ir_right[n] = IFFT(H_decode_right[n])[:L_filt]
     END

OUTPUT:
  - decode_ir_left[16][L_filt]:  16 left-ear decoding FIR filters
  - decode_ir_right[16][L_filt]: 16 right-ear decoding FIR filters
  - Total: 32 FIR filters, each L_filt samples long
```

Typical filter lengths:
- 128 taps at 48 kHz (2.67 ms) -- minimum, some HF detail lost
- 256 taps at 48 kHz (5.33 ms) -- good quality/latency tradeoff
- 512 taps at 48 kHz (10.67 ms) -- high quality, noticeable latency

---

## 4. Convolution Implementation

### 4.1 Uniformly Partitioned Overlap-Save (UPOLS)

UPOLS is the standard algorithm for real-time FIR convolution with predictable, constant CPU load per audio block. It is the recommended approach for HRTF convolution in a JUCE plugin.

**Core concept:** The impulse response (decoding filter) is split into partitions of size B (the audio block size). Each partition is pre-transformed to the frequency domain. At runtime, the input is FFT-transformed once per block, and each partition is applied via frequency-domain multiplication. A delay line accumulates partial results.

### 4.2 Algorithm Detail

```
PREPROCESSING (done once at init / when filters change):

  Given:
    B = audio block size (e.g., 256 samples)
    h[L] = FIR filter (decoding filter), length L samples
    K = ceil(L / B) = number of partitions

  1. Partition the filter:
     h_0 = h[0..B-1]
     h_1 = h[B..2B-1]
     ...
     h_{K-1} = h[(K-1)*B .. L-1]   (zero-pad last partition if needed)

  2. Zero-pad each partition to length 2B:
     h_k_padded = [h_k, zeros(B)]   // length 2B

  3. FFT each partition:
     H_k = FFT(h_k_padded)          // length 2B complex spectrum
     Store H_0, H_1, ..., H_{K-1}


RUNTIME PROCESSING (per audio block):

  Input: x_new[B] = new input samples (one audio block)

  1. INPUT PACKING: Concatenate with previous block
     x_buf = [x_prev, x_new]    // length 2B
     x_prev = x_new             // save for next iteration

  2. FFT the input buffer:
     X = FFT(x_buf)             // length 2B complex

  3. FREQUENCY DOMAIN DELAY LINE (FDL):
     Push X into position 0 of the FDL
     Shift existing entries: FDL[k] = FDL[k-1] for k = K-1 down to 1
     FDL[0] = X

  4. ACCUMULATE: Complex multiply and sum
     Y = zeros(2B, complex)
     FOR k = 0 TO K-1:
         Y += FDL[k] .* H_k     // element-wise complex multiply
     END

  5. IFFT:
     y_buf = IFFT(Y)            // length 2B real

  6. OUTPUT PACKING: Take last B samples
     output = y_buf[B .. 2B-1]  // discard first B samples (overlap-save)

  RETURN output[B]
```

### 4.3 Applying UPOLS to 16-Channel Ambisonics Binaural Decoding

For HOA3 to binaural, we need 32 parallel convolutions (16 ambisonics channels x 2 ears). The key optimization is that **all 16 channels share the same audio block**, so some work can be shared.

```
Per audio block processing:

  INPUT: ambi_in[16][B]  (16 ambisonics channels, B samples each)

  1. FFT each input channel (16 FFTs):
     FOR n = 0 TO 15:
         x_buf_n = [x_prev_n, ambi_in[n]]    // 2B samples
         X_n = FFT(x_buf_n)                    // 2B complex
         Push X_n into FDL_n
     END

  2. Accumulate binaural output (32 multiply-accumulate operations per partition):
     Y_left  = zeros(2B, complex)
     Y_right = zeros(2B, complex)

     FOR k = 0 TO K-1:
         FOR n = 0 TO 15:
             Y_left  += FDL_n[k] .* H_left_n_k
             Y_right += FDL_n[k] .* H_right_n_k
         END
     END

  3. IFFT and output (2 IFFTs):
     out_left  = IFFT(Y_left)[B .. 2B-1]
     out_right = IFFT(Y_right)[B .. 2B-1]

  OUTPUT: stereo_out[2][B] = {out_left, out_right}
```

**Computational cost per audio block:**

```
FFTs:     16 (input) + 2 (output) = 18 FFTs of size 2B
Multiplies: 16 channels * 2 ears * K partitions = 32K complex multiplications of length 2B

For B = 256, K = 2 (512-tap filter):
  FFT size = 512
  18 FFTs of size 512
  64 complex multiplications of 512 elements

For B = 256 at 48 kHz: ~5.3 ms per block
  Total: ~18 * 512 * log2(512) * 5 FLOPS + 64 * 512 * 6 FLOPS
       = ~18 * 512 * 9 * 5 + 64 * 512 * 6
       = 414,720 + 196,608 = ~611,000 FLOPS per block
       = ~611,000 / 0.0053 = ~115 MFLOPS

This is well within the capacity of a single modern CPU core (capable of ~10-50 GFLOPS).
```

### 4.4 FFT Size Selection

The FFT size for UPOLS must be exactly 2 * blockSize:

| Audio Block Size B | FFT Size 2B | Partitions for 256-tap filter | Partitions for 512-tap filter |
|-------------------|-------------|------------------------------|------------------------------|
| 64 | 128 | 4 | 8 |
| 128 | 256 | 2 | 4 |
| 256 | 512 | 1 | 2 |
| 512 | 1024 | 1 | 1 |
| 1024 | 2048 | 1 | 1 |

**Observations:**
- Larger block sizes need fewer partitions and fewer FFTs, but add latency
- For a 256-tap filter at B=256, only K=1 partition is needed (simplest case)
- For a 512-tap filter at B=128, K=4 partitions are needed

### 4.5 Optimization: Channel Energy Thresholding

Not all 16 ambisonics channels contribute equally to the binaural output. Higher-order channels (order 3, channels 9-15) contribute less energy for diffuse-ish content. An optimization:

```cpp
// Skip convolution for channels whose decoding filter has negligible energy
for (int n = 0; n < 16; ++n)
{
    float filterEnergy = computeRMS(decodeFilter[n]);
    if (filterEnergy < threshold)  // e.g., threshold = -60 dB
        continue;  // skip this channel's convolution

    // ... perform convolution for channel n
}
```

In practice, for well-designed MagLS filters, all 16 channels have meaningful energy and this optimization provides minimal savings. It is more useful for higher orders (5th+) where many high-order channels become negligible.

### 4.6 JUCE dsp::Convolution vs Manual FFT Implementation

**JUCE `dsp::Convolution` (built-in):**

Pros:
- Zero-latency, fixed-latency, and non-uniform partitioned modes
- Thread-safe impulse response loading (wait-free on audio thread)
- Handles resampling and trimming automatically
- Well-tested, production-ready

Cons:
- Designed for 1-in-1-out or stereo convolution, not 16-in-2-out
- Would need 32 separate `dsp::Convolution` instances (one per SH channel per ear)
- Each instance does its own FFT of its input -- no sharing of input FFTs across channels
- Each instance maintains independent state, no batch optimization
- Memory overhead: 32 instances each with their own FFT buffers and delay lines
- The `process()` method operates on `juce::dsp::ProcessContext` which is designed for matched I/O

**Manual FFT implementation using `juce::dsp::FFT`:**

Pros:
- Share input FFTs across all 32 convolutions (save 14 FFTs per block)
- Single accumulation buffer per ear (memory efficient)
- Full control over partition count, latency, filter updates
- Can batch-optimize the multiply-accumulate loop with SIMD
- Lower total memory: shared FDL for all channels

Cons:
- More code to write and maintain
- Must handle thread safety for filter swapping manually
- Must implement overlap-save bookkeeping

**Recommendation: Manual implementation using `juce::dsp::FFT`.**

The 16-to-2 channel topology of ambisonics-to-binaural decoding does not map cleanly onto `dsp::Convolution`'s architecture. The shared-input-FFT optimization alone saves ~44% of the FFT cost. A manual UPOLS implementation using `juce::dsp::FFT` for the FFT/IFFT operations gives the best performance and most predictable behavior.

```cpp
// JUCE FFT usage for manual UPOLS
juce::dsp::FFT fft(/* order = */ std::log2(2 * blockSize));  // e.g., order=9 for 512-point FFT

// Forward FFT (real input -> complex output)
// Input: 2B real samples in fftBuffer (interleaved as [re, im, re, im, ...])
// juce::dsp::FFT expects interleaved complex, so pack real data appropriately
fft.performRealOnlyForwardTransform(fftBuffer.data());

// Inverse FFT
fft.performRealOnlyInverseTransform(fftBuffer.data());
```

---

## 5. Head Rotation Support

### 5.1 Rotating the Ambisonics Soundfield

The principal advantage of the ambisonics intermediate approach (vs. per-grain HRTF convolution) is that **head rotation is trivially applied by rotating the ambisonics soundfield** before decoding. When the listener turns their head to the right by angle alpha, we counter-rotate the soundfield to the left by alpha, maintaining a stable spatial scene relative to the external world.

This is done by multiplying the ambisonics signal vector by a rotation matrix **before** the binaural decoding stage:

```
Pipeline with head rotation:

  Grain synthesis -> HOA3 encoding (16 ch) -> Rotation matrix R -> Binaural decode -> Stereo out
```

### 5.2 Rotation Matrix Structure (Block-Diagonal)

The rotation matrix for spherical harmonics is **block-diagonal**: SH components of order n only mix with other components of the same order n. For HOA3 (orders 0 through 3), the rotation matrix has the structure:

```
R = blkdiag(R_0, R_1, R_2, R_3)

where:
  R_0 = [1]                     (1x1, order 0 is rotation-invariant)
  R_1 = 3x3 matrix              (order 1, same as Cartesian rotation matrix)
  R_2 = 5x5 matrix              (order 2)
  R_3 = 7x7 matrix              (order 3)

Total: 16x16 block-diagonal matrix
```

The full rotation matrix R is 16x16 but is extremely sparse due to the block-diagonal structure. Only 1 + 9 + 25 + 49 = 84 elements are potentially nonzero (out of 256 total).

### 5.3 Computing Rotation Sub-Matrices per Order

For a 3D rotation defined by yaw (alpha), pitch (beta), and roll (gamma), the rotation sub-matrices can be computed using the decomposition:

```
R(alpha, beta, gamma) = Rz(alpha) * Ry(beta) * Rz(gamma)

where Rz and Ry are z-axis and y-axis rotations in the SH domain.
```

**Z-axis rotation (yaw) for SH order n:**

Z-axis rotation is the simplest because it maps directly to the azimuthal component of spherical harmonics. For each order n, the 2n+1 x 2n+1 sub-matrix is:

```
For order n, the z-rotation by angle alpha transforms:
  Y_n^{-m} and Y_n^{m} via 2x2 rotation blocks:

  [Y_n^{-m}']   [cos(m*alpha)  -sin(m*alpha)] [Y_n^{-m}]
  [Y_n^{ m}'] = [sin(m*alpha)   cos(m*alpha)] [Y_n^{ m}]

  Y_n^0 is unchanged.
```

**Y-axis rotation (pitch) for SH order n:**

Y-axis rotation is more complex. The efficient approach from Zotter's thesis uses the decomposition:

```
Ry(beta) = Rz(-90) * Ry90 * Rz(beta + 180) * Ry90^(-1) * Rz(90)
```

Where `Ry90` is the SH rotation matrix for a fixed 90-degree y-axis rotation, which can be **precomputed** and stored as a constant table. The IEM ambisonics library provides precomputed `Ry90` matrices up to order 21.

**Full rotation for order n:**

```
R_n(alpha, beta, gamma) = Rz_n(alpha + 90) * Ry90_n * Rz_n(beta + 180) * Ry90_n^T * Rz_n(gamma + 90)
```

This requires only:
- 3 z-axis rotations (cheap: just sin/cos computations)
- 2 multiplications by precomputed Ry90 matrices
- 2 matrix multiplications of size (2n+1) x (2n+1)

### 5.4 Efficient C++ Implementation

```cpp
class AmbisonicRotator
{
public:
    static constexpr int ORDER = 3;
    static constexpr int NUM_CHANNELS = (ORDER + 1) * (ORDER + 1);  // 16

    void setRotation(float yawRadians, float pitchRadians, float rollRadians)
    {
        // Build rotation sub-matrices for each order
        // Order 0: identity (1x1)
        rotationMatrix[0][0] = 1.0f;

        // Order 1: standard 3D rotation matrix (3x3)
        // Using ZYZ Euler angle convention
        buildOrder1Rotation(yawRadians, pitchRadians, rollRadians);

        // Orders 2 and 3: use recurrence relations or Ry90 decomposition
        buildHigherOrderRotation(2, yawRadians, pitchRadians, rollRadians);
        buildHigherOrderRotation(3, yawRadians, pitchRadians, rollRadians);
    }

    void process(const float* input, float* output)
    {
        // Apply block-diagonal rotation
        // Order 0 (channel 0)
        output[0] = input[0];

        // Order 1 (channels 1-3)
        applySubMatrix(input + 1, output + 1, 1);  // 3x3

        // Order 2 (channels 4-8)
        applySubMatrix(input + 4, output + 4, 2);  // 5x5

        // Order 3 (channels 9-15)
        applySubMatrix(input + 9, output + 9, 3);  // 7x7
    }

private:
    // Storage for per-order rotation sub-matrices
    // Order n has (2n+1) x (2n+1) elements
    float order1[3][3];
    float order2[5][5];
    float order3[7][7];

    // Full 16x16 matrix (mostly zeros, block-diagonal)
    float rotationMatrix[NUM_CHANNELS][NUM_CHANNELS] = {};

    void buildOrder1Rotation(float yaw, float pitch, float roll)
    {
        // Order 1 SH rotation = standard rotation matrix
        // BUT with SN3D/ACN ordering: channels are Y(1,-1), Y(1,0), Y(1,1)
        // which correspond to y, z, x axes respectively
        // So the rotation matrix maps (y,z,x) -> (y',z',x')

        float cy = std::cos(yaw),   sy = std::sin(yaw);
        float cp = std::cos(pitch), sp = std::sin(pitch);
        float cr = std::cos(roll),  sr = std::sin(roll);

        // Full rotation matrix in (x,y,z) coordinates:
        // Rz(yaw) * Ry(pitch) * Rx(roll)
        // Then permuted to ACN order (y, z, x)

        // R_xyz[0][0] = cy*cp
        // R_xyz[0][1] = cy*sp*sr - sy*cr
        // R_xyz[0][2] = cy*sp*cr + sy*sr
        // R_xyz[1][0] = sy*cp
        // R_xyz[1][1] = sy*sp*sr + cy*cr
        // R_xyz[1][2] = sy*sp*cr - cy*sr
        // R_xyz[2][0] = -sp
        // R_xyz[2][1] = cp*sr
        // R_xyz[2][2] = cp*cr

        // Permute to ACN order (Y=y, Z=z, X=x) -> indices: y=1, z=2, x=0
        // ACN mapping: acn1=Y, acn2=Z, acn3=X
        // So sub-matrix row/col order is (y, z, x):

        order1[0][0] =  sy*sp*sr + cy*cr;   // y->y
        order1[0][1] =  cp*sr;              // z->y
        order1[0][2] =  cy*sp*sr - sy*cr;   // x->y
        order1[1][0] =  sy*sp*cr - cy*sr;   // y->z
        order1[1][1] =  cp*cr;              // z->z
        order1[1][2] =  cy*sp*cr + sy*sr;   // x->z
        order1[2][0] =  sy*cp;              // y->x
        order1[2][1] = -sp;                 // z->x
        order1[2][2] =  cy*cp;              // x->x
    }

    void applySubMatrix(const float* in, float* out, int order)
    {
        int size = 2 * order + 1;
        float* matrix = (order == 1) ? &order1[0][0] :
                        (order == 2) ? &order2[0][0] : &order3[0][0];

        for (int i = 0; i < size; ++i)
        {
            float sum = 0.0f;
            for (int j = 0; j < size; ++j)
                sum += matrix[i * size + j] * in[j];
            out[i] = sum;
        }
    }

    void buildHigherOrderRotation(int order, float yaw, float pitch, float roll)
    {
        // For orders 2 and 3, use the recurrence relation approach
        // or the precomputed Ry90 decomposition
        // See: Ivanic & Ruedenberg (1996) "Rotation Matrices for Real
        //      Spherical Harmonics. Direct Determination by Recursion"
        // Implementation detail omitted for brevity -- use SAF library
        // function saf_sh_rotation() or implement recurrence
    }
};
```

**Note on higher-order rotation:** Computing rotation sub-matrices for orders 2+ is mathematically involved. The recommended approach for production code is to use the **Spatial Audio Framework (SAF)** library's `rotateAxisCoeffs()` function, or the recursion method from Ivanic and Ruedenberg (1996). The IEM plugin suite also provides open-source implementations of SH rotation.

### 5.5 Real-Time Head Tracking Integration

Head trackers communicate rotation data via OSC (Open Sound Control) messages. The standard protocol formats are:

```
OSC Message Formats:

/yaw    float    (yaw angle in degrees, -180 to 180)
/pitch  float    (pitch angle in degrees, -90 to 90)
/roll   float    (roll angle in degrees, -180 to 180)

OR packed:
/ypr    float float float    (yaw, pitch, roll in one message)

OR quaternion:
/quaternion  float float float float   (qw, qx, qy, qz)
```

**JUCE OSC integration:**

```cpp
#include <juce_osc/juce_osc.h>

class HeadTracker : private juce::OSCReceiver,
                    private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    HeadTracker()
    {
        addListener(this);
        connect(9000);  // Listen on UDP port 9000
    }

    void oscMessageReceived(const juce::OSCMessage& message) override
    {
        if (message.getAddressPattern() == "/ypr" && message.size() == 3)
        {
            float yaw   = message[0].getFloat32();
            float pitch = message[1].getFloat32();
            float roll  = message[2].getFloat32();

            // Convert to radians and store atomically
            yawRadians.store(juce::degreesToRadians(yaw));
            pitchRadians.store(juce::degreesToRadians(pitch));
            rollRadians.store(juce::degreesToRadians(roll));
        }
        else if (message.getAddressPattern() == "/quaternion" && message.size() == 4)
        {
            float qw = message[0].getFloat32();
            float qx = message[1].getFloat32();
            float qy = message[2].getFloat32();
            float qz = message[3].getFloat32();
            quaternionToEuler(qw, qx, qy, qz);
        }
    }

    float getYaw()   const { return yawRadians.load(); }
    float getPitch() const { return pitchRadians.load(); }
    float getRoll()  const { return rollRadians.load(); }

private:
    std::atomic<float> yawRadians{0.0f};
    std::atomic<float> pitchRadians{0.0f};
    std::atomic<float> rollRadians{0.0f};

    void quaternionToEuler(float qw, float qx, float qy, float qz)
    {
        // Standard quaternion to Euler conversion
        float sinp = 2.0f * (qw * qy - qz * qx);
        float pitch = std::abs(sinp) >= 1.0f ?
            std::copysign(juce::MathConstants<float>::halfPi, sinp) : std::asin(sinp);

        float siny_cosp = 2.0f * (qw * qz + qx * qy);
        float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
        float yaw = std::atan2(siny_cosp, cosy_cosp);

        float sinr_cosp = 2.0f * (qw * qx + qy * qz);
        float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
        float roll = std::atan2(sinr_cosp, cosr_cosp);

        yawRadians.store(yaw);
        pitchRadians.store(pitch);
        rollRadians.store(roll);
    }
};
```

### 5.6 Smoothing Head Rotation Updates

Head tracking data typically arrives at 50-200 Hz, while audio processing runs at ~188 Hz (48000/256). To prevent audible artifacts from discontinuous rotation changes, apply exponential smoothing:

```cpp
// In processBlock, before applying rotation:
float smoothingCoeff = 0.1f;  // Adjust for responsiveness vs smoothness
currentYaw   += smoothingCoeff * (targetYaw   - currentYaw);
currentPitch += smoothingCoeff * (targetPitch - currentPitch);
currentRoll  += smoothingCoeff * (targetRoll  - currentRoll);

rotator.setRotation(currentYaw, currentPitch, currentRoll);
```

For the ambisonics rotation to work correctly, the rotation must be applied **sample-by-sample or block-by-block** to the HOA3 signal **before** the binaural decoding convolution. Since the binaural decoding filters are static (precomputed from HRTF), rotating the ambisonics input is equivalent to the listener turning their head.

---

## 6. Practical Implementation in JUCE

### 6.1 Architecture Overview

```
processBlock() flow:

  1. GRAIN SYNTHESIS
     Input audio -> grain engine -> per-grain HOA3 encoding -> accumulate to HOA3 buffer
     Result: ambiBuffer[16][blockSize]

  2. HEAD ROTATION (optional)
     Read head tracker angles (atomic)
     Apply rotation matrix R to ambiBuffer
     ambiBuffer = R * ambiBuffer

  3. BINAURAL DECODE (UPOLS convolution)
     Convolve 16 ambisonics channels with 32 precomputed decoding filters
     Result: stereoOut[2][blockSize]

  4. OUTPUT
     Copy stereoOut to output buffer
```

### 6.2 Buffer Management

```cpp
class BinauralAmbisonicsDecoder
{
public:
    static constexpr int AMBI_ORDER = 3;
    static constexpr int NUM_AMBI_CHANNELS = (AMBI_ORDER + 1) * (AMBI_ORDER + 1);  // 16
    static constexpr int NUM_EARS = 2;
    static constexpr int NUM_FILTERS = NUM_AMBI_CHANNELS * NUM_EARS;  // 32

private:
    // Intermediate HOA3 buffer: 16 channels x blockSize samples
    juce::AudioBuffer<float> ambiBuffer;

    // Stereo output buffer: 2 channels x blockSize samples
    juce::AudioBuffer<float> stereoBuffer;

    // Convolution state per filter
    struct ConvolutionState
    {
        std::vector<float> prevInputBlock;       // B samples, previous input block
        std::vector<std::complex<float>> fdl;    // Frequency domain delay line
        std::vector<std::vector<std::complex<float>>> filterPartitions;  // K partitions, each 2B complex
    };

    // 16 input channels, each with its own input history
    std::array<std::vector<float>, NUM_AMBI_CHANNELS> prevInputBlocks;
    std::array<std::vector<std::complex<float>>, NUM_AMBI_CHANNELS> inputSpectra;

    // 32 filter spectra (16 channels x 2 ears)
    std::array<std::vector<std::complex<float>>, NUM_FILTERS> filterSpectraLeft;   // [16][fftSize]
    std::array<std::vector<std::complex<float>>, NUM_FILTERS> filterSpectraRight;  // [16][fftSize]

    // Accumulation buffers for left and right ears
    std::vector<std::complex<float>> accumLeft;
    std::vector<std::complex<float>> accumRight;

    // FFT engine
    std::unique_ptr<juce::dsp::FFT> fft;
    int fftSize = 0;
    int blockSize = 0;
    int numPartitions = 0;

    // Rotator
    AmbisonicRotator rotator;

    // Head tracker
    HeadTracker headTracker;
};
```

### 6.3 prepareToPlay Implementation

```cpp
void BinauralAmbisonicsDecoder::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    blockSize = samplesPerBlock;
    fftSize = blockSize * 2;
    int fftOrder = static_cast<int>(std::log2(fftSize));

    // Initialize FFT engine
    fft = std::make_unique<juce::dsp::FFT>(fftOrder);

    // Allocate intermediate buffers
    ambiBuffer.setSize(NUM_AMBI_CHANNELS, blockSize);
    stereoBuffer.setSize(2, blockSize);

    // Allocate convolution state
    for (int n = 0; n < NUM_AMBI_CHANNELS; ++n)
    {
        prevInputBlocks[n].resize(blockSize, 0.0f);
        inputSpectra[n].resize(fftSize, {0.0f, 0.0f});
    }

    accumLeft.resize(fftSize, {0.0f, 0.0f});
    accumRight.resize(fftSize, {0.0f, 0.0f});

    // Load HRTF and compute MagLS decoding filters
    loadAndComputeDecodingFilters(sampleRate);
}

void BinauralAmbisonicsDecoder::loadAndComputeDecodingFilters(double sampleRate)
{
    // 1. Load HRTF from SOFA file (using libmysofa)
    int filterLength = 0;
    int err = MYSOFA_OK;
    auto* hrtf = mysofa_open(sofaFilePath.toRawUTF8(),
                             static_cast<float>(sampleRate),
                             &filterLength, &err);
    if (err != MYSOFA_OK) return;

    // 2. Extract HRIRs at measurement positions
    // (implementation depends on SOFA file structure)

    // 3. Run MagLS algorithm to compute 32 decoding filters
    // (see Section 2.4 pseudocode)

    // 4. FFT the decoding filters and store
    for (int n = 0; n < NUM_AMBI_CHANNELS; ++n)
    {
        // Zero-pad filter to fftSize, then FFT
        std::vector<float> paddedFilter(fftSize, 0.0f);
        std::copy(decodingFilterLeft[n].begin(),
                  decodingFilterLeft[n].end(),
                  paddedFilter.begin());
        fft->performRealOnlyForwardTransform(paddedFilter.data());

        // Store as complex spectrum
        // (juce::dsp::FFT stores interleaved real/imag)
        for (int k = 0; k < fftSize / 2 + 1; ++k)
        {
            filterSpectraLeft[n][k] = {paddedFilter[k * 2],
                                        paddedFilter[k * 2 + 1]};
        }

        // Repeat for right ear
        std::copy(decodingFilterRight[n].begin(),
                  decodingFilterRight[n].end(),
                  paddedFilter.begin());
        std::fill(paddedFilter.begin() + decodingFilterRight[n].size(),
                  paddedFilter.end(), 0.0f);
        fft->performRealOnlyForwardTransform(paddedFilter.data());

        for (int k = 0; k < fftSize / 2 + 1; ++k)
        {
            filterSpectraRight[n][k] = {paddedFilter[k * 2],
                                         paddedFilter[k * 2 + 1]};
        }
    }

    mysofa_close(hrtf);
}
```

### 6.4 processBlock Implementation

```cpp
void BinauralAmbisonicsDecoder::processBlock(juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();

    // ---- Step 1: Fill ambiBuffer from grain engine ----
    // (Grain engine writes directly into ambiBuffer during grain synthesis)
    // ambiBuffer[0..15][0..numSamples-1] contains the HOA3 signal

    // ---- Step 2: Apply head rotation (if head tracker is connected) ----
    if (headTrackingEnabled)
    {
        rotator.setRotation(headTracker.getYaw(),
                           headTracker.getPitch(),
                           headTracker.getRoll());

        // Apply rotation sample-by-sample (or per-block for efficiency)
        float rotatedSample[NUM_AMBI_CHANNELS];
        for (int s = 0; s < numSamples; ++s)
        {
            float inputSample[NUM_AMBI_CHANNELS];
            for (int n = 0; n < NUM_AMBI_CHANNELS; ++n)
                inputSample[n] = ambiBuffer.getSample(n, s);

            rotator.process(inputSample, rotatedSample);

            for (int n = 0; n < NUM_AMBI_CHANNELS; ++n)
                ambiBuffer.setSample(n, s, rotatedSample[n]);
        }
    }

    // ---- Step 3: UPOLS Binaural Convolution ----

    // Clear accumulation buffers
    std::fill(accumLeft.begin(), accumLeft.end(), std::complex<float>{0.0f, 0.0f});
    std::fill(accumRight.begin(), accumRight.end(), std::complex<float>{0.0f, 0.0f});

    for (int n = 0; n < NUM_AMBI_CHANNELS; ++n)
    {
        // 3a. Input packing: [prev_block | current_block] -> 2B samples
        std::vector<float> fftBuffer(fftSize * 2, 0.0f);  // interleaved real/imag for JUCE FFT
        for (int s = 0; s < blockSize; ++s)
        {
            fftBuffer[s * 2] = prevInputBlocks[n][s];                    // previous block
            fftBuffer[(s + blockSize) * 2] = ambiBuffer.getSample(n, s); // current block
        }

        // Save current block for next iteration
        for (int s = 0; s < blockSize; ++s)
            prevInputBlocks[n][s] = ambiBuffer.getSample(n, s);

        // 3b. Forward FFT
        fft->performRealOnlyForwardTransform(fftBuffer.data());

        // 3c. Complex multiply-accumulate with left and right ear filters
        for (int k = 0; k < fftSize; ++k)
        {
            std::complex<float> X(fftBuffer[k * 2], fftBuffer[k * 2 + 1]);

            accumLeft[k]  += X * filterSpectraLeft[n][k];
            accumRight[k] += X * filterSpectraRight[n][k];
        }
    }

    // 3d. Inverse FFT for left ear
    std::vector<float> outBufferLeft(fftSize * 2, 0.0f);
    for (int k = 0; k < fftSize; ++k)
    {
        outBufferLeft[k * 2]     = accumLeft[k].real();
        outBufferLeft[k * 2 + 1] = accumLeft[k].imag();
    }
    fft->performRealOnlyInverseTransform(outBufferLeft.data());

    // 3e. Inverse FFT for right ear
    std::vector<float> outBufferRight(fftSize * 2, 0.0f);
    for (int k = 0; k < fftSize; ++k)
    {
        outBufferRight[k * 2]     = accumRight[k].real();
        outBufferRight[k * 2 + 1] = accumRight[k].imag();
    }
    fft->performRealOnlyInverseTransform(outBufferRight.data());

    // 3f. Output packing: take last B samples (overlap-save)
    auto* leftOut  = buffer.getWritePointer(0);
    auto* rightOut = buffer.getWritePointer(1);
    for (int s = 0; s < numSamples; ++s)
    {
        leftOut[s]  = outBufferLeft[(s + blockSize) * 2];   // skip first B samples
        rightOut[s] = outBufferRight[(s + blockSize) * 2];
    }
}
```

### 6.5 Latency Reporting

The UPOLS convolution adds exactly `blockSize` samples of latency (due to the overlap-save buffering). This must be reported to the DAW:

```cpp
int getLatencySamples() const
{
    // Convolution latency = blockSize samples
    // Additional latency from filter partitioning if K > 1
    return blockSize;  // For K=1 (filter fits in one partition)
}
```

For multi-partition filters (K > 1), the latency is still `blockSize` samples because UPOLS processes all partitions within the current block using a delay line.

### 6.6 Memory Budget Estimation

For HOA3 binaural decoding at 48 kHz with 256-sample blocks and 256-tap filters:

```
Component                          | Size
-----------------------------------|------------------------
Ambisonics buffer (16 x 256)       | 16 KB
Stereo output buffer (2 x 256)     | 2 KB
Previous input blocks (16 x 256)   | 16 KB
Input FFT buffers (16 x 512 cplx)  | 64 KB
Filter spectra (32 x 512 complex)  | 128 KB
Accumulation buffers (2 x 512 cplx)| 8 KB
FFT engine internal state          | ~4 KB
Rotation matrix (16 x 16)          | 1 KB
-----------------------------------|------------------------
TOTAL                              | ~239 KB
```

This is negligible on modern systems. Even with 512-tap filters and multiple partitions, the total memory stays well under 1 MB.

### 6.7 Thread Safety for Filter Swapping

When the user loads a new HRTF or changes ambisonics order, the decoding filters must be recomputed. This should happen on a background thread, with the new filters swapped in atomically:

```cpp
// Use a double-buffer approach:
struct DecodingFilters
{
    std::array<std::vector<std::complex<float>>, 16> left;
    std::array<std::vector<std::complex<float>>, 16> right;
};

std::atomic<DecodingFilters*> activeFilters{nullptr};
std::unique_ptr<DecodingFilters> filterA, filterB;

// Background thread computes new filters into the inactive buffer
void onNewHRTFLoaded()
{
    auto* inactive = (activeFilters.load() == filterA.get()) ?
                     filterB.get() : filterA.get();

    computeMagLSFilters(inactive);  // heavy computation

    activeFilters.store(inactive);  // atomic swap
}

// Audio thread reads from active filters
void processBlock(...)
{
    auto* filters = activeFilters.load();
    // Use filters->left and filters->right for convolution
}
```

---

## 7. Alternatives to Full Convolution

### 7.1 Parametric HRTF Models (Parallel Biquad Sections)

Instead of storing full HRIR impulse responses, HRTFs can be modeled as a **bank of parallel second-order IIR (biquad) filter sections**:

```
H_HRTF(z) = G * sum_{k=1}^{P}  (b0_k + b1_k*z^-1 + b2_k*z^-2) / (1 + a1_k*z^-1 + a2_k*z^-2)

where P = number of parallel sections (typically 6-12 per ear per direction)
```

**Advantages:**
- **Much lighter CPU load:** P biquads per channel vs. full FIR convolution
- **Simple interpolation:** Smoothly interpolate biquad coefficients between directions
- **Tiny memory footprint:** ~60-120 coefficients per direction vs. 256-512 HRIR samples
- **No FFT required:** Time-domain IIR processing, no block latency
- **JUCE support:** `juce::dsp::IIR::Filter` provides ready-made biquad implementation

**Disadvantages:**
- **Reduced accuracy:** Parametric model approximates the HRTF, losing fine detail
- **HRTF fitting required:** Must fit biquad parameters to measured HRTFs (offline process)
- **Stability concerns:** IIR filters can become unstable if coefficients are interpolated carelessly

**Implementation sketch:**

```cpp
struct ParametricHRTF
{
    static constexpr int NUM_SECTIONS = 8;  // parallel biquad sections

    struct BiquadCoeffs
    {
        float b0, b1, b2, a1, a2;
    };

    // Per-direction: P biquad sections for each ear
    BiquadCoeffs sections[NUM_SECTIONS];
    float itdSamples;  // interaural time delay
};

class ParametricBinauralRenderer
{
    // For each ambisonics channel, store left/right parametric HRTFs
    std::array<ParametricHRTF, 16> leftEarFilters;
    std::array<ParametricHRTF, 16> rightEarFilters;

    // 16 channels * 2 ears * 8 sections = 256 biquad filter instances
    juce::dsp::IIR::Filter<float> biquads[16][2][8];

    void processBlock(const float ambiInput[16][B], float stereoOut[2][B])
    {
        std::fill_n(stereoOut[0], B, 0.0f);
        std::fill_n(stereoOut[1], B, 0.0f);

        for (int ch = 0; ch < 16; ++ch)
        {
            for (int ear = 0; ear < 2; ++ear)
            {
                for (int sec = 0; sec < 8; ++sec)
                {
                    // Process input through this biquad section
                    float sectionOut[B];
                    biquads[ch][ear][sec].process(ambiInput[ch], sectionOut, B);

                    // Accumulate to output
                    for (int s = 0; s < B; ++s)
                        stereoOut[ear][s] += sectionOut[s];
                }
            }
        }
    }
};
```

**CPU comparison:**
```
UPOLS convolution (256-tap filter, B=256):
  18 FFTs + 32 complex multiply-accumulate = ~115 MFLOPS

Parametric biquads (8 sections, 16 channels, 2 ears):
  256 biquads * B * 5 multiply-adds = 256 * 256 * 5 = 327,680 ops/block
  = ~327,680 / 0.0053s = ~62 MFLOPS

Parametric is ~1.9x cheaper than UPOLS for this configuration.
```

The parametric approach is viable as a **lightweight fallback mode** when CPU budget is constrained, trading accuracy for efficiency.

### 7.2 Neural Network Ambisonics-to-Binaural

Recent research (2022-2025) has explored deep learning approaches for ambisonics-to-binaural decoding:

**U-Net architecture (Lluvia et al., 2022):**
- Input: 1st-order ambisonics (4 channels) STFT
- Output: Binaural stereo STFT
- Training: Paired ambisonics-binaural dataset
- Loss: Combined time-domain and frequency-domain losses
- Results: Outperforms conventional methods on objective metrics

**Conv-TasNet for ambisonics super-resolution (2025):**
- Input: 1st-order ambisonics (4 channels), time domain
- Output: Higher-order ambisonics (e.g., 3rd order, 16 channels)
- Uses data-driven upsampling to enhance spatial resolution
- Reports 80% improvement in perceived quality over traditional methods

**ICLR 2025: "Both Ears Wide Open":**
- End-to-end neural binaural rendering
- Directly maps spatial audio to binaural output
- Achieves comparable subjective quality to conventional rendering

**Current status for plugin integration:** Neural approaches are promising but face practical barriers for real-time plugin use:
- Inference latency (even optimized models add several ms)
- Model size (10-100 MB for weights)
- Limited HRTF personalization (model is trained on specific HRTFs)
- Lack of production-ready C++ inference frameworks optimized for audio block sizes

**Recommendation:** Monitor this space but use conventional MagLS + UPOLS for production. Consider neural super-resolution as a future enhancement (upsampling 1st-order to 3rd-order ambisonics before conventional binaural decoding).

### 7.3 Basic Stereo Panning as Fallback

When binaural processing is too expensive (e.g., on very low-powered devices or when the user does not need binaural), a simple stereo panning fallback should be available:

```cpp
// Simple stereo panning from ambisonics (use only W, Y, and X channels)
void ambiToStereoSimple(const float ambiInput[16][B], float stereoOut[2][B], int B)
{
    const float* w = ambiInput[0];   // ACN0: omnidirectional
    const float* y = ambiInput[1];   // ACN1: left-right axis
    const float* x = ambiInput[3];   // ACN3: front-back axis

    for (int s = 0; s < B; ++s)
    {
        // Mid-side decode with front emphasis
        float mid  = w[s] + 0.5f * x[s];   // center + front emphasis
        float side = y[s] * 0.7f;           // left-right width

        stereoOut[0][s] = mid + side;  // left
        stereoOut[1][s] = mid - side;  // right
    }
}
```

This discards channels 2, 4-15 (elevation, higher-order spatial detail) but preserves the basic left-right spatial image with zero additional CPU cost. It serves as a fallback for stereo-only DAWs (e.g., Ableton Live) or when the user explicitly disables binaural rendering.

A more sophisticated stereo downmix can incorporate the Z (up-down) channel for a sense of elevation and higher-order channels for improved spatial impression:

```cpp
// Enhanced stereo downmix using more ambisonics channels
void ambiToStereoEnhanced(const float ambiInput[16][B], float stereoOut[2][B], int B)
{
    const float* w = ambiInput[0];   // W
    const float* y = ambiInput[1];   // Y (left-right)
    const float* z = ambiInput[2];   // Z (up-down)
    const float* x = ambiInput[3];   // X (front-back)

    for (int s = 0; s < B; ++s)
    {
        float mid  = w[s] * 0.7071f + x[s] * 0.5f;
        float side = y[s] * 0.5f;

        // Add slight Z contribution for "height presence"
        float height = z[s] * 0.15f;

        stereoOut[0][s] = mid + side + height;
        stereoOut[1][s] = mid - side + height;
    }
}
```

---

## 8. References

### Core Algorithms

1. Schoerkhuber, C., Zotter, F., & Hoeldrich, R. (2018). "Binaural Rendering of Ambisonic Signals via Magnitude Least Squares." *Proceedings of DAGA 2018*, Munich. [ResearchGate](https://www.researchgate.net/publication/325080691_Binaural_Rendering_of_Ambisonic_Signals_via_Magnitude_Least_Squares)

2. Hold, C., Gamper, H., Pulkki, V., Raghuvanshi, N., & Tashev, I. J. (2023). "Magnitude-Least-Squares Binaural Ambisonic Rendering with Phase Continuation." *Proceedings of DAGA 2023*. [DEGA](https://pub.dega-akustik.de/DAGA_2023/data/articles/000535.pdf)

3. Engel, I., Goodman, D., & Picinali, L. (2021). "Improving Binaural Rendering with Bilateral Ambisonics and MagLS." [ResearchGate](https://www.researchgate.net/publication/355773450_Improving_Binaural_Rendering_with_Bilateral_Ambisonics_and_MagLS)

4. Deppisch, T. et al. (2021). "End-to-End Magnitude Least Squares Binaural Rendering of Spherical Microphone Array Signals." [GitHub](https://github.com/thomasdeppisch/eMagLS)

5. Lluvia, M. et al. (2025). "Ambisonics Binaural Rendering via Masked Magnitude Least Squares." [arXiv:2501.18224](https://arxiv.org/abs/2501.18224)

### HRTF and Convolution

6. Wefers, F. (2015). "Partitioned convolution algorithms for real-time auralization." Ph.D. Dissertation, RWTH Aachen University. [RWTH](https://publications.rwth-aachen.de/record/466561/files/466561.pdf)

7. Cuevas-Rodriguez, M. et al. (2019). "3D Tune-In Toolkit: An open-source library for real-time binaural spatialisation." *PLOS ONE*. [PLOS ONE](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0211899)

8. Zaar, J. & Fernandez, J. (2022). "Assessing HRTF preprocessing methods for Ambisonics rendering through perceptual models." *Acta Acustica*. [Acta Acustica](https://acta-acustica.edpsciences.org/articles/aacus/full_html/2022/01/aacus210029/aacus210029.html)

### HRTF Databases

9. **SADIE II Database** -- University of York. [Download](https://www.york.ac.uk/sadie-project/database.html) | [Zenodo](https://zenodo.org/records/10886409)

10. **TH Cologne KU100 HRTFs** -- Bernschutz, B. "A Spherical Far Field HRIR/HRTF Compilation of the Neumann KU 100." [Zenodo](https://zenodo.org/records/3928297)

11. **CIPIC HRTF Database** -- UC Davis CIPIC Interface Lab. [UC Davis](https://www.ece.ucdavis.edu/cipic/)

12. **SOFA Convention (AES69)** -- [sofaconventions.org](https://www.sofaconventions.org/mediawiki/index.php/SOFA_(Spatially_Oriented_Format_for_Acoustics))

### Software and Libraries

13. **Spatial Audio Framework (SAF)** -- McCormack, L. ISC License (core) / GPLv2 (optional modules). [GitHub](https://github.com/leomccormack/Spatial_Audio_Framework)

14. **SPARTA Plugin Suite** -- McCormack, L. Binaural ambisonics decoder (AmbiBIN) with MagLS. [Website](https://leomccormack.github.io/sparta-site/) | [GitHub](https://github.com/leomccormack/SPARTA)

15. **libmysofa** -- Hoene, C. Lightweight SOFA reader in C. [GitHub](https://github.com/hoene/libmysofa)

16. **spaudiopy** -- Python spatial audio processing library with MagLS implementation. [Documentation](https://spaudiopy.readthedocs.io/en/latest/spaudiopy.decoder.html)

17. **libspatialaudio** -- Videolabs. C++ ambisonics encoding/decoding and binaural rendering. LGPLv2.1. [GitHub](https://github.com/videolabs/libspatialaudio)

18. **JUCE dsp::Convolution** -- JUCE Framework. [API Reference](https://docs.juce.com/master/classdsp_1_1Convolution.html)

### Spherical Harmonics and Rotation

19. Zotter, F. (2009). "Analysis and Synthesis of Sound-Radiation with Spherical Arrays." Ph.D. thesis, University of Music and Performing Arts Graz.

20. **IEM Spherical Harmonics Rotation** -- Precomputed Ry90 matrices. [IEM](https://ambisonics.iem.at/xchange/fileformat/docs/spherical-harmonics-rotation)

21. Ivanic, J. & Ruedenberg, K. (1996). "Rotation Matrices for Real Spherical Harmonics. Direct Determination by Recursion." *J. Phys. Chem.*, 100(15), 6342-6347.

22. Lecomte, P. (2015). "On the Use of a Lebedev Grid for Ambisonics." [ResearchGate](https://www.researchgate.net/publication/286379460_On_the_Use_of_a_Lebedev_Grid_for_Ambisonics)

### Parametric HRTF and Neural Approaches

23. Gutierrez-Parera, P. & Lopez, J. J. (2020). "An Efficient Implementation of Parallel Parametric HRTF Models for Binaural Sound Synthesis in Mobile Multimedia." *IEEE Trans. Multimedia*. [IEEE](https://ieeexplore.ieee.org/document/9028216/)

24. Bank, B. & Ramos, G. (2017). "A Parallel Approach to HRTF Approximation and Interpolation Based on a Parametric Filter Model." *IEEE Signal Processing Letters*. [IEEE](https://ieeexplore.ieee.org/abstract/document/8013136)

25. Lluvia, M. et al. (2022). "Binaural Rendering of Ambisonic Signals by Neural Networks." [arXiv:2211.02301](https://arxiv.org/abs/2211.02301)

### Head Tracking

26. **Hedrot** -- Open-source head tracker with OSC output. [Website](https://abaskind.github.io/hedrot/) | [GitHub](https://github.com/abaskind/hedrot)

27. **nvsonic Head Tracker** -- Low-cost Arduino-based 3DOF head tracker. [GitHub](https://github.com/trsonic/nvsonic-head-tracker)

---

## Appendix A: Quick Reference -- HOA3 Binaural Decoding Parameter Summary

| Parameter | Value |
|-----------|-------|
| Ambisonics order | 3 |
| SH channel count | 16 (ACN 0-15) |
| Normalization | SN3D (AmbiX) |
| Aliasing frequency (HOA3) | 1875 Hz |
| Recommended virtual speakers | 26 (Lebedev) or 50 (Lebedev) |
| Binaural decoding filters | 32 (16 channels x 2 ears) |
| Recommended filter length | 256 taps at 48 kHz |
| FFT size for UPOLS | 2 x blockSize |
| Convolution latency | blockSize samples |
| Memory budget | ~240 KB (256-sample blocks, 256-tap filters) |
| Recommended HRTF | TH Cologne KU100 (2702 dirs, 48 kHz, SOFA) |
| Decoding method | MagLS (magnitude least-squares) |
| Head tracking protocol | OSC /ypr or /quaternion |
| Rotation matrix structure | 16x16 block-diagonal (1x1 + 3x3 + 5x5 + 7x7) |

## Appendix B: Dependency Recommendations

| Dependency | Purpose | License | Integration |
|-----------|---------|---------|-------------|
| **libmysofa** | SOFA/HRTF file loading | BSD-3 | CMake subdirectory |
| **juce::dsp::FFT** | FFT/IFFT for UPOLS | JUCE license | Built-in |
| **juce::dsp::IIR::Filter** | Biquad for parametric fallback | JUCE license | Built-in |
| **juce_osc** | Head tracking OSC | JUCE license | Built-in module |
| **SAF (optional)** | MagLS computation, SH rotation | ISC (core) | CMake subdirectory |

For maximum portability and minimal external dependencies, the recommended stack is:
- **libmysofa** for SOFA loading (tiny, BSD-licensed, no deps)
- **JUCE built-in FFT** for all frequency-domain operations
- **Manual implementation** of MagLS, SH rotation, and UPOLS using the algorithms described in this document
- **SAF** as an optional accelerator for MagLS precomputation (can be used offline only, removing runtime dependency)
