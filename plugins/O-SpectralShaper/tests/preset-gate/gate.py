#!/usr/bin/env python3
"""O-SpectralShaper preset + state gate: preset-manager module vs the 1.10.2 local copy.

Usage: gate.py <path to O-SpectralShaper-preset-gate>
Build: cmake -DOUARICON_BUILD_TESTS=ON ...; ninja O-SpectralShaper-preset-gate
Touches the real ~/Library/O-SpectralShaper/Presets bank (rewrites the factory files
only; the temporary user preset is deleted). baseline-1.10.2/ came from this same
driver compiled against the 1.10.2 header.

Every factory preset carries all 7 parameters and a customState, so the module's
reset-to-defaults (WR-01) and empty-customLoad (1.0.7) change no value here, and
a fresh instance starts on "Default" (in the list), so the module's unanchored-Prev
change (IN-03) never fires. Expected: every dump line identical to 1.10.2. The only
behaviour change is the factory-version sentinel (checks 3-4).
"""
import json, os, re, subprocess, sys
from pathlib import Path

here = Path(__file__).parent
base = here / "baseline-1.10.2"
driver = sys.argv[1]
bank = Path.home() / "Library/O-SpectralShaper/Presets"
factory = bank / "Factory"
sentinel = factory / ".factory-version"
version = re.search(r"VERSION (\d+\.\d+\.\d+)", (here / "../../CMakeLists.txt").read_text()).group(1)
fails = 0

def check(ok, label, extra=""):
    global fails
    fails += not ok
    print(f"{'PASS' if ok else 'FAIL'}  {label} {extra}")

def run(blobs):
    r = subprocess.run([driver, str(blobs), "restore"], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"driver failed rc={r.returncode}: {r.stderr}")
    return r.stdout.splitlines()

def bank_times(t=None):
    files = sorted(factory.glob("*.json")) + [sentinel]
    if t is not None:
        for f in files:
            os.utime(f, (t, t))
    return {f.name: f.stat().st_mtime for f in files}

def show(diff):
    for a, b in diff[:8]:
        print("   -", a[:150], "\n   +", b[:150])

user_existed = (bank / "User").exists()
names = sorted(p.stem for p in base.glob("*.bin"))

# 1. Fresh sentinel-less bank: identical to 1.10.2 except navFreshPrev.
sentinel.unlink(missing_ok=True)
now = run(base)
want = (base / "dump.tsv").read_text().splitlines()
diff = [(a, b) for a, b in zip(want, now) if a != b]
check(len(want) == len(now) and not diff, "dump identical to 1.10.2 (params, curves, state blobs, nav)",
      f"({len(want)} lines, {len(diff)} differ)")
show(diff)
fresh = [l for l in now if l.startswith("navFresh")]
check(fresh == want[:2], "fresh-instance nav unchanged (anchored on Default)", " / ".join(fresh))
curves = sum("Curve\t" in l for l in want)
check(curves > 0, "curves are in the dump", f"({curves} curve lines)")
other = [(a, b) for a, b in zip(want, now) if a.startswith("otherBuildBlob") and a != b]
check(not other, "1.10.2 session blobs reopen identically", f"({sum(l.startswith('otherBuildBlob') for l in want)} lines)")

# 2. Factory JSON: same values as 1.10.2, stamped with this version.
badf = []
for f in sorted((base / "factory").glob("*.json")):
    old, new = json.loads(f.read_text()), json.loads((factory / f.name).read_text())
    if new.pop("version") != version or old.pop("version") == version or old != new:
        badf.append(f.name)
nf = len(list((base / "factory").glob("*.json")))
check(not badf and nf == len(names), f"{nf} factory files match, version {version}", badf or "")

# 3. Sentinel: stamped, then no rewrite at the same version.
check(sentinel.exists() and sentinel.read_text().strip() == version, "sentinel stamped")
t0 = 1_700_000_000
before = bank_times(t0)
run(base)
check(bank_times() == before, "same version: 0 factory files rewritten")

# 4. Negative control: a stale sentinel must rewrite, so check 3 can fail.
sentinel.write_text("0.0.0")
bank_times(t0)
run(base)
changed = sum(v != t0 for k, v in bank_times().items() if not k.startswith("."))
check(changed >= len(names), "stale sentinel: bank rewritten", f"({changed} files)")
check(sentinel.read_text().strip() == version, "sentinel restamped")

# 5. Temp user preset gone.
check(not (bank / "User/__preset_gate__.json").exists(), "temporary user preset removed")
if not user_existed:
    try:
        (bank / "User").rmdir()
    except OSError:
        pass

print("GATE", "FAIL" if fails else "PASS")
sys.exit(1 if fails else 0)
