#!/bin/bash
# PostCompact context injection via SessionStart(compact)
# Stdout from this script IS injected into Claude's post-compaction context.
# Reads the snapshot file written by PreCompact.sh.

SNAPSHOT=".claude/compaction-snapshot.md"

if [ -f "$SNAPSHOT" ]; then
  cat "$SNAPSHOT"
  echo ""
  echo "---"
  echo "Context restored from pre-compaction snapshot."
else
  echo "No compaction snapshot available. Use /continue [PluginName] to load plugin context."
fi

exit 0
