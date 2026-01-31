---
name: plugin-focus
description: Switch focus to a specific plugin with isolated state loading
skill: plugin-context
args: "[plugin_name]"
---

# /focus {plugin}

Switch focus to a specific plugin, loading only that plugin's state. Implements plugin state isolation to prevent context pollution from other plugins.

## Usage

```
/plugin-focus [plugin_name]    # Set focused plugin
/plugin-focus                  # Show current focus
```

## Arguments

- `plugin_name` - Plugin to focus on (optional)

## Plugin Isolation Principle

When focused on a plugin:
- Load ONLY that plugin's `.planning/` state
- Do NOT load other plugins' STATUS.md or plans
- Central registry read-only for awareness
- Module dependencies tracked but not cross-loaded

## Process

1. **Validate plugin exists**
   Check `plugins/{plugin}/` directory exists
   Check `.planning/workflow/registry.json` has entry

2. **Run state validation** (for target plugin only)
   @.claude/skills/state-validation/SKILL.md
   Validate registry entry matches STATUS.md
   If issues: offer recovery before switching

3. **Update active plugin state**
   Write to `.planning/workflow/active-plugin.json`:
   ```json
   {
     "plugin": "{plugin}",
     "focusedAt": "{ISO 8601 timestamp}",
     "loadedContext": {
       "statusMd": true,
       "currentPlan": "{path or null}",
       "currentContext": "{path or null}"
     }
   }
   ```

4. **Load plugin context**
   Read `plugins/{plugin}/.planning/STATUS.md`
   Read current stage's CONTEXT.md if exists
   Read current plan if in execute phase
   Load checkpoint if exists

5. **Display focus summary**
   Show: plugin name, stage, phase, status
   Show: loaded context files
   Show: suggested next command

## State Loaded

| File | Loaded | Purpose |
|------|--------|---------|
| STATUS.md | Always | Current plugin state |
| BRIEF.md | On demand | Ideation output |
| ARCHITECTURE.md | On demand | Planning output |
| stages/{stage}/CONTEXT.md | If exists | Stage discussion |
| stages/{stage}/PLAN.md | If in execute | Current plan |
| dependencies.json | Always | Module awareness |

## State NOT Loaded

- Other plugins' STATUS.md
- Other plugins' plans or context
- Central registry details (beyond validation)
- Historical checkpoints from other plugins

This isolation ensures:
- Context budget preserved for actual work
- No confusion from stale state of other plugins
- Clear mental model of current work scope

## Read-Only Peek

To view another plugin without switching focus:
```
/peek {other-plugin}
```
Shows STATUS.md frontmatter only. No context loading. Does not change active-plugin.json.

## Output Format

**Set focus:**
```
FOCUS: O-IntonationPad
======================
Stage: 2-dsp (DSP Implementation)
Phase: execute
Status: active

Context loaded:
- plugins/O-IntonationPad/.planning/STATUS.md
- plugins/O-IntonationPad/.planning/stages/2-dsp/02-01-PLAN.md

Dependencies: juce_audio_processors, juce_audio_basics

State: healthy (validated)

Next: /plugin:execute to continue current plan
      /plugin:status for full context
```

**Show current focus (no argument):**
```
CURRENT FOCUS
=============
Plugin: O-IntonationPad
Stage: 2-dsp
Phase: execute

Focused since: 2026-01-30T14:30:00Z

Next: /continue to resume work
      /plugin-focus {other} to switch
```

**Focus with validation issues:**
```
FOCUS: O-IntonationPad - VALIDATION REQUIRED
============================================

STATE VALIDATION REPORT
=======================
Status: inconsistent
Timestamp: 2026-01-31T10:00:00Z

ISSUES FOUND:
-------------
1. [ERROR] phase_consistency: Registry and STATUS.md disagree
   Registry value: plan
   STATUS.md value: execute
   Recoverable: YES

RECOVERY OPTIONS:
1. Sync registry from STATUS.md (recommended)
2. Sync STATUS.md from registry
3. Manual repair

Select option [1-3] to proceed with focus:
```

**Plugin not found:**
```
Plugin not found: O-NonExistent

Available plugins:
1. O-IntonationPad (Stage 2-dsp, Phase execute)
2. O-AnotherPlugin (Stage 1-foundation, Phase discuss)

Use: /plugin-focus {name}
```

## Examples

**Set focus:**
```
/plugin-focus O-IntonationPad

FOCUS: O-IntonationPad
======================
Stage: 2-dsp, Phase: execute
Status: active

Context loaded. Next: /continue or /plugin:execute
```

**Show current focus:**
```
/plugin-focus

Currently focused: O-IntonationPad
Stage: 2-dsp, Phase: execute
```

## Behavior

Once focused, these commands will use the focused plugin by default:
- `/plugin-status` -> `/plugin-status O-IntonationPad`
- `/plugin-discuss` -> `/plugin-discuss O-IntonationPad`
- `/plugin-plan` -> `/plugin-plan O-IntonationPad`
- `/continue` -> `/continue O-IntonationPad`

## Parallel Instance Support

Multiple Claude Code instances can focus different plugins:
- Each instance writes to active-plugin.json with its focus
- Registry.json is read for awareness, written only for state updates
- No file locking needed for different-plugin focus
- Same-plugin focus from multiple instances: last writer wins (by design)

Note: This is designed for the common case of working on different plugins. If you need to work on the same plugin from multiple instances, coordinate manually.

## Registry Updates

On focus:
1. Update `registry.json` focused field
2. Update `active-plugin.json` with focus state
3. Set `lastActivity` timestamp
4. Validate before completing switch

## Related Commands

- `/continue` - Resume last session (same plugin)
- `/reconcile` - Validate current plugin state
- `/peek {plugin}` - View another plugin without switching
- `/plugin-list` - See all plugins
- `/plugin-status` - Detailed status
