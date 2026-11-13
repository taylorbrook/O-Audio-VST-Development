"""O-ParametricEQ Stage 2 research model (float64, NumPy). Re-implements ARCHITECTURE.md
"Algorithm Details" so the Stage 0 [MODEL] tables can be reproduced. Everything is
vectorised: design inputs broadcast, responses evaluate on a trailing frequency axis."""
import numpy as np
PI = np.pi

def matched_pair(w, Q):
    """w rad/sample (may exceed pi), Q may be inf. Returns dict of arrays."""
    w, Q = np.broadcast_arrays(np.asarray(w, float), np.asarray(Q, float))
    q = 1.0 / (2.0 * Q)
    und = q < 1.0
    with np.errstate(invalid='ignore', over='ignore'):
        r = np.exp(-q * w)
        th = np.minimum(w * np.sqrt(np.where(und, 1 - q * q, 0.0)), PI)
        s2 = np.sin(th / 2) ** 2
        P0u = np.expm1(-q * w) ** 2 + 4 * r * s2
        P1u = (1 + r) ** 2 - 4 * r * s2
        d = w * np.sqrt(np.where(und, 0.0, q * q - 1))
        e1 = -np.expm1(-q * w + d); e2 = -np.expm1(-q * w - d)
        P0o = e1 * e2; P1o = (2 - e1) * (2 - e2)
    P0 = np.where(und, P0u, P0o); P1 = np.maximum(np.where(und, P1u, P1o), 1e-12)
    Om = -np.expm1(-2 * q * w)
    g = np.sqrt(P0 / P1); k = 2 * Om / np.sqrt(P0 * P1)
    f = w / PI
    hN = f * f / np.sqrt((1 - f * f) ** 2 + f * f / (Q * Q))
    return dict(P0=P0, P1=P1, Om=Om, g=g, k=k, hN=hN)

def mlr(wz, Qz, wp, Qp, c=1.0):
    """Gain-bearing section. Returns (g,k,mL,mB,mH,G,cL,cH)."""
    N = matched_pair(wz, Qz); D = matched_pair(wp, Qp)
    g = D['g']; k = D['k']
    mL = N['P0'] / D['P0']; mB = 2 * N['Om'] / np.sqrt(D['P0'] * D['P1']); mH = N['P1'] / D['P1']
    G = N['g'] ** 2 / N['hN']
    cL = c * D['P0'] / N['P0']; cH = c * (D['hN'] * D['P1']) / (N['hN'] * N['P1'])
    return g, k, mL, mB, mH, G, cL, cH

def svf_mag2(w, g, k, mL, mB, mH):
    X = np.tan(w / 2) ** 2 / g ** 2
    return ((mL - mH * X) ** 2 + mB ** 2 * X) / ((1 - X) ** 2 + k ** 2 * X)

def op_mag2(w, G, cL, cH):
    X = np.tan(w / 2) ** 2 / G ** 2
    return (cL ** 2 + cH ** 2 * X) / (1 + X)

def sec_db(w, p):
    """p = 8-tuple of arrays shaped (...); w shaped (F,). Returns (..., F) dB."""
    p = [np.asarray(x, float)[..., None] for x in p]
    return 10 * np.log10(svf_mag2(w, *p[:5]) * op_mag2(w, *p[5:]))

def an2_db(wa, wz, Qz, wp, Qp, c=1.0):
    """analog c (wp²/wz²)(s²+(wz/Qz)s+wz²)/(s²+(wp/Qp)s+wp²) in dB, wa (F,), others (...)"""
    wz, Qz, wp, Qp, c = [np.asarray(x, float)[..., None] for x in (wz, Qz, wp, Qp, c)]
    num = (wz ** 2 - wa ** 2) ** 2 + (wz * wa / Qz) ** 2
    den = (wp ** 2 - wa ** 2) ** 2 + (wp * wa / Qp) ** 2
    return 10 * np.log10(c ** 2 * (wp / wz) ** 4 * num / den)

def bell_proto(wc, Qu, gdb):
    A = 10 ** (np.asarray(gdb, float) / 40)
    return wc, Qu / A, wc, Qu * A, 1.0

def bell_design(wc, Qu, gdb, recentre=True):
    """Bell with the >0.95 pi re-centre rule. Inputs broadcast."""
    wc, Qu, gdb = np.broadcast_arrays(*[np.asarray(x, float) for x in (wc, Qu, gdb)])
    wL = 0.95 * PI
    wcd = wc.copy(); Qd = Qu.copy(); gd = gdb.copy()
    if recentre:
        hi = wc > wL
        if hi.any():
            pz = bell_proto(wc[hi], Qu[hi], gdb[hi])
            GeDb = an2_db(np.array([wL]), *pz)[..., 0]; TDb = an2_db(np.array([wL / 2]), *pz)[..., 0]
            Ge = 10 ** (GeDb / 20); T = 10 ** (TDb / 10)
            num = 0.25 * (Ge - T / Ge); den = 0.5625 * (T - 1)
            ok = (num > 0) & (den > 0) if False else (num * den > 0)
            with np.errstate(all='ignore'):
                Qe = np.sqrt(np.where(ok, num / den, 1.0))
            # Qe here is Qp_e/ sqrt? derivation: bell at wL gain Ge: |H(wL/2)|^2 = (0.5625+0.25 Ge/ (Qe^2) ...)
            Qe = np.clip(np.where(ok, Qe, Qu[hi]), 0.025, 160)
            wcd[hi] = wL; Qd[hi] = Qe; gd[hi] = GeDb
    return mlr(*bell_proto(wcd, Qd, gd)), (wcd, Qd, gd)

def butter_qs(N):
    return [1 / (2 * np.sin((2 * i + 1) * PI / (2 * N))) for i in range(N // 2)]

def shelf_sections(wc, Qu, gdb, N, high=False, tilt=False):
    """Analog section list for an order-N shelf: list of ('2', wz,Qz,wp,Qp,c) / ('1', wz, wp, c, G1, high)."""
    Gl = 10 ** (np.asarray(gdb, float) / 20)
    r = Gl ** (1 / (2 * N))
    out = []
    qs = butter_qs(N)
    for i, Qi in enumerate(qs):
        Q = Qi * Qu if i == 0 else Qi * np.ones_like(Qu * 1.0)
        if high: out.append(('2', wc / r, Q, wc * r, Q, 1.0 + 0 * Gl))
        else:    out.append(('2', wc * r, Q, wc / r, Q, Gl ** (2 / N)))
    if N % 2:
        out.append(('1', wc, Gl ** (1 / N), high))
    return out

def vic1_shelf(wc, G, high):
    """Vicanek 2019 first-order shelf section of linear gain G at corner wc. Returns (G1,cL,cH)."""
    fc = np.asarray(wc, float) / PI; G = np.asarray(G, float)
    Gh = G if high else 1 / G
    fm = 0.9; phm = 1 - np.cos(PI * fm)
    al = (2 / PI ** 2) * (1 / fm ** 2 + 1 / (Gh * fc ** 2)) - 1 / phm
    be = (2 / PI ** 2) * (1 / fm ** 2 + Gh / fc ** 2) - 1 / phm
    a1 = -al / (1 + al + np.sqrt(1 + 2 * al)); b = -be / (1 + be + np.sqrt(1 + 2 * be))
    b0 = (1 + a1) / (1 + b); b1 = b * b0
    G1 = (1 + a1) / (1 - a1); cL = (b0 + b1) / (1 + a1); cH = (b0 - b1) / (1 - a1)
    if not high: cL = cL * G; cH = cH * G
    return G1, cL, cH

def an1_shelf_db(wa, wc, G, high):
    wc, G = [np.asarray(x, float)[..., None] for x in (wc, G)]
    Gh = G if high else 1 / G
    x2 = (wa / wc) ** 2
    m2 = (1 + Gh * x2) / (1 + x2 / Gh)
    return 10 * np.log10(m2 * (1 if high else G ** 2))

def shelf_db(w, wa, wc, Qu, gdb, N, high=False):
    """Returns (digital dB, analog dB) of the full order-N shelf, shapes (..., F)."""
    dig = 0; ana = 0
    for s in shelf_sections(wc, Qu, gdb, N, high):
        if s[0] == '2':
            dig = dig + sec_db(w, mlr(*s[1:])); ana = ana + an2_db(wa, *s[1:])
        else:
            G1, cL, cH = [np.asarray(x, float)[..., None] for x in vic1_shelf(s[1], s[2], s[3])]
            dig = dig + 10 * np.log10(op_mag2(w, G1, cL, cH)); ana = ana + an1_shelf_db(wa, s[1], s[2], s[3])
    return dig, ana

def fgrid(fs, n=3000, lo=10.0, hi=20000.0):
    f = np.geomspace(lo, min(hi, 0.4999 * fs), n)
    return f, 2 * PI * f / fs
