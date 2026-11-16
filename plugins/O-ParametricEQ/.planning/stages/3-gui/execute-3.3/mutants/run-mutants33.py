#!/usr/bin/env python3
"""PLAN-3.3 T11 mutant control (from execute-3.2/mutants/run-mutants.py).

Each mutant is a textual edit to a scratch copy of Source/ (never the repo
tree). Depending on its target it is scored by:
  - bench:   the editor bench rebuilt against the copy (-DPEQ_SRC), one run
             (1 cycle, 1 rep, 3 s idle, no size rows); verdict33.py names the
             failing rows;
  - harness: the render harness built from the repo build's commands with the
             copy's Source (build-harness.sh), run with --rows33;
  - node:    tests/meter-ballistics/ballistics.mjs on the copy's index.html.
A mutant is CAUGHT when every one of its target rows fails. The unmodified
copy runs first as the control of each scorer and must pass.

usage: run-mutants33.py <scratch dir> [mutant ids...]
"""
import json, os, re, shutil, subprocess, sys, time

REPO = "/Users/taylorbrook/Dev/VST-development"
PEQ = f"{REPO}/plugins/O-ParametricEQ"
HERE = os.path.dirname(os.path.abspath(__file__))
BENCH = os.path.join(HERE, "..", "editor-bench")
SCR = os.path.abspath(sys.argv[1])
ONLY = sys.argv[2:]
SRC = os.path.join(SCR, "Source")
BUILD = os.path.join(SCR, "build-mut")
LOCK = os.path.join(BENCH, "lockstate")
HTML = "ui/public/index.html"
EDITOR = "PluginEditor.cpp"
PROC = "PluginProcessor.cpp"
LISTEN = "dsp/Listen.h"
PEAK = "editor/PeakHold.h"
ANALYZER = "analysis/SpectrumAnalyzer.cpp"
FILES = (HTML, EDITOR, PROC, LISTEN, PEAK, ANALYZER)

# (id, file, old, new, scorer, targets)
MUTANTS = [
    ("control-bench", None, None, None, "bench", []),
    ("control-harness", None, None, None, "harness", []),
    ("control-node", None, None, None, "node", []),
    ("listen-flag-never-set", LISTEN, "            nonFiniteVoice = true;\n", "            (void) 0;\n", "harness", ["N1e"]),
    ("flag-inside-or", PROC,
     [("        if (listen.takeNonFiniteVoice())\n            finite = false;\n\n        if (! finite)\n",
       "        if (! finite || listen.takeNonFiniteVoice())\n")], None, "harness", ["N1e-u"]),
    ("process-skipped-while-off", EDITOR, "    analyzer->process (dtSeconds);\n",
     "    if (audioProcessor.analyzerMode.load() != 0) analyzer->process (dtSeconds);\n", "bench", ["E-an-stale"]),
    ("emit-every-tick", EDITOR, "        if (analyzerSent[(size_t) tap]\n            && std::memcmp",
     "        if (false && analyzerSent[(size_t) tap]\n            && std::memcmp", "bench", ["E-idle"]),
    ("null-to-zero", HTML, "(v === null || v === undefined) ? NaN : v / 10", "(v === null || v === undefined) ? 0 : v / 10",
     "bench", ["E-an-nyq"]),
    ("span-centre-reverted", HTML, "xs[i] = PLOT_MARGIN.l + (i + 0.5) / n * plotW;", "xs[i] = PLOT_MARGIN.l + i / (n - 1) * plotW;",
     "bench", ["E-an-peak"]),
    ("peakhold-depth-1", PEAK, "void setDepth (int h) noexcept      { depth = std::clamp (h, 1, capacity); }",
     "void setDepth (int h) noexcept      { (void) h; depth = 1; }", "bench+harness", ["E-meter", "M1"]),
    ("listen-at-pointerdown", HTML, "        ui.drag = { b, ox, oy, moved: false, gain: false, sx: x, sy: y };\n",
     "        ui.drag = { b, ox, oy, moved: false, gain: false, sx: x, sy: y };\n        ui.dragListen = b; syncListen();\n",
     "bench", ["E-listen33"]),
    ("no-return-to-latch", HTML, "        ui.dragListen = 0;                      // back to the latch, or off\n",
     "        ui.dragListen = 0; ui.latch = 0;\n", "bench", ["E-listen33"]),
    ("disable-hook-only-in-delete", HTML,
     [("            if (k === 'enabled') st.valueChangedEvent.addListener(() => {\n                if (!st.getValue()) setTimeout(() => { if (!st.getValue()) endListenOn(b); }, LISTEN_DISABLE_SETTLE_MS);\n            });\n", ""),
      ("    toggleState[bid(b, 'enabled')].setValue(false);   // listen ends through the enabled hook\n",
       "    toggleState[bid(b, 'enabled')].setValue(false);\n    endListenOn(b);\n")], None, "bench", ["E-listen33"]),
    ("timer-30hz", EDITOR, "                startTimerHz (60);", "                startTimerHz (30);", "bench", ["E-rate"]),
    ("meter-decay-per-frame", HTML, "function meterStep(state, target, dt) {\n",
     "function meterStep(state, target, dt) {\n    dt = dt > 0 ? 1 / 60 : 0;\n", "node", ["meter-ballistics"]),
    ("off-rule-removed", ANALYZER, "        if ((processor.analyzerMode.load (std::memory_order_acquire) & tapBit) == 0)\n            t.valid = 0;\n",
     "", "bench+harness", ["E-an-stale", "AO2"]),
]

# Which listen sub-checks each listen mutant must fail (PLAN-3.3 T11 table)
LISTEN_CHECKS = {
    "listen-at-pointerdown": ["(b)"],
    "no-return-to-latch": ["(c) release"],
    "disable-hook-only-in-delete": ["(d)", "(e) preset"],
}


def sh(cmd, log, cwd=None):
    with open(log, "w") as f:
        return subprocess.call(cmd, shell=True, stdout=f, stderr=subprocess.STDOUT, cwd=cwd)


def wait_unlocked():
    while subprocess.run([LOCK], capture_output=True, text=True).stdout.strip() != "0":
        time.sleep(20)


def apply(mid, rel, old, new, pristine):
    for p, text in pristine.items():
        open(os.path.join(SRC, p), "w").write(text)
    if rel is None:
        return None
    path = os.path.join(SRC, rel)
    t = open(path).read()
    edits = old if isinstance(old, list) else [(old, new)]
    bad = [a for a, _ in edits if t.count(a) != 1]
    if bad:
        return f"anchor not unique: {bad[0][:60]}"
    for a, b in edits:
        t = t.replace(a, b)
    open(path, "w").write(t)
    return None


def score_bench(mid):
    rc = sh(f"ninja -C '{BUILD}'", os.path.join(SCR, f"build-{mid}.log"))
    if rc != 0:
        return {"error": "bench build failed"}
    exe = os.path.join(BUILD, "EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench")
    out = os.path.join(SCR, f"{mid}.json")
    wait_unlocked()
    sh(f"caffeinate -d -u -i '{exe}' --cycles 1 --reps 1 --idle 3 --nosize --out '{out}' -NSAppSleepDisabled YES",
       os.path.join(SCR, f"{mid}.log"))
    locked = subprocess.run([LOCK], capture_output=True, text=True).stdout.strip() != "0"
    v = subprocess.run([sys.executable, os.path.join(BENCH, "verdict33.py"), out, "--quiet"], capture_output=True, text=True).stdout.strip()
    failing = v[5:].split(",") if v.startswith("FAIL ") else []
    rec = {"benchVerdict": v, "benchFailing": failing, "lockedDuringRun": locked}
    try:
        d = json.load(open(out))
        rec["failedChecks"] = d.get("failedChecks", [])
    except Exception as e:
        rec["failedChecks"] = [f"no json: {e}"]
    return rec


def score_harness(mid):
    out = os.path.join(SCR, f"h-{mid}")
    rc = sh(f"SRC_DIR='{SRC}' OUT_DIR='{out}' '{HERE}/build-harness.sh'", os.path.join(SCR, f"hbuild-{mid}.log"))
    if rc != 0:
        return {"error": "harness build failed"}
    log = os.path.join(SCR, f"h-{mid}.log")
    sh(f"'{out}/harness' --rows33", log, cwd=PEQ)
    text = open(log).read()
    failing = sorted(set(m.group(1) for m in re.finditer(r"\[FAIL\] (\S+)", text)))
    tally = re.findall(r"(\d+) PASS, (\d+) FAIL", text)
    return {"harnessFailing": failing, "harnessTally": tally[-1] if tally else None}


def score_node(mid):
    r = subprocess.run(["node", f"{PEQ}/tests/meter-ballistics/ballistics.mjs", os.path.join(SRC, HTML)], capture_output=True, text=True)
    return {"nodeFailing": ["meter-ballistics"] if r.returncode != 0 else [], "nodeTail": r.stdout.strip().splitlines()[-3:]}


def main():
    os.makedirs(SCR, exist_ok=True)
    if os.path.exists(SRC):
        shutil.rmtree(SRC)
    shutil.copytree(f"{PEQ}/Source", SRC)
    pristine = {p: open(os.path.join(SRC, p)).read() for p in FILES}

    if sh(f"cmake -S '{BENCH}' -B '{BUILD}' -G Ninja -DCMAKE_BUILD_TYPE=Release -DPEQ_SRC='{SRC}'", os.path.join(SCR, "cfg.log")) != 0:
        print("configure failed"); sys.exit(2)

    results = []
    for mid, rel, old, new, scorer, targets in MUTANTS:
        if ONLY and mid not in ONLY:
            continue
        err = apply(mid, rel, old, new, pristine)
        rec = {"mutant": mid, "scorer": scorer, "targets": targets}
        if err:
            rec["error"] = err; results.append(rec); print(json.dumps(rec), flush=True); continue
        failing = []
        if "bench" in scorer:
            r = score_bench(mid); rec.update(r); failing += r.get("benchFailing", [])
        if "harness" in scorer:
            r = score_harness(mid); rec.update(r); failing += r.get("harnessFailing", [])
        if scorer == "node":
            r = score_node(mid); rec.update(r); failing += r.get("nodeFailing", [])
        if mid.startswith("control"):
            rec["caught"] = None
            rec["controlPass"] = not failing and "error" not in rec
        else:
            caught = all(t in failing for t in targets) and "error" not in rec
            if mid in LISTEN_CHECKS:
                fc = rec.get("failedChecks", [])
                caught = caught and all(any(f.startswith("E-listen33 / " + c) for f in fc) for c in LISTEN_CHECKS[mid])
            rec["caught"] = caught
        results.append(rec)
        print(json.dumps(rec), flush=True)

    for p, text in pristine.items():
        open(os.path.join(SRC, p), "w").write(text)
    out = os.path.join(HERE, "mutants33-results.json")
    merged = {r["mutant"]: r for r in (json.load(open(out)) if os.path.exists(out) else [])}
    for r in results:
        merged[r["mutant"]] = r
    order = [m[0] for m in MUTANTS]
    results = sorted(merged.values(), key=lambda r: order.index(r["mutant"]) if r["mutant"] in order else 99)
    json.dump(results, open(out, "w"), indent=1)
    real = [r for r in results if not r["mutant"].startswith("control")]
    print("caught", sum(1 for r in real if r.get("caught")), "of", len(real),
          "| controls", [(r["mutant"], r.get("controlPass")) for r in results if r["mutant"].startswith("control")])


if __name__ == "__main__":
    main()
