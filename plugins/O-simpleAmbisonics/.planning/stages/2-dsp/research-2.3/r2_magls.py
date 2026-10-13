"""R2 — own numpy MagLS vs spaudiopy.decoder.magls_bin (oracle), orders 1/2/3, SADIE II D1 48 k.
Own code written from the deep-dive §2.3–2.5 + spaudiopy's documented 'avg' continuation.
Comparison is in the SIGNAL domain (reconstructed binaural IR at test directions), which is
convention-free (spaudiopy's Y is N3D-ACN; ours is SN3D-ACN, AmbiX)."""
import numpy as np, sofar, time, sys, warnings
warnings.filterwarnings('ignore')
from math import factorial
from spaudiopy import decoder, sig, sph

SOFA = "/Users/taylorbrook/Dev/VST-development/plugins/O-simpleAmbisonics/.cache/sadie/D1_HRIR_SOFA/D1_48K_24bit_256tap_FIR_SOFA.sofa"
s = sofar.read_sofa(SOFA, verbose=False)
ir = s.Data_IR                      # (Q, 2, 256)
fs = float(s.Data_SamplingRate)
azd = s.SourcePosition[:, 0]; eld = s.SourcePosition[:, 1]
az = np.deg2rad(azd); el = np.deg2rad(eld)
Q, _, T = ir.shape
C, R_HEAD = 343.0, 0.0875

# ── own SN3D / ACN real SH (generic triple product, no Condon–Shortley, sin(|m|a) for m<0) ──
def legendre(n, m, s_, c_):
    if n == 0: return np.ones_like(s_)
    if n == 1: return s_ if m == 0 else c_
    if n == 2: return [0.5*(3*s_*s_-1), 3*s_*c_, 3*c_*c_][m]
    if n == 3: return [0.5*s_*(5*s_*s_-3), 1.5*c_*(5*s_*s_-1), 15*s_*c_*c_, 15*c_*c_*c_][m]
def sn3d_matrix(azr, elr, order):
    s_, c_ = np.sin(elr), np.cos(elr)
    cols = []
    for n in range(order+1):
        for m in range(-n, n+1):
            am = abs(m)
            N = np.sqrt((1.0 if m == 0 else 2.0) * factorial(n-am) / factorial(n+am))
            Tm = np.sin(am*azr) if m < 0 else (np.ones_like(azr) if m == 0 else np.cos(m*azr))
            cols.append(N * legendre(n, am, s_, c_) * Tm)
    return np.stack(cols, axis=1)          # (Q, (N+1)^2)

def magls_own(hrirs, Ymat, f_a, nfft=1024, blend=False, hf_cont='avg', n_delta=5):
    """hrirs (Q, L) one ear. Returns (nsh, nfft) real IR (not truncated)."""
    H = np.fft.rfft(hrirs, n=nfft, axis=1)          # (Q, nb)
    freqs = np.fft.rfftfreq(nfft, 1/fs)
    nb = len(freqs)
    Yp = np.linalg.pinv(Ymat)                        # (nsh, Q)
    nsh = Ymat.shape[1]
    k_a = int(np.argmin(np.abs(freqs - f_a)))        # spaudiopy: nearest bin
    Hls = Yp @ H                                      # LS at every bin (nsh, nb)
    out = Hls.copy()
    if hf_cont == 'avg':
        ph = np.unwrap(np.angle(Hls[0, :k_a]))
        dphi = np.mean(np.diff(ph)[k_a-n_delta-1:k_a-1])
    else:
        dphi = 0.0
    if not blend:
        k_start = k_a
        for k in range(k_start, nb):
            phi = np.angle(Ymat @ out[:, k-1]) + dphi
            out[:, k] = Yp @ (np.abs(H[:, k]) * np.exp(1j*phi))
    else:
        # half-octave blend: MagLS recursion starts at f_a/√2, output = α·LS + (1−α)·MagLS
        k_lo = int(np.argmin(np.abs(freqs - f_a/np.sqrt(2))))
        mag = Hls.copy()
        for k in range(k_lo, nb):
            phi = np.angle(Ymat @ mag[:, k-1]) + dphi
            mag[:, k] = Yp @ (np.abs(H[:, k]) * np.exp(1j*phi))
        with np.errstate(divide='ignore'):
            lr = np.log2(np.maximum(freqs, 1e-9) / f_a)
        alpha = np.where(lr < -0.5, 1.0, np.where(lr > 0.5, 0.0, np.cos(0.5*np.pi*lr)**2))
        out = alpha[None, :] * Hls + (1-alpha)[None, :] * mag
    return np.fft.irfft(out, n=nfft, axis=1), k_a

def taper_like_spaudiopy(h, L=256):
    h = h[..., :L].copy()
    h[..., -32:] *= np.hanning(64)[32:]
    h[..., :4] *= np.hanning(8)[:4]
    return h

# spaudiopy oracle
hr = sig.HRIRs(ir[:, 0, :], ir[:, 1, :], az, np.pi/2 - el, fs)
Ysp = {N: sph.sh_matrix(N, az, np.pi/2 - el, 'real') for N in (1, 2, 3)}

# test directions for the signal-domain comparison (deg)
tests = [(0, 0), (90, 0), (-90, 0), (180, 0), (37, -12), (0, 90), (-120, 45), (60, -30)]
t_az = np.deg2rad([t[0] for t in tests]); t_el = np.deg2rad([t[1] for t in tests])

print(f"Q {Q} taps {T} fs {fs}")
for N in (1, 2, 3):
    f_a = N * C / (2*np.pi*R_HEAD)
    Y = sn3d_matrix(az, el, N)
    t0 = time.time()
    own = np.stack([magls_own(ir[:, e, :], Y, f_a)[0] for e in (0, 1)])       # (2, nsh, 1024)
    k_a = magls_own(ir[:, 0, :], Y, f_a)[1]
    own_t = taper_like_spaudiopy(own)
    t1 = time.time()
    orc = decoder.magls_bin(hr, N, f_trans=f_a, hf_cont='avg')              # (2, nsh, 256) N3D
    t2 = time.time()
    # signal-domain reconstruction at test dirs
    Yt_own = sn3d_matrix(t_az, t_el, N)                     # (8, nsh)
    Yt_sp = sph.sh_matrix(N, t_az, np.pi/2 - t_el, 'real')
    rec_own = np.einsum('dn,ent->edt', Yt_own, own_t)       # (2, 8, 256)
    rec_sp = np.einsum('dn,ent->edt', Yt_sp, orc)
    err = np.max(np.abs(rec_own - rec_sp)); ref = np.max(np.abs(rec_sp))
    # per-channel conversion check: SN3D h = N3D h · r_n  with r_n = Y_n3d/Y_sn3d (constant per order)
    ratio = np.array([float(np.dot(Ysp[N][:, c], Y[:, c]) / np.dot(Y[:, c], Y[:, c])) for c in range(Y.shape[1])])
    conv_err = np.max(np.abs(own_t - orc * ratio[None, :, None]))
    # magnitude-response agreement per SH channel / ear in dB (bins up to 20 kHz, where |H| is not tiny)
    Fo = np.fft.rfft(own_t, n=1024, axis=-1); Fs_ = np.fft.rfft(orc * ratio[None, :, None], n=1024, axis=-1)
    fr = np.fft.rfftfreq(1024, 1/fs); band = (fr > 50) & (fr < 20000)
    mo = np.abs(Fo[..., band]); ms = np.abs(Fs_[..., band])
    big = ms > 1e-3 * ms.max()
    mag_db = np.max(np.abs(20*np.log10(mo[big]/ms[big])))
    print(f"order {N}: f_a {f_a:.1f} Hz → bin {k_a} ({k_a*fs/1024:.1f} Hz); own {t1-t0:.1f}s, spaudiopy {t2-t1:.1f}s")
    print(f"   signal-domain max|err| {err:.3e} (peak {ref:.3f}, rel {err/ref:.2e}) at 8 test dirs")
    print(f"   per-order N3D→SN3D ratios {np.unique(np.round(ratio, 6))}; coefficient max|err| after conversion {conv_err:.3e}")
    print(f"   magnitude agreement (|H|>1e-3·max, 50 Hz–20 kHz): max {mag_db:.2e} dB")
    # the blend variant — how much it moves the filters vs the hard switch
    ownb = np.stack([magls_own(ir[:, e, :], Y, f_a, blend=True)[0] for e in (0, 1)])
    ownb_t = taper_like_spaudiopy(ownb)
    rec_b = np.einsum('dn,ent->edt', Yt_own, ownb_t)
    Fb = np.fft.rfft(rec_b, n=1024, axis=-1); Fh = np.fft.rfft(rec_own, n=1024, axis=-1)
    bb = (fr > f_a/2) & (fr < 2*f_a)
    d_db = 20*np.log10(np.abs(Fb[..., bb]) / np.maximum(np.abs(Fh[..., bb]), 1e-12))
    print(f"   half-octave blend vs hard switch: reconstructed |H| differs by max {np.max(np.abs(d_db)):.2f} dB in [f_a/2, 2f_a]; time-domain max|Δ| {np.max(np.abs(rec_b-rec_own)):.3e}")
    # spaudiopy default f_trans
    print(f"   spaudiopy default f_trans would be N·600 = {N*600} Hz (bin {int(np.argmin(np.abs(fr - N*600)))})")
    np.savez(f"/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/c9ee1982-4683-4a8c-9e4c-dc2d06c371de/scratchpad/own_magls_{N}.npz",
             full=own, full_blend=ownb, k_a=k_a, f_a=f_a)
print("done")
