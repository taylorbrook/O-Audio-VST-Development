/*
   This file is part of O-AnalogEQ, an Ouaricon Audio plugin.
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

    ui_tip_render_check.js — O-AnalogEQ hover-help, RENDERED.

    ── WHY THIS FILE EXISTS ────────────────────────────────────────────────────

    NO OTHER GATE IN THIS REPO CAN SEE A RENDERED TOOLTIP.

      check-i18n      reads the table statically. Its assertion 2 counts
                      TIP_BINDINGS rows and checks each key exists in I18N.
                      Thirteen bindings pointing at thirteen selectors that match
                      nothing, on a page with no renderer at all, is a PASS.
      check-ui-labels has no tooltip awareness whatsoever — it sweeps
                      [data-i18n] elements and never looks at data-tip.
      boot-all-uis    counts aria-label and title. It never counts data-tip.

    v1.2.0 of this plugin had NO #tooltip node, NO .tooltip rule and NO hover
    handler, which is exactly the state in which authoring thirteen bodies and
    binding them ships thirteen invisible strings past three green gates.

    This gate is the seat where that failure becomes loud. Assertion 2 below is
    the whole point: a tip that never appeared is a FAIL, not a warning.

    ── WHAT IT IS NOT ──────────────────────────────────────────────────────────

    It is deliberately NOT a port of the three committed ui_tooltip_clamp_check.js
    gates (O-Tapestop, O-Bitrot, O-ReverseDelay, ~800 lines each). Those are built
    around the OTHER renderer family — measure-then-pin placement with an
    above/below flip and a help-toggle state — and none of that exists here.

    ── THE FRAME IS 920 x 220, AND THE HEIGHT IS THE POOR AXIS ─────────────────

    204 px of usable height once the renderer's 8 px margins are taken off, in a
    page whose controls run from y=52 (the band captions) to y=193 (the bottom
    of the Q toggles). So assertion 4 (the four-edge clamp) is not a formality
    here: the naive y + 16 offset puts the tip's bottom past the frame at most
    anchors, and on the HIGH anchors a flip to the other side of the cursor
    lands above the top rail. [4b] below turns "the clamp is the normal path on
    this frame" into a count rather than a claim, and it separately counts the
    placements that are outside on BOTH sides of the flip and therefore land on
    the SECOND clamp — the assertion O-Bass's carried trap 3 exists for.

    The viewport is pinned to the SHIPPING size read out of PluginEditor.cpp,
    never a default 1280x720 — a clamp gate at the wrong viewport measures a
    page that has room and certifies nothing
    (pattern_tooltip_clamp_gate_viewport_sensitive).

    ── AND FRENCH IS THE OTHER HALF ────────────────────────────────────────────

    French runs 15-20 % longer, wraps to more lines against the 420 px max-width
    cap, and GROWS THE TIP'S HEIGHT. Every assertion therefore runs in both
    languages rather than in English with a French spot-check.

    Usage:
        node plugins/O-AnalogEQ/tests/ui_tip_render_check.js
        node plugins/O-AnalogEQ/tests/ui_tip_render_check.js --verbose

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

const PLUGIN  = 'O-AnalogEQ';
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

    const dir = fs.mkdtempSync(path.join(os.tmpdir(), `oanalogeq-i18n-`));
    const dst = path.join(dir, 'i18n.mjs');
    fs.copyFileSync(src, dst);
    try {
        return await import(pathToFileURL(dst).href);
    } finally {
        fs.rmSync(dir, { recursive: true, force: true });
    }
}

// ── the max-width cap, PARSED out of index.html rather than retyped ─────────
//
// The line-count arithmetic the copy is authored against is a function of the
// cap; a gate carrying its own copy of the number would keep agreeing with
// itself after somebody changed the CSS. It is also what the negative control's
// plant is SIZED against below — a plant sized by habit is a control that
// cannot fail (O-Tremolo's 40x plant fit inside a 400 px frame and reported
// nothing).
function readTipMaxWidth() {
    const html = fs.readFileSync(
        path.join(REPO_ROOT, 'plugins', PLUGIN, 'Source', 'ui', 'public', 'index.html'), 'utf8');
    const block = html.match(/\.tooltip\s*\{([^}]*)\}/);
    if (!block) return null;
    const m = block[1].match(/max-width:\s*([0-9.]+)px/);
    return m ? parseFloat(m[1]) : null;
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
    const MARGIN = 8;                      // the renderer's own clamp margin
    note(`shipping frame from PluginEditor.cpp setSize(): ${W} x ${H}`);
    note(`usable height for a tip: ${H} - 2 x ${MARGIN} = ${H - 2 * MARGIN} px`);

    const CAP = readTipMaxWidth();
    note(`tooltip max-width parsed from index.html: ${CAP === null ? 'NOT FOUND' : CAP + 'px'}`);

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
            '[0] no uncaught page error during load (a TDZ throw takes this whole inline module, '
            + 'and on this plugin that is the ENTIRE UI)',
            pageErrors.slice(0, 3).join(' | '));

        check(CAP !== null && CAP > 0,
            `[0] .tooltip carries a max-width cap — without one French runs off the right edge `
            + `instead of wrapping (parsed ${CAP}px)`);

        // ── the surface itself ──────────────────────────────────────────────
        const surface = await page.evaluate(READ_TIP);
        // `surface && typeof surface === 'object'`, not `!== null`: an evaluate
        // that returns an unserialisable value yields undefined, and `undefined
        // !== null` is a PASS over a surface nobody read. That false pass really
        // happened on the O-Bass gate's first run.
        check(surface && typeof surface === 'object',
            '[0] the #tooltip surface exists in the DOM and was READ (not undefined)');
        if (surface && typeof surface === 'object') {
            check(surface.position === 'fixed',
                `[0] the surface is position: fixed — got ${surface.position}`);
            check(surface.pointerEvents === 'none',
                `[0] the surface is pointer-events: none (or it steals the hover keeping it open) `
                + `— got ${surface.pointerEvents}`);
            // "enters check-ui-labels' sweep" would be the natural thing to write
            // here and it is FALSE — measured on O-Bass. Un-hiding this surface
            // leaves check-ui-labels byte-identically green. What DOES catch it
            // is check-i18n assertion 10, boot-all-uis' text count, and this
            // assertion.
            check(surface.visibility === 'hidden' || surface.opacity === '0',
                `[0] the surface is HIDDEN at rest — un-hidden it becomes an unkeyed `
                + `text node (check-i18n assertion 10) and a permanent overlay `
                + `(visibility ${surface.visibility}, opacity ${surface.opacity})`);
            check(surface.body === '' && surface.title === '',
                `[0] the surface is EMPTY at rest — authored text in the markup would ship `
                + `one language of a tooltip that applyI18n never rewrote`);
            // The settings popover is z-index 61 inside a z-index 60 cluster.
            // A tip painted UNDER the panel it describes is invisible to any
            // check reading only `visibility` (O-Texture's fern, z-index 1000).
            check(parseInt(surface.zIndex, 10) > 61,
                `[0] the surface paints ABOVE .settings-popover (61) — got z-index ${surface.zIndex}`);
        }

        // ── 1. EVERY TIP_BINDINGS SELECTOR RESOLVES ─────────────────────────
        //
        // applyI18n's own failure here is a console.warn, which boot-all-uis
        // prints and nothing fails on. A binding that finds no element is a FAIL
        // in this gate, not a warning.
        //
        // T17's "bind to the ids the UI already uses" is wrong here TWICE, and
        // the two halves are asserted separately because they fail
        // independently. The four dual knobs have ids — #lf_freq_knob and
        // #lf_gain_knob — and both are pointer-events: none, so a tip bound to
        // either could never open; the addressable node is the container. And
        // that container is 65 x 65 inside an 85 x 85 wrapper that carries the
        // frequency scale, so wrapperResolved below is what proves the
        // closest('.dual-knob-wrapper') walk actually landed on the bigger cell
        // rather than silently falling back to the container itself.
        const resolution = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
            const el = document.querySelector(sel);
            if (!el) return { sel, key, found: false };
            const target = wrapper ? (el.closest(wrapper) || el) : el;
            const r = target.getBoundingClientRect();
            return {
                sel, key, found: true,
                wrapperAsked: !!wrapper,
                wrapperResolved: wrapper ? !!el.closest(wrapper) : null,
                anchorTag: target.tagName.toLowerCase(),
                anchorClass: (typeof target.className === 'string' ? target.className : ''),
                anchorId: target.id || '',
                hasTip: target.hasAttribute('data-tip'),
                hasTitle: target.hasAttribute('data-tip-title'),
                anchorIsSelf: target === el,
                selfPointerEvents: getComputedStyle(el).pointerEvents,
                box: { x: r.left, y: r.top, w: r.width, h: r.height },
            };
        }), TIP_BINDINGS.map(b => [b[0], b[1], b[2] || null]));

        for (const r of resolution) {
            check(r.found, `[1] selector resolves: ${r.sel}  (key ${r.key})`);
            if (!r.found) continue;
            check(r.hasTip && r.hasTitle,
                `[1] applyI18n wrote data-tip + data-tip-title onto the anchor for ${r.sel}`);
            if (r.wrapperAsked)
                check(r.wrapperResolved && !r.anchorIsSelf,
                    `[1] ${r.sel} anchors on its WRAPPER, not the container — the hover cell is `
                    + `${r.box.w.toFixed(0)} x ${r.box.h.toFixed(0)}`);
            if (verbose) note(`${r.sel} -> ${r.anchorTag}.${r.anchorClass}`
                            + (r.anchorIsSelf ? ' (self)' : ' (wrapper)')
                            + ` at ${r.box.x.toFixed(0)},${r.box.y.toFixed(0)} `
                            + `${r.box.w.toFixed(0)}x${r.box.h.toFixed(0)}`);
        }
        check(tipWarns.length === 0,
            '[1] applyI18n logged no "tip target not found" warning',
            tipWarns.slice(0, 3).join(' | '));

        // ── 1b. EVERY BINDING LANDS ON A DISTINCT NODE ──────────────────────
        //
        // applyI18n writes onto whatever the selector resolves to, so two rows
        // resolving to ONE node mean the second silently overwrites the first —
        // while check-i18n cheerfully reports two bound tips. That is the trap
        // that forced this plugin's four dual knobs to carry ONE tip each for
        // two parameters; this assertion is what proves the resulting table has
        // no accidental instance of it left.
        const distinct = await page.evaluate((bindings) => {
            const nodes = bindings.map(([sel, , wrapper]) => {
                const el = document.querySelector(sel);
                if (!el) return null;
                return wrapper ? (el.closest(wrapper) || el) : el;
            });
            const dupes = [];
            for (let i = 0; i < nodes.length; i++)
                for (let j = i + 1; j < nodes.length; j++)
                    if (nodes[i] && nodes[i] === nodes[j]) dupes.push([bindings[i][0], bindings[j][0]]);
            return { count: nodes.filter(Boolean).length, dupes };
        }, TIP_BINDINGS.map(b => [b[0], b[1], b[2] || null]));
        check(distinct.dupes.length === 0,
            `[1b] all ${TIP_BINDINGS.length} bindings land on DISTINCT nodes — a second row on one `
            + `node overwrites the first while check-i18n reports two bound tips`,
            distinct.dupes.map(d => d.join(' == ')).join('; '));

        // ── 1c. THE CHROME BINDS BARE, AND THAT MATTERS ─────────────────────
        //
        // #gear-btn and #lang-select share the ancestor .settings-cluster, so a
        // wrapper walk on either resolves BOTH to the cluster and hovering the
        // language selector opens the GEAR's tip (O-Comp hit exactly this).
        // The rows are bare on purpose; this asserts the reason is still true.
        const chromeShare = await page.evaluate(() => {
            const g = document.getElementById('gear-btn');
            const l = document.getElementById('lang-select');
            if (!g || !l) return null;
            const gc = g.closest('.settings-cluster');
            const lc = l.closest('.settings-cluster');
            return { shared: !!gc && gc === lc };
        });
        check(chromeShare && chromeShare.shared,
            '[1c] #gear-btn and #lang-select DO share .settings-cluster — which is why both '
            + 'TIP_BINDINGS rows are bare, and this assertion is what keeps that reason honest');
        check(TIP_BINDINGS.filter(b => /^#(gear-btn|lang-select)$/.test(b[0])).every(b => !b[2]),
            '[1c] neither chrome row carries a wrapper');

        // ── the driving loop ────────────────────────────────────────────────
        //
        // The eleven parameter anchors are hovered with the settings popover
        // CLOSED and the two chrome anchors with it OPEN, because #lang-select
        // does not exist to a pointer until the panel is open — it is inside a
        // [hidden] container. The gear is hovered in the open state too, so the
        // pass covers the state a user is actually in when they reach for it.
        const anchorsOpen   = ['#gear-btn', '#lang-select', '#tips-toggle'];
        const anchorsClosed = TIP_BINDINGS.map(b => b[0]).filter(s => !anchorsOpen.includes(s));
        check(anchorsClosed.length + anchorsOpen.length === TIP_BINDINGS.length,
            `[1] every binding is driven below — ${anchorsClosed.length} parameter + `
            + `${anchorsOpen.length} chrome vs ${TIP_BINDINGS.length} row(s)`);

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
            // check passes on stale text. (W-2, 2) is the top-right corner,
            // which on this page is bare paper beside the preset bar.
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
            // mid-flight value cannot be read as "not shown" — the artefact that
            // made O-Comp's first keyboard probe report a false "never opens".
            await page.waitForTimeout(220);
            return { box, tip: await page.evaluate(READ_TIP) };
        }

        const heights = { en: {}, fr: {} };
        const placement = [];
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
                    // words. Equality is the only test that catches it, and on
                    // this page eight of the thirteen bodies are near-identical
                    // sentences differing by one band name.
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

                    if (drivenStates.length <= 2) {
                        heights[lang][sel] = t.rect.h;
                        placement.push({
                            lang, sel,
                            cx: got.box.x, cy: got.box.y,
                            x: t.rect.left, y: t.rect.top, w: t.rect.w, h: t.rect.h,
                            // The naive placement the renderer would have used with
                            // no clamp at all, recorded so "the clamp is the normal
                            // path here" is a number rather than a claim.
                            naiveX: got.box.x + 14,
                            naiveY: got.box.y + 16,
                            flippedX: got.box.x - t.rect.w - 14,
                            flippedY: got.box.y - t.rect.h - 12,
                        });
                    }

                    if (verbose)
                        note(`${lang} ${sel}: cursor ${got.box.x.toFixed(0)},${got.box.y.toFixed(0)} `
                           + `-> tip ${t.rect.left.toFixed(1)},${t.rect.top.toFixed(1)} `
                           + `${t.rect.w.toFixed(1)}x${t.rect.h.toFixed(1)}`);
                }
            }
        }

        // ── 4b. THE CLAMP REALLY IS THE NORMAL PATH ON THIS FRAME ───────────
        //
        // Assertion 4 passing at every anchor is also what a page with acres of
        // room would report. This turns "the four-edge clamp is not an edge case
        // at 920 x 220" into a count: how many placements would have been
        // OUTSIDE the frame under the naive cursor offset, and how many are
        // STILL outside after one flip and therefore land on the second clamp.
        console.log('');
        const naiveOutY = placement.filter(p => p.naiveY + p.h > H - MARGIN);
        const flipOutY  = placement.filter(p => p.naiveY + p.h > H - MARGIN && p.flippedY < MARGIN);
        const naiveOutX = placement.filter(p => p.naiveX + p.w > W - MARGIN);
        const flipOutX  = placement.filter(p => p.naiveX + p.w > W - MARGIN && p.flippedX < MARGIN);
        check(naiveOutY.length > 0,
            `[4b] ${naiveOutY.length}/${placement.length} placement(s) overflow the naive cursor `
            + `offset VERTICALLY and are placed by the flip — the clamp is a live path on this `
            + `frame, not dead code`);
        note(`horizontal: ${naiveOutX.length}/${placement.length} would overflow the right edge under `
           + `the naive x + 14 offset, ${flipOutX.length} of those still outside after the flip `
           + `(920 px is this page's rich axis)`);

        // ── THE DISPATCH'S "THIS IS M2's O-CHORUS" IS FALSE, MEASURED ───────
        //
        // O-Chorus at 700 x 125 reports 20/20 naive overflows with 17 outside on
        // BOTH sides of the flip; O-Bass at 420 x 320 places every anchor by
        // flipping. This page does neither, and the reason is arithmetic rather
        // than luck: 220 px is short, but a 204 px well against tips that run
        // 51.1 to 78.1 px leaves 126 px of headroom, and the anchors sit at
        // y 62..184 rather than filling the frame. So the SHIPPED copy never
        // reaches the second clamp on either axis.
        //
        // That is a fact about the copy, not about the renderer, and it is
        // exactly the state in which the second clamp line ships as untested
        // code that a later French edit would be the first to exercise. [4c]
        // below drives it deliberately instead of asserting the shipped copy
        // happens to.
        note(`the SHIPPED copy is outside on BOTH sides of the vertical flip at `
           + `${flipOutY.length}/${placement.length} placements — a MEASUREMENT, not a `
           + `requirement. [4c] drives the second clamp deliberately rather than relying on it.`);
        const tallest = placement.reduce((a, p) => (p.h > a.h ? p : a), placement[0]);
        note(`tallest tip anywhere: ${tallest.lang} ${tallest.sel} at ${tallest.h.toFixed(1)} px — `
           + `${(H - 2 * MARGIN - tallest.h).toFixed(1)} px of headroom in the ${H - 2 * MARGIN} px well`);
        const widest = placement.reduce((a, p) => (p.w > a.w ? p : a), placement[0]);
        note(`widest tip anywhere: ${widest.lang} ${widest.sel} at ${widest.w.toFixed(1)} px against `
           + `the ${CAP} px cap`);

        // ── 4c. THE SECOND CLAMP, DRIVEN DELIBERATELY ───────────────────────
        //
        // "One flip is not enough — the flipped result needs clamping again" is
        // O-Bass's carried trap 3 and it is the line most likely to be got
        // wrong on a short frame. On THIS page the shipped copy never reaches
        // it ([4b] above), so asserting it against the shipped tips would be
        // decoration. It is driven instead.
        //
        // The band is arithmetic, computed from the anchor rather than guessed.
        // For an anchor whose centre is at cy, a tip of height h is:
        //     outside BELOW the cursor when   cy + 16 + h  >  H - M
        //     outside ABOVE the cursor when   cy - h - 12  <  M
        // Both are true together only for h in a window, and #lf_on's centre at
        // y=62.5 in a 220 px frame gives roughly 133.5 < h < 204. A plant whose
        // height lands inside that window is outside on BOTH sides of the flip,
        // so the ONLY thing that can place it inside the frame is the second
        // clamp. If that line were deleted the tip would be placed at
        // 50.5 - h, i.e. off the top of the page by ~100 px.
        //
        // The plant is SEARCHED for rather than sized by habit: a plant that
        // misses the window low is placed by the flip and proves nothing, and
        // one that misses high cannot be placed at all (which is [6]'s job, not
        // this one). O-Tremolo's 40x plant fit its frame and reported nothing.
        {
            console.log('\n-- 4c. the second clamp, driven');
            await setPopover(false);
            await page.evaluate(() => window.__setLanguage('en'));
            await page.waitForTimeout(150);

            const cy = await page.evaluate(() => {
                const r = document.getElementById('lf_on').getBoundingClientRect();
                return r.top + r.height / 2;
            });
            const loH = (H - MARGIN) - (cy + 16);      // taller than this: below fails
            const hiH = (cy - 12) - MARGIN;            // taller than this: above fails
            note(`#lf_on centre y=${cy.toFixed(1)} -> a tip is outside BOTH ways when its height is `
               + `in (${loH.toFixed(1)}, ${H - 2 * MARGIN}] px (above already fails past `
               + `${hiH.toFixed(1)} px)`);

            let hit = null;
            for (let reps = 20; reps <= 90; reps += 4) {
                const body = 'second clamp probe. '.repeat(reps);
                await page.evaluate((b) =>
                    document.getElementById('lf_on').setAttribute('data-tip', b), body);
                const got = await hoverAnchor('#lf_on');
                if (!got) break;
                const h = got.tip.rect.h;
                if (h > loH && h <= H - 2 * MARGIN) {
                    hit = { reps, chars: body.length, h, rect: got.tip.rect, cy };
                    break;
                }
            }

            check(hit !== null,
                `[4c] a plant was found whose height lands in the both-sides-outside window`
                + (hit ? ` — ${hit.chars} chars, ${hit.h.toFixed(1)} px tall` : ' — none in 20..90 reps'),
                hit ? null : 'without one, the second clamp is not being exercised at all');

            if (hit) {
                const naiveBottom = hit.cy + 16 + hit.h;
                const flippedTop  = hit.cy - hit.h - 12;
                check(naiveBottom > H - MARGIN && flippedTop < MARGIN,
                    `[4c] the plant really IS outside on both sides — naive bottom `
                    + `${naiveBottom.toFixed(1)} past ${H - MARGIN}, flipped top `
                    + `${flippedTop.toFixed(1)} above ${MARGIN}. A plant outside on only ONE side `
                    + `is placed by the flip and says nothing about the second clamp`);
                const out = outsideViewport(hit.rect, W, H);
                check(out.length === 0,
                    `[4c] and the FLIPPED RESULT IS RE-CLAMPED, so it is inside anyway — top `
                    + `${hit.rect.top.toFixed(1)}, bottom ${hit.rect.bottom.toFixed(1)} in a ${H} px `
                    + `frame. Unclamped it would sit at ${flippedTop.toFixed(1)}, `
                    + `${Math.abs(flippedTop - MARGIN).toFixed(0)} px off the top of the page`,
                    out.length ? out.join('; ') : null);

                // ── WHICH LINE ACTUALLY DID IT, and the answer is not the one
                // the reference family's comments name.
                //
                // position() re-clamps twice: an `ny = H - M - h` line, and the
                // `Math.max(M, ny)` floor underneath it. Measured here, the
                // placement is the FLOOR, at exactly M.
                //
                // And the other line is UNREACHABLE BY CONSTRUCTION, which is
                // arithmetic rather than a property of this page: after the flip
                // ny = y - h - 12, so `ny + h` collapses to `y - 12`, and that
                // exceeds H - M only when y > H - M + 12 = 224 — a cursor
                // position outside a 220 px viewport. The same collapse holds on
                // the x axis (`nx + w` becomes `x - 14`) and on every frame in
                // the family, because it never mentions h at all. O-Chorus's copy
                // of this renderer says "every vertical placement on this page
                // lands on the line below, not on the flip above it"; the
                // behaviour it describes is real and the line it credits is not
                // the one doing it. Reported, not edited — the family stays
                // byte-shaped, and the FLOOR is what this assertion pins.
                check(Math.abs(hit.rect.top - MARGIN) < 0.5,
                    `[4c] the re-clamp that fired is the ${MARGIN} px FLOOR, not the `
                    + `H - M - h line — top ${hit.rect.top.toFixed(1)} vs M ${MARGIN} `
                    + `(H - M - h would be ${(H - MARGIN - hit.h).toFixed(1)})`);
                check((cy - 12) <= (H - MARGIN),
                    `[4c] and the H - M - h line is unreachable BY CONSTRUCTION: after a flip `
                    + `ny + h collapses to y - 12 = ${(cy - 12).toFixed(1)}, which cannot exceed `
                    + `H - M = ${H - MARGIN} for any cursor inside a ${H} px viewport`);
            }

            // restore from the TABLE, and prove the restore took
            await page.evaluate(() => window.__setLanguage('en'));
            await page.waitForTimeout(150);
            const back = await hoverAnchor('#lf_on');
            check(back && back.tip.body === I18N['tip.lfOn'].en.b,
                '[4c] restored: #lf_on carries the table body again, byte-equal');
        }

        // ── 5. FRENCH REALLY IS TALLER, and English really came back ────────
        //
        // The point of running both languages is that French wraps to more lines
        // against the max-width cap. If it did NOT, the two passes would be the
        // same measurement twice and assertion 4's French half would be
        // decoration. Measured from the FIRST en pass and the fr pass, so no
        // extra driving is needed to make the claim.
        const allSel = [...anchorsClosed, ...anchorsOpen];
        const grew   = allSel.filter(s => heights.fr[s] > heights.en[s] + 0.5);
        const same   = allSel.filter(s => Math.abs(heights.fr[s] - heights.en[s]) <= 0.5);
        const shrank = allSel.filter(s => heights.fr[s] < heights.en[s] - 0.5);
        check(grew.length > 0,
            `[5] French GROWS at least one tip's height against the ${CAP} px cap`,
            grew.length ? null
                : 'no tip grew: the fr pass measured the same boxes as en, so its clamp half is decoration');
        note(`${grew.length} grew, ${same.length} unchanged, ${shrank.length} SHRANK in French`);
        for (const s of allSel)
            note(`   ${s.padEnd(50)} en ${heights.en[s].toFixed(1)} -> fr ${heights.fr[s].toFixed(1)}`);

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const backEn = await hoverAnchor('#analog');
        check(backEn && backEn.tip.title === I18N['tip.analog'].en.t
                     && backEn.tip.body  === I18N['tip.analog'].en.b,
            '[5] English comes back after the French pass — byte-equal again');

        // ── 5b. THE KNOB'S OWN VALUE READOUT, MEASURED NOT ASSUMED ──────────
        //
        // .value-tooltip (#lf_tooltip and friends) is this page's PRE-EXISTING
        // readout chip: it appears on .dual-knob-container:hover, 20 px below
        // the knob, and shows "100 Hz / 0.0 dB". Hovering a dual knob therefore
        // shows TWO surfaces at once, and if the new hover-help covered the
        // readout the user is reading, that would be a regression in a working
        // control. Reported as a number rather than asserted away — a
        // zero-overlap claim would be a design promise this stage did not make.
        {
            const ov = await page.evaluate(() => {
                const el = document.querySelector('.dual-knob-container[data-param-outer="lf_freq"]');
                const rt = document.getElementById('lf_tooltip');
                const tip = document.getElementById('tooltip');
                const a = tip.getBoundingClientRect();
                const b = rt.getBoundingClientRect();
                const ox = Math.max(0, Math.min(a.right, b.right) - Math.max(a.left, b.left));
                const oy = Math.max(0, Math.min(a.bottom, b.bottom) - Math.max(a.top, b.top));
                return { overlap: Math.round(ox * oy), readoutOpacity: getComputedStyle(rt).opacity,
                         readoutText: rt.textContent };
            });
            note(`.value-tooltip while the LF hover-help is open: opacity ${ov.readoutOpacity}, `
               + `text "${ov.readoutText}", overlap with the tip ${ov.overlap} px2`);
        }

        // ── 5c. A CROSS-CELL DRAG DOES NOT OPEN THE NEIGHBOUR'S TIP ─────────
        //
        // O-Chorus needed a drag.active flag because a drag crossing into a
        // neighbouring cell opens that neighbour's tip mid-gesture, over the
        // readout the user is dragging to reach; pointerdown alone cannot cover
        // it because the pointerover arrives after it. THIS page does not need
        // the flag, and the reason is setPointerCapture(e.pointerId) at
        // index.html:1017 — every pointer event for the rest of the drag is
        // retargeted to the captured container. That is a CLAIM until it is
        // driven, so it is driven.
        {
            await setPopover(false);
            const boxes = await page.evaluate(() => {
                const g = (p) => document.querySelector(`.dual-knob-container[data-param-outer="${p}"]`)
                                        .getBoundingClientRect();
                const a = g('lf_freq'), b = g('lmf_freq');
                return { from: { x: a.left + a.width / 2, y: a.top + a.height / 2 },
                         to:   { x: b.left + b.width / 2, y: b.top + b.height / 2 } };
            });
            await page.mouse.move(boxes.from.x, boxes.from.y);
            await page.waitForTimeout(200);
            await page.mouse.down();
            await page.mouse.move(boxes.to.x, boxes.to.y, { steps: 12 });
            await page.waitForTimeout(220);
            const mid = await page.evaluate(READ_TIP);
            await page.mouse.up();
            await page.waitForTimeout(120);
            check(!(mid.shown && mid.visibility === 'visible' && mid.opacity === '1'),
                '[5c] a drag from the LF knob into the LMF cell opens NO tip — pointer capture '
                + 'retargets every boundary event, so this page needs no drag.active flag',
                mid.shown ? `tip showing "${mid.title}" mid-drag` : null);
            await page.mouse.move(W - 2, 2);
            await page.waitForTimeout(150);
        }

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
        // THE PLANT IS SIZED AGAINST THIS FRAME AND THIS CAP, NOT BY HABIT.
        // O-Tremolo's 40x plant — 880 chars, roughly 390 px — FIT inside a
        // 400 px frame with 384 px of clamp room and reported nothing, and a
        // plant that fits is indistinguishable from a gate that cannot see. The
        // well here is 204 px and the line box is ~13.5 px, so the ceiling is
        // ~13 body lines; at a 420 px cap a line holds roughly 90 characters, so
        // ~1200 characters is the break-even and anything under it proves
        // nothing. This plant is 4800 characters — four times break-even — and
        // the assertion below CONFIRMS IT ACTUALLY OVERFLOWED rather than
        // assuming the arithmetic.
        console.log('\n-- negative control (assertion 4 harness-blindness)');
        const PLANT = ('overflow probe. ').repeat(300);      // 4800 chars
        note(`plant: ${PLANT.length} chars against a ${H - 2 * MARGIN} px well and a ${CAP} px cap`);
        await page.evaluate((body) => {
            document.getElementById('analog').setAttribute('data-tip', body);
        }, PLANT);

        const planted = await hoverAnchor('#analog');
        const plantedOut = planted ? outsideViewport(planted.tip.rect, W, H) : ['tip did not render'];
        check(planted && planted.tip.body === PLANT,
            '[6] the plant actually reached the surface (a plant that never rendered proves nothing)');
        check(planted && planted.tip.rect.h > (H - 2 * MARGIN),
            `[6] the plant is BIGGER THAN THE WELL — ${planted ? planted.tip.rect.h.toFixed(1) : '?'} px `
            + `vs ${H - 2 * MARGIN} px, so there is no x or y at which the clamp could place it. `
            + `A plant that FIT would make the next assertion vacuous`);
        check(plantedOut.length > 0,
            `[6] the planted over-long body OVERFLOWS and assertion 4 reports it — `
            + `${plantedOut.join('; ') || 'NOTHING REPORTED'}`,
            plantedOut.length ? `planted tip ${planted.tip.rect.w.toFixed(1)} x `
                              + `${planted.tip.rect.h.toFixed(1)}, frame ${W} x ${H}`
                              : 'assertion 4 is BLIND — every [4] pass above is decoration');

        // restore from the TABLE, and prove the restore took
        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const restored = await hoverAnchor('#analog');
        check(restored && restored.tip.body === I18N['tip.analog'].en.b,
            '[6] restored: the anchor carries the table body again, byte-equal');
        check(restored && outsideViewport(restored.tip.rect, W, H).length === 0,
            '[6] restored: assertion 4 is green again at the same anchor');

        // ══════════════════════════════ 7. THE FOCUS LATCH, BOTH HALVES ══
        //
        // A mouse click on a <button> focuses it. An unconditional focusin rule
        // therefore re-opens the tip that pointerdown just hid, with the pointer
        // still on the anchor and no further pointerover coming — and the tip sits
        // on top of whatever the click opened. Measured on the two M1 pilots that
        // shipped without the latch: 146 x 35 px of popover covered on O-Bass,
        // 161 x 29 px on O-AnalogSaturation.
        //
        // BOTH halves are asserted, separately and on purpose. Asserting only that
        // a click leaves no tip lets the feature decay into "focus never shows a
        // tip", which passes that assertion perfectly and silently removes the
        // keyboard half of hover-help.
        //
        // A STATIC regex for `lastInputWasPointer` would stay green with the guard
        // clause deleted — the declaration, the pointerdown write and the keydown
        // clear all survive. Only the behavioural control below discriminates.
        console.log('\n-- 7. the focus latch');

        // The popover must be CLOSED going in, so the click below OPENS it and
        // the overlap is a number rather than a null. Left open by the section
        // above, the same click would CLOSE it, the comparison rect would be
        // gone, and the control would still fail — but silently, with nothing
        // to quote. "146 x 35" and "161 x 29" are what made this defect
        // reportable on the pilots rather than arguable.
        await setPopover(false);
        await page.mouse.move(1, 1);
        await page.waitForTimeout(150);
        // BLUR FIRST, and this line is the whole reason the assertion below can
        // fail at all. An earlier section of this gate leaves focus ON #gear-btn,
        // and clicking an ALREADY-FOCUSED element fires no focusin — so without
        // this the check reports "no tip after a click" for a page with no latch
        // whatsoever. The first version of this assertion on the pilots passed
        // 125/125 with the latch DELETED, and O-Tremolo went further: with the
        // latch removed AND this line deleted its whole suite passed 186/186.
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
            return {
                shown, overlap,
                panelOpen: !!pr,
                tip: { x: r.left, y: r.top, w: r.width, h: r.height },
                panel: pr ? { x: pr.left, y: pr.top, w: pr.width, h: pr.height } : null,
                focused: document.activeElement ? document.activeElement.id : null,
            };
        });
        // Without this the overlap number is silently null and the control below
        // fails with nothing to quote.
        check(afterClick.panelOpen,
            '[7] the click OPENED the settings popover — the rect the overlap is measured against');
        check(afterClick.focused === 'gear-btn',
            `[7] the click really did move focus onto #gear-btn — got #${afterClick.focused}. `
            + `Without a focus MOVE there is no focusin and the assertion below cannot fail`);
        check(!afterClick.shown,
            '[7] a POINTER click opens no tip — the latch suppresses the focusin arm'
            + (afterClick.overlap ? ` (it covered the popover by ${afterClick.overlap} px2)` : ''),
            afterClick.shown
                ? `tip [${afterClick.tip.x.toFixed(0)},${afterClick.tip.y.toFixed(0)},`
                  + `${afterClick.tip.w.toFixed(0)}x${afterClick.tip.h.toFixed(0)}] vs popover `
                  + (afterClick.panel
                      ? `[${afterClick.panel.x.toFixed(0)},${afterClick.panel.y.toFixed(0)},`
                        + `${afterClick.panel.w.toFixed(0)}x${afterClick.panel.h.toFixed(0)}]`
                      : '[closed]')
                  + `, focus on #${afterClick.focused}`
                : null);

        // The keyboard half. A real tab-ring walk, not a programmatic .focus():
        // Chromium reports :focus-visible false for a .focus() that follows a click,
        // and .focus() on an already-focused element fires no event at all — either
        // one would report "no tip" and record that as correct.
        //
        // THE SETTLE IS 150 ms, PAST THE 120 ms OPACITY TRANSITION, and the
        // predicate below is `opacity !== '0'` rather than a threshold. O-Comp's
        // first version of this probe slept 80 ms into that transition while
        // treating anything under 0.99 as hidden, reported "none in 20 tabs" for a
        // path that demonstrably works, and the obvious response to that reading is
        // to DELETE the latch — which makes the click defect green again by way of
        // a probe artefact (pattern_quiesce_before_stimulus_in_async_ui_gates).
        //
        // KEYBOARD REACH ON THIS PAGE IS PARTIAL BY DESIGN and that is a finding,
        // not a gap this gate should paper over: the eleven parameter anchors are
        // pointer-drag divs with no tabindex and never were keyboard-operable, so
        // the only tabbable anchors are #gear-btn and #lang-select.
        await page.keyboard.press('Escape');
        await page.waitForTimeout(150);
        let kbHit = null;
        for (let i = 1; i <= 20; i++) {
            await page.keyboard.press('Tab');
            await page.waitForTimeout(150);
            const r = await page.evaluate(() => {
                const t = document.getElementById('tooltip');
                const cs = getComputedStyle(t);
                const rect = t.getBoundingClientRect();
                return { shown: cs.visibility !== 'hidden' && cs.opacity !== '0',
                         text: (t.textContent || '').trim(),
                         rect: { left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom,
                                 w: rect.width, h: rect.height },
                         on: document.activeElement ? (document.activeElement.id || document.activeElement.className) : null };
            });
            if (r.shown && r.text) { kbHit = { press: i, ...r }; break; }
        }
        check(kbHit !== null,
            '[7] a KEYBOARD tab still opens a tip — the accessibility half survives the latch'
            + (kbHit ? ` (tab #${kbHit.press} on ${kbHit.on})` : ' — none in 20 tabs'));
        // A focus-placed tip is placed from the anchor's own rect rather than a
        // cursor, which is a DIFFERENT branch of position(). The gear sits 8 px
        // from the bottom of a 220 px frame, so this placement clamps every time
        // and is asserted rather than assumed.
        if (kbHit) {
            const kbOut = outsideViewport(kbHit.rect, W, H);
            check(kbOut.length === 0,
                `[7] the KEYBOARD-placed tip is inside ${W} x ${H} too — a focus placement runs a `
                + `different branch of position() than a hover (${kbHit.rect.w.toFixed(1)} x `
                + `${kbHit.rect.h.toFixed(1)} at ${kbHit.rect.left.toFixed(1)},${kbHit.rect.top.toFixed(1)})`,
                kbOut.length ? kbOut.join('; ') : null);
        }
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
