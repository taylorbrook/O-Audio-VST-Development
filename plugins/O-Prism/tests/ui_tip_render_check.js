/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    ui_tip_render_check.js — O-Prism hover-help, RENDERED.

    THE FIRST RUNNABLE GATE THIS PLUGIN HAS EVER HAD. `tests/` held
    i18n-states.json and ui-stub/generic-overrides.json before this file: data,
    both of them, read by check-ui-labels and by serve-ui. Nothing in
    plugins/O-Prism executed.

    ── WHY IT EXISTS ───────────────────────────────────────────────────────────

    NO OTHER GATE IN THIS REPO CAN SEE A RENDERED TOOLTIP.

      check-i18n      reads the table statically. 107 bindings pointing at 107
                      selectors that match nothing, on a page with no renderer at
                      all, is a PASS.
      check-ui-labels has no tooltip awareness whatsoever.
      boot-all-uis    counts aria-label and title. It never counts data-tip.

    So authoring 107 bodies into i18n.js and binding them, with no other change,
    would have shipped 107 invisible strings past three green gates. v1.21.0 of
    this plugin had NO #tooltip node, NO .tooltip rule and NO hover handler,
    which is exactly the state in which that failure is silent.

    ── THE FRAME IS 1200 x 800, READ NOT GUESSED ───────────────────────────────

    Pinned to the shipping size parsed out of PluginEditor.cpp's setSize(), never
    a default 1280x720 (pattern_tooltip_clamp_gate_viewport_sensitive). It is the
    LARGEST frame in the suite, which is why the negative control below SEARCHES
    for a plant size that overflows instead of reusing a habitual multiplier —
    O-Tremolo's 40x plant fit inside a 400 px frame and reported nothing, and a
    plant that fits is indistinguishable from a gate that cannot see.

    ── THE PAGE IS A FIVE-TAB DECK, AND FOUR TABS ARE ZERO-SIZE AT LOAD ────────

    `.tab-content` is `display: none` unless `.active` (index.html:317-325), so
    #mod-tab, #tuning-tab, #effects-tab and #wavetable-tab cannot be hovered or
    measured until they are opened. Every tab switch below goes through the
    page's OWN path — a click on the tab — and NEVER by stripping the class:
    stripping it measures a state the plugin only reaches by a click nobody made.
    The same rule governs the four LFO Division dropdowns, which are
    `display: none` until their Sync button is pressed.

    ── THE CONTROLS THAT WERE RUN ON THIS GATE ─────────────────────────────────

    An assertion that has only ever passed is indistinguishable from one that
    cannot fail. Six defects were planted, one at a time, and each was restored
    from a namespaced copy under scratchpad/O-Prism/m3/ — never with
    `git checkout --`, which takes the uncommitted fix along with the plant.
    Baseline: 2180 assertions, 0 failures.

      1. Delete `if (lastInputWasPointer) return;` from the focusin arm
         -> [7] FAILS, 3319 px2 of the gear's own tip lying across the settings
         popover the click had just opened. Note that `grep -c
         lastInputWasPointer` still returns 3 with the guard gone, which is why a
         static scan is a presence note here and never the control (O-Comp).
      2. Delete the guard AND the gate's own activeElement.blur()
         -> 2180 PASS. The blur IS load-bearing on this page; the full 2x2 is in
         the comment at section 7.
      3. Delete `if (pointerHeld) return;` from pointerover
         -> [7b] FAILS: dragging oscAPos across oscALevel opens the neighbour's
         tip over the knob being turned.
      4. Delete the pointerup / pointercancel release
         -> 1290 FAIL, 890 pass. The flag latches on the first click and no tip
         ever opens again. A guard that is only ever set is a permanent off
         switch, and here it is a loud one rather than a silent one.
      5. Break ONE binding's wrapper (`.dropdown-group` -> `.no-such-wrapper`)
         -> [1] FAILS and [2], [3], [4] all still PASS, in both languages,
         because applyI18n falls back `el.closest(w) || el` and the tip opens on
         the wrong-sized cell. That is why [1] is a hard FAIL and never a warning
         (M2 finding 3, from O-Reed).
      6. Point two bindings at one node (two knobs walking to `.osc-params`)
         -> [1b] FAILS, plus every [3] on both rows, because the second write
         overwrote the first. check-i18n reports two bound tips either way.

    Sections 6 and 6b are themselves controls and run on every invocation.

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const { pathToFileURL } = require('url');

const REPO_ROOT = path.resolve(__dirname, '..', '..', '..');
const S = require(path.join(REPO_ROOT, 'scripts', 'serve-ui.js'));

const PLUGIN  = 'O-Prism';
const verbose = process.argv.includes('--verbose');

let failed = 0;
let passes = 0;

function check(cond, msg, detail) {
    if (cond) { ++passes; if (verbose) console.log(`  PASS: ${msg}`); }
    else      { ++failed; console.log(`  FAIL: ${msg}`); if (detail) console.log(`        ${detail}`); }
    return !!cond;
}

function note(msg) { console.log(`   ${msg}`); }

// ── the table, loaded as a MODULE rather than regex-scraped ─────────────────
//
// i18n.js is an ES module inside a package with no "type": "module", so a bare
// dynamic import() of it is parsed as CommonJS and throws on the first `export`.
// Copying it to a .mjs in a per-run mkdtemp is the honest fix: the gate then
// compares against the REAL exported objects — including the string
// concatenations all 107 bodies are authored with — rather than against a
// regex's idea of them (pattern_test_fixture_mirrors_drift_silently).
async function loadTable() {
    const src = path.join(REPO_ROOT, 'plugins', PLUGIN, 'Source', 'ui', 'public', 'js', 'i18n.js');
    if (!fs.existsSync(src)) throw new Error(`i18n.js not found at ${src}`);

    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'oprism-i18n-'));
    const dst = path.join(dir, 'i18n.mjs');
    fs.copyFileSync(src, dst);
    try {
        return await import(pathToFileURL(dst).href);
    } finally {
        fs.rmSync(dir, { recursive: true, force: true });
    }
}

// ── reading the rendered surface ────────────────────────────────────────────
//
// The body is read as the concatenation of the surface's own TEXT NODES and the
// title as .tip-title's textContent, because that is exactly how the renderer
// builds it: createElement + appendChild(createTextNode). Reading
// tip.textContent whole would glue the title onto the front of the body and turn
// assertion 3 into a substring test by accident.
const READ_TIP = () => {
    const tip = document.getElementById('tooltip');
    if (!tip) return null;
    const cs = getComputedStyle(tip);
    const r  = tip.getBoundingClientRect();
    const titleEl = tip.querySelector('.tip-title');
    let body = '';
    for (const n of tip.childNodes) if (n.nodeType === 3) body += n.nodeValue;
    return {
        shown: tip.classList.contains('show'),
        visibility: cs.visibility,
        opacity: cs.opacity,
        ariaHidden: tip.getAttribute('aria-hidden'),
        pointerEvents: cs.pointerEvents,
        position: cs.position,
        zIndex: cs.zIndex,
        maxWidth: cs.maxWidth,
        title: titleEl ? titleEl.textContent : '',
        body,
        rect: { left: r.left, top: r.top, right: r.right, bottom: r.bottom, w: r.width, h: r.height },
    };
};

const EPS = 0.01;

function outsideViewport(rect, W, H) {
    const out = [];
    if (rect.left   < -EPS)     out.push(`left ${rect.left.toFixed(2)} < 0`);
    if (rect.top    < -EPS)     out.push(`top ${rect.top.toFixed(2)} < 0`);
    if (rect.right  > W + EPS)  out.push(`right ${rect.right.toFixed(2)} > ${W}`);
    if (rect.bottom > H + EPS)  out.push(`bottom ${rect.bottom.toFixed(2)} > ${H}`);
    return out;
}

// Which tab each anchor lives on, so the driver can open the right one. READ OFF
// THE MARKUP, and the first draft of this map got it wrong in a way worth
// recording: the Performance panel — PB Range, Glide Mode and Glide — reads like
// tuning and IS tuning, but it sits on #synth-tab (index.html:1326-1333), not on
// #tuning-tab (which starts at :1509). Only the A4 knob and the Stretch slider
// are on the Tuning tab. Guessing cost three "anchor is not hoverable" failures.
//
// Everything not listed here is on #synth-tab, which is `.active` at load, or is
// chrome in the header / footer, outside the tab deck entirely.
const TAB_OF = (sel) => {
    if (/^#(toggle|knob|select)-(delay|chorus|dist|reverb|eq)/.test(sel)) return 'effects';
    if (sel === '#ref-pitch-knob' || sel === '#octave-stretch') return 'tuning';
    return null;   // synth tab, or header/footer chrome
};

// `display: none` at load, revealed only by that LFO's own Sync button.
const HIDDEN_UNTIL_SYNC = new Set([
    '#select-lfo1Division', '#select-lfo2Division',
    '#select-lfo3Division', '#select-lfo4Division',
]);
// The mirror image: the Rate knob is hidden once Sync is ON.
const HIDDEN_WHEN_SYNC = new Set([
    '#knob-lfo1Rate', '#knob-lfo2Rate', '#knob-lfo3Rate', '#knob-lfo4Rate',
]);

(async () => {
    console.log(`ui_tip_render_check — ${PLUGIN} hover-help, RENDERED\n`);

    const chromiumPkg = S.resolvePlaywright();
    if (!chromiumPkg) {
        console.error('Playwright is not resolvable. NOTHING was verified — this is not a pass.');
        console.error('  npm i -D playwright   (or run under npx playwright)');
        process.exit(77);
    }
    const { chromium } = chromiumPkg;

    const size = S.readEditorSize(PLUGIN, REPO_ROOT);
    if (!size) {
        console.error('Could not read setSize() out of Source/PluginEditor.cpp. A clamp gate that '
                    + 'cannot learn the shipping frame would measure a viewport nobody ships.');
        process.exit(1);
    }
    const W = size.w, H = size.h;
    note(`shipping frame from PluginEditor.cpp setSize(): ${W} x ${H}`);

    const table = await loadTable();
    const { I18N, TIP_BINDINGS } = table;
    note(`table: ${Object.keys(I18N).length} I18N entries, ${TIP_BINDINGS.length} TIP_BINDINGS rows`);

    if (!Array.isArray(TIP_BINDINGS) || TIP_BINDINGS.length === 0) {
        check(false, '[0] TIP_BINDINGS is non-empty — a render gate over zero bindings is vacuous');
        process.exit(1);
    }

    // The three chrome anchors: #gear-btn is always visible, but #lang-select
    // and (since v1.23.0) #tips-toggle live inside `.settings-popover`, which is
    // `display: none` until the gear is clicked.
    const POPOVER_ONLY = new Set(['#lang-select', '#tips-toggle']);

    const built = S.buildRoot(PLUGIN, { repoRoot: REPO_ROOT });
    const misses = [];
    const { port, close } = await S.serve(built.root, (u) => misses.push(u));

    const pageErrors = [];
    const consoleErrors = [];
    const tipWarns = [];

    const browser = await chromium.launch();
    const page = await browser.newPage({ viewport: { width: W, height: H }, deviceScaleFactor: 1 });

    page.on('pageerror', (e) => pageErrors.push(String(e)));
    page.on('console', (m) => {
        const t = m.text();
        if (m.type() === 'error') consoleErrors.push(t);
        if (/tip target not found/.test(t)) tipWarns.push(t);
    });

    try {
        await page.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });
        // The tuning IIFE starts on setTimeout(tryInit, 300) and then awaits
        // several native fns, so 900 ms is the settle point at which the Tuning
        // tab's interval list and library have actually been injected.
        await page.waitForTimeout(900);

        const vp = await page.evaluate(() => ({ w: window.innerWidth, h: window.innerHeight }));
        check(vp.w === W && vp.h === H,
            `[0] viewport really is ${W} x ${H} — got ${vp.w} x ${vp.h}`);

        check(pageErrors.length === 0,
            '[0] no uncaught page error during load (a TDZ throw takes every later initializer)',
            pageErrors.slice(0, 3).join(' | '));

        // ── the surface itself ──────────────────────────────────────────────
        const surface = await page.evaluate(READ_TIP);
        // `surface && typeof surface === 'object'`, not `!== null`: a
        // page.evaluate whose argument is a function-source STRING returns the
        // function object, which is unserialisable and arrives as undefined —
        // and `undefined !== null` is a PASS over a surface nobody read. That
        // false pass really happened, on O-Bass's first run of this gate.
        check(surface && typeof surface === 'object',
            '[0] the #tooltip surface exists in the DOM and was READ (not undefined)');
        if (surface && typeof surface === 'object') {
            check(surface.position === 'fixed',
                `[0] the surface is position: fixed — got ${surface.position}`);
            check(surface.pointerEvents === 'none',
                '[0] the surface is pointer-events: none (or it steals the hover keeping it open) '
                + `— got ${surface.pointerEvents}`);
            check(surface.visibility === 'hidden' || surface.opacity === '0',
                '[0] the surface is HIDDEN at rest '
                + `(visibility ${surface.visibility}, opacity ${surface.opacity})`);
            // #lang-select is itself an anchor and lives INSIDE .settings-popover
            // (z-index 9998); the preset menu is 9999. A surface that did not
            // out-stack both would open BEHIND the control that revealed it.
            check(Number(surface.zIndex) > 9999,
                `[0] the surface out-stacks the preset menu (9999) and the settings popover (9998) `
                + `— z-index ${surface.zIndex}`);
            note(`surface: ${surface.position}, z-index ${surface.zIndex}, `
               + `max-width ${surface.maxWidth}, visibility ${surface.visibility}`);
        }

        // ── the page's own state drivers ────────────────────────────────────
        //
        // Every one of these goes through a real click on a real control. M2's
        // finding 5 is the governing rule and this page is where it bites
        // hardest: FOUR of the five tabs and FOUR dropdowns are display:none at
        // load, and stripping any of those classes would measure a state the
        // plugin only reaches by a click nobody made.
        let currentTab = 'synth';
        async function openTab(name) {
            const want = name || 'synth';
            if (want === currentTab) return want;
            await page.click(`.tab[data-tab="${want}"]`);
            await page.waitForTimeout(160);
            currentTab = await page.evaluate(() => {
                const a = document.querySelector('.tab-content.active');
                return a ? a.id.replace('-tab', '') : null;
            });
            return currentTab;
        }

        let popoverOpen = false;
        async function setPopover(open) {
            const isOpen = await page.evaluate(() =>
                document.getElementById('settings-popover').classList.contains('visible'));
            if (isOpen !== open) {
                await page.click('#gear-btn');
                await page.waitForTimeout(160);
            }
            popoverOpen = await page.evaluate(() =>
                document.getElementById('settings-popover').classList.contains('visible'));
            return popoverOpen;
        }

        // Sync state of the four LFOs, driven by clicking each Sync button —
        // never by editing style.display.
        let lfoSynced = false;
        async function setLfoSync(on) {
            if (lfoSynced === on) return lfoSynced;
            await openTab('synth');
            for (let i = 1; i <= 4; i++) {
                await page.click(`#toggle-lfo${i}Sync`);
                await page.waitForTimeout(40);
            }
            await page.waitForTimeout(120);
            lfoSynced = await page.evaluate(() =>
                document.getElementById('lfo1-div-wrap').style.display !== 'none');
            return lfoSynced;
        }

        // ── 1. EVERY TIP_BINDINGS SELECTOR RESOLVES ─────────────────────────
        //
        // applyI18n's own failure here is a console.warn, which boot-all-uis
        // prints and nothing fails on. A binding that finds no element is a FAIL
        // in this gate, not a warning — and it has to be, because
        // `el.closest(w) || el` FALLS BACK to the bare element, so a broken
        // WRAPPER leaves assertions 2, 3 and 4 all green while the tip opens on
        // the wrong-sized cell (M2 finding 3, from O-Reed).
        //
        // Resolution is measured with the popover OPEN and every tab visited,
        // because `display: none` gives a zero rect and the area numbers below
        // would all read 0.
        console.log('-- 1. binding resolution');
        await setPopover(true);
        const resolution = [];
        for (const tab of ['synth', 'effects', 'tuning']) {
            await openTab(tab);
            const rows = TIP_BINDINGS.filter(b => (TAB_OF(b[0]) || 'synth') === tab);
            const got = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
                const el = document.querySelector(sel);
                if (!el) return { sel, key, found: false };
                const target = wrapper ? (el.closest(wrapper) || el) : el;
                const tr = target.getBoundingClientRect();
                const er = el.getBoundingClientRect();
                return {
                    sel, key, found: true,
                    wrapperDeclared: !!wrapper,
                    wrapperResolved: wrapper ? !!el.closest(wrapper) : null,
                    anchorTag: target.tagName.toLowerCase(),
                    anchorId: target.id || '',
                    anchorClass: typeof target.className === 'string' ? target.className : '',
                    hasTip: target.hasAttribute('data-tip'),
                    hasTitle: target.hasAttribute('data-tip-title'),
                    anchorIsSelf: target === el,
                    area: Math.round(tr.width * tr.height),
                    selfArea: Math.round(er.width * er.height),
                    // The path from <body>, so "every binding lands on a DISTINCT
                    // node" is checkable across three separate page.evaluate calls.
                    nodePath: (() => {
                        const parts = [];
                        for (let n = target; n && n.nodeType === 1 && n !== document.body; n = n.parentElement)
                            parts.unshift(n.id ? '#' + n.id
                                : n.tagName.toLowerCase() + ':nth-child('
                                  + (1 + [...n.parentElement.children].indexOf(n)) + ')');
                        return parts.join('>');
                    })(),
                };
            }), rows.map(b => [b[0], b[1], b[2] || null]));
            resolution.push(...got);
        }

        for (const r of resolution) {
            check(r.found, `[1] selector resolves: ${r.sel}  (key ${r.key})`);
            if (!r.found) continue;
            check(r.hasTip && r.hasTitle,
                `[1] applyI18n wrote data-tip + data-tip-title onto the anchor for ${r.sel}`);
            if (r.wrapperResolved === false)
                check(false, `[1] the declared wrapper for ${r.sel} did NOT resolve — the tip fell `
                           + 'back onto the id\'d node, which is not the hover target it was bound to');
            if (verbose) note(`${r.sel} -> ${r.anchorTag}${r.anchorId ? '#' + r.anchorId : ''}`
                            + (r.anchorIsSelf ? ' (self)' : ' (wrapper)')
                            + ` area ${r.selfArea} -> ${r.area} px2`);
        }
        check(resolution.length === TIP_BINDINGS.length,
            `[1] every TIP_BINDINGS row was resolved exactly once — ${resolution.length} of `
            + `${TIP_BINDINGS.length}`);
        check(tipWarns.length === 0,
            '[1] applyI18n logged no "tip target not found" warning',
            tipWarns.slice(0, 3).join(' | '));

        // ── 1b. EVERY BINDING LANDS ON A DISTINCT NODE ──────────────────────
        //
        // M2 finding 4: a second row silently overwriting the first passes
        // check-i18n while reporting two bound tips. On this page the live risk
        // is the wrapper walk — 23 `#select-*` rows walk to `.dropdown-group`,
        // and a `.dropdown-group` holding two selects would collapse two tips
        // onto one node.
        const paths = resolution.filter(r => r.found).map(r => r.nodePath);
        const dupPaths = paths.filter((p, i) => paths.indexOf(p) !== i);
        check(dupPaths.length === 0,
            `[1b] all ${paths.length} bindings land on DISTINCT nodes — a shared node would mean `
            + 'one tip silently overwrote another', dupPaths.join(' | '));

        // The minimum-hover-area assertion is NOT made here. Twelve of the 107
        // anchors measure 0 px2 in this pass because they are `display: none` in
        // the state it runs in — the four LFO Division dropdowns behind their Sync
        // buttons, and eight controls on tabs this pass has already left. A
        // threshold applied to a rect that is zero because the tab is closed
        // measures the driver, not the page. It is made in the `en` hover loop
        // below instead, against the box the user actually gets. See [1c] there.

        const idSelectors = resolution.filter(r => r.sel.startsWith('#')).length;
        const viaWrapper  = resolution.filter(r => r.wrapperDeclared && !r.anchorIsSelf).length;
        const areaGain    = resolution.filter(r => r.area > r.selfArea).length;
        note(`selector half: ${idSelectors} of ${resolution.length} bindings use an id`);
        note(`target half:   ${viaWrapper} of ${resolution.length} resolve through a wrapper, `
           + `${areaGain} of which enlarge the hover area`);

        // ── the driving loop ────────────────────────────────────────────────
        async function hoverAnchor(sel, wrapper) {
            // Move somewhere neutral first so pointerover definitely fires on the
            // next move: a pointer already inside the anchor generates no new
            // pointerover, and the tip would be measured in whatever state the
            // PREVIOUS anchor left it — which is exactly how a "contains" check
            // passes on stale text. (2, 2) is the header's dead margin; the
            // top-right corner would be inside the gear cluster.
            await page.mouse.move(2, 2);
            await page.waitForTimeout(50);

            const box = await page.evaluate(({ sel, wrapper }) => {
                const el = document.querySelector(sel);
                if (!el) return null;
                const target = wrapper ? (el.closest(wrapper) || el) : el;
                const r = target.getBoundingClientRect();
                if (r.width < 1 || r.height < 1) return null;
                return { x: r.left + r.width / 2, y: r.top + r.height / 2, w: r.width, h: r.height };
            }, { sel, wrapper: wrapper || null });

            if (!box) return null;
            await page.mouse.move(box.x, box.y);
            // Past the 0.12 s opacity transition, so `opacity` is settled and a
            // mid-flight value cannot be read as "not shown" — O-Comp's carried
            // trap 2, whose obvious response to the false failure is to delete
            // the focus latch.
            await page.waitForTimeout(210);
            return { box, tip: await page.evaluate(READ_TIP) };
        }

        // Drive one anchor through its own visibility prerequisites, then hover.
        async function reachAndHover(sel, wrapper) {
            if (POPOVER_ONLY.has(sel) || sel === '#gear-btn') {
                await openTab('synth');
                await setPopover(POPOVER_ONLY.has(sel));
            } else {
                await setPopover(false);
                await openTab(TAB_OF(sel));
                if (HIDDEN_UNTIL_SYNC.has(sel))      await setLfoSync(true);
                else if (HIDDEN_WHEN_SYNC.has(sel))  await setLfoSync(false);
            }
            return hoverAnchor(sel, wrapper);
        }

        // ── S. THE FOUR HIDDEN DROPDOWNS, AT REST AND THEN DRIVEN ───────────
        //
        // Pinned as an assertion rather than left implicit. At load lfoNSync is
        // Off, so #lfoN-div-wrap is `display: none` and NO pointer can reach the
        // Division dropdown inside it. That is shipped interaction code
        // (bindLfoSync, index.html:2900), not a renderer property, and recording
        // it here means a later change to that gating shows up in this file
        // instead of silently changing what 4 of 107 tips do.
        console.log('\n-- S. the four sync-gated dropdowns');
        await setPopover(false);
        await openTab('synth');
        const restHidden = await page.evaluate(() => [1, 2, 3, 4].map(i => ({
            div:  getComputedStyle(document.getElementById('lfo' + i + '-div-wrap')).display,
            rate: getComputedStyle(document.getElementById('lfo' + i + '-rate-wrap')).display,
        })));
        check(restHidden.every(r => r.div === 'none' && r.rate !== 'none'),
            '[S] at rest, all four LFO Division dropdowns are display:none and all four Rate knobs '
            + 'are shown — the default state, asserted so a change to it is visible here');
        const divAtRest = await hoverAnchor('#select-lfo1Division', '.dropdown-group');
        check(divAtRest === null,
            '[S] and a POINTER cannot open the hidden Division dropdown\'s tip at rest '
            + '(zero-size rect)');

        const synced = await setLfoSync(true);
        check(synced === true,
            '[S] clicking the four Sync buttons — the page\'s own path, never a style edit — '
            + 'reveals the Division dropdowns');
        await setLfoSync(false);
        note('the loop below drives each LFO anchor into the state that reveals it, one at a time');

        // ── 2/3/4, in en then fr then en ────────────────────────────────────
        const allSel = TIP_BINDINGS.map(b => b[0]);
        let seenEn = false;

        for (const lang of ['en', 'fr', 'en']) {
            const isReturn = lang === 'en' && seenEn;
            seenEn = seenEn || lang === 'en';
            console.log(`\n-- 2/3/4. language: ${lang}${isReturn ? ' (return pass)' : ''}`);

            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(180);

            let clamped = 0, flipped = 0, maxH = 0, measured = 0;
            const hoverArea = {};

            for (const sel of allSel) {
                const b = TIP_BINDINGS.find(x => x[0] === sel);
                const [, key, wrapper] = b;
                const entry = (I18N[key] || {})[lang] || {};

                const got = await reachAndHover(sel, wrapper);
                if (!got) { check(false, `[2][${lang}] ${sel} is hoverable`); continue; }
                const t = got.tip;
                measured++;
                hoverArea[sel] = got.box.w * got.box.h;

                // ── 2. THE VACUITY GUARD ────────────────────────────────────
                // A tip that never showed is the failure this whole file exists
                // for. It FAILS.
                const visible = t.shown && t.visibility === 'visible' && t.opacity === '1';
                check(visible,
                    `[2][${lang}] hovering ${sel} SHOWS the tip `
                    + `(show=${t.shown} visibility=${t.visibility} opacity=${t.opacity})`);
                check(t.title.trim() !== '' && t.body.trim() !== '',
                    `[2][${lang}] ${sel} renders non-empty title AND body `
                    + `(${t.title.length} / ${t.body.length} chars)`);
                check(t.ariaHidden === 'false',
                    `[2][${lang}] ${sel} sets aria-hidden="false" while shown — got ${t.ariaHidden}`);

                // ── 3. BYTE-EQUAL, not "contains" ───────────────────────────
                // A .tip-title that silently kept the PREVIOUS anchor's text
                // passes a contains check on a body mentioning the same words.
                check(t.title === entry.t,
                    `[3][${lang}] ${sel} title is byte-equal to I18N['${key}'].${lang}.t`,
                    t.title === entry.t ? null : `rendered "${t.title}" vs table "${entry.t}"`);
                check(t.body === entry.b,
                    `[3][${lang}] ${sel} body is byte-equal to I18N['${key}'].${lang}.b`,
                    t.body === entry.b ? null
                        : `rendered ${t.body.length} chars vs table ${(entry.b || '').length} chars`);

                // ── 4. INSIDE THE VIEWPORT, ALL FOUR EDGES ──────────────────
                const out = outsideViewport(t.rect, W, H);
                check(out.length === 0,
                    `[4][${lang}] ${sel} tip rect is fully inside ${W} x ${H} `
                    + `(${t.rect.w.toFixed(1)} x ${t.rect.h.toFixed(1)} at `
                    + `${t.rect.left.toFixed(1)},${t.rect.top.toFixed(1)})`,
                    out.length ? out.join('; ') : null);

                if (t.rect.left <= 8.5 || Math.abs(t.rect.right - (W - 8)) < 0.5
                    || t.rect.top <= 8.5 || Math.abs(t.rect.bottom - (H - 8)) < 0.5) clamped++;
                if (t.rect.left < got.box.x || t.rect.top < got.box.y) flipped++;
                maxH = Math.max(maxH, t.rect.h);

                if (verbose)
                    note(`${lang} ${sel}: cursor ${got.box.x.toFixed(0)},${got.box.y.toFixed(0)} `
                       + `-> tip ${t.rect.left.toFixed(1)},${t.rect.top.toFixed(1)} `
                       + `${t.rect.w.toFixed(1)}x${t.rect.h.toFixed(1)}`);
            }
            check(measured === allSel.length,
                `[2][${lang}] every one of the ${allSel.length} anchors was reached and measured — `
                + `got ${measured}`);
            note(`${lang}: ${flipped} tip(s) placed by FLIP, ${clamped} touching a clamp edge, `
               + `tallest ${maxH.toFixed(1)} px`);

            // ── 1c. EVERY ANCHOR IS BIG ENOUGH FOR A HUMAN TO HOVER ─────────
            //
            // M2 finding 4, the other shape: O-MicrotonalSampler's #ctrl-attack
            // is a 1x1 px, opacity-0, pointer-events:none input that resolves,
            // satisfies check-i18n, and no human could ever open. Measured HERE,
            // on the box each anchor has in the state the driver puts it in,
            // rather than in the resolution pass where a closed tab reads 0.
            // 300 px2 is roughly an 18x17 target; the smallest real control on
            // this page is a bypass button.
            const areas = Object.entries(hoverArea);
            const tiny = areas.filter(([, a]) => a < 300);
            const minA = areas.reduce((m, [s, a]) => (a < m[1] ? [s, a] : m), ['', Infinity]);
            check(tiny.length === 0,
                `[1c][${lang}] every anchor gives at least 300 px2 of hover area — smallest is `
                + `${minA[0]} at ${Math.round(minA[1])} px2`,
                tiny.map(([s, a]) => `${s} ${Math.round(a)} px2`).join(', '));
        }

        // ── 5. FRENCH REALLY IS TALLER, and English really came back ────────
        //
        // Re-measured rather than inferred: the point of running both languages
        // is that French wraps to more lines against the 280 px max-width cap. If
        // it did NOT, the two passes would be the same measurement twice and
        // assertion 4's French half would be decoration.
        console.log('\n-- 5. French height vs English');
        const SAMPLE = allSel.filter(s => (TAB_OF(s) === null)
                                       && !HIDDEN_UNTIL_SYNC.has(s)
                                       && !POPOVER_ONLY.has(s)
                                       && s !== '#gear-btn').slice(0, 30);
        async function heightsFor(lang) {
            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(180);
            const h = {};
            for (const sel of SAMPLE) {
                const got = await reachAndHover(sel, TIP_BINDINGS.find(x => x[0] === sel)[2]);
                h[sel] = got ? got.tip.rect.h : -1;
            }
            return h;
        }
        const hEn = await heightsFor('en');
        const hFr = await heightsFor('fr');
        const grew   = SAMPLE.filter(s => hFr[s] > hEn[s] + 0.5);
        const shrank = SAMPLE.filter(s => hFr[s] < hEn[s] - 0.5);
        const same   = SAMPLE.filter(s => Math.abs(hFr[s] - hEn[s]) <= 0.5);
        check(grew.length > 0,
            `[5] French GROWS at least one tip's height against the 280 px cap — `
            + `${grew.length} grew, ${same.length} unchanged, ${shrank.length} shrank `
            + `(of ${SAMPLE.length} sampled)`,
            grew.length ? null
                : 'no tip grew: the fr pass measured the same boxes as en, so its clamp half is decoration');
        note(`tallest en ${Math.max(...SAMPLE.map(s => hEn[s])).toFixed(1)} px, `
           + `tallest fr ${Math.max(...SAMPLE.map(s => hFr[s])).toFixed(1)} px`);
        if (verbose) for (const s of grew) note(`  grew: ${s} ${hEn[s].toFixed(0)} -> ${hFr[s].toFixed(0)}`);

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(180);
        const backEn = await reachAndHover('#knob-oscAPos', undefined);
        check(backEn && backEn.tip.title === I18N['tip.oscAPos'].en.t
                     && backEn.tip.body  === I18N['tip.oscAPos'].en.b,
            '[5] English comes back after the French pass — byte-equal again');

        // ── 6. THE NEGATIVE CONTROL, WITH ITS PLANT SIZE SEARCHED FOR ───────
        //
        // Assertion 4 has passed at every anchor in both languages. That is
        // indistinguishable from an assertion 4 that CANNOT SEE an overflow until
        // one is planted and it reports.
        //
        // THE PLANT IS SEARCHED FOR, NOT GUESSED. This is the largest frame in
        // the suite — 1200 x 800, with 784 px of vertical clamp room — and
        // O-Tremolo's habitual 40x plant (880 chars) fit inside a 400 px frame
        // and reported nothing. The loop below doubles the body until the
        // rendered tip is TALLER THAN THE FRAME, then asserts that it overflowed;
        // a plant that fits is indistinguishable from a gate that cannot see.
        //
        // The plant is a DOM ATTRIBUTE write, not a file edit. applyI18n() is the
        // only writer of data-tip, so window.__setLanguage('en') restores the real
        // value exactly and there is nothing on disk to lose — the scar behind
        // that rule is O-GrainScatter's, which lost a whole uncommitted edit to a
        // `git checkout --` used to undo a plant. A namespaced copy of the file
        // was also taken at scratchpad/O-Prism/m3/ before this run, per the
        // standing rule, and is not needed by this mechanism.
        console.log('\n-- 6. negative control (assertion 4 harness-blindness)');
        const PLANT_SEL = '#knob-oscAPos';
        await setPopover(false);
        await openTab('synth');

        let reps = 100, planted = null, plantBody = '';
        for (; reps <= 6400; reps *= 2) {
            plantBody = 'overflow probe. '.repeat(reps);
            await page.evaluate(({ sel, body }) => {
                document.querySelector(sel).setAttribute('data-tip', body);
            }, { sel: PLANT_SEL, body: plantBody });
            planted = await hoverAnchor(PLANT_SEL, undefined);
            const h = planted ? planted.tip.rect.h : 0;
            note(`plant search: ${reps} reps (${plantBody.length} chars) -> ${h.toFixed(0)} px tall `
               + `in a ${H} px frame`);
            if (h > H) break;
        }
        const plantedOut = planted ? outsideViewport(planted.tip.rect, W, H) : ['tip did not render'];
        check(planted && planted.tip.body === plantBody,
            '[6] the plant actually reached the surface (a plant that never rendered proves nothing)');
        check(planted && planted.tip.rect.h > H,
            `[6] the SEARCHED plant is BIGGER THAN THE FRAME — `
            + `${planted ? planted.tip.rect.h.toFixed(0) : '?'} px tall against ${H} px, at `
            + `${plantBody.length} chars. A plant that fits is indistinguishable from a gate that `
            + 'cannot see');
        check(plantedOut.length > 0,
            '[6] a planted over-long body OVERFLOWS and assertion 4 reports it — '
            + `${plantedOut.join('; ') || 'NOTHING REPORTED'}`,
            plantedOut.length ? null
                              : 'assertion 4 is BLIND — every [4] pass above is decoration');

        // restore from the TABLE, and prove the restore took
        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(180);
        const restored = await hoverAnchor(PLANT_SEL, undefined);
        check(restored && restored.tip.body === I18N['tip.oscAPos'].en.b,
            '[6] restored: the anchor carries the table body again, byte-equal');
        check(restored && outsideViewport(restored.tip.rect, W, H).length === 0,
            '[6] restored: assertion 4 is green again at the same anchor');

        // ── 6b. THE CLAMP AFTER THE FLIP — a POSITIVE control ───────────────
        //
        // Every shipped tip on this 1200 x 800 page places with room to spare, so
        // the four-edge clamp is dead code as far as sections 2-5 can tell and
        // "assertion 4 passed hundreds of times" says nothing about it.
        //
        // M2 finding 1 is the trap here: the post-flip RE-CLAMP inside position()
        // is unreachable by construction on all eleven ports — after a flip
        // `ny = y - h - 12`, so a test of the form `ny + h > vh - M` collapses to
        // `y - 12 > vh - M` and stops mentioning the tip's size at all. This
        // renderer does not use that shape: its clamp is an unconditional
        // Math.min/Math.max pair that runs AFTER the flip. The assertion below
        // drives that floor DIRECTLY — a plant that fits on NEITHER side of the
        // cursor flips upward to a negative `top` and is then floored to the 8 px
        // margin, so landing exactly at 8 is the clamp doing the work and nothing
        // else. It does not claim the flip did it.
        console.log('\n-- 6b. the clamp after the flip (positive control)');
        // THE PROBE SIZE IS SEARCHED FOR, AND THE WINDOW IS NARROW. For a cursor
        // at y, the tip must be taller than max(H − 24 − y, y − 20) so it fits on
        // NEITHER side, and shorter than H so the pass below is the clamp rather
        // than the "bigger than the frame" case section 6 already covers. At
        // y ≈ 400 in an 800 px frame that window is roughly 390 to 799 px — about
        // 1100 to 2300 characters at this surface's 280 px cap. A doubling search
        // steps straight over it: the first draft went 60 → 120 → … → 1920 reps
        // and landed on an 8481 px tip, which is section 6's probe again wearing
        // section 6b's name. Stepping by 5 reps (65 chars) finds the window.
        const MID_SEL = '#knob-ampSustain';
        let tall = null, tallBody = '';
        for (let r = 20; r <= 400; r += 5) {
            tallBody = 'clamp probe. '.repeat(r);
            await page.evaluate(({ sel, body }) => {
                document.querySelector(sel).setAttribute('data-tip', body);
            }, { sel: MID_SEL, body: tallBody });
            tall = await hoverAnchor(MID_SEL, undefined);
            if (!tall) break;
            const fitsBelow = tall.box.y + 16 + tall.tip.rect.h <= H - 8;
            const fitsAbove = tall.box.y - tall.tip.rect.h - 12 >= 8;
            if (!fitsBelow && !fitsAbove && tall.tip.rect.h < H) break;
        }
        if (tall) {
            const wouldNotFitBelow = tall.box.y + 16 + tall.tip.rect.h > H - 8;
            const wouldNotFitAbove = tall.box.y - tall.tip.rect.h - 12 < 8;
            check(wouldNotFitBelow && wouldNotFitAbove,
                `[6b] the probe fits on NEITHER side of the cursor at y=${tall.box.y.toFixed(0)} `
                + `(${tall.tip.rect.h.toFixed(0)} px tall in ${H} px) — otherwise this control is a `
                + 'second copy of assertion 4 rather than a test of the clamp');
            check(Math.abs(tall.tip.rect.top - 8) < 0.5,
                `[6b] the flipped tip is CLAMPED to the 8 px margin — top `
                + `${tall.tip.rect.top.toFixed(2)}, where the flip alone gives `
                + `${(tall.box.y - tall.tip.rect.h - 12).toFixed(0)}`);
            check(outsideViewport(tall.tip.rect, W, H).length === 0,
                '[6b] and the clamped result is fully inside the frame');
            note(`clamp probe ${tallBody.length} chars, `
               + `${tall.tip.rect.w.toFixed(1)} x ${tall.tip.rect.h.toFixed(1)} placed at `
               + `${tall.tip.rect.left.toFixed(1)},${tall.tip.rect.top.toFixed(1)}`);
        } else {
            check(false, '[6b] the clamp probe rendered');
        }

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(180);
        const restored2 = await hoverAnchor(MID_SEL, undefined);
        check(restored2 && restored2.tip.body === I18N['tip.ampSustain'].en.b,
            '[6b] restored: the anchor carries the table body again, byte-equal');

        // ══════════════════════ 7. THE FOCUS LATCH, BOTH HALVES ════════════
        //
        // A mouse click on a <button> focuses it. An unconditional focusin rule
        // therefore re-opens the tip that pointerdown just hid, with the pointer
        // still on the anchor and no further pointerover coming — and the tip
        // sits on top of whatever the click opened. Measured on two sibling
        // plugins that landed without the latch: 146 x 35 px on O-Bass and
        // 161 x 29 px on O-AnalogSaturation, of the gear's tip across the
        // settings popover.
        //
        // BOTH halves are asserted, separately and on purpose. Asserting only
        // that a click leaves no tip lets the feature decay into "focus never
        // shows a tip", which passes that assertion perfectly and silently
        // removes the keyboard half of hover-help.
        //
        // A STATIC regex for `lastInputWasPointer` would NOT discriminate: O-Comp
        // proved that deleting only `if (lastInputWasPointer) return;` leaves the
        // declaration, the pointerdown write and the keydown clear all matching.
        // Only the behavioural control below sees it.
        console.log('\n-- 7. the focus latch');

        await setPopover(false);
        await openTab('synth');
        await page.mouse.move(1, 1);
        await page.waitForTimeout(150);
        // BLUR FIRST, and this line is the whole reason the assertion below can
        // fail at all. An earlier section leaves focus on a control, and clicking
        // an ALREADY-FOCUSED element fires no focusin — so without this the check
        // reports "no tip after a click" for a page with no latch whatsoever.
        // O-Bass passed 125/125 that way; O-Tremolo then passed 186/186 with BOTH
        // the latch and this line removed, which is the control ON the control.
        // IT IS LOAD-BEARING HERE, and the full 2x2 was run rather than assumed
        // (M2 finding 6: O-TextureForge is in the other cell, where section order
        // had already blurred the gear and the line made no difference):
        //
        //                    blur present   blur removed
        //   latch present    2180 PASS      2180 PASS
        //   latch removed    FAIL [7]       2180 PASS
        //                    3319 px2
        //
        // The control fires in exactly one cell. Delete this line and the
        // assertion below becomes a second copy of the claim.
        await page.evaluate(() => document.activeElement && document.activeElement.blur());
        await page.waitForTimeout(100);
        await page.click('#gear-btn');
        await page.waitForTimeout(320);
        const afterClick = await page.evaluate(() => {
            const t = document.getElementById('tooltip');
            const cs = getComputedStyle(t);
            const r = t.getBoundingClientRect();
            const panel = document.getElementById('settings-popover');
            const pr = panel.classList.contains('visible') ? panel.getBoundingClientRect() : null;
            const shown = cs.visibility !== 'hidden' && cs.opacity !== '0';
            let overlap = 0;
            if (shown && pr) {
                const ox = Math.max(0, Math.min(r.right, pr.right) - Math.max(r.left, pr.left));
                const oy = Math.max(0, Math.min(r.bottom, pr.bottom) - Math.max(r.top, pr.top));
                overlap = Math.round(ox * oy);
            }
            // Measure the overlap rather than only observing the tip: a number a
            // later run can compare is what makes this defect reportable instead
            // of arguable.
            return { shown, overlap, activeEl: document.activeElement
                                             ? (document.activeElement.id || document.activeElement.tagName)
                                             : null };
        });
        check(!afterClick.shown,
            '[7] a POINTER click opens no tip — the latch suppresses the focusin arm'
            + (afterClick.overlap ? ` (it covered the popover by ${afterClick.overlap} px2)` : ''));
        note(`after the click, focus is on ${afterClick.activeEl} — so a focusin DID fire`);

        // The keyboard half. A real tab-ring walk, not a programmatic .focus():
        // Chromium reports :focus-visible false for a .focus() that follows a
        // click, and .focus() on an already-focused element fires no event at all
        // — either one would report "no tip" and record that as correct.
        //
        // `visibility`, not a hard opacity threshold, and 200 ms per press: a
        // probe sampling 80 ms into a 120 ms transition reports a false "never
        // opens", and the obvious response to that failure is to delete the latch
        // (O-Comp, carried trap 2).
        await page.keyboard.press('Escape');
        await page.waitForTimeout(150);
        let kbHit = null;
        for (let i = 1; i <= 30; i++) {
            await page.keyboard.press('Tab');
            await page.waitForTimeout(200);
            const r = await page.evaluate(() => {
                const t = document.getElementById('tooltip');
                const cs = getComputedStyle(t);
                return { shown: cs.visibility !== 'hidden',
                         text: (t.textContent || '').trim(),
                         on: document.activeElement
                             ? (document.activeElement.id
                                || (typeof document.activeElement.className === 'string'
                                    ? document.activeElement.className : '')
                                || document.activeElement.tagName)
                             : null };
            });
            if (r.shown && r.text) { kbHit = { press: i, ...r }; break; }
        }
        check(kbHit !== null,
            '[7] a KEYBOARD tab still opens a tip — the accessibility half survives the latch'
            + (kbHit ? ` (tab #${kbHit.press} on ${kbHit.on})` : ' — none in 30 tabs'));
        await page.keyboard.press('Escape');
        await page.waitForTimeout(150);

        // ══════════════════════ 7b. THE DRAG GUARD, HOLD AND RELEASE ═══════
        //
        // Renderer property 10, and on this page it is REQUIRED rather than
        // optional. bindKnob (index.html:2420) starts a drag on `mousedown` and
        // tracks `document.mousemove`; it does NOT call setPointerCapture, so
        // boundary events are not retargeted for the duration of a drag and
        // straying into the neighbouring knob fires a pointerover there. That is
        // the M2 O-TextureForge finding, and this page has the shape it applies
        // to — O-AnalogEQ, which has pointer capture, needed no guard at all.
        //
        // BOTH SIDES ARE ASSERTED. A guard that is only ever set is a permanent
        // off switch that passes the first half and silently kills hover-help.
        console.log('\n-- 7b. the drag guard');
        await setPopover(false);
        await openTab('synth');
        const dragBoxes = await page.evaluate(() => {
            const a = document.querySelector('#knob-oscAPos').getBoundingClientRect();
            const b = document.querySelector('#knob-oscALevel').getBoundingClientRect();
            return { a: { x: a.left + a.width / 2, y: a.top + a.height / 2 },
                     b: { x: b.left + b.width / 2, y: b.top + b.height / 2 } };
        });
        await page.mouse.move(dragBoxes.a.x, dragBoxes.a.y);
        await page.waitForTimeout(210);
        await page.mouse.down();
        await page.mouse.move(dragBoxes.b.x, dragBoxes.b.y, { steps: 8 });
        await page.waitForTimeout(210);
        const midDrag = await page.evaluate(READ_TIP);
        check(midDrag && !midDrag.shown,
            '[7b] dragging from one knob across its neighbour opens NO tip over the control being '
            + `turned (shown=${midDrag ? midDrag.shown : '?'})`);
        await page.mouse.up();
        await page.waitForTimeout(120);
        // THE RELEASE. Move away and back so a genuine pointerover is generated.
        const afterRelease = await hoverAnchor('#knob-oscALevel', undefined);
        check(afterRelease && afterRelease.tip.shown
              && afterRelease.tip.title === I18N['tip.oscALevel'].en.t,
            '[7b] and after pointerup the guard RELEASES — hovering opens the tip again '
            + '(a guard that is only ever set is a permanent off switch that passes the line above)');

        // ── housekeeping ────────────────────────────────────────────────────
        console.log('');
        check(pageErrors.length === 0, '[8] no uncaught page error across the whole sweep',
            pageErrors.slice(0, 3).join(' | '));
        check(consoleErrors.length === 0, '[8] no console.error across the whole sweep',
            consoleErrors.slice(0, 3).join(' | '));
        check(misses.length === 0, '[8] every requested resource was served',
            misses.slice(0, 5).join(', '));

        // A native title= would render a second, untranslated OS tooltip
        // competing with this surface (contract section 4). Repo-wide it is 0
        // today, and this renderer must not reintroduce one. Counted across all
        // five tabs, because this page rendered 6 native titles at load and 248
        // once the Matrix and Rotation views had been opened, and every gate in
        // the repo saw only the six (v1.21.0's finding).
        let nativeTitles = 0;
        for (const tab of ['synth', 'mod', 'tuning', 'effects', 'wavetable']) {
            await openTab(tab);
            nativeTitles += await page.evaluate(() => document.querySelectorAll('[title]').length);
        }
        check(nativeTitles === 0,
            `[8] zero native title= attributes across all five tabs — got ${nativeTitles}`);

        // The count this whole dispatch turns on: 105 of O-Prism's 173 parameters
        // have a control whose anchor exists when applyI18n() runs, and all 105
        // are bound. The other 68 are reported in i18n.js's I18N header and in the
        // commit message — 64 async mod-matrix rows, `tonic` (async), and three
        // with no control at all (`tuningPreset`, `stereoWidth`, `velocityCurve`).
        // v1.23.0: THREE chrome tips — #tips-toggle joined #gear-btn and
        // #lang-select when the settings popover grew a hover-help switch.
        check(TIP_BINDINGS.length === 108,
            `[8] 105 parameter tips + 3 chrome tips = 108 bindings — got ${TIP_BINDINGS.length}`);
        const modAnchors = await page.evaluate(() =>
            document.querySelectorAll('[id^="modSlot"]').length);
        check(modAnchors === 0,
            '[8] the 64 mod-matrix parameters really have NO id\'d anchor on the page — the '
            + 'evidence for decision item 14, asserted rather than asserted-about',
            `found ${modAnchors}`);

    } finally {
        await browser.close();
        await close();
        fs.rmSync(built.root, { recursive: true, force: true });
    }

    console.log(`\n${failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`}`
              + `   (${passes} passed)`);
    process.exit(failed === 0 ? 0 : 1);
})().catch((e) => { console.error(e); process.exit(1); });
