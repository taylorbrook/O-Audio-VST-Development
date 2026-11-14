#!/usr/bin/env python3
"""O-GranulatorInstrument: measurements on the factory .flac files (1.2.0).

  factory_measure.py [DIR]     default Resources/factory

Prints, per source in the manifest: bytes, length, peak, RMS, the loud-frame
level (90th-percentile 400 ms frame of the channel mean: the FLV1 measure),
and for the pitched ones the measured fundamental against the manifest root.
Then a likeness table for the struck / plucked synthetics: spectral centroid
and the strongest partials (as ratios to the root) over the first 250 ms,
where these sources differ, and the time to -30 dB.

An ensemble (choir, saw_stack, the chords) reads its fundamental as the power
centroid of the cluster; `breath` too (noise through a resonance: the
estimate carries a few cents of spread whatever the seed).

Needs numpy and soundfile. Reads only; writes nothing.
"""
import json, math, os, sys
import numpy as np
import soundfile as sf

HERE = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SR = 48000
# Pitched synthetics: the partial the estimate reads (ratio to the root's frequency).
PITCHED = {"harmonic": 1, "pluck": 1, "bell": 1, "vowel": 1, "fm_bell": 1, "glass": 1, "bowed": 1, "reed": 1, "organ": 1,
           "choir": 1, "sub": 1, "saw_stack": 1, "chord_minor": 1, "chord_sus": 1, "kalimba": 1, "marimba_bar": 1,
           "metal_plate": 1, "shimmer": 8, "breath": 1}
ENSEMBLE = ["choir", "saw_stack", "chord_minor", "chord_sus", "breath"]
LIKENESS = ["bell", "fm_bell", "glass", "metal_plate", "pluck", "kalimba", "marimba_bar"]


def frames_db(m, hop):
    n = len(m) // hop
    return 10 * np.log10(np.mean(m[:n * hop].reshape(n, hop) ** 2, axis=1) + 1e-12)


def spectrum(m):
    n = len(m)
    N = 1 << (int(math.ceil(math.log2(n))) + 2)
    return np.abs(np.fft.rfft(m * np.hanning(n), N)), N


def peak_near(X, N, hz, width=0.03):
    a, b = int(hz * (1 - width) * N / SR), int(hz * (1 + width) * N / SR) + 2
    j = a + int(np.argmax(X[a:b]))
    y0, y1, y2 = np.log(X[j - 1:j + 2] + 1e-30)
    return (j + 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2)) * SR / N


def centroid_near(X, N, hz, width=0.03):
    a, b = int(hz * (1 - width) * N / SR), int(hz * (1 + width) * N / SR) + 2
    p = X[a:b] ** 2
    return float((np.arange(a, b) * p).sum() / p.sum()) * SR / N


def top_partials(X, N, count=3):
    Y = X.copy()
    Y[:int(40 * N / SR)] = 0
    out = []
    for _ in range(count):
        j = int(np.argmax(Y))
        out.append((j * SR / N, 20 * math.log10(X[j] / X.max())))
        w = int(25 * N / SR)
        Y[max(0, j - w):j + w] = 0
    return out


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "Resources/factory")
    with open(os.path.join(HERE, "Resources/factory/manifest.json")) as f:
        m = json.load(f)
    total = 0
    like = {}
    print("%-17s %-9s ch    len     bytes   peak    rms   loud | root          measured" % ("id", "kind"))
    for s in m["sources"]:
        p = os.path.join(d, s["id"] + ".flac")
        if not os.path.exists(p):
            print("%-17s MISSING" % s["id"])
            continue
        x, sr = sf.read(p, dtype="float64", always_2d=True)
        assert sr == SR
        size = os.path.getsize(p)
        total += size
        mono = x.mean(axis=1)
        loud = float(np.percentile(frames_db(mono, int(0.4 * SR)), 90))
        line = "%-17s %-9s %d %7.2f %9d %6.2f %6.2f %6.1f |" % (
            s["id"], s["kind"], x.shape[1], len(x) / SR, size, 20 * math.log10(np.abs(x).max()),
            20 * math.log10(math.sqrt((x ** 2).mean())), loud)
        X = None
        if s["id"] in PITCHED:
            X, N = spectrum(mono)
            ref = 440.0 * 2 ** ((s["root_note"] + s["root_cents"] / 100.0 - 69) / 12) * PITCHED[s["id"]]
            f = centroid_near(X, N, ref) if s["id"] in ENSEMBLE else peak_near(X, N, ref)
            line += " %3d %+5.1f ct   %9.3f Hz  %+6.2f ct%s" % (s["root_note"], s["root_cents"], f, 1200 * math.log2(f / ref),
                                                           "  (partial %d)" % PITCHED[s["id"]] if PITCHED[s["id"]] != 1 else "")
        print(line)
        if s["id"] in LIKENESS:
            A, NA = spectrum(mono[:int(0.25 * SR)])
            f0 = 440.0 * 2 ** ((s["root_note"] - 69) / 12)
            fr = frames_db(mono, 480)
            below = np.where(fr < fr.max() - 30)[0]
            below = below[below > int(np.argmax(fr))]
            like[s["id"]] = (float((np.arange(len(A)) * A ** 2).sum() / (A ** 2).sum()) * SR / NA,
                             (below[0] - int(np.argmax(fr))) * 0.01 if len(below) else float("nan"),
                             [(f / f0, db) for f, db in top_partials(A, NA, 4)])
    print("total %d B" % total)
    print("\nlikeness (first 250 ms): centroid   -30 dB at   strongest four partials (ratio to the root, dB re the strongest)")
    for k in LIKENESS:
        if k in like:
            c, t, tp = like[k]
            print("  %-20s %7.0f Hz   %5.2f s    %s" % (k, c, t, "   ".join("%5.2f (%+.0f)" % q for q in tp)))


if __name__ == "__main__":
    main()
