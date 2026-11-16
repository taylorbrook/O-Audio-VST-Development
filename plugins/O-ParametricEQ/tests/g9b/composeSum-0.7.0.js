// composeSum as shipped in O-ParametricEQ 0.7.0 (Source/ui/public/index.html at
// a7067efe), BEFORE PLAN-3.2 P3. It reads each band's placement from the relay
// (bandChoice) instead of the packet. Kept verbatim as the negative half of
// tests/g9b control 5: fed relay placements the engine ignores on mono, this
// text must miss the C++ closed form by more than 0.05 dB, or the control
// cannot see the bug it guards against. Never edited; never shipped.
function composeSum(bands, responses, n, view) {
    const out = new Float64Array(n);
    for (let i = 0; i < n; i++) {
        // A = [[a, b], [c, d]], complex; starts as identity
        let ar = 1, ai = 0, br = 0, bi = 0, cr = 0, ci = 0, dr = 1, di = 0, valid = true;
        for (const b of bands) {
            const R = responses.get(b);
            if (!R) continue;
            const fr = R.re[i], fi = R.im[i];
            if (Number.isNaN(fr)) { valid = false; break; }
            // band matrix B = [[p, q], [r, s]]
            let pr, pi, qr, qi, rr, ri, sr, si;
            const hr = (fr - 1) / 2, hi = fi / 2;
            switch (bandChoice(b, 'placement')) {
                case 1:  pr = fr; pi = fi; qr = 0; qi = 0; rr = 0; ri = 0; sr = 1; si = 0; break;
                case 2:  pr = 1; pi = 0; qr = 0; qi = 0; rr = 0; ri = 0; sr = fr; si = fi; break;
                case 3:  pr = 1 + hr; pi = hi; qr = hr; qi = hi; rr = hr; ri = hi; sr = 1 + hr; si = hi; break;
                case 4:  pr = 1 + hr; pi = hi; qr = -hr; qi = -hi; rr = -hr; ri = -hi; sr = 1 + hr; si = hi; break;
                default: pr = fr; pi = fi; qr = 0; qi = 0; rr = 0; ri = 0; sr = fr; si = fi;
            }
            // A = B . A
            const nar = pr * ar - pi * ai + qr * cr - qi * ci, nai = pr * ai + pi * ar + qr * ci + qi * cr;
            const nbr = pr * br - pi * bi + qr * dr - qi * di, nbi = pr * bi + pi * br + qr * di + qi * dr;
            const ncr = rr * ar - ri * ai + sr * cr - si * ci, nci = rr * ai + ri * ar + sr * ci + si * cr;
            const ndr = rr * br - ri * bi + sr * dr - si * di, ndi = rr * bi + ri * br + sr * di + si * dr;
            ar = nar; ai = nai; br = nbr; bi = nbi; cr = ncr; ci = nci; dr = ndr; di = ndi;
        }
        if (!valid) { out[i] = NaN; continue; }
        let zr, zi;
        if (view === 1)      { zr = dr; zi = di; }
        else if (view === 2) { zr = (ar + br + cr + dr) / 2; zi = (ai + bi + ci + di) / 2; }
        else if (view === 3) { zr = (ar - br - cr + dr) / 2; zi = (ai - bi - ci + di) / 2; }
        else                 { zr = ar; zi = ai; }
        out[i] = magDb(zr, zi);
    }
    return out;
}
