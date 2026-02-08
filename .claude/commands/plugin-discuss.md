---
name: plugin-discuss
description: Interactive context gathering for a stage (GSD discuss phase)
skill: plugin-phases
args: "[plugin_name?] [stage?]"
---

# /plugin-discuss

**Purpose:** Execute the DISCUSS phase of the GSD cycle for a specific plugin stage. Gathers context through interactive questioning and produces CONTEXT.md.

## Usage

```
/plugin-discuss [plugin_name] [stage]     # Specific plugin and stage
/plugin-discuss [stage]                   # Focused plugin, specific stage
/plugin-discuss                           # Focused plugin, current stage
```

## Arguments

- `plugin_name` - Plugin to discuss (optional, defaults to focused)
- `stage` - Stage: `0-ideation`, `1-foundation`, `2-dsp`, `3-gui`, `4-polish`

## Examples

```
/plugin-discuss O-IntonationPad 2-dsp    # Discuss DSP stage
/plugin-discuss 2-dsp                    # Use focused plugin
/plugin-discuss                          # Use focused plugin and current stage
```

## What This Command Does

1. **Loads existing context:**
   - BRIEF.md (creative vision)
   - ROADMAP.md (implementation plan)
   - Previous stage VERIFICATION.md (if not Stage 0)
   - Any existing CONTEXT.md for this stage

2. **Asks adaptive questions based on stage:**
   - Stage 0: Plugin vision, target users, sonic goals, constraints
   - Stage 1: Parameter confirmation, sample rate handling, buffer requirements
   - Stage 2: Algorithm approach, edge cases, performance requirements
   - Stage 3: Layout preferences, control mapping, visual style
   - Stage 4: Testing requirements, release targets, packaging needs

3. **Produces:**
   - `plugins/[Name]/.planning/stages/[N]-[name]/CONTEXT.md`

4. **Updates:**
   - `plugins/[Name]/.planning/STATUS.md` with discuss phase complete

## Output: CONTEXT.md

```markdown
# Stage [N]: [StageName] - Context

## Discussion Summary

**Date:** [YYYY-MM-DD]
**Participants:** User, Claude

## Requirements Confirmed

- [Requirement 1 with clarifications]
- [Requirement 2 with clarifications]

## Constraints Identified

- [Constraint 1]
- [Constraint 2]

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| [Topic] | [Choice] | [Why] |

## Open Questions

- [Any unresolved questions for research phase]

## Next Phase

Ready for: [research/plan] phase
```

## Stage-Specific Questions

### Stage 0 (Ideation)
- What's the core purpose of this plugin?
- Who is the target user?
- What sonic character should it have?
- What are the must-have vs nice-to-have features?

### Stage 1 (Foundation)
- Are the parameters in ROADMAP.md complete and correct?
- Any special sample rate considerations?
- Buffer size requirements?
- Latency tolerance?

### Stage 2 (DSP)
- Which algorithm approach from ARCHITECTURE.md?
- Performance targets (CPU usage)?
- Edge cases to handle?
- Quality vs performance tradeoffs?

### Stage 3 (UI)
- Layout preferences (compact, expanded, modular)?
- Which aesthetic template?
- Special visualization needs?
- Accessibility requirements?

### Stage 4 (Polish)
- Testing scope (manual, automated, DAW list)?
- Packaging format (installer, zip)?
- Release targets (internal, beta, public)?

## MANDATORY Completion Handoff

**After completing the discuss phase, you MUST present the two-step handoff and STOP. Do NOT skip this.**

```
━━━ PHASE COMPLETE ━━━

**[PluginName]** — Stage [N] ([StageName]) / discuss phase

CONTEXT.md created with:
- [Key point 1]
- [Key point 2]
- [Key point 3]

━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ▶ Next Up

**research phase** — Investigate implementation approach

**Step 1:** `/clear` — fresh context window
**Step 2:** `/plugin-research [PluginName] [N]-[stage]`

━━━━━━━━━━━━━━━━━━━━━━━━━━━

**Also available:**

- `/plugin-plan [PluginName] [N]-[stage]` → Skip research, go to planning
- `/plugin-status [PluginName]` → Check status

━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**Rules:**
1. Always include full plugin name in commands (copy-paste ready)
2. Always include `/clear` as Step 1 before the next command
3. Present the handoff and STOP — do NOT auto-invoke the next phase
4. Do NOT relegate `/clear` to a footnote or omit it

See: `.claude/references/handoff-protocol.md`

## Integration

**Follows:** Previous stage VERIFICATION.md (except Stage 0)
**Precedes:** /plugin-research or /plugin-plan
**Updates:** STATUS.md with current GSD phase
