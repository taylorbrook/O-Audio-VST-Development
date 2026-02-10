#!/bin/bash
# task-validator-dispatch.sh
# Called by TaskCompleted hook. Routes task content to relevant domain validators
# based on keyword matching.
#
# Input: JSON on stdin with fields: task_id, task_subject, task_description
# Exit 0: Task completes normally (all validations passed or non-code task)
# Exit 2: Task blocked with feedback (one or more validators failed)
#
# Only code-touching tasks get validated. Docs/config tasks skip validation.

set -euo pipefail

# Read task content from stdin
INPUT=$(cat)
TASK_SUBJECT=$(echo "$INPUT" | jq -r '.task_subject // empty' 2>/dev/null || echo "")
TASK_DESCRIPTION=$(echo "$INPUT" | jq -r '.task_description // empty' 2>/dev/null || echo "")
TASK_CONTENT="$TASK_SUBJECT $TASK_DESCRIPTION"

# Skip non-code tasks: only validate if task mentions code-related content
CODE_PATTERNS='\.cpp|\.h|\.cmake|\.html|\.js|\.css|processBlock|prepareToPlay|parameter|APVTS|WebView|relay|CMakeLists'
if ! echo "$TASK_CONTENT" | grep -qEi "$CODE_PATTERNS"; then
    exit 0  # Not a code task, skip validation
fi

# Detect active plugin from task content or environment variable
PLUGIN_NAME="${ACTIVE_PLUGIN:-}"
if [ -z "$PLUGIN_NAME" ]; then
    # Try to extract O-PluginName from task content
    PLUGIN_NAME=$(echo "$TASK_CONTENT" | grep -oE 'O-[A-Za-z]+' | head -1 || true)
fi

if [ -z "$PLUGIN_NAME" ]; then
    exit 0  # Cannot determine plugin, skip validation
fi

PLUGIN_PATH="plugins/$PLUGIN_NAME"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VALIDATORS_DIR="$SCRIPT_DIR/validators"
ERRORS=""

# --- Validator dispatch based on keyword matching ---

# DSP safety check: processBlock, prepareToPlay, DSP, audio processing, dsp component
if echo "$TASK_CONTENT" | grep -qEi 'processBlock|prepareToPlay|DSP|audio.processing|dsp.component'; then
    if [ -f "$VALIDATORS_DIR/validate-dsp-components.py" ]; then
        RESULT=$(cd "$CLAUDE_PROJECT_DIR" && python3 "$VALIDATORS_DIR/validate-dsp-components.py" 2>&1) || {
            ERRORS="${ERRORS}[DSP Components] ${RESULT}\n"
        }
    fi
    if [ -f "$VALIDATORS_DIR/validate-silent-failures.py" ]; then
        RESULT=$(cd "$CLAUDE_PROJECT_DIR" && python3 "$VALIDATORS_DIR/validate-silent-failures.py" 2>&1) || {
            ERRORS="${ERRORS}[Silent Failures] ${RESULT}\n"
        }
    fi
fi

# Parameter matching check: parameter, APVTS, createParameterLayout, parameter-spec
if echo "$TASK_CONTENT" | grep -qEi 'parameter|APVTS|createParameterLayout|parameter.spec'; then
    if [ -f "$VALIDATORS_DIR/validate-parameters.py" ]; then
        RESULT=$(cd "$CLAUDE_PROJECT_DIR" && python3 "$VALIDATORS_DIR/validate-parameters.py" 2>&1) || {
            ERRORS="${ERRORS}[Parameters] ${RESULT}\n"
        }
    fi
fi

# WebView binding check: WebView, relay, index.html, binding, GUI, editor
if echo "$TASK_CONTENT" | grep -qEi 'WebView|relay|index\.html|binding|GUI|editor'; then
    if [ -f "$VALIDATORS_DIR/validate-gui-bindings.py" ]; then
        RESULT=$(cd "$CLAUDE_PROJECT_DIR" && python3 "$VALIDATORS_DIR/validate-gui-bindings.py" 2>&1) || {
            ERRORS="${ERRORS}[GUI Bindings] ${RESULT}\n"
        }
    fi
fi

# Cross-contract check: contract, ARCHITECTURE, ROADMAP, BRIEF, cross-contract
if echo "$TASK_CONTENT" | grep -qEi 'contract|ARCHITECTURE|ROADMAP|BRIEF|cross.contract'; then
    if [ -f "$VALIDATORS_DIR/validate-cross-contract.py" ]; then
        RESULT=$(cd "$CLAUDE_PROJECT_DIR" && python3 "$VALIDATORS_DIR/validate-cross-contract.py" 2>&1) || {
            ERRORS="${ERRORS}[Cross-Contract] ${RESULT}\n"
        }
    fi
fi

# Checksum verification: checksum, STATUS.md, SHA, integrity
if echo "$TASK_CONTENT" | grep -qEi 'checksum|STATUS\.md|SHA|integrity'; then
    if [ -f "$VALIDATORS_DIR/validate-checksums.py" ]; then
        RESULT=$(cd "$CLAUDE_PROJECT_DIR" && python3 "$VALIDATORS_DIR/validate-checksums.py" 2>&1) || {
            ERRORS="${ERRORS}[Checksums] ${RESULT}\n"
        }
    fi
fi

# Resource accountability: research, resource, frontmatter
if echo "$TASK_CONTENT" | grep -qEi 'research|resource|frontmatter'; then
    if [ -f "$VALIDATORS_DIR/validate-resource-accountability.py" ]; then
        RESULT=$(cd "$CLAUDE_PROJECT_DIR" && python3 "$VALIDATORS_DIR/validate-resource-accountability.py" 2>&1) || {
            ERRORS="${ERRORS}[Resource Accountability] ${RESULT}\n"
        }
    fi
fi

# --- Report results ---

if [ -n "$ERRORS" ]; then
    echo "Task validation failed for $PLUGIN_NAME:" >&2
    echo -e "$ERRORS" >&2
    echo "" >&2
    echo "Fix the issues above and retry the task." >&2
    exit 2  # Block task completion with feedback
fi

exit 0  # All validations passed
