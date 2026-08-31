/*
   This file is part of O-Detune, an Ouaricon Audio plugin.
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

    ui_tip_render_check.js — O-Detune hover-help, RENDERED.

    ── WHY THIS FILE EXISTS ────────────────────────────────────────────────────

    NO OTHER GATE IN THIS REPO CAN SEE A RENDERED TOOLTIP.

      check-i18n      reads the table statically. Its assertion 2 counts
                      TIP_BINDINGS rows and checks each key exists in I18N.
                      Eighteen bindings pointing at eighteen selectors that
                      match nothing, on a page with no renderer at all, is a
                      PASS.
      check-ui-labels has no tooltip awareness whatsoever — it sweeps
                      [data-i18n] elements and never looks at data-tip. Its
                      whole output is BYTE-IDENTICAL before and after this
                      feature landed on this plugin; that was measured, not
                      assumed.
      boot-all-uis    counts aria-label and title. It never counts data-tip.

    So authoring eighteen tooltip bodies into i18n.js and binding them, with no
    other change, would have shipped eighteen INVISIBLE strings past three green
    gates. v1.6.0 of this plugin had NO #tooltip node, NO .tooltip rule and NO
    hover handler.

    ── WHAT IT ASSERTS ─────────────────────────────────────────────────────────

      [0] the surface exists, is fixed, hidden at rest, pointer-events: none,
          and out-stacks BOTH raised layers on this page
      [1] every TIP_BINDINGS selector resolves, its wrapper resolves, and
          applyI18n wrote both attributes onto the anchor
      [2] hovering each anchor SHOWS the tip with non-empty title and body
          — the vacuity guard, and the assertion this whole file exists for
      [3] the rendered title and body are BYTE-EQUAL to the table's en/fr
      [4] the tip rectangle is fully inside 600 x 480, all four edges
      [5] French really is taller than English somewhere, and English returns
      [6] a planted over-long body OVERFLOWS and [4] reports it — the
          harness-blindness control
      [7] the focus latch, BOTH halves, separately

    ── THREE ANCHORS NEED A STATE DRIVEN FIRST ─────────────────────────────────

      #random_amt_knob   its .random-knob-container is `display: none` until
                         unison_dist is Random (index.html:519-525).
      #width             mono_safe defaults ON, which puts `.disabled` on
                         .slider-container — `pointer-events: none`. A POINTER
                         cannot reach that anchor until Mono-Safe is switched
                         off. That is a shipped v1.5.4 interaction, reported as
                         a finding and NOT changed here; the gate drives
                         Mono-Safe off and says so.
      #lang-select       lives inside a [hidden] popover until the gear is
                         clicked.

    Pinned to the SHIPPING size parsed out of PluginEditor.cpp's setSize(),
    never a default 1280x720: a clamp gate at the wrong viewport measures a page
    that has room and certifies nothing
    (pattern_tooltip_clamp_gate_viewport_sensitive).

    ── AND FRENCH IS THE OTHER HALF ────────────────────────────────────────────

    French runs 15-20 % longer, wraps to more lines against the 240 px
    max-width cap, and GROWS THE TIP'S HEIGHT. A tip that fits in English can
    therefore overflow the bottom in French, which is why every assertion runs
    in both languages rather than in English with a French spot-check.

    Usage:
        node plugins/O-Detune/tests/ui_tip_render_check.js
        node plugins/O-Detune/tests/ui_tip_render_check.js --verbose

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

const PLUGIN  = 'O-Detune';
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
// of them. A fixture that mirrored the table would drift silently
// (pattern_test_fixture_mirrors_drift_silently).
async function loadTable() {
    const src = path.join(REPO_ROOT, 'plugins', PLUGIN, 'Source', 'ui', 'public', 'js', 'i18n.js');
    if (!fs.existsSync(src)) throw new Error(`i18n.js not found at ${src}`);

    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'odetune-i18n-'));
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

// The fourteen anchors reachable in the page's DEFAULT state, plus the three
// that are not. Each list is a plain array of selectors; the wrapper for each
// comes from TIP_BINDINGS itself, never re-typed here.
const ANCHORS_DEFAULT = [
    '#wobble_era', '#wobble_shape', '#wobble_rate_knob', '#wobble_sync', '#wobble_depth_knob',
    '#blend_knob',
    '#unison_voices', '#unison_dist', '#unison_detune_knob', '#unison_spread_knob',
    '#delay_knob', '#feedback_knob', '#mono_safe', '#mix_knob',
];
const ANCHOR_RANDOM = '#random_amt_knob';   // needs unison_dist = Random
const ANCHOR_WIDTH  = '#width';             // needs mono_safe = Off
const ANCHORS_CHROME = ['#gear-btn', '#lang-select'];   // need the popover open

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
    const wrapperOf = (sel) => {
        const b = TIP_BINDINGS.find(x => x[0] === sel);
        return b ? (b[2] || null) : null;
    };
    const keyOf = (sel) => {
        const b = TIP_BINDINGS.find(x => x[0] === sel);
        return b ? b[1] : null;
    };

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
            // "enters check-ui-labels' sweep" would be the natural thing to
            // write here and it is FALSE — measured on O-Bass, and again on
            // this plugin: check-ui-labels' output is byte-identical before and
            // after this feature. What DOES catch an un-hidden surface is
            // check-i18n assertion 10, boot-all-uis' text count — and this
            // assertion.
            check(surface.visibility === 'hidden',
                '[0] the surface is HIDDEN at rest — un-hidden it becomes an unkeyed text node '
                + `(check-i18n assertion 10) and a permanent overlay (visibility ${surface.visibility})`);
            // This page has TWO raised layers: .preset-dropdown at 100 and
            // .settings-popover at 61. #lang-select is itself an anchor and
            // lives INSIDE the popover, so a tip that did not out-stack both
            // would open behind the control that opened it.
            check(Number(surface.zIndex) > 100,
                `[0] the surface out-stacks .preset-dropdown (100) and .settings-popover (61) — `
                + `z-index ${surface.zIndex}`);
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
            return {
                sel, key, found: true,
                wrapperResolved: wrapper ? !!el.closest(wrapper) : null,
                anchorTag: target.tagName.toLowerCase(),
                anchorId: target.id || '',
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
            if (r.wrapperResolved === false)
                check(false, `[1] the declared wrapper for ${r.sel} did NOT resolve — the tip fell `
                           + 'back onto the id\'d node, which is not the hover target it was bound to');
            if (verbose) note(`${r.sel} -> ${r.anchorTag}#${r.anchorId}.${r.anchorClass}`
                            + (r.anchorIsSelf ? ' (self)' : ' (wrapper)'));
        }
        // The two chrome rows are bare ON PURPOSE — .settings-cluster holds the
        // gear AND the popover, so a wrapper walk from either would resolve to
        // the same node and hovering #lang-select would open the gear's tip
        // (O-Comp's carried trap). Assert the anchors really are DISTINCT
        // rather than trusting the table not to have grown a wrapper.
        const chromeDistinct = await page.evaluate(() => {
            const g = document.querySelector('#gear-btn');
            const l = document.querySelector('#lang-select');
            return !!g && !!l && g !== l
                && g.getAttribute('data-tip') !== l.getAttribute('data-tip');
        });
        check(chromeDistinct,
            '[1] #gear-btn and #lang-select carry DISTINCT tips — a wrapper walk to '
            + '.settings-cluster would collapse them onto one node');

        check(tipWarns.length === 0,
            '[1] applyI18n logged no "tip target not found" warning',
            tipWarns.slice(0, 3).join(' | '));

        // ── state drivers ───────────────────────────────────────────────────
        async function setPopover(open) {
            const isOpen = await page.evaluate(() =>
                !document.getElementById('settings-popover').hidden);
            if (isOpen !== open) {
                await page.click('#gear-btn');
                await page.waitForTimeout(150);
            }
            return await page.evaluate(() => !document.getElementById('settings-popover').hidden);
        }
        // unison_dist -> Random reveals #random_amt_container (.visible).
        async function setDistRandom(on) {
            await page.evaluate((v) => {
                const s = document.getElementById('unison_dist');
                s.value = v;
                s.dispatchEvent(new Event('change', { bubbles: true }));
            }, on ? '2' : '0');
            await page.waitForTimeout(150);
            return await page.evaluate(() => {
                const c = document.getElementById('random_amt_container');
                return !!c && getComputedStyle(c).display !== 'none';
            });
        }
        // mono_safe OFF removes .disabled from .slider-container, which is
        // pointer-events: none while it is on. Clicked rather than set, because
        // the page's own click handler is what flips the toggle state.
        async function setMonoSafe(on) {
            const cur = await page.evaluate(() =>
                document.getElementById('mono_safe').classList.contains('active'));
            if (cur !== on) {
                await page.click('#mono_safe');
                await page.waitForTimeout(150);
            }
            return await page.evaluate(() => ({
                active: document.getElementById('mono_safe').classList.contains('active'),
                pe: getComputedStyle(document.getElementById('width').parentElement).pointerEvents,
            }));
        }

        async function hoverAnchor(sel) {
            const wrapper = wrapperOf(sel);
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
                if (r.width === 0 || r.height === 0) return null;
                return { x: r.left + r.width / 2, y: r.top + r.height / 2, w: r.width, h: r.height };
            }, { sel, wrapper });
            if (!box) return null;
            await page.mouse.move(box.x, box.y);
            // Past the 0.12 s opacity transition, so `opacity` is settled and a
            // mid-flight value cannot be read as "not shown"
            // (pattern_quiesce_before_stimulus_in_async_ui_gates — O-Comp's
            // first tab probe sampled at 80 ms into a 120 ms fade and reported
            // a path that demonstrably works as "never opens").
            await page.waitForTimeout(240);
            return { box, tip: await page.evaluate(READ_TIP) };
        }

        function assertAnchor(lang, sel, got) {
            const key = keyOf(sel);
            const entry = (I18N[key] || {})[lang] || {};
            if (!got) { check(false, `[2][${lang}] ${sel} is hoverable`); return; }
            const t = got.tip;

            // ── 2. THE VACUITY GUARD ────────────────────────────────────────
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

            // ── 3. BYTE-EQUAL, not "contains" ───────────────────────────────
            // A .tip-title that silently kept the PREVIOUS anchor's text passes
            // a contains check on a body that mentions the same words. Equality
            // is the only test that catches it.
            check(t.title === entry.t,
                `[3][${lang}] ${sel} title is byte-equal to I18N['${key}'].${lang}.t`,
                t.title === entry.t ? null : `rendered "${t.title}" vs table "${entry.t}"`);
            check(t.body === entry.b,
                `[3][${lang}] ${sel} body is byte-equal to I18N['${key}'].${lang}.b`,
                t.body === entry.b ? null
                    : `rendered ${t.body.length} chars vs table ${entry.b.length} chars`);

            // ── 4. INSIDE THE VIEWPORT, ALL FOUR EDGES ──────────────────────
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

        // ── the driving loop, en -> fr -> en ────────────────────────────────
        const drivenStates = [];

        for (const lang of ['en', 'fr', 'en']) {
            const pass = drivenStates.includes(lang) ? ' (return pass)' : '';
            drivenStates.push(lang);
            console.log(`\n-- language: ${lang}${pass}`);

            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(150);

            // default state: popover closed, dist Linear, mono-safe on
            check(await setPopover(false) === false, `[.][${lang}] settings popover is closed`);
            await setDistRandom(false);
            await setMonoSafe(true);
            for (const sel of ANCHORS_DEFAULT) assertAnchor(lang, sel, await hoverAnchor(sel));

            // #random_amt_knob — hidden until unison_dist is Random
            const randomShown = await setDistRandom(true);
            check(randomShown, `[.][${lang}] unison_dist = Random reveals #random_amt_container`);
            assertAnchor(lang, ANCHOR_RANDOM, await hoverAnchor(ANCHOR_RANDOM));
            await setDistRandom(false);

            // #width — pointer-unreachable while Mono-Safe is on
            const ms = await setMonoSafe(false);
            check(ms.active === false && ms.pe !== 'none',
                `[.][${lang}] Mono-Safe off releases .slider-container from pointer-events: none `
                + `(active=${ms.active} pointer-events=${ms.pe})`);
            assertAnchor(lang, ANCHOR_WIDTH, await hoverAnchor(ANCHOR_WIDTH));
            await setMonoSafe(true);

            // the chrome — #lang-select does not exist to a pointer until the
            // panel is open
            check(await setPopover(true) === true, `[.][${lang}] settings popover is OPEN`);
            for (const sel of ANCHORS_CHROME) assertAnchor(lang, sel, await hoverAnchor(sel));
            await setPopover(false);
        }

        // ── 5. FRENCH REALLY IS TALLER, and English really came back ────────
        //
        // Re-measured here rather than inferred from the loop: the point of
        // running both languages is that French wraps to more lines against the
        // 240 px max-width cap, and if it did NOT the two passes would be the
        // same measurement twice and assertion 4's French half would be
        // decoration.
        console.log('\n-- 5. French height vs English');
        async function heightsFor(lang) {
            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(150);
            await setPopover(false);
            const h = {};
            for (const sel of ANCHORS_DEFAULT) {
                const got = await hoverAnchor(sel);
                h[sel] = got ? got.tip.rect.h : -1;
            }
            return h;
        }
        const hEn = await heightsFor('en');
        const hFr = await heightsFor('fr');
        const grew = ANCHORS_DEFAULT.filter(s => hFr[s] > hEn[s] + 0.5);
        const same = ANCHORS_DEFAULT.filter(s => Math.abs(hFr[s] - hEn[s]) <= 0.5);
        check(grew.length > 0,
            `[5] French GROWS at least one tip's height against the 240 px cap — ${grew.length} of `
            + `${ANCHORS_DEFAULT.length} grew`,
            grew.length ? null
                : 'no tip grew: the fr pass measured the same boxes as en, so its clamp half is decoration');
        note(ANCHORS_DEFAULT.map(s => `${s.replace('#', '')} ${hEn[s].toFixed(0)}->${hFr[s].toFixed(0)}`).join(', '));
        if (same.length) note(`${same.length} tip(s) the same height in both languages: ${same.join(', ')}`);

        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const backEn = await hoverAnchor('#mix_knob');
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
        // SIZED AGAINST THIS FRAME, not by habit. max-width is 240 px, so the
        // reachable overflow on this renderer is VERTICAL: 480 px of frame less
        // 16 px of clamp margin and 16 px of padding+border leaves ~448 px, and
        // at 11 px/1.4 line-height that is ~29 lines of ~40 characters. A 40x
        // "overflow probe. " plant (640 chars, ~16 lines) FITS here and would
        // report nothing — O-Tremolo measured exactly that failure at 400 px.
        // 200 repeats is 3200 chars, ~80 lines, ~1230 px: comfortably past the
        // clamp's reach, and the plant is asserted to have actually overflowed.
        console.log('\n-- 6. negative control (assertion 4 harness-blindness)');
        const PLANT = ('overflow probe. ').repeat(200);
        await page.evaluate((body) => {
            const el = document.querySelector('#wobble_depth_knob').closest('.knob-container');
            el.setAttribute('data-tip', body);
        }, PLANT);

        const planted = await hoverAnchor('#wobble_depth_knob');
        const plantedOut = planted ? outsideViewport(planted.tip.rect, W, H) : ['tip did not render'];
        check(plantedOut.length > 0,
            '[6] a planted over-long body OVERFLOWS and assertion 4 reports it — '
            + `${plantedOut.join('; ') || 'NOTHING REPORTED'}`,
            plantedOut.length ? `planted tip ${planted.tip.rect.w.toFixed(1)} x `
                              + `${planted.tip.rect.h.toFixed(1)}, frame ${W} x ${H}`
                              : 'assertion 4 is BLIND — every [4] pass above is decoration');
        check(planted && planted.tip.body === PLANT,
            '[6] the plant actually reached the surface (a plant that never rendered proves nothing)');

        // restore from the TABLE, and prove the restore took
        await page.evaluate(() => window.__setLanguage('en'));
        await page.waitForTimeout(150);
        const restored = await hoverAnchor('#wobble_depth_knob');
        check(restored && restored.tip.body === I18N['tip.depth'].en.b,
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
        // O-AnalogSaturation. On THIS page the gear is 6 px from the
        // bottom-left corner and the popover opens directly above it, so a
        // parked gear tip lands square on the language selector.
        //
        // BOTH halves are asserted, separately and on purpose. Asserting only
        // that a click leaves no tip lets the feature decay into "focus never
        // shows a tip", which passes that assertion perfectly and silently
        // removes the keyboard half of hover-help.
        console.log('\n-- 7. the focus latch');

        await setPopover(false);
        await page.mouse.move(1, 1);
        await page.waitForTimeout(150);
        // BLUR FIRST, and this line is the whole reason the assertion below can
        // fail at all. An earlier section of this gate leaves focus ON
        // #gear-btn, and clicking an ALREADY-FOCUSED element fires no focusin —
        // so without this the check reports "no tip after a click" for a page
        // with no latch whatsoever. The orchestrator's first version of this
        // assertion passed 125/125 with the latch deleted for exactly that
        // reason, and O-Tremolo then proved the same thing from the other side:
        // latch removed AND blur removed passed 186/186.
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
            + (afterClick.overlap ? ` (it covered the popover by ${afterClick.overlap} px2)` : ''),
            afterClick.shown ? `focus landed on #${afterClick.focused}` : null);

        // The keyboard half. A real tab-ring walk, not a programmatic .focus():
        // Chromium reports :focus-visible false for a .focus() that follows a
        // click, and .focus() on an already-focused element fires no event at
        // all — either one would report "no tip" and record that as correct.
        //
        // `visibility`, not an opacity threshold, and 200 ms per press: O-Comp's
        // first version of this walk sampled 80 ms into a 120 ms fade while
        // treating anything under opacity 0.99 as hidden, reported "none in 20
        // tabs" for a path that works, and the obvious response to that failure
        // is to DELETE the latch — which makes the click defect green again by
        // way of a probe artefact.
        await page.keyboard.press('Escape');
        await page.waitForTimeout(200);
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
                             ? (document.activeElement.id || document.activeElement.className) : null };
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

        console.log('');
        note(`states driven: en/fr/en x { default, unison_dist=Random, mono_safe=Off, popover open }`);
        note(`states NOT driven: wobble_sync on (the Rate readout's musical-division branch — it `
           + `changes a READOUT node, never a tip), preset dropdown open (no anchors inside it)`);

    } finally {
        await browser.close();
        await close();
        fs.rmSync(built.root, { recursive: true, force: true });
    }

    console.log(`\n${failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`}`
              + `   (${passes} passed)`);
    process.exit(failed === 0 ? 0 : 1);
})().catch((e) => { console.error(e); process.exit(1); });
