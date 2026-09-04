/*
   This file is part of O-AnalogSaturation, an Ouaricon Audio plugin.
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
    O-AnalogSaturation — the gate that HOVERS.

    Manual run:  node plugins/O-AnalogSaturation/tests/ui_tip_render_check.js
    Exit code = number of failed assertions (0 = all pass).

    ── WHY THIS FILE EXISTS ────────────────────────────────────────────────────

    NO OTHER GATE IN THIS REPO CAN SEE A RENDERED TOOLTIP.

      check-i18n.js      reads the table statically. Its assertion 2 counts
                         TIP_BINDINGS rows. Six rows over a page with no
                         renderer is six bound rows and a green gate.
      check-ui-labels.js has no tooltip awareness whatsoever. It measures
                         [data-i18n] elements; a tip is neither.
      boot-all-uis.js    counts aria-label and title. It never looks at
                         data-tip.

    So before v1.3.0 this plugin could have shipped six authored bodies, six
    bindings, and NOTHING ON SCREEN — no #tooltip surface, no .tooltip rule, no
    hover handler — past all three of them green. That is what this file is
    against. Its load-bearing assertion is §3: the tip must actually become
    VISIBLE and carry TEXT. Everything else is detail around that.

    It is deliberately NOT a port of the three committed ui_tooltip_clamp_check
    gates (O-Tapestop, O-Bitrot, O-ReverseDelay, ~40 KB each). Those assume the
    OTHER renderer family — measure-then-pin placement with an above/below flip
    and a help-toggle. This page runs the cursor-following O-simpleFM family.

    ── THE NEGATIVE CONTROL IS PART OF THE GATE, NOT A ONE-OFF ─────────────────

    §8 plants a 2400-character body onto a live anchor, hovers it, and requires
    §5's containment logic to REPORT the overflow. A containment check that
    passes both ways is decoration (pattern_probe_must_target_the_branch_the_
    fix_changed), and "no failures" from a sweep that cannot see failures is
    indistinguishable from a clean page. The plant is written to a runtime
    ATTRIBUTE, never to a file: nothing in the repo or the working tree is
    mutated by running this, so it cannot eat an uncommitted edit the way a
    `git checkout --` restore ate O-GrainScatter's.

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const vm   = require('vm');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const PLUGIN     = path.basename(pluginRoot);

const { buildRoot, serve, resolvePlaywright, readEditorSize } =
    require(path.join(repoRoot, 'scripts', 'serve-ui.js'));

let failed = 0;
const check = (cond, desc) => {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
};

// ── i18n.js is an ES module outside any package.json ────────────────────────
// node can neither require() nor import() it. Strip the export keyword and
// evaluate the declarations, exactly as scripts/check-i18n.js does — that gate's
// assertion 7 independently proves the file holds nothing but export
// declarations, so there is nothing else in it to run.
function loadI18n(file) {
    const src = fs.readFileSync(file, 'utf8')
        .replace(/(^|\n)(\s*)export\s+(const|let|function|class)\s/g, '$1$2$3 ');
    const sandbox = { console, out: null };
    vm.createContext(sandbox);
    vm.runInContext(src + '\n;out = { LANGUAGES, I18N, LABELS, TIP_BINDINGS };', sandbox);
    return sandbox.out;
}

// The parameter COUNT comes from the runtime dump, never from a literal here.
// A constant mirroring a plugin constant has drifted silently in this repo
// twice (pattern_test_fixture_mirrors_drift_silently); a count read from
// .planning/params.tsv cannot.
function runtimeParamCount(file) {
    if (!fs.existsSync(file)) return null;
    const m = fs.readFileSync(file, 'utf8').match(/^#\s*params\s+(\d+)\s*$/m);
    return m ? parseInt(m[1], 10) : null;
}

// ── the two chrome anchors every M-stage page carries ───────────────────────
const CHROME = ['#gear-btn', '#lang-select', '#tips-toggle'];

// #lang-select lives inside the settings popover, which ships `hidden`. A hover
// on a display:none element resolves to nothing, so the popover is opened first.
// This is the ONLY anchor on this page behind a disclosure.
const NEEDS_POPOVER = new Set(['#lang-select']);

// Somewhere with no [data-tip] ancestor, used to quiesce between anchors. The
// bottom-left corner is inside .container and clear of .quality-buttons
// (x 50..295, y 395..423); .border above it is pointer-events: none.
const NEUTRAL = { x: 10, y: 440 };

(async () => {
    console.log(`== ${PLUGIN} ui_tip_render_check ==`);

    const pw = resolvePlaywright();
    if (!pw) {
        console.log('  SKIP: playwright is not installed — this gate cannot run.');
        console.log('        It does NOT pass vacuously: exit 1.');
        process.exit(1);
    }

    const i18nFile = path.join(pluginRoot, 'Source', 'ui', 'public', 'js', 'i18n.js');
    const { I18N, TIP_BINDINGS } = loadI18n(i18nFile);

    const size = readEditorSize(PLUGIN);
    if (!size) { console.log('  FAIL: no setSize() in PluginEditor.cpp — the shipping frame is unknown'); process.exit(1); }
    console.log(`  frame: ${size.w} x ${size.h} (read from PluginEditor.cpp, not mirrored here)`);

    const built = buildRoot(PLUGIN);
    const misses = [];
    const server = await serve(built.root, (u) => misses.push(u));
    const consoleErrors = [];

    const browser = await pw.chromium.launch();
    const page = await browser.newPage({ viewport: { width: size.w, height: size.h } });
    page.on('console', (m) => {
        if (m.type() === 'error' || m.type() === 'warning') consoleErrors.push(`${m.type()}: ${m.text()}`);
    });
    page.on('pageerror', (e) => consoleErrors.push(`pageerror: ${e.message}`));

    await page.goto(`http://127.0.0.1:${server.port}/index.html`, { waitUntil: 'load' });
    await page.waitForTimeout(350);

    // ── helpers ─────────────────────────────────────────────────────────────

    const openPopover = async () => {
        const open = await page.evaluate(() => {
            const p = document.getElementById('settings-popover');
            return !!p && !p.hidden;
        });
        if (!open) { await page.click('#gear-btn'); await page.waitForTimeout(150); }
    };

    // Quiesce, THEN apply the stimulus. A read taken while the previous tip is
    // still fading reports the previous anchor's box
    // (pattern_quiesce_before_stimulus_in_async_ui_gates).
    const hoverAnchor = async (sel) => {
        if (NEEDS_POPOVER.has(sel)) await openPopover();
        await page.mouse.move(NEUTRAL.x, NEUTRAL.y);
        await page.waitForFunction(() => {
            const t = document.getElementById('tooltip');
            return !!t && getComputedStyle(t).visibility === 'hidden';
        }, null, { timeout: 3000 }).catch(() => {});
        await page.hover(sel);
        await page.waitForFunction(() => {
            const t = document.getElementById('tooltip');
            return !!t && getComputedStyle(t).visibility === 'visible' && getComputedStyle(t).opacity === '1';
        }, null, { timeout: 3000 }).catch(() => {});
        await page.waitForTimeout(60);
    };

    // Everything the assertions need, read in ONE evaluate so no two numbers can
    // come from two different frames.
    const readTip = () => page.evaluate(() => {
        const t = document.getElementById('tooltip');
        if (!t) return { present: false };
        const cs = getComputedStyle(t);
        const r  = t.getBoundingClientRect();
        const titleEl = t.querySelector('.tip-title');
        let body = '';
        for (const n of t.childNodes) if (n !== titleEl) body += n.textContent;
        return {
            present: true,
            visible: cs.visibility === 'visible' && cs.opacity !== '0' && cs.display !== 'none',
            pointerEvents: cs.pointerEvents,
            position: cs.position,
            title: titleEl ? titleEl.textContent : null,
            body,
            rect: { l: r.left, t: r.top, r: r.right, b: r.bottom, w: r.width, h: r.height },
            vw: window.innerWidth, vh: window.innerHeight,
        };
    });

    const inside = (s) => s.rect.l >= 0 && s.rect.t >= 0 && s.rect.r <= s.vw && s.rect.b <= s.vh;
    const overflowOf = (s) => [
        s.rect.l < 0            ? `left by ${(-s.rect.l).toFixed(1)}px`         : null,
        s.rect.t < 0            ? `top by ${(-s.rect.t).toFixed(1)}px`          : null,
        s.rect.r > s.vw         ? `right by ${(s.rect.r - s.vw).toFixed(1)}px`  : null,
        s.rect.b > s.vh         ? `bottom by ${(s.rect.b - s.vh).toFixed(1)}px` : null,
    ].filter(Boolean).join(', ');

    // ═══════════════════════════════════════════════════════ 1. bindings ══
    console.log('\n-- 1. every TIP_BINDINGS selector resolves');

    const resolved = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
        const el = document.querySelector(sel);
        if (!el) return { sel, key, wrapper: wrapper || null, found: false };
        const target = wrapper ? (el.closest(wrapper) || el) : el;
        const pathOf = (n) => {
            const parts = [];
            for (let e = n; e && e.nodeType === 1 && e !== document.documentElement; e = e.parentElement) {
                let s = e.tagName.toLowerCase();
                if (e.id) { s = '#' + e.id; parts.unshift(s); break; }
                if (e.className && typeof e.className === 'string' && e.className.trim())
                    s += '.' + e.className.trim().split(/\s+/).join('.');
                parts.unshift(s);
            }
            return parts.join('>');
        };
        return {
            sel, key, wrapper: wrapper || null, found: true,
            wrapperFound: wrapper ? !!el.closest(wrapper) : true,
            anchorPath: pathOf(target),
            hasTip: target.hasAttribute('data-tip'),
            hasTipTitle: target.hasAttribute('data-tip-title'),
        };
    }), TIP_BINDINGS.map((b) => [b[0], b[1], b[2] || null]));

    check(TIP_BINDINGS.length > 0,
        `[1] TIP_BINDINGS is non-empty — ${TIP_BINDINGS.length} binding(s). An empty table would `
        + 'make every assertion below pass by having nothing to drive');

    const unresolved = resolved.filter((r) => !r.found);
    check(unresolved.length === 0,
        '[1] every TIP_BINDINGS selector finds an element — a binding that resolves to nothing is '
        + 'an applyI18n "tip target not found" warning and a control with no help'
        + (unresolved.length ? ` — ${unresolved.map((r) => r.sel).join(', ')}` : ''));

    const noWrapper = resolved.filter((r) => r.found && !r.wrapperFound);
    check(noWrapper.length === 0,
        '[1] every wrapper selector resolves from its child — closest() falling back to the child '
        + 'would silently bind the 4 px stroke instead of the cell the user aims at'
        + (noWrapper.length ? ` — ${noWrapper.map((r) => `${r.sel} -> ${r.wrapper}`).join(', ')}` : ''));

    const unwritten = resolved.filter((r) => r.found && !(r.hasTip && r.hasTipTitle));
    check(unwritten.length === 0,
        '[1] applyI18n() wrote data-tip-title AND data-tip onto every resolved anchor'
        + (unwritten.length ? ` — ${unwritten.map((r) => r.sel).join(', ')}` : ''));

    const paths = resolved.filter((r) => r.found).map((r) => r.anchorPath);
    const dupes = paths.filter((p, i) => paths.indexOf(p) !== i);
    check(dupes.length === 0,
        '[1] no two bindings resolve to the SAME element — a copy-pasted wrapper would give one '
        + 'control two tips and leave another with none'
        + (dupes.length ? ` — ${[...new Set(dupes)].join(', ')}` : ''));

    for (const r of resolved)
        console.log(`     ${r.found ? 'ok ' : 'XX '} ${r.sel.padEnd(18)} -> ${r.anchorPath || '(nothing)'}   [${r.key}]`);

    // ═════════════════════════════════════════════════════ 2. coverage ══
    console.log('\n-- 2. coverage against the RUNTIME parameter inventory');

    const nParams = runtimeParamCount(path.join(pluginRoot, '.planning', 'params.tsv'));
    check(nParams !== null,
        '[2] .planning/params.tsv exists and declares a parameter count — without the runtime dump '
        + 'this gate has no non-guessed inventory to check coverage against');
    if (nParams !== null)
        check(TIP_BINDINGS.length === nParams + CHROME.length,
            `[2] one tip per runtime parameter plus the ${CHROME.length} chrome anchors — expected `
            + `${nParams} + ${CHROME.length} = ${nParams + CHROME.length}, got ${TIP_BINDINGS.length}. `
            + 'A parameter added later without a tip fails HERE rather than shipping unhelped');

    for (const c of CHROME)
        check(TIP_BINDINGS.some((b) => b[0] === c),
            `[2] ${c} is bound — the gear tip is what tells a user hover-help exists at all`);

    // ═══════════════════════════════════ 3-5 / 6 / 7. the rendered tip ══
    const anchors = resolved.filter((r) => r.found);

    async function driveLanguage(lang, label) {
        console.log(`\n-- ${label}`);
        await page.evaluate((l) => window.__setLanguage(l), lang);
        await page.waitForTimeout(120);

        let shown = 0, exact = 0, contained = 0;
        const notShown = [], mismatched = [], overflowed = [];

        for (const a of anchors) {
            await hoverAnchor(a.sel);
            const s = await readTip();
            const want = I18N[a.key][lang];

            // §3 — THE VACUITY GUARD. A tip that never showed must fail.
            if (s.present && s.visible && (s.title || '').trim() !== '' && (s.body || '').trim() !== '') ++shown;
            else notShown.push(a.sel);

            // §4 — byte-EQUAL, not "contains". A .tip-title that silently kept a
            // previous anchor's text passes a contains check.
            if (s.title === want.t && s.body === want.b) ++exact;
            else mismatched.push(`${a.sel}: title ${JSON.stringify(s.title)} vs ${JSON.stringify(want.t)}`
                                 + (s.body === want.b ? '' : ` / body differs (${(s.body || '').length} vs ${want.b.length} chars)`));

            // §5 — all four edges.
            if (inside(s)) ++contained;
            else overflowed.push(`${a.sel} overflows ${overflowOf(s)} `
                                 + `(rect ${s.rect.l.toFixed(0)},${s.rect.t.toFixed(0)} `
                                 + `${s.rect.w.toFixed(0)}x${s.rect.h.toFixed(0)} in ${s.vw}x${s.vh})`);
        }

        check(shown === anchors.length,
            `[${lang}] every anchor's hover PAINTS a tip with non-empty title and body — `
            + `${shown}/${anchors.length}. This is the assertion the whole file exists for: authoring `
            + 'copy with no renderer would ship invisible strings past three green gates'
            + (notShown.length ? ` — silent: ${notShown.join(', ')}` : ''));

        check(exact === anchors.length,
            `[${lang}] rendered title and body are BYTE-EQUAL to the ${lang} table entry — `
            + `${exact}/${anchors.length}`
            + (mismatched.length ? `\n         ${mismatched.slice(0, 4).join('\n         ')}` : ''));

        check(contained === anchors.length,
            `[${lang}] every tip rectangle is fully inside the ${size.w}x${size.h} frame, all four `
            + `edges — ${contained}/${anchors.length}`
            + (overflowed.length ? `\n         ${overflowed.slice(0, 4).join('\n         ')}` : ''));

        return { shown, exact, contained };
    }

    const en1 = await driveLanguage('en', '3-5. ENGLISH');
    const fr  = await driveLanguage('fr', '6. FRENCH — 15-20% longer copy, wrapping taller against the 260 px cap');
    const en2 = await driveLanguage('en', '7. BACK TO ENGLISH — the switch is reversible');

    check(en2.exact === en1.exact && en2.shown === en1.shown,
        `[7] English comes back identical after the round trip — shown ${en1.shown}->${en2.shown}, `
        + `exact ${en1.exact}->${en2.exact}. A one-way switch would leave a French plugin on an `
        + 'English preference (pattern_webview_one_shot_state_push_stale_on_preset_load)');

    // ═════════════════════════════════════════ 8. THE NEGATIVE CONTROL ══
    console.log('\n-- 8. negative control — is §5 able to SEE an overflow at all?');

    // Planted onto a live ATTRIBUTE, not into a file: running this gate mutates
    // nothing on disk, so it can never eat an uncommitted edit. __setLanguage()
    // rewrites every anchor's attributes from the table, which is the restore.
    const victim = anchors[0];
    await page.evaluate(([sel, wrapper]) => {
        const el = document.querySelector(sel);
        const target = wrapper ? (el.closest(wrapper) || el) : el;
        target.setAttribute('data-tip', 'OVERFLOW CONTROL. '.repeat(150));
    }, [victim.sel, victim.wrapper]);

    await hoverAnchor(victim.sel);
    const planted = await readTip();
    const plantedDetected = !inside(planted);

    check(plantedDetected,
        `[8] a 2400-character body on ${victim.sel} IS reported as overflowing — `
        + (plantedDetected
            ? `${overflowOf(planted)} (rect ${planted.rect.w.toFixed(0)}x${planted.rect.h.toFixed(0)} `
              + `in ${planted.vw}x${planted.vh})`
            : `it measured ${planted.rect.w.toFixed(0)}x${planted.rect.h.toFixed(0)} and FIT, which means `
              + '§5 above cannot distinguish a contained tip from an overflowing one and its three '
              + 'passes are decoration'));

    // Restore, and prove the restore took — a negative control that leaves the
    // page planted would poison anything that ran after it.
    await page.evaluate(() => window.__setLanguage('en'));
    await page.waitForTimeout(120);
    await hoverAnchor(victim.sel);
    const restored = await readTip();
    check(restored.body === I18N[victim.key].en.b && inside(restored),
        '[8] the plant is fully restored from the table — body byte-equal to the en entry again '
        + 'and the rect back inside the frame');

    // ════════════════════════════════════════════ 9. renderer contract ══
    console.log('\n-- 9. the surface\'s load-bearing CSS');

    const surface = await page.evaluate(() => {
        const t = document.getElementById('tooltip');
        if (!t) return null;
        const cs = getComputedStyle(t);
        return { position: cs.position, pointerEvents: cs.pointerEvents, zIndex: cs.zIndex,
                 maxWidth: cs.maxWidth, hiddenWhenIdle: null };
    });
    check(!!surface, '[9] the #tooltip surface exists in the DOM');
    if (surface) {
        check(surface.position === 'fixed',
            `[9] position: fixed — the renderer positions in viewport coordinates (got ${surface.position})`);
        check(surface.pointerEvents === 'none',
            `[9] pointer-events: none — without it the surface steals the hover keeping it open `
            + `(got ${surface.pointerEvents})`);
    }

    await page.mouse.move(NEUTRAL.x, NEUTRAL.y);
    await page.waitForTimeout(250);
    const idle = await page.evaluate(() => {
        const t = document.getElementById('tooltip');
        const cs = getComputedStyle(t);
        return { visibility: cs.visibility, opacity: cs.opacity };
    });
    check(idle.visibility === 'hidden' || idle.opacity === '0',
        `[9] an idle tip is visibility:hidden / opacity:0, so it enters neither check-ui-labels' `
        + `label sweep nor its English-vs-French geometry diff (got visibility=${idle.visibility} `
        + `opacity=${idle.opacity})`);

    // Escape and pointerdown both hide it — #intensityKnob starts a drag on
    // mousedown and the tip must be out of the way first.
    await hoverAnchor(anchors[0].sel);
    await page.keyboard.press('Escape');
    await page.waitForTimeout(200);
    const afterEsc = await page.evaluate(() => getComputedStyle(document.getElementById('tooltip')).visibility);
    check(afterEsc === 'hidden', `[9] Escape hides the tip (got visibility=${afterEsc})`);

    // ══════════════════════════════ 9b. THE FOCUS LATCH, BOTH HALVES ══
    //
    // A mouse click on a <button> focuses it. An unconditional focusin rule
    // therefore re-opens the tip that pointerdown just hid, with the pointer
    // still on the anchor and no further pointerover coming — and the tip sits
    // on top of whatever the click opened. Measured on this page before the
    // fix: the gear's tip covered the settings popover by 161 x 29 px.
    //
    // BOTH halves are asserted, separately and on purpose. Asserting only that
    // a click leaves no tip lets the feature decay into "focus never shows a
    // tip", which passes that assertion perfectly and silently removes the
    // keyboard half of hover-help.
    console.log('\n-- 9b. the focus latch');

    await page.mouse.move(1, 1);
    await page.waitForTimeout(150);
    // BLUR FIRST, and this line is the whole reason the assertion below can
    // fail at all. An earlier section of this gate leaves focus ON #gear-btn,
    // and clicking an ALREADY-FOCUSED element fires no focusin — so without
    // this the check reported "no tip after a click" for a page with no latch
    // whatsoever. Verified: with the latch removed the control now fails, and
    // with the blur removed it passes either way.
    await page.evaluate(() => document.activeElement && document.activeElement.blur());
    await page.waitForTimeout(100);
    await page.click('#gear-btn');
    await page.waitForTimeout(300);
    const afterClick = await page.evaluate(() => {
        const t = document.getElementById('tooltip');
        const cs = getComputedStyle(t);
        const r = t.getBoundingClientRect();
        const ls = document.getElementById('lang-select');
        const panel = ls ? ls.closest('div') : null;
        const pr = panel ? panel.getBoundingClientRect() : null;
        const shown = cs.visibility !== 'hidden' && cs.opacity !== '0';
        let overlap = 0;
        if (shown && pr) {
            const ox = Math.max(0, Math.min(r.right, pr.right) - Math.max(r.left, pr.left));
            const oy = Math.max(0, Math.min(r.bottom, pr.bottom) - Math.max(r.top, pr.top));
            overlap = Math.round(ox * oy);
        }
        return { shown, overlap };
    });
    check(!afterClick.shown,
        '[9b] a POINTER click opens no tip — the latch suppresses the focusin arm'
        + (afterClick.overlap ? ` (it covered the popover by ${afterClick.overlap} px2)` : ''));

    // The keyboard half. A real tab-ring walk, not a programmatic .focus():
    // Chromium reports :focus-visible false for a .focus() that follows a click,
    // and .focus() on an already-focused element fires no event at all — either
    // one would report "no tip" and record that as correct.
    await page.keyboard.press('Escape');
    await page.waitForTimeout(150);
    let kbHit = null;
    for (let i = 1; i <= 20; i++) {
        await page.keyboard.press('Tab');
        await page.waitForTimeout(60);
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
        '[9b] a KEYBOARD tab still opens a tip — the accessibility half survives the latch'
        + (kbHit ? ` (tab #${kbHit.press} on ${kbHit.on})` : ' — none in 20 tabs'));
    await page.keyboard.press('Escape');
    await page.waitForTimeout(150);

    // ═══════════════════════════════════════════════════ 10. hygiene ══
    console.log('\n-- 10. page hygiene');
    const tipWarnings = consoleErrors.filter((e) => /tip target not found|missing key/.test(e));
    check(tipWarnings.length === 0,
        '[10] no "tip target not found" / "missing key" warning from applyI18n'
        + (tipWarnings.length ? ` — ${tipWarnings.slice(0, 3).join(' | ')}` : ''));
    check(consoleErrors.length === 0,
        '[10] no console error or warning during the whole run'
        + (consoleErrors.length ? ` — ${consoleErrors.length}: ${consoleErrors.slice(0, 3).join(' | ')}` : ''));
    check(misses.length === 0,
        '[10] every requested resource was served — an unserved js/i18n.js is a 404 that presents '
        + 'as a page stuck in English and nothing else'
        + (misses.length ? ` — ${misses.slice(0, 5).join(', ')}` : ''));

    await browser.close();
    await server.close();
    fs.rmSync(built.root, { recursive: true, force: true });

    console.log(`\n== ${failed === 0 ? 'ALL CHECKS PASSED' : failed + ' CHECK(S) FAILED'} ==`);
    process.exit(failed);
})().catch((e) => {
    console.error('ui_tip_render_check: ' + (e && e.stack ? e.stack : e));
    process.exit(1);
});
