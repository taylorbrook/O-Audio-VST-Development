"""Verify 3.1: row verdicts from one editor-bench JSON (independent of analyze.py)."""
import json, sys

d = json.load(open(sys.argv[1]))
r = d.get("rows", {})
fails = []

def chk(name, ok):
    if not ok:
        fails.append(name)

c0 = d["cycles"][0] if d.get("cycles") else {}
chk("E-open", bool(c0) and 0 <= c0.get("secondFrameMs", -1) <= 500 and c0.get("pageReadyMs", -1) >= 0)
cons = [c.get("console", {}) for c in d.get("cycles", [])]
chk("E-props", bool(c0) and c0.get("allProps", {}).get("n") == 196 and c0.get("allProps", {}).get("check", {}).get("b24_freq", {}).get("skew") == 0.203214481472969
    and all(c.get("nWarn", 0) == 0 and c.get("nErr", 0) == 0 and c.get("unknownWarn", 0) == 0 for c in cons))
bi = r.get("E-bind-in", {})
chk("E-bind-in", bi.get("n") == 196 and bi.get("nBad") == 0 and bi.get("maxNormErr", 1) <= 1e-6)
bo = r.get("E-bind-out", {})
chk("E-bind-out", bo.get("n") == 196 and bo.get("valueBad") == 0 and bo.get("gestureBad") == 0
    and bo.get("beginTotal") == 196 and bo.get("endTotal") == 196 and bo.get("maxScaledErrOverTol", 9) <= 1)
chk("E-disabled", r.get("E-disabled", {}).get("passCount") == 64)
chk("E-ui", r.get("E-ui", {}).get("passCount") == "14/14")
ls = r.get("E-listen", {})
lsok = ls.get("listenAfterClose") == -1 and all(v.get("pass") for k, v in ls.items() if k.startswith("ls"))
chk("E-listen", lsok)
ty = r.get("E-type", {})
chk("E-type", abs(ty.get("1000", {}).get("hostNorm", -1) - 0.5) <= 1e-6 and ty.get("1000", {}).get("gestureBegin") == 1)
res = r.get("E-res", {})
MIME = {".html": "text/html", ".js": "application/javascript", ".css": "text/css", ".woff2": "font/woff2", "/": "text/html"}
def mime_ok(row):
    u = row["url"]; ext = "/" if u.endswith("/") else u[u.rfind("."):]
    return row.get("type", "").startswith(MIME.get(ext, "?"))
rows = res.get("fetch", {}).get("rows", [])
chk("E-res", not res.get("resourceNotFoundBeforeBenchFetch") and len(rows) >= 11 and all(x.get("status") == 200 and mime_ok(x) for x in rows)
    and res.get("fonts", {}).get("check16") is True)
sz = r.get("E-size", {})
chk("E-size", c0.get("openWidth") == 960 and c0.get("openHeight") == 600 and all(v.get("pass") for k, v in sz.items() if isinstance(v, dict) and "pass" in v) and len([1 for v in sz.values() if isinstance(v, dict) and "pass" in v]) >= 4)
pr = d.get("presets", [])
methods = {x["method"] for x in pr}
chk("E-preset", len(methods) >= 5 and all(x.get("applyOk") and x.get("hostMismatch") == 0 and x.get("settledPaint", {}).get("framesArmToPaint", 99) <= 2 for x in pr))
chk("timeouts", not d.get("timeouts"))
chk("listenBandAtEnd", d.get("listenBandAtEnd") == -1)
print("FAIL " + ",".join(fails) if fails else "ALL-PASS")
