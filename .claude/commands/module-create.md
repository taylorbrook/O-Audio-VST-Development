---
name: module:create
description: Extract code from plugin into reusable module
skill: module-system
args: "[module_name] --from [plugin_name]"
---

# /module:create

Extract code from an existing plugin into a reusable module. The source plugin becomes the first dependent. Includes backup and rollback on failure.

## Usage

```
/module:create [module_name] --from [plugin_name]
```

## Arguments

- `module_name` - Name for the new module (e.g., `microtonality`)
- `--from plugin_name` - Source plugin to extract from

## Workflow

```
┌──────────────────────────────────────────────────────────────┐
│ 1. IDENTIFY - Select files/classes to extract               │
│ 2. BACKUP - Archive source plugin state                     │
│ 3. CREATE - Build module directory structure                │
│ 4. MOVE - Transfer files to module                          │
│ 5. UPDATE - Modify plugin to use module                     │
│ 6. VALIDATE - Build and test plugin                         │
│ 7. FINALIZE - Commit or rollback                            │
└──────────────────────────────────────────────────────────────┘
```

## Interactive Example

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
4. tuning (recommended based on file names)
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
✓ Changes committed
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

## Rollback on Failure

If the build fails after extraction:

```
Step 8: Validating build...
Building O-Lyrica_VST3...
✗ Build failed

Error: undefined reference to 'ScalaParser::parse'

Rolling back...
✓ Files restored from backup
✓ CMakeLists.txt restored
✓ Changes reverted

Extraction failed: Missing dependency in extracted files.
Suggestion: Include ScalaParser.h and ScalaParser.cpp in selection.
```

## Generated module.yaml

```yaml
name: microtonality
version: 1.0.0
description: |
  Extracted from O-Lyrica.
  [Add detailed description]

category: tuning
author: Ouaricon Audio
origin: O-Lyrica

provides:
  cpp-header: OuariconTuningEngine.h
  js-module: tuning-panel.js
  js-module: pitch-circle.js

config: {}
dependencies: []

requirements:
  juce_modules:
    - juce_audio_processors
  cpp_standard: 20

sources:
  cpp:
    - OuariconTuningEngine.h
    - OuariconTuningEngine.cpp
    - ScalaParser.h
    - ScalaParser.cpp
  js:
    - tuning-panel.js
    - pitch-circle.js

used_by:
  - plugin: O-Lyrica
    version: 1.0.0

changelog:
  - version: 1.0.0
    date: 2026-01-29
    changes:
      - Initial extraction from O-Lyrica
```

## Related Commands

- `/module:list` - See existing modules
- `/module:add [plugin] [module]` - Add module to another plugin
- `/module:info [name]` - View module details
