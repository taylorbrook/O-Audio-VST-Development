/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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

    preset_name_fit.js — PF1 / PF1n (Stage 4, PLAN-4 P160, UI-04)

    The 16 factory preset names (parsed from Source/FactoryPresets.h) must fit
    the preset bar's name box, #preset-name-text (.pname b, nowrap + ellipsis),
    in EN / FR / zh-Hans: scrollWidth <= clientWidth for every name (the
    names are not localised, but the box font can differ per language). Real
    page, served stub, the embedded fonts loaded first. The preset MENU items
    (.pm-item) are measured too and must fit their 300 px list.

    PF1n: a 40-character name in the same box must NOT fit — the gate is live.

    Usage:  node plugins/O-Granulator/tests/preset_name_fit.js
    Exit 0 = all fit and PF1n overflows; 1 = a failure; 77 = Playwright
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
    const re = /\{ "((?:Ambient|Harmoniser|Rhythmic|Cascade) \\xc2\\xb7 [^"]+)",/g;
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
    console.log(`== O-Granulator preset_name_fit (PF1 / PF1n) — ${names.length} names ==`);
    let failures = 0;
    const fail = (msg) => { ++failures; console.log(`  FAIL: ${msg}`); };
    if (names.length !== 16) fail(`parsed ${names.length} names from FactoryPresets.h (want 16)`);

    const built = S.buildRoot('O-Granulator');
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
        // menu items: the page's own item class in the real list box
        const menu = document.getElementById('preset-menu');
        const wasHidden = menu.hidden;
        menu.hidden = false;
        const items = [];
        for (const n of ns) {
            const it = document.createElement('button');
            it.className = 'pm-item';
            it.textContent = n;
            it.style.whiteSpace = 'nowrap';
            menu.appendChild(it);
            items.push({ n, sw: it.scrollWidth, cw: it.clientWidth });
            it.remove();
        }
        menu.hidden = wasHidden;
        return { box: out, menu: items };
    }, list);

    for (const lang of ['en', 'fr', 'zh-Hans']) {
        await page.evaluate((l) => window.__setLanguage(l), lang);
        await page.evaluate(() => document.fonts.ready);
        const r = await measure(names);
        const worst = r.box.reduce((a, x) => (x.sw > a.sw ? x : a), r.box[0]);   // the box is content-sized up to its 172 px max
        const over = r.box.filter((x) => x.sw > x.cw);
        const menuOver = r.menu.filter((x) => x.sw > x.cw);
        console.log(`  ${over.length ? 'FAIL' : 'PASS'}: PF1 ${lang}: 16 names in .pname b — widest "${worst.n}" ${worst.sw} / ${worst.cw} px`);
        if (over.length) fail(`PF1 ${lang}: ${over.map((x) => `"${x.n}" ${x.sw}/${x.cw}`).join(', ')}`);
        console.log(`  ${menuOver.length ? 'FAIL' : 'PASS'}: PF1 ${lang}: 16 names in .pm-item — ${menuOver.length} overflow`);
        if (menuOver.length) fail(`PF1 menu ${lang}: ${menuOver.map((x) => `"${x.n}" ${x.sw}/${x.cw}`).join(', ')}`);
    }
    await page.evaluate(() => window.__setLanguage('en'));
    const nc = await measure(['Harmoniser · A Forty Character Preset Nm']);
    const ncName = nc.box[0];
    const ncFails = ncName.sw > ncName.cw;
    console.log(`  ${ncFails ? 'PASS' : 'FAIL'}: PF1n a ${ncName.n.length}-character name overflows the box (${ncName.sw} / ${ncName.cw} px) — the gate is live`);
    if (!ncFails) fail('PF1n: the 40-character name fits — the gate is vacuous');
    if (errors.length) fail(`page errors: ${errors.slice(0, 3).join(' | ')}`);

    await browser.close();
    await srv.close();
    console.log(failures === 0 ? 'ALL PASS' : `${failures} FAILURE(S)`);
    process.exit(failures === 0 ? 0 : 1);
})().catch((e) => { console.log(`preset_name_fit crashed: ${e.stack || e}`); process.exit(1); });
