---
name: module:add
description: Add module dependency to a plugin with registry tracking
skill: module-system
args: "[plugin_name] [module_name]"
---

# /module:add

Add a module dependency to a plugin. Updates CMakeLists.txt, copies JS files, tracks installation in the central registry with content hash, and triggers rebuild.

## Usage

```
/module:add [plugin_name] [module_name]
```

## Arguments

- `plugin_name` - Target plugin (e.g., `O-IntonationPad`)
- `module_name` - Module to add (e.g., `scala-tuning-engine`)

## Workflow (Registry v3.0.0)

### Step 1: Validate inputs

1. Check plugin exists in `PLUGINS.md` (and that `plugins/{plugin}/` is on disk)
2. Check module exists in `modules/registry.yaml` under `modules:`
3. Check plugin is not already listed in that module's `used_by:` entries
4. If validation fails: report error and stop

### Step 2: Read module metadata

1. Get `version`, `path`, `category` from that module's entry in `modules/registry.yaml`
2. Read integration info from `modules/{category}/{module}/module.yaml` if present

### Step 3: Compute content hash

1. Run: `python3 modules/scripts/compute-hash.py modules/{category}/{module}`
2. Store result as both `contentHash` and `originalHash` (identical on fresh install)
3. Hash format: `sha256:{16 hex chars}`

### Step 4: Update CMakeLists.txt

1. Check for `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)`
2. If missing, add it after `project()` declaration
3. Add `ouaricon_add_module(${PROJECT_NAME} {module})` line
4. If already present, skip (module already added at CMake level)

### Step 5: Copy JS files (if present)

1. Check for `modules/{category}/{module}/js/` directory
2. Copy all `.js` files to `plugins/{plugin}/Source/ui/public/modules/`
3. Create destination directory if needed

### Step 6: Update modules/registry.yaml

The registry stores the relation as `used_by` **per module**, not as a `modules` array per plugin.

1. **Add the plugin to that module's `used_by:` list** (if not already present):
   ```yaml
   used_by:
     - plugin: {plugin}
       version: {plugin version from its CMakeLists.txt}
   ```

2. **Bump `version` and `last_updated`** at the top of `modules/registry.yaml`

3. Preferred: run `scripts/regen-registry-used-by.sh`, which regenerates every `used_by`
   list from disk truth and bumps the header automatically — then confirm the diff

### Step 7: Trigger build

1. Run: `cd build && ninja {plugin}_VST3 {plugin}_AU`
2. Report build success or failure

### Step 8: Report result

Show each step with status and provide integration instructions.

## Example Output

```
/module:add O-IntonationPad scala-tuning-engine

Adding module: scala-tuning-engine v1.0.0 to O-IntonationPad
=========================================================

Step 1: Validating inputs
  [ok] Plugin O-IntonationPad exists in registry
  [ok] Module scala-tuning-engine exists (v1.0.0)
  [ok] Module not already installed

Step 2: Reading module metadata
  [ok] Category: tuning
  [ok] Path: modules/tuning/scala-tuning-engine

Step 3: Computing content hash
  [ok] Hash: sha256:b6c81f83d245841c

Step 4: Updating CMakeLists.txt
  [ok] OuariconModules.cmake include present
  [ok] Added ouaricon_add_module(${PROJECT_NAME} scala-tuning-engine)

Step 5: Copying JS files
  [ok] Copied tuning-panel.js to ui/public/modules/
  [ok] Copied pitch-circle.js to ui/public/modules/

Step 6: Updating modules/registry.yaml
  [ok] Added O-IntonationPad to scala-tuning-engine used_by
  [ok] Bumped registry version + last_updated

Step 7: Building plugin
  [ok] ninja O-IntonationPad_VST3 O-IntonationPad_AU succeeded

=========================================================
Module added successfully!

Integration Instructions:
---------------------------------------------------------

1. Include header in PluginProcessor.h:
   #include "modules/tuning/scala-tuning-engine/cpp/OuariconTuningEngine.h"

2. Add member variable:
   OuariconTuningEngine tuningEngine;

3. Import JS module in your UI:
   import { TuningPanel } from './modules/tuning-panel.js';
   import { PitchCircle } from './modules/pitch-circle.js';

Rebuild with: ninja O-IntonationPad_VST3 O-IntonationPad_AU
```

## Error Handling

**Principle:** Partial state + error on failure

If any step fails:
1. Leave what succeeded (don't rollback)
2. Report what failed with clear error message
3. Stop execution
4. User can manually fix and retry

**Do NOT:**
- Auto-repair without prompting
- Silently fix issues
- Continue past failures

### Common Errors

| Error | Cause | Fix |
|-------|-------|-----|
| Plugin not found | Typo in name | Check `/plugin:list` |
| Module not found | Typo or missing registration | Check the `modules:` section of `modules/registry.yaml` |
| Already installed | Plugin already in the module's `used_by` list | No action needed, or use `/module:upgrade` |
| Build failed | Missing dependencies | Check ninja output, fix code |

## CMakeLists.txt Changes

```cmake
# Added by /module:add (if not present)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Added for this module
ouaricon_add_module(${PROJECT_NAME} scala-tuning-engine)
```

## Registry Changes

Edits land in `modules/registry.yaml`, which stores `used_by` per module.

### Before

```yaml
  - name: scala-tuning-engine
    path: tuning/scala-tuning-engine
    version: 1.0.0
    category: tuning
    used_by: []
```

### After

```yaml
  - name: scala-tuning-engine
    path: tuning/scala-tuning-engine
    version: 1.0.0
    category: tuning
    used_by:
      - plugin: O-IntonationPad
        version: 2.8.0
```

## Related Commands

- `/module:list` - See available modules
- `/module:info [name]` - Module details before adding
- `/module:remove [plugin] [module]` - Stop tracking (keeps code)
- `/module:upgrade [module]` - Update to latest version
