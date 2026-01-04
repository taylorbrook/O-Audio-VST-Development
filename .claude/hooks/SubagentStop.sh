#!/bin/bash
# SubagentStop hook - Deterministic validation after each subagent completes
# Layer 1 validation: Fast pattern matching (1-3s), blocks workflow on failure

INPUT=$(cat)
SUBAGENT=$(echo "$INPUT" | jq -r '.subagent_name // empty' 2>/dev/null)

# Gracefully skip if we can't extract subagent name
if [ -z "$SUBAGENT" ]; then
  echo "Hook not relevant: no subagent_name in input, skipping gracefully"
  exit 0
fi

# Check relevance FIRST - only validate our implementation subagents
if [[ ! "$SUBAGENT" =~ ^(foundation-shell-agent|dsp-agent|gui-agent)$ ]]; then
  echo "Hook not relevant for subagent: $SUBAGENT, skipping gracefully"
  exit 0
fi

# Extract plugin name from context if available
PLUGIN_NAME=$(echo "$INPUT" | jq -r '.plugin_name // empty' 2>/dev/null)

# Layer 0: Contract integrity validation (MANDATORY for all stages)
# Run before stage-specific validation
if [ -n "$PLUGIN_NAME" ]; then
  PLUGIN_PATH="plugins/$PLUGIN_NAME"
  export PLUGIN_PATH

  echo "Validating contract integrity for $PLUGIN_NAME..." >&2

  # 1. Verify contract checksums (Stages 1-4 only)
  python3 .claude/hooks/validators/validate-checksums.py "$PLUGIN_PATH"
  CHECKSUM_RESULT=$?
  if [ $CHECKSUM_RESULT -eq 1 ]; then
    echo "❌ Contract checksum validation FAILED" >&2
    exit 2  # Block workflow
  elif [ $CHECKSUM_RESULT -eq 2 ]; then
    echo "⚠️  Contract checksum validation: warnings detected" >&2
    # Continue but warn
  fi

  # 2. Validate cross-contract consistency (all stages)
  python3 .claude/hooks/validators/validate-cross-contract.py "$PLUGIN_PATH"
  CROSS_RESULT=$?
  if [ $CROSS_RESULT -eq 1 ]; then
    echo "❌ Cross-contract validation FAILED" >&2
    exit 2  # Block workflow
  elif [ $CROSS_RESULT -eq 2 ]; then
    echo "⚠️  Cross-contract validation: warnings detected" >&2
    # Continue but warn
  fi

  echo "✓ Contract integrity validated" >&2
fi

# Execute hook logic based on subagent
case "$SUBAGENT" in
  foundation-shell-agent)
    echo "Validating foundation-shell-agent output (Stage 1)..."

    # Validate foundation (CMakeLists.txt, source files, build)
    python3 .claude/hooks/validators/validate-foundation.py
    FOUNDATION_RESULT=$?
    if [ $FOUNDATION_RESULT -ne 0 ]; then
      echo "Foundation validation FAILED: CMakeLists.txt missing or build failed" >&2
      exit 2  # Block workflow
    fi

    # Validate parameters (APVTS matches parameter-spec.md)
    python3 .claude/hooks/validators/validate-parameters.py
    PARAMS_RESULT=$?
    if [ $PARAMS_RESULT -ne 0 ]; then
      echo "Parameter validation FAILED: Parameters from spec missing in code" >&2
      exit 2  # Block workflow
    fi

    echo "Foundation-shell validation PASSED"
    ;;

  dsp-agent)
    echo "Validating dsp-agent output (Stage 2)..."
    python3 .claude/hooks/validators/validate-dsp-components.py
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
      echo "DSP component validation FAILED: Components from architecture missing" >&2
      exit 2  # Block workflow
    fi
    echo "DSP component validation PASSED"
    ;;

  gui-agent)
    echo "Validating gui-agent output (Stage 3)..."
    python3 .claude/hooks/validators/validate-gui-bindings.py
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
      echo "GUI binding validation FAILED: HTML IDs don't match relay IDs" >&2
      exit 2  # Block workflow
    fi
    echo "GUI binding validation PASSED"
    ;;
esac

exit 0
