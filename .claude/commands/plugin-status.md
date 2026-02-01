---
name: plugin-status
description: Detailed phase breakdown for a plugin
skill: plugin-context
args: "[plugin_name?]"
---

# /plugin-status

Show detailed phase progress for a plugin, including all stages and their phase completion status.

## Usage

```
/plugin-status [plugin_name]   # Status for specific plugin
/plugin-status                 # Status for focused plugin
```

## Arguments

- `plugin_name` - Plugin to show status for (optional, defaults to focused)

## Output

Shows:
- Current stage and phase
- Express mode status
- Phase progress for all stages (checkmarks, arrows, dots)
- Module dependencies
- Resume command

## Example

```
O-IntonationPad Status
══════════════════════════════════════════════════════════════

Stage: 2 of 4 (DSP)
Phase: plan
Mode: express

Phase Progress
──────────────────────────────────────────────────────────────

  Stage 0: Ideation                               ✓ COMPLETE
  ├── discuss ✓  research ✓  plan ✓  execute ✓  verify ✓

  Stage 1: Foundation                             ✓ COMPLETE
  ├── discuss ✓  research ✓  plan ✓  execute ✓  verify ✓

  Stage 2: DSP                                    → IN PROGRESS
  ├── discuss ✓  research ✓  plan →  execute ·  verify ·

  Stage 3: GUI                                    · PENDING
  ├── discuss ·  research ·  plan ·  execute ·  verify ·

  Stage 4: Polish                                 · PENDING
  ├── discuss ·  research ·  plan ·  execute ·  verify ·

Module Dependencies: scala-tuning-engine@1.0.0

Resume: /continue O-IntonationPad
Next: /plugin-plan O-IntonationPad 2-dsp
```

## Related Commands

- `/plugin-list` - Overview of all plugins
- `/plugin-focus` - Set active plugin
- `/continue` - Resume work
