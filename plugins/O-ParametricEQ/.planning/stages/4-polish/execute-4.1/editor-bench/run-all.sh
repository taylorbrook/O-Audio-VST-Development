#!/bin/bash
# PLAN-4.1 T26: the 3.3 run, one Release process by default (RUNS=1: the 3.3 rows as
# regression + the 4.1 rows, verdict33.py and verdict41.py on the same JSON), then
# the Debug --live pass (assertions, leaks). quiet() also waits for any other
# EditorBench (the page-mutant runner).
# PLAN-3.3 T11 (from PLAN-3.2 T10). Release: 5 fresh processes (each: 10 open /
# close cycles; the 3.1, 3.2 and 3.3 rows in cycle 0 with 10 reps x 5 preset
# methods; 10 s idle; then the size rows). Debug: 1 process, 10 cycles, cycles
# >= 1 live (noise, Pre+Post, meter, listen on at close), assertions + leak
# detector (E-cycle-dbg).
#
# WKWebView throttles requestAnimationFrame and timers while the window is
# occluded, and a LOCKED SCREEN occludes every window: rows then time out or
# read seconds of lag. Every process waits for an unlocked screen first, runs
# under caffeinate, and the run is void if the screen locked during it.
B="$(cd "$(dirname "$0")" && pwd)"
REL="$B/build-rel/EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench"
DBG="$B/build-dbg/EditorBench_artefacts/Debug/EditorBench.app/Contents/MacOS/EditorBench"
LOCK="${LOCKSTATE:-$B/lockstate}"
N="${RUNS:-1}"
mkdir -p "$B/results"
[ -x "$LOCK" ] || swiftc -O -o "$LOCK" "$B/lockstate.swift" || exit 1

quiet() { until ! pgrep -f pluginval >/dev/null && ! pgrep -x auval >/dev/null && ! pgrep -x ninja >/dev/null                 && ! pgrep -f "render-harness|designer-harness|probe33|g9b.mjs|EditorBench.app" >/dev/null; do echo "waiting for validators / builds / harnesses..."; sleep 15; done; }
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

for i in $(seq 1 "$N"); do
  run "rel-$i" "$REL" --cycles 10 --reps 10 --idle 10
done
run dbg "$DBG" --cycles 10 --reps 2 --idle 2 --live
echo "dbg assertions: $(grep -c 'JUCE Assertion' "$B/results/dbg.log")"
echo "dbg leak lines: $(grep -ci 'leak' "$B/results/dbg.log")"
for f in "$B"/results/rel-*.json; do
  echo "== $(basename "$f"): 3.3 rows $(python3 "$B/verdict33.py" "$f" --quiet) | 4.1 rows $(python3 "$B/verdict41.py" "$f" --quiet)"
done
echo "== dbg.json: 3.3 rows $(python3 "$B/verdict33.py" "$B/results/dbg.json" --quiet --live) | 4.1 rows $(python3 "$B/verdict41.py" "$B/results/dbg.json" --quiet)"
