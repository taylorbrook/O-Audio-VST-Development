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

    ui_factory_picker_check.js — the Factory picker in a real page (1.2.0)

    ui_frontend_check.js is a static closure; what this file asserts needs a
    layout engine. Real page, served stub (scripts/serve-ui.js), the 41
    sources of tests/ui-stub/generic-overrides.json (dumped from the table by
    the render harness), embedded fonts loaded first.

      FK1  content: one item per manifest source, each under the heading of
           its manifest category, table order inside a group; 7 headings, 4
           columns, the flat (DOM) order is the plan's reading order.
      FK2  geometry, per language (en / fr / zh-Hans) and per lane (1 - 4):
           no vertical scroll, the panel inside the 1200 x 800 frame with the
           15 px margin, every item is the top element at its own centre,
           every heading and every name fits its box (nowrap).
      FK3  keyboard: the listbox holds focus; Down / Up walk the flat order
           and wrap; Home / End; Right / Left land on the nearest row of the
           neighbouring column (a deep row clamps to the shorter column's
           last item; the outer columns stop); type-ahead (a run, then one
           letter repeated); aria-activedescendant names the highlighted item
           after every move; Enter calls loadFactorySlot { slot, id } and
           closes; Escape closes and returns focus to the lane's link.
      FK4  dismissal: a press outside closes; the link toggles.

    Controls (each must FAIL its predicate): a 60-character name does not fit
    its row; a 100 px max-height makes the list scroll; a panel moved to
    top 700 px leaves the frame; a missing source is seen by FK1.

    Usage:  node plugins/O-GranulatorInstrument/tests/ui_factory_picker_check.js [--shots DIR]
    --shots DIR writes picker-en.png / picker-fr.png / picker-zh-Hans.png.
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

const COLUMNS = [['tonal', 'voice'], ['plucked'], ['texture', 'drone'], ['rhythmic', 'field']];
const shotsAt = process.argv.indexOf('--shots');
const shotsDir = shotsAt > 0 ? path.resolve(process.argv[shotsAt + 1]) : null;

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) {
        console.log('ui_factory_picker_check: Playwright could not be resolved — NOTHING verified (exit 77 is not a pass).');
        process.exit(77);
    }
    const manifest = JSON.parse(fs.readFileSync(path.join(pluginRoot, 'Resources', 'factory', 'manifest.json'), 'utf8'));
    const sources = manifest.sources;
    const want = [];   // the reading order: column by column, category by category, table order inside
    for (const col of COLUMNS) for (const cat of col) for (const s of sources) if (s.category === cat) want.push(s);
    console.log(`== O-GranulatorInstrument ui_factory_picker_check — ${sources.length} sources ==`);
    let failures = 0;
    const check = (ok, msg) => { if (!ok) ++failures; console.log(`  ${ok ? 'PASS' : 'FAIL'}: ${msg}`); return ok; };

    const built = S.buildRoot('O-GranulatorInstrument');
    const srv = await S.serve(built.root);
    const browser = await pw.chromium.launch();
    const page = await browser.newPage({ viewport: { width: 1200, height: 800 } });
    const errors = [];
    page.on('pageerror', (e) => errors.push(`pageerror: ${e.message}`));
    await page.goto(`http://127.0.0.1:${srv.port}/`);
    await page.waitForFunction(() => window.__stubStates && document.querySelectorAll('#factory-list .it').length > 0, null, { timeout: 15000 });
    await page.evaluate(() => document.fonts.ready);
    await page.waitForTimeout(200);
    // The stub's lane 1 holds a source; the picker lives on an empty lane.
    const emptyAll = () => page.evaluate(() => document.querySelectorAll('.lane').forEach((l) => l.classList.add('empty')));
    await emptyAll();

    // ── FK1 ───────────────────────────────────────────────────────────────
    const dom = await page.evaluate(() => {
        const L = document.getElementById('factory-list');
        return {
            cols: [...L.querySelectorAll('.col')].map((c) => [...c.querySelectorAll('.grp:not([hidden])')].map((g) => g.dataset.cat)),
            heads: [...L.querySelectorAll('.grp:not([hidden]) .gh')].length,
            items: [...L.querySelectorAll('.it')].map((it) => ({ id: it.dataset.id, name: it.textContent, cat: it.closest('.grp').dataset.cat, domId: it.id })),
        };
    });
    const sameOrder = (items) => items.length === want.length && items.every((it, k) => it.id === want[k].id && it.name === want[k].name && it.cat === want[k].category);
    check(sources.length === 41 && sameOrder(dom.items), `FK1 ${dom.items.length} items == the manifest's ${sources.length}, each under its category, reading order column by column`);
    check(dom.heads === 7 && JSON.stringify(dom.cols) === JSON.stringify(COLUMNS), `FK1 7 headings in 4 columns: ${dom.cols.map((c) => c.join(' + ')).join(' | ')}`);
    check(!sameOrder(dom.items.slice(1)), 'FK1n control: the list without its first source FAILS the content predicate');

    // ── FK2 ───────────────────────────────────────────────────────────────
    const open = async (n) => {
        await page.evaluate((k) => { const t = document.getElementById(`lane-factory-${k}`); t.scrollIntoView(); t.click(); }, n);
        await page.waitForFunction(() => !document.getElementById('factory-list').hidden);
    };
    const close = () => page.evaluate(() => document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Escape', bubbles: true })));
    const geometry = () => page.evaluate(() => {
        const L = document.getElementById('factory-list'), f = document.getElementById('plugin').getBoundingClientRect(), r = L.getBoundingClientRect();
        const items = [...L.querySelectorAll('.it')];
        const covered = items.filter((it) => {
            const b = it.getBoundingClientRect();
            return document.elementFromPoint(b.left + b.width / 2, b.top + b.height / 2) !== it;
        }).map((it) => it.dataset.id);
        const wideNames = items.filter((it) => it.scrollWidth > it.clientWidth).map((it) => it.dataset.id);
        const heads = [...L.querySelectorAll('.grp:not([hidden]) .gh')].map((gh) => {
            const sp = gh.querySelector('span');
            return { t: sp.textContent, w: sp.getBoundingClientRect().width, box: gh.clientWidth, over: gh.scrollWidth > gh.clientWidth };
        });
        return { x: r.left - f.left, y: r.top - f.top, w: r.width, h: r.height, sh: L.scrollHeight, ch: L.clientHeight, covered, wideNames, heads };
    });
    const inside = (g) => g.x >= 15 && g.y >= 15 && g.x + g.w <= 1200 - 15 && g.y + g.h <= 785;
    const noScroll = (g) => g.sh <= g.ch;
    // A heading keeps at least 10 px of its rule (the ::after line) beside it: 140 px column - 6 px gap - 10 px.
    const headsFit = (g) => g.heads.every((h) => !h.over && h.w <= h.box - 16);
    for (const lang of ['en', 'fr', 'zh-Hans']) {
        await page.evaluate((l) => window.__setLanguage(l), lang);
        await page.evaluate(() => document.fonts.ready);
        await emptyAll();
        let ok = true, worst = null;
        const notes = [];
        for (let n = 1; n <= 4; n++) {
            await open(n);
            const g = await geometry();
            const laneOk = inside(g) && noScroll(g) && g.covered.length === 0 && g.wideNames.length === 0 && headsFit(g);
            if (!laneOk) notes.push(`lane ${n}: ${JSON.stringify({ x: g.x, y: g.y, w: g.w, h: g.h, sh: g.sh, ch: g.ch, covered: g.covered, wide: g.wideNames, heads: g.heads.filter((h) => h.over || h.w > h.box - 16) })}`);
            ok = ok && laneOk;
            worst = g.heads.reduce((a, h) => (!a || h.w > a.w ? h : a), worst);
            if (n === 1 && shotsDir) {
                fs.mkdirSync(shotsDir, { recursive: true });
                await page.screenshot({ path: path.join(shotsDir, `picker-${lang}.png`) });
            }
            if (n === 4) notes.unshift(`panel ${g.w} x ${g.h} px, scrollHeight ${g.sh} / clientHeight ${g.ch}`);
            await close();
        }
        check(ok, `FK2 ${lang}: lanes 1 - 4, inside the frame, no scroll, 41 items uncovered, names and headings fit — ${notes.join('; ')}; widest heading "${worst.t}" ${worst.w.toFixed(1)} / ${worst.box - 16} px`);
    }
    await page.evaluate(() => window.__setLanguage('en'));
    await emptyAll();
    await open(2);
    const ctl = await page.evaluate(() => {
        const L = document.getElementById('factory-list'), it = L.querySelector('.it'), keep = it.textContent;
        it.textContent = 'A Sixty Character Source Name That Cannot Fit A 140 px Column';
        const wide = it.scrollWidth > it.clientWidth;
        it.textContent = keep;
        L.style.maxHeight = '100px';
        const scrolls = L.scrollHeight > L.clientHeight;
        L.style.maxHeight = '';
        const top = L.style.top;
        L.style.top = '700px';
        const f = document.getElementById('plugin').getBoundingClientRect(), r = L.getBoundingClientRect();
        const out = r.top - f.top + r.height > 785;
        L.style.top = top;
        return { wide, scrolls, out };
    });
    check(ctl.wide, 'FK2n control: a 60-character name overflows its row (the fit predicate is live)');
    check(ctl.scrolls, 'FK2n control: max-height 100 px makes the list scroll (the no-scroll predicate is live)');
    check(ctl.out, 'FK2n control: the panel at top 700 px leaves the frame (the inside predicate is live)');
    await close();

    // ── FK3 ───────────────────────────────────────────────────────────────
    const state = () => page.evaluate(() => {
        const L = document.getElementById('factory-list'), hl = [...L.querySelectorAll('.it.hl')];
        return { open: !L.hidden, focus: document.activeElement ? document.activeElement.id : '', hl: hl.length === 1 ? hl[0].dataset.id : (hl.length ? 'MANY' : ''),
                 active: L.getAttribute('aria-activedescendant') || '' };
    });
    const key = async (k) => { await page.keyboard.press(k); return state(); };
    const idx = (id) => want.findIndex((s) => s.id === id);
    let agree = true;                                   // aria-activedescendant == the highlighted item, after every move
    const moved = (st) => { agree = agree && st.active === (st.hl ? `factory-it-${st.hl}` : ''); return st.hl; };

    await open(3);
    let st = await state();
    check(st.open && st.focus === 'factory-list' && st.hl === '' && st.active === '', `FK3 open: the listbox holds focus, nothing highlighted (focus "${st.focus}")`);
    const walk = [];
    for (let k = 0; k < want.length + 1; k++) walk.push(moved(await key('ArrowDown')));
    check(walk.slice(0, want.length).every((id, k) => id === want[k].id) && walk[want.length] === want[0].id, `FK3 Down x ${want.length + 1}: the flat reading order, then wraps to "${want[0].name}"`);
    check(moved(await key('ArrowUp')) === want[want.length - 1].id && moved(await key('Home')) === want[0].id && moved(await key('End')) === want[want.length - 1].id,
          'FK3 Up from the first wraps to the last; Home; End');

    // Left / Right: the nearest row of the neighbouring column, computed from the rendered rows.
    const rows = await page.evaluate(() => [...document.querySelectorAll('#factory-list .it')].map((it) => ({ id: it.dataset.id, y: it.offsetTop, col: [...it.closest('.cols').children].indexOf(it.closest('.col')) })));
    const nearest = (id, dir) => {
        const me = rows.find((r) => r.id === id), c = me.col + dir;
        const pool = rows.filter((r) => r.col === c);
        if (!pool.length) return id;
        return pool.reduce((a, r) => (Math.abs(r.y - me.y) < Math.abs(a.y - me.y) ? r : a), pool[0]).id;
    };
    const goTo = async (id) => { await key('Home'); for (let k = 0; k < idx(id); k++) await key('ArrowDown'); return (await state()).hl === id; };
    const deepest = rows.filter((r) => r.col === 2).reduce((a, r) => (r.y > a.y ? r : a));       // the last item of the tallest column
    const lastOf = (c) => rows.filter((r) => r.col === c).reduce((a, r) => (r.y > a.y ? r : a)).id;
    let lr = await goTo(want[2].id);
    const r1 = moved(await key('ArrowRight'));
    lr = lr && r1 === nearest(want[2].id, 1) && rows.find((r) => r.id === r1).col === 1;
    const back = moved(await key('ArrowLeft'));
    lr = lr && rows.find((r) => r.id === back).col === 0;
    const stay = moved(await key('ArrowLeft'));
    const stay2 = moved(await key('ArrowLeft'));
    lr = lr && stay === nearest(back, -1) && stay === back && stay2 === back;
    check(lr, `FK3 Right from "${want[2].name}" -> "${want[idx(r1)].name}" (nearest row of column 2), Left returns to column 1 and stops there`);
    let clamp = await goTo(deepest.id);
    const r2 = moved(await key('ArrowRight'));
    const l2 = (await goTo(deepest.id)) && moved(await key('ArrowLeft'));
    clamp = clamp && r2 === lastOf(3) && r2 === nearest(deepest.id, 1) && l2 === lastOf(1) && moved(await key('ArrowLeft')) !== '' ;
    await goTo(lastOf(3));
    clamp = clamp && moved(await key('ArrowRight')) === lastOf(3);
    check(clamp, `FK3 from the deepest row ("${want[idx(deepest.id)].name}"): Right clamps to the last of column 4 ("${want[idx(r2)].name}"), Left to the last of column 2 ("${want[idx(l2)] ? want[idx(l2)].name : l2}"); Right on column 4 stays`);

    // Type-ahead.
    const type = async (text) => { let s; for (const ch of text) s = await key(ch === ' ' ? 'Space' : ch); return moved(s); };
    const firstWith = (q, from) => { for (let j = 0; j < want.length; j++) { const s = want[(from + j) % want.length]; if (s.name.toLowerCase().startsWith(q)) return s.id; } return ''; };
    await key('Home');
    await page.waitForTimeout(800);
    const tFm = await type('fm');
    await page.waitForTimeout(800);
    const tSawSt = await type('saw s');
    await page.waitForTimeout(800);
    const c1 = await type('c'); const c1want = firstWith('c', idx('saw_stack') + 1);
    await page.waitForTimeout(800);
    const c2 = await type('c'); const c2want = firstWith('c', idx(c1) + 1);
    await page.waitForTimeout(800);
    const cc = await type('cc');   // one letter repeated: two steps through the c names
    const ccWant = firstWith('c', idx(firstWith('c', idx(c2) + 1)) + 1);
    await page.waitForTimeout(800);
    const none = await type('zq');
    check(tFm === 'fm_bell' && tSawSt === 'saw_stack' && c1 === c1want && c2 === c2want && c2 !== c1 && cc === ccWant && none === cc,
          `FK3 type-ahead: "fm" -> ${tFm}, "saw s" -> ${tSawSt}, "c" -> ${c1}, "c" again -> ${c2}, "cc" -> ${cc}, "zq" (no match) stays`);
    check(agree, 'FK3 aria-activedescendant names the highlighted item after every move');

    // Enter picks: loadFactorySlot { slot, id }, the list closes.
    await page.waitForTimeout(800);
    await type('contra');
    await page.evaluate(() => { window.__stubNativeCalls.length = 0; });
    st = await key('Enter');
    const calls = await page.evaluate(() => window.__stubNativeCalls.filter((c) => c.name === 'loadFactorySlot').map((c) => JSON.stringify(c.args)));
    check(!st.open && st.active === '' && calls.length === 1 && /"slot":\s*3/.test(calls[0]) && /"id":\s*"contrabassoon"/.test(calls[0]),
          `FK3 Enter on Contrabassoon from lane 3: one loadFactorySlot call ${calls.join(' ')}, the list closes`);

    await emptyAll();
    await open(4);
    await key('ArrowDown');
    st = await key('Escape');
    check(!st.open && st.focus === 'lane-factory-4' && st.active === '', `FK3 Escape closes and returns focus to the lane's link (focus "${st.focus}")`);
    const expanded = await page.evaluate(() => document.getElementById('lane-factory-4').getAttribute('aria-expanded'));
    check(expanded === 'false', 'FK3 the link reads aria-expanded="false" after the close');

    // ── FK4 ───────────────────────────────────────────────────────────────
    await open(2);
    await page.mouse.click(1100, 700);
    const afterOutside = await state();
    await open(2);
    await page.evaluate(() => document.getElementById('lane-factory-2').click());
    const afterToggle = await state();
    check(!afterOutside.open && !afterToggle.open, 'FK4 a press outside closes the list; the link toggles it');
    await open(1);
    const hit = await page.evaluate(() => { const it = document.getElementById('factory-it-kalimba'); const b = it.getBoundingClientRect(); return { x: b.left + b.width / 2, y: b.top + b.height / 2 }; });
    await page.evaluate(() => { window.__stubNativeCalls.length = 0; });
    await page.mouse.click(hit.x, hit.y);
    const clickCalls = await page.evaluate(() => window.__stubNativeCalls.filter((c) => c.name === 'loadFactorySlot').map((c) => JSON.stringify(c.args)));
    check(clickCalls.length === 1 && /"slot":\s*1/.test(clickCalls[0]) && /"id":\s*"kalimba"/.test(clickCalls[0]) && !(await state()).open,
          `FK4 a click on Kalimba from lane 1: ${clickCalls.join(' ')}`);

    check(errors.length === 0, `no page errors${errors.length ? ': ' + errors.slice(0, 3).join(' | ') : ''}`);
    await browser.close();
    await srv.close();
    console.log(failures === 0 ? 'ALL PASS' : `${failures} FAILURE(S)`);
    process.exit(failures === 0 ? 0 : 1);
})().catch((e) => { console.log(`ui_factory_picker_check crashed: ${e.stack || e}`); process.exit(1); });
