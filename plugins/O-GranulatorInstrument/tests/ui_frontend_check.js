/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
    O-GranulatorInstrument — WebView frontend static closure (Stage 3 /
    Phase 3.1, PLAN-3.1 PD18, RESEARCH-3.1 §7).

    Copied from plugins/O-Granulator/tests/ui_frontend_check.js and adapted:
    261 parameters through three editor ID tables (176 / 70 / 15), ten page
    binders (knob, fader, lane handle, root field, choice knob, segment row,
    <select>, rec_target radio, scale trigger, toggle), 34 native functions
    spread over the page + preset-manager.js + webview-drop-streaming.js, a
    drop-session manager between the relays and the WebView.

    Every class pinned here ships green through ninja, auval and pluginval:

      1. The inline module parses (node --check); a SyntaxError kills the UI.
      2. TDZ discipline: exactly one top-level init() call, the LAST statement.
      3. Bridge closure: native names used by the page (getNativeFunction,
         callNative, slotAction) + preset-manager.js + webview-drop-streaming.js
         == withNativeFunction names == 34, both directions, none twice; every
         async dialog hoists a SafePointer and bare-returns on a dead editor.
      4. The `Juce` ES-module namespace is imported and passed to every binder,
         to the preset manager and to the drop module.
      5. No viewport units anywhere in the page.
      6. Resource closure: every juce_add_binary_data SOURCE of the UI target is
         served by exactly ONE getResource() branch, every branch serves exactly
         one SOURCE, and the url's file name is the source's; bare paths only.
      7. Source/ui/public/modules/* is not tracked.
      8. Four-way closure by ID AND TYPE, both directions, no duplicates: fresh
         param-dump == committed .planning/params.tsv == editor kSliderIds /
         kComboIds / kToggleIds (relay + attachment class per table) == the ids
         the page's init() binds; every page id has its element and (floats) a
         FORMAT entry.
      9. Editor member order relays x3 -> dropSessions -> webView ->
         attachments x3 -> webView2RuntimeLink; the constructor creates them in
         the same order; every webView-> use outside the constructor is
         null-guarded; never setVisible(false) on it.
     10. Windows block: withBackend (webview2), withUserDataFolder (created
         first, never %TEMP%), areOptionsSupported probe; the CMake WebView flags.
     11. Geometry: setSize (1200, 800) + setResizable (false, false) == body and
         #plugin 1200 x 800 px.
     12. Telemetry keys the LIVE telemetryToVar (const UiTelemetry&) emits
         (Source/GiUiJson.h; PLAN-3.3 PD14 — the frozen 3.1 placeholder is no
         longer the key source) are a subset of the keys the page reads, per
         object (top level, lanes[], rec); the stub fixture's getTelemetry has
         the same top-level keys; one reader per consuming feed (PD15): the
         editor calls buildUiTelemetry once and reads no feed itself, and in
         PluginProcessor.cpp every frames.read / takeOutPeaks / takeScPeaks /
         stolenCount read sits inside buildUiTelemetry.
     13. Headless isolation: the render harness never compiles the editor; the
         processor includes / creates it only under JUCE_WEB_BROWSER.
     14. No DevLoader / OGI_DEV_LOADER token under Source/ (ui/public/modules
         excluded) or in CMakeLists.txt.
     15. Phase 3.2 closure (PLAN-3.2 PD26): every 3.2 key is EMITTED by its
         C++ encoder (Source/GiUiJson.h: the UiTelemetry telemetryToVar,
         slotInfoToVar, slotOverviewToVar) AND READ by the page function that
         uses it; no "unavailable" answer left in the editor; the three lane
         natives are live; both failure toasts exist in en / fr / zh-Hans and
         are raised by the page. Amended at 3.3 (PD14): the live encoder emits
         14 top-level / 3 + lo, hi lane / 8 rec keys and the frozen 3.1
         placeholder's set is a subset of it, the difference exactly lfoPh (top)
         and lo, hi (lane).
     16. Phase 3.3 closure (PLAN-3.3 PD23): every 3.3 key / column / bit is
         EMITTED by the live encoder AND READ by its page function; frozen /
         modRouted / stale are not emitted; the page's mono rule
         (PER_VOICE_SOURCES, isPerVoiceSource, isGlobalDest) equals the
         transcription the harness row UMB1 tests the engine against
         (tests/render-harness/ui_gates_33.h, namespace pageRule) for all
         17 x 125 x LFO-mode pairs; the LFO dot uses the phase offset (PD16);
         class gate (D49a): no state in tests/i18n-states.json calls __setLabel
         on a node a page reader writes — the reader-written ids are DERIVED
         from the page (every setLabel target inside the functions reachable
         from the setInterval / requestAnimationFrame roots).

    Usage (from anywhere):
      node plugins/O-GranulatorInstrument/tests/ui_frontend_check.js
      node plugins/O-GranulatorInstrument/tests/ui_frontend_check.js --negative-controls   (alias --nc)
    Exit code = number of failed assertions (0 = all pass). With
    --negative-controls the script re-runs itself on four scratch copies (an
    editor with one ID moved kSliderIds -> kToggleIds; an editor with one
    withNativeFunction removed; a GiUiJson.h whose live encoder emits "grains"
    under another name; a GiUiJson.h whose live encoder no longer emits
    lfoPh) and exits 0 only if EVERY copy fails the check that targets it.
    Scratch copies live under os.tmpdir() (set TMPDIR to move them) and are
    removed; the tree is never written.
    --editor-cpp <path>  point sections 3 / 6 / 8 / 9 / 10 / 12 / 15 at another PluginEditor.cpp
    --json-h <path>      point sections 12 / 13 / 15 / 16 at another GiUiJson.h
    --page <path>        point the page checks at another index.html
    --param-dump <path>  override the param-dump binary (default: the build tree;
                         a missing binary is retried for 90 s, never built here)

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const PLUGIN     = 'O-GranulatorInstrument';
const S = require(path.join(repoRoot, 'scripts', 'serve-ui.js'));   // CMake SOURCES + BinaryData symbol rule (read-only use)

const argv = process.argv.slice(2);
const argVal = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };

const htmlPath      = argVal('--page') || path.join(pluginRoot, 'Source', 'ui', 'public', 'index.html');
const editorCppPath = argVal('--editor-cpp') || path.join(pluginRoot, 'Source', 'PluginEditor.cpp');
const editorHPath   = path.join(pluginRoot, 'Source', 'PluginEditor.h');
const jsonHPath     = argVal('--json-h') || path.join(pluginRoot, 'Source', 'GiUiJson.h');
const i18nPath      = path.join(pluginRoot, 'Source', 'ui', 'public', 'js', 'i18n.js');
const procCppPath   = path.join(pluginRoot, 'Source', 'PluginProcessor.cpp');
const cmakePath     = path.join(pluginRoot, 'CMakeLists.txt');
const harnessPath   = path.join(pluginRoot, 'tests', 'render-harness', 'main.cpp');
const tsvPath       = path.join(pluginRoot, '.planning', 'params.tsv');
const overridesPath = path.join(pluginRoot, 'tests', 'ui-stub', 'generic-overrides.json');
const paramDumpBin  = argVal('--param-dump') || path.join(repoRoot, 'build', 'plugins', PLUGIN,
    `${PLUGIN}-param-dump_artefacts`, 'Release', `${PLUGIN}-param-dump`);

const N_FLOAT = 176, N_CHOICE = 70, N_BOOL = 15, N_PARAMS = 261, N_NATIVE = 34;

// ───────────────────────────────────────────── negative controls (PD18) ──
if (argv.includes('--negative-controls') || argv.includes('--nc')) {
    const sources = { editor: fs.readFileSync(editorCppPath, 'utf8'), jsonH: fs.readFileSync(jsonHPath, 'utf8') };
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'ogi-ufc-nc-'));
    const variants = [
        {
            name: 'NC1 one ID moved kSliderIds -> kToggleIds (glide)',
            expect: /FAIL: \[8\]/,
            make: (s) => s.replace(/(kSliderIds \{[\s\S]*?)\n\s*"glide",/, '$1')
                          .replace(/(kToggleIds \{\n)/, '$1        "glide",\n'),
        },
        {
            name: 'NC2 one withNativeFunction removed (getWindowCurve)',
            expect: /FAIL: \[3\]/,
            make: (s) => s.replace('withNativeFunction ("getWindowCurve"', 'withNativeFunction_REMOVED ("getWindowCurve"'),
        },
        {
            // PLAN-3.2 PD26: the page keeps reading t.grains, the encoder sends another name.
            name: 'NC3 one telemetry key renamed in the live encoder (grains -> grainRows)',
            file: 'jsonH',
            expect: /FAIL: \[15\] emitted and read: grains\b/,
            make: (s) => s.replace('obj->setProperty ("grains", grains);', 'obj->setProperty ("grainRows", grains);'),
        },
        {
            // PLAN-3.3 PD23: the page keeps reading t.lfoPh, the encoder stops sending it.
            name: 'NC4 lfoPh removed from the live encoder',
            file: 'jsonH',
            expect: /FAIL: \[16\] emitted and read: lfoPh\b/,
            make: (s) => s.replace('obj->setProperty ("lfoPh", lfoPh);', '/* lfoPh not emitted (NC4) */'),
        },
    ];
    let bad = 0;
    console.log(`== ${PLUGIN} ui_frontend_check — negative controls ==`);
    for (const v of variants) {
        const which = v.file || 'editor';
        const src = sources[which];
        const mutated = v.make(src);
        if (mutated === src) { console.log(`  FAIL: ${v.name} — the mutation did not apply`); ++bad; continue; }
        const p = path.join(dir, which === 'jsonH' ? 'GiUiJson.h' : 'PluginEditor.cpp');
        fs.writeFileSync(p, mutated);
        const pass = argv.filter((a, i) => ['--page', '--param-dump'].includes(a) || ['--page', '--param-dump'].includes(argv[i - 1]));
        const r = spawnSync(process.execPath, [__filename, which === 'jsonH' ? '--json-h' : '--editor-cpp', p, ...pass], { encoding: 'utf8', maxBuffer: 1 << 26 });
        const failedTarget = v.expect.test(r.stdout);
        const fails = (r.stdout.match(/^ {2}FAIL: .*$/gm) || []);
        const ok = r.status !== 0 && failedTarget;
        console.log(`  ${ok ? 'PASS' : 'FAIL'}: ${v.name} -> exit ${r.status}, targeted check ${failedTarget ? 'FAILED (as it must)' : 'did NOT fail'}`);
        for (const f of fails.slice(0, 4)) console.log(`        ${f.trim().slice(0, 170)}`);
        if (!ok) ++bad;
    }
    fs.rmSync(dir, { recursive: true, force: true });
    console.log(bad === 0 ? `negative controls: all ${variants.length} copies FAIL the closure — the gate is live`
                          : `negative controls: ${bad} did not fail`);
    process.exit(bad);
}

const html      = fs.readFileSync(htmlPath, 'utf8');
const editorCpp = fs.readFileSync(editorCppPath, 'utf8');
const editorH   = fs.readFileSync(editorHPath, 'utf8');
const jsonH     = fs.readFileSync(jsonHPath, 'utf8');
const procCpp   = fs.readFileSync(procCppPath, 'utf8');
const cmake     = fs.readFileSync(cmakePath, 'utf8');
const harness   = fs.readFileSync(harnessPath, 'utf8');
// C/C++ source with comments removed (for "never mentions X" checks).
const stripC = (src) => src.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}
const setDiff = (a, b) => [...a].filter((x) => !b.has(x));
const dups = (arr) => arr.filter((x, i) => arr.indexOf(x) !== i);
const show = (arr) => (arr.length ? ` — ${arr.slice(0, 6).join(', ')}${arr.length > 6 ? ', …' : ''}` : '');
const range = (n) => Array.from({ length: n }, (_, i) => i + 1);

console.log(`== ${PLUGIN} ui_frontend_check ==`);
console.log(`   editor: ${path.relative(repoRoot, editorCppPath)}`);
console.log(`   page:   ${path.relative(repoRoot, htmlPath)}`);

// The inline controller (the only <script type="module">).
const modMatch = html.match(/<script type="module">([\s\S]*?)<\/script>/);
const moduleSrc = modMatch ? modMatch[1] : '';

// The UI binary-data target's SOURCES, as the build embeds them.
const uiSources = S.binaryDataSources(PLUGIN).filter((s) => !/\.flac$/i.test(s.abs));
const sourceByBase = (base) => uiSources.find((s) => path.basename(s.abs) === base);

// ───────────────────────────────────────────────────────────── 1. syntax ──
console.log('-- [1] syntax');
{
    check(!!modMatch && (html.match(/^<script type="module">/gm) || []).length === 1,
        '[1] index.html has exactly one inline <script type="module">');
    const res = spawnSync(process.execPath, ['--input-type=module', '--check'], { input: moduleSrc, encoding: 'utf8' });
    check(res.status === 0, '[1] the inline module parses (node --check)' + (res.status !== 0 ? `\n${res.stderr}` : ''));
}

// ─────────────────────────────────────────────────────────────── 2. TDZ ──
console.log('-- [2] TDZ discipline');
{
    const calls = moduleSrc.match(/^\s*init\(\);\s*$/gm) || [];
    check(calls.length === 1, `[2] exactly one top-level init() call (found ${calls.length})`);
    const lastStmt = moduleSrc.replace(/\/\/[^\n]*$/gm, '').trim().split('\n').pop().trim();
    check(lastStmt === 'init();', `[2] init() is the LAST statement of the module (last: "${lastStmt}")`);
}

// ─────────────────────────────────────────────────── 3. bridge closure ──
console.log('-- [3] native-function bridge closure');
{
    const pmSrc = sourceByBase('preset-manager.js'), dropSrc = sourceByBase('webview-drop-streaming.js');
    check(!!pmSrc && pmSrc.exists && !!dropSrc && dropSrc.exists,
        '[3] preset-manager.js and webview-drop-streaming.js are embedded SOURCES that exist');
    const presetJs = pmSrc && pmSrc.exists ? fs.readFileSync(pmSrc.abs, 'utf8') : '';
    const dropJs   = dropSrc && dropSrc.exists ? fs.readFileSync(dropSrc.abs, 'utf8') : '';

    // The page reaches natives three ways: a literal getNativeFunction('x'),
    // callNative('x', …) and slotAction('x', …) (both funnel into getNativeFunction(name)).
    const direct = [...moduleSrc.matchAll(/getNativeFunction\(\s*'([A-Za-z]+)'\s*\)/g)].map((m) => m[1]);
    const viaCall = [...moduleSrc.matchAll(/\b(?:callNative|slotAction)\(\s*'([A-Za-z]+)'/g)].map((m) => m[1]);
    const pageNames = new Set([...direct, ...viaCall]);
    const funnel = /async function callNative\(name, arg\) \{\s*\n\s*try \{ return await Juce\.getNativeFunction\(name\)\(arg\); \}/.test(moduleSrc)
                && /async function slotAction\(name, n, extra\) \{\s*\n\s*const r = await callNative\(name,/.test(moduleSrc);
    check(funnel, '[3] callNative / slotAction funnel into Juce.getNativeFunction(name)');
    // Any other non-literal getNativeFunction( call would be a name this scan cannot see.
    const nonLiteral = [...moduleSrc.matchAll(/getNativeFunction\(\s*([^')\s][^)]*)\)/g)].map((m) => m[1]).filter((a) => a !== 'name');
    check(nonLiteral.length === 0, '[3] no other computed native name in the page' + show(nonLiteral));

    const pmNames   = new Set([...presetJs.matchAll(/getNativeFunction\(\s*"([A-Za-z]+)"\s*\)/g)].map((m) => m[1]));
    const dropNames = new Set([...stripC(dropJs).matchAll(/getNativeFunction\(\s*'([A-Za-z]+)'\s*\)/g)].map((m) => m[1]));
    const used = new Set([...pageNames, ...pmNames, ...dropNames]);
    const regList = [...editorCpp.matchAll(/\.withNativeFunction\s*\(\s*"([A-Za-z]+)"/g)].map((m) => m[1]);
    const reg = new Set(regList);
    const anyReg = (editorCpp.match(/\.withNativeFunction\s*\(/g) || []).length;
    check(pageNames.size === 20, `[3] page uses 20 native fns (found ${pageNames.size})`);
    check(pmNames.size === 10, `[3] preset-manager.js uses 10 native fns (found ${pmNames.size})`);
    check(dropNames.size === 4, `[3] webview-drop-streaming.js uses 4 native fns (found ${dropNames.size})`);
    check(pageNames.size + pmNames.size + dropNames.size === used.size, '[3] the three name sets are disjoint');
    check(anyReg === regList.length, `[3] every withNativeFunction registers a LITERAL name (${regList.length} of ${anyReg})`);
    check(dups(regList).length === 0, '[3] no native fn registered twice' + show(dups(regList)));
    const unreg = setDiff(used, reg), unused = setDiff(reg, used);
    check(unreg.length === 0, '[3] every page / preset-manager / drop-streaming fn is registered (withNativeFunction)' + show(unreg));
    check(unused.length === 0, '[3] every registered fn is used by the page, preset-manager.js or webview-drop-streaming.js' + show(unused));
    check(reg.size === N_NATIVE && used.size === N_NATIVE, `[3] surface is exactly ${N_NATIVE} = ${N_NATIVE} (registered ${reg.size}, used ${used.size})`);

    // Async file dialogs: SafePointer hoisted to a local; bare return on a dead editor.
    const launches = (editorCpp.match(/launchAsync\s*\(/g) || []).length;
    const hoisted  = (editorCpp.match(/auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> \(this\);/g) || []).length;
    const bare     = (editorCpp.match(/if \(safeThis == nullptr\)\s*\n?\s*return;/g) || []).length;
    check(launches >= 1 && hoisted === launches && bare === launches,
        `[3] ${launches} launchAsync dialogs: SafePointer hoisted (${hoisted}) and bare-return on null (${bare})`);
}

// ─────────────────────────────────────────────────── 4. Juce namespace ──
console.log('-- [4] Juce namespace');
{
    check(/import \* as Juce from '\.\/js\/juce\/index\.js';/.test(moduleSrc), "[4] import * as Juce from './js/juce/index.js'");
    const calls = ['KNOB_IDS.forEach((id) => bindKnob(Juce, id));', 'CHOICE_KNOB_IDS.forEach((id) => bindChoiceKnob(Juce, id));',
                   'HANDLE_IDS.forEach((id) => bindHandle(Juce, id));', 'FIELD_IDS.forEach((id) => bindField(Juce, id));',
                   'TOGGLE_IDS.forEach((id) => bindToggle(Juce, id));', 'SEGMENT_IDS.forEach((id) => bindSegmented(Juce, id));',
                   "bindSelectCombo(Juce, 'grain_limit',", "bindRadio(Juce, 'rec_target');", 'initMatrix(Juce);',
                   'initScaleBand(Juce, n);', 'initTelemetry(Juce);', 'loadParameterDefaults(Juce);'];
    const missing = calls.filter((c) => !moduleSrc.includes(c));
    check(missing.length === 0, '[4] every binder receives the Juce namespace' + show(missing));
    check(/getNativeFunction: Juce\.getNativeFunction,/.test(moduleSrc), '[4] the preset manager gets Juce.getNativeFunction');
    check(/juce: Juce,/.test(moduleSrc), '[4] the drop module gets the ES-module namespace (juce: Juce)');
    const code = moduleSrc.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
    check(!/window\.__JUCE__|postMessage\(/.test(code), '[4] no raw window.__JUCE__ / postMessage path in the controller');
}

// ───────────────────────────────────────────────────── 5. viewport units ──
console.log('-- [5] viewport units');
{
    const vp = html.match(/\b\d+(?:\.\d+)?(?:vh|vw|dvh|svh|lvh|dvw|svw|lvw|vmin|vmax)\b/g) || [];
    check(vp.length === 0, '[5] no viewport units in index.html' + show(vp));
}

// ─────────────────────────────────────────────────── 6. resource closure ──
console.log('-- [6] resource closure');
{
    check(uiSources.length === 10 && uiSources.every((s) => s.exists),
        `[6] 10 UI juce_add_binary_data SOURCES, all on disk (found ${uiSources.length})` + show(uiSources.filter((s) => !s.exists).map((s) => s.raw)));
    check(dups(uiSources.map((s) => s.symbol)).length === 0, '[6] no two SOURCES mangle to the same BinaryData symbol' + show(dups(uiSources.map((s) => s.symbol))));
    const fnStart = editorCpp.indexOf('GranulatorInstrumentEditor::getResource (const juce::String& url)\n{');
    const fnBody = fnStart >= 0 ? editorCpp.slice(fnStart, editorCpp.indexOf('\n}\n', fnStart)) : '';
    check(fnStart >= 0, '[6] getResource (const juce::String& url) found');
    // One branch = one `if (url == …)` statement up to the next.
    const branches = fnBody.split(/\n\s*if \(url == /).slice(1).map((b) => {
        const text = 'url == ' + b;
        return {
            urls: [...text.matchAll(/url == "([^"]+)"/g)].map((m) => m[1]),
            syms: [...new Set([...text.matchAll(/UIBinaryData::([A-Za-z0-9_]+)/g)].map((m) => m[1]).filter((s) => !/Size$/.test(s)))],
            sizes: [...new Set([...text.matchAll(/UIBinaryData::([A-Za-z0-9_]+)Size\b/g)].map((m) => m[1]))],
        };
    });
    check(branches.length === uiSources.length, `[6] ${branches.length} resource branches for ${uiSources.length} SOURCES`);
    const multi = branches.filter((b) => b.syms.length !== 1 || b.sizes.length !== 1 || b.sizes[0] !== b.syms[0]);
    check(multi.length === 0, '[6] every branch serves exactly one symbol with its own …Size' + show(multi.map((b) => b.urls.join('|'))));
    const perSource = uiSources.map((s) => ({ s, hits: branches.filter((b) => b.syms.includes(s.symbol)) }));
    const notOne = perSource.filter((x) => x.hits.length !== 1);
    check(notOne.length === 0, '[6] every SOURCE is served by exactly ONE branch' + show(notOne.map((x) => `${x.s.symbol} x${x.hits.length}`)));
    const want = new Set(uiSources.map((s) => s.symbol));
    const stray = branches.flatMap((b) => b.syms).filter((s) => !want.has(s));
    check(stray.length === 0, '[6] every branch serves a SOURCE' + show(stray));
    const misnamed = perSource.filter((x) => x.hits.length === 1
        && !x.hits[0].urls.some((u) => path.basename(u) === path.basename(x.s.abs)));
    check(misnamed.length === 0, '[6] each branch url ends in its SOURCE file name' + show(misnamed.map((x) => `${x.hits[0].urls[0]} -> ${x.s.symbol}`)));
    const urls = branches.flatMap((b) => b.urls);
    check(urls.length === 11 && dups(urls).length === 0 && urls.every((u) => u.startsWith('/')),
        `[6] 11 distinct bare-path comparisons ("/" + 10 files; found ${urls.length})` + show(dups(urls)));
    // Every url the page imports / links resolves to a branch.
    const refs = [...html.matchAll(/(?:from\s+|import\(\s*)'\.\/([^']+)'/g)].map((m) => '/' + m[1])
        .concat([...html.matchAll(/<link[^>]+href="([^"]+)"/g)].map((m) => (m[1].startsWith('/') ? m[1] : '/' + m[1].replace(/^\.\//, ''))));
    const unserved = [...new Set(refs)].filter((u) => !urls.includes(u));
    check(refs.length >= 5 && unserved.length === 0, `[6] every module / stylesheet the page loads has a branch (${[...new Set(refs)].length})` + show(unserved));
    check(!/juce:\/\/|https:\/\/juce\.backend/.test(stripC(editorCpp) + moduleSrc), '[6] no hard-coded juce:// or https://juce.backend scheme');
    check(/\.withResourceProvider \(\[this\] \(const auto& url\) \{ return getResource \(url\); \}\)/.test(editorCpp),
        '[6] withResourceProvider -> getResource');
}

// ───────────────────────────────────────────────── 7. modules untracked ──
console.log('-- [7] module copies untracked');
{
    const r = spawnSync('git', ['ls-files', `plugins/${PLUGIN}/Source/ui/public/modules`], { cwd: repoRoot, encoding: 'utf8' });
    check(r.status === 0 && r.stdout.trim() === '', '[7] Source/ui/public/modules/* is not tracked' + (r.stdout.trim() ? `: ${r.stdout.trim()}` : ''));
}

// ───────────────────────────────────────── 8. four-way closure, ID + TYPE ──
console.log('-- [8] four-way closure (param-dump == params.tsv == editor tables == page binders), by ID and TYPE');
{
    const parseTsv = (text) => {
        const out = new Map();
        let cols = null;
        for (const line of text.split('\n')) {
            if (!line.trim()) continue;
            if (line.startsWith('#')) {
                const maybe = line.replace(/^#\s*/, '').split('\t').map((c) => c.trim());
                if (maybe.includes('id') && maybe.includes('flags')) cols = maybe;
                continue;
            }
            const cells = line.split('\t');
            const id = cols ? cells[cols.indexOf('id')] : cells[0];
            const flags = cols ? cells[cols.indexOf('flags')] : cells[cells.length - 1];
            if (out.has(id)) out.set(`${id} (DUPLICATE)`, 'dup');
            out.set(id, /\bboolean\b/.test(flags) ? 'bool' : /\bdiscrete\b/.test(flags) ? 'choice' : 'float');
        }
        return out;
    };
    // The binary can be momentarily absent while a build relinks it: wait, never build.
    let fresh = null;
    const sleep = (ms) => Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, ms);
    for (let attempt = 0; attempt < 30 && fresh === null; ++attempt) {
        if (fs.existsSync(paramDumpBin)) {
            const r = spawnSync(paramDumpBin, [], { encoding: 'utf8', maxBuffer: 1 << 24 });
            if (r.status === 0 && r.stdout.length > 0) { fresh = r.stdout; break; }
        }
        sleep(3000);
    }
    check(fresh !== null, `[8] fresh ${PLUGIN}-param-dump ran (${path.relative(repoRoot, paramDumpBin)})`);
    const committed = fs.readFileSync(tsvPath, 'utf8');
    check(fresh !== null && fresh === committed, '[8] fresh param-dump == committed .planning/params.tsv (byte-identical)');
    const apvts = parseTsv(fresh !== null ? fresh : '');
    check(apvts.size === N_PARAMS, `[8] APVTS has ${N_PARAMS} parameters, no duplicate id (fresh dump: ${apvts.size})`);

    const table = (name) => {
        const m = editorCpp.match(new RegExp(`const juce::StringArray ${name} \\{([\\s\\S]*?)\\};`));
        return m ? [...m[1].matchAll(/"([a-z0-9_]+)"/g)].map((x) => x[1]) : [];
    };
    const ed = { float: table('kSliderIds'), choice: table('kComboIds'), bool: table('kToggleIds') };
    check(ed.float.length === N_FLOAT && ed.choice.length === N_CHOICE && ed.bool.length === N_BOOL,
        `[8] editor tables ${N_FLOAT} / ${N_CHOICE} / ${N_BOOL} (found ${ed.float.length} / ${ed.choice.length} / ${ed.bool.length})`);
    // Each table feeds the relay + attachment class of its type.
    check(/for \(const auto& id : kSliderIds\)\s*\n\s*sliderRelays\.push_back \(std::make_unique<juce::WebSliderRelay> \(id\)\);/.test(editorCpp)
       && /for \(const auto& id : kComboIds\)\s*\n\s*comboRelays\.push_back \(std::make_unique<juce::WebComboBoxRelay> \(id\)\);/.test(editorCpp)
       && /for \(const auto& id : kToggleIds\)\s*\n\s*toggleRelays\.push_back \(std::make_unique<juce::WebToggleButtonRelay> \(id\)\);/.test(editorCpp),
        '[8] kSliderIds -> WebSliderRelay, kComboIds -> WebComboBoxRelay, kToggleIds -> WebToggleButtonRelay');
    check(/kSliderIds\[i\]\);[\s\S]{0,300}?WebSliderParameterAttachment> \(\s*\*param, \*sliderRelays\[/.test(editorCpp)
       && /kComboIds\[i\]\);[\s\S]{0,300}?WebComboBoxParameterAttachment> \(\s*\*param, \*comboRelays\[/.test(editorCpp)
       && /kToggleIds\[i\]\);[\s\S]{0,300}?WebToggleButtonParameterAttachment> \(\s*\*param, \*toggleRelays\[/.test(editorCpp),
        '[8] each table attaches with its own attachment class to its own relay vector');
    for (const t of ['slider', 'combo', 'toggle'])
        check(new RegExp(`for \\(const auto& relay : ${t}Relays\\)\\s*\\n\\s*options = options\\.withOptionsFrom \\(\\*relay\\);`).test(editorCpp),
            `[8] every ${t} relay is registered with withOptionsFrom`);

    const arr = (name) => {
        const m = moduleSrc.match(new RegExp(`const ${name} = \\[([\\s\\S]*?)\\];`));
        return m ? [...m[1].matchAll(/'([a-z0-9_]+)'/g)].map((x) => x[1]) : [];
    };
    const slotsM = moduleSrc.match(/const MOD_SLOTS = (\d+);/);
    const slots = slotsM ? Number(slotsM[1]) : 0;
    const scaleIds = /function initScaleBand\(juce, n\) \{\s*\n\s*const id = `s\$\{n\}_scale`;\s*\n\s*const st = juce\.getComboBoxState\(id\);/.test(moduleSrc)
                  && /for \(let n = 1; n <= 4; n\+\+\) initScaleBand\(Juce, n\);/.test(moduleSrc)
        ? [1, 2, 3, 4].map((n) => `s${n}_scale`) : [];
    const matrixOk = /bindSelectCombo\(juce, `mod\$\{m\}_source`, SOURCE_LABELERS\);/.test(moduleSrc)
                  && /bindSelectCombo\(juce, `mod\$\{m\}_dest`, destLabelers\);/.test(moduleSrc)
                  && /bindFader\(juce, `mod\$\{m\}_depth`\);/.test(moduleSrc)
                  && /for \(let m = 1; m <= MOD_SLOTS; m\+\+\) \{\s*\n\s*bindSelectCombo/.test(moduleSrc);
    const pg = {
        knob: arr('KNOB_IDS'), fader: arr('FADER_IDS'), handle: arr('HANDLE_IDS'), field: arr('FIELD_IDS'),
        choiceKnob: arr('CHOICE_KNOB_IDS'), segment: arr('SEGMENT_IDS'), select: arr('SELECT_IDS'), toggle: arr('TOGGLE_IDS'),
    };
    const radioIds = moduleSrc.includes("bindRadio(Juce, 'rec_target');") ? ['rec_target'] : [];
    const matrixSelects = range(slots).flatMap((m) => [`mod${m}_source`, `mod${m}_dest`]);
    check(slots === 16 && matrixOk, `[8] initMatrix binds mod1..16 source / dest (select) + depth (fader) (MOD_SLOTS ${slots})`);
    check(scaleIds.length === 4, '[8] initScaleBand binds s1..s4_scale as ComboBoxStates');
    check(radioIds.length === 1, "[8] init() runs bindRadio(Juce, 'rec_target')");
    check(JSON.stringify(pg.fader) === JSON.stringify(range(16).map((m) => `mod${m}_depth`)),
        '[8] FADER_IDS == mod1..16_depth (the ids initMatrix fader-binds)');
    check(JSON.stringify(pg.select) === JSON.stringify(['grain_limit']) && moduleSrc.includes("bindSelectCombo(Juce, 'grain_limit',"),
        "[8] SELECT_IDS == ['grain_limit'] and init() select-binds it");
    // The binders init() actually calls for each table, and no binder call outside them.
    for (const [call, what] of [['KNOB_IDS.forEach((id) => bindKnob(Juce, id));', 'KNOB_IDS -> bindKnob'],
                                ['CHOICE_KNOB_IDS.forEach((id) => bindChoiceKnob(Juce, id));', 'CHOICE_KNOB_IDS -> bindChoiceKnob'],
                                ['HANDLE_IDS.forEach((id) => bindHandle(Juce, id));', 'HANDLE_IDS -> bindHandle'],
                                ['FIELD_IDS.forEach((id) => bindField(Juce, id));', 'FIELD_IDS -> bindField'],
                                ['TOGGLE_IDS.forEach((id) => bindToggle(Juce, id));', 'TOGGLE_IDS -> bindToggle'],
                                ['SEGMENT_IDS.forEach((id) => bindSegmented(Juce, id));', 'SEGMENT_IDS -> bindSegmented']])
        check(moduleSrc.includes(call), `[8] init() runs ${what}`);
    const binderCalls = (moduleSrc.match(/(?<!function )\bbind(?:Knob|ChoiceKnob|Handle|Field|Toggle|Segmented|SelectCombo|Fader|Radio)\((?:Juce|juce), /g) || []).length;
    check(binderCalls === 6 + 1 + 1 + 3, `[8] no binder call besides the ${6 + 1 + 1 + 3} accounted for (found ${binderCalls})`);
    const stateGets = (moduleSrc.match(/\.get(?:Slider|ComboBox|Toggle)State\(/g) || []).length;
    check(stateGets === 10, `[8] exactly 10 get*State call sites (one per binder: 9 bind* + initScaleBand; found ${stateGets})`);

    const page = {
        float: [...pg.knob, ...pg.fader, ...pg.handle, ...pg.field],
        choice: [...pg.choiceKnob, ...pg.segment, ...pg.select, ...radioIds, ...scaleIds, ...matrixSelects],
        bool: pg.toggle,
    };
    console.log(`   page binders: knob ${pg.knob.length}, fader ${pg.fader.length}, handle ${pg.handle.length}, field ${pg.field.length}, `
              + `choiceKnob ${pg.choiceKnob.length}, segment ${pg.segment.length}, select ${pg.select.length + matrixSelects.length}, `
              + `radio ${radioIds.length}, scale ${scaleIds.length}, toggle ${pg.toggle.length}`);
    const allPage = [...page.float, ...page.choice, ...page.bool];
    const allEd = [...ed.float, ...ed.choice, ...ed.bool];
    check(allPage.length === N_PARAMS && dups(allPage).length === 0, `[8] no page id bound twice (${allPage.length})` + show(dups(allPage)));
    check(allEd.length === N_PARAMS && dups(allEd).length === 0, `[8] no editor id listed twice (${allEd.length})` + show(dups(allEd)));
    for (const ty of ['float', 'choice', 'bool']) {
        const A = new Set([...apvts].filter(([, t]) => t === ty).map(([id]) => id));
        const E = new Set(ed[ty]), P = new Set(page[ty]);
        check(A.size > 0 && setDiff(A, E).length === 0 && setDiff(E, A).length === 0,
            `[8] ${ty}: APVTS == editor (${A.size} / ${E.size})` + show([...setDiff(A, E).map((x) => `-${x}`), ...setDiff(E, A).map((x) => `+${x}`)]));
        check(A.size > 0 && setDiff(A, P).length === 0 && setDiff(P, A).length === 0,
            `[8] ${ty}: APVTS == page (${A.size} / ${P.size})` + show([...setDiff(A, P).map((x) => `-${x}`), ...setDiff(P, A).map((x) => `+${x}`)]));
        check(setDiff(E, P).length === 0 && setDiff(P, E).length === 0,
            `[8] ${ty}: editor == page (${E.size} / ${P.size})` + show([...setDiff(E, P).map((x) => `-${x}`), ...setDiff(P, E).map((x) => `+${x}`)]));
    }
    // Registration order: the editor tables follow the APVTS order (the relays are filled by index).
    const order = [...apvts.keys()];
    for (const [ty, name] of [['float', 'kSliderIds'], ['choice', 'kComboIds'], ['bool', 'kToggleIds']])
        check(JSON.stringify(ed[ty]) === JSON.stringify(order.filter((id) => apvts.get(id) === ty)), `[8] ${name} is in APVTS registration order`);

    // Every page id has the element its binder looks up (exactly once).
    const count = (id) => html.split(`id="${id}"`).length - 1;
    const need = [
        ...[...pg.knob, ...pg.choiceKnob].flatMap((id) => [`knob-${id}`, `val-${id}`, `cell-${id}`]),
        ...pg.fader.flatMap((id) => [`fader-${id}`, `val-${id}`]),
        ...pg.handle.map((id) => `handle-${id}`),
        ...pg.field.map((id) => `field-${id}`),
        ...pg.toggle.map((id) => `toggle-${id}`),
        ...pg.segment.map((id) => `seg-${id}`),
        ...[...pg.select, ...matrixSelects].map((id) => `combo-${id}`),
        ...radioIds.map((id) => `radio-${id}`),
        ...[1, 2, 3, 4].flatMap((n) => [`scale-trig-s${n}`, `scale-name-s${n}`]),
    ];
    const missing = need.filter((id) => count(id) !== 1);
    check(missing.length === 0, `[8] every bound id has its element exactly once (${need.length} elements)` + show(missing.map((id) => `${id} x${count(id)}`)));
    const fmt = (moduleSrc.match(/const FORMAT = \{([\s\S]*?)\n    \};/) || [])[1] || '';
    const fmtKeys = new Set([...fmt.matchAll(/^\s*([a-z0-9_]+):/gm)].map((m) => m[1]));
    const noFmt = page.float.filter((id) => !fmtKeys.has(id));
    const extraFmt = setDiff(fmtKeys, new Set(page.float));
    check(noFmt.length === 0 && extraFmt.length === 0 && page.float.length === N_FLOAT,
        `[8] FORMAT has exactly the ${N_FLOAT} float ids (${fmtKeys.size} keys)` + show([...noFmt.map((x) => `-${x}`), ...extraFmt.map((x) => `+${x}`)]));
}

// ──────────────────────────────────────────────────── 9. member order ──
console.log('-- [9] editor member order and WebView lifetime');
{
    const pos = (s, re) => { const m = s.match(re); return m ? m.index : -1; };
    const h = {
        sr: pos(editorH, /std::vector<std::unique_ptr<juce::WebSliderRelay>>\s+sliderRelays;/),
        cr: pos(editorH, /std::vector<std::unique_ptr<juce::WebComboBoxRelay>>\s+comboRelays;/),
        tr: pos(editorH, /std::vector<std::unique_ptr<juce::WebToggleButtonRelay>>\s+toggleRelays;/),
        ds: pos(editorH, /std::unique_ptr<Ouaricon::WebViewDropStreaming::SessionManager> dropSessions;/),
        wv: pos(editorH, /std::unique_ptr<juce::WebBrowserComponent> webView;/),
        sa: pos(editorH, /std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>\s+sliderAttachments;/),
        ca: pos(editorH, /std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>>\s+comboAttachments;/),
        ta: pos(editorH, /std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>>\s+toggleAttachments;/),
        ln: pos(editorH, /std::unique_ptr<juce::HyperlinkButton> webView2RuntimeLink;/),
    };
    check(Object.values(h).every((v) => v >= 0), '[9] all nine members declared' + show(Object.entries(h).filter(([, v]) => v < 0).map(([k]) => k)));
    check(h.sr < h.cr && h.cr < h.tr && h.tr < h.ds && h.ds < h.wv && h.wv < h.sa && h.sa < h.ca && h.ca < h.ta && h.ta < h.ln,
        '[9] header order: relays (slider, combo, toggle) -> dropSessions -> webView -> attachments (slider, combo, toggle) -> webView2RuntimeLink');
    const ctorStart = editorCpp.indexOf('GranulatorInstrumentEditor::GranulatorInstrumentEditor (GranulatorInstrumentProcessor& p)');
    const ctorEnd = editorCpp.indexOf('GranulatorInstrumentEditor::~GranulatorInstrumentEditor()');
    const ctor = editorCpp.slice(ctorStart, ctorEnd);
    const c = {
        sr: pos(ctor, /sliderRelays\.push_back/), cr: pos(ctor, /comboRelays\.push_back/), tr: pos(ctor, /toggleRelays\.push_back/),
        ds: pos(ctor, /dropSessions = std::make_unique<Ouaricon::WebViewDropStreaming::SessionManager>/),
        opts: pos(ctor, /withOptionsFrom \(\*relay\)/),
        dropFns: pos(ctor, /dropSessions->getNativeFunctions\(\)/),
        wv: pos(ctor, /webView = std::make_unique<juce::WebBrowserComponent> \(options\);/),
        sa: pos(ctor, /sliderAttachments\.push_back/), ca: pos(ctor, /comboAttachments\.push_back/), ta: pos(ctor, /toggleAttachments\.push_back/),
        link: pos(ctor, /webView2RuntimeLink = std::make_unique<juce::HyperlinkButton>/),
    };
    check(ctorStart >= 0 && ctorEnd > ctorStart && Object.values(c).every((v) => v >= 0)
       && c.sr < c.cr && c.cr < c.tr && c.tr < c.ds && c.ds < c.opts && c.opts < c.dropFns && c.dropFns < c.wv
       && c.wv < c.sa && c.sa < c.ca && c.ca < c.ta && c.ta < c.link,
        '[9] constructor order: relays x3 -> dropSessions -> options -> webView -> attachments x3 -> runtime link'
        + show(Object.entries(c).filter(([, v]) => v < 0).map(([k]) => k)));
    const lastNative = Math.max(...[...ctor.matchAll(/\.withNativeFunction\s*\(/g)].map((m) => m.index), -1);
    check(lastNative >= 0 && lastNative < c.wv, '[9] every withNativeFunction precedes the WebView construction');
    const outside = editorCpp.slice(0, ctorStart) + editorCpp.slice(ctorEnd);
    const uses = outside.split('\n').map((l, i, all) => ({ l, i, all })).filter((x) => /webView->/.test(x.l));
    const unguarded = uses.filter((x) => !x.all.slice(Math.max(0, x.i - 3), x.i + 1).some((p) => /if \(webView != nullptr\)/.test(p)));
    check(unguarded.length === 0, `[9] every webView-> use outside the constructor is null-guarded (${uses.length})` + show(unguarded.map((x) => x.l.trim())));
    check(!/webView->setVisible\s*\(false\)/.test(editorCpp), '[9] never setVisible(false) on the WebView');
    check(/withKeepPageLoadedWhenBrowserIsHidden\(\)/.test(editorCpp) && /withNativeIntegrationEnabled\(\)/.test(editorCpp),
        '[9] native integration + keep-page-loaded-when-hidden');
    check(/GranulatorInstrumentEditor::~GranulatorInstrumentEditor\(\) = default;/.test(editorCpp), '[9] destructor = default (members die in reverse order)');
}

// ──────────────────────────────────────────────────── 10. Windows block ──
console.log('-- [10] Windows WebView2');
{
    const win = (editorCpp.match(/#if JUCE_WINDOWS([\s\S]*?)#else/) || [])[1] || '';
    check(/\.withBackend \(juce::WebBrowserComponent::Options::Backend::webview2\)/.test(win), '[10] withBackend (webview2)');
    check(/\.withUserDataFolder \(webView2DataFolder\)/.test(win) && /webView2DataFolder\.createDirectory\(\);/.test(win)
       && win.indexOf('createDirectory') < win.indexOf('.withUserDataFolder')
       && /userApplicationDataDirectory/.test(win) && !/tempDirectory/.test(stripC(win))
       && /\.getChildFile \("O-GranulatorInstrument"\)/.test(win),
        '[10] withUserDataFolder (app-data/Ouaricon/WebView2/O-GranulatorInstrument, created first; never %TEMP%)');
    check(/areOptionsSupported \(options\)/.test(win), '[10] areOptionsSupported probe');
    const cm = S.readCmake(PLUGIN);
    check(/NEEDS_WEBVIEW2 TRUE/.test(cm) && /JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1/.test(cm)
       && /NEEDS_WEB_BROWSER TRUE/.test(cm) && /JUCE_WEB_BROWSER=1/.test(cm),
        '[10] CMake: NEEDS_WEB_BROWSER, NEEDS_WEBVIEW2, JUCE_WEB_BROWSER=1, static WebView2 linking');
}

// ────────────────────────────────────────────────────────── 11. geometry ──
console.log('-- [11] geometry');
{
    check(/setResizable \(false, false\);\s*\n\s*setSize \(1200, 800\);/.test(editorCpp), '[11] setResizable (false, false) + setSize (1200, 800)');
    const css = (sel) => (html.match(new RegExp(`\\n${sel} \\{([^}]*)\\}`)) || [])[1] || '';
    for (const sel of ['body', '#plugin'])
        check(/width: 1200px;/.test(css(sel)) && /height: 800px;/.test(css(sel)), `[11] ${sel} is 1200 x 800 px`);
}

// ─────────────────────────────────────────────── 12. telemetry key closure ──
console.log('-- [12] telemetry keys');
{
    // PLAN-3.3 PD14: the key source is the LIVE encoder (the one getTelemetry
    // calls), not the frozen 3.1 placeholder telemetryToVar (std::uint32_t …).
    const fnStart = jsonH.indexOf('inline juce::var telemetryToVar (const GranulatorInstrumentProcessor::UiTelemetry& t)');
    const fnEnd = fnStart >= 0 ? jsonH.indexOf('\n    }\n', fnStart) : -1;
    const body = fnStart >= 0 && fnEnd > fnStart ? jsonH.slice(fnStart, fnEnd) : '';
    check(!!body, '[12] the live encoder telemetryToVar (const GranulatorInstrumentProcessor::UiTelemetry& t) is found in GiUiJson.h');
    const keysOf = (recv) => new Set([...body.matchAll(new RegExp(`\\b${recv}->setProperty \\("([A-Za-z]+)"`, 'g'))].map((m) => m[1]));
    const emitted = { top: keysOf('obj'), lane: keysOf('o'), rec: keysOf('rec') };
    const literal = (body.match(/->setProperty \("/g) || []).length;
    // modNorm's keys are the routed destination numbers, set by ONE computed call.
    const modNormCalls = (body.match(/\bmodNorm->setProperty \(/g) || []).length;
    const anyCalls = (body.match(/->setProperty \(/g) || []).length;
    check(emitted.top.size === 14 && emitted.lane.size === 5 && emitted.rec.size === 8
       && literal === emitted.top.size + emitted.lane.size + emitted.rec.size && modNormCalls === 1 && anyCalls === literal + modNormCalls,
        `[12] the live telemetryToVar emits ${emitted.top.size} top-level + ${emitted.lane.size} lane + ${emitted.rec.size} rec keys `
        + `(${literal} literal setProperty calls + ${modNormCalls} computed modNorm key; ${anyCalls} in all)`);
    const code = moduleSrc.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
    const reads = (re) => new Set([...code.matchAll(re)].map((m) => m[1]));
    const read = {
        top:  reads(/\b(?:t|telemetry)\.([A-Za-z]+)\b/g),
        lane: reads(/\b(?:l|lane)\.([A-Za-z]+)\b/g),
        rec:  reads(/\b(?:r|(?:t|telemetry)\.rec)\.([A-Za-z]+)\b/g),
    };
    for (const k of ['top', 'lane', 'rec']) {
        const unread = setDiff(emitted[k], read[k]);
        check(unread.length === 0, `[12] every ${k === 'top' ? 'top-level' : k} key the editor emits is read by the page` + show(unread));
    }
    // 3.2 (PLAN-3.2 PD12 / T8): the native encodes the processor's ONE telemetry
    // builder with the editor's clock; nothing else in the editor reads a
    // consuming feed (the grain frame, the sidechain peak).
    const edCode = stripC(editorCpp);
    check(/complete \(GiUiJson::telemetryToVar \(\s*processorRef\.buildUiTelemetry \(juce::Time::getMillisecondCounterHiRes\(\)\)\)\);/.test(edCode),
        '[12] the getTelemetry native uses the shared builder (GiUiJson::telemetryToVar (processorRef.buildUiTelemetry (now)))');
    const builds = (edCode.match(/\bbuildUiTelemetry\b/g) || []).length;
    // PLAN-3.3 PD15: the feeds buildUiTelemetry consumes — the grain frame, the
    // output peaks L / R, the sidechain peaks L / R, the steal ring's cursor.
    const feedRe = /\breadUiFrame\b|\btakeScPeak\b|\bframes\s*\.\s*read\b|\btakeOutPeaks\b|\btakeScPeaks\b|\bstolenCount\b|\btakeOutPeak\b/g;
    const otherReaders = edCode.match(feedRe) || [];
    check(builds === 1 && otherReaders.length === 0,
        `[12] buildUiTelemetry is called exactly once in the editor (${builds}) and no other frame / peak / steal-ring reader exists (${otherReaders.length})` + show(otherReaders));
    // ... and inside the processor every read of those feeds is in buildUiTelemetry.
    const pc = stripC(procCpp);
    const bStart = pc.indexOf('GranulatorInstrumentProcessor::UiTelemetry GranulatorInstrumentProcessor::buildUiTelemetry (double nowMs)');
    const bEnd = bStart >= 0 ? pc.indexOf('\n}\n', bStart) : -1;
    const inside = bStart >= 0 && bEnd > bStart ? pc.slice(bStart, bEnd) : '';
    const outsideB = bStart >= 0 && bEnd > bStart ? pc.slice(0, bStart) + pc.slice(bEnd) : pc;
    const feedsIn = (s) => ({
        frame: (s.match(/\bframes\s*\.\s*read\s*\(/g) || []).length, out: (s.match(/\btakeOutPeaks\s*\(/g) || []).length,
        sc: (s.match(/\btakeScPeaks\s*\(/g) || []).length, steal: (s.match(/\bstolenCount\s*\.\s*load\s*\(/g) || []).length,
    });
    const fin = feedsIn(inside), fout = feedsIn(outsideB);
    const strays = (outsideB.match(feedRe) || []);
    check(!!inside && fin.frame === 1 && fin.out === 1 && fin.sc === 1 && fin.steal >= 1 && Object.values(fout).every((n) => n === 0) && strays.length === 0,
        `[12] PluginProcessor.cpp: frames.read / takeOutPeaks / takeScPeaks / stolenCount.load only inside buildUiTelemetry `
        + `(inside ${fin.frame} / ${fin.out} / ${fin.sc} / ${fin.steal}; outside ${fout.frame} / ${fout.out} / ${fout.sc} / ${fout.steal})` + show(strays));
    let fixture = null;
    try { fixture = JSON.parse(fs.readFileSync(overridesPath, 'utf8')).natives.getTelemetry; } catch (_) { /* reported below */ }
    const fx = new Set(fixture ? Object.keys(fixture) : []);
    check(!!fixture && setDiff(fx, emitted.top).length === 0 && setDiff(emitted.top, fx).length === 0,
        '[12] the stub fixture getTelemetry (generic-overrides.json) has exactly the emitted top-level keys'
        + show([...setDiff(emitted.top, fx).map((x) => `-${x}`), ...setDiff(fx, emitted.top).map((x) => `+${x}`)]));
}

// ─────────────────────────────────────────────── 13. headless isolation ──
console.log('-- [13] render harness never sees the editor');
{
    check(!/#include\s+"PluginEditor/.test(stripC(harness)) && !/GranulatorInstrumentEditor/.test(stripC(harness)), '[13] render-harness main.cpp does not include PluginEditor');
    check(/#if JUCE_WEB_BROWSER\s*\n\s*#include "PluginEditor\.h"/.test(procCpp)
       && /#if JUCE_WEB_BROWSER\s*\n\s*return new GranulatorInstrumentEditor \(\*this\);/.test(procCpp),
        '[13] PluginProcessor.cpp includes / creates the editor only under JUCE_WEB_BROWSER');
    check(!/#include\s+<juce_gui_extra|WebBrowserComponent/.test(stripC(jsonH)), '[13] GiUiJson.h has no WebBrowserComponent types');
}

// ───────────────────────────────────────────────── 14. DevLoader removed ──
console.log('-- [14] DevLoader removed');
{
    const srcDir = path.join(pluginRoot, 'Source');
    const skip = path.join(srcDir, 'ui', 'public', 'modules');
    const hits = [];
    let scanned = 0;
    const token = /DevLoader|OGI_DEV_LOADER/;
    const walk = (dir) => {
        for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
            const p = path.join(dir, e.name);
            if (p === skip) continue;
            if (token.test(e.name)) hits.push(`${path.relative(pluginRoot, p)} (file name)`);
            if (e.isDirectory()) { walk(p); continue; }
            if (/\.(woff2?|ttf|otf|png|jpe?g|gif|webp|ico|flac|wav)$/i.test(e.name)) continue;
            ++scanned;
            const lines = fs.readFileSync(p, 'utf8').split('\n');
            lines.forEach((l, i) => { if (token.test(l)) hits.push(`${path.relative(pluginRoot, p)}:${i + 1}`); });
        }
    };
    walk(srcDir);
    cmake.split('\n').forEach((l, i) => { if (token.test(l)) hits.push(`CMakeLists.txt:${i + 1}`); });
    check(scanned >= 10 && hits.length === 0, `[14] no DevLoader / OGI_DEV_LOADER token under Source/ (${scanned} files; ui/public/modules excluded) or CMakeLists.txt` + show(hits));
}

// ───────────────────────────────────────────── 15. Phase 3.2 closure (PD26) ──
console.log('-- [15] Phase 3.2 closure: emitted and read, live natives, failure toasts');
{
    const code = moduleSrc.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
    // One C++ function body: from its signature to the first "\n    }\n" (the
    // namespace-level closing brace of an inline function in GiUiJson.h).
    const cppFn = (sig) => {
        const a = jsonH.indexOf(sig);
        if (a < 0) return '';
        const b = jsonH.indexOf('\n    }\n', a);
        return b > a ? jsonH.slice(a, b) : '';
    };
    // One page function body: from "function name(" to the next top-of-controller
    // function declaration (4-space indent), so a reader elsewhere does not count.
    const jsFn = (name) => {
        const m = code.match(new RegExp(`\\n    (?:async )?function ${name}\\(`));
        if (!m) return '';
        const rest = code.slice(m.index + 1);
        const next = rest.slice(1).search(/\n    (?:async )?function [A-Za-z]/);
        return next < 0 ? rest : rest.slice(0, next + 1);
    };
    const tel32 = cppFn('inline juce::var telemetryToVar (const GranulatorInstrumentProcessor::UiTelemetry& t)');
    const tel31 = cppFn('inline juce::var telemetryToVar (std::uint32_t');
    const slotFn = cppFn('inline juce::var slotInfoToVar (const ogi::SlotInfo& info');
    const ovFn = cppFn('inline juce::var slotOverviewToVar (const GranulatorInstrumentProcessor& p, int slot0)');
    check(!!tel32 && !!tel31 && !!slotFn && !!ovFn, '[15] the four encoders are found in GiUiJson.h (live telemetry, frozen 3.1 placeholder, slot, overview)');
    const keys = (body, recv) => new Set([...body.matchAll(new RegExp(`\\b${recv}->setProperty \\("([A-Za-z]+)"`, 'g'))].map((m) => m[1]));
    const e32 = { top: keys(tel32, 'obj'), lane: keys(tel32, 'o'), rec: keys(tel32, 'rec') };
    const e31 = { top: keys(tel31, 'obj'), lane: keys(tel31, 'o'), rec: keys(tel31, 'rec') };
    const eqList = (a, b) => JSON.stringify([...a].sort()) === JSON.stringify([...b].sort());
    // PLAN-3.3 PD13 / PD14: the live set is explicit (14 / 3 + lo, hi / 8); the
    // frozen placeholder's set is a subset; the difference is exactly lfoPh
    // (top level) and lo, hi (lane) — nothing of 3.1 / 3.2 renamed or dropped.
    const laneStatic = ['ovRev', 'fb', 'rec'];
    check(e32.top.size === 14 && e32.lane.size === 5 && e32.rec.size === 8
       && laneStatic.every((k) => e32.lane.has(k)) && e32.lane.has('lo') && e32.lane.has('hi'),
        `[15] the live encoder emits 14 top-level + 3 lane (+ lo, hi) + 8 rec keys (found ${e32.top.size} + ${e32.lane.size} + ${e32.rec.size})`);
    check(/if \(lane\.hasLoHi\)\s*\n\s*\{\s*\n\s*o->setProperty \("lo", [^\n]*\n\s*o->setProperty \("hi", /.test(tel32),
        '[15] lanes[].lo / .hi are emitted only under lane.hasLoHi (absent otherwise, never 0)');
    const sub = setDiff(e31.top, e32.top).length === 0 && setDiff(e31.lane, e32.lane).length === 0 && setDiff(e31.rec, e32.rec).length === 0;
    const dTop = setDiff(e32.top, e31.top), dLane = setDiff(e32.lane, e31.lane), dRec = setDiff(e32.rec, e31.rec);
    check(e31.top.size === 13 && e31.lane.size === 3 && e31.rec.size === 8 && sub
       && eqList(dTop, ['lfoPh']) && eqList(dLane, ['lo', 'hi']) && dRec.length === 0,
        `[15] frozen placeholder (13 / 3 / 8) ⊂ live, difference exactly lfoPh (top) and lo, hi (lane)`
        + ` (found +top [${dTop}], +lane [${dLane}], +rec [${dRec}]`
        + `${sub ? '' : '; placeholder keys missing from live: ' + [...setDiff(e31.top, e32.top), ...setDiff(e31.lane, e32.lane), ...setDiff(e31.rec, e32.rec)].join(', ')})`);

    const fn = {
        drawLane: jsFn('drawLane'), renderRec: jsFn('renderRec'), ingest: jsFn('ingestTelemetry'), renderLane: jsFn('renderLane'),
        noteErr: jsFn('noteSlotErrors'), counts: jsFn('updateReadouts'), fetchOv: jsFn('fetchOverview'), slotAction: jsFn('slotAction'),
    };
    const missingFn = Object.entries(fn).filter(([, b]) => !b).map(([k]) => k);
    check(missingFn.length === 0, '[15] the page functions that read the 3.2 keys are found' + show(missingFn));
    // key, emitted (C++), read (page): [label, emitted?, read?, where]
    const rows = [
        ['fb',           e32.lane.has('fb') && /lane\.hasFb\s*\?/.test(tel32),   /\blane\.fb\b/.test(fn.drawLane),                     'lanes[].fb -> drawLane'],
        ['rec',          e32.lane.has('rec') && /lane\.hasRec\s*\?/.test(tel32), /\blane\.rec\b/.test(fn.drawLane),                    'lanes[].rec -> drawLane'],
        ['voices',       /obj->setProperty \("voices", voices\);/.test(tel32),   /\bt\.voices\b/.test(fn.drawLane),                    'voices -> drawLane'],
        ['grains',       /obj->setProperty \("grains", grains\);/.test(tel32),   /\bt\.grains\b/.test(fn.drawLane),                    'grains -> drawLane'],
        ['counts',       /obj->setProperty \("counts", counts\);/.test(tel32),   /\bt\.counts\b/.test(fn.counts),                      'counts -> updateReadouts'],
        ['rec.run',      /rec->setProperty \("run", t\.rec\.run\);/.test(tel32),       /\br\.run\b/.test(fn.renderRec),              'rec.run -> renderRec'],
        ['rec.slot',     /rec->setProperty \("slot", t\.rec\.slot\);/.test(tel32),     /\br\.slot\b/.test(fn.renderRec),             'rec.slot -> renderRec'],
        ['rec.pos',      /rec->setProperty \("pos", r4 \(/.test(tel32),                /\br\.pos\b/.test(fn.renderRec),              'rec.pos -> renderRec'],
        ['rec.len',      /rec->setProperty \("len", r4 \(/.test(tel32),                /\br\.len\b/.test(fn.renderRec),              'rec.len -> renderRec'],
        ['rec.signal',   /rec->setProperty \("signal", t\.rec\.signal\);/.test(tel32), /\br\.signal\b/.test(fn.renderRec),           'rec.signal -> renderRec'],
        ['rec.capped',   /rec->setProperty \("capped", t\.rec\.capped\);/.test(tel32), /\br\.capped\b/.test(fn.renderRec),           'rec.capped -> renderRec'],
        ['rec.peak',     /rec->setProperty \("peak", peak\);/.test(tel32),             /\bt\.rec\.peak\b/.test(fn.ingest),            'rec.peak -> ingestTelemetry'],
        ['dirty (slot)', /o->setProperty \("dirty", dirty\);/.test(slotFn),            /\b[sq]\.dirty\b/.test(fn.renderLane),          'slots[].dirty -> renderLane'],
        ['dirty (overview)', /obj->setProperty \("dirty", dt\);/.test(ovFn),           /\bov\.dirty\b/.test(fn.drawLane),             'overview dirty[] -> drawLane'],
        ['encodedBytes', /o->setProperty \("encodedBytes", encodedBytes\);/.test(slotFn), /\bs\.encodedBytes\b/.test(fn.renderLane),  'slots[].encodedBytes -> renderLane'],
        ['error',        /o->setProperty \("error", info\.lastError\);/.test(slotFn), /\bs\.error\b/.test(fn.noteErr),               'slots[].error -> noteSlotErrors'],
    ];
    for (const [key, emitted, read, where] of rows)
        check(emitted && read, `[15] emitted and read: ${key} (${where})${emitted ? '' : ' — NOT emitted by the encoder'}${read ? '' : ' — NOT read by the page'}`);

    // The real values reach the encoders: grain rows are the eight page columns,
    // the slot flags come from the processor, the overview answer carries its rev.
    check(/row\.add \(g\.stream\);\s*\n\s*row\.add \(r4 \(static_cast<double> \(g\.normPos\)\)\);\s*\n\s*row\.add \(r4 \(static_cast<double> \(g\.pan\)\)\);\s*\n\s*row\.add \(r4 \(static_cast<double> \(g\.env\)\)\);\s*\n\s*row\.add \(r4 \(static_cast<double> \(g\.gain\)\)\);\s*\n\s*row\.add \(r4 \(static_cast<double> \(g\.lengthSec\)\)\);\s*\n\s*row\.add \(g\.voice\);\s*\n\s*row\.add \(g\.flags\);/.test(tel32),
        '[15] a grain row is [stream, normPos, pan, env, gain, lengthSec, voice, flags], the order drawLane indexes');
    const edCode = stripC(editorCpp);
    check(/dirty\[static_cast<std::size_t> \(s\)\] = processorRef\.getSlotDirty \(s\);/.test(edCode)
       && /complete \(GiUiJson::slotStateToVar \(revision, infos, dirty\)\);/.test(edCode),
        '[15] the getSlotState native passes the processor\'s dirty flags (getSlotDirty -> slotStateToVar (revision, infos, dirty))');
    check(/obj->setProperty \("rev", revToInt \(rev\)\);/.test(ovFn) && /typeof o\.rev === 'number' \? o\.rev : rev/.test(fn.fetchOv),
        '[15] the overview answer carries its rev and fetchOverview stores the ANSWER\'s rev (PD8)');

    // No refused-by-design answer left.
    const un = editorCpp.split('\n').map((l, i) => ({ l, i })).filter((x) => /"unavailable"/.test(x.l));
    check(un.length === 0, '[15] no "unavailable" literal left in PluginEditor.cpp' + show(un.map((x) => `:${x.i + 1}`)));
    // The three lane natives are registered and the page calls them through slotAction.
    const lane = ['clearSlot', 'saveRecording', 'relinkSlot'];
    const laneBad = lane.filter((n) => !new RegExp(`\\.withNativeFunction\\s*\\(\\s*"${n}"`).test(editorCpp) || !new RegExp(`slotAction\\('${n}', n\\)`).test(code));
    const regCount = new Set([...editorCpp.matchAll(/\.withNativeFunction\s*\(\s*"([A-Za-z]+)"/g)].map((m) => m[1])).size;
    check(laneBad.length === 0 && regCount === N_NATIVE,
        `[15] clearSlot / saveRecording / relinkSlot are registered and called with the lane number; the surface is still ${N_NATIVE} (${regCount})` + show(laneBad));

    // Failure toasts (PD17): three languages, the {n} variable, raised by the page.
    const i18n = fs.existsSync(i18nPath) ? fs.readFileSync(i18nPath, 'utf8') : '';
    for (const [key, raiser, rule] of [
        ['toast.loadFailed', fn.noteErr, /errs\[n - 1\] && errs\[n - 1\] !== slotErrSeen\[n - 1\]/],
        ['toast.saveFailed', fn.slotAction, /res\.ok === false && name === 'saveRecording' && res\.error !== 'cancelled'/]]) {
        const block = (i18n.match(new RegExp(`\\n    '${key.replace('.', '\\.')}': \\{([\\s\\S]*?)\\n    \\},`)) || [])[1] || '';
        const langs = { en: /\n\s*en: \{ t: "([^"]*)"/, fr: /\n\s*fr: \{ t: "([^"]*)"/, 'zh-Hans': /\n\s*'zh-Hans': \{ t: "([^"]*)"/ };
        const bad = Object.entries(langs).filter(([, re]) => { const m = block.match(re); return !m || !m[1].trim() || !m[1].includes('{n}'); }).map(([l]) => l);
        const texts = Object.values(langs).map((re) => (block.match(re) || [])[1]);
        check(!!block && bad.length === 0 && new Set(texts).size === 3,
            `[15] ${key} has en / fr / zh-Hans texts, each with {n}, all different (js/i18n.js)` + show(bad));
        check(new RegExp(`setLabel\\(el, '${key.replace('.', '\\.')}', \\{ n: String\\(n\\) \\}\\)`).test(raiser) && rule.test(raiser),
            `[15] the page raises ${key} with { n } under its rule (${key === 'toast.loadFailed' ? 'error changed to non-empty' : 'saveRecording refused, not cancelled'})`);
    }
}

// ───────────────────────────────────────────── 16. Phase 3.3 closure (PD23) ──
console.log('-- [16] Phase 3.3 closure: emitted and read, mono rule == UMB1 transcription, LFO dot, state class gate');
{
    const code = moduleSrc.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
    const fnNames = [...code.matchAll(/\n {4}(?:async )?function ([A-Za-z0-9_]+)\(/g)].map((m) => m[1]);
    const jsFn = (name) => {
        const m = code.match(new RegExp(`\\n    (?:async )?function ${name}\\(`));
        if (!m) return '';
        const rest = code.slice(m.index + 1);
        const next = rest.slice(1).search(/\n    (?:async )?function [A-Za-z]/);
        return next < 0 ? rest : rest.slice(0, next + 1);
    };
    const a = jsonH.indexOf('inline juce::var telemetryToVar (const GranulatorInstrumentProcessor::UiTelemetry& t)');
    const b = a >= 0 ? jsonH.indexOf('\n    }\n', a) : -1;
    const tel = a >= 0 && b > a ? jsonH.slice(a, b) : '';
    check(!!tel, '[16] the live encoder is found');
    const keys = (recv) => new Set([...tel.matchAll(new RegExp(`\\b${recv}->setProperty \\("([A-Za-z]+)"`, 'g'))].map((m) => m[1]));
    const top = keys('obj'), lane = keys('o'), rec = keys('rec');

    const fn = {};
    for (const n of ['drawVoices', 'drawMenvFigure', 'drawLfos', 'drawModSources', 'drawAmpFigure', 'updateLiveModulation',
                     'modulatedNorm', 'ingestTelemetry', 'updateReadouts', 'drawLane'])
        fn[n] = jsFn(n);
    const missingFn = Object.entries(fn).filter(([, s]) => !s).map(([k]) => k);
    check(missingFn.length === 0, '[16] the page functions that read the 3.3 keys are found' + show(missingFn));
    const tRead = (body, key) => new RegExp(`\\b(?:t|telemetry)\\.${key}\\b`).test(body || '');
    // The voice row as the encoder writes it: state, note, amp, 4 scans, wantSt.
    const voiceRow = /row\.add \(v\.state\);\s*\n\s*row\.add \(v\.note\);\s*\n\s*row\.add \(r4 \(static_cast<double> \(v\.amp\)\)\);\s*\n\s*for \(const auto sc : v\.scan\)\s*\n\s*row\.add \(r4 \(static_cast<double> \(sc\)\)\);\s*\n\s*row\.add \(r4 \(static_cast<double> \(v\.wantSt\)\)\);/.test(tel);
    // Four scan columns (v[3]…v[6]) so wantSt is column 8 (v[7]): UiTelemetry's scan is std::array<float, kGiStreams>, kGiStreams 4.
    const scanDecl = /std::array<float, ogi::kGiStreams> scan\b/.test(fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.h'), 'utf8'));
    const streamsN = Number((/inline constexpr int kGiStreams = (\d+);/.exec(fs.readFileSync(path.join(pluginRoot, 'Source', 'dsp', 'GiConstants.h'), 'utf8')) || [])[1]);
    const scanN = scanDecl ? String(streamsN) : '';
    const procC = stripC(procCpp);
    // key, emitted (C++), read (page), where
    const rows = [
        ['newest', /obj->setProperty \("newest", t\.newest\);/.test(tel),
            ['drawVoices', 'drawMenvFigure', 'drawLfos', 'drawModSources'].every((f) => tRead(fn[f], 'newest')),
            'drawVoices / drawMenvFigure / drawLfos / drawModSources'],
        ['menv', /obj->setProperty \("menv", menv\);/.test(tel) && /row\.add \(r4 \(static_cast<double> \(m\.e1\)\)\);\s*\n\s*row\.add \(m\.st1\);\s*\n\s*row\.add \(r4 \(static_cast<double> \(m\.e2\)\)\);\s*\n\s*row\.add \(m\.st2\);\s*\n\s*row\.add \(m\.ampSt\);/.test(tel),
            tRead(fn.drawAmpFigure, 'menv') && tRead(fn.drawMenvFigure, 'menv') && /me\[i\]\[4\]/.test(fn.drawAmpFigure) && /m\[\(e - 1\) \* 2\]/.test(fn.drawMenvFigure) && /m\[\(e - 1\) \* 2 \+ 1\]/.test(fn.drawMenvFigure),
            'drawAmpFigure (ampSt, column 5) / drawMenvFigure (e1, st1, e2, st2)'],
        ['sources', /obj->setProperty \("sources", sources\);/.test(tel),
            ['drawLfos', 'drawModSources', 'updateLiveModulation', 'drawVoices'].every((f) => tRead(fn[f], 'sources')),
            'drawLfos / drawModSources / updateLiveModulation / drawVoices'],
        ['modNorm', /obj->setProperty \("modNorm", juce::var \(modNorm\)\);/.test(tel) && /if \(t\.modRouted\[d\]\)\s*\n\s*modNorm->setProperty \(/.test(tel),
            tRead(fn.modulatedNorm, 'modNorm') && /modulatedNorm\(/.test(fn.updateLiveModulation) && /modulatedNorm\(/.test(fn.drawLane),
            'modulatedNorm (routed keys only) <- updateLiveModulation (rings) / drawLane (edge tick)'],
        ['lfoPh', /obj->setProperty \("lfoPh", lfoPh\);/.test(tel), tRead(fn.drawLfos, 'lfoPh'), 'drawLfos'],
        ['out', /obj->setProperty \("out", out\);/.test(tel) && /out\.add \(r4 \(static_cast<double> \(t\.outL\)\)\);\s*\n\s*out\.add \(r4 \(static_cast<double> \(t\.outR\)\)\);/.test(tel),
            tRead(fn.ingestTelemetry, 'out') && /peaks\[0\]/.test(fn.ingestTelemetry) && /peaks\[1\]/.test(fn.ingestTelemetry), 'ingestTelemetry ([L, R])'],
        ['rec.peak [L, R]', /peak\.add \(r4 \(static_cast<double> \(t\.rec\.peakL\)\)\);\s*\n\s*peak\.add \(r4 \(static_cast<double> \(t\.rec\.peakR\)\)\);/.test(tel) && rec.has('peak'),
            /\bt\.rec\.peak\b/.test(fn.ingestTelemetry) && /sc\[0\]/.test(fn.ingestTelemetry) && /sc\[1\]/.test(fn.ingestTelemetry), 'ingestTelemetry ([L, R])'],
        ['limit', /obj->setProperty \("limit", t\.limit\);/.test(tel), tRead(fn.updateReadouts, 'limit'), 'updateReadouts'],
        ['lanes[].lo / .hi', lane.has('lo') && lane.has('hi'), /\blane\.lo\b/.test(fn.drawLane) && /\blane\.hi\b/.test(fn.drawLane), 'drawLane'],
        ['voice row column 8 (wantSt)', voiceRow && scanN === '4', /Number\(v\[7\]\)/.test(fn.drawVoices) && /Math\.abs\(want\) > 48/.test(fn.drawVoices), 'drawVoices v[7] (> 48 -> vstate.clamp)'],
        ['voice states 2 / 3', voiceRow, /st === 2\) setLabel\(stt, 'vstate\.release'\)/.test(fn.drawVoices) && /st === 3\) setLabel\(stt, 'vstate\.stolen'\)/.test(fn.drawVoices), 'drawVoices (vstate.release / vstate.stolen)'],
        ['grain bit 2 (clamp)', /row\.add \(g\.flags\);/.test(tel) && /g\.flags \|= 2;/.test(procC), /\(Number\(gr\[7\]\) & 2\)/.test(fn.drawLane), 'drawLane (warn colour)'],
    ];
    for (const [key, emitted, read, where] of rows)
        check(emitted && read, `[16] emitted and read: ${key} (${where})${emitted ? '' : ' — NOT emitted by the encoder'}${read ? '' : ' — NOT read by the page'}`);
    const notSent = ['frozen', 'modRouted', 'stale'].filter((k) => top.has(k) || lane.has(k) || rec.has(k) || new RegExp(`setProperty \\("${k}"`).test(tel));
    check(notSent.length === 0, '[16] not emitted: frozen, modRouted, stale (no page reader)' + show(notSent));

    // ── the mono rule: page == the UMB1 transcription (ui_gates_33.h pageRule) ──
    const g33Path = path.join(pluginRoot, 'tests', 'render-harness', 'ui_gates_33.h');
    const g33 = fs.existsSync(g33Path) ? stripC(fs.readFileSync(g33Path, 'utf8')) : '';
    const nsM = g33.match(/namespace pageRule\s*\{([\s\S]*?)\n {4}\}/);
    const ns = nsM ? nsM[1] : '';
    const cInt = (name) => { const m = ns.match(new RegExp(`\\b${name} = (-?\\d+)`)); return m ? Number(m[1]) : NaN; };
    const cPv = ((ns.match(/kPerVoiceSources\[\] = \{([^}]*)\}/) || [])[1] || '').split(',').map((x) => Number(x.trim())).filter((x) => Number.isFinite(x));
    const C = { lfoLo: cInt('kLfoSrcLo'), lfoHi: cInt('kLfoSrcHi'), g: cInt('kGlobalDest'), gLo: cInt('kGlobalDestLo'), gHi: cInt('kGlobalDestHi'),
                rLo: cInt('kLfoRateDestLo'), rHi: cInt('kLfoRateDestHi'), rBase: cInt('kLfoRateDestBase') };
    const cFnOk = /return src >= kLfoSrcLo && src <= kLfoSrcHi && lfoMode\[src - 1\] == 1;/.test(ns)
               && /if \(d == kGlobalDest \|\| \(d >= kGlobalDestLo && d <= kGlobalDestHi\)\)\s*\n\s*return true;/.test(ns)
               && /if \(d >= kLfoRateDestLo && d <= kLfoRateDestHi\)\s*\n\s*return lfoMode\[d - kLfoRateDestBase - 1\] == 0;/.test(ns);
    check(!!ns && cPv.length > 0 && Object.values(C).every(Number.isFinite) && cFnOk,
        `[16] ui_gates_33.h pageRule found: kPerVoiceSources [${cPv}], LFO src ${C.lfoLo}–${C.lfoHi}, global ${C.g} + ${C.gLo}–${C.gHi}, LFO rate ${C.rLo}–${C.rHi} (base ${C.rBase})`);
    const pagePv = ((moduleSrc.match(/const PER_VOICE_SOURCES = \[([^\]]*)\];/) || [])[1] || '').split(',').map((x) => Number(x.trim())).filter((x) => Number.isFinite(x));
    check(JSON.stringify(pagePv) === JSON.stringify([5, 6, 7, 8, 9, 11, 12, 13]) && JSON.stringify(pagePv) === JSON.stringify(cPv),
        `[16] PER_VOICE_SOURCES == [5, 6, 7, 8, 9, 11, 12, 13] == pageRule::kPerVoiceSources (page [${pagePv}], harness [${cPv}])`);
    // Evaluate the page's own two functions (their text, a comboIndex stub) against
    // the C++ transcription for every source x destination x LFO-mode set.
    let pageFns = null;
    try {
        const src = (n) => { const s = jsFn(n); if (!s) throw new Error(`${n} not found`); return s; };
        pageFns = new Function('PER_VOICE_SOURCES', 'comboIndex', `${src('isPerVoiceSource')}\n${src('isGlobalDest')}\nreturn { isPerVoiceSource, isGlobalDest };`);
    } catch (e) { pageFns = null; check(false, `[16] the page's isPerVoiceSource / isGlobalDest could not be evaluated — ${e.message}`); }
    if (pageFns) {
        const ruleIsPv = (s, mode) => cPv.includes(s) || (s >= C.lfoLo && s <= C.lfoHi && mode[s - 1] === 1);
        const ruleIsG = (d, mode) => d === C.g || (d >= C.gLo && d <= C.gHi) || (d >= C.rLo && d <= C.rHi ? mode[d - C.rBase - 1] === 0 : false);
        let pairs = 0, bad = 0, monoOn = 0, first = '';
        for (let m = 0; m < 16; m++) {                                   // every combination of the four lfoN_mode values
            const mode = [0, 1, 2, 3].map((i) => (m >> i) & 1);
            const P = pageFns(pagePv, (id) => { const k = /^lfo(\d)_mode$/.exec(id); return k ? mode[Number(k[1]) - 1] : 0; });
            for (let s = 1; s <= 17; s++) for (let d = 1; d <= 125; d++) {
                ++pairs;
                const page = P.isPerVoiceSource(s) && P.isGlobalDest(d), rule = ruleIsPv(s, mode) && ruleIsG(d, mode);
                if (page) ++monoOn;
                if (page !== rule) { ++bad; if (!first) first = `modes ${mode.join('')} src ${s} dest ${d}: page ${page}, rule ${rule}`; }
            }
        }
        check(pairs === 16 * 17 * 125 && bad === 0 && monoOn > 0,
            `[16] the page's mono rule == the UMB1 transcription: ${pairs - bad} / ${pairs} pairs (17 x 125 x 16 LFO-mode sets; mono-on ${monoOn})${first ? ' — first ' + first : ''}`);
        const gl = (mode) => { const P = pageFns(pagePv, (id) => { const k = /^lfo(\d)_mode$/.exec(id); return k ? mode[Number(k[1]) - 1] : 0; }); return range(125).filter((d) => P.isGlobalDest(d)); };
        const g0 = gl([0, 0, 0, 0]), g1 = gl([1, 1, 1, 1]);
        check(JSON.stringify(g0) === JSON.stringify([1, ...range(8).map((i) => 113 + i), 122, 123, 124, 125]) && JSON.stringify(g1) === JSON.stringify([1, ...range(8).map((i) => 113 + i)]),
            `[16] isGlobalDest ranges: 1, 114–121 always; 122–125 only while that LFO is Global (Global: [${g0.join(',')}]; Per-voice: [${g1.join(',')}])`);
    }

    // ── the LFO dot sits on the drawn curve: Y uses the phase offset (PD16) ──
    const lf = fn.drawLfos || '';
    check(/const pp = \(\(p % 1\) \+ 1\) % 1, X = 4 \+ pp \/ 2 \* \(w - 8\), Y = h \/ 2 - lfoShapeAt\(shape, \(pp \+ ph0\) % 1\) \* \(h \/ 2 - 6\);/.test(lf)
       && /const p = i \/ 120 \* 2 \+ ph0, v = lfoShapeAt\(shape, p % 1\)/.test(lf) && !/lfoShapeAt\(shape, pp\)/.test(lf),
        '[16] LFO dot: Y = lfoShapeAt(shape, (pp + ph0) % 1), the curve\'s own phase; X = pp / 2 (PD16)');

    // ── class gate (D49a): no state __setLabel's a node a page reader writes ──
    // Roots: the functions the page arms with setInterval / requestAnimationFrame.
    const roots = [...new Set([...code.matchAll(/\b(?:setInterval|requestAnimationFrame)\(\s*([A-Za-z0-9_]+)/g)].map((m) => m[1]))].filter((n) => fnNames.includes(n));
    const reach = new Set(roots), queue = [...roots];
    while (queue.length) {
        const body = jsFn(queue.shift());
        for (const n of fnNames) if (!reach.has(n) && new RegExp(`(?<![.\\w])${n}\\(`).test(body)) { reach.add(n); queue.push(n); }
    }
    // Every setLabel(<x>, …) target inside those functions, resolved to an id (template parts -> wildcards).
    const toRe = (lit) => lit.replace(/[.*+?^${}()|[\]\\]/g, (c) => '\\' + c).replace(/\\\$\\\{[^}]*\\\}/g, '[^\\s\'"]+');
    const written = new Map(), unresolved = [], created = [];
    for (const n of reach) {
        const body = jsFn(n);
        for (const m of body.matchAll(/(?<![.\w])setLabel\(\s*([A-Za-z_$][\w$]*)\s*,/g)) {
            const v = m[1];
            let id = null;
            const decl = body.match(new RegExp(`\\b${v.replace('$', '\\$')} = document\\.getElementById\\((['\`])([^'\`]+)\\1\\)`));
            const vq = v.replace('$', '\\$');
            const sub = body.match(new RegExp(`\\b${vq} = [\\w$.]+\\.querySelector\\((['\`])([^'\`]+)\\1\\)`));
            const made = new RegExp(`\\b${vq} = (?:svgEl|document\\.createElement(?:NS)?)\\(`).test(body);
            if (decl) id = decl[2];
            else if (v === 'el' && /showToast\(\(el\) => setLabel\(el,/.test(body)) id = 'toast';
            else if (v === 'toastEl') id = 'toast';
            else if (sub) { created.push(`${n}: ${v} = …querySelector('${sub[2]}')`); continue; }   // a child of a built row: no id to address
            else if (made) { created.push(`${n}: ${v} = a node it creates`); continue; }
            if (id === null) { unresolved.push(`${n}: setLabel(${v}, …)`); continue; }
            if (!written.has(id)) written.set(id, new Set());
            written.get(id).add(n);
        }
    }
    const writtenRe = [...written.keys()].map((id) => ({ id, re: new RegExp(`^${toRe(id)}$`) }));
    check(roots.length >= 2 && unresolved.length === 0 && ['rec-status-t', 'rec-prog-s'].every((id) => written.has(id)),
        `[16] reader-written ids derived from the page: roots [${roots.join(', ')}] reach ${reach.size} functions, `
        + `${written.size} id targets (${[...written.keys()].join(', ')}) + ${created.length} id-less (created nodes / row children)` + show(unresolved));
    // A state's __setLabel target: a literal getElementById / querySelector('#id').
    const setLabelTargets = (src) => {
        const out = [];
        for (const m of String(src).matchAll(/__setLabel\(\s*([^,]+),/g)) {
            const t = m[1].trim();
            const lit = t.match(/^document\.getElementById\((['"])([^'"]+)\1\)$/) || t.match(/^document\.querySelector\((['"])#([A-Za-z0-9_-]+)\1\)$/);
            out.push(lit ? { id: lit[2] } : { id: null, raw: t });
        }
        return out;
    };
    const violations = (states) => {
        const bad = [];
        for (const st of states) for (const t of setLabelTargets(st.eval || '')) {
            if (t.id === null) bad.push(`${st.name}: __setLabel on an unresolvable node (${t.raw})`);
            else { const w = writtenRe.find((x) => x.re.test(t.id)); if (w) bad.push(`${st.name}: __setLabel #${t.id} (written by ${[...written.get(w.id)].join(' / ')})`); }
        }
        return bad;
    };
    // The gate must catch the three 3.2 vacuous states (SUMMARY-3.2 deviation 3) — its own liveness.
    const vacuous32 = [
        { name: 'rec-wait-level', eval: "window.__setLabel(document.getElementById('rec-status-t'), 'rec.waitLevel'); window.__setLabel(document.getElementById('rec-prog-s'), 'rec.capped', { s: '30 s' });" },
        { name: 'rec-no-sidechain', eval: "window.__setLabel(document.getElementById('rec-status-t'), 'rec.noSidechain');" },
        { name: 'rec-recording', eval: "window.__setLabel(document.getElementById('rec-status-t'), 'rec.recording', { n: 'IV' });" },
    ];
    const caught = violations(vacuous32);
    check(caught.length === 4, `[16] class gate is live: the three 3.2 vacuous states are caught (${caught.length} / 4 __setLabel calls)`);
    let statesJson = null;
    try { statesJson = JSON.parse(fs.readFileSync(path.join(pluginRoot, 'tests', 'i18n-states.json'), 'utf8')); } catch (_) { /* reported below */ }
    const v = Array.isArray(statesJson) ? violations(statesJson) : ['tests/i18n-states.json missing or unparsable'];
    const nSet = Array.isArray(statesJson) ? statesJson.reduce((k, st) => k + setLabelTargets(st.eval || '').length, 0) : 0;
    check(v.length === 0, `[16] class gate (D49a): no state in tests/i18n-states.json __setLabel's a reader-written node `
        + `(${Array.isArray(statesJson) ? statesJson.length : 0} states, ${nSet} __setLabel call(s))` + show(v));
}

console.log(failed === 0 ? '\nALL PASS' : `\n${failed} FAILED`);
process.exit(Math.min(failed, 250));
