---
name: plugin-list
description: List all plugins with stage/phase status
skill: plugin-context
---

# /plugin-list

List all registered plugins with their current development stage and phase.

## Usage

```
/plugin-list
```

## Output

Displays a table of all plugins showing:
- Plugin name
- Current stage (0-ideation through 4-polish, or complete)
- Current phase (discuss, research, plan, execute, verify)
- Status (active, paused, blocked, released)
- Focused indicator (→)

## Example

```
Plugin Registry
═══════════════════════════════════════════════════════════════

  PLUGIN              STAGE           PHASE      STATUS
  ─────────────────────────────────────────────────────────────
→ O-IntonationPad     2-dsp           plan       active
  O-Bass              complete        -          released
  O-Tremolo           complete        -          released
  O-Lyrica            4-polish        execute    active

Legend: → = focused
```

## Related Commands

- `/plugin-focus [name]` - Set active plugin
- `/plugin-status [name]` - Detailed status
- `/start [name]` - Create new plugin
