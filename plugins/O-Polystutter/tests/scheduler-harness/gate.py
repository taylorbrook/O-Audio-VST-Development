import itertools, math, os, subprocess, sys
import numpy as np

# v1.17.0 gate: where the v1.16.0 onsets land, block-size null (32 / 512 / variable,
# fresh instances, <= -120 dBFS), bit-identity vs v1.16.0 where its onsets were
# already exact, and controls that must FAIL (else the gates above are vacuous).
H = os.environ.get("OUT", os.path.join(os.environ.get("TMPDIR", "/tmp"), "psh"))
OUT = os.path.join(H, "renders"); os.makedirs(OUT, exist_ok=True)
FS, LEN = 48000.0, 480000
fails = []

def render(eng, cfg, block, seed, onsets=False):
    f = os.path.join(OUT, f"{eng}_{cfg}_{block}_{seed}.f32")
    args = [os.path.join(H, f"psh_{eng}"), cfg, block, str(seed), f]
    if onsets: args.append(f[:-4] + ".onsets")
    subprocess.run(args, check=True)
    if onsets:
        rows = [l.split() for l in open(f[:-4] + ".onsets")]
        return [(int(a), int(b), k) for a, b, k in rows]
    return np.fromfile(f, dtype=np.float32)

def db(x): return 20 * np.log10(x) if x > 0 else -np.inf
def resid(a, b): return db(float(np.max(np.abs(a.astype(np.float64) - b.astype(np.float64)))))
def check(name, ok, detail): print(("PASS " if ok else "FAIL ") + name + "  " + detail); ok or fails.append(name)

SEED = 0x5EED1234

print("== 0. where the onsets land (logged trigger sample - ideal sample)")
# onsets_beat: 128 BPM, transport starts at sample 1234, sequencer off, prob 100 → every boundary fires
SUBDIV_PPQ = [0.5, 1.0, 0.5, 1.0]   # lanes 1-4: 1/8, 1/4, 1/8, 1/4
ppq_per_sample = 128.0 / (60.0 * FS)
def ideal_beats(lane):
    s = SUBDIV_PPQ[lane]; out = []
    k = math.ceil(1234 * ppq_per_sample / s)
    while True:
        t = math.ceil(k * s / ppq_per_sample - 1e-6) - 1234
        if t >= LEN: return out
        out.append(t); k += 1
for eng in ("old", "new"):
    for block in ("32", "512"):
        ons = render(eng, "onsets_beat", block, SEED, onsets=True)
        errs = []
        for lane in range(4):
            got = sorted(t for l, t, k in ons if l == lane)
            ideal = [t for t in ideal_beats(lane) if t >= int(block)]   # v1.16.0 skips the first playing block
            # pair each ideal boundary with the first logged trigger at/after it
            j = 0
            for t in ideal:
                while j < len(got) and got[j] < t: j += 1
                if j < len(got): errs.append(got[j] - t)
        e = np.array(errs)
        print(f"   beat {eng} block {block}: n={len(e)}  late min {e.min()}  max {e.max()}  mean {e.mean():.1f} samples")
        if eng == "new": check(f"new beat onsets exact block{block}", e.max() == 0 and e.min() == 0, f"max |err| {np.abs(e).max()}")
        else: check(f"old beat onsets late by up to a block (block{block})", e.min() >= 0 and e.max() > int(block) // 2 and e.max() < int(block), "")
    for block in ("32", "512"):
        ons = render(eng, "onsets_midi", block, SEED, onsets=True)
        cfg_notes = sorted({t for l, t, k in render("new", "onsets_midi", block, SEED, onsets=True)})  # new logs the note sample
        got = sorted({t for l, t, k in ons})
        e = np.array([g - min(cfg_notes, key=lambda n: abs(n - g)) for g in got])
        print(f"   midi {eng} block {block}: n={len(e)}  err min {e.min()}  max {e.max()} samples (negative = EARLY)")
        if eng == "old": check(f"old midi onsets early by the note offset (block{block})", e.max() <= 0 and e.min() < -(int(block) // 2), "")

print("== A. block-size null, fresh instances (new engine), residual must be <= -120 dBFS")
for cfg in ("beat128", "beatprob", "midi", "loop", "exact_beat", "exact_midi"):
    r = {b: render("new", cfg, b, SEED) for b in ("32", "512", "var")}
    peak = db(float(np.max(np.abs(r["512"]))))
    worst = max(resid(r[a], r[b]) for a, b in itertools.combinations(r, 2))
    check(f"null {cfg}", worst <= -120 and peak > -40, f"worst residual {worst:.1f} dBFS, signal peak {peak:.1f} dBFS")

print("== B. bit-identity vs v1.16.0 where its onsets were already exact")
for cfg in ("exact_beat", "exact_midi"):
    for block in ("32", "480"):
        a = render("old", cfg, block, SEED); b = render("new", cfg, block, SEED)
        check(f"bit-identical {cfg} block{block}", np.array_equal(a, b) and np.max(np.abs(a)) > 0.01,
              f"max |diff| {np.max(np.abs(a.astype(np.float64) - b)):.3g}, peak {np.max(np.abs(a)):.3f}")

print("== C. controls (each must DIFFER, else the gates above are vacuous)")
# v1.16.0 also started the end-of-cycle fade-out at a block start; v1.17.0 starts it on
# the sample the last repeat expires. A cycle (lane 4: 1/8T x 4 = 32000) that ends
# mid-block at 480 therefore differs, which is why exact_midi gives lane 4 3 repeats.
a, b = render("old", "exact_midi_r4", "480", SEED), render("new", "exact_midi_r4", "480", SEED)
check("old vs new differ when a cycle ends mid-block (exact_midi_r4 @480)", resid(a, b) > -60, f"residual {resid(a, b):.1f} dBFS")
a, b = render("old", "beat128", "32", SEED), render("old", "beat128", "512", SEED)
check("old engine is block-size dependent (beat128 32 vs 512)", resid(a, b) > -60, f"residual {resid(a, b):.1f} dBFS")
a, b = render("old", "midi", "512", SEED), render("new", "midi", "512", SEED)
check("old vs new differ on unaligned MIDI", resid(a, b) > -60, f"residual {resid(a, b):.1f} dBFS")
a, b = render("new", "beatprob", "512", SEED), render("new", "beatprob", "512", SEED + 1)
check("seed changes the render (beatprob)", resid(a, b) > -60, f"residual {resid(a, b):.1f} dBFS")
a, b = render("new", "beatprob", "512", SEED), render("new", "beatprob", "32", SEED)
check("same seed reproduces across instances and block sizes", resid(a, b) <= -120, f"residual {resid(a, b):.1f} dBFS")
# WR-04: the loop (2 bars = 180000 samples) wraps at render samples 180000 and 360000;
# the 1/4 lane's downbeat there must fire in v1.17.0 and was dropped by v1.16.0
for eng, want in (("new", True), ("old", False)):
    ons = render(eng, "loop", "512", SEED, onsets=True)
    hit = all(any(l == 3 and t == w for l, t, k in ons) for w in (180000, 360000))
    check(f"loop-wrap downbeat {'fires' if want else 'dropped'} ({eng})", hit == want, "")
# the wet path must matter in every exact config, or bit-identity only proves the dry path
for cfg in ("exact_beat", "exact_midi"):
    ons = render("new", cfg, "32", SEED, onsets=True)
    check(f"{cfg} fires triggers on every lane", len({l for l, t, k in ons}) == 4, f"{len(ons)} triggers")

print(f"\n{'ALL PASS' if not fails else str(len(fails)) + ' FAIL: ' + ', '.join(fails)}")
sys.exit(1 if fails else 0)
