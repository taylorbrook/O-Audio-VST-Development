#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Meter ballistics gate (PLAN-3.3 T9a): the page's meterStep at 60 and 120 Hz.
#   plugins/O-ParametricEQ/tests/meter-ballistics/run.sh
# Needs node. Exit 0 PASS, non-zero FAIL.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN="$(cd "$HERE/../.." && pwd)"
node "$HERE/ballistics.mjs" "$PLUGIN/Source/ui/public/index.html"
