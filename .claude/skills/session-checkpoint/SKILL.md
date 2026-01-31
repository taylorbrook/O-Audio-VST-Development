# Session Checkpoint Skill

## Purpose

Create and restore checkpoints for session continuity. Checkpoints capture task-level state so users can resume exactly where they left off.

This skill implements fine-grained checkpointing: a checkpoint is created after each significant task completes, enabling precise resume points rather than coarse stage-level recovery.

## Checkpoint Location

```
.planning/workflow/checkpoints/
├── {plugin}/
│   ├── {timestamp}-{task}.json    # Individual checkpoints
│   └── latest.json                # Copy of most recent checkpoint
└── .index.json                    # Fast lookup index across all plugins
```

## Checkpoint Creation

### When to Create

- After each task completes in a PLAN.md
- On context clear before handoff
- On `/pause` command
- On explicit `/checkpoint` command

### Checkpoint Format

Checkpoints follow the schema at `.planning/workflow/schemas/checkpoint.schema.json`:

```json
{
  "$schema": "../schemas/checkpoint.schema.json",
  "version": "1.0.0",
  "plugin": "O-IntonationPad",
  "timestamp": "2026-01-30T14:30:00Z",
  "stage": "2-dsp",
  "phase": "execute",
  "task": {
    "plan": "02-01-PLAN.md",
    "taskNumber": 3,
    "taskTitle": "Implement wavetable oscillator"
  },
  "state": {
    "completedTasks": [1, 2, 3],
    "remainingTasks": [4, 5],
    "handoffContext": "WavetableOscillator class complete with 2048-sample tables"
  },
  "filesModified": [
    "plugins/O-IntonationPad/Source/DSP/WavetableOscillator.h",
    "plugins/O-IntonationPad/Source/DSP/WavetableOscillator.cpp"
  ]
}
```

### Creation Protocol

1. **Gather current state:**
   - Read `plugins/{plugin}/.planning/STATUS.md` for stage, phase
   - Identify current plan file and task progress
   - Collect list of files modified in current session

2. **Generate timestamp:**
   - Use ISO 8601 format: `YYYY-MM-DDTHH:MM:SSZ`
   - For file names, replace `:` with `-` for filesystem safety

3. **Write checkpoint file:**
   - Path: `checkpoints/{plugin}/{timestamp}-task-{N}.json`
   - Validate against schema before writing
   - Example: `checkpoints/O-IntonationPad/2026-01-30T14-30-00-task-03.json`

4. **Update latest pointer:**
   - Copy checkpoint to `checkpoints/{plugin}/latest.json`
   - This provides O(1) lookup for resume

5. **Update index:**
   - Add entry to `.index.json`
   - Prune if count > 15 (see Retention Policy)

### File Naming Convention

`{YYYY-MM-DD}T{HH-MM-SS}-task-{N}.json`

Examples:
- `2026-01-30T14-30-00-task-03.json`
- `2026-01-30T16-45-12-task-01.json`

### Creation Implementation

```bash
# 1. Create plugin checkpoint directory if needed
mkdir -p .planning/workflow/checkpoints/{plugin}

# 2. Generate file-safe timestamp
TIMESTAMP=$(date -u +"%Y-%m-%dT%H-%M-%S")

# 3. Write checkpoint (example with jq)
cat > .planning/workflow/checkpoints/{plugin}/${TIMESTAMP}-task-{N}.json << 'EOF'
{
  "$schema": "../schemas/checkpoint.schema.json",
  "version": "1.0.0",
  ...
}
EOF

# 4. Copy to latest
cp .planning/workflow/checkpoints/{plugin}/${TIMESTAMP}-task-{N}.json \
   .planning/workflow/checkpoints/{plugin}/latest.json
```

## Checkpoint Restoration

### Protocol

1. **Locate checkpoint:**
   - Read `checkpoints/{plugin}/latest.json`
   - If no latest.json, check `.index.json` for last known checkpoint
   - If no checkpoints exist, fall back to STATUS.md only

2. **Validate checkpoint:**
   - Verify JSON is parseable
   - Validate against checkpoint.schema.json
   - Check referenced files still exist

3. **Load state:**
   - Set stage/phase from checkpoint
   - Load referenced PLAN.md file
   - Set task pointer to first remaining task
   - Parse handoffContext for resume context

4. **Load supporting context:**
   - Read `plugins/{plugin}/.planning/STATUS.md`
   - Read `plugins/{plugin}/.planning/stages/{stage}/CONTEXT.md` if exists
   - Read current `PLAN.md` if in execute phase
   - DO NOT load historical summaries or other plugins' state

5. **Display resume summary:**
   - Show plugin, stage, phase
   - Show last completed task and next task
   - Show files from last session
   - Show next command to run

### Resume Output Format

**With checkpoint:**
```
SESSION RESUMED
===============
Plugin: O-IntonationPad
Stage: 2-dsp
Phase: execute

Last completed: Task 3: Implement wavetable oscillator
Next task: Task 4: Add wavetable interpolation

Files from last session:
- Source/DSP/WavetableOscillator.h
- Source/DSP/WavetableOscillator.cpp

Handoff context: WavetableOscillator class complete with 2048-sample tables

State: healthy (validated)

Ready to continue. Next: complete Task 4 or /status for full context.
```

**Without checkpoint (first session):**
```
SESSION RESUMED
===============
Plugin: O-IntonationPad
Stage: 2-dsp
Phase: execute

No checkpoint found - starting fresh from STATUS.md

Current state:
- Stage: 2-dsp
- Phase: execute

Next: Run /plugin:execute to begin execution
```

**After context clear:**
```
RESUMING SESSION
================
Last session ended at: 2026-01-30T14:30:00Z
Reason: Context clear for fresh execution

Plugin: O-IntonationPad
Stage: 2-dsp
Phase: execute
Plan: 02-01-PLAN.md

Next step: Run /plugin:execute to continue plan execution

Copy command: /plugin:execute
```

### Restoration Implementation

```bash
# 1. Check for latest checkpoint
LATEST=".planning/workflow/checkpoints/{plugin}/latest.json"
if [[ -f "$LATEST" ]]; then
    # 2. Parse checkpoint
    STAGE=$(jq -r '.stage' "$LATEST")
    PHASE=$(jq -r '.phase' "$LATEST")
    PLAN=$(jq -r '.task.plan' "$LATEST")
    LAST_TASK=$(jq -r '.task.taskNumber' "$LATEST")
    REMAINING=$(jq -r '.state.remainingTasks[0]' "$LATEST")
    HANDOFF=$(jq -r '.state.handoffContext' "$LATEST")

    # 3. Load context files
    cat "plugins/{plugin}/.planning/STATUS.md"
    cat "plugins/{plugin}/.planning/stages/${STAGE}/${PLAN}"
fi
```

## Index Management

`.index.json` provides fast lookup across all plugins:

```json
{
  "lastUpdated": "2026-01-30T14:30:00Z",
  "checkpoints": {
    "O-IntonationPad": {
      "latest": "2026-01-30T14-30-00-task-03.json",
      "count": 5,
      "oldestKept": "2026-01-30T10-00-00-task-01.json"
    },
    "O-AnotherPlugin": {
      "latest": "2026-01-29T09-15-00-task-02.json",
      "count": 2,
      "oldestKept": "2026-01-29T08-00-00-task-01.json"
    }
  }
}
```

### Index Update Protocol

On checkpoint creation:

1. Read existing `.index.json` (or initialize if missing)
2. Update plugin entry:
   - Set `latest` to new checkpoint filename
   - Increment `count`
   - Update `oldestKept` if pruning occurred
3. Update `lastUpdated` timestamp
4. Write back to `.index.json`

## Retention Policy

Prevent checkpoint bloat while preserving important recovery points.

### Keep Per Plugin

- **Last 10 checkpoints** - Recent history for fine-grained rollback
- **All phase-boundary checkpoints** - Checkpoints where phase changes (verify -> next phase)
- **Oldest checkpoint per stage** - First checkpoint of each stage for stage-level rollback

### Prune Trigger

When checkpoint count exceeds 15 for a plugin:

1. Identify checkpoints to keep (based on rules above)
2. Delete excess checkpoints (oldest first, excluding protected ones)
3. Update `.index.json` with new `oldestKept`

### Pruning Implementation

```bash
# Count checkpoints for plugin
COUNT=$(ls -1 .planning/workflow/checkpoints/{plugin}/*.json 2>/dev/null | grep -v latest.json | wc -l)

if [[ $COUNT -gt 15 ]]; then
    # List all checkpoints sorted by timestamp (oldest first)
    # Delete until count <= 10, keeping:
    # - Phase boundary checkpoints
    # - Oldest per stage
    # - Last 10
fi
```

### Phase Boundary Detection

A checkpoint is a phase boundary if the next checkpoint (or current STATUS.md) has a different phase:

```
checkpoint-task-05.json: phase = "execute"
checkpoint-task-06.json: phase = "verify"   <- phase boundary
checkpoint-task-07.json: phase = "verify"
```

## Error Handling

### Checkpoint Not Found

```
No checkpoint found for O-IntonationPad.

Options:
1. Start from STATUS.md state
2. Run /reconcile to rebuild state

Proceeding with STATUS.md...
```

### Corrupted Checkpoint

```
Checkpoint corrupted: invalid JSON structure

Recovery options:
1. Use previous checkpoint (if available)
2. Start from STATUS.md state
3. Run /reconcile to rebuild

Select option [1-3]:
```

### Schema Validation Failure

```
Checkpoint validation failed:
- Missing required field: task.taskTitle
- Invalid phase value: "exec" (expected: discuss|research|plan|execute|verify)

Recovery options:
1. Ignore checkpoint, use STATUS.md
2. Attempt auto-repair (fill defaults)
3. Manual edit checkpoint file

Select option [1-3]:
```

## Related Skills

- **state-validation** (@.claude/skills/state-validation/SKILL.md) - Validates state before loading checkpoint
- **state-recovery** (@.claude/skills/state-recovery/SKILL.md) - Recovery when checkpoint is corrupted
- **context-resume** - Higher-level resume orchestration using this skill

## Integration Points

### Commands Using This Skill

| Command | Usage |
|---------|-------|
| `/continue` | Restore checkpoint on session resume |
| `/pause` | Create checkpoint before pausing |
| `/checkpoint` | Explicit checkpoint creation |
| `/plugin:execute` | Create checkpoint after each task |

### Automatic Checkpoint Triggers

1. **Task completion** - After each `<task>` in a PLAN.md completes
2. **Phase transition** - When moving from one phase to another
3. **Context clear** - Before session handoff
4. **User request** - Via `/pause` or `/checkpoint`
