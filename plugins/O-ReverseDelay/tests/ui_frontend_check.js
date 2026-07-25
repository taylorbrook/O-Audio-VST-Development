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
     14. All 10 controls carry tooltip copy, and showTooltip pins the measured
         width BEFORE placing (pattern_fixed_tooltip_shrink_to_fit_edge) — the
         `mix` knob is the control this would otherwise shrink-wrap.

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const path = require('path');
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
    // fetches exactly one fn (getParameterDefaults); the other ten are fetched
    // by the shared js/preset-manager.js, which app.js imports dynamically.
    // Scanning app.js alone would read 1 vs 11 and false-FAIL on correct code.
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
    check(called.size === 11 && registered.size === 11,
        `bridge surface is exactly 11 fns (getParameterDefaults + 10 preset)`
        + ` — got JS=${called.size} C++=${registered.size}`);
    check(called.has('getParameterDefaults') && registered.has('getParameterDefaults'),
        'getParameterDefaults is called by the JS AND registered in C++');

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

    const processorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.cpp'), 'utf8');
    check(/#if JUCE_WEB_BROWSER\s*\n\s*#include "PluginEditor\.h"/.test(processorCpp),
        'PluginEditor.h is included only inside an #if JUCE_WEB_BROWSER guard');

    const processorH = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.h'), 'utf8');
    check(!/PluginEditor\.h/.test(processorH),
        'PluginProcessor.h stays free of editor includes');
}

// ------------------------------------------- 12. Stage 4: preset bar geometry
{
    check(/setSize\s*\(\s*940\s*,\s*484\s*\)/.test(editorCpp),
        'editor setSize is 940 x 484');
    const heights = css.match(/height:\s*484px/g) || [];
    check(heights.length >= 2,
        `styles.css declares 484px in BOTH html/body and .frame — found ${heights.length}`);
    check(!/height:\s*440px/.test(css),
        'no 440px height survives in styles.css (the Stage-3 value)');

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

    // Button copy is content and lives in HTML. A shared JS updater writing
    // textContent is what left O-MultiBandCompressor's band buttons reading
    // "Off Off Off" in every DAW since launch.
    check(/id="preset-delete"[\s\S]{0,200}?data-label="Delete"/.test(html)
       && /id="preset-delete"[\s\S]{0,200}?data-confirm="/.test(html),
        '#preset-delete carries data-label and data-confirm (copy authored in HTML)');
    check(/btn\.dataset\.(confirm|label)/.test(appJs),
        'the delete button restores its label from data-attrs, never a JS literal');

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
        'knob-lowCut', 'knob-highCut', 'knob-width', 'knob-mix'];

    const missingTips = TIP_ANCHORS.filter(id => {
        const m = html.match(new RegExp(`id="${id}"[\\s\\S]{0,400}?>`));
        return !m || !/data-tip=/.test(m[0]) || !/data-tip-title=/.test(m[0]);
    });
    check(missingTips.length === 0,
        `all ${TIP_ANCHORS.length} controls carry data-tip + data-tip-title`
        + (missingTips.length ? ' — MISSING: ' + missingTips.join(', ') : ''));

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

    // D13 scoped this to hover help only: no toggle, no persisted state, and so
    // no 12th native function.
    check(!/setTooltipsEnabled/.test(appJs) && !/setTooltipsEnabled/.test(editorCpp),
        'no tooltip-enable native fn (D13: tooltips only, the bridge stays at 11)');
}

console.log(failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`);
process.exit(failed);
