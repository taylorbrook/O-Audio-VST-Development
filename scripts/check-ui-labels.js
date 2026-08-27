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
    const r = (el) => {
        const b = el.getBoundingClientRect();
        return { x: b.x, y: b.y, w: b.width, h: b.height };
    };

    const pathOf = (el) => {
        const bits = [];
        let n = el;
        while (n && n.nodeType === 1 && bits.length < 6) {
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
        docScroll: {
            w: Math.max(document.documentElement.scrollWidth, document.body ? document.body.scrollWidth : 0),
            h: Math.max(document.documentElement.scrollHeight, document.body ? document.body.scrollHeight : 0),
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
            overflow: `${cs.overflow} ${cs.overflowX}`,
            whiteSpace: cs.whiteSpace,
            leaf: el.children.length === 0,
            scrollWidth: el.scrollWidth,
            clientWidth: el.clientWidth,
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
        out.others.push({ path: pathOf(el), rect: r(el) });
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

        if (state.click) {
            const el = await page.$(state.click);
            if (!el) { check(false, `[state ${state.name}] selector ${state.click} exists`); continue; }
            await el.click({ force: true });
            await page.waitForTimeout(250);
        }

        const snaps = {};
        for (const lang of ['en', 'fr']) {
            await page.evaluate((l) => window.__setLanguage(l), lang);
            await page.waitForTimeout(180);

            const beforeState = await page.evaluate(PROBE);

            // ── 3. dataset.label === textContent, AFTER A STATE PASS ───────
            stateMechanism = await page.evaluate(STATE_PASS);
            await page.waitForTimeout(180);
            const afterState = await page.evaluate(PROBE);

            snaps[lang] = { before: beforeState, after: afterState };

            for (const l of beforeState.labels) if (l.visible) seenVisible.add(l.path);
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
        }

        // ── 5. no spill past the offsetParent's padding box ────────────────
        for (const lang of ['en', 'fr']) {
            const spilled = snaps[lang].before.labels.filter((l) => {
                if (!l.visible || !l.parentPad) return false;
                const p = l.parentPad, b = l.rect;
                return b.x < p.x - TOL || b.y < p.y - TOL
                    || b.x + b.w > p.x + p.w + TOL || b.y + b.h > p.y + p.h + TOL;
            });
            check(spilled.length === 0,
                `[5][${lang}] every label stays inside its offsetParent's padding box`
                + (spilled.length ? ` — ${spilled.length}: ${spilled.slice(0, 4).map((l) => l.key).join(', ')}` : ''));
        }

        // ── 6. nothing crosses the shipping frame ──────────────────────────
        for (const lang of ['en', 'fr']) {
            const s = snaps[lang].before;
            const out = s.labels.filter((l) => l.visible
                && (l.rect.x < -TOL || l.rect.y < -TOL
                 || l.rect.x + l.rect.w > size.w + TOL || l.rect.y + l.rect.h > size.h + TOL));
            check(out.length === 0,
                `[6][${lang}] no label rect crosses the ${size.w} x ${size.h} frame`
                + (out.length ? ` — ${out.length}: ${out.slice(0, 4).map((l) => `${l.key} @${l.rect.x.toFixed(0)},${l.rect.y.toFixed(0)} ${l.rect.w.toFixed(0)}x${l.rect.h.toFixed(0)}`).join(' | ')}` : ''));
            check(s.docScroll.w <= size.w + 1 && s.docScroll.h <= size.h + 1,
                `[6][${lang}] the document's own scroll extent stays inside the frame — `
                + `${s.docScroll.w} x ${s.docScroll.h} vs ${size.w} x ${size.h}`);
        }

        // ── 7. THE GEOMETRY DIFF — the primary detector ────────────────────
        // The frame is fixed, so any element that is NOT a label and NOT inside
        // one and MOVED between the two passes was pushed by a French string.
        // getBoundingClientRect only, so it is immune to the scrollHeight traps
        // assertion 4 has to work around — and it is the only assertion that
        // sees a WRAP, which is what happens on the nine plugins that have
        // neither `nowrap` nor `ellipsis`.
        const enOthers = new Map(en.others.map((o) => [o.path, o.rect]));
        const moved = [];
        for (const o of fr.others) {
            const e = enOthers.get(o.path);
            if (!e) continue;               // appeared/disappeared with language: reported separately
            const d = { dx: o.rect.x - e.x, dy: o.rect.y - e.y, dw: o.rect.w - e.w, dh: o.rect.h - e.h };
            if (Math.abs(d.dx) > TOL || Math.abs(d.dy) > TOL || Math.abs(d.dw) > TOL || Math.abs(d.dh) > TOL)
                moved.push({ path: o.path, ...d });
        }
        check(moved.length === 0,
            `[7][GEOMETRY DIFF] no non-label element moved between English and French at a fixed frame`
            + (moved.length ? ` — ${moved.length} moved:\n` + moved.slice(0, 12).map((m) =>
                `        ${m.path}  dx=${m.dx.toFixed(1)} dy=${m.dy.toFixed(1)} dw=${m.dw.toFixed(1)} dh=${m.dh.toFixed(1)}`).join('\n') : ''));

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
        for (let i = 0; i < enVis.length; ++i) {
            for (let j = i + 1; j < enVis.length; ++j) {
                if (overlaps(enVis[i].rect, enVis[j].rect)) continue;
                const a = frByPath.get(enVis[i].path), b = frByPath.get(enVis[j].path);
                if (!a || !b || !a.visible || !b.visible) continue;
                if (overlaps(a.rect, b.rect)) newOverlaps.push(`${enVis[i].key} x ${enVis[j].key}`);
            }
        }
        check(newOverlaps.length === 0,
            `[8] two labels disjoint in English do not intersect in French`
            + (newOverlaps.length ? ` — ${newOverlaps.length}: ${newOverlaps.slice(0, 4).join(', ')}` : ''));

        console.log(`   state-update pass driven via: ${stateMechanism}`);
    }

    // ── coverage report ────────────────────────────────────────────────────
    const allPaths = await page.evaluate(() => {
        const pathOf = (el) => {
            const bits = [];
            let n = el;
            while (n && n.nodeType === 1 && bits.length < 6) {
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
