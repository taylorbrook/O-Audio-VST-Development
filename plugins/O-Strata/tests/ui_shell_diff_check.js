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
/*
  ==============================================================================

    ui_shell_diff_check.js
    O-Strata — the inherited shell must render pixel-identical to O-Prism.

    Stage 3 Round A re-forked the page from O-Prism v1.26.0 (4f12ef57) and
    spliced the v2 mockup's NEW regions in (Terrain tab, oscillator cards, the
    terrain view). Everything else — Mod, Tuning, Effects, the header, the
    footer — is supposed to be O-Prism's, byte for byte in the JS regions that
    check-i18n [6] and the Task 5 range diffs compare, and PIXEL for pixel on
    screen. A byte diff cannot see a CSS rule that landed one selector too far,
    a dropped rule, or a card grid that no longer sums to the frame, so this
    gate measures the rendered page instead (pattern_ui_gate_asserts_attributes_
    never_rendered_geometry): two serve-ui instances (O-Prism from its working
    tree, which is byte-identical to 4f12ef57; O-Strata from Source/ui/public),
    one screenshot per inherited tab on each, and a canvas diff INSIDE a
    Playwright page — no pixelmatch, no pngjs, no new dependency.

    ── WHAT IS INJECTED ON THE O-PRISM SIDE ─────────────────────────────────
    The v2 mockup carries five INTENDED body edits to the shell (RESEARCH
    §2.1, PLAN Decision 21). They are added to the O-Prism page as a <style>
    tag before its screenshot, so the diff isolates the merge: a pixel that
    differs is a pixel the merge moved, not one the plan asked for.

    ── WHAT IS MASKED ───────────────────────────────────────────────────────
    Regions whose CONTENT differs by design, derived from BOTH pages' DOM
    (getBoundingClientRect, padded 2 px, unioned) at screenshot time, never
    typed as numbers: the plugin name (h1), the subtitle, the preset-browser
    row (the wider O-STRATA h1 moves it 2 px right — a space-between header —
    and its text comes from the stub's preset list), the whole tab bar (fifth
    tab reads Terrain, not Wavetable), the mod-matrix info line (46
    destinations, not 26), the dst select FACES on the Mod tab (the stub seeds
    each slot's default from a 46-name list instead of a 26-name one; the
    column boxes themselves measured identical to the pixel on both pages) and
    the two oscillator cards (the v2 cards: Terrain / Orbit selects, Orbit
    Size, the ⬡ / ≋ toggle). Anything outside those boxes must be identical.

    ── NEGATIVE CONTROLS ────────────────────────────────────────────────────
    A diff that reads 0 proves nothing until it is shown to read > 0 on a
    known difference (pattern_zipper_sweep_probe_needs_liveness_gate). Two:
    the O-Strata page against a second screenshot of itself (0), and a planted
    `.section-header{margin-left:1px}` on the O-Strata side against O-Prism
    (must be > 0 outside the masks, on every tab it is run on).

    Usage:  node plugins/O-Strata/tests/ui_shell_diff_check.js   (from the repo root)
    Exit 2 on any failure; exit 77 when Playwright cannot be resolved (never a
    green SKIP — this file is the only evidence the inherited tabs have).

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..', '..');
const S = require(path.join(repoRoot, 'scripts', 'serve-ui.js'));

const SHIP_W = 1200, SHIP_H = 800;
const TABS = ['synth', 'mod', 'tuning', 'effects'];

// The v2 mockup's intended body edits to the inherited shell (PLAN Decision 21).
const INTENDED_EDITS = [
    '.mod-col-dst{flex:1.35}',
    '.effect-section .section-header{margin-bottom:12px}',
    '.knob-value,.footer-value{font-variant-numeric:tabular-nums}',
    'html,body{cursor:default}',
    '.octave-stretch-slider{min-width:0}',
].join('\n');

// Mask sources: selectors whose rendered boxes (on the O-Strata page) are
// excluded from the comparison, with the reason each one differs by design.
const MASKS = [
    ['.header-bar h1',       'plugin name: O-STRATA vs O-PRISM'],
    ['.subtitle',            'label.subtitle: Wave-Terrain vs Wavetable (wider, so the space-between header re-spaces)'],
    ['.preset-browser',      'the whole browser row: the wider h1 shifts it 2 px right (space-between), and the display text comes from the stub list'],
    ['.tab-bar',             'fifth tab: Terrain vs Wavetable'],
    ['.mod-matrix-info',     'label.modMatrixInfo: 46 destinations vs 26'],
    ['.mod-col-dst select',  'the dst FACE text: the stub seeds each slot from a 46-name list vs a 26-name one (column boxes measured identical to the pixel)'],
    ['#card-oscA',           'v2 oscillator card A (Terrain / Orbit selects, Orbit Size, view toggle)'],
    ['#card-oscB',           'v2 oscillator card B'],
];

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}

async function openTab(page, name) {
    // Left-edge click: the settings popover hangs over the rightmost tab's right
    // half, and a centre click there is intercepted (the tip gate's rule).
    await page.click(`.tab[data-tab="${name}"]`, { position: { x: 16, y: 12 } });
    await page.waitForTimeout(300);
    await page.evaluate(() => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r))));
    const active = await page.evaluate(() => {
        const a = document.querySelector('.tab-content.active');
        return a ? a.id.replace('-tab', '') : null;
    });
    if (active !== name) throw new Error(`tab ${name} did not activate (active = ${active})`);
}

async function masksFor(page) {
    return page.evaluate((masks) => {
        const out = [];
        for (const [sel, why] of masks) {
            for (const el of document.querySelectorAll(sel)) {
                const r = el.getBoundingClientRect();
                if (r.width <= 0 || r.height <= 0) continue;   // display:none on this tab — nothing to mask
                out.push({ sel, why, x: Math.floor(r.left) - 2, y: Math.floor(r.top) - 2,
                           w: Math.ceil(r.width) + 4, h: Math.ceil(r.height) + 4 });
            }
        }
        return out;
    }, MASKS);
}

async function shot(page) {
    const buf = await page.screenshot({ type: 'png', fullPage: false });
    return 'data:image/png;base64,' + buf.toString('base64');
}

// Canvas diff in a scratch page: any channel difference counts; masked pixels are skipped.
async function diff(scratch, a, b, masks) {
    return scratch.evaluate(async ({ a, b, masks }) => {
        const load = (src) => new Promise((res, rej) => { const i = new Image(); i.onload = () => res(i); i.onerror = rej; i.src = src; });
        const ia = await load(a), ib = await load(b);
        if (ia.width !== ib.width || ia.height !== ib.height)
            return { error: `size mismatch ${ia.width}x${ia.height} vs ${ib.width}x${ib.height}` };
        const W = ia.width, H = ia.height;
        const cv = (img) => { const c = document.createElement('canvas'); c.width = W; c.height = H;
                              const g = c.getContext('2d', { willReadFrequently: true }); g.drawImage(img, 0, 0); return g.getImageData(0, 0, W, H).data; };
        const da = cv(ia), db = cv(ib);
        const masked = new Uint8Array(W * H);
        let maskedPx = 0;
        for (const m of masks)
            for (let y = Math.max(0, m.y); y < Math.min(H, m.y + m.h); y++)
                for (let x = Math.max(0, m.x); x < Math.min(W, m.x + m.w); x++)
                    if (!masked[y * W + x]) { masked[y * W + x] = 1; ++maskedPx; }
        let count = 0, inside = 0, minX = W, minY = H, maxX = -1, maxY = -1;
        for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
            const i = (y * W + x) * 4;
            if (da[i] === db[i] && da[i + 1] === db[i + 1] && da[i + 2] === db[i + 2] && da[i + 3] === db[i + 3]) continue;
            if (masked[y * W + x]) { ++inside; continue; }
            ++count;
            if (x < minX) minX = x; if (x > maxX) maxX = x; if (y < minY) minY = y; if (y > maxY) maxY = y;
        }
        return { W, H, count, inside, maskedPx, bbox: count ? { x: minX, y: minY, w: maxX - minX + 1, h: maxY - minY + 1 } : null };
    }, { a, b, masks });
}

(async () => {
    console.log(`ui_shell_diff_check — O-Strata inherited tabs vs O-Prism (working tree = 4f12ef57), ${SHIP_W}x${SHIP_H}, DPR 1, en`);

    const pw = S.resolvePlaywright();
    if (pw == null) {
        console.log('  FAIL: playwright not resolvable — install with `npx playwright install chromium`; the shell diff is NOT verified.');
        process.exit(77);
    }
    const { chromium } = pw;

    const builtP = S.buildRoot('O-Prism',  { repoRoot });
    const builtS = S.buildRoot('O-Strata', { repoRoot });
    console.log(`   O-Prism  ui root ${builtP.uiRootLabel}, stub=${builtP.stubKind}, seed=${builtP.seedFrom}`);
    console.log(`   O-Strata ui root ${builtS.uiRootLabel}, stub=${builtS.stubKind}, seed=${builtS.seedFrom}`);
    const missP = [], missS = [];
    const srvP = await S.serve(builtP.root, (rel) => missP.push(rel));
    const srvS = await S.serve(builtS.root, (rel) => missS.push(rel));

    const browser = await chromium.launch();
    const ctx = await browser.newContext({ viewport: { width: SHIP_W, height: SHIP_H }, deviceScaleFactor: 1 });
    const pageP = await ctx.newPage(), pageS = await ctx.newPage(), scratch = await ctx.newPage();
    const errs = { P: [], S: [] };
    pageP.on('pageerror', (e) => errs.P.push(String(e && e.message ? e.message : e)));
    pageS.on('pageerror', (e) => errs.S.push(String(e && e.message ? e.message : e)));

    await pageP.goto(`http://127.0.0.1:${srvP.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
    await pageS.goto(`http://127.0.0.1:${srvS.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
    await scratch.setContent('<!doctype html><html><body></body></html>');
    await pageP.waitForTimeout(700);
    await pageS.waitForTimeout(700);
    await pageP.addStyleTag({ content: INTENDED_EDITS });
    console.log(`   injected on O-Prism: ${INTENDED_EDITS.split('\n').join('  ')}`);

    check(errs.P.length === 0, `O-Prism page boots without pageerror${errs.P.length ? ': ' + errs.P.join(' | ') : ''}`);
    check(errs.S.length === 0, `O-Strata page boots without pageerror${errs.S.length ? ': ' + errs.S.join(' | ') : ''}`);

    const shotsS = {}, shotsP = {};
    console.log('\n-- inherited tabs: differing pixels OUTSIDE the masks (must be 0)');
    for (const tab of TABS) {
        await openTab(pageP, tab);
        await openTab(pageS, tab);
        // Union of BOTH pages' boxes: a masked element that sits 2 px further right
        // on one page (the header re-spacing) must be excluded on both.
        const masks = [...await masksFor(pageS), ...await masksFor(pageP)];
        shotsP[tab] = await shot(pageP);
        shotsS[tab] = await shot(pageS);
        const d = await diff(scratch, shotsP[tab], shotsS[tab], masks);
        if (d.error) { check(false, `[${tab}] ${d.error}`); continue; }
        const maskList = masks.map((m) => `${m.sel} ${m.w}x${m.h}@${m.x},${m.y}`).join(', ');
        console.log(`   [${tab}] masks (${masks.length}, ${d.maskedPx} px): ${maskList}`);
        console.log(`   [${tab}] differing px inside masks ${d.inside}, outside ${d.count}${d.bbox ? ` bbox ${d.bbox.w}x${d.bbox.h}@${d.bbox.x},${d.bbox.y}` : ''}`);
        check(d.count === 0, `[${tab}] 0 differing pixels outside the masks (got ${d.count}${d.bbox ? `, bbox ${d.bbox.w}x${d.bbox.h}@${d.bbox.x},${d.bbox.y}` : ''})`);
    }

    console.log('\n-- negative controls');
    // (a) O-Strata against a second screenshot of itself, same tab: 0.
    await openTab(pageS, 'effects');
    const again = await shot(pageS);
    const self = await diff(scratch, shotsS.effects, again, []);
    check(!self.error && self.count === 0, `self-diff (O-Strata effects vs itself, no masks) = ${self.error || self.count}`);
    // (b) a planted 1 px shift on O-Strata's section headers must show up outside the masks.
    await pageS.addStyleTag({ content: '.section-header{margin-left:1px}' });
    await pageS.waitForTimeout(150);
    await pageS.evaluate(() => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r))));
    const plantedMasks = [...await masksFor(pageS), ...await masksFor(pageP)];
    const planted = await diff(scratch, shotsP.effects, await shot(pageS), plantedMasks);
    check(!planted.error && planted.count > 0, `planted .section-header{margin-left:1px} on effects → ${planted.error || planted.count} differing px outside the masks (must be > 0; the probe is live)`);

    if (missP.length) console.log(`   note: O-Prism 404s: ${[...new Set(missP)].join(', ')}`);
    if (missS.length) console.log(`   note: O-Strata 404s: ${[...new Set(missS)].join(', ')}`);

    await browser.close();
    await srvP.close(); await srvS.close();
    fs.rmSync(builtP.root, { recursive: true, force: true });
    fs.rmSync(builtS.root, { recursive: true, force: true });

    console.log(`\n${failed === 0 ? 'ALL CHECKS PASSED' : failed + ' FAILED'}`);
    process.exit(failed === 0 ? 0 : 2);
})().catch((e) => { console.error('ui_shell_diff_check: ' + (e && e.stack ? e.stack : e)); process.exit(2); });
