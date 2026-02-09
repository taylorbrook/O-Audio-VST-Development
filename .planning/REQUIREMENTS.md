# Requirements: Plugin Freedom System v1.3

**Defined:** 2026-02-08
**Core Value:** Reliable collaborative workflow that produces professional-quality plugins — where agents execute quality work that doesn't require constant rework.

## v1.3 Requirements

Requirements for System Modernization (Opus 4.6 + GSD Alignment). Each maps to roadmap phases.

### Platform Alignment

- [ ] **PLAT-01**: System migrates all agents from explicit thinking config to adaptive thinking (Opus 4.6 deprecation)
- [ ] **PLAT-02**: PreCompact.sh references correct `.planning/` paths instead of legacy `.ideas/` paths
- [ ] **PLAT-03**: System has zero assistant message prefills (Opus 4.6 breaking change — returns 400)
- [ ] **PLAT-04**: Agent effort parameter replaces binary Sonnet/Opus model selection with per-agent effort tuning
- [ ] **PLAT-05**: Compaction uses domain-aware custom instructions preserving plugin-specific context (parameter IDs, DSP components, contract paths)
- [ ] **PLAT-06**: Compaction pause injects critical contract data after compaction before response continues
- [ ] **PLAT-07**: dsp-agent and research-planning-agent always use Opus model (model profile refinement)

### GSD Deduplication

- [ ] **GSDD-01**: gsd-tools CLI handles state operations (advance-plan, update-progress, record-metric) replacing manual STATE.md parsing in agents
- [ ] **GSDD-02**: GSD frontmatter operations (get/set/merge/validate) replace custom Python frontmatter validation where schema-compatible
- [ ] **GSDD-03**: GSD verification suite replaces 6 structural validators (plan-structure, phase-completeness, references, commits, artifacts, key-links) while preserving 6 domain validators
- [ ] **GSDD-04**: Post-plan validation cross-references CONTEXT.md user decisions with PLAN.md tasks (context compliance)

### Context Persistence

- [ ] **CTXP-01**: Complex DSP agents (complexity >= 4) can use 1M context window for full research document loading
- [ ] **CTXP-02**: Per-plugin history digest compiles stage decisions into structured JSON for fast cross-stage context loading
- [ ] **CTXP-03**: Express plugin creation uses auto mode (auto-discuss, auto-research, auto-plan) instead of skipping phases
- [ ] **CTXP-04**: Five key agents (troubleshoot, dsp, gui, research-planning, validation) have persistent memory across sessions

### Agent Intelligence

- [ ] **AGNT-01**: Agent Teams available for parallel research exploration (read-heavy, no file conflicts)
- [ ] **AGNT-02**: Agent Teams available for cross-stage review with debate (read-only analysis)
- [ ] **AGNT-03**: Plan approval gates allow team lead to review/reject teammate plans before implementation
- [ ] **AGNT-04**: Delegate mode restricts orchestrator to coordination-only tools (no implementation)
- [ ] **AGNT-05**: TaskCompleted hooks enable per-task validation within plans (exit code 2 prevents completion)
- [ ] **AGNT-06**: Configurable branching strategy (none/phase/milestone with optional squash merge)
- [ ] **AGNT-07**: Template variants auto-select summary complexity based on task complexity (minimal/standard/complex)

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Cross-Plugin Intelligence

- **XPLG-01**: Cross-plugin knowledge transfer references successful implementations from other plugins
- **XPLG-02**: Decision provenance chains trace decisions to source research documents

### Infrastructure

- **INFR-01**: Windows installer automation (NSIS via GitHub Actions)
- **INFR-02**: CI/CD integration for plugin validation on commit
- **INFR-03**: Automatic rebuild propagation when modules update

## Out of Scope

| Feature | Reason |
|---------|--------|
| Agent Teams for implementation stages | File conflicts — implementation modifies overlapping files (PluginProcessor.cpp, PluginEditor.cpp). Teams only for read-heavy work. |
| Replacing all 12 PFS validators with GSD | 6 domain validators encode audio expertise (DSP safety, APVTS matching, WebView bindings) that GSD cannot know about |
| 1M context for all agents | 2x cost ($10/$37.50 vs $5/$25 per MTok). Most agents work fine within 200K. |
| Nested Agent Teams | Not supported — teammates cannot spawn their own teams |
| Custom agent monitoring dashboard | Claude Code provides built-in teammate monitoring (Shift+Up/Down, tmux split) |
| Fast mode for all agents | 6x cost increase ($30/$150 per MTok) with negligible latency improvement for short-running agents |
| Removing PreCompact hook entirely | Even with domain-aware compaction, hook serves as deterministic safety net |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| PLAT-01 | Phase 14 | Pending |
| PLAT-02 | Phase 14 | Pending |
| PLAT-03 | Phase 14 | Pending |
| PLAT-04 | Phase 14 | Pending |
| PLAT-05 | Phase 15 | Pending |
| PLAT-06 | Phase 15 | Pending |
| PLAT-07 | Phase 14 | Pending |
| GSDD-01 | Phase 16 | Pending |
| GSDD-02 | Phase 16 | Pending |
| GSDD-03 | Phase 16 | Pending |
| GSDD-04 | Phase 16 | Pending |
| CTXP-01 | Phase 15 | Pending |
| CTXP-02 | Phase 15 | Pending |
| CTXP-03 | Phase 15 | Pending |
| CTXP-04 | Phase 15 | Pending |
| AGNT-01 | Phase 17 | Pending |
| AGNT-02 | Phase 17 | Pending |
| AGNT-03 | Phase 17 | Pending |
| AGNT-04 | Phase 17 | Pending |
| AGNT-05 | Phase 17 | Pending |
| AGNT-06 | Phase 17 | Pending |
| AGNT-07 | Phase 17 | Pending |

**Coverage:**
- v1.3 requirements: 22 total
- Mapped to phases: 22
- Unmapped: 0

---
*Requirements defined: 2026-02-08*
*Last updated: 2026-02-08 — traceability updated with phase mappings*
