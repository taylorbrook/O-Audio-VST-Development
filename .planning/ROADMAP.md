# Milestone v1.3: System Modernization (Opus 4.6 + GSD Alignment)

**Status:** In Progress
**Phases:** 14-17
**Total Plans:** TBD

## Overview

Audit and modernize the Plugin Freedom System to leverage Opus 4.6 capabilities and align with GSD 1.18.0 features — eliminating custom code where the framework now provides native support, improving context persistence across sessions, and selectively adopting agent teams for parallel read-heavy work. The four phases move from lowest risk (preventing deprecation breakage) to highest risk (experimental agent teams), with canary plugin testing after every change to protect 35+ production plugins.

## Phases

- [x] **Phase 14: Platform Alignment** - Prevent breakage from Opus 4.6 deprecations and adopt effort-based model tuning
- [ ] **Phase 15: Context Persistence** - Eliminate information loss between sessions and across agents
- [ ] **Phase 16: GSD Deduplication** - Replace custom code with GSD 1.18.0 native equivalents
- [ ] **Phase 17: Agent Intelligence** - Adopt agent teams for parallel research/review and fine-grained quality hooks

## Phase Details

### Phase 14: Platform Alignment
**Goal**: All agents run on Opus 4.6 without deprecation warnings, broken paths, or API errors — and use effort-tuned model profiles instead of binary Sonnet/Opus switching
**Depends on**: Phase 13 (v1.2 complete)
**Requirements**: PLAT-01, PLAT-02, PLAT-03, PLAT-04, PLAT-07
**Success Criteria** (what must be TRUE):
  1. Every agent invocation uses adaptive thinking (no explicit `thinking: {type: "enabled"}` or `budget_tokens` remaining in any agent definition)
  2. PreCompact.sh references only `.planning/` paths — zero references to `.ideas/` or `.continue-here.md`
  3. No skill or agent prefills an assistant message (Opus 4.6 returns 400 on prefilled assistant messages)
  4. Agent effort levels are configured per-agent (dsp-agent and research-planning-agent at max/high, foundation-shell at medium, etc.) replacing the binary Sonnet/Opus model selection
  5. Canary plugin (O-SimpleReverb) builds and validates successfully after all changes
**Plans**: 4 plans
Plans:
- [x] 14-01-PLAN.md — Agent & critic definition cleanup (model frontmatter removal + agent-profiles.json)
- [x] 14-02-PLAN.md — Skill & command reference updates (effort terminology + Co-Authored-By fix)
- [x] 14-03-PLAN.md — Plugin content migration (.ideas/ to .planning/ + .gitignore rules)
- [x] 14-04-PLAN.md — Hook/script path updates + canary testing (O-SimpleReverb + O-AnalogEQ)

### Phase 15: Context Persistence
**Goal**: Agents retain critical context across compaction events and sessions — plugin parameters, DSP components, contract paths, and stage decisions survive context loss
**Depends on**: Phase 14
**Requirements**: PLAT-05, PLAT-06, CTXP-01, CTXP-02, CTXP-03, CTXP-04
**Success Criteria** (what must be TRUE):
  1. After compaction, the agent's context still contains the current plugin name, stage/phase, parameter IDs, DSP component names, and contract paths (verified by manual inspection of post-compaction context)
  2. Complex DSP agents (complexity >= 4) load full research documents via 1M context window instead of 200-word summaries
  3. A per-plugin history digest file exists that compiles stage decisions into structured JSON, loadable by any agent in under 500 tokens
  4. Express plugin creation (`/implement --auto`) generates plans via auto mode instead of skipping planning entirely
  5. Five agents (troubleshoot, dsp, gui, research-planning, validation) persist learned patterns across sessions via `.claude/agent-memory/`
**Plans**: TBD

### Phase 16: GSD Deduplication
**Goal**: Custom PFS code that duplicates GSD 1.18.0 functionality is replaced with framework equivalents — reducing maintenance burden while preserving all 6 domain-specific validators
**Depends on**: Phase 14, Phase 15
**Requirements**: GSDD-01, GSDD-02, GSDD-03, GSDD-04
**Success Criteria** (what must be TRUE):
  1. State operations (advance-plan, update-progress, record-metric) use `gsd-tools` CLI instead of manual STATE.md parsing — verified by grep showing zero manual STATE.md write operations in agent code
  2. Frontmatter operations use GSD get/set/merge where schema-compatible, with custom Python validator retained only for PFS-specific 10-field research schema
  3. Six structural validators (plan-structure, phase-completeness, references, commits, artifacts, key-links) are replaced by GSD verification suite — six domain validators (DSP safety, APVTS matching, WebView bindings, checksums, cross-contract, resource accountability) remain untouched
  4. Post-plan validation cross-references CONTEXT.md user decisions with PLAN.md tasks, flagging contradictions before execution begins
**Plans**: TBD

### Phase 17: Agent Intelligence
**Goal**: Parallel agent teams are available for read-heavy research and review workflows, with fine-grained quality hooks and configurable branching — while the sequential Stage 0-4 pipeline remains the primary implementation path
**Depends on**: Phases 14-16
**Requirements**: AGNT-01, AGNT-02, AGNT-03, AGNT-04, AGNT-05, AGNT-06, AGNT-07
**Success Criteria** (what must be TRUE):
  1. Research phase can spawn 2-3 parallel researchers (DSP algorithms, UI patterns, module audit) that share findings via debate — verified on canary plugin research with no file conflicts
  2. Cross-stage review can spawn parallel critics (DSP, UI, architecture) that produce a unified review report — verified as read-only with no file modifications
  3. Plan approval gates allow the team lead to review and reject teammate plans before any implementation begins
  4. Delegate mode restricts the orchestrator to coordination-only tools (no direct file writes during delegated work)
  5. TaskCompleted hooks enable per-task validation within plans — exit code 2 prevents task completion with feedback, wiring domain validators to individual plan tasks
**Plans**: TBD

## Progress

**Execution Order:** 14 -> 15 -> 16 -> 17

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 14. Platform Alignment | v1.3 | 4/4 | ✓ Complete | 2026-02-08 |
| 15. Context Persistence | v1.3 | 0/TBD | Not started | - |
| 16. GSD Deduplication | v1.3 | 0/TBD | Not started | - |
| 17. Agent Intelligence | v1.3 | 0/TBD | Not started | - |

---

## Key Constraints

- **P34**: Classify all custom code as duplicate/extension/workaround BEFORE removing (Phase 16)
- **P36**: Agent Teams ONLY for read-heavy parallel work (research, review), NOT implementation (Phase 17)
- **P37**: Create terminology mapping document as prerequisite (Phase 17)
- **P40**: Canary plugin testing (O-SimpleReverb) after EVERY change — 35+ production plugins depend on this

## Research Flags

| Phase | Depth | Rationale |
|-------|-------|-----------|
| 14. Platform Alignment | SKIP | Official Opus 4.6 docs comprehensive, migration path clear |
| 15. Context Persistence | LOW | Compaction API well-documented, history digest straightforward |
| 16. GSD Deduplication | MEDIUM | Audit 12 validators for structural vs domain classification, map gsd-tools commands |
| 17. Agent Intelligence | HIGH | Experimental feature, needs proof-of-concept with canary plugin before production use |

---

<details>
<summary>Previous Milestones</summary>

**v1.0 MVP** -- Phases 1-7, 21 plans (shipped 2026-02-01)
**v1.1 Cleanup & Workflow Polish** -- Phases 8-9, 4 plans (shipped 2026-02-02)
**v1.2 Agent Intelligence & Resource Orchestration** -- Phases 10-13, 12 plans (shipped 2026-02-06)

See `.planning/milestones/` for archived roadmaps.

</details>

---
*Roadmap created: 2026-02-08*
*Last updated: 2026-02-08 — Phase 14 complete*
