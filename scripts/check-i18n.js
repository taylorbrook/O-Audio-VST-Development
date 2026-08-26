/*
   This file is part of the Ouaricon Audio plugin suite.
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

    check-i18n.js — repo-level i18n consistency + drift gate.

    CONTEXT.md accepts 43 hand-copies of the i18n runtime as a deliberate cost,
    matching this repo's existing no-shared-UI-module convention. This script is
    the only mitigation available under that rule: it makes the cost DETECTABLE.

    Nine assertions per localized plugin:

      1  en and fr key sets in I18N are identical, and every entry has t + b.
      2  every key referenced by TIP_BINDINGS exists in I18N.
      3  index.html carries ZERO data-tip / data-tip-title / data-tooltip
         literals — the copy has fully left the markup.
      4  no French entry is a straight passthrough of the English unless it
         carries an explicit `sameAsEn: true`.
      5  every fr entry carries an explicit boolean `reviewed`.
      6  DRIFT GATE — the applyI18n + initI18n region of app.js, comment-
         stripped and whitespace-normalised, equals scripts/i18n-canon.js.
      7  i18n.js has no top-level statement outside export declarations.
      8  i18n.js is in juce_add_binary_data SOURCES *and* in a getResource()
         branch. (Generalises O-Octagon's §21 to all 43 — the highest-frequency
         mistake in this work: a file embedded but not served, or served but not
         embedded, is a 404 that presents as a missing panel and nothing else.)
      9  i18n.js references no innerHTML and no string literal contains `<` —
         machine-drafted French must not open a markup path.

    Usage:
        node scripts/check-i18n.js
        node scripts/check-i18n.js --plugin O-MultiBandCompressor
        node scripts/check-i18n.js --root /tmp/fixture      (negative controls)

    Exit code = number of failed assertions (0 = all pass), matching the
    existing per-plugin gates.

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');
const vm   = require('vm');

const CANON = require(path.join(__dirname, 'i18n-canon.js'));

// ─────────────────────────────────────────────────────────────────── args ──
const argv = process.argv.slice(2);
const argValue = (flag) => {
    const i = argv.indexOf(flag);
    return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null;
};

const onlyPlugin = argValue('--plugin');
const repoRoot   = argValue('--root') || path.resolve(__dirname, '..');
const pluginsDir = path.join(repoRoot, 'plugins');

// ──────────────────────────────────────────────────────────── assertions ──
let failed = 0;
let scope  = '';

function head(name) {
    scope = name;
    console.log(`\n-- ${name}`);
}

function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: [${scope}] ${desc}`);
    if (!cond) ++failed;
}

// ────────────────────────────────────────────────────────── JS scanning ──
// One scanner serves three jobs: comment stripping, string-literal collection,
// and top-level statement segmentation. It has to be string- AND regex-aware,
// because the canonical tr() contains /\{(\w+)\}/g — a regex whose braces would
// corrupt naive depth tracking, and whose slashes would look like a comment.
const REGEX_PRECEDERS = new Set(['', '(', ',', '=', ':', '[', '!', '&', '|', '?',
                                 '{', '}', ';', '+', '-', '*', '%', '~', '^',
                                 '<', '>', 'n' /* return */]);

function scanJs(src) {
    const strings = [];
    const topLevel = [];
    let code = '';
    let depth = 0;
    let prevSig = '';
    let stmtStart = 0;
    let i = 0;
    const n = src.length;

    while (i < n) {
        const c = src[i];
        const d = src[i + 1];

        if (c === '/' && d === '/') {
            while (i < n && src[i] !== '\n') ++i;
            continue;
        }

        if (c === '/' && d === '*') {
            i += 2;
            while (i < n && !(src[i] === '*' && src[i + 1] === '/')) ++i;
            i += 2;
            continue;
        }

        if (c === '"' || c === "'" || c === '`') {
            let j = i + 1;
            let lit = '';
            while (j < n) {
                if (src[j] === '\\') { lit += src[j] + (src[j + 1] || ''); j += 2; continue; }
                if (src[j] === c) break;
                lit += src[j];
                ++j;
            }
            strings.push(lit);
            code += c + lit + c;
            i = j + 1;
            prevSig = c;
            continue;
        }

        if (c === '/' && REGEX_PRECEDERS.has(prevSig)) {
            let j = i + 1;
            let inClass = false;
            while (j < n) {
                if (src[j] === '\\') { j += 2; continue; }
                if (src[j] === '[') inClass = true;
                else if (src[j] === ']') inClass = false;
                else if (src[j] === '/' && !inClass) break;
                else if (src[j] === '\n') break;
                ++j;
            }
            code += src.slice(i, j + 1);
            i = j + 1;
            while (i < n && /[gimsuyd]/.test(src[i])) { code += src[i]; ++i; }
            prevSig = '/';
            continue;
        }

        if (c === '(' || c === '[' || c === '{') ++depth;
        if (c === ')' || c === ']' || c === '}') --depth;

        code += c;
        if (!/\s/.test(c)) prevSig = c;

        if (depth === 0 && (c === ';' || c === '}')) {
            const text = code.slice(stmtStart).trim();
            if (text) topLevel.push(text);
            stmtStart = code.length;
        }

        ++i;
    }

    const tail = code.slice(stmtStart).trim();
    if (tail) topLevel.push(tail);

    return { code, strings, topLevel };
}

const normalise = s => s.replace(/\s+/g, ' ').trim();

// Pull the applyI18n + initI18n region out of a file: from the first
// declaration through the close of initI18n. Returns null if either end is
// missing, which is itself a failure the caller reports.
function extractI18nRegion(code) {
    const start = code.indexOf(CANON.I18N_CANON_BODY_START);
    if (start < 0) return null;

    const fnAt = code.indexOf(`function ${CANON.I18N_CANON_BODY_END_FN}`, start);
    if (fnAt < 0) return null;

    const braceAt = code.indexOf('{', fnAt);
    if (braceAt < 0) return null;

    let depth = 0;
    for (let i = braceAt; i < code.length; ++i) {
        if (code[i] === '{') ++depth;
        else if (code[i] === '}') {
            --depth;
            if (depth === 0) return code.slice(start, i + 1);
        }
    }
    return null;
}

const CANON_REGION = (() => {
    const region = extractI18nRegion(scanJs(CANON.I18N_CANON).code);
    if (!region) {
        console.error('check-i18n: could not extract the canon region from scripts/i18n-canon.js. '
                    + 'The drift gate cannot run — refusing to pass vacuously.');
        process.exit(1);
    }
    return normalise(region);
})();

// ──────────────────────────────────────────────────── loading i18n.js ──
// i18n.js is an ES module living outside any package.json, so node cannot
// require() it and cannot import() it as ESM either. Strip the export keyword
// and evaluate the declarations in a sandbox. Assertion 7 independently proves
// the file has nothing but export declarations in it, so there is nothing else
// to evaluate.
function loadI18nModule(src) {
    const transformed = src.replace(/(^|\n)(\s*)export\s+(const|let|function|class)\s/g,
                                    '$1$2$3 ');
    const sandbox = { console: { warn() {}, error() {}, log() {} } };
    vm.createContext(sandbox);
    vm.runInContext(
        `${transformed}\n;globalThis.__i18nExports = { LANGUAGES, I18N, TIP_BINDINGS, tr };`,
        sandbox,
        { timeout: 5000 });
    return sandbox.__i18nExports;
}

// ──────────────────────────────────────────────────────── HTML comments ──
const stripHtmlComments = src => src.replace(/<!--[\s\S]*?-->/g, '');

// ───────────────────────────────────────────────────────────── discovery ──
const UI_ROOTS = [
    ['Source', 'ui', 'public'],
    ['Resources', 'ui'],
];

function discover() {
    if (!fs.existsSync(pluginsDir)) return [];

    const found = [];

    for (const name of fs.readdirSync(pluginsDir).sort()) {
        if (onlyPlugin && name !== onlyPlugin) continue;

        const pluginRoot = path.join(pluginsDir, name);
        if (!fs.statSync(pluginRoot).isDirectory()) continue;

        for (const rootParts of UI_ROOTS) {
            const uiRoot  = path.join(pluginRoot, ...rootParts);
            const i18nJs  = path.join(uiRoot, 'js', 'i18n.js');
            if (!fs.existsSync(i18nJs)) continue;

            found.push({
                name,
                pluginRoot,
                uiRoot,
                uiRootLabel: rootParts.join('/'),
                i18nJs,
                appJs:     path.join(uiRoot, 'js', 'app.js'),
                indexHtml: path.join(uiRoot, 'index.html'),
                cmake:     path.join(pluginRoot, 'CMakeLists.txt'),
                editorCpp: path.join(pluginRoot, 'Source', 'PluginEditor.cpp'),
            });
        }
    }

    return found;
}

// ──────────────────────────────────────────────────────────── the checks ──
function checkPlugin(p) {
    head(`${p.name} (${p.uiRootLabel})`);

    const i18nSrc  = fs.readFileSync(p.i18nJs, 'utf8');
    const i18nScan = scanJs(i18nSrc);

    // ── 7. no top-level statement outside export declarations ────────────
    // Checked FIRST: assertion 1-5 evaluate this file, and evaluating a file
    // with side effects is exactly what the rule forbids.
    const strays = i18nScan.topLevel.filter(s => !/^export\s/.test(s));
    check(strays.length === 0,
        `[7] i18n.js has no top-level statement outside export declarations`
        + (strays.length ? ` — found ${strays.length}: ${strays.map(s => s.slice(0, 48)).join(' | ')}` : ''));

    // ── 9. no markup path ────────────────────────────────────────────────
    check(!/innerHTML/.test(i18nScan.code),
        '[9] i18n.js references no innerHTML');

    const angled = i18nScan.strings.filter(s => s.includes('<'));
    check(angled.length === 0,
        `[9] no string literal in i18n.js contains "<"`
        + (angled.length ? ` — found ${angled.length}: ${angled.slice(0, 3).map(s => s.slice(0, 40)).join(' | ')}` : ''));

    // ── load the table ───────────────────────────────────────────────────
    let mod = null;
    try {
        mod = loadI18nModule(i18nSrc);
    } catch (e) {
        check(false, `[1] i18n.js evaluates — ${e.message}`);
        return;
    }

    const { LANGUAGES, I18N, TIP_BINDINGS } = mod;

    check(Array.isArray(LANGUAGES) && LANGUAGES.join(',') === 'en,fr',
        `[1] LANGUAGES is exactly ['en','fr'] — got ${JSON.stringify(LANGUAGES)}`);

    const keys = I18N && typeof I18N === 'object' ? Object.keys(I18N) : [];
    check(keys.length > 0, `[1] I18N has entries — got ${keys.length}`);

    // ── 1. en/fr key sets identical, every entry has t + b ───────────────
    const missingLang = [];
    const missingField = [];
    for (const k of keys) {
        const e = I18N[k] || {};
        for (const lang of ['en', 'fr']) {
            if (!e[lang]) { missingLang.push(`${k}.${lang}`); continue; }
            if (typeof e[lang].t !== 'string' || typeof e[lang].b !== 'string')
                missingField.push(`${k}.${lang}`);
        }
    }
    check(missingLang.length === 0,
        `[1] every I18N key has both en and fr`
        + (missingLang.length ? ` — missing: ${missingLang.slice(0, 6).join(', ')}` : ''));
    check(missingField.length === 0,
        `[1] every language entry has string t and b`
        + (missingField.length ? ` — malformed: ${missingField.slice(0, 6).join(', ')}` : ''));

    // ── 2. TIP_BINDINGS keys resolve ─────────────────────────────────────
    const bindings = Array.isArray(TIP_BINDINGS) ? TIP_BINDINGS : [];
    check(bindings.length > 0, `[2] TIP_BINDINGS has entries — got ${bindings.length}`);

    const unresolved = bindings.filter(b => !Array.isArray(b) || !(b[1] in (I18N || {})))
                               .map(b => (Array.isArray(b) ? b[1] : String(b)));
    check(unresolved.length === 0,
        `[2] every TIP_BINDINGS key exists in I18N`
        + (unresolved.length ? ` — dangling: ${unresolved.slice(0, 6).join(', ')}` : ''));

    // ── 4. no silent French passthrough ──────────────────────────────────
    const passthrough = keys.filter(k => {
        const e = I18N[k];
        if (!e || !e.en || !e.fr) return false;
        if (e.fr.sameAsEn === true) return false;
        return e.fr.t === e.en.t && e.fr.b === e.en.b;
    });
    check(passthrough.length === 0,
        `[4] no fr entry is a straight copy of en without sameAsEn: true`
        + (passthrough.length ? ` — ${passthrough.length}: ${passthrough.slice(0, 6).join(', ')}` : ''));

    // ── 5. explicit reviewed flag ────────────────────────────────────────
    const unflagged = keys.filter(k => typeof (I18N[k] && I18N[k].fr || {}).reviewed !== 'boolean');
    check(unflagged.length === 0,
        `[5] every fr entry carries an explicit boolean reviewed`
        + (unflagged.length ? ` — missing on ${unflagged.length}: ${unflagged.slice(0, 6).join(', ')}` : ''));

    const unreviewed = keys.filter(k => (I18N[k] && I18N[k].fr || {}).reviewed === false).length;

    // ── 3. copy has left the markup ──────────────────────────────────────
    if (!fs.existsSync(p.indexHtml)) {
        check(false, '[3] index.html exists');
    } else {
        const html = stripHtmlComments(fs.readFileSync(p.indexHtml, 'utf8'));
        for (const attr of ['data-tip=', 'data-tip-title=', 'data-tooltip=']) {
            const hits = (html.match(new RegExp(attr.replace(/-/g, '\\-'), 'g')) || []).length;
            check(hits === 0, `[3] index.html carries zero ${attr} literals — got ${hits}`);
        }
    }

    // ── 6. drift gate ────────────────────────────────────────────────────
    if (!fs.existsSync(p.appJs)) {
        check(false, '[6] app.js exists');
    } else {
        const appScan = scanJs(fs.readFileSync(p.appJs, 'utf8'));

        check(appScan.code.includes(CANON.I18N_CANON_IMPORT),
            '[6] app.js carries the canonical i18n.js import line verbatim');

        const region = extractI18nRegion(appScan.code);
        if (region === null) {
            check(false, '[6] app.js contains an extractable applyI18n/initI18n region');
        } else {
            check(normalise(region) === CANON_REGION,
                '[6] the applyI18n/initI18n region matches scripts/i18n-canon.js');
        }

        check(/initI18n\s*\(\s*\)\s*;/.test(appScan.code.replace(/function\s+initI18n\s*\(\s*\)/, '')),
            '[6] initI18n() is actually CALLED — a block nobody calls localizes nothing');
    }

    // ── 8. embedded AND served ───────────────────────────────────────────
    if (!fs.existsSync(p.cmake)) {
        check(false, '[8] CMakeLists.txt exists');
    } else {
        const cmake = fs.readFileSync(p.cmake, 'utf8')
                        .replace(/(^|\n)\s*#[^\n]*/g, '$1');
        const inSources = /juce_add_binary_data\s*\([\s\S]*?\)/g;
        let embedded = false;
        for (const block of cmake.match(inSources) || []) {
            if (/[\w/]*ui[\w/]*\/js\/i18n\.js/.test(block)) embedded = true;
        }
        check(embedded, '[8] js/i18n.js appears in a juce_add_binary_data SOURCES block');
    }

    if (!fs.existsSync(p.editorCpp)) {
        check(false, '[8] PluginEditor.cpp exists');
    } else {
        const editor = scanJs(fs.readFileSync(p.editorCpp, 'utf8')).code;
        check(/["']\/?js\/i18n\.js["']/.test(editor) && /i18n_js/.test(editor),
            '[8] PluginEditor.cpp serves js/i18n.js from a getResource() branch');
    }

    return { keys: keys.length, unreviewed };
}

// ───────────────────────────────────────────────────────────────── main ──
const plugins = discover();

console.log('check-i18n — repo i18n consistency + drift gate');
console.log(`  repo root: ${repoRoot}`);
if (onlyPlugin) console.log(`  filter:    --plugin ${onlyPlugin}`);

if (plugins.length === 0) {
    console.log(onlyPlugin
        ? `\n0 plugins localized (no i18n.js under either UI root of ${onlyPlugin})`
        : '\n0 plugins localized');
    process.exit(0);
}

const summary = [];
for (const p of plugins) {
    const r = checkPlugin(p);
    if (r) summary.push({ name: p.name, ...r });
}

console.log('\n-- unreviewed French (native-speaker worklist)');
if (summary.length === 0) {
    console.log('  (nothing to report — a plugin failed before its table could be read)');
} else {
    for (const s of summary)
        console.log(`  ${s.name.padEnd(30)} ${String(s.unreviewed).padStart(4)} / ${s.keys} entries unreviewed`);
    const total = summary.reduce((a, s) => a + s.unreviewed, 0);
    console.log(`  ${'TOTAL'.padEnd(30)} ${String(total).padStart(4)}`);
}

console.log(`\n${failed === 0 ? 'ALL CHECKS PASS' : `${failed} FAILED`} — ${plugins.length} localized plugin(s)`);
process.exit(failed);
