---
name: plugin-handoff
description: Create handoff document for stage transition
skill: plugin-workflow
args: "[plugin_name?] [stage]"
---

# /plugin-handoff

Create a validated handoff document at stage completion for transition to the next stage.

## Usage

```
/plugin-handoff [plugin_name] [stage]    # Specific plugin and stage
/plugin-handoff [stage]                  # Focused plugin, specific stage
```

## Arguments

- `plugin_name` - Plugin to create handoff for (optional, defaults to focused)
- `stage` - Stage being completed: `0-ideation`, `1-foundation`, `2-dsp`, `3-gui`

**Note:** Stage 4-polish does not produce a handoff (final stage).

## Prerequisites

**Stage execution must be complete.** The stage should have SUMMARY.md confirming completion.

## Schema Mapping

| Stage | Handoff Schema | Next Stage |
|-------|---------------|------------|
| 0-ideation | handoff-0-to-1.schema.json | 1-foundation |
| 1-foundation | handoff-1-to-2.schema.json | 2-dsp |
| 2-dsp | handoff-2-to-3.schema.json | 3-gui |
| 3-gui | handoff-3-to-4.schema.json | 4-polish |

## Behavior

### 1. Load Plugin State

Read plugin's current state from:
- `plugins/[plugin]/.planning/STATUS.md` - Current stage/phase
- `.planning/workflow/registry.json` - Registered state

### 2. Gather Stage Information

From `plugins/[plugin]/.planning/stages/[stage]/`:
- `CONTEXT.md` - Stage context and goals
- `PLAN.md` - What was planned
- `SUMMARY.md` - What was done, decisions made

### 3. Create HANDOFF.json

Create `plugins/[plugin]/.planning/stages/[stage]/HANDOFF.json`:

```json
{
  "$schema": "../../../.planning/workflow/schemas/handoff-X-to-Y.schema.json",
  "schemaVersion": "1.0.0",
  "plugin": "[plugin-name]",
  "timestamp": "[ISO 8601 datetime]",
  "summary": "[one-sentence stage completion summary]",

  "stateSnapshot": {
    "stage": "[current-stage]",
    "phase": "complete",
    ... [stage-specific fields]
  },

  "artifacts": {
    "required": [
      { "path": "[relative/path/to/file]", "exists": true, "type": "[type]" }
    ],
    "validation": {
      "allRequiredPresent": true,
      "checkedAt": "[ISO 8601 datetime]"
    }
  },

  "decisions": [
    {
      "id": "[STAGE-NNN]",
      "decision": "[what was decided]",
      "rationale": "[why]",
      "date": "[YYYY-MM-DD]",
      "alternatives": ["[option1]", "[option2]"],
      "supersedes": null
    }
  ],

  "contextForNextStage": {
    "receivingStage": "[next-stage]",
    "keyInformation": [
      "[key info 1]",
      "[key info 2]"
    ],
    "continueCommand": "/plugin-execute [plugin] [next-stage]"
  }
}
```

### 4. Validate Handoff

Run validation script:

```bash
.planning/workflow/scripts/validate-handoff.sh plugins/[plugin]/.planning/stages/[stage]/HANDOFF.json
```

Must pass:
- Schema validation against handoff-X-to-Y.schema.json
- Artifact existence (all paths with `exists: true` must exist)

### 5. Update Central Decision Log

Append decisions to `plugins/[plugin]/.planning/DECISIONS.md`:

```markdown
## [Stage] Decisions

| ID | Decision | Date |
|----|----------|------|
| FOUNDATION-001 | Use APVTS for parameter management | 2026-01-29 |
```

### 6. Output

```
Handoff created: plugins/[plugin]/.planning/stages/[stage]/HANDOFF.json

Validation: PASSED
Decisions logged: 3 entries appended to DECISIONS.md

Stage transition ready.
Next: /plugin-execute [plugin] [next-stage]
```

## Stage-Specific Fields

### 0-ideation to 1-foundation
```json
"stateSnapshot": {
  "stage": "0-ideation",
  "phase": "complete",
  "parameterCount": [number],
  "ideaValidated": true
}
```

### 1-foundation to 2-dsp
```json
"stateSnapshot": {
  "stage": "1-foundation",
  "phase": "complete",
  "parameterCount": [number],
  "buildVerified": true,
  "source": ["STATUS.md", "registry.json"]
}
```

### 2-dsp to 3-gui
```json
"stateSnapshot": {
  "stage": "2-dsp",
  "phase": "complete",
  "dspComplexity": "[low|medium|high]",
  "realtimeSafe": true,
  "source": ["STATUS.md", "registry.json"]
}
```

### 3-gui to 4-polish
```json
"stateSnapshot": {
  "stage": "3-gui",
  "phase": "complete",
  "uiComplete": true,
  "webviewIntegrated": true,
  "source": ["STATUS.md", "registry.json"]
}
```

## Decision ID Pattern

Generate IDs as `[STAGE-PREFIX]-[NNN]`:
- 0-ideation: `IDEATION-001`, `IDEATION-002`
- 1-foundation: `FOUNDATION-001`, `FOUNDATION-002`
- 2-dsp: `DSP-001`, `DSP-002`
- 3-gui: `GUI-001`, `GUI-002`

Check existing DECISIONS.md to avoid ID collisions.

## Example

```
/plugin-handoff O-IntonationPad 1-foundation

Creating handoff for O-IntonationPad stage 1-foundation
==============================================================

Loading stage context...
  CONTEXT.md: Found
  PLAN.md: Found
  SUMMARY.md: Found

Gathering artifacts...
  Source/PluginProcessor.h: exists
  Source/PluginProcessor.cpp: exists
  CMakeLists.txt: exists

Extracting decisions...
  3 decisions found in SUMMARY.md

Creating HANDOFF.json...
  Schema: handoff-1-to-2.schema.json
  Path: plugins/O-IntonationPad/.planning/stages/1-foundation/HANDOFF.json

Validating...
  Schema: PASSED
  Artifacts: 3/3 present

Updating DECISIONS.md...
  3 entries appended

HANDOFF CREATED
==============================================================
Path: plugins/O-IntonationPad/.planning/stages/1-foundation/HANDOFF.json
Next: /plugin-execute O-IntonationPad 2-dsp
```

## Error Handling

### Stage not complete
```
ERROR: Stage 1-foundation not complete

STATUS.md shows phase: "execute" (not "complete")
Complete stage execution before creating handoff.

Run: /plugin-verify O-IntonationPad 1-foundation
```

### Missing artifacts
```
ERROR: Artifacts missing

Required files not found:
  - Source/DSP/ChordGenerator.h
  - Source/DSP/ChordGenerator.cpp

Fix missing files and retry.
```

### Validation failure
```
ERROR: Handoff validation failed

Schema errors:
  - stateSnapshot.buildVerified: required field missing

Fix handoff and retry, or use --force to bypass (not recommended).
```

## Related Commands

- `/plugin-execute` - Execute stage (produces content for handoff)
- `/plugin-verify` - Verify stage before handoff
- `/plugin-status` - Check stage completion status
