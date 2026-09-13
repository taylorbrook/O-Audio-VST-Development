/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
// ═══════════════════════════════════════════════════════════════════
// terrain-view.js — O-Strata's 3D terrain view (Stage 3 Round B, plan
// Decisions 28, 29, 33, 36, 37).
//
//   • Orbits.h ported VERBATIM (rawOrbit / maxRadius / baseOrbit + the 33 × 129
//     Superellipse LUT with the same quadrant folding; Squarcle through the same
//     Padé tanh as juce::dsp::FastMathApproximations) and the oscillator's affine
//     in TerrainOscillator::scan's order. `tests/orbit-golden.mjs` holds this
//     within 1e-4 of the harness's `--gate orbits` dump.
//   • The camera (solved per aspect), the heightmap sampler (64 × 64 corner grid,
//     row 0 = y −1, pushed from C++ as `terrainHeightmap`), the two renderers —
//     Canvas 2D (mini canvases, and the big view's fallback) and WebGL2 (R32F
//     heightmap texture, screen-space ribbons, no GLSL `flat`) — behind one
//     interface, and the PERF-03 frame ring.
//
// DOM-free at import time: nothing above a function body touches `document` or
// `window`; makeScene* receive the canvas. No prose is written to the DOM here
// (check-i18n [12]) — the HUD string lives in index.html. Embedded in the one
// juce_add_binary_data target and served by getResource as /js/terrain-view.js.
// ═══════════════════════════════════════════════════════════════════

export const TAU = Math.PI * 2;
export const HM_N = 64;          // heightmap grid (TerrainViewFeed::kHeightmapN)
export const HSCALE = 0.45;      // world height per unit terrain value
const clamp = (v, lo, hi) => v < lo ? lo : v > hi ? hi : v;

// ═══════════════════════════════════════════════════════════════════
// Orbits.h — verbatim port. Kind order = the osc?Orbit choice list:
//   0 Ellipse, 1 Superellipse, 2 Limaçon, 3/4/5 Epitrochoid 3/5/7,
//   6/7/8 Hypocycloid 3/5/7, 9 Butterfly, 10 Squarcle.
// ═══════════════════════════════════════════════════════════════════

// Superellipse LUT: r(θ, n) = (|cos θ|ⁿ + |sin θ|ⁿ)^(−1/n), n ∈ [0.5, 6], θ over one
// quadrant [0, π/2] (4-fold reflection symmetry). 33 × 129, built once at module load
// with Math.pow (< 1 ms) exactly as SuperellipseLUT::build, stored as float32.
const LUT_N = 33, LUT_T = 129, LUT_NMIN = 0.5, LUT_NMAX = 6.0;
const QUADRANT = 1.5707963267948966;
const SUPER_LUT = (() => {
    const t = new Float32Array(LUT_N * LUT_T);
    for (let ni = 0; ni < LUT_N; ni++) {
        const n = LUT_NMIN + (LUT_NMAX - LUT_NMIN) * ni / (LUT_N - 1);
        for (let ti = 0; ti < LUT_T; ti++) {
            const theta = QUADRANT * ti / (LUT_T - 1);
            const c = Math.abs(Math.cos(theta)), s = Math.abs(Math.sin(theta));
            const sum = Math.pow(c, n) + Math.pow(s, n);
            t[ni * LUT_T + ti] = Math.pow(sum, -1.0 / n);
        }
    }
    return t;
})();

// SuperellipseLUT::lookup — bilinear; θ in radians (any value), n ∈ [0.5, 6].
function superLookup(theta, n) {
    let t = theta - QUADRANT * Math.floor(theta / QUADRANT);
    if (t < 0) t = 0;
    const tf = t * ((LUT_T - 1) / QUADRANT);
    let ti = Math.trunc(tf);
    if (ti > LUT_T - 2) ti = LUT_T - 2;
    const tfr = tf - ti;
    const nf = clamp((n - LUT_NMIN) * ((LUT_N - 1) / (LUT_NMAX - LUT_NMIN)), 0, LUT_N - 1);
    let ni = Math.trunc(nf);
    if (ni > LUT_N - 2) ni = LUT_N - 2;
    const nfr = nf - ni;
    const r0 = ni * LUT_T, r1 = r0 + LUT_T;
    const a = SUPER_LUT[r0 + ti] + tfr * (SUPER_LUT[r0 + ti + 1] - SUPER_LUT[r0 + ti]);
    const b = SUPER_LUT[r1 + ti] + tfr * (SUPER_LUT[r1 + ti + 1] - SUPER_LUT[r1 + ti]);
    return a + nfr * (b - a);
}

// juce::dsp::FastMathApproximations::tanh — the 7/6 Padé the Squarcle branch uses.
function padeTanh(x) {
    const x2 = x * x;
    const num = x * (135135 + x2 * (17325 + x2 * (378 + x2)));
    const den = 135135 + x2 * (62370 + x2 * (3150 + 28 * x2));
    return num / den;
}

// OrbitDetail::rawOrbit — the raw curve at θ for Orbit Mod m (before normalisation / affine).
function rawOrbit(kind, theta, m, scratch) {
    const c = Math.cos(theta), s = Math.sin(theta);
    switch (kind) {
        case 0: return [c, s];                                                        // Ellipse
        case 1: { const r = superLookup(theta, 0.5 + 5.5 * m); return [r * c, r * s]; }   // Superellipse
        case 2: { const r = 1.0 + 2.0 * m * c; return [r * c, r * s]; }              // Limaçon
        case 3: case 4: case 5: {                                                     // Epitrochoid: p + 1
            const p1 = kind === 3 ? 4.0 : kind === 4 ? 6.0 : 8.0;
            const d = 0.1 + 2.9 * m;
            return [p1 * c - d * Math.cos(p1 * theta), p1 * s - d * Math.sin(p1 * theta)];
        }
        case 6: case 7: case 8: {                                                     // Hypocycloid: p − 1
            const pm = kind === 6 ? 2.0 : kind === 7 ? 4.0 : 6.0;
            const d = 0.1 + 1.9 * m;
            return [pm * c + d * Math.cos(pm * theta), pm * s - d * Math.sin(pm * theta)];
        }
        case 9: {                                                                     // Butterfly (C++ axis order: polar (r, c, s))
            const s2 = s * s, s5 = s2 * s2 * s;
            const r = Math.exp(c) - 2.0 * Math.cos(4.0 * theta) + 2.0 * m * s5;
            return [r * c, r * s];
        }
        case 10: {                                                                    // Squarcle
            const k = 0.3 + 6.0 * m;
            const ax = clamp(k * c, -3.0, 3.0), ay = clamp(k * s, -3.0, 3.0);
            return [padeTanh(ax) * scratch.invTanhK, padeTanh(ay) * scratch.invTanhK];
        }
        default: return [c, s];
    }
}

// OrbitDetail::maxRadius — 64-point scan of the raw curve.
function maxRadius(kind, m, scratch) {
    if (kind === 0) return 1.0;
    let maxR2 = 1.0e-12;
    for (let i = 0; i < 64; i++) {
        const theta = 6.283185307179586 * i / 64.0;
        const p = rawOrbit(kind, theta, m, scratch);
        maxR2 = Math.max(maxR2, p[0] * p[0] + p[1] * p[1]);
    }
    return Math.sqrt(maxR2);
}

// OrbitScratch as TerrainOscillator::updateBlockRate fills it for (kind, m).
export function orbitScratch(kind, m) {
    const sc = { normFactor: 1.0, invTanhK: kind === 10 ? 1.0 / Math.tanh(0.3 + 6.0 * m) : 1.0 };
    sc.normFactor = 1.0 / maxRadius(kind, m, sc);
    return sc;
}
let scKind = -1, scM = NaN, scCache = null;
function scratchFor(kind, m) {
    if (kind !== scKind || m !== scM) { scCache = orbitScratch(kind, m); scKind = kind; scM = m; }
    return scCache;
}

/** Normalised base orbit (max radius 1 at Orbit Mod m) — Orbits.h baseOrbit(). */
export function baseOrbit(kind, theta, m) {
    const sc = scratchFor(kind, m), p = rawOrbit(kind, theta, m, sc);
    return [p[0] * sc.normFactor, p[1] * sc.normFactor];
}

/** The oscillator's affine of the base orbit (TerrainOscillator::scan order: aspect on
    y, rotate, r = 0.05 + 0.95 · size, centre, clamp ±1). `p` = the page's snapshot():
    orbit (kind index), m ∈ [0, 1], size ∈ [0, 1], aspect (raw 0.1–1), rot (radians),
    cx / cy ∈ [−1, 1]. */
export function orbitPoint(p, th) {
    const b = baseOrbit(p.orbit, th, p.m);
    const bx = b[0], by = b[1] * p.aspect;
    const r = 0.05 + 0.95 * p.size;
    const ca = Math.cos(p.rot), sa = Math.sin(p.rot);
    return [clamp((bx * ca - by * sa) * r + p.cx, -1, 1), clamp((bx * sa + by * ca) * r + p.cy, -1, 1)];
}

// ═══════════════════════════════════════════════════════════════════
// Heightmap sampling (the pushed 64 × 64 R32F grid: x_i = −1 + 2i/63, row j = 0 ⇒ y = −1)
// ═══════════════════════════════════════════════════════════════════
export function sampleH(hm, x, y) {           // bilinear, x/y in [-1,1]
    const fx = (clamp(x, -1, 1) + 1) * 0.5 * (HM_N - 1), fy = (clamp(y, -1, 1) + 1) * 0.5 * (HM_N - 1);
    const i0 = Math.floor(fx), j0 = Math.floor(fy), i1 = Math.min(i0 + 1, HM_N - 1), j1 = Math.min(j0 + 1, HM_N - 1);
    const tx = fx - i0, ty = fy - j0, d = hm.data;
    const a = d[j0 * HM_N + i0] * (1 - tx) + d[j0 * HM_N + i1] * tx;
    const b = d[j1 * HM_N + i0] * (1 - tx) + d[j1 * HM_N + i1] * tx;
    return a * (1 - ty) + b * ty;
}
export function surfaceH(hm, p, x, y) {
    let h = sampleH(hm, x, y);
    if (p.sat > 0) { const k = 1 + p.sat * 4; h = Math.tanh(k * h) / Math.tanh(k); }   // TerrainOscillator::saturate: g = 1 + 4·sat
    return h;
}

/** The ≋ view's data from a pushed cycle (Float32Array 512 × {px, py, y}) — pure. */
export function cycleFor(d) {
    if (!d) return null;
    const n = d.length / 3, y = new Float64Array(n);
    let peak = 1e-6;
    for (let i = 0; i < n; i++) { y[i] = d[i * 3 + 2]; peak = Math.max(peak, Math.abs(y[i])); }
    return { y: y, n: n, peak: peak };
}

// ═══════════════════════════════════════════════════════════════════
// CAMERA (solved per aspect, not picked: see v2-ui.yaml view_3d.renderer.camera)
// ═══════════════════════════════════════════════════════════════════
function perspective(fovy, aspect, near, far) {
    const f = 1 / Math.tan(fovy / 2), nf = 1 / (near - far);
    return new Float32Array([f / aspect, 0, 0, 0, 0, f, 0, 0, 0, 0, (far + near) * nf, -1, 0, 0, 2 * far * near * nf, 0]);
}
function lookAt(eye, c, up) {
    let zx = eye[0] - c[0], zy = eye[1] - c[1], zz = eye[2] - c[2];
    let l = Math.hypot(zx, zy, zz); zx /= l; zy /= l; zz /= l;
    let xx = up[1] * zz - up[2] * zy, xy = up[2] * zx - up[0] * zz, xz = up[0] * zy - up[1] * zx;
    l = Math.hypot(xx, xy, xz); xx /= l; xy /= l; xz /= l;
    const yx = zy * xz - zz * xy, yy = zz * xx - zx * xz, yz = zx * xy - zy * xx;
    return new Float32Array([xx, yx, zx, 0, xy, yy, zy, 0, xz, yz, zz, 0,
        -(xx * eye[0] + xy * eye[1] + xz * eye[2]), -(yx * eye[0] + yy * eye[1] + yz * eye[2]), -(zx * eye[0] + zy * eye[1] + zz * eye[2]), 1]);
}
function mmul(a, b) {
    const o = new Float32Array(16);
    for (let i = 0; i < 4; i++) for (let j = 0; j < 4; j++) { let s = 0; for (let k = 0; k < 4; k++) s += a[k * 4 + j] * b[i * 4 + k]; o[i * 4 + j] = s; }
    return o;
}
const CAM = { yaw: -0.62, pitch: 0.52, fov: 0.82 };
function viewProjAt(aspect, dist) {
    const e = [dist * Math.cos(CAM.pitch) * Math.sin(CAM.yaw), dist * Math.sin(CAM.pitch), dist * Math.cos(CAM.pitch) * Math.cos(CAM.yaw)];
    return { vp: mmul(perspective(CAM.fov, aspect, 0.1, 40), lookAt(e, [0, 0, 0], [0, 1, 0])), eye: e };
}
const FIT_CORNERS = [];
for (const x of [-1, 1]) for (const z of [-1, 1]) for (const h of [-0.28, 0.28]) FIT_CORNERS.push([x, h, z]);
export function viewProj(aspect) {
    let dist = 2.10;
    for (let it = 0; it < 60; it++) {
        const M = viewProjAt(aspect, dist);
        let worst = 0;
        for (let i = 0; i < FIT_CORNERS.length; i++) {
            const c = FIT_CORNERS[i], m = M.vp;
            const w = m[3] * c[0] + m[7] * c[1] + m[11] * c[2] + m[15];
            worst = Math.max(worst, Math.abs((m[0] * c[0] + m[4] * c[1] + m[8] * c[2] + m[12]) / w), Math.abs((m[1] * c[0] + m[5] * c[1] + m[9] * c[2] + m[13]) / w));
        }
        if (worst <= 0.97) return M;
        dist *= 1.025;
    }
    return viewProjAt(aspect, dist);
}
// Screen → the y = 0 plane (the inverse of the x, z, w homography of VP by Cramer; RESEARCH §2.9).
function unprojectWith(M, W, H, px, py) {
    const m = M.vp, nx = px / W * 2 - 1, ny = 1 - py / H * 2;
    const a1 = m[0] - nx * m[3], b1 = m[8] - nx * m[11], c1 = m[12] - nx * m[15];
    const a2 = m[1] - ny * m[3], b2 = m[9] - ny * m[11], c2 = m[13] - ny * m[15];
    const det = a1 * b2 - a2 * b1;
    if (Math.abs(det) < 1e-9) return null;
    return [(-c1 * b2 + c2 * b1) / det, (-a1 * c2 + a2 * c1) / det];
}

// ═══════════════════════════════════════════════════════════════════
// PERF-03 (plan Decision 37): a 300-frame ring around the big view's render —
// finish-inclusive (gl.finish() before the second clock read) and submit-only —
// plus burst(k): k back-to-back draws with a synthetic θ sweep and one finish().
// Idle cost zero: the ring is written only when a frame is drawn. The page
// forwards snapshot() to the native `reportViewPerf` when the ring wraps
// (onWrap) and after every burst.
// ═══════════════════════════════════════════════════════════════════
export const perf = (() => {
    const K = 300;
    const finish = new Float64Array(K), submit = new Float64Array(K);
    let idx = 0, filled = 0, meta = { mode: '', dpr: 1, w: 0, h: 0 }, burstMean = null, burstFn = null;
    const self = {
        size: K,
        onWrap: null,          // (snapshot) => void, set by the page
        record(finishMs, submitMs, m) {
            finish[idx] = finishMs; submit[idx] = submitMs; meta = m;
            idx = (idx + 1) % K; if (filled < K) filled++;
            if (idx === 0 && typeof self.onWrap === 'function') { try { self.onWrap(self.snapshot()); } catch (e) { /* the report is best-effort */ } }
        },
        attach(fn) { burstFn = fn; },
        burst(k) {
            k = k > 0 ? Math.round(k) : K;
            if (!burstFn) return null;
            burstMean = burstFn(k);
            return burstMean;
        },
        snapshot() {
            let sum = 0, max = 0, sumSubmit = 0;
            for (let i = 0; i < filled; i++) { sum += finish[i]; max = Math.max(max, finish[i]); sumSubmit += submit[i]; }
            return {
                mode: meta.mode, dpr: meta.dpr, w: meta.w, h: meta.h, n: filled,
                mean: filled ? sum / filled : 0, max: max, submitMean: filled ? sumSubmit / filled : 0,
                burstMean: burstMean
            };
        }
    };
    return self;
})();

// Wraps a scene's draw so the big view is timed (finish-inclusive + submit-only) and
// registers the burst driver. `finishFn` blocks until the GPU is done (gl.finish) or is null.
function instrument(scene, drawFn, finishFn) {
    let last = null, attached = false;
    const burst = (k) => {
        if (!last) return null;
        const t0 = performance.now();
        for (let i = 0; i < k; i++) {
            const th = i / k * TAU;
            drawFn(last.hm, last.p, { theta: th, x: 0, y: 0, h: 0, synthetic: true }, last.cycle, last.mode, true);
        }
        if (finishFn) finishFn();
        return (performance.now() - t0) / k;
    };
    scene.render = (hm, p, playhead, cycle, mode, big) => {
        if (!big) { drawFn(hm, p, playhead, cycle, mode, big); return; }
        if (!attached) { perf.attach(burst); attached = true; }   // the big view owns the burst driver
        last = { hm, p, cycle, mode };
        const t0 = performance.now();
        drawFn(hm, p, playhead, cycle, mode, big);
        const t1 = performance.now();
        if (finishFn) finishFn();
        const t2 = performance.now();
        const sz = scene.size();
        perf.record(t2 - t0, t1 - t0, { mode: scene.kind, dpr: scene.dpr(), w: sz[0], h: sz[1] });
    };
    return scene;
}

// Playhead → the scan point's world position. Before the first `terrainState` push (null),
// or under a synthetic burst sweep, the point rides the undisplaced orbit at θ.
function scanPoint(hm, p, playhead, orbAt) {
    if (playhead && !playhead.synthetic) return [playhead.x, playhead.h * HSCALE + 0.012, playhead.y];
    const th = playhead ? playhead.theta : 0;
    return orbAt(th);
}
function waveIndex(playhead, N) {
    const th = playhead ? playhead.theta : 0;
    const k = Math.round(th / TAU * N);
    return ((k % N) + N) % N;
}

// ═══════════════════════════════════════════════════════════════════
// RENDERERS. makeScene() returns the same interface from either path:
//   Canvas 2D  — the two 160×90 Synth canvases, and the big view's fallback
//   WebGL2     — the big view (R32F heightmap texture, ribbon lines, no GLSL `flat`)
// The wireframe is the expensive static layer; both paths cache it and only
// redraw the trail / orbit / scan layer per frame.
//   render (hm, p, playhead, cycle, mode, big):
//     hm       {gen, data: Float32Array(4096)} — the pushed heightmap (or the zero map)
//     p        snapshot() of the oscillator
//     playhead {theta, x, y, h} from `terrainState`, or null before the first push
//     cycle    Float32Array(512 × 3) from `terrainCycle`, or null — the feedback trail
//     mode     '3d' | 'wave'
// ═══════════════════════════════════════════════════════════════════
export function makeScene2D(canvas, grid, step) {
    const wire = document.createElement('canvas');
    let key = '', W = 0, H = 0, dpr = 1, M = null;
    function fit() {
        const r = canvas.getBoundingClientRect(), d = window.devicePixelRatio || 1;
        const w = Math.max(1, Math.round(r.width)), h = Math.max(1, Math.round(r.height));
        if (w !== W || h !== H || d !== dpr) {
            W = w; H = h; dpr = d;
            canvas.width = Math.round(W * dpr); canvas.height = Math.round(H * dpr);
            wire.width = canvas.width; wire.height = canvas.height;
            M = viewProj(W / H); key = '';
        }
        if (!M) M = viewProj(W / H);
    }
    function proj(x, h, z) {
        const m = M.vp, w = m[3] * x + m[7] * h + m[11] * z + m[15];
        return [((m[0] * x + m[4] * h + m[8] * z + m[12]) / w * 0.5 + 0.5) * W, (0.5 - (m[1] * x + m[5] * h + m[9] * z + m[13]) / w * 0.5) * H];
    }
    function unproject(px, py) { if (!M) fit(); return unprojectWith(M, W, H, px, py); }
    function buildWire(hm) {
        const k = [hm.gen, W, H, dpr].join('|');
        if (k === key) return;
        key = k;
        const c = wire.getContext('2d');
        c.setTransform(dpr, 0, 0, dpr, 0, 0); c.clearRect(0, 0, W, H);
        const N = grid, S = step;
        c.beginPath();
        [[-1, -1], [1, -1], [1, 1], [-1, 1]].forEach((q, i) => { const s = proj(q[0], 0, q[1]); i ? c.lineTo(s[0], s[1]) : c.moveTo(s[0], s[1]); });
        c.closePath();
        c.fillStyle = 'rgba(139,115,85,0.055)'; c.fill();
        c.strokeStyle = 'rgba(139,115,85,0.32)'; c.lineWidth = 1; c.setLineDash([3, 3]); c.stroke(); c.setLineDash([]);
        const eye = M.eye, lines = [];
        for (let j = 0; j < N; j += S) lines.push({ row: true, k: j, d: Math.hypot(eye[0], eye[2] - (j / (N - 1) * 2 - 1)) });
        for (let i = 0; i < N; i += S) lines.push({ row: false, k: i, d: Math.hypot(eye[0] - (i / (N - 1) * 2 - 1), eye[2]) });
        lines.sort((a, b) => b.d - a.d);
        const dmin = Math.min.apply(null, lines.map(l => l.d)), dmax = Math.max.apply(null, lines.map(l => l.d));
        c.lineWidth = 1;
        lines.forEach(l => {
            const t = dmax > dmin ? (l.d - dmin) / (dmax - dmin) : 0;
            c.strokeStyle = 'rgba(139,115,85,' + (0.64 - 0.34 * t).toFixed(3) + ')';
            c.beginPath();
            for (let n = 0; n < N; n++) {
                const i = l.row ? n : l.k, j = l.row ? l.k : n;
                const s = proj(i / (N - 1) * 2 - 1, sampleH(hm, i / (N - 1) * 2 - 1, j / (N - 1) * 2 - 1) * HSCALE, j / (N - 1) * 2 - 1);
                n ? c.lineTo(s[0], s[1]) : c.moveTo(s[0], s[1]);
            }
            c.stroke();
        });
    }
    function draw3d(hm, p, playhead, cycle, big) {
        fit(); buildWire(hm);
        const c = canvas.getContext('2d');
        c.setTransform(dpr, 0, 0, dpr, 0, 0); c.clearRect(0, 0, W, H); c.drawImage(wire, 0, 0, W, H);
        const N = big ? 320 : 160;
        // the feedback trail: the pushed cycle's displaced path lifted onto the heightmap (Decision 33)
        if (p.fb > 0.001 && cycle) {
            const n = cycle.length / 3;
            c.strokeStyle = 'rgba(201,130,43,0.75)'; c.lineWidth = big ? 1.4 : 1; c.beginPath();
            for (let i = 0; i <= n; i++) { const k = (i % n) * 3, x = cycle[k], y = cycle[k + 1]; const s = proj(x, surfaceH(hm, p, x, y) * HSCALE + 0.008, y); i ? c.lineTo(s[0], s[1]) : c.moveTo(s[0], s[1]); }
            c.stroke();
        }
        const orbAt = (th) => { const o = orbitPoint(p, th); return [o[0], surfaceH(hm, p, o[0], o[1]) * HSCALE + 0.012, o[1]]; };
        c.strokeStyle = '#8BA870'; c.lineWidth = big ? 2 : 1.5; c.lineJoin = 'round'; c.beginPath();
        for (let i = 0; i <= N; i++) {
            const o = orbAt((i % N) / N * TAU), s = proj(o[0], o[1], o[2]);
            i ? c.lineTo(s[0], s[1]) : c.moveTo(s[0], s[1]);
        }
        c.stroke();
        const sp = scanPoint(hm, p, playhead, orbAt), s = proj(sp[0], sp[1], sp[2]), rr = big ? 5 : 3;
        c.fillStyle = 'rgba(107,142,78,0.22)'; c.beginPath(); c.arc(s[0], s[1], rr * 2.4, 0, TAU); c.fill();
        c.fillStyle = '#6B8E4E'; c.beginPath(); c.arc(s[0], s[1], rr, 0, TAU); c.fill();
    }
    function drawWave(p, playhead, cycle, big) {
        fit();
        const c = canvas.getContext('2d');
        c.setTransform(dpr, 0, 0, dpr, 0, 0); c.clearRect(0, 0, W, H);
        const w = cycleFor(cycle), N = w ? w.n : 512, pad = big ? 40 : 9, mid = H / 2;
        const amp = (H / 2 - pad) / Math.max(1, w ? w.peak : 1), X = (i) => pad + (W - pad * 2) * (i / N);
        c.strokeStyle = 'rgba(139,115,85,0.26)'; c.lineWidth = 1; c.beginPath(); c.moveTo(pad, mid); c.lineTo(W - pad, mid); c.stroke();
        if (big) { c.strokeStyle = 'rgba(139,115,85,0.16)'; c.setLineDash([2, 4]); [0.25, 0.5, 0.75].forEach(f => { c.beginPath(); c.moveTo(X(N * f), pad); c.lineTo(X(N * f), H - pad); c.stroke(); }); c.setLineDash([]); }
        if (!w) return;   // no cycle pushed yet: baseline only
        c.strokeStyle = '#5C4033'; c.lineWidth = big ? 1.8 : 1.4; c.lineJoin = 'round'; c.lineCap = 'round'; c.beginPath();
        for (let i = 0; i <= N; i++) { const y = mid - w.y[i % N] * amp; i ? c.lineTo(X(i), y) : c.moveTo(X(i), y); }
        c.stroke();
        const pi = waveIndex(playhead, N);
        c.fillStyle = '#6B8E4E'; c.beginPath(); c.arc(X(pi), mid - w.y[pi] * amp, big ? 5 : 3, 0, TAU); c.fill();
    }
    const scene = {
        render: null, unproject: unproject, fit: fit, size: () => [W, H], dpr: () => dpr, kind: '2d',
        lastGLError: () => 0, debugLoseContext: () => false, debugRestoreContext: () => false
    };
    return instrument(scene, (hm, p, playhead, cycle, mode, big) => (mode === 'wave' ? drawWave(p, playhead, cycle, big) : draw3d(hm, p, playhead, cycle, big)), null);
}

// WebGL2 path. Returns null when a context cannot be created (caller falls back).
// `onNeedsRedraw` is called after a context restore so the page redraws.
export function makeSceneGL(canvas, grid, step, onNeedsRedraw) {
    const gl = canvas.getContext('webgl2', { antialias: true, alpha: true, premultipliedAlpha: true, preserveDrawingBuffer: false, powerPreference: 'low-power' });
    if (!gl) return null;
    let W = 0, H = 0, dpr = 1, M = null, lost = false, wireGen = -1, glErrAfterUpload = null;
    let R = null;                                    // GL resources (rebuilt after a context restore)
    const loseExt = gl.getExtension('WEBGL_lose_context');

    const VS_WIRE = `#version 300 es
        precision highp float; precision highp sampler2D;
        in vec2 a_ij;
        uniform mat4 u_vp; uniform sampler2D u_hm; uniform float u_n; uniform float u_hscale; uniform vec3 u_eye; uniform vec2 u_dist;
        out float v_alpha;
        void main() {
            float x = a_ij.x / (u_n - 1.0) * 2.0 - 1.0;
            float z = a_ij.y / (u_n - 1.0) * 2.0 - 1.0;
            float h = texelFetch(u_hm, ivec2(a_ij), 0).r * u_hscale;
            float d = distance(u_eye.xz, vec2(x, z));
            float t = clamp((d - u_dist.x) / max(1e-4, u_dist.y - u_dist.x), 0.0, 1.0);
            v_alpha = 0.64 - 0.34 * t;
            gl_Position = u_vp * vec4(x, h, z, 1.0);
        }`;
    const FS_WIRE = `#version 300 es
        precision mediump float;
        in float v_alpha; uniform vec4 u_color; out vec4 o;
        void main() { o = vec4(u_color.rgb * v_alpha * u_color.a, v_alpha * u_color.a); }`;
    // Screen-space ribbon: each polyline vertex is emitted twice (a_side ±1) with
    // its neighbours, and extruded along the screen normal by u_width pixels.
    const VS_RIB = `#version 300 es
        precision highp float;
        in vec3 a_pos; in vec3 a_prev; in vec3 a_next; in float a_side;
        uniform mat4 u_vp; uniform vec2 u_vpx; uniform float u_width;
        void main() {
            vec4 c = u_vp * vec4(a_pos, 1.0), p = u_vp * vec4(a_prev, 1.0), n = u_vp * vec4(a_next, 1.0);
            vec2 cs = c.xy / c.w * u_vpx, ps = p.xy / p.w * u_vpx, ns = n.xy / n.w * u_vpx;
            vec2 d1 = cs - ps, d2 = ns - cs;
            vec2 dir = normalize((length(d1) > 1e-4 ? normalize(d1) : vec2(0)) + (length(d2) > 1e-4 ? normalize(d2) : vec2(0)) + vec2(1e-6));
            vec2 nrm = vec2(-dir.y, dir.x);
            vec2 off = nrm * a_side * u_width * 0.5 / u_vpx * c.w;
            gl_Position = vec4(c.xy + off, c.z, c.w);
        }`;
    const FS_FLAT = `#version 300 es
        precision mediump float; uniform vec4 u_color; out vec4 o;
        void main() { o = vec4(u_color.rgb * u_color.a, u_color.a); }`;
    const VS_PT = `#version 300 es
        precision highp float; in vec3 a_pos; uniform mat4 u_vp; uniform float u_size;
        void main() { gl_Position = u_vp * vec4(a_pos, 1.0); gl_PointSize = u_size; }`;
    const FS_PT = `#version 300 es
        precision mediump float; uniform vec4 u_color; out vec4 o;
        void main() { if (length(gl_PointCoord - 0.5) > 0.5) discard; o = vec4(u_color.rgb * u_color.a, u_color.a); }`;

    function compile(vs, fs) {
        const mk = (t, s) => { const sh = gl.createShader(t); gl.shaderSource(sh, s); gl.compileShader(sh); if (!gl.getShaderParameter(sh, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(sh)); return sh; };
        const pr = gl.createProgram(); gl.attachShader(pr, mk(gl.VERTEX_SHADER, vs)); gl.attachShader(pr, mk(gl.FRAGMENT_SHADER, fs)); gl.linkProgram(pr);
        if (!gl.getProgramParameter(pr, gl.LINK_STATUS)) throw new Error(gl.getProgramInfoLog(pr));
        const u = {}; const n = gl.getProgramParameter(pr, gl.ACTIVE_UNIFORMS);
        for (let i = 0; i < n; i++) { const info = gl.getActiveUniform(pr, i); u[info.name] = gl.getUniformLocation(pr, info.name); }
        return { pr: pr, u: u };
    }
    // Ribbon geometry: interleaved [pos(3) prev(3) next(3) side(1)] = 10 floats per vertex.
    function ribbonData(pts, closed) {
        const n = pts.length / 3;
        const out = new Float32Array(n * 2 * 10);
        for (let i = 0; i < n; i++) {
            const ip = closed ? (i - 1 + n) % n : Math.max(0, i - 1), inx = closed ? (i + 1) % n : Math.min(n - 1, i + 1);
            for (let s = 0; s < 2; s++) {
                const o = (i * 2 + s) * 10;
                out[o] = pts[i * 3]; out[o + 1] = pts[i * 3 + 1]; out[o + 2] = pts[i * 3 + 2];
                out[o + 3] = pts[ip * 3]; out[o + 4] = pts[ip * 3 + 1]; out[o + 5] = pts[ip * 3 + 2];
                out[o + 6] = pts[inx * 3]; out[o + 7] = pts[inx * 3 + 1]; out[o + 8] = pts[inx * 3 + 2];
                out[o + 9] = s ? 1 : -1;
            }
        }
        return out;
    }
    function makeRibbonVAO(prog) {
        const vao = gl.createVertexArray(); gl.bindVertexArray(vao);
        const buf = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, buf);
        const st = 40;
        [['a_pos', 3, 0], ['a_prev', 3, 12], ['a_next', 3, 24], ['a_side', 1, 36]].forEach(([n, sz, off]) => {
            const loc = gl.getAttribLocation(prog.pr, n); gl.enableVertexAttribArray(loc); gl.vertexAttribPointer(loc, sz, gl.FLOAT, false, st, off);
        });
        gl.bindVertexArray(null);
        return { vao, buf };
    }
    function initResources() {
        const N = grid, S = step;
        const ij = new Float32Array(N * N * 2);
        for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) { ij[(j * N + i) * 2] = i; ij[(j * N + i) * 2 + 1] = j; }
        const idx = [];
        for (let j = 0; j < N; j += S) for (let i = 0; i < N - 1; i++) idx.push(j * N + i, j * N + i + 1);
        for (let i = 0; i < N; i += S) for (let j = 0; j < N - 1; j++) idx.push(j * N + i, (j + 1) * N + i);
        const wire = compile(VS_WIRE, FS_WIRE), rib = compile(VS_RIB, FS_FLAT), pt = compile(VS_PT, FS_PT);
        const vaoWire = gl.createVertexArray(); gl.bindVertexArray(vaoWire);
        const bIJ = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, bIJ); gl.bufferData(gl.ARRAY_BUFFER, ij, gl.STATIC_DRAW);
        const aIJ = gl.getAttribLocation(wire.pr, 'a_ij'); gl.enableVertexAttribArray(aIJ); gl.vertexAttribPointer(aIJ, 2, gl.FLOAT, false, 0, 0);
        const bIdx = gl.createBuffer(); gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, bIdx); gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint32Array(idx), gl.STATIC_DRAW);
        gl.bindVertexArray(null);
        const tex = gl.createTexture(); gl.bindTexture(gl.TEXTURE_2D, tex);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST); gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE); gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.R32F, HM_N, HM_N, 0, gl.RED, gl.FLOAT, null);
        // Dynamic ribbon (trail / orbit / wave) and the point VAO.
        const dyn = makeRibbonVAO(rib);
        const vaoPt = gl.createVertexArray(); gl.bindVertexArray(vaoPt);
        const bPt = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, bPt);
        const aP = gl.getAttribLocation(pt.pr, 'a_pos'); gl.enableVertexAttribArray(aP); gl.vertexAttribPointer(aP, 3, gl.FLOAT, false, 0, 0);
        gl.bindVertexArray(null);
        // Ground: two triangles at y = 0, drawn through the point shader's sibling (flat colour).
        const gnd = compile(`#version 300 es
            precision highp float; in vec3 a_pos; uniform mat4 u_vp; void main() { gl_Position = u_vp * vec4(a_pos, 1.0); }`, FS_FLAT);
        const vaoGnd = gl.createVertexArray(); gl.bindVertexArray(vaoGnd);
        const bGnd = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, bGnd);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, 0, -1, 1, 0, -1, 1, 0, 1, -1, 0, -1, 1, 0, 1, -1, 0, 1]), gl.STATIC_DRAW);
        const aG = gl.getAttribLocation(gnd.pr, 'a_pos'); gl.enableVertexAttribArray(aG); gl.vertexAttribPointer(aG, 3, gl.FLOAT, false, 0, 0);
        gl.bindVertexArray(null);
        // Dashed edge of the ground square: ONE static VBO (Decision 36) — 4 edges × 22 dashes
        // as a single TRIANGLE_STRIP with degenerate joins between dashes (the mockup drew
        // 88 bufferData + drawArrays per frame).
        const edgePts = [[-1, -1], [1, -1], [1, 1], [-1, 1]], dashes = [];
        for (let e = 0; e < 4; e++) {
            const a = edgePts[e], b = edgePts[(e + 1) % 4], D = 44;
            for (let s = 0; s < D; s += 2) {
                const t0 = s / D, t1 = (s + 1) / D;
                dashes.push(ribbonData([a[0] + (b[0] - a[0]) * t0, 0, a[1] + (b[1] - a[1]) * t0, a[0] + (b[0] - a[0]) * t1, 0, a[1] + (b[1] - a[1]) * t1], false));
            }
        }
        let nEdge = 0; for (const d of dashes) nEdge += d.length / 10 + 2;
        const edgeData = new Float32Array(nEdge * 10); let o = 0;
        for (const d of dashes) {
            edgeData.set(d.subarray(0, 10), o); o += 10;          // degenerate lead-in (repeats the first vertex)
            edgeData.set(d, o); o += d.length;
            edgeData.set(d.subarray(d.length - 10), o); o += 10; // degenerate lead-out (repeats the last vertex)
        }
        const edge = makeRibbonVAO(rib);
        gl.bindBuffer(gl.ARRAY_BUFFER, edge.buf); gl.bufferData(gl.ARRAY_BUFFER, edgeData, gl.STATIC_DRAW);
        R = { wire, rib, pt, gnd, vaoWire, vaoRib: dyn.vao, bRib: dyn.buf, vaoEdge: edge.vao, nEdge, vaoPt, vaoGnd, bPt, tex, nIdx: idx.length };
        wireGen = -1;
        gl.enable(gl.BLEND); gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_ALPHA); gl.disable(gl.DEPTH_TEST);
    }
    canvas.addEventListener('webglcontextlost', (e) => { e.preventDefault(); lost = true; R = null; }, false);
    canvas.addEventListener('webglcontextrestored', () => {
        lost = false;
        try { initResources(); } catch (e) { console.error('[terrain] GL restore failed:', e); }
        if (typeof onNeedsRedraw === 'function') onNeedsRedraw();
    }, false);
    try { initResources(); } catch (e) { console.error('[terrain] GL init failed:', e); return null; }

    function fit() {
        const r = canvas.getBoundingClientRect(), d = window.devicePixelRatio || 1;
        const w = Math.max(1, Math.round(r.width)), h = Math.max(1, Math.round(r.height));
        if (w !== W || h !== H || d !== dpr) {
            W = w; H = h; dpr = d;
            canvas.width = Math.round(W * dpr); canvas.height = Math.round(H * dpr);
            M = viewProj(W / H);
        }
        if (!M) M = viewProj(W / H);
    }
    function unproject(px, py) { if (!M) fit(); return unprojectWith(M, W, H, px, py); }
    const IDENT = new Float32Array([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
    const col = (r, g, b, a) => [r / 255, g / 255, b / 255, a];
    function useRibbon(vp, widthPx, color) {
        gl.useProgram(R.rib.pr);
        gl.uniformMatrix4fv(R.rib.u.u_vp, false, vp); gl.uniform2f(R.rib.u.u_vpx, canvas.width / 2, canvas.height / 2);
        gl.uniform1f(R.rib.u.u_width, widthPx * dpr); gl.uniform4fv(R.rib.u.u_color, color);
    }
    function ribbon(vp, pts, closed, widthPx, color) {          // pts: flat [x,y,z,...] world (or NDC when vp = IDENT)
        const n = pts.length / 3; if (n < 2) return;
        const out = ribbonData(pts, closed);
        useRibbon(vp, widthPx, color); gl.bindVertexArray(R.vaoRib);
        gl.bindBuffer(gl.ARRAY_BUFFER, R.bRib); gl.bufferData(gl.ARRAY_BUFFER, out, gl.DYNAMIC_DRAW);
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, n * 2);
        gl.bindVertexArray(null);
    }
    function point(vp, x, y, z, sizePx, color) {
        gl.useProgram(R.pt.pr); gl.bindVertexArray(R.vaoPt);
        gl.bindBuffer(gl.ARRAY_BUFFER, R.bPt); gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([x, y, z]), gl.DYNAMIC_DRAW);
        gl.uniformMatrix4fv(R.pt.u.u_vp, false, vp); gl.uniform1f(R.pt.u.u_size, sizePx * dpr); gl.uniform4fv(R.pt.u.u_color, color);
        gl.drawArrays(gl.POINTS, 0, 1);
        gl.bindVertexArray(null);
    }
    function uploadHeightmap(hm) {
        if (hm.gen === wireGen) return;
        wireGen = hm.gen;
        gl.bindTexture(gl.TEXTURE_2D, R.tex);
        gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, HM_N, HM_N, gl.RED, gl.FLOAT, hm.data);
        if (glErrAfterUpload === null) glErrAfterUpload = gl.getError();   // read once, after the first upload (the gate reads it)
    }
    function begin() {
        gl.viewport(0, 0, canvas.width, canvas.height);
        gl.clearColor(0, 0, 0, 0); gl.clear(gl.COLOR_BUFFER_BIT);
    }
    function draw3d(hm, p, playhead, cycle, big) {
        fit(); if (lost || !R) return;
        begin(); uploadHeightmap(hm);
        const vp = M.vp, eye = M.eye;
        // ground wash + the static dashed edge
        gl.useProgram(R.gnd.pr); gl.bindVertexArray(R.vaoGnd);
        gl.uniformMatrix4fv(R.gnd.u.u_vp, false, vp); gl.uniform4fv(R.gnd.u.u_color, col(139, 115, 85, 0.055));
        gl.drawArrays(gl.TRIANGLES, 0, 6); gl.bindVertexArray(null);
        useRibbon(vp, 1, col(139, 115, 85, 0.32)); gl.bindVertexArray(R.vaoEdge);
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, R.nEdge); gl.bindVertexArray(null);
        // wireframe from the R32F heightmap (vertex texture fetch; alpha ramps with distance from the eye)
        let dmin = Infinity, dmax = 0;
        for (const x of [-1, 1]) for (const z of [-1, 1]) { const d = Math.hypot(eye[0] - x, eye[2] - z); dmin = Math.min(dmin, d); dmax = Math.max(dmax, d); }
        gl.useProgram(R.wire.pr); gl.bindVertexArray(R.vaoWire);
        gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, R.tex); gl.uniform1i(R.wire.u.u_hm, 0);
        gl.uniformMatrix4fv(R.wire.u.u_vp, false, vp); gl.uniform1f(R.wire.u.u_n, grid); gl.uniform1f(R.wire.u.u_hscale, HSCALE);
        gl.uniform3fv(R.wire.u.u_eye, eye); gl.uniform2f(R.wire.u.u_dist, dmin, dmax); gl.uniform4fv(R.wire.u.u_color, col(139, 115, 85, 1));
        gl.drawElements(gl.LINES, R.nIdx, gl.UNSIGNED_INT, 0); gl.bindVertexArray(null);
        const N = big ? 320 : 160;
        // the feedback trail: the pushed cycle's displaced path lifted onto the heightmap (Decision 33)
        if (p.fb > 0.001 && cycle) {
            const n = cycle.length / 3, pts = new Float32Array(n * 3);
            for (let i = 0; i < n; i++) { const x = cycle[i * 3], y = cycle[i * 3 + 1]; pts[i * 3] = x; pts[i * 3 + 1] = surfaceH(hm, p, x, y) * HSCALE + 0.008; pts[i * 3 + 2] = y; }
            ribbon(vp, pts, true, big ? 1.4 : 1, col(201, 130, 43, 0.75));
        }
        const orbAt = (th) => { const o = orbitPoint(p, th); return [o[0], surfaceH(hm, p, o[0], o[1]) * HSCALE + 0.012, o[1]]; };
        const orb = new Float32Array(N * 3);
        for (let i = 0; i < N; i++) { const o = orbAt(i / N * TAU); orb[i * 3] = o[0]; orb[i * 3 + 1] = o[1]; orb[i * 3 + 2] = o[2]; }
        ribbon(vp, orb, true, big ? 2 : 1.5, col(139, 168, 112, 1));
        const sp = scanPoint(hm, p, playhead, orbAt), rr = big ? 5 : 3;
        point(vp, sp[0], sp[1], sp[2], rr * 4.8, col(107, 142, 78, 0.22));
        point(vp, sp[0], sp[1], sp[2], rr * 2,   col(107, 142, 78, 1));
    }
    function drawWave(p, playhead, cycle, big) {
        fit(); if (lost || !R) return;
        begin();
        const w = cycleFor(cycle), N = w ? w.n : 512, pad = big ? 40 : 9;
        const amp = (H / 2 - pad) / Math.max(1, w ? w.peak : 1);
        const X = (i) => (pad + (W - pad * 2) * (i / N)) / W * 2 - 1, Y = (v) => -(v * amp) / (H / 2);
        ribbon(IDENT, [X(0), 0, 0, X(N), 0, 0], false, 1, col(139, 115, 85, 0.26));
        if (big) [0.25, 0.5, 0.75].forEach(f => { for (let s = 0; s < H - pad * 2; s += 6) { const y0 = 1 - (pad + s) / H * 2, y1 = 1 - (pad + s + 2) / H * 2; ribbon(IDENT, [X(N * f), y0, 0, X(N * f), y1, 0], false, 1, col(139, 115, 85, 0.16)); } });
        if (!w) return;   // no cycle pushed yet: baseline only
        const pts = new Float32Array((N + 1) * 3);
        for (let i = 0; i <= N; i++) { pts[i * 3] = X(i); pts[i * 3 + 1] = Y(w.y[i % N]); pts[i * 3 + 2] = 0; }
        ribbon(IDENT, pts, false, big ? 1.8 : 1.4, col(92, 64, 51, 1));
        const pi = waveIndex(playhead, N);
        point(IDENT, X(pi), Y(w.y[pi]), 0, (big ? 5 : 3) * 2, col(107, 142, 78, 1));
    }
    const scene = {
        render: null, unproject: unproject, fit: fit, size: () => [W, H], dpr: () => dpr, kind: 'webgl2',
        lastGLError: () => (glErrAfterUpload === null ? gl.getError() : glErrAfterUpload),
        debugLoseContext: () => { if (!loseExt) return false; loseExt.loseContext(); return true; },
        debugRestoreContext: () => { if (!loseExt) return false; loseExt.restoreContext(); return true; }
    };
    return instrument(scene, (hm, p, playhead, cycle, mode, big) => (mode === 'wave' ? drawWave(p, playhead, cycle, big) : draw3d(hm, p, playhead, cycle, big)), () => gl.finish());
}

/** makeScene (canvas, grid, step, preferGL, onGLUnavailable, onNeedsRedraw): WebGL2 when
    asked for and available, else Canvas 2D — `onGLUnavailable` (nullable) is called once
    on the fallback so the page can show its localised placeholder (Decision 36: it stays). */
export function makeScene(canvas, grid, step, preferGL, onGLUnavailable, onNeedsRedraw) {
    if (preferGL) {
        const gls = makeSceneGL(canvas, grid, step, onNeedsRedraw);
        if (gls) return gls;
        if (typeof onGLUnavailable === 'function') onGLUnavailable();
    }
    return makeScene2D(canvas, grid, step);
}
