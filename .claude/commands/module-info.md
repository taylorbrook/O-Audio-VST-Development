---
name: module:info
description: Show detailed module information and dependents
skill: module-system
args: "[module_name]"
---

# /module:info

Show detailed information about a module including its API, configuration options, and which plugins use it.

## Usage

```
/module:info [module_name]
```

## Arguments

- `module_name` - Name of the module (e.g., `scala-tuning-engine`)

## Output

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

Path: modules/tuning/scala-tuning-engine/
```

## Related Commands

- `/module:list` - List all modules
- `/module:add [plugin] [module]` - Add this module to a plugin
