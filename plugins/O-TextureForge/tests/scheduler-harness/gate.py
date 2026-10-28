import itertools, os, subprocess, sys
import numpy as np

# v1.6.0 gate: block-size null (32 / 512 / variable, fresh instances, <= -120 dBFS),
# bit-identity vs the baseline where its onsets were already exact, and controls.
H = os.environ.get("OUT", os.path.join(os.environ.get("TMPDIR", "/tmp"), "tfh"))
OUT = os.path.join(H, "renders"); os.makedirs(OUT, exist_ok=True)
fails = []

def render(eng, mode, dens, var, seed, block, align, path, tail=0):
    f = os.path.join(OUT, f"{eng}_{mode}_{dens}_{var}_{seed}_{block}_{align}_{path}_{tail}.f32")
    subprocess.run([os.path.join(H, f"tfh_{eng}"), str(mode), str(dens), str(var), str(seed), str(block),
                    str(align), path, f, str(tail)], check=True)
    return np.fromfile(f, dtype=np.float32)

def db(x): return 20 * np.log10(x) if x > 0 else -np.inf
def resid(a, b): return db(float(np.max(np.abs(a.astype(np.float64) - b.astype(np.float64)))))
def check(name, ok, detail): print(("PASS " if ok else "FAIL ") + name + "  " + detail); ok or fails.append(name)

SEED = 0x5EED1234
print("== A. block-size null, fresh instances (new engine), residual must be <= -120 dBFS")
cfgs = [(2, 8, 0.4, "kd"), (2, 37, 0.4, "kd"), (2, 64, 0.4, "kd"), (2, 37, 0.4, "proj"), (2, 37, 0.7, "rand"),
        (0, 8, 0.4, "kd"), (1, 8, 0.4, "kd"), (0, 8, 0.6, "rand")]
for mode, dens, var, path in cfgs:
    for tail in (0, 1):
        r = {b: render("new", mode, dens, var, SEED, b, 0, path, tail) for b in ("32", "512", "var")}
        peak = db(float(np.max(np.abs(r["512"]))))
        worst = max(resid(r[a], r[b]) for a, b in itertools.combinations(r, 2))
        check(f"null mode{mode} d{dens} v{var} {path} tail{tail}", worst <= -120 and peak > -40,
              f"worst residual {worst:.1f} dBFS, signal peak {peak:.1f} dBFS")

print("== B. bit-identity vs v1.5.4 where onsets were already exact (variation 0)")
for mode, align, block in ((0, 512, "512"), (1, 512, "512"), (0, 32, "32"), (2, 882, "882")):
    for path in ("kd", "proj"):
        a = render("old", mode, 64, 0.0, SEED, block, align, path)
        b = render("new", mode, 64, 0.0, SEED, block, align, path)
        check(f"bit-identical mode{mode} block{block} {path}", np.array_equal(a, b) and np.max(np.abs(a)) > 0.01,
              f"max |diff| {np.max(np.abs(a - b)):.3g}, peak {np.max(np.abs(a)):.3f}")

print("== C. controls (each must DIFFER, else the gates above are vacuous)")
o32 = render("old", 2, 37, 0.0, SEED, "32", 0, "kd"); o512 = render("old", 2, 37, 0.0, SEED, "512", 0, "kd")
check("old engine fails the null (drone 32 vs 512)", resid(o32, o512) > -60, f"{resid(o32, o512):.1f} dBFS")
o32 = render("old", 0, 8, 0.0, SEED, "32", 0, "kd"); o512 = render("old", 0, 8, 0.0, SEED, "512", 0, "kd")
check("old engine fails the null (MIDI free events 32 vs 512)", resid(o32, o512) > -60, f"{resid(o32, o512):.1f} dBFS")
n512 = render("new", 2, 37, 0.0, SEED, "512", 0, "kd")
o512 = render("old", 2, 37, 0.0, SEED, "512", 0, "kd")
check("new drone differs from old at 512", resid(n512, o512) > -60, f"{resid(n512, o512):.1f} dBFS")
s1 = render("new", 2, 37, 0.4, 1, "512", 0, "kd"); s2 = render("new", 2, 37, 0.4, 2, "512", 0, "kd")
check("seed changes the render (variation 0.4)", resid(s1, s2) > -60, f"{resid(s1, s2):.1f} dBFS")
s1 = render("new", 0, 8, 0.6, 1, "512", 0, "rand"); s2 = render("new", 0, 8, 0.6, 2, "512", 0, "rand")
check("seed changes the render (rand path)", resid(s1, s2) > -60, f"{resid(s1, s2):.1f} dBFS")
s1 = render("new", 2, 37, 0.0, 1, "512", 0, "kd"); s2 = render("new", 2, 37, 0.0, 2, "512", 0, "kd")
check("seed is inert at variation 0", np.array_equal(s1, s2), "")

print(f"\n{len(fails)} failure(s)" + (": " + ", ".join(fails) if fails else ""))
sys.exit(1 if fails else 0)
