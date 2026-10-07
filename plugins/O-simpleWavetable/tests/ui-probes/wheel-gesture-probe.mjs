#!/usr/bin/env node
/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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
// G-S3W3-WHEEL — wheel + gesture probe for the O-simpleWavetable page
// (Stage 4 PLAN D-AO, D-AP; recipe R-WHEEL).
//
// Serves the page headless behind the GENERIC bridge stub
// (scripts/ui-stub/generic-juce-stub.js, the one check-ui-labels uses) and
// drives real wheel / pointer input. Every verdict is read from the stub's
// relay STATE (window.__stubStates) or the stub's native-call log
// (window.__stubNativeCalls), never from DOM text.
//
// Arms:
//   stepped     bit_depth: a 50-event pixel flick (deltaY -4 every 8 ms, one
//               200 px burst) moves 1..3 detents; one mouse notch (deltaMode 1,
//               3 lines) after a quiet gap moves exactly 1.            (W3)
//   stack       the bank stack (32 frames): the same flick moves 1..3 frames;
//               one notch moves exactly 1 frame.                       (W3)
//   continuous  lfo_depth: the same flick moves 1..3 nudges; one notch moves
//               exactly 1 nudge.                                       (W3b)
//   N8          a stack wheel burst, then a hero-knob drag starting inside the
//               burst's 250 ms gesture window: `position`'s
//               sliderDragStarted / sliderDragEnded strictly alternate.
//   N13         a 3-detent pointer drag on bit_depth: the stepKnobDrag calls
//               are begin, >= 1 move, end — and the page makes 0 bit_depth
//               setChoiceIndex calls (per-detent complete gestures) during it.
//   liveness    every arm above moved its control at least once, so no
//               verdict passes by the probe failing to reach the control.
//
// Usage:
//   node plugins/O-simpleWavetable/tests/ui-probes/wheel-gesture-probe.mjs
//        [--root <dir>] [--negative-control] [--verbose]
//
//   --root  the page tree to serve. Accepts the UI dir itself (it holds
//           index.html), or any tree containing
//           plugins/O-simpleWavetable/Source/ui/public or Source/ui/public —
//           e.g. a `git archive f4eea85a` extraction (copy the gitignored
//           Source/ui/public/modules/*.js into it first). Default: this
//           plugin's Source/ui/public.
//   --negative-control   annotate each failing arm "FAILS as designed" (and a
//           passing one "PASSES — negative control is VACUOUS"). The exit code
//           does not change: it is still the number of failed arms.
//
// Exit: 0 = every arm PASS; n = the number of FAILED arms; 77 = Playwright or
// its Chromium is not available — NOTHING was verified, and 77 is never a pass.
//
// TEST TOOL ONLY. Never shipped, never embedded.
// ============================================================================

import { createRequire } from 'module';
import { fileURLToPath } from 'url';
import fs from 'fs';
import os from 'os';
import path from 'path';

const require  = createRequire(import.meta.url);
const HERE     = path.dirname(fileURLToPath(import.meta.url));
const PLUGIN   = path.resolve(HERE, '..', '..');
const REPO     = path.resolve(PLUGIN, '..', '..');
const S        = require(path.join(REPO, 'scripts', 'serve-ui.js'));

const argv = process.argv.slice(2);
const argVal = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const NC      = argv.includes('--negative-control');
const VERBOSE = argv.includes('--verbose');

// ── constants of the probe (input shape, not page constants) ────────────────
const FLICK_EVENTS  = 50;    // one trackpad flick ...
const FLICK_DY      = -4;    // ... of 200 px total
const FLICK_GAP_MS  = 8;
const NOTCH_DY      = -3;    // a mouse notch: 3 lines
const QUIET_MS      = 450;   // > the page's 250 ms burst gap, so the next event starts a burst
const MAX_FLICK     = 3;     // RESEARCH A §5: a flick moves <= ~3 steps
const STACK_FRAMES  = 32;
const NUDGE         = 0.02;  // the page's continuous nudge on a 0..1 stub range with no interval

// bit_depth's real 15 choices (PluginProcessor.cpp createParameterLayout),
// started mid-range so a runaway flick has room to show itself either way.
const BIT_DEPTH_CHOICES = ['Full', '16', '15', '14', '13', '12', '11', '10', '9', '8', '7', '6', '5', '4', '3'];
const LFO_DIV_CHOICES   = ['4 bars', '2 bars', '1/1', '1/2', '1/4', '1/8', '1/16', '1/32',
                           '1/2.', '1/4.', '1/8.', '1/16.', '1/2T', '1/4T', '1/8T', '1/16T'];
const BANK_CHOICES      = ['Sine → Saw', 'Sine → Square', 'Pulse Width', 'Formant', 'Drive', 'Imported'];
const START_IDX = 7;

const SEED = {
  sliders: {},
  toggles: {},
  combos: {
    bit_depth: { choices: BIT_DEPTH_CHOICES, def: START_IDX },
    lfo_div:   { choices: LFO_DIV_CHOICES, def: 2 },
    bank:      { choices: BANK_CHOICES, def: 0 },
  },
  natives: {
    getCurrentPreset: 'Init - Additive Build',
    getPresetList: ['Init - Additive Build', 'Stepped Scan', 'Smooth Scan', 'Alias Demo', 'Drive Sweep',
                    'Vowel Pad', 'Pulse Narrowing', '8-bit PPG', '4-bit PPG'],
    getPresetCatalog: {
      factory: [
        { name: 'Init - Additive Build', id: 'init' }, { name: 'Stepped Scan', id: 'steppedSmooth' },
        { name: 'Smooth Scan', id: 'smoothScan' }, { name: 'Alias Demo', id: 'aliasDemo' },
        { name: 'Drive Sweep', id: 'driveSweep' }, { name: 'Vowel Pad', id: 'vowelPad' },
        { name: 'Pulse Narrowing', id: 'pulseNarrowing' }, { name: '8-bit PPG', id: 'ppg8bit' },
        { name: '4-bit PPG', id: 'ppg4bit' },
      ],
      user: [],
    },
  },
  __from: 'wheel-gesture-probe seed',
};

// ── the page tree ───────────────────────────────────────────────────────────
function resolveUiDir() {
  const root = argVal('--root');
  if (!root) return path.join(PLUGIN, 'Source', 'ui', 'public');
  const abs = path.resolve(root);
  const candidates = [
    abs,
    path.join(abs, 'plugins', 'O-simpleWavetable', 'Source', 'ui', 'public'),
    path.join(abs, 'Source', 'ui', 'public'),
    path.join(abs, 'public'),
  ];
  for (const c of candidates) if (fs.existsSync(path.join(c, 'index.html'))) return c;
  return null;
}

function buildTree(uiDir) {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'oswt-wheel-probe-'));
  fs.cpSync(uiDir, tmp, { recursive: true });
  const juceDir = path.join(tmp, 'js', 'juce');
  fs.mkdirSync(juceDir, { recursive: true });
  fs.copyFileSync(path.join(REPO, 'scripts', 'ui-stub', 'generic-juce-stub.js'), path.join(juceDir, 'index.js'));
  const preamble = fs.readFileSync(path.join(REPO, 'scripts', 'ui-stub', 'stub-preamble.js'), 'utf8')
                 + `\nwindow.__stubOverrides = ${JSON.stringify(SEED)};\n`;
  fs.writeFileSync(path.join(juceDir, 'stub-preamble.js'), preamble);
  const indexPath = path.join(tmp, 'index.html');
  const html = fs.readFileSync(indexPath, 'utf8');
  const headAt = html.search(/<head\b[^>]*>/i);
  if (headAt < 0) throw new Error('index.html has no <head>');
  const at = html.indexOf('>', headAt) + 1;
  fs.writeFileSync(indexPath, html.slice(0, at) + '\n<script src="/js/juce/stub-preamble.js"></script>' + html.slice(at));
  return tmp;
}

// ── in-page helpers ─────────────────────────────────────────────────────────
async function installInstrumentation(page) {
  return page.evaluate(() => {
    const st = window.__stubStates;
    if (!st) return 'window.__stubStates missing — not the generic stub';
    const pos = st.sliders.get('position');
    const bd  = st.combos.get('bit_depth');
    const dep = st.sliders.get('lfo_depth');
    if (!pos || !bd || !dep) return 'a probed state was never requested by the page (position / bit_depth / lfo_depth)';
    const log = { position: [], bitDepthSets: 0 };
    window.__probeLog = log;
    const s0 = pos.sliderDragStarted.bind(pos);
    const e0 = pos.sliderDragEnded.bind(pos);
    pos.sliderDragStarted = () => { log.position.push('S'); s0(); };
    pos.sliderDragEnded   = () => { log.position.push('E'); e0(); };
    const set0 = bd.setChoiceIndex.bind(bd);
    bd.setChoiceIndex = (i) => { log.bitDepthSets++; set0(i); };
    window.__probeWheel = async (sel, n, dy, mode, gapMs) => {
      const el = document.querySelector(sel);
      if (!el) return false;
      for (let i = 0; i < n; i++) {
        el.dispatchEvent(new WheelEvent('wheel', { deltaX: 0, deltaY: dy, deltaMode: mode, bubbles: true, cancelable: true }));
        if (gapMs > 0) await new Promise((r) => setTimeout(r, gapMs));
      }
      return true;
    };
    return null;
  });
}

const sleep = (page, ms) => page.waitForTimeout(ms);
const bitIdx   = (page) => page.evaluate(() => window.__stubStates.combos.get('bit_depth').getChoiceIndex());
const setBit   = (page, i) => page.evaluate((v) => {
  const st = window.__stubStates.combos.get('bit_depth');
  st.choiceIndex = v; st.valueChangedEvent.callListeners();   // direct: not a page setChoiceIndex call
}, i);
const sliderN  = (page, id) => page.evaluate((k) => window.__stubStates.sliders.get(k).getNormalisedValue(), id);
const setSlider = (page, id, n) => page.evaluate(([k, v]) => window.__stubStates.sliders.get(k).setNormalisedValue(v), [id, n]);
const wheel    = (page, sel, n, dy, mode, gap) => page.evaluate(([s, a, b, c, d]) => window.__probeWheel(s, a, b, c, d), [sel, n, dy, mode, gap]);

// ── reporting ───────────────────────────────────────────────────────────────
const results = [];
function arm(name, checks) {
  const ok = checks.every((c) => c.ok);
  for (const c of checks) console.log(`  ${c.ok ? 'PASS' : 'FAIL'} ${name}: ${c.what} — ${c.got}`);
  let line = `${ok ? 'PASS' : 'FAIL'} G-S3W3-WHEEL[${name}]`;
  if (NC && name !== 'liveness') line += ok ? '  — PASSES: negative control is VACUOUS' : '  — FAILS as designed';
  console.log(line);
  results.push({ name, ok });
}

// ── main ────────────────────────────────────────────────────────────────────
async function main() {
  const pw = S.resolvePlaywright();
  if (!pw) {
    console.error('wheel-gesture-probe: Playwright unresolvable — NOTHING was verified (exit 77, never a pass).');
    console.error('                     npx playwright install chromium, then re-run.');
    process.exit(77);
  }
  const uiDir = resolveUiDir();
  if (!uiDir) { console.error(`wheel-gesture-probe: no index.html under --root ${argVal('--root')}`); process.exit(6); }

  const tree = buildTree(uiDir);
  const srv = await S.serve(tree);
  let browser;
  try { browser = await pw.chromium.launch(); }
  catch (e) {
    console.error(`wheel-gesture-probe: Chromium did not launch — NOTHING was verified (exit 77): ${String(e.message).split('\n')[0]}`);
    await srv.close(); fs.rmSync(tree, { recursive: true, force: true });
    process.exit(77);
  }

  console.log(`G-S3W3-WHEEL — O-simpleWavetable page: ${uiDir}`);
  if (NC) console.log('  (negative-control mode: failing arms are expected)');

  const page = await browser.newPage({ viewport: { width: 1120, height: 780 } });
  const pageErrors = [];
  page.on('pageerror', (e) => pageErrors.push(String(e && e.message ? e.message : e)));
  if (VERBOSE) page.on('console', (m) => console.log(`  [console.${m.type()}] ${m.text()}`));

  await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
  await sleep(page, 800);

  const instErr = await installInstrumentation(page);
  if (instErr) {
    console.log(`FAIL setup: ${instErr}`);
    await browser.close(); await srv.close(); fs.rmSync(tree, { recursive: true, force: true });
    process.exit(6);
  }

  const live = {};

  // ── stepped (W3) ──
  {
    await setBit(page, START_IDX);
    await sleep(page, QUIET_MS);
    await wheel(page, '#knob-bit_depth', FLICK_EVENTS, FLICK_DY, 0, FLICK_GAP_MS);
    const flick = Math.abs((await bitIdx(page)) - START_IDX);
    await sleep(page, QUIET_MS);
    await setBit(page, START_IDX);
    await wheel(page, '#knob-bit_depth', 1, NOTCH_DY, 1, 0);
    const notch = Math.abs((await bitIdx(page)) - START_IDX);
    live.stepped = flick >= 1 && notch >= 1;
    arm('stepped', [
      { what: `200 px flick moves <= ${MAX_FLICK} detents`, got: `${flick} detent(s)`, ok: flick <= MAX_FLICK },
      { what: 'one notch moves exactly 1 detent', got: `${notch} detent(s)`, ok: notch === 1 },
    ]);
    await sleep(page, QUIET_MS);
  }

  // ── stack (W3) ──
  {
    await page.evaluate((n) => {
      const f = (k) => Array.from({ length: 128 }, (_, i) => Math.sin(2 * Math.PI * (i / 128) * (1 + k * 0.1)) * 0.8);
      window.__stubEmit('bankUpdate', { bank: 0, imported: false, numFrames: n, filename: '', frames: Array.from({ length: n }, (_, k) => f(k)) });
    }, STACK_FRAMES);
    await sleep(page, 300);
    await setSlider(page, 'position', 0.5);
    await sleep(page, QUIET_MS);
    const frames = (a, b) => Math.round(Math.abs(b - a) * (STACK_FRAMES - 1));
    let a = await sliderN(page, 'position');
    await wheel(page, '#bankWrap', FLICK_EVENTS, FLICK_DY, 0, FLICK_GAP_MS);
    const flick = frames(a, await sliderN(page, 'position'));
    await sleep(page, QUIET_MS);
    await setSlider(page, 'position', 0.5);
    a = await sliderN(page, 'position');
    await wheel(page, '#bankWrap', 1, NOTCH_DY, 1, 0);
    const notch = frames(a, await sliderN(page, 'position'));
    live.stack = flick >= 1 && notch >= 1;
    arm('stack', [
      { what: `200 px flick moves <= ${MAX_FLICK} frames`, got: `${flick} frame(s) of ${STACK_FRAMES}`, ok: flick <= MAX_FLICK },
      { what: 'one notch moves exactly 1 frame', got: `${notch} frame(s)`, ok: notch === 1 },
    ]);
    await sleep(page, QUIET_MS);
  }

  // ── continuous (W3b) ──
  {
    await setSlider(page, 'lfo_depth', 0.5);
    await sleep(page, QUIET_MS);
    const nudges = (a, b) => Math.round(Math.abs(b - a) / NUDGE);
    let a = await sliderN(page, 'lfo_depth');
    await wheel(page, '#knob-lfo_depth', FLICK_EVENTS, FLICK_DY, 0, FLICK_GAP_MS);
    const flick = nudges(a, await sliderN(page, 'lfo_depth'));
    await sleep(page, QUIET_MS);
    await setSlider(page, 'lfo_depth', 0.5);
    a = await sliderN(page, 'lfo_depth');
    await wheel(page, '#knob-lfo_depth', 1, NOTCH_DY, 1, 0);
    const notch = nudges(a, await sliderN(page, 'lfo_depth'));
    live.continuous = flick >= 1 && notch >= 1;
    arm('continuous', [
      { what: `200 px flick moves <= ${MAX_FLICK} nudges`, got: `${flick} nudge(s)`, ok: flick <= MAX_FLICK },
      { what: 'one notch moves exactly 1 nudge', got: `${notch} nudge(s)`, ok: notch === 1 },
    ]);
    await sleep(page, QUIET_MS);
  }

  // ── N8: stack wheel burst overlapping a hero-knob drag ──
  {
    await setSlider(page, 'position', 0.5);
    await sleep(page, QUIET_MS);
    await page.evaluate(() => { window.__probeLog.position.length = 0; });
    const before = await sliderN(page, 'position');
    await wheel(page, '#bankWrap', 5, FLICK_DY, 0, FLICK_GAP_MS);          // opens the stack's gesture
    const box = await page.locator('#knob-position').boundingBox();
    const cx = box.x + box.width / 2, cy = box.y + box.height / 2;
    await page.mouse.move(cx, cy);
    await page.mouse.down();                                               // well inside the 250 ms window
    for (let i = 1; i <= 6; i++) await page.mouse.move(cx, cy - 6 * i);
    await page.mouse.up();
    await sleep(page, QUIET_MS);                                            // the stack's timer ends its gesture
    const seq = (await page.evaluate(() => window.__probeLog.position.join('')));
    const after = await sliderN(page, 'position');
    live.N8 = Math.abs(after - before) > 1e-6;
    arm('N8', [
      { what: 'position sliderDragStarted / Ended strictly alternate (S E S E …)', got: `"${seq}"`, ok: /^(SE)+$/.test(seq) },
    ]);
  }

  // ── N13: a 3-detent pointer drag on bit_depth ──
  {
    const start = 3;
    await setBit(page, start);
    await sleep(page, 150);
    await page.evaluate(() => { window.__probeLog.bitDepthSets = 0; window.__probeMark = window.__stubNativeCalls.length; });
    const box = await page.locator('#knob-bit_depth').boundingBox();
    const cx = box.x + box.width / 2, cy = box.y + box.height / 2;
    await page.mouse.move(cx, cy);
    await page.mouse.down();
    for (let i = 1; i <= 9; i++) await page.mouse.move(cx, cy - 5 * i);     // 45 px = 3 detents of 14 px
    await page.mouse.up();
    await sleep(page, 200);
    const r = await page.evaluate(() => ({
      calls: window.__stubNativeCalls.slice(window.__probeMark).filter((c) => c.name === 'stepKnobDrag')
                                     .map((c) => ({ phase: c.args[1], idx: c.args[2], id: c.args[0] })),
      sets: window.__probeLog.bitDepthSets,
      idx: window.__stubStates.combos.get('bit_depth').getChoiceIndex(),
    }));
    const phases = r.calls.map((c) => c.phase).join(',');
    const moves = r.calls.filter((c) => c.phase === 1);
    const bracket = r.calls.length >= 3 && r.calls[0].phase === 0 && r.calls[r.calls.length - 1].phase === 2
                 && moves.length >= 1 && moves.length === r.calls.length - 2
                 && r.calls.every((c) => c.id === 'bit_depth');
    const lastMove = moves.length ? moves[moves.length - 1].idx : null;
    live.N13 = (lastMove !== null && lastMove !== start) || r.idx !== start;
    arm('N13', [
      { what: 'stepKnobDrag = begin, >= 1 move, end (bit_depth)', got: `phases [${phases}]`, ok: bracket },
      { what: '0 bit_depth setChoiceIndex calls during the drag', got: `${r.sets} call(s)`, ok: r.sets === 0 },
      { what: 'the drag reached index start + 3', got: `last move ${lastMove}`, ok: lastMove === start + 3 },
    ]);
  }

  // ── liveness ──
  arm('liveness', Object.entries(live).map(([k, v]) => ({ what: `${k} moved its control`, got: v ? 'moved' : 'did not move', ok: v })));

  if (pageErrors.length) {
    console.log(`  NOTE: ${pageErrors.length} page error(s):`);
    for (const e of pageErrors) console.log(`    ${e}`);
  }

  await browser.close();
  await srv.close();
  fs.rmSync(tree, { recursive: true, force: true });

  const failed = results.filter((r) => !r.ok).length;
  console.log(`\nG-S3W3-WHEEL: ${results.length - failed} / ${results.length} arm(s) PASS`
            + (NC ? `  (negative control: ${failed} arm(s) failed)` : ''));
  process.exit(failed);
}

main().catch((e) => { console.error('wheel-gesture-probe crashed:', e); process.exit(6); });
