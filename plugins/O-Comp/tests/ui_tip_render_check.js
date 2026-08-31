/*
   This file is part of O-Comp, an Ouaricon Audio plugin.
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
    O-Comp — hover-help RENDER verification at the shipping viewport (v1.7.0).

    WHY THIS FILE EXISTS. No gate in this repo can see a rendered tooltip.
    check-i18n reads the table statically and is satisfied by TIP_BINDINGS
    being non-empty and its keys resolving. check-ui-labels has no tooltip
    awareness whatsoever — it classes the surface as pointer-events: none
    decoration and never as a label. boot-all-uis counts aria-label and title
    and never data-tip. So a plugin can author copy, bind it, and pass all three
    green while showing nothing on screen — which is exactly the state O-Comp
    was in before v1.7.0: canon v2's applyI18n() writes data-tip-title and
    data-tip onto the anchors and stops there, and this page had no #tooltip
    element, no `.tooltip` rule and no hover handler to read them.

    This file is the assertion those three cannot make. It drives the REAL
    page — same index.html, same inline <style>, same inline module, only
    js/juce/index.js swapped for the shared stub — in a browser pinned to the
    exact shipping frame, and measures the rectangle that actually paints.

    NOT a port of the three committed ui_tooltip_clamp_check.js gates
    (O-Tapestop, O-Bitrot, O-ReverseDelay). Those are ~40 KB each and are built
    around the OTHER renderer family: measure-then-pin placement with an
    above/below flip and an arrow. O-Comp ports O-simpleFM's delegated
    cursor-following renderer, which has no arrow and no pinned width, so those
    assertions would not describe it.

    WHAT IS ASSERTED, per anchor, in English and again in French:

      1. every TIP_BINDINGS selector resolves AND its closest(wrapper) walk
         lands on a real element carrying both tip attributes — a binding that
         finds nothing is a FAIL, not a warning, because applyI18n() only
         console.warns about it;
      2. hovering the anchor makes the surface VISIBLE with non-empty text —
         the vacuity guard, and the assertion this whole file exists for;
      3. the rendered title and body are BYTE-EQUAL to the table entry. Not
         "contains": a .tip-title that silently kept the previous anchor's text
         passes a contains check and fails this one;
      4. the tip rectangle is fully inside the 620 x 360 frame on all four
         edges. 360px is the shortest dimension in batch M1 after O-Chorus and
         O-DigiDelay, so the vertical clamp is the normal path here.

    Then French (2-4 again against the fr entries), then back to English to
    prove the switch is reversible rather than one-way.

    TWO NEGATIVE CONTROLS, because "all pass" is worthless without them:

      NC-1  an over-long body is planted in the SERVED copy of js/i18n.js and
            the page reloaded; assertion 4 must REPORT the overflow. Restored
            from a namespaced per-run copy, never with `git checkout --`, which
            would take any uncommitted work in the same file with it
            (O-GrainScatter lost a whole edit that way).
      NC-2  a plain mouse click must NOT leave a tip parked on screen, and a
            keyboard tab MUST still open one. Those are the two halves of the
            lastInputWasPointer latch this plugin adds over the O-simpleFM
            reference, asserted SEPARATELY: checking only the first lets the
            feature decay into "focus never shows a tip", which passes it
            perfectly while silently removing the keyboard half of hover-help.

            NC-2a BLURS BEFORE THE CLICK, and that line is the whole reason it
            can fail at all. The section above it leaves focus on #gear-btn (the
            popover's own Escape handler puts it there), and clicking an
            ALREADY-FOCUSED element fires no focusin — so without the blur the
            assertion reports "no tip after a click" for a page with no latch
            whatsoever. The pilots' first version passed 125/125 with the latch
            deleted.

    Usage:  node plugins/O-Comp/tests/ui_tip_render_check.js
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

const PLUGIN    = 'O-Comp';
const publicDir = path.join(pluginRoot, 'Source', 'ui', 'public');
const htmlPath  = path.join(publicDir, 'index.html');
const i18nPath  = path.join(publicDir, 'js', 'i18n.js');

// Mirrored from PluginEditor.cpp and the inline <style>, and CROSS-CHECKED
// against both below rather than trusted. A fixture that mirrors a constant
// without checking it starts describing the release before it and keeps
// passing (pattern_test_fixture_mirrors_drift_silently).
const SHIP_W = 620;
const SHIP_H = 360;
const MARGIN = 8;               // setupTooltips()'s clamp margin
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

    const html    = fs.readFileSync(htmlPath, 'utf8');
    const i18nSrc = fs.readFileSync(i18nPath, 'utf8');

    // ── 0. the mirrored constants, guarded ──────────────────────────────────
    const size = S.readEditorSize(PLUGIN);
    check(size && size.w === SHIP_W && size.h === SHIP_H,
        `editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below `
        + `(PluginEditor.cpp says ${size ? `${size.w} x ${size.h}` : 'NOTHING'})`);

    // Parsed from the .tooltip RULE specifically. This page also carries a
    // max-width on .preset-name and widths on .settings-popover, .toggle-button
    // and .preset-action-btn, so a loose scan for the first max-width would
    // silently measure against one of those.
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
        + 'properties that keep an unshown surface out of check-ui-labels\' sweep and stop a '
        + 'shown one stealing its own hover');
    check(/id="tooltip"/.test(html), 'index.html carries the #tooltip surface');
    check(/lastInputWasPointer/.test(html),
        'the focus arm is latched to the last input device — an unconditional focusin rule '
        + 'parks a tip over the popover a click just opened (NC-2 below measures it)');
    check(!/\btitle\s*=\s*["']/.test(html.replace(/<title>[\s\S]*?<\/title>/, '')),
        'no native title= attribute reintroduced by the renderer (contract §4)');

    // ── the table ───────────────────────────────────────────────────────────
    const { I18N, TIP_BINDINGS, LANGUAGES } = loadTable(i18nSrc);
    check(Array.isArray(TIP_BINDINGS) && TIP_BINDINGS.length > 0,
        `TIP_BINDINGS parsed from js/i18n.js — ${TIP_BINDINGS.length} anchor(s)`);
    check(LANGUAGES.join(',') === 'en,fr', `LANGUAGES is en,fr — got ${LANGUAGES.join(',')}`);

    // The three canvas entries (empty body) must stay OUT of the bindings. A
    // binding on one of them would paint an empty surface on hover and would
    // still satisfy check-i18n assertion 2.
    const bound = new Set(TIP_BINDINGS.map(b => b[1]));
    const boundEmpty = [...bound].filter(k => !(I18N[k] && I18N[k].en && I18N[k].en.b.trim()));
    check(boundEmpty.length === 0,
        `no EMPTY-body I18N entry is bound — the three canvas.* captions are fillText strings, `
        + `not tips` + (boundEmpty.length ? ` — bound anyway: ${boundEmpty.join(', ')}` : ''));

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
    // At 1280x720 every anchor on this page has room below and to the right, the
    // clamp never engages, and every assertion below would pass while the real
    // 620 x 360 frame overflowed
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

    // Non-vacuity: the inline module must have RUN. A TDZ throw out of module
    // evaluation leaves the HTML looking correct and every control dead
    // (pattern_module_toplevel_init_tdz), and on this plugin the ENTIRE UI is
    // one inline module. #ratio-value is authored "4.0:1" in the markup and is
    // rewritten by updateKnob() from the stub's seeded default (params.tsv
    // defaultNorm 0.052632 -> 2.0), so a changed reading proves setupKnob ran.
    const ratioText = await page.evaluate(() => (document.getElementById('ratio-value') || {}).textContent || '');
    check(ratioText.trim() === '2.0:1',
        `the inline module ran — #ratio-value was rewritten from the markup's "4.0:1" to the `
        + `seeded default, and reads "${ratioText.trim()}"`);
    const hasHooks = await page.evaluate(() => typeof window.__setLanguage === 'function');
    check(hasHooks, 'window.__setLanguage exists — the canon block evaluated');

    // The surface exists and is INVISIBLE at rest. This is the geometry claim
    // that keeps check-ui-labels byte-identical before and after this feature.
    const idle = await page.evaluate(`(${READ_TIP})()`);
    check(idle !== null && typeof idle === 'object',
        'the #tooltip surface is present in the DOM at rest');
    check(idle && !idle.visible && idle.body === '' && idle.title === null,
        `at rest the surface is hidden and EMPTY (opacity ${idle ? idle.opacity : 'n/a'}) — an `
        + `authored literal in it would ship English in both languages and enter check-i18n's `
        + `text-node sweep`);

    // ── 1. every binding resolves, selector AND wrapper walk ────────────────
    const resolution = await page.evaluate((bindings) => bindings.map(([sel, key, wrapper]) => {
        const el = document.querySelector(sel);
        if (!el) return { key, sel, ok: false, why: 'selector matched nothing' };
        const target = wrapper ? (el.closest(wrapper) || null) : el;
        if (!target) return { key, sel, ok: false, why: `closest(${wrapper}) matched nothing` };
        const r = target.getBoundingClientRect();
        return {
            key, sel, ok: true,
            tag: target.tagName.toLowerCase()
                 + (target.id ? '#' + target.id
                    : target.className ? '.' + String(target.className).split(' ')[0] : ''),
            area: `${Math.round(r.width)}x${Math.round(r.height)}`,
            hasTip: target.hasAttribute('data-tip') && target.hasAttribute('data-tip-title'),
        };
    }), TIP_BINDINGS);

    for (const r of resolution) {
        check(r.ok, `[1] binding ${r.key} resolves — ${r.sel}`
            + (r.ok ? ` -> ${r.tag} (${r.area})` : ` (${r.why})`));
        if (r.ok) check(r.hasTip,
            `[1] applyI18n wrote data-tip-title + data-tip onto ${r.key}'s anchor (${r.tag})`);
    }
    check(tipWarns.length === 0,
        `[1] applyI18n logged no "tip target not found" warning`
        + (tipWarns.length ? ` — ${tipWarns.length}: ${tipWarns[0]}` : ''));

    // ── the hover driver ────────────────────────────────────────────────────
    // Hovers the SELECTOR (the addressable child), never the wrapper: that is
    // what a user's pointer actually lands on, and closest('[data-tip]') is what
    // has to walk up from it. Hovering .control-group directly would prove the
    // wrapper carries a tip and nothing about whether the knob reaches it.
    //
    // The pointer is parked at the frame's top-left corner between anchors so
    // pointerout really fires; without that, moving straight from one anchor to
    // the next would leave `active` unchanged on some paths and the previous
    // anchor's text on screen — the stale-title case assertion 3 exists for.
    //
    // page.evaluate() given a STRING evaluates it as an EXPRESSION, so passing
    // READ_TIP directly returns the FUNCTION OBJECT, which is unserialisable and
    // arrives as undefined — and `undefined !== null` is a PASS over a surface
    // nobody read. Invoked explicitly instead.
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
        // mid-transition returns opacity 0.4 with a rect that is already final,
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
        for (const [sel, key] of TIP_BINDINGS) {
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
                `[3][${lang}] ${key}: rendered title is BYTE-EQUAL to the table — `
                + `"${st.title}" vs "${entry.t}"`);
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
            await page.keyboard.press('Escape');   // closes the popover; also clears the latch
            await page.waitForTimeout(150);
        }
        return seen;
    };

    await sweep('en');

    // ── 5. French, then back ────────────────────────────────────────────────
    // French runs 15-20% longer, wraps to more lines against the max-width cap
    // and grows the tip's HEIGHT, so a tip that fits in English can overflow the
    // bottom of a 360px frame in French. That is why the whole sweep repeats
    // rather than spot-checking one anchor.
    await page.evaluate((l) => window.__setLanguage(l), 'fr');
    await page.waitForTimeout(150);
    const frLang = await page.evaluate(() => document.getElementById('lang-select').value);
    check(frLang === 'fr', `[5] window.__setLanguage('fr') took — selector reads "${frLang}"`);
    await sweep('fr');

    await page.evaluate((l) => window.__setLanguage(l), 'en');
    await page.waitForTimeout(150);
    const backSt = await hoverAndRead('#threshold-knob');
    check(backSt.visible && backSt.title === I18N['tip.threshold'].en.t
          && backSt.body === I18N['tip.threshold'].en.b,
        `[5] switching back to English restores the English tip byte-for-byte — "${backSt.title}"`);
    await park();

    // ── NC-2. the focus latch, both halves ──────────────────────────────────
    // This plugin diverges from the O-simpleFM reference here on purpose, and a
    // divergence with no control is a regression waiting to be inherited.
    console.log('\n-- NC-2: the focus latch (a click must not pin a tip; a key must open one)');

    await page.mouse.move(1, 1);
    await page.waitForTimeout(150);
    // BLUR FIRST, and this line is the whole reason NC-2a can fail at all. The
    // sweep above ends on Escape, and this page's popover handler focuses
    // #gear-btn on Escape — so without the blur the click lands on an ALREADY
    // FOCUSED element, fires no focusin, and "no tip after a click" is true for
    // a page with no latch whatsoever.
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
        return { shown, overlap, popoverOpen: !!pr,
                 tip: [Math.round(r.x), Math.round(r.y), Math.round(r.width), Math.round(r.height)] };
    });
    check(afterClick.popoverOpen, '[NC-2a] the click really did open the settings popover');
    check(!afterClick.shown,
        '[NC-2a] a POINTER click opens no tip — the latch suppresses the focusin arm'
        + (afterClick.shown
           ? ` — it is showing at [${afterClick.tip.join(',')}] and covers the popover by `
             + `${afterClick.overlap} px2`
           : ''));

    // The keyboard half. A real tab-ring walk, not a programmatic .focus():
    // Chromium reports :focus-visible false for a .focus() that follows a click,
    // and .focus() on an already-focused element fires no event at all — either
    // one would report "no tip" and record that as correct.
    await page.keyboard.press('Escape');
    await page.waitForTimeout(150);
    await page.evaluate(() => document.activeElement && document.activeElement.blur());
    await page.waitForTimeout(100);
    let kbHit = null;
    for (let i = 1; i <= 20; i++) {
        await page.keyboard.press('Tab');
        // AWAITED past the 0.12s fade, not slept for a fixed 80ms. READ_TIP calls
        // a tip visible only at opacity > 0.99, and 80ms into a 120ms transition
        // reads ~0.66 — which is how the first run of this control reported
        // "none in 20 tabs" for a keyboard path that demonstrably works (tab #5,
        // 250 x 115 at 236,37). A control that fails for its own timing reasons
        // is worse than no control: it would have been "fixed" by deleting the
        // latch.
        await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 400 })
                  .catch(() => {});
        const r = await page.evaluate(`(${READ_TIP})()`);
        const on = await page.evaluate(() => (document.activeElement || {}).id || null);
        if (r && r.visible && (r.title || '').length) { kbHit = { press: i, on, ...r }; break; }
    }
    check(kbHit !== null,
        '[NC-2b] a KEYBOARD tab still opens a tip — the accessibility half survives the latch'
        + (kbHit ? ` (tab #${kbHit.press} on #${kbHit.on}, "${kbHit.title}")` : ' — none in 20 tabs'));
    check(kbHit !== null && inFrame(kbHit.rect),
        '[NC-2b] the focus-placed tip is inside the frame'
        + (kbHit ? ` — ${kbHit.rect.w.toFixed(1)} x ${kbHit.rect.h.toFixed(1)}, ${edges(kbHit.rect)}` : ''));
    await page.keyboard.press('Escape');
    await page.waitForTimeout(150);
    await park();

    // ── NC-1. plant an over-long body; assertion 4 must report it ───────────
    // A namespaced per-run directory, never a bare filename at a shared temp
    // root: several executors run in this scratchpad at once and a bare
    // i18n.orig.js is not yours. Restored by COPY, never with
    // `git checkout -- <file>`.
    console.log('\n-- NC-1: plant an over-long body and confirm assertion 4 reports the overflow');
    const nc = fs.mkdtempSync(path.join(os.tmpdir(), 'ocomp-tip-nc-'));
    const servedI18n = path.join(built.root, 'js', 'i18n.js');
    const backup = path.join(nc, 'i18n.served.orig.js');
    fs.copyFileSync(servedI18n, backup);
    try {
        const LONG = ('Une phrase de contrôle négatif, délibérément beaucoup trop longue pour '
                    + 'tenir dans le cadre, répétée afin de faire déborder la surface par le bas. ')
                    .repeat(9);
        const before = fs.readFileSync(backup, 'utf8');
        const planted = before.replace(
            /('tip\.threshold'[\s\S]*?en:\s*\{\s*t:\s*"Threshold",\s*\n\s*b:\s*)"[^"]*"/,
            `$1"${LONG}"`);
        check(planted !== before,
            '[NC-1] the plant actually edited tip.threshold\'s en body — a no-op replace would '
            + 'make this control vacuous');
        fs.writeFileSync(servedI18n, planted);

        await page.goto(url, { waitUntil: 'networkidle' });
        const bad = await hoverAndRead('#threshold-knob');
        check(bad.visible, `[NC-1] the planted tip still renders (${bad.rect.h.toFixed(1)}px tall)`);
        check(!inFrame(bad.rect),
            `[NC-1] assertion 4 REPORTS the overflow — rect ${bad.rect.w.toFixed(1)} x `
            + `${bad.rect.h.toFixed(1)} at ${bad.rect.x.toFixed(1)},${bad.rect.y.toFixed(1)} leaves `
            + `the ${SHIP_W} x ${SHIP_H} frame (${edges(bad.rect)}). If this PASSES as in-frame, `
            + `assertion 4 is decoration and every green above means nothing`);
    } finally {
        fs.copyFileSync(backup, servedI18n);
    }

    await page.goto(url, { waitUntil: 'networkidle' });
    const restored = await hoverAndRead('#threshold-knob');
    check(restored.visible && restored.body === I18N['tip.threshold'].en.b && inFrame(restored.rect),
        `[NC-1] restored from the namespaced copy — tip.threshold is back inside the frame `
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
