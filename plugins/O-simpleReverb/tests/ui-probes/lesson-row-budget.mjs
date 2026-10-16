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
// lesson-row-budget.mjs — the TWO lesson rows are each a WIDTH BUDGET, per
// language (family-retrofit PLAN T8, RESEARCH R8), ported from
// O-simpleAmbisonics' single-row probe.
//
//   Spaces  6 fixed-width (84 px) type buttons — the TYPE choice strings,
//           English everywhere (M2) — and the row's hint
//   Ideas   Init + 4 concept buttons (content-sized, localized) and the hint
//
// Each row shares its 23 px line with the row label and the right-aligned hover
// hint. A reviewed French caption that grows a few px wraps the row or slides
// under the hint, and check-ui-labels cannot see it (the buttons are labels,
// allowed to move; the strip they sit in is width-pinned). So, per row:
//
//   every .tour-btn on ONE line (equal tops within 1 px), every caption fits
//   its own button (scrollWidth <= clientWidth), and
//   Σ button widths + (n − 1) × gap  ≤  (hint TEXT left − 8) − strip.left
//
// with ≥ 8 px slack required in fr (the widest language) and ≥ 0 elsewhere. It
// prints the slack per row and language. Re-run after ANY lesson caption edit.
//
// The natives arm: clicking all 11 buttons calls applyFactoryPreset once each
// with its stable id, in order; the clicks alone light NOTHING (the lit button
// follows the C++ presetState push), which the arm then emits.
//
// Every arm runs at 0.6x / 1.0x / 1.5x of the editor size (geometry in DESIGN
// px: getBoundingClientRect / uiScale).
//
// Exit = number of failed arms; 77 = Playwright unresolvable — NOTHING was
// verified, never a pass.
// Usage: node plugins/O-simpleReverb/tests/ui-probes/lesson-row-budget.mjs [--scale 1.0]
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
const SCALES = argVal('--scale') ? [Number(argVal('--scale'))] : [0.6, 1.0, 1.5];

const LANGS        = ['en', 'fr', 'zh-Hans'];
const HINT_GAP     = 8;    // the budget keeps this much air before the hint
const FR_MIN_SLACK = 8;    // the French row keeps ≥ 8 px
const ROWS = [
  ['Spaces', '.tour-row.types', ['booth', 'room', 'hall', 'spring', 'plate', 'ambient']],
  ['Ideas',  '.tour-row.ideas', ['init', 'sizeVsDecay', 'character', 'lowCut', 'insertSend']],
];
const LESSON_IDS = ROWS.flatMap(([, , ids]) => ids);

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
  if (!/overrides/.test(built.seedFrom)) { console.error('lesson-row-budget: stub not seeded from generic-overrides.json (exit 6)'); process.exit(6); }

  for (const scale of SCALES) {
    const vw = Math.round(es.w * scale), vh = Math.round(es.h * scale);
    const S_ = `${scale.toFixed(1)}x`;
    const page = await browser.newPage({ viewport: { width: vw, height: vh } });
    const errors = [];
    page.on('pageerror', (e) => errors.push(String(e && e.message ? e.message : e)));
    await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
    await page.waitForFunction(() => typeof window.__setLanguage === 'function' && document.querySelectorAll('.tour-btn').length === 11, null, { timeout: 10000 });
    await page.evaluate(() => document.fonts.ready);

    const setup = await page.evaluate(() => ({ title: document.title, iw: innerWidth, ih: innerHeight, tf: document.getElementById('plugin').style.transform || 'none' }));
    const setupOk = /O-simpleReverb/.test(setup.title) && setup.iw === vw && setup.ih === vh && (scale === 1 ? setup.tf === 'none' : setup.tf === `scale(${scale})`);
    console.log(`\n== ${S_}: "${setup.title}" · seed=${built.seedFrom} · viewport ${setup.iw}×${setup.ih} · #plugin ${setup.tf}`);
    if (!setupOk) {
      console.error('lesson-row-budget: wrong page, wrong viewport or wrong scale — refusing to measure (exit 6)');
      await browser.close(); await srv.close(); process.exit(6);
    }

    for (const lang of LANGS) {
      await page.evaluate((l) => window.__setLanguage(l), lang);
      await page.waitForTimeout(200);
      for (const [rowName, sel, ids] of ROWS) {
        const m = await page.evaluate((rs) => {
          const sc = parseFloat((document.getElementById('plugin').style.transform.match(/scale\(([\d.]+)\)/) || [0, 1])[1]);
          const row = document.querySelector(rs);
          const stripEl = row.querySelector('.tour-buttons');
          const strip = stripEl.getBoundingClientRect();
          const hint = row.querySelector('.tour-hint');
          const range = document.createRange(); range.selectNodeContents(hint);
          const hintText = range.getBoundingClientRect();   // the TEXT, not the pinned box
          const btns = [...stripEl.querySelectorAll('.tour-btn')].map((b) => {
            const r = b.getBoundingClientRect();
            return { id: b.dataset.preset, text: b.textContent, w: r.width / sc, top: r.top / sc, right: r.right / sc, sw: b.scrollWidth, cw: b.clientWidth };
          });
          const gap = parseFloat(getComputedStyle(stripEl).columnGap) || 0;
          return { stripLeft: strip.left / sc, hintLeft: hintText.left / sc, hintFits: hint.scrollWidth <= hint.clientWidth, gap, btns };
        }, sel);
        const sum = m.btns.reduce((a, b) => a + b.w, 0);
        const used = sum + (m.btns.length - 1) * m.gap;
        const avail = (m.hintLeft - HINT_GAP) - m.stripLeft;
        const slack = avail - used;
        const tops = m.btns.map((b) => b.top);
        const oneLine = Math.max(...tops) - Math.min(...tops) <= 1;
        const capsFit = m.btns.every((b) => b.sw <= b.cw);
        const idsOk = JSON.stringify(m.btns.map((b) => b.id)) === JSON.stringify(ids);
        const minSlack = lang === 'fr' ? FR_MIN_SLACK : 0;
        console.log(`  ${lang} ${rowName}: strip.left ${m.stripLeft.toFixed(1)} · Σ ${sum.toFixed(1)} + ${m.btns.length - 1} × ${m.gap} = ${used.toFixed(1)} · `
                  + `hint text left ${m.hintLeft.toFixed(1)} · available ${avail.toFixed(1)} · SLACK ${slack.toFixed(1)} px`);
        for (const b of m.btns) console.log(`      ${b.w.toFixed(1).padStart(6)}  ${b.text}${b.sw > b.cw ? `  (caption ${b.sw}/${b.cw} OVERFLOWS)` : ''}`);
        arm(oneLine && capsFit && idsOk && m.hintFits && slack >= minSlack,
            `[${S_} ${lang}] ${rowName}: ${m.btns.length} buttons on one line, captions fit, hint fits, slack ${slack.toFixed(1)} px ≥ ${minSlack} px`);
      }
    }

    // ── natives arm: each button → applyFactoryPreset(<stable id>) ──
    await page.evaluate((l) => window.__setLanguage(l), 'en');
    const before = await page.evaluate(() => (window.__stubNativeCalls || []).length);
    for (const id of LESSON_IDS) { await page.click(`#lesson-${id}`); await page.waitForTimeout(30); }
    const calls = await page.evaluate((n) => (window.__stubNativeCalls || []).slice(n)
      .filter((c) => c.name === 'applyFactoryPreset').map((c) => c.args[0]), before);
    arm(JSON.stringify(calls) === JSON.stringify(LESSON_IDS), `[${S_} natives] 11 clicks → 11 applyFactoryPreset calls with the stable ids, in order (${JSON.stringify(calls)})`);
    const litByClick = await page.evaluate(() => [...document.querySelectorAll('.tour-btn.active')].map((b) => b.id));
    arm(litByClick.length === 0, `[${S_} P11] the clicks alone light nothing (${JSON.stringify(litByClick)})`);
    await page.evaluate(() => window.__stubEmit('presetState', { name: 'Lesson - Insert vs Send', id: 'insertSend', factory: true, modified: false }));
    await page.waitForTimeout(40);
    const lit = await page.evaluate(() => [...document.querySelectorAll('.tour-btn.active')].map((b) => b.id));
    arm(lit.length === 1 && lit[0] === 'lesson-insertSend', `[${S_} P11] presetState { insertSend, unmodified } lights exactly that button (${JSON.stringify(lit)})`);
    await page.evaluate(() => window.__stubEmit('presetState', { name: 'Lesson - Insert vs Send', id: 'insertSend', factory: true, modified: true }));
    await page.waitForTimeout(40);
    const litMod = await page.evaluate(() => document.querySelectorAll('.tour-btn.active').length);
    arm(litMod === 0, `[${S_} P11] the same preset MODIFIED lights nothing (${litMod} lit)`);

    arm(errors.length === 0, `[${S_}] no page error (${errors.length}${errors.length ? ': ' + errors[0] : ''})`);
    await page.close();
  }

  await browser.close(); await srv.close();
  fs.rmSync(built.root, { recursive: true, force: true });
  console.log(failed ? `\n${failed} FAILED` : '\nALL PASS');
  process.exit(failed);
})().catch((e) => { console.error(e); process.exit(70); });
