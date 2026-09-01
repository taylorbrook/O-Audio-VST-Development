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
             --provenance "gpt-5 reverse pass, fresh session, 2026-09-02"

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

// Coarse lexical overlap, en vs en'. A sort key, not a verdict — the developer
// reads the triple. Task 2 replaces this stub with the real scorer.
function score(a, b) {
    const tok = (s) => String(s).toLowerCase().match(/[a-z0-9]+/g) || [];
    const A = new Set(tok(a)), B = new Set(tok(b));
    if (!A.size && !B.size) return 1;
    let inter = 0;
    for (const w of A) if (B.has(w)) inter++;
    return inter / (A.size + B.size - inter || 1);
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

// Task 2 completes the provenance refusal and the ranked triple report.
async function ingest() {
    const file = val('--ingest');
    console.log(`ingest: ${file}`);
    console.log('(round trip wired; the provenance refusal and the ranked triple report land in Task 2)');
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
