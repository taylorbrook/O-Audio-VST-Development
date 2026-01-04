# Backward Compatibility - Stage 4/5 Migration

**Context:** This file is part of the context-resume skill.
**Purpose:** Handle legacy handoffs with Stage 4 or Stage 5 references (pre-unified validation system)

---

## Overview

Before the unified validation system (Prompts 060-062), the workflow had 5 stages:
- Stage 1: Build System Ready
- Stage 2: Audio Engine Working
- Stage 3: UI Integrated
- **Stage 4: Validation** (validation-agent as separate stage)
- **Stage 5: Final verification** (presets, packaging)

After unification:
- Stage 1: Build System Ready (validated automatically)
- Stage 2: Audio Engine Working (validated automatically)
- Stage 3: UI Integrated (validated automatically)
- After Stage 3 validation passes: Plugin complete, ready to install

## Detection

During Step 2 (Parse Context), check for legacy stage numbers:

```yaml
# Legacy handoff file
stage: 4
status: "Testing"
next_action: "Run pluginval"
```

OR

```yaml
# Legacy handoff file
stage: 5
status: "Finalizing"
next_action: "Create presets"
```

**Detection logic:**

```
if stage == 4 or stage == 5:
  legacy_workflow = true
  migrated_stage = 3
  migrated_status = "Stage 3 complete, plugin validated, ready to install"
```

## Migration

When legacy stage detected:

1. **Parse legacy state:**
   - If stage == 4 → Plugin completed implementation, needs validation
   - If stage == 5 → Plugin completed validation, needs installation

2. **Map to new state:**
   - Both Stage 4 and Stage 5 → "Stage 3 complete, plugin validated, ready to install"
   - Set stage = 3 in memory (don't update file yet)
   - Status = "Plugin complete and validated"

3. **Present migration notice:**
   ```
   ⚠️ WORKFLOW MIGRATION DETECTED

   This plugin was created with the 5-stage workflow (Stage [4/5]).
   The system now uses a 3-stage workflow with automatic validation.

   Old workflow:
   - Stage 1: Build System Ready
   - Stage 2: Audio Engine Working
   - Stage 3: UI Integrated
   - Stage 4: Validation (separate stage)
   - Stage 5: Finalization

   New workflow:
   - Stage 1: Build System Ready (validated automatically)
   - Stage 2: Audio Engine Working (validated automatically)
   - Stage 3: UI Integrated (validated automatically)
   - After Stage 3: Plugin complete, ready to install

   Your plugin is at: Stage 3 complete (validated and ready to install)

   What would you like to do?

   1. Install plugin to system folders (recommended)
   2. Run manual tests (/test command)
   3. Make improvements (/improve command)
   4. Other

   Choose (1-4): _
   ```

4. **Update handoff file after user chooses action:**
   - Update stage: 3
   - Update status: "Plugin complete and validated"
   - Add migration note: "Migrated from Stage [4/5] to Stage 3 (unified validation)"
   - Commit changes: `git commit -m "migrate: [PluginName] workflow to unified validation system"`

## Routing

After migration notice, route based on user selection:

**Option 1: Install plugin**
- Invoke plugin-lifecycle skill (Mode 1: Installation)
- Update PLUGINS.md status to 📦 Installed
- Update .continue-here.md to "installed" status

**Option 2: Run manual tests**
- Invoke plugin-testing skill
- Explain: "Automatic validation already ran during workflow. This is additional manual verification."
- After tests complete, offer installation

**Option 3: Make improvements**
- Invoke plugin-improve skill
- Normal improvement flow

**Option 4: Other**
- Collect free-form input
- Reassess based on response

## File Updates

After migration and user action completes:

**Update .continue-here.md:**
```yaml
plugin: [PluginName]
stage: 3
status: "Plugin complete and validated"
last_updated: [timestamp]
migration_note: "Migrated from Stage [4/5] on [date]"
validation: "Automatic validation completed during Stage 3"
ready_for: "installation"
```

**Update PLUGINS.md:**
- Keep existing status (✅ Working or 📦 Installed)
- No changes needed (status already correct)

**Commit migration:**
```bash
git add plugins/[PluginName]/.continue-here.md
git commit -m "migrate: [PluginName] workflow to unified validation (Stage [4/5] → Stage 3)"
```

## Edge Cases

**Case 1: Stage 4 handoff with failing tests**

Legacy state:
```yaml
stage: 4
status: "Testing failed"
next_action: "Fix validation errors"
errors: ["pluginval crash in VST3", "parameter automation broken"]
```

Migration:
- Map to Stage 3
- Status: "Stage 3 complete, validation found issues"
- Present migration notice with option to investigate errors
- Route to plugin-improve for fixes

**Case 2: Stage 5 handoff with presets created**

Legacy state:
```yaml
stage: 5
status: "Presets created"
next_action: "Install plugin"
presets_count: 5
```

Migration:
- Map to Stage 3
- Status: "Stage 3 complete, ready to install"
- Note: Presets already exist, no need to recreate
- Route to installation directly

**Case 3: Stage 4 handoff without validation results**

Legacy state:
```yaml
stage: 4
status: "Validation not started"
next_action: "Run pluginval"
```

Migration:
- Map to Stage 3
- Status: "Stage 3 complete, validation needed"
- Offer two paths:
  1. Trust automatic validation (already ran during Stage 3)
  2. Run manual tests to verify
- Recommend option 1 (validation already automatic)

## Success Criteria

Migration is successful when:

- Legacy stage detected correctly (4 or 5)
- User sees clear explanation of change
- Handoff file updated to new format
- Plugin routable to correct next action
- No functionality lost (presets, validation results preserved)
- Git commit documents migration

## Testing

Test these scenarios:

1. Stage 4 handoff with clean validation
2. Stage 5 handoff ready to install
3. Stage 4 handoff with errors
4. Missing validation results in Stage 4
5. User chooses each menu option (install, test, improve, other)
