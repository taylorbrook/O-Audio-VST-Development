---
name: plugin-pause
description: Checkpoint current work, create handoff document
skill: plugin-context
args: "[plugin_name?]"
---

# /plugin-pause

Checkpoint current work and create a handoff document for later resumption. Captures context about what you were working on and any important decisions.

## Usage

```
/plugin-pause [plugin_name]    # Pause specific plugin
/plugin-pause                  # Pause focused plugin
```

## Arguments

- `plugin_name` - Plugin to pause (optional, defaults to focused)

## Behavior

1. Prompts for handoff context:
   - What were you working on?
   - Key decisions or context to preserve?
   - Blockers or concerns?

2. Updates STATUS.md with handoff information

3. Sets plugin status to "paused" in registry

4. Creates git commit checkpoint

## Example

```
/plugin-pause O-IntonationPad

Pausing O-IntonationPad at Stage 2-dsp, Phase plan

What were you working on?
> Designing the JI ratio calculation algorithm

Any key decisions to preserve?
> Decided on 5-limit JI for triads, 7-limit for extensions

Blockers or concerns?
> Need to research voice leading optimization

✓ Handoff saved to STATUS.md
✓ Committed: "checkpoint: O-IntonationPad paused at 2-dsp/plan"

Resume with: /plugin-resume O-IntonationPad
```

## Related Commands

- `/plugin-resume` - Resume paused work
- `/plugin-status` - Check current state
- `/continue` - Alternative resume command
