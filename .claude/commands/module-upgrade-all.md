---
name: module:upgrade-all
description: Batch upgrade all modules across all plugins with preview
skill: module-system
args: "[--dry-run]"
---

# /module:upgrade-all

Upgrade all modules across all plugins with a comprehensive preview before proceeding. Shows all available updates, breaking changes, and per-plugin status in a single summary.

## Usage

```
/module:upgrade-all [--dry-run]
```

## Arguments

- `--dry-run` - Show preview only, do not make any changes (optional)

## Key Behavior

**Always shows preview before proceeding.** User decision from CONTEXT.md: "/module:upgrade-all exists with preview before proceeding"

This command is the batch equivalent of `/module:upgrade`, applying the same per-plugin workflow to all outdated modules at once.

## Workflow

### 1. Scan All Modules

For each module in `registry.modules`:
1. Get current version from registry
2. For each plugin in registry with this module installed:
   - Get installed version from `plugins.{plugin}.modules[]`
   - Compare versions using `python3 modules/scripts/semver.py compare`
   - If update available, add to results

### 2. Show Preview (Always)

```
/module:upgrade-all

Module Update Preview
=====================================================================

scala-tuning-engine: 1.0.0 -> 1.1.0
  Plugins: O-IntonationPad (modified), O-Marimba
  Breaking: No

preset-manager: 1.0.0 -> 1.2.0
  Plugins: O-Marimba, O-Tremolo, O-Lyrica
  Breaking: No

analog-eq-unit: 1.2.0 -> 2.0.0
  Plugins: O-Marimba
  Breaking: YES (major version)

Summary:
  3 modules with updates
  5 plugin updates total
  1 breaking change

Proceed with upgrades? [y/n]:
```

**Key elements in preview:**
- Module name with version change
- List of plugins using the module
- "(modified)" indicator for plugins with local customizations
- Breaking change warning for major version updates
- Summary counts

### 3. If Confirmed (or not --dry-run)

For each module with updates:
1. Run the same workflow as `/module:upgrade`
2. For each dependent plugin:
   - Check for customizations
   - If customized: ask keep/update choice
   - If clean: proceed with update
   - Store rollback before updating
   - Update files and registry
   - Trigger rebuild
   - On build failure: offer rollback

### 4. Report Summary

```
Upgrade Complete
---------------------------------------------------------------------
Modules updated: 3
Plugins updated: 4
Skipped (customizations): 1

Details:
  scala-tuning-engine:
    O-IntonationPad: Skipped (local customizations)
    O-Marimba: Updated 1.0.0 -> 1.1.0

  preset-manager:
    O-Marimba: Updated 1.0.0 -> 1.2.0
    O-Tremolo: Updated 1.0.0 -> 1.2.0
    O-Lyrica: Updated 1.0.0 -> 1.2.0

  analog-eq-unit:
    O-Marimba: Updated 1.2.0 -> 2.0.0
```

## --dry-run Flag

Shows preview only, does not prompt or make changes.

```
/module:upgrade-all --dry-run

Module Update Preview (DRY RUN)
=====================================================================

scala-tuning-engine: 1.0.0 -> 1.1.0
  Plugins: O-IntonationPad (modified), O-Marimba
  Breaking: No

...

Summary:
  3 modules with updates
  5 plugin updates total
  1 breaking change

No changes made (dry run).
```

Use `--dry-run` to check what would happen before committing to upgrades.

## Complete Example with Mixed Results

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

analog-eq-unit: 1.2.0 -> 2.0.0
  Plugins: O-Tremolo
  Breaking: YES (major version)

Summary:
  3 modules with updates
  4 plugin updates total
  1 breaking change

Proceed with upgrades? [y/n]: y

Processing scala-tuning-engine (1.0.0 -> 1.1.0)
---------------------------------------------------------------------

[1/2] O-IntonationPad
  Status: LOCAL CUSTOMIZATIONS DETECTED

  This plugin has modified the module files.
  Options:
    [k] Keep local version (skip update)
    [u] Update (will overwrite your changes)

  Choose: k

  Skipped (keeping local version)

[2/2] O-Marimba
  Status: Clean (no modifications)

  Saving rollback...
  Copying new files...
  Updating registry...
  Building...
  Build succeeded.

Processing preset-manager (1.0.0 -> 1.2.0)
---------------------------------------------------------------------

[1/2] O-Marimba
  Status: Clean (no modifications)

  Saving rollback...
  Copying new files...
  Updating registry...
  Building...
  Build succeeded.

[2/2] O-Tremolo
  Status: Clean (no modifications)

  Saving rollback...
  Copying new files...
  Updating registry...
  Building...
  Build succeeded.

Processing analog-eq-unit (1.2.0 -> 2.0.0)
---------------------------------------------------------------------

[1/1] O-Tremolo
  Status: Clean (no modifications)

  WARNING: BREAKING CHANGE - Major version update (1.x -> 2.x)

  Saving rollback...
  Copying new files...
  Updating registry...
  Building...

  BUILD FAILED

  Error: 'EQBandMode' is not defined

  Rollback to previous version? [y/n]: y

  Restoring from backup...
  Registry reverted to 1.2.0
  Rebuilding...
  Build succeeded.

  Rolled back successfully.

Upgrade Complete
=====================================================================
Modules updated: 2
Plugins updated: 3
Skipped (customizations): 1
Rolled back (build failure): 1

Details:
  scala-tuning-engine:
    O-IntonationPad: Skipped (local customizations)
    O-Marimba: Updated 1.0.0 -> 1.1.0

  preset-manager:
    O-Marimba: Updated 1.0.0 -> 1.2.0
    O-Tremolo: Updated 1.0.0 -> 1.2.0

  analog-eq-unit:
    O-Tremolo: Rolled back (build failed)
```

## Error Cases

### No Updates Available

```
/module:upgrade-all

All modules are up to date.
Nothing to upgrade.
```

### No Modules Installed

```
/module:upgrade-all

No modules are installed in any plugins.
Use /module:add [plugin] [module] to add modules.
```

## Customization Detection

Uses the same customization detection as `/module:upgrade`:

```bash
python3 modules/scripts/check-customizations.py <plugin> <module>
```

- Exit code 0: Clean (no modifications)
- Exit code 1: Modified (local customizations detected)
- Exit code 2: Error

Modified plugins are marked with "(modified)" in the preview and prompt for keep/update choice.

## Scripts Used

| Script | Purpose |
|--------|---------|
| `modules/scripts/semver.py compare` | Version comparison |
| `modules/scripts/semver.py is-major` | Detect breaking changes |
| `modules/scripts/check-customizations.py` | Detect local modifications |
| `modules/scripts/compute-hash.py` | Calculate content hash |

## Related Commands

- `/module:upgrade [module]` - Upgrade single module
- `/module:list` - See all modules
- `/module:info [name]` - View module details
- `/plugin:focus [plugin]` - Shows update notification if available
