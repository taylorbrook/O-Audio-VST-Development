#!/usr/bin/env python3
"""Compare parity_dump output: new module vs each source fork and vs the old module.

usage: parity_gate.py <build-dir> [names...]   (default: baseline prism strata formant bells)
Exit 0 iff every required case matches and every accepted divergence still diverges.
"""
import subprocess
import sys
from pathlib import Path

# Cases where the module must equal the fork that the fix came from.
REQUIRED = {
    "prism": ["scl.negcents", "scl.blankdesc", "scl.pitchcomment", "scl.malformed", "scl.short",
              "scl.count0", "scl.count5000", "scl.valid", "kbm.ref_c", "kbm.ref_a4", "kbm.map_huge",
              "kbm.octdeg7", "kbm.save_reference", "edit.eleven", "gen.rank2_order",
              "exp.svg_12tet", "exp.html_suboctave"],
    "formant": ["scl.negcents", "scl.blankdesc", "scl.pitchcomment", "scl.malformed", "scl.short",
                "scl.count0", "scl.count5000", "scl.tworatios", "scl.infratio", "scl.valid",
                "kbm.ref_c", "kbm.ref_a4", "kbm.octdeg7", "kbm.degree_wrap", "kbm.save_reference",
                "edit.eleven", "gen.rank2_order", "gen.rank2_huge", "gen.rank2_nonfinite",
                "emb.partch43", "exp.svg_12tet", "exp.html_suboctave", "exp.html_escape"],
    "bells": ["scl.count0", "gen.rank2_order", "exp.et_zero_degrees"],
}
REQUIRED["strata"] = REQUIRED["prism"]

# Deliberate differences: (tree, case) -> why the module does not follow that fork.
ACCEPTED = {
    ("formant", "kbm.map_huge"): "Formant clamps the size to 128; module rejects the file (Prism WR-12)",
    ("bells", "kbm.degree_wrap"): "Bells clamps past-the-end degrees to scaleSize-1; module wraps into the next period (Formant, Scala spec)",
}


def run(build, name):
    exe = Path(build) / f"parity-{name}_artefacts" / "Release" / f"parity-{name}"
    # The baseline lacks CR-08/CR-09: the huge cases never return or exhaust memory.
    args = [str(exe)] + (["--skip-hanging"] if name == "baseline" else [])
    out = subprocess.run(args, capture_output=True, text=True, check=True, timeout=120).stdout
    return dict(line.split("\t", 1) for line in out.splitlines() if "\t" in line)


def short(v):
    return v if len(v) < 110 else v[:110] + "..."


def main():
    build = sys.argv[1]
    names = sys.argv[2:] or ["baseline", "prism", "strata", "formant", "bells"]
    module = run(build, "module")
    bad = 0

    for name in names:
        other = run(build, name)
        if name == "baseline":
            cases = [c for c in module if c.startswith("dflt.")]
        else:
            cases = REQUIRED[name]
        same = sum(module[c] == other.get(c) for c in cases)
        print(f"{name}: {same}/{len(cases)} required cases equal")
        for c in cases:
            if module[c] != other.get(c):
                bad += 1
                print(f"  DIFF {c}\n    module {short(module[c])}\n    {name} {short(other.get(c, '<absent>'))}")
        for (tree, c), why in ACCEPTED.items():
            if tree != name:
                continue
            if module[c] == other.get(c):
                bad += 1
                print(f"  STALE accepted divergence {c}: now equal")
            else:
                print(f"  accepted divergence {c}: {why}")
    print("PASS" if bad == 0 else f"FAIL ({bad})")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
