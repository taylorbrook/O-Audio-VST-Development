# State Validation Skill

## Purpose

Detect inconsistencies in Plugin Freedom System state files. Runs automatically on `/continue`, `/focus`, and manually via `/reconcile`.

This skill implements **level-based reconciliation**: check ALL state on every invocation (not just what changed). This ensures idempotent, reliable validation that catches drift regardless of cause.

## Validation Protocol

### 1. Schema Validation

For each state file, validate against its schema:

| File | Schema | Location |
|------|--------|----------|
| `.planning/workflow/registry.json` | registry.schema.json | `.planning/workflow/schemas/` |
| `.planning/workflow/active-plugin.json` | active-plugin.schema.json | `.planning/workflow/schemas/` |
| `plugins/{name}/.planning/STATUS.md` | YAML frontmatter conventions | N/A (documented below) |

**Registry schema checks:**
- `$schema` field present and matches `./schemas/registry.schema.json`
- `version` matches semver pattern `^\d+\.\d+\.\d+$`
- `focused` is string or null
- `plugins` is object with valid PluginEntry values
- Each plugin entry has: path, stage, phase, status, created (required)

**Active plugin schema checks:**
- `$schema` field present
- `plugin` is string or null
- `focusedAt` is ISO 8601 datetime or null
- `loadedContext` object (if present) has valid structure

**STATUS.md frontmatter conventions:**
```yaml
---
stage: 2-dsp           # Must match registry enum
phase: execute         # Must match registry enum
status: active         # Must match registry enum
last-updated: YYYY-MM-DDTHH:MM:SSZ  # ISO 8601
---
```

### 2. Cross-File Consistency Checks

Compare fields that must match across files:

| Source | Field | Must Match | Severity |
|--------|-------|------------|----------|
| `registry.json` | `plugins.{name}.stage` | `STATUS.md` frontmatter `stage` | ERROR |
| `registry.json` | `plugins.{name}.phase` | `STATUS.md` frontmatter `phase` | ERROR |
| `registry.json` | `plugins.{name}.status` | `STATUS.md` frontmatter `status` | ERROR |
| `registry.json` | `focused` | `active-plugin.json` `plugin` | ERROR |
| `registry.json` | `plugins.{name}` exists | `plugins/{name}/` directory exists | ERROR |

### 3. Existence Checks

| Check | Severity | Description |
|-------|----------|-------------|
| Registry plugin has STATUS.md | ERROR | Every plugin in registry must have `plugins/{name}/.planning/STATUS.md` |
| Plugin directory in registry | WARNING | Every `plugins/*/` directory with `.planning/` should have registry entry |
| Focused plugin exists | ERROR | If registry.focused is set, that plugin must exist in registry.plugins |
| Active plugin matches focus | ERROR | active-plugin.json.plugin must match registry.focused |

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
  location: string;      // e.g., "registry.json:plugins.O-IntonationPad.stage"
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
1. REGISTRY FILE
   1.1 File exists at .planning/workflow/registry.json
   1.2 File is valid JSON (parseable)
   1.3 File matches registry.schema.json structure
   1.4 Version field is valid semver

2. ACTIVE PLUGIN FILE
   2.1 File exists at .planning/workflow/active-plugin.json
   2.2 File is valid JSON (parseable)
   2.3 File matches active-plugin.schema.json structure

3. FOCUS CONSISTENCY
   3.1 registry.focused === active-plugin.plugin
   3.2 If focused is set, plugin exists in registry.plugins

4. FOR EACH PLUGIN IN REGISTRY
   4.1 Plugin directory exists at {path}
   4.2 STATUS.md exists at {path}/.planning/STATUS.md
   4.3 STATUS.md frontmatter is valid YAML
   4.4 stage matches: registry vs STATUS.md
   4.5 phase matches: registry vs STATUS.md
   4.6 status matches: registry vs STATUS.md

5. ORPHAN DETECTION (warnings only)
   5.1 Scan plugins/*/ directories
   5.2 For each with .planning/, check registry entry exists
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
1. [ERROR] stage_consistency: Registry and STATUS.md disagree on stage
   Location: plugins.O-IntonationPad.stage
   Registry value: 1-foundation
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
1. [ERROR] registry_schema: Registry file has invalid structure
   Location: .planning/workflow/registry.json
   Error: Missing required field 'version'
   Recoverable: NO (manual intervention or rebuild required)

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
# Registry
cat .planning/workflow/registry.json | jq -e '.' || echo "Invalid JSON"

# Active plugin
cat .planning/workflow/active-plugin.json | jq -e '.' || echo "Invalid JSON"

# STATUS.md frontmatter (extract YAML between --- markers)
sed -n '/^---$/,/^---$/p' plugins/{name}/.planning/STATUS.md | head -n -1 | tail -n +2
```

**Comparing values:**
- Use exact string match for stage, phase, status
- Normalize stage names (e.g., "2-dsp" and "2" should match)
- Handle null vs missing field (treat as equivalent)

**Error classification:**
- JSON parse error -> corrupted
- Missing required field -> corrupted
- Field value mismatch -> inconsistent
- Missing STATUS.md -> corrupted
- Missing plugin directory -> corrupted
- Orphan plugin (no registry entry) -> warning only

## Related Skills

- **state-recovery** (@.claude/skills/state-recovery/SKILL.md) - Repair procedures for detected issues
- **workflow-reconciliation** (@.claude/skills/workflow-reconciliation/SKILL.md) - Broader workflow state sync
