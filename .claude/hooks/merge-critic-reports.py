#!/usr/bin/env python3
"""
merge-critic-reports.py

Merges multiple critic JSON reports into a unified severity-ranked report.
Conforms output to critic-report-unified.schema.json.

Usage:
    python3 merge-critic-reports.py [--stage STAGE] [--plugin PLUGIN] report1.json report2.json ...
    python3 merge-critic-reports.py [--stage STAGE] [--plugin PLUGIN] --dir /path/to/reports/

Exit codes:
    0 - Progression allowed (no blockers)
    1 - Progression blocked (blockers found)
    2 - Usage error or malformed input

Can also be imported as a module:
    from merge_critic_reports import merge_reports, SEVERITY_ORDER
"""

import json
import sys
import os
from datetime import datetime, timezone
from pathlib import Path

SEVERITY_ORDER = {"blocker": 0, "error": 1, "warning": 2, "note": 3, "info": 4}

SEVERITY_NORMALIZE = {
    "error": "blocker",
    "blocker": "blocker",
    "warning": "warning",
    "note": "note",
    "info": "info",
}


def merge_reports(report_paths, stage=None, plugin=None):
    """
    Merge critic reports into unified severity-ranked output.

    Args:
        report_paths: List of Path objects pointing to critic JSON reports.
        stage: Stage identifier (e.g., "2-dsp"). Auto-detected from first report if not provided.
        plugin: Plugin name. Auto-detected from first report if not provided.

    Returns:
        dict conforming to critic-report-unified.schema.json
    """
    all_issues = []
    critics_run = []
    skipped_reports = []

    for path in report_paths:
        try:
            with open(path) as f:
                content = f.read().strip()
                if not content:
                    skipped_reports.append(str(path))
                    print(f"WARNING: Empty report file skipped: {path}", file=sys.stderr)
                    continue
                report = json.loads(content)
        except json.JSONDecodeError as e:
            skipped_reports.append(str(path))
            print(f"WARNING: Malformed JSON in {path}: {e}", file=sys.stderr)
            continue
        except OSError as e:
            skipped_reports.append(str(path))
            print(f"WARNING: Cannot read {path}: {e}", file=sys.stderr)
            continue

        if not isinstance(report, dict):
            skipped_reports.append(str(path))
            print(f"WARNING: Report is not a JSON object: {path}", file=sys.stderr)
            continue

        critic_name = report.get("critic", "unknown")
        critics_run.append(critic_name)

        # Auto-detect stage and plugin from first valid report
        if stage is None:
            stage = report.get("stage", "unknown")
        if plugin is None:
            plugin = report.get("plugin", "unknown")

        for issue in report.get("issues", []):
            raw_severity = issue.get("severity", "note")
            severity = SEVERITY_NORMALIZE.get(raw_severity, raw_severity)

            unified_issue = {
                "severity": severity,
                "critic": critic_name,
                "id": issue.get("id", "UNKNOWN-000"),
                "category": issue.get("category", ""),
                "location": issue.get("location", ""),
                "description": issue.get("description", ""),
                "fixSuggestion": issue.get("fixSuggestion", ""),
            }
            all_issues.append(unified_issue)

    # Sort by severity rank (blockers first)
    all_issues.sort(key=lambda x: SEVERITY_ORDER.get(x["severity"], 99))

    blocking_count = sum(1 for i in all_issues if i["severity"] == "blocker")
    warning_count = sum(1 for i in all_issues if i["severity"] == "warning")
    note_count = sum(
        1 for i in all_issues if i["severity"] in ("note", "info")
    )

    unified_report = {
        "stage": stage or "unknown",
        "plugin": plugin or "unknown",
        "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "critics_run": critics_run,
        "unified_issues": all_issues,
        "progression_allowed": blocking_count == 0,
        "blocking_count": blocking_count,
        "warning_count": warning_count,
        "note_count": note_count,
    }

    return unified_report


def collect_report_paths(args):
    """
    Collect report file paths from command-line arguments.

    Handles both individual file paths and --dir directory scanning.
    """
    paths = []
    dir_mode = False
    stage = None
    plugin = None
    i = 0

    while i < len(args):
        arg = args[i]
        if arg == "--dir":
            if i + 1 >= len(args):
                print("ERROR: --dir requires a directory path", file=sys.stderr)
                sys.exit(2)
            dir_path = Path(args[i + 1])
            if not dir_path.is_dir():
                print(f"ERROR: Not a directory: {dir_path}", file=sys.stderr)
                sys.exit(2)
            paths.extend(sorted(dir_path.glob("*.json")))
            dir_mode = True
            i += 2
        elif arg == "--stage":
            if i + 1 >= len(args):
                print("ERROR: --stage requires a value", file=sys.stderr)
                sys.exit(2)
            stage = args[i + 1]
            i += 2
        elif arg == "--plugin":
            if i + 1 >= len(args):
                print("ERROR: --plugin requires a value", file=sys.stderr)
                sys.exit(2)
            plugin = args[i + 1]
            i += 2
        elif arg in ("--help", "-h"):
            print(__doc__)
            sys.exit(0)
        else:
            path = Path(arg)
            if path.is_file():
                paths.append(path)
            else:
                print(f"WARNING: File not found, skipped: {arg}", file=sys.stderr)
            i += 1

    return paths, stage, plugin


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(
            "Usage: merge-critic-reports.py [--stage STAGE] [--plugin PLUGIN] "
            "report1.json report2.json ...",
            file=sys.stderr,
        )
        print(
            "       merge-critic-reports.py [--stage STAGE] [--plugin PLUGIN] "
            "--dir /path/to/reports/",
            file=sys.stderr,
        )
        sys.exit(2)

    report_paths, cli_stage, cli_plugin = collect_report_paths(sys.argv[1:])

    if not report_paths:
        print("ERROR: No valid report files found", file=sys.stderr)
        sys.exit(2)

    unified = merge_reports(report_paths, stage=cli_stage, plugin=cli_plugin)

    # Output unified report as JSON to stdout
    print(json.dumps(unified, indent=2))

    # Exit code based on progression
    if unified["progression_allowed"]:
        sys.exit(0)
    else:
        sys.exit(1)
