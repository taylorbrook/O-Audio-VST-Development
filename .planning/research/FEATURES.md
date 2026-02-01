# Feature Research: v1.1 Improvements

**Domain:** Repository hygiene tooling and workflow planning phases
**Researched:** 2026-02-01
**Overall confidence:** HIGH (verified against official docs and established codebase patterns)

---

## Context: v1.1 Milestone

This research focuses on two capability areas for v1.1:

1. **Repository Cleanup** - Remove build artifacts from git history (currently 584MB .git with 50MB+ binaries)
2. **Workflow Planning Phase** - Add structured planning between investigation and implementation for complex improvements

These are incremental enhancements to the existing Plugin Freedom System, not a fundamental overhaul.

---

## Repository Cleanup

### Table Stakes

Features required for a functional repository cleanup capability.

| Feature | Why Expected | Complexity | Dependencies |
|---------|--------------|------------|--------------|
| **Build artifact removal from history** | Primary use case - 584MB .git with 50MB+ binaries in history | Medium | git-filter-repo or BFG |
| **Pre-cleanup backup** | Cannot undo history rewrite without backup | Low | git clone --mirror |
| **Large file detection** | Must identify cleanup targets before removal | Low | git rev-list, cat-file |
| **Team coordination protocol** | History rewrite requires all collaborators to re-clone | Low | Documentation |
| **Post-cleanup garbage collection** | BFG/filter-repo don't physically delete data | Low | git gc --aggressive |
| **Verification report** | Confirm cleanup succeeded, compare before/after sizes | Low | du -sh .git |

**Notes:**
- git-filter-repo is now recommended over BFG (BFG is simpler but git-filter-repo has more features and better maintenance)
- The 40-55MB build artifacts in git history are primary cleanup targets
- Cleanup is destructive and non-reversible - backup is non-negotiable

### Differentiators

Nice-to-have features that improve usability.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Automated detection script** | Find large files without manual git commands | Low | One-time script |
| **Dry-run mode** | Preview what would be removed before committing | Medium | git-filter-repo supports this |
| **Pattern-based cleanup** | Remove by extension (*.a, *.o) not just specific files | Low | Both tools support globs |
| **.gitignore validation** | Ensure cleaned patterns won't recur | Low | Parse and verify |
| **Size threshold configuration** | Define what "large" means (default: 1MB) | Low | User preference |
| **Incremental cleanup** | Remove files added since last cleanup | Medium | Track cleanup history |

---

## Workflow Planning Phase

### Table Stakes

Features required for a planning phase between investigation and implementation.

| Feature | Why Expected | Complexity | Dependencies |
|---------|--------------|------------|--------------|
| **Implementation plan document** | Technical blueprint before coding (the "40-20-40 rule") | Medium | Template system |
| **Task decomposition** | Break improvement into sequential, testable steps | Medium | Existing PLAN.md pattern |
| **Architecture decisions** | Document choices with rationale (ADR-style) | Medium | None |
| **File modification manifest** | Know exactly which files will be changed | Low | Static analysis |
| **Verification criteria** | How to know each task succeeded | Low | Part of plan format |
| **Dependency identification** | Which tasks must complete before others | Low | DAG structure |
| **Risk assessment** | Flag potentially complex or unknown areas | Medium | Research integration |

**Notes:**
- The existing O-Bass planning structure (CONTEXT.md + XX-PLAN.md) is a strong pattern to follow
- Planning phase should produce artifacts similar to stage-level plans
- "Waterfall in 15 minutes" - rapid structured planning pays off enormously

### Differentiators

Nice-to-have features that improve planning quality.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Affected component analysis** | Auto-detect which DSP components, UI elements touched | Medium | AST parsing or grep |
| **Backward compatibility check** | Warn if change affects parameter serialization | Medium | Schema validation |
| **Complexity estimation** | Predict implementation time from plan structure | High | Heuristics needed |
| **Alternative approaches section** | Document paths not taken and why | Low | ADR best practice |
| **Rollback strategy** | How to undo if implementation fails | Low | Git-based, straightforward |
| **Integration with research** | Link plan tasks to research findings | Medium | Cross-reference system |
| **Plan validation gate** | Require human approval before implementation | Low | Checkpoint protocol exists |
| **Context gathering automation** | Auto-collect relevant files for plan context | Medium | Glob + grep automation |

---

## Planning Phase Flow

Based on research, the recommended planning phase structure:

```
Phase 0.5: Investigation (EXISTS)
    |
    v
Phase 1.0: Planning (NEW) -----> Produces: PLAN.md
    |                                       - Objective
    |                                       - Tasks with dependencies
    |                                       - Files to modify
    |                                       - Verification criteria
    |                                       - Architecture decisions
    v
[Approval Gate] -----> User confirms plan
    |
    v
Phase 0.9: Pre-implementation (EXISTS - backup, version bump)
    |
    v
Phase 3: Implementation (EXISTS - execute plan)
```

### Planning Document Template

Based on existing O-Bass patterns and ADR best practices:

```markdown
---
improvement: [name]
phase: planning
created: [date]
depends_on: [investigation output]
files_modified: [list]
complexity: [low/medium/high]
autonomous: [true/false]
---

## Objective

[What we're trying to achieve and why]

## Architecture Decisions

### Decision 1: [Name]
- **Context:** [Why this decision is needed]
- **Options considered:** [Alternatives]
- **Decision:** [What we chose]
- **Rationale:** [Why this option]
- **Consequences:** [What this means for implementation]

## Implementation Tasks

<task wave="1" type="auto">
  <name>Task 1: [Name]</name>
  <files>[Files to modify]</files>
  <action>[What to do]</action>
  <verify>[How to verify success]</verify>
  <done>[Success criteria]</done>
</task>

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| [risk] | Low/Med/High | [impact] | [mitigation] |

## Verification Criteria

- [ ] [Criterion 1]
- [ ] [Criterion 2]

## Rollback Strategy

[How to undo if needed]
```

---

## Anti-Features

Features to explicitly NOT build and why.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| **Automatic cleanup without backup** | Catastrophic data loss risk | Always require explicit backup step |
| **Force-push automation** | Risk of overwriting collaborator work | Manual confirmation required |
| **Planning without investigation** | Plans based on assumptions fail | Investigation phase is prerequisite |
| **Overly detailed plans** | Paralysis by analysis, plans become stale | Keep plans focused on "what" and "why", not line-by-line "how" |
| **Auto-generated code from plans** | Plans inform, not dictate implementation | Executor has discretion within plan boundaries |
| **Complex task dependency graphs** | Hard to understand, fragile to changes | Prefer linear sequences with clear wave structure |
| **Git history rewrite during active development** | Collaborator chaos | Cleanup only at project boundaries or release points |
| **Planning phase for trivial changes** | Overhead exceeds benefit | Keep Phase 0.5 investigation for small changes, planning for complex ones |

### When to Skip Planning Phase

Planning phase adds value for complex changes but creates overhead for simple ones. Skip planning when:

1. **Single-file changes** - Investigation sufficient
2. **Bug fixes with obvious cause** - Fix is clear from investigation
3. **Parameter value adjustments** - No architectural impact
4. **Documentation updates** - No implementation complexity
5. **Styling-only UI changes** - No DSP or state changes

Invoke planning phase when:

1. **Multiple components affected** - Need coordination strategy
2. **Architecture decisions required** - ADRs needed
3. **Unknown implementation path** - Research gaps exist
4. **Breaking changes possible** - Risk assessment needed
5. **New algorithm implementation** - Technical design required

---

## Feature Dependencies

```
Repository Cleanup
├── Large file detection (prerequisite)
├── Backup creation (prerequisite)
├── Build artifact removal (core)
├── Garbage collection (post-step)
└── Verification report (post-step)

Planning Phase
├── Investigation output (prerequisite - Phase 0.5)
├── Plan document creation (core)
│   ├── Architecture decisions
│   ├── Task decomposition
│   └── Verification criteria
├── Approval gate (checkpoint)
└── Pre-implementation (successor - Phase 0.9)
```

---

## Complexity Estimates

### Repository Cleanup

| Task | Complexity | Rationale |
|------|------------|-----------|
| Implement detection script | Low | Well-documented git commands |
| Integrate git-filter-repo | Medium | Tool installation, wrapper needed |
| Create backup/restore flow | Low | Standard git operations |
| Team coordination docs | Low | Documentation only |
| **Total** | **Medium** | Mostly tooling integration |

### Planning Phase

| Task | Complexity | Rationale |
|------|------------|-----------|
| Define plan document schema | Low | Template from existing patterns |
| Create planning phase workflow | Medium | Integration with existing phases |
| Implement approval gate | Low | Checkpoint protocol exists |
| Add skip-planning heuristics | Medium | Need to define thresholds |
| **Total** | **Medium** | Building on existing infrastructure |

---

## Integration with Existing System

### Repository Cleanup Integration Points

- **Entry point:** New `/cleanup` command or menu option in system setup
- **State tracking:** Record cleanup events in `.planning/cleanup-log.md`
- **Verification:** Post-cleanup size report and .gitignore validation

### Planning Phase Integration Points

| Existing Phase | Integration |
|----------------|-------------|
| Phase 0.5 (Investigation) | Planning consumes investigation output |
| Phase 0.9 (Pre-implementation) | Planning gate before pre-implementation |
| Phase 3 (Implementation) | Implementation follows plan document |
| Phase 4 (Verification) | Uses verification criteria from plan |

### Files to Create/Modify

**Repository Cleanup:**
- `scripts/repo-cleanup.sh` - Detection and cleanup automation
- `docs/repo-cleanup-protocol.md` - Team coordination documentation

**Planning Phase:**
- `.claude/skills/plugin-improve/templates/PLAN.md` - Plan document template
- `.claude/skills/plugin-improve/references/planning-workflow.md` - Planning phase workflow
- Update `.claude/commands/improve.md` - Add planning phase routing

---

## MVP Recommendation

### Phase 1: Repository Cleanup (Low Effort, High Impact)

Build first because:
- Immediate benefit (smaller repo, faster clones)
- Low complexity (existing tools)
- No dependencies on other features

Include:
1. Large file detection script
2. Backup protocol
3. git-filter-repo integration
4. Verification report
5. Team coordination docs

### Phase 2: Planning Phase (Medium Effort, High Value)

Build second because:
- Builds on existing patterns (O-Bass PLAN.md)
- Reduces rework on complex improvements
- Addresses gap in current workflow

Include:
1. Plan document template
2. Planning phase in plugin-improve workflow
3. Approval gate checkpoint
4. Skip-planning heuristics

### Defer to v1.2

- Automated complexity estimation
- Cross-session plan persistence
- Plan-to-implementation tracing

---

## Sources

### Repository Cleanup
- [git-filter-repo](https://github.com/newren/git-filter-repo) - Official replacement for git-filter-branch (HIGH confidence)
- [BFG Repo-Cleaner](https://rtyley.github.io/bfg-repo-cleaner/) - Simpler alternative (HIGH confidence)
- [Converting from BFG to git-filter-repo](https://github.com/newren/git-filter-repo/blob/main/Documentation/converting-from-bfg-repo-cleaner.md) (HIGH confidence)
- [Repo Hygiene Best Practices](https://www.repotocloud.com/repo-hygiene-keeping-your-github-projects-sane/) (MEDIUM confidence)

### Workflow Planning
- [Architecture Decision Records](https://adr.github.io/) - ADR format and best practices (HIGH confidence)
- [AWS ADR Best Practices](https://aws.amazon.com/blogs/architecture/master-architecture-decision-records-adrs-best-practices-for-effective-decision-making/) (HIGH confidence)
- [Addy Osmani's LLM Coding Workflow](https://addyosmani.com/blog/ai-coding-workflow/) - Planning-first approach (HIGH confidence)
- [Software Development Process Phases](https://monday.com/blog/rnd/software-development-process/) - 40-20-40 rule (MEDIUM confidence)
- [Technical Design Document Templates](https://www.atlassian.com/work-management/knowledge-sharing/documentation/software-design-document) (MEDIUM confidence)

### Existing Codebase Patterns
- `/plugins/O-Bass/.planning/stages/01-core-dsp-foundation/01-CONTEXT.md` - Context gathering pattern
- `/plugins/O-Bass/.planning/stages/01-core-dsp-foundation/01-01-PLAN.md` - Plan document format
- `/.claude/skills/plugin-ideation/references/improvement-workflow.md` - Current improvement flow

---

## Quality Gate Verification

- [x] Categories are clear (table stakes vs differentiators vs anti-features)
- [x] Complexity noted for each feature (Low/Medium/High)
- [x] Dependencies on existing features identified
- [x] Integration points with existing system documented
- [x] MVP recommendation with phased approach
- [x] Sources documented with confidence levels

---

*Researched: 2026-02-01*
*Confidence: HIGH - Verified against official documentation and established codebase patterns*
