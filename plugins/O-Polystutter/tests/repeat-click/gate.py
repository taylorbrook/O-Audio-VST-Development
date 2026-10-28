# Pitched-down repeat click gate. Metric: worst |2nd difference| of the lane's wet
# output over the repeat region, normalised by a pure tone at the output pitch
# (A * w_out^2), so a clean repeat train reads ~1-few and a step reads ~1e3-1e4.
# Run: build.sh, then gate.py new (must ALL PASS) and gate.py old (v1.18.2 must FAIL).
# Verdict: pitched render vs the SAME-MOTION control (unity pitch at the same OUTPUT
# frequency, same trigger, subdivision, repeats, swing and reverse = the same repeat
# boundaries) <= 2.0x, worst over the frequency set on each side. Plus: unity with
# swing vs unity without (the swing pre-delay cut) <= 2.0x.
import itertools, math, os, subprocess, sys
import numpy as np
D = os.environ.get("OUT", os.path.join(os.environ.get("TMPDIR", "/tmp"), "prc")); FS = 48000.0; TRIG = 48000
eng = sys.argv[1]; fails = []
def render(p, f, sub, reps, rev, sw):
    o = os.path.join(D, "r", f"{eng}_{p}_{f}_{sub}_{reps}_{rev}_{sw}.f32"); os.makedirs(os.path.dirname(o), exist_ok=True)
    subprocess.run([os.path.join(D, f"probe_{eng}"), str(p), str(f), str(sub), str(reps), str(rev), str(sw), o], check=True)
    y = np.fromfile(o, dtype=np.float32).astype(np.float64); assert np.isfinite(y).all(); return y
def score(y, f, p):
    seg = y[TRIG - 4:]
    A = np.max(np.abs(seg)); w = 2 * math.pi * f * 2 ** (p / 12) / FS
    c = np.abs(np.diff(seg, 2)); n = int(np.argmax(c))
    return c[n] / (A * w * w), TRIG - 4 + n + 1
worst = {}
for p, f, sub, reps, rev, sw in itertools.product((-12, -7, -1, 5), (110.0, 233.0, 347.0), (2, 1), (4,), (0, 1), (0, 40)):
    r, at = score(render(p, f, sub, reps, rev, sw), f, p)
    fo = round(f * 2 ** (p / 12), 3)                              # same motion AND same output pitch, unity
    c, _ = score(render(0, fo, sub, reps, rev, sw), fo, 0)
    # the click metric of a linear fade's kink depends on the tone's phase where the
    # fade ends, so compare the worst over the same frequency set, not render pairs
    k = (p, rev, sw); w = worst.setdefault(k, [(0,), 0.0])
    w[0] = max(w[0], (r, at, f, sub)); w[1] = max(w[1], c)
for (p, rev, sw), ((r, at, f, sub), c) in sorted(worst.items()):
    ratio = r / c; ok = ratio <= 2.0
    print(f"{'PASS' if ok else 'FAIL'} pitch {p:+3d} st rev {rev} swing {sw:2d}: worst {ratio:9.1f}x vs same-motion unity "
          f"({r:9.1f} / {c:6.2f}, @{at} f={f} sub={sub})"); ok or fails.append((p, rev, sw))
# the swing pre-delay: unity with swing must click no worse than unity without
for rev in (0, 1):
    cs = {sw: max(score(render(0, f, sub, 4, rev, sw), f, 0)[0] for f in (110.0, 233.0, 347.0) for sub in (2, 1)) for sw in (0, 40)}
    ok = cs[40] / cs[0] <= 2.0
    print(f"{'PASS' if ok else 'FAIL'} unity swing 40 vs swing 0, rev {rev}: {cs[40] / cs[0]:7.1f}x ({cs[40]:.1f} / {cs[0]:.1f})"); ok or fails.append(("swing", rev))
print(f"\n{eng}: {'ALL PASS' if not fails else str(len(fails)) + ' FAIL'}")
