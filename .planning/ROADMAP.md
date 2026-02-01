# Roadmap: Plugin Freedom System Overhaul

## Milestones

- [x] **v1.0 MVP** - Phases 1-7 (shipped 2026-02-01)
- [ ] **v1.1 Cleanup & Workflow Polish** - Phases 8-9 (in progress)
- [ ] **v1.2 Windows Distribution** - TBD (planned)

## Phases

<details>
<summary>v1.0 MVP (Phases 1-7) - SHIPPED 2026-02-01</summary>

### Phase 1: Agent Contracts
**Goal**: Every agent has explicit, validated contracts defining what it accepts and produces
**Depends on**: Nothing (foundation phase)
**Requirements**: AGNT-01, AGNT-02, AGNT-03, AGNT-04, AGNT-05, AGNT-06, AGNT-07
**Success Criteria** (what must be TRUE):
  1. Each of the 9 existing agents has a JSON schema defining required inputs
  2. Each agent has a JSON schema defining expected outputs with validation
  3. Agent invocation fails fast with clear error when inputs are invalid
  4. Each agent has documented scope boundaries (explicit does/doesn't do lists)
  5. Audit report identifies gaps, overlaps, and missing agents with specs for new agents
**Plans**: 3 plans

Plans:
- [x] 01-01-PLAN.md - Audit existing agents and create gap analysis
- [x] 01-02-PLAN.md - Define input/output schemas for all 9 agents
- [x] 01-03-PLAN.md - Implement contract validation and missing agent specs

### Phase 2: State Management
**Goal**: Workflow state persists reliably across sessions with automatic corruption detection, recovery, and multi-plugin isolation
**Depends on**: Phase 1 (contracts define what state agents need)
**Requirements**: STAT-01, STAT-02, STAT-03, STAT-04, STAT-05, STAT-06
**Success Criteria** (what must be TRUE):
  1. All workflow state persists to .planning/ files following GSD pattern
  2. Session resume via /continue restores full context from last checkpoint
  3. Inconsistencies between STATUS.md and registry are detected automatically
  4. Corrupted state triggers recovery mechanism that auto-repairs
  5. Context boundary transitions include explicit next-command instructions
  6. Plugin state is isolated - each plugin's state under `plugins/{Name}/.planning/`, `/focus` loads only that plugin's context
**Plans**: 4 plans

Plans:
- [x] 02-01-PLAN.md - Create workflow state infrastructure and migrate registry
- [x] 02-02-PLAN.md - Build state validation and recovery mechanisms
- [x] 02-03-PLAN.md - Implement session resume and plugin isolation
- [x] 02-04-PLAN.md - Gap closure: fix registry inconsistency and create checkpoint (verification)

### Phase 3: Structured Handoffs
**Goal**: Stage transitions preserve context through schema-validated handoff documents
**Depends on**: Phase 2 (state management provides persistence layer for handoffs)
**Requirements**: HAND-01, HAND-02, HAND-03, HAND-04, HAND-05
**Success Criteria** (what must be TRUE):
  1. Each stage boundary has a schema-validated handoff document format
  2. Stage transitions fail if required artifacts are missing
  3. Decision audit trail captures why choices were made at each stage
  4. Handoff schemas are versioned with semver for evolution
  5. Context clear messages include copy-paste slash command for continuation
**Plans**: 2 plans

Plans:
- [x] 03-01-PLAN.md - Create handoff schemas for all stage boundaries (0-1, 1-2, 2-3, 3-4)
- [x] 03-02-PLAN.md - Implement validation scripts and /plugin-handoff command

### Phase 4: Verification Infrastructure
**Goal**: Independent verification through generator-critic loops catches issues before stage transitions
**Depends on**: Phase 1 (contracts define what to verify against), Phase 3 (handoffs define transition points)
**Requirements**: CRIT-01, CRIT-02, CRIT-03, CRIT-04
**Success Criteria** (what must be TRUE):
  1. Critic agent validates outputs against contracts before any stage handoff
  2. Iterative refinement loop runs until quality threshold met (max 3 iterations)
  3. Domain-specific critics exist for DSP (real-time rules) and UI (polish standards)
  4. Token budget awareness stops iteration when budget exceeded
**Plans**: 2 plans

Plans:
- [x] 04-01-PLAN.md - Build critic agent infrastructure (schemas, orchestration script)
- [x] 04-02-PLAN.md - Implement domain-specific critics (DSP, UI) and /plugin-critique command

### Phase 5: Quality Gates
**Goal**: Stage progression blocked until measurable success criteria pass
**Depends on**: Phase 4 (verification infrastructure provides gate checks)
**Requirements**: GATE-01, GATE-02, GATE-03, GATE-04, GATE-05, GATE-06
**Success Criteria** (what must be TRUE):
  1. Automated verification runs at each stage boundary (0->1, 1->2, 2->3, 3->4)
  2. Each stage has measurable success criteria (not subjective pass/fail)
  3. Stage progression physically blocked until gate passes
  4. Verification depth uniform at every gate (user decision: no tiered/fast-path)
  5. Code review step integrated at end of implementation phases
**Plans**: 3 plans

Plans:
- [x] 05-01-PLAN.md - Create gate report schema and unified run-gate.sh script
- [x] 05-02-PLAN.md - Create code review checklist template and run-code-review.sh
- [x] 05-03-PLAN.md - Integrate gates into /plugin-execute and /plugin-handoff commands

### Phase 6: Domain Specialization
**Goal**: Agents encode professional domain expertise that catches domain-specific quality issues
**Depends on**: Phase 5 (gates enforce expertise is applied consistently)
**Requirements**: DOMN-01, DOMN-02, DOMN-03, DOMN-04, DOMN-05, DOMN-06
**Success Criteria** (what must be TRUE):
  1. DSP agent rejects code with allocations, locks, or file I/O in processBlock
  2. GUI agent enforces APVTS atomic patterns and relay lifecycle rules
  3. All relevant agents follow JUCE 8 best practices by default
  4. Professional quality standards defined (what makes commercial plugins pro)
  5. Music theory agent spec exists for domain-aware tuning assistance
**Plans**: 3 plans

Plans:
- [x] 06-01-PLAN.md - Encode exhaustive real-time safety rules in DSP agent and critic
- [x] 06-02-PLAN.md - Encode thread-safety patterns in GUI agent and critic
- [x] 06-03-PLAN.md - Define professional quality standards and create specialist agent specs

### Phase 7: Module System
**Goal**: Module add/remove/update works reliably with tracked dependencies
**Depends on**: Phase 1 (contracts define module interfaces)
**Requirements**: MODS-01, MODS-02, MODS-03, MODS-04
**Success Criteria** (what must be TRUE):
  1. /module:add and /module:remove work reliably across all plugins
  2. Module registry tracks which plugins depend on which modules
  3. Module versions use semver with compatibility checks
  4. Documentation exists for manual rebuild after module updates
**Plans**: 4 plans

Plans:
- [x] 07-01-PLAN.md - Extend registry schema and migrate to v3.0.0 with module tracking
- [x] 07-02-PLAN.md - Implement reliable /module:add and /module:remove with content hash
- [x] 07-03-PLAN.md - Add semver comparison and customization detection for safe upgrades
- [x] 07-04-PLAN.md - Integrate update notifications and create documentation

</details>

### v1.1 Cleanup & Workflow Polish (In Progress)

**Milestone Goal:** Clean up repository debt and enhance plugin-improve workflow with structured planning phase

#### Phase 8: Repository Cleanup
**Goal**: Repository is clean, fast to clone, and properly protected from future artifact accumulation
**Depends on**: Nothing (v1.1 foundation, independent of workflow changes)
**Requirements**: REPO-01, REPO-02, REPO-03, REPO-04, REPO-05, REPO-06
**Success Criteria** (what must be TRUE):
  1. Repository size reduced from ~584MB to under 100MB (verified via `du -sh .git`)
  2. Fresh clone completes in under 30 seconds on typical broadband
  3. `git status` shows no .DS_Store, .o, .a, or build/ files as untracked after clean build
  4. Pre-cleanup backup branch exists and can be recovered if needed
  5. CI/CD pipelines pass on first run after history rewrite (no stale cache failures)
**Plans**: 2 plans

Plans:
- [x] 08-01-PLAN.md - Create backup and execute git history cleanup
- [x] 08-02-PLAN.md - Verify cleanup, update .gitignore, clear CI caches

#### Phase 9: Workflow Planning Phase
**Goal**: Complex plugin improvements have a planning step that reduces rework
**Depends on**: Phase 8 (repository cleanup complete - clean working state)
**Requirements**: PLAN-01, PLAN-02, PLAN-03, PLAN-04, PLAN-05, PLAN-06, PLAN-07
**Success Criteria** (what must be TRUE):
  1. Tier 2/3 improvements trigger Phase 0.6 Planning before implementation starts
  2. Tier 1 improvements skip planning entirely (no added overhead)
  3. Planning output includes task breakdown with dependencies that can be followed sequentially
  4. User must explicitly approve plan before implementation begins (cannot be bypassed accidentally)
  5. Deep-research handoff (Phase 0.45) still works - skips planning when research already done
**Plans**: 2 plans

Plans:
- [ ] 09-01-PLAN.md - Create planning protocol reference and template
- [ ] 09-02-PLAN.md - Update SKILL.md with Phase 0.6 and conditional triggers

### v1.2 Windows Distribution (Planned)

**Milestone Goal:** Automated Windows installer creation in CI/CD

*Phases TBD when v1.2 starts*

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8 -> 9

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Agent Contracts | v1.0 | 3/3 | Complete | 2026-01-30 |
| 2. State Management | v1.0 | 4/4 | Complete | 2026-01-30 |
| 3. Structured Handoffs | v1.0 | 2/2 | Complete | 2026-01-31 |
| 4. Verification Infrastructure | v1.0 | 2/2 | Complete | 2026-01-31 |
| 5. Quality Gates | v1.0 | 3/3 | Complete | 2026-01-31 |
| 6. Domain Specialization | v1.0 | 3/3 | Complete | 2026-01-31 |
| 7. Module System | v1.0 | 4/4 | Complete | 2026-02-01 |
| 8. Repository Cleanup | v1.1 | 2/2 | Complete | 2026-02-01 |
| 9. Workflow Planning Phase | v1.1 | 0/2 | Not started | - |

## Coverage Validation

### v1.0 Requirements (Complete)

**Total v1.0 Requirements:** 35
**Mapped:** 35/35 (100%)

| Category | Requirements | Phase | Count |
|----------|-------------|-------|-------|
| Agent Contracts | AGNT-01 to AGNT-07 | Phase 1 | 7 |
| State Management | STAT-01 to STAT-06 | Phase 2 | 6 |
| Structured Handoffs | HAND-01 to HAND-05 | Phase 3 | 5 |
| Generator-Critic | CRIT-01 to CRIT-04 | Phase 4 | 4 |
| Quality Gates | GATE-01 to GATE-06 | Phase 5 | 6 |
| Domain Expertise | DOMN-01 to DOMN-06 | Phase 6 | 6 |
| Module System | MODS-01 to MODS-04 | Phase 7 | 4 |

### v1.1 Requirements (Active)

**Total v1.1 Requirements:** 13
**Mapped:** 13/13 (100%)

| Category | Requirements | Phase | Count |
|----------|-------------|-------|-------|
| Repository Cleanup | REPO-01 to REPO-06 | Phase 8 | 6 |
| Workflow Planning | PLAN-01 to PLAN-07 | Phase 9 | 7 |

No orphaned requirements. No duplicates.

---
*Roadmap created: 2026-01-30*
*Last updated: 2026-02-01 - Phase 8 complete (636MB → 58MB)*
