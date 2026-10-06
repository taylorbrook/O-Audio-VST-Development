import numpy as np
from wt_model import *

def render2(tb, f, fs, nsamp, floorL=0, interp="linear"):
    x = f * N / fs
    L = int(np.clip(np.ceil(np.log2(x)), floorL, LEVELS - 1))
    tab = tb[L].astype(np.float64)
    tab3 = np.concatenate([[tab[N-1]], tab[:N], tab[:3]])  # index offset 1: tab3[i+1]=tab[i]
    inc = f / fs
    phase = (np.arange(nsamp, dtype=np.float64) * inc) % 1.0
    idx = phase * N
    i0 = np.minimum(idx.astype(np.int64), N - 1)
    t = idx - i0
    if interp == "linear":
        y = tab3[i0+1] + t * (tab3[i0+2] - tab3[i0+1])
    else:  # 4-pt Hermite
        xm1, x0, x1, x2 = tab3[i0], tab3[i0+1], tab3[i0+2], tab3[i0+3]
        c1 = 0.5*(x1-xm1); c2 = xm1 - 2.5*x0 + 2*x1 - 0.5*x2; c3 = 0.5*(x2-xm1) + 1.5*(x0-x1)
        y = ((c3*t + c2)*t + c1)*t + x0
    return y.astype(np.float32), L

def metric_both(y, f, fs):
    M = len(y); w = np.kaiser(M, 38.0)
    Y = np.abs(np.fft.rfft(y.astype(np.float64) * w))
    freqs = np.arange(len(Y)) * fs / M; g = 14 * fs / M
    harm = freqs <= g
    for kk in range(1, int((fs/2)//f) + 1): harm |= np.abs(freqs - kk*f) <= g
    inh = Y[~harm].max()
    rel_peak = 20*np.log10(inh / Y[harm].max())
    # relative to a full-scale sine of the same RMS as the signal (window gain cancels)
    rms = np.sqrt(np.mean(y.astype(np.float64)**2)); sine_peak_bin = rms*np.sqrt(2) * w.sum()/2
    rel_rms = 20*np.log10(inh / sine_peak_bin)
    return rel_peak, rel_rms

tables = {"saw1023": build_levels(*saw1023_spectrum()), "square1023": build_levels(*pulse_frame_spectrum(1)),
          "pulse16": build_levels(*pulse_frame_spectrum(16)), "pulse24": build_levels(*pulse_frame_spectrum(24)),
          "pulse32": build_levels(*pulse_frame_spectrum(32)), "drive32": build_levels(*drive_frame_spectrum(32)),
          "formant32": None}
notes = sorted(set(list(range(21, 109, 3)) + [21,22,23,24,25,26,27,28,29,30,31,32,33,45,57,69,81,93,105,108]))
for (floorL, interp) in [(0,"linear"), (1,"linear"), (1,"hermite")]:
    print(f"\n== floorL={floorL} interp={interp}: worst over A0..C8 [rel peak harmonic | rel same-RMS sine]")
    for fs in (44100.0, 48000.0, 96000.0):
        for name, tb in tables.items():
            if tb is None: continue
            worst = (-999, 0, 0, 0)
            for n in notes:
                y, L = render2(tb, midi_hz(n), fs, 1 << 17, floorL, interp)
                a, b = metric_both(y, midi_hz(n), fs)
                if a > worst[0]: worst = (a, b, n, L)
            yc, Lc = render2(tb, midi_hz(108), fs, 1 << 17, floorL, interp)
            c8 = metric_both(yc, midi_hz(108), fs)[0]
            print(f"fs={fs:6.0f} {name:11s} worst {worst[0]:7.1f} | {worst[1]:7.1f} dB @MIDI {worst[2]} L={worst[3]}   C8 {c8:7.1f}")
