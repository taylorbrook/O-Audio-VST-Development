---
name: destroy
description: Completely remove plugin - source code, binaries, registry entries, everything
argument-hint: "[PluginName]"
---

# /destroy - Nuclear Cleanup

**Purpose:** Direct shortcut to completely remove a plugin. Invokes plugin-lifecycle skill with mode: 'destroy'.

**⚠️ WARNING:** This is irreversible (except via backup). Use `/reset-to-ideation` if you want to keep the idea/mockups.

## Behavior

<preconditions enforcement="blocking">
  <check target="argument" condition="not_empty">
    Plugin name argument MUST be provided
    Usage: /destroy [PluginName]
  </check>
  <check target="PLUGINS.md" condition="plugin_exists">
    Plugin entry for $1 MUST exist in PLUGINS.md
    Run /dream to create new plugins
  </check>
  <check target="status" condition="not_equal(🚧)">
    Plugin status MUST NOT be 🚧 In Progress
    Complete or abandon current work before destroying
  </check>
</preconditions>

<routing>
  When preconditions verified, invoke the plugin-lifecycle skill with mode: 'destroy'.
</routing>

## Quick Reference

**What gets removed:**
- Everything: source code, binaries, build artifacts, PLUGINS.md entry

**Safety features:**
- Timestamped backup created before deletion
- Requires typing exact plugin name to confirm
- Blocks if status is 🚧 (protects in-progress work)

## Implementation

See `.claude/skills/plugin-lifecycle/references/mode-4-destroy.md` for complete destruction workflow.

## State File Contract

**Reads:**
- `PLUGINS.md` - Validates plugin exists, checks status field
- `plugins/[PluginName]/` - Source directory to be removed

**Writes:**
- `PLUGINS.md` - Removes plugin entry
- `backups/destroyed/[PluginName]_[timestamp].tar.gz` - Creates backup archive

**Modified by skill:** All state updates handled by plugin-lifecycle skill

## Example Confirmation

```
⚠️  WARNING: Destructive Operation

About to PERMANENTLY DELETE:
- Source: plugins/[PluginName]/ (124 files, 3.2 MB)
- Binaries: VST3 + AU (8.1 MB total)
- Build artifacts: build/plugins/[PluginName]/ (45 MB)
- PLUGINS.md entry

A backup will be created in backups/destroyed/

This CANNOT be undone (except by restoring backup).

Type the plugin name to confirm: _
```

## Success Output

```
✓ [PluginName] DESTROYED

Removed:
- Source code: plugins/[PluginName]/ (3.2 MB)
- VST3: ~/Library/Audio/Plug-Ins/VST3/[ProductName].vst3 (4.1 MB)
- AU: ~/Library/Audio/Plug-Ins/Components/[ProductName].component (4.0 MB)
- Build artifacts: build/plugins/[PluginName]/ (45 MB)
- PLUGINS.md entry

Backup available at:
backups/destroyed/[PluginName]_20251110_234512.tar.gz (3.5 MB)
```

## Routes To

`plugin-lifecycle` skill (mode: 'destroy')

## Related Commands

- `/uninstall` - Remove binaries only, keep source
- `/reset-to-ideation` - Remove implementation, keep idea/mockups
- `/clean` - Interactive menu to choose cleanup operation
