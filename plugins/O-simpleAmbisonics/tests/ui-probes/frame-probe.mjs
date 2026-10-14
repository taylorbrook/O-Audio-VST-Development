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
// ============================================================================
// frame-probe — UI-01 / UI-02 / UI-03 / UI-04 / UI-05 / U1 gate for the
// O-simpleAmbisonics page (Stage 3 PLAN T8).
//
// Serves the REAL page tree (scripts/serve-ui.js buildRoot, generic stub,
// tests/ui-stub/generic-overrides.json — the banner must read seed=overrides)
// and feeds it the EXACT payload strings the editor emits for real processor
// states (tests/ui-fixtures/frames.json, written by the render harness through
// MeterFramePayload.h). Stub parameter states are set to each fixture's params.
// Verdicts are read from CANVAS PIXELS (and, for the drags, the relay states),
// polled until two consecutive reads agree — never a blind sleep.
//
// Arms: A1–A13 (PLAN T8 table); A14 (Stage 4 PLAN T5 — chevron / puck separation).
//
// Usage:
//   node plugins/O-simpleAmbisonics/tests/ui-probes/frame-probe.mjs
//        [--negative-control fuma|pole|idle|chevron] [--verbose]
//
//   --negative-control fuma   left_o1 is emitted with shGain[1] / shGain[3]
//                             swapped (FuMa-style WXYZ order): A1 must FAIL
//   --negative-control pole   the page's `probe-anchor:pole-hold` condition is
//                             removed: A9 must FAIL
//   --negative-control idle   the page's `probe-anchor:idle-fallback` branch
//                             is forced to the frame: A12 must FAIL
//   --negative-control chevron the page's `probe-anchor:chevron-slide` slide is
//                             disabled (the chevron stays on the puck's ray):
//                             A14 must FAIL
//   A control whose patch matches zero or more than one line exits 6 (a
//   control that patches nothing is vacuous).
//
// Exit: normal run — 0 = 14/14 PASS, n = failed arms. Control run — 0 = the
// target arm FAILS as designed, 1 = it passed (the control is VACUOUS).
// 77 = Playwright / Chromium unavailable: NOTHING was verified, never a pass.
//
// TEST TOOL ONLY. Never shipped, never embedded.
// ============================================================================

import { createRequire } from 'module';
import { fileURLToPath } from 'url';
import fs from 'fs';
import path from 'path';

const require = createRequire(import.meta.url);
const HERE    = path.dirname(fileURLToPath(import.meta.url));
const PLUGIN  = path.resolve(HERE, '..', '..');
const REPO    = path.resolve(PLUGIN, '..', '..');
const S       = require(path.join(REPO, 'scripts', 'serve-ui.js'));
const NAME    = 'O-simpleAmbisonics';

const argv = process.argv.slice(2);
const argVal = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const CONTROL = argVal('--negative-control');
const VERBOSE = argv.includes('--verbose');
const CONTROL_TARGET = { fuma: 'A1', pole: 'A9', idle: 'A12', chevron: 'A14' };
if (CONTROL && !CONTROL_TARGET[CONTROL]) { console.error(`unknown --negative-control ${CONTROL}`); process.exit(6); }

const FIXTURES = JSON.parse(fs.readFileSync(path.join(PLUGIN, 'tests', 'ui-fixtures', 'frames.json'), 'utf8'));
const FX = Object.fromEntries(FIXTURES.map((f) => [f.name, f]));

// ── tolerances (probe constants, not page constants) ────────────────────────
const BAR_TOL_PX   = 2.5;   // bar length vs |value| · half-width
const PUCK_TOL_PX  = 3;
const SAME_TOL_PX  = 1;
const SETTLE_MS    = 60;
const SETTLE_MAX   = 4000;

// ── the served tree (+ the control's page patch) ────────────────────────────
function patchAnchor(root, anchor, rewrite) {
  const file = path.join(root, 'index.html');
  const lines = fs.readFileSync(file, 'utf8').split('\n');
  const hits = lines.map((l, i) => (l.includes(anchor) ? i : -1)).filter((i) => i >= 0);
  if (hits.length !== 1) { console.error(`control patch: anchor "${anchor}" matched ${hits.length} lines (need exactly 1)`); process.exit(6); }
  const before = lines[hits[0]];
  const after = rewrite(before);
  if (after === before) { console.error(`control patch: rewrite left "${anchor}" unchanged`); process.exit(6); }
  lines[hits[0]] = after;
  fs.writeFileSync(file, lines.join('\n'));
  console.log(`  control patch: ${before.trim()}  ->  ${after.trim()}`);
}

// ── in-page measurement (evaluated inside the page) ─────────────────────────
const PAGE_HELPERS = () => {
  const dprOf = (c) => c.width / c.getBoundingClientRect().width;
  // The canvases are transparent: composite every read over the well's own
  // background, or a 14 %-alpha fill reads as its full colour.
  let BG = null;
  // The wells paint a vertical gradient var(--well-top) → var(--well-bot): use its colour at the read row.
  const hex = (v) => { const m = v.trim().match(/^#([0-9a-f]{6})$/i); if (!m) return null; const n = parseInt(m[1], 16); return [(n >> 16) & 255, (n >> 8) & 255, n & 255]; };
  const rootStyle = getComputedStyle(document.documentElement);
  const WT = hex(rootStyle.getPropertyValue('--well-top')) || [30, 23, 16];
  const WB = hex(rootStyle.getPropertyValue('--well-bot')) || [30, 23, 16];
  let ROWS = 1;
  const bgOf = () => WT;
  const read = (c) => { BG = bgOf(c); ROWS = c.height; return c.getContext('2d').getImageData(0, 0, c.width, c.height); };
  const px = (img, w, x, y) => {
    const i = (y * w + x) * 4, a = img.data[i + 3] / 255, t = y / Math.max(1, ROWS - 1);
    return [0, 1, 2].map((k) => img.data[i + k] * a + (WT[k] + (WB[k] - WT[k]) * t) * (1 - a));
  };
  const isSage  = ([r, g, b]) => g > r + 8 && g > b + 40 && g > 90;
  const isAmber = ([r, g, b]) => r > g + 15 && g > b + 35 && r > 120;
  const isPeak  = ([r, g, b]) => r > 110 && Math.abs(r - g) < 35 && r - b > 20 && r - b < 80;
  const isBrass = ([r, g, b]) => r > 215 && g > 165 && b < 160 && r - b > 80 && g - b > 45;

  window.__probe = {
    bars() {
      const c = document.getElementById('chanCanvas');
      const well = document.getElementById('chanWell');
      const ctx = c.getContext('2d'), d = dprOf(c), W = c.width;
      const img = read(c);
      const wr = well.getBoundingClientRect();
      const out = [];
      for (let k = 0; k < 16; k++) {
        const cell = document.querySelector(`.bar-cell[data-acn="${k}"]`).getBoundingClientRect();
        const rx = cell.left - wr.left - 2, ry = cell.top - wr.top - 2, rw = cell.width;
        const xc = rx + rw / 2, half = rw / 2 - 2;
        const yBar = Math.round((ry + 3 + 4.5) * d), yPeak = Math.round((ry + 3 + 9 + 3) * d);
        let pos = 0, neg = 0, peak = 0;
        for (let x = Math.round((xc + 1) * d); x <= Math.round((xc + half + 2) * d); x++) if (isSage(px(img, W, x, yBar))) pos = x / d - xc;
        for (let x = Math.round((xc - 1) * d); x >= Math.round((xc - half - 2) * d); x--) if (isAmber(px(img, W, x, yBar))) neg = xc - x / d;
        for (let x = Math.round((xc - half - 1) * d); x <= Math.round((xc + half + 1) * d); x++) if (isPeak(px(img, W, x, yPeak))) peak++;
        out.push({ pos: +pos.toFixed(1), neg: +neg.toFixed(1), half, peak });
      }
      return out;
    },
    top() {
      const c = document.getElementById('topCanvas');
      const ctx = c.getContext('2d'), d = dprOf(c), W = c.width, H = c.height;
      const img = read(c);
      const w = W / d, h = H / d, cx = w / 2, cy = h / 2 + 3, R = Math.min(w, h) / 2 - 14;
      let sx = 0, sy = 0, n = 0, tx = 0, ty = 0, tn = 0;
      for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
        const p = px(img, W, x, y);
        if (isBrass(p)) { sx += x; sy += y; n++; }
        const rr = Math.hypot(x / d - cx, y / d - cy);
        if (rr >= R + 3 && rr <= R + 10 && isSage(p)) { tx += x / d - cx; ty += y / d - cy; tn++; }
      }
      return { cx, cy, R, puck: n ? [+(sx / n / d).toFixed(2), +(sy / n / d).toFixed(2)] : null, puckN: n,
               tick: tn ? [+(tx / tn).toFixed(2), +(ty / tn).toFixed(2)] : null, tickN: tn };
    },
    // bright sage mask of a box (design px) on the top canvas — for the rose ▲ glyph
    glyphMask(bx, by, size) {
      const c = document.getElementById('topCanvas');
      const ctx = c.getContext('2d'), d = dprOf(c), W = c.width;
      const img = read(c);
      const m = [];
      for (let y = Math.round((by - size / 2) * d); y < Math.round((by + size / 2) * d); y++)
        for (let x = Math.round((bx - size / 2) * d); x < Math.round((bx + size / 2) * d); x++) {
          const [r, g, b] = px(img, W, x, y);
          m.push(g > 150 && g > r + 15 && g > b + 50 ? 1 : 0);
        }
      return m;
    },
    // angular extent (deg) of the positive-lobe fill at radius rBeam(g0)
    beamExtent(g0) {
      const c = document.getElementById('topCanvas');
      const ctx = c.getContext('2d'), d = dprOf(c), W = c.width;
      const img = read(c);
      const w = W / d, h = c.height / d, cx = w / 2, cy = h / 2 + 3, R = Math.min(w, h) / 2 - 14, rHead = 20;
      const r = rHead + g0 * (R - rHead) * 0.94 - 1.5;   // just inside the lobe outline at g = g0
      let n = 0;
      for (let a = 0; a < 360; a += 0.5) {
        const x = Math.round((cx - Math.sin(a * Math.PI / 180) * r) * d), y = Math.round((cy - Math.cos(a * Math.PI / 180) * r) * d);
        const [pr, pg, pb] = px(img, W, x, y);
        if (pg >= pr && pg > pb + 12) n++;
      }
      return n * 0.5;
    },
    // A14: the top canvas as a WARM mask (design-px coordinates). strict = the
    // full-alpha brass / amber inks (puck disc, ▲ glyph, chevron); relaxed adds
    // the half-alpha brass halo. The beam's amber lobe (≤ 0.5 alpha) and every
    // sage / parchment ink fall outside both.
    warmMask(strict) {
      const c = document.getElementById('topCanvas');
      const d = dprOf(c), W = c.width, H = c.height;
      const img = read(c);
      const out = [];
      for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
        const [r, g, b] = px(img, W, x, y);
        const hit = strict ? (r > 180 && r - b > 90 && g > 120 && r >= g)
                           : (r > 120 && r - b > 55 && r > g + 10 && g > 90);
        if (hit) out.push([x / d, y / d]);
      }
      return out;
    },
    canvasBytes(id) {
      const c = document.getElementById(id);
      const data = c.getContext('2d').getImageData(0, 0, c.width, c.height).data;
      let h = 0; for (let i = 0; i < data.length; i += 7) h = (h * 31 + data[i]) | 0;
      return h;
    },
    canvasData(id) { const c = document.getElementById(id); return Array.from(c.getContext('2d').getImageData(0, 0, c.width, c.height).data); },
    setParams(params) {
      const st = window.__stubStates;
      for (const [id, v] of Object.entries(params)) {
        if (st.sliders.has(id)) {
          const s = st.sliders.get(id), p = s.properties;
          const prop = Math.min(1, Math.max(0, (v - p.start) / (p.end - p.start)));
          if (Math.abs(s.getScaledValue() - v) > 1e-9) s.setNormalisedValue(Math.pow(prop, p.skew));
        } else if (st.combos.has(id)) {
          const s = st.combos.get(id); if (s.getChoiceIndex() !== v) s.setChoiceIndex(v);
        } else if (st.toggles.has(id)) {
          const s = st.toggles.get(id); if (!!s.getValue() !== !!v) s.setValue(!!v);
        }
      }
    },
  };
  return true;
};

// ── reporting ───────────────────────────────────────────────────────────────
const results = [];
function arm(name, req, checks) {
  const ok = checks.every((c) => c.ok);
  for (const c of checks) console.log(`    ${c.ok ? 'ok  ' : 'FAIL'} ${c.what} — ${c.got}`);
  let line = `${ok ? 'PASS' : 'FAIL'} ${name} (${req})`;
  if (CONTROL && name === CONTROL_TARGET[CONTROL]) line += ok ? '  — PASSES: negative control is VACUOUS' : '  — FAILS as designed';
  console.log(line);
  results.push({ name, ok });
}
const near = (a, b, tol) => Math.abs(a - b) <= tol;

// ── main ────────────────────────────────────────────────────────────────────
async function main() {
  const pw = S.resolvePlaywright();
  if (!pw) { console.error('frame-probe: Playwright unresolvable — NOTHING was verified (exit 77, never a pass).'); process.exit(77); }

  const built = S.buildRoot(NAME);
  if (CONTROL === 'pole') patchAnchor(built.root, 'probe-anchor:pole-hold', (l) => l.replace('if (!held) ', ''));
  if (CONTROL === 'idle') patchAnchor(built.root, 'probe-anchor:idle-fallback', (l) => l.replace('if (f && f.live === 1) return f;', 'if (f) return f;'));
  if (CONTROL === 'chevron') patchAnchor(built.root, 'probe-anchor:chevron-slide', (l) => l.replace('if (Math.abs(qx) < need) ax', 'if (false) ax'));

  const es = S.readEditorSize(NAME); const vw = es.w, vh = es.h;
  const srv = await S.serve(built.root);
  let browser;
  try { browser = await pw.chromium.launch(); }
  catch (e) {
    console.error(`frame-probe: Chromium did not launch — NOTHING was verified (exit 77): ${String(e.message).split('\n')[0]}`);
    await srv.close(); process.exit(77);
  }
  console.log(`frame-probe — ${NAME}  ui=${built.uiRootLabel}  stub=${built.stubKind}  seed=${built.seedFrom}  viewport=${vw}x${vh}@2${CONTROL ? `  CONTROL=${CONTROL}` : ''}`);

  const page = await browser.newPage({ viewport: { width: vw, height: vh }, deviceScaleFactor: 2 });
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e && e.message ? e.message : e)));
  if (VERBOSE) page.on('console', (m) => console.log(`  [console.${m.type()}] ${m.text()}`));
  await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
  await page.waitForFunction(() => window.__stubStates && document.querySelectorAll('.bar-cell').length === 16, null, { timeout: 10000 });
  await page.evaluate(() => document.fonts && document.fonts.ready);
  await page.evaluate(PAGE_HELPERS);

  const setup = await page.evaluate(() => ({ title: document.title, iw: window.innerWidth, ih: window.innerHeight }));
  const setupOk = /O-simpleAmbisonics/.test(setup.title) && built.seedFrom === 'overrides' && setup.iw === vw && setup.ih === vh;
  console.log(`  setup: title "${setup.title}", seed=${built.seedFrom}, innerWidth ${setup.iw}x${setup.ih} — ${setupOk ? 'ok' : 'WRONG PAGE / SEED / VIEWPORT'}`);
  if (!setupOk) { await browser.close(); await srv.close(); process.exit(6); }

  const settle = async (fn) => {
    let prev = JSON.stringify(await fn()), t0 = Date.now();
    for (;;) {
      await page.waitForTimeout(SETTLE_MS);
      const cur = JSON.stringify(await fn());
      if (cur === prev || Date.now() - t0 > SETTLE_MAX) return JSON.parse(cur);
      prev = cur;
    }
  };
  const payloadOf = (name) => {
    let p = FX[name].payload;
    if (CONTROL === 'fuma' && name === 'left_o1') {
      const o = JSON.parse(p); const t = o.shGain[1]; o.shGain[1] = o.shGain[3]; o.shGain[3] = t; p = JSON.stringify(o);
    }
    return p;
  };
  const show = async (name, extraParams) => {
    await page.evaluate((p) => window.__probe.setParams(p), { ...FX[name].params, ...(extraParams || {}) });
    await page.evaluate((p) => window.__stubEmit('meterFrame', p), payloadOf(name));
  };
  const bars = () => settle(() => page.evaluate(() => window.__probe.bars()));
  const top  = () => settle(() => page.evaluate(() => window.__probe.top()));
  const val  = (b, k) => (b[k].pos - b[k].neg) / b[k].half;   // signed bar value in units of the half-width
  const barIs = (b, k, v) => Math.abs((b[k].pos - b[k].neg) - v * b[k].half) <= BAR_TOL_PX;
  const fmtBar = (b, k) => `${val(b, k).toFixed(3)} (+${b[k].pos} / −${b[k].neg} px of ${b[k].half})`;
  const rOf1m = (t) => { const rIn = 26, rOut = t.R - 6; return rIn + (rOut - rIn) * Math.log(1 / 0.2) / Math.log(100); };

  // ── A1 ──
  {
    await show('front_o1'); const bf = await bars();
    await show('left_o1');  const bl = await bars();
    arm('A1', 'UI-01', [
      { what: 'front: W +1, X +1, Y 0', ok: barIs(bf, 0, 1) && barIs(bf, 3, 1) && barIs(bf, 1, 0), got: `W ${fmtBar(bf, 0)}, X ${fmtBar(bf, 3)}, Y ${fmtBar(bf, 1)}` },
      { what: 'left: W unchanged (±2 px), Y +1, X 0', ok: Math.abs(bl[0].pos - bf[0].pos) <= 2 && barIs(bl, 1, 1) && barIs(bl, 3, 0), got: `W ${fmtBar(bl, 0)}, Y ${fmtBar(bl, 1)}, X ${fmtBar(bl, 3)}` },
    ]);
  }
  // ── A2 ──
  {
    await show('behind_o1'); const bb = await bars();
    await show('overhead_o2'); const bo = await bars();
    arm('A2', 'UI-01', [
      { what: 'behind: X amber, length 1', ok: bb[3].neg > 0 && bb[3].pos === 0 && barIs(bb, 3, -1), got: `X ${fmtBar(bb, 3)}` },
      { what: 'overhead: Z +1, X 0, Y 0', ok: barIs(bo, 2, 1) && barIs(bo, 3, 0) && barIs(bo, 1, 0), got: `Z ${fmtBar(bo, 2)}, X ${fmtBar(bo, 3)}, Y ${fmtBar(bo, 1)}` },
    ]);
  }
  // ── A3 + A11 ──
  {
    const bands = [], counts = [], dims = [];
    for (const [name, want] of [['o1_az45', 4], ['o2_az45', 9], ['o3_az45', 16]]) {
      await show(name);
      const r = await settle(() => page.evaluate(() => ({
        on: document.querySelectorAll('#chanRows .band.on').length,
        offOpacity: Math.max(0, ...[...document.querySelectorAll('#chanRows .band:not(.on) .sh-icon')].map((e) => +getComputedStyle(e).opacity)),
        enc: document.getElementById('stEncoderSub').textContent, rot: document.getElementById('stRotationSub').textContent,
      })));
      bands.push(r.on); dims.push(r.offOpacity); counts.push([want, r.enc, r.rot]);
    }
    const icons = await page.evaluate(() => ({
      letters: [...document.querySelectorAll('#chanRows .ltr')].map((e) => e.textContent).join(' '),
      svgs: document.querySelectorAll('#chanRows .sh-icon svg').length,
      paths: [...document.querySelectorAll('#chanRows .sh-icon svg')].map((s) => s.querySelectorAll('path, polygon').length),
    }));
    arm('A3', 'UI-01', [
      { what: '.band.on count 2 / 3 / 4', ok: bands.join(',') === '2,3,4', got: bands.join(' / ') },
      { what: 'inactive rows dimmed (icon opacity < 0.5)', ok: dims[0] < 0.5 && dims[1] < 0.5, got: dims.slice(0, 2).join(' / ') },
      { what: '16 icons, ACN letters W Y Z X V T R S U Q O M K L N P', ok: icons.svgs === 16 && icons.paths.every((n) => n > 0) && icons.letters === 'W Y Z X V T R S U Q O M K L N P', got: `${icons.svgs} svgs, letters ${icons.letters}` },
    ]);
    // A11 station labels + highlight
    await page.evaluate(() => window.__probe.setParams({ yaw: 0 }));
    await page.waitForTimeout(1600);   // the 1.4 s station hold elapses
    const offBefore = await page.evaluate(() => document.getElementById('st-rotation').classList.contains('on'));
    await page.evaluate(() => window.__probe.setParams({ yaw: 30 }));
    const onAfter = await settle(() => page.evaluate(() => document.getElementById('st-rotation').classList.contains('on')));
    await page.evaluate(() => window.__probe.setParams({ yaw: 0 }));
    const countsOk = counts.every(([n, enc, rot]) => new RegExp(`(^|\\D)${n}(\\D|$)`).test(enc) && new RegExp(`(^|\\D)${n}(\\D|$)`).test(rot));
    arm('A11', 'UI-04', [
      { what: 'stub yaw change lights #st-rotation', ok: !offBefore && onAfter, got: `before ${offBefore}, after ${onAfter}` },
      { what: 'channel-count labels 4 / 9 / 16', ok: countsOk, got: counts.map(([n, e, r]) => `${n}: "${e}" "${r}"`).join('; ') },
    ]);
  }
  // ── A4 + A5 ──
  {
    await show('front_o1'); const tf = await top();
    const maskFrontTop = await page.evaluate(([x, y]) => window.__probe.glyphMask(x, y, 14), [tf.cx, tf.cy - (tf.R - 22)]);
    const maskFrontLeft = await page.evaluate(([x, y]) => window.__probe.glyphMask(x, y, 14), [tf.cx - (tf.R - 22), tf.cy]);
    await show('left_o1'); const tl = await top();
    const r1 = rOf1m(tl);
    arm('A4', 'UI-02', [
      { what: `left_o1 puck left of the head at r(1 m) = ${r1.toFixed(1)} ± ${PUCK_TOL_PX} px`, ok: !!tl.puck && near(tl.puck[0], tl.cx - r1, PUCK_TOL_PX) && near(tl.puck[1], tl.cy, PUCK_TOL_PX),
        got: `centroid ${JSON.stringify(tl.puck)} (${tl.puckN} px), head (${tl.cx}, ${tl.cy})` },
    ]);
    await show('yaw90_front');
    const ty = await settle(() => page.evaluate(() => { const t = window.__probe.top(); return t; }));
    await page.waitForTimeout(400);   // the compass ease converges (snaps within 1e-3)
    const ty2 = await top();
    const maskYawLeft = await page.evaluate(([x, y]) => window.__probe.glyphMask(x, y, 14), [ty2.cx - (ty2.R - 22), ty2.cy]);
    const iou = (a, b) => { let i = 0, u = 0; for (let k = 0; k < a.length; k++) { i += a[k] & b[k]; u += a[k] | b[k]; } return u ? i / u : 0; };
    const glyphMatch = iou(maskYawLeft, maskFrontTop), glyphControl = iou(maskFrontLeft, maskFrontTop);
    arm('A5', 'UI-02', [
      { what: 'yaw 90: puck unchanged vs front_o1 (± 1 px)', ok: !!ty2.puck && !!tf.puck && near(ty2.puck[0], tf.puck[0], SAME_TOL_PX) && near(ty2.puck[1], tf.puck[1], SAME_TOL_PX), got: `${JSON.stringify(ty2.puck)} vs ${JSON.stringify(tf.puck)}` },
      { what: 'rim tick moved to the left', ok: !!ty2.tick && ty2.tick[0] < -(ty2.R - 5) && Math.abs(ty2.tick[1]) < 3 && !!tf.tick && tf.tick[1] < -(tf.R - 5), got: `front ${JSON.stringify(tf.tick)} → yaw90 ${JSON.stringify(ty2.tick)}` },
      { what: 'rose ▲ at screen-left (glyph mask IoU vs the front ▲ > 0.6; the +90° text there at yaw 0 < 0.4)', ok: glyphMatch > 0.6 && glyphControl < 0.4, got: `IoU ${glyphMatch.toFixed(2)} (control ${glyphControl.toFixed(2)})` },
    ]);
    void ty;
    // ── A9 pole hold ──
    await show('pole_a'); const pa = await top();
    await show('pole_b'); const pb = await top();
    const tickOk = (t) => !!t.tick && near(t.tick[0], ty2.tick[0], SAME_TOL_PX) && near(t.tick[1], ty2.tick[1], SAME_TOL_PX);
    arm('A9', '§Q6 pole hold', [
      { what: 'yaw90_front → pole_a → pole_b: rim tick stays at +90 (± 1 px)', ok: tickOk(pa) && tickOk(pb), got: `${JSON.stringify(ty2.tick)} → ${JSON.stringify(pa.tick)} → ${JSON.stringify(pb.tick)}` },
    ]);
  }
  // ── A6 / A7 drags ──
  {
    const instrument = (id) => page.evaluate((k) => {
      const s = window.__stubStates.sliders.get(k);
      const log = { starts: 0, ends: 0, values: [] };
      if (!s.__probeWrapped) {
        const s0 = s.sliderDragStarted.bind(s), e0 = s.sliderDragEnded.bind(s);
        s.sliderDragStarted = () => { window.__dragLog[k].starts++; s0(); };
        s.sliderDragEnded   = () => { window.__dragLog[k].ends++; e0(); };
        s.valueChangedEvent.addListener(() => window.__dragLog[k].values.push(s.getScaledValue()));
        s.__probeWrapped = true;
      }
      window.__dragLog = window.__dragLog || {};
      window.__dragLog[k] = log;
    }, id);
    await show('front_o1');
    await instrument('azimuth');
    const g = await page.evaluate(() => { const r = document.getElementById('topCanvas').getBoundingClientRect(); return { x: r.left, y: r.top, w: r.width, h: r.height }; });
    const cx = g.x + g.w / 2, cy = g.y + g.h / 2 + 3, rr = 80;
    const at = (deg) => [cx - Math.sin(deg * Math.PI / 180) * rr, cy - Math.cos(deg * Math.PI / 180) * rr];   // AmbiX az → screen
    let p = at(-90); await page.mouse.move(p[0], p[1]); await page.mouse.down();
    for (let a = -95; a >= -270; a -= 5) { p = at(a); await page.mouse.move(p[0], p[1]); }
    await page.mouse.up();
    const la = await page.evaluate(() => window.__dragLog.azimuth);
    const rear = la.values.filter((v) => Math.abs(v) < 90);
    const crossed = la.values.some((v) => v < -170) && la.values.some((v) => v > 170);
    const final = la.values[la.values.length - 1];
    arm('A6', 'UI-02', [
      { what: 'one gesture: starts == ends == 1', ok: la.starts === 1 && la.ends === 1, got: `${la.starts} / ${la.ends}` },
      { what: 'crosses ±180 with no |az| < 90 sample in the rear crossing', ok: crossed && rear.length === 0, got: `${la.values.length} samples, crossed ${crossed}, |az|<90: ${rear.length}` },
      { what: 'final +90 ± 1', ok: near(final, 90, 1), got: `${final.toFixed(2)}` },
    ]);
    await instrument('elevation');
    const gs = await page.evaluate(() => { const r = document.getElementById('sideCanvas').getBoundingClientRect(); return { x: r.left, y: r.top, w: r.width, h: r.height }; });
    const scx = gs.x + 24, scy = gs.y + gs.h / 2 + 3, sr = 60;
    const sat = (deg) => [scx + Math.cos(deg * Math.PI / 180) * sr, scy - Math.sin(deg * Math.PI / 180) * sr];
    let q = sat(0); await page.mouse.move(q[0], q[1]); await page.mouse.down();
    for (let a = 5; a <= 45; a += 5) { q = sat(a); await page.mouse.move(q[0], q[1]); }
    await page.mouse.up();
    const le = await page.evaluate(() => window.__dragLog.elevation);
    const ef = le.values[le.values.length - 1];
    const rising = le.values.every((v, i) => i === 0 || v >= le.values[i - 1] - 1e-9);
    arm('A7', 'UI-02', [
      { what: 'one gesture: starts == ends == 1', ok: le.starts === 1 && le.ends === 1, got: `${le.starts} / ${le.ends}` },
      { what: 'elevation rises (up = +) to the pointer angle 45 ± 1', ok: rising && near(ef, 45, 1), got: `${le.values.length} samples, final ${ef?.toFixed(2)}` },
    ]);
    await page.evaluate(() => window.__probe.setParams({ azimuth: 0, elevation: 0 }));
  }
  // ── A8 beam ──
  {
    const ext = [];
    for (const name of ['o1_az45', 'o2_az45', 'o3_az45']) {
      await show(name);
      ext.push(await settle(() => page.evaluate(() => window.__probe.beamExtent(0.707))));
    }
    arm('A8', 'UI-03', [
      { what: 'beam extent at r = rBeam(0.707) strictly decreases o1 → o2 → o3', ok: ext[0] > ext[1] && ext[1] > ext[2] && ext[2] > 0, got: ext.map((e) => `${e}°`).join(' → ') },
    ]);
  }
  // ── A10 decoder view ──
  {
    await show('o3_az45');
    await settle(() => page.evaluate(() => window.__probe.canvasBytes('decCanvas')));
    const a = await page.evaluate(() => window.__probe.canvasData('decCanvas'));
    await show('stereo_o3');
    await settle(() => page.evaluate(() => window.__probe.canvasBytes('decCanvas')));
    const b = await page.evaluate(() => window.__probe.canvasData('decCanvas'));
    let diff = 0; for (let i = 0; i < a.length; i += 4) if (Math.abs(a[i] - b[i]) + Math.abs(a[i + 1] - b[i + 1]) + Math.abs(a[i + 2] - b[i + 2]) > 60) diff++;
    const frac = diff / (a.length / 4);
    arm('A10', 'UI-05', [
      { what: 'Binaural → Stereo (decoderMix 1 → 0): > 2 % of #decCanvas pixels change', ok: frac > 0.02, got: `${(frac * 100).toFixed(2)} %` },
    ]);
  }
  // ── A12 idle before the first block ──
  {
    await show('idle_before_first_block');
    const b = await bars(); const t = await top();
    const op = await settle(() => page.evaluate(() => { const w = document.getElementById('chanWell'); return { idle: w.classList.contains('idle'), op: +getComputedStyle(w).opacity }; }));
    const r1 = rOf1m(t);
    await show('left_o1');
    const op2 = await settle(() => page.evaluate(() => { const w = document.getElementById('chanWell'); return { idle: w.classList.contains('idle'), op: +getComputedStyle(w).opacity }; }));
    arm('A12', 'U1', [
      { what: 'idle frame + stub az 90: W +1, Y +1, X 0 from the parameters', ok: barIs(b, 0, 1) && barIs(b, 1, 1) && barIs(b, 3, 0), got: `W ${fmtBar(b, 0)}, Y ${fmtBar(b, 1)}, X ${fmtBar(b, 3)}` },
      { what: `puck left at r(1 m) ± ${PUCK_TOL_PX} px`, ok: !!t.puck && near(t.puck[0], t.cx - r1, PUCK_TOL_PX) && near(t.puck[1], t.cy, PUCK_TOL_PX), got: JSON.stringify(t.puck) },
      { what: '#chanWell.idle, opacity ≤ 0.9; then left_o1 (live) removes it', ok: op.idle && op.op <= 0.9 && !op2.idle && op2.op > 0.99, got: `idle ${op.idle} ${op.op} → ${op2.idle} ${op2.op}` },
    ]);
  }
  // ── A13 stale after a publish ──
  {
    await show('left_o1');
    const b0 = await bars(); const t0 = await top();
    const peaks0 = b0.reduce((s, x) => s + x.peak, 0);
    await show('idle_stale_after_publish');
    const tStart = Date.now(); let peaksNow = peaks0, tZero = -1;
    while (Date.now() - tStart < 1500) {
      peaksNow = (await page.evaluate(() => window.__probe.bars())).reduce((s, x) => s + x.peak, 0);
      if (peaksNow === 0) { tZero = Date.now() - tStart; break; }
      await page.waitForTimeout(30);
    }
    const b1 = await bars(); const t1 = await top();
    const idle1 = await page.evaluate(() => document.getElementById('chanWell').classList.contains('idle'));
    await page.evaluate(() => window.__probe.setParams({ azimuth: -90 }));
    const b2 = await bars(); const t2 = await top();
    // live again: the same stub change must NOT move the puck
    await page.evaluate(() => window.__probe.setParams({ azimuth: 90 }));
    await page.evaluate((p) => window.__stubEmit('meterFrame', p), payloadOf('left_o1'));
    const t3 = await top();
    await page.evaluate(() => window.__probe.setParams({ azimuth: -90 }));
    const t4 = await top();
    await show('silent_after_play');
    const b5 = await bars();
    const well5 = await settle(() => page.evaluate(() => document.getElementById('chanWell').classList.contains('idle')));
    const peaks5 = b5.reduce((s, x) => s + x.peak, 0);
    arm('A13', 'U1', [
      { what: 'left_o1 shows peaks, then the stale frame decays them to 0 within 1 s', ok: peaks0 > 0 && tZero >= 0 && tZero <= 1000, got: `${peaks0} peak px → 0 after ${tZero} ms` },
      { what: 'stale: bars stay Y +1, puck ± 1 px, idle set', ok: barIs(b1, 1, 1) && !!t1.puck && near(t1.puck[0], t0.puck[0], SAME_TOL_PX) && near(t1.puck[1], t0.puck[1], SAME_TOL_PX) && idle1, got: `Y ${fmtBar(b1, 1)}, puck ${JSON.stringify(t1.puck)} vs ${JSON.stringify(t0.puck)}, idle ${idle1}` },
      { what: 'idle: stub az −90 moves the puck right of the head and Y turns amber', ok: !!t2.puck && t2.puck[0] > t2.cx + 20 && b2[1].neg > 0 && barIs(b2, 1, -1), got: `puck ${JSON.stringify(t2.puck)}, Y ${fmtBar(b2, 1)}` },
      { what: 'live: the same stub change does NOT move the puck', ok: !!t3.puck && !!t4.puck && near(t3.puck[0], t4.puck[0], SAME_TOL_PX) && t4.puck[0] < t4.cx - 20, got: `${JSON.stringify(t3.puck)} → ${JSON.stringify(t4.puck)}` },
      { what: 'silent_after_play: peaks 0 and the well NOT dimmed', ok: peaks5 === 0 && well5 === false, got: `${peaks5} peak px, idle ${well5}` },
    ]);
  }

  // ── A14 chevron / puck separation (Stage 4 PLAN T5, P13) ──
  // Idle frame, so the puck follows the PARAMETERS. az = yaw = 20 puts the puck
  // and the front-elevation chevron on one ray; d 4.4 m puts the puck on the
  // chevron's R − 40 radius; el 30 grows the altitude halo and the ▲ glyph.
  // Render A (pitch 0: no chevron) gives the puck / halo / ▲ mask; render B
  // (pitch 40) adds the chevron: B's strict warm pixels that A does not have.
  {
    const scene = { azimuth: 20, yaw: 20, elevation: 30, distance: 4.4, roll: 0, order: 2, decoder: 0 };
    await show('idle_before_first_block', { ...scene, pitch: 0 });
    await page.waitForTimeout(600);   // the compass ease converges
    await top();
    const A = await page.evaluate(() => ({ strict: window.__probe.warmMask(true), relaxed: window.__probe.warmMask(false), t: window.__probe.top() }));
    await page.evaluate(() => window.__probe.setParams({ pitch: 40 }));
    await page.waitForTimeout(600);
    await top();
    const B = await page.evaluate(() => window.__probe.warmMask(true));
    const key = (p) => `${Math.round(p[0] * 2)},${Math.round(p[1] * 2)}`;
    const inA = new Set(A.relaxed.map(key));
    const puck = A.t.puck;
    // Chevron pixels: new in B, and within 60 px of the puck (the chevron's
    // whole neighbourhood; nothing else warm changes with pitch there).
    const chev = B.filter((p) => !inA.has(key(p)) && puck && Math.hypot(p[0] - puck[0], p[1] - puck[1]) < 60);
    let gap = Infinity;
    for (const c of chev) for (const a of A.relaxed) {
      if (puck && Math.hypot(a[0] - puck[0], a[1] - puck[1]) > 40) continue;   // the puck cluster only
      const dd = Math.hypot(c[0] - a[0], c[1] - a[1]);
      if (dd < gap) gap = dd;
    }
    arm('A14', 'P13 chevron', [
      { what: 'chevron drawn: ≥ 12 new full-alpha amber px at pitch 40 near the puck', ok: chev.length >= 12, got: `${chev.length} px (puck ${JSON.stringify(puck)})` },
      { what: 'gap between the chevron and the puck / halo / ▲ mask ≥ 4 px', ok: chev.length > 0 && gap >= 4, got: `${Number.isFinite(gap) ? gap.toFixed(2) : '—'} px` },
    ]);
    await page.evaluate(() => window.__probe.setParams({ azimuth: 0, yaw: 0, elevation: 0, distance: 1, pitch: 0 }));
  }

  console.log(errors.length ? `  page errors (${errors.length}): ${errors.slice(0, 3).join(' | ')}` : '  page errors: 0');
  await browser.close(); await srv.close();
  fs.rmSync(built.root, { recursive: true, force: true });

  const failed = results.filter((r) => !r.ok).length + (errors.length ? 1 : 0);
  if (CONTROL) {
    const target = results.find((r) => r.name === CONTROL_TARGET[CONTROL]);
    const vacuous = !target || target.ok;
    console.log(`control ${CONTROL}: target ${CONTROL_TARGET[CONTROL]} ${vacuous ? 'PASSED — VACUOUS' : 'FAILS as designed'}`);
    process.exit(vacuous ? 1 : 0);
  }
  console.log(`${results.length - results.filter((r) => !r.ok).length} / ${results.length} arms PASS${errors.length ? ' (+ page errors)' : ''}`);
  process.exit(failed);
}

main().catch((e) => { console.error(e); process.exit(6); });
