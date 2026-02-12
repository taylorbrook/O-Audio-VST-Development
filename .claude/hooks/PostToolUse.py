#!/usr/bin/env python3
"""PostToolUse hook - Real-time code quality validation for JUCE best practices."""

import json
import os
import re
import subprocess
import sys
from pathlib import Path


def read_status_stage(status_path):
    """Extract the stage number from a STATUS.md file."""
    try:
        text = Path(status_path).read_text(encoding="utf-8")
        match = re.search(r"^stage:\s*(\S+)", text, re.MULTILINE)
        if match:
            return match.group(1).strip()
    except OSError:
        pass
    return None


def extract_process_block(content):
    """Extract processBlock function body from C++ content."""
    # Match from the processBlock signature to its closing brace
    # Uses a brace-counting approach for accuracy
    match = re.search(r"void\s+\w+::processBlock\s*\(.*?AudioBuffer", content)
    if not match:
        # Also try without class qualifier
        match = re.search(r"void\s+processBlock\s*\(.*?AudioBuffer", content)
    if not match:
        return None

    start = match.start()
    # Find the opening brace
    brace_pos = content.find("{", start)
    if brace_pos == -1:
        return None

    # Count braces to find matching close
    depth = 0
    for i in range(brace_pos, len(content)):
        if content[i] == "{":
            depth += 1
        elif content[i] == "}":
            depth -= 1
            if depth == 0:
                return content[brace_pos : i + 1]

    # If we didn't find matching brace, return what we have
    return content[brace_pos:]


def check_realtime_safety(process_block, file_path):
    """Check processBlock for real-time safety violations."""
    errors = []
    warnings_list = []

    # Check for heap allocation
    if re.search(r"\bnew\s+|delete\s+|\bmalloc\b|\bfree\b", process_block):
        errors.append(
            "ERROR: Heap allocation detected in processBlock (new/delete/malloc/free)"
        )

    # Check for blocking locks
    if re.search(
        r"std::mutex|\.lock\(\)|\.try_lock\(\)|std::lock_guard|std::unique_lock",
        process_block,
    ):
        errors.append("ERROR: Blocking mutex/lock detected in processBlock")

    # Check for I/O operations
    if re.search(
        r"File::|URL::|FileOutputStream|FileInputStream", process_block
    ):
        errors.append("ERROR: File I/O operations detected in processBlock")

    # Check for console output
    if re.search(r"std::cout|std::cerr|\bprintf\b|\bfprintf\b", process_block):
        errors.append(
            "ERROR: Console output detected in processBlock (std::cout/printf)"
        )

    # Best practice warnings (advisory only)
    if "ScopedNoDenormals" not in process_block:
        warnings_list.append("WARNING: Missing ScopedNoDenormals in processBlock")

    return errors, warnings_list


def main():
    # Read JSON input from stdin
    try:
        raw_input = sys.stdin.read()
        data = json.loads(raw_input)
    except (json.JSONDecodeError, ValueError):
        sys.exit(0)

    tool_name = data.get("tool_name", "")

    # Check relevance FIRST - only validate on Write/Edit to plugin source files
    if tool_name not in ("Write", "Edit"):
        sys.exit(0)

    tool_input = data.get("tool_input", {})
    file_path = tool_input.get("file_path", "")

    if not file_path:
        sys.exit(0)

    file_path_obj = Path(file_path)

    # ---- Layer 0: Contract immutability enforcement (CRITICAL) ----
    contract_match = re.search(
        r"plugins[/\\]([^/\\]+)[/\\]\.planning[/\\]"
        r"(creative-brief|parameter-spec|architecture|plan)\.md$",
        file_path,
    )
    if contract_match:
        plugin_name = contract_match.group(1)
        contract_file = contract_match.group(2) + ".md"
        plugin_path = Path("plugins") / plugin_name
        status_path = plugin_path / ".planning" / "STATUS.md"

        if status_path.is_file():
            stage = read_status_stage(status_path)
            if stage in ("1", "2", "3", "4"):
                print("", file=sys.stderr)
                print(
                    "[ERROR] CONTRACT IMMUTABILITY VIOLATION", file=sys.stderr
                )
                print("", file=sys.stderr)
                print(
                    f"Cannot modify {contract_file} during Stage {stage} (implementation).",
                    file=sys.stderr,
                )
                print("", file=sys.stderr)
                print(
                    "Contracts are immutable during Stages 1-4 to prevent drift.",
                    file=sys.stderr,
                )
                print(
                    "All implementation stages reference the same contract specifications.",
                    file=sys.stderr,
                )
                print("", file=sys.stderr)
                print("If you need to modify contracts:", file=sys.stderr)
                print("  1. Return to Stage 0-1 (planning)", file=sys.stderr)
                print("  2. Update contracts", file=sys.stderr)
                print(
                    "  3. Restart implementation from Stage 1", file=sys.stderr
                )
                print("", file=sys.stderr)
                print(
                    "Alternatively, use /improve after Stage 4 to make changes with versioning.",
                    file=sys.stderr,
                )
                sys.exit(1)

    # ---- Determine validation level based on file type ----
    plugin_source = False
    plugin_file = False

    # Check if plugin source file (.cpp or .h in plugins/*/Source/)
    if re.search(r"plugins[/\\][^/\\]+[/\\]Source[/\\].*\.(cpp|h)$", file_path):
        plugin_source = True
        plugin_file = True
    # Check if other plugin file (CMakeLists.txt or ui files)
    elif re.search(
        r"plugins[/\\][^/\\]+[/\\](CMakeLists\.txt|ui[/\\].*\.(html|js))$",
        file_path,
    ):
        plugin_file = True

    # Skip if not a plugin file
    if not plugin_file:
        sys.exit(0)

    # ---- Extract file content based on tool type ----
    file_content = ""
    if tool_name == "Write":
        file_content = tool_input.get("content", "")
    elif tool_name == "Edit":
        file_content = tool_input.get("new_string", "")

    if not file_content:
        sys.exit(0)

    # ---- Layer 1: Real-time safety checks (processBlock only, C++ source) ----
    if plugin_source:
        process_block = extract_process_block(file_content)
        if process_block:
            errors, warnings_list = check_realtime_safety(
                process_block, file_path
            )

            if errors:
                print(
                    f"Real-time safety violations detected in {file_path}:",
                    file=sys.stderr,
                )
                for err in errors:
                    print(f"  {err}", file=sys.stderr)
                print("", file=sys.stderr)
                print(
                    "These violations can cause audio dropouts, glitches, or crashes.",
                    file=sys.stderr,
                )
                print(
                    "processBlock must be real-time safe (no allocations, locks, or I/O).",
                    file=sys.stderr,
                )
                sys.exit(1)  # Block workflow on ERROR

            if warnings_list:
                print(
                    f"Code quality recommendations for {file_path}:",
                    file=sys.stderr,
                )
                for warn in warnings_list:
                    print(f"  {warn}", file=sys.stderr)
                # Don't block on warnings, just inform

    # ---- Layer 2: Silent failure pattern detection (all plugin files) ----
    print("Running silent failure pattern detection...", file=sys.stderr)

    # Determine python command
    python_cmd = "python3"
    if sys.platform == "win32":
        python_cmd = sys.executable

    validator_path = (
        Path(".claude") / "hooks" / "validators" / "validate-silent-failures.py"
    )

    if validator_path.is_file():
        try:
            result = subprocess.run(
                [python_cmd, str(validator_path)],
                timeout=30,
            )
            exit_code = result.returncode
        except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
            # If validator can't run, don't block
            exit_code = 0

        if exit_code == 1:
            # Critical patterns detected - workflow blocked
            print("", file=sys.stderr)
            print(
                "[ERROR] PostToolUse validation FAILED: Silent failure patterns detected",
                file=sys.stderr,
            )
            print(
                "Fix these issues before proceeding (they compile but fail at runtime)",
                file=sys.stderr,
            )
            sys.exit(1)
        elif exit_code == 2:
            # Warnings detected - inform but don't block
            print("", file=sys.stderr)
            print(
                "[WARN] PostToolUse validation: Warnings detected (see above)",
                file=sys.stderr,
            )
            print(
                "Consider addressing these issues for better code quality",
                file=sys.stderr,
            )

    sys.exit(0)


if __name__ == "__main__":
    main()
