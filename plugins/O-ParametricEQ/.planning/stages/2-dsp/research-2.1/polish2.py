import numpy as np, polish as P
def fixed2(phi0, free, t, target, K, mu=0.0, lam=1e-3, clamp=None, wts=None):
    """Fixed-K regularised Gauss-Newton. mu: absolute Tikhonov weight pulling toward the seed
    (dB^2 per squared log-parameter). clamp: max |total departure from seed| per log-parameter.
    Returns phi, S0 (seed objective on the match grid), S1 (final objective on the match grid)."""
    phi = phi0.copy(); free = np.asarray(free); n = len(free); I = np.eye(n)
    wts = np.ones_like(target) if wts is None else wts
    S0 = None
    for it in range(K):
        db, J = P.resp_jac(phi, t); r = (db - target) * wts
        if S0 is None: S0 = (r * r).sum(-1)
        Jf = J[..., free] * wts[..., None]
        A = np.einsum('bfi,bfj->bij', Jf, Jf); b = np.einsum('bfi,bf->bi', Jf, r)
        dg = np.einsum('bii->bi', A)
        A = A + (lam * dg + mu + 1e-12)[:, :, None] * I
        b = b + mu * (phi[:, free] - phi0[:, free])
        phi[:, free] -= np.linalg.solve(A, b[..., None])[..., 0]
        if clamp is not None:
            phi[:, free] = phi0[:, free] + np.clip(phi[:, free] - phi0[:, free], -clamp, clamp)
    r = (P.resp_jac(phi, t, False)[0] - target) * wts
    return phi, S0, (r * r).sum(-1)
