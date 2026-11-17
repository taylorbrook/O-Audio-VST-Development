#!/usr/bin/env python3
"""PLAN-4.1 task 27, page half: mutants M1 - M3, scored by the editor bench.

Shape of stages/3-gui/execute-3.3/mutants/run-mutants33.py (bench scorer only).
Each mutant is ONE textual edit (asserted to apply exactly once) to a scratch
copy of Source/ (never the repo tree). The 4.1 editor bench is rebuilt against
the copy (-DPEQ_SRC) and run once (1 cycle, 1 rep, 3 s idle, no size rows;
the bench re-execs under its own CFFIXED_USER_HOME and aborts before
constructing a processor if the home is not the fake one). verdict41.py names
the failing 4.1 rows. A mutant is CAUGHT when its target row fails. The
unmodified copy runs first through the same runner as the control and must
pass every 4.1 row.

usage: run-mutants41-page.py <scratch dir> [ids...]
"""
import json, os, re, shutil, subprocess, sys, time

REPO = "/Users/taylorbrook/Dev/VST-development"
PEQ = f"{REPO}/plugins/O-ParametricEQ"
HERE = os.path.dirname(os.path.abspath(__file__))
BENCH = os.path.abspath(os.path.join(HERE, "..", "editor-bench"))
SCR = os.path.abspath(sys.argv[1])
ONLY = sys.argv[2:]
SRC = os.path.join(SCR, "Source")
BUILD = os.path.join(SCR, "build-mut")
LOCK = os.path.join(BENCH, "lockstate")
HTML = "ui/public/index.html"
OUT = os.path.join(HERE, "mutants41-page-results.json")

# (id, description, row, old, new)
MUTANTS = [
    ("control", "unmodified Source copy, same runner", None, None, None),
    ("M1", "deleteBand without the endListenOn(b) call (listen ends only through the 80 ms enabled hook)",
     "E-del-listen",
     "    endListenOn(b);\n    let next = 0;",
     "    let next = 0;"),
    ("M2", ".tip-point removed at fade start (old code: hideTooltip calls clearPointTip at once)",
     "E-tipfade",
     """    if (tooltipEl.classList.contains('tip-point')) {
        pointTipAnchor = null;          // the re-place observer stops; the box keeps its text and size
        schedulePointTipRemoval();
    } else {
        clearPointTip();
    }
""",
     "    clearPointTip();\n"),
    ("M3", "strip #btn-listen aria-pressed bound to ui.listen instead of ui.latch",
     "E-listen33-aria",
     "$('btn-listen').setAttribute('aria-pressed', ui.latch === s ? 'true' : 'false');",
     "$('btn-listen').setAttribute('aria-pressed', ui.listen === s ? 'true' : 'false');"),
]


def sh(cmd, log):
    with open(log, "w") as f:
        return subprocess.call(cmd, shell=True, stdout=f, stderr=subprocess.STDOUT)


def lockstate():
    return subprocess.run([LOCK], capture_output=True, text=True).stdout.strip()


def quiet():
    pats = ["pluginval", "render-harness", "designer-harness", "probe33", "g9b.mjs"]
    while True:
        busy = [p for p in pats if subprocess.run(["pgrep", "-f", p], capture_output=True).returncode == 0]
        busy += [p for p in ("ninja", "auval") if subprocess.run(["pgrep", "-x", p], capture_output=True).returncode == 0]
        if not busy:
            return
        print("waiting for", busy, flush=True)
        time.sleep(15)


def main():
    os.makedirs(SCR, exist_ok=True)
    if os.path.exists(SRC):
        shutil.rmtree(SRC)
    shutil.copytree(f"{PEQ}/Source", SRC)
    pristine = open(os.path.join(SRC, HTML)).read()
    if sh(f"cmake -S '{BENCH}' -B '{BUILD}' -G Ninja -DCMAKE_BUILD_TYPE=Release -DPEQ_SRC='{SRC}'",
          os.path.join(SCR, "cfg.log")) != 0:
        print("configure failed"); sys.exit(2)

    results = []
    for mid, desc, row, old, new in MUTANTS:
        if ONLY and mid not in ONLY:
            continue
        t = pristine
        rec = {"id": mid, "description": desc, "row": row}
        if old is not None:
            if t.count(old) != 1:
                rec.update(caught=False, evidence=f"anchor count {t.count(old)} (not applied)")
                results.append(rec); print(json.dumps(rec), flush=True); continue
            t = t.replace(old, new)
        open(os.path.join(SRC, HTML), "w").write(t)

        # Build: the shared quiet() wait does not apply to our own ninja; the timing run does.
        if sh(f"ninja -C '{BUILD}'", os.path.join(SCR, f"build-{mid}.log")) != 0:
            rec.update(caught=False, evidence="bench build failed")
            results.append(rec); print(json.dumps(rec), flush=True); continue
        exe = os.path.join(BUILD, "EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench")
        out = os.path.join(SCR, f"{mid}.json")
        quiet()
        while lockstate() != "0":
            time.sleep(20)
        sh(f"caffeinate -d -u -i '{exe}' --cycles 1 --reps 1 --idle 3 --nosize --out '{out}' -NSAppSleepDisabled YES",
           os.path.join(SCR, f"{mid}.log"))
        locked = lockstate() != "0"
        v41 = subprocess.run([sys.executable, os.path.join(BENCH, "verdict41.py"), out], capture_output=True, text=True).stdout
        v33 = subprocess.run([sys.executable, os.path.join(BENCH, "verdict33.py"), out, "--quiet"], capture_output=True, text=True).stdout.strip()
        last = v41.strip().splitlines()[-1] if v41.strip() else "no output"
        failing = last[5:].split(",") if last.startswith("FAIL ") else []
        rowLines = {m.group(2): m.group(0).strip() for m in re.finditer(r"\[(PASS|FAIL)\] (\S+).*", v41)}
        rec.update(verdict41=last, failing41=failing, verdict33=v33, lockedDuringRun=locked)
        try:
            d = json.load(open(out))
            rec["failedChecks"] = d.get("failedChecks", [])
        except Exception as e:
            rec["failedChecks"] = [f"no json: {e}"]
        if row is None:
            rec["caught"] = None
            rec["controlPass"] = last == "ALL-PASS" and not locked
            rec["evidence"] = " | ".join(rowLines.get(r, "") for r in
                                         ("E-del-listen", "E-listen33-aria", "E-tipfade", "E-tip-lang", "E-tip-v13", "E-preset-listen"))
        else:
            rec["caught"] = row in failing and not locked
            rec["evidence"] = rowLines.get(row, "row missing from verdict41 output") + \
                (" | failing checks: " + "; ".join(f for f in rec["failedChecks"] if f.startswith(row + " / ")))
        results.append(rec)
        print(json.dumps(rec), flush=True)

    open(os.path.join(SRC, HTML), "w").write(pristine)
    merged = {r["id"]: r for r in (json.load(open(OUT)) if os.path.exists(OUT) else [])}
    for r in results:
        merged[r["id"]] = r
    order = [m[0] for m in MUTANTS]
    allr = sorted(merged.values(), key=lambda r: order.index(r["id"]) if r["id"] in order else 99)
    json.dump(allr, open(OUT, "w"), indent=1, ensure_ascii=False)
    real = [r for r in allr if r["id"] != "control"]
    print("caught", sum(1 for r in real if r.get("caught")), "of", len(real),
          "| control", next((r.get("controlPass") for r in allr if r["id"] == "control"), None))


if __name__ == "__main__":
    main()
