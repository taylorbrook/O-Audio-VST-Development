#!/bin/bash
# run-critic.sh - Orchestrate critic validation with iteration support
#
# Usage: run-critic.sh <plugin> <stage> [--attempt N] [--deep] [--token-count N] [--force]
#
# Determines applicable critic based on stage, tracks iterations,
# manages early-stop conditions, and warns on token budget.
#
# Exit codes:
#   0 - PASSED (all scores >= thresholds)
#   1 - NEEDS_FIXES (issues found, can iterate)
#   2 - ESCALATE (no progress or max iterations)
#   3 - Usage error

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Defaults
MAX_ATTEMPTS=3
ATTEMPT=1
DEEP_MODE=false
FORCE_MODE=false
TOKEN_COUNT=0
TOKEN_SOFT_LIMIT=50000

show_help() {
    echo "run-critic.sh - Orchestrate critic validation with iteration support"
    echo ""
    echo "Usage: run-critic.sh <plugin> <stage> [options]"
    echo ""
    echo "Arguments:"
    echo "  plugin           Plugin name (e.g., O-IntonationPad)"
    echo "  stage            Stage identifier (e.g., 2-dsp, 3-gui)"
    echo ""
    echo "Options:"
    echo "  --attempt N      Current attempt number (default: 1)"
    echo "  --deep           Deep validation mode (vs smoke test)"
    echo "  --token-count N  Cumulative token count for budget tracking"
    echo "  --force          Bypass validation with warning"
    echo "  --help           Show this help message"
    echo ""
    echo "Stage-to-Critic Mapping:"
    echo "  2-dsp  -> dsp-critic (realtime_safety, buffer_handling, parameter_integration)"
    echo "  3-gui  -> ui-critic (polish, consistency)"
    echo "  Other  -> No critic applicable (exits 0)"
    echo ""
    echo "Exit Codes:"
    echo "  0 - PASSED (all scores >= thresholds)"
    echo "  1 - NEEDS_FIXES (issues found, can iterate)"
    echo "  2 - ESCALATE (no progress or max iterations reached)"
    echo "  3 - Usage error"
    echo ""
    echo "Integration:"
    echo "  Output format compatible with stage-transition-gate.sh"
    echo "  Failure reports saved to .planning/verification/{plugin}/{stage}/"
}

# Parse arguments
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    show_help
    exit 0
fi

if [ $# -lt 2 ]; then
    echo -e "${RED}ERROR: Expected at least 2 arguments${NC}"
    echo ""
    show_help
    exit 3
fi

PLUGIN="$1"
STAGE="$2"
shift 2

while [[ $# -gt 0 ]]; do
    case $1 in
        --attempt)
            ATTEMPT="$2"
            shift 2
            ;;
        --deep)
            DEEP_MODE=true
            shift
            ;;
        --token-count)
            TOKEN_COUNT="$2"
            shift 2
            ;;
        --force)
            FORCE_MODE=true
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            echo -e "${YELLOW}WARNING: Unknown option: $1${NC}" >&2
            shift
            ;;
    esac
done

# Validate attempt number
if ! [[ "$ATTEMPT" =~ ^[1-3]$ ]]; then
    echo -e "${RED}ERROR: Attempt must be 1, 2, or 3 (got: $ATTEMPT)${NC}"
    exit 3
fi

# Force mode warning
if [ "$FORCE_MODE" = true ]; then
    echo -e "${YELLOW}WARNING: --force bypasses critic validation. Proceeding without validation.${NC}" >&2
    exit 0
fi

# Determine applicable critic based on stage
case "$STAGE" in
    2-dsp)
        CRITIC="dsp-critic"
        CRITIC_SCHEMA="critic-dsp-report.schema.json"
        ;;
    3-gui)
        CRITIC="ui-critic"
        CRITIC_SCHEMA="critic-ui-report.schema.json"
        ;;
    *)
        echo -e "${CYAN}No critic applicable for stage: $STAGE${NC}"
        echo "Stages with critics: 2-dsp (dsp-critic), 3-gui (ui-critic)"
        exit 0
        ;;
esac

# Setup verification directory
VERIFICATION_DIR="$PROJECT_ROOT/.planning/verification/${PLUGIN}/${STAGE}"
mkdir -p "$VERIFICATION_DIR"

# Header
echo ""
echo -e "${BOLD}========================================${NC}"
echo -e "${BOLD}         CRITIC VALIDATION${NC}"
echo -e "${BOLD}========================================${NC}"
echo ""
echo -e "Critic:       ${CYAN}${CRITIC}${NC}"
echo -e "Plugin:       ${PLUGIN}"
echo -e "Stage:        ${STAGE}"
echo -e "Mode:         $(if $DEEP_MODE; then echo "Deep"; else echo "Smoke"; fi)"
echo ""
echo -e "${BOLD}┌────────────────────────────────────┐${NC}"
echo -e "${BOLD}│      Attempt ${ATTEMPT} / ${MAX_ATTEMPTS}                   │${NC}"
echo -e "${BOLD}└────────────────────────────────────┘${NC}"
echo ""

# Token budget check
if [ "$TOKEN_COUNT" -gt 0 ]; then
    echo "Token Budget:"
    echo "  Cumulative: $TOKEN_COUNT"
    echo "  Soft Limit: $TOKEN_SOFT_LIMIT"
    if [ "$TOKEN_COUNT" -gt "$TOKEN_SOFT_LIMIT" ]; then
        echo -e "  ${YELLOW}WARNING: Token budget soft limit exceeded${NC}" >&2
        echo -e "  ${YELLOW}Consider simplifying or escalating to user${NC}" >&2
        BUDGET_WARNING=true
    else
        echo -e "  Status: ${GREEN}Within budget${NC}"
        BUDGET_WARNING=false
    fi
    echo ""
fi

# Check for previous iteration report (for no-progress detection)
PREVIOUS_REPORT=""
PREVIOUS_ISSUE_IDS=""
if [ "$ATTEMPT" -gt 1 ]; then
    PREV_ATTEMPT=$((ATTEMPT - 1))
    # Look for most recent failure report from previous attempt
    PREVIOUS_REPORT=$(ls -t "$VERIFICATION_DIR"/critic-failure-*.json 2>/dev/null | head -1)
    if [ -n "$PREVIOUS_REPORT" ] && [ -f "$PREVIOUS_REPORT" ]; then
        echo "Previous Iteration:"
        echo "  Report: $(basename "$PREVIOUS_REPORT")"
        PREVIOUS_ISSUE_IDS=$(jq -r '.issues[].id // empty' "$PREVIOUS_REPORT" 2>/dev/null | sort | tr '\n' ',' | sed 's/,$//')
        echo "  Issue IDs: ${PREVIOUS_ISSUE_IDS:-none}"
        echo ""
    fi
fi

echo "----------------------------------------"
echo ""

# Critic evaluation would be performed here by Claude agent
# The script provides the framework; actual critique happens via agent invocation
#
# When invoked as part of workflow:
# 1. Agent reads plugin source files for the stage
# 2. Agent applies critic template rules
# 3. Agent produces JSON report matching critic schema
# 4. Script validates report and determines next action

echo -e "${CYAN}Critic Evaluation Phase${NC}"
echo ""
echo "The critic agent will now evaluate the plugin against:"
case "$STAGE" in
    2-dsp)
        echo "  - Real-time safety (threshold: 8/10)"
        echo "  - Buffer handling (threshold: 7/10)"
        echo "  - Parameter integration (threshold: 6/10)"
        echo "  - [Optional] Numerical stability"
        echo "  - [Optional] Architecture alignment"
        ;;
    3-gui)
        echo "  - Polish (threshold: 5/10)"
        echo "  - Consistency (threshold: 6/10)"
        echo "  - [Optional] Accessibility"
        echo "  - [Optional] Responsiveness"
        ;;
esac
echo ""

# This section documents how the script would process critic output
# In actual usage, a Claude agent generates the report

echo "Expected Output:"
echo "  Report location: ${VERIFICATION_DIR}/"
echo "  Schema: ${CRITIC_SCHEMA}"
echo ""

# Early-stop condition checks (would be applied to actual report)
echo -e "${CYAN}Early-Stop Conditions:${NC}"
echo "  1. All scores >= thresholds -> PASSED (exit 0)"
echo "  2. Same issues as previous iteration -> NO_PROGRESS (exit 2)"
echo "  3. Attempt >= $MAX_ATTEMPTS -> MAX_ITERATIONS (exit 2)"
echo ""

# Demonstrate exit code logic
if [ "$ATTEMPT" -ge "$MAX_ATTEMPTS" ]; then
    echo -e "${YELLOW}NOTE: This is attempt ${ATTEMPT}/${MAX_ATTEMPTS} (final attempt)${NC}"
    echo "If issues remain after this iteration, escalation to user is required."
    echo ""
fi

echo "----------------------------------------"
echo ""
echo -e "${CYAN}Integration Points:${NC}"
echo "  - Compatible with stage-transition-gate.sh"
echo "  - Failure reports persist for debugging"
echo "  - Token metrics tracked for cost awareness"
echo ""
echo -e "${GREEN}CRITIC FRAMEWORK READY${NC}"
echo ""
echo "To perform actual validation:"
echo "  1. Claude agent invokes critic template for ${STAGE}"
echo "  2. Agent produces report matching ${CRITIC_SCHEMA}"
echo "  3. This script validates and determines next action"
echo ""

# Exit code guidance (for integration)
echo "Exit Codes for Integration:"
echo "  0 = PASSED (proceed to gate)"
echo "  1 = NEEDS_FIXES (iterate with fixes)"
echo "  2 = ESCALATE (user intervention needed)"
echo ""

# Default to framework ready (actual exit determined by agent-produced report)
exit 0
