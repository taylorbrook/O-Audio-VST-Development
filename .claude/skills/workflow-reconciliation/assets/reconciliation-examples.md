# Reconciliation Report Examples

## Example 1: After UI Mockup Completion

### Scenario
User completed Phase 6 of ui-mockup workflow but didn't commit changes. Reconciliation detects uncommitted files and offers to fix.

### Report Output

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📋 Reconciliation Report for DriveVerb
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Current Workflow: ui-mockup (Phase 6 - Complete)

State File Analysis:

✓ .planning/STATUS.md
  Status: UP TO DATE
  Last updated: 2025-11-12T14:30:00Z

✓ parameter-spec.md
  Status: LOCKED AND CURRENT

✗ PLUGINS.md
  Current: 🚧 Stage 3
  Expected: ✅ Working (after UI implementation)
  Action: UPDATE status emoji

Git Status:

✓ Staged: None
✗ Unstaged: 5 files
  - plugins/DriveVerb/.planning/mockups/v1-PluginEditor.h
  - plugins/DriveVerb/.planning/mockups/v1-PluginEditor.cpp
  - plugins/DriveVerb/.planning/mockups/v1-CMakeLists.txt
  - plugins/DriveVerb/.planning/mockups/v1-integration-checklist.md
  - plugins/DriveVerb/.planning/mockups/v1-ui.html
✗ Untracked: None
✗ Uncommitted: 5 workflow artifacts

Proposed Actions:

1. Update PLUGINS.md status: 🚧 Stage 3 → ✅ Working
2. Stage all 5 mockup implementation files
3. Commit with message: "feat(DriveVerb): Complete UI mockup v1 with implementation files"

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

What should I do?
1. Fix everything automatically - Execute all updates and commit
2. Show me the diffs first - Preview file changes before committing
3. Fix files only (no commit) - Update state files but don't stage/commit
4. Update .planning/STATUS.md only - Minimal checkpoint
5. Skip reconciliation - I'll handle it manually
6. Other

Choose (1-6): _
```

### User Choice: 1 (Fix automatically)

### Remediation Output

```
Reconciliation complete:
✓ Updated PLUGINS.md status to ✅
✓ Staged 5 files
✓ Committed: feat(DriveVerb): Complete UI mockup v1 with implementation files

All state files synchronized. Workflow can resume without context loss.
```

---

## Example 2: After Subagent Completion

### Scenario
foundation-shell-agent completed Stage 1 but didn't create .planning/STATUS.md handoff file. Reconciliation detects missing handoff and offers to create it.

### Report Output

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📋 Reconciliation Report for SimpleTremolo
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Current Workflow: plugin-workflow (Stage 1 - Complete)

State File Analysis:

✗ .planning/STATUS.md
  Status: MISSING
  Expected: Handoff file with stage=2, status=complete
  Action: CREATE handoff file

✓ PLUGINS.md
  Status: UP TO DATE
  Entry: 🚧 Stage 2 | SimpleTremolo

Git Status:

✓ Staged: 4 files (CMakeLists.txt, PluginProcessor.h, PluginProcessor.cpp, README.md)
✗ Unstaged: None
✗ Untracked: None
✗ Uncommitted: Staged changes not committed

Proposed Actions:

1. Create .planning/STATUS.md with:
   - plugin: SimpleTremolo
   - workflow: plugin-workflow
   - stage: 2
   - status: complete
   - last_updated: 2025-11-12T15:45:00Z
2. Commit staged changes with message: "feat(SimpleTremolo): Complete Stage 1 (Foundation) - build system operational"

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

What should I do?
1. Fix everything automatically - Create handoff and commit
2. Show me the diffs first - Preview handoff file content
3. Fix files only (no commit) - Create handoff but don't commit
4. Update .planning/STATUS.md only - Create handoff, ignore staged changes
5. Skip reconciliation - I'll handle it manually
6. Other

Choose (1-6): _
```

### User Choice: 2 (Show diffs first)

### Diff Preview

```
Creating .planning/STATUS.md with content:

---
plugin: SimpleTremolo
workflow: plugin-workflow
stage: 2
status: complete
last_updated: 2025-11-12T15:45:00Z
---

# plugin-workflow - Stage 1 - Complete

Foundation layer established. Build system operational, plugin loads successfully.

Next: Invoke dsp-agent for Stage 2 (DSP Core implementation).

─────────────────────────────────────────────────────

Commit will include:
- plugins/SimpleTremolo/.planning/STATUS.md (new file)
- plugins/SimpleTremolo/CMakeLists.txt
- plugins/SimpleTremolo/Source/PluginProcessor.h
- plugins/SimpleTremolo/Source/PluginProcessor.cpp
- plugins/SimpleTremolo/README.md

Commit message:
"feat(SimpleTremolo): Complete Stage 1 (Foundation) - build system operational"

─────────────────────────────────────────────────────

What should I do?
1. Proceed with commit
2. Abort reconciliation
3. Other

Choose (1-3): _
```

---

## Example 3: No Gaps Found

### Scenario
User runs `/reconcile` on a plugin with all state files current and no uncommitted changes.

### Report Output

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📋 Reconciliation Report for DriveVerb
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Current Workflow: plugin-workflow (Stage 2 - In Progress)

State File Analysis:

✓ .planning/STATUS.md
  Status: UP TO DATE
  Last updated: 2025-11-12T14:30:00Z

✓ PLUGINS.md
  Status: UP TO DATE
  Entry: 🚧 Stage 2 | DriveVerb

Git Status:

✓ Staged: None
✓ Unstaged: None
✓ Untracked: None
✓ Uncommitted: None

Proposed Actions:

None - all state files synchronized.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

What's next?
1. All good - return to workflow (recommended)
2. Show me the state files anyway
3. Force reconciliation (update timestamps)
4. Other

Choose (1-4): _
```
