# New Plugin Workflow

This workflow guides you through capturing a complete creative vision for a new plugin.

**Progress Phases:**
1. Phase 1: Free-form collection
2. Phase 2: Gap analysis
3. Phase 3: Question batch generation
4. Phase 4: Decision gate (finalize or iterate)
5. Phase 5: Plugin name validation (if needed)
6. Phase 6: Document creation (creative-brief.md)
7. Phase 7: Session handoff (.continue-here.md)
8. Phase 8: Decision menu (next action)

---

<critical_sequence>
<sequence_name>new_plugin_workflow</sequence_name>
<enforcement>must_complete_in_order</enforcement>
<phases>

## Phase 1: Free-Form Collection

<step number="1" required="true">
Must start with open question:
```
What would you like to build?

Tell me about your plugin idea. Share as much or as little as you want—I'll ask follow-ups to fill any gaps in type, concept, parameters, and use cases.
```

**Extract from response:**
- Plugin name (if mentioned)
- Plugin type (effect/synth/utility)
- Core concept and sonic goals
- Parameter ideas and ranges
- UI vision and layout preferences
- Use cases and target users
- Inspirations and references
</step>

## Phase 2: Gap Analysis and Question Prioritization

<step number="2" required="true">
**Question Priority Tiers:**

- **Tier 1 (Critical):** Plugin type (effect/synth/utility), core concept (what it does)
- **Tier 2 (Functional):** Parameters and ranges, processing behavior, signal flow
- **Tier 3 (Context):** Use cases, inspirations, special features (presets, MIDI, modulation)
- **Tier 4 (NEVER ASK):** UI details - if user volunteers UI info, capture it in the brief but NEVER prompt for UI in ideation phase

**Extract from Phase 1 response, then identify gaps:**

1. Parse user's free-form description
2. Check which tiers are covered
3. Identify missing critical/functional information
4. Never ask about already-provided information

**Example of smart extraction:**

```
User: "I want a tape delay with wow and flutter modulation. Should have three knobs and a vintage aesthetic."

Extracted:
- Type: Effect ✓
- Core concept: Tape delay with modulation ✓
- Parameters: wow, flutter (2 mentioned, 3 total) ✓
- UI: vintage, three knobs ✓ (capture but don't expand)

Gaps identified:
- What should the third knob control? (Tier 2)
- What ranges for wow/flutter? (Tier 2)
- Specific tape reference? (Tier 3)
- Primary use case? (Tier 3)
```
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
- Users can skip questions via "Other" option and typing "skip"

**See [adaptive-questioning-examples.md](adaptive-questioning-examples.md) for detailed question batch examples.**

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
  question: "Ready to finalize the creative brief?"
  header: "Next step"
  options:
    - label: "Yes, finalize it", description: "Create creative-brief.md"
    - label: "Ask me 4 more questions", description: "Continue refining"
    - label: "Let me add more context first", description: "Provide additional details"
```

**Must wait for user response. **Never** auto-proceed.**

**Route based on answer:**
- Option 1 → Proceed to Phase 6 (document creation)
- Option 2 → Return to Phase 2 (re-analyze gaps, generate next 4 questions)
- Option 3 → Collect free-form text, merge with context, return to Phase 2
</step>
</decision_gate>

**Context accumulation example:**

After Batch 1 answers: "Feedback", "Moderate 0-15%", "Space Echo", "Both"

Updated context:
- Parameters: wow (0-15%), flutter (0-15%), feedback (need range) ✓
- Inspiration: Space Echo ✓
- Use case: versatile ✓

New gaps for Batch 2:
- Feedback range? (Tier 2)
- Delay time range? (Tier 2)
- Tempo sync? (Tier 3)
- Specific Space Echo model reference? (Tier 3)

## Phase 5: Plugin Name (if not yet provided)

<step number="5" required="true">
**Must check if plugin name was provided before creating documents.**

If name NOT yet provided, Must ask via AskUserQuestion:

```
Question:
  question: "What should this plugin be called?"
  header: "Plugin name"
  options:
    - label: "[SuggestedName1]", description: "Based on core concept"
    - label: "[SuggestedName2]", description: "Alternative naming"
    - label: "[SuggestedName3]", description: "Different approach"
    - label: "Other", description: "I'll provide my own name"

Where suggested names are generated from the core concept.
Examples:
- Tape delay → "TapeAge", "VintageDelay", "FlutterDelay"
- 808 clap → "ClapMachine", "FlamClap", "808Clap"
- Distortion → "SaturnDrive", "WarmClip", "HarmonicDirt"
```

**If name already provided** (in initial description or in additional context), skip this phase entirely.

**Name validation:**
- Must be UpperCamelCase (e.g., "TapeAge", not "tape age" or "tapeage")
- No spaces or special characters
- If user provides invalid name, suggest cleaned version
</step>

## Phase 6: Document Creation

<step number="6" required="true">
**Must wait until user chooses "finalize" and name is confirmed, then create:**

**File:** `plugins/[PluginName]/.ideas/creative-brief.md`

**Format:**
```markdown
# [PluginName] - Creative Brief

## Overview

**Type:** [Effect/Synth/Utility]
**Core Concept:** [One-sentence description]
**Status:** 💡 Ideated
**Created:** [Date]

## Vision

[Prose description of plugin concept, sonic goals, inspiration]

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| [Name] | [Min-Max] | [Value] | [Purpose] |
| ... | ... | ... | ... |

## UI Concept

**Layout:** [Description]
**Visual Style:** [Description]
**Key Elements:** [List special UI components]

## Use Cases

- [Scenario 1]
- [Scenario 2]
- [Scenario 3]

## Inspirations

- [Plugin/hardware reference 1]
- [Plugin/hardware reference 2]
- [Sonic reference]

## Technical Notes

[Any specific DSP approaches, algorithms, or technical considerations mentioned]

## Next Steps

- [ ] Create UI mockup (`/dream [PluginName]` → option 3)
- [ ] Start implementation (`/implement [PluginName]`)
```

**Also update PLUGINS.md:**

Add entry if doesn't exist:
```markdown
### [PluginName]

**Status:** 💡 Ideated
**Type:** [Effect/Synth/Utility]
**Created:** [Date]
**Description:** [One-sentence summary]
```
</step>

## Phase 7: Session Handoff

<state_requirement>
<requirement>must_create_continue_file</requirement>
<step number="7" required="true">
**Must create handoff file for resuming later:**

**File:** `plugins/[PluginName]/.continue-here.md`

**Format:**
```markdown
---
plugin: [PluginName]
stage: ideation
status: creative_brief_complete
last_updated: [YYYY-MM-DD HH:MM:SS]
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief has been finalized for [PluginName]. Ready to proceed to UI mockup or implementation.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined
- Parameters specified
- UI vision captured
- Use cases identified

## Next Steps

1. Create UI mockup to visualize design (recommended)
2. Start implementation directly
3. Research similar plugins for inspiration

## Context to Preserve

**Key Decisions:**
- Plugin type: [Effect/Synth/Utility]
- Core concept: [Brief summary]

**Files Created:**
- plugins/[PluginName]/.ideas/creative-brief.md
```
</step>
</state_requirement>

## Phase 8: Decision Menu

<decision_gate>
<gate_name>next_action</gate_name>
<blocking>true</blocking>
<checkpoint_protocol>true</checkpoint_protocol>

<step number="8" required="true">
**Must present next steps:**

```
✓ Creative brief complete: [PluginName]

What's next?
1. Quick params + parallel workflow (18 min faster) ← Provide minimal parameter list now, UI design happens in parallel with DSP research
2. Full UI mockup first (traditional workflow) ← Design UI now, parameters extracted afterward
3. Start implementation directly (not recommended - requires existing mockup)
4. Research similar plugins ← Find inspiration and examples
5. Save for later
6. Other

Choose (1-6): _
```

<delegation_rules>
**Handle responses:**

<delegation_rule skill="quick-params-workflow" trigger="option_1">
- Condition: User chooses option 1 (Quick params + parallel workflow)
- Action: Execute Phase 8.1 (Quick Parameter Capture)
- After capture complete: Present parallel execution decision menu
</delegation_rule>

<delegation_rule skill="ui-mockup" trigger="option_2">
- Condition: User chooses option 2 (Full UI mockup first)
- Action: Must invoke ui-mockup skill via Skill tool
- Do NOT attempt to create mockup within this skill
</delegation_rule>

<delegation_rule skill="plugin-workflow" trigger="option_3">
- Condition: User chooses option 3 (Start implementation directly)
- Action: Must invoke plugin-workflow skill via Skill tool
- Warning: Must warn user about contract requirements before delegating
</delegation_rule>

<delegation_rule skill="deep-research" trigger="option_4">
- Condition: User chooses option 4 (Research similar plugins)
- Action: Must invoke deep-research skill via Skill tool
</delegation_rule>

- Option 5 → Confirm handoff file created, exit
- Option 6 → Ask what they'd like to do
</delegation_rules>
</step>
</decision_gate>

## Phase 8.1: Quick Parameter Capture (Parallel Workflow Path)

<step number="8.1" required="conditional" condition="user_chose_quick_params">
**Only execute if user selected option 1 (Quick params + parallel workflow) in Phase 8.**

**Purpose:** Capture minimal parameter definitions to enable Stage 0 DSP research to begin immediately.

**Interactive capture workflow:**

```
Quick Parameter Capture for Stage 0

You'll provide minimal parameter definitions to enable DSP research.
Full UI design happens separately (in parallel).

Ready to capture parameters? (y/n): _
```

**For each parameter, collect via AskUserQuestion:**

1. **Parameter ID:**
   ```
   question: "Parameter ID (lowercase, no spaces, e.g., 'filterCutoff')?"
   header: "Param ID"
   options:
     - label: "[suggest from creative brief if mentioned]"
     - label: "Other", description: "Custom parameter ID"
   multiSelect: false
   ```

2. **Parameter Type:**
   ```
   question: "Parameter type?"
   header: "Type"
   options:
     - label: "Float", description: "Continuous value (knob/slider)"
     - label: "Choice", description: "Discrete options (dropdown/buttons)"
     - label: "Bool", description: "On/off toggle (switch/checkbox)"
   multiSelect: false
   ```

3. **Range/Choices (type-dependent):**

   **If Float:**
   ```
   question: "Range for [paramId]?"
   header: "Range"
   options:
     - label: "0 to 1", description: "Normalized (common for mix/gain)"
     - label: "20 to 20000 Hz", description: "Frequency range"
     - label: "-60 to 12 dB", description: "Decibel range"
     - label: "0 to 100 ms", description: "Time (milliseconds)"
     - label: "Other", description: "Custom range"
   ```

   Then ask for default value and units (if applicable).

   **If Choice:**
   ```
   question: "How many options for [paramId]?"
   header: "Options"
   options:
     - label: "2 options", description: "Binary choice"
     - label: "3 options", description: "Three-way"
     - label: "4+ options", description: "Multiple choices"
   ```

   Then collect option labels interactively.

   **If Bool:**
   ```
   question: "Default state for [paramId]?"
   header: "Default"
   options:
     - label: "Off (false)", description: "Starts disabled"
     - label: "On (true)", description: "Starts enabled"
   ```

4. **DSP Purpose:**
   ```
   question: "What does [paramId] control in the audio processing? (1-2 sentences)"
   header: "DSP Purpose"
   options:
     - label: "[suggest based on param name]"
     - label: "Other", description: "Custom description"
   ```

5. **Add another parameter?**
   ```
   question: "Add another parameter?"
   header: "Next"
   options:
     - label: "Yes", description: "Add another parameter"
     - label: "No", description: "That's all parameters"
   multiSelect: false
   ```

   Loop until user selects "No".

**After all parameters captured:**

1. Generate `parameter-spec-draft.md` using template from assets/parameter-spec-draft-template.md
2. Save to `plugins/[PluginName]/.ideas/parameter-spec-draft.md`
3. Update PLUGINS.md status to "💡 Ideated (Draft Params)"
4. Commit changes:
   ```bash
   git add plugins/[PluginName]/.ideas/parameter-spec-draft.md
   git add PLUGINS.md
   git commit -m "feat([PluginName]): draft parameters captured for parallel workflow"
   ```

5. Present parallel execution decision menu:
   ```
   ✓ Draft parameters captured: [N] parameters

   Files created:
   - parameter-spec-draft.md ([N] parameters)

   What's next?

   1. Start Stage 0 research now (recommended) ← Begin DSP research immediately
   2. Design UI mockup now ← Create visual design in parallel
   3. Do both in parallel ← Start research, then design (maximum time savings)
   4. Other

   Choose (1-4): _
   ```

**Handle menu choices:**

- **Option 1:** Invoke plugin-planning skill for Stage 0 (will accept draft params)
  ```
  Skill tool:
  - skill: "plugin-planning"
  - prompt: "Begin Stage 0 research for [PluginName] using draft parameter-spec"
  ```

- **Option 2:** Invoke ui-mockup skill (will validate against draft when generating full spec)
  ```
  Skill tool:
  - skill: "ui-mockup"
  - prompt: "Create UI mockup for [PluginName] using parameter-spec-draft.md"
  ```

- **Option 3:** Invoke plugin-planning skill first, then suggest ui-mockup in parallel
  ```
  Skill tool:
  - skill: "plugin-planning"
  - prompt: "Begin Stage 0 research for [PluginName]. After completion, suggest running ui-mockup in parallel."
  ```
  After plugin-planning completes, present option to start ui-mockup.

- **Option 4:** Collect custom input

</step>

</phases>
</critical_sequence>

---

## Grounded Feasibility

When user proposes ambitious ideas (physical modeling, ML, 3D graphics), flag technical complexity without shutting down creativity: "That's interesting! [Challenge] might be complex—we can research approaches in Stage 0. Continue exploring?"

## Continuous Iteration Support

User can request deep dives:

```
User: "Ask me more about the UI"
→ System focuses on UI-specific questions

User: "Let's explore presets"
→ System asks about preset strategy

User: "Tell me what you think about the DSP"
→ System provides feasibility analysis
```

**Support free-form exploration until user says "finalize."**

## Error Handling

**If plugin name contains invalid characters:**
```
Plugin names should be UpperCamelCase with no spaces or special characters.

Suggested: [CleanName]
Use this name? (y/n): _
```

**If creative brief already exists:**
```
Creative brief already exists for [PluginName].

Options:
1. View existing brief
2. Create improvement proposal instead (/improve)
3. Overwrite (will lose existing brief)

Choose (1-3): _
```
