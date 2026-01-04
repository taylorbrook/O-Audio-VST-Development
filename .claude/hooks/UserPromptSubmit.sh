#!/bin/bash
# UserPromptSubmit - Auto-inject context for /continue

# Check relevance FIRST
if [[ ! "$USER_PROMPT" =~ ^/continue ]]; then
  echo "Hook not relevant (not /continue command), skipping gracefully"
  exit 0
fi

# Extract plugin name
PLUGIN_NAME=$(echo "$USER_PROMPT" | awk '{print $2}')

# Find handoff file
if [ -n "$PLUGIN_NAME" ]; then
  HANDOFF="plugins/$PLUGIN_NAME/.continue-here.md"
else
  HANDOFF=$(find plugins -name ".continue-here.md" -type f -printf '%T@ %p\n' 2>/dev/null | sort -rn | head -1 | cut -d' ' -f2)
fi

if [ ! -f "$HANDOFF" ]; then
  echo "No handoff file found"
  exit 0
fi

# Inject handoff content into context
echo "Loading context from $HANDOFF..."
cat "$HANDOFF"

# Load referenced contracts
PLUGIN=$(dirname "$HANDOFF" | xargs basename)
echo ""
echo "--- Contracts ---"
[ -f "plugins/$PLUGIN/.ideas/parameter-spec.md" ] && cat "plugins/$PLUGIN/.ideas/parameter-spec.md"
[ -f "plugins/$PLUGIN/.ideas/architecture.md" ] && cat "plugins/$PLUGIN/.ideas/architecture.md"

exit 0
