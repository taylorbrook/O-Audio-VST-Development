---
name: module-system
description: Manage Ouaricon shared modules - list, add, remove, create (extract), upgrade. Enables code reuse across plugins with versioning and dependency tracking.
allowed-tools:
  - Read
  - Write
  - Edit
  - Bash
  - Glob
  - Grep
  - AskUserQuestion
commands:
  - name: module:list
    description: List all available modules
  - name: module:info
    description: Show module details and dependents
    args: "[module_name]"
  - name: module:add
    description: Add module dependency to plugin
    args: "[plugin_name] [module_name]"
  - name: module:remove
    description: Remove module dependency from plugin
    args: "[plugin_name] [module_name]"
  - name: module:create
    description: Extract code from plugin into reusable module
    args: "[module_name] --from [plugin_name]"
  - name: module:upgrade
    description: Upgrade module and rebuild dependents
    args: "[module_name]"
---

# module-system Skill

**Purpose:** Manage the Ouaricon Module System - reusable, versioned components for JUCE plugin development. Enables code sharing across plugins with proper dependency tracking.

## Module Structure

```
modules/
├── cmake/
│   └── OuariconModules.cmake    # CMake integration
├── registry.yaml                 # Master module index
├── [category]/
│   └── [module-name]/
│       ├── module.yaml           # Module metadata
│       ├── README.md             # Documentation
│       ├── cpp/                  # C++ source files
│       │   └── *.h, *.cpp
│       ├── js/                   # JavaScript modules
│       │   └── *.js
│       └── tests/                # Optional tests
```

## Commands

### /module:list

List all available modules grouped by category.

**Output:**
```
Ouaricon Module System
═══════════════════════════════════════════════════════════════

  CORE
  ────────────────────────────────────────────────────────────
  webview-relay-manager    v1.0.0    Relay lifecycle management
  resource-provider        v1.0.0    WebView resource serving

  PERSISTENCE
  ────────────────────────────────────────────────────────────
  preset-manager           v1.0.0    JSON preset system

  METERING
  ────────────────────────────────────────────────────────────
  vu-meter                 v1.0.0    Thread-safe VU metering

  TUNING
  ────────────────────────────────────────────────────────────
  scala-tuning-engine      v1.0.0    Microtonal tuning system

  EFFECTS
  ────────────────────────────────────────────────────────────
  analog-eq-unit           v1.2.0    4-band parametric EQ
  compressor-unit          v1.2.1    Dynamics compressor

  UI
  ────────────────────────────────────────────────────────────
  playable-keyboard        v1.0.0    Interactive piano keyboard

Total: 8 modules
```

**Implementation:**
```bash
python3 modules/scripts/list-modules.py
```

### /module:info [module_name]

Show detailed information about a module.

**Output:**
```
Module: scala-tuning-engine
═══════════════════════════════════════════════════════════════

Version: 1.0.0
Category: tuning
Origin: O-Lyrica

Description:
Complete microtonal tuning system supporting 12-TET, custom scales
via Scala (.scl/.kbm) files, and MTS-ESP integration.

Provides:
  - cpp-class: OuariconTuningEngine
  - js-module: tuning-panel.js
  - js-module: pitch-circle.js

Native Functions:
  - loadScalaFile, loadKBMFile
  - saveScalaFile, saveKBMFile
  - setTuningIntervals, getTuningIntervals
  - setTonicNote

Dependencies: (none)

Used by:
  - O-Lyrica (origin)
  - O-Marimba

Configuration:
  default_mode: "12tet"
  reference_pitch_default: 440.0
```

### /module:add [plugin_name] [module_name]

Add a module dependency to a plugin.

**Implementation:**
1. Verify plugin exists
2. Verify module exists in registry
3. Check for existing dependency (skip if already added)
4. Update plugin's CMakeLists.txt:
   ```cmake
   include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
   ouaricon_add_module(${PROJECT_NAME} [module_name])
   ```
5. Create/update `plugins/[Name]/.planning/modules.json`
6. Update module's `used_by` in registry.yaml
7. Copy JS files to plugin's `Source/ui/public/modules/`
8. Show integration instructions

**Example:**
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

### /module:remove [plugin_name] [module_name]

Remove a module dependency from a plugin.

**Implementation:**
1. Verify module is used by plugin
2. Remove from CMakeLists.txt
3. Update modules.json
4. Remove JS files from plugin's modules folder
5. Update registry.yaml `used_by`
6. Warn about code that may need updating

**Example:**
```
/module:remove O-IntonationPad scala-tuning-engine

Removing module: scala-tuning-engine
─────────────────────────────────────────────────────────────

⚠ Warning: Code references found:
  - PluginProcessor.h:15 - #include "...OuariconTuningEngine.h"
  - PluginProcessor.cpp:45 - tuningEngine.setTuningMode(...)

These references will need manual cleanup.

Continue? (y/n): y

✓ Removed from CMakeLists.txt
✓ Updated modules.json
✓ Removed tuning-panel.js
✓ Removed pitch-circle.js
✓ Updated registry.yaml

Manual cleanup required - see warnings above.
```

### /module:create [module_name] --from [plugin_name]

Extract code from a plugin into a reusable module. The original plugin becomes the first dependent.

**Workflow:**
```
┌──────────────────────────────────────────────────────────────┐
│ 1. IDENTIFY CODE                                             │
│    - User specifies files/classes to extract                 │
│    - Scan for dependencies and usages                        │
├──────────────────────────────────────────────────────────────┤
│ 2. CREATE BACKUP                                             │
│    - Archive source plugin state                             │
│    - Store in .backup/[timestamp]/                           │
├──────────────────────────────────────────────────────────────┤
│ 3. CREATE MODULE                                             │
│    - Create module directory structure                       │
│    - Move source files to module                             │
│    - Generate module.yaml metadata                           │
│    - Generate README.md documentation                        │
├──────────────────────────────────────────────────────────────┤
│ 4. UPDATE PLUGIN                                             │
│    - Update CMakeLists.txt to use module                     │
│    - Update includes to module paths                         │
│    - Create modules.json dependency file                     │
├──────────────────────────────────────────────────────────────┤
│ 5. VALIDATE                                                  │
│    - Build plugin with module dependency                     │
│    - Verify plugin still works                               │
├──────────────────────────────────────────────────────────────┤
│ 6. FINALIZE OR ROLLBACK                                      │
│    - If validation passes: commit changes                    │
│    - If validation fails: restore from backup                │
└──────────────────────────────────────────────────────────────┘
```

**Implementation:**
```python
def create_module(module_name, source_plugin, files_to_extract, category):
    # 1. Create backup
    backup_path = create_backup(source_plugin)

    try:
        # 2. Determine module path
        module_path = f"modules/{category}/{module_name}"

        # 3. Create module structure
        create_module_structure(module_path)

        # 4. Move files
        for file in files_to_extract:
            if file.endswith('.h') or file.endswith('.cpp'):
                move_to(file, f"{module_path}/cpp/")
            elif file.endswith('.js'):
                move_to(file, f"{module_path}/js/")

        # 5. Generate module.yaml
        generate_module_yaml(module_path, module_name, source_plugin, files_to_extract)

        # 6. Update source plugin
        update_plugin_cmake(source_plugin, module_name)
        update_plugin_includes(source_plugin, module_path)
        create_modules_json(source_plugin, module_name)

        # 7. Update registry
        add_to_registry(module_name, module_path, category, source_plugin)

        # 8. Validate build
        if not build_plugin(source_plugin):
            raise BuildError("Plugin build failed")

        # 9. Commit
        commit_extraction(module_name, source_plugin)
        remove_backup(backup_path)

        return Success(f"Module {module_name} created from {source_plugin}")

    except Exception as e:
        # Rollback
        restore_from_backup(backup_path)
        return Error(f"Extraction failed: {e}")
```

**Interactive flow:**
```
/module:create microtonality --from O-Lyrica

Creating module: microtonality
Source plugin: O-Lyrica
═══════════════════════════════════════════════════════════════

Step 1: Identify code to extract
─────────────────────────────────────────────────────────────

Which files should be extracted?

C++ files in O-Lyrica/Source:
  1. [ ] OuariconTuningEngine.h
  2. [ ] OuariconTuningEngine.cpp
  3. [ ] ScalaParser.h
  4. [ ] ScalaParser.cpp

JS files in O-Lyrica/Source/ui/public:
  5. [ ] tuning-panel.js
  6. [ ] pitch-circle.js

Enter numbers to select (e.g., 1,2,3,5,6): 1,2,3,4,5,6

Step 2: Module category
─────────────────────────────────────────────────────────────

1. core
2. persistence
3. metering
4. tuning (recommended)
5. effects
6. ui

Choose category: 4

Step 3: Creating backup...
✓ Backup created: .backup/2026-01-29T10-30-00/

Step 4: Creating module structure...
✓ modules/tuning/microtonality/
✓ modules/tuning/microtonality/cpp/
✓ modules/tuning/microtonality/js/

Step 5: Moving files...
✓ OuariconTuningEngine.h → cpp/
✓ OuariconTuningEngine.cpp → cpp/
✓ ScalaParser.h → cpp/
✓ ScalaParser.cpp → cpp/
✓ tuning-panel.js → js/
✓ pitch-circle.js → js/

Step 6: Generating module.yaml...
✓ module.yaml created

Step 7: Updating O-Lyrica...
✓ CMakeLists.txt updated
✓ Include paths updated
✓ modules.json created

Step 8: Validating build...
Building O-Lyrica_VST3...
✓ Build successful

Step 9: Finalizing...
✓ Registry updated
✓ Changes committed: "module: extract microtonality from O-Lyrica"
✓ Backup removed

Module created successfully!
─────────────────────────────────────────────────────────────

Path: modules/tuning/microtonality
Version: 1.0.0
Origin: O-Lyrica
Dependents: O-Lyrica

Use in other plugins:
  /module:add [PluginName] microtonality
```

### /module:upgrade [module_name]

Upgrade a module and rebuild all dependent plugins.

**Implementation:**
1. Read module's `used_by` from registry
2. For each dependent plugin:
   - Clear CMake cache
   - Rebuild plugin
   - Run validation
3. Report results

## Module YAML Schema

```yaml
name: [module-name]
version: [semver]
description: |
  Multi-line description of what the module does.

category: [core|persistence|metering|tuning|modulation|synthesis|effects|ui]
author: Ouaricon Audio

provides:
  cpp-header: [HeaderFile.h]
  cpp-class: [ClassName]
  js-module: [module.js]
  native-functions:
    - [functionName]

config:
  [config_key]:
    type: [string|integer|float|boolean]
    default: [value]
    description: [what it does]

dependencies:
  - [other-module-name]

requirements:
  juce_modules:
    - [juce_module_name]
  cpp_standard: [17|20]

sources:
  cpp:
    - [file.h]
    - [file.cpp]
  js:
    - [file.js]

used_by:
  - plugin: [PluginName]
    version: [version-when-added]

changelog:
  - version: [version]
    date: [YYYY-MM-DD]
    changes:
      - [change description]
```

## State Files

### Plugin's modules.json

Located at `plugins/[Name]/.planning/modules.json`:

```json
{
  "plugin": "O-IntonationPad",
  "modules": [
    {
      "name": "scala-tuning-engine",
      "version": "1.0.0",
      "added": "2026-01-29"
    },
    {
      "name": "webview-relay-manager",
      "version": "1.0.0",
      "added": "2026-01-29"
    }
  ]
}
```

### Registry Entry

In `modules/registry.yaml`:

```yaml
modules:
  - name: microtonality
    path: tuning/microtonality
    version: 1.0.0
    description: |
      Complete microtonal tuning system...
    category: tuning
    origin: O-Lyrica
    provides:
      - cpp-class: OuariconTuningEngine
      - js-module: tuning-panel.js
    tags: [tuning, scala, microtuning]
    reuse_score: 9
    used_by:
      - plugin: O-Lyrica
        version: 1.0.0
```

## Error Handling

**Module not found:**
```
Error: Module 'foo-bar' not found.

Available modules:
  /module:list

Create new module:
  /module:create foo-bar --from [PluginName]
```

**Extraction build failure:**
```
✗ Build failed after extraction

Error: undefined reference to 'OuariconTuningEngine::initialize'

Rolling back...
✓ Restored from backup
✓ Changes reverted

The extraction failed because the module has hidden dependencies.
Review the code and try again with all necessary files selected.
```

## Integration Points

**Reads:**
- `modules/registry.yaml`
- `modules/[category]/[name]/module.yaml`
- `plugins/[Name]/CMakeLists.txt`
- `plugins/[Name]/.planning/modules.json`

**Writes:**
- `modules/registry.yaml`
- `modules/[category]/[name]/*`
- `plugins/[Name]/CMakeLists.txt`
- `plugins/[Name]/.planning/modules.json`
- `plugins/[Name]/Source/ui/public/modules/*.js`
