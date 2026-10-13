"""R2/R3/R4/R7/R8 — MagLS validated on the OBJECTIVE (magnitude fit) vs spaudiopy; transcription identity;
truncation / onset / peak / level match / ILD-ITD / stimulus liveness numbers from the own bank."""
import numpy as np, sofar, warnings, time
warnings.filterwarnings('ignore')
from math import factorial
from spaudiopy import decoder, sig, sph

SOFA = "/Users/taylorbrook/Dev/VST-development/plugins/O-simpleAmbisonics/.cache/sadie/D1_HRIR_SOFA/D1_48K_24bit_256tap_FIR_SOFA.sofa"
s = sofar.read_sofa(SOFA, verbose=False)
ir = s.Data_IR; fs = float(s.Data_SamplingRate)
az = np.deg2rad(s.SourcePosition[:, 0]); el = np.deg2rad(s.SourcePosition[:, 1])
Q = ir.shape[0]; C, R_HEAD = 343.0, 0.0875; NFFT = 1024
freqs = np.fft.rfftfreq(NFFT, 1/fs)

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

H = np.fft.rfft(ir, n=NFFT, axis=2)                        # (Q, 2, nb)

def magls(Ymat, f_a, mode='hard', n_delta=5):
    """Both ears. mode: 'hard' (spaudiopy-identical structure) | 'phasor' (own half-octave target blend)."""
    Yp = np.linalg.pinv(Ymat); nsh = Ymat.shape[1]; nb = len(freqs)
    k_a = int(np.argmin(np.abs(freqs - f_a)))
    out = np.zeros((2, nsh, nb), complex)
    for e in (0, 1):
        Hls = Yp @ H[:, e, :]
        o = Hls.copy()
        ph = np.unwrap(np.angle(Hls[0, :k_a])); dphi = np.mean(np.diff(ph)[k_a-n_delta-1:k_a-1])
        if mode == 'hard':
            for k in range(k_a, nb):
                phi = np.angle(Ymat @ o[:, k-1]) + dphi
                o[:, k] = Yp @ (np.abs(H[:, e, k]) * np.exp(1j*phi))
        else:
            # half-octave transition on the TARGET: unit phasors of the measured phase and the continued phase are
            # interpolated (α from the deep-dive), renormalised, then ONE LS fit per bin — no complex blend of solutions.
            lr = np.log2(np.maximum(freqs, 1e-9) / f_a)
            alpha = np.where(lr < -0.5, 1.0, np.where(lr > 0.5, 0.0, np.cos(0.5*np.pi*lr)**2))
            k_lo = int(np.argmin(np.abs(freqs - f_a/np.sqrt(2))))
            for k in range(k_lo, nb):
                cont = np.exp(1j*(np.angle(Ymat @ o[:, k-1]) + dphi))
                meas = np.exp(1j*np.angle(H[:, e, k]))
                mix = alpha[k]*meas + (1-alpha[k])*cont
                mix = mix / np.maximum(np.abs(mix), 1e-12)
                o[:, k] = Yp @ (np.abs(H[:, e, k]) * mix)
        out[e] = o
    return out, k_a

def fit_error_db(Ymat, hnm, k_lo, k_hi):
    """RMS over directions of the magnitude fit error in dB per bin, then mean over bins."""
    rec = np.einsum('qn,enk->eqk', Ymat, hnm[:, :, k_lo:k_hi])
    meas = np.abs(H[:, :, k_lo:k_hi]).transpose(1, 0, 2)
    r = 20*np.log10(np.maximum(np.abs(rec), 1e-9) / np.maximum(meas, 1e-9))
    return np.sqrt(np.mean(r**2))

hr = sig.HRIRs(ir[:, 0, :], ir[:, 1, :], az, np.pi/2 - el, fs)
print("spaudiopy HRIRs dtype:", hr.left.dtype)
band_edges = [(50, 600), (600, 2000), (2000, 5000), (5000, 10000), (10000, 20000)]
def bands(): return [(int(np.argmin(np.abs(freqs-a))), int(np.argmin(np.abs(freqs-b))), f"{a//1000 if a>=1000 else a}{'k' if a>=1000 else ''}–{b//1000}k") for a, b in band_edges]

# ── transcription identity: spaudiopy's own code structure with ITS Y vs decoder.magls_bin ──
for N in (1, 3):
    f_a = N*C/(2*np.pi*R_HEAD)
    Ysp = sph.sh_matrix(N, az, np.pi/2 - el, 'real')
    mine_on_Ysp, _ = magls(Ysp, f_a, 'hard')
    h_mine = np.fft.irfft(mine_on_Ysp, n=NFFT, axis=2)[..., :256].copy(); h_mine[..., -32:] *= np.hanning(64)[32:]; h_mine[..., :4] *= np.hanning(8)[:4]
    orc = decoder.magls_bin(hr, N, f_trans=f_a, hf_cont='avg')
    print(f"order {N}: own loop on spaudiopy's N3D Y vs decoder.magls_bin: max|Δ| {np.max(np.abs(h_mine - orc)):.3e} (peak {np.abs(orc).max():.3f})")
    # pinv of column-scaled Y: how far is the projector?
    Y = sn3d_matrix(az, el, N)
    P1 = Y @ np.linalg.pinv(Y); P2 = Ysp @ np.linalg.pinv(Ysp)
    print(f"   projector Y·Y⁺ SN3D vs N3D: max|Δ| {np.max(np.abs(P1 - P2)):.2e}")

# ── objective comparison + bank numbers ──
results = {}
for N in (1, 2, 3):
    f_a = N*C/(2*np.pi*R_HEAD)
    Y = sn3d_matrix(az, el, N); Ysp = sph.sh_matrix(N, az, np.pi/2 - el, 'real')
    own, k_a = magls(Y, f_a, 'hard')
    ownp, _ = magls(Y, f_a, 'phasor')
    orc = decoder.magls_bin(hr, N, f_trans=f_a, hf_cont='avg')
    Osp = np.fft.rfft(orc, n=NFFT, axis=2)             # spaudiopy, truncated+tapered (N3D)
    # truncate own the same way for a like-for-like objective
    def trunc(hnm):
        h = np.fft.irfft(hnm, n=NFFT, axis=2)[..., :256].copy(); h[..., -32:] *= np.hanning(64)[32:]; h[..., :4] *= np.hanning(8)[:4]
        return np.fft.rfft(h, n=NFFT, axis=2), h
    Oown, hown = trunc(own); Oownp, hownp = trunc(ownp)
    print(f"\norder {N} (f_a {f_a:.0f} Hz, bin {k_a}) — magnitude fit error, RMS dB over 8802 dirs × bins (after 256-tap truncation+taper):")
    print(f"   {'band':10s} {'own-hard':>9s} {'own-phasor':>11s} {'spaudiopy':>10s} {'LS-only':>8s}")
    Yp = np.linalg.pinv(Y); Hls = np.stack([Yp @ H[:, e, :] for e in (0, 1)]); Ols, _ = trunc(Hls)
    for lo, hi, name in bands():
        print(f"   {name:10s} {fit_error_db(Y, Oown, lo, hi):9.3f} {fit_error_db(Y, Oownp, lo, hi):11.3f} {fit_error_db(Ysp, Osp, lo, hi):10.3f} {fit_error_db(Y, Ols, lo, hi):8.3f}")
    # untruncated LS region identity (own vs own-phasor below f_a/√2 and vs plain LS)
    print(f"   untruncated: own-hard == LS below f_a: {np.max(np.abs(own[:, :, :k_a] - Hls[:, :, :k_a])):.2e}")
    # ── R3: truncation energy, onset, pre-ringing (own-hard, untruncated 1024 IR) ──
    hfull = np.fft.irfft(own, n=NFFT, axis=2)                    # (2, nsh, 1024)
    e_tot = np.sum(hfull**2);
    print(f"   R3 energy outside taps [0,256): {100*(1-np.sum(hfull[..., :256]**2)/e_tot):.4f}%  outside [0,192): {100*(1-np.sum(hfull[..., :192]**2)/e_tot):.4f}%  wrap-around tail (taps 900..1023): {100*np.sum(hfull[..., 900:]**2)/e_tot:.2e}%")
    env = np.max(np.abs(hfull), axis=(0, 1))                      # bank envelope per tap
    pk = env.max()
    for thr in (1e-2, 1e-3, 1e-4):
        first = int(np.argmax(env > thr*pk));
        print(f"   R3 first tap where bank envelope > {thr:g}·peak: {first}", end="")
    print(f"; envelope peak at tap {int(np.argmax(env))}; peak |h| {pk:.4f}")
    # per-direction reconstructed onset (10 % of its own peak) over all 8802 dirs, through the truncated bank
    rec = np.einsum('qn,ent->eqt', Y, hown)                       # (2, Q, 256)
    on = np.array([[int(np.argmax(np.abs(rec[e, q]) > 0.1*np.abs(rec[e, q]).max())) for q in range(0, Q, 7)] for e in (0, 1)])
    print(f"   R3 reconstructed onsets (10 %) over directions: min {on.min()} median {int(np.median(on))} max {on.max()}  (raw SADIE: 64 / 90 / 120)")
    # ── R4: level match — A-weighted white level, front source, mean of ears, vs stereo 0.75 ──
    def a_weight(f):
        f2 = f**2
        ra = 12194**2 * f2**2 / ((f2 + 20.6**2) * np.sqrt((f2 + 107.7**2) * (f2 + 737.9**2)) * (f2 + 12194**2))
        return ra / (12194**2 * 1000**2 / ((1000**2 + 20.6**2) * np.sqrt((1000**2 + 107.7**2) * (1000**2 + 737.9**2)) * (1000**2 + 12194**2)))
    A2 = a_weight(np.maximum(freqs, 1.0))**2; A2[0] = 0
    def front_level(hnm_spec, Ymat, azd=0.0, eld=0.0, weighted=True):
        y = Ymat if Ymat.ndim == 1 else None
        yv = sn3d_matrix(np.array([np.deg2rad(azd)]), np.array([np.deg2rad(eld)]), N)[0]
        rec = np.einsum('n,enk->ek', yv, hnm_spec)
        w = A2 if weighted else np.ones_like(A2)
        p = np.mean([np.sum(w * np.abs(rec[e])**2) for e in (0, 1)])
        return np.sqrt(p / np.sum(w))                                # white-excitation RMS gain
    stereo_front = 0.75
    lvl = front_level(Oown, Y); lvl_u = front_level(Oown, Y, weighted=False)
    match_db = 20*np.log10(stereo_front / lvl)
    print(f"   R4 front/order {N}: A-weighted white gain {lvl:.4f} ({20*np.log10(lvl):+.2f} dBFS-ish), unweighted {lvl_u:.4f}; match to stereo 0.75 = {match_db:+.2f} dB")
    results[N] = dict(Oown=Oown, hown=hown, Y=Y, lvl=lvl, match_db=match_db, hfull=hfull)
    # W channel LF flatness (own-hard): |H_W| over 50–500 Hz relative to 200 Hz
    W = np.abs(Oown[:, 0, :]); b = (freqs > 50) & (freqs < 500); k200 = int(np.argmin(np.abs(freqs-200)))
    print(f"   R4 W-channel LF flatness 50–500 Hz: L {20*np.log10(W[0][b].max()/W[0][k200]):+.2f}/{20*np.log10(W[0][b].min()/W[0][k200]):+.2f} dB  R {20*np.log10(W[1][b].max()/W[1][k200]):+.2f}/{20*np.log10(W[1][b].min()/W[1][k200]):+.2f} dB")
    # ── R8: ILD / ITD at ±90° through the truncated bank ──
    for azd in (90.0, -90.0):
        yv = sn3d_matrix(np.array([np.deg2rad(azd)]), np.array([0.0]), N)[0]
        r = np.einsum('n,ent->et', yv, hown)                       # (2, 256)
        rmsL, rmsR = np.sqrt(np.mean(r[0]**2)), np.sqrt(np.mean(r[1]**2))
        onL = int(np.argmax(np.abs(r[0]) > 0.1*np.abs(r[0]).max())); onR = int(np.argmax(np.abs(r[1]) > 0.1*np.abs(r[1]).max()))
        xc = np.correlate(r[0], r[1], 'full'); lag = int(np.argmax(np.abs(xc))) - (len(r[1]) - 1)
        # ITD via cross-correlation of LP-filtered (< 1.5 kHz) signals — the usual
        Rf = np.fft.rfft(r, n=1024, axis=1); Rf[:, freqs > 1500] = 0; rl = np.fft.irfft(Rf, n=1024, axis=1)
        xcl = np.correlate(rl[0], rl[1], 'full'); lagl = int(np.argmax(xcl)) - (len(rl[1]) - 1)
        # per-band ILD
        Fb = np.fft.rfft(r, n=1024, axis=1); ild_bands = []
        for lo, hi in [(200, 1000), (1000, 3000), (3000, 8000), (8000, 16000)]:
            m = (freqs >= lo) & (freqs < hi); ild_bands.append(10*np.log10(np.sum(np.abs(Fb[0][m])**2)/np.sum(np.abs(Fb[1][m])**2)))
        print(f"   R8 az {azd:+.0f}: broadband ILD L/R {20*np.log10(rmsL/rmsR):+.2f} dB; per-band {['%+.1f' % v for v in ild_bands]}; onset L {onL} R {onR} (Δ {onR-onL:+d}); xcorr lag (L vs R) {lag:+d}, LP<1.5k lag {lagl:+d} samples")
    # ── R7: stimulus liveness — level difference order-1 vs order-3 (computed after the loop) ──

# cross-order numbers
print("\nR4 cross-order front level (A-weighted white gain), own-hard, common scale NOT yet applied:")
for N in (1, 2, 3):
    print(f"   order {N}: {results[N]['lvl']:.4f}  ({20*np.log10(results[N]['lvl']/results[3]['lvl']):+.2f} dB re order 3); match-to-0.75 would be {results[N]['match_db']:+.2f} dB")
scale = 0.75 / results[3]['lvl']
print(f"   → common scale s = 0.75/level3 = {scale:.4f} ({20*np.log10(scale):+.2f} dB); after it: order1 {20*np.log10(scale*results[1]['lvl']/0.75):+.2f} dB, order2 {20*np.log10(scale*results[2]['lvl']/0.75):+.2f} dB vs stereo")
# headroom: max |H| over bins and directions at order 3 after the scale
Y3 = results[3]['Y']; rec3 = np.einsum('qn,enk->eqk', Y3[::5], results[3]['Oown']) * scale
print(f"   headroom: max |H(f)| over 1/5 of the directions × bins at order 3 after scale: {np.abs(rec3).max():.3f} ({20*np.log10(np.abs(rec3).max()):+.2f} dB) at f {freqs[np.unravel_index(np.argmax(np.abs(rec3)), rec3.shape)[2]]:.0f} Hz; peak tap after scale {scale*np.abs(results[3]['hown']).max():.4f}")
# R7: stimulus liveness — front source, order 1 vs order 3, tone levels at 100 Hz / 3 kHz / 6 kHz; and vs stereo
for f0 in (100, 1000, 3000, 6000):
    k = int(np.argmin(np.abs(freqs - f0)))
    yv1 = sn3d_matrix(np.array([0.0]), np.array([0.0]), 1)[0]; yv3 = sn3d_matrix(np.array([0.0]), np.array([0.0]), 3)[0]
    g1 = np.abs(np.einsum('n,enk->ek', yv1, results[1]['Oown'])[:, k]) * scale; g3 = np.abs(np.einsum('n,enk->ek', yv3, results[3]['Oown'])[:, k]) * scale
    print(f"   R7 front tone {f0} Hz: |H| order1 L/R {g1[0]:.3f}/{g1[1]:.3f}, order3 {g3[0]:.3f}/{g3[1]:.3f} → Δ {20*np.log10(g3[0]/g1[0]):+.2f}/{20*np.log10(g3[1]/g1[1]):+.2f} dB; vs stereo 0.75: order3 {20*np.log10(g3[0]/0.75):+.2f} dB")
# side source (az +90) order 1 vs 3 at 3 kHz — where the ILD differs most
k = int(np.argmin(np.abs(freqs - 3000)))
for N in (1, 3):
    yv = sn3d_matrix(np.array([np.pi/2]), np.array([0.0]), N)[0]; g = np.abs(np.einsum('n,enk->ek', yv, results[N]['Oown'])[:, k]) * scale
    print(f"   R7 az +90 tone 3 kHz order {N}: L {g[0]:.3f} R {g[1]:.3f} (ILD {20*np.log10(g[0]/g[1]):+.1f} dB)")
np.savez("/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/c9ee1982-4683-4a8c-9e4c-dc2d06c371de/scratchpad/own_bank.npz",
         h1=results[1]['hown']*scale, h2=results[2]['hown']*scale, h3=results[3]['hown']*scale, scale=scale)
print("done")
