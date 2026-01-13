---
name: modules
description: Manage Ouaricon modules - list, add, upgrade, info
---

# /modules

Manage the Ouaricon Module System - reusable components for JUCE plugin development.

## Usage

```
/modules                     # List all available modules
/modules list               # Same as above
/modules add <module>       # Add module to current plugin
/modules info <module>      # Show module details
/modules check              # Check for available updates
/modules upgrade <module>   # Upgrade module in current plugin
```

## Available Commands

### /modules (or /modules list)

List all available modules in the Ouaricon Module System.

**Implementation:**
1. Read `modules/registry.yaml`
2. Display modules grouped by category
3. Show version, reuse score, and brief description

### /modules add <module-name>

Add a module to the current plugin.

**Implementation:**
1. Verify current directory is a plugin
2. Find module in registry
3. Add to plugin's CMakeLists.txt via `ouaricon_add_module()`
4. Copy any JS files to `Source/ui/public/modules/`
5. Create/update `modules.lock.yaml`
6. Show integration instructions

### /modules info <module-name>

Show detailed information about a module.

**Implementation:**
1. Read `modules/{category}/{module-name}/module.yaml`
2. Display:
   - Description
   - Version
   - What it provides (classes, functions, JS modules)
   - Configuration options
   - Usage examples from README.md
   - Which plugins currently use it

### /modules check

Check all plugins for available module updates.

**Implementation:**
1. Run `modules/scripts/check-updates.py`
2. Display any available updates
3. Suggest upgrade commands

### /modules upgrade <module-name>

Upgrade a module in the current plugin.

**Implementation:**
1. Run `modules/scripts/upgrade-module.py [plugin] [module]`
2. Display changes
3. Show any migration notes
4. Rebuild plugin

## Module System Overview

The Ouaricon Module System provides reusable, versioned components:

| Category | Modules | Purpose |
|----------|---------|---------|
| core | webview-relay-manager | Prevents WebView destruction crashes |
| persistence | preset-manager | JSON presets with factory/user separation |
| metering | vu-meter | Thread-safe VU metering with animations |

## Integration with Planning

During Stage 0 (Research & Planning), the research-planning-agent should:
1. Query available modules via registry.yaml
2. Recommend modules based on creative brief requirements
3. Include module dependencies in architecture.md

## Related Commands

- `/plan` - Stage 0 planning (can suggest modules)
- `/implement` - Stage 1-3 implementation (uses modules)
- `/continue` - Resume work (respects modules.lock.yaml)
