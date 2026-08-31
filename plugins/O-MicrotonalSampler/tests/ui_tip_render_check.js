/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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
    O-MicrotonalSampler — hover-help RENDER verification at the shipping
    viewport (v1.25.0, Stage M batch M2).

    WHY THIS FILE EXISTS. No gate in this repo can see a rendered tooltip.
    check-i18n reads the table statically and is satisfied by TIP_BINDINGS being
    non-empty and its keys resolving. check-ui-labels has no tooltip awareness
    whatsoever — measured on this plugin: its output is byte-identical before and
    after the whole of v1.25.0. boot-all-uis counts aria-label and title and
    never data-tip. So a plugin can author copy, bind it, and pass all three
    green while showing nothing on screen at all — which is exactly the state
    O-MicrotonalSampler was in at v1.24.0: canon v2's applyI18n() writes
    data-tip-title and data-tip onto the anchors and stops there, and this page
    had no #tooltip element, no #tooltip rule and no hover handler to read them.

    This file is the assertion those three cannot make. It drives the REAL page
    — the same index.html, the same css/sampler-shell.css, the same
    js/sampler-app.js, only js/juce/index.js swapped for the shared stub and the
    natives seeded from tests/ui-stub/generic-overrides.json — in a browser
    pinned to the exact shipping frame, and measures the rectangle that paints.

    NOT a port of the three committed ui_tooltip_clamp_check.js gates
    (O-Tapestop, O-Bitrot, O-ReverseDelay). Those are ~40 KB each and describe
    the OTHER renderer family: measure-then-pin placement with an above/below
    flip and an arrow. This page ports O-simpleFM's delegated cursor-following
    renderer, which has neither, so those assertions would not describe it.

    WHAT IS ASSERTED, per anchor, in English and again in French:

      1. every TIP_BINDINGS selector resolves, its wrapper walk lands somewhere,
         and applyI18n() wrote both attributes onto it. A binding that finds
         nothing is a FAIL, not a warning, because applyI18n() only
         console.warns about it. AND every binding lands on a DISTINCT node:
         applyI18n writes onto whatever a selector resolves to, so two bindings
         on one node mean the second silently overwrites the first while
         check-i18n cheerfully reports two bound tips;
      2. hovering a DESCENDANT of the anchor makes the surface VISIBLE with
         non-empty text. Hovering a descendant rather than the anchor's own box
         is the point: it is what the pointer actually lands on, and it is the
         only thing that exercises the delegated closest('[data-tip]') walk this
         renderer is built out of;
      3. the rendered title and body are BYTE-EQUAL to the table entry. Not
         "contains" — a .tip-title that silently kept the previous anchor's text
         passes a contains check and fails this one;
      4. the tip rectangle is fully inside the 900 x 640 frame on all four
         edges, and no wider than the #tooltip max-width cap.

    Then French (2-4 again against the fr entries), then back to English to prove
    the switch is reversible rather than one-way.

    THREE NEGATIVE CONTROLS, because "all pass" is worthless without them:

      NC-1  an over-long body is planted in the SERVED copy of js/i18n.js and
            the page reloaded; assertion 4 must REPORT the overflow. The plant
            is SIZED AGAINST THIS FRAME, not by habit: O-Tremolo's 40x plant fit
            inside a 400px frame and reported nothing, which is a control that
            cannot fail. At 12px/1.38 in a 240px content box this page fits
            roughly 40 characters per 16.6px line, so 640px of frame is about
            37 lines — the plant below is ~4200 characters, ~105 lines, ~1740px,
            and the run PRINTS the measured height so a future reader can see it
            actually overflowed rather than trusting the arithmetic.
            Restored from a namespaced per-run copy, never with
            `git checkout -- <file>`, which would take any uncommitted work in
            the same file with it (O-GrainScatter lost a whole edit that way).
      NC-2  the focus latch, BOTH halves, separately: a pointer click must leave
            no tip parked on screen, AND a real tab-ring walk must still open
            one. Asserting only the first lets the feature decay into "focus
            never shows a tip", which passes that assertion perfectly while
            silently deleting the keyboard half of hover-help.
            THE CLICK IS PRECEDED BY A BLUR, and that line is load-bearing:
            clicking an ALREADY-FOCUSED element fires no focusin at all, so
            without it the assertion passes for a page with no latch whatsoever
            (the orchestrator measured 125/125 green with the latch deleted, and
            O-Tremolo measured 186/186 with the blur deleted too). The overlap
            with the settings popover is measured in px2 rather than merely
            observed, so a later run has a number to compare.
            A STATIC grep for `lastInputWasPointer` is NOT this control and is
            not treated as one: the declaration, the pointerdown write and the
            keydown clear all survive deleting `if (lastInputWasPointer) return;`
            and every grep still matches (O-Comp). The grep below is a presence
            note; NC-2a is the discriminator.
      NC-3  the pointerout child-boundary rule: moving between two children of
            the SAME anchor must not flicker the tip off. Every knob cell here
            holds an SVG, a caption, a readout and a hidden range input.

    THE KEYBOARD PROBE AWAITS THE TRANSITION rather than sleeping past it. The
    surface fades in over 0.12s and `visible` demands opacity > 0.99, so a fixed
    80ms sleep reads ~0.66 and records "no tip" on a tab that DID open one —
    and the obvious response to that false failure is to delete the latch, which
    makes the NC-2a defect green again by way of a probe artefact (O-Comp,
    O-SimpleReverb; pattern_quiesce_before_stimulus_in_async_ui_gates).

    Usage:  node plugins/O-MicrotonalSampler/tests/ui_tip_render_check.js
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

const PLUGIN   = 'O-MicrotonalSampler';
// Resources/ui, NOT Source/ui/public. Source/ui/public exists in this plugin
// but holds only the shared drop-streaming module copy and is not embedded —
// CMakeLists.txt says so in as many words. Cross-checked against serve-ui's own
// resolver below rather than trusted twice.
const UI_ROOT  = path.join(pluginRoot, 'Resources', 'ui');
const htmlPath = path.join(UI_ROOT, 'index.html');
const cssPath  = path.join(UI_ROOT, 'css', 'sampler-shell.css');
const appPath  = path.join(UI_ROOT, 'js', 'sampler-app.js');
const i18nPath = path.join(UI_ROOT, 'js', 'i18n.js');

// Mirrored from PluginEditor.cpp, css/sampler-shell.css and js/sampler-app.js,
// and CROSS-CHECKED against all three below. A fixture that mirrors a constant
// without checking it starts describing the release before it and keeps passing
// (pattern_test_fixture_mirrors_drift_silently).
const SHIP_W = 900;
const SHIP_H = 640;
const MARGIN = 8;               // setupTooltips()'s clamp margin
const DOCUMENTED_MAX_W = 260;   // #tooltip max-width

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}

// ── the table, read the way check-i18n reads it ─────────────────────────────
// i18n.js is an ES module outside any package.json, so node can neither
// require() nor import() it synchronously here. Evaluated in a vm sandbox with
// the export keywords stripped. NEVER retyped into this file: a literal list of
// anchors in a test fails for the wrong reason the first time a control gains a
// tip.
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
        // The cursor the placement was computed from. position() offsets by
        // (+14, +16) and FLIPS to the far side when that overflows, so
        // tip.y < cursor.y is the vertical flip and tip.x < cursor.x is the
        // horizontal one. Recorded rather than inferred from the anchor's box,
        // which is not where the pointer is.
        cursor: window.__tipProbePointer || null,
    };
}`;

// Where the pointer actually lands, per anchor. For the nine knob cells and the
// dynamics toggle the TIP_BINDINGS selector IS the anchor, so a descendant is
// named here to exercise the closest('[data-tip]') walk; for the six
// wrapper-bound controls the selector is already a descendant of its anchor and
// is used as-is; the two chrome anchors have no meaningful child.
const hoverSelFor = (sel) => {
    if (/^\[data-knob-id=/.test(sel))    return `${sel} .ouaricon-knob-visual`;
    if (sel === '.dynamics-mode-control') return '.dynamics-mode-control .seg-toggle';
    if (sel === '#technique-tabs')        return '#technique-tabs .tech-tab';
    return sel;
};

// Anchors that are not reachable at rest. The trigger panel is a <details> that
// renderTriggerPanel() only un-hides once technique_count > 1 (the stub seeds
// 8) and which still has to be OPENED before its children have boxes; the
// language selector lives inside a popover that ships hidden.
const NEEDS_TRIGGER_PANEL = new Set(['#cc-trigger-enabled', '#cc-trigger-number',
                                     '#pc-trigger-enabled']);
const NEEDS_POPOVER       = new Set(['#lang-select']);

(async () => {
    console.log(`== ${PLUGIN} ui_tip_render_check ==`);
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}\n`);

    const html    = fs.readFileSync(htmlPath, 'utf8');
    const css     = fs.readFileSync(cssPath, 'utf8');
    const appSrc  = fs.readFileSync(appPath, 'utf8');
    const i18nSrc = fs.readFileSync(i18nPath, 'utf8');

    // ── 0. the mirrored constants, guarded ──────────────────────────────────
    const size = S.readEditorSize(PLUGIN);
    check(size && size.w === SHIP_W && size.h === SHIP_H,
        `editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below `
        + `(PluginEditor.cpp says ${size ? `${size.w} x ${size.h}` : 'NOTHING'})`);

    // Parsed from the #tooltip RULE specifically. This stylesheet also carries a
    // width on .settings-popover and min-widths on four pinned captions, so a
    // loose scan for the first max-width would silently measure one of those.
    const tipRule  = css.match(/#tooltip\s*\{[\s\S]*?\}/);
    const capMatch = tipRule && tipRule[0].match(/max-width:\s*(\d+(?:\.\d+)?)px/);
    const MAX_W    = capMatch ? parseFloat(capMatch[1]) : NaN;
    check(Number.isFinite(MAX_W) && MAX_W === DOCUMENTED_MAX_W,
        `#tooltip max-width parsed from css/sampler-shell.css is the documented `
        + `${DOCUMENTED_MAX_W}px — got ${capMatch ? capMatch[1] + 'px' : 'NOTHING'}. French `
        + `wraps INSIDE this cap, so moving it changes every tip height and therefore every `
        + `clamp decision below`);
    check(new RegExp(`MARGIN\\s*=\\s*${MARGIN}\\b`).test(appSrc),
        `setupTooltips()'s clamp MARGIN is ${MARGIN} (this file mirrors it)`);
    check(/position:\s*fixed/.test(tipRule ? tipRule[0] : '')
       && /visibility:\s*hidden/.test(tipRule ? tipRule[0] : '')
       && /pointer-events:\s*none/.test(tipRule ? tipRule[0] : ''),
        '#tooltip is position:fixed + visibility:hidden + pointer-events:none — the three '
        + 'properties that keep an unshown surface out of check-ui-labels\' sweep and stop a '
        + 'shown one stealing its own hover');
    check(/id="tooltip"/.test(html), 'index.html carries the #tooltip surface');
    check(/lastInputWasPointer/.test(appSrc),
        'the focus latch identifier is present in the shipped controller — a PRESENCE note '
        + 'only: this same grep stays green with the guard clause deleted, so NC-2a below is '
        + 'what proves the latch WORKS');
    check(!/\btitle\s*=\s*["']/.test(html.replace(/<title>[\s\S]*?<\/title>/, '')),
        'no native title= attribute reintroduced by the renderer (contract §4)');
    check(/initI18n\(\);\s*setupTooltips\(\);/.test(appSrc),
        'setupTooltips() is called AFTER initI18n() and inside the same try/catch — no anchor '
        + 'carries data-tip until the sweep has painted the attributes');

    // ── the table ───────────────────────────────────────────────────────────
    const { I18N, TIP_BINDINGS, LANGUAGES } = loadTable(i18nSrc);
    check(Array.isArray(TIP_BINDINGS) && TIP_BINDINGS.length > 0,
        `TIP_BINDINGS parsed from js/i18n.js — ${TIP_BINDINGS.length} row(s)`);
    check(LANGUAGES.join(',') === 'en,fr', `LANGUAGES is en,fr — got ${LANGUAGES.join(',')}`);

    // The 51 body-less entries are NOT hover-help and must not become it. This
    // is the assertion that catches an accidental body on a toast the day
    // somebody pastes prose into the wrong row.
    const bodied = Object.keys(I18N).filter(
        (k) => (I18N[k].en && I18N[k].en.b) || (I18N[k].fr && I18N[k].fr.b));
    check(bodied.every((k) => k.startsWith('tip.')),
        `every bodied I18N entry is a tip.* key — the ${Object.keys(I18N).length - bodied.length} `
        + `toast / dialog / composed-aria entries still carry b: '' on both languages`
        + (bodied.every((k) => k.startsWith('tip.')) ? '' :
           ` (offenders: ${bodied.filter((k) => !k.startsWith('tip.')).join(', ')})`));
    const boundKeys = new Set(TIP_BINDINGS.map((b) => b[1]));
    check(bodied.every((k) => boundKeys.has(k)),
        `every bodied entry is bound — an orphan fails check-i18n assertion 2 and would be `
        + `invisible copy`);

    // The decimal separator, settled 2026-08-30: a body is PROSE and takes the
    // French comma. The readout keeps its point (D-03 exempts the readout NODE).
    // O-SimpleReverb shipped a point in M1 without flagging it at all.
    const frPoints = [];
    for (const k of Object.keys(I18N)) {
        const f = I18N[k].fr;
        if (!f) continue;
        for (const s of [f.t || '', f.b || '']) if (/\d+\.\d+/.test(s)) frPoints.push(k);
    }
    check(frPoints.length === 0,
        `no French entry spells a decimal with a POINT`
        + (frPoints.length ? ` — ${[...new Set(frPoints)].join(', ')}` : ''));

    const pw = S.resolvePlaywright();
    if (pw == null) {
        console.log('\n  SKIP: playwright not resolvable. Install with');
        console.log('        npx playwright install chromium');
        console.log('  The hover-help render and its edge clamp are NOT verified without it.');
        process.exit(77);
    }

    const built = S.buildRoot(PLUGIN);
    check(built.uiRootLabel === 'Resources/ui',
        `the served root really is Resources/ui (serve-ui resolved "${built.uiRootLabel}" `
        + `from ${built.uiRootFrom})`);

    const misses = [];
    const srv = await S.serve(built.root, (m) => misses.push(m));
    const browser = await pw.chromium.launch();
    // `viewport`, NOT `viewportSize` — the latter is the getter's name and is
    // silently IGNORED as a launch option, leaving Chromium's 1280x720 default.
    // At 1280 x 720 every anchor on this page has room below and to the right,
    // the clamp never engages, and every assertion below would pass while the
    // real 900 x 640 frame overflowed
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

    // The trigger panel and the technique bar are only populated once the stub's
    // getTechniqueState / getTriggerState pulls have landed, and the control
    // strip only exists after renderControlStrip(). Opening the <details> is
    // therefore part of REACHING the page, not part of a state sweep.
    // Records the live cursor so a placement can be classified as flipped or
    // not. Registered on every load, before any hover.
    await page.addInitScript(() => {
        window.__tipProbePointer = null;
        document.addEventListener('pointermove', (e) => {
            window.__tipProbePointer = { x: e.clientX, y: e.clientY };
        }, true);
        document.addEventListener('pointerover', (e) => {
            window.__tipProbePointer = { x: e.clientX, y: e.clientY };
        }, true);
    });

    const openPanels = async () => {
        await page.waitForSelector('#technique-tabs .tech-tab', { timeout: 4000 });
        await page.evaluate(() => {
            const d = document.getElementById('trigger-panel');
            if (d) { d.hidden = false; d.open = true; }
        });
        await page.waitForSelector('#cc-trigger-number', { state: 'visible', timeout: 4000 });
    };

    await page.goto(url, { waitUntil: 'networkidle' });
    await openPanels();

    const vp = page.viewportSize();
    check(vp.width === SHIP_W && vp.height === SHIP_H,
        `the browser really is ${SHIP_W} x ${SHIP_H} — got ${vp.width} x ${vp.height}`);

    // Non-vacuity: js/sampler-app.js must have RUN. A TDZ throw out of module
    // evaluation leaves the HTML looking correct and every control dead
    // (pattern_module_toplevel_init_tdz). Two independent witnesses, both empty
    // in the authored markup and written only by the module: the nine knob cells
    // renderControlStrip() injects into an empty <footer>, and the technique tab
    // strip renderTechniqueBar() injects into an empty <div>.
    const alive = await page.evaluate(() => ({
        knobs: document.querySelectorAll('#control-strip .ouaricon-knob').length,
        tabs:  document.querySelectorAll('#technique-tabs .tech-tab').length,
    }));
    check(alive.knobs === 9,
        `js/sampler-app.js ran — renderControlStrip() built ${alive.knobs} knob cells into a `
        + `<footer> the markup leaves empty (expected 9)`);
    check(alive.tabs > 1,
        `js/sampler-app.js ran — renderTechniqueBar() built ${alive.tabs} technique tab(s) into `
        + `a <div> the markup leaves empty`);

    // ── 1. every binding resolves, selector AND wrapper walk, DISTINCT nodes ─
    const resolution = await page.evaluate((bindings) => {
        const nodes = [];
        return bindings.map(([sel, key, wrapper]) => {
            const el = document.querySelector(sel);
            if (!el) return { key, sel, ok: false, why: 'selector matched nothing' };
            const target = wrapper ? (el.closest(wrapper) || null) : el;
            if (!target) return { key, sel, ok: false, why: `closest(${wrapper}) matched nothing` };
            const dup = nodes.indexOf(target);
            nodes.push(target);
            return {
                key, sel, ok: true,
                dupOf: dup >= 0 ? bindings[dup][1] : null,
                tag: target.tagName.toLowerCase()
                     + (target.id ? '#' + target.id
                        : (target.className ? '.' + String(target.className).split(' ')[0] : '')),
                hasTip: target.hasAttribute('data-tip') && target.hasAttribute('data-tip-title'),
            };
        });
    }, TIP_BINDINGS);

    for (const r of resolution) {
        check(r.ok, `[1] binding ${r.key} resolves — ${r.sel}${r.ok ? ` -> ${r.tag}` : ` (${r.why})`}`);
        if (!r.ok) continue;
        check(r.hasTip,
            `[1] applyI18n wrote data-tip-title + data-tip onto ${r.key}'s anchor (${r.tag})`);
        check(r.dupOf === null,
            `[1] ${r.key} lands on a node no other binding claims`
            + (r.dupOf ? ` — COLLIDES with ${r.dupOf} on ${r.tag}; the second setAttribute `
                         + `silently overwrites the first` : ''));
    }
    check(tipWarns.length === 0,
        `[1] applyI18n logged no "tip target not found" warning`
        + (tipWarns.length ? ` — ${tipWarns.length}: ${tipWarns[0]}` : ''));

    // ── the hover driver ────────────────────────────────────────────────────
    // page.evaluate() given a STRING evaluates it as an EXPRESSION, so passing
    // READ_TIP directly returns the function OBJECT rather than calling it — and
    // an unserializable return arrives as undefined, which reads exactly like
    // "the tip was never there" and sails through a truthiness assertion
    // (O-Bass). Invoked explicitly instead.
    const readTip = () => page.evaluate(`(${READ_TIP})()`);

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
        // mid-transition returns opacity ~0.6 with a rect that is already final,
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

    const sweep = async (lang) => {
        console.log(`\n-- ${lang.toUpperCase()}: hover every anchor, byte-compare, measure the rect`);
        let popoverOpen = false;
        const seen = [];
        let clamped = 0, flippedV = 0, flippedH = 0;
        for (const [sel, key] of TIP_BINDINGS) {
            const entry = (I18N[key] || {})[lang];
            if (!entry) { check(false, `[2] ${key} has an ${lang} entry`); continue; }

            if (NEEDS_TRIGGER_PANEL.has(sel)) {
                await page.evaluate(() => {
                    const d = document.getElementById('trigger-panel');
                    if (d) { d.hidden = false; d.open = true; }
                });
            }
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
            if (st.rect.x <= MARGIN + 0.5 || st.rect.y <= MARGIN + 0.5
                || SHIP_W - st.rect.x - st.rect.w <= MARGIN + 0.5
                || SHIP_H - st.rect.y - st.rect.h <= MARGIN + 0.5) ++clamped;
            if (st.cursor) {
                if (st.rect.y < st.cursor.y) ++flippedV;
                if (st.rect.x < st.cursor.x) ++flippedH;
            }
        }
        // REPORTED, not asserted, and reported as measured rather than as a
        // slogan. A 900 x 640 frame is roomy: unlike O-Chorus at 700 x 125 or
        // O-Bass at 420 x 320, most placements here need neither flip nor rail,
        // and claiming otherwise would be a sentence contradicted by its own
        // number. The clamp path IS exercised — by NC-1, whose plant lands on
        // the 8px top rail — and the flip path by the control-strip knobs, which
        // sit 50px off the bottom edge.
        console.log(`   placements: ${flippedV}/${seen.length} flipped ABOVE the cursor, `
                    + `${flippedH}/${seen.length} flipped LEFT, `
                    + `${clamped}/${seen.length} landed on an ${MARGIN}px rail`);
        check(seen.length === TIP_BINDINGS.length,
            `[2][${lang}] every one of the ${TIP_BINDINGS.length} bound anchors was driven `
            + `— got ${seen.length}`);
        if (popoverOpen) {
            await page.keyboard.press('Escape');       // also clears the focus latch
            await page.waitForTimeout(150);
        }
        return seen;
    };

    await sweep('en');

    // ── NC-3. the child-boundary rule ───────────────────────────────────────
    // pointerout fires at EVERY internal boundary. Without the
    // anchorOf(relatedTarget) === active guard the tip flickers off and on as
    // the pointer crosses from the SVG to the caption inside the same knob cell.
    console.log('\n-- NC-3: moving between two children of the SAME anchor must not hide the tip');
    const knobSel = '[data-knob-id="ctrl-attack"]';
    const beforeMove = await hoverAndRead(`${knobSel} .ouaricon-knob-visual`);
    await page.hover(`${knobSel} .ouaricon-knob-label`, { force: true });
    await page.waitForTimeout(200);
    const afterMove = await readTip();
    check(beforeMove.visible && afterMove.visible
          && afterMove.title === beforeMove.title && afterMove.title === I18N['tip.attack'].en.t,
        `[NC-3] the tip survives the SVG -> caption boundary inside the knob cell `
        + `— still "${afterMove.title}"`);
    await park();

    // ── 5. French, then back ────────────────────────────────────────────────
    // French runs 15-20% longer, wraps to more lines against the max-width cap
    // and grows the tip's HEIGHT, so a tip that fits in English can overflow the
    // bottom of the frame in French. That is why the whole sweep repeats rather
    // than spot-checking one anchor.
    await page.evaluate((l) => window.__setLanguage(l), 'fr');
    await page.waitForTimeout(200);
    const frLang = await page.evaluate(() => document.getElementById('lang-select').value);
    check(frLang === 'fr', `[5] window.__setLanguage('fr') took — selector reads "${frLang}"`);
    await sweep('fr');

    await page.evaluate((l) => window.__setLanguage(l), 'en');
    await page.waitForTimeout(200);
    const backSt = await hoverAndRead('[data-knob-id="ctrl-output-gain"] .ouaricon-knob-visual');
    check(backSt.visible && backSt.title === I18N['tip.outputGain'].en.t
          && backSt.body === I18N['tip.outputGain'].en.b,
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
    // at all. The sweep above leaves focus on #gear-btn (the popover pass clicked
    // it), and clicking an ALREADY-FOCUSED element fires no focusin — so without
    // this the check reports "no tip after a click" for a page with no latch
    // whatsoever. Measured on the two M1 pilots: 125/125 green with the latch
    // deleted; and on O-Tremolo, 186/186 green with this blur deleted too.
    await page.evaluate(() => { if (document.activeElement) document.activeElement.blur(); });
    await page.waitForTimeout(100);
    await page.click('#gear-btn');
    await page.waitForTimeout(350);
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
    await page.waitForTimeout(200);
    await page.evaluate(() => { if (document.activeElement) document.activeElement.blur(); });
    await park();
    let kbHit = null;
    for (let i = 1; i <= 24; ++i) {
        await page.keyboard.press('Tab');
        // AWAITED, not slept past — see the header. An 80ms read inside the
        // 120ms fade reports "no tip" on a tab that DID open one, and the
        // obvious response to that false failure is to delete the latch.
        await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 500 })
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
                          : ' — none in 24 tabs'));
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
    console.log('\n-- NC-1: plant an over-long body and confirm assertion 4 reports the overflow');
    const nc = fs.mkdtempSync(path.join(os.tmpdir(), 'omts-tip-nc-'));
    const servedI18n = path.join(built.root, 'js', 'i18n.js');
    const backup = path.join(nc, 'i18n.served.orig.js');
    fs.copyFileSync(servedI18n, backup);
    const origSrc = fs.readFileSync(backup, 'utf8');
    try {
        // SIZED AGAINST THIS FRAME. ~4200 chars at ~40 chars/line and 16.6px per
        // line is ~1740px against 640px of frame — not a habit-sized 40x plant
        // that fits and reports nothing (O-Tremolo).
        // No straight apostrophe anywhere in the plant: the bodies in this file
        // are SINGLE-quoted, so one would close the string and break the module
        // rather than overflow the frame — a plant that fails for the wrong
        // reason is a control that proves nothing.
        const LONG = ('Une phrase de controle negatif, deliberement beaucoup trop longue pour '
                    + 'tenir dans le cadre de 640 pixels, repetee afin de faire deborder la '
                    + 'surface par le bas et par le haut a la fois. ').repeat(20);
        const planted = origSrc.replace(
            /('tip\.attack':[\s\S]*?en:\s*\{\s*t:\s*'Attack',\s*\n\s*b:\s*)'[^']*'/,
            `$1'${LONG}'`);
        check(planted !== origSrc,
            `[NC-1] the plant actually edited tip.attack's en body — a no-op replace would make `
            + `this control vacuous`);
        fs.writeFileSync(servedI18n, planted);

        await page.goto(url, { waitUntil: 'networkidle' });
        await openPanels();
        const bad = await hoverAndRead(`${knobSel} .ouaricon-knob-visual`);
        check(bad.visible,
            `[NC-1] the planted tip still renders — ${bad.rect.w.toFixed(1)} x `
            + `${bad.rect.h.toFixed(1)}px against a ${SHIP_H}px frame`);
        check(!inFrame(bad.rect),
            `[NC-1] assertion 4 REPORTS the overflow — rect ${bad.rect.w.toFixed(1)} x `
            + `${bad.rect.h.toFixed(1)} at ${bad.rect.x.toFixed(1)},${bad.rect.y.toFixed(1)} `
            + `leaves the ${SHIP_W} x ${SHIP_H} frame (${edges(bad.rect)}). If this PASSES as `
            + `in-frame, assertion 4 is decoration and every green above means nothing`);
    } finally {
        fs.copyFileSync(backup, servedI18n);
    }

    await page.goto(url, { waitUntil: 'networkidle' });
    await openPanels();
    const restored = await hoverAndRead(`${knobSel} .ouaricon-knob-visual`);
    check(fs.readFileSync(servedI18n, 'utf8') === origSrc,
        `[NC-1] the served copy is byte-identical to the pre-plant snapshot again`);
    check(restored.visible && restored.body === I18N['tip.attack'].en.b && inFrame(restored.rect),
        `[NC-1] restored — tip.attack is back inside the frame (${edges(restored.rect)})`);
    fs.rmSync(nc, { recursive: true, force: true });

    // ── housekeeping ────────────────────────────────────────────────────────
    console.log('');
    check(pageErrors.length === 0,
        `no uncaught page error across the whole run`
        + (pageErrors.length ? ` — ${pageErrors.length}: ${pageErrors[0].slice(0, 160)}` : ''));
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
