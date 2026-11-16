#!/bin/bash
# PLAN-3.1 T11. Release: 5 fresh processes (each: 10 open / close cycles, the
# test rows in cycle 0 with 10 reps x 5 preset methods, 10 s idle, then the
# size rows). Debug: 1 process, 10 cycles, assertions + leak detector.
# Timed runs wait until no pluginval / auval is running (shared machine).
B="$(cd "$(dirname "$0")" && pwd)"
REL="$B/build-rel/EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench"
DBG="$B/build-dbg/EditorBench_artefacts/Debug/EditorBench.app/Contents/MacOS/EditorBench"
mkdir -p "$B/results"

quiet() { until ! pgrep -f pluginval >/dev/null && ! pgrep -x auval >/dev/null; do echo "waiting for validators..."; sleep 15; done; }

for run in 1 2 3 4 5; do
  quiet
  "$REL" --cycles 10 --reps 10 --idle 10 --out "$B/results/rel-$run.json" -NSAppSleepDisabled YES > "$B/results/rel-$run.log" 2>&1
  echo "rel-$run exit $?"
done
quiet
"$DBG" --cycles 10 --reps 2 --idle 2 --out "$B/results/dbg.json" -NSAppSleepDisabled YES > "$B/results/dbg.log" 2>&1
echo "dbg exit $?"
echo "dbg assertions: $(grep -c 'JUCE Assertion' "$B/results/dbg.log")"
echo "dbg leak lines: $(grep -ci 'leak' "$B/results/dbg.log")"
