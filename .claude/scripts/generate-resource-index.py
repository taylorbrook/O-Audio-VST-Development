#!/usr/bin/env python3
"""
Resource Index Generator

Reads YAML frontmatter from all research documents and produces
.claude/resource-index.json — the manifest used by the discovery script.

Usage:
    python3 .claude/scripts/generate-resource-index.py
"""

import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

import yaml

try:
    import jsonschema
except ImportError:
    print("Error: jsonschema package required. Install with: pip install jsonschema", file=sys.stderr)
    sys.exit(1)

# Paths (follows plugin-registry.py conventions)
SCRIPT_DIR = Path(__file__).parent
CLAUDE_DIR = SCRIPT_DIR.parent
PROJECT_ROOT = CLAUDE_DIR.parent
RESEARCH_DIR = PROJECT_ROOT / "research"
SCHEMA_PATH = CLAUDE_DIR / "resource-index.schema.json"
OUTPUT_PATH = CLAUDE_DIR / "resource-index.json"

# Required frontmatter fields
REQUIRED_FIELDS = ["title", "summary", "domain", "type", "keywords", "stages", "agents"]


def parse_frontmatter(file_path):
    """Extract YAML frontmatter from a markdown file.

    Returns the parsed dict or None if no valid frontmatter found.
    """
    try:
        content = file_path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as e:
        print(f"  Warning: Could not read {file_path}: {e}", file=sys.stderr)
        return None

    match = re.match(r'^---\n(.*?)\n---', content, re.DOTALL)
    if not match:
        return None

    try:
        data = yaml.safe_load(match.group(1))
    except yaml.YAMLError as e:
        print(f"  Warning: Invalid YAML in {file_path}: {e}", file=sys.stderr)
        return None

    if not isinstance(data, dict):
        return None

    # Verify all required fields are present
    missing = [f for f in REQUIRED_FIELDS if f not in data]
    if missing:
        print(f"  Warning: {file_path} missing fields: {', '.join(missing)}", file=sys.stderr)
        return None

    return data


def build_document_entry(file_path, frontmatter):
    """Build a document entry dict from file path and parsed frontmatter."""
    # Use relative path from project root
    rel_path = file_path.relative_to(PROJECT_ROOT)
    # Normalize to forward slashes (for cross-platform consistency)
    path_str = str(rel_path).replace("\\", "/")

    return {
        "path": path_str,
        "title": frontmatter["title"],
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
    skipped = []

    for md_file in md_files:
        frontmatter = parse_frontmatter(md_file)
        if frontmatter is None:
            skipped.append(str(md_file.relative_to(PROJECT_ROOT)))
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

    # Validate against schema
    schema = load_schema()
    try:
        jsonschema.validate(manifest, schema)
    except jsonschema.ValidationError as e:
        print(f"Error: Manifest validation failed: {e.message}", file=sys.stderr)
        print(f"  Path: {' -> '.join(str(p) for p in e.absolute_path)}", file=sys.stderr)
        sys.exit(1)

    # Write output
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)
        f.write("\n")

    # Report
    print(f"Resource index generated: {OUTPUT_PATH.relative_to(PROJECT_ROOT)}")
    print(f"  Documents indexed: {len(documents)}")
    if skipped:
        print(f"  Files skipped (no valid frontmatter): {len(skipped)}")
        for s in skipped:
            print(f"    - {s}")

    return manifest


if __name__ == "__main__":
    generate_manifest()
