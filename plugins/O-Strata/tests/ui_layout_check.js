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

    Stage 3 Round B (PLAN Decision 45) adds four sections after the walk:
      webgl        #terrainView 700 x 540 inside the 704 x 544 wrap at DPR 1 AND in a
                   deviceScaleFactor: 2 context (backing 700·DPR x 540·DPR),
                   window.__strataScene.kind === 'webgl2', lastGLError() === 0 after the
                   R32F upload, debugLoseContext() → no page error, debugRestoreContext()
                   → a frame draws again (the perf ring advances)
      fallback     a context whose getContext('webgl2') returns null: Canvas 2D path,
                   #terrain-webgl-unavailable visible in every language, every Terrain-tab
                   layout row re-run
      interaction  page.mouse drag / wheel / ⌥-drag / ⌥-click on #terrainView move
                   oscAOrbCX/CY / oscAPos / oscAOrbRot with ONE sliderDragStarted/Ended
                   pair per gesture (spy on the stub instances); after Osc B the same
                   drag moves oscB* and leaves oscA* alone
      drop         a synthetic File dropped on #terrainView / #canvasWrapB → exactly one
                   importTerrainImageData call with the right osc / name / bytes
                   (window.__stubNativeCalls); > 2 MiB and a .jpg name → the notice, no call
    plus source greps (placeholder token gone, the module embedded AND served, the
    four natives referenced by the page AND registered by the editor).

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

    // Round B (Decision 45): the placeholder is gone, the module is embedded AND served, the natives exist on both sides
    const placeholder = (html.match(/PLACEHOLDER \(Stage 3 Round B/g) || []).length;
    check(placeholder === 0, `placeholder token "PLACEHOLDER (Stage 3 Round B" count 0 in index.html (${placeholder})`);
    const cmake = fs.readFileSync(path.join(pluginRoot, 'CMakeLists.txt'), 'utf8');
    const bd = cmake.match(/juce_add_binary_data\s*\([\s\S]*?\)/g) || [];
    check(bd.length === 1 && /Source\/ui\/public\/js\/terrain-view\.js/.test(bd[0]), `js/terrain-view.js is in the ONE juce_add_binary_data SOURCES block (${bd.length} block(s))`);
    check(/"\/js\/terrain-view\.js"/.test(cpp) && /BinaryData::terrainview_js\b/.test(cpp), 'PluginEditor.cpp serves /js/terrain-view.js from a getResource() branch (BinaryData::terrainview_js)');
    check(/from '\.\/js\/terrain-view\.js'/.test(html), "index.html imports './js/terrain-view.js' from its module script");
    for (const fn of ['requestTerrainRepush', 'reportViewPerf', 'chooseTerrainImage', 'importTerrainImageData']) {
        const inHtml = new RegExp(`getNativeFunction\\s*\\(\\s*['"]${fn}['"]`).test(html);
        const inCpp  = new RegExp(`withNativeFunction\\s*\\(\\s*"${fn}"`).test(cpp);
        check(inHtml && inCpp, `native ${fn} is referenced by index.html AND registered by PluginEditor.cpp (${inHtml} / ${inCpp})`);
    }
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


    // ══ D. Round B (PLAN Decision 45): webgl / fallback / interaction / drop ══
    await setLang('en');
    await closeSettings();
    await switchTab('terrain');
    const HM_PUSH = (terrainStates.find((s) => s.name === 'terrain-heightmap-pushed') || {}).eval;
    check(typeof HM_PUSH === 'string', 'state terrain-heightmap-pushed exists (its eval seeds the heightmap for the webgl section)');

    const PROBE_VIEW = () => {
        const c = document.getElementById('terrainView'), w = document.getElementById('terrain-view-wrap'), sc = window.__strataScene;
        return { cw: c.clientWidth, ch: c.clientHeight, bw: c.width, bh: c.height, wow: w.offsetWidth, woh: w.offsetHeight,
                 kind: sc ? sc.kind : null, dpr: window.devicePixelRatio, n: window.__strataPerf ? window.__strataPerf.snapshot().n : -1 };
    };
    const TERRAIN_HITS = ['#btn-terImport', '#terrain-view-wrap .osc-view-btn[data-view="3d"]', '#terrain-view-wrap .osc-view-btn[data-view="wave"]', '#seg-terQuality .terrain-seg-btn[data-idx="0"]', '#seg-terQuality .terrain-seg-btn[data-idx="1"]', '#seg-terQuality .terrain-seg-btn[data-idx="2"]', '#select-terTerrain', '#select-terOrbit', '.tab-bar .tab[data-tab="terrain"]'];

    async function webglSection(pg, errs, dpr) {
        head(`webgl — DPR ${dpr}`);
        await pg.evaluate(() => window.switchTab('terrain')); await pg.waitForTimeout(150);
        if (HM_PUSH) { await pg.evaluate((src) => { (0, eval)(src); }, HM_PUSH); await pg.waitForTimeout(250); }
        const v = await pg.evaluate(PROBE_VIEW);
        check(v.dpr === dpr, `[dpr${dpr}] window.devicePixelRatio === ${dpr} (${v.dpr})`);
        check(v.cw === 700 && v.ch === 540, `[dpr${dpr}] #terrainView is 700 x 540 CSS px (${v.cw} x ${v.ch})`);
        check(v.wow === 704 && v.woh === 544, `[dpr${dpr}] #terrain-view-wrap border-box is 704 x 544 (${v.wow} x ${v.woh})`);
        check(v.bw === 700 * dpr && v.bh === 540 * dpr, `[dpr${dpr}] backing store is 700·${dpr} x 540·${dpr} (${v.bw} x ${v.bh})`);
        check(v.kind === 'webgl2', `[dpr${dpr}] window.__strataScene.kind === 'webgl2' (${v.kind})`);
        const err = await pg.evaluate(() => window.__strataScene.lastGLError());
        check(err === 0, `[dpr${dpr}] lastGLError() === 0 (NO_ERROR) after the R32F heightmap upload (${err})`);
        const before = errs.length, n0 = v.n;
        const lost = await pg.evaluate(() => window.__strataScene.debugLoseContext());
        await pg.waitForTimeout(250);
        check(lost === true && errs.length === before, `[dpr${dpr}] debugLoseContext() → no page error (${errs.length - before} new)`);
        const restored = await pg.evaluate(() => window.__strataScene.debugRestoreContext());
        await pg.waitForTimeout(400);
        await pg.evaluate(() => window.__stubEmit('terrainState', { osc: 'A', theta: 0.5, x: 0.1, y: 0.1, h: 0.2 }));
        await pg.waitForTimeout(250);
        const n1 = await pg.evaluate(() => window.__strataPerf.snapshot().n);
        check(restored === true && n1 > n0, `[dpr${dpr}] debugRestoreContext() → a frame draws again (perf ring n ${n0} → ${n1})`);
    }
    await webglSection(page, pageErrors, 1);
    {
        const ctx2 = await browser.newContext({ viewport: { width: SHIP_W, height: SHIP_H }, deviceScaleFactor: 2 });
        const page2 = await ctx2.newPage(); const errs2 = [];
        page2.on('pageerror', (e) => errs2.push(String(e && e.message ? e.message : e)));
        await page2.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
        await page2.waitForTimeout(900);
        await webglSection(page2, errs2, 2);
        check(errs2.length === 0, `[dpr2] no page errors in the DPR 2 context (${errs2.length})${errs2.length ? ': ' + errs2.slice(0, 2).join(' | ') : ''}`);
        await ctx2.close();
    }

    // fallback: getContext('webgl2') → null → the Canvas 2D path + the localised placeholder; every Terrain-tab layout row holds
    {
        head('fallback — forced-null webgl2');
        const ctx3 = await browser.newContext({ viewport: { width: SHIP_W, height: SHIP_H }, deviceScaleFactor: 1 });
        await ctx3.addInitScript(() => {
            const orig = HTMLCanvasElement.prototype.getContext;
            HTMLCanvasElement.prototype.getContext = function (type, ...rest) { return type === 'webgl2' ? null : orig.call(this, type, ...rest); };
        });
        const page3 = await ctx3.newPage(); const errs3 = [];
        page3.on('pageerror', (e) => errs3.push(String(e && e.message ? e.message : e)));
        await page3.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
        await page3.waitForTimeout(900);
        await page3.evaluate(() => window.switchTab('terrain')); await page3.waitForTimeout(150);
        if (HM_PUSH) { await page3.evaluate((src) => { (0, eval)(src); }, HM_PUSH); await page3.waitForTimeout(250); }
        const kind = await page3.evaluate(() => window.__strataScene && window.__strataScene.kind);
        check(kind === '2d', `[fallback] window.__strataScene.kind === '2d' (${kind})`);
        for (const lang of LANGS) {
            await page3.evaluate((x) => window.__setLanguage(x), lang); await page3.waitForTimeout(180);
            const badge = await page3.evaluate(() => { const e = document.getElementById('terrain-webgl-unavailable'); const cs = getComputedStyle(e); const r = e.getBoundingClientRect(); return { display: cs.display, text: e.textContent.trim(), w: r.width, h: r.height, key: e.dataset.i18n }; });
            check(badge.display !== 'none' && badge.w > 0 && badge.h > 0 && badge.text.length > 0 && badge.key === 'label.webglUnavailable',
                `[fallback/${lang}] #terrain-webgl-unavailable visible with label.webglUnavailable (${badge.display}, ${badge.w.toFixed(1)} x ${badge.h.toFixed(1)}, "${badge.text}")`);
            const t = await page3.evaluate(PROBE_TAB);
            check(t && t.id === 'terrain-tab' && t.sh === t.ch && t.ch === TAB_BUDGET, `[fallback/${lang}] #terrain-tab scrollHeight === clientHeight === ${TAB_BUDGET} (${t ? `${t.sh}/${t.ch}` : 'no tab'})`);
            check(t && t.sw === t.cw, `[fallback/${lang}] #terrain-tab scrollWidth === clientWidth`);
            const ov = await page3.evaluate(PROBE_TERRAIN_REGIONS);
            for (const o of ov) check(!o.missing && o.area === 0, `[fallback/${lang}] ${o.a} x ${o.b} overlap ${o.missing ? 'MISSING' : o.area.toFixed(2)} px²`);
            const hits = await page3.evaluate(PROBE_HITS, TERRAIN_HITS);
            for (const h of hits) check(h.ok === true, `[fallback/${lang}] elementFromPoint hits ${h.sel}${h.ok ? '' : ` (got ${h.missing ? 'MISSING' : h.zero ? 'zero-size' : h.hit})`}`);
            const vf = await page3.evaluate(() => { const e = document.getElementById('val-terFreq'); return e ? e.textContent : null; });
            check(vf === '1.00×', `[fallback/${lang}] #val-terFreq reads 1.00× (got ${JSON.stringify(vf)})`);
            const v3 = await page3.evaluate(PROBE_VIEW);
            check(v3.cw === 700 && v3.ch === 540 && v3.bw === 700 && v3.bh === 540, `[fallback/${lang}] #terrainView 700 x 540 with a 700 x 540 backing store (${v3.cw} x ${v3.ch}, ${v3.bw} x ${v3.bh})`);
        }
        check(errs3.length === 0, `[fallback] no page errors (${errs3.length})${errs3.length ? ': ' + errs3.slice(0, 2).join(' | ') : ''}`);
        await ctx3.close();
    }

    // interaction (Decision 38): drag → CX / CY, wheel → Pos, ⌥-drag → Rot, ⌥-click → Rot = 0, A ↔ B repoint
    {
        head('interaction');
        await setLang('en'); await closeSettings(); await switchTab('terrain');
        await page.evaluate(() => { const b = document.querySelector('#terrain-osc-toggle .wt-osc-btn[data-osc="A"]'); if (b) b.click(); });
        await page.waitForTimeout(120);
        const SPY = ['oscAOrbCX', 'oscAOrbCY', 'oscAPos', 'oscAOrbRot', 'oscBOrbCX', 'oscBOrbCY'];
        await page.evaluate((ids) => {
            window.__gestureSpy = {};
            for (const id of ids) {
                const st = window.__stubStates.sliders.get(id);
                const rec = window.__gestureSpy[id] = { started: 0, ended: 0 };
                const s0 = st.sliderDragStarted.bind(st), e0 = st.sliderDragEnded.bind(st);
                st.sliderDragStarted = () => { rec.started++; s0(); };
                st.sliderDragEnded = () => { rec.ended++; e0(); };
            }
        }, SPY);
        const vals = () => page.evaluate((ids) => { const o = {}; for (const id of ids) o[id] = window.__stubStates.sliders.get(id).getNormalisedValue(); return o; }, SPY);
        const spy = () => page.evaluate(() => JSON.parse(JSON.stringify(window.__gestureSpy)));
        const resetSpy = () => page.evaluate(() => { for (const k in window.__gestureSpy) { window.__gestureSpy[k].started = 0; window.__gestureSpy[k].ended = 0; } });
        const box = await page.locator('#terrainView').boundingBox();
        check(box && Math.round(box.width) === 700 && Math.round(box.height) === 540, `#terrainView bounding box 700 x 540 (${box ? `${box.width} x ${box.height}` : 'MISSING'})`);
        const cx = box.x + box.width / 2, cy = box.y + box.height / 2;
        // drag → Centre X / Y, one pair each, the point lands under the cursor
        const v0 = await vals();
        await page.mouse.move(cx - 60, cy + 20); await page.mouse.down(); await page.mouse.move(cx + 60, cy - 20, { steps: 6 }); await page.mouse.up();
        await page.waitForTimeout(150);
        const v1 = await vals(); let s = await spy();
        check(v1.oscAOrbCX !== v0.oscAOrbCX && v1.oscAOrbCY !== v0.oscAOrbCY, `drag across #terrainView moves oscAOrbCX (${v0.oscAOrbCX.toFixed(3)} → ${v1.oscAOrbCX.toFixed(3)}) and oscAOrbCY (${v0.oscAOrbCY.toFixed(3)} → ${v1.oscAOrbCY.toFixed(3)})`);
        check(s.oscAOrbCX.started === 1 && s.oscAOrbCX.ended === 1 && s.oscAOrbCY.started === 1 && s.oscAOrbCY.ended === 1, `drag = ONE Started / Ended pair on CX and on CY (CX ${s.oscAOrbCX.started}/${s.oscAOrbCX.ended}, CY ${s.oscAOrbCY.started}/${s.oscAOrbCY.ended})`);
        const landed = await page.evaluate(({ px, py }) => { const r = document.getElementById('terrainView').getBoundingClientRect(); return window.__strataScene.unproject(px - r.left, py - r.top); }, { px: cx + 60, py: cy - 20 });
        check(landed && Math.abs((landed[0] + 1) / 2 - v1.oscAOrbCX) < 2e-3 && Math.abs((landed[1] + 1) / 2 - v1.oscAOrbCY) < 2e-3,
            `the centre landed under the cursor (unproject ${landed ? `${landed[0].toFixed(3)}, ${landed[1].toFixed(3)}` : 'null'} vs CX/CY ${(v1.oscAOrbCX * 2 - 1).toFixed(3)}, ${(v1.oscAOrbCY * 2 - 1).toFixed(3)})`);
        // wheel → Orbit Size, ≤ 0.05 per event, one pair per burst
        await resetSpy();
        await page.mouse.move(cx, cy); await page.mouse.wheel(0, 120); await page.waitForTimeout(350);
        const v2 = await vals(); s = await spy();
        const dPos = v2.oscAPos - v1.oscAPos;
        check(dPos !== 0 && Math.abs(dPos) <= 0.05 + 1e-6, `wheel (deltaY 120) moves oscAPos by ≤ 0.05 (Δ ${dPos.toFixed(4)})`);
        check(s.oscAPos.started === 1 && s.oscAPos.ended === 1, `wheel burst = ONE Started / Ended pair after 250 ms (${s.oscAPos.started}/${s.oscAPos.ended})`);
        // ⌥-drag → Rotation (0.5° / px)
        await resetSpy();
        await page.keyboard.down('Alt');
        await page.mouse.move(cx, cy); await page.mouse.down(); await page.mouse.move(cx + 40, cy, { steps: 4 }); await page.mouse.up();
        await page.keyboard.up('Alt'); await page.waitForTimeout(150);
        const v3 = await vals(); s = await spy();
        check(v3.oscAOrbRot !== v2.oscAOrbRot && Math.abs(v3.oscAOrbRot - (v2.oscAOrbRot + 40 * 0.5 / 360)) < 2e-3, `⌥-drag 40 px rotates oscAOrbRot by 20° (${v2.oscAOrbRot.toFixed(4)} → ${v3.oscAOrbRot.toFixed(4)})`);
        check(s.oscAOrbRot.started === 1 && s.oscAOrbRot.ended === 1 && s.oscAOrbCX.started === 0, `⌥-drag = ONE pair on Rot, none on CX (Rot ${s.oscAOrbRot.started}/${s.oscAOrbRot.ended}, CX ${s.oscAOrbCX.started})`);
        // ⌥-click (< 3 px) → Rotation = 0
        await resetSpy();
        await page.keyboard.down('Alt'); await page.mouse.move(cx + 10, cy + 10); await page.mouse.down(); await page.mouse.up(); await page.keyboard.up('Alt');
        await page.waitForTimeout(150);
        const v4 = await vals(); s = await spy();
        check(v4.oscAOrbRot === 0 && s.oscAOrbRot.started === 1 && s.oscAOrbRot.ended === 1, `⌥-click (< 3 px) resets oscAOrbRot to 0 inside ONE pair (${v4.oscAOrbRot}, ${s.oscAOrbRot.started}/${s.oscAOrbRot.ended})`);
        // Osc B: the same drag moves oscB* and leaves oscA* alone
        await page.evaluate(() => document.querySelector('#terrain-osc-toggle .wt-osc-btn[data-osc="B"]').click()); await page.waitForTimeout(150);
        await resetSpy();
        await page.mouse.move(cx - 40, cy); await page.mouse.down(); await page.mouse.move(cx + 40, cy + 30, { steps: 5 }); await page.mouse.up();
        await page.waitForTimeout(150);
        const v5 = await vals(); s = await spy();
        check(v5.oscBOrbCX !== v4.oscBOrbCX && v5.oscBOrbCY !== v4.oscBOrbCY && v5.oscAOrbCX === v4.oscAOrbCX && v5.oscAOrbCY === v4.oscAOrbCY,
            `after Osc B the drag moves oscBOrbCX/CY (${v4.oscBOrbCX.toFixed(3)} → ${v5.oscBOrbCX.toFixed(3)}) and leaves oscAOrbCX/CY unchanged`);
        check(s.oscBOrbCX.started === 1 && s.oscBOrbCX.ended === 1 && s.oscAOrbCX.started === 0, `the Osc B gesture pairs on oscB* only (B ${s.oscBOrbCX.started}/${s.oscBOrbCX.ended}, A ${s.oscAOrbCX.started})`);
        await page.evaluate(() => document.querySelector('#terrain-osc-toggle .wt-osc-btn[data-osc="A"]').click()); await page.waitForTimeout(120);
    }

    // drop (Decisions 41, 44): a synthetic File through the real handlers; the stub records the native call
    {
        head('drop');
        const PNG_1x1 = 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAAAAAA6fptVAAAACklEQVR4nGNoAAAAggCBd81ytgAAAABJRU5ErkJggg==';   // a 67-byte 1 x 1 greyscale PNG
        const dropFile = (sel, name, big) => page.evaluate(({ sel, name, b64, big }) => {
            let bytes;
            if (big) { bytes = new Uint8Array(big); const sig = atob(b64); for (let i = 0; i < 8; i++) bytes[i] = sig.charCodeAt(i); }
            else { const bin = atob(b64); bytes = new Uint8Array(bin.length); for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i); }
            const file = new File([bytes], name, { type: 'image/png' });
            const dt = new DataTransfer(); dt.items.add(file);
            document.querySelector(sel).dispatchEvent(new DragEvent('drop', { bubbles: true, cancelable: true, dataTransfer: dt }));
        }, { sel, name, b64: PNG_1x1, big: big || 0 });
        const calls = () => page.evaluate(() => (window.__stubNativeCalls || []).filter((c) => c.name === 'importTerrainImageData').map((c) => ({ args: c.args })));
        const notice = () => page.evaluate(() => { const e = document.getElementById('terrain-notice'); return { visible: e.classList.contains('visible'), key: e.dataset.i18n, text: e.textContent.trim() }; });
        const hasSpy = await page.evaluate(() => Array.isArray(window.__stubNativeCalls));
        check(hasSpy, 'window.__stubNativeCalls is recorded by the generic stub');
        await page.evaluate(() => { if (window.__stubNativeCalls) window.__stubNativeCalls.length = 0; window.__showTerrainNotice(null, 0); });
        await dropFile('#terrainView', 'probe.png'); await page.waitForTimeout(400);
        let c = await calls();
        check(c.length === 1 && c[0].args[0] === 'A' && c[0].args[1] === 'probe.png' && c[0].args[2] === PNG_1x1,
            `drop of a 1 x 1 PNG on #terrainView → exactly one importTerrainImageData ('A', 'probe.png', <base64>) (${c.length} call(s)${c.length ? `: ${c[0].args[0]}, ${c[0].args[1]}, ${c[0].args[2] === PNG_1x1 ? 'bytes match' : 'bytes DIFFER'}` : ''})`);
        let n = await notice();
        check(!n.visible, `no notice after a successful drop (visible ${n.visible})`);
        await dropFile('#terrainView', 'big.png', 2 * 1024 * 1024 + 1); await page.waitForTimeout(400);
        c = await calls(); n = await notice();
        check(c.length === 1 && n.visible && n.key === 'label.importTooLarge' && n.text.length > 0, `a 2 MiB + 1 byte .png → label.importTooLarge notice ("${n.text}"), no new call (${c.length})`);
        await dropFile('#terrainView', 'photo.jpg'); await page.waitForTimeout(400);
        c = await calls(); n = await notice();
        check(c.length === 1 && n.visible && n.key === 'label.importFailed' && n.text.length > 0, `a .jpg name → label.importFailed notice ("${n.text}"), no new call (${c.length})`);
        await page.evaluate(() => window.__showTerrainNotice(null, 0));
        n = await notice();
        check(!n.visible && n.key === 'label.sourceMissing', `#terrain-notice restored to the STATUS-driven label.sourceMissing form (visible ${n.visible}, key ${n.key})`);
        await switchTab('synth');
        await dropFile('#canvasWrapB', 'probe.png'); await page.waitForTimeout(400);
        c = await calls();
        check(c.length === 2 && c[1].args[0] === 'B' && c[1].args[1] === 'probe.png', `drop on #canvasWrapB → importTerrainImageData ('B', 'probe.png', …) (${c.length} call(s)${c.length > 1 ? `: ${c[1].args[0]}` : ''})`);
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
