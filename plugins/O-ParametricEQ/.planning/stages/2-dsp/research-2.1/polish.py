"""Batch Levenberg-Marquardt on log section parameters. phi = ln(g,k,mL,mB,mH,G,cL,cH)."""
import numpy as np
C = 10 / np.log(10)
IDX = dict(g=0, k=1, mL=2, mB=3, mH=4, G=5, cL=6, cH=7)

def resp_jac(phi, t, jac=True):
    """phi (B,8), t=tan^2(w/2) (B,F) or (F,). Returns dB (B,F) and d dB / d phi (B,F,8)."""
    g, k, mL, mB, mH, G, cL, cH = [np.exp(phi[:, i])[:, None] for i in range(8)]
    X = t / g ** 2; Y = t / G ** 2
    u = mL - mH * X
    Nn = u * u + mB * mB * X; Dn = (1 - X) ** 2 + k * k * X
    N1 = cL * cL + cH * cH * Y; D1 = 1 + Y
    db = C * (np.log(Nn) - np.log(Dn) + np.log(N1) - np.log(D1))
    if not jac: return db, None
    J = np.empty(db.shape + (8,))
    J[..., 0] = -2 * X * ((-2 * mH * u + mB * mB) / Nn - (-2 * (1 - X) + k * k) / Dn)
    J[..., 1] = -2 * k * k * X / Dn
    J[..., 2] = 2 * mL * u / Nn
    J[..., 3] = 2 * mB * mB * X / Nn
    J[..., 4] = -2 * mH * X * u / Nn
    J[..., 5] = -2 * Y * (cH * cH / N1 - 1 / D1)
    J[..., 6] = 2 * cL * cL / N1
    J[..., 7] = 2 * cH * cH * Y / N1
    return db, J * C

def fit(phi0, free, t, target, wts=None, iters=60, pnorms=(2, 4, 8, 16, 32, 64), lam0=1e-2):
    """Converged (reference) fit: IRLS p-norm continuation, adaptive LM with accept/reject."""
    phi = phi0.copy(); B = phi.shape[0]; free = np.asarray(free)
    wts = np.ones_like(target) if wts is None else wts
    lam = np.full(B, lam0)
    def obj(r, p): return (np.abs(r) ** p).sum(-1) ** (1 / p)
    for p in pnorms:
        db, J = resp_jac(phi, t); r = (db - target) * wts; f = obj(r, p)
        for it in range(iters):
            mx = np.abs(r).max(-1, keepdims=True) + 1e-300
            v = (np.abs(r) / mx) ** (p - 2) + 1e-9 if p > 2 else np.ones_like(r)
            Jf = J[..., free] * wts[..., None]
            A = np.einsum('bfi,bf,bfj->bij', Jf, v, Jf); b = np.einsum('bfi,bf->bi', Jf, v * r)
            dg = np.einsum('bii->bi', A)
            A2 = A + (lam[:, None] * (dg + 1e-12))[:, :, None] * np.eye(len(free))
            try: step = np.linalg.solve(A2, b[..., None])[..., 0]
            except np.linalg.LinAlgError: step = np.zeros_like(b)
            step = np.clip(step, -1.0, 1.0)
            cand = phi.copy(); cand[:, free] -= step
            db2, J2 = resp_jac(cand, t); r2 = (db2 - target) * wts; f2 = obj(r2, p)
            ok = np.isfinite(f2) & (f2 < f)
            phi[ok] = cand[ok]; r[ok] = r2[ok]; J[ok] = J2[ok]; f[ok] = f2[ok]
            lam = np.clip(np.where(ok, lam * 0.4, lam * 4), 1e-9, 1e9)
    return phi

def fixed(phi0, free, t, target, wts, iters, lam, prior=0.0):
    """Runtime candidate: fixed iteration count, fixed damping, no accept/reject, 2-norm.
    lam: Marquardt diagonal scaling; prior: Tikhonov pull toward the seed (per unit log-parameter)."""
    phi = phi0.copy(); free = np.asarray(free); n = len(free)
    for it in range(iters):
        db, J = resp_jac(phi, t); r = (db - target) * wts
        Jf = J[..., free] * wts[..., None]
        A = np.einsum('bfi,bfj->bij', Jf, Jf); b = np.einsum('bfi,bf->bi', Jf, r)
        dg = np.einsum('bii->bi', A)
        A = A + (lam * dg + prior)[:, :, None] * np.eye(n) if np.ndim(lam * dg + prior) == 2 else A
        b = b + prior * (phi[:, free] - phi0[:, free])
        phi[:, free] -= np.linalg.solve(A, b[..., None])[..., 0]
    return phi
