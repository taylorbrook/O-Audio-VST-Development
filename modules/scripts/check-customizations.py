#!/usr/bin/env python3
"""
Check if a plugin has customized a module.

Compares the current content hash of installed module files against the
originalHash stored in the registry at installation time. This detects
local modifications that would be lost during module upgrade.

Usage:
    # Check single module
    python3 check-customizations.py O-IntonationPad scala-tuning-engine

    # Output if not modified:
    scala-tuning-engine: clean
    Hash: sha256:a1b2c3d4e5f6g7h8

    # Output if modified:
    scala-tuning-engine: MODIFIED
    Original: sha256:a1b2c3d4e5f6g7h8
    Current:  sha256:x9y8z7w6v5u4t3s2

    Modified files detected. Run /module:upgrade with care.

Exit codes:
    0: Module is clean (no modifications)
    1: Module has been modified
    2: Error (module not found, registry error, etc.)
"""

import json
import sys
from pathlib import Path

# Import compute_module_hash from compute-hash.py
# Add the script directory to path for import
script_dir = Path(__file__).parent
sys.path.insert(0, str(script_dir))

from pathlib import Path as PathLib

# Re-import to use the function
import importlib.util
spec = importlib.util.spec_from_file_location("compute_hash", script_dir / "compute-hash.py")
compute_hash_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compute_hash_module)
compute_module_hash = compute_hash_module.compute_module_hash


def load_registry() -> dict:
    """Load the registry.json file."""
    registry_path = Path(__file__).parent.parent.parent / ".planning" / "workflow" / "registry.json"

    if not registry_path.exists():
        raise FileNotFoundError(f"Registry not found: {registry_path}")

    with open(registry_path, 'r') as f:
        return json.load(f)


def find_installed_module(registry: dict, plugin_name: str, module_name: str) -> dict | None:
    """Find the InstalledModule entry for a plugin/module combination."""
    plugins = registry.get("plugins", {})

    if plugin_name not in plugins:
        return None

    plugin = plugins[plugin_name]
    modules = plugin.get("modules", [])

    for module in modules:
        if module.get("name") == module_name:
            return module

    return None


def get_module_path(registry: dict, module_name: str) -> Path | None:
    """Get the path to a module from the registry."""
    modules = registry.get("modules", {})

    if module_name not in modules:
        return None

    module_info = modules[module_name]
    return Path(__file__).parent.parent.parent / module_info["path"]


def get_installed_module_path(plugin_name: str, module_name: str) -> Path:
    """Get path to where module is installed in a plugin."""
    # Modules are installed in the plugin's Source directory
    # The exact location depends on module category, but we can check common patterns
    base = Path(__file__).parent.parent.parent / "plugins" / plugin_name / "Source"

    # Check for module.yaml in the module source to get structure info
    # For now, we assume modules are copied to a modules/ subdirectory
    return base / "modules" / module_name


def is_modified(current_hash: str, original_hash: str) -> bool:
    """Check if plugin's copy differs from original."""
    return current_hash != original_hash


def check_module_customization(plugin_name: str, module_name: str) -> tuple[bool, str, str]:
    """
    Check if a module has been customized in a plugin.

    Args:
        plugin_name: Name of the plugin
        module_name: Name of the module

    Returns:
        Tuple of (is_modified, current_hash, original_hash)

    Raises:
        ValueError: If module not found or not installed in plugin
    """
    registry = load_registry()

    # Find the installed module entry
    installed = find_installed_module(registry, plugin_name, module_name)
    if installed is None:
        raise ValueError(f"Module '{module_name}' not found in plugin '{plugin_name}'")

    # Get the original hash from registry
    original_hash = installed.get("originalHash") or installed.get("contentHash")
    if not original_hash:
        raise ValueError(f"No original hash recorded for '{module_name}' in '{plugin_name}'")

    # Get the module source path to compute current hash
    # Note: We compare against the SOURCE module, not the installed copy
    # This tells us if the plugin has diverged from the module source
    module_path = get_module_path(registry, module_name)
    if module_path is None or not module_path.exists():
        # If source doesn't exist, check the installed copy
        installed_path = get_installed_module_path(plugin_name, module_name)
        if not installed_path.exists():
            raise ValueError(f"Cannot find module files for '{module_name}'")
        module_path = installed_path

    # Compute current hash
    current_hash = compute_module_hash(module_path)

    return (is_modified(current_hash, original_hash), current_hash, original_hash)


def main():
    """CLI interface for customization checking."""
    if len(sys.argv) < 3:
        print("Usage: check-customizations.py <plugin_name> <module_name>", file=sys.stderr)
        print("", file=sys.stderr)
        print("Check if a plugin has modified a module's files.", file=sys.stderr)
        print("", file=sys.stderr)
        print("Exit codes:", file=sys.stderr)
        print("  0: Module is clean (no modifications)", file=sys.stderr)
        print("  1: Module has been modified", file=sys.stderr)
        print("  2: Error (module not found, etc.)", file=sys.stderr)
        sys.exit(2)

    plugin_name = sys.argv[1]
    module_name = sys.argv[2]

    try:
        modified, current_hash, original_hash = check_module_customization(plugin_name, module_name)

        if modified:
            print(f"{module_name}: MODIFIED")
            print(f"Original: {original_hash}")
            print(f"Current:  {current_hash}")
            print("")
            print("Modified files detected. Run /module:upgrade with care.")
            sys.exit(1)
        else:
            print(f"{module_name}: clean")
            print(f"Hash: {current_hash}")
            sys.exit(0)

    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(2)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(2)
    except Exception as e:
        print(f"Unexpected error: {e}", file=sys.stderr)
        sys.exit(2)


if __name__ == '__main__':
    main()
