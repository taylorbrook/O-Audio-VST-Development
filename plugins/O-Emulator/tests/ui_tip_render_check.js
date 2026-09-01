/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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
    O-Emulator — hover-help RENDER verification at the shipping viewport (v1.2.1).

    WHY THIS FILE EXISTS. No gate in this repo can see a rendered tooltip.
    check-i18n reads the table statically and is satisfied by TIP_BINDINGS
    being non-empty and its keys resolving. check-ui-labels has no tooltip
    awareness whatsoever. boot-all-uis counts aria-label and title and never
    data-tip. So a plugin can author copy, bind it, and pass all three green
    while shipping nothing on screen — which is precisely the state O-Emulator
    was in before v1.2.0: canon v2's applyI18n() writes data-tip-title and
    data-tip onto the anchors and stops there, and this page had no #tooltip
    element, no .tooltip rule and no hover handler to read them.

    This file is the assertion those three cannot make. It drives the REAL
    page — same index.html, same inline <style>, same inline module, only
    js/juce/index.js swapped for the shared stub — in a browser pinned to the
    exact shipping frame, and measures the rectangle that actually paints.

    NOT a port of the three committed ui_tooltip_clamp_check.js gates
    (O-Tapestop, O-Bitrot, O-ReverseDelay). Those are ~40 KB each and are built
    around the OTHER renderer family: measure-then-pin placement with an
    above/below flip and an arrow to keep inside the box. O-Emulator ports
    O-simpleFM's delegated cursor-following renderer, which has no arrow and no
    pinned width, so those assertions would not describe it.

    WHAT IS ASSERTED, per anchor, in English and again in French:

      1. every TIP_BINDINGS selector resolves, and its wrapper walk lands on a
         real element — a binding that finds nothing is a FAIL, not a warning,
         because applyI18n() only console.warns about it;
      2. hovering the anchor makes the surface VISIBLE with non-empty text —
         the vacuity guard, and the assertion this whole file exists for;
      3. the rendered title and body are BYTE-EQUAL to the table entry. Not
         "contains": a .tip-title that silently kept the previous anchor's text
         passes a contains check and fails this one;
      4. the tip rectangle is fully inside the 620 x 430 frame on all four
         edges. This is the assertion the small frame exists to break — the
         gear sits 16 px from the bottom-right corner, so its tip is on the
         clamp's flip-and-clamp path, not on the easy path.

    Then French (assertions 2-4 again against the fr entries), then back to
    English to prove the switch is reversible rather than one-way.

    TWO NEGATIVE CONTROLS, because "all pass" is worthless without them:

      NC-1  an over-long body is planted in the SERVED copy of js/i18n.js and
            the page reloaded; assertion 4 must report the overflow. Restored
            from a namespaced per-run copy, never with `git checkout --`, which
            would take any uncommitted work in the same file with it.
      NC-2  a plain mouse click must NOT leave a tip parked on screen, and a
            keyboard focus MUST open one. Those are the two halves of the
            lastInputWasPointer latch this plugin added over the reference
            renderer, and without NC-2 the latch could silently become "focus
            never shows a tip at all", which is a dead accessibility path that
            every other assertion here would pass straight through.

    Usage:  node plugins/O-Emulator/tests/ui_tip_render_check.js
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

const PLUGIN    = 'O-Emulator';
const publicDir = path.join(pluginRoot, 'Source', 'ui', 'public');
const htmlPath  = path.join(publicDir, 'index.html');
const i18nPath  = path.join(publicDir, 'js', 'i18n.js');

// Mirrored from PluginEditor.cpp and the inline <style>, and CROSS-CHECKED
// against both below. A fixture that mirrors a constant without checking it
// starts describing the release before it and keeps passing
// (pattern_test_fixture_mirrors_drift_silently).
const SHIP_W = 620;
const SHIP_H = 430;
const MARGIN = 8;          // setupTooltips()'s clamp margin
const DOCUMENTED_MAX_W = 250;   // .tooltip max-width

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
    };
}`;

(async () => {
    console.log(`== ${PLUGIN} ui_tip_render_check ==`);
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}\n`);

    const html      = fs.readFileSync(htmlPath, 'utf8');
    const i18nSrc   = fs.readFileSync(i18nPath, 'utf8');
    const editorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginEditor.cpp'), 'utf8');

    // ── 0. the mirrored constants, guarded ──────────────────────────────────
    const size = S.readEditorSize(PLUGIN);
    check(size && size.w === SHIP_W && size.h === SHIP_H,
        `editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below `
        + `(PluginEditor.cpp says ${size ? `${size.w} x ${size.h}` : 'NOTHING'})`);

    // Parsed from the .tooltip RULE specifically. This page also carries a
    // width on .settings-popover and .preset-name, so a loose scan for the
    // first max-width would silently measure against one of those.
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
        '.tooltip is position:fixed + visibility:hidden + pointer-events:none — '
        + 'the three properties that keep an unshown surface out of check-ui-labels\' '
        + 'sweep and stop a shown one stealing its own hover');
    check(/id="tooltip"/.test(html), 'index.html carries the #tooltip surface');
    check(!/\btitle\s*=\s*["']/.test(html.replace(/<title>[\s\S]*?<\/title>/, '')),
        'no native title= attribute reintroduced by the renderer (contract §4)');

    // ── the table ───────────────────────────────────────────────────────────
    const { I18N, TIP_BINDINGS, LANGUAGES } = loadTable(i18nSrc);
    check(Array.isArray(TIP_BINDINGS) && TIP_BINDINGS.length > 0,
        `TIP_BINDINGS parsed from js/i18n.js — ${TIP_BINDINGS.length} anchor(s) expected`);
    check(LANGUAGES.join(',') === 'en,fr', `LANGUAGES is en,fr — got ${LANGUAGES.join(',')}`);

    const pw = S.resolvePlaywright();
    if (pw == null) {
        console.log('\n  SKIP: playwright not resolvable. Install with');
        console.log('        npx playwright install chromium');
        console.log('  The hover-help render and its edge clamp are NOT verified without it.');
        process.exit(77);
    }

    const built = S.buildRoot(PLUGIN);
    const misses = [];
    const srv = await S.serve(built.root, (m) => misses.push(m));
    const browser = await pw.chromium.launch();
    // `viewport`, NOT `viewportSize` — the latter is the getter's name and is
    // silently IGNORED as a launch option, leaving Chromium's 1280x720 default.
    // At 1280 every anchor on this page has room to its right, the clamp never
    // engages, and every assertion below would pass while the real 620 px frame
    // overflowed (pattern_tooltip_clamp_gate_viewport_sensitive).
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
    // (pattern_module_toplevel_init_tdz), and on this plugin the entire UI is
    // one inline module. The console spec readout is born as &nbsp; and is
    // written only by the console relay listener, so populated text proves it.
    const alive = await page.evaluate(() => (document.getElementById('consoleInfo') || {}).textContent || '');
    check(/SNES/.test(alive),
        `the inline module ran — #consoleInfo reads "${alive.trim().slice(0, 40)}"`);

    // ── 1. every binding resolves, selector AND wrapper walk ────────────────
    const resolution = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
        const el = document.querySelector(sel);
        if (!el) return { key, sel, ok: false, why: 'selector matched nothing' };
        const target = wrapper ? (el.closest(wrapper) || null) : el;
        if (!target) return { key, sel, ok: false, why: `closest(${wrapper}) matched nothing` };
        return {
            key, sel, ok: true,
            tag: target.tagName.toLowerCase() + (target.className ? '.' + String(target.className).split(' ')[0] : ''),
            hasTip: target.hasAttribute('data-tip') && target.hasAttribute('data-tip-title'),
        };
    }), TIP_BINDINGS);

    for (const r of resolution) {
        check(r.ok, `[1] binding ${r.key} resolves — ${r.sel}${r.ok ? ` -> ${r.tag}` : ` (${r.why})`}`);
        if (r.ok) check(r.hasTip,
            `[1] applyI18n wrote data-tip-title + data-tip onto ${r.key}'s anchor (${r.tag})`);
    }
    check(tipWarns.length === 0,
        `[1] applyI18n logged no "tip target not found" warning`
        + (tipWarns.length ? ` — ${tipWarns.length}: ${tipWarns[0]}` : ''));

    // ── the hover driver ────────────────────────────────────────────────────
    // Hovers the SELECTOR (the addressable child), not the wrapper: that is
    // what a user's pointer actually lands on, and closest('[data-tip]') is
    // what has to walk up from it. Hovering the wrapper directly would prove
    // the wrapper carries a tip and nothing about whether the child reaches it.
    //
    // The pointer is parked at the frame's top-left corner between anchors so
    // pointerout really fires; without that, moving straight from one anchor to
    // the next inside the same cell would leave `active` unchanged and the
    // previous anchor's text on screen — the exact stale-title case assertion 3
    // is written to catch.
    // page.evaluate() given a STRING evaluates it as an EXPRESSION, so passing
    // READ_TIP directly returns the function object rather than calling it —
    // and an unserializable return arrives as undefined, which reads exactly
    // like "the tip was never there". Invoked explicitly instead.
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
        // mid-transition returns opacity 0.4 and a rect that is already final,
        // so a fixed sleep either flakes or hides a tip that never showed.
        await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 2000 })
                  .catch(() => {});
        return readTip();
    };

    const inFrame = (r) =>
        r.x >= 0 && r.y >= 0 && r.x + r.w <= SHIP_W && r.y + r.h <= SHIP_H;
    const edges = (r) =>
        `L${r.x.toFixed(1)} T${r.y.toFixed(1)} R${(SHIP_W - r.x - r.w).toFixed(1)} B${(SHIP_H - r.y - r.h).toFixed(1)}`;

    // #lang-select lives inside a popover that ships hidden, so it is not
    // hoverable until the gear is clicked. Everything else is reachable at rest.
    const NEEDS_POPOVER = new Set(['#lang-select']);

    const sweep = async (lang) => {
        console.log(`\n-- ${lang.toUpperCase()}: hover every anchor, byte-compare, measure the rect`);
        let popoverOpen = false;
        const seen = [];
        for (const [sel, key, wrapper] of TIP_BINDINGS) {
            const entry = (I18N[key] || {})[lang];
            if (!entry) { check(false, `[2] ${key} has an ${lang} entry`); continue; }

            if (NEEDS_POPOVER.has(sel) && !popoverOpen) {
                await page.click('#gear-btn', { force: true });
                await page.waitForSelector('#settings-popover:not([hidden])', { timeout: 2000 });
                popoverOpen = true;
            }

            const st = await hoverAndRead(sel);
            seen.push(key);

            check(st !== null && st.visible,
                `[2][${lang}] ${key}: hovering ${sel} SHOWS the tip (opacity ${st ? st.opacity : 'n/a'})`);
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
                `[4][${lang}] ${key}: rect ${st.rect.w.toFixed(1)} x ${st.rect.h.toFixed(1)} is fully `
                + `inside ${SHIP_W} x ${SHIP_H} — edge clearances ${edges(st.rect)}`);
        }
        check(seen.length === TIP_BINDINGS.length,
            `[2][${lang}] every one of the ${TIP_BINDINGS.length} bound anchors was driven — got ${seen.length}`);
        if (popoverOpen) {
            await page.keyboard.press('Escape');       // also clears the focus latch
            await page.waitForTimeout(120);
        }
        return seen;
    };

    await sweep('en');

    // ── 5. French, then back ────────────────────────────────────────────────
    // French runs 15-20% longer, wraps to more lines against the max-width cap
    // and grows the tip's HEIGHT, so a tip that fits in English can overflow the
    // bottom of a 430 px frame in French. That is why the whole sweep repeats
    // rather than spot-checking one anchor.
    await page.evaluate((l) => window.__setLanguage(l), 'fr');
    await page.waitForTimeout(150);
    const frLang = await page.evaluate(() => document.getElementById('lang-select').value);
    check(frLang === 'fr', `[5] window.__setLanguage('fr') took — selector reads "${frLang}"`);
    await sweep('fr');

    await page.evaluate((l) => window.__setLanguage(l), 'en');
    await page.waitForTimeout(150);
    const backSt = await hoverAndRead('.knob[data-param="crush"]');
    check(backSt.visible && backSt.title === I18N['tip.crush'].en.t
          && backSt.body === I18N['tip.crush'].en.b,
        `[5] switching back to English restores the English tip byte-for-byte `
        + `— "${backSt.title}"`);
    await park();

    // ── NC-2. the focus latch, both halves ──────────────────────────────────
    // This plugin diverges from the O-simpleFM reference here on purpose, and a
    // divergence with no control is a regression waiting to be inherited.
    console.log('\n-- NC-2: the focus latch (a click must not pin a tip; a key must open one)');
    await page.click('#gear-btn', { force: true });
    await page.waitForTimeout(250);
    const afterClick = await readTip();
    check(!afterClick.visible,
        `[NC-2a] a mouse click leaves NO tip parked on screen — without the latch the `
        + `gear's own tip stays pinned across the popover it just opened `
        + `(measured before the fix: 250 x 115 at 320,284 over a popover at 434,330)`);

    // Any real keydown releases the latch; then a focus must open the tip. If
    // this half failed, the latch would have silently killed the keyboard path
    // and NC-2a alone would still be green.
    //
    // Driven with REAL Tab presses, not element.focus(). A programmatic focus()
    // on the element the click already focused fires NO focusin at all, so the
    // first version of this control read "no tip" and blamed the latch for a
    // defect in the test — a probe that reports the right verdict for the wrong
    // reason. Walking the tab ring also proves the keyboard actually REACHES
    // the anchors: it crosses the five preset buttons and the five console
    // segments, each of which paints its own tip on the way past.
    await page.keyboard.press('Escape');
    await page.waitForTimeout(120);
    await page.evaluate(() => { if (document.activeElement) document.activeElement.blur(); });
    await park();
    let tabs = 0;
    let focusedId = null;
    while (tabs < 30) {
        await page.keyboard.press('Tab');
        ++tabs;
        focusedId = await page.evaluate(() => (document.activeElement || {}).id || null);
        if (focusedId === 'gear-btn') break;
    }
    check(focusedId === 'gear-btn',
        `[NC-2b] Tab reaches #gear-btn from the top of the ring — ${tabs} press(es), `
        + `landed on "${focusedId}"`);
    await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 2000 }).catch(() => {});
    const afterKey = await readTip();
    check(afterKey.visible && afterKey.title === I18N['tip.gearBtn'].en.t,
        `[NC-2b] keyboard focus DOES open the tip — "${afterKey.title}" `
        + `(the accessibility half the latch must not kill)`);
    check(afterKey.visible && inFrame(afterKey.rect),
        `[NC-2b] the focus-placed tip is inside the frame — ${edges(afterKey.rect)}`);
    await page.keyboard.press('Escape');
    await park();

    // ── NC-1. plant an over-long body; assertion 4 must report it ───────────
    // A namespaced per-run directory, never a bare filename at a shared temp
    // root: several executors run in this scratchpad at once and a bare
    // i18n.orig.js is not yours. Restored by COPY, never with
    // `git checkout -- <file>`, which takes any uncommitted work in the same
    // file with it (O-GrainScatter lost a whole edit that way).
    console.log('\n-- NC-1: plant an over-long body and confirm assertion 4 reports the overflow');
    const nc = fs.mkdtempSync(path.join(os.tmpdir(), 'oemu-tip-nc-'));
    const servedI18n = path.join(built.root, 'js', 'i18n.js');
    const backup = path.join(nc, 'i18n.served.orig.js');
    fs.copyFileSync(servedI18n, backup);
    try {
        const LONG = ('Une phrase de contrôle négatif, délibérément beaucoup trop longue pour '
                    + 'tenir dans le cadre, répétée afin de faire déborder la surface par le bas. ')
                    .repeat(9);
        const planted = fs.readFileSync(backup, 'utf8')
            .replace(/(\'tip\.crush\'[\s\S]*?en:\s*\{\s*t:\s*"Crush",\s*\n\s*b:\s*)"[^"]*"/,
                     `$1"${LONG}"`);
        check(planted !== fs.readFileSync(backup, 'utf8'),
            `[NC-1] the plant actually edited tip.crush's en body — a no-op replace would `
            + `make this control vacuous`);
        fs.writeFileSync(servedI18n, planted);

        await page.goto(url, { waitUntil: 'networkidle' });
        const bad = await hoverAndRead('.knob[data-param="crush"]');
        check(bad.visible, `[NC-1] the planted tip still renders (${bad.rect.h.toFixed(1)}px tall)`);
        check(!inFrame(bad.rect),
            `[NC-1] assertion 4 REPORTS the overflow — rect ${bad.rect.w.toFixed(1)} x `
            + `${bad.rect.h.toFixed(1)} at ${bad.rect.x.toFixed(1)},${bad.rect.y.toFixed(1)} `
            + `leaves the ${SHIP_W} x ${SHIP_H} frame (${edges(bad.rect)}). If this PASSES as `
            + `in-frame, assertion 4 is decoration and every green above means nothing`);
    } finally {
        fs.copyFileSync(backup, servedI18n);
    }

    await page.goto(url, { waitUntil: 'networkidle' });
    const restored = await hoverAndRead('.knob[data-param="crush"]');
    check(restored.visible && restored.body === I18N['tip.crush'].en.b && inFrame(restored.rect),
        `[NC-1] restored from the namespaced copy — tip.crush is back inside the frame `
        + `(${edges(restored.rect)})`);
    fs.rmSync(nc, { recursive: true, force: true });

    // ── housekeeping ────────────────────────────────────────────────────────
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
