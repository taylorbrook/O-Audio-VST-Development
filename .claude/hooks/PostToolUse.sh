#!/bin/bash
# PostToolUse hook - Real-time code quality validation for JUCE best practices
# Layer 1 validation: Fast pattern matching (<2s), blocks on real-time safety violations
# Layer 2 validation: Silent failure detection (runs after file modifications)

INPUT=$(cat)
TOOL_NAME=$(echo "$INPUT" | jq -r '.tool_name // empty' 2>/dev/null)

# Check relevance FIRST - only validate on Write/Edit to plugin source files
if [[ ! "$TOOL_NAME" =~ ^(Write|Edit)$ ]]; then
  exit 0
fi

FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // empty' 2>/dev/null)

# Layer 0: Contract immutability enforcement (CRITICAL)
# Block modifications to contract files during Stages 1-4
if [[ "$FILE_PATH" =~ plugins/([^/]+)/.ideas/(creative-brief|parameter-spec|architecture|plan)\.md$ ]]; then
  # Extract plugin name from regex match
  PLUGIN_NAME="${BASH_REMATCH[1]}"
  PLUGIN_PATH="plugins/$PLUGIN_NAME"

  # Check if .continue-here.md exists
  if [ -f "$PLUGIN_PATH/.continue-here.md" ]; then
    # Extract current stage
    STAGE=$(grep -E '^stage:' "$PLUGIN_PATH/.continue-here.md" | head -1 | sed 's/stage: *//')

    # Block if in implementation stages (1-4)
    if [[ "$STAGE" =~ ^[1-4]$ ]]; then
      CONTRACT_FILE=$(basename "$FILE_PATH")
      echo "" >&2
      echo "❌ CONTRACT IMMUTABILITY VIOLATION" >&2
      echo "" >&2
      echo "Cannot modify $CONTRACT_FILE during Stage $STAGE (implementation)." >&2
      echo "" >&2
      echo "Contracts are immutable during Stages 1-4 to prevent drift." >&2
      echo "All implementation stages reference the same contract specifications." >&2
      echo "" >&2
      echo "If you need to modify contracts:" >&2
      echo "  1. Return to Stage 0-1 (planning)" >&2
      echo "  2. Update contracts" >&2
      echo "  3. Restart implementation from Stage 1" >&2
      echo "" >&2
      echo "Alternatively, use /improve after Stage 4 to make changes with versioning." >&2
      exit 1
    fi
  fi
fi

# Determine validation level based on file type
PLUGIN_SOURCE=false
PLUGIN_FILE=false

if [[ "$FILE_PATH" =~ plugins/.*/Source/.*\.(cpp|h)$ ]]; then
  PLUGIN_SOURCE=true
  PLUGIN_FILE=true
elif [[ "$FILE_PATH" =~ plugins/.*/(CMakeLists\.txt|ui/.*\.(html|js))$ ]]; then
  PLUGIN_FILE=true
fi

# Skip if not a plugin file
if [ "$PLUGIN_FILE" = false ]; then
  exit 0
fi

# Extract file content based on tool type
if [[ "$TOOL_NAME" == "Write" ]]; then
  FILE_CONTENT=$(echo "$INPUT" | jq -r '.tool_input.content // empty' 2>/dev/null)
elif [[ "$TOOL_NAME" == "Edit" ]]; then
  FILE_CONTENT=$(echo "$INPUT" | jq -r '.tool_input.new_string // empty' 2>/dev/null)
fi

if [ -z "$FILE_CONTENT" ]; then
  exit 0
fi

# Layer 1: Real-time safety checks (processBlock only, for C++ source files)
if [ "$PLUGIN_SOURCE" = true ]; then
  # Extract processBlock function for real-time safety checks
  PROCESS_BLOCK=$(echo "$FILE_CONTENT" | awk '/void.*processBlock.*AudioBuffer/{flag=1} flag{print} /^}$/{if(flag) exit}')

  if [ -n "$PROCESS_BLOCK" ]; then

# Real-time safety violation checks
ERRORS=""

# Check for heap allocation
if echo "$PROCESS_BLOCK" | grep -qE '\bnew\s+|delete\s+|malloc|free\b'; then
  ERRORS="${ERRORS}\nERROR: Heap allocation detected in processBlock (new/delete/malloc/free)"
fi

# Check for blocking locks
if echo "$PROCESS_BLOCK" | grep -qE 'std::mutex|\.lock\(\)|\.try_lock\(\)|std::lock_guard|std::unique_lock'; then
  ERRORS="${ERRORS}\nERROR: Blocking mutex/lock detected in processBlock"
fi

# Check for I/O operations
if echo "$PROCESS_BLOCK" | grep -qE 'File::|URL::|FileOutputStream|FileInputStream'; then
  ERRORS="${ERRORS}\nERROR: File I/O operations detected in processBlock"
fi

# Check for console output
if echo "$PROCESS_BLOCK" | grep -qE 'std::cout|std::cerr|printf|fprintf'; then
  ERRORS="${ERRORS}\nERROR: Console output detected in processBlock (std::cout/printf)"
fi

# Best practice warnings (advisory only)
WARNINGS=""

# Check for ScopedNoDenormals
if ! echo "$PROCESS_BLOCK" | grep -q 'ScopedNoDenormals'; then
  WARNINGS="${WARNINGS}\nWARNING: Missing ScopedNoDenormals in processBlock"
fi

    # Report results
    if [ -n "$ERRORS" ]; then
      echo "Real-time safety violations detected in $FILE_PATH:" >&2
      echo -e "$ERRORS" >&2
      echo "" >&2
      echo "These violations can cause audio dropouts, glitches, or crashes." >&2
      echo "processBlock must be real-time safe (no allocations, locks, or I/O)." >&2
      exit 1  # Block workflow on ERROR
    fi

    if [ -n "$WARNINGS" ]; then
      echo "Code quality recommendations for $FILE_PATH:" >&2
      echo -e "$WARNINGS" >&2
      # Don't block on warnings, just inform
    fi
  fi
fi

# Layer 2: Silent failure pattern detection (all plugin files)
# Run comprehensive scan on Write/Edit to catch known silent failures
echo "Running silent failure pattern detection..." >&2
python3 .claude/hooks/validators/validate-silent-failures.py
RESULT=$?

if [ $RESULT -eq 1 ]; then
  # Critical patterns detected - workflow blocked
  echo "" >&2
  echo "❌ PostToolUse validation FAILED: Silent failure patterns detected" >&2
  echo "Fix these issues before proceeding (they compile but fail at runtime)" >&2
  exit 1
elif [ $RESULT -eq 2 ]; then
  # Warnings detected - inform but don't block
  echo "" >&2
  echo "⚠️  PostToolUse validation: Warnings detected (see above)" >&2
  echo "Consider addressing these issues for better code quality" >&2
fi

exit 0
