#!/usr/bin/env python3
"""UserPromptSubmit - Auto-inject context for /continue."""

import os
import sys
from pathlib import Path


def find_most_recent_status():
    """Find the most recently modified STATUS.md across all plugins."""
    plugins_dir = Path("plugins")
    if not plugins_dir.is_dir():
        return None

    status_files = list(plugins_dir.glob("*/.planning/STATUS.md"))
    if not status_files:
        return None

    # Sort by modification time, most recent first
    status_files.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    return status_files[0]


def extract_plugin_name_from_status(status_path):
    """Extract plugin name from STATUS.md path using parent navigation.

    Replaces: dirname | xargs dirname | xargs basename
    STATUS.md -> .planning -> plugin_name
    """
    return status_path.parent.parent.name


def main():
    # Check relevance FIRST
    user_prompt = os.environ.get("USER_PROMPT", "")
    if not user_prompt.startswith("/continue"):
        print("Hook not relevant (not /continue command), skipping gracefully")
        sys.exit(0)

    # Extract plugin name from second word of USER_PROMPT
    parts = user_prompt.split()
    plugin_name = parts[1] if len(parts) > 1 else ""

    # Find handoff file
    handoff = None
    if plugin_name:
        candidate = Path("plugins") / plugin_name / ".planning" / "STATUS.md"
        if candidate.is_file():
            handoff = candidate
    else:
        handoff = find_most_recent_status()

    if handoff is None or not handoff.is_file():
        print("No handoff file found")
        sys.exit(0)

    # Inject handoff content into context
    print(f"Loading context from {handoff}...")
    try:
        print(handoff.read_text(encoding="utf-8"))
    except OSError as exc:
        print(f"Error reading {handoff}: {exc}", file=sys.stderr)
        sys.exit(0)

    # Load referenced contracts
    plugin = extract_plugin_name_from_status(handoff)
    plugin_planning = Path("plugins") / plugin / ".planning"

    print()
    print("--- Contracts ---")

    param_spec = plugin_planning / "parameter-spec.md"
    if param_spec.is_file():
        try:
            print(param_spec.read_text(encoding="utf-8"))
        except OSError:
            pass

    architecture = plugin_planning / "architecture.md"
    if architecture.is_file():
        try:
            print(architecture.read_text(encoding="utf-8"))
        except OSError:
            pass

    sys.exit(0)


if __name__ == "__main__":
    main()
