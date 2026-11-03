#!/usr/bin/env python3
"""O-Comp preset + state gate: preset-manager module vs the 1.11.3 local copy.

Usage: gate.py <path to O-Comp-preset-gate>
Build: cmake -DOUARICON_BUILD_TESTS=ON ...; cmake --build <dir> --target O-Comp-preset-gate
Touches the real ~/Library/O-Comp/Presets bank (rewrites the factory files only;
the temporary user preset is deleted). baseline-1.11.3/ came from this same driver
compiled against the 1.11.3 header.

Two expected differences from 1.11.3, both module behaviour:
  * The factory presets carry 7 keys and omit sc_source/sc_hpf/sc_lpf/sc_listen.
    1.11.3 left those at whatever was live; the module resets omitted keys to their
    defaults first (WR-01). So in the full-perturb run, only the sc_* lines of a
    factory load may differ, and they must read the default. The scdefault run
    (sc_* already at default) must match 1.11.3 line for line, state blobs included.
  * Prev with no in-list anchor: 1.11.3 went to the last preset, the module goes to
    the first (IN-03). Next is unchanged.
"""
import json, os, re, subprocess, sys
from pathlib import Path

here = Path(__file__).parent
base = here / "baseline-1.11.3"
driver = sys.argv[1]
bank = Path.home() / "Library/O-Comp/Presets"
factory = bank / "Factory"
sentinel = factory / ".factory-version"
version = re.search(r"VERSION (\d+\.\d+\.\d+)", (here / "../../CMakeLists.txt").read_text()).group(1)
SC_DEFAULT = {"sc_source": "Internal", "sc_hpf": "0.0", "sc_lpf": "0.0", "sc_listen": "Off"}
fails = 0

def check(ok, label, extra=""):
    global fails
    fails += not ok
    print(f"{'PASS' if ok else 'FAIL'}  {label} {extra}")

def run(blobs, *mode):
    r = subprocess.run([driver, str(blobs), "restore", *mode], capture_output=True, text=True)
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

# 1. scdefault run on a fresh sentinel-less bank: identical to 1.11.3 except navFreshPrev.
sentinel.unlink(missing_ok=True)
now = run(base / "scdefault", "scdefault")
want = (base / "scdefault/dump.tsv").read_text().splitlines()
diff = [(a, b) for a, b in zip(want, now) if a != b]
nav = [d for d in diff if d[0].startswith("navFreshPrev")]
rest = [d for d in diff if d not in nav]
check(len(want) == len(now) and not rest, "scdefault: dump identical to 1.11.3 (state blobs included)",
      f"({len(want)} lines, {len(rest)} differ)")
show(rest)
check(nav == [("navFreshPrev\tVocal Smooth", f"navFreshPrev\t{names[0]}")], "unanchored Prev: last -> first (IN-03)",
      nav[0][1] if nav else "")

# 2. Full-perturb run: only factory-load sc_* lines differ, and they read the default.
now = run(base)
want = (base / "dump.tsv").read_text().splitlines()
diff = [(a, b) for a, b in zip(want, now) if a != b]
expected, bad = 0, []
for a, b in diff:
    fa, fb = a.split("\t"), b.split("\t")
    tag = fa[0]
    if tag == "navFreshPrev":
        continue
    if tag.startswith("state\t") or fa[0] == "state":
        expected += 1          # blob carries the reset sc_* values
        continue
    if tag.split(":")[0] in ("applied", "reopen") and fa[1] in SC_DEFAULT and fb[3] == SC_DEFAULT[fa[1]]:
        expected += 1
        continue
    bad.append((a, b))
check(len(want) == len(now) and not bad, "full: only factory-load sc_* lines differ, all at default",
      f"({len(want)} lines, {expected} expected diffs, {len(bad)} unexpected)")
show(bad)
other = [(a, b) for a, b in zip(want, now) if a.startswith("otherBuildBlob") and a != b]
check(not other, "1.11.3 session blobs reopen identically", f"({sum(l.startswith('otherBuildBlob') for l in want)} lines)")

# 3. Factory JSON: same values as 1.11.3, stamped with this version.
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

# 6. Temp user preset gone.
check(not (bank / "User/__preset_gate__.json").exists(), "temporary user preset removed")
if not user_existed:
    try:
        (bank / "User").rmdir()
    except OSError:
        pass

print("GATE", "FAIL" if fails else "PASS")
sys.exit(1 if fails else 0)
