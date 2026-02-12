#!/usr/bin/env python3
"""SubagentStop hook - Deterministic validation after each subagent completes."""

import json
import os
import subprocess
import sys
from pathlib import Path


def get_python_cmd():
    """Return the appropriate python command for this platform."""
    if sys.platform == "win32":
        return sys.executable
    return "python3"


def run_validator(python_cmd, validator_path, args=None, capture_stderr=False):
    """Run a validator script and return its exit code.

    Args:
        python_cmd: Python interpreter to use.
        validator_path: Path to the validator script.
        args: Optional list of arguments to pass.
        capture_stderr: If True, redirect stderr to devnull (for silent runs).

    Returns:
        Exit code from the validator, or 0 if the validator cannot be found/run.
    """
    script = Path(validator_path)
    if not script.is_file():
        return 0

    cmd = [python_cmd, str(script)] + (args or [])
    try:
        stderr_target = subprocess.DEVNULL if capture_stderr else None
        result = subprocess.run(
            cmd,
            timeout=60,
            stdout=subprocess.DEVNULL if capture_stderr else None,
            stderr=stderr_target,
        )
        return result.returncode
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
        return 0


def main():
    # Read JSON input from stdin
    try:
        raw_input = sys.stdin.read()
        data = json.loads(raw_input)
    except (json.JSONDecodeError, ValueError):
        print("Hook not relevant: could not parse JSON input, skipping gracefully")
        sys.exit(0)

    subagent = data.get("subagent_name", "")

    # Gracefully skip if we can't extract subagent name
    if not subagent:
        print("Hook not relevant: no subagent_name in input, skipping gracefully")
        sys.exit(0)

    python_cmd = get_python_cmd()
    validators_dir = Path(".claude") / "hooks" / "validators"

    # ---- Resource accountability validation (warning-only, NEVER blocks workflow) ----
    agent_type = data.get("agent_type") or data.get("subagent_name", "")
    transcript_path = data.get("agent_transcript_path", "")
    plugin_name_acct = data.get("plugin_name", "")

    if agent_type:
        accountability_script = validators_dir / "validate-resource-accountability.py"
        if accountability_script.is_file():
            acct_args = [
                agent_type,
                plugin_name_acct or "",
                transcript_path or "",
            ]
            try:
                result = subprocess.run(
                    [python_cmd, str(accountability_script)] + acct_args,
                    capture_output=True,
                    text=True,
                    timeout=30,
                )
                # Print stderr output as warnings (never block)
                if result.stderr:
                    for line in result.stderr.splitlines():
                        print(line, file=sys.stderr)
            except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
                pass
        # Intentionally ignore exit code -- accountability NEVER blocks workflow

    # ---- Check relevance - only validate our implementation subagents ----
    valid_subagents = ("foundation-shell-agent", "dsp-agent", "gui-agent")
    if subagent not in valid_subagents:
        print(f"Hook not relevant for subagent: {subagent}, skipping gracefully")
        sys.exit(0)

    # Extract plugin name from context if available
    plugin_name = data.get("plugin_name", "")

    # ---- Layer 0: Contract integrity validation (MANDATORY for all stages) ----
    if plugin_name:
        plugin_path = Path("plugins") / plugin_name
        os.environ["PLUGIN_PATH"] = str(plugin_path)

        print(f"Validating contract integrity for {plugin_name}...", file=sys.stderr)

        # 1. Verify contract checksums (Stages 1-4 only)
        checksum_result = run_validator(
            python_cmd,
            validators_dir / "validate-checksums.py",
            args=[str(plugin_path)],
        )
        if checksum_result == 1:
            print(
                "[ERROR] Contract checksum validation FAILED", file=sys.stderr
            )
            sys.exit(2)  # Block workflow
        elif checksum_result == 2:
            print(
                "[WARN] Contract checksum validation: warnings detected",
                file=sys.stderr,
            )

        # 2. Validate cross-contract consistency (all stages)
        cross_result = run_validator(
            python_cmd,
            validators_dir / "validate-cross-contract.py",
            args=[str(plugin_path)],
        )
        if cross_result == 1:
            print(
                "[ERROR] Cross-contract validation FAILED", file=sys.stderr
            )
            sys.exit(2)  # Block workflow
        elif cross_result == 2:
            print(
                "[WARN] Cross-contract validation: warnings detected",
                file=sys.stderr,
            )

        print("[OK] Contract integrity validated", file=sys.stderr)

    # ---- Stage-specific validation dispatch ----
    if subagent == "foundation-shell-agent":
        print("Validating foundation-shell-agent output (Stage 1)...")

        # Validate foundation (CMakeLists.txt, source files, build)
        foundation_result = run_validator(
            python_cmd, validators_dir / "validate-foundation.py"
        )
        if foundation_result != 0:
            print(
                "Foundation validation FAILED: CMakeLists.txt missing or build failed",
                file=sys.stderr,
            )
            sys.exit(2)  # Block workflow

        # Validate parameters (APVTS matches parameter-spec.md)
        params_result = run_validator(
            python_cmd, validators_dir / "validate-parameters.py"
        )
        if params_result != 0:
            print(
                "Parameter validation FAILED: Parameters from spec missing in code",
                file=sys.stderr,
            )
            sys.exit(2)  # Block workflow

        print("Foundation-shell validation PASSED")

    elif subagent == "dsp-agent":
        print("Validating dsp-agent output (Stage 2)...")

        dsp_result = run_validator(
            python_cmd, validators_dir / "validate-dsp-components.py"
        )
        if dsp_result != 0:
            print(
                "DSP component validation FAILED: Components from architecture missing",
                file=sys.stderr,
            )
            sys.exit(2)  # Block workflow

        print("DSP component validation PASSED")

    elif subagent == "gui-agent":
        print("Validating gui-agent output (Stage 3)...")

        gui_result = run_validator(
            python_cmd, validators_dir / "validate-gui-bindings.py"
        )
        if gui_result != 0:
            print(
                "GUI binding validation FAILED: HTML IDs don't match relay IDs",
                file=sys.stderr,
            )
            sys.exit(2)  # Block workflow

        print("GUI binding validation PASSED")

    sys.exit(0)


if __name__ == "__main__":
    main()
