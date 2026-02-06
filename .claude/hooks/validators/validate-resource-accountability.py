#!/usr/bin/env python3
"""
Resource Accountability Validator

Compares MUST-READ (primary tier) research resources injected into an agent's
context against the resources the agent self-reported consulting in its report.

WARNING-ONLY GUARANTEE: This script ALWAYS exits 0 regardless of validation
results. Accountability gaps produce stderr warnings but NEVER block workflow.

Usage:
    # CLI (called by hook or manually)
    python3 .claude/hooks/validators/validate-resource-accountability.py <agent-type> [plugin-name] [transcript-path]

    # As library
    from validate_resource_accountability import validate
    validate("dsp-agent", "O-EQ", "/path/to/transcript.jsonl")

    # No args = silent exit 0
    python3 .claude/hooks/validators/validate-resource-accountability.py
"""

import importlib.util
import json
import re
import sys
from pathlib import Path

# Paths (follows validate-checksums.py / inject-context.py conventions)
SCRIPT_DIR = Path(__file__).parent
HOOKS_DIR = SCRIPT_DIR.parent
CLAUDE_DIR = HOOKS_DIR.parent
SCRIPTS_DIR = CLAUDE_DIR / "scripts"

# Import discover-resources.py via importlib.util (hyphenated filename requires this)
_discover_spec = importlib.util.spec_from_file_location(
    "discover_resources", SCRIPTS_DIR / "discover-resources.py"
)
_discover_module = importlib.util.module_from_spec(_discover_spec)
_discover_spec.loader.exec_module(_discover_module)
discover = _discover_module.discover

# Agent-to-stage mapping for canonical pipeline stages
AGENT_STAGE_MAP = {
    "foundation-shell-agent": 1,
    "dsp-agent": 2,
    "gui-agent": 3,
    "research-planning-agent": 0,
    "polish-agent": 4,
}


def _extract_report_from_transcript(transcript_path, agent_type):
    """Parse a JSONL transcript file to find the agent's report.

    Strategies tried per line:
      (a) Full line is valid JSON with 'agent' key matching agent_type
      (b) Line contains a JSON code block with the report

    Args:
        transcript_path: Path to the JSONL transcript file
        agent_type: Agent name to match in the report

    Returns:
        The parsed report dict if found, or None.
        On parse failure, emits a single warning and returns None.
    """
    transcript = Path(transcript_path)
    if not transcript.exists():
        print(
            f"WARNING: Transcript file not found: {transcript_path}",
            file=sys.stderr,
        )
        return None

    try:
        lines = transcript.read_text().splitlines()
    except OSError:
        print(
            f"WARNING: Could not read {agent_type} transcript for accountability validation",
            file=sys.stderr,
        )
        return None

    for line in lines:
        line = line.strip()
        if not line:
            continue

        # Strategy (a): Full line is valid JSON with agent key
        try:
            obj = json.loads(line)
            if isinstance(obj, dict) and obj.get("agent") == agent_type:
                return obj
        except (json.JSONDecodeError, TypeError):
            pass

        # Strategy (b): Line contains a JSON code block with the report
        json_blocks = re.findall(r"```(?:json)?\s*(\{.*?\})\s*```", line, re.DOTALL)
        for block in json_blocks:
            try:
                obj = json.loads(block)
                if isinstance(obj, dict) and obj.get("agent") == agent_type:
                    return obj
            except (json.JSONDecodeError, TypeError):
                continue

        # Strategy (b) variant: try extracting JSON objects from non-JSONL content
        # (e.g., assistant message text containing embedded JSON)
        json_candidates = re.findall(r"\{[^{}]*\"agent\"[^{}]*\}", line)
        for candidate in json_candidates:
            try:
                obj = json.loads(candidate)
                if isinstance(obj, dict) and obj.get("agent") == agent_type:
                    return obj
            except (json.JSONDecodeError, TypeError):
                continue

    return None


def validate(agent_type, plugin_name=None, transcript_path=None):
    """Validate resource accountability for an agent.

    Compares MUST-READ (primary tier) resources that should have been injected
    against the resources_consulted field in the agent's self-report.

    Emits warnings to stderr for gaps. NEVER raises exceptions or exits non-zero.

    Args:
        agent_type: Agent name (e.g., "dsp-agent")
        plugin_name: Plugin being built (optional, passed to discover())
        transcript_path: Path to JSONL transcript file containing agent report
    """
    # Look up stage; skip unknown agents silently
    stage = AGENT_STAGE_MAP.get(agent_type)
    if stage is None:
        return

    # Discover what SHOULD have been injected
    results = discover(stage=stage, agent_role=agent_type)

    # Filter to primary tier only (MUST-READ resources, score >= 0.75)
    must_reads = [r for r in results if r["tier"] == "primary"]

    # If no primary-tier results, nothing to validate
    if not must_reads:
        return

    # If no transcript path provided, we can't validate
    if not transcript_path:
        print(
            f"WARNING: No transcript path provided for {agent_type} accountability validation",
            file=sys.stderr,
        )
        return

    # Parse transcript to find the agent's report
    report = _extract_report_from_transcript(transcript_path, agent_type)

    if report is None:
        print(
            f"WARNING: Could not parse {agent_type} transcript for accountability validation",
            file=sys.stderr,
        )
        return

    # Check if resources_consulted field exists in the report
    resources_consulted = report.get("resources_consulted")

    if resources_consulted is None:
        # Field entirely missing -- agent did not implement accountability
        print(
            f"WARNING: {agent_type} did not include resources_consulted in report "
            f"(MUST-READ resources were injected)",
            file=sys.stderr,
        )
        return

    # Build set of reported paths for fast lookup
    reported_paths = set()
    if isinstance(resources_consulted, list):
        for entry in resources_consulted:
            if isinstance(entry, dict) and "path" in entry:
                reported_paths.add(entry["path"])
            elif isinstance(entry, str):
                # Tolerate plain string entries
                reported_paths.add(entry)

    # Compare: check each MUST-READ resource against reported paths
    for resource in must_reads:
        if resource["path"] not in reported_paths:
            print(
                f"WARNING: {agent_type} did not report consulting MUST-READ resource: "
                f"{resource['path']}",
                file=sys.stderr,
            )


if __name__ == "__main__":
    try:
        agent_type = sys.argv[1] if len(sys.argv) > 1 else None
        plugin_name = sys.argv[2] if len(sys.argv) > 2 else None
        transcript_path = sys.argv[3] if len(sys.argv) > 3 else None

        if agent_type:
            validate(agent_type, plugin_name, transcript_path)
    except Exception as exc:
        # NEVER block workflow -- accountability is warning-only
        print(
            f"WARNING: Resource accountability validation failed with error: {exc}",
            file=sys.stderr,
        )

    sys.exit(0)  # ALWAYS exit 0 -- never block workflow
