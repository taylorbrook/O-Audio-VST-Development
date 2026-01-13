#!/usr/bin/env python3
"""
check-updates.py
Checks all plugins for available module updates.

Usage:
    ./check-updates.py                    # Check all plugins
    ./check-updates.py OuariconMarimba    # Check specific plugin
"""

import sys
import os
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


def get_registry_versions(registry_path: Path) -> dict[str, str]:
    """Get current versions of all modules from registry."""
    registry = load_yaml(registry_path)
    versions = {}
    for module in registry.get('modules', []):
        versions[module['name']] = module['version']
    return versions


def get_locked_versions(lock_path: Path) -> dict[str, str]:
    """Get locked module versions from a plugin."""
    if not lock_path.exists():
        return {}
    lock_data = load_yaml(lock_path)
    return lock_data.get('locked_versions', {})


def compare_versions(v1: str, v2: str) -> int:
    """
    Compare semantic versions.
    Returns: -1 if v1 < v2, 0 if equal, 1 if v1 > v2
    """
    def parse(v):
        return tuple(int(x) for x in v.split('.'))

    p1, p2 = parse(v1), parse(v2)
    if p1 < p2:
        return -1
    elif p1 > p2:
        return 1
    return 0


def check_plugin_updates(plugin_path: Path, registry_versions: dict[str, str]) -> list[dict]:
    """Check a single plugin for module updates."""
    lock_path = plugin_path / 'modules.lock.yaml'
    locked = get_locked_versions(lock_path)

    updates = []
    for module_name, locked_version in locked.items():
        if module_name in registry_versions:
            current_version = registry_versions[module_name]
            if compare_versions(locked_version, current_version) < 0:
                updates.append({
                    'module': module_name,
                    'locked': locked_version,
                    'available': current_version
                })

    return updates


def main():
    # Find paths
    script_dir = Path(__file__).parent
    modules_dir = script_dir.parent
    project_root = modules_dir.parent
    plugins_dir = project_root / 'plugins'
    registry_path = modules_dir / 'registry.yaml'

    if not registry_path.exists():
        print(f"Error: Registry not found at {registry_path}")
        sys.exit(1)

    # Load registry versions
    registry_versions = get_registry_versions(registry_path)
    print(f"Loaded {len(registry_versions)} modules from registry\n")

    # Determine which plugins to check
    if len(sys.argv) > 1:
        plugin_names = sys.argv[1:]
    else:
        # Check all plugins
        plugin_names = [p.name for p in plugins_dir.iterdir() if p.is_dir()]

    # Check each plugin
    any_updates = False
    for plugin_name in plugin_names:
        plugin_path = plugins_dir / plugin_name
        if not plugin_path.exists():
            print(f"Warning: Plugin not found: {plugin_name}")
            continue

        lock_file = plugin_path / 'modules.lock.yaml'
        if not lock_file.exists():
            print(f"{plugin_name}: No modules.lock.yaml (no modules installed)")
            continue

        updates = check_plugin_updates(plugin_path, registry_versions)

        if updates:
            any_updates = True
            print(f"{plugin_name}:")
            for u in updates:
                print(f"  {u['module']}: {u['locked']} -> {u['available']}")
        else:
            locked = get_locked_versions(lock_file)
            if locked:
                print(f"{plugin_name}: All {len(locked)} modules up to date")
            else:
                print(f"{plugin_name}: No modules locked")

    if not any_updates:
        print("\nAll plugins are up to date!")
    else:
        print("\nRun 'upgrade-module.py <plugin> <module>' to upgrade")


if __name__ == '__main__':
    main()
