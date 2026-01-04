#!/usr/bin/env python3
"""
Cross-Contract Consistency Validator

Validates consistency across plugin contract files:
- creative-brief.md
- parameter-spec.md
- architecture.md
- plan.md

Called by SubagentStop hook before stage transitions.
"""

import sys
import os
from pathlib import Path

# Add validators directory to path
sys.path.insert(0, str(Path(__file__).parent))

from contract_validator import ContractValidator, ValidationResult


def main():
    """Run cross-contract validation."""

    # Get plugin path from environment or argument
    plugin_path = os.environ.get('PLUGIN_PATH')
    if not plugin_path and len(sys.argv) > 1:
        plugin_path = sys.argv[1]

    if not plugin_path:
        print("ERROR: PLUGIN_PATH not set and no argument provided", file=sys.stderr)
        return 1

    # Initialize validator
    validator = ContractValidator(plugin_path)

    # Run cross-contract validation
    print("Validating cross-contract consistency...", file=sys.stderr)
    results = validator.validate_cross_contract_consistency()

    # Categorize results
    errors = []
    warnings = []
    info = []

    for result in results:
        if result.severity == 'error' and not result.passed:
            errors.append(result)
        elif result.severity == 'warning' and not result.passed:
            warnings.append(result)
        elif result.passed and result.severity == 'info':
            info.append(result)

    # Print results
    if info:
        print("\n✓ Cross-contract validation checks:", file=sys.stderr)
        for result in info:
            print(f"  ✓ {result.message}", file=sys.stderr)

    if warnings:
        print("\n⚠️  Warnings:", file=sys.stderr)
        for result in warnings:
            print(f"  ⚠️  {result.message}", file=sys.stderr)

    if errors:
        print("\n❌ Errors:", file=sys.stderr)
        for result in errors:
            print(f"  ❌ {result.message}", file=sys.stderr)
        print("\nCross-contract validation FAILED", file=sys.stderr)
        print("Fix inconsistencies before proceeding to next stage", file=sys.stderr)
        return 1

    if warnings:
        print("\nCross-contract validation PASSED with warnings", file=sys.stderr)
        return 2  # Warning level

    print("\nCross-contract validation PASSED", file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
