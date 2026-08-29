/*
   This file is part of the Ouaricon Audio plugin suite.
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

    check-ui-labels.js — the both-language LABEL render gate. This is what
    retires D-04.

    It generalises the both-language sweep already committed in
    plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js rather than starting
    fresh: same shape, same vacuity guards, same exit-77 convention, same
    per-language failure labelling — an unlabelled French-only failure reads as a
    mysterious regression in a file that never mentions French.

    ── ONE FILE, REPO-LEVEL. Flag this at Checkpoint 3. ────────────────────────

    The three tooltip clamp gates live in plugins/<Name>/tests/. This one does
    not, and that is a deliberate departure:

      - the CONTEXT no-shared-module rule governs shipped UI RUNTIME code, not
        test tooling;
      - scripts/check-i18n.js is already the precedent for a repo-level
        per-plugin gate;
      - 43 hand-copies of a Playwright file is precisely the drift this repo has
        paid for twice.

    The cost is real: this gate does not run from a plugin's own tests/
    directory the way every other gate there does, so a plugin-scoped CI job
    would have to know about it. Per-plugin knowledge lives in an optional
    plugins/<Name>/tests/i18n-states.json.

    ── The GEOMETRY DIFF is the primary detector, not the clip check ───────────

    Repo-wide there are only ~33 `text-overflow: ellipsis` declarations and
    ~110 `white-space: nowrap`. NINE plugins have neither. On those, French does
    not clip a label — it makes the label TALLER and pushes the row. A clip check
    is blind to that, and would certify exactly the plugins most at risk.

    So the load-bearing assertion is 7: at a FIXED frame, any element that is
    neither a label nor inside one must occupy the identical rectangle in both
    languages. Anything that moved was pushed by a French string, and it is
    reported by name with its delta.

    It uses getBoundingClientRect only. scrollHeight does not cross a `flex: 1`
    stage and is clamped on a grid container, so it lies about any container
    (pattern_scrollheight_does_not_cross_a_flex_stage,
    pattern_scrollheight_clamped_on_grid_container) — the clip check below is
    restricted to leaf label elements for that reason, and the diff is immune.

    ── Exit codes ─────────────────────────────────────────────────────────────

        0    all assertions passed
        n>0  n assertions failed
        77   Playwright unresolvable — NOTHING was verified, never a pass

    A plugin with zero [data-i18n] elements exits NON-ZERO with "nothing to
    measure". A gate that passes because there is nothing to check is the
    vacuous pass this whole design exists to prevent.

    Usage:
        node scripts/check-ui-labels.js --plugin O-Tapestop
        node scripts/check-ui-labels.js --plugin O-Tapestop --verbose
        node scripts/check-ui-labels.js --plugin O-Fixture --root /tmp/fix

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');
const S    = require(path.join(__dirname, 'serve-ui.js'));

const argv    = process.argv.slice(2);
const val     = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const plugin  = val('--plugin');
const repoRoot = val('--root') || S.REPO_ROOT;
const verbose = argv.includes('--verbose');

// Sub-pixel text metrics differ run to run by a few hundredths of a pixel. A
// zero tolerance would drown assertion 7's signal in noise; a loose one would
// hide a one-line wrap. Half a pixel is well under the smallest real shift
// (a wrapped line is >= the line-height) and well over the measured jitter.
const TOL = 0.5;

// Below this fraction of labels changing between the two passes, the run is
// assumed not to have switched language at all.
const MIN_LANG_DIFF_FRACTION = 0.25;

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}

if (!plugin) {
    console.log('usage: node scripts/check-ui-labels.js --plugin <Name> [--root DIR] [--verbose]');
    process.exit(2);
}

// ── the page-side probe ────────────────────────────────────────────────────
// Everything measured in ONE evaluate per language, so nothing can shift
// between two round trips.
const PROBE = () => {
    // ── FREEZE DECLARATIVE ANIMATION BEFORE MEASURING ──────────────────────
    //
    // Assertion 7 compares two rect sweeps taken ~180 ms apart, and reads any
    // difference as "a French string pushed this". An element that is ANIMATING
    // moves on its own between the two samples, so it reports as a French
    // geometry failure when nothing about the language moved it at all — the
    // wall-clock-inside-a-verdict shape, and a different answer every run.
    //
    // Found on O-simplePhysicalModelSynth, whose signal-flow diagram carries a
    // SMIL <animateTransform> circling a pulse around the Karplus-Strong delay
    // loop at 1.7 s per turn: #loopPulse, #loopPulseSpin and their parent
    // #stringSkin reported dx=-18.6 dy=-13.3 with the page in a single language.
    //
    // Pausing here rather than at page load means it also covers an animation a
    // state pass starts. pauseAnimations() is idempotent and never resumed, so
    // every snapshot in the run — both languages, every state — is measured at
    // the SAME frozen timeline position.
    //
    // This reaches DECLARATIVE animation only. A requestAnimationFrame loop is
    // the page's own code, not an Animation object, so neither call touches it
    // — see the EN -> EN animation control below, which measures the animated
    // set instead of declaring it and therefore covers both.
    //
    // CSSTransition is deliberately EXCLUDED. The state pass drives every slider
    // on the page and this repo's knob stems transition their rotation; pausing a
    // transition mid-flight would freeze a stem at an arbitrary intermediate
    // angle and invent exactly the phantom diff this block removes.
    for (const svg of document.querySelectorAll('svg')) {
        try { svg.pauseAnimations(); } catch (e) { /* not an SVGSVGElement */ }
    }
    if (typeof document.getAnimations === 'function') {
        for (const a of document.getAnimations()) {
            try { if (a.constructor && a.constructor.name !== 'CSSTransition') a.pause(); }
            catch (e) { /* already finished */ }
        }
    }

    const r = (el) => {
        const b = el.getBoundingClientRect();
        return { x: b.x, y: b.y, w: b.width, h: b.height };
    };

    // The path is the KEY assertion 7 diffs English against French on, so it
    // has to identify an element uniquely. It used to stop after 6 segments,
    // and on a page with more than six levels of repeated structure that cap
    // made two different elements share a key.
    //
    // O-Bassoon is the proof. Its sound tab is two `.sections-grid` rows of
    // `.section-box > .param-row > .knob-control > .knob-wrapper > svg > path`;
    // measured from an SVG child, six segments run out ABOVE the grid that
    // tells row 1 from row 2, so 24 elements collapsed onto 12 keys. The diff
    // builds its English map last-writer-wins, so row-1 elements were compared
    // against row-2 rectangles and the gate reported `dy=-190.4` — exactly the
    // distance between the rows — as a French geometry failure on a page whose
    // French geometry is perfect. Held at ENGLISH on both sides it reported the
    // same 12.
    //
    // The cap also cut the other way: two real elements collapsed onto one key
    // means the survivor is diffed and the other is never compared at all, so
    // the same defect can make assertion 7 VACUOUSLY GREEN.
    //
    // Uncapped, the nth-child chain makes a path unique by construction, which
    // is why there is no length limit to tune. The one way two elements can
    // still collide is a DUPLICATE id in the markup — invalid HTML, and a bug
    // in its own right — so the collision count is asserted below rather than
    // assumed away.
    const pathOf = (el) => {
        const bits = [];
        let n = el;
        while (n && n.nodeType === 1) {
            if (n.id) { bits.unshift('#' + n.id); break; }
            const p = n.parentElement;
            const i = p ? [...p.children].indexOf(n) + 1 : 1;
            bits.unshift(`${n.tagName.toLowerCase()}${n.className && typeof n.className === 'string'
                ? '.' + n.className.trim().split(/\s+/).slice(0, 2).join('.') : ''}:nth-child(${i})`);
            n = p;
        }
        return bits.join('>');
    };

    const visible = (el) => {
        const cs = getComputedStyle(el);
        if (cs.display === 'none' || cs.visibility === 'hidden' || cs.opacity === '0') return false;
        const b = el.getBoundingClientRect();
        return b.width > 0 && b.height > 0;
    };

    // ── THE PAINT LAYER A LABEL LIVES IN ────────────────────────────────
    //
    // Assertion 8 asks whether two labels that are disjoint in English
    // INTERSECT in French. It compares rectangles, and a rectangle has no z.
    //
    // A settings popover is an OPAQUE PANEL DRAWN OVER THE PAGE ON PURPOSE.
    // Every plugin in this rollout has one, and on a page whose first content
    // row happens to sit under it, one of the popover's own captions overlaps
    // the page text beneath it in BOTH languages — the gate only notices when
    // the French string is the one long enough to reach that far, and then
    // reports a collision that a user can never see, on a panel doing exactly
    // what a panel is for. (Found on O-simpleBeatmaker: the language caption
    // against the tail of the step-grid hint.)
    //
    // The fix is to record which paint layer each label is in and compare only
    // labels in the SAME one. A layer here is a positioned ancestor with a
    // numeric z-index AND a background that actually paints — all three
    // conditions, because that is what makes it opaque and on top. Two labels
    // inside the same popover are still compared with each other, and two
    // labels both in the page are still compared with each other; only a pair
    // that spans the boundary is skipped, and the skips are REPORTED.
    const overlayOf = (el) => {
        let n = el;
        while (n && n.nodeType === 1) {
            const cs = getComputedStyle(n);
            const bg = cs.backgroundColor || '';
            // A GRADIENT is a background-image, not a background-color, and
            // reads as rgba(0,0,0,0) here. Testing only the colour made this
            // blind to every gradient-backed panel: O-Contrabass's
            // .settings-popover is `background: linear-gradient(#4A3226,#3E2A20)`
            // and was not recognised as a paint layer at all, so its own
            // captions were compared against the page beneath the panel.
            const bgImg = cs.backgroundImage || '';
            const paints = (bg !== '' && bg !== 'transparent' && !/,\s*0\s*\)$/.test(bg))
                || (bgImg !== '' && bgImg !== 'none');
            if ((cs.position === 'absolute' || cs.position === 'fixed')
                && cs.zIndex !== 'auto' && paints)
                return pathOf(n);
            n = n.parentElement;
        }
        return null;
    };

    const labels = [...document.querySelectorAll('[data-i18n]')];
    const labelSet = new Set(labels);
    const insideLabel = (el) => {
        let n = el;
        while (n) { if (labelSet.has(n)) return true; n = n.parentElement; }
        return false;
    };

    const out = {
        labels: [],
        others: [],
        // dw/dh are documentElement — the element that actually scrolls, and
        // the one the committed clamp gates assert. bw/bh are body, reported
        // alongside because a body wider than documentElement means a child is
        // overflowing something with overflow:hidden, which is worth SEEING
        // without being a language failure.
        docScroll: {
            dw: document.documentElement.scrollWidth,
            dh: document.documentElement.scrollHeight,
            bw: document.body ? document.body.scrollWidth : 0,
            bh: document.body ? document.body.scrollHeight : 0,
        },
        viewport: { w: window.innerWidth, h: window.innerHeight },
        attrKeyed: [...document.querySelectorAll('[data-i18n-aria],[data-i18n-placeholder],[data-i18n-alt]')]
            .map((el) => ({
                path: pathOf(el),
                aria: el.getAttribute('aria-label'),
                placeholder: el.getAttribute('placeholder'),
                alt: el.getAttribute('alt'),
            })),
    };

    for (const el of labels) {
        const cs = getComputedStyle(el);
        const parent = el.offsetParent;
        out.labels.push({
            key: el.dataset.i18n,
            path: pathOf(el),
            text: el.textContent,
            datasetLabel: el.dataset.label === undefined ? null : el.dataset.label,
            visible: visible(el),
            rect: r(el),
            overlay: overlayOf(el),
            overflow: `${cs.overflow} ${cs.overflowX}`,
            whiteSpace: cs.whiteSpace,
            leaf: el.children.length === 0,
            scrollWidth: el.scrollWidth,
            clientWidth: el.clientWidth,
            // The RENDERED text's own width, and the content box it has to sit
            // in. Measured with a Range because scrollWidth CANNOT see this:
            // on a leaf whose overflow is `visible` the browser reports
            // scrollWidth === clientWidth however far the text spills, so a
            // check built on it is blind to precisely the elements that have
            // no clipping to protect them. Stage F found two French CHARACTER
            // segments on O-Tapestop overrunning their 58 px button by 6 and
            // 10 px with every other assertion green.
            textWidth: (() => {
                if (el.children.length) return null;
                const rng = document.createRange();
                rng.selectNodeContents(el);
                return rng.getBoundingClientRect().width;
            })(),
            contentWidth: el.clientWidth - parseFloat(cs.paddingLeft) - parseFloat(cs.paddingRight),
            // THE VERTICAL TWIN. The width pair above is blind to the other way
            // a French caption escapes: it WRAPS. A longer string in a box with
            // a constrained height wraps to two, three, four lines and spills
            // DOWNWARD, and every width measurement stays green because each
            // line is narrow enough. Found on O-AnalogSaturation v1.2.0 as a
            // negative control that failed to fire: a 43-char wrappable caption
            // in a 28px-high button rendered 44px of text in plain sight with
            // the whole gate passing. The same Range spans every line box, so
            // its HEIGHT is the wrapped height, which is exactly the number
            // clientHeight cannot give on an overflow:visible leaf.
            //
            // Measured with the LINE COUNT beside it, because height alone
            // over-reports. A leaf styled with a line-height TIGHTER than its
            // font's natural line box renders a 14px line box inside a 13px
            // content box and overhangs by 1px on a four-letter English word
            // that cannot wrap at all — O-simpleBeatmaker's six voice captions
            // do exactly this, in ENGLISH, and calling that a spill would put a
            // shipped plugin red over a deliberate typographic choice. Wrapping
            // is what this assertion is for, and wrapping IS more than one line
            // box: a Range over a leaf's contents yields one rect per line.
            textHeight: (() => {
                if (el.children.length) return null;
                const rng = document.createRange();
                rng.selectNodeContents(el);
                return rng.getBoundingClientRect().height;
            })(),
            textLines: (() => {
                if (el.children.length) return null;
                const rng = document.createRange();
                rng.selectNodeContents(el);
                return rng.getClientRects().length;
            })(),
            contentHeight: el.clientHeight - parseFloat(cs.paddingTop) - parseFloat(cs.paddingBottom),
            // clientWidth is DEFINED as 0 for a non-replaced inline element —
            // an inline box has no content box of its own, it is a run of line
            // boxes. Comparing a Range width against it therefore reports every
            // inline label as spilling, in ENGLISH, on correct markup:
            // O-Bitrot's nine panel captions are `<span>`s inside a
            // `white-space: nowrap` caption and fired all nine. The spill check
            // skips them and SAYS SO rather than quietly passing; assertions 5,
            // 6 and 7 still measure their rects, so the label is not unwatched
            // — only this one check cannot express itself about it.
            inlineBox: cs.display === 'inline',
            // Every ancestor's path, so assertion 8b can tell "this label grew
            // into the box that CONTAINS it" — not a collision — from "this
            // label grew into a box beside it", which is.
            ancestors: (() => {
                const a = [];
                for (let n = el.parentElement; n; n = n.parentElement) a.push(pathOf(n));
                return a;
            })(),
            parentPad: parent ? (() => {
                const pcs = getComputedStyle(parent);
                const pb = parent.getBoundingClientRect();
                return {
                    path: pathOf(parent),
                    x: pb.x + parseFloat(pcs.borderLeftWidth) ,
                    y: pb.y + parseFloat(pcs.borderTopWidth),
                    w: pb.width - parseFloat(pcs.borderLeftWidth) - parseFloat(pcs.borderRightWidth),
                    h: pb.height - parseFloat(pcs.borderTopWidth) - parseFloat(pcs.borderBottomWidth),
                };
            })() : null,
        });
    }

    for (const el of document.querySelectorAll('*')) {
        if (el.tagName === 'SCRIPT' || el.tagName === 'STYLE' || el.tagName === 'HEAD'
            || el.tagName === 'META' || el.tagName === 'LINK' || el.tagName === 'TITLE') continue;
        if (insideLabel(el)) continue;
        if (!visible(el)) continue;
        out.others.push({ path: pathOf(el), rect: r(el), overlay: overlayOf(el),
                          inert: getComputedStyle(el).pointerEvents === 'none' });
    }

    return out;
};

// A real state-update pass. Driving the stub's slider states fires the page's
// own valueChangedEvent listeners, which is what makes
// `dataset.label === textContent` a meaningful assertion instead of a
// restatement of what applyLabel just wrote — the failure it guards
// (pattern_js_state_updater_overwrites_html_labels) happens precisely when an
// updater has run.
const STATE_PASS = () => {
    let mechanism = 'events-only';
    const st = window.__stubStates;
    if (st && st.sliders) {
        mechanism = 'stub slider states';
        let i = 0;
        for (const s of st.sliders.values()) {
            try { s.setNormalisedValue(((i++ * 0.37) % 1) * 0.8 + 0.1); } catch (e) { /* reported by the caller */ }
        }
        for (const t of st.toggles.values()) { try { t.setValue(!t.getValue()); } catch (e) {} }
        for (const c of st.combos.values()) {
            try { c.setChoiceIndex((c.getChoiceIndex() + 1) % Math.max(1, c.properties.choices.length)); } catch (e) {}
        }
    }
    for (const el of document.querySelectorAll('input, select, textarea')) {
        el.dispatchEvent(new Event('input',  { bubbles: true }));
        el.dispatchEvent(new Event('change', { bubbles: true }));
    }
    return mechanism;
};

const overlaps = (a, b) =>
    a.x < b.x + b.w - TOL && b.x < a.x + a.w - TOL &&
    a.y < b.y + b.h - TOL && b.y < a.y + a.h - TOL;

(async () => {
    console.log(`== check-ui-labels — ${plugin} ==`);

    const size = S.readEditorSize(plugin, repoRoot);
    if (!size) {
        console.error(`  no setSize(W, H) parsed from ${plugin}/Source/PluginEditor.cpp — the shipping `
                    + 'frame is the whole point of this gate and it will not guess one.');
        process.exit(1);
    }
    console.log(`   viewport pinned to the SHIPPING frame ${size.w} x ${size.h}, parsed from PluginEditor.cpp\n`);

    const pw = S.resolvePlaywright();
    if (pw == null) {
        console.log('  SKIP: playwright not resolvable. Install with');
        console.log('        npx playwright install chromium');
        console.log('  French label geometry is NOT verified without it.');
        process.exit(77);   // distinct from 0: "could not verify" is not "passed"
    }
    const { chromium } = pw;

    const built = S.buildRoot(plugin, { repoRoot });
    console.log(`   ui root ${built.uiRootLabel} (${built.uiRootFrom}), stub=${built.stubKind}, seed=${built.seedFrom}`);
    if (built.unplaced.length) console.log(`   WARNING unmapped embed: ${built.unplaced.join(', ')}`);

    const misses = [];
    const srv = await S.serve(built.root, (rel) => misses.push(rel));

    const browser = await chromium.launch();
    // `viewport`, NOT `viewportSize` — the latter is the getter's name and is
    // silently IGNORED as a launch option, leaving Chromium's 1280x720 default.
    // Measuring French overflow at a viewport wider than the plugin ships is
    // the exact failure this file exists to catch.
    const ctx  = await browser.newContext({ viewport: { width: size.w, height: size.h } });
    const page = await ctx.newPage();

    const pageErrors = [];
    page.on('pageerror', (e) => pageErrors.push(String(e && e.message ? e.message : e)));

    await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
    await page.waitForTimeout(700);

    // ── optional per-plugin states ─────────────────────────────────────────
    const statesFile = path.join(S.pluginRoot(plugin, repoRoot), 'tests', 'i18n-states.json');
    let states = [{ name: 'default', click: null }];
    if (fs.existsSync(statesFile)) {
        try {
            const extra = JSON.parse(fs.readFileSync(statesFile, 'utf8'));
            if (Array.isArray(extra)) states = states.concat(extra);
            console.log(`   states: default + ${states.length - 1} from tests/i18n-states.json`);
        } catch (e) {
            console.log(`   WARNING tests/i18n-states.json did not parse: ${e.message}`);
        }
    } else {
        console.log('   states: default only (no tests/i18n-states.json) — '
                  + 'unseen labels are REPORTED below rather than silently skipped');
    }

    const hasSwitch = await page.evaluate(() => typeof window.__setLanguage === 'function');
    check(hasSwitch, 'window.__setLanguage is exposed — the canon block is present and ran');

    const labelCount = await page.evaluate(() => document.querySelectorAll('[data-i18n]').length);

    if (labelCount === 0) {
        console.log('\n  NOTHING TO MEASURE: this plugin has zero [data-i18n] elements.');
        console.log('  Exiting NON-ZERO deliberately. A gate that passes because there is nothing');
        console.log('  to check is the vacuous pass this whole design exists to prevent — the');
        console.log('  plugin has not been retrofitted onto canon v2 yet.');
        await browser.close(); await srv.close();
        fs.rmSync(built.root, { recursive: true, force: true });
        process.exit(1);
    }

    console.log(`   ${labelCount} [data-i18n] element(s) to measure\n`);

    const neverVisible = new Set();
    const seenVisible  = new Set();
    let stateMechanism = null;

    for (const state of states) {
        console.log(`-- state: ${state.name}`);

        // THREE ACTIONS, NOT ONE. `click` alone cannot reach two shapes that
        // are ordinary in this repo, and a label the gate cannot reveal is a
        // label it certifies by never looking at it:
        //
        //   dblclick — the house idiom for a popover (O-Octagon's
        //              speaker -> output assignment, v1.1.0). A single click on
        //              the same glyph does something else entirely.
        //   eval     — a state the PLUGIN owns, reached through the plugin's
        //              OWN committed ui-stub hook. O-Octagon's three frame
        //              banners are rendered from getStatus() and cannot be
        //              clicked into existence; its stub already exposes
        //              window.__OCTAGON_STUB__.setStatus for exactly this.
        //
        // eval runs the plugin's own hook, in the plugin's own stub, from the
        // plugin's own tests/ file. It is not a back door into the page: a
        // states file that drove the DOM directly would be measuring a state
        // the plugin cannot actually produce, and that is worth failing on
        // review rather than forbidding here.
        if (state.click || state.dblclick) {
            const sel = state.click || state.dblclick;
            const el = await page.$(sel);
            if (!el) { check(false, `[state ${state.name}] selector ${sel} exists`); continue; }
            if (state.dblclick) await el.dblclick({ force: true });
            else                await el.click({ force: true });
            await page.waitForTimeout(250);
        }
        if (state.eval) {
            try { await page.evaluate((src) => { (0, eval)(src); }, state.eval); }
            catch (e) { check(false, `[state ${state.name}] eval ran — ${e.message}`); continue; }
            await page.waitForTimeout(350);
        }

        // TWO SWEEPS, NOT ONE INTERLEAVED SWEEP. Every `before` snapshot is
        // taken before ANY state pass has run, and only then does the state
        // pass run for each language.
        //
        // Interleaving them — probe en, drive state, probe fr — silently
        // measures the two languages at DIFFERENT PARAMETER VALUES, because the
        // state pass moves every slider, toggle and combo and does not put them
        // back. Assertion 7 then reports every knob stem in the plugin as
        // "moved between English and French" when what actually moved was the
        // parameter behind it. On O-MultiBandCompressor that was 53 phantom
        // rows, all of them rotated .knob-stem boxes.
        //
        // It stayed invisible through Stage F because it only bites where the
        // stub really drives parameters: O-Tapestop's own committed ui-stub
        // exposes no window.__stubStates, so its state pass was events-only and
        // moved nothing. The GENERIC stub does drive them, so every plugin
        // without a hand-written stub would have hit this.
        const snaps = {};
        for (const lang of ['en', 'fr']) {
            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(180);
            snaps[lang] = { before: await page.evaluate(PROBE) };
            for (const l of snaps[lang].before.labels) if (l.visible) seenVisible.add(l.path);
        }

        // ── THE ANIMATION CONTROL — an EN -> EN pass, before any state pass ──
        // Assertion 7 reads "any element that moved was pushed by a French
        // string", and that only holds if the page HOLDS STILL. The PROBE
        // freezes declarative animation — SMIL via pauseAnimations(), the Web
        // Animations API via getAnimations() — but neither API can reach a
        // requestAnimationFrame loop writing attributes from the wall clock,
        // because that is the page's own code and not an Animation object.
        //
        // O-Chorus is the proof. Its #lfo-dot is an SVG circle whose cx/cy are
        // rewritten every frame by lfoLoop(timestamp), so it has no fixed
        // rectangle at all: the diff reported dy=-24.0 on one run and dy=+0.4
        // on the next, with every French string identical. Held at ENGLISH on
        // both sides it reported the same element moving dx=4.9 dy=19.6, which
        // is the `3be873eb` signature — a failure that reproduces with the
        // language held constant is never a French failure.
        //
        // So the animated set is MEASURED rather than declared: probe English
        // twice more and treat any element whose rectangle is not stable across
        // the three samples as animated. That covers any mechanism, including
        // ones no pause API knows about, and it can only make the gate MORE
        // permissive — an element genuinely pushed by French does not move
        // EN -> EN, so it stays asserted.
        //
        // The hole this leaves, named rather than glossed: an element that both
        // animates AND is pushed by French is excluded. The NOTE below prints
        // its EN -> FR delta beside its EN -> EN spread so it stays visible.
        //
        // Two extra samples, not one: a periodic animation can land on the same
        // phase twice. The spread is taken across all three.
        const enControl = [];
        await page.evaluate((l) => window.__setLanguage(l), 'en');
        await page.waitForTimeout(180);
        enControl.push(await page.evaluate(PROBE));
        await page.waitForTimeout(150);
        enControl.push(await page.evaluate(PROBE));

        const animated = new Map();   // path -> the EN -> EN spread, per axis
        {
            const byPath = new Map();
            for (const s of [snaps.en.before, ...enControl])
                for (const o of s.others) {
                    if (!byPath.has(o.path)) byPath.set(o.path, []);
                    byPath.get(o.path).push(o.rect);
                }
            for (const [p, rects] of byPath) {
                if (rects.length < 2) continue;
                const spread = (k) => Math.max(...rects.map((r) => r[k])) - Math.min(...rects.map((r) => r[k]));
                const d = { dx: spread('x'), dy: spread('y'), dw: spread('w'), dh: spread('h') };
                if (Math.max(d.dx, d.dy, d.dw, d.dh) > TOL) animated.set(p, d);
            }
        }

        // ── 3. dataset.label === textContent, AFTER A STATE PASS ───────────
        for (const lang of ['en', 'fr']) {
            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(180);
            stateMechanism = await page.evaluate(STATE_PASS);
            await page.waitForTimeout(180);
            snaps[lang].after = await page.evaluate(PROBE);
        }

        const en = snaps.en.before, fr = snaps.fr.before;

        // ── 1. every label has text, no surviving {token} ──────────────────
        for (const lang of ['en', 'fr']) {
            const s = snaps[lang].before;
            const empty  = s.labels.filter((l) => l.visible && l.text.trim() === '');
            const tokens = s.labels.filter((l) => /\{\w+\}/.test(l.text));
            check(empty.length === 0,
                `[1][${lang}] every visible [data-i18n] renders non-empty text`
                + (empty.length ? ` — ${empty.length} empty: ${empty.slice(0, 4).map((l) => l.key).join(', ')}` : ''));
            check(tokens.length === 0,
                `[1][${lang}] no {token} placeholder survives into rendered text`
                + (tokens.length ? ` — ${tokens.length}: ${tokens.slice(0, 4).map((l) => `${l.key}="${l.text.slice(0, 24)}"`).join(', ')}` : ''));
        }

        // ── 2. VACUITY GUARD — French actually rendered ────────────────────
        // Without this, a run in which __setLanguage silently did nothing
        // measures English twice and reports a confident, worthless pass.
        // Stage D proved this catches a real no-op on all three plugins it was
        // tried against.
        const enByKey = new Map(en.labels.map((l) => [l.path, l.text]));
        const differing = fr.labels.filter((l) => enByKey.get(l.path) !== l.text).length;
        const fraction  = fr.labels.length ? differing / fr.labels.length : 0;
        check(fraction >= MIN_LANG_DIFF_FRACTION,
            `[2][vacuity] French actually rendered — ${differing}/${fr.labels.length} labels `
            + `(${(fraction * 100).toFixed(0)}%) differ from English, need >= ${MIN_LANG_DIFF_FRACTION * 100}%`);

        // Attributes too: a keyed aria-label that never changes is a sweep that
        // silently skipped the attribute pass.
        if (en.attrKeyed.length) {
            const enAttr = new Map(en.attrKeyed.map((a) => [a.path, JSON.stringify([a.aria, a.placeholder, a.alt])]));
            const attrDiff = fr.attrKeyed.filter((a) => enAttr.get(a.path) !== JSON.stringify([a.aria, a.placeholder, a.alt])).length;
            check(attrDiff > 0,
                `[2][vacuity] keyed ATTRIBUTES actually changed language — ${attrDiff}/${fr.attrKeyed.length}`);
        }

        // ── 3. the ownership mirror, after init/switch AND a state pass ────
        for (const lang of ['en', 'fr']) {
            for (const [when, snap] of [['after switch', snaps[lang].before], ['after a state pass', snaps[lang].after]]) {
                const broken = snap.labels.filter((l) => l.datasetLabel !== l.text);
                check(broken.length === 0,
                    `[3][${lang}] dataset.label === textContent for every [data-i18n] ${when} `
                    + `(the systemic form of pattern_js_state_updater_overwrites_html_labels)`
                    + (broken.length ? ` — ${broken.length} broken: ${broken.slice(0, 4).map((l) => `${l.key}: label=${JSON.stringify(String(l.datasetLabel).slice(0, 16))} text=${JSON.stringify(l.text.slice(0, 16))}`).join(' | ')}` : ''));
            }
        }

        // ── 4. no clip. LEAF label elements only ───────────────────────────
        // scrollHeight/scrollWidth lie about a container: they do not cross a
        // `flex: 1` stage and are clamped on a grid container. A gate built on
        // them for containers would certify a real overflow.
        for (const lang of ['en', 'fr']) {
            const clipped = snaps[lang].before.labels.filter((l) =>
                l.visible && l.leaf && !/visible/.test(l.overflow) && l.scrollWidth > l.clientWidth + 1);
            check(clipped.length === 0,
                `[4][${lang}] no leaf label is clipped by its own overflow`
                + (clipped.length ? ` — ${clipped.length}: ${clipped.slice(0, 4).map((l) => `${l.key} ${l.scrollWidth}>${l.clientWidth}`).join(', ')}` : ''));

            // ── and the half the clip check cannot see ─────────────────────
            // A leaf with overflow:visible never reports a scrollWidth larger
            // than its clientWidth, so the check above passes on a label whose
            // text is spilling out of its button in plain sight. This one
            // measures the text and asks whether it fits.
            const spillable = snaps[lang].before.labels.filter((l) =>
                l.visible && l.leaf && l.textWidth != null && !l.inlineBox);
            const skippedInline = snaps[lang].before.labels.filter((l) =>
                l.visible && l.leaf && l.textWidth != null && l.inlineBox);
            if (lang === 'en' && skippedInline.length)
                console.log(`   NOTE: [4] ${skippedInline.length} label(s) are non-replaced INLINE boxes, `
                    + 'which have no content box of their own (clientWidth is 0 by definition). '
                    + 'The text-spill check cannot express itself about them and skips them; '
                    + `assertions 5, 6 and 7 still measure their rects: ${skippedInline.slice(0, 5).map((l) => l.key).join(', ')}`);
            const spilling = spillable.filter((l) => l.textWidth > l.contentWidth + 0.5);
            check(spilling.length === 0,
                `[4][${lang}] no leaf label's TEXT is wider than its own content box `
                + `(the case overflow:visible hides from scrollWidth)`
                + (spilling.length ? ` — ${spilling.length}: ${spilling.slice(0, 4).map((l) =>
                    `${l.key} "${l.text.slice(0, 14)}" ${l.textWidth.toFixed(1)}>${l.contentWidth.toFixed(1)}`).join(', ')}` : ''));

            // ── the OTHER half the clip check cannot see: the wrap ─────────
            // A caption that is too long does not always overrun sideways. Give
            // it a space to break at and it wraps instead, stacking line boxes
            // downward out of a fixed-height control while every width stays
            // inside its box. Same skip list as the width check, and for the
            // same reason: an inline box has no content box of its own.
            const wrapping = spillable.filter((l) =>
                l.textHeight != null && l.textLines > 1 && l.textHeight > l.contentHeight + 0.5);

            // The single-line overhang is NOT a failure — but it is not hidden
            // either, or this assertion would be the silent kind it exists to
            // replace. It is reported once, in English, with its numbers.
            const overhang = spillable.filter((l) =>
                l.textHeight != null && l.textLines <= 1 && l.textHeight > l.contentHeight + 0.5);
            if (lang === 'en' && overhang.length)
                console.log(`   NOTE: [4] ${overhang.length} single-line label(s) have a line box taller `
                    + 'than their content box. That is a tight line-height, not a wrap, so it is not a '
                    + `failure: ${overhang.slice(0, 5).map((l) =>
                        `${l.key} ${l.textHeight.toFixed(1)}>${l.contentHeight.toFixed(1)}`).join(', ')}`);
            check(wrapping.length === 0,
                `[4][${lang}] no leaf label's TEXT is taller than its own content box `
                + `(the caption that WRAPS out of a fixed-height control)`
                + (wrapping.length ? ` — ${wrapping.length}: ${wrapping.slice(0, 4).map((l) =>
                    `${l.key} "${l.text.slice(0, 14)}" ${l.textHeight.toFixed(1)}>${l.contentHeight.toFixed(1)}`).join(', ')}` : ''));
        }

        // ── 5. no label spills its offsetParent MORE in French ─────────────
        // Measured as a DELTA against English, not as an absolute.
        //
        // Stage F, on O-Tapestop: the absolute form of this assertion failed on
        // three `.group-label` panel legends — IN ENGLISH. They are
        // `position: absolute; top: -9px` and deliberately straddle the panel
        // border, the fieldset-legend idiom, which is an authored layout the
        // developer sees on screen every day. An i18n gate that fails on it is
        // not reporting a French problem; it is arguing with the design, and it
        // would do so on most plugins in the suite. A gate that is red for a
        // whole rollout stops being read.
        //
        // The failure this assertion actually exists to catch is a label that
        // GREW out of its cell in French. That is exactly `spill(fr) >
        // spill(en)`. The English spill is reported so it is never silent.
        const spillOf = (l) => {
            if (!l.visible || !l.parentPad) return 0;
            const p = l.parentPad, b = l.rect;
            return Math.max(0, p.x - b.x, p.y - b.y,
                               (b.x + b.w) - (p.x + p.w), (b.y + b.h) - (p.y + p.h));
        };
        const enSpill = new Map(en.labels.map((l) => [l.path, spillOf(l)]));
        const grew = fr.labels.filter((l) => spillOf(l) > (enSpill.get(l.path) || 0) + TOL);
        check(grew.length === 0,
            `[5] no label spills its offsetParent's padding box MORE in French than in English`
            + (grew.length ? ` — ${grew.length}: ${grew.slice(0, 4).map((l) =>
                `${l.key} ${(enSpill.get(l.path) || 0).toFixed(1)}px -> ${spillOf(l).toFixed(1)}px`).join(', ')}` : ''));

        const authored = en.labels.filter((l) => spillOf(l) > TOL);
        if (authored.length)
            console.log(`   NOTE: [5] ${authored.length} label(s) already overhang their offsetParent in ENGLISH `
                + `— an authored layout, reported not asserted: `
                + authored.slice(0, 4).map((l) => `${l.key} ${spillOf(l).toFixed(1)}px`).join(', '));

        // ── 6. nothing crosses the shipping frame MORE in French ───────────
        //
        // A DELTA against English, for the same reason assertion 5 is one, and
        // discovered the same way — by meeting a plugin whose authored English
        // layout already violates the absolute form.
        //
        // Stage I, on O-Orbit: its #controls-container is `flex: 1` with
        // `overflow-y: auto` and its three parameter groups need 555px in a
        // 226px pane, so eleven labels sit BELOW the 600px frame at rest. That
        // is the plugin's D4 resizable design, it is byte-identical at the
        // pre-retrofit commit, and the eleven are the SAME eleven with the SAME
        // overshoot to the decimal in both languages — French contributes
        // exactly zero. The absolute form reported 8 failures per run on a
        // plugin whose French geometry is perfect, which is the gate arguing
        // with the design rather than reporting a French problem.
        //
        // The failure this assertion exists to catch is a label pushed out of
        // the frame BY French, and that is `overshoot(fr) > overshoot(en)`. It
        // is not weakened by the change: the same run that produced the eleven
        // also caught label.import leaving the right edge at 803px in French
        // and 731+58 in English, and the delta form still fails on it (NC-9).
        //
        // The English overshoot is REPORTED so it can never be silent, and the
        // per-language document scroll-extent checks below stay ABSOLUTE — a
        // page whose own scroll extent exceeds its frame is a different and
        // genuinely broken thing, and O-Orbit passes those in both languages.
        const frameOver = (l) => {
            if (!l.visible) return 0;
            const b = l.rect;
            return Math.max(0, -b.x, -b.y, (b.x + b.w) - size.w, (b.y + b.h) - size.h);
        };
        const enOver = new Map(en.labels.map((l) => [l.path, frameOver(l)]));
        const pushedOut = fr.labels.filter((l) => frameOver(l) > (enOver.get(l.path) || 0) + TOL);
        check(pushedOut.length === 0,
            `[6] no label crosses the ${size.w} x ${size.h} frame MORE in French than in English`
            + (pushedOut.length ? ` — ${pushedOut.length}: ${pushedOut.slice(0, 4).map((l) =>
                `${l.key} ${(enOver.get(l.path) || 0).toFixed(1)}px -> ${frameOver(l).toFixed(1)}px `
                + `@${l.rect.x.toFixed(0)},${l.rect.y.toFixed(0)} ${l.rect.w.toFixed(0)}x${l.rect.h.toFixed(0)}`).join(' | ')}` : ''));

        const authoredOut = en.labels.filter((l) => frameOver(l) > TOL);
        if (authoredOut.length)
            console.log(`   NOTE: [6] ${authoredOut.length} label(s) already fall outside the `
                + `${size.w} x ${size.h} frame in ENGLISH — an authored layout (a scrolling pane, `
                + `a collapsed panel), reported not asserted: `
                + authoredOut.slice(0, 4).map((l) => `${l.key} ${frameOver(l).toFixed(1)}px`).join(', '));

        for (const lang of ['en', 'fr']) {
            const s = snaps[lang].before;
            // documentElement, NOT max(documentElement, body). body.scrollWidth
            // counts a child that the real scroll container clips: Stage F found
            // O-Tapestop's decorative .botanical-overlay running 20 px past the
            // frame, giving body 880 where documentElement reports 860 — in
            // BOTH languages, and identically at the pre-retrofit commit. The
            // committed tests/ui_tooltip_clamp_check.js asserts documentElement
            // and passes; when this tool disagreed with that gate about the same
            // plugin's geometry, the tool was wrong.
            check(s.docScroll.dw <= size.w + 1 && s.docScroll.dh <= size.h + 1,
                `[6][${lang}] the document's own scroll extent stays inside the frame — `
                + `${s.docScroll.dw} x ${s.docScroll.dh} vs ${size.w} x ${size.h}`);
        }

        check(fr.docScroll.dw <= en.docScroll.dw + TOL && fr.docScroll.dh <= en.docScroll.dh + TOL,
            `[6] French does not enlarge the document's scroll extent — `
            + `en ${en.docScroll.dw} x ${en.docScroll.dh}, fr ${fr.docScroll.dw} x ${fr.docScroll.dh}`);
        if (en.docScroll.bw > en.docScroll.dw + 1 || en.docScroll.bh > en.docScroll.dh + 1)
            console.log(`   NOTE: [6] body scrolls to ${en.docScroll.bw} x ${en.docScroll.bh} where documentElement `
                + `reports ${en.docScroll.dw} x ${en.docScroll.dh} — a child overflows a clipped ancestor. `
                + `Reported, not asserted: it is identical in both languages.`);

        // ── 7. THE GEOMETRY DIFF — the primary detector ────────────────────
        // The frame is fixed, so any element that is NOT a label and NOT inside
        // one and MOVED between the two passes was pushed by a French string.
        // getBoundingClientRect only, so it is immune to the scrollHeight traps
        // assertion 4 has to work around — and it is the only assertion that
        // sees a WRAP, which is what happens on the nine plugins that have
        // neither `nowrap` nor `ellipsis`.
        const enOthers = new Map(en.others.map((o) => [o.path, o.rect]));
        const moved = [];
        const movedButAnimated = [];    // excluded from the assertion, never silent
        for (const o of fr.others) {
            const e = enOthers.get(o.path);
            if (!e) continue;               // appeared/disappeared with language: reported separately
            const d = { dx: o.rect.x - e.x, dy: o.rect.y - e.y, dw: o.rect.w - e.w, dh: o.rect.h - e.h };
            if (Math.abs(d.dx) > TOL || Math.abs(d.dy) > TOL || Math.abs(d.dw) > TOL || Math.abs(d.dh) > TOL)
                (animated.has(o.path) ? movedButAnimated : moved).push({ path: o.path, ...d });
        }
        check(moved.length === 0,
            `[7][GEOMETRY DIFF] no non-label element moved between English and French at a fixed frame`
            + (animated.size ? ` (${animated.size} animated element(s) excluded — see NOTE)` : '')
            + (moved.length ? ` — ${moved.length} moved:\n` + moved.slice(0, 12).map((m) =>
                `        ${m.path}  dx=${m.dx.toFixed(1)} dy=${m.dy.toFixed(1)} dw=${m.dw.toFixed(1)} dh=${m.dh.toFixed(1)}`).join('\n') : ''));

        if (animated.size)
            console.log(`   NOTE: [7] ${animated.size} element(s) MOVE WITH THE LANGUAGE HELD CONSTANT — `
                + `an animation this page drives itself, not a French push. Excluded from the diff and `
                + `reported here with the EN -> EN spread: `
                + [...animated.entries()].slice(0, 6).map(([p, d]) =>
                    `${p} dx=${d.dx.toFixed(1)} dy=${d.dy.toFixed(1)} dw=${d.dw.toFixed(1)} dh=${d.dh.toFixed(1)}`).join(' | '));
        for (const m of movedButAnimated)
            console.log(`   NOTE: [7] ${m.path} is ANIMATED and also differs EN -> FR by `
                + `dx=${m.dx.toFixed(1)} dy=${m.dy.toFixed(1)} dw=${m.dw.toFixed(1)} dh=${m.dh.toFixed(1)} — `
                + `EN -> EN spread dx=${animated.get(m.path).dx.toFixed(1)} dy=${animated.get(m.path).dy.toFixed(1)} `
                + `dw=${animated.get(m.path).dw.toFixed(1)} dh=${animated.get(m.path).dh.toFixed(1)}. A French push `
                + `hiding inside an animation would show an EN -> FR delta well OUTSIDE that spread.`);

        const appeared = fr.others.filter((o) => !enOthers.has(o.path)).map((o) => o.path);
        const frPaths  = new Set(fr.others.map((o) => o.path));
        const vanished = en.others.filter((o) => !frPaths.has(o.path)).map((o) => o.path);
        check(appeared.length === 0 && vanished.length === 0,
            `[7][GEOMETRY DIFF] the visible element SET is identical in both languages`
            + (appeared.length ? ` — ${appeared.length} appeared in fr: ${appeared.slice(0, 3).join(', ')}` : '')
            + (vanished.length ? ` — ${vanished.length} vanished in fr: ${vanished.slice(0, 3).join(', ')}` : ''));

        // ── 8. no NEW overlap ──────────────────────────────────────────────
        const enVis = en.labels.filter((l) => l.visible);
        const frByPath = new Map(fr.labels.map((l) => [l.path, l]));
        const newOverlaps = [];
        const crossLayer = [];
        for (let i = 0; i < enVis.length; ++i) {
            for (let j = i + 1; j < enVis.length; ++j) {
                if (overlaps(enVis[i].rect, enVis[j].rect)) continue;
                const a = frByPath.get(enVis[i].path), b = frByPath.get(enVis[j].path);
                if (!a || !b || !a.visible || !b.visible) continue;
                if (!overlaps(a.rect, b.rect)) continue;
                // Different PAINT LAYERS — an opaque floating panel over the
                // page. Reported, never silently dropped.
                if ((enVis[i].overlay || null) !== (enVis[j].overlay || null)) {
                    crossLayer.push(`${enVis[i].key} x ${enVis[j].key}`);
                    continue;
                }
                newOverlaps.push(`${enVis[i].key} x ${enVis[j].key}`);
            }
        }
        if (crossLayer.length)
            console.log(`   NOTE: [8] ${crossLayer.length} pair(s) intersect in French but sit in DIFFERENT `
                + `paint layers — an opaque floating panel over the page, which cannot visually collide with `
                + `what it covers: ${crossLayer.slice(0, 4).join(', ')}`);
        check(newOverlaps.length === 0,
            `[8] two labels disjoint in English do not intersect in French`
            + (newOverlaps.length ? ` — ${newOverlaps.length}: ${newOverlaps.slice(0, 4).join(', ')}` : ''));

        // ── 8b. a label must not grow into a NON-label element ─────────────
        // THE THIRD CLIFF. Assertions 4/5 catch a caption that SPILLS its own
        // box; assertion 7 catches one that PUSHES a sibling. A caption in a box
        // that is absolutely positioned, width-pinned and height-free does
        // NEITHER — it wraps, grows downward inside its own box, exceeds no
        // width, exceeds no content height (the box grew with it), and pushes
        // nothing because absolute positioning takes it out of flow. It simply
        // lands ON TOP of whatever is beneath it.
        //
        // O-AnalogEQ is the proof. Its four .band-label captions are
        // `position: absolute` with an inline `width: 85px` and no fixed
        // height. Planting `PLATEAU BF` on label.band.lf with the caption's
        // `nowrap` removed grows it dh=+13.00px, reaching y=86 into a knob ring
        // that begins at y=75 — and the whole gate printed ALL CHECKS PASSED.
        // Assertion 8 could not see it either, because a knob ring is not a
        // label and 8 compares labels to labels.
        //
        // Same form as 8: only a NEW intersection counts. A label already over
        // something in English is an authored layout, and a label growing into
        // its own ANCESTOR is not a collision at all — hence the ancestor list.
        // Elements inside a label are already excluded from `others`.
        // The PAINT LAYER rule from assertion 8 applies here for the same reason
        // and was found the same way — by a shipped plugin going red on a
        // collision no user can see. O-Contrabass's `help-toggle` caption lives
        // inside the settings popover; the knob-control it "intersects" is on
        // the page UNDERNEATH an opaque panel doing exactly what a panel is for.
        // Same rule, same reporting: only a pair spanning the boundary is
        // skipped, and every skip is printed.
        const frOthers = new Map(fr.others.map((o) => [o.path, o]));
        const grewInto = [];
        const grewCrossLayer = [];
        const inertSkips = new Set();
        for (const L of enVis) {
            const fl = frByPath.get(L.path);
            if (!fl || !fl.visible) continue;
            // ONLY a label that GREW. This assertion exists for the caption that
            // wraps and expands over its neighbour; a label that merely MOVED is
            // a different phenomenon and not evidence of a collision.
            //
            // O-IntonationPad proves the distinction matters. Its `Tuning` tab
            // becomes `Gamme` — French SHORTER by a character — the tab row
            // re-centres, the button shifts, and it grazes two panel edges it
            // previously cleared. Nothing grew, nothing overlapped anything a
            // user can see, and a shipped green plugin went red. Contract §8
            // says a gate is never red for a whole rollout.
            if (fl.rect.w <= L.rect.w + TOL && fl.rect.h <= L.rect.h + TOL) continue;
            for (const o of en.others) {
                if (L.ancestors && L.ancestors.includes(o.path)) continue;
                // `pointer-events: none` is DECORATION the user cannot reach:
                // O-Bassoon's .botanical-overlay is a full-bleed <img> at
                // opacity 0.18 behind the whole page, and a caption reaching it
                // is not a collision with anything. Recorded as a skip below so
                // the exemption can never be silent.
                if (o.inert) { inertSkips.add(o.path); continue; }
                if (overlaps(L.rect, o.rect)) continue;      // together in English already
                const fo = frOthers.get(o.path);
                if (!fo || !overlaps(fl.rect, fo.rect)) continue;
                ((L.overlay || null) !== (o.overlay || null) ? grewCrossLayer : grewInto)
                    .push(`${L.key} x ${o.path}`);
            }
        }
        if (inertSkips.size)
            console.log(`   NOTE: [8b] ${inertSkips.size} non-label element(s) skipped as DECORATION `
                + `(pointer-events: none), which a caption cannot collide with: `
                + `${[...inertSkips].slice(0, 3).join(', ')}`);
        if (grewCrossLayer.length)
            console.log(`   NOTE: [8b] ${grewCrossLayer.length} label/non-label pair(s) intersect in French `
                + `across DIFFERENT paint layers — an opaque floating panel over the page, which cannot `
                + `visually collide with what it covers: ${grewCrossLayer.slice(0, 4).join(', ')}`);
        check(grewInto.length === 0,
            `[8b] no label intersects a NON-label element it cleared in English`
            + (grewInto.length ? ` — ${grewInto.length}: ${grewInto.slice(0, 4).join(', ')}` : ''));

        console.log(`   state-update pass driven via: ${stateMechanism}`);
    }

    // ── coverage report ────────────────────────────────────────────────────
    const allPaths = await page.evaluate(() => {
        const pathOf = (el) => {
            const bits = [];
            let n = el;
            while (n && n.nodeType === 1) {
                if (n.id) { bits.unshift('#' + n.id); break; }
                const p = n.parentElement;
                const i = p ? [...p.children].indexOf(n) + 1 : 1;
                bits.unshift(`${n.tagName.toLowerCase()}${n.className && typeof n.className === 'string'
                    ? '.' + n.className.trim().split(/\s+/).slice(0, 2).join('.') : ''}:nth-child(${i})`);
                n = p;
            }
            return bits.join('>');
        };
        return [...document.querySelectorAll('[data-i18n]')].map(pathOf);
    });
    for (const p of allPaths) if (!seenVisible.has(p)) neverVisible.add(p);

    console.log('\n-- coverage');
    console.log(`  ${seenVisible.size} of ${allPaths.length} [data-i18n] elements were VISIBLE in at least one state`);
    if (neverVisible.size) {
        console.log(`  ${neverVisible.size} never became visible and were therefore NEVER MEASURED:`);
        for (const p of [...neverVisible].slice(0, 12)) console.log(`      ${p}`);
        console.log('  Add plugins/' + plugin + '/tests/i18n-states.json to drive the states that reveal them.');
        console.log('  Reported rather than asserted: an unmeasured label is a coverage hole, not a');
        console.log('  geometry failure, and conflating the two would make the gate lie in both directions.');
    }

    check(pageErrors.length === 0,
        `no uncaught page error during the sweep`
        + (pageErrors.length ? ` — ${pageErrors.length}: ${pageErrors[0].slice(0, 120)}` : ''));

    const realMisses = [...new Set(misses)].filter((m) => !/favicon/.test(m));
    check(realMisses.length === 0,
        `every requested resource was served`
        + (realMisses.length ? ` — 404: ${realMisses.slice(0, 5).join(', ')}` : ''));

    if (verbose) console.log(`\n  served root: ${built.root}`);

    await browser.close();
    await srv.close();
    if (!verbose) fs.rmSync(built.root, { recursive: true, force: true });

    console.log(`\n${failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} FAILED ==`}`);
    process.exit(failed);
})().catch((e) => {
    console.error('check-ui-labels: harness failure —', e);
    process.exit(1);
});
