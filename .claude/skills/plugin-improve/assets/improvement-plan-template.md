# Improvement Plan Template

**Purpose**: Template for generating implementation plans for Tier 2/3 improvements.

**Used by**: Phase 0.6 Implementation Planning (see references/implementation-planning.md)

---

## Implementation Plan

**Improvement:** [Brief description from user request]
**Tier:** [2 or 3]
**Investigation Summary:** [Key findings from Phase 0.5]
**Generated:** [YYYY-MM-DD]

### Tasks

1. [First task description - outcome-focused, NOT file-path-centric]
   - **Outcome:** [What is different when this is complete]
   - **Verification:** [How to check this worked]
   - **Dependencies:** None

2. [Second task description] (after 1)
   - **Outcome:** [What is different when this is complete]
   - **Verification:** [How to check this worked]
   - **Dependencies:** Task 1

3. [Third task description] (after 2)
   - **Outcome:** [What is different when this is complete]
   - **Verification:** [How to check this worked]
   - **Dependencies:** Task 2

[Continue for all tasks...]

### Risk Notes

[Any concerns, potential complications, fallback options. Claude's discretion - include what's useful per improvement.]

### Estimated Effort

[Quick/Moderate/Significant based on task count and complexity]

- **Quick:** 1-3 tasks, single component, straightforward
- **Moderate:** 4-6 tasks, multiple components, some complexity
- **Significant:** 7+ tasks, architectural changes, high complexity

---

**Approve this plan?**

1. Yes, proceed with implementation
2. No, revise the plan
3. No, cancel improvement
4. Other

Choose (1-4): _

---

## Template Usage Notes

### Outcome-Focused Task Descriptions

Write tasks describing WHAT changes, not WHERE to edit:

- **Good:** "Add resonance parameter to filter section"
- **Bad:** "Edit PluginProcessor.cpp line 42"

- **Good:** "Parameter responds to MIDI CC"
- **Bad:** "Add CC handling in processBlock"

### Dependency Notation

Use explicit "(after N)" suffix:

```
Task A
Task B (after A)
Task C (after A)      <- B and C can run in parallel after A
Task D (after B, C)   <- D needs both B and C complete
```

### Task Granularity

Adjust task count to match complexity:
- Tier 2 improvements: typically 3-5 tasks
- Tier 3 improvements: typically 8-12 tasks

Don't create tasks like "Run build" that should be implicit.

### Partial Approval

User can approve subset of tasks:
- Mark approved tasks with [x]
- Unchecked tasks remain in plan for later
- Track partial execution state in STATUS.md

---

## Example: Adding Resonance Parameter (Tier 2)

```markdown
## Implementation Plan

**Improvement:** Add resonance parameter to filter section
**Tier:** 2 (Moderate complexity)
**Investigation Summary:** Filter lacks resonance control. DSP module supports resonance but parameter not exposed.
**Generated:** 2026-02-02

### Tasks

1. Add RESO parameter to parameter layout
   - **Outcome:** New RESO parameter (0-100%) defined in processor
   - **Verification:** Parameter appears in host automation list
   - **Dependencies:** None

2. Connect parameter to DSP module (after 1)
   - **Outcome:** Moving RESO slider audibly changes filter resonance
   - **Verification:** Sweep from 0-100% produces expected resonance effect
   - **Dependencies:** Task 1

3. Add UI control for resonance (after 1)
   - **Outcome:** RESO knob appears in filter section of UI
   - **Verification:** Visual control present and moves smoothly
   - **Dependencies:** Task 1

4. Test preset save/load (after 2, 3)
   - **Outcome:** RESO value persists in presets and automation
   - **Verification:** Save preset, reload, RESO unchanged
   - **Dependencies:** Tasks 2, 3

### Risk Notes

- Filter DSP module may need coefficient recalculation optimization
- High resonance values could cause self-oscillation (consider adding limiter)

### Estimated Effort

Moderate (4 tasks, 2-3 components affected)

---

Approve this plan?

1. Yes, proceed with implementation
2. No, revise the plan
3. No, cancel improvement
4. Other

Choose (1-4): _
```
