---
name: reconcile
description: Validate Plugin Freedom System state and repair inconsistencies
arguments: []
---

# /reconcile

Run state validation and offer recovery options if issues found.

## When to Use

- After manual file edits to STATUS.md or registry
- When session state seems wrong
- After git merge that touched .planning/ files
- Debugging "plugin not found" or stale state issues
- Periodically to ensure state health

## Process

### 1. Load validation skill

@.claude/skills/state-validation/SKILL.md

### 2. Run all validation checks

Execute the check sequence from state-validation:

1. **Registry validation**
   - File exists at `.planning/workflow/registry.json`
   - Valid JSON structure
   - Matches registry.schema.json

2. **Active plugin validation**
   - File exists at `.planning/workflow/active-plugin.json`
   - Valid JSON structure
   - Matches active-plugin.schema.json

3. **Focus consistency**
   - `registry.focused` === `active-plugin.plugin`
   - Focused plugin exists in registry

4. **Per-plugin validation**
   For each plugin in registry:
   - Plugin directory exists
   - STATUS.md exists with valid YAML frontmatter
   - stage/phase/status match between registry and STATUS.md

5. **Orphan detection** (warnings)
   - Scan `plugins/*/` directories
   - Flag any with `.planning/` but no registry entry

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
3. Sync from Registry
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
1. stage_consistency: Registry and STATUS.md disagree on stage
   Location: plugins.O-IntonationPad.stage
   Registry: 1-foundation
   STATUS.md: 2-dsp

RECOMMENDED ACTION:
Load recovery skill and synchronize. STATUS.md is typically more current.

---

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

## Related Commands

- `/continue` - Runs validation automatically on session resume
- `/focus` - Runs validation automatically when switching plugins
- `/plugin-status` - Shows detailed plugin phase breakdown

## Related Skills

- @.claude/skills/state-validation/SKILL.md - Validation check logic
- @.claude/skills/state-recovery/SKILL.md - Recovery procedures
