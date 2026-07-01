#!/usr/bin/env python3
"""SubagentStart hook: Inject persistent memory for target agents."""

import json
import os
import sys
from pathlib import Path


def main():
    """Read agent_type from stdin JSON and inject matching memory file as additionalContext."""
    try:
        project_root = Path(__file__).resolve().parent.parent.parent

        # Read hook input from stdin
        raw_input = sys.stdin.read()
        if not raw_input.strip():
            sys.exit(0)

        try:
            input_data = json.loads(raw_input)
        except json.JSONDecodeError:
            sys.exit(0)

        # Extract agent_type from input JSON
        agent_type = input_data.get("agent_type", "")
        if not agent_type:
            sys.exit(0)

        # Construct memory file path
        memory_file = project_root / ".claude" / "agent-memory" / f"{agent_type}.md"

        # Env-gated debug echo (stderr ONLY — stdout is reserved for the
        # hookSpecificOutput JSON contract; any stdout pollution corrupts the
        # additionalContext payload).
        if os.environ.get("CLAUDE_HOOK_DEBUG"):
            print(
                f"[SubagentStart] inject-agent-memory: agent_type={agent_type!r} "
                f"memory_file_found={memory_file.is_file()}",
                file=sys.stderr,
            )

        # If memory file doesn't exist, exit silently
        if not memory_file.is_file():
            sys.exit(0)

        # Read memory file content
        content = memory_file.read_text(encoding="utf-8")

        # If content is empty, exit silently
        if not content.strip():
            sys.exit(0)

        # Output JSON response with additionalContext
        output = {
            "hookSpecificOutput": {
                "hookEventName": "SubagentStart",
                "additionalContext": content,
            }
        }
        print(json.dumps(output, ensure_ascii=False))

    except Exception:
        pass

    sys.exit(0)


if __name__ == "__main__":
    main()
