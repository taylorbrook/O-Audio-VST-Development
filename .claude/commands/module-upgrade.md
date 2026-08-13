---
name: module:upgrade
description: Upgrade module with per-plugin choice, customization warning, and rollback support
skill: module-system
args: "[module_name]"
---

# /module:upgrade

Upgrade a module to its latest version with per-plugin control. Detects local customizations and offers rollback on build failure.

## Usage

```
/module:upgrade [module_name]
```

## Arguments

- `module_name` - Module to upgrade (e.g., `preset-manager`)

## Workflow

### 1. Check for Updates

```bash
# Compare module version in registry vs installed version in each plugin
python3 modules/scripts/semver.py compare <installed> <available>
```

- Load `modules/registry.yaml` (the per-plugin `modules` array in the retired workflow state file is no longer written)
- Get the available module version from the module entry's `version` field
- For each entry in that module's `used_by:` list, compare its recorded `version` against the module's `version`
- List all plugins where update is available

### 2. For Each Dependent Plugin (Per-Plugin Choice)

#### a. Check for Customizations

```bash
python3 modules/scripts/check-customizations.py <plugin> <module>
```

- Compute current content hash of installed files
- Compare with originalHash stored at install time

#### b. If Modified (Exit Code 1)

Show warning and offer choice:

```
[1/2] O-IntonationPad
  Installed: 1.0.0
  Status: LOCAL CUSTOMIZATIONS DETECTED

  This plugin has modified the module files.
  Original hash: sha256:a1b2c3d4e5f6g7h8
  Current hash:  sha256:x9y8z7w6v5u4t3s2

  Options:
    [k] Keep local version (skip update)
    [u] Update (will overwrite your changes)

  Choose:
```

User decides: "Warn on local customizations, give choice to keep or overwrite"

#### c. If Not Modified (Exit Code 0)

Show changelog and ask for confirmation:

```
[2/2] O-Marimba
  Installed: 1.0.0
  Status: Clean (no modifications)

  Update to 1.1.0? [y/n]:
```

#### d. If Major Version Update

```bash
python3 modules/scripts/semver.py is-major <old> <new>
```

If true, show prominent warning:

```
  ⚠️  BREAKING CHANGE: Major version update (1.x -> 2.x)

  Known breaking changes:
    - API signature changed for initPreset()
    - PresetCategory enum values renamed

  This may require code changes. Proceed? [y/n]:
```

### 3. Apply Update (If Confirmed)

#### a. Store Rollback

```bash
mkdir -p .planning/workflow/rollback/<plugin>/<module>/
cp -R <installed_files> .planning/workflow/rollback/<plugin>/<module>/
```

#### b. Copy New Module Files

```bash
cp -R modules/<category>/<module>/* plugins/<plugin>/Source/modules/<module>/
```

#### c. Update InstalledModule Entry in Registry

```json
{
  "name": "<module>",
  "version": "<new_version>",
  "installedAt": "<original_install_time>",
  "updatedAt": "<current_time_iso8601>",
  "modified": false,
  "contentHash": "<new_hash>",
  "originalHash": "<new_hash>"
}
```

Note: originalHash is reset on update (new baseline for future customization detection)

#### d. Trigger Rebuild

```bash
cd build && ninja <plugin>_VST3 <plugin>_AU
```

### 4. On Build Failure

Offer rollback:

```
Build failed!

Error: 'PresetCategory' is not defined

Rollback to previous version? [y/n]:
```

If user accepts:

```bash
# Restore files from rollback
cp -R .planning/workflow/rollback/<plugin>/<module>/* plugins/<plugin>/Source/modules/<module>/

# Revert registry entry to previous version
# (stored before update)

# Cleanup rollback
rm -rf .planning/workflow/rollback/<plugin>/<module>/

# Rebuild with old version
cd build && ninja <plugin>_VST3 <plugin>_AU
```

### 5. Report Result

Show summary for all plugins:

```
Upgrade complete!
─────────────────────────────────────────────────────────────
Module: preset-manager 1.0.0 -> 1.1.0

Results:
  O-IntonationPad: Skipped (local customizations kept)
  O-Marimba: Updated successfully
  O-Tremolo: Rolled back (build failed)
```

## Complete Example with Customizations

```
/module:upgrade scala-tuning-engine

Checking module: scala-tuning-engine
─────────────────────────────────────────────────────────────

Current version: 1.0.0
Latest version: 1.1.0

Changelog (1.0.0 -> 1.1.0):
  - Added MTS-ESP full support
  - Fixed tonic rotation edge case
  - Added pitch bend range parameter

Dependent plugins (2):

[1/2] O-IntonationPad
  Installed: 1.0.0
  Status: LOCAL CUSTOMIZATIONS DETECTED

  This plugin has modified the module files.
  Original hash: sha256:a1b2c3d4e5f6g7h8
  Current hash:  sha256:x9y8z7w6v5u4t3s2

  Options:
    [k] Keep local version (skip update)
    [u] Update (will overwrite your changes)

  Choose: k

  Skipping O-IntonationPad (keeping local version)

[2/2] O-Marimba
  Installed: 1.0.0
  Status: Clean (no modifications)

  Update to 1.1.0? [y/n]: y

  Saving rollback...
  Backed up to .planning/workflow/rollback/O-Marimba/scala-tuning-engine/
  Copying new files...
  Module updated
  Registry updated
  Building...
  O-Marimba_VST3 built
  O-Marimba_AU built

Upgrade complete!
─────────────────────────────────────────────────────────────
Module: scala-tuning-engine 1.0.0 -> 1.1.0

Results:
  O-IntonationPad: Skipped (local customizations kept)
  O-Marimba: Updated successfully
```

## Example with Major Version Update

```
/module:upgrade preset-manager

Checking module: preset-manager
─────────────────────────────────────────────────────────────

Current version: 1.2.0
Latest version: 2.0.0

⚠️  MAJOR VERSION UPDATE

This is a breaking change. Review the changelog carefully.

Changelog (1.2.0 -> 2.0.0):
  BREAKING CHANGES:
  - initPreset() now requires PresetMetadata parameter
  - PresetCategory enum renamed to Category
  - Factory presets moved from presets/ to factory-presets/

  New features:
  - Added preset tagging system
  - Added preset search
  - Added preset import/export

Dependent plugins (2):

[1/2] OuariconMarimba
  Installed: 1.2.0
  Status: Clean (no modifications)

  ⚠️  BREAKING CHANGE: Major version update (1.x -> 2.x)

  This update will require code changes. Proceed? [y/n]: y

  Saving rollback...
  ...
```

## Example with Rollback

```
[2/2] O-Tremolo
  Installed: 1.0.0
  Status: Clean (no modifications)

  Update to 2.0.0? [y/n]: y

  Saving rollback...
  Backed up to .planning/workflow/rollback/O-Tremolo/preset-manager/
  Copying new files...
  Module updated
  Registry updated
  Building...

  BUILD FAILED

  Error output:
  O-Tremolo/Source/PluginProcessor.cpp:42:5: error: no matching
  function for call to 'initPreset'
    initPreset();
    ^~~~~~~~~~
  note: candidate function not viable: requires 1 argument, but
  0 were provided

  Rollback to previous version? [y/n]: y

  Restoring from backup...
  Registry reverted to 1.0.0
  Rebuilding...
  O-Tremolo_VST3 built
  O-Tremolo_AU built

  Rolled back successfully.
```

## Error Cases

### Module Not Found

```
/module:upgrade nonexistent-module

Error: Module 'nonexistent-module' not found in registry.

Available modules:
  - preset-manager (1.0.0)
  - vu-meter (1.0.0)
  - scala-tuning-engine (1.0.0)
```

### No Dependents

```
/module:upgrade playable-keyboard

Module: playable-keyboard
Version: 1.0.0

No plugins are using this module.
Nothing to upgrade.
```

### Already Up-to-date

```
/module:upgrade scala-tuning-engine

Module: scala-tuning-engine
All plugins are already on the latest version (1.0.0).
Nothing to upgrade.
```

## Scripts Used

| Script | Purpose |
|--------|---------|
| `modules/scripts/semver.py compare` | Version comparison |
| `modules/scripts/semver.py is-major` | Detect breaking changes |
| `modules/scripts/check-customizations.py` | Detect local modifications |
| `modules/scripts/compute-hash.py` | Calculate content hash |

## Related Commands

- `/module:info [name]` - View current module version
- `/module:list` - See all modules
- `/module:add [plugin] [module]` - Add module to plugin
- `/module:upgrade-all` - Upgrade all modules across all plugins
