/*
   This file is part of O-Reed, an Ouaricon Audio plugin.
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

    ui_tip_render_check.js
    O-Reed — hover-help RENDER verification at the shipping viewport (v1.3.0).

    WHY THIS FILE EXISTS. No gate in this repo can see a rendered tooltip.
    check-i18n reads the table statically and is satisfied by TIP_BINDINGS being
    non-empty and its keys resolving. check-ui-labels has no tooltip awareness
    whatsoever — measured on this plugin: its output is BYTE-IDENTICAL before
    and after the whole of v1.3.0, all four states, every count included.
    boot-all-uis counts aria-label and title and never data-tip. So a plugin can
    author copy, bind it, and pass all three green while showing nothing on
    screen — which is exactly the state O-Reed was in at v1.2.0: canon v2's
    applyI18n() writes data-tip-title and data-tip onto the anchors and stops
    there, and that page had no #tooltip element, no .tooltip rule and no hover
    handler to read them.

    This file is the assertion those three cannot make. It drives the REAL page
    — same index.html, same inline <style>, same inline module, only the JUCE
    bridge stubbed — at the exact shipping frame, and measures the rectangle
    that actually paints.

    NOT a port of the three committed ui_tooltip_clamp_check.js gates
    (O-Tapestop, O-Bitrot, O-ReverseDelay). Those are ~40 KB each and are built
    around the OTHER renderer family: measure-then-pin placement with an
    above/below flip and an arrow. O-Reed ports O-simpleFM's delegated
    cursor-following renderer, which has no arrow and no pinned width, so those
    assertions would not describe it.

    ── THE SECTIONS ARE EXPANDED FIRST, AND THAT IS LOAD-BEARING ─────────────

    Six of O-Reed's seven sections ship COLLAPSED, and `.section-content` is
    `max-height: 0; overflow: hidden` — which does NOT remove their children
    from layout, it merely clips them from painting. 23 of the 27 knob anchors
    live inside one. A sweep that hovered them where they sit at rest would be
    hovering a clipped box and would report whatever the elements painted over
    them happened to do. So this gate opens every section first, the same way
    tests/i18n-states.json does for check-ui-labels, and then sweeps. That is a
    real user state (the section headers are click-to-toggle) and it is the only
    state in which all 35 anchors have a hoverable box.

    `.instrument-panel` is `overflow-y: auto`, so expanding everything makes it
    a scroll container rather than growing the document — the document's own
    scroll extent stays 900 x 600, which check-ui-labels asserts separately.
    Playwright scrolls that panel to reach each anchor; the tip is
    `position: fixed`, so it is placed against the VIEWPORT whatever the panel
    has scrolled to.

    WHAT IS ASSERTED, per anchor, in English and again in French:

      1. every TIP_BINDINGS selector resolves AND its closest(wrapper) walk
         lands on a real element carrying both attributes — a binding that finds
         nothing is a FAIL, not a warning, because applyI18n() only
         console.warns about it;
      2. hovering a DESCENDANT of the anchor makes the surface VISIBLE with
         non-empty text. Hovering a descendant rather than the anchor itself is
         the point: it is what the pointer actually lands on, and it exercises
         the delegated closest('[data-tip]') walk that makes this renderer work
         at all;
      3. the rendered title and body are BYTE-EQUAL to the table entry. Not
         "contains": a .tip-title that silently kept the previous anchor's text
         passes a contains check and fails this one;
      4. the tip rectangle is fully inside the 900 x 600 frame on all four
         edges, and no wider than the .tooltip max-width cap.

    Then French (2-4 again against the fr entries), then back to English to
    prove the switch is reversible rather than one-way.

    THREE NEGATIVE CONTROLS, because "all pass" is worthless without them:

      NC-1  an over-long body is planted in the SERVED copy of js/i18n.js and
            the page reloaded; assertion 4 must REPORT the overflow. THE PLANT
            IS SIZED AGAINST THIS FRAME AND THIS max-width, not by habit:
            O-Tremolo measured a 40x plant FITTING inside a 400px frame and
            reporting nothing, and a plant that fits is indistinguishable from a
            gate that cannot see. The plant's own height is asserted before the
            overflow claim is believed. Restored from a namespaced per-run copy,
            never with `git checkout --`, which would take any uncommitted work
            in the same file with it (O-GrainScatter lost a whole edit that way).
      NC-2  the focus latch, BOTH halves, separately: a pointer click must leave
            no tip parked on screen, AND a real tab-ring walk must still open
            one. Asserting only the first lets the feature decay into "focus
            never shows a tip", which passes that assertion perfectly while
            silently removing the keyboard half of hover-help.
            THE CLICK IS PRECEDED BY A BLUR. Clicking an already-focused element
            fires no focusin at all, so without the blur this assertion passes
            for a page with NO latch whatsoever — the orchestrator measured
            exactly that on the M1 pilots (125/125 green with the latch deleted)
            and O-Tremolo then measured 186/186 green with the latch AND the
            blur both removed, which is the control ON the control. The overlap
            with the settings popover is measured in px2 rather than merely
            observed, so a later run has a number to compare.
            The keyboard half AWAITS the 0.12s fade rather than sleeping past
            it: O-Comp measured an 80ms read reporting "never opens" for a path
            that demonstrably works, and the obvious response to that reading is
            to delete the latch.
      NC-3  the pointerout child-boundary rule: moving between two children of
            the SAME anchor must not flicker the tip off.

    Usage:  node plugins/O-Reed/tests/ui_tip_render_check.js
    Exit code = number of failed assertions (0 = all pass, 77 = could not run).
    Requires Playwright (`npx playwright install chromium` once).

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const vm   = require('vm');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const S          = require(path.join(repoRoot, 'scripts', 'serve-ui.js'));

const PLUGIN   = 'O-Reed';
// O-Reed's served root is Resources/ui, not Source/ui/public — three of the
// eight plugins in batch M2 are. Read from serve-ui's own resolver below rather
// than trusted from here, but named so the divergence is visible.
const UI_ROOT  = path.join(pluginRoot, 'Resources', 'ui');
const htmlPath = path.join(UI_ROOT, 'index.html');
const i18nPath = path.join(UI_ROOT, 'js', 'i18n.js');

// Mirrored from PluginEditor.cpp and the inline <style>, and CROSS-CHECKED
// against both below. A fixture that mirrors a constant without checking it
// starts describing the release before it and keeps passing
// (pattern_test_fixture_mirrors_drift_silently).
const SHIP_W = 900;
const SHIP_H = 600;
const MARGIN = 8;               // setupTooltips()'s clamp margin
const DOCUMENTED_MAX_W = 260;   // .tooltip max-width
const KNOB_COUNT = 27;          // .knob-control[data-param] nodes in the markup
const MARKER_COUNT = 15;        // XY_PRESETS entries populateXYMarkers() writes

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}

// ── the table, read the way check-i18n reads it ─────────────────────────────
// i18n.js is an ES module outside any package.json, so node can neither
// require() nor import() it synchronously. Evaluated in a vm sandbox with the
// export keywords stripped. NEVER retyped here: a literal list of anchors in a
// test file fails for the wrong reason the first time a control gains a tip.
function loadTable(src) {
    const stripped = src.replace(/(^|\n)(\s*)export\s+(const|let|function|class)\s/g, '$1$2$3 ');
    const box = { console: { warn() {}, error() {}, log() {} } };
    vm.createContext(box);
    vm.runInContext(`${stripped}\n;globalThis.__x = { I18N, TIP_BINDINGS, LANGUAGES };`,
                    box, { timeout: 5000 });
    return box.__x;
}

// ── the page-side probe, injected once and reused ───────────────────────────
// Reads the surface's computed visibility, its rect, and its two text parts
// SEPARATELY: the title lives in a .tip-title span and the body is the text
// nodes after it, exactly as setupTooltips() builds them with createElement +
// createTextNode. Reading tip.textContent alone would concatenate the two and
// make assertion 3 unable to tell a swapped title from a swapped body.
const READ_TIP = `() => {
    const t = document.getElementById('tooltip');
    if (!t) return null;
    const cs = getComputedStyle(t);
    const r  = t.getBoundingClientRect();
    const ti = t.querySelector('.tip-title');
    const body = Array.from(t.childNodes)
        .filter((n) => n.nodeType === Node.TEXT_NODE)
        .map((n) => n.textContent).join('');
    return {
        visible: cs.visibility === 'visible' && parseFloat(cs.opacity) > 0.99,
        opacity: cs.opacity,
        title: ti ? ti.textContent : null,
        body,
        rect: { x: r.x, y: r.y, w: r.width, h: r.height },
    };
}`;

// Six of the seven sections ship collapsed; see the header. Same expression
// tests/i18n-states.json uses, so the two gates drive the same state.
const EXPAND_ALL = () =>
    document.querySelectorAll('.section.collapsed').forEach((s) => s.classList.remove('collapsed'));

(async () => {
    console.log(`== ${PLUGIN} ui_tip_render_check ==`);
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}\n`);

    const html    = fs.readFileSync(htmlPath, 'utf8');
    const i18nSrc = fs.readFileSync(i18nPath, 'utf8');

    // ── 0. the mirrored constants, guarded ──────────────────────────────────
    const size = S.readEditorSize(PLUGIN);
    check(size && size.w === SHIP_W && size.h === SHIP_H,
        `editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below `
        + `(PluginEditor.cpp says ${size ? `${size.w} x ${size.h}` : 'NOTHING'})`);

    // Parsed from the .tooltip RULE specifically. This page also carries a width
    // on .settings-popover and a max-width on .knob-label, so a loose scan for
    // the first max-width would silently measure one of those.
    const tipRule  = html.match(/\.tooltip\s*\{[\s\S]*?\}/);
    const capMatch = tipRule && tipRule[0].match(/max-width:\s*(\d+(?:\.\d+)?)px/);
    const MAX_W    = capMatch ? parseFloat(capMatch[1]) : NaN;
    check(Number.isFinite(MAX_W) && MAX_W === DOCUMENTED_MAX_W,
        `.tooltip max-width parsed from the inline <style> is the documented ${DOCUMENTED_MAX_W}px `
        + `— got ${capMatch ? capMatch[1] + 'px' : 'NOTHING'}. French wraps INSIDE this cap, so `
        + `moving it changes every tip height and therefore every clamp decision below`);
    check(new RegExp(`MARGIN\\s*=\\s*${MARGIN}\\b`).test(html),
        `setupTooltips()'s clamp MARGIN is ${MARGIN} (this file mirrors it)`);
    check(/position:\s*fixed/.test(tipRule ? tipRule[0] : '')
       && /visibility:\s*hidden/.test(tipRule ? tipRule[0] : '')
       && /pointer-events:\s*none/.test(tipRule ? tipRule[0] : ''),
        '.tooltip is position:fixed + visibility:hidden + pointer-events:none — the three '
        + 'properties that keep an unshown surface out of check-ui-labels\' sweep and stop a '
        + 'shown one stealing its own hover');
    check(/id="tooltip"/.test(html), 'index.html carries the #tooltip surface');
    // A STATIC match only. O-Comp measured that this same regex stays green
    // with the `if (lastInputWasPointer) return;` guard clause deleted, because
    // the declaration, the pointerdown write and the keydown clear all survive.
    // NC-2 below is the only thing that discriminates.
    check(/lastInputWasPointer/.test(html),
        'the focus latch is PRESENT in the shipped page (NC-2 below is what proves it WORKS — '
        + 'this line alone would pass with the guard clause removed)');
    check(!/\btitle\s*=\s*["']/.test(html.replace(/<title>[\s\S]*?<\/title>/, '')),
        'no native title= attribute reintroduced by the renderer (contract §4)');

    // ── the table ───────────────────────────────────────────────────────────
    const { I18N, TIP_BINDINGS, LANGUAGES } = loadTable(i18nSrc);
    check(Array.isArray(TIP_BINDINGS) && TIP_BINDINGS.length > 0,
        `TIP_BINDINGS parsed from js/i18n.js — ${TIP_BINDINGS.length} anchor(s)`);
    check(LANGUAGES.join(',') === 'en,fr', `LANGUAGES is en,fr — got ${LANGUAGES.join(',')}`);

    const pw = S.resolvePlaywright();
    if (pw == null) {
        console.log('\n  SKIP: playwright not resolvable. Install with');
        console.log('        npx playwright install chromium');
        console.log('  The hover-help render and its edge clamp are NOT verified without it.');
        process.exit(77);
    }

    const built = S.buildRoot(PLUGIN);
    check(built.uiRootLabel === 'Resources/ui',
        `the served root really is Resources/ui — not the Source/ui/public most of the suite `
        + `uses (serve-ui resolved "${built.uiRootLabel}" from ${built.uiRootFrom})`);

    const misses = [];
    const srv = await S.serve(built.root, (m) => misses.push(m));
    const browser = await pw.chromium.launch();
    // `viewport`, NOT `viewportSize` — the latter is the getter's name and is
    // silently IGNORED as a launch option, leaving Chromium's 1280x720 default.
    // At 1280 x 720 every anchor on this page has room below and to the right,
    // the clamp never engages, and every assertion below would pass while the
    // real 900 x 600 frame overflowed
    // (pattern_tooltip_clamp_gate_viewport_sensitive).
    const page = await browser.newPage({ viewport: { width: SHIP_W, height: SHIP_H } });

    const pageErrors = [];
    const tipWarns   = [];
    page.on('pageerror', (e) => pageErrors.push(String(e)));
    page.on('console', (m) => {
        const t = m.text();
        if (m.type() === 'error') pageErrors.push(t);
        if (/tip target not found/.test(t)) tipWarns.push(t);
    });

    const url = `http://127.0.0.1:${srv.port}/index.html`;
    const load = async () => {
        await page.goto(url, { waitUntil: 'networkidle' });
        await page.evaluate(EXPAND_ALL);
        await page.waitForTimeout(120);
    };
    await load();

    const vp = page.viewportSize();
    check(vp.width === SHIP_W && vp.height === SHIP_H,
        `the browser really is ${SHIP_W} x ${SHIP_H} — got ${vp.width} x ${vp.height}`);

    // Non-vacuity: the module must have RUN. A TDZ throw out of module
    // evaluation leaves the HTML looking correct and every control dead
    // (pattern_module_toplevel_init_tdz), and on this plugin the ENTIRE UI is
    // ONE inline module — 27 knobs, six dropdowns, the XY pad, the tab bar and
    // the i18n block would all go together. Two independent witnesses, both
    // empty in the authored HTML and written only by the module: the knob SVGs
    // (createKnobSVG) and the XY markers (populateXYMarkers).
    const alive = await page.evaluate(() => ({
        svgs: document.querySelectorAll('.knob-wrapper > svg.knob-svg').length,
        markers: document.querySelectorAll('.xy-preset-marker').length,
        expanded: document.querySelectorAll('.section.collapsed').length,
    }));
    check(alive.svgs === KNOB_COUNT,
        `the inline module ran — createKnobSVG built ${alive.svgs} knob SVGs into markup that `
        + `authors none (expected ${KNOB_COUNT})`);
    check(alive.markers === MARKER_COUNT,
        `the inline module ran — populateXYMarkers wrote ${alive.markers} XY markers into an `
        + `element the markup leaves empty (expected ${MARKER_COUNT})`);
    check(alive.expanded === 0,
        `every collapsible section is open — ${alive.expanded} still collapsed. 23 of the 27 knob `
        + `anchors sit inside one, clipped by max-height:0, so a sweep at rest would measure `
        + `boxes nobody can hover`);

    // ── 1. every binding resolves, selector AND wrapper walk ────────────────
    const resolution = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
        const el = document.querySelector(sel);
        if (!el) return { key, sel, ok: false, why: 'selector matched nothing' };
        const target = wrapper ? (el.closest(wrapper) || null) : el;
        if (!target) return { key, sel, ok: false, why: `closest(${wrapper}) matched nothing` };
        return {
            key, sel, ok: true,
            tag: target.tagName.toLowerCase()
                 + (target.className ? '.' + String(target.className).split(' ')[0] : ''),
            hasTip: target.hasAttribute('data-tip') && target.hasAttribute('data-tip-title'),
        };
    }), TIP_BINDINGS);

    for (const r of resolution) {
        check(r.ok, `[1] binding ${r.key} resolves — ${r.sel}${r.ok ? ` -> ${r.tag}` : ` (${r.why})`}`);
        if (r.ok) check(r.hasTip,
            `[1] applyI18n wrote data-tip-title + data-tip onto ${r.key}'s anchor (${r.tag})`);
    }
    check(tipWarns.length === 0,
        `[1] applyI18n logged no "tip target not found" warning`
        + (tipWarns.length ? ` — ${tipWarns.length}: ${tipWarns[0]}` : ''));

    // The chrome pair, checked as a PAIR. O-Comp's carried trap is that a
    // wrapper walk which reaches a shared ancestor makes hovering one chrome
    // control open the other's tip; here .settings-cluster holds both, so
    // #gear-btn is bound bare and #lang-select walks only to .settings-row.
    const chrome = await page.evaluate(() => {
        const gear = document.querySelector('#gear-btn');
        const sel  = document.querySelector('#lang-select');
        const row  = sel ? sel.closest('.settings-row') : null;
        return {
            gearAnchor: gear ? gear.closest('[data-tip]') === gear : false,
            rowIsAnchor: !!(row && row.hasAttribute('data-tip')),
            rowHoldsGear: !!(row && row.contains(gear)),
            rowCount: document.querySelectorAll('.settings-row').length,
            rowHoldsLang: !!(row && row.contains(sel)),
            rowHoldsTips: !!(row && document.getElementById('tips-toggle')
                             && row.contains(document.getElementById('tips-toggle'))),
        };
    });
    check(chrome.gearAnchor && chrome.rowIsAnchor && !chrome.rowHoldsGear,
        `[1] the two chrome anchors are DISJOINT — #gear-btn is its own anchor, #lang-select `
        + `walks to .settings-row, and .settings-row does not contain the gear (O-Comp's trap)`);
    // v1.4.0 REPLACES A PROXY WITH THE PROPERTY IT STOOD FOR. Until the hover-help
    // switch landed there was exactly one .settings-row, and this line asserted
    // that count — a proxy for "closest() is right by construction, not by luck"
    // (O-Tremolo's .waveform-section). There are two rows now, and the count is
    // no longer the question: the binding resolves #lang-select by its unique id
    // and then walks ANCESTORS with closest(), which cannot reach a sibling row
    // however many exist. What must hold is that the resolved wrapper is the row
    // holding #lang-select and NOT the one holding the switch, asserted directly.
    check(chrome.rowHoldsLang && !chrome.rowHoldsTips,
        `[1] #lang-select's wrapper is ITS OWN row — contains #lang-select `
        + `(${chrome.rowHoldsLang}) and not #tips-toggle (${chrome.rowHoldsTips}), `
        + `across ${chrome.rowCount} .settings-row node(s)`);

    // ── the hover driver ────────────────────────────────────────────────────
    // Hovers a DESCENDANT of the anchor wherever one exists, not the anchor
    // itself. For a knob the anchor is .knob-control — the whole 68px column —
    // and what the pointer lands on is the 50px SVG inside it, so hovering the
    // column's own box would prove the column carries a tip and nothing about
    // whether the delegated closest('[data-tip]') walk reaches it from a child.
    // For a dropdown the anchor is the .dropdown-control wrapper and the child
    // is the <select> the binding names.
    //
    // page.evaluate() given a STRING evaluates it as an EXPRESSION, so passing
    // READ_TIP directly returns the function OBJECT rather than calling it —
    // and an unserializable return arrives as undefined, which reads exactly
    // like "the tip was never there" and sails through a truthiness assertion
    // (O-Bass, batch M1). Invoked explicitly instead.
    const readTip = () => page.evaluate(`(${READ_TIP})()`);
    const hoverSelFor = (sel) => {
        if (sel.startsWith('.knob-control')) return `${sel} .knob-wrapper`;
        if (sel === '#dualBore-toggle')      return '#dualBore-toggle .toggle-track';
        return sel;
    };

    const park = async () => {
        await page.mouse.move(2, 2);
        await page.waitForFunction(
            `(${READ_TIP})() === null || !(${READ_TIP})().visible`, null, { timeout: 2000 }
        ).catch(() => {});
    };

    const hoverAndRead = async (sel) => {
        await park();
        await page.hover(sel, { force: true });
        // AWAITED, not slept past: the surface fades in over 0.12s and reading
        // mid-transition returns opacity 0.4 with a rect that is already final,
        // so a fixed sleep either flakes or hides a tip that never showed.
        await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 2000 })
                  .catch(() => {});
        return readTip();
    };

    const inFrame = (r) =>
        r.x >= 0 && r.y >= 0 && r.x + r.w <= SHIP_W && r.y + r.h <= SHIP_H;
    const edges = (r) =>
        `L${r.x.toFixed(1)} T${r.y.toFixed(1)} R${(SHIP_W - r.x - r.w).toFixed(1)} `
        + `B${(SHIP_H - r.y - r.h).toFixed(1)}`;

    // #lang-select lives inside a popover that ships hidden, so it is not
    // hoverable until the gear is clicked. Everything else is reachable once
    // the sections are open.
    const NEEDS_POPOVER = new Set(['#lang-select']);

    const sweep = async (lang) => {
        console.log(`\n-- ${lang.toUpperCase()}: hover every anchor, byte-compare, measure the rect`);
        let popoverOpen = false;
        const seen = [];
        let tallest = { key: null, h: 0 };
        for (const [sel, key] of TIP_BINDINGS) {
            const entry = (I18N[key] || {})[lang];
            if (!entry) { check(false, `[2] ${key} has an ${lang} entry`); continue; }

            if (NEEDS_POPOVER.has(sel) && !popoverOpen) {
                await page.click('#gear-btn', { force: true });
                await page.waitForSelector('#settings-popover:not([hidden])', { timeout: 2000 });
                popoverOpen = true;
            }

            const st = await hoverAndRead(hoverSelFor(sel));
            seen.push(key);

            check(st !== null && st.visible,
                `[2][${lang}] ${key}: hovering ${hoverSelFor(sel)} SHOWS the tip `
                + `(opacity ${st ? st.opacity : 'n/a'})`);
            if (!st || !st.visible) continue;
            if (st.rect.h > tallest.h) tallest = { key, h: st.rect.h };

            check((st.title || '').length > 0 && (st.body || '').length > 0,
                `[2][${lang}] ${key}: the surface carries non-empty title AND body`);
            check(st.title === entry.t,
                `[3][${lang}] ${key}: rendered title is BYTE-EQUAL to the table `
                + `— "${st.title}" vs "${entry.t}"`);
            check(st.body === entry.b,
                `[3][${lang}] ${key}: rendered body is BYTE-EQUAL to the table `
                + `(${st.body.length} vs ${entry.b.length} chars)`
                + (st.body === entry.b ? '' : `\n        got: ${st.body.slice(0, 90)}`));
            check(st.rect.w <= MAX_W + 0.5,
                `[4][${lang}] ${key}: width ${st.rect.w.toFixed(1)}px is within the ${MAX_W}px cap`);
            check(inFrame(st.rect),
                `[4][${lang}] ${key}: rect ${st.rect.w.toFixed(1)} x ${st.rect.h.toFixed(1)} is `
                + `fully inside ${SHIP_W} x ${SHIP_H} — edge clearances ${edges(st.rect)}`);
        }
        check(seen.length === TIP_BINDINGS.length,
            `[2][${lang}] every one of the ${TIP_BINDINGS.length} bound anchors was driven `
            + `— got ${seen.length}`);
        console.log(`   tallest ${lang} tip: ${tallest.key} at ${tallest.h.toFixed(1)}px `
            + `in a ${SHIP_H}px frame`);
        if (popoverOpen) {
            await page.keyboard.press('Escape');       // also clears the focus latch
            await page.waitForTimeout(120);
        }
        return tallest;
    };

    const tallestEn = await sweep('en');

    // ── NC-3. the child-boundary rule ───────────────────────────────────────
    // pointerout fires at every internal boundary. Without the
    // anchorOf(relatedTarget) === active guard the tip flickers off and on as
    // the pointer crosses from the SVG wrapper to the caption inside the same
    // cell. Driven on breathPressure, the first knob on the page and one of the
    // four that are visible without expanding anything.
    console.log('\n-- NC-3: moving between two children of the SAME anchor must not hide the tip');
    const bpSel = '.knob-control[data-param="breathPressure"]';
    const beforeMove = await hoverAndRead(`${bpSel} .knob-wrapper`);
    await page.hover(`${bpSel} .knob-label`, { force: true });
    await page.waitForTimeout(200);
    const afterMove = await readTip();
    check(beforeMove.visible && afterMove.visible
          && afterMove.title === beforeMove.title && afterMove.title === I18N['tip.breath'].en.t,
        `[NC-3] the tip survives the SVG -> caption boundary inside .knob-control `
        + `— still "${afterMove.title}"`);
    await park();

    // ── 5. French, then back ────────────────────────────────────────────────
    // French runs 15-20% longer, wraps to more lines against the max-width cap
    // and grows the tip's HEIGHT, so a tip that fits in English can overflow the
    // bottom of the frame in French. That is why the whole sweep repeats rather
    // than spot-checking one anchor.
    await page.evaluate((l) => window.__setLanguage(l), 'fr');
    await page.waitForTimeout(150);
    const frLang = await page.evaluate(() => document.getElementById('lang-select').value);
    check(frLang === 'fr', `[5] window.__setLanguage('fr') took — selector reads "${frLang}"`);
    const tallestFr = await sweep('fr');
    console.log(`   FR grows the tallest tip ${tallestEn.h.toFixed(1)} -> ${tallestFr.h.toFixed(1)}px`);

    await page.evaluate((l) => window.__setLanguage(l), 'en');
    await page.waitForTimeout(150);
    const backSt = await hoverAndRead(`${bpSel} .knob-wrapper`);
    check(backSt.visible && backSt.title === I18N['tip.breath'].en.t
          && backSt.body === I18N['tip.breath'].en.b,
        `[5] switching back to English restores the English tip byte-for-byte `
        + `— "${backSt.title}"`);
    await park();

    // ── NC-2. the focus latch, both halves ──────────────────────────────────
    // This plugin diverges from the O-simpleFM reference here on purpose, and a
    // divergence with no control is a regression waiting to be inherited.
    console.log('\n-- NC-2: the focus latch (a click must not pin a tip; a key must still open one)');
    await page.mouse.move(1, 1);
    await page.waitForTimeout(150);
    // BLUR FIRST, and this line is the whole reason the assertion below can FAIL
    // at all. The sweep above leaves focus on #gear-btn (the popover pass
    // clicked it), and clicking an ALREADY-FOCUSED element fires no focusin —
    // so without this the check reports "no tip after a click" for a page with
    // no latch whatsoever. Measured on the M1 pilots: 125/125 green with the
    // latch deleted, and 186/186 on O-Tremolo with the latch AND this line both
    // deleted.
    await page.evaluate(() => { if (document.activeElement) document.activeElement.blur(); });
    await page.waitForTimeout(100);
    await page.click('#gear-btn');
    await page.waitForTimeout(300);
    const afterClick = await page.evaluate(() => {
        const t  = document.getElementById('tooltip');
        const cs = getComputedStyle(t);
        const r  = t.getBoundingClientRect();
        const p  = document.getElementById('settings-popover');
        const pr = p && !p.hidden ? p.getBoundingClientRect() : null;
        const shown = cs.visibility !== 'hidden' && parseFloat(cs.opacity) > 0.01;
        let overlap = 0;
        if (shown && pr) {
            const ox = Math.max(0, Math.min(r.right, pr.right) - Math.max(r.left, pr.left));
            const oy = Math.max(0, Math.min(r.bottom, pr.bottom) - Math.max(r.top, pr.top));
            overlap = Math.round(ox * oy);
        }
        return {
            shown, overlap,
            tip: `[${Math.round(r.x)},${Math.round(r.y)},${Math.round(r.width)}x${Math.round(r.height)}]`,
            pop: pr ? `[${Math.round(pr.x)},${Math.round(pr.y)},${Math.round(pr.width)}x${Math.round(pr.height)}]` : 'closed',
        };
    });
    check(!afterClick.shown,
        `[NC-2a] a POINTER click on #gear-btn leaves NO tip parked on screen — the latch `
        + `suppresses the focusin arm`
        + (afterClick.shown
            ? ` (tip ${afterClick.tip} covers the popover ${afterClick.pop} by `
              + `${afterClick.overlap} px2)`
            : ` (popover ${afterClick.pop} is unobscured)`));

    // The keyboard half. A real tab-ring walk, not a programmatic .focus():
    // Chromium reports :focus-visible false for a .focus() that follows a click,
    // and .focus() on an already-focused element fires no event at all — either
    // one would report "no tip" and record that as correct.
    await page.keyboard.press('Escape');
    await page.waitForTimeout(150);
    await page.evaluate(() => { if (document.activeElement) document.activeElement.blur(); });
    await park();
    let kbHit = null;
    for (let i = 1; i <= 20; ++i) {
        await page.keyboard.press('Tab');
        // AWAITED, not slept past. The surface fades in over 0.12s and `visible`
        // demands opacity > 0.99, so a fixed 80 ms sleep reads part-way and
        // records "no tip" on a tab that DID open one — O-Comp measured exactly
        // that, reported "none in 20 tabs" for a path that works, and the
        // obvious response to that reading is to delete the latch.
        await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 400 })
                  .catch(() => {});
        const r = await page.evaluate(`(${READ_TIP})()`);
        const on = await page.evaluate(() => {
            const a = document.activeElement;
            return a ? (a.id || a.className || a.tagName.toLowerCase()) : null;
        });
        if (r && r.visible && r.title) { kbHit = { press: i, on, ...r }; break; }
    }
    check(kbHit !== null,
        `[NC-2b] a KEYBOARD tab still OPENS a tip — the accessibility half the latch must not `
        + `kill` + (kbHit ? ` (tab #${kbHit.press} on "${kbHit.on}", title "${kbHit.title}")`
                          : ' — none in 20 tabs'));
    check(kbHit !== null && inFrame(kbHit.rect),
        `[NC-2b] the focus-placed tip is inside the frame`
        + (kbHit ? ` — ${edges(kbHit.rect)}` : ''));
    await page.keyboard.press('Escape');
    await park();

    // ── NC-1. plant an over-long body; assertion 4 must report it ───────────
    // A namespaced per-run directory, never a bare filename at a shared temp
    // root: several executors run in this scratchpad at once and a bare
    // i18n.orig.js is not yours. Restored by COPY, never with
    // `git checkout -- <file>`.
    //
    // THE PLANT IS SIZED AGAINST THIS FRAME. The surface is capped at 260px
    // wide with 22px of horizontal padding, so roughly 238px of text at 11.5px
    // — about 43 characters a line — over a 1.45 line-height, so ~16.7px per
    // line. Clearing 600px of frame takes ~36 lines, ~1550 characters. The
    // repeat below is ~5300, roughly 3.4x what is needed, and the height is
    // ASSERTED before the overflow claim is believed: O-Tremolo measured a
    // plant that FIT inside its frame and reported nothing, and a plant that
    // fits is indistinguishable from a gate that cannot see.
    console.log('\n-- NC-1: plant an over-long body and confirm assertion 4 reports the overflow');
    const nc = fs.mkdtempSync(path.join(os.tmpdir(), 'oreed-tip-nc-'));
    const servedI18n = path.join(built.root, 'js', 'i18n.js');
    const backup = path.join(nc, 'i18n.served.orig.js');
    fs.copyFileSync(servedI18n, backup);
    const origSrc = fs.readFileSync(backup, 'utf8');
    try {
        // No apostrophe anywhere in the plant: the bodies in this file are
        // SINGLE-quoted, so one would close the string and break the module
        // rather than overflow the frame — a plant that fails for the wrong
        // reason is a control that proves nothing.
        const LONG = ('Une phrase de controle negatif, deliberement beaucoup trop longue pour '
                    + 'tenir dans le cadre, repetee afin de faire deborder la surface par le bas. ')
                    .repeat(36);
        const planted = origSrc.replace(
            /('tip\.breath':[\s\S]*?en: \{ t: 'Breath Pressure',\s*\n\s*b: )'[^']*'/,
            `$1'${LONG}'`);
        check(planted !== origSrc,
            `[NC-1] the plant actually edited tip.breath's en body — a no-op replace would make `
            + `this control vacuous`);
        fs.writeFileSync(servedI18n, planted);

        await load();
        const bad = await hoverAndRead(`${bpSel} .knob-wrapper`);
        check(bad.visible, `[NC-1] the planted tip still renders`);
        check(bad.visible && bad.rect.h > SHIP_H,
            `[NC-1] the plant is BIGGER THAN THE FRAME — ${bad.rect.h.toFixed(1)}px tall against `
            + `${SHIP_H}px. A plant the clamp can absorb reports nothing and proves nothing `
            + `(O-Tremolo measured a 40x plant fitting inside a 400px frame)`);
        check(!inFrame(bad.rect),
            `[NC-1] assertion 4 REPORTS the overflow — rect ${bad.rect.w.toFixed(1)} x `
            + `${bad.rect.h.toFixed(1)} at ${bad.rect.x.toFixed(1)},${bad.rect.y.toFixed(1)} `
            + `leaves the ${SHIP_W} x ${SHIP_H} frame (${edges(bad.rect)}). If this PASSES as `
            + `in-frame, assertion 4 is decoration and every green above means nothing`);
    } finally {
        fs.copyFileSync(backup, servedI18n);
    }

    await load();
    const restored = await hoverAndRead(`${bpSel} .knob-wrapper`);
    check(fs.readFileSync(servedI18n, 'utf8') === origSrc,
        `[NC-1] the served copy is byte-identical to the pre-plant snapshot again`);
    check(restored.visible && restored.body === I18N['tip.breath'].en.b && inFrame(restored.rect),
        `[NC-1] restored — tip.breath is back inside the frame (${edges(restored.rect)})`);
    fs.rmSync(nc, { recursive: true, force: true });

    // ── housekeeping ────────────────────────────────────────────────────────
    // The two pre-existing bind warnings are NOT errors and are expected: the
    // page's own bindSliderParam('referencePitch') and
    // bindComboBox('tuningSystem', 'tuningSystem') each console.warn at load
    // because neither parameter has a control on this page. They are reported
    // in the v1.3.0 commit message rather than fixed here.
    console.log('');
    check(pageErrors.length === 0,
        `no uncaught page error across the whole run`
        + (pageErrors.length ? ` — ${pageErrors.length}: ${pageErrors[0].slice(0, 140)}` : ''));
    const realMisses = [...new Set(misses)].filter((m) => !/favicon/.test(m));
    check(realMisses.length === 0,
        `every requested resource was served`
        + (realMisses.length ? ` — 404: ${realMisses.slice(0, 5).join(', ')}` : ''));

    await browser.close();
    await srv.close();
    fs.rmSync(built.root, { recursive: true, force: true });

    console.log(`\n${failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} FAILED ==`}`);
    process.exit(failed);
})().catch((e) => {
    console.error('ui_tip_render_check: harness failure —', e);
    process.exit(1);
});
