#!/usr/bin/env python3
"""Mutation run for O-ParametricEQ Phase 2.5 (OutputStage.h, AutoGain.h, the processor's output-stage glue).
Nothing in the repo or build/ is written: each mutant is a copy of Source/ with one edit, compiled into
this directory against the unmodified harness sources (and the unmodified JUCE module objects in build/),
then run through the designer harness (d) and the render harness (r).

Optional third checker (p): --probe [build.sh path] runs `SRC_DIR=<mutant Source> OUT_DIR=<mutant dir>
bash build.sh`; the probe binary is <mutant dir>/probe, or else the last stdout line of build.sh if it is an
executable path. Its [FAIL] rows are parsed like the harnesses'.

usage: mutation_run.py [-jN] [--probe [path/to/build.sh]] [--no-d] [--no-r] [ID ...]
"""
import os, re, shutil, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

REPO = "/Users/taylorbrook/Dev/VST-development"
P = REPO + "/plugins/O-ParametricEQ"
MUT = os.path.dirname(os.path.abspath(__file__))
DEFAULT_PROBE_SH = os.path.dirname(MUT) + "/probe/build.sh"
OBJ = "plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir/"
PROC_O = OBJ + "Source/PluginProcessor.cpp.o"
MAIN_O = OBJ + "tests/render-harness/main.cpp.o"
OUT = "plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness"
TIMEOUT = 3600

cc_proc = open(MUT + "/cc_proc.txt").read().strip()
cc_main = open(MUT + "/cc_main.txt").read().strip()
link = open(MUT + "/link.txt").read().strip()

OS, AG, PC = "dsp/OutputStage.h", "dsp/AutoGain.h", "PluginProcessor.cpp"
STAGE_CALL = """            outputStage.process (io[0] != nullptr ? work[0] + pos : nullptr,
                                 io[1] != nullptr ? work[1] + pos : nullptr,
                                 m);
"""
BANDS_LOOP = """            for (int a = 0; a < numListed; ++a)
                bands[static_cast<std::size_t> (listed[static_cast<std::size_t> (a)])]
                    .process (work[0] + pos, work[1] + pos, m, stereoLayout, fadeTable, designer);
"""
MUTANTS = [
    ("M0", "unmodified control", []),
    # --- OutputStage.h
    ("OS1", "settle X 13.0622 -> 10 (slower smoother)", [(OS, "smootherSettleX = 13.0622;", "smootherSettleX = 10.0;", 1)]),
    ("OS2", "two poles instead of four", [(OS, "        y3 += a * (y2 - y3);\n        y4 += a * (y3 - y4);", "        y3 = y2;\n        y4 = y3;", 1)]),
    ("OS3", "snap tolerance 1e-9 -> 1e-6", [(OS, "smootherSnapTolerance = 1.0e-9;", "smootherSnapTolerance = 1.0e-6;", 1)]),
    ("OS4", "never snaps (never settles after a move)", [(OS, "            y1 = y2 = y3 = y4 = T;\n            settled = true;", "            (void) 0;", 1)]),
    ("OS5", "setTarget leaves the smoother settled (hard step)", [(OS, "        settled = sameGain (y1, target) && sameGain (y2, target) && sameGain (y3, target) && sameGain (y4, target);", "        settled = true;", 1)]),
    ("OS6", "setTarget restarts all stages from the current output", [(OS, "        settled = sameGain (y1, target) && sameGain (y2, target) && sameGain (y3, target) && sameGain (y4, target);", "        y1 = y2 = y3 = y4;\n        settled = sameGain (y4, target);", 1)]),
    ("OS9", "settled path drops auto gain (g = og only)", [(OS, "            const double g = ag.getTarget() * og.getTarget();", "            const double g = og.getTarget();", 1)]),
    ("OS10", "ramp path does not gain R", [(OS, "            if (R != nullptr)\n                R[i] *= g;\n", "", 1)]),
    ("OS11", "output gain clamp +-24 dB", [(OS, "outputGainMinDb = -36.0;", "outputGainMinDb = -24.0;", 1), (OS, "outputGainMaxDb = 36.0;", "outputGainMaxDb = 24.0;", 1)]),
    ("OS12", "non-finite output gain reads +36 dB", [(OS, "std::isfinite (rawDb) ? rawDb : 0.0", "std::isfinite (rawDb) ? rawDb : 36.0", 1)]),
    ("OS13", "isUnity ignores the pending output target", [(OS, "og.isSettledAt (1.0) && sameGain (pendingOg, 1.0) && ag.isSettledAt (1.0)", "og.isSettledAt (1.0) && ag.isSettledAt (1.0)", 1)]),
    ("OS14", "isUnity ignores auto gain", [(OS, "og.isSettledAt (1.0) && sameGain (pendingOg, 1.0) && ag.isSettledAt (1.0)", "og.isSettledAt (1.0) && sameGain (pendingOg, 1.0)", 1)]),
    ("OS15", "auto-gain time 100 -> 50 ms", [(OS, "autoGainSeconds = 0.100;", "autoGainSeconds = 0.050;", 1)]),
    ("OS16", "output-gain time 20 -> 25 ms", [(OS, "outputGainSeconds = 0.020;", "outputGainSeconds = 0.025;", 1)]),
    ("OS18", "snap tests the last stage only", [(OS, "        if (std::abs (y1 - T) <= smootherSnapTolerance\n            && std::abs (y2 - T) <= smootherSnapTolerance\n            && std::abs (y3 - T) <= smootherSnapTolerance\n            && std::abs (y4 - T) <= smootherSnapTolerance)", "        if (std::abs (y4 - T) <= smootherSnapTolerance)", 1)]),
    ("OS19", "constant path taken when only og is settled (auto gain jumps)", [(OS, "        if (ag.isSettled() && og.isSettled())", "        if (og.isSettled())", 1)]),
    # --- AutoGain.h
    ("AG1", "256 points", [(AG, "autoGainPoints = 512;", "autoGainPoints = 256;", 1)]),
    ("AG2", "inclusive grid instead of midpoints", [(AG, "const double u = (static_cast<double> (i) + 0.5) / static_cast<double> (autoGainPoints);", "const double u = static_cast<double> (i) / static_cast<double> (autoGainPoints - 1);", 1)]),
    ("AG3", "top fraction 0.45 -> 0.5 fs", [(AG, "autoGainTopFraction = 0.45;", "autoGainTopFraction = 0.5;", 1)]),
    ("AG4", "top 16 -> 20 kHz", [(AG, "autoGainHighHz = 16000.0;", "autoGainHighHz = 20000.0;", 1)]),
    ("AG5", "bottom 40 -> 20 Hz", [(AG, "autoGainLowHz = 40.0;", "autoGainLowHz = 20.0;", 1)]),
    ("AG6", "clamp top +12 -> +18", [(AG, "autoGainMaxDb = 12.0;", "autoGainMaxDb = 18.0;", 1)]),
    ("AG7", "clamp bottom -18 -> -24", [(AG, "autoGainMinDb = -18.0;", "autoGainMinDb = -24.0;", 1)]),
    ("AG8", "Side bands contribute on stereo (designed, enter no lane)", [(AG, "        return ! (stereo && input.placement == Placement::side);", "        (void) stereo; return true;", 1)]),
    ("AG9", "isDirty never reports an edited evaluated band", [(AG, "            if (! sameBandInput (in, band.lastEval)\n                && (band.contributes || contributes (in, stereo)))\n                return true;", "            (void) in;", 1)]),
    ("AG10", "isDirty misses non-contributing -> contributing", [(AG, "                && (band.contributes || contributes (in, stereo)))", "                && band.contributes)", 1)]),
    ("AG11", "isDirty ignores an unevaluated band", [(AG, "            if (! band.evaluated)\n            {\n                if (contributes (in, stereo))\n                    return true;\n\n                continue;\n            }", "            if (! band.evaluated)\n                continue;", 1)]),
    ("AG12", "a band is re-designed only when active changes", [(AG, "            if (! force && band.evaluated && sameBandInput (in, band.lastEval))", "            if (! force && band.evaluated && in.active == band.lastEval.active)", 1)]),
    ("AG13", "everything-cut guard reads 0 dB", [(AG, "        else if (mean <= 1.0e-30)\n            db = autoGainMaxDb;", "        else if (mean <= 1.0e-30)\n            db = 0.0;", 1)]),
    ("AG14", "inf guard reads 0 dB", [(AG, "        if (std::isinf (mean))\n            db = autoGainMinDb;", "        if (std::isinf (mean))\n            db = 0.0;", 1)]),
    ("AG15", "mean of the L lane only", [(AG, "            sum += 0.5 * (laneL[ui] + laneR[ui]);", "            sum += laneL[ui];", 1)]),
    ("AG16", "Mid band left out of the R lane", [(AG, "const bool intoR = ! stereo || placement == Placement::stereo || placement == Placement::mid || placement == Placement::right;", "const bool intoR = ! stereo || placement == Placement::stereo || placement == Placement::right;", 1)]),
    ("AG18", "a 0 dB gain type contributes", [(AG, "        if (isGainType (input.settings.type) && sameValue (input.settings.gainDbEff, 0.0))\n            return false;\n", "", 1)]),
    ("AG19", "refresh period one tick longer", [(AG, "    const double r = std::round (autoGainRefreshSamples48k / 48000.0 * sampleRateHz / k);", "    const double r = std::round (autoGainRefreshSamples48k / 48000.0 * sampleRateHz / k) + 1.0;", 1)]),
    # --- PluginProcessor.cpp
    ("PX1", "step 5 returns on an empty list whatever the stage", [(PC, "    if (numListed == 0 && stageIdleForBlock())\n", "    if (numListed == 0)\n", 1)]),
    ("PX2", "step 6: no lane conversion for a non-idle stage", [(PC, "        touched[0] = true;\n        touched[1] = stereoLayout;\n", "", 1)]),
    ("PX3", "step 6: non-idle stage converts L only", [(PC, "        touched[1] = stereoLayout;\n", "        touched[1] = false;\n", 1)]),
    ("PX4", "idle predicate ignores agOn != agWanted", [(PC, "    if (! outputStage.isUnity() || agOn != agWanted)\n", "    if (! outputStage.isUnity())\n", 1)]),
    ("PX5", "idle predicate ignores dirty bands", [(PC, "    return ! autoGain.isDirty (desired.data(), stereoLayout);", "    (void) desired; return true;", 1)]),
    ("PX6", "no seeding at the step-4 restart (first block, un-bypass)", [(PC, "    if (restart)\n        seedOutputStage();\n", "", 1)]),
    ("PX7", "no seeding at the step-7 restart", [(PC, "                    seedOutputStage();\n\n                    numListed = collectRunningBands (listed);", "                    numListed = collectRunningBands (listed);", 1)]),
    ("PX8", "seeding leaves auto gain at 1.0 (ramps in)", [(PC, "        autoTarget = autoGain.evaluateAll (desired.data(), stereoLayout, designer);", "        (void) autoGain.evaluateAll (desired.data(), stereoLayout, designer);", 1)]),
    ("PX9", "every tick is a refresh tick", [(PC, "                if (tickIndex % static_cast<std::uint64_t> (juce::jmax (1, refreshTicks)) == 0)", "                if ((void) tickIndex, true)", 1)]),
    ("PX10", "refresh index from the block-relative position", [(PC, "                const std::uint64_t tickIndex = (blockStart + static_cast<std::uint64_t> (start + pos))", "                const std::uint64_t tickIndex = (static_cast<std::uint64_t> (start + pos))", 1)]),
    ("PX11", "output target applied at the block start, not the next tick", [(PC, "    outputStage.setOutputPending (eq::outputGainTarget (static_cast<double> (rawValues[kRawOutputGain]->load())));", "    outputStage.setOutputPending (eq::outputGainTarget (static_cast<double> (rawValues[kRawOutputGain]->load()))); outputStage.applyOutputPending();", 1)]),
    ("PX13", "no peak update on the processBlockBypassed route", [(PC, "    processAudio (buffer, true);\n    updateOutputPeak (buffer);", "    processAudio (buffer, true);", 1)]),
    ("PX14", "peak overwritten per block (no max-hold)", [(PC, "        if (peak > slot.load (std::memory_order_relaxed))\n            slot.store (peak, std::memory_order_relaxed);", "        slot.store (peak, std::memory_order_relaxed);", 1)]),
    ("PX16", "stage before the bands", [(PC, STAGE_CALL, "", 1), (PC, BANDS_LOOP, STAGE_CALL + BANDS_LOOP, 1)]),
    ("PX17", "step-7 jump on an empty list whatever the stage (2.4 form)", [(PC, "                    if (numListed == 0 && outputStage.isUnity())", "                    if (numListed == 0)", 1)]),
    ("PX18", "stage given no R lane", [(PC, "                                 io[1] != nullptr ? work[1] + pos : nullptr,", "                                 nullptr,", 1)]),
    ("PX20", "auto_gain never takes effect at a refresh (seeding only)", [(PC, "                    agOn = agWanted;\n\n                    if (agOn)", "                    if (agOn)", 1)]),
]


def replace_nth(text, old, new, nth):
    n = text.count(old)
    if n == 0:
        raise RuntimeError("pattern not found: " + old[:70].replace("\n", "\\n"))
    if nth == 0:
        return text.replace(old, new)
    if n != 1 and nth == 1:
        raise RuntimeError(f"pattern occurs {n} times (expected once): " + old[:70].replace("\n", "\\n"))
    if nth > n:
        raise RuntimeError("nth > count")
    idx = -1
    for _ in range(nth):
        idx = text.index(old, idx + 1)
    return text[:idx] + new + text[idx + len(old):]


def sh(cmd, cwd, log, timeout=TIMEOUT, env=None):
    with open(log, "w") as f:
        try:
            return subprocess.run(["/bin/bash", "-c", cmd], cwd=cwd, stdout=f, stderr=subprocess.STDOUT, timeout=timeout,
                                  env=env).returncode
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
    return last or "no summary line"


def result(log, rc):
    f = fails(log)
    t = "TIMEOUT" if rc == -99 else f"rc {rc}"
    if rc < 0 and rc != -99:
        t += f" (signal {-rc})"
    return f"{t} | {tally(log)} | FAIL: {', '.join(f) or 'none'}"


def run(mut, opts):
    mid, what, edits = mut
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
    if opts["d"] and (not edits or (files - {PC})):
        cmd = (f'/usr/bin/c++ -DGOLDEN_DIR=\\"{P}/tests/golden\\" {inc_new}-O3 -DNDEBUG -std=c++17 -arch arm64 '
               f'-mmacosx-version-min=10.13 -O3 -w {P}/tests/designer-harness/main.cpp -o {M}/designer')
        rc = sh(cmd, M, M + "/d-build.log")
        if rc != 0:
            res["d"] = f"BUILD rc {rc}"
        else:
            rc = sh(f"{M}/designer", M, M + "/d-run.log")
            res["d"] = result(M + "/d-run.log", rc)
    if opts["r"]:
        ok = True
        c = cc_proc.replace(inc_old, inc_new).replace(PROC_O, M + "/proc.o") \
                   .replace(f"-c {P}/Source/PluginProcessor.cpp", f"-c {M}/Source/PluginProcessor.cpp") + " -w"
        assert inc_new in c and M + "/Source/PluginProcessor.cpp" in c and PROC_O not in c
        if sh(c, REPO + "/build", M + "/r-build-proc.log") != 0:
            ok = False
        main_o = MAIN_O
        # main.cpp includes PluginProcessor.h and the dsp headers: recompile it whenever a header changed
        if ok and (files - {PC}):
            c = cc_main.replace(inc_old, inc_new).replace(MAIN_O, M + "/main.o") + " -w"
            assert MAIN_O not in c
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
            rc = sh(f"{M}/render", M, M + "/r-run.log")
            res["r"] = result(M + "/r-run.log", rc)
    if opts["p"]:
        env = dict(os.environ, SRC_DIR=M + "/Source", OUT_DIR=M)
        rc = sh(f"bash {opts['probe_sh']}", os.path.dirname(opts["probe_sh"]), M + "/p-build.log", env=env)
        binary = M + "/probe"
        if not os.access(binary, os.X_OK):
            lines = [l.strip() for l in open(M + "/p-build.log", errors="replace") if l.strip()]
            if lines and os.access(lines[-1], os.X_OK):
                binary = lines[-1]
        if rc != 0 or not os.access(binary, os.X_OK):
            res["p"] = f"BUILD rc {rc}"
        else:
            rc = sh(binary, M, M + "/p-run.log")
            res["p"] = result(M + "/p-run.log", rc)
    line = f"{mid}  {what}\n   designer: {res.get('d', '-')}\n   render:   {res.get('r', '-')}"
    if opts["p"]:
        line += f"\n   probe:    {res.get('p', '-')}"
    print(line, flush=True)
    return line


if __name__ == "__main__":
    args = sys.argv[1:]
    opts = {"d": "--no-d" not in args, "r": "--no-r" not in args, "p": False, "probe_sh": DEFAULT_PROBE_SH}
    sel, jobs, i = [], 7, 0
    while i < len(args):
        a = args[i]
        if a.startswith("-j"):
            jobs = int(a[2:])
        elif a == "--probe":
            opts["p"] = True
            if i + 1 < len(args) and args[i + 1].endswith(".sh"):
                opts["probe_sh"] = os.path.abspath(args[i + 1]); i += 1
        elif a in ("--no-d", "--no-r"):
            pass
        elif a == "--check":
            # only apply the edits, report any that do not apply exactly
            for mid, what, edits in MUTANTS:
                for f, old, new, nth in edits:
                    try:
                        replace_nth(open(f"{P}/Source/{f}").read(), old, new, nth)
                    except RuntimeError as e:
                        print(mid, "EDIT FAILED:", e)
            print(len(MUTANTS), "mutants checked"); sys.exit(0)
        else:
            sel.append(a)
        i += 1
    todo = [m for m in MUTANTS if not sel or m[0] in sel]
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        out = list(ex.map(lambda m: run(m, opts), todo))
    name = "all" if not sel else "_".join(sel)
    open(MUT + "/results-" + name + ".txt", "w").write("\n".join(out) + "\n")
