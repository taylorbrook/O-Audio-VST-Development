# emit_constants.py — Task 1 of band-limited-reader (O-GrainScatter 3.3.0 / grain-engine 1.1.0)
#
# Produces the hard-coded constants of modules/synthesis/grain-engine/cpp/BandlimitedReader.h:
#   1. the 127-tap Kaiser (beta 9.2) half-band: centre 0.5, even offsets exactly 0, the 32 odd
#      taps rescaled so they sum to 0.5 (DC exactly 1, Nyquist exactly 0);
#   2. per sinc set S0..S7 the (upper edge, fc, beta) of a 16-tap Kaiser-windowed sinc, designed
#      at the 44.1 kHz point (a = 20/44.1, the strictest), maximising the output -1 dB passband
#      with every audible spurious component <= -60 dB at the set's upper edge;
#   3. a predicted passband/stopband table per set.
#
# Run: python3 emit_constants.py   (numpy + scipy; ~1-2 min). Prints a C++ snippet at the end.
import numpy as np
from scipy import signal
import sys

T = 16
OS = 64
NFFT = 1 << 18
TARGET = -60.0
DESIGN_MARGIN = 0.5          # design to -60.5 so float rows + phase lerp keep the -60 verdict

# ── 1. half-band ─────────────────────────────────────────────────────────────
N = 127
BETA_HB = 9.2      # 9.0 gives -89.95 dB once the centre is pinned to 0.5; 9.2 gives -92.0
n = np.arange(N) - (N - 1) // 2
hb = 0.5 * np.sinc(n / 2) * np.kaiser(N, BETA_HB)
hb[(n % 2 == 0)] = 0.0
hb[(N - 1) // 2] = 0.5
odd = (n % 2 != 0)
hb[odd] *= 0.5 / hb[odd].sum()
w, H = signal.freqz(hb, worN=1 << 16, fs=1)
hb_rows = []
for fs in (44.1, 48.0, 96.0):
    delta = 0.5 * (0.5 - 20 / fs)
    sb = 20 * np.log10(np.abs(H[w >= 0.25 + delta]).max())
    hb_rows.append((fs, delta, sb))
even_ok = all(hb[k] == 0.0 for k in range(N) if (n[k] % 2 == 0 and n[k] != 0))
uniq = [hb[(N - 1) // 2 + 2 * m + 1] for m in range(32)]   # offsets +1, +3, ..., +63

# ── 2. sinc sets ─────────────────────────────────────────────────────────────
def kernel(fc, beta):
    t = np.arange(-T / 2 * OS, T / 2 * OS + 1) / OS
    wv = np.i0(beta * np.sqrt(np.clip(1 - (2 * t / T) ** 2, 0, 1))) / np.i0(beta)
    return t, 2 * fc * np.sinc(2 * fc * t) * wv

def resp(h):
    X = np.abs(np.fft.rfft(h, NFFT)); f = np.arange(len(X)) * OS / NFFT
    return f, X / X[0]

def worst_alias(f, X, r, a, fmax, npts=800, fmin=0.002, weight=None):
    """Worst audible spurious (dB re input) for tones f0 in [fmin, fmax] (level units) read at
    ratio r: every image m of the kernel response that folds below a, except the wanted
    m = 0 component when it lies below the output Nyquist. `weight(f0)` scales the tone
    (the half-band's own response for level >= 1 content above a)."""
    f0 = np.linspace(fmin, fmax, npts); worst = -300.0
    wt = np.ones_like(f0) if weight is None else weight(f0)
    for m in range(-12, 13):
        ff = np.abs(f0 + m); g = np.interp(ff, f, X) * wt
        out = np.abs(((ff * r + 0.5) % 1) - 0.5)
        mask = (out < a) & ~((m == 0) & (f0 * r <= 0.5))
        if mask.any():
            worst = max(worst, 20 * np.log10(g[mask].max() + 1e-30))
    return worst

def out_pb(f, X, r, a):
    """Output -1 dB passband (cycles/sample at the output) at ratio r."""
    i = np.argmax(20 * np.log10(X + 1e-30) < -1)
    g = min(f[i], a if r < 1 else 9)
    return g * r

A44 = 20 / 44.1
_whb, _Hhb = signal.freqz(hb, worN=1 << 16, fs=1)
def hb_weight(g):
    """Level >= 1 content at level frequency g is level-0 content at g/2 (parent units)
    through the half-band: real input in (a, 0.5] at the half-band's transition gain."""
    return np.interp(g / 2, _whb, np.abs(_Hhb))

def constraint(f, X, r, a, k):
    """S0: tones up to a (r' <= 1 never folds level content above a below a at r' = 1).
    S1..S7: also the level >= 1 transition band (a, 0.5], weighted by the half-band."""
    wa = worst_alias(f, X, r, a, a)
    if k >= 1:
        wa = max(wa, worst_alias(f, X, r, a, 0.5, npts=200, fmin=a, weight=hb_weight))
    return wa
edges = [1.0] + [2 ** (k / 6) for k in range(1, 7)] + [4.0]

def design(r_edge, a, target, k):
    best = None
    for beta in np.arange(3.0, 10.01, 0.25):
        lo, hi = 0.25, 1.1                     # fcn: fc = 0.5 * fcn / max(r, 1)
        f, X = resp(kernel(0.5 * lo / max(r_edge, 1), beta)[1])
        if constraint(f, X, r_edge, a, k) > target:
            continue
        for _ in range(22):
            mid = 0.5 * (lo + hi)
            f, X = resp(kernel(0.5 * mid / max(r_edge, 1), beta)[1])
            if constraint(f, X, r_edge, a, k) <= target: lo = mid
            else: hi = mid
        f, X = resp(kernel(0.5 * lo / max(r_edge, 1), beta)[1])
        p = out_pb(f, X, r_edge, a)
        if best is None or p > best[0]:
            best = (p, beta, lo)
    return best

def design_s7(a):
    """Best effort for r' in (2, 4): the same passband-maximising design as S1..S6, at an
    edge of 2^1.5 = 2.83 (16 taps cannot reach -60 dB at 4). -60 dB therefore holds to
    r' 2.83 (Doppler 1.41 at the top of an octave); (2.83, 4) is reported."""
    return design(2 ** 1.5, a, TARGET - DESIGN_MARGIN, 7)

sets = []
for k in range(7):
    r = edges[k]
    b = design(r, A44, TARGET - DESIGN_MARGIN, k)
    if b is None:
        print(f"S{k}: CANNOT reach {TARGET} dB with {T} taps at r'={r:.4f}"); sys.exit(1)
    p, beta, fcn = b
    sets.append(dict(k=k, edge=r, fc=0.5 * fcn / max(r, 1), beta=beta, fcn=fcn))
    print(f"S{k} edge {r:.5f}: fcn {fcn:.4f} beta {beta:.2f}  out -1 dB {p * 44.1:.2f} kHz @44.1", flush=True)
s7 = design_s7(A44)
sets.append(dict(k=7, edge=4.0, fc=0.5 * s7[2] / 2 ** 1.5, beta=s7[1], fcn=s7[2]))
print(f"S7 (best effort, designed at 2.83): fcn {s7[2]:.4f} beta {s7[1]:.2f}  out -1 dB {s7[0] * 44.1:.2f} kHz @44.1", flush=True)

# ── 3. tables ────────────────────────────────────────────────────────────────
print(f"\n=== Half-band (127 taps, Kaiser beta {BETA_HB}) ===")
print(f"centre = {hb[(N - 1) // 2]!r}, even offsets exactly 0: {even_ok}, DC = {hb.sum():.15f}, "
      f"Nyquist = {np.sum(hb * (-1.0) ** np.arange(N)):.3e}")
for fs, d, sb in hb_rows:
    print(f"  fs {fs:5.1f} k: delta {d:.4f}, stopband from {0.25 + d:.4f} = {sb:.1f} dB  [{'PASS' if sb <= -90 else 'FAIL'} <= -90]")

print("\n=== Sinc sets (16 taps), predicted ===")
print(" set  r' range            fc(lvl)  beta  | worst spurious dB over range (a=20/44.1) | out -1 dB kHz @44.1 lo..hi | @48 lo..hi")
ok_all = True
for s in sets:
    k = s['k']
    lo_r = 0.125 if k == 0 else edges[k - 1]
    hi_r = s['edge'] if k < 7 else 3.99
    f, X = resp(kernel(s['fc'], s['beta'])[1])
    grid = np.geomspace(lo_r * (1.0001 if k else 1), hi_r, 9)
    worst = max(constraint(f, X, r, A44, k) for r in grid)
    worst48 = max(worst_alias(f, X, r, 20 / 48, 20 / 48) for r in grid)
    full = max(worst_alias(f, X, r, A44, 0.5) for r in grid)   # unweighted content up to Nyquist (level 0 input above 20 kHz)
    pb_lo = out_pb(f, X, grid[0], A44); pb_hi = out_pb(f, X, hi_r, A44)
    s.update(worst=worst, pb_lo=pb_lo, pb_hi=pb_hi)
    verdict = ("PASS" if worst <= TARGET else "FAIL") if k < 7 else "report"
    if k < 7 and worst > TARGET: ok_all = False
    print(f"  S{k}  ({lo_r:.4f}, {hi_r:.4f}]  {s['fc']:.5f}  {s['beta']:4.2f} | {worst:6.1f} [{verdict}]  (48 k {worst48:6.1f}; content to Nyquist {full:6.1f}) "
          f"| {pb_lo * 44.1:5.1f} .. {pb_hi * 44.1:5.1f} | {pb_lo * 48:5.1f} .. {pb_hi * 48:5.1f}")
f, X = resp(kernel(sets[7]['fc'], sets[7]['beta'])[1])
print(f"  S7 achieved: r' 2.83 -> {constraint(f, X, 2 ** 1.5, A44, 7):.1f} dB, r' 3.99 -> {constraint(f, X, 3.99, A44, 7):.1f} dB")
f, X = resp(kernel(sets[0]['fc'], sets[0]['beta'])[1])
print("  S0 at r' < 1 on level >= 1 (Doppler < 1 on a latched level; half-band-weighted content above a):")
print("    " + "  ".join(f"r' {r:.2f}: {worst_alias(f, X, r, A44, 0.5, npts=200, fmin=A44, weight=hb_weight):6.1f}" for r in (0.5, 0.7, 0.85, 0.9, 0.95, 0.99)))

print("\n=== Parity: research run3.py settings reproduce RESEARCH §5 (unweighted, 300 tones, coarse grid, no margin) ===")
for r, ref in {1.0: 15.9, 1.414: 12.6, 1.99: 9.1}.items():
    best = None
    for beta in np.arange(4, 9.5, 0.5):
        for fcn in np.linspace(0.4, 1.1, 36):
            f, X = resp(kernel(0.5 * fcn / max(r, 1), beta)[1])
            if worst_alias(f, X, r, A44, A44, npts=300) <= -60:
                p = out_pb(f, X, r, A44)
                if best is None or p > best: best = p
    bp = None
    for beta in np.arange(4, 9.5, 0.5):
        for fcn in np.linspace(0.4, 1.1, 36):
            f, X = resp(kernel(0.5 * fcn / max(r, 1), beta)[1])
            if worst_alias(f, X, r, A44, A44, npts=300) <= -60 and abs(out_pb(f, X, r, A44) - best) < 1e-12:
                bp = worst_alias(f, X, r, A44, A44, npts=4000)
    print(f"  r' {r:5.3f}: research-optimal design re-checked on a 4000-tone grid: {bp:.1f} dB")
    print(f"  r' {r:5.3f}: {best * 44.1:5.2f} kHz vs research {ref:4.1f}  diff {best * 44.1 - ref:+.2f}  [{'PASS' if abs(best * 44.1 - ref) <= 0.3 else 'FAIL'} <= 0.3]", flush=True)

print("\n=== Passband vs RESEARCH §5 (T16 @44.1, output -1 dB kHz) ===")
research = {1.0: 15.9, 1.414: 12.6, 1.99: 9.1}
setof = {1.0: 0, 1.414: 3, 1.99: 6}
for r, ref in research.items():
    s = sets[setof[r]]
    f, X = resp(kernel(s['fc'], s['beta'])[1])
    p = out_pb(f, X, r, A44) * 44.1
    print(f"  r' {r:5.3f} (S{s['k']}): {p:5.2f} kHz vs research {ref:4.1f}  diff {p - ref:+.2f}  [{'PASS' if abs(p - ref) <= 0.3 else 'FAIL'} <= 0.3]")

print("\n=== C++ ===")
print("// Generated by research-scripts/emit_constants.py — do not edit by hand.")
print("// Half-band odd taps at centre offsets +1, +3, ..., +63 (mirror for negative offsets).")
print("static constexpr double kHalfbandOdd[32] = {")
for i in range(0, 32, 4):
    print("    " + ", ".join(f"{v:.17e}" for v in uniq[i:i + 4]) + ",")
print("};")
print("// Sinc sets: { upper edge r', fc (cycles per level sample), Kaiser beta }")
print("static constexpr SincSetDesign kSincSets[kNumSets] = {")
for s in sets:
    print(f"    {{ {s['edge']:.17g}, {s['fc']:.17g}, {s['beta']:.17g} }},   // S{s['k']}")
print("};")
print("\nALL S0-S6 PASS" if ok_all else "\nSOME SET FAILS")
