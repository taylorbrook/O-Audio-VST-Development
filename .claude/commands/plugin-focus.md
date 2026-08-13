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
- `PLUGINS.md` read-only, for roster awareness only
- Module dependencies tracked but not cross-loaded

## Process

1. **Validate plugin exists**
   Check `plugins/{plugin}/` directory exists
   Check `PLUGINS.md` has a row for it

2. **Run state validation** (for target plugin only)
   @.claude/skills/state-validation/SKILL.md
   Validate the PLUGINS.md row matches STATUS.md
   If issues: offer recovery before switching

3. **Update focus state**
   STATUS.md is authoritative — there is no separate focus-state file.
   In `plugins/{plugin}/.planning/STATUS.md` frontmatter, set:
   ```yaml
   focused: true
   focusedAt: "{ISO 8601 timestamp}"
   ```
   Then clear `focused: true` from the previously focused plugin's STATUS.md.

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
| modules/registry.yaml | Always | Module awareness (`used_by`) |

## State NOT Loaded

- Other plugins' STATUS.md
- Other plugins' plans or context
- Other plugins' PLUGINS.md detail (beyond the roster row)
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
Shows STATUS.md frontmatter only. No context loading. Does not change any plugin's `focused` flag.

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
1. [ERROR] phase_consistency: PLUGINS.md and STATUS.md disagree
   PLUGINS.md value: plan
   STATUS.md value: execute
   Recoverable: YES

RECOVERY OPTIONS:
1. Sync the PLUGINS.md row from STATUS.md (recommended)
2. Sync STATUS.md from the PLUGINS.md row
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
- Each instance writes `focused: true` only into ITS plugin's own STATUS.md
- `PLUGINS.md` is read for awareness; it is union-merged, so only ever edit your own row
- No file locking needed for different-plugin focus — the writes touch different files
- Same-plugin focus from multiple instances: last writer wins (by design)

Note: This is designed for the common case of working on different plugins. If you need to work on the same plugin from multiple instances, coordinate manually.

## Module Update Check

When focusing a plugin, check for available module updates to notify the user proactively.

### Workflow

1. **Find the plugin's modules** in `modules/registry.yaml` — scan every module entry's
   `used_by:` list for `- plugin: {plugin}` (the registry stores `used_by` per module,
   not a `modules` array per plugin)
2. **For each match:**
   - Get the installed version from that `used_by` entry's `version`
   - Get the current version from the module entry's `version`
   - Compare using semver: `python3 modules/scripts/semver.py compare <installed> <current>`
   - If current > installed, add to updates list
3. **Display notification** (only if updates exist)

### Output Format

**If updates available:**
```
/plugin:focus O-IntonationPad

Focusing: O-IntonationPad
Stage: 4-wavetable
Status: active

Module updates available:
-------------------------------------------------------------------
  scala-tuning-engine: 1.0.0 -> 1.1.0
  playable-keyboard: 1.0.0 -> 1.0.1

Run /module:upgrade [module] to update.
Or /module:upgrade-all for batch update with preview.
-------------------------------------------------------------------
```

**If no updates available:** Show nothing (don't clutter output).

### Update Check Implementation

```python
# Pseudocode for update check
for module in registry["modules"]:
    entry = next((u for u in module["used_by"] if u["plugin"] == plugin), None)
    if entry is None:
        continue
    installed_version = entry["version"]
    current_version = module["version"]

    result = run(f"python3 modules/scripts/semver.py compare {installed_version} {current_version}")
    # Returns: 0 (v1 == v2), 1 (v1 > v2), or 255 (v1 < v2)

    if result.returncode == 255:  # installed < current
        updates.append({
            "name": module["name"],
            "from": installed_version,
            "to": current_version
        })
```

### Rationale

User decision from CONTEXT.md: "Notify on plugin focus when module updates are available"

This proactive notification helps users stay aware of available improvements without being intrusive (only shows when updates exist).

## Focus State Updates

On focus:
1. Set `focused: true` in `plugins/{plugin}/.planning/STATUS.md` frontmatter
2. Clear `focused: true` from the previously focused plugin's STATUS.md
3. Set `lastActivity` timestamp in STATUS.md
4. Validate before completing switch
5. **Check for module updates** (new step)

## Related Commands

- `/continue` - Resume last session (same plugin)
- `/reconcile` - Validate current plugin state
- `/peek {plugin}` - View another plugin without switching
- `/plugin-list` - See all plugins
- `/plugin-status` - Detailed status
