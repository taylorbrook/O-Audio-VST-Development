# Phase 9: Workflow Planning Phase - Research

**Researched:** 2026-02-02
**Domain:** AI agent workflow design / task planning for plugin improvements
**Confidence:** HIGH

## Summary

This phase adds a **Phase 0.6 (Implementation Planning)** step to the existing `plugin-improve` workflow. The goal is to reduce rework on complex (Tier 2/3) improvements by requiring an explicit planning step before implementation begins.

The research focused on: (1) AI agent task planning patterns from industry, (2) approval workflow design patterns, (3) existing codebase patterns for tier detection and STATUS.md structure, and (4) dependency notation for task breakdown.

The standard approach is a **conditional planning gate** that triggers based on complexity signals (keyword patterns + scope analysis), produces an outcome-focused task breakdown with explicit dependencies, stores the plan in STATUS.md, and gates implementation behind a simple approval prompt.

**Primary recommendation:** Insert Phase 0.6 between Phase 0.5 (Investigation) and Phase 0.9 (Backup Verification), triggered only for Tier 2/3 improvements. Use soft suggestion with user override capability.

## Standard Stack

This phase involves **workflow/protocol design** rather than external libraries. The "stack" is the existing skill system infrastructure.

### Core
| Component | Location | Purpose | Why Standard |
|-----------|----------|---------|--------------|
| SKILL.md | `.claude/skills/plugin-improve/SKILL.md` | Phase definitions | Existing workflow system |
| STATUS.md | `plugins/[Name]/.planning/STATUS.md` | State tracking | Existing per-plugin state store |
| References folder | `.claude/skills/plugin-improve/references/` | Protocol docs | Existing reference pattern |
| Assets folder | `.claude/skills/plugin-improve/assets/` | Templates | Existing template pattern |

### Supporting
| Pattern | Source | Purpose | When to Use |
|---------|--------|---------|-------------|
| Tier detection | `references/investigation-tiers.md` | Complexity classification | Extend for planning trigger |
| Research detection | `references/research-detection.md` | Handoff protocol | Adapt for planning skip |
| Checkpoint protocol | SKILL.md state_requirement | Approval gates | Model for planning approval |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| STATUS.md storage | Separate PLAN-IMPROVEMENT.md | Adds file sprawl; STATUS.md already tracks state |
| Keyword + scope detection | Explicit user tier flag | Adds friction; auto-detect preferred |
| Simple Yes/No approval | Multi-stage review | Overkill for improvement workflow |

## Architecture Patterns

### Recommended Workflow Position

```
Phase 0: Specificity Detection
  |
Phase 0.3: Clarification Questions
  |
Phase 0.4: Decision Gate
  |
Phase 0.45: Research Detection (MANDATORY)
  |
[Research found?] ─YES→ Offer choice: "Plan anyway or proceed?"
  | NO
  v
Phase 0.5: Investigation (Tier 1/2/3 auto-detected)
  |
[Tier 1?] ─YES→ Skip to Phase 0.9 (no planning needed)
  | NO (Tier 2 or 3)
  v
**Phase 0.6: Implementation Planning** ← NEW
  |
[Approved?] ─YES→ Phase 0.9 (Backup Verification)
            ─NO→ Revision menu
```

### Pattern 1: Conditional Planning Gate

**What:** Planning activates based on tier detection, not user request
**When to use:** Automatically when Tier 2/3 detected
**Logic:**
```
IF tier == 1:
    SKIP Phase 0.6 (log: "Tier 1 - fast path")
ELSE IF tier IN [2, 3]:
    SUGGEST: "This looks complex. Planning recommended."
    IF user accepts OR --no-plan NOT provided:
        EXECUTE Phase 0.6
    ELSE:
        SKIP Phase 0.6 (log: "User bypassed planning")
```

### Pattern 2: Outcome-Focused Task Breakdown

**What:** Tasks describe outcomes, not file paths
**When to use:** All planning output
**Example:**
```markdown
## Implementation Plan

1. Add resonance parameter to filter section
   - Outcome: New RESO parameter (0-100%) appears in UI and affects filter
   - Dependencies: None (first task)

2. Connect parameter to DSP (after 1)
   - Outcome: Moving RESO slider audibly changes filter resonance
   - Dependencies: Task 1 complete

3. Test in DAW (after 2)
   - Outcome: Parameter automatable, presets save/load correctly
   - Dependencies: Task 2 complete
```

### Pattern 3: Explicit Dependency Notation

**What:** Dependencies use "(after N)" suffix
**When to use:** Any task that cannot start until prior completes
**Example:**
```
Task A
Task B (after A)
Task C (after A)      ← B and C can run in parallel after A
Task D (after B, C)   ← D needs both B and C complete
```

### Pattern 4: Plan Storage in STATUS.md

**What:** Improvement plan stored as section in existing STATUS.md
**When to use:** When plan generated for Tier 2/3 improvement
**Structure:**
```markdown
## Active Improvement Plan

**Improvement:** [Brief description from user request]
**Tier:** 2 (Moderate complexity)
**Generated:** 2026-02-02

### Tasks

1. [Task description]
   - Outcome: [What changes when complete]
   - Dependencies: None

2. [Task description] (after 1)
   - Outcome: [What changes when complete]
   - Dependencies: Task 1

### Status

- [x] Task 1: Complete
- [ ] Task 2: In progress
- [ ] Task 3: Pending

### Approval

Approved: Yes (2026-02-02)
```

### Anti-Patterns to Avoid

- **File-path-centric tasks:** "Edit PluginProcessor.cpp line 42" is fragile. Use outcome focus instead: "Parameter responds to MIDI CC"
- **Implicit dependencies:** Don't assume readers know task order. Be explicit: "(after 2)"
- **Forced planning for Tier 1:** Simple typo fix doesn't need task breakdown. Keep fast path fast.
- **Approval bypass:** Implementation must not proceed without explicit approval. This is a gate, not a suggestion.
- **Ignoring deep-research handoff:** If research already done, don't force full planning - offer choice.

## Don't Hand-Roll

This phase is about workflow design, not code. The main "don't hand-roll" concern is:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Tier detection | New classification system | Extend existing `investigation-tiers.md` | Already has Tier 1/2/3 with clear criteria |
| Plan storage | New file format | STATUS.md section | Consistent with existing state management |
| Approval flow | Complex multi-reviewer | Simple Yes/No with revision options | Matches existing checkpoint protocol |
| Dependency tracking | Graph traversal | Explicit "(after N)" notation | Human-readable, no tooling needed |

**Key insight:** This phase extends existing patterns rather than introducing new infrastructure. The skill system already has tier detection, STATUS.md storage, and checkpoint protocols - use them.

## Common Pitfalls

### Pitfall 1: Over-Engineering Tier Detection

**What goes wrong:** Complex NLP to classify improvements adds latency and false positives
**Why it happens:** Trying to be "smart" about classification
**How to avoid:** Use simple keyword patterns + scope heuristics:
- Keywords: "refactor", "overhaul", "redesign", "multiple", "architecture", "rewrite"
- Scope: Multiple files mentioned, multiple parameters affected, DSP changes
**Warning signs:** Classification taking >1 second, frequent user corrections

### Pitfall 2: Planning Overhead for Simple Fixes

**What goes wrong:** User wants to fix typo, gets forced through 5-minute planning
**Why it happens:** Not preserving Tier 1 fast path
**How to avoid:** Tier 1 improvements skip planning entirely. Detection MUST happen before planning.
**Warning signs:** Users saying "just do it", using --no-plan frequently on small changes

### Pitfall 3: Approval Gate Bypass

**What goes wrong:** Implementation starts before user approves plan
**Why it happens:** Auto-proceeding after presenting plan
**How to avoid:** Use WAIT pattern from existing checkpoint protocol. Present menu, STOP, wait for response.
**Warning signs:** Users surprised by changes they didn't approve

### Pitfall 4: Lost Deep-Research Context

**What goes wrong:** User ran /research, then /improve - research findings ignored
**Why it happens:** Planning runs fresh investigation instead of using existing findings
**How to avoid:** Phase 0.45 (Research Detection) MUST still run. When research detected, offer choice: "Plan based on research findings or proceed directly?"
**Warning signs:** Duplicate investigation work, user frustration

### Pitfall 5: Rigid Task Granularity

**What goes wrong:** Every improvement gets 10-task breakdown regardless of actual complexity
**Why it happens:** Template-driven planning without judgment
**How to avoid:** Task count should match complexity. Tier 2 might be 3-5 tasks. Tier 3 might be 8-12. Claude's discretion.
**Warning signs:** Tasks like "Task 5: Run build" that should be implicit

### Pitfall 6: Dependencies That Create Serial Bottleneck

**What goes wrong:** All tasks sequential when some could parallelize
**Why it happens:** Defaulting to "after previous" for every task
**How to avoid:** Analyze actual dependencies. UI work and DSP work might parallelize. Only add dependency when required.
**Warning signs:** Execution taking 3x longer than necessary

## Code Examples

These are **protocol patterns**, not code. Adapted from existing codebase patterns.

### Tier Detection Enhancement

Add to `references/investigation-tiers.md`:

```markdown
## Planning Trigger Detection (Phase 0.5 Extension)

After auto-detecting tier, determine planning trigger:

### Tier 1: No Planning
- Cosmetic changes, typo fixes, single-parameter adjustments
- Single-file scope
- Known patterns from troubleshooting/
- **Action:** Skip Phase 0.6 entirely

### Tier 2: Planning Suggested
- Logic errors, multi-parameter changes, integration fixes
- 2-5 files affected
- DSP changes (but not architectural)
- **Keywords detected:** "change", "fix", "modify", "update", "adjust"
- **Action:** Soft suggestion: "This looks moderately complex. Planning recommended."

### Tier 3: Planning Required
- Architectural changes, major refactors, multi-system improvements
- 5+ files affected
- New features touching multiple components
- **Keywords detected:** "refactor", "overhaul", "redesign", "rewrite", "architecture"
- **Action:** Strong suggestion: "This is a significant change. Planning recommended before starting."
```

### Phase 0.6 Protocol

New file: `references/implementation-planning.md`

```markdown
# Implementation Planning Protocol (Phase 0.6)

**Purpose:** Create task breakdown before implementation to reduce rework.
**Trigger:** Tier 2/3 improvements (auto-detected in Phase 0.5)
**Skip condition:** Tier 1 OR user provides --no-plan flag OR user declines suggestion

## Entry Point

After Phase 0.5 Investigation completes with Tier 2 or 3:

1. Present soft suggestion (Tier 2) or strong suggestion (Tier 3)
2. Wait for user response
3. If accepted, generate plan
4. Present plan for approval
5. Wait for approval before proceeding

## Plan Generation

Based on investigation findings, create task breakdown:

1. Identify distinct implementation steps
2. Determine dependencies between steps
3. Write outcome-focused task descriptions
4. Add verification criteria per task

## Plan Format

See assets/improvement-plan-template.md for structure.

## Approval Flow

Present plan with options:
1. Approve - proceed to Phase 0.9
2. Revise - modify plan based on feedback
3. Cancel - abort improvement workflow
4. Other - collect free-form response

## Post-Approval

Store approved plan in STATUS.md under "## Active Improvement Plan"
Update STATUS.md frontmatter: `active_improvement: true`
```

### Planning Template

New file: `assets/improvement-plan-template.md`

```markdown
## Implementation Plan

**Improvement:** [Brief description from user request]
**Tier:** [2 or 3]
**Investigation Summary:** [Key findings from Phase 0.5]
**Generated:** [Date]

### Tasks

1. [First task description]
   - **Outcome:** [What is different when this is complete]
   - **Verification:** [How to check this worked]
   - **Dependencies:** None

2. [Second task description] (after 1)
   - **Outcome:** [What is different when this is complete]
   - **Verification:** [How to check this worked]
   - **Dependencies:** Task 1

[Continue for all tasks...]

### Risk Notes

[Any concerns, potential complications, fallback options]

### Estimated Effort

[Quick/Moderate/Significant based on task count and complexity]

---

Approve this plan?

1. Yes, proceed with implementation
2. No, revise the plan
3. No, cancel improvement
4. Other

Choose (1-4): _
```

### Deep-Research Handoff Adaptation

Update `references/research-detection.md` decision logic:

```markdown
### If Research Detected (Modified for Phase 0.6)

When research findings detected in conversation history:

1. Display findings summary (existing)

2. Present enhanced choice menu:
   ```
   Research findings available. How should I proceed?

   1. Plan based on research - Create task breakdown from findings
   2. Proceed directly - Skip planning, use findings in implementation
   3. Investigate further - Run fresh investigation (Phase 0.5)
   4. Other

   Choose (1-4): _
   ```

3. Handle responses:
   - Option 1 → Generate plan using research findings, proceed to Phase 0.6 approval
   - Option 2 → Skip Phase 0.6, proceed to Phase 0.9 (existing behavior)
   - Option 3 → Run Phase 0.5 despite research existing
   - Option 4 → Collect free-form text
```

### Bypass After Failure Reminder

Add to Phase 0.5 or Phase 0.6:

```markdown
## Historical Context Check

Before tier detection, check for recent bypass failures:

1. Read .planning/STATUS.md
2. Look for `last_bypass_failure: true` in frontmatter
3. If found:
   - Present reminder: "The last improvement had issues after bypassing planning. Would you like to plan this one?"
   - Clear flag after presenting: `last_bypass_failure: false`

When bypass leads to failure (detected in Phase 5 or later):
- Set flag: `last_bypass_failure: true` in STATUS.md
- Log for future reminder
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| No planning for improvements | Conditional planning for Tier 2/3 | Phase 9 (this phase) | Reduces rework on complex changes |
| Implicit task ordering | Explicit "(after N)" notation | Phase 9 (this phase) | Clear dependencies |
| Separate plan files | Plan in STATUS.md | Phase 9 (this phase) | Single source of state |

**Current industry patterns (2026):**
- Plan-and-Execute: Create strategy first, execute second (standard in agentic AI)
- Conditional gates: Planning based on complexity, not universal
- Human-in-the-loop: Approval checkpoints for significant actions
- Outcome focus: Describe what changes, not how to change it

## Open Questions

### 1. Tier 2 vs Tier 3 Boundary

**What we know:**
- Tier 2 is moderate (2-5 files, logic changes)
- Tier 3 is significant (architectural, multi-system)

**What's unclear:**
- Exact threshold for "architectural" vs "significant logic change"
- Whether to surface tier classification to user

**Recommendation:** Leave as Claude's discretion per CONTEXT.md decision. Don't over-specify the boundary - judgment call based on investigation findings.

### 2. Partial Approval Granularity

**What we know:**
- User decided: partial approval supported
- User can pick which tasks to execute now

**What's unclear:**
- How to track partial execution state in STATUS.md
- Whether to re-plan remaining tasks or keep original plan

**Recommendation:** Add task-level checkboxes to plan. User can approve subset. Unchecked tasks remain in plan for later.

### 3. Plan Versioning

**What we know:**
- Plans live in STATUS.md
- Revisions possible after rejection

**What's unclear:**
- Whether to keep revision history or just latest plan
- How to handle mid-execution plan changes

**Recommendation:** Keep only latest approved plan. If mid-execution changes needed, create new plan iteration (rare case - usually just complete current plan first).

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/Dev/VST-development/.claude/skills/plugin-improve/SKILL.md` - Existing workflow structure
- `/Users/taylorbrook/Dev/VST-development/.claude/skills/plugin-improve/references/investigation-tiers.md` - Tier detection patterns
- `/Users/taylorbrook/Dev/VST-development/.claude/skills/plugin-improve/references/research-detection.md` - Handoff protocol
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Freeze/.planning/STATUS.md` - STATUS.md structure example
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/.planning/STATUS.md` - STATUS.md with GSD cycle tracking

### Secondary (MEDIUM confidence)
- [Top AI Agentic Workflow Patterns 2026](https://medium.com/@Deep-concept/top-ai-agentic-workflow-patterns-that-will-lead-in-2026-0e4755fdc6f6) - Plan-and-Execute pattern
- [Google Multi-Agent Design Patterns](https://www.infoq.com/news/2026/01/multi-agent-design-patterns/) - Hierarchical decomposition
- [Agentic AI Design Patterns 2026](https://research.aimultiple.com/agentic-ai-design-patterns/) - Planning pattern fundamentals
- [Multi-Model Routing 2026](https://medium.com/@MateCloud/why-2026-is-the-year-of-multi-model-routing-technical-challenges-and-system-design-2457dcdd2209) - Tiered routing strategies
- [Approval Workflow Design Patterns](https://www.cflowapps.com/approval-workflow-design-patterns/) - Conditional and sequential approval patterns
- [DAG Workflow Structure](https://www.workflows.guru/workflow-types/dags-workflows) - Task dependency notation
- [Guardrails for AI Agents](https://www.reco.ai/hub/guardrails-for-ai-agents) - Human approval triggers

### Tertiary (LOW confidence)
- General AI agent trends articles (validated concepts against codebase patterns)

## Metadata

**Confidence breakdown:**
- Workflow position: HIGH - Clear integration point in existing phase flow
- Tier detection: HIGH - Extends existing investigation-tiers.md pattern
- Plan format: HIGH - Follows existing STATUS.md and template patterns
- Approval flow: HIGH - Matches existing checkpoint protocol pattern
- Dependency notation: MEDIUM - Novel notation, but simple and human-readable

**Research date:** 2026-02-02
**Valid until:** 90+ days (workflow patterns, not library versions)
