#!/bin/bash
# Stop - Stage completion enforcement

# Check if workflow in progress
PLUGIN_STATUS=$(grep "🚧" PLUGINS.md | head -1)
if [ -z "$PLUGIN_STATUS" ]; then
  echo "No workflow in progress, skipping"
  exit 0
fi

# Extract plugin name and stage
PLUGIN_NAME=$(echo "$PLUGIN_STATUS" | grep -oP '### \K\w+')
CURRENT_STAGE=$(echo "$PLUGIN_STATUS" | grep -oP 'Stage \K\d+')

# Verify stage committed
LAST_COMMIT=$(git log -1 --format="%s")
if [[ ! "$LAST_COMMIT" =~ "Stage $CURRENT_STAGE" ]]; then
  echo "⚠️ Warning: Stage $CURRENT_STAGE not committed"
  echo "Expected commit for Stage $CURRENT_STAGE, found: $LAST_COMMIT"
  exit 1  # Block if stage not properly committed
fi

echo "Stage $CURRENT_STAGE properly committed"
exit 0
