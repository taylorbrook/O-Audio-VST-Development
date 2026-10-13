import numpy as np, sofar, warnings
warnings.filterwarnings('ignore')
from math import factorial
from spaudiopy import decoder, sig, sph
SOFA = "/Users/taylorbrook/Dev/VST-development/plugins/O-simpleAmbisonics/.cache/sadie/D1_HRIR_SOFA/D1_48K_24bit_256tap_FIR_SOFA.sofa"
s = sofar.read_sofa(SOFA, verbose=False); ir = s.Data_IR; fs = 48000.0
az = np.deg2rad(s.SourcePosition[:, 0]); el = np.deg2rad(s.SourcePosition[:, 1]); Q = ir.shape[0]
C, R_HEAD, NFFT = 343.0, 0.0875, 1024; freqs = np.fft.rfftfreq(NFFT, 1/fs)
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
H = np.fft.rfft(ir, n=NFFT, axis=2)
def magls(Ymat, f_a, dphi_form='spaudiopy'):
    Yp = np.linalg.pinv(Ymat); nsh = Ymat.shape[1]; nb = len(freqs); k_a = int(np.argmin(np.abs(freqs - f_a)))
    out = np.zeros((2, nsh, nb), complex)
    for e in (0, 1):
        Hls = Yp @ H[:, e, :]; o = Hls.copy()
        uw = np.unwrap(np.angle(Hls[0, :k_a]))
        dphi = np.mean(np.diff(uw[k_a-6:k_a-1])) if dphi_form == 'spaudiopy' else np.mean(np.diff(uw)[k_a-6:k_a-1])
        for k in range(k_a, nb):
            o[:, k] = Yp @ (np.abs(H[:, e, k]) * np.exp(1j*(np.angle(Ymat @ o[:, k-1]) + dphi)))
        out[e] = o
    return out, k_a
def trunc(hnm, T=0):
    h = np.fft.irfft(hnm, n=NFFT, axis=2)[..., T:T+256].copy(); h[..., -32:] *= np.hanning(64)[32:]; h[..., :4] *= np.hanning(8)[:4]; return h
hr = sig.HRIRs(ir[:, 0, :], ir[:, 1, :], az, np.pi/2 - el, fs)
print("== transcription identity with spaudiopy's dphi (slice THEN diff, 4 values) ==")
banks = {}
for N in (1, 2, 3):
    f_a = N*C/(2*np.pi*R_HEAD); Ysp = sph.sh_matrix(N, az, np.pi/2 - el, 'real'); Y = sn3d_matrix(az, el, N)
    own_sp, _ = magls(Ysp, f_a); orc = decoder.magls_bin(hr, N, f_trans=f_a, hf_cont='avg')
    print(f"order {N}: own loop (N3D Y, spaudiopy dphi) vs decoder.magls_bin max|Δ| {np.max(np.abs(trunc(own_sp) - orc)):.3e}")
    own, k_a = magls(Y, f_a)
    ratio = np.array([float(np.dot(Ysp[:, c], Y[:, c]) / np.dot(Y[:, c], Y[:, c])) for c in range(Y.shape[1])])
    print(f"   own SN3D vs spaudiopy·ratio: max|Δ| {np.max(np.abs(trunc(own) - orc*ratio[None, :, None])):.3e} (peak {np.abs(orc*ratio[None,:,None]).max():.3f}) — same filters up to the per-order SN3D factor")
    banks[N] = (own, Y, k_a)
print("\n== R3 trim table (own bank, untruncated 1024-tap IRs; energies relative to the whole bank) ==")
for N in (1, 2, 3):
    hfull = np.fft.irfft(banks[N][0], n=NFFT, axis=2); etot = np.sum(hfull**2)
    row = []
    for T in (0, 16, 24, 32, 40, 48, 56, 64):
        lost_head = 100*np.sum(hfull[..., :T]**2)/etot; lost_tail = 100*np.sum(hfull[..., T+256:]**2)/etot
        row.append(f"T{T}: head {lost_head:.4f}% tail {lost_tail:.4f}%")
    print(f"order {N}: " + " | ".join(row))
    # front / side / earliest onsets (10 % of own peak) through the truncated bank with T = 0 and T = 32
    Y = banks[N][1]
    for T in (0, 32):
        h = trunc(banks[N][0], T); rec = np.einsum('qn,ent->eqt', Y, h)
        on = np.array([[int(np.argmax(np.abs(rec[e, q]) > 0.1*np.abs(rec[e, q]).max())) for q in range(0, Q, 3)] for e in (0, 1)])
        yf = sn3d_matrix(np.array([0.0]), np.array([0.0]), N)[0]; rf = np.einsum('n,ent->et', yf, h)
        onf = [int(np.argmax(np.abs(rf[e]) > 0.1*np.abs(rf[e]).max())) for e in (0, 1)]; pkf = [int(np.argmax(np.abs(rf[e]))) for e in (0, 1)]
        # energy-based onset: first sample where cumulative energy of the front response exceeds 1 %
        ce = np.cumsum(rf**2, axis=1)/np.sum(rf**2, axis=1, keepdims=True); on1 = [int(np.argmax(ce[e] > 0.01)) for e in (0, 1)]
        print(f"   T {T}: onsets(10 %) over dirs min/median/max {on.min()}/{int(np.median(on))}/{on.max()}; FRONT onset10% L/R {onf}, peak {pkf}, 1 %-energy onset {on1}")
print("\n== R4 W LF response (own bank, order 3), relative to 200 Hz ==")
own, Y, k_a = banks[3]; W = np.abs(own[:, 0, :])
k200 = int(np.argmin(np.abs(freqs-200)))
for f0 in (46.875, 93.75, 140.6, 187.5, 281, 375, 469, 562, 750, 1000):
    k = int(np.argmin(np.abs(freqs-f0))); print(f"   {freqs[k]:7.1f} Hz: L {20*np.log10(W[0][k]/W[0][k200]):+.2f} dB R {20*np.log10(W[1][k]/W[1][k200]):+.2f} dB")
# raw SADIE front HRTF LF (is the dip in the data?)
Hf = np.abs(H[12, :, :]); print(f"   raw SADIE front HRTF: 46.9 Hz {20*np.log10(Hf[0][1]/Hf[0][k200]):+.2f} dB, 93.8 Hz {20*np.log10(Hf[0][2]/Hf[0][k200]):+.2f} dB re 200 Hz (left)")
print("\n== R8 ITD by LP cross-correlation per order, LP cutoffs ==")
for N in (1, 2, 3):
    own, Y, k_a = banks[N]; h = trunc(own)
    for azd in (90.0,):
        yv = sn3d_matrix(np.array([np.deg2rad(azd)]), np.array([0.0]), N)[0]; r = np.einsum('n,ent->et', yv, h)
        outs = []
        for fc in (300, 500, 800, 1500):
            Rf = np.fft.rfft(r, n=4096, axis=1); fr = np.fft.rfftfreq(4096, 1/fs); Rf[:, fr > fc] = 0; rl = np.fft.irfft(Rf, n=4096, axis=1)
            xc = np.correlate(rl[0], rl[1], 'full'); lag = int(np.argmax(xc)) - (len(rl[1]) - 1); outs.append(f"LP<{fc}: {lag:+d}")
        # group-delay difference below f_a from the phase of L/R
        Rf = np.fft.rfft(r, n=4096, axis=1); ph = np.unwrap(np.angle(Rf[0]/Rf[1])); band = (fr > 100) & (fr < min(600, N*600))
        gd = -np.gradient(ph, 2*np.pi*fr)*fs; 
        print(f"order {N} az +90: " + ", ".join(outs) + f"; interaural group delay 100–{int(min(600, N*600))} Hz: median {np.median(gd[band]):+.1f} samples (f_a {N*C/(2*np.pi*R_HEAD):.0f} Hz)")
print("\n== R7 side source (az +90) at 100 Hz: bank difference order 1 vs 3 (level + phase) and the predicted snapped-step ratio ==")
k = int(np.argmin(np.abs(freqs-100)))
g = {}
for N in (1, 3):
    own, Y, k_a = banks[N]; h = trunc(own); yv = sn3d_matrix(np.array([np.pi/2]), np.array([0.0]), N)[0]
    r = np.einsum('n,ent->et', yv, h); Rf = np.fft.rfft(r, n=NFFT, axis=1)[:, k]; g[N] = Rf
    print(f"   order {N}: |H(100 Hz)| L {np.abs(Rf[0]):.4f} R {np.abs(Rf[1]):.4f}, phase L {np.degrees(np.angle(Rf[0])):+.1f}° R {np.degrees(np.angle(Rf[1])):+.1f}°")
for e, nm in ((0, 'L'), (1, 'R')):
    step = np.abs(g[3][e] - g[1][e]) * 0.5          # worst-case instantaneous jump of a 0.5-amplitude 100 Hz tone
    base = 0.5 * max(np.abs(g[1][e]), np.abs(g[3][e])) * (2*np.pi*100/fs)**2
    print(f"   {nm}: |ΔH| {np.abs(g[3][e]-g[1][e]):.4f} → worst snapped step {step:.4e} vs static curvature {base:.3e} → predicted negative-control ratio ≈ {step/base:.0f}")
print("\n== float LinearRamp arrival, descending 1→0 ==")
step=np.float32(64.0/(0.02*48000.0)); cur=np.float32(1); n=0
while True:
    d=np.float32(0)-cur; n+=1
    if abs(d)<=step: cur=np.float32(0); break
    cur=np.float32(cur+np.copysign(step,d))
print("   float32 1->0 arrives at step", n, "(0->1 was 15; double 0->1 was 16)")
c=1.0; n=0; sd=64.0/(0.02*48000.0)
while True:
    d=0-c; n+=1
    if abs(d)<=sd: c=0; break
    c+=np.copysign(sd,d)
print("   double 1->0 arrives at step", n)
# 44.1 k and 96 k ramp steps too
for f in (44100.0, 96000.0):
    st=np.float32(64.0/(0.02*f)); cur=np.float32(0); n=0
    while True:
        d=np.float32(1)-cur; n+=1
        if d<=st: break
        cur=np.float32(cur+st)
    sd=64.0/(0.02*f); c=0.0; m=0
    while True:
        d=1-c; m+=1
        if d<=sd: break
        c+=sd
    print(f"   fs {f:.0f}: float 0->1 arrives at step {n}, double at {m} (ceil(0.02·fs/64) = {int(np.ceil(0.02*f/64))})")
