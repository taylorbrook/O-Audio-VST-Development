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
      6. WR-04 — dblclick reset reads paramDefaults, not a bare 0.5.
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

// --------------------------------------- 6. WR-04: dblclick resets to default
{
    check(/paramDefaults\[relayId\]/.test(appJs),
        'WR-04: dblclick reset reads paramDefaults[relayId] (APVTS default), not bare 0.5');
    check(/pullParameterDefaults\(\)/.test(appJs),
        'WR-04: pullParameterDefaults() is invoked at boot');
}

// ------------------------------------------- 7. WR-05: wheel gesture wrapping
{
    const wheel = appJs.match(/addEventListener\('wheel'[\s\S]*?\{ passive: false \}\);/);
    check(!!wheel && /sliderDragStarted\(\)/.test(wheel[0]),
        'WR-05: wheel handler opens a sliderDragStarted gesture');
    check(!!wheel && /sliderDragEnded\(\)/.test(wheel[0]),
        'WR-05: wheel handler closes the gesture (idle timeout → sliderDragEnded)');
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
