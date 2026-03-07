#!/usr/bin/env python3
"""SubagentStop hook: Write back learnings to agent memory files.

After a subagent completes, this hook extracts error patterns, fixes, and key
learnings from the session data and appends them to the per-agent memory file.
This closes the learning loop so agents get smarter over time.

Safety: NEVER exits non-zero. All exceptions caught. 10KB file cap.
"""

import json
import re
import sys
from datetime import date
from pathlib import Path

MAX_MEMORY_SIZE = 10_240  # 10KB cap
MAX_LEARNING_LENGTH = 200

# Patterns that indicate an error occurred
ERROR_PATTERNS = re.compile(
    r"(?:ERROR|FAILED|error|fatal|Error|Failed):\s*(.+)",
    re.IGNORECASE,
)

# Patterns that indicate a resolution or learning
RESOLUTION_PATTERNS = re.compile(
    r"(?:The fix (?:is|was)|The solution (?:is|was)|The issue was|Root cause:|"
    r"Fixed by|Resolved by|Workaround:|The problem was|Solution:)\s*(.+)",
    re.IGNORECASE,
)


def is_duplicate(learning: str, existing_content: str) -> bool:
    """Check if a learning already exists in memory via key-phrase matching.

    Extracts significant phrases (3+ word sequences, technical terms) from the
    learning and checks if any appear in the existing content.
    """
    if not learning or not existing_content:
        return False

    # Normalize for comparison
    learning_lower = learning.lower().strip()
    existing_lower = existing_content.lower()

    # Direct substring check on the core message (skip the prefix like plugin name)
    # Extract the main content after any colon prefix
    core = learning_lower.split(": ", 1)[-1] if ": " in learning_lower else learning_lower

    # Check if the core phrase (or a substantial portion) already exists
    if core in existing_lower:
        return True

    # Extract technical terms (CamelCase, snake_case, or terms with special chars)
    tech_terms = re.findall(
        r"[A-Z][a-z]+[A-Z]\w+|[a-z]+_[a-z_]+|\w+\.\w+\(\)|@\w+", learning
    )
    # If any technical term appears in existing content, likely duplicate
    for term in tech_terms:
        if term.lower() in existing_lower:
            return True

    return False


def extract_learnings_from_results(tool_results, existing_content: str) -> list[str]:
    """Extract error-fix patterns from tool_results.

    Looks for sequences of failed tool calls followed by successful retries,
    and investigation patterns (reading JUCE source, then applying a fix).
    """
    learnings = []

    if not tool_results:
        return learnings

    # Handle tool_results as list of dicts or as a single dict
    if isinstance(tool_results, dict):
        tool_results = [tool_results]

    if not isinstance(tool_results, list):
        return learnings

    # Track error-then-fix sequences
    last_error = None

    for entry in tool_results:
        if not isinstance(entry, dict):
            continue

        tool_name = entry.get("tool", entry.get("name", ""))
        result_text = str(entry.get("result", entry.get("output", "")))
        exit_code = entry.get("exit_code", entry.get("exitCode", 0))

        # Detect errors in Bash tool calls
        if tool_name in ("Bash", "bash") and exit_code != 0:
            # Extract the error message
            error_match = ERROR_PATTERNS.search(result_text)
            if error_match:
                last_error = error_match.group(1).strip()[:100]
            elif result_text.strip():
                last_error = result_text.strip().split("\n")[0][:100]

        # Detect successful retry after error
        elif tool_name in ("Bash", "bash") and exit_code == 0 and last_error:
            # A successful run after an error suggests a fix was found
            learning = f"Build fix: {last_error} -- resolved"
            if len(learning) > MAX_LEARNING_LENGTH:
                learning = learning[:MAX_LEARNING_LENGTH]
            if not is_duplicate(learning, existing_content):
                learnings.append(learning)
            last_error = None

        # Detect investigation of source files (Read/Grep on JUCE source)
        elif tool_name in ("Read", "Grep", "read", "grep"):
            target = str(entry.get("path", entry.get("file_path", "")))
            if "JUCE" in target or "juce" in target:
                # Note: the actual learning from source investigation is hard to
                # extract without the full conversation context. Log the investigation.
                pass

    return learnings


def extract_learnings_from_transcript(
    transcript_path: str, existing_content: str
) -> list[str]:
    """Extract learnings from agent transcript file.

    Reads the transcript and looks for error-fix patterns, resolution statements,
    and workarounds.
    """
    learnings = []

    if not transcript_path:
        return learnings

    transcript_file = Path(transcript_path)
    if not transcript_file.is_file():
        return learnings

    try:
        # Read with size guard (don't load huge transcripts)
        content = transcript_file.read_text(encoding="utf-8", errors="replace")
        if len(content) > 500_000:
            # Only scan the last 500KB for recent learnings
            content = content[-500_000:]
    except (OSError, PermissionError):
        return learnings

    lines = content.split("\n")

    # Scan for resolution patterns
    for i, line in enumerate(lines):
        match = RESOLUTION_PATTERNS.search(line)
        if match:
            resolution = match.group(1).strip()
            if len(resolution) < 20:
                # Too short to be useful, skip
                continue

            learning = resolution
            if len(learning) > MAX_LEARNING_LENGTH:
                learning = learning[:MAX_LEARNING_LENGTH]

            if not is_duplicate(learning, existing_content):
                learnings.append(learning)

    # Scan for error-then-success build patterns
    error_lines = []
    for i, line in enumerate(lines):
        if ERROR_PATTERNS.search(line):
            error_lines.append((i, line.strip()[:100]))

    # For each error, check if a success follows within 50 lines
    for err_idx, err_text in error_lines:
        success_window = lines[err_idx : err_idx + 50]
        success_found = any(
            re.search(r"(?:BUILD SUCCESS|PASSED|Succeeded|✓|SYNTAX OK)", l)
            for l in success_window
        )
        if success_found:
            learning = f"Error resolved: {err_text}"
            if len(learning) > MAX_LEARNING_LENGTH:
                learning = learning[:MAX_LEARNING_LENGTH]
            if not is_duplicate(learning, existing_content):
                learnings.append(learning)

    return learnings


def append_to_memory(memory_path: Path, new_learnings: list[str]) -> bool:
    """Append learnings to the Learned Patterns section of the memory file.

    Inserts new entries before the ## Common Issues section (or at end of
    ## Learned Patterns if Common Issues doesn't exist).
    Updates the ## Last Updated line.

    Returns True if file was written, False if skipped.
    """
    content = memory_path.read_text(encoding="utf-8")

    # Build the new entries
    entries = "\n".join(f"- {learning}" for learning in new_learnings)

    # Find insertion point: before ## Common Issues
    common_issues_match = re.search(r"\n(## Common Issues)", content)
    if common_issues_match:
        insert_pos = common_issues_match.start()
        content = content[:insert_pos] + "\n" + entries + "\n" + content[insert_pos:]
    else:
        # No Common Issues section -- append after ## Learned Patterns content
        learned_match = re.search(r"(## Learned Patterns\n)", content)
        if learned_match:
            # Find the end of the learned patterns section (next ## or end)
            section_end = re.search(
                r"\n##\s", content[learned_match.end() :]
            )
            if section_end:
                insert_pos = learned_match.end() + section_end.start()
                content = (
                    content[:insert_pos] + "\n" + entries + "\n" + content[insert_pos:]
                )
            else:
                # No next section, append at end
                content = content.rstrip() + "\n" + entries + "\n"
        else:
            # No Learned Patterns section at all, append at end
            content = content.rstrip() + "\n\n## Learned Patterns\n" + entries + "\n"

    # Update Last Updated
    today = date.today().isoformat()
    last_updated_pattern = re.compile(r"(## Last Updated\n).*", re.DOTALL)
    if last_updated_pattern.search(content):
        content = last_updated_pattern.sub(
            rf"\g<1>{today} (auto write-back)", content
        )
    else:
        content = content.rstrip() + f"\n\n## Last Updated\n{today} (auto write-back)\n"

    # Check size cap before writing
    if len(content.encode("utf-8")) > MAX_MEMORY_SIZE:
        return False

    memory_path.write_text(content, encoding="utf-8")
    return True


def main():
    try:
        raw = sys.stdin.read()
        if not raw.strip():
            sys.exit(0)

        try:
            data = json.loads(raw)
        except json.JSONDecodeError:
            sys.exit(0)

        agent_type = data.get("agent_type", "")
        if not agent_type:
            sys.exit(0)

        # Determine project root (script is at .claude/hooks/write-back-agent-memory.py)
        project_root = Path(__file__).resolve().parent.parent.parent

        # Construct memory file path
        memory_path = project_root / ".claude" / "agent-memory" / f"{agent_type}.md"
        if not memory_path.is_file():
            sys.exit(0)

        # Read existing content for deduplication
        existing_content = memory_path.read_text(encoding="utf-8")

        # Collect learnings from available sources
        all_learnings = []

        # Source 1: tool_results from stdin JSON
        tool_results = data.get("tool_results", None)
        if tool_results:
            all_learnings.extend(
                extract_learnings_from_results(tool_results, existing_content)
            )

        # Source 2: transcript file
        transcript_path = data.get("agent_transcript_path", "")
        if transcript_path:
            all_learnings.extend(
                extract_learnings_from_transcript(transcript_path, existing_content)
            )

        # Deduplicate within batch
        seen = set()
        unique_learnings = []
        for learning in all_learnings:
            key = learning.lower().strip()
            if key not in seen:
                seen.add(key)
                unique_learnings.append(learning)

        # Only write if we have new learnings
        if unique_learnings:
            append_to_memory(memory_path, unique_learnings)

    except Exception:
        # NEVER block workflow -- swallow all exceptions
        pass

    sys.exit(0)


if __name__ == "__main__":
    main()
