/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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
    O-Wind — hover-help RENDER verification at the shipping viewport (v1.18.0).

    THE FIRST RUNNABLE GATE IN THIS PLUGIN. plugins/O-Wind/tests/ held only
    i18n-states.json before this file, and there is no Source/tests/ here.

    WHY IT EXISTS. No gate in this repo can see a rendered tooltip. check-i18n
    reads the table statically and is satisfied by TIP_BINDINGS being non-empty
    and its keys resolving. check-ui-labels has no tooltip awareness whatsoever
    — it passed byte-for-byte identically before and after this whole feature
    landed, all three states, both languages. boot-all-uis counts aria-label and
    title and never data-tip. So a plugin can author 52 bodies, bind all 52, and
    pass all three green while showing nothing at all — which is exactly what
    O-Wind was one commit away from: at v1.17.0 canon v2's applyI18n() wrote
    data-tip-title and data-tip onto the anchors and stopped, and this page had
    no #tooltip element, no .tooltip rule and no hover handler to read them.

    NOT a port of the three committed ui_tooltip_clamp_check.js gates
    (O-Tapestop, O-Bitrot, O-ReverseDelay). Those are ~40 KB each and describe
    the OTHER renderer family — measure-then-pin placement with an above/below
    flip and an arrow. O-Wind ports O-simpleFM's delegated, cursor-following
    renderer, which has neither.

    WHAT IS ASSERTED, per anchor, in English and again in French:

      1. every TIP_BINDINGS selector resolves, its wrapper walk resolves, and
         the 52 of them land on 52 DISTINCT nodes. applyI18n() falls back
         `el.closest(w) || el`, so a BROKEN WRAPPER still opens a tip — on the
         wrong-sized cell — and assertions 2, 3 and 4 all pass over it in both
         languages (measured on O-Reed). [1] must therefore be a hard FAIL, and
         it checks the wrapper separately from the selector;
      2. hovering a DESCENDANT of the anchor makes the surface VISIBLE with
         non-empty text. A descendant rather than the anchor's own box: what the
         pointer lands on inside a 72 x 87.8 px .knob-control is the 55 px SVG,
         and hovering the column itself would prove the column carries an
         attribute and nothing about whether the delegated closest() walk
         reaches it from a child;
      3. the rendered title and body are BYTE-EQUAL to the table entry. Not
         "contains": a .tip-title that silently kept the previous anchor's text
         passes a contains check and fails this one;
      4. the tip rectangle is fully inside the 900 x 600 frame on all four
         edges, and no wider than the .tooltip max-width cap.

    THIS PAGE IS A TAB DECK AND TWO OF ITS STATES ARE NOT THE DEFAULT.
    21 of the 52 anchors live inside #tab-effects, a .tab-panel that is
    `display: none` until its tab is clicked, and 4 more (the ADSR knobs) sit
    inside #adsr-knobs, which carries `.adsr-disabled` — opacity 0.35 and
    `pointer-events: none` — whenever adsrEnabled is Off, which is the
    parameter's DEFAULT and therefore the state the stub seeds from
    params.tsv. Both are driven THROUGH THE PAGE'S OWN PATH: the tab through a
    real click on .tab-btn, the ADSR gate through the stub's own toggle state,
    whose valueChangedEvent runs updateDisabledState(). Stripping either class
    would measure a state the plugin only reaches by an action nobody took.
    The gated REST state is pinned by its own assertion, so the gating cannot
    silently disappear either.

    FIVE NEGATIVE CONTROLS, because "all pass" is worthless without them:

      NC-1  an over-long body is planted in the SERVED copy of js/i18n.js and
            the page reloaded; assertion 4 must REPORT the overflow. THE PLANT
            WAS SEARCHED FOR, NOT GUESSED. At this frame and this 260 px cap the
            boundary was measured at repeat 9 -> 10 of the unit sentence:
            9x = 1314 chars = 553.8 px tall and still IN frame; 10x = 1460 chars
            = 602.4 px and OUT. A habitual 40x would have worked here, but a
            plant that fits is indistinguishable from a gate that cannot see
            (O-Tremolo), so the height is ASSERTED below rather than assumed.
            Restored from a namespaced per-run copy, never `git checkout --`,
            which would take any uncommitted work in the same file with it.
      NC-2  the focus latch, BOTH halves, separately: a pointer click must leave
            no tip parked on screen, AND a real tab-ring walk must still open
            one. Asserting only the first lets the feature decay into "focus
            never shows a tip", which passes that assertion perfectly while
            deleting the keyboard half of hover-help.
            THE CLICK IS PRECEDED BY A BLUR. Clicking an already-focused element
            fires no focusin at all, so without the blur the assertion passes
            for a page with NO latch whatsoever — the orchestrator measured
            125/125 green that way on two M1 pilots, and O-Tremolo proved the
            converse at 186/186. Whether the blur is load-bearing HERE is a
            2x2 measurement, not an inheritance; the result is recorded in this
            plugin's commit message.
            A STATIC grep for `lastInputWasPointer` is NOT this control:
            deleting only `if (lastInputWasPointer) return;` leaves the
            declaration, the write and the clear, and every grep still matches.
            Confirmed on six plugins. The spelling check below is labelled a
            presence note for that reason.
      NC-3  the pointerout child-boundary rule: moving between two children of
            the SAME anchor must not flicker the tip off. OBSERVED AS CLASS
            MUTATIONS, because the two obvious instruments are both blind and
            both were measured to be: a post-hoc read passed 774/774 with the
            guard deleted, and so did a per-frame opacity sampler (minimum
            opacity 1 over 25 frames). pointerout and pointerover for one
            pointer move land in the SAME task, so the surface is hidden and
            reopened before the style system settles and before any frame
            renders. A MutationObserver records each attribute mutation
            individually and is the only instrument that sees the pair.
      NC-4  the mid-drag guard. Both knob families here begin a drag on
            mousedown and track document mousemove, and NEITHER calls
            setPointerCapture — checked, not assumed, because O-AnalogEQ needed
            no guard for exactly that reason. So a drag straying into a
            neighbouring cell would open that neighbour's tip over the control
            being turned. The guard must also RELEASE on pointerup, or it is a
            permanent off switch that passes.
      NC-5  the 8 px FLOOR, driven directly. M2 correction 1 established that
            the post-flip re-clamp's far rail is unreachable across eleven
            ports; the arithmetic in setupTooltips() shows it is unreachable
            here too, and a census over 47 driven anchors measured the flip
            firing 8x on x and 6x on y with the floor and the ceiling at ZERO.
            So the floor is asserted on a PLANTED tip big enough to reach it,
            rather than credited from a green sweep it never touched.
            (pattern_review_recomputes_instead_of_measuring, wearing a clamp.)

    Usage:  node plugins/O-Wind/tests/ui_tip_render_check.js
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

const PLUGIN   = 'O-Wind';
// Resources/ui, not Source/ui/public. Read from serve-ui's own resolver below
// rather than trusted from here, but named so the divergence is visible in the
// file that depends on it.
const UI_ROOT  = path.join(pluginRoot, 'Resources', 'ui');
const htmlPath = path.join(UI_ROOT, 'index.html');
const i18nPath = path.join(UI_ROOT, 'js', 'i18n.js');

// Mirrored from PluginEditor.cpp and the inline <style>, and CROSS-CHECKED
// against both below. A fixture that mirrors a constant without checking it
// starts describing the release before it and keeps passing
// (pattern_test_fixture_mirrors_drift_silently).
const SHIP_W = 900;
const SHIP_H = 600;
const MARGIN = 8;               // setupTooltips()'s clamp margin
const DOCUMENTED_MAX_W = 260;   // .tooltip max-width
const SOUND_KNOBS = 26;         // .knob-control cells createKnobSVG() builds into
const FX_KNOBS    = 16;         // .knob-container cells makeFxKnob() builds
const TIP_COUNT   = 53;         // 50 parameters with a control + 3 chrome (v1.19.0: + #tips-toggle)

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
// createTextNode. Reading tip.textContent alone concatenates the two and makes
// assertion 3 unable to tell a swapped title from a swapped body.
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

// The 21 anchors that live inside #tab-effects. Derived from the SELECTOR, not
// from a hand-written list, so a binding added to either tab classifies itself.
const isEffectsAnchor = (sel) => /Knob$|BypassBtn$|#delayModeSelect$/.test(sel);
// The 4 anchors inside #adsr-knobs, which is pointer-events:none at rest.
const isAdsrKnob = (sel) => /data-param="adsr(Attack|Decay|Sustain|Release)"/.test(sel);

(async () => {
    console.log(`== ${PLUGIN} ui_tip_render_check ==`);
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}\n`);

    const html    = fs.readFileSync(htmlPath, 'utf8');
    const i18nSrc = fs.readFileSync(i18nPath, 'utf8');

    // A STATIC SCAN THAT CANNOT TELL CODE FROM A COMMENT REPORTS ITS OWN
    // DOCUMENTATION, and this file caught itself doing it twice on the first
    // run: the `setPointerCapture` absence check matched the comment in
    // setupTooltips() that EXPLAINS the absence, and the native-title check
    // matched the Stage-K comment quoting the `valueDisplay.title = '...'`
    // line it had just deleted. Both were FAILs over text nobody ships.
    // check-i18n assertion 11 gets this right, which is why it passes on the
    // same file; this gate has to do the same thing rather than inherit the
    // verdict. Whole-line `//` comments and HTML comments are removed; a `//`
    // inside a URL or a string is left alone because it is never at the start
    // of a line here.
    const codeOnly = (src) => src
        .replace(/<!--[\s\S]*?-->/g, '')
        .split('\n').filter((l) => !/^\s*\/\//.test(l)).join('\n');
    const htmlCode = codeOnly(html);

    // ── 0. the mirrored constants, guarded ──────────────────────────────────
    const size = S.readEditorSize(PLUGIN);
    check(size && size.w === SHIP_W && size.h === SHIP_H,
        `[0] editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below `
        + `(PluginEditor.cpp says ${size ? `${size.w} x ${size.h}` : 'NOTHING'})`);

    // Parsed from the .tooltip RULE specifically. This page also carries pinned
    // widths on .settings-popover, .preset-save-btn, .toggle-label,
    // .instrument-selector label, .fx-title, .fx-bypass-btn and
    // #tab-effects .knob-container — eight of them — so a loose scan for the
    // first width would silently measure one of those instead.
    const tipRule  = html.match(/\.tooltip\s*\{[\s\S]*?\}/);
    const capMatch = tipRule && tipRule[0].match(/max-width:\s*(\d+(?:\.\d+)?)px/);
    const MAX_W    = capMatch ? parseFloat(capMatch[1]) : NaN;
    check(Number.isFinite(MAX_W) && MAX_W === DOCUMENTED_MAX_W,
        `[0] .tooltip max-width parsed from the inline <style> is the documented `
        + `${DOCUMENTED_MAX_W}px — got ${capMatch ? capMatch[1] + 'px' : 'NOTHING'}. French wraps `
        + `INSIDE this cap, so moving it changes every tip height and therefore every clamp `
        + `decision below`);
    check(new RegExp(`const M = ${MARGIN};`).test(htmlCode),
        `[0] setupTooltips()'s clamp margin is ${MARGIN} (this file mirrors it)`);
    check(/position:\s*fixed/.test(tipRule ? tipRule[0] : '')
       && /visibility:\s*hidden/.test(tipRule ? tipRule[0] : '')
       && /pointer-events:\s*none/.test(tipRule ? tipRule[0] : ''),
        '[0] .tooltip is position:fixed + visibility:hidden + pointer-events:none — the three '
        + 'properties that keep an unshown surface out of the label sweep and stop a shown one '
        + 'stealing its own hover');
    check(/id="tooltip"/.test(html), '[0] index.html carries the #tooltip surface');
    check(/lastInputWasPointer/.test(htmlCode),
        '[0] the focus latch is present in the shipped page — NOTE: this is a SPELLING check '
        + 'only. Deleting `if (lastInputWasPointer) return;` leaves the declaration, the write '
        + 'and the clear in place and this line still passes, confirmed on six plugins. NC-2 '
        + 'below is what proves it WORKS');
    check(/pointerHeld/.test(htmlCode),
        '[0] the drag guard is present — same caveat as the line above; NC-4 is the control');
    check(!/setPointerCapture/.test(htmlCode),
        '[0] this page does NOT use setPointerCapture, so the drag guard is REQUIRED rather '
        + 'than redundant. O-AnalogEQ needed none because its knobs retarget every boundary '
        + 'event for the duration of a drag; "no guard needed" is a measurement, not a default');
    check(!/\btitle\s*=\s*["']/.test(htmlCode.replace(/<title>[\s\S]*?<\/title>/, '')),
        '[0] no native title= attribute reintroduced by the renderer (contract §4)');
    check(!/tip\.(innerHTML|outerHTML)/.test(htmlCode),
        '[0] the renderer never touches innerHTML — localized copy must not reach a markup path');

    // ── the table ───────────────────────────────────────────────────────────
    const { I18N, TIP_BINDINGS, LANGUAGES } = loadTable(i18nSrc);
    check(Array.isArray(TIP_BINDINGS) && TIP_BINDINGS.length === TIP_COUNT,
        `[0] TIP_BINDINGS parsed from js/i18n.js — ${TIP_BINDINGS.length} anchor(s), expected `
        + `${TIP_COUNT} (50 parameters with a control + #gear-btn + #lang-select + #tips-toggle; the dump has `
        + `56 rows and six parameters have no control on this page)`);
    check(LANGUAGES.join(',') === 'en,fr', `[0] LANGUAGES is en,fr — got ${LANGUAGES.join(',')}`);
    check(TIP_BINDINGS.filter(([s]) => isEffectsAnchor(s)).length === 21,
        `[0] 21 anchors classify as Effects-tab (got `
        + `${TIP_BINDINGS.filter(([s]) => isEffectsAnchor(s)).length}) — the tab must be driven `
        + `for them, and this count is what makes the sweep below non-vacuous`);

    const pw = S.resolvePlaywright();
    if (pw == null) {
        console.log('\n  SKIP: playwright not resolvable. Install with');
        console.log('        npx playwright install chromium');
        console.log('  The hover-help render and its edge clamp are NOT verified without it.');
        process.exit(77);
    }

    const built = S.buildRoot(PLUGIN);
    check(built.uiRootLabel === 'Resources/ui',
        `[0] the served root really is Resources/ui (serve-ui resolved "${built.uiRootLabel}" `
        + `from ${built.uiRootFrom})`);

    const misses = [];
    const srv = await S.serve(built.root, (m) => misses.push(m));
    const browser = await pw.chromium.launch();
    // `viewport`, NOT `viewportSize` — the latter is the getter's name and is
    // silently IGNORED as a launch option, leaving Chromium's 1280x720 default.
    // At 1280 x 720 every anchor on this page has room below and to the right,
    // the flip never engages, and every assertion below would pass while the
    // real 900 x 600 frame overflowed
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
        `[0] the browser really is ${SHIP_W} x ${SHIP_H} — got ${vp.width} x ${vp.height}`);

    // Non-vacuity: the module must have RUN. A TDZ throw out of module
    // evaluation leaves the HTML looking correct and every control dead
    // (pattern_module_toplevel_init_tdz), and on this plugin the ENTIRE UI is
    // one inline module — 26 Sound knobs, the 16 script-built Effects knobs,
    // the preset browser and the i18n block would all go together. Both
    // witnesses are empty in the authored markup and written only by the module.
    const alive = await page.evaluate(() => ({
        soundSvgs: document.querySelectorAll('.knob-wrapper > svg').length,
        fxCells:   document.querySelectorAll('#tab-effects .knob-container').length,
        anchors:   document.querySelectorAll('[data-tip]').length,
    }));
    check(alive.soundSvgs === SOUND_KNOBS,
        `[0] the inline module ran — createKnobSVG built ${alive.soundSvgs} Sound-tab knob SVGs `
        + `into markup that authors none (expected ${SOUND_KNOBS})`);
    check(alive.fxCells === FX_KNOBS,
        `[0] initializeEffects() ran — makeFxKnob built ${alive.fxCells} Effects cells into an `
        + `empty .fx-knobs (expected ${FX_KNOBS}). initI18n() runs AFTER it, which is why all `
        + `21 Effects anchors resolve even though the tab is display:none`);
    check(alive.anchors === TIP_BINDINGS.length,
        `[0] applyI18n painted ${alive.anchors} [data-tip] anchors (expected `
        + `${TIP_BINDINGS.length}) — including the 21 on the hidden tab`);

    // ── the two gated states, pinned at REST first ──────────────────────────
    const rest = await page.evaluate(() => {
        const fx = document.getElementById('tab-effects');
        const ak = document.getElementById('adsr-knobs');
        const one = document.querySelector('.knob-control[data-param="adsrAttack"]');
        return {
            fxActive: fx.classList.contains('active'),
            fxW: fx.getBoundingClientRect().width,
            adsrDisabled: ak.classList.contains('adsr-disabled'),
            adsrPE: getComputedStyle(one).pointerEvents,
        };
    });
    check(!rest.fxActive && rest.fxW === 0,
        `[0-rest] #tab-effects is inactive and 0px wide at load — its 21 anchors are `
        + `UNHOVERABLE until the tab is clicked. Pinned so the tab gating cannot vanish`);
    check(rest.adsrDisabled && rest.adsrPE === 'none',
        `[0-rest] #adsr-knobs carries .adsr-disabled at load (pointer-events: ${rest.adsrPE}) — `
        + `adsrEnabled defaults to Off, so FOUR of the 52 anchors are mouse-unreachable in the `
        + `state the plugin ships in. Pinned, then driven through the page's own path below`);

    // Driven through the STUB'S OWN toggle state, which fires the page's real
    // valueChangedEvent -> updateDisabledState() path. Stripping .adsr-disabled
    // would test a state the plugin only reaches by a click nobody made
    // (M2 finding 5).
    const adsrDriven = await page.evaluate(() => {
        const st = window.__stubStates && window.__stubStates.toggles.get('adsrEnabled');
        if (!st) return { drove: false };
        st.setValue(true);
        const ak = document.getElementById('adsr-knobs');
        const one = document.querySelector('.knob-control[data-param="adsrAttack"]');
        return { drove: true, disabled: ak.classList.contains('adsr-disabled'),
                 pe: getComputedStyle(one).pointerEvents,
                 w: one.getBoundingClientRect().width };
    });
    check(adsrDriven.drove && !adsrDriven.disabled && adsrDriven.pe === 'auto'
          && adsrDriven.w > 0,
        `[0] adsrEnabled driven ON through the stub's own toggle state — #adsr-knobs is live `
        + `(pointer-events: ${adsrDriven.pe}, cell ${adsrDriven.w ? adsrDriven.w.toFixed(1) : 0}px `
        + `wide). Without this, four anchors could not be hovered at all and their whole rows `
        + `of assertions would be a false pass on elements nobody can reach`);

    // ── 1. every binding resolves, its WRAPPER resolves, distinct nodes ─────
    // The wrapper is checked SEPARATELY from the selector because applyI18n
    // falls back `el.closest(w) || el`: break a wrapper and the tip still opens,
    // on the wrong-sized cell, and assertions 2/3/4 all pass over it in both
    // languages. [1] is the only thing that can see it, so it is a hard FAIL.
    const resolution = await page.evaluate((bindings) => {
        const seen = [];
        return bindings.map(([sel, key, wrapper]) => {
            const el = document.querySelector(sel);
            if (!el) return { key, sel, ok: false, why: 'selector matched nothing' };
            const walked = wrapper ? el.closest(wrapper) : el;
            if (wrapper && !walked)
                return { key, sel, ok: false, why: `closest(${wrapper}) matched nothing — `
                         + `applyI18n would have fallen back to the bare element and the tip `
                         + `would still open, on the wrong cell` };
            const target = walked;
            const dup = seen.indexOf(target) >= 0;
            seen.push(target);
            const r = target.getBoundingClientRect();
            return {
                key, sel, ok: true, dup, wrapper: wrapper || null,
                tag: target.tagName.toLowerCase()
                     + (target.className ? '.' + String(target.className).split(' ')[0] : ''),
                area: Math.round(r.width * r.height),
                hasTip: target.hasAttribute('data-tip') && target.hasAttribute('data-tip-title'),
            };
        });
    }, TIP_BINDINGS);

    for (const r of resolution) {
        check(r.ok, `[1] ${r.key} resolves — ${r.sel}`
            + (r.ok ? ` -> ${r.tag}${r.wrapper ? ` via closest(${r.wrapper})` : ''}` : ` (${r.why})`));
        if (r.ok) check(r.hasTip,
            `[1] applyI18n wrote data-tip-title + data-tip onto ${r.key}'s anchor (${r.tag})`);
    }
    check(resolution.every((r) => r.ok && !r.dup),
        `[1] all ${TIP_BINDINGS.length} bindings land on DISTINCT nodes — two on one node means `
        + `the second silently overwrites the first while check-i18n reports both as bound`);
    check(tipWarns.length === 0,
        `[1] applyI18n logged no "tip target not found" warning`
        + (tipWarns.length ? ` — ${tipWarns.length}: ${tipWarns[0]}` : ''));

    // A minimum hover area, on the anchors that are measurable at rest. The
    // failure this guards is real and has three shapes across M2:
    // a 1x1 px hidden <input> that is a valid id, a pair of pointer-events:none
    // rings, and a 4 px SVG stroke. Anything under 400 px2 is not a hover target
    // a human can hold open.
    const tiny = resolution.filter((r) => r.ok && r.area > 0 && r.area < 400);
    check(tiny.length === 0,
        `[1] every measurable anchor is at least 400 px2 of hover area`
        + (tiny.length ? ` — ${tiny.map((t) => `${t.key} ${t.area}px2`).join(', ')}` : ''));

    // ── the hover driver ────────────────────────────────────────────────────
    // page.evaluate() given a STRING evaluates it as an EXPRESSION, so passing
    // READ_TIP directly returns the function OBJECT rather than calling it —
    // and an unserializable return arrives as undefined, which reads exactly
    // like "the tip was never there" and sails through a truthiness assertion
    // (measured on O-Bass). Invoked explicitly instead.
    const readTip = () => page.evaluate(`(${READ_TIP})()`);
    // What the pointer actually lands on. For a .knob-control it is the 55 px
    // SVG wrapper inside the 72 px column; for an Effects cell it is the 44 px
    // vine face inside the 66 px container.
    const hoverSelFor = (sel) => {
        if (sel.startsWith('.knob-control')) return `${sel} .knob-wrapper`;
        if (/Knob$/.test(sel)) return `${sel} .knob-visual`;
        return sel;
    };

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
        // so a fixed sleep either flakes or records "no tip" for a path that
        // demonstrably works — and the obvious response to THAT failure is to
        // delete the fix (O-Comp).
        await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 2000 })
                  .catch(() => {});
        return readTip();
    };

    const showTab = async (which) => {
        const active = await page.evaluate(() => {
            const p = document.querySelector('.tab-panel.active');
            return p ? p.id : null;
        });
        if (active === `tab-${which}`) return;
        await page.click(`.tab-btn[data-tab="${which}"]`);
        await page.waitForFunction(
            (w) => document.getElementById(`tab-${w}`).classList.contains('active'), which,
            { timeout: 2000 });
        await page.waitForTimeout(120);
    };

    const inFrame = (r) =>
        r.x >= 0 && r.y >= 0 && r.x + r.w <= SHIP_W && r.y + r.h <= SHIP_H;
    const edges = (r) =>
        `L${r.x.toFixed(1)} T${r.y.toFixed(1)} R${(SHIP_W - r.x - r.w).toFixed(1)} `
        + `B${(SHIP_H - r.y - r.h).toFixed(1)}`;

    // #lang-select lives inside a popover that ships hidden, so it is not
    // hoverable until the gear is clicked. Everything else is reachable once
    // its tab is active and the ADSR gate is open.
    const NEEDS_POPOVER = new Set(['#lang-select']);

    const placement = { flippedLeft: 0, flippedUp: 0, onRail: 0, total: 0,
                        minBottom: Infinity, minRight: Infinity };

    const sweep = async (lang) => {
        console.log(`\n-- ${lang.toUpperCase()}: hover every anchor, byte-compare, measure the rect`);
        let popoverOpen = false;
        const seen = [];
        for (const [sel, key] of TIP_BINDINGS) {
            const entry = (I18N[key] || {})[lang];
            if (!entry) { check(false, `[2] ${key} has an ${lang} entry`); continue; }

            await showTab(isEffectsAnchor(sel) ? 'effects' : 'sound');

            if (NEEDS_POPOVER.has(sel) && !popoverOpen) {
                await page.click('#gear-btn', { force: true });
                await page.waitForSelector('#settings-popover:not([hidden])', { timeout: 2000 });
                popoverOpen = true;
            }

            const hs = hoverSelFor(sel);
            const st = await hoverAndRead(hs);
            seen.push(key);

            check(st !== null && st.visible,
                `[2][${lang}] ${key}: hovering ${hs} SHOWS the tip `
                + `(opacity ${st ? st.opacity : 'n/a'})`);
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
                `[4][${lang}] ${key}: rect ${st.rect.w.toFixed(1)} x ${st.rect.h.toFixed(1)} is `
                + `fully inside ${SHIP_W} x ${SHIP_H} — edge clearances ${edges(st.rect)}`);

            // Placement census, reported rather than asserted: how often the
            // flip and the rails actually engage on this frame. A renderer that
            // never flips on 900 x 600 would make the whole second pass dead
            // code, and a number here says whether it is exercised.
            const box = await page.evaluate((s) => {
                const e = document.querySelector(s);
                const r = e.getBoundingClientRect();
                return { cx: r.left + r.width / 2, cy: r.top + r.height / 2 };
            }, hs);
            ++placement.total;
            if (st.rect.x < box.cx) ++placement.flippedLeft;
            if (st.rect.y < box.cy) ++placement.flippedUp;
            if (Math.abs(st.rect.x - MARGIN) < 0.5
             || Math.abs(st.rect.y - MARGIN) < 0.5
             || Math.abs(SHIP_W - st.rect.x - st.rect.w - MARGIN) < 0.5
             || Math.abs(SHIP_H - st.rect.y - st.rect.h - MARGIN) < 0.5) ++placement.onRail;
            placement.minBottom = Math.min(placement.minBottom, SHIP_H - st.rect.y - st.rect.h);
            placement.minRight  = Math.min(placement.minRight,  SHIP_W - st.rect.x - st.rect.w);
        }
        check(seen.length === TIP_BINDINGS.length,
            `[2][${lang}] every one of the ${TIP_BINDINGS.length} bound anchors was driven `
            + `— got ${seen.length}`);
        if (popoverOpen) {
            await page.keyboard.press('Escape');       // also clears the focus latch
            await page.waitForTimeout(150);
        }
        return seen;
    };

    await sweep('en');
    await showTab('sound');

    // ── the surface must not eat its own hover ──────────────────────────────
    // pointer-events: none is declared in the rule and asserted statically
    // above; this is the behavioural half. With a tip open, the point at its
    // own centre must hit-test to something that is NOT the tooltip, or the
    // pointer bounces between anchor and surface and the tip flickers.
    // A rect-comparing gate is BLIND to paint order
    // (pattern_rect_gate_cannot_see_paint_order), so this is elementFromPoint.
    const openSt = await hoverAndRead('.knob-control[data-param="material"] .knob-wrapper');
    const hit = await page.evaluate(() => {
        const t = document.getElementById('tooltip');
        const r = t.getBoundingClientRect();
        const e = document.elementFromPoint(r.left + r.width / 2, r.top + r.height / 2);
        return e ? (e.id ? '#' + e.id : e.tagName.toLowerCase()
                    + (e.className ? '.' + String(e.className).split(' ')[0] : '')) : null;
    });
    check(openSt.visible && hit !== '#tooltip',
        `[4] the open surface does not hit-test to itself — elementFromPoint at its centre `
        + `returns "${hit}", so pointer-events:none is in force`);
    await park();

    // ── NC-3. the child-boundary rule ───────────────────────────────────────
    // pointerout fires at every internal boundary. .knob-control holds THREE
    // children (the SVG wrapper, the caption and the readout), so without the
    // anchorOf(relatedTarget) === active guard the tip is hidden and then
    // immediately reopened as the pointer crosses from the SVG to the caption
    // inside the same cell — a visible flicker.
    //
    // THE OBVIOUS VERSION OF THIS CONTROL CANNOT FAIL, AND THIS FILE SHIPPED IT
    // FIRST. "Hover the wrapper, hover the caption, read the tip" passed
    // 774/774 with the guard DELETED, because hide() is followed within the
    // same gesture by a fresh pointerover on the caption — active is null by
    // then, so the tip REOPENS, and a read 200 ms later sees a perfectly normal
    // open tip. The dip is real and lasts about a frame; a post-hoc read is
    // structurally blind to it. Same family as M2 finding 3: an assertion that
    // looks like it covers something it cannot see.
    //
    // So the surface is SAMPLED every animation frame across the transit and
    // the assertion is on the MINIMUM opacity observed. With the guard the
    // minimum is 1; without it the .show class is removed and the 0.12 s fade
    // starts, which a 60 Hz sampler cannot miss.
    console.log('\n-- NC-3: moving between two children of the SAME anchor must not hide the tip');
    const tcSel = '.knob-control[data-param="toneColor"]';
    const beforeMove = await hoverAndRead(`${tcSel} .knob-wrapper`);
    // A MutationObserver on the surface's class attribute, NOT a frame sampler
    // and NOT a post-hoc read. Both of those were tried and BOTH passed with
    // the guard deleted, for the same reason: a single pointer move dispatches
    // pointerout and pointerover in ONE task, so the `show` class is removed
    // and re-added before the style system ever settles. No frame observes the
    // dip and no later read can. A MutationObserver records EVERY attribute
    // mutation individually, including an off/on pair inside one task, which is
    // exactly the thing the guard controls.
    await page.evaluate(() => {
        const t = document.getElementById('tooltip');
        window.__classChanges = [];
        window.__mo = new MutationObserver((recs) => {
            for (const r of recs)
                if (r.attributeName === 'class')
                    window.__classChanges.push(t.classList.contains('show'));
        });
        window.__mo.observe(t, { attributes: true, attributeFilter: ['class'] });
    });
    // Stepped, so the pointer also passes through the 4px flex gap between the
    // wrapper and the caption — a position whose target is .knob-control
    // ITSELF, which is the anchor. That is the transit the guard exists for.
    const wrapBox = await page.locator(`${tcSel} .knob-wrapper`).boundingBox();
    const lblBox  = await page.locator(`${tcSel} .knob-label`).boundingBox();
    await page.mouse.move(wrapBox.x + wrapBox.width / 2, wrapBox.y + wrapBox.height / 2,
                          { steps: 4 });
    await page.mouse.move(lblBox.x + lblBox.width / 2, lblBox.y + lblBox.height / 2,
                          { steps: 12 });
    await page.waitForTimeout(200);
    const transit = await page.evaluate(() => {
        window.__mo.disconnect();
        return { changes: window.__classChanges.slice() };
    });
    const afterMove = await readTip();
    const hidden = transit.changes.filter((on) => on === false).length;
    check(hidden === 0,
        `[NC-3] the \`show\` class was never REMOVED while the pointer crossed `
        + `.knob-wrapper -> the 4px gap -> .knob-label inside one .knob-control `
        + `— ${transit.changes.length} class mutation(s), ${hidden} of them a hide. `
        + `A post-hoc read and a per-frame opacity sampler are both BLIND to this: `
        + `pointerout and pointerover land in one task, so the tip is hidden and `
        + `reopened before any frame renders. Both were tried and both passed with `
        + `the guard deleted`);
    check(beforeMove.visible && afterMove.visible
          && afterMove.title === beforeMove.title && afterMove.title === I18N['tip.toneColor'].en.t,
        `[NC-3] and it is still the same tip afterwards — "${afterMove.title}"`);
    await park();

    // ── NC-4. the mid-drag guard ────────────────────────────────────────────
    // bindSliderParam() begins a drag on mousedown and tracks it on document
    // mousemove, and there is no setPointerCapture anywhere on this page
    // (asserted at [0]). Without `pointerHeld`, pointerover opens the
    // neighbouring cell's tip on top of the gesture.
    console.log('\n-- NC-4: a drag crossing into a neighbouring knob must not open its tip');
    const mtSel = '.knob-control[data-param="material"] .knob-wrapper';
    const acSel = '.knob-control[data-param="airColumn"] .knob-wrapper';
    const mtBox = await page.locator(mtSel).boundingBox();
    const acBox = await page.locator(acSel).boundingBox();
    await page.mouse.move(mtBox.x + mtBox.width / 2, mtBox.y + mtBox.height / 2);
    await page.mouse.down();
    await page.mouse.move(acBox.x + acBox.width / 2, acBox.y + acBox.height / 2, { steps: 8 });
    await page.waitForTimeout(250);
    const midDrag = await readTip();
    await page.mouse.up();
    check(!midDrag.visible,
        `[NC-4] no tip opened while the button was held across the neighbouring cell `
        + (midDrag.visible ? `(got "${midDrag.title}")` : '(surface stayed hidden)'));
    // And the guard must RELEASE: a hover after mouseup opens normally again.
    const afterDrag = await hoverAndRead(acSel);
    check(afterDrag.visible && afterDrag.title === I18N['tip.airColumn'].en.t,
        `[NC-4] the guard releases on mouseup — hovering the same cell now opens `
        + `"${afterDrag.title}". Without this half the guard could be a permanent off switch `
        + `that passes the line above`);
    await park();

    // ── 5. French, then back ────────────────────────────────────────────────
    // French runs 15-20% longer, wraps to more lines against the max-width cap
    // and grows the tip's HEIGHT, so a tip that fits in English can overflow the
    // bottom of the frame in French. That is why the whole sweep repeats rather
    // than spot-checking one anchor.
    await page.evaluate((l) => window.__setLanguage(l), 'fr');
    await page.waitForTimeout(150);
    const frLang = await page.evaluate(() => document.getElementById('lang-select').value);
    check(frLang === 'fr', `[5] window.__setLanguage('fr') took — selector reads "${frLang}"`);
    await sweep('fr');

    await showTab('sound');
    await page.evaluate((l) => window.__setLanguage(l), 'en');
    await page.waitForTimeout(150);
    const backSt = await hoverAndRead('.knob-control[data-param="growl"] .knob-wrapper');
    check(backSt.visible && backSt.title === I18N['tip.growl'].en.t
          && backSt.body === I18N['tip.growl'].en.b,
        `[5] switching back to English restores the English tip byte-for-byte `
        + `— "${backSt.title}"`);
    await park();

    console.log(`\n   placement census over ${placement.total} hovers (both languages): `
        + `${placement.flippedLeft} placed left of the cursor, ${placement.flippedUp} above it, `
        + `${placement.onRail} landed on an ${MARGIN}px rail; tightest clearances `
        + `bottom ${placement.minBottom.toFixed(1)}px, right ${placement.minRight.toFixed(1)}px`);

    // ── NC-2. the focus latch, both halves ──────────────────────────────────
    console.log('\n-- NC-2: the focus latch (a click must not pin a tip; a key must still open one)');
    await page.mouse.move(1, 1);
    await page.waitForTimeout(150);
    // BLUR FIRST, and this line is the whole reason the assertion below can FAIL
    // at all. The sweep above leaves focus on #gear-btn (the popover pass
    // clicked it), and clicking an ALREADY-FOCUSED element fires no focusin —
    // so without this the check reports "no tip after a click" for a page with
    // no latch whatsoever.
    await page.evaluate(() => { if (document.activeElement) document.activeElement.blur(); });
    await page.waitForTimeout(100);
    await page.click('#gear-btn');
    await page.waitForTimeout(300);
    const afterClick = await page.evaluate(() => {
        const t  = document.getElementById('tooltip');
        const cs = getComputedStyle(t);
        const r  = t.getBoundingClientRect();
        const p  = document.getElementById('settings-popover');
        const pr = p && !p.hidden ? p.getBoundingClientRect() : null;
        const shown = cs.visibility !== 'hidden' && parseFloat(cs.opacity) > 0.01;
        let overlap = 0;
        if (shown && pr) {
            const ox = Math.max(0, Math.min(r.right, pr.right) - Math.max(r.left, pr.left));
            const oy = Math.max(0, Math.min(r.bottom, pr.bottom) - Math.max(r.top, pr.top));
            overlap = Math.round(ox * oy);
        }
        return {
            shown, overlap,
            tip: `[${Math.round(r.x)},${Math.round(r.y)},${Math.round(r.width)}x${Math.round(r.height)}]`,
            pop: pr ? `[${Math.round(pr.x)},${Math.round(pr.y)},${Math.round(pr.width)}x${Math.round(pr.height)}]` : 'closed',
        };
    });
    check(!afterClick.shown,
        `[NC-2a] a POINTER click on #gear-btn leaves NO tip parked on screen — the latch `
        + `suppresses the focusin arm`
        + (afterClick.shown
            ? ` (tip ${afterClick.tip} covers the popover ${afterClick.pop} by `
              + `${afterClick.overlap} px2)`
            : ` (popover ${afterClick.pop} is unobscured)`));

    // The keyboard half. A real tab-ring walk, not a programmatic .focus():
    // Chromium reports :focus-visible false for a .focus() that follows a click,
    // and .focus() on an already-focused element fires no event at all — either
    // one would report "no tip" and record that as correct.
    await page.keyboard.press('Escape');
    await page.waitForTimeout(150);
    await page.evaluate(() => { if (document.activeElement) document.activeElement.blur(); });
    await park();
    let kbHit = null;
    for (let i = 1; i <= 20; ++i) {
        await page.keyboard.press('Tab');
        // AWAITED, not slept past. The surface fades in over 0.12s and `visible`
        // demands opacity > 0.99, so a fixed 80 ms sleep reads mid-fade and
        // records "no tip" on a tab that DID open one. O-Comp reported exactly
        // that artefact, and the obvious response to it is to delete the latch.
        await page.waitForFunction(`(${READ_TIP})().visible`, null, { timeout: 400 })
                  .catch(() => {});
        const r = await page.evaluate(`(${READ_TIP})()`);
        const on = await page.evaluate(() => {
            const a = document.activeElement;
            return a ? (a.id || a.className || a.tagName.toLowerCase()) : null;
        });
        if (r && r.visible && r.title) { kbHit = { press: i, on, ...r }; break; }
    }
    check(kbHit !== null,
        `[NC-2b] a KEYBOARD tab still OPENS a tip — the accessibility half the latch must not `
        + `kill` + (kbHit ? ` (tab #${kbHit.press} on "${kbHit.on}", title "${kbHit.title}")`
                          : ' — none in 20 tabs'));
    check(kbHit !== null && inFrame(kbHit.rect),
        `[NC-2b] the focus-placed tip is inside the frame`
        + (kbHit ? ` — ${edges(kbHit.rect)}` : ''));
    await page.keyboard.press('Escape');
    await park();

    // ── NC-1 + NC-5. plant an over-long body ────────────────────────────────
    // A namespaced per-run directory, never a bare filename at a shared temp
    // root: several executors run in this scratchpad at once and a bare
    // i18n.orig.js is not yours. Restored by COPY, never with
    // `git checkout -- <file>`, which takes the uncommitted fix with it.
    console.log('\n-- NC-1 / NC-5: plant an over-long body; assertion 4 must report the overflow');
    const nc = fs.mkdtempSync(path.join(os.tmpdir(), 'ownd-tip-nc-'));
    const servedI18n = path.join(built.root, 'js', 'i18n.js');
    const backup = path.join(nc, 'i18n.served.orig.js');
    fs.copyFileSync(servedI18n, backup);
    const origSrc = fs.readFileSync(backup, 'utf8');
    try {
        // THE SIZE WAS SEARCHED FOR, NOT GUESSED. Injecting the unit sentence at
        // repeat 4..12 into a live anchor at this frame measured, in px of tip
        // height: 4x 262.5 in, 5x 327.2 in, 6x 375.8 in, 7x 440.5 in (and the
        // FLIP first engages here, moving the tip from y 196.5 to the 8 px
        // rail), 8x 489.1 in, 9x 553.8 in, 10x 602.4 OUT. The boundary is
        // 9 -> 10 against the 584 px the frame can hold. 20x is a comfortable
        // multiple of it and its height is ASSERTED below rather than assumed:
        // a plant that FITS is indistinguishable from a gate that cannot see
        // (O-Tremolo's 40x fit inside a 400 px frame and reported nothing).
        //
        // No apostrophe anywhere in the plant: the bodies in i18n.js are
        // SINGLE-quoted, so one would close the string and break the module
        // rather than overflow the frame — a plant that fails for the wrong
        // reason is a control that proves nothing.
        const UNIT = 'Une phrase de controle negatif, deliberement beaucoup trop longue pour '
                   + 'tenir dans le cadre, repetee afin de faire deborder la surface par le bas. ';
        const LONG = UNIT.repeat(20);
        check(LONG.length > 1460,
            `[NC-1] the plant is ${LONG.length} chars — above the 1460 measured as this frame's `
            + `overflow boundary at the ${MAX_W}px cap. A plant that FITS is indistinguishable `
            + `from a gate that cannot see`);
        const planted = origSrc.replace(
            /(en: \{ t: 'Tone Color',\n)[\s\S]*?(\n        fr: \{ t: 'Timbre')/,
            `$1              b: '${LONG}' },$2`);
        check(planted !== origSrc,
            `[NC-1] the plant actually edited tip.toneColor's en body — a no-op replace would `
            + `make this control vacuous`);
        fs.writeFileSync(servedI18n, planted);

        await page.goto(url, { waitUntil: 'networkidle' });
        const bad = await hoverAndRead(`${tcSel} .knob-wrapper`);
        check(bad.visible, `[NC-1] the planted tip still renders (${bad.rect.h.toFixed(1)}px tall)`);
        check(bad.rect.h > SHIP_H - 2 * MARGIN,
            `[NC-1] the planted surface is ${bad.rect.h.toFixed(1)}px tall, taller than the `
            + `${SHIP_H - 2 * MARGIN}px the frame can hold — the plant is big enough to defeat `
            + `the clamp, which is what makes the next line a control`);
        check(!inFrame(bad.rect),
            `[NC-1] assertion 4 REPORTS the overflow — rect ${bad.rect.w.toFixed(1)} x `
            + `${bad.rect.h.toFixed(1)} at ${bad.rect.x.toFixed(1)},${bad.rect.y.toFixed(1)} `
            + `leaves the ${SHIP_W} x ${SHIP_H} frame (${edges(bad.rect)}). If this PASSES as `
            + `in-frame, assertion 4 is decoration and every green above means nothing`);

        // NC-5. THE FLOOR, DRIVEN DIRECTLY. This planted tip is taller than the
        // frame, so the flip fires and produces a large NEGATIVE y; the only
        // thing that puts it back on the page is `Math.max(M, ny)`. Asserting
        // the top edge is exactly the 8 px rail is the assertion M2 correction 1
        // asks for — the far rail is unreachable by construction and must not be
        // credited with a placement it never made.
        // The Y axis only, and that is the honest scope. This anchor sits at
        // x 394 in a 900 px frame with a 260 px tip, so the x preferred side
        // never overflows and x is never flipped OR clamped — it reports 408.1,
        // the plain `cursor + 14`. Asserting both axes here would have been an
        // assertion that passes for the wrong reason on one of them, which is
        // the failure mode this whole file is arguing against. Measured on the
        // first run and narrowed rather than widened.
        check(Math.abs(bad.rect.y - MARGIN) < 0.5,
            `[NC-5] the FLOOR is what lands the flipped tip: top ${bad.rect.y.toFixed(1)} is the `
            + `${MARGIN}px rail. Without Math.max(M, ...) this tip's flipped y is about `
            + `${Math.round(bad.rect.y - bad.rect.h - 28)}, i.e. ${Math.round(bad.rect.h)}px off `
            + `the top of the page. x is ${bad.rect.x.toFixed(1)} — unflipped and unclamped, `
            + `because this anchor has 490px of room to its right`);
    } finally {
        fs.copyFileSync(backup, servedI18n);
    }

    await page.goto(url, { waitUntil: 'networkidle' });
    const restored = await hoverAndRead(`${tcSel} .knob-wrapper`);
    check(fs.readFileSync(servedI18n, 'utf8') === origSrc,
        `[NC-1] the served copy is byte-identical to the pre-plant snapshot again`);
    check(restored.visible && restored.body === I18N['tip.toneColor'].en.b
          && inFrame(restored.rect),
        `[NC-1] restored — tip.toneColor is back inside the frame (${edges(restored.rect)})`);
    fs.rmSync(nc, { recursive: true, force: true });

    // ── housekeeping ────────────────────────────────────────────────────────
    console.log('');
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
