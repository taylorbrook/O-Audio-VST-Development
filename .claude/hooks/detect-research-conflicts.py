#!/usr/bin/env python3
"""
detect-research-conflicts.py
Analyzes researcher findings for contradictions and incompatible approaches.
Returns JSON with conflict details and blocking status.

Usage:
    python3 detect-research-conflicts.py --dir <findings-directory>
    python3 detect-research-conflicts.py --files <file1.json> <file2.json> ...

Exit 0: No blocking conflicts found
Exit 1: Blocking conflicts found (planning should be paused)
"""

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple


# Contradiction pairs: approaches that are fundamentally incompatible
# when recommended for the SAME processing stage or component
CONTRADICTION_PAIRS = [
    # Processing domain conflicts
    ("time-domain", "frequency-domain"),
    ("time domain", "frequency domain"),
    # Channel configuration conflicts
    ("mono processing", "stereo processing"),
    ("mono", "stereo"),
    # Latency model conflicts
    ("zero-latency", "lookahead"),
    ("zero latency", "look-ahead"),
    ("zero-latency", "look-ahead"),
    ("zero latency", "lookahead"),
    # State model conflicts
    ("stateless", "stateful"),
    # Filter architecture conflicts (audio-domain specific)
    ("fir filter", "iir filter"),
    ("fir", "iir"),
    # Processing granularity conflicts
    ("sample-by-sample", "block-based"),
    ("sample by sample", "block based"),
    ("per-sample", "block processing"),
    # SIMD requirement conflicts
    ("simd required", "scalar only"),
    ("simd", "scalar"),
    # Synthesis approach conflicts
    ("additive synthesis", "subtractive synthesis"),
    ("wavetable", "physical modeling"),
    # Reverb architecture conflicts
    ("convolution reverb", "algorithmic reverb"),
    ("convolution", "feedback delay network"),
    # Windowing conflicts
    ("overlap-add", "overlap-save"),
]


def load_findings_from_json(file_path: Path) -> Optional[Dict]:
    """Load researcher findings from a JSON file."""
    try:
        with open(file_path) as f:
            return json.load(f)
    except (json.JSONDecodeError, IOError) as e:
        print(f"WARNING: Could not load {file_path}: {e}", file=sys.stderr)
        return None


def load_findings_from_markdown(file_path: Path) -> Optional[Dict]:
    """Parse researcher findings from a Markdown file into a dict."""
    try:
        content = file_path.read_text()
    except IOError as e:
        print(f"WARNING: Could not read {file_path}: {e}", file=sys.stderr)
        return None

    finding = {
        "domain": "",
        "recommendations": [],
        "juce_modules": [],
        "approach": "",
        "confidence": "",
        "source_file": str(file_path),
    }

    # Extract domain
    domain_match = re.search(r"^## Domain\s*\n(.+)", content, re.MULTILINE)
    if domain_match:
        finding["domain"] = domain_match.group(1).strip()

    # Extract approach description
    approach_match = re.search(
        r"^## Approach Description\s*\n((?:(?!^## ).+\n?)*)",
        content,
        re.MULTILINE,
    )
    if approach_match:
        finding["approach"] = approach_match.group(1).strip()

    # Extract JUCE modules
    modules_match = re.search(
        r"^## JUCE Modules Needed\s*\n((?:(?!^## ).+\n?)*)",
        content,
        re.MULTILINE,
    )
    if modules_match:
        modules_text = modules_match.group(1)
        for line in modules_text.split("\n"):
            line = line.strip().lstrip("- ")
            if line:
                # Extract module name (e.g., "juce::dsp::FFT" from "juce::dsp::FFT - purpose")
                module_match = re.match(r"(juce::\S+)", line)
                if module_match:
                    finding["juce_modules"].append(module_match.group(1))

    # Extract recommendations
    recs_match = re.search(
        r"^## Recommendations\s*\n((?:(?!^## ).+\n?)*)",
        content,
        re.MULTILINE,
    )
    if recs_match:
        recs_text = recs_match.group(1)
        for line in recs_text.split("\n"):
            line = line.strip()
            if line and re.match(r"^\d+\.", line):
                finding["recommendations"].append(
                    re.sub(r"^\d+\.\s*", "", line)
                )

    # Extract confidence
    conf_match = re.search(r"^## Confidence Level\s*\n(.+)", content, re.MULTILINE)
    if conf_match:
        finding["confidence"] = conf_match.group(1).strip()

    return finding


def load_findings(directory: Path) -> List[Dict]:
    """Load all researcher findings from a directory."""
    findings = []

    if not directory.exists():
        print(f"ERROR: Findings directory not found: {directory}", file=sys.stderr)
        return findings

    # Try JSON files first, then Markdown
    for ext in ("*.json", "*.md"):
        for file_path in sorted(directory.glob(ext)):
            if file_path.name.startswith("."):
                continue

            if ext == "*.json":
                finding = load_findings_from_json(file_path)
            else:
                finding = load_findings_from_markdown(file_path)

            if finding:
                finding["source_file"] = str(file_path)
                findings.append(finding)

    return findings


def check_approach_contradictions(
    f1: Dict, f2: Dict
) -> List[Dict]:
    """Check two findings for incompatible approach recommendations."""
    conflicts = []

    f1_approach = f1.get("approach", "").lower()
    f2_approach = f2.get("approach", "").lower()

    # Combine approach with recommendations for broader matching
    f1_text = f1_approach + " " + " ".join(
        r.lower() for r in f1.get("recommendations", [])
    )
    f2_text = f2_approach + " " + " ".join(
        r.lower() for r in f2.get("recommendations", [])
    )

    for term_a, term_b in CONTRADICTION_PAIRS:
        a_in_f1 = term_a in f1_text
        b_in_f1 = term_b in f1_text
        a_in_f2 = term_a in f2_text
        b_in_f2 = term_b in f2_text

        # Conflict: one researcher recommends A, the other recommends B
        if (a_in_f1 and b_in_f2 and not b_in_f1) or (
            b_in_f1 and a_in_f2 and not a_in_f2
        ):
            conflicts.append(
                {
                    "type": "incompatible_approach",
                    "domain_a": f1.get("domain", "unknown"),
                    "domain_b": f2.get("domain", "unknown"),
                    "term_a": term_a,
                    "term_b": term_b,
                    "conflict": (
                        f"Researcher ({f1.get('domain', 'A')}) recommends "
                        f"'{term_a}' but Researcher ({f2.get('domain', 'B')}) "
                        f"recommends '{term_b}'"
                    ),
                    "source_a": f1.get("source_file", ""),
                    "source_b": f2.get("source_file", ""),
                    "blocking": True,
                }
            )

        # Also check the reverse direction explicitly
        if (b_in_f1 and a_in_f2 and not a_in_f1):
            conflicts.append(
                {
                    "type": "incompatible_approach",
                    "domain_a": f1.get("domain", "unknown"),
                    "domain_b": f2.get("domain", "unknown"),
                    "term_a": term_b,
                    "term_b": term_a,
                    "conflict": (
                        f"Researcher ({f1.get('domain', 'A')}) recommends "
                        f"'{term_b}' but Researcher ({f2.get('domain', 'B')}) "
                        f"recommends '{term_a}'"
                    ),
                    "source_a": f1.get("source_file", ""),
                    "source_b": f2.get("source_file", ""),
                    "blocking": True,
                }
            )

    return conflicts


def check_module_conflicts(f1: Dict, f2: Dict) -> List[Dict]:
    """Check for JUCE module conflicts between findings."""
    conflicts = []

    f1_modules = set(f1.get("juce_modules", []))
    f2_modules = set(f2.get("juce_modules", []))

    # Shared modules are fine -- it means agreement
    # Conflicts arise when both researchers mention the same processing need
    # but recommend different modules for it

    # Check for known incompatible module pairs
    incompatible_module_pairs = [
        # FFT implementations
        ("juce::dsp::FFT", "custom FFT"),
        # Reverb implementations
        ("juce::dsp::Reverb", "custom reverb"),
    ]

    f1_module_text = " ".join(f1_modules).lower()
    f2_module_text = " ".join(f2_modules).lower()

    for mod_a, mod_b in incompatible_module_pairs:
        if mod_a.lower() in f1_module_text and mod_b.lower() in f2_module_text:
            conflicts.append(
                {
                    "type": "module_conflict",
                    "domain_a": f1.get("domain", "unknown"),
                    "domain_b": f2.get("domain", "unknown"),
                    "module_a": mod_a,
                    "module_b": mod_b,
                    "conflict": (
                        f"Researcher ({f1.get('domain', 'A')}) uses {mod_a} "
                        f"but Researcher ({f2.get('domain', 'B')}) uses {mod_b}"
                    ),
                    "source_a": f1.get("source_file", ""),
                    "source_b": f2.get("source_file", ""),
                    "blocking": False,  # Module conflicts are warnings, not blockers
                }
            )

    return conflicts


def detect_conflicts(findings: List[Dict]) -> Dict:
    """
    Compare all researcher findings for incompatible approaches.

    Args:
        findings: List of finding dicts with keys:
            - domain: str
            - recommendations: list[str]
            - juce_modules: list[str]
            - approach: str
            - confidence: str

    Returns:
        Dict with:
            - conflicts: list of conflict dicts
            - blocking: bool (True if any blocking conflicts)
            - summary: str (human-readable summary)
    """
    all_conflicts = []

    for i, f1 in enumerate(findings):
        for f2 in findings[i + 1 :]:
            # Check approach contradictions
            approach_conflicts = check_approach_contradictions(f1, f2)
            all_conflicts.extend(approach_conflicts)

            # Check module conflicts
            module_conflicts = check_module_conflicts(f1, f2)
            all_conflicts.extend(module_conflicts)

    # Determine if any conflicts are blocking
    has_blocking = any(c.get("blocking", False) for c in all_conflicts)

    # Generate summary
    if not all_conflicts:
        summary = "No conflicts detected between researcher findings."
    else:
        blocking_count = sum(1 for c in all_conflicts if c.get("blocking"))
        warning_count = len(all_conflicts) - blocking_count
        parts = []
        if blocking_count:
            parts.append(f"{blocking_count} blocking conflict(s)")
        if warning_count:
            parts.append(f"{warning_count} warning(s)")
        summary = f"Found {', '.join(parts)} across {len(findings)} researcher findings."

    return {
        "conflicts": all_conflicts,
        "blocking": has_blocking,
        "findings_count": len(findings),
        "summary": summary,
    }


def main():
    parser = argparse.ArgumentParser(
        description="Detect conflicts between researcher findings"
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "--dir", type=Path, help="Directory containing findings files"
    )
    group.add_argument(
        "--files", nargs="+", type=Path, help="Individual findings files"
    )
    parser.add_argument(
        "--json", action="store_true", help="Output as JSON (default)"
    )

    args = parser.parse_args()

    # Load findings
    if args.dir:
        findings = load_findings(args.dir)
    else:
        findings = []
        for fp in args.files:
            if fp.suffix == ".json":
                f = load_findings_from_json(fp)
            else:
                f = load_findings_from_markdown(fp)
            if f:
                f["source_file"] = str(fp)
                findings.append(f)

    if len(findings) < 2:
        result = {
            "conflicts": [],
            "blocking": False,
            "findings_count": len(findings),
            "summary": f"Only {len(findings)} finding(s) loaded. Need at least 2 for conflict detection.",
        }
        print(json.dumps(result, indent=2))
        sys.exit(0)

    # Detect conflicts
    result = detect_conflicts(findings)

    # Output
    print(json.dumps(result, indent=2))

    # Exit code based on blocking status
    if result["blocking"]:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()
