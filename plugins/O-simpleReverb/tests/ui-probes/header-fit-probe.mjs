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
// header-fit-probe.mjs — the fit arms the label gate is blind to (memory
// pattern_ui_labels_gate_blind_to_width_pinned_data_i18n), ported from
// O-simpleAmbisonics for the O-simpleReverb page (family-retrofit PLAN T8):
//
//   H1  the #chipBody readout ("Room · RT60 1.10 s · pre 15 ms") fits its pinned
//       236 px chip for all 6 types at DECAY 0.5 and 2.0 (the longest RT60)
//   H2  the header: .title-block 425 ± 0.5; .header-strip scrollWidth <=
//       clientWidth; every preset-bar caption fits its pinned box; #gear-btn
//       left edge identical in every language (± 0.5) and inside the frame
//   H3  presetState with each of the 11 lessons and the 48 bank presets -> its
//       label in #presetName is NOT ellipsised; a 64-character user name IS
//       ellipsised and neither the box width nor any bar element's left moves
//   H4  ELLIPSIS CENSUS: every visible element whose computed text-overflow is
//       `ellipsis` fits (scrollWidth <= clientWidth) in 9 page states (6 types,
//       Character dark + Low Cut in, Character bright, Wet 0 / Dry 0) — the
//       station sub-lines, well captions, knob subs, group routes and notes
//
// Every arm runs at 0.6x / 1.0x / 1.5x of the editor size in en / fr / zh-Hans.
// Geometry is read in DESIGN px (getBoundingClientRect / uiScale); scrollWidth
// and clientWidth are layout px and do not scale.
//
// Usage:
//   node plugins/O-simpleReverb/tests/ui-probes/header-fit-probe.mjs
//        [--negative-control chipbody] [--scale 1.0] [--lang en]
//
//   --negative-control chipbody  #chipBody is overwritten with a v2-style long
//                                readout at en: H1 [1.0x en] must FAIL
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
const NAME    = 'O-simpleReverb';

const argv = process.argv.slice(2);
const argVal = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const CONTROL = argVal('--negative-control');
const CONTROL_TARGET = { chipbody: 'H1 [1.0x en]' };
if (CONTROL && !CONTROL_TARGET[CONTROL]) { console.error(`unknown --negative-control ${CONTROL}`); process.exit(6); }
const SCALES = argVal('--scale') ? [Number(argVal('--scale'))] : [0.6, 1.0, 1.5];
const LANGS  = argVal('--lang') ? [argVal('--lang')] : ['en', 'fr', 'zh-Hans'];

const OVR = JSON.parse(fs.readFileSync(path.join(PLUGIN, 'tests', 'ui-stub', 'generic-overrides.json'), 'utf8'));
const CATALOG = OVR.natives.getPresetCatalog;
const TITLE_W = 425, TOL = 0.5;
const LONG_NAME = 'A very long user preset name that runs past the edge of the box!';   // 64 chars
const DEFAULTS = OVR.natives.getParameterDefaults;
const STATES = [
  ...[0, 1, 2, 3, 4, 5].map((t) => [`TYPE ${t}`, { ...DEFAULTS, TYPE: t }]),
  ['Character dark, Low Cut in', { ...DEFAULTS, TYPE: 2, CHARACTER: -100, LPON: 1, LPFREQ: 400 }],
  ['Character bright', { ...DEFAULTS, TYPE: 5, CHARACTER: 100, DECAY: 2.0, SIZE: 100 }],
  ['Wet 0 / Dry 0', { ...DEFAULTS, TYPE: 4, WET: 0, DRY: 0 }],
];

const results = [];
function arm(name, ok, got) {
  let line = `${ok ? 'PASS' : 'FAIL'} ${name} — ${got}`;
  if (CONTROL && name === CONTROL_TARGET[CONTROL]) line += ok ? '  — PASSES: negative control is VACUOUS' : '  — FAILS as designed';
  console.log(line);
  results.push({ name, ok });
}

const SET_PARAMS = (params) => {
  const st = window.__stubStates;
  for (const [id, v] of Object.entries(params)) {
    if (st.sliders.has(id)) {
      const s = st.sliders.get(id), p = s.properties;
      const prop = Math.min(1, Math.max(0, (v - p.start) / (p.end - p.start)));
      s.setNormalisedValue(Math.pow(prop, p.skew));
    } else if (st.combos.has(id)) st.combos.get(id).setChoiceIndex(v);
    else if (st.toggles.has(id)) st.toggles.get(id).setValue(!!v);
  }
};

(async () => {
  const pw = S.resolvePlaywright();
  if (!pw) { console.error('header-fit-probe: Playwright unresolvable — NOTHING was verified (exit 77, never a pass).'); process.exit(77); }

  const built = S.buildRoot(NAME);
  const es = S.readEditorSize(NAME);
  const srv = await S.serve(built.root);
  let browser;
  try { browser = await pw.chromium.launch(); }
  catch (e) {
    console.error(`header-fit-probe: chromium did not launch — ${e.message} (exit 77)`);
    await srv.close(); process.exit(77);
  }
  console.log(`header-fit-probe — ui=${built.uiRootLabel} · seed=${built.seedFrom} · editor ${es.w}×${es.h}${CONTROL ? `  CONTROL=${CONTROL}` : ''}`);
  if (!/overrides/.test(built.seedFrom)) { console.error('header-fit-probe: stub not seeded from generic-overrides.json (exit 6)'); process.exit(6); }
  const errors = [];

  for (const scale of SCALES) {
    const vw = Math.round(es.w * scale), vh = Math.round(es.h * scale);
    const page = await browser.newPage({ viewport: { width: vw, height: vh } });
    page.on('pageerror', (e) => errors.push(`[${scale}x] ${String(e && e.message ? e.message : e)}`));
    await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
    await page.waitForFunction(() => typeof window.__setLanguage === 'function' && window.__stubStates, null, { timeout: 10000 });
    await page.evaluate(() => document.fonts.ready);
    await page.waitForTimeout(200);

    const setup = await page.evaluate(() => ({ title: document.title, iw: innerWidth, ih: innerHeight, tf: document.getElementById('plugin').style.transform || 'none' }));
    const setupOk = /O-simpleReverb/.test(setup.title) && setup.iw === vw && setup.ih === vh && (scale === 1 ? setup.tf === 'none' : setup.tf === `scale(${scale})`);
    console.log(`\n== ${scale.toFixed(1)}x: "${setup.title}" · viewport ${setup.iw}×${setup.ih} · #plugin ${setup.tf}`);
    if (!setupOk) {
      console.error('header-fit-probe: wrong page, wrong viewport or wrong scale — refusing to measure (exit 6)');
      await browser.close(); await srv.close(); process.exit(6);
    }
    const S_ = `${scale.toFixed(1)}x`;
    const setParams = (p) => page.evaluate(SET_PARAMS, p);
    const emit = (st) => page.evaluate((x) => window.__stubEmit('presetState', x), st);
    let gear0 = null;

    for (const lang of LANGS) {
      await page.evaluate((l) => window.__setLanguage(l), lang);
      await page.waitForTimeout(120);
      const tag = `[${S_} ${lang}]`;

      // ── H1: the tail chip, every type at both DECAY extremes ──
      {
        const rows = []; let ok = true;
        for (const t of [0, 1, 2, 3, 4, 5]) for (const dec of [0.5, 2.0]) {
          await setParams({ ...DEFAULTS, TYPE: t, DECAY: dec });
          await page.waitForTimeout(40);
          if (CONTROL === 'chipbody' && lang === 'en' && scale === 1)
            await page.evaluate(() => { document.getElementById('chipBody').textContent = 'Ambient · RT60 14.00 s · pre-delay 35 ms · modulated'; });
          const m = await page.evaluate(() => { const b = document.getElementById('chipBody'); return { sw: b.scrollWidth, cw: b.clientWidth, text: b.textContent }; });
          if (m.sw > m.cw) { ok = false; rows.push(`"${m.text}" ${m.sw}/${m.cw}`); }
        }
        arm(`H1 ${tag}`, ok, ok ? '12 readouts fit' : rows.join('; '));
      }

      // ── H2: the header strip ──
      {
        await setParams(DEFAULTS);
        await page.waitForTimeout(60);
        const m = await page.evaluate(() => {
          const q = (s) => document.querySelector(s);
          const sc = parseFloat((q('#plugin').style.transform.match(/scale\(([\d.]+)\)/) || [0, 1])[1]);
          const fitOf = (el) => ({ sw: el.scrollWidth, cw: el.clientWidth });
          const caps = ['#presetPrev', '#presetNameText', '#presetNext', '#presetSave', '#presetDelete', '#chipTail .chip-head', '#chipBody', '.subtitle']
            .map((s) => [s, fitOf(q(s))]);
          const pr = q('#plugin').getBoundingClientRect(), g = q('#gear-btn').getBoundingClientRect();
          return { title: q('.title-block').getBoundingClientRect().width / sc, gear: +((g.left - pr.left) / sc).toFixed(2),
                   gearRight: (g.right - pr.left) / sc, strip: fitOf(q('.header-strip')), caps };
        });
        if (gear0 === null) gear0 = m.gear;
        const capsOk = m.caps.every(([, f]) => f.sw <= f.cw);
        const ok = Math.abs(m.title - TITLE_W) <= TOL && Math.abs(m.gear - gear0) <= TOL && m.gearRight <= 1180 - 16 && m.strip.sw <= m.strip.cw && capsOk;
        arm(`H2 ${tag}`, ok, `title ${m.title.toFixed(2)} · gear left ${m.gear} (first ${gear0}) right ${m.gearRight.toFixed(1)} · strip ${m.strip.sw}/${m.strip.cw} · `
          + (m.caps.filter(([, f]) => f.sw > f.cw).map(([s, f]) => `${s} ${f.sw}/${f.cw} OVERFLOWS`).join(', ') || `${m.caps.length} captions fit`));
      }

      // ── H3: the name box with every lesson and bank label, then a 64-char user name ──
      {
        const barGeom = () => page.evaluate(() => {
          const sc = parseFloat((document.getElementById('plugin').style.transform.match(/scale\(([\d.]+)\)/) || [0, 1])[1]);
          const lefts = ['#presetPrev', '#presetName', '#presetNext', '#presetSave', '#presetDelete']
            .map((s) => +(document.querySelector(s).getBoundingClientRect().left / sc).toFixed(2));
          const t = document.getElementById('presetNameText');
          return { lefts, w: +(document.getElementById('presetName').getBoundingClientRect().width / sc).toFixed(2), sw: t.scrollWidth, cw: t.clientWidth, text: t.textContent };
        });
        const rows = []; let ok = true, geom0 = null;
        const named = CATALOG.lessons.map((l) => ({ name: l.name, id: l.id })).concat(CATALOG.factory.map((n) => ({ name: n, id: '' })));
        for (const { name, id } of named) {
          await emit({ name, id, factory: true, modified: false });
          const g = await barGeom();
          if (!geom0) geom0 = g;
          const same = JSON.stringify(g.lefts) === JSON.stringify(geom0.lefts) && g.w === geom0.w;
          ok = ok && g.sw <= g.cw && same;
          if (!(g.sw <= g.cw) || !same) rows.push(`"${g.text}" ${g.sw}/${g.cw}${same ? '' : ' MOVED'}`);
        }
        await emit({ name: LONG_NAME, id: '', factory: false, modified: false });
        const gl = await barGeom();
        const longOk = gl.sw > gl.cw && gl.text === LONG_NAME && JSON.stringify(gl.lefts) === JSON.stringify(geom0.lefts) && gl.w === geom0.w;
        ok = ok && longOk;
        arm(`H3 ${tag}`, ok, `${named.length} labels fit (${rows.length ? rows.join('; ') : 'none ellipsised'}) · 64-char user name ${gl.sw}/${gl.cw} ${gl.sw > gl.cw ? 'ellipsised' : 'NOT ellipsised'} · box ${gl.w} px ${longOk ? 'unchanged' : 'MOVED'}`);
        await emit({ name: 'Init', id: 'init', factory: true, modified: false });
      }

      // ── H4: the ellipsis census over 9 page states ──
      {
        const over = new Map(); let counted = 0;
        for (const [label, p] of STATES) {
          await setParams(p);
          await page.waitForTimeout(60);
          const r = await page.evaluate(() => {
            const out = []; let n = 0;
            for (const el of document.querySelectorAll('#plugin *')) {
              if (el.closest('[hidden]') || el.offsetParent === null) continue;
              if (getComputedStyle(el).textOverflow !== 'ellipsis' || el.textContent.trim() === '') continue;
              if (el.id === 'presetNameText') continue;   // H3 owns it (a user name MAY ellipsise)
              n++;
              if (el.scrollWidth > el.clientWidth) out.push(`${el.id ? '#' + el.id : el.className || el.tagName} "${el.textContent.trim().slice(0, 48)}" ${el.scrollWidth}/${el.clientWidth}`);
            }
            return { n, out };
          });
          counted = Math.max(counted, r.n);
          for (const o of r.out) if (!over.has(o)) over.set(o, label);
        }
        arm(`H4 ${tag}`, over.size === 0 && counted >= 20,
          `${counted} ellipsis boxes × ${STATES.length} states` + (over.size ? ' — ' + [...over].map(([o, s]) => `${o} (${s})`).join('; ') : ', all fit'));
        await setParams(DEFAULTS);
      }
    }
    await page.close();
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
