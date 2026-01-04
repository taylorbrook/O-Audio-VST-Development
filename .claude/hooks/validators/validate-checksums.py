#!/usr/bin/env python3
"""
Contract Checksum Validator

Verifies contract file checksums match values stored in .continue-here.md.
Called by SubagentStop hook before stage transitions during Stages 1-4.
"""

import sys
import os
import re
from pathlib import Path

# Add validators directory to path
sys.path.insert(0, str(Path(__file__).parent))

from contract_validator import ContractValidator, ValidationResult


def extract_checksums_from_continue_here(continue_here_path: Path) -> dict:
    """
    Extract contract checksums from .continue-here.md frontmatter.

    Args:
        continue_here_path: Path to .continue-here.md

    Returns:
        Dict mapping contract names to checksums
    """
    if not continue_here_path.exists():
        return {}

    with open(continue_here_path, 'r') as f:
        content = f.read()

    checksums = {}

    # Extract YAML frontmatter section
    frontmatter_match = re.search(r'^---\n(.*?)\n---', content, re.DOTALL | re.MULTILINE)
    if not frontmatter_match:
        return checksums

    frontmatter = frontmatter_match.group(1)

    # Extract contract_checksums section
    if 'contract_checksums:' not in frontmatter:
        return checksums

    # Parse checksums
    in_checksums = False
    for line in frontmatter.split('\n'):
        if line.strip() == 'contract_checksums:':
            in_checksums = True
            continue
        elif in_checksums:
            if line.startswith('  ') and ':' in line:
                # Parse "  creative_brief: sha256:..."
                key, value = line.strip().split(':', 1)
                checksums[key.strip()] = value.strip()
            else:
                # End of checksums section
                break

    return checksums


def main():
    """Run checksum validation."""

    # Get plugin path from environment or argument
    plugin_path = os.environ.get('PLUGIN_PATH')
    if not plugin_path and len(sys.argv) > 1:
        plugin_path = sys.argv[1]

    if not plugin_path:
        print("ERROR: PLUGIN_PATH not set and no argument provided", file=sys.stderr)
        return 1

    # Initialize validator
    validator = ContractValidator(plugin_path)

    # Check if we're in an immutable stage
    stage = validator.get_current_stage()
    if stage is None:
        print("No stage information found in .continue-here.md", file=sys.stderr)
        return 0

    if stage < 1 or stage > 4:
        print(f"Stage {stage}: Contract immutability not required", file=sys.stderr)
        return 0

    print(f"Stage {stage}: Validating contract checksums...", file=sys.stderr)

    # Extract stored checksums
    continue_here_path = validator.continue_here_path
    stored_checksums = extract_checksums_from_continue_here(continue_here_path)

    if not stored_checksums:
        print("WARNING: No contract checksums found in .continue-here.md", file=sys.stderr)
        print("Checksums should be calculated at Stage 0 completion", file=sys.stderr)
        return 2  # Warning level

    # Verify checksums
    results = validator.verify_checksums(stored_checksums)

    # Categorize results
    errors = []
    verified = []

    for result in results:
        if result.severity == 'error' and not result.passed:
            errors.append(result)
        elif result.passed:
            verified.append(result)

    # Print results
    if verified:
        print("\n✓ Contract checksums verified:", file=sys.stderr)
        for result in verified:
            print(f"  ✓ {result.message}", file=sys.stderr)

    if errors:
        print("\n❌ Checksum verification FAILED:", file=sys.stderr)
        for result in errors:
            print(f"  ❌ {result.message}", file=sys.stderr)
        print("\nContract files have been modified during implementation!", file=sys.stderr)
        print("This violates the immutability requirement for Stages 1-4.", file=sys.stderr)
        print("\nTo fix:", file=sys.stderr)
        print("  1. Review contract changes", file=sys.stderr)
        print("  2. Revert to original contracts (git checkout)", file=sys.stderr)
        print("  3. OR restart from Stage 1 with updated contracts", file=sys.stderr)
        return 1

    print("\nContract checksum validation PASSED", file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
