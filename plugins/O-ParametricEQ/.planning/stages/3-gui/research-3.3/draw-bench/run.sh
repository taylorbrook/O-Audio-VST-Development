#!/bin/bash
# research-3.3 draw-bench. Usage: ./run.sh <name> rel|cache [extra args]
# Waits for an unlocked screen, runs under caffeinate, voids the run if the
# screen locked during it (WKWebView throttles rAF on a locked screen; see
# execute-3.2/editor-bench/run-all.sh).
B="$(cd "$(dirname "$0")" && pwd)"
name="$1"; kind="$2"; shift 2
if [ "$kind" = cache ]; then
  BIN="$B/build-cache/EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench"; X=--cachebuild
else
  BIN="$B/build-rel/EditorBench_artefacts/Release/EditorBench.app/Contents/MacOS/EditorBench"; X=
fi
LOCK="$B/lockstate"
mkdir -p "$B/results"
until ! pgrep -f pluginval >/dev/null && ! pgrep -x auval >/dev/null; do echo "waiting for validators..."; sleep 15; done
until [ "$("$LOCK")" = "0" ]; do sleep 20; done
echo "$name lockedBefore $("$LOCK")"
caffeinate -d -u -i "$BIN" --drawbench --cycles 1 --notests --nosize $X "$@" --out "$B/results/$name.json" -NSAppSleepDisabled YES > "$B/results/$name.log" 2>&1
rc=$?
locked="$("$LOCK")"
echo "$name exit $rc lockedAfter $locked"
[ "$locked" = "0" ] || echo "$name VOID: the screen locked during the run"
grep -E "^(heavy|light)/" "$B/results/$name.log"
