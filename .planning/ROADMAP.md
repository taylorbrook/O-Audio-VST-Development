# Roadmap: Plugin Freedom System Overhaul

## Overview

Transform the Plugin Freedom System from a collection of loosely-coordinated agents into a contract-driven, quality-gated workflow that produces professional-quality audio plugins with minimal rework. The journey follows a foundation-first approach: establish contracts and state management before building verification infrastructure, then implement quality gates, enhance domain specialization, and finally address module system reliability.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Agent Contracts** - Define explicit input/output contracts for all agents
- [ ] **Phase 2: State Management** - Harden file-based state persistence and session continuity
- [ ] **Phase 3: Structured Handoffs** - Implement schema-validated handoff documents between stages
- [ ] **Phase 4: Verification Infrastructure** - Build generator-critic loops and domain validation
- [ ] **Phase 5: Quality Gates** - Implement blocking gates at stage boundaries
- [ ] **Phase 6: Domain Specialization** - Encode professional domain expertise into agents
- [ ] **Phase 7: Module System** - Fix module reliability and dependency tracking

## Phase Details

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
- [x] 01-01-PLAN.md — Audit existing agents and create gap analysis
- [x] 01-02-PLAN.md — Define input/output schemas for all 9 agents
- [x] 01-03-PLAN.md — Implement contract validation and missing agent specs

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
  6. Plugin state is isolated — each plugin's state under `plugins/{Name}/.planning/`, `/focus` loads only that plugin's context
**Plans**: 4 plans (3 infrastructure + 1 gap closure)

Plans:
- [x] 02-01-PLAN.md — Create workflow state infrastructure and migrate registry
- [x] 02-02-PLAN.md — Build state validation and recovery mechanisms
- [x] 02-03-PLAN.md — Implement session resume and plugin isolation
- [ ] 02-04-PLAN.md — Gap closure: fix registry inconsistency and create checkpoint (verification)

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
- [ ] 03-01-PLAN.md — Create handoff schemas for all stage boundaries (0-1, 1-2, 2-3, 3-4)
- [ ] 03-02-PLAN.md — Implement validation scripts and /plugin-handoff command

### Phase 4: Verification Infrastructure
**Goal**: Independent verification through generator-critic loops catches issues before stage transitions
**Depends on**: Phase 1 (contracts define what to verify against), Phase 3 (handoffs define transition points)
**Requirements**: CRIT-01, CRIT-02, CRIT-03, CRIT-04
**Success Criteria** (what must be TRUE):
  1. Critic agent validates outputs against contracts before any stage handoff
  2. Iterative refinement loop runs until quality threshold met (max 3 iterations)
  3. Domain-specific critics exist for DSP (real-time rules) and UI (polish standards)
  4. Token budget awareness stops iteration when budget exceeded
**Plans**: TBD

Plans:
- [ ] 04-01: Build critic agent infrastructure
- [ ] 04-02: Implement domain-specific critics (DSP, UI)

### Phase 5: Quality Gates
**Goal**: Stage progression blocked until measurable success criteria pass
**Depends on**: Phase 4 (verification infrastructure provides gate checks)
**Requirements**: GATE-01, GATE-02, GATE-03, GATE-04, GATE-05, GATE-06
**Success Criteria** (what must be TRUE):
  1. Automated verification runs at each stage boundary (0->1, 1->2, 2->3, 3->4)
  2. Each stage has measurable success criteria (not subjective pass/fail)
  3. Stage progression physically blocked until gate passes
  4. Verification depth matches complexity (smoke test vs full validation)
  5. Code review step integrated at end of implementation phases
**Plans**: TBD

Plans:
- [ ] 05-01: Define gate criteria for each stage boundary
- [ ] 05-02: Implement blocking gates with tiered verification
- [ ] 05-03: Integrate code review and simplification passes

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
**Plans**: TBD

Plans:
- [ ] 06-01: Encode real-time safety rules in DSP agent
- [ ] 06-02: Encode thread-safety patterns in GUI agent
- [ ] 06-03: Define professional quality standards and specialist agent specs

### Phase 7: Module System
**Goal**: Module add/remove/update works reliably with tracked dependencies
**Depends on**: Phase 1 (contracts define module interfaces)
**Requirements**: MODS-01, MODS-02, MODS-03, MODS-04
**Success Criteria** (what must be TRUE):
  1. /module:add and /module:remove work reliably across all plugins
  2. Module registry tracks which plugins depend on which modules
  3. Module versions use semver with compatibility checks
  4. Documentation exists for manual rebuild after module updates
**Plans**: TBD

Plans:
- [ ] 07-01: Fix module add/remove reliability
- [ ] 07-02: Implement dependency tracking and version management

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Agent Contracts | 3/3 | Complete | 2026-01-30 |
| 2. State Management | 3/4 | Gap closure pending | - |
| 3. Structured Handoffs | 0/2 | Ready for execution | - |
| 4. Verification Infrastructure | 0/2 | Not started | - |
| 5. Quality Gates | 0/3 | Not started | - |
| 6. Domain Specialization | 0/3 | Not started | - |
| 7. Module System | 0/2 | Not started | - |

## Coverage Validation

**Total v1 Requirements:** 35
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

No orphaned requirements. No duplicates.

---
*Roadmap created: 2026-01-30*
*Last updated: 2026-01-31 — Phase 2 gap closure plan created (02-04)*
