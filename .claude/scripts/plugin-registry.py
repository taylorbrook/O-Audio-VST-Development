#!/usr/bin/env python3
"""
Plugin Registry Manager

Utilities for managing the plugin registry and phase state machine.
Used by skills to track plugin state, phases, and transitions.
"""

import json
import os
import sys
from datetime import datetime
from pathlib import Path
import re

# Paths
SCRIPT_DIR = Path(__file__).parent
CLAUDE_DIR = SCRIPT_DIR.parent
PROJECT_ROOT = CLAUDE_DIR.parent
REGISTRY_PATH = CLAUDE_DIR / "plugin-registry.json"
CONSTANTS_PATH = CLAUDE_DIR / "schemas" / "phase-constants.json"

# Load constants
def load_constants():
    """Load phase constants from schema file."""
    with open(CONSTANTS_PATH) as f:
        return json.load(f)

CONSTANTS = None

def get_constants():
    global CONSTANTS
    if CONSTANTS is None:
        CONSTANTS = load_constants()
    return CONSTANTS

# Registry operations
def load_registry():
    """Load the plugin registry, creating if needed."""
    if not REGISTRY_PATH.exists():
        return {"version": "2.0.0", "focused": None, "plugins": {}}
    with open(REGISTRY_PATH) as f:
        return json.load(f)

def save_registry(registry):
    """Save the plugin registry."""
    with open(REGISTRY_PATH, 'w') as f:
        json.dump(registry, f, indent=2)

def get_focused_plugin():
    """Get the currently focused plugin name."""
    registry = load_registry()
    return registry.get("focused")

def set_focused_plugin(plugin_name):
    """Set the focused plugin."""
    registry = load_registry()
    if plugin_name and plugin_name not in registry["plugins"]:
        print(f"Error: Plugin '{plugin_name}' not in registry", file=sys.stderr)
        return False
    registry["focused"] = plugin_name
    save_registry(registry)
    return True

def register_plugin(plugin_name, path=None):
    """Register a new plugin in the registry."""
    registry = load_registry()
    if plugin_name in registry["plugins"]:
        print(f"Plugin '{plugin_name}' already registered", file=sys.stderr)
        return False

    registry["plugins"][plugin_name] = {
        "path": path or f"plugins/{plugin_name}",
        "stage": "0-ideation",
        "phase": "discuss",
        "status": "active",
        "created": datetime.now().strftime("%Y-%m-%d"),
        "lastActivity": datetime.now().isoformat(),
        "modules": [],
        "expressMode": False,
        "blockedBy": None
    }
    save_registry(registry)
    return True

def get_plugin(plugin_name):
    """Get plugin entry from registry."""
    registry = load_registry()
    return registry["plugins"].get(plugin_name)

def update_plugin(plugin_name, updates):
    """Update plugin entry in registry."""
    registry = load_registry()
    if plugin_name not in registry["plugins"]:
        print(f"Error: Plugin '{plugin_name}' not found", file=sys.stderr)
        return False

    registry["plugins"][plugin_name].update(updates)
    registry["plugins"][plugin_name]["lastActivity"] = datetime.now().isoformat()
    save_registry(registry)
    return True

def list_plugins(status_filter=None):
    """List all plugins, optionally filtered by status."""
    registry = load_registry()
    plugins = registry["plugins"]

    if status_filter:
        plugins = {k: v for k, v in plugins.items() if v["status"] == status_filter}

    return plugins

# Phase state machine
def get_next_phase(current_phase):
    """Get the next phase in the GSD cycle."""
    constants = get_constants()
    return constants["transitions"]["nextPhase"].get(current_phase)

def get_next_stage(current_stage):
    """Get the next stage in the development lifecycle."""
    constants = get_constants()
    return constants["transitions"]["nextStage"].get(current_stage)

def advance_phase(plugin_name, skip_phases=None):
    """Advance to the next phase, skipping specified phases."""
    skip_phases = skip_phases or []
    plugin = get_plugin(plugin_name)
    if not plugin:
        return None

    current_phase = plugin["phase"]
    current_stage = plugin["stage"]

    # Get next phase
    next_phase = get_next_phase(current_phase)

    # Skip phases if requested
    while next_phase in skip_phases:
        next_phase = get_next_phase(next_phase)

    if next_phase is None:
        # End of phase cycle, advance to next stage
        next_stage = get_next_stage(current_stage)
        if next_stage is None or next_stage == "complete":
            update_plugin(plugin_name, {
                "stage": "complete",
                "phase": None,
                "status": "released"
            })
            return {"stage": "complete", "phase": None}
        else:
            update_plugin(plugin_name, {
                "stage": next_stage,
                "phase": "discuss"
            })
            return {"stage": next_stage, "phase": "discuss"}
    else:
        update_plugin(plugin_name, {"phase": next_phase})
        return {"stage": current_stage, "phase": next_phase}

def set_phase(plugin_name, stage, phase):
    """Explicitly set stage and phase."""
    update_plugin(plugin_name, {
        "stage": stage,
        "phase": phase
    })
    return {"stage": stage, "phase": phase}

# STATUS.md operations
def parse_status_frontmatter(status_path):
    """Parse YAML frontmatter from STATUS.md."""
    with open(status_path) as f:
        content = f.read()

    # Extract YAML frontmatter
    match = re.match(r'^---\n(.*?)\n---', content, re.DOTALL)
    if not match:
        return {}

    yaml_content = match.group(1)
    # Simple YAML parsing (avoid external dependency)
    frontmatter = {}
    for line in yaml_content.split('\n'):
        if ':' in line:
            key, value = line.split(':', 1)
            key = key.strip()
            value = value.strip()
            # Handle arrays
            if value.startswith('[') and value.endswith(']'):
                value = [v.strip().strip('"\'') for v in value[1:-1].split(',') if v.strip()]
            # Handle booleans
            elif value.lower() == 'true':
                value = True
            elif value.lower() == 'false':
                value = False
            elif value == 'null':
                value = None
            frontmatter[key] = value

    return frontmatter

def get_status_path(plugin_name):
    """Get the STATUS.md path for a plugin."""
    plugin = get_plugin(plugin_name)
    if not plugin:
        return None
    return PROJECT_ROOT / plugin["path"] / ".planning" / "STATUS.md"

def sync_registry_from_status(plugin_name):
    """Sync registry with STATUS.md (STATUS.md is source of truth)."""
    status_path = get_status_path(plugin_name)
    if not status_path or not status_path.exists():
        print(f"STATUS.md not found for {plugin_name}", file=sys.stderr)
        return False

    frontmatter = parse_status_frontmatter(status_path)
    if not frontmatter:
        return False

    updates = {}
    if "current_stage" in frontmatter:
        updates["stage"] = frontmatter["current_stage"]
    if "current_phase" in frontmatter:
        updates["phase"] = frontmatter["current_phase"]
    if "express_mode" in frontmatter:
        updates["expressMode"] = frontmatter["express_mode"]
    if "modules" in frontmatter:
        updates["modules"] = frontmatter["modules"]

    if updates:
        update_plugin(plugin_name, updates)
    return True

# CLI interface
def main():
    if len(sys.argv) < 2:
        print("Usage: plugin-registry.py <command> [args...]")
        print("\nCommands:")
        print("  list [status]           - List all plugins")
        print("  get <name>              - Get plugin details")
        print("  register <name> [path]  - Register new plugin")
        print("  focus <name>            - Set focused plugin")
        print("  focus                   - Get focused plugin")
        print("  update <name> <json>    - Update plugin fields")
        print("  advance <name>          - Advance to next phase")
        print("  set-phase <name> <stage> <phase> - Set stage/phase")
        print("  sync <name>             - Sync registry from STATUS.md")
        sys.exit(1)

    command = sys.argv[1]

    if command == "list":
        status_filter = sys.argv[2] if len(sys.argv) > 2 else None
        plugins = list_plugins(status_filter)
        print(json.dumps(plugins, indent=2))

    elif command == "get":
        if len(sys.argv) < 3:
            print("Usage: plugin-registry.py get <name>", file=sys.stderr)
            sys.exit(1)
        plugin = get_plugin(sys.argv[2])
        if plugin:
            print(json.dumps(plugin, indent=2))
        else:
            sys.exit(1)

    elif command == "register":
        if len(sys.argv) < 3:
            print("Usage: plugin-registry.py register <name> [path]", file=sys.stderr)
            sys.exit(1)
        name = sys.argv[2]
        path = sys.argv[3] if len(sys.argv) > 3 else None
        if register_plugin(name, path):
            print(f"Registered: {name}")
        else:
            sys.exit(1)

    elif command == "focus":
        if len(sys.argv) > 2:
            if set_focused_plugin(sys.argv[2]):
                print(f"Focused: {sys.argv[2]}")
            else:
                sys.exit(1)
        else:
            focused = get_focused_plugin()
            if focused:
                print(focused)
            else:
                print("(none)")

    elif command == "update":
        if len(sys.argv) < 4:
            print("Usage: plugin-registry.py update <name> <json>", file=sys.stderr)
            sys.exit(1)
        updates = json.loads(sys.argv[3])
        if update_plugin(sys.argv[2], updates):
            print(f"Updated: {sys.argv[2]}")
        else:
            sys.exit(1)

    elif command == "advance":
        if len(sys.argv) < 3:
            print("Usage: plugin-registry.py advance <name>", file=sys.stderr)
            sys.exit(1)
        result = advance_phase(sys.argv[2])
        if result:
            print(json.dumps(result))
        else:
            sys.exit(1)

    elif command == "set-phase":
        if len(sys.argv) < 5:
            print("Usage: plugin-registry.py set-phase <name> <stage> <phase>", file=sys.stderr)
            sys.exit(1)
        result = set_phase(sys.argv[2], sys.argv[3], sys.argv[4])
        print(json.dumps(result))

    elif command == "sync":
        if len(sys.argv) < 3:
            print("Usage: plugin-registry.py sync <name>", file=sys.stderr)
            sys.exit(1)
        if sync_registry_from_status(sys.argv[2]):
            print(f"Synced: {sys.argv[2]}")
        else:
            sys.exit(1)

    else:
        print(f"Unknown command: {command}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
