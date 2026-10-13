#!/usr/bin/env python3
"""
generate_binaural_filters.py — O-simpleAmbisonics Phase 2.3 (PLAN-2.3 T1).

SADIE II D1 (KU100) HRIRs → per-order MagLS binaural decoding filters
(orders 1 / 2 / 3, ACN / SN3D / AmbiX, +y = left = azimuth +90°) → three
generated artifacts:

  Source/dsp/BinauralFilterData.h          the plugin's filter bank (THE artifact)
  tests/render-harness/BinauralFilterGoldens.h   test-only scipy resample goldens (44.1 k / 96 k)
  scripts/FILTERS.md                       provenance + every self-check number
  .cache/binaural_filters.npz              banks + goldens + numbers (gitignored)

Run from anywhere (paths resolve from this file's location):

  uv run --python 3.12 --with "sofar==1.3.0,numpy,scipy<1.15,spaudiopy" \
      scripts/generate_binaural_filters.py [--trim 32] [--blend hard|target] [--no-download]

The algorithm is the research transcription (RESEARCH-2.3 R2, r2d_final.py):
sn3d_matrix / magls / trunc verbatim. spaudiopy is the OUTSIDE ORACLE only —
nothing of it is emitted; if it cannot be imported the run FAILS (F3).

Decisions bound by PLAN-2.3: C1 trim T = 32; C2 hard switch with 'avg'
phase continuation (slice-then-diff estimator), no half-octave blend
(`--blend target` keeps the safe target-domain blend as the one-flag
alternative — recorded as a wording deviation from ARCHITECTURE); C7 the
self-checks below, any FAIL aborts without emitting; C8 header contents;
C12 goldens header.
"""
import argparse
import hashlib
import json
import os
import re
import sys
import time
import urllib.request
import warnings
import zipfile
from math import factorial

import numpy as np

warnings.filterwarnings("ignore")

# ── locations ────────────────────────────────────────────────────────────────
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PLUGIN_ROOT = os.path.dirname(SCRIPT_DIR)
CACHE = os.path.join(PLUGIN_ROOT, ".cache")
ZIP_PATH = os.path.join(CACHE, "D1_HRIR_SOFA.zip")
SOFA_DIR = os.path.join(CACHE, "sadie", "D1_HRIR_SOFA")
SOFA_48K = os.path.join(SOFA_DIR, "D1_48K_24bit_256tap_FIR_SOFA.sofa")
HEADER_OUT = os.path.join(PLUGIN_ROOT, "Source", "dsp", "BinauralFilterData.h")
GOLDENS_OUT = os.path.join(PLUGIN_ROOT, "tests", "render-harness", "BinauralFilterGoldens.h")
FILTERS_MD = os.path.join(SCRIPT_DIR, "FILTERS.md")
NPZ_OUT = os.path.join(CACHE, "binaural_filters.npz")

ZENODO_RECORD = "12092466"
ZIP_URL = f"https://zenodo.org/records/{ZENODO_RECORD}/files/D1_HRIR_SOFA.zip?download=1"
ZIP_SHA256 = "366321fa78f211bacc0ec6bea96701625b196a5db54ec16748b0eab0b9705f75"
SOFA_48K_SHA256 = "9af7cb19531e52fb7ae8ec92621e6ab62b1d5fe584b3742be36699a0ddb0ccd4"
SOFA_OTHER = {  # provenance only (R1) — listed, never read
    "D1_44K_16bit_256tap_FIR_SOFA.sofa": "efffbf7f8e963c8162f24f44944ed6f8a44f5d3c546e76a4d74a650a0e3766d2",
    "D1_96K_24bit_512tap_FIR_SOFA.sofa": "49c46541517cd8c43927297fc9106b8a9aee7aec25c3a6ba6d144b6da2f98983",
}

# ── physics / design constants (ARCHITECTURE §Binaural Decoder) ──────────────
C_SOUND = 343.0
R_HEAD = 0.0875
NFFT = 1024
TAPS = 256
FS = 48000.0
STEREO_FRONT_LEVEL = 0.75          # the ±60° cardioid pair's exact front gain (A4)
ORDERS = (1, 2, 3)

# ── self-check bookkeeping ───────────────────────────────────────────────────
CHECKS = []          # (name, ok, detail)
NUMBERS = {}         # everything FILTERS.md records


def check(name, ok, detail=""):
    CHECKS.append((name, bool(ok), detail))
    print(f"  [{'PASS' if ok else 'FAIL'}] {name}{' — ' + detail if detail else ''}")


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# ── data acquisition ─────────────────────────────────────────────────────────
def acquire(no_download):
    os.makedirs(CACHE, exist_ok=True)
    if not os.path.exists(ZIP_PATH):
        if no_download:
            sys.exit(f"FATAL: {ZIP_PATH} missing and --no-download given")
        print(f"downloading {ZIP_URL} → {ZIP_PATH}")
        urllib.request.urlretrieve(ZIP_URL, ZIP_PATH)
    zsha = sha256_file(ZIP_PATH)
    check("sha256 D1_HRIR_SOFA.zip == pinned", zsha == ZIP_SHA256, zsha)
    if zsha != ZIP_SHA256:
        sys.exit("FATAL: zip sha256 mismatch — refusing to read (R1)")
    if not os.path.exists(SOFA_48K):
        print(f"extracting {ZIP_PATH} → {os.path.dirname(SOFA_DIR)}")
        with zipfile.ZipFile(ZIP_PATH) as z:
            z.extractall(os.path.dirname(SOFA_DIR))
    ssha = sha256_file(SOFA_48K)
    check("sha256 D1_48K_24bit_256tap_FIR_SOFA.sofa == pinned", ssha == SOFA_48K_SHA256, ssha)
    if ssha != SOFA_48K_SHA256:
        sys.exit("FATAL: 48 k SOFA sha256 mismatch — refusing to read (R1)")
    others = {}
    for name, pinned in SOFA_OTHER.items():
        p = os.path.join(SOFA_DIR, name)
        others[name] = sha256_file(p) if os.path.exists(p) else "(not extracted)"
        if os.path.exists(p):
            check(f"sha256 {name} == pinned (provenance only)", others[name] == pinned, others[name])
    NUMBERS["zip_sha256"] = zsha
    NUMBERS["sofa48_sha256"] = ssha
    NUMBERS["sofa_other_sha256"] = others


# ── SH / MagLS (r2d_final.py, verbatim) ──────────────────────────────────────
def legendre(n, m, s_, c_):
    if n == 0:
        return np.ones_like(s_)
    if n == 1:
        return s_ if m == 0 else c_
    if n == 2:
        return [0.5 * (3 * s_ * s_ - 1), 3 * s_ * c_, 3 * c_ * c_][m]
    if n == 3:
        return [0.5 * s_ * (5 * s_ * s_ - 3), 1.5 * c_ * (5 * s_ * s_ - 1), 15 * s_ * c_ * c_, 15 * c_ * c_ * c_][m]


def sn3d_matrix(azr, elr, order):
    """ACN / SN3D / AmbiX real SH matrix (Q × (order+1)²) — the harness refSh16 table.
    T_m = sin(|m| a) for m < 0 (so Y_1^{-1} ∝ sin a: +y = left = az +90°)."""
    s_, c_ = np.sin(elr), np.cos(elr)
    cols = []
    for n in range(order + 1):
        for m in range(-n, n + 1):
            am = abs(m)
            N = np.sqrt((1.0 if m == 0 else 2.0) * factorial(n - am) / factorial(n + am))
            Tm = np.sin(am * azr) if m < 0 else (np.ones_like(azr) if m == 0 else np.cos(m * azr))
            cols.append(N * legendre(n, am, s_, c_) * Tm)
    return np.stack(cols, axis=1)


class Bank:
    """Holds the SOFA-derived spectra and the MagLS machinery for one dataset."""

    def __init__(self, ir, az, el):
        self.ir = ir
        self.az = az
        self.el = el
        self.Q = ir.shape[0]
        self.freqs = np.fft.rfftfreq(NFFT, 1 / FS)
        self.H = np.fft.rfft(ir, n=NFFT, axis=2)          # (Q, 2, nb)

    def magls(self, Ymat, f_a, blend="hard"):
        """Both ears. 'hard' = LS below k_a, magnitude-only above with the previous-bin
        phase + 'avg' Δφ (spaudiopy's SLICE-THEN-DIFF estimator) — C2.
        'target' = the safe target-domain half-octave blend (r2c_objective 'phasor')."""
        H, freqs = self.H, self.freqs
        Yp = np.linalg.pinv(Ymat)
        nsh = Ymat.shape[1]
        nb = len(freqs)
        k_a = int(np.argmin(np.abs(freqs - f_a)))
        out = np.zeros((2, nsh, nb), complex)
        for e in (0, 1):
            Hls = Yp @ H[:, e, :]
            o = Hls.copy()
            uw = np.unwrap(np.angle(Hls[0, :k_a]))
            dphi = np.mean(np.diff(uw[k_a - 6:k_a - 1]))          # slice THEN diff (4 values)
            if blend == "hard":
                for k in range(k_a, nb):
                    o[:, k] = Yp @ (np.abs(H[:, e, k]) * np.exp(1j * (np.angle(Ymat @ o[:, k - 1]) + dphi)))
            else:
                lr = np.log2(np.maximum(freqs, 1e-9) / f_a)
                alpha = np.where(lr < -0.5, 1.0, np.where(lr > 0.5, 0.0, np.cos(0.5 * np.pi * lr) ** 2))
                k_lo = int(np.argmin(np.abs(freqs - f_a / np.sqrt(2))))
                for k in range(k_lo, nb):
                    cont = np.exp(1j * (np.angle(Ymat @ o[:, k - 1]) + dphi))
                    meas = np.exp(1j * np.angle(H[:, e, k]))
                    mix = alpha[k] * meas + (1 - alpha[k]) * cont
                    mix = mix / np.maximum(np.abs(mix), 1e-12)
                    o[:, k] = Yp @ (np.abs(H[:, e, k]) * mix)
            out[e] = o
        return out, k_a

    def ls_only(self, Ymat):
        Yp = np.linalg.pinv(Ymat)
        return np.stack([Yp @ self.H[:, e, :] for e in (0, 1)])

    def fit_error_db(self, Ymat, hnm_spec, k_lo, k_hi):
        """RMS over directions × bins of the magnitude fit error in dB (r2c_objective)."""
        rec = np.einsum("qn,enk->eqk", Ymat, hnm_spec[:, :, k_lo:k_hi])
        meas = np.abs(self.H[:, :, k_lo:k_hi]).transpose(1, 0, 2)
        r = 20 * np.log10(np.maximum(np.abs(rec), 1e-9) / np.maximum(meas, 1e-9))
        return float(np.sqrt(np.mean(r ** 2)))


def trunc(hnm, T=0):
    """(2, nsh, 1024-IR spectrum) → (2, nsh, 256) taps [T, T+256) with spaudiopy's taper:
    last 32 × hanning(64)[32:], first 4 × hanning(8)[:4] (tap 0 becomes exactly 0)."""
    h = np.fft.irfft(hnm, n=NFFT, axis=2)[..., T:T + TAPS].copy()
    h[..., -32:] *= np.hanning(64)[32:]
    h[..., :4] *= np.hanning(8)[:4]
    return h


# ── measurements (R4 / R7 / R8 / C10) ────────────────────────────────────────
def a_weight(f):
    """IEC 61672 A-weighting magnitude, normalised to 1 at 1 kHz."""
    def ra(x):
        x2 = x ** 2
        return 12194 ** 2 * x2 ** 2 / ((x2 + 20.6 ** 2) * np.sqrt((x2 + 107.7 ** 2) * (x2 + 737.9 ** 2)) * (x2 + 12194 ** 2))
    return ra(f) / ra(1000.0)


def front_level(h_taps, order, weighted=True):
    """A-weighted RMS gain of white noise through the FRONT response of a (2, nsh, 256)
    bank, power-mean of the ears, DC excluded (R4 definition)."""
    freqs = np.fft.rfftfreq(NFFT, 1 / FS)
    A2 = a_weight(np.maximum(freqs, 1.0)) ** 2
    A2[0] = 0.0
    w = A2 if weighted else np.ones_like(A2)
    if not weighted:
        w[0] = 0.0
    spec = np.fft.rfft(h_taps, n=NFFT, axis=2)
    yv = sn3d_matrix(np.array([0.0]), np.array([0.0]), order)[0]
    rec = np.einsum("n,enk->ek", yv, spec)
    p = np.mean([np.sum(w * np.abs(rec[e]) ** 2) for e in (0, 1)])
    return float(np.sqrt(p / np.sum(w)))


def response_at(h_taps, order, az_deg, el_deg=0.0):
    yv = sn3d_matrix(np.array([np.deg2rad(az_deg)]), np.array([np.deg2rad(el_deg)]), order)[0]
    return np.einsum("n,ent->et", yv, h_taps)       # (2, taps)


def itd_lp_xcorr(L, R, fs, fc):
    """C10: brick-wall low-pass both ears at fc on a 4096-point FFT, then the lag d
    maximising Σ_n L[n]·R[n+d]. POSITIVE = R lags = L LEADS (the nearer ear at +90°)."""
    n = 4096
    FL, FR = np.fft.rfft(L, n), np.fft.rfft(R, n)
    f = np.fft.rfftfreq(n, 1 / fs)
    FL[f > fc] = 0.0
    FR[f > fc] = 0.0
    c = np.fft.irfft(np.conj(FL) * FR, n)
    d = int(np.argmax(c))
    return d - n if d > n // 2 else d


def ild_db(resp, lo=None, hi=None, fs=FS):
    """Broadband rms ratio L/R in dB, or the band-limited power ratio when lo/hi given."""
    if lo is None:
        return 20 * np.log10(np.sqrt(np.mean(resp[0] ** 2)) / np.sqrt(np.mean(resp[1] ** 2)))
    F = np.fft.rfft(resp, n=NFFT, axis=1)
    fr = np.fft.rfftfreq(NFFT, 1 / fs)
    m = (fr >= lo) & (fr < hi)
    return 10 * np.log10(np.sum(np.abs(F[0][m]) ** 2) / np.sum(np.abs(F[1][m]) ** 2))


def onset10(x):
    return int(np.argmax(np.abs(x) > 0.1 * np.abs(x).max()))


# ── emission (F9: %.9g + parse-back) ─────────────────────────────────────────
def fmt_f32(v):
    s = "%.9g" % float(np.float32(v))
    if not any(c in s for c in ".einN"):
        s += ".0"
    return s + "f"


def emit_array(f, name, arr, indent="    "):
    """arr: float32 [ch][ear][tap] → nested braces, 8 values per line.
    No digits in the inner comments: the parse-back strips // comments anyway."""
    nch, near, nt = arr.shape
    f.write(f"inline constexpr float {name}[{nch}][{near}][{nt}] =\n{{\n")
    for ch in range(nch):
        f.write(f"{indent}{{ // channel\n")
        for ear in range(near):
            f.write(f"{indent}    {{ // {'left' if ear == 0 else 'right'}\n")
            row = arr[ch, ear]
            for t0 in range(0, nt, 8):
                vals = ", ".join(fmt_f32(v) for v in row[t0:t0 + 8])
                f.write(f"{indent}        {vals}{',' if t0 + 8 < nt else ''}\n")
            f.write(f"{indent}    }}{',' if ear == 0 else ''}\n")
        f.write(f"{indent}}}{',' if ch < nch - 1 else ''}\n")
    f.write("};\n\n")


FLOAT_RE = re.compile(r"[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?")


def parse_array(text, name):
    """The parser the pedalboard measurer copies: find `name[...] =`, take the brace
    block up to `};`, strip // comments, read every float literal, float32."""
    m = re.search(re.escape(name) + r"\[(\d+)\]\[(\d+)\]\[(\d+)\]\s*=\s*\{", text)
    if m is None:
        raise ValueError(f"{name} not found")
    dims = tuple(int(d) for d in m.groups())
    start = m.end()
    end = text.index("};", start)
    body = re.sub(r"//[^\n]*", "", text[start:end])
    vals = np.array([np.float32(float(v)) for v in FLOAT_RE.findall(body)], dtype=np.float32)
    return vals.reshape(dims)


AGPL = """/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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
"""


# ── main ─────────────────────────────────────────────────────────────────────
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--trim", type=int, default=32, help="kBinauralFilterTrimSamples (C1)")
    ap.add_argument("--blend", choices=("hard", "target"), default="hard", help="C2: hard switch (default) or the target-domain blend")
    ap.add_argument("--no-download", action="store_true")
    args = ap.parse_args()
    T = args.trim
    t0 = time.time()

    import importlib.metadata as md
    import sofar
    import scipy
    import scipy.signal
    try:
        from spaudiopy import decoder, sig, sph
    except Exception as ex:   # F3: the oracle is mandatory
        sys.exit(f"FATAL: spaudiopy import failed ({ex}) — the oracle self-check cannot run; pin scipy<1.15")
    versions = {p: md.version(p) for p in ("sofar", "numpy", "scipy", "spaudiopy")}
    versions["python"] = sys.version.split()[0]
    NUMBERS["versions"] = versions
    print("resolved:", versions)

    print("== acquire ==")
    acquire(args.no_download)

    s = sofar.read_sofa(SOFA_48K, verbose=False)
    ir = np.asarray(s.Data_IR, dtype=np.float64)
    fs_file = float(np.squeeze(s.Data_SamplingRate))
    check("SOFA fs == 48000 and Data.IR shape (8802, 2, 256)", fs_file == FS and ir.shape == (8802, 2, 256),
          f"fs {fs_file:.0f}, shape {ir.shape}")
    az = np.deg2rad(s.SourcePosition[:, 0])
    el = np.deg2rad(s.SourcePosition[:, 1])
    NUMBERS["sofa_history"] = str(getattr(s, "GLOBAL_History", ""))
    NUMBERS["sofa_license"] = str(getattr(s, "GLOBAL_License", "")).split("\n")[0][:80]
    NUMBERS["sofa_num_dirs"] = int(ir.shape[0])
    bank = Bank(ir, az, el)
    freqs = bank.freqs

    # ── the design ──────────────────────────────────────────────────────────
    print(f"== MagLS per order (blend = {args.blend}, trim T = {T}) ==")
    own = {}        # order → (spec (2,nsh,nb), Y, k_a, f_a)
    for N in ORDERS:
        f_a = N * C_SOUND / (2 * np.pi * R_HEAD)
        Y = sn3d_matrix(az, el, N)
        spec, k_a = bank.magls(Y, f_a, args.blend)
        own[N] = (spec, Y, k_a, f_a)
        print(f"   order {N}: f_a {f_a:.1f} Hz → bin {k_a} ({freqs[k_a]:.0f} Hz)")
    NUMBERS["f_a"] = {N: own[N][3] for N in ORDERS}
    NUMBERS["k_a"] = {N: own[N][2] for N in ORDERS}

    # ── oracle self-check at T = 0, before trim / scale (C7) ────────────────
    print("== oracle: spaudiopy.decoder.magls_bin (hf_cont='avg') ==")
    hr = sig.HRIRs(ir[:, 0, :], ir[:, 1, :], az, np.pi / 2 - el, FS)
    oracle = {}
    for N in ORDERS:
        spec, Y, k_a, f_a = own[N]
        Ysp = sph.sh_matrix(N, az, np.pi / 2 - el, "real")
        own_sp, _ = bank.magls(Ysp, f_a, "hard")
        orc = decoder.magls_bin(hr, N, f_trans=f_a, hf_cont="avg")
        d1 = float(np.max(np.abs(trunc(own_sp, 0) - orc)))
        ratio = np.array([float(np.dot(Ysp[:, c], Y[:, c]) / np.dot(Y[:, c], Y[:, c])) for c in range(Y.shape[1])])
        d2 = float(np.max(np.abs(trunc(spec, 0) - orc * ratio[None, :, None]))) if args.blend == "hard" else float("nan")
        check(f"oracle order {N}: own loop on spaudiopy's N3D Y vs magls_bin, max|Δ| <= 1e-9", d1 <= 1e-9, f"{d1:.3e}")
        if args.blend == "hard":
            check(f"oracle order {N}: own SN3D bank vs magls_bin × per-order N3D→SN3D factor, max|Δ| <= 1e-9", d2 <= 1e-9,
                  f"{d2:.3e} (peak {np.abs(orc * ratio[None, :, None]).max():.3f}; factors {np.unique(np.round(ratio, 6))})")
        else:
            print(f"   info: --blend target → the tap null vs spaudiopy does not apply (own vs oracle structure differ by design); {d1:.3e} on the hard loop")
        oracle[N] = (orc, Ysp)
        NUMBERS.setdefault("oracle_null", {})[N] = (d1, d2)

    # ── objective table (R2) ────────────────────────────────────────────────
    print("== objective: magnitude fit error, RMS dB over 8802 dirs × bins, after truncation + taper ==")
    bands = [(50, 600), (600, 2000), (2000, 5000), (5000, 10000), (10000, 20000)]
    objective = {}
    for N in ORDERS:
        spec, Y, k_a, f_a = own[N]
        Oown = np.fft.rfft(trunc(spec, T), n=NFFT, axis=2)
        Ols = np.fft.rfft(trunc(bank.ls_only(Y), T), n=NFFT, axis=2)
        orc, Ysp = oracle[N]
        Osp = np.fft.rfft(orc, n=NFFT, axis=2)
        rows = []
        print(f"   order {N}: {'band':>12s} {'own':>8s} {'spaudiopy':>10s} {'LS-only':>8s}")
        for lo, hi in bands:
            klo, khi = int(np.argmin(np.abs(freqs - lo))), int(np.argmin(np.abs(freqs - hi)))
            r = (bank.fit_error_db(Y, Oown, klo, khi), bank.fit_error_db(Ysp, Osp, klo, khi), bank.fit_error_db(Y, Ols, klo, khi))
            rows.append((f"{lo}–{hi}", *r))
            print(f"            {lo:>5d}–{hi:<6d} {r[0]:8.3f} {r[1]:10.3f} {r[2]:8.3f}")
        objective[N] = rows
    NUMBERS["objective"] = objective

    # ── trim + taper + level match (C1 / R4) ────────────────────────────────
    print("== trim, taper, level match ==")
    h_raw = {N: trunc(own[N][0], T) for N in ORDERS}            # (2, nsh, 256), unscaled
    lvl = {N: front_level(h_raw[N], N) for N in ORDERS}
    lvl_u = {N: front_level(h_raw[N], N, weighted=False) for N in ORDERS}
    scale = STEREO_FRONT_LEVEL / lvl[3]
    level_db = 20 * np.log10(scale)
    h_scaled = {N: h_raw[N] * scale for N in ORDERS}
    after = {N: 20 * np.log10(front_level(h_scaled[N], N) / STEREO_FRONT_LEVEL) for N in ORDERS}
    print(f"   A-weighted white gain, front, before scale: " + ", ".join(f"order {N} {lvl[N]:.4f} (unweighted {lvl_u[N]:.4f})" for N in ORDERS))
    print(f"   common scale s = 0.75 / level3 = {scale:.6f} ({level_db:+.3f} dB)")
    check("level match order 3 front == stereo 0.75 within 1e-6 dB after the common scale", abs(after[3]) < 1e-6, f"{after[3]:+.2e} dB")
    check("level match orders 1 / 2 within ±1 dB of stereo after the common scale (expect −0.37 / −0.03)",
          abs(after[1]) <= 1.0 and abs(after[2]) <= 1.0, f"{after[1]:+.3f} / {after[2]:+.3f} dB")
    NUMBERS["level"] = {"before": lvl, "unweighted": lvl_u, "scale": scale, "scale_db": level_db, "after_db": after}

    # energy loss of the trim (R3)
    loss = {}
    for N in ORDERS:
        hfull = np.fft.irfft(own[N][0], n=NFFT, axis=2)
        etot = np.sum(hfull ** 2)
        loss[N] = (100 * np.sum(hfull[..., :T] ** 2) / etot, 100 * np.sum(hfull[..., T + TAPS:] ** 2) / etot)
        print(f"   trim loss order {N}: head {loss[N][0]:.4f} %, tail {loss[N][1]:.4f} %")
    NUMBERS["trim_loss_pct"] = loss

    # float32 banks in [ch][ear][tap] layout — THE artifact
    banks32 = {N: np.ascontiguousarray(h_scaled[N].transpose(1, 0, 2)).astype(np.float32) for N in ORDERS}
    finite = all(np.all(np.isfinite(b)) for b in banks32.values())
    check("no NaN / inf in the three banks", finite)
    tap0 = all(np.all(b[:, :, 0] == 0.0) for b in banks32.values())
    check("tap 0 of every FIR exactly 0 (taper)", tap0)
    peak_tap = {N: float(np.abs(banks32[N]).max()) for N in ORDERS}
    print("   peak |tap|: " + ", ".join(f"order {N} {peak_tap[N]:.4f}" for N in ORDERS))
    NUMBERS["peak_tap"] = peak_tap

    # ── checks on the float32 banks (what the engine will convolve) ─────────
    h32 = {N: banks32[N].astype(np.float64).transpose(1, 0, 2) for N in ORDERS}   # (2, nsh, 256)

    # W channel LF flatness (R4)
    W = np.abs(np.fft.rfft(h32[3][:, 0, :], n=NFFT, axis=1))
    k200 = int(np.argmin(np.abs(freqs - 200)))
    band = (freqs >= 50) & (freqs <= 200)
    wdev = [(float(20 * np.log10(W[e][band].max() / W[e][k200])), float(20 * np.log10(W[e][band].min() / W[e][k200]))) for e in (0, 1)]
    check("W channel (order 3) within ±1 dB of its 200 Hz value over 50–200 Hz, both ears",
          all(abs(a) <= 1.0 and abs(b) <= 1.0 for a, b in wdev), f"L {wdev[0][0]:+.2f}/{wdev[0][1]:+.2f} dB, R {wdev[1][0]:+.2f}/{wdev[1][1]:+.2f} dB")
    NUMBERS["w_flatness_db"] = wdev

    # ILD / ITD at ±90° (R8 / C10)
    ild = {}
    itd = {}
    ildb = {}
    for N in (1, 2, 3):
        f_a = own[N][3]
        fc = min(1500.0, 0.8 * f_a)
        for azd in (90.0, -90.0):
            r = response_at(h32[N], N, azd)
            ild[(N, azd)] = float(ild_db(r))
            itd[(N, azd)] = itd_lp_xcorr(r[0], r[1], FS, fc)
            ildb[(N, azd)] = [float(ild_db(r, lo, hi)) for lo, hi in ((200, 1000), (1000, 3000), (3000, 8000), (8000, 16000))]
        print(f"   order {N}: ILD +90 {ild[(N, 90.0)]:+.2f} dB / −90 {ild[(N, -90.0)]:+.2f} dB; per band +90 {['%+.1f' % v for v in ildb[(N, 90.0)]]}; "
              f"ITD (LP < {fc:.0f} Hz) +90 {itd[(N, 90.0)]:+d} / −90 {itd[(N, -90.0)]:+d} samples")
    for N in (1, 3):
        check(f"ILD order {N}: >= +6 dB at az +90 (left louder, AmbiX sign) and <= −6 dB at az −90",
              ild[(N, 90.0)] >= 6.0 and ild[(N, -90.0)] <= -6.0, f"{ild[(N, 90.0)]:+.2f} / {ild[(N, -90.0)]:+.2f} dB (expect +12.3 / −11.9 and +13.9 / −14.6)")
        check(f"ITD order {N} by low-passed xcorr: L leads at +90 by 25–45 samples, R leads at −90 by 25–45",
              25 <= itd[(N, 90.0)] <= 45 and -45 <= itd[(N, -90.0)] <= -25, f"{itd[(N, 90.0)]:+d} / {itd[(N, -90.0)]:+d} (expect ≈ +36 / −36, +37 / −37)")
    # sign anchors for the ITD helper: a synthetic delayed pair, and the raw SADIE az 90° HRIR (no pre-ringing: onsets L 73 / R 104)
    x = np.random.default_rng(1).standard_normal(1024)
    synth = itd_lp_xcorr(x, np.concatenate([np.zeros(17), x[:-17]]), FS, 1500.0)
    i90 = int(np.argmin(np.hypot(((s.SourcePosition[:, 0] - 90 + 180) % 360) - 180, s.SourcePosition[:, 1])))
    raw90 = itd_lp_xcorr(ir[i90, 0], ir[i90, 1], FS, 1500.0)
    check("ITD helper sign: R = L delayed by 17 reads +17; raw SADIE az 90° HRIR reads +25…+45 (L leads)",
          synth == 17 and 25 <= raw90 <= 45, f"synthetic {synth:+d}, raw SADIE {raw90:+d} (onset difference 31)")
    NUMBERS["ild_db"] = {f"{N}@{int(a):+d}": v for (N, a), v in ild.items()}
    NUMBERS["ild_band_db"] = {f"{N}@{int(a):+d}": v for (N, a), v in ildb.items()}
    NUMBERS["itd_samples"] = {f"{N}@{int(a):+d}": v for (N, a), v in itd.items()}

    # onsets (C4 / F6): front, 10 % of own peak, through the float32 bank
    onsets = [0]
    onsets_r = [0]
    for N in ORDERS:
        rf = response_at(h32[N], N, 0.0)
        onsets.append(onset10(rf[0]))
        onsets_r.append(onset10(rf[1]))
    print(f"   front onsets (10 %) L {onsets[1:]} R {onsets_r[1:]} (expect L 84 / 70 / 50 at T = 32)")
    check("front onset constants > 0 and < 128 for every order", all(0 < o < 128 for o in onsets[1:]), str(onsets))
    NUMBERS["onsets_front_L"] = onsets
    NUMBERS["onsets_front_R"] = onsets_r

    # headroom (F7): max |H| over all directions × bins at order 3 after the scale
    spec3 = np.fft.rfft(h32[3], n=NFFT, axis=2)
    Y3 = own[3][1]
    hmax, hmax_k = 0.0, 0
    for q0 in range(0, Y3.shape[0], 1000):
        rec = np.abs(np.einsum("qn,enk->eqk", Y3[q0:q0 + 1000], spec3))
        i = np.unravel_index(np.argmax(rec), rec.shape)
        if rec[i] > hmax:
            hmax, hmax_k = float(rec[i]), int(i[2])
    print(f"   headroom: max |H| over directions × bins at order 3 = {hmax:.3f} ({20 * np.log10(hmax):+.2f} dB) at {freqs[hmax_k]:.0f} Hz (expect +6.5 dB at 9.6 kHz)")
    NUMBERS["headroom"] = (hmax, 20 * np.log10(hmax), float(freqs[hmax_k]))

    # ── goldens (C12): scipy resample_poly of the float32 header filters ────
    print("== goldens: scipy.signal.resample_poly (defaults) × down/up — DC-gain preserving ==")
    # resample_poly preserves the WAVEFORM amplitude (Σ h scales by up/down), which for a FILTER
    # means |H| doubles at 96 k. A filter moved to a new rate must keep its frequency response, so
    # the golden is scaled by down/up (= 48000/fs). The engine's delta calibration (dividing by
    # the resampled delta's sum = kernel area / ratio) lands on the same normalisation (C3).
    gold = {}
    for fs_out, (up, down), L in ((44100, (147, 160), 256), (96000, (2, 1), 512)):
        for N in ORDERS:
            src = banks32[N].astype(np.float64)
            out = np.zeros((src.shape[0], 2, L), np.float32)
            for ch in range(src.shape[0]):
                for ear in range(2):
                    g = scipy.signal.resample_poly(src[ch, ear], up, down) * (down / up)
                    n = min(L, g.size)
                    out[ch, ear, :n] = g[:n].astype(np.float32)
            gold[(fs_out, N)] = out
        # self-check: DC gain preserved (Σ taps within 0.5 % of the 48 k bank, order 3 W left)
        dc48 = float(banks32[3][0, 0].astype(np.float64).sum())
        dcr = float(gold[(fs_out, 3)][0, 0].astype(np.float64).sum())
        check(f"golden {fs_out}: DC gain of order-3 W/left preserved (Σ taps within 0.5 % of 48 k)", abs(dcr / dc48 - 1.0) < 5e-3, f"{dcr:.5f} vs {dc48:.5f}")
        print(f"   fs {fs_out}: up/down {up}/{down}, {L} taps ({scipy.signal.resample_poly(np.zeros(256), up, down).size} native), scale {down / up:.6f}")

    # ── abort on any FAIL before emitting (C7) ───────────────────────────────
    fails = [c for c in CHECKS if not c[1]]
    if fails:
        print(f"FAILED self-checks ({len(fails)}): nothing emitted")
        for n, _, d in fails:
            print(f"   - {n}: {d}")
        sys.exit(1)

    # ── emit the plugin header ──────────────────────────────────────────────
    print("== emit ==")
    regen = 'uv run --python 3.12 --with "sofar==1.3.0,numpy,scipy<1.15,spaudiopy" scripts/generate_binaural_filters.py'
    with open(HEADER_OUT, "w") as f:
        f.write(AGPL)
        f.write("/*\n  ==============================================================================\n\n")
        f.write("    BinauralFilterData — GENERATED, do not edit. Regenerate with\n")
        f.write(f"      {regen}\n\n")
        f.write("    SADIE II D1 (Neumann KU100) MagLS binaural decoding filters, orders 1 / 2 / 3,\n")
        f.write("    ACN / SN3D / AmbiX (+y = left = azimuth +90°), ear 0 = left, 48 kHz, 256 taps.\n")
        f.write("    Layout: kMagLsFiltersN[(N+1)²][2][256]; the engine resamples at fs ≠ 48 k\n")
        f.write("    (BinauralConvolver::prepare, DC-calibrated WindowedSinc — PLAN-2.3 C3).\n\n")
        f.write("    Provenance (scripts/FILTERS.md has every number):\n")
        f.write(f"      source     Zenodo record {ZENODO_RECORD}, D1_HRIR_SOFA.zip → D1_48K_24bit_256tap_FIR_SOFA.sofa\n")
        f.write(f"      sha256     zip {ZIP_SHA256}\n                 sofa {SOFA_48K_SHA256}\n")
        f.write(f"      license    {NUMBERS['sofa_license']}\n")
        f.write(f"      history    {NUMBERS['sofa_history']}\n")
        f.write(f"      packages   " + ", ".join(f"{k} {v}" for k, v in versions.items()) + "\n")
        f.write(f"      design     MagLS: LS below f_a = N·c/(2π·0.0875 m), magnitude-only above with the\n")
        f.write(f"                 previous-bin phase + 'avg' Δφ continuation ({'HARD SWITCH, no half-octave blend — C2' if args.blend == 'hard' else 'target-domain half-octave blend'});\n")
        f.write(f"                 1024-point design grid, k_a = {NUMBERS['k_a']}; trim T = {T} (taps [{T}, {T + TAPS}) of the 1024-point IR),\n")
        f.write(f"                 taper = spaudiopy's (last 32 × hanning(64)[32:], first 4 × hanning(8)[:4]).\n")
        f.write(f"      level      common scale s = 0.75 / A-weighted-white front gain at order 3 = {scale:.6f} ({level_db:+.3f} dB)\n")
        f.write(f"                 → orders 1 / 2 / 3 vs the stereo pair's 0.75: {after[1]:+.3f} / {after[2]:+.3f} / {after[3]:+.1e} dB\n")
        f.write(f"      onsets     front, left ear, first sample ≥ 10 % of peak: {onsets[1:]} (orders 1 / 2 / 3)\n")
        f.write(f"      headroom   max |H| over directions × bins at order 3 = {20 * np.log10(hmax):+.2f} dB at {freqs[hmax_k]:.0f} Hz (F7)\n")
        f.write("\n  ==============================================================================\n*/\n\n")
        f.write("#pragma once\n\nnamespace OSimpleAmbisonics::dsp\n{\n\n")
        f.write(f"inline constexpr int   kBinauralFilterSampleRate   = {int(FS)};\n")
        f.write(f"inline constexpr int   kBinauralFilterTaps         = {TAPS};\n")
        f.write(f"inline constexpr int   kBinauralFilterTrimSamples  = {T};\n")
        f.write(f"inline constexpr int   kBinauralOnsetFrontSamples[4] = {{ {onsets[0]}, {onsets[1]}, {onsets[2]}, {onsets[3]} }};   // front source, left ear, 10 % of peak (C4)\n")
        f.write(f"inline constexpr float kBinauralLevelMatchDb       = {fmt_f32(level_db)};   // baked into the filters (D2); kStereoLevelMatch stays 1.0f\n")
        f.write("inline constexpr float kBinauralAliasingHz[4]      = { 0.0f, " + ", ".join(fmt_f32(own[N][3]) for N in ORDERS) + " };   // f_a = N·343/(2π·0.0875)\n")
        f.write("inline constexpr bool  kBinauralFilterDfe          = true;   // diffuse-field equalised AS SHIPPED by SADIE II (GLOBAL_History); no own EQ\n")
        f.write(f'inline constexpr const char* kBinauralFilterSource  = "SADIE II D1 (KU100), Zenodo record {ZENODO_RECORD}, D1_48K_24bit_256tap_FIR_SOFA.sofa";\n')
        f.write(f'inline constexpr const char* kBinauralFilterLicense = "Apache-2.0";\n')
        f.write(f'inline constexpr const char* kBinauralFilterSha256  = "{SOFA_48K_SHA256}";\n')
        f.write(f'inline constexpr const char* kBinauralFilterBlend   = "{"hard-switch+avg-continuation" if args.blend == "hard" else "target-domain-half-octave+avg-continuation"}";\n\n')
        for N in ORDERS:
            emit_array(f, f"kMagLsFilters{N}", banks32[N])
        f.write("} // namespace OSimpleAmbisonics::dsp\n")

    # parse-back (F9)
    text = open(HEADER_OUT).read()
    pb = all(np.array_equal(parse_array(text, f"kMagLsFilters{N}"), banks32[N]) for N in ORDERS)
    check("header parse-back bit-equal to the float32 banks (F9)", pb)
    if not pb:
        sys.exit("FATAL: parse-back mismatch")

    # ── emit the goldens header (test-only) ─────────────────────────────────
    with open(GOLDENS_OUT, "w") as f:
        f.write(AGPL)
        f.write("// BinauralFilterGoldens — GENERATED, test-only — never in the plugin sources.\n")
        f.write(f"// Regenerate with {regen}\n")
        f.write("// scipy.signal.resample_poly (defaults: Kaiser β = 5) of the float32 header filters\n")
        f.write("// (kMagLsFiltersN, trimmed + tapered + scaled), SCALED BY down/up so the filter's\n")
        f.write("// frequency response (DC gain) is preserved — resample_poly alone preserves the waveform\n")
        f.write("// amplitude, which would read +6 dB at 96 k: 44.1 k = resample_poly (h, 147, 160) · 160/147,\n")
        f.write("// 236 samples zero-padded to 256; 96 k = resample_poly (h, 2, 1) · 1/2, 512. Zero-phase\n")
        f.write("// aligned: sample i ↔ engine table sample i (PLAN-2.3 C12). Gate tolerances in C3.\n")
        f.write(f"// scipy {versions['scipy']}, numpy {versions['numpy']}.\n\n#pragma once\n\nnamespace OSimpleAmbisonics::test\n{{\n\n")
        for fs_out in (44100, 96000):
            for N in ORDERS:
                emit_array(f, f"kBinauralGolden{fs_out}_{N}", gold[(fs_out, N)])
        f.write("} // namespace OSimpleAmbisonics::test\n")
    gtext = open(GOLDENS_OUT).read()
    pbg = all(np.array_equal(parse_array(gtext, f"kBinauralGolden{fs_out}_{N}"), gold[(fs_out, N)]) for fs_out in (44100, 96000) for N in ORDERS)
    check("goldens header parse-back bit-equal", pbg)

    header_sha = sha256_file(HEADER_OUT)
    goldens_sha = sha256_file(GOLDENS_OUT)

    # ── npz ─────────────────────────────────────────────────────────────────
    np.savez(NPZ_OUT, h1=banks32[1], h2=banks32[2], h3=banks32[3],
             g44100_1=gold[(44100, 1)], g44100_2=gold[(44100, 2)], g44100_3=gold[(44100, 3)],
             g96000_1=gold[(96000, 1)], g96000_2=gold[(96000, 2)], g96000_3=gold[(96000, 3)],
             meta=json.dumps(NUMBERS, default=lambda o: o.tolist() if hasattr(o, "tolist") else str(o)))
    npz_sha = sha256_file(NPZ_OUT)

    # ── FILTERS.md ──────────────────────────────────────────────────────────
    with open(FILTERS_MD, "w") as f:
        f.write("# O-simpleAmbisonics — binaural filter bank provenance (GENERATED by scripts/generate_binaural_filters.py)\n\n")
        f.write(f"Generated {time.strftime('%Y-%m-%d %H:%M:%S')} in {time.time() - t0:.1f} s on {sys.platform}.\n\n")
        f.write("## Source data\n\n")
        f.write(f"- SADIE II D1 (Neumann KU100), Zenodo record {ZENODO_RECORD}: `{ZIP_URL}`\n")
        f.write(f"- zip sha256 `{ZIP_SHA256}`\n")
        f.write(f"- `D1_48K_24bit_256tap_FIR_SOFA.sofa` sha256 `{SOFA_48K_SHA256}` — **the generator's input** ({NUMBERS['sofa_num_dirs']} directions, 256 taps, 48 kHz)\n")
        for name, pinned in SOFA_OTHER.items():
            f.write(f"- `{name}` sha256 `{pinned}` (provenance only, never read)\n")
        f.write(f"- License: {NUMBERS['sofa_license']}\n- GLOBAL_History: {NUMBERS['sofa_history']}\n")
        f.write("- DFE: as shipped by SADIE II (GLOBAL_History) — no own diffuse-field equalisation (R4)\n\n")
        f.write("## Environment (pinned)\n\n")
        f.write(f"`{regen}`\n\nResolved: " + ", ".join(f"{k} {v}" for k, v in versions.items()) + "\n\n")
        f.write("## Design choices\n\n")
        f.write(f"- **Trim T = {T}** (C1): taps [{T}, {T + TAPS}) of the 1024-point IR, spaudiopy's taper. Energy lost: " +
                "; ".join(f"order {N} head {loss[N][0]:.4f} % / tail {loss[N][1]:.4f} %" for N in ORDERS) + "\n")
        f.write(f"- **Blend: {args.blend}** (C2). " + ("Hard switch at k_a with the 'avg' phase continuation (spaudiopy's slice-then-diff Δφ estimator), NO half-octave blend. "
                "RESEARCH-2.3 R2 measured the literal complex blend of the LS and MagLS solutions CANCELLING — |H| moves by up to 27.6 dB at order 3 inside [f_a/2, 2f_a] "
                "(2.6 / 4.1 dB at orders 1 / 2) — and the safe target-domain blend costing 0.3–0.6 dB of magnitude fit in the transition band for a phase benefit no gate measures. "
                "This is a recorded WORDING DEVIATION from ARCHITECTURE / ROADMAP 'half-octave blend'; `--blend target` is the one-flag alternative."
                if args.blend == "hard" else "Target-domain half-octave blend (unit phasors of measured and continued phase interpolated, renormalised, one LS fit per bin).") + "\n")
        f.write(f"- f_a = N·343/(2π·0.0875): " + ", ".join(f"order {N} {own[N][3]:.1f} Hz → bin {own[N][2]}" for N in ORDERS) + "\n")
        f.write(f"- Level match (D2 / R4): common scale s = 0.75 / A-weighted-white front gain at order 3 = {scale:.6f} ({level_db:+.3f} dB), baked into all three banks; `kStereoLevelMatch` stays 1.0f.\n\n")
        f.write("## Self-checks (every line PASS, else nothing is emitted)\n\n")
        for n, ok, d in CHECKS:
            f.write(f"- [{'PASS' if ok else 'FAIL'}] {n}" + (f" — {d}" if d else "") + "\n")
        f.write("\n## Numbers\n\n")
        f.write("### Objective (magnitude fit error, RMS dB over 8802 directions × bins, after truncation + taper)\n\n")
        for N in ORDERS:
            f.write(f"| order {N} band | own | spaudiopy | LS-only |\n|---|---|---|---|\n")
            for row in objective[N]:
                f.write(f"| {row[0]} Hz | {row[1]:.3f} | {row[2]:.3f} | {row[3]:.3f} |\n")
            f.write("\n")
        f.write("### Level match\n\n| | order 1 | order 2 | order 3 |\n|---|---|---|---|\n")
        f.write("| A-weighted white gain, front, before scale | " + " | ".join(f"{lvl[N]:.4f}" for N in ORDERS) + " |\n")
        f.write("| unweighted | " + " | ".join(f"{lvl_u[N]:.4f}" for N in ORDERS) + " |\n")
        f.write("| vs stereo 0.75 after the common scale | " + " | ".join(f"{after[N]:+.3f} dB" for N in ORDERS) + " |\n\n")
        f.write(f"### W channel LF flatness (order 3, re 200 Hz, 50–200 Hz): L {wdev[0][0]:+.2f}/{wdev[0][1]:+.2f} dB, R {wdev[1][0]:+.2f}/{wdev[1][1]:+.2f} dB\n\n")
        f.write("### ILD / ITD through the float32 banks (el 0)\n\n| order | ILD +90 | ILD −90 | per band +90 (200–1 k / 1–3 k / 3–8 k / 8–16 k) | ITD +90 | ITD −90 | LP cutoff |\n|---|---|---|---|---|---|---|\n")
        for N in (1, 2, 3):
            fc = min(1500.0, 0.8 * own[N][3])
            f.write(f"| {N} | {ild[(N, 90.0)]:+.2f} dB | {ild[(N, -90.0)]:+.2f} dB | {' / '.join('%+.1f' % v for v in ildb[(N, 90.0)])} | {itd[(N, 90.0)]:+d} | {itd[(N, -90.0)]:+d} | {fc:.0f} Hz |\n")
        f.write("\nITD = lag maximising Σ L[n]·R[n+d] after a brick-wall low-pass on a 4096-point FFT (C10); positive = L leads.\n\n")
        f.write(f"### Onsets (front, 10 % of own peak): L {onsets[1:]}, R {onsets_r[1:]} → `kBinauralOnsetFrontSamples = {{ {', '.join(str(o) for o in onsets)} }}`\n\n")
        f.write(f"### Headroom: max |H| at order 3 = {hmax:.3f} ({20 * np.log10(hmax):+.2f} dB) at {freqs[hmax_k]:.0f} Hz; peak |tap| " + ", ".join(f"order {N} {peak_tap[N]:.4f}" for N in ORDERS) + "\n\n")
        f.write("## Artifacts\n\n")
        f.write(f"- `Source/dsp/BinauralFilterData.h` sha256 `{header_sha}`\n")
        f.write(f"- `tests/render-harness/BinauralFilterGoldens.h` sha256 `{goldens_sha}`\n")
        f.write(f"- `.cache/binaural_filters.npz` sha256 `{npz_sha}` (gitignored)\n\n")
        f.write("## Regeneration note (F4)\n\n")
        f.write("The MagLS phase recursion amplifies roundoff, so MagLS taps are not unique: byte-identical regeneration is a same-machine / same-BLAS property. "
                "On this machine with the pinned environment the header reproduces byte-identically (compare the sha256 above). Elsewhere, compare on the objective table "
                "and the oracle null (≤ 1e-9 against spaudiopy), not on bytes. The committed header is the canonical artifact.\n")

    print(f"wrote {HEADER_OUT} (sha256 {header_sha})")
    print(f"wrote {GOLDENS_OUT} (sha256 {goldens_sha})")
    print(f"wrote {NPZ_OUT} (sha256 {npz_sha})")
    print(f"wrote {FILTERS_MD}")
    fails = [c for c in CHECKS if not c[1]]
    print(f"{'ALL PASS' if not fails else 'FAILED'} ({len(CHECKS)} self-checks, {len(fails)} failure(s)) in {time.time() - t0:.1f} s")
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main())
