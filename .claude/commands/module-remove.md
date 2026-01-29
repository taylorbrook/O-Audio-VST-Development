---
name: module:remove
description: Remove module dependency from a plugin
skill: module-system
args: "[plugin_name] [module_name]"
---

# /module:remove

Remove a module dependency from a plugin. Warns about code references that need manual cleanup.

## Usage

```
/module:remove [plugin_name] [module_name]
```

## Arguments

- `plugin_name` - Target plugin (e.g., `O-IntonationPad`)
- `module_name` - Module to remove (e.g., `scala-tuning-engine`)

## Behavior

1. Verifies module is used by plugin
2. Scans for code references (includes, usages)
3. Warns about manual cleanup needed
4. Asks for confirmation
5. Removes from CMakeLists.txt
6. Updates modules.json
7. Removes JS files from plugin's modules folder
8. Updates registry.yaml `used_by`

## Example

```
/module:remove O-IntonationPad scala-tuning-engine

Removing module: scala-tuning-engine
─────────────────────────────────────────────────────────────

⚠ Warning: Code references found:
  - PluginProcessor.h:15 - #include "...OuariconTuningEngine.h"
  - PluginProcessor.cpp:45 - tuningEngine.setTuningMode(...)
  - PluginProcessor.cpp:72 - tuningEngine.getFrequency(...)
  - index.html:8 - import { TuningPanel } from './modules/tuning-panel.js'

These references will need manual cleanup after removal.

Continue? (y/n): y

✓ Removed from CMakeLists.txt
✓ Updated modules.json
✓ Removed tuning-panel.js
✓ Removed pitch-circle.js
✓ Updated registry.yaml

⚠ Manual cleanup required:
  Remove the code references listed above, then rebuild.
```

## Related Commands

- `/module:add [plugin] [module]` - Add module back
- `/module:list` - Available modules
