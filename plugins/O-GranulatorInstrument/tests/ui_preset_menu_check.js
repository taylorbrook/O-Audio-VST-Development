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

    ui_preset_menu_check.js — the two-pane preset menu in a real page (1.3.0,
    preset-bank-expansion T11; a port of ui_factory_picker_check.js)

    ui_frontend_check.js [18] is the static half; this file needs a layout
    engine. Real page, served stub (scripts/serve-ui.js), the 160-preset
    getPresetMenu of tests/ui-stub/generic-overrides.json (dumped by the render
    harness through GiUiJson::presetMenuToVar), embedded fonts loaded first.
    A second page routes the stub preamble to add two user presets (one named
    "Pads · Mine"), a current preset (Rhythmic · Sub Pump) and a loadPreset
    that succeeds.

      PM1  content: 10 groups in menu order; every group's rows == the
           fixture's (full name in data-name, the short name shown), counts
           20 x 7 / Field 12 / Live 8 / User; the count badge; users only in
           User, with their full name (a user "Pads · Mine" is not in Pads).
      PM2  geometry, per language (en / fr / zh-Hans) and per group: the box
           inside the 1200 x 800 frame (15 px margin, bottom 785); a group of
           <= 20 presets shows without a scrollbar; every row is the top
           element at its own centre; every short name, group header (with its
           count) and the file row fit their box (nowrap).
      PM3  keyboard: open on a preset outside the bank -> the groups listbox
           holds focus on Drones; Down walks the 10 groups then the file row
           and wraps; Up from Drones -> the file row; Home / End; Right -> the
           presets listbox; Down / Up wrap over the rows; Left back; Enter on a
           group goes right; Enter on a row calls loadPreset with the FULL name
           and closes (focus back on the preset name); Enter on the file row
           calls loadPresetFromFile; Escape and Tab close;
           aria-activedescendant names the highlighted option after every move.
      PM4  open selects the current preset's group and its row (.sel,
           aria-selected); hover on a group switches the right pane; a click
           on a row loads it and the bar shows the full name; a press outside
           closes; the name toggles.

    Controls (each must FAIL its predicate): a fixture missing a group, and one
    with the user preset merged into Pads, fail PM1; a 60-character short name
    overflows its row; a 100 px pane scrolls; the box at top 700 px leaves the
    frame.

    Usage:  node plugins/O-GranulatorInstrument/tests/ui_preset_menu_check.js [--shots DIR]
    --shots DIR writes preset-menu-en.png / -fr.png / -zh-Hans.png (Pads open).
    Exit 0 = all pass and every control fired; 1 = a failure; 77 = Playwright
    unavailable (NOTHING verified — not a pass).

  ==============================================================================
*/
'use strict';
const fs = require('fs');
const path = require('path');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const S = require(path.join(repoRoot, 'scripts', 'serve-ui.js'));

const GROUPS = ['drones', 'expressive', 'field', 'keys', 'live', 'motion', 'pads', 'rhythmic', 'textures', 'user'];
const COUNTS = { drones: 20, expressive: 20, field: 12, keys: 20, live: 8, motion: 20, pads: 20, rhythmic: 20, textures: 20 };
const PREFIX = { drones: 'Drones', expressive: 'Expressive', field: 'Field', keys: 'Keys', live: 'Live', motion: 'Motion', pads: 'Pads', rhythmic: 'Rhythmic', textures: 'Textures' };
const DOT = ' \u00b7 ';
const shotsAt = process.argv.indexOf('--shots');
const shotsDir = shotsAt > 0 ? path.resolve(process.argv[shotsAt + 1]) : null;

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) {
        console.log('ui_preset_menu_check: Playwright could not be resolved — NOTHING verified (exit 77 is not a pass).');
        process.exit(77);
    }
    const fixture = JSON.parse(fs.readFileSync(path.join(pluginRoot, 'tests', 'ui-stub', 'generic-overrides.json'), 'utf8')).natives.getPresetMenu;
    const total = Array.isArray(fixture) ? fixture.reduce((k, g) => k + g.presets.length, 0) : 0;
    console.log(`== O-GranulatorInstrument ui_preset_menu_check — ${total} presets in the fixture ==`);
    let failures = 0;
    const check = (ok, msg) => { if (!ok) ++failures; console.log(`  ${ok ? 'PASS' : 'FAIL'}: ${msg}`); return ok; };

    // The second page's fixture: what presetMenuToVar answers with two user presets on disk.
    const userFixture = fixture.map((g) => (g.group === 'user'
        ? { group: 'user', presets: [{ name: 'My Patch', short: 'My Patch' }, { name: `Pads${DOT}Mine`, short: `Pads${DOT}Mine` }] } : g));
    const CURRENT = `Rhythmic${DOT}Sub Pump`;

    const built = S.buildRoot('O-GranulatorInstrument');
    const srv = await S.serve(built.root);
    const browser = await pw.chromium.launch();
    const errors = [];
    const newPage = async (extra) => {
        const page = await browser.newPage({ viewport: { width: 1200, height: 800 } });
        page.on('pageerror', (e) => errors.push(`pageerror: ${e.message}`));
        if (extra) {
            await page.route('**/js/juce/stub-preamble.js', async (route) => {
                const resp = await route.fetch();
                const body = (await resp.text()) + `\n(function () { const n = window.__stubOverrides.natives; ${extra} })();\n`;
                await route.fulfill({ response: resp, body, headers: { ...resp.headers(), 'content-type': 'text/javascript' } });
            });
        }
        await page.goto(`http://127.0.0.1:${srv.port}/`);
        await page.waitForFunction(() => window.__stubStates && document.querySelectorAll('#pm-groups .pm-g').length === 10, null, { timeout: 15000 });
        await page.evaluate(() => document.fonts.ready);
        await page.waitForTimeout(300);
        return page;
    };
    const page = await newPage(null);

    const isOpen = (p) => p.evaluate(() => !document.getElementById('preset-menu').hidden);
    const open = async (p) => {
        for (let k = 0; k < 20 && !(await isOpen(p)); k++) {            // the bar is wired once the module has initialized
            await p.evaluate(() => document.getElementById('preset-name').click());
            await p.waitForTimeout(150);
        }
        await p.waitForFunction(() => !document.getElementById('preset-menu').hidden && document.querySelector('#pm-groups .pm-g.act'));
    };
    const close = (p) => p.keyboard.press('Escape');
    const showGroup = (p, g) => p.evaluate((k) => document.getElementById(`pm-g-${k}`).click(), g);
    const dom = (p) => p.evaluate(() => {
        const out = { order: [...document.querySelectorAll('#pm-groups .pm-g')].map((g) => g.dataset.group), groups: {} };
        for (const g of document.querySelectorAll('#pm-groups .pm-g')) {
            g.click();
            out.groups[g.dataset.group] = {
                n: g.querySelector('.n').textContent,
                rows: [...document.querySelectorAll('#pm-presets .pm-p')].map((r) => ({ name: r.dataset.name, short: r.textContent })),
                empty: !document.getElementById('pm-empty').hidden,
                label: document.getElementById('pm-presets').getAttribute('aria-labelledby'),
            };
        }
        return out;
    });

    // ── PM1 ───────────────────────────────────────────────────────────────
    // The predicate: DOM == fixture, users only in User with their full name, factory rows short + prefix == name.
    const content = (d, fx) => {
        const bad = [];
        if (JSON.stringify(d.order) !== JSON.stringify(GROUPS)) bad.push(`order ${d.order.join(' ')}`);
        if (!Array.isArray(fx) || fx.length !== GROUPS.length) { bad.push(`fixture groups ${Array.isArray(fx) ? fx.length : 'none'}`); return bad; }
        const users = new Set(fx.find((g) => g.group === 'user').presets.map((p) => p.name));
        for (const [k, key] of GROUPS.entries()) {
            const want = fx[k], got = d.groups[key];
            if (!want || want.group !== key) { bad.push(`fixture group ${k} is ${want && want.group}, want ${key}`); continue; }
            if (!got) { bad.push(`${key} missing in the page`); continue; }
            if (key !== 'user' && want.presets.length !== COUNTS[key]) bad.push(`${key} ${want.presets.length} presets, want ${COUNTS[key]}`);
            if (JSON.stringify(got.rows) !== JSON.stringify(want.presets.map((p) => ({ name: p.name, short: p.short })))) bad.push(`${key} rows != fixture`);
            if (got.n !== String(want.presets.length)) bad.push(`${key} badge "${got.n}"`);
            if (got.empty !== (want.presets.length === 0)) bad.push(`${key} empty note ${got.empty}`);
            if (got.label !== `pm-gh-${key}`) bad.push(`${key} labelled by ${got.label}`);
            for (const r of got.rows) {
                if (key === 'user') { if (r.short !== r.name) bad.push(`user row "${r.short}" is not its full name`); }
                else if (users.has(r.name)) bad.push(`user preset "${r.name}" listed under ${key}`);
                else if (`${PREFIX[key]}${DOT}${r.short}` !== r.name) bad.push(`${key} row "${r.short}" != its name "${r.name}"`);
            }
        }
        return bad;
    };
    await open(page);
    const d1 = await dom(page);
    await close(page);
    const b1 = content(d1, fixture);
    check(b1.length === 0, `PM1 10 groups in menu order; rows == the fixture (${GROUPS.map((g) => `${g} ${d1.groups[g] ? d1.groups[g].rows.length : '-'}`).join(', ')}); badges; the empty note on User only` + (b1.length ? ` — ${b1.slice(0, 4).join('; ')}` : ''));
    check(content(d1, fixture.slice(0, 9)).length > 0, 'PM1n control: a fixture missing the User group FAILS the content predicate');

    const page2 = await newPage(`n.getPresetMenu = ${JSON.stringify(userFixture)}; n.getCurrentPreset = ${JSON.stringify(CURRENT)}; n.loadPreset = true;`);
    await open(page2);
    const st2 = await page2.evaluate(() => ({
        act: (document.querySelector('#pm-groups .pm-g.act') || {}).id,
        focus: document.activeElement ? document.activeElement.id : '',
        hl: [...document.querySelectorAll('#pm-presets .pm-p.hl')].map((r) => r.dataset.name),
        sel: [...document.querySelectorAll('#pm-presets .pm-p.sel')].map((r) => r.dataset.name),
        ariaSel: [...document.querySelectorAll('#pm-presets .pm-p[aria-selected="true"]')].map((r) => r.dataset.name),
        active: document.getElementById('pm-presets').getAttribute('aria-activedescendant'),
        hlId: (document.querySelector('#pm-presets .pm-p.hl') || {}).id,
    }));
    const d2 = await dom(page2);
    await close(page2);
    const b2 = content(d2, userFixture);
    check(b2.length === 0 && d2.groups.user.rows.length === 2 && d2.groups.pads.rows.length === 20 && !d2.groups.pads.rows.some((r) => r.name === `Pads${DOT}Mine`),
          `PM1 user presets in User only, full names shown: User ${JSON.stringify(d2.groups.user.rows.map((r) => r.short))}, Pads ${d2.groups.pads.rows.length}` + (b2.length ? ` — ${b2.slice(0, 4).join('; ')}` : ''));
    const mergedFx = userFixture.map((g) => (g.group === 'pads' ? { ...g, presets: g.presets.concat([{ name: `Pads${DOT}Mine`, short: 'Mine' }]) } : g));
    const mergedDom = { ...d2, groups: { ...d2.groups, pads: { ...d2.groups.pads, rows: d2.groups.pads.rows.concat([{ name: `Pads${DOT}Mine`, short: 'Mine' }]), n: '21' } } };
    check(content(mergedDom, mergedFx).length > 0, 'PM1n control: the user "Pads · Mine" merged into Pads FAILS the content predicate');

    // ── PM4 (open on the current preset) ─────────────────────────────────
    check(st2.act === 'pm-g-rhythmic' && st2.focus === 'pm-presets' && st2.hl.length === 1 && st2.hl[0] === CURRENT && st2.sel.length === 1 && st2.sel[0] === CURRENT
          && st2.ariaSel.length === 1 && st2.ariaSel[0] === CURRENT && st2.active === st2.hlId,
          `PM4 open on "${CURRENT}": group ${st2.act}, focus ${st2.focus}, highlighted + .sel + aria-selected ${JSON.stringify(st2.hl)}, activedescendant ${st2.active}`);

    // ── PM2 ───────────────────────────────────────────────────────────────
    const geometry = (p) => p.evaluate(() => {
        const M = document.getElementById('preset-menu'), P = document.getElementById('pm-presets');
        const f = document.getElementById('plugin').getBoundingClientRect(), r = M.getBoundingClientRect();
        const rows = [...P.querySelectorAll('.pm-p')];
        const vis = (el) => { const b = el.getBoundingClientRect(); const pr = P.getBoundingClientRect(); return b.top >= pr.top - 0.5 && b.bottom <= pr.bottom + 0.5; };
        const covered = rows.filter((el) => vis(el)).filter((el) => { const b = el.getBoundingClientRect(); return document.elementFromPoint(b.left + b.width / 2, b.top + b.height / 2) !== el; }).map((el) => el.textContent);
        const wide = rows.filter((el) => el.scrollWidth > el.clientWidth).map((el) => el.textContent);
        const heads = [...document.querySelectorAll('#pm-groups .pm-g')].map((g) => {
            const sp = g.querySelector('span[data-i18n]'), n = g.querySelector('.n');
            const gb = g.getBoundingClientRect(), sb = sp.getBoundingClientRect(), nb = n.getBoundingClientRect();
            return { t: sp.textContent, w: sb.width, over: g.scrollWidth > g.clientWidth || nb.right > gb.right - 4 || sb.right > nb.left - 4 };
        });
        const file = document.getElementById('pm-file');
        const fb = file.getBoundingClientRect(), lb = document.querySelector('#preset-menu .pm-left').getBoundingClientRect();
        const textW = (el) => { const rg = document.createRange(); rg.selectNodeContents(el); return rg.getBoundingClientRect().width; };
        const widest = rows.reduce((a, el) => Math.max(a, textW(el)), 0);
        return { x: r.left - f.left, y: r.top - f.top, w: r.width, h: r.height, sh: P.scrollHeight, ch: P.clientHeight, n: rows.length, covered, wide, heads,
                 fileOver: file.scrollWidth > file.clientWidth || fb.bottom > lb.bottom + 0.5, fileT: file.textContent, fileW: textW(file), widest, paneW: P.clientWidth - 26, rowH: rows.length ? rows[0].getBoundingClientRect().height : 0 };
    });
    const inside = (g) => g.x >= 15 && g.y >= 15 && g.x + g.w <= 1200 - 15 && g.y + g.h <= 785;
    const noScroll = (g) => g.n > 20 || g.sh <= g.ch;
    const shots = [];
    for (const lang of ['en', 'fr', 'zh-Hans']) {
        await page.evaluate((l) => window.__setLanguage(l), lang);
        await page.evaluate(() => document.fonts.ready);
        await open(page);
        let ok = true, box = null, widest = { w: 0 }, widestRow = 0, paneW = 0, pads = null;
        const notes = [];
        for (const g of GROUPS) {
            await showGroup(page, g);
            const m = await geometry(page);
            box = m;
            const gOk = inside(m) && noScroll(m) && m.covered.length === 0 && m.wide.length === 0 && m.heads.every((h) => !h.over) && !m.fileOver;
            if (!gOk) notes.push(`${g}: ${JSON.stringify({ x: m.x, y: m.y, w: m.w, h: m.h, sh: m.sh, ch: m.ch, covered: m.covered.slice(0, 3), wide: m.wide.slice(0, 3), heads: m.heads.filter((h) => h.over), file: m.fileOver })}`);
            ok = ok && gOk;
            widest = m.heads.reduce((a, h) => (h.w > a.w ? h : a), widest);
            widestRow = Math.max(widestRow, m.widest); paneW = m.paneW;
            if (g === 'pads') pads = m;
            if (g === 'pads' && shotsDir) {
                fs.mkdirSync(shotsDir, { recursive: true });
                const file = path.join(shotsDir, `preset-menu-${lang}.png`);
                await page.screenshot({ path: file });
                shots.push(file);
            }
        }
        check(ok, `PM2 ${lang}: box ${box.w} x ${box.h} at (${box.x}, ${box.y}), inside the frame; 10 groups, <= 20 rows without a scrollbar (Pads: ${pads.n} rows x ${pads.rowH} px, scrollHeight ${pads.sh} / clientHeight ${pads.ch}); rows uncovered; widest short name ${widestRow.toFixed(1)} / ${paneW} px of text box; widest header "${widest.t}" ${widest.w.toFixed(1)} px; file row "${box.fileT}" ${box.fileW.toFixed(1)} px fits` + (notes.length ? ` — ${notes.join('; ')}` : ''));
        await close(page);
    }
    await page.evaluate(() => window.__setLanguage('en'));
    await open(page);
    await showGroup(page, 'pads');
    const ctl = await page.evaluate(() => {
        const M = document.getElementById('preset-menu'), P = document.getElementById('pm-presets'), row = P.querySelector('.pm-p'), keep = row.textContent;
        row.textContent = 'A Sixty Character Short Preset Name That Cannot Fit The Pane!';
        const wide = row.scrollWidth > row.clientWidth;
        row.textContent = keep;
        P.style.maxHeight = '100px';
        const scrolls = P.scrollHeight > P.clientHeight;
        P.style.maxHeight = '';
        const top = M.style.top;
        M.style.top = '700px';
        const f = document.getElementById('plugin').getBoundingClientRect(), r = M.getBoundingClientRect();
        const out = r.top - f.top + r.height > 785;
        M.style.top = top;
        return { wide, scrolls, out };
    });
    check(ctl.wide, 'PM2n control: a 60-character short name overflows its row (the fit predicate is live)');
    check(ctl.scrolls, 'PM2n control: a 100 px pane scrolls (the no-scroll predicate is live)');
    check(ctl.out, 'PM2n control: the box at top 700 px leaves the frame (the inside predicate is live)');
    await close(page);

    // ── PM3 ───────────────────────────────────────────────────────────────
    const state = (p) => p.evaluate(() => {
        const G = document.getElementById('pm-groups'), P = document.getElementById('pm-presets');
        const ghl = [...G.querySelectorAll('.pm-g.hl')], phl = [...P.querySelectorAll('.pm-p.hl')];
        return { open: !document.getElementById('preset-menu').hidden, focus: document.activeElement ? document.activeElement.id : '',
                 act: (G.querySelector('.pm-g.act') || {}).id || '', ghl: ghl.length === 1 ? ghl[0].id : (ghl.length ? 'MANY' : ''),
                 phl: phl.length === 1 ? phl[0].id : (phl.length ? 'MANY' : ''), phlName: phl.length === 1 ? phl[0].dataset.name : '',
                 gad: G.getAttribute('aria-activedescendant') || '', pad: P.getAttribute('aria-activedescendant') || '',
                 expanded: document.getElementById('preset-name').getAttribute('aria-expanded') };
    });
    let agree = true;                         // the focused listbox's aria-activedescendant == its highlighted option
    const key = async (k) => {
        await page.keyboard.press(k);
        const s = await state(page);
        if (s.focus === 'pm-groups') agree = agree && s.gad === s.ghl && s.gad === s.act && s.pad === '';
        if (s.focus === 'pm-presets') agree = agree && s.pad === s.phl;
        return s;
    };
    await open(page);
    let st = await state(page);
    check(st.open && st.focus === 'pm-groups' && st.act === 'pm-g-drones' && st.gad === 'pm-g-drones' && st.pad === '' && st.expanded === 'true',
          `PM3 open on a preset outside the bank ("Default"): the groups listbox holds focus on Drones (focus "${st.focus}", ${st.gad}), aria-expanded ${st.expanded}`);
    const walk = [];
    for (let k = 0; k < 11; k++) { const s = await key('ArrowDown'); walk.push(s.focus === 'pm-file' ? 'file' : s.act.replace('pm-g-', '')); }
    check(JSON.stringify(walk) === JSON.stringify([...GROUPS.slice(1), 'file', 'drones']),
          `PM3 Down x 11: ${walk.join(' > ')} (the file row after User, then wraps)`);
    const upFromFirst = await key('ArrowUp');
    const upAgain = await key('ArrowUp');
    const home = await key('Home'); const end = await key('End');
    check(upFromFirst.focus === 'pm-file' && upAgain.act === 'pm-g-user' && upAgain.focus === 'pm-groups' && home.act === 'pm-g-drones' && end.act === 'pm-g-user',
          `PM3 Up from Drones -> the file row, Up again -> User; Home -> ${home.act}; End -> ${end.act}`);
    await key('Home');
    const right = await key('ArrowRight');
    check(right.focus === 'pm-presets' && right.phl === 'pm-p-0' && right.pad === 'pm-p-0' && right.ghl === '',
          `PM3 Right: the presets listbox holds focus on the first row (${right.phl}, "${right.phlName}")`);
    const nRows = COUNTS.drones;
    let rowsWalk = true;
    for (let k = 1; k < nRows; k++) { const s = await key('ArrowDown'); rowsWalk = rowsWalk && s.phl === `pm-p-${k}`; }
    const rWrap = await key('ArrowDown'); const rUp = await key('ArrowUp'); const rHome = await key('Home'); const rEnd = await key('End');
    check(rowsWalk && rWrap.phl === 'pm-p-0' && rUp.phl === `pm-p-${nRows - 1}` && rHome.phl === 'pm-p-0' && rEnd.phl === `pm-p-${nRows - 1}`,
          `PM3 Down walks the ${nRows} Drones rows and wraps; Up from the first -> the last; Home / End`);
    const left = await key('ArrowLeft');
    check(left.focus === 'pm-groups' && left.ghl === 'pm-g-drones' && left.phl === '' && left.pad === '', `PM3 Left: back to the groups on Drones, the row highlight cleared (focus "${left.focus}")`);
    await key('ArrowDown');                   // Expressive
    const enterG = await key('Enter');
    const enterGDown = await key('ArrowDown');
    check(enterG.focus === 'pm-presets' && enterG.act === 'pm-g-expressive' && enterG.phl === 'pm-p-0' && enterGDown.phl === 'pm-p-1',
          `PM3 Enter on a group goes right (${enterG.act}, ${enterG.phl}), Down -> ${enterGDown.phl}`);
    await page.evaluate(() => { window.__stubNativeCalls.length = 0; });
    const wantName = enterGDown.phlName;
    st = await key('Enter');
    let calls = await page.evaluate(() => window.__stubNativeCalls.filter((c) => c.name === 'loadPreset').map((c) => c.args[0]));
    check(!st.open && st.focus === 'preset-name' && st.expanded === 'false' && st.gad === '' && st.pad === '' && calls.length === 1 && calls[0] === wantName && wantName.startsWith(`Expressive${DOT}`),
          `PM3 Enter on a row: one loadPreset("${calls.join('", "')}") with the full name, the menu closes, focus on the preset name`);
    await open(page);
    await key('Home'); await key('ArrowUp');  // the file row
    await page.evaluate(() => { window.__stubNativeCalls.length = 0; });
    st = await key('Enter');
    calls = await page.evaluate(() => window.__stubNativeCalls.filter((c) => c.name === 'loadPresetFromFile').length);
    check(!st.open && calls === 1, `PM3 Enter on the file row: loadPresetFromFile called once (${calls}), the menu closes`);
    await page.waitForTimeout(200);
    await open(page);
    await key('ArrowRight');
    st = await key('Escape');
    check(!st.open && st.focus === 'preset-name' && st.expanded === 'false', `PM3 Escape closes and returns focus to the preset name (focus "${st.focus}")`);
    await open(page);
    await page.keyboard.press('Tab');
    st = await state(page);
    check(!st.open, 'PM3 Tab closes the menu');
    check(agree, 'PM3 aria-activedescendant names the highlighted option of the focused listbox after every move');

    // ── PM4 (mouse) ───────────────────────────────────────────────────────
    await open(page2);
    const centre = (p, sel) => p.evaluate((s) => { const b = document.querySelector(s).getBoundingClientRect(); return { x: b.left + b.width / 2, y: b.top + b.height / 2 }; }, sel);
    let c = await centre(page2, '#pm-g-pads');
    await page2.mouse.move(c.x, c.y);
    await page2.mouse.move(c.x + 2, c.y);
    const hov = await page2.evaluate(() => ({ act: document.querySelector('#pm-groups .pm-g.act').id, label: document.getElementById('pm-presets').getAttribute('aria-labelledby'),
        first: (document.querySelector('#pm-presets .pm-p') || {}).dataset }));
    check(hov.act === 'pm-g-pads' && hov.label === 'pm-gh-pads' && hov.first && hov.first.name.startsWith(`Pads${DOT}`), `PM4 hover on Pads switches the right pane (${hov.act}, labelled by ${hov.label})`);
    c = await centre(page2, '#pm-p-3');
    const clickName = await page2.evaluate(() => document.getElementById('pm-p-3').dataset.name);
    await page2.evaluate(() => { window.__stubNativeCalls.length = 0; });
    await page2.mouse.click(c.x, c.y);
    await page2.waitForTimeout(150);
    const afterClick = await page2.evaluate(() => ({ calls: window.__stubNativeCalls.filter((x) => x.name === 'loadPreset').map((x) => x.args[0]),
        bar: document.getElementById('preset-name-text').textContent, open: !document.getElementById('preset-menu').hidden }));
    check(afterClick.calls.length === 1 && afterClick.calls[0] === clickName && afterClick.bar === clickName && !afterClick.open,
          `PM4 a click on the 4th Pads row loads "${afterClick.calls.join('", "')}"; the bar shows the full name "${afterClick.bar}"`);
    await open(page2);
    await page2.mouse.click(1100, 700);
    const outside = await isOpen(page2);
    await open(page2);
    await page2.evaluate(() => document.getElementById('preset-name').click());
    await page2.waitForTimeout(100);
    const toggled = await isOpen(page2);
    check(!outside && !toggled, 'PM4 a press outside closes the menu; the preset name toggles it');

    check(errors.length === 0, `no page errors${errors.length ? ': ' + errors.slice(0, 3).join(' | ') : ''}`);
    if (shots.length) console.log(`  shots: ${shots.join(', ')}`);
    await browser.close();
    await srv.close();
    console.log(failures === 0 ? 'ALL PASS' : `${failures} FAILURE(S)`);
    process.exit(failures === 0 ? 0 : 1);
})().catch((e) => { console.log(`ui_preset_menu_check crashed: ${e.stack || e}`); process.exit(1); });
