---
name: module:upgrade
description: Upgrade module and rebuild all dependent plugins
skill: module-system
args: "[module_name]"
---

# /module:upgrade

Upgrade a module to its latest version and rebuild all plugins that depend on it.

## Usage

```
/module:upgrade [module_name]
```

## Arguments

- `module_name` - Module to upgrade (e.g., `preset-manager`)

## Behavior

1. Check for module updates (compare local vs latest)
2. Show changelog for new version
3. Ask for confirmation
4. Update module files
5. For each dependent plugin:
   - Clear CMake cache
   - Rebuild VST3 and AU
   - Run validation
6. Report results

## Example

```
/module:upgrade preset-manager

Upgrading module: preset-manager
═══════════════════════════════════════════════════════════════

Current version: 1.0.0
Latest version: 1.1.0

Changelog:
─────────────────────────────────────────────────────────────
v1.1.0 (2026-01-28):
  - Added preset categories support
  - Fixed preset ordering bug
  - Improved save/load performance

Dependent plugins (3):
  - O-Marimba
  - O-Tremolo
  - O-Lyrica

Upgrade and rebuild all? (y/n): y

Upgrading module...
✓ Module updated to v1.1.0

Rebuilding dependents...

[1/3] O-Marimba
  ✓ CMake cache cleared
  ✓ VST3 built
  ✓ AU built
  ✓ Validation passed

[2/3] O-Tremolo
  ✓ CMake cache cleared
  ✓ VST3 built
  ✓ AU built
  ✓ Validation passed

[3/3] O-Lyrica
  ✓ CMake cache cleared
  ✓ VST3 built
  ✓ AU built
  ✓ Validation passed

Upgrade complete!
─────────────────────────────────────────────────────────────
Module: preset-manager v1.1.0
Plugins rebuilt: 3/3 successful
```

## Partial Failure

If some plugins fail to rebuild:

```
[2/3] O-Tremolo
  ✓ CMake cache cleared
  ✗ Build failed

  Error: 'PresetCategory' is not defined

  This plugin may need code updates for the new API.

  Options:
  1. View build log
  2. Skip and continue
  3. Abort upgrade

Choose: 2

Skipping O-Tremolo...

[3/3] O-Lyrica
  ...

Upgrade partially complete
─────────────────────────────────────────────────────────────
Module: preset-manager v1.1.0
Plugins rebuilt: 2/3 successful

Failed plugins need manual attention:
  - O-Tremolo: Build failed (API change)

See module changelog for migration notes.
```

## Related Commands

- `/module:info [name]` - View current module version
- `/module:list` - See all modules
