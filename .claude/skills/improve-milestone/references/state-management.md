# State Management

This document defines the state tracking protocol for improve-milestone.

---

## State Locations

### Primary: STATUS.yaml

Location: `plugins/[Name]/.planning/improvements/[milestone-slug]/STATUS.yaml`

This is the authoritative state for the milestone.

```yaml
# STATUS.yaml schema
milestone: add-chorus-effect          # Slug from description
created: 2026-02-02T10:00:00Z         # When milestone started
lastActivity: 2026-02-02T14:30:00Z    # Most recent update

phases:
  discuss:
    status: complete                   # pending | in_progress | complete | skipped
    startedAt: 2026-02-02T10:00:00Z
    completedAt: 2026-02-02T10:15:00Z
    skippedReason: null               # If skipped, why

  research:
    status: complete
    startedAt: 2026-02-02T10:20:00Z
    completedAt: 2026-02-02T11:00:00Z
    skippedReason: null

  plan:
    status: in_progress
    startedAt: 2026-02-02T11:05:00Z
    completedAt: null
    approvedAt: null                  # Plan approval timestamp
    domain: dsp                       # Detected domain
    executeAgent: dsp-agent           # Agent for execute phase

  execute:
    status: pending
    startedAt: null
    completedAt: null
    backupPath: null                  # Set when backup created
    tasksCompleted: 0
    totalTasks: 0

  verify:
    status: pending
    startedAt: null
    completedAt: null
    result: null                      # passed | passed_with_notes | failed

versionBump: minor                    # patch | minor | major
baseVersion: 1.1.1                    # Version before improvement
targetVersion: 1.2.0                  # Version after improvement

error: null                           # Error info if failed
errorPhase: null                      # Which phase failed
```

### Secondary: registry.json

Location: `.planning/workflow/registry.json`

Quick lookup field for active milestones:

```json
{
  "plugins": {
    "O-Bells": {
      "path": "plugins/O-Bells",
      "stage": "4-polish",
      "status": "complete",
      "activeMilestone": "add-chorus-effect"  // <-- Added field
    }
  }
}
```

---

## State Updates by Phase

### On Milestone Start

1. Create directory: `plugins/[Name]/.planning/improvements/[slug]/`
2. Initialize STATUS.yaml:
   ```yaml
   milestone: [slug]
   created: [now]
   lastActivity: [now]
   phases:
     discuss: { status: pending }
     research: { status: pending }
     plan: { status: pending }
     execute: { status: pending }
     verify: { status: pending }
   versionBump: null
   baseVersion: null
   targetVersion: null
   ```
3. Update registry.json:
   ```json
   "O-Bells": {
     ...existing fields...,
     "activeMilestone": "[slug]"
   }
   ```

### On Phase Start

Update STATUS.yaml:
```yaml
phases:
  [phase]:
    status: in_progress
    startedAt: [now]
lastActivity: [now]
```

### On Phase Complete

Update STATUS.yaml:
```yaml
phases:
  [phase]:
    status: complete
    completedAt: [now]
lastActivity: [now]
```

### On Phase Skip

Update STATUS.yaml:
```yaml
phases:
  [phase]:
    status: skipped
    skippedReason: "User provided --skip-[phase] flag"
lastActivity: [now]
```

### On Plan Approval

Update STATUS.yaml:
```yaml
phases:
  plan:
    status: complete
    completedAt: [now]
    approvedAt: [now]
    domain: [detected]
    executeAgent: [agent]
versionBump: [type]
baseVersion: [current]
targetVersion: [calculated]
lastActivity: [now]
```

### On Backup Creation (Execute Start)

Update STATUS.yaml:
```yaml
phases:
  execute:
    status: in_progress
    startedAt: [now]
    backupPath: backups/[PluginName]/v[baseVersion]/
lastActivity: [now]
```

### On Task Completion (Execute Phase)

Update STATUS.yaml:
```yaml
phases:
  execute:
    tasksCompleted: [N]
    totalTasks: [M]
lastActivity: [now]
```

### On Milestone Complete

1. Update STATUS.yaml:
   ```yaml
   phases:
     verify:
       status: complete
       completedAt: [now]
       result: passed  # or passed_with_notes
   lastActivity: [now]
   ```

2. Update registry.json (remove activeMilestone):
   ```json
   "O-Bells": {
     ...existing fields...,
     "activeMilestone": null
   }
   ```

### On Error

Update STATUS.yaml:
```yaml
error: "Build failed with exit code 1"
errorPhase: execute
phases:
  [errorPhase]:
    status: in_progress  # Stays in_progress, not failed
lastActivity: [now]
```

---

## Reading State

### Check for Active Milestone

```javascript
// Quick check via registry
const registry = JSON.parse(fs.readFileSync('.planning/workflow/registry.json'));
const plugin = registry.plugins[pluginName];

if (plugin.activeMilestone) {
  // Load full state
  const statusPath = `plugins/${pluginName}/.planning/improvements/${plugin.activeMilestone}/STATUS.yaml`;
  const status = yaml.parse(fs.readFileSync(statusPath));

  // Find current phase
  const currentPhase = Object.entries(status.phases)
    .find(([name, phase]) => phase.status === 'in_progress')?.[0]
    || Object.entries(status.phases)
        .find(([name, phase]) => phase.status === 'pending')?.[0];
}
```

### Determine Next Phase

```javascript
function getNextPhase(status) {
  const phaseOrder = ['discuss', 'research', 'plan', 'execute', 'verify'];

  for (const phase of phaseOrder) {
    if (status.phases[phase].status === 'pending') {
      return phase;
    }
    if (status.phases[phase].status === 'in_progress') {
      return phase; // Resume current
    }
  }

  return null; // All complete
}
```

---

## State Validation

### On Resume

Before resuming a milestone, validate state consistency:

1. **Directory exists:** `plugins/[Name]/.planning/improvements/[slug]/`
2. **STATUS.yaml valid:** All required fields present
3. **Registry matches:** `activeMilestone` matches slug
4. **Phase outputs exist:** Completed phases have output files
5. **No orphaned state:** If execute started, backup exists

### Repair Procedures

**Missing output file:**
- Mark phase as pending
- Re-run phase

**Registry mismatch:**
- Trust STATUS.yaml as authoritative
- Update registry to match

**Missing backup (execute in progress):**
- HALT
- Create backup before continuing
