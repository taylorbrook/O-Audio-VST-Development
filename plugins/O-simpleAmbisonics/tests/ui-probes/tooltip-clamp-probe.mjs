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
// tooltip-clamp-probe.mjs — the hover-help card stays inside the WINDOW at the
// two sizes the editor really takes (Stage 3 C20, UI-06 / UI-07).
//
// WHY TWO VIEWPORTS. Whether the edge clamp fires depends entirely on the
// window size (memory pattern_tooltip_clamp_gate_viewport_sensitive): a gate run
// in a roomy browser passes while the shipping window overflows. This editor is
// resizable 0.6×–1.5× at a fixed aspect, and the page scales ITSELF by a CSS
// transform while the tooltip — a body child outside #plugin — does NOT scale.
// So the worst case is the 0.6× floor (708 × 468): the anchors shrink toward
// each other and the edges while the 300 px card keeps its size. Both the
// design size (1180 × 780) and the floor are measured, and innerWidth /
// innerHeight are asserted to equal the requested viewport before anything is
// measured (a mismatched viewport would gate the wrong window).
//
// WHAT IS ASSERTED, per viewport × language × anchor (azimuth knob, Output Level
// knob, the gear, the last lesson button — the left, right-middle, top-right
// and bottom-right extremes of the page):
//   1. the tip became visible (the dwell is awaited by polling .visible);
//   2. its box lies inside the window with ≥ 4 px on every side;
//   3. it ends at its NATURAL width — the shrink-to-fit trap: a fixed box
//      measured at its previous offset collapses to the space left of the edge
//      and re-wraps into a ribbon (memory pattern_fixed_tooltip_shrink_to_fit_edge).
//      The natural width is re-measured on a detached clone at left 0, and the
//      card must match it within 1 px and never exceed .tooltip's max-width.
// VACUITY: the right-edge clamp must ENGAGE at least once per viewport (the gear
// and the last lesson button sit at the right edge), or the edge arm proved
// nothing.
//
// The hover-help layer is forced ON through localStorage['osamb.tipsEnabled']
// before the page loads (the switch persists its state there).
//
// Exit = number of failed arms; 77 = Playwright unresolvable — NOTHING was
// verified, never a pass.
// Usage: node plugins/O-simpleAmbisonics/tests/ui-probes/tooltip-clamp-probe.mjs
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

const VIEWPORTS = [{ w: 1180, h: 780 }, { w: 708, h: 468 }];
const LANGS     = ['en', 'fr', 'zh-Hans'];
const ANCHORS   = [
  ['azimuth knob',      '[data-param="azimuth"]'],
  ['Output Level knob', '[data-param="outputLevel"]'],
  ['gear',              '#gear-btn'],
  ['last lesson',       '#lesson-speakersHeadphones'],
];
const EDGE = 4;

let failed = 0;
function arm(ok, what) {
  console.log(`  ${ok ? 'PASS' : 'FAIL'} ${what}`);
  if (!ok) ++failed;
}

(async () => {
  const pw = S.resolvePlaywright();
  if (!pw) { console.error('tooltip-clamp-probe: Playwright unresolvable — NOTHING was verified (exit 77, never a pass).'); process.exit(77); }
  const es = S.readEditorSize(NAME);
  if (es.w !== VIEWPORTS[0].w || es.h !== VIEWPORTS[0].h) {
    console.error(`tooltip-clamp-probe: the editor is ${es.w}×${es.h}, this probe was written for 1180×780 — update VIEWPORTS (exit 6)`);
    process.exit(6);
  }

  const built = S.buildRoot(NAME);
  const srv = await S.serve(built.root);
  let browser;
  try { browser = await pw.chromium.launch(); }
  catch (e) {
    console.error(`tooltip-clamp-probe: chromium did not launch — ${e.message} (exit 77)`);
    await srv.close(); process.exit(77);
  }
  console.log(`seed=${built.seedFrom}`);
  if (built.seedFrom !== 'overrides') { console.error('tooltip-clamp-probe: stub not seeded from generic-overrides.json (exit 6)'); process.exit(6); }

  for (const vp of VIEWPORTS) {
    const ctx = await browser.newContext({ viewport: { width: vp.w, height: vp.h } });
    await ctx.addInitScript(() => { try { localStorage.setItem('osamb.tipsEnabled', 'true'); } catch (e) { /* no storage */ } });
    const page = await ctx.newPage();
    const errors = [];
    page.on('pageerror', (e) => errors.push(String(e && e.message ? e.message : e)));
    await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
    await page.waitForFunction(() => typeof window.__setLanguage === 'function', null, { timeout: 10000 });
    await page.evaluate(() => document.fonts.ready);
    await page.waitForTimeout(300);

    const env = await page.evaluate(() => ({
      iw: innerWidth, ih: innerHeight, title: document.title,
      tips: document.getElementById('tips-toggle').getAttribute('aria-pressed'),
      maxW: parseFloat(getComputedStyle(document.getElementById('tooltip')).maxWidth),
      scale: document.getElementById('plugin').style.transform || 'none',
    }));
    console.log(`\n== viewport ${vp.w}×${vp.h}: innerWidth ${env.iw} × innerHeight ${env.ih} · page transform ${env.scale} · hover help ${env.tips} · .tooltip max-width ${env.maxW}px`);
    const envOk = env.iw === vp.w && env.ih === vp.h && /O-simpleAmbisonics/.test(env.title) && env.tips === 'true' && env.maxW > 0;
    arm(envOk, `[${vp.w}×${vp.h}] window is the requested size, the right page, hover help ON`);
    if (!envOk) { await ctx.close(); continue; }

    let clampedRight = 0;
    for (const lang of LANGS) {
      await page.evaluate((l) => window.__setLanguage(l), lang);
      await page.waitForTimeout(150);
      for (const [label, sel] of ANCHORS) {
        // Leave whatever is hovered, then hover the anchor's centre.
        await page.mouse.move(vp.w / 2, 2);
        await page.waitForFunction(() => !document.getElementById('tooltip').classList.contains('visible'), null, { timeout: 2000 }).catch(() => {});
        const box = await page.locator(sel).first().boundingBox();
        if (!box) { arm(false, `[${vp.w}×${vp.h}][${lang}] ${label}: anchor ${sel} has a box`); continue; }
        await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
        const shown = await page.waitForFunction(() => document.getElementById('tooltip').classList.contains('visible'), null, { timeout: 3000 })
          .then(() => true).catch(() => false);
        if (!shown) { arm(false, `[${vp.w}×${vp.h}][${lang}] ${label}: the tip appeared`); continue; }
        const m = await page.evaluate(() => {
          const t = document.getElementById('tooltip');
          const r = t.getBoundingClientRect();
          // Natural width: a detached clone, unpinned, at the left edge.
          const c = t.cloneNode(true);
          c.style.width = ''; c.style.left = '0px'; c.style.top = '0px'; c.style.visibility = 'hidden';
          document.body.appendChild(c);
          const natural = c.getBoundingClientRect().width;
          c.remove();
          return { l: r.left, r: r.right, t: r.top, b: r.bottom, w: r.width, natural,
                   title: (t.querySelector('.tooltip-title') || {}).textContent || '', placement: t.dataset.placement };
        });
        const inside = m.l >= EDGE - 0.01 && m.r <= vp.w - EDGE + 0.01 && m.t >= EDGE - 0.01 && m.b <= vp.h - EDGE + 0.01;
        const natural = Math.abs(m.w - m.natural) <= 1 && m.w <= env.maxW + 0.5;
        if (m.r >= vp.w - 8 - 0.5) ++clampedRight;
        arm(inside && natural,
            `[${vp.w}×${vp.h}][${lang}] ${label} "${m.title}": box ${m.l.toFixed(1)}–${m.r.toFixed(1)} × ${m.t.toFixed(1)}–${m.b.toFixed(1)} (${m.placement}), `
            + `width ${m.w.toFixed(1)} vs natural ${m.natural.toFixed(1)} (max ${env.maxW})`);
      }
    }
    arm(clampedRight > 0, `[${vp.w}×${vp.h}] vacuity: the right-edge clamp engaged ${clampedRight}× (must be ≥ 1)`);
    arm(errors.length === 0, `[${vp.w}×${vp.h}] no page error (${errors.length}${errors.length ? ': ' + errors[0] : ''})`);
    await ctx.close();
  }

  await browser.close(); await srv.close();
  fs.rmSync(built.root, { recursive: true, force: true });
  console.log(failed ? `\n${failed} FAILED` : '\nALL PASS');
  process.exit(failed);
})().catch((e) => { console.error(e); process.exit(70); });
