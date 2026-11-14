#!/usr/bin/env python3
"""Mutation run for O-ParametricEQ Phase 2.3 verify. Nothing in the repo or build/ is written.
Each mutant: a copy of Source/ with one edit, compiled against the unmodified harness sources and
the verify probe, then run through the designer harness (d), the render harness (r) and the probe (p)."""
import os, re, shutil, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

REPO = "/Users/taylorbrook/Dev/VST-development"
P = REPO + "/plugins/O-ParametricEQ"
MUT = os.path.dirname(os.path.abspath(__file__))
PROBE = os.path.dirname(MUT) + "/probe"
OBJ = "plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir/"
PROC_O = OBJ + "Source/PluginProcessor.cpp.o"
MAIN_O = OBJ + "tests/render-harness/main.cpp.o"
OUT = "plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness"

cc_proc = open(MUT + "/cc_proc.txt").read().strip()
cc_main = open(MUT + "/cc_main.txt").read().strip()
link = open(MUT + "/link.txt").read().strip()
link_probe = open(PROBE + "/link_probe.txt").read().strip()

CR, SE, BA, RO, BP = "dsp/ControlRamp.h", "dsp/SectionEngine.h", "dsp/Band.h", "dsp/Router.h", "dsp/BandParams.h"
PC = "PluginProcessor.cpp"

# id, what, harnesses, [(file, old, new, nth)]   nth: 0 = all occurrences, k = k-th only
MUTANTS = [
    ("M0", "unmodified control", "drp", []),
    # --- the control ramp
    ("C1", "ramp length 10 ms", "drp", [(CR, "const double ticks = rampSeconds * sampleRateHz / grid;", "const double ticks = 0.5 * rampSeconds * sampleRateHz / grid;", 1)]),
    ("C3", "arrival designs from the ramp values, not the target's settings", "drp",
        [(CR, "        if (counter <= 0)\n            return target;\n\n        BandSettings s = target;", "        BandSettings s = target;", 1)]),
    ("C4", "K = 32 at 48 kHz", "drp", [(CR, "(sampleRateHz <= 48000.0) ? 16", "(sampleRateHz <= 48000.0) ? 32", 1)]),
    ("C5", "gain not ramped (jumps to the target at the first tick)", "drp", [(CR, "            gainDb += incGainDb;", "            gainDb = targetGainDb;", 1)]),
    ("C6", "Q not ramped", "drp", [(CR, "            lnQ += incLnQ;", "            lnQ = targetLnQ;", 1)]),
    ("C7", "frequency not ramped", "drp", [(CR, "            lnF += incLnF;", "            lnF = targetLnF;", 1)]),
    ("C8", "a retarget while running keeps the old counter", "drp", [(CR, "        counter = length;", "        counter = (counter > 0) ? counter : length;", 1)]),
    # --- the band
    ("B3", "setDesign clears the state", "drp", [(BA, "        numSections = count;\n", "        numSections = count; reset();\n", 1)]),
    ("B5", "tick step 1 keeps the accumulated sum", "drp",
        [(BA, "            for (int i = 0; i < numSections; ++i)\n                current[i] = intervalEnd[i];\n\n            cancelInterpolation();", "            cancelInterpolation();", 1)]),
    ("B6", "processPair steps the coefficients twice per sample", "drp",
        [(BA, "            stepCoefficients();\n            samples0[i] = tick (st0, samples0[i]);", "            stepCoefficients();\n            samples0[i] = tick (st0, samples0[i]);\n            stepCoefficients();", 1)]),
    ("B10", "moving flag latched by the last interval", "drp",
        [(BA, "const bool moving = ramp.isRunning() || ! sameValue (offset, lastOffsetDb);", "const bool moving = ramp.isRunning() || wasInterpolating || ! sameValue (offset, lastOffsetDb);", 1)]),
    ("B13", "snapToTarget does nothing", "drp", [(BA, "        if (! isMoving())\n            return;\n\n        const BandSettings target = hasPending", "        if (true)\n            return;\n\n        const BandSettings target = hasPending", 1)]),
    ("B14", "processLane does not step on the ramping path", "drp", [(BA, "                stepCoefficients();\n                samples[i] = tick (st, samples[i]);", "                samples[i] = tick (st, samples[i]);", 1)]),
    ("B15", "snapTo keeps a pending target", "drp", [(BA, "        ramp.snap (target);\n        hasPending = false;", "        ramp.snap (target);", 1)]),
    ("B17", "setDesign does not cancel an interpolation", "drp", [(BA, "            sections[i] = SectionRuntime();\n        }\n\n        cancelInterpolation();", "            sections[i] = SectionRuntime();\n        }", 1)]),
    # --- the router
    ("B7", "Mid loop does not step on the ramping path", "drp", [(RO, "                band.stepCoefficients();\n\n                const double l = left[i];\n                const double r = right[i];\n                const double m", "                const double l = left[i];\n                const double r = right[i];\n                const double m", 1)]),
    ("B7s", "Side loop does not step on the ramping path", "drp", [(RO, "                band.stepCoefficients();\n\n                const double l = left[i];\n                const double r = right[i];\n                const double s", "                const double l = left[i];\n                const double r = right[i];\n                const double s", 1)]),
    ("R7", "Mid uses L + R (router and prediction)", "drp", [(RO, "const double m = 0.5 * (l + r);", "const double m = (l + r);", 0)]),
    # --- the ramping tick
    ("SE1", "a1, a2 not re-derived while ramping", "drp", [(SE, "        rt.svf.a1 = 1.0 / (1.0 + p.g * (p.g + p.k));\n        rt.svf.a2 = p.g * rt.svf.a1;\n", "", 1)]),
    ("SE2", "one-pole Gc held while ramping", "drp", [(SE, "        rt.onePole.Gc = p.G / (1.0 + p.G);\n", "", 1)]),
    ("SE3", "one-pole mix (cL, cH) not interpolated", "drp", [(SE, "        p.cL += d.cL;\n        p.cH += d.cH;\n", "", 1)]),
    ("SE5", "SVF mix (mL, mB, mH) not interpolated", "drp", [(SE, "        p.mL += d.mL;\n        p.mB += d.mB;\n        p.mH += d.mH;\n", "", 1)]),
    # --- raw -> settings
    ("P3", "band bypass ignored", "drp", [(BP, "in.active = enabled && ! bypass;", "in.active = enabled; (void) bypass;", 1)]),
    ("P5", "a type change classified continuous", "drp", [(BP, "        || current.settings.type != next.settings.type\n", "", 1)]),
    ("P10", "no canonical gain on the gainless types", "drp", [(BP, "    if (! isGainType (in.settings.type))\n        in.settings.gainDbEff = 0.0;\n", "", 1)]),
    # --- the processor
    ("X7", "gain_scale missing from the change snapshot", "rp", [(PC, "s.gainScale     = gainScaleRaw;", "s.gainScale     = 100.0f; juce::ignoreUnused (gainScaleRaw);", 1),
                 (PC, "eq::makeBandInput (now.raw, now.gainScale, 0.0)", "eq::makeBandInput (now.raw, gainScaleRaw, 0.0)", 1)]),
    ("X8", "prepareToPlay does not invalidate the snapshots", "rp", [(PC, "        slot.valid = false;", "        (void) slot;", 1)]),
    ("X20", "processBlockBypassed does not advance the clock", "rp", [(PC, "    juce::ignoreUnused (advanceTickClock (buffer.getNumSamples()));", "    juce::ignoreUnused (buffer.getNumSamples());", 1)]),
    ("X21", "the bypass branch does not advance the clock", "rp", [(PC, "    {\n        clearAndSnapAllBands();\n        return;\n    }", "    {\n        tickClock = blockStart;\n        clearAndSnapAllBands();\n        return;\n    }", 1)]),
    ("X22", "the no-active-band return does not advance the clock", "rp", [(PC, "    if (numActive == 0)\n        return;", "    if (numActive == 0)\n    {\n        tickClock = blockStart;\n        return;\n    }", 1)]),
    ("X23", "non-finite check removed", "rp", [(PC, "        if (! finite)\n            clearAndSnapAllBands();", "        juce::ignoreUnused (finite);", 1)]),
    ("X25", "a continuous change snaps (no ramp)", "rp", [(PC, "                    if (next.active)\n                        band.setTarget (next.settings);", "                    if (next.active)\n                        band.snapTo (next.settings, designer);", 1)]),
    ("X26", "a structural change keeps the state", "rp", [(PC, "                        band.snapTo (next.settings, designer);\n                        band.reset();", "                        band.snapTo (next.settings, designer);", 1)]),
    ("X28", "the tick phase restarts at every chunk", "rp", [(PC, "        const int n = juce::jmin (preparedBlockSize, numSamples - start);\n", "        const int n = juce::jmin (preparedBlockSize, numSamples - start);\n        phase = static_cast<int> (blockStart % static_cast<std::uint64_t> (grid));\n", 1)]),
    ("X29", "prepareToPlay does not zero the clock", "rp", [(PC, "    tickClock = 0;\n\n    for (auto& slot : slots)", "    for (auto& slot : slots)", 1)]),
    ("X35", "prepareToPlay does not hand the bands the grid", "rp", [(PC, "        band.setControlGrid (controlGrid, rampTicks);\n", "        juce::ignoreUnused (rampTicks);\n", 1)]),
]


def replace_nth(text, old, new, nth):
    n = text.count(old)
    if n == 0:
        raise RuntimeError("pattern not found: " + old[:60])
    if nth == 0:
        return text.replace(old, new)
    if nth > n:
        raise RuntimeError("nth > count")
    idx = -1
    for _ in range(nth):
        idx = text.index(old, idx + 1)
    return text[:idx] + new + text[idx + len(old):]


def sh(cmd, cwd, log, timeout=1500):
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


def probe_fails(log):
    return sum(1 for line in open(log, errors="replace") if "[FAIL]" in line)


def tally(log):
    last = ""
    for line in open(log, errors="replace"):
        if re.search(r"\d+ PASS, \d+ FAIL", line):
            last = line.strip()
    return last


def run(mut):
    mid, what, which, edits = mut
    M = f"{MUT}/{mid}"
    shutil.rmtree(M, ignore_errors=True)
    os.makedirs(M)
    shutil.copytree(P + "/Source", M + "/Source")
    files = set()
    try:
        for f, old, new, nth in edits:
            path = f"{M}/Source/{f}"
            t = open(path).read()
            open(path, "w").write(replace_nth(t, old, new, nth))
            files.add(f)
    except RuntimeError as e:
        line = f"{mid}  {what}\n   EDIT FAILED: {e}"
        print(line, flush=True)
        return line
    res = {}
    inc_old = f"-I{P}/Source "
    inc_new = f"-I{M}/Source "
    if "d" in which:
        cmd = (f'/usr/bin/c++ -DGOLDEN_DIR=\\"{P}/tests/golden\\" {inc_new}-O3 -DNDEBUG -std=c++17 -arch arm64 '
               f'-mmacosx-version-min=10.13 -w {P}/tests/designer-harness/main.cpp -o {M}/designer')
        rc = sh(cmd, M, M + "/d-build.log")
        if rc != 0:
            res["d"] = f"BUILD rc {rc}"
        else:
            rc = sh(f"{M}/designer", M, M + "/d-run.log", 1500)
            res["d"] = f"rc {rc} | {tally(M + '/d-run.log')} | FAIL: {', '.join(fails(M + '/d-run.log')) or 'none'}"
    ok = True
    c = cc_proc.replace(inc_old, inc_new).replace(PROC_O, M + "/proc.o").replace(f"-c {P}/Source/PluginProcessor.cpp", f"-c {M}/Source/PluginProcessor.cpp") + " -w"
    assert inc_new in c and M + "/Source/PluginProcessor.cpp" in c
    if sh(c, REPO + "/build", M + "/r-build-proc.log") != 0:
        ok = False
    if "r" in which:
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
            rc = sh(f"{M}/render", M, M + "/r-run.log", 1500)
            res["r"] = f"rc {rc} | {tally(M + '/r-run.log')} | FAIL: {', '.join(fails(M + '/r-run.log')) or 'none'}"
    if "p" in which and ok:
        c = link_probe.replace(PROC_O, M + "/proc.o").replace("-o " + PROBE + "/probe", "-o " + M + "/probe")
        assert "-o " + M + "/probe" in c
        if sh(c, REPO + "/build", M + "/p-link.log") != 0:
            res["p"] = "BUILD failed"
        else:
            rc = sh(f"{M}/probe", M, M + "/p-run.log", 600)
            res["p"] = f"rc {rc} | {tally(M + '/p-run.log')} | {probe_fails(M + '/p-run.log')} rows fail"
    line = f"{mid}  {what}\n   designer: {res.get('d', '-')}\n   render:   {res.get('r', '-')}\n   probe:    {res.get('p', '-')}"
    print(line, flush=True)
    return line


if __name__ == "__main__":
    sel = [a for a in sys.argv[1:] if not a.startswith("-j")]
    jobs = next((int(a[2:]) for a in sys.argv[1:] if a.startswith("-j")), 4)
    todo = [m for m in MUTANTS if not sel or m[0] in sel]
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        out = list(ex.map(run, todo))
    open(MUT + "/results-" + ("all" if not sel else "_".join(sel)) + ".txt", "w").write("\n".join(out) + "\n")
