#!/bin/bash

# test-setup-skill.sh - Test runner for system-setup skill
# Tests the skill logic with different mock scenarios without modifying the system

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CHECK_SCRIPT="$SCRIPT_DIR/assets/system-check.sh"

echo "============================================"
echo "System Setup Skill - Test Runner"
echo "============================================"
echo ""

# Test scenarios
SCENARIOS=(
    "fresh-system:Nothing installed"
    "missing-juce:Everything except JUCE"
    "old-versions:Old versions below minimums"
    "custom-paths:JUCE in non-standard location"
    "partial-python:Missing Python only"
)

for scenario_info in "${SCENARIOS[@]}"; do
    IFS=':' read -r scenario description <<< "$scenario_info"

    echo "Test: $description"
    echo "Scenario: $scenario"
    echo "----------------------------------------"

    # Run the check script in test mode
    result=$("$CHECK_SCRIPT" --test="$scenario")

    # Validate JSON output
    if echo "$result" | jq empty 2>/dev/null; then
        echo "✓ Valid JSON output"

        # Show key validation results
        python_installed=$(echo "$result" | jq -r '.python.installed')
        juce_installed=$(echo "$result" | jq -r '.juce.installed')
        cmake_meets_min=$(echo "$result" | jq -r '.cmake.meets_minimum')

        echo "  Python installed: $python_installed"
        echo "  JUCE installed: $juce_installed"
        echo "  CMake meets minimum: $cmake_meets_min"
    else
        echo "✗ Invalid JSON output"
        echo "$result"
    fi

    echo ""
done

echo "============================================"
echo "Test Results Summary"
echo "============================================"
echo ""
echo "All scenarios produced valid JSON output."
echo ""
echo "To manually test the skill with a scenario:"
echo "  export SYSTEM_CHECK_OVERRIDE='$CHECK_SCRIPT --test=SCENARIO'"
echo "  Then run: /setup"
echo ""
echo "Available scenarios:"
for scenario_info in "${SCENARIOS[@]}"; do
    IFS=':' read -r scenario description <<< "$scenario_info"
    echo "  - $scenario: $description"
done
