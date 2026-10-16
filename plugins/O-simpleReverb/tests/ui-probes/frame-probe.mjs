#!/usr/bin/env node
/*
   This file is part of O-simpleReverb, an Ouaricon Audio plugin.
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
// frame-probe — the live decay well and the output meter of the O-simpleReverb
// page (family-retrofit PLAN T8), ported from O-simpleAmbisonics' frame-probe.
//
// Serves the REAL page tree (scripts/serve-ui.js buildRoot, generic stub,
// tests/ui-stub/generic-overrides.json — the banner must read seed=overrides)
// and feeds it the EXACT payload strings the editor emits for real processor
// states (tests/ui-fixtures/frames.json, written by `render-check
// --write-frames` through MeterFramePayload.h), at the fixture's own frame
// rate, in-page, so the wall-clock scroll sees real spacing. Stub parameter
// states are set to each fixture's params. Verdicts are read from CANVAS PIXELS,
// polled until two consecutive reads agree — never a blind sleep.
//
//   F1  hall_burst live: the wet tail (sage), the input envelope (amber) and the
//       ANCHORED brass RT60 slope are drawn; the held last value reaches the
//       right edge; the picture scrolls while live (two reads differ)
//   F2  the brass slope's width = (anchor dB + 60) / 60 x baseT60 x DECAY over
//       the 6 s window, for Hall and for Booth (anchor recomputed here from the
//       payloads by an independent copy of the rule), ± 5 px
//   F3  live -> idle freezes the picture: a DECAY move after the idle frame
//       redraws the slope at half width (DECAY 1.0 -> 0.5) while the sage
//       trace's right edge stays put (± 1 px)
//   F4  clip_sine (outPeak 2.19): both clip lamps lit, both bars full; the idle
//       frame empties both bars, and the lamps clear after the 1.2 s hold
//
// Every arm runs at 0.6x / 1.0x / 1.5x of the editor size, in en / fr / zh-Hans
// (the page scales itself by a CSS transform; canvases fold uiScale into their
// backing store, so the probe reads in DESIGN px through c.width / clientWidth).
//
// Usage:
//   node plugins/O-simpleReverb/tests/ui-probes/frame-probe.mjs
//        [--negative-control freeze|slope] [--scale 1.0] [--lang en] [--verbose]
//
//   --negative-control freeze  the page's live -> idle `frozenAt = now;` becomes
//                              `frozenAt = 0;`: F3 must FAIL
//   --negative-control slope   the slope's `t60 * 1000` becomes `t60 * 500`:
//                              F2 must FAIL
//   A control whose patch matches zero or more than one line exits 6.
//
// Exit: normal run — 0 = every arm PASS, n = failed arms. Control run — 0 = the
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
const NAME    = 'O-simpleReverb';

const argv = process.argv.slice(2);
const argVal = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const CONTROL = argVal('--negative-control');
const VERBOSE = argv.includes('--verbose');
const CONTROL_TARGET = { freeze: 'F3', slope: 'F2' };
if (CONTROL && !CONTROL_TARGET[CONTROL]) { console.error(`unknown --negative-control ${CONTROL}`); process.exit(6); }
const SCALES = argVal('--scale') ? [Number(argVal('--scale'))] : [0.6, 1.0, 1.5];
const LANGS  = argVal('--lang') ? [argVal('--lang')] : ['en', 'fr', 'zh-Hans'];

const FIXTURES = JSON.parse(fs.readFileSync(path.join(PLUGIN, 'tests', 'ui-fixtures', 'frames.json'), 'utf8'));
const FX = Object.fromEntries(FIXTURES.map((f) => [f.name, f]));
const OVR = JSON.parse(fs.readFileSync(path.join(PLUGIN, 'tests', 'ui-stub', 'generic-overrides.json'), 'utf8'));
const MODEL = OVR.natives.getReverbModel.types;

// ── tolerances (probe constants, not page constants) ────────────────────────
const SLOPE_TOL_PX = 5;
const SAME_TOL_PX  = 1;
const SETTLE_MS    = 60;
const SETTLE_MAX   = 4000;
// The page's own law, restated independently (index.html drawDecay / trackBurst).
const WINDOW_MS = 6000, FLOOR_DB = -60, ON_DB = -50, OFF_DB = -60, SEARCH_MS = 160;

// The anchor the page should hang the slope from, recomputed from the payloads
// with frame index x frameMs as the clock.
function expectedAnchorDb(fx) {
  const P = fx.payloads.map((s) => JSON.parse(s));
  const pre = MODEL[fx.params.TYPE].preDelayMs;
  let on = false, endT = 0, searching = false, best = null, anchor = null;
  P.forEach((p, i) => {
    if (p.live !== 1) return;
    const t = i * fx.frameMs;
    if (p.inDb > ON_DB) { on = true; searching = false; best = null; return; }
    if (on && p.inDb < OFF_DB) { on = false; endT = t; searching = true; best = null; }
    if (!searching) return;
    if (t <= endT + pre + SEARCH_MS) { if (!best || p.wetDb > best) best = p.wetDb; }
    else { searching = false; if (best !== null && best > FLOOR_DB + 6) anchor = best; }
  });
  return anchor;
}
const slopeWidth = (anchorDb, t60, plotW) => (Math.min(0, anchorDb) - FLOOR_DB) / 60 * t60 * 1000 / WINDOW_MS * plotW;

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
  const hex = (v) => { const m = v.trim().match(/^#([0-9a-f]{6})$/i); if (!m) return null; const n = parseInt(m[1], 16); return [(n >> 16) & 255, (n >> 8) & 255, n & 255]; };
  const rs = getComputedStyle(document.documentElement);
  const WT = hex(rs.getPropertyValue('--well-top')) || [42, 33, 24];
  const WB = hex(rs.getPropertyValue('--well-bot')) || [28, 22, 15];
  // The canvases are transparent: composite every read over the well's own gradient.
  const grab = (id) => {
    const c = document.getElementById(id);
    const img = c.getContext('2d').getImageData(0, 0, c.width, c.height);
    const d = c.width / c.clientWidth;   // backing px per DESIGN px (dpr x uiScale)
    const H = c.height;
    const px = (x, y) => {
      const i = (y * c.width + x) * 4, a = img.data[i + 3] / 255, t = y / Math.max(1, H - 1);
      return [0, 1, 2].map((k) => img.data[i + k] * a + (WT[k] + (WB[k] - WT[k]) * t) * (1 - a));
    };
    return { c, d, px, W: c.width, H, w: c.clientWidth, h: c.clientHeight };
  };
  // Inks: sage #9ec46f, amber #e8b04a, brass #f3cd86, rust #e98a5a — mutually exclusive.
  const isSage  = ([r, g, b]) => g > r + 8 && g > b + 40 && g > 120;
  const isAmber = ([r, g, b]) => r > 200 && r > g + 30 && g > 130 && b < 100;
  const isBrass = ([r, g, b]) => r > 215 && g > 185 && b > 110 && b < 165;
  const isRust  = ([r, g, b]) => r > 200 && g > 110 && g < 160 && b > 60 && b < 120;
  const isFill  = (p) => Math.max(...p) > 110;   // any lit meter gradient stop (green .. amber .. rust); the empty track and grid stay < 90

  window.__probe = {
    decay() {
      const { d, px, W, H, w, h } = grab('decayCanvas');
      const L = 34, R = w - 10, top = 58, bot = h - 22;
      let sage = 0, amber = 0, brass = 0, sageRight = -1, bMin = Infinity, bMax = -Infinity;
      for (let y = Math.round((top - 3) * d); y <= Math.min(H - 1, Math.round((bot + 3) * d)); y++)
        for (let x = Math.round(L * d); x <= Math.min(W - 1, Math.round(R * d)); x++) {
          const p = px(x, y);
          if (isSage(p))  { sage++; sageRight = Math.max(sageRight, x / d); }
          if (isAmber(p)) amber++;
          if (isBrass(p)) { brass++; bMin = Math.min(bMin, x / d); bMax = Math.max(bMax, x / d); }
        }
      const n = d * d;   // counts in DESIGN px²
      return { L, R, plotW: R - L, sage: Math.round(sage / n), amber: Math.round(amber / n), brass: Math.round(brass / n),
               sageRight: +sageRight.toFixed(2), brassW: brass ? +(bMax - bMin).toFixed(2) : 0,
               brassL: brass ? +bMin.toFixed(2) : null, brassR: brass ? +bMax.toFixed(2) : null };
    },
    out() {
      const { d, px, w, h } = grab('outCanvas');
      const top = 8, bot = h - 16, left = 22, bw = Math.floor((w - left - 6 - 8) / 2);
      return [0, 1].map((i) => {
        const x = left + 2 + i * (bw + 6), xc = Math.round((x + bw / 2) * d);
        const lamp = isRust(px(xc, Math.round(3 * d)));
        let barTop = null;
        for (let y = Math.round((top + 1) * d); y <= Math.round((bot - 1) * d); y++) if (isFill(px(xc, y))) { barTop = y / d; break; }
        return { lamp, barTop: barTop === null ? null : +barTop.toFixed(1), top, bot };
      });
    },
    // One frame, then two animation frames (the meter falls between frames): read at once.
    emitAndReadOut(payload) {
      window.__stubEmit('meterFrame', payload);
      return new Promise((res) => requestAnimationFrame(() => requestAnimationFrame(() => res(window.__probe.out()))));
    },
    hash(id) {
      const c = document.getElementById(id);
      const data = c.getContext('2d').getImageData(0, 0, c.width, c.height).data;
      let hh = 0; for (let i = 0; i < data.length; i += 7) hh = (hh * 31 + data[i]) | 0;
      return hh;
    },
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
    // The payloads at the fixture's own frame rate, in-page (wall-clock spacing).
    feed(payloads, ms) {
      return new Promise((res) => {
        let i = 0;
        const id = setInterval(() => {
          window.__stubEmit('meterFrame', payloads[i++]);
          if (i >= payloads.length) { clearInterval(id); res(); }
        }, ms);
      });
    },
  };
  return true;
};

// ── reporting ───────────────────────────────────────────────────────────────
const results = [];
function arm(name, tag, checks) {
  const ok = checks.every((c) => c.ok);
  if (VERBOSE || !ok) for (const c of checks) console.log(`      ${c.ok ? 'ok  ' : 'FAIL'} ${c.what} — ${c.got}`);
  let line = `  ${ok ? 'PASS' : 'FAIL'} ${name} [${tag}]`;
  if (CONTROL && name === CONTROL_TARGET[CONTROL]) line += ok ? '  — PASSES: negative control is VACUOUS' : '  — FAILS as designed';
  console.log(line);
  results.push({ name, tag, ok });
}
const near = (a, b, tol) => Math.abs(a - b) <= tol;

// ── main ────────────────────────────────────────────────────────────────────
async function main() {
  const pw = S.resolvePlaywright();
  if (!pw) { console.error('frame-probe: Playwright unresolvable — NOTHING was verified (exit 77, never a pass).'); process.exit(77); }

  const built = S.buildRoot(NAME);
  if (CONTROL === 'freeze') patchAnchor(built.root, 'frozenAt = now;', (l) => l.replace('frozenAt = now;', 'frozenAt = 0;'));
  if (CONTROL === 'slope')  patchAnchor(built.root, '/ 60 * t60 * 1000;', (l) => l.replace('t60 * 1000', 't60 * 500'));

  const es = S.readEditorSize(NAME);
  const srv = await S.serve(built.root);
  let browser;
  try { browser = await pw.chromium.launch(); }
  catch (e) {
    console.error(`frame-probe: Chromium did not launch — NOTHING was verified (exit 77): ${String(e.message).split('\n')[0]}`);
    await srv.close(); process.exit(77);
  }
  console.log(`frame-probe — ${NAME}  ui=${built.uiRootLabel}  stub=${built.stubKind}  seed=${built.seedFrom}  editor=${es.w}x${es.h}${CONTROL ? `  CONTROL=${CONTROL}` : ''}`);
  if (!/overrides/.test(built.seedFrom)) { console.error('frame-probe: stub not seeded from generic-overrides.json (exit 6)'); process.exit(6); }

  const hallAnchor = expectedAnchorDb(FX.hall_burst), boothAnchor = expectedAnchorDb(FX.booth_burst);
  console.log(`  expected anchors (from the payloads): hall ${hallAnchor} dB, booth ${boothAnchor} dB`);
  const errors = [];

  for (const scale of SCALES) for (const lang of LANGS) {
    const vw = Math.round(es.w * scale), vh = Math.round(es.h * scale);
    const tag = `${scale.toFixed(1)}x ${lang}`;
    console.log(`\n== ${tag} (viewport ${vw}x${vh}@2)`);
    const page = await browser.newPage({ viewport: { width: vw, height: vh }, deviceScaleFactor: 2 });
    page.on('pageerror', (e) => errors.push(`[${tag}] ${String(e && e.message ? e.message : e)}`));
    if (VERBOSE) page.on('console', (m) => console.log(`  [console.${m.type()}] ${m.text()}`));
    await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
    await page.waitForFunction(() => window.__stubStates && typeof window.__setLanguage === 'function', null, { timeout: 10000 });
    await page.evaluate(() => document.fonts && document.fonts.ready);
    await page.evaluate(PAGE_HELPERS);
    await page.evaluate((l) => window.__setLanguage(l), lang);

    const setup = await page.evaluate(() => ({ title: document.title, iw: innerWidth, ih: innerHeight, tf: document.getElementById('plugin').style.transform || 'none' }));
    const setupOk = /O-simpleReverb/.test(setup.title) && setup.iw === vw && setup.ih === vh && (scale === 1 ? setup.tf === 'none' : setup.tf === `scale(${scale})`);
    console.log(`  setup: "${setup.title}", inner ${setup.iw}x${setup.ih}, #plugin transform ${setup.tf} — ${setupOk ? 'ok' : 'WRONG PAGE / VIEWPORT / SCALE'}`);
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
    const feed = async (name) => {
      await page.evaluate((p) => window.__probe.setParams(p), FX[name].params);
      await page.evaluate(([p, ms]) => window.__probe.feed(p, ms), [FX[name].payloads, FX[name].frameMs]);
    };
    const idle = () => page.evaluate((p) => window.__stubEmit('meterFrame', p), FX.idle_edge.payloads[FX.idle_edge.payloads.length - 1]);
    const decay = () => page.evaluate(() => window.__probe.decay());

    // ── F1 + F2 (hall) ──
    await feed('hall_burst');
    const d1 = await decay();
    const h1 = await page.evaluate(() => window.__probe.hash('decayCanvas'));
    await page.waitForTimeout(250);
    const h2 = await page.evaluate(() => window.__probe.hash('decayCanvas'));
    arm('F1', tag, [
      { what: 'sage wet tail, amber input, anchored brass slope drawn', ok: d1.sage >= 60 && d1.amber >= 8 && d1.brass >= 20, got: `sage ${d1.sage}, amber ${d1.amber}, brass ${d1.brass} px²` },
      { what: 'the held last value reaches the right edge (± 3 px)', ok: near(d1.sageRight, d1.R, 3), got: `sage right ${d1.sageRight} vs R ${d1.R}` },
      { what: 'live: the picture scrolls (two reads 250 ms apart differ)', ok: h1 !== h2, got: `${h1} vs ${h2}` },
    ]);
    const wantHall = hallAnchor === null ? NaN : slopeWidth(hallAnchor, MODEL[2].baseT60 * 1.0, d1.plotW);

    // ── F3: live -> idle freezes; a DECAY move redraws the slope only ──
    await idle();
    const f0 = await settle(decay);
    await page.waitForTimeout(400);
    await page.evaluate(() => window.__probe.setParams({ DECAY: 0.5 }));
    const f1 = await settle(decay);
    const ratio = f0.brassW > 0 ? f1.brassW / f0.brassW : 0;
    arm('F3', tag, [
      { what: 'DECAY 1.0 -> 0.5 after the idle frame: slope width x 0.5 (± 0.08)', ok: near(ratio, 0.5, 0.08), got: `${f0.brassW} -> ${f1.brassW} px (x ${ratio.toFixed(3)})` },
      { what: 'frozen: the sage trace right edge stays put (± 1 px)', ok: f0.sageRight > 0 && near(f1.sageRight, f0.sageRight, SAME_TOL_PX), got: `${f0.sageRight} -> ${f1.sageRight}` },
    ]);

    // ── F2 booth (and the hall reading above) ──
    await page.evaluate(() => window.__probe.setParams({ DECAY: 1.0 }));
    await feed('booth_burst');
    const d2 = await decay();
    const wantBooth = boothAnchor === null ? NaN : slopeWidth(boothAnchor, MODEL[0].baseT60 * 1.0, d2.plotW);
    // The brass extent includes the 2.6 px anchor dot left of the anchor.
    arm('F2', tag, [
      { what: `Hall slope width = (${hallAnchor} + 60) / 60 x ${MODEL[2].baseT60} s over 6 s (± ${SLOPE_TOL_PX} px)`, ok: near(d1.brassW, wantHall, SLOPE_TOL_PX), got: `${d1.brassW} px vs ${wantHall.toFixed(1)}` },
      { what: `Booth slope width = (${boothAnchor} + 60) / 60 x ${MODEL[0].baseT60} s (± ${SLOPE_TOL_PX} px)`, ok: near(d2.brassW, wantBooth, SLOPE_TOL_PX), got: `${d2.brassW} px vs ${wantBooth.toFixed(1)}` },
    ]);

    // ── F4: the output meter ──
    await feed('clip_sine');
    const o1 = await page.evaluate((p) => window.__probe.emitAndReadOut(p), FX.clip_sine.payloads[FX.clip_sine.payloads.length - 1]);
    await idle();
    const o2 = await settle(() => page.evaluate(() => window.__probe.out().map((c) => c.barTop)));
    await page.waitForTimeout(1400);
    const o3 = await settle(() => page.evaluate(() => window.__probe.out()));
    arm('F4', tag, [
      { what: 'clip_sine: both clip lamps lit, both bars full (top within 2 px of the 0 dB line)', ok: o1.every((c) => c.lamp && c.barTop !== null && c.barTop <= c.top + 2), got: JSON.stringify(o1) },
      { what: 'idle frame: both bars empty', ok: o2.every((t) => t === null), got: JSON.stringify(o2) },
      { what: 'after the 1.2 s hold: both lamps clear', ok: o3.every((c) => !c.lamp), got: JSON.stringify(o3.map((c) => c.lamp)) },
    ]);

    await page.close();
  }

  console.log(errors.length ? `\n  page errors (${errors.length}): ${errors.slice(0, 3).join(' | ')}` : '\n  page errors: 0');
  await browser.close(); await srv.close();
  fs.rmSync(built.root, { recursive: true, force: true });

  const failed = results.filter((r) => !r.ok).length + (errors.length ? 1 : 0);
  if (CONTROL) {
    const targets = results.filter((r) => r.name === CONTROL_TARGET[CONTROL]);
    const vacuous = targets.length === 0 || targets.some((r) => r.ok);
    console.log(`control ${CONTROL}: target ${CONTROL_TARGET[CONTROL]} ${vacuous ? 'PASSED somewhere — VACUOUS' : `FAILS as designed (${targets.length}/${targets.length})`}`);
    process.exit(vacuous ? 1 : 0);
  }
  console.log(`${results.length - results.filter((r) => !r.ok).length} / ${results.length} arms PASS${errors.length ? ' (+ page errors)' : ''}`);
  process.exit(failed);
}

main().catch((e) => { console.error(e); process.exit(6); });
