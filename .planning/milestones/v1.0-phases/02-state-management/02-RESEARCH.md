# Phase 2: State Management - Research

**Researched:** 2026-01-30
**Domain:** File-based workflow state persistence, session resume, corruption detection/recovery
**Confidence:** HIGH

## Summary

Phase 2 focuses on hardening the Plugin Freedom System's state persistence layer. The research investigated JSON schema validation libraries, atomic file write patterns, checkpoint/restore mechanisms, file locking for concurrent access, cross-file consistency detection, and bidirectional dependency tracking.

The current system has foundational state infrastructure: `plugin-registry.json` (global registry), `STATUS.md` (per-plugin state), and various PLAN/CONTEXT files. The phase goal is to create a robust `.planning/workflow/` state directory with automatic validation, corruption detection, and recovery options.

The recommended approach: (1) use Zod for schema validation (TypeScript-first, better DX than Ajv for this use case), (2) atomic file writes via write-file-atomic pattern for JSON state, (3) reconciliation loop pattern for cross-file consistency, (4) task-level checkpoints stored as JSON in `checkpoints/`, (5) proper-lockfile for concurrent access coordination, (6) bidirectional dependency tracking via forward refs in plugin files + reverse index in central registry.

**Primary recommendation:** Implement a level-based reconciliation pattern where validation runs on `/continue` and `/focus`, compares desired state (from contracts) against actual state (filesystem), and presents user with repair options when inconsistencies are detected.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Zod | 4.x | Schema validation | TypeScript-first, 2kb core, excellent error messages, JSON Schema export |
| write-file-atomic | 6.x | Atomic JSON writes | npm-maintained, handles temp files + rename atomically |
| proper-lockfile | 4.x | File locking | Works across processes, handles stale locks, network FS safe |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Ajv | 8.x | JSON Schema validation | If need to validate against existing JSON Schema files |
| steno | 0.4.x | Fast async file writes | High-frequency state updates (alternative to write-file-atomic) |
| chokidar | 3.x | File watching | If need real-time state change detection |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Zod | Ajv | Ajv faster for high-volume, but worse TypeScript DX; Zod better for schema definition |
| write-file-atomic | Manual rename | More code, edge cases (Windows, network FS) |
| proper-lockfile | No locking | Simpler but risks corruption with parallel instances |
| JSON state files | SQLite | Overkill for this use case; JSON is Git-friendly |

**No npm install needed:** Validation can be Claude-enforced (conceptually validate against schemas). The libraries are reference patterns for when machine validation is added.

## Architecture Patterns

### Recommended State File Structure
```
.planning/
├── STATE.md              # GSD project state (existing)
├── ROADMAP.md            # GSD roadmap (existing)
├── PROJECT.md            # GSD project info (existing)
├── phases/               # GSD phase work (existing)
├── config.json           # GSD configuration (existing)
└── workflow/             # NEW: Plugin Freedom System runtime state
    ├── registry.json         # Plugin registry (moved from .claude/)
    ├── active-plugin.json    # Current focus state
    ├── module-deps.json      # Reverse dependency index (module -> plugins)
    ├── schemas/              # State validation schemas
    │   ├── registry.schema.json
    │   ├── checkpoint.schema.json
    │   └── active-plugin.schema.json
    └── checkpoints/          # Resume points
        ├── {plugin}/
        │   ├── {timestamp}-{task}.json
        │   └── latest.json   # Symlink to most recent
        └── .index.json       # Checkpoint index for fast lookup

plugins/{Name}/.planning/
├── STATUS.md             # Plugin state (existing pattern)
├── dependencies.json     # Forward dependency refs (plugin -> modules)
├── BRIEF.md             # Ideation output (existing)
├── ARCHITECTURE.md       # Planning output (existing)
└── stages/
    └── {stage}/
        ├── CONTEXT.md
        ├── RESEARCH.md
        ├── PLAN.md
        └── checkpoints/   # Stage-scoped checkpoints
```

### Pattern 1: Level-Based Reconciliation Loop
**What:** On `/continue` or `/focus`, compare desired state (what the schema expects) against actual state (what files contain)
**When to use:** Every session resume and focus switch
**Example:**
```typescript
// Source: Kubernetes reconciliation pattern adapted for file state
interface ReconciliationResult {
  status: 'consistent' | 'inconsistent' | 'corrupted';
  discrepancies: Discrepancy[];
  suggestedActions: RecoveryAction[];
}

interface Discrepancy {
  type: 'missing_field' | 'value_mismatch' | 'cross_file_inconsistency' | 'schema_violation';
  location: string;      // e.g., "registry.json:plugins.O-IntonationPad.stage"
  expected: unknown;
  actual: unknown;
}

// Reconciliation runs as level-based (not edge-triggered)
// - Check ALL state, not just what changed
// - Idempotent: running twice gives same result
// - Safe to retry on failure
```

### Pattern 2: Atomic State Updates
**What:** All state file writes use temp file + rename pattern
**When to use:** Every write to registry.json, active-plugin.json, checkpoints
**Example:**
```typescript
// Source: npm/write-file-atomic pattern
async function writeStateAtomic(filePath: string, data: object): Promise<void> {
  const content = JSON.stringify(data, null, 2);
  const tempPath = `${filePath}.${process.pid}.tmp`;

  // 1. Write to temp file
  await fs.writeFile(tempPath, content, 'utf8');

  // 2. Atomic rename (safe even if crash mid-operation)
  await fs.rename(tempPath, filePath);
}
```

### Pattern 3: Checkpoint Granularity
**What:** Checkpoints created at task completion, not phase completion
**When to use:** After each significant task in PLAN.md completes
**Example:**
```json
// .planning/workflow/checkpoints/O-IntonationPad/2026-01-30T14-30-00-task-03.json
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
    "handoffContext": "WavetableOscillator class complete, ready for voice integration"
  },
  "filesModified": [
    "plugins/O-IntonationPad/Source/DSP/WavetableOscillator.h",
    "plugins/O-IntonationPad/Source/DSP/WavetableOscillator.cpp"
  ]
}
```

### Pattern 4: Bidirectional Dependency Tracking
**What:** Each plugin tracks its deps; central index maps deps back to plugins
**When to use:** Module changes that may affect multiple plugins
**Example:**
```json
// plugins/O-IntonationPad/.planning/dependencies.json
{
  "modules": [
    { "name": "scala-tuning-engine", "version": "1.13.0", "purpose": "JI/tuning calculations" }
  ],
  "ouariconModules": ["webview-bridge"],
  "juceModules": ["juce_dsp", "juce_audio_processors"]
}

// .planning/workflow/module-deps.json (reverse index)
{
  "scala-tuning-engine": ["O-IntonationPad", "O-MicrotonalSynth"],
  "webview-bridge": ["O-IntonationPad", "O-Tremolo", "O-Bass"],
  "juce_dsp": ["O-IntonationPad", "O-Tremolo", "O-Bass", "O-Compressor"]
}
```

### Pattern 5: Cross-File Consistency Detection
**What:** STATUS.md frontmatter must match registry.json entry
**When to use:** Validation on `/continue`, `/focus`, `/reconcile`
**Example:**
```typescript
// Source: Data reconciliation patterns
interface ConsistencyCheck {
  files: string[];
  fieldMappings: FieldMapping[];
}

const REGISTRY_STATUS_CONSISTENCY: ConsistencyCheck = {
  files: ['registry.json', 'STATUS.md'],
  fieldMappings: [
    { registry: 'plugins.{name}.stage', status: 'frontmatter.stage' },
    { registry: 'plugins.{name}.phase', status: 'frontmatter.phase' },
    { registry: 'plugins.{name}.status', status: 'frontmatter.status' }
  ]
};

// Detection: if values differ, flag as inconsistency
// Resolution: user chooses source of truth (registry or STATUS.md)
```

### Anti-Patterns to Avoid
- **Silent auto-repair:** Don't fix corruption without user acknowledgment; surprises erode trust
- **Staleness detection:** Don't treat "last modified > N days" as corruption; false positives
- **Global state in single file:** Don't put all plugin state in one file; harder to isolate failures
- **Edge-triggered validation:** Don't only validate on write; level-based catches drift
- **Tight coupling to file layout:** Use schemas to define contracts, not hardcoded paths

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Atomic file writes | Manual temp + rename | write-file-atomic pattern | Windows edge cases, error handling, cleanup |
| File locking | Custom lock files | proper-lockfile | Stale lock detection, cross-process, network FS |
| Schema validation | String matching | Zod schemas | Type inference, error messages, composability |
| JSON parsing with validation | JSON.parse + manual checks | Zod .parse() | Unified validation, type narrowing, detailed errors |
| Cross-file consistency | Ad-hoc comparisons | Reconciliation loop | Idempotent, systematic, actionable output |
| Checkpoint storage | Flat files | Index + individual checkpoints | Fast lookup, pruning, partial restore |

**Key insight:** State management seems simple until edge cases (concurrent access, partial writes, schema evolution, recovery from corruption). The patterns above handle these; custom solutions will rediscover them painfully.

## Common Pitfalls

### Pitfall 1: Optimistic Locking Without Validation
**What goes wrong:** Two Claude instances write to registry.json simultaneously; last write wins, losing data
**Why it happens:** Assuming single-user access; forgetting parallel instances are supported
**How to avoid:** Use proper-lockfile before any registry write; short lock duration
**Warning signs:** "Missing plugin" errors after parallel work sessions

### Pitfall 2: Checkpoint Bloat
**What goes wrong:** Checkpoints accumulate indefinitely; disk fills, performance degrades
**Why it happens:** Task-level granularity without pruning strategy
**How to avoid:** Index file tracks checkpoints; prune to keep N most recent per plugin + phase boundaries
**Warning signs:** Checkpoint directory > 100 files; slow `/continue` startup

### Pitfall 3: Schema Evolution Breaking Resume
**What goes wrong:** New schema version makes old checkpoints unreadable
**Why it happens:** Forgetting checkpoints are long-lived; breaking changes to schema
**How to avoid:** Versioned schemas; migration path documented; old schemas kept for reading
**Warning signs:** "Invalid checkpoint" errors after system update

### Pitfall 4: Frontmatter Parsing Fragility
**What goes wrong:** STATUS.md frontmatter becomes invalid YAML; entire file rejected
**Why it happens:** Manual edits, encoding issues, merge conflicts
**How to avoid:** Lenient parser with fallback; warn but don't fail; preserve raw frontmatter
**Warning signs:** Users reporting "corrupted" STATUS.md after Git operations

### Pitfall 5: Recovery Option Paralysis
**What goes wrong:** Corruption detected, user presented with 5 options, doesn't know which to pick
**Why it happens:** Over-designing recovery; not guiding user to best option
**How to avoid:** Rank options by safety; default to safest; explain tradeoffs concisely
**Warning signs:** Users asking "which option should I pick?"

### Pitfall 6: Cross-File Consistency Without Clear Source of Truth
**What goes wrong:** registry.json and STATUS.md disagree; reconciler doesn't know which is correct
**Why it happens:** Both files writable; no canonical "last writer"
**How to avoid:** Define source of truth per field; document in schema
**Warning signs:** Reconciliation flips values back and forth

## Code Examples

Verified patterns from official sources and established libraries:

### Zod Schema for Registry Entry
```typescript
// Source: https://zod.dev/ - Zod 4.x schema definition
import { z } from 'zod';

const PluginEntrySchema = z.object({
  path: z.string().min(1),
  stage: z.enum(['0-ideation', '1-foundation', '2-dsp', '3-gui', '4-polish', 'complete']),
  phase: z.enum(['discuss', 'research', 'plan', 'execute', 'verify']).nullable(),
  status: z.enum(['active', 'paused', 'blocked', 'released', 'archived']),
  created: z.string().date(),
  lastActivity: z.string().datetime().optional(),
  modules: z.array(z.string()).default([]),
  expressMode: z.boolean().default(false),
  blockedBy: z.string().nullable().optional()
});

const RegistrySchema = z.object({
  $schema: z.string().optional(),
  version: z.string(),
  focused: z.string().nullable(),
  plugins: z.record(z.string(), PluginEntrySchema)
});

// Validation with detailed errors
function validateRegistry(data: unknown): RegistrySchema | never {
  const result = RegistrySchema.safeParse(data);
  if (!result.success) {
    const formatted = result.error.format();
    throw new ValidationError('Registry schema violation', formatted);
  }
  return result.data;
}
```

### Checkpoint Schema
```typescript
// Source: Checkpoint/restore patterns adapted from LangGraph
const CheckpointSchema = z.object({
  $schema: z.string().optional(),
  version: z.literal('1.0.0'),
  plugin: z.string(),
  timestamp: z.string().datetime(),
  stage: z.string(),
  phase: z.enum(['discuss', 'research', 'plan', 'execute', 'verify']),
  task: z.object({
    plan: z.string(),
    taskNumber: z.number().int().positive(),
    taskTitle: z.string()
  }),
  state: z.object({
    completedTasks: z.array(z.number().int()),
    remainingTasks: z.array(z.number().int()),
    handoffContext: z.string()
  }),
  filesModified: z.array(z.string())
});
```

### Reconciliation Check
```typescript
// Source: Kubernetes reconciliation loop pattern
interface ReconciliationReport {
  timestamp: string;
  status: 'healthy' | 'inconsistent' | 'corrupted';
  checks: CheckResult[];
  recommendedAction: string;
}

interface CheckResult {
  check: string;
  passed: boolean;
  details?: string;
}

async function reconcile(pluginName: string): Promise<ReconciliationReport> {
  const checks: CheckResult[] = [];

  // Check 1: Registry entry exists
  const registry = await loadRegistry();
  const entry = registry.plugins[pluginName];
  checks.push({
    check: 'registry_entry_exists',
    passed: !!entry,
    details: entry ? undefined : `Plugin ${pluginName} not in registry`
  });

  // Check 2: STATUS.md exists and is valid
  const statusPath = `plugins/${pluginName}/.planning/STATUS.md`;
  const statusExists = await fileExists(statusPath);
  checks.push({
    check: 'status_file_exists',
    passed: statusExists
  });

  if (statusExists && entry) {
    // Check 3: Cross-file consistency
    const status = await parseStatusFrontmatter(statusPath);
    const stageMatch = normalizeStage(entry.stage) === normalizeStage(status.stage);
    checks.push({
      check: 'stage_consistency',
      passed: stageMatch,
      details: stageMatch ? undefined :
        `Registry: ${entry.stage}, STATUS.md: ${status.stage}`
    });
  }

  // Determine overall status
  const failedChecks = checks.filter(c => !c.passed);
  const status = failedChecks.length === 0 ? 'healthy' :
                 failedChecks.some(c => c.check.includes('schema')) ? 'corrupted' :
                 'inconsistent';

  return {
    timestamp: new Date().toISOString(),
    status,
    checks,
    recommendedAction: getRecommendedAction(status, failedChecks)
  };
}
```

### Active Plugin State
```typescript
// Source: Session state patterns
const ActivePluginSchema = z.object({
  $schema: z.string().optional(),
  plugin: z.string().nullable(),
  focusedAt: z.string().datetime().nullable(),
  sessionId: z.string().optional(),
  loadedContext: z.object({
    statusMd: z.boolean(),
    currentPlan: z.string().nullable(),
    currentContext: z.string().nullable()
  }).optional()
});

// .planning/workflow/active-plugin.json
// {
//   "plugin": "O-IntonationPad",
//   "focusedAt": "2026-01-30T14:00:00Z",
//   "loadedContext": {
//     "statusMd": true,
//     "currentPlan": "stages/2-dsp/02-01-PLAN.md",
//     "currentContext": "stages/2-dsp/02-CONTEXT.md"
//   }
// }
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Status in Markdown only | JSON + Markdown hybrid | 2025 | Machine-readable state, human-readable docs |
| Global state file | Per-plugin + central index | 2025 | Better isolation, easier recovery |
| Manual session notes | Structured checkpoints | 2025-2026 | Reliable resume, audit trail |
| Trust file writes | Atomic writes + locking | Long-standing | Corruption prevention |
| Implicit contracts | Schema validation | 2024-2026 | Catches errors early, actionable messages |

**Deprecated/outdated:**
- `PLUGINS.md` for registry: Being replaced by `registry.json` (machine-readable)
- Single global STATUS.md: Now per-plugin in `plugins/{Name}/.planning/STATUS.md`

## Open Questions

Things that couldn't be fully resolved:

1. **Checkpoint retention policy**
   - What we know: Task-level granularity creates many checkpoints
   - What's unclear: Exact retention (keep last N? keep phase boundaries? time-based?)
   - Recommendation: Start with keep-last-10-per-plugin + all phase boundaries; tune based on usage

2. **Lock timeout for parallel instances**
   - What we know: proper-lockfile supports configurable stale timeout
   - What's unclear: Optimal timeout for Claude sessions (which can be long)
   - Recommendation: 60-second stale timeout; refresh on write; warn if lock held

3. **Schema versioning strategy**
   - What we know: Schemas will evolve; checkpoints are long-lived
   - What's unclear: Whether to embed version in filename or schema $id
   - Recommendation: Use `version` field in schema; keep old schemas for reading; document migration

4. **Frontmatter parser choice**
   - What we know: STATUS.md uses YAML frontmatter; needs parsing
   - What's unclear: Whether to use gray-matter, js-yaml, or simple regex
   - Recommendation: Use established parser (gray-matter) but with lenient mode + error recovery

## Sources

### Primary (HIGH confidence)
- [Zod documentation](https://zod.dev/) - Schema validation API, JSON Schema export
- [write-file-atomic](https://github.com/npm/write-file-atomic) - Atomic file write pattern
- [proper-lockfile](https://github.com/moxystudio/node-proper-lockfile) - File locking for Node.js
- Existing `.claude/schemas/plugin-registry.schema.json` - Current project patterns
- Existing `/continue` and `/reconcile` commands - Current behavior specification

### Secondary (MEDIUM confidence)
- [Claude Code checkpointing](https://code.claude.com/docs/en/checkpointing) - Session checkpoint concepts
- [Kubernetes reconciliation loop](https://dev.to/adipolak/kubernetes-self-healing-reconciliation-loop-4aj5) - Level-based reconciliation pattern
- [Ajv TypeScript guide](https://ajv.js.org/guide/typescript.html) - JSON Schema validation with types
- [Data reconciliation patterns](https://airbyte.com/data-engineering-resources/data-reconciliation) - Consistency detection

### Tertiary (LOW confidence)
- WebSearch results on file locking patterns - General ecosystem patterns
- WebSearch results on checkpoint/restore - Background concepts

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Zod, write-file-atomic, proper-lockfile are well-documented, battle-tested
- Architecture patterns: HIGH - Based on existing project structure + Kubernetes-style reconciliation
- Pitfalls: MEDIUM - Derived from concurrent system patterns + state management experience
- Checkpoint format: MEDIUM - Synthesized from LangGraph/Claude Code patterns

**Research date:** 2026-01-30
**Valid until:** 2026-03-01 (60 days - state management patterns are stable)
