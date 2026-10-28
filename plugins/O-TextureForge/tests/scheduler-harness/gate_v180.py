import itertools, os, subprocess, sys
import numpy as np

# v1.8.0 gate: steerable descriptor weights (sqrt(w) pre-scaled tree), cached k = 16
# candidates with a softmax(-D^2/tau) pick, and the recently-used penalty ring.
# Run after `build180.sh` (tfh_old = the v1.7.0 backup, both builds seeded).
#   A. defaults bit-identical to v1.7.0: every mode and search path, three block
#      patterns, two seeds; also with a SearchIndex built at all-100 % weights and
#      with Pick Spread / Repeat Avoid explicitly 0
#   B. block-size null with every feature engaged (32 / 512 / variable)
#   C. the features do what they say, on the drone pick log (no CC events, so the
#      target is fixed; 500 picks at density 64):
#      C1 Energy Weight 400 % lands nearer the Energy target than 100 %, 100 % nearer
#         than 0 %; Timbre Weight 0 % nearer than 100 %
#      C2 Pick Spread 0 with Variation 0 repeats one grain (the "machine gun");
#         distinct grains rise and the nearest grain's share falls with spread
#      C3 Repeat Avoid 100 % never repeats a grain back to back, in both search paths
#   D. no heap allocation inside processBlock with every feature engaged
H = os.environ.get("OUT", os.path.join(os.environ.get("TMPDIR", "/tmp"), "tfh"))
OUT = os.path.join(H, "renders180"); os.makedirs(OUT, exist_ok=True)
fails = []

def run(eng, mode, dens, var, seed, block, path, env=None, tag="", tail=0):
    f = os.path.join(OUT, f"{eng}_{mode}_{dens}_{var}_{seed}_{block}_{path}_{tail}_{tag}.f32")
    e = dict(os.environ, TF_BLR="0")
    e.update(env or {})
    r = subprocess.run([os.path.join(H, f"tfh_{eng}"), str(mode), str(dens), str(var), str(seed), block,
                        "0", path, f, str(tail)], check=True, env=e, capture_output=True, text=True)
    return np.fromfile(f, dtype=np.float32), r.stderr

def render(*a, **k): return run(*a, **k)[0]

def picks(env, var=0.0, tag=""):
    log = os.path.join(OUT, f"picks_{tag}.txt")
    e = dict(env); e["TF_PICKS"] = log; e["TF_NO_EVENTS"] = "1"
    render("new", 2, 64, var, 7, "32", "kd", env=e, tag="p" + tag)
    a = np.loadtxt(log, ndmin=2)
    return a[:, 1].astype(int), a[:, 2:5]

def db(x): return 20 * np.log10(x) if x > 0 else -np.inf
def resid(a, b): return db(float(np.max(np.abs(a.astype(np.float64) - b.astype(np.float64)))))
def check(name, ok, detail): print(("PASS " if ok else "FAIL ") + name + "  " + detail); ok or fails.append(name)

print("== A. defaults bit-identical to v1.7.0 (seeded, mipmap off in both)")
cases = [(2, 8, 0.4, "kd"), (2, 64, 0.7, "kd"), (2, 37, 0.4, "proj"), (2, 37, 0.6, "rand"),
         (1, 8, 0.4, "kd"), (1, 8, 0.6, "proj"), (0, 8, 0.4, "kd"), (0, 8, 0.6, "rand")]
variants = [("plain", {}), ("index@100%", {"TF_W": "1,1,1,1", "TF_W_FORCE": "1"}),
            ("spread0+avoid0", {"TF_SPREAD": "0", "TF_AVOID": "0", "TF_W": "1,1,1,1"})]
n = 0
for (mode, dens, var, path), seed, block in itertools.product(cases, (0, 12345), ("32", "512", "var")):
    a = render("old", mode, dens, var, seed, block, path)
    for vname, env in variants:
        if vname != "plain" and (block != "512" or path != "kd"):
            continue
        b = render("new", mode, dens, var, seed, block, path, env=env, tag=vname)
        ok = np.array_equal(a, b) and np.max(np.abs(a)) > 0.01
        n += 1
        if not ok or vname != "plain":
            check(f"mode{mode} d{dens} v{var} {path} seed{seed} {block} {vname}", ok,
                  f"max |diff| {np.max(np.abs(a - b)):.3g}, peak {np.max(np.abs(a)):.3f}")
print(f"   {n} renders compared")

print("== B. block-size null, all features on (W 0.5,2,1,4  spread 0.5  avoid 0.5)")
ON = {"TF_W": "0.5,2,1,4", "TF_SPREAD": "0.5", "TF_AVOID": "0.5"}
for mode, dens, var, path in ((2, 37, 0.4, "kd"), (2, 64, 0.0, "kd"), (1, 8, 0.4, "kd"), (0, 8, 0.4, "kd")):
    for tail in (0, 1):
        r = {b: render("new", mode, dens, var, 99, b, path, env=ON, tag="on", tail=tail) for b in ("32", "512", "var")}
        worst = max(resid(r[x], r[y]) for x, y in itertools.combinations(r, 2))
        peak = db(float(np.max(np.abs(r["512"]))))
        check(f"null mode{mode} d{dens} v{var} tail{tail}", worst <= -120 and peak > -40,
              f"worst residual {worst:.1f} dBFS, peak {peak:.1f} dBFS")
off = render("new", 2, 37, 0.4, 99, "512", "kd")
on = render("new", 2, 37, 0.4, 99, "512", "kd", env=ON, tag="on")
check("control: features on differ from off", resid(on, off) > -60, f"{resid(on, off):.1f} dBFS")

print("== C1. steering (drone, Energy 0.9 -> z target 0.8, Variation 0.3)")
T = {"TF_ENERGY": "0.9", "TF_BRIGHT": "0.5", "TF_TEXTURE": "0.5"}
def err(w, dim=0, target=0.8):
    idx, d = picks(dict(T, TF_W=w), var=0.3, tag="w" + w.replace(",", "_"))
    return float(np.mean(np.abs(d[:, dim] - target))), len(idx)
e4, k4 = err("1,1,1,4"); e1, k1 = err("1,1,1,1"); e0, k0 = err("1,1,1,0")
check("Energy Weight 400% < 100% < 0% (mean |energy - target|)", e4 < e1 < e0 and min(k4, k1, k0) > 300,
      f"{e4:.3f} < {e1:.3f} < {e0:.3f}  ({k1} picks)")
et0, _ = err("0,1,1,1")
check("Timbre Weight 0% lands nearer the macro target than 100%", et0 < e1, f"{et0:.3f} < {e1:.3f}")
eb4, _ = err("1,4,1,1", dim=1, target=0.0); eb0, _ = err("1,0,1,1", dim=1, target=0.0)
check("Brightness Weight 400% < 0% (mean |brightness - target|)", eb4 < eb0, f"{eb4:.3f} < {eb0:.3f}")

print("== C2. Pick Spread (Variation 0)")
def stats(env, tag):
    idx, _ = picks(env, var=0.0, tag=tag)
    rep = float(np.mean(idx[1:] == idx[:-1]))
    top = float(np.mean(idx == np.bincount(idx).argmax()))
    return len(set(idx.tolist())), rep, top, len(idx)
s0 = stats({}, "s0")
check("spread 0: one grain, every pick a repeat (machine gun)", s0[0] == 1 and s0[1] == 1.0,
      f"distinct {s0[0]}, repeat {s0[1]:.2f}, picks {s0[3]}")
prev = s0
for s in ("0.25", "0.5", "1"):
    cur = stats({"TF_SPREAD": s}, "s" + s)
    check(f"spread {s}: no fewer distinct (16 is the cap), lower top share than the step below",
          cur[0] >= prev[0] and cur[2] < prev[2],
          f"distinct {cur[0]} (k=16), repeat {cur[1]:.3f}, top share {cur[2]:.3f}")
    prev = cur
check("spread 1: draws on at least 12 of the 16 candidates", prev[0] >= 12, f"distinct {prev[0]}")

print("== C3. Repeat Avoid (Variation 0)")
for name, env in (("nearest path", {"TF_AVOID": "1"}), ("cached path", {"TF_AVOID": "1", "TF_SPREAD": "0.3"})):
    d, rep, top, k = stats(env, "a" + name.split()[0])
    check(f"avoid 100% {name}: no back-to-back repeat", rep == 0.0 and d >= 8, f"distinct {d}, repeat {rep:.3f}, picks {k}")
d5, rep5, _, _ = stats({"TF_AVOID": "0.5"}, "a05")
check("avoid 50% nearest path: fewer repeats than 0%", rep5 < 1.0, f"distinct {d5}, repeat {rep5:.3f}")

print("== D. allocations inside processBlock")
for mode, env in ((2, ON), (0, ON), (2, {})):
    _, err_ = run("new", mode, 64 if mode == 2 else 8, 0.4, 5, "var", "kd", env=dict(env, TF_ALLOC="1"), tag="alloc")
    cnt = int(err_.strip().split()[-1])
    check(f"mode{mode} {'on' if env else 'off'}: 0 allocations", cnt == 0, f"{cnt}")

print(f"\n{len(fails)} failure(s)" + (": " + ", ".join(fails) if fails else ""))
sys.exit(1 if fails else 0)
