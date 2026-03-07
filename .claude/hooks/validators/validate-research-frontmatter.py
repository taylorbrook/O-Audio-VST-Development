#!/usr/bin/env python3
"""
Validate YAML frontmatter in research documents.

Ensures research markdown files contain valid YAML frontmatter with all
required fields and correct enum values. Skips non-research files silently.

Exit codes:
  0 - Valid frontmatter (or file is not a research document)
  1 - Invalid or missing frontmatter
  2 - Warning (reserved for future use)

Usage:
  python3 validate-research-frontmatter.py <file_path>
"""
import json
import sys
import re
from datetime import date
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Optional

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML not installed", file=sys.stderr)
    sys.exit(1)


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REQUIRED_FIELDS = {"title", "created", "domain", "type", "keywords"}

VALID_DOMAINS = {"dsp", "ui", "architecture", "tooling", "market-research", "ml",
                 "spatial-audio", "cross-platform"}
VALID_TYPES = {"research", "algorithm", "guide", "market-research"}
VALID_AGENTS = {"dsp", "ui", "build", "research"}  # Used for optional validation when agents field is present

KEYWORD_PATTERN = re.compile(r"^[a-z0-9-]+$")
FRONTMATTER_PATTERN = re.compile(r"^---\n(.*?)\n---", re.DOTALL)
JUCE_VERSION_PATTERN = re.compile(r"^\d+\.\d+\.\d+$")


# ---------------------------------------------------------------------------
# Result dataclass (matches project validator conventions)
# ---------------------------------------------------------------------------

@dataclass
class ValidationResult:
    """Result of a frontmatter validation check."""
    passed: bool
    errors: List[str] = field(default_factory=list)
    file_path: Optional[str] = None

    def report(self) -> int:
        """Print errors to stderr and return exit code."""
        if self.passed:
            return 0
        for error in self.errors:
            print(f"FRONTMATTER ERROR [{self.file_path}]: {error}", file=sys.stderr)
        return 1


# ---------------------------------------------------------------------------
# Core validation logic
# ---------------------------------------------------------------------------

def parse_frontmatter(content: str) -> Optional[dict]:
    """Extract and parse YAML frontmatter from markdown content.

    Returns parsed dict or None if no frontmatter found.
    Raises yaml.YAMLError if YAML is malformed.
    """
    match = FRONTMATTER_PATTERN.match(content)
    if not match:
        return None
    return yaml.safe_load(match.group(1))


def validate_date_field(fm: dict, field_name: str, errors: list):
    """Validate a date field (created or last_verified).

    Handles both datetime.date objects (PyYAML auto-parsed) and strings.
    """
    value = fm.get(field_name)
    if value is None:
        return  # Missing field caught by required check
    if isinstance(value, date):
        return  # PyYAML parsed as date object -- valid
    if isinstance(value, str):
        try:
            date.fromisoformat(value)
        except ValueError:
            errors.append(f"Invalid {field_name} date '{value}' (must be YYYY-MM-DD)")
    else:
        errors.append(f"'{field_name}' must be a date (YYYY-MM-DD)")


def validate_juce_version(fm: dict, errors: list):
    """Validate juce_version field matches semver pattern."""
    value = fm.get("juce_version")
    if value is None:
        return  # Missing field caught by required check
    value_str = str(value)
    if not JUCE_VERSION_PATTERN.match(value_str):
        errors.append(f"Invalid juce_version '{value}' (must be semver like 8.0.4)")


def validate_frontmatter(filepath: Path) -> ValidationResult:
    """Validate YAML frontmatter in a research document.

    Returns a ValidationResult with pass/fail status and error details.
    """
    result = ValidationResult(passed=True, file_path=str(filepath))

    # Read file content
    try:
        content = filepath.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as e:
        result.passed = False
        result.errors.append(f"Cannot read file: {e}")
        return result

    # Parse frontmatter
    try:
        fm = parse_frontmatter(content)
    except yaml.YAMLError as e:
        result.passed = False
        result.errors.append(f"Invalid YAML in frontmatter: {e}")
        return result

    if fm is None:
        result.passed = False
        result.errors.append("Missing YAML frontmatter (expected --- delimiters)")
        return result

    if not isinstance(fm, dict):
        result.passed = False
        result.errors.append("Frontmatter is not a YAML mapping")
        return result

    errors = []

    # ---- Check required fields ----
    missing = REQUIRED_FIELDS - set(fm.keys())
    if missing:
        errors.append(f"Missing required fields: {sorted(missing)}")

    # ---- Validate domain enum ----
    domain = fm.get("domain")
    if domain is not None and domain not in VALID_DOMAINS:
        errors.append(
            f"Invalid domain '{domain}' (must be one of {sorted(VALID_DOMAINS)})"
        )

    # ---- Validate type enum ----
    doc_type = fm.get("type")
    if doc_type is not None and doc_type not in VALID_TYPES:
        errors.append(
            f"Invalid type '{doc_type}' (must be one of {sorted(VALID_TYPES)})"
        )

    # ---- Validate keywords ----
    keywords = fm.get("keywords")
    if isinstance(keywords, list):
        if len(keywords) < 3:
            errors.append(f"Too few keywords ({len(keywords)}); minimum is 3")
        if len(keywords) > 15:
            errors.append(f"Too many keywords ({len(keywords)}); maximum is 15")
        for kw in keywords:
            if not isinstance(kw, str) or not KEYWORD_PATTERN.match(kw):
                errors.append(
                    f"Keyword '{kw}' must be a lowercase alphanumeric string with hyphens"
                )
    elif keywords is not None:
        errors.append("'keywords' must be a list")

    # ---- Validate stages ----
    stages = fm.get("stages")
    if isinstance(stages, list):
        if len(stages) < 1:
            errors.append("'stages' must contain at least 1 entry")
        for stage in stages:
            if not isinstance(stage, int) or stage < 0 or stage > 4:
                errors.append(f"Invalid stage '{stage}' (must be integer 0-4)")
    elif stages is not None:
        errors.append("'stages' must be a list")

    # ---- Validate agents ----
    agents = fm.get("agents")
    if isinstance(agents, list):
        if len(agents) < 1:
            errors.append("'agents' must contain at least 1 entry")
        for agent in agents:
            if agent not in VALID_AGENTS:
                errors.append(
                    f"Invalid agent role '{agent}' (must be one of {sorted(VALID_AGENTS)})"
                )
    elif agents is not None:
        errors.append("'agents' must be a list")

    # ---- Validate title length ----
    title = fm.get("title")
    if isinstance(title, str):
        if len(title) < 1:
            errors.append("'title' must not be empty")
        if len(title) > 150:
            errors.append(f"'title' too long ({len(title)} chars); maximum is 150")

    # ---- Validate summary length ----
    summary = fm.get("summary")
    if isinstance(summary, str):
        if len(summary) < 10:
            errors.append(f"'summary' too short ({len(summary)} chars); minimum is 10")
        if len(summary) > 500:
            errors.append(f"'summary' too long ({len(summary)} chars); maximum is 500")

    # ---- Validate date fields ----
    validate_date_field(fm, "created", errors)
    validate_date_field(fm, "last_verified", errors)

    # ---- Validate juce_version ----
    validate_juce_version(fm, errors)

    if errors:
        result.passed = False
        result.errors = errors

    return result


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    """Main entry point. Accepts file path via stdin JSON (PostToolUse hook) or argv[1]."""
    file_path_str = ""

    # Try reading stdin JSON (PostToolUse hook mode)
    if not sys.stdin.isatty():
        try:
            raw_input = sys.stdin.read()
            data = json.loads(raw_input)
            tool_input = data.get("tool_input", {})
            file_path_str = tool_input.get("file_path", "")
        except (json.JSONDecodeError, ValueError):
            file_path_str = ""

    # Fall back to argv[1] (CLI usage)
    if not file_path_str and len(sys.argv) >= 2:
        file_path_str = sys.argv[1]

    if not file_path_str:
        print("Usage: validate-research-frontmatter.py <file_path>", file=sys.stderr)
        return 1

    filepath = Path(file_path_str)

    # Skip non-existent files silently
    if not filepath.exists():
        return 0

    # Skip non-.md files silently
    if filepath.suffix != ".md":
        return 0

    # Resolve to check if file is in a research/ directory
    resolved = filepath.resolve()
    parts = resolved.parts

    # Check if any part of the path is "research"
    if "research" not in parts:
        return 0

    # Skip README.md files silently
    if filepath.name == "README.md":
        return 0

    # Validate frontmatter
    result = validate_frontmatter(filepath)
    return result.report()


if __name__ == "__main__":
    sys.exit(main())
