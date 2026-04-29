#!/usr/bin/env bash
# Phase 2.4b R35-pre — spectral pre-flight at SUB_HARMONICS=1.0 with bias-active.
# Verifies pass_subharmAudible threshold per RESEARCH §18.6 BEFORE R35 atomic commit.
# Exit code: 0 on STRICT-PASS or SOFT-PASS; 1 on HARD-FAIL.
set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
HARNESS="$REPO_ROOT/build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test"
if [ ! -x "$HARNESS" ]; then
    cd "$REPO_ROOT/build"
    cmake --build . --target O-Contrabass-render-test --parallel >/dev/null
    cd - >/dev/null
fi
# Harness may exit non-zero on pass_combo=false (e.g. subharmEnergyRatio under
# strict threshold) — JSON is still valid for the pre-flight evaluation below.
"$HARNESS" --sub-harmonics \
         --out /tmp/preflight-subharm.wav \
         --json /tmp/preflight-subharm.json >/dev/null 2>&1 || true
RATIO=$(python3 -c "import json; print(json.load(open('/tmp/preflight-subharm.json'))['subharmEnergyRatio'])")
echo "subharmEnergyRatio = $RATIO"
python3 - <<EOF
import sys
r = $RATIO
if r >= 0.40:
    print("STRICT-PASS — proceed to R35 atomic commit")
    sys.exit(0)
elif r >= 0.30:
    print("SOFT-PASS within v1.0 budget — Phase 2.4-bis remediation flag (track in R35 commit body)")
    sys.exit(0)
else:
    print("HARD-FAIL — escalate to architecture §661 fallback 1 retune (kForceBoost 0.8->0.4 in SubHarmonicBias.h)")
    sys.exit(1)
EOF
