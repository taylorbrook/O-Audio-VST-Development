#!/usr/bin/env python3
"""
Offline prototype: 3D geometry -> single-cycle wavetable frames.

  * procedural meshes (sphere, torus, twisted star extrusion, noise-displaced sphere)
  * optional OBJ loader (triangulates fans)
  * plane/mesh slicing -> segments -> closed loops
  * loop -> 2048-sample cycle: radial unwrap r(theta) and arc-length unwrap (x(s), y(s), d(s))
  * frame coherence: start-point alignment (fixed ray, cross-correlation)
  * DC removal + normalisation; harmonic census
  * SDF / 3D-noise volume sampled along a torus-knot orbit
  * Serum-style WAV export + stacked-frame PNG

Everything is numpy; scipy only for the WAV writer; matplotlib for the PNG.
"""
import sys, time, math, os
import numpy as np

N = 2048                # samples per frame (WavetableData::kTableSize)
OUT = os.path.dirname(os.path.abspath(__file__))
rng = np.random.default_rng(1234)

# ----------------------------------------------------------------------------
# 1. Meshes
# ----------------------------------------------------------------------------
def grid_to_tris(V, nu, nv, wrap_u=True, wrap_v=True):
    """V is (nu*nv,3) laid out row-major (u major). Returns (T,3) int triangle list."""
    tris = []
    for i in range(nu if wrap_u else nu - 1):
        i2 = (i + 1) % nu
        for j in range(nv if wrap_v else nv - 1):
            j2 = (j + 1) % nv
            a, b, c, d = i * nv + j, i2 * nv + j, i2 * nv + j2, i * nv + j2
            tris.append((a, b, c)); tris.append((a, c, d))
    return np.asarray(tris, dtype=np.int64)

def mesh_sphere(nu=64, nv=32, r=1.0):
    th = np.linspace(0, 2 * np.pi, nu, endpoint=False)
    ph = np.linspace(0, np.pi, nv)                      # poles included (degenerate tris ok)
    T, P = np.meshgrid(th, ph, indexing="ij")
    V = np.stack([r * np.sin(P) * np.cos(T), r * np.sin(P) * np.sin(T), r * np.cos(P)], -1).reshape(-1, 3)
    return V, grid_to_tris(V, nu, nv, wrap_u=True, wrap_v=False)

def mesh_torus(nu=96, nv=48, R=1.0, r=0.4):
    th = np.linspace(0, 2 * np.pi, nu, endpoint=False)
    ph = np.linspace(0, 2 * np.pi, nv, endpoint=False)
    T, P = np.meshgrid(th, ph, indexing="ij")
    V = np.stack([(R + r * np.cos(P)) * np.cos(T), (R + r * np.cos(P)) * np.sin(T), r * np.sin(P)], -1).reshape(-1, 3)
    return V, grid_to_tris(V, nu, nv)

def mesh_twisted_star(nu=128, nz=64, points=5, inner=0.45, outer=1.0, twist=np.pi, height=2.0):
    """Star cross-section extruded along z with twist; caps closed by a centre vertex fan.
    Star with inner/outer = 0.45 is NON-star-shaped w.r.t. the centroid? No: a regular star polygon
    IS star-shaped about its centre. So we also offset the star centre sideways (crescent-like)
    to make radial unwrap fail: an off-centre notch is added."""
    th = np.linspace(0, 2 * np.pi, nu, endpoint=False)
    zs = np.linspace(-height / 2, height / 2, nz)
    Vs = []
    for k, z in enumerate(zs):
        rot = twist * (k / (nz - 1))
        rad = inner + (outer - inner) * (0.5 + 0.5 * np.cos(points * th)) ** 2
        # deep notch that makes the contour non-star-shaped about its centroid
        notch = np.exp(-((np.mod(th - 0.3, 2 * np.pi) - np.pi) ** 2) / 0.02)
        rad = rad * (1 - 0.7 * notch)
        x, y = rad * np.cos(th + rot), rad * np.sin(th + rot)
        Vs.append(np.stack([x, y, np.full_like(x, z)], -1))
    V = np.concatenate(Vs, 0)             # index = k*nu + i
    tris = []
    for k in range(nz - 1):
        for i in range(nu):
            i2 = (i + 1) % nu
            a, b, c, d = k * nu + i, k * nu + i2, (k + 1) * nu + i2, (k + 1) * nu + i
            tris.append((a, b, c)); tris.append((a, c, d))
    # caps
    cb = len(V); ct = cb + 1
    V = np.concatenate([V, [[0, 0, zs[0]], [0, 0, zs[-1]]]], 0)
    for i in range(nu):
        i2 = (i + 1) % nu
        tris.append((cb, i2, i))
        tris.append((ct, (nz - 1) * nu + i, (nz - 1) * nu + i2))
    return V, np.asarray(tris, dtype=np.int64)

# --- 3D value noise (fBm), numpy only; stands in for Perlin/simplex -----------
_perm = rng.permutation(256)
_perm = np.concatenate([_perm, _perm])
def _hash3(ix, iy, iz):
    return _perm[(_perm[(_perm[ix & 255] + (iy & 255)) & 255] + (iz & 255)) & 255] / 255.0 * 2 - 1
def value_noise3(p):
    p = np.asarray(p, dtype=np.float64)
    i = np.floor(p).astype(np.int64); f = p - i
    f = f * f * (3 - 2 * f)   # smoothstep
    out = 0.0
    for dz in (0, 1):
        for dy in (0, 1):
            for dx in (0, 1):
                w = (f[..., 0] if dx else 1 - f[..., 0]) * (f[..., 1] if dy else 1 - f[..., 1]) * (f[..., 2] if dz else 1 - f[..., 2])
                out = out + w * _hash3(i[..., 0] + dx, i[..., 1] + dy, i[..., 2] + dz)
    return out
def fbm3(p, octaves=4, lac=2.0, gain=0.5):
    a, s, tot = 1.0, 0.0, 0.0
    p = np.asarray(p, dtype=np.float64)
    for _ in range(octaves):
        s = s + a * value_noise3(p); tot += a; p = p * lac + 17.3; a *= gain
    return s / tot

def mesh_noisy_sphere(nu=96, nv=48, amp=0.25, freq=2.0):
    V, T = mesh_sphere(nu, nv, 1.0)
    d = 1 + amp * fbm3(V * freq)
    return V * d[:, None], T

def mesh_crescent(nu=160, nz=48, height=2.0):
    """Extruded crescent (disc minus offset disc). Centroid lies OUTSIDE the material -> not star-shaped."""
    th = np.linspace(0, 2 * np.pi, nu, endpoint=False)
    # outer arc from -a..a, inner arc back
    a = 2.4
    n1 = nu // 2; n2 = nu - n1
    t1 = np.linspace(-a, a, n1)
    outer = np.stack([np.cos(t1), np.sin(t1)], -1)
    t2 = np.linspace(a, -a, n2)
    # inner circle centred at (0.55,0), radius chosen so the tips meet the outer arc
    cx = 0.55; ri = np.hypot(np.cos(a) - cx, np.sin(a))
    ang2 = np.arctan2(np.sin(a), np.cos(a) - cx)
    t2 = np.linspace(ang2, -ang2, n2)
    inner = np.stack([cx + ri * np.cos(t2), ri * np.sin(t2)], -1)
    prof = np.concatenate([outer, inner], 0)
    zs = np.linspace(-height / 2, height / 2, nz)
    Vs = []
    for k, z in enumerate(zs):
        sc = 1.0 + 0.3 * np.sin(np.pi * k / (nz - 1))     # gentle bulge so slices differ
        Vs.append(np.concatenate([prof * sc, np.full((nu, 1), z)], 1))
    V = np.concatenate(Vs, 0)
    tris = []
    for k in range(nz - 1):
        for i in range(nu):
            i2 = (i + 1) % nu
            a_, b, c, d = k * nu + i, k * nu + i2, (k + 1) * nu + i2, (k + 1) * nu + i
            tris.append((a_, b, c)); tris.append((a_, c, d))
    # caps: fan from a vertex ON the profile (crescent is not star-shaped about its centre, but a fan from
    # the tip vertex 0 is still a valid triangulation for slicing purposes since caps are never sliced)
    for i in range(1, nu - 1):
        tris.append((0, i + 1, i))
        tris.append(((nz - 1) * nu, (nz - 1) * nu + i, (nz - 1) * nu + i + 1))
    return V, np.asarray(tris, dtype=np.int64)

def load_obj(path):
    V, F = [], []
    with open(path) as fh:
        for ln in fh:
            if ln.startswith("v "):
                V.append([float(t) for t in ln.split()[1:4]])
            elif ln.startswith("f "):
                idx = [int(t.split("/")[0]) - 1 for t in ln.split()[1:]]
                for k in range(1, len(idx) - 1):
                    F.append((idx[0], idx[k], idx[k + 1]))
    return np.asarray(V, float), np.asarray(F, np.int64)

# ----------------------------------------------------------------------------
# 2. Slicing: plane z = h  ->  segments -> closed loops
# ----------------------------------------------------------------------------
def slice_mesh(V, T, h, eps=1e-9):
    """Vectorised edge/plane intersection. Returns list of (M,2) closed loops (2D, last != first)."""
    z = V[:, 2] - h
    # nudge vertices exactly on the plane (classic slicer trick: avoid vertex-on-plane cases)
    z = np.where(np.abs(z) < eps, eps, z)
    tz = z[T]                                            # (T,3)
    s = np.sign(tz)
    cross = ~((s[:, 0] == s[:, 1]) & (s[:, 1] == s[:, 2]))
    tri = T[cross]; tzc = tz[cross]
    if len(tri) == 0:
        return []
    segs = np.empty((len(tri), 2, 2)); keys = np.empty((len(tri), 2, 2), dtype=np.int64)
    fill = np.zeros(len(tri), dtype=np.int64)
    for a, b in ((0, 1), (1, 2), (2, 0)):
        za, zb = tzc[:, a], tzc[:, b]
        m = (za * zb) < 0
        t = za[m] / (za[m] - zb[m])
        pa, pb = V[tri[m, a]], V[tri[m, b]]
        P = pa + (pb - pa) * t[:, None]
        rows = np.nonzero(m)[0]
        col = fill[rows]
        segs[rows, col] = P[:, :2]
        ia, ib = tri[m, a], tri[m, b]
        keys[rows, col] = np.stack([np.minimum(ia, ib), np.maximum(ia, ib)], -1)   # edge id (shared by 2 tris)
        fill[rows] += 1
    assert np.all(fill == 2)
    # chain: each edge key appears in exactly two segments on a closed manifold
    kflat = keys.reshape(-1, 2)
    uniq, inv = np.unique(kflat, axis=0, return_inverse=True)
    inv = inv.reshape(-1, 2)                              # segment -> (node a, node b)
    nseg = len(inv); nnode = len(uniq)
    adj = [[] for _ in range(nnode)]
    for si, (a, b) in enumerate(inv):
        adj[a].append(si); adj[b].append(si)
    used = np.zeros(nseg, bool); loops = []
    for s0 in range(nseg):
        if used[s0]:
            continue
        loop_nodes = []
        si = s0; node = inv[si, 0]; start = node
        while True:
            used[si] = True
            loop_nodes.append(node)
            other = inv[si, 1] if inv[si, 0] == node else inv[si, 0]
            node = other
            if node == start:
                break
            nxt = [x for x in adj[node] if not used[x]]
            if not nxt:
                break            # open chain (non-manifold input) – keep what we have
            si = nxt[0]
        pts = np.array([segs.reshape(-1, 2)[np.argmax(inv.reshape(-1) == n)] for n in loop_nodes])
        if len(pts) >= 3:
            loops.append(pts)
    return loops

def loop_area(P):
    x, y = P[:, 0], P[:, 1]
    return 0.5 * np.sum(x * np.roll(y, -1) - np.roll(x, -1) * y)

def orient_ccw(P):
    return P if loop_area(P) > 0 else P[::-1].copy()

# ----------------------------------------------------------------------------
# 3. Loop -> single cycle
# ----------------------------------------------------------------------------
def resample_arclength(P, n=N):
    """Uniform arc-length resampling of a closed polyline (returns n points, closed implicitly)."""
    Q = np.vstack([P, P[:1]])
    seg = np.linalg.norm(np.diff(Q, axis=0), axis=1)
    s = np.concatenate([[0], np.cumsum(seg)])
    L = s[-1]
    t = np.linspace(0, L, n, endpoint=False)
    x = np.interp(t, s, Q[:, 0]); y = np.interp(t, s, Q[:, 1])
    return np.stack([x, y], -1), L

def centroid_area(P):
    """Area-weighted centroid of a simple polygon."""
    x, y = P[:, 0], P[:, 1]; x1, y1 = np.roll(x, -1), np.roll(y, -1)
    c = x * y1 - x1 * y; A = 0.5 * c.sum()
    if abs(A) < 1e-12:
        return P.mean(0)
    return np.array([((x + x1) * c).sum(), ((y + y1) * c).sum()]) / (6 * A)

def unwrap_radial(P, n=N):
    """r(theta) about the centroid, theta uniform on [0,2pi). Returns (wave, is_star_shaped)."""
    c = centroid_area(P); Q = P - c
    th = np.arctan2(Q[:, 1], Q[:, 0]); r = np.hypot(Q[:, 0], Q[:, 1])
    # star-shaped test: theta must be monotone around the loop (after CCW orientation)
    dth = np.diff(np.unwrap(np.concatenate([th, th[:1]])))
    star = bool(np.all(dth > 0)) and abs(dth.sum() - 2 * np.pi) < 1e-6
    thu = np.unwrap(th); thu -= thu[0]
    if not star:
        # multi-valued: emulate what a naive implementation does – ray-cast, take the FIRST hit
        # (dense dense sampling → nearest theta bin, keeps max r) — documents the failure
        grid = np.linspace(0, 2 * np.pi, n, endpoint=False)
        Qd, _ = resample_arclength(P, 8 * n); Qd = Qd - c
        thd = np.mod(np.arctan2(Qd[:, 1], Qd[:, 0]), 2 * np.pi); rd = np.hypot(Qd[:, 0], Qd[:, 1])
        b = np.minimum((thd / (2 * np.pi) * n).astype(int), n - 1)
        out = np.zeros(n); np.maximum.at(out, b, rd)
        # fill empty bins
        empty = out == 0
        if empty.any():
            out[empty] = np.interp(grid[empty], grid[~empty], out[~empty], period=2 * np.pi)
        return out, False
    order = np.argsort(np.mod(th, 2 * np.pi))
    ths, rs = np.mod(th, 2 * np.pi)[order], r[order]
    grid = np.linspace(0, 2 * np.pi, n, endpoint=False)
    return np.interp(grid, ths, rs, period=2 * np.pi), True

def unwrap_arclength(P, n=N):
    """Returns dict of x(s), y(s), d(s)=signed centroid distance (sign = outside/inside of mean radius)."""
    Q, L = resample_arclength(P, n)
    c = centroid_area(P)
    d = np.hypot(*(Q - c).T)
    return {"x": Q[:, 0], "y": Q[:, 1], "d": d - d.mean(), "L": L, "pts": Q, "c": c}

# ----------------------------------------------------------------------------
# 4. Alignment / conditioning
# ----------------------------------------------------------------------------
def align_fixed_ray(pts, c, ray=0.0):
    """Rotate the resampled loop so index 0 is the crossing of the +x ray from the centroid.
    Uses the LAST outward crossing of angle `ray` (deterministic for star-shaped contours)."""
    th = np.mod(np.arctan2(pts[:, 1] - c[1], pts[:, 0] - c[0]) - ray + np.pi, 2 * np.pi) - np.pi
    # crossing where th goes from <0 to >=0
    cross = np.nonzero((th < 0) & (np.roll(th, -1) >= 0))[0]
    if len(cross) == 0:
        k = int(np.argmin(np.abs(th)))
    else:
        # pick the crossing with the largest radius (outermost) to be stable under notches
        r = np.hypot(pts[cross, 0] - c[0], pts[cross, 1] - c[1])
        k = int(cross[np.argmax(r)] + 1) % len(pts)
    return np.roll(pts, -k, axis=0), k

def align_xcorr(prev, cur):
    """Circular shift of `cur` maximising correlation with `prev` (FFT). Returns shifted, shift."""
    X = np.fft.rfft(cur); Y = np.fft.rfft(prev)
    corr = np.fft.irfft(np.conj(X) * Y, n=len(cur))
    k = int(np.argmax(np.abs(corr)))          # also test polarity: a sign flip is a frame jump too
    sgn = 1.0 if corr[k] >= 0 else -1.0
    return sgn * np.roll(cur, k), k

def condition(w, mode="peak"):
    w = w - w.mean()
    if mode == "peak":
        p = np.max(np.abs(w)); return w / p if p > 0 else w
    r = np.sqrt(np.mean(w * w)); return w / (r * np.sqrt(2)) if r > 0 else w   # sine-equivalent RMS

def rms_diff(A):
    """mean RMS difference between adjacent frames (A: frames x N), plus max."""
    d = np.sqrt(np.mean(np.diff(A, axis=0) ** 2, axis=1))
    return float(d.mean()), float(d.max())

def harmonics_above(w, db=-60.0):
    S = np.abs(np.fft.rfft(w)) / (len(w) / 2)
    S[0] = 0
    ref = S.max()
    return int(np.max(np.nonzero(S > ref * 10 ** (db / 20))[0])) if ref > 0 else 0

# ----------------------------------------------------------------------------
# 5. Pipelines
# ----------------------------------------------------------------------------
def bake_mesh(V, T, frames=128, method="d", multiloop="largest", margin=0.02, ray=0.0, verbose=True, label=""):
    zmin, zmax = V[:, 2].min(), V[:, 2].max()
    hs = zmin + (zmax - zmin) * (margin + (1 - 2 * margin) * np.arange(frames) / (frames - 1))
    raw, ray_al, xc_al = [], [], []
    radial_fail = 0; loops_per = []; t0 = time.perf_counter()
    prev_xc = None; ray_shifts = []; xc_shifts = []
    for h in hs:
        loops = slice_mesh(V, T, h)
        loops = [orient_ccw(L) for L in loops if abs(loop_area(L)) > 1e-9]
        loops_per.append(len(loops))
        if not loops:
            raw.append(np.zeros(N)); ray_al.append(np.zeros(N)); xc_al.append(np.zeros(N)); continue
        if multiloop == "largest":
            loops = [max(loops, key=lambda L: abs(loop_area(L)))]
        waves_raw, waves_ray = [], []
        for L in loops:
            if method == "radial":
                w, ok = unwrap_radial(L); radial_fail += (not ok)
                waves_raw.append(w); waves_ray.append(w)      # radial is already ray-anchored (theta=0)
            else:
                u = unwrap_arclength(L)
                comp = u[method]
                waves_raw.append(comp)
                pts_al, k = align_fixed_ray(u["pts"], u["c"], ray)
                ray_shifts.append(k)
                waves_ray.append(np.roll(comp, -k))
        if multiloop == "sum":
            wr = np.sum(waves_raw, 0); wy = np.sum(waves_ray, 0)
        elif multiloop == "concat":   # each loop gets a share of the cycle proportional to its length
            Ls = [len(x) for x in waves_raw]
            def cat(ws):
                parts = []; lens = [abs(loop_area(L)) for L in loops]; tot = sum(lens)
                acc = 0
                for i, w in enumerate(ws):
                    n = int(round(N * lens[i] / tot)) if i < len(ws) - 1 else N - acc
                    parts.append(np.interp(np.linspace(0, len(w), n, endpoint=False), np.arange(len(w) + 1), np.concatenate([w, w[:1]])))
                    acc += n
                return np.concatenate(parts)
            wr, wy = cat(waves_raw), cat(waves_ray)
        else:
            wr, wy = waves_raw[0], waves_ray[0]
        raw.append(condition(wr)); ray_al.append(condition(wy))
        if prev_xc is None:
            prev_xc = ray_al[-1]
        else:
            prev_xc, kk = align_xcorr(prev_xc, ray_al[-1])
            xc_shifts.append(min(kk, N - kk))
        xc_al.append(prev_xc)
    dt = time.perf_counter() - t0
    raw, ray_al, xc_al = map(np.asarray, (raw, ray_al, xc_al))
    stats = {
        "label": label, "method": method, "multiloop": multiloop, "frames": frames, "tris": len(T),
        "time_total_s": dt, "time_per_frame_ms": 1000 * dt / frames,
        "rms_raw": rms_diff(raw), "rms_ray": rms_diff(ray_al), "rms_xcorr": rms_diff(xc_al),
        "radial_fail_frames": radial_fail, "loops_min_max": (min(loops_per), max(loops_per)),
        "harmonics_-60dB": [harmonics_above(xc_al[i]) for i in (0, frames // 4, frames // 2, 3 * frames // 4, frames - 1)],
        "harmonics_-40dB": [harmonics_above(xc_al[i], -40) for i in (0, frames // 4, frames // 2, 3 * frames // 4, frames - 1)],
        "crest_dB": (20 * np.log10(np.max(np.abs(xc_al), 1) / (np.sqrt(np.mean(xc_al ** 2, 1)) + 1e-12))).round(1),
        "ray_shift_samples": ray_shifts, "xcorr_shift_samples": xc_shifts,
    }
    if verbose:
        print(f"[{label:18s} {method:6s} {multiloop:7s}] tris={len(T):6d} {1000*dt/frames:6.2f} ms/frame "
              f"| RMS adj-frame diff raw={stats['rms_raw'][0]:.3f}/{stats['rms_raw'][1]:.3f} "
              f"ray={stats['rms_ray'][0]:.3f}/{stats['rms_ray'][1]:.3f} xcorr={stats['rms_xcorr'][0]:.3f}/{stats['rms_xcorr'][1]:.3f} "
              f"| loops={stats['loops_min_max']} radialFail={radial_fail} | harm>-60dB={stats['harmonics_-60dB']} >-40dB={stats['harmonics_-40dB']}")
        if ray_shifts:
            rs = np.diff(ray_shifts); rs = np.minimum(np.abs(rs), N - np.abs(rs))
            print(f"    start-point drift raw (fixed-ray shift needed, samples): mean={np.mean(ray_shifts):.0f} adj-frame |Δ| mean={rs.mean():.1f} max={rs.max()} ; residual xcorr shift after ray: mean={np.mean(xc_shifts):.1f} max={max(xc_shifts)} ; crest dB min/med/max={stats['crest_dB'].min()}/{np.median(stats['crest_dB'])}/{stats['crest_dB'].max()}")
    return xc_al, ray_al, raw, stats

# --- volume variant -----------------------------------------------------------
def torus_sdf(p, R=1.0, r=0.4):
    q = np.stack([np.hypot(p[..., 0], p[..., 1]) - R, p[..., 2]], -1)
    return np.linalg.norm(q, axis=-1) - r

def torus_knot(n, p=2, q=3, R=1.0, r=0.5, phase=0.0):
    t = np.linspace(0, 2 * np.pi, n, endpoint=False) + phase
    x = (R + r * np.cos(q * t)) * np.cos(p * t)
    y = (R + r * np.cos(q * t)) * np.sin(p * t)
    z = r * np.sin(q * t)
    return np.stack([x, y, z], -1)

def bake_volume(field, frames=128, label="", scale_range=(0.3, 1.6), shape="tanh"):
    t0 = time.perf_counter(); raw, xc = [], []; prev = None
    for k in range(frames):
        s = scale_range[0] + (scale_range[1] - scale_range[0]) * k / (frames - 1)
        orb = torus_knot(N) * s
        f = field(orb)
        if shape == "tanh":
            f = np.tanh(3 * f)
        w = condition(f); raw.append(w)
        if prev is None:
            prev = w
        else:
            prev, _ = align_xcorr(prev, w)
        xc.append(prev)
    dt = time.perf_counter() - t0
    raw, xc = np.asarray(raw), np.asarray(xc)
    st = {"label": label, "time_per_frame_ms": 1000 * dt / frames, "rms_raw": rms_diff(raw), "rms_xcorr": rms_diff(xc),
          "harmonics_-60dB": [harmonics_above(raw[i]) for i in (0, frames // 2, frames - 1)]}
    print(f"[{label:18s} volume        ] {st['time_per_frame_ms']:6.2f} ms/frame | RMS raw={st['rms_raw'][0]:.3f}/{st['rms_raw'][1]:.3f} "
          f"xcorr={st['rms_xcorr'][0]:.3f}/{st['rms_xcorr'][1]:.3f} | harm>-60dB={st['harmonics_-60dB']}")
    return raw, xc, st

# ----------------------------------------------------------------------------
# 6. Export
# ----------------------------------------------------------------------------
def write_wav(path, frames, sr=44100):
    from scipy.io import wavfile
    wavfile.write(path, sr, np.asarray(frames, np.float32).reshape(-1))

def plot_stack(path, frames, title, every=2):
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(figsize=(11, 7), facecolor="#111")
    ax.set_facecolor("#111")
    F = frames[::every]; n = len(F)
    x = np.arange(N) / N
    for i, w in enumerate(F[::-1]):
        j = n - 1 - i
        ox, oy = 0.45 * j / n, 1.6 * j / n
        c = plt.cm.viridis(j / max(n - 1, 1))
        ax.fill_between(x + ox, oy - 0.3, w * 0.12 + oy, color="#111", zorder=i)
        ax.plot(x + ox, w * 0.12 + oy, color=c, lw=0.7, zorder=i + 0.5)
    ax.set_title(title, color="w"); ax.axis("off")
    fig.savefig(path, dpi=130, bbox_inches="tight", facecolor="#111"); plt.close(fig)

def plot_contours(path, V, T, hs, label):
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, axs = plt.subplots(2, len(hs), figsize=(3.2 * len(hs), 6.4))
    for j, h in enumerate(hs):
        loops = [orient_ccw(L) for L in slice_mesh(V, T, h)]
        ax = axs[0, j]
        for L in loops:
            Q = np.vstack([L, L[:1]]); ax.plot(Q[:, 0], Q[:, 1], lw=1)
            c = centroid_area(L); ax.plot(*c, "r+")
        ax.set_aspect("equal"); ax.set_title(f"{label} h={h:.2f} loops={len(loops)}", fontsize=8)
        ax = axs[1, j]
        if loops:
            L = max(loops, key=lambda L: abs(loop_area(L)))
            wr, ok = unwrap_radial(L); u = unwrap_arclength(L)
            ax.plot(condition(wr), lw=0.7, label=f"radial star={ok}")
            ax.plot(condition(u["d"]), lw=0.7, label="arclen d(s)")
            ax.plot(condition(u["x"]), lw=0.7, label="arclen x(s)")
            ax.legend(fontsize=6)
    fig.tight_layout(); fig.savefig(path, dpi=110); plt.close(fig)

# ----------------------------------------------------------------------------
def main():
    meshes = {
        "sphere": mesh_sphere(),
        "torus": mesh_torus(),
        "twisted_star": mesh_twisted_star(),
        "noisy_sphere": mesh_noisy_sphere(),
        "crescent": mesh_crescent(),
    }
    if len(sys.argv) > 1 and sys.argv[1].endswith(".obj"):
        meshes["obj"] = load_obj(sys.argv[1])
    for k, (V, T) in meshes.items():
        print(f"mesh {k}: {len(V)} verts, {len(T)} tris")

    results = {}
    print("\n=== slice sweep, 128 frames, per method (largest loop only) ===")
    for name, (V, T) in meshes.items():
        for method in ("radial", "d", "x", "y"):
            xc, ray, raw, st = bake_mesh(V, T, 128, method=method, label=name)
            results[(name, method, "largest")] = (xc, st)

    print("\n=== torus multi-loop policies (arc-length d(s)) ===")
    V, T = meshes["torus"]
    for pol in ("largest", "sum", "concat"):
        xc, ray, raw, st = bake_mesh(V, T, 128, method="d", multiloop=pol, label="torus")
        results[("torus", "d", pol)] = (xc, st)

    print("\n=== 256-frame timing (twisted_star, d) and a 20k-tri mesh ===")
    bake_mesh(*meshes["twisted_star"], 256, method="d", label="twisted_star")
    Vb, Tb = mesh_noisy_sphere(nu=200, nv=100)
    bake_mesh(Vb, Tb, 256, method="d", label=f"noisy_sphere_{len(Tb)//1000}k")

    print("\n=== volume orbit sweep ===")
    vol_raw, vol_xc, st = bake_volume(torus_sdf, 128, label="torusSDF")
    noise_raw, noise_xc, st2 = bake_volume(lambda p: fbm3(p * 1.5), 128, label="fbm3-noise")

    # export
    best = results[("twisted_star", "d", "largest")][0]
    write_wav(os.path.join(OUT, "twisted_star_d_128.wav"), best)
    write_wav(os.path.join(OUT, "torus_d_concat_128.wav"), results[("torus", "d", "concat")][0])
    write_wav(os.path.join(OUT, "torusSDF_knot_128.wav"), vol_xc)
    plot_stack(os.path.join(OUT, "twisted_star_d_128.png"), best, "twisted_star, arc-length d(s), xcorr-aligned, 128 frames")
    plot_stack(os.path.join(OUT, "torusSDF_knot_128.png"), vol_xc, "torus SDF sampled on a (2,3) torus knot, scale sweep, 128 frames")
    plot_contours(os.path.join(OUT, "contours.png"), *meshes["twisted_star"], [-0.9, -0.3, 0.3, 0.9], "twisted_star")
    plot_contours(os.path.join(OUT, "contours_torus.png"), *meshes["torus"], [-0.35, 0.0, 0.35], "torus")
    print("\nwrote wav/png to", OUT)

if __name__ == "__main__":
    main()
