/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
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
    O-ReverseDelay — WebView frontend regression checks (Stage 3 + Stage 4).
    Ported from plugins/O-Contrabass/tests/ui_frontend_check.js; adapted for an
    EXTERNAL js/app.js module (not an inline <script>).

    Stage 4 REPAIRED sections 3 and 9, both of which FAILED on correct code once
    the preset bar landed: §3 scanned app.js alone and hard-asserted a bridge
    surface of 1 (it is now spread across app.js + the shared preset-manager.js,
    and is 11), and §9's embedded-set regex matched only Source/ui/public paths
    while the new binary-data entry is a modules/… path reached by a DYNAMIC
    import. Sections 12–14 are the new Stage-4 coverage.

    Manual run:  node plugins/O-ReverseDelay/tests/ui_frontend_check.js
    Exit code = number of failed assertions (0 = all pass).

    The frontend has no executable test harness (it runs inside the plugin's
    WKWebView), so this pins the silent-failure classes statically — every one
    of these ships green through ninja, auval AND pluginval:

      1. app.js parses (node --check); a SyntaxError kills the ENTIRE UI.
      2. TDZ discipline — a single init() call as the LAST statement, so no
         top-level initializer can reach a not-yet-initialised binding
         (pattern_module_toplevel_init_tdz).
      3. JS↔C++ bridge closure — every getNativeFunction name (across BOTH
         app.js and preset-manager.js) is registered with withNativeFunction,
         and vice versa (pattern_webview_native_fn_bridge_gap). Expected
         surface: exactly 11. Also the MSVC hoisted-SafePointer and
         bare-return-on-null contracts for the two dialog fns.
      4. Readouts come from SliderState.getScaledValue() and refresh on
         propertiesChanged; the FORMAT table carries units only, no range
         constants (pattern_webview_knob_readout_scaled_value).
      5. Dblclick reset reads paramDefaults, never a JS default table.
      6. The HTML-authored FREE/SYNC labels are never written via textContent
         (pattern_js_state_updater_overwrites_html_labels).
      7. The `Juce` ES-module namespace is used, not window.__JUCE__
         (critical_juce_webview_namespace_vs_postmessage).
      8. Resource provider never hard-codes a scheme and matches bare paths
         (critical_webview_resource_provider_and_schemes).
      9. Three-way closure: HTML/JS references == getResource() entries ==
         the juce_add_binary_data SOURCES list.
     10. Both Windows WebView2 CMake flags are set
         (critical_webview2_static_linking).
     11. Editor member order relays -> webView -> attachments, and the render
         harness never compiles PluginEditor.cpp
         (pattern_render_harness_breaks_on_webview_editor).
     12. Geometry is 940 x 484 in the editor AND both CSS spots, and the preset
         band occupies exactly the 44 px the frame grew by.
     13. Preset-bar IDs exist; the delete copy lives in data-attrs; the bar
         initialiser is hoisted, called from inside init(), and try/catch'd so a
         bar failure cannot take the ten knobs down (pattern_module_toplevel_init_tdz,
         pattern_js_state_updater_overwrites_html_labels).
     14. All 14 controls carry tooltip copy, and showTooltip pins the measured
         width BEFORE placing (pattern_fixed_tooltip_shrink_to_fit_edge) — the
         `mix` knob is the control this would otherwise shrink-wrap.
     15. v1.1.0 four-way knob closure: createParameterLayout == kSliderIds ==
         KNOB_IDS == the knob-<id>/val-<id> elements, plus a FORMAT entry and a
         ui-stub range for each. A knob wired in three of the four places is a
         silently dead control. Also pins the four randomisation defaults at 0,
         which is what keeps every v1.0 session and preset sounding unchanged.

    NOTE: sections 1-15 are STATIC. The tooltip edge-clamp is viewport-sensitive
    and CANNOT be verified here — see tests/ui_tooltip_clamp_check.js, which
    drives the real page at the real 940 x 743 shipping size.

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const path = require('path');
const vm = require('vm');   // v1.9.0 — section 14 evaluates js/i18n.js
const { spawnSync } = require('child_process');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');

const publicDir     = path.join(pluginRoot, 'Source', 'ui', 'public');
const htmlPath      = path.join(publicDir, 'index.html');
const appJsPath     = path.join(publicDir, 'js', 'app.js');
const cssPath       = path.join(publicDir, 'css', 'styles.css');
const editorCppPath = path.join(pluginRoot, 'Source', 'PluginEditor.cpp');
const editorHPath   = path.join(pluginRoot, 'Source', 'PluginEditor.h');
const cmakePath     = path.join(pluginRoot, 'CMakeLists.txt');
const harnessCMake  = path.join(pluginRoot, 'tests', 'render-harness', 'CMakeLists.txt');

// Stage 4: the shared preset module is embedded from the module tree, not from
// Source/ui/public. Sections 3 and 9 both have to know about it.
const presetJsPath  = path.join(repoRoot, 'modules', 'persistence', 'preset-manager',
                                'js', 'preset-manager.js');

const html      = fs.readFileSync(htmlPath, 'utf8');
const appJs     = fs.readFileSync(appJsPath, 'utf8');
const css       = fs.readFileSync(cssPath, 'utf8');
const editorCpp = fs.readFileSync(editorCppPath, 'utf8');
const editorH   = fs.readFileSync(editorHPath, 'utf8');
const cmake     = fs.readFileSync(cmakePath, 'utf8');
const harness   = fs.readFileSync(harnessCMake, 'utf8');
const presetJs  = fs.readFileSync(presetJsPath, 'utf8');

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}

console.log('== O-ReverseDelay ui_frontend_check ==');

// ---------------------------------------------------------------- 1. syntax
{
    const res = spawnSync(process.execPath, ['--input-type=module', '--check'], {
        input: appJs, encoding: 'utf8'
    });
    check(res.status === 0,
        'js/app.js parses as an ES module — a SyntaxError silently kills the whole UI'
        + (res.status !== 0 ? `\n${res.stderr}` : ''));
    check(/<script type="module" src="js\/app\.js">/.test(html),
        'index.html loads app.js as type="module"');
}

// ------------------------------------------------------------------ 2. TDZ
{
    const initCalls = appJs.match(/^init\(\);\s*$/gm) || [];
    check(initCalls.length === 1, 'exactly one top-level init() call');

    // The init() call must be the final statement, so every module-level
    // binding above it is already initialised when it runs.
    const lastMeaningful = appJs
        .split('\n')
        .map(l => l.trim())
        .filter(l => l && !l.startsWith('//'))
        .pop();
    check(lastMeaningful === 'init();',
        'init() is the LAST statement in app.js (no top-level code may follow it)');

    const initIdx = appJs.search(/^init\(\);\s*$/m);
    const declTail = appJs.slice(initIdx).match(/^(?:const|let|var)\s/m);
    check(!declTail, 'no module-level declarations after the init() call');
}

// ------------------------------------------------- 3. native-fn bridge gaps
{
    // Stage 4: the JS side of the bridge is spread across TWO files. app.js
    // fetches three fns (getParameterDefaults + v1.3.0's getGrainMeter +
    // v1.4.0's getWindowCurve); the other ten are fetched by the shared
    // js/preset-manager.js, which app.js imports dynamically. Scanning app.js
    // alone would read 3 vs 13 and false-FAIL on correct code.
    const called = new Set();
    for (const src of [appJs, presetJs])
        for (const m of src.matchAll(/getNativeFunction\(\s*["']([A-Za-z0-9_]+)["']/g))
            called.add(m[1]);

    const registered = new Set();
    for (const m of editorCpp.matchAll(/withNativeFunction\s*\(\s*(?:juce::Identifier\()?\s*"([A-Za-z0-9_]+)"/g))
        registered.add(m[1]);

    // This is the part with the real value: an unregistered fn passes ninja,
    // auval AND pluginval while its control sits silently dead in every DAW.
    const missing = [...called].filter(n => !registered.has(n));
    const dead    = [...registered].filter(n => !called.has(n));

    check(missing.length === 0,
        `all ${called.size} native fns called from JS are registered in PluginEditor.cpp`
        + (missing.length ? ' — MISSING: ' + missing.join(', ') : ''));
    check(dead.length === 0,
        'no dead C++ registrations (registered but never called from JS)'
        + (dead.length ? ' — DEAD: ' + dead.join(', ') : ''));
    // The count is a moving literal and it is SUPPOSED to move: a census
    // assertion is only worth anything because a bridge change fails it loudly
    // instead of drifting. Running justification, one line per bump:
    //   11 -> 12  v1.3.0  getGrainMeter
    //   12 -> 13  v1.4.0  getWindowCurve
    //   13 -> 15  v1.9.0  getUiLanguage + setUiLanguage (the hover-help language
    //                     pair). NOT a tooltip on/off pair — see section 14.
    check(called.size === 15 && registered.size === 15,
        `bridge surface is exactly 15 fns (getParameterDefaults + getGrainMeter`
        + ` + getWindowCurve + getUiLanguage + setUiLanguage + 10 preset)`
        + ` — got JS=${called.size} C++=${registered.size}`);
    check(called.has('getParameterDefaults') && registered.has('getParameterDefaults'),
        'getParameterDefaults is called by the JS AND registered in C++');
    // v1.3.0 (B2): the meter is the one native fn whose failure is INVISIBLE in a
    // still screenshot — an unwired readout renders its em-dash placeholder and
    // looks like a control waiting for audio. Hence a named check, not just a
    // count.
    check(called.has('getGrainMeter') && registered.has('getGrainMeter'),
        'getGrainMeter is called by the JS AND registered in C++');
    // v1.4.0: same invisible-failure argument as the meter, and stronger — an
    // unwired curve leaves an EMPTY panel, which reads as a display waiting for
    // audio rather than as a broken bridge.
    check(called.has('getWindowCurve') && registered.has('getWindowCurve'),
        'getWindowCurve is called by the JS AND registered in C++');

    // savePresetWithDialog is the fn the Save button actually calls; an earlier
    // 9-fn reading of the module contract omitted it, and module.yaml's own
    // native-functions list is still stale at 9. The JS is authoritative.
    const PRESET_FNS = ['savePreset', 'savePresetWithDialog', 'loadPreset',
        'loadPresetFromFile', 'getPresetList', 'getCurrentPreset',
        'selectNextPreset', 'selectPreviousPreset', 'deletePreset', 'isFactoryPreset'];
    const missingPreset = PRESET_FNS.filter(n => !registered.has(n));
    check(missingPreset.length === 0,
        'all 10 preset-manager fns are registered in PluginEditor.cpp'
        + (missingPreset.length ? ' — MISSING: ' + missingPreset.join(', ') : ''));

    // Both dialog fns must resolve {success, name}; preset-manager.js checks
    // `result && result.success`, so a bare bool silently no-ops the bar.
    check((editorCpp.match(/setProperty\s*\(\s*"success"/g) || []).length >= 2,
        'both dialog fns build a {success, name} result object, not a bare bool');

    // MSVC resolves `this` in a NESTED lambda's capture-initialiser to the
    // enclosing closure — a hard compile error that Apple Clang accepts, so it
    // only surfaces on the first Windows CI build and blocks /publish
    // (critical_msvc_safepointer_init_capture_nested_lambda).
    check(!/\[\s*safeThis\s*=\s*juce::Component::SafePointer/.test(editorCpp),
        'SafePointer(this) is hoisted to a local, never init-captured in a nested lambda (MSVC)');

    // complete() on the null path is itself a UAF — the completion is owned by
    // the already-dead WebView Impl (pattern_webview_launchasync_safepointer_no_complete).
    check(!/if\s*\(\s*safeThis\s*==\s*nullptr\s*\)\s*\n?\s*complete/.test(editorCpp),
        'the safeThis == nullptr path bare-returns and never calls complete()');
}

// ---------------------------------------- 4. readouts use getScaledValue()
{
    check(/\.getScaledValue\(\)/.test(appJs),
        'knob readouts computed from getScaledValue() (real C++ range + skew)');

    const fmtMatch = appJs.match(/const FORMAT = \{[\s\S]*?\n\};/);
    check(!!fmtMatch, 'FORMAT display-format table found');
    if (fmtMatch) {
        check(!/\bmin\s*:/.test(fmtMatch[0]) && !/\bmax\s*:/.test(fmtMatch[0]),
            'FORMAT has no min/max fields — display formatting only, no JS range map');
        // The four skewed params must not carry literal range endpoints in JS.
        check(!/\b(2000|20000|3162|316|158)\b/.test(fmtMatch[0]),
            'FORMAT carries no parameter range/skew-centre constants');
    }
    check(/propertiesChangedEvent\.addListener/.test(appJs),
        'controls refresh when the real range/choices arrive via propertiesChanged');

    // The only permitted range math is the documented inverse of
    // getNormalisedValue(), and it must read LIVE properties, never literals.
    const scaledToNorm = appJs.match(/function scaledToNorm[\s\S]*?\n\}/);
    check(!!scaledToNorm && /st\.properties/.test(scaledToNorm[0]),
        'scaledToNorm() derives start/end/skew from live st.properties');
}

// -------------------------------------- 5. dblclick reset uses paramDefaults
{
    check(/paramDefaults\[id\]/.test(appJs),
        'dblclick reset reads paramDefaults (from getParameterDefaults), not a JS constant');
    check(!/setNormalisedValue\(\s*0?\.5\s*\)/.test(appJs),
        'no bare setNormalisedValue(0.5) reset shortcut');
    check(/dblclick/.test(appJs), 'a dblclick handler is bound');
}

// ------------------------------ 6. HTML-authored labels are never overwritten
{
    // FREE / SYNC live in index.html; the refresh path may only touch classes
    // and aria-pressed. Any textContent write to a segment is the O-MBC bug.
    check(/id="seg-free"[^>]*>Free</.test(html) && /id="seg-sync"[^>]*>Sync</.test(html),
        'FREE/SYNC labels are authored in index.html');
    check(!/seg(Free|Sync)\.textContent\s*=/.test(appJs),
        'app.js never assigns textContent on the syncMode segments');
    check(/seg(Free|Sync)\.setAttribute\("aria-pressed"/.test(appJs),
        'segment state is published via aria-pressed');
    check(/classList\.toggle\("active"/.test(appJs),
        'segment state is shown by toggling the active class');
}

// ------------------------------------------- 7. Juce namespace, not __JUCE__
{
    check(/import \* as Juce from "\.\/juce\/index\.js"/.test(appJs),
        'app.js imports the Juce ES-module namespace');
    check(!/window\.__JUCE__/.test(appJs),
        'app.js never touches window.__JUCE__ (no getNativeFunction on it)');
    check(!/window\.__JUCE__/.test(html),
        'index.html never touches window.__JUCE__');
}

// -------------------------------------------- 8. resource provider hygiene
{
    check(!/"juce:\/\//.test(editorCpp) && !/"https:\/\/juce\.backend/.test(editorCpp),
        'getResource() hard-codes no URL scheme (paths arrive bare)');
    check(/url == "\/"/.test(editorCpp),
        'getResource() matches the bare root path "/"');
    check(/getResourceProviderRoot\(\)/.test(editorCpp),
        'goToURL uses WebBrowserComponent::getResourceProviderRoot()');
    check(/charset=utf-8/.test(editorCpp),
        'text resources are served with charset=utf-8');
}

// --------------------------- 9. three-way closure: HTML == provider == CMake
{
    const refs = new Set();
    for (const m of html.matchAll(/(?:src|href)="(?:\.\/)?((?:js|css|img)\/[^"]+)"/g))
        refs.add('/' + m[1]);
    // app.js imports ./juce/index.js relative to /js/, which in turn imports
    // ./check_native_interop.js — both must be served.
    for (const m of appJs.matchAll(/from\s+["']\.\/([^"']+)["']/g))
        refs.add('/js/' + m[1]);
    // ...and the DYNAMIC import of the preset module, which the static `from`
    // regex above cannot see.
    for (const m of appJs.matchAll(/import\(\s*["']\.\/([^"']+)["']\s*\)/g))
        refs.add('/js/' + m[1]);
    refs.add('/js/juce/check_native_interop.js');

    const provided = new Set();
    for (const m of editorCpp.matchAll(/url == "([^"]+)"/g)) provided.add(m[1]);

    const missingProvider = [...refs].filter(r => !provided.has(r));
    check(missingProvider.length === 0,
        `all ${refs.size} local UI references have resource-provider entries`
        + (missingProvider.length ? ' — MISSING: ' + missingProvider.join(', ') : ''));

    // Every provider entry (minus the "/" alias) must be a real embedded file.
    // Strip # comments FIRST: the block's non-greedy match ends at the first
    // ')', so a single parenthesis in an explanatory comment would silently
    // truncate the SOURCES list and turn correct code into a FAIL. Same
    // treatment §11 already gives the harness CMake.
    const cmakeCode = cmake.split('\n').map(l => l.replace(/#.*$/, '')).join('\n');
    const binaryBlock = cmakeCode.match(/juce_add_binary_data\([\s\S]*?\)/);
    check(!!binaryBlock, 'juce_add_binary_data block found in CMakeLists.txt');
    if (binaryBlock) {
        // servedPath -> absolute path on disk. Two source shapes: files under
        // Source/ui/public (served at their own sub-path) and files pulled in
        // from the shared module tree, which getResource() re-homes under /js/.
        // Matching only the first shape made this section FAIL on correct
        // Stage-4 code, because the new entry is a modules/… path.
        const embeddedFiles = new Map();
        for (const m of binaryBlock[0].matchAll(/Source\/ui\/public\/(\S+)/g))
            embeddedFiles.set('/' + m[1], path.join(publicDir, m[1]));
        for (const m of binaryBlock[0].matchAll(/\$\{CMAKE_SOURCE_DIR\}\/(modules\/\S*\/js\/([^/\s]+\.js))/g))
            embeddedFiles.set('/js/' + m[2], path.join(repoRoot, m[1]));

        const embedded = new Set(embeddedFiles.keys());

        const notEmbedded = [...provided].filter(p => p !== '/' && p !== '/index.html' && !embedded.has(p));
        check(notEmbedded.length === 0,
            'every getResource() path is in the juce_add_binary_data SOURCES list'
            + (notEmbedded.length ? ' — MISSING: ' + notEmbedded.join(', ') : ''));

        const notServed = [...embedded].filter(e => e !== '/index.html' && !provided.has(e));
        check(notServed.length === 0,
            'every embedded UI file is served by getResource()'
            + (notServed.length ? ' — UNSERVED: ' + notServed.join(', ') : ''));

        // The embedded files must actually exist on disk — resolved against the
        // tree each one actually came from, not blindly against publicDir.
        const absent = [...embeddedFiles].filter(([, abs]) => !fs.existsSync(abs)).map(([p]) => p);
        check(absent.length === 0,
            'every juce_add_binary_data source exists on disk'
            + (absent.length ? ' — ABSENT: ' + absent.join(', ') : ''));

        check(embedded.has('/js/preset-manager.js'),
            'the shared preset-manager.js is embedded (from modules/, into the SAME UIResources target)');
        check((cmake.match(/juce_add_binary_data\s*\(/g) || []).length === 1,
            'exactly ONE juce_add_binary_data target — a second would default to '
            + 'NAMESPACE BinaryData and duplicate-symbol against UIBinaryData');

        check(/NAMESPACE\s+UIBinaryData/.test(binaryBlock[0]) && /HEADER_NAME\s+UIBinaryData\.h/.test(binaryBlock[0]),
            'binary-data target uses a distinct NAMESPACE *and* HEADER_NAME');
    }
}

// -------------------------------------------------- 10. Windows WebView2
{
    check(/NEEDS_WEB_BROWSER\s+TRUE/.test(cmake), 'NEEDS_WEB_BROWSER TRUE');
    check(/NEEDS_WEBVIEW2\s+TRUE/.test(cmake), 'NEEDS_WEBVIEW2 TRUE');
    check(/JUCE_WEB_BROWSER=1/.test(cmake), 'JUCE_WEB_BROWSER=1');
    check(/JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1/.test(cmake),
        'JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 (or the Windows WebView blanks)');
    check(/withUserDataFolder/.test(editorCpp),
        'WinWebView2 options set a writable withUserDataFolder');
}

// ------------------------------- 11. member order + harness source isolation
{
    const relayIdx  = editorH.search(/std::vector<std::unique_ptr<juce::WebSliderRelay>>/);
    const webIdx    = editorH.search(/std::unique_ptr<juce::WebBrowserComponent>/);
    const attachIdx = editorH.search(/std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>/);
    check(relayIdx > 0 && webIdx > relayIdx && attachIdx > webIdx,
        'editor member order is relays -> webView -> attachments (destroy-in-reverse)');

    // Strip # comments first — the harness CMake *documents* why the editor is
    // excluded, and that prose must not read as a source entry.
    const harnessCode = harness.split('\n').map(l => l.replace(/#.*$/, '')).join('\n');
    check(!/PluginEditor\.cpp/.test(harnessCode),
        'the render harness never compiles PluginEditor.cpp');
    check(/JUCE_WEB_BROWSER=0/.test(harness),
        'the render harness defines JUCE_WEB_BROWSER=0');

    // v1.7.2 (WR-01): the harness's JucePlugin_VersionString must be DERIVED from
    // the plugin target, never mirrored as a literal.
    //
    // This is the third time this value has been checked by hand and the second
    // time it had silently drifted: 1.2.0 while the plugin shipped 1.3.0/1.4.0,
    // then 1.5.0 across v1.6.0, v1.7.0 and v1.7.1. Both preset sentinels key off
    // it, so a stale value makes probes N and R audit older on-disk presets and
    // report a pass. A comment saying "keep in sync" cannot enforce that; this can
    // (pattern_test_fixture_mirrors_drift_silently).
    check(/JucePlugin_VersionString="\$\{_ORD_VERSION\}"/.test(harnessCode),
        'the harness derives JucePlugin_VersionString from the plugin target, not a literal');
    check(/get_target_property\(_ORD_VERSION\s+OuariconReverseDelay\s+JUCE_VERSION\)/.test(harnessCode),
        '_ORD_VERSION is read from the OuariconReverseDelay target JUCE_VERSION property');
    check(!/JucePlugin_VersionString="\d+\.\d+\.\d+"/.test(harnessCode),
        'no hardcoded version literal remains in the harness CMake');

    const processorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.cpp'), 'utf8');
    check(/#if JUCE_WEB_BROWSER\s*\n\s*#include "PluginEditor\.h"/.test(processorCpp),
        'PluginEditor.h is included only inside an #if JUCE_WEB_BROWSER guard');

    const processorH = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.h'), 'utf8');
    check(!/PluginEditor\.h/.test(processorH),
        'PluginProcessor.h stays free of editor includes');
}

// -------------------------------- 12. geometry: preset bar + the three rows
{
    check(/setSize\s*\(\s*940\s*,\s*768\s*\)/.test(editorCpp),
        'editor setSize is 940 x 768 (v1.7.1 chassis)');
    const heights = css.match(/height:\s*768px/g) || [];
    check(heights.length >= 2,
        `styles.css declares 768px in BOTH html/body and .frame — found ${heights.length}`);
    check(!/height:\s*440px/.test(css) && !/height:\s*484px/.test(css)
          && !/height:\s*743px/.test(css) && !/height:\s*972px/.test(css),
        'no superseded frame height survives in styles.css '
        + '(440 Stage-3, 484 Stage-4, 743 v1.1-v1.6, 972 v1.7.0)');

    // v1.7.1: the frame must FIT A 1080p SCREEN with a host's chrome above it,
    // which is the whole point of the release and the one property a future
    // panel row could quietly take back. 900 is the budget: 1080 less the macOS
    // menu bar (~25), a DAW plugin window's title/header strip (~40-70) and a
    // margin. Asserted here rather than left to the comment block, because the
    // v1.7.0 comment block is exactly what asserted the slack away.
    const shipH = Number((editorCpp.match(/setSize\s*\(\s*940\s*,\s*(\d+)\s*\)/) || [])[1]);
    check(shipH > 0 && shipH <= 900,
        `the frame fits a 1080p screen with host chrome — ${shipH} px, budget 900`);
    // The WIDTH must not have moved with it. The tooltip edge-clamp gate below is
    // horizontal and only fires at the real shipping width, so a height change
    // leaves the clamp geometry under test intact and a width change would not
    // (pattern_tooltip_clamp_gate_viewport_sensitive). Asserted rather than
    // trusted, because "we only made it taller" is exactly the claim that stops
    // being true in the release that also nudges a column.
    check(/width:\s*940px/.test(css) && /setSize\s*\(\s*940\s*,/.test(editorCpp),
        'the frame is still 940 px WIDE — the clamp verification survives a height-only resize');

    // The row heights, pinned. If any of them drifts, the rows no longer fit the
    // frame and the tooltip clamp verification below is measuring a stale layout.
    const rowGap = (css.match(/\.groups\s*\{[\s\S]*?gap:\s*(\d+)px/) || [])[1];
    const row2H  = (css.match(/\.group-row-2 \.group\s*\{[\s\S]*?height:\s*(\d+)px/) || [])[1];
    const row1H  = (css.match(/\n\.group\s*\{[\s\S]*?height:\s*(\d+)px/) || [])[1];
    // Row 3 has no height rule of its own — it INHERITS row 1's from the base
    // .group rule, which is why the sum below reads row1 twice.
    // v1.7.1: 145 + 14 + 245 + 14 + 145 = 563.
    const rowsTotal = Number(row1H) + Number(rowGap) + Number(row2H)
                      + Number(rowGap) + Number(row1H);
    check(row1H === '145' && rowGap === '14' && row2H === '245' && rowsTotal === 563,
        `row geometry is 145 + 14 + 245 + 14 + 145 = 563 `
        + `— got row1=${row1H} gap=${rowGap} row2=${row2H} total=${rowsTotal}`);

    // The claim v1.7.1 exists to correct. .groups is flex:1, so its height is the
    // frame's content box MINUS header, preset band and footer — NOT the row sum
    // — and it centres the rows in what is left. Every comment from v1.1.0 to
    // v1.7.0 called the row sum "exactly zero slack" and none of them ever did
    // this subtraction, which is how 93.5 px of centred nothing survived five
    // releases and a 972 px frame (pattern_test_fixture_mirrors_drift_silently:
    // the fixture agreed with the comment because it mirrored the same sum).
    //
    // Chrome is measured, not guessed — these are rendered values from the
    // stub page at the shipping viewport, and ui_tooltip_clamp_check.js renders
    // the same page, so a drift here shows up there as an overflow.
    const CHROME = 6 + 32 + 70.5 + 44 + 23;   // border, padding, header, band, footer
    const groupsH = shipH - CHROME;
    const slack   = groupsH - rowsTotal;
    // The .group-label cartouches sit at top:-9px, straddling each panel's top
    // border, so row 1 needs >= 9 px of clearance under the preset band's rule.
    // Slack is centred, so half of it is what row 1 actually gets.
    check(slack >= 18 && slack <= 40,
        `.groups slack is deliberate and bounded — ${slack.toFixed(1)} px `
        + `(${(slack / 2).toFixed(1)} above row 1; >= 9 needed for .group-label, `
        + `and the whole band <= 40 or it is dead space again)`);

    // Both rows must share ONE width contract or the columns stop aligning.
    // All THREE rows share one contract (v1.7.0), so the 276 px selector is now a
    // group list ending in .group-drift rather than .group-count alone.
    check(/\.group-random,/.test(css) && /\.group-source,/.test(css)
          && /\.group-drift\s*\{\s*width:\s*276px/.test(css),
        'rows 2 and 3 reuse row 1\'s pinned widths (190 | 190 | 276 | 190)');
    // v1.6.0 filled the last reserved panel: SPACE -> MOTION. The assertion that
    // used to live here was the mirror image — v1.3.0 renamed row 2's 276 px
    // column MOTION -> COUNT and this checked that no stale `.group-motion` rule
    // survived it. The name is deliberately live again, on a DIFFERENT panel
    // (190 px, row 2, column 4), so what has to be asserted is that it is REAL:
    // a width rule, a class on the section, and three bound controls inside it.
    check(/\.group-motion\s*[,{]/.test(css) && /class="group group-motion"/.test(html),
        'MOTION is a real panel — .group-motion rule + class both present');
    check(!/\.group-space\s*[,{]/.test(css) && !/class="[^"]*group-space/.test(html),
        'no .group-space rule or class survives the v1.6.0 rename');
    // v1.8.0 spends the reserve, and the assertion flips back for the third
    // time. The rule has now been deleted (v1.6.0), restored (v1.7.0) and
    // deleted again (v1.8.0), tracking whether a reserve actually exists — which
    // is the point of checking it at all. A .group-reserved rule surviving with
    // no reserved panel is dead CSS that still reads like a live layout
    // decision; the ornament surviving in the MARKUP would be worse, drawing a
    // fleuron on top of Diffusion and Drive.
    check(!/\.group-reserved\s*[,{]/.test(css) && !/class="group-reserved"/.test(html),
        'no .group-reserved rule or ornament survives v1.8.0 spending the reserve');
    {
        const start = html.indexOf('class="group group-colour"');
        const end   = start >= 0 ? html.indexOf('</section>', start) : -1;
        const colour = start >= 0 && end > start ? html.slice(start, end) : '';
        check(colour.includes('data-param="diffusion"')
              && colour.includes('data-param="drive"')
              && colour.includes('id="val-diffusion"')
              && colour.includes('id="val-drive"'),
            'COLOUR is a real panel: both B4 #7/#8 knobs bound, both readouts present');
        // Two cells at 72 px with the shared 14 px .group-body gap = 158 px, and
        // the panel is 190. This is the arithmetic that made the fill free — if a
        // later release adds a third control here it FAILS, which is the point:
        // that is the resize the reserve was spent to defer, and it should stop
        // the build rather than silently overflow the panel.
        const cells = (colour.match(/class="knob-cell"/g) || []).length;
        check(cells === 2,
            `COLOUR holds exactly 2 knob-cells (2x72 + 14 = 158 of 190 px) — got ${cells}`);
    }
    {
        const start = html.indexOf('class="group group-motion"');
        const end   = start >= 0 ? html.indexOf('</section>', start) : -1;
        const motion = start >= 0 && end > start ? html.slice(start, end) : '';
        check(motion.includes('id="seg-freeze-on"')
              && motion.includes('id="knob-direction"')
              && motion.includes('id="knob-regenMakeup"'),
            'Freeze, Direction and Regen are all inside the MOTION panel');
    }
    // Both MOTION overrides must be SCOPED. `.segments` and `.segment` are shared
    // with TIME's vertical Free/Sync pair, so an unscoped `flex-direction: row`
    // here would lay that one out sideways too — the same class of change the
    // WINDOW scoping check below guards against.
    check(/\.group-motion \.segments\s*\{/.test(css) && /\.group-motion \.segment\s*\{/.test(css),
        'MOTION\'s segment overrides are scoped to that panel');
    // Extract the WINDOW <section> and look inside it, rather than guessing a
    // character distance — the panel carries long comments and a fixed lookahead
    // silently reported "not inside" for markup that was.
    {
        const start = html.indexOf('class="group group-window"');
        const end   = start >= 0 ? html.indexOf('</section>', start) : -1;
        const winSection = start >= 0 && end > start ? html.slice(start, end) : '';
        check(winSection.includes('id="envelopeCanvas"')
              && winSection.includes('id="knob-tukeyTaper"')
              && winSection.includes('id="combo-grainShape"'),
            'Shape, Tilt, Taper AND the envelope canvas are all inside the WINDOW panel');
    }
    // Every size override for the shrunk WINDOW controls must be SCOPED to that
    // panel. An unscoped .knob or .knob-cell rule here would resize all eight
    // panels, which is the kind of change that looks fine in the one screenshot
    // someone checks and wrong everywhere else.
    ['\\.knob-cell', '\\.knob', '\\.knob-stem', '\\.division-select', '\\.select-cell']
      .forEach((sel) => {
        const unscoped = new RegExp(`(^|\\n)\\s*${sel}\\s*\\{[^}]*?(width:\\s*4\\d px|height:\\s*(4\\d|20)px)`);
        check(!unscoped.test(css),
            `the shrunk ${sel.replace(/\\/g, '')} sizes are scoped to .group-window, not global`);
      });
    check((css.match(/\.group-window\s+\.(knob|knob-cell|knob-stem|division-select|select-cell|group-body)/g) || []).length >= 5,
        'WINDOW-scoped size overrides are present (knob, cell, stem, select, body)');
    check(/\.groups\s*\{[\s\S]*?flex-direction:\s*column/.test(css),
        '.groups is the column holding both rows');
    check(/class="group-row group-row-1"/.test(html) && /class="group-row group-row-2"/.test(html)
          && /class="group-row group-row-3"/.test(html),
        'index.html wraps all THREE panel rows in .group-row');
    // v1.7.0's own panels, by the same standard MOTION is held to: a width rule,
    // a class on the section, and the controls actually inside it.
    {
        const panelHas = (cls, ids) => {
            const start = html.indexOf(`class="group ${cls}"`);
            const end   = start >= 0 ? html.indexOf('</section>', start) : -1;
            const sec   = start >= 0 && end > start ? html.slice(start, end) : '';
            return ids.every((id) => sec.includes(id));
        };
        check(panelHas('group-source', ['id="seg-source-mono"', 'id="seg-source-stereo"']),
            'both Source segments are inside the SOURCE panel');
        check(panelHas('group-duck', ['id="knob-duck"']),
            'Duck is inside the DUCK panel');
        check(panelHas('group-drift', ['id="knob-driftRate"', 'id="knob-driftDepth"']),
            'Rate and Depth are both inside the DRIFT panel');
        // Same scoping rule MOTION's segments are held to: .segments and .segment
        // are shared with TIME's VERTICAL Free/Sync pair, so an unscoped
        // flex-direction here would lay that one out sideways too.
        check(/\.group-source \.segments\s*\{/.test(css) && /\.group-source \.segment\s*\{/.test(css),
            'SOURCE\'s segment overrides are scoped to that panel');
    }

    // The band and the height increase must be the same 44 px, or the panels
    // and footer move (D15's whole low-regression premise).
    const barBlock = css.match(/\.preset-bar\s*\{[\s\S]*?\}/);
    check(!!barBlock, '.preset-bar rule found');
    if (barBlock) {
        const h  = (barBlock[0].match(/height:\s*(\d+)px/) || [])[1];
        const mb = (barBlock[0].match(/margin-bottom:\s*(\d+)px/) || [])[1];
        check(h && mb && (Number(h) + Number(mb)) === 44,
            `.preset-bar occupies exactly 44px (height ${h} + margin-bottom ${mb})`);
    }
}

// ------------------------------------------ 13. Stage 4: preset bar bindings
{
    const BAR_IDS = ['preset-name', 'preset-prev', 'preset-next',
                     'preset-save', 'preset-load', 'preset-delete'];
    const missingIds = BAR_IDS.filter(id => !new RegExp(`id="${id}"`).test(html));
    check(missingIds.length === 0,
        'all 6 preset-bar element IDs are present in index.html'
        + (missingIds.length ? ' — MISSING: ' + missingIds.join(', ') : ''));

    // Button copy is content and must never be a literal in app.js. A shared JS
    // updater writing textContent is what left O-MultiBandCompressor's band
    // buttons reading "Off Off Off" in every DAW since launch.
    //
    // v1.10.0: this assertion was REWRITTEN, and it still guards that same rule.
    // Through v1.9.0 the two faces were data-label / data-confirm attributes
    // authored on the button, and this checked for them. Those attributes are
    // gone by design: an attribute holds ONE string, so on a two-language page a
    // switch while the button was armed would have restored the ENGLISH armed
    // face. The faces are KEYS now, resolved through setLabel(), which makes the
    // button a [data-i18n] element the language sweep owns.
    //
    // The evidence moved with them. A key is checkable in three ways the
    // attribute never was: it must be a plain string LITERAL (a computed key
    // could hide a raw caption), it must RESOLVE in LABELS or I18N, and the
    // element must declare a starting key in the markup so the pre-applyI18n
    // fallback is the right word. All three are asserted below.
    check(/id="preset-delete"[\s\S]{0,200}?data-i18n="label\.delete"/.test(html),
        '#preset-delete declares its unarmed key in the markup (data-i18n="label.delete")');

    const deleteSetLabels = [...appJs.matchAll(/setLabel\(\s*btn\s*,\s*(['"])([\w.]+)\1\s*\)/g)]
        .map((m) => m[2]);
    check(deleteSetLabels.includes('label.delete') && deleteSetLabels.includes('ui.confirm'),
        'the delete button swaps its face through setLabel() with PLAIN STRING keys, never a JS '
        + `literal caption — found [${deleteSetLabels.join(', ')}], expected label.delete and ui.confirm`);
    check(!/btn\.textContent\s*=/.test(appJs),
        'nothing writes the delete button\'s textContent directly — applyLabel() owns it');

    // window.confirm is a silent no-op or a throw in some JUCE WebView backends;
    // the module falls back to it unless onConfirmDelete is supplied.
    check(/onConfirmDelete\s*:/.test(appJs),
        'an onConfirmDelete hook is supplied (never the window.confirm fallback)');
    check(!/window\.confirm/.test(appJs),
        'app.js never calls window.confirm');

    // TDZ: the bar initialiser must be a hoisted declaration invoked from INSIDE
    // init(), with no module-level import() — a top-level initialiser reaching a
    // not-yet-initialised binding throws out of module evaluation and silently
    // kills all ten already-working knobs (pattern_module_toplevel_init_tdz).
    check(/^async function initPresetBar\(/m.test(appJs),
        'initPresetBar is a hoisted function declaration');
    check(/function init\(\)\s*\{[\s\S]*?\n\}/.test(appJs)
       && /function init\(\)\s*\{[\s\S]*?initPresetBar\(\);[\s\S]*?\n\}/.test(appJs),
        'initPresetBar() is called from INSIDE init()');
    check(!/^\s*initPresetBar\(\);\s*$/m.test(appJs.replace(/function init\(\)\s*\{[\s\S]*?\n\}/, '')),
        'initPresetBar() is never called at module top level');
    check(/^\s*(?:const|let|var)\s+presetManager\s*=/m.test(appJs),
        'presetManager is declared in the top state block, not lazily mid-file');

    // A bar failure must not take the ten verified controls down with it.
    const barFn = appJs.match(/async function initPresetBar\(\)[\s\S]*?\n\}/);
    check(!!barFn && /try\s*\{/.test(barFn[0]) && /catch\s*\(/.test(barFn[0]),
        'initPresetBar wraps its import + init in try/catch (the bar dies alone)');
}

// ----------------------------------------------- 14. Stage 4: tooltip layer
{
    const TIP_ANCHORS = ['syncSegments', 'combo-noteDivision',
        'knob-delayTime', 'knob-grainSize', 'knob-density', 'knob-feedback',
        'knob-lowCut', 'knob-highCut', 'knob-width', 'knob-mix',
        // v1.1.0 RANDOM panel
        'knob-jitter', 'knob-delayScatter', 'knob-sizeRandom', 'knob-gainRandom',
        // v1.2.0 WINDOW panel — the select is an anchor too, exactly as
        // combo-noteDivision is; a tooltip inventory that only listed knobs
        // would leave every choice control undocumented.
        'combo-grainShape', 'knob-grainTilt',
        // v1.3.0 COUNT panel. The meter is a tooltip anchor without being a
        // control, which is deliberate: "Active 9 / Overlap 5.6x" is the one
        // thing on this page that reports rather than sets, and it needs to say
        // so somewhere.
        'knob-grainCount', 'grainMeter',
        // v1.4.0 WINDOW's Taper + the ENVELOPE display. The display is an anchor
        // without being a control, as grainMeter is: it reports rather than sets,
        // and it needs to say so somewhere.
        'knob-tukeyTaper', 'envelopeCell',
        // v1.6.0 MOTION panel. These were missing from the inventory when the
        // panel shipped — the list is hand-maintained, so it drifts exactly the
        // way a fixture that mirrors the page drifts
        // (pattern_test_fixture_mirrors_drift_silently). Backfilled here.
        'freezeSegments', 'knob-direction', 'knob-regenMakeup',
        // v1.7.0 row 3. The segment pair is an anchor in its own right, as
        // syncSegments and freezeSegments are — a tooltip inventory that only
        // listed knobs would leave every mode control undocumented.
        'sourceSegments', 'knob-duck', 'knob-driftRate', 'knob-driftDepth',
        // v1.7.2 COLOUR. Missing from this inventory when the panel shipped —
        // the list is hand-maintained, so it drifts exactly the way a fixture
        // that mirrors the page drifts
        // (pattern_test_fixture_mirrors_drift_silently). This is the SECOND
        // time; the first was v1.6.0's MOTION panel, backfilled above. Found
        // here because v1.9.0 cross-checks the inventory against TIP_BINDINGS,
        // which is derived from the page rather than typed.
        'knob-diffusion', 'knob-drive'];

    // ── v1.9.0: this assertion was REWRITTEN, and made stronger ─────────────
    //
    // Through v1.7.3 it read the two attributes straight out of index.html. That
    // fails by construction now: the copy has moved into js/i18n.js and
    // applyI18n() writes the attributes at runtime, so there is nothing left in
    // the markup to match and scripts/check-i18n.js assertion 3 fails the plugin
    // if a literal ever reappears there.
    //
    // The replacement is not a weakening. The old form could only ever say "some
    // string is present"; this one says every anchor is BOUND in TIP_BINDINGS,
    // resolves to a key that exists, and carries BOTH an en and an fr entry — so
    // a missing translation now fails here, which the old assertion was
    // structurally incapable of noticing.
    //
    // i18n.js is an ES module outside any package.json, so node can neither
    // require() nor import() it synchronously. Evaluated in a vm sandbox with the
    // export keywords stripped, exactly as scripts/check-i18n.js does it —
    // check-i18n assertion 7 independently proves the file holds nothing but
    // export declarations, so there is nothing else to evaluate.
    const i18nPath = path.join(publicDir, 'js', 'i18n.js');
    check(fs.existsSync(i18nPath), 'js/i18n.js exists (the hover-help copy table)');

    let I18N = null, TIP_BINDINGS = null, LABELS = null;
    if (fs.existsSync(i18nPath)) {
        try {
            const src = fs.readFileSync(i18nPath, 'utf8')
                .replace(/(^|\n)(\s*)export\s+(const|let|function|class)\s/g, '$1$2$3 ');
            const sandbox = { console: { warn() {}, error() {}, log() {} } };
            vm.createContext(sandbox);
            vm.runInContext(`${src}\n;globalThis.__x = { I18N, TIP_BINDINGS, LABELS };`,
                            sandbox, { timeout: 5000 });
            ({ I18N, TIP_BINDINGS, LABELS } = sandbox.__x);
        } catch (e) {
            check(false, `js/i18n.js evaluates — ${e.message}`);
        }
    }

    if (I18N && TIP_BINDINGS) {
        // Anchor -> the key it is bound to, derived from TIP_BINDINGS rather
        // than assumed to be id-equals-key. Only '#id' selectors are inverted
        // here; this page has no wrapper-bound tip and the coverage check below
        // would report one as uncovered rather than silently passing it.
        const boundKey = new Map();
        for (const [selector, key] of TIP_BINDINGS) {
            const m = /^#([A-Za-z0-9_-]+)$/.exec(selector);
            if (m) boundKey.set(m[1], key);
        }

        const unbound = TIP_ANCHORS.filter(id => !boundKey.has(id));
        check(unbound.length === 0,
            `all ${TIP_ANCHORS.length} controls are bound in TIP_BINDINGS`
            + (unbound.length ? ' — UNBOUND: ' + unbound.join(', ') : ''));

        // Every key referenced must exist and be translated BOTH ways. A French
        // entry that was never written is the failure this catches.
        const badKey = [];
        for (const id of TIP_ANCHORS) {
            const key = boundKey.get(id);
            if (key === undefined) continue;          // already reported above
            const e = I18N[key];
            if (!e)                                    badKey.push(`${id} -> ${key} (no such key)`);
            else if (!e.en || !e.en.t || !e.en.b)      badKey.push(`${id} -> ${key} (en incomplete)`);
            else if (!e.fr || !e.fr.t || !e.fr.b)      badKey.push(`${id} -> ${key} (fr MISSING)`);
        }
        check(badKey.length === 0,
            `every anchor's key has a complete en AND fr entry in js/i18n.js`
            + (badKey.length ? ' — BAD: ' + badKey.join('; ') : ''));

        // The binding is useless if the element it names is not on the page.
        // applyI18n() only console.warns on a miss, so without this the tip
        // would simply never appear and nothing would say so.
        const dangling = [...boundKey.keys()]
            .filter(id => !new RegExp(`id="${id}"`).test(html));
        check(dangling.length === 0,
            `every TIP_BINDINGS selector resolves to an id in index.html`
            + (dangling.length ? ' — DANGLING: ' + dangling.join(', ') : ''));
    }

    // ── v1.10.0: the SAME shape, extended to LABELS ────────────────────────
    // The anchor-coverage assertion above earns its extension: in v1.9.0 it
    // immediately found that the COLOUR panel had shipped without adding
    // knob-diffusion and knob-drive to the inventory. Labels can drift the same
    // way and more easily, because there are more of them and because a key can
    // be reused from I18N rather than declared in LABELS.
    //
    // Every data-i18n / data-i18n-aria value in the markup must RESOLVE — in
    // LABELS first, falling back to I18N exactly as trLabel() does — and must
    // carry a title in BOTH languages. A key that resolves in English only
    // ships an English caption on a French page and nothing else would say so.
    if (I18N) {
        const markupKeys = [...html.matchAll(/data-i18n(?:-aria|-placeholder|-alt)?="([^"]+)"/g)]
            .map((m) => m[1]);
        check(markupKeys.length > 0, 'index.html carries data-i18n keys at all');

        const resolve = (k) => (LABELS && LABELS[k]) || I18N[k];
        const badLabel = [];
        for (const key of new Set(markupKeys)) {
            const e = resolve(key);
            if (!e)                              badLabel.push(`${key} (no such key)`);
            else if (!e.en || !e.en.t)           badLabel.push(`${key} (en incomplete)`);
            else if (!e.fr || !e.fr.t)           badLabel.push(`${key} (fr MISSING)`);
        }
        check(badLabel.length === 0,
            `every data-i18n key in index.html resolves in LABELS or I18N and carries en AND fr`
            + (badLabel.length ? ' — BAD: ' + badLabel.join('; ') : ''));

        // D13 in its label form. This plugin has NO hover-help toggle and never
        // will, so it has no ui.on / ui.off pair — the popover carries the
        // language selector alone. A key named for one appearing here would mean
        // the toggle had been reintroduced through the label table.
        const toggleKeys = [...new Set(markupKeys)].filter((k) => /^ui\.(on|off)$/.test(k));
        check(toggleKeys.length === 0,
            'no ui.on / ui.off label key exists — D13: this plugin has no hover-help toggle'
            + (toggleKeys.length ? ' — FOUND: ' + toggleKeys.join(', ') : ''));
    }

    check(/id="tooltip"/.test(html), 'the #tooltip host element exists');
    check(/\.tooltip\s*\{[\s\S]*?position:\s*fixed/.test(css),
        '.tooltip is position:fixed (escapes .frame\'s inset shadow)');

    // MEASURE-THEN-PIN: the width must be released, measured from left:0, and
    // PINNED before any `left` is applied. Measuring at the previous offset
    // under-reports the width, and a near-edge `left` then re-wraps a 230px tip
    // into a ~70px ribbon. Here `mix` (right-most) is the exposed control, and
    // no automated gate but this one can see it
    // (pattern_fixed_tooltip_shrink_to_fit_edge).
    const showFn = appJs.match(/function showTooltip\([\s\S]*?\n\}/);
    check(!!showFn, 'showTooltip() found');
    if (showFn) {
        const body     = showFn[0];
        const release  = body.search(/style\.width\s*=\s*['"]{2}/);
        const measure  = body.search(/getBoundingClientRect\(\)\.width/);
        const pin      = body.search(/style\.width\s*=\s*`\$\{width\}px`/);
        const placeIdx = body.search(/style\.left\s*=\s*`\$\{left\}px`/);
        check(release >= 0 && measure > release && pin > measure && placeIdx > pin,
            'tooltip width is released -> measured -> PINNED before `left` is applied');
        check(/style\.left\s*=\s*['"]0px['"]/.test(body),
            'the measurement happens at left:0, not at the previous offset');
        check(/--arrow-x/.test(body) && /--arrow-x/.test(css),
            'the arrow tracks the anchor via --arrow-x after clamping');
        check(/Math\.max\(\s*TOOLTIP_MARGIN/.test(body) && /Math\.min\(\s*maxLeft/.test(body),
            'left is clamped into [MARGIN, innerWidth - width - MARGIN]');
    }

    // A tip must not hang over a knob mid-drag.
    check(/tooltipSuppressed\s*=\s*true/.test(appJs) && /tooltipSuppressed\s*=\s*false/.test(appJs),
        'tooltips are suppressed on pointerdown and re-enabled on pointerup');

    // v1.7.2 (WR-05): a knob drag must CAPTURE the pointer and must terminate on
    // cancel and lost-capture as well as on up.
    //
    // With window listeners and only a `pointerup` to end the drag, any path that
    // does not deliver that event leaves `dragging` true and the listeners
    // attached — drag out of the plugin window and release over the DAW, let the
    // host take a modal grab, or let the OS synthesise a pointercancel. Two silent
    // consequences: the knob follows the cursor with no button held, and
    // sliderDragStarted() is left unmatched so the host's automation gesture stays
    // open (Logic and Live latch automation write on that parameter).
    //
    // Asserted structurally because neither this script nor
    // ui_tooltip_clamp_check.js can reach the state — both dispatch synthetic
    // events that always deliver their pointerup.
    check(/setPointerCapture\(\s*e\.pointerId\s*\)/.test(appJs),
        'knob pointerdown captures the pointer (setPointerCapture)');
    check(/knob\.addEventListener\(\s*["']pointercancel["']/.test(appJs),
        'knob drags terminate on pointercancel, not only pointerup');
    check(/knob\.addEventListener\(\s*["']lostpointercapture["']/.test(appJs),
        'knob drags terminate on lostpointercapture');
    check(!/window\.addEventListener\(\s*["']pointermove["']/.test(appJs),
        'knob pointermove is bound to the knob, not to window (a lost pointerup would strand it)');
    // sliderDragEnded() must be reachable from the same handler all four paths
    // share, or the gesture can still be left open.
    check(/const onUp = \([\s\S]{0,400}?st\.sliderDragEnded\(\)/.test(appJs),
        'the shared onUp handler is what calls sliderDragEnded()');

    // ── D13: hover help is DISPLAY-ONLY on this plugin ──────────────────────
    //
    // No on/off toggle, no persisted enabled flag, and therefore no
    // setTooltipsEnabled native function. This is a locked user decision and it
    // is NOT relaxed here — nine other plugins in the suite have that toggle;
    // O-ReverseDelay deliberately does not, and its settings popover carries the
    // language selector alone.
    //
    // v1.9.0 TIGHTENED the test rather than weakening it. Through v1.7.3 it was
    // a bare substring search for the name anywhere in either file, which
    // conflates two different things: REGISTERING the function, and writing a
    // comment that says the function is deliberately absent. v1.9.0's comments
    // explaining the absence contain the literal name, so the old form failed on
    // source that honours D13 exactly — a gate reporting a violation of a rule
    // the code is obeying.
    //
    // So it now matches the two forms that would constitute a real violation:
    // a C++ withNativeFunction registration, and a JS getNativeFunction fetch.
    // Either would put the function on the bridge; a comment cannot. The section
    // 3 census is the second line of defence — a registration would also move
    // the count off 15 and fail there.
    const cppRegisters = /withNativeFunction\s*\(\s*(?:juce::Identifier\()?\s*["']setTooltipsEnabled["']/.test(editorCpp);
    const jsFetches    = /getNativeFunction\s*\(\s*["']setTooltipsEnabled["']/.test(appJs);
    check(!cppRegisters && !jsFetches,
        'no tooltip-enable native fn is registered or fetched (D13: display-only hover help)'
        + (cppRegisters ? ' — REGISTERED in PluginEditor.cpp' : '')
        + (jsFetches ? ' — FETCHED in app.js' : ''));

    // The other half of D13: no persisted enabled flag on the processor either.
    // A toggle could in principle be wired without a native function, through the
    // state tree alone, and this is what would catch that.
    const processorH = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.h'), 'utf8');
    check(!/tooltipsEnabled/.test(processorH),
        'the processor holds no tooltipsEnabled state (D13: nothing to persist)');
}

// ------------------------- 15. v1.1.0: four-way knob/parameter closure
// The knob inventory now lives in FOUR places: createParameterLayout() in the
// processor, kSliderIds in the editor (relays + attachments), KNOB_IDS in
// app.js, and the knob-<id> elements in index.html. Wire a parameter into
// three of them and the fourth leaves a control that is silently dead — the
// same failure class as an unregistered native function (section 3), and just
// as invisible to ninja, auval and pluginval.
//
// v1.1.0 added four knobs at once across all four files, which is exactly when
// this drifts.
{
    const processorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.cpp'), 'utf8');

    // Float params declared in createParameterLayout(). The three choice params
    // (syncMode, noteDivision, grainShape) and — since v1.6.0 — every BOOL param
    // are deliberately excluded: none of them is a knob, and each reaches the
    // page through a relay of its own type.
    const layoutIds = new Set();
    for (const m of processorCpp.matchAll(/ParameterID\s*\{\s*"([A-Za-z0-9_]+)",\s*1\s*\}/g))
        layoutIds.add(m[1]);
    for (const m of processorCpp.matchAll(/AudioParameterChoice>\(\s*\n?\s*juce::ParameterID\s*\{\s*"([A-Za-z0-9_]+)"/g))
        layoutIds.delete(m[1]);
    ['syncMode', 'noteDivision', 'grainShape'].forEach(id => layoutIds.delete(id));

    // v1.6.0 — bool params, scraped the same way the choices are, and then
    // closed against kToggleIds in BOTH directions below. Removed from layoutIds
    // first so the knob closure does not report `freeze` as a missing knob.
    //
    // This is the check that catches a bool landing in the wrong relay list.
    // WebSliderRelay attaches to an AudioParameterBool without complaint and
    // produces a control whose state never updates — build, auval and pluginval
    // all pass, and the switch is simply dead
    // (pattern_webview_native_fn_bridge_gap by a different route).
    const boolIds = new Set();
    for (const m of processorCpp.matchAll(/AudioParameterBool>\(\s*\n?\s*juce::ParameterID\s*\{\s*"([A-Za-z0-9_]+)"/g)) {
        boolIds.add(m[1]);
        layoutIds.delete(m[1]);
    }

    const toggleIds = new Set();
    const toggleBlock = editorCpp.match(/kToggleIds\s*\{([\s\S]*?)\};/);
    if (toggleBlock)
        for (const m of toggleBlock[1].matchAll(/"([A-Za-z0-9_]+)"/g)) toggleIds.add(m[1]);

    // v1.7.0 — the CHOICE closure, and it is the one gap the v1.6.0 bool closure
    // left open. Through v1.6.0 the choice ids were excluded from the knob
    // closure by a hand-written list and then checked against nothing: a choice
    // parameter that never reached kComboIds would simply be absent from every
    // assertion here. `sourceMode` is precisely that case — it is a choice living
    // next to three new floats, and WebSliderRelay would attach to it without
    // complaint and produce a control whose index never updates.
    //
    // Closed in BOTH directions, like the bool one, and then end to end into
    // app.js: every choice must be built through getComboBoxState there, whether
    // it is drawn as a select (noteDivision, grainShape) or as a segment pair
    // (syncMode, sourceMode).
    const choiceIds = new Set();
    for (const m of processorCpp.matchAll(/AudioParameterChoice>\(\s*\n?\s*juce::ParameterID\s*\{\s*"([A-Za-z0-9_]+)"/g))
        choiceIds.add(m[1]);

    const comboIds = new Set();
    const comboBlock = editorCpp.match(/kComboIds\s*\{([\s\S]*?)\};/);
    if (comboBlock)
        for (const m of comboBlock[1].matchAll(/"([A-Za-z0-9_]+)"/g)) comboIds.add(m[1]);

    const editorIds = new Set();
    const sliderBlock = editorCpp.match(/kSliderIds\s*\{([\s\S]*?)\};/);
    if (sliderBlock)
        for (const m of sliderBlock[1].matchAll(/"([A-Za-z0-9_]+)"/g)) editorIds.add(m[1]);

    const jsIds = new Set();
    const knobBlock = appJs.match(/const KNOB_IDS = \[([\s\S]*?)\];/);
    if (knobBlock)
        for (const m of knobBlock[1].matchAll(/"([A-Za-z0-9_]+)"/g)) jsIds.add(m[1]);

    const htmlIds = new Set();
    for (const m of html.matchAll(/id="knob-([A-Za-z0-9_]+)"/g)) htmlIds.add(m[1]);

    const readoutIds = new Set();
    for (const m of html.matchAll(/id="val-([A-Za-z0-9_]+)"/g)) readoutIds.add(m[1]);

    const diff = (a, b) => [...a].filter(x => !b.has(x));
    const diffOk = (a, b) => diff(a, b).length === 0 && diff(b, a).length === 0;
    const diffDetail = (a, b) => {
        const missing = diff(a, b), extra = diff(b, a);
        return (missing.length ? ' — missing: ' + missing.join(', ') : '')
             + (extra.length   ? ' — unexpected: ' + extra.join(', ')  : '');
    };

    check(sliderBlock && knobBlock, 'kSliderIds and KNOB_IDS blocks both found');

    // v1.6.0 — the bool closure, asserted in both directions and end to end:
    // APVTS bool <-> editor kToggleIds <-> a segment pair in the HTML <-> a
    // getToggleState() call in app.js. Four places, exactly like the knobs.
    check(!!comboBlock, 'kComboIds block found in PluginEditor.cpp');
    check(comboBlock && diffOk(choiceIds, comboIds),
        `APVTS choice params == editor kComboIds (${choiceIds.size})`
        + (comboBlock ? diffDetail(choiceIds, comboIds) : ''));
    [...choiceIds].forEach((id) => {
        check(new RegExp(`getComboBoxState\\(\\s*(COMBO_[A-Z_]+|["']${id}["'])`).test(appJs)
              || new RegExp(`COMBO_[A-Z_]+\\s*=\\s*["']${id}["']`).test(appJs),
            `${id} reaches app.js through a ComboBoxState (not a slider or toggle state)`);
        check(!editorIds.has(id) && !toggleIds.has(id),
            `${id} is absent from kSliderIds and kToggleIds (a choice has no SliderState)`);
    });

    check(!!toggleBlock, 'kToggleIds block found in PluginEditor.cpp');
    check(diff(boolIds, toggleIds).length === 0 && diff(toggleIds, boolIds).length === 0,
        `APVTS bool params == editor kToggleIds (${boolIds.size})`
        + (diff(boolIds, toggleIds).length ? ' — NO RELAY: ' + diff(boolIds, toggleIds).join(', ') : '')
        + (diff(toggleIds, boolIds).length ? ' — NO PARAM: ' + diff(toggleIds, boolIds).join(', ') : ''));

    // A bool must reach the page through getToggleState, never getSliderState or
    // getComboBoxState — the wrong call builds a state the backend never updates.
    //
    // Tested against app.js with COMMENTS STRIPPED, and that is not fastidiousness:
    // the first run of this check failed because app.js's own header explains the
    // trap in prose containing the literal `getSliderState("freeze")`, so the file
    // was flagged by its own documentation of the thing it does not do. Exactly
    // the failure the .group-motion selector check above records — a substring
    // test reading a changelog as code — and the fix is the same one, applied to
    // the other language.
    const appJsCode = appJs
        .replace(/\/\*[\s\S]*?\*\//g, ' ')
        .replace(/(^|[^:])\/\/[^\n]*/g, '$1');

    for (const id of boolIds) {
        check(new RegExp(`getToggleState\\(\\s*(TOGGLE_[A-Z_]+|["']${id}["'])`).test(appJsCode),
            `${id} is bound with getToggleState in app.js (not a slider or combo state)`);
        check(!new RegExp(`getSliderState\\(\\s*["']${id}["']`).test(appJsCode)
              && !new RegExp(`getComboBoxState\\(\\s*["']${id}["']`).test(appJsCode),
            `${id} is NOT bound as a slider or combo state`);
        check(!jsIds.has(id) && !editorIds.has(id),
            `${id} is absent from KNOB_IDS and kSliderIds (a bool has no SliderState)`);
    }

    check(diff(layoutIds, editorIds).length === 0 && diff(editorIds, layoutIds).length === 0,
        `APVTS float params == editor kSliderIds (${layoutIds.size})`
        + (diff(layoutIds, editorIds).length ? ' — NO RELAY: ' + diff(layoutIds, editorIds).join(', ') : '')
        + (diff(editorIds, layoutIds).length ? ' — NO PARAM: ' + diff(editorIds, layoutIds).join(', ') : ''));

    check(diff(editorIds, jsIds).length === 0 && diff(jsIds, editorIds).length === 0,
        `editor kSliderIds == app.js KNOB_IDS (${editorIds.size})`
        + (diff(editorIds, jsIds).length ? ' — UNBOUND IN JS: ' + diff(editorIds, jsIds).join(', ') : '')
        + (diff(jsIds, editorIds).length ? ' — NO RELAY: ' + diff(jsIds, editorIds).join(', ') : ''));

    check(diff(jsIds, htmlIds).length === 0,
        'every KNOB_IDS entry has a knob-<id> element in index.html'
        + (diff(jsIds, htmlIds).length ? ' — MISSING: ' + diff(jsIds, htmlIds).join(', ') : ''));

    check(diff(jsIds, readoutIds).length === 0,
        'every KNOB_IDS entry has a val-<id> readout in index.html'
        + (diff(jsIds, readoutIds).length ? ' — MISSING: ' + diff(jsIds, readoutIds).join(', ') : ''));

    // A knob with no FORMAT entry falls back to toFixed(2) and silently shows a
    // unitless number where every neighbour shows "%" or "ms".
    const fmtBlock = appJs.match(/const FORMAT = \{([\s\S]*?)\n\};/);
    const fmtIds = new Set();
    if (fmtBlock)
        for (const m of fmtBlock[1].matchAll(/^\s*([A-Za-z0-9_]+)\s*:/gm)) fmtIds.add(m[1]);
    check(diff(jsIds, fmtIds).length === 0,
        'every knob has a FORMAT entry (else the readout loses its unit)'
        + (diff(jsIds, fmtIds).length ? ' — MISSING: ' + diff(jsIds, fmtIds).join(', ') : ''));

    // Every parameter added after v1.0.0 MUST default to the engine's NO-OP —
    // that is what keeps existing sessions and presets sounding exactly as they
    // did, and it is the whole reason each of these releases is a MINOR bump.
    //
    // The no-op is NOT always 0. v1.1's four randomisations are off at 0, but
    // v1.2's grainTilt is symmetric at 0.5 and hard-tilted at 0 — so this is a
    // table of expected values rather than a blanket "=== 0", and getting that
    // distinction wrong is precisely the silent re-voicing this guards against
    // (pattern_activating_dead_param_default_timbre).
    const NOOP_DEFAULTS = {
        jitter:       0,     // v1.1.0 (B3) — off
        delayScatter: 0,
        sizeRandom:   0,
        gainRandom:   0,
        grainTilt:    0.5,   // v1.2.0 (B1) — SYMMETRIC, the shipped Hann window
        tukeyTaper:   0.5,   // v1.4.0 — v1.2.0's FROZEN Tukey taper. Not 0.01
                             // (the range min) and not 1.0 (which is Hann).
                             // Fourth release running where the no-op default is
                             // a specific number rather than zero.
        grainCount:   8,     // v1.3.0 (B2) — v1.2.0's HARD-CODED ceiling.
                             // Not 2 (the range min) and emphatically not 16
                             // (the new max): 8 is the only value at which
                             // `min + d·(ceiling−min)` reproduces v1.0.1's
                             // `2 + d·6` bitwise, which is what lets every
                             // saved session keep its density meaning. Density
                             // is stored DENORMALISED, so there is no migration
                             // available if this is ever wrong
                             // (critical_apvts_denormalised_vs_preset_normalised).
        direction:    0,     // v1.6.0 (B4 #2) — all-reverse, the shipped read law.
        regenMakeup:  0,     // v1.6.0 (B4 #3) — 0 dB, i.e. regenMakeupGain()
                             // returns exactly 1.0f and the loop gain is bitwise
                             // what v1.5.0 shipped. First release since v1.1.0
                             // where the new controls' no-ops really ARE zero,
                             // which is why they sit in this table by value.
    };

    // grainCount's default is written as the named constant kLegacyOverlapMax
    // rather than a literal, so the numeric scan below cannot see it. Assert the
    // binding instead — which is the stronger check anyway: the point is that the
    // parameter default and the coherence-trim anchor are the SAME constant, and
    // a literal 8.0f in either place could drift from the other.
    {
        const decl = processorCpp.match(
            /ParameterID\s*\{\s*"grainCount",\s*1\s*\}[\s\S]*?\n\s*(kLegacyOverlapMax)\)\);/);
        check(!!decl,
            'grainCount defaults to kLegacyOverlapMax (the named constant, not a literal 8)');
        // Read locally — section 5's processorH is scoped to its own block.
        const procH = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.h'), 'utf8');
        const konst = procH.match(/kLegacyOverlapMax\s*=\s*([0-9.]+)f/);
        check(!!konst && Number(konst[1]) === 8,
            'kLegacyOverlapMax is 8 — v1.2.0\'s hard-coded overlap ceiling'
            + (konst ? ` — got ${konst[1]}` : ' — constant not found'));
        // Both trims must be anchored at that same constant and return EXACTLY
        // 1.0f at or below it, or the bit-identity claim is false.
        check(/loopCountTrim[\s\S]{0,400}?overlap <= kLegacyOverlapMax[\s\S]{0,80}?return 1\.0f;/.test(procH),
            'loopCountTrim returns exactly 1.0f at overlap <= kLegacyOverlapMax');
    }

    // v1.4.0 — tukeyTaper's default is likewise a named constant, and the same
    // binding check is the stronger one: WindowLut's α stats grid and the
    // parameter default must be the SAME 0.5, or the grid's "exact at the
    // default" guarantee would be exact at a value the parameter never takes.
    {
        const winH = fs.readFileSync(
            path.join(pluginRoot, 'Source', 'dsp', 'WindowLut.h'), 'utf8');

        const decl = processorCpp.match(
            /ParameterID\s*\{\s*"tukeyTaper",\s*1\s*\}[\s\S]*?\n\s*WindowLut::kTukeyTaperDefault\)\);/);
        check(!!decl,
            'tukeyTaper defaults to WindowLut::kTukeyTaperDefault (the named constant)');

        const konst = winH.match(/kTukeyTaperDefault\s*=\s*([0-9.]+)f/);
        check(!!konst && Number(konst[1]) === 0.5,
            "kTukeyTaperDefault is 0.5 — v1.2.0's frozen taper"
            + (konst ? ` — got ${konst[1]}` : ' — constant not found'));

        // The parameter's STEP must match the stats grid's step, or α stops
        // landing exactly on precomputed entries and the default's bitwise
        // guarantee is gone. This is the one drift that would be silent.
        const step = winH.match(/kTukeyTaperStep\s*=\s*([0-9.]+)f/);
        const steps = winH.match(/kNumTaperSteps\s*=\s*(\d+)/);
        check(!!step && !!steps && Math.abs(Number(step[1]) * Number(steps[1]) - 1.0) < 1e-9,
            'kTukeyTaperStep x kNumTaperSteps == 1.0, so every reachable alpha is a grid point'
            + (step && steps ? ` — ${step[1]} x ${steps[1]}` : ''));

        // And the range must be built FROM those constants rather than from
        // literals that could drift from them.
        check(/ParameterID\s*\{\s*"tukeyTaper"[\s\S]{0,400}?WindowLut::kTukeyTaperMin[\s\S]{0,200}?WindowLut::kTukeyTaperMax[\s\S]{0,200}?WindowLut::kTukeyTaperStep/.test(processorCpp),
            'tukeyTaper range/step come from the WindowLut constants, not literals');
    }

    delete NOOP_DEFAULTS.grainCount;   // asserted above, by binding not by value
    delete NOOP_DEFAULTS.tukeyTaper;   // ditto

    for (const [id, expected] of Object.entries(NOOP_DEFAULTS)) {
        // Tolerates both `…range), 0.0f,` (attributes follow) and
        // `…range), 0.5f))` (no attributes) — grainTilt carries no unit label,
        // so its declaration ends differently from every neighbour's.
        const decl = processorCpp.match(
            new RegExp(`ParameterID\\s*\\{\\s*"${id}",\\s*1\\s*\\}[\\s\\S]*?\\),\\s*([0-9.]+)f[,)]`));
        check(!!decl && Number(decl[1]) === expected,
            `${id} defaults to ${expected} (existing sessions and presets must be unchanged)`
            + (decl ? ` — got ${decl[1]}` : ' — declaration not found'));
    }

    // v1.6.0 — freeze is a BOOL, so its default is a trailing `false` rather than
    // a float, and the numeric scan above cannot see it. Asserted separately for
    // the same reason every other new parameter's default is: a v1.0-v1.5 session
    // has no key here, resolves to this, and must not arrive frozen.
    {
        const decl = processorCpp.match(
            /AudioParameterBool>\(\s*\n?\s*juce::ParameterID\s*\{\s*"freeze",\s*1\s*\}[\s\S]{0,120}?,\s*(true|false)\)\);/);
        check(!!decl && decl[1] === 'false',
            'freeze defaults to false (an existing session must not open held)'
            + (decl ? ` — got ${decl[1]}` : ' — declaration not found'));
    }

    // v1.6.0 — regenMakeup's ceiling is a MEASURED stability bound, not a taste,
    // so the parameter range must come FROM the named constant. A literal here
    // that drifted above kRegenMakeupMaxDb would let the knob reach a setting the
    // loop-stability probe never measured — and the failure is a clipped output
    // at extreme settings, which nothing else in the suite would report.
    {
        const procH = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.h'), 'utf8');
        check(/ParameterID\s*\{\s*"regenMakeup",\s*1\s*\}[\s\S]{0,300}?kRegenMakeupMaxDb/.test(processorCpp),
            'regenMakeup\'s range max is kRegenMakeupMaxDb, not a literal');
        check(/regenMakeupGain[\s\S]{0,200}?dB <= 0\.0f[\s\S]{0,60}?return 1\.0f;/.test(procH),
            'regenMakeupGain returns exactly 1.0f at 0 dB (the bitwise no-op)');
    }

    // grainShape is a choice, so its default is a trailing INDEX rather than a
    // float. Index 0 must be Hann: an absent key in a v1.0/v1.1 preset resolves
    // to 0, so reordering WindowLut::Shape would re-voice shipped work even
    // though every default here still read "0".
    {
        const shapeDecl = processorCpp.match(
            /ParameterID\s*\{\s*"grainShape",\s*1\s*\}[\s\S]*?juce::StringArray\s*\{([\s\S]*?)\},\s*(\d+)\)/);
        check(!!shapeDecl && Number(shapeDecl[2]) === 0,
            'grainShape defaults to index 0'
            + (shapeDecl ? ` — got ${shapeDecl[2]}` : ' — declaration not found'));
        check(!!shapeDecl && /^\s*"Hann"/.test(shapeDecl[1]),
            'grainShape index 0 is "Hann" (the shipped v1.0/v1.1 window)');

        // The C++ StringArray is the single source of truth; the stub mirrors it
        // only so the page renders. Drift here means the browser render under
        // test shows options the plugin does not have.
        const stubShape = fs.readFileSync(
            path.join(pluginRoot, 'tests', 'ui-stub', 'juce-stub.js'), 'utf8')
            .match(/grainShape:\s*\[([\s\S]*?)\]/);
        const names = (s) => (s ? [...s.matchAll(/"([^"]+)"/g)].map(m => m[1]) : []);
        check(JSON.stringify(names(shapeDecl && shapeDecl[1])) === JSON.stringify(names(stubShape && stubShape[1]))
              && names(shapeDecl && shapeDecl[1]).length === 5,
            'ui-stub grainShape choices match the C++ StringArray (5 entries)');
    }

    // The stub renders the real page in a browser; a stale range there means the
    // rendered readout disagrees with the plugin and the render proves nothing.
    const stub = fs.readFileSync(path.join(pluginRoot, 'tests', 'ui-stub', 'juce-stub.js'), 'utf8');
    const stubBlock = stub.match(/const RANGES = \{([\s\S]*?)\n\};/);
    const stubIds = new Set();
    if (stubBlock)
        for (const m of stubBlock[1].matchAll(/^\s*([A-Za-z0-9_]+)\s*:/gm)) stubIds.add(m[1]);
    check(diff(jsIds, stubIds).length === 0,
        'the ui-stub declares a range for every knob'
        + (diff(jsIds, stubIds).length ? ' — MISSING: ' + diff(jsIds, stubIds).join(', ') : ''));
    check(/delayTime:\s*\{\s*start:\s*50,\s*end:\s*4000/.test(stub),
        'ui-stub delayTime range tracks the v1.0.1 widening (50-4000, not 50-2000)');

    // v1.5.0: grainSize's max AND skew centre both moved (500->4000, 158->316).
    // Asserted against the C++ CONSTANTS rather than against literals repeated
    // here, because a literal in the test drifts exactly as silently as the
    // literal in the stub did — this check exists because the stub sat at
    // 50-500 after the range moved, which would have made every browser-rendered
    // readout disagree with the plugin while the suite stayed green.
    {
        const procH = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.h'), 'utf8');
        const cppNum = (name) => {
            const m = procH.match(new RegExp(name + '\\s*=\\s*([0-9.]+)f'));
            return m ? parseFloat(m[1]) : null;
        };
        const gMin    = cppNum('kGrainSizeMinMs');
        const gMax    = cppNum('kGrainSizeMaxMs');
        const gCentre = cppNum('kGrainSizeSkewCentreMs');

        const stubGrain = stub.match(
            /grainSize:\s*\{\s*start:\s*([0-9.]+),\s*end:\s*([0-9.]+),\s*skew:\s*skewForCentre\(\s*([0-9.]+),\s*([0-9.]+),\s*([0-9.]+)\)/);

        check(gMin !== null && gMax !== null && gCentre !== null && stubGrain !== null
                && parseFloat(stubGrain[1]) === gMin
                && parseFloat(stubGrain[2]) === gMax
                && parseFloat(stubGrain[3]) === gMin
                && parseFloat(stubGrain[4]) === gMax
                && parseFloat(stubGrain[5]) === gCentre,
            'ui-stub grainSize range/skew match the C++ kGrainSize* constants'
                + (stubGrain && gMax !== null
                     ? ` — C++ ${gMin}-${gMax} c${gCentre}, stub ${stubGrain[1]}-${stubGrain[2]} c${stubGrain[5]}`
                     : ' — could not parse'));
    }
}

console.log(failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`);
process.exit(failed);
