# Roadmap: Plugin Freedom System

## Milestones

- ✅ **v1.0 MVP** -- Phases 1-7 (shipped 2026-02-01)
- ✅ **v1.1 Cleanup & Workflow Polish** -- Phases 8-9 (shipped 2026-02-02)
- ✅ **v1.2 Agent Intelligence & Resource Orchestration** -- Phases 10-13 (shipped 2026-02-06)
- ✅ **v1.3 System Modernization** -- Phases 14-17 (shipped 2026-02-10)
- [ ] **v1.4 System Hygiene & Quality Gates** -- Phases 18-22 (in progress)

## Phases

<details>
<summary>v1.0 MVP (Phases 1-7) -- SHIPPED 2026-02-01</summary>

- [x] Phase 1: Agent Contracts (3 plans) -- completed
- [x] Phase 2: State Management (4 plans) -- completed
- [x] Phase 3: Structured Handoffs (2 plans) -- completed
- [x] Phase 4: Verification Infrastructure (2 plans) -- completed
- [x] Phase 5: Quality Gates (3 plans) -- completed
- [x] Phase 6: Domain Specialization (3 plans) -- completed
- [x] Phase 7: Module System (4 plans) -- completed

</details>

<details>
<summary>v1.1 Cleanup & Workflow Polish (Phases 8-9) -- SHIPPED 2026-02-02</summary>

- [x] Phase 8: Repository Cleanup (2 plans) -- completed
- [x] Phase 9: Workflow Planning Phase (2 plans) -- completed

</details>

<details>
<summary>v1.2 Agent Intelligence & Resource Orchestration (Phases 10-13) -- SHIPPED 2026-02-06</summary>

- [x] Phase 10: Resource Discovery (3 plans) -- completed
- [x] Phase 11: Context Injection (3 plans) -- completed
- [x] Phase 12: Accountability (3 plans) -- completed
- [x] Phase 13: Self-Maintenance (3 plans) -- completed

</details>

<details>
<summary>v1.3 System Modernization (Phases 14-17) -- SHIPPED 2026-02-10</summary>

- [x] Phase 14: Platform Alignment (4 plans) -- completed 2026-02-08
- [x] Phase 15: Context Persistence (4 plans) -- completed 2026-02-09
- [x] Phase 16: GSD Deduplication (3 plans) -- completed 2026-02-09
- [x] Phase 17: Agent Intelligence (3 plans) -- completed 2026-02-10

</details>

### v1.4 System Hygiene & Quality Gates (In Progress)

**Milestone Goal:** Address all findings from the full system review -- activate dormant quality gates, remove dead code, fix research governance gaps, consolidate overlapping subsystems, and add structural improvements for agent memory and validation.

- [ ] **Phase 18: Dead Code Removal** - Delete all dead .sh hooks, vestigial hooks.json, unreferenced agents, deprecated registry, and add __pycache__ to gitignore
- [ ] **Phase 19: Quality Gate Activation** - Activate 3 dormant hooks (SubagentStop, research frontmatter, resource index regen) in settings.json
- [ ] **Phase 20: Research Governance & Review** - Regenerate resource index, standardize frontmatter across all research docs, audit for coverage gaps, fill gaps, and flag stale content
- [ ] **Phase 21: Skill & Infrastructure Consolidation** - Merge plugin-phases into plugin-workflow, update references, clean up agent memory placeholders, relocate docs-only files, exclude dev artifacts
- [ ] **Phase 22: Structural Improvements** - Build agent memory write-back mechanism, resolve validation cache (activate or remove), verify and clean up canary-test.sh

## Phase Details

### Phase 18: Dead Code Removal
**Goal**: The system has zero dead code from the Python migration and no vestigial configuration files -- a clean baseline before adding new hooks
**Depends on**: Nothing (first phase of v1.4)
**Requirements**: DEAD-01, DEAD-02, DEAD-03, DEAD-04, DEAD-05
**Success Criteria** (what must be TRUE):
  1. No `.sh` files exist in `.claude/hooks/` -- all 10 shell hook files are gone
  2. `hooks.json` does not exist and `settings.json` is the sole hook configuration (no shadow config)
  3. No agent definition exists that is referenced by zero commands or skills (aesthetics-agent, dynamic-researcher, music-theory-agent are gone)
  4. `.claude/plugin-registry.json` does not exist (deprecated duplicate removed)
  5. `__pycache__` directories are not tracked by git, and `.gitignore` contains `__pycache__/` and `*.pyc` patterns
**Plans:** 3 plans
Plans:
- [ ] 18-01-PLAN.md -- Delete .sh hooks, verify hooks.json gone, remove __pycache__ from git
- [ ] 18-02-PLAN.md -- Delete dead agents and deprecated plugin-registry.json, scrub references
- [ ] 18-03-PLAN.md -- Deep scan .claude/ for additional dead code, document findings

### Phase 19: Quality Gate Activation
**Goal**: All three dormant quality hooks are active in settings.json and firing on their respective events
**Depends on**: Phase 18 (hooks.json must be gone before modifying settings.json to avoid confusion)
**Requirements**: GATE-01, GATE-02, GATE-03
**Success Criteria** (what must be TRUE):
  1. SubagentStop hook fires after every subagent completion and validates agent contracts (running SubagentStop.py)
  2. Research frontmatter is validated on every Write/Edit to `research/*.md` files (validate-research-frontmatter.py active in settings.json PostToolUse)
  3. Resource index is automatically regenerated when research files are written (regenerate-manifest.py active in settings.json PostToolUse)
**Plans**: TBD

### Phase 20: Research Governance & Review
**Goal**: The research corpus has complete indexing, consistent frontmatter across all documents, identified coverage gaps filled with new docs, and stale content flagged for refresh
**Depends on**: Phase 19 (frontmatter validation and index regeneration hooks must be active so new/updated docs are automatically validated going forward)
**Requirements**: RSRC-01, RSRC-02, RSRC-03, RSCH-01, RSCH-02, RSCH-03
**Success Criteria** (what must be TRUE):
  1. `resource-index.json` contains entries for all research documents (not just 27 of 54)
  2. Every research document in `research/` has valid YAML frontmatter with at minimum: title, created, domain, type, and keywords fields
  3. `frontmatter-issues.txt` is removed (validator hook handles this going forward)
  4. A coverage audit document exists identifying which domains have research docs and which gaps were found
  5. New research documents exist for identified coverage gaps, and stale documents are flagged with frontmatter noting they need refresh
**Plans**: TBD

### Phase 21: Skill & Infrastructure Consolidation
**Goal**: Overlapping skills are merged, commands reference the correct skill, and infrastructure clutter (empty placeholders, documentation-only files in wrong locations, dev artifacts) is resolved
**Depends on**: Phase 18 (dead agents must be removed before consolidating skills that might reference them)
**Requirements**: SKIL-01, SKIL-02, INFR-01, INFR-02, INFR-03, INFR-04
**Success Criteria** (what must be TRUE):
  1. `plugin-phases` skill directory does not exist -- its content has been merged into `plugin-workflow`
  2. No command or skill file references `plugin-phases` by name (all updated to `plugin-workflow`)
  3. Agent memory files either contain meaningful seed patterns or do not exist (no empty "No patterns recorded yet" placeholders)
  4. `agent-profiles.json` and `preferences-README.md` are relocated out of `.claude/` root (moved to a documentation location or inlined where needed)
  5. Aesthetic test-preview HTML files are excluded from the repo via `.gitignore` or relocated to a dev-only location
**Plans**: TBD

### Phase 22: Structural Improvements
**Goal**: Agents can persist learnings across sessions via a write-back mechanism, and dead infrastructure (validation cache, duplicate canary scripts) is resolved
**Depends on**: Phase 21 (agent memory cleanup must be done before building write-back mechanism on top of it)
**Requirements**: STRC-01, STRC-02, STRC-03
**Success Criteria** (what must be TRUE):
  1. A post-agent hook or equivalent mechanism exists that extracts key learnings from agent sessions and writes them to agent memory files
  2. The validation cache system is either activated (caching validation results to avoid redundant checks) or fully removed (validation-cache.md, validation-cache.sh, empty cache file all deleted)
  3. `canary-test.sh` is verified against `canary-test.py` -- if duplicate functionality, the .sh version is deleted; if unique, it is documented
**Plans**: TBD

## Progress

**Execution Order:** 18 -> 19 -> 20 -> 21 -> 22

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1-7 | v1.0 | 21/21 | Complete | 2026-02-01 |
| 8-9 | v1.1 | 4/4 | Complete | 2026-02-02 |
| 10-13 | v1.2 | 12/12 | Complete | 2026-02-06 |
| 14-17 | v1.3 | 14/14 | Complete | 2026-02-10 |
| 18. Dead Code Removal | v1.4 | 0/3 | Planned | - |
| 19. Quality Gate Activation | v1.4 | 0/? | Not started | - |
| 20. Research Governance & Review | v1.4 | 0/? | Not started | - |
| 21. Skill & Infrastructure Consolidation | v1.4 | 0/? | Not started | - |
| 22. Structural Improvements | v1.4 | 0/? | Not started | - |

**Total: 17 phases complete, 51 plans complete, 4 milestones shipped. v1.4: 5 phases planned (23 requirements).**

---
*Roadmap created: 2026-01-30*
*Last updated: 2026-03-05 -- Phase 18 plans created*
