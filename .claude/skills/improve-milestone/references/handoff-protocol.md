# Handoff Protocol

This document defines the two-step handoff format for phase transitions in improve-milestone.

---

## Purpose

The handoff protocol ensures clean context transitions between phases by:

1. Persisting state to files (not conversation history)
2. Instructing user to clear context
3. Providing clear resume instructions
4. Showing progress visualization

---

## Two-Step Format

After each phase completes, present this format:

```
✓ [Phase Name] Complete

[Brief summary of what was accomplished]

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │   next     │            │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

---

## Phase-Specific Handoffs

### After Discuss Phase

```
✓ Discuss Phase Complete

Requirements captured in CONTEXT.md:
- [Key requirement 1]
- [Key requirement 2]
- [Key requirement 3]

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │   next     │            │            │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

### After Research Phase

```
✓ Research Phase Complete

Findings documented in RESEARCH.md:
- Approach: [recommended approach name]
- Domain: [DSP/GUI/Polish/Mixed]
- Complexity: [Low/Medium/High]
- Files affected: [count]

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │   next     │            │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

### After Plan Phase

```
✓ Plan Phase Complete

Task breakdown in PLAN.md:
- Tasks: [count] ([parallelizable count] can run in parallel)
- Domain: [DSP/GUI/Polish/Mixed]
- Agent: [dsp-agent/gui-agent/general-purpose]
- Version: [base] → [target] ([bump type])

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │     ✓      │   next     │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

### After Execute Phase

```
✓ Execute Phase Complete

Implementation documented in SUMMARY.md:
- Tasks completed: [N]/[N]
- Files changed: [count]
- Build status: [Success/With warnings]

Step 1: /clear
Step 2: /improve-milestone [PluginName]

Progress:
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │     ✓      │     ✓      │   next     │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

### After Verify Phase (Success)

```
✓ Verify Phase Complete - All Goals Achieved

Verification results in VERIFICATION.md:
- Requirements: [N]/[N] passed
- Pluginval: Level [N] passed
- Regressions: None detected

Completing milestone:
- Updating CHANGELOG.md → v[target]
- Creating git commit
- Creating git tag v[target]

Milestone "[slug]" complete!

What would you like to do next?

1. Test in DAW
2. Make another improvement
3. Create new plugin
4. Other

Choose (1-4): _
```

### After Verify Phase (Issues)

```
⚠ Verify Phase Complete - Issues Found

Verification results in VERIFICATION.md:
- Requirements: [N]/[M] passed
- Issues found: [count]

1. Return to execute phase (fix issues)
2. Accept as-is (document known issues)
3. Rollback to backup (v[base])
4. Other

Choose (1-4): _
```

---

## Progress Visualization

### Symbol Key

| Symbol | Meaning |
|--------|---------|
| ✓ | Phase complete |
| ▶ | Phase in progress |
| next | Next phase to run |
| skip | Phase will be skipped |
| (empty) | Phase pending |

### Examples

**Just started:**
```
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│   next     │            │            │            │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

**Research skipped:**
```
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │   skip     │   next     │            │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

**Execute in progress:**
```
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│  DISCUSS   │  RESEARCH  │    PLAN    │  EXECUTE   │   VERIFY   │
│     ✓      │     ✓      │     ✓      │     ▶      │            │
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

---

## Why Two Steps?

### Step 1: /clear

Clears conversation context to:
- Prevent context overflow on large improvements
- Start each phase with focused context
- Avoid confusion from stale conversation history

### Step 2: /improve-milestone [PluginName]

Resumes milestone by:
- Reading STATUS.yaml for current phase
- Loading relevant phase outputs
- Continuing from last checkpoint

---

## Alternative: Single-Context Mode

For small milestones that fit in one context, user can skip /clear:

```
✓ [Phase] Complete

To continue without clearing context, just say "continue" or "next phase".

For fresh context (recommended for large improvements):
  Step 1: /clear
  Step 2: /improve-milestone [PluginName]
```

The orchestrator detects if user says "continue" and proceeds without /clear instruction.

---

## Error Recovery Handoff

If a phase encounters an error:

```
⚠ [Phase] Encountered Error

Error: [error message]
State saved to: STATUS.yaml

Options:
1. Retry current phase
2. Skip to next phase (if skippable)
3. Rollback and abandon
4. Other

Choose (1-4): _
```

After recovery:
```
Step 1: /clear (recommended for fresh start)
Step 2: /improve-milestone [PluginName]
```

---

## Pause/Resume

User can pause mid-milestone:

```
Milestone paused at: [phase] phase

To resume later:
  /improve-milestone [PluginName]

State preserved in:
  plugins/[Name]/.planning/improvements/[slug]/STATUS.yaml
```
