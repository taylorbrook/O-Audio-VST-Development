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
// lesson-row-budget.mjs — the UI-08 lesson row is a WIDTH BUDGET, per language.
//
// Eight [data-i18n] buttons share one 28 px row with the "Lessons" label and
// the right-aligned hover hint. A reviewed French caption that grows a few px
// wraps the row or slides under the hint, and check-ui-labels cannot see it:
// the buttons are labels (allowed to move), and the strip they sit in is
// width-pinned (Stage 3 T11). So the budget is a script, not an eyeball
// (memory pattern_composed_readout_needs_setlabel_tokens_and_tour_row_budget):
//
//   all 8 .tour-btn on ONE line (equal tops within 1 px), and
//   Σ button widths + 7 × gap  ≤  (hint.left − 8) − strip.left
//
// It prints the slack per language. Re-run it after ANY change to a lesson
// caption (RESEARCH §Q9: the fr slack was 86 px at the v1 mockup).
//
// A second arm checks the wiring the stub CAN see: clicking each button calls
// the applyFactoryPreset native once with its stable id (the stub only logs the
// call; the parameter moves are gated in C++ by harness ST6). Stage 4 (P11):
// a click alone lights NOTHING — the lit button follows the C++ presetState
// push, which the arm then emits for the last lesson.
//
// Serves the REAL page tree through scripts/serve-ui.js (generic stub, the
// plugin's generic-overrides.json — the banner must read seed=overrides) at the
// editor's own size, read from PluginEditor.cpp.
//
// Exit = number of failed arms (one per language + the natives arm);
// 77 = Playwright unresolvable — NOTHING was verified, never a pass.
// Usage: node plugins/O-simpleAmbisonics/tests/ui-probes/lesson-row-budget.mjs
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

const LANGS      = ['en', 'fr', 'zh-Hans'];
const HINT_GAP   = 8;     // the budget keeps this much air before the hint
const FR_MIN_SLACK = 8;   // PLAN T12 acceptance: the French row keeps ≥ 8 px
const LESSON_IDS = ['pointSource', 'orderStep', 'behindYou', 'overhead',
                    'rotateRoom', 'farNear', 'slowOrbit', 'speakersHeadphones'];

let failed = 0;
function arm(ok, what) {
  console.log(`${ok ? 'PASS' : 'FAIL'} ${what}`);
  if (!ok) ++failed;
}

(async () => {
  const pw = S.resolvePlaywright();
  if (!pw) { console.error('lesson-row-budget: Playwright unresolvable — NOTHING was verified (exit 77, never a pass).'); process.exit(77); }

  const built = S.buildRoot(NAME);
  const es = S.readEditorSize(NAME);
  const srv = await S.serve(built.root);
  let browser;
  try { browser = await pw.chromium.launch(); }
  catch (e) {
    console.error(`lesson-row-budget: chromium did not launch — ${e.message} (exit 77)`);
    await srv.close(); process.exit(77);
  }
  const page = await browser.newPage({ viewport: { width: es.w, height: es.h } });
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e && e.message ? e.message : e)));
  await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
  await page.waitForFunction(() => typeof window.__setLanguage === 'function' && document.querySelectorAll('.tour-btn').length === 8, null, { timeout: 10000 });
  await page.evaluate(() => document.fonts.ready);

  const setup = await page.evaluate(() => ({ title: document.title, iw: innerWidth, ih: innerHeight }));
  const setupOk = /O-simpleAmbisonics/.test(setup.title) && built.seedFrom === 'overrides' && setup.iw === es.w && setup.ih === es.h;
  console.log(`setup: "${setup.title}" · seed=${built.seedFrom} · viewport ${setup.iw}×${setup.ih} (editor ${es.w}×${es.h})`);
  if (!setupOk) {
    console.error('lesson-row-budget: wrong page, wrong seed or wrong viewport — refusing to measure (exit 6)');
    await browser.close(); await srv.close(); process.exit(6);
  }

  for (const lang of LANGS) {
    await page.evaluate((l) => window.__setLanguage(l), lang);
    await page.waitForTimeout(200);
    const m = await page.evaluate(() => {
      const strip = document.querySelector('.tour-buttons').getBoundingClientRect();
      const hint  = document.querySelector('.tour-hint');
      const range = document.createRange(); range.selectNodeContents(hint);
      const hintText = range.getBoundingClientRect();   // the TEXT, not the pinned box
      const btns  = [...document.querySelectorAll('.tour-btn')].map((b) => {
        const r = b.getBoundingClientRect();
        return { text: b.textContent, w: r.width, top: r.top, right: r.right };
      });
      const gap = parseFloat(getComputedStyle(document.querySelector('.tour-buttons')).columnGap) || 0;
      return { stripLeft: strip.left, hintLeft: hintText.left, gap, btns };
    });
    const sum = m.btns.reduce((a, b) => a + b.w, 0);
    const used = sum + (m.btns.length - 1) * m.gap;
    const avail = (m.hintLeft - HINT_GAP) - m.stripLeft;
    const slack = avail - used;
    const tops = m.btns.map((b) => b.top);
    const oneLine = Math.max(...tops) - Math.min(...tops) <= 1;
    const lastRight = Math.max(...m.btns.map((b) => b.right));
    const minSlack = lang === 'fr' ? FR_MIN_SLACK : 0;
    console.log(`  ${lang}: strip.left ${m.stripLeft.toFixed(1)} · Σ buttons ${sum.toFixed(1)} + 7 × ${m.gap} = ${used.toFixed(1)} · `
              + `hint text left ${m.hintLeft.toFixed(1)} · available ${avail.toFixed(1)} · last button right ${lastRight.toFixed(1)} · SLACK ${slack.toFixed(1)} px`);
    for (const b of m.btns) console.log(`      ${b.w.toFixed(1).padStart(6)}  ${b.text}`);
    arm(oneLine && slack >= minSlack,
        `[${lang}] 8 lesson buttons on one line, slack ${slack.toFixed(1)} px ≥ ${minSlack} px before the hint`);
  }

  // ── natives arm: each button → applyFactoryPreset(<stable id>) ──
  await page.evaluate((l) => window.__setLanguage(l), 'en');
  const before = await page.evaluate(() => (window.__stubNativeCalls || []).length);
  for (const id of LESSON_IDS) { await page.click(`#lesson-${id}`); await page.waitForTimeout(40); }
  const calls = await page.evaluate((n) => (window.__stubNativeCalls || []).slice(n)
    .filter((c) => c.name === 'applyFactoryPreset').map((c) => c.args[0]), before);
  console.log(`  applyFactoryPreset calls: ${JSON.stringify(calls)}`);
  arm(JSON.stringify(calls) === JSON.stringify(LESSON_IDS), `[natives] 8 clicks → 8 applyFactoryPreset calls with the 8 stable ids, in order`);
  const litByClick = await page.evaluate(() => [...document.querySelectorAll('.tour-btn.active')].map((b) => b.id));
  arm(litByClick.length === 0, `[P11] the clicks alone light nothing (${JSON.stringify(litByClick)})`);
  await page.evaluate(() => window.__stubEmit('presetState', { name: 'Speakers vs Headphones', id: 'speakersHeadphones', factory: true, modified: false }));
  await page.waitForTimeout(40);
  const lit = await page.evaluate(() => [...document.querySelectorAll('.tour-btn.active')].map((b) => b.id));
  arm(lit.length === 1 && lit[0] === 'lesson-speakersHeadphones', `[P11] presetState { speakersHeadphones, unmodified } lights exactly that button (${JSON.stringify(lit)})`);
  await page.evaluate(() => window.__stubEmit('presetState', { name: 'Speakers vs Headphones', id: 'speakersHeadphones', factory: true, modified: true }));
  await page.waitForTimeout(40);
  const litMod = await page.evaluate(() => document.querySelectorAll('.tour-btn.active').length);
  arm(litMod === 0, `[P11] the same preset MODIFIED lights nothing (${litMod} lit)`);

  arm(errors.length === 0, `no page error (${errors.length}${errors.length ? ': ' + errors[0] : ''})`);

  await browser.close(); await srv.close();
  fs.rmSync(built.root, { recursive: true, force: true });
  console.log(failed ? `\n${failed} FAILED` : '\nALL PASS');
  process.exit(failed);
})().catch((e) => { console.error(e); process.exit(70); });
