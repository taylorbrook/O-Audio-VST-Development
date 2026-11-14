#!/usr/bin/env python3
"""Mutation run for O-ParametricEQ Phase 2.2 verify. Nothing in the repo or build/ is written."""
import os, re, shutil, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

REPO = "/Users/taylorbrook/Dev/VST-development"
P = REPO + "/plugins/O-ParametricEQ"
MUT = os.path.dirname(os.path.abspath(__file__))
OBJ = "plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir/"
PROC_O = OBJ + "Source/PluginProcessor.cpp.o"
MAIN_O = OBJ + "tests/render-harness/main.cpp.o"
OUT = "plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness"

cc_proc = open(MUT + "/cc_proc.txt").read().strip()
cc_main = open(MUT + "/cc_main.txt").read().strip()
link = open(MUT + "/link.txt").read().strip()

SE, BA, RO, BP = "dsp/SectionEngine.h", "dsp/Band.h", "dsp/Router.h", "dsp/BandParams.h"
PC, PH = "PluginProcessor.cpp", "PluginProcessor.h"

# id, harnesses, [(file, old, new, nth)]   nth: 0 = all occurrences, k = k-th only
MUTANTS = [
    ("M0", "dr", []),
    ("E1", "dr", [(SE, "c.mH * (x - c.k * v1 - v2)", "c.mH * (x - v1 - v2)", 1)]),
    ("E2", "dr", [(SE, "ic2 = 2.0 * v2 - ic2;", "ic2 = v2;", 1)]),
    ("E3", "dr", [(SE, "s = lp + v;", "s = lp;", 1)]),
    ("E4", "dr", [(SE, "r.svf.a3 = p.g * r.svf.a2;", "r.svf.a3 = r.svf.a2;", 1)]),
    ("E5", "dr", [(SE, "return c.mL * c.a3 + c.mB * c.a2 + c.mH * (1.0 - c.k * c.a2 - c.a3);",
                   "return c.mL * c.a3 + c.mH * (1.0 - c.k * c.a2 - c.a3);", 1)]),
    ("B1", "dr", [(BA, "if (sec.hasOnePole)", "if (false && sec.hasOnePole)", 1)]),
    ("B2", "dr", [(BA, "state[i].ic1[0] = 0.0;  state[i].ic1[1] = 0.0;", "state[i].ic1[0] = 0.0;", 1)]),
    ("B3", "dr", [(BA, "numSections = count;", "numSections = count; reset();", 1)]),
    ("B4", "dr", [(BA, "const int l = (lane != 0) ? 1 : 0;", "const int l = 0; (void) lane;", 2)]),
    ("R1", "dr", [(RO, "right[i] = r + d;", "right[i] = r - d;", 1)]),
    ("R4", "dr", [(RO, "if (placement == Placement::left)\n            return lane == 0;",
                   "if (placement == Placement::left)\n            return lane == 0 || lane == 1;", 1)]),
    ("R6", "dr", [(RO, "right[i] = r - d;", "right[i] = r + d;", 1)]),
    ("R7", "dr", [(RO, "const double m = 0.5 * (l + r);", "const double m = (l + r);", 0)]),
    ("R8", "dr", [(RO, "const double m = 0.5 * (l + r);", "const double m = (l + r);", 2)]),
    ("P1", "dr", [(BP, "in.settings.slopeIndex = 0;", "{}", 1)]),
    ("P2", "dr", [(BP, "gainEffMax = 60.0;", "gainEffMax = 30.0;", 1)]),
    ("P3", "dr", [(BP, "in.active = enabled && ! bypass;", "in.active = enabled; (void) bypass;", 1)]),
    ("P4", "dr", [(BP, "raw >= 0.5f;", "raw > 0.5f;", 1)]),
    ("P5", "dr", [(BP, "        || current.settings.type != next.settings.type\n", "", 1)]),
    ("P6", "dr", [(BP, "return static_cast<int> (v + 0.5);", "return static_cast<int> (v);", 1)]),
    ("P7", "dr", [(BP, "return stereoLayout ? placement : Placement::stereo;", "return stereoLayout ? placement : placement;", 1)]),
    ("P8", "dr", [(BP, "in.settings.q = 1.0;", "{}", 1)]),
    ("P9", "dr", [(BP, "const double product = gainDb * scale;", "const double product = gainDb * 100.0; (void) scale;", 1)]),
    ("X1", "r", [(PC, "    {\n        resetAllBands();\n        return;\n    }", "    {\n        return;\n    }", 1)]),
    ("X2", "r", [(PC, "    clearOutputsWithoutInput (buffer);\n    resetAllBands();\n}", "    clearOutputsWithoutInput (buffer);\n}", 1)]),
    ("X4", "r", [(PH, "return 2.0; }", "return 0.0; }", 1)]),
    ("X5", "r", [(PC, "                    band.reset();\n                    break;", "                    break;", 1)]),
    ("X6", "r", [(PC, "        if (touched[lane])\n            io[lane]", "        if (touched[lane] || stereoLayout)\n            io[lane]", 1)]),
    ("X7", "r", [(PC, "s.gainScale     = gainScaleRaw;", "s.gainScale     = 100.0f;", 1),
                 (PC, "eq::makeBandInput (now.raw, now.gainScale,", "eq::makeBandInput (now.raw, gainScaleRaw,", 1)]),
    ("X8", "r", [(PC, "        slot.valid = false;", "        (void) slot;", 1)]),
    ("X9", "r", [(PC, "    resetAllBands();\n\n    for (auto& slot : slots)", "    for (auto& slot : slots)", 1)]),
    ("X11", "r", [(PC, "setLatencySamples (0);", "setLatencySamples (1);", 1)]),
    ("X12", "r", [(PC, "designer.prepare (sampleRate);", "designer.prepare (48000.0); juce::ignoreUnused (sampleRate);", 1)]),
    ("X17", "r", [(PC, "                    // structural change and designs it then.\n                    if (next.active)", "                    // structural change and designs it then.\n                    if (false)", 1)]),
    ("X18", "r", [(PC, "    if (numActive == 0)\n        return;", "    if (numActive < 0)\n        return;", 1)]),
]


def replace_nth(text, old, new, nth):
    n = text.count(old)
    if n == 0:
        raise RuntimeError("pattern not found: " + old[:50])
    if nth == 0:
        return text.replace(old, new)
    if nth > n:
        raise RuntimeError("nth > count")
    idx = -1
    for _ in range(nth):
        idx = text.index(old, idx + 1)
    return text[:idx] + new + text[idx + len(old):]


def sh(cmd, cwd, log, timeout=900):
    with open(log, "w") as f:
        try:
            return subprocess.run(["/bin/bash", "-c", cmd], cwd=cwd, stdout=f, stderr=subprocess.STDOUT, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            f.write("\nTIMEOUT\n")
            return -99


def fails(log):
    out = []
    for line in open(log, errors="replace"):
        if "[FAIL]" in line:
            m = re.search(r"\[FAIL\]\s+(\S+(?: \([iv]+\))?)", line)
            out.append(m.group(1) if m else line.strip()[:40])
    return out


def tally(log):
    last = ""
    for line in open(log, errors="replace"):
        if re.search(r"\d+ PASS, \d+ FAIL", line):
            last = line.strip()
    return last


def run(mut):
    mid, which, edits = mut
    M = f"{MUT}/{mid}"
    shutil.rmtree(M, ignore_errors=True)
    os.makedirs(M)
    shutil.copytree(P + "/Source", M + "/Source")
    files = set()
    for f, old, new, nth in edits:
        path = f"{M}/Source/{f}"
        t = open(path).read()
        open(path, "w").write(replace_nth(t, old, new, nth))
        files.add(f)
    res = {"id": mid}
    inc_old = f"-I{P}/Source "
    inc_new = f"-I{M}/Source "
    if "d" in which:
        cmd = (f'/usr/bin/c++ -DGOLDEN_DIR=\\"{P}/tests/golden\\" {inc_new}-O3 -DNDEBUG -std=c++17 -arch arm64 '
               f'-mmacosx-version-min=10.13 -w {P}/tests/designer-harness/main.cpp -o {M}/designer')
        rc = sh(cmd, M, M + "/d-build.log")
        if rc != 0:
            res["d"] = f"BUILD rc {rc}"
        else:
            rc = sh(f"{M}/designer", M, M + "/d-run.log", 600)
            res["d"] = f"rc {rc} | {tally(M + '/d-run.log')} | FAIL: {', '.join(fails(M + '/d-run.log')) or 'none'}"
    if "r" in which:
        ok = True
        c = cc_proc.replace(inc_old, inc_new).replace(PROC_O, M + "/proc.o").replace(f"-c {P}/Source/PluginProcessor.cpp", f"-c {M}/Source/PluginProcessor.cpp") + " -w"
        assert inc_new in c and M + "/Source/PluginProcessor.cpp" in c
        if sh(c, REPO + "/build", M + "/r-build-proc.log") != 0:
            ok = False
        main_o = MAIN_O
        if ok and (files - {PC}):
            c = cc_main.replace(inc_old, inc_new).replace(MAIN_O, M + "/main.o") + " -w"
            if sh(c, REPO + "/build", M + "/r-build-main.log") != 0:
                ok = False
            main_o = M + "/main.o"
        if ok:
            c = link.replace(PROC_O, M + "/proc.o").replace(MAIN_O, main_o).replace("-o " + OUT, "-o " + M + "/render")
            assert "-o " + M + "/render" in c
            if sh(c, REPO + "/build", M + "/r-link.log") != 0:
                ok = False
        if not ok:
            res["r"] = "BUILD failed"
        else:
            rc = sh(f"{M}/render", M, M + "/r-run.log", 600)
            res["r"] = f"rc {rc} | {tally(M + '/r-run.log')} | FAIL: {', '.join(fails(M + '/r-run.log')) or 'none'}"
    line = f"{mid}\n   designer: {res.get('d', '-')}\n   render:   {res.get('r', '-')}"
    print(line, flush=True)
    return line


if __name__ == "__main__":
    sel = sys.argv[1:]
    todo = [m for m in MUTANTS if not sel or m[0] in sel]
    with ThreadPoolExecutor(max_workers=4) as ex:
        out = list(ex.map(run, todo))
    open(MUT + "/results-" + ("all" if not sel else "_".join(sel)) + ".txt", "w").write("\n".join(out) + "\n")
