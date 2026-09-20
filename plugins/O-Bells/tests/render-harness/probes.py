#!/usr/bin/env python3
"""O-Bells engine probes — the measurements behind RC-1..RC-4 in
improvements/preset-differentiation-v4.6-v4.8.md, on the processor-level harness.

    python3 plugins/O-Bells/tests/render-harness/probes.py

Everything runs from parameter DEFAULTS plus overrides, humanize 0. Gated: the
random-parameter pair median against the brief's v4.5.1 row (11.5 tap / 10.3
held, +-1 dB). The rest is printed for the Step 2/3 gates to read.
"""
import random, sys, tempfile
import numpy as np
from report import MODES, SEED_A, SEED_B, SR, TOL, centroid, dist, feat, matrix, render, self_noise, t40

RANDOM_BASELINE = {'tap': 11.5, 'held': 10.3}
MATERIALS = ['Bronze', 'Brass', 'Steel', 'Aluminum', 'Cast Iron']


def fields(**params):
    return ['humanize=0'] + [f'{k}={v}' for k, v in params.items()]


def lvl(x, a, b):
    m = x.mean(axis=1)[int(a * SR):int(b * SR)]
    return 20 * np.log10(np.sqrt((m ** 2).mean()) + 1e-9)


def main():
    failures = []
    with tempfile.TemporaryDirectory() as out:
        # RC-1 — material, as the processor passes it (choice index).
        jobs = [(n, fields(material=i)) for i, n in enumerate(MATERIALS)]
        S = render(jobs, out, 'mat', 6, 6, SEED_A); F = [feat(x) for x in S]
        S2 = render(jobs, out, 'mat_b', 6, 6, SEED_B)
        print('MATERIAL (held 6 s): dist to Cast Iron / to Bronze / level 2.5-3 s / centroid .25-.7 s')
        for i, n in enumerate(MATERIALS):
            print(f'  {n:10s} d(CastIron)={dist(F[i], F[4]):4.1f}  d(Bronze)={dist(F[i], F[0]):4.1f}  {lvl(S[i], 2.5, 3):6.1f} dB  {centroid(S[i], .25, .7):5.0f} Hz')
        print(f'  self-noise {self_noise(S, S2):.1f} dB')

        # RC-2 — sub layer decay with and without bloom.
        jobs = [('sub, bloom 0', fields(octaveBlendSub=.6, bloomAmount=0)),
                ('sub, bloom .05', fields(octaveBlendSub=.6, bloomAmount=.05)),
                ('no sub, bloom .05', fields(bloomAmount=.05))]
        print('\nOCTAVE LAYER DECAY (held 12 s): level dB in 1-s windows at 0, 2, 4, 6, 8, 10 s')
        for (n, _), x in zip(jobs, render(jobs, out, 'oct', 12, 12, SEED_A)):
            print(f'  {n:18s}', ' '.join(f'{lvl(x, t, t + 1):6.1f}' for t in range(0, 12, 2)))

        # RC-3 — damping law.
        jobs = [(f'damping {d}', fields(damping=d, bodyTime=4000, humSustain=90)) for d in (0, .5, 1)]
        print('\nDAMPING -> ring after a 0.25 s tap (T40, s)')
        for (n, _), x in zip(jobs, render(jobs, out, 'damp', .25, 12, SEED_A)):
            print(f'  {n:12s} {t40(x):5.2f}')

        # RC-4 — reachable range: 60 random points across the parameter space.
        random.seed(1); jobs = []
        for i in range(60):
            d = {k: random.uniform(lo, hi) for k, (lo, hi) in dict(
                inharmonicity=(0, 1), damping=(0, 1), overtoneBrightness=(0, 1), acousticBrightness=(0, 1),
                strikePosition=(0, 1), malletHardness=(0, 1), bloomAmount=(0, 1), bloomSpeed=(0, 1), shimmer=(0, 1),
                octaveBlendSub=(0, 1), octaveBlendOct=(0, 1), partialTuning=(-100, 100), nonlinearEffects=(0, 1),
                attackLevel=(0, 1), strikeTime=(5, 100), brilliance=(0, 100), bodyTime=(100, 5000),
                humSustain=(0, 100), airAbsorption=(0, 1), pitchEnvelope=(0, 1)).items()}
            d['material'] = random.choice([0, 4]); d['strikeNoiseChar'] = random.randint(0, 2)
            d['unisonCount'] = random.randint(1, 4)
            jobs.append((f'r{i}', fields(**d)))
        print('\nRANGE: 60 random parameter points, pairwise')
        for tag, hold, total in MODES:
            D = matrix(render(jobs, out, 'rand_' + tag, hold, total, SEED_A)); iu = np.triu_indices(len(jobs), 1)
            med = float(np.median(D[iu])); ref = RANDOM_BASELINE[tag]; bad = abs(med - ref) > TOL
            print(f'  {tag:5s} median {med:.1f} (v4.5.1 {ref:.1f}, {med - ref:+.1f})  p90 {np.percentile(D[iu], 90):.1f}  max {D[iu].max():.1f}'
                  + ('   <-- outside %.1f dB' % TOL if bad else ''))
            if bad:
                failures.append(f'random {tag} median {med:.1f} vs baseline {ref:.1f}')

    print('\n' + ('RANGE GATE: PASS' if not failures else 'RANGE GATE: FAIL\n  ' + '\n  '.join(failures)))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
