"""PLAN-3.1 T11: summarise results/rel-*.json + results/dbg.json -> results/summary.json (+ stdout)."""
import json, glob, statistics, os, re

B = os.path.dirname(os.path.abspath(__file__))
R = os.path.join(B, "results")


def mw(xs):
    xs = [x for x in xs if x is not None and x >= 0]
    if not xs:
        return None
    return {"median": round(statistics.median(xs), 1), "worst": round(max(xs), 1), "n": len(xs)}


def load(pattern):
    return [(f, json.load(open(f))) for f in sorted(glob.glob(os.path.join(R, pattern)))]


rel = load("rel-*.json")
dbg = load("dbg.json")
S = {"processes": {"release": len(rel), "debug": len(dbg)}}
D = [d for _, d in rel]

# E-open
cold = [d["cycles"][0] for d in D]
warm = [c for d in D for c in d["cycles"][1:]]
S["E-open"] = {
    lbl: {k: mw([c.get(k) for c in cs]) for k in
          ["ctorMs", "scriptStartMs", "dclMs", "allPropsMs", "firstPaintMs", "secondFrameMs", "pageReadyMs", "closeMs"]}
    for lbl, cs in [("cold", cold), ("warm", warm)]}
S["E-open"]["coldPerProcess"] = [round(c["secondFrameMs"], 1) for c in cold]
S["E-open"]["gate500"] = all(0 <= c["secondFrameMs"] <= 500 for c in cold)

# E-props
c0 = D[0]["cycles"][0]
S["E-props"] = {
    "check": c0["allProps"]["check"],
    "bootEvalsPosts": [c0["allProps"]["evals"], c0["allProps"]["posts"]],
    "nProps": c0["allProps"]["n"],
    "bootConsoleMax": {k: max(c["console"].get(k, 0) for d in D for c in d["cycles"]) for k in ["nWarn", "nErr", "unknownWarn"]},
    "bootNatives": c0["console"]["natives"],
    "consoleAfterTests": [{k: d["rows"]["consoleAfterTests"].get(k) for k in ["nWarn", "nErr", "unknownWarn", "warns"]} for d in D],
}

# E-idle
S["E-idle"] = [{"ms": round(d["rows"]["E-idle"]["ms"]), "evals": d["rows"]["E-idle"]["evals"], "posts": d["rows"]["E-idle"]["posts"]} for d in D]

# E-preset
S["E-preset"] = {}
for m in ["msgLoop", "setState", "offThread", "preset", "single"]:
    P = [p for d in D for p in d["presets"] if p["method"] == m]
    S["E-preset"][m] = {
        "runs": len(P),
        "expected": sorted(set(p["expected"] for p in P)),
        "applyMs": mw([p["applyMs"] for p in P]),
        "settledRecvMs": mw([p.get("settledRecvMs") for p in P]),
        "settledPaintRecvMs": mw([p.get("settledPaintRecvMs") for p in P]),
        "pageMsArmToLast": mw([p["settled"]["msArmToLast"] for p in P if "settled" in p]),
        "framesArmToLast": mw([p["settled"]["framesArmToLast"] for p in P if "settled" in p]),
        "framesArmToPaint": mw([p["settledPaint"]["framesArmToPaint"] for p in P if "settledPaint" in p]),
        "missingSettle": sum(1 for p in P if "settledPaint" not in p),
        "hostMismatch": sum(p.get("hostMismatch", 0) for p in P),
        "applyFailed": sum(1 for p in P if not p.get("applyOk", True)),
        "framesMaxLe2": all(p["settledPaint"]["framesArmToPaint"] <= 2 for p in P if "settledPaint" in p),
    }

# Bind / type / disabled / ui / listen / size / res
S["E-bind-in"] = [{k: d["rows"]["E-bind-in"][k] for k in ["n", "nSlider", "nBad", "nScaledExact", "maxNormErr", "maxScaledErr", "maxTextErr"]} for d in D]
S["E-bind-out"] = [{k: d["rows"]["E-bind-out"][k] for k in ["n", "valueBad", "gestureBad", "beginTotal", "endTotal", "maxNormErr", "maxScaledErrOverTol", "normErrOver1e6", "settleMs"]} for d in D]
S["E-bind-out-normList"] = [d["rows"]["E-bind-out"]["normErrOver1e6List"] for d in D]
S["E-type"] = [{t: {k: v[k] for k in ["hostNorm", "expectNorm", "normErr", "planExpectNorm", "normErrVsPlan", "hostHz", "hostText", "gestureBegin", "gestureEnd"]}
                | {"fieldText": v["page"]["fieldText"], "inputOpened": v["page"]["inputOpened"]}
                for t, v in d["rows"]["E-type"].items()} for d in D[:1]]
S["E-type-allProcesses"] = [{t: [round(v["normErr"], 12), v["gestureBegin"], v["gestureEnd"]] for t, v in d["rows"]["E-type"].items()} for d in D]
S["E-disabled"] = [d["rows"]["E-disabled"]["passCount"] for d in D]
S["E-ui"] = [d["rows"]["E-ui"]["passCount"] for d in D]
S["E-ui-presetLoad"] = D[0]["rows"]["E-ui"]["unchangedByPresetLoad"]["report"]
S["E-listen"] = [{"calls": [[v["arg"], v["listenBand"], v["pass"]] for k, v in sorted(d["rows"]["E-listen"].items()) if k.startswith("ls")],
                  "afterClose": d["rows"]["E-listen"].get("listenAfterClose")} for d in D]
S["E-size"] = [{k: [v["editor"], v["pass"]] for k, v in d["rows"]["E-size"].items()} for d in D]
res = D[0]["rows"]["E-res"]
S["E-res"] = {
    "rows": [[r["url"].replace("juce://juce.backend", ""), r.get("status"), r.get("type"), r.get("bytes"), r.get("referenced")] for r in res["fetch"]["rows"]],
    "notFoundBeforeBenchFetch": [d["rows"]["E-res"]["resourceNotFoundBeforeBenchFetch"] for d in D],
    "notFoundAll": [d["rows"]["E-res"]["resourceNotFoundLogAll"] for d in D],
    "fonts": [{k: d["rows"]["E-res"]["fonts"].get(k) for k in ["check16", "check105", "checkItalic", "faces", "page", "dEBG", "dFallback", "dRefs"]}
              | {"diffEBG": d["rows"]["E-res"]["fonts"]["refEBG"]["diffVsPage"], "diffFallback": d["rows"]["E-res"]["fonts"]["refFallback"]["diffVsPage"],
                 "advEBG": d["rows"]["E-res"]["fonts"]["refEBG"]["adv"], "advFallback": d["rows"]["E-res"]["fonts"]["refFallback"]["adv"]} for d in D],
}
S["raf"] = [d.get("raf") for d in D]
S["timeouts"] = [d["timeouts"] for d in D]

# E-cycle
cyc = {"release": [], "debug": []}
for f, d in rel + dbg:
    log = open(f.replace(".json", ".log")).read()
    cyc["debug" if d["build"] == "Debug" else "release"].append({
        "file": os.path.basename(f), "cycles": len(d["cycles"]), "done": "BENCH DONE" in log,
        "assertions": len(re.findall(r"JUCE Assertion", log)), "leakLines": len(re.findall(r"(?i)leak", log)),
        "timeouts": len(d["timeouts"]), "closeMs": mw([c["closeMs"] for c in d["cycles"]])})
S["E-cycle"] = cyc
if dbg:
    S["debugRows"] = {k: dbg[0][1]["rows"][k].get("passCount") for k in ["E-disabled", "E-ui"]}

json.dump(S, open(os.path.join(R, "summary.json"), "w"), indent=1)
print(json.dumps(S, indent=1))
