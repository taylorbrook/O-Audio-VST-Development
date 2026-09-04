#!/usr/bin/env node
/**
 * gen-zh-trad-only.js — derive AND audit the rule Z3 Traditional-only set.
 *
 * The set lives as a literal in scripts/i18n-zh-lint.js between the two
 * ZH_TRAD_ONLY_SENTINEL markers. This script is the only thing that may write
 * it, and the only thing that can check it.
 *
 * WHY THIS IS COMMITTED. The first generator was kept as a throwaway in a task
 * scratchpad on the reasoning that the OUTPUT is the artifact and the fetch is
 * not a build step. That is exactly why an incorrect derivation survived: the
 * rule could not be re-read, so nobody could see that it flagged 像 (U+50CF),
 * the character in the glossary's only accepted rendering of `pan`. A derived
 * literal whose derivation is unreadable is a hand-written literal wearing a
 * provenance comment.
 *
 * Usage
 *   node scripts/gen-zh-trad-only.js --ts <TSCharacters.txt> --st <STCharacters.txt> \
 *        --target scripts/i18n-zh-lint.js
 *       Derives the set from the two OpenCC dictionaries, asserts the
 *       spotchecks, and rewrites ONLY the region between the sentinels.
 *
 *   node scripts/gen-zh-trad-only.js --verify scripts/i18n-zh-lint.js
 *       Re-reads the committed literal back out of the target, rebuilds the
 *       set, and asserts the same spotchecks. Needs no network and no
 *       dictionary files — this is the form a later reviewer can run.
 *       Exit 0 on pass, exit 1 naming every character on the wrong side.
 *
 * Fetch (never committed, never executed — only characters cross the boundary):
 *   curl -fsSL -o /tmp/TSCharacters.txt https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/TSCharacters.txt
 *   curl -fsSL -o /tmp/STCharacters.txt https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/STCharacters.txt
 */

'use strict';

const fs     = require('fs');
const path   = require('path');
const crypto = require('crypto');

const argv = process.argv.slice(2);
const val  = (k) => { const i = argv.indexOf(k); return i >= 0 ? argv[i + 1] : null; };

const TS_URL = 'https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/TSCharacters.txt';
const ST_URL = 'https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/STCharacters.txt';
const FETCHED = '2026-09-04';

const SENTINEL     = '// ZH_TRAD_ONLY_SENTINEL';
const SENTINEL_END = '// ZH_TRAD_ONLY_SENTINEL_END';

// ── the spotcheck, asserted in BOTH modes ───────────────────────────────────
// IN  — genuinely Traditional-only; Z3 must still fire on these.
// OUT — live simplified or ambiguous both-scripts characters; Z3 must be
//       silent. 声 and 像 together are 声像, the glossary's only accepted
//       rendering of `pan` (scripts/i18n-zh-glossary.js), inherited by
//       `pan rnd` / `pan spray` / `pan sync`. 乾 徵 於 夥 are four of the 18
//       characters that map to themselves in a LATER position of their own TS
//       value list — the ambiguous set that must not be flagged either.
const SPOT_IN  = ['這', '後', '說', '龍', '幾'];               // 這 後 說 龍 幾
const SPOT_OUT = ['这', '音', '声', '像',                          // 这 音 声 像
                  '乾', '徵', '於', '夥'];                          // 乾 徵 於 夥

const EXPECT_CHARS = 3198;
const EXPECT_BYTES = 9660;

const cp = (s) => `U+${s.codePointAt(0).toString(16).toUpperCase().padStart(4, '0')}`;

function die(msg) {
    console.error(`gen-zh-trad-only: ${msg}`);
    process.exit(1);
}

// ── parsing ─────────────────────────────────────────────────────────────────
// A key is the first TAB-separated field of every line that is non-empty and
// does NOT begin with '#'. THE COMMENT FILTER IS LOAD-BEARING: the served files
// carry a '#' header plus 895 '# @tofu-risk:' annotation lines, and counting
// those as keys inflates the set from 4139 to 5047.
function parseDict(file, label) {
    const text = fs.readFileSync(file, 'utf8');
    const map  = new Map();
    for (const raw of text.split('\n')) {
        const line = raw.replace(/\r$/, '');
        if (!line.trim() || line.startsWith('#')) continue;
        const fields = line.split('\t');
        const key    = fields[0];
        if (!key) continue;
        const values = (fields[1] || '').trim().split(/\s+/).filter(Boolean);
        map.set(key, values);
    }
    if (!map.size) die(`${label} (${file}) parsed to zero keys — the parse is wrong.`);
    return map;
}

function sha256(file) {
    return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
}

// ── the corrected rule ──────────────────────────────────────────────────────
// Traditional-only = a TS key that is NOT an ST key AND does not appear
// anywhere in its own TS value list.
//
// The third clause is the 2026-09-04 fix. A TS key whose value list contains
// itself survives T->S conversion unchanged, so it is a live simplified
// character no matter that the table has an entry for it. TSCharacters.txt
// line 418 reads `像<TAB>像 象`. Such keys never appear as ST keys, so the old
// two-clause rule could not see the class at all: 941 characters were in the
// set for exactly this reason.
//
// Exclude on self appearing ANYWHERE in the value list, not merely first. 923
// of the 941 map to themselves first; the other 18 (乾 剋 劄 吒 夥 徵 扞 於 昇
// 氾 祕 脩 蒐 薹 袷 谿 釐 陞) map to themselves in a later position and are the
// ambiguous both-scripts characters the rule already said must not be flagged.
function derive(ts, st) {
    const out = [];
    for (const [key, values] of ts) {
        if ([...key].length !== 1) {
            die(`TS key ${JSON.stringify(key)} is not a single code point — a `
                + `multi-character key in a CHARACTER table means the parse is wrong.`);
        }
        if (st.has(key)) continue;
        if (values.includes(key)) continue;
        out.push(key);
    }
    return out.sort((a, b) => a.codePointAt(0) - b.codePointAt(0));
}

// ── the spotcheck assertion, shared by both modes ───────────────────────────
function assertSpotcheck(set, what) {
    const wrong = [];
    for (const ch of SPOT_IN)  if (!set.has(ch)) wrong.push(`${ch} ${cp(ch)} should be IN the set but is OUT`);
    for (const ch of SPOT_OUT) if (set.has(ch))  wrong.push(`${ch} ${cp(ch)} should be OUT of the set but is IN`);
    if (wrong.length) {
        console.error(`\ngen-zh-trad-only: SPOTCHECK FAILED on ${what} — ${wrong.length} character(s) on the wrong side:`);
        for (const w of wrong) console.error(`  ${w}`);
        console.error('\nRefusing to proceed. A set that fails the spotcheck is never written and never accepted.');
        process.exit(1);
    }
}

const byteLen = (chars) => Buffer.byteLength(chars.join(''), 'utf8');

// ── literal emission ────────────────────────────────────────────────────────
function segments(chars, perSegment = 128) {
    const segs = [];
    for (let i = 0; i < chars.length; i += perSegment) {
        segs.push(chars.slice(i, i + perSegment).join(''));
    }
    return segs;
}

function renderBlock(chars, tsDigest, stDigest) {
    const segs = segments(chars);
    const lit  = segs.map((s) => `    '${s}'`).join(' +\n');
    return `${SENTINEL}
// PROVENANCE — this set is DERIVED, never hand written. Regenerate, do not edit.
//
//   generator scripts/gen-zh-trad-only.js — COMMITTED, and it audits its own
//             output offline via \`--verify\`. The first generator was a
//             scratchpad throwaway, and that is precisely why an incorrect
//             derivation survived unread until it flagged the glossary's own
//             rendering of \`pan\`; the decision is reversed here on purpose.
//   sources   ${TS_URL}
//             ${ST_URL}
//   sha256    TSCharacters.txt  ${tsDigest}
//             STCharacters.txt  ${stDigest}
//   fetched   ${FETCHED} (digests unchanged from the 2026-09-01 fetch — the
//             inputs did not drift; the RULE was wrong)
//   rule      Traditional-only = a TS key that is NOT an ST key AND does not
//             appear anywhere in its own TS value list.
//             A TS key is a character the Traditional->Simplified table has to
//             convert; if that same character is ALSO an ST key it is a live
//             simplified form too (or an ambiguous both-scripts character) and
//             must not be flagged. THE THIRD CLAUSE IS THE 2026-09-04 FIX: a TS
//             key whose value list contains ITSELF survives T->S conversion
//             unchanged, so it is a live simplified character as well, and such
//             keys never appear as ST keys — the old two-clause rule could not
//             see the class at all. It held 941 characters, among them 像
//             (U+50CF), whose TS line reads \`像<TAB>像 象\` and which is the
//             second character of 声像, the glossary's only accepted rendering
//             of \`pan\`. Self is excluded ANYWHERE in the value list, not
//             merely first: 18 characters (乾 剋 劄 吒 夥 徵 扞 於 昇 氾 祕 脩
//             蒐 薹 袷 谿 釐 陞) map to themselves in a later position and are
//             the ambiguous both-scripts set.
//   parsing   A key is the first TAB-separated field of every line that is
//             non-empty and does NOT begin with '#'; the values are the
//             whitespace-separated tokens of the second field. THE COMMENT
//             FILTER IS LOAD-BEARING: the served files carry a '#' header plus
//             895 '# @tofu-risk:' annotation lines, and counting those as keys
//             inflates the set from 4139 to 5047 and the literal from 12.5 KB
//             to 77 KB. The generator also asserts that every TS key is a
//             single code point and stops if one is not — a multi-character
//             key in a CHARACTER table means the parse is wrong.
//   size      ${chars.length} characters, ${byteLen(chars)} bytes UTF-8
//   spotcheck IN  這 後 說 龍 幾
//             OUT 这 音 声 像 乾 徵 於 夥
//             Asserted by the generator before it wrote this block, and
//             re-asserted against this committed literal by
//             \`node scripts/gen-zh-trad-only.js --verify scripts/i18n-zh-lint.js\`,
//             which stops rather than accept a set that fails it.
//
//   regenerate:
//     curl -fsSL -o /tmp/TSCharacters.txt ${TS_URL}
//     curl -fsSL -o /tmp/STCharacters.txt ${ST_URL}
//     node scripts/gen-zh-trad-only.js --ts /tmp/TSCharacters.txt --st /tmp/STCharacters.txt --target scripts/i18n-zh-lint.js
//
// The fetched files are never committed and never executed. Only these
// characters — inert data — cross the network-to-source boundary.
const TRAD_ONLY = new Set([...(
${lit}
)]);
${SENTINEL_END}`;
}

// ── reading the committed literal back out ──────────────────────────────────
// Anchored on the `new Set([...(` construct, NOT on the surrounding comment,
// which legitimately contains quoted text and example characters.
function readCommitted(file) {
    const text  = fs.readFileSync(file, 'utf8');
    const start = text.indexOf('new Set([...(');
    if (start < 0) die(`${file} contains no \`new Set([...(\` construct — nothing to verify.`);
    if (text.indexOf('new Set([...(', start + 1) >= 0) {
        die(`${file} contains more than one \`new Set([...(\` construct — refusing to guess which is the Z3 set.`);
    }
    const end = text.indexOf(')]);', start);
    if (end < 0) die(`${file} has an unterminated \`new Set([...(\` construct.`);
    const body = text.slice(start + 'new Set([...('.length, end);
    const segs = [...body.matchAll(/'([^']*)'/g)].map((m) => m[1]);
    if (!segs.length) die(`${file}: the Z3 literal holds no quoted segments.`);
    return [...segs.join('')];
}

// ── modes ───────────────────────────────────────────────────────────────────
function modeVerify(file) {
    if (!fs.existsSync(file)) die(`--verify: ${file} does not exist.`);
    const chars = readCommitted(file);
    const set   = new Set(chars);
    if (set.size !== chars.length) {
        die(`--verify: the committed literal holds ${chars.length} characters but only `
            + `${set.size} distinct ones — there are duplicates.`);
    }
    console.log(`gen-zh-trad-only --verify ${file}`);
    console.log(`  characters ${chars.length}`);
    console.log(`  bytes      ${byteLen(chars)} UTF-8`);
    assertSpotcheck(set, 'the committed literal');
    console.log(`  spotcheck  IN  ${SPOT_IN.join(' ')}  — all present`);
    console.log(`  spotcheck  OUT ${SPOT_OUT.join(' ')}  — all absent`);
    if (chars.length !== EXPECT_CHARS || byteLen(chars) !== EXPECT_BYTES) {
        console.log(`  NOTE: expected ${EXPECT_CHARS} characters / ${EXPECT_BYTES} bytes; `
            + `the set has moved. Confirm the dictionaries changed before accepting this.`);
    }
    console.log('  PASS');
}

function modeGenerate(tsFile, stFile, target) {
    for (const [f, what] of [[tsFile, '--ts'], [stFile, '--st'], [target, '--target']]) {
        if (!f) die(`${what} is required in generate mode.`);
        if (!fs.existsSync(f)) die(`${what}: ${f} does not exist.`);
    }
    const tsDigest = sha256(tsFile);
    const stDigest = sha256(stFile);
    console.log('gen-zh-trad-only — deriving the rule Z3 Traditional-only set');
    console.log(`  sha256 TSCharacters.txt  ${tsDigest}`);
    console.log(`  sha256 STCharacters.txt  ${stDigest}`);

    const ts = parseDict(tsFile, 'TSCharacters.txt');
    const st = parseDict(stFile, 'STCharacters.txt');
    console.log(`  TS keys ${ts.size}   ST keys ${st.size}`);

    const chars = derive(ts, st);
    const set   = new Set(chars);
    console.log(`  derived ${chars.length} characters, ${byteLen(chars)} bytes UTF-8`);

    // Assert BEFORE writing anything.
    assertSpotcheck(set, 'the derived set');
    console.log(`  spotcheck IN  ${SPOT_IN.join(' ')}  — all present`);
    console.log(`  spotcheck OUT ${SPOT_OUT.join(' ')}  — all absent`);

    if (chars.length !== EXPECT_CHARS || byteLen(chars) !== EXPECT_BYTES) {
        die(`derived ${chars.length} characters / ${byteLen(chars)} bytes, but `
            + `${EXPECT_CHARS} / ${EXPECT_BYTES} were predicted. Refusing to commit a set `
            + `nobody predicted — re-check the dictionaries, then update the expectation `
            + `deliberately if they really changed.`);
    }

    const text = fs.readFileSync(target, 'utf8');
    const occurrences = (needle) => {
        let n = 0, i = 0;
        for (;;) { const j = text.indexOf(needle, i); if (j < 0) break; n++; i = j + 1; }
        return n;
    };
    // SENTINEL is a prefix of SENTINEL_END, so count it by its own line.
    const openCount  = text.split('\n').filter((l) => l.trim() === SENTINEL).length;
    const closeCount = occurrences(SENTINEL_END);
    if (openCount !== 1) die(`${target}: found ${openCount} \`${SENTINEL}\` lines — expected exactly 1.`);
    if (closeCount !== 1) die(`${target}: found ${closeCount} \`${SENTINEL_END}\` markers — expected exactly 1.`);

    const startIdx = text.indexOf(SENTINEL);
    const endIdx   = text.indexOf(SENTINEL_END);
    if (endIdx < startIdx) die(`${target}: the end sentinel precedes the start sentinel.`);

    const rewritten = text.slice(0, startIdx)
        + renderBlock(chars, tsDigest, stDigest)
        + text.slice(endIdx + SENTINEL_END.length);
    fs.writeFileSync(target, rewritten, 'utf8');
    console.log(`  rewrote the sentinel region of ${path.relative(process.cwd(), target) || target}`);
    console.log('  DONE — now run: node scripts/gen-zh-trad-only.js --verify ' + target);
}

// ── main ────────────────────────────────────────────────────────────────────
const verifyTarget = val('--verify');
if (verifyTarget) {
    modeVerify(verifyTarget);
} else if (argv.includes('--ts') || argv.includes('--st') || argv.includes('--target')) {
    modeGenerate(val('--ts'), val('--st'), val('--target'));
} else {
    console.error('usage:');
    console.error('  node scripts/gen-zh-trad-only.js --ts <TSCharacters.txt> --st <STCharacters.txt> --target scripts/i18n-zh-lint.js');
    console.error('  node scripts/gen-zh-trad-only.js --verify scripts/i18n-zh-lint.js');
    process.exit(2);
}
