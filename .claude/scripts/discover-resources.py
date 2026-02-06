#!/usr/bin/env python3
"""
Resource Discovery Script

Queries the resource-index.json manifest and returns ranked relevant resources
based on pipeline stage, agent role, and optional keyword matching.

Usage:
    # As a library
    from discover_resources import discover
    results = discover(stage=2, agent_role="dsp", keywords=["fft", "spectral"])

    # CLI for testing
    python3 .claude/scripts/discover-resources.py --stage 2 --role dsp --keywords fft spectral
"""

import argparse
import json
import sys
from pathlib import Path

try:
    import jsonschema
    HAS_JSONSCHEMA = True
except ImportError:
    HAS_JSONSCHEMA = False

# Paths (follows plugin-registry.py conventions)
SCRIPT_DIR = Path(__file__).parent
CLAUDE_DIR = SCRIPT_DIR.parent
PROJECT_ROOT = CLAUDE_DIR.parent
MANIFEST_PATH = CLAUDE_DIR / "resource-index.json"
SCHEMA_PATH = CLAUDE_DIR / "resource-index.schema.json"

# Agent name-to-role mapping
AGENT_ROLE_MAP = {
    "dsp-agent": "dsp",
    "gui-agent": "ui",
    "ui-design-agent": "ui",
    "ui-finalization-agent": "ui",
    "foundation-shell-agent": "build",
    "research-planning-agent": "research",
    "polish-agent": "dsp",
    "validation-agent": "build",
    "troubleshoot-agent": "dsp",
    "music-theory-agent": "dsp",
    "aesthetics-agent": "ui",
}

# Valid roles (for pass-through)
VALID_ROLES = {"dsp", "ui", "build", "research"}

# Scoring weights
WEIGHT_STAGE = 0.4
WEIGHT_ROLE = 0.35
WEIGHT_KEYWORD = 0.25
KEYWORD_NORM_DIVISOR = 3.0

# Tier thresholds
TIER_PRIMARY = 0.75


def resolve_role(agent_name_or_role):
    """Resolve an agent name to its role, or return the input if already a valid role.

    Examples:
        resolve_role("dsp-agent") -> "dsp"
        resolve_role("dsp") -> "dsp"
        resolve_role("gui-agent") -> "ui"
    """
    if agent_name_or_role in VALID_ROLES:
        return agent_name_or_role
    return AGENT_ROLE_MAP.get(agent_name_or_role, agent_name_or_role)


def load_and_validate_manifest():
    """Load the resource index manifest and validate against JSON Schema.

    Returns the manifest dict, or None if missing/invalid.
    Per decision: empty results returned silently on failure.
    Degrades gracefully when jsonschema is not available (loads without validation).
    """
    if not MANIFEST_PATH.exists():
        return None

    try:
        with open(MANIFEST_PATH) as f:
            manifest = json.load(f)
    except (json.JSONDecodeError, OSError):
        return None

    if HAS_JSONSCHEMA:
        if not SCHEMA_PATH.exists():
            return None

        try:
            with open(SCHEMA_PATH) as f:
                schema = json.load(f)
        except (json.JSONDecodeError, OSError):
            return None

        try:
            jsonschema.validate(manifest, schema)
        except jsonschema.ValidationError:
            return None
    else:
        print("Warning: jsonschema not available, skipping manifest validation", file=sys.stderr)

    return manifest


def score_document(doc, stage, agent_role, keywords):
    """Score a document against the query parameters.

    Scoring weights:
        - Stage match: 0.4 (document covers this pipeline stage)
        - Agent role match: 0.35 (document targets this agent role)
        - Keyword overlap: 0.25 (normalized by KEYWORD_NORM_DIVISOR)

    Returns (score, match_reasons) tuple.
    """
    score = 0.0
    match_reasons = []

    # Stage match: 0.4 weight
    if stage in doc["stages"]:
        score += WEIGHT_STAGE
        match_reasons.append("stage_match")

    # Agent role match: 0.35 weight
    if agent_role in doc["agents"]:
        score += WEIGHT_ROLE
        match_reasons.append("role_match")

    # Keyword overlap: 0.25 weight (normalized)
    if keywords:
        doc_keywords = set(doc["keywords"])
        query_keywords = set(k.lower() for k in keywords)
        overlap = doc_keywords & query_keywords
        if overlap:
            keyword_score = min(len(overlap) / KEYWORD_NORM_DIVISOR, 1.0) * WEIGHT_KEYWORD
            score += keyword_score
            for kw in sorted(overlap):
                match_reasons.append(f"keyword:{kw}")

    return score, match_reasons


def determine_tier(score, match_reasons):
    """Determine the result tier based on score and match dimensions.

    primary: matches BOTH stage AND agent role (score >= 0.75)
    supplementary: above threshold but doesn't match both dimensions
    """
    has_stage = "stage_match" in match_reasons
    has_role = "role_match" in match_reasons

    if has_stage and has_role and score >= TIER_PRIMARY:
        return "primary"
    return "supplementary"


def discover(
    stage,
    agent_role,
    keywords=None,
    plugin_name=None,
    max_results=10,
    threshold=0.3,
):
    """Discover relevant research resources for a given task context.

    Args:
        stage: Current pipeline stage (0-4)
        agent_role: Agent role ("dsp", "ui", "build", "research") or agent name
        keywords: Optional list of task-specific keywords
        plugin_name: Optional plugin name (reserved for future use)
        max_results: Maximum number of results to return (safety cap)
        threshold: Minimum relevance score to include

    Returns:
        List of result dicts sorted by relevance (descending), each containing:
        - path: Relative path to the research document
        - title: Document title
        - summary: Document summary
        - relevance: Relevance score (0.0-1.0)
        - tier: "primary" or "supplementary"
        - match_reasons: List of match reason strings
    """
    # Resolve agent name to role
    role = resolve_role(agent_role)

    # Load and validate manifest
    manifest = load_and_validate_manifest()
    if manifest is None:
        return []

    results = []
    for doc in manifest["documents"]:
        score, reasons = score_document(doc, stage, role, keywords)

        if score < threshold:
            continue

        tier = determine_tier(score, reasons)

        results.append({
            "path": doc["path"],
            "title": doc["title"],
            "summary": doc["summary"],
            "last_verified": doc.get("last_verified", ""),
            "relevance": round(score, 4),
            "tier": tier,
            "match_reasons": reasons,
        })

    # Sort by relevance descending, then by path for deterministic ordering
    results.sort(key=lambda r: (-r["relevance"], r["path"]))

    # Apply safety cap
    return results[:max_results]


def main():
    """CLI interface for testing discovery queries."""
    parser = argparse.ArgumentParser(
        description="Discover relevant research resources for a task context"
    )
    parser.add_argument(
        "--stage", type=int, required=True, choices=range(5),
        help="Pipeline stage (0-4)"
    )
    parser.add_argument(
        "--role", type=str, required=True,
        help="Agent role (dsp, ui, build, research) or agent name (e.g., dsp-agent)"
    )
    parser.add_argument(
        "--keywords", nargs="*", default=None,
        help="Optional task-specific keywords"
    )
    parser.add_argument(
        "--max-results", type=int, default=10,
        help="Maximum results to return (default: 10)"
    )
    parser.add_argument(
        "--threshold", type=float, default=0.3,
        help="Minimum relevance score (default: 0.3)"
    )

    args = parser.parse_args()

    results = discover(
        stage=args.stage,
        agent_role=args.role,
        keywords=args.keywords,
        max_results=args.max_results,
        threshold=args.threshold,
    )

    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
