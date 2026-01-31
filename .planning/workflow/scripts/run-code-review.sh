#!/bin/bash
#
# run-code-review.sh - Human code review checkpoint for stage boundaries
#
# Usage: run-code-review.sh <plugin> <stage> [--skip-review]
#
# Exit codes:
#   0 = APPROVED (proceed to next stage)
#   1 = CHANGES_REQUESTED or BLOCKED (issues to address)
#   2 = SKIPPED (bypassed with justification logged)
#

set -e

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Script location for finding template
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEMPLATE_PATH="${SCRIPT_DIR}/../templates/code-review-checklist.md"
PROJECT_ROOT="${SCRIPT_DIR}/../../.."

# Show help
show_help() {
    cat << EOF
${BOLD}Code Review Checkpoint${NC}

Usage: run-code-review.sh <plugin> <stage> [options]

Arguments:
  plugin        Plugin name (e.g., O-IntonationPad)
  stage         Stage number: 1, 2, 3, or 4

Options:
  --skip-review    Skip interactive review (prompts for justification)
  --help, -h       Show this help message

Exit Codes:
  0    APPROVED - proceed to next stage
  1    CHANGES_REQUESTED or BLOCKED - issues to address
  2    SKIPPED - review bypassed (justification logged)

Examples:
  run-code-review.sh O-IntonationPad 2
  run-code-review.sh MyPlugin 3 --skip-review

Stage Reference:
  Stage 1: Foundation (project setup, parameters)
  Stage 2: DSP (audio processing, real-time safety)
  Stage 3: GUI (interface, thread safety)
  Stage 4: Polish (validation, presets, docs)
EOF
    exit 0
}

# Validate arguments
validate_args() {
    if [[ -z "$PLUGIN" ]]; then
        echo -e "${RED}Error: Plugin name required${NC}" >&2
        echo "Usage: run-code-review.sh <plugin> <stage> [--skip-review]" >&2
        exit 1
    fi

    if [[ -z "$STAGE" ]]; then
        echo -e "${RED}Error: Stage number required${NC}" >&2
        echo "Usage: run-code-review.sh <plugin> <stage> [--skip-review]" >&2
        exit 1
    fi

    if ! [[ "$STAGE" =~ ^[1-4]$ ]]; then
        echo -e "${RED}Error: Stage must be 1, 2, 3, or 4${NC}" >&2
        exit 1
    fi

    if [[ ! -f "$TEMPLATE_PATH" ]]; then
        echo -e "${RED}Error: Template not found at $TEMPLATE_PATH${NC}" >&2
        exit 1
    fi

    PLUGIN_DIR="${PROJECT_ROOT}/plugins/${PLUGIN}"
    if [[ ! -d "$PLUGIN_DIR" ]]; then
        echo -e "${YELLOW}Warning: Plugin directory not found: $PLUGIN_DIR${NC}" >&2
        echo -e "${YELLOW}Continuing anyway (may be reviewing before plugin creation)${NC}" >&2
    fi
}

# Get stage name from number
get_stage_name() {
    case "$1" in
        1) echo "Foundation" ;;
        2) echo "DSP" ;;
        3) echo "GUI" ;;
        4) echo "Polish" ;;
        *) echo "Unknown" ;;
    esac
}

# Display the checklist section for a specific stage
display_stage_checklist() {
    local stage="$1"
    local stage_name=$(get_stage_name "$stage")

    echo -e "\n${BOLD}${CYAN}════════════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${CYAN}  CODE REVIEW: ${PLUGIN} - Stage ${stage} (${stage_name})${NC}"
    echo -e "${BOLD}${CYAN}════════════════════════════════════════════════════════════${NC}\n"

    # Display mandatory simplification pass
    echo -e "${BOLD}${YELLOW}MANDATORY SIMPLIFICATION PASS${NC}"
    echo -e "${YELLOW}───────────────────────────────${NC}"
    echo "Review these items regardless of stage:"
    echo "  [ ] Dead code removed (commented-out code, unused functions)"
    echo "  [ ] Duplicate logic consolidated (DRY principle applied)"
    echo "  [ ] Magic numbers extracted to named constants"
    echo "  [ ] Complex conditionals simplified (early returns, guard clauses)"
    echo "  [ ] Unused includes/imports removed"
    echo ""

    # Display stage-specific section
    echo -e "${BOLD}${BLUE}STAGE ${stage}: ${stage_name^^} REVIEW${NC}"
    echo -e "${BLUE}───────────────────────────────${NC}"

    case "$stage" in
        1)
            echo "  [ ] CMakeLists.txt follows project patterns (JUCE setup, targets)"
            echo "  [ ] APVTS parameter setup correct (types, ranges, defaults)"
            echo "  [ ] Plugin metadata accurate (name, manufacturer, format codes)"
            echo "  [ ] Basic project structure in place (Source/, .planning/)"
            echo "  [ ] Parameter IDs match ARCHITECTURE.md specification"
            ;;
        2)
            echo "  [ ] No memory allocations in processBlock (no new, malloc, vector resize)"
            echo "  [ ] ScopedNoDenormals present at start of processBlock"
            echo "  [ ] All parameters connected to DSP (no orphan parameters)"
            echo "  [ ] Buffer handling safe (zero-length check, sample loop bounds)"
            echo "  [ ] Signal flow matches ARCHITECTURE.md"
            echo "  [ ] Smoothing applied to parameter changes (avoid zipper noise)"
            echo "  [ ] Stereo/mono handling correct (respects channel count)"
            ;;
        3)
            echo "  [ ] Member declaration order correct (APVTS before components)"
            echo "  [ ] APVTS attachment patterns used correctly (attachments outlive components)"
            echo "  [ ] No audio thread access from GUI (no direct processor state access)"
            echo "  [ ] Responsive layout implemented (resizeable or fixed as designed)"
            echo "  [ ] Visual consistency with mockup (colors, spacing, typography)"
            echo "  [ ] All controls functional and labeled"
            echo "  [ ] Keyboard accessibility where appropriate"
            ;;
        4)
            echo "  [ ] pluginval strictness 10 passes (no crashes, no warnings)"
            echo "  [ ] Presets functional (save/load state works correctly)"
            echo "  [ ] Documentation complete (README with usage, parameter reference)"
            echo "  [ ] Build artifacts verified (VST3 + AU both load in DAW)"
            echo "  [ ] Final UX polish applied (tooltips, sensible defaults)"
            echo "  [ ] Performance acceptable (CPU usage reasonable for feature set)"
            echo "  [ ] No console warnings or errors in release build"
            ;;
    esac
    echo ""
}

# Handle skip-review mode
handle_skip_review() {
    local stage_name=$(get_stage_name "$STAGE")

    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}  SKIPPING CODE REVIEW${NC}"
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
    echo -e "${BOLD}Why are you skipping the Stage ${STAGE} (${stage_name}) review?${NC}"
    echo "Please provide a justification (required):"
    echo ""

    read -r -p "> " justification

    if [[ -z "$justification" ]]; then
        echo -e "${RED}Error: Justification is required to skip review${NC}" >&2
        exit 1
    fi

    # Log the bypass
    local bypass_log="${PROJECT_ROOT}/plugins/${PLUGIN}/.planning/gate-bypasses.log"
    mkdir -p "$(dirname "$bypass_log")"

    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    echo "[${timestamp}] REVIEW_SKIPPED stage=${STAGE} (${stage_name}): ${justification}" >> "$bypass_log"

    echo ""
    echo -e "${YELLOW}Review skip logged to: gate-bypasses.log${NC}"
    echo -e "${YELLOW}Justification: ${justification}${NC}"
    echo ""

    exit 2
}

# Prompt for verdict
prompt_verdict() {
    echo -e "${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${GREEN}  REVIEW VERDICT${NC}"
    echo -e "${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
    echo "Select your verdict:"
    echo ""
    echo -e "  ${GREEN}[A]${NC} APPROVED       - Code meets quality bar, proceed to next stage"
    echo -e "  ${YELLOW}[C]${NC} CHANGES        - Minor issues found, address before proceeding"
    echo -e "  ${RED}[B]${NC} BLOCKED        - Major issues require redesign"
    echo ""

    while true; do
        read -r -p "Enter verdict (A/C/B): " verdict
        case "${verdict^^}" in
            A|APPROVED)
                VERDICT="APPROVED"
                break
                ;;
            C|CHANGES)
                VERDICT="CHANGES_REQUESTED"
                break
                ;;
            B|BLOCKED)
                VERDICT="BLOCKED"
                break
                ;;
            *)
                echo -e "${RED}Invalid choice. Please enter A, C, or B.${NC}"
                ;;
        esac
    done
}

# Collect reviewer notes
collect_notes() {
    echo ""
    echo -e "${BOLD}Reviewer Notes${NC}"

    if [[ "$VERDICT" == "APPROVED" ]]; then
        echo "(Optional - press Enter to skip, or type notes and press Enter)"
    else
        echo "(Required for CHANGES_REQUESTED or BLOCKED verdicts)"
    fi
    echo ""

    local notes=""
    echo "Enter notes (multi-line: end with empty line):"
    while IFS= read -r line; do
        [[ -z "$line" ]] && break
        notes+="${line}"$'\n'
    done

    if [[ "$VERDICT" != "APPROVED" && -z "$notes" ]]; then
        echo -e "${RED}Error: Notes required for non-APPROVED verdicts${NC}" >&2
        exit 1
    fi

    NOTES="$notes"
}

# Save review to file
save_review() {
    local stage_name=$(get_stage_name "$STAGE")
    local review_dir="${PROJECT_ROOT}/plugins/${PLUGIN}/.planning/stages/${STAGE}-$(echo "$stage_name" | tr '[:upper:]' '[:lower:]')"
    mkdir -p "$review_dir"

    local review_file="${review_dir}/review-notes.md"
    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

    cat > "$review_file" << EOF
# Code Review: Stage ${STAGE} (${stage_name})

**Plugin:** ${PLUGIN}
**Stage:** ${STAGE} - ${stage_name}
**Date:** ${timestamp}
**Verdict:** ${VERDICT}

## Reviewer Notes

${NOTES:-_No notes provided_}

---
*Generated by run-code-review.sh*
EOF

    echo ""
    echo -e "${CYAN}Review saved to: ${review_file}${NC}"
}

# Display final result
display_result() {
    local stage_name=$(get_stage_name "$STAGE")

    echo ""
    echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    case "$VERDICT" in
        APPROVED)
            echo -e "${BOLD}${GREEN}  REVIEW COMPLETE: APPROVED${NC}"
            echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
            echo ""
            echo -e "${GREEN}Stage ${STAGE} (${stage_name}) review passed.${NC}"
            echo -e "${GREEN}Proceed to next stage.${NC}"
            exit 0
            ;;
        CHANGES_REQUESTED)
            echo -e "${BOLD}${YELLOW}  REVIEW COMPLETE: CHANGES REQUESTED${NC}"
            echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
            echo ""
            echo -e "${YELLOW}Stage ${STAGE} (${stage_name}) review found issues.${NC}"
            echo -e "${YELLOW}Address feedback before proceeding.${NC}"
            exit 1
            ;;
        BLOCKED)
            echo -e "${BOLD}${RED}  REVIEW COMPLETE: BLOCKED${NC}"
            echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
            echo ""
            echo -e "${RED}Stage ${STAGE} (${stage_name}) review blocked.${NC}"
            echo -e "${RED}Major issues require redesign or significant rework.${NC}"
            exit 1
            ;;
    esac
}

# Main function
main() {
    local SKIP_REVIEW=false
    PLUGIN=""
    STAGE=""
    VERDICT=""
    NOTES=""

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --help|-h)
                show_help
                ;;
            --skip-review)
                SKIP_REVIEW=true
                shift
                ;;
            *)
                if [[ -z "$PLUGIN" ]]; then
                    PLUGIN="$1"
                elif [[ -z "$STAGE" ]]; then
                    STAGE="$1"
                else
                    echo -e "${RED}Error: Unexpected argument: $1${NC}" >&2
                    exit 1
                fi
                shift
                ;;
        esac
    done

    # Validate
    validate_args

    # Handle skip mode
    if [[ "$SKIP_REVIEW" == true ]]; then
        handle_skip_review
    fi

    # Interactive review
    display_stage_checklist "$STAGE"
    prompt_verdict
    collect_notes
    save_review
    display_result
}

main "$@"
