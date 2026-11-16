import json, glob, statistics, sys, os
B = os.path.dirname(os.path.abspath(__file__))

def mw(xs):
    xs = [x for x in xs if x is not None and x >= 0]
    if not xs: return "n/a"
    return f"{statistics.median(xs):.1f} / {max(xs):.1f} (n={len(xs)})"

for mode in ["full", "fallback"]:
    files = sorted(glob.glob(f"{B}/results/rel-{mode}-*.json"))
    if not files: continue
    D = [json.load(open(f)) for f in files]
    print(f"\n===== {mode}: {len(D)} processes =====")
    cold = [d["cycles"][0] for d in D]
    warm = [c for d in D for c in d["cycles"][1:]]
    for label, cs in [("cold (cycle 0)", cold), ("warm (cycles 1-9)", warm)]:
        print(f"-- {label}")
        for k in ["ctorMs", "webViewCtorMs", "attachMs", "optionsMs", "scriptStartMs", "dclMs", "allPropsMs", "firstPaintMs", "secondFrameMs", "closeMs"]:
            print(f"   {k:16s} {mw([c.get(k) for c in cs])}")
    print("   cycleMs (warm)  ", mw([c["cycleMs"] for c in warm]))
    c0 = D[0]["cycles"][0]
    print("   relays", c0["numRelays"], "listeners", c0["numEventListeners"], "estUserScriptBytes", c0["estUserScriptBytes"], "eventIdScript", c0["estEventIdScriptBytes"], "initData", c0["estInitDataBytes"])
    print("   allProps evals/posts", c0["allProps"]["evals"], c0["allProps"]["posts"], "check", json.dumps(c0["allProps"]["check"])[:300])
    cons = [c.get("console", {}) for d in D for c in d["cycles"]]
    print("   console nWarn max", max(x.get("nWarn", 0) for x in cons), "nErr max", max(x.get("nErr", 0) for x in cons), "unknown", max(x.get("unknownWarn", 0) for x in cons))
    print("   warns sample", cons[0].get("warns"))
    print("   nativeCalls", c0["nativeCalls"], "missing", c0["missingResources"])
    print("   timeouts", [d["timeouts"] for d in D])
    print("   raf", [ (round(d["raf"]["median"],1), round(d["raf"]["mean"],2)) for d in D if d.get("raf")])
    for m in ["single", "msgLoop", "setState", "offThread"]:
        P = [p for d in D for p in d["presets"] if p["method"] == m]
        print(f"-- preset {m}: expected {sorted(set(p['expected'] for p in P))}")
        print(f"   applyMs (C++ sync)    {mw([p['applyMs'] for p in P])}")
        print(f"   settledRecvMs         {mw([p.get('settledRecvMs') for p in P])}")
        print(f"   settledPaintRecvMs    {mw([p.get('settledPaintRecvMs') for p in P])}")
        print(f"   curvePaintRecvMs      {mw([p.get('curvePaintRecvMs') for p in P])}")
        fr = [p['settledPaint']['framesArmToPaint'] for p in P if 'settledPaint' in p]
        frl = [p['settled']['framesArmToLast'] for p in P if 'settled' in p]
        frc = [p['curvePaint']['framesArmToCurvePaint'] for p in P if 'curvePaint' in p]
        print(f"   frames arm->last value {mw(frl)} ; arm->paint {mw(fr)} ; arm->curve paint {mw(frc)}")
        print(f"   page msArmToLast      {mw([p['settled']['msArmToLast'] for p in P if 'settled' in p])}  bytes {sorted(set(p['settled']['bytes'] for p in P if 'settled' in p))[:3]}")
        print(f"   missing settle: {sum(1 for p in P if 'settledPaint' not in p)}")
    for d in D:
        print("   idle", {k: (v.get("evals"), v.get("posts"), round(v.get("ms", 0))) for k, v in d["idle"].items()})
