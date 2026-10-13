import numpy as np, os
D = "r6/build/r6_artefacts/Release"
h = np.loadtxt(os.path.join(D, "h48.txt")); j = np.loadtxt(os.path.join(D, "h48000_juce.txt"))
seg = j[100:100+256]
F1 = np.fft.rfft(seg, 8192); F2 = np.fft.rfft(h, 8192); f = np.fft.rfftfreq(8192, 1/48000)
ratio = np.abs(F1)/np.maximum(np.abs(F2), 1e-12)
strong = np.abs(F2) > 0.05*np.abs(F2).max()
for lo, hi in [(20, 1000), (1000, 5000), (5000, 10000), (10000, 15000), (15000, 20000), (20000, 22000), (22000, 24000)]:
    m = (f >= lo) & (f < hi) & strong
    if m.any(): print(f"ratio-1.0 passband |J|/|H| {lo:5d}–{hi:5d} Hz: mean {20*np.log10(ratio[m].mean()):+.3f} dB, min {20*np.log10(ratio[m].min()):+.3f}, max {20*np.log10(ratio[m].max()):+.3f}")
# phase: is it an integer delay of 100? group delay from phase difference
ph = np.unwrap(np.angle(F1/np.maximum(F2, 1e-12)))
m = (f > 100) & (f < 15000) & strong
gd = -np.gradient(ph, f*2*np.pi)
print("ratio-1.0 group delay vs input (samples, 100 Hz–15 kHz):", np.round(np.percentile(gd[m]*48000, [5, 50, 95]), 3), "(expected 0 after the 100-sample shift)")
print("time-domain max|J-h| at d=100:", np.max(np.abs(seg - h)), "at d=99:", np.max(np.abs(j[99:99+256]-h)), "d=101:", np.max(np.abs(j[101:101+256]-h)))
# the implied kernel: deconvolve via impulse? emulate: JUCE table at integer positions — we can't read the table; infer from a least-squares fit of a 201-tap symmetric kernel
import numpy.linalg as la
# build convolution matrix of h for kernel taps -100..100
L = 201; X = np.zeros((656, L))
for i, m_ in enumerate(range(-100, 101)):
    X[max(0, -m_):656-max(0, m_), i] = 0  # placeholder
# simpler: solve in frequency domain with the full dump length
Hf = np.fft.rfft(np.concatenate([h, np.zeros(656-256)])); Jf = np.fft.rfft(j[100:756] if j.size >= 756 else np.concatenate([j[100:], np.zeros(756-j.size)]))
K = np.fft.irfft(Jf/np.where(np.abs(Hf) > 0.02*np.abs(Hf).max(), Hf, np.inf), n=656)
print("implied kernel centre taps (−3..3):", np.round(K[[-3,-2,-1,0,1,2,3]], 5), "sum of |taps| beyond ±1:", np.sum(np.abs(K[2:-1])))
