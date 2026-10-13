import numpy as np, os
from scipy import signal
from fractions import Fraction
D = "r6/build/r6_artefacts/Release"
h = np.loadtxt(os.path.join(D, "h48.txt")); j1 = np.loadtxt(os.path.join(D, "h48000_juce.txt"))
g1 = np.sum(j1[100:100+256]) / np.sum(h)
print(f"ratio 1.0 DC gain sum(J)/sum(h) = {g1:.6f} ({20*np.log10(g1):+.4f} dB); max|J/g1 - h| = {np.max(np.abs(j1[100:356]/g1 - h)):.3e}")
for fs in (96000, 44100, 88200, 192000):
    j = np.loadtxt(os.path.join(D, f"h{fs}_juce.txt")); ratio = 48000/fs; zpre = 60 if fs in (44100, 88200) else 0; d = int(round((100+zpre)/ratio))
    fr = Fraction(fs, 48000); up, down = fr.numerator, fr.denominator
    L = int(np.ceil(256*fs/48000)); L64 = ((L+63)//64)*64
    seg = j[d:d+L64]
    for name, g in (("poly β5", signal.resample_poly(h, up, down)), ("poly β14", signal.resample_poly(h, up, down, window=('kaiser', 14.0)))):
        gg = np.zeros(L64); n = min(L64, g.size); gg[:n] = g[:n]
        gdc = np.sum(seg)/np.sum(gg)                     # DC calibration from the data itself
        e_raw = np.max(np.abs(seg - gg)); e_cal = np.max(np.abs(seg/gdc - gg)); e_g1 = np.max(np.abs(seg/g1 - gg))
        F1 = np.fft.rfft(seg/gdc, 8192); F2 = np.fft.rfft(gg, 8192); f = np.fft.rfftfreq(8192, 1/fs)
        band = (f > 100) & (f < 20000) & (np.abs(F2) > 1e-2*np.abs(F2).max()); mdb = np.max(np.abs(20*np.log10(np.abs(F1[band])/np.abs(F2[band]))))
        band2 = (f > 100) & (f < 16000) & (np.abs(F2) > 1e-2*np.abs(F2).max()); mdb2 = np.max(np.abs(20*np.log10(np.abs(F1[band2])/np.abs(F2[band2]))))
        print(f"fs {fs} vs {name:8s}: DC gain {gdc:.5f}; max|err| raw {e_raw:.2e} → /g1 {e_g1:.2e} → /gdc {e_cal:.2e} (rel {e_cal/np.abs(gg).max():.2e}); |H| diff ≤ {mdb:.3f} dB (<20k) / {mdb2:.3f} dB (<16k)")
# impulse-train calibration idea: what does JUCE give for a Kronecker delta? emulate from the ratio-1 data only (we have no impulse dump) — note for the C++ follow-up
