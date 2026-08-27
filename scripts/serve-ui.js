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

    serve-ui.js — assemble and serve ANY plugin's WebView page headless.

    Factored from the three committed clamp gates (O-ReverseDelay, O-Bitrot,
    O-Tapestop), which each carry their own hand-written buildRoot()/serve()
    pair. Those three keep theirs: this module is additive, and a gate that
    passes today must not start depending on a file written this afternoon.

    Why it exists: only 5 of 43 plugins have a bridge stub, so 38 cannot be
    rendered at all — and a French-label overflow gate that cannot load the page
    is not a gate.

    ── What "the served tree" means ────────────────────────────────────────────

    The WebView does NOT see the source directory. It sees whatever
    PluginEditor::getResource() hands back, and that is assembled from a
    juce_add_binary_data SOURCES block that can pull files from OUTSIDE the UI
    root — 8 plugins embed shared module JS from ${CMAKE_SOURCE_DIR}/modules/.
    A harness that copies only the UI root serves a 404 for those, which
    presents as a missing panel and nothing else.

    So the tree is assembled the way the plugin assembles it:

      1. copy the UI root byte-identical;
      2. for every juce_add_binary_data SOURCE, compute its BinaryData symbol
         with JUCE's own mangling rule, look that symbol up in the url == "..."
         branches of PluginEditor.cpp, and place the file at THAT url;
      3. overlay the bridge stub at js/juce/index.js.

    Step 2 is what makes /modules/preset-manager.js and /js/tuning-panel.js
    resolve on the 8 plugins that embed them from the module tree.

    ── The UI root is READ, never guessed ──────────────────────────────────────

    Two plugins (O-MicrotonalSampler, O-Orbit) carry BOTH a Source/ui/public and
    a Resources/ui. Only one of them is embedded; the other is a staging leftover
    that renders a stale page. A fixed probe order picks right for those two by
    luck and would pick wrong for the next one, so the root is derived from the
    CMake SOURCES block — the same list the build embeds — and probe order is
    only the fallback when no block parses.

    ── Port 0, always ──────────────────────────────────────────────────────────

    A fixed port silently serves a concurrent session's files
    (pattern_ui_test_server_port_clash_serves_other_session). listen(0) and
    report back what the OS gave.

    Usage (as a module):
        const { buildRoot, serve, readEditorSize } = require('./serve-ui.js');

    Usage (as a CLI, for inspection):
        node scripts/serve-ui.js --plugin O-Tapestop --report
        node scripts/serve-ui.js --plugin O-Tapestop --hold    # serve until ^C

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const http = require('http');

const REPO_ROOT = path.resolve(__dirname, '..');

// ───────────────────────────────────────────────────────────── discovery ──

function pluginsDir(repoRoot = REPO_ROOT) { return path.join(repoRoot, 'plugins'); }

function listPlugins(repoRoot = REPO_ROOT) {
    const dir = pluginsDir(repoRoot);
    if (!fs.existsSync(dir)) return [];
    return fs.readdirSync(dir).sort().filter((n) => {
        const p = path.join(dir, n);
        return fs.statSync(p).isDirectory() && fs.existsSync(path.join(p, 'CMakeLists.txt'));
    });
}

function pluginRoot(name, repoRoot = REPO_ROOT) { return path.join(pluginsDir(repoRoot), name); }

// ─────────────────────────────────────────────────────── CMake scanning ──

// Comment-stripped, so a SOURCES entry that lives inside a `#` comment (there
// are several — the CMakeLists in this repo document their own embeds heavily)
// is never mistaken for a real one.
function readCmake(name, repoRoot = REPO_ROOT) {
    const f = path.join(pluginRoot(name, repoRoot), 'CMakeLists.txt');
    if (!fs.existsSync(f)) return '';
    return fs.readFileSync(f, 'utf8').replace(/(^|\n)[ \t]*#[^\n]*/g, '$1');
}

// JUCE's build_tools::makeBinaryDataIdentifierName: ' ' and '.' become '_',
// every other non-identifier character is REMOVED (hyphens vanish rather than
// converting — critical_binary_data_strips_hyphens), and a leading digit is
// prefixed. `preset-manager.js` -> `presetmanager_js`.
function binaryDataSymbol(fileName) {
    let s = fileName.replace(/ /g, '_').replace(/\./g, '_').replace(/[^A-Za-z0-9_]/g, '');
    if (/^[0-9]/.test(s)) s = '_' + s;
    return s;
}

function expandCmakePath(raw, name, repoRoot = REPO_ROOT) {
    let p = raw.trim().replace(/^["']|["']$/g, '');
    p = p.replace(/\$\{CMAKE_SOURCE_DIR\}/g, repoRoot)
         .replace(/\$\{CMAKE_CURRENT_SOURCE_DIR\}/g, pluginRoot(name, repoRoot))
         .replace(/\$\{PROJECT_SOURCE_DIR\}/g, repoRoot);
    if (/\$\{/.test(p)) return null;              // an unresolved variable is not a path
    return path.isAbsolute(p) ? p : path.join(pluginRoot(name, repoRoot), p);
}

// Every juce_add_binary_data(...) block in the plugin's CMakeLists, flattened to
// one list of {raw, abs, symbol, exists}. Balanced-paren scan rather than a
// /^\)/ line anchor: several blocks in this repo close on an indented line.
function binaryDataSources(name, repoRoot = REPO_ROOT) {
    const cmake = readCmake(name, repoRoot);
    const out = [];

    let at = 0;
    for (;;) {
        const start = cmake.indexOf('juce_add_binary_data', at);
        if (start < 0) break;

        const open = cmake.indexOf('(', start);
        if (open < 0) break;

        let depth = 0, end = -1;
        for (let i = open; i < cmake.length; ++i) {
            if (cmake[i] === '(') ++depth;
            else if (cmake[i] === ')') { if (--depth === 0) { end = i; break; } }
        }
        if (end < 0) break;

        const block = cmake.slice(open + 1, end);
        at = end + 1;

        const sourcesAt = block.search(/\bSOURCES\b/);
        if (sourcesAt < 0) continue;

        for (const tok of block.slice(sourcesAt + 'SOURCES'.length).split(/\s+/)) {
            if (!tok || !/\.[A-Za-z0-9]+$/.test(tok)) continue;
            const abs = expandCmakePath(tok, name, repoRoot);
            if (!abs) continue;
            out.push({
                raw: tok,
                abs,
                symbol: binaryDataSymbol(path.basename(abs)),
                exists: fs.existsSync(abs),
            });
        }
    }

    return out;
}

// ───────────────────────────────────────────────────────────── UI root ──

const UI_ROOT_CANDIDATES = [
    path.join('Resources', 'ui'),
    path.join('Source', 'ui', 'public'),
];

function resolveUiRoot(name, repoRoot = REPO_ROOT) {
    const root = pluginRoot(name, repoRoot);

    // Preferred: whichever candidate the EMBEDDED sources actually live under.
    const votes = new Map();
    for (const s of binaryDataSources(name, repoRoot)) {
        if (!s.abs.startsWith(root + path.sep)) continue;
        const rel = path.relative(root, s.abs);
        for (const cand of UI_ROOT_CANDIDATES)
            if (rel.startsWith(cand + path.sep)) votes.set(cand, (votes.get(cand) || 0) + 1);
    }

    let best = null, bestN = 0;
    for (const [cand, n] of votes) if (n > bestN) { best = cand; bestN = n; }

    if (best) return { uiRoot: path.join(root, best), label: best.split(path.sep).join('/'), from: 'cmake' };

    for (const cand of UI_ROOT_CANDIDATES) {
        if (fs.existsSync(path.join(root, cand, 'index.html')))
            return { uiRoot: path.join(root, cand), label: cand.split(path.sep).join('/'), from: 'probe' };
    }

    return null;
}

// ─────────────────────────────────────────────── editor size (fact 7) ──

// setSize(W, H) out of PluginEditor.cpp. NEVER a mirrored literal in a fixture:
// a constant that mirrors a plugin constant has drifted silently in this repo
// twice (pattern_test_fixture_mirrors_drift_silently).
//
// Returns the LAST setSize in the file when several appear (O-TextureForge has
// two identical ones) and null when none parses — a caller that cannot learn the
// shipping frame must say so rather than measure 1280x720.
function readEditorSize(name, repoRoot = REPO_ROOT) {
    const f = path.join(pluginRoot(name, repoRoot), 'Source', 'PluginEditor.cpp');
    if (!fs.existsSync(f)) return null;

    const src = fs.readFileSync(f, 'utf8').replace(/\/\/[^\n]*/g, '').replace(/\/\*[\s\S]*?\*\//g, '');
    const hits = [...src.matchAll(/setSize\s*\(\s*(\d+)\s*,\s*(\d+)\s*\)/g)];
    if (hits.length === 0) return null;

    const m = hits[hits.length - 1];
    return { w: parseInt(m[1], 10), h: parseInt(m[2], 10), occurrences: hits.length };
}

// ───────────────────────────────────────────── url <-> BinaryData symbol ──

// getResource() is a ladder of `if (url == "/x") return ...BinaryData::x_y`.
// Pairing the two is what lets a file embedded from outside the UI root be
// placed at the path the page actually requests.
function urlSymbolMap(name, repoRoot = REPO_ROOT) {
    const f = path.join(pluginRoot(name, repoRoot), 'Source', 'PluginEditor.cpp');
    const map = new Map();
    if (!fs.existsSync(f)) return map;

    const src = fs.readFileSync(f, 'utf8');

    for (const m of src.matchAll(/url\s*==\s*"([^"]+)"/g)) {
        const url = m[1];
        // A window rather than a line: the symbol is typically 1-3 lines below,
        // inside a makeVector(...) or makeBinaryResource(...) call.
        const window = src.slice(m.index, m.index + 400);
        const nextUrl = window.slice(1).search(/url\s*==\s*"/);
        const scoped = nextUrl >= 0 ? window.slice(0, nextUrl + 1) : window;

        for (const s of scoped.matchAll(/(?:[A-Za-z_]\w*)?BinaryData\s*::\s*(\w+)/g)) {
            const sym = s[1];
            if (/Size$/.test(sym)) continue;
            if (!map.has(sym)) map.set(sym, url.startsWith('/') ? url : '/' + url);
            break;
        }
    }

    return map;
}


// ────────────────────────────────────────────── stub seeding (fixtures) ──

// A param-dump TSV, when one exists, is the only NON-GUESSED source of a
// plugin's real parameter ranges: a regex over createParameterLayout() provably
// undercounts in this repo (O-Prism finds 0 usable IDs statically and 173 at
// runtime), so the choice is between a runtime dump and admitting ignorance.
//
// Nothing in the tree has one yet — generating them is a build step
// (-DOUARICON_BUILD_TESTS=ON) that Stage E deliberately does not take. This
// parser is exercised against a fixture rather than left as dead code, and the
// mode it selected is reported on window.__stubReport.rangesFrom so a caller
// can never mistake neutral fixtures for measured ranges.
function parseParamDumpTsv(text) {
    const sliders = {};
    const toggles = {};
    let rows = 0;

    let cols = null;
    for (const raw of text.split('\n')) {
        const line = raw.replace(/\r$/, '');
        if (!line.trim()) continue;

        if (line.startsWith('#')) {
            // The column header is the last #-prefixed line before the rows.
            const maybe = line.replace(/^#\s*/, '').split('\t').map(c => c.trim());
            if (maybe.includes('id') && maybe.includes('defaultNorm')) cols = maybe;
            continue;
        }
        if (!cols) continue;

        const cells = line.split('\t');
        const get = (k) => { const i = cols.indexOf(k); return i >= 0 ? (cells[i] ?? '').trim() : ''; };

        const id = get('id');
        if (!id || id === '<no-id>') continue;
        ++rows;

        const flags = get('flags');
        const num   = (s) => { const m = String(s).match(/-?\d+(?:\.\d+)?/); return m ? parseFloat(m[0]) : null; };

        if (/\bboolean\b/.test(flags)) {
            toggles[id] = num(get('defaultNorm')) >= 0.5;
            continue;
        }

        const lo = num(get('textAtMin'));
        const hi = num(get('textAtMax'));
        const dv = num(get('defaultText'));

        if (lo === null || hi === null || lo === hi) continue;

        const entry = { start: lo, end: hi, skew: 1, interval: 0, label: get('label') || '' };
        entry.def = (dv !== null && dv >= Math.min(lo, hi) && dv <= Math.max(lo, hi))
            ? dv
            : lo + (hi - lo) * 0.5;

        const steps = parseInt(get('numSteps'), 10);
        if (Number.isFinite(steps) && steps > 0 && steps < 2147483647) entry.numSteps = steps;

        sliders[id] = entry;
    }

    return { sliders, toggles, rows };
}

// tests/ui-stub/generic-overrides.json is the hand-written escape hatch for the
// handful of values a TSV cannot supply — a combo box's CHOICE STRINGS above
// all, which are C++ StringArrays and appear in no parameter dump. It is merged
// LAST so a human's judgement beats a parsed number.
function stubSeed(name, repoRoot = REPO_ROOT) {
    const seed = { sliders: {}, toggles: {}, combos: {}, natives: {} };
    let from = 'neutral-defaults';

    const tsv = path.join(pluginRoot(name, repoRoot), '.planning', 'params.tsv');
    if (fs.existsSync(tsv)) {
        const parsed = parseParamDumpTsv(fs.readFileSync(tsv, 'utf8'));
        if (parsed.rows > 0) {
            Object.assign(seed.sliders, parsed.sliders);
            Object.assign(seed.toggles, parsed.toggles);
            from = `param-dump (${parsed.rows} rows)`;
        }
    }

    const ov = path.join(pluginRoot(name, repoRoot), 'tests', 'ui-stub', 'generic-overrides.json');
    if (fs.existsSync(ov)) {
        try {
            const j = JSON.parse(fs.readFileSync(ov, 'utf8'));
            for (const k of ['sliders', 'toggles', 'combos', 'natives'])
                if (j[k]) Object.assign(seed[k], j[k]);
            from = from === 'neutral-defaults' ? 'overrides' : from + ' + overrides';
        } catch (e) {
            // A malformed override file is reported, never silently ignored: a
            // typo'd JSON that boots the page with neutral fixtures looks
            // exactly like a correct one.
            seed.__error = `generic-overrides.json did not parse: ${e.message}`;
        }
    }

    seed.__from = from;
    return seed;
}

// ─────────────────────────────────────────────────────────── the stub ──

// A per-plugin tests/ui-stub/juce-stub.js ALWAYS wins. The five that have one
// keep using it, so this tool cannot regress a gate that already passes; the
// generic stub is what the other 38 get.
function stubFor(name, repoRoot = REPO_ROOT) {
    const own = path.join(pluginRoot(name, repoRoot), 'tests', 'ui-stub', 'juce-stub.js');
    if (fs.existsSync(own)) return { path: own, kind: 'plugin' };
    return { path: path.join(__dirname, 'ui-stub', 'generic-juce-stub.js'), kind: 'generic' };
}

// ──────────────────────────────────────────────────────── build the tree ──

function buildRoot(name, opts = {}) {
    const repoRoot = opts.repoRoot || REPO_ROOT;
    const ui = resolveUiRoot(name, repoRoot);
    if (!ui) throw new Error(`serve-ui: no UI root for ${name} (no index.html under Resources/ui or Source/ui/public, and no CMake SOURCES under either)`);

    const indexHtml = path.join(ui.uiRoot, 'index.html');
    if (!fs.existsSync(indexHtml))
        throw new Error(`serve-ui: ${name} resolves its UI root to ${ui.label} but there is no index.html there`);

    const root = fs.mkdtempSync(path.join(os.tmpdir(), `ouaricon-ui-${name}-`));
    fs.cpSync(ui.uiRoot, root, { recursive: true });

    // ── embedded sources placed at the url getResource() serves them from ──
    const urls = urlSymbolMap(name, repoRoot);
    const placed = [];
    const unplaced = [];
    const missing = [];

    for (const s of binaryDataSources(name, repoRoot)) {
        if (!s.exists) { missing.push(s.raw); continue; }

        const insideUiRoot = s.abs.startsWith(ui.uiRoot + path.sep);
        const url = urls.get(s.symbol);

        if (!url) {
            // Already served from its natural place inside the copied tree, or
            // genuinely unmapped — the latter is reported, never swallowed.
            if (!insideUiRoot) unplaced.push(`${s.raw} (symbol ${s.symbol})`);
            continue;
        }

        const dest = path.join(root, url.replace(/^\//, ''));
        if (!dest.startsWith(root)) { unplaced.push(`${s.raw} -> ${url} (escapes the served root)`); continue; }

        fs.mkdirSync(path.dirname(dest), { recursive: true });
        fs.copyFileSync(s.abs, dest);
        if (!insideUiRoot) placed.push(`${path.relative(repoRoot, s.abs)} -> ${url}`);
    }

    // ── the bridge stub ──
    const stub = stubFor(name, repoRoot);
    const stubDest = path.join(root, 'js', 'juce', 'index.js');
    fs.mkdirSync(path.dirname(stubDest), { recursive: true });
    fs.copyFileSync(stub.path, stubDest);

    // ── the preamble, GENERIC STUB ONLY ──
    //
    // O-TextureForge bundles the JUCE frontend library into app.bundle.js and
    // never imports js/juce/index.js, so overlaying that file reaches it not at
    // all — the bundled library reads window.__JUCE__ directly. A preamble in
    // <head> is the only place a global can be installed before a bundle runs.
    //
    // Deliberately NOT injected when a plugin brings its own stub: those five
    // have committed gates that pass against a tree without it, and a harness
    // that alters the page for a plugin whose gate is already green is a
    // regression waiting to be blamed on something else.
    let preamble = false;
    const seed = stubSeed(name, repoRoot);

    if (stub.kind === 'generic' && opts.preamble !== false) {
        const src = path.join(__dirname, 'ui-stub', 'stub-preamble.js');
        if (fs.existsSync(src)) {
            // The seed is INLINED into the preamble rather than fetched. The
            // stub module is evaluated at import time and cannot await a fetch
            // before exporting getSliderState — a page importing an unresolved
            // binding throws at module scope and takes the whole UI down
            // (pattern_module_toplevel_init_tdz).
            const preambleSrc = fs.readFileSync(src, 'utf8')
                + `\nwindow.__stubOverrides = ${JSON.stringify(seed)};\n`;
            fs.writeFileSync(path.join(root, 'js', 'juce', 'stub-preamble.js'), preambleSrc);

            const html = fs.readFileSync(path.join(root, 'index.html'), 'utf8');
            const tag  = '\n<script src="/js/juce/stub-preamble.js"></script>';
            const headAt = html.search(/<head\b[^>]*>/i);
            if (headAt >= 0) {
                const insertAt = html.indexOf('>', headAt) + 1;
                fs.writeFileSync(path.join(root, 'index.html'),
                                 html.slice(0, insertAt) + tag + html.slice(insertAt));
                preamble = true;
            }
        }
    }

    return {
        root,
        plugin: name,
        uiRoot: ui.uiRoot,
        uiRootLabel: ui.label,
        uiRootFrom: ui.from,
        stubKind: stub.kind,
        stubPath: stub.path,
        seedFrom: seed.__from,
        seedError: seed.__error || null,
        preamble,
        placed,
        unplaced,
        missing,
    };
}

// ───────────────────────────────────────────────────────────── the server ──

const MIME = {
    '.html': 'text/html; charset=utf-8',
    '.htm':  'text/html; charset=utf-8',
    '.css':  'text/css; charset=utf-8',
    '.js':   'application/javascript; charset=utf-8',
    '.mjs':  'application/javascript; charset=utf-8',
    '.json': 'application/json; charset=utf-8',
    '.svg':  'image/svg+xml',
    '.png':  'image/png',
    '.jpg':  'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.gif':  'image/gif',
    '.webp': 'image/webp',
    '.ico':  'image/x-icon',
    '.woff': 'font/woff',
    '.woff2':'font/woff2',
    '.ttf':  'font/ttf',
    '.otf':  'font/otf',
    '.wasm': 'application/wasm',
};

function serve(root, onMiss) {
    const server = http.createServer((req, res) => {
        const rel  = decodeURIComponent(req.url.split('?')[0]);
        const file = path.join(root, rel === '/' ? 'index.html' : rel);

        if (!file.startsWith(root) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) {
            if (typeof onMiss === 'function') onMiss(rel);
            res.writeHead(404); res.end('not found'); return;
        }

        res.writeHead(200, { 'Content-Type': MIME[path.extname(file).toLowerCase()] || 'application/octet-stream' });
        fs.createReadStream(file).pipe(res);
    });

    return new Promise((resolve) => server.listen(0, '127.0.0.1', () =>
        resolve({ server, port: server.address().port, close: () => new Promise(r => server.close(r)) })));
}

// ────────────────────────────────────────────────────────────── playwright ──

// Verbatim in behaviour from the three committed clamp gates: resolve from a
// local install, a global install, or npx's cache. It deliberately does NOT
// install anything — a verification script that mutates the machine to make
// itself pass is not a verification script.
function resolvePlaywright() {
    const { execSync } = require('child_process');
    const candidates = ['playwright'];

    try { candidates.push(path.join(execSync('npm root -g', { encoding: 'utf8' }).trim(), 'playwright')); }
    catch { /* npm not on PATH — the other candidates still stand */ }

    const npxCache = path.join(os.homedir(), '.npm', '_npx');
    if (fs.existsSync(npxCache)) {
        for (const dir of fs.readdirSync(npxCache)) {
            const p = path.join(npxCache, dir, 'node_modules', 'playwright');
            if (fs.existsSync(p)) candidates.push(p);
        }
    }

    for (const c of candidates) { try { return require(c); } catch { /* next */ } }
    return null;
}

module.exports = {
    REPO_ROOT,
    listPlugins,
    pluginRoot,
    readCmake,
    binaryDataSymbol,
    binaryDataSources,
    resolveUiRoot,
    readEditorSize,
    urlSymbolMap,
    stubFor,
    parseParamDumpTsv,
    stubSeed,
    buildRoot,
    serve,
    resolvePlaywright,
    MIME,
};

// ────────────────────────────────────────────────────────────────── CLI ──
if (require.main === module) {
    const argv = process.argv.slice(2);
    const val  = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
    const only = val('--plugin');

    if (!only) {
        console.log('usage: node scripts/serve-ui.js --plugin <Name> [--report] [--hold]');
        console.log('\nplugins:');
        for (const n of listPlugins()) {
            const ui = resolveUiRoot(n);
            const sz = readEditorSize(n);
            console.log(`  ${n.padEnd(30)} ${(ui ? ui.label : 'NO UI ROOT').padEnd(18)} `
                      + `${sz ? sz.w + 'x' + sz.h : 'NO setSize'}  stub=${stubFor(n).kind}`);
        }
        process.exit(0);
    }

    const r = buildRoot(only);
    console.log(JSON.stringify({ ...r, size: readEditorSize(only) }, null, 2));

    if (argv.includes('--hold')) {
        serve(r.root).then(({ port }) => {
            console.log(`\nserving http://127.0.0.1:${port}/index.html  (^C to stop)`);
        });
    } else {
        fs.rmSync(r.root, { recursive: true, force: true });
    }
}
