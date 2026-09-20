#!/usr/bin/env python3
"""O-Bells engine probes — the measurements behind RC-1..RC-4 in
improvements/preset-differentiation-v4.6-v4.8.md, on the processor-level harness.

    python3 plugins/O-Bells/tests/render-harness/probes.py

Everything runs from parameter DEFAULTS plus overrides, humanize 0. Gated:
  - the random-parameter pair median against RANDOM_BASELINE (+-1 dB);
  - Step 2 (v4.6.0), RC-1: the five materials pairwise >= MATERIAL_MIN dB, both seeds;
  - Step 2 (v4.6.0), RC-2: the held sub layer's own band falls between 2 s and 10 s
    exactly as it does with Bloom off (+-SUB_TRACK dB), at every bloom amount, and
    by >= SUB_FALL_MIN dB at damping 1.
  - Step 2 (v4.6.0), unison: the prime of every unison voice sits on its designed
    detune (+-UNISON_TOL cents) — v4.5.2 had no negative side.
  - identity: configs Step 2 must not touch render BIT-IDENTICAL to v4.5.2.
The rest is printed for the Step 3 gates to read.
"""
import hashlib, random, sys, tempfile
import numpy as np
from report import MODES, SEED_A, SEED_B, SR, TOL, centroid, dist, feat, matrix, render, self_noise, t40

RANDOM_BASELINE = {'tap': 11.5, 'held': 10.3}
MATERIALS = ['Bronze', 'Brass', 'Steel', 'Aluminum', 'Cast Iron']
MATERIAL_MIN = 5.0
# The sub layer's hum partial (C4: 65 Hz) — no other layer puts energy here (-95 dB
# without it), so the band reads the layer, not the mix. The fall is a ratio of one
# partial's amplitude, so it is seed-independent.
SUB_BAND = (50, 90)
SUB_TRACK = 0.5
# 12 dB is what the hum-stage law gives this partial in 8 s at damping 1 (12.3).
# At the default damping 0.7 it gives 9.1, bloom or no bloom — so the absolute
# figure is gated at damping 1 and the default is gated against the bloom-0 row.
# v4.5.2 reads 0.0 on every bloom > 0 row.
SUB_FALL_MIN = 12.0
UNISON_TOL = 3.0
# sha256[:16] of the float32 render (hold 2 s, total 4 s, seed 7), recorded from a
# harness built on the v4.5.2 tree — NOT from the build under test. Unison 1, no
# octave layer under bloom: nothing Step 2 fixed is reachable. A later step that
# changes one of these on purpose narrows the config and re-records from the OLD
# tree; it never re-records from its own build. Same machine / toolchain only.
IDENTITY = [
    ('defaults', ['humanize=0'], 'b03f120bb0bf07b3'),
    ('bronze, bloom .5', ['humanize=0', 'material=0', 'bloomAmount=0.5', 'unisonCount=1', 'octaveBlendSub=0', 'octaveBlendOct=0'], '8623ba1720d678bd'),
    ('cast iron', ['humanize=0', 'material=4', 'unisonCount=1', 'octaveBlendSub=0', 'octaveBlendOct=0'], '20ca3e39fbfa7fa7'),
    ('humanize .7, damping .8', ['humanize=0.7', 'damping=0.8', 'unisonCount=1', 'octaveBlendSub=0', 'octaveBlendOct=0'], '2025c4965dbfac2b'),
    ('sub .6, bloom 0', ['humanize=0', 'octaveBlendSub=0.6', 'bloomAmount=0', 'unisonCount=1'], '47c8d37b466bae43'),
]
C4 = 261.6256


def fields(**params):
    return ['humanize=0'] + [f'{k}={v}' for k, v in params.items()]


def lvl(x, a, b):
    m = x.mean(axis=1)[int(a * SR):int(b * SR)]
    return 20 * np.log10(np.sqrt((m ** 2).mean()) + 1e-9)


def band(x, a, b, lo, hi):
    seg = x.mean(axis=1)[int(a * SR):int(b * SR)]; w = np.hanning(len(seg))
    P = np.abs(np.fft.rfft(seg * w)) ** 2 / (w ** 2).sum(); fr = np.fft.rfftfreq(len(seg), 1 / SR)
    return 10 * np.log10(P[(fr >= lo) & (fr < hi)].sum() + 1e-18)


def main():
    failures = []
    with tempfile.TemporaryDirectory() as out:
        print('IDENTITY vs v4.5.2 (bit-exact)')
        for (n, _, ref), x in zip(IDENTITY, render([(n, f) for n, f, _ in IDENTITY], out, 'id', 2, 4, 7)):
            got = hashlib.sha256(x.astype(np.float32).tobytes()).hexdigest()[:16]
            print(f'  {n:24s} {got}' + ('' if got == ref else f'   <-- FAIL, v4.5.2 {ref}'))
            if got != ref:
                failures.append(f'identity "{n}": {got}, v4.5.2 {ref}')
        print()

        # RC-1 — material, as the processor passes it (choice index).
        jobs = [(n, fields(material=i)) for i, n in enumerate(MATERIALS)]
        S = render(jobs, out, 'mat', 6, 6, SEED_A); F = [feat(x) for x in S]
        S2 = render(jobs, out, 'mat_b', 6, 6, SEED_B)
        print('MATERIAL (held 6 s): dist to Cast Iron / to Bronze / level 2.5-3 s / centroid .25-.7 s')
        for i, n in enumerate(MATERIALS):
            print(f'  {n:10s} d(CastIron)={dist(F[i], F[4]):4.1f}  d(Bronze)={dist(F[i], F[0]):4.1f}  {lvl(S[i], 2.5, 3):6.1f} dB  {centroid(S[i], .25, .7):5.0f} Hz')
        print(f'  self-noise {self_noise(S, S2):.1f} dB')
        iu = np.triu_indices(len(MATERIALS), 1)
        for seed, sigs in ((SEED_A, S), (SEED_B, S2)):
            D = matrix(sigs); i, j = [k[D[iu].argmin()] for k in iu]; bad = D[iu].min() < MATERIAL_MIN
            print(f'  seed {seed}: closest pair {MATERIALS[i]} <-> {MATERIALS[j]} {D[iu].min():.1f} dB (gate >= {MATERIAL_MIN:.1f})'
                  + ('   <-- FAIL' if bad else ''))
            if bad:
                failures.append(f'material {MATERIALS[i]} <-> {MATERIALS[j]} {D[iu].min():.1f} dB (seed {seed})')

        # RC-2 — sub layer decay with and without bloom.
        jobs = [('sub, bloom 0', fields(octaveBlendSub=.6, bloomAmount=0)),
                ('sub, bloom .05', fields(octaveBlendSub=.6, bloomAmount=.05)),
                ('no sub, bloom .05', fields(bloomAmount=.05))]
        print('\nOCTAVE LAYER DECAY (held 12 s): level dB in 1-s windows at 0, 2, 4, 6, 8, 10 s')
        for (n, _), x in zip(jobs, render(jobs, out, 'oct', 12, 12, SEED_A)):
            print(f'  {n:18s}', ' '.join(f'{lvl(x, t, t + 1):6.1f}' for t in range(0, 12, 2)))

        print(f'\nSUB LAYER ({SUB_BAND[0]}-{SUB_BAND[1]} Hz, held 12 s): fall between 2 s and 10 s, dB')
        for damping in (0.7, 1.0):
            jobs = [(f'bloom {b}', fields(octaveBlendSub=.6, bloomAmount=b, damping=damping)) for b in (0, .05, .5, 1)]
            falls = [band(x, 2, 3, *SUB_BAND) - band(x, 10, 11, *SUB_BAND) for x in render(jobs, out, f'subfall_{damping}', 12, 12, SEED_A)]
            print(f'  damping {damping}: ' + '  '.join(f'{n} {f:5.1f}' for (n, _), f in zip(jobs, falls)))
            for (n, _), f in zip(jobs[1:], falls[1:]):
                if abs(f - falls[0]) > SUB_TRACK:
                    failures.append(f'sub layer, damping {damping}, {n}: falls {f:.1f} dB, bloom 0 falls {falls[0]:.1f}')
                if damping == 1.0 and f < SUB_FALL_MIN:
                    failures.append(f'sub layer, damping 1.0, {n}: falls {f:.1f} dB < {SUB_FALL_MIN:.1f}')

        # Unison spread — prime partial peaks, 50 cents, harmonic table, in cents from C4.
        want = {1: [0], 2: [-25, 25], 3: [-50, 0, 50], 4: [-37.5, -12.5, 12.5, 37.5]}
        jobs = [(f'unison {u}', fields(unisonCount=u, unisonDetune=50, inharmonicity=0, stereoSpread=0)) for u in want]
        print('\nUNISON (detune 50 c): prime peaks, cents from C4')
        for (n, _), x, u in zip(jobs, render(jobs, out, 'unison', 6, 6, SEED_A), want):
            seg = x.mean(axis=1)[SR:5 * SR]; P = np.abs(np.fft.rfft(seg * np.hanning(len(seg))))
            fr = np.fft.rfftfreq(len(seg), 1 / SR); m = (fr > 235) & (fr < 290); Pm, fm = P[m], fr[m]
            got = [1200 * np.log2(fm[i] / C4) for i in range(1, len(Pm) - 1)
                   if Pm[i] > Pm[i - 1] and Pm[i] > Pm[i + 1] and Pm[i] > 0.1 * Pm.max()]
            bad = len(got) != len(want[u]) or any(abs(g - w) > UNISON_TOL for g, w in zip(got, want[u]))
            print(f'  {n}: ' + ' '.join(f'{g:+6.1f}' for g in got) + ('   <-- FAIL, want ' + str(want[u]) if bad else ''))
            if bad:
                failures.append(f'{n}: prime peaks {[round(g, 1) for g in got]}, want {want[u]}')

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

    print('\n' + ('PROBE GATES: PASS' if not failures else 'PROBE GATES: FAIL\n  ' + '\n  '.join(failures)))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
