#!/usr/bin/env python3
"""
Context Injection Utility

Discovers relevant research resources and formats them for agent prompt injection.
Combines Phase 10 discovery with content extraction and token budget management.

Usage:
    # As a library
    from inject_context import inject_context
    context_block = inject_context(plugin_name="O-EQ", stage=2, agent_type="dsp-agent")

    # CLI
    python3 .claude/scripts/inject-context.py --stage 2 --agent dsp-agent --plugin O-EQ
    python3 .claude/scripts/inject-context.py --stage 2 --agent dsp-agent --plugin O-EQ --keywords fft spectral
    python3 .claude/scripts/inject-context.py --stage 3 --agent gui-agent --plugin O-EQ --budget 3000
"""

import argparse
import re
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("Error: PyYAML package required. Install with: pip install pyyaml", file=sys.stderr)
    sys.exit(1)

# Paths (follows plugin-registry.py / discover-resources.py conventions)
SCRIPT_DIR = Path(__file__).parent
CLAUDE_DIR = SCRIPT_DIR.parent
PROJECT_ROOT = CLAUDE_DIR.parent

# Import discover-resources.py via importlib.util (hyphenated filename requires this)
import importlib.util

_discover_spec = importlib.util.spec_from_file_location(
    "discover_resources", SCRIPT_DIR / "discover-resources.py"
)
_discover_module = importlib.util.module_from_spec(_discover_spec)
_discover_spec.loader.exec_module(_discover_module)
discover = _discover_module.discover
resolve_role = _discover_module.resolve_role

# Stage pattern file mapping
STAGE_PATTERN_MAP = {
    1: "troubleshooting/patterns/stage-1-patterns.md",
    2: "troubleshooting/patterns/stage-2-patterns.md",
    3: "troubleshooting/patterns/stage-3-patterns.md",
}

# Token budget defaults
DEFAULT_TOKEN_BUDGET = 4000
PATTERN_TOKEN_CAP = 2500
SUPPLEMENTARY_RESERVE = 200


def estimate_tokens(text):
    """Estimate token count using conservative heuristic.

    Uses len(text) / 3.5 which slightly overestimates for safety margin.

    Args:
        text: Input text string.

    Returns:
        Estimated token count as integer.
    """
    if not text:
        return 0
    return int(len(text) / 3.5)


def extract_content(filepath, max_tokens=800):
    """Extract content from a research document within token budget.

    Parses YAML frontmatter for title/summary, then includes the first
    ## section from the body if within budget.

    Args:
        filepath: Path to the research document (relative to PROJECT_ROOT).
        max_tokens: Maximum tokens for extracted content.

    Returns:
        Extracted content string, or empty string on failure.
    """
    full_path = PROJECT_ROOT / filepath
    if not full_path.exists():
        print(f"Warning: Resource file not found: {filepath}", file=sys.stderr)
        return ""

    try:
        raw = full_path.read_text(encoding="utf-8")
    except OSError as e:
        print(f"Warning: Could not read {filepath}: {e}", file=sys.stderr)
        return ""

    # Parse YAML frontmatter
    fm_match = re.match(r"^---\n(.*?)\n---\n?(.*)", raw, re.DOTALL)
    if not fm_match:
        # No frontmatter -- use first lines as fallback
        lines = raw.strip().split("\n")
        title = lines[0].lstrip("# ").strip() if lines else filepath
        body = "\n".join(lines[1:]) if len(lines) > 1 else ""
        result = f"**{title}**\n\n"
    else:
        fm_text, body = fm_match.group(1), fm_match.group(2)
        try:
            fm = yaml.safe_load(fm_text)
        except yaml.YAMLError:
            fm = {}

        title = fm.get("title", Path(filepath).stem)
        summary = fm.get("summary", "")

        result = f"**{title}**\n"
        if summary:
            result += f"{summary}\n\n"

    # Extract first ## section from body if within budget
    if body:
        sections = re.split(r"\n(?=## )", body.strip())
        for section in sections:
            section = section.strip()
            if not section:
                continue
            # Skip table of contents sections
            if section.lower().startswith("## table of contents"):
                continue
            candidate = result + section + "\n"
            if estimate_tokens(candidate) <= max_tokens:
                result = candidate
                break

    # Append source path reference
    result += f"\n_Full document: {filepath}_"

    # Final budget check -- truncate if still over
    if estimate_tokens(result) > max_tokens:
        chars = int(max_tokens * 3.5)
        result = result[:chars] + f"\n\n_[Truncated. Full document: {filepath}]_"

    return result


def get_stage_pattern_content(stage, max_tokens=PATTERN_TOKEN_CAP):
    """Load stage-specific pattern file content within token budget.

    Args:
        stage: Pipeline stage number (0-4).
        max_tokens: Maximum tokens for pattern content.

    Returns:
        Pattern file content string, or None if no pattern for this stage.
    """
    pattern_rel = STAGE_PATTERN_MAP.get(stage)
    if pattern_rel is None:
        return None

    pattern_path = PROJECT_ROOT / pattern_rel
    if not pattern_path.exists():
        print(f"Warning: Stage pattern file not found: {pattern_rel}", file=sys.stderr)
        return None

    try:
        content = pattern_path.read_text(encoding="utf-8")
    except OSError as e:
        print(f"Warning: Could not read stage pattern file: {e}", file=sys.stderr)
        return None

    if estimate_tokens(content) <= max_tokens:
        return content

    # Truncate line-by-line to fit budget
    lines = content.split("\n")
    truncated = []
    running = 0
    for line in lines:
        line_tokens = estimate_tokens(line + "\n")
        if running + line_tokens > max_tokens - 50:  # Reserve space for truncation notice
            break
        truncated.append(line)
        running += line_tokens

    truncated_text = "\n".join(truncated)
    truncated_text += f"\n\n_[Truncated for token budget. Full file: {pattern_rel}]_"
    return truncated_text


def format_context_block(primary_resources, supplementary_resources, pattern_content):
    """Format discovered resources into XML-delimited context block.

    Args:
        primary_resources: List of (result_dict, extracted_content) tuples for primary tier.
        supplementary_resources: List of result dicts for supplementary tier.
        pattern_content: Stage pattern content string, or None.

    Returns:
        Formatted context block string.
    """
    parts = []
    parts.append("<research_context>")
    parts.append("## Relevant Research Resources")
    parts.append("")
    parts.append("The following research has been automatically discovered as relevant")
    parts.append("to your current task. Primary resources are MUST-READ.")

    # Primary tier
    if primary_resources:
        parts.append("")
        parts.append("### MUST-READ (Primary Tier)")
        for result, content in primary_resources:
            parts.append("")
            parts.append(f"**{result['title']}** (relevance: {result['relevance']})")
            parts.append(content)

    # Supplementary tier
    if supplementary_resources:
        parts.append("")
        parts.append("### Supplementary")
        for result in supplementary_resources:
            parts.append(f"- **{result['title']}**: {result['summary']}")
            parts.append(f"  _Path: {result['path']}_")

    # Stage patterns
    if pattern_content:
        parts.append("")
        parts.append("### Stage Patterns (auto-injected)")
        parts.append(pattern_content)

    parts.append("</research_context>")
    return "\n".join(parts)


def inject_context(plugin_name, stage, agent_type, keywords=None, token_budget=DEFAULT_TOKEN_BUDGET):
    """Discover relevant research and format as a context block for agent prompts.

    This is the main entry point. Calls discover() to find relevant resources,
    extracts content within the token budget, auto-injects stage patterns,
    and returns a formatted XML-delimited block.

    Args:
        plugin_name: Plugin name (e.g., "O-EQ"). Passed to discover().
        stage: Pipeline stage (0-4).
        agent_type: Agent name or role (e.g., "dsp-agent", "dsp").
        keywords: Optional list of task-specific keywords.
        token_budget: Maximum total tokens for the output (default 4000).

    Returns:
        Formatted context block string, or empty string on failure.
    """
    try:
        remaining_budget = token_budget

        # 1. Load stage pattern file (first priority)
        pattern_content = get_stage_pattern_content(stage, max_tokens=min(PATTERN_TOKEN_CAP, remaining_budget))
        if pattern_content:
            remaining_budget -= estimate_tokens(pattern_content)

        # 2. Discover resources
        results = discover(
            stage=stage,
            agent_role=agent_type,
            keywords=keywords,
            plugin_name=plugin_name,
        )

        # 3. Separate tiers
        primary_results = [r for r in results if r["tier"] == "primary"]
        supplementary_results = [r for r in results if r["tier"] == "supplementary"]

        # 4. Handle zero results with no pattern
        if not results and pattern_content is None:
            return "<research_context>\nNo relevant research resources discovered for this context.\n</research_context>"

        # 5. Reserve budget for supplementary mentions
        supplementary_budget = min(SUPPLEMENTARY_RESERVE, remaining_budget)
        primary_budget = remaining_budget - supplementary_budget

        # 6. Extract primary resource content within budget
        primary_extracted = []
        used_primary = 0
        for result in primary_results:
            per_resource_max = min(800, primary_budget - used_primary)
            if per_resource_max < 100:
                # Not enough budget -- move to supplementary
                supplementary_results.append(result)
                continue
            content = extract_content(result["path"], max_tokens=per_resource_max)
            if content:
                content_tokens = estimate_tokens(content)
                if used_primary + content_tokens <= primary_budget:
                    primary_extracted.append((result, content))
                    used_primary += content_tokens
                else:
                    # Over budget -- move to supplementary
                    supplementary_results.append(result)
            else:
                supplementary_results.append(result)

        # 7. Build supplementary mentions within budget
        supplementary_formatted = []
        used_supp = 0
        for result in supplementary_results:
            mention = f"- **{result['title']}**: {result['summary']}\n  _Path: {result['path']}_"
            mention_tokens = estimate_tokens(mention)
            if used_supp + mention_tokens <= supplementary_budget:
                supplementary_formatted.append(result)
                used_supp += mention_tokens

        # 8. Format output
        block = format_context_block(primary_extracted, supplementary_formatted, pattern_content)

        # 9. Final budget enforcement
        if estimate_tokens(block) > token_budget:
            # Trim supplementary to fit
            while supplementary_formatted and estimate_tokens(block) > token_budget:
                supplementary_formatted.pop()
                block = format_context_block(primary_extracted, supplementary_formatted, pattern_content)

            # If still over, truncate by character
            if estimate_tokens(block) > token_budget:
                max_chars = int(token_budget * 3.5)
                block = block[:max_chars]
                # Ensure closing tag
                if "</research_context>" not in block:
                    block += "\n</research_context>"

        return block

    except Exception as e:
        print(f"Warning: Context injection failed: {e}", file=sys.stderr)
        return ""


def main():
    """CLI interface for context injection."""
    parser = argparse.ArgumentParser(
        description="Inject relevant research context for agent prompts"
    )
    parser.add_argument(
        "--stage", type=int, required=True, choices=range(5),
        help="Pipeline stage (0-4)"
    )
    parser.add_argument(
        "--agent", type=str, required=True,
        help="Agent name or role (e.g., dsp-agent, gui-agent, dsp, ui)"
    )
    parser.add_argument(
        "--plugin", type=str, default=None,
        help="Plugin name (e.g., O-EQ)"
    )
    parser.add_argument(
        "--keywords", nargs="*", default=None,
        help="Optional task-specific keywords"
    )
    parser.add_argument(
        "--budget", type=int, default=DEFAULT_TOKEN_BUDGET,
        help=f"Token budget override (default: {DEFAULT_TOKEN_BUDGET})"
    )

    args = parser.parse_args()

    result = inject_context(
        plugin_name=args.plugin,
        stage=args.stage,
        agent_type=args.agent,
        keywords=args.keywords,
        token_budget=args.budget,
    )

    if result:
        print(result)


if __name__ == "__main__":
    main()
