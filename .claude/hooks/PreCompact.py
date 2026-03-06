#!/usr/bin/env python3
"""PreCompact - Write domain-aware snapshot before context compaction."""

import json
import re
import sys
from datetime import datetime
from pathlib import Path


def main():
    """Write a compaction snapshot capturing active plugin context."""
    try:
        project_root = Path(__file__).resolve().parent.parent.parent
        snapshot_path = project_root / ".claude" / "compaction-snapshot.md"

        lines = []
        lines.append("# Active Context Snapshot")
        lines.append(f"Generated: {datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')}")
        lines.append("")

        # Find focused plugin from PLUGINS.md
        focused = ""
        in_progress = ""
        plugins_md_path = project_root / "PLUGINS.md"
        if plugins_md_path.is_file():
            try:
                plugins_text = plugins_md_path.read_text(encoding="utf-8")
                for line in plugins_text.splitlines():
                    if "\u0001f6a7" in line or "🚧" in line:
                        match = re.search(r"O-[A-Za-z]+", line)
                        if match:
                            focused = match.group(0)
                            break
            except OSError:
                pass

        # Always gather in-progress list
        if plugins_md_path.is_file():
            try:
                plugins_text = plugins_md_path.read_text(encoding="utf-8")
                progress_lines = []
                for line in plugins_text.splitlines():
                    if "🚧" in line:
                        progress_lines.append(line)
                        if len(progress_lines) >= 5:
                            break
                in_progress = "\n".join(progress_lines)
            except OSError:
                pass

        if focused:
            lines.append(f"## Active Plugin: {focused}")
            lines.append("")

            plugin_dir = project_root / "plugins" / focused / ".planning"

            # Load DIGEST.json if exists (most token-efficient)
            digest_path = plugin_dir / "DIGEST.json"
            if digest_path.is_file():
                try:
                    digest_content = digest_path.read_text(encoding="utf-8")
                    lines.append("### Context Digest")
                    lines.append("```json")
                    lines.append(digest_content)
                    lines.append("```")
                    lines.append("")
                except OSError:
                    pass

            # Extract STATUS.md frontmatter (stage, phase, workflow_mode)
            status_path = plugin_dir / "STATUS.md"
            if status_path.is_file():
                try:
                    status_content = status_path.read_text(encoding="utf-8")
                    # Extract YAML frontmatter between --- delimiters
                    frontmatter_match = re.search(
                        r"^(---\s*\n.*?\n---)", status_content, re.DOTALL
                    )
                    if frontmatter_match:
                        lines.append("### Current State")
                        lines.append(frontmatter_match.group(1))
                        lines.append("")
                except OSError:
                    pass

            # Extract parameter IDs from parameter-spec.md (table rows, first 20)
            param_spec_path = plugin_dir / "parameter-spec.md"
            if param_spec_path.is_file():
                try:
                    param_content = param_spec_path.read_text(encoding="utf-8")
                    table_rows = [
                        line
                        for line in param_content.splitlines()
                        if re.match(r"^\|.*\|.*\|", line)
                    ]
                    if table_rows:
                        lines.append("### Parameter IDs")
                        for row in table_rows[:20]:
                            lines.append(row)
                        lines.append("")
                except OSError:
                    pass

            # List contract paths that exist
            lines.append("### Contract Paths")
            contract_files = [
                "BRIEF.md",
                "parameter-spec.md",
                Path("research") / "ARCHITECTURE.md",
                "ROADMAP.md",
            ]
            for f in contract_files:
                contract_path = plugin_dir / f
                if contract_path.is_file():
                    # Use forward-slash relative path for display
                    relative = plugin_dir.relative_to(project_root) / f
                    lines.append(f"- {relative.as_posix()}")
            lines.append("")

        if in_progress:
            lines.append("## In-Progress Plugins")
            lines.append(in_progress)
            lines.append("")

        # Write snapshot file
        snapshot_path.parent.mkdir(parents=True, exist_ok=True)
        snapshot_path.write_text("\n".join(lines), encoding="utf-8")

    except Exception:
        pass

    sys.exit(0)


if __name__ == "__main__":
    main()
