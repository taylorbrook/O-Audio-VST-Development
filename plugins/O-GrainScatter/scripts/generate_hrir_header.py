#!/usr/bin/env python3
"""
Generate HRIRData.h for O-GrainScatter binaural decoder.

Approach:
1. 26-node Lebedev grid for virtual speaker positions
2. Synthetic spherical head model HRIRs (Woodworth ITD + Brown-Duda ILD)
3. Compute SH decoder matrix (pseudoinverse of SH matrix at speaker positions)
4. Fold decoder matrix into HRIR filters -> 16 input x 2 ears x 128 taps
5. Output as C++ constexpr float arrays

The result is 32 FIR filters that go directly from HOA3 ambisonics to binaural stereo.
"""

import numpy as np
from scipy.special import factorial
import sys
import os

# ============================================================
# Constants
# ============================================================
FILTER_LENGTH = 128
SAMPLE_RATE = 48000
HEAD_RADIUS = 0.0875  # meters (average human head)
SPEED_OF_SOUND = 343.0  # m/s
NUM_SH_CHANNELS = 16
AMBI_ORDER = 3

# ============================================================
# 26-node Lebedev grid (exact for SH degree 7, good for order 3)
# ============================================================
def get_lebedev_26():
    """Return 26 Lebedev grid points as (azimuth, elevation) in radians + weights."""
    # The 26-node grid consists of:
    # 6 points on coordinate axes (octahedron vertices)
    # 12 points on coordinate plane diagonals (cuboctahedron edges)
    # 8 points on space diagonals (cube vertices)

    points = []
    weights = []

    # Weight values for 26-node Lebedev
    w1 = 1.0 / 21.0  # 6 axis points
    w2 = 4.0 / 105.0  # 12 edge points
    w3 = 27.0 / 840.0  # 8 vertex points

    # 6 axis points: +/-x, +/-y, +/-z
    axis_dirs = [
        (1, 0, 0), (-1, 0, 0),
        (0, 1, 0), (0, -1, 0),
        (0, 0, 1), (0, 0, -1),
    ]
    for d in axis_dirs:
        points.append(d)
        weights.append(w1)

    # 12 edge midpoints (on coordinate planes, 45 degrees)
    s = 1.0 / np.sqrt(2.0)
    edge_dirs = [
        (s, s, 0), (s, -s, 0), (-s, s, 0), (-s, -s, 0),
        (s, 0, s), (s, 0, -s), (-s, 0, s), (-s, 0, -s),
        (0, s, s), (0, s, -s), (0, -s, s), (0, -s, -s),
    ]
    for d in edge_dirs:
        points.append(d)
        weights.append(w2)

    # 8 cube vertices (space diagonals)
    t = 1.0 / np.sqrt(3.0)
    cube_dirs = [
        (t, t, t), (t, t, -t), (t, -t, t), (t, -t, -t),
        (-t, t, t), (-t, t, -t), (-t, -t, t), (-t, -t, -t),
    ]
    for d in cube_dirs:
        points.append(d)
        weights.append(w3)

    # Convert Cartesian to spherical (azimuth, elevation)
    az_el = []
    for (x, y, z) in points:
        az = np.arctan2(y, x)
        el = np.arcsin(np.clip(z, -1.0, 1.0))
        az_el.append((az, el))

    # Normalize weights to sum to 4*pi (sphere surface area)
    weights = np.array(weights)
    weights *= 4.0 * np.pi / np.sum(weights)

    return az_el, weights, points


# ============================================================
# Real spherical harmonics (ACN/SN3D)
# ============================================================
def real_sh(n, m, az, el):
    """Compute single real SH coefficient Y_n^m(az, el) with SN3D normalization."""
    abs_m = abs(m)

    # SN3D normalization factor
    delta = 1.0 if m == 0 else 0.0
    norm = np.sqrt((2.0 - delta) * factorial(n - abs_m, exact=True)
                   / factorial(n + abs_m, exact=True))

    # Associated Legendre polynomial P_n^|m|(sin(el))
    # Without Condon-Shortley phase
    se = np.sin(el)
    ce = np.cos(el)
    se2 = se * se
    ce2 = ce * ce

    if n == 0:
        P = 1.0
    elif n == 1:
        if abs_m == 0: P = se
        else:          P = ce
    elif n == 2:
        if abs_m == 0:   P = (3 * se2 - 1) / 2
        elif abs_m == 1: P = 3 * se * ce
        else:            P = 3 * ce2
    elif n == 3:
        if abs_m == 0:   P = se * (5 * se2 - 3) / 2
        elif abs_m == 1: P = (3.0 / 2.0) * ce * (5 * se2 - 1)
        elif abs_m == 2: P = 15 * se * ce2
        else:            P = 15 * ce * ce2
    else:
        raise ValueError(f"Order {n} not supported")

    # Azimuthal function
    if m < 0:
        T = np.sin(abs_m * az)
    elif m == 0:
        T = 1.0
    else:
        T = np.cos(m * az)

    return norm * P * T


def sh_matrix(directions, order=3):
    """Build SH matrix Y[(N+1)^2 x L] evaluated at L directions."""
    n_sh = (order + 1) ** 2
    L = len(directions)
    Y = np.zeros((n_sh, L))

    for i, (az, el) in enumerate(directions):
        acn = 0
        for n in range(order + 1):
            for m in range(-n, n + 1):
                Y[acn, i] = real_sh(n, m, az, el)
                acn += 1
    return Y


# ============================================================
# Synthetic HRIR generation (spherical head model)
# ============================================================
def woodworth_itd(azimuth, elevation):
    """Woodworth formula for interaural time difference (seconds).
    For a rigid sphere of radius HEAD_RADIUS."""
    # Project azimuth to horizontal plane considering elevation
    theta = np.arcsin(np.clip(np.sin(azimuth) * np.cos(elevation), -1.0, 1.0))

    if abs(theta) < 1e-10:
        return 0.0

    # Woodworth formula: ITD = (a/c) * (theta + sin(theta))
    return (HEAD_RADIUS / SPEED_OF_SOUND) * (abs(theta) + np.sin(abs(theta))) * np.sign(theta)


def brown_duda_hrtf(frequency, theta_inc):
    """Brown-Duda spherical head shadow model.
    theta_inc: incidence angle relative to ear (radians, 0 = ipsilateral)
    Returns magnitude (linear)."""
    if frequency < 1.0:
        return 1.0

    alpha_min = 0.1
    # Frequency-dependent head shadow
    w = 2.0 * np.pi * frequency
    wo = SPEED_OF_SOUND / HEAD_RADIUS

    cos_theta = np.cos(theta_inc)
    alpha = 1.0 + alpha_min / 2.0 + (1.0 - alpha_min / 2.0) * cos_theta

    # Transfer function magnitude
    H = alpha * (1.0 + 1j * w / (alpha * wo)) / (1.0 + 1j * w / wo)
    return np.abs(H)


def generate_synthetic_hrir(az, el, ear='left'):
    """Generate a synthetic HRIR for one direction and one ear.
    Returns FILTER_LENGTH samples at SAMPLE_RATE."""
    # Angle of incidence relative to ear
    # Left ear at +90 degrees azimuth, right ear at -90 degrees
    if ear == 'left':
        ear_az = np.pi / 2.0
    else:
        ear_az = -np.pi / 2.0

    # Angular distance from source to ear
    # Using great-circle distance on unit sphere
    x_s = np.cos(az) * np.cos(el)
    y_s = np.sin(az) * np.cos(el)
    z_s = np.sin(el)

    x_e = np.cos(ear_az)
    y_e = np.sin(ear_az)
    z_e = 0.0

    cos_angle = np.clip(x_s * x_e + y_s * y_e + z_s * z_e, -1.0, 1.0)
    theta_inc = np.arccos(cos_angle)

    # ITD: delay for the far ear
    itd = woodworth_itd(az, el)

    if ear == 'left':
        delay_sec = max(0.0, -itd)  # positive ITD means left ear is closer (less delay)
    else:
        delay_sec = max(0.0, itd)

    delay_samples = delay_sec * SAMPLE_RATE

    # Build frequency response using Brown-Duda model
    nfft = FILTER_LENGTH * 2
    freqs = np.fft.rfftfreq(nfft, 1.0 / SAMPLE_RATE)

    H = np.zeros(len(freqs), dtype=complex)
    for i, f in enumerate(freqs):
        mag = brown_duda_hrtf(f, theta_inc)
        # Apply delay as phase shift
        phase = -2.0 * np.pi * f * delay_sec
        H[i] = mag * np.exp(1j * phase)

    # Inverse FFT to get impulse response
    ir = np.fft.irfft(H, nfft)

    # Window and truncate to FILTER_LENGTH
    ir = ir[:FILTER_LENGTH]
    # Apply raised cosine fade-out on last 16 samples
    fade_len = 16
    fade = 0.5 * (1.0 + np.cos(np.linspace(0, np.pi, fade_len)))
    ir[-fade_len:] *= fade

    # Normalize peak
    peak = np.max(np.abs(ir))
    if peak > 0:
        ir /= peak

    return ir.astype(np.float32)


# ============================================================
# Main: generate binaural decoding filters
# ============================================================
def main():
    print("Generating HOA3 binaural decoding filters...")
    print(f"  Filter length: {FILTER_LENGTH} taps")
    print(f"  Sample rate: {SAMPLE_RATE} Hz")
    print(f"  Lebedev grid: 26 nodes")
    print(f"  SH channels: {NUM_SH_CHANNELS}")

    # 1. Get Lebedev grid
    directions, weights, cartesian = get_lebedev_26()

    # 2. Build SH matrix at speaker positions
    Y = sh_matrix(directions, AMBI_ORDER)
    print(f"  SH matrix shape: {Y.shape}")
    print(f"  SH matrix condition number: {np.linalg.cond(Y):.2f}")

    # 3. Compute pseudoinverse decoder matrix D [L x N_SH]
    # D = Y^T * (Y * Y^T)^(-1)  -- but with quadrature weights:
    # D_i,n = w_i * Y_n(Omega_i) / (4*pi)
    # For the Lebedev grid, using weighted pseudoinverse
    W = np.diag(weights)
    # D = W @ Y.T @ inv(Y @ W @ Y.T)
    # But simpler: weighted pseudoinverse
    YW = Y @ W
    D = np.linalg.solve(YW @ Y.T, YW).T  # D is [L x N_SH]
    print(f"  Decoder matrix shape: {D.shape}")

    # 4. Generate HRIRs for each speaker direction
    hrir_left = np.zeros((len(directions), FILTER_LENGTH), dtype=np.float32)
    hrir_right = np.zeros((len(directions), FILTER_LENGTH), dtype=np.float32)

    for i, (az, el) in enumerate(directions):
        hrir_left[i] = generate_synthetic_hrir(az, el, 'left')
        hrir_right[i] = generate_synthetic_hrir(az, el, 'right')

    # 5. Fold decoder matrix into HRIR filters:
    #    binauralFilter[n][ear][tap] = sum_i D[i,n] * hrir[i][tap]
    #
    #    This gives us 16 left-ear filters and 16 right-ear filters.
    #    At runtime: outLeft[s] = sum_n (ambi[n][s] conv binauralLeft[n])
    binaural_left = np.zeros((NUM_SH_CHANNELS, FILTER_LENGTH), dtype=np.float64)
    binaural_right = np.zeros((NUM_SH_CHANNELS, FILTER_LENGTH), dtype=np.float64)

    for n in range(NUM_SH_CHANNELS):
        for i in range(len(directions)):
            binaural_left[n] += D[i, n] * hrir_left[i].astype(np.float64)
            binaural_right[n] += D[i, n] * hrir_right[i].astype(np.float64)

    # Normalize to prevent clipping
    max_val = max(np.max(np.abs(binaural_left)), np.max(np.abs(binaural_right)))
    if max_val > 0:
        scale = 0.95 / max_val
        binaural_left *= scale
        binaural_right *= scale
    print(f"  Peak filter value after normalization: {np.max(np.abs(binaural_left)):.4f}")

    # 6. Write C++ header
    output_path = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "Source", "dsp", "HRIRData.h"
    )

    with open(output_path, 'w') as f:
        f.write("#pragma once\n\n")
        f.write("// Auto-generated HOA3 binaural decoding filters.\n")
        f.write("// Generated by scripts/generate_hrir_header.py\n")
        f.write("//\n")
        f.write(f"// 26-node Lebedev grid, synthetic spherical head model\n")
        f.write(f"// (Woodworth ITD + Brown-Duda magnitude transfer)\n")
        f.write(f"// Filter length: {FILTER_LENGTH} taps at {SAMPLE_RATE} Hz\n")
        f.write(f"// Layout: binauralFilters[16 SH channels][2 ears][{FILTER_LENGTH} taps]\n")
        f.write("//   ear 0 = left, ear 1 = right\n\n")

        f.write(f"static constexpr int kBinauralFilterLength = {FILTER_LENGTH};\n\n")

        f.write(f"static constexpr float kBinauralFilters[{NUM_SH_CHANNELS}][2][{FILTER_LENGTH}] =\n")
        f.write("{\n")

        for ch in range(NUM_SH_CHANNELS):
            f.write(f"    {{ // SH channel {ch}\n")
            for ear in range(2):
                ear_name = "left" if ear == 0 else "right"
                data = binaural_left[ch] if ear == 0 else binaural_right[ch]
                f.write(f"        {{ // {ear_name}\n")
                f.write("            ")
                for t in range(FILTER_LENGTH):
                    val = float(data[t])
                    f.write(f"{val: .8e}f")
                    if t < FILTER_LENGTH - 1:
                        f.write(",")
                        if (t + 1) % 6 == 0:
                            f.write("\n            ")
                        else:
                            f.write(" ")
                f.write("\n        }")
                if ear == 0:
                    f.write(",")
                f.write("\n")
            f.write("    }")
            if ch < NUM_SH_CHANNELS - 1:
                f.write(",")
            f.write("\n")

        f.write("};\n")

    print(f"  Written to: {output_path}")
    print(f"  File size: {os.path.getsize(output_path) / 1024:.1f} KB")
    print("Done.")


if __name__ == "__main__":
    main()
