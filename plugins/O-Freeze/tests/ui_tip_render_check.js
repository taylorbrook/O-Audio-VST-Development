/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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

    ui_tip_render_check.js — O-Freeze hover-help, RENDERED.

    ── WHY THIS FILE EXISTS ────────────────────────────────────────────────────

    NO OTHER GATE IN THIS REPO CAN SEE A RENDERED TOOLTIP.

      check-i18n      reads the table statically. Its assertion 2 counts
                      TIP_BINDINGS rows and checks each key exists in I18N.
                      Fourteen bindings pointing at fourteen selectors that
                      match nothing, on a page with no renderer at all, is a
                      PASS.
      check-ui-labels has no tooltip awareness whatsoever — it sweeps
                      [data-i18n] elements and never looks at data-tip.
      boot-all-uis    counts aria-label and title. It never counts data-tip.

    So authoring fourteen tooltip bodies into i18n.js and binding them, with no
    other change, would have shipped twenty-eight invisible strings past three
    green gates. v2.1.0 of this plugin had NO #tooltip node, NO .tooltip rule
    and NO hover handler — the exact state in which that failure is silent.

    THIS PLUGIN HAS THE SCAR ALREADY, from the other direction. Stage K reported
    that check-i18n's I18N_EXEMPT set is matched by TEXT rather than by element,
    so the exempt entry for the MODE option "Threshold" silently covers
    #threshold-knob's caption as well. A gate that certifies the absence of a
    thing it cannot see is the failure mode this file exists to close for
    hover-help.

    ── WHAT IT IS NOT ──────────────────────────────────────────────────────────

    Deliberately NOT a port of the three committed ui_tooltip_clamp_check.js
    gates (O-Tapestop, O-Bitrot, O-ReverseDelay, ~800 lines each). Those are
    built around the OTHER renderer family — measure-then-pin placement with an
    above/below flip and a help-toggle state — none of which exists here.

    ── THE FRAME IS 550 x 530, READ NOT GUESSED ────────────────────────────────

    Pinned to the SHIPPING size parsed out of PluginEditor.cpp's setSize(),
    never a default 1280x720: a clamp gate at the wrong viewport measures a page
    that has room and certifies nothing
    (pattern_tooltip_clamp_gate_viewport_sensitive). The LFO group sits at
    y 430+ inside a 530 px frame, so every tip down there flips ABOVE the cursor
    and then has to be clamped again — one flip is not enough (the O-Bass
    finding), and this is the page that shows it.

    ── AND FRENCH IS THE OTHER HALF ────────────────────────────────────────────

    French runs 15-20 % longer, wraps to more lines against the 230 px
    max-width cap, and GROWS THE TIP'S HEIGHT. A tip that fits in English can
    therefore overflow the bottom in French, which is why every assertion runs
    in both languages rather than in English with a French spot-check.

    Usage:
        node plugins/O-Freeze/tests/ui_tip_render_check.js
        node plugins/O-Freeze/tests/ui_tip_render_check.js --verbose

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

const PLUGIN  = 'O-Freeze';
const verbose = process.argv.includes('--verbose');

let failed = 0;
let passes = 0;

function check(cond, msg, detail) {
    if (cond) { ++passes; console.log(`  PASS: ${msg}`); }
    else      { ++failed; console.log(`  FAIL: ${msg}`); if (detail) console.log(`        ${detail}`); }
    return !!cond;
}

function note(msg) { console.log(`   ${msg}`); }

// ── the table, loaded as a MODULE rather than regex-scraped ─────────────────
//
// i18n.js is an ES module inside a package with no "type": "module", so a bare
// dynamic import() of it is parsed as CommonJS and throws on the first `export`.
// Copying it to a .mjs in a per-run mkdtemp is the honest fix: the gate then
// compares against the REAL exported objects rather than against a regex's idea
// of them. A fixture that mirrors the table would drift silently
// (pattern_test_fixture_mirrors_drift_silently).
async function loadTable() {
    const src = path.join(REPO_ROOT, 'plugins', PLUGIN, 'Source', 'ui', 'public', 'js', 'i18n.js');
    if (!fs.existsSync(src)) throw new Error(`i18n.js not found at ${src}`);

    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'ofreeze-i18n-'));
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
    note(`table: ${Object.keys(I18N).length} I18N entries, ${TIP_BINDINGS.length} TIP_BINDINGS rows\n`);

    if (!Array.isArray(TIP_BINDINGS) || TIP_BINDINGS.length === 0) {
        check(false, '[0] TIP_BINDINGS is non-empty — a render gate over zero bindings is vacuous');
        process.exit(1);
    }

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
            '[0] no uncaught page error during load (a TDZ throw takes this whole inline module)',
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
            // "it enters check-ui-labels' sweep" would be the natural thing to
            // write here and it is FALSE — measured on O-Bass. Un-hiding the
            // surface leaves check-ui-labels byte-identically green (a fixed box
            // at 0,0 has the same rect in both languages, so it neither moves
            // nor changes the visible element SET). What DOES catch it is
            // check-i18n assertion 10, boot-all-uis' text count — and this
            // assertion.
            check(surface.visibility === 'hidden' || surface.opacity === '0',
                '[0] the surface is HIDDEN at rest — un-hidden it becomes an unkeyed text node '
                + '(check-i18n assertion 10) and a permanent overlay '
                + `(visibility ${surface.visibility}, opacity ${surface.opacity})`);
            // #lang-select is itself an anchor and lives INSIDE #settings-popover
            // (z-index 61), so a tip that did not out-stack the panel would open
            // BEHIND the control that opened it.
            check(Number(surface.zIndex) > 61,
                `[0] the surface out-stacks #settings-popover (61) — z-index ${surface.zIndex}`);
        }

        // ── 1. EVERY TIP_BINDINGS SELECTOR RESOLVES ─────────────────────────
        //
        // applyI18n's own failure here is a console.warn, which boot-all-uis
        // prints and nothing fails on. A binding that finds no element is a
        // FAIL in this gate, not a warning.
        const resolution = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
            const el = document.querySelector(sel);
            if (!el) return { sel, key, found: false };
            const target = wrapper ? (el.closest(wrapper) || el) : el;
            const r = target.getBoundingClientRect();
            return {
                sel, key, found: true,
                wrapperResolved: wrapper ? !!el.closest(wrapper) : null,
                anchorTag: target.tagName.toLowerCase(),
                anchorId: target.id || '',
                hasTip: target.hasAttribute('data-tip'),
                hasTitle: target.hasAttribute('data-tip-title'),
                anchorIsSelf: target === el,
                w: r.width, h: r.height,
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
            if (verbose) note(`${r.sel} -> ${r.anchorTag}#${r.anchorId} `
                            + `${r.w.toFixed(1)}x${r.h.toFixed(1)}`
                            + (r.anchorIsSelf ? ' (self)' : ' (wrapper)'));
        }
        check(tipWarns.length === 0,
            '[1] applyI18n logged no "tip target not found" warning',
            tipWarns.slice(0, 3).join(' | '));

        // ── the driving loop ────────────────────────────────────────────────
        //
        // The twelve parameter anchors are hovered with the settings popover
        // CLOSED and the two chrome anchors with it OPEN, because #lang-select
        // does not exist to a pointer until the panel is open — it is inside a
        // [hidden] container. The gear is hovered in the open state too, so the
        // pass covers the state a user is actually in when they reach for it.
        const anchorsClosed = ['#freeze-button', '#reverse-toggle', '#mode-toggle',
                               '#threshold-knob', '#drift-knob', '#grain-size-knob',
                               '#grain-count-knob', '#detune-knob', '#mix-knob',
                               '#lfo-rate-knob', '#lfo-depth-knob', '#lfo-shape-toggle'];
        const anchorsOpen   = ['#gear-btn', '#lang-select', '#tips-toggle'];

        check(anchorsClosed.length + anchorsOpen.length === TIP_BINDINGS.length,
            `[1] the driving loop covers every binding — ${anchorsClosed.length} + `
            + `${anchorsOpen.length} vs ${TIP_BINDINGS.length} rows`);

        // THE TARGET HALF, asserted rather than assumed. Every binding on this
        // page is BARE — no wrapper — and that is only correct because each id'd
        // node is already the cell the user aims at. A .knob is a flex COLUMN
        // holding the 60 px visual, the caption and the readout, so it is ~60 px
        // wide and ~90 px tall; if a future edit re-pointed a binding at the
        // 4 px `circle.knob-vine` stroke instead, the anchor would collapse to a
        // sliver and the tip would become one nobody can open.
        //
        // SCOPED TO THE TWELVE PARAMETER ANCHORS, and the scope is the finding.
        // The first draft applied the floor to all fourteen and failed on both
        // chrome anchors for reasons that are not defects: #gear-btn is 22 x 22
        // by design (the page's own header button size, a Stage-K decision), and
        // #lang-select measures 0 x 0 here because it lives inside
        // #settings-popover[hidden] and has no box until the panel is open. A
        // floor those two must clear either has to be lowered until it stops
        // catching a stroke, or measured in a state they do not share. They are
        // driven with the panel OPEN in the loop below, where assertion 2 —
        // "hovering it SHOWS the tip" — is the stronger statement anyway: a
        // zero-box anchor cannot be hovered at all.
        const paramRes = resolution.filter(r => r.found && anchorsClosed.includes(r.sel));
        check(paramRes.length === anchorsClosed.length,
            `[1] all ${anchorsClosed.length} parameter anchors resolved — got ${paramRes.length}`);
        const tiny = paramRes.filter(r => r.w < 24 || r.h < 24);
        check(tiny.length === 0,
            '[1] every PARAMETER anchor is a real hover CELL, not an inner stroke — min 24 x 24 px',
            tiny.map(r => `${r.sel} ${r.w.toFixed(1)}x${r.h.toFixed(1)}`).join(', '));
        if (verbose) {
            const chrome = resolution.filter(r => r.found && anchorsOpen.includes(r.sel));
            note('chrome anchors, measured with the popover CLOSED: '
               + chrome.map(r => `${r.sel} ${r.w.toFixed(1)}x${r.h.toFixed(1)}`).join(', '));
        }

        async function setPopover(open) {
            const isOpen = await page.evaluate(() =>
                !document.getElementById('settings-popover').hidden);
            if (isOpen !== open) {
                await page.click('#gear-btn');
                await page.waitForTimeout(120);
            }
            return await page.evaluate(() => !document.getElementById('settings-popover').hidden);
        }

        async function hoverAnchor(sel, wrapper) {
            // Move somewhere neutral first so pointerover definitely fires on
            // the next move: a pointer already inside the anchor generates no
            // new pointerover, and the tip would be measured in whatever state
            // the PREVIOUS anchor left it — which is exactly how a "contains"
            // check passes on stale text. (2, 2) is the top-left corner of the
            // header bar, which carries no anchor.
            await page.mouse.move(2, 2);
            await page.waitForTimeout(60);

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
            // mid-flight value cannot be read as "not shown". O-Comp's first
            // keyboard control sampled 80 ms into a 120 ms fade, reported a
            // false "never opens", and the obvious response to that reading is
            // to delete the latch (pattern_quiesce_before_stimulus_in_async_ui_gates).
            await page.waitForTimeout(220);
            return { box, tip: await page.evaluate(READ_TIP) };
        }

        const drivenStates = [];

        for (const lang of ['en', 'fr', 'en']) {
            const pass = drivenStates.filter(s => s === lang).length === 0 ? '' : ' (return pass)';
            drivenStates.push(lang);
            console.log(`\n-- language: ${lang}${pass}`);

            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(150);

            for (const group of [{ open: false, list: anchorsClosed }, { open: true, list: anchorsOpen }]) {
                const state = await setPopover(group.open);
                check(state === group.open,
                    `[·] settings popover is ${group.open ? 'OPEN' : 'closed'} for this group — got `
                    + `${state ? 'open' : 'closed'}`);

                for (const sel of group.list) {
                    const b = TIP_BINDINGS.find(x => x[0] === sel);
                    if (!b) { check(false, `[·] ${sel} is in TIP_BINDINGS`); continue; }
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
                            : `rendered ${t.body.length} chars vs table ${entry.b.length} chars`);

                    // ── 4. INSIDE THE VIEWPORT, ALL FOUR EDGES ──────────────
                    const out = outsideViewport(t.rect, W, H);
                    check(out.length === 0,
                        `[4][${lang}] ${sel} tip rect is fully inside ${W} x ${H} `
                        + `(${t.rect.w.toFixed(1)} x ${t.rect.h.toFixed(1)} at `
                        + `${t.rect.left.toFixed(1)},${t.rect.top.toFixed(1)})`,
                        out.length ? out.join('; ') : null);

                    if (verbose)
                        note(`${lang} ${sel}: cursor ${got.box.x.toFixed(0)},${got.box.y.toFixed(0)} `
                           + `-> tip ${t.rect.left.toFixed(1)},${t.rect.top.toFixed(1)} `
                           + `${t.rect.w.toFixed(1)}x${t.rect.h.toFixed(1)}`);
                }
            }
        }

        // ── 5. FRENCH REALLY IS TALLER, and English really came back ────────
        //
        // Re-measured here rather than inferred from the loop: the point of
        // running both languages is that French wraps to more lines against the
        // 230 px max-width cap, and if it did NOT the two passes would be the
        // same measurement twice and assertion 4's French half would be
        // decoration.
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
        const grew = anchorsClosed.filter(s => hFr[s] > hEn[s] + 0.5);
        const same = anchorsClosed.filter(s => Math.abs(hFr[s] - hEn[s]) <= 0.5);
        console.log('');
        check(grew.length > 0,
            '[5] French GROWS at least one tip\'s height against the 230 px cap — '
            + anchorsClosed.map(s => `${s} ${hEn[s].toFixed(0)}->${hFr[s].toFixed(0)}`).join(', '),
            grew.length ? null
                : 'no tip grew: the fr pass measured the same boxes as en, so its clamp half is decoration');
        if (same.length) note(`${same.length} tip(s) the same height in both languages: ${same.join(', ')}`);

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const backEn = await hoverAnchor('#mix-knob', undefined);
        check(backEn && backEn.tip.title === I18N['tip.mix'].en.t
                     && backEn.tip.body  === I18N['tip.mix'].en.b,
            '[5] English comes back after the French pass — byte-equal again');

        // ── 6. THE NEGATIVE CONTROL ─────────────────────────────────────────
        //
        // Assertion 4 has passed at every anchor in both languages. That is
        // indistinguishable from an assertion 4 that CANNOT SEE an overflow
        // until one is planted and it reports.
        //
        // The plant is a DOM attribute write, not a file edit: applyI18n is the
        // only writer of data-tip, so `window.__setLanguage('en')` restores the
        // real value exactly and there is nothing to check out. A
        // `git checkout -- <file>` restore would take an uncommitted fix with
        // it — O-GrainScatter lost a whole edit that way.
        //
        // THE PLANT IS SIZED AGAINST THIS FRAME, NOT BY HABIT. O-Tremolo's 40x
        // plant (880 chars, ~390 px) FIT inside a 400 px frame with 384 px of
        // clamp room and reported nothing — a plant that fits is
        // indistinguishable from a gate that cannot see. Here: max-width 230 px
        // gives ~208 px of content, ~39 chars a line at 11 px Georgia, 15.4 px a
        // line; the frame allows 530 - 16 = 514 px, minus ~33 px of chrome and
        // title, so ~31 lines fit. 140 repeats is 2240 chars, roughly 57 lines,
        // ~880 px — comfortably past it. The assertion below also prints the
        // measured height, so a future reader can see the margin rather than
        // trust this arithmetic.
        console.log('\n-- negative control (assertion 4 harness-blindness)');
        const PLANT = ('overflow probe. ').repeat(140);
        await page.evaluate((body) => {
            document.querySelector('#drift-knob').setAttribute('data-tip', body);
        }, PLANT);

        const planted = await hoverAnchor('#drift-knob', undefined);
        const plantedOut = planted ? outsideViewport(planted.tip.rect, W, H) : ['tip did not render'];
        check(plantedOut.length > 0,
            '[6] a planted over-long body OVERFLOWS and assertion 4 reports it — '
            + `${plantedOut.join('; ') || 'NOTHING REPORTED'}`,
            plantedOut.length ? `planted tip ${planted.tip.rect.w.toFixed(1)} x `
                              + `${planted.tip.rect.h.toFixed(1)}, frame ${W} x ${H}`
                              : 'assertion 4 is BLIND — every [4] pass above is decoration');
        if (planted) note(`plant measured ${planted.tip.rect.w.toFixed(1)} x `
                        + `${planted.tip.rect.h.toFixed(1)} px in a ${W} x ${H} frame`);
        check(planted && planted.tip.body === PLANT,
            '[6] the plant actually reached the surface (a plant that never rendered proves nothing)');

        // restore from the TABLE, and prove the restore took
        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const restored = await hoverAnchor('#drift-knob', undefined);
        check(restored && restored.tip.body === I18N['tip.drift'].en.b,
            '[6] restored: the anchor carries the table body again, byte-equal');
        check(restored && outsideViewport(restored.tip.rect, W, H).length === 0,
            '[6] restored: assertion 4 is green again at the same anchor');

        // ══════════════════════════════ 7. THE FOCUS LATCH, BOTH HALVES ══
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
        // A STATIC regex for `lastInputWasPointer` would NOT do this job: the
        // declaration, the pointerdown write and the keydown clear all survive
        // deleting the guard clause, so every grep still matches (the O-Comp
        // finding). Only the behavioural control below discriminates.
        console.log('\n-- 7. the focus latch');

        await setPopover(false);
        await page.mouse.move(2, 2);
        await page.waitForTimeout(150);
        // BLUR FIRST, and this line is the whole reason the assertion below can
        // fail at all. setPopover() above leaves focus ON #gear-btn, and
        // clicking an ALREADY-FOCUSED element fires no focusin — so without this
        // the check reports "no tip after a click" for a page with no latch
        // whatsoever. The orchestrator's first version of this assertion passed
        // 125/125 with the latch deleted for exactly that reason, and O-Tremolo
        // then proved the blur is load-bearing by deleting both and going green.
        await page.evaluate(() => document.activeElement && document.activeElement.blur());
        await page.waitForTimeout(100);
        await page.click('#gear-btn');
        await page.waitForTimeout(300);
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
            return { shown, overlap, focused: document.activeElement ? document.activeElement.id : null };
        });
        check(!afterClick.shown,
            '[7] a POINTER click opens no tip — the latch suppresses the focusin arm'
            + (afterClick.overlap ? ` (it covered the popover by ${afterClick.overlap} px2)` : ''));
        // The control on the control: if the click did not move focus onto the
        // gear, the focusin arm was never exercised and the assertion above is
        // decoration whatever it reported.
        check(afterClick.focused === 'gear-btn',
            `[7] the click really did focus #gear-btn — otherwise no focusin fired and the `
            + `assertion above proves nothing (focus is on "${afterClick.focused}")`);

        // The keyboard half. A real tab-ring walk, not a programmatic .focus():
        // Chromium reports :focus-visible false for a .focus() that follows a
        // click, and .focus() on an already-focused element fires no event at
        // all — either one would report "no tip" and record that as correct.
        //
        // The ring here is short: #gear-btn, then #lang-select once the panel is
        // open. Escape closes the panel and returns focus to the gear, so the
        // walk starts from a blurred document instead.
        await page.evaluate(() => document.activeElement && document.activeElement.blur());
        await page.waitForTimeout(150);
        let kbHit = null;
        for (let i = 1; i <= 20; i++) {
            await page.keyboard.press('Tab');
            // Past the 0.12 s fade, for the same reason hoverAnchor waits 220 ms.
            await page.waitForTimeout(220);
            const r = await page.evaluate(() => {
                const t = document.getElementById('tooltip');
                const cs = getComputedStyle(t);
                return { shown: cs.visibility !== 'hidden' && cs.opacity !== '0',
                         text: (t.textContent || '').trim(),
                         on: document.activeElement ? (document.activeElement.id || document.activeElement.className) : null };
            });
            if (r.shown && r.text) { kbHit = { press: i, ...r }; break; }
        }
        check(kbHit !== null,
            '[7] a KEYBOARD tab still opens a tip — the accessibility half survives the latch'
            + (kbHit ? ` (tab #${kbHit.press} on ${kbHit.on})` : ' — none in 20 tabs'));
        await page.keyboard.press('Escape');
        await page.waitForTimeout(150);
        const afterEsc = await page.evaluate(() => {
            const t = document.getElementById('tooltip');
            const cs = getComputedStyle(t);
            return cs.visibility !== 'hidden' && cs.opacity !== '0';
        });
        check(!afterEsc, '[7] Escape hides the tip');

        // ── housekeeping ────────────────────────────────────────────────────
        console.log('');
        check(pageErrors.length === 0, '[8] no uncaught page error across the whole sweep',
            pageErrors.slice(0, 3).join(' | '));
        check(consoleErrors.length === 0, '[8] no console.error across the whole sweep',
            consoleErrors.slice(0, 3).join(' | '));
        check(misses.length === 0, '[8] every requested resource was served',
            misses.slice(0, 5).join(', '));

        // A native title= would render a second, untranslated OS tooltip
        // competing with this surface (contract §4). Repo-wide it is 0 today.
        const nativeTitles = await page.evaluate(() =>
            document.querySelectorAll('[title]').length);
        check(nativeTitles === 0,
            `[8] zero native title= attributes on the page — got ${nativeTitles}`);

    } finally {
        await browser.close();
        await close();
        fs.rmSync(built.root, { recursive: true, force: true });
    }

    console.log(`\n${failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`}`
              + `   (${passes} passed)`);
    process.exit(failed === 0 ? 0 : 1);
})().catch((e) => { console.error(e); process.exit(1); });
