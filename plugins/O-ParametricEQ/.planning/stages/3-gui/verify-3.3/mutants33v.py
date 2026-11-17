#!/usr/bin/env python3
"""VERIFY 3.3 mutation run (runner from execute-3.3/mutants/run-mutants33.py, new mutants).

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
EX = os.path.join(HERE, "..", "execute-3.3")
BENCH = os.path.join(EX, "editor-bench")
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

# (id, file, old, new, scorer, targets). VERIFY 3.3 mutants: none of them is
# one of execute's 13. targets [] = a probe (is there any row at all?).
MUTANTS = [
    ("control-bench", None, None, None, "bench", []),
    ("control-harness", None, None, None, "harness", []),
    ("control-node", None, None, None, "node", []),
    # editor
    ("W1-emit-ignores-mode", EDITOR, "        if ((mode & (1 << tap)) == 0)\n            continue;\n", "", "bench", ["E-an-modes"]),
    ("W2-tap-events-swapped", EDITOR, 'tap == 0 ? "analyzerPre" : "analyzerPost"', 'tap == 0 ? "analyzerPost" : "analyzerPre"', "bench", ["E-an-peak"]),
    ("W4-level-every-tick", EDITOR, "    if (count == lastLevelCount && level[0] == lastLevelSent[0]", "    if (false && count == lastLevelCount && level[0] == lastLevelSent[0]", "bench", ["E-idle33"]),
    ("W5-mono-sends-two", EDITOR, "    const int count = stereo ? 2 : 1;", "    const int count = 2; (void) stereo;", "bench", ["E-meter"]),
    ("W21-reload-keeps-sent", EDITOR, "            analyzerSent = { false, false };\n            lastLevelCount = 0;\n", "", "bench", []),
    ("W22-no-mode-add-reemit", EDITOR, "            analyzerSent[(size_t) tap] = false;\n", "            (void) 0;\n", "bench", []),
    # PeakHold.h (encode, meter feed)
    ("W3-nyquist-at-fs", PEAK, "const double nyquist = sampleRateHz * 0.5;", "const double nyquist = sampleRateHz;", "bench", ["E-an-nyq"]),
    ("W6-power-db", PEAK, "std::round (200.0 * std::log10", "std::round (100.0 * std::log10", "bench+harness", ["E-meter", "M1"]),
    ("W7-depth-no-plus-one", PEAK, "const int h = (int) std::ceil (ticksPerBlock) + 1;", "const int h = (int) std::ceil (ticksPerBlock);", "bench+harness", ["E-meter", "M1"]),
    # SpectrumAnalyzer Off rule
    ("W8-off-rule-bits-swapped", ANALYZER, "const int tapBit = (&t == &taps[0]) ? 1 : 2;", "const int tapBit = (&t == &taps[0]) ? 2 : 1;", "bench+harness", ["E-an-peak"]),
    # Listen.h
    ("W17-goidle-keeps-flag", LISTEN, "        nonFiniteVoice = false;\n        moveKind = ListenMove::none;", "        moveKind = ListenMove::none;", "harness", []),
    ("W18-voice-check-right-only", LISTEN, "(! std::isfinite (outL[count - 1])", "(false", "harness", ["N1e"]),
    # page: listen
    ("W9-settle-removed", HTML, "if (!st.getValue()) setTimeout(() => { if (!st.getValue()) endListenOn(b); }, LISTEN_DISABLE_SETTLE_MS);", "if (!st.getValue()) endListenOn(b);", "bench", ["E-listen33"]),
    ("W10-drag-listen-sticks", HTML, "        ui.dragListen = 0;                      // back to the latch, or off\n        syncListen();\n", "", "bench", ["E-listen33"]),
    ("W11-bypass-ends-listen", HTML, "            if (k === 'enabled') st.valueChangedEvent.addListener(() => {\n",
     "            if (k === 'bypass') st.valueChangedEvent.addListener(() => { if (st.getValue()) endListenOn(b); });\n            if (k === 'enabled') st.valueChangedEvent.addListener(() => {\n", "bench", ["E-listen33"]),
    ("W15-strip-button-follows-listen", HTML, "$('btn-listen').setAttribute('aria-pressed', ui.latch === s", "$('btn-listen').setAttribute('aria-pressed', ui.listen === s", "bench", []),
    # page: meter, fills, frame
    ("W12-meter-mono-right-silent", HTML, "r = a.length > 1 ? Number(a[1]) / 10 : l;", "r = a.length > 1 ? Number(a[1]) / 10 : METER_LO;", "bench", ["E-meter"]),
    ("W13-meter-never-rests", HTML, "        meter.raf = meterActive() ? requestAnimationFrame(tick) : 0;", "        meter.raf = requestAnimationFrame(tick);", "bench", ["E-meter-rest"]),
    ("W14-fills-ignore-bypass", HTML, "            if (!byp) {\n                const k = sel ? 1.5", "            if (true) {\n                const k = sel ? 1.5", "bench", ["E-fills"]),
    ("W16-double-draw-restored", HTML, "    if (ts === lastDrawFrame) return;\n", "", "bench", ["E-fps"]),
    # page: ballistics (node)
    ("W19-no-peak-hold", HTML, "const floor = -60, rate = 20, holdS = 1;", "const floor = -60, rate = 20, holdS = 0;", "node", ["meter-ballistics"]),
    ("W20-release-10dbps", HTML, "const floor = -60, rate = 20, holdS = 1;", "const floor = -60, rate = 10, holdS = 1;", "node", ["meter-ballistics"]),
]

LISTEN_CHECKS = {
    "W9-settle-removed": ["(f)"],
    "W10-drag-listen-sticks": ["(a) release"],
    "W11-bypass-ends-listen": ["(g)"],
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
    rc = sh(f"SRC_DIR='{SRC}' OUT_DIR='{out}' '{EX}/mutants/build-harness.sh'", os.path.join(SCR, f"hbuild-{mid}.log"))
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
            caught = (all(t in failing for t in targets) if targets else bool(failing)) and "error" not in rec
            if mid in LISTEN_CHECKS:
                fc = rec.get("failedChecks", [])
                caught = caught and all(any(f.startswith("E-listen33 / " + c) for f in fc) for c in LISTEN_CHECKS[mid])
            rec["caught"] = caught
        results.append(rec)
        print(json.dumps(rec), flush=True)

    for p, text in pristine.items():
        open(os.path.join(SRC, p), "w").write(text)
    out = os.path.join(HERE, "mutants33v-results.json")
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
