#!/usr/bin/env python3
"""
Migrate registry.json to v3.0.0 with module tracking.

This script:
1. Reads modules/registry.yaml and .planning/workflow/registry.json
2. Builds the modules section from registry.yaml data
3. Updates plugin entries to use InstalledModule objects
4. Bumps version to 3.0.0
5. Writes updated registry.json with atomic write pattern

Usage:
    python3 modules/scripts/migrate-registry.py

IMPORTANT: Does NOT auto-repair or silently fix issues.
If YAML parsing fails or data is inconsistent, prints clear error and exits.
"""

import json
import os
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path

# Try to import PyYAML
try:
    import yaml
except ImportError:
    print("ERROR: PyYAML is required. Install with: pip install pyyaml", file=sys.stderr)
    sys.exit(1)


def load_yaml(path: Path) -> dict:
    """Load YAML file and return dict."""
    if not path.exists():
        print(f"ERROR: YAML file not found: {path}", file=sys.stderr)
        sys.exit(1)

    try:
        with open(path, 'r') as f:
            return yaml.safe_load(f)
    except yaml.YAMLError as e:
        print(f"ERROR: Failed to parse YAML: {path}", file=sys.stderr)
        print(f"  {e}", file=sys.stderr)
        sys.exit(1)


def load_json(path: Path) -> dict:
    """Load JSON file and return dict."""
    if not path.exists():
        print(f"ERROR: JSON file not found: {path}", file=sys.stderr)
        sys.exit(1)

    try:
        with open(path, 'r') as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        print(f"ERROR: Failed to parse JSON: {path}", file=sys.stderr)
        print(f"  {e}", file=sys.stderr)
        sys.exit(1)


def build_dependents_from_used_by(module: dict) -> list:
    """Extract plugin names from used_by array."""
    used_by = module.get('used_by', [])
    if not used_by:
        return []

    # used_by is a list of dicts with 'plugin' and 'version' keys
    return [entry.get('plugin', '') for entry in used_by if isinstance(entry, dict) and entry.get('plugin')]


def clean_description(desc: str) -> str:
    """Clean multi-line description to single line."""
    if not desc:
        return ""
    # Replace newlines with spaces and collapse multiple spaces
    return ' '.join(desc.split())


def build_module_entry(module: dict, now_iso: str) -> dict:
    """Build a ModuleEntry from registry.yaml module data."""
    dependents = build_dependents_from_used_by(module)

    return {
        "version": module.get('version', '1.0.0'),
        "path": f"modules/{module.get('path', module.get('name', 'unknown'))}",
        "category": module.get('category', 'unknown'),
        "description": clean_description(module.get('description', '')),
        "author": "Ouaricon Audio",
        "changelogUrl": None,
        "compatibilityNotes": None,
        "dependents": dependents,
        "lastUpdated": now_iso,
        "usageStats": {
            "addCount": len(dependents),
            "removeCount": 0
        }
    }


def atomic_write_json(path: Path, data: dict) -> None:
    """Write JSON file atomically using temp file + rename."""
    # Write to temp file in same directory
    fd, temp_path = tempfile.mkstemp(dir=path.parent, suffix='.json.tmp')
    try:
        with os.fdopen(fd, 'w') as f:
            json.dump(data, f, indent=2)
            f.write('\n')  # Trailing newline
        # Atomic rename
        os.rename(temp_path, path)
    except Exception:
        # Clean up temp file on error
        if os.path.exists(temp_path):
            os.unlink(temp_path)
        raise


def migrate_registry(project_root: Path) -> None:
    """Main migration logic."""
    yaml_path = project_root / 'modules' / 'registry.yaml'
    json_path = project_root / '.planning' / 'workflow' / 'registry.json'

    print(f"Loading: {yaml_path}")
    yaml_data = load_yaml(yaml_path)

    print(f"Loading: {json_path}")
    registry = load_json(json_path)

    # Check current version
    current_version = registry.get('version', '1.0.0')
    print(f"Current registry version: {current_version}")

    if current_version == '3.0.0':
        print("Registry already at v3.0.0, no migration needed.")
        return

    # Get current ISO timestamp
    now_iso = datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')

    # Extract modules from YAML
    yaml_modules = yaml_data.get('modules', [])
    if not yaml_modules:
        print("ERROR: No modules found in registry.yaml", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(yaml_modules)} modules in registry.yaml")

    # Build modules section
    modules_section = {}
    for module in yaml_modules:
        name = module.get('name')
        if not name:
            print("ERROR: Module without name found in registry.yaml", file=sys.stderr)
            sys.exit(1)

        print(f"  Processing module: {name}")
        modules_section[name] = build_module_entry(module, now_iso)

    # Update plugin entries
    plugins = registry.get('plugins', {})
    for plugin_name, plugin_entry in plugins.items():
        # Convert existing modules array to empty InstalledModule array
        # (Actual module installations will populate this later)
        old_modules = plugin_entry.get('modules', [])
        if old_modules and isinstance(old_modules, list):
            if old_modules and isinstance(old_modules[0], str):
                # Old format: list of strings like "microtonality@1.2.0"
                print(f"  Converting {plugin_name} modules from string array to empty InstalledModule array")
                print(f"    (Previous modules: {old_modules})")
        plugin_entry['modules'] = []  # Empty array for now

    # Update registry
    registry['version'] = '3.0.0'
    registry['modules'] = modules_section

    # Write atomically
    print(f"Writing updated registry to: {json_path}")
    atomic_write_json(json_path, registry)

    print(f"\nMigration complete!")
    print(f"  Version: 2.0.0 -> 3.0.0")
    print(f"  Modules added: {len(modules_section)}")
    print(f"  Plugins updated: {len(plugins)}")


def main():
    # Determine project root (script is in modules/scripts/)
    script_dir = Path(__file__).parent.resolve()
    project_root = script_dir.parent.parent

    print(f"Project root: {project_root}")

    try:
        migrate_registry(project_root)
    except Exception as e:
        print(f"ERROR: Migration failed: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
