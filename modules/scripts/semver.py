#!/usr/bin/env python3
"""
Semver parsing and comparison utility.

Implements npm/node-semver semantics for version comparison, range checking,
and update classification (major/minor/patch).

Usage:
    # Compare two versions
    python3 semver.py compare 1.0.0 1.2.0
    # Output: -1 (first is older)

    # Check if update is major
    python3 semver.py is-major 1.0.0 2.0.0
    # Output: true

    # Check range satisfaction
    python3 semver.py satisfies 1.2.3 "^1.0.0"
    # Output: true

Exit codes:
    0: Success
    1: Comparison result / false result
    2: Error (invalid arguments, invalid version format)
"""

import re
import sys
from typing import Tuple, Optional


def parse_version(v: str) -> Tuple[int, int, int, Optional[str]]:
    """
    Parse semver string into (major, minor, patch, prerelease).

    Args:
        v: Version string like "1.2.3" or "1.2.3-alpha.1"

    Returns:
        Tuple of (major: int, minor: int, patch: int, prerelease: str | None)

    Raises:
        ValueError: If version string is invalid
    """
    # Match: major.minor.patch with optional prerelease
    match = re.match(r'^(\d+)\.(\d+)\.(\d+)(?:-([a-zA-Z0-9][a-zA-Z0-9.\-]*))?$', v)
    if not match:
        raise ValueError(f"Invalid semver: {v}")

    major = int(match.group(1))
    minor = int(match.group(2))
    patch = int(match.group(3))
    prerelease = match.group(4)

    return (major, minor, patch, prerelease)


def compare_prerelease(pr1: Optional[str], pr2: Optional[str]) -> int:
    """
    Compare prerelease identifiers per semver spec.

    Rules:
    - No prerelease > has prerelease (1.0.0 > 1.0.0-alpha)
    - Compare identifiers left to right
    - Numeric identifiers compare numerically
    - Alphanumeric identifiers compare lexically
    - Numeric < alphanumeric
    - Fewer identifiers < more identifiers (if all preceding are equal)

    Returns:
        -1 if pr1 < pr2, 0 if equal, 1 if pr1 > pr2
    """
    # No prerelease beats any prerelease
    if pr1 is None and pr2 is not None:
        return 1
    if pr1 is not None and pr2 is None:
        return -1
    if pr1 is None and pr2 is None:
        return 0

    # Split into identifiers
    ids1 = pr1.split('.')
    ids2 = pr2.split('.')

    # Compare identifier by identifier
    for i in range(min(len(ids1), len(ids2))):
        id1 = ids1[i]
        id2 = ids2[i]

        # Check if identifiers are numeric
        is_num1 = id1.isdigit()
        is_num2 = id2.isdigit()

        if is_num1 and is_num2:
            # Both numeric: compare as integers
            n1, n2 = int(id1), int(id2)
            if n1 < n2:
                return -1
            if n1 > n2:
                return 1
        elif is_num1:
            # Numeric < alphanumeric
            return -1
        elif is_num2:
            # Alphanumeric > numeric
            return 1
        else:
            # Both alphanumeric: lexical comparison
            if id1 < id2:
                return -1
            if id1 > id2:
                return 1

    # All compared identifiers are equal; more identifiers wins
    if len(ids1) < len(ids2):
        return -1
    if len(ids1) > len(ids2):
        return 1

    return 0


def compare_versions(v1: str, v2: str) -> int:
    """
    Compare two semver versions.

    Args:
        v1: First version string
        v2: Second version string

    Returns:
        -1 if v1 < v2
         0 if v1 == v2
         1 if v1 > v2

    Raises:
        ValueError: If either version is invalid
    """
    p1 = parse_version(v1)
    p2 = parse_version(v2)

    # Compare major.minor.patch
    for i in range(3):
        if p1[i] < p2[i]:
            return -1
        if p1[i] > p2[i]:
            return 1

    # Compare prerelease
    return compare_prerelease(p1[3], p2[3])


def is_major_update(old: str, new: str) -> bool:
    """
    Check if update is a breaking change (major version bump).

    Args:
        old: Currently installed version
        new: Available version

    Returns:
        True if new.major > old.major
    """
    p_old = parse_version(old)
    p_new = parse_version(new)
    return p_new[0] > p_old[0]


def is_compatible(installed: str, available: str) -> bool:
    """
    Check if an update is available.

    Args:
        installed: Currently installed version
        available: Available version

    Returns:
        True if available version is higher than installed
    """
    return compare_versions(installed, available) < 0


def satisfies_range(version: str, range_spec: str) -> bool:
    """
    Check if version satisfies a range specification.

    Supports:
        ^1.2.0 - Caret: allow minor/patch updates within same major
        ~1.2.0 - Tilde: allow only patch updates within same minor
        >=1.0.0 - Greater than or equal
        >1.0.0 - Greater than
        <=1.0.0 - Less than or equal
        <1.0.0 - Less than
        1.0.0 - Exact match

    Args:
        version: Version to check
        range_spec: Range specification

    Returns:
        True if version satisfies the range
    """
    v = parse_version(version)

    # Caret (^): Allow minor and patch updates within same major
    if range_spec.startswith('^'):
        base = parse_version(range_spec[1:])
        # Must have same major
        if v[0] != base[0]:
            return False
        # Must be >= base
        return (v[1], v[2]) >= (base[1], base[2])

    # Tilde (~): Allow patch updates only within same minor
    if range_spec.startswith('~'):
        base = parse_version(range_spec[1:])
        # Must have same major and minor
        if v[0] != base[0] or v[1] != base[1]:
            return False
        # Must be >= base patch
        return v[2] >= base[2]

    # Greater than or equal (>=)
    if range_spec.startswith('>='):
        base = range_spec[2:]
        return compare_versions(version, base) >= 0

    # Greater than (>)
    if range_spec.startswith('>'):
        base = range_spec[1:]
        return compare_versions(version, base) > 0

    # Less than or equal (<=)
    if range_spec.startswith('<='):
        base = range_spec[2:]
        return compare_versions(version, base) <= 0

    # Less than (<)
    if range_spec.startswith('<'):
        base = range_spec[1:]
        return compare_versions(version, base) < 0

    # Exact match
    return compare_versions(version, range_spec) == 0


def main():
    """CLI interface for semver operations."""
    if len(sys.argv) < 2:
        print("Usage: semver.py <command> [args]", file=sys.stderr)
        print("Commands:", file=sys.stderr)
        print("  compare <v1> <v2>     Compare two versions (-1, 0, 1)", file=sys.stderr)
        print("  is-major <old> <new>  Check if update is major (true/false)", file=sys.stderr)
        print("  satisfies <v> <range> Check if version satisfies range", file=sys.stderr)
        print("  parse <version>       Parse version and show components", file=sys.stderr)
        sys.exit(2)

    command = sys.argv[1]

    try:
        if command == 'compare':
            if len(sys.argv) < 4:
                print("Usage: semver.py compare <v1> <v2>", file=sys.stderr)
                sys.exit(2)
            result = compare_versions(sys.argv[2], sys.argv[3])
            print(result)
            sys.exit(0)

        elif command == 'is-major':
            if len(sys.argv) < 4:
                print("Usage: semver.py is-major <old> <new>", file=sys.stderr)
                sys.exit(2)
            result = is_major_update(sys.argv[2], sys.argv[3])
            print("true" if result else "false")
            sys.exit(0 if result else 1)

        elif command == 'satisfies':
            if len(sys.argv) < 4:
                print("Usage: semver.py satisfies <version> <range>", file=sys.stderr)
                sys.exit(2)
            result = satisfies_range(sys.argv[2], sys.argv[3])
            print("true" if result else "false")
            sys.exit(0 if result else 1)

        elif command == 'parse':
            if len(sys.argv) < 3:
                print("Usage: semver.py parse <version>", file=sys.stderr)
                sys.exit(2)
            major, minor, patch, prerelease = parse_version(sys.argv[2])
            print(f"major: {major}")
            print(f"minor: {minor}")
            print(f"patch: {patch}")
            print(f"prerelease: {prerelease if prerelease else 'none'}")
            sys.exit(0)

        else:
            print(f"Unknown command: {command}", file=sys.stderr)
            sys.exit(2)

    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(2)


if __name__ == '__main__':
    main()
