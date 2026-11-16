#!/bin/bash
# Release: 5 processes per mode (each: 10 open/close cycles, 10 reps x 4 preset methods, idle 5 s x 2)
# Modes interleaved so drift (thermal, background load) hits both equally.
B="$(cd "$(dirname "$0")" && pwd)"
REL="$B/build-rel/RelayBench_artefacts/Release/RelayBench.app/Contents/MacOS/RelayBench"
DBG="$B/build-dbg/RelayBench_artefacts/Debug/RelayBench.app/Contents/MacOS/RelayBench"
mkdir -p "$B/results"
for run in 1 2 3 4 5; do
  "$REL" --cycles 10 --reps 10 --idle 5 --out "$B/results/rel-full-$run.json" -NSAppSleepDisabled YES > "$B/results/rel-full-$run.log" 2>&1
  echo "rel-full-$run exit $?"
  "$REL" --fallback --cycles 10 --reps 10 --idle 5 --out "$B/results/rel-fallback-$run.json" -NSAppSleepDisabled YES > "$B/results/rel-fallback-$run.log" 2>&1
  echo "rel-fallback-$run exit $?"
done
# Debug: leak detector + assertion log, full mode, 10 cycles
"$DBG" --cycles 10 --reps 2 --idle 1 --out "$B/results/dbg-full.json" -NSAppSleepDisabled YES > "$B/results/dbg-full.log" 2>&1
echo "dbg-full exit $?"
"$DBG" --fallback --cycles 10 --reps 2 --idle 1 --out "$B/results/dbg-fallback.json" -NSAppSleepDisabled YES > "$B/results/dbg-fallback.log" 2>&1
echo "dbg-fallback exit $?"
