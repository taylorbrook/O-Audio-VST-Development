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

    ui_layout_check.js
    O-Strata — the page's LAYOUT BUDGET, measured on the rendered page at the
    shipping 1200 x 800 frame (Stage 3 Round A, PLAN Decision 21 / 25).

    check-ui-labels measures label geometry across languages; it is blind to the
    tab budget (BRIEF decision 8: every tab fits 666 px without scrolling), to a
    wrapped .osc-params row, to the width-pinned [data-i18n] set (assertion 7
    excludes labels by design — memory pattern_ui_labels_gate_blind_to_width_
    pinned_data_i18n), and to paint order. This gate covers those, for every
    language x every tab x the default state + the Terrain states of
    tests/i18n-states.json (replayed cumulatively, exactly as check-ui-labels
    replays them).

    Served through scripts/serve-ui.js (generic stub + tests/ui-stub/
    generic-overrides.json, port 0), Playwright resolved from wherever it lives.

    Usage (from the repo root):
        node plugins/O-Strata/tests/ui_layout_check.js
        node plugins/O-Strata/tests/ui_layout_check.js --viewport 1200x780   # liveness: must FAIL
        node plugins/O-Strata/tests/ui_layout_check.js --inject-css ".terrain-main{padding-top:30px}"

    Exit code: 0 = ALL PASS, 2 = any FAIL, 77 = Playwright not resolvable.

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const S          = require(path.join(repoRoot, 'scripts', 'serve-ui.js'));
const PLUGIN     = path.basename(pluginRoot);

const argv = process.argv.slice(2);
const val  = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const vp   = (val('--viewport') || '1200x800').split('x').map(Number);
const SHIP_W = vp[0], SHIP_H = vp[1];
const INJECT_CSS = val('--inject-css');
const verbose = argv.includes('--verbose');

const TAB_BUDGET = 666;          // 800 - 34 header - 32 tab bar - 68 footer (index.html CARDS comment)
const OSC_PARAMS_RIGHT_MAX = 1175;
const PIN_TOL = 0.5;             // sub-pixel text jitter across runs

let failed = 0, passed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (cond) ++passed; else ++failed;
}
function head(t) { console.log(`\n-- ${t}`); }

// ── page-side probes (one evaluate each, so nothing shifts between round trips) ──
const PROBE_TAB = () => {
    const active = document.querySelector('.tab-content.active');
    if (!active) return null;
    return { id: active.id, sh: active.scrollHeight, ch: active.clientHeight, sw: active.scrollWidth, cw: active.clientWidth };
};

const PROBE_TERRAIN_REGIONS = () => {
    const sel = ['.terrain-toprow', '.terrain-view-col', '.terrain-panel-col'];
    const rects = sel.map((s) => { const e = document.querySelector('#terrain-tab ' + s); return e ? { s, r: e.getBoundingClientRect() } : null; });
    const out = [];
    for (let i = 0; i < rects.length; ++i) for (let j = i + 1; j < rects.length; ++j) {
        if (!rects[i] || !rects[j]) { out.push({ a: sel[i], b: sel[j], missing: true }); continue; }
        const a = rects[i].r, b = rects[j].r;
        const w = Math.max(0, Math.min(a.right, b.right) - Math.max(a.left, b.left));
        const h = Math.max(0, Math.min(a.bottom, b.bottom) - Math.max(a.top, b.top));
        out.push({ a: sel[i], b: sel[j], area: w * h });
    }
    return out;
};

const PROBE_OSC_PARAMS = () => Array.from(document.querySelectorAll('#synth-tab .osc-params')).map((el) => {
    const kids = Array.from(el.children).map((c) => { const r = c.getBoundingClientRect(); return { top: r.top, right: r.right, w: r.width, h: r.height }; });
    const tops = kids.map((k) => k.top);
    // ONE ROW: a wrapped child sits at least its own height lower; the 2 px dropdown-group vs
    // knob-container offset inside the row is O-Prism's own (align-items: flex-start).
    return { n: kids.length, minTop: Math.min(...tops), maxTop: Math.max(...tops), minH: Math.min(...kids.map((k) => k.h)), right: Math.max(...kids.map((k) => k.right)), zeroWidth: kids.filter((k) => k.w === 0).length };
});

const PROBE_PINNED = () => {
    const out = {};
    const put = (key, el) => { if (el) { const r = el.getBoundingClientRect(); out[key] = { w: r.width, h: r.height }; } else out[key] = null; };
    document.querySelectorAll('.tab-bar .tab').forEach((el, i) => put(`.tab[${i}]`, el));
    document.querySelectorAll('#terrain-osc-toggle .wt-osc-btn').forEach((el, i) => put(`.wt-osc-btn[${i}]`, el));
    document.querySelectorAll('#seg-terQuality .terrain-seg-btn').forEach((el, i) => put(`.terrain-seg-btn[${i}]`, el));
    put('#tips-toggle', document.getElementById('tips-toggle'));
    return out;
};

const PROBE_HITS = (selectors) => selectors.map((sel) => {
    const el = document.querySelector(sel);
    if (!el) return { sel, missing: true };
    const r = el.getBoundingClientRect();
    if (r.width === 0 || r.height === 0) return { sel, zero: true };
    const hit = document.elementFromPoint(r.left + r.width / 2, r.top + r.height / 2);
    return { sel, ok: hit === el || el.contains(hit), hit: hit ? (hit.tagName.toLowerCase() + (hit.id ? '#' + hit.id : '') + (hit.className && typeof hit.className === 'string' ? '.' + hit.className.trim().split(/\s+/).join('.') : '')) : null };
});

const SETTINGS_OPEN = () => document.getElementById('settings-popover') && document.getElementById('settings-popover').classList.contains('visible');

// ── source greps (no browser needed) ──
function sourceGreps() {
    head('source greps');
    const indexPath  = path.join(pluginRoot, 'Source', 'ui', 'public', 'index.html');
    const editorPath = path.join(pluginRoot, 'Source', 'PluginEditor.cpp');
    const html = fs.readFileSync(indexPath, 'utf8');
    const cpp  = fs.readFileSync(editorPath, 'utf8');
    const lines = html.split('\n');
    const vw = html.match(/[0-9](vh|vw|dvh|svh)\b/g) || [];
    check(vw.length === 0, `no viewport units in index.html (${vw.length} match(es))`);
    const modules = lines.filter((l) => /^<script type="module">/.test(l)).length;
    check(modules === 1, `exactly one <script type="module"> tag line (${modules})`);
    check(/user-select:\s*none/.test(html), 'user-select: none present');
    check(/contextmenu/.test(html), 'contextmenu handler present');
    const hb = html.match(/html,\s*body\s*\{[^}]*\}/);
    check(hb !== null && /height:\s*100%/.test(hb[0]), 'html, body { … } carries height: 100%');
    const evHtml = (html.match(/evaluateJavascript/g) || []).length;
    const evCpp  = (cpp.match(/evaluateJavascript/g) || []).length;
    check(evHtml === 0 && evCpp === 0, `evaluateJavascript count 0 / 0 (index.html ${evHtml}, PluginEditor.cpp ${evCpp})`);
}

async function main() {
    console.log(`ui_layout_check — ${PLUGIN} at ${SHIP_W} x ${SHIP_H}${INJECT_CSS ? `  (injected CSS: ${INJECT_CSS})` : ''}`);

    sourceGreps();

    const pw = S.resolvePlaywright();
    if (pw == null) {
        console.log('\n  SKIP: playwright not resolvable (npx playwright install chromium). Layout is NOT verified.');
        process.exit(77);
    }
    const { chromium } = pw;

    // languages, from disk (the canon exposes only __setLanguage at runtime)
    const i18nPath = path.join(pluginRoot, 'Source', 'ui', 'public', 'js', 'i18n.js');
    const m = fs.readFileSync(i18nPath, 'utf8').match(/export\s+const\s+LANGUAGES\s*=\s*\[([^\]]*)\]/);
    const LANGS = m ? m[1].split(',').map((s) => s.trim().replace(/^['"]|['"]$/g, '')).filter(Boolean) : ['en'];
    check(LANGS[0] === 'en' && LANGS.length >= 2, `LANGUAGES parsed from js/i18n.js: ${LANGS.join(', ')}`);

    // states: the Terrain walk of tests/i18n-states.json (cumulative, no reload)
    const statesFile = path.join(pluginRoot, 'tests', 'i18n-states.json');
    const allStates = fs.existsSync(statesFile) ? JSON.parse(fs.readFileSync(statesFile, 'utf8')) : [];
    const terrainStates = allStates.filter((s) => typeof s.name === 'string' && s.name.startsWith('terrain'));
    check(terrainStates.length >= 12, `${terrainStates.length} Terrain states from tests/i18n-states.json (need >= 12)`);

    const built = S.buildRoot(PLUGIN, { repoRoot });
    const misses = [];
    const srv = await S.serve(built.root, (rel) => misses.push(rel));
    const browser = await chromium.launch();
    const ctx = await browser.newContext({ viewport: { width: SHIP_W, height: SHIP_H }, deviceScaleFactor: 1 });
    const page = await ctx.newPage();
    const pageErrors = [];
    page.on('pageerror', (e) => pageErrors.push(String(e && e.message ? e.message : e)));

    await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
    await page.waitForTimeout(900);
    if (INJECT_CSS) await page.addStyleTag({ content: INJECT_CSS });

    const hasSwitch = await page.evaluate(() => typeof window.__setLanguage === 'function');
    check(hasSwitch, 'window.__setLanguage is exposed (canon present)');
    const terrainTab = await page.evaluate(() => !!document.getElementById('terrain-tab') && document.querySelectorAll('.tab-bar .tab').length === 5);
    check(terrainTab, '#terrain-tab exists and the tab bar has five tabs');

    const TABS = ['synth', 'mod', 'tuning', 'effects', 'terrain'];
    const switchTab = async (t) => { await page.evaluate((n) => window.switchTab(n), t); await page.waitForTimeout(120); };
    const setLang = async (l) => { await page.evaluate((x) => window.__setLanguage(x), l); await page.waitForTimeout(180); };
    const closeSettings = async () => { await page.evaluate(() => { const p = document.getElementById('settings-popover'); if (p) p.classList.remove('visible'); }); };

    const pinnedByLang = {};
    const measured = { budget: {}, right: {}, widths: {} };

    // ── A. default state: every tab x every language ──
    for (const lang of LANGS) {
        await setLang(lang);
        head(`default state — ${lang}`);
        for (const tab of TABS) {
            await switchTab(tab);
            const t = await page.evaluate(PROBE_TAB);
            check(t && t.id === `${tab}-tab`, `[${lang}/${tab}] active tab is #${tab}-tab`);
            if (!t) continue;
            measured.budget[`${lang}/${tab}`] = `${t.sh}/${t.ch}`;
            check(t.sh === t.ch && t.ch === TAB_BUDGET, `[${lang}/${tab}] scrollHeight === clientHeight === ${TAB_BUDGET} (scroll ${t.sh}, client ${t.ch})`);
            check(t.sw === t.cw, `[${lang}/${tab}] scrollWidth === clientWidth (scroll ${t.sw}, client ${t.cw})`);

            if (tab === 'synth') {
                const rows = await page.evaluate(PROBE_OSC_PARAMS);
                check(rows.length === 2, `[${lang}/synth] two .osc-params rows found (${rows.length})`);
                rows.forEach((r, i) => {
                    check(r.zeroWidth === 0, `[${lang}/synth] .osc-params[${i}] children all laid out (${r.zeroWidth} zero-width)`);
                    check(r.maxTop - r.minTop < r.minH / 2, `[${lang}/synth] .osc-params[${i}] is ONE row (${r.n} children, top spread ${(r.maxTop - r.minTop).toFixed(2)} px < half the shortest child ${(r.minH / 2).toFixed(1)})`);
                    check(r.right <= OSC_PARAMS_RIGHT_MAX, `[${lang}/synth] .osc-params[${i}] rightmost child right ${r.right.toFixed(2)} <= ${OSC_PARAMS_RIGHT_MAX}`);
                    measured.right[`${lang}/osc-params[${i}]`] = r.right.toFixed(2);
                });
                const hits = await page.evaluate(PROBE_HITS, ['#canvasWrapA .osc-view-btn[data-view="3d"]', '#canvasWrapA .osc-view-btn[data-view="wave"]', '#canvasWrapB .osc-view-btn[data-view="3d"]', '#canvasWrapB .osc-view-btn[data-view="wave"]']);
                for (const h of hits) check(h.ok === true, `[${lang}/synth] elementFromPoint hits ${h.sel}${h.ok ? '' : ` (got ${h.missing ? 'MISSING' : h.zero ? 'zero-size' : h.hit})`}`);
            }
            if (tab === 'mod') {
                const opts = await page.evaluate(() => Array.from(document.querySelectorAll('.mod-col-dst select')).map((s) => s.options.length));
                check(opts.length === 16 && opts.every((n) => n === 46), `[${lang}/mod] every .mod-col-dst select has 46 options (${opts.length} selects: ${[...new Set(opts)].join(',')})`);
            }
            if (tab === 'terrain') {
                const ov = await page.evaluate(PROBE_TERRAIN_REGIONS);
                for (const o of ov) check(!o.missing && o.area === 0, `[${lang}/terrain] ${o.a} x ${o.b} overlap ${o.missing ? 'MISSING' : o.area.toFixed(2)} px²`);
                // #select-terEdge sits in #terrain-image-row, inert (pointer-events: none) unless Terrain = Imported… — hit-tested in the terrain-imported state below
                const hits = await page.evaluate(PROBE_HITS, ['#btn-terImport', '#terrain-view-wrap .osc-view-btn[data-view="3d"]', '#terrain-view-wrap .osc-view-btn[data-view="wave"]', '#seg-terQuality .terrain-seg-btn[data-idx="0"]', '#seg-terQuality .terrain-seg-btn[data-idx="1"]', '#seg-terQuality .terrain-seg-btn[data-idx="2"]', '#select-terTerrain', '#select-terOrbit', '.tab-bar .tab[data-tab="terrain"]']);
                const inert = await page.evaluate(() => document.getElementById('terrain-image-row').classList.contains('is-inert'));
                check(inert, `[${lang}/terrain] #terrain-image-row is inert at default (Terrain != Imported…)`);
                for (const h of hits) check(h.ok === true, `[${lang}/terrain] elementFromPoint hits ${h.sel}${h.ok ? '' : ` (got ${h.missing ? 'MISSING' : h.zero ? 'zero-size' : h.hit})`}`);
                const vf = await page.evaluate(() => { const e = document.getElementById('val-terFreq'); return e ? e.textContent : null; });
                check(vf === '1.00×', `[${lang}/terrain] #val-terFreq reads 1.00× at default (got ${JSON.stringify(vf)})`);
                await page.evaluate(() => document.getElementById('settings-popover').classList.add('visible'));
                pinnedByLang[lang] = await page.evaluate(PROBE_PINNED);
                await closeSettings();
            }
        }
    }

    // ── B. width-pinned [data-i18n] border-boxes equal across languages ──
    head('width-pinned [data-i18n] boxes across languages');
    const en = pinnedByLang.en || {};
    const keys = Object.keys(en);
    check(keys.length === 5 + 2 + 3 + 1, `${keys.length} pinned boxes measured in en (expect 11: .tab x5, .wt-osc-btn x2, .terrain-seg-btn x3, #tips-toggle)`);
    for (const key of keys) {
        const e = en[key];
        check(e !== null, `[en] ${key} present`);
        if (!e) continue;
        measured.widths[key] = `${e.w.toFixed(2)}`;
        for (const lang of LANGS.filter((l) => l !== 'en')) {
            const o = (pinnedByLang[lang] || {})[key];
            check(o && Math.abs(o.w - e.w) <= PIN_TOL && Math.abs(o.h - e.h) <= PIN_TOL,
                `[${lang}] ${key} border-box equals en (en ${e.w.toFixed(2)} x ${e.h.toFixed(2)}, ${lang} ${o ? `${o.w.toFixed(2)} x ${o.h.toFixed(2)}` : 'MISSING'})`);
        }
    }

    // ── C. the Terrain states, cumulative, every language ──
    for (const lang of LANGS) {
        await setLang(lang);
        await closeSettings();
        head(`Terrain states — ${lang}`);
        for (const state of terrainStates) {
            if (state.click || state.dblclick) {
                const sel = state.click || state.dblclick;
                const el = await page.$(sel);
                if (!el) { check(false, `[${lang}/${state.name}] selector ${sel} exists`); continue; }
                if (state.dblclick) await el.dblclick({ force: true }); else await el.click({ force: true });
                await page.waitForTimeout(250);
            }
            if (state.eval) {
                try { await page.evaluate((src) => { (0, eval)(src); }, state.eval); }
                catch (e) { check(false, `[${lang}/${state.name}] eval ran — ${e.message}`); continue; }
                await page.waitForTimeout(350);
            }
            const t = await page.evaluate(PROBE_TAB);
            if (!t) { check(false, `[${lang}/${state.name}] an active tab exists`); continue; }
            check(t.sh === t.ch && t.ch === TAB_BUDGET, `[${lang}/${state.name}] #${t.id} scrollHeight === clientHeight === ${TAB_BUDGET} (scroll ${t.sh}, client ${t.ch})`);
            check(t.sw === t.cw, `[${lang}/${state.name}] #${t.id} scrollWidth === clientWidth (scroll ${t.sw}, client ${t.cw})`);
            if (t.id === 'terrain-tab') {
                const ov = await page.evaluate(PROBE_TERRAIN_REGIONS);
                for (const o of ov) check(!o.missing && o.area === 0, `[${lang}/${state.name}] ${o.a} x ${o.b} overlap ${o.missing ? 'MISSING' : o.area.toFixed(2)} px²`);
            }
            if (state.name === 'terrain-imported') {
                const inert = await page.evaluate(() => document.getElementById('terrain-image-row').classList.contains('is-inert'));
                check(!inert, `[${lang}/${state.name}] #terrain-image-row un-greys when Terrain = Imported…`);
                const hits = await page.evaluate(PROBE_HITS, ['#select-terEdge', '[data-knob="terBlur"] .knob-visual']);
                for (const h of hits) check(h.ok === true, `[${lang}/${state.name}] elementFromPoint hits ${h.sel}${h.ok ? '' : ` (got ${h.missing ? 'MISSING' : h.zero ? 'zero-size' : h.hit})`}`);
            }
            if (t.id === 'synth-tab') {
                const rows = await page.evaluate(PROBE_OSC_PARAMS);
                rows.forEach((r, i) => check(r.maxTop - r.minTop < r.minH / 2 && r.right <= OSC_PARAMS_RIGHT_MAX,
                    `[${lang}/${state.name}] .osc-params[${i}] one row, right ${r.right.toFixed(2)} <= ${OSC_PARAMS_RIGHT_MAX}`));
            }
        }
        // put the walk back to its start for the next language (the states end on the Synth tab)
        await switchTab('synth');
    }

    head('runtime');
    check(pageErrors.length === 0, `no page errors (${pageErrors.length})${pageErrors.length ? ': ' + pageErrors.slice(0, 3).join(' | ') : ''}`);
    const realMisses = misses.filter((r) => r !== '/favicon.ico');
    check(realMisses.length === 0, `no 404 subresources (${realMisses.length})${realMisses.length ? ': ' + realMisses.slice(0, 5).join(' ') : ''}`);

    if (verbose) console.log('\nmeasured:', JSON.stringify(measured, null, 2));
    else console.log(`\nmeasured: budget ${[...new Set(Object.values(measured.budget))].join(' ')} | .osc-params right ${Object.entries(measured.right).map(([k, v]) => `${k}=${v}`).join(' ')} | pinned widths ${Object.entries(measured.widths).map(([k, v]) => `${k}=${v}`).join(' ')}`);

    await browser.close();
    await srv.close();
    fs.rmSync(built.root, { recursive: true, force: true });

    console.log(`\n${failed === 0 ? 'ALL PASS' : 'FAILED'} — ${passed} passed, ${failed} failed`);
    process.exit(failed === 0 ? 0 : 2);
}

main().catch((e) => { console.error(e); process.exit(99); });
