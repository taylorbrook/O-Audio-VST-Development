# Branching Strategy

**Context:** This file is part of the plugin-workflow skill.
**Referenced by:** SKILL.md Reference Files section
**Purpose:** Configurable branching modes for plugin development workflow (AGNT-06)

---

## Overview

The branching strategy controls how git branches are managed during plugin development. Three modes are available, with optional squash merge for each.

## Modes

### 1. `none` (default)

All work happens on the current branch. No branch creation or merging.

**When to use:**
- Single-phase work
- Quick fixes and improvements
- Solo development without parallel workstreams
- Most plugin implementations (recommended default)

**Example workflow:**
```
main: A -- B -- C -- D -- E (all work here)
```

### 2. `phase`

Creates a branch per phase. Each branch is named `phase/{phase-name}` and merged to main after phase verification.

**When to use:**
- Multi-phase milestones with potential rollback needs
- Parallel phase work (multiple developers)
- When phases need independent review before merge

**Example workflow:**
```
main:                    A -------- M1 -------- M2
                          \       /   \        /
phase/14-migration:        B -- C      \      /
                                        \    /
phase/15-context:                        D -- E
```

**Branch naming:** `phase/{phase-number}-{phase-slug}`
- Example: `phase/17-agent-intelligence`

### 3. `milestone`

Creates a branch per milestone. Each branch is named `milestone/{version}` and merged to main after milestone completion.

**When to use:**
- Large refactors spanning many phases
- Experimental work that may be discarded
- When the entire milestone needs review before merge

**Example workflow:**
```
main:               A ----------------------- M
                     \                       /
milestone/v1.3:       B -- C -- D -- E -- F
```

**Branch naming:** `milestone/{version}`
- Example: `milestone/v1.3`

## Squash Merge

When squash merge is enabled, all commits on the feature branch are squashed into a single merge commit on the target branch.

**Squash merge benefits:**
- Clean main branch history
- Single revert point per phase/milestone
- Easier to read git log

**Squash merge trade-offs:**
- Loses individual commit granularity on main
- Individual task commits preserved only on feature branch (until deleted)

**Configuration:** Squash merge is configurable per mode and defaults to off.

## Configuration

Branching configuration is stored as a convention document. If runtime configuration is needed, it can be placed at `.planning/workflow/config/branching.json`.

**Convention format:**
```json
{
  "mode": "none",
  "squash": false
}
```

**Valid values:**
- `mode`: `"none"` | `"phase"` | `"milestone"`
- `squash`: `true` | `false`

**Default:** `{ "mode": "none", "squash": false }`

## Branch Naming Convention

| Mode | Pattern | Example |
|------|---------|---------|
| none | N/A | N/A |
| phase | `phase/{phase-number}-{phase-slug}` | `phase/17-agent-intelligence` |
| milestone | `milestone/{version}` | `milestone/v1.3` |

## Branch Lifecycle

### Phase mode

1. **Create:** At phase start, create `phase/{phase-name}` from main
2. **Work:** All phase plans execute on the phase branch
3. **Verify:** After phase verification passes, merge to main
4. **Clean up:** Delete phase branch after merge

### Milestone mode

1. **Create:** At milestone start, create `milestone/{version}` from main
2. **Work:** All milestone phases execute on the milestone branch
3. **Verify:** After milestone audit passes, merge to main
4. **Clean up:** Delete milestone branch after merge

## Integration with GSD Tools

The GSD tooling (`gsd-tools.js`) reads branching configuration from the init response:
- `branching_strategy`: Current mode
- `phase_branch_template`: Template for phase branch names
- `milestone_branch_template`: Template for milestone branch names

The execute-phase workflow checks the branching strategy and creates/merges branches automatically when in `phase` or `milestone` mode.

## When to Change Modes

| Situation | Recommended Mode |
|-----------|-----------------|
| Normal development | `none` |
| Starting a risky refactor | `milestone` |
| Team review needed per phase | `phase` |
| Experimental feature spike | `milestone` with squash |
| Quick bug fix | `none` |
| CI/CD integration | `phase` (for PR-per-phase) |
