#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# G9 (b) gate, one command (PLAN-3.2 P4): build the render harness, dump the
# 260 XR-g9 draws, score the shipping page's evaluator against them.
#
#   plugins/O-ParametricEQ/tests/g9b/run.sh            # build dir: <repo>/build
#   BUILD_DIR=/path/to/build plugins/O-ParametricEQ/tests/g9b/run.sh
#
# Needs a build configured with -DOUARICON_BUILD_TESTS=ON, and node (the UI
# gates already do). Exit 0 PASS, non-zero FAIL.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN="$(cd "$HERE/../.." && pwd)"
REPO="$(cd "$PLUGIN/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO/build}"
HARNESS="$BUILD_DIR/plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness"

ninja -C "$BUILD_DIR" O-ParametricEQ-render-harness > /dev/null
[ -x "$HARNESS" ] || { echo "FAIL: no harness at $HARNESS"; exit 1; }

DUMP="$(mktemp -d "${TMPDIR:-/tmp}/opeq-g9b-XXXXXX")"
trap 'rm -rf "$DUMP"' EXIT

( cd "$PLUGIN" && "$HARNESS" --dump-g9 "$DUMP" )
node "$HERE/g9b.mjs" "$PLUGIN/Source/ui/public/index.html" "$DUMP"
