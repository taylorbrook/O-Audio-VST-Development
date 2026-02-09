#!/bin/bash
# PreCompact - Preserve contracts before context compaction

# Preserve global state
if [ -f "PLUGINS.md" ]; then
  echo "=== PLUGINS.md (Global State) ==="
  cat "PLUGINS.md"
  echo ""
fi

# Find all plugins with contracts
PLUGINS=$(find plugins -type d -maxdepth 1 -mindepth 1 2>/dev/null)

for PLUGIN in $PLUGINS; do
  PLUGIN_NAME=$(basename "$PLUGIN")

  echo "=== Plugin: $PLUGIN_NAME ==="

  # Preserve all contract files
  if [ -f "$PLUGIN/.planning/creative-brief.md" ]; then
    echo "--- creative-brief.md ---"
    cat "$PLUGIN/.planning/creative-brief.md"
    echo ""
  fi

  if [ -f "$PLUGIN/.planning/parameter-spec.md" ]; then
    echo "--- parameter-spec.md ---"
    cat "$PLUGIN/.planning/parameter-spec.md"
    echo ""
  fi

  if [ -f "$PLUGIN/.planning/architecture.md" ]; then
    echo "--- architecture.md ---"
    cat "$PLUGIN/.planning/architecture.md"
    echo ""
  fi

  if [ -f "$PLUGIN/.planning/plan.md" ]; then
    echo "--- plan.md ---"
    cat "$PLUGIN/.planning/plan.md"
    echo ""
  fi

  # CRITICAL: Preserve workflow state
  if [ -f "$PLUGIN/.planning/STATUS.md" ]; then
    echo "--- STATUS.md (WORKFLOW STATE) ---"
    cat "$PLUGIN/.planning/STATUS.md"
    echo ""
  fi

  # List mockups if they exist
  if [ -d "$PLUGIN/.planning/mockups" ]; then
    echo "--- mockups/ ---"
    ls -lh "$PLUGIN/.planning/mockups"
    echo "Mockup files preserved in repository"
    echo ""
  fi
done

exit 0
