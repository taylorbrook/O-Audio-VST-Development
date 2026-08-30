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

    Fifteen assertions per localized plugin. 1-9 apply to every plugin; 10-13
    and 15 describe canon v2 and are reported SKIPPED on a plugin still on v1,
    because a gate that goes red the moment it is written and stays red for a
    whole rollout teaches the team to ignore gates:

      1  en and fr key sets in I18N are identical, and every entry has t + b.
      2  every key referenced by TIP_BINDINGS exists in I18N, and the bound
         COUNT is reported. Zero bindings is legitimate — a plugin with no
         hover-help — but only when no I18N entry carries a body; an emptied
         TIP_BINDINGS over a bodied table is orphaned copy and fails.
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
     10  LABEL COVERAGE (v2). Every HTML text node the extractor classifies
         LABEL sits inside a [data-i18n] element, or matches an I18N_EXEMPT
         entry. WITHOUT THIS a plugin passes at 100% tooltip coverage while
         every label is still hard-coded English — the assertion the whole
         expansion is unverifiable without.
     11  ATTRIBUTE COVERAGE (v2). Every aria-label / placeholder / alt carrying
         prose is keyed or exempt, and ZERO native title= attributes remain
         (contract §4 — a native title on an element that has a data-tip renders
         a second, untranslated OS tooltip).
     12  JS-STRING COVERAGE (v2). No prose string is written to textContent /
         innerText in the controller unless it is exempt; a converted site goes
         through setLabel and therefore produces no row at all. Composed
         templates are flagged individually — they need {token} entries.
     13  NO INFLECTION LOGIC inside a localized string (v2). A ternary or a
         conditional plural suffix inside a setLabel argument fails, and a
         setLabel key that is not a plain string literal fails (contract §6).
     14  Every I18N_EXEMPT entry carries a non-empty reason, and no UNSCOPED
         entry collides with a key that is live on the same page. A bare skip
         list hides a missed label as a deliberate one; an entry is
         [text, reason] or [text, reason, scope], where scope is a
         comma-separated list of `tag`, `.class` or `#id` matched against the
         node's own parent and its ancestors. Unscoped stays legal — most
         exemptions are not ambiguous — but a string that is exempt AND keyed
         on the same page must say WHERE it is exempt.
     15  KEYS RESOLVE (v2). Every data-i18n value in the markup and every
         setLabel key exists in LABELS or I18N, and every LABELS key is
         referenced by at least one element or setLabel call — a dead key is a
         translation nobody sees, drifting silently.

    Usage:
        node scripts/check-i18n.js
        node scripts/check-i18n.js --plugin O-MultiBandCompressor
        node scripts/check-i18n.js --root /tmp/fixture      (negative controls)

    CANON V2 IS THE ONLY CANON, as of Stage L. All 43 plugins are on it, canon
    v1 has been deleted from i18n-canon.js, and a plugin whose applyI18n/initI18n
    region matches neither now simply fails assertion 6.

    `--strict-v2` is still ACCEPTED and does nothing. It is spelled into the
    stage briefs, the plugin CHANGELOGs and a year of commit messages, and a
    flag that used to mean something must not start erroring at the one moment
    somebody pastes an old command to check an old claim.

    Exit code = number of failed assertions (0 = all pass), matching the
    existing per-plugin gates.

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');
const vm   = require('vm');

const CANON = require(path.join(__dirname, 'i18n-canon.js'));

// Assertions 10-12 must count exactly what i18n-extract.js counts. Two
// independent scanners disagreeing about what a label IS would produce a gate
// that contradicts the worklist a plugin stage is working from — and the
// resolution would be a judgement call every time.
const EXTRACT = require(path.join(__dirname, 'i18n-extract.js'));

// ─────────────────────────────────────────────────────────────────── args ──
const argv = process.argv.slice(2);
const argValue = (flag) => {
    const i = argv.indexOf(flag);
    return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null;
};

const onlyPlugin = argValue('--plugin');
// Accepted and ignored — canon v2 is the only canon. See the header.
void argv.includes('--strict-v2');
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
            // A `}` at depth zero already closed the statement, so the `;` that
            // terminates `export const X = {...};` opens a fresh segment holding
            // nothing but that semicolon. An empty statement is not a top-level
            // statement, and rejecting it would force every plugin to write the
            // object-literal exports in a shape that dodges this scanner rather
            // than one chosen on its merits.
            if (text && text !== ';') topLevel.push(text);
            stmtStart = code.length;
        }

        ++i;
    }

    const tail = code.slice(stmtStart).trim();
    if (tail && tail !== ';') topLevel.push(tail);

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

const canonRegion = (src, label) => {
    const region = extractI18nRegion(scanJs(src).code);
    if (!region) {
        console.error(`check-i18n: could not extract the ${label} region from scripts/i18n-canon.js. `
                    + 'The drift gate cannot run — refusing to pass vacuously.');
        process.exit(1);
    }
    return normalise(region);
};

const CANON_REGION_V2 = canonRegion(CANON.I18N_CANON_V2, 'canon v2');

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
    // LABELS and I18N_EXEMPT are v2 additions and are absent on a v1 plugin, so
    // they are read through typeof rather than named directly — a ReferenceError
    // here would report "i18n.js does not evaluate" on a file that is perfectly
    // correct for the canon it is on.
    vm.runInContext(
        `${transformed}\n;globalThis.__i18nExports = { LANGUAGES, I18N, TIP_BINDINGS, tr,`
        + ` LABELS: typeof LABELS === 'undefined' ? null : LABELS,`
        + ` I18N_EXEMPT: typeof I18N_EXEMPT === 'undefined' ? null : I18N_EXEMPT };`,
        sandbox,
        { timeout: 5000 });
    return sandbox.__i18nExports;
}

// ──────────────────────────────────────────────────────── HTML comments ──
const stripHtmlComments = src => src.replace(/<!--[\s\S]*?-->/g, '');

// ────────────────────────────────────────────────── inline module source ──
// A plugin whose controller is an inline <script type="module"> rather than a
// js/app.js file. O-Bitrot is the first; nothing about that layout is wrong, so
// the drift gate reads the module from wherever it actually lives.
//
// Deliberately matches only type="module": a classic <script> cannot carry an
// `import`, so scanning one would look for a canon block that could not be
// there and report a confusing failure. Returns the LARGEST module block, which
// on a page with several is the controller rather than a shim.
function readInlineModule(indexHtml) {
    if (!fs.existsSync(indexHtml)) return null;

    const html = fs.readFileSync(indexHtml, 'utf8');
    const blocks = [...html.matchAll(/<script\b[^>]*type=["']module["'][^>]*>([\s\S]*?)<\/script>/g)]
        .map(m => m[1])
        .filter(code => code.trim().length > 0);

    if (blocks.length === 0) return null;

    blocks.sort((a, b) => b.length - a.length);
    return { label: 'the inline <script type="module"> in index.html',
             code: blocks[0], inline: true };
}

// ───────────────────────────────────────────── external module script src ──
// A plugin whose controller is a MODULE FILE the page loads by src, under a
// name that is not app.js. O-Texture is the first: its page ends with
// <script type="module" src="js/main.js">, and js/main.js is a perfectly
// ordinary controller carrying the canon block, the relays and every binding.
//
// Before this, the resolver was `js/app.js OR an inline block`, so O-Texture
// fell through both and assertion 6 reported "a controller module exists"
// FALSE on a plugin that is entirely correct — the wrong-shaped assertion this
// file's own assertion-6 comment warns about, one filename convention later.
//
// DERIVED FROM THE PAGE, never from a filename guess: whatever index.html
// actually loads as type="module" IS the controller. js/juce/ is excluded — it
// is verbatim JUCE frontend code, not authored page code, and on every plugin
// here it is imported by the controller rather than loaded by the page. When a
// page loads more than one, the LARGEST wins, which is the same tie-break
// readInlineModule already uses and for the same reason.
function readModuleScriptSrc(indexHtml, uiRoot) {
    if (!fs.existsSync(indexHtml)) return null;

    const html = fs.readFileSync(indexHtml, 'utf8');
    const candidates = [];

    for (const m of html.matchAll(/<script\b[^>]*\btype=["']module["'][^>]*>/g)) {
        const tag = m[0];
        const src = tag.match(/\bsrc=["']([^"']+)["']/);
        if (!src) continue;                                  // inline; not ours

        const rel = src[1].replace(/^\.?\//, '');
        if (/^(https?:)?\/\//.test(src[1])) continue;          // remote
        if (rel.startsWith('js/juce/')) continue;            // verbatim JUCE

        const full = path.join(uiRoot, rel);
        if (!fs.existsSync(full) || !fs.statSync(full).isFile()) continue;

        const code = fs.readFileSync(full, 'utf8');
        if (code.trim().length === 0) continue;
        candidates.push({ label: rel, code, inline: false });
    }

    if (candidates.length === 0) return null;

    candidates.sort((a, b) => b.code.length - a.code.length);
    return candidates[0];
}

// ───────────────────────────────────────────── every inline script on the page ──
// readInlineModule above answers "where does the CANON block live" and returns
// ONE block. Assertions 12, 13 and 15 ask a different question — "does any
// shipped JS write raw prose, and is every key both live and resolvable" — and
// for that the answer is every script the page actually runs.
//
// THE HOLE THIS CLOSES. pageModules was assembled as: the inline module IF it
// was also the canon target, plus the .js files in the ui/js directory. On a
// plugin that has js/app.js AND an inline controller, moduleSrc resolves to
// app.js, `moduleSrc.inline` is false, and the inline block is in no directory
// — so it was scanned by nothing. O-Lyrica is that shape: a 1,647-line inline
// <script type="module"> holding the tuning controller and a ~350-line classic
// <script> holding the scale-generator forms, between them writing "Factory",
// "Custom", "No presets available" and three innerHTML label forms. All of it
// would have passed assertion 12 green by not being looked at.
//
// Same shape as the venue.js gap and the dataset.i18nAria gap before it: the
// gate reporting a rule as satisfied because its scan could not reach the code.
// DERIVED FROM THE PAGE, never from whichever single module happened to be the
// canon target.
//
// CLASSIC SCRIPTS ARE INCLUDED HERE and are deliberately NOT included in
// readInlineModule. The exclusion there is correct and stays: a classic script
// cannot carry an `import`, so hunting a canon block in one would report a
// confusing failure. But it can absolutely write prose to textContent, which is
// all assertions 12/13/15 read. The type filter matches i18n-extract.js's own
// jsSources loop exactly, so the GATE and the WORKLIST see the same set.
function readInlineScripts(indexHtml) {
    if (!fs.existsSync(indexHtml)) return [];

    const html = fs.readFileSync(indexHtml, 'utf8');
    const { elements } = EXTRACT.scanHtml(html);
    const out = [];

    for (const el of elements) {
        if (el.tag !== 'script' || !el.raw) continue;
        const type = el.attrs.type ? el.attrs.type.value : '';
        if (type && type !== 'module' && type !== 'text/javascript') continue;
        if (el.raw.trim().length === 0) continue;
        out.push({
            label: `index.html inline <script${type ? ` type="${type}"` : ''}>`,
            code: el.raw,
            inline: true,
        });
    }

    return out;
}

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

    // A plugin with NO hover-help has an EMPTY I18N and a populated LABELS: it
    // is a page that speaks French with nothing to hover. `keys.length > 0`
    // would fail every one of those, so the requirement is that the TABLE
    // carries copy — in either half — and both counts are reported either way.
    const labelKeysEarly = (mod.LABELS && typeof mod.LABELS === 'object')
        ? Object.keys(mod.LABELS) : [];
    check(keys.length + labelKeysEarly.length > 0,
        `[1] the table carries copy — ${keys.length} I18N + ${labelKeysEarly.length} LABELS`);

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

    // `bindings.length > 0` was right while every localized plugin had hover-
    // help. It is wrong for a plugin that has none: zero bindings is that
    // plugin's CORRECT state, not a defect.
    //
    // Dropping the assertion outright would go vacuous the other way — a
    // shipped tooltip plugin whose TIP_BINDINGS was emptied by an edit would
    // pass with 40 orphaned tooltip bodies in its table and nothing on screen.
    //
    // The discriminator is the TABLE, not the count: a tooltip entry is the one
    // that carries a non-empty body `b`. (A LABEL lives in LABELS; a composed
    // string housed in I18N carries an EMPTY body — O-Polystutter v1.14.0 —
    // and correctly does not demand a binding.) So: bindings may be empty ONLY
    // if no I18N entry has a body. Either way the count is REPORTED, so "0 tips
    // bound" is a line in the log rather than a silent pass.
    const tipBodied = keys.filter((k) => {
        const e = I18N[k] || {};
        return ['en', 'fr'].some((l) => e[l] && typeof e[l].b === 'string' && e[l].b.trim() !== '');
    });
    check(bindings.length > 0 || tipBodied.length === 0,
        `[2] ${bindings.length} tip(s) bound`
        + (bindings.length > 0 ? ''
           : tipBodied.length === 0
             ? ' — this plugin has no hover-help, which is a state, not a gap'
             : ` — ORPHANED: ${tipBodied.length} I18N entries carry a body that nothing binds`
               + `: ${tipBodied.slice(0, 6).join(', ')}`));

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

    // ── 1 / 4 / 5, applied to LABELS as well ─────────────────────────────
    // Assertions 1, 4 and 5 and the reviewer worklist above read I18N only.
    // Stage F landed 40 French LABEL strings on O-Tapestop and EVERY ONE was
    // invisible to all four: unchecked for an en/fr pair, unchecked for a
    // silent English passthrough, unchecked for the reviewed flag, and absent
    // from the native-speaker worklist that is the whole point of flagging
    // machine drafts. Forty unreviewed strings reported as zero is worse than
    // no worklist, because it reads as done.
    //
    // A LABELS entry is {en:{t}, fr:{t, reviewed}} — one string, no body,
    // because a label is not a tooltip. So this is the same three checks
    // against a narrower shape, not a copy of the block above.
    const LABELS_EARLY = mod.LABELS;
    let labelsUnreviewed = 0;
    let labelsCount = 0;
    if (LABELS_EARLY != null) {
        const lkeys = typeof LABELS_EARLY === 'object' ? Object.keys(LABELS_EARLY) : [];
        labelsCount = lkeys.length;

        const lMissing = [];
        for (const k of lkeys) {
            const e = LABELS_EARLY[k] || {};
            for (const lang of ['en', 'fr'])
                if (!e[lang] || typeof e[lang].t !== 'string') lMissing.push(`${k}.${lang}`);
        }
        check(lMissing.length === 0,
            `[1] every LABELS key has en and fr with a string t`
            + (lMissing.length ? ` — ${lMissing.length} malformed: ${lMissing.slice(0, 6).join(', ')}` : ''));

        const lPass = lkeys.filter((k) => {
            const e = LABELS_EARLY[k];
            if (!e || !e.en || !e.fr) return false;
            if (e.fr.sameAsEn === true) return false;
            return e.fr.t === e.en.t;
        });
        check(lPass.length === 0,
            `[4] no LABELS fr entry is a straight copy of en without sameAsEn: true`
            + (lPass.length ? ` — ${lPass.length}: ${lPass.slice(0, 6).join(', ')}` : ''));

        const lUnflagged = lkeys.filter((k) =>
            typeof ((LABELS_EARLY[k] && LABELS_EARLY[k].fr) || {}).reviewed !== 'boolean');
        check(lUnflagged.length === 0,
            `[5] every LABELS fr entry carries an explicit boolean reviewed`
            + (lUnflagged.length ? ` — missing on ${lUnflagged.length}: ${lUnflagged.slice(0, 6).join(', ')}` : ''));

        labelsUnreviewed = lkeys.filter((k) =>
            ((LABELS_EARLY[k] && LABELS_EARLY[k].fr) || {}).reviewed === false).length;
    }

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
    //
    // The controller does NOT always live in js/app.js. O-Bitrot's is one
    // inline <script type="module"> in index.html and O-Texture's is a module
    // FILE the page loads by src under another name (js/main.js) — both
    // legitimate layouts this gate has to describe rather than fail: reporting
    // "[6] app.js exists" FALSE on a plugin that is entirely correct is the
    // same wrong-shaped assertion as O-Octagon's double-quote-only import scan.
    //
    // The import SPECIFIER differs with the module's depth, and only the
    // specifier does. From js/app.js — or any other module file in js/, such as
    // O-Texture's js/main.js — the table is './i18n.js'; from an inline module
    // at the UI root it is './js/i18n.js'. Both are accepted; the
    // applyI18n/initI18n BODY that this gate byte-compares is identical either
    // way, and that body is the part 43 hand-copies can actually drift in.
    //
    // THREE SHAPES, IN ORDER, and the order is load-bearing: js/app.js first so
    // no already-migrated plugin can change verdict, then whatever the page
    // loads as type="module" by src (O-Texture's js/main.js), then the inline
    // block. A plugin with js/app.js resolves exactly as it did before this
    // clause existed.
    const moduleSrc = fs.existsSync(p.appJs)
        ? { label: 'app.js', code: fs.readFileSync(p.appJs, 'utf8'), inline: false }
        : (readModuleScriptSrc(p.indexHtml, p.uiRoot) || readInlineModule(p.indexHtml));

    // ── THE CONTROLLER IS ONE FILE; THE PAGE IS NOT ──────────────────────
    //
    // moduleSrc above is the CANON-DRIFT target and stays one file: the
    // applyI18n/initI18n block lives in exactly one place, and assertion 6
    // byte-compares it there.
    //
    // Assertions 12, 13 and 15 are a different question. They ask "does any
    // shipped JS write raw prose, and is every key both live and resolvable" —
    // and eleven plugins in this repo split their page across sibling modules
    // that moduleSrc never names. O-Octagon has SEVEN: js/venue.js alone is 796
    // lines of controller code. Until this fix, two raw English prose writes in
    // venue.js passed assertion 12 green, and two keys referenced ONLY from
    // venue.js reported as DEAD — the gate describing a violation of a rule the
    // code was obeying, and in the same breath passing a rule the code was
    // breaking.
    //
    // DERIVED FROM THE DIRECTORY, never a transcribed list. That is the move
    // O-Octagon's own ui_frontend_check §21 makes for the same reason: a list
    // naming [app.js] literally makes every assertion over it PASS BY NOT
    // LOOKING the moment a second module appears. js/juce/ is excluded — it is
    // verbatim JUCE, not authored page code.
    const jsDir = path.join(path.dirname(p.i18nJs));
    const pageModules = [];
    // Every inline script the page runs, whether or not one of them is also the
    // canon target. Deduped by code so a plugin whose controller IS the inline
    // module is not scanned twice; moduleSrc's label is kept because it is the
    // one the drift assertions above already name.
    const inlineScripts = readInlineScripts(p.indexHtml);
    if (moduleSrc !== null && moduleSrc.inline) {
        pageModules.push(moduleSrc);
    }
    for (const s of inlineScripts) {
        if (pageModules.some((m) => m.code === s.code)) continue;
        pageModules.push(s);
    }
    // THE GATE AND THE WORKLIST NEED DIFFERENT SETS, and the comment above was
    // wrong to claim otherwise ("the type filter matches i18n-extract.js's own
    // jsSources loop exactly, so the GATE and the WORKLIST see the same set").
    //
    // i18n-extract's JS_SKIP answers "whose code may this tool propose edits
    // to", so it drops vendored shared modules: editing one here edits every
    // plugin that embeds it. That is a WORKLIST concern. Assertion 15 asks a
    // different question — "is every key live" — and a vendored module the page
    // actually runs references keys and writes prose like any other script.
    // Applying JS_SKIP wholesale reported 37 live O-IntonationPad keys as DEAD,
    // because its diverged local `js/tuning-panel.js` copy is where they are
    // referenced. So only the ONE overlapping reason is borrowed:
    //
    // A MINIFIED BUNDLE IS NOT MERELY NOISY TO SCAN, IT IS UNPARSEABLE.
    // `stripJsComments` disambiguates a regex literal from a division by
    // context, and minified code defeats that: on O-TextureForge's 220,686-byte
    // `app.bundle.js` it blanked 38,118 bytes — 17.3% — across 711 runs, and
    // `window.__setLabel` at byte 213,029 fell inside a blanked run. So the
    // gate reported all EIGHT of that plugin's live keys as DEAD (assertion 15)
    // while assertion 12 went vacuous in the same breath: raw English prose
    // planted in the bundle and rebuilt was scanned and PASSED.
    const bundlesSkipped = [];
    if (fs.existsSync(jsDir)) {
        for (const f of fs.readdirSync(jsDir).sort()) {
            if (!f.endsWith('.js') || f === 'i18n.js') continue;
            const full = path.join(jsDir, f);
            if (!fs.statSync(full).isFile()) continue;
            const rel = 'js/' + f;
            if (/\.(bundle|min)\.js$/.test(f)) { bundlesSkipped.push(rel); continue; }
            pageModules.push({ label: rel, code: fs.readFileSync(full, 'utf8'), inline: false });
        }
    }

    // A bundled plugin's prose lives in the AUTHORED source, which is not in
    // the served root at all — O-TextureForge builds `Source/ui/src/app.js`
    // into `Source/ui/public/js/app.bundle.js`. Skipping the bundle without
    // finding that source would trade a false failure for a silent blindness,
    // which is the worse of the two. So the source tree is scanned in its
    // place, and if there is a bundle with no findable source the gate SAYS SO
    // rather than passing on an empty set.
    if (bundlesSkipped.length) {
        const srcDir = path.join(p.pluginRoot, 'Source', 'ui', 'src');
        const authored = [];
        const walk = (dir) => {
            if (!fs.existsSync(dir)) return;
            for (const e of fs.readdirSync(dir, { withFileTypes: true }).sort((a, b) => a.name.localeCompare(b.name))) {
                const full = path.join(dir, e.name);
                if (e.isDirectory()) { walk(full); continue; }
                if (!e.name.endsWith('.js') || e.name === 'i18n.js') continue;
                if (/\.(bundle|min)\.js$/.test(e.name)) continue;
                const rel = path.relative(p.pluginRoot, full).split(path.sep).join('/');
                authored.push({ label: rel, code: fs.readFileSync(full, 'utf8'), inline: false });
            }
        };
        walk(srcDir);
        for (const m of authored) pageModules.push(m);
        check(authored.length > 0,
            `[12] a served bundle (${bundlesSkipped.join(', ')}) has authored source to scan in `
            + 'Source/ui/src — a minified bundle cannot be parsed, so without its source '
            + 'assertions 12, 13 and 15 can see none of this plugin\'s JS'
            + (authored.length ? ` — ${authored.length} file(s): ${authored.map((m) => m.label).join(', ')}` : ''));
    }

    let canonVersion = null;
    let appCode = '';

    if (moduleSrc === null) {
        check(false, '[6] a controller module exists — js/app.js, or an inline '
            + '<script type="module"> in index.html');
    } else {
        const appScan = scanJs(moduleSrc.code);
        appCode = appScan.code;

        const region = extractI18nRegion(appScan.code);
        const normalised = region === null ? null : normalise(region);

        // ONE canon. During the rollout this accepted either v1 or v2 and
        // reported which, because changing the canon in place would have turned
        // the gate red the moment v2 was committed and kept it red for the whole
        // migration. That migration is finished — all 43 plugins are on v2 — so
        // the dual acceptance is gone and matching v2 is the only pass.
        if (normalised === CANON_REGION_V2) canonVersion = 'v2';

        // './i18n.js' as written, or the same line re-rooted for an inline
        // module. Nothing else passes — a hand-rolled import shape would.
        const importLines = [CANON.I18N_CANON_V2_IMPORT];

        const importOk = importLines.some((line) =>
            appScan.code.includes(line)
            || (moduleSrc.inline && appScan.code.includes(line.replace("'./i18n.js'", "'./js/i18n.js'"))));

        check(importOk,
            `[6] ${moduleSrc.label} carries the canonical i18n.js import line verbatim`
            + (moduleSrc.inline ? " (or its './js/i18n.js' inline-module form)" : '')
            + (canonVersion ? ` — canon ${canonVersion}` : ''));

        if (region === null) {
            check(false, `[6] ${moduleSrc.label} contains an extractable applyI18n/initI18n region`);
        } else {
            check(canonVersion !== null,
                '[6] the applyI18n/initI18n region matches scripts/i18n-canon.js '
                + `(canon v2) — ${canonVersion ? `on ${canonVersion}` : 'does NOT match'}`);
        }

        check(/initI18n\s*\(\s*\)\s*;/.test(appScan.code.replace(/function\s+initI18n\s*\(\s*\)/, '')),
            '[6] initI18n() is actually CALLED — a block nobody calls localizes nothing');
    }

    // ── 3b. data-i18n is a KEY, not copy ─────────────────────────────────
    // Assertion 3 requires the markup to be empty of tooltip-copy literals. It
    // must never start rejecting data-i18n, which names a key. Neither spelling
    // collides today, but the next attribute added could, so the non-collision
    // is asserted rather than assumed.
    for (const attr of ['data-tip=', 'data-tip-title=', 'data-tooltip=']) {
        check(!'data-i18n= data-i18n-aria= data-i18n-placeholder= data-i18n-alt= data-i18n-vars='
                .includes(attr),
            `[3] the assertion-3 literal ${attr} does not match any data-i18n attribute name`);
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

    // ════════════════════════ assertions 10-15 ═══════════════════════════
    const LABELS      = mod.LABELS;
    const I18N_EXEMPT = mod.I18N_EXEMPT;

    // ── 14. exemptions carry reasons. Runs on ANY canon: a v1 plugin that has
    //    started a skip list must not be allowed to grow a reasonless one.
    if (I18N_EXEMPT != null) {
        const bad = (Array.isArray(I18N_EXEMPT) ? I18N_EXEMPT : []).filter(
            (e) => !Array.isArray(e) || typeof e[0] !== 'string' || typeof e[1] !== 'string' || e[1].trim() === '');
        const badScope = (Array.isArray(I18N_EXEMPT) ? I18N_EXEMPT : []).filter(
            (e) => Array.isArray(e) && e.length > 2 && (typeof e[2] !== 'string' || e[2].trim() === ''));
        check(Array.isArray(I18N_EXEMPT),
            `[14] I18N_EXEMPT is an array of [string, reason] or [string, reason, scope] entries`);
        check(badScope.length === 0,
            `[14] every I18N_EXEMPT scope is a non-empty string`
            + (badScope.length ? ` — ${badScope.length} malformed: ${badScope.slice(0, 4).map((e) => JSON.stringify(e[0])).join(', ')}` : ''));
        check(bad.length === 0,
            `[14] every I18N_EXEMPT entry carries a non-empty reason`
            + (bad.length ? ` — ${bad.length} without one: ${bad.slice(0, 4).map((e) => JSON.stringify(Array.isArray(e) ? e[0] : e)).join(', ')}` : ''));
    }

    if (canonVersion !== 'v2') {
        console.log(`  SKIP: [${scope}] [10-15] canon v2 assertions — this plugin does not `
            + `match the canon, so assertion 6 has ALREADY failed it and these would only `
            + `restate that failure against a block the gate cannot read.`);
        return { keys: keys.length + labelsCount, unreviewed: unreviewed + labelsUnreviewed,
                 tipKeys: keys.length, labelKeys: labelsCount, canon: canonVersion };
    }

    // ── I18N_EXEMPT, AND THE SCOPE THAT MAKES AN EXEMPTION HONEST ────────
    //
    // An entry is [text, reason] or [text, reason, scope].
    //
    // WITHOUT A SCOPE AN EXEMPTION IS MATCHED BY TEXT ALONE, so it silences
    // assertion 10 for EVERY node with that text. A string exempt for one
    // element — a choice option, a readout, a wordmark fragment — also silences
    // a DIFFERENT element that says the same thing and was simply never keyed,
    // and a missed label hides as a deliberate one. O-Detune is the case that
    // proved it: `Random` is a wobble_shape option, a unison_dist option, AND a
    // caption that must translate. Delete `label.random` and its data-i18n and
    // the gate stayed GREEN.
    //
    // A scope is a comma-separated list of `tag`, `.class` or `#id`, matched
    // against the text node's own parent and its ancestors. That is the whole
    // language, and it is enough: the five ambiguous strings in the repo when
    // this landed needed exactly three shapes — `option`, `.knob-value` and
    // `.title-accent`. A full CSS engine would be a second parser to keep in
    // step with the first.
    //
    // Unscoped entries REMAIN LEGAL, because most exemptions are not ambiguous:
    // a product name or `Hz` says the same thing everywhere it appears, and
    // demanding a scope there is noise that teaches people to write one without
    // thinking. The scope is required only where the ambiguity is real — see
    // the assertion below assertion 10.
    const exemptEntries = (Array.isArray(I18N_EXEMPT) ? I18N_EXEMPT : [])
        .filter((e) => Array.isArray(e) && typeof e[0] === 'string')
        .map((e) => ({ text: e[0], reason: e[1], scope: typeof e[2] === 'string' && e[2].trim() ? e[2] : null }));
    const exemptSet = new Set(exemptEntries.map((e) => e.text));

    const scopeMatches = (scope, el) => {
        const sels = scope.split(',').map((x) => x.trim()).filter(Boolean);
        for (let a = el; a; a = a.parent) {
            for (const sel of sels) {
                if (sel.startsWith('#')) { if (a.id === sel.slice(1)) return true; }
                else if (sel.startsWith('.')) { if (a.classes.includes(sel.slice(1))) return true; }
                else if (a.tag === sel.toLowerCase()) return true;
            }
        }
        return false;
    };

    // An element is exempt if ANY entry for its text either carries no scope or
    // carries one this element sits inside.
    const exemptForEl = (text, el) => exemptEntries.some(
        (e) => e.text === text && (e.scope === null || (el && scopeMatches(e.scope, el))));
    const known = new Set([...Object.keys(I18N || {}),
                           ...Object.keys(LABELS && typeof LABELS === 'object' ? LABELS : {})]);

    const htmlSrc = fs.existsSync(p.indexHtml) ? fs.readFileSync(p.indexHtml, 'utf8') : '';
    const { elements, texts } = EXTRACT.scanHtml(htmlSrc);

    const keyedAncestor = (el) => {
        let a = el;
        while (a) { if (a.attrs['data-i18n']) return true; a = a.parent; }
        return false;
    };

    // ── 10. label coverage ───────────────────────────────────────────────
    // WITHOUT THIS a plugin passes at 100% tooltip coverage with every label
    // still hard-coded English.
    const uncovered = [];
    // AN EXEMPTION IS MATCHED BY TEXT, SO IT SILENCES EVERY NODE WITH THAT TEXT.
    // That is the shape of the hole: a string exempt for one element — a choice
    // option, a unit, a product name — also silences a DIFFERENT element that
    // happens to say the same thing and was simply never keyed. O-Detune is the
    // first live collision: `Random` is a wobble_shape option, a unison_dist
    // option, AND a caption that must translate. Its executor's control is the
    // proof — delete `label.random` and its `data-i18n`, leaving bare English,
    // and the gate stays green; the identical deletion on `Spread`, whose text
    // is not exempt, fails assertion 10 by name.
    //
    // Reordering these two guards does NOT fix it, though it is the obvious
    // suggestion and two executors made it independently: both branches are
    // `continue`, so the node is skipped either way and the control still
    // passes. A real fix scopes each exemption to the element that earned it,
    // which changes the I18N_EXEMPT contract across every localized plugin —
    // too big to take mid-rollout, and a decision rather than a repair.
    //
    // What IS free is refusing to let it stay invisible. Each exemption reports
    // how many unkeyed LABEL nodes it silenced, and any string that is BOTH
    // exempt and keyed on the same page is named: that is the ambiguous case,
    // where the gate cannot tell a deliberate exemption from a missed label.
    const silencedUnscoped = new Map();
    for (const t of texts) {
        const el = t.parent;
        if (!el || el.tag === 'script' || el.tag === 'style' || el.tag === 'title') continue;
        if (EXTRACT.classify(t.text).cls !== 'LABEL') continue;
        if (exemptForEl(t.text, el)) {
            // Only an UNSCOPED entry can hide a missed label; a scoped one that
            // matched this element said exactly where it applies.
            const byUnscoped = exemptEntries.some((e) => e.text === t.text && e.scope === null);
            if (byUnscoped && !keyedAncestor(el))
                silencedUnscoped.set(t.text, (silencedUnscoped.get(t.text) || 0) + 1);
            continue;
        }
        if (keyedAncestor(el)) continue;
        uncovered.push(`${t.text.slice(0, 34)} @${el.id ? '#' + el.id : el.tag}`);
    }
    check(uncovered.length === 0,
        `[10] every LABEL text node sits inside a [data-i18n] element, or is I18N_EXEMPT`
        + (uncovered.length ? ` — ${uncovered.length} uncovered: ${uncovered.slice(0, 5).join(' | ')}` : ''));

    // ── 14. AN AMBIGUOUS EXEMPTION MUST BE SCOPED ────────────────────────
    // The precise hazard is not "a string is exempt". It is "a string is exempt
    // AND the same string is keyed on the same page" — that is the state in
    // which the gate cannot tell a deliberate skip from a label somebody forgot,
    // because both look identical to a text match. Anywhere else an unscoped
    // exemption is fine and demanding a scope would be noise.
    const keyedTexts = new Set(texts.filter((t) => t.parent && keyedAncestor(t.parent)).map((t) => t.text));
    const ambiguous = [...silencedUnscoped.keys()].filter((txt) => keyedTexts.has(txt));
    check(ambiguous.length === 0,
        `[14] no UNSCOPED I18N_EXEMPT entry is also keyed on the page — an exemption that `
        + 'collides with a live key cannot distinguish a deliberate skip from a missed label, '
        + 'so it must carry a scope (a tag, .class or #id its occurrences sit inside)'
        + (ambiguous.length ? ` — ${ambiguous.length}: ${ambiguous.slice(0, 5).map((txt) =>
            `"${txt.slice(0, 20)}" silences ${silencedUnscoped.get(txt)} unkeyed node(s)`).join(', ')}` : ''));

    // ── 11. attribute coverage, and ZERO native title= ───────────────────
    const DATASET_FOR = { 'aria-label': 'data-i18n-aria', placeholder: 'data-i18n-placeholder', alt: 'data-i18n-alt' };
    const unkeyedAttrs = [];
    const nativeTitles = [];

    for (const el of elements) {
        if (el.attrs.title) nativeTitles.push(`${el.id ? '#' + el.id : el.tag}[title="${el.attrs.title.value.slice(0, 24)}"]`);

        for (const [attr, dataAttr] of Object.entries(DATASET_FOR)) {
            const rec = el.attrs[attr];
            if (!rec) continue;
            const text = EXTRACT.decodeEntities(rec.value).trim();
            if (EXTRACT.classify(text).cls !== 'LABEL') continue;
            if (exemptForEl(text, el)) continue;
            if (el.attrs[dataAttr]) continue;
            unkeyedAttrs.push(`${el.id ? '#' + el.id : el.tag}@${attr}="${text.slice(0, 24)}"`);
        }
    }

    check(unkeyedAttrs.length === 0,
        `[11] every aria-label / placeholder / alt carrying prose is keyed or exempt`
        + (unkeyedAttrs.length ? ` — ${unkeyedAttrs.length} unkeyed: ${unkeyedAttrs.slice(0, 5).join(' | ')}` : ''));

    // A NATIVE TITLE DOES NOT HAVE TO BE IN THE MARKUP.
    //
    // The loop above walks `elements`, which is scanHtml()'s parse of
    // index.html. `el.title = '...'` in the controller produces a native title
    // on the rendered page that this scan cannot see, so "zero native title=
    // remain" was reachable while any number of them shipped. On O-Wind that
    // was SIXTEEN of the nineteen the page actually rendered — every FX
    // readout carried `valueDisplay.title = 'Double-click to edit'` — and the
    // markup declared only three. O-Lyrica shipped one under the same claim.
    //
    // boot-all-uis' per-plugin `title=` column is the only thing that ever saw
    // these, and it is a REPORT, not a gate: it prints the count and exits 0.
    //
    // So §4 is enforced over both sources at once. Splitting them into two
    // assertions would let a plugin pass the one it happens to trip.
    const TITLE_WRITE = /(?:\.\s*title\s*=|setAttribute\s*\(\s*['"]title['"]\s*,)\s*(['"`])((?:\\.|(?!\1)[\s\S])*)\1/g;
    for (const pm of pageModules) {
        const stripped = EXTRACT.stripJsComments(pm.code);
        for (const m of stripped.matchAll(TITLE_WRITE)) {
            // `el.title = ''` REMOVES a title. Clearing one is the fix, not the
            // defect, so an empty write must not fail the plugin that applied it.
            if (m[2].trim() === '') continue;
            // document.title is the window/tab name, not a tooltip.
            const before = stripped.slice(Math.max(0, m.index - 20), m.index);
            if (/\bdocument\s*$/.test(before)) continue;
            nativeTitles.push(`${pm.label}[title="${m[2].replace(/\s+/g, ' ').slice(0, 24)}"]`);
        }
    }

    check(nativeTitles.length === 0,
        `[11] zero native title= remain, in the MARKUP AND IN THE JS (contract §4 — a `
        + `native title renders a second, untranslated OS tooltip competing with the `
        + `measure-then-pin renderer; a JS-written one is invisible to the markup scan)`
        + (nativeTitles.length ? ` — ${nativeTitles.length}: ${nativeTitles.slice(0, 5).join(' | ')}` : ''));

    // ── 12 / 13 / 15. the controller module ──────────────────────────────
    if (moduleSrc === null) {
        check(false, '[12] a controller module exists to scan');
    } else {
        // NON-VACUITY: a directory read that returned nothing would make every
        // assertion below pass by having nothing to scan — the exact failure
        // §21 of O-Octagon's gate exists to catch in its own module registry.
        check(pageModules.length > 0,
            `[12] there is shipped page JS to scan — ${pageModules.length} module(s): `
            + pageModules.map((m) => m.label).join(', '));

        const jsRows = pageModules.flatMap((m) => EXTRACT.extractJsRows(m));

        // A site converted to setLabel writes no textContent at all, so it
        // produces NO row. What is left is what was never converted.
        const rawProse = jsRows.filter((r) => r.cls === 'LABEL' && !exemptSet.has(r.text));
        const composed = rawProse.filter((r) => r.source === 'js-composed');

        check(rawProse.length === 0,
            `[12] no prose string is written to textContent / innerText outside setLabel`
            + (rawProse.length ? ` — ${rawProse.length}: ${rawProse.slice(0, 5).map((r) => `${r.file}:${r.line} ${JSON.stringify(r.text.slice(0, 24))}`).join(' | ')}` : ''));
        if (composed.length)
            console.log(`  NOTE: [${scope}] [12] ${composed.length} of those are COMPOSED templates — `
                + `they need {token} entries, not flat ones: `
                + composed.slice(0, 4).map((r) => `${r.file}:${r.line}`).join(', '));

        // ── 13. no inflection inside a localized string ──────────────────
        const setLabelCalls = pageModules.flatMap(
            (m) => EXTRACT.readSetLabelCalls(m.code).map((c) => ({ ...c, file: m.label })));
        const withTernary = setLabelCalls.filter((c) => c.conditional);
        const nonLiteralKey = setLabelCalls.filter((c) => c.key === null);

        check(withTernary.length === 0,
            `[13] no ternary or conditional plural suffix inside a setLabel argument (contract §6 — `
            + `French pluralizes zero as singular, so copy is authored around the inflection)`
            + (withTernary.length ? ` — ${withTernary.length} at ${withTernary.map((c) => `${c.file}:${c.line}`).join(', ')}` : ''));
        check(nonLiteralKey.length === 0,
            `[13] every setLabel key is a plain string literal — a computed key cannot be checked, `
            + `and a raw copy string there would ship English`
            + (nonLiteralKey.length ? ` — ${nonLiteralKey.length} at ${nonLiteralKey.map((c) => `${c.file}:${c.line}`).join(', ')}` : ''));

        // ── 15. keys resolve, and nothing is dead ────────────────────────
        const markupKeys = new Set();
        for (const el of elements)
            for (const a of ['data-i18n', 'data-i18n-aria', 'data-i18n-placeholder', 'data-i18n-alt'])
                if (el.attrs[a] && el.attrs[a].value) markupKeys.add(el.attrs[a].value);

        const jsKeys = new Set(setLabelCalls.filter((c) => c.key).map((c) => c.key));

        // A key declared by ASSIGNING dataset.i18n / .i18nAria / .i18nPlaceholder
        // / .i18nAlt is a reference too. An element the controller creates at
        // runtime — a preset-dropdown row, a confirmation strip — cannot carry
        // the attribute in the markup, and setLabel() is not available for an
        // ATTRIBUTE key: it writes textContent. Without this scan the only
        // in-canon way to localize a dynamically created element's accessible
        // name reports as a dead key, which is the gate describing a violation
        // of a rule the code is obeying. Third instance of that shape in this
        // task; see the O-Octagon and O-ReverseDelay notes in the summary.
        //
        // Only a plain string literal counts. `el.dataset.i18nAria =
        // label.dataset.i18n` (canon's labelKnob shape) is a COMPUTED key: it
        // adds nothing here and cannot, which is the same rule assertion 13
        // already applies to a computed setLabel key.
        const datasetKeys = new Set();
        for (const pm of pageModules)
            for (const m of EXTRACT.stripJsComments(pm.code)
                    .matchAll(/\.dataset\.(i18n|i18nAria|i18nPlaceholder|i18nAlt)\s*=\s*(['"])([^'"]+)\2/g))
                datasetKeys.add(m[3]);

        // A key declared inside markup a module INJECTS with innerHTML is a
        // reference too. The three sets above all read the page as AUTHORED:
        // index.html's attributes, a literal setLabel key, a literal
        // `.dataset.i18n* =`. A module that builds a subtree from a template and
        // keys the captions inside it declares its keys in none of them, so all
        // 37 of O-IntonationPad's tuning-panel captions reported DEAD while
        // being read on every language change — the gate describing a violation
        // of a rule the code was obeying, for the fourteenth time in this task.
        //
        // Same discovery path assertion 12 uses to look for UNKEYED copy in the
        // same templates, so the two cannot disagree about what a template says.
        const injectedKeys = new Set();
        for (const pm of pageModules)
            for (const k of EXTRACT.markupKeyRefs(pm.code)) injectedKeys.add(k);

        const referenced = new Set([...markupKeys, ...jsKeys, ...datasetKeys, ...injectedKeys]);

        const dangling = [...referenced].filter((k) => !known.has(k));
        check(dangling.length === 0,
            `[15] every data-i18n / setLabel key exists in LABELS or I18N`
            + (dangling.length ? ` — ${dangling.length} dangling: ${dangling.slice(0, 6).join(', ')}` : ''));

        const labelKeys = Object.keys(LABELS && typeof LABELS === 'object' ? LABELS : {});
        const dead = labelKeys.filter((k) => !referenced.has(k));
        check(dead.length === 0,
            `[15] every LABELS key is referenced by an element or a setLabel call — a dead key is a `
            + `translation nobody sees, drifting silently`
            + (dead.length ? ` — ${dead.length} dead: ${dead.slice(0, 6).join(', ')}` : ''));
    }

    return { keys: keys.length + labelsCount, unreviewed: unreviewed + labelsUnreviewed,
             tipKeys: keys.length, labelKeys: labelsCount, canon: canonVersion };
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
        console.log(`  ${s.name.padEnd(30)} ${String(s.unreviewed).padStart(4)} / ${s.keys} entries unreviewed`
            + `   (${s.tipKeys} tooltip, ${s.labelKeys} label)`);
    const total = summary.reduce((a, s) => a + s.unreviewed, 0);
    console.log(`  ${'TOTAL'.padEnd(30)} ${String(total).padStart(4)}`);
}

// The migration worklist outlived its migration. It stays as a NON-VACUITY
// report rather than being deleted with the canon it tracked: it is the one
// line that would show a plugin silently dropping off canon, and "43 on v2"
// read at a glance is what makes the count above mean something.
console.log('\n-- canon');
{
    const onCanon = summary.filter((s) => s.canon === 'v2').map((s) => s.name);
    const offCanon = summary.filter((s) => s.canon !== 'v2').map((s) => s.name);
    console.log(`  canon v2   ${String(onCanon.length).padStart(3)}  ${onCanon.join(', ')}`);
    if (offCanon.length)
        console.log(`  OFF CANON  ${String(offCanon.length).padStart(3)}  ${offCanon.join(', ')}`
            + '  — each has already failed assertion 6 by name');
}

console.log(`\n${failed === 0 ? 'ALL CHECKS PASS' : `${failed} FAILED`} — ${plugins.length} localized plugin(s)`);
process.exit(failed);
