---
name: module:add
description: Add module dependency to a plugin
skill: module-system
args: "[plugin_name] [module_name]"
---

# /module:add

Add a module dependency to a plugin. Updates CMakeLists.txt, copies JS files, and provides integration instructions.

## Usage

```
/module:add [plugin_name] [module_name]
```

## Arguments

- `plugin_name` - Target plugin (e.g., `O-IntonationPad`)
- `module_name` - Module to add (e.g., `scala-tuning-engine`)

## Behavior

1. Verifies plugin and module exist
2. Checks if module already added (skips if yes)
3. Updates plugin's CMakeLists.txt with module include
4. Creates/updates modules.json in plugin's .planning/
5. Copies JS files to plugin's ui/public/modules/
6. Updates module's `used_by` in registry.yaml
7. Shows integration instructions

## Example

```
/module:add O-IntonationPad scala-tuning-engine

Adding module: scala-tuning-engine v1.0.0
─────────────────────────────────────────────────────────────

✓ Updated CMakeLists.txt
✓ Created modules.json
✓ Copied tuning-panel.js to ui/public/modules/
✓ Copied pitch-circle.js to ui/public/modules/

Integration:
─────────────────────────────────────────────────────────────
1. Include header in PluginProcessor.h:
   #include "modules/tuning/scala-tuning-engine/cpp/OuariconTuningEngine.h"

2. Add member variable:
   OuariconTuningEngine tuningEngine;

3. Import JS module in index.html:
   import { TuningPanel } from './modules/tuning-panel.js';

Rebuild with: ninja O-IntonationPad_VST3 O-IntonationPad_AU
```

## CMakeLists.txt Changes

```cmake
# Added by /module:add
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(${PROJECT_NAME} scala-tuning-engine)
```

## Related Commands

- `/module:list` - See available modules
- `/module:info [name]` - Module details before adding
- `/module:remove [plugin] [module]` - Remove module
