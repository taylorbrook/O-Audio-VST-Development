import itertools, os, subprocess, sys
import numpy as np

# v1.7.0 gate: the band-limited mipmap read for rate > 1 grains.
# Run after `build.sh <v1.6.2 backup dsp>` (tfh_old = v1.6.2).
#   A. block-size null with the mipmap built (32 / 512 / variable, fresh instances),
#      Pitch-Mapped notes 40..99 (rate up to 6.7, mip levels 0..2): <= -120 dBFS
#   B. bit-identical to v1.6.2 wherever no grain plays above rate 1: Trigger+Modulate
#      and Drone (rate 1), and Pitch-Mapped with every note clamped to 60
#   C. controls: the band-limited path is live (mipmap on vs off differ; full
#      Pitch-Mapped differs from v1.6.2)
H = os.environ.get("OUT", os.path.join(os.environ.get("TMPDIR", "/tmp"), "tfh"))
OUT = os.path.join(H, "renders170"); os.makedirs(OUT, exist_ok=True)
fails = []
SEED = 0   # main.cpp sets rngSeed only in the new build; 0 = the old build's default

def render(eng, mode, dens, var, block, path, blr=1, notemax=None, tail=0):
    f = os.path.join(OUT, f"{eng}_{mode}_{dens}_{var}_{block}_{path}_{blr}_{notemax}_{tail}.f32")
    env = dict(os.environ, TF_BLR=str(blr))
    if notemax is not None: env["TF_NOTE_MAX"] = str(notemax)
    subprocess.run([os.path.join(H, f"tfh_{eng}"), str(mode), str(dens), str(var), str(SEED), block,
                    "0", path, f, str(tail)], check=True, env=env)
    return np.fromfile(f, dtype=np.float32)

def db(x): return 20 * np.log10(x) if x > 0 else -np.inf
def resid(a, b): return db(float(np.max(np.abs(a.astype(np.float64) - b.astype(np.float64)))))
def check(name, ok, detail): print(("PASS " if ok else "FAIL ") + name + "  " + detail); ok or fails.append(name)

print("== A. block-size null, mipmap built, fresh instances: <= -120 dBFS")
for mode, dens, var, path in ((0, 8, 0.4, "kd"), (0, 8, 0.6, "rand"), (0, 8, 0.4, "proj"), (1, 8, 0.4, "kd"), (2, 37, 0.4, "kd")):
    for tail in (0, 1):
        r = {b: render("new", mode, dens, var, b, path, tail=tail) for b in ("32", "512", "var")}
        peak = db(float(np.max(np.abs(r["512"]))))
        worst = max(resid(r[a], r[b]) for a, b in itertools.combinations(r, 2))
        check(f"null mode{mode} d{dens} v{var} {path} tail{tail}", worst <= -120 and peak > -40,
              f"worst residual {worst:.1f} dBFS, signal peak {peak:.1f} dBFS")

print("== B. bit-identical to v1.6.2 where no grain is above rate 1")
for mode, dens, var, block, path, nm in ((2, 8, 0.4, "512", "kd", None), (2, 64, 0.4, "var", "kd", None),
                                         (2, 37, 0.7, "32", "rand", None), (1, 8, 0.4, "512", "kd", None),
                                         (1, 8, 0.6, "var", "rand", None), (0, 8, 0.4, "512", "kd", 60),
                                         (0, 8, 0.6, "var", "rand", 60), (0, 8, 0.4, "32", "proj", 60)):
    a = render("old", mode, dens, var, block, path, notemax=nm); b = render("new", mode, dens, var, block, path, notemax=nm)
    check(f"mode{mode} d{dens} v{var} {block} {path} notemax{nm}", np.array_equal(a, b) and np.max(np.abs(a)) > 0.01,
          f"max |diff| {np.max(np.abs(a - b)):.3g}, peak {np.max(np.abs(a)):.3f}")

print("== C. controls (must differ)")
a = render("new", 0, 8, 0.4, "512", "kd", blr=1); b = render("new", 0, 8, 0.4, "512", "kd", blr=0)
check("mipmap on vs off (Pitch-Mapped, notes 40..99)", resid(a, b) > -60, f"{resid(a, b):.1f} dBFS")
o = render("old", 0, 8, 0.4, "512", "kd")
check("v1.7.0 vs v1.6.2 (Pitch-Mapped, notes 40..99)", resid(a, o) > -60, f"{resid(a, o):.1f} dBFS")
check("mipmap off = v1.6.2 (fallback path is the old read)", np.array_equal(b, o), f"max |diff| {np.max(np.abs(b - o)):.3g}")

print(f"\n{len(fails)} failure(s)" + (": " + ", ".join(fails) if fails else ""))
sys.exit(1 if fails else 0)
