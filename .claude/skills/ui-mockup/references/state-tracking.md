# State Tracking Protocol

## Overview

State management in ui-mockup workflow uses `.continue-here.md` for workflow mode and skips state updates in standalone mode.

## Mode Detection

Check for workflow vs standalone mode before any state operation:

**Workflow Mode:**
- File exists: `plugins/[PluginName]/.continue-here.md`
- File contains: `current_stage` field
- Actions: Update state after each phase completion

**Standalone Mode:**
- No `.continue-here.md` file present
- Invoked directly via natural language or /dream
- Actions: Skip all state updates, generate mockups independently

## State Schema

### Fields Managed by ui-mockup Skill

```yaml
# Mockup versioning
mockup_latest_version: 3              # Highest v[N] in .ideas/mockups/
mockup_finalized: true                # Design approved via Phase 5.5 menu
finalized_version: 2                  # Which version was finalized

# Brief synchronization
brief_updated_from_mockup: true       # Phase 5.6 automatic update completed
mockup_version_synced: 2              # Which mockup version synced to brief
brief_update_timestamp: "2025-11-13T15:30:00Z"  # ISO-8601 format

# Phase completion tracking
ui_design_phase_complete: true        # Phase A (YAML + test HTML) done
ui_scaffolding_phase_complete: true   # Phase B (5 implementation files) done

# Workflow integration
next_action: "proceed_to_stage_1"     # Set after mockup finalization
```

### Fields Read (Not Modified) by ui-mockup

```yaml
# Read for workflow detection
current_stage: 0                      # Stage number (0-4)
plugin_name: "TapeAge"                # Plugin being developed

# Read for context
workflow_mode: "express"              # Manual or express mode
```

## State Update Responsibility

### Subagents Update State

**ui-design-agent updates:**
- `mockup_latest_version` → Incremented after creating v[N]-ui.yaml
- `ui_design_phase_complete` → Set to true after successful YAML + HTML generation
- Commits changes before returning JSON report

**ui-finalization-agent updates:**
- `ui_scaffolding_phase_complete` → Set to true after all 5 implementation files created
- `next_action` → Set based on workflow state (proceed to Stage 1, or mark standalone complete)
- Commits changes before returning JSON report

**Orchestrator updates:**
- `mockup_finalized` → Set to true when user selects "Finalize" in Phase 5.5
- `finalized_version` → Set to version user approved
- `brief_updated_from_mockup` → Set to true after Phase 5.6 brief sync
- `mockup_version_synced` → Version number synced to brief
- `brief_update_timestamp` → ISO-8601 timestamp of sync

### Update Timing

**After Phase A completion (ui-design-agent):**
```yaml
mockup_latest_version: {N}
ui_design_phase_complete: true
```

**After Phase 5.5 user approval:**
```yaml
mockup_finalized: true
finalized_version: {N}
```

**After Phase 5.6 brief sync:**
```yaml
brief_updated_from_mockup: true
mockup_version_synced: {N}
brief_update_timestamp: "{ISO-8601}"
```

**After Phase B completion (ui-finalization-agent):**
```yaml
ui_scaffolding_phase_complete: true
next_action: "proceed_to_stage_1"
```

## State Verification Protocol

### Problem: Boolean Flags vs Content Correctness

Subagents return `stateUpdated: true` to indicate they attempted state modification. However, flag doesn't guarantee correctness - agent could write wrong values.

### Solution: Content Verification

After subagent returns `stateUpdated: true`, orchestrator verifies actual state contents match expected values.

### Verification After ui-design-agent

**Expected state changes:**
```yaml
mockup_latest_version: {N}           # Must match version agent reported in JSON
ui_design_phase_complete: true       # Must be set to true
```

**Verification logic:**
1. Read `.continue-here.md`
2. Parse YAML
3. Check `mockup_latest_version == JSON.version`
4. Check `ui_design_phase_complete == true`
5. If mismatch → Present state recovery menu

### Verification After ui-finalization-agent

**Expected state changes:**
```yaml
ui_scaffolding_phase_complete: true  # Must be set to true
next_action: "proceed_to_stage_1"    # Must be set (workflow mode)
```

**Verification logic:**
1. Read `.continue-here.md`
2. Parse YAML
3. Check `ui_scaffolding_phase_complete == true`
4. Check `next_action` is set (if workflow mode)
5. If mismatch → Present state recovery menu

### State Recovery Menu

When verification fails or agent returns `stateUpdated: false`:

```
⚠️ State verification failed: [specific mismatch]

What would you like to do?

1. Verify manually - Read .continue-here.md and check contents
2. Fix state - Update fields to correct values
3. Continue anyway - Proceed despite state mismatch
4. Other

Choose (1-4): _
```

## Defense in Depth

**Layer 1:** Agent sets `stateUpdated: true` in JSON report
- Indicates agent attempted modification
- Not proof of correctness

**Layer 2:** Orchestrator verifies field contents
- Reads actual state file
- Compares against expected values from JSON report
- Catches silent corruption from agent bugs

**Why both layers:**
- Boolean flag: Fast check that agent didn't skip state update entirely
- Content verification: Ensures values are correct, not just present

## Cross-Skill Coordination

### plugin-workflow Skill Integration

**plugin-workflow reads ui-mockup state:**
- `mockup_finalized` → Determines if UI design phase complete
- `finalized_version` → Which mockup to use in Stage 3 (GUI implementation)
- `ui_scaffolding_phase_complete` → All implementation files ready

**plugin-workflow owns workflow mode detection:**
- ui-mockup accepts workflow mode as context (doesn't detect independently)
- Reduces coupling between skills
- Single responsibility: plugin-workflow orchestrates stages, ui-mockup handles UI

### plugin-improve Skill Integration

**plugin-improve reads ui-mockup state:**
- `mockup_latest_version` → Determines next version number for UI redesign
- `finalized_version` → Current production UI version

**plugin-improve may invoke ui-mockup:**
- When redesigning UI as feature improvement
- Passes workflow context (not detected by ui-mockup)

## State Conflicts and Resolution

### Scenario: Missing State File

If `.continue-here.md` doesn't exist but workflow context suggests it should:

1. Log warning
2. Assume standalone mode
3. Continue without state updates
4. Present completion menu explaining state wasn't tracked

### Scenario: Corrupt State YAML

If YAML parse fails:

1. Present error menu:
```
✗ State file parse failed: [parse error]

What would you like to do?

1. Inspect file - Show .continue-here.md contents
2. Reset state - Recreate with current values
3. Manual fix - I'll edit YAML manually
4. Other

Choose (1-4): _
```

### Scenario: Version Mismatch

If state says `mockup_latest_version: 2` but filesystem has v3-ui.yaml:

1. Trust filesystem (source of truth)
2. Update state to match filesystem
3. Log warning about mismatch
4. Continue with corrected version

## Commit Protocol

**Who commits:**
- Subagents commit their own state changes
- Orchestrator commits state changes it makes directly (finalization, brief sync)

**Commit message format:**
```bash
# After Phase A (ui-design-agent)
git commit -m "feat(${PLUGIN_NAME}): UI mockup v${VERSION} design iteration"

# After user approval (orchestrator)
git commit -m "docs(${PLUGIN_NAME}): finalize UI mockup v${VERSION}"

# After Phase 5.6 brief sync (orchestrator)
git commit -m "docs(${PLUGIN_NAME}): sync creative brief with finalized mockup v${VERSION}"

# After Phase B (ui-finalization-agent)
git commit -m "feat(${PLUGIN_NAME}): UI mockup v${VERSION} implementation scaffolding"
```

## Testing State Management

### Verification Checklist

After ui-mockup workflow completes:

1. **Read `.continue-here.md`**
   - All ui-mockup fields present
   - Values match what subagents reported in JSON
   - YAML is valid (no parse errors)

2. **Check filesystem matches state**
   - `mockup_latest_version` matches highest v[N] in `.ideas/mockups/`
   - `finalized_version` has corresponding finalization marker in YAML
   - All files reported in JSON `filesCreated` actually exist

3. **Verify git history**
   - State changes committed after each phase
   - Commit messages follow format above
   - No uncommitted state changes left

4. **Test state recovery**
   - Manually corrupt state field
   - Run workflow continuation
   - Verify recovery menu appears
   - Verify correction restores valid state
