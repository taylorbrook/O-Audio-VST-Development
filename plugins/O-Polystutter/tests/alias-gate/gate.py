# O-Polystutter alias gate (v1.19.0): pitched-up repeats read the grain-engine
# BandlimitedReader mipmap instead of the linear read.
#
# Run build.sh, then `python3 gate.py` (new must ALL PASS; the old-engine controls
# are part of the run). One RepeatLane offline (probe.cpp), wet output only.
#
#   1  alias: a sine whose transposed frequency f0 * r is past Nyquist, so the ideal
#      output is silence. Audible (20 Hz - 20 kHz) energy re the same sine at unity,
#      over 8192 samples of the first repeat's interior (Blackman-Harris).
#      Verdict new <= -60 dB at 44.1 and 48 kHz, every mip level and both edges of
#      each octave; control: v1.18.3 must FAIL every row (else the gate is blind).
#   2  in-band: f0 * r = 1 kHz and 9 kHz. Wanted level within +-1 dB of unity and
#      spurious <= -60 dB (new). Old printed.
#   3  bit-identity vs v1.18.3 wherever the ratio is <= 1 (unity, pitched down,
#      reverse, swing, pitch random <= 0, retriggers, noise input): must be equal.
#      Control: a pitched-up config must differ.
#   4  block-size null (new): random pitch -12..+12 and +12..+24 per repeat,
#      reverse, swing, retriggers; blocks 1 / 32 / 480 / variable: bit-identical.
#   5  cost (REPORT): ns per sample at +24 st, new vs old.
import itertools, math, os, subprocess, sys
import numpy as np
D = os.environ.get("OUT", os.path.join(os.environ.get("TMPDIR", "/tmp"), "pag"))
R = os.path.join(D, "r"); os.makedirs(R, exist_ok=True)
fails = []
def check(name, ok, detail): print(("PASS " if ok else "FAIL ") + name + "  " + detail); ok or fails.append(name)

def render(eng, fs, bpm, sub, reps, pitch, rnd, rev, sw, inp, blk, trig, n):
    key = "_".join(str(x) for x in (eng, fs, bpm, sub, reps, pitch, rnd, rev, sw, inp, blk, trig.replace(",", "-"), n)).replace(":", "~")
    o = os.path.join(R, key + ".f32")
    p = subprocess.run([os.path.join(D, f"probe_{eng}"), str(fs), str(bpm), str(sub), str(reps), str(pitch), rnd,
                        str(rev), str(sw), inp, str(blk), trig, str(n), o], check=True, capture_output=True, text=True)
    y = np.fromfile(o, dtype=np.float32).reshape(-1, 2).astype(np.float64)
    assert np.isfinite(y).all()
    return y, float(p.stderr.split()[-1])

def bh(N):
    k = np.arange(N); a = (0.35875, 0.48829, 0.14128, 0.01168)
    return a[0] - a[1] * np.cos(2 * np.pi * k / N) + a[2] * np.cos(4 * np.pi * k / N) - a[3] * np.cos(6 * np.pi * k / N)
N = 8192; W = bh(N)
def spectrum(y, T, fs):
    seg = y[T + 1024:T + 1024 + N, 0] * W
    P = np.abs(np.fft.rfft(seg)) ** 2
    f = np.fft.rfftfreq(N, 1 / fs)
    return f, P
def split(y, T, fs, want):
    f, P = spectrum(y, T, fs)
    aud = (f >= 20) & (f <= 20000)
    near = np.zeros_like(aud)
    if want < fs / 2: near = np.abs(f - want) <= 7 * fs / N
    return P[aud & ~near].sum(), P[near].sum(), P.sum()
def db(x): return 10 * math.log10(x) if x > 0 else -400.0
def pitch_args(st):
    # base pitch <= 12; the rest through pitch random (min = max, unquantized)
    base = min(st, 12.0); extra = round(st - base, 4)
    return base, ("none" if extra == 0 else str(extra))

T = 88200
def tone(eng, fs, st, f0, rev=0):
    base, rnd = pitch_args(st)
    y, ns = render(eng, fs, 60, 0, 1, base, rnd, rev, 0, f"sine:{f0}", 512, str(T if fs == 44100 else 96000), (T if fs == 44100 else 96000) + fs)
    return y, (T if fs == 44100 else 96000)

print("== 1. alias: transposed sine past Nyquist, audible energy re unity (verdict new <= -60 dB; old must FAIL)")
# (st, f0 at 44.1k). Levels: <+12 L0 (S1..S6), +12..+24 L1, +24 L2 (S0).
ALIAS = [(1.0, 20000), (5.0, 19500), (7.0, 18000), (11.9, 13500), (12.0, 13500), (12.1, 13500), (17.0, 10500),
         (19.0, 9000), (23.9, 7000), (24.0, 7000)]
for fs in (44100, 48000):
    for (st, f0), rev in itertools.product(ALIAS, (0, 1)):
        if rev and st not in (7.0, 12.1, 23.9, 24.0): continue
        if fs == 48000: f0 = min(19800.0, f0 * 1.09)
        r = 2 ** (st / 12); want = f0 * r
        if want <= fs / 2: f0 = (fs / 2 + 1500) / r; want = f0 * r
        ref = None; row = {}
        for eng in ("new", "old"):
            yu, Tu = tone(eng, fs, 0.0, f0)
            ref = split(yu, Tu, fs, f0)[2]
            y, Ty = tone(eng, fs, st, f0, rev)
            row[eng] = db(split(y, Ty, fs, want)[0] / ref)
        ok = row["new"] <= -60.0
        check(f"alias {fs} +{st:4.1f} st rev {rev} f0 {f0:7.0f} -> {want/1000:5.2f}k", ok and row["old"] > -60.0,
              f"new {row['new']:7.1f} dB   old {row['old']:7.1f} dB" + ("" if row["old"] > -60 else "  (control did not fail)"))

print("== 2. in-band: wanted level re unity within +-1 dB, spurious <= -60 dB (new)")
for fs in (44100,):
    for st, fout in itertools.product((5.0, 7.0, 12.1, 19.0, 24.0), (1000.0, 9000.0)):
        r = 2 ** (st / 12); f0 = fout / r; res = {}
        for eng in ("new", "old"):
            yu, Tu = tone(eng, fs, 0.0, fout)
            _, nu, tu = split(yu, Tu, fs, fout)
            y, Ty = tone(eng, fs, st, f0)
            sp, nr, tot = split(y, Ty, fs, fout)
            res[eng] = (db(nr / nu), db(sp / tu))
        ok = abs(res["new"][0]) <= 1.0 and res["new"][1] <= -60.0
        check(f"in-band +{st:4.1f} st -> {fout/1000:4.1f}k", ok,
              f"new level {res['new'][0]:+6.2f} dB spurious {res['new'][1]:7.1f} dB | old level {res['old'][0]:+6.2f} dB spurious {res['old'][1]:7.1f} dB")

print("== 3. bit-identity vs v1.18.3 where ratio <= 1 (noise input, retriggers, block 256)")
TR = "30000,61000,200000"
BIT = [(0, "none", 0, 0, 2), (-5, "none", 0, 0, 2), (-12, "none", 1, 0, 1), (-7, "none", 1, 40, 2), (0, "none", 1, 40, 1),
       (0, "-12:0", 0, 0, 2), (-3, "-9:0", 1, 25, 1), (12, "-12", 0, 0, 2)]
for p, rnd, rev, sw, sub in BIT:
    a, _ = render("old", 44100, 120, sub, 4, p, rnd, rev, sw, "noise", 256, TR, 300000)
    b, _ = render("new", 44100, 120, sub, 4, p, rnd, rev, sw, "noise", 256, TR, 300000)
    check(f"bit-identical pitch {p:+d} rand {rnd:>6} rev {rev} swing {sw:2d} sub {sub}",
          np.array_equal(a, b) and np.abs(a).max() > 0.05, f"max |diff| {np.abs(a - b).max():.3g}, peak {np.abs(a).max():.3f}")
a, _ = render("old", 44100, 120, 2, 4, 7, "none", 0, 0, "noise", 256, TR, 300000)
b, _ = render("new", 44100, 120, 2, 4, 7, "none", 0, 0, "noise", 256, TR, 300000)
check("control: pitched-up (+7) differs from v1.18.3", np.abs(a - b).max() > 1e-3, f"max |diff| {np.abs(a - b).max():.3g}")

print("== 4. block-size null (new): blocks 1 / 32 / 480 / var, must be bit-identical")
for p, rnd, rev, sw in ((0, "-12:12", 0, 30), (0, "-12:12", 1, 0), (12, "0:12", 0, 0), (12, "0:12", 1, 30), (12, "12", 1, 0)):
    rs = {b: render("new", 48000, 120, 1, 8, p, rnd, rev, sw, "noise", b, "20000,47000,150000,151000", 260000)[0] for b in ("1", "32", "480", "var")}
    worst = max(np.abs(rs[x] - rs[y]).max() for x, y in itertools.combinations(rs, 2))
    old, _ = render("old", 48000, 120, 1, 8, p, rnd, rev, sw, "noise", "480", "20000,47000,150000,151000", 260000)
    check(f"null pitch {p:+d} rand {rnd:>6} rev {rev} swing {sw:2d}", worst == 0.0 and np.abs(rs["480"]).max() > 0.05,
          f"worst |diff| {worst:.3g}; differs from v1.18.3 by {np.abs(rs['480'] - old).max():.3g}")

print("== 5. cost (REPORT): ns per processed sample, one lane at +24 st, 1/16 x 16 repeats, retriggered")
for eng in ("old", "new"):
    ts = [render(eng, 48000, 120, 2, 16, 12, "12", 0, 0, "noise", 512, ",".join(str(48000 * k + 7) for k in range(1, 20)), 1000000 + k)[1] for k in range(3)]
    print(f"   {eng}: {min(ts):.1f} ns/sample")

print(f"\n{'ALL PASS' if not fails else str(len(fails)) + ' FAIL: ' + ', '.join(fails)}")
sys.exit(1 if fails else 0)
