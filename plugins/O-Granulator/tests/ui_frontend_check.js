/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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
    O-Granulator — WebView frontend static closure (Stage 3 / Phase 3.1,
    PLAN-3.1 P109, RESEARCH-3.1 R104 / R113 layer 1).

    Ported from plugins/O-ReverseDelay/tests/ui_frontend_check.js (§3 bridge
    closure, §11 member order, §15 four-way closure), adapted to this plugin:
    the page is ONE file (inline module controller), the editor binds 220
    parameters through three ID tables, and the APVTS side comes from a FRESH
    O-Granulator-param-dump run (createParameterLayout is table-driven — a
    regex over PluginProcessor.cpp finds nothing).

    Every class pinned here ships green through ninja, auval and pluginval:

      1. The inline module parses (node --check); a SyntaxError kills the UI.
      2. TDZ discipline: exactly one top-level init() call, the LAST statement
         (pattern_module_toplevel_init_tdz).
      3. Bridge closure: getNativeFunction names (page + preset-manager.js) ==
         withNativeFunction names == 21, both directions
         (pattern_webview_native_fn_bridge_gap); the async dialog fns hoist a
         SafePointer and bare-return on a dead editor.
      4. The `Juce` ES-module namespace is imported and passed to the binders
         (critical_juce_webview_namespace_vs_postmessage).
      5. No viewport units anywhere in the page.
      6. Resource closure: every juce_add_binary_data SOURCE has a
         granulatorUiResource branch and vice versa; bare paths only, no scheme.
      7. Source/ui/public/modules/* is not tracked (gitignored build artefact).
      8. Four-way closure by ID AND TYPE, both directions, no duplicates:
         fresh param-dump == committed .planning/params.tsv == editor
         kSliderIds / kComboIds / kToggleIds (relay + attachment classes per
         table) == page tables; every page id has its element.
      9. Editor member order relays → webView → attachments → webView2RuntimeLink,
         the constructor creates them in the same order, every webView-> use
         outside the constructor is null-guarded, never setVisible(false) on it.
     10. Windows block: withBackend (webview2), withUserDataFolder (created
         first), areOptionsSupported probe; both CMake WebView2 flags.
     11. Geometry: setSize (1200, 800) + setResizable (false, false) == body
         and #plugin 1200 × 800 px.
     12. Telemetry keys the editor can emit (ScaleStateJson.h) ⊆ the keys the
         page reads — a typo'd key silently reads as "default".
     13. The render harness never compiles the editor.

    Usage:
      node plugins/O-Granulator/tests/ui_frontend_check.js
      node plugins/O-Granulator/tests/ui_frontend_check.js --negative-controls
    Exit code = number of failed assertions (0 = all pass). With
    --negative-controls the script re-runs itself on two scratch editor copies
    (one ID moved kSliderIds → kToggleIds; one withNativeFunction removed) and
    exits 0 only if BOTH copies fail the check that targets them.
    --editor-cpp <path> points section 3/8/9 at another PluginEditor.cpp.
    --param-dump <path> overrides the param-dump binary (default: the build tree).

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');

const argv = process.argv.slice(2);
const argVal = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };

const htmlPath      = path.join(pluginRoot, 'Source', 'ui', 'public', 'index.html');
const editorCppPath = argVal('--editor-cpp') || path.join(pluginRoot, 'Source', 'PluginEditor.cpp');
const editorHPath   = path.join(pluginRoot, 'Source', 'PluginEditor.h');
const jsonHPath     = path.join(pluginRoot, 'Source', 'ScaleStateJson.h');
const procCppPath   = path.join(pluginRoot, 'Source', 'PluginProcessor.cpp');
const cmakePath     = path.join(pluginRoot, 'CMakeLists.txt');
const harnessPath   = path.join(pluginRoot, 'tests', 'render-harness', 'main.cpp');
const tsvPath       = path.join(pluginRoot, '.planning', 'params.tsv');
const presetJsPath  = path.join(repoRoot, 'modules', 'persistence', 'preset-manager', 'js', 'preset-manager.js');
const paramDumpBin  = argVal('--param-dump') || path.join(repoRoot, 'build', 'plugins', 'O-Granulator',
    'O-Granulator-param-dump_artefacts', 'Release', 'O-Granulator-param-dump');

// ───────────────────────────────────────────── negative controls (P109) ──
if (argv.includes('--negative-controls')) {
    const src = fs.readFileSync(editorCppPath, 'utf8');
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'ogran-ufc-nc-'));
    const variants = [
        {
            name: 'NC1 one ID moved kSliderIds -> kToggleIds',
            expect: /FAIL: .*\[8\]/,
            make: (s) => s.replace(/(kSliderIds \{[\s\S]*?)\n\s*"s2_drive",/, '$1')
                          .replace(/(kToggleIds \{\n)/, '$1        "s2_drive",\n'),
        },
        {
            name: 'NC2 one withNativeFunction removed (getWindowCurve)',
            expect: /FAIL: .*\[3\]/,
            make: (s) => s.replace('withNativeFunction ("getWindowCurve"', 'withNativeFunction_REMOVED ("getWindowCurve"'),
        },
    ];
    let bad = 0;
    for (const v of variants) {
        const mutated = v.make(src);
        if (mutated === src) { console.log(`  FAIL: ${v.name} — the mutation did not apply`); ++bad; continue; }
        const p = path.join(dir, 'PluginEditor.cpp');
        fs.writeFileSync(p, mutated);
        const r = spawnSync(process.execPath, [__filename, '--editor-cpp', p], { encoding: 'utf8' });
        const failedTarget = v.expect.test(r.stdout);
        const ok = r.status !== 0 && failedTarget;
        console.log(`  ${ok ? 'PASS' : 'FAIL'}: ${v.name} -> exit ${r.status}, targeted check ${failedTarget ? 'FAILED (as it must)' : 'did NOT fail'}`);
        if (!ok) ++bad;
    }
    fs.rmSync(dir, { recursive: true, force: true });
    console.log(bad === 0 ? 'negative controls: both copies FAIL the closure — the gate is live'
                          : `negative controls: ${bad} did not fail`);
    process.exit(bad);
}

const html      = fs.readFileSync(htmlPath, 'utf8');
const editorCpp = fs.readFileSync(editorCppPath, 'utf8');
const editorH   = fs.readFileSync(editorHPath, 'utf8');
// C/C++ source with comments removed (for "never mentions X" checks).
const stripC = (src) => src.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
const jsonH     = fs.readFileSync(jsonHPath, 'utf8');
const procCpp   = fs.readFileSync(procCppPath, 'utf8');
const cmake     = fs.readFileSync(cmakePath, 'utf8');
const harness   = fs.readFileSync(harnessPath, 'utf8');
const presetJs  = fs.readFileSync(presetJsPath, 'utf8');

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}
const setDiff = (a, b) => [...a].filter((x) => !b.has(x));
const dups = (arr) => arr.filter((x, i) => arr.indexOf(x) !== i);
const show = (arr) => (arr.length ? ` — ${arr.slice(0, 6).join(', ')}${arr.length > 6 ? ', …' : ''}` : '');

console.log('== O-Granulator ui_frontend_check ==');
console.log(`   editor: ${path.relative(repoRoot, editorCppPath)}`);

// The inline controller (the only <script type="module">).
const modMatch = html.match(/<script type="module">([\s\S]*?)<\/script>/);
const moduleSrc = modMatch ? modMatch[1] : '';

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
    const pageNames = new Set([...moduleSrc.matchAll(/getNativeFunction\(\s*'([A-Za-z]+)'\s*\)/g)].map((m) => m[1]));
    const pmNames   = new Set([...presetJs.matchAll(/getNativeFunction\(\s*"([A-Za-z]+)"\s*\)/g)].map((m) => m[1]));
    const used = new Set([...pageNames, ...pmNames]);
    const regList = [...editorCpp.matchAll(/\.withNativeFunction\s*\(\s*"([A-Za-z]+)"/g)].map((m) => m[1]);
    const reg = new Set(regList);
    check(pageNames.size === 11, `[3] page calls 11 native fns (found ${pageNames.size})`);
    check(pmNames.size === 10, `[3] preset-manager.js calls 10 native fns (found ${pmNames.size})`);
    check(dups(regList).length === 0, '[3] no native fn registered twice' + show(dups(regList)));
    const unreg = setDiff(used, reg), unused = setDiff(reg, used);
    check(unreg.length === 0, `[3] every page / preset-manager fn is registered (withNativeFunction)` + show(unreg));
    check(unused.length === 0, `[3] every registered fn is called by the page or preset-manager.js` + show(unused));
    check(reg.size === 21 && used.size === 21, `[3] surface is exactly 21 = 21 (registered ${reg.size}, used ${used.size})`);

    // Async file dialogs: SafePointer hoisted to a local, captured by copy; bare return on a dead editor.
    const launches = (editorCpp.match(/launchAsync\s*\(/g) || []).length;
    const hoisted  = (editorCpp.match(/auto safeThis = juce::Component::SafePointer<GranulatorEditor> \(this\);/g) || []).length;
    const bare     = (editorCpp.match(/if \(safeThis == nullptr\)\s*\n?\s*return;/g) || []).length;
    check(launches === 4 && hoisted === 4 && bare === 4,
        `[3] 4 launchAsync dialogs: SafePointer hoisted (${hoisted}) and bare-return on null (${bare})`);
}

// ─────────────────────────────────────────────────── 4. Juce namespace ──
console.log('-- [4] Juce namespace');
{
    check(/import \* as Juce from '\.\/js\/juce\/index\.js';/.test(moduleSrc), "[4] import * as Juce from './js/juce/index.js'");
    check(/bindKnob\(Juce, id\)/.test(moduleSrc) && /initMatrix\(Juce\)/.test(moduleSrc) && /initTelemetry\(Juce\)/.test(moduleSrc),
        '[4] the binders receive the Juce namespace');
    check(!/window\.__JUCE__\.backend\.emitEvent|postMessage\(/.test(moduleSrc), '[4] no raw __JUCE__ backend / postMessage path');
}

// ───────────────────────────────────────────────────── 5. viewport units ──
console.log('-- [5] viewport units');
{
    const vp = html.match(/\b\d+(?:\.\d+)?(?:vh|vw|dvh|svh|lvh|vmin|vmax)\b/g) || [];
    check(vp.length === 0, `[5] no viewport units in index.html` + show(vp));
}

// ─────────────────────────────────────────────────── 6. resource closure ──
console.log('-- [6] resource closure');
{
    const block = (cmake.match(/juce_add_binary_data\s*\(\s*OuariconGranulator_UIResources([\s\S]*?)\)/) || [])[1] || '';
    const sources = [...block.matchAll(/^\s*(\S+\.(?:html|js|css|woff2))\s*$/gm)].map((m) => m[1]);
    // juce_add_binary_data symbol: basename, '.' and '-' rules (hyphens STRIPPED).
    const sym = (p) => path.basename(p).replace(/-/g, '').replace(/[^A-Za-z0-9_]/g, '_');
    const want = new Set(sources.map(sym));
    const fnStart = editorCpp.indexOf('granulatorUiResource (const juce::String& url)\n{');
    const fnBody = fnStart >= 0 ? editorCpp.slice(fnStart, editorCpp.indexOf('\n}\n', fnStart)) : '';
    const have = new Set([...fnBody.matchAll(/UIBinaryData::([A-Za-z0-9_]+?)(?:Size)?\b/g)].map((m) => m[1]));
    check(sources.length === 9, `[6] 9 juce_add_binary_data SOURCES (found ${sources.length})`);
    check(setDiff(want, have).length === 0, '[6] every SOURCE has a granulatorUiResource branch' + show(setDiff(want, have)));
    check(setDiff(have, want).length === 0, '[6] every branch serves a SOURCE' + show(setDiff(have, want)));
    const urls = [...fnBody.matchAll(/url == "([^"]+)"/g)].map((m) => m[1]);
    check(urls.length >= 10 && urls.every((u) => u.startsWith('/')), `[6] bare-path comparisons only (${urls.length})`);
    check(!/juce:\/\/|https:\/\/juce\.backend/.test(editorCpp + moduleSrc), '[6] no hard-coded juce:// or https://juce.backend scheme');
    check(/withResourceProvider \(\[\] \(const auto& url\) \{ return granulatorUiResource \(url\); \}\)/.test(editorCpp),
        '[6] withResourceProvider -> granulatorUiResource (no duplicate member map)');
}

// ───────────────────────────────────────────────── 7. modules untracked ──
console.log('-- [7] module copies untracked');
{
    const r = spawnSync('git', ['ls-files', 'plugins/O-Granulator/Source/ui/public/modules'], { cwd: repoRoot, encoding: 'utf8' });
    check(r.status === 0 && r.stdout.trim() === '', '[7] Source/ui/public/modules/* is not tracked' + (r.stdout.trim() ? `: ${r.stdout.trim()}` : ''));
}

// ───────────────────────────────────────── 8. four-way closure, ID + TYPE ──
console.log('-- [8] four-way closure (param-dump == params.tsv == editor tables == page tables), by ID and TYPE');
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
            out.set(id, /\bboolean\b/.test(flags) ? 'bool' : /\bdiscrete\b/.test(flags) ? 'choice' : 'float');
        }
        return out;
    };
    let fresh = null;
    if (fs.existsSync(paramDumpBin)) {
        const r = spawnSync(paramDumpBin, [], { encoding: 'utf8', maxBuffer: 1 << 24 });
        if (r.status === 0) fresh = r.stdout;
    }
    check(fresh !== null, `[8] fresh O-Granulator-param-dump ran (${path.relative(repoRoot, paramDumpBin)})`);
    const committed = fs.readFileSync(tsvPath, 'utf8');
    check(fresh !== null && fresh === committed, '[8] fresh param-dump == committed .planning/params.tsv (byte-identical)');
    const apvts = parseTsv(fresh || committed);
    check(apvts.size === 220, `[8] APVTS has 220 parameters (dump: ${apvts.size})`);

    const table = (name) => {
        const m = editorCpp.match(new RegExp(`const juce::StringArray ${name} \\{([\\s\\S]*?)\\};`));
        return m ? [...m[1].matchAll(/"([a-z0-9_]+)"/g)].map((x) => x[1]) : [];
    };
    const ed = { float: table('kSliderIds'), choice: table('kComboIds'), bool: table('kToggleIds') };
    check(ed.float.length === 140 && ed.choice.length === 59 && ed.bool.length === 21,
        `[8] editor tables 140 / 59 / 21 (found ${ed.float.length} / ${ed.choice.length} / ${ed.bool.length})`);
    // Each table feeds the relay + attachment class of its type.
    check(/for \(const auto& id : kSliderIds\)\s*\n\s*sliderRelays\.push_back \(std::make_unique<juce::WebSliderRelay> \(id\)\);/.test(editorCpp)
       && /for \(const auto& id : kComboIds\)\s*\n\s*comboRelays\.push_back \(std::make_unique<juce::WebComboBoxRelay> \(id\)\);/.test(editorCpp)
       && /for \(const auto& id : kToggleIds\)\s*\n\s*toggleRelays\.push_back \(std::make_unique<juce::WebToggleButtonRelay> \(id\)\);/.test(editorCpp),
        '[8] kSliderIds -> WebSliderRelay, kComboIds -> WebComboBoxRelay, kToggleIds -> WebToggleButtonRelay');
    check(/kSliderIds\[i\][\s\S]{0,300}WebSliderParameterAttachment/.test(editorCpp)
       && /kComboIds\[i\][\s\S]{0,300}WebComboBoxParameterAttachment/.test(editorCpp)
       && /kToggleIds\[i\][\s\S]{0,300}WebToggleButtonParameterAttachment/.test(editorCpp),
        '[8] each table attaches with its own attachment class');
    for (const t of ['kSliderIds', 'kComboIds', 'kToggleIds'])
        check(new RegExp(`for \\(const auto& relay : ${t.replace('k', '').replace('Ids', '').toLowerCase()}Relays\\)\\s*\\n\\s*options = options\\.withOptionsFrom \\(\\*relay\\);`).test(editorCpp),
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
    const range = (n) => Array.from({ length: n }, (_, i) => i + 1);
    const pg = {
        knob: arr('KNOB_IDS'), fader: arr('FADER_IDS'), choiceKnob: arr('CHOICE_KNOB_IDS'),
        segment: arr('SEGMENT_IDS'), select: arr('SELECT_IDS'), toggle: arr('TOGGLE_IDS'),
    };
    check(slots === 16 && matrixOk, `[8] initMatrix binds mod1..16 source / dest (select) + depth (fader) (MOD_SLOTS ${slots})`);
    check(scaleIds.length === 4, '[8] initScaleBand binds s1..s4_scale as ComboBoxStates');
    check(JSON.stringify(pg.fader) === JSON.stringify(range(16).map((m) => `mod${m}_depth`)),
        '[8] FADER_IDS == mod1..16_depth (the ids initMatrix fader-binds)');
    // The binders init() actually calls for each table.
    for (const [call, what] of [['KNOB_IDS.forEach((id) => bindKnob(Juce, id));', 'KNOB_IDS -> bindKnob'],
                                ['CHOICE_KNOB_IDS.forEach((id) => bindChoiceKnob(Juce, id));', 'CHOICE_KNOB_IDS -> bindChoiceKnob'],
                                ['TOGGLE_IDS.forEach((id) => bindToggle(Juce, id));', 'TOGGLE_IDS -> bindToggle'],
                                ['SEGMENT_IDS.forEach((id) => bindSegmented(Juce, id));', 'SEGMENT_IDS -> bindSegmented'],
                                ['SELECT_IDS.forEach((id) => bindSelectCombo(Juce, id));', 'SELECT_IDS -> bindSelectCombo']])
        check(moduleSrc.includes(call), `[8] init() runs ${what}`);
    const page = {
        float: [...pg.knob, ...pg.fader],
        choice: [...pg.choiceKnob, ...pg.segment, ...pg.select, ...scaleIds,
                 ...range(slots).flatMap((m) => [`mod${m}_source`, `mod${m}_dest`])],
        bool: pg.toggle,
    };
    const allPage = [...page.float, ...page.choice, ...page.bool];
    const allEd = [...ed.float, ...ed.choice, ...ed.bool];
    check(dups(allPage).length === 0, `[8] no page id bound twice (${allPage.length})` + show(dups(allPage)));
    check(dups(allEd).length === 0, `[8] no editor id listed twice (${allEd.length})` + show(dups(allEd)));
    for (const ty of ['float', 'choice', 'bool']) {
        const A = new Set([...apvts].filter(([, t]) => t === ty).map(([id]) => id));
        const E = new Set(ed[ty]), P = new Set(page[ty]);
        check(setDiff(A, E).length === 0 && setDiff(E, A).length === 0,
            `[8] ${ty}: APVTS == editor (${A.size} / ${E.size})` + show([...setDiff(A, E).map((x) => `-${x}`), ...setDiff(E, A).map((x) => `+${x}`)]));
        check(setDiff(A, P).length === 0 && setDiff(P, A).length === 0,
            `[8] ${ty}: APVTS == page (${A.size} / ${P.size})` + show([...setDiff(A, P).map((x) => `-${x}`), ...setDiff(P, A).map((x) => `+${x}`)]));
    }
    // Every page id has the element its binder looks up.
    const has = (id) => html.includes(`id="${id}"`);
    const missing = [
        ...[...pg.knob, ...pg.choiceKnob].filter((id) => !has(`knob-${id}`) || !has(`val-${id}`)),
        ...pg.fader.filter((id) => !has(`fader-${id}`) || !has(`val-${id}`)),
        ...pg.toggle.filter((id) => !has(`toggle-${id}`)),
        ...pg.segment.filter((id) => !has(`seg-${id}`)),
        ...[...pg.select, ...range(slots).flatMap((m) => [`mod${m}_source`, `mod${m}_dest`])].filter((id) => !has(`combo-${id}`)),
        ...[1, 2, 3, 4].filter((n) => !has(`scale-trig-s${n}`) || !has(`scale-name-s${n}`)).map((n) => `scale-trig-s${n}`),
    ];
    check(missing.length === 0, '[8] every bound id has its element (knob/val, fader, toggle, seg, combo, scale trigger)' + show(missing));
    const fmt = (moduleSrc.match(/const FORMAT = \{([\s\S]*?)\n    \};/) || [])[1] || '';
    const fmtKeys = new Set([...fmt.matchAll(/^\s*([a-z0-9_]+):/gm)].map((m) => m[1]));
    const noFmt = page.float.filter((id) => !fmtKeys.has(id));
    check(noFmt.length === 0 && page.float.length === 140,
        `[8] every float id has a FORMAT entry (${fmtKeys.size} keys)` + show(noFmt));
}

// ──────────────────────────────────────────────────── 9. member order ──
console.log('-- [9] editor member order and WebView lifetime');
{
    const pos = (s, re) => { const m = s.match(re); return m ? m.index : -1; };
    const h = {
        sr: pos(editorH, /std::vector<std::unique_ptr<juce::WebSliderRelay>>\s+sliderRelays;/),
        cr: pos(editorH, /std::vector<std::unique_ptr<juce::WebComboBoxRelay>>\s+comboRelays;/),
        tr: pos(editorH, /std::vector<std::unique_ptr<juce::WebToggleButtonRelay>>\s+toggleRelays;/),
        wv: pos(editorH, /std::unique_ptr<juce::WebBrowserComponent> webView;/),
        sa: pos(editorH, /sliderAttachments;/),
        ca: pos(editorH, /comboAttachments;/),
        ta: pos(editorH, /toggleAttachments;/),
        ln: pos(editorH, /std::unique_ptr<juce::HyperlinkButton> webView2RuntimeLink;/),
    };
    check(Object.values(h).every((v) => v >= 0), '[9] all eight members declared');
    check(h.sr < h.cr && h.cr < h.tr && h.tr < h.wv && h.wv < h.sa && h.sa < h.ca && h.ca < h.ta && h.ta < h.ln,
        '[9] header order: relays (slider, combo, toggle) -> webView -> attachments -> webView2RuntimeLink');
    const ctorStart = editorCpp.indexOf('GranulatorEditor::GranulatorEditor (GranulatorProcessor& p)');
    const ctorEnd = editorCpp.indexOf('GranulatorEditor::~GranulatorEditor()');
    const ctor = editorCpp.slice(ctorStart, ctorEnd);
    const c = {
        relay: pos(ctor, /sliderRelays\.push_back/),
        opts: pos(ctor, /withOptionsFrom \(\*relay\)/),
        wv: pos(ctor, /webView = std::make_unique<juce::WebBrowserComponent> \(options\);/),
        att: pos(ctor, /sliderAttachments\.push_back/),
        link: pos(ctor, /webView2RuntimeLink = std::make_unique<juce::HyperlinkButton>/),
    };
    check(ctorStart >= 0 && Object.values(c).every((v) => v >= 0) && c.relay < c.opts && c.opts < c.wv && c.wv < c.att && c.att < c.link,
        '[9] constructor order: relays -> options -> webView -> attachments -> runtime link');
    const outside = editorCpp.slice(0, ctorStart) + editorCpp.slice(ctorEnd);
    const uses = outside.split('\n').map((l, i, all) => ({ l, i, all })).filter((x) => /webView->/.test(x.l));
    const unguarded = uses.filter((x) => !x.all.slice(Math.max(0, x.i - 3), x.i + 1).some((p) => /if \(webView != nullptr\)/.test(p)));
    check(unguarded.length === 0, `[9] every webView-> use outside the constructor is null-guarded (${uses.length})` + show(unguarded.map((x) => x.l.trim())));
    check(!/webView->setVisible \(false\)|webView->setVisible\(false\)/.test(editorCpp), '[9] never setVisible(false) on the WebView');
    check(/withKeepPageLoadedWhenBrowserIsHidden\(\)/.test(editorCpp) && /withNativeIntegrationEnabled\(\)/.test(editorCpp),
        '[9] native integration + keep-page-loaded-when-hidden');
    check(/GranulatorEditor::~GranulatorEditor\(\) = default;/.test(editorCpp), '[9] destructor = default (members die in reverse order)');
}

// ──────────────────────────────────────────────────── 10. Windows block ──
console.log('-- [10] Windows WebView2');
{
    const win = (editorCpp.match(/#if JUCE_WINDOWS([\s\S]*?)#else/) || [])[1] || '';
    check(/\.withBackend \(juce::WebBrowserComponent::Options::Backend::webview2\)/.test(win), '[10] withBackend (webview2)');
    check(/\.withUserDataFolder \(webView2DataFolder\)/.test(win) && /webView2DataFolder\.createDirectory\(\);/.test(win)
       && win.indexOf('createDirectory') < win.indexOf('withUserDataFolder')
       && /userApplicationDataDirectory/.test(win) && !/tempDirectory/.test(win),
        '[10] withUserDataFolder (app-data/Ouaricon/WebView2/O-Granulator, created first; never %TEMP%)');
    check(/areOptionsSupported \(options\)/.test(win), '[10] areOptionsSupported probe');
    check(/NEEDS_WEBVIEW2 TRUE/.test(cmake) && /JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1/.test(cmake)
       && /NEEDS_WEB_BROWSER TRUE/.test(cmake) && /JUCE_WEB_BROWSER=1/.test(cmake),
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
    const fnStart = jsonH.indexOf('inline juce::var telemetryToVar');
    const body = jsonH.slice(fnStart);
    const emitted = new Set([...body.matchAll(/(?:obj|p)->setProperty \("([A-Za-z]+)"/g)].map((m) => m[1]));
    const read = new Set([...moduleSrc.matchAll(/\b(?:t|telemetry|p)\.([A-Za-z]+)\b/g)].map((m) => m[1]));
    const unread = setDiff(emitted, read);
    check(emitted.size >= 20, `[12] telemetryToVar emits ${emitted.size} keys`);
    check(unread.length === 0, '[12] every key the editor can emit is read by the page' + show(unread));
    check(/telemetryToVar \(t, ! ok\)/.test(editorCpp), '[12] the getTelemetry native uses the shared builder');
    check(/ogran::ui::buildScaleState \(processorRef\)/.test(editorCpp) && /proc\.getCustomTable\(\)/.test(jsonH),
        '[12] getScaleState uses the shared builder; collection 31 via getCustomTable() (R109)');
}

// ─────────────────────────────────────────────── 13. headless isolation ──
console.log('-- [13] render harness never sees the editor');
{
    check(!/#include\s+"PluginEditor/.test(stripC(harness)) && !/GranulatorEditor/.test(stripC(harness)), '[13] render-harness main.cpp does not include PluginEditor');
    check(/#if JUCE_WEB_BROWSER\s*\n\s*#include "PluginEditor\.h"/.test(procCpp)
       && /#if JUCE_WEB_BROWSER\s*\n\s*return new GranulatorEditor \(\*this\);/.test(procCpp),
        '[13] PluginProcessor.cpp includes / creates the editor only under JUCE_WEB_BROWSER');
    check(!/#include\s+<juce_gui_extra|WebBrowserComponent/.test(stripC(jsonH)), '[13] ScaleStateJson.h has no WebBrowserComponent types');
}

console.log(failed === 0 ? '\nALL PASS' : `\n${failed} FAILED`);
process.exit(failed);
