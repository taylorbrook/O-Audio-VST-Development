/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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
    O-Bowed — hover-help RENDER verification at the shipping viewport (v1.6.2).

    WHY THIS FILE EXISTS. No gate in this repo can see a rendered tooltip.
    check-i18n reads the table statically and is satisfied by TIP_BINDINGS being
    non-empty and its keys resolving. check-ui-labels has no tooltip awareness
    whatsoever. boot-all-uis counts aria-label and title and never data-tip. So
    a plugin can author 30 bodies, bind all 30, and pass all three green while
    showing nothing at all on screen — which is exactly the state O-Bowed was in
    at v1.5.0: canon v2's applyI18n() writes data-tip-title and data-tip onto
    the anchors and stops, and this page had no #tooltip element, no .tooltip
    rule and no hover handler to read them.

    This file is the assertion those three cannot make. It drives the REAL page
    — same index.html, same inline <style>, same single inline module — at the
    exact shipping frame, and measures the rectangle that actually paints.

    NOT a port of the three committed ui_tooltip_clamp_check.js gates
    (O-Tapestop, O-Bitrot, O-ReverseDelay). Those are ~40 KB each and describe
    the OTHER renderer family: measure-then-pin placement with an above/below
    flip and an arrow. O-Bowed ports O-simpleFM's delegated, cursor-following
    renderer, which has neither, so those assertions would not describe it.

    WHAT IS ASSERTED, per anchor, in English and again in French:

      1. every TIP_BINDINGS selector resolves, and the 30 of them land on 30
         DISTINCT nodes. applyI18n() writes onto whatever a selector resolves
         to, so two bindings on one node mean the second silently overwrites the
         first — while check-i18n cheerfully reports two bound tips;
      2. hovering a DESCENDANT of the anchor makes the surface VISIBLE with
         non-empty text. A descendant rather than the anchor itself is the
         point: .knob-control is the 62 px column and what the pointer lands on
         is the 55 px SVG inside it, so hovering the column's own box would
         prove the column carries an attribute and nothing about whether the
         delegated closest('[data-tip]') walk reaches it from a child;
      3. the rendered title and body are BYTE-EQUAL to the table entry. Not
         "contains": a .tip-title that silently kept the previous anchor's text
         passes a contains check and fails this one;
      4. the tip rectangle is fully inside the 900 x 600 frame on all four
         edges, and no wider than the .tooltip max-width cap.

    Then French (2-4 again against the fr entries), then back to English to
    prove the switch is reversible rather than one-way.

    FOUR NEGATIVE CONTROLS, because "all pass" is worthless without them:

      NC-1  an over-long body is planted in the SERVED copy of js/i18n.js and
            the page reloaded; assertion 4 must REPORT the overflow. THE PLANT
            IS SIZED AGAINST THIS FRAME AND ITS HEIGHT IS ASSERTED, not assumed:
            O-Tremolo's 40x plant fit inside a 400 px frame and reported
            nothing, and a plant that fits is indistinguishable from a gate that
            cannot see. Restored from a namespaced per-run copy, never with
            `git checkout --`, which would take any uncommitted work in the same
            file with it (O-GrainScatter lost a whole edit that way).
      NC-2  the focus latch, BOTH halves, separately: a pointer click must leave
            no tip parked on screen, AND a real tab-ring walk must still open
            one. Asserting only the first lets the feature decay into "focus
            never shows a tip", which passes that assertion perfectly while
            silently deleting the keyboard half of hover-help.
            THE CLICK IS PRECEDED BY A BLUR, and that line is what gives the
            assertion its power. Clicking an already-focused element fires no
            focusin at all, so without the blur it passes for a page with NO
            latch whatsoever — the orchestrator measured exactly that on the two
            M1 pilots (125/125 green with the latch deleted), and O-Tremolo
            proved the converse (latch removed AND blur removed: 186/186 green).
            The overlap with the settings popover is measured in px2 rather than
            merely observed, so a later run has a number to compare.
            A STATIC grep for `lastInputWasPointer` is NOT this control: the
            declaration, the write and the clear all survive deleting the guard
            clause. Only the behavioural half discriminates.
      NC-3  the pointerout child-boundary rule: moving between two children of
            the SAME anchor must not flicker the tip off.
      NC-4  the mid-drag guard: this page starts a knob drag on mousedown and
            tracks it on document mousemove, so a drag that crosses into a
            neighbouring .knob-control would otherwise open that neighbour's tip
            mid-gesture. pointerdown alone cannot cover it — pointerover arrives
            after it.

    Usage:  node plugins/O-Bowed/tests/ui_tip_render_check.js
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

const PLUGIN   = 'O-Bowed';
// Resources/ui, not Source/ui/public — one of three in batch M2. Read from
// serve-ui's own resolver below rather than trusted from here, but named so the
// divergence is visible in the file that depends on it.
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
const KNOB_COUNT = 28;          // .knob-control nodes the module builds an SVG into

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
// createTextNode. Reading tip.textContent alone concatenates the two and makes
// assertion 3 unable to tell a swapped title from a swapped body.
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

    // Parsed from the .tooltip RULE specifically. This page also carries pinned
    // widths on .settings-popover, .preset-save-btn and .tuning-btn, so a loose
    // scan for the first width would silently measure one of those.
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
        + 'properties that keep an unshown surface out of the label sweep and stop a shown one '
        + 'stealing its own hover');
    check(/id="tooltip"/.test(html), 'index.html carries the #tooltip surface');
    check(/lastInputWasPointer/.test(html),
        'the focus latch is present in the shipped page — NOTE: this is a SPELLING check only. '
        + 'Deleting the guard clause leaves the declaration, the write and the clear in place '
        + 'and this line still passes. NC-2 below is what proves it WORKS');
    check(!/\btitle\s*=\s*["']/.test(html.replace(/<title>[\s\S]*?<\/title>/, '')),
        'no native title= attribute reintroduced by the renderer (contract §4)');

    // ── the table ───────────────────────────────────────────────────────────
    const { I18N, TIP_BINDINGS, LANGUAGES } = loadTable(i18nSrc);
    check(Array.isArray(TIP_BINDINGS) && TIP_BINDINGS.length === 30,
        `TIP_BINDINGS parsed from js/i18n.js — ${TIP_BINDINGS.length} anchor(s), expected 30 `
        + `(${KNOB_COUNT} parameters with a control + #gear-btn + #lang-select)`);
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
        `the served root really is Resources/ui (serve-ui resolved "${built.uiRootLabel}" `
        + `from ${built.uiRootFrom})`);

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
    await page.goto(url, { waitUntil: 'networkidle' });

    const vp = page.viewportSize();
    check(vp.width === SHIP_W && vp.height === SHIP_H,
        `the browser really is ${SHIP_W} x ${SHIP_H} — got ${vp.width} x ${vp.height}`);

    // Non-vacuity: the module must have RUN. A TDZ throw out of module
    // evaluation leaves the HTML looking correct and every control dead
    // (pattern_module_toplevel_init_tdz), and on this plugin the ENTIRE UI is
    // one inline module — 28 knobs, three visualisation canvases, the preset
    // browser and the i18n block would all go together. The witness is empty in
    // the authored markup and written only by the module.
    const alive = await page.evaluate(() => ({
        svgs: document.querySelectorAll('.knob-wrapper > svg.knob-svg').length,
        anchors: document.querySelectorAll('[data-tip]').length,
    }));
    check(alive.svgs === KNOB_COUNT,
        `the inline module ran — createKnobSVG built ${alive.svgs} knob SVGs into markup that `
        + `authors none (expected ${KNOB_COUNT})`);
    check(alive.anchors === TIP_BINDINGS.length,
        `applyI18n painted ${alive.anchors} [data-tip] anchors (expected ${TIP_BINDINGS.length})`);

    // ── the conditionally-hidden anchor ─────────────────────────────────────
    // #sympAmount-ctrl is display:none while sympatheticCount reads 0, which is
    // the parameter's DEFAULT and therefore the state the stub seeds from
    // params.tsv. Driven up through the stub's own state object, which fires the
    // page's real valueChangedEvent -> updateSympVisibility() path rather than
    // reaching in and stripping the class. Without this, one of the 30 anchors
    // is unhoverable and its whole row of assertions would be a false pass on
    // an element nobody can reach.
    const revealed = await page.evaluate(() => {
        const st = window.__stubStates && window.__stubStates.sliders.get('sympatheticCount');
        if (!st) return { drove: false };
        st.setNormalisedValue(0.5);
        const el = document.getElementById('sympAmount-ctrl');
        return { drove: true, hidden: el.classList.contains('hidden'),
                 w: el.getBoundingClientRect().width };
    });
    check(revealed.drove && !revealed.hidden && revealed.w > 0,
        `[0] sympatheticCount driven off 0 through the stub's own state — #sympAmount-ctrl is `
        + `visible (${revealed.w ? revealed.w.toFixed(1) : 0}px wide). It is display:none at the `
        + `parameter default, so without this its tip could not be hovered at all`);

    // ── 1. every binding resolves, and lands on a DISTINCT node ─────────────
    const resolution = await page.evaluate((bindings) => {
        const seen = [];
        return bindings.map(([sel, key, wrapper]) => {
            const el = document.querySelector(sel);
            if (!el) return { key, sel, ok: false, why: 'selector matched nothing' };
            const target = wrapper ? (el.closest(wrapper) || null) : el;
            if (!target) return { key, sel, ok: false, why: `closest(${wrapper}) matched nothing` };
            const dup = seen.indexOf(target) >= 0;
            seen.push(target);
            return {
                key, sel, ok: true, dup,
                tag: target.tagName.toLowerCase()
                     + (target.className ? '.' + String(target.className).split(' ')[0] : ''),
                hasTip: target.hasAttribute('data-tip') && target.hasAttribute('data-tip-title'),
            };
        });
    }, TIP_BINDINGS);

    for (const r of resolution) {
        check(r.ok, `[1] binding ${r.key} resolves — ${r.sel}${r.ok ? ` -> ${r.tag}` : ` (${r.why})`}`);
        if (r.ok) check(r.hasTip,
            `[1] applyI18n wrote data-tip-title + data-tip onto ${r.key}'s anchor (${r.tag})`);
    }
    check(resolution.every((r) => r.ok && !r.dup),
        `[1] all ${TIP_BINDINGS.length} bindings land on DISTINCT nodes — two on one node means `
        + `the second silently overwrites the first while check-i18n reports both as bound`);
    check(tipWarns.length === 0,
        `[1] applyI18n logged no "tip target not found" warning`
        + (tipWarns.length ? ` — ${tipWarns.length}: ${tipWarns[0]}` : ''));

    // ── the hover driver ────────────────────────────────────────────────────
    // page.evaluate() given a STRING evaluates it as an EXPRESSION, so passing
    // READ_TIP directly returns the function OBJECT rather than calling it —
    // and an unserializable return arrives as undefined, which reads exactly
    // like "the tip was never there" and sails through a truthiness assertion
    // (measured on O-Bass). Invoked explicitly instead.
    const readTip = () => page.evaluate(`(${READ_TIP})()`);
    const hoverSelFor = (sel) => (sel.startsWith('.knob-control') ? `${sel} .knob-wrapper` : sel);

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
        // so a fixed sleep either flakes or records "no tip" for a path that
        // demonstrably works — and the obvious response to THAT failure is to
        // delete the fix (O-Comp).
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
    // hoverable until the gear is clicked. Everything else is reachable at rest.
    const NEEDS_POPOVER = new Set(['#lang-select']);

    const placement = { flippedLeft: 0, flippedUp: 0, onRail: 0, total: 0 };

    const sweep = async (lang) => {
        console.log(`\n-- ${lang.toUpperCase()}: hover every anchor, byte-compare, measure the rect`);
        let popoverOpen = false;
        const seen = [];
        for (const [sel, key] of TIP_BINDINGS) {
            const entry = (I18N[key] || {})[lang];
            if (!entry) { check(false, `[2] ${key} has an ${lang} entry`); continue; }

            if (NEEDS_POPOVER.has(sel) && !popoverOpen) {
                await page.click('#gear-btn', { force: true });
                await page.waitForSelector('#settings-popover:not([hidden])', { timeout: 2000 });
                popoverOpen = true;
            }

            const hs = hoverSelFor(sel);
            const st = await hoverAndRead(hs);
            seen.push(key);

            check(st !== null && st.visible,
                `[2][${lang}] ${key}: hovering ${hs} SHOWS the tip `
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

            // Placement census, reported rather than asserted: how often the
            // flip and the rail actually engage on this frame. A renderer that
            // never flips on 900 x 600 would make the second clamp pass dead
            // code, and a number here says whether it is exercised.
            const box = await page.evaluate((s) => {
                const e = document.querySelector(s);
                const r = e.getBoundingClientRect();
                return { cx: r.left + r.width / 2, cy: r.top + r.height / 2 };
            }, hs);
            ++placement.total;
            if (st.rect.x < box.cx) ++placement.flippedLeft;
            if (st.rect.y < box.cy) ++placement.flippedUp;
            if (Math.abs(st.rect.x - MARGIN) < 0.5
             || Math.abs(st.rect.y - MARGIN) < 0.5
             || Math.abs(SHIP_W - st.rect.x - st.rect.w - MARGIN) < 0.5
             || Math.abs(SHIP_H - st.rect.y - st.rect.h - MARGIN) < 0.5) ++placement.onRail;
        }
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

    // ── the surface must not eat its own hover ──────────────────────────────
    // pointer-events: none is declared in the rule and asserted statically
    // above; this is the behavioural half. With a tip open, the point at its
    // own centre must hit-test to something that is NOT the tooltip, or the
    // pointer bounces between anchor and surface and the tip flickers.
    // (A rect-comparing gate is blind to paint order — pattern_rect_gate_cannot
    // _see_paint_order — so this is an elementFromPoint check, not a rect one.)
    const openSt = await hoverAndRead('.knob-control[data-param="rosin"] .knob-wrapper');
    const hit = await page.evaluate(() => {
        const t = document.getElementById('tooltip');
        const r = t.getBoundingClientRect();
        const e = document.elementFromPoint(r.left + r.width / 2, r.top + r.height / 2);
        return e ? (e.id ? '#' + e.id : e.tagName.toLowerCase()
                    + (e.className ? '.' + String(e.className).split(' ')[0] : '')) : null;
    });
    check(openSt.visible && hit !== '#tooltip',
        `[4] the open surface does not hit-test to itself — elementFromPoint at its centre `
        + `returns "${hit}", so pointer-events:none is in force`);
    await park();

    // ── NC-3. the child-boundary rule ───────────────────────────────────────
    // pointerout fires at every internal boundary. Without the
    // anchorOf(relatedTarget) === active guard the tip flickers off and on as
    // the pointer crosses from the SVG to the caption inside the same cell.
    console.log('\n-- NC-3: moving between two children of the SAME anchor must not hide the tip');
    const bsSel = '.knob-control[data-param="bowSpeed"]';
    const beforeMove = await hoverAndRead(`${bsSel} .knob-wrapper`);
    await page.hover(`${bsSel} .knob-label`, { force: true });
    await page.waitForTimeout(200);
    const afterMove = await readTip();
    check(beforeMove.visible && afterMove.visible
          && afterMove.title === beforeMove.title && afterMove.title === I18N['tip.bowSpeed'].en.t,
        `[NC-3] the tip survives the .knob-wrapper -> .knob-label boundary inside `
        + `.knob-control — still "${afterMove.title}"`);
    await park();

    // ── NC-4. the mid-drag guard ────────────────────────────────────────────
    // bindSliderParam() begins a drag on mousedown and tracks it on document
    // mousemove, so the pointer can cross into a neighbouring cell with the
    // button held. Without the `dragging` guard, pointerover opens that
    // neighbour's tip on top of the gesture.
    console.log('\n-- NC-4: a drag crossing into a neighbouring knob must not open its tip');
    const bpSel = '.knob-control[data-param="bowPressure"] .knob-wrapper';
    const bsBox = await page.locator(`${bsSel} .knob-wrapper`).boundingBox();
    const bpBox = await page.locator(bpSel).boundingBox();
    await page.mouse.move(bsBox.x + bsBox.width / 2, bsBox.y + bsBox.height / 2);
    await page.mouse.down();
    await page.mouse.move(bpBox.x + bpBox.width / 2, bpBox.y + bpBox.height / 2, { steps: 8 });
    await page.waitForTimeout(250);
    const midDrag = await readTip();
    await page.mouse.up();
    check(!midDrag.visible,
        `[NC-4] no tip opened while the button was held across the neighbouring cell `
        + (midDrag.visible ? `(got "${midDrag.title}")` : '(surface stayed hidden)'));
    // And the guard must RELEASE: a hover after mouseup opens normally again.
    const afterDrag = await hoverAndRead(bpSel);
    check(afterDrag.visible && afterDrag.title === I18N['tip.bowPressure'].en.t,
        `[NC-4] the guard releases on mouseup — hovering the same cell now opens `
        + `"${afterDrag.title}". Without this half the guard could be a permanent off switch`);
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
    await sweep('fr');

    await page.evaluate((l) => window.__setLanguage(l), 'en');
    await page.waitForTimeout(150);
    const backSt = await hoverAndRead('.knob-control[data-param="brightness"] .knob-wrapper');
    check(backSt.visible && backSt.title === I18N['tip.brightness'].en.t
          && backSt.body === I18N['tip.brightness'].en.b,
        `[5] switching back to English restores the English tip byte-for-byte `
        + `— "${backSt.title}"`);
    await park();

    console.log(`\n   placement census over ${placement.total} hovers (both languages): `
        + `${placement.flippedLeft} placed left of the cursor, ${placement.flippedUp} above it, `
        + `${placement.onRail} landed on an ${MARGIN}px rail`);

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
    // no latch whatsoever.
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
        // demands opacity > 0.99, so a fixed 80 ms sleep reads mid-fade and
        // records "no tip" on a tab that DID open one. O-Comp reported exactly
        // that artefact, and the obvious response to it is to delete the latch.
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
    console.log('\n-- NC-1: plant an over-long body and confirm assertion 4 reports the overflow');
    const nc = fs.mkdtempSync(path.join(os.tmpdir(), 'obwd-tip-nc-'));
    const servedI18n = path.join(built.root, 'js', 'i18n.js');
    const backup = path.join(nc, 'i18n.served.orig.js');
    fs.copyFileSync(servedI18n, backup);
    const origSrc = fs.readFileSync(backup, 'utf8');
    try {
        // SIZED AGAINST THIS FRAME, not by habit. The usable height is
        // SHIP_H - 2*MARGIN = 584px; at the 260px cap the content box is 238px
        // and 12px/1.35 serif gives ~16.2px per line, so ~36 lines fill it and
        // ~45 chars fit on a line — roughly 1600 characters to reach the rail.
        // 6000 is a comfortable multiple of that, and the HEIGHT is asserted
        // below rather than assumed: O-Tremolo's 40x plant fit inside a 400px
        // frame and reported nothing.
        //
        // No apostrophe anywhere in the plant: the bodies in this file are
        // SINGLE-quoted, so one would close the string and break the module
        // rather than overflow the frame — a plant that fails for the wrong
        // reason is a control that proves nothing.
        const UNIT = 'Une phrase de controle negatif, deliberement beaucoup trop longue pour '
                   + 'tenir dans le cadre, repetee afin de faire deborder la surface par le bas. ';
        const LONG = UNIT.repeat(40);
        check(LONG.length > 1600,
            `[NC-1] the plant is ${LONG.length} chars — above the ~1600 this 900 x 600 frame `
            + `needs at the ${MAX_W}px cap. A plant that FITS is indistinguishable from a gate `
            + `that cannot see`);
        const planted = origSrc.replace(
            /(en: \{ t: 'Bow Speed',\n)[\s\S]*?(\n        fr: \{ t: 'Vitesse)/,
            `$1              b: '${LONG}' },$2`);
        check(planted !== origSrc,
            `[NC-1] the plant actually edited tip.bowSpeed's en body — a no-op replace would `
            + `make this control vacuous`);
        fs.writeFileSync(servedI18n, planted);

        await page.goto(url, { waitUntil: 'networkidle' });
        const bad = await hoverAndRead(`${bsSel} .knob-wrapper`);
        check(bad.visible, `[NC-1] the planted tip still renders (${bad.rect.h.toFixed(1)}px tall)`);
        check(bad.rect.h > SHIP_H - 2 * MARGIN,
            `[NC-1] the planted surface is ${bad.rect.h.toFixed(1)}px tall, taller than the `
            + `${SHIP_H - 2 * MARGIN}px the frame can hold — the plant is big enough to defeat `
            + `the clamp, which is what makes the next line a control`);
        check(!inFrame(bad.rect),
            `[NC-1] assertion 4 REPORTS the overflow — rect ${bad.rect.w.toFixed(1)} x `
            + `${bad.rect.h.toFixed(1)} at ${bad.rect.x.toFixed(1)},${bad.rect.y.toFixed(1)} `
            + `leaves the ${SHIP_W} x ${SHIP_H} frame (${edges(bad.rect)}). If this PASSES as `
            + `in-frame, assertion 4 is decoration and every green above means nothing`);
    } finally {
        fs.copyFileSync(backup, servedI18n);
    }

    await page.goto(url, { waitUntil: 'networkidle' });
    const restored = await hoverAndRead(`${bsSel} .knob-wrapper`);
    check(fs.readFileSync(servedI18n, 'utf8') === origSrc,
        `[NC-1] the served copy is byte-identical to the pre-plant snapshot again`);
    check(restored.visible && restored.body === I18N['tip.bowSpeed'].en.b && inFrame(restored.rect),
        `[NC-1] restored — tip.bowSpeed is back inside the frame (${edges(restored.rect)})`);
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
