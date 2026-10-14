#!/usr/bin/env node
/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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
/*
  ==============================================================================

    generate_sh_icons.mjs — the 16 spherical-harmonic pattern icons (Stage 3,
    UI-01), rendered OFFLINE from the real SN3D functions. Node built-ins only.

      node plugins/O-simpleAmbisonics/scripts/generate_sh_icons.mjs
          writes Source/ui/public/js/shicons.js (data only, no prose) and
          .planning/stages/3-gui/sh-icons-contact.html (review sheet)
      node … --check
          regenerates in memory and byte-compares with the committed
          shicons.js; exit 1 on drift

    Rendering: the balloon r(e, a) = |Y_acn| / max|Y_acn| (per-icon
    normalised — an icon is a shape glyph, not a level) on an elevation ×
    azimuth mesh, orthographic, camera "the top-down sphere view tilted toward
    you": front recedes up, left stays left, ELEVATION_DEG down from the
    horizon plus a TURN_DEG turn toward the right so X and Z do not stack.
    Back-face culled quads in painter's order (far first); polarity = sign of
    Y at the quad centre (sage + / amber −, the meter colours); three Lambert
    steps from a fixed light. 18 × 18 viewBox, 2-decimal coordinates.

    Self-checks (any failure exits non-zero WITHOUT writing):
      1 the closed forms (ShEncoder.h:107-136) equal an INDEPENDENT SN3D
        formula N·P_l^|m|(sin e)·trig(m a) — associated-Legendre recursion,
        no Condon–Shortley phase — to 1e-12 on a 13 × 7 grid
      2 the ACN → letter table is W Y Z X V T R S U Q O M K L N P
      3 polarity anchors X(front) Y(left) Z(up) R(up) U(front) V(front-left)
        all > 0
      4 two in-memory runs are byte-identical; the module carries this file's
        sha256 and the camera constants
      5 every icon has ≥ 1 path and no stroke thinner than 0.6 viewBox units

  ==============================================================================
*/

import { createHash } from 'node:crypto';
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = dirname(fileURLToPath(import.meta.url));
const PLUGIN = resolve(HERE, '..');
const OUT_JS = join(PLUGIN, 'Source/ui/public/js/shicons.js');
const OUT_SHEET = join(PLUGIN, '.planning/stages/3-gui/sh-icons-contact.html');

// ── camera / mesh constants (tuned at the visual checkpoint) ─────────────────
const ELEVATION_DEG = 55;    // camera above the horizon
const TURN_DEG = 20;         // camera turned toward the listener's right
const MESH_E = 16;           // elevation steps (pole to pole)
const MESH_A = 32;           // azimuth steps
const BOX = 18, HALF = 9, FIT = 8.4;
const STROKE_W = 0.6;        // seam-closing stroke, same colour as the fill
const SAGE = '#9ec46f', AMBER = '#e8b04a';
const SHADE_K = [0.62, 0.81, 1.0];
const LIGHT = [0.35, 0.55, 0.76];   // camera space: right, up, toward the viewer (normalised below)

const LETTERS = ['W', 'Y', 'Z', 'X', 'V', 'T', 'R', 'S', 'U', 'Q', 'O', 'M', 'K', 'L', 'N', 'P'];
const LETTERS_REF = 'W Y Z X V T R S U Q O M K L N P'.split(' ');   // Wikipedia, ACN order

const D2R = Math.PI / 180;

// ── 1a. closed forms, lifted from Source/dsp/ShEncoder.h:107-136 (double) ────
const kSqrt3_2 = Math.sqrt(3) / 2, kSqrt15_2 = Math.sqrt(15) / 2, kSqrt5_8 = Math.sqrt(5 / 8), kSqrt3_8 = Math.sqrt(3 / 8);
function shClosed(az, el) {
  const sinA = Math.sin(az), cosA = Math.cos(az), sinE = Math.sin(el), cosE = Math.cos(el);
  const sin2A = 2 * sinA * cosA, cos2A = cosA * cosA - sinA * sinA;
  const sin3A = sinA * (3 * cosA * cosA - sinA * sinA), cos3A = cosA * (cosA * cosA - 3 * sinA * sinA);
  const cosE2 = cosE * cosE, cosE3 = cosE2 * cosE, sinE2 = sinE * sinE, sin2E = 2 * sinE * cosE;
  return [
    1,
    sinA * cosE, sinE, cosA * cosE,
    kSqrt3_2 * sin2A * cosE2, kSqrt3_2 * sinA * sin2E, 0.5 * (3 * sinE2 - 1), kSqrt3_2 * cosA * sin2E, kSqrt3_2 * cos2A * cosE2,
    kSqrt5_8 * sin3A * cosE3, kSqrt15_2 * sin2A * sinE * cosE2, kSqrt3_8 * sinA * cosE * (5 * sinE2 - 1),
    0.5 * sinE * (5 * sinE2 - 3), kSqrt3_8 * cosA * cosE * (5 * sinE2 - 1), kSqrt15_2 * cos2A * sinE * cosE2,
    kSqrt5_8 * cos3A * cosE3,
  ];
}

// ── 1b. independent SN3D: N_l^|m| · P_l^|m|(sin e) · (cos m a | sin |m| a) ───
function fact(n) { let f = 1; for (let i = 2; i <= n; i++) f *= i; return f; }
function legendre(l, m, x) {   // associated Legendre, NO Condon–Shortley phase
  let pmm = 1;
  const s = Math.sqrt(Math.max(0, 1 - x * x));
  for (let i = 1; i <= m; i++) pmm *= (2 * i - 1) * s;
  if (l === m) return pmm;
  let pm1 = x * (2 * m + 1) * pmm;
  if (l === m + 1) return pm1;
  let pll = 0;
  for (let ll = m + 2; ll <= l; ll++) {
    pll = (x * (2 * ll - 1) * pm1 - (ll + m - 1) * pmm) / (ll - m);
    pmm = pm1; pm1 = pll;
  }
  return pll;
}
function shIndependent(az, el) {
  const out = new Array(16);
  for (let l = 0; l <= 3; l++)
    for (let m = -l; m <= l; m++) {
      const am = Math.abs(m);
      const n = Math.sqrt((m === 0 ? 1 : 2) * fact(l - am) / fact(l + am));
      const trig = m > 0 ? Math.cos(m * az) : m < 0 ? Math.sin(am * az) : 1;
      out[l * l + l + m] = n * legendre(l, am, Math.sin(el)) * trig;
    }
  return out;
}

// ── self-checks 1–3 ──────────────────────────────────────────────────────────
function selfChecks() {
  const lines = [];
  let maxErr = 0;
  for (let i = 0; i < 13; i++)
    for (let j = 0; j < 7; j++) {
      const az = (-180 + i * 30) * D2R, el = (-90 + j * 30) * D2R;
      const a = shClosed(az, el), b = shIndependent(az, el);
      for (let k = 0; k < 16; k++) maxErr = Math.max(maxErr, Math.abs(a[k] - b[k]));
    }
  if (!(maxErr < 1e-12)) throw new Error(`check 1 FAILED: closed forms vs independent SN3D max |err| ${maxErr}`);
  lines.push(`check 1 PASS: closed forms == independent SN3D (13 x 7 grid), max |err| ${maxErr.toExponential(2)}`);

  if (LETTERS.join(' ') !== LETTERS_REF.join(' ')) throw new Error('check 2 FAILED: letter table');
  lines.push(`check 2 PASS: ACN letters ${LETTERS.join(' ')}`);

  const at = (az, el) => shClosed(az * D2R, el * D2R);
  const anchors = [['X front', at(0, 0)[3]], ['Y left', at(90, 0)[1]], ['Z up', at(0, 90)[2]],
                   ['R up', at(0, 90)[6]], ['U front', at(0, 0)[8]], ['V front-left 45', at(45, 0)[4]]];
  const bad = anchors.filter(([, v]) => !(v > 0));
  if (bad.length) throw new Error(`check 3 FAILED: ${bad.map((b) => b[0]).join(', ')}`);
  lines.push(`check 3 PASS: polarity anchors ${anchors.map(([n, v]) => `${n} ${v.toFixed(3)}`).join(', ')}`);
  return lines;
}

// ── camera basis (world AmbiX: x front, y LEFT, z up) ────────────────────────
function norm(v) { const l = Math.hypot(...v); return v.map((c) => c / l); }
function dot(a, b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
function cross(a, b) { return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]; }
const CAM_AZ = -(180 - TURN_DEG) * D2R;   // behind, turned toward the right (−y)
const CAM_EL = ELEVATION_DEG * D2R;
const C = [Math.cos(CAM_EL) * Math.cos(CAM_AZ), Math.cos(CAM_EL) * Math.sin(CAM_AZ), Math.sin(CAM_EL)];   // toward the viewer
const U = norm([0 - C[2] * C[0], 0 - C[2] * C[1], 1 - C[2] * C[2]]);   // world z projected: screen up
const RIGHT = cross(C.map((c) => -c), U);                               // view dir × up = screen right
const LIGHT_W = (() => { const l = norm(LIGHT); return norm([RIGHT[0] * l[0] + U[0] * l[1] + C[0] * l[2], RIGHT[1] * l[0] + U[1] * l[1] + C[1] * l[2], RIGHT[2] * l[0] + U[2] * l[1] + C[2] * l[2]]); })();

const fx = (v) => (Math.round(v * 100) / 100).toFixed(2).replace(/\.?0+$/, '').replace(/^-0$/, '0') || '0';

function renderIcon(acn) {
  // per-icon max |Y|
  const val = (e, a) => shClosed(a, e)[acn];
  let maxAbs = 0;
  for (let i = 0; i <= 90; i++) for (let j = 0; j < 180; j++) maxAbs = Math.max(maxAbs, Math.abs(val(-Math.PI / 2 + i * Math.PI / 90, j * 2 * Math.PI / 180)));

  const vert = (i, j) => {
    const e = -Math.PI / 2 + i * Math.PI / MESH_E, a = j * 2 * Math.PI / MESH_A;
    const r = Math.abs(val(e, a)) / maxAbs;
    return [r * Math.cos(e) * Math.cos(a), r * Math.cos(e) * Math.sin(a), r * Math.sin(e)];
  };

  const quads = [];
  for (let i = 0; i < MESH_E; i++)
    for (let j = 0; j < MESH_A; j++) {
      const p = [vert(i, j), vert(i, j + 1), vert(i + 1, j + 1), vert(i + 1, j)];
      // normal from the two diagonals (robust for the pole triangles)
      const n = cross([p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2]], [p[3][0] - p[1][0], p[3][1] - p[1][1], p[3][2] - p[1][2]]);
      const nl = Math.hypot(...n);
      if (nl < 1e-9) continue;
      const nn = n.map((c) => c / nl);
      const centre = [0, 1, 2].map((k) => (p[0][k] + p[1][k] + p[2][k] + p[3][k]) / 4);
      // outward: the normal points away from the origin side of the balloon
      const outward = dot(nn, centre) >= 0 ? nn : nn.map((c) => -c);
      if (dot(outward, C) <= 0) continue;                         // back-face cull
      const ec = -Math.PI / 2 + (i + 0.5) * Math.PI / MESH_E, ac = (j + 0.5) * 2 * Math.PI / MESH_A;
      const pol = val(ec, ac) >= 0 ? 1 : -1;
      const lam = Math.max(0, dot(outward, LIGHT_W));
      const shade = lam > 0.66 ? 2 : lam > 0.33 ? 1 : 0;
      quads.push({ p, depth: dot(centre, C), pol, shade });
    }

  quads.sort((a, b) => a.depth - b.depth || 0);                    // far first (stable in Node ≥ 12)

  // projection + per-icon fit
  let ext = 1e-9;
  for (const q of quads) for (const v of q.p) ext = Math.max(ext, Math.abs(dot(v, RIGHT)), Math.abs(dot(v, U)));
  const s = FIT / ext;
  const proj = (v) => [HALF + s * dot(v, RIGHT), HALF - s * dot(v, U)];

  // merge consecutive quads of the same style into one path (keeps painter's order)
  const paths = [];
  for (const q of quads) {
    const pts = q.p.map(proj);
    const area = Math.abs((pts[2][0] - pts[0][0]) * (pts[3][1] - pts[1][1]) - (pts[3][0] - pts[1][0]) * (pts[2][1] - pts[0][1])) / 2;
    if (area < 0.004) continue;
    const sub = `M${fx(pts[0][0])},${fx(pts[0][1])}L${fx(pts[1][0])},${fx(pts[1][1])}L${fx(pts[2][0])},${fx(pts[2][1])}L${fx(pts[3][0])},${fx(pts[3][1])}Z`;
    const last = paths[paths.length - 1];
    if (last && last.pol === q.pol && last.shade === q.shade) last.d += sub;
    else paths.push({ d: sub, pol: q.pol, shade: q.shade });
  }
  return { acn, letter: LETTERS[acn], paths };
}

function shadeColour(pol, shade) {
  const hex = pol > 0 ? SAGE : AMBER;
  const n = parseInt(hex.slice(1), 16), k = SHADE_K[shade];
  const c = [(n >> 16) & 255, (n >> 8) & 255, n & 255].map((x) => Math.round(x * k));
  return `#${c.map((x) => x.toString(16).padStart(2, '0')).join('')}`;
}

function buildModule(sha) {
  const icons = [];
  for (let acn = 0; acn < 16; acn++) icons.push(renderIcon(acn));
  for (const ic of icons) if (ic.paths.length < 1) throw new Error(`check 5 FAILED: icon ${ic.acn} has no path`);
  if (STROKE_W < 0.6) throw new Error('check 5 FAILED: stroke under 0.6');

  const colours = { '1': SHADE_K.map((_, i) => shadeColour(1, i)), '-1': SHADE_K.map((_, i) => shadeColour(-1, i)) };
  const lines = [];
  lines.push('// generated by plugins/O-simpleAmbisonics/scripts/generate_sh_icons.mjs — do not edit');
  lines.push(`export const SH_ICONS_META = Object.freeze({ generator: 'scripts/generate_sh_icons.mjs', sha256: '${sha}', elevationDeg: ${ELEVATION_DEG}, turnDeg: ${TURN_DEG}, mesh: [${MESH_E}, ${MESH_A}], viewBox: '0 0 ${BOX} ${BOX}', strokeWidth: ${STROKE_W} });`);
  lines.push(`export const SH_ICON_COLOURS = Object.freeze({ pos: Object.freeze(${JSON.stringify(colours['1'])}), neg: Object.freeze(${JSON.stringify(colours['-1'])}) });`);
  lines.push('export const SH_ICONS = Object.freeze([');
  for (const ic of icons) {
    lines.push(`  Object.freeze({ acn: ${ic.acn}, letter: '${ic.letter}', paths: Object.freeze([`);
    for (const p of ic.paths) lines.push(`    { d: '${p.d}', pol: ${p.pol}, shade: ${p.shade} },`);
    lines.push('  ]) }),');
  }
  lines.push(']);');
  return { text: lines.join('\n') + '\n', icons, colours };
}

// ── contact sheet: 3D (generated) beside the mockup's 2D slices ──────────────
function sliceIcon(acn) {   // port of v1-ui.html shIconSvg (sheet only)
  const S3 = Math.sqrt(3), S58 = Math.sqrt(5 / 8), S15 = Math.sqrt(15), S38 = Math.sqrt(3 / 8);
  const shVec = (x, y, z) => [1, y, z, x, S3 * x * y, S3 * y * z, (3 * z * z - 1) / 2, S3 * x * z, S3 * (x * x - y * y) / 2,
    S58 * y * (3 * x * x - y * y), S15 * x * y * z, S38 * y * (5 * z * z - 1), z * (5 * z * z - 3) / 2, S38 * x * (5 * z * z - 1),
    S15 * z * (x * x - y * y) / 2, S58 * x * (x * x - 3 * y * y)];
  const planes = { xy: (t) => [Math.cos(t), Math.sin(t), 0], xz: (t) => [Math.cos(t), 0, Math.sin(t)], yz: (t) => [0, Math.cos(t), Math.sin(t)],
    dz: (t) => [Math.cos(t) * Math.SQRT1_2, Math.cos(t) * Math.SQRT1_2, Math.sin(t)] };
  const screen = { xy: (d) => [-d[1], -d[0]], xz: (d) => [d[0], -d[2]], yz: (d) => [-d[1], -d[2]], dz: (d) => [(d[0] + d[1]) * Math.SQRT1_2, -d[2]] };
  let best = 'xy', bestMax = -1;
  for (const k in planes) { let mx = 0; for (let i = 0; i < 72; i++) { const d = planes[k](i * 2 * Math.PI / 72); mx = Math.max(mx, Math.abs(shVec(...d)[acn])); } if (mx > bestMax + 1e-6) { bestMax = mx; best = k; } }
  const pos = [], neg = [];
  for (let i = 0; i <= 96; i++) {
    const d = planes[best](i * 2 * Math.PI / 96), v = shVec(...d)[acn] / Math.max(bestMax, 1e-6), sc = screen[best](d), r = Math.abs(v) * 8.6;
    const pt = `${(9 + sc[0] * r).toFixed(2)},${(9 + sc[1] * r).toFixed(2)}`;
    pos.push(v > 0 ? pt : '9,9'); neg.push(v < 0 ? pt : '9,9');
  }
  return `<svg viewBox="0 0 18 18"><circle cx="9" cy="9" r="8.6" fill="none" stroke="rgba(214,196,160,0.18)" stroke-width="0.5"/>`
    + `<polygon points="${pos.join(' ')}" fill="rgba(158,196,111,0.78)" stroke="#b9dc8c" stroke-width="0.6" stroke-linejoin="round"/>`
    + `<polygon points="${neg.join(' ')}" fill="rgba(232,176,74,0.72)" stroke="#f0c66a" stroke-width="0.6" stroke-linejoin="round"/></svg>`;
}
function symbol3d(ic, colours) {
  const body = ic.paths.map((p) => { const c = colours[String(p.pol)][p.shade]; return `<path d="${p.d}" fill="${c}" stroke="${c}" stroke-width="${STROKE_W}" stroke-linejoin="round"/>`; }).join('');
  return `<symbol id="sh3d-${ic.acn}" viewBox="0 0 18 18"><circle cx="9" cy="9" r="8.6" fill="none" stroke="rgba(214,196,160,0.18)" stroke-width="0.6"/>${body}</symbol>`;
}
function symbolSlice(acn) { return sliceIcon(acn).replace('<svg viewBox="0 0 18 18">', `<symbol id="shsl-${acn}" viewBox="0 0 18 18">`).replace(/<\/svg>$/, '</symbol>'); }
const useOf = (id) => `<svg viewBox="0 0 18 18"><use href="#${id}"/></svg>`;
function contactSheet(built) {
  const scales = [0.6, 1, 1.5, 3];
  const row = (label, fn) => scales.map((k) => `<tr><th>${label} ${k}x</th>${built.icons.map((ic) => `<td><div class="ic" style="width:${18 * k}px;height:${18 * k}px">${fn(ic)}</div></td>`).join('')}</tr>`).join('\n');
  return `<!doctype html>
<html><head><meta charset="utf-8"><title>O-simpleAmbisonics — SH icon contact sheet (3D vs slice)</title>
<style>
  body { background: #e9dfca; font: 13px Georgia, serif; color: #2a2118; margin: 24px; }
  .well { background: #1e1710; border-radius: 6px; padding: 14px; display: inline-block; }
  table { border-collapse: collapse; } th { color: #d6c4a0; font-weight: normal; text-align: right; padding: 4px 10px; white-space: nowrap; }
  td { padding: 4px 6px; text-align: center; vertical-align: middle; } .ic svg { width: 100%; height: 100%; display: block; }
  thead th { text-align: center; color: #d6c4a0; } p { max-width: 900px; }
</style></head><body>
<svg width="0" height="0" style="position:absolute">${built.icons.map((ic) => symbol3d(ic, built.colours)).join('')}${built.icons.map((ic) => symbolSlice(ic.acn)).join('')}</svg>
<h2>SH pattern icons — pick at the visual checkpoint (R4)</h2>
<p>Top block: generated 3D SN3D balloons (camera ${ELEVATION_DEG}° down, ${TURN_DEG}° turn right; sage +, amber −). Bottom block: the mockup's 2D-slice icons (shIconSvg). The 0.6x and 1x rows decide; on the page the icon box is 18 px x uiScale. Generator sha256 ${built.sha}.</p>
<div class="well"><table>
<thead><tr><th></th>${built.icons.map((ic) => `<th>${ic.acn}<br>${ic.letter}</th>`).join('')}</tr></thead>
${row('3D', (ic) => useOf(`sh3d-${ic.acn}`))}
<tr><td colspan="17" style="height:14px"></td></tr>
${row('slice', (ic) => useOf(`shsl-${ic.acn}`))}
</table></div>
</body></html>
`;
}

// ── main ─────────────────────────────────────────────────────────────────────
function main() {
  const check = process.argv.includes('--check');
  const lines = selfChecks();
  const sha = createHash('sha256').update(readFileSync(fileURLToPath(import.meta.url))).digest('hex');
  const a = buildModule(sha), b = buildModule(sha);
  if (a.text !== b.text) throw new Error('check 4 FAILED: two runs differ');
  lines.push(`check 4 PASS: two runs byte-identical (${a.text.length} bytes), sha256 ${sha.slice(0, 16)}…`);
  const nPaths = a.icons.map((ic) => ic.paths.length);
  lines.push(`check 5 PASS: every icon has paths (min ${Math.min(...nPaths)}, max ${Math.max(...nPaths)}), stroke ${STROKE_W}`);
  lines.forEach((l) => console.log(l));

  if (check) {
    let committed = '';
    try { committed = readFileSync(OUT_JS, 'utf8'); } catch { /* missing */ }
    if (committed !== a.text) { console.error('DRIFT: shicons.js differs from the generator output — re-run without --check'); process.exit(1); }
    console.log('--check: shicons.js matches the generator');
    return;
  }
  writeFileSync(OUT_JS, a.text);
  mkdirSync(dirname(OUT_SHEET), { recursive: true });
  writeFileSync(OUT_SHEET, contactSheet({ ...a, sha }));
  console.log(`wrote ${OUT_JS}\nwrote ${OUT_SHEET}`);
}

try { main(); } catch (e) { console.error(String(e.message || e)); process.exit(1); }
