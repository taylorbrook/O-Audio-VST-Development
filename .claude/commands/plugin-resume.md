---
name: plugin:resume
description: Restore context from handoff and continue work
skill: plugin-context
args: "[plugin_name?]"
---

# /plugin:resume

Restore context from a previous handoff and continue work on a paused plugin.

## Usage

```
/plugin:resume [plugin_name]   # Resume specific plugin
/plugin:resume                 # Resume focused plugin
```

## Arguments

- `plugin_name` - Plugin to resume (optional, defaults to focused)

## Behavior

1. Reads handoff context from STATUS.md
2. Loads relevant stage artifacts (CONTEXT.md, PLAN.md, etc.)
3. Updates plugin status to "active"
4. Sets as focused plugin
5. Presents context summary and next actions

## Example

```
/plugin:resume O-IntonationPad

Resuming O-IntonationPad
══════════════════════════════════════════════════════════════

Stage: 2-dsp
Phase: plan

Handoff Context:
─────────────────────────────────────────────────────────────
Working on: Designing the JI ratio calculation algorithm
Key decisions:
  - 5-limit JI for triads, 7-limit for extensions
Concerns:
  - Need to research voice leading optimization

Recent Artifacts:
─────────────────────────────────────────────────────────────
- stages/2-dsp/CONTEXT.md (discuss output)
- stages/2-dsp/RESEARCH.md (research output)

Continue with:
1. /plugin:plan O-IntonationPad 2-dsp (recommended)
2. /plugin:research O-IntonationPad 2-dsp (re-research)
3. /plugin:status O-IntonationPad
```

## Related Commands

- `/plugin:pause` - Pause work
- `/continue` - Alternative resume command
- `/plugin:status` - Check state before resuming
