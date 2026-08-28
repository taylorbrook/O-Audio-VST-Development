/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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

    ui_frontend_check.js
    O-Contrabass — Stage 3 WebView frontend regression checks.
    Ported from O-MicrotonalSampler v1.23.7 (checks 1/2/6/8 generic; paths and
    regexes adapted to O-Contrabass's inline-module + Options-chain style).

    Manual run:  node plugins/O-Contrabass/tests/ui_frontend_check.js
    Exit code = number of failed assertions (0 = all pass).

    The frontend has no executable test harness (it runs inside the plugin's
    WKWebView), so this pins the silent-failure classes statically:

      1. The inline <script type="module"> parses (node --check) — a load-time
         SyntaxError kills the ENTIRE UI silently (build/auval/pluginval all
         stay green; see feedback_module_extraction_regression_check).
      2. JS↔C++ bridge closure — every native fn name the JS (inline script +
         canonical preset-manager.js + canonical tuning-panel.js) calls is
         registered via withNativeFunction in PluginEditor.cpp (an
         unregistered name fails silently at runtime; see
         pattern_webview_native_fn_bridge_gap). Expected surface: 36 fns
         (2 mockup + 10 preset + 20 tuning + 2 hover-help + 2 language).
      3. Knob readouts come from SliderState.getScaledValue() (real C++
         NormalisableRange incl. skew) and refresh on propertiesChanged; the
         FMT table carries no min/max range constants.
      4. Dblclick reset reads paramDefaults (getParameterDefaults native fn),
         never a bare JS constant.
      5. No unguarded window.confirm in the plugin's own frontend (dead in
         WKWebView — no UIDelegate wiring).
      6. Resource-provider closure — every non-juce local file the HTML
         references has a provider entry in getResource(). The specifier scan
         is SHAPE-AGNOSTIC (v1.8.0): see the note at the assertion.
      7. i18n closure (v1.8.0) — every data-i18n / data-i18n-aria key in the
         markup resolves in LABELS or I18N and carries BOTH languages, every
         tip anchor is bound in TIP_BINDINGS, and the hover-help toggle swaps
         its face through setLabel() with plain string keys.

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const path = require('path');
const os = require('os');
const { spawnSync } = require('child_process');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot = path.resolve(pluginRoot, '..', '..');

const htmlPath = path.join(pluginRoot, 'Source', 'ui', 'public', 'index.html');
const editorCppPath = path.join(pluginRoot, 'Source', 'PluginEditor.cpp');
const presetJsPath = path.join(repoRoot, 'modules', 'persistence', 'preset-manager', 'js', 'preset-manager.js');
const tuningJsPath = path.join(repoRoot, 'modules', 'tuning', 'scala-tuning-engine', 'js', 'tuning-panel.js');

const html = fs.readFileSync(htmlPath, 'utf8');
const editorCpp = fs.readFileSync(editorCppPath, 'utf8');
const presetJs = fs.readFileSync(presetJsPath, 'utf8');
const tuningJs = fs.readFileSync(tuningJsPath, 'utf8');

const inlineMatch = html.match(/<script type="module">([\s\S]*?)<\/script>/);
const inlineJs = inlineMatch ? inlineMatch[1] : '';

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}

console.log('== O-Contrabass ui_frontend_check ==');

// ---------------------------------------------------------------- 1. syntax
{
    check(!!inlineMatch, 'inline <script type="module"> block found in index.html');
    const tmp = path.join(os.tmpdir(), 'ocontrabass-inline-module.mjs');
    fs.writeFileSync(tmp, inlineJs);
    const res = spawnSync(process.execPath, ['--input-type=module', '--check'], {
        input: inlineJs, encoding: 'utf8'
    });
    check(res.status === 0,
        'inline module script parses (node --check) — a SyntaxError silently kills the whole UI'
        + (res.status !== 0 ? `\n${res.stderr}` : ''));
}

// ------------------------------------------------- 2. native-fn bridge gaps
{
    const called = new Set();
    const collect = (src) => {
        for (const m of src.matchAll(/getNativeFunction\(\s*["']([A-Za-z0-9_]+)["']/g))
            called.add(m[1]);
    };
    collect(inlineJs);
    collect(presetJs);
    collect(tuningJs);

    const registered = new Set();
    for (const m of editorCpp.matchAll(/withNativeFunction\(\s*(?:juce::Identifier\()?\s*"([A-Za-z0-9_]+)"/g))
        registered.add(m[1]);

    const missing = [...called].filter(n => !registered.has(n));
    const dead = [...registered].filter(n => !called.has(n));
    check(missing.length === 0,
        `all ${called.size} native fns called from JS are registered in PluginEditor.cpp`
        + (missing.length ? ' — MISSING: ' + missing.join(', ') : ''));
    check(dead.length === 0,
        'no dead C++ registrations (registered but never called from JS)'
        + (dead.length ? ' — DEAD: ' + dead.join(', ') : ''));
    // v1.8.0 adds the interface-language pair (getUiLanguage / setUiLanguage),
    // the same shape as the v1.7.0 hover-help pair beside it: 34 -> 36.
    check(called.size === 36 && registered.size === 36,
        `bridge surface is exactly 36 fns (2 mockup + 10 preset + 20 tuning + 2 hover-help `
        + `+ 2 language) — got JS=${called.size} C++=${registered.size}`);
    check(called.has('getParameterDefaults') && registered.has('getParameterDefaults'),
        'getParameterDefaults is called by the JS AND registered in C++');
}

// -------------------------------------- 3. readouts use getScaledValue()
{
    check(/state\.getScaledValue\(\)/.test(inlineJs),
        'knob readouts computed from state.getScaledValue() (real C++ range + skew)');
    const fmtMatch = inlineJs.match(/const FMT = \{[\s\S]*?\n\};/);
    check(!!fmtMatch, 'FMT display-format table found');
    if (fmtMatch) {
        check(!/\bmin\s*:/.test(fmtMatch[0]) && !/\bmax\s*:/.test(fmtMatch[0]),
            'FMT has no min/max fields — display formatting only, no JS range map');
    }
    check(/propertiesChangedEvent\.addListener/.test(inlineJs),
        'knobs refresh when the real range arrives via propertiesChanged');
}

// -------------------------------------- 4. dblclick reset uses paramDefaults
{
    check(/paramDefaults\[paramId\]/.test(inlineJs),
        'dblclick reset reads paramDefaults (from getParameterDefaults), not a JS constant');
    check(!/setNormalisedValue\(\s*0\.5\s*\)/.test(inlineJs),
        'no bare setNormalisedValue(0.5) reset shortcut');
}

// ---------------------------------------------------- 5. no window.confirm
{
    check(!/window\.confirm/.test(inlineJs),
        'no window.confirm in the plugin inline script (dead in WKWebView)');
}

// ------------------------------------- 6. resource-provider closure (local)
//
// THE SCAN IS SHAPE-AGNOSTIC, and that is the point (v1.8.0).
//
// Through v1.7.2 this enumerated THREE import forms: a `src=`/`href=` in
// double quotes, a dynamic `import("./…")`, and a NAMESPACE import
// (`import * as X from "./…"`). v1.8.0 adds a NAMED import —
// `import { LANGUAGES, … } from './js/i18n.js'` — which is a fourth shape and
// matched none of the three. The scan therefore counted 4 references where the
// page has 5, and reported PASS over a file with no getResource() branch: the
// exact silent-404 blank-UI failure this assertion exists to prevent, certified
// green. Same class as the four wrong-shaped gate assumptions found earlier in
// this rollout, and the single-quote half of it is the one the plan warned to
// look for here before touching anything.
//
// It now matches ANY module specifier — named, default, namespace,
// side-effect-only or dynamic — in either quote style, and asserts the derived
// set is non-empty and contains the new table, because a regex that silently
// stops matching makes every assertion below it pass by having nothing to check.
{
    const refs = new Set();
    for (const m of html.matchAll(/(?:src|href)\s*=\s*(["'])(?:\.\/)?((?:js|css)\/[^"']+)\1/g))
        refs.add('/' + m[2]);
    for (const m of inlineJs.matchAll(
            /\bimport\s*(?:\(\s*)?(?:[\w*{}\s,$]+?\s+from\s*)?(["'])\.\/((?:js|css)\/[^"']+)\1/g))
        refs.add('/' + m[2]);

    check(refs.size > 0, `the specifier scan found local UI references — ${refs.size}: `
        + [...refs].sort().join(', '));
    check(refs.has('/js/i18n.js'),
        'the specifier scan sees the NAMED import of js/i18n.js (the shape the v1.7.2 scan missed)');

    const provided = new Set();
    for (const m of editorCpp.matchAll(/url == "([^"]+)"/g)) provided.add(m[1]);

    const missing = [...refs].filter(r => !provided.has(r));
    check(missing.length === 0,
        `all ${refs.size} local UI file references have resource-provider entries`
        + (missing.length ? ' — MISSING: ' + missing.join(', ') : ''));
}

// ------------------------------------------------- 7. i18n closure (v1.8.0)
//
// scripts/check-i18n.js already asserts the CANON and the coverage. This
// section asserts the things only this plugin knows: that every key its markup
// names resolves in BOTH languages, that every tip anchor it authors is bound,
// and that the one caption written from script is written the in-canon way.
//
// The i18n.js module is loaded by stripping `export` and evaluating in a vm
// sandbox — the O-ReverseDelay v1.10.0 shape. A require() would fail on the ES
// module syntax and a static regex over the table would go quiet the moment the
// table is reformatted.
{
    const vm = require('vm');
    const i18nPath = path.join(pluginRoot, 'Source', 'ui', 'public', 'js', 'i18n.js');
    check(fs.existsSync(i18nPath), 'js/i18n.js exists (the interface copy table)');

    let I18N = null, LABELS = null, TIP_BINDINGS = null, I18N_EXEMPT = null;
    if (fs.existsSync(i18nPath)) {
        try {
            const src = fs.readFileSync(i18nPath, 'utf8')
                .replace(/(^|\n)(\s*)export\s+(const|let|function|class)\s/g, '$1$2$3 ');
            const sandbox = { console: { warn() {}, error() {}, log() {} } };
            vm.createContext(sandbox);
            vm.runInContext(`${src}\n;globalThis.__x = { I18N, LABELS, TIP_BINDINGS, I18N_EXEMPT };`,
                            sandbox, { timeout: 5000 });
            ({ I18N, LABELS, TIP_BINDINGS, I18N_EXEMPT } = sandbox.__x);
        } catch (e) {
            check(false, `js/i18n.js evaluates — ${e.message}`);
        }
    }

    if (I18N && LABELS && TIP_BINDINGS) {
        // ── every markup key resolves, in BOTH languages ────────────────
        // trLabel() looks in LABELS first and falls back to I18N; this mirrors
        // that order rather than assuming a key lives in one table.
        const markupKeys = [];
        for (const m of html.matchAll(/data-i18n(?:-aria|-placeholder|-alt)?="([\w.-]+)"/g))
            markupKeys.push(m[1]);
        check(markupKeys.length > 0,
            `the markup declares i18n keys — ${markupKeys.length} occurrence(s)`);

        const resolve = (k) => LABELS[k] || I18N[k];
        const bad = [];
        for (const k of new Set(markupKeys)) {
            const e = resolve(k);
            if (!e)                                   bad.push(`${k} (no such key)`);
            else if (!e.en || !e.en.t)                bad.push(`${k} (en missing)`);
            else if (!e.fr || !e.fr.t)                bad.push(`${k} (fr MISSING — an English caption on a French page)`);
        }
        check(bad.length === 0,
            `all ${new Set(markupKeys).size} distinct markup keys resolve in LABELS or I18N with both languages`
            + (bad.length ? ' — ' + bad.join(', ') : ''));

        // ── every tip anchor is bound ───────────────────────────────────
        // Derived from the markup, never a transcribed list: a list would make
        // this pass by not looking the moment an anchor is added. Every anchor
        // on this page carries either an id, a data-param or one of the five
        // authored selectors, and TIP_BINDINGS must address it.
        const bound = new Set(TIP_BINDINGS.map(b => b[1]));
        const unresolved = [...bound].filter(k => !I18N[k]);
        check(unresolved.length === 0,
            `every TIP_BINDINGS key exists in I18N`
            + (unresolved.length ? ' — ' + unresolved.join(', ') : ''));

        const missingFr = [...bound].filter(k => !I18N[k] || !I18N[k].fr
            || !I18N[k].fr.t || !I18N[k].fr.b);
        check(missingFr.length === 0,
            `every bound tooltip carries a French title AND body`
            + (missingFr.length ? ' — ' + missingFr.join(', ') : ''));

        // ── the selectors actually address something ────────────────────
        // Five anchors carry neither an id nor a data-param and are addressed
        // by an authored class selector. A selector that stops matching makes
        // applyI18n log a warning nobody reads and leaves the anchor with NO
        // tip at all, in both languages.
        const unmatched = [];
        for (const [sel] of TIP_BINDINGS) {
            let re = null;
            let m = /^#([\w-]+)$/.exec(sel);
            if (m) re = new RegExp(`id="${m[1]}"`);
            else if ((m = /^\.([\w-]+)$/.exec(sel))) re = new RegExp(`class="[^"]*\\b${m[1]}\\b`);
            else if ((m = /^\[([\w-]+)="([^"]+)"\]$/.exec(sel))) re = new RegExp(`${m[1]}="${m[2]}"`);
            if (re === null) { unmatched.push(`${sel} (unrecognised selector shape)`); continue; }
            if (!re.test(html)) unmatched.push(sel);
        }
        check(unmatched.length === 0,
            `all ${TIP_BINDINGS.length} TIP_BINDINGS selectors match an element authored in index.html`
            + (unmatched.length ? ' — UNMATCHED: ' + unmatched.join(', ') : ''));

        // ── the two script-written captions ─────────────────────────────
        // The hover-help toggle is the only two-state caption on this page. Its
        // faces must be setLabel() calls with PLAIN STRING keys, never a JS
        // literal and never a ternary — a literal ships English on a French
        // page, and a ternary is where inflection creeps in (contract §6).
        const setLabelKeys = [...inlineJs.matchAll(/setLabel\(\s*[\w.]+\s*,\s*(['"])([\w.]+)\1/g)]
            .map(m => m[2]);
        check(setLabelKeys.includes('ui.on') && setLabelKeys.includes('ui.off'),
            'the hover-help toggle swaps its face through setLabel() with plain string keys — '
            + `found [${setLabelKeys.join(', ')}], expected ui.on and ui.off`);
        check(!/toggleEl\.textContent\s*=/.test(inlineJs),
            "nothing writes the hover-help toggle's textContent directly — applyLabel() owns it");
        check(/id="help-toggle"[\s\S]{0,320}?data-i18n="ui\.off"/.test(html),
            'the toggle declares its unlit key in the markup (data-i18n="ui.off"), so the '
            + 'pre-applyI18n fallback is the right word');

        // ── the canon block sits ABOVE the eager bindings ───────────────
        // This module has no init(): every bind* call runs at module top level,
        // and bindActiveStrings() calls setLabel(). With the canon below them
        // `uiLanguage` is in its temporal dead zone and the call throws a
        // ReferenceError that takes the whole UI down
        // (pattern_module_toplevel_init_tdz — the Stage H trap on MBC and
        // O-Bitrot). Positional, because that is what the bug is.
        const canonAt = inlineJs.indexOf("let uiLanguage = 'en';");
        const bindAt  = inlineJs.indexOf('.forEach(bindKnob);');
        const setLabelDefAt = inlineJs.indexOf('function setLabel(');
        const setLabelUseAt = inlineJs.indexOf('setLabel(valueEl,');
        check(canonAt >= 0 && bindAt >= 0 && canonAt < bindAt,
            'the canon block is declared ABOVE the eager bind* calls '
            + `(canon @${canonAt}, first binding @${bindAt})`);
        check(setLabelDefAt >= 0 && setLabelUseAt >= 0 && setLabelDefAt < setLabelUseAt,
            'setLabel() is defined before bindActiveStrings() calls it '
            + `(def @${setLabelDefAt}, use @${setLabelUseAt})`);

        // ── D-03: readouts stay English ─────────────────────────────────
        // Everything left in FMT must be number + unit symbol. "of 4" was the
        // one connective in there and moved to readout.activeStrings in v1.8.0;
        // this stops another one being added without a key.
        const fmtBlock = (inlineJs.match(/const FMT = \{[\s\S]*?\n\};/) || [''])[0];
        const prose = [...fmtBlock.matchAll(/"\s*([A-Za-z]{2,})[^"]*"/g)]
            .map(m => m[1])
            .filter(w => !['Hz', 'kHz', 'dB', 'dBFS', 'ms', 'N'].includes(w));
        check(prose.length === 0,
            'no connective word remains in the FMT readout table (D-03 covers a number and a '
            + 'unit SYMBOL; a word needs a key)'
            + (prose.length ? ' — ' + prose.join(', ') : ''));
    }
}

console.log(failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`);
process.exit(failed);
