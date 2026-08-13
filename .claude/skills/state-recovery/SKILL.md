# State Recovery Skill

## Purpose

Repair corrupted or inconsistent Plugin Freedom System state. Always prompts user before making changes.

This skill is invoked after state-validation detects issues. It presents recovery options, explains tradeoffs, and executes the user's chosen repair strategy.

## Recovery Principle

**Never auto-repair silently.** Present options, explain tradeoffs, let user choose.

Silent auto-repair erodes trust. Users should understand what changed and why.

## Recovery Options

### Option 1: Manual Repair Instructions

**When to offer:** Any inconsistency or corruption
**What it does:** Provides exact commands/edits to fix the issue manually
**Risk level:** LOWEST (user controls every change)

**Format:**
```
MANUAL REPAIR INSTRUCTIONS
==========================

Issue: PLUGINS.md and STATUS.md disagree on stage

Steps:
1. Open: plugins/O-IntonationPad/.planning/STATUS.md
2. Find: stage: 1-foundation
3. Change to: stage: 2-dsp
4. Save file
5. Run /reconcile to verify

Alternative - update the roster row instead:
1. Open: PLUGINS.md
2. Find: the O-IntonationPad row
3. Change from: 1-foundation
4. Change to: 2-dsp
5. Save file (edit ONLY that plugin's row -- the file is union-merged)
6. Run /reconcile to verify
```

**When this is best:** User wants full control, has specific knowledge about correct value, or learning the system.

### Option 2: Reset to Last Checkpoint

**When to offer:** Valid checkpoint exists for the plugin
**What it does:** Restores state files from checkpoint snapshot
**Risk level:** MEDIUM (loses changes since checkpoint)

**Prerequisites:**
- Checkpoint file exists at `.planning/workflow/checkpoints/{plugin}/latest.json`
- Checkpoint is newer than 24 hours (configurable threshold)
- Checkpoint passes schema validation

**Process:**
```
1. Verify checkpoint exists and is valid
2. Show checkpoint metadata:
   - Created: 2026-01-30T14:30:00Z
   - Stage: 2-dsp
   - Phase: execute
   - Last task: "Implement wavetable oscillator"
3. Warn about what will be lost:
   - Changes since checkpoint will be discarded
   - Files not in checkpoint will be orphaned
4. Confirm with user
5. Restore STATUS.md from checkpoint
6. Re-sync that plugin's PLUGINS.md row from the restored STATUS.md
7. Re-validate to confirm
```

**When this is best:** Corruption happened during current session, checkpoint represents known-good state.

### Option 3: Rebuild from Filesystem

**When to offer:** PLUGINS.md corruption, missing rows, orphan plugins
**What it does:** Scans plugins/ directory and rebuilds the PLUGINS.md roster
**Risk level:** MEDIUM (may lose metadata not in filesystem)

**Rebuild logic:**
```
1. Scan plugins/*/ directories
2. For each directory with .planning/:
   a. Read .planning/STATUS.md if exists
   b. Extract stage, phase, status from frontmatter
   c. If STATUS.md missing, use defaults:
      - stage: "0-ideation"
      - phase: "discuss"
      - status: "active"
   d. Create a PLUGINS.md row with:
      - plugin name
      - status from above
      - version from plugins/{name}/CHANGELOG.md top entry
      - updated: CHANGELOG date or today
3. Set focused: true in exactly one plugin's STATUS.md:
   a. First active plugin (if any)
   b. none (if no active plugins)
4. Write the rebuilt PLUGINS.md table
5. Re-derive module usage with scripts/regen-registry-used-by.sh
6. Re-validate to confirm
```

**What is preserved:**
- Stage, phase, status (from STATUS.md)
- Plugin directory structure
- All source files

**What may be lost:**
- Module dependency lists (regenerate with scripts/regen-registry-used-by.sh)
- Express mode settings
- Blocked by descriptions
- Original created dates (approximated from filesystem)

**When this is best:** Registry is corrupted beyond repair, want fresh start with current plugins.

## Source of Truth Rules

When files disagree, determine source of truth by field:

| Field | Source of Truth | Rationale |
|-------|-----------------|-----------|
| `stage` | STATUS.md | Updated during execution, closer to actual work |
| `phase` | STATUS.md | Updated during phase transitions |
| `status` | STATUS.md | User may edit directly |
| `path` | Filesystem | Directory location is canonical |
| `created` | Registry | Original creation date, not derivable |
| `modules` | Registry | Updated via /module commands only |
| `expressMode` | Registry | Configuration setting |
| `blockedBy` | STATUS.md | User documents blockers |
| `focused` | Registry | Authoritative for focus state |
| `plugin` (active) | Registry.focused | Derived from registry |

**Application:** When stage/phase/status disagree, trust STATUS.md. When registry-only fields are corrupted, user must manually restore or accept defaults.

## Recovery Execution Protocol

1. **Present validation findings**
   - Load from state-validation skill output
   - Show issue summary

2. **List applicable recovery options**
   - Always offer Option 1 (Manual)
   - Offer Option 2 if checkpoint exists
   - Offer Option 3 if registry corruption

3. **Explain risk/benefit of each**
   - Risk level (Low/Medium/High)
   - What will be changed
   - What might be lost

4. **Wait for user choice**
   - Never auto-proceed
   - Accept number (1/2/3) or description

5. **Execute chosen recovery**
   - Follow option-specific process
   - Show each change as it happens

6. **Re-run validation**
   - Invoke state-validation after repair
   - Confirm all issues resolved

7. **Report final state**
   - Summary of changes made
   - Current state (healthy/still issues)

## User Prompts

### Inconsistency Found

```
STATE INCONSISTENCY DETECTED

Registry and STATUS.md disagree on plugin stage.
  Plugin: O-IntonationPad
  Registry: 1-foundation
  STATUS.md: 2-dsp

Recovery options:
1. Manual repair (safest) - I'll show you exactly what to edit
2. Sync from STATUS.md (recommended) - Update registry to match STATUS.md
3. Sync from Registry - Update STATUS.md to match registry
4. Skip - Continue with current state (not recommended)

Which option? [1/2/3/4]
```

### Corruption Found

```
STATE CORRUPTION DETECTED

PLUGINS.md table has invalid structure.
  File: PLUGINS.md
  Error: Duplicate row for O-IntonationPad at line 234 (union-merge artifact)

Recovery options:
1. Manual repair - I'll guide you through fixing the table
2. Reset to checkpoint - Restore from 2026-01-30T14:30:00Z (loses recent changes)
3. Rebuild from filesystem - Reconstruct the roster from plugins/ directory

Which option? [1/2/3]
```

### Missing Roster Row

```
ORPHAN PLUGIN DETECTED

Plugin directory exists but has no PLUGINS.md row.
  Directory: plugins/O-NewPlugin
  STATUS.md: exists (stage: 1-foundation, phase: execute)

Recovery options:
1. Add to registry - Create registry entry from STATUS.md
2. Mark as archived - Add entry with status: archived
3. Ignore - Leave as orphan (not recommended)

Which option? [1/2/3]
```

### Multiple Issues

```
MULTIPLE STATE ISSUES DETECTED

Found 3 issues:

1. [ERROR] stage_consistency: PLUGINS.md/STATUS.md mismatch for O-IntonationPad
2. [ERROR] focus_consistency: two plugins both carry focused: true
3. [WARNING] orphan_plugin: plugins/O-TestPlugin has no PLUGINS.md row

Recovery strategy:
1. Fix all issues automatically (sync from STATUS.md, add orphans)
2. Fix one at a time - I'll walk you through each
3. Manual repair - I'll show commands for each fix
4. Skip - Continue with current state

Which option? [1/2/3/4]
```

## Post-Recovery Validation

After any recovery action, ALWAYS re-run validation:

```
RECOVERY COMPLETE

Changes made:
- Updated STATUS.md: O-IntonationPad stage = "2-dsp"

Re-validating state...

STATE VALIDATION REPORT
=======================
Status: healthy
Timestamp: 2026-01-30T15:05:00Z

Checks Run: 12
Passed: 12
Failed: 0

State validated successfully. All issues resolved.
```

If issues remain:

```
RECOVERY PARTIAL

Changes made:
- Updated STATUS.md: O-IntonationPad stage = "2-dsp"

Re-validating state...

STATE VALIDATION REPORT
=======================
Status: inconsistent
Timestamp: 2026-01-30T15:05:00Z

Checks Run: 12
Passed: 11
Failed: 1

REMAINING ISSUES:
1. [ERROR] focus_consistency: ...

Would you like to continue recovery? [y/n]
```

## Related Skills

- **state-validation** (@.claude/skills/state-validation/SKILL.md) - Detects issues that trigger recovery
- **workflow-reconciliation** (@.claude/skills/workflow-reconciliation/SKILL.md) - Broader workflow state sync
