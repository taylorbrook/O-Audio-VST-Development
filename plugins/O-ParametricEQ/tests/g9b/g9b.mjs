// SPDX-License-Identifier: AGPL-3.0-or-later
//
// G9 (b) gate (PLAN-3.2 P4; RESEARCH-3.2 Q2): the page's OWN response maths
// against the C++ closed form.
//
//   node tests/g9b/g9b.mjs <index.html> <dumpdir>
//
// <dumpdir> is written by `O-ParametricEQ-render-harness --dump-g9 <dumpdir>`:
// one JSON per XR-g9 draw (250 stereo sNNN, 10 mono mNNN) holding the
// processor's DisplayModel packets, the header, a grid (192 log points + each
// active band's frequency) and `cpp`, composeFromPackets on the unit circle
// (LL, RR, MM, SS as re / im pairs).
//
// evalBand, magDb and composeSum are LIFTED BY NAME from the shipping page with
// brace matching: the gate runs the text that ships, never a copy. Packets go
// in 1-based, as the editor sends them; all 24 bands are offered to composeSum,
// which composes those whose packet is `active` with the packet's effective
// placement (P3). The relay placement the page could read instead is stubbed
// to a value the engine ignores on mono (odd bands Left, even Right).
//
// PASS needs all of:
//   - worst |JS - C++| <= 0.01 dB at the scored points (XR-g9's floors: within
//     80 dB of the largest maximum among the views, and >= -120 dB);
//   - vacuity guards: each lifted function found exactly once, >= 100 000
//     scored points, every rate (44.1 / 48 / 88.2 / 96 / 192 kHz) and the mono
//     set contribute points, no NaN below Nyquist;
//   - negative controls, each worst > 0.05 dB: (1) k x 1.01 in every SVF
//     section, (2) onePoles dropped, (3) the first Left and Mid bands swapped
//     in order, (4) Side composed with the Mid matrix (one-token mutation of
//     the lifted composeSum), (5) relay placement on mono: the shipping
//     composeSum's result is IDENTICAL with the relay stub and with the packet
//     placement, while the 0.7.0 composeSum (composeSum-0.7.0.js) fed the same
//     stub misses by > 0.05 dB, (6) the `active` filter removed.
//
// Exit 0 PASS, 1 FAIL, 2 usage / input error.

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const GATE_DB = 0.01, CONTROL_DB = 0.05, MIN_SCORED = 100000;
const RATES = [44100, 48000, 88200, 96000, 192000];

const [, , htmlPath, dumpDir] = process.argv;
if (!htmlPath || !dumpDir) {
    console.error('usage: node g9b.mjs <index.html> <dumpdir>');
    process.exit(2);
}

const here = path.dirname(fileURLToPath(import.meta.url));
const failures = [];
const check = (ok, what) => { console.log(`  [${ok ? 'PASS' : 'FAIL'}] ${what}`); if (!ok) failures.push(what); };

// ── Lift ────────────────────────────────────────────────────────────────────
function liftAll(src, name) {
    const found = [];
    const needle = 'function ' + name + '(';
    for (let i = src.indexOf(needle); i >= 0; i = src.indexOf(needle, i + 1)) {
        const j = src.indexOf('{', i);
        let d = 0, end = -1;
        for (let k = j; k < src.length; k++) {
            if (src[k] === '{') d++;
            else if (src[k] === '}' && --d === 0) { end = k; break; }
        }
        if (end < 0) throw new Error(`unbalanced braces in ${name}`);
        found.push(src.slice(i, end + 1));
    }
    return found;
}

const page = fs.readFileSync(htmlPath, 'utf8');
const lifted = {};
for (const name of ['evalBand', 'magDb', 'composeSum']) {
    const copies = liftAll(page, name);
    check(copies.length === 1, `vacuity: function ${name} found exactly once in ${path.basename(htmlPath)} (found ${copies.length})`);
    lifted[name] = copies[0] || '';
}
const old = liftAll(fs.readFileSync(path.join(here, 'composeSum-0.7.0.js'), 'utf8'), 'composeSum');
check(old.length === 1, 'vacuity: the 0.7.0 composeSum fixture holds exactly one composeSum');
if (failures.length) { console.log('\nG9 (b) FAIL'); process.exit(1); }

// Control 4: Side composed with the Mid matrix. One token per sign, on the one
// Side line of the lifted text; the line must exist exactly once.
const SIDE = 'case 4:  pr = 1 + hr; pi = hi; qr = -hr; qi = -hi; rr = -hr; ri = -hi;';
const SIDE_AS_MID = 'case 4:  pr = 1 + hr; pi = hi; qr = hr; qi = hi; rr = hr; ri = hi;';
check(lifted.composeSum.split(SIDE).length === 2, 'vacuity: control 4 finds the Side matrix line exactly once');
const sideAsMid = lifted.composeSum.replace(SIDE, SIDE_AS_MID);

// Control 6: the `active` filter removed (inactive bands composed too).
const ACTIVE = 'if (pk && pk.active && R)';
check(lifted.composeSum.split(ACTIVE).length === 2, 'vacuity: control 6 finds the active filter exactly once');
const noActive = lifted.composeSum.replace(ACTIVE, 'if (pk && R)');

// The page's globals the lifted text reads: displaySampleRate, bandPackets
// (1-based) and bandChoice (relay reads; stubbed).
function build(composeText) {
    return new Function('ctx', `let displaySampleRate = 0;
const bandPackets = ctx.bandPackets;
const bandChoice = (b, key) => ctx.relay(b, key);
${lifted.evalBand}
${lifted.magDb}
${composeText}
return { setRate: (r) => { displaySampleRate = r; }, evalBand, magDb, composeSum };`);
}
const variants = {
    ship: build(lifted.composeSum),
    sideAsMid: build(sideAsMid),
    noActive: build(noActive),
    old070: build(old[0]),
};

// ── Data ────────────────────────────────────────────────────────────────────
const files = fs.existsSync(dumpDir) ? fs.readdirSync(dumpDir).filter((f) => /^[sm]\d{3}\.json$/.test(f)).sort() : [];
check(files.length === 260, `vacuity: 260 draws in the dump (found ${files.length})`);
const draws = files.map((f) => ({ name: f, ...JSON.parse(fs.readFileSync(path.join(dumpDir, f), 'utf8')) }));

const cppDb = (re, im) => 10 * Math.log10(Math.max(re * re + im * im, 1e-300));
const relayStub = (d) => (b, key) => {
    if (key !== 'placement') throw new Error(`composeSum read relay ${key}`);
    return d.stereo ? d.packets[b - 1].placement : (b % 2 ? 1 : 2);   // mono: L / R, which the engine ignores
};

// One draw through one variant. opts: mutate (packets -> packets), order
// (bands -> bands), activeOnly (offer only active bands: the 0.7.0 text has
// no active filter of its own), relay (placement source).
function run(d, variant, opts = {}) {
    const packets = opts.mutate ? opts.mutate(structuredClone(d.packets)) : d.packets;
    const bandPackets = new Array(25).fill(null);
    for (const pk of packets) bandPackets[pk.band + 1] = { ...pk, band: pk.band + 1 };
    const P = variants[variant]({ bandPackets, relay: opts.relay || relayStub(d) });
    P.setRate(d.header.sampleRate);
    const n = d.grid.length, responses = new Map();
    let bands = [];
    for (let b = 1; b <= 24; b++) {
        if (opts.activeOnly && !bandPackets[b].active) continue;
        const R = { re: new Float64Array(n), im: new Float64Array(n) };
        P.evalBand(bandPackets[b], d.grid, R.re, R.im);
        responses.set(b, R);
        bands.push(b);
    }
    if (opts.order) bands = opts.order(bands, bandPackets);
    const views = d.stereo ? 4 : 1, out = [];
    for (let v = 0; v < views; v++) out.push(P.composeSum(bands, responses, n, v));
    return out;
}

// Worst error of one variant over a set of draws, at XR-g9's scored points.
function score(set, variant, opts) {
    let worst = 0, scored = 0, nan = 0, at = '';
    const byRate = {}, pointsByRate = {};
    let monoPoints = 0;
    for (const d of set) {
        const js = run(d, variant, opts);
        const views = js.length;
        const cpp = [];
        let top = -Infinity;
        for (let v = 0; v < views; v++) {
            cpp.push(d.cpp.map((c) => cppDb(c[2 * v], c[2 * v + 1])));
            for (let i = 0; i < 192; i++) top = Math.max(top, cpp[v][i]);
        }
        for (let v = 0; v < views; v++) for (let i = 0; i < d.grid.length; i++) {
            const a = js[v][i], c = cpp[v][i];
            if (Number.isNaN(a)) { nan++; continue; }
            if (c < top - 80 || c < -120) continue;
            const e = Math.abs(a - c);
            scored++;
            const r = d.header.sampleRate;
            pointsByRate[r] = (pointsByRate[r] || 0) + 1;
            if (!d.stereo) monoPoints++;
            byRate[r] = Math.max(byRate[r] || 0, e);
            if (e > worst) { worst = e; at = `${d.name} view ${v} ${d.grid[i].toFixed(3)} Hz: C++ ${c.toFixed(4)} dB, page ${a.toFixed(4)} dB`; }
        }
    }
    return { worst, scored, nan, at, byRate, pointsByRate, monoPoints };
}

// ── The gate ────────────────────────────────────────────────────────────────
console.log(`G9 (b): ${path.relative(process.cwd(), htmlPath)} vs ${draws.length} XR-g9 draws`);
const main = score(draws, 'ship');
for (const r of RATES)
    console.log(`    ${r} Hz  worst ${(main.byRate[r] ?? NaN).toExponential(2)} dB over ${main.pointsByRate[r] || 0} points`);
console.log(`    worst at ${main.at}`);
check(main.worst <= GATE_DB, `G9 (b) worst ${main.worst.toExponential(3)} dB <= ${GATE_DB} dB`);
check(main.scored >= MIN_SCORED, `vacuity: ${main.scored} scored points >= ${MIN_SCORED}`);
check(RATES.every((r) => (main.pointsByRate[r] || 0) > 0), 'vacuity: every rate contributes points');
check(main.monoPoints > 0, `vacuity: the mono set contributes points (${main.monoPoints})`);
check(main.nan === 0, `no NaN below Nyquist (${main.nan})`);

const mono = draws.filter((d) => !d.stereo), stereo = draws.filter((d) => d.stereo);

// Controls 1 - 4 and 6: each must miss by more than 0.05 dB.
const controls = [
    ['1 k x 1.01 in every SVF section', draws, 'ship', {
        mutate: (pks) => { for (const pk of pks) pk.sections = pk.sections.map((s) => [s[0], s[1] * 1.01, s[2], s[3], s[4]]); return pks; } }],
    ['2 onePoles dropped', draws, 'ship', {
        mutate: (pks) => { for (const pk of pks) pk.onePoles = []; return pks; } }],
    ['3 first Left and Mid bands swapped in order', stereo, 'ship', {
        order: (bands, bp) => {
            const run = bands.filter((b) => bp[b].active);
            const l = run.find((b) => Number(bp[b].placement) === 1), m = run.find((b) => Number(bp[b].placement) === 3);
            if (!l || !m) return run;
            const out = run.slice(), il = out.indexOf(l), im = out.indexOf(m);
            out[il] = m; out[im] = l;
            return out;
        } }],
    ['4 Side composed with the Mid matrix', draws, 'sideAsMid', {}],
    ['6 the active filter removed', draws, 'noActive', {}],
];
for (const [name, set, variant, opts] of controls) {
    const s = score(set, variant, opts);
    check(s.worst > CONTROL_DB, `control ${name}: worst ${s.worst.toFixed(3)} dB > ${CONTROL_DB} dB (${s.scored} points)`);
}

// Control 5: relay placement on mono (RESEARCH-3.2 finding 3).
{
    let maxDiff = 0, compared = 0;
    for (const d of mono) {
        const withStub = run(d, 'ship');
        const withPacket = run(d, 'ship', { relay: (b) => d.packets[b - 1].placement });
        for (let v = 0; v < withStub.length; v++) for (let i = 0; i < d.grid.length; i++) {
            const a = withStub[v][i], b = withPacket[v][i];
            if (Number.isNaN(a) && Number.isNaN(b)) continue;
            maxDiff = Math.max(maxDiff, Number.isNaN(a) || Number.isNaN(b) ? Infinity : Math.abs(a - b));
            compared++;
        }
    }
    check(compared > 0 && maxDiff === 0,
        `control 5a: on the ${mono.length} mono draws the shipping composeSum ignores relay placement (max diff ${maxDiff} dB over ${compared} points)`);
    const before = score(mono, 'old070', { activeOnly: true });
    check(before.worst > CONTROL_DB,
        `control 5b: the 0.7.0 composeSum fed the same relay placements misses by ${before.worst.toFixed(3)} dB > ${CONTROL_DB} dB (${before.at})`);
}

console.log(failures.length ? `\nG9 (b) FAIL (${failures.length})` : '\nG9 (b) PASS');
process.exit(failures.length ? 1 : 0);
