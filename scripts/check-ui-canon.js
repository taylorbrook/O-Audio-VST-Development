#!/usr/bin/env node
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

    check-ui-canon.js — REPORT-ONLY drift report for hand-copied UI components.

    R3 of 260924-nho-UI-DESIGN-REVIEW.md. Every plugin hand-copies its UI JS;
    without a canon the copies drift (13–22 distinct bodies per component).
    This report turns that into a per-plugin burn-down list. It never fails:
    converging a plugin is a behaviour change, done per plugin via /improve
    with a version bump, never as a sweep.

    ── What is compared ─────────────────────────────────────────────────────
    Each component in scripts/ui-canon.js COMPONENTS is a group of function
    names. The plugin's `function NAME(...) { ... }` declarations are pulled
    out with a real JS lexer and compared with the canon (O-ReverseDelay at the
    commit recorded in UI_CANON_SOURCE — data, never the live file):

      canon       exact after stripping comments and whitespace
      shape       identical code; only string-literal / template text differs
      v:<8-hex>   the code differs; identical variants share the short hash
      —           absent: no definition of any canon or alias name

    ── What is scanned ──────────────────────────────────────────────────────
    The UI root is READ from the plugin's juce_add_binary_data SOURCES via
    scripts/serve-ui.js resolveUiRoot (probing only as its fallback). Under it:
    every *.js / *.mjs (recursively) plus the inline <script> bodies of
    index.html, and every .js under <plugin>/Source/ui/src when present (bundler
    sources). Excluded: js/juce/, any modules/ or node_modules/ directory,
    any basename that is a modules/ file (vendored copies are derived, not
    authored), and bundles (*.bundle.js, *.min.js, a line over 2000 chars, or
    a sourceMappingURL).

    ── Exit code ────────────────────────────────────────────────────────────
    Always 0, in every mode, including internal errors (printed, never
    swallowed). CI runs it as a continue-on-error step of ui-static-gates.

    ── Known limits ─────────────────────────────────────────────────────────
    Regex-versus-division is decided by the previous significant token — a
    heuristic, like every hand lexer. A desynced scan can only produce a false
    VARIANT, never a false canon: canon is exact equality with the canon text,
    which a mangled token stream cannot reproduce by accident.

    Usage:
      node scripts/check-ui-canon.js [--repo-root <dir>] [--plugin <Name>]
      node scripts/check-ui-canon.js --self-test
      node scripts/check-ui-canon.js --emit-canon <rev>

  ==============================================================================
*/

'use strict';

const fs            = require('fs');
const path          = require('path');
const crypto        = require('crypto');
const child_process = require('child_process');

const CANON   = require('./ui-canon.js');
const SERVE   = require('./serve-ui.js');
const EXTRACT = require('./i18n-extract.js');

const { COMPONENTS, UI_CANON_SOURCE } = CANON;

// ═══════════════════════════════════════════════════════════════════ lexer ══

function prepSource(src) {
    let s = String(src);
    if (s.charCodeAt(0) === 0xFEFF) s = s.slice(1);
    return s.replace(/\r\n?/g, '\n');
}

function isWordChar(c) {
    if (c === undefined || c === '') return false;
    const k = c.charCodeAt(0);
    return (k >= 48 && k <= 57) || (k >= 65 && k <= 90) || (k >= 97 && k <= 122)
        || k === 95 || k === 36 || k >= 0x80;
}

const REGEX_AFTER_PUNCT = new Set('(,=:[!&|?{};+-*%~^<>'.split(''));
const REGEX_AFTER_WORD  = new Set(['return', 'typeof', 'instanceof', 'in', 'of', 'new', 'delete',
                                   'void', 'throw', 'case', 'do', 'else', 'yield', 'await']);

function regexAllowed(prev) {
    if (!prev) return true;
    if (prev.t === 'punct') return REGEX_AFTER_PUNCT.has(prev.v);
    if (prev.t === 'word')  return REGEX_AFTER_WORD.has(prev.v);
    if (prev.t === 'tq')    return prev.x === '${';     // start of an interpolation
    return false;                                        // str / regex / closed template
}

// Tokens {t, v, s, e}; t ∈ ws | comment | word | punct | str | tq | regex.
// tq pieces also carry x: the closing delimiter ('${', '`' or '' if unterminated).
// A template's ${ … } is lexed as CODE (with its own nesting), which is the trap
// i18n-extract.js paid for once (O-Prism, O-Lyrica). Never throws; every
// iteration advances at least one character; unterminated constructs run to EOF.
function lexJs(input) {
    const src = prepSource(input);
    const n = src.length;
    const toks = [];
    const stack = [];
    let prev = null;
    let i = 0;

    const push = (t, s, e, extra) => {
        e = Math.min(Math.max(e, s + 1), n);
        const tok = { t, v: src.slice(s, e), s, e };
        if (extra) Object.assign(tok, extra);
        toks.push(tok);
        if (t !== 'ws' && t !== 'comment') prev = tok;
        return e;
    };

    // A quasi piece from `start` (a backtick or the } closing an interpolation)
    // through the next ${ or closing backtick, inclusive.
    const lexQuasi = (start) => {
        let j = start + 1;
        while (j < n) {
            const c = src[j];
            if (c === '\\') { j += 2; continue; }
            if (c === '`') return push('tq', start, j + 1, { x: '`' });
            if (c === '$' && src[j + 1] === '{') { stack.push('tmpl'); return push('tq', start, j + 2, { x: '${' }); }
            ++j;
        }
        return push('tq', start, n, { x: '' });
    };

    while (i < n) {
        const c = src[i];
        const d = src[i + 1];

        if (c === ' ' || c === '\t' || c === '\n' || c === '\r' || c === '\f' || c === '\v'
            || c === '\u00a0' || c === '\ufeff' || c === '\u2028' || c === '\u2029') {
            let j = i + 1;
            while (j < n && /[ \t\n\r\f\v\u00a0\ufeff\u2028\u2029]/.test(src[j])) ++j;
            i = push('ws', i, j);
            continue;
        }

        if (c === '/' && d === '/') {
            let j = src.indexOf('\n', i);
            if (j < 0) j = n;
            i = push('comment', i, j);
            continue;
        }

        if (c === '/' && d === '*') {
            const j = src.indexOf('*/', i + 2);
            i = push('comment', i, j < 0 ? n : j + 2);
            continue;
        }

        if (c === '`') { i = lexQuasi(i); continue; }

        if (c === '}') {
            const top = stack.pop();
            if (top === 'tmpl') { i = lexQuasi(i); continue; }
            i = push('punct', i, i + 1);
            continue;
        }

        if (c === '{') { stack.push('brace'); i = push('punct', i, i + 1); continue; }

        if (c === '\'' || c === '"') {
            let j = i + 1;
            while (j < n) {
                const ch = src[j];
                if (ch === '\\') { j += 2; continue; }
                if (ch === c) { ++j; break; }
                if (ch === '\n') break;
                ++j;
            }
            i = push('str', i, j);
            continue;
        }

        if (c === '/') {
            if (regexAllowed(prev)) {
                let j = i + 1, inClass = false, ok = false;
                while (j < n) {
                    const ch = src[j];
                    if (ch === '\\') { j += 2; continue; }
                    if (ch === '\n') break;
                    if (inClass) { if (ch === ']') inClass = false; }
                    else if (ch === '[') inClass = true;
                    else if (ch === '/') { ++j; ok = true; break; }
                    ++j;
                }
                if (ok) {
                    while (j < n && /[A-Za-z]/.test(src[j])) ++j;
                    i = push('regex', i, j);
                    continue;
                }
            }
            i = push('punct', i, i + 1);
            continue;
        }

        if (isWordChar(c)) {
            let j = i + 1;
            while (j < n && isWordChar(src[j])) ++j;
            i = push('word', i, j);
            continue;
        }

        i = push('punct', i, i + 1);
    }

    return toks;
}

const isSig = (t) => t.t !== 'ws' && t.t !== 'comment';

// Every `[async] function [*] NAME (…) { … }` declaration. Braces are matched on
// punct tokens only — str/tq/regex/comment tokens hide theirs. The scan resumes
// just after NAME, so nested declarations are found too.
function extractFunctions(input, tokens, warnings) {
    const src = prepSource(input);
    const toks = tokens || lexJs(src);
    const sigIdx = [];
    for (let k = 0; k < toks.length; ++k) if (isSig(toks[k])) sigIdx.push(k);
    const S = (m) => (m >= 0 && m < sigIdx.length ? toks[sigIdx[m]] : null);
    const out = [];

    for (let m = 0; m < sigIdx.length; ++m) {
        const fn = S(m);
        if (fn.t !== 'word' || fn.v !== 'function') continue;

        let q = m + 1;
        if (S(q) && S(q).t === 'punct' && S(q).v === '*') ++q;
        const nameTok = S(q);
        if (!nameTok || nameTok.t !== 'word') continue;
        const open = S(q + 1);
        if (!open || open.t !== 'punct' || open.v !== '(') continue;

        let depth = 0, closeParen = -1;
        for (let r = q + 1; r < sigIdx.length; ++r) {
            const t = S(r);
            if (t.t !== 'punct') continue;
            if (t.v === '(') ++depth;
            else if (t.v === ')' && --depth === 0) { closeParen = r; break; }
        }
        if (closeParen < 0) { if (warnings) warnings.push(`unmatched parameter list for ${nameTok.v}`); continue; }

        const brace = S(closeParen + 1);
        if (!brace || brace.t !== 'punct' || brace.v !== '{') continue;

        depth = 0;
        let closeBrace = -1;
        for (let r = closeParen + 1; r < sigIdx.length; ++r) {
            const t = S(r);
            if (t.t !== 'punct') continue;
            if (t.v === '{') ++depth;
            else if (t.v === '}' && --depth === 0) { closeBrace = r; break; }
        }
        if (closeBrace < 0) { if (warnings) warnings.push(`unmatched body for ${nameTok.v}`); continue; }

        const asyncTok = S(m - 1);
        const firstK = asyncTok && asyncTok.t === 'word' && asyncTok.v === 'async' ? sigIdx[m - 1] : sigIdx[m];
        const lastK = sigIdx[closeBrace];
        const start = toks[firstK].s;
        const end = toks[lastK].e;

        out.push({ name: nameTok.v, text: src.slice(start, end), tokens: toks.slice(firstK, lastK + 1), start });
    }

    return out;
}

// ═══════════════════════════════════════════════════════════ normalisation ══

function joinNorm(parts) {
    let out = '';
    for (const v of parts) {
        if (!v) continue;
        const a = out[out.length - 1], b = v[0];
        if (out && ((isWordChar(a) && isWordChar(b)) || ('+-'.includes(a) && '+-'.includes(b)))) out += ' ';
        out += v;
    }
    return out;
}

function normExact(tokens) {
    return joinNorm(tokens.filter(isSig).map((t) => t.v));
}

function normShape(tokens) {
    return joinNorm(tokens.filter(isSig).map((t) => {
        if (t.t === 'str') return '"S"';
        if (t.t === 'tq') return t.v[0] + 'S' + (t.x || '');
        return t.v;
    }));
}

const sha256 = (s) => crypto.createHash('sha256').update(s).digest('hex');
const short = (h) => h.slice(0, 8);

// ═════════════════════════════════════════════════════════════════ sources ══

function walkFiles(dir, skipDir) {
    const out = [];
    const rec = (d) => {
        let ents;
        try { ents = fs.readdirSync(d, { withFileTypes: true }); } catch { return; }
        for (const e of ents) {
            const p = path.join(d, e.name);
            if (e.isDirectory()) { if (!skipDir || !skipDir(e.name, p)) rec(p); }
            else if (e.isFile()) out.push(p);
        }
    };
    rec(dir);
    return out;
}

function moduleJsBasenames(repoRoot) {
    const set = new Set();
    for (const f of walkFiles(path.join(repoRoot, 'modules'), (n) => n === 'node_modules'))
        if (/\.js$/.test(f)) set.add(path.basename(f));
    return set;
}

const toPosix = (p) => p.split(path.sep).join('/');

function isBundle(file, text) {
    const b = path.basename(file);
    if (/\.bundle\.js$|\.min\.js$/.test(b)) return true;
    if (text.includes('sourceMappingURL=')) return true;
    return text.split('\n').some((l) => l.length > 2000);
}

function collectJsUnder(dir, labelBase, moduleBasenames, out) {
    if (!fs.existsSync(dir)) return;
    const files = walkFiles(dir, (name) => name === 'modules' || name === 'node_modules');
    for (const f of files) {
        if (!/\.(m?js)$/.test(f)) continue;
        const rel = toPosix(path.relative(dir, f));
        if (('/' + rel).includes('/js/juce/')) continue;
        if (moduleBasenames.has(path.basename(f))) continue;
        const text = fs.readFileSync(f, 'utf8');
        if (isBundle(f, text)) continue;
        out.push({ label: `${labelBase}/${rel}`, code: text });
    }
}

// All authored JS for one plugin, sorted by label for determinism.
function pluginSources(name, repoRoot, moduleBasenames) {
    const ui = SERVE.resolveUiRoot(name, repoRoot);
    if (!ui) return { uiRoot: null, sources: [] };

    const sources = [];
    const indexHtml = path.join(ui.uiRoot, 'index.html');
    if (fs.existsSync(indexHtml)) {
        const html = fs.readFileSync(indexHtml, 'utf8');
        let k = 0;
        for (const el of EXTRACT.scanHtml(html).elements) {
            if (el.tag !== 'script' || !el.raw || el.raw.trim().length === 0) continue;
            const type = el.attrs.type ? el.attrs.type.value : '';
            if (type && type !== 'module' && type !== 'text/javascript') continue;
            sources.push({ label: `${ui.label}/index.html <script #${++k}>`, code: el.raw });
        }
    }

    collectJsUnder(ui.uiRoot, ui.label, moduleBasenames, sources);

    const srcDir = path.join(SERVE.pluginRoot(name, repoRoot), 'Source', 'ui', 'src');
    collectJsUnder(srcDir, 'Source/ui/src', moduleBasenames, sources);

    sources.sort((a, b) => (a.label < b.label ? -1 : a.label > b.label ? 1 : 0));
    return { uiRoot: ui.label, sources };
}

// ══════════════════════════════════════════════════════════ classification ══

function buildCanon() {
    const text = CANON.readCanonText();
    const defs = extractFunctions(text);
    const out = {};
    const broken = [];
    for (const c of COMPONENTS) {
        const parts = [];
        let ok = true;
        for (const name of c.canon) {
            const hits = defs.filter((d) => d.name === name);
            if (hits.length !== 1) { ok = false; broken.push(`CANON BROKEN: ${c.id} ${name} found ${hits.length} times`); continue; }
            parts.push(hits[0]);
        }
        out[c.id] = ok ? {
            ok: true,
            exact: parts.map((d) => normExact(d.tokens)).join('\n'),
            shape: parts.map((d) => normShape(d.tokens)).join('\n'),
        } : { ok: false };
        if (ok) out[c.id].hash = sha256(out[c.id].exact);
    }
    return { components: out, broken };
}

// Defs of every name, per source order.
function defsByName(sources) {
    const map = new Map();
    const warnings = [];
    for (const s of sources) {
        const toks = lexJs(s.code);
        for (const d of extractFunctions(s.code, toks, warnings)) {
            if (!map.has(d.name)) map.set(d.name, []);
            map.get(d.name).push({ ...d, source: s.label });
        }
    }
    return { map, warnings };
}

function classifyComponent(comp, canonComp, defs) {
    const names = [...comp.canon, ...comp.aliases];
    const found = [];
    for (const nm of names) for (const d of defs.get(nm) || []) found.push(d);

    if (!canonComp.ok) return { status: 'CANON?', hash: null, found: found.map((d) => d.name) };

    const canonDefs = comp.canon.map((nm) => defs.get(nm) || []);
    if (canonDefs.every((a) => a.length === 1)) {
        const ex = canonDefs.map((a) => normExact(a[0].tokens)).join('\n');
        const sh = canonDefs.map((a) => normShape(a[0].tokens)).join('\n');
        const foundNames = comp.canon.slice();
        if (ex === canonComp.exact) return { status: 'canon', hash: sha256(ex), found: foundNames };
        if (sh === canonComp.shape) return { status: 'shape', hash: sha256(ex), shapeHash: sha256(sh), found: foundNames };
        return { status: 'variant', hash: sha256(ex), shapeHash: sha256(sh), found: foundNames };
    }

    if (found.length) {
        const ex = found.map((d) => normExact(d.tokens)).join('\n');
        return { status: 'variant', hash: sha256(ex), found: found.map((d) => d.name) };
    }

    return { status: 'absent', hash: null, found: [] };
}

function cellText(r) {
    if (!r) return 'ERR';
    if (r.status === 'variant') return 'v:' + short(r.hash);
    if (r.status === 'absent') return '—';
    return r.status;
}

// ════════════════════════════════════════════════════════════════════ scan ══

function scan(repoRoot, onlyPlugin) {
    const errors = [];
    const canon = buildCanon();
    const all = SERVE.listPlugins(repoRoot);
    const plugins = onlyPlugin ? all.filter((p) => p === onlyPlugin) : all;
    const moduleBasenames = moduleJsBasenames(repoRoot);
    const rows = [];

    for (const name of plugins) {
        const row = { name, uiRoot: null, sources: [], components: {}, notes: [], error: null, defined: new Set() };
        try {
            const { uiRoot, sources } = pluginSources(name, repoRoot, moduleBasenames);
            row.uiRoot = uiRoot;
            row.sources = sources.map((s) => s.label);
            if (!uiRoot) row.notes.push('(no UI root)');
            else if (!sources.length) row.notes.push('no JS sources found');

            const { map, warnings } = defsByName(sources);
            for (const w of warnings) row.notes.push('parse warning: ' + w);
            for (const nm of map.keys()) row.defined.add(nm);

            if (uiRoot) for (const c of COMPONENTS)
                row.components[c.id] = classifyComponent(c, canon.components[c.id], map);
        } catch (e) {
            row.error = e && e.message ? e.message : String(e);
            errors.push({ scope: name, message: row.error });
        }
        rows.push(row);
    }

    return { canon, rows, errors, allCount: all.length, onlyPlugin };
}

// ══════════════════════════════════════════════════════════════════ report ══

function componentStats(comp, rows) {
    const counts = { canon: 0, shape: 0, variant: 0, distinct: 0, absent: 0, error: 0 };
    const groups = new Map();
    const distinct = new Set();

    for (const r of rows) {
        if (!r.uiRoot && !r.error) continue;
        const c = r.error ? null : r.components[comp.id];
        let key, status;
        if (!c) { counts.error++; continue; }
        status = c.status;
        if (status === 'canon')        { counts.canon++;  key = 'canon'; }
        else if (status === 'shape')   { counts.shape++;  key = 'shape:' + c.hash; }
        else if (status === 'variant') { counts.variant++; distinct.add(c.hash); key = 'v:' + c.hash; }
        else if (status === 'absent')  { counts.absent++; key = 'absent'; }
        else { counts.error++; continue; }
        if (!groups.has(key)) groups.set(key, { status, hash: c.hash, names: c.found.join('+'), plugins: [] });
        groups.get(key).plugins.push(r.name);
    }
    counts.distinct = distinct.size;

    const definedBy = {};
    for (const nm of [...comp.canon, ...comp.aliases])
        definedBy[nm] = rows.filter((r) => r.defined.has(nm)).map((r) => r.name);

    const order = { canon: 0, shape: 1, variant: 2, absent: 4 };
    const gl = [...groups.values()].sort((a, b) =>
        (b.plugins.length - a.plugins.length) || (order[a.status] - order[b.status]) || String(a.hash).localeCompare(String(b.hash)));

    return { counts, definedBy, groups: gl };
}

function countsLine(id, c) {
    return `${id}: canon ${c.canon} · shape ${c.shape} · variant ${c.variant} (distinct ${c.distinct}) · absent ${c.absent} · error ${c.error}`;
}

function printReport(res, repoRoot) {
    const L = (s = '') => console.log(s);
    const { canon, rows } = res;

    L('check-ui-canon — REPORT-ONLY (exit 0 always)');
    L(`canon: ${UI_CANON_SOURCE.plugin} v${UI_CANON_SOURCE.version} @ ${UI_CANON_SOURCE.commit.slice(0, 8)} ${UI_CANON_SOURCE.file}`);
    for (const c of COMPONENTS) {
        const cc = canon.components[c.id];
        L(`  canon ${c.id}: ${cc.ok ? short(cc.hash) : 'BROKEN'}  [${c.canon.join('+')}]`);
    }
    for (const b of canon.broken) L(b);
    L(`repo root: ${repoRoot}`);
    L(`plugins scanned: ${rows.length} (UI root resolved: ${rows.filter((r) => r.uiRoot).length})`);
    if (res.onlyPlugin && !rows.length) L(`no plugin named '${res.onlyPlugin}' (plugins/<Name>/CMakeLists.txt)`);
    L();

    const W = Math.max(24, ...rows.map((r) => r.name.length + 2));
    const CW = Math.max(12, ...COMPONENTS.map((c) => c.id.length + 2));
    L('plugin'.padEnd(W) + COMPONENTS.map((c) => c.id.padEnd(CW)).join('').trimEnd());
    for (const r of rows) {
        let line = r.name.padEnd(W);
        if (r.error) line += COMPONENTS.map(() => 'ERR'.padEnd(CW)).join('');
        else if (!r.uiRoot) line += '(no UI root)';
        else line += COMPONENTS.map((c) => cellText(r.components[c.id]).padEnd(CW)).join('');
        L(line.trimEnd());
    }
    for (const r of rows) for (const n of r.notes) if (n !== '(no UI root)') L(`  note ${r.name}: ${n}`);

    for (const c of COMPONENTS) {
        const st = componentStats(c, rows);
        L();
        L(`── ${c.id} (${c.label})`);
        L(countsLine(c.id, st.counts));
        for (const [nm, list] of Object.entries(st.definedBy)) L(`    ${nm}: defined in ${list.length} plugins`);
        for (const g of st.groups) {
            const n = g.plugins.length;
            if (g.status === 'canon')        L(`  canon ×${n}  ${g.plugins.join(', ')}`);
            else if (g.status === 'shape')   L(`  shape ×${n}  [${g.names}]  ${g.plugins.join(', ')}`);
            else if (g.status === 'variant') L(`  v:${short(g.hash)} ×${n}  [${g.names}]  ${g.plugins.join(', ')}`);
            else                             L(`  absent ×${n}  ${g.plugins.join(', ')}`);
        }
    }

    if (res.errors.length) {
        L();
        L('── errors');
        for (const e of res.errors) L(`  ${e.scope}: ${e.message}`);
    }
}

// ═════════════════════════════════════════════════════════════ emit canon ══

const REV_RE = /^[A-Za-z0-9][A-Za-z0-9._/~^-]{0,99}$/;

function emitCanonText(rev) {
    if (!REV_RE.test(rev)) throw new Error(`invalid rev '${rev}'`);
    const git = (args) => child_process.execFileSync('git', args,
        { cwd: SERVE.REPO_ROOT, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024, stdio: ['ignore', 'pipe', 'pipe'] });

    const text = git(['show', rev + ':' + UI_CANON_SOURCE.file]);
    const defs = extractFunctions(text);
    const names = [];
    for (const c of COMPONENTS) for (const nm of c.canon) if (!names.includes(nm)) names.push(nm);

    const problems = [];
    const blocks = [];
    for (const nm of names) {
        const hits = defs.filter((d) => d.name === nm);
        if (hits.length !== 1) { problems.push(`${nm} defined ${hits.length} times at ${rev}`); continue; }
        blocks.push(hits[0].text.split('\n').map((l) => (l.length ? '//| ' + l : '//|')).join('\n'));
    }

    const out = ['// ' + CANON.CANON_BEGIN, blocks.join('\n//|\n'), '// ' + CANON.CANON_END].join('\n') + '\n';

    let hint = '';
    try {
        const sha = git(['rev-parse', rev]).trim();
        const cm = git(['show', `${rev}:plugins/${UI_CANON_SOURCE.plugin}/CMakeLists.txt`]);
        const v = cm.match(/^\s*VERSION\s+"?([0-9][0-9.]*)"?/m);
        hint = `rev ${sha}  VERSION ${v ? v[1] : '?'}`;
    } catch (e) { hint = 'could not resolve rev metadata: ' + e.message; }

    return { text: out, problems, hint };
}

// ═══════════════════════════════════════════════════════════════ self-test ══

function selfTest() {
    const results = [];
    const check = (id, fn) => {
        try {
            const r = fn();
            if (r === 'SKIP') results.push({ id, status: 'skip' });
            else results.push({ id, status: r ? 'pass' : 'fail' });
        } catch (e) {
            results.push({ id, status: 'fail', msg: e.message });
        }
    };

    // L1 — the trap fixture. Every construct here hides a brace from a lexer
    // that respects JS, and exposes it to one that does not.
    const L1 = [
        'function trap(s, n) {',
        "  const a = '}';",
        '  const b = "{";',
        "  const t = `a${ {b: 1}.b }c${ `x${'}'}` }`;",
        '  const r = /[}{]/g;',
        '  const d = n / 12;',
        '  const u = `${Math.floor(n / 12) - 1}`;',
        '  // } in a line comment',
        '  /* } in a block comment */',
        '  if (r.test(s)) { return /}/.test(s); }',
        '  return a + b + t + d + u;',
        '}/*MARK*/',
        'function after() { return 1; }',
        '',
    ].join('\n');

    check('L1', () => {
        const defs = extractFunctions(L1);
        const trap = defs.find((d) => d.name === 'trap');
        const after = defs.find((d) => d.name === 'after');
        const mark = L1.indexOf('/*MARK*/');
        if (!trap || trap.start + trap.text.length !== mark) return false;
        if (!after || !after.text.endsWith('{ return 1; }')) return false;

        // NEGATIVE CONTROL: a naive brace counter, blind to strings, comments
        // and regex, must end `trap` somewhere else — proof the fixture traps.
        const open = L1.indexOf('{');
        let depth = 0, naiveEnd = -1;
        for (let i = open; i < L1.length; ++i) {
            if (L1[i] === '{') ++depth;
            else if (L1[i] === '}' && --depth === 0) { naiveEnd = i + 1; break; }
        }
        return naiveEnd !== mark;
    });

    check('L2', () => {
        const src = 'async function outer() { function inner() {} }';
        const defs = extractFunctions(src);
        const outer = defs.find((d) => d.name === 'outer');
        return !!outer && outer.text.startsWith('async') && outer.text === src
            && defs.some((d) => d.name === 'inner');
    });

    const failed = results.filter((r) => r.status === 'fail');
    const skipped = results.filter((r) => r.status === 'skip').length;
    const ran = results.length - skipped;
    if (failed.length) {
        console.log(`SELF-TEST FAIL ${failed.map((r) => r.id).join(' ')}`);
        for (const f of failed) if (f.msg) console.log(`  ${f.id}: ${f.msg}`);
    } else {
        console.log(`SELF-TEST PASS ${ran}/${ran} (skipped ${skipped})`);
    }
}

// ════════════════════════════════════════════════════════════════════ main ══

function main(argv) {
    const val = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };

    if (argv.includes('--self-test')) { selfTest(); return; }

    if (argv.includes('--emit-canon')) {
        const rev = val('--emit-canon');
        if (!rev || !REV_RE.test(rev)) {
            console.error(`check-ui-canon: --emit-canon needs a revision matching ${REV_RE} (got '${rev || ''}')`);
            return;
        }
        const r = emitCanonText(rev);
        for (const p of r.problems) console.error('check-ui-canon: ERROR ' + p);
        process.stdout.write(r.text);
        console.error(r.hint);
        return;
    }

    const repoRoot = path.resolve(val('--repo-root') || SERVE.REPO_ROOT);
    if (!fs.existsSync(repoRoot)) {
        console.log(`check-ui-canon: repo root not found: ${repoRoot}`);
        return;
    }

    const res = scan(repoRoot, val('--plugin'));
    printReport(res, repoRoot);
}

if (require.main === module) {
    try {
        main(process.argv.slice(2));
    } catch (e) {
        console.log(`check-ui-canon: internal error — ${e && e.message ? e.message : e}`);
    }
    // exitCode, not process.exit(): stdout to a pipe is asynchronous on macOS,
    // and an explicit exit would truncate a long report mid-write.
    process.exitCode = 0;
}

module.exports = { lexJs, extractFunctions, normExact, normShape, prepSource };
