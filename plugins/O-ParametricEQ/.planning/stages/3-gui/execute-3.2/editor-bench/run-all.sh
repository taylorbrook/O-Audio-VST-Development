#!/bin/bash
# PLAN-3.2 T10. Release: 5 fresh processes (each: 10 open / close cycles; the
# 3.1 rows and the 3.2 rows in cycle 0 with 10 reps x 5 preset methods; 10 s
# idle; then the size rows). Debug: 1 process, 10 cycles with eqSections live,
# assertions + leak detector (E-cycle-dbg).
#
# WKWebView throttles requestAnimationFrame and timers while the window is
# occluded, and a LOCKED SCREEN occludes every window: rows then time out or
# read seconds of lag (execute-3.2 session, 2026-11-15: the unmodified 3.1
# bench failed cycle 0 the same way). Every process therefore waits for an
# unlocked screen first, runs under caffeinate, and the run is void if the
# screen locked during it (checked again after).
B="$(cd "$(dirname "$0")" && pwd)"
REL="$B/build-rel/EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench"
DBG="$B/build-dbg/EditorBench_artefacts/Debug/EditorBench.app/Contents/MacOS/EditorBench"
LOCK="${LOCKSTATE:-$B/lockstate}"
mkdir -p "$B/results"
[ -x "$LOCK" ] || swiftc -O -o "$LOCK" "$B/lockstate.swift" || exit 1

quiet() { until ! pgrep -f pluginval >/dev/null && ! pgrep -x auval >/dev/null; do echo "waiting for validators..."; sleep 15; done; }
unlocked() { until [ "$("$LOCK")" = "0" ]; do sleep 20; done; }
run() {   # name, binary, args...
  local name="$1"; shift
  quiet; unlocked
  caffeinate -d -u -i "$@" --out "$B/results/$name.json" -NSAppSleepDisabled YES > "$B/results/$name.log" 2>&1
  local rc=$?
  local locked; locked="$("$LOCK")"
  echo "$name exit $rc lockedAfter $locked"
  [ "$locked" = "0" ] || echo "$name VOID: the screen locked during the run"
}

for run in 1 2 3 4 5; do
  run "rel-$run" "$REL" --cycles 10 --reps 10 --idle 10
done
run dbg "$DBG" --cycles 10 --reps 2 --idle 2
echo "dbg assertions: $(grep -c 'JUCE Assertion' "$B/results/dbg.log")"
echo "dbg leak lines: $(grep -ci 'leak' "$B/results/dbg.log")"
for f in "$B"/results/rel-*.json "$B/results/dbg.json"; do
  echo "== $(basename "$f"): $(python3 "$B/verdict32.py" "$f" --quiet)"
done
