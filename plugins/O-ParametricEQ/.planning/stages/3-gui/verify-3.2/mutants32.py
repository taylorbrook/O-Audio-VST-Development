#!/usr/bin/env python3
"""Verify 3.2: an independent mutant run through the execute-3.2 editor bench.

None of these mutants is one of execute's seven. Each is ONE textual edit to a
scratch copy of Source/ (never the repo tree); the bench is rebuilt against the
copy (-DPEQ_SRC) and run once (1 cycle, 1 rep, 3 s idle, no size rows).
Verdicts come from verdict32.py. A mutant with a named target row is CAUGHT
when that row fails. A mutant whose target is "probe" asks a question (is
there any row for this?): its failing rows are recorded, nothing is scored.
The unmodified copy runs first and last as the control (ALL-PASS both).

usage: mutants32.py <scratch dir> [mutant ids...]
"""
import json, os, shutil, subprocess, sys, time

REPO = "/Users/taylorbrook/Dev/VST-development"
PEQ = f"{REPO}/plugins/O-ParametricEQ"
HERE = os.path.dirname(os.path.abspath(__file__))
BENCH = os.path.join(PEQ, ".planning/stages/3-gui/execute-3.2/editor-bench")
SCR = os.path.abspath(sys.argv[1])
ONLY = sys.argv[2:]
SRC = os.path.join(SCR, "Source")
BUILD = os.path.join(SCR, "build-mut")
LOCK = os.path.join(BENCH, "lockstate")
HTML = "ui/public/index.html"
ED = "PluginEditor.cpp"

M = [
    ("control", None, None, None, None),
    # ── editor ──
    ("V1-timer-30Hz", ED, "                startTimerHz (60);", "                startTimerHz (30);", "E-stopped"),
    ("V2-band-0-based", ED, 'obj->setProperty ("band", b + 1);', 'obj->setProperty ("band", b);', "E-eq-init"),
    ("V3-flag-never-cleared", ED, "    fullPushPending = false;\n", "", "E-eq-idle"),
    ("V4-changed-ignored", ED, "    if (full || ! changed.isEmpty())", "    if (full)", "E-stopped"),
    ("V5-header-stereo-true", ED, 'payload->setProperty ("stereo",     header["stereo"]);',
     'payload->setProperty ("stereo",     true);', "E-mono"),
    # ── page: curve ──
    ("V6-active-filter-dropped", HTML, "if (pk && pk.active && R) run.push", "if (pk && R) run.push", "probe"),
    ("V7-global-bypass-ignored", HTML, "composeSum(globalBypass ? [] : running, responses", "composeSum(running, responses", "probe"),
    ("V8-sumView-always-LL", HTML, "return pk ? [0, 0, 1, 2, 3][Number(pk.placement)] || 0 : 0;", "return 0;", "probe"),
    # ── page: drag gestures (P5) ──
    ("V9-dgain-ignores-scale", HTML, "d.gain = !!TYPE_HAS_GAIN[bandChoice(d.b, 'type')] && gainScale() > 0.01;",
     "d.gain = !!TYPE_HAS_GAIN[bandChoice(d.b, 'type')];", "E-drag"),
    ("V10-dgain-ignores-type", HTML, "d.gain = !!TYPE_HAS_GAIN[bandChoice(d.b, 'type')] && gainScale() > 0.01;",
     "d.gain = gainScale() > 0.01;", "E-drag"),
    ("V11-onUp-skips-gain-end", HTML,
     "            if (d.gain) sliderState[bid(d.b, 'gain')].sliderDragEnded();   // only the pairs opened\n", "", "E-drag"),
    # ── page: U5 tip ──
    ("V12-tip-gated-by-help", HTML, "function showBandsFullTip(clientX, clientY) {\n    if (!tooltipEl) return;",
     "function showBandsFullTip(clientX, clientY) {\n    if (!tooltipEl || !tipsEnabled) return;", "E-full"),
    ("V13-hover-keeps-point-binding", HTML, "    clearPointTip();\n    tooltipEl.textContent = '';",
     "    tooltipEl.textContent = '';", "probe"),
    # ── page: interactions already in 3.1, gated in 3.2 ──
    ("V14-parseEntry-no-exponent", HTML, r"const m = s.match(/^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)\s*(k)?/);",
     r"const m = s.match(/^([+-]?\d*\.?\d+)\s*(k)?/);", "E-ends"),
    ("V15-create-keeps-placement", HTML, "        comboState[bid(b, 'placement')].setChoiceIndex(0);   // Stereo\n", "", "E-create"),
    ("V16-create-keeps-q", HTML, "        if (qDef !== null) writeScaled(sliderState[bid(b, 'q')], qDef);\n", "", "E-create"),
    ("V17-dblclick-create-guard-gone", HTML, "if (!b || performance.now() - ui.createdAt < CREATE_DBLCLICK_MS) return;",
     "if (!b) return;", "E-dblclick"),
    ("V18-wheel-gesture-per-event", HTML,
     "        wheelTouch(st);\n        setScaled(st, clamp(st.getScaledValue() * Math.exp(-dy * (e.shiftKey ? 0.0006 : 0.0025)), p.start, p.end));",
     "        wheelEnd(st); wheelTouch(st);\n        setScaled(st, clamp(st.getScaledValue() * Math.exp(-dy * (e.shiftKey ? 0.0006 : 0.0025)), p.start, p.end));",
     "E-wheel"),
    ("V19-delete-selects-none", HTML, "    selectBand(next);\n    requestRender();", "    selectBand(0);\n    requestRender();", "E-delete"),
    ("V20-create-not-bypass-cleared", HTML, "        toggleState[bid(b, 'bypass')].setValue(false);\n", "", "E-create"),
    ("control-end", None, None, None, None),
]


def sh(cmd, log, cwd=None):
    with open(log, "w") as f:
        return subprocess.call(cmd, shell=True, stdout=f, stderr=subprocess.STDOUT, cwd=cwd)


def lockstate():
    return subprocess.run([LOCK], capture_output=True, text=True).stdout.strip()


def main():
    os.makedirs(SCR, exist_ok=True)
    if os.path.exists(SRC):
        shutil.rmtree(SRC)
    shutil.copytree(f"{PEQ}/Source", SRC)
    pristine = {p: open(os.path.join(SRC, p)).read() for p in (HTML, ED)}

    if sh(f"cmake -S '{BENCH}' -B '{BUILD}' -G Ninja -DCMAKE_BUILD_TYPE=Release -DPEQ_SRC='{SRC}'",
          os.path.join(SCR, "cfg.log")) != 0:
        print("configure failed"); sys.exit(2)

    results = []
    for mid, rel, old, new, target in M:
        if ONLY and mid not in ONLY:
            continue
        for p, text in pristine.items():
            open(os.path.join(SRC, p), "w").write(text)
        if rel is not None:
            path = os.path.join(SRC, rel)
            t = open(path).read()
            if t.count(old) != 1:
                rec = {"mutant": mid, "error": f"anchor count {t.count(old)}"}
                results.append(rec); print(json.dumps(rec), flush=True); continue
            open(path, "w").write(t.replace(old, new))
        if sh(f"ninja -C '{BUILD}'", os.path.join(SCR, f"build-{mid}.log")) != 0:
            rec = {"mutant": mid, "error": "build failed"}
            results.append(rec); print(json.dumps(rec), flush=True); continue
        exe = os.path.join(BUILD, "EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench")
        out = os.path.join(SCR, f"{mid}.json")
        if os.path.exists(out):
            os.remove(out)
        while lockstate() != "0":
            time.sleep(20)
        rc = sh(f"caffeinate -d -u -i '{exe}' --cycles 1 --reps 1 --idle 3 --nosize --out '{out}' -NSAppSleepDisabled YES",
                os.path.join(SCR, f"{mid}.log"))
        locked = lockstate() != "0"
        if os.path.exists(out):
            v = subprocess.run([sys.executable, os.path.join(BENCH, "verdict32.py"), out, "--quiet"],
                               capture_output=True, text=True).stdout.strip()
        else:
            v = f"FAIL no-json rc={rc}"
        failing = v[5:].split(",") if v.startswith("FAIL ") else []
        rec = {"mutant": mid, "target": target, "verdict": v, "failing": failing, "rc": rc, "lockedDuringRun": locked}
        if target is None:
            rec["caught"] = None
            rec["controlPass"] = not failing
        elif target == "probe":
            rec["caught"] = None
        else:
            # verdict32 names some rows with a suffix ("E-eq-init (every open)")
            rec["caught"] = any(f == target or f.startswith(target + " (") for f in failing)
        results.append(rec)
        print(json.dumps(rec), flush=True)

    for p, text in pristine.items():
        open(os.path.join(SRC, p), "w").write(text)
    json.dump(results, open(os.path.join(HERE, "mutants32-results.json"), "w"), indent=1)
    scored = [r for r in results if r.get("target") not in (None, "probe")]
    print("caught", sum(1 for r in scored if r.get("caught")), "of", len(scored))


if __name__ == "__main__":
    main()
