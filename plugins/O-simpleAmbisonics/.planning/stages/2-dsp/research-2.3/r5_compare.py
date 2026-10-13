"""R5 — JUCE WindowedSinc resample (dumped by the r6 scratch) vs scipy resample_poly and FFT resample goldens."""
import numpy as np, os
from scipy import signal
from fractions import Fraction
D = "/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/c9ee1982-4683-4a8c-9e4c-dc2d06c371de/scratchpad/r6/build/r6_artefacts/Release"
h = np.loadtxt(os.path.join(D, "h48.txt"))
print("h48 taps", h.size, "peak", np.abs(h).max())
for fs in (96000, 44100, 88200, 192000):
    j = np.loadtxt(os.path.join(D, f"h{fs}_juce.txt"))
    ratio = 48000 / fs
    zpre = 60 if fs in (44100, 88200, 176400) else 0
    delay = (100 + zpre) / ratio
    fr = Fraction(fs, 48000); up, down = fr.numerator, fr.denominator
    L = int(np.ceil(256 * fs / 48000)); L64 = ((L + 63) // 64) * 64
    # goldens: resample_poly (kaiser β 5, default) and a long-kernel variant, and FFT resample on a zero-padded block
    g_poly = signal.resample_poly(h, up, down)
    g_poly_long = signal.resample_poly(h, up, down, window=('kaiser', 14.0), padtype='constant')
    pad = np.concatenate([h, np.zeros(1024 - 256)])
    g_fft = signal.resample(pad, int(1024 * fs / 48000))[:L64 + 64]
    print(f"fs {fs}: up/down {up}/{down}, taps {L} → rounded {L64} (K {L64//64}), expected JUCE delay {delay} out samples (integer: {float(delay).is_integer()})")
    # integer alignment search near the expected delay
    best = None
    for d in range(int(delay) - 3, int(delay) + 4):
        seg = j[d:d + L64]
        if seg.size < L64: continue
        for name, g in (("poly", g_poly), ("poly_k14", g_poly_long), ("fft", g_fft)):
            gg = np.zeros(L64); n = min(L64, g.size); gg[:n] = g[:n]
            e = np.max(np.abs(seg - gg)); rel = e / np.abs(gg).max()
            # magnitude response difference up to 20 kHz
            F1 = np.fft.rfft(seg, 8192); F2 = np.fft.rfft(gg, 8192); f = np.fft.rfftfreq(8192, 1 / fs)
            band = (f > 100) & (f < 20000) & (np.abs(F2) > 1e-2 * np.abs(F2).max())
            mdb = np.max(np.abs(20 * np.log10(np.abs(F1[band]) / np.abs(F2[band]))))
            if best is None or e < best[0]: best = (e, d, name)
            if d == int(delay):
                print(f"   d {d} vs {name:9s}: max|err| {e:.3e} (rel {rel:.2e}), |H| diff ≤ {mdb:.3f} dB (100 Hz–20 kHz, strong bins); energy beyond tap {L}: {100*np.sum(seg[L:]**2)/np.sum(seg**2):.3e}%")
    print(f"   best integer alignment: d {best[1]} vs {best[2]} max|err| {best[0]:.3e}")
    # tail: how much of the resampled FIR lies beyond L64 (should be ~0: the input is 256 taps + the sinc tails)
    tail = j[int(delay) + L64:int(delay) + L64 + 200]
    print(f"   JUCE output beyond L64: max |x| {np.abs(tail).max():.3e}; pre-ring before delay: max |x| {np.abs(j[:int(delay)]).max():.3e}")
# also: how different are the two scipy goldens from each other (is the golden itself well-defined?)
for fs in (96000, 44100):
    fr = Fraction(fs, 48000); up, down = fr.numerator, fr.denominator
    a = signal.resample_poly(h, up, down); b = signal.resample_poly(h, up, down, window=('kaiser', 14.0))
    print(f"fs {fs}: resample_poly default vs kaiser-14: max|Δ| {np.max(np.abs(a - b)):.3e} (peak {np.abs(a).max():.3f})")
