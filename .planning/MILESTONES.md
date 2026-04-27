# Project Milestones: Plugin Freedom System

## v1.5 Microtonal Shared Module & Suite Propagation (Shipped: 2026-04-27)

**Delivered:** Promoted the validated VST3 Note Expression pattern from O-Lyrica spikes into a shared `note-expression` module and propagated Dorico microtonal playback across all 8 pitched plugins (O-Lyrica, O-Bells, O-Prism, O-Wind, O-IntonationPad, O-Reed, O-Bowed, O-Formant). Authored the canonical Dorico `.doricolib` expression-map asset, bundled it cross-platform (PKG + EXE) with one-time Library Manager Import activation, and consolidated internal developer-reference notes for future website authoring.

**Phases completed:** 23-25 (16 plans)

**Key accomplishments:**

- Extracted shared `note-expression` module from O-Lyrica spike code — header-only voice helper (`applyPendingTuning`), NEC + VST3Extensions raw-event queue, JUCE 8.0.4 patch tooling with idempotent re-apply script and CMake-time marker check
- Resolved AU re-link via two-TU split (`NoteExpression.cpp` SharedCode + `vst3/NoteExpression_VST3.cpp` VST3-only) + custom-deleter pimpl + reusable `scripts/verify-au-link.sh` AU verification gate; codified per-format module-source convention in `OuariconModules.cmake`
- Propagated module to 7 cohort plugins via `/improve` workflow — version bumps, CHANGELOG entries, STATUS updates, fresh installs; full Dorico 3-point smoke gate PASS on all 8 plugins (quarter-sharp +50¢, no attack zipper, polyphonic noteId correlation)
- Authored canonical `Ouaricon-VST3-NoteExpression.doricolib` (6,431 B, 48-container kScoreLibrary skeleton + injected ExpressionMapDefinition); pivoted v1→v2→v3 from `.doricoexpmap` drop → `.dorico_pt` Playback Template → standalone `.doricolib` + Library Manager Import (Path B locked) after empirical validation
- Cross-platform installer bundling for all 8 cohort plugins (PKG on macOS + EXE on Windows) with D-08 STRICT-PASS validation; lands at `~/Library/Application Support/Ouaricon/Microtonal Suite/` and `%APPDATA%\Ouaricon\Microtonal Suite\`
- Consolidated internal developer-reference notes at `research/microtonal-dorico-integration.md` (4 H2 sections, 554 lines, `audience: internal-dev-only`) covering module architecture, canonical Dorico setup procedure, host-side behavior quirks, troubleshooting signatures

**Stats:**

- 707 files changed
- +145,617 / −1,747 (heavy due to JUCE patch tooling, factory-skeleton bootstrapping, research consolidation, and Path A artifact churn during v1→v2→v3 evolution)
- 3 phases, 16 plans, 245 commits
- 33 requirements satisfied (100% coverage)
- 4 days from start to ship (2026-04-24 → 2026-04-27)

**Git range:** `4ce5b13` → `6db3a79`

**Key decisions:**

- `/improve [PluginName]` workflow as single propagation mechanism for Phase 24 — no direct source edits across 7 production plugins
- Module-format split convention: `cpp/` SharedCode-bound, `cpp/vst3/` VST3-only, `cpp/au/` AU-only — codified after AU re-link surfaced VST3 SDK symbol leakage
- Path B over Path A in Phase 25 v3: ship single `.doricolib` + one-time Library Manager Import instead of `.dorico_pt` Playback Template with auto-discovery
- CID-free `<pluginNames />` element in canonical `.doricolib` — endpoint-binding deferred to user via Dorico's `Play → Endpoints → Expression Map` dropdown
- D-10 amend-forward strategy for v3 cleanup — surgical deletion of Path A artifacts (playback-template/ subtree, `ouaricon_extract_vst3_cids` helper, `.dorico_pt` packing, dual-write logic) preserves git history clarity

**Tech debt / carry-forward:**

- BL-01 Windows multi-account UAC test deferred to UAT (persisted in `25-HUMAN-UAT.md`) — D-08 canary ran on single-account dev box
- DEF-24-01 O-Lyrica `auval -v` exit 255 on APVTS Meta Param Flag invariant — pre-existing parameter-implementation issue, NOT a note-expression module regression
- Wave 0 v3 auto-discovery probe FAIL logged as informational/non-blocking — Path B explicit-import flow is the primary supported activation path
- `.dorico_pt` Playback Template machinery unbuilt this milestone (deferred per Path B decision); usable foundation if Path A revisited in v1.6+

**What's next:** `/gsd-new-milestone` for v1.6

---

## v1.4 System Hygiene & Quality Gates (Shipped: 2026-03-07)

**Delivered:** Complete system hygiene pass driven by full system review -- dead code purged, quality gates activated, research corpus standardized and expanded, skills consolidated, and agent memory write-back mechanism built for persistent learning

**Phases completed:** 18-22 (13 plans, 24 tasks)

**Key accomplishments:**

- Purged all dead code: 10 .sh hooks (816 lines), hooks.json, 3 dead agents (473 lines), deprecated registry, __pycache__ from git tracking
- Activated 3 dormant quality gates in settings.json: SubagentStop contract validation, research frontmatter validation, resource index auto-regeneration
- Standardized all 53 research docs with 5-field YAML frontmatter, grew corpus to 64 docs with 10 gap-fill deep-dives covering all plugin technique domains
- Merged plugin-phases into plugin-workflow skill, populated 4 agent memory files with seed patterns from production experience
- Built agent memory write-back hook: agents now persist learnings across sessions via SubagentStop with deduplication and 10KB cap
- Removed validation cache dead infrastructure (5 files) and duplicate canary-test.sh

**Stats:**

- 328 files changed
- 20,873 lines added, 50,581 lines deleted (-29,708 net)
- 5 phases, 13 plans, 24 tasks
- 23 requirements satisfied (100% coverage)
- 2 days from start to ship (2026-03-05 -> 2026-03-07)

**Git range:** `5bc7a2e` -> `02f78a0`

**Tech debt (4 items):**
- SubagentStop.py subprocess timeout (60s) exceeds settings.json hook timeout (30s) -- non-blocking
- Stop.py and UserPromptSubmit.py are orphaned hooks (not wired in settings.json)
- write-back-agent-memory.py has intentional pass in JUCE investigation branch
- End-to-end write-back test requires real subagent sessions (human-verification)

**What's next:** `/gsd:new-milestone` for next version

---

## v1.3 System Modernization (Shipped: 2026-02-10)

**Delivered:** Opus 4.6 platform migration, multi-layer context persistence, GSD state deduplication, and agent team infrastructure for parallel research and review — with canary validation protecting 35+ production plugins

**Phases completed:** 14-17 (14 plans total)

**Key accomplishments:**

- Migrated all 13 agents from deprecated model/thinking configs to effort-level tuning (low/medium/max) on Opus 4.6
- Built multi-layer context persistence: compaction snapshots, per-plugin DIGEST.json with complexity-based research routing, persistent agent memory via SubagentStart hooks
- Migrated all STATE.md mutations across 8 workflows to gsd-tools CLI, eliminating manual file editing
- Shipped research team orchestration with debate-format conflict resolution and dynamic domain assignment
- Created parallel critic review system with severity-ranked unified reporting and approval gates
- Implemented context compliance verification with tokenized keyword matching and configurable thresholds
- Connected research teams, critics, branching strategies, and template auto-selection into plugin-workflow

**Stats:**

- 289 files created/modified
- 12,988 lines added
- 4 phases, 14 plans
- 22 requirements satisfied (100% coverage)
- 3 days from start to ship (2026-02-08 → 2026-02-10)
- 48 commits in milestone range

**Git range:** `8ae62f4` → `b261f61`

**What's next:** Cross-plugin intelligence, decision provenance, or Windows distribution (v2)

---

## v1.2 Agent Intelligence & Resource Orchestration (Shipped: 2026-02-06)

**Delivered:** Resource discovery, context injection, accountability tracking, and self-maintaining manifest system — agents now automatically receive relevant research with full traceability and graceful degradation

**Phases completed:** 10-13 (12 plans total)

**Key accomplishments:**

- Built resource discovery engine with weighted scoring (stage/role/keyword) completing in 63ms
- Created context injection pipeline delivering relevant research to all agents within 4,000 token budget
- Implemented resource accountability tracking across all 11 agents with warning-level validation
- Established self-maintaining manifest with auto-regeneration hooks and staleness detection (90-day threshold)
- Retrofitted 27 research documents with 10-field YAML frontmatter including temporal tracking
- Achieved graceful degradation — agents proceed normally if any component is missing or fails

**Stats:**

- 102 files created/modified
- 8,806 lines added
- 4 phases, 12 plans
- 15 requirements satisfied (100% coverage)
- 2 days from start to ship
- 40 commits in milestone range

**Git range:** `6fc6330` → `3e86dfc`

**What's next:** Cross-plugin intelligence, decision provenance, or Windows distribution (v1.3)

---

## v1.1 Cleanup & Workflow Polish (Shipped: 2026-02-02)

**Delivered:** Repository cleanup (91% size reduction) and structured planning workflow for complex plugin improvements

**Phases completed:** 8-9 (4 plans total)

**Key accomplishments:**

- Reduced repository from 636MB to 58MB via git-filter-repo (91% reduction)
- Reduced clone time from ~2 minutes to 4 seconds (97% faster)
- Created 173-pattern .gitignore preventing future artifact accumulation
- Added Phase 0.6 Implementation Planning to plugin-improve workflow
- Integrated conditional planning triggers for Tier 2/3 improvements
- Documented backup recovery procedures

**Stats:**

- 104 files created/modified
- 2 phases, 4 plans
- 2 days from start to ship
- 28 commits in milestone range

**Git range:** `80ad126` → `1f35d17`

**What's next:** Windows distribution automation (v1.2)

---

## v1.0 MVP (Shipped: 2026-02-01)

**Delivered:** Complete Plugin Freedom System overhaul with contract-driven agents, quality gates, and module system

**Phases completed:** 1-7 (21 plans total)

**Key accomplishments:**

- 9 specialized agents with validated JSON Schema contracts
- Registry-based state management with checkpoints and recovery
- Structured handoffs with schema validation at stage boundaries
- Generator-critic loops for iterative quality improvement
- Quality gates blocking stage progression until criteria pass
- Domain expertise encoding (DSP real-time safety, UI thread safety)
- Reliable module system with semver and dependency tracking

**Stats:**

- 35 requirements satisfied (100% coverage)
- 7 phases, 21 plans
- 2 days from start to ship

**Git range:** `f4279c2` → `8059963` (approximate)

**What's next:** Repository cleanup and workflow polish (v1.1) ✓

---

*Milestones archive created: 2026-02-02*


