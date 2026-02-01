# Module System Documentation

Complete reference for the Ouaricon Module System - reusable components shared across JUCE plugins.

## Overview

### What Are Modules?

Modules are **"copied and tracked"** reusable components. Unlike traditional dependencies that are linked at build time, modules are:

1. **Copied** into each plugin's source tree
2. **Tracked** in the central registry for version management
3. **Independent** - each plugin can have different versions or customizations

This approach provides:
- Full control over plugin code (no external dependencies at runtime)
- Easy customization per plugin
- Clear tracking of which plugins use which modules

### Module Categories

| Category | Description | Examples |
|----------|-------------|----------|
| **core** | Essential infrastructure | webview-relay-manager, resource-provider |
| **persistence** | State management | preset-manager |
| **metering** | Audio analysis | vu-meter |
| **tuning** | Pitch/tuning systems | scala-tuning-engine |
| **effects** | DSP processing units | analog-eq-unit, compressor-unit |
| **ui** | Interface components | playable-keyboard |

### Relationship to CMake

Modules integrate with CMake through `OuariconModules.cmake`:

```cmake
# In your plugin's CMakeLists.txt
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Add modules (generates include paths and source files)
ouaricon_add_module(${PROJECT_NAME} scala-tuning-engine)
ouaricon_add_module(${PROJECT_NAME} preset-manager)
```

## Quick Start

### Adding a Module

```bash
/module:add O-IntonationPad scala-tuning-engine
```

This:
1. Validates plugin and module exist
2. Computes content hash for tracking
3. Updates CMakeLists.txt with `ouaricon_add_module()`
4. Copies JS files to ui/public/modules/
5. Adds InstalledModule entry to registry
6. Triggers plugin rebuild

### Listing Available Modules

```bash
/module:list
```

Shows all modules grouped by category with versions.

### Getting Module Info

```bash
/module:info scala-tuning-engine
```

Shows version, category, description, dependencies, and which plugins use it.

## Commands Reference

### /module:add [plugin] [module]

Add a module to a plugin with full tracking.

**What it does:**
- Copies module files to plugin source tree
- Updates CMakeLists.txt with include directive
- Tracks installation in registry with content hash
- Triggers auto-rebuild of plugin

**Example:**
```
/module:add O-IntonationPad preset-manager

Adding module: preset-manager v1.0.0 to O-IntonationPad
=========================================================

Step 1: Validating inputs
  [ok] Plugin O-IntonationPad exists in registry
  [ok] Module preset-manager exists (v1.0.0)
  [ok] Module not already installed

Step 2: Computing content hash
  [ok] Hash: sha256:a1b2c3d4e5f6g7h8

Step 3: Updating CMakeLists.txt
  [ok] Added ouaricon_add_module(${PROJECT_NAME} preset-manager)

Step 4: Updating registry.json
  [ok] Added InstalledModule entry

Step 5: Building plugin
  [ok] Build succeeded

Module added successfully!
```

### /module:remove [plugin] [module]

Stop tracking a module (code remains in plugin).

**What it does:**
- Removes InstalledModule entry from registry
- Removes plugin from module's dependents list
- Optionally comments out CMakeLists.txt line
- **Does NOT delete source files**

**Why soft removal?**
- Safety: Plugin code may depend on module files
- Flexibility: Keep customized code while stopping updates
- Reversibility: Easy to re-add tracking later

**Example:**
```
/module:remove O-IntonationPad scala-tuning-engine

Module tracking removed successfully.

Module code remains in plugin:
  - Source includes from modules/tuning/scala-tuning-engine/cpp/
  - ui/public/modules/tuning-panel.js
```

### /module:upgrade [module]

Update a module across all plugins that use it.

**What it does:**
- Checks for available updates (semver comparison)
- Detects local customizations in each plugin
- Offers per-plugin choice: keep or update
- Stores rollback before updating
- Reverts on build failure if requested

**Example:**
```
/module:upgrade preset-manager

[1/2] O-IntonationPad
  Status: LOCAL CUSTOMIZATIONS DETECTED
  Options:
    [k] Keep local version (skip update)
    [u] Update (will overwrite your changes)
  Choose: k
  Skipped (keeping local version)

[2/2] O-Marimba
  Status: Clean (no modifications)
  Update to 1.1.0? [y/n]: y
  Build succeeded.
```

### /module:upgrade-all

Batch upgrade all modules with preview.

**What it does:**
- Scans all modules for available updates
- Shows comprehensive preview before any changes
- Processes each module with per-plugin choice
- Reports summary of all changes

**Flags:**
- `--dry-run`: Show preview only, make no changes

**Example:**
```
/module:upgrade-all

Module Update Preview
=====================================================================

scala-tuning-engine: 1.0.0 -> 1.1.0
  Plugins: O-IntonationPad (modified), O-Marimba
  Breaking: No

preset-manager: 1.0.0 -> 1.2.0
  Plugins: O-Marimba, O-Tremolo
  Breaking: No

Summary:
  2 modules with updates
  4 plugin updates total
  0 breaking changes

Proceed with upgrades? [y/n]:
```

### /module:list

Show all available modules.

**Output:**
```
Ouaricon Module System
===============================================================

  CORE
  ---------------------------------------------------------------
  webview-relay-manager    v1.0.0    Relay lifecycle management
  resource-provider        v1.0.0    WebView resource serving

  TUNING
  ---------------------------------------------------------------
  scala-tuning-engine      v1.0.0    Microtonal tuning system

Total: 8 modules
```

### /module:info [module]

Detailed module information.

**Output:**
```
Module: scala-tuning-engine
===============================================================

Version: 1.0.0
Category: tuning

Description:
Complete microtonal tuning system supporting 12-TET, custom scales
via Scala (.scl/.kbm) files, and MTS-ESP integration.

Provides:
  - cpp-class: OuariconTuningEngine
  - js-module: tuning-panel.js
  - js-module: pitch-circle.js

Used by:
  - O-Lyrica
  - O-Marimba

Path: modules/tuning/scala-tuning-engine/
```

## Registry Structure

The module system uses `.planning/workflow/registry.json` with schema version 3.0.0.

### Global Module Catalog

```json
{
  "modules": {
    "scala-tuning-engine": {
      "version": "1.0.0",
      "path": "modules/tuning/scala-tuning-engine",
      "category": "tuning",
      "description": "Microtonal tuning system",
      "dependents": ["O-IntonationPad", "O-Marimba"],
      "lastUpdated": "2026-02-01",
      "usageStats": {
        "addCount": 5,
        "removeCount": 1
      }
    }
  }
}
```

### InstalledModule Entries (Per Plugin)

```json
{
  "plugins": {
    "O-IntonationPad": {
      "modules": [
        {
          "name": "scala-tuning-engine",
          "version": "1.0.0",
          "installedAt": "2026-02-01T15:30:00Z",
          "modified": false,
          "contentHash": "sha256:b6c81f83d245841c",
          "originalHash": "sha256:b6c81f83d245841c"
        }
      ]
    }
  }
}
```

**Fields explained:**
- `name`: Module identifier
- `version`: Version when installed
- `installedAt`: ISO 8601 timestamp of installation
- `modified`: True if user has customized module files
- `contentHash`: Current SHA-256 hash (truncated to 16 hex chars)
- `originalHash`: Hash at install time (baseline for modification detection)

### Content Hash for Customization Tracking

When `contentHash != originalHash`, the module has been modified locally.

Hash computation:
```bash
python3 modules/scripts/compute-hash.py <module_path>
# Returns: sha256:<16 hex chars>
```

The hash includes relative file paths + contents to detect both edits and renames.

## Manual Rebuild Guide

### When Manual Intervention is Needed

**1. After /module:remove if you want full code removal**

The soft removal keeps code. To fully remove:

```bash
# 1. Delete JS files
rm plugins/O-IntonationPad/Source/ui/public/modules/tuning-panel.js
rm plugins/O-IntonationPad/Source/ui/public/modules/pitch-circle.js

# 2. Remove #include directives from C++ code
# Edit PluginProcessor.h, remove:
#   #include "modules/tuning/scala-tuning-engine/cpp/OuariconTuningEngine.h"

# 3. Remove member variables and usage
# Edit PluginProcessor.h/.cpp, remove:
#   OuariconTuningEngine tuningEngine;
#   and all tuningEngine.* calls

# 4. Comment out in CMakeLists.txt
# Comment this line:
#   ouaricon_add_module(${PROJECT_NAME} scala-tuning-engine)

# 5. Rebuild
cd /Users/taylorbrook/Dev/VST-development/build
ninja O-IntonationPad_VST3 O-IntonationPad_AU
```

**2. After upgrade fails with rollback**

If an upgrade fails and you rolled back:
- Check build log for specific errors
- May need code changes to use new module API
- Review module changelog for breaking changes

**3. After manual file edits**

If you edit module files directly:
- Your `contentHash` will differ from `originalHash`
- `modified: true` will be set in registry
- `/module:upgrade` will warn before overwriting

To resync tracking after edits:
- Run `/module:add` again (optional - updates hash baseline)
- Or accept that customizations are tracked

### Rebuild Commands

```bash
# Navigate to build directory
cd /Users/taylorbrook/Dev/VST-development/build

# Rebuild single plugin (both formats)
ninja O-IntonationPad_VST3 O-IntonationPad_AU

# If CMake configuration changed (rare)
rm -rf CMakeCache.txt CMakeFiles/
cmake ..
ninja

# Full rebuild (if needed)
ninja clean
ninja
```

### Post-Build Verification

After rebuilding, install and verify in DAW:

```bash
# Clear macOS AU cache (CRITICAL)
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

# Remove old binaries
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-IntonationPad.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-IntonationPad.component

# Install fresh binaries
cp -R build/plugins/O-IntonationPad/O-IntonationPad_artefacts/Release/VST3/O-IntonationPad.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-IntonationPad/O-IntonationPad_artefacts/Release/AU/O-IntonationPad.component ~/Library/Audio/Plug-Ins/Components/

# Verify AU registration
auval -a | grep -i IntonationPad
```

## Troubleshooting

### Module not found in CMake

**Symptom:** Build error about missing module headers or sources

**Cause:** `ouaricon_add_module()` line missing or malformed

**Fix:**
1. Check CMakeLists.txt has the include:
   ```cmake
   include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
   ```
2. Verify module line exists:
   ```cmake
   ouaricon_add_module(${PROJECT_NAME} scala-tuning-engine)
   ```
3. Re-run CMake: `cd build && cmake ..`

### Registry out of sync

**Symptom:** Commands report module installed but files missing (or vice versa)

**Cause:** Manual file operations bypassed registry

**Fix:**
1. Run `/plugin:status` to check state
2. Registry is source of truth for tracking
3. If files exist but not tracked: run `/module:add` to register
4. If tracked but files missing: run `/module:remove` then `/module:add`

### Hash mismatch after manual edits

**Symptom:** `/module:upgrade` warns about local customizations

**Cause:** You modified module files directly (expected behavior)

**This is normal.** The system tracks this intentionally:
- `modified: true` in registry tracks customization
- `/module:upgrade` will ask before overwriting
- Choose "keep" to preserve your changes

To reset baseline (accept current state as "clean"):
```bash
# Re-compute and update hash in registry
python3 modules/scripts/compute-hash.py <module_path>
# Then update originalHash in registry.json to match
```

### Build fails after module upgrade

**Symptom:** Upgrade succeeded but build fails with API errors

**Cause:** Breaking API changes in new module version

**Fix:**
1. Check if rollback was offered - accept it
2. Review module changelog for breaking changes
3. Update your code to match new API
4. Rebuild manually

### Module JS files not loading

**Symptom:** WebView UI can't find module JavaScript

**Cause:** JS files not copied or wrong path

**Fix:**
1. Check `plugins/{plugin}/Source/ui/public/modules/` for files
2. Verify import path in your UI code:
   ```javascript
   import { TuningPanel } from './modules/tuning-panel.js';
   ```
3. Re-run `/module:add` if files missing

## Design Decisions

Key decisions from Phase 7 context discussion:

### Per-Plugin Versioning
Each plugin gets its own version of a module. No forced single version across all plugins. This allows plugins at different development stages to use appropriate module versions.

### Soft Removal
Module removal keeps code in place. This is safer (doesn't break builds) and reversible. User can manually delete files if full removal is desired.

### Verbose Output by Default
All commands show each step as it happens. No silent operations. This helps diagnose issues and understand what changed.

### No Auto-Repair
The system never auto-repairs without prompting. If something is wrong, it reports the issue and waits for user decision. This prevents well-intentioned automation from making things worse.

### Per-Plugin Choice on Upgrade
When upgrading a module, each plugin using it gets individual choice. Plugins with customizations can keep their version while others update. This respects per-plugin variation.

### Content Hash Tracking
SHA-256 hash of all module files (including relative paths) detects both content changes and file renames. Hash is truncated to 16 hex chars for readability while maintaining collision resistance.

## Related Documentation

- `.planning/workflow/registry.schema.json` - Registry JSON Schema
- `.planning/workflow/dependencies.schema.json` - Module dependency schema
- `modules/scripts/` - Python utilities (compute-hash, semver, check-customizations)
- `modules/cmake/OuariconModules.cmake` - CMake integration

## Command Quick Reference

| Command | Purpose |
|---------|---------|
| `/module:list` | List all available modules |
| `/module:info [name]` | Detailed module information |
| `/module:add [plugin] [module]` | Add module to plugin |
| `/module:remove [plugin] [module]` | Stop tracking (code stays) |
| `/module:upgrade [module]` | Update module across plugins |
| `/module:upgrade-all` | Batch upgrade with preview |
