# Requirements: Plugin Freedom System v1.4

**Defined:** 2026-03-05
**Core Value:** Reliable collaborative workflow that produces professional-quality plugins — where agents execute quality work that doesn't require constant rework.
**Source:** Quick task 14 — Full system review (SYSTEM-REVIEW.md)

## v1.4 Requirements

Requirements for this milestone. Each maps to roadmap phases.

### Dead Code Removal

- [ ] **DEAD-01**: All 10 dead `.sh` hook files are deleted (816 lines)
- [ ] **DEAD-02**: Vestigial `hooks.json` is deleted and `settings.json` is documented as sole authoritative hook config
- [ ] **DEAD-03**: 3 unreferenced agent definitions are removed (aesthetics-agent, dynamic-researcher, music-theory-agent — 473 lines)
- [ ] **DEAD-04**: Deprecated `.claude/plugin-registry.json` is removed
- [ ] **DEAD-05**: `__pycache__` directories are removed from tracking and added to `.gitignore`

### Quality Gates

- [ ] **GATE-01**: SubagentStop hook is activated in `settings.json` (contract validation after subagent completion)
- [ ] **GATE-02**: Research frontmatter validation hook is activated in `settings.json`
- [ ] **GATE-03**: Resource index auto-regeneration hook is activated in `settings.json`

### Research Governance

- [ ] **RSRC-01**: `resource-index.json` is regenerated to cover all research docs (currently 27 of 54)
- [ ] **RSRC-02**: All research docs have standardized YAML frontmatter (currently ~50% lack it)
- [ ] **RSRC-03**: `frontmatter-issues.txt` is cleaned up or removed

### Research Review

- [ ] **RSCH-01**: Audit all research docs for topical coverage gaps — identify domains with missing or thin documentation
- [ ] **RSCH-02**: Create new research documents to fill identified gaps (e.g., missing DSP topics, underserved JUCE API areas, cross-platform patterns)
- [ ] **RSCH-03**: Flag and refresh stale or outdated research documents (pre-JUCE 8 content, deprecated APIs, superseded techniques)

### Skill Consolidation

- [ ] **SKIL-01**: `plugin-phases` skill is merged into `plugin-workflow` skill, eliminating overlap
- [ ] **SKIL-02**: Commands referencing `plugin-phases` are updated to reference `plugin-workflow`

### Infrastructure Cleanup

- [ ] **INFR-01**: Empty agent memory placeholder files are either populated with seed patterns or removed
- [ ] **INFR-02**: `agent-profiles.json` (no runtime effect) is moved out of `.claude/` root or documented inline
- [ ] **INFR-03**: `preferences-README.md` (453 lines) is relocated from `.claude/` root
- [ ] **INFR-04**: Aesthetic test-preview HTML files are excluded from the repo or relocated

### Structural Improvements

- [ ] **STRC-01**: Agent memory write-back mechanism exists (post-agent hook or equivalent that records learnings)
- [ ] **STRC-02**: Validation cache system is either activated or dead infrastructure removed
- [ ] **STRC-03**: `canary-test.sh` dead code is verified and removed if duplicate of `.py` version

## Future Requirements

Deferred to future milestones. Tracked but not in current roadmap.

### Skill Namespace

- **SKNS-01**: Reorganize 10 `plugin-*` skills into 3-4 clearer domains (create, build, ship, maintain)

### Module System

- **MODS-01**: Evaluate module-* command usage frequency and simplify if adoption is low

### Schema Enforcement

- **SCHM-01**: Add automated JSON schema validation (currently documentation-only)

### Command Consolidation

- **CMDS-01**: Merge 4 minimal plugin-* commands (list, pause, resume, status) into single `plugin-info` command

## Out of Scope

| Feature | Reason |
|---------|--------|
| Building new plugins | Focus is on system improvement, not plugin creation |
| Changing JUCE version | Staying on JUCE 8 |
| Rewriting agents from scratch | Iterative improvement of existing definitions |
| Adding new agent types | Focus is on cleaning up and activating existing infrastructure |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| DEAD-01 | — | Pending |
| DEAD-02 | — | Pending |
| DEAD-03 | — | Pending |
| DEAD-04 | — | Pending |
| DEAD-05 | — | Pending |
| GATE-01 | — | Pending |
| GATE-02 | — | Pending |
| GATE-03 | — | Pending |
| RSRC-01 | — | Pending |
| RSRC-02 | — | Pending |
| RSRC-03 | — | Pending |
| RSCH-01 | — | Pending |
| RSCH-02 | — | Pending |
| RSCH-03 | — | Pending |
| SKIL-01 | — | Pending |
| SKIL-02 | — | Pending |
| INFR-01 | — | Pending |
| INFR-02 | — | Pending |
| INFR-03 | — | Pending |
| INFR-04 | — | Pending |
| STRC-01 | — | Pending |
| STRC-02 | — | Pending |
| STRC-03 | — | Pending |

**Coverage:**
- v1.4 requirements: 23 total
- Mapped to phases: 0
- Unmapped: 23

---
*Requirements defined: 2026-03-05*
*Last updated: 2026-03-05 after initial definition*
