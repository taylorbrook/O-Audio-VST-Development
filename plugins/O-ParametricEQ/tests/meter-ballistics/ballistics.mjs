// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Meter ballistics gate (PLAN-3.3 T9a; P5): the page's OWN meterStep, lifted
// by name from the shipping index.html with brace matching, run at 60 Hz and
// 120 Hz frame rates.
//
//   node tests/meter-ballistics/ballistics.mjs <index.html>
//
// PASS needs all of:
//   - vacuity: function meterStep found exactly once;
//   - attack: a step from -60 to -6 reads -6 on the first frame (both rates);
//   - release: after a drop -6 -> -60 the level at 0.25 / 0.5 / 1 s is within
//     0.1 dB between 60 and 120 Hz, and within 0.1 dB of -6 - 20 t;
//   - peak: holds -6 until 1.0 s (+- one frame), then falls at 20 dB/s
//     (-16 +- 0.1 dB at 1.5 s), at both rates; never below the level;
//   - negative control: the lifted text mutated to a per-frame decay (the
//     level's 20 x dt replaced by 20 / 60 per frame) differs by > 1 dB between
//     the two rates at 0.5 s.
//
// Exit 0 PASS, 1 FAIL, 2 usage.

import fs from 'node:fs';
import path from 'node:path';

const [, , htmlPath] = process.argv;
if (!htmlPath) { console.error('usage: node ballistics.mjs <index.html>'); process.exit(2); }

const failures = [];
const check = (ok, what) => { console.log(`  [${ok ? 'PASS' : 'FAIL'}] ${what}`); if (!ok) failures.push(what); };

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
const copies = liftAll(page, 'meterStep');
check(copies.length === 1, `vacuity: function meterStep found exactly once in ${path.basename(htmlPath)} (found ${copies.length})`);
if (failures.length) { console.log('\nmeter ballistics FAIL'); process.exit(1); }

const build = (text) => new Function(`${text}\nreturn meterStep;`)();
const ship = build(copies[0]);

const LEVEL_TOKEN = 'state.level - rate * dt';
check(copies[0].split(LEVEL_TOKEN).length === 2, `vacuity: the control finds "${LEVEL_TOKEN}" exactly once`);
const perFrame = build(copies[0].replace(LEVEL_TOKEN, 'state.level - rate / 60'));

const near = (a, b, tol) => Math.abs(a - b) <= tol;
const fmt = (v) => v.toFixed(3);

// Runs a sequence: settled at `from` (level and peak, hold 0), then target
// `to` for `seconds` at `hz`. Returns per-frame states with their times.
function run(step, hz, from, to, seconds) {
    const dt = 1 / hz;
    let s = { level: from, peak: from, hold: 0 };
    const out = [];
    for (let i = 1; i <= Math.round(seconds * hz); i++) {
        s = step(s, to, dt);
        out.push({ t: i * dt, ...s });
    }
    return out;
}
const at = (seq, hz, t) => seq[Math.round(t * hz) - 1];

// Attack
for (const hz of [60, 120]) {
    const s = ship({ level: -60, peak: -60, hold: 0 }, -6, 1 / hz);
    check(s.level === -6 && s.peak === -6, `attack ${hz} Hz: -60 -> -6 in one frame (level ${fmt(s.level)}, peak ${fmt(s.peak)})`);
}

// Release and peak
const seq = { 60: run(ship, 60, -6, -60, 3), 120: run(ship, 120, -6, -60, 3) };
for (const t of [0.25, 0.5, 1.0]) {
    const a = at(seq[60], 60, t).level, b = at(seq[120], 120, t).level, want = Math.max(-60, -6 - 20 * t);
    check(near(a, b, 0.1) && near(a, want, 0.1) && near(b, want, 0.1),
          `release at ${t} s: 60 Hz ${fmt(a)}, 120 Hz ${fmt(b)}, want ${fmt(want)} (+- 0.1 dB)`);
}
for (const hz of [60, 120]) {
    const dt = 1 / hz, q = seq[hz];
    const held = q.filter((s) => s.t <= 1.0 - dt + 1e-9).every((s) => s.peak === -6);
    const firstFall = q.find((s) => s.peak < -6);
    const p15 = at(q, hz, 1.5).peak;
    const neverBelow = q.every((s) => s.peak >= s.level);
    check(held && firstFall && near(firstFall.t, 1.0, dt + 1e-9) && near(p15, -16, 0.1) && neverBelow,
          `peak ${hz} Hz: held -6 to ${fmt(firstFall ? firstFall.t - dt : NaN)} s, first fall at ${fmt(firstFall ? firstFall.t : NaN)} s, ${fmt(p15)} dB at 1.5 s (want -16), never below the level: ${neverBelow}`);
}

// Negative control: a per-frame decay is rate-dependent.
{
    const a = at(run(perFrame, 60, -6, -60, 1), 60, 0.5).level;
    const b = at(run(perFrame, 120, -6, -60, 1), 120, 0.5).level;
    check(Math.abs(a - b) > 1, `control (per-frame decay): 60 Hz ${fmt(a)} vs 120 Hz ${fmt(b)} at 0.5 s differ by > 1 dB (fires)`);
}

console.log(failures.length ? `\nmeter ballistics FAIL (${failures.length})` : '\nmeter ballistics PASS');
process.exit(failures.length ? 1 : 0);
