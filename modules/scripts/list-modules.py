#!/usr/bin/env python3
"""
list-modules.py
Lists all available modules in the Ouaricon Module System.

Usage:
    ./list-modules.py           # List all modules
    ./list-modules.py -v        # Verbose output with descriptions
    ./list-modules.py -c core   # Filter by category
"""

import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("Error: PyYAML required. Install with: pip install pyyaml")
    sys.exit(1)


def load_yaml(path: Path) -> dict:
    """Load a YAML file."""
    with open(path, 'r') as f:
        return yaml.safe_load(f)


def main():
    # Parse args
    verbose = '-v' in sys.argv
    category_filter = None
    if '-c' in sys.argv:
        idx = sys.argv.index('-c')
        if idx + 1 < len(sys.argv):
            category_filter = sys.argv[idx + 1]

    # Find registry
    script_dir = Path(__file__).parent
    modules_dir = script_dir.parent
    registry_path = modules_dir / 'registry.yaml'

    if not registry_path.exists():
        print(f"Error: Registry not found at {registry_path}")
        sys.exit(1)

    registry = load_yaml(registry_path)

    # Print header
    print("=" * 60)
    print("Ouaricon Module System")
    print(f"Version: {registry.get('version', 'unknown')}")
    print("=" * 60)
    print()

    # Group modules by category
    by_category = {}
    for module in registry.get('modules', []):
        cat = module.get('category', 'uncategorized')
        if cat not in by_category:
            by_category[cat] = []
        by_category[cat].append(module)

    # Print modules
    for cat_info in registry.get('categories', []):
        cat_name = cat_info['name']

        if category_filter and cat_name != category_filter:
            continue

        if cat_name not in by_category:
            continue

        print(f"[{cat_name.upper()}] {cat_info.get('description', '')}")
        print("-" * 60)

        for module in by_category[cat_name]:
            name = module['name']
            version = module.get('version', '?')
            reuse = module.get('reuse_score', '?')

            print(f"  {name} (v{version}) - Reuse: {reuse}/10")

            if verbose:
                desc = module.get('description', '').strip()
                if desc:
                    # Wrap description
                    lines = desc.split('\n')
                    for line in lines[:3]:  # First 3 lines only
                        print(f"    {line.strip()}")

                provides = module.get('provides', [])
                if provides:
                    print(f"    Provides: {', '.join(str(p) for p in provides)}")

                used_by = module.get('used_by', [])
                if used_by:
                    plugins = [u.get('plugin', u) if isinstance(u, dict) else u for u in used_by]
                    print(f"    Used by: {', '.join(plugins)}")

                print()

        print()

    # Summary
    total = sum(len(m) for m in by_category.values())
    print(f"Total: {total} modules in {len(by_category)} categories")


if __name__ == '__main__':
    main()
