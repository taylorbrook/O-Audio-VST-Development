---
name: module:list
description: List all available Ouaricon modules
skill: module-system
---

# /module:list

List all available modules in the Ouaricon Module System, grouped by category.

## Usage

```
/module:list
```

## Output

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

## Related Commands

- `/module:info [name]` - Detailed module information
- `/module:add [plugin] [module]` - Add module to plugin
- `/module:create [name] --from [plugin]` - Extract new module
