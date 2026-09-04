/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    ui_tip_render_check.js — O-Formant hover-help, RENDERED.

    THE FIRST RUNNABLE GATE IN THIS PLUGIN. tests/ held only i18n-states.json
    before v1.27.0, and there is no Source/tests/ either (checked, because
    O-MicrotonalSampler's runnable gate lived there and "run every gate in
    tests/" would have missed it).

    ── WHY IT EXISTS ───────────────────────────────────────────────────────────

    NO OTHER GATE IN THIS REPO CAN SEE A RENDERED TOOLTIP.

      check-i18n      reads the table statically. Its assertion 2 counts
                      TIP_BINDINGS rows and checks each key exists in I18N.
                      Fifty-seven bindings pointing at fifty-seven selectors
                      that match nothing, on a page with no renderer at all, is
                      a PASS.
      check-ui-labels has no tooltip awareness whatsoever. Its entire output is
                      BYTE-IDENTICAL before and after this feature landed —
                      measured with a file-copy swap, not assumed, and recorded
                      in the v1.27.0 commit message.
      boot-all-uis    counts aria-label and title. It never counts data-tip, and
                      it fails on console.error but not console.warn.

    So authoring 57 tooltip bodies into i18n.js and binding them, with no other
    change, would have shipped 57 invisible strings past three green gates.

    ── THE FRAME IS 800 x 600, READ NOT GUESSED ────────────────────────────────

    Pinned to the SHIPPING size parsed out of PluginEditor.cpp's setSize(),
    never Chromium's 1280x720 default: a clamp gate at the wrong viewport
    measures a page that has room and certifies nothing
    (pattern_tooltip_clamp_gate_viewport_sensitive).

    800 x 600 is a ROOMY frame by this task's standards — M1's O-Chorus was
    700 x 125 and M2's O-AnalogEQ 920 x 220 — which makes the negative control
    below the assertion most likely to go vacuous. It was SEARCHED for rather
    than guessed, and the search is recorded at section 6: a habitual 40x plant
    (640 chars) renders 248 px tall here, FITS, and reports nothing. Break-even
    is between 1600 and 1920 characters.

    ── THIS PAGE IS A FOUR-TAB DECK, AND THAT IS THE DEFINING PROBLEM ──────────

    #synth-tab (.active at load), #effects-tab, #lyrics-tab, #tuning-tab. A
    .tab-content without .active is `display: none` — a zero-size rect that
    cannot be hovered and cannot be measured. Twenty-one of the 57 anchors live
    on the effects tab and one on the lyrics tab.

    Every one of them is reached by CLICKING THE TAB, which is the page's own
    path. Stripping the class off a hidden panel measures a state the plugin
    only reaches by a click nobody made (M2 finding 5).

    And .right-col scrolls: its content is 690 px tall inside a 340 px box, so
    ten synth-tab anchors sit below the fold at load. They are brought into view
    with Element.scrollIntoView(), which drives the same scrollTop the user's
    wheel drives on a container the page itself declared `overflow-y: auto`.

    ── AND FRENCH IS THE OTHER HALF ────────────────────────────────────────────

    French runs 15-20 % longer, wraps to more lines against the 260 px max-width
    cap, and GROWS THE TIP'S HEIGHT. A tip that fits in English can therefore
    overflow the bottom in French, which is why every assertion runs in both
    languages rather than in English with a French spot-check.

    Usage:
        node plugins/O-Formant/tests/ui_tip_render_check.js
        node plugins/O-Formant/tests/ui_tip_render_check.js --verbose

    Exit codes:
        0   every assertion passed
        1   at least one assertion failed, or the harness itself broke
        77  Playwright unresolvable — NOTHING was verified. Never a pass.

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const { pathToFileURL } = require('url');

const REPO_ROOT = path.resolve(__dirname, '..', '..', '..');
const S = require(path.join(REPO_ROOT, 'scripts', 'serve-ui.js'));

const PLUGIN  = 'O-Formant';
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
// concatenations all 57 bodies are authored with — rather than against a
// regex's idea of them. A fixture that mirrored the table would drift silently
// (pattern_test_fixture_mirrors_drift_silently).
async function loadTable() {
    const src = path.join(REPO_ROOT, 'plugins', PLUGIN, 'Source', 'ui', 'public', 'js', 'i18n.js');
    if (!fs.existsSync(src)) throw new Error(`i18n.js not found at ${src}`);
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'oformant-i18n-'));
    const dst = path.join(dir, 'i18n.mjs');
    fs.copyFileSync(src, dst);
    try { return await import(pathToFileURL(dst).href); }
    finally { fs.rmSync(dir, { recursive: true, force: true }); }
}

// ── reading the rendered surface ────────────────────────────────────────────
//
// The body is read as the concatenation of the surface's own TEXT NODES and the
// title as .tip-title's textContent, because that is exactly how the renderer
// builds it: createElement + appendChild(createTextNode). Reading
// tip.textContent whole would glue the title onto the front of the body and
// turn assertion 3 into a substring test by accident.
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
        visibility: cs.visibility, opacity: cs.opacity,
        ariaHidden: tip.getAttribute('aria-hidden'),
        pointerEvents: cs.pointerEvents, position: cs.position,
        zIndex: cs.zIndex, maxWidth: cs.maxWidth,
        title: titleEl ? titleEl.textContent : '',
        body,
        rect: { left: r.left, top: r.top, right: r.right, bottom: r.bottom, w: r.width, h: r.height },
    };
};

const EPS = 0.01;
function outsideViewport(rect, W, H) {
    const out = [];
    if (rect.left   < -EPS)    out.push(`left ${rect.left.toFixed(2)} < 0`);
    if (rect.top    < -EPS)    out.push(`top ${rect.top.toFixed(2)} < 0`);
    if (rect.right  > W + EPS) out.push(`right ${rect.right.toFixed(2)} > ${W}`);
    if (rect.bottom > H + EPS) out.push(`bottom ${rect.bottom.toFixed(2)} > ${H}`);
    return out;
}

// The seven parameters with no control anywhere on this page. Pinned as an
// assertion rather than left in a comment: if one of them ever gains a control,
// this gate says so instead of the tip count silently staying at 57.
const PAGE_UNREACHABLE = [
    'consonantVOT', 'sourceFilterCoupling',
    'tuning_masterTune', 'tuning_tuningMode', 'tuning_octaveStretch',
    'tuning_pitchBendRange', 'tuning_temperamentPreset',
];

(async () => {
    console.log(`ui_tip_render_check — ${PLUGIN} hover-help, RENDERED\n`);

    const chromiumPkg = S.resolvePlaywright();
    if (!chromiumPkg) {
        console.error('Playwright is not resolvable. NOTHING was verified — this is not a pass.');
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

    const CHROME = new Set(['#gear-btn', '#lang-select', '#tips-toggle']);

    const built = S.buildRoot(PLUGIN, { repoRoot: REPO_ROOT });
    const misses = [];
    const { port, close } = await S.serve(built.root, (u) => misses.push(u));

    const pageErrors = [], consoleErrors = [], consoleWarns = [], tipWarns = [];

    const browser = await chromium.launch();
    // `viewport`, not `viewportSize`. A clamp gate at 1280x720 has room the
    // shipping frame does not and certifies nothing.
    const page = await browser.newPage({ viewport: { width: W, height: H }, deviceScaleFactor: 1 });

    page.on('pageerror', (e) => pageErrors.push(String(e)));
    page.on('console', (m) => {
        const t = m.text();
        if (m.type() === 'error')   consoleErrors.push(t);
        if (m.type() === 'warning') consoleWarns.push(t);
        if (/tip target not found/.test(t)) tipWarns.push(t);
    });

    try {
        await page.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });
        await page.waitForTimeout(600);

        const vp = await page.evaluate(() => ({ w: window.innerWidth, h: window.innerHeight }));
        check(vp.w === W && vp.h === H, `[0] viewport really is ${W} x ${H} — got ${vp.w} x ${vp.h}`);
        check(pageErrors.length === 0,
            '[0] no uncaught page error during load (a TDZ throw takes every later initializer)',
            pageErrors.slice(0, 3).join(' | '));

        // ── 0b. THE LANGUAGE BRIDGE IS BOUND ────────────────────────────────
        //
        // A REGRESSION GUARD FOR A DEFECT THIS STAGE FOUND SHIPPED. v1.26.0's
        // js/main.js imported four NAMED bindings from ./juce/index.js and no
        // namespace, while the canon block reaches the language pair as
        // `Juce.getNativeFunction`. `Juce` was therefore never bound, initI18n's
        // first statement threw ReferenceError, and its own try/catch degraded
        // the throw to the console.warn asserted against below — so the language
        // preference was never read from C++ at open and never written back on
        // change, on a plugin whose C++ half was complete.
        //
        // boot-all-uis fails on console.error and not on console.warn, which is
        // exactly why this shipped. Asserted on the WARNING TEXT rather than on
        // the presence of an import line: a static grep for `import * as Juce`
        // would stay green if the canon were ever re-spelled.
        const bridgeWarn = consoleWarns.filter(w => /Language preference not available/.test(w));
        check(bridgeWarn.length === 0,
            '[0b] initI18n bound the language bridge — no "Language preference not available" warning '
            + '(the v1.26.0 ReferenceError this release fixes)',
            bridgeWarn.slice(0, 2).join(' | '));
        const langPulled = await page.evaluate(() => document.getElementById('lang-select').value);
        check(langPulled === 'en',
            `[0b] the one-shot getUiLanguage() pull settled and left the selector at "en" — got "${langPulled}"`);

        // ── 0c. THE TAB DECK, AS SHIPPED ────────────────────────────────────
        const deck = await page.evaluate(() => [...document.querySelectorAll('.tab-content')].map(p => {
            const r = p.getBoundingClientRect();
            return { id: p.id, active: p.classList.contains('active'),
                     display: getComputedStyle(p).display, area: Math.round(r.width * r.height) };
        }));
        note(`tab deck: ${deck.map(d => `${d.id}${d.active ? '*' : ''}`).join(' ')}`);
        check(deck.length === 4 && deck.filter(d => d.active).length === 1 && deck[0].active,
            `[0c] four tab panels, exactly one active at load and it is ${deck[0] && deck[0].id}`);
        check(deck.filter(d => !d.active).every(d => d.display === 'none' && d.area === 0),
            '[0c] every inactive panel is display:none with a ZERO rect — which is why every anchor '
            + 'below is reached by CLICKING ITS TAB and never by stripping a class',
            deck.filter(d => !d.active && d.area !== 0).map(d => `${d.id} ${d.area}px2`).join(', '));

        // ── 0d. THE TOOLTIP SURFACE ─────────────────────────────────────────
        const surface = await page.evaluate(READ_TIP);
        // `surface && typeof surface === 'object'`, not `!== null`: a
        // page.evaluate whose argument is a function-source STRING returns the
        // function object, which is unserialisable and arrives as undefined —
        // and `undefined !== null` is a PASS over a surface nobody read. That
        // false pass really happened, on O-Bass's first run of this gate.
        check(surface && typeof surface === 'object',
            '[0d] the #tooltip surface exists in the DOM and was READ (not undefined)');
        if (surface && typeof surface === 'object') {
            check(surface.position === 'fixed', `[0d] the surface is position: fixed — got ${surface.position}`);
            check(surface.pointerEvents === 'none',
                `[0d] the surface is pointer-events: none — got ${surface.pointerEvents}`);
            check(surface.visibility === 'hidden' || surface.opacity === '0',
                `[0d] the surface is HIDDEN at rest (visibility ${surface.visibility}, opacity ${surface.opacity})`);
            // #lang-select is itself an anchor and lives INSIDE .settings-popover
            // (z-index 60), so a tip that did not out-stack the panel would open
            // BEHIND the control that revealed it.
            check(Number(surface.zIndex) > 60,
                `[0d] the surface out-stacks .settings-popover (60) — z-index ${surface.zIndex}`);
            note(`surface: ${surface.position}, z-index ${surface.zIndex}, max-width ${surface.maxWidth}`);
        }

        // ── 0e. THE SEVEN PAGE-UNREACHABLE PARAMETERS ───────────────────────
        const stray = await page.evaluate((ids) => ids.filter(id =>
            document.querySelectorAll(`[data-param="${id}"]`).length > 0
            || document.body.innerHTML.indexOf(`"${id}Slider"`) >= 0), PAGE_UNREACHABLE);
        check(stray.length === 0,
            `[0e] the ${PAGE_UNREACHABLE.length} parameters with no control still have none — none of them `
            + 'gained one to satisfy a tip count', stray.join(', '));

        // ── 1. EVERY TIP_BINDINGS SELECTOR RESOLVES ─────────────────────────
        //
        // applyI18n's own failure here is a console.warn, which boot-all-uis
        // prints and nothing fails on. A binding that finds no element is a
        // FAIL in this gate, not a warning.
        //
        // AND THE WRAPPER HALF IS A HARD FAIL TOO, which is M2's most important
        // gate-shape finding (from O-Reed): applyI18n falls back
        // `el.closest(w) || el`, so a BROKEN wrapper still opens a tip — on the
        // wrong-sized cell — and assertions 2, 3 and 4 all still pass in both
        // languages. A gate that only checks "the tip appeared with the right
        // text inside the viewport" cannot see a broken wrapper at all.
        //
        // REPRODUCED HERE at v1.27.0 rather than taken on trust: changing
        // '.consonant-xy-wrap' to '.consonant-xy-wrap-BROKEN' produced exactly
        // ONE failure — this one — while [2], [3] and [4] passed at that anchor
        // in both languages, because the fallback armed the bare <canvas>.
        console.log('-- 1. binding resolution');
        const resolution = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
            const el = document.querySelector(sel);
            if (!el) return { sel, key, found: false };
            const target = wrapper ? (el.closest(wrapper) || el) : el;
            const tr = target.getBoundingClientRect();
            const er = el.getBoundingClientRect();
            const panel = target.closest('.tab-content');
            return {
                sel, key, found: true,
                wrapperDeclared: !!wrapper,
                wrapperResolved: wrapper ? !!el.closest(wrapper) : null,
                anchorIsSelf: target === el,
                tab: panel ? panel.id.replace(/-tab$/, '') : null,
                nodeKey: (function pathOf(n) {          // a stable per-node identity
                    const parts = [];
                    for (let e = n; e && e.nodeType === 1; e = e.parentElement) {
                        let i = 1; for (let s = e.previousElementSibling; s; s = s.previousElementSibling) ++i;
                        parts.unshift(`${e.tagName}:${i}`);
                    }
                    return parts.join('/');
                })(target),
                hasTip: target.hasAttribute('data-tip'),
                hasTitle: target.hasAttribute('data-tip-title'),
                area: Math.round(tr.width * tr.height),
                selfArea: Math.round(er.width * er.height),
            };
        }), TIP_BINDINGS.map(b => [b[0], b[1], b[2] || null]));

        for (const r of resolution) {
            check(r.found, `[1] selector resolves: ${r.sel}  (key ${r.key})`);
            if (!r.found) continue;
            check(r.hasTip && r.hasTitle,
                `[1] applyI18n wrote data-tip + data-tip-title onto the anchor for ${r.sel}`);
            if (r.wrapperResolved === false)
                check(false, `[1] the declared wrapper for ${r.sel} did NOT resolve — applyI18n's `
                           + '`closest(w) || el` fallback would arm the id\'d node instead, silently, '
                           + 'and assertions 2-4 would all still pass');
        }
        check(tipWarns.length === 0, '[1] applyI18n logged no "tip target not found" warning',
            tipWarns.slice(0, 3).join(' | '));

        // EVERY BINDING LANDS ON A DISTINCT NODE. A second row that silently
        // overwrote the first passes check-i18n while reporting two bound tips
        // (M2 finding 4). Nine of the fifteen M2 plugins added this
        // independently; it is standard now.
        const nodeKeys = resolution.filter(r => r.found).map(r => r.nodeKey);
        const dupes = nodeKeys.filter((k, i) => nodeKeys.indexOf(k) !== i);
        check(dupes.length === 0,
            `[1] all ${nodeKeys.length} bindings land on DISTINCT nodes — a second row overwriting the `
            + 'first passes check-i18n while reporting two bound tips', dupes.join(', '));

        // A MINIMUM HOVER AREA. O-MicrotonalSampler's #ctrl-attack was a 1x1 px
        // opacity-0 input that resolved, satisfied check-i18n, and could never
        // be opened by a human. #gear-btn is the tightest anchor on this page.
        const MIN_AREA = 300;
        const tiny = resolution.filter(r => r.found && r.area > 0 && r.area < MIN_AREA);
        check(tiny.length === 0,
            `[1] every anchor is at least ${MIN_AREA} px2 of hover area — an anchor nobody can point at `
            + 'is a tip nobody can open', tiny.map(r => `${r.sel} ${r.area}px2`).join(', '));
        const areas = resolution.filter(r => r.found && r.area > 0).map(r => r.area);
        note(`smallest visible hover area ${Math.min(...areas)} px2, largest ${Math.max(...areas)} px2`);

        const idSelectors = resolution.filter(r => r.sel.startsWith('#')).length;
        const viaWrapper  = resolution.filter(r => r.wrapperDeclared && !r.anchorIsSelf).length;
        note(`selector half: ${idSelectors} of ${resolution.length} bindings use an id`);
        note(`target half:   ${viaWrapper} of ${resolution.length} resolve through a wrapper`);

        const byTab = {};
        for (const r of resolution) byTab[r.tab === null ? 'chrome' : r.tab] = (byTab[r.tab === null ? 'chrome' : r.tab] || 0) + 1;
        note(`anchors per tab: ${Object.entries(byTab).map(([k, v]) => `${k} ${v}`).join(', ')}`);

        // ── the driving helpers ─────────────────────────────────────────────
        //
        // THROUGH THE PAGE'S OWN PATH, ALWAYS. The tab is CLICKED, never
        // un-hidden by hand; the scroll container is scrolled, never re-laid
        // out. Stripping display:none off #effects-tab would measure a state
        // the plugin only reaches by a click nobody made.
        async function selectTab(name) {
            if (!name) return true;
            const already = await page.evaluate((n) =>
                document.getElementById(`${n}-tab`).classList.contains('active'), name);
            if (already) return true;
            await page.click(`.tab[data-tab="${name}"]`);
            await page.waitForTimeout(140);
            return await page.evaluate((n) =>
                document.getElementById(`${n}-tab`).classList.contains('active'), name);
        }

        async function setPopover(open) {
            const isOpen = await page.evaluate(() =>
                document.getElementById('settings-popover').classList.contains('open'));
            if (isOpen !== open) {
                await page.click('#gear-btn');
                await page.waitForTimeout(160);
            }
            return await page.evaluate(() =>
                document.getElementById('settings-popover').classList.contains('open'));
        }

        async function hoverAnchor(sel, wrapper) {
            // Move somewhere neutral first so pointerover definitely fires on
            // the next move: a pointer already inside the anchor generates no
            // new pointerover, and the tip would then be measured in whatever
            // state the PREVIOUS anchor left it — which is exactly how a
            // "contains" check passes on stale text. (2, 2) is the header's
            // dead left margin; the top RIGHT corner is the gear.
            await page.mouse.move(2, 2);
            await page.waitForTimeout(45);

            const box = await page.evaluate(({ sel, wrapper }) => {
                const el = document.querySelector(sel);
                if (!el) return null;
                const target = wrapper ? (el.closest(wrapper) || el) : el;
                // .right-col holds 690 px of content in a 340 px box, so ten
                // synth-tab anchors start below the fold. scrollIntoView drives
                // the same scrollTop a wheel drives on a container the page
                // itself declared overflow-y: auto.
                //
                // UNCONDITIONALLY, and that is not tidiness. The first draft of
                // this gate scrolled only when the rect fell outside the
                // VIEWPORT — which is blind to an element clipped by its SCROLL
                // CONTAINER. A previous anchor's scroll left .right-col's
                // scrollTop somewhere, glottalRd's rect read y=71 (above
                // .right-col's own top edge of 90), the pointer landed on the
                // tab bar, no pointerover fired, and the surface was measured
                // still carrying the PREVIOUS anchor's text. That failure looks
                // like a copy bug and is a harness bug.
                target.scrollIntoView({ block: 'center', inline: 'nearest' });
                const r = target.getBoundingClientRect();
                const x = r.left + r.width / 2, y = r.top + r.height / 2;
                // And confirm the cursor will actually land ON the anchor.
                // elementFromPoint is the only reading that distinguishes "the
                // tip did not open" from "the gate aimed at the wrong pixel".
                const hit = document.elementFromPoint(x, y);
                const onAnchor = !!(hit && hit.closest && hit.closest('[data-tip]') === target);
                return { x, y, w: r.width, h: r.height, onAnchor,
                         hit: hit ? (hit.id || hit.className || hit.tagName) : null };
            }, { sel, wrapper: wrapper || null });

            if (!box) return null;
            await page.mouse.move(box.x, box.y);
            // Past the 0.12 s opacity transition, so `opacity` is settled and a
            // mid-flight value cannot be read as "not shown" — O-Comp's carried
            // trap, whose obvious response to the false failure is to delete the
            // focus latch.
            await page.waitForTimeout(200);
            return { box, tip: await page.evaluate(READ_TIP) };
        }

        // group the anchors by the tab they live on, chrome last
        const tabsUsed = [...new Set(resolution.filter(r => r.tab).map(r => r.tab))];
        const groups = tabsUsed.map(t => ({ tab: t, popover: false,
                                            list: resolution.filter(r => r.tab === t).map(r => r.sel) }))
                        .concat([{ tab: null, popover: true,
                                   list: resolution.filter(r => r.tab === null).map(r => r.sel) }]);

        // ══════════════ 2 / 3 / 4, in en -> fr -> en ═══════════════════════
        let seenEn = false;
        for (const lang of ['en', 'fr', 'en']) {
            const isReturn = lang === 'en' && seenEn;
            seenEn = seenEn || lang === 'en';
            console.log(`\n-- 2/3/4. language: ${lang}${isReturn ? ' (return pass)' : ''}`);

            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(150);

            let clamped = 0, flipped = 0, maxH = 0;

            for (const group of groups) {
                if (group.tab) {
                    const ok = await selectTab(group.tab);
                    check(ok, `[.] ${group.tab} tab is active for this group (clicked, not un-hidden)`);
                } else {
                    const st = await setPopover(true);
                    check(st, '[.] the settings popover is OPEN for the chrome group');
                }

                for (const sel of group.list) {
                    const b = TIP_BINDINGS.find(x => x[0] === sel);
                    if (!b) { check(false, `[.] ${sel} is in TIP_BINDINGS`); continue; }
                    const [, key, wrapper] = b;
                    const entry = (I18N[key] || {})[lang] || {};

                    const got = await hoverAnchor(sel, wrapper);
                    if (!got) { check(false, `[2][${lang}] ${sel} is hoverable`); continue; }
                    check(got.box.onAnchor,
                        `[2][${lang}] the cursor lands ON ${sel} — elementFromPoint resolves to this `
                        + 'anchor, so a "tip did not open" below is the renderer and not the aim',
                        got.box.onAnchor ? null : `hit ${got.box.hit} instead`);
                    const t = got.tip;

                    // ── 2. THE VACUITY GUARD ────────────────────────────────
                    // A tip that never showed is the failure this whole file
                    // exists for. It FAILS.
                    const visible = t.shown && t.visibility === 'visible' && t.opacity === '1';
                    check(visible, `[2][${lang}] hovering ${sel} SHOWS the tip `
                        + `(show=${t.shown} visibility=${t.visibility} opacity=${t.opacity})`);
                    check(t.title.trim() !== '' && t.body.trim() !== '',
                        `[2][${lang}] ${sel} renders non-empty title AND body `
                        + `(${t.title.length} / ${t.body.length} chars)`);
                    check(t.ariaHidden === 'false',
                        `[2][${lang}] ${sel} sets aria-hidden="false" while shown — got ${t.ariaHidden}`);

                    // ── 3. BYTE-EQUAL, not "contains" ───────────────────────
                    // A .tip-title that silently kept the PREVIOUS anchor's text
                    // passes a contains check on a body that mentions the same
                    // words. Equality is the only test that catches it.
                    check(t.title === entry.t,
                        `[3][${lang}] ${sel} title is byte-equal to I18N['${key}'].${lang}.t`,
                        t.title === entry.t ? null : `rendered "${t.title}" vs table "${entry.t}"`);
                    check(t.body === entry.b,
                        `[3][${lang}] ${sel} body is byte-equal to I18N['${key}'].${lang}.b`,
                        t.body === entry.b ? null
                            : `rendered ${t.body.length} chars vs table ${(entry.b || '').length} chars`);

                    // ── 4. INSIDE THE VIEWPORT, ALL FOUR EDGES ──────────────
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
                if (!group.tab) await setPopover(false);
            }
            note(`${lang}: ${flipped} tip(s) placed by FLIP, ${clamped} touching a clamp edge, `
               + `tallest ${maxH.toFixed(1)} px`);
        }

        // ── 5. FRENCH REALLY IS TALLER, and English really came back ────────
        //
        // Re-measured rather than inferred from the loop: the point of running
        // both languages is that French wraps to more lines against the 260 px
        // cap, and if it did NOT the two passes would be the same measurement
        // twice and assertion 4's French half would be decoration.
        console.log('\n-- 5. French height vs English');
        const SAMPLE = resolution.filter(r => r.tab === 'synth').map(r => r.sel);
        async function heightsFor(lang) {
            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(150);
            await selectTab('synth');
            const h = {};
            for (const sel of SAMPLE) {
                const got = await hoverAnchor(sel, TIP_BINDINGS.find(x => x[0] === sel)[2]);
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
            `[5] French GROWS at least one tip's height against the 260 px cap — ${grew.length} grew, `
            + `${same.length} unchanged, ${shrank.length} shrank`,
            grew.length ? null
                : 'no tip grew: the fr pass measured the same boxes as en, so its clamp half is decoration');
        note(`tallest en ${Math.max(...SAMPLE.map(s => hEn[s])).toFixed(1)} px, `
           + `tallest fr ${Math.max(...SAMPLE.map(s => hFr[s])).toFixed(1)} px`);
        if (verbose) for (const s of grew) note(`  grew: ${s} ${hEn[s].toFixed(0)} -> ${hFr[s].toFixed(0)}`);

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const backEn = await hoverAnchor('.knob-wrap[data-param="glottalRd"]', null);
        check(backEn && backEn.tip.title === I18N['tip.glottalRd'].en.t
                     && backEn.tip.body  === I18N['tip.glottalRd'].en.b,
            '[5] English comes back after the French pass — byte-equal again');

        // ── 6. THE NEGATIVE CONTROL ─────────────────────────────────────────
        //
        // Assertion 4 has passed at every anchor in both languages. That is
        // indistinguishable from an assertion 4 that CANNOT SEE an overflow
        // until one is planted and it reports.
        //
        // THE PLANT SIZE WAS SEARCHED FOR, NOT GUESSED, and the search is the
        // finding: on this 800 x 600 frame with a 260 px cap and 11 px Garamond,
        //
        //     640 chars (the habitual 40x plant) -> 248 px tall -> FITS, reports nothing
        //     960 chars                          -> 341 px tall -> FITS
        //    1280 chars                          -> 449 px      -> FITS (flip fires here)
        //    1600 chars                          -> 556 px      -> FITS, clamped to top 8
        //    1920 chars                          -> 649 px      -> OVERFLOWS the bottom
        //
        // Break-even is between 1600 and 1920 characters. The plant below is
        // 4800, roughly 2.5x break-even, and the assertion immediately under it
        // CONFIRMS the plant overflowed rather than assuming it. A plant that
        // fits is indistinguishable from a gate that cannot see.
        //
        // The plant is a DOM ATTRIBUTE write, not a file edit. applyI18n() is
        // the only writer of data-tip, so window.__setLanguage('en') restores
        // the real value exactly and there is nothing on disk to lose —
        // O-GrainScatter lost a whole uncommitted edit to a `git checkout --`
        // used to undo a plant.
        console.log('\n-- 6. negative control (assertion 4 harness-blindness)');
        await selectTab('synth');
        const PLANT_SEL = '.knob-wrap[data-param="glottalRd"]';
        const PLANT = 'overflow probe. '.repeat(300);
        await page.evaluate(({ sel, body }) =>
            document.querySelector(sel).setAttribute('data-tip', body), { sel: PLANT_SEL, body: PLANT });

        const planted = await hoverAnchor(PLANT_SEL, null);
        const plantedOut = planted ? outsideViewport(planted.tip.rect, W, H) : ['tip did not render'];
        check(planted && planted.tip.body === PLANT,
            '[6] the plant actually reached the surface (a plant that never rendered proves nothing)');
        check(planted && planted.tip.rect.h > H,
            `[6] the plant is BIGGER THAN THE FRAME — ${planted ? planted.tip.rect.h.toFixed(0) : '?'} px `
            + `tall against ${H} px`);
        check(plantedOut.length > 0,
            '[6] a planted over-long body OVERFLOWS and assertion 4 reports it — '
            + `${plantedOut.join('; ') || 'NOTHING REPORTED'}`,
            plantedOut.length ? null : 'assertion 4 is BLIND — every [4] pass above is decoration');
        if (planted) note(`planted tip ${planted.tip.rect.w.toFixed(1)} x `
                        + `${planted.tip.rect.h.toFixed(1)} in a ${W} x ${H} frame`);

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const restored = await hoverAnchor(PLANT_SEL, null);
        check(restored && restored.tip.body === I18N['tip.glottalRd'].en.b,
            '[6] restored: the anchor carries the table body again, byte-equal');
        check(restored && outsideViewport(restored.tip.rect, W, H).length === 0,
            '[6] restored: assertion 4 is green again at the same anchor');

        // ── 6b. THE CLAMP AFTER THE FLIP — a POSITIVE control ───────────────
        //
        // Every shipped tip on this page places with room to spare, so the
        // four-edge clamp is dead code as far as sections 2-5 can tell and
        // "assertion 4 passed N times" says nothing about it.
        //
        // M2 finding 1 is the trap this section is shaped around. In the ported
        // renderer the re-clamp used to be written `if (ny + r.height >
        // innerHeight - M)`, which AFTER A FLIP substitutes ny = y - h - 12 and
        // collapses to `y - 12 > innerHeight - M` — it stops mentioning the
        // tip's size at all and can only fire for a cursor outside the viewport.
        // O-Chorus credited that line with behaviour the Math.max floor was
        // producing. This renderer clamps with an unconditional
        // Math.min(Math.max(...)) pair instead, and the assertion below DRIVES
        // THE FLOOR DIRECTLY rather than crediting a branch.
        //
        // NEGATIVE-CONTROLLED at v1.27.0: swapping the unconditional
        // Math.min(Math.max(...)) pair for the conditional form puts this tip at
        // top -367.61 — 368 px off the page — while ALL 171 shipped [4]
        // assertions stay green, because no shipped tip on this page is tall
        // enough to flip past the top. Only this section sees it.
        //
        // A 1600-char plant is 556 px tall. At a cursor near y=155 it fits
        // NEITHER below (155 + 16 + 556 = 727 > 592) NOR above
        // (155 - 556 - 12 = -413 < 8), so it flips upward to a negative top and
        // is then clamped to the 8 px margin. A PASS at exactly 8 is the clamp
        // doing the work; without it this tip would sit ~413 px off the page and
        // assertion 4 would report `top < 0`.
        console.log('\n-- 6b. the clamp after the flip (positive control)');
        const TALL = 'clamp probe. '.repeat(123);          // ~1600 chars
        await page.evaluate(({ sel, body }) =>
            document.querySelector(sel).setAttribute('data-tip', body), { sel: PLANT_SEL, body: TALL });
        const tall = await hoverAnchor(PLANT_SEL, null);
        if (tall) {
            const wouldNotFitBelow = tall.box.y + 16 + tall.tip.rect.h > H - 8;
            const wouldNotFitAbove = tall.box.y - tall.tip.rect.h - 12 < 8;
            check(wouldNotFitBelow && wouldNotFitAbove,
                `[6b] the plant fits on NEITHER side of the cursor at y=${tall.box.y.toFixed(0)} `
                + `(${tall.tip.rect.h.toFixed(0)} px tall in ${H} px) — otherwise this control is a `
                + 'second copy of assertion 4 rather than a test of the clamp');
            check(Math.abs(tall.tip.rect.top - 8) < 0.5,
                `[6b] the flipped tip is CLAMPED to the 8 px margin — top ${tall.tip.rect.top.toFixed(2)}, `
                + `where the flip alone gives ${(tall.box.y - tall.tip.rect.h - 12).toFixed(0)}`);
            check(outsideViewport(tall.tip.rect, W, H).length === 0,
                '[6b] and the clamped result is fully inside the frame');
            note(`clamp probe ${tall.tip.rect.w.toFixed(1)} x ${tall.tip.rect.h.toFixed(1)} placed at `
               + `${tall.tip.rect.left.toFixed(1)},${tall.tip.rect.top.toFixed(1)}`);
        } else check(false, '[6b] the clamp probe rendered');

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const restored2 = await hoverAnchor(PLANT_SEL, null);
        check(restored2 && restored2.tip.body === I18N['tip.glottalRd'].en.b,
            '[6b] restored: the anchor carries the table body again, byte-equal');

        // ══════════════ 7. THE FOCUS LATCH, BOTH HALVES ════════════════════
        //
        // A mouse click on a <button> focuses it. An unconditional focusin rule
        // therefore re-opens the tip that pointerdown just hid, with the pointer
        // still on the anchor and no further pointerover coming — and the tip
        // sits on top of whatever the click opened. Measured on the two sibling
        // plugins that landed without the latch: the gear's tip covered the
        // settings popover by 146 x 35 px on O-Bass and 161 x 29 px on
        // O-AnalogSaturation.
        //
        // BOTH halves are asserted, separately and on purpose. Asserting only
        // that a click leaves no tip lets the feature decay into "focus never
        // shows a tip", which passes that assertion perfectly and silently
        // removes the keyboard half of hover-help.
        //
        // A STATIC regex for `lastInputWasPointer` would NOT discriminate:
        // deleting only `if (lastInputWasPointer) return;` leaves the
        // declaration, the pointerdown write and the keydown clear all matching
        // (O-Comp, confirmed on six plugins since). Only the behavioural control
        // below sees it.
        console.log('\n-- 7. the focus latch');
        await selectTab('synth');
        await setPopover(false);
        await page.mouse.move(1, 1);
        await page.waitForTimeout(150);
        // BLUR FIRST. Clicking an ALREADY-FOCUSED element fires no focusin at
        // all, so without this line the check reports "no tip after a click" for
        // a page with no latch whatsoever — O-Bass passed 125/125 that way, and
        // O-Tremolo passed 186/186 with BOTH the latch and this line removed.
        //
        // ON THIS PAGE THE BLUR IS NOT LOAD-BEARING, and that is MEASURED, not
        // inherited. The full 2x2 was run at v1.27.0:
        //
        //     latch  blur   result
        //       yes   yes   ALL CHECKS PASSED (1360)
        //       yes   no    ALL CHECKS PASSED (1360)
        //       no    yes   FAIL [7] — the gear tip covered the popover, 7114 px2
        //       no    no    FAIL [7] — 7114 px2, identical
        //
        // So this line is an ACCIDENT OF SECTION ORDER here, exactly as
        // O-TextureForge found on its own page: section 7b runs after 7, and the
        // last thing section 6b does is a hover, so focus is already off the
        // gear by the time the click lands. It is kept because it is one edit
        // from load-bearing again — move a section and the assertion silently
        // becomes decoration. Do not delete it, and do not inherit the verdict
        // for the next plugin either: run the 2x2.
        await page.evaluate(() => document.activeElement && document.activeElement.blur());
        await page.waitForTimeout(100);
        await page.click('#gear-btn');
        await page.waitForTimeout(320);
        const afterClick = await page.evaluate(() => {
            const t = document.getElementById('tooltip');
            const cs = getComputedStyle(t);
            const r = t.getBoundingClientRect();
            const panel = document.getElementById('settings-popover');
            const pr = panel && panel.classList.contains('open') ? panel.getBoundingClientRect() : null;
            const shown = cs.visibility !== 'hidden' && cs.opacity !== '0';
            let overlap = 0;
            if (shown && pr) {
                const ox = Math.max(0, Math.min(r.right, pr.right) - Math.max(r.left, pr.left));
                const oy = Math.max(0, Math.min(r.bottom, pr.bottom) - Math.max(r.top, pr.top));
                overlap = Math.round(ox * oy);
            }
            // Measure the overlap rather than only observing the tip: a number a
            // later run can compare is what makes this defect reportable rather
            // than arguable.
            return { shown, overlap, activeEl: document.activeElement
                        ? (document.activeElement.id || document.activeElement.tagName) : null };
        });
        check(!afterClick.shown,
            '[7] a POINTER click opens no tip — the latch suppresses the focusin arm'
            + (afterClick.overlap ? ` (it covered the popover by ${afterClick.overlap} px2)` : ''));
        note(`after the click, focus is on ${afterClick.activeEl} — so a focusin DID fire`);

        // The keyboard half. A real tab-ring walk, not a programmatic .focus():
        // Chromium reports :focus-visible false for a .focus() that follows a
        // click, and .focus() on an already-focused element fires no event at
        // all — either one would report "no tip" and record that as correct.
        //
        // `visibility`, not a hard opacity threshold, and 200 ms per press: a
        // probe sampling 80 ms into a 120 ms transition reports a false "never
        // opens", and the obvious response to THAT failure is to delete the
        // latch (O-Comp).
        await page.keyboard.press('Escape');
        await page.waitForTimeout(150);
        let kbHit = null;
        for (let i = 1; i <= 26; i++) {
            await page.keyboard.press('Tab');
            await page.waitForTimeout(200);
            const r = await page.evaluate(() => {
                const t = document.getElementById('tooltip');
                const cs = getComputedStyle(t);
                return { shown: cs.visibility !== 'hidden',
                         text: (t.textContent || '').trim(),
                         on: document.activeElement
                             ? (document.activeElement.id
                                || document.activeElement.getAttribute('data-param')
                                || document.activeElement.className) : null };
            });
            if (r.shown && r.text) { kbHit = { press: i, ...r }; break; }
        }
        check(kbHit !== null,
            '[7] a KEYBOARD tab still opens a tip — the accessibility half survives the latch'
            + (kbHit ? ` (tab #${kbHit.press} on ${kbHit.on})` : ' — none in 26 tabs'));
        await page.keyboard.press('Escape');
        await page.waitForTimeout(150);

        // ── 7b. THE DRAG GUARD — measured, not assumed ──────────────────────
        //
        // M2's tenth renderer property (from O-TextureForge): a knob that starts
        // a drag on pointerdown and tracks document.pointermove will open a
        // NEIGHBOUR's tip over the control being turned when the drag strays
        // into another cell, and pointerdown alone cannot cover it because
        // pointerover arrives after it.
        //
        // THIS PAGE NEEDS NO SEPARATE FLAG, and that is a measurement rather
        // than a default. All three drag surfaces call setPointerCapture on
        // pointerdown — bindKnobs (main.js), bindXYPad, bindConsonantXYPad — so
        // every boundary event is retargeted to the captured element for the
        // duration of the drag, the same shape O-AnalogEQ had. The drag below
        // crosses three neighbouring cells and asserts no tip appears; the
        // RELEASE is asserted too, or a permanent off switch would pass here.
        console.log('\n-- 7b. drag across neighbouring cells');
        await selectTab('synth');
        const dragBoxes = await page.evaluate(() => ['glottalRd', 'vibratoRate', 'vibratoDelay'].map((p) => {
            const e = document.querySelector(`.knob-wrap[data-param="${p}"]`);
            e.scrollIntoView({ block: 'center' });
            const r = e.getBoundingClientRect();
            return { p, x: r.left + r.width / 2, y: r.top + r.height / 2 };
        }));
        await page.mouse.move(dragBoxes[0].x, dragBoxes[0].y);
        await page.waitForTimeout(200);
        await page.mouse.down();
        let tipDuringDrag = false;
        for (const b of dragBoxes.slice(1)) {
            await page.mouse.move(b.x, b.y, { steps: 8 });
            await page.waitForTimeout(160);
            if (await page.evaluate(() =>
                getComputedStyle(document.getElementById('tooltip')).visibility !== 'hidden'))
                tipDuringDrag = true;
        }
        await page.mouse.up();
        await page.waitForTimeout(120);
        check(!tipDuringDrag,
            '[7b] dragging from one knob across two neighbours opens NO tip — setPointerCapture '
            + 'retargets the boundary events, so no extra drag flag is needed on this page');
        // THE RELEASE. Without this the assertion above is satisfied by a
        // renderer that simply never shows a tip again.
        const afterDrag = await hoverAnchor('.knob-wrap[data-param="vibratoDelay"]', null);
        check(afterDrag && afterDrag.tip.shown,
            '[7b] and a hover after pointerup still opens a tip — the guard RELEASED');

        // ── 8. housekeeping ─────────────────────────────────────────────────
        console.log('');
        check(pageErrors.length === 0, '[8] no uncaught page error across the whole sweep',
            pageErrors.slice(0, 3).join(' | '));
        check(consoleErrors.length === 0, '[8] no console.error across the whole sweep',
            consoleErrors.slice(0, 3).join(' | '));
        check(misses.length === 0, '[8] every requested resource was served', misses.slice(0, 5).join(', '));

        // A native title= would render a second, untranslated OS tooltip
        // competing with this surface (contract §4). Repo-wide it is 0 today,
        // and this renderer must not reintroduce one.
        const nativeTitles = await page.evaluate(() => document.querySelectorAll('[title]').length);
        check(nativeTitles === 0, `[8] zero native title= attributes on the page — got ${nativeTitles}`);

        // One tip per on-page CONTROL. 47 [data-param] cells (45 knobs + 2
        // toggles) plus 10 anchors that carry no data-param: the two XY pads,
        // the topology segmented control, the delay-mode <select>, the four
        // effect bypass buttons and the two chrome controls.
        const dataParams = await page.evaluate(() => document.querySelectorAll('[data-param]').length);
        check(dataParams === 47,
            `[8] the page carries 47 [data-param] control cells — got ${dataParams}`);
        // v1.28.0: ELEVEN non-[data-param] anchors, not ten — #tips-toggle joined
        // #gear-btn and #lang-select when the settings popover grew a hover-help
        // switch.
        check(TIP_BINDINGS.length === dataParams + 11,
            `[8] one tip per control: ${dataParams} [data-param] cells + 11 non-data-param anchors `
            + `vs ${TIP_BINDINGS.length} bindings`);
        check(resolution.filter(r => r.found).length === TIP_BINDINGS.length,
            `[8] every TIP_BINDINGS row resolved — ${resolution.filter(r => r.found).length} of `
            + `${TIP_BINDINGS.length}`);

    } finally {
        await browser.close();
        await close();
        fs.rmSync(built.root, { recursive: true, force: true });
    }

    console.log(`\n${failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`}`
              + `   (${passes} passed)`);
    process.exit(failed === 0 ? 0 : 1);
})().catch((e) => { console.error(e); process.exit(1); });
