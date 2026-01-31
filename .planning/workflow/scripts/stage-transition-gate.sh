#!/bin/bash
# stage-transition-gate.sh - Gates stage transitions on handoff validity
#
# Usage: stage-transition-gate.sh <plugin> <from-stage> <to-stage>
#
# Blocks transition if:
#   - No handoff document exists for from-stage
#   - Handoff validation fails (schema or artifacts)
#   - Handoff's receivingStage doesn't match to-stage
#
# Exit codes:
#   0 - GATE PASSED (transition approved)
#   1 - GATE BLOCKED (validation or missing handoff)
#   2 - Usage error

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
VALIDATE_SCRIPT="$SCRIPT_DIR/validate-handoff.sh"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

show_help() {
    echo "stage-transition-gate.sh - Gate stage transitions on handoff validity"
    echo ""
    echo "Usage: stage-transition-gate.sh <plugin> <from-stage> <to-stage>"
    echo ""
    echo "Arguments:"
    echo "  plugin       Plugin name (e.g., O-IntonationPad)"
    echo "  from-stage   Stage transitioning from (e.g., 1-foundation)"
    echo "  to-stage     Stage transitioning to (e.g., 2-dsp)"
    echo ""
    echo "Stages: 0-ideation, 1-foundation, 2-dsp, 3-gui, 4-polish"
    echo ""
    echo "Checks:"
    echo "  1. Handoff document exists for from-stage"
    echo "  2. Handoff passes schema + artifact validation"
    echo "  3. Handoff's receivingStage matches to-stage"
    echo ""
    echo "Exit codes:"
    echo "  0 - GATE PASSED (transition approved)"
    echo "  1 - GATE BLOCKED (handoff missing/invalid/mismatch)"
    echo "  2 - Usage error"
}

# Parse arguments
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    show_help
    exit 0
fi

if [ $# -ne 3 ]; then
    echo -e "${RED}ERROR: Expected 3 arguments${NC}"
    echo ""
    show_help
    exit 2
fi

PLUGIN="$1"
FROM_STAGE="$2"
TO_STAGE="$3"

echo "Stage Transition Gate Check"
echo "========================================"
echo "Plugin:     $PLUGIN"
echo "From Stage: $FROM_STAGE"
echo "To Stage:   $TO_STAGE"
echo "========================================"
echo ""

# Construct handoff path
HANDOFF_FILE="$PROJECT_ROOT/plugins/${PLUGIN}/.planning/stages/${FROM_STAGE}/HANDOFF.json"

# Check 1: Handoff file exists
echo -e "${CYAN}Check 1: Handoff document exists${NC}"
if [ ! -f "$HANDOFF_FILE" ]; then
    echo -e "${RED}GATE BLOCKED: No handoff document for stage ${FROM_STAGE}${NC}"
    echo ""
    echo "Expected: $HANDOFF_FILE"
    echo ""
    echo -e "${YELLOW}Create handoff with: /plugin-handoff ${PLUGIN} ${FROM_STAGE}${NC}"
    exit 1
fi
echo -e "${GREEN}  Found: $HANDOFF_FILE${NC}"
echo ""

# Check 2: Handoff validation passes
echo -e "${CYAN}Check 2: Handoff validation${NC}"
if ! "$VALIDATE_SCRIPT" "$HANDOFF_FILE"; then
    echo ""
    echo -e "${RED}GATE BLOCKED: Handoff validation failed${NC}"
    echo ""
    echo "Fix validation errors and try again."
    exit 1
fi
echo ""

# Check 3: Receiving stage matches
echo -e "${CYAN}Check 3: Receiving stage matches${NC}"
DECLARED_NEXT=$(jq -r '.contextForNextStage.receivingStage // empty' "$HANDOFF_FILE")

if [ -z "$DECLARED_NEXT" ]; then
    echo -e "${RED}GATE BLOCKED: No receivingStage declared in handoff${NC}"
    echo ""
    echo "Handoff must specify contextForNextStage.receivingStage"
    exit 1
fi

if [ "$DECLARED_NEXT" != "$TO_STAGE" ]; then
    echo -e "${RED}GATE BLOCKED: Handoff targets ${DECLARED_NEXT}, not ${TO_STAGE}${NC}"
    echo ""
    echo "Handoff was created for transition to '$DECLARED_NEXT',"
    echo "but you're trying to transition to '$TO_STAGE'."
    echo ""
    echo "Options:"
    echo "  1. Transition to the declared stage: $DECLARED_NEXT"
    echo "  2. Regenerate handoff for correct transition"
    exit 1
fi
echo -e "${GREEN}  Declared: $DECLARED_NEXT (matches)${NC}"
echo ""

# All checks passed
echo "========================================"
echo -e "${GREEN}GATE PASSED: Transition ${FROM_STAGE} -> ${TO_STAGE} approved${NC}"
echo "========================================"
echo ""
echo "Next step: Start ${TO_STAGE} execution"
echo "Command: /plugin-execute ${PLUGIN} ${TO_STAGE}"
exit 0
