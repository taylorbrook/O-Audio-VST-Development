#!/usr/bin/env python3
"""O-Bass preset + state gate: preset-manager module vs the 1.8.3 local copy.

Usage: gate.py <path to O-Bass-preset-gate>
Build: cmake -DOUARICON_BUILD_TESTS=ON ...; cmake --build <dir> --target O-Bass-preset-gate
Touches the real ~/Library/OBass/Presets bank (rewrites the factory files only;
the temporary user preset is deleted). baseline-1.8.3/ came from this same driver
compiled against the 1.8.3 header.
"""
import json, os, re, subprocess, sys
from pathlib import Path

here = Path(__file__).parent
base = here / "baseline-1.8.3"
driver = sys.argv[1]
bank = Path.home() / "Library/OBass/Presets"
factory = bank / "Factory"
sentinel = factory / ".factory-version"
version = re.search(r"VERSION (\d+\.\d+\.\d+)", (here / "../../CMakeLists.txt").read_text()).group(1)
fails = 0

def check(ok, label, extra=""):
    global fails
    fails += not ok
    print(f"{'PASS' if ok else 'FAIL'}  {label} {extra}")

def run():
    r = subprocess.run([driver, str(base), "restore"], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"driver failed rc={r.returncode}: {r.stderr}")
    return r.stdout.splitlines()

def bank_times(t=None):
    files = sorted(factory.glob("*.json")) + [sentinel]
    if t is not None:
        for f in files:
            os.utime(f, (t, t))
    return {f.name: f.stat().st_mtime for f in files}

user_existed = (bank / "User").exists()

# 1. Fresh sentinel-less bank: every observable outcome matches the old header.
sentinel.unlink(missing_ok=True)
names = sorted(p.stem for p in base.glob("*.bin"))
out = run()
# "Aggressive Colored" is a leftover file in some banks, absent from the factory table.
keep = lambda l: not l.startswith("count") and "Aggressive Colored" not in l
now = [l for l in out if keep(l)]
want = (base / "dump.tsv").read_text().splitlines()
diff = [(a, b) for a, b in zip(want, now) if a != b]
check(len(want) == len(now) and not diff, "dump identical to 1.8.3", f"({len(want)} lines, {len(diff)} differ)")
for a, b in diff[:8]:
    print("   -", a[:150], "\n   +", b[:150])

# 2. Factory JSON: same values as 1.8.3, stamped with this version.
bad = []
for f in sorted((base / "factory").glob("*.json")):
    old, new = json.loads(f.read_text()), json.loads((factory / f.name).read_text())
    if new.pop("version") != version or old.pop("version") == version or old != new:
        bad.append(f.name)
check(not bad, f"{len(names)} factory files match, version {version}", bad or "")

# 3. Sentinel: stamped, then no rewrite at the same version.
check(sentinel.read_text().strip() == version, "sentinel stamped", sentinel.read_text().strip())
t0 = 1_700_000_000
before = bank_times(t0)
run()
check(bank_times() == before, "same version: 0 factory files rewritten")

# 4. Negative control: a stale sentinel must rewrite, so check 3 can fail.
sentinel.write_text("0.0.0")
bank_times(t0)
run()
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
