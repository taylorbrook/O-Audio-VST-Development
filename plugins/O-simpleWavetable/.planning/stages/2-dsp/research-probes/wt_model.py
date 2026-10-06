"""Numerical sanity model for O-simpleWavetable Stage 2.1/2.2.

Mirrors ARCHITECTURE.md: 2048-pt tables, 11 mip levels Kmax_L = min(1023, 1024>>L),
DC + Nyquist zeroed, level-0 peak normalised to 1.0, float32 tables, double phase,
linear in-cycle read with guard sample, strict level L = ceil(log2(f*2048/fs)).
"""
import numpy as np, sys, time

N = 2048
LEVELS = 11
KMAX = [min(1023, 1024 >> L) for L in range(LEVELS)]

def spectrum_to_table(re, im):
    """re/im: arrays len 1025 (bins 0..1024) -> real table via irfft (1/N scaling, like JUCE)."""
    X = re + 1j * im
    return np.fft.irfft(X, n=N)

def sine_phase_spectrum(b):
    """b[n] amplitude of sin(2 pi n i/N), n=1..len(b). JUCE/numpy inverse scales 1/N:
    sin = (e^{j} - e^{-j})/2j -> X[n] = -j*N*b/2."""
    re = np.zeros(N // 2 + 1); im = np.zeros(N // 2 + 1)
    n = np.arange(1, len(b) + 1)
    im[n] = -0.5 * N * np.asarray(b)
    return re, im

def build_levels(re, im):
    re = re.copy(); im = im.copy()
    re[0] = im[0] = 0.0          # DC
    re[N // 2] = im[N // 2] = 0.0  # Nyquist
    lvl0 = spectrum_to_table(np.where(np.arange(N//2+1) <= KMAX[0], re, 0), np.where(np.arange(N//2+1) <= KMAX[0], im, 0))
    p = np.max(np.abs(lvl0))
    re /= p; im /= p
    out = []
    k = np.arange(N // 2 + 1)
    for L in range(LEVELS):
        m = (k >= 1) & (k <= KMAX[L])
        t = spectrum_to_table(np.where(m, re, 0), np.where(m, im, 0)).astype(np.float32)
        out.append(np.append(t, t[0]))  # guard
    return out

# ---------------- banks ----------------
def saw_frame(k):
    return [1.0 / n for n in range(1, k + 1)]

def square_frame(k):
    c = 1 + 15 * (k - 1) / 31
    b = np.zeros(31)
    for j in range(1, 17):
        n = 2 * j - 1
        b[n - 1] = (1.0 / n) * np.clip(c - (j - 1), 0, 1)
    return b

def drive_frame_spectrum(k, osf=1):
    g = 0.25 * 100 ** ((k - 1) / 31)
    M = N * osf
    i = np.arange(M)
    x = np.tanh(g * np.sin(2 * np.pi * i / M)) / np.tanh(g)
    X = np.fft.rfft(x)[: N // 2 + 1] / osf   # forward unnormalised, like JUCE
    return X.real.copy(), X.imag.copy()

def pulse_frame_spectrum(k):
    d = 0.5 * (1 / 16) ** ((k - 1) / 31)
    re = np.zeros(N // 2 + 1); im = np.zeros(N // 2 + 1)
    n = np.arange(1, 1024)
    a = (2 / (n * np.pi)) * np.sin(n * np.pi * d)
    # x = sum a_n cos(2 pi n (phi - 0.5)) = sum a_n (-1)^n cos(2 pi n phi); cos -> X[n] = N*a/2 (real)
    re[n] = 0.5 * N * a * ((-1.0) ** n)
    return re, im

def saw1023_spectrum():
    return sine_phase_spectrum([1.0 / n for n in range(1, 1024)])

# ---------------- oscillator ----------------
def render(levels_by_L, f, fs, nsamp, bandlimit=True):
    x = f * N / fs
    if bandlimit:
        L = int(np.clip(np.ceil(np.log2(x)), 0, LEVELS - 1)) if x > 0 else 0
    else:
        L = 0
    tab = levels_by_L[L]
    inc = f / fs
    phase = (np.arange(nsamp, dtype=np.float64) * inc) % 1.0
    idx = phase * N
    i0 = np.minimum(idx.astype(np.int64), N - 1)
    t = (idx - i0).astype(np.float32)
    a = tab[i0]; b = tab[i0 + 1]
    y = a + t * (b - a)
    return y.astype(np.float32), L

def kaiser_metric(y, f, fs, guard_hz=None):
    M = len(y)
    w = np.kaiser(M, 38.0)
    Y = np.abs(np.fft.rfft(y.astype(np.float64) * w))
    freqs = np.arange(len(Y)) * fs / M
    binhz = fs / M
    g = guard_hz if guard_hz is not None else 14 * binhz  # kaiser 38 mainlobe ~ +/-12 bins
    harm = np.zeros(len(Y), bool)
    kmax = int((fs / 2) // f)
    for kk in range(1, kmax + 1):
        harm |= np.abs(freqs - kk * f) <= g
    harm |= freqs <= g  # DC region
    peak = Y[harm].max()
    inh = Y[~harm].max() if (~harm).any() else 1e-30
    return 20 * np.log10(inh / peak)

def midi_hz(n):
    return 440.0 * 2 ** ((n - 69) / 12)

if __name__ == "__main__":
    t0 = time.time()
    # sanity: Kaiser window floor on a pure sine
    fs = 44100.0; M = 1 << 17
    s = np.sin(2 * np.pi * 1000.3 * np.arange(M) / fs).astype(np.float32)
    print("measurer floor (float32 sine, kaiser38): %.1f dB" % kaiser_metric(s, 1000.3, fs))

    drive32 = build_levels(*drive_frame_spectrum(32))
    drive32_os = build_levels(*drive_frame_spectrum(32, osf=8))
    pulse32 = build_levels(*pulse_frame_spectrum(32))
    pulse1 = build_levels(*pulse_frame_spectrum(1))
    saw1023 = build_levels(*saw1023_spectrum())
    print("build ok %.2fs" % (time.time() - t0))

    tables = {"drive32": drive32, "pulse1(square1023)": pulse1, "pulse32": pulse32, "saw1023": saw1023}
    print("\n== C8 (MIDI 108), bandlimit On")
    for fs in (44100.0, 48000.0, 96000.0):
        for name, tb in tables.items():
            y, L = render(tb, midi_hz(108), fs, 1 << 17)
            print("fs=%6.0f %-20s L=%d Kmax=%4d  max inharmonic = %7.1f dB" % (fs, name, L, KMAX[L], kaiser_metric(y, midi_hz(108), fs)))

    print("\n== sweep worst case (every 3rd note A0..C8 + all A's), bandlimit On")
    notes = sorted(set(list(range(21, 109, 3)) + [21, 22, 23, 24, 33, 45, 57, 69, 81, 93, 105, 108]))
    for fs in (44100.0, 48000.0, 96000.0):
        for name, tb in tables.items():
            worst = (-999, None, None)
            for n in notes:
                y, L = render(tb, midi_hz(n), fs, 1 << 17)
                m = kaiser_metric(y, midi_hz(n), fs)
                if m > worst[0]:
                    worst = (m, n, L)
            print("fs=%6.0f %-20s worst %7.1f dB at MIDI %d (L=%d)" % (fs, name, worst[0], worst[1], worst[2]))

    print("\n== DSP-02 demo: bandlimit Off, Sine->Saw frame 32 at C7, 44.1k")
    saw32 = build_levels(*sine_phase_spectrum(saw_frame(32)))
    y, L = render(saw32, midi_hz(96), 44100.0, 1 << 17, bandlimit=False)
    print("  saw32 C7 Off: %.1f dB   (On: %.1f dB)" % (kaiser_metric(y, midi_hz(96), 44100.0),
          kaiser_metric(render(saw32, midi_hz(96), 44100.0, 1 << 17)[0], midi_hz(96), 44100.0)))
    y, L = render(drive32, midi_hz(96), 44100.0, 1 << 17, bandlimit=False)
    print("  drive32 C7 Off: %.1f dB" % kaiser_metric(y, midi_hz(96), 44100.0))

    print("\n== drive table aliasing from 2048-pt time-domain evaluation (frame 32, level 0)")
    d1 = np.fft.rfft(drive32[0][:N].astype(np.float64)); d8 = np.fft.rfft(drive32_os[0][:N].astype(np.float64))
    err = np.abs(d1 - d8)
    print("  max |bin diff| rel h1: %.1f dB (bin %d)" % (20*np.log10(err.max()/np.abs(d8[1])), err.argmax()))
    print("total %.1fs" % (time.time() - t0))
