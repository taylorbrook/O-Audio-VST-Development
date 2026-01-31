#!/bin/bash
# run-gate.sh - Unified quality gate for stage transitions
#
# Usage: run-gate.sh <plugin> <from-stage> <to-stage> [--force] [--skip-review]
#
# Runs all verification checks for a stage transition:
# - Critical: schema validation, build, pluginval, domain critics (stage-dependent)
# - Advisory: code style, naming conventions, documentation (placeholder)
# - Review: human code review checkpoint (end of gate)
#
# Exit codes:
#   0 - GATE PASSED
#   1 - GATE BLOCKED (critical check failed)
#   2 - GATE BYPASSED (--force used)
#   3 - Usage error

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Configuration
CHECK_TIMEOUT=60        # seconds per check
RETRY_DELAY=2           # initial delay, doubles each retry
MAX_RETRIES=3

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

show_help() {
    echo "run-gate.sh - Unified quality gate for stage transitions"
    echo ""
    echo "Usage: run-gate.sh <plugin> <from-stage> <to-stage> [options]"
    echo ""
    echo "Arguments:"
    echo "  plugin       Plugin name (e.g., O-IntonationPad)"
    echo "  from-stage   Stage transitioning from (e.g., 1-foundation)"
    echo "  to-stage     Stage transitioning to (e.g., 2-dsp)"
    echo ""
    echo "Options:"
    echo "  --force       Bypass failed checks with justification (logged)"
    echo "  --skip-review Skip human code review checkpoint"
    echo "  --help, -h    Show this help message"
    echo ""
    echo "Critical Checks (must pass):"
    echo "  - Schema validation (handoff document)"
    echo "  - Build (ninja Plugin_VST3 Plugin_AU)"
    echo "  - Pluginval (strictness level 10)"
    echo "  - DSP critic (stage 2+)"
    echo "  - UI critic (stage 3+)"
    echo ""
    echo "Advisory Checks (report only):"
    echo "  - Style, naming, documentation (placeholders)"
    echo ""
    echo "Exit Codes:"
    echo "  0 - PASSED (all critical checks passed)"
    echo "  1 - BLOCKED (critical check failed)"
    echo "  2 - BYPASSED (--force used with justification)"
    echo "  3 - Usage error"
    echo ""
    echo "Stage-Dependent Critics:"
    echo "  0-ideation -> 1-foundation: No critics"
    echo "  1-foundation -> 2-dsp: No critics"
    echo "  2-dsp -> 3-gui: DSP critic is critical"
    echo "  3-gui -> 4-polish: DSP + UI critics are critical"
}

# Parse arguments
PLUGIN=""
FROM_STAGE=""
TO_STAGE=""
FORCE_MODE=false
SKIP_REVIEW=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --force)
            FORCE_MODE=true
            shift
            ;;
        --skip-review)
            SKIP_REVIEW=true
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        -*)
            echo -e "${RED}ERROR: Unknown option: $1${NC}"
            exit 3
            ;;
        *)
            if [ -z "$PLUGIN" ]; then
                PLUGIN="$1"
            elif [ -z "$FROM_STAGE" ]; then
                FROM_STAGE="$1"
            elif [ -z "$TO_STAGE" ]; then
                TO_STAGE="$1"
            fi
            shift
            ;;
    esac
done

# Validate arguments
if [ -z "$PLUGIN" ] || [ -z "$FROM_STAGE" ] || [ -z "$TO_STAGE" ]; then
    echo -e "${RED}ERROR: Required arguments: plugin, from-stage, to-stage${NC}"
    echo ""
    show_help
    exit 3
fi

# Extract stage numbers for gate naming
FROM_NUM=$(echo "$FROM_STAGE" | grep -o '^[0-9]' || echo "0")
TO_NUM=$(echo "$TO_STAGE" | grep -o '^[0-9]' || echo "0")
GATE_ID="gate-${FROM_NUM}-to-${TO_NUM}"

# Setup paths
PLUGIN_DIR="$PROJECT_ROOT/plugins/${PLUGIN}"
PLANNING_DIR="$PLUGIN_DIR/.planning"
STAGE_DIR="$PLANNING_DIR/stages/${FROM_STAGE}"
REPORT_FILE="$STAGE_DIR/gate-report.json"
BYPASS_LOG="$PLANNING_DIR/gate-bypasses.log"
HANDOFF_FILE="$STAGE_DIR/HANDOFF.json"
BUILD_DIR="$PROJECT_ROOT/build"
VST3_PATH="$BUILD_DIR/plugins/${PLUGIN}/${PLUGIN}_artefacts/Release/VST3/${PLUGIN}.vst3"

# Initialize timing
START_TIME=$(date +%s%3N 2>/dev/null || echo $(($(date +%s) * 1000)))

# Track results
declare -a CRITICAL_RESULTS=()
declare -a ADVISORY_RESULTS=()
CRITICAL_PASSED=true
FAILED_CHECKS=()

# Header
echo ""
echo -e "${BOLD}================================================================${NC}"
echo -e "${BOLD}                     QUALITY GATE${NC}"
echo -e "${BOLD}================================================================${NC}"
echo ""
echo -e "Plugin:      ${CYAN}${PLUGIN}${NC}"
echo -e "Transition:  ${FROM_STAGE} -> ${TO_STAGE}"
echo -e "Gate ID:     ${GATE_ID}"
echo ""

# Run a check with retry logic
# Returns 0 on success, 1 on failure
# Sets CHECK_DURATION, CHECK_ATTEMPTS, CHECK_OUTPUT, CHECK_ERROR
run_check() {
    local NAME=$1
    local CMD=$2
    local ATTEMPT=1
    local DELAY=$RETRY_DELAY

    CHECK_DURATION=0
    CHECK_ATTEMPTS=0
    CHECK_OUTPUT=""
    CHECK_ERROR=""

    while [ $ATTEMPT -le $MAX_RETRIES ]; do
        CHECK_ATTEMPTS=$ATTEMPT
        echo -ne "  [$NAME] Attempt $ATTEMPT/$MAX_RETRIES... "

        local CHECK_START=$(date +%s%3N 2>/dev/null || echo $(($(date +%s) * 1000)))
        local TMP_OUTPUT=$(mktemp)
        local TMP_ERROR=$(mktemp)

        if timeout $CHECK_TIMEOUT bash -c "$CMD" >"$TMP_OUTPUT" 2>"$TMP_ERROR"; then
            local CHECK_END=$(date +%s%3N 2>/dev/null || echo $(($(date +%s) * 1000)))
            CHECK_DURATION=$((CHECK_END - CHECK_START))
            CHECK_OUTPUT=$(cat "$TMP_OUTPUT" 2>/dev/null | head -c 10000 || true)
            rm -f "$TMP_OUTPUT" "$TMP_ERROR"
            echo -e "${GREEN}PASSED${NC} (${CHECK_DURATION}ms)"
            return 0
        fi

        local CHECK_END=$(date +%s%3N 2>/dev/null || echo $(($(date +%s) * 1000)))
        CHECK_DURATION=$((CHECK_END - CHECK_START))
        CHECK_ERROR=$(cat "$TMP_ERROR" 2>/dev/null | head -c 10000 || true)
        CHECK_OUTPUT=$(cat "$TMP_OUTPUT" 2>/dev/null | head -c 10000 || true)
        rm -f "$TMP_OUTPUT" "$TMP_ERROR"

        # Automatic retry on first failure (handles transient issues)
        if [ $ATTEMPT -eq 1 ]; then
            echo -e "${YELLOW}RETRY${NC} (transient failure, waiting ${DELAY}s)"
            sleep $DELAY
            DELAY=$((DELAY * 2))  # Exponential backoff: 2s, 4s, 8s
            ATTEMPT=$((ATTEMPT + 1))
            continue
        fi

        echo -e "${RED}FAILED${NC} (${CHECK_DURATION}ms)"
        ATTEMPT=$((ATTEMPT + 1))
    done

    return 1
}

# Add result to critical array (JSON format for report)
add_critical_result() {
    local NAME=$1
    local STATUS=$2
    local DURATION=$3
    local ATTEMPTS=$4
    local OUTPUT="${5:-}"
    local ERROR="${6:-}"

    # Escape JSON strings
    OUTPUT=$(echo "$OUTPUT" | jq -Rs '.')
    ERROR=$(echo "$ERROR" | jq -Rs '.')

    local RESULT="{\"name\":\"$NAME\",\"status\":\"$STATUS\",\"duration\":$DURATION,\"attempts\":$ATTEMPTS"
    [ "$OUTPUT" != '""' ] && RESULT="$RESULT,\"output\":$OUTPUT"
    [ "$ERROR" != '""' ] && RESULT="$RESULT,\"error\":$ERROR"
    RESULT="$RESULT}"

    CRITICAL_RESULTS+=("$RESULT")
}

# Add result to advisory array
add_advisory_result() {
    local NAME=$1
    local STATUS=$2
    local DURATION=$3
    local ATTEMPTS=$4
    local OUTPUT="${5:-}"
    local ERROR="${6:-}"

    OUTPUT=$(echo "$OUTPUT" | jq -Rs '.')
    ERROR=$(echo "$ERROR" | jq -Rs '.')

    local RESULT="{\"name\":\"$NAME\",\"status\":\"$STATUS\",\"duration\":$DURATION,\"attempts\":$ATTEMPTS"
    [ "$OUTPUT" != '""' ] && RESULT="$RESULT,\"output\":$OUTPUT"
    [ "$ERROR" != '""' ] && RESULT="$RESULT,\"error\":$ERROR"
    RESULT="$RESULT}"

    ADVISORY_RESULTS+=("$RESULT")
}

# Ensure stage directory exists
mkdir -p "$STAGE_DIR"

echo -e "${BOLD}CRITICAL CHECKS${NC}"
echo "----------------------------------------------------------------"

# Critical Check 1: Schema validation (invoke validate-handoff.sh)
if [ -f "$HANDOFF_FILE" ]; then
    if run_check "schema" "$SCRIPT_DIR/validate-handoff.sh '$HANDOFF_FILE'"; then
        add_critical_result "schema" "passed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT"
    else
        CRITICAL_PASSED=false
        FAILED_CHECKS+=("schema")
        add_critical_result "schema" "failed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT" "$CHECK_ERROR"
    fi
else
    echo -e "  [schema] ${YELLOW}SKIPPED${NC} (no handoff at $HANDOFF_FILE)"
    add_critical_result "schema" "skipped" "0" "1" "" "No handoff file found"
fi

# Critical Check 2: Build (ninja Plugin_VST3 Plugin_AU)
if [ -d "$BUILD_DIR" ]; then
    if run_check "build" "cd '$BUILD_DIR' && ninja ${PLUGIN}_VST3 ${PLUGIN}_AU"; then
        add_critical_result "build" "passed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT"
    else
        CRITICAL_PASSED=false
        FAILED_CHECKS+=("build")
        add_critical_result "build" "failed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT" "$CHECK_ERROR"
    fi
else
    echo -e "  [build] ${YELLOW}SKIPPED${NC} (no build directory)"
    add_critical_result "build" "skipped" "0" "1" "" "Build directory not found"
fi

# Critical Check 3: Pluginval (strictness 10 on VST3)
if command -v pluginval &> /dev/null && [ -d "$VST3_PATH" ]; then
    if run_check "pluginval" "pluginval --strictness-level 10 --validate '$VST3_PATH'"; then
        add_critical_result "pluginval" "passed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT"
    else
        CRITICAL_PASSED=false
        FAILED_CHECKS+=("pluginval")
        add_critical_result "pluginval" "failed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT" "$CHECK_ERROR"
    fi
elif [ ! -d "$VST3_PATH" ]; then
    echo -e "  [pluginval] ${YELLOW}SKIPPED${NC} (VST3 not built)"
    add_critical_result "pluginval" "skipped" "0" "1" "" "VST3 plugin not found at $VST3_PATH"
else
    echo -e "  [pluginval] ${YELLOW}SKIPPED${NC} (pluginval not installed)"
    add_critical_result "pluginval" "skipped" "0" "1" "" "pluginval command not found"
fi

# Critical Check 4: DSP critic (critical at stage 2+, invoke run-critic.sh)
# Stage-dependent: DSP critic is critical when transitioning TO 3-gui or 4-polish
case "$TO_STAGE" in
    3-gui|4-polish)
        if run_check "dsp-critic" "$SCRIPT_DIR/run-critic.sh '$PLUGIN' 2-dsp"; then
            add_critical_result "dsp-critic" "passed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT"
        else
            CRITICAL_PASSED=false
            FAILED_CHECKS+=("dsp-critic")
            add_critical_result "dsp-critic" "failed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT" "$CHECK_ERROR"
        fi
        ;;
    *)
        echo -e "  [dsp-critic] ${CYAN}N/A${NC} (not required for $FROM_STAGE -> $TO_STAGE)"
        ;;
esac

# Critical Check 5: UI critic (critical at stage 3+, invoke run-critic.sh)
# Stage-dependent: UI critic is critical when transitioning TO 4-polish
case "$TO_STAGE" in
    4-polish)
        if run_check "ui-critic" "$SCRIPT_DIR/run-critic.sh '$PLUGIN' 3-gui"; then
            add_critical_result "ui-critic" "passed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT"
        else
            CRITICAL_PASSED=false
            FAILED_CHECKS+=("ui-critic")
            add_critical_result "ui-critic" "failed" "$CHECK_DURATION" "$CHECK_ATTEMPTS" "$CHECK_OUTPUT" "$CHECK_ERROR"
        fi
        ;;
    *)
        echo -e "  [ui-critic] ${CYAN}N/A${NC} (not required for $FROM_STAGE -> $TO_STAGE)"
        ;;
esac

echo ""
echo -e "${BOLD}ADVISORY CHECKS${NC}"
echo "----------------------------------------------------------------"

# Advisory checks are placeholders - report only, never block
# These will be implemented in future work

echo -ne "  [style] "
# Placeholder for code style check
add_advisory_result "style" "skipped" "0" "1" "" "Placeholder - future work"
echo -e "${CYAN}SKIPPED${NC} (placeholder)"

echo -ne "  [naming] "
# Placeholder for naming conventions check
add_advisory_result "naming" "skipped" "0" "1" "" "Placeholder - future work"
echo -e "${CYAN}SKIPPED${NC} (placeholder)"

echo -ne "  [docs] "
# Placeholder for documentation check
add_advisory_result "docs" "skipped" "0" "1" "" "Placeholder - future work"
echo -e "${CYAN}SKIPPED${NC} (placeholder)"

echo ""

# Calculate total duration
END_TIME=$(date +%s%3N 2>/dev/null || echo $(($(date +%s) * 1000)))
TOTAL_DURATION=$((END_TIME - START_TIME))

echo "----------------------------------------------------------------"
echo -e "Total duration: ${TOTAL_DURATION}ms"
echo ""

# Build JSON report
build_report() {
    local STATUS=$1
    local BYPASS_USED=${2:-false}
    local BYPASS_JUSTIFICATION="${3:-}"
    local REVIEW_STATUS="${4:-}"
    local REVIEW_NOTES="${5:-}"
    local REVIEW_SKIPPED=${6:-false}
    local SKIP_JUSTIFICATION="${7:-}"

    # Build critical checks array
    local CRITICAL_JSON="["
    local FIRST=true
    for result in "${CRITICAL_RESULTS[@]}"; do
        [ "$FIRST" = true ] || CRITICAL_JSON="$CRITICAL_JSON,"
        CRITICAL_JSON="$CRITICAL_JSON$result"
        FIRST=false
    done
    CRITICAL_JSON="$CRITICAL_JSON]"

    # Build advisory checks array
    local ADVISORY_JSON="["
    FIRST=true
    for result in "${ADVISORY_RESULTS[@]}"; do
        [ "$FIRST" = true ] || ADVISORY_JSON="$ADVISORY_JSON,"
        ADVISORY_JSON="$ADVISORY_JSON$result"
        FIRST=false
    done
    ADVISORY_JSON="$ADVISORY_JSON]"

    # Build failed checks array for bypass
    local FAILED_JSON="["
    FIRST=true
    for check in "${FAILED_CHECKS[@]}"; do
        [ "$FIRST" = true ] || FAILED_JSON="$FAILED_JSON,"
        FAILED_JSON="$FAILED_JSON\"$check\""
        FIRST=false
    done
    FAILED_JSON="$FAILED_JSON]"

    # Construct full report
    cat > "$REPORT_FILE" <<EOF
{
  "gate": "$GATE_ID",
  "plugin": "$PLUGIN",
  "fromStage": "$FROM_STAGE",
  "toStage": "$TO_STAGE",
  "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "checks": {
    "critical": $CRITICAL_JSON,
    "advisory": $ADVISORY_JSON
  },
  "overallStatus": "$STATUS",
  "totalDuration": $TOTAL_DURATION
EOF

    # Add bypass info if used
    if [ "$BYPASS_USED" = true ]; then
        cat >> "$REPORT_FILE" <<EOF
,
  "bypass": {
    "used": true,
    "justification": $(echo "$BYPASS_JUSTIFICATION" | jq -Rs '.'),
    "failedChecks": $FAILED_JSON
  }
EOF
    fi

    # Add review info if present
    if [ -n "$REVIEW_STATUS" ]; then
        cat >> "$REPORT_FILE" <<EOF
,
  "review": {
    "status": "$REVIEW_STATUS",
    "notes": $(echo "$REVIEW_NOTES" | jq -Rs '.'),
    "skipped": $REVIEW_SKIPPED
EOF
        if [ "$REVIEW_SKIPPED" = true ] && [ -n "$SKIP_JUSTIFICATION" ]; then
            echo ",    \"skipJustification\": $(echo "$SKIP_JUSTIFICATION" | jq -Rs '.')" >> "$REPORT_FILE"
        fi
        echo "  }" >> "$REPORT_FILE"
    fi

    echo "}" >> "$REPORT_FILE"
}

# Gate decision
if [ "$CRITICAL_PASSED" = true ]; then
    # Code review checkpoint
    REVIEW_STATUS=""
    REVIEW_NOTES=""
    REVIEW_SKIPPED=false
    SKIP_JUSTIFICATION=""

    if [ "$SKIP_REVIEW" = true ]; then
        echo -e "${YELLOW}Code review skipped (--skip-review)${NC}"
        read -p "Justification for skipping review: " SKIP_JUSTIFICATION
        if [ -z "$SKIP_JUSTIFICATION" ]; then
            SKIP_JUSTIFICATION="No justification provided"
        fi
        REVIEW_STATUS="skipped"
        REVIEW_SKIPPED=true
    else
        echo -e "${CYAN}Code Review Checkpoint${NC}"
        echo ""
        echo "Review focus for $TO_STAGE:"
        case "$TO_STAGE" in
            1-foundation)
                echo "  - CMakeLists.txt follows project patterns"
                echo "  - APVTS parameter setup correct"
                echo "  - Plugin metadata accurate"
                ;;
            2-dsp)
                echo "  - No allocations in processBlock"
                echo "  - ScopedNoDenormals present"
                echo "  - All parameters affect DSP"
                echo "  - Buffer handling safe"
                ;;
            3-gui)
                echo "  - Member declaration order correct"
                echo "  - APVTS attachment patterns used"
                echo "  - No audio thread access from GUI"
                echo "  - Responsive layout implemented"
                ;;
            4-polish)
                echo "  - pluginval strictness 10 passes"
                echo "  - Presets functional"
                echo "  - Documentation complete"
                echo "  - Build artifacts verified"
                ;;
        esac
        echo ""
        echo "Mandatory simplification pass:"
        echo "  [ ] Dead code removed"
        echo "  [ ] Duplicate logic consolidated"
        echo "  [ ] Magic numbers extracted"
        echo "  [ ] Complex conditionals simplified"
        echo ""
        read -p "Review complete? [y/N]: " REVIEW_RESPONSE

        if [ "$REVIEW_RESPONSE" = "y" ] || [ "$REVIEW_RESPONSE" = "Y" ]; then
            read -p "Review notes (optional): " REVIEW_NOTES
            REVIEW_STATUS="approved"
        else
            echo -e "${YELLOW}Review pending - gate waiting${NC}"
            build_report "BLOCKED" false "" "" "" false ""
            exit 1
        fi
    fi

    # Generate report and exit success
    build_report "PASSED" false "" "$REVIEW_STATUS" "$REVIEW_NOTES" "$REVIEW_SKIPPED" "$SKIP_JUSTIFICATION"

    echo ""
    echo -e "${GREEN}================================================================${NC}"
    echo -e "${GREEN}                     GATE PASSED${NC}"
    echo -e "${GREEN}================================================================${NC}"
    echo ""
    echo "Report: $REPORT_FILE"
    echo "Next: Start $TO_STAGE execution"
    exit 0
else
    # Critical checks failed
    if [ "$FORCE_MODE" = true ]; then
        echo -e "${YELLOW}Gate blocked, but --force requested${NC}"
        echo "Failed checks: ${FAILED_CHECKS[*]}"
        echo ""
        read -p "Justification (required): " JUSTIFICATION

        if [ -z "$JUSTIFICATION" ]; then
            echo -e "${RED}ERROR: Justification required for --force bypass${NC}"
            build_report "BLOCKED"
            exit 1
        fi

        # Log bypass to audit file
        mkdir -p "$(dirname "$BYPASS_LOG")"
        cat >> "$BYPASS_LOG" <<EOF

## Bypass: $FROM_STAGE -> $TO_STAGE
**Date:** $(date -u +"%Y-%m-%dT%H:%M:%SZ")
**Failed checks:** ${FAILED_CHECKS[*]}
**Justification:** $JUSTIFICATION
**User:** $(whoami)
---
EOF

        # Generate report with bypass info
        build_report "BYPASSED" true "$JUSTIFICATION"

        echo ""
        echo -e "${YELLOW}================================================================${NC}"
        echo -e "${YELLOW}                     GATE BYPASSED${NC}"
        echo -e "${YELLOW}================================================================${NC}"
        echo ""
        echo "Bypass logged to: $BYPASS_LOG"
        echo "Report: $REPORT_FILE"
        echo ""
        echo "WARNING: Quality gate was bypassed. Address failed checks soon."
        exit 2
    else
        # Generate blocked report
        build_report "BLOCKED"

        echo -e "${RED}================================================================${NC}"
        echo -e "${RED}                     GATE BLOCKED${NC}"
        echo -e "${RED}================================================================${NC}"
        echo ""
        echo "Failed checks: ${FAILED_CHECKS[*]}"
        echo ""
        echo "Options:"
        echo "  1. Fix issues and retry"
        echo "  2. Use --force to bypass (requires justification)"
        echo ""
        echo "Report: $REPORT_FILE"
        exit 1
    fi
fi
