/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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
    O-TextureForge — hover-help RENDER verification at 900 x 600 (v1.2.0)

    WHY THIS FILE EXISTS. No gate in this repo can see a rendered tooltip.
    check-i18n reads the table statically and is satisfied by TIP_BINDINGS being
    non-empty and its keys resolving. check-ui-labels has no tooltip awareness
    whatsoever — its output on this plugin is BYTE-IDENTICAL before and after
    this whole feature landed, verified by diff, not asserted. boot-all-uis
    counts aria-label and title and never data-tip. So a plugin can author copy,
    bind it and pass all three green while shipping nothing on screen at all,
    which is exactly the state this page was in at v1.1.0: canon v2's applyI18n()
    writes data-tip-title and data-tip onto the anchors and stops there, and this
    page had no #tooltip element, no .tooltip rule and no hover handler to read
    them.

    NOT a port of the three committed ui_tooltip_clamp_check.js gates
    (O-Tapestop, O-Bitrot, O-ReverseDelay). Those are ~40 KB each and describe
    the OTHER renderer family — measure-then-pin placement with an above/below
    flip and an arrow. This page ports O-simpleFM's delegated, cursor-following
    renderer, which has neither, so those assertions would not describe it.

    THE PAGE THIS DRIVES IS THE WEBPACK-BUNDLED ONE. index.html loads
    js/app.bundle.js as a classic script and js/i18n_init.js as a module; the
    renderer and the canon both live in the MODULE, never in the bundle. The
    liveness checks below prove BOTH ran, because either one dying silently
    leaves a page that still looks correct.

    WHAT IS ASSERTED, per anchor, in English and again in French:

      1. every TIP_BINDINGS selector resolves AND its wrapper walk lands on a
         real element — a binding that finds nothing is a FAIL here, because
         applyI18n() only console.warns about it — and every binding lands on a
         DISTINCT node;
      2. hovering the anchor makes the surface VISIBLE carrying non-empty text.
         This is the vacuity guard and the assertion the whole file exists for;
      3. the rendered title and body are BYTE-EQUAL to the table entry. Not
         "contains" — a .tip-title that silently kept the previous anchor's text
         passes a contains check and fails this one;
      4. the tip rectangle is fully inside the 900 x 600 frame on all four
         edges, and within the 260 px max-width cap;
      5. the surface paints ABOVE body::after, the fern overlay at z-index 100.

    Then French (2-5 again against the fr entries), then back to English to
    prove the switch is reversible rather than one-way.

    FOUR NEGATIVE CONTROLS, because "all pass" is worthless without them:

      NC-1  an over-long body is planted in the SERVED copy of js/i18n.js and
            the page reloaded; assertion 4 must REPORT the overflow. The plant
            is SIZED AGAINST THIS FRAME and its rendered height is printed, not
            assumed: O-Tremolo's 40x plant fit inside a 400 px frame and
            reported nothing, and a plant that fits is indistinguishable from a
            gate that cannot see. Restored from a per-run mkdtemp copy, never
            with `git checkout -- <file>`, which would take the uncommitted
            params.tsv and CMake work in this tree with it (O-GrainScatter lost
            a whole edit that way).
      NC-2  a plain mouse click must leave NO tip parked on screen, and a real
            keyboard tab MUST still open one. Those are the two halves of the
            lastInputWasPointer latch this page adds over the reference
            renderer. Asserting only the first lets the feature decay into
            "focus never shows a tip", which passes it perfectly while silently
            removing the keyboard half of hover-help.

            THE BLUR BEFORE THE CLICK IS WHAT MAKES NC-2a ABLE TO FAIL. Clicking
            an ALREADY-FOCUSED element fires no focusin at all, so on a gate
            whose earlier section left focus on #gear-btn the assertion is green
            for a page with no latch whatsoever.

            MEASURED HERE, all four combinations, not inherited:

                latch  blur   NC-2a                 NC-2b
                yes    yes    PASS (287/287)        PASS
                yes    no     PASS                  PASS
                no     yes    FAIL — 4648 px2       PASS
                no     no     FAIL — 4648 px2       PASS

            ON THIS PAGE THE BLUR IS NOT WHAT GIVES THE CONTROL ITS POWER, and
            saying so is more useful than repeating the general warning. The
            section that runs immediately before this one is NC-4, which is
            pure mouse work, and the French sweep before it already blurred the
            gear after its Escape — so the click lands on an unfocused button
            either way and focusin fires either way. The blur stays because
            that is an accident of the order the sections happen to run in, and
            a control whose ability to fail depends on what an earlier section
            left focused is one edit away from being decoration. The 4648 px2
            is the gear's own tip lying across the settings popover the click
            had just opened. NC-2b passing in all four runs is what proves the
            two halves are independent rather than one claim counted twice.

            A FIFTH RUN, the harness-blindness control: with setupTooltips()
            commented out of the deferred init, this file reports 36 failures
            while check-i18n prints ALL CHECKS PASS and check-ui-labels is
            BYTE-IDENTICAL to its pre-feature baseline. That is the measurement
            behind the first paragraph of this comment.
      NC-3  the z-index, against the fern that would hide a tip. body::after
            paints a 250 x 340 botanical overlay at z-index 100 over the whole
            bottom-right corner — which is where the Gain and Crossfade knob
            cells and the right end of the drop zone sit. A hover check reading
            only `visibility` cannot see a tip painted UNDER it.
      NC-4  the DRAG GUARD, which is this page's one behavioural addition to the
            reference renderer. Every knob starts its drag on mousedown and
            tracks document.mousemove, so a vertical drag that strays into a
            neighbouring .knob-row would otherwise fire pointerover and open
            THAT row's tip on top of the control being turned. Driven with real
            mouse down / move / up, and the release is asserted too — a guard
            that never clears is a renderer that stops working after the first
            click.

    Usage:  node plugins/O-TextureForge/tests/ui_tip_render_check.js
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

const PLUGIN    = 'O-TextureForge';
const publicDir = path.join(pluginRoot, 'Source', 'ui', 'public');
const cssPath   = path.join(publicDir, 'css', 'ouaricon-naturalist.css');
const htmlPath  = path.join(publicDir, 'index.html');
const initPath  = path.join(publicDir, 'js', 'i18n_init.js');
const i18nPath  = path.join(publicDir, 'js', 'i18n.js');

// Mirrored from PluginEditor.cpp, the stylesheet and js/i18n_init.js — and every
// one is CROSS-CHECKED against its source below. A fixture that mirrors a
// constant without checking it starts describing the release before it and keeps
// passing (pattern_test_fixture_mirrors_drift_silently).
const SHIP_W = 900;
const SHIP_H = 600;
const MARGIN = 8;              // setupTooltips()'s clamp margin
const DOCUMENTED_MAX_W = 260;  // .tooltip max-width
const FERN_Z = 100;            // body::after, the botanical overlay

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

    const html    = fs.readFileSync(htmlPath, 'utf8');
    const css     = fs.readFileSync(cssPath, 'utf8');
    const initSrc = fs.readFileSync(initPath, 'utf8');
    const i18nSrc = fs.readFileSync(i18nPath, 'utf8');

    // ── 0. the mirrored constants, guarded ──────────────────────────────────
    const size = S.readEditorSize(PLUGIN);
    check(size && size.w === SHIP_W && size.h === SHIP_H,
        `[0] editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below `
        + `(PluginEditor.cpp says ${size ? `${size.w} x ${size.h}` : 'NOTHING'})`);

    // Parsed from the .tooltip RULE specifically. This stylesheet also carries a
    // width on .settings-popover and widths on .knob and .knob-row .knob-label,
    // so a loose scan for the first max-width would silently measure one of
    // those instead.
    const tipRule  = css.match(/\n\.tooltip\s*\{[\s\S]*?\n\}/);
    const capMatch = tipRule && tipRule[0].match(/max-width:\s*(\d+(?:\.\d+)?)px/);
    const MAX_W    = capMatch ? parseFloat(capMatch[1]) : NaN;
    check(Number.isFinite(MAX_W) && MAX_W === DOCUMENTED_MAX_W,
        `[0] .tooltip max-width parsed from the stylesheet is the documented ${DOCUMENTED_MAX_W}px `
        + `— got ${capMatch ? capMatch[1] + 'px' : 'NOTHING'}. French wraps INSIDE this cap, so `
        + `moving it changes every tip height and therefore every clamp decision below`);
    check(new RegExp(`MARGIN\\s*=\\s*${MARGIN}\\b`).test(initSrc),
        `[0] setupTooltips()'s clamp MARGIN is ${MARGIN} (this file mirrors it)`);
    check(/position:\s*fixed/.test(tipRule ? tipRule[0] : '')
       && /visibility:\s*hidden/.test(tipRule ? tipRule[0] : '')
       && /pointer-events:\s*none/.test(tipRule ? tipRule[0] : ''),
        '[0] .tooltip is position:fixed + visibility:hidden + pointer-events:none — the three '
        + "properties that keep an unshown surface out of check-i18n assertion 10's text sweep "
        + 'and stop a shown one from stealing its own hover');
    check(/id="tooltip"/.test(html), '[0] index.html carries the #tooltip surface');

    // HTML COMMENTS ARE STRIPPED FIRST, exactly as check-i18n assertion 11 does
    // it: this page's markup documents its own history in comments, and a naive
    // scan reads that prose as live attributes.
    const markup = html.replace(/<!--[\s\S]*?-->/g, '')
                       .replace(/<title>[\s\S]*?<\/title>/, '');
    check(/-->/.test(html) && !/<!--/.test(markup),
        '[0] the comment strip above actually ran — without it this scan reads prose inside '
        + 'the markup comments as live attributes');
    check(!/\btitle\s*=\s*["']/.test(markup),
        '[0] no native title= attribute reintroduced by the renderer (contract §4). Repo-wide '
        + 'native title= is 0 and this page must not be the one that puts it back');

    // The renderer is called AFTER initI18n(), inside the same try/catch. Before
    // initI18n() every anchor is bare and every hover would open an empty box.
    // v1.3.0: the same try/catch now also carries initializeTipsToggle(), which
    // needs setLabel() and would be a TDZ throw taking the whole module if it
    // were called at top level. The ORDER assertion is what matters and is
    // unchanged — initI18n() first, then the renderer — so the pattern admits
    // further guarded calls after setupTooltips() rather than pinning the line
    // byte-for-byte and going red the next time one is added.
    check(/try\s*\{\s*initI18n\(\);\s*setupTooltips\(\);[^}]*\}\s*catch/.test(initSrc),
        '[0] setupTooltips() is called AFTER initI18n() and inside the same try/catch');
    check(/try\s*\{[^}]*initializeTipsToggle\(\);[^}]*\}\s*catch/.test(initSrc),
        '[0] initializeTipsToggle() is inside that SAME guarded block — it reads setLabel, '
        + 'and a top-level call reaching a lower let/const is the TDZ throw that takes the '
        + 'whole module with it');

    // The renderer is NOT in the webpack bundle. If it ever migrates there,
    // src/app.js becomes the source of truth and this whole file is pointed at
    // the wrong artefact.
    check(/function setupTooltips\s*\(/.test(initSrc)
       && !fs.readFileSync(path.join(pluginRoot, 'Source', 'ui', 'src', 'app.js'), 'utf8')
              .includes('setupTooltips'),
        '[0] the renderer lives in js/i18n_init.js and NOT in the webpack input src/app.js — '
        + 'putting it in the bundle would ship the label table twice and need a webpack '
        + 'rebuild inside every copy change');

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
    // 900 px frame overflowed (pattern_tooltip_clamp_gate_viewport_sensitive).
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
    await page.waitForTimeout(400);

    const vp = page.viewportSize();
    check(vp.width === SHIP_W && vp.height === SHIP_H,
        `[0] the browser really is ${SHIP_W} x ${SHIP_H} — got ${vp.width} x ${vp.height}`);

    // ── liveness, BOTH scripts ──────────────────────────────────────────────
    // The classic bundle: .knob-indicator carries no inline style in the markup,
    // and only setupKnob's updateDisplay() writes one. A populated transform is
    // proof the bundle reached its bindings rather than merely parsing.
    const bundleAlive = await page.evaluate(
        () => (document.querySelector('.knob[data-param="energy"] .knob-indicator') || {})
                  .style?.transform || '');
    check(/rotate\(/.test(bundleAlive),
        `[0] js/app.bundle.js ran — the energy indicator carries a JS-written transform `
        + `"${bundleAlive}"`);
    // The module: __setLanguage is published by the canon at the END of
    // i18n_init.js, so its presence proves module evaluation reached the bottom
    // and no TDZ throw took the later initializers with it.
    const moduleAlive = await page.evaluate(
        () => typeof window.__setLanguage === 'function' && typeof window.__setLabel === 'function');
    check(moduleAlive,
        '[0] js/i18n_init.js ran to the bottom — window.__setLanguage and window.__setLabel '
        + 'are both published (a TDZ throw at module scope would leave the page looking correct '
        + 'and every tip dead)');

    // ── NC-3. the z-index, against the fern that would hide a tip ───────────
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
        check(r.ok, `[1] binding ${r.key} resolves — ${r.sel}`
            + (r.ok ? ` -> ${r.tag} (${r.area})` : ` (${r.why})`));
        if (r.ok) check(r.hasTip,
            `[1] applyI18n wrote data-tip-title + data-tip onto ${r.key}'s anchor (${r.tag})`);
    }
    check(tipWarns.length === 0,
        '[1] applyI18n logged no "tip target not found" warning'
        + (tipWarns.length ? ` — ${tipWarns.length}: ${tipWarns[0]}` : ''));

    // Every binding must land on a DISTINCT element. applyI18n writes onto
    // whatever a selector resolves to, so two bindings on one node mean the
    // second overwrites the first, leaving one entry permanently unrenderable
    // while check-i18n reports both as bound. This page is exposed to it twice:
    // #gear-btn and #lang-select share .settings-cluster, and #midi-mode shares
    // .bottom-controls with #drop-zone, so all three bind BARE.
    const targets = await page.evaluate((bindings) => {
        const all = [...document.querySelectorAll('*')];
        return bindings.map(([sel, key, wrapper]) => {
            const el = document.querySelector(sel);
            const t = el ? (wrapper ? el.closest(wrapper) || el : el) : null;
            return { key, idx: t ? all.indexOf(t) : -1 };
        });
    }, TIP_BINDINGS);
    const dupes = targets.filter((t, i) => targets.findIndex((u) => u.idx === t.idx) !== i);
    check(dupes.length === 0,
        `[1] every binding lands on a DISTINCT element — ${TIP_BINDINGS.length} bindings, `
        + `${new Set(targets.map((t) => t.idx)).size} targets`
        + (dupes.length ? ` — COLLIDING: ${dupes.map((d) => d.key).join(', ')}` : ''));

    // ── the hover driver ────────────────────────────────────────────────────
    // Hovers the SELECTOR (the addressable child), not the wrapper: that is what
    // a user's pointer lands on, and closest('[data-tip]') is what has to walk
    // up from it. Hovering the wrapper directly would prove the wrapper carries
    // a tip and nothing about whether the child reaches it.
    //
    // page.evaluate() given a STRING evaluates it as an EXPRESSION, so passing
    // READ_TIP directly returns the function OBJECT rather than calling it — and
    // an unserializable return arrives as `undefined`, which reads exactly like
    // "the tip was never there" and sails through a truthiness assertion (O-Bass
    // shipped its first green run that way). Invoked explicitly instead.
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
        // so a fixed sleep either flakes or reports a false "never opened" —
        // which is the artefact whose obvious response is to delete the latch
        // (O-Comp, O-SimpleReverb, M1).
        await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 2000 })
                  .catch(() => {});
        return readTip();
    };

    const inFrame = (r) =>
        r.x >= 0 && r.y >= 0 && r.x + r.w <= SHIP_W && r.y + r.h <= SHIP_H;
    const edges = (r) =>
        `L${r.x.toFixed(1)} T${r.y.toFixed(1)} R${(SHIP_W - r.x - r.w).toFixed(1)} `
        + `B${(SHIP_H - r.y - r.h).toFixed(1)}`;

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
                `[2][${lang}] ${key}: hovering ${sel} SHOWS the tip `
                + `(opacity ${st ? st.opacity : 'n/a'})`);
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
                `[4][${lang}] ${key}: rect ${st.rect.w.toFixed(1)} x ${st.rect.h.toFixed(1)} is `
                + `fully inside ${SHIP_W} x ${SHIP_H} — edge clearances ${edges(st.rect)}`);
            check(Number(st.zIndex) > FERN_Z,
                `[NC-3][${lang}] ${key}: the surface paints ABOVE the fern overlay — `
                + `z-index ${st.zIndex} vs ${FERN_Z}`);
            check(st.pointerEvents === 'none',
                `[NC-3][${lang}] ${key}: the surface is pointer-events:none and cannot steal `
                + 'the hover keeping it open');
        }
        check(seen.length === TIP_BINDINGS.length,
            `[2][${lang}] every one of the ${TIP_BINDINGS.length} bound anchors was driven `
            + `— got ${seen.length}`);
        if (popoverOpen) {
            await page.keyboard.press('Escape');
            await page.waitForTimeout(150);
            // Escape returns focus to the gear (initializeSettingsPopover), and
            // that focus IS keyboard-driven, so the gear's tip legitimately
            // opens. Blur it so the next section starts from a clean surface.
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
    const backSt = await hoverAndRead('.knob[data-param="energy"]');
    check(backSt.visible && backSt.title === I18N['tip.energy'].en.t
          && backSt.body === I18N['tip.energy'].en.b,
        `[5] switching back to English restores the English tip byte-for-byte — `
        + `"${backSt.title}"`);
    await park();

    // ── NC-4. the drag guard ────────────────────────────────────────────────
    // Real mouse events, not a synthesised pointerdown: the knobs bind mousedown
    // and the guard binds pointerdown, and only a real device gesture fires both
    // in the order a user produces.
    console.log('\n-- NC-4: a knob drag must not open a neighbour\'s tip, and must release');
    const rowBox = async (param) => page.evaluate((p) => {
        const el = document.querySelector(`.knob[data-param="${p}"]`);
        const r = el.getBoundingClientRect();
        return { x: r.x + r.width / 2, y: r.y + r.height / 2 };
    }, param);
    const energyPt = await rowBox('energy');
    const texturePt = await rowBox('texture');
    await park();
    await page.mouse.move(energyPt.x, energyPt.y);
    await page.waitForTimeout(200);
    const beforeDrag = await readTip();
    check(beforeDrag.visible && beforeDrag.title === I18N['tip.energy'].en.t,
        '[NC-4] the energy row\'s tip is open before the drag begins — the control is live');
    await page.mouse.down();
    await page.mouse.move(texturePt.x, texturePt.y, { steps: 8 });
    await page.waitForTimeout(250);
    const midDrag = await readTip();
    check(!midDrag.visible,
        '[NC-4] dragging from the energy knob across the texture row opens NO tip — the '
        + 'pointerHeld guard holds'
        + (midDrag.visible ? ` (it showed "${midDrag.title}")` : ''));
    await page.mouse.up();
    await park();
    const afterDrag = await hoverAndRead('.knob[data-param="texture"]');
    check(afterDrag.visible && afterDrag.title === I18N['tip.texture'].en.t,
        '[NC-4] the guard RELEASES on pointerup — hovering works again after the drag '
        + '(a guard that never clears is a renderer that dies after the first click)');
    await park();

    // ── NC-2. the focus latch, both halves ──────────────────────────────────
    console.log('\n-- NC-2: the focus latch (a click must not pin a tip; a key must open one)');
    await page.mouse.move(1, 1);
    await page.waitForTimeout(150);
    // BLUR FIRST. Clicking an ALREADY-FOCUSED element fires no focusin at all,
    // so without this the assertion is green for a page with no latch whatsoever
    // — see the 2x2 table in this file's header, whose bottom row was measured
    // on this page with both the latch and this line removed.
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
    check(afterClick.popoverOpen,
        '[NC-2a] the click did open the settings popover — the control is live');
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
        // PAST the 0.12s fade, and `visible` reads computed visibility rather
        // than a hard opacity threshold: a probe sampling mid-transition reports
        // a false "never opens", and the obvious response to that failure is to
        // delete the latch (O-Comp, M1).
        await page.waitForTimeout(200);
        const r = await readTip();
        const on = await page.evaluate(
            () => (document.activeElement ? (document.activeElement.id
                   || document.activeElement.className || document.activeElement.tagName) : null));
        if (r && r.visible && (r.title || '').length) {
            kbHit = { press: i, title: r.title, on, rect: r.rect };
            break;
        }
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
    // SIZED AGAINST THIS FRAME, not by habit. 600px tall with a 8px rail top and
    // bottom leaves 584px of clamp room, and at 11px/1.4 inside a 260px cap that
    // is roughly 38 lines, so the plant is built to overrun it by a wide margin
    // and its RENDERED HEIGHT is printed below. A plant that fits is
    // indistinguishable from a gate that cannot see (O-Tremolo, M1).
    //
    // A per-run mkdtemp directory, never a bare filename at a shared temp root:
    // several executors run in this scratchpad at once and a bare i18n.orig.js
    // is not yours. Restored by COPY, never with `git checkout -- <file>`.
    console.log('\n-- NC-1: plant an over-long body and confirm assertion 4 reports the overflow');
    const nc = fs.mkdtempSync(path.join(os.tmpdir(), 'otf-tip-nc-'));
    const servedI18n = path.join(built.root, 'js', 'i18n.js');
    const backup = path.join(nc, 'i18n.served.orig.js');
    fs.copyFileSync(servedI18n, backup);
    try {
        const LONG = ('A deliberately over-long negative-control sentence, far too long to fit '
                    + 'inside the frame, repeated so that the surface overruns the bottom rail. ')
                    .repeat(22);
        const orig = fs.readFileSync(backup, 'utf8');
        const planted = orig.replace(
            /('tip\.energy':[\s\S]*?en:\s*\{\s*t:\s*'Energy',\s*\n\s*b:\s*)'[^']*'/,
            `$1'${LONG}'`);
        check(planted !== orig,
            "[NC-1] the plant actually edited tip.energy's en body — a no-op replace would make "
            + 'this control vacuous');
        fs.writeFileSync(servedI18n, planted);

        await page.goto(url, { waitUntil: 'networkidle' });
        await page.waitForTimeout(300);
        const bad = await hoverAndRead('.knob[data-param="energy"]');
        check(bad.visible,
            `[NC-1] the planted tip still renders — ${bad.rect.h.toFixed(1)}px tall against `
            + `${SHIP_H - 2 * MARGIN}px of clamp room, so it is big enough to defeat the clamp`);
        check(!inFrame(bad.rect),
            `[NC-1] assertion 4 REPORTS the overflow — rect ${bad.rect.w.toFixed(1)} x `
            + `${bad.rect.h.toFixed(1)} at ${bad.rect.x.toFixed(1)},${bad.rect.y.toFixed(1)} `
            + `leaves the ${SHIP_W} x ${SHIP_H} frame (${edges(bad.rect)}). If this PASSES as `
            + 'in-frame, assertion 4 is decoration and every green above means nothing');
    } finally {
        fs.copyFileSync(backup, servedI18n);
    }

    await page.goto(url, { waitUntil: 'networkidle' });
    await page.waitForTimeout(300);
    const restored = await hoverAndRead('.knob[data-param="energy"]');
    check(restored.visible && restored.body === I18N['tip.energy'].en.b && inFrame(restored.rect),
        '[NC-1] restored from the per-run copy — tip.energy is back inside the frame '
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
