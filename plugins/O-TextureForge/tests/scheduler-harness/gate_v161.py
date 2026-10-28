import os, subprocess, sys
import numpy as np

# v1.6.1 gate (b): double read positions change nothing at unity pitch on a corpus
# under 2^24 samples. Run after `build.sh <v1.6.0 backup dsp>` (tfh_old = v1.6.0),
# then gate (a): `tfl_new` / `tfl_old` from longcorpus.cpp, built the same way.
H = os.environ.get("OUT", os.path.join(os.environ.get("TMPDIR", "/tmp"), "tfh"))
OUT = os.path.join(H, "renders161"); os.makedirs(OUT, exist_ok=True)
fails = []

def render(eng, mode, dens, var, block, align, path):
    f = os.path.join(OUT, f"{eng}_{mode}_{dens}_{var}_{block}_{align}_{path}.f32")
    subprocess.run([os.path.join(H, f"tfh_{eng}"), str(mode), str(dens), str(var), str(0x5EED1234), block,
                    str(align), path, f], check=True)
    return np.fromfile(f, dtype=np.float32)

def check(name, ok, detail): print(("PASS " if ok else "FAIL ") + name + "  " + detail); ok or fails.append(name)

print("== B. bit-identical to v1.6.0 at unity pitch (drone, Trigger+Modulate)")
for mode, dens, var, block, path in ((2, 8, 0.4, "512", "kd"), (2, 64, 0.4, "var", "kd"), (2, 37, 0.7, "32", "rand"),
                                     (2, 37, 0.4, "512", "proj"), (1, 8, 0.4, "512", "kd"), (1, 8, 0.6, "var", "rand")):
    a = render("old", mode, dens, var, block, 0, path); b = render("new", mode, dens, var, block, 0, path)
    check(f"mode{mode} d{dens} v{var} {block} {path}", np.array_equal(a, b) and np.max(np.abs(a)) > 0.01,
          f"max |diff| {np.max(np.abs(a - b)):.3g}, peak {np.max(np.abs(a)):.3f}")

print("== A. long corpus: far grain nulls the near one AND matches an exact double-position reference")
def tfl(eng, off, note):
    return [float(x) for x in subprocess.run([os.path.join(H, f"tfl_{eng}"), str(off), str(note)],
                                             capture_output=True, text=True, check=True).stdout.split()]
for off in (529200, 4194304, 16900000):
    for note in (60, 67, 89):
        new, old = tfl("new", off, note), tfl("old", off, note)
        check(f"offset {off} note {note}", new[0] <= -120 and new[2] <= -120 and new[1] > -20,
              f"new: vs near {new[0]:.1f}, vs ref {new[2]:.1f} | old: vs near {old[0]:.1f}, vs ref {old[2]:.1f} dBFS")
check("control: old engine misses the reference (note 89, 12 s: accumulated float drift)", tfl("old", 529200, 89)[2] > -60, "")
check("control: old engine fails past 2^24 at unity pitch", tfl("old", 16900000, 60)[0] > -60, "")

print(f"\n{len(fails)} failure(s)" + (": " + ", ".join(fails) if fails else ""))
sys.exit(1 if fails else 0)
