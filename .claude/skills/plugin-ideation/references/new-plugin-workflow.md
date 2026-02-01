# New Plugin Workflow

This workflow guides you through capturing a complete creative vision for a new plugin.

**Progress Phases:**
1. Phase 1: Free-form collection
2. Phase 2: Gap analysis
3. Phase 3: Question batch generation
4. Phase 4: Decision gate (finalize or iterate)
5. Phase 5: Plugin name validation (if needed)
6. Phase 6: Document creation (BRIEF.md)
7. Phase 6.5: Requirements extraction (REQUIREMENTS.md)
8. Phase 7: Session handoff (.planning/STATUS.md)
9. Phase 8: Decision menu (next action)

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
    - label: "Yes, finalize it", description: "Create BRIEF.md"
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

**File:** `plugins/[PluginName]/.planning/BRIEF.md`

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

- [ ] Create UI mockup (`/start [PluginName]` → option 3)
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

## Phase 6.5: Requirements Extraction

<step number="6.5" required="true">
**Must extract formal requirements from BRIEF.md immediately after creating it.**

**Purpose:** Transform creative brief into explicit, verifiable requirements with acceptance criteria.

**Extraction Process:**

1. **Read the just-created BRIEF.md**
2. **Extract requirements by category:**

   | Source Section | Requirement Category |
   |---------------|---------------------|
   | Vision / Core Concept | FUNC (functional) |
   | Parameters table | DSP, FUNC |
   | UI Concept | UI |
   | Technical Notes | PERF, QUAL |
   | Use Cases | FUNC, COMPAT |

3. **Assign priorities:**
   - `must` — Core functionality that defines the plugin (blocks release)
   - `should` — Expected features for good UX (strong expectation)
   - `nice` — Enhancements that add polish (optional for v1.0)

4. **Add standard requirements:**
   - PERF-01: Real-time safe processing (always `must`)
   - COMPAT-01: Passes pluginval (always `must`)
   - QUAL-01: No audio artifacts (always `must`)

5. **Generate acceptance criteria:**
   - Each requirement needs 1-3 specific, testable criteria
   - Criteria should be verifiable during `/plugin-verify`

**File:** `plugins/[PluginName]/.planning/REQUIREMENTS.md`

**Format:**
```markdown
# [PluginName] - Requirements

---
version: 1.0.0
plugin: [PluginName]
created: [YYYY-MM-DD]
lastUpdated: [YYYY-MM-DD]
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** [N]
**Coverage:** must: [N] | should: [N] | nice: [N]

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | [Extracted from core concept] | must | pending | stage-2 |
| FUNC-02 | [Extracted from use cases] | should | pending | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | [Extracted from parameters/technical notes] | must | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | [Extracted from UI concept] | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock) | must | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges | must | pending | stage-2 |

## Acceptance Criteria Details

### FUNC-01: [Title]

**Description:** [Full description from brief]

**Acceptance Criteria:**
- [ ] [Testable criterion 1]
- [ ] [Testable criterion 2]

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-*, DSP-*, PERF-01, QUAL-* |
| stage-3 | UI-* |
| stage-4 | COMPAT-*, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| [From BRIEF.md if mentioned] | [Reason] | v1.1+ |

---
*Generated from BRIEF.md on [YYYY-MM-DD]*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
```

**Verification:** Requirements document must have:
- At least 1 `must` priority requirement per category (FUNC, DSP, PERF, COMPAT, QUAL)
- Acceptance criteria for all `must` requirements
- Traceability to verification stage

**Commit:**
```bash
git add plugins/[PluginName]/.planning/REQUIREMENTS.md
git commit -m "docs([PluginName]): extract requirements from creative brief"
```
</step>

## Phase 7: Session Handoff

<state_requirement>
<requirement>must_create_continue_file</requirement>
<step number="7" required="true">
**Must create handoff file for resuming later:**

**File:** `plugins/[PluginName]/.planning/STATUS.md`

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
- Requirements extracted with acceptance criteria

## Next Steps

1. Create UI mockup to visualize design (recommended)
2. Start implementation directly
3. Research similar plugins for inspiration

## Context to Preserve

**Key Decisions:**
- Plugin type: [Effect/Synth/Utility]
- Core concept: [Brief summary]

**Files Created:**
- plugins/[PluginName]/.planning/BRIEF.md
- plugins/[PluginName]/.planning/REQUIREMENTS.md
```
</step>
</state_requirement>

## Phase 8: Decision Menu (Handoff Point)

<decision_gate>
<gate_name>next_action</gate_name>
<blocking>true</blocking>
<checkpoint_protocol>true</checkpoint_protocol>
<handoff_required>true</handoff_required>

<step number="8" required="true">
**CRITICAL: This is a handoff point. Present clean continuation format, do NOT auto-proceed.**

**Must present using continuation format:**

```
---

## ✓ Ideation Complete

**[PluginName]** — [one-line description from brief]

Files created:
- `plugins/[PluginName]/.planning/BRIEF.md`
- `plugins/[PluginName]/.planning/REQUIREMENTS.md`
- `plugins/[PluginName]/.planning/STATUS.md`

---

## ▶ Next Up

**Stage 0: Planning** — Research DSP approach and create architecture

`/plan [PluginName]`

<sub>`/clear` first → fresh context window</sub>

---

**Also available:**

- `/start [PluginName]` → Create UI mockup first (option 3)
- `/research [topic]` → Research similar plugins
- Save for later (handoff file created)

---
```

**Do NOT invoke any skills directly.** Present the handoff and STOP.

The user will:
1. Run `/clear` to reset context
2. Paste the command they want
3. Continue in a fresh context window

**Why handoffs matter:**
- Ideation consumes significant context tokens
- Planning needs fresh context for research agents
- Clean handoffs prevent context pollution
- User controls when to proceed

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
2. Save to `plugins/[PluginName]/.planning/parameter-spec-draft.md`
3. Update PLUGINS.md status to "💡 Ideated (Draft Params)"
4. Commit changes:
   ```bash
   git add plugins/[PluginName]/.planning/parameter-spec-draft.md
   git add PLUGINS.md
   git commit -m "feat([PluginName]): draft parameters captured for parallel workflow"
   ```

5. Present handoff with continuation format:

   ```
   ---

   ## ✓ Draft Parameters Captured

   **[PluginName]** — [N] parameters defined

   Files created:
   - `plugins/[PluginName]/.planning/parameter-spec-draft.md`

   ---

   ## ▶ Next Up

   **Stage 0: Planning** — Research DSP approach and create architecture

   `/plan [PluginName]`

   <sub>`/clear` first → fresh context window</sub>

   ---

   **Also available:**

   - `/start [PluginName]` → Create UI mockup (can run in parallel)
   - Save for later (handoff file created)

   ---
   ```

**Do NOT invoke any skills directly.** Present the handoff and STOP.

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
