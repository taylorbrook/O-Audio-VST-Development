// G9 (b) bench: the page's own evalBand / magDb / composeSum, lifted verbatim
// from Source/ui/public/index.html, against the C++ unit-circle closed form
// (composeFromPackets) on the XR-g9 draws. usage: node g9b.js <index.html> <dumpdir> [mode]
// mode: "packet" (compose with packet.active / packet.placement; default) or
//       "raw-mono" (mono files composed with a relay placement the engine ignores on mono: odd bands Left, even bands Right)
'use strict';
const fs = require('fs'), path = require('path');
const [, , html, dir, mode = 'packet'] = process.argv;
const src = fs.readFileSync(html, 'utf8');
function lift(name) {
    const i = src.indexOf('function ' + name + '(');
    if (i < 0) throw new Error('missing ' + name);
    let j = src.indexOf('{', i), d = 0;
    for (let k = j; k < src.length; k++) {
        if (src[k] === '{') d++;
        else if (src[k] === '}' && --d === 0) return src.slice(i, k + 1);
    }
    throw new Error('unbalanced ' + name);
}
const body = ['evalBand', 'magDb', 'composeSum'].map(lift).join('\n');
const make = new Function('ctx', `let displaySampleRate = 0; const bandChoice = (b, k) => ctx.placement(b);
${body}
return { setRate: (r) => { displaySampleRate = r; }, evalBand, magDb, composeSum };`);

const cppDb = (re, im) => 10 * Math.log10(Math.max(re * re + im * im, 1e-300));
let worst = 0, worstAll = 0, scored = 0, files = 0, worstAt = '', floorHits = 0, nanHits = 0;
const byRate = {};
for (const f of fs.readdirSync(dir).filter((x) => x.endsWith('.json')).sort()) {
    const d = JSON.parse(fs.readFileSync(path.join(dir, f), 'utf8'));
    const ctx = { placement: (b) => (mode === 'raw-mono' && !d.stereo ? (b % 2 ? 1 : 2) : d.packets[b - 1].placement) };
    const P = make(ctx);
    P.setRate(d.header.sampleRate);
    const n = d.grid.length, bands = [], responses = new Map();
    for (const pk of d.packets) {
        if (!pk.active) continue;
        const b = pk.band + 1;   // the editor sends 1-based
        bands.push(b);
        const R = { re: new Float64Array(n), im: new Float64Array(n) };
        P.evalBand(pk, d.grid, R.re, R.im);
        responses.set(b, R);
    }
    const views = d.stereo ? 4 : 1;
    const js = [], cpp = [];
    let top = -Infinity;
    for (let v = 0; v < views; v++) {
        js.push(P.composeSum(bands, responses, n, v));
        cpp.push(d.cpp.map((c) => cppDb(c[2 * v], c[2 * v + 1])));
        for (let i = 0; i < 192; i++) top = Math.max(top, cpp[v][i]);   // floor from the 192-point grid, as XR-g9
    }
    files++;
    for (let v = 0; v < views; v++) for (let i = 0; i < n; i++) {
        const a = js[v][i], c = cpp[v][i];
        if (Number.isNaN(a)) { nanHits++; continue; }
        if (a === -200) floorHits++;
        const e = Math.abs(a - c);
        if (c >= top - 80 && c >= -120) {
            scored++;
            if (e > worst) { worst = e; worstAt = `${f} view ${v} f ${d.grid[i].toFixed(3)} Hz cpp ${c.toFixed(4)} js ${a.toFixed(4)}`; }
            const r = d.header.sampleRate;
            byRate[r] = Math.max(byRate[r] || 0, e);
        }
        if (c >= -120) worstAll = Math.max(worstAll, e);
    }
}
console.log(JSON.stringify({ mode, files, scored, worstScoredDb: worst, worstAt, worstByRate: byRate, worstAboveMinus120Db: worstAll, floorHits, nanHits }, null, 1));
