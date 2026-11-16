"""PLAN-3.2 T10: row verdicts from one editor-bench JSON.

usage: python3 verdict32.py <bench.json> [--quiet]
Prints one line per row, then ALL-PASS or FAIL <rows>. The 3.1 rows use the
rules of verify-3.1/verdict.py (plus E-idle); the 3.2 rows read the bench's
own per-check verdicts (rows[...][check].pass) and the per-cycle / per-preset
records. Exit 0 ALL-PASS, 1 otherwise.
"""
import json, sys

d = json.load(open(sys.argv[1]))
quiet = "--quiet" in sys.argv
r = d.get("rows", {})
fails, lines = [], []


def chk(name, ok, note=""):
    lines.append(f"  [{'PASS' if ok else 'FAIL'}] {name}{('  ' + note) if note else ''}")
    if not ok:
        fails.append(name)


# ── 3.1 rows (verify-3.1/verdict.py rules) ───────────────────────────────────
c0 = d["cycles"][0] if d.get("cycles") else {}
chk("E-open", bool(c0) and 0 <= c0.get("secondFrameMs", -1) <= 500 and c0.get("pageReadyMs", -1) >= 0,
    f"cold {c0.get('secondFrameMs', -1):.1f} ms")
cons = [c.get("console", {}) for c in d.get("cycles", [])]
chk("E-props", bool(c0) and c0.get("allProps", {}).get("n") == 196
    and c0.get("allProps", {}).get("check", {}).get("b24_freq", {}).get("skew") == 0.203214481472969
    and all(c.get("nWarn", 0) == 0 and c.get("nErr", 0) == 0 and c.get("unknownWarn", 0) == 0 for c in cons))
cat = r.get("consoleAfterTests", {})
chk("console after tests", cat.get("nWarn", 1) == 0 and cat.get("nErr", 1) == 0, json.dumps(cat.get("warns", []))[:300])
idle = r.get("E-idle", {})
chk("E-idle", bool(idle) and not idle.get("evals") and not idle.get("posts"), f"evals {idle.get('evals')} posts {idle.get('posts')}")
bi = r.get("E-bind-in", {})
chk("E-bind-in", bi.get("n") == 196 and bi.get("nBad") == 0 and bi.get("maxNormErr", 1) <= 1e-6)
bo = r.get("E-bind-out", {})
chk("E-bind-out", bo.get("n") == 196 and bo.get("valueBad") == 0 and bo.get("gestureBad") == 0
    and bo.get("beginTotal") == 196 and bo.get("endTotal") == 196 and bo.get("maxScaledErrOverTol", 9) <= 1)
chk("E-disabled", r.get("E-disabled", {}).get("passCount") == 64)
chk("E-ui", r.get("E-ui", {}).get("passCount") == "14/14")
ls = r.get("E-listen", {})
chk("E-listen", ls.get("listenAfterClose") == -1 and all(v.get("pass") for k, v in ls.items() if k.startswith("ls")))
ty = r.get("E-type", {})
chk("E-type", abs(ty.get("1000", {}).get("hostNorm", -1) - 0.5) <= 1e-6 and ty.get("1000", {}).get("gestureBegin") == 1)
res = r.get("E-res", {})
MIME = {".html": "text/html", ".js": "application/javascript", ".css": "text/css", ".woff2": "font/woff2", "/": "text/html"}


def mime_ok(row):
    u = row["url"]
    ext = "/" if u.endswith("/") else u[u.rfind("."):]
    return (row.get("type") or "").startswith(MIME.get(ext, "?"))


rows = res.get("fetch", {}).get("rows", [])
chk("E-res", not res.get("resourceNotFoundBeforeBenchFetch") and len(rows) >= 11
    and all(x.get("status") == 200 and mime_ok(x) for x in rows) and res.get("fonts", {}).get("check16") is True)
sz = r.get("E-size", {})
if sz:
    chk("E-size", c0.get("openWidth") == 960 and c0.get("openHeight") == 600
        and all(v.get("pass") for v in sz.values() if isinstance(v, dict) and "pass" in v)
        and len([1 for v in sz.values() if isinstance(v, dict) and "pass" in v]) >= 4)
pr = d.get("presets", [])
methods = {x["method"] for x in pr}
chk("E-preset", len(methods) >= 5 and all(x.get("applyOk") and x.get("hostMismatch") == 0
                                          and x.get("settledPaint", {}).get("framesArmToPaint", 99) <= 2 for x in pr),
    f"{len(pr)} loads, methods {sorted(methods)}")
chk("timeouts", not d.get("timeouts"), json.dumps(d.get("timeouts"))[:300])
chk("listenBandAtEnd", d.get("listenBandAtEnd") == -1)

# ── 3.2 rows ─────────────────────────────────────────────────────────────────
init = [c.get("eqInit", {}) for c in d.get("cycles", [])]
chk("E-eq-init (every open)", bool(init) and all(i.get("pass") for i in init),
    f"{sum(1 for i in init if i.get('pass'))}/{len(init)} opens: first push 24 bands 1..24, 48 kHz, rateKnown")
# preset paths: the next push carries every changed band; the painted curve matches
cover = [x for x in pr if "eqMissing" in x]
chk("E-eq-init (preset paths: changed bands pushed)", len(cover) == len(pr) and all(x["eqMissing"] == 0 for x in cover),
    f"{len(cover)} loads, missing {sum(x.get('eqMissing', 0) for x in cover)}")
pv = sum(x.get("probeValid", 0) for x in pr)
ph = sum(x.get("probeHits", 0) for x in pr)
withCols = sum(1 for x in pr if x.get("probeValid", 0) >= 3)
chk("E-eq-init (preset paths: painted curve = engine)", pv > 0 and ph == pv and withCols >= len(pr) // 2,
    f"{ph}/{pv} probe columns hit over {len(pr)} loads; {withCols} loads with >= 3 in-plot columns")
eqi = r.get("E-eq-idle", {})
chk("E-eq-idle", bool(eqi.get("pass")), f"C++ emits {eqi.get('cppEmits')}, page eqSections {eqi.get('jsEqSections')}, all events {eqi.get('jsAllEvents')} over {eqi.get('idleSec')} s")

for name in ["E-eq-reload", "E-create", "E-full", "E-drag", "E-wheel", "E-dblclick", "E-delete", "E-ends", "E-stopped", "E-mono", "E-lag", "E-draw"]:
    row = r.get(name, {})
    checks = {k: v for k, v in row.items() if isinstance(v, dict) and "pass" in v}
    bad = [k for k, v in checks.items() if not v["pass"]]
    note = f"{len(checks) - len(bad)}/{len(checks)}"
    if name == "E-lag" and checks:
        det = next(iter(checks.values())).get("detail", {})
        note += f"  median {det.get('medianMs', -1):.1f} / p99 {det.get('p99Ms', -1):.1f} / max {det.get('maxMs', -1):.1f} ms over {det.get('matched')} moves (bound {det.get('boundMs', 0):.1f})"
    if name == "E-draw" and checks:
        note += "  " + str(next(iter(checks.values())).get("detail", ""))
    if bad:
        note += "  failing: " + "; ".join(bad)
    chk(name, bool(checks) and not bad, note)

fc = d.get("failedChecks", [])
chk("bench failedChecks empty", not fc, "; ".join(fc)[:400])

if not quiet:
    print("\n".join(lines))
print("FAIL " + ",".join(fails) if fails else "ALL-PASS")
sys.exit(1 if fails else 0)
