#!/usr/bin/env python3
"""
Research Document Freshness Verification Utility

Checks and optionally updates last_verified and juce_version fields in research
document frontmatter. Supports three modes:

  --check          List stale documents (last_verified > 90 days)
  --all            Batch-update all documents to today's date + current JUCE version
  file [file ...]  Update specific files only

Uses regex-based line replacement to preserve frontmatter formatting.

Usage:
    python3 .claude/scripts/verify-freshness.py --check
    python3 .claude/scripts/verify-freshness.py --all
    python3 .claude/scripts/verify-freshness.py --all --juce-version 8.1.0
    python3 .claude/scripts/verify-freshness.py research/fft-best-practices.md
"""

import argparse
import re
import sys
from datetime import date
from pathlib import Path

# Paths (follows project conventions)
SCRIPT_DIR = Path(__file__).parent
CLAUDE_DIR = SCRIPT_DIR.parent
PROJECT_ROOT = CLAUDE_DIR.parent
RESEARCH_DIR = PROJECT_ROOT / "research"

# Freshness thresholds
STALENESS_THRESHOLD_DAYS = 90
DEFAULT_JUCE_VERSION = "8.0.4"


def _set_threshold(days):
    """Update the module-level staleness threshold."""
    global STALENESS_THRESHOLD_DAYS
    STALENESS_THRESHOLD_DAYS = days


def parse_last_verified(filepath):
    """Extract last_verified date from a markdown file's frontmatter.

    Args:
        filepath: Path object to the markdown file.

    Returns:
        (date_str, date_obj) tuple. date_obj is None if unparseable.
    """
    try:
        content = filepath.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return None, None

    match = re.search(r"^last_verified:\s*(.+)$", content, re.MULTILINE)
    if not match:
        return None, None

    date_str = match.group(1).strip().strip("'\"")
    try:
        return date_str, date.fromisoformat(date_str)
    except (ValueError, TypeError):
        return date_str, None


def check_freshness(files=None):
    """Check freshness of research documents and report stale ones.

    Args:
        files: Optional list of specific file paths. If None, checks all research docs.

    Returns:
        List of (filepath, date_str, days_old) tuples for stale documents.
    """
    if files is None:
        md_files = sorted(RESEARCH_DIR.rglob("*.md"))
        md_files = [f for f in md_files if f.name != "README.md"]
    else:
        md_files = [Path(f) if Path(f).is_absolute() else PROJECT_ROOT / f for f in files]

    stale = []
    fresh = []
    unknown = []
    today = date.today()

    for filepath in md_files:
        if not filepath.exists():
            print(f"  Warning: File not found: {filepath}", file=sys.stderr)
            continue

        date_str, verified_date = parse_last_verified(filepath)
        rel_path = filepath.relative_to(PROJECT_ROOT) if filepath.is_relative_to(PROJECT_ROOT) else filepath

        if verified_date is None:
            unknown.append((str(rel_path), date_str or "missing"))
            continue

        days_old = (today - verified_date).days

        if days_old > STALENESS_THRESHOLD_DAYS:
            stale.append((str(rel_path), date_str, days_old))
        else:
            fresh.append((str(rel_path), date_str, days_old))

    # Report
    print(f"Freshness check ({today.isoformat()}, threshold: {STALENESS_THRESHOLD_DAYS} days)")
    print(f"  Total documents: {len(md_files)}")
    print(f"  Fresh: {len(fresh)}")
    print(f"  Stale: {len(stale)}")
    if unknown:
        print(f"  Unknown/missing date: {len(unknown)}")

    if stale:
        print("\nStale documents:")
        for rel_path, date_str, days_old in sorted(stale, key=lambda x: -x[2]):
            print(f"  {rel_path} (verified: {date_str}, {days_old} days ago)")

    if unknown:
        print("\nDocuments with missing/invalid last_verified:")
        for rel_path, date_str in sorted(unknown):
            print(f"  {rel_path} ({date_str})")

    return stale


def update_verified(filepath, juce_version=DEFAULT_JUCE_VERSION):
    """Update last_verified and juce_version in a file's frontmatter.

    Uses regex-based line replacement to preserve formatting.

    Args:
        filepath: Path object to the markdown file.
        juce_version: JUCE version string to set.

    Returns:
        True if file was updated, False otherwise.
    """
    try:
        content = filepath.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as e:
        print(f"  Warning: Could not read {filepath}: {e}", file=sys.stderr)
        return False

    today_str = date.today().isoformat()
    original = content

    # Update last_verified line
    content = re.sub(
        r"^(last_verified:\s*).*$",
        f"\\g<1>{today_str}",
        content,
        count=1,
        flags=re.MULTILINE,
    )

    # Update juce_version line
    content = re.sub(
        r"^(juce_version:\s*).*$",
        f"\\g<1>\"{juce_version}\"",
        content,
        count=1,
        flags=re.MULTILINE,
    )

    if content == original:
        return False

    try:
        filepath.write_text(content, encoding="utf-8")
    except OSError as e:
        print(f"  Warning: Could not write {filepath}: {e}", file=sys.stderr)
        return False

    return True


def batch_update(files=None, juce_version=DEFAULT_JUCE_VERSION):
    """Batch-update last_verified and juce_version for research documents.

    Args:
        files: Optional list of specific file paths. If None, updates all research docs.
        juce_version: JUCE version string to set.

    Returns:
        Number of files updated.
    """
    if files is None:
        md_files = sorted(RESEARCH_DIR.rglob("*.md"))
        md_files = [f for f in md_files if f.name != "README.md"]
    else:
        md_files = [Path(f) if Path(f).is_absolute() else PROJECT_ROOT / f for f in files]

    today_str = date.today().isoformat()
    updated = 0
    skipped = 0

    for filepath in md_files:
        if not filepath.exists():
            print(f"  Warning: File not found: {filepath}", file=sys.stderr)
            skipped += 1
            continue

        # Only update files that have frontmatter
        content = filepath.read_text(encoding="utf-8")
        if not content.startswith("---\n"):
            skipped += 1
            continue

        if update_verified(filepath, juce_version):
            rel_path = filepath.relative_to(PROJECT_ROOT) if filepath.is_relative_to(PROJECT_ROOT) else filepath
            print(f"  Updated: {rel_path} -> {today_str} (JUCE {juce_version})")
            updated += 1
        else:
            skipped += 1

    print(f"\nBatch update complete: {updated} updated, {skipped} skipped")
    return updated


def main():
    """CLI interface for freshness verification."""
    parser = argparse.ArgumentParser(
        description="Verify and update freshness of research document frontmatter"
    )

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--check", action="store_true",
        help="List stale documents (last_verified > 90 days)"
    )
    group.add_argument(
        "--all", action="store_true",
        help="Batch-update all research documents to today's date"
    )

    parser.add_argument(
        "files", nargs="*", default=[],
        help="Specific files to update (when not using --check or --all)"
    )
    parser.add_argument(
        "--juce-version", type=str, default=DEFAULT_JUCE_VERSION,
        help=f"JUCE version to set (default: {DEFAULT_JUCE_VERSION})"
    )
    parser.add_argument(
        "--threshold", type=int, default=STALENESS_THRESHOLD_DAYS,
        help=f"Staleness threshold in days (default: {STALENESS_THRESHOLD_DAYS})"
    )

    args = parser.parse_args()

    # Apply threshold override via module-level variable
    _set_threshold(args.threshold)

    if args.check:
        stale = check_freshness()
        sys.exit(1 if stale else 0)
    elif args.all:
        batch_update(juce_version=args.juce_version)
    elif args.files:
        batch_update(files=args.files, juce_version=args.juce_version)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
