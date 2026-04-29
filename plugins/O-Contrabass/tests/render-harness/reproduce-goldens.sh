#!/usr/bin/env bash
# Phase 2.4a R34-pre / Phase 2.4b R35a / Phase 2.4c R36e — canonical reproduction
# of all 13 O-Contrabass render goldens. Locks invocations against duration-
# dependence trap (RESEARCH.md §17.10 Risk #10). Phase 2.4b extension (PLAN
# rev-9 pin #3): adds `sub-harmonics` + `sub-harmonics-stability` entries; HR-9
# IEEE 754 identity arithmetic preserves bit-exact regression for the 10 carry-
# forward goldens at SUB_HARMONICS=0 default. Phase 2.4c extension (PLAN rev-10
# pin #4): adds `saturator-tail-comparison` entry; HR-11 (zero production DSP
# edits) trivially preserves the 12 carry-forward goldens byte-identical.
# Usage: ./reproduce-goldens.sh           — render to /tmp/repro/, diff vs committed
#        ./reproduce-goldens.sh --quiet   — exit code only, no stdout
# Bash 3.2 compatible (macOS system bash) — uses parallel arrays, not associative.
set -euo pipefail

QUIET="${1:-}"
REPO_ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
GOLDEN_DIR="$REPO_ROOT/plugins/O-Contrabass/tests/render-harness/golden"
HARNESS="$REPO_ROOT/build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test"
OUTDIR="/tmp/repro"

mkdir -p "$OUTDIR"
cd "$REPO_ROOT/build"

# Build harness if missing.
if [ ! -x "$HARNESS" ]; then
    cmake --build . --target O-Contrabass-render-test --parallel >/dev/null
fi

# Parallel arrays (bash 3.2 compatible). Canonical invocations per RESEARCH §17.1:
# sustain 60 / release 5 default for sustained modes; 3-s notes for note-sequence;
# mode-locked sustain for vibrato/macro/slow/schelleng.
NAMES=(stiffness-zero-pre string-A string-D string-G detune-sweep-A note-sequence vibrato macro-sweep slow-lfo schelleng-stress sub-harmonics sub-harmonics-stability saturator-tail-comparison)
INVOCS=(
    "--note 28 --velocity 0.7 --sustain 60 --release 5 --infinite-sustain 1.0 --string-stiffness 0"
    "--string A"
    "--string D"
    "--string G"
    "--detune-sweep A"
    "--note-sequence 28:3,33:3,38:3,43:3,28:3"
    "--vibrato"
    "--macro-sweep"
    "--slow-lfo"
    "--schelleng-stress"
    "--sub-harmonics"
    "--sub-harmonics-stability"
    "--saturator-tail-comparison"
)

FAIL=0
TOTAL=0
for i in "${!NAMES[@]}"; do
    g="${NAMES[$i]}"
    invoc="${INVOCS[$i]}"
    [ -f "$GOLDEN_DIR/$g.wav.sha256" ] || continue
    wav="$OUTDIR/$g.wav"
    json="$OUTDIR/$g.json"
    # Harness may exit non-zero on quality-gate FAILs (peak / RMS thresholds);
    # WAVs are still bit-deterministic. sha256 is the truth-bar, not exit code.
    $HARNESS $invoc --out "$wav" --json "$json" >/dev/null 2>&1 || true
    computed=$(shasum -a 256 "$wav" | awk '{print $1}')
    expected=$(awk '{print $1}' "$GOLDEN_DIR/$g.wav.sha256")
    TOTAL=$((TOTAL + 1))
    if [ "$computed" = "$expected" ]; then
        [ -z "$QUIET" ] && echo "[PASS] $g  $computed"
    else
        [ -z "$QUIET" ] && echo "[FAIL] $g  computed=$computed  expected=$expected"
        FAIL=$((FAIL + 1))
    fi
done

if [ "$FAIL" -gt 0 ]; then
    [ -z "$QUIET" ] && echo "FAILED: $FAIL of $TOTAL goldens drifted"
    exit 1
fi
[ -z "$QUIET" ] && echo "OK: all $TOTAL goldens reproduce byte-identical"
exit 0
