/*
   This file is part of O-Bells, an Ouaricon Audio plugin.
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
    O-Bells — hover-help RENDER verification at the shipping viewport (v4.3.0).

    THE FIRST RUNNABLE GATE THIS PLUGIN HAS. tests/ holds i18n-states.json and
    ui-stub/ — data, consumed by scripts/check-ui-labels.js — and nothing
    executable. There is no Source/tests/ either (checked; that is where
    O-MicrotonalSampler hid its gate from batch M2's "run everything in tests/"
    instruction).

    WHY IT EXISTS. No gate in this repo can see a rendered tooltip.
    check-i18n reads the table statically and is satisfied by TIP_BINDINGS being
    non-empty and its keys resolving. check-ui-labels has no tooltip awareness
    whatsoever — measured on this plugin: its whole 14-state output is identical
    before and after v4.3.0 except for one retired label. boot-all-uis counts
    aria-label and title and never data-tip — also measured here: text=147
    aria=19 title=0 before AND after. So a plugin can author copy, bind it, and
    pass all three green while showing nothing on screen, which is exactly the
    state O-Bells was in at v4.2.0: applyI18n() writes data-tip-title and
    data-tip onto the anchors and stops, and that page had no #tooltip element,
    no .tooltip rule and no hover handler to read them.

    This file is the assertion those three cannot make. It drives the REAL page
    — same index.html, same inline <style>, same two inline modules, same
    vendored js/tuning-panel.js, only the JUCE bridge stubbed — at the exact
    shipping frame, and measures the rectangle that actually paints.

    NOT a port of the three committed ui_tooltip_clamp_check.js gates
    (O-Tapestop, O-Bitrot, O-ReverseDelay). Those are ~40 KB each and are built
    around the OTHER renderer family: measure-then-pin placement with an
    above/below flip and an arrow. O-Bells ports O-simpleFM's delegated
    cursor-following renderer, which has neither.

    ── THIS PAGE IS A THREE-PANEL TAB DECK, AND THAT IS LOAD-BEARING ─────────

    A .tab-content that is not .active is `display: none` — a zero-size rect
    that cannot be hovered and cannot be measured. 42 of the 65 anchors are on
    #instrument-tab, 2 on #tuning-tab and 21 on #effects-tab.

    EVERY STATE IS ENTERED THROUGH THE PAGE'S OWN PATH — the tab is CLICKED.
    Stripping `display:none` off #tuning-tab would measure a state the plugin
    only reaches by a click nobody made (M2 finding 5). The same rule covers the
    two other hidden states:

      - the six Bloom fine sliders live in #bloom-fine-content, `display: none`
        until #bloom-fine-toggle is clicked. Clicking it ALSO puts
        `pointer-events: none` on the two main Bloom cells, so those two are
        swept BEFORE the toggle is thrown and the gating itself is pinned as its
        own assertion, [1e], rather than being worked around silently;
      - #lang-select is inside #settings-popover, which ships `hidden`. The gear
        is clicked.

    ── TWO ANCHORS ARRIVE LATE, ON PURPOSE ──────────────────────────────────

    #ref-pitch-knob and #octave-stretch are built by js/tuning-panel.js, which
    index.html imports inside an async IIFE. They do not exist when initI18n()
    sweeps, so applyI18n() console.warns "tip target not found" for each on that
    first pass — and then window.__reapplyI18n(), which this page already
    carries and which the panel's own init calls after mounting, binds them.

    That is the difference between O-Bells and O-Reed, whose referencePitch was
    reported page-unreachable in batch M2 for the identical DOM shape: O-Reed's
    page has no __reapplyI18n. Assertion [1b] therefore does not relax the
    warning check — it pins the warning set to EXACTLY those two selectors, so a
    third one appearing is still a failure.

    WHAT IS ASSERTED, per anchor, in English and again in French:

      1. every TIP_BINDINGS selector resolves AND its closest(wrapper) walk
         lands on a real element carrying both attributes — a binding that finds
         nothing is a hard FAIL, never a warning, because applyI18n() falls back
         to `el.closest(w) || el` and a broken wrapper still opens a tip on the
         wrong-sized cell while assertions 2, 3 and 4 all stay green (M2
         finding 3, from O-Reed). All 65 targets must also be DISTINCT nodes and
         each must clear a minimum hover area;
      2. hovering a DESCENDANT of the anchor makes the surface VISIBLE with
         non-empty text. Hovering a descendant is the point: it is what the
         pointer lands on, and it exercises the delegated closest('[data-tip]')
         walk that makes this renderer work at all;
      3. the rendered title and body are BYTE-EQUAL to the table entry. Not
         "contains": a .tip-title that silently kept the previous anchor's text
         passes a contains check and fails this one;
      4. the tip rectangle is fully inside the 800 x 600 frame on all four
         edges, and no wider than the .tooltip max-width cap.

    Then French (2-4 again against the fr entries), then back to English to
    prove the switch is reversible rather than one-way.

    FIVE NEGATIVE CONTROLS, because "all pass" is worthless without them:

      NC-1  an over-long body is planted in the SERVED copy of js/i18n.js and
            the page reloaded; assertion 4 must REPORT the overflow. THE PLANT
            IS SEARCHED FOR, NOT GUESSED. 800 x 600 is a roomy frame and a
            habitual 40x plant fits inside it and reports nothing — O-Tremolo
            measured exactly that on a 400px frame, and O-Detune measured ~448px
            of clamp room on a smaller frame than this one. The search doubles
            the repeat count until the rendered height actually exceeds the
            frame, and reports the count it landed on.
      NC-1b the CLAMP FLOOR, driven directly rather than credited. On this frame
            with real copy NEITHER clamp arm is reachable — the arithmetic is in
            the [4b] note below — so the sweep passing proves nothing about the
            clamp. The oversized plant is the only state in which the floor
            fires, and it is asserted there, on the number: top === MARGIN.
            Batch M2 found eleven ports crediting a post-flip re-clamp line that
            cannot execute; this gate does not repeat that.
      NC-2  the focus latch, BOTH halves, separately: a pointer click must leave
            no tip parked on screen, AND a real tab-ring walk must still open
            one. Asserting only the first lets the feature decay into "focus
            never shows a tip", which passes that assertion perfectly while
            silently removing the keyboard half of hover-help.
            THE CLICK IS PRECEDED BY A BLUR. Clicking an already-focused element
            fires no focusin at all, so without the blur this assertion passes
            for a page with NO latch whatsoever. The overlap with the settings
            popover is measured in px2 rather than merely observed.
            The keyboard half AWAITS the 0.12s fade rather than sleeping past
            it: O-Comp measured an 80ms read reporting "never opens" for a path
            that demonstrably works, and the obvious response to that reading is
            to delete the latch.
      NC-3  the pointerout child-boundary rule: moving between two children of
            the SAME anchor must not flicker the tip off.
      NC-4  the DRAG GUARD, both directions. This page's sliders, FX knobs and
            A4 knob all start a drag on mousedown and track document.mousemove,
            and none of them calls setPointerCapture — so without the guard a
            drag that strays into a neighbouring cell opens the NEIGHBOUR's tip
            over the control under the user's hand. The control asserts the HOLD
            (no tip while dragging across a neighbour) AND the RELEASE (the
            neighbour's tip opens normally afterwards). Asserting only the hold
            would pass for a permanent off switch.
      NC-5  the double-click value editor. Its <input> is created inside the
            .knob-container anchor and focused programmatically; neither the tip
            nor the editor should end up fighting for the same box.

    Usage:  node plugins/O-Bells/tests/ui_tip_render_check.js
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

const PLUGIN  = 'O-Bells';
// O-Bells' served root is Resources/ui, NOT the Source/ui/public most of the
// suite uses. Named here so the divergence is visible, and cross-checked
// against serve-ui's own CMake-derived resolver below rather than trusted.
const UI_ROOT   = path.join(pluginRoot, 'Resources', 'ui');
const htmlPath  = path.join(UI_ROOT, 'index.html');
const i18nPath  = path.join(UI_ROOT, 'js', 'i18n.js');

// Mirrored from PluginEditor.cpp and the inline <style>, and CROSS-CHECKED
// against both below. A fixture that mirrors a constant without checking it
// starts describing the release before it and keeps passing
// (pattern_test_fixture_mirrors_drift_silently).
const SHIP_W = 800;
const SHIP_H = 600;
const MARGIN = 8;               // setupTooltips()'s clamp margin
const DOCUMENTED_MAX_W = 260;   // .tooltip max-width
const FX_KNOB_COUNT = 16;       // makeFxKnob() calls across the four FX sections
const SLIDER_COUNT  = 35;       // sliderParams entries in index.html
const MIN_HOVER_AREA = 400;     // px2 — the floor an anchor must clear to be openable

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

// Which state each anchor needs. Derived from the selector, never hand-listed
// twice: a second list drifts from TIP_BINDINGS the first time a tip is added.
const FINE = new Set(['bloomSpeedLow', 'bloomSpeedMid', 'bloomSpeedHigh',
                      'bloomAmountLow', 'bloomAmountMid', 'bloomAmountHigh']);
const TUNING_SELS  = new Set(['#ref-pitch-knob', '#octave-stretch']);
const POPOVER_SELS = new Set(['#lang-select']);
const paramOf = (sel) => (sel.match(/data-param="([^"]+)"/) || [])[1] || null;
const tabFor  = (sel) => {
    if (TUNING_SELS.has(sel)) return 'tuning';
    if (/^#(chorus|delay|eq|reverb)/.test(sel)) return 'effects';
    return 'instrument';
};
const needsFine = (sel) => FINE.has(paramOf(sel) || '');

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
    // on .settings-popover and a max-height on .preset-dropdown, so a loose scan
    // for the first max-width would silently measure one of those.
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
    // STATIC matches only, and labelled as such. O-Comp measured that this same
    // regex stays green with the `if (lastInputWasPointer) return;` guard clause
    // deleted, because the declaration, the pointerdown write and the keydown
    // clear all survive; the same is true of pointerHeld with the
    // `if (pointerHeld) return;` line removed. NC-2 and NC-4 are the only things
    // that discriminate.
    check(/lastInputWasPointer/.test(html),
        'PRESENCE NOTE ONLY: the focus latch appears in the shipped page (NC-2 is what proves '
        + 'it WORKS — this line alone passes with the guard clause removed)');
    check(/pointerHeld/.test(html),
        'PRESENCE NOTE ONLY: the drag guard appears in the shipped page (NC-4 is the control)');
    // A CALL, not the word: the renderer's own comment names it, so a bare
    // substring test fails on the documentation explaining why the flag exists.
    check(!/\.setPointerCapture\s*\(/.test(html),
        'nothing in index.html CALLS setPointerCapture — which is WHY the drag guard is needed '
        + 'here and was not needed on O-AnalogEQ, where pointer capture already retargets every '
        + 'boundary event for the duration of a drag (M2 finding 2). If this ever fails, re-check '
        + 'whether the pointerHeld flag is still doing anything');
    check(/if \(isEditing\(\)\) return;/.test(html),
        'PRESENCE NOTE ONLY: the editor guard is inside show(), not focusin — NC-5 is what '
        + 'proves the placement, and the focusin-only draft passed every static check while '
        + 'failing NC-5');
    check(!/\btitle\s*=\s*["']/.test(html.replace(/<title>[\s\S]*?<\/title>/, '')),
        'no native title= attribute reintroduced by the renderer (contract §4)');
    check(!/toggle-tooltip/.test(html.replace(/<!--[\s\S]*?-->/g, '')),
        'the bespoke .hi-fi-toggle hover note is GONE — a second hover surface at z-index 100 '
        + 'would have painted alongside #tooltip on the same hover');

    // ── the table ───────────────────────────────────────────────────────────
    const { I18N, TIP_BINDINGS, LANGUAGES } = loadTable(i18nSrc);
    check(Array.isArray(TIP_BINDINGS) && TIP_BINDINGS.length > 0,
        `TIP_BINDINGS parsed from js/i18n.js — ${TIP_BINDINGS.length} anchor(s)`);
    check(LANGUAGES.join(',') === 'en,fr', `LANGUAGES is en,fr — got ${LANGUAGES.join(',')}`);
    check(Object.keys(I18N).length === TIP_BINDINGS.length,
        `every I18N entry is bound and every binding has an entry — ${Object.keys(I18N).length} `
        + `entries vs ${TIP_BINDINGS.length} bindings`);
    // French convention, checked in the file rather than by eye. A decimal
    // POINT inside a French body is the split M1 had and settled the other way
    // (O-SimpleReverb shipped one without flagging it at all).
    const frPoints = Object.entries(I18N)
        .filter(([, e]) => /\d+\.\d+/.test((e.fr && e.fr.b) || ''))
        .map(([k]) => k);
    check(frPoints.length === 0,
        `no French body uses a decimal POINT — a tooltip body is prose and takes the French `
        + `comma; the READOUT keeps its point under D-03`
        + (frPoints.length ? ` (offenders: ${frPoints.join(', ')})` : ''));
    const unreviewed = Object.values(I18N).filter((e) => e.fr && e.fr.reviewed === false).length;
    check(unreviewed === Object.keys(I18N).length,
        `all ${unreviewed} French entries are flagged reviewed: false — no native speaker has `
        + `read them and none may be hidden from the worklist`);

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
    // silently IGNORED as a launch option, leaving Chromium's 1280x720 default,
    // where every anchor has room below and to the right and the clamp never
    // engages (pattern_tooltip_clamp_gate_viewport_sensitive).
    const page = await browser.newPage({ viewport: { width: SHIP_W, height: SHIP_H } });

    const pageErrors = [];
    let   tipWarns   = [];
    page.on('pageerror', (e) => pageErrors.push(String(e)));
    page.on('console', (m) => {
        const t = m.text();
        if (m.type() === 'error') pageErrors.push(t);
        if (/tip target not found/.test(t)) tipWarns.push(t);
    });

    const url = `http://127.0.0.1:${srv.port}/index.html`;
    const load = async () => {
        tipWarns = [];
        await page.goto(url, { waitUntil: 'networkidle' });
        // The tuning panel is imported inside an async IIFE; wait for the DOM it
        // builds rather than for a fixed sleep, so the two late anchors are
        // present before anything is measured.
        await page.waitForSelector('#ref-pitch-knob', { timeout: 5000 }).catch(() => {});
        await page.waitForTimeout(150);
    };
    await load();

    const vp = page.viewportSize();
    check(vp.width === SHIP_W && vp.height === SHIP_H,
        `the browser really is ${SHIP_W} x ${SHIP_H} — got ${vp.width} x ${vp.height}`);

    // Non-vacuity: the module must have RUN. A TDZ throw out of module
    // evaluation leaves the HTML looking correct and every control dead
    // (pattern_module_toplevel_init_tdz), and on this page the ENTIRE UI is two
    // inline modules — 35 sliders, the tab bar, the preset browser, the sixteen
    // FX knobs and the i18n block would all go together. Three independent
    // witnesses, each written only by the module into markup that authors none:
    // the FX knob containers, the tuning panel's subtree, and the FX bypass
    // captions setLabel() writes.
    const alive = await page.evaluate(() => ({
        fxKnobs: document.querySelectorAll('.fx-knobs .knob-container').length,
        sliders: document.querySelectorAll('.slider[data-param]').length,
        panel:   document.querySelectorAll('#tuning-container .tuning-panel, #tuning-container > *').length,
        refKnob: !!document.getElementById('ref-pitch-knob'),
        bypassLabelled: document.getElementById('chorusBypassBtn')
                        ? document.getElementById('chorusBypassBtn').dataset.label || ''
                        : null,
    }));
    check(alive.fxKnobs === FX_KNOB_COUNT,
        `the inline module ran — makeFxKnob built ${alive.fxKnobs} .knob-container cells into four `
        + `divs the markup leaves EMPTY (expected ${FX_KNOB_COUNT})`);
    check(alive.sliders === SLIDER_COUNT,
        `${alive.sliders} .slider[data-param] cells in the markup (expected ${SLIDER_COUNT}, the `
        + `length of sliderParams)`);
    check(alive.refKnob && alive.panel > 0,
        `the lazily-imported tuning panel MOUNTED — #tuning-container has ${alive.panel} child `
        + `subtree(s) and #ref-pitch-knob exists. Without this the two late anchors below would `
        + `fail for the wrong reason`);
    check(alive.bypassLabelled === 'On' || alive.bypassLabelled === 'Off',
        `setLabel() wrote the bypass caption — #chorusBypassBtn carries dataset.label `
        + `"${alive.bypassLabelled}"`);

    // ── 1. every binding resolves, selector AND wrapper walk ────────────────
    // Run per state, because a selector inside a display:none tab still
    // RESOLVES (querySelector does not care about layout) but its rect is 0x0.
    // So resolution is checked once, globally, and the hover AREA is checked in
    // the state the anchor is actually reachable in.
    const resolution = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
        const el = document.querySelector(sel);
        if (!el) return { key, sel, ok: false, why: 'selector matched nothing' };
        const target = wrapper ? (el.closest(wrapper) || null) : el;
        if (!target) return { key, sel, ok: false, why: `closest(${wrapper}) matched nothing` };
        // A stable identity for the distinctness check below.
        let n = target, path = [];
        while (n && n !== document.body) {
            const p = n.parentElement;
            path.unshift(p ? [...p.children].indexOf(n) : 0);
            n = p;
        }
        return {
            key, sel, ok: true, path: path.join('/'),
            tag: target.tagName.toLowerCase()
                 + (target.id ? '#' + target.id
                    : (target.className && typeof target.className === 'string'
                       ? '.' + target.className.trim().split(/\s+/)[0] : '')),
            hasTip: target.hasAttribute('data-tip') && target.hasAttribute('data-tip-title'),
            bare: !wrapper,
        };
    }), TIP_BINDINGS);

    for (const r of resolution) {
        check(r.ok, `[1] binding ${r.key} resolves — ${r.sel}${r.ok ? ` -> ${r.tag}` : ` (${r.why})`}`);
        if (r.ok) check(r.hasTip,
            `[1] applyI18n wrote data-tip-title + data-tip onto ${r.key}'s anchor (${r.tag})`);
    }

    // M2 finding 4: a second binding whose wrapper walk lands on a node the
    // first one already owns silently overwrites it, and check-i18n happily
    // reports two bound tips.
    const paths = resolution.filter((r) => r.ok).map((r) => r.path);
    const dupes = paths.filter((p, i) => paths.indexOf(p) !== i);
    check(dupes.length === 0,
        `[1] all ${paths.length} anchors are DISTINCT nodes — a wrapper walk that collides `
        + `overwrites the first tip and still reports two bound`
        + (dupes.length ? ` (${dupes.length} collision(s))` : ''));

    // [1b] The warning set is PINNED, not relaxed. See the header.
    const EXPECTED_LATE = ['#ref-pitch-knob', '#octave-stretch'];
    const warnedSels = [...new Set(tipWarns.map((w) => w.replace(/.*tip target not found:\s*/, '')))].sort();
    check(JSON.stringify(warnedSels) === JSON.stringify([...EXPECTED_LATE].sort()),
        `[1b] the only "tip target not found" warnings are the two lazily-mounted tuning-panel `
        + `anchors — got [${warnedSels.join(', ')}], expected [${EXPECTED_LATE.join(', ')}]. They `
        + `bind on the panel's own window.__reapplyI18n(); the warning is the whole cost and `
        + `boot-all-uis filters on console.error, so it never sees it`);

    // [1c] the chrome pair, checked as a PAIR. O-Comp's carried trap is that a
    // wrapper walk reaching a shared ancestor makes hovering one chrome control
    // open the other's tip; here .settings-cluster holds BOTH the gear and the
    // popover, so #gear-btn is bound bare and #lang-select walks only to
    // .settings-row.
    const chrome = await page.evaluate(() => {
        const gear = document.querySelector('#gear-btn');
        const sel  = document.querySelector('#lang-select');
        const row  = sel ? sel.closest('.settings-row') : null;
        return {
            gearAnchor: gear ? gear.closest('[data-tip]') === gear : false,
            rowIsAnchor: !!(row && row.hasAttribute('data-tip')),
            rowHoldsGear: !!(row && row.contains(gear)),
            rowCount: document.querySelectorAll('.settings-row').length,
            clusterHoldsBoth: !!(document.querySelector('.settings-cluster')
                && document.querySelector('.settings-cluster').contains(gear)
                && document.querySelector('.settings-cluster').contains(sel)),
        };
    });
    check(chrome.gearAnchor && chrome.rowIsAnchor && !chrome.rowHoldsGear,
        `[1c] the two chrome anchors are DISJOINT — #gear-btn is its own anchor, #lang-select `
        + `walks to .settings-row, and .settings-row does not contain the gear`);
    check(chrome.clusterHoldsBoth,
        `[1c] .settings-cluster really does contain BOTH — which is why the gear is bound bare. `
        + `If this ever fails the bare binding has stopped being load-bearing`);
    check(chrome.rowCount === 1,
        `[1c] .settings-row is unique on this page — ${chrome.rowCount} node(s). A wrapper class `
        + `that matches twice makes closest() right by luck (O-Tremolo's .waveform-section)`);

    // ── the state driver — the page's own path, never a class strip ─────────
    let curTab = 'instrument';
    let fineOn = false;
    let popoverOpen = false;
    const setTab = async (t) => {
        if (curTab === t) return;
        await page.click(`.tab[data-tab="${t}"]`, { force: true });
        await page.waitForSelector(`#${t}-tab.active`, { timeout: 2000 });
        await page.waitForTimeout(120);
        curTab = t;
    };
    const setFine = async (on) => {
        if (fineOn === on) return;
        await setTab('instrument');
        await page.click('#bloom-fine-toggle', { force: true });
        await page.waitForTimeout(150);
        fineOn = on;
    };
    const setPopover = async (on) => {
        if (popoverOpen === on) return;
        await page.click('#gear-btn', { force: true });
        if (on) await page.waitForSelector('#settings-popover:not([hidden])', { timeout: 2000 });
        await page.waitForTimeout(120);
        popoverOpen = on;
    };
    const ensureStateFor = async (sel) => {
        await setPopover(POPOVER_SELS.has(sel));
        await setTab(tabFor(sel));
        await setFine(needsFine(sel));
    };

    // ── the hover driver ────────────────────────────────────────────────────
    // Hovers a DESCENDANT of the anchor wherever one exists. For the 55
    // wrapper-bound anchors the SELECTOR is already the descendant — the 6px
    // .slider track inside a .param-control column, the 44px SVG box inside a
    // .knob-container — so hovering the selector is exactly the delegated
    // closest('[data-tip]') walk. For the 10 bare-bound anchors a child is
    // named where one exists, so those exercise the walk too.
    //
    // page.evaluate() given a STRING evaluates it as an EXPRESSION, so passing
    // READ_TIP directly returns the function OBJECT rather than calling it —
    // and an unserializable return arrives as undefined, which reads exactly
    // like "the tip was never there" and sails through a truthiness assertion
    // (O-Bass, batch M1). Invoked explicitly instead.
    const readTip = () => page.evaluate(`(${READ_TIP})()`);
    const BARE_CHILD = {
        '#bloom-fine-toggle': '#bloom-fine-toggle .toggle-label',
        '#lp-filter-toggle':  '#lp-filter-toggle .toggle-label',
        '#hi-fi-toggle':      '#hi-fi-toggle .toggle-label',
    };
    const hoverSelFor = (sel) => BARE_CHILD[sel] || sel;

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

    // ── [1d] minimum hover area, measured in each anchor's own state ────────
    // M2 finding 4: an anchor can resolve, satisfy check-i18n and be
    // unopenable — a 1x1 hidden input, or a cell whose every child is
    // pointer-events:none.
    console.log('\n-- [1d] every anchor has a hoverable box in the state it lives in');
    let smallest = { key: null, area: Infinity };
    for (const [sel, key] of TIP_BINDINGS) {
        await ensureStateFor(sel);
        const box = await page.evaluate((s) => {
            const el = document.querySelector(s);
            if (!el) return null;
            const a = el.closest('[data-tip]') || el;
            const r = a.getBoundingClientRect();
            const cs = getComputedStyle(a);
            return { w: r.width, h: r.height, pe: cs.pointerEvents };
        }, sel);
        const area = box ? box.w * box.h : 0;
        if (area < smallest.area) smallest = { key, area };
        check(box !== null && area >= MIN_HOVER_AREA && box.pe !== 'none',
            `[1d] ${key}: anchor box ${box ? `${box.w.toFixed(1)} x ${box.h.toFixed(1)} = `
                + `${Math.round(area)} px2, pointer-events ${box.pe}` : 'MISSING'} `
            + `(floor ${MIN_HOVER_AREA} px2)`);
    }
    console.log(`   smallest hover target: ${smallest.key} at ${Math.round(smallest.area)} px2`);

    // ── [1e] the Bloom gating, pinned so it cannot silently vanish ──────────
    // Both directions of a REAL default state, driven through the toggle rather
    // than by stripping a class.
    console.log('\n-- [1e] the Bloom fine-control gating is a real state, driven by the toggle');
    await setPopover(false);
    await setTab('instrument');
    await setFine(false);
    const restGate = await page.evaluate(() => {
        const r = document.getElementById('bloom-fine-content').getBoundingClientRect();
        const main = document.querySelector('.slider[data-param="bloomSpeed"]').closest('.param-control');
        return { fineH: r.height, mainPe: getComputedStyle(main).pointerEvents };
    });
    check(restGate.fineH === 0 && restGate.mainPe !== 'none',
        `[1e] AT REST: #bloom-fine-content is ${restGate.fineH}px tall (display:none) and the main `
        + `Bloom cells are hoverable (pointer-events ${restGate.mainPe}). The six fine anchors are `
        + `swept only after the toggle is clicked`);
    await setFine(true);
    const onGate = await page.evaluate(() => {
        const r = document.getElementById('bloom-fine-content').getBoundingClientRect();
        const main = document.querySelector('.slider[data-param="bloomSpeed"]').closest('.param-control');
        return { fineH: r.height, mainPe: getComputedStyle(main).pointerEvents };
    });
    check(onGate.fineH > 0 && onGate.mainPe === 'none',
        `[1e] WITH FINE ON: the six fine cells are ${onGate.fineH.toFixed(1)}px of real box and the `
        + `two main Bloom cells become pointer-events:${onGate.mainPe} — so their tips are `
        + `unreachable in THAT state and are swept in the default one instead`);
    await setFine(false);

    // ── the sweep ───────────────────────────────────────────────────────────
    const sweep = async (lang) => {
        console.log(`\n-- ${lang.toUpperCase()}: hover every anchor, byte-compare, measure the rect`);
        const seen = [];
        let tallest = { key: null, h: 0 };
        let flipped = 0, clampedX = 0, clampedY = 0;
        for (const [sel, key] of TIP_BINDINGS) {
            const entry = (I18N[key] || {})[lang];
            if (!entry) { check(false, `[2] ${key} has an ${lang} entry`); continue; }

            await ensureStateFor(sel);
            const st = await hoverAndRead(hoverSelFor(sel));
            seen.push(key);

            check(st !== null && st.visible,
                `[2][${lang}] ${key}: hovering ${hoverSelFor(sel)} SHOWS the tip `
                + `(opacity ${st ? st.opacity : 'n/a'})`);
            if (!st || !st.visible) continue;
            if (st.rect.h > tallest.h) tallest = { key, h: st.rect.h };
            if (st.rect.x <= MARGIN + 0.01) ++clampedX;
            if (st.rect.y <= MARGIN + 0.01) ++clampedY;

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
            + `in a ${SHIP_H}px frame; ${clampedX} sat on the x floor, ${clampedY} on the y floor`);
        await setPopover(false);
        await setTab('instrument');
        await park();
        return tallest;
    };

    const tallestEn = await sweep('en');

    // [4b] — the clamp arithmetic, stated rather than assumed. Recorded because
    // batch M2 found eleven ports crediting a re-clamp line that cannot fire,
    // and the same trap in reverse would be claiming a clamp this frame never
    // exercises. With MAX_W 260 and MARGIN 8 on an 800px frame, an x-flip needs
    // cursorX > 518 and the x-floor needs cursorX < 282 — disjoint, so the
    // x-floor is unreachable. On y, a flip needs cursorY > 584 - h and the
    // y-floor needs cursorY < h + 20, which meet only when h > 282. The tallest
    // real tip measured above is nowhere near that. NC-1b drives the floor in
    // the one state where it CAN fire.
    const Y_FLOOR_MIN_H = MAX_W + 22;   // h > MAX_W + 22 — see the comment above
    console.log(`\n-- [4b] clamp reachability on this frame: the tallest real EN tip is `
        + `${tallestEn.h.toFixed(1)}px; the y floor cannot fire below ${Y_FLOOR_MIN_H}px and the `
        + `x floor cannot fire at all. So the sweep passing says nothing about the clamp — `
        + `NC-1b is where it is actually driven.`);

    // ── NC-3. the child-boundary rule ───────────────────────────────────────
    // pointerout fires at every internal boundary. Without the
    // anchorOf(relatedTarget) === active guard the tip flickers off and on as
    // the pointer crosses from the slider track to the caption inside the same
    // .param-control. Driven on damping, the first cell on the page.
    console.log('\n-- NC-3: moving between two children of the SAME anchor must not hide the tip');
    const dampSel = '.slider[data-param="damping"]';
    const beforeMove = await hoverAndRead(dampSel);
    await page.hover('[data-i18n="label.damping"]', { force: true });
    await page.waitForTimeout(200);
    const afterMove = await readTip();
    check(beforeMove.visible && afterMove.visible
          && afterMove.title === beforeMove.title && afterMove.title === I18N['tip.damping'].en.t,
        `[NC-3] the tip survives the slider -> caption boundary inside .param-control `
        + `— still "${afterMove.title}"`);
    await park();

    // ── NC-4. the drag guard, BOTH directions ───────────────────────────────
    // The HOLD: press on one slider, drag across its neighbour, and no tip may
    // open. The RELEASE: after mouseup, that same neighbour must open normally.
    // Asserting only the hold passes for a permanent off switch.
    console.log('\n-- NC-4: the drag guard holds during a drag AND releases after it');
    await park();
    const boxes = await page.evaluate(() => {
        const b = (s) => {
            const r = document.querySelector(s).getBoundingClientRect();
            return { x: r.x + r.width / 2, y: r.y + r.height / 2 };
        };
        return { a: b('.slider[data-param="damping"]'),
                 n: b('.slider[data-param="overtoneBrightness"]') };
    });
    await page.mouse.move(boxes.a.x, boxes.a.y);
    await page.mouse.down();
    await page.mouse.move(boxes.n.x, boxes.n.y, { steps: 12 });
    await page.waitForTimeout(250);
    const duringDrag = await readTip();
    check(!duringDrag.visible,
        `[NC-4a] dragging from Damping across Overtone Brightness opens NO tip — the drag guard `
        + `holds` + (duringDrag.visible ? ` (it opened "${duringDrag.title}")` : ''));
    await page.mouse.up();
    await page.waitForTimeout(150);
    await park();
    const afterRelease = await hoverAndRead('.slider[data-param="overtoneBrightness"]');
    check(afterRelease.visible && afterRelease.title === I18N['tip.overtoneBrightness'].en.t,
        `[NC-4b] after mouseup the same neighbour opens normally — "${afterRelease.title}". `
        + `Without this half the guard could be a permanent off switch and NC-4a would still pass`);
    await park();

    // ── NC-5. the double-click value editor ─────────────────────────────────
    // Its <input> is created INSIDE the .knob-container anchor and focused
    // programmatically. Note 10 in the renderer.
    console.log('\n-- NC-5: the double-click value editor does not leave a tip over itself');
    await setTab('effects');
    await page.dblclick('#chorusRateValue', { force: true });
    await page.waitForTimeout(300);
    const editing = await page.evaluate(() => {
        const t = document.getElementById('tooltip');
        const cs = getComputedStyle(t);
        const inp = document.querySelector('#chorusRateValue input');
        return {
            shown: cs.visibility !== 'hidden' && parseFloat(cs.opacity) > 0.01,
            hasInput: !!inp,
            focused: !!inp && document.activeElement === inp,
        };
    });
    check(editing.hasInput && editing.focused,
        `[NC-5] the double-click editor opened and took focus — the state the suppression is for `
        + `(input ${editing.hasInput}, focused ${editing.focused})`);
    check(!editing.shown,
        `[NC-5a] no tip is parked over the editor the user is typing into. The FIRST draft of the
        guard sat in focusin and FAILED here: replacing the readout's text node with an <input>
        mutates the DOM under a stationary cursor and Chromium dispatches a fresh pointerover,
        which the focus latch has nothing to say about`.replace(/\s+/g, ' '));
    // The RELEASE half. A guard that never lets go is a permanent off switch
    // and NC-5a would still pass — the same lesson as NC-4b.
    await page.keyboard.press('Escape');
    await page.waitForTimeout(200);
    await park();
    const afterEdit = await hoverAndRead('#chorusRateKnob');
    check(afterEdit.visible && afterEdit.title === I18N['tip.chorusRate'].en.t,
        `[NC-5b] once the editor is dismissed the knob's tip opens again — "${afterEdit.title}". `
        + `Without this half the editor guard could be permanent and NC-5a would still pass`);
    await setTab('instrument');
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
    const backSt = await hoverAndRead(dampSel);
    check(backSt.visible && backSt.title === I18N['tip.damping'].en.t
          && backSt.body === I18N['tip.damping'].en.b,
        `[5] switching back to English restores the English tip byte-for-byte `
        + `— "${backSt.title}"`);
    await park();

    // ── NC-2. the focus latch, both halves ──────────────────────────────────
    console.log('\n-- NC-2: the focus latch (a click must not pin a tip; a key must still open one)');
    await page.mouse.move(1, 1);
    await page.waitForTimeout(150);
    // BLUR FIRST, and this line is the whole reason the assertion below can FAIL
    // at all. The sweep above leaves focus wherever its last click put it, and
    // clicking an ALREADY-FOCUSED element fires no focusin — so without this the
    // check reports "no tip after a click" for a page with no latch whatsoever.
    // Measured on the M1 pilots: 125/125 green with the latch deleted, and
    // 186/186 on O-Tremolo with the latch AND this line both deleted. The 2x2 is
    // run by hand at each release; the v4.3.0 result is in the commit message.
    await page.evaluate(() => { if (document.activeElement) document.activeElement.blur(); });
    await page.waitForTimeout(100);
    await page.click('#gear-btn');
    popoverOpen = true;
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
    popoverOpen = false;
    await page.evaluate(() => { if (document.activeElement) document.activeElement.blur(); });
    await park();
    let kbHit = null;
    for (let i = 1; i <= 25; ++i) {
        await page.keyboard.press('Tab');
        // AWAITED, not slept past. The surface fades in over 0.12s and `visible`
        // demands opacity > 0.99, so a fixed 80 ms sleep reads part-way and
        // records "no tip" on a tab that DID open one — O-Comp measured exactly
        // that, reported "none in 20 tabs" for a path that works, and the
        // obvious response to that reading is to delete the latch.
        await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 400 })
                  .catch(() => {});
        const r = await readTip();
        const on = await page.evaluate(() => {
            const a = document.activeElement;
            return a ? (a.id || a.className || a.tagName.toLowerCase()) : null;
        });
        if (r && r.visible && r.title) { kbHit = { press: i, on, ...r }; break; }
    }
    check(kbHit !== null,
        `[NC-2b] a KEYBOARD tab still OPENS a tip — the accessibility half the latch must not `
        + `kill` + (kbHit ? ` (tab #${kbHit.press} on "${kbHit.on}", title "${kbHit.title}")`
                          : ' — none in 25 tabs'));
    check(kbHit !== null && inFrame(kbHit.rect),
        `[NC-2b] the focus-placed tip is inside the frame`
        + (kbHit ? ` — ${edges(kbHit.rect)}` : ''));
    await page.keyboard.press('Escape');
    await park();

    // ── NC-1. plant an over-long body; assertion 4 must report it ───────────
    // A namespaced per-run directory, never a bare filename at a shared temp
    // root: several executors run in this scratchpad at once and a bare
    // i18n.orig.js is not yours. Restored by COPY, never with
    // `git checkout -- <file>`, which would take any uncommitted work in the
    // same file with it (O-GrainScatter lost a whole edit that way).
    //
    // THE PLANT IS SEARCHED FOR. 800 x 600 is roomy: with a 260px cap and
    // ~11.5px/1.45 type there is well over 500px of vertical room to absorb a
    // plant, and a habitual 40x repeat is not obviously enough. Rather than
    // compute a number and trust it, the loop doubles the repeat count until
    // the rendered height actually EXCEEDS the frame, and the height is
    // asserted before the overflow claim is believed. A plant that fits is
    // indistinguishable from a gate that cannot see.
    console.log('\n-- NC-1: search for a plant that overflows, then confirm assertion 4 reports it');
    const nc = fs.mkdtempSync(path.join(os.tmpdir(), 'obells-tip-nc-'));
    const servedI18n = path.join(built.root, 'js', 'i18n.js');
    const backup = path.join(nc, 'i18n.served.orig.js');
    fs.copyFileSync(servedI18n, backup);
    const origSrc = fs.readFileSync(backup, 'utf8');
    let plantHit = null;
    try {
        // No ASCII apostrophe anywhere in the plant: the bodies in this file are
        // SINGLE-quoted, so one would close the string and break the module
        // rather than overflow the frame — a plant that fails for the wrong
        // reason is a control that proves nothing.
        const UNIT = 'Une phrase de controle negatif, deliberement beaucoup trop longue pour '
                   + 'tenir dans le cadre, repetee afin de faire deborder la surface par le bas. ';
        for (const reps of [24, 48, 96, 192]) {
            const planted = origSrc.replace(
                /('tip\.damping':[\s\S]*?en: \{ t: 'Damping',\s*\n\s*b: )'[^']*'/,
                `$1'${UNIT.repeat(reps)}'`);
            if (planted === origSrc) {
                check(false, `[NC-1] the plant regex matched nothing — a no-op replace would make `
                    + `this whole control vacuous`);
                break;
            }
            fs.writeFileSync(servedI18n, planted);
            await load();
            curTab = 'instrument'; fineOn = false; popoverOpen = false;
            const bad = await hoverAndRead(dampSel);
            console.log(`   ${reps}x (${UNIT.length * reps} chars) -> tip `
                + `${bad.visible ? `${bad.rect.w.toFixed(1)} x ${bad.rect.h.toFixed(1)}` : 'NOT SHOWN'}`);
            if (bad.visible && bad.rect.h > SHIP_H) { plantHit = { reps, ...bad }; break; }
        }
        check(plantHit !== null,
            `[NC-1] the search FOUND a plant bigger than the frame`
            + (plantHit ? ` — ${plantHit.reps}x, ${plantHit.rect.h.toFixed(1)}px tall against `
                          + `${SHIP_H}px` : ' — none of 24/48/96/192x overflowed, which means '
                          + 'this control cannot fail and proves nothing'));
        if (plantHit) {
            check(!inFrame(plantHit.rect),
                `[NC-1] assertion 4 REPORTS the overflow — rect ${plantHit.rect.w.toFixed(1)} x `
                + `${plantHit.rect.h.toFixed(1)} at ${plantHit.rect.x.toFixed(1)},`
                + `${plantHit.rect.y.toFixed(1)} leaves the ${SHIP_W} x ${SHIP_H} frame `
                + `(${edges(plantHit.rect)}). If this PASSES as in-frame, assertion 4 is `
                + `decoration and every green above means nothing`);
            // NC-1b — THE CLAMP FLOOR, driven. See [4b]: this is the only state
            // on this frame in which the Math.max floor can fire, so it is the
            // only place the line can be honestly claimed.
            check(Math.abs(plantHit.rect.y - MARGIN) < 0.51,
                `[NC-1b] the y CLAMP FLOOR fired — the oversized tip sits at top `
                + `${plantHit.rect.y.toFixed(2)}px, which is the ${MARGIN}px margin, not the `
                + `cursor-relative +16 the natural placement would give. Deleting the Math.max `
                + `puts this tip off the top of the page; the post-flip re-clamp line above it `
                + `cannot fire at any cursor inside the viewport (M2 finding 1)`);
        }
    } finally {
        fs.copyFileSync(backup, servedI18n);
    }

    await load();
    curTab = 'instrument'; fineOn = false; popoverOpen = false;
    const restored = await hoverAndRead(dampSel);
    check(fs.readFileSync(servedI18n, 'utf8') === origSrc,
        `[NC-1] the served copy is byte-identical to the pre-plant snapshot again`);
    check(restored.visible && restored.body === I18N['tip.damping'].en.b && inFrame(restored.rect),
        `[NC-1] restored — tip.damping is back inside the frame (${edges(restored.rect)})`);
    fs.rmSync(nc, { recursive: true, force: true });

    // ── housekeeping ────────────────────────────────────────────────────────
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
