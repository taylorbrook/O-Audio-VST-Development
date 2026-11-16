// Per-frame cost of the page's curve maths (evalBand x active bands + composeSum),
// on the XR-g9 packets with the most sections, at the 1600 px grid (800 points + bands).
'use strict';
const fs = require('fs'), path = require('path');
const [, , html, dir] = process.argv;
const src = fs.readFileSync(html, 'utf8');
const lift = (n) => { const i = src.indexOf('function ' + n + '('); let d = 0; for (let k = src.indexOf('{', i); ; k++) { if (src[k] === '{') d++; else if (src[k] === '}' && --d === 0) return src.slice(i, k + 1); } };
const P = new Function(`let displaySampleRate = 0; const bandChoice = (b) => 0;
${['evalBand', 'magDb', 'composeSum'].map(lift).join('\n')}
return { setRate: (r) => { displaySampleRate = r; }, evalBand, composeSum };`)();
const files = fs.readdirSync(dir).filter((f) => f.endsWith('.json')).map((f) => JSON.parse(fs.readFileSync(path.join(dir, f), 'utf8')));
const secs = (d) => d.packets.filter((p) => p.active).reduce((a, p) => a + p.sections.length, 0);
files.sort((a, b) => secs(b) - secs(a));
const d = files[0];
P.setRate(d.header.sampleRate);
const freqs = []; for (let i = 0; i <= 800; i++) freqs.push(10 * Math.pow(3000, i / 800));
const act = d.packets.filter((p) => p.active), n = freqs.length;
const times = [];
for (let it = 0; it < 300; it++) {
    const t0 = process.hrtime.bigint();
    const resp = new Map(), bands = [];
    for (const pk of act) { const R = { re: new Float64Array(n), im: new Float64Array(n) }; P.evalBand(pk, freqs, R.re, R.im); resp.set(pk.band + 1, R); bands.push(pk.band + 1); }
    P.composeSum(bands, resp, n, 0);
    times.push(Number(process.hrtime.bigint() - t0) / 1e6);
}
times.sort((a, b) => a - b);
console.log(`bands ${act.length}, sections ${secs(d)}, points ${n}: median ${times[150].toFixed(3)} ms, p99 ${times[297].toFixed(3)} ms (node ${process.version}, V8)`);
