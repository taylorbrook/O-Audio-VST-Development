---
name: reconcile
description: Validate Plugin Freedom System state and repair inconsistencies
arguments: []
---

# /reconcile

Run state validation and offer recovery options if issues found.

## When to Use

- After manual file edits to STATUS.md or PLUGINS.md
- When session state seems wrong
- After git merge that touched .planning/ files
- Debugging "plugin not found" or stale state issues
- Periodically to ensure state health

## Process

### 1. Load validation skill

@.claude/skills/state-validation/SKILL.md

### 2. Run all validation checks

Execute the check sequence from state-validation:

1. **Roster validation**
   - `PLUGINS.md` exists and its table parses
   - Every row's `plugins/{name}/` directory exists on disk

2. **Focus consistency**
   - At most one plugin carries `focused: true` in its STATUS.md frontmatter

3. **Per-plugin validation**
   For each plugin row in `PLUGINS.md`:
   - Plugin directory exists
   - `plugins/{name}/.planning/STATUS.md` exists with valid YAML frontmatter
   - The PLUGINS.md status column matches STATUS.md frontmatter `status`

4. **Orphan detection** (warnings)
   - Scan `plugins/*/` directories
   - Flag any with `.planning/` but no `PLUGINS.md` row

### 3. Report findings

Display STATE VALIDATION REPORT:

```
STATE VALIDATION REPORT
=======================
Status: [healthy|inconsistent|corrupted]
Timestamp: [ISO 8601]

Checks Run: N
Passed: M
Failed: K

[If failed checks:]
ISSUES FOUND:
-------------
1. [check_name]: [description]
   Location: [file:field]
   Expected: [value]
   Actual: [value]

2. ...

RECOMMENDED ACTION:
[Based on severity]
```

### 4. If issues found

Load recovery skill: @.claude/skills/state-recovery/SKILL.md

Present recovery options based on issue type:

**For inconsistencies:**
1. Manual repair (safest)
2. Sync from STATUS.md (recommended)
3. Sync from PLUGINS.md
4. Skip

**For corruption:**
1. Manual repair
2. Reset to checkpoint (if available)
3. Rebuild from filesystem

Execute user's chosen recovery, then re-validate to confirm fix.

### 5. If healthy

```
State validated successfully. No issues found.
```

## Example Output

**Healthy state:**
```
STATE VALIDATION REPORT
=======================
Status: healthy
Timestamp: 2026-01-30T15:00:00Z

Checks Run: 12
Passed: 12
Failed: 0

State validated successfully. No issues found.
```

**Inconsistent state:**
```
STATE VALIDATION REPORT
=======================
Status: inconsistent
Timestamp: 2026-01-30T15:00:00Z

Checks Run: 12
Passed: 11
Failed: 1

ISSUES FOUND:
-------------
1. status_consistency: PLUGINS.md and STATUS.md disagree on status
   Location: PLUGINS.md row O-IntonationPad
   PLUGINS.md: 1-foundation
   STATUS.md: 2-dsp

RECOMMENDED ACTION:
Load recovery skill and synchronize. STATUS.md is typically more current.

---

STATE INCONSISTENCY DETECTED

PLUGINS.md and STATUS.md disagree on plugin state.
  Plugin: O-IntonationPad
  PLUGINS.md: 1-foundation
  STATUS.md: 2-dsp

Recovery options:
1. Manual repair (safest) - I'll show you exactly what to edit
2. Sync from STATUS.md (recommended) - Update the PLUGINS.md row to match STATUS.md
3. Sync from PLUGINS.md - Update STATUS.md to match the PLUGINS.md row
4. Skip - Continue with current state (not recommended)

Which option? [1/2/3/4]
```

## Related Commands

- `/continue` - Runs validation automatically on session resume
- `/focus` - Runs validation automatically when switching plugins
- `/plugin-status` - Shows detailed plugin phase breakdown

## Related Skills

- @.claude/skills/state-validation/SKILL.md - Validation check logic
- @.claude/skills/state-recovery/SKILL.md - Recovery procedures
