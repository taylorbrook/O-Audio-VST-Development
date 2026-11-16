#!/usr/bin/env python3
"""PLAN-3.2 T10 mutant control.

Each mutant is ONE textual edit to a scratch copy of Source/ (never the repo
tree). The editor bench is rebuilt against the copy (-DPEQ_SRC) and run once
(1 cycle, 1 rep, 3 s idle, no size rows); verdict32.py names the rows that
fail. A mutant is CAUGHT when its target row fails. The unmodified copy runs
first as the control and must pass every row.

Mutant P3 is also scored by the G9 (b) gate: g9b.mjs on the mutated page
against a --dump-g9 dump (a FAIL there is a second catch).

usage: run-mutants.py <scratch dir> [mutant ids...]
"""
import json, os, shutil, subprocess, sys, time

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

MUTANTS = [
    ("control", None, None, None, None),
    ("E15", EDITOR, "obj->setProperty (id, param->convertFrom0to1 (param->getDefaultValue()));",
     "obj->setProperty (id, param->getDefaultValue());", "E-dblclick"),
    ("onUp-skips-freq-end", HTML, "            sliderState[bid(d.b, 'freq')].sliderDragEnded();\n", "", "E-drag"),
    ("P5-reverted", HTML, [("            if (d.gain) gSt.sliderDragStarted();                 // and gain, when it moves",
                            "            gSt.sliderDragStarted();"),
                           ("            if (d.gain) sliderState[bid(d.b, 'gain')].sliderDragEnded();   // only the pairs opened",
                            "            sliderState[bid(d.b, 'gain')].sliderDragEnded();")], None, "E-drag"),
    ("P3-reverted", HTML, "if (pk && pk.active && R) run.push([R, Number(pk.placement) || 0]);",
     "if (pk && pk.active && R) run.push([R, bandChoice(b, 'placement')]);", "E-mono"),
    ("fullPush-never-set", EDITOR, "            fullPushPending = true;\n            complete (true);",
     "            complete (true);", "E-eq-reload"),
    ("emit-on-empty", EDITOR, "    if (full || ! changed.isEmpty())", "    if (true)", "E-eq-idle"),
    ("refused-create-silent", HTML, "                showBandsFullTip(e.clientX, e.clientY);\n", "", "E-full"),
]


def sh(cmd, log, cwd=None):
    with open(log, "w") as f:
        return subprocess.call(cmd, shell=True, stdout=f, stderr=subprocess.STDOUT, cwd=cwd)


def wait_unlocked():
    while subprocess.run([LOCK], capture_output=True, text=True).stdout.strip() != "0":
        time.sleep(20)


def main():
    os.makedirs(SCR, exist_ok=True)
    if os.path.exists(SRC):
        shutil.rmtree(SRC)
    shutil.copytree(f"{PEQ}/Source", SRC)
    pristine = {p: open(os.path.join(SRC, p)).read() for p in (HTML, EDITOR)}

    # One --dump-g9 for the page mutants (built from the repo harness)
    dump = os.path.join(SCR, "g9dump")
    if not os.path.isdir(dump) or len(os.listdir(dump)) < 260:
        h = f"{REPO}/build/plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness"
        sh(f"'{h}' --dump-g9 '{dump}'", os.path.join(SCR, "g9dump.log"), cwd=PEQ)

    cfg = sh(f"cmake -S '{BENCH}' -B '{BUILD}' -G Ninja -DCMAKE_BUILD_TYPE=Release -DPEQ_SRC='{SRC}'",
             os.path.join(SCR, "cfg.log"))
    if cfg != 0:
        print("configure failed"); sys.exit(2)

    results = []
    for mid, rel, old, new, target in MUTANTS:
        if ONLY and mid not in ONLY:
            continue
        for p, text in pristine.items():
            open(os.path.join(SRC, p), "w").write(text)
        if rel is not None:
            path = os.path.join(SRC, rel)
            t = open(path).read()
            edits = old if isinstance(old, list) else [(old, new)]
            bad = [a for a, _ in edits if t.count(a) != 1]
            if bad:
                results.append({"mutant": mid, "error": f"anchor not unique: {bad[0][:60]}"}); continue
            for a, b in edits:
                t = t.replace(a, b)
            open(path, "w").write(t)
        rc = sh(f"ninja -C '{BUILD}'", os.path.join(SCR, f"build-{mid}.log"))
        if rc != 0:
            results.append({"mutant": mid, "error": "build failed"}); continue
        exe = os.path.join(BUILD, "EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench")
        out = os.path.join(SCR, f"{mid}.json")
        wait_unlocked()
        sh(f"caffeinate -d -u -i '{exe}' --cycles 1 --reps 1 --idle 3 --nosize --out '{out}' -NSAppSleepDisabled YES",
           os.path.join(SCR, f"{mid}.log"))
        locked = subprocess.run([LOCK], capture_output=True, text=True).stdout.strip() != "0"
        v = subprocess.run([sys.executable, os.path.join(BENCH, "verdict32.py"), out, "--quiet"], capture_output=True, text=True).stdout.strip()
        failing = v[5:].split(",") if v.startswith("FAIL ") else []
        rec = {"mutant": mid, "target": target, "verdict": v, "failing": failing, "lockedDuringRun": locked}
        if mid == "P3-reverted":
            g = subprocess.run(["node", f"{PEQ}/tests/g9b/g9b.mjs", os.path.join(SRC, HTML), dump], capture_output=True, text=True)
            rec["g9b"] = "FAIL" if g.returncode != 0 else "PASS"
            rec["g9bTail"] = g.stdout.strip().splitlines()[-4:]
        rec["caught"] = (not failing) if target is None else (target in failing)
        results.append(rec)
        print(json.dumps(rec), flush=True)

    for p, text in pristine.items():
        open(os.path.join(SRC, p), "w").write(text)
    json.dump(results, open(os.path.join(HERE, "mutants-results.json"), "w"), indent=1)
    print("caught", sum(1 for r in results if r.get("caught") and r["mutant"] != "control"),
          "of", sum(1 for r in results if r["mutant"] != "control"),
          "| control", next((r.get("verdict") for r in results if r["mutant"] == "control"), "not run"))


if __name__ == "__main__":
    main()
