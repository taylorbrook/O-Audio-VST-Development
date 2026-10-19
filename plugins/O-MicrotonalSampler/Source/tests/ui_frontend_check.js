/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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
    O-MicrotonalSampler — v1.23.7 WebView frontend regression checks
    (review WR-01..05 / IN-01..05, REVIEW-ui-frontend.md).

    Manual run:  node Source/tests/ui_frontend_check.js
    Exit code = number of failed assertions (0 = all pass).

    The frontend has no executable test harness (it runs inside the plugin's
    WKWebView), so this pins the v1.23.7 fixes statically:

      1. sampler-app.js parses (node --check) — a load-time SyntaxError kills
         the ENTIRE UI silently (build/auval/pluginval all stay green; see
         feedback_module_extraction_regression_check).
      2. JS↔C++ bridge closure — every native fn name the JS calls is
         registered in PluginEditor.cpp's buildNativeFunctionRegistry (an
         unregistered name fails silently at runtime; see
         pattern_webview_native_fn_bridge_gap). Pins the new
         getParameterDefaults registration (WR-04).
      3. WR-01 — KNOB_FORMATS carries no hard-coded min/max ranges (readouts
         come from SliderState.getScaledValue(), which applies the real C++
         NormalisableRange incl. skew), and every SLIDER_BINDINGS relayId has
         a KNOB_FORMATS entry.
      4. WR-02 — both technique/trigger subscribers guard `.backend`.
      5. WR-03/IN-01 — trigger tables + KS fields update in place through the
         activeElement-guarded writer; no innerHTML rebuild in the update path.
      6. WR-04 / quick 261019-gk9 — the knob reset is Alt/Option-click and
         reads paramDefaults (never a bare 0.5); knob and trim dblclick open
         attachValueEntry and never write a value.
      7. WR-05 — the wheel handler opens/closes a slider gesture.
      8. IN-05 — no window.confirm anywhere in sampler-app.js (dead in
         WKWebView: no UIDelegate wiring).

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const root = path.resolve(__dirname, '..', '..');
const appJsPath = path.join(root, 'Resources', 'ui', 'js', 'sampler-app.js');
const editorCppPath = path.join(root, 'Source', 'PluginEditor.cpp');

const appJs = fs.readFileSync(appJsPath, 'utf8');
const editorCpp = fs.readFileSync(editorCppPath, 'utf8');

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}

console.log('== ui_frontend_check ==');

// ---------------------------------------------------------------- 1. syntax
{
    const res = spawnSync(process.execPath, ['--check', appJsPath], { encoding: 'utf8' });
    check(res.status === 0,
        'sampler-app.js parses (node --check) — a SyntaxError silently kills the whole UI'
        + (res.status !== 0 ? `\n${res.stderr}` : ''));
}

// ------------------------------------------------- 2. native-fn bridge gaps
{
    const called = new Set();
    for (const m of appJs.matchAll(/getNativeFunction\(\s*'([A-Za-z0-9_]+)'/g)) called.add(m[1]);
    for (const m of appJs.matchAll(/invokeNative\(\s*'([A-Za-z0-9_]+)'/g)) called.add(m[1]);

    const registered = new Set();
    for (const m of editorCpp.matchAll(/\{\s*"([A-Za-z0-9_]+)"\s*,/g)) registered.add(m[1]);

    const missing = [...called].filter(n => !registered.has(n));
    check(missing.length === 0,
        `all ${called.size} native fns called from sampler-app.js are registered in `
        + `PluginEditor.cpp (bridge-gap guard)${missing.length ? ' — MISSING: ' + missing.join(', ') : ''}`);

    check(called.has('getParameterDefaults') && registered.has('getParameterDefaults'),
        'WR-04: getParameterDefaults is called by the JS AND registered in C++');
}

// -------------------------------------- 3. WR-01: no hard-coded knob ranges
{
    const kfMatch = appJs.match(/const KNOB_FORMATS = \{[\s\S]*?\n\};/);
    check(!!kfMatch, 'KNOB_FORMATS block found');
    if (kfMatch) {
        check(!/\bmin\s*:/.test(kfMatch[0]) && !/\bmax\s*:/.test(kfMatch[0]),
            'WR-01: KNOB_FORMATS has no min/max fields — readout ranges come from '
            + 'SliderState properties (C++ NormalisableRange), not JS constants');
    }
    check(/fmt\.format\(state\.getScaledValue\(\)\)/.test(appJs),
        'WR-01: knob readout is computed from state.getScaledValue() (applies real range + skew)');
    check(/propertiesChangedEvent\.addListener/.test(appJs),
        'WR-01: knob refreshes when the real range arrives via propertiesChanged');

    const bindings = new Set();
    for (const m of appJs.matchAll(/relayId:\s*'([A-Za-z0-9_]+)'/g)) bindings.add(m[1]);
    const kfKeys = new Set();
    if (kfMatch) for (const m of kfMatch[0].matchAll(/'([A-Za-z0-9_]+)':\s*\{/g)) kfKeys.add(m[1]);
    // dynamics_mode is a combo relay, not a knob — only knob relayIds need formats.
    const knobIds = [...bindings].filter(id => id !== 'dynamics_mode');
    const unformatted = knobIds.filter(id => !kfKeys.has(id));
    check(knobIds.length >= 9 && unformatted.length === 0,
        `every SLIDER_BINDINGS knob relayId (${knobIds.length}) has a KNOB_FORMATS entry`
        + (unformatted.length ? ' — MISSING: ' + unformatted.join(', ') : ''));
}

// ------------------------------------------------- 4. WR-02: .backend guards
{
    for (const fn of ['subscribeTechniqueStateUpdates', 'subscribeTriggerStateUpdates']) {
        const body = appJs.match(new RegExp(`function ${fn}\\(\\) \\{[\\s\\S]*?\\n\\}`));
        check(!!body && /!window\.__JUCE__ \|\| !window\.__JUCE__\.backend/.test(body[0]),
            `WR-02: ${fn} guards .backend before dereferencing it`);
    }
}

// ------------------------- 5. WR-03/IN-01: in-place updates, focus preserved
{
    check(/function setInputValueUnlessFocused\(/.test(appJs)
        && /function ensureTriggerRows\(/.test(appJs),
        'WR-03: reconciliation helpers (ensureTriggerRows / setInputValueUnlessFocused) exist');

    const rtp = appJs.match(/function renderTriggerPanel\(\) \{[\s\S]*?\n\}/);
    check(!!rtp && !/innerHTML\s*=/.test(rtp[0]),
        'WR-03: renderTriggerPanel no longer innerHTML-rebuilds (updates rows in place)');
    check(!!rtp && (rtp[0].match(/setInputValueUnlessFocused\(/g) || []).length >= 5,
        'WR-03: renderTriggerPanel writes CC/PC inputs through the activeElement-guarded setter');

    const rtb = appJs.match(/function renderTechniqueBar\(\) \{[\s\S]*?\n\}/);
    check(!!rtb && (rtb[0].match(/setInputValueUnlessFocused\(/g) || []).length >= 2,
        'IN-01: renderTechniqueBar KS low/high writes are activeElement-guarded');
}

// ------------- 6. WR-04 + quick 261019-gk9: Alt-click reset, dblclick entry
{
    // The handler body of each addEventListener(type, ...) call, up to the
    // paren that closes it.
    const handlers = (type) => {
        const out = [];
        const re = new RegExp(`addEventListener\\(\\s*'${type}'\\s*,`, 'g');
        let m;
        while ((m = re.exec(appJs))) {
            let i = re.lastIndex, depth = 1;
            for (; i < appJs.length; i++) {
                if (appJs[i] === '(') depth++;
                else if (appJs[i] === ')' && --depth === 0) break;
            }
            out.push(appJs.slice(re.lastIndex, i));
        }
        return out;
    };

    const altReset = handlers('pointerdown').find(b =>
        /e\.altKey/.test(b) && /paramDefaults\[relayId\]/.test(b));
    check(!!altReset,
        'WR-04/gk9: the knob Alt-click reset reads paramDefaults[relayId] (APVTS default)');
    check(!!altReset && !/:\s*0\.5\b/.test(altReset) && /typeof paramDefaults\[relayId\] !== 'number'\) return/.test(altReset),
        'WR-04/gk9: Alt-click is a no-op until paramDefaults lands — no bare 0.5 fallback');
    check(!!altReset && /sliderDragStarted\(\)[\s\S]*setNormalisedValue\([\s\S]*sliderDragEnded\(\)/.test(altReset),
        'gk9: Alt-click reset is bracketed sliderDragStarted -> setNormalisedValue -> sliderDragEnded');
    check(!!altReset && /stopImmediatePropagation\(\)/.test(altReset) && /\}, true\);/.test(appJs.slice(appJs.indexOf(altReset))),
        'gk9: Alt-click reset is a capture-phase pointerdown that stops the drag (stopImmediatePropagation)');

    const dbl = handlers('dblclick');
    const knobDbl = dbl.filter(b => /attachValueEntry\(valueEl, state, entryFormat\)/.test(b));
    check(knobDbl.length === 1,
        'gk9: the knob dblclick opens attachValueEntry (value entry)');
    const trimDbl = dbl.filter(b => /attachValueEntry\((techReadout|readout), (techAdapter|layerAdapter), /.test(b));
    check(trimDbl.length === 4,
        `gk9: technique + layer trim sliders and readouts open attachValueEntry over the adapter (${trimDbl.length}/4)`);
    const writers = [...knobDbl, ...trimDbl].filter(b =>
        /setNormalisedValue\(|invokeNative\(|\.value\s*=(?!=)|paramDefaults/.test(b));
    check(writers.length === 0,
        'gk9: no knob/trim dblclick handler writes a value or reads a default (the reset is Alt-click)');
    check(/function attachValueEntry\(/.test(appJs) && /function makeTrimAdapter|const makeTrimAdapter/.test(appJs),
        'gk9: attachValueEntry and the 5-member trim adapter are defined');

    check(/pullParameterDefaults\(\)/.test(appJs),
        'WR-04: pullParameterDefaults() is invoked at boot');
}

// ------------------------------------------- 7. WR-05: wheel gesture wrapping
{
    const wheel = appJs.match(/addEventListener\('wheel'[\s\S]*?\{ passive: false \}\);/);
    check(!!wheel && /sliderDragStarted\(\)/.test(wheel[0]),
        'WR-05: wheel handler opens a sliderDragStarted gesture');
    // The close runs through the idle timer's endWheelGesture(), which is the
    // one place that calls sliderDragEnded() for the wheel (v1.28.0 R7 moved
    // it out of the handler body, so the old in-body regex went stale).
    const endFn = appJs.match(/const endWheelGesture = \(\) => \{[\s\S]*?\n    \};/);
    check(!!wheel && (/sliderDragEnded\(\)/.test(wheel[0])
            || (/setTimeout\(endWheelGesture/.test(wheel[0]) && !!endFn && /sliderDragEnded\(\)/.test(endFn[0]))),
        'WR-05: wheel handler closes the gesture (idle timeout → endWheelGesture → sliderDragEnded)');
}

// ------------------------------------------ 8. IN-05: window.confirm is dead
{
    // Strip // line comments first — the docs deliberately mention
    // window.confirm() when explaining WHY it can't be used.
    const code = appJs.replace(/\/\/[^\n]*/g, '');
    check(!/window\.confirm\s*\(/.test(code),
        'IN-05: no window.confirm calls remain in sampler-app.js (dead in WKWebView)');
}

console.log(failed === 0
    ? '\nALL CHECKS PASSED'
    : `\n${failed} CHECK(S) FAILED`);
process.exit(failed);
