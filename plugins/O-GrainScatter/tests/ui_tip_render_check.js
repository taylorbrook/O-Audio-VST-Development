/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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

    ui_tip_render_check.js — O-GrainScatter hover-help, RENDERED.

    ── WHY THIS FILE EXISTS ────────────────────────────────────────────────────

    NO OTHER GATE IN THIS REPO CAN SEE A RENDERED TOOLTIP.

      check-i18n      reads the table statically. Its assertion 2 counts
                      TIP_BINDINGS rows and checks each key exists in I18N.
                      Thirty-eight bindings pointing at thirty-eight selectors
                      that match nothing, on a page with no renderer at all, is
                      a PASS.
      check-ui-labels has no tooltip awareness whatsoever. Its whole output is
                      BYTE-IDENTICAL before and after this feature landed —
                      measured, not assumed, and reported in the v2.6.0 commit.
      boot-all-uis    counts aria-label and title. It never counts data-tip.

    So authoring 38 tooltip bodies into i18n.js and binding them, with no other
    change, would have shipped 38 invisible strings past three green gates.
    v2.5.0 of this plugin had NO #tooltip node, NO .tooltip rule and NO hover
    handler, which is exactly the state in which that failure is silent.

    THIS PLUGIN HAS THE SCAR ALREADY, from the other direction. Its Stage-K work
    lost a whole uncommitted edit when a negative control was restored with
    `git checkout -- <file>`, which took the fix along with the plant. The
    negative control below is a DOM ATTRIBUTE write, restored by re-running
    applyI18n() — there is no file to check out, and nothing on disk to lose.

    ── WHAT IT IS NOT ──────────────────────────────────────────────────────────

    Deliberately NOT a port of the three committed ui_tooltip_clamp_check.js
    gates (O-Tapestop, O-Bitrot, O-ReverseDelay, ~800 lines each). Those are
    built around the OTHER renderer family — measure-then-pin placement with an
    above/below flip and a help-toggle state — none of which exists here.

    ── THE FRAME IS 900 x 800, READ NOT GUESSED ────────────────────────────────

    Pinned to the SHIPPING size parsed out of PluginEditor.cpp's setSize(),
    never a default 1280x720: a clamp gate at the wrong viewport measures a page
    that has room and certifies nothing
    (pattern_tooltip_clamp_gate_viewport_sensitive). This is the roomiest frame
    in batch M2, which is precisely why the negative control below is sized
    against 800 px of vertical clamp room rather than by habit — O-Tremolo's 40x
    plant FIT inside a 400 px frame and reported nothing, and a plant that fits
    is indistinguishable from a gate that cannot see.

    ── AND FRENCH IS THE OTHER HALF ────────────────────────────────────────────

    French runs 15-20 % longer, wraps to more lines against the 280 px
    max-width cap, and GROWS THE TIP'S HEIGHT. A tip that fits in English can
    therefore overflow the bottom in French, which is why every assertion runs
    in both languages rather than in English with a French spot-check.

    Usage:
        node plugins/O-GrainScatter/tests/ui_tip_render_check.js
        node plugins/O-GrainScatter/tests/ui_tip_render_check.js --verbose

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

const PLUGIN  = 'O-GrainScatter';
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
// concatenations the 38 bodies are authored with — rather than against a
// regex's idea of them. A fixture that mirrored the table would drift silently
// (pattern_test_fixture_mirrors_drift_silently).
async function loadTable() {
    const src = path.join(REPO_ROOT, 'plugins', PLUGIN, 'Source', 'ui', 'public', 'js', 'i18n.js');
    if (!fs.existsSync(src)) throw new Error(`i18n.js not found at ${src}`);

    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'ograinscatter-i18n-'));
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
// The body is read as the concatenation of the surface's own TEXT NODES, and
// the title as .tip-title's textContent, because that is exactly how the
// renderer builds it: createElement + appendChild(createTextNode). Reading
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

(async () => {
    console.log(`ui_tip_render_check — ${PLUGIN} hover-help, RENDERED\n`);

    const chromiumPkg = S.resolvePlaywright();
    if (!chromiumPkg) {
        console.error('Playwright is not resolvable. NOTHING was verified — this is not a pass.');
        console.error('  npm i -D playwright   (or run under npx playwright)');
        process.exit(77);
    }
    const { chromium } = chromiumPkg;

    // ── the shipping frame, READ not guessed ────────────────────────────────
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

    // The two chrome anchors live inside a [hidden] popover and do not exist to
    // a pointer until the gear is clicked.
    const CHROME = new Set(['#gear-btn', '#lang-select']);

    // ── THE GATED FOURTEEN ──────────────────────────────────────────────────
    //
    // FOUND BY THIS GATE ON ITS FIRST RUN, and it is a property of the page
    // rather than of the renderer. Fourteen of the 36 controls are INERT in
    // this plugin's default state and carry `pointer-events: none` while they
    // are, so no pointerover ever reaches them and no tip can open:
    //
    //   setupPitchGate   (app.js:326-345)  dims .dropdown-container for scale,
    //       root_note and pitch_mode whenever pitch_random is below 0.01, via
    //       the .dimmed class (index.html:287, opacity .25 + pointer-events
    //       none). pitch_random defaults to 0.0, so all three are inert at
    //       load.
    //   setupSpatialGate (app.js:351-375)  sets style.pointerEvents = 'none' on
    //       the ten spatial .knob-container elements and the trajectory
    //       .dropdown-container whenever spatial_mode is Off, which is its
    //       default.
    //
    // The information is NOT lost to a user: the page shows #pitch-hint and
    // #spatial-hint in exactly those states, and both say what to raise. The
    // gating is shipped, working interaction code, so this gate DRIVES the page
    // out of both gated states through the page's own listeners rather than
    // changing the gating to suit itself. The rest state is pinned as an
    // assertion below so that a future change to either gate is visible here.
    const GATED = new Set([
        'select[data-param="scale"]', 'select[data-param="root_note"]',
        'select[data-param="pitch_mode"]', 'select[data-param="trajectory"]',
        '.knob[data-param="azimuth"]', '.knob[data-param="elevation"]',
        '.knob[data-param="az_spread"]', '.knob[data-param="el_spread"]',
        '.knob[data-param="distance"]', '.knob[data-param="spatial_width"]',
        '.knob[data-param="traj_speed"]', '.knob[data-param="dist_lpf"]',
        '.knob[data-param="doppler"]', '.knob[data-param="spatial_smooth"]',
    ]);

    const anchorsClosed = TIP_BINDINGS.map(b => b[0]).filter(s => !CHROME.has(s));
    const anchorsOpen   = TIP_BINDINGS.map(b => b[0]).filter(s =>  CHROME.has(s));
    note(`${anchorsClosed.length} parameter anchor(s) + ${anchorsOpen.length} chrome anchor(s), `
       + `${GATED.size} of them behind a dim gate\n`);

    const built = S.buildRoot(PLUGIN, { repoRoot: REPO_ROOT });
    const misses = [];
    const { port, close } = await S.serve(built.root, (u) => misses.push(u));

    const pageErrors = [];
    const consoleErrors = [];
    const tipWarns = [];

    const browser = await chromium.launch();
    // `viewport`, not `viewportSize`. A clamp gate at 1280x720 has room the
    // shipping frame does not and certifies nothing.
    const page = await browser.newPage({ viewport: { width: W, height: H }, deviceScaleFactor: 1 });

    page.on('pageerror', (e) => pageErrors.push(String(e)));
    page.on('console', (m) => {
        const t = m.text();
        if (m.type() === 'error') consoleErrors.push(t);
        if (/tip target not found/.test(t)) tipWarns.push(t);
    });

    try {
        await page.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });
        await page.waitForTimeout(500);

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
            // "it would enter check-ui-labels' sweep" would be the natural thing
            // to write here and it is FALSE — measured on O-Bass. Un-hiding the
            // surface leaves that gate byte-identically green, because a fixed
            // box at 0,0 has the same rectangle in both languages: it neither
            // moves nor changes the visible element SET. What DOES catch it is
            // check-i18n assertion 10, boot-all-uis' text count — and this.
            check(surface.visibility === 'hidden' || surface.opacity === '0',
                '[0] the surface is HIDDEN at rest '
                + `(visibility ${surface.visibility}, opacity ${surface.opacity})`);
            // #lang-select is itself an anchor and lives INSIDE .settings-popover
            // (z-index 61), so a tip that did not out-stack the panel would open
            // BEHIND the control that revealed it.
            check(Number(surface.zIndex) > 61,
                `[0] the surface out-stacks .settings-popover (61) — z-index ${surface.zIndex}`);
            note(`surface: ${surface.position}, z-index ${surface.zIndex}, `
               + `max-width ${surface.maxWidth}, visibility ${surface.visibility}`);
        }

        // ── 1. EVERY TIP_BINDINGS SELECTOR RESOLVES ─────────────────────────
        //
        // applyI18n's own failure here is a console.warn, which boot-all-uis
        // prints and nothing fails on. A binding that finds no element is a
        // FAIL in this gate, not a warning.
        //
        // Both halves of "bind to the ids the UI already uses" are checked
        // separately, because they fail independently — 34 of the 36 parameter
        // selectors here are attribute selectors rather than ids, AND 34 of
        // them resolve through a wrapper rather than onto the id'd node.
        console.log('-- 1. binding resolution');
        const resolution = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
            const el = document.querySelector(sel);
            if (!el) return { sel, key, found: false };
            const target = wrapper ? (el.closest(wrapper) || el) : el;
            return {
                sel, key, found: true,
                wrapperDeclared: !!wrapper,
                wrapperResolved: wrapper ? !!el.closest(wrapper) : null,
                anchorTag: target.tagName.toLowerCase(),
                anchorClass: target.className || '',
                hasTip: target.hasAttribute('data-tip'),
                hasTitle: target.hasAttribute('data-tip-title'),
                anchorIsSelf: target === el,
                // The hover AREA the binding actually arms, which is the number
                // the wrapper walk exists to change: a 48 px circle against the
                // 62 px cell that holds its caption and its readout.
                area: Math.round(target.getBoundingClientRect().width
                               * target.getBoundingClientRect().height),
                selfArea: Math.round(el.getBoundingClientRect().width
                                   * el.getBoundingClientRect().height),
            };
        }), TIP_BINDINGS.map(b => [b[0], b[1], b[2] || null]));

        for (const r of resolution) {
            check(r.found, `[1] selector resolves: ${r.sel}  (key ${r.key})`);
            if (!r.found) continue;
            check(r.hasTip && r.hasTitle,
                `[1] applyI18n wrote data-tip + data-tip-title onto the anchor for ${r.sel}`);
            if (r.wrapperResolved === false)
                check(false, `[1] the declared wrapper for ${r.sel} did NOT resolve — the tip fell `
                           + 'back onto the id\'d node, which is not the hover target it was bound to');
            if (verbose) note(`${r.sel} -> ${r.anchorTag}.${r.anchorClass}`
                            + (r.anchorIsSelf ? ' (self)' : ' (wrapper)')
                            + ` area ${r.selfArea} -> ${r.area} px2`);
        }
        check(tipWarns.length === 0,
            '[1] applyI18n logged no "tip target not found" warning',
            tipWarns.slice(0, 3).join(' | '));

        const bySel = new Map(resolution.map(r => [r.sel, r]));
        const idSelectors  = resolution.filter(r => r.sel.startsWith('#')).length;
        const viaWrapper   = resolution.filter(r => r.wrapperDeclared && !r.anchorIsSelf).length;
        const areaGain     = resolution.filter(r => r.area > r.selfArea).length;
        note(`selector half: ${idSelectors} of ${resolution.length} bindings use an id`);
        note(`target half:   ${viaWrapper} of ${resolution.length} resolve through a wrapper, `
           + `${areaGain} of which enlarge the hover area`);

        // ── the driving loop ────────────────────────────────────────────────
        async function setPopover(open) {
            const isOpen = await page.evaluate(() =>
                !document.getElementById('settings-popover').hidden);
            if (isOpen !== open) {
                await page.click('#gear-btn');
                await page.waitForTimeout(140);
            }
            return await page.evaluate(() => !document.getElementById('settings-popover').hidden);
        }

        async function hoverAnchor(sel, wrapper) {
            // Move somewhere neutral first so pointerover definitely fires on
            // the next move: a pointer already inside the anchor generates no
            // new pointerover, and the tip would be measured in whatever state
            // the PREVIOUS anchor left it — which is exactly how a "contains"
            // check passes on stale text. (2, 2) is the header's dead margin;
            // the top-RIGHT corner would be inside the gear cluster.
            await page.mouse.move(2, 2);
            await page.waitForTimeout(50);

            const box = await page.evaluate(({ sel, wrapper }) => {
                const el = document.querySelector(sel);
                if (!el) return null;
                const target = wrapper ? (el.closest(wrapper) || el) : el;
                const r = target.getBoundingClientRect();
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

        // ── G. THE GATED FOURTEEN, AT REST AND THEN DRIVEN ──────────────────
        //
        // Pinned as an assertion rather than left implicit. At load, pitch_random
        // is 0.0 and spatial_mode is Off, so fourteen controls carry
        // `pointer-events: none` and NO pointer can open their tip. That is a
        // property of shipped gating code (app.js:326-375), not of the renderer,
        // and this section records it so a later change to either gate shows up
        // here instead of silently changing what 14 of 38 tips do.
        //
        // Note the asymmetry, which is worth knowing and is NOT fixed here:
        // `pointer-events: none` does not remove an element from the tab ring, so
        // the three gated <select>s still open their tip from the KEYBOARD while
        // being unreachable by mouse.
        console.log('\n-- G. the dim gates');
        const gatedList = [...GATED];
        const restRead = [];
        for (const sel of gatedList.slice(0, 3).concat(gatedList.slice(-3))) {
            const b = TIP_BINDINGS.find(x => x[0] === sel);
            const got = await hoverAnchor(sel, b[2]);
            restRead.push({ sel, shown: got ? got.tip.shown : null });
        }
        check(restRead.every(r => r.shown === false),
            `[G] at rest, a POINTER opens no tip on a dim-gated control — ${restRead.length} sampled, `
            + `${restRead.filter(r => r.shown).length} unexpectedly shown`,
            restRead.filter(r => r.shown).map(r => r.sel).join(', '));

        const hintsAtRest = await page.evaluate(() => ({
            pitch: document.getElementById('pitch-hint').classList.contains('visible'),
            spatial: document.getElementById('spatial-hint').classList.contains('visible'),
        }));
        check(hintsAtRest.pitch && hintsAtRest.spatial,
            '[G] the page explains BOTH gated states in-line instead — #pitch-hint and '
            + `#spatial-hint are both visible at rest (${hintsAtRest.pitch}, ${hintsAtRest.spatial})`);

        // Driven through the page's OWN listeners, not by editing the DOM: the
        // stub exposes its state registry precisely so a gate can run a real
        // state pass, and setNormalisedValue / setChoiceIndex fire the
        // valueChangedEvent that setupPitchGate and setupSpatialGate subscribe
        // to. Reaching in and stripping the .dimmed class would test a page
        // nobody ships.
        const ungated = await page.evaluate(() => {
            window.__stubStates.sliders.get('pitch_random').setNormalisedValue(0.5);
            window.__stubStates.combos.get('spatial_mode').setChoiceIndex(1);
            return {
                dimmed: document.querySelectorAll('.dropdown-container.dimmed').length,
                inert: [...document.querySelectorAll('#spatial-group .knob-container')]
                        .filter(k => k.style.pointerEvents === 'none').length,
            };
        });
        await page.waitForTimeout(400);   // past the 0.3 s opacity transition
        check(ungated.dimmed === 0 && ungated.inert === 0,
            '[G] driving pitch_random to 0.5 and spatial_mode to Scatter through the page\'s own '
            + `listeners clears both gates — ${ungated.dimmed} dimmed, ${ungated.inert} inert left`);
        note('every assertion below this line runs with both gates OPEN, which is the only state '
           + 'in which all 36 parameter tips are pointer-reachable');

        for (const lang of ['en', 'fr', 'en']) {
            const isReturn = lang === 'en' && page.__seenEn;
            page.__seenEn = page.__seenEn || lang === 'en';
            console.log(`\n-- 2/3/4. language: ${lang}${isReturn ? ' (return pass)' : ''}`);

            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(150);

            let clamped = 0, flipped = 0, maxH = 0;

            for (const group of [{ open: false, list: anchorsClosed }, { open: true, list: anchorsOpen }]) {
                const state = await setPopover(group.open);
                check(state === group.open,
                    `[.] settings popover is ${group.open ? 'OPEN' : 'closed'} for this group — got `
                    + `${state ? 'open' : 'closed'}`);

                for (const sel of group.list) {
                    const b = TIP_BINDINGS.find(x => x[0] === sel);
                    if (!b) { check(false, `[.] ${sel} is in TIP_BINDINGS`); continue; }
                    const [, key, wrapper] = b;
                    const entry = (I18N[key] || {})[lang] || {};

                    const got = await hoverAnchor(sel, wrapper);
                    if (!got) { check(false, `[2][${lang}] ${sel} is hoverable`); continue; }
                    const t = got.tip;

                    // ── 2. THE VACUITY GUARD ────────────────────────────────
                    // A tip that never showed is the failure this whole file
                    // exists for. It FAILS.
                    const visible = t.shown && t.visibility === 'visible' && t.opacity === '1';
                    check(visible,
                        `[2][${lang}] hovering ${sel} SHOWS the tip `
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

                    // Placement bookkeeping, so "0 failures" is a number rather
                    // than a verdict: which anchors needed the flip, and which
                    // then needed the clamp on top of it.
                    if (t.rect.left <= 8.5 || Math.abs(t.rect.right - (W - 8)) < 0.5
                        || t.rect.top <= 8.5 || Math.abs(t.rect.bottom - (H - 8)) < 0.5) clamped++;
                    if (t.rect.left < got.box.x || t.rect.top < got.box.y) flipped++;
                    maxH = Math.max(maxH, t.rect.h);

                    if (verbose)
                        note(`${lang} ${sel}: cursor ${got.box.x.toFixed(0)},${got.box.y.toFixed(0)} `
                           + `-> tip ${t.rect.left.toFixed(1)},${t.rect.top.toFixed(1)} `
                           + `${t.rect.w.toFixed(1)}x${t.rect.h.toFixed(1)}`);
                }
            }
            note(`${lang}: ${flipped} tip(s) placed by FLIP, ${clamped} touching a clamp edge, `
               + `tallest ${maxH.toFixed(1)} px`);
        }

        // ── 5. FRENCH REALLY IS TALLER, and English really came back ────────
        //
        // Re-measured here rather than inferred from the loop: the point of
        // running both languages is that French wraps to more lines against the
        // 280 px max-width cap, and if it did NOT the two passes would be the
        // same measurement twice and assertion 4's French half would be
        // decoration.
        console.log('\n-- 5. French height vs English');
        async function heightsFor(lang) {
            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(150);
            await setPopover(false);
            const h = {};
            for (const sel of anchorsClosed) {
                const got = await hoverAnchor(sel, TIP_BINDINGS.find(x => x[0] === sel)[2]);
                h[sel] = got ? got.tip.rect.h : -1;
            }
            return h;
        }
        const hEn = await heightsFor('en');
        const hFr = await heightsFor('fr');
        const grew   = anchorsClosed.filter(s => hFr[s] > hEn[s] + 0.5);
        const shrank = anchorsClosed.filter(s => hFr[s] < hEn[s] - 0.5);
        const same   = anchorsClosed.filter(s => Math.abs(hFr[s] - hEn[s]) <= 0.5);
        check(grew.length > 0,
            `[5] French GROWS at least one tip's height against the 280 px cap — `
            + `${grew.length} grew, ${same.length} unchanged, ${shrank.length} shrank`,
            grew.length ? null
                : 'no tip grew: the fr pass measured the same boxes as en, so its clamp half is decoration');
        note(`tallest en ${Math.max(...anchorsClosed.map(s => hEn[s])).toFixed(1)} px, `
           + `tallest fr ${Math.max(...anchorsClosed.map(s => hFr[s])).toFixed(1)} px`);
        if (verbose)
            for (const s of grew) note(`  grew: ${s} ${hEn[s].toFixed(0)} -> ${hFr[s].toFixed(0)}`);

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const backEn = await hoverAnchor('.knob[data-param="grain_size"]', '.knob-container');
        check(backEn && backEn.tip.title === I18N['tip.grainSize'].en.t
                     && backEn.tip.body  === I18N['tip.grainSize'].en.b,
            '[5] English comes back after the French pass — byte-equal again');

        // ── 6. THE NEGATIVE CONTROL ─────────────────────────────────────────
        //
        // Assertion 4 has passed at every anchor in both languages. That is
        // indistinguishable from an assertion 4 that CANNOT SEE an overflow
        // until one is planted and it reports.
        //
        // THE PLANT IS SIZED AGAINST THIS FRAME, not by habit. O-Tremolo's 40x
        // plant — 880 chars, ~390 px tall — FIT inside a 400 px frame with
        // 384 px of clamp room and reported nothing. This frame is 800 px tall
        // with 784 px of clamp room, and the surface caps at 280 px wide with
        // ~11 px text, so roughly 45 characters land per line and 15.4 px of
        // height per line: about 2300 characters is break-even. The plant below
        // is 4800, and the assertion immediately under it CONFIRMS the plant
        // actually overflowed rather than assuming it.
        //
        // The plant is a DOM ATTRIBUTE write, not a file edit. applyI18n() is
        // the only writer of data-tip, so window.__setLanguage('en') restores
        // the real value exactly and there is nothing on disk to restore. That
        // matters here specifically: this plugin lost a whole uncommitted edit
        // in Stage K to a `git checkout -- <file>` used to undo a plant.
        console.log('\n-- 6. negative control (assertion 4 harness-blindness)');
        const PLANT_SEL = '.knob[data-param="grain_size"]';
        const PLANT = 'overflow probe. '.repeat(300);
        await page.evaluate(({ sel, body }) => {
            document.querySelector(sel).closest('.knob-container')
                    .setAttribute('data-tip', body);
        }, { sel: PLANT_SEL, body: PLANT });

        const planted = await hoverAnchor(PLANT_SEL, '.knob-container');
        const plantedOut = planted ? outsideViewport(planted.tip.rect, W, H) : ['tip did not render'];
        check(planted && planted.tip.body === PLANT,
            '[6] the plant actually reached the surface (a plant that never rendered proves nothing)');
        check(planted && planted.tip.rect.h > H,
            `[6] the plant is BIGGER THAN THE FRAME — ${planted ? planted.tip.rect.h.toFixed(0) : '?'} px `
            + `tall against ${H} px. A plant that fits is indistinguishable from a gate that cannot see`);
        check(plantedOut.length > 0,
            '[6] a planted over-long body OVERFLOWS and assertion 4 reports it — '
            + `${plantedOut.join('; ') || 'NOTHING REPORTED'}`,
            plantedOut.length ? null
                              : 'assertion 4 is BLIND — every [4] pass above is decoration');
        if (planted) note(`planted tip ${planted.tip.rect.w.toFixed(1)} x `
                        + `${planted.tip.rect.h.toFixed(1)} in a ${W} x ${H} frame`);

        // restore from the TABLE, and prove the restore took
        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const restored = await hoverAnchor(PLANT_SEL, '.knob-container');
        check(restored && restored.tip.body === I18N['tip.grainSize'].en.b,
            '[6] restored: the anchor carries the table body again, byte-equal');
        check(restored && outsideViewport(restored.tip.rect, W, H).length === 0,
            '[6] restored: assertion 4 is green again at the same anchor');

        // ── 6b. THE CLAMP, EXERCISED — a POSITIVE control ───────────────────
        //
        // Every shipped tip on this page placed with room to spare: 19 of 36
        // needed the FLIP, and NOT ONE touched a clamp edge. So the four-edge
        // clamp is dead code as far as sections 2-5 can tell, and "assertion 4
        // passed 216 times" says nothing about it.
        //
        // O-Bass's carried trap 3 is the shape that bites here: ONE FLIP IS NOT
        // ENOUGH, and the flipped result needs clamping again. A plant tall
        // enough that NEITHER side of the cursor fits flips upward to a negative
        // `top` and is then clamped to the 8 px margin. If the clamp did not run
        // after the flip, this tip would sit at roughly y = -180 and assertion 4
        // would report `top < 0` — so a PASS here, at exactly the margin, is the
        // clamp doing the work rather than the flip.
        console.log('\n-- 6b. the clamp after the flip (positive control)');
        const MID_SEL = '.knob[data-param="probability"]';
        const TALL = 'clamp probe. '.repeat(130);
        await page.evaluate(({ sel, body }) => {
            document.querySelector(sel).closest('.knob-container')
                    .setAttribute('data-tip', body);
        }, { sel: MID_SEL, body: TALL });

        const tall = await hoverAnchor(MID_SEL, '.knob-container');
        if (tall) {
            const wouldNotFitBelow = tall.box.y + 16 + tall.tip.rect.h > H - 8;
            const wouldNotFitAbove = tall.box.y - tall.tip.rect.h - 12 < 8;
            check(wouldNotFitBelow && wouldNotFitAbove,
                `[6b] the plant fits on NEITHER side of the cursor at y=${tall.box.y.toFixed(0)} `
                + `(${tall.tip.rect.h.toFixed(0)} px tall in ${H} px) — otherwise this control is `
                + 'a second copy of assertion 4 rather than a test of the clamp');
            check(Math.abs(tall.tip.rect.top - 8) < 0.5,
                `[6b] the flipped tip is CLAMPED to the 8 px margin — top ${tall.tip.rect.top.toFixed(2)}, `
                + `where the flip alone gives ${(tall.box.y - tall.tip.rect.h - 12).toFixed(0)}`);
            check(outsideViewport(tall.tip.rect, W, H).length === 0,
                '[6b] and the clamped result is fully inside the frame');
            note(`clamp probe ${tall.tip.rect.w.toFixed(1)} x ${tall.tip.rect.h.toFixed(1)} `
               + `placed at ${tall.tip.rect.left.toFixed(1)},${tall.tip.rect.top.toFixed(1)}`);
        } else {
            check(false, '[6b] the clamp probe rendered');
        }

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const restored2 = await hoverAnchor(MID_SEL, '.knob-container');
        check(restored2 && restored2.tip.body === I18N['tip.probability'].en.b,
            '[6b] restored: the anchor carries the table body again, byte-equal');

        // ══════════════════════════ 7. THE FOCUS LATCH, BOTH HALVES ═════════
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
        // O-Comp proved that deleting only the `if (lastInputWasPointer) return;`
        // guard leaves the declaration, the pointerdown write and the keydown
        // clear all matching. Only the behavioural control below sees it.
        console.log('\n-- 7. the focus latch');

        await setPopover(false);
        await page.mouse.move(1, 1);
        await page.waitForTimeout(150);
        // BLUR FIRST, and this line is the whole reason the assertion below can
        // fail at all. An earlier section of this gate leaves focus ON
        // #gear-btn, and clicking an ALREADY-FOCUSED element fires no focusin —
        // so without this the check reports "no tip after a click" for a page
        // with no latch whatsoever. That version passed 125/125 with the latch
        // deleted, on O-Bass; O-Tremolo then passed 186/186 with BOTH the latch
        // and this line removed, which is the control ON the control.
        await page.evaluate(() => document.activeElement && document.activeElement.blur());
        await page.waitForTimeout(100);
        await page.click('#gear-btn');
        await page.waitForTimeout(320);
        const afterClick = await page.evaluate(() => {
            const t = document.getElementById('tooltip');
            const cs = getComputedStyle(t);
            const r = t.getBoundingClientRect();
            const panel = document.getElementById('settings-popover');
            const pr = panel && !panel.hidden ? panel.getBoundingClientRect() : null;
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
        // click, and .focus() on an already-focused element fires no event at
        // all — either one would report "no tip" and record that as correct.
        //
        // `visibility`, not a hard opacity threshold, and 200 ms per press: a
        // probe sampling 80 ms into a 120 ms transition reports a false "never
        // opens", and the obvious response to that failure is to delete the
        // latch (O-Comp).
        await page.keyboard.press('Escape');
        await page.waitForTimeout(150);
        let kbHit = null;
        for (let i = 1; i <= 24; i++) {
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
                                || document.activeElement.className)
                             : null };
            });
            if (r.shown && r.text) { kbHit = { press: i, ...r }; break; }
        }
        check(kbHit !== null,
            '[7] a KEYBOARD tab still opens a tip — the accessibility half survives the latch'
            + (kbHit ? ` (tab #${kbHit.press} on ${kbHit.on})` : ' — none in 24 tabs'));
        await page.keyboard.press('Escape');
        await page.waitForTimeout(150);

        // ── housekeeping ────────────────────────────────────────────────────
        console.log('');
        check(pageErrors.length === 0, '[8] no uncaught page error across the whole sweep',
            pageErrors.slice(0, 3).join(' | '));
        check(consoleErrors.length === 0, '[8] no console.error across the whole sweep',
            consoleErrors.slice(0, 3).join(' | '));
        check(misses.length === 0, '[8] every requested resource was served',
            misses.slice(0, 5).join(', '));

        // A native title= would render a second, untranslated OS tooltip
        // competing with this surface (contract §4). Repo-wide it is 0 today,
        // and this renderer must not reintroduce one.
        const nativeTitles = await page.evaluate(() =>
            document.querySelectorAll('[title]').length);
        check(nativeTitles === 0,
            `[8] zero native title= attributes on the page — got ${nativeTitles}`);

        // Every parameter that HAS a control is bound. 36 of 36 on this page,
        // asserted rather than counted by eye: a binding quietly dropped from
        // the table would leave a knob with no hover-help and every other
        // assertion here green.
        const boundParams = await page.evaluate(() =>
            document.querySelectorAll('[data-param]').length);
        check(bySel.size === TIP_BINDINGS.length,
            `[8] every TIP_BINDINGS row was resolved — ${bySel.size} of ${TIP_BINDINGS.length}`);
        check(TIP_BINDINGS.length === boundParams + 2,
            `[8] one tip per on-page control plus the two chrome anchors — `
            + `${boundParams} [data-param] elements + 2 vs ${TIP_BINDINGS.length} bindings`);

    } finally {
        await browser.close();
        await close();
        fs.rmSync(built.root, { recursive: true, force: true });
    }

    console.log(`\n${failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`}`
              + `   (${passes} passed)`);
    process.exit(failed === 0 ? 0 : 1);
})().catch((e) => { console.error(e); process.exit(1); });
