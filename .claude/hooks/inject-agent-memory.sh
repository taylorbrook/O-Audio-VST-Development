#!/usr/bin/env bash
# SubagentStart hook: Inject persistent memory for target agents
# Reads agent_type from stdin JSON, loads matching memory file as additionalContext

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Read hook input from stdin
INPUT=$(cat)

# Extract agent_type from input JSON
AGENT_TYPE=$(echo "$INPUT" | jq -r '.agent_type // empty')

# If no agent_type, exit silently
if [ -z "$AGENT_TYPE" ]; then
    exit 0
fi

# Construct memory file path
MEMORY_FILE="$PROJECT_DIR/.claude/agent-memory/${AGENT_TYPE}.md"

# If memory file doesn't exist, exit silently (agent starts without memory)
if [ ! -f "$MEMORY_FILE" ]; then
    exit 0
fi

# Read memory file content
CONTENT=$(cat "$MEMORY_FILE")

# If content is empty, exit silently
if [ -z "$CONTENT" ]; then
    exit 0
fi

# Output JSON response with additionalContext
jq -n --arg ctx "$CONTENT" '{
    "hookSpecificOutput": {
        "hookEventName": "SubagentStart",
        "additionalContext": $ctx
    }
}'
