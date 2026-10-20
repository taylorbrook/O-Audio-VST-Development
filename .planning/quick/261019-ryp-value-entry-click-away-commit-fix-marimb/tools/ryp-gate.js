// ryp click-away static gate (quick 261019-ryp). Run from the repo root.
//
//   node .planning/quick/261019-ryp-value-entry-click-away-commit-fix-marimb/tools/ryp-gate.js <file>[=N] ...
//
// Per file, N = the number of value-entry sites. Default N = the number of
// createElement('input') calls left after comments are stripped. Append =N to a
// path only when the file also builds a NON-entry <input> (record why in SUMMARY).
//
// A file PASSES when it has exactly N document-level CAPTURE-phase pointerdown
// registrations whose handler (a) compares the event target with `!==` and
// (b) blurs the field (or calls a commit function), AND every such handler name
// is removed capture-phase at least as many times as it is added.
//
// It is a floor, not the proof: the headless probe (tools/ryp-probe.js) is the
// proof that the field actually commits and closes when another dial is grabbed.
// Minified bundles (O-TextureForge app.bundle.js) are not gated here; gate the
// source and let the probe exercise the bundle.
'use strict';
const fs = require('fs');

const blank = (s) => s.replace(/[^\n]/g, ' ');
const stripComments = (s) => s
    .replace(/\/\*[\s\S]*?\*\//g, blank)
    .replace(/(^|[^:'"\\])(\/\/.*)$/gm, (m, a, b) => a + blank(b));
const lineOf = (src, i) => src.slice(0, i).split('\n').length;
const CAP = String.raw`(?:true|!0|\{\s*capture\s*:\s*(?:true|!0)\s*\})`;
const addRe = () => new RegExp(String.raw`document\.addEventListener\(\s*['"]pointerdown['"]\s*,\s*([\w$]+)\s*,\s*${CAP}\s*\)`, 'g');
const remRe = () => new RegExp(String.raw`document\.removeEventListener\(\s*['"]pointerdown['"]\s*,\s*([\w$]+)\s*,\s*${CAP}\s*\)`, 'g');
const esc = (s) => s.replace(/[$]/g, '\\$');

const args = process.argv.slice(2);
if (!args.length) { console.log('usage: ryp-gate.js <file>[=N] ...'); process.exit(2); }
let fail = 0;

for (const a of args) {
    const m = a.match(/^(.*?)(?:=(\d+))?$/);
    const file = m[1];
    if (!fs.existsSync(file)) { fail++; console.log(`FAIL-NO-FILE ${file}`); continue; }
    const src = stripComments(fs.readFileSync(file, 'utf8'));
    const counted = (src.match(/createElement\(\s*['"]input['"]\s*\)/g) || []).length;
    const sites = m[2] !== undefined ? Number(m[2]) : counted;

    const isBlurHandler = (name) => {
        const def = new RegExp(String.raw`(?:\b(?:const|let|var)\s+${esc(name)}\s*=|\bfunction\s+${esc(name)}\s*\()`, 'g');
        let d;
        while ((d = def.exec(src))) {
            const body = src.slice(d.index, d.index + 320);
            if (/\.target\s*!==/.test(body) && /(\.blur\s*\(\s*\)|\bcommit\w*\s*\()/.test(body)) return true;
        }
        return false;
    };

    const adds = new Map(), rems = new Map();
    const other = [];
    let r, re = addRe();
    while ((r = re.exec(src))) {
        if (isBlurHandler(r[1])) adds.set(r[1], [...(adds.get(r[1]) || []), lineOf(src, r.index)]);
        else other.push(`${r[1]}@${lineOf(src, r.index)}`);
    }
    re = remRe();
    while ((r = re.exec(src))) {
        if (adds.has(r[1])) rems.set(r[1], (rems.get(r[1]) || 0) + 1);
    }
    const addCount = [...adds.values()].reduce((n, l) => n + l.length, 0);
    let ok = true;
    if (addCount !== sites) {
        ok = false;
        console.log(`FAIL-HANDLER-COUNT ${file}: entry sites=${sites}${m[2] !== undefined ? ` (override; counted ${counted})` : ''}, click-away handlers=${addCount}`);
    }
    for (const [name, lines] of adds) {
        const n = rems.get(name) || 0;
        if (n < lines.length) {
            ok = false;
            console.log(`FAIL-NOT-REMOVED ${file}: ${name} added ${lines.length}x (lines ${lines.join(',')}), removed ${n}x`);
        }
    }
    if (other.length) console.log(`NOTE ${file}: other document capture pointerdown listeners left alone: ${other.join(' ')}`);
    if (ok) console.log(`OK ${file}: entry sites=${sites}, click-away handlers=${addCount} (lines ${[...adds.values()].flat().join(',') || '-'})`);
    else fail++;
}
console.log(`ryp-gate: ${args.length} file(s), ${fail} failure(s)`);
process.exit(fail ? 1 : 0);
