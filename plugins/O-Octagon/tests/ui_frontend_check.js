/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
    O-Octagon — 31 STATIC sections over the WebView UI and the C++ that serves it.

    Every section here is a failure this repo has actually shipped at least once,
    written as a gate instead of as a comment that hopes. None of them is
    reachable by ninja, auval or pluginval: each one builds green, validates
    green, and presents as a dead control, a blank panel or a value that is
    quietly wrong.

    Its sibling tests/ui_layout_check.js drives a real browser and MEASURES the
    rendered page; this file reads source. The split is deliberate — a static
    check cannot prove a box fits, and a rendered check cannot prove the C++ side
    of a bridge is closed.

    Usage:  node plugins/O-Octagon/tests/ui_frontend_check.js
    Exit code = number of failed assertions (0 = all pass).

  ==============================================================================
*/

'use strict';

const fs        = require('fs');
const path      = require('path');
const { spawnSync } = require('child_process');

const pluginRoot = path.resolve(__dirname, '..');
const publicDir  = path.join(pluginRoot, 'Source', 'ui', 'public');

const P = {
    html:        path.join(publicDir, 'index.html'),
    css:         path.join(publicDir, 'css', 'styles.css'),
    appJs:       path.join(publicDir, 'js', 'app.js'),
    roomJs:      path.join(publicDir, 'js', 'roomplan.js'),
    juceJs:      path.join(publicDir, 'js', 'juce', 'index.js'),
    editorCpp:   path.join(pluginRoot, 'Source', 'PluginEditor.cpp'),
    editorH:     path.join(pluginRoot, 'Source', 'PluginEditor.h'),
    procCpp:     path.join(pluginRoot, 'Source', 'PluginProcessor.cpp'),
    gainH:       path.join(pluginRoot, 'Source', 'DSP', 'GainStage.h'),
    cmake:       path.join(pluginRoot, 'CMakeLists.txt'),
    harnessMake: path.join(pluginRoot, 'tests', 'render-harness', 'CMakeLists.txt'),
    stubJs:      path.join(pluginRoot, 'tests', 'ui-stub', 'juce-stub.js'),

    // ── Phase 3.2 ──
    venueJs:     path.join(publicDir, 'js', 'venue.js'),
    venueFileH:  path.join(pluginRoot, 'Source', 'Data', 'VenueFile.h'),
    venueFileCpp:path.join(pluginRoot, 'Source', 'Data', 'VenueFile.cpp'),
    venueModelH: path.join(pluginRoot, 'Source', 'Data', 'VenueModel.h'),
    pingH:       path.join(pluginRoot, 'Source', 'DSP', 'VerifyPing.h'),
    pingCpp:     path.join(pluginRoot, 'Source', 'DSP', 'VerifyPing.cpp'),
    gainCpp:     path.join(pluginRoot, 'Source', 'DSP', 'GainStage.cpp'),
    chanCpp:     path.join(pluginRoot, 'Source', 'DSP', 'ChannelMap.cpp'),
    procH:       path.join(pluginRoot, 'Source', 'PluginProcessor.h'),
};

const S = Object.fromEntries(Object.entries(P).map(([k, v]) => [k, fs.readFileSync(v, 'utf8')]));

let failed = 0;
let section = '';

function head(n, title) {
    section = String(n);
    console.log(`\n-- section ${n}: ${title}`);
}

function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: [${section}] ${desc}`);
    if (!cond) ++failed;
}

const setsEqual = (a, b) => a.size === b.size && [...a].every(v => b.has(v));
const diff = (a, b) => [...a].filter(v => !b.has(v));

// Several sections below ban a TOKEN — `window.__JUCE__`, `juce://`, a float
// literal. Every one of those tokens also appears in the COMMENT explaining why
// it is banned, in the very file being scanned. Scanning raw source therefore
// fails on correct code, which is not a safe failure: the obvious "fix" is to
// delete the explanation. Strip comments first and assert against code only.
const stripComments = src => src
    .replace(/\/\*[\s\S]*?\*\//g, '')
    .replace(/(^|[^:"'\\])\/\/[^\n]*/g, '$1');

const CODE = {
    editorCpp: stripComments(S.editorCpp),
};

// Phase 3.3. Read directly rather than through PAGE_MODULES, because section 6 runs long before
// that list is indexed by name and a missing file must fail there as an empty string rather than
// as a TypeError three sections later.
const ELEVATION_SRC = fs.existsSync(path.join(publicDir, 'js', 'elevation.js'))
    ? fs.readFileSync(path.join(publicDir, 'js', 'elevation.js'), 'utf8')
    : '';

// ────────────────────────────────────────── the page modules, DERIVED (P51) ──
// Every .js authored directly under Source/ui/public/js/ — and NOT js/juce/,
// which is verbatim JUCE and is covered by §9's three-way closure instead.
//
// Seven sections below scan "the shipped JS". Until 3.2 each of them named
// [S.appJs, S.roomJs] literally, so a third module would have made every one of
// them PASS BY NOT LOOKING. That is the sixth time this vacuity class has been
// caught in this repo, and naming a new file in seven places is the fix that has
// to be remembered a seventh time. Deriving the list is the fix that does not —
// the same move kSliderIds makes by building from oo::params::id(i) instead of
// transcribing a table. §21 is what makes the derivation itself trustworthy: a
// directory read that returned nothing would otherwise pass every one of them.
const PAGE_MODULE_DIR = path.join(publicDir, 'js');
const PAGE_MODULES = fs.readdirSync(PAGE_MODULE_DIR)
    .filter(f => f.endsWith('.js'))
    .sort()
    .map(f => {
        const src = fs.readFileSync(path.join(PAGE_MODULE_DIR, f), 'utf8');
        return { name: `js/${f}`, path: `/js/${f}`, src, code: stripComments(src) };
    });

// Extracts a brace-balanced block starting at the first '{' at or after `from`.
function blockAt(src, from) {
    const start = src.indexOf('{', from);
    if (start < 0) return '';
    let depth = 0;
    for (let i = start; i < src.length; ++i) {
        if (src[i] === '{') ++depth;
        else if (src[i] === '}') { if (--depth === 0) return src.slice(start, i + 1); }
    }
    return src.slice(start);
}

console.log('== O-Octagon ui_frontend_check ==');

// ─────────────────────────────────────────────────────────────── 1. syntax ──
head(1, 'the JS parses as ES modules, and every script tag says so');
{
    for (const { name, src } of PAGE_MODULES) {
        const res = spawnSync(process.execPath, ['--input-type=module', '--check'],
                              { input: src, encoding: 'utf8' });
        check(res.status === 0,
            `${name} parses as an ES module — a SyntaxError kills the ENTIRE UI`
            + (res.status !== 0 ? `\n${res.stderr}` : ''));
    }

    const scripts = [...S.html.matchAll(/<script\b([^>]*)>/g)].map(m => m[1]);
    check(scripts.length > 0, `index.html has ${scripts.length} script tag(s)`);
    check(scripts.every(a => /type="module"/.test(a)),
        'every script tag is type="module" (a classic script cannot import)');
}

// ────────────────────────────────────────────────────────────────── 2. TDZ ──
head(2, 'a single init() call, and it is the LAST statement');
{
    // A top-level statement that reaches a not-yet-initialised binding throws a
    // ReferenceError out of module evaluation and silently kills every later
    // initializer — the whole UI, not the one line that touched it
    // (pattern_module_toplevel_init_tdz).
    const initCalls = S.appJs.match(/^init\(\);\s*$/gm) || [];
    check(initCalls.length === 1, `exactly one top-level init() call — found ${initCalls.length}`);

    const lastMeaningful = S.appJs.split('\n').map(l => l.trim())
        .filter(l => l && !l.startsWith('//')).pop();
    check(lastMeaningful === 'init();',
        `init() is the last statement in app.js — last line is "${lastMeaningful}"`);

    const initIdx = S.appJs.search(/^init\(\);\s*$/m);
    check(!S.appJs.slice(initIdx).match(/^(?:const|let|var)\s/m),
        'no module-level declarations after the init() call');

    // roomplan.js is a pure module: it must export and never self-execute.
    check(!/^\s*createRoomPlan\(/m.test(S.roomJs),
        'roomplan.js has no top-level call of its own — app.js initialises it inside init()');
    check(/roomPlan = createRoomPlan\(/.test(S.appJs) && /try\s*\{[\s\S]{0,400}createRoomPlan\(/.test(S.appJs),
        'the plan is created INSIDE a try/catch, so a failed plan cannot take the 18 bindings down');
}

// ───────────────────────────────────────────────── 3. native-fn bridge gap ──
head(3, 'bridge closure in BOTH directions; the surface is exactly THIRTEEN (P65)');
{
    // An unregistered fn is a silently dead control that passes build, auval AND
    // pluginval (pattern_webview_native_fn_bridge_gap). Diffed both ways: a name
    // called and not registered is a dead control; registered and not called is
    // dead weight that will be mistaken for a contract.
    const called = new Set();
    for (const { src } of PAGE_MODULES) {
        for (const m of src.matchAll(/getNativeFunction\(\s*["']([A-Za-z0-9_]+)["']/g)) called.add(m[1]);
        for (const m of src.matchAll(/nativeFn\(\s*["']([A-Za-z0-9_]+)["']/g)) called.add(m[1]);
    }

    const registered = new Set();
    for (const m of S.editorCpp.matchAll(/withNativeFunction\s*\(\s*"([A-Za-z0-9_]+)"/g))
        registered.add(m[1]);

    const stubbed = new Set();
    const fnsBlock = S.stubJs.match(/const NATIVE_FNS = \{([\s\S]*?)\n\};/);
    if (fnsBlock) for (const m of fnsBlock[1].matchAll(/^\s{2}([A-Za-z0-9_]+):/gm)) stubbed.add(m[1]);

    // 3 -> 13 at Phase 3.2 (P65), 13 -> 18 at Phase 3.3 (P81), 18 -> 20 at v1.1.0. This literal
    // MOVES EVERY TIME and FAILS LOUDLY until every one of the twenty exists in all three places,
    // which is the only way a count assertion is worth anything: a count that silently tracked
    // whatever was registered would assert nothing at all.
    //
    // The five 3.3 ones are getMeters, getScenes, applyScene, storeScene and getFieldGrid. UI-05
    // adds NONE — getVenueGeometry already carries per-speaker z, rake.front/rear, the bbox and
    // the centroid (3.2's P55), and named-scene membership rides that same payload (P79).
    //
    // v1.1.0 adds the two WRITES of the speaker→output surface: assignSpeakerOutput (the Room
    // plan's double-click popover) and applyOutputOrderPreset (the Venue rail's one-click sets).
    // The READ side rides getVenueGeometry as per-speaker `output` — no new read call.
    //
    // v1.2.0 adds the hover-help ("?" toggle) persistence pair: setTooltipsEnabled (the toggle's
    // click) and getTooltipsEnabled (PULLED by the page at init — a C++ push would fire before
    // the module evaluated and silently never arrive).
    // v1.4.0 adds ONE: applySuggestedDelays, the Delay row's Derive button. The delays
    // themselves ride getVenueGeometry as per-speaker `delayMs` (P55's argument, a third time)
    // and the ms/metres toggle is a pure view state, so neither adds a call.
    check(registered.size === 23,
        `PluginEditor.cpp registers exactly 23 native functions — ${registered.size}: ${[...registered].sort().join(', ')}`);
    check(setsEqual(called, registered),
        `JS calls == C++ registers${setsEqual(called, registered) ? ''
            : ` — called-not-registered: [${diff(called, registered)}], registered-not-called: [${diff(registered, called)}]`}`);
    check(setsEqual(stubbed, registered),
        `the ui-stub whitelist == the C++ surface${setsEqual(stubbed, registered) ? ''
            : ` — stub-only: [${diff(stubbed, registered)}], cpp-only: [${diff(registered, stubbed)}]`}`);

    // Rejecting an unknown name is how a bridge gap surfaces in the stub instead
    // of as a dead control in a DAW.
    check(/return \(\) => Promise\.reject\(/.test(S.stubJs),
        'an UNKNOWN native-function name REJECTS in the stub rather than resolving');
}

// ───────────────────────────────────────────────────── 4. readout provenance ──
head(4, 'readouts come from getScaledValue(); the FORMAT table carries no range constants');
{
    // A JS min/max map drifts from the C++ NormalisableRange and the drift is
    // invisible (pattern_webview_knob_readout_scaled_value).
    check(/getScaledValue\(\)/.test(S.appJs), 'app.js reads values through getScaledValue()');

    const fmt = S.appJs.match(/const FORMAT = \{([\s\S]*?)\n\};/);
    check(fmt !== null, 'the FORMAT table is present and parseable');

    if (fmt) {
        const keys = new Set([...fmt[1].matchAll(/\{\s*([^}]*)\}/g)]
            .flatMap(m => [...m[1].matchAll(/([A-Za-z0-9_]+)\s*:/g)].map(k => k[1])));
        check(setsEqual(keys, new Set(['unit', 'dp'])),
            `FORMAT entries carry ONLY unit + dp — found {${[...keys].sort().join(', ')}}`);
        check(!/\b(start|end|skew|min|max)\s*:/.test(fmt[1]),
            'no start / end / skew / min / max anywhere in the FORMAT table');
    }

    // Range and skew must be read from the state's own properties, nowhere else.
    check(/state\.properties/.test(S.appJs),
        'the dblclick round trip converts through the LIVE properties, not a JS range');
}

// ──────────────────────────────────────────────────────── 5. dblclick reset ──
head(5, 'dblclick reset reads getParameterDefaults, never a JS default table');
{
    check(/paramDefaults = await nativeFn\("getParameterDefaults"\)\(\)/.test(S.appJs),
        'defaults are fetched from the plugin');
    check(/dblclick/.test(S.appJs) && /paramDefaults\[id\]/.test(S.appJs),
        'the dblclick handler resolves its value out of that payload');

    // The specific trap this closes: w1..w8 default to 1.0, not to their range
    // minimum, and blur/airAmount are non-endpoint too. A JS table gets those
    // three wrong quietly.
    check(!/const\s+DEFAULTS\s*=/.test(S.appJs) && !/const\s+RANGES\s*=/.test(S.appJs),
        'app.js contains no DEFAULTS and no RANGES table of its own');
}

// ─────────────────────────────────────────────────── 6. HTML-authored labels ──
head(6, 'HTML-authored labels are never written via textContent');
{
    // A shared JS state updater writing textContent erases HTML-authored labels
    // and passes every build gate (pattern_js_state_updater_overwrites_html_labels).
    const receivers = new Set([...PAGE_MODULES.map(m => m.src).join('\n')
        .matchAll(/([A-Za-z_$][A-Za-z0-9_$]*)\.textContent\s*=/g)].map(m => m[1]));

    // AN EXPLICIT WHITELIST, REVIEWED WHEN IT GROWS — the property being protected is that no
    // AUTHORED text is ever overwritten, not that only two identifiers exist. 3.3 adds the
    // elevation strip's two readings (UI-05/2), which are .cell-value nodes in the group title row,
    // beside an authored <h2> exactly as .safe-copy sits beside .safe-tag.
    // v1.2.0 adds the hover-help surface's three: tipEl is #tooltip itself — a node no label
    // shares, whose whole content is rebuilt per show — and t / b are createElement'd title and
    // body children that never existed in the authored HTML at all.
    // v1.4.0 adds ONE: delayUnitNode, the Delay column header's unit. The word "Delay" beside
    // it is AUTHORED AND NEVER WRITTEN — only the unit changes — and the ms/m toggle carries two
    // authored labels switched by class, so the whole feature costs exactly one receiver. The
    // binding is proved below, as earOut / srcOut / tipEl are.
    const VALUE_RECEIVERS = new Set(['value', 'el', 'earOut', 'srcOut', 'tipEl', 't', 'b',
                                     'delayUnitNode']);

    check(receivers.size > 0, `textContent is written through ${receivers.size} receiver(s)`);
    check([...receivers].every(r => VALUE_RECEIVERS.has(r)),
        `every textContent write goes through a dedicated value node — receivers {${[...receivers].sort().join(', ')}}`);

    // ...and the three v1.2.0 receivers really are the tooltip's own nodes: tipEl binds the one
    // shared surface, and t / b are created fresh inside showTip, never queried from the page.
    check(/const tipEl = document\.getElementById\("tooltip"\)/.test(S.appJs),
        'tipEl binds #tooltip — the one shared hover-help surface');

    // ...and the v1.4.0 receiver is a DEDICATED value node, not the header itself.
    //
    // READ OFF PAGE_MODULES, NOT OFF VENUE_CODE. That const is declared further down this file and
    // is in its TEMPORAL DEAD ZONE here — touching it throws ReferenceError at load and takes the
    // whole gate with it, which is this repo's own pattern_module_toplevel_init_tdz firing inside
    // the script that checks for it. PAGE_MODULES is already initialised: section 6 opened by
    // reading it.
    const venueSrc = (PAGE_MODULES.find(m => m.name.endsWith('venue.js')) || {}).code || '';

    check(/const delayUnitNode = need\("vcol-delay-unit"\)/.test(venueSrc),
        'delayUnitNode binds #vcol-delay-unit — a span, not the <th>');
    check(/<th class="vcol-head vcol-num">Delay&#8202;<span class="vcell-value" id="vcol-delay-unit">/.test(S.html),
        'and that span sits INSIDE an authored <th> whose "Delay" is never rewritten');
    // The toggle writes no text at all — if it ever starts to, this fires before the whitelist does.
    check(! /delayUnitButtons\[[a-z]+\]\.textContent/.test(venueSrc),
        'the ms/m toggle switches by class — it writes no textContent');
    check(/const t = document\.createElement\("div"\)/.test(S.appJs)
          && /const b = document\.createElement\("div"\)/.test(S.appJs),
        't / b are createElement\'d tooltip children, not authored nodes');

    // ...and the two 3.3 receivers really are bound to dedicated value nodes, not to a heading.
    check(/const earOut = document\.getElementById\("elev-ear"\)/.test(ELEVATION_SRC)
          && /const srcOut = document\.getElementById\("elev-src"\)/.test(ELEVATION_SRC),
        'earOut / srcOut bind #elev-ear and #elev-src — the strip\'s two dedicated readouts');

    // ...and those two locals are bound to value nodes only.
    check(/const value = document\.getElementById\(`val-\$\{id\}`\)/.test(S.appJs),
        '`value` is bound to the val-<id> readout node');
    // An explicit whitelist rather than a prefix rule, and it is reviewed when
    // it grows: the property being protected is "`el` never binds a node whose
    // text was AUTHORED". 3.2 adds two frame-level nodes app.js writes — the
    // mapInvalid copy and the negotiated set name — and both are value nodes
    // beside an authored tag, exactly as .safe-copy is beside .safe-tag.
    // 3.3 adds one: the field legend, which prints the dB span the plugin returned beside the plan
    // caption's authored "Field" key — the same tag/value pairing as every entry above it.
    const EL_TARGETS = /^(?:readout-|venue-name$|vset-name$|map-invalid-copy$|field-legend$)/;
    const elBindings = [...S.appJs.matchAll(/const el = document\.getElementById\("([^"]+)"\)/g)].map(m => m[1]);
    check(elBindings.length > 0 && elBindings.every(id => EL_TARGETS.test(id)),
        `\`el\` is bound only to dedicated value nodes — {${elBindings.join(', ')}}`);

    // The labels themselves must actually carry authored text.
    const LABEL_CLASSES = ['cell-label', 'w-label', 'group-title', 'screen-tab',
                           'readout-label', 'caption-key', 'glyph-num', 'safe-tag',
                           'map-tag', 'vcol-head', 'vcell-num'];
    for (const cls of LABEL_CLASSES) {
        const re = new RegExp(`class="[^"]*\\b${cls}\\b[^"]*"[^>]*>\\s*([^<\\s][^<]*)`, 'g');
        check([...S.html.matchAll(re)].length > 0, `.${cls} carries authored text in index.html`);
    }
}

// ──────────────────────────────────────────────────── 7. the Juce namespace ──
head(7, 'the Juce ES-module namespace, not window.__JUCE__');
{
    // Reaching for window.__JUCE__ and hand-rolling postMessage makes panels go
    // silently dead (critical_juce_webview_namespace_vs_postmessage).
    check(/import \* as Juce from "\.\/juce\/index\.js"/.test(S.appJs),
        'app.js imports the Juce namespace from js/juce/index.js');
    const reaching = PAGE_MODULES.filter(m => /__JUCE__/.test(m.code)).map(m => m.name);
    check(reaching.length === 0,
        `no shipped module reaches for window.__JUCE__, comments stripped — ${PAGE_MODULES.length} scanned`
        + (reaching.length ? ` — ${reaching.join(', ')}` : ''));
    check(/Juce\.getSliderState\(/.test(S.appJs) && /Juce\.getNativeFunction\(/.test(S.appJs),
        'both bridge entry points go through the namespace');
}

// ────────────────────────────────────────────────── 8. the resource provider ──
head(8, 'getResource matches BARE PATHS and hard-codes no scheme');
{
    // The callback receives a bare path; there is no scheme to strip and the
    // scheme differs per platform (critical_webview_resource_provider_and_schemes).
    check(!/juce:\/\//.test(CODE.editorCpp) && !/juce\.backend/.test(CODE.editorCpp),
        'PluginEditor.cpp hard-codes no juce:// or https://juce.backend (comments stripped)');

    const body = blockAt(S.editorCpp, S.editorCpp.indexOf('OctagonEditor::getResource'));
    const comparisons = [...body.matchAll(/url == "([^"]*)"/g)].map(m => m[1]);
    check(comparisons.length > 0, `getResource compares ${comparisons.length} paths by equality`);
    check(comparisons.every(u => u === '/' || u.startsWith('/')),
        'every comparison is against a leading-slash bare path');
    check(/charset=utf-8/.test(body), 'text resources are served with charset=utf-8');
    check(/goToURL \(juce::WebBrowserComponent::getResourceProviderRoot\(\)\)/.test(S.editorCpp),
        'the page is loaded from getResourceProviderRoot(), not a literal URL');
}

// ───────────────────────────────────────────────── 9. three-way resource closure ──
head(9, 'HTML/JS refs == getResource entries == juce_add_binary_data SOURCES');
{
    // A file embedded but not served, or served but not embedded, is a 404 that
    // shows up as a missing panel and nothing else.
    const sourcesBlock = S.cmake.match(/juce_add_binary_data\(OuariconOctagon_UIResources[\s\S]*?SOURCES([\s\S]*?)\n\)/);
    const embedded = new Set(
        sourcesBlock ? [...sourcesBlock[1].matchAll(/Source\/ui\/public(\/\S+)/g)].map(m => m[1]) : []);

    const body = blockAt(S.editorCpp, S.editorCpp.indexOf('OctagonEditor::getResource'));
    const served = new Set([...body.matchAll(/url == "([^"]*)"/g)].map(m => m[1]).filter(u => u !== '/'));

    // What the page actually asks for, resolved to served paths.
    const referenced = new Set(['/index.html']);
    for (const m of S.html.matchAll(/(?:href|src)="([^"]+)"/g))
        if (!/^(https?:|data:)/.test(m[1])) referenced.add('/' + m[1].replace(/^\.?\//, ''));
    for (const m of S.appJs.matchAll(/from "\.\/([^"]+)"/g)) referenced.add('/js/' + m[1]);
    for (const m of S.juceJs.matchAll(/^import "\.\/([^"]+)";/gm)) referenced.add('/js/juce/' + m[1]);

    // 7 -> 11 at Phase 3.3: scenes.js, meters.js, field.js, elevation.js. A LITERAL, and it moves
    // every phase for the same reason §3's does — a count that tracked whatever happened to be in
    // SOURCES would assert nothing at all.
    check(embedded.size === 11, `juce_add_binary_data embeds 11 files — ${embedded.size}`);
    check(setsEqual(served, embedded),
        `served == embedded${setsEqual(served, embedded) ? ''
            : ` — served-only: [${diff(served, embedded)}], embedded-only: [${diff(embedded, served)}]`}`);
    check(setsEqual(referenced, embedded),
        `referenced == embedded${setsEqual(referenced, embedded) ? ''
            : ` — referenced-only: [${diff(referenced, embedded)}], embedded-only: [${diff(embedded, referenced)}]`}`);

    // check_native_interop.js embedded AND served, or the page can hang
    // (juce8-critical-patterns section 13).
    check(served.has('/js/juce/check_native_interop.js'),
        'check_native_interop.js is embedded and served');

    // juce_add_binary_data STRIPS hyphens; the files are authored hyphen-free so
    // there is no transform to remember (critical_binary_data_strips_hyphens).
    check([...embedded].every(f => !path.basename(f).includes('-')),
        'no embedded filename contains a hyphen');
    check(/NAMESPACE\s+UIBinaryData/.test(S.cmake),
        'the binary-data target has a DISTINCT namespace (critical_dual_binary_data_namespace_collision)');
}

// ─────────────────────────────────────────────────────── 10. WebView2 flags ──
head(10, 'both Windows WebView2 flags are set');
{
    // Without static linking the Windows WebView renders BLANK in DAW hosts,
    // silently (critical_webview2_static_linking).
    check(/NEEDS_WEBVIEW2\s+TRUE/.test(S.cmake), 'NEEDS_WEBVIEW2 TRUE');
    check(/JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1/.test(S.cmake),
        'JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1');
    check(/NEEDS_WEB_BROWSER\s+TRUE/.test(S.cmake) && /JUCE_WEB_BROWSER=1/.test(S.cmake),
        'NEEDS_WEB_BROWSER TRUE and JUCE_WEB_BROWSER=1');
    // A default user-data folder is denied in most DAW hosts and falls back to
    // the IE backend with no resource provider — a blank page, no error
    // (critical_webview2_runtime_gotchas_windows).
    check(/withUserDataFolder/.test(S.editorCpp), 'WinWebView2 options set an explicit user-data folder');
}

// ───────────────────────────────────── 11. destruction order + harness target ──
head(11, 'member order relays -> webView -> attachments; PluginEditor.cpp absent from the harness');
{
    // C++ destroys in REVERSE. An attachment declared before the WebView is
    // destroyed after it and calls into a freed component — a release-build
    // crash on plugin reload (juce8-critical-patterns section 3).
    const iRelay  = S.editorH.indexOf('sliderRelays;');
    const iWeb    = S.editorH.indexOf('webView;');
    const iAttach = S.editorH.indexOf('sliderAttachments;');
    check(iRelay > 0 && iWeb > iRelay && iAttach > iWeb,
        `declaration order is relays(${iRelay}) -> webView(${iWeb}) -> attachments(${iAttach})`);

    check(/std::make_unique<juce::WebSliderParameterAttachment>\s*\(\s*\*param,\s*\*sliderRelays\[i\],\s*nullptr\)/.test(S.editorCpp),
        'the attachment takes THREE arguments, third nullptr');

    // 32 render-harness probes die silently if the editor enters that target
    // (pattern_render_harness_breaks_on_webview_editor).
    check(!/PluginEditor\.cpp/.test(S.harnessMake),
        'PluginEditor.cpp is NOT in tests/render-harness/CMakeLists.txt');
    check(/target_sources\(OuariconOctagon[\s\S]*?Source\/PluginEditor\.cpp/.test(S.cmake),
        'PluginEditor.cpp IS in target_sources(OuariconOctagon)');
    check(/#if JUCE_WEB_BROWSER\s*\n\s*return new OctagonEditor \(\*this\);\s*\n\s*#else\s*\n\s*return new juce::GenericAudioProcessorEditor/.test(S.procCpp),
        'createEditor\'s two arms diverge, with the generic editor demoted to #else');
    check(/#if JUCE_WEB_BROWSER\s*\n\s*#include "PluginEditor\.h"\s*\n\s*#endif/.test(S.procCpp),
        'PluginEditor.h is included ONLY from inside the guard');
}

// ────────────────────────────────────────────────────── 12. gesture pairing ──
head(12, 'every parameter write is bracketed, and the puck brackets BOTH axes (N1/P39)');
{
    // WebSliderParameterAttachment::sliderValueChanged routes a JS write through
    // setValueAsPartOfGesture — setValueNotifyingHost with NO
    // beginChangeGesture/endChangeGesture (juce_ParameterAttachments.cpp:324 ->
    // :76). Without the brackets Logic's Touch/Latch may MOVE THE SOUND AND NOT
    // RECORD IT, and nothing in build, auval or pluginval can see it.
    let totalOpens = 0;
    for (const { name, src } of PAGE_MODULES) {
        const starts = (src.match(/sliderDragStarted\(\)/g) || []).length;
        const ends   = (src.match(/sliderDragEnded\(\)/g) || []).length;
        totalOpens += starts;
        check(starts === ends,
            `${name}: ${starts} sliderDragStarted / ${ends} sliderDragEnded — balanced`);
    }
    // A module that writes no parameter at all — venue.js writes 42 VENUE values
    // and zero parameters — balances at 0/0, which is correct and must not fail.
    // The vacuity that opens is closed once, globally: if EVERY module balanced
    // at zero, the per-module check above would be asserting nothing.
    check(totalOpens > 0,
        `${totalOpens} gesture opens across ${PAGE_MODULES.length} shipped modules `
        + '— zero would make the per-module balance check vacuous');

    // The puck is the ONLY two-parameter gesture at 3.1: it must open on BOTH.
    const open  = blockAt(S.roomJs, S.roomJs.indexOf('function openPuckGesture'));
    const close = blockAt(S.roomJs, S.roomJs.indexOf('function closePuckGesture'));
    check(/srcX\.state\.sliderDragStarted\(\)/.test(open) && /srcY\.state\.sliderDragStarted\(\)/.test(open),
        'openPuckGesture opens on BOTH srcX and srcY');
    check(/srcX\.state\.sliderDragEnded\(\)/.test(close) && /srcY\.state\.sliderDragEnded\(\)/.test(close),
        'closePuckGesture closes BOTH srcX and srcY');

    // An interrupted drag that never closes leaves the host in an open
    // automation-write region.
    for (const ev of ['pointerup', 'pointercancel', 'lostpointercapture']) {
        check(new RegExp(`addEventListener\\("${ev}", closePuckGesture\\)`).test(S.roomJs),
            `the puck closes its bracket on ${ev}`);
        check(new RegExp(`addEventListener\\("${ev}", closeGesture\\)`).test(S.appJs),
            `the ordinary sliders close their bracket on ${ev}`);
    }

    // Every setNormalisedValue in roomplan.js must sit on the drag path, which
    // is gated by `dragging` — set only inside openPuckGesture.
    const moveBody = blockAt(S.roomJs, S.roomJs.indexOf('puck.addEventListener("pointermove"'));
    check(/if \(!dragging/.test(moveBody) && /setNormalisedValue/.test(moveBody),
        'the pointermove write is guarded by `dragging`, which only openPuckGesture sets');

    // Render on echo; NEVER write on echo. WebSliderParameterAttachment::setValue
    // sets ignoreCallbacks and sliderValueChanged carries a jassertfalse on that
    // path (juce_ParameterAttachments.cpp:326).
    const echo = [...S.roomJs.matchAll(/valueChangedEvent\.addListener\(([^)]*)\)/g)].map(m => m[1].trim());
    check(echo.length > 0 && echo.every(fn => fn === 'renderPuck' || fn === 'render'),
        `echo listeners only PAINT — {${echo.join(', ')}}`);
    const renderPuckBody = blockAt(S.roomJs, S.roomJs.indexOf('function renderPuck'));
    check(!/setNormalisedValue/.test(renderPuckBody), 'renderPuck() never writes a parameter');
}

// ──────────────────────────────────── 13. getVenueGeometry is wired to the venue ──
head(13, 'getVenueGeometry reads getVenue() and carries NO bbox literal (UI-02/5c)');
{
    // The only way parts (a) and (b) of UI-02/5 could BOTH be correct and the
    // readout still be wired to a constant.
    const i = S.editorCpp.indexOf('withNativeFunction ("getVenueGeometry"');
    check(i > 0, 'getVenueGeometry is registered');
    const body = blockAt(S.editorCpp, i);

    check(/processorRef\.getVenue\(\)/.test(body) && /processorRef\.getHull\(\)/.test(body),
        'the lambda reads processorRef.getVenue() and processorRef.getHull()');
    for (const acc of ['bbMinX()', 'bbMaxX()', 'bbMinY()', 'bbMaxY()'])
        check(body.includes(acc), `the envelope is built from venue.${acc}`);

    // No floating-point literal anywhere in the body — every bound is an
    // accessor, the margin rule is two named constants at namespace scope, and
    // the degenerate threshold is oo::plane::kMinSpan referenced by name.
    const code = body.replace(/\/\/[^\n]*/g, '').replace(/\/\*[\s\S]*?\*\//g, '');
    const floats = [...code.matchAll(/\b\d+\.\d+f?\b/g)].map(m => m[0]);
    check(floats.length === 0,
        `no float literal in the getVenueGeometry body${floats.length ? ` — found ${floats.join(', ')}` : ''}`);
    check(/oo::plane::kMinSpan/.test(body),
        'the degenerate test references oo::plane::kMinSpan, the single definition');
    check(/kEnvelopeMarginFraction/.test(body) && /kEnvelopeMarginMinM/.test(body),
        'the 15 % margin rule is two NAMED constants, not inline numbers');
}

// ────────────────────────────────── 14. no native round trip in a pointer path ──
head(14, 'no getNativeFunction awaited inside pointermove / mousemove / rAF (P42)');
{
    // getNativeFunction is an async round trip whose promises can resolve OUT OF
    // ORDER, so a readout written inside .then() can apply an older response
    // after a newer one and show a STALE metre while the puck is current.
    const hot = [];
    for (const { name, src } of PAGE_MODULES) {
        for (const marker of ['"pointermove"', '"mousemove"', 'requestAnimationFrame']) {
            let from = 0;
            for (;;) {
                const i = src.indexOf(marker, from);
                if (i < 0) break;
                const body = blockAt(src, i);
                if (/nativeFn\(|getNativeFunction\(|await /.test(body)) hot.push(`${name}:${marker}`);
                from = i + marker.length;
            }
        }
    }
    check(hot.length === 0,
        `no native call or await inside a hot pointer handler${hot.length ? ` — ${hot.join(', ')}` : ''}`);

    check(/cachedVenueGen/.test(S.appJs) && /gen !== cachedVenueGen/.test(S.appJs),
        'the geometry cache is invalidated by a venueGen comparison on the existing poll');
    check(/deps\.onSourceMoved/.test(S.roomJs),
        'the pointermove path recomputes metres from the CACHED geometry via a callback');
}

// ───────────────────────────────────────── 15. the stub mirrors the real ranges ──
head(15, 'the stub\'s 18 ranges + defaults match createParameterLayout() PARSED FROM SOURCE');
{
    // The precedent's own comments record its stub range table drifting FIVE
    // times. This is the fix for that class rather than another instance of it
    // (pattern_test_fixture_mirrors_drift_silently).
    const num = s => parseFloat(String(s).replace(/f$/, ''));
    const layout = new Map();

    const direct = /makeFloat\s*\(\s*"([A-Za-z0-9_]+)"\s*,\s*"[^"]*"\s*,\s*linearRange\s*\(\s*(-?[\d.]+f?)\s*,\s*(-?[\d.]+f?)\s*\)\s*,\s*(-?[\d.]+f?)/g;
    for (const m of S.procCpp.matchAll(direct))
        layout.set(m[1], { start: num(m[2]), end: num(m[3]), def: num(m[4]) });

    // The eight weights are built in a loop, so they have no literal id in the
    // source. Parsed from the loop body, and expanded here.
    const wLoop = S.procCpp.match(
        /makeFloat\s*\(\s*"w"\s*\+\s*juce::String\s*\(i\)[\s\S]*?linearRange\s*\(\s*(-?[\d.]+f?)\s*,\s*(-?[\d.]+f?)\s*\)\s*,\s*\n?\s*(-?[\d.]+f?)/);
    check(wLoop !== null, 'the w1..w8 loop in createParameterLayout() is parseable');
    if (wLoop)
        for (let i = 1; i <= 8; ++i)
            layout.set(`w${i}`, { start: num(wLoop[1]), end: num(wLoop[2]), def: num(wLoop[3]) });

    check(layout.size === 18, `createParameterLayout() parsed — ${layout.size} parameters (expect 18)`);

    const stubBlock = S.stubJs.match(/const RANGES = \{([\s\S]*?)\n\};/);
    check(stubBlock !== null, 'the stub RANGES table is parseable');

    const stub = new Map();
    if (stubBlock)
        for (const m of stubBlock[1].matchAll(
            /^\s{2}([A-Za-z0-9_]+):\s*\{\s*start:\s*(-?[\d.]+),\s*end:\s*(-?[\d.]+),\s*skew:\s*(-?[\d.]+),[^}]*def:\s*(-?[\d.]+)/gm))
            stub.set(m[1], { start: num(m[2]), end: num(m[3]), skew: num(m[4]), def: num(m[5]) });

    check(stub.size === 18, `the stub declares ${stub.size} ranges (expect 18)`);
    check(setsEqual(new Set(stub.keys()), new Set(layout.keys())),
        `stub ids == layout ids${setsEqual(new Set(stub.keys()), new Set(layout.keys())) ? ''
            : ` — stub-only [${diff(new Set(stub.keys()), new Set(layout.keys()))}], cpp-only [${diff(new Set(layout.keys()), new Set(stub.keys()))}]`}`);

    const drift = [];
    for (const [id, r] of layout) {
        const s = stub.get(id);
        if (!s) continue;
        if (Math.abs(s.start - r.start) > 1e-6 || Math.abs(s.end - r.end) > 1e-6
            || Math.abs(s.def - r.def) > 1e-6)
            drift.push(`${id}(cpp ${r.start}..${r.end} def ${r.def} / stub ${s.start}..${s.end} def ${s.def})`);
    }
    check(drift.length === 0, `every range AND default matches${drift.length ? ` — ${drift.join('; ')}` : ''}`);

    // All 18 ranges are linear by design — there is no skewForCentre helper to
    // get wrong, and this is what makes that claim executable.
    check([...stub.values()].every(s => s.skew === 1), 'all 18 stub skews are 1 (every range is linear)');

    // The two neutral-default traps, named so a failure says WHICH.
    for (let i = 1; i <= 8; ++i)
        check(layout.get(`w${i}`)?.def === 1, `w${i} defaults to 1.0, NOT its range minimum`);
    check(layout.get('blur')?.def === 0.03, 'blur defaults to 0.03, a non-endpoint');
    check(layout.get('airAmount')?.def === 0.35, 'airAmount defaults to 0.35, a non-endpoint');
}

// ─────────────────────────────────────────────────────── 16. four-way closure ──
head(16, 'createParameterLayout == params::id() == relay derivation == DOM ids');
{
    // A control wired in three of the four places is silently dead.
    // The trailing comma is load-bearing: the eight weights are built in a loop
    // as `makeFloat ("w" + juce::String (i), ...)`, so a regex that stops at the
    // closing quote harvests a nineteenth "parameter" called `w` and reports
    // 19 == 18 as a mismatch on correct code.
    const layoutIds = new Set([...S.procCpp.matchAll(/makeFloat\s*\(\s*"([A-Za-z0-9_]+)"\s*,/g)].map(m => m[1]));
    for (let i = 1; i <= 8; ++i) layoutIds.add(`w${i}`);

    const idTable = S.gainH.match(/static constexpr const char\* ids\[\][\s\S]*?\{([\s\S]*?)\};/);
    const paramsIds = new Set(idTable ? [...idTable[1].matchAll(/"([A-Za-z0-9_]+)"/g)].map(m => m[1]) : []);

    const domIds = new Set([...S.html.matchAll(/id="ctl-([A-Za-z0-9_]+)"/g)].map(m => m[1]));

    const appBlock = S.appJs.match(/const PARAM_IDS = \[([\s\S]*?)\];/);
    const appIds = new Set(appBlock ? [...appBlock[1].matchAll(/"([A-Za-z0-9_]+)"/g)].map(m => m[1]) : []);

    check(layoutIds.size === 18 && paramsIds.size === 18 && domIds.size === 18 && appIds.size === 18,
        `all four lists are 18 — layout ${layoutIds.size}, params::id ${paramsIds.size}, DOM ${domIds.size}, app ${appIds.size}`);
    check(setsEqual(layoutIds, paramsIds),
        `createParameterLayout == oo::params::id()${setsEqual(layoutIds, paramsIds) ? ''
            : ` — [${diff(layoutIds, paramsIds)}] / [${diff(paramsIds, layoutIds)}]`}`);
    check(setsEqual(paramsIds, domIds),
        `oo::params::id() == DOM ctl-<id>${setsEqual(paramsIds, domIds) ? ''
            : ` — [${diff(paramsIds, domIds)}] / [${diff(domIds, paramsIds)}]`}`);
    check(setsEqual(domIds, appIds),
        `DOM ctl-<id> == app.js PARAM_IDS${setsEqual(domIds, appIds) ? ''
            : ` — [${diff(domIds, appIds)}] / [${diff(appIds, domIds)}]`}`);

    // Every one also needs a readout node and a stub range, or it renders blank.
    const valIds = new Set([...S.html.matchAll(/id="val-([A-Za-z0-9_]+)"/g)].map(m => m[1]));
    check(setsEqual(valIds, domIds),
        `every control has a val-<id> readout${setsEqual(valIds, domIds) ? '' : ` — [${diff(domIds, valIds)}]`}`);

    // PluginEditor.cpp must DERIVE its relay list rather than transcribe one: a
    // second hand-written table is the mirrored fixture this project has watched
    // drift five times, and a static_assert two files away could not fire on it.
    check(/oo::params::id \(i\)/.test(S.editorCpp) || /oo::params::id \(static_cast<int> \(i\)\)/.test(S.editorCpp),
        'PluginEditor.cpp builds its relays from oo::params::id(i)');
    check(!/kSliderIds\s*(\{|=)/.test(S.editorCpp),
        'PluginEditor.cpp carries NO hand-written kSliderIds list');
    check(/for \(int i = 0; i < static_cast<int> \(oo::params::kCount\); \+\+i\)/.test(S.editorCpp),
        'the relay loop is bounded by oo::params::kCount, not by a literal 18');
}

// ──────────────────────────────────────────────────────── 17. the frame size ──
head(17, '1100 x 720 in setSize AND in html/body/.frame');
{
    // The precedent records this pair diverging across three separate resizes.
    check(/setSize \(1100, 720\)/.test(S.editorCpp), 'PluginEditor.cpp setSize(1100, 720)');
    check((S.editorCpp.match(/setSize\s*\(/g) || []).length === 1, 'exactly ONE setSize call');

    const htmlBody = S.css.match(/html,\s*body\s*\{([\s\S]*?)\}/);
    check(htmlBody !== null && /width:\s*1100px/.test(htmlBody[1]) && /height:\s*720px/.test(htmlBody[1]),
        'css html, body is 1100 x 720');

    const frame = S.css.match(/\.frame\s*\{([\s\S]*?)\}/);
    check(frame !== null && /width:\s*1100px/.test(frame[1]) && /height:\s*720px/.test(frame[1]),
        'css .frame is 1100 x 720');

    // Canvas is a CSS REPLACED element — left+right does NOT stretch it, and the
    // collapse is silent (o-textureforge-cursor-bug).
    const canvasRule = S.css.match(/#plan-backdrop\s*\{([\s\S]*?)\}/);
    check(canvasRule !== null && /width:\s*calc\(/.test(canvasRule[1]) && /height:\s*calc\(/.test(canvasRule[1]),
        '#plan-backdrop is sized with explicit width/height in calc()');
    check(canvasRule !== null && !/\bright:/.test(canvasRule[1]) && !/\bbottom:/.test(canvasRule[1]),
        '#plan-backdrop is NOT stretched with left+right / top+bottom');
    check(/canvas\.width = Math\.round\(rect\.width \* dpr\)/.test(S.roomJs)
          && /ctx\.scale\(dpr, dpr\)/.test(S.roomJs),
        'the DPR backing store is set and the context scaled');
}

// ───────────────────────────────────────────────────────── 18. tabular-nums ──
head(18, 'font-variant-numeric: tabular-nums on every value readout class');
{
    // Not taste. UI-01 column-aligns 50 venue values at 3.2, and a mis-scanned
    // metre is a measurement error that propagates silently into the solve.
    const VALUE_CLASSES = ['.cell-value', '.w-value', '.readout', '.glyph-num', '.caption-note',
                           '.vcell-value', '.vfield', '.map-copy'];
    for (const sel of VALUE_CLASSES) {
        // ANCHORED AT A RULE BOUNDARY. Phase 3.3 added `.elev-readouts .cell-value { ... }`, and
        // an unanchored scan harvested THAT rule's body instead of `.cell-value`'s — reporting the
        // mono stack and the tabular-nums treatment as missing from a stylesheet that has both.
        // A descendant selector ending in the class is not the class's own rule.
        const rule = S.css.match(new RegExp(`(?:^|\\n)\\s*\\${sel}\\s*\\{([\\s\\S]*?)\\}`));
        check(rule !== null && /font-variant-numeric:\s*tabular-nums/.test(rule[1]),
            `${sel} sets font-variant-numeric: tabular-nums`);
        check(rule !== null && /font-family:\s*var\(--mono\)/.test(rule[1]),
            `${sel} uses the mono stack`);
    }

    // Every element in the HTML that JS writes a value into must carry one of
    // those classes — otherwise a readout could be added without the treatment
    // and its metres would drift out of column on the next digit change.
    //
    // The WHOLE TAG is matched, not the run from the id attribute to '>': the
    // markup here writes class before id, so an id-anchored scan sees no class
    // at all and reports every readout untreated.
    //
    // 3.2 puts 42 measured metre values in a column, which is what the comment
    // at the top of styles.css always said this rule existed for. The 42 inputs
    // and the eight CLASS cells join the scan; so do the four rail/banner value
    // nodes. 20 -> 75.
    //
    // 75 -> 84 at v1.4.0: the eight vf-N-delay inputs, plus the Delay header's unit span. The
    // span is IN THE SCAN because JS writes into it — which is the rule as stated, and it is
    // exactly why it was authored as a dedicated .vcell-value node instead of the <th>: a
    // header written wholesale would carry .vcol-head and fail the treatment check below.
    //
    // 84 -> 85 at v1.5.0: val-decorr, the readout of the 18th parameter.
    const VALUE_IDS = /^(?:val-[A-Za-z0-9_]+|readout-[a-z]+|venue-name|vf-[A-Za-z0-9-]+|vclass-[0-9]+|vvenue-name|vset-name|vping-state|vpreset-current|map-invalid-copy|vcol-delay-unit)$/;
    const valueNodes = [...S.html.matchAll(/<[a-z]+\b[^>]*\bid="([^"]+)"[^>]*>/g)]
        .filter(m => VALUE_IDS.test(m[1]))
        .map(m => m[0]);
    check(valueNodes.length === 85, `85 value nodes found in index.html — ${valueNodes.length}`);

    const classOf = tag => (tag.match(/class="([^"]*)"/) || [, ''])[1].split(/\s+/);
    const untreated = valueNodes.filter(n =>
        !VALUE_CLASSES.some(c => classOf(n).includes(c.slice(1))));
    check(untreated.length === 0,
        `every value node carries a tabular class${untreated.length ? ` — ${untreated.join(' | ')}` : ''}`);
}

// ────────────────────────────────────────────────── 19. one projection function ──
head(19, 'ONE coordinate mapping — no second (v - min) / (max - min) form (P46)');
{
    // Three layers with three coordinate derivations would be three things free
    // to drift; one function used by all three cannot.
    const definers = PAGE_MODULES.filter(m => /export function metresToPx\(/.test(m.src));
    check(definers.length === 1,
        `metresToPx is defined exactly once across the shipped modules — [${definers.map(m => m.name).join(', ')}]`);

    const definer = definers[0];
    const proj = definer ? blockAt(definer.code, definer.code.indexOf('export function metresToPx(')) : '';

    // Every OTHER module may CALL the projection but must never re-derive one.
    for (const m of PAGE_MODULES.filter(m => m !== definer))
        check(!/function\s+metresToPx\s*\(/.test(m.code),
            `${m.name} does no projection of its own — it delegates to ${definer ? definer.name : 'roomplan.js'}`);

    // Any line that divides while touching an envelope/bbox bound is a
    // projection, and there must be exactly one such place — IN ANY MODULE. This
    // is the assertion NC1 drives: a second (v - min) / span form dropped into
    // venue.js was invisible to this section until the registry was derived.
    const offenders = PAGE_MODULES.flatMap(m => m.code.split('\n')
        .filter(l => /\bmin[XY]\b/.test(l) && /\//.test(l))
        .filter(l => !proj.includes(l.trim()))
        .map(l => `${m.name}: ${l.trim()}`));
    check(offenders.length === 0,
        `every (v - min) / span form lives inside metresToPx${offenders.length ? ` — ${offenders.join(' | ')}` : ''}`);

    // The degenerate decision is a payload FLAG, never a transcribed threshold —
    // a naive min + n*(max-min) diverges from the C++ on exactly the degenerate
    // venues Phase 2.1 spent a whole matrix on.
    check(/b\.degenerateX \?/.test(S.roomJs) && /b\.degenerateY \?/.test(S.roomJs),
        'normToMetres branches on the payload degenerate FLAGS');
    const transcribers = PAGE_MODULES.filter(m => /1\.0e-6|1e-6|kMinSpan/.test(m.code)).map(m => m.name);
    check(transcribers.length === 0,
        `no transcribed kMinSpan threshold anywhere in the shipped JS${transcribers.length ? ` — ${transcribers.join(', ')}` : ''}`);

    // The accumulator is clamped AT THE ACCUMULATOR, not only at the write.
    // Clamping only the written value IS the sticky-edge bug (N5 / P41).
    check(/acc\.x = clamp01\(acc\.x \+/.test(S.roomJs) && /acc\.y = clamp01\(acc\.y \+/.test(S.roomJs),
        'the relative-delta accumulator is clamped and STORED BACK');
}

// ────────────────────────────────────────────────────────── 20. MSVC habits ──
head(20, 'MSVC hazards are authored out now, not fixed at port time');
{
    // Written as habits rather than as port-time fixes: neither has a call site
    // at 3.1 (the FileChooser work is 3.2), but both are silent on Apple Clang
    // and hard errors on the first Windows CI build.
    const lambdas = [...S.editorCpp.matchAll(/\[[^\]]*\]\s*\([^)]*\)\s*(?:mutable\s*)?(?:->\s*[\w:<>*& ]+\s*)?\{/g)];
    let bad = 0;
    for (const m of lambdas) {
        const body = blockAt(S.editorCpp, m.index + m[0].length - 1);
        if (/(?<!static\s)\bconstexpr\b/.test(body)) ++bad;
    }
    check(bad === 0,
        `no non-static constexpr inside a lambda (MSVC C3493) — ${lambdas.length} lambda(s) scanned`);
    check(/^\s*constexpr float kEnvelopeMarginFraction/m.test(S.editorCpp),
        'the envelope constants live at NAMESPACE scope, which is why C3493 has no site here');

    // COMMENT-STRIPPED, and 3.2 is the phase that proved why: the comment explaining this ban
    // necessarily contains the banned token, so a raw scan fails on correct code and the obvious
    // "fix" is to delete the explanation. That is the rule stated at the top of this file, and
    // this section was the one place not following it.
    check(!/\[\s*safeThis\s*=\s*juce::Component::SafePointer/.test(CODE.editorCpp),
        'no SafePointer(this) init-capture in a nested lambda (MSVC resolves `this` to the closure)');

    // juce::String (const char*) converts through CharPointer_ASCII and mangles
    // any byte above 127, with no compiler warning
    // (critical_juce_string_char_ctor_is_ascii_only).
    const nonAscii = S.editorCpp.split('\n')
        .map((l, i) => [i + 1, l])
        .filter(([, l]) => !/^\s*(\/\/|\*|\/\*)/.test(l))
        .filter(([, l]) => /"[^"]*[^\x00-\x7F][^"]*"/.test(l));
    check(nonAscii.length === 0,
        `no non-ASCII inside a C++ string literal in PluginEditor.cpp${nonAscii.length ? ` — line ${nonAscii[0][0]}` : ''}`);
}

// ────────────────────────────────────────── 21. the derived module registry ──
head(21, 'the page-module registry is non-empty and EQUALS the embedded set (P51)');
{
    // §1/§3/§6/§7/§12/§14/§19 all iterate PAGE_MODULES rather than naming files.
    // That trades one failure mode for another: a directory read that returns
    // NOTHING makes all seven pass by iterating an empty list — the very vacuity
    // the derivation exists to close, one layer down. These three assertions are
    // what make the derivation trustworthy, and the third is the load-bearing
    // one: a module that exists on disk but was never embedded 404s at runtime,
    // and a name in SOURCES with no file behind it fails the build much later.
    check(PAGE_MODULES.length >= 3,
        `Source/ui/public/js/ holds ${PAGE_MODULES.length} authored module(s), expect >= 3 `
        + `— ${PAGE_MODULES.map(m => m.name).join(', ')}`);

    const sourcesBlock = S.cmake.match(/juce_add_binary_data\(OuariconOctagon_UIResources[\s\S]*?SOURCES([\s\S]*?)\n\)/);
    const embedded = new Set(
        sourcesBlock ? [...sourcesBlock[1].matchAll(/Source\/ui\/public(\/\S+)/g)].map(m => m[1]) : []);

    // js/juce/* is verbatim JUCE and is NOT a page module; §9 owns those two.
    const embeddedModules = new Set([...embedded].filter(p => /^\/js\/[^/]+\.js$/.test(p)));
    const derived = new Set(PAGE_MODULES.map(m => m.path));

    const notEmbedded = diff(derived, embeddedModules);
    check(notEmbedded.length === 0,
        `every derived module is in juce_add_binary_data SOURCES${notEmbedded.length ? ` — missing ${notEmbedded.join(', ')}` : ''}`);

    check(setsEqual(derived, embeddedModules),
        `derived set == the set §9 computes from CMake${setsEqual(derived, embeddedModules) ? ''
            : ` — on-disk-only: [${diff(derived, embeddedModules)}], SOURCES-only: [${diff(embeddedModules, derived)}]`}`);
}

// ══════════════════════════════════════════════════════════════════════════
// PHASE 3.2 — sections 22-31.
// ══════════════════════════════════════════════════════════════════════════

const VENUE_CODE = stripComments(S.venueJs);

// ────────────────────────────────────── 22. ONE setVenue, and 42 ids closed ──
head(22, 'ONE setVenue carrying all 50; no per-field write surface; the ids close FOUR ways (D8/P52)');
{
    // 42 async round trips whose promises may resolve out of order, against a model that recomputes
    // bbox, centroid, rigScale and the convex hull on every one, is P38's torn read arriving on the
    // WRITE side. There is exactly one call site and it carries everything.
    const calls = [...VENUE_CODE.matchAll(/nativeFn\(\s*"setVenue"\s*\)/g)];
    check(calls.length === 1, `exactly ONE setVenue call site in venue.js — ${calls.length}`);

    // ── (1) the DOM ──
    // 42 -> 50 at v1.4.0 (the eight vf-N-delay fields). This literal MOVES EVERY TIME the
    // venue's value count does, for the same reason the native-function count above does.
    const htmlIds = new Set([...S.html.matchAll(/id="(vf-[A-Za-z0-9-]+)"/g)].map(m => m[1]));
    check(htmlIds.size === 50, `index.html carries 50 vf-* field ids — ${htmlIds.size}`);

    // ── (2) venue.js's table, DERIVED from its own two constants rather than transcribed ──
    const countM = VENUE_CODE.match(/const SPEAKER_COUNT = (\d+);/);
    const keysM  = VENUE_CODE.match(/const NUMERIC_KEYS = \[([^\]]*)\]/);
    check(countM !== null && keysM !== null, 'venue.js SPEAKER_COUNT and NUMERIC_KEYS are parseable');

    const jsIds = new Set();
    if (countM && keysM) {
        const keys = [...keysM[1].matchAll(/"([a-z]+)"/g)].map(m => m[1]);
        for (let n = 1; n <= Number(countM[1]); ++n) {
            jsIds.add(`vf-label-${n}`);
            for (const k of keys) jsIds.add(`vf-${n}-${k}`);
        }
        for (const m of VENUE_CODE.matchAll(/"(vf-rake-[a-z]+)"/g)) jsIds.add(m[1]);
    }
    check(setsEqual(htmlIds, jsIds),
        `DOM ids == venue.js's derived table${setsEqual(htmlIds, jsIds) ? ''
            : ` — dom-only [${diff(htmlIds, jsIds)}], js-only [${diff(jsIds, htmlIds)}]`}`);

    // ── (3) the payload venue.js builds ──
    const payload = blockAt(VENUE_CODE, VENUE_CODE.indexOf('function buildPayload'));
    const rowKeys = new Set([...payload.matchAll(/^\s+([a-zA-Z]+):/gm)].map(m => m[1]));
    check(setsEqual(rowKeys, new Set(['n', 'label', 'x', 'y', 'z', 'trimDb', 'delayMs'])),
        `each speaker row carries exactly label + x + y + z + trimDb + delayMs — {${[...rowKeys].sort().join(', ')}}`);
    check(/rake:\s*\{\s*front,\s*rear\s*\}/.test(payload),
        'and the payload carries the two rake heights — 8 x 6 + 2 = 50 in ONE call');

    // ── (3b) v1.4.0 — THE WIRE UNIT IS ALWAYS MILLISECONDS ──
    //
    // The column can DISPLAY metres, so the one thing that must never happen is a payload
    // carrying whatever the toggle happened to be showing. buildPayload must run the pending
    // edit — and ONLY the pending edit — through displayToMs; the committed fallback is already
    // ms and converting it a second time would scale every untouched delay by 343 on any commit
    // made while the column was in metres.
    check(/delayMs:\s*clampDelayMs\(/.test(payload),
        'the delay on the wire is railed by clampDelayMs before it leaves the page');
    check(/pendingNumber\(r\.n, "delay"\) === undefined\s*\?\s*base\.delayMs/.test(payload),
        'and an UNTOUCHED delay falls back to committed ms WITHOUT a second conversion');
    check(/:\s*displayToMs\(pendingNumber\(r\.n, "delay"\)\)/.test(payload),
        'while a PENDING delay converts from the displayed unit exactly once');

    // ── (4) the VenueModel setters the C++ side reaches for ──
    const setVenueBody = blockAt(S.editorCpp, S.editorCpp.indexOf('withNativeFunction ("setVenue"'));
    for (const setter of ['setSpeakerPosition', 'setSpeakerTrimDb', 'setSpeakerDelayMs',
                          'setSpeakerLabel', 'setRake'])
        check(setVenueBody.includes(setter), `setVenue reaches VenueModel::${setter}() — 50 covered`);

    // THE ABSENCE THAT KEEPS applyVenueEdit PUBLIC FROM BEING A HOLE (P52). Probe BL calls it
    // directly and must keep being able to; the editor must reach the venue ONLY through the
    // checked entry point, because an invalid map is AUDIBLE (N8).
    check(! /applyVenueEdit\s*\(/.test(CODE.editorCpp),
        'PluginEditor.cpp contains NO applyVenueEdit ( call — the editor goes through the guard');
    check(/applyVenueEditChecked\s*\(/.test(CODE.editorCpp),
        'PluginEditor.cpp DOES call applyVenueEditChecked (so the check above is not vacuous)');
}

// ───────────────────────────────────────────── 23. type=text, explicit parse ──
head(23, 'every venue field is type="text" + inputmode="decimal", and the parse is EXPLICIT (D12)');
{
    // On invalid content a number input reports .value === "" and .valueAsNumber === NaN, so
    // "typed abc" and "cleared the field" are INDISTINGUISHABLE and FUNC-02 criterion 1 becomes
    // untestable. This is the assertion that keeps that door shut.
    const venueSection = S.html.slice(S.html.indexOf('id="screen-venue"'));
    const inputs = [...venueSection.matchAll(/<input\b[^>]*>/g)].map(m => m[0]);

    // 42 -> 50 at v1.4.0 (the eight vf-N-delay fields). The delay column is type="text" for
    // exactly the reason the other four are: on invalid content a number input cannot tell
    // "typed abc" from "cleared the field".
    check(inputs.length === 50, `50 inputs on the Venue screen — ${inputs.length}`);
    check(inputs.every(t => /type="text"/.test(t)), 'every one is type="text"');
    check(inputs.every(t => /inputmode="decimal"/.test(t)), 'every one carries inputmode="decimal"');
    check(! /type="number"/.test(S.html), 'type="number" appears NOWHERE in index.html');

    // parseFloat("7.25abc") returns 7.25, so parseFloat ALONE accepts trailing junk. The guard is
    // the point — a coordinate that silently became 7.25 propagates into the DBAP solve.
    check(/Number\.parseFloat\(/.test(VENUE_CODE), 'venue.js parses with Number.parseFloat');
    check(/Number\.isFinite\(/.test(VENUE_CODE), 'and rejects non-finite results');
    const parse = blockAt(VENUE_CODE, VENUE_CODE.indexOf('function parseNumber'));
    check(/test\(s\)/.test(parse) && /return null/.test(parse),
        'and guards the WHOLE string with a pattern, so trailing junk is rejected rather than truncated');
}

// ──────────────────────────────── 24. the label column holds; the rest revert ──
head(24, 'label HOLDS and MARKS where every numeric column REVERTS (N8/P53)');
{
    // A reachability argument, not a taste one: every route from (L, R) to (R, L) passes through a
    // duplicate, so a label that reverted on collision would make the swap UNREACHABLE.
    const numeric = blockAt(VENUE_CODE, VENUE_CODE.indexOf('function bindNumeric'));
    const label   = blockAt(VENUE_CODE, VENUE_CODE.indexOf('function bindLabel'));

    check(/input\.value = revertText\(\)/.test(numeric),
        'bindNumeric REVERTS the field to the last committed value');
    check(/is-invalid/.test(numeric), 'and marks it .is-invalid');

    check(! /revertText/.test(label), 'bindLabel does NOT revert — the pending edit is HELD');
    check(/is-colliding/.test(label) || /applyLabelMarks/.test(label),
        'and both colliding rows are marked instead');

    // While the set is not a permutation the page does not call setVenue AT ALL.
    const commit = blockAt(VENUE_CODE, VENUE_CODE.indexOf('function commit'));
    check(/marks\.size > 0\)\s*return/.test(commit.replace(/\s+/g, ' ').replace(/ /g, ' ')) ||
          /if \(marks\.size > 0\) return/.test(commit),
        'commit() returns BEFORE the setVenue call when any row is marked');
    check(commit.indexOf('collidingRows') < commit.indexOf('nativeFn("setVenue")'),
        'the collision test runs BEFORE the write, not after it');
}

// ─────────────────────────────── 25. no UI state waits on a promise (N4/P64) ──
head(25, 'authoritative UI state converges on the venueGen poll, never on a completion (N4/P64)');
{
    // emitCompletionEvent calls emitEventIfBrowserIsVisible, which DROPS the event when the
    // component is hidden: no error, no rejection, no log, and the await never returns — inside a
    // page withKeepPageLoadedWhenBrowserIsHidden() has kept alive to have awaited it.
    const paintCalls = [...VENUE_CODE.matchAll(/\bpaintFields\(\)/g)];
    check(paintCalls.length >= 1, `paintFields() is called ${paintCalls.length} time(s)`);

    const setGeom = blockAt(VENUE_CODE, VENUE_CODE.indexOf('setGeometry(g)'));
    check(/paintFields\(\)/.test(setGeom),
        'the table repaints from setGeometry() — i.e. from the venueGen refresh path');

    // No .then() handler may be the thing that ESTABLISHES committed state.
    const thenBodies = [...VENUE_CODE.matchAll(/\.then\(/g)]
        .map(m => blockAt(VENUE_CODE, m.index));

    const establishing = thenBodies.filter(b => /committed\s*=/.test(b));
    check(establishing.length === 0,
        `no .then() handler ASSIGNS committed — ${thenBodies.length} handler(s) scanned`);

    // ── WHY THIS RULE WAS SPLIT IN TWO (v1.3.2, CODE_REVIEW WR-05) ───────────
    //
    // Until v1.3.2 the single check above also rejected any `paintFields()`
    // inside a .then(), and the two are not the same thing. `committed = ...`
    // ESTABLISHES state: if the completion is dropped the page never learns the
    // truth, which is the whole content of N4/P64. `paintFields()` RE-RENDERS
    // state the venueGen poll has already established: if that completion is
    // dropped the page is left exactly where it already was, so it can make the
    // UI no staler than not calling it at all. Only the first is a convergence
    // hazard.
    //
    // WR-05 needs the second: on a C++ REJECTION nothing publishes, venueGen
    // never moves, and the poll-driven repaint the rest of this section is about
    // never runs — so the rejected text sat in the input while the model held
    // the old value, unbounded in SAFE mode where no commit can ever succeed.
    // The repaint has to happen on the refusal, because the refusal is the only
    // event there is.
    //
    // The rule is NARROWED, not dropped: a repaint in a completion is allowed
    // ONLY in the branch that handles a refusal. A .then() that repainted
    // unconditionally would still be a convergence claim resting on a
    // completion, and still fails here.
    const repainting = thenBodies.filter(b => /paintFields\(\)/.test(b));
    const guardedByRefusal = repainting.filter(b => /ok\s*===\s*false/.test(b));

    check(repainting.length === guardedByRefusal.length,
        `every .then() that repaints is inside the ok:false REFUSAL branch — `
        + `${guardedByRefusal.length}/${repainting.length}`);

    // And app.js drives it from the poll it already had.
    check(/venueScreen\.setGeometry\(geometry\)/.test(S.appJs),
        'app.js feeds the Venue screen from refreshGeometry(), which the venueGen poll triggers');

    // The ping poll is started BEFORE its start promise is awaited, so a dropped completion leaves
    // a sounding ping with a LIVE indicator rather than a dead one.
    const request = blockAt(VENUE_CODE, VENUE_CODE.indexOf('function requestPing'));
    check(request.indexOf('startPingPoll()') < request.indexOf('nativeFn("startPing")'),
        'startPingPoll() runs BEFORE startPing is called, not inside its .then()');
}

// ────────────────────────────────── 26. no JS timer computes a speaker index ──
head(26, 'the ping indicator READS getPingState().speaker; no JS timer derives it (D14/P61)');
{
    check(/nativeFn\(\s*"getPingState"\s*\)/.test(VENUE_CODE), 'venue.js polls getPingState');

    const render = blockAt(VENUE_CODE, VENUE_CODE.indexOf('function renderPing'));
    check(/state\.speaker/.test(render), 'renderPing reads state.speaker from the RETURNED payload');

    // A drifted setInterval would name speaker 5 while 6 sounds, during the one procedure whose
    // entire purpose is confirming that speaker N is speaker N.
    //
    // SCOPED TO venue.js, WHICH IS THE MODULE THAT OWNS THE PING, and deliberately not to the whole
    // page. app.js converts mapInvalidSpeaker from the 0-based row ochan::MapDiagnosis reports to
    // the 1-based one the banner shows, which is a display conversion of a DIAGNOSIS and not a
    // derivation of a ping step — banning it would be banning the wrong thing and would push the
    // conversion somewhere less visible.
    const offenders = VENUE_CODE.split('\n')
        .filter(l => /\bspeaker\b/i.test(l))
        .filter(l => /(\+\+|--|\+=|-=|\s%\s|\bspeaker\s*[+\-*/]\s*\w)/i.test(l))
        .map(l => l.trim());
    check(offenders.length === 0,
        `venue.js does no arithmetic on a speaker index${offenders.length ? ` — ${offenders.join(' | ')}` : ''}`);

    // The poll runs at 100 ms and ONLY while pinging: STATUS_POLL_MS is 500, which is LONGER than
    // the 400 ms auto-cycle gap and can miss a gap entirely.
    const pollM = VENUE_CODE.match(/const PING_POLL_MS = (\d+);/);
    const statusM = S.appJs.match(/const STATUS_POLL_MS = (\d+);/);
    check(pollM !== null && statusM !== null && Number(pollM[1]) < Number(statusM[1]),
        `the ping poll (${pollM ? pollM[1] : '?'} ms) is FASTER than the status poll `
        + `(${statusM ? statusM[1] : '?'} ms) — the gap it must resolve is 400 ms`);
    check(/pingTimer === null\) return/.test(stripComments(S.venueJs).replace(/\s+/g, ' ')) ||
          /if \(pingTimer !== null\) return/.test(VENUE_CODE),
        'the interval is started at most once and cleared when the ping stops');

    // 3.2 stays pull-only: emitEvent IS emitEventIfBrowserIsVisible, so a dropped push never
    // retries where a poll self-heals (P61), and the ui-stub does not model addEventListener.
    const pushers = PAGE_MODULES.filter(m => /addEventListener\(\s*["'][a-zA-Z]+["']\s*,/.test(m.code)
                                             && /backend\./.test(m.code)).map(m => m.name);
    check(pushers.length === 0, 'no module reaches for a backend push transport');
}

// ────────────────────────────────────────── 27. FUNC-05's one greppable claim ──
head(27, 'EXACTLY ONE setCustomStateCallbacks, touching only SCENES (FUNC-05, reshaped at 3.3)');
{
    // applyPresetJson iterates processor.getParameters() and resolves via getParameter(id) — it
    // never walks apvts.state's children, so it can never reach VENUE. FUNC-05 criterion 1 holds
    // BY CONSTRUCTION.
    //
    // ── THIS ASSERTION CHANGED SHAPE AT 3.3, AND THE CHANGE WAS PLANNED (D17 / P80) ────────────
    // The ONE path by which a preset can reach non-parameter state is a registered custom-state
    // callback. Through 3.2 there was none, and "the symbol appears in zero of N files" was the
    // strongest available form. At 3.3 EXACTLY ONE becomes legitimate, for SCENES — so the gate
    // now asserts the count AND the reach, because "none" would fail on correct code and deleting
    // the registration to satisfy it would delete the feature.
    //
    // A weaker gate here would be the whole risk: this is the only door between a musical preset
    // and the 42 measured venue values, and it must stay exactly one door wide. Probe CL measures
    // the same claim at runtime, with the callback live.
    //
    // COMMENT-STRIPPED, per the rule at the top of this file: the comments that explain the
    // registration necessarily name the symbol.
    const sources = fs.readdirSync(path.join(pluginRoot, 'Source'), { recursive: true })
        .filter(f => typeof f === 'string' && /\.(cpp|h)$/.test(f))
        .map(f => path.join(pluginRoot, 'Source', f));

    const carriers = sources.filter(f => /setCustomStateCallbacks\s*\(/.test(stripComments(fs.readFileSync(f, 'utf8'))));

    check(carriers.length === 1 && path.basename(carriers[0]) === 'PluginEditor.cpp',
        `setCustomStateCallbacks is registered EXACTLY ONCE across ${sources.length} O-Octagon source `
        + `files, in PluginEditor.cpp — found ${carriers.length}: [${carriers.map(f => path.basename(f)).join(', ')}]`);

    // AND ITS BODY REACHES SCENES AND NOTHING ELSE. The two lambdas delegate to scenesToVar /
    // scenesFromVar; neither may name the venue, and neither may touch apvts.state directly.
    if (carriers.length === 1) {
        const src = stripComments(fs.readFileSync(carriers[0], 'utf8'));
        const at = src.indexOf('setCustomStateCallbacks');
        const body = src.slice(at, at + 400);

        check(/scenesToVar\s*\(\s*\)/.test(body) && /scenesFromVar\s*\(/.test(body),
            'the registration delegates to scenesToVar / scenesFromVar and to nothing else');
        check(! /[Vv]enue/.test(body),
            `the registration body does not name the venue${/[Vv]enue/.test(body) ? ' — IT DOES' : ''}`);

        // The two functions it delegates to must be equally narrow: a callback that reached VENUE
        // one level down would satisfy every assertion above.
        const procSrc = stripComments(S.procCpp);
        const toVar   = blockAt(procSrc, procSrc.indexOf('OOctagonProcessor::scenesToVar'));
        const fromVar = blockAt(procSrc, procSrc.indexOf('OOctagonProcessor::scenesFromVar'));

        check(toVar.length > 0 && fromVar.length > 0, 'both halves are defined in PluginProcessor.cpp');
        check(! /\bvenue\b/.test(toVar) && ! /\bvenue\b/.test(fromVar),
            'and NEITHER touches the venue model — SCENES is the whole reach of the preset path');
        check(/sceneStore/.test(toVar) && /sceneStore/.test(fromVar),
            'both go through sceneStore, which is the only thing they can serialise');
    }

    // The module's JS side wires TEN native functions in its constructor and createPresetBar()
    // writes container.innerHTML — pattern_js_state_updater_overwrites_html_labels by construction.
    const vendored = PAGE_MODULES.filter(m => /preset/i.test(m.name)).map(m => m.name);
    check(vendored.length === 0, `preset-manager.js is NOT vendored${vendored.length ? ` — ${vendored}` : ''}`);

    const htmlCode = S.html.replace(/<!--[\s\S]*?-->/g, '');
    check(! /createPresetBar/.test(htmlCode) && ! PAGE_MODULES.some(m => /createPresetBar/.test(m.code)),
        'createPresetBar() is never called (comments stripped)');

    // Four functions, not the module's ten.
    const presetFns = ['savePreset', 'loadPreset', 'getPresetList', 'getCurrentPreset'];
    for (const fn of presetFns)
        check(new RegExp(`withNativeFunction \\("${fn}"`).test(S.editorCpp),
            `${fn} is registered`);

    // The option elements are BUILT, never assigned as HTML.
    check(/document\.createElement\("option"\)/.test(VENUE_CODE),
        'the preset list is built with createElement');
    check(! /innerHTML/.test(VENUE_CODE), 'venue.js never assigns innerHTML');
}

// ──────────────────────────────────── 28. the preset load brackets all 18 (N5) ──
head(28, 'the loadPreset call site brackets ALL 18 parameters (N5/P59)');
{
    // applyPresetJson calls setValueNotifyingHost DIRECTLY on the parameter, not through a
    // ParameterAttachment — so F3's unchanged-write skip does NOT apply (callIfParameterValueChanged
    // is a member of the ATTACHMENT), and setValueNotifyingHost carries no brackets of its own. Up
    // to 34 bare host writes per load, which in Logic with a lane in Latch or Touch is a recorded
    // sweep the operator never performed.
    const body = blockAt(S.editorCpp, S.editorCpp.indexOf('withNativeFunction ("loadPreset"'));

    check(body.length > 0, 'the loadPreset registration is present');
    check(/beginChangeGesture\(\)/.test(body), 'it opens a gesture');
    check(/endChangeGesture\(\)/.test(body), 'and closes one');

    // Bounded by oo::params::kCount, never by a literal 18 — the same rule §16 enforces on the
    // relay loop, so adding a parameter cannot leave one unbracketed.
    check(/oo::params::kCount/.test(body),
        'the bracket loops are bounded by oo::params::kCount, not by a literal 18');
    check(/oo::params::id \(i\)/.test(body),
        'and the ids come from oo::params::id(i), not a second transcribed list');

    // The close must be reachable on BOTH paths — an interrupted load that never closed would
    // leave the host in an open automation-write region on all seventeen.
    check(body.indexOf('endChangeGesture') > body.indexOf('presetManager.loadPreset'),
        'endChangeGesture runs AFTER the load, unconditionally');

    // And never by editing the shared module: four other plugins depend on it.
    const moduleH = path.join(pluginRoot, '..', '..', 'modules', 'persistence', 'preset-manager',
                              'cpp', 'OuariconPresetManager.h');
    if (fs.existsSync(moduleH)) {
        const mod = fs.readFileSync(moduleH, 'utf8');
        check(! /beginChangeGesture/.test(mod),
            'the SHARED module was not edited — the fix lives at O-Octagon\'s call site');
    }
}

// ───────────────────────────────── 29. the chooser completions (UI-01/3b) ──
head(29, 'chooser completions: hoisted SafePointer, BARE return, and no parallel serialiser');
{
    // (b) of UI-01 criterion 3, and it is what makes (a) non-vacuous: two correct halves wired to
    // DIFFERENT code is the failure this class of gate exists for.
    for (const fn of ['saveVenue', 'loadVenue']) {
        const body = blockAt(S.editorCpp, S.editorCpp.indexOf(`withNativeFunction ("${fn}"`));

        check(/juce::Component::SafePointer<OctagonEditor> safeThis \{ this \};/.test(body),
            `${fn}: the SafePointer is a HOISTED LOCAL (MSVC resolves an init-capture's \`this\` to the closure)`);
        check(/\[safeThis, complete\]/.test(body), `${fn}: it is captured BY COPY into the completion`);
        check(/if \(safeThis == nullptr\)\s*\n\s*return;/.test(body),
            `${fn}: a dead pointer returns BARE — never complete(false), which is itself a UAF`);
        check(/launchAsync/.test(body), `${fn}: goes through FileChooser::launchAsync`);
    }

    check(/oo::venuefile::save/.test(CODE.editorCpp) && /oo::venuefile::load/.test(CODE.editorCpp),
        'the completions call exactly oo::venuefile::save / ::load');

    // NO PARALLEL VENUE SERIALISATION PATH. The one way (a) could be correct and the button still
    // be wired to something else.
    //
    // PluginProcessor.cpp's getStateInformation DOES call state.createXml(), and that is the
    // SESSION path — N2 requires it stay exactly as it is, because
    // OuariconPresetManager::setStateFromXml calls replaceState() and nothing else, which would
    // bypass §4.1's readVenueFromState() -> rebuildChannelMap() ordering. So the assertion is
    // scoped to the file that owns the chooser, plus a check that the processor's two sites are
    // still only the session pair.
    const serialisers = /createXml|parseXML|XmlDocument/;

    check(! serialisers.test(CODE.editorCpp),
        'PluginEditor.cpp serialises NOTHING itself — the chooser delegates to oo::venuefile');

    const procCode = stripComments(S.procCpp);
    const procSites = (procCode.match(/createXml|parseXML|XmlDocument/g) || []).length;
    check(procSites === 1,
        `PluginProcessor.cpp has exactly ONE serialisation site — ${procSites} (getStateInformation, N2)`);
    check(/getStateInformation/.test(procCode.slice(0, procCode.search(serialisers))),
        'and it is inside the session path, not a second venue path');

    check(serialisers.test(S.venueFileCpp),
        'VenueFile.cpp IS the one that serialises a .venue (so the checks above are not vacuous)');

    // The file rules Q6 bought: fresh model, forward version surfaced, malformed root rejected.
    check(/LoadResult::forwardVersion/.test(S.venueFileCpp) && /LoadResult::malformedRoot/.test(S.venueFileCpp),
        'a forward @schemaVersion is SURFACED and a malformed root REJECTED');
    check(/oo::VenueModel loaded;/.test(CODE.editorCpp),
        'loadVenue loads into a FRESH default-constructed model, never the live venue');
    check(/applyVenueEditChecked/.test(blockAt(S.editorCpp, S.editorCpp.indexOf('withNativeFunction ("loadVenue"'))),
        'and applies it through the SAME guard a typed label goes through');
}

// ────────────────────────── 30. the ping is a POST-WRITE overwrite, no reset ──
head(30, 'the ping overwrites AFTER the write and AFTER the NaN guard; no new reset() (§7.2/P23/P30)');
{
    // v1.4.0 MOVED THIS ANCHOR. The per-sample write used to BE the matrix expression; the
    // matrix result is now named `y` first, so the delay can read it before it is stored. The
    // invariant is unchanged and so is what this measures — solve, then guard, then ping.
    const iWrite = S.gainCpp.indexOf('const float y = (gL[');
    const iGuard = S.gainCpp.indexOf('if (! std::isfinite (lastL)) airL.reset();');
    const iPing  = S.gainCpp.indexOf('ping->overwrite (out, kNumSpeakers, start, count)');

    check(iWrite > 0 && iGuard > iWrite, 'the NaN guard sits after the per-sample write');
    check(iPing > iGuard,
        `the ping override sits AFTER both — write ${iWrite}, guard ${iGuard}, ping ${iPing}`);

    // Through the SAME out[] pointers, which are snapshot.speakerToBuffer. Writing to raw channel i
    // would be correct on an identity map and wrong on every other one.
    check(/out\[i\] = buffer\.getWritePointer \(snapshot\.speakerToBuffer/.test(S.gainCpp),
        'out[] is resolved through snapshot.speakerToBuffer — so the ping tests the MAP');

    // ── P23/P30's ONE RESET SITE EVER, still true ──
    //
    // 20 sites, and every one is accounted for. THE POINT OF THE COUNT IS THAT IT IS A LEDGER,
    // NOT A BUDGET: a reset that is not on this list is an unlabelled second initialisation path,
    // and P23 says the first 240 samples of every render depend on there being exactly one.
    //
    //   prepare(), step 2 — the smoother/filter teleports:  6
    //     the 17 gain smoothers (3 loops), v1.4.0's eight delay ramps (1),
    //     and v1.5.0's decorrMix + decorrDepth (2).
    //   prepare(), step 4 — post-teleport re-derivation:    3
    //     v1.5.0's two chains (2) and v1.4.0's delay lines (1). ARMING, not initialising:
    //     the teleport moved the ramps under a gate computed one line earlier.
    //   updateControl() — edge-triggered ARMING/RECOVERY:   5
    //     DSP-07/7's airAmount edge (2), v1.5.0's decorr engage edge (2), and
    //     v1.4.0's alignment-line engage edge (1).
    //   renderChunk() — P27's air re-seed:                  2
    //   renderChunk() — DSP-07/8's NaN RECOVERY:            4
    //     the two air filters (2) and v1.5.0's two chains (2).
    //
    // 3.2 added NONE. v1.4.0 added three. v1.5.0 adds eight, all of them ARMING or RECOVERY
    // sites rather than initialisation sites, each labelled as such at the call.
    const resets = (S.gainCpp.match(/\.reset\s*\(/g) || []).length;
    check(resets === 20, `GainStage.cpp has exactly 20 reset() sites — ${resets}`);

    // ── v1.4.0 — THE DELAY SITS BEFORE THE PING, WHICH IS THE WHOLE POINT ──
    // The ping bypasses DBAP, the weights, the hull trim, the air filter, the per-speaker trim and
    // outputGain so that a ping from the wrong speaker has exactly ONE possible cause. A delayed
    // ping would add a second. Measured as an ORDERING, because "the delay is applied first" is
    // the same statement as "the ping overwrites what the delay produced".
    const iDelay = S.gainCpp.indexOf('alignDelay[k].pushSample (0, y)');
    check(iDelay > iWrite && iDelay < iGuard,
        `the delay reads the matrix result and writes before the NaN guard — write ${iWrite}, delay ${iDelay}, guard ${iGuard}`);
    check(iPing > iDelay, 'and the ping overwrites AFTER it, so the ping is never delayed');

    check(! /\.reset\s*\(/.test(S.pingCpp),
        'VerifyPing.cpp calls reset() NOWHERE — the 20 ms raised cosine owns both discontinuities');
    check(/prepare \(spec\)/.test(S.pingCpp) || /hp\.prepare/.test(S.pingCpp),
        'its filters are prepare()d, which is mandatory: the default s1 is the vector { 2 }');

    // Sample-counted clocks, so probes BS and BT can measure them offline at all.
    for (const k of ['kOnSeconds', 'kGapSeconds', 'kLatchSeconds', 'kAutoCycleSeconds'])
        check(new RegExp(`${k}`).test(S.pingH), `VerifyPing declares ${k}`);
    check(/sampleRate/.test(S.pingCpp) && /toSamples/.test(S.pingCpp),
        'and converts every one to SAMPLES from the prepared rate');
    // COMMENT-STRIPPED — both of these bans are explained in comments that name them.
    const pingCode = stripComments(S.pingH) + '\n' + stripComments(S.pingCpp);

    check(! /juce::Timer/.test(pingCode),
        'no juce::Timer anywhere — a message-thread timer is unmeasurable in a render harness');

    // Member-owned Random, never getSystemRandom() (§F9).
    check(/juce::Random rng;/.test(S.pingH), 'the Random is MEMBER-OWNED');
    check(! /getSystemRandom/.test(pingCode),
        'getSystemRandom() is never reached for (pattern_rng_stream_interleave_blocksize)');

    // D11's stops.
    check(/processBlockBypassed/.test(S.procH), 'processBlockBypassed is OVERRIDDEN (D11)');
    check(/processorRef\.stopVerifyPing\(\)/.test(S.editorCpp),
        'the editor destructor stops the ping');
    check(/\.abort\(\)/.test(S.procCpp), 'and the audio thread aborts one on a mapped -> not-mapped flip');
}

// ──────────────────────────────────────────── 31. MSVC + ASCII, extended ──
head(31, 'MSVC and ASCII habits extended to every file 3.2 added (constraints 6, 10)');
{
    // THE FILES 3.2 ADDED, plus PluginEditor.cpp because 3.2 added ~300 lines to it.
    //
    // ChannelMap.cpp is deliberately NOT here. It carries an em-dash inside a failure message, but
    // that message is built with `+` on an already-constructed juce::String, which does not go
    // through the CharPointer_ASCII conversion — adjudicated at 3.1 when D-2 was fixed
    // (VenueModel.cpp's String(const char*) form was the one that mangled). Scanning it would
    // report a defect that is not one, and a gate that cries wolf gets edited rather than obeyed.
    const NEW_CPP = [['VenueFile.h', S.venueFileH], ['VenueFile.cpp', S.venueFileCpp],
                     ['VerifyPing.h', S.pingH], ['VerifyPing.cpp', S.pingCpp],
                     ['PluginEditor.cpp', S.editorCpp]];

    // juce::String (const char*) converts through CharPointer_ASCII and mangles any byte above 127
    // with NO compiler warning. 3.2 writes far more user-facing C++ text than 3.1 did — three
    // MapFailure reasons, the forward-version warning, the ping refusal — and D-2 was a real
    // defect, silent since Stage 2 (critical_juce_string_char_ctor_is_ascii_only).
    for (const [name, src] of NEW_CPP) {
        const bad = src.split('\n')
            .map((l, i) => [i + 1, l])
            .filter(([, l]) => !/^\s*(\/\/|\*|\/\*)/.test(l))
            .filter(([, l]) => /"[^"]*[^\x00-\x7F][^"]*"/.test(l));
        check(bad.length === 0,
            `${name}: no non-ASCII inside a C++ string literal${bad.length ? ` — line ${bad[0][0]}` : ''}`);
    }

    // MSVC C3493: a non-static constexpr declared inside a lambda.
    for (const [name, src] of NEW_CPP) {
        const lambdas = [...src.matchAll(/\[[^\]]*\]\s*\([^)]*\)\s*(?:mutable\s*)?(?:->\s*[\w:<>*& ]+\s*)?\{/g)];
        let bad = 0;
        for (const m of lambdas) {
            const body = blockAt(src, m.index + m[0].length - 1);
            if (/(?<!static\s)\bconstexpr\b/.test(body)) ++bad;
        }
        check(bad === 0, `${name}: no non-static constexpr inside a lambda — ${lambdas.length} scanned`);
    }

    // MSVC resolves `this` in a nested-lambda INIT-CAPTURE to the closure. 3.2 is the phase that
    // finally has call sites for this, and they hoist to a local instead.
    for (const [name, src] of NEW_CPP)
        check(! /\[\s*safeThis\s*=\s*juce::Component::SafePointer/.test(stripComments(src)),
            `${name}: no SafePointer(this) init-capture in a nested lambda (comments stripped)`);
}

// ══════════════════════════════════════════════════════════════════════════
// PHASE 3.3 — sections 32-42. Scenes, meters, the field, the elevation strip.
// ══════════════════════════════════════════════════════════════════════════

const MOD = Object.fromEntries(PAGE_MODULES.map(m => [m.name.replace(/^js\//, ''), m]));

const SCENES_JS    = MOD['scenes.js'];
const METERS_JS    = MOD['meters.js'];
const FIELD_JS     = MOD['field.js'];
const ELEVATION_JS = MOD['elevation.js'];

// ─────────────────────────────────── 32. the page does no speaker arithmetic ──
head(32, 'scene membership is RETURNED WHOLE — the page compares no speaker coordinate (D19/P79)');
{
    // A JS re-derivation of D16's predicate would be a mirrored fixture
    // (pattern_test_fixture_mirrors_drift_silently) over R1, the highest-risk component in the
    // project — and it is exactly what makes FUNC-06/2's PERMUTATION probe meaningful: a
    // fixed-index implementation must FAIL that probe, and it cannot fail a test the page never
    // runs. NC2 drops a centroid comparison into the scene module and fires this.
    check(SCENES_JS !== undefined, 'js/scenes.js is a derived page module');

    if (SCENES_JS !== undefined) {
        // The predicate's own vocabulary. Not one of these may appear in the module that resolves
        // a scene: centroid, bounding box, half-span, hull classification.
        const banned = ['centroid', 'bbox', 'hullPts', 'INTERIOR', 'ON_EDGE', 'VERTEX',
                        'classify', 'Math.abs', 'halfSpan'];
        const found = banned.filter(t => SCENES_JS.code.includes(t));

        check(found.length === 0,
            `js/scenes.js contains none of the predicate's vocabulary${found.length ? ` — FOUND: ${found.join(', ')}` : ''}`);

        // And it reads the payload the C++ built, WHOLE.
        check(/geometry\?\.scenes|geometry\.scenes/.test(SCENES_JS.code),
            'it consumes geometry.scenes — the set the plugin resolved');
        check(/\.indices\b/.test(SCENES_JS.code),
            'and renders the returned indices rather than a locally derived set');
    }

    // Nothing ELSE on the page may derive one either — the roomplan preview renders indices it is
    // handed, and app.js only forwards them.
    const derivers = PAGE_MODULES.filter(m =>
        /\bclassify\s*\(/.test(m.code) || /!==?\s*['"]INTERIOR['"]/.test(m.code)).map(m => m.name);
    check(derivers.length === 0,
        `no shipped module classifies a speaker${derivers.length ? ` — ${derivers.join(', ')}` : ''}`);

    // The C++ side must be the one that does, or the payload is empty and §32 passes vacuously.
    check(/oo::scenes::resolve\s*\(/.test(CODE.editorCpp),
        'PluginEditor.cpp resolves membership through oo::scenes::resolve — so there IS a set to return');
}

// ───────────────────────────── 33. every in-flight guard releases on a DEADLINE ──
head(33, 'no guard releases only on settlement — every one carries a deadline (P71/N9)');
{
    // ── THIS IS A REPAIR, NOT A NEW RULE ──────────────────────────────────────────────────────
    // A native completion is DROPPED, not rejected, when the browser is hidden (N4). A dropped
    // completion is neither an exception nor a rejection, so NEITHER `catch` NOR `finally` RUNS,
    // and a flag cleared only in a `finally` stays true for the LIFE OF THE PAGE.
    //
    // RESEARCH-3.3 N9 MEASURED THAT IN SHIPPED 3.2 CODE: one dropped getVenueGeometry left the
    // envelope readout frozen at 15.60 x 19.50 m against a real 39.00 x 52.00 m, unrecovered
    // across five poll ticks with the transport restored. D20's "fixed interval + in-flight guard"
    // is NECESSARY BUT NOT SUFFICIENT as written.
    const guarded = PAGE_MODULES.filter(m => /\binFlight\b|InFlight/.test(m.code));

    check(guarded.length >= 3,
        `${guarded.length} module(s) carry an in-flight guard — ${guarded.map(m => m.name).join(', ')}`);

    for (const m of guarded) {
        check(/performance\.now\(\)/.test(m.code),
            `${m.name}: the guard is timestamped — it reads performance.now()`);
        // LINE-BASED, because the deadline is legitimately an EXPRESSION in one of the three
        // modules — meters.js writes it as `METER_POLL_MS * GUARD_DEADLINE_TICKS` so the interval
        // and the number of missed ticks cannot drift apart. What the gate requires is the SHAPE:
        // elapsed-since compared against something named as a deadline, on one line.
        const releases = m.code.split('\n').some(line =>
            /-\s*\w*(?:Since|At)\b/.test(line) && /[<>]/.test(line) && /DEADLINE|GUARD/i.test(line));

        check(releases,
            `${m.name}: and it is RELEASED on that deadline, not only on settlement`);
    }

    // The three specific sites, named, so a deleted guard is a named failure rather than a
    // silently smaller `guarded` list.
    check(/GEOMETRY_GUARD_DEADLINE_MS/.test(S.appJs),
        'app.js refreshGeometry carries the deadline that repairs N9 — the LIVE 3.2 defect');
    check(METERS_JS !== undefined && /GUARD_DEADLINE_TICKS/.test(METERS_JS.code),
        'js/meters.js\'s 30 Hz poll carries one, expressed in INTERVALS so the two cannot drift');
    check(FIELD_JS !== undefined && /GUARD_DEADLINE_MS/.test(FIELD_JS.code),
        'js/field.js carries one');

    // ── AND pollStatus MUST NOT GAIN ONE (P71 rule 3) ─────────────────────────────────────────
    // It has NO guard, which is precisely why it is the one poll already safe: setInterval fires
    // the next tick regardless, so a dropped tick leaks one pending promise and the poll
    // SELF-HEALS. That leak is bounded and acceptable; the latch is not. "Tidying" it would
    // convert the safe path into the broken one.
    const pollBody = blockAt(S.appJs, S.appJs.indexOf('async function pollStatus'));
    check(pollBody.length > 0 && ! /InFlight/.test(pollBody),
        'pollStatus still has NO in-flight guard — the one poll that self-heals must stay that way');
}

// ───────────────────────────────────── 34. the meter poll is a fixed interval ──
head(34, 'the 30 Hz poll is a fixed setInterval, never poll().then(poll) (P71/N4)');
{
    // A recursion whose next tick is scheduled by the previous COMPLETION dies permanently the
    // first time one is dropped — the completion never arrives, so nothing ever schedules again
    // (pattern_webview_completion_gated_on_isvisible). A fixed interval fires regardless.
    check(METERS_JS !== undefined, 'js/meters.js is a derived page module');

    if (METERS_JS !== undefined) {
        check(/window\.setInterval\s*\(/.test(METERS_JS.code),
            'the poll is a window.setInterval');
        check(! /setTimeout/.test(METERS_JS.code),
            'and there is no setTimeout anywhere in it — no self-rescheduling path exists');

        // The completion handler must not schedule the next poll.
        const tickBody = blockAt(METERS_JS.code, METERS_JS.code.indexOf('function tick()'));
        check(tickBody.length > 0 && ! /setInterval|setTimeout|requestAnimationFrame/.test(tickBody),
            'tick() schedules nothing — the interval owns the clock');

        // The BALLISTICS are per rAF frame and the poll only moves the target. Applying a
        // per-frame coefficient on the poll clock is
        // pattern_block_rate_envelope_breaks_blocksize_invariance in UI form.
        check(/ATTACK_PER_FRAME/.test(METERS_JS.code) && /DECAY_PER_FRAME/.test(METERS_JS.code),
            'the 0.5 / 0.12 coefficients are named PER_FRAME');
        check(! /ATTACK_PER_FRAME|DECAY_PER_FRAME/.test(tickBody),
            'and tick() — the POLL — applies neither of them; it moves the target only');
        check(/requestAnimationFrame/.test(METERS_JS.code),
            'the ballistics run on requestAnimationFrame');
        check(/PEAK_HOLD_MS/.test(METERS_JS.code) && /PEAK_RELEASE_DB_PER_S/.test(METERS_JS.code),
            'the 1.5 s hold and 20 dB/s release are WALL CLOCK constants, not frame counts');
    }
}

// ────────────────────────── 35. setStateFromXml is never reached (N13's trap) ──
head(35, 'no setStateFromXml / getStateAsXml call site anywhere in O-Octagon (N13)');
{
    // ONE TRAP IN THE SAME HEADER as the callback §27 now permits. setStateFromXml
    // (OuariconPresetManager.h:585-604) calls customLoad on a DIFFERENT condition from
    // applyPresetJson and, above it, does `parameters.replaceState(...)` — WHICH WOULD REPLACE THE
    // WHOLE TREE, VENUE INCLUDED. O-Octagon does not use that path and MUST NOT START.
    //
    // A one-line gate, because the failure it prevents is the silent loss of 42 measured values on
    // a preset load.
    const sources = fs.readdirSync(path.join(pluginRoot, 'Source'), { recursive: true })
        .filter(f => typeof f === 'string' && /\.(cpp|h)$/.test(f))
        .map(f => path.join(pluginRoot, 'Source', f));

    const callers = sources.filter(f =>
        /\.(setStateFromXml|getStateAsXml)\s*\(/.test(stripComments(fs.readFileSync(f, 'utf8'))));

    check(callers.length === 0,
        `neither is called in any of ${sources.length} O-Octagon source files`
        + (callers.length ? ` — ${callers.map(f => path.basename(f)).join(', ')}` : ''));

    // NON-VACUITY: the symbols must exist in the module, or this is a search for nothing.
    const modulePath = path.join(pluginRoot, '..', '..', 'modules', 'persistence',
                                 'preset-manager', 'cpp', 'OuariconPresetManager.h');
    if (fs.existsSync(modulePath)) {
        const mod = fs.readFileSync(modulePath, 'utf8');
        check(/setStateFromXml/.test(mod) && /replaceState/.test(mod),
            'and the module really does expose setStateFromXml with a replaceState inside it');
    }
}

// ───────────────────────────────── 36. scene labels are HTML-authored ──
head(36, 'scene labels are AUTHORED and RENDERED; state goes to data-* + aria-pressed');
{
    // A shared JS state updater writing textContent onto a label node erases it silently and
    // passes every build gate (pattern_js_state_updater_overwrites_html_labels). ROADMAP names
    // this a Phase 3.3 test criterion in its own right, which is why it is a section and not a
    // clause.
    const htmlCode = S.html.replace(/<!--[\s\S]*?-->/g, '');

    for (const label of ['ALL', 'FRONT', 'REAR', 'LEFT', 'RIGHT', 'SIDES', 'STORE'])
        check(new RegExp(`>${label}<`).test(htmlCode), `"${label}" is authored in index.html`);

    if (SCENES_JS !== undefined) {
        check(! /textContent/.test(SCENES_JS.code),
            'js/scenes.js never writes textContent — not on a label, not anywhere');
        check(/aria-pressed/.test(SCENES_JS.code) && /dataset\./.test(SCENES_JS.code),
            'state travels through aria-pressed and data-* attributes');
        check(! /innerHTML/.test(SCENES_JS.code), 'and it never assigns innerHTML');
    }

    // The elevation readouts DO get written — they are values, in dedicated .cell-value nodes, and
    // that is the whole distinction this rule draws.
    if (ELEVATION_JS !== undefined)
        check(/elev-ear/.test(ELEVATION_JS.code) && /elev-src/.test(ELEVATION_JS.code),
            'the elevation strip writes its two VALUES into dedicated nodes, never onto a label');
}

// ─────────────────────────────────────────── 37. two polls, at two rates ──
head(37, 'getStatus stays at 2 Hz and getMeters runs at ~30 Hz — two polls, measured reason (Q4)');
{
    // getStatus builds a juce::String from getBus(false,0)->getCurrentLayout().getDescription() on
    // EVERY call. Folding the meters into it would run that at 30 Hz — thirty string constructions
    // a second on the message thread — for a value that changes only on renegotiation.
    check(/STATUS_POLL_MS\s*=\s*500\b/.test(S.appJs), 'app.js polls getStatus every 500 ms');
    check(METERS_JS !== undefined && /METER_POLL_MS\s*=\s*3[0-9]\b/.test(METERS_JS.code),
        'js/meters.js polls getMeters at ~30 Hz');

    // Each function is called from EXACTLY ONE module, so neither rate can be reached by the other
    // payload through a second call site.
    const callers = (name) => PAGE_MODULES.filter(m =>
        new RegExp(`nativeFn\\(\\s*["']${name}["']`).test(m.code)).map(m => m.name);

    check(callers('getStatus').join(',') === 'js/app.js',
        `getStatus is called from app.js only — [${callers('getStatus').join(', ')}]`);
    check(callers('getMeters').join(',') === 'js/meters.js',
        `getMeters is called from meters.js only — [${callers('getMeters').join(', ')}]`);

    // And the C++ side really does pay the String cost on getStatus, or the split has no reason.
    const statusBody = blockAt(CODE.editorCpp, CODE.editorCpp.indexOf('withNativeFunction ("getStatus"'));
    check(/getCurrentLayout\s*\(\)\s*\.getDescription/.test(statusBody),
        'getStatus really does construct the layout description string on every call');

    const metersBody = blockAt(CODE.editorCpp, CODE.editorCpp.indexOf('withNativeFunction ("getMeters"'));
    check(metersBody.length > 0 && ! /getDescription|juce::String/.test(metersBody),
        'and getMeters constructs no juce::String at all — eight floats and nothing else');
    check(/readAndZeroMeters\s*\(\)/.test(metersBody),
        'the read ZEROES C++-side, so a dropped frame widens the window instead of losing the peak');
}

// ─────────────────────────── 38. offscreen canvas, blitted (UI-04 criterion 3) ──
head(38, 'the field is decoded to an OFFSCREEN canvas and blitted, structurally (UI-04/3)');
{
    check(FIELD_JS !== undefined, 'js/field.js is a derived page module');

    if (FIELD_JS !== undefined) {
        check(/document\.createElement\("canvas"\)/.test(FIELD_JS.code),
            'an OFFSCREEN canvas is created, at the grid\'s own resolution');
        check(/\batob\s*\(/.test(FIELD_JS.code), 'the payload is base64, decoded with atob');
        check(/createImageData|putImageData/.test(FIELD_JS.code), 'written with putImageData');
        check(/drawImage\s*\(/.test(FIELD_JS.code), 'and BLITTED with drawImage');

        // The chain must be in that order, or "offscreen and blitted" is three unrelated calls.
        const iAtob = FIELD_JS.code.indexOf('atob(');
        const iPut  = FIELD_JS.code.indexOf('putImageData');
        const iDraw = FIELD_JS.code.indexOf('drawImage');
        check(iAtob >= 0 && iPut > iAtob && iDraw > iPut,
            `the chain is atob -> putImageData -> drawImage, in that order (${iAtob} < ${iPut} < ${iDraw})`);

        // NO PER-PIXEL WORK AGAINST THE PLAN BOX. The browser's own smoothing is what turns a
        // 32 x 40 grid into a gradient; a per-pixel loop at 448 x 560 would be the CPU spike
        // UI-04/3 forbids.
        check(/imageSmoothingEnabled/.test(FIELD_JS.code),
            'the upscale is the browser\'s bilinear smoothing, not a JS loop');

        // NO JS RE-DERIVATION OF THE SOLVE (UI-04/1). The page decodes bytes; it never computes a
        // distance, a rolloff or a gain.
        const solveish = ['Math.pow', 'Math.sqrt', 'Math.hypot', 'rolloff', 'denom'];
        const found = solveish.filter(t => FIELD_JS.code.includes(t));
        check(found.length === 0,
            `js/field.js performs no part of the solve${found.length ? ` — FOUND: ${found.join(', ')}` : ''}`);

        // The legend prints the RETURNED span. Without it the picture is decoration: the field over
        // a raked audience plane is genuinely flat, so a normalised ramp with no number looks
        // informative and is not.
        check(/minDb/.test(FIELD_JS.code) && /maxDb/.test(FIELD_JS.code),
            'and the legend is built from the returned minDb / maxDb');
    }

    // The QUANTITY, in C++: 1/k = sqrt(denom), never max_i v_i^2. N10 measured the latter as
    // identically 1.0000 everywhere with one active weight. NC6 restores it and fires CB.
    check(/float\* outInvK/.test(S.gainH) || /outInvK/.test(fs.readFileSync(
              path.join(pluginRoot, 'Source', 'DSP', 'DbapSolver.h'), 'utf8')),
        'dbap::solve exposes outInvK — the UN-NORMALISED field the solver already computes');

    const samplerCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'DSP', 'FieldSampler.cpp'), 'utf8');
    const samplerCode = stripComments(samplerCpp);
    check(/hullproc::hullTrimGain/.test(samplerCode) && /hull::isInside/.test(samplerCode)
          && /dbap::solve/.test(samplerCode),
        'the sampler follows the FULL chain — hull test, projection, solve, trim — through the '
        + 'shipping functions');
    check(! /std::pow/.test(samplerCode),
        'and it introduces no pow of its own, so probe AE\'s powCalls == 16 is untouched');
}

// ──────────────────────────────────── 39. UI-04 descopes by a flag (UI-04/4) ──
head(39, 'the field is a separate draw layer, imported by nothing but app.js (UI-04/4)');
{
    // "Descopable without touching any other component" is a STRUCTURAL claim, and this is its
    // executable form: exactly one module imports the field, the backdrop it paints on is a canvas
    // that has existed since 3.1 as its own layer, and the plan module's painter hook defaults to
    // null.
    const importers = PAGE_MODULES.filter(m => /from\s+["']\.\/field\.js["']/.test(m.code)).map(m => m.name);
    check(importers.join(',') === 'js/app.js',
        `js/field.js is imported by app.js and nothing else — [${importers.join(', ')}]`);

    const htmlCode = S.html.replace(/<!--[\s\S]*?-->/g, '');
    check(/<canvas id="plan-backdrop">/.test(htmlCode),
        '#plan-backdrop is its own element — a separate layer, authored at 3.1 for exactly this');

    check(/let fieldPainter = null/.test(S.roomJs),
        'roomplan.js\'s painter hook defaults to null — not installing it IS the descope');
    check(/if \(fieldPainter !== null\)/.test(S.roomJs),
        'and the draw path is guarded, so a null painter leaves the 3.1 backdrop exactly as it was');
}

// ──────────────────────────────────── 40. UI-05 descopes by a flag (UI-05/4) ──
head(40, 'the elevation strip is the column\'s last child, imported by nothing but app.js (UI-05/4)');
{
    const importers = PAGE_MODULES.filter(m => /from\s+["']\.\/elevation\.js["']/.test(m.code)).map(m => m.name);
    check(importers.join(',') === 'js/app.js',
        `js/elevation.js is imported by app.js and nothing else — [${importers.join(', ')}]`);

    // LAST CHILD IN THE SOURCE, which is the static half of what layout §22 measures on the
    // rendered tree. Both exist because a future phase can break either one alone.
    const htmlCode = S.html.replace(/<!--[\s\S]*?-->/g, '');
    const column = htmlCode.slice(htmlCode.indexOf('<div class="controls-column">'));
    const groups = [...column.matchAll(/<section class="group" id="([a-z-]+)"/g)].map(m => m[1]);

    check(groups[groups.length - 1] === 'group-elevation',
        `#group-elevation is the LAST group authored in the controls column — [${groups.join(', ')}]`);

    // Nothing else may depend on the strip's presence.
    check(! /elev-/.test(S.roomJs) && (SCENES_JS === undefined || ! /elev-/.test(SCENES_JS.code)),
        'no other module reaches for an elevation element');
}

// ────────────────────────── 41. the rake line spans the BBOX and nothing more ──
head(41, 'the rake line is drawn ONLY bbMinY -> bbMaxY (P76 rule 1 / UI-05/1)');
{
    // earHeight EXTRAPOLATES LINEARLY outside bbMinY..bbMaxY (VenueModel.h:173-177). A single line
    // across the whole ENVELOPE therefore has BOTH ends move when rakeRear moves — and UI-05
    // criterion 1's negative half is precisely that the FRONT endpoint does NOT, because
    // earHeight(bbMinY) == rakeFront for any rakeRear (RESEARCH-2.2 H5). NC7 draws the
    // whole-envelope line and watches that half fail.
    check(ELEVATION_JS !== undefined, 'js/elevation.js is a derived page module');

    if (ELEVATION_JS !== undefined) {
        const drawRake = blockAt(ELEVATION_JS.code, ELEVATION_JS.code.indexOf('function drawRake()'));

        check(drawRake.length > 0, 'drawRake() is present');

        // The SOLID line's endpoints are the bbox, at the two rake values.
        check(/at\s*\(\s*b\.minY\s*,\s*r\.front\s*\)/.test(drawRake),
            'the front endpoint is (bbMinY, rakeFront) — the value earHeight returns there for ANY rakeRear');
        check(/at\s*\(\s*b\.maxY\s*,\s*r\.rear\s*\)/.test(drawRake),
            'the rear endpoint is (bbMaxY, rakeRear)');

        // The extrapolation is a DIFFERENT element, so no assertion can read one for the other.
        check(/rakeExt\.setAttribute/.test(drawRake) && /rake\.setAttribute/.test(drawRake),
            'the extrapolated continuation is a SEPARATE element (#elev-rake-ext), drawn dashed');

        const htmlCode = S.html.replace(/<!--[\s\S]*?-->/g, '');
        check(/id="elev-rake"/.test(htmlCode) && /id="elev-rake-ext"/.test(htmlCode),
            'and both elements exist in the HTML, so the two are never one line');

        // The solid line must not be built from the ENVELOPE.
        // FROM the solid line's endpoints TO the extrapolation's — `const e = geometry.envelope`
        // is declared above both, so slicing from the top would always see the envelope named.
        const solidHalf = drawRake.slice(drawRake.indexOf('const front ='),
                                         drawRake.indexOf('const extA'));
        check(! /\be\.minY\b/.test(solidHalf) && ! /\be\.maxY\b/.test(solidHalf),
            'the solid line never reads the envelope — only the extrapolation does');
    }
}

// ───────────────────────── 42. the height axis is venue-derived and QUANTISED ──
head(42, 'the height axis follows the venue and is quantised to a 1 m step (P76 rule 2)');
{
    // An axis that auto-fits rakeRear RESCALES when rakeRear changes, and UI-05/1 would then be
    // measuring a rescale instead of a move — passing its positive half for entirely the wrong
    // reason. Quantising means an ordinary rake edit never moves the axis at all, and the
    // criterion's own "front endpoint unchanged" half becomes the guard.
    if (ELEVATION_JS !== undefined) {
        const derive = blockAt(ELEVATION_JS.code, ELEVATION_JS.code.indexOf('function deriveAxisMax'));

        check(derive.length > 0, 'deriveAxisMax() is present');
        check(/g\.speakers/.test(derive) && /g\.rake/.test(derive),
            'the axis is derived from the VENUE — the tallest speaker and both rake ends');
        check(/Math\.ceil/.test(derive) && /AXIS_STEP_M/.test(derive),
            'and QUANTISED UP to a whole AXIS_STEP_M');
        check(/AXIS_STEP_M\s*=\s*1\b/.test(ELEVATION_JS.code),
            'the step is 1 m');
        check(/AXIS_MIN_M/.test(derive),
            'with a floor, so a flat rig cannot collapse the axis to zero');

        // NO EXAGGERATION FACTOR. Two scales is ordinary for a section drawing; what it requires
        // is a LABELLED axis, which drawAxis provides.
        check(! /EXAGGERAT|exaggerat/i.test(ELEVATION_JS.code),
            'no exaggeration factor is applied — the two scales are labelled, not faked');
        check(/elev-axis-label/.test(ELEVATION_JS.code),
            'and the height axis is labelled in metres');

        // ONE PROJECTION, BOTH AXES (§19's rule, honoured rather than worked around).
        check(/import \{[^}]*metresToPx[^}]*\} from "\.\/roomplan\.js"/.test(ELEVATION_JS.code),
            'the strip projects through the SAME metresToPx the two plans use');
        check(/import \{[^}]*earHeight[^}]*\} from "\.\/roomplan\.js"/.test(ELEVATION_JS.code),
            'and earHeight is imported, not re-implemented');

        // Rule 3: the MARKER clamps, the NUMBERS never do.
        const marker = blockAt(ELEVATION_JS.code, ELEVATION_JS.code.indexOf('function drawMarker()'));
        check(/chevron/.test(marker), 'a clamped marker shows a chevron');
        check(/earM\.toFixed|srcM\.toFixed/.test(marker),
            'and BOTH readouts print the unclamped metres');
        check(! /shownM\.toFixed/.test(marker),
            'the CLAMPED value is never what the readout prints');
    }
}

// ────────────────────── 43. the delay column holds no constant of its own (v1.4.0) ──
head(43, 'the ms/metres conversion divides by what C++ SENT — the page owns no speed of sound (D19)');
{
    const VENUE = PAGE_MODULES.find(m => m.name.endsWith('venue.js'));
    check(VENUE !== undefined, 'js/venue.js is a derived page module');

    if (VENUE !== undefined) {
        // ── THE LITERAL THAT MUST NOT BE THERE ────────────────────────────────────────────────
        //
        // A `343` written into this module would be a mirrored fixture over
        // oo::plane::kSpeedOfSoundMps — the same constant VenueModel::suggestedDelaysMs() divides
        // by. They agree today. They stop agreeing the first time either moves, and the symptom
        // is a Delay column reading metres the Derive button disagrees with, in a tool whose
        // whole claim is that the numbers on screen are the room's.
        //
        // This is v1.3.5's own `blur`-fallback bug stated as a rule: a default in two places went
        // stale for two minor versions with nothing on screen to show for it.
        //
        // The DELAY_FALLBACK_* pair is exempt BY NAME and only there: it covers the frames before
        // the first geometry payload lands, when the table has nothing to display anyway.
        const body = VENUE.code.split('\n')
            .filter(l => ! /DELAY_FALLBACK_/.test(l))
            .join('\n');

        check(! /\b343(\.\d+)?\b/.test(body),
            'venue.js contains no 343 outside the named pre-payload fallbacks');

        // And the rail is not transcribed either — it is OOctagonProcessor::kVenueDelayClampMs.
        check(! /\bmaxDelayMs\s*=\s*50\b/.test(body),
            'nor a hard-coded 50 ms rail');

        // What it DOES do: read both off the payload, every refresh.
        check(/g\.speedOfSound/.test(VENUE.code) && /g\.maxDelayMs/.test(VENUE.code),
            'it reads speedOfSound and maxDelayMs off the geometry payload');

        // ── AND THE TWO CONVERSIONS ARE ONE PAIR, NOT SCATTERED ───────────────────────────────
        // Every ms<->metres crossing goes through msToDisplay / displayToMs. A third inline
        // `* speedOfSound` anywhere is how one of the crossings ends up with the direction wrong
        // — and a wrong direction is a factor of 343 that still looks like a plausible number.
        //
        // COUNTS THE ARITHMETIC, NOT THE IDENTIFIER. The name also appears in the declaration and
        // in setGeometry's two guards and its assignment, none of which convert anything; a count
        // of bare occurrences would move every time that plumbing was touched and would assert
        // nothing about the conversions themselves.
        const crossings = [...VENUE.code.matchAll(/[*\/]\s*speedOfSound/g)].length;
        check(crossings === 2,
            `speedOfSound appears in exactly TWO arithmetic expressions — got ${crossings}`);
        check(/\* speedOfSound/.test(VENUE.code) && /\/ speedOfSound/.test(VENUE.code),
            'and they are one multiply and one divide — the pair, not the same direction twice');
    }

    // The C++ side must actually SEND them, or every check above passes vacuously
    // (pattern_gate_passes_because_of_a_different_bug).
    check(/setProperty \("speedOfSound", oo::plane::kSpeedOfSoundMps\)/.test(CODE.editorCpp),
        'getVenueGeometry sends oo::plane::kSpeedOfSoundMps — so there IS a number to divide by');
    check(/setProperty \("maxDelayMs",\s+OOctagonProcessor::kVenueDelayClampMs\)/.test(CODE.editorCpp),
        'and the rail comes from OOctagonProcessor::kVenueDelayClampMs');
}

// ─────────────────────────────────────────────────────────────────── done ──
console.log(`\n${failed === 0 ? 'ALL SECTIONS PASS' : `${failed} FAILED`} — 43 sections`);
process.exit(failed);
