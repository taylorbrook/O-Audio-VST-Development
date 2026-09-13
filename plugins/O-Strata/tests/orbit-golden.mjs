#!/usr/bin/env node
// orbit-golden.mjs — the Stage 3 orbit golden (plan Decision 30).
//
// Asserts that Source/ui/public/js/terrain-view.js (the verbatim Orbits.h port the
// view draws its orbit ribbon with) reproduces the harness's dump within 1e-4:
//
//   O-Strata-render-test --gate orbits --out /tmp/orbits.json
//   node plugins/O-Strata/tests/orbit-golden.mjs /tmp/orbits.json
//
// The JSON is {thetaOffset: 0.5, n: 512, m: [...], affine: {aspect, rotDeg, size, cx, cy},
// orbits: [{kind, name, m, base: [[x, y] × 512], affine: [[x, y] × 512]} × 44]} at
// θ_i = 2π (i + thetaOffset) / n — HALF-STEP, so the Superellipse LUT nodes are never
// hit (RESEARCH C16). `base` is compared against baseOrbit (kind, θ, m), `affine` against
// orbitPoint (p, θ) with p in the page's snapshot() units. Prints a 44-row table; exit 2
// on any miss (the dump is never committed — the binary is the oracle).
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { baseOrbit, orbitPoint } from '../Source/ui/public/js/terrain-view.js';

const TOL = 1e-4;
const file = process.argv[2];
if (!file) { console.error('usage: node orbit-golden.mjs <orbits.json>   (from O-Strata-render-test --gate orbits --out PATH)'); process.exit(2); }
const G = JSON.parse(fs.readFileSync(path.resolve(file), 'utf8'));
const n = G.n, off = G.thetaOffset, A = G.affine;
const p0 = { size: A.size, aspect: A.aspect, rot: A.rotDeg * Math.PI / 180, cx: A.cx, cy: A.cy };

let worst = 0, fails = 0;
console.log(`orbit-golden — ${G.orbits.length} curves × ${n} θ (offset ${off}) from ${path.relative(process.cwd(), path.resolve(file))}; tol ${TOL}`);
console.log('  kind name           m     max|Δ| base   max|Δ| affine  verdict');
for (const o of G.orbits) {
    let dBase = 0, dAff = 0;
    const p = Object.assign({ orbit: o.kind, m: o.m }, p0);
    for (let i = 0; i < n; i++) {
        const th = 2 * Math.PI * (i + off) / n;
        const b = baseOrbit(o.kind, th, o.m), a = orbitPoint(p, th);
        dBase = Math.max(dBase, Math.abs(b[0] - o.base[i][0]), Math.abs(b[1] - o.base[i][1]));
        dAff = Math.max(dAff, Math.abs(a[0] - o.affine[i][0]), Math.abs(a[1] - o.affine[i][1]));
    }
    const ok = dBase <= TOL && dAff <= TOL;
    if (!ok) fails++;
    worst = Math.max(worst, dBase, dAff);
    console.log(`  ${String(o.kind).padStart(4)} ${o.name.padEnd(14)} ${o.m.toFixed(1)}   ${dBase.toExponential(2).padStart(10)}   ${dAff.toExponential(2).padStart(10)}     ${ok ? 'PASS' : 'FAIL'}`);
}
console.log(`\n${fails === 0 ? 'ALL PASS' : 'FAILED'} — ${G.orbits.length - fails} / ${G.orbits.length} curves within ${TOL} (worst ${worst.toExponential(2)})`);
process.exit(fails === 0 ? 0 : 2);
