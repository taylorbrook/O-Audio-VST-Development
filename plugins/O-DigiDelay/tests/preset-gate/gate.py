#!/usr/bin/env python3
"""O-DigiDelay preset + state gate: preset-manager module vs the 1.9.3 local copy.

Usage: gate.py <path to O-DigiDelay-preset-gate>
Build: cmake -DOUARICON_BUILD_TESTS=ON ...; ninja O-DigiDelay-preset-gate
Touches the real ~/Library/O-DigiDelay/Presets bank (rewrites the factory files only;
the temporary user presets are deleted). baseline-1.9.3/ came from this same driver
compiled against the 1.9.3 header.

The module must reproduce the two local 1.9.3 behaviours, so their lines must match:
  * bypass is never saved, applied or reset by a preset (setExcludedParameters).
  * IN-07: every set inside a gesture, values clamped, non-numbers load as the default
    (setValidatedApply, module 1.1.1).
Unanchored Prev is tolerated if it moves to the first preset (module IN-03), but
1.9.3 already did that, so every line is expected to match.
"""
import json, os, re, subprocess, sys
from pathlib import Path

here = Path(__file__).parent
base = here / "baseline-1.9.3"
driver = sys.argv[1]
bank = Path.home() / "Library/O-DigiDelay/Presets"
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

user_existed = (bank / "User").exists()
names = sorted(p.stem for p in base.glob("*.bin"))

# 1. Fresh sentinel-less bank: identical to 1.9.3 except the unanchored Prev.
sentinel.unlink(missing_ok=True)
now = run(base)
want = (base / "dump.tsv").read_text().splitlines()
diff = [(a, b) for a, b in zip(want, now) if a != b]
nav = [d for d in diff if d[0].startswith("navFreshPrev")]
rest = [d for d in diff if d not in nav]
check(len(want) == len(now) and not rest, "dump identical to 1.9.3 (state blobs included)",
      f"({len(want)} lines, {len(rest)} differ)")
for a, b in rest[:8]:
    print("   -", a[:150], "\n   +", b[:150])
check(not nav or nav == [(nav[0][0], f"navFreshPrev\t{names[0]}")], "unanchored Prev: first preset (IN-03)",
      nav[0][1] if nav else "(unchanged)")

# 2. The behaviours the move must keep, read straight off this build's output.
bypass = [l for l in now if l.split("\t")[1:2] == ["bypass"]]
check(bypass and all(l.split("\t")[3] == "On" for l in bypass if not l.startswith("otherBuildBlob")),
      "bypass stays On through every preset load", f"({len(bypass)} lines)")
g = [l for l in now if l.startswith("gesture\t")]
check(len(g) == len(names) + 1 and all("setsInsideGesture=1\tunbalanced=0\tbypassTouched=0" in l for l in g),
      "every load: sets inside gestures, balanced, bypass untouched", f"({len(g)} loads)")
check("userFileHasBypass\t0" in now, "user save omits bypass")
other = [(a, b) for a, b in zip(want, now) if a.startswith("otherBuildBlob") and a != b]
check(not other, "1.9.3 session blobs reopen identically", f"({sum(l.startswith('otherBuildBlob') for l in want)} lines)")

# 3. Factory JSON: same values as 1.9.3, stamped with this version.
badf = []
for f in sorted((base / "factory").glob("*.json")):
    old, new = json.loads(f.read_text()), json.loads((factory / f.name).read_text())
    if new.pop("version") != version or old.pop("version") == version or old != new:
        badf.append(f.name)
check(not badf, f"{len(names)} factory files match, version {version}", badf or "")

# 4. Sentinel: stamped, then no rewrite at the same version.
check(sentinel.read_text().strip() == version, "sentinel stamped", sentinel.read_text().strip())
t0 = 1_700_000_000
before = bank_times(t0)
run(base)
check(bank_times() == before, "same version: 0 factory files rewritten")

# 5. Negative control: a stale sentinel must rewrite, so check 4 can fail.
sentinel.write_text("0.0.0")
bank_times(t0)
run(base)
changed = sum(v != t0 for k, v in bank_times().items() if not k.startswith("."))
check(changed >= len(names), "stale sentinel: bank rewritten", f"({changed} files)")
check(sentinel.read_text().strip() == version, "sentinel restamped")

# 6. Temp user presets gone.
check(not any((bank / "User").glob("__preset_gate*")), "temporary user presets removed")
if not user_existed:
    try:
        (bank / "User").rmdir()
    except OSError:
        pass

print("GATE", "FAIL" if fails else "PASS")
sys.exit(1 if fails else 0)
