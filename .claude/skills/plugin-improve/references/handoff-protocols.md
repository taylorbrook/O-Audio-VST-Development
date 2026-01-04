# Handoff Protocol: deep-research → plugin-improve

## Overview

The plugin-improve skill can receive pre-computed research findings from the deep-research skill, preserving expensive research context (Opus + extended thinking) across skill boundaries.

## Handoff Trigger

When deep-research completes investigation and user chooses "Apply solution", deep-research invokes plugin-improve directly via the Skill tool.

## Detection Mechanism

**Phase 0.45: Research Detection** scans conversation history for:

1. **Skill attribution:** Messages from deep-research skill
2. **Research artifacts:** Tier 1/2/3 reports
3. **Problem analysis:** Root cause + recommended solution

## Workflow

```
User: /research [problem]
  ↓
deep-research skill invoked
  ↓
Investigates (Tier 1/2/3)
  ↓
Presents findings with decision menu
  ↓
User: "Apply solution" (Option 1)
  ↓
deep-research invokes plugin-improve via Skill tool
  ↓
plugin-improve Phase 0.45 detects research
  ↓
Extracts findings, skips investigation
  ↓
Proceeds to implementation approval
```

## Information Extracted

When handoff detected, extract:

- **Problem:** [from research report]
- **Root Cause:** [from research analysis]
- **Solution:** [from research recommendations]
- **Steps:** [from research implementation plan]

## Benefits

1. **Context preservation:** Research findings carry forward
2. **Efficiency:** Skip redundant investigation (Phase 0.5)
3. **Quality:** Deep-research uses Opus + extended thinking
4. **Separation of concerns:** Investigation vs. implementation

## Handoff Detection Code

```markdown
<handoff_protocol source="deep-research" target="plugin-improve">
## Phase 0.45: Research Detection

**Scan recent messages for:**

- Messages from deep-research skill
- Research reports (Tier 1/2/3)
- Problem analysis (root cause + solution)

**If deep-research findings found:**

Extract and present:
```
✓ Research already completed (deep-research Level N)

Problem: [extracted]
Root Cause: [extracted]
Solution: [extracted]
Steps: [extracted]

Ready to implement? (y/n): _
```

Wait for user approval → Proceed to Phase 0.9

**If NO research findings found:**

Proceed to Phase 0.5 (Investigation) - perform fresh root cause analysis

</handoff_protocol>
```

## Alternative: No Handoff

If Phase 0.45 finds no research findings:
- Proceed to Phase 0.5 (Investigation)
- Perform fresh root cause analysis
- Use current skill context (Sonnet, no extended thinking)
