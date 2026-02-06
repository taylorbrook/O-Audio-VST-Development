#!/usr/bin/env bash
# PostToolUse hook: Auto-regenerate resource-index.json when research files are written/edited.
#
# Trigger: write|edit operations on research/*.md files
# Behavior: Runs generate-resource-index.py in the background.
# Safety: Always exits 0 — never blocks agent workflow. All errors swallowed.
#
# Registered in hooks.json under PostToolUse with matcher "write|edit".

# Always exit 0 regardless of what happens
trap 'exit 0' EXIT ERR

# Determine project root (hooks dir is .claude/hooks/)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
GENERATOR="$PROJECT_ROOT/.claude/scripts/generate-resource-index.py"

# Check if the written/edited file is a research document
# FILE_PATH is set by the hook environment
FILE_PATH="${FILE_PATH:-}"

if [ -z "$FILE_PATH" ]; then
    exit 0
fi

# Only trigger for research/*.md files
case "$FILE_PATH" in
    */research/*.md|research/*.md)
        ;;
    *)
        exit 0
        ;;
esac

# Skip README.md (not a research document)
BASENAME="$(basename "$FILE_PATH")"
if [ "$BASENAME" = "README.md" ]; then
    exit 0
fi

# Verify generator script exists
if [ ! -f "$GENERATOR" ]; then
    exit 0
fi

# Run manifest regeneration (stdout/stderr swallowed)
python3 "$GENERATOR" >/dev/null 2>&1 || true

exit 0
