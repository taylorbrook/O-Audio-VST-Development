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
  if [ -f "$PLUGIN/.ideas/creative-brief.md" ]; then
    echo "--- creative-brief.md ---"
    cat "$PLUGIN/.ideas/creative-brief.md"
    echo ""
  fi

  if [ -f "$PLUGIN/.ideas/parameter-spec.md" ]; then
    echo "--- parameter-spec.md ---"
    cat "$PLUGIN/.ideas/parameter-spec.md"
    echo ""
  fi

  if [ -f "$PLUGIN/.ideas/architecture.md" ]; then
    echo "--- architecture.md ---"
    cat "$PLUGIN/.ideas/architecture.md"
    echo ""
  fi

  if [ -f "$PLUGIN/.ideas/plan.md" ]; then
    echo "--- plan.md ---"
    cat "$PLUGIN/.ideas/plan.md"
    echo ""
  fi

  # CRITICAL: Preserve workflow state
  if [ -f "$PLUGIN/.continue-here.md" ]; then
    echo "--- .continue-here.md (WORKFLOW STATE) ---"
    cat "$PLUGIN/.continue-here.md"
    echo ""
  fi

  # List mockups if they exist
  if [ -d "$PLUGIN/.ideas/mockups" ]; then
    echo "--- mockups/ ---"
    ls -lh "$PLUGIN/.ideas/mockups"
    echo "Mockup files preserved in repository"
    echo ""
  fi
done

exit 0
