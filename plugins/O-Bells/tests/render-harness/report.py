#!/usr/bin/env python3
"""O-Bells preset-differentiation report.

Drives O-Bells-render-test (processor-level, voice isolated — see main.cpp) over
every factory preset, tap (0.25 s hold) and held (6 s), C4, vel 0.8, 48 kHz, and
prints pairwise distance / T40 / centroid per preset.

Metric: RMS dB difference of level-normalised log-band energies (24 bands,
60 Hz-16 kHz) over 7 log-spaced time frames (0-6 s), floor -60 dB.

    cmake -B build -G Ninja -DOUARICON_BUILD_TESTS=ON && ninja -C build O-Bells-render-test
    python3 plugins/O-Bells/tests/render-harness/report.py            # report + baseline gate
    python3 plugins/O-Bells/tests/render-harness/report.py --no-gate  # report only

Needs numpy. Exit 1 when a gated quantity is more than TOL dB off BASELINE.
"""
import argparse, os, subprocess, sys, tempfile
import numpy as np

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
BIN = os.path.join(REPO, 'build/plugins/O-Bells/O-Bells-render-test_artefacts/Release/O-Bells-render-test')
SR = 48000
EDGES = [0, .02, .08, .25, .7, 1.5, 3.0, 6.0]
BANDS = np.geomspace(60, 16000, 25)
MODES = [('tap', 0.25, 6.0), ('held', 6.0, 6.0)]
SEED_A, SEED_B = 1, 2

# Re-anchor ONLY in a step whose CHANGELOG says the sound changed.
#
# v4.6.0 (Step 2) re-anchored: material mapping, octave-layer decay under bloom and
# the unison spread all changed the sound on purpose. Seeds are fixed, so these are
# exact for SEED_A / SEED_B. The v4.5.1 rows they replace (brief, "Measured baseline"):
#   tap  3.4 / 13.0 / 7.4 / 3.7     held 3.2 / 14.1 / 8.2 / 3.2
# and the note below is about that v4.5.1 'held pair min' cell.
#
# One cell is NOT the brief's number: held 'pair min' was 4.1 there. That was a
# single clock-seeded draw of an extreme statistic — the scratch harness that
# produced it re-runs at 2.7 / 2.8 / 3.5, and this harness gives 2.7..3.5 (mean
# 3.2) over 8 seeds, never 4.1. 3.2 is that 8-seed mean. Every other cell is the
# brief's and sits inside its own 8-seed range (median sd 0.2, p10 sd 0.2,
# min sd 0.3).
BASELINE_VERSION = 'v4.6.0'
BASELINE = {
    'tap':  {'self-noise': 2.5, 'pair median': 13.3, 'pair p10': 8.0, 'pair min': 3.5},
    'held': {'self-noise': 2.4, 'pair median': 14.7, 'pair p10': 9.3, 'pair min': 3.6},
}
TOL = 1.0


def presets():
    out = subprocess.run([BIN, '--list'], check=True, capture_output=True, text=True).stdout
    return [tuple(l.split('|', 1)) for l in out.splitlines() if '|' in l]


def render(jobs, outdir, tag, hold, total, seed, note=60, vel=0.8):
    """jobs: list of (label, [fields]) — fields are 'preset=Cat/Name' or 'paramId=value'."""
    d = os.path.join(outdir, tag)
    os.makedirs(d, exist_ok=True)
    jobfile = os.path.join(outdir, tag + '.txt')
    with open(jobfile, 'w') as f:
        for label, fields in jobs:
            f.write('|'.join([label] + list(fields)) + '\n')
    subprocess.run([BIN, '--render', jobfile, d, f'--note={note}', f'--vel={vel}', f'--hold={hold}',
                    f'--total={total}', f'--seed={seed}'], check=True, stdout=subprocess.DEVNULL)
    return [np.fromfile(os.path.join(d, f'{i}.f32'), dtype=np.float32).reshape(-1, 2).astype(np.float64)
            for i in range(len(jobs))]


def feat(x):
    m = x.mean(axis=1); F = []
    for a, b in zip(EDGES[:-1], EDGES[1:]):
        seg = m[int(a * SR):int(b * SR)]; w = np.hanning(len(seg))
        P = np.abs(np.fft.rfft(seg * w)) ** 2 / (w ** 2).sum()
        fr = np.fft.rfftfreq(len(seg), 1 / SR)
        F.append([P[(fr >= lo) & (fr < hi)].sum() for lo, hi in zip(BANDS[:-1], BANDS[1:])])
    return 10 * np.log10(np.array(F) + 1e-12)


def dist(Fa, Fb, floor=-60):
    # level-normalise each to its own peak band, floor at -60 dB rel
    A = np.maximum(Fa - Fa.max(), floor); B = np.maximum(Fb - Fb.max(), floor)
    mask = (A > floor) | (B > floor)
    return np.sqrt(((A - B)[mask] ** 2).mean())


def matrix(sigs):
    F = [feat(x) for x in sigs]; n = len(F); D = np.zeros((n, n))
    for i in range(n):
        for j in range(n): D[i, j] = dist(F[i], F[j])
    return D


def self_noise(A, B):
    return float(np.mean([dist(feat(a), feat(b)) for a, b in zip(A, B)]))


def t40(x, db=-40):
    m = np.abs(x).max(axis=1); hop = 480
    e = np.array([m[i:i + hop].max() for i in range(0, len(m) - hop, hop)])
    e = 20 * np.log10(e / e.max() + 1e-9)
    idx = np.where(e > db)[0]
    return idx[-1] * hop / SR if len(idx) else 0


def centroid(x, a, b):
    seg = x.mean(axis=1)[int(a * SR):int(b * SR)]
    P = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
    fr = np.fft.rfftfreq(len(seg), 1 / SR)
    return (P * fr).sum() / P.sum()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--out', help='keep renders + D_<mode>.npy here (default: a temp dir, deleted)')
    ap.add_argument('--no-gate', action='store_true', help='print the report, skip the baseline gate')
    args = ap.parse_args()

    if not os.path.exists(BIN):
        sys.exit(f'{BIN} not built — configure with -DOUARICON_BUILD_TESTS=ON and `ninja O-Bells-render-test`')

    tmp = None if args.out else tempfile.TemporaryDirectory()
    out = args.out or tmp.name
    os.makedirs(out, exist_ok=True)

    P = presets()
    names = [n for _, n in P]
    jobs = [(n, [f'preset={c}/{n}']) for c, n in P]
    print(f'{len(P)} factory presets')
    failures = []

    for tag, hold, total in MODES:
        S = render(jobs, out, tag, hold, total, SEED_A)
        S2 = render(jobs, out, tag + '_b', hold, total, SEED_B)
        again = render(jobs[:1], out, tag + '_det', hold, total, SEED_A)
        if not np.array_equal(again[0], S[0]):
            failures.append(f'{tag}: same seed, different render — the seed hook no longer covers every RNG')

        D = matrix(S); iu = np.triu_indices(len(P), 1)
        got = {'self-noise': self_noise(S, S2), 'pair median': float(np.median(D[iu])),
               'pair p10': float(np.percentile(D[iu], 10)), 'pair min': float(D[iu].min())}
        print(f'\n== {tag} (hold {hold}s) ==  pair max {D[iu].max():.1f} dB')
        print(f'  {"quantity":12s} {"now":>6s} {BASELINE_VERSION:>7s} {"delta":>6s}')
        for k, v in got.items():
            ref = BASELINE[tag][k]; bad = abs(v - ref) > TOL
            print(f'  {k:12s} {v:6.1f} {ref:7.1f} {v - ref:+6.1f}{"   <-- outside " + str(TOL) + " dB" if bad else ""}')
            if bad:
                failures.append(f'{tag} {k}: {v:.1f} vs baseline {ref:.1f}')

        print(f'\n  {"preset":28s} {"T40(s)":>6s} {"cen0-80ms":>9s} {"cen.25-.7":>9s}  nearest (dist)')
        for i, n in enumerate(names):
            d = D[i].copy(); d[i] = 1e9; j = d.argmin()
            print(f'  {n:28s} {t40(S[i]):6.2f} {centroid(S[i], 0, .08):9.0f} {centroid(S[i], .25, .7):9.0f}  {names[j]} ({d[j]:.1f})')
        if args.out:
            np.save(os.path.join(out, f'D_{tag}.npy'), D)

    if args.no_gate:
        return 0
    print('\n' + ('BASELINE GATE: PASS (all within %.1f dB)' % TOL if not failures else 'BASELINE GATE: FAIL\n  ' + '\n  '.join(failures)))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
