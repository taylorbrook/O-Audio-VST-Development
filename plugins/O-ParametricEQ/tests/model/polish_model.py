# SPDX-License-Identifier: AGPL-3.0-or-later
"""O-ParametricEQ reference model of the accuracy polish (float64), Phase 2.1b.

Promoted from .planning/stages/2-dsp/research-2.1/cand.py (`polish`) and polish.py
(`resp_jac`) (PLAN-2.1b T2). It implements .planning/research/ARCHITECTURE.md "Accuracy
polish (v1, Phase 2.1b)" and is the independent reference for
tests/golden/designs-polished.tsv. The C++ (Source/dsp/Polish.h) must agree with it; it
never reads the C++. The 6x6 solve here is LAPACK (numpy.linalg.solve); the C++ uses a
Cholesky factorisation, which is why the golden tolerance is 1e-6 and not 1e-9.

Since the rework (RESEARCH-2.1b-continuity): every step is length-limited (STEP_LIMIT), and
nothing is polished below MIN_FS.

Entry points
    Grid (fs)                                         the 16 fixed match points of one rate
    polish_section (grid, proto, seed)                -> (8-tuple, weight, steps)
    design_band_polished (type, fc_hz, gain_db_eff, q, slope_index, fs)
                                                      -> section dicts as eqmodel.design_band,
                                                         each with 'weight' and 'steps'

The constants below are part of the design (RESEARCH-2.1 pitfall 5). They mirror
Source/dsp/PolishConstants.h; changing one changes every acceptance number.
"""
import math

import numpy as np

import eqmodel as m

PI = math.pi

M_FIXED = 16                # fixed, log-spaced match points
TIED_SCALES = (0.5, 1.5)    # h = s / (2 Q)
N_TIED = 10                 # (centre + 2 x 2) for the zero pair and for the pole pair
N_POINTS = M_FIXED + N_TIED
K_STEPS = 2                 # anchored Gauss-Newton steps
K_EXTRA = 2                 # further steps of the blended extension
LAMBDA = 1e-3               # Marquardt diagonal scaling
MU = 0.1                    # anchor to the seed, dB^2 per unit log-parameter^2
SA = 0.002                  # dB RMS: at or below this the seed is kept exactly
SB = 0.006                  # dB RMS: the accept weight's first factor reaches 1 here
SX = 0.03                   # dB RMS after K steps: the extension fades in from SX to 2 SX
ACCEPT_RAMP = 0.1           # objective reduction at which the second factor reaches 1
STEP_LIMIT = 0.35           # smooth limit on the norm of every step (six free log-parameters)
MIN_FS = 44100.0            # Hz: below this nothing is polished (the seed is returned)
F_LO = 20.0                 # Hz
F_HI = 20000.0              # Hz
FS_FRACTION = 0.499         # band top = min (F_HI, FS_FRACTION fs)

C_DB = 10.0 / math.log(10.0)
FREE = (0, 1, 3, 4, 5, 7)   # g, k, mB, mH, G, cH; mL (2) and cL (6) stay at the seed


sstep = m.sstep             # cubic smoothstep on [0, 1]; one definition, in eqmodel


class Grid:
    """The fixed part of the match grid for one sample rate."""

    def __init__(self, fs):
        self.fs = float(fs)
        self.f_lo = F_LO
        self.f_hi = min(F_HI, FS_FRACTION * self.fs)
        self.w_lo = 2.0 * PI * self.f_lo / self.fs
        self.w_hi = 2.0 * PI * self.f_hi / self.fs
        self.w = [2.0 * PI * (self.f_lo * math.pow(self.f_hi / self.f_lo, i / (M_FIXED - 1.0))) / self.fs
                  for i in range(M_FIXED)]


def match_points(grid, proto):
    """-> (w, t = tan^2 (w/2), ln |Ha|^2), each of length N_POINTS, for the analog section
    proto = (wz, Qz, wp, Qp, c)."""
    wz, Qz, wp, Qp, c = proto
    w = list(grid.w)
    for w0, q in ((wz, Qz), (wp, Qp)):
        w.append(w0)
        for s in TIED_SCALES:
            h = s / (2.0 * q)
            rt = math.sqrt(1.0 + h * h)
            w.append(w0 * (rt + h))
            w.append(w0 * (rt - h))
    for j in range(M_FIXED, N_POINTS):
        w[j] = min(max(w[j], grid.w_lo), grid.w_hi)
    w = np.array(w)
    t = np.tan(w / 2.0) ** 2
    wz2 = wz * wz
    wp2 = wp * wp
    kz = wz2 / (Qz * Qz)
    kp = wp2 / (Qp * Qp)
    sc = c * c * (wp2 * wp2) / (wz2 * wz2)
    x = w * w
    tl = np.log(sc * ((wz2 - x) ** 2 + kz * x) / ((wp2 - x) ** 2 + kp * x))
    return w, t, tl


def resp_jac(phi, t, jac=True):
    """phi: 8 log-parameters ln (g, k, mL, mB, mH, G, cL, cH); t = tan^2 (w/2), shape (F,).
    Returns L = ln |H|^2 (F,) and dL / d phi_free (F, 6) in the order of FREE."""
    g, k, mL, mB, mH, G, cL, cH = [math.exp(v) for v in phi]
    X = t / (g * g)
    Y = t / (G * G)
    u = mL - mH * X
    Nn = u * u + mB * mB * X
    Dn = (1.0 - X) ** 2 + k * k * X
    N1 = cL * cL + cH * cH * Y
    D1 = 1.0 + Y
    L = np.log((Nn * N1) / (Dn * D1))
    if not jac:
        return L, None
    J = np.empty(t.shape + (6,))
    J[:, 0] = -2.0 * X * ((-2.0 * mH * u + mB * mB) / Nn - (-2.0 * (1.0 - X) + k * k) / Dn)
    J[:, 1] = -2.0 * k * k * X / Dn
    J[:, 2] = 2.0 * mB * mB * X / Nn
    J[:, 3] = -2.0 * mH * X * u / Nn
    J[:, 4] = -2.0 * Y * (cH * cH / N1 - 1.0 / D1)
    J[:, 5] = 2.0 * cH * cH * Y / N1
    return L, J


def _objective(phi, t, tl):
    L, _ = resp_jac(phi, t, False)
    r = C_DB * (L - tl)
    return float((r * r).sum())


def _step(phi, phi0, t, tl):
    """One anchored Gauss-Newton step, length-limited:
    d <- d R / sqrt (R^2 + |d|^2), R = STEP_LIMIT. Returns the new phi, or None for a
    non-finite result."""
    L, J = resp_jac(phi, t)
    r = C_DB * (L - tl)
    Jd = C_DB * J
    A = Jd.T @ Jd
    b = Jd.T @ r
    free = list(FREE)
    A = A + np.diag(LAMBDA * np.diag(A) + MU)
    b = b + MU * (phi[free] - phi0[free])
    if not (np.isfinite(A).all() and np.isfinite(b).all()):
        return None
    try:
        d = np.linalg.solve(A, b)
    except np.linalg.LinAlgError:
        return None
    d = d * (STEP_LIMIT / math.sqrt(STEP_LIMIT * STEP_LIMIT + float((d * d).sum())))
    out = phi.copy()
    out[free] = out[free] - d
    if not np.isfinite(out).all():
        return None
    return out


def polish_section(grid, proto, seed, info=None):
    """Polish one boost-side gain2 section toward its TRUE analog section.

    proto = (wz, Qz, wp, Qp, c); seed = (g, k, mL, mB, mH, G, cL, cH), all > 0.
    Returns (params 8-tuple, accept weight in [0, 1], steps in {0, 2, 4}). Where the
    weight is 0 the returned tuple IS the seed (same floats)."""
    seed = tuple(float(v) for v in seed)
    with np.errstate(all='ignore'):
        _, t, tl = match_points(grid, proto)
        phi0 = np.log(np.array(seed))
        if not (np.isfinite(phi0).all() and np.isfinite(tl).all()):
            return seed, 0.0, 0

        S0 = _objective(phi0, t, tl)
        if not math.isfinite(S0) or S0 / N_POINTS <= SA * SA:
            return seed, 0.0, 0                           # skip: exact

        steps = 0
        phi = phi0
        for _ in range(K_STEPS):
            phi = _step(phi, phi0, t, tl)
            steps += 1
            if phi is None:
                return seed, 0.0, steps

        S2 = _objective(phi, t, tl)
        if not math.isfinite(S2):
            return seed, 0.0, steps
        wx = sstep((math.sqrt(S2 / N_POINTS) - SX) / SX)
        if info is not None:
            info['wx'] = wx
        if wx > 0.0:
            phi4 = phi
            for _ in range(K_EXTRA):
                steps += 1
                if phi4 is not None:
                    phi4 = _step(phi4, phi0, t, tl)
            if phi4 is not None:                          # a non-finite phi4 leaves phi2
                phi = phi + wx * (phi4 - phi)

        S1 = _objective(phi, t, tl)
        if not (math.isfinite(S1) and np.isfinite(phi).all()):
            return seed, 0.0, steps
        a = sstep((S0 / N_POINTS - SA * SA) / (SB * SB - SA * SA)) * sstep((1.0 - S1 / S0) / ACCEPT_RAMP)
        if info is not None:
            info['S0'] = S0
            info['S1'] = S1
        if not (a > 0.0):
            return seed, 0.0, steps

        out = list(seed)                                  # mL and cL: the seed's own floats
        for i in FREE:
            out[i] = math.exp(phi0[i] + a * (phi[i] - phi0[i]))
        if not all(math.isfinite(v) and v > 0.0 for v in out):
            return seed, 0.0, steps
        return tuple(out), a, steps


_GRIDS = {}


def grid_for(fs):
    g = _GRIDS.get(fs)
    if g is None:
        g = _GRIDS[fs] = Grid(fs)
    return g


def design_band_polished(type, fc_hz, gain_db_eff, q, slope_index, fs):
    """eqmodel.design_band with every gain2 section of a Bell, Low Shelf, High Shelf or Tilt
    polished on the boost side against its true prototype. The tilt scalar is folded after
    the polish (the polish sees the plain high-shelf section) and a cut is the inverse of
    the polished boost. Other types are the closed form. Every section gets 'weight' and
    'steps'.

    The seed is eqmodel's closed form (shelf / tilt pairs re-centred there); the target is
    the true section. Below MIN_FS nothing is polished: the result is the seed, weights 0."""
    t = m.type_name(type)
    if t not in m.GAIN_TYPES or fs < MIN_FS:
        secs = m.design_band(t, fc_hz, gain_db_eff, q, slope_index, fs)
        for s in secs:
            s['weight'] = 0.0
            s['steps'] = 0
        return secs

    gabs = abs(gain_db_eff)
    grid = grid_for(fs)
    secs = m.design_band('highShelf' if t == 'tilt' else t, fc_hz, gabs, q, slope_index, fs)
    for s in secs:
        s['weight'] = 0.0
        s['steps'] = 0
        if s['cls'] == 'gain2':
            p, a, steps = polish_section(grid, s['trueProto'], m._params(s))
            m._set_params(s, p)
            s['weight'] = a
            s['steps'] = steps

    if t == 'tilt':
        scalar = 10.0 ** (-gabs / 40.0)                   # D8, folded into the last section
        secs[-1]['cL'] = secs[-1]['cL'] * scalar
        secs[-1]['cH'] = secs[-1]['cH'] * scalar
        secs[-1]['tiltScalar'] = scalar

    if gain_db_eff < 0.0:                                 # P8: a cut is the inverse of the boost
        for s in secs:
            m._set_params(s, m.invert_params(m._params(s), s['hasSvf'], s['hasOnePole']))
            s['inverted'] = True
    return secs
