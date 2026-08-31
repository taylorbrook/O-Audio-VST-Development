/*
   This file is part of O-Bass, an Ouaricon Audio plugin.
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

    ui_tip_render_check.js — O-Bass hover-help, RENDERED.

    ── WHY THIS FILE EXISTS ────────────────────────────────────────────────────

    NO OTHER GATE IN THIS REPO CAN SEE A RENDERED TOOLTIP.

      check-i18n      reads the table statically. Its assertion 2 counts
                      TIP_BINDINGS rows and checks each key exists in I18N. Five
                      bindings pointing at five selectors that match nothing, on
                      a page with no renderer at all, is a PASS.
      check-ui-labels has no tooltip awareness whatsoever — it sweeps
                      [data-i18n] elements and never looks at data-tip.
      boot-all-uis    counts aria-label and title. It never counts data-tip.

    So authoring five tooltip bodies into i18n.js and binding them, with no other
    change, would have shipped five invisible strings past three green gates.
    v1.4.0 of this plugin had NO #tooltip node, NO .tooltip rule and NO hover
    handler, which is exactly the state in which that failure is silent.

    This gate is the seat where that failure becomes loud. Assertion 2 below is
    the whole point: a tip that never appeared is a FAIL, not a warning.

    ── WHAT IT IS NOT ──────────────────────────────────────────────────────────

    It is deliberately NOT a port of the three committed ui_tooltip_clamp_check.js
    gates (O-Tapestop, O-Bitrot, O-ReverseDelay, ~800 lines each). Those are built
    around the OTHER renderer family — measure-then-pin placement with an
    above/below flip and a help-toggle state — and none of that exists here. A
    port would have been 40 KB of assertions about a mechanism this page does not
    have.

    ── THE FRAME IS 420 x 320, THE SMALLEST IN BATCH M1 ────────────────────────

    Which makes assertion 4 (the four-edge clamp) the assertion that actually
    fires here rather than a formality. A tip opened on the OUTPUT knob wants to
    start 208 px to the right of a cursor already at x ~300 on a 420 px frame; one
    opened on the gear wants to start 16 px below a cursor at y ~304 on a 320 px
    frame. Both must flip. The viewport is pinned to the SHIPPING size read out of
    PluginEditor.cpp, never a default 1280x720 — a clamp gate at the wrong
    viewport measures a page that has room and certifies nothing
    (pattern_tooltip_clamp_gate_viewport_sensitive).

    ── AND FRENCH IS THE OTHER HALF ────────────────────────────────────────────

    French runs 15-20 % longer, wraps to more lines against the 208 px max-width
    cap, and GROWS THE TIP'S HEIGHT. A tip that fits in English can therefore
    overflow the bottom in French, which is why every assertion runs in both
    languages rather than in English with a French spot-check.

    Usage:
        node plugins/O-Bass/tests/ui_tip_render_check.js
        node plugins/O-Bass/tests/ui_tip_render_check.js --verbose

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

const PLUGIN  = 'O-Bass';
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
// compares against the REAL exported objects — including the string
// concatenations the bodies are authored with — rather than against a regex's
// idea of them. A fixture that mirrors the table would drift silently
// (pattern_test_fixture_mirrors_drift_silently).
async function loadTable() {
    const src = path.join(REPO_ROOT, 'plugins', PLUGIN, 'Source', 'ui', 'public', 'js', 'i18n.js');
    if (!fs.existsSync(src)) throw new Error(`i18n.js not found at ${src}`);

    const dir = fs.mkdtempSync(path.join(os.tmpdir(), `obass-i18n-`));
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
// The body is read as the concatenation of the surface's own TEXT NODES, and the
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
        // `surface && typeof surface === 'object'`, not `!== null`: an evaluate
        // that returns an unserialisable value yields undefined, and `undefined
        // !== null` is a PASS over a surface nobody read. That false pass really
        // happened on this gate's first run.
        check(surface && typeof surface === 'object',
            '[0] the #tooltip surface exists in the DOM and was READ (not undefined)');
        if (surface && typeof surface === 'object') {
            check(surface.position === 'fixed',
                `[0] the surface is position: fixed — got ${surface.position}`);
            check(surface.pointerEvents === 'none',
                `[0] the surface is pointer-events: none (or it steals the hover keeping it open) `
                + `— got ${surface.pointerEvents}`);
            // "enters check-ui-labels' sweep" would be the natural thing to write
            // here and it is FALSE — measured. Un-hiding this surface leaves
            // check-ui-labels byte-identically green (a fixed box at 0,0 has the
            // same rect in both languages, so it neither moves nor changes the
            // visible element SET). What DOES catch it is check-i18n assertion 10
            // and boot-all-uis' text count — and this assertion.
            check(surface.visibility === 'hidden' || surface.opacity === '0',
                `[0] the surface is HIDDEN at rest — un-hidden it becomes an unkeyed `
                + `text node (check-i18n assertion 10) and a permanent overlay `
                + `(visibility ${surface.visibility}, opacity ${surface.opacity})`);
        }

        // ── 1. EVERY TIP_BINDINGS SELECTOR RESOLVES ─────────────────────────
        //
        // applyI18n's own failure here is a console.warn, which boot-all-uis
        // prints and nothing fails on. A binding that finds no element is a FAIL
        // in this gate, not a warning.
        const resolution = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
            const el = document.querySelector(sel);
            if (!el) return { sel, key, found: false };
            const target = wrapper ? (el.closest(wrapper) || el) : el;
            return {
                sel, key, found: true,
                wrapperResolved: wrapper ? !!el.closest(wrapper) : null,
                anchorTag: target.tagName.toLowerCase(),
                anchorClass: target.className || '',
                hasTip: target.hasAttribute('data-tip'),
                hasTitle: target.hasAttribute('data-tip-title'),
                anchorIsSelf: target === el,
            };
        }), TIP_BINDINGS.map(b => [b[0], b[1], b[2] || null]));

        for (const r of resolution) {
            check(r.found, `[1] selector resolves: ${r.sel}  (key ${r.key})`);
            if (!r.found) continue;
            check(r.hasTip && r.hasTitle,
                `[1] applyI18n wrote data-tip + data-tip-title onto the anchor for ${r.sel}`);
            if (verbose) note(`${r.sel} -> ${r.anchorTag}.${r.anchorClass}`
                            + (r.anchorIsSelf ? ' (self)' : ' (wrapper)'));
        }
        check(tipWarns.length === 0,
            '[1] applyI18n logged no "tip target not found" warning',
            tipWarns.slice(0, 3).join(' | '));

        // ── the driving loop ────────────────────────────────────────────────
        //
        // The three knobs are hovered with the settings popover CLOSED and the
        // two chrome anchors with it OPEN, because #lang-select does not exist
        // to a pointer until the panel is open — it is inside a [hidden]
        // container. The gear is hovered in the open state too, so the pass
        // covers the state a user is actually in when they reach for it.
        const anchorsClosed = ['#frequencyKnob', '#enhanceKnob', '#outputKnob'];
        const anchorsOpen   = ['#gear-btn', '#lang-select'];

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
            // check passes on stale text.
            await page.mouse.move(W - 2, 2);
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
            // mid-flight value cannot be read as "not shown".
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
        // max-width cap, and if it did NOT the two passes would be the same
        // measurement twice and assertion 4's French half would be decoration.
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
            `[5] French GROWS at least one tip's height against the 208 px cap — `
            + anchorsClosed.map(s => `${s} ${hEn[s].toFixed(0)}->${hFr[s].toFixed(0)}`).join(', '),
            grew.length ? null
                : 'no tip grew: the fr pass measured the same boxes as en, so its clamp half is decoration');
        if (same.length) note(`${same.length} tip(s) the same height in both languages: ${same.join(', ')}`);

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const backEn = await hoverAnchor('#outputKnob', '.knob-container');
        check(backEn && backEn.tip.title === I18N['tip.output'].en.t
                     && backEn.tip.body  === I18N['tip.output'].en.b,
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
        // `git checkout -- <file>` restore would take an uncommitted fix with it
        // — O-GrainScatter lost a whole edit that way.
        //
        // The plant is TALL, not wide: max-width: 208px means a long body wraps
        // rather than running off the right edge, so the reachable overflow on
        // this renderer is vertical. A body that cannot fit in 320 - 16 px of
        // frame is one the clamp is unable to place, and assertion 4 must say so.
        console.log('\n-- negative control (assertion 4 harness-blindness)');
        const PLANT = ('overflow probe. ').repeat(60);
        await page.evaluate((body) => {
            const el = document.querySelector('#outputKnob').closest('.knob-container');
            el.setAttribute('data-tip', body);
        }, PLANT);

        const planted = await hoverAnchor('#outputKnob', '.knob-container');
        const plantedOut = planted ? outsideViewport(planted.tip.rect, W, H) : ['tip did not render'];
        check(plantedOut.length > 0,
            `[6] a planted over-long body OVERFLOWS and assertion 4 reports it — `
            + `${plantedOut.join('; ') || 'NOTHING REPORTED'}`,
            plantedOut.length ? `planted tip ${planted.tip.rect.w.toFixed(1)} x `
                              + `${planted.tip.rect.h.toFixed(1)}, frame ${W} x ${H}`
                              : 'assertion 4 is BLIND — every [4] pass above is decoration');
        check(planted && planted.tip.body === PLANT,
            '[6] the plant actually reached the surface (a plant that never rendered proves nothing)');

        // restore from the TABLE, and prove the restore took
        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const restored = await hoverAnchor('#outputKnob', '.knob-container');
        check(restored && restored.tip.body === I18N['tip.output'].en.b,
            '[6] restored: the anchor carries the table body again, byte-equal');
        check(restored && outsideViewport(restored.tip.rect, W, H).length === 0,
            '[6] restored: assertion 4 is green again at the same anchor');

        // ── housekeeping ────────────────────────────────────────────────────
        console.log('');
        check(pageErrors.length === 0, '[7] no uncaught page error across the whole sweep',
            pageErrors.slice(0, 3).join(' | '));
        check(consoleErrors.length === 0, '[7] no console.error across the whole sweep',
            consoleErrors.slice(0, 3).join(' | '));
        check(misses.length === 0, '[7] every requested resource was served',
            misses.slice(0, 5).join(', '));

        // A native title= would render a second, untranslated OS tooltip
        // competing with this surface (contract §4). Repo-wide it is 0 today.
        const nativeTitles = await page.evaluate(() =>
            document.querySelectorAll('[title]').length);
        check(nativeTitles === 0,
            `[7] zero native title= attributes on the page — got ${nativeTitles}`);

    } finally {
        await browser.close();
        await close();
        fs.rmSync(built.root, { recursive: true, force: true });
    }

    console.log(`\n${failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`}`
              + `   (${passes} passed)`);
    process.exit(failed === 0 ? 0 : 1);
})().catch((e) => { console.error(e); process.exit(1); });
