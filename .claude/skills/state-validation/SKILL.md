# State Validation Skill

## Purpose

Detect inconsistencies in Plugin Freedom System state files. Runs automatically on `/continue`, `/focus`, and manually via `/reconcile`.

This skill implements **level-based reconciliation**: check ALL state on every invocation (not just what changed). This ensures idempotent, reliable validation that catches drift regardless of cause.

## Validation Protocol

### 1. Structural Validation

The central JSON state files are **retired**. STATUS.md is authoritative; `PLUGINS.md`
is the roster. Validate these:

| File | Validated against | Location |
|------|-------------------|----------|
| `PLUGINS.md` | Markdown table shape (one row per plugin) | repo root |
| `plugins/{name}/.planning/STATUS.md` | YAML frontmatter conventions | per plugin |
| `modules/registry.yaml` | YAML parses; `used_by` entries name real plugins | repo root |

**PLUGINS.md structural checks:**
- File exists and its plugin table parses
- Every row's Plugin cell names a directory under `plugins/`
- No duplicate rows for the same plugin (a duplicate is the classic union-merge artifact)

**STATUS.md frontmatter conventions:**
```yaml
---
stage: 2-dsp           # Must be a known stage
phase: execute         # Must be a known phase
status: active         # Must be a known status
focused: true          # At most one plugin repo-wide
last-updated: YYYY-MM-DDTHH:MM:SSZ  # ISO 8601
---
```

### 2. Cross-File Consistency Checks

Compare fields that must match across files:

| Source | Field | Must Match | Severity |
|--------|-------|------------|----------|
| `PLUGINS.md` | row status cell | `STATUS.md` frontmatter `status` | ERROR |
| `PLUGINS.md` | row version cell | `plugins/{name}/CHANGELOG.md` top version | ERROR |
| `PLUGINS.md` | row exists | `plugins/{name}/` directory exists | ERROR |
| `STATUS.md` | `focused: true` | at most one across all plugins | ERROR |
| `modules/registry.yaml` | `used_by[].plugin` | `plugins/{name}/` directory exists | ERROR |

### 3. Existence Checks

| Check | Severity | Description |
|-------|----------|-------------|
| Roster plugin has STATUS.md | ERROR | Every `PLUGINS.md` row must have `plugins/{name}/.planning/STATUS.md` |
| Plugin directory in roster | WARNING | Every `plugins/*/` directory with `.planning/` should have a `PLUGINS.md` row |
| Focused plugin exists | ERROR | If a STATUS.md sets `focused: true`, that plugin must have a `PLUGINS.md` row |
| Single focus | ERROR | No more than one plugin may carry `focused: true` |

## Result Format

```typescript
interface ReconciliationResult {
  status: 'healthy' | 'inconsistent' | 'corrupted';
  timestamp: string;  // ISO 8601
  checksRun: number;
  checksPassed: number;
  checksFailed: number;
  issues: Issue[];
  recommendedAction: string;
}

interface Issue {
  checkName: string;
  severity: 'error' | 'warning';
  description: string;
  location: string;      // e.g., "plugins/O-IntonationPad/.planning/STATUS.md:stage"
  expected: unknown;
  actual: unknown;
  recoverable: boolean;
}
```

**Status determination:**
- `healthy`: All checks pass (checksFailed === 0)
- `inconsistent`: Cross-file mismatches exist (recoverable with sync)
- `corrupted`: Schema violations or missing required files (needs repair)

## Check Execution Order

Execute checks in this order (fail-fast on corruption):

```
1. ROSTER FILE
   1.1 File exists at PLUGINS.md
   1.2 The plugin table parses
   1.3 No duplicate rows for the same plugin

2. MODULE REGISTRY
   2.1 File exists at modules/registry.yaml
   2.2 File is valid YAML (parseable)
   2.3 Header version field is valid semver

3. FOCUS CONSISTENCY
   3.1 At most one STATUS.md carries focused: true
   3.2 If focus is set, that plugin has a PLUGINS.md row

4. FOR EACH PLUGIN ROW IN PLUGINS.md
   4.1 Plugin directory exists at plugins/{name}/
   4.2 STATUS.md exists at plugins/{name}/.planning/STATUS.md
   4.3 STATUS.md frontmatter is valid YAML
   4.4 status matches: PLUGINS.md row vs STATUS.md
   4.5 version matches: PLUGINS.md row vs CHANGELOG.md top entry

5. ORPHAN DETECTION (warnings only)
   5.1 Scan plugins/*/ directories
   5.2 For each with .planning/, check a PLUGINS.md row exists
   5.3 Warn if orphan found (not blocking)
```

## Output Format

When validation runs:

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

**Inconsistent state (recoverable):**
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
1. [ERROR] status_consistency: PLUGINS.md and STATUS.md disagree
   Location: PLUGINS.md row O-IntonationPad
   PLUGINS.md value: 1-foundation
   STATUS.md value: 2-dsp
   Recoverable: YES

RECOMMENDED ACTION:
Load state-recovery skill and synchronize from STATUS.md (typically more current).
See: @.claude/skills/state-recovery/SKILL.md
```

**Corrupted state (needs repair):**
```
STATE VALIDATION REPORT
=======================
Status: corrupted
Timestamp: 2026-01-30T15:00:00Z

Checks Run: 5
Passed: 4
Failed: 1

ISSUES FOUND:
-------------
1. [ERROR] roster_structure: PLUGINS.md table has invalid structure
   Location: PLUGINS.md
   Error: Duplicate row for O-IntonationPad (union-merge artifact)
   Recoverable: NO (manual intervention required)

RECOMMENDED ACTION:
Load state-recovery skill for repair options.
See: @.claude/skills/state-recovery/SKILL.md
```

## When to Run Validation

| Trigger | Behavior |
|---------|----------|
| `/continue` (session resume) | Run full validation before loading plugin context |
| `/focus {plugin}` | Run validation before switching focus |
| `/reconcile` | Manual trigger - run full validation, offer recovery |
| After git merge | Recommended - state files may have conflicts |
| After manual edits | Recommended - catch typos in YAML/JSON |

## Implementation Guidance

**Reading files for validation:**
```bash
# Roster rows
grep -E '^\| O-' PLUGINS.md

# Duplicate-row check (union-merge artifact)
grep -E '^\| [A-Za-z0-9-]+ \|' PLUGINS.md | cut -d'|' -f2 | sort | uniq -d

# Module registry
python3 -c "import yaml,sys; yaml.safe_load(open('modules/registry.yaml'))" || echo "Invalid YAML"

# STATUS.md frontmatter (extract YAML between --- markers)
sed -n '/^---$/,/^---$/p' plugins/{name}/.planning/STATUS.md | head -n -1 | tail -n +2
```

**Comparing values:**
- Use exact string match for stage, phase, status
- Normalize stage names (e.g., "2-dsp" and "2" should match)
- Handle null vs missing field (treat as equivalent)

**Error classification:**
- YAML/table parse error -> corrupted
- Missing required field -> corrupted
- Field value mismatch -> inconsistent
- Missing STATUS.md -> corrupted
- Missing plugin directory -> corrupted
- Orphan plugin (no PLUGINS.md row) -> warning only

## Related Skills

- **state-recovery** (@.claude/skills/state-recovery/SKILL.md) - Repair procedures for detected issues
- **workflow-reconciliation** (@.claude/skills/workflow-reconciliation/SKILL.md) - Broader workflow state sync
