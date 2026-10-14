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
// header-fit-probe.mjs — Stage 4 (PLAN T5 / T10) fit arms the label gate is
// blind to (memory pattern_ui_labels_gate_blind_to_width_pinned_data_i18n):
//
//   H1  the #encReadN readout fits its 144 px .enc-read box (nowrap + ellipsis)
//       at order 3 and order 2, in en / fr / zh-Hans. check-ui-labels [4] skips
//       non-replaced inline boxes (scripts/check-ui-labels.js:798-810), so this
//       is the only arm that sees the box overflow.
//   H2  the header after the latency chip's retirement (R-D1), at order 3 (the
//       widest label.chipOrder) in en / fr / zh-Hans: .title-block 425 ± 0.5;
//       .header-strip scrollWidth <= clientWidth; #chipFormat's body and every
//       preset-bar caption fit their pinned boxes; #gear-btn left = the Stage-4
//       T1 baseline (1135) ± 0.5 px.
//   H3  presetState with each factory preset -> its localized label in
//       #presetName is NOT ellipsised (every language); a 64-character user
//       name IS ellipsised and neither the box width nor any bar element's left
//       edge moves.
//
// Serves the REAL page tree through scripts/serve-ui.js (generic stub, the
// plugin's generic-overrides.json — the banner must read seed=overrides) at the
// editor's own size, read from PluginEditor.cpp.
//
// Usage:
//   node plugins/O-simpleAmbisonics/tests/ui-probes/header-fit-probe.mjs
//        [--negative-control encreadn] [--root <repo root of an alternate tree>]
//
//   --negative-control encreadn  #encReadN is set to the pre-Stage-4 copy
//                                "(3+1)² = 16 channels": H1 [en] must FAIL
//
// Exit = number of failed arms (control run: 0 = the target FAILS as designed,
// 1 = it passed — the control is VACUOUS); 6 = wrong page / seed / viewport;
// 77 = Playwright unresolvable — NOTHING was verified, never a pass.
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
const ROOT    = argVal('--root');
const CONTROL_TARGET = { encreadn: 'H1 [en]' };
if (CONTROL && !CONTROL_TARGET[CONTROL]) { console.error(`unknown --negative-control ${CONTROL}`); process.exit(6); }

const LANGS = ['en', 'fr', 'zh-Hans'];
// Stage-4 T1 baseline (stages/4-polish SUMMARY): title 425, gear left 1135, all three languages.
const TITLE_W = 425, GEAR_LEFT = 1135, TOL = 0.5;
const FACTORY = [
  ['Init', 'init'], ['Point Source (FOA)', 'pointSource'], ['Order 1-2-3', 'orderStep'], ['Behind You', 'behindYou'],
  ['Overhead', 'overhead'], ['Rotate the Room', 'rotateRoom'], ['Far Away - Up Close', 'farNear'],
  ['Slow Orbit', 'slowOrbit'], ['Speakers vs Headphones', 'speakersHeadphones'],
];
const LONG_NAME = 'A very long user preset name that runs past the edge of the box!';   // 64 chars

const results = [];
function arm(name, ok, got) {
  let line = `${ok ? 'PASS' : 'FAIL'} ${name} — ${got}`;
  if (CONTROL && name === CONTROL_TARGET[CONTROL]) line += ok ? '  — PASSES: negative control is VACUOUS' : '  — FAILS as designed';
  console.log(line);
  results.push({ name, ok });
}

(async () => {
  const pw = S.resolvePlaywright();
  if (!pw) { console.error('header-fit-probe: Playwright unresolvable — NOTHING was verified (exit 77, never a pass).'); process.exit(77); }

  const built = S.buildRoot(NAME, ROOT ? { repoRoot: path.resolve(ROOT) } : {});
  const es = S.readEditorSize(NAME);
  const srv = await S.serve(built.root);
  let browser;
  try { browser = await pw.chromium.launch(); }
  catch (e) {
    console.error(`header-fit-probe: chromium did not launch — ${e.message} (exit 77)`);
    await srv.close(); process.exit(77);
  }
  const page = await browser.newPage({ viewport: { width: es.w, height: es.h } });
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e && e.message ? e.message : e)));
  await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
  await page.waitForFunction(() => typeof window.__setLanguage === 'function' && window.__stubStates, null, { timeout: 10000 });
  await page.evaluate(() => document.fonts.ready);

  const setup = await page.evaluate(() => ({ title: document.title, iw: innerWidth, ih: innerHeight }));
  const setupOk = /O-simpleAmbisonics/.test(setup.title) && built.seedFrom === 'overrides' && setup.iw === es.w && setup.ih === es.h;
  console.log(`header-fit-probe — "${setup.title}" · ui=${built.uiRootLabel} · seed=${built.seedFrom} · viewport ${setup.iw}×${setup.ih} (editor ${es.w}×${es.h})${CONTROL ? `  CONTROL=${CONTROL}` : ''}`);
  if (!setupOk) {
    console.error('header-fit-probe: wrong page, wrong seed or wrong viewport — refusing to measure (exit 6)');
    await browser.close(); await srv.close(); process.exit(6);
  }

  const setOrder = (idx) => page.evaluate((i) => window.__stubStates.combos.get('order').setChoiceIndex(i), idx);

  // ── H1: #encReadN inside .enc-read ──
  for (const lang of LANGS) {
    await page.evaluate((l) => window.__setLanguage(l), lang);
    const rows = [];
    let ok = true;
    for (const idx of [2, 1]) {
      await setOrder(idx);
      await page.waitForTimeout(120);
      if (CONTROL === 'encreadn' && lang === 'en') {
        await page.evaluate((i) => { document.getElementById('encReadN').textContent = `(${i + 1}+1)² = ${(i + 2) * (i + 2)} channels`; }, idx);
      }
      const m = await page.evaluate(() => {
        const box = document.getElementById('encReadN').parentElement;   // .enc-read
        return { sw: box.scrollWidth, cw: box.clientWidth, text: box.textContent.replace(/\s+/g, ' ').trim() };
      });
      ok = ok && m.sw <= m.cw;
      rows.push(`order ${idx + 1}: "${m.text}" ${m.sw} / ${m.cw} px`);
    }
    arm(`H1 [${lang}]`, ok, rows.join(' · '));
  }

  // ── H2: the header strip after the chip retirement ──
  await setOrder(2);
  for (const lang of LANGS) {
    await page.evaluate((l) => window.__setLanguage(l), lang);
    await page.waitForTimeout(150);
    const m = await page.evaluate(() => {
      const q = (s) => document.querySelector(s);
      const fitOf = (el) => ({ sw: el.scrollWidth, cw: el.clientWidth });
      const hs = q('.header-strip');
      const caps = ['#presetPrev', '#presetName .preset-name-text', '#presetNext', '#presetSave', '#presetDelete', '#chipFormat .chip-body']
        .map((s) => [s, fitOf(q(s))]);
      return { title: q('.title-block').getBoundingClientRect().width, gear: q('#gear-btn').getBoundingClientRect().left,
               strip: fitOf(hs), caps, latency: !!q('#chipLatency') };
    });
    const capsOk = m.caps.every(([, f]) => f.sw <= f.cw);
    const ok = Math.abs(m.title - TITLE_W) <= TOL && Math.abs(m.gear - GEAR_LEFT) <= TOL && m.strip.sw <= m.strip.cw && capsOk && !m.latency;
    arm(`H2 [${lang}]`, ok, `title ${m.title} · gear ${m.gear} · strip ${m.strip.sw} / ${m.strip.cw} · captions `
      + m.caps.map(([s, f]) => `${s.replace(/^#/, '')} ${f.sw}/${f.cw}`).join(', ') + (m.latency ? ' · #chipLatency STILL PRESENT' : ''));
  }

  // ── H3: the name box with every factory label, then a 64-char user name ──
  const emit = (st) => page.evaluate((x) => window.__stubEmit('presetState', x), st);
  const barGeom = () => page.evaluate(() => {
    const lefts = ['#presetPrev', '#presetName', '#presetNext', '#presetSave', '#presetDelete']
      .map((s) => +document.querySelector(s).getBoundingClientRect().left.toFixed(2));
    const t = document.querySelector('#presetName .preset-name-text');
    return { lefts, w: +document.querySelector('#presetName').getBoundingClientRect().width.toFixed(2), sw: t.scrollWidth, cw: t.clientWidth, text: t.textContent };
  });
  for (const lang of LANGS) {
    await page.evaluate((l) => window.__setLanguage(l), lang);
    const rows = [];
    let ok = true, geom0 = null;
    for (const [name, id] of FACTORY) {
      await emit({ name, id, factory: true, modified: false });
      await page.waitForTimeout(30);
      const g = await barGeom();
      if (!geom0) geom0 = g;
      ok = ok && g.sw <= g.cw && JSON.stringify(g.lefts) === JSON.stringify(geom0.lefts) && g.w === geom0.w;
      if (!(g.sw <= g.cw)) rows.push(`"${g.text}" ${g.sw}/${g.cw} ELLIPSISED`);
    }
    await emit({ name: LONG_NAME, id: '', factory: false, modified: false });
    await page.waitForTimeout(30);
    const gl = await barGeom();
    const longOk = gl.sw > gl.cw && gl.text === LONG_NAME && JSON.stringify(gl.lefts) === JSON.stringify(geom0.lefts) && gl.w === geom0.w;
    ok = ok && longOk;
    arm(`H3 [${lang}]`, ok, `9 factory labels fit (${rows.length ? rows.join('; ') : 'none ellipsised'}) · 64-char user name ${gl.sw}/${gl.cw} ${gl.sw > gl.cw ? 'ellipsised' : 'NOT ellipsised'} · box ${gl.w} px, lefts ${JSON.stringify(gl.lefts)} ${longOk ? 'unchanged' : 'MOVED'}`);
  }

  arm('no page error', errors.length === 0, `${errors.length}${errors.length ? ': ' + errors[0] : ''}`);

  await browser.close(); await srv.close();
  fs.rmSync(built.root, { recursive: true, force: true });

  if (CONTROL) {
    const t = results.find((r) => r.name === CONTROL_TARGET[CONTROL]);
    const vacuous = !t || t.ok;
    console.log(`control ${CONTROL}: target ${CONTROL_TARGET[CONTROL]} ${vacuous ? 'PASSED — VACUOUS' : 'FAILS as designed'}`);
    process.exit(vacuous ? 1 : 0);
  }
  const failed = results.filter((r) => !r.ok).length;
  console.log(failed ? `\n${failed} FAILED` : '\nALL PASS');
  process.exit(failed);
})().catch((e) => { console.error(e); process.exit(70); });
