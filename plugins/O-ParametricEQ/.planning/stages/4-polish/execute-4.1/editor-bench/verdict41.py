"""PLAN-4.1 T26: verdicts for the 4.1 editor-bench rows from one bench JSON.

usage: python3 verdict41.py <bench.json> [--quiet]

Thresholds (RESEARCH-4 Q6 / Q1; the bench applies them per check and this
script re-reads the key numbers so the thresholds are visible here too):
  E-del-listen     every band: page post of setListenBand(0) < 20 ms after the
                   Delete key AND C++ arrival < 20 ms after the same-channel
                   'delStart' marker; listen -1 and the band disabled after.
  E-listen33-aria  latch off, drag band 3: >= 10 frames sampled, 0 with
                   aria-pressed "true"; listenBand 2 and band 3 selected
                   during the drag (non-vacuous); off after release.
  E-tipfade        11.5px at every frame with opacity > 0 after the dismiss and
                   at the synchronous read after hideTooltip; >= 2 frames caught
                   mid-fade; class + label binding gone after the fade with the
                   box back at the body size (!= 11.5px); a re-show 40 ms in keeps
                   the class past the 160 ms fallback; the no-transition path
                   clears it by the timer.
  E-tip-lang       centre and right edge x fr and zh-Hans: text = the language's
                   LABELS entry, width differs from en by > 0.5 px, left >= 0,
                   left + width <= innerWidth, top >= 0, bottom <= innerHeight.
  E-tip-v13        hover help on, point tip up: the hover tip has no
                   data-i18n "bands-full", carries the control's own text, and
                   keeps it through a language re-apply.
  E-preset-listen  latched b03: Kick Tighten load -> listenBand 2 at 400 ms
                   and 1 s; De-Mud load -> -1 at 400 ms, b03 off.
Prints one line per row, then ALL-PASS or FAIL <rows>. Exit 0 / 1.
"""
import json, sys

d = json.load(open(sys.argv[1]))
quiet = "--quiet" in sys.argv
r = d.get("rows", {})
fails, lines = [], []

ROWS = ["E-del-listen", "E-listen33-aria", "E-tipfade", "E-tip-lang", "E-tip-v13", "E-preset-listen"]
MIN_CHECKS = {"E-del-listen": 3, "E-listen33-aria": 2, "E-tipfade": 5, "E-tip-lang": 4, "E-tip-v13": 1, "E-preset-listen": 4}
FRAME_MS = 20.0


def checks_of(row):
    return {k: v for k, v in row.items() if isinstance(v, dict) and "pass" in v}


for name in ROWS:
    row = r.get(name, {})
    cs = checks_of(row)
    bad = [k for k, v in cs.items() if not v["pass"]]
    ok = len(cs) >= MIN_CHECKS[name] and not bad
    note = f"{len(cs) - len(bad)}/{len(cs)}"
    if name == "E-del-listen":
        posts = [c.get("detail", {}).get("pagePostMs", -1) for c in cs.values()]
        arr = [c.get("detail", {}).get("nativeArrivalMs", -1) for c in cs.values()]
        ok = ok and posts and all(0 <= x < FRAME_MS for x in posts) and all(0 <= x < FRAME_MS for x in arr)
        note += "  page post ms " + ", ".join(f"{x:.2f}" for x in posts) + " | C++ arrival ms " + ", ".join(f"{x:.2f}" for x in arr)
    if name == "E-listen33-aria":
        det = next((c.get("detail", {}) for k, c in cs.items() if k.startswith("latch off")), {})
        note += f"  frames {det.get('frames')}, true {det.get('framesTrue')}, listen during {det.get('listenDuringDrag')}, selected {det.get('selectedDuringDrag')}"
    if name == "E-tipfade":
        note += f"  fontSize {row.get('fontSizeMinPx')} - {row.get('fontSizeMaxPx')} px over the fade, {row.get('framesMidFade')} mid-fade frames, re-show at opacity {row.get('reshowMidOpacity')}"
    if name == "E-tip-lang":
        parts = []
        for k, c in cs.items():
            det = c.get("detail", {})
            parts.append(f"{k.split(':')[0]}: w {det.get('widthEn', 0):.1f}->{det.get('widthAfter', 0):.1f}, right {det.get('rightEdge', 0):.1f}/{det.get('innerWidth')}")
        note += "  " + "; ".join(parts)
    if bad:
        note += "  failing: " + "; ".join(bad)
    lines.append(f"  [{'PASS' if ok else 'FAIL'}] {name}  {note}")
    if not ok:
        fails.append(name)

lines.append(f"  [info] real-home Factory untouched: {d.get('realFactoryUntouched')}; bench factory: {d.get('factory')}")
if d.get("realFactoryUntouched") is False:
    fails.append("real-home-Factory")

if not quiet:
    print("\n".join(lines))
print("FAIL " + ",".join(fails) if fails else "ALL-PASS")
sys.exit(1 if fails else 0)
