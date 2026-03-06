#!/usr/bin/env python3
"""
Resource Index Generator

Reads YAML frontmatter from all research documents and produces
.claude/resource-index.json — the manifest used by the discovery script.

Usage:
    python3 .claude/scripts/generate-resource-index.py
"""

import json
import os
import re
import sys
import tempfile
from datetime import date, datetime, timezone
from pathlib import Path

import yaml

try:
    import jsonschema
    HAS_JSONSCHEMA = True
except ImportError:
    HAS_JSONSCHEMA = False

# Paths
SCRIPT_DIR = Path(__file__).parent
CLAUDE_DIR = SCRIPT_DIR.parent
PROJECT_ROOT = CLAUDE_DIR.parent
RESEARCH_DIR = PROJECT_ROOT / "research"
SCHEMA_PATH = CLAUDE_DIR / "resource-index.schema.json"
OUTPUT_PATH = CLAUDE_DIR / "resource-index.json"
ISSUES_PATH = CLAUDE_DIR / "frontmatter-issues.txt"

# Required frontmatter fields
REQUIRED_FIELDS = ["title", "summary", "domain", "type", "keywords", "stages", "agents",
                   "created", "last_verified", "juce_version"]


def parse_frontmatter(file_path):
    """Extract YAML frontmatter from a markdown file.

    Returns (parsed_dict, skip_reason) tuple.
    parsed_dict is None if no valid frontmatter found; skip_reason explains why.
    """
    try:
        content = file_path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as e:
        print(f"  Warning: Could not read {file_path}: {e}", file=sys.stderr)
        return None, f"read error: {e}"

    match = re.match(r'^---\n(.*?)\n---', content, re.DOTALL)
    if not match:
        return None, "no frontmatter"

    try:
        data = yaml.safe_load(match.group(1))
    except yaml.YAMLError as e:
        print(f"  Warning: Invalid YAML in {file_path}: {e}", file=sys.stderr)
        return None, "invalid YAML"

    if not isinstance(data, dict):
        return None, "frontmatter is not a YAML mapping"

    # Verify all required fields are present
    missing = [f for f in REQUIRED_FIELDS if f not in data]
    if missing:
        print(f"  Warning: {file_path} missing fields: {', '.join(missing)}", file=sys.stderr)
        return None, f"missing fields: {', '.join(missing)}"

    return data, None


def build_document_entry(file_path, frontmatter):
    """Build a document entry dict from file path and parsed frontmatter."""
    # Use relative path from project root
    rel_path = file_path.relative_to(PROJECT_ROOT)
    # Normalize to forward slashes (for cross-platform consistency)
    path_str = str(rel_path).replace("\\", "/")

    # Handle PyYAML date objects -> ISO string
    created = frontmatter["created"]
    if isinstance(created, date):
        created = created.isoformat()
    else:
        created = str(created)

    last_verified = frontmatter["last_verified"]
    if isinstance(last_verified, date):
        last_verified = last_verified.isoformat()
    else:
        last_verified = str(last_verified)

    juce_version = str(frontmatter["juce_version"])

    return {
        "path": path_str,
        "title": frontmatter["title"],
        "created": created,
        "last_verified": last_verified,
        "juce_version": juce_version,
        "summary": frontmatter["summary"],
        "domain": frontmatter["domain"],
        "type": frontmatter["type"],
        "keywords": frontmatter["keywords"],
        "stages": frontmatter["stages"],
        "agents": frontmatter["agents"],
    }


def load_schema():
    """Load the JSON Schema for manifest validation."""
    if not SCHEMA_PATH.exists():
        print(f"Error: Schema not found at {SCHEMA_PATH}", file=sys.stderr)
        sys.exit(1)

    with open(SCHEMA_PATH) as f:
        return json.load(f)


def generate_manifest():
    """Generate the resource-index.json manifest from research document frontmatter."""
    if not RESEARCH_DIR.exists():
        print(f"Error: Research directory not found at {RESEARCH_DIR}", file=sys.stderr)
        sys.exit(1)

    # Find all markdown files, skip README.md
    md_files = sorted(RESEARCH_DIR.rglob("*.md"))
    md_files = [f for f in md_files if f.name != "README.md"]

    documents = []
    skipped = []  # List of (relative_path, reason) tuples

    for md_file in md_files:
        frontmatter, skip_reason = parse_frontmatter(md_file)
        if frontmatter is None:
            rel_path = str(md_file.relative_to(PROJECT_ROOT))
            skipped.append((rel_path, skip_reason))
            continue

        entry = build_document_entry(md_file, frontmatter)
        documents.append(entry)

    # Sort by path for deterministic output
    documents.sort(key=lambda d: d["path"])

    # Build manifest
    manifest = {
        "$schema": "./resource-index.schema.json",
        "version": "1.0.0",
        "generated": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "documents": documents,
    }

    # Validate against schema (skip if jsonschema not available)
    if HAS_JSONSCHEMA:
        schema = load_schema()
        try:
            jsonschema.validate(manifest, schema)
        except jsonschema.ValidationError as e:
            print(f"Error: Manifest validation failed: {e.message}", file=sys.stderr)
            print(f"  Path: {' -> '.join(str(p) for p in e.absolute_path)}", file=sys.stderr)
            sys.exit(1)
    else:
        print("Warning: jsonschema not available, skipping manifest validation", file=sys.stderr)

    # Atomic write: write to temp file then rename
    try:
        fd, temp_path = tempfile.mkstemp(
            dir=str(OUTPUT_PATH.parent), suffix='.tmp', prefix='.resource-index-'
        )
        with os.fdopen(fd, 'w', encoding='utf-8') as f:
            json.dump(manifest, f, indent=2, ensure_ascii=False)
            f.write("\n")
        os.replace(temp_path, str(OUTPUT_PATH))
    except OSError as e:
        # Clean up temp file on failure
        try:
            os.unlink(temp_path)
        except OSError:
            pass
        print(f"Error: Failed to write manifest: {e}", file=sys.stderr)
        sys.exit(1)

    # Write or clean up frontmatter issues file
    today = date.today().isoformat()
    if skipped:
        with open(ISSUES_PATH, 'w', encoding='utf-8') as f:
            for rel_path, reason in skipped:
                f.write(f"[{today}] {rel_path}: {reason}\n")
    else:
        ISSUES_PATH.unlink(missing_ok=True)

    # Report
    print(f"Resource index generated: {OUTPUT_PATH.relative_to(PROJECT_ROOT)}")
    print(f"  Documents indexed: {len(documents)}")
    if skipped:
        print(f"  Files skipped (no valid frontmatter): {len(skipped)}")
        for rel_path, reason in skipped:
            print(f"    - {rel_path}: {reason}")

    return manifest


if __name__ == "__main__":
    generate_manifest()
