#!/usr/bin/env python3
"""Scratch mutation runner for the Phase 3.1 owed-row port. Reads only: the repo's
mutation_run.py (MUTANTS list) and build/ (compile/link commands, JUCE objects).
Writes only under SCR. usage: run.py [--full] ID ..."""
import os, re, shutil, subprocess, sys, importlib.util, time
from concurrent.futures import ThreadPoolExecutor
SCR = os.path.join(os.environ.get('TMPDIR', '/tmp'), 'opeq-mut')  # scratch; run from a copy of the tree placed at SCR/tree
REPO = "/Users/taylorbrook/Dev/VST-development"
BUILD = REPO + "/build"
RP = REPO + "/plugins/O-ParametricEQ"
T = SCR + "/tree"
WORK = SCR + "/work"
LID = os.environ.get('LID', '')  # a 0.6.0 --dump-sessions dir
os.environ.setdefault("WORK", WORK); os.environ.setdefault("LID", LID)
spec = importlib.util.spec_from_file_location("mr", RP + "/.planning/stages/2-dsp/verify-2.6/mutation_run.py")
mr = importlib.util.module_from_spec(spec); spec.loader.exec_module(mr)
MUT = {m[0]: m for m in mr.MUTANTS}
RCMDS, RTU, ROUT = mr.RCMDS, mr.RTU, mr.ROUT
RSRC = {"main": RP + "/tests/render-harness/main.cpp", "PluginProcessor": RP + "/Source/PluginProcessor.cpp",
        "SpectrumAnalyzer": RP + "/Source/analysis/SpectrumAnalyzer.cpp", "DisplayModel": RP + "/Source/analysis/DisplayModel.cpp"}

def run(mid, full):
    _, what, edits = MUT[mid]
    M = f"{WORK}/{mid}"
    shutil.rmtree(M, ignore_errors=True); os.makedirs(M)
    shutil.copytree(T + "/Source", M + "/Source")
    for f, old, new, nth in edits:
        path = f"{M}/Source/{f}"; t = open(path).read()
        open(path, "w").write(mr.replace_nth(t, old, new, nth))
    objs = dict(RTU)
    for tu, obj in RTU.items():
        src_new = T + "/tests/render-harness/main-only.cpp" if tu == "main" else RSRC[tu].replace(RP + "/Source", M + "/Source")
        c = mr.cc_for(RCMDS, obj).replace(f"-I{RP}/Source ", f"-I{M}/Source ").replace(f"-c {RSRC[tu]}", f"-c {src_new}") \
            .replace(f"-o {obj} ", f"-o {M}/{tu}.o ")
        assert f"-o {M}/{tu}.o " in c and f"-c {src_new}" in c and f"-I{M}/Source " in c
        if mr.sh(c, BUILD, f"{M}/build-{tu}.log") != 0:
            line = f"{mid}  BUILD FAILED ({tu})"; print(line, flush=True); return line
        objs[tu] = f"{M}/{tu}.o"
    l = mr.link_for(RCMDS, ROUT)
    for tu, obj in RTU.items():
        assert f" {obj} " in l
        l = l.replace(f" {obj} ", f" {objs[tu]} ")
    l = l.replace("-o " + ROUT, "-o " + M + "/render")
    if mr.sh(l, BUILD, M + "/link.log") != 0:
        line = f"{mid}  LINK FAILED"; print(line, flush=True); return line
    env = dict(os.environ)
    if not full: env["OPEQ_OWED26_ONLY"] = "1"
    t0 = time.time()
    rc = mr.sh(f"{M}/render --lid-dir {LID}", M, M + "/run.log", env=env)
    dt = time.time() - t0
    line = f"{mid}  {what}\n   render{' FULL' if full else ''}: {mr.result(M + '/run.log', rc)}  ({dt:.1f} s)"
    print(line, flush=True)
    for f in os.listdir(M):
        if f.endswith(".o"): os.remove(os.path.join(M, f))
    shutil.rmtree(M + "/Source", ignore_errors=True)
    return line

if __name__ == "__main__":
    a = sys.argv[1:]; full = "--full" in a
    jobs = int(next((x[2:] for x in a if x.startswith("-j")), "3"))
    ids = [x for x in a if not x.startswith("-")]
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        out = list(ex.map(lambda m: run(m, full), ids))
    open(f"{WORK}/results-{'full-' if full else ''}{'_'.join(ids)}.txt", "w").write("\n".join(out) + "\n")
