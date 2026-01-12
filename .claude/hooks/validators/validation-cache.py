#!/usr/bin/env python3
"""
Validation Cache Manager
Implements hash-based caching to skip redundant pluginval runs.

Usage:
  validation-cache.py check <plugin_name> <stage>   # Check if cached result exists
  validation-cache.py store <plugin_name> <stage> <result> [summary]  # Store result
  validation-cache.py clear <plugin_name> [stage]  # Clear cache (all or specific stage)
  validation-cache.py status <plugin_name>         # Show cache status

Exit codes for 'check':
  0 = Cache HIT (cached PASS result, can skip validation)
  1 = Cache MISS (no cache or files changed, must validate)
  2 = Cache HIT but FAIL (previous validation failed, must re-validate after fix)
"""

import hashlib
import json
import os
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# Files to hash for each stage
STAGE_FILES = {
    1: [
        "CMakeLists.txt",
        "Source/PluginProcessor.h",
        "Source/PluginProcessor.cpp",
    ],
    2: [
        "Source/PluginProcessor.h",
        "Source/PluginProcessor.cpp",
    ],
    3: [
        "Source/PluginEditor.h",
        "Source/PluginEditor.cpp",
        "CMakeLists.txt",
        "Source/ui/public/index.html",
        "Source/ui/public/js/main.js",
    ]
}

# Additional glob patterns for each stage
STAGE_GLOBS = {
    1: [],
    2: ["Source/**/*.cpp", "Source/**/*.h"],
    3: ["Source/ui/public/**/*"]
}


def get_cache_dir(plugin_name: str) -> Path:
    """Get cache directory for a plugin"""
    return Path("plugins") / plugin_name / ".validation-cache"


def get_cache_file(plugin_name: str, stage: int) -> Path:
    """Get cache file path for a specific stage"""
    return get_cache_dir(plugin_name) / f"stage-{stage}.json"


def compute_file_hash(file_path: Path) -> Optional[str]:
    """Compute MD5 hash of a file"""
    if not file_path.exists():
        return None
    try:
        content = file_path.read_bytes()
        return hashlib.md5(content).hexdigest()
    except (PermissionError, OSError):
        return None


def compute_stage_hash(plugin_dir: Path, stage: int) -> Tuple[str, List[str]]:
    """Compute combined hash for all files relevant to a stage"""
    files_to_hash = []
    hashes = []

    # Add explicit files
    for rel_path in STAGE_FILES.get(stage, []):
        file_path = plugin_dir / rel_path
        if file_path.exists():
            files_to_hash.append(rel_path)
            h = compute_file_hash(file_path)
            if h:
                hashes.append(h)

    # Add glob patterns
    for pattern in STAGE_GLOBS.get(stage, []):
        for file_path in plugin_dir.glob(pattern):
            if file_path.is_file():
                rel_path = str(file_path.relative_to(plugin_dir))
                if rel_path not in files_to_hash:
                    files_to_hash.append(rel_path)
                    h = compute_file_hash(file_path)
                    if h:
                        hashes.append(h)

    # Combine all hashes
    if hashes:
        combined = hashlib.md5("".join(sorted(hashes)).encode()).hexdigest()
    else:
        combined = "empty"

    return combined, sorted(files_to_hash)


def load_cache(plugin_name: str, stage: int) -> Optional[Dict]:
    """Load cached result for a stage"""
    cache_file = get_cache_file(plugin_name, stage)
    if not cache_file.exists():
        return None
    try:
        return json.loads(cache_file.read_text())
    except (json.JSONDecodeError, OSError):
        return None


def save_cache(plugin_name: str, stage: int, result: str, summary: str = "") -> None:
    """Save validation result to cache"""
    plugin_dir = Path("plugins") / plugin_name
    if not plugin_dir.exists():
        print(f"Plugin directory not found: {plugin_dir}", file=sys.stderr)
        return

    cache_dir = get_cache_dir(plugin_name)
    cache_dir.mkdir(parents=True, exist_ok=True)

    combined_hash, files_hashed = compute_stage_hash(plugin_dir, stage)

    cache_data = {
        "hash": combined_hash,
        "timestamp": datetime.now().isoformat(),
        "result": result.upper(),
        "files_hashed": files_hashed,
        "pluginval_summary": summary,
        "stage": stage
    }

    cache_file = get_cache_file(plugin_name, stage)
    cache_file.write_text(json.dumps(cache_data, indent=2))
    print(f"Cached Stage {stage} result: {result.upper()}")


def check_cache(plugin_name: str, stage: int) -> int:
    """
    Check if cached validation result is still valid.
    Returns: 0=HIT+PASS, 1=MISS, 2=HIT+FAIL
    """
    plugin_dir = Path("plugins") / plugin_name
    if not plugin_dir.exists():
        print(f"Plugin not found: {plugin_name}", file=sys.stderr)
        return 1

    cached = load_cache(plugin_name, stage)
    if not cached:
        print(f"Stage {stage}: No cache found (MISS)")
        return 1

    current_hash, current_files = compute_stage_hash(plugin_dir, stage)

    if current_hash != cached.get("hash"):
        changed_since = cached.get("timestamp", "unknown")
        print(f"Stage {stage}: Files changed since {changed_since} (MISS)")
        return 1

    result = cached.get("result", "").upper()
    cached_time = cached.get("timestamp", "")
    summary = cached.get("pluginval_summary", "")

    if result == "PASS":
        print(f"Stage {stage}: Cache HIT (PASS from {cached_time})")
        if summary:
            print(f"  Summary: {summary}")
        return 0
    else:
        print(f"Stage {stage}: Cache HIT but previous result was {result}")
        print("  Must re-validate after fixes")
        return 2


def clear_cache(plugin_name: str, stage: Optional[int] = None) -> None:
    """Clear cache for a plugin (all stages or specific stage)"""
    cache_dir = get_cache_dir(plugin_name)
    if not cache_dir.exists():
        print(f"No cache found for {plugin_name}")
        return

    if stage is not None:
        cache_file = get_cache_file(plugin_name, stage)
        if cache_file.exists():
            cache_file.unlink()
            print(f"Cleared Stage {stage} cache for {plugin_name}")
        else:
            print(f"No Stage {stage} cache found for {plugin_name}")
    else:
        import shutil
        shutil.rmtree(cache_dir)
        print(f"Cleared all validation cache for {plugin_name}")


def show_status(plugin_name: str) -> None:
    """Show cache status for a plugin"""
    cache_dir = get_cache_dir(plugin_name)

    print(f"━━━ Validation Cache Status: {plugin_name} ━━━")

    if not cache_dir.exists():
        print("No cache found")
        return

    for stage in [1, 2, 3]:
        cached = load_cache(plugin_name, stage)
        if cached:
            result = cached.get("result", "?")
            timestamp = cached.get("timestamp", "?")[:19]
            files_count = len(cached.get("files_hashed", []))
            summary = cached.get("pluginval_summary", "")[:50]

            status = "PASS" if result == "PASS" else "FAIL"
            print(f"  Stage {stage}: {status} ({timestamp}) - {files_count} files")
            if summary:
                print(f"           {summary}")
        else:
            print(f"  Stage {stage}: Not cached")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    command = sys.argv[1]

    if command == "check":
        if len(sys.argv) < 4:
            print("Usage: validation-cache.py check <plugin_name> <stage>", file=sys.stderr)
            sys.exit(1)
        plugin_name = sys.argv[2]
        stage = int(sys.argv[3])
        exit_code = check_cache(plugin_name, stage)
        sys.exit(exit_code)

    elif command == "store":
        if len(sys.argv) < 5:
            print("Usage: validation-cache.py store <plugin_name> <stage> <result> [summary]", file=sys.stderr)
            sys.exit(1)
        plugin_name = sys.argv[2]
        stage = int(sys.argv[3])
        result = sys.argv[4]
        summary = sys.argv[5] if len(sys.argv) > 5 else ""
        save_cache(plugin_name, stage, result, summary)

    elif command == "clear":
        if len(sys.argv) < 3:
            print("Usage: validation-cache.py clear <plugin_name> [stage]", file=sys.stderr)
            sys.exit(1)
        plugin_name = sys.argv[2]
        stage = int(sys.argv[3]) if len(sys.argv) > 3 else None
        clear_cache(plugin_name, stage)

    elif command == "status":
        if len(sys.argv) < 3:
            print("Usage: validation-cache.py status <plugin_name>", file=sys.stderr)
            sys.exit(1)
        plugin_name = sys.argv[2]
        show_status(plugin_name)

    else:
        print(f"Unknown command: {command}", file=sys.stderr)
        print(__doc__)
        sys.exit(1)


if __name__ == "__main__":
    main()
