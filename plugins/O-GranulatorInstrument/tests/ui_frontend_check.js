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
     12. Telemetry keys telemetryToVar emits (Source/GiUiJson.h) are a subset of
         the keys the page reads, per object (top level, lanes[], rec); the
         stub fixture's getTelemetry has the same top-level keys.
     13. Headless isolation: the render harness never compiles the editor; the
         processor includes / creates it only under JUCE_WEB_BROWSER.
     14. No DevLoader / OGI_DEV_LOADER token under Source/ (ui/public/modules
         excluded) or in CMakeLists.txt.
     15. Phase 3.2 closure (PLAN-3.2 PD26): every 3.2 key is EMITTED by its
         C++ encoder (Source/GiUiJson.h: the UiTelemetry telemetryToVar,
         slotInfoToVar, slotOverviewToVar) AND READ by the page function that
         uses it; no "unavailable" answer left in the editor; the three lane
         natives are live; both failure toasts exist in en / fr / zh-Hans and
         are raised by the page.

    Usage (from anywhere):
      node plugins/O-GranulatorInstrument/tests/ui_frontend_check.js
      node plugins/O-GranulatorInstrument/tests/ui_frontend_check.js --negative-controls   (alias --nc)
    Exit code = number of failed assertions (0 = all pass). With
    --negative-controls the script re-runs itself on three scratch copies (an
    editor with one ID moved kSliderIds -> kToggleIds; an editor with one
    withNativeFunction removed; a GiUiJson.h whose 3.2 encoder emits "grains"
    under another name) and exits 0 only if EVERY copy fails the check that
    targets it. Scratch copies live under os.tmpdir() (set TMPDIR to move
    them) and are removed.
    --editor-cpp <path>  point sections 3 / 6 / 8 / 9 / 10 / 12 / 15 at another PluginEditor.cpp
    --json-h <path>      point sections 12 / 13 / 15 at another GiUiJson.h
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
            name: 'NC3 one telemetry key renamed in the 3.2 encoder (grains -> grainRows)',
            file: 'jsonH',
            expect: /FAIL: \[15\] emitted and read: grains\b/,
            make: (s) => s.replace('obj->setProperty ("grains", grains);', 'obj->setProperty ("grainRows", grains);'),
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
    const fnStart = jsonH.indexOf('inline juce::var telemetryToVar (std::uint32_t');
    const fnEnd = fnStart >= 0 ? jsonH.indexOf('return juce::var (obj);', fnStart) : -1;
    const body = fnStart >= 0 && fnEnd > fnStart ? jsonH.slice(fnStart, fnEnd) : '';
    const keysOf = (recv) => new Set([...body.matchAll(new RegExp(`\\b${recv}->setProperty \\("([A-Za-z]+)"`, 'g'))].map((m) => m[1]));
    const emitted = { top: keysOf('obj'), lane: keysOf('o'), rec: keysOf('rec') };
    const total = (body.match(/->setProperty \("/g) || []).length;
    check(emitted.top.size === 13 && emitted.lane.size === 3 && emitted.rec.size === 8
       && total === emitted.top.size + emitted.lane.size + emitted.rec.size,
        `[12] telemetryToVar emits ${emitted.top.size} top-level + ${emitted.lane.size} lane + ${emitted.rec.size} rec keys (${total} setProperty calls)`);
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
    const otherReaders = edCode.match(/\breadUiFrame\b|\btakeScPeak\b|\bframes\s*\.\s*read\b/g) || [];
    check(builds === 1 && otherReaders.length === 0,
        `[12] buildUiTelemetry is called exactly once in the editor (${builds}) and no other frame / sidechain-peak reader exists (${otherReaders.length})` + show(otherReaders));
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
    check(!!tel32 && !!tel31 && !!slotFn && !!ovFn, '[15] the four encoders are found in GiUiJson.h (3.2 telemetry, 3.1 placeholder, slot, overview)');
    const keys = (body, recv) => new Set([...body.matchAll(new RegExp(`\\b${recv}->setProperty \\("([A-Za-z]+)"`, 'g'))].map((m) => m[1]));
    const e32 = { top: keys(tel32, 'obj'), lane: keys(tel32, 'o'), rec: keys(tel32, 'rec') };
    const e31 = { top: keys(tel31, 'obj'), lane: keys(tel31, 'o'), rec: keys(tel31, 'rec') };
    const same = (a, b) => setDiff(a, b).length === 0 && setDiff(b, a).length === 0;
    check(e32.top.size === 13 && e32.lane.size === 3 && e32.rec.size === 8
       && same(e32.top, e31.top) && same(e32.lane, e31.lane) && same(e32.rec, e31.rec),
        `[15] the 3.2 encoder emits the 3.1 key set, nothing renamed or dropped (${e32.top.size} top-level + ${e32.lane.size} lane + ${e32.rec.size} rec)`
        + show([...setDiff(e31.top, e32.top), ...setDiff(e31.lane, e32.lane), ...setDiff(e31.rec, e32.rec)].map((x) => `-${x}`)
            .concat([...setDiff(e32.top, e31.top), ...setDiff(e32.lane, e31.lane), ...setDiff(e32.rec, e31.rec)].map((x) => `+${x}`))));

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

console.log(failed === 0 ? '\nALL PASS' : `\n${failed} FAILED`);
process.exit(Math.min(failed, 250));
