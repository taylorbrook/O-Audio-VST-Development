#!/usr/bin/env python3
"""
Compute SHA-256 content hash for a module directory.

Usage:
    ./compute-hash.py modules/tuning/scala-tuning-engine
    ./compute-hash.py modules/tuning/scala-tuning-engine --verify sha256:a1b2c3d4e5f6g7h8

The hash is computed deterministically from:
1. All source files (sorted by relative path)
2. Each file contributes: relative_path + file_contents

This ensures renamed files produce different hashes.

Exit codes:
    0: Success (or verification passed)
    1: Verification failed (hash mismatch)
    2: Error (invalid arguments, missing path, etc.)
"""

import hashlib
import sys
from pathlib import Path


# File extensions to include in hash computation
INCLUDE_EXTENSIONS = {
    '.cpp', '.h', '.hpp',  # C++ sources
    '.js', '.ts',          # JavaScript/TypeScript
    '.yaml', '.json',      # Config files
    '.md',                 # Documentation
}

# Directories to exclude
EXCLUDE_DIRS = {
    '__pycache__',
    'node_modules',
    '.git',
}


def compute_module_hash(module_path: Path) -> str:
    """
    Compute SHA-256 hash of all module source files.

    Args:
        module_path: Path to module directory

    Returns:
        Hash string in format "sha256:{first 16 hex characters}"
    """
    hasher = hashlib.sha256()

    # Get all files, sorted for deterministic ordering
    all_files = sorted(module_path.rglob("*"))

    for file_path in all_files:
        # Skip directories
        if not file_path.is_file():
            continue

        # Skip hidden files (starting with .)
        if file_path.name.startswith('.'):
            continue

        # Skip excluded directories
        if any(excluded in file_path.parts for excluded in EXCLUDE_DIRS):
            continue

        # Only include files with matching extensions
        if file_path.suffix not in INCLUDE_EXTENSIONS:
            continue

        # Hash relative path first (ensures renamed files produce different hashes)
        rel_path = str(file_path.relative_to(module_path))
        hasher.update(rel_path.encode('utf-8'))

        # Hash file contents
        hasher.update(file_path.read_bytes())

    return f"sha256:{hasher.hexdigest()[:16]}"


def main():
    # Parse arguments
    if len(sys.argv) < 2:
        print("Usage: compute-hash.py <module_path> [--verify <hash>]", file=sys.stderr)
        sys.exit(2)

    module_path = Path(sys.argv[1])
    verify_hash = None

    # Check for --verify flag
    if len(sys.argv) >= 4 and sys.argv[2] == '--verify':
        verify_hash = sys.argv[3]

    # Validate module path
    if not module_path.exists():
        print(f"Error: Module path does not exist: {module_path}", file=sys.stderr)
        sys.exit(2)

    if not module_path.is_dir():
        print(f"Error: Module path is not a directory: {module_path}", file=sys.stderr)
        sys.exit(2)

    # Compute hash
    computed_hash = compute_module_hash(module_path)

    # Verification mode
    if verify_hash:
        if computed_hash == verify_hash:
            print(f"Verification passed: {computed_hash}")
            sys.exit(0)
        else:
            print(f"Verification failed:", file=sys.stderr)
            print(f"  Expected: {verify_hash}", file=sys.stderr)
            print(f"  Computed: {computed_hash}", file=sys.stderr)
            sys.exit(1)

    # Normal mode: print hash
    print(computed_hash)
    sys.exit(0)


if __name__ == '__main__':
    main()
