import numpy as np, time
from wt_model import *

VOW = {"A":(730,1090,2440),"E":(530,1840,2480),"I":(270,2290,3010),"O":(570,840,2410),"U":(300,870,2240)}
ORDER = ["A","E","I","O","U"]; ANCH = [1, 8.75, 16.5, 24.25, 32]; BW = (90.0,110.0,170.0); FREF=110.0
def formants(k):
    for s in range(4):
        if ANCH[s] <= k <= ANCH[s+1]:
            u = (k - ANCH[s])/(ANCH[s+1]-ANCH[s]); a = VOW[ORDER[s]]; b = VOW[ORDER[s+1]]
            return [np.exp((1-u)*np.log(a[i]) + u*np.log(b[i])) for i in range(3)]
def R(f, F, B):
    h = B/2
    return (F*F + h*h) / np.sqrt(((f-F)**2 + h*h)*((f+F)**2 + h*h))
def formant_frame(k):
    Fs = formants(k); n = np.arange(1, 49)
    env = np.ones(48)
    for i in range(3): env *= R(n*FREF, Fs[i], BW[i])
    return (1.0/n) * env

banks = {
 "saw":     [build_levels(*sine_phase_spectrum(saw_frame(k))) for k in range(1,33)],
 "square":  [build_levels(*sine_phase_spectrum(square_frame(k))) for k in range(1,33)],
 "pulse":   [build_levels(*pulse_frame_spectrum(k)) for k in range(1,33)],
 "formant": [build_levels(*sine_phase_spectrum(formant_frame(k))) for k in range(1,33)],
 "drive":   [build_levels(*drive_frame_spectrum(k)) for k in range(1,33)],
}
def spec(t):  # double FFT of the 2048 float32 samples; bin magnitudes as harmonic amplitudes
    X = np.fft.rfft(t[:N].astype(np.float64)); return np.abs(X) * 2 / N
for name, frames in banks.items():
    peaks = [20*np.log10(np.max(np.abs(f[0][:N]))) for f in frames]
    dc = max(20*np.log10(abs(np.mean(f[0][:N].astype(np.float64))) + 1e-300) for f in frames)
    above = -999.0
    for f in frames:
        for L in range(LEVELS):
            S = spec(f[L]); ref = S.max()
            if KMAX[L] < 1024:
                above = max(above, 20*np.log10(S[KMAX[L]+1:].max()/ref + 1e-300))
    guard_ok = all(np.array_equal(f[L][N], f[L][0]) for f in frames for L in range(LEVELS))
    distinct = len({f[0].tobytes() for f in frames})
    print(f"{name:8s} peak dB [{min(peaks):+.3f},{max(peaks):+.3f}]  DC max {dc:7.1f} dB  max above Kmax {above:7.1f} dB  guards {guard_ok}  distinct {distinct}/32")
    print(f"         frame1 s[1]={frames[0][0][1]:+.4f} s[512]={frames[0][0][512]:+.4f}  frame32 s[0]={frames[31][0][0]:+.4f}")
# Saw frame k exactly 1..k
ok = True
for k in range(1,33):
    S = spec(banks["saw"][k-1][0]); ref = S[1]
    if not (all(S[1:k+1] > 1e-3*ref) and 20*np.log10(S[k+1:].max()/ref) < -120): ok = False; print("saw frame", k, "bad")
print("saw frame k == h1..k :", ok)
# Square
evens = max(20*np.log10(spec(f[0])[2:64:2].max()/spec(f[0]).max()) for f in banks["square"])
S32 = spec(banks["square"][31][0]); top = np.max(np.nonzero(S32 > 1e-6*S32.max()))
print(f"square evens max {evens:.1f} dB, frame32 top harmonic h{top}")
# Drive monotonic
h = np.array([[20*np.log10(spec(f[0])[n]/spec(f[0])[1]) for n in (3,5,7)] for f in banks["drive"]])
print("drive frame1 h3 %.2f dB, frame32 h3 %.2f dB; strictly increasing h3/h5/h7:" % (h[0,0], h[31,0]), [bool(np.all(np.diff(h[:,i])>0)) for i in range(3)])
# Pulse polarity / first null
for k in (1,32):
    d = 0.5*(1/16)**((k-1)/31); print(f"pulse frame {k}: duty {d:.4f}, first null h{round(1/d)}")
print("formant F at frames 1,8.75,16.5,24.25,32:", [ [round(x) for x in formants(k)] for k in ANCH])
# Gibbs: max level peak across all levels/frames
print("max |sample| over all levels:", {n: round(float(max(np.max(np.abs(f[L])) for f in fr for L in range(LEVELS))),3) for n,fr in banks.items()})
