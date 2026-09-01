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

    i18n-fr-lint.js — French typography and terminology, across every plugin.

    WHAT IT IS. check-i18n.js proves the MECHANISM: every key resolves, every
    French entry carries a reviewed flag, the canon has not drifted. It says
    nothing about whether the French is any good. This file checks the part of
    "good" that a regex can see, so that the part a person has to read is the
    only part left for the person.

    It is a REPORT by default and exits 0. `--strict` exits 2 on any failure,
    for the moment every plugin is clean and the developer chooses to make it a
    gate. The repo already knows what a permanently-red gate does to the habit
    of reading gates, and on the day this file was written 43 of 43 plugins
    failed it.

    ── The checks ────────────────────────────────────────────────────────────

      T1  apostrophe     ' between letters → U+2019 ’   (l’entrée, d’archet)
      T2  decimal        \d.\d in French → a comma          (0,1 à 100 ms)
      T3  percent        a plain space or none before %  → U+00A0 (50 %)
      T4  colon          no U+00A0 before :               (Plage : −6 à +6 dB)
      T5  ; ! ?          no U+00A0 before ; ! ?           (Confirmer ?)
      T6  minus          -digit after a space/paren → U+2212 (−40 à +40 dB)
      T7  unit           number and unit with a plain space or none → U+00A0
      G1  glossary       a LABEL or tooltip TITLE whose English is in
                         scripts/i18n-fr-glossary.js TERMS renders as one of
                         its accepted French forms (casing ignored)
      C1  casing         the French follows the English caption's casing —
                         an all-caps caption stays all-caps, a mixed-case one
                         does not shout
      F1  forbidden      a word from FORBIDDEN_IN_LABELS in a label/title, or
                         from FORBIDDEN_IN_PROSE in a body

    T1–T7 run on every French string: LABELS, tooltip titles, tooltip bodies.
    G1, C1 and F1(labels) run on LABELS and tooltip titles only; prose is a
    person's job. Bodies get F1(prose).

    INFO, never a failure: entries marked `sameAsEn: true`, and entries
    carrying a `termNote` (a reasoned glossary exemption — see the glossary
    header). Both are listed so the reviewer sees them, and neither counts.

    ── Why U+00A0 and not U+202F ─────────────────────────────────────────────

    The Imprimerie nationale wants a THIN no-break space before ; ! ? and a
    full one before :. Every plugin page ships its own web fonts and U+202F
    has no glyph in some of them — it would render as a box in the one place
    no gate looks. U+00A0 renders in every font that renders a space. One
    character, everywhere, is the rule.

    Usage:
        node scripts/i18n-fr-lint.js                    # all plugins, report
        node scripts/i18n-fr-lint.js --plugin O-Comp    # one plugin
        node scripts/i18n-fr-lint.js --strict           # exit 2 on any failure
        node scripts/i18n-fr-lint.js --verbose          # every finding, not 12

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');
const { pathToFileURL } = require('url');
const G = require(path.join(__dirname, 'i18n-fr-glossary.js'));
// TERMS keys are looked up by norm(en), which strips a trailing period - so a
// key that ends in one ('tuning panel failed to load.') could never match
// (O-Contrabass N7, the only such key of ~240). Normalise the keys the same way.
const TERMS = Object.fromEntries(Object.entries(G.TERMS).map(([k, v]) => [k.replace(/\.$/, '').trim().toLowerCase(), v]));

const argv    = process.argv.slice(2);
const val     = (k) => { const i = argv.indexOf(k); return i >= 0 ? argv[i + 1] : null; };
const only    = val('--plugin');
const strict  = argv.includes('--strict');
const verbose = argv.includes('--verbose');
const MAX_SHOWN = verbose ? Infinity : 12;

const ROOT = path.resolve(__dirname, '..');
const UI_ROOTS = ['Source/ui/public/js/i18n.js', 'Resources/ui/js/i18n.js'];

const NBSP = ' ';
// `\d ?unit`: a MISSING space is as wrong as a plain one (O-Comp pilot: the
// first draft required exactly one ASCII space and let `440Hz` through). `%`
// is T3's and is filtered out here so `50 %` does not report twice.
const UNIT_RE = new RegExp(`\\d ?(${G.UNITS.filter((u) => u !== '%').map((u) => u.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')).join('|')})(?![A-Za-zÀ-ÿ])`);

// The lint's normalisation, mirrored in the glossary header. Change both.
function norm(s) {
    return String(s).replace(/’/g, "'").replace(/[  ]/g, ' ').trim()
        .replace(/\s+/g, ' ').replace(/\.$/, '').toLowerCase();
}
const letters = (s) => String(s).replace(/\{[^}]*\}/g, '').replace(/[^A-Za-zÀ-ÿŒœ]/g, '');
const isAllCaps = (s) => { const l = letters(s); return l.length >= 2 && l === l.toUpperCase(); };
const hasLower  = (s) => /[a-zà-ÿœ]/.test(letters(s));

function typography(fr, en) {
    const out = [];
    if (/[A-Za-zÀ-ÿ]'[A-Za-zÀ-ÿŒœ]/.test(fr))            out.push('T1');
    // An IDENTIFIER keeps its point; a NUMBER takes a comma. Identifiers are:
    // a surround-format name (Logic's "7.1", O-Octagon N4), any token with two
    // or more dots ("7.1.4", "1.10.0"), and a token that follows a version word
    // ("pre-1.10", "la version 1.10", "v1.7", O-Bitrot N5). "7,1" is not a
    // format and "1,10" is not a version. NOT "any token the English also
    // carries": the English writes real decimals with a point too, so that
    // rule exempted exactly the defect T2 exists for (it zeroed the column).
    const frStripped = fr
        .replace(/\b(?:5\.1|7\.1|9\.1|5\.1\.2|5\.1\.4|7\.1\.2|7\.1\.4|9\.1\.4|9\.1\.6)\b/g, '')
        .replace(/\d+(?:\.\d+){2,}/g, '')
        .replace(/(\b(?:v|ver\.?|version|pre-|antérieure? à la version)\s*)\d+(?:\.\d+)+/gi, '$1');
    if (/\d\.\d/.test(frStripped))                        out.push('T2');
    if (/\d ?%/.test(fr))                                 out.push('T3');
    if (/(^|[^ ]):(?=\s|$)/.test(fr) && !/https?:/.test(fr)) out.push('T4');
    if (/(^|[^ ])[;!?]/.test(fr))                     out.push('T5');
    if (/(^|[\s(«])-\d/.test(fr))                         out.push('T6');
    // A decade name (60s, 70s, 80s — O-Detune's wobble_era faces) is not a
    // number missing its space before "seconds". Same family as T2's list.
    if (UNIT_RE.test(fr.replace(/\b[5-9]0s\b/g, '')))    out.push('T7');
    return out;
}

function forbidden(fr, table) {
    const hits = [];
    const low = norm(fr);
    for (const w of Object.keys(table)) {
        // norm() drops a TRAILING period from the value, so a key that ends in
        // one ('dériv.', 'fréq.', 'flatt.') could never match the caption it was
        // written for (O-MultiBandCompressor N4: "Dériv." drew G1 and not F1).
        // Match the stem; the lookahead already refuses a longer word.
        const stem = w.replace(/\.$/, '');
        const re = new RegExp(`(^|[^a-zà-ÿœ])${stem.replace(/\./g, '\\.')}(?![a-zà-ÿœ])`);
        if (re.test(low)) hits.push(w);
    }
    return hits;
}

async function lintPlugin(name) {
    const rel = UI_ROOTS.find((r) => fs.existsSync(path.join(ROOT, 'plugins', name, r)));
    if (!rel) return { name, error: 'no i18n.js under either UI root' };
    let m;
    try { m = await import(pathToFileURL(path.join(ROOT, 'plugins', name, rel)).href); }
    catch (e) { return { name, error: `import failed: ${String(e.message).split('\n')[0]}` }; }

    const rows = [];
    for (const [k, v] of Object.entries(m.LABELS || {}))
        rows.push({ kind: 'label', key: k, en: v.en?.t ?? '', fr: v.fr?.t ?? '', frObj: v.fr || {} });
    for (const [k, v] of Object.entries(m.I18N || {})) {
        rows.push({ kind: 'title', key: k, en: v.en?.t ?? '', fr: v.fr?.t ?? '', frObj: v.fr || {},
                    // sameAsEn is ENTRY-scoped in check-i18n (title AND body). A title that
                    // equals its English over a translated body needs no flag - and must
                    // not get one, or assertion 4 is disarmed for the entry (O-Tapestop N3).
                    bodyTranslated: (v.fr?.b ?? '') !== '' && norm(v.fr?.b ?? '') !== norm(v.en?.b ?? '') });
        if ((v.en?.b ?? '') !== '' || (v.fr?.b ?? '') !== '')
            rows.push({ kind: 'body', key: k, en: v.en?.b ?? '', fr: v.fr?.b ?? '', frObj: v.fr || {} });
    }

    const findings = [];   // {code, kind, key, fr, note}
    const info = { sameAsEn: [], termNote: [] };
    for (const r of rows) {
        if (!r.fr) continue;
        for (const code of typography(r.fr, r.en)) findings.push({ code, ...r });
        // A prose-forbidden word is forbidden EVERYWHERE - a LABELS dialog message
        // or an aria name is prose too (O-MicrotonalSampler N7: four 'plugiciel'
        // in label./aria. entries drew nothing while the scan was body-only).
        for (const w of forbidden(r.fr, G.FORBIDDEN_IN_PROSE))
            findings.push({ code: 'F1', ...r, note: `"${w}" → ${G.FORBIDDEN_IN_PROSE[w]}` });
        if (r.kind === 'body') continue;
        // labels and titles from here on
        // A straight copy is the CONDITION (fr === en), not the flag. The first
        // draft counted the flag and printed 0 on a page with an unflagged copy
        // (O-Texture pilot, tip.mode). Both are listed; the unflagged ones are
        // the ones check-i18n assertion 4 will refuse once they are LABELS or
        // once a tooltip's body matches too.
        if (norm(r.fr) === norm(r.en)) info.sameAsEn.push({ ...r, flagged: r.frObj.sameAsEn === true || r.bodyTranslated === true });
        // A termNote is THE reasoned exemption, and it exempts the entry from
        // both term checks — G1 and F1. The first draft guarded only G1, so an
        // entry was printed as EXEMPT and counted as an F1 finding in the same
        // run (O-simpleFM pilot, label.knobFixedHz "Fréq. fixe").
        const exempt = typeof r.frObj.termNote === 'string' && r.frObj.termNote.trim() !== '';
        if (exempt) info.termNote.push(r);
        const allowed = TERMS[norm(r.en)];
        const accepted = (allowed || []).map(norm).includes(norm(r.fr));
        if (!exempt && allowed && !accepted)
            findings.push({ code: 'G1', ...r, note: `"${r.en}" → ${allowed.join(' | ')}` });
        // A rendering the glossary itself accepts for this English is never a
        // forbidden word: "Écart total" is the settled term for "Total span"
        // and must not draw F1 for containing "écart". Found by the O-Chorus
        // pilot before any tuning-panel plugin could hit it.
        if (!exempt && !accepted)
            for (const w of forbidden(r.fr, G.FORBIDDEN_IN_LABELS))
                findings.push({ code: 'F1', ...r, note: `"${w}" → ${G.FORBIDDEN_IN_LABELS[w]}` });
        if (isAllCaps(r.en) && hasLower(r.fr))
            findings.push({ code: 'C1', ...r, note: 'English caption is all-caps; French is not' });
        else if (hasLower(r.en) && isAllCaps(r.fr) && letters(r.fr).length >= 4)
            findings.push({ code: 'C1', ...r, note: 'English caption is mixed-case; French shouts' });
    }
    return { name, rows: rows.length, findings, info };
}

(async () => {
    const plugins = fs.readdirSync(path.join(ROOT, 'plugins'))
        .filter((n) => n.startsWith('O-') && fs.statSync(path.join(ROOT, 'plugins', n)).isDirectory())
        .filter((n) => !only || n === only).sort();
    if (!plugins.length) { console.error(`i18n-fr-lint: no plugin matches --plugin ${only}`); process.exit(1); }

    console.log('i18n-fr-lint — French typography and terminology');
    console.log(`  plugins: ${plugins.length}   mode: ${strict ? 'STRICT (exit 2 on failure)' : 'report'}\n`);

    const CODES = ['T1', 'T2', 'T3', 'T4', 'T5', 'T6', 'T7', 'G1', 'C1', 'F1'];
    const totals = Object.fromEntries(CODES.map((c) => [c, 0]));
    let failedPlugins = 0, errors = 0, sameAsEnTotal = 0, sameAsEnFlagged = 0, termNoteTotal = 0;

    console.log('  ' + 'plugin'.padEnd(28) + ' rows ' + CODES.map((c) => c.padStart(4)).join('') + '   total');
    const details = [];
    for (const name of plugins) {
        const r = await lintPlugin(name);
        if (r.error) { errors++; console.log(`  ${name.padEnd(28)} ERROR ${r.error}`); continue; }
        const per = Object.fromEntries(CODES.map((c) => [c, r.findings.filter((f) => f.code === c).length]));
        for (const c of CODES) totals[c] += per[c];
        const n = r.findings.length;
        if (n) failedPlugins++;
        sameAsEnTotal += r.info.sameAsEn.length; sameAsEnFlagged += r.info.sameAsEn.filter((x) => x.flagged).length; termNoteTotal += r.info.termNote.length;
        console.log(`  ${name.padEnd(28)} ${String(r.rows).padStart(4)} ` + CODES.map((c) => String(per[c] || '·').padStart(4)).join('') + `   ${String(n).padStart(5)}${n ? '' : '  ✓'}`);
        if (n || r.info.termNote.length || r.info.sameAsEn.some((x) => !x.flagged)) details.push(r);
    }
    console.log('  ' + '─'.repeat(28 + 6 + CODES.length * 4 + 8));
    console.log('  ' + 'TOTAL'.padEnd(28) + '      ' + CODES.map((c) => String(totals[c]).padStart(4)).join('') + `   ${String(Object.values(totals).reduce((a, b) => a + b, 0)).padStart(5)}`);

    for (const r of details) {
        console.log(`\n-- ${r.name}`);
        const shown = r.findings.slice(0, MAX_SHOWN);
        for (const f of shown) {
            const snip = f.fr.length > 90 ? f.fr.slice(0, 87) + '…' : f.fr;
            console.log(`  ${f.code}  ${f.kind.padEnd(5)} ${f.key.padEnd(34)} "${snip}"${f.note ? `   ← ${f.note}` : ''}`);
        }
        if (r.findings.length > shown.length) console.log(`  … ${r.findings.length - shown.length} more (--verbose)`);
        for (const t of r.info.termNote) console.log(`  EXEMPT (termNote) ${t.kind} ${t.key}: "${t.fr}" — ${t.frObj.termNote}`);
        for (const c of r.info.sameAsEn.filter((x) => !x.flagged)) console.log(`  INFO  straight copy, unflagged: ${c.kind} ${c.key}: "${c.fr}"`);
    }

    console.log(`\n-- summary`);
    console.log(`  plugins with findings: ${failedPlugins} / ${plugins.length}${errors ? `   (${errors} could not be read)` : ''}`);
    console.log(`  straight copies fr === en (info): ${sameAsEnTotal}, of which ${sameAsEnFlagged} are covered (sameAsEn: true, or a title over a translated body)   termNote exemptions (info): ${termNoteTotal}`);
    console.log(`  codes: T1 apostrophe  T2 decimal point  T3 % spacing  T4 colon  T5 ;!?  T6 minus  T7 unit  G1 glossary  C1 casing  F1 forbidden word`);
    const anyFail = failedPlugins > 0 || errors > 0;
    if (strict && anyFail) { console.log('\nSTRICT: failures present — exit 2'); process.exit(2); }
    console.log(strict ? '\nSTRICT: clean — exit 0' : '\nThis is a REPORT. Exit 0 means the run completed, not that the French is clean.');
})();
