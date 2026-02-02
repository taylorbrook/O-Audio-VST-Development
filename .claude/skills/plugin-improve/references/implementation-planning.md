# Implementation Planning Protocol (Phase 0.6)

**Purpose:** Create task breakdown before implementation to reduce rework.
**Trigger:** Tier 2/3 improvements (auto-detected in Phase 0.5)
**Skip condition:** Tier 1 OR user provides --no-plan flag OR user declines suggestion

---

## Contents

1. [Entry Point](#entry-point)
2. [Plan Generation](#plan-generation)
3. [Plan Format](#plan-format)
4. [Approval Flow](#approval-flow)
5. [Post-Approval](#post-approval)
6. [Historical Context Check](#historical-context-check)
7. [Integration Diagram](#integration-diagram)
8. [Anti-Patterns](#anti-patterns)

---

## Entry Point

After Phase 0.5 Investigation completes with Tier 2 or 3:

1. **Present suggestion based on tier:**

   **Tier 2 (soft suggestion):**
   ```
   This looks moderately complex. Planning recommended.

   Would you like me to create an implementation plan before starting?

   1. Yes, create plan
   2. No, proceed directly
   3. Other

   Choose (1-3): _
   ```

   **Tier 3 (strong suggestion):**
   ```
   This is a significant change. Planning recommended before starting.

   Would you like me to create an implementation plan?

   1. Yes, create plan (recommended)
   2. No, proceed directly
   3. Other

   Choose (1-3): _
   ```

2. **Wait for user response**

3. **Handle response:**
   - Option 1 → Generate plan
   - Option 2 → Skip Phase 0.6, proceed to Phase 0.9 (log: "User bypassed planning")
   - Option 3 → Collect free-form text, reassess

4. **If plan generated:** Present plan for approval

5. **Wait for approval** before proceeding to Phase 0.9

---

## Plan Generation

Based on investigation findings from Phase 0.5, create task breakdown:

### Step 1: Identify Distinct Implementation Steps

Break the improvement into atomic tasks:
- Each task should be independently verifiable
- Each task should have a clear "done" state
- Tasks should be small enough to complete without context-switching

### Step 2: Determine Dependencies Between Steps

Analyze which tasks depend on others:
- Independent tasks can run in parallel
- Dependent tasks use "(after N)" notation
- Minimize serial dependencies where possible

### Step 3: Write Outcome-Focused Task Descriptions

Describe WHAT changes, not WHERE to edit:
- **Good:** "Add resonance parameter to filter section"
- **Bad:** "Edit PluginProcessor.cpp line 42"

Focus on user-visible outcomes:
- **Good:** "Parameter responds to MIDI CC"
- **Bad:** "Add CC handling code"

### Step 4: Add Verification Criteria Per Task

Each task needs clear success criteria:
- How to test the task is complete
- What behavior should be observable
- What state should change

---

## Plan Format

Use the template at `assets/improvement-plan-template.md`.

The template provides:
- Header section (improvement, tier, investigation summary, date)
- Tasks section with outcome/verification/dependencies structure
- Risk notes section
- Estimated effort section
- 4-option approval menu

**Task granularity guidance:**
- Tier 2 improvements: typically 3-5 tasks
- Tier 3 improvements: typically 8-12 tasks

---

## Approval Flow

Present completed plan with options:

```
[Generated plan displayed here]

---

Approve this plan?

1. Yes, proceed with implementation
2. No, revise the plan
3. No, cancel improvement
4. Other

Choose (1-4): _
```

### Handle Responses

**Option 1 - Approve:**
- Store plan in STATUS.md (see Post-Approval)
- Proceed to Phase 0.9 (Backup Verification)
- Begin implementation after backup verified

**Option 2 - Revise:**
```
What would you like to change?

1. Add tasks
2. Remove tasks
3. Reorder tasks
4. Change scope
5. Other

Choose (1-5): _
```
- Collect feedback
- Regenerate plan with modifications
- Re-present for approval

**Option 3 - Cancel:**
```
Improvement cancelled. No changes made.

Would you like to:
1. Start a different improvement
2. Research this issue further
3. Return to main menu
4. Other

Choose (1-4): _
```

**Option 4 - Other:**
- Collect free-form text
- Interpret intent
- Either revise plan or proceed as appropriate

### Partial Approval

User can approve subset of tasks:
- Present plan with checkboxes
- User marks which tasks to execute now
- Unchecked tasks remain in plan for later
- Track partial execution state in STATUS.md

---

## Post-Approval

### Store Plan in STATUS.md

After user approves plan, store it in the plugin's STATUS.md:

```markdown
## Active Improvement Plan

**Improvement:** [Brief description from user request]
**Tier:** [2 or 3]
**Generated:** [YYYY-MM-DD]
**Approved:** [YYYY-MM-DD]

### Tasks

1. [Task description]
   - **Outcome:** [What changes when complete]
   - **Verification:** [How to check]
   - **Status:** [ ] Pending

2. [Task description] (after 1)
   - **Outcome:** [What changes when complete]
   - **Verification:** [How to check]
   - **Status:** [ ] Pending

[Continue for all tasks...]

### Progress

- [x] Plan approved
- [ ] Task 1: Pending
- [ ] Task 2: Pending
- [ ] Implementation complete
```

### Update STATUS.md Frontmatter

Add/update these fields:

```yaml
---
active_improvement: true
improvement_tier: 2
plan_approved: 2026-02-02
---
```

### Clear Plan After Completion

When all tasks complete successfully:
1. Move plan to "## Completed Improvements" section (with date)
2. Set `active_improvement: false` in frontmatter
3. Remove `improvement_tier` and `plan_approved` fields

---

## Historical Context Check

Before tier detection in Phase 0.5, check for recent bypass failures:

### Check for Bypass Failure Flag

1. Read plugin's `.planning/STATUS.md`
2. Look for `last_bypass_failure: true` in frontmatter

### If Flag Found

Present reminder:
```
Note: The last improvement had issues after bypassing planning.
Would you like to plan this one?

1. Yes, create plan (recommended)
2. No, proceed without planning
3. Tell me more about what happened

Choose (1-3): _
```

Then clear the flag:
```yaml
---
last_bypass_failure: false  # Cleared after presenting reminder
---
```

### Set Flag on Bypass Failure

When user bypassed planning AND implementation fails (detected in Phase 5 or later):
1. Set `last_bypass_failure: true` in STATUS.md frontmatter
2. Log for future reminder

This provides gentle guidance without being preachy.

---

## Integration Diagram

```
Phase 0.5: Investigation (Tier 1/2/3 auto-detected)
     |
     v
[Tier 1?] ─YES─────────────────────────────────> Skip to Phase 0.9
     |                                             (no planning needed)
     | NO (Tier 2 or 3)
     v
[Historical Context Check]
     |
     v
**Phase 0.6: Implementation Planning** <─── YOU ARE HERE
     |
     v
[Present suggestion]
     |
     v
[User accepts?] ─NO──> [--no-plan flag?] ─YES──> Skip to Phase 0.9
     |                        |                    (log: bypass)
     | YES                    | NO
     v                        v
[Generate plan]         [User declines]
     |                        |
     v                        v
[Present for approval]   Skip to Phase 0.9
     |                    (log: user declined)
     v
[Approved?] ─NO──> [Revise/Cancel menu]
     |                   |
     | YES               v
     v              [Handle choice]
[Store in STATUS.md]
     |
     v
Phase 0.9: Backup Verification
     |
     v
[Implementation phases...]
```

### Key Integration Points

**Input from Phase 0.5:**
- Tier classification (2 or 3)
- Investigation findings
- Root cause analysis
- Recommended approach

**Output to Phase 0.9:**
- Approved plan (stored in STATUS.md)
- Skip signal (if Tier 1 or user bypassed)
- Context for implementation phases

---

## Anti-Patterns

<anti_pattern severity="CRITICAL">
**Never skip approval gate:**
- Plan MUST be approved before implementation starts
- This is a gate, not a suggestion
- Implementation without approval = violation

**Never force planning on Tier 1:**
- Tier 1 = simple fixes that don't need breakdown
- Forcing planning adds friction with no benefit
- Keep the fast path fast

**Never use file-path-centric tasks:**
- "Edit PluginProcessor.cpp line 42" is fragile
- Files and lines change; outcomes don't
- Focus on what changes, not where

**Never ignore deep-research handoff:**
- If research already done (Phase 0.45), use those findings
- Don't re-investigate what's already investigated
- Planning uses research output, doesn't replace it

**Never create implicit dependencies:**
- If Task B needs Task A, say so: "(after A)"
- Don't assume readers know the order
- Explicit dependencies enable parallelization

**Never rigidly template tasks:**
- Task count should match complexity
- 10 tasks for a typo fix = over-engineering
- 2 tasks for architectural change = under-planning
</anti_pattern>

---

## Benefits

- **Reduced rework:** Catch scope issues before implementation starts
- **Clear progress:** Track which tasks complete vs remain
- **User control:** Approval gate ensures user agrees with approach
- **Context preservation:** Plan persists in STATUS.md for session continuity
- **Gentle guidance:** Bypass failure reminder without being preachy
