/*
   This file is part of O-ParametricEQ, an Ouaricon Audio plugin.
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

    preset_name_fit.js — PN1 / PN1n (Stage 4, PLAN-4.1 task 19; a port of
    O-GranulatorInstrument tests/preset_name_fit.js, which has a menu this
    plugin does not)

    The factory preset names (13; PARSED from Source/FactoryPresets.h, so a
    renamed preset is re-checked) must fit the preset bar's name box,
    #preset-name (flex: 1, nowrap + ellipsis), at the SMALLEST window,
    800 x 500, in en / fr / zh-Hans: scrollWidth <= clientWidth for every
    name. The names are not localised, but the page font stack can differ per
    language. Real page, served stub, the embedded fonts loaded first.

    PN1n: a 40-character name in the same box must NOT fit — the gate is live.

    Usage:  node plugins/O-ParametricEQ/tests/preset-name-fit/preset_name_fit.js
    Exit 0 = all fit and PN1n overflows; 1 = a failure; 77 = Playwright
    unavailable (NOTHING verified — not a pass).

  ==============================================================================
*/
'use strict';
const fs = require('fs');
const path = require('path');

const pluginRoot = path.resolve(__dirname, '..', '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const S = require(path.join(repoRoot, 'scripts', 'serve-ui.js'));

const EXPECTED = 13;

// Every name a def is declared with: preset ("<name>") in factoryPresetSpecs().
function presetNames() {
    const src = fs.readFileSync(path.join(pluginRoot, 'Source', 'FactoryPresets.h'), 'utf8');
    const re = /\bpreset \("([^"]+)"\)/g;
    const out = [];
    let m;
    while ((m = re.exec(src)) !== null) out.push(m[1]);
    return out;
}

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) {
        console.log('preset_name_fit: Playwright could not be resolved — NOTHING verified (exit 77 is not a pass).');
        process.exit(77);
    }
    const names = presetNames();
    console.log(`== O-ParametricEQ preset_name_fit (PN1 / PN1n) — ${names.length} names ==`);
    let failures = 0;
    const fail = (msg) => { ++failures; console.log(`  FAIL: ${msg}`); };
    if (names.length !== EXPECTED) fail(`parsed ${names.length} names from FactoryPresets.h (want ${EXPECTED})`);

    // repoRoot from this script's own location: a scratch copy of the plugin
    // serves its own page (serve-ui.js resolves its symlinked __dirname to the
    // real repo otherwise; VERIFICATION-4.1 Issue 4). In-repo, the same root.
    const built = S.buildRoot('O-ParametricEQ', { repoRoot });
    const srv = await S.serve(built.root);
    const browser = await pw.chromium.launch();
    const page = await browser.newPage({ viewport: { width: 800, height: 500 } });
    const errors = [];
    page.on('pageerror', (e) => errors.push(`pageerror: ${e.message}`));
    await page.goto(`http://127.0.0.1:${srv.port}/`);
    await page.waitForFunction(() => typeof window.__setLanguage === 'function' && document.getElementById('preset-name'), null, { timeout: 15000 });
    await page.evaluate(() => document.fonts.ready);
    await page.waitForTimeout(200);

    const measure = (list) => page.evaluate((ns) => {
        const b = document.getElementById('preset-name');
        const keep = b.textContent;
        const out = [];
        for (const n of ns) {
            b.textContent = n;
            const rg = document.createRange();
            rg.selectNodeContents(b);
            const cs = getComputedStyle(b);
            // text: the glyph run itself plus the box's own padding, for the "widest" readout
            const text = Math.ceil(rg.getBoundingClientRect().width + parseFloat(cs.paddingLeft) + parseFloat(cs.paddingRight));
            out.push({ n, sw: b.scrollWidth, cw: b.clientWidth, text });
        }
        b.textContent = keep;
        return out;
    }, list);

    for (const lang of ['en', 'fr', 'zh-Hans']) {
        await page.evaluate((l) => window.__setLanguage(l), lang);
        await page.evaluate(() => document.fonts.ready);
        const r = await measure(names);
        const worst = r.reduce((a, x) => (x.text > a.text ? x : a), r[0]);
        const over = r.filter((x) => x.sw > x.cw);
        console.log(`  ${over.length ? 'FAIL' : 'PASS'}: PN1 ${lang}: ${names.length} names in #preset-name at 800 x 500 — widest "${worst.n}" ${worst.text} / ${worst.cw} px (text + padding)`);
        if (over.length) fail(`PN1 ${lang}: ${over.map((x) => `"${x.n}" ${x.sw}/${x.cw}`).join(', ')}`);
    }
    await page.evaluate(() => window.__setLanguage('en'));
    const nc = (await measure(['A Forty Character Preset Name For Gating']))[0];
    const ncFails = nc.sw > nc.cw;
    console.log(`  ${ncFails ? 'PASS' : 'FAIL'}: PN1n a ${nc.n.length}-character name overflows the box (${nc.sw} / ${nc.cw} px) — the gate is live`);
    if (!ncFails) fail('PN1n: the 40-character name fits — the gate is vacuous');
    if (errors.length) fail(`page errors: ${errors.slice(0, 3).join(' | ')}`);

    await browser.close();
    await srv.close();
    console.log(failures === 0 ? 'ALL PASS' : `${failures} FAILURE(S)`);
    process.exit(failures === 0 ? 0 : 1);
})().catch((e) => { console.log(`preset_name_fit crashed: ${e.stack || e}`); process.exit(1); });
