# Quick Task 003: Handoff Format Consistency

## Goal

Align all plugin workflow stage handoffs with the GSD-style boxed format, including explicit `/clear` + next command two-step sequence.

## Problem

The `context-resume` skill and other presentation points show "messy" handoffs like:
```
Next: Run /implement O-FreqPulse to begin Stage 1 implementation, or /plugin:plan to create the detailed execution plan first.
```

Instead of the clean GSD-style format:
```
━━━ SESSION RESUMED ━━━

## ▶ Next Up

**Stage 1: Foundation** — Build system setup, parameters, plugin shell

**Step 1:** `/clear` — fresh context window
**Step 2:** `/implement O-FreqPulse`

━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Tasks

### Task 1: Update context-parsing.md

Update `.claude/skills/context-resume/references/context-parsing.md` Step 3 "Build Summary" section to use the standard handoff format from `.claude/references/handoff-protocol.md`.

Files: `.claude/skills/context-resume/references/context-parsing.md`

### Task 2: Audit plugin-workflow handoff points

Check `.claude/skills/plugin-workflow/SKILL.md` presentation sections match the protocol.

Files: `.claude/skills/plugin-workflow/SKILL.md`

### Task 3: Check plugin-phases output

Verify `.claude/skills/plugin-phases/SKILL.md` uses consistent format.

Files: `.claude/skills/plugin-phases/SKILL.md`

## Success Criteria

- [ ] All stage completion messages use `━━━` separators
- [ ] All "Next Up" sections use explicit Step 1/Step 2 format
- [ ] Plugin name always included in commands
- [ ] No prose-style "Next:" instructions remain
