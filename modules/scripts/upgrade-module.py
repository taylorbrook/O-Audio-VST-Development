#!/usr/bin/env python3
"""
upgrade-module.py
Upgrades a module in a plugin to the latest version.

Usage:
    ./upgrade-module.py OuariconMarimba preset-manager
"""

import sys
import os
from pathlib import Path
from datetime import datetime

try:
    import yaml
except ImportError:
    print("Error: PyYAML required. Install with: pip install pyyaml")
    sys.exit(1)


def load_yaml(path: Path) -> dict:
    """Load a YAML file."""
    with open(path, 'r') as f:
        return yaml.safe_load(f)


def save_yaml(path: Path, data: dict):
    """Save a YAML file."""
    with open(path, 'w') as f:
        yaml.dump(data, f, default_flow_style=False, sort_keys=False)


def get_module_info(registry_path: Path, module_name: str) -> dict | None:
    """Get module info from registry."""
    registry = load_yaml(registry_path)
    for module in registry.get('modules', []):
        if module['name'] == module_name:
            return module
    return None


def upgrade_module(plugin_path: Path, module_name: str, new_version: str):
    """Update the lock file with new module version."""
    lock_path = plugin_path / 'modules.lock.yaml'

    if lock_path.exists():
        lock_data = load_yaml(lock_path)
    else:
        lock_data = {'locked_versions': {}}

    old_version = lock_data.get('locked_versions', {}).get(module_name, 'none')
    lock_data['locked_versions'][module_name] = new_version
    lock_data['last_updated'] = datetime.now().strftime('%Y-%m-%d')

    save_yaml(lock_path, lock_data)
    return old_version


def main():
    if len(sys.argv) < 3:
        print("Usage: upgrade-module.py <plugin-name> <module-name>")
        print("Example: upgrade-module.py OuariconMarimba preset-manager")
        sys.exit(1)

    plugin_name = sys.argv[1]
    module_name = sys.argv[2]

    # Find paths
    script_dir = Path(__file__).parent
    modules_dir = script_dir.parent
    project_root = modules_dir.parent
    plugins_dir = project_root / 'plugins'
    registry_path = modules_dir / 'registry.yaml'

    # Validate plugin exists
    plugin_path = plugins_dir / plugin_name
    if not plugin_path.exists():
        print(f"Error: Plugin not found: {plugin_path}")
        sys.exit(1)

    # Get module info from registry
    module_info = get_module_info(registry_path, module_name)
    if not module_info:
        print(f"Error: Module '{module_name}' not found in registry")
        sys.exit(1)

    new_version = module_info['version']

    # Check current version
    lock_path = plugin_path / 'modules.lock.yaml'
    if lock_path.exists():
        lock_data = load_yaml(lock_path)
        current_version = lock_data.get('locked_versions', {}).get(module_name)
        if current_version == new_version:
            print(f"{module_name} is already at version {new_version}")
            return
    else:
        current_version = None

    # Perform upgrade
    old_version = upgrade_module(plugin_path, module_name, new_version)

    if old_version == 'none':
        print(f"Added {module_name} v{new_version} to {plugin_name}")
    else:
        print(f"Upgraded {module_name} in {plugin_name}: {old_version} -> {new_version}")

    # Check for migration guide
    module_path = modules_dir / module_info['path']
    migrations_dir = module_path / 'migrations'
    if migrations_dir.exists() and old_version != 'none':
        migration_file = migrations_dir / f"v{old_version.replace('.', '_')}_to_v{new_version.replace('.', '_')}.md"
        if migration_file.exists():
            print(f"\nMigration guide available: {migration_file}")
            print("Please review before rebuilding.")

    print(f"\nNext steps:")
    print(f"  1. Review any migration guides")
    print(f"  2. Rebuild the plugin: cmake --build build")
    print(f"  3. Test the plugin")


if __name__ == '__main__':
    main()
