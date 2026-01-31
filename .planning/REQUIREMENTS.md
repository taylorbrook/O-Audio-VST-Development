# Requirements: Plugin Freedom System Overhaul

**Defined:** 2026-01-29
**Core Value:** Reliable collaborative workflow producing professional-quality plugins

## v1 Requirements

Requirements for the system overhaul. Each maps to roadmap phases.

### Agent Contracts

- [x] **AGNT-01**: Each agent has explicit JSON schema defining required inputs
- [x] **AGNT-02**: Each agent has explicit JSON schema defining expected outputs
- [x] **AGNT-03**: Validation runs before agent invocation, rejecting invalid inputs
- [x] **AGNT-04**: Each agent has documented scope boundaries (does/doesn't do)
- [x] **AGNT-05**: Each agent has defined tool inventory (10-20 tools max)
- [x] **AGNT-06**: Audit existing 9 agents for gaps and overlaps
- [x] **AGNT-07**: Identify and spec missing agents (music theory, aesthetics, etc.)

### State Management

- [x] **STAT-01**: All workflow state persisted to files (GSD pattern)
- [x] **STAT-02**: Session resume restores full context from last checkpoint
- [x] **STAT-03**: State validation detects inconsistencies between STATUS.md and registry
- [x] **STAT-04**: Recovery mechanism auto-repairs corrupted state
- [x] **STAT-05**: Clear hand-off instructions at context boundaries with next slash command
- [x] **STAT-06**: Plugin state isolation — each plugin's planning/progress/changelog under `plugins/{Name}/.planning/`, `/focus` loads only that plugin's state

### Structured Handoffs

- [x] **HAND-01**: Schema-validated handoff documents between stages
- [x] **HAND-02**: Explicit artifact requirements for each stage boundary
- [x] **HAND-03**: Decision audit trail tracking why choices were made
- [x] **HAND-04**: Versioned handoff formats (semver for schema evolution)
- [x] **HAND-05**: Context clear messages with copy-paste slash command for continuation

### Quality Gates

- [ ] **GATE-01**: Automated verification runs at each stage boundary
- [ ] **GATE-02**: Measurable success criteria defined for each stage
- [ ] **GATE-03**: Progression blocked until gate passes
- [ ] **GATE-04**: Tiered verification (smoke test vs full validation by complexity)
- [ ] **GATE-05**: Code review integrated at end of implementation phases
- [ ] **GATE-06**: Code simplification pass after implementation phases

### Generator-Critic Loops

- [ ] **CRIT-01**: Critic agent validates outputs before stage handoff
- [ ] **CRIT-02**: Iterative refinement loop until quality threshold met
- [ ] **CRIT-03**: Domain-specific critics (DSP critic knows real-time rules, UI critic knows polish)
- [ ] **CRIT-04**: Token budget awareness stops iteration if budget exceeded

### Domain Expertise Encoding

- [ ] **DOMN-01**: Real-time safety rules encoded in DSP agent (no allocation in processBlock)
- [ ] **DOMN-02**: Thread-safety patterns encoded in GUI agent (APVTS atomics, relay lifecycle)
- [ ] **DOMN-03**: JUCE 8 best practices baked into relevant agents
- [ ] **DOMN-04**: Professional quality standards defined (what makes commercial plugins pro)
- [ ] **DOMN-05**: Music theory/tuning agent spec for domain-aware assistance
- [ ] **DOMN-06**: Aesthetics agent spec for design-aware UI guidance

### Module System

- [ ] **MODS-01**: Module add/remove works reliably across plugins
- [ ] **MODS-02**: Dependency tracking knows which plugins use which modules
- [ ] **MODS-03**: Version management with semver and compatibility checks
- [ ] **MODS-04**: Documentation for manual rebuild integration after module updates

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Automation

- **AUTO-01**: Automatic rebuild propagation when modules update
- **AUTO-02**: CI/CD integration for plugin validation on commit
- **AUTO-03**: Automated performance benchmarking for DSP agents

### Advanced Features

- **ADVN-01**: Multi-plugin project orchestration (build related plugins together)
- **ADVN-02**: A/B comparison testing for DSP quality validation
- **ADVN-03**: Cross-DAW compatibility matrix testing

### Observability

- **OBSV-01**: Full tracing for agent invocations and decisions
- **OBSV-02**: Token usage tracking and cost reporting
- **OBSV-03**: Quality metrics dashboard

## Out of Scope

| Feature | Reason |
|---------|--------|
| Changing JUCE version | JUCE 8 is proven and working |
| Abandoning GSD phases | The discuss->research->plan->execute->verify model is valuable |
| Building plugins during overhaul | Focus is on system improvement |
| Automatic cross-DAW testing | Requires infrastructure not yet available |
| Real-time collaboration | Single-user workflow is sufficient |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| AGNT-01 | Phase 1 | Complete |
| AGNT-02 | Phase 1 | Complete |
| AGNT-03 | Phase 1 | Complete |
| AGNT-04 | Phase 1 | Complete |
| AGNT-05 | Phase 1 | Complete |
| AGNT-06 | Phase 1 | Complete |
| AGNT-07 | Phase 1 | Complete |
| STAT-01 | Phase 2 | Complete |
| STAT-02 | Phase 2 | Complete |
| STAT-03 | Phase 2 | Complete |
| STAT-04 | Phase 2 | Complete |
| STAT-05 | Phase 2 | Complete |
| STAT-06 | Phase 2 | Complete |
| HAND-01 | Phase 3 | Complete |
| HAND-02 | Phase 3 | Complete |
| HAND-03 | Phase 3 | Complete |
| HAND-04 | Phase 3 | Complete |
| HAND-05 | Phase 3 | Complete |
| CRIT-01 | Phase 4 | Pending |
| CRIT-02 | Phase 4 | Pending |
| CRIT-03 | Phase 4 | Pending |
| CRIT-04 | Phase 4 | Pending |
| GATE-01 | Phase 5 | Pending |
| GATE-02 | Phase 5 | Pending |
| GATE-03 | Phase 5 | Pending |
| GATE-04 | Phase 5 | Pending |
| GATE-05 | Phase 5 | Pending |
| GATE-06 | Phase 5 | Pending |
| DOMN-01 | Phase 6 | Pending |
| DOMN-02 | Phase 6 | Pending |
| DOMN-03 | Phase 6 | Pending |
| DOMN-04 | Phase 6 | Pending |
| DOMN-05 | Phase 6 | Pending |
| DOMN-06 | Phase 6 | Pending |
| MODS-01 | Phase 7 | Pending |
| MODS-02 | Phase 7 | Pending |
| MODS-03 | Phase 7 | Pending |
| MODS-04 | Phase 7 | Pending |

**Coverage:**
- v1 requirements: 35 total
- Mapped to phases: 35
- Unmapped: 0

---
*Requirements defined: 2026-01-29*
*Last updated: 2026-01-31 — Phases 2-3 complete (STAT-01 to STAT-06, HAND-01 to HAND-05)*
