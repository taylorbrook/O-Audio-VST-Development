#!/usr/bin/env python3
"""Stop hook - Stage completion enforcement."""

import re
import subprocess
import sys
from pathlib import Path


def main():
    """Verify that the current workflow stage has been properly committed."""
    project_root = Path(__file__).resolve().parent.parent.parent
    plugins_md_path = project_root / "PLUGINS.md"

    # Check if workflow in progress
    if not plugins_md_path.is_file():
        print("No PLUGINS.md found, skipping")
        sys.exit(0)

    try:
        plugins_text = plugins_md_path.read_text(encoding="utf-8")
    except OSError:
        print("Cannot read PLUGINS.md, skipping")
        sys.exit(0)

    # Find first line with construction emoji
    plugin_status = ""
    for line in plugins_text.splitlines():
        if "🚧" in line:
            plugin_status = line
            break

    if not plugin_status:
        print("No workflow in progress, skipping")
        sys.exit(0)

    # Extract plugin name and stage
    name_match = re.search(r"### (\w+)", plugin_status)
    plugin_name = name_match.group(1) if name_match else "unknown"

    stage_match = re.search(r"Stage (\d+)", plugin_status)
    if not stage_match:
        print(f"No stage number found for {plugin_name}, skipping")
        sys.exit(0)

    current_stage = stage_match.group(1)

    # Verify stage committed
    try:
        result = subprocess.run(
            ["git", "log", "-1", "--format=%s"],
            cwd=str(project_root),
            capture_output=True,
            text=True,
        )
        last_commit = result.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        print("Cannot read git log, skipping")
        sys.exit(0)

    if f"Stage {current_stage}" not in last_commit:
        print(f"Warning: Stage {current_stage} not committed")
        print(
            f"Expected commit for Stage {current_stage}, found: {last_commit}"
        )
        sys.exit(1)

    print(f"Stage {current_stage} properly committed")
    sys.exit(0)


if __name__ == "__main__":
    main()
