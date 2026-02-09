#!/bin/bash
# PreCompact - Write domain-aware snapshot before context compaction
# This script's stdout is NOT injected into post-compaction context.
# Instead, it writes a snapshot file that SessionStart(compact) reads.
# Reference: 15-RESEARCH.md Pattern 1, Stage 1

SNAPSHOT=".claude/compaction-snapshot.md"

{
  echo "# Active Context Snapshot"
  echo "Generated: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo ""

  # Detect focused plugin from registry
  FOCUSED=""
  if [ -f ".claude/plugin-registry.json" ]; then
    FOCUSED=$(python3 -c "
import json, sys
try:
    with open('.claude/plugin-registry.json') as f:
        reg = json.load(f)
    print(reg.get('focused', ''))
except:
    pass
" 2>/dev/null)
  fi

  # Find in-progress plugins from PLUGINS.md as fallback
  IN_PROGRESS=""
  if [ -z "$FOCUSED" ] && [ -f "PLUGINS.md" ]; then
    FOCUSED=$(grep -E '🚧' PLUGINS.md 2>/dev/null | head -1 | grep -oE 'O-[A-Za-z]+' | head -1)
  fi

  # Always gather in-progress list
  if [ -f "PLUGINS.md" ]; then
    IN_PROGRESS=$(grep -E '🚧' PLUGINS.md 2>/dev/null | head -5)
  fi

  if [ -n "$FOCUSED" ]; then
    echo "## Active Plugin: $FOCUSED"
    echo ""

    PLUGIN_DIR="plugins/$FOCUSED/.planning"

    # Load DIGEST.json if exists (most token-efficient)
    if [ -f "$PLUGIN_DIR/DIGEST.json" ]; then
      echo "### Context Digest"
      echo '```json'
      cat "$PLUGIN_DIR/DIGEST.json" 2>/dev/null
      echo '```'
      echo ""
    fi

    # Extract STATUS.md frontmatter (stage, phase, workflow_mode)
    if [ -f "$PLUGIN_DIR/STATUS.md" ]; then
      echo "### Current State"
      sed -n '/^---$/,/^---$/p' "$PLUGIN_DIR/STATUS.md" 2>/dev/null
      echo ""
    fi

    # Extract parameter IDs from parameter-spec.md (table rows, first 20)
    if [ -f "$PLUGIN_DIR/parameter-spec.md" ]; then
      echo "### Parameter IDs"
      grep -E '^\|.*\|.*\|' "$PLUGIN_DIR/parameter-spec.md" 2>/dev/null | head -20
      echo ""
    fi

    # List contract paths that exist
    echo "### Contract Paths"
    for f in BRIEF.md parameter-spec.md research/ARCHITECTURE.md ROADMAP.md; do
      [ -f "$PLUGIN_DIR/$f" ] && echo "- $PLUGIN_DIR/$f"
    done
    echo ""
  fi

  if [ -n "$IN_PROGRESS" ]; then
    echo "## In-Progress Plugins"
    echo "$IN_PROGRESS"
    echo ""
  fi

} > "$SNAPSHOT"

exit 0
