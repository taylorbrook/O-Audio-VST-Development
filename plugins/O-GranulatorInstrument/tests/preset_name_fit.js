/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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

    preset_name_fit.js — PN1 / PN1n (Stage 4, PLAN-4 PD13 / T15; a port of
    O-Granulator's PF1 / PF1n — "PF" is taken in this plugin's harness)

    The factory preset names (160 at 1.3.0; parsed from Source/FactoryPresets.h) must fit
    the preset bar's name box, #preset-name-text (.pname b, nowrap + ellipsis),
    in EN / FR / zh-Hans: scrollWidth <= clientWidth for every name (the
    names are not localised, but the box font can differ per language). Real
    page, served stub, the embedded fonts loaded first. 1.3.0 (T11): the
    two-pane preset MENU shows the SHORT name (the "<Cat> · " prefix dropped)
    in a .pm-p row of the right listbox (#pm-presets, 310 px pane); every short
    name must fit that row, and every group header (with its count) must fit
    its 150 px left-pane row, in each language.

    PN1n: a 40-character name in the same box must NOT fit — the gate is live.
    PN1n2: an 82-character short name in a .pm-p row must NOT fit.

    Usage:  node plugins/O-GranulatorInstrument/tests/preset_name_fit.js
    Exit 0 = all fit and PN1n overflows; 1 = a failure; 77 = Playwright
    unavailable (NOTHING verified — not a pass).

  ==============================================================================
*/
'use strict';
const fs = require('fs');
const path = require('path');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const S = require(path.join(repoRoot, 'scripts', 'serve-ui.js'));

function presetNames() {
    const src = fs.readFileSync(path.join(pluginRoot, 'Source', 'FactoryPresets.h'), 'utf8');
    const re = /\{ "((?:Drones|Expressive|Field|Keys|Live|Motion|Pads|Rhythmic|Textures) \\xc2\\xb7 [^"]+)",/g;
    const out = [];
    let m;
    while ((m = re.exec(src)) !== null) out.push(m[1].replace(/\\xc2\\xb7/g, '·'));
    return out;
}

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) {
        console.log('preset_name_fit: Playwright could not be resolved — NOTHING verified (exit 77 is not a pass).');
        process.exit(77);
    }
    const names = presetNames();
    console.log(`== O-GranulatorInstrument preset_name_fit (PN1 / PN1n) — ${names.length} names ==`);
    let failures = 0;
    const fail = (msg) => { ++failures; console.log(`  FAIL: ${msg}`); };
    if (names.length !== 160) fail(`parsed ${names.length} names from FactoryPresets.h (want 160)`);   // 1.3.0: kBankSize

    const built = S.buildRoot('O-GranulatorInstrument');
    const srv = await S.serve(built.root);
    const browser = await pw.chromium.launch();
    const page = await browser.newPage({ viewport: { width: 1200, height: 800 } });
    const errors = [];
    page.on('pageerror', (e) => errors.push(`pageerror: ${e.message}`));
    await page.goto(`http://127.0.0.1:${srv.port}/`);
    await page.waitForFunction(() => window.__stubStates && document.getElementById('preset-name-text'), null, { timeout: 15000 });
    await page.evaluate(() => document.fonts.ready);
    await page.waitForTimeout(200);

    const measure = (list) => page.evaluate((ns) => {
        const b = document.getElementById('preset-name-text');
        const keep = b.textContent;
        const out = [];
        for (const n of ns) {
            b.textContent = n;
            out.push({ n, sw: b.scrollWidth, cw: b.clientWidth });
        }
        b.textContent = keep;
        // menu rows: the page's own row class in the real right-pane listbox, SHORT name
        const menu = document.getElementById('preset-menu'), pane = document.getElementById('pm-presets');
        const wasHidden = menu.hidden;
        menu.hidden = false;
        const items = [];
        const dot = ' \u00b7 ';
        for (const n of ns) {
            const it = document.createElement('div');
            it.className = 'pm-p';
            it.textContent = n.includes(dot) ? n.slice(n.indexOf(dot) + dot.length) : n;
            pane.appendChild(it);
            items.push({ n: it.textContent, sw: it.scrollWidth, cw: it.clientWidth });
            it.remove();
        }
        // group headers (localized) with their count badge, in the 150 px left pane
        const heads = [...document.querySelectorAll('#pm-groups .pm-g')].map((g) => {
            const sp = g.querySelector('span[data-i18n]'), c = g.querySelector('.n'), keep = c.textContent;
            c.textContent = '20';
            const gb = g.getBoundingClientRect(), sb = sp.getBoundingClientRect(), nb = c.getBoundingClientRect();
            const r = { n: sp.textContent, w: sb.width, over: g.scrollWidth > g.clientWidth || sb.right > nb.left - 4 || nb.right > gb.right - 4 };
            c.textContent = keep;
            return r;
        });
        menu.hidden = wasHidden;
        return { box: out, menu: items, heads };
    }, list);

    for (const lang of ['en', 'fr', 'zh-Hans']) {
        await page.evaluate((l) => window.__setLanguage(l), lang);
        await page.evaluate(() => document.fonts.ready);
        const r = await measure(names);
        const worst = r.box.reduce((a, x) => (x.sw > a.sw ? x : a), r.box[0]);   // the box is content-sized up to its 172 px max
        const over = r.box.filter((x) => x.sw > x.cw);
        const menuOver = r.menu.filter((x) => x.sw > x.cw);
        const headOver = r.heads.filter((x) => x.over);
        const widestShort = r.menu.reduce((a, x) => (x.sw > a.sw ? x : a), r.menu[0]);
        const widestHead = r.heads.reduce((a, x) => (x.w > a.w ? x : a), r.heads[0]);
        console.log(`  ${over.length ? 'FAIL' : 'PASS'}: PN1 ${lang}: ${names.length} names in .pname b — widest "${worst.n}" ${worst.sw} / ${worst.cw} px`);
        if (over.length) fail(`PN1 ${lang}: ${over.map((x) => `"${x.n}" ${x.sw}/${x.cw}`).join(', ')}`);
        console.log(`  ${menuOver.length ? 'FAIL' : 'PASS'}: PN1 ${lang}: ${names.length} short names in a #pm-presets .pm-p row — ${menuOver.length} overflow (row ${widestShort.cw} px)`);
        if (menuOver.length) fail(`PN1 menu ${lang}: ${menuOver.map((x) => `"${x.n}" ${x.sw}/${x.cw}`).join(', ')}`);
        console.log(`  ${headOver.length ? 'FAIL' : 'PASS'}: PN1 ${lang}: ${r.heads.length} group headers + count fit their left-pane row — widest "${widestHead.n}" ${widestHead.w.toFixed(1)} px`);
        if (headOver.length || r.heads.length !== 10) fail(`PN1 heads ${lang}: ${r.heads.length} headers, over: ${headOver.map((x) => x.n).join(', ')}`);
    }
    await page.evaluate(() => window.__setLanguage('en'));
    const nc = await measure(['Expressive · A Forty Character Preset Na']);
    const ncName = nc.box[0];
    const ncFails = ncName.sw > ncName.cw;
    console.log(`  ${ncFails ? 'PASS' : 'FAIL'}: PN1n a ${ncName.n.length}-character name overflows the box (${ncName.sw} / ${ncName.cw} px) — the gate is live`);
    if (!ncFails) fail('PN1n: the 40-character name fits — the gate is vacuous');
    const nc2 = (await measure(['Pads \u00b7 An Eighty Character Short Preset Name That Can Never Fit The Right Pane Row At All'])).menu[0];
    const nc2Fails = nc2.sw > nc2.cw;
    console.log(`  ${nc2Fails ? 'PASS' : 'FAIL'}: PN1n2 a ${nc2.n.length}-character short name overflows its .pm-p row (${nc2.sw} / ${nc2.cw} px) — the menu gate is live`);
    if (!nc2Fails) fail('PN1n2: the long short name fits — the menu gate is vacuous');
    if (errors.length) fail(`page errors: ${errors.slice(0, 3).join(' | ')}`);

    await browser.close();
    await srv.close();
    console.log(failures === 0 ? 'ALL PASS' : `${failures} FAILURE(S)`);
    process.exit(failures === 0 ? 0 : 1);
})().catch((e) => { console.log(`preset_name_fit crashed: ${e.stack || e}`); process.exit(1); });
