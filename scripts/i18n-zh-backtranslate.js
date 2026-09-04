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

    i18n-zh-backtranslate.js — the en -> zh -> en' round trip, per plugin.

    ── REPORT TODAY, GATE LATER ──────────────────────────────────────────────

    This tool ships as a REPORT: it exits 0 regardless of what it finds — even
    when it REFUSES an ingest. It is promoted to a GATE (exit 2 on any finding)
    only once the Stage 2 pilot (O-Chorus) is at zero findings, which is the
    exact lifecycle scripts/i18n-fr-lint.js went through. A half-built review
    tool must not be able to block Stage 2.

    ── WHY IT EXISTS ─────────────────────────────────────────────────────────

    The developer reads French; the French `reviewed: true` flip meant literally
    "the developer read it". That lane is CLOSED for Chinese — this project has
    no native Chinese reader. The substitute is a back-translation: a second,
    INDEPENDENT pass renders the shipped Chinese back into English, and the
    developer reads the drift in a language they can read. That is what
    `reviewed: 'bt'` asserts, and 'bt' is the ship bar for this rollout.

    ── WHAT THIS TOOL DOES, MECHANICALLY ─────────────────────────────────────

      enumerate  walk all 43 plugins by dynamic ESM import over both UI roots
                 and collect every zh-Hans string with its English source
      withhold   --emit writes id + zh ONLY. The English is deliberately left
                 out of the batch
      join       --ingest re-attaches the returned en' to the stored en and zh
      diff       prints the en -> zh -> en' triple, worst drift first
      score      lexical overlap between en and en', as a coarse sort key
      record     refuses to report a triple whose reverse-pass provenance is
                 unrecorded, or is the same pass that produced the Chinese

    ── WHAT THIS TOOL CANNOT DO ──────────────────────────────────────────────

    It cannot produce the en' itself. That requires a SEPARATE, INDEPENDENT
    pass — a different agent, session or service that has never seen the English
    source. Withholding the English is the whole independence mechanism: a
    reverse pass that can see the source round-trips the source's own vocabulary
    and reads clean while the Chinese is wrong. A triple whose provenance is
    unrecorded proves nothing, so the tool refuses to report one.

    Usage:
        node scripts/i18n-zh-backtranslate.js                     # coverage report
        node scripts/i18n-zh-backtranslate.js --plugin O-Chorus
        node scripts/i18n-zh-backtranslate.js --verbose           # every row
        node scripts/i18n-zh-backtranslate.js --emit O-Chorus --out /tmp/b.tsv \
             --forward-provenance "claude-opus draft, 2026-09-01"
        node scripts/i18n-zh-backtranslate.js --ingest /tmp/b.en.tsv \
             --provenance "gpt-5 reverse pass, fresh session, 2026-09-02" \
             [--manifest /tmp/b.tsv.manifest.json]
             # manifest resolution, first hit wins: an explicit --manifest (never
             # silently ignored) -> <ingest>.manifest.json -> the emit form with an
             # `.en` segment dropped before the final extension -> a scan of the
             # ingest file's directory, which must find exactly one. Zero, two or
             # more, unparseable, or no recorded forward pass all REFUSE.

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');
const { pathToFileURL } = require('url');

const LANG = 'zh-Hans';
const REVIEWED_ENUM = ['mt', 'bt', 'native'];

const argv    = process.argv.slice(2);
const val     = (k) => { const i = argv.indexOf(k); return i >= 0 ? argv[i + 1] : null; };
const only    = val('--plugin');
const verbose = argv.includes('--verbose');
const MAX_SHOWN = verbose ? Infinity : 12;

const ROOT = path.resolve(__dirname, '..');
const UI_ROOTS = ['Source/ui/public/js/i18n.js', 'Resources/ui/js/i18n.js'];

function pluginList() {
    return fs.readdirSync(path.join(ROOT, 'plugins'))
        .filter((n) => n.startsWith('O-') && fs.statSync(path.join(ROOT, 'plugins', n)).isDirectory())
        .filter((n) => !only || n === only).sort();
}

// The same dynamic-ESM walk the lint uses. A hand-rolled parser would drift
// from the 10 plugins that keep their table under Resources/ui.
async function readPlugin(name) {
    const rel = UI_ROOTS.find((r) => fs.existsSync(path.join(ROOT, 'plugins', name, r)));
    if (!rel) return { name, error: 'no i18n.js under either UI root', rows: [] };
    let m;
    try { m = await import(pathToFileURL(path.join(ROOT, 'plugins', name, rel)).href); }
    catch (e) { return { name, error: `import failed: ${String(e.message).split('\n')[0]}`, rows: [] }; }
    const rows = [];
    for (const [k, v] of Object.entries(m.LABELS || {}))
        rows.push({ id: `${name}|label|${k}`, plugin: name, kind: 'label', key: k,
                    en: v.en?.t ?? '', zh: v[LANG]?.t ?? '', reviewed: v[LANG]?.reviewed });
    for (const [k, v] of Object.entries(m.I18N || {})) {
        rows.push({ id: `${name}|title|${k}`, plugin: name, kind: 'title', key: k,
                    en: v.en?.t ?? '', zh: v[LANG]?.t ?? '', reviewed: v[LANG]?.reviewed });
        if ((v.en?.b ?? '') !== '' || (v[LANG]?.b ?? '') !== '')
            rows.push({ id: `${name}|body|${k}`, plugin: name, kind: 'body', key: k,
                        en: v.en?.b ?? '', zh: v[LANG]?.b ?? '', reviewed: v[LANG]?.reviewed });
    }
    return { name, rows };
}

async function collect() {
    const plugins = pluginList();
    const out = [];
    for (const n of plugins) out.push(await readPlugin(n));
    return { plugins, results: out };
}

// Coarse lexical overlap, en vs en-prime, on the unit interval. This is a SORT
// KEY, not a verdict: it exists so the worst drift floats to the top of a 3789
// row report. The verdict is the developer reading the triple. A high score is
// not a pass — a back-translation can be lexically identical and still describe
// a different control, which is exactly why the triple is printed in full.
const STOP = new Set(['the', 'a', 'an', 'of', 'to', 'and', 'or', 'in', 'on', 'for', 'is', 'are', 'be', 'this', 'that', 'it', 'its', 'with', 'as', 'at', 'by']);
function tokens(s) {
    return (String(s).toLowerCase().match(/[a-z0-9]+/g) || [])
        .map((w) => w.replace(/(?:ies)$/, 'y').replace(/(?:es|s)$/, ''))
        .filter((w) => w && !STOP.has(w));
}
function score(a, b) {
    const A = new Set(tokens(a)), B = new Set(tokens(b));
    if (!A.size && !B.size) return 1;
    if (!A.size || !B.size) return 0;
    let inter = 0;
    for (const w of A) if (B.has(w)) inter++;
    const jaccard = inter / (A.size + B.size - inter);
    // Containment rescues a correct back-translation that is merely wordier
    // than the caption it came from ("Mix" -> "wet/dry mix amount").
    const containment = inter / Math.min(A.size, B.size);
    return (2 * jaccard + containment) / 3;
}

function emit() {
    const target = val('--emit');
    const out = val('--out');
    if (!out) {
        console.log('REFUSED: --emit requires --out <path>. The tool does not guess a write path.');
        return Promise.resolve();
    }
    return collect().then(({ results }) => {
        const rows = results.flatMap((r) => r.rows).filter((r) => r.zh);
        // THE ENGLISH IS WITHHELD. This is the independence mechanism, not an
        // oversight: a reverse pass that can see the source round-trips the
        // source's own vocabulary and reads clean while the Chinese is wrong.
        const tsv = rows.map((r) => `${r.id}\t${String(r.zh).replace(/\t|\n/g, ' ')}`).join('\n') + '\n';
        fs.writeFileSync(out, tsv, 'utf8');
        const manifest = {
            emittedAt: new Date().toISOString(),
            target: target || '--all',
            rows: rows.length,
            forwardProvenance: val('--forward-provenance') || null,
            note: 'The English source is deliberately absent from the batch. Return id \\t en-prime.',
        };
        fs.writeFileSync(`${out}.manifest.json`, JSON.stringify(manifest, null, 2) + '\n', 'utf8');
        console.log(`emitted ${rows.length} rows (id + zh only; the English is withheld) -> ${out}`);
        console.log(`manifest -> ${out}.manifest.json`);
        if (!rows.length) console.log('VACUITY: the corpus carries no zh-Hans string yet, so the batch is empty — nothing was emitted to translate back.');
    });
}

const DEFAULT_HEADLINE = 'back-translation provenance is missing or identical to the forward pass — this triple proves nothing';
const DEFAULT_REMEDY = [
    'Re-run with --provenance "<what produced the en-prime, and when>", naming a pass that',
    'is NOT the one that produced the Chinese. A reverse pass that can see the source — or',
    'that IS the source — round-trips its own vocabulary and reads clean while the Chinese',
    'is wrong. That is the failure this refusal exists to prevent.',
];

// Every refusal goes through here so they all keep the tool's report-only
// exit-0 contract: a refusal is a reported RESULT, never a crash.
function refuse(why, opts = {}) {
    console.log(`REFUSED: ${opts.headline || DEFAULT_HEADLINE}`);
    for (const line of [].concat(why)) console.log(`  ${line}`);
    for (const line of opts.remedy || DEFAULT_REMEDY) console.log(`  ${line}`);
    console.log(`\nREPORT ONLY — exit 0. A refusal is a reported result, not a crash.`);
}

// ── manifest resolution ─────────────────────────────────────────────────────
// The manifest is the ONLY record of which pass produced the Chinese, so the
// identity refusal cannot run without it.
//
// It used to be derived in one expression — `val('--manifest') ||
// `${file}.manifest.json`` — from the file passed to --ingest, which is the
// RETURNED en' file, while --emit writes its manifest beside its --out file.
// In the documented workflow (emit /tmp/b.tsv, ingest /tmp/b.en.tsv) those two
// never coincide, so the lookup landed on /tmp/b.en.tsv.manifest.json, which
// never exists. `manifest` stayed null, `fwd` stayed null, the identity refusal
// was skipped, and the report printed a parenthetical announcing its own
// inertness before carrying on to print triples anyway.
const MANIFEST_SUFFIX = '.manifest.json';

// `b.en.tsv` -> `b.tsv`: drop an `.en` segment sitting immediately before the
// final extension. This is what makes the documented workflow resolve with no
// argument at all.
function emitFormOf(file) {
    const m = path.basename(file).match(/^(.*)\.en(\.[^.]+)$/);
    return m ? path.join(path.dirname(file), m[1] + m[2]) : null;
}

// Returns { path } on success, or { refusal: {why, opts} } — never guesses.
function resolveManifestPath(file) {
    const headline = 'the batch manifest could not be resolved — without it the forward pass is unknown and the identity check cannot run';

    // 1. An explicit --manifest is NEVER silently ignored.
    const explicit = val('--manifest');
    if (explicit) {
        if (!fs.existsSync(explicit)) {
            return { refusal: { why: [`--manifest ${explicit} does not exist.`,
                                     'An explicit path is taken at its word; the tool does not fall back to a search behind your back.'],
                                opts: { headline, remedy: ['Pass the manifest --emit actually wrote, or drop --manifest to let the tool derive it.'] } } };
        }
        return { path: explicit };
    }

    // 2. Derived from the ingest path, in a fixed documented order.
    const tried = [`${file}${MANIFEST_SUFFIX}`];
    const emitForm = emitFormOf(file);
    if (emitForm) tried.push(`${emitForm}${MANIFEST_SUFFIX}`);
    for (const c of tried) if (fs.existsSync(c)) return { path: c };

    // 3. A scan of the ingest file's own directory. Exactly one match, or refuse.
    const dir = path.dirname(path.resolve(file));
    let found = [];
    try {
        found = fs.readdirSync(dir).filter((n) => n.endsWith(MANIFEST_SUFFIX)).sort()
            .map((n) => path.join(dir, n));
    } catch { found = []; }

    if (found.length === 1) return { path: found[0] };
    if (found.length > 1) {
        return { refusal: { why: [`AMBIGUOUS: ${found.length} manifests sit beside ${file} and none matches a derived name:`,
                                 ...found.map((f) => `    ${f}`),
                                 'The tool does not guess which batch this file came back from — joining against the wrong',
                                 'manifest silently reports triples for rows that were never in this batch.'],
                            opts: { headline, remedy: ['Name the right one with --manifest <path>.'] } } };
    }
    return { refusal: { why: ['No manifest found. Tried, in order:',
                             ...tried.map((f) => `    ${f}`),
                             `    a scan of ${dir} for *${MANIFEST_SUFFIX} (0 matches)`],
                        opts: { headline, remedy: ['Pass --manifest <path> naming the manifest --emit wrote beside its --out file.'] } } };
}

async function ingest() {
    const file = val('--ingest');
    const provenance = val('--provenance');

    // The refusal is checked BEFORE anything is read. A triple whose reverse
    // pass is unrecorded proves nothing, so there is nothing to gain by
    // parsing anything first — and it costs no file read.
    if (!provenance || !String(provenance).trim())
        return refuse('no --provenance was given.');

    // The batch file is checked BEFORE the manifest, so a mistyped batch path
    // reports the missing file rather than a manifest complaint about it.
    if (!file || !fs.existsSync(file)) { console.log(`ingest: no such file: ${file}`); console.log('\nREPORT ONLY — exit 0.'); return; }

    const resolved = resolveManifestPath(file);
    if (resolved.refusal) return refuse(resolved.refusal.why, resolved.refusal.opts);
    const manifestPath = resolved.path;

    // A manifest that fails to parse must REFUSE naming the error. Swallowing
    // the throw and continuing with a null manifest is the same inert path by
    // another route.
    let manifest;
    try { manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8')); }
    catch (e) {
        return refuse([`${manifestPath} could not be parsed: ${String(e.message).split('\n')[0]}`,
                       'A manifest that does not parse records nothing, and a triple with no recorded forward pass proves nothing.'],
                      { headline: 'the batch manifest is unreadable — the identity check cannot run',
                        remedy: ['Re-emit the batch, or repair the manifest JSON.'] });
    }

    const fwd = manifest && manifest.forwardProvenance;
    if (!fwd || !String(fwd).trim()) {
        return refuse([`${manifestPath} records no forward provenance.`,
                       'The batch was emitted without --forward-provenance, so there is nothing to compare',
                       'the reverse pass against: the tool cannot tell an independent pass from the pass',
                       'that produced the Chinese, which is the ONE thing this check exists to tell apart.'],
                      { headline: 'the batch was emitted with no recorded forward pass — the identity check cannot run',
                        remedy: ['Re-emit with --forward-provenance "<what produced the Chinese, and when>", then re-run the reverse pass.'] });
    }
    if (String(fwd).trim().toLowerCase() === String(provenance).trim().toLowerCase())
        return refuse(`--provenance is byte-identical to the forward pass recorded at emit time (${JSON.stringify(fwd)}).`);

    const returned = new Map();
    for (const line of fs.readFileSync(file, 'utf8').split('\n')) {
        if (!line.trim() || line.startsWith('#')) continue;
        const [id, ...rest] = line.split('\t');
        if (id && rest.length) returned.set(id.trim(), rest.join(' ').trim());
    }
    if (!returned.size) {
        console.log(`ingest: ${file} carries no "id <TAB> en-prime" row.`);
        console.log(`  provenance recorded: ${JSON.stringify(provenance)}`);
        console.log('\nREPORT ONLY — exit 0.');
        return;
    }

    const { results } = await collect();
    const byId = new Map(results.flatMap((r) => r.rows).map((r) => [r.id, r]));

    const triples = [];
    const orphans = [];
    for (const [id, enPrime] of returned) {
        const row = byId.get(id);
        if (!row) { orphans.push(id); continue; }
        triples.push({ ...row, enPrime, s: score(row.en, enPrime) });
    }
    triples.sort((a, b) => a.s - b.s);   // worst drift first

    console.log('i18n-zh-backtranslate --ingest — en -> zh -> en\' triples, worst drift first');
    console.log(`  provenance (reverse pass): ${JSON.stringify(provenance)}`);
    // No parenthetical about a missing manifest: there is no longer a path on
    // which ingest reports a triple without a resolved manifest, which is the
    // whole fix. The refusal always existed — it simply never had its input.
    console.log(`  manifest: ${manifestPath}`);
    console.log(`  forward pass recorded at emit: ${JSON.stringify(fwd)}`);
    console.log(`  joined: ${triples.length}${orphans.length ? `   unjoinable ids: ${orphans.length}` : ''}\n`);
    for (const t of triples.slice(0, MAX_SHOWN)) {
        console.log(`  ${t.s.toFixed(2)}  ${t.id}`);
        console.log(`        en   ${t.en}`);
        console.log(`        zh   ${t.zh}`);
        console.log(`        en'  ${t.enPrime}`);
    }
    if (triples.length > MAX_SHOWN) console.log(`  … ${triples.length - MAX_SHOWN} more (--verbose)`);
    for (const id of orphans.slice(0, 5)) console.log(`  UNJOINABLE ${id} — no such row in the live corpus`);
    console.log(`\n  The score is a SORT KEY, not a verdict. A high score is not a pass: a`);
    console.log(`  back-translation can be lexically identical and still name a different control.`);
    console.log(`\nREPORT ONLY — exit 0. This becomes a gate (exit 2) once the O-Chorus pilot is at zero findings.`);
}

async function report() {
    const { plugins, results } = await collect();
    console.log('i18n-zh-backtranslate — en -> zh -> en\' coverage');
    console.log(`  plugins: ${plugins.length}   (REPORT: exits 0 whatever it finds; becomes a gate once the O-Chorus pilot is at zero)\n`);
    console.log('  ' + 'plugin'.padEnd(28) + ' rows   zh    mt    bt native  none');

    let zhTotal = 0, rowTotal = 0, errors = 0;
    const tally = { mt: 0, bt: 0, native: 0, none: 0 };
    const shownRows = [];
    for (const r of results) {
        if (r.error) { errors++; console.log(`  ${r.name.padEnd(28)} ERROR ${r.error}`); continue; }
        const zhRows = r.rows.filter((x) => x.zh);
        const per = { mt: 0, bt: 0, native: 0, none: 0 };
        for (const x of zhRows) per[REVIEWED_ENUM.includes(x.reviewed) ? x.reviewed : 'none']++;
        for (const k of Object.keys(tally)) tally[k] += per[k];
        zhTotal += zhRows.length; rowTotal += r.rows.length;
        shownRows.push(...zhRows);
        console.log(`  ${r.name.padEnd(28)} ${String(r.rows.length).padStart(4)} ${String(zhRows.length).padStart(4)} `
            + `${String(per.mt).padStart(5)} ${String(per.bt).padStart(5)} ${String(per.native).padStart(6)} ${String(per.none).padStart(5)}`);
    }
    console.log('  ' + '─'.repeat(64));
    console.log('  ' + 'TOTAL'.padEnd(28) + ` ${String(rowTotal).padStart(4)} ${String(zhTotal).padStart(4)} `
        + `${String(tally.mt).padStart(5)} ${String(tally.bt).padStart(5)} ${String(tally.native).padStart(6)} ${String(tally.none).padStart(5)}`);

    for (const x of shownRows.slice(0, MAX_SHOWN))
        console.log(`  ${x.kind.padEnd(5)} ${x.id.padEnd(48)} "${String(x.zh).slice(0, 60)}"  reviewed=${JSON.stringify(x.reviewed)}`);
    if (shownRows.length > MAX_SHOWN) console.log(`  … ${shownRows.length - MAX_SHOWN} more (--verbose)`);

    console.log(`\n-- summary`);
    if (zhTotal === 0) {
        console.log(`  VACUITY: 0 zh-Hans entries found across ${plugins.length} plugins — nothing was checked.`);
        console.log(`  This is not a pass. No back-translation is possible until the rollout writes Chinese.`);
    } else {
        console.log(`  zh-Hans strings: ${zhTotal} of ${rowTotal} rows${errors ? `   (${errors} plugins could not be read)` : ''}`);
        console.log(`  BELOW SHIP BAR — at reviewed:'mt' or unflagged: ${tally.mt + tally.none}. The ship bar is 'bt'.`);
    }
    console.log(`\nREPORT ONLY — exit 0. This becomes a gate (exit 2) once the O-Chorus pilot is at zero findings.`);
}

(async () => {
    if (argv.includes('--emit')) { await emit(); return; }
    if (argv.includes('--ingest')) { await ingest(); return; }
    await report();
})();
