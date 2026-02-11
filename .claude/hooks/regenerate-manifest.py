#!/usr/bin/env python3
"""PostToolUse hook: Auto-regenerate resource-index.json when research files are written/edited."""

import os
import subprocess
import sys
from pathlib import Path


def main():
    """Run generate-resource-index.py when a research markdown file is written or edited."""
    try:
        project_root = Path(__file__).resolve().parent.parent.parent
        generator = project_root / ".claude" / "scripts" / "generate-resource-index.py"

        # Check if the written/edited file is a research document
        file_path = os.environ.get("FILE_PATH", "")
        if not file_path:
            sys.exit(0)

        file_path_obj = Path(file_path)

        # Only trigger for research/*.md files (any parent named "research")
        parts = file_path_obj.parts
        is_research = False
        for i, part in enumerate(parts):
            if part == "research" and i < len(parts) - 1:
                # Check if the file itself is an .md file
                if file_path_obj.suffix.lower() == ".md":
                    is_research = True
                    break

        if not is_research:
            sys.exit(0)

        # Skip README.md (not a research document)
        if file_path_obj.name == "README.md":
            sys.exit(0)

        # Verify generator script exists
        if not generator.is_file():
            sys.exit(0)

        # Run manifest regeneration (stdout/stderr swallowed)
        subprocess.run(
            [sys.executable, str(generator)],
            cwd=str(project_root),
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    except Exception:
        pass

    sys.exit(0)


if __name__ == "__main__":
    main()
