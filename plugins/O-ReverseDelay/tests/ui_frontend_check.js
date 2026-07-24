/*
  ==============================================================================

    ui_frontend_check.js
    O-ReverseDelay — Stage 3 WebView frontend regression checks.
    Ported from plugins/O-Contrabass/tests/ui_frontend_check.js; adapted for an
    EXTERNAL js/app.js module (not an inline <script>), no preset/tuning
    modules, and a single-function native bridge.

    Manual run:  node plugins/O-ReverseDelay/tests/ui_frontend_check.js
    Exit code = number of failed assertions (0 = all pass).

    The frontend has no executable test harness (it runs inside the plugin's
    WKWebView), so this pins the silent-failure classes statically — every one
    of these ships green through ninja, auval AND pluginval:

      1. app.js parses (node --check); a SyntaxError kills the ENTIRE UI.
      2. TDZ discipline — a single init() call as the LAST statement, so no
         top-level initializer can reach a not-yet-initialised binding
         (pattern_module_toplevel_init_tdz).
      3. JS↔C++ bridge closure — every getNativeFunction name is registered
         with withNativeFunction, and vice versa
         (pattern_webview_native_fn_bridge_gap). Expected surface: exactly 1.
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

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const pluginRoot = path.resolve(__dirname, '..');

const publicDir     = path.join(pluginRoot, 'Source', 'ui', 'public');
const htmlPath      = path.join(publicDir, 'index.html');
const appJsPath     = path.join(publicDir, 'js', 'app.js');
const editorCppPath = path.join(pluginRoot, 'Source', 'PluginEditor.cpp');
const editorHPath   = path.join(pluginRoot, 'Source', 'PluginEditor.h');
const cmakePath     = path.join(pluginRoot, 'CMakeLists.txt');
const harnessCMake  = path.join(pluginRoot, 'tests', 'render-harness', 'CMakeLists.txt');

const html      = fs.readFileSync(htmlPath, 'utf8');
const appJs     = fs.readFileSync(appJsPath, 'utf8');
const editorCpp = fs.readFileSync(editorCppPath, 'utf8');
const editorH   = fs.readFileSync(editorHPath, 'utf8');
const cmake     = fs.readFileSync(cmakePath, 'utf8');
const harness   = fs.readFileSync(harnessCMake, 'utf8');

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
    const called = new Set();
    for (const m of appJs.matchAll(/getNativeFunction\(\s*["']([A-Za-z0-9_]+)["']/g))
        called.add(m[1]);

    const registered = new Set();
    for (const m of editorCpp.matchAll(/withNativeFunction\s*\(\s*(?:juce::Identifier\()?\s*"([A-Za-z0-9_]+)"/g))
        registered.add(m[1]);

    const missing = [...called].filter(n => !registered.has(n));
    const dead    = [...registered].filter(n => !called.has(n));

    check(missing.length === 0,
        `all ${called.size} native fns called from JS are registered in PluginEditor.cpp`
        + (missing.length ? ' — MISSING: ' + missing.join(', ') : ''));
    check(dead.length === 0,
        'no dead C++ registrations (registered but never called from JS)'
        + (dead.length ? ' — DEAD: ' + dead.join(', ') : ''));
    check(called.size === 1 && registered.size === 1,
        `bridge surface is exactly 1 fn (getParameterDefaults) — got JS=${called.size} C++=${registered.size}`);
    check(called.has('getParameterDefaults') && registered.has('getParameterDefaults'),
        'getParameterDefaults is called by the JS AND registered in C++');
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
    refs.add('/js/juce/check_native_interop.js');

    const provided = new Set();
    for (const m of editorCpp.matchAll(/url == "([^"]+)"/g)) provided.add(m[1]);

    const missingProvider = [...refs].filter(r => !provided.has(r));
    check(missingProvider.length === 0,
        `all ${refs.size} local UI references have resource-provider entries`
        + (missingProvider.length ? ' — MISSING: ' + missingProvider.join(', ') : ''));

    // Every provider entry (minus the "/" alias) must be a real embedded file.
    const binaryBlock = cmake.match(/juce_add_binary_data\([\s\S]*?\)/);
    check(!!binaryBlock, 'juce_add_binary_data block found in CMakeLists.txt');
    if (binaryBlock) {
        const embedded = new Set();
        for (const m of binaryBlock[0].matchAll(/Source\/ui\/public\/(\S+)/g))
            embedded.add('/' + m[1]);

        const notEmbedded = [...provided].filter(p => p !== '/' && p !== '/index.html' && !embedded.has(p));
        check(notEmbedded.length === 0,
            'every getResource() path is in the juce_add_binary_data SOURCES list'
            + (notEmbedded.length ? ' — MISSING: ' + notEmbedded.join(', ') : ''));

        const notServed = [...embedded].filter(e => e !== '/index.html' && !provided.has(e));
        check(notServed.length === 0,
            'every embedded UI file is served by getResource()'
            + (notServed.length ? ' — UNSERVED: ' + notServed.join(', ') : ''));

        // The embedded files must actually exist on disk.
        const absent = [...embedded].filter(e => !fs.existsSync(path.join(publicDir, e.slice(1))));
        check(absent.length === 0,
            'every juce_add_binary_data source exists on disk'
            + (absent.length ? ' — ABSENT: ' + absent.join(', ') : ''));

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

    const processorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.cpp'), 'utf8');
    check(/#if JUCE_WEB_BROWSER\s*\n\s*#include "PluginEditor\.h"/.test(processorCpp),
        'PluginEditor.h is included only inside an #if JUCE_WEB_BROWSER guard');

    const processorH = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.h'), 'utf8');
    check(!/PluginEditor\.h/.test(processorH),
        'PluginProcessor.h stays free of editor includes');
}

console.log(failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`);
process.exit(failed);
