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
         pattern_webview_native_fn_bridge_gap). Expected surface: 32 fns
         (2 mockup + 10 preset + 20 tuning).
      3. Knob readouts come from SliderState.getScaledValue() (real C++
         NormalisableRange incl. skew) and refresh on propertiesChanged; the
         FMT table carries no min/max range constants.
      4. Dblclick reset reads paramDefaults (getParameterDefaults native fn),
         never a bare JS constant.
      5. No unguarded window.confirm in the plugin's own frontend (dead in
         WKWebView — no UIDelegate wiring).
      6. Resource-provider closure — every non-juce local file the HTML
         references has a provider entry in getResource().

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
    check(called.size === 34 && registered.size === 34,
        `bridge surface is exactly 34 fns (2 mockup + 10 preset + 20 tuning + 2 hover-help) — got JS=${called.size} C++=${registered.size}`);
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
{
    const refs = new Set();
    for (const m of html.matchAll(/(?:src|href)="(\.\/|)((?:js|css)\/[^"]+)"/g)) refs.add('/' + m[2]);
    for (const m of inlineJs.matchAll(/import\(\s*["']\.\/((?:js|css)\/[^"']+)["']\s*\)/g)) refs.add('/' + m[1]);
    for (const m of inlineJs.matchAll(/import\s+\*\s+as\s+\w+\s+from\s+["']\.\/((?:js|css)\/[^"']+)["']/g)) refs.add('/' + m[1]);

    const provided = new Set();
    for (const m of editorCpp.matchAll(/url == "([^"]+)"/g)) provided.add(m[1]);

    const missing = [...refs].filter(r => !provided.has(r));
    check(missing.length === 0,
        `all ${refs.size} local UI file references have resource-provider entries`
        + (missing.length ? ' — MISSING: ' + missing.join(', ') : ''));
}

console.log(failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`);
process.exit(failed);
