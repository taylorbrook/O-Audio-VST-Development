"""R2 debug — where do own MagLS and spaudiopy diverge? Compare the LS region (convention), dphi, per-bin spectra."""
import numpy as np, sofar, warnings
warnings.filterwarnings('ignore')
from math import factorial
from spaudiopy import decoder, sig, sph
exec(open("/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/c9ee1982-4683-4a8c-9e4c-dc2d06c371de/scratchpad/r2_magls.py").read().split("# spaudiopy oracle")[0].split("# ── own SN3D")[1].replace("", ""), globals()) if False else None

SOFA = "/Users/taylorbrook/Dev/VST-development/plugins/O-simpleAmbisonics/.cache/sadie/D1_HRIR_SOFA/D1_48K_24bit_256tap_FIR_SOFA.sofa"
s = sofar.read_sofa(SOFA, verbose=False)
ir = s.Data_IR; fs = float(s.Data_SamplingRate)
az = np.deg2rad(s.SourcePosition[:, 0]); el = np.deg2rad(s.SourcePosition[:, 1])
C, R_HEAD = 343.0, 0.0875

def legendre(n, m, s_, c_):
    if n == 0: return np.ones_like(s_)
    if n == 1: return s_ if m == 0 else c_
    if n == 2: return [0.5*(3*s_*s_-1), 3*s_*c_, 3*c_*c_][m]
    if n == 3: return [0.5*s_*(5*s_*s_-3), 1.5*c_*(5*s_*s_-1), 15*s_*c_*c_, 15*c_*c_*c_][m]
def sn3d_matrix(azr, elr, order):
    s_, c_ = np.sin(elr), np.cos(elr); cols = []
    for n in range(order+1):
        for m in range(-n, n+1):
            am = abs(m); N = np.sqrt((1.0 if m == 0 else 2.0) * factorial(n-am) / factorial(n+am))
            Tm = np.sin(am*azr) if m < 0 else (np.ones_like(azr) if m == 0 else np.cos(m*azr))
            cols.append(N * legendre(n, am, s_, c_) * Tm)
    return np.stack(cols, axis=1)

hr = sig.HRIRs(ir[:, 0, :], ir[:, 1, :], az, np.pi/2 - el, fs)
print("HRIRs intact:", np.allclose(hr.left, ir[:, 0, :]), np.allclose(hr.right, ir[:, 1, :]), hr.left.shape, "azi/zen intact", np.allclose(hr.azi, az), np.allclose(hr.zen, np.pi/2-el))
for N in (1, 3):
    Y = sn3d_matrix(az, el, N); Ysp = sph.sh_matrix(N, az, np.pi/2 - el, 'real')
    ratio = np.array([float(np.dot(Ysp[:, c], Y[:, c]) / np.dot(Y[:, c], Y[:, c])) for c in range(Y.shape[1])])
    resid = np.max(np.abs(Ysp - Y * ratio[None, :]))
    print(f"order {N}: Y proportional per column? residual {resid:.2e}; ratios {np.round(ratio, 6)}")
    # LS region signal-domain comparison + dphi
    nfft = 1024; H = np.fft.rfft(ir[:, 0, :], n=nfft, axis=1); freqs = np.fft.rfftfreq(nfft, 1/fs)
    f_a = N*C/(2*np.pi*R_HEAD); k_a = int(np.argmin(np.abs(freqs - f_a)))
    Yp = np.linalg.pinv(Y); Hls = Yp @ H
    ph = np.unwrap(np.angle(Hls[0, :k_a])); dphi_own = np.mean(np.diff(ph)[k_a-6:k_a-1])
    orc = decoder.magls_bin(hr, N, f_trans=f_a, hf_cont='avg')
    Osp = np.fft.rfft(orc[0], n=nfft, axis=1)       # spaudiopy left, (nsh, nb) — N3D; truncated+tapered though
    # redo own with the same truncation/taper for a per-bin comparison
    out = Hls.copy()
    for k in range(k_a, len(freqs)):
        phi = np.angle(Y @ out[:, k-1]) + dphi_own
        out[:, k] = Yp @ (np.abs(H[:, k]) * np.exp(1j*phi))
    h_own = np.fft.irfft(out, n=nfft, axis=1)[:, :256].copy(); h_own[:, -32:] *= np.hanning(64)[32:]; h_own[:, :4] *= np.hanning(8)[:4]
    Oown = np.fft.rfft(h_own, n=nfft, axis=1) / ratio[:, None]          # to N3D for a direct per-coefficient compare
    d = np.abs(Oown - Osp); mag = np.abs(Osp)
    for lo, hi, name in [(0, k_a, "LS region"), (k_a, k_a+5, "first 5 MagLS bins"), (k_a+5, 200, "mid"), (200, 513, "HF")]:
        print(f"   {name:20s} max|ΔH| {d[:, lo:hi].max():.3e} vs max|H| {mag[:, lo:hi].max():.3e}; max|Δ|/|H| on strong bins "
              f"{np.max(d[:, lo:hi][mag[:, lo:hi] > 0.1*mag.max()] / mag[:, lo:hi][mag[:, lo:hi] > 0.1*mag.max()]):.3e}")
    # spaudiopy's dphi
    sp_dphi = np.mean(np.diff(np.unwrap(np.angle((np.linalg.pinv(Ysp) @ H)[0, :k_a])))[k_a-6:k_a-1])
    print(f"   dphi own {dphi_own:.6f} spaudiopy-form {sp_dphi:.6f} rad/bin → group delay {-dphi_own/(2*np.pi*freqs[1])*fs:.2f} samples")
    # time-domain W-channel left: peak positions
    print(f"   W left peak index own {np.argmax(np.abs(h_own[0]))} spaudiopy {np.argmax(np.abs(orc[0,0]))}; max|h| own {np.abs(h_own[0]).max():.4f} sp(N3D→SN3D) {np.abs(orc[0,0]*ratio[0]).max():.4f}")
    # try: is spaudiopy's HF phase from the previous bin of the *tapered/truncated*? no. Try matching with pinv differences:
    print(f"   pinv agreement (scaled): {np.max(np.abs(np.linalg.pinv(Ysp) - Yp / ratio[:, None])):.2e}")
