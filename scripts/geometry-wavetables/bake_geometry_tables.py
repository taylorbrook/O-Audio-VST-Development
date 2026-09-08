#!/usr/bin/env python3
"""
bake_geometry_tables.py — O-Prism "Geometry" factory wavetable bank.

Bakes eight wavetables OFFLINE from the committed research prototype
(research/wavetable-synthesis-3d-geometry-prototypes/mesh-slice/) and writes:

  plugins/O-Prism/Source/dsp/GeometryTablesData.h      committed, int16 frames
  scripts/geometry-wavetables/manifest.json             committed, SHA-256 per table
  plugins/O-Prism/.planning/evidence/geometry-wavs/*.wav  gitignored previews

No generator code ships in the plugin: the header is plain `constexpr int16_t`
arrays (no juce_add_binary_data — that strips hyphens and collides namespaces).

Conditioning (mirrors O-Prism's WavetableImporter and the O-Strata DSP-04/05
contract): per-frame DC removal -> frame 0 rotated to its max-positive-slope
zero crossing -> chained FFT cross-correlation alignment with polarity test
-> GLOBAL peak normalisation across all frames. Never per-frame peak.

Gates (script exits non-zero on any failure):
  (a) pitch   harmonic 1 is the strongest partial or within 6 dB of it in
              >= 95 % of frames
  (b) travel  end-to-end RMS(frame 0, frame N-1) >= 0.3 OR centroid ratio >= 2
  (c) smooth  adjacent-frame RMS <= 0.05
  (d) sane    no NaN/Inf; max |int16| == 32767 (global peak exactly 1.0)

Deterministic: the prototype seeds its permutation table at import
(default_rng(1234)); nothing here draws randomness. Re-running yields a
byte-identical header (`--verify` proves it against manifest.json).

Usage:
    python3 scripts/geometry-wavetables/bake_geometry_tables.py            # bake + write
    python3 scripts/geometry-wavetables/bake_geometry_tables.py --verify   # re-bake, compare SHA, write nothing
    python3 scripts/geometry-wavetables/bake_geometry_tables.py --only "Noise Knot" --stats
"""
import sys, os, json, hashlib, argparse, math
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
PROTO = os.path.join(ROOT, "research", "wavetable-synthesis-3d-geometry-prototypes", "mesh-slice")
sys.path.insert(0, PROTO)
import mesh_slice_wavetable as M  # noqa: E402  (seeds default_rng(1234) at import)

N = M.N  # 2048 == WavetableData::kTableSize
HEADER = os.path.join(ROOT, "plugins", "O-Prism", "Source", "dsp", "GeometryTablesData.h")
MANIFEST = os.path.join(HERE, "manifest.json")
WAV_DIR = os.path.join(ROOT, "plugins", "O-Prism", ".planning", "evidence", "geometry-wavs")
INT16_PEAK = 32767

# ---------------------------------------------------------------------------
# Sources the prototype lacks
# ---------------------------------------------------------------------------
def rotate_x(V, a):
    c, s = math.cos(a), math.sin(a)
    y, z = V[:, 1], V[:, 2]
    return np.stack([V[:, 0], c * y - s * z, s * y + c * z], -1)

def rotate_y(V, a):
    c, s = math.cos(a), math.sin(a)
    x, z = V[:, 0], V[:, 2]
    return np.stack([c * x + s * z, V[:, 1], -s * x + c * z], -1)

def box_sdf(p, b=(0.5, 0.4, 0.3)):
    d = np.abs(p) - np.asarray(b)
    return np.linalg.norm(np.maximum(d, 0), axis=-1) + np.minimum(np.max(d, axis=-1), 0)

def gyroid(p, k=2.6):
    q = p * k
    return (np.sin(q[..., 0]) * np.cos(q[..., 1])
            + np.sin(q[..., 1]) * np.cos(q[..., 2])
            + np.sin(q[..., 2]) * np.cos(q[..., 0]))

# ---------------------------------------------------------------------------
# Frame producers — each returns (frames, N) float64, DC NOT yet removed
# ---------------------------------------------------------------------------
def slice_component(V, T, h, method):
    loops = [M.orient_ccw(L) for L in M.slice_mesh(V, T, h) if abs(M.loop_area(L)) > 1e-9]
    if not loops:
        raise RuntimeError(f"empty slice at h={h}")
    L = max(loops, key=lambda L: abs(M.loop_area(L)))
    u = M.unwrap_arclength(L)
    return u[method]

def mesh_tilt(V, T, frames, method, axis, tilt_deg, h=0.0):
    rot = rotate_x if axis == "x" else rotate_y
    out = []
    for k in range(frames):
        a = math.radians(tilt_deg[0] + (tilt_deg[1] - tilt_deg[0]) * k / (frames - 1))
        out.append(slice_component(rot(V, a), T, h, method))
    return np.asarray(out)

def mesh_tilt_z(V, T, frames, method, axis, tilt_deg, zspan):
    """Tilt and slice height swept together — tilt alone leaves the star just
    under the travel gate (0.27-0.29 RMS end to end)."""
    rot = rotate_x if axis == "x" else rotate_y
    out = []
    for k in range(frames):
        u = k / (frames - 1)
        a = math.radians(tilt_deg[0] + (tilt_deg[1] - tilt_deg[0]) * u)
        h = zspan[0] + (zspan[1] - zspan[0]) * u
        out.append(slice_component(rot(V, a), T, h, method))
    return np.asarray(out)

def mesh_z(V, T, frames, method, zspan):
    out = []
    for k in range(frames):
        h = zspan[0] + (zspan[1] - zspan[0]) * k / (frames - 1)
        out.append(slice_component(V, T, h, method))
    return np.asarray(out)

def orbit_scale(field, frames, offset, scale, p=2, q=3, drive=3.0):
    off = np.asarray(offset, float)
    knot = M.torus_knot(N, p=p, q=q)
    out = []
    for k in range(frames):
        s = scale[0] + (scale[1] - scale[0]) * k / (frames - 1)
        out.append(np.tanh(drive * field(knot * s + off)))
    return np.asarray(out)

def terrain_rings(frames, r=(0.15, 1.0), c=(0.13, 0.21), aspect=0.7):
    th = 2 * np.pi * np.arange(N) / N
    out = []
    for k in range(frames):
        rr = r[0] + (r[1] - r[0]) * k / (frames - 1)
        x = c[0] + rr * np.cos(th)
        y = c[1] + aspect * rr * np.sin(th)
        out.append(np.sin(2 * np.pi * x) + np.sin(2 * np.pi * y))   # sum, not product: the product's h2 wins past r ~ 0.5
    return np.asarray(out)

# ---------------------------------------------------------------------------
# Conditioning — DC, alignment chain, GLOBAL peak
# ---------------------------------------------------------------------------
def roll_to_rising_zero(w):
    """Rotate so index 0 sits on the negative->positive zero crossing with the
    steepest positive slope (frame 0 anchor, O-Strata CONTEXT 'Alignment')."""
    nxt = np.roll(w, -1)
    cross = np.nonzero((w < 0) & (nxt >= 0))[0]
    if len(cross) == 0:
        return w
    k = cross[np.argmax(nxt[cross] - w[cross])]
    return np.roll(w, -(k + 1))

def condition(frames):
    A = np.asarray(frames, dtype=np.float64)
    A = A - A.mean(axis=1, keepdims=True)                 # per-frame DC removal
    A[0] = roll_to_rising_zero(A[0])
    for i in range(1, len(A)):
        A[i], _ = M.align_xcorr(A[i - 1], A[i])          # chained xcorr + polarity
    peak = np.max(np.abs(A))                              # GLOBAL peak, all frames
    if peak <= 0:
        raise RuntimeError("all-zero table")
    return A / peak

def to_int16(A):
    q = np.round(A * INT16_PEAK).astype(np.int64)
    q = np.clip(q, -INT16_PEAK, INT16_PEAK)               # symmetric, never -32768
    return q.astype(np.int16)

# ---------------------------------------------------------------------------
# Metrics + gates
# ---------------------------------------------------------------------------
def spectrum(w):
    S = np.abs(np.fft.rfft(w)) / (len(w) / 2)
    S[0] = 0.0
    return S

def metrics(A):
    F = len(A)
    h1_rel, cent, npart, fpeak = [], [], [], []
    for w in A:
        S = spectrum(w); ref = S.max()
        db = 20 * np.log10(np.maximum(S / ref, 1e-12)) if ref > 0 else np.full_like(S, -200.0)
        h1_rel.append(float(db[1]))
        k = np.arange(len(S))
        cent.append(float(np.sum(k * S ** 2) / max(np.sum(S ** 2), 1e-30)))
        npart.append(int(np.sum(db >= -40)))
        fpeak.append(float(np.max(np.abs(w))))
    h1_rel = np.asarray(h1_rel); cent = np.asarray(cent)
    adj = np.sqrt(np.mean(np.diff(A, axis=0) ** 2, axis=1))
    return {
        "frames": F,
        "h1_within_6dB_pct": float(np.mean(h1_rel >= -6.0) * 100),
        "h1_rel_dB_median": float(np.median(h1_rel)),
        "h1_rel_dB_min": float(h1_rel.min()),
        "end_to_end_rms": float(np.sqrt(np.mean((A[0] - A[-1]) ** 2))),
        "end_to_end_corr": float(np.corrcoef(A[0], A[-1])[0, 1]),
        "centroid_first": float(cent[0]), "centroid_last": float(cent[-1]),
        "centroid_ratio": float(max(cent[0], cent[-1]) / max(min(cent[0], cent[-1]), 1e-9)),
        "partials_first_mid_last": [npart[0], npart[F // 2], npart[-1]],
        "adjacent_rms_max": float(adj.max()),
        "adjacent_rms_mean": float(adj.mean()),
        "frame_peak_min_dB": float(20 * np.log10(max(min(fpeak), 1e-9))),
        "finite": bool(np.all(np.isfinite(A))),
        "peak": float(np.max(np.abs(A))),
    }

def gates(m, q):
    fails = []
    if m["h1_within_6dB_pct"] < 95.0:
        fails.append(f"(a) pitch: h1 within 6 dB in only {m['h1_within_6dB_pct']:.1f} % of frames (min {m['h1_rel_dB_min']:.1f} dB)")
    if not (m["end_to_end_rms"] >= 0.3 or m["centroid_ratio"] >= 2.0):
        fails.append(f"(b) travel: end-to-end RMS {m['end_to_end_rms']:.3f} < 0.3 and centroid ratio {m['centroid_ratio']:.2f} < 2")
    if m["adjacent_rms_max"] > 0.05:
        fails.append(f"(c) smooth: adjacent-frame RMS max {m['adjacent_rms_max']:.3f} > 0.05")
    if not m["finite"]:
        fails.append("(d) non-finite sample")
    if int(np.max(np.abs(q.astype(np.int32)))) != INT16_PEAK:
        fails.append(f"(d) int16 peak {int(np.max(np.abs(q.astype(np.int32))))} != {INT16_PEAK}")
    return fails

# ---------------------------------------------------------------------------
# The bank — order == factory indices 28..35. Names <= 16 chars.
#
# Every orbit table rides a (1,q) torus-knot path: p == 1 gives the path one
# excursion around the axis per cycle, which is what puts harmonic 1 on top.
# A (2,3) knot (the research default) folds the period and fails gate (a) at
# every offset tried (scan of 2026-09-08). The offset breaks the remaining
# centrosymmetry; centred orbits play a twelfth or an octave up.
# ---------------------------------------------------------------------------
def blob_sdf(p, amp=0.4, freq=2.0):
    """Signed distance-ish to an fBm-displaced unit sphere (the research Blob),
    read by an orbit instead of a slice: its XY slices are near-circles, so a
    slice readout is a sine in every frame and has no travel."""
    return np.linalg.norm(p, axis=-1) - (1.0 + amp * M.fbm3(p * freq))

def bank():
    star = M.mesh_twisted_star()                       # 5 points, inner 0.45, twist pi
    star_pts = M.mesh_twisted_star(inner=0.3)          # deeper points
    return [
        # name            ident          frames  producer
        ("Star Tilt",     "StarTilt",     32, lambda f: mesh_tilt_z(*star, f, "x", "x", (0.0, 30.0), (-0.5, 0.5))),
        ("Star Points",   "StarPoints",   32, lambda f: mesh_tilt(*star_pts, f, "y", "y", (0.0, 50.0))),
        ("Blob Orbit",    "BlobOrbit",    32, lambda f: orbit_scale(blob_sdf, f, (0.60, 0.35, 0.20), (0.6, 1.2), p=1, q=2)),
        ("Knot Torus",    "KnotTorus",    32, lambda f: orbit_scale(M.torus_sdf, f, (0.30, 0.20, 0.10), (0.6, 1.2), p=1, q=2)),
        ("Knot Box",      "KnotBox",      32, lambda f: orbit_scale(box_sdf, f, (0.45, 0.30, 0.15), (0.4, 1.3), p=1, q=5)),
        ("Gyroid Orbit",  "GyroidOrbit",  32, lambda f: orbit_scale(lambda p: gyroid(p, 2.4), f, (0.15, 0.10, 0.05), (0.6, 1.2), p=1, q=5)),
        ("Noise Knot",    "NoiseKnot",    32, lambda f: orbit_scale(lambda p: M.fbm3(p * 1.0), f, (0.45, 0.30, 0.15), (0.6, 1.2), p=1, q=3)),
        ("Terrain Rings", "TerrainRings", 32, lambda f: terrain_rings(f, (0.15, 0.6), (0.13, 0.21), 0.5)),
    ]

# ---------------------------------------------------------------------------
# Writers
# ---------------------------------------------------------------------------
def sha256_bytes(b):
    return hashlib.sha256(b).hexdigest()

def write_header(path, tables):
    lines = []
    w = lines.append
    w("/*")
    w("   This file is part of O-Prism, an Ouaricon Audio plugin.")
    w("   Copyright (C) 2026  Ouaricon Audio")
    w("")
    w("   SPDX-License-Identifier: AGPL-3.0-or-later")
    w("")
    w("   This program is free software: you can redistribute it and/or modify")
    w("   it under the terms of the GNU Affero General Public License as published by")
    w("   the Free Software Foundation, either version 3 of the License, or")
    w("   (at your option) any later version.")
    w("")
    w("   This program is distributed in the hope that it will be useful,")
    w("   but WITHOUT ANY WARRANTY; without even the implied warranty of")
    w("   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the")
    w("   GNU Affero General Public License for more details.")
    w("")
    w("   You should have received a copy of the GNU Affero General Public License")
    w("   along with this program.  If not, see <https://www.gnu.org/licenses/>.")
    w("*/")
    w("/*")
    w("  ==============================================================================")
    w("")
    w("    GeometryTablesData.h — GENERATED. DO NOT EDIT.")
    w("    O-Prism - Microtonal Wavetable Synthesizer")
    w("    Ouaricon Audio")
    w("")
    w("    Baked by scripts/geometry-wavetables/bake_geometry_tables.py from the")
    w("    research prototype (mesh-slice/mesh_slice_wavetable.py). Frames are")
    w("    int16, DC-free, cross-correlation aligned and GLOBALLY peak-normalised")
    w("    (max |value| == 32767 over the whole table, never per frame). The loader")
    w("    in WavetableFactory.cpp copies level 0 verbatim and generates mipmaps;")
    w("    it must not re-normalise. SHA-256 per table: manifest.json beside the")
    w("    script. Included by WavetableFactory.cpp only.")
    w("")
    w("  ==============================================================================")
    w("*/")
    w("")
    w("#pragma once")
    w("#include <cstdint>")
    w("")
    w("namespace GeometryTables")
    w("{")
    w("")
    w("struct Table")
    w("{")
    w("    const char*    name;")
    w("    int            numFrames;")
    w("    const int16_t* data;   // numFrames * kFrameSize, frame-major")
    w("};")
    w("")
    w("inline constexpr int kFrameSize = %d;" % N)
    w("inline constexpr int kNumTables = %d;" % len(tables))
    w("")
    for name, ident, frames, q in tables:
        w("// %s — %d frames" % (name, frames))
        w("inline constexpr int16_t k%s[%d * %d] = {" % (ident, frames, N))
        flat = q.reshape(-1)
        per = 32
        for i in range(0, len(flat), per):
            w("    " + ",".join(str(int(v)) for v in flat[i:i + per]) + ",")
        w("};")
        w("")
    w("inline constexpr Table kTables[kNumTables] = {")
    for name, ident, frames, q in tables:
        w('    { "%s", %d, k%s },' % (name, frames, ident))
    w("};")
    w("")
    w("} // namespace GeometryTables")
    w("")
    text = "\n".join(lines)
    with open(path, "w", newline="\n") as fh:
        fh.write(text)
    return text

def write_wav(path, A):
    from scipy.io import wavfile
    wavfile.write(path, 44100, np.asarray(A, np.float32).reshape(-1))

# ---------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--verify", action="store_true", help="re-bake and compare SHA-256 against manifest.json; write nothing")
    ap.add_argument("--only", help="bake a single table by name (stats only, no write)")
    ap.add_argument("--stats", action="store_true", help="print full metrics")
    ap.add_argument("--no-gates", action="store_true", help="report gate failures but keep going (exploration)")
    args = ap.parse_args()

    tables, report, failures = [], {}, []
    for name, ident, frames, make in bank():
        if args.only and name != args.only:
            continue
        A = condition(make(frames))
        q = to_int16(A)
        m = metrics(q.astype(np.float64) / INT16_PEAK)   # measure what ships, not the float
        fails = gates(m, q)
        status = "PASS" if not fails else "FAIL"
        print(f"[{status}] {name:14s} {frames:3d} fr | h1<=6dB {m['h1_within_6dB_pct']:5.1f}% (med {m['h1_rel_dB_median']:+5.1f} dB)"
              f" | travel rms {m['end_to_end_rms']:.3f} corr {m['end_to_end_corr']:+.2f} cent {m['centroid_first']:.1f}->{m['centroid_last']:.1f}"
              f" | adj max {m['adjacent_rms_max']:.3f} | partials {m['partials_first_mid_last']} | quietest frame {m['frame_peak_min_dB']:+.1f} dB")
        for f in fails:
            print("       " + f)
        if args.stats:
            print("       " + json.dumps(m))
        if fails:
            failures.append((name, fails))
        tables.append((name, ident, frames, q))
        report[name] = m

    if failures and not args.no_gates:
        print(f"\n{len(failures)} table(s) failed a gate — nothing written.")
        return 2
    if args.only:
        return 0

    manifest = {
        "generator": "scripts/geometry-wavetables/bake_geometry_tables.py",
        "prototype": "research/wavetable-synthesis-3d-geometry-prototypes/mesh-slice/mesh_slice_wavetable.py",
        "frame_size": N,
        "first_factory_index": 28,
        "tables": [
            {"index": 28 + i, "name": name, "ident": ident, "frames": frames,
             "sha256_int16_le": sha256_bytes(q.astype("<i2").tobytes()),
             "metrics": report[name]}
            for i, (name, ident, frames, q) in enumerate(tables)
        ],
    }

    if args.verify:
        if not os.path.exists(MANIFEST):
            print("no manifest.json to verify against"); return 3
        with open(MANIFEST) as fh:
            old = json.load(fh)
        bad = 0
        for a, b in zip(old["tables"], manifest["tables"]):
            same = a["sha256_int16_le"] == b["sha256_int16_le"] and a["name"] == b["name"] and a["frames"] == b["frames"]
            print(f"  {'ok  ' if same else 'DIFF'} {b['name']:14s} {b['sha256_int16_le'][:16]}")
            bad += (not same)
        if len(old["tables"]) != len(manifest["tables"]):
            bad += 1
        # the committed header must be the one this bake would write
        import tempfile
        with tempfile.NamedTemporaryFile("w", suffix=".h", delete=False) as tf:
            tmp = tf.name
        text = write_header(tmp, tables); os.unlink(tmp)
        with open(HEADER) as fh:
            same_header = fh.read() == text
        print(f"  {'ok  ' if same_header else 'DIFF'} GeometryTablesData.h byte-identical")
        bad += (not same_header)
        print("verify:", "PASS" if bad == 0 else f"FAIL ({bad})")
        return 0 if bad == 0 else 4

    text = write_header(HEADER, tables)
    manifest["header_sha256"] = sha256_bytes(text.encode("utf-8"))
    with open(MANIFEST, "w") as fh:
        json.dump(manifest, fh, indent=2); fh.write("\n")
    os.makedirs(WAV_DIR, exist_ok=True)
    for name, ident, frames, q in tables:
        write_wav(os.path.join(WAV_DIR, f"{ident}.wav"), q.astype(np.float32) / INT16_PEAK)
    print(f"\nwrote {os.path.relpath(HEADER, ROOT)} ({len(text) // 1024} KB), {os.path.relpath(MANIFEST, ROOT)}, {len(tables)} WAVs -> {os.path.relpath(WAV_DIR, ROOT)}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
