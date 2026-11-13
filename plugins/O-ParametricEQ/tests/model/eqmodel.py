# SPDX-License-Identifier: AGPL-3.0-or-later
"""O-ParametricEQ reference model of the closed-form section designer (float64).

Promoted from .planning/stages/2-dsp/research-2.1/eqmodel.py (PLAN-2.1 T2). It implements
.planning/research/ARCHITECTURE.md "Algorithm Details" and is the independent reference
for tests/golden/designs.tsv. The C++ designer must agree with it; it never reads the C++.

Design is scalar and uses the `math` module (the platform libm, the same functions the
C++ calls). Response evaluation uses NumPy and takes an array of w (rad/sample).

Entry points
    design_band (type, fc_hz, gain_db_eff, q, slope_index, fs) -> list of section dicts
    band_db (sections, w)                                       -> digital magnitude, dB
    band_analog_db (type, fc_hz, gain_db_eff, q, slope_index, fs, w)
                                                                -> TRUE analog prototype, dB
    band_digital_db (type, fc_hz, gain_db_eff, q, slope_index, fs, w)
                                                                -> band_db (design_band (...), w)

Conventions (PLAN-2.1)
    P3   Every gain-bearing band is designed on its boost side (|gain|) and a cut is the
         algebraic inverse of each section (RESEARCH-2.1 Q5). Notch and Band Pass are never
         inverted. Gain exactly 0 (and -0.0) takes the boost path.
    D6   Slope is read only by lowShelf, highShelf, lowCut, highCut, tilt.
    D8   Tilt = high shelf of gEff times 10^(-gEff/40); the scalar is folded into the LAST
         section's cL, cH.
    D9   Q0 <- Q0 * Qu on cuts, shelves and tilt; Q is not read at 6 dB/oct.
    D10  Bell re-centred above 0.95 pi; notch clamped at 0.95 pi; band pass at 0.9 pi;
         everything else designed at the true frequency.

Section order: second-order sections in Butterworth index order (Q0 first), the
first-order section last.
"""
import math

import numpy as np

PI = math.pi

TYPES = ('bell', 'lowShelf', 'highShelf', 'lowCut', 'highCut', 'notch', 'bandPass', 'tilt')
SLOPE_ORDER = (1, 2, 3, 4, 6, 8, 12, 16)          # filter order N per bNN_slope index
SECTION_CLASSES = ('gain2', 'gain1', 'cutLp2', 'cutHp2', 'cutLp1', 'cutHp1', 'notch', 'bandPass')
MAX_SECTIONS = 8

GAIN_TYPES = ('bell', 'lowShelf', 'highShelf', 'tilt')
SLOPE_TYPES = ('lowShelf', 'highShelf', 'lowCut', 'highCut', 'tilt')

BELL_RECENTRE = 0.95        # x pi
NOTCH_CLAMP = 0.95          # x pi
BANDPASS_CLAMP = 0.9        # x pi
QE_MIN, QE_MAX = 0.025, 160.0
P1_FLOOR = 1e-12
LP_MATCH = 0.75             # x pi, 3-point matched low-pass
HP_MATCH = 0.9              # x pi, matched high-pass
VIC_FM = 0.9                # Vicanek 2019 match frequency, units of Nyquist
_TINY = 1e-300              # floor inside log10 so an exact zero reads -3000 dB, not -inf


# ----------------------------------------------------------------------------------------
# MatchedPair (ARCHITECTURE "MatchedPair"; expm1 forms, never a1 / a2)
# ----------------------------------------------------------------------------------------

def matched_pair(w, Q):
    """Analog pair s^2 + (w/Q) s + w^2 -> dict P0, P1, Om, g, k, hN.
    w rad/sample (may exceed pi); Q may be math.inf (q = 0)."""
    q = 1.0 / (2.0 * Q)
    if q < 1.0:
        r = math.exp(-q * w)
        th = min(w * math.sqrt(1.0 - q * q), PI)
        s = math.sin(th / 2.0)
        s2 = s * s
        em = math.expm1(-q * w)
        P0 = em * em + 4.0 * r * s2                       # = 1 + a1 + a2
        P1 = (1.0 + r) * (1.0 + r) - 4.0 * r * s2         # = 1 - a1 + a2
    else:
        d = w * math.sqrt(q * q - 1.0)
        e1 = -math.expm1(-q * w + d)                      # 1 - p1
        e2 = -math.expm1(-q * w - d)                      # 1 - p2
        P0 = e1 * e2
        P1 = (2.0 - e1) * (2.0 - e2)
    P1 = max(P1, P1_FLOOR)
    Om = -math.expm1(-2.0 * q * w)                        # = 1 - a2
    g = math.sqrt(P0 / P1)
    k = 2.0 * Om / math.sqrt(P0 * P1)
    f = w / PI
    f2 = f * f
    hN = f2 / math.sqrt((1.0 - f2) * (1.0 - f2) + f2 / (Q * Q))   # analog 2-pole LP at Nyquist
    return dict(P0=P0, P1=P1, Om=Om, g=g, k=k, hN=hN)


# ----------------------------------------------------------------------------------------
# Section designs. Each returns the 8-tuple (g, k, mL, mB, mH, G, cL, cH); halves that a
# section class does not have are None.
# ----------------------------------------------------------------------------------------

def mlr(wz, Qz, wp, Qp, c=1.0):
    """Matched low-pass ratio: SVF + corrector one-pole. Qz may be math.inf (notch)."""
    N = matched_pair(wz, Qz)
    D = matched_pair(wp, Qp)
    g = D['g']
    k = D['k']
    mL = N['P0'] / D['P0']
    mB = 2.0 * N['Om'] / math.sqrt(D['P0'] * D['P1'])
    mH = N['P1'] / D['P1']
    G = N['g'] * N['g'] / N['hN']
    cL = c * D['P0'] / N['P0']
    cH = c * (D['hN'] * D['P1']) / (N['hN'] * N['P1'])
    return g, k, mL, mB, mH, G, cL, cH


def _x_of(w, g):
    t = math.tan(w / 2.0)
    return t * t / (g * g)


def matched_lp2(wc, Q):
    """3-point matched low-pass (High Cut second-order section). SVF only."""
    D = matched_pair(wc, Q)
    wm = min(wc, LP_MATCH * PI)
    Xm = _x_of(wm, D['g'])
    y = (wm / wc) * (wm / wc)
    Ta = 1.0 / ((1.0 - y) * (1.0 - y) + y / (Q * Q))      # analog |H|^2 at wm
    mL = 1.0
    mH = D['hN']
    mB2 = (Ta * ((1.0 - Xm) * (1.0 - Xm) + D['k'] * D['k'] * Xm)
           - (1.0 - mH * Xm) * (1.0 - mH * Xm)) / Xm
    mB = math.sqrt(max(mB2, 0.0))
    return D['g'], D['k'], mL, mB, mH, None, None, None


def matched_hp2(wc, Q):
    """Matched high-pass (Low Cut second-order section), double zero at DC. SVF only."""
    D = matched_pair(wc, Q)
    wm = min(wc, HP_MATCH * PI)
    Xm = _x_of(wm, D['g'])
    y = (wm / wc) * (wm / wc)
    Ta = y * y / ((1.0 - y) * (1.0 - y) + y / (Q * Q))
    mH = math.sqrt(Ta * ((1.0 - Xm) * (1.0 - Xm) + D['k'] * D['k'] * Xm)) / Xm
    return D['g'], D['k'], 0.0, 0.0, mH, None, None, None


def cut_lp1(wc):
    """First-order low-pass wc / (s + wc): matched pole e^(-wc), analog DC and Nyquist gain."""
    pw = PI / wc
    return None, None, None, None, None, math.tanh(wc / 2.0), 1.0, 1.0 / math.sqrt(1.0 + pw * pw)


def cut_hp1(wc):
    """First-order high-pass s / (s + wc)."""
    pw = PI / wc
    return None, None, None, None, None, math.tanh(wc / 2.0), 0.0, pw / math.sqrt(1.0 + pw * pw)


def vic1_shelf(wc, G, high):
    """Vicanek 2019 first-order shelf section of linear gain G at corner wc (eq. 10-12,
    fm = 0.9). Returns the one-pole (G1, cL, cH). Low shelf of gain G = high shelf of gain
    1/G scaled by G."""
    fc = wc / PI
    Gh = G if high else 1.0 / G
    fm = VIC_FM
    phm = 1.0 - math.cos(PI * fm)
    al = (2.0 / (PI * PI)) * (1.0 / (fm * fm) + 1.0 / (Gh * fc * fc)) - 1.0 / phm
    be = (2.0 / (PI * PI)) * (1.0 / (fm * fm) + Gh / (fc * fc)) - 1.0 / phm
    a1 = -al / (1.0 + al + math.sqrt(1.0 + 2.0 * al))
    b = -be / (1.0 + be + math.sqrt(1.0 + 2.0 * be))
    b0 = (1.0 + a1) / (1.0 + b)
    b1 = b * b0
    G1 = (1.0 + a1) / (1.0 - a1)
    cL = (b0 + b1) / (1.0 + a1)
    cH = (b0 - b1) / (1.0 - a1)
    if not high:
        cL = cL * G
        cH = cH * G
    return G1, cL, cH


def bandpass_exact_peak(wc, Q):
    """Exact-peak matched band-pass (single zero at DC, unity and maximum at wc). SVF only."""
    D = matched_pair(wc, Q)
    X0 = _x_of(wc, D['g'])
    mH = math.sqrt(max(1.0 - 1.0 / (X0 * X0), 0.0))
    mB = math.sqrt(max(D['k'] * D['k'] + 2.0 * (1.0 - X0) / X0, 0.0))
    return D['g'], D['k'], 0.0, mB, mH, None, None, None


def notch_design(wc, Q):
    """Notch = MLR with Qz = +inf (zeros on the unit circle at wc). wc already clamped."""
    return mlr(wc, math.inf, wc, Q, 1.0)


# ----------------------------------------------------------------------------------------
# Inverse map (RESEARCH-2.1 Q5). Needs mL, mH, cL, cH > 0, which holds on the boost side.
# ----------------------------------------------------------------------------------------

def invert_params(p, has_svf, has_one_pole):
    g, k, mL, mB, mH, G, cL, cH = p
    if has_svf:
        rt = math.sqrt(mL * mH)
        g, k, mL, mB, mH = g * math.sqrt(mL / mH), mB / rt, 1.0 / mL, k / rt, 1.0 / mH
    if has_one_pole:
        G, cL, cH = G * cL / cH, 1.0 / cL, 1.0 / cH
    return g, k, mL, mB, mH, G, cL, cH


# ----------------------------------------------------------------------------------------
# Prototypes (ARCHITECTURE "Per-type prototypes"), always on the boost side
# ----------------------------------------------------------------------------------------

def butter_qs(N):
    """Butterworth section Qs for order N, highest first."""
    return [1.0 / (2.0 * math.sin((2 * i + 1) * PI / (2.0 * N))) for i in range(N // 2)]


def bell_proto(wc, Qu, gdb):
    """(wz, Qz, wp, Qp, c) of the bell prototype."""
    A = 10.0 ** (gdb / 40.0)
    return wc, Qu / A, wc, Qu * A, 1.0


def _an2_mag2(wa, wz, Qz, wp, Qp, c=1.0):
    """Scalar |Ha|^2 of c (wp^2/wz^2)(s^2+(wz/Qz)s+wz^2)/(s^2+(wp/Qp)s+wp^2) at s = j wa."""
    num = (wz * wz - wa * wa) ** 2 + (wz * wa / Qz) ** 2
    den = (wp * wp - wa * wa) ** 2 + (wp * wa / Qp) ** 2
    return c * c * (wp / wz) ** 4 * num / den


def bell_design_proto(wc, Qu, gabs):
    """Design-side bell prototype: the true one, or re-centred at 0.95 pi when wc is above it.
    Ge = |Ha(wL)|, T = |Ha(wL/2)|^2, Qe = sqrt (0.25 (Ge - T/Ge) / (0.5625 (T - 1)));
    numerator or denominator <= 0 -> Qu; clamp [0.025, 160]."""
    true = bell_proto(wc, Qu, gabs)
    wL = BELL_RECENTRE * PI
    if not (wc > wL):
        return true
    Ge = math.sqrt(_an2_mag2(wL, *true))
    T = _an2_mag2(wL / 2.0, *true)
    num = 0.25 * (Ge - T / Ge)
    den = 0.5625 * (T - 1.0)
    Qe = math.sqrt(num / den) if (num > 0.0 and den > 0.0) else Qu
    Qe = min(max(Qe, QE_MIN), QE_MAX)
    Ae = math.sqrt(Ge)                                    # = 10^(Ge_dB / 40)
    return wL, Qe / Ae, wL, Qe * Ae, 1.0


def _sec(cls, params, true, design=None, **extra):
    has_svf = params[0] is not None
    has_one_pole = params[5] is not None
    s = dict(cls=cls, hasSvf=has_svf, hasOnePole=has_one_pole,
             g=params[0], k=params[1], mL=params[2], mB=params[3], mH=params[4],
             G=params[5], cL=params[6], cH=params[7],
             trueProto=true, designProto=design if design is not None else true)
    s.update(extra)
    return s


def _params(s):
    return s['g'], s['k'], s['mL'], s['mB'], s['mH'], s['G'], s['cL'], s['cH']


def _set_params(s, p):
    s['g'], s['k'], s['mL'], s['mB'], s['mH'], s['G'], s['cL'], s['cH'] = p


def type_name(t):
    """Accepts a name from TYPES or the bNN_type index."""
    if isinstance(t, str):
        if t not in TYPES:
            raise ValueError('unknown type %r' % (t,))
        return t
    return TYPES[int(t)]


def _shelf_sections(wc, Qu, gabs, N, high):
    """Boost-side shelf of order N (Gl >= 1). Analog tuples are (wz, Qz, wp, Qp, c) with c the
    DC gain; the first-order section uses (wz, None, wp, None, c) the same way."""
    Gl = 10.0 ** (gabs / 20.0)
    r = Gl ** (1.0 / (2.0 * N))
    out = []
    for i, Qi in enumerate(butter_qs(N)):
        Q = Qi * Qu if i == 0 else Qi                     # D9: resonance on Q0 only
        if high:
            a = (wc / r, Q, wc * r, Q, 1.0)
        else:
            a = (wc * r, Q, wc / r, Q, Gl ** (2.0 / N))
        out.append(_sec('gain2', mlr(*a), a))
    if N % 2:
        Gs = Gl ** (1.0 / N)                              # the section's own gain
        G1, cL, cH = vic1_shelf(wc, Gs, high)
        a = (wc / r, None, wc * r, None, 1.0) if high else (wc * r, None, wc / r, None, Gs)
        out.append(_sec('gain1', (None, None, None, None, None, G1, cL, cH), a,
                        wc=wc, gain=Gs, high=high))
    return out


def _cut_sections(wc, Qu, N, lowpass):
    out = []
    for i, Qi in enumerate(butter_qs(N)):
        Q = Qi * Qu if i == 0 else Qi
        a = (None, None, wc, Q, 1.0)
        if lowpass:
            out.append(_sec('cutLp2', matched_lp2(wc, Q), a))
        else:
            out.append(_sec('cutHp2', matched_hp2(wc, Q), a))
    if N % 2:
        a = (None, None, wc, None, 1.0)
        out.append(_sec('cutLp1', cut_lp1(wc), a) if lowpass else _sec('cutHp1', cut_hp1(wc), a))
    return out


# ----------------------------------------------------------------------------------------
# The entry point
# ----------------------------------------------------------------------------------------

def band_is_inverted(type, gain_db_eff):
    return type_name(type) in GAIN_TYPES and gain_db_eff < 0.0


def design_band(type, fc_hz, gain_db_eff, q, slope_index, fs):
    """Band settings -> list of section dicts, in the interface order.

    Each section: cls (SECTION_CLASSES), hasSvf, hasOnePole, g k mL mB mH (None when
    not hasSvf), G cL cH (None when not hasOnePole), trueProto and designProto (boost side;
    (wz, Qz, wp, Qp, c), unused entries None), and for gain1 also wc, gain, high.
    gain_db_eff is already scaled and clamped to +-60."""
    t = type_name(type)
    wc = 2.0 * PI * fc_hz / fs
    gabs = abs(gain_db_eff)

    if t == 'bell':
        true = bell_proto(wc, q, gabs)
        design = bell_design_proto(wc, q, gabs)
        secs = [_sec('gain2', mlr(*design), true, design)]
    elif t == 'notch':
        wd = min(wc, NOTCH_CLAMP * PI)
        secs = [_sec('notch', notch_design(wd, q), (wc, math.inf, wc, q, 1.0),
                     (wd, math.inf, wd, q, 1.0))]
    elif t == 'bandPass':
        wd = min(wc, BANDPASS_CLAMP * PI)
        secs = [_sec('bandPass', bandpass_exact_peak(wd, q), (None, None, wc, q, 1.0),
                     (None, None, wd, q, 1.0))]
    else:
        N = SLOPE_ORDER[int(slope_index)]
        if t == 'lowShelf':
            secs = _shelf_sections(wc, q, gabs, N, False)
        elif t == 'highShelf':
            secs = _shelf_sections(wc, q, gabs, N, True)
        elif t == 'tilt':
            secs = _shelf_sections(wc, q, gabs, N, True)
            scalar = 10.0 ** (-gabs / 40.0)               # D8, folded into the last section
            secs[-1]['cL'] = secs[-1]['cL'] * scalar
            secs[-1]['cH'] = secs[-1]['cH'] * scalar
            secs[-1]['tiltScalar'] = scalar
        elif t == 'highCut':
            secs = _cut_sections(wc, q, N, True)
        else:                                             # lowCut
            secs = _cut_sections(wc, q, N, False)

    if t in GAIN_TYPES and gain_db_eff < 0.0:             # P3: a cut is the inverse of the boost
        for s in secs:
            _set_params(s, invert_params(_params(s), s['hasSvf'], s['hasOnePole']))
            s['inverted'] = True
    return secs


# ----------------------------------------------------------------------------------------
# Digital response (ARCHITECTURE "SVF section and one-pole"). w in rad/sample, 0 < w < pi.
# ----------------------------------------------------------------------------------------

def svf_mag2(w, g, k, mL, mB, mH):
    X = np.tan(np.asarray(w, float) / 2) ** 2 / g ** 2
    return ((mL - mH * X) ** 2 + mB ** 2 * X) / ((1 - X) ** 2 + k ** 2 * X)


def op_mag2(w, G, cL, cH):
    X = np.tan(np.asarray(w, float) / 2) ** 2 / G ** 2
    return (cL ** 2 + cH ** 2 * X) / (1 + X)


def sec_db(w, p):
    """dB of a full 8-tuple (g, k, mL, mB, mH, G, cL, cH); every entry must be a number."""
    return 10 * np.log10(np.maximum(svf_mag2(w, *p[:5]) * op_mag2(w, *p[5:]), _TINY))


def section_mag2(s, w):
    """|H|^2 of one designed section dict, using only the halves it has."""
    w = np.asarray(w, float)
    m2 = np.ones_like(w)
    if s['hasSvf']:
        m2 = m2 * svf_mag2(w, s['g'], s['k'], s['mL'], s['mB'], s['mH'])
    if s['hasOnePole']:
        m2 = m2 * op_mag2(w, s['G'], s['cL'], s['cH'])
    return m2


def band_db(sections, w):
    """Digital magnitude in dB of a designed band (the list from design_band) at w (array,
    rad/sample, strictly below pi)."""
    w = np.asarray(w, float)
    db = np.zeros_like(w)
    for s in sections:
        db = db + 10 * np.log10(np.maximum(section_mag2(s, w), _TINY))
    return db


def band_digital_db(type, fc_hz, gain_db_eff, q, slope_index, fs, w):
    return band_db(design_band(type, fc_hz, gain_db_eff, q, slope_index, fs), w)


# ----------------------------------------------------------------------------------------
# Analog prototype response. wa is the analog frequency on the same rad/sample scale
# (f = wa fs / 2 pi); it may be an array.
# ----------------------------------------------------------------------------------------

def an2_db(wa, wz, Qz, wp, Qp, c=1.0):
    """c (wp^2/wz^2)(s^2+(wz/Qz)s+wz^2)/(s^2+(wp/Qp)s+wp^2) in dB. Qz may be math.inf."""
    wa = np.asarray(wa, float)
    num = (wz ** 2 - wa ** 2) ** 2 + (wz * wa / Qz) ** 2
    den = (wp ** 2 - wa ** 2) ** 2 + (wp * wa / Qp) ** 2
    return 10 * np.log10(np.maximum(c ** 2 * (wp / wz) ** 4 * num / den, _TINY))


def an1_shelf_db(wa, wc, G, high):
    """Vicanek first-order shelf prototype of linear gain G at corner wc, in dB."""
    wa = np.asarray(wa, float)
    Gh = G if high else 1.0 / G
    x2 = (wa / wc) ** 2
    m2 = (1 + Gh * x2) / (1 + x2 / Gh)
    return 10 * np.log10(m2 * (1.0 if high else G ** 2))


def an_lp2_db(wa, wc, Q):
    y = (np.asarray(wa, float) / wc) ** 2
    return 10 * np.log10(np.maximum(1.0 / ((1 - y) ** 2 + y / Q ** 2), _TINY))


def an_hp2_db(wa, wc, Q):
    y = (np.asarray(wa, float) / wc) ** 2
    return 10 * np.log10(np.maximum(y ** 2 / ((1 - y) ** 2 + y / Q ** 2), _TINY))


def an_lp1_db(wa, wc):
    y = (np.asarray(wa, float) / wc) ** 2
    return 10 * np.log10(np.maximum(1.0 / (1 + y), _TINY))


def an_hp1_db(wa, wc):
    y = (np.asarray(wa, float) / wc) ** 2
    return 10 * np.log10(np.maximum(y / (1 + y), _TINY))


def an_notch_db(wa, wc, Q):
    wa = np.asarray(wa, float)
    n = (wc ** 2 - wa ** 2) ** 2
    return 10 * np.log10(np.maximum(n, _TINY) / (n + (wc * wa / Q) ** 2))


def an_bp_db(wa, wc, Q):
    wa = np.asarray(wa, float)
    n = (wc * wa / Q) ** 2
    return 10 * np.log10(np.maximum(n / ((wc ** 2 - wa ** 2) ** 2 + n), _TINY))


def band_analog_db(type, fc_hz, gain_db_eff, q, slope_index, fs, w):
    """TRUE analog prototype magnitude in dB at w (array, rad/sample; f = w fs / 2 pi).
    True means before the bell re-centre and before the notch / band-pass clamps. Built on
    the boost side and negated for a cut (the prototypes are exact reciprocals)."""
    t = type_name(type)
    wa = np.asarray(w, float)
    wc = 2.0 * PI * fc_hz / fs
    gabs = abs(gain_db_eff)

    if t == 'notch':
        return an_notch_db(wa, wc, q)
    if t == 'bandPass':
        return an_bp_db(wa, wc, q)
    if t == 'bell':
        db = an2_db(wa, *bell_proto(wc, q, gabs))
        return -db if gain_db_eff < 0.0 else db

    N = SLOPE_ORDER[int(slope_index)]
    qs = butter_qs(N)
    db = np.zeros_like(wa)
    if t in ('lowCut', 'highCut'):
        for i, Qi in enumerate(qs):
            Q = Qi * q if i == 0 else Qi
            db = db + (an_lp2_db(wa, wc, Q) if t == 'highCut' else an_hp2_db(wa, wc, Q))
        if N % 2:
            db = db + (an_lp1_db(wa, wc) if t == 'highCut' else an_hp1_db(wa, wc))
        return db

    high = t != 'lowShelf'                                # highShelf and tilt
    Gl = 10.0 ** (gabs / 20.0)
    r = Gl ** (1.0 / (2.0 * N))
    for i, Qi in enumerate(qs):
        Q = Qi * q if i == 0 else Qi
        if high:
            db = db + an2_db(wa, wc / r, Q, wc * r, Q, 1.0)
        else:
            db = db + an2_db(wa, wc * r, Q, wc / r, Q, Gl ** (2.0 / N))
    if N % 2:
        db = db + an1_shelf_db(wa, wc, Gl ** (1.0 / N), high)
    if t == 'tilt':
        db = db - gabs / 2.0                              # 20 log10 (10^(-g/40))
    return -db if gain_db_eff < 0.0 else db


# ----------------------------------------------------------------------------------------
# Grids
# ----------------------------------------------------------------------------------------

def fgrid(fs, n=3000, lo=10.0, hi=20000.0):
    """The research scoring grid: n log points lo .. min (hi, 0.4999 fs). Returns (f, w)."""
    f = np.geomspace(lo, min(hi, 0.4999 * fs), n)
    return f, 2 * PI * f / fs
