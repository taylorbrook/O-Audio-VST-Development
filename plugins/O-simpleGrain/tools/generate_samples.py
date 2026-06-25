#!/usr/bin/env python3
"""
O-simpleGrain — built-in source sample generator.

Synthesizes the four shipping v1.0 built-in granular sources
(fire / voice / water / piano) procedurally so they are reproducible and
version-controlled. These ARE the final built-ins (user decision 2026-06-24,
"procedural as final v1.0"), embedded via juce_add_binary_data in Stage 2.3.

Output: plugins/O-simpleGrain/Source/samples/{fire,voice,water,piano}.wav
Format: mono, 44100 Hz, 24-bit PCM, peak-normalized to -1 dBFS.

Run:  python3 plugins/O-simpleGrain/tools/generate_samples.py
Deps: numpy, scipy
"""

import os
import struct
import numpy as np
from scipy.signal import butter, sosfilt, lfilter

SR = 44100
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "Source", "samples")
SEED = 20260624  # deterministic, reproducible assets


# ----------------------------------------------------------------------------
# Helpers
# ----------------------------------------------------------------------------
def _rng():
    return np.random.default_rng(SEED)


def normalize(x, peak_dbfs=-1.0):
    x = np.asarray(x, dtype=np.float64)
    x = x - np.mean(x)  # remove DC
    peak = np.max(np.abs(x))
    if peak < 1e-9:
        return x
    target = 10.0 ** (peak_dbfs / 20.0)
    return x * (target / peak)


def fade(x, fade_in_ms=5.0, fade_out_ms=40.0):
    n = len(x)
    fi = int(SR * fade_in_ms / 1000.0)
    fo = int(SR * fade_out_ms / 1000.0)
    env = np.ones(n)
    if fi > 0:
        env[:fi] = np.linspace(0.0, 1.0, fi)
    if fo > 0:
        env[-fo:] = np.linspace(1.0, 0.0, fo)
    return x * env


def bandpass(x, lo, hi, order=2):
    sos = butter(order, [lo / (SR / 2), hi / (SR / 2)], btype="band", output="sos")
    return sosfilt(sos, x)


def lowpass(x, fc, order=2):
    sos = butter(order, fc / (SR / 2), btype="low", output="sos")
    return sosfilt(sos, x)


def highpass(x, fc, order=2):
    sos = butter(order, fc / (SR / 2), btype="high", output="sos")
    return sosfilt(sos, x)


def formant(x, freq, bw, gain):
    """Single resonant formant via a 2-pole bandpass-ish resonator."""
    r = np.exp(-np.pi * bw / SR)
    theta = 2 * np.pi * freq / SR
    a = [1.0, -2.0 * r * np.cos(theta), r * r]
    b = [gain * (1.0 - r)]
    return lfilter(b, a, x)


def write_wav24(path, x):
    """Write mono float (-1..1) as 24-bit PCM WAV."""
    x = np.clip(x, -1.0, 1.0)
    ints = np.round(x * (2**23 - 1)).astype(np.int32)
    frames = bytearray()
    for v in ints:
        v = int(v) & 0xFFFFFF
        frames += struct.pack("<i", v)[:3]  # little-endian 3 bytes
    n = len(ints)
    byte_rate = SR * 3
    block_align = 3
    data_size = n * 3
    with open(path, "wb") as f:
        f.write(b"RIFF")
        f.write(struct.pack("<I", 36 + data_size))
        f.write(b"WAVE")
        f.write(b"fmt ")
        f.write(struct.pack("<I", 16))
        f.write(struct.pack("<H", 1))            # PCM
        f.write(struct.pack("<H", 1))            # mono
        f.write(struct.pack("<I", SR))
        f.write(struct.pack("<I", byte_rate))
        f.write(struct.pack("<H", block_align))
        f.write(struct.pack("<H", 24))           # bits
        f.write(b"data")
        f.write(struct.pack("<I", data_size))
        f.write(bytes(frames))


# ----------------------------------------------------------------------------
# fire — filtered brown-noise bed + random exponentially-decaying crackles
# ----------------------------------------------------------------------------
def make_fire(dur=2.6):
    rng = _rng()
    n = int(SR * dur)
    # Roar bed: brown noise, low-passed, gently undulating.
    white = rng.standard_normal(n)
    brown = np.cumsum(white)
    brown = lowpass(brown, 350, order=2)
    brown = brown / (np.max(np.abs(brown)) + 1e-9)
    lfo = 0.6 + 0.4 * (0.5 + 0.5 * np.sin(2 * np.pi * 0.7 * np.arange(n) / SR))
    bed = brown * lfo * 0.5

    # Crackles: random short bandpassed noise bursts with fast exp decay.
    crackle = np.zeros(n)
    n_cracks = int(dur * 28)
    for _ in range(n_cracks):
        start = rng.integers(0, n - 1)
        length = int(SR * rng.uniform(0.004, 0.030))
        end = min(n, start + length)
        seg = end - start
        if seg <= 1:
            continue
        env = np.exp(-np.linspace(0, rng.uniform(6, 14), seg))
        burst = rng.standard_normal(seg) * env
        fc = rng.uniform(900, 4500)
        burst = bandpass(burst, max(200, fc * 0.6), min(SR / 2 - 500, fc * 1.6), order=2)
        crackle[start:end] += burst * rng.uniform(0.5, 1.0)

    x = bed + crackle * 0.9
    x = highpass(x, 60, order=2)
    return fade(normalize(x), 8, 60)


# ----------------------------------------------------------------------------
# water — bandpassed noise stream + rising-pitch "bloop" bubbles
# ----------------------------------------------------------------------------
def make_water(dur=2.6):
    rng = _rng()
    n = int(SR * dur)
    # Stream bed: bandpassed white noise, mid-high.
    bed = bandpass(rng.standard_normal(n), 600, 6000, order=2) * 0.18

    # Bubbles: short sine chirps that rise in frequency with quick decay.
    bubbles = np.zeros(n)
    n_bub = int(dur * 22)
    for _ in range(n_bub):
        start = rng.integers(0, n - 1)
        length = int(SR * rng.uniform(0.020, 0.090))
        end = min(n, start + length)
        seg = end - start
        if seg <= 2:
            continue
        t = np.arange(seg) / SR
        f0 = rng.uniform(400, 1200)
        f1 = f0 * rng.uniform(1.6, 3.0)        # bubble pitch rises as it collapses
        inst = np.linspace(f0, f1, seg)
        phase = 2 * np.pi * np.cumsum(inst) / SR
        env = np.exp(-np.linspace(0, rng.uniform(5, 9), seg))
        bubbles[start:end] += np.sin(phase) * env * rng.uniform(0.4, 1.0)

    x = bed + bubbles * 0.7
    return fade(normalize(x), 8, 60)


# ----------------------------------------------------------------------------
# voice — glottal pulse train through "ah" formants + light breath
# ----------------------------------------------------------------------------
def make_voice(dur=2.2):
    rng = _rng()
    n = int(SR * dur)
    t = np.arange(n) / SR
    f0 = 123.47  # B2-ish
    vibrato = 1.0 + 0.006 * np.sin(2 * np.pi * 5.2 * t)
    inst = f0 * vibrato
    phase = 2 * np.pi * np.cumsum(inst) / SR
    # Band-limited-ish glottal source: sum of a few harmonics with 1/k rolloff.
    src = np.zeros(n)
    for k in range(1, 30):
        if f0 * k > SR / 2 - 1000:
            break
        src += np.sin(k * phase) / k
    src = src / (np.max(np.abs(src)) + 1e-9)

    # "ah" formants (F1..F3) — male-ish.
    out = (
        formant(src, 730, 80, 1.0)
        + formant(src, 1090, 90, 0.5)
        + formant(src, 2440, 120, 0.18)
    )
    breath = bandpass(rng.standard_normal(n), 1500, 5000, order=2) * 0.015
    out = out + breath

    # Natural-ish amplitude arc (swell in, settle, fade).
    arc = np.minimum(1.0, t / 0.12) * np.exp(-np.maximum(0.0, t - dur * 0.7) * 1.2)
    out = out * arc
    return fade(normalize(out), 12, 120)


# ----------------------------------------------------------------------------
# piano — struck slightly-inharmonic partials + onset hammer noise
# ----------------------------------------------------------------------------
def make_piano(dur=3.0, f0=130.81):  # C3
    rng = _rng()
    n = int(SR * dur)
    t = np.arange(n) / SR
    B = 0.0008  # inharmonicity coefficient
    out = np.zeros(n)
    k = 1
    while True:
        fk = f0 * k * np.sqrt(1.0 + B * k * k)
        if fk > SR / 2 - 500:
            break
        # higher partials decay faster; amplitude rolls off
        decay = 0.6 + 2.2 * (k ** 0.8)
        amp = (1.0 / (k ** 1.2)) * np.exp(-decay * t)
        out += amp * np.sin(2 * np.pi * fk * t + rng.uniform(0, 2 * np.pi))
        k += 1
    # Hammer transient: brief filtered noise burst at onset.
    hn = int(SR * 0.012)
    hammer = np.zeros(n)
    hammer[:hn] = rng.standard_normal(hn) * np.exp(-np.linspace(0, 8, hn))
    hammer = bandpass(hammer, 800, 6000, order=2) * 0.5
    out = out + hammer
    # Fast attack
    atk = int(SR * 0.004)
    out[:atk] *= np.linspace(0, 1, atk)
    return fade(normalize(out), 2, 200)


# ----------------------------------------------------------------------------
def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    gens = {
        "fire": make_fire,
        "water": make_water,
        "voice": make_voice,
        "piano": make_piano,
    }
    for name, fn in gens.items():
        x = fn()
        path = os.path.abspath(os.path.join(OUT_DIR, f"{name}.wav"))
        write_wav24(path, x)
        dur = len(x) / SR
        print(f"  wrote {name}.wav  ({dur:.2f}s, {len(x)} samples, 24-bit/{SR}Hz)")
    print(f"Done -> {os.path.abspath(OUT_DIR)}")


if __name__ == "__main__":
    main()
