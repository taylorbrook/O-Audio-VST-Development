#!/bin/bash
# validate-handoff.sh - Validates handoff document against schema + artifact existence
#
# Usage: validate-handoff.sh <handoff-file> [--force]
#
# Validates:
# 1. Handoff JSON against its $schema reference
# 2. All artifacts declared with exists=true actually exist on disk
#
# Exit codes:
#   0 - Validation passed
#   1 - Validation failed (schema or artifact)
#   2 - Usage error

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
SCHEMA_DIR="$PROJECT_ROOT/.planning/workflow/schemas"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

show_help() {
    echo "validate-handoff.sh - Validate handoff document against schema"
    echo ""
    echo "Usage: validate-handoff.sh <handoff-file> [--force]"
    echo ""
    echo "Arguments:"
    echo "  handoff-file    Path to HANDOFF.json file to validate"
    echo "  --force         Bypass validation failures with warning (use with caution)"
    echo ""
    echo "Validates:"
    echo "  - JSON syntax"
    echo "  - Schema conformance (via ajv-cli)"
    echo "  - Artifact existence (files declared with exists: true)"
    echo ""
    echo "Exit codes:"
    echo "  0 - Validation passed"
    echo "  1 - Validation failed"
    echo "  2 - Usage error"
}

# Parse arguments
FORCE_MODE=false
HANDOFF_FILE=""

for arg in "$@"; do
    case $arg in
        --force)
            FORCE_MODE=true
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            if [ -z "$HANDOFF_FILE" ]; then
                HANDOFF_FILE="$arg"
            fi
            ;;
    esac
done

# Validate arguments
if [ -z "$HANDOFF_FILE" ]; then
    echo -e "${RED}ERROR: No handoff file specified${NC}"
    echo ""
    show_help
    exit 2
fi

if [ ! -f "$HANDOFF_FILE" ]; then
    echo -e "${RED}ERROR: Handoff file not found: $HANDOFF_FILE${NC}"
    exit 1
fi

# Force mode warning
if [ "$FORCE_MODE" = true ]; then
    echo -e "${YELLOW}WARNING: --force bypasses validation. Handoff may be invalid.${NC}" >&2
fi

# Track validation status
VALIDATION_FAILED=false

# Step 1: Parse JSON and extract schema reference
echo "Validating: $HANDOFF_FILE"
echo "----------------------------------------"

# Check valid JSON
if ! jq empty "$HANDOFF_FILE" 2>/dev/null; then
    echo -e "${RED}ERROR: Invalid JSON in handoff file${NC}"
    if [ "$FORCE_MODE" = true ]; then
        echo -e "${YELLOW}--force: Continuing despite invalid JSON${NC}"
        VALIDATION_FAILED=true
    else
        exit 1
    fi
fi

# Extract $schema reference
SCHEMA_REF=$(jq -r '."$schema" // empty' "$HANDOFF_FILE")

if [ -z "$SCHEMA_REF" ]; then
    echo -e "${RED}ERROR: No \$schema reference found in handoff${NC}"
    if [ "$FORCE_MODE" = true ]; then
        echo -e "${YELLOW}--force: Continuing without schema validation${NC}"
        VALIDATION_FAILED=true
    else
        exit 1
    fi
else
    # Determine schema file path
    # Handle both relative paths (../../../.planning/workflow/schemas/handoff-X-to-Y.schema.json)
    # and bare filenames (handoff-X-to-Y.schema.json)
    SCHEMA_BASENAME=$(basename "$SCHEMA_REF")
    SCHEMA_FILE="$SCHEMA_DIR/$SCHEMA_BASENAME"

    echo "Schema: $SCHEMA_BASENAME"

    # Check schema file exists
    if [ ! -f "$SCHEMA_FILE" ]; then
        echo -e "${RED}ERROR: Schema file not found: $SCHEMA_FILE${NC}"
        if [ "$FORCE_MODE" = true ]; then
            echo -e "${YELLOW}--force: Continuing without schema validation${NC}"
            VALIDATION_FAILED=true
        else
            exit 1
        fi
    else
        # Step 2: Validate against schema using ajv-cli
        echo "Running schema validation..."

        # ajv-cli needs to resolve $ref to decision-entry.schema.json
        # Run from schema directory for proper resolution
        if ! (cd "$SCHEMA_DIR" && npx --yes ajv-cli validate \
            --spec=draft2020 \
            -s "$SCHEMA_BASENAME" \
            -d "$HANDOFF_FILE" \
            -r "decision-entry.schema.json" 2>&1); then
            echo -e "${RED}VALIDATION FAILED: Handoff does not match schema${NC}"
            if [ "$FORCE_MODE" = true ]; then
                echo -e "${YELLOW}--force: Continuing despite schema validation failure${NC}"
                VALIDATION_FAILED=true
            else
                exit 1
            fi
        else
            echo -e "${GREEN}Schema validation: PASSED${NC}"
        fi
    fi
fi

# Step 3: Verify artifact existence
echo ""
echo "Checking artifact existence..."

# Determine plugin directory (handoff is at plugins/NAME/.planning/stages/X-stage/HANDOFF.json)
# Go up from HANDOFF.json location to get plugin root
HANDOFF_DIR=$(dirname "$HANDOFF_FILE")
PLUGIN_DIR=$(cd "$HANDOFF_DIR/../.." && pwd)

# Extract artifacts where exists=true
MISSING_ARTIFACTS=()
ARTIFACT_COUNT=0

while IFS= read -r artifact_path; do
    if [ -n "$artifact_path" ]; then
        ((ARTIFACT_COUNT++))
        FULL_PATH="$PLUGIN_DIR/$artifact_path"
        if [ ! -e "$FULL_PATH" ]; then
            MISSING_ARTIFACTS+=("$artifact_path")
        fi
    fi
done < <(jq -r '.artifacts.required[]? | select(.exists == true) | .path' "$HANDOFF_FILE" 2>/dev/null)

if [ ${#MISSING_ARTIFACTS[@]} -gt 0 ]; then
    echo -e "${RED}ERROR: Missing artifacts (declared as exists: true):${NC}"
    for missing in "${MISSING_ARTIFACTS[@]}"; do
        echo "  - $missing"
    done
    if [ "$FORCE_MODE" = true ]; then
        echo -e "${YELLOW}--force: Continuing despite missing artifacts${NC}"
        VALIDATION_FAILED=true
    else
        echo ""
        echo -e "${RED}VALIDATION FAILED: Artifacts missing${NC}"
        exit 1
    fi
else
    if [ $ARTIFACT_COUNT -gt 0 ]; then
        echo -e "${GREEN}Artifacts: $ARTIFACT_COUNT required, all present${NC}"
    else
        echo "Artifacts: none declared as required"
    fi
fi

# Final result
echo ""
echo "========================================"
if [ "$VALIDATION_FAILED" = true ]; then
    if [ "$FORCE_MODE" = true ]; then
        echo -e "${YELLOW}VALIDATION BYPASSED (--force)${NC}"
        echo "Handoff may be invalid. Proceed with caution."
        exit 0
    else
        echo -e "${RED}VALIDATION FAILED${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}VALIDATION PASSED${NC}"
    exit 0
fi
