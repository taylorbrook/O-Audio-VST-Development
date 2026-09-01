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

    i18n-zh-lint.js — Simplified Chinese typography and terminology, across
    every plugin.

    ── REPORT TODAY, GATE LATER ──────────────────────────────────────────────

    This tool ships as a REPORT: it exits 0 regardless of what it finds. It is
    promoted to a GATE — exit 2 on any finding — only once the Stage 2 pilot
    (O-Chorus) is at zero findings. That is the exact lifecycle
    scripts/i18n-fr-lint.js went through: report-only on the day 43 of 43
    plugins failed it, a gate on 2026-08-31 once the rollout had taken every
    plugin to 0. Shipping it as a gate on day one would let a half-built lint
    block Stage 2, which is the failure this ordering exists to avoid.

    WHAT IT IS. check-i18n.js proves the MECHANISM: every key resolves, every
    entry carries a reviewed flag, the canon has not drifted. It says nothing
    about whether the Chinese is any good. This file checks the part of "good"
    that a regex can see, so the part a person has to read is the only part
    left for the person.

    ── The checks ────────────────────────────────────────────────────────────

      Z1  punctuation    ASCII , . : ; ? ! ( ) inside Han prose — zh takes the
                         full-width forms. ASCII punctuation INSIDE a Latin or
                         unit token is legal and is masked out first.
      Z2  no U+00A0      a no-break space before : ; ! ? % — the deliberate
                         INVERSE of French T3/T4/T5. The full-width forms carry
                         their own half-em sidebearing; adding U+00A0 doubles it.
      Z3  variant        a Traditional-only character in a zh-Hans table. The
                         set is DERIVED from OpenCC dictionary data, never hand
                         written — see the provenance block below.
      Z4  Latin/CJK      spacing between a Latin/digit run and a Han run is one
                         plain U+0020 everywhere or nowhere; INCONSISTENCY is
                         the finding, not either form. A thin space (U+2009 /
                         U+200A) fires unconditionally — it has no glyph in some
                         of the faces this suite ships and would render as a box
                         where no gate looks. Same reasoning that chose U+00A0
                         over U+202F for French.
      Z5  glossary       a LABEL or tooltip TITLE whose English is a TERMS key
                         in scripts/i18n-zh-glossary.js renders as one of its
                         accepted zh-Hans forms. The French G1 analogue.
      Z6  budget         a rendering longer than its MEASURED character budget
                         (maxChars = floor(cellWidthPx / fontSizePx)). A term
                         with no measured cell is UNBUDGETED and Z6 is inert on
                         it — the summary block prints how many, because an
                         inert rule that does not announce itself is the "gate
                         green on unchecked content" failure this rollout exists
                         to prevent.
      Z7  full-width     full-width Latin letters or digits (ＬＦＯ, ２) — a
                         classic machine-translation artifact.
      F1  forbidden      a rendering from FORBIDDEN_IN_LABELS in a label or
                         title, or from FORBIDDEN_IN_PROSE in a body.
      R1  reviewed       a zh-Hans entry whose `reviewed` is absent or is not
                         one of 'mt' | 'bt' | 'native'. An entry at 'mt' is
                         reported separately as BELOW SHIP BAR (info) — that
                         disclosure is the value this rule adds over
                         check-i18n.js assertion [5], which only asks that the
                         flag be present.

    NOT PORTED, deliberately. French T1-T7 are French typography and Z2 is the
    exact inverse of three of them; porting them would be actively wrong, not
    merely useless. C1 (casing) has no zh analogue at all — Han has no case and
    `text-transform: uppercase` is a no-op on it.

    INFO, never a finding: entries marked `sameAsEn: true`, and entries carrying
    a `termNote` (a reasoned glossary exemption). A termNote exempts the entry
    from BOTH term rules — Z5 and F1 — exactly as it does in French.
    I18N_EXEMPT stays check-i18n.js's concern; this lint neither reads nor
    re-interprets it.

    Usage:
        node scripts/i18n-zh-lint.js                    # all plugins, report
        node scripts/i18n-zh-lint.js --plugin O-Chorus  # one plugin
        node scripts/i18n-zh-lint.js --verbose          # every finding, not 12
        node scripts/i18n-zh-lint.js --codes            # the rule codes, one line
        node scripts/i18n-zh-lint.js --self-test        # prove each rule fires

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const { pathToFileURL } = require('url');
const G = require(path.join(__dirname, 'i18n-zh-glossary.js'));

const LANG  = 'zh-Hans';
const CODES = ['Z1', 'Z2', 'Z3', 'Z4', 'Z5', 'Z6', 'Z7', 'F1', 'R1'];
const REVIEWED_ENUM = ['mt', 'bt', 'native'];

const argv    = process.argv.slice(2);
const val     = (k) => { const i = argv.indexOf(k); return i >= 0 ? argv[i + 1] : null; };
const only    = val('--plugin');
const verbose = argv.includes('--verbose');
const MAX_SHOWN = verbose ? Infinity : 12;

const ROOT = path.resolve(__dirname, '..');
const UI_ROOTS = ['Source/ui/public/js/i18n.js', 'Resources/ui/js/i18n.js'];

// ── Traditional-only character set (rule Z3) ────────────────────────────────
// ZH_TRAD_ONLY_SENTINEL — replaced in Task 2 by a generated literal derived
// from OpenCC dictionary data. Until then Z3 is declared and inert.
const TRAD_ONLY = new Set();
// ZH_TRAD_ONLY_SENTINEL_END

// ── normalisation ───────────────────────────────────────────────────────────
// The English side is normalised exactly as the French lint normalises it, so
// the two glossaries stay interchangeable for Stage-2 tooling.
function normEn(s) {
    return String(s).replace(/’/g, "'").replace(/[   ]/g, ' ')
        .trim().replace(/\s+/g, ' ').replace(/\.$/, '').toLowerCase();
}
// The Chinese side drops a trailing period in EITHER script — a caption written
// with the ideographic full stop is the same caption.
function normZh(s) {
    return String(s).replace(/[   ]/g, ' ').trim().replace(/\s+/g, ' ')
        .replace(/[.。．]$/, '').toLowerCase();
}

const TERMS = Object.fromEntries(Object.entries(G.TERMS).map(([k, v]) => [normEn(k), v]));
const BUDGETS = Object.fromEntries(Object.entries(G.BUDGETS).map(([k, v]) => [normEn(k), v]));

const HAN = /\p{Script=Han}/u;

// ── Z1 ──────────────────────────────────────────────────────────────────────
// Mask every Latin/unit token before looking for ASCII punctuation. This mask
// is the rule's whole difficulty: `延迟 (delay) 20 ms` is legal and must stay
// silent, while `混音, 深度.` must fire.
function maskLatin(s) {
    let out = String(s);
    // A parenthesised aside with no Han inside is a Latin gloss; the parens
    // belong to it and go with it.
    out = out.replace(/\(([^()]*)\)/g, (m, inner) => (HAN.test(inner) ? m : ' '));
    // A Latin/number token, including any ASCII punctuation BETWEEN two
    // alphanumerics (1.5, kHz/ms, don't, 20-40) and a trailing percent.
    out = out.replace(/[A-Za-z0-9]+(?:[.,:'’\/\-][A-Za-z0-9]+)*%?/g, ' ');
    return out;
}
const ASCII_PUNCT = /[,.:;?!()]/;
function ruleZ1(zh) {
    if (!HAN.test(zh)) return false;   // a pure-Latin entry is not Han prose
    return ASCII_PUNCT.test(maskLatin(zh));
}

// ── Z2 ──────────────────────────────────────────────────────────────────────
function ruleZ2(zh) {
    return / [:;!?%：；！？％]/.test(zh);
}

// ── Z3 ──────────────────────────────────────────────────────────────────────
// Code-point iteration. CJK extension characters are surrogate pairs and
// charAt/index iteration splits them in half.
function ruleZ3(zh) {
    if (!TRAD_ONLY.size) return null;   // inert until the set is generated
    const hits = [...String(zh)].filter((ch) => TRAD_ONLY.has(ch));
    return hits.length ? [...new Set(hits)] : null;
}

// ── Z4 ──────────────────────────────────────────────────────────────────────
// Every boundary between a Latin/digit run and a Han run, classified by the
// gap between them.
function boundaryKinds(s) {
    const kinds = [];
    const re = /(?:[A-Za-z0-9]([    ]*)\p{Script=Han})|(?:\p{Script=Han}([    ]*)[A-Za-z0-9])/gu;
    let m;
    while ((m = re.exec(s)) !== null) {
        const gap = m[1] !== undefined ? m[1] : m[2];
        if (/[  ]/.test(gap)) kinds.push('thin');
        else if (gap === '') kinds.push('none');
        else if (gap === ' ') kinds.push('space');
        else kinds.push('other');
        re.lastIndex = m.index + 1;   // boundaries may overlap
    }
    return kinds;
}

// ── Z6 ──────────────────────────────────────────────────────────────────────
function ruleZ6(en, zh) {
    const b = BUDGETS[normEn(en)];
    if (!b || typeof b.maxChars !== 'number') return null;   // UNBUDGETED: inert
    const n = G.charCount(zh);
    return n > b.maxChars ? { n, max: b.maxChars } : null;
}

// ── Z7 ──────────────────────────────────────────────────────────────────────
function ruleZ7(zh) {
    // Full-width digits, upper- and lower-case Latin. NOT full-width
    // punctuation — that is the correct form and Z1 exists to require it.
    return /[０-９Ａ-Ｚａ-ｚ]/.test(zh);
}

// ── F1 ──────────────────────────────────────────────────────────────────────
// Chinese has no word delimiter, so containment is the only available test —
// the French stem/lookahead machinery has nothing to anchor to here.
function forbidden(zh, table) {
    const hay = normZh(zh);
    return Object.keys(table).filter((w) => hay.includes(normZh(w)));
}

// ── row extraction ──────────────────────────────────────────────────────────
function rowsFromModule(m) {
    const rows = [];
    for (const [k, v] of Object.entries(m.LABELS || {}))
        rows.push({ kind: 'label', key: k, en: v.en?.t ?? '', zh: v[LANG]?.t ?? '', zhObj: v[LANG] || null });
    for (const [k, v] of Object.entries(m.I18N || {})) {
        rows.push({ kind: 'title', key: k, en: v.en?.t ?? '', zh: v[LANG]?.t ?? '', zhObj: v[LANG] || null });
        if ((v.en?.b ?? '') !== '' || (v[LANG]?.b ?? '') !== '')
            rows.push({ kind: 'body', key: k, en: v.en?.b ?? '', zh: v[LANG]?.b ?? '', zhObj: v[LANG] || null, isBody: true });
    }
    return rows;
}

// ── the lint proper ─────────────────────────────────────────────────────────
// Rows in, findings out. Every entry point — a real plugin, a self-test fixture
// — goes through this one function, so a rule proven by the self-test is the
// same code that runs on the corpus.
function lintRows(rows) {
    const findings = [];
    const info = { sameAsEn: [], termNote: [], mt: [] };
    const withBoundaries = [];

    for (const r of rows) {
        if (!r.zh) continue;
        const exempt = !!(r.zhObj && typeof r.zhObj.termNote === 'string' && r.zhObj.termNote.trim() !== '');

        if (ruleZ1(r.zh)) findings.push({ code: 'Z1', ...r, note: 'ASCII punctuation in Han prose' });
        if (ruleZ2(r.zh)) findings.push({ code: 'Z2', ...r, note: 'U+00A0 before punctuation; zh full-width forms carry their own sidebearing' });
        const z3 = ruleZ3(r.zh);
        if (z3) findings.push({ code: 'Z3', ...r, note: `Traditional-only: ${z3.join(' ')}` });
        if (ruleZ7(r.zh)) findings.push({ code: 'Z7', ...r, note: 'full-width Latin or digits' });

        const kinds = boundaryKinds(r.zh);
        if (kinds.includes('thin'))
            findings.push({ code: 'Z4', ...r, note: 'thin space (U+2009/U+200A) between Latin and Han — no glyph in some faces' });
        if (kinds.length) withBoundaries.push({ row: r, kinds });

        if (r.isBody) {
            for (const w of forbidden(r.zh, G.FORBIDDEN_IN_PROSE))
                findings.push({ code: 'F1', ...r, note: `"${w}" -> ${G.FORBIDDEN_IN_PROSE[w]}` });
            continue;
        }

        // labels and tooltip titles from here on
        if (String(r.zh).trim() === String(r.en).trim())
            info.sameAsEn.push({ ...r, flagged: r.zhObj?.sameAsEn === true });
        if (exempt) info.termNote.push(r);

        const allowed = TERMS[normEn(r.en)];
        const accepted = (allowed || []).map(normZh).includes(normZh(r.zh));
        if (!exempt && allowed && !accepted)
            findings.push({ code: 'Z5', ...r, note: `"${r.en}" -> ${allowed.join(' | ')}` });
        // A rendering the glossary itself ACCEPTS for this English can never be
        // a forbidden rendering — the French precedent, kept verbatim.
        if (!exempt && !accepted)
            for (const w of forbidden(r.zh, G.FORBIDDEN_IN_LABELS))
                findings.push({ code: 'F1', ...r, note: `"${w}" -> ${G.FORBIDDEN_IN_LABELS[w]}` });

        const z6 = ruleZ6(r.en, r.zh);
        if (z6) findings.push({ code: 'Z6', ...r, note: `${z6.n} characters over a measured budget of ${z6.max}` });

        // R1 is entry-scoped: evaluated on labels and titles, never twice for
        // the title/body pair of one tooltip.
        const rev = r.zhObj ? r.zhObj.reviewed : undefined;
        if (!REVIEWED_ENUM.includes(rev))
            findings.push({ code: 'R1', ...r, note: `reviewed=${JSON.stringify(rev)} — must be 'mt' | 'bt' | 'native'` });
        else if (rev === 'mt') info.mt.push(r);
    }

    // Z4's consistency half is TABLE-scoped: one form or the other, across the
    // whole table. Neither form is wrong on its own.
    const counts = { space: 0, none: 0 };
    for (const w of withBoundaries)
        for (const k of w.kinds) if (k in counts) counts[k]++;
    if (counts.space > 0 && counts.none > 0) {
        const minority = counts.none <= counts.space ? 'none' : 'space';
        for (const w of withBoundaries)
            if (w.kinds.includes(minority))
                findings.push({ code: 'Z4', ...w.row,
                    note: `Latin/Han spacing is inconsistent in this table (${counts.space} spaced, ${counts.none} unspaced); this entry uses the minority form "${minority}"` });
    }

    return { findings, info, zhEntries: rows.filter((r) => !r.isBody && r.zhObj).length };
}

async function lintPlugin(name) {
    const rel = UI_ROOTS.find((r) => fs.existsSync(path.join(ROOT, 'plugins', name, r)));
    if (!rel) return { name, error: 'no i18n.js under either UI root' };
    let m;
    try { m = await import(pathToFileURL(path.join(ROOT, 'plugins', name, rel)).href); }
    catch (e) { return { name, error: `import failed: ${String(e.message).split('\n')[0]}` }; }
    const rows = rowsFromModule(m);
    return { name, rows: rows.length, ...lintRows(rows) };
}

// ── self-test ───────────────────────────────────────────────────────────────
// A rule that cannot be SHOWN to fire on a deliberate violation is not
// implemented — it is decorative. Every rule declares its own violation/control
// pair here. Fixtures that need a plugin-shaped ESM module are written to
// os.tmpdir() at run time and deleted; nothing is ever written under plugins/
// and no fixture is committed.
const MOD = (labels, i18n = '{}') =>
    `export const LANGUAGES = ['en', 'fr', 'zh-Hans'];\n`
    + `export const LABELS = ${labels};\n`
    + `export const I18N = ${i18n};\n`;

const SELF_TESTS = {
    Z5: {
        why: 'a glossary term rendered as something the glossary does not accept',
        violation: { module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '强度', reviewed: 'bt' } } }`) },
        control:   { module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '深度', reviewed: 'bt' } } }`) },
    },
    R1: {
        why: "a zh-Hans entry with no reviewed flag, or one outside 'mt'|'bt'|'native'",
        violation: { module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '深度' } } }`) },
        control:   { module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '深度', reviewed: 'bt' } } }`) },
    },
};

async function materialise(spec) {
    if (spec.rows) return spec.rows;
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'zh-lint-fixture-'));
    const file = path.join(dir, 'i18n.mjs');
    try {
        fs.writeFileSync(file, spec.module, 'utf8');
        const m = await import(pathToFileURL(file).href);
        return rowsFromModule(m);
    } finally {
        fs.rmSync(dir, { recursive: true, force: true });
    }
}

async function selfTest() {
    console.log('i18n-zh-lint --self-test — every rule against a deliberate violation and a clean control\n');
    let proven = 0;
    for (const code of CODES) {
        const t = SELF_TESTS[code];
        if (!t) { console.log(`  SELF-TEST ${code} NOT YET IMPLEMENTED — declared, inert, and not counted`); continue; }
        const vio = lintRows(await materialise(t.violation)).findings.some((f) => f.code === code);
        const ctl = lintRows(await materialise(t.control)).findings.some((f) => f.code === code);
        if (vio && !ctl) { proven++; console.log(`  SELF-TEST ${code} FIRES on violation, silent on control   (${t.why})`); }
        else {
            const which = !vio && ctl ? 'silent on the violation AND fires on the control'
                : !vio ? 'silent on the violation' : 'fires on the control';
            console.log(`  SELF-TEST ${code} BROKEN: ${which}`);
        }
    }
    console.log(`\nSELF-TEST: ${proven}/${CODES.length}`);
}

// ── main ────────────────────────────────────────────────────────────────────
(async () => {
    if (argv.includes('--codes')) { console.log(CODES.join(' ')); return; }
    if (argv.includes('--self-test')) { await selfTest(); return; }

    const plugins = fs.readdirSync(path.join(ROOT, 'plugins'))
        .filter((n) => n.startsWith('O-') && fs.statSync(path.join(ROOT, 'plugins', n)).isDirectory())
        .filter((n) => !only || n === only).sort();
    if (!plugins.length) { console.error(`i18n-zh-lint: no plugin matches --plugin ${only}`); return; }

    console.log('i18n-zh-lint — Simplified Chinese typography and terminology');
    console.log(`  plugins: ${plugins.length}   (REPORT: exits 0 whatever it finds; becomes a gate once the O-Chorus pilot is at zero)\n`);

    const totals = Object.fromEntries(CODES.map((c) => [c, 0]));
    let failedPlugins = 0, errors = 0, zhTotal = 0, sameAsEnTotal = 0, termNoteTotal = 0, mtTotal = 0;

    console.log('  ' + 'plugin'.padEnd(28) + ' rows   zh ' + CODES.map((c) => c.padStart(4)).join('') + '   total');
    const details = [];
    for (const name of plugins) {
        const r = await lintPlugin(name);
        if (r.error) { errors++; console.log(`  ${name.padEnd(28)} ERROR ${r.error}`); continue; }
        const per = Object.fromEntries(CODES.map((c) => [c, r.findings.filter((f) => f.code === c).length]));
        for (const c of CODES) totals[c] += per[c];
        const n = r.findings.length;
        if (n) failedPlugins++;
        zhTotal += r.zhEntries;
        sameAsEnTotal += r.info.sameAsEn.length; termNoteTotal += r.info.termNote.length; mtTotal += r.info.mt.length;
        console.log(`  ${name.padEnd(28)} ${String(r.rows).padStart(4)} ${String(r.zhEntries).padStart(4)} `
            + CODES.map((c) => String(per[c] || '·').padStart(4)).join('') + `   ${String(n).padStart(5)}`);
        if (n || r.info.termNote.length) details.push(r);
    }
    console.log('  ' + '─'.repeat(28 + 11 + CODES.length * 4 + 8));
    console.log('  ' + 'TOTAL'.padEnd(28) + '      ' + String(zhTotal).padStart(4) + ' '
        + CODES.map((c) => String(totals[c]).padStart(4)).join('')
        + `   ${String(Object.values(totals).reduce((a, b) => a + b, 0)).padStart(5)}`);

    for (const r of details) {
        console.log(`\n-- ${r.name}`);
        const shown = r.findings.slice(0, MAX_SHOWN);
        for (const f of shown) {
            const snip = f.zh.length > 90 ? f.zh.slice(0, 87) + '…' : f.zh;
            console.log(`  ${f.code}  ${f.kind.padEnd(5)} ${f.key.padEnd(34)} "${snip}"${f.note ? `   ← ${f.note}` : ''}`);
        }
        if (r.findings.length > shown.length) console.log(`  … ${r.findings.length - shown.length} more (--verbose)`);
        for (const t of r.info.termNote) console.log(`  EXEMPT (termNote) ${t.kind} ${t.key}: "${t.zh}" — ${t.zhObj.termNote}`);
    }

    console.log(`\n-- summary`);
    if (zhTotal === 0) {
        // A vacuity result is NOT a pass, and this tool says so in its own
        // words. There is no success banner on this branch by design.
        console.log(`  VACUITY: 0 zh-Hans entries found across ${plugins.length} plugins — nothing was checked.`);
        console.log(`  This is not a pass. It is the correct Stage 1 result: the rollout has not yet written any Chinese.`);
    } else {
        console.log(`  zh-Hans entries checked: ${zhTotal}   plugins with findings: ${failedPlugins} / ${plugins.length}${errors ? `   (${errors} could not be read)` : ''}`);
    }
    console.log(`  straight copies zh === en (info): ${sameAsEnTotal}   termNote exemptions (info): ${termNoteTotal}`);
    console.log(`  BELOW SHIP BAR — entries at reviewed:'mt' (machine draft, unchecked): ${mtTotal}`);
    console.log(`  codes: Z1 ASCII punctuation  Z2 U+00A0 before punctuation  Z3 Traditional-only  Z4 Latin/CJK spacing  Z5 glossary  Z6 budget  Z7 full-width Latin  F1 forbidden  R1 reviewed enum`);
    console.log(`\nREPORT ONLY — exit 0. This becomes a gate (exit 2) once the O-Chorus pilot is at zero findings.`);
})();
