/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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
    O-Texture — hover-help RENDER verification at the shipping viewport (v0.3.0)

    WHY THIS FILE EXISTS. No gate in this repo can see a rendered tooltip.
    check-i18n reads the table statically and is satisfied by TIP_BINDINGS being
    non-empty and its keys resolving. check-ui-labels has no tooltip awareness
    whatsoever — its output is BYTE-IDENTICAL before and after this whole
    feature landed. boot-all-uis counts aria-label and title and never data-tip.
    So a plugin can author copy, bind it, and pass all three green while
    shipping nothing on screen at all, which is precisely the state O-Texture
    was in at v0.2.0: canon v2's applyI18n() writes data-tip-title and data-tip
    onto the anchors and stops there, and this page had no tooltip element, no
    .tooltip rule and no hover handler to read them.

    This file is the assertion those three cannot make. It drives the REAL page
    — same index.html, same css/ouaricon-naturalist.css, same js/main.js, only
    js/juce/index.js swapped for the shared bridge stub — in a browser pinned to
    the exact shipping frame, and measures the rectangle that actually paints.

    NOT a port of the three committed ui_tooltip_clamp_check.js gates
    (O-Tapestop, O-Bitrot, O-ReverseDelay). Those are ~40 KB each and describe
    the OTHER renderer family: measure-then-pin placement with an above/below
    flip and an arrow. O-Texture ports O-simpleFM's delegated, cursor-following
    renderer, which has neither, so those assertions would not describe it.

    WHAT IS ASSERTED, per anchor, in English and again in French:

      1. every TIP_BINDINGS selector resolves AND its wrapper walk lands on a
         real element — a binding that finds nothing is a FAIL here, because
         applyI18n() only console.warns about it;
      2. hovering the anchor makes the surface VISIBLE carrying non-empty text.
         This is the vacuity guard and the assertion the whole file exists for;
      3. the rendered title and body are BYTE-EQUAL to the table entry. Not
         "contains" — a .tip-title that silently kept the previous anchor's
         text passes a contains check and fails this one;
      4. the tip rectangle is fully inside the 800 x 600 frame on all four
         edges, and inside the max-width cap.

    Then French (2-4 again against the fr entries), then back to English to
    prove the switch is reversible rather than one-way.

    THREE NEGATIVE CONTROLS, because "all pass" is worthless without them:

      NC-1  an over-long body is planted in the SERVED copy of js/i18n.js and
            the page reloaded; assertion 4 must REPORT the overflow. Restored
            from a namespaced per-run copy, never with `git checkout --`, which
            would take the uncommitted params.tsv and CMake work in this tree
            with it (O-GrainScatter lost a whole edit that way).
      NC-2  a plain mouse click must leave NO tip parked on screen, and a real
            keyboard tab MUST still open one. Those are the two halves of the
            lastInputWasPointer latch this page adds over the reference
            renderer. Asserting only the first lets the feature decay into
            "focus never shows a tip", which passes it perfectly while silently
            removing the keyboard half of hover-help.
            THE BLUR ON LINE ~470 IS WHAT MAKES NC-2a ABLE TO FAIL. An earlier
            section of this gate leaves focus on #gear-btn, and clicking an
            ALREADY-FOCUSED element fires no focusin at all — so without the
            blur the assertion is green for a page with no latch whatsoever.
      NC-3  the z-index. body::after paints a 180 x 180 fern at z-index 1000
            over the bottom-right corner, directly on top of .freeze-toggle.
            A visibility-only hover check cannot see a tip painted UNDER it,
            so the surface's computed z-index is asserted against the fern's.

    Usage:  node plugins/O-Texture/tests/ui_tip_render_check.js
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

const PLUGIN    = 'O-Texture';
const publicDir = path.join(pluginRoot, 'Source', 'ui', 'public');
const cssPath   = path.join(publicDir, 'css', 'ouaricon-naturalist.css');
const htmlPath  = path.join(publicDir, 'index.html');
const mainPath  = path.join(publicDir, 'js', 'main.js');
const i18nPath  = path.join(publicDir, 'js', 'i18n.js');

// Mirrored from PluginEditor.cpp, the stylesheet and js/main.js — and every one
// is CROSS-CHECKED against its source below. A fixture that mirrors a constant
// without checking it starts describing the release before it and keeps passing
// (pattern_test_fixture_mirrors_drift_silently).
const SHIP_W = 800;
const SHIP_H = 600;
const MARGIN = 8;               // setupTooltips()'s clamp margin
const DOCUMENTED_MAX_W = 260;   // .tooltip max-width
const FERN_Z = 1000;            // body::after, the botanical overlay

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
// Reads the surface's computed visibility, its rect, its z-index and its two
// text parts SEPARATELY: the title lives in a .tip-title span and the body is
// the text nodes after it, exactly as setupTooltips() builds them with
// createElement + createTextNode. Reading tip.textContent alone would
// concatenate the two and leave assertion 3 unable to tell a swapped title from
// a swapped body.
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
        zIndex: cs.zIndex,
        pointerEvents: cs.pointerEvents,
        title: ti ? ti.textContent : null,
        body,
        rect: { x: r.x, y: r.y, w: r.width, h: r.height },
    };
}`;

(async () => {
    console.log(`== ${PLUGIN} ui_tip_render_check ==`);
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}\n`);

    const html = fs.readFileSync(htmlPath, 'utf8');
    const css  = fs.readFileSync(cssPath, 'utf8');
    const main = fs.readFileSync(mainPath, 'utf8');
    const i18nSrc = fs.readFileSync(i18nPath, 'utf8');

    // ── 0. the mirrored constants, guarded ──────────────────────────────────
    const size = S.readEditorSize(PLUGIN);
    check(size && size.w === SHIP_W && size.h === SHIP_H,
        `[0] editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below `
        + `(PluginEditor.cpp says ${size ? `${size.w} x ${size.h}` : 'NOTHING'})`);

    // Parsed from the .tooltip RULE specifically. This stylesheet also carries a
    // width on .settings-popover and a width/height on .knob, .source-button and
    // .freeze-button, so a loose scan for the first max-width would silently
    // measure against one of those.
    const tipRule  = css.match(/\n\.tooltip\s*\{[\s\S]*?\n\}/);
    const capMatch = tipRule && tipRule[0].match(/max-width:\s*(\d+(?:\.\d+)?)px/);
    const MAX_W    = capMatch ? parseFloat(capMatch[1]) : NaN;
    check(Number.isFinite(MAX_W) && MAX_W === DOCUMENTED_MAX_W,
        `[0] .tooltip max-width parsed from the stylesheet is the documented ${DOCUMENTED_MAX_W}px `
        + `— got ${capMatch ? capMatch[1] + 'px' : 'NOTHING'}. French wraps INSIDE this cap, so `
        + `moving it changes every tip height and therefore every clamp decision below`);
    check(new RegExp(`MARGIN\\s*=\\s*${MARGIN}\\b`).test(main),
        `[0] setupTooltips()'s clamp MARGIN is ${MARGIN} (this file mirrors it)`);
    check(/position:\s*fixed/.test(tipRule ? tipRule[0] : '')
       && /visibility:\s*hidden/.test(tipRule ? tipRule[0] : '')
       && /pointer-events:\s*none/.test(tipRule ? tipRule[0] : ''),
        '[0] .tooltip is position:fixed + visibility:hidden + pointer-events:none — the three '
        + 'properties that keep an unshown surface out of check-ui-labels\' sweep and stop a '
        + 'shown one from stealing its own hover');
    check(/id="tooltip"/.test(html), '[0] index.html carries the #tooltip surface');
    // HTML COMMENTS ARE STRIPPED FIRST, exactly as check-i18n assertion 11 does
    // it. This page's markup DOCUMENTS the six title="Coming soon" attributes
    // v0.2.0 deleted, inside a comment — a naive scan reads that prose as a live
    // attribute and fails a page that is correct. (It did, on this gate's first
    // run.) The <title> ELEMENT is stripped too; it is not an attribute.
    const markup = html.replace(/<!--[\s\S]*?-->/g, '')
                       .replace(/<title>[\s\S]*?<\/title>/, '');
    check(/-->/.test(html) && !/<!--/.test(markup),
        '[0] the comment strip above actually ran — without it this scan reads the '
        + 'documented-and-deleted title= attributes in the markup comments as live ones');
    check(!/\btitle\s*=\s*["']/.test(markup),
        '[0] no native title= attribute reintroduced by the renderer (contract §4)');
    // The renderer is called AFTER initI18n(), inside the same try/catch. Before
    // initI18n() every anchor is bare and every hover would open an empty box.
    // v0.4.0: the same try/catch now also carries initializeTipsToggle(), which
    // needs setLabel() and would be a TDZ throw taking the whole module if it
    // were called at top level. The ORDER assertion is what matters and is
    // unchanged — initI18n() first, then the renderer — so the pattern admits
    // further guarded calls after setupTooltips() rather than pinning the line
    // byte-for-byte and going red the next time one is added.
    check(/try\s*\{\s*initI18n\(\);\s*setupTooltips\(\);[^}]*\}\s*catch/.test(main),
        '[0] setupTooltips() is called AFTER initI18n() and inside the same try/catch');
    check(/try\s*\{[^}]*initializeTipsToggle\(\);[^}]*\}\s*catch/.test(main),
        '[0] initializeTipsToggle() is inside that SAME guarded block — it reads setLabel, '
        + 'and a top-level call reaching a lower let/const is the TDZ throw that takes the '
        + 'whole module with it');

    // ── the table ───────────────────────────────────────────────────────────
    const { I18N, TIP_BINDINGS, LANGUAGES } = loadTable(i18nSrc);
    check(Array.isArray(TIP_BINDINGS) && TIP_BINDINGS.length > 0,
        `[0] TIP_BINDINGS parsed from js/i18n.js — ${TIP_BINDINGS.length} anchor(s)`);
    check(LANGUAGES.join(',') === 'en,fr', `[0] LANGUAGES is en,fr — got ${LANGUAGES.join(',')}`);

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
    // At 1280 every anchor on this page has room to its right, the horizontal
    // flip never engages, and every assertion below would pass while the real
    // 800 px frame overflowed (pattern_tooltip_clamp_gate_viewport_sensitive).
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
        `[0] the browser really is ${SHIP_W} x ${SHIP_H} — got ${vp.width} x ${vp.height}`);

    // Non-vacuity: js/main.js must have RUN. A TDZ throw out of module
    // evaluation leaves the HTML looking correct and every control dead
    // (pattern_module_toplevel_init_tdz). The knob indicator's transform is
    // written ONLY by updateKnob() — the markup carries no inline style — so a
    // populated transform proves the module reached its bindings.
    const alive = await page.evaluate(
        () => (document.querySelector('#knob-brightness .knob-indicator') || {}).style?.transform || '');
    check(/rotate\(/.test(alive),
        `[0] js/main.js ran — the brightness indicator carries a JS-written transform "${alive}"`);

    // ── NC-3. the z-index, against the fern that would hide a tip ───────────
    // A hover check that reads only `visibility` cannot see a tip painted UNDER
    // the 180 x 180 botanical overlay body::after pins to the bottom-right
    // corner at z-index 1000 — which is exactly where .freeze-toggle and the
    // right end of the source row sit.
    const fernZ = (css.match(/body::after\s*\{[\s\S]*?z-index:\s*(\d+)/) || [])[1];
    check(Number(fernZ) === FERN_Z,
        `[NC-3] body::after still paints at z-index ${FERN_Z} — parsed ${fernZ}`);

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
            hasTip: target.hasAttribute('data-tip') && target.hasAttribute('data-tip-title'),
            area: `${Math.round(r.width)}x${Math.round(r.height)} at ${Math.round(r.x)},${Math.round(r.y)}`,
        };
    }), TIP_BINDINGS);

    for (const r of resolution) {
        check(r.ok, `[1] binding ${r.key} resolves — ${r.sel}${r.ok ? ` -> ${r.tag} (${r.area})` : ` (${r.why})`}`);
        if (r.ok) check(r.hasTip,
            `[1] applyI18n wrote data-tip-title + data-tip onto ${r.key}'s anchor (${r.tag})`);
    }
    check(tipWarns.length === 0,
        '[1] applyI18n logged no "tip target not found" warning'
        + (tipWarns.length ? ` — ${tipWarns.length}: ${tipWarns[0]}` : ''));

    // Every binding must land on a DISTINCT element. Two bindings resolving to
    // the same node is the specific failure this page is exposed to — X and Y
    // share the XY pad canvas, and a second setAttribute would overwrite the
    // first, leaving one entry permanently unrenderable while check-i18n
    // reported both as bound.
    const targets = await page.evaluate((bindings) => {
        const seen = [];
        for (const [sel, key, wrapper] of bindings) {
            const el = document.querySelector(sel);
            const t = el ? (wrapper ? el.closest(wrapper) || el : el) : null;
            seen.push({ key, idx: t ? [...document.querySelectorAll('*')].indexOf(t) : -1 });
        }
        return seen;
    }, TIP_BINDINGS);
    const dupes = targets.filter((t, i) => targets.findIndex((u) => u.idx === t.idx) !== i);
    check(dupes.length === 0,
        `[1] every binding lands on a DISTINCT element — ${TIP_BINDINGS.length} bindings, `
        + `${new Set(targets.map((t) => t.idx)).size} targets`
        + (dupes.length ? ` — COLLIDING: ${dupes.map((d) => d.key).join(', ')}` : ''));

    // ── the hover driver ────────────────────────────────────────────────────
    // Hovers the SELECTOR (the addressable child), not the wrapper: that is what
    // a user's pointer actually lands on, and closest('[data-tip]') is what has
    // to walk up from it. Hovering the wrapper directly would prove the wrapper
    // carries a tip and nothing about whether the child reaches it.
    //
    // .first(), because applyI18n uses document.querySelector — first match —
    // and two of these selectors (.source-button, .mode-toggle button) match a
    // whole row. Hovering a different member than the one applyI18n keyed would
    // be measuring a different question.
    //
    // page.evaluate() given a STRING evaluates it as an EXPRESSION, so passing
    // READ_TIP directly returns the function OBJECT rather than calling it — and
    // an unserializable return arrives as `undefined`, which reads exactly like
    // "the tip was never there" and sails through a truthiness assertion.
    // Invoked explicitly instead.
    const readTip = () => page.evaluate(`(${READ_TIP})()`);

    const park = async () => {
        await page.mouse.move(2, 2);
        await page.waitForFunction(
            `(${READ_TIP})() === null || !(${READ_TIP})().visible`, null, { timeout: 2000 }
        ).catch(() => {});
    };

    const hoverAndRead = async (sel) => {
        await park();
        await page.locator(sel).first().hover({ force: true });
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
            check(Number(st.zIndex) > FERN_Z,
                `[NC-3][${lang}] ${key}: the surface paints ABOVE the fern overlay `
                + `— z-index ${st.zIndex} vs ${FERN_Z}`);
        }
        check(seen.length === TIP_BINDINGS.length,
            `[2][${lang}] every one of the ${TIP_BINDINGS.length} bound anchors was driven — got ${seen.length}`);
        if (popoverOpen) {
            await page.keyboard.press('Escape');
            await page.waitForTimeout(150);
            // Escape returns focus to the gear (initializeSettingsPopover), and
            // that focus IS keyboard-driven, so the gear's tip legitimately
            // opens. Blur it so the next sweep starts from a clean surface.
            await page.evaluate(() => document.activeElement && document.activeElement.blur());
            await park();
        }
        return seen;
    };

    await sweep('en');

    // ── 5. French, then back ────────────────────────────────────────────────
    // French runs 15-20% longer, wraps to more lines against the max-width cap
    // and grows the tip's HEIGHT, so a tip that fits in English can overflow the
    // bottom in French. That is why the whole sweep repeats rather than
    // spot-checking one anchor.
    await page.evaluate((l) => window.__setLanguage(l), 'fr');
    await page.waitForTimeout(150);
    const frLang = await page.evaluate(() => document.getElementById('lang-select').value);
    check(frLang === 'fr', `[5] window.__setLanguage('fr') took — selector reads "${frLang}"`);
    await sweep('fr');

    await page.evaluate((l) => window.__setLanguage(l), 'en');
    await page.waitForTimeout(150);
    const backSt = await hoverAndRead('#xy-pad');
    check(backSt.visible && backSt.title === I18N['tip.xyPad'].en.t
          && backSt.body === I18N['tip.xyPad'].en.b,
        `[5] switching back to English restores the English tip byte-for-byte — "${backSt.title}"`);
    await park();

    // ── 6. the disabled members of the two button ROWS ──────────────────────
    // SOURCE and MODE are ROW parameters: five of the six source buttons and one
    // of the two mode buttons ship `disabled`, and a disabled control is exactly
    // where a user most wants to know why. This is REPORTED rather than asserted
    // one way: Chromium's dispatch behaviour over a disabled form control is the
    // thing being measured, and a gate that asserted the answer it happened to
    // get would be recording a browser detail as a requirement.
    console.log('\n-- 6. hovering a DISABLED member of a row-bound parameter');
    for (const [label, sel, key] of [['source row (Metal)', '.source-button:nth-of-type(2)', 'tip.source'],
                                     ['mode row (Transform)', '.mode-toggle button:disabled', 'tip.mode']]) {
        const st = await hoverAndRead(sel);
        check(st && st.visible && st.title === I18N[key].en.t,
            `[6] ${label}: the row's tip opens over a DISABLED button — `
            + `"${st && st.title}" (Chromium retargets a pointer event over a disabled form `
            + 'control to the nearest enabled ancestor, which is the row the wrapper binds; '
            + 'bound to the buttons themselves this would be dead over five of six)');
    }
    await park();

    // ── NC-2. the focus latch, both halves ──────────────────────────────────
    console.log('\n-- NC-2: the focus latch (a click must not pin a tip; a key must open one)');
    await page.mouse.move(1, 1);
    await page.waitForTimeout(150);
    // BLUR FIRST. Clicking an ALREADY-FOCUSED element fires no focusin at all,
    // so on a gate whose earlier section left focus on #gear-btn this assertion
    // is green for a page with no latch whatsoever — the orchestrator's first
    // version of this control passed 125/125 with the latch deleted.
    //
    // MEASURED HERE, all four combinations, rather than inherited:
    //
    //     latch  blur   NC-2a              NC-2b
    //     yes    yes    PASS (208/208)     PASS
    //     yes    no     PASS               PASS
    //     no     yes    FAIL  5130 px2     PASS
    //     no     no     FAIL  5130 px2     PASS
    //
    // So on THIS page the blur is not what makes the control able to fail —
    // sweep() already blurs after its Escape, so the click lands on an
    // unfocused gear either way. It stays because that is an accident of the
    // order sections run in, and a control whose ability to fail depends on
    // what an earlier section happened to leave focused is one edit from being
    // decoration. The 5130 px2 is the gear's own tip lying across the settings
    // popover the click had just opened. NC-2b passing in all four runs is what
    // proves the two halves are independent rather than one claim counted twice.
    await page.evaluate(() => document.activeElement && document.activeElement.blur());
    await page.waitForTimeout(100);
    await page.click('#gear-btn');
    await page.waitForTimeout(300);
    // Measured as an OVERLAP AREA, not merely observed. "A tip is showing"
    // becomes a number a later run can compare.
    const afterClick = await page.evaluate(() => {
        const t = document.getElementById('tooltip');
        const cs = getComputedStyle(t);
        const r = t.getBoundingClientRect();
        const p = document.getElementById('settings-popover');
        const pr = p && !p.hidden ? p.getBoundingClientRect() : null;
        const shown = cs.visibility !== 'hidden' && cs.opacity !== '0';
        let overlap = 0;
        if (shown && pr) {
            const ox = Math.max(0, Math.min(r.right, pr.right) - Math.max(r.left, pr.left));
            const oy = Math.max(0, Math.min(r.bottom, pr.bottom) - Math.max(r.top, pr.top));
            overlap = Math.round(ox * oy);
        }
        return { shown, overlap, popoverOpen: !!pr };
    });
    check(afterClick.popoverOpen, '[NC-2a] the click did open the settings popover — the control is live');
    check(!afterClick.shown,
        '[NC-2a] a POINTER click leaves NO tip parked on screen — the latch suppresses the '
        + 'focusin arm'
        + (afterClick.overlap ? ` (it covered the popover by ${afterClick.overlap} px2)` : ''));

    // The keyboard half. A real tab-ring walk, not a programmatic .focus():
    // Chromium reports :focus-visible false for a .focus() that follows a click,
    // and .focus() on an already-focused element fires no event at all — either
    // one would report "no tip" and record that as correct.
    await page.keyboard.press('Escape');
    await page.waitForTimeout(150);
    await page.evaluate(() => document.activeElement && document.activeElement.blur());
    await park();
    let kbHit = null;
    for (let i = 1; i <= 20; ++i) {
        await page.keyboard.press('Tab');
        await page.waitForTimeout(80);
        const r = await readTip();
        const on = await page.evaluate(
            () => (document.activeElement ? (document.activeElement.id
                   || document.activeElement.className || document.activeElement.tagName) : null));
        if (r && r.visible && (r.title || '').length) { kbHit = { press: i, title: r.title, on, rect: r.rect }; break; }
    }
    check(kbHit !== null,
        '[NC-2b] a KEYBOARD tab still opens a tip — the accessibility half survives the latch'
        + (kbHit ? ` (tab #${kbHit.press} on ${kbHit.on}, "${kbHit.title}")` : ' — none in 20 tabs'));
    check(kbHit !== null && inFrame(kbHit.rect),
        '[NC-2b] the focus-placed tip is inside the frame'
        + (kbHit ? ` — ${edges(kbHit.rect)}` : ''));
    await page.keyboard.press('Escape');
    await page.evaluate(() => document.activeElement && document.activeElement.blur());
    await park();

    // ── NC-1. plant an over-long body; assertion 4 must report it ───────────
    // A namespaced per-run directory, never a bare filename at a shared temp
    // root: several executors run in this scratchpad at once and a bare
    // i18n.orig.js is not yours. Restored by COPY, never with
    // `git checkout -- <file>`.
    console.log('\n-- NC-1: plant an over-long body and confirm assertion 4 reports the overflow');
    const nc = fs.mkdtempSync(path.join(os.tmpdir(), 'otex-tip-nc-'));
    const servedI18n = path.join(built.root, 'js', 'i18n.js');
    const backup = path.join(nc, 'i18n.served.orig.js');
    fs.copyFileSync(servedI18n, backup);
    try {
        const LONG = ('Une phrase de controle negatif, deliberement beaucoup trop longue pour '
                    + 'tenir dans le cadre, repetee afin de faire deborder la surface par le bas. ')
                    .repeat(12);
        const orig = fs.readFileSync(backup, 'utf8');
        const planted = orig.replace(
            /('tip\.xyPad'[\s\S]*?en:\s*\{\s*t:\s*"XY Pad",\s*\n\s*b:\s*)"[^"]*"/,
            `$1"${LONG}"`);
        check(planted !== orig,
            '[NC-1] the plant actually edited tip.xyPad\'s en body — a no-op replace would make '
            + 'this control vacuous');
        fs.writeFileSync(servedI18n, planted);

        await page.goto(url, { waitUntil: 'networkidle' });
        const bad = await hoverAndRead('#xy-pad');
        check(bad.visible, `[NC-1] the planted tip still renders (${bad.rect.h.toFixed(1)}px tall)`);
        check(!inFrame(bad.rect),
            `[NC-1] assertion 4 REPORTS the overflow — rect ${bad.rect.w.toFixed(1)} x `
            + `${bad.rect.h.toFixed(1)} at ${bad.rect.x.toFixed(1)},${bad.rect.y.toFixed(1)} `
            + `leaves the ${SHIP_W} x ${SHIP_H} frame (${edges(bad.rect)}). If this PASSES as `
            + 'in-frame, assertion 4 is decoration and every green above means nothing');
    } finally {
        fs.copyFileSync(backup, servedI18n);
    }

    await page.goto(url, { waitUntil: 'networkidle' });
    const restored = await hoverAndRead('#xy-pad');
    check(restored.visible && restored.body === I18N['tip.xyPad'].en.b && inFrame(restored.rect),
        `[NC-1] restored from the namespaced copy — tip.xyPad is back inside the frame `
        + `(${edges(restored.rect)})`);
    fs.rmSync(nc, { recursive: true, force: true });

    // ── housekeeping ────────────────────────────────────────────────────────
    check(pageErrors.length === 0,
        'no uncaught page error across the whole run'
        + (pageErrors.length ? ` — ${pageErrors.length}: ${pageErrors[0].slice(0, 140)}` : ''));
    const realMisses = [...new Set(misses)].filter((m) => !/favicon/.test(m));
    check(realMisses.length === 0,
        'every requested resource was served'
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
