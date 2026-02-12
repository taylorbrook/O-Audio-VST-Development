#!/usr/bin/env python3
"""PostCompact context injection via SessionStart(compact)."""

import sys
from pathlib import Path


def main():
    """Read and output the compaction snapshot for post-compaction context restoration."""
    try:
        project_root = Path(__file__).resolve().parent.parent.parent
        snapshot_path = project_root / ".claude" / "compaction-snapshot.md"

        if snapshot_path.is_file():
            content = snapshot_path.read_text(encoding="utf-8")
            print(content)
            print("")
            print("---")
            print("Context restored from pre-compaction snapshot.")
        else:
            print(
                "No compaction snapshot available. "
                "Use /continue [PluginName] to load plugin context."
            )

    except Exception:
        print(
            "No compaction snapshot available. "
            "Use /continue [PluginName] to load plugin context."
        )

    sys.exit(0)


if __name__ == "__main__":
    main()
