# Roadmap: Plugin Freedom System Overhaul

## Milestones

- ✅ **v1.0 MVP** - Phases 1-7 (shipped 2026-02-01)
- ✅ **v1.1 Cleanup & Workflow Polish** - Phases 8-9 (shipped 2026-02-02)
- 🚧 **v1.2 Agent Intelligence & Resource Orchestration** - Phases 10-13 (in progress)

## Phases

<details>
<summary>✅ v1.0 MVP (Phases 1-7) — SHIPPED 2026-02-01</summary>

- [x] Phase 1: Agent Contracts (3/3 plans) — completed 2026-01-30
- [x] Phase 2: State Management (4/4 plans) — completed 2026-01-30
- [x] Phase 3: Structured Handoffs (2/2 plans) — completed 2026-01-31
- [x] Phase 4: Verification Infrastructure (2/2 plans) — completed 2026-01-31
- [x] Phase 5: Quality Gates (3/3 plans) — completed 2026-01-31
- [x] Phase 6: Domain Specialization (3/3 plans) — completed 2026-01-31
- [x] Phase 7: Module System (4/4 plans) — completed 2026-02-01

**Archive:** `.planning/milestones/v1.0-ROADMAP.md` (when created)

</details>

<details>
<summary>✅ v1.1 Cleanup & Workflow Polish (Phases 8-9) — SHIPPED 2026-02-02</summary>

- [x] Phase 8: Repository Cleanup (2/2 plans) — completed 2026-02-01
- [x] Phase 9: Workflow Planning Phase (2/2 plans) — completed 2026-02-02

**Archive:** `.planning/milestones/v1.1-ROADMAP.md`

</details>

### 🚧 v1.2 Agent Intelligence & Resource Orchestration (In Progress)

**Milestone Goal:** Ensure all agents are invoked with relevant knowledge base resources and report what they consulted, with full traceability and graceful degradation.

- [x] **Phase 10: Resource Index & Discovery** - Static manifest and keyword-based discovery engine — completed 2026-02-05
- [ ] **Phase 11: Context Injection Pipeline** - Deliver discovered resources to agents before execution
- [ ] **Phase 12: Accountability & Validation** - Agents report resources consulted; hooks validate usage
- [ ] **Phase 13: Maintenance Tooling & Hardening** - Auto-generation, freshness tracking, graceful degradation

## Phase Details

### Phase 10: Resource Index & Discovery
**Goal**: The system can identify which research documents, patterns, and knowledge base entries are relevant to any given agent task
**Depends on**: Phase 9 (v1.1 complete)
**Requirements**: DISC-01, DISC-02, DISC-03, DISC-04
**Success Criteria** (what must be TRUE):
  1. A JSON manifest exists at `.claude/resource-index.json` cataloging all research docs with keywords, categories, stage applicability, and summaries
  2. Running the discovery script with a task context (plugin name, stage, agent type) returns a ranked list of relevant resources in under 1 second
  3. Discovery results change based on the current plugin stage (e.g., Stage 2 surfaces DSP research, Stage 3 surfaces UI research)
  4. Discovery results change based on the invoking agent (dsp-agent receives DSP resources, gui-agent receives UI resources, not vice versa)
  5. Discovery script validates the manifest against its JSON schema before use
**Plans**: 4 plans

Plans:
- [x] 10-01-PLAN.md — Schema & validation infrastructure (JSON Schema, frontmatter validator, jsonschema install)
- [x] 10-02-PLAN.md — Frontmatter retrofit batch 1 (13 DSP-core research docs)
- [x] 10-03-PLAN.md — Frontmatter retrofit batch 2 (9 remaining root docs + 4 stutter-effects)
- [x] 10-04-PLAN.md — Generator & discovery scripts + integration verification

### Phase 11: Context Injection Pipeline
**Goal**: Agents automatically receive relevant research resources as part of their execution context, without manual prompt construction
**Depends on**: Phase 10
**Requirements**: INJT-01, INJT-02, INJT-03, INJT-04
**Success Criteria** (what must be TRUE):
  1. When a skill orchestrator spawns a stage agent via Task(), the prompt includes discovered resource summaries and excerpted content via inject-context.py
  2. Skill orchestrators (plugin-workflow, plugin-planning, improve-milestone) include a resource section in agent prompts before Task() calls
  3. Stage-specific troubleshooting patterns (e.g., stage-2-patterns.md) are auto-injected inline by inject-context.py for the corresponding agent without manual configuration
  4. Total injected research context stays within 4,000 tokens per agent invocation (no context window budget exhaustion)
**Plans**: 2 plans

Plans:
- [ ] 11-01-PLAN.md — Context injection utility script (inject-context.py with content extraction, budget management, CLI)
- [ ] 11-02-PLAN.md — Skill integration + ROADMAP update (modify all 7+ skill files, end-to-end verification)

### Phase 12: Accountability & Validation
**Goal**: The system tracks which resources agents actually consulted and warns when expected resources were skipped
**Depends on**: Phase 11
**Requirements**: ACCT-01, ACCT-02, ACCT-03
**Success Criteria** (what must be TRUE):
  1. Subagent report JSON schema includes an optional `resources_consulted` field that all existing agents can populate without breaking backward compatibility
  2. All five stage agents (dsp-agent, gui-agent, foundation-shell-agent, research-planning-agent, polish-agent) include `resources_consulted` in their reports when resources were injected
  3. SubagentStop hook logs a warning when an agent received MUST-READ resources but did not report consulting them (warning only, never blocks workflow)
**Plans**: TBD

Plans:
- [ ] 12-01: TBD
- [ ] 12-02: TBD

### Phase 13: Maintenance Tooling & Hardening
**Goal**: The resource system maintains itself with minimal manual effort and degrades gracefully when components are missing or stale
**Depends on**: Phase 10
**Requirements**: MAINT-01, MAINT-02, MAINT-03, MAINT-04
**Success Criteria** (what must be TRUE):
  1. Running the index auto-generation script rebuilds the manifest from research/ folder metadata (YAML frontmatter), producing a valid manifest without manual JSON editing
  2. All research documents have YAML frontmatter with created date, last_verified date, and JUCE version fields
  3. Discovery warns visibly when injecting a resource whose last_verified date is older than 90 days
  4. Agents proceed normally if the manifest is missing, the discovery script fails, or no resources match — with a logged warning but no workflow interruption
**Plans**: TBD

Plans:
- [ ] 13-01: TBD
- [ ] 13-02: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10 → 11 → 12 → 13

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
| 9. Workflow Planning Phase | v1.1 | 2/2 | Complete | 2026-02-02 |
| 10. Resource Index & Discovery | v1.2 | 4/4 | Complete | 2026-02-05 |
| 11. Context Injection Pipeline | v1.2 | 0/2 | Not started | - |
| 12. Accountability & Validation | v1.2 | 0/? | Not started | - |
| 13. Maintenance Tooling & Hardening | v1.2 | 0/? | Not started | - |

## Coverage Summary

### v1.0 Requirements: 35/35 (100%)
### v1.1 Requirements: 13/13 (100%)
### v1.2 Requirements: 15/15 (100%)

| Requirement | Phase |
|-------------|-------|
| DISC-01 | Phase 10 |
| DISC-02 | Phase 10 |
| DISC-03 | Phase 10 |
| DISC-04 | Phase 10 |
| INJT-01 | Phase 11 |
| INJT-02 | Phase 11 |
| INJT-03 | Phase 11 |
| INJT-04 | Phase 11 |
| ACCT-01 | Phase 12 |
| ACCT-02 | Phase 12 |
| ACCT-03 | Phase 12 |
| MAINT-01 | Phase 13 |
| MAINT-02 | Phase 13 |
| MAINT-03 | Phase 13 |
| MAINT-04 | Phase 13 |

See `.planning/milestones/v1.X-REQUIREMENTS.md` for archived requirement details.

---
*Roadmap created: 2026-01-30*
*Last updated: 2026-02-05 — Phase 11 planned (2 plans)*
