---
name: module:remove
description: Stop tracking module updates for a plugin (code remains)
skill: module-system
args: "[plugin_name] [module_name]"
---

# /module:remove

Stop tracking a module for a plugin. Updates the registry to remove the dependency relationship, but **code remains in the plugin**. Future module updates will not propagate to this plugin.

Think of modules as "copied and tracked" rather than "linked". Removal stops the tracking, not the code.

## Usage

```
/module:remove [plugin_name] [module_name]
```

## Arguments

- `plugin_name` - Target plugin (e.g., `O-IntonationPad`)
- `module_name` - Module to stop tracking (e.g., `scala-tuning-engine`)

## Key Behavior

**Module removal keeps code in plugin, just stops future updates from propagating.**

This is a "soft" removal:
- Registry entries are removed (tracking stops)
- CMakeLists.txt is optionally commented out
- Source files stay in place (plugin still compiles)
- JS files stay in ui/public/modules/
- User can manually delete files if desired

## Workflow (Registry v3.0.0)

### Step 1: Validate inputs

1. Check plugin exists in `.planning/workflow/registry.json`
2. Check module is in plugin's `modules` array
3. If validation fails: report error and stop

### Step 2: Update registry.json

1. **Remove InstalledModule entry** from `plugins.{plugin}.modules` array
2. **Remove plugin from dependents** in `modules.{module}.dependents` array
3. **Increment usage stats**: `modules.{module}.usageStats.removeCount += 1`

### Step 3: Update CMakeLists.txt (optional, ask user)

**Options presented to user:**
- **Option A: Keep as-is** (default) - `ouaricon_add_module()` line stays, code compiles normally
- **Option B: Comment out** - Line is commented, module disabled but easy to re-enable

CMakeLists.txt is NOT automatically modified without user choice.

### Step 4: DO NOT delete files

Files remain in place:
- Source includes from `modules/{category}/{module}/cpp/`
- JS files in `ui/public/modules/`
- Any plugin-local customizations

### Step 5: Report result

Show what was removed from registry and clarify that code remains.

## Example Output

```
/module:remove O-IntonationPad scala-tuning-engine

Removing module: scala-tuning-engine from O-IntonationPad
=========================================================

Step 1: Validating inputs
  [ok] Plugin O-IntonationPad exists in registry
  [ok] Module scala-tuning-engine is installed (v1.0.0)

Step 2: Updating registry.json
  [ok] Removed InstalledModule entry from plugin
  [ok] Removed O-IntonationPad from module dependents
  [ok] Incremented removeCount (now 1)

Step 3: CMakeLists.txt
  [?] What should I do with the ouaricon_add_module() line?
      A) Keep as-is (code still compiles) [default]
      B) Comment out (disable module)

  > User chose: A) Keep as-is
  [ok] CMakeLists.txt unchanged

=========================================================
Module tracking removed successfully.

Module code remains in plugin:
---------------------------------------------------------
  - Source includes from modules/tuning/scala-tuning-engine/cpp/
  - ui/public/modules/tuning-panel.js
  - ui/public/modules/pitch-circle.js

Future module updates will NOT propagate to this plugin.

To fully remove the code:
---------------------------------------------------------
1. Delete the JS files:
   rm plugins/O-IntonationPad/Source/ui/public/modules/tuning-panel.js
   rm plugins/O-IntonationPad/Source/ui/public/modules/pitch-circle.js

2. Remove #include directives from your C++ code:
   - PluginProcessor.h: #include "modules/tuning/..."

3. Remove member variables and usage:
   - OuariconTuningEngine tuningEngine;

4. Comment/remove in CMakeLists.txt:
   # ouaricon_add_module(${PROJECT_NAME} scala-tuning-engine)

5. Rebuild the plugin:
   cd build && ninja O-IntonationPad_VST3 O-IntonationPad_AU
```

## Registry Changes

### Before

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
  },
  "modules": {
    "scala-tuning-engine": {
      "dependents": ["O-IntonationPad", "OuariconMarimba"],
      "usageStats": { "addCount": 2, "removeCount": 0 }
    }
  }
}
```

### After

```json
{
  "plugins": {
    "O-IntonationPad": {
      "modules": []
    }
  },
  "modules": {
    "scala-tuning-engine": {
      "dependents": ["OuariconMarimba"],
      "usageStats": { "addCount": 2, "removeCount": 1 }
    }
  }
}
```

## Why Soft Removal?

1. **Safety:** User code may depend on the module. Hard deletion could break builds.
2. **Flexibility:** User might want to keep customized module code but stop receiving updates.
3. **Reversibility:** Easy to re-add tracking without reinstalling files.
4. **Clarity:** Clear separation between "tracking" and "code presence".

## Comparison: Remove vs Full Delete

| Aspect | `/module:remove` | Manual full delete |
|--------|------------------|-------------------|
| Registry entry | Removed | Removed |
| CMakeLists.txt | User choice | Must edit |
| Source files | Kept | Deleted |
| JS files | Kept | Deleted |
| Future updates | Stopped | N/A |
| Plugin compiles | Yes | After code cleanup |

## Error Handling

**Principle:** Partial state + error on failure

If any step fails:
1. Leave what succeeded (don't rollback)
2. Report what failed with clear error message
3. Stop execution
4. User can manually fix and retry

### Common Errors

| Error | Cause | Fix |
|-------|-------|-----|
| Plugin not found | Typo in name | Check `/plugin:list` |
| Module not installed | Module not in plugin's array | Nothing to remove |

## Related Commands

- `/module:add [plugin] [module]` - Add module back with tracking
- `/module:list` - Available modules
- `/module:upgrade [module]` - Update module (requires it to be tracked)
