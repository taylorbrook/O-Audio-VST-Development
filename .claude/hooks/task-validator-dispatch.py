#!/usr/bin/env python3
"""task-validator-dispatch.py - Routes task content to relevant domain validators."""

import json
import os
import re
import subprocess
import sys
from pathlib import Path


# Code-related patterns that indicate a code task
CODE_PATTERNS = re.compile(
    r"\.(cpp|h|cmake|html|js|css)"
    r"|processBlock|prepareToPlay|parameter|APVTS"
    r"|WebView|relay|CMakeLists",
    re.IGNORECASE,
)

# Validator dispatch rules: (pattern, validator_scripts, label)
DISPATCH_RULES = [
    (
        re.compile(
            r"processBlock|prepareToPlay|DSP|audio.processing|dsp.component",
            re.IGNORECASE,
        ),
        ["validate-dsp-components.py", "validate-silent-failures.py"],
        ["DSP Components", "Silent Failures"],
    ),
    (
        re.compile(
            r"parameter|APVTS|createParameterLayout|parameter.spec",
            re.IGNORECASE,
        ),
        ["validate-parameters.py"],
        ["Parameters"],
    ),
    (
        re.compile(
            r"WebView|relay|index\.html|binding|GUI|editor",
            re.IGNORECASE,
        ),
        ["validate-gui-bindings.py"],
        ["GUI Bindings"],
    ),
    (
        re.compile(
            r"contract|ARCHITECTURE|ROADMAP|BRIEF|cross.contract",
            re.IGNORECASE,
        ),
        ["validate-cross-contract.py"],
        ["Cross-Contract"],
    ),
    (
        re.compile(
            r"checksum|STATUS\.md|SHA|integrity",
            re.IGNORECASE,
        ),
        ["validate-checksums.py"],
        ["Checksums"],
    ),
    (
        re.compile(
            r"research|resource|frontmatter",
            re.IGNORECASE,
        ),
        ["validate-resource-accountability.py"],
        ["Resource Accountability"],
    ),
]


def get_python_cmd():
    """Return the appropriate python command for this platform."""
    if sys.platform == "win32":
        return sys.executable
    return "python3"


def main():
    # Read task content from stdin
    try:
        raw_input = sys.stdin.read()
        data = json.loads(raw_input)
    except (json.JSONDecodeError, ValueError):
        sys.exit(0)

    task_subject = data.get("task_subject", "")
    task_description = data.get("task_description", "")
    task_content = f"{task_subject} {task_description}"

    # Skip non-code tasks: only validate if task mentions code-related content
    if not CODE_PATTERNS.search(task_content):
        sys.exit(0)  # Not a code task, skip validation

    # Detect active plugin from task content or environment variable
    plugin_name = os.environ.get("ACTIVE_PLUGIN", "")
    if not plugin_name:
        # Try to extract O-PluginName from task content
        match = re.search(r"O-[A-Za-z]+", task_content)
        if match:
            plugin_name = match.group(0)

    if not plugin_name:
        sys.exit(0)  # Cannot determine plugin, skip validation

    plugin_path = Path("plugins") / plugin_name

    # Resolve paths
    script_dir = Path(__file__).resolve().parent
    validators_dir = script_dir / "validators"

    # Determine project directory for running validators
    project_dir = os.environ.get("CLAUDE_PROJECT_DIR", "")
    if not project_dir:
        # Fall back to two levels up from hooks dir
        project_dir = str(script_dir.parent.parent)

    python_cmd = get_python_cmd()
    errors = []

    # ---- Validator dispatch based on keyword matching ----
    for pattern, validator_scripts, labels in DISPATCH_RULES:
        if pattern.search(task_content):
            for validator_script, label in zip(validator_scripts, labels):
                validator_path = validators_dir / validator_script
                if validator_path.is_file():
                    try:
                        env = os.environ.copy()
                        env["ACTIVE_PLUGIN"] = plugin_name
                        result = subprocess.run(
                            [python_cmd, str(validator_path)],
                            capture_output=True,
                            text=True,
                            cwd=project_dir,
                            timeout=60,
                            env=env,
                        )
                        if result.returncode != 0:
                            output = (
                                result.stderr.strip()
                                or result.stdout.strip()
                                or "Validation failed"
                            )
                            errors.append(f"[{label}] {output}")
                    except subprocess.TimeoutExpired:
                        errors.append(f"[{label}] Validator timed out")
                    except (FileNotFoundError, OSError) as exc:
                        errors.append(f"[{label}] Could not run validator: {exc}")

    # ---- Report results ----
    if errors:
        print(
            f"Task validation failed for {plugin_name}:", file=sys.stderr
        )
        for err in errors:
            print(f"  {err}", file=sys.stderr)
        print("", file=sys.stderr)
        print("Fix the issues above and retry the task.", file=sys.stderr)
        sys.exit(2)  # Block task completion with feedback

    sys.exit(0)  # All validations passed


if __name__ == "__main__":
    main()
