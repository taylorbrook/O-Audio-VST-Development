# Improvement Workflow

This workflow guides you through brainstorming improvements to existing plugins.

**Progress Phases:**
1. Phase 0: Vagueness detection (brainstorm vs implement)
2. Phase 1: Free-form collection
3. Phase 2: Gap analysis
4. Phase 3: Question batch generation
5. Phase 4: Decision gate (finalize or iterate)
6. Phase 5: Document creation (improvement proposal)
7. Phase 6: Session handoff (.continue-here.md)
8. Phase 7: Decision menu (next action)

---

<critical_sequence>
<sequence_name>improvement_workflow</sequence_name>
<enforcement>must_complete_in_order</enforcement>
<phases>

## Phase 0: Vagueness Detection

<decision_gate>
<gate_name>vagueness_check</gate_name>
<blocking>true</blocking>

Check if request is specific:

**Request IS specific if it has:**
- Feature name (e.g., "resonance parameter")
- Action (e.g., "add", "fix", "change")
- Acceptance criteria (e.g., "range 0-1", "increase to 500px")

**Request IS vague if lacking above:**
- "improve the filters"
- "better presets"
- "UI feels cramped"

**If vague, present choice:**
```
Your request is somewhat vague. How should I proceed?

1. Brainstorm approaches first (recommended) ← Explore options together
2. Implement something reasonable ← I'll investigate and propose a solution

Choose (1-2): _
```

If option 1 chosen, continue with improvement brainstorming.
If option 2 chosen, exit to plugin-improve skill.
</decision_gate>

## Phase 1: Free-Form Collection

<step number="1" required="true">
Ask:
```
What would you like to improve in [PluginName]?

Describe what you want to change, add, or fix. I'll ask follow-ups for anything missing.
```

**Extract from response:**
- What aspect (DSP/Parameters/UI/Features)
- Current pain point or limitation
- Proposed change
- Why this improvement matters
- Backward compatibility concerns
- How to test success
</step>

## Phase 2: Gap Analysis and Question Prioritization

<step number="2" required="true">
**Question Priority Tiers:**

- **Tier 1 (Critical):** What aspect (DSP/Parameters/UI/Features), current state vs proposed change
- **Tier 2 (Implementation):** Implementation details, testing criteria, backward compatibility
- **Tier 3 (Context):** Rationale, success metrics, version impact

**Extract from Phase 1 response, then identify gaps:**

1. Parse user's improvement description
2. Check which tiers are covered
3. Identify missing critical information
4. Never ask about already-provided information
</step>

## Phase 3: Question Batch Generation

<step number="3" required="true">
**Must generate exactly 4 questions using AskUserQuestion based on identified gaps (4 questions balance thoroughness with user fatigue).**

**Rules:**
- If 4+ gaps exist: ask top 4 by tier priority
- If 3 gaps exist: ask 3 questions (not 4) and proceed to decision gate
- If <3 gaps exist: pad with "nice to have" tier 3 questions to reach 4
- Provide meaningful options (not just open text prompts)
- Always include "Other" option for custom input

**See [improvement-mode-examples.md](improvement-mode-examples.md) for detailed question batch examples.**

**After receiving answers:**
1. Accumulate context with previous responses
2. Re-analyze gaps
3. Proceed to decision gate
</step>

## Phase 4: Decision Gate

<decision_gate>
<gate_name>finalize_or_continue</gate_name>
<blocking>true</blocking>
<checkpoint_protocol>true</checkpoint_protocol>

<step number="4" required="true">
**Must use AskUserQuestion with 3 options after each question batch:**

```
Question:
  question: "Ready to finalize the improvement brief?"
  header: "Next step"
  options:
    - label: "Yes, finalize it", description: "Create improvement proposal"
    - label: "Ask me 4 more questions", description: "Continue refining"
    - label: "Let me add more context first", description: "Provide additional details"
```

**Must wait for user response. **Never** auto-proceed.**

**Route based on answer:**
- Option 1 → Proceed to Phase 5 (document creation)
- Option 2 → Return to Phase 2 (re-analyze gaps, generate next 4 questions)
- Option 3 → Collect free-form text, merge with context, return to Phase 2
</step>
</decision_gate>

## Phase 5: Document Creation

<step number="5" required="true">

Create: `plugins/[PluginName]/.ideas/improvements/[feature-name].md`

**Format:**
```markdown
# [PluginName] - [Improvement Name]

**Created:** [Date]
**Type:** [Feature/Enhancement/Fix]
**Aspect:** [DSP/Parameters/UI/Features]
**Version Impact:** [PATCH/MINOR/MAJOR]

## Current State

[Description of current behavior or limitation]

## Proposed Change

[Detailed description of what should change]

## Rationale

[Why this improvement matters]

## Implementation Notes

[Technical considerations, files to modify, algorithms to use]

## Backward Compatibility

[Breaking/Non-breaking, migration strategy if needed]

## Testing Criteria

- [ ] [Test 1]
- [ ] [Test 2]
- [ ] [Test 3]

## Success Metrics

[How to know the improvement is complete and working]
```
</step>

## Phase 6: Session Handoff

<state_requirement>
<requirement>must_create_continue_file</requirement>
<step number="6" required="true">
**Must create handoff file for resuming later:**

**File:** `plugins/[PluginName]/.continue-here.md`

```markdown
---
plugin: [PluginName]
stage: improvement_planning
status: improvement_brief_complete
improvement: [feature-name]
last_updated: [YYYY-MM-DD HH:MM:SS]
---

# Resume Point

## Current State: Improvement Brief Complete

Improvement proposal finalized for [PluginName]: [ImprovementName]

## Completed So Far

**Planning:** ✓ Complete
- Current state analyzed
- Proposed change defined
- Testing criteria established

## Next Steps

1. Start implementation (/improve [PluginName])
2. Research implementation approaches
3. Review existing code

## Context to Preserve

**Improvement:** [feature-name]
**Type:** [Feature/Enhancement/Fix]
**Version Impact:** [PATCH/MINOR/MAJOR]

**Files Created:**
- plugins/[PluginName]/.ideas/improvements/[feature-name].md
```
</step>
</state_requirement>

## Phase 7: Decision Menu

<decision_gate>
<gate_name>next_action</gate_name>
<blocking>true</blocking>
<checkpoint_protocol>true</checkpoint_protocol>

<step number="7" required="true">
**Must present next steps:**

```
✓ Improvement brief complete: [ImprovementName]

What's next?
1. Start implementation (recommended)
2. Research implementation approaches ← Find examples and best practices
3. Review existing code first
4. Save for later
5. Other

Choose (1-5): _
```

<delegation_rules>
**Handle responses:**

<delegation_rule skill="plugin-improve" trigger="option_1">
- Condition: User chooses option 1
- Action: Must invoke plugin-improve skill via Skill tool
</delegation_rule>

<delegation_rule skill="deep-research" trigger="option_2">
- Condition: User chooses option 2
- Action: Must invoke deep-research skill via Skill tool
</delegation_rule>

- Option 3 → Read relevant source files, then re-present menu
- Option 4 → Confirm handoff file created, exit
- Option 5 → Ask what they'd like to do
</delegation_rules>
</step>
</decision_gate>

</phases>
</critical_sequence>

---

## Vagueness Detection Rules

<vagueness_check>
**Specific request has:**
- Named feature + Concrete action + Measurable criteria

**Vague request lacks:**
- Named feature OR concrete action OR measurable criteria

**Decision rule:**
If 2+ vague indicators AND 0 specific indicators:
→ MUST present choice: "Brainstorm approaches first" vs "Implement something reasonable"
</vagueness_check>

## Error Handling

**If improvement file already exists:**
```
Improvement proposal "[feature-name].md" already exists.

Options:
1. Create new version (/improve is better for implementing existing proposals)
2. Choose different name
3. Overwrite existing (not recommended)

Choose (1-3): _
```
