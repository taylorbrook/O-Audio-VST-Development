# Plugin Freedom System Overhaul

## What This Is

A comprehensive overhaul of Ouaricon Audio's JUCE plugin development system — the agents, workflows, skills, state tracking, and module system that enable collaborative creation of professional audio plugins. This is a meta-project about improving the tooling, not building a specific plugin.

## Core Value

Reliable collaborative workflow that produces professional-quality plugins — where the user guides key decisions and agents execute quality work that doesn't require constant rework.

## Current State

**Latest shipped:** v1.4 System Hygiene & Quality Gates (2026-03-07)
**In flight:** v1.5 Microtonal Shared Module & Suite Propagation — Phase 23 (extract) complete 2026-04-25; Phase 24 (propagate to 8 cohort plugins) complete 2026-04-26; Phase 25 (package & internal docs, Path B locked) complete 2026-04-27 with 1 human-UAT carry-forward (Windows multi-account UAC test). v1.5 ready for ship audit.

**System overview:**
- 23 phases completed across 5 milestones (Phase 23 closed out the v1.5 extract milestone)
- 64 plans executed
- 108 requirements satisfied
- Repository: 58MB (91% reduction from 636MB)
- Clone time: 4 seconds
- 64 research documents indexed with 5-field minimum YAML frontmatter
- All agents running on Opus 4.6 with effort-level tuning
- Multi-layer context persistence (compaction snapshots, DIGEST.json, agent memory with write-back)
- Agent teams available for parallel research and review
- gsd-tools CLI handles all state operations
- 3 active quality gates: SubagentStop contract validation, research frontmatter validation, resource index auto-regeneration
- Agent memory write-back: learnings persist across sessions with deduplication and 10KB cap

## Requirements

### Validated

- ✓ JUCE 8 audio framework integration — existing
- ✓ GSD hybrid workflow (discuss→research→plan→execute→verify) — existing
- ✓ WebView-based UI architecture — existing
- ✓ Module system for code reuse — v1.0 (reliable with tracking)
- ✓ 13 specialized agents with contracts — v1.0
- ✓ State tracking via STATUS.md and registry — v1.0 (reconciliation working)
- ✓ Structured handoffs with schema validation — v1.0
- ✓ Quality gates at stage boundaries — v1.0
- ✓ Generator-critic loops — v1.0
- ✓ Domain expertise encoding (DSP, UI) — v1.0
- ✓ Repository cleanup (636MB → 58MB) — v1.1
- ✓ Comprehensive .gitignore (173 patterns) — v1.1
- ✓ Backup and recovery procedures — v1.1
- ✓ Phase 0.6 Implementation Planning workflow — v1.1
- ✓ Conditional planning for Tier 2/3 improvements — v1.1
- ✓ Express mode bypass (--no-plan flag) — v1.1
- ✓ Resource discovery system (weighted scoring, 63ms) — v1.2
- ✓ Context injection pipeline (auto-inject within 4K token budget) — v1.2
- ✓ Agent usage accountability (resources_consulted field) — v1.2
- ✓ Self-maintaining manifest (auto-regeneration, 90-day staleness) — v1.2
- ✓ 27 research documents with 10-field YAML frontmatter — v1.2
- ✓ Graceful degradation (agents proceed if discovery fails) — v1.2
- ✓ Opus 4.6 platform migration (adaptive thinking, effort-level tuning) — v1.3
- ✓ Agent teams for parallel research (debate-format conflict resolution) — v1.3
- ✓ Agent teams for cross-stage review (parallel critics, unified reporting) — v1.3
- ✓ Context persistence engine (compaction snapshots, DIGEST.json, agent memory) — v1.3
- ✓ GSD deduplication (STATE.md ops via gsd-tools CLI) — v1.3
- ✓ Context compliance verification (decision-to-task cross-referencing) — v1.3
- ✓ Plan approval gates (auto-approve/gate/escalate) — v1.3
- ✓ TaskCompleted hooks (per-task validation with 7 domain validators) — v1.3
- ✓ Configurable branching strategy (none/phase/milestone) — v1.3
- ✓ Template auto-selection (minimal/standard/complex based on task complexity) — v1.3
- ✓ Auto mode for express plugin creation (--auto flag) — v1.3
- ✓ Dead code removal (10 .sh hooks, hooks.json, 3 dead agents, deprecated registry) — v1.4
- ✓ Quality gate activation (SubagentStop, frontmatter validation, resource index regen) — v1.4
- ✓ Research governance (64 docs with standardized frontmatter, 10 gap-fill deep-dives) — v1.4
- ✓ Skill consolidation (plugin-phases merged into plugin-workflow) — v1.4
- ✓ Agent memory seed patterns (4 agents populated with production experience) — v1.4
- ✓ Agent memory write-back mechanism (persistent learning via SubagentStop) — v1.4
- ✓ Infrastructure cleanup (validation cache removed, canary-test deduplicated, doc files relocated) — v1.4
- ✓ Shared `note-expression` module extracted with header-only public API — v1.5 Phase 23
- ✓ JUCE patch tooling (named .patch artefact + idempotent apply script + module.cmake marker check) — v1.5 Phase 23
- ✓ O-Lyrica refactored as reference consumer (zero plugin-local NE code; TuningEngine composition preserved) — v1.5 Phase 23
- ✓ Per-format module-source routing convention (`cpp/<format>/` → `${TARGET}_<FORMAT>`) — v1.5 Phase 23
- ✓ Reusable `scripts/verify-au-link.sh` AU verification gate — v1.5 Phase 23

### Active

## Current Milestone: v1.5 Microtonal Shared Module & Suite Propagation

**Goal:** Promote the validated VST3 Note Expression pattern (O-Lyrica spikes 001–003) into a shared Ouaricon module and propagate Dorico microtonal playback across all pitched plugins, with every per-plugin rollout tracked through the standard `/improve` workflow.

**Target features:**
- Shared microtonal module (candidate name `dsp/note-expression` — confirm via `/module-list`) extracted from cleaned spike code
- O-Lyrica refactored as reference consumer, integrating the module with its existing `TuningEngine`
- Module propagated to 7 pitched plugins via `/improve` — each with version bump, CHANGELOG entry, STATUS.md update, regression test
- Pre-configured `.doricoexpmap` file bundled in every affected plugin's installer
- Internal technical notes (developer reference, not end-user manuals) as source material for future website manual/quickstart authoring

**Key context:**
- Implementation bible: auto-loaded `spike-findings-VST-development` skill (synthesized from spikes 001–003, 2026-04-22/23)
- Seed `microtonal-shared-module.md` triggered — O-Lyrica spike validated
- Correlate NE by `noteId`, never by MIDI pitch (Dorico represents quarter-sharp C4 as `pitch=C#4, NE=-50¢`)
- Apply per-voice tuning to `currentFrequency` before DSP `trigger(...)` to avoid attack-zipper
- Per-plugin rollout uses `/improve` — no raw edits, all tracked with version + changelog
- Local JUCE patch lives in-repo, re-applied on JUCE updates

### Deferred (v2+)

- [ ] Cross-plugin knowledge transfer (reference successful implementations)
- [ ] Decision provenance chains (trace decisions to source research docs)
- [ ] Module-research cross-referencing
- [ ] Windows installer automation (NSIS via GitHub Actions)
- [ ] Add/Remove Programs integration
- [ ] Plugin naming standardization
- [ ] CI/CD integration for plugin validation on commit
- [ ] Automatic rebuild propagation when modules update
- [ ] Automated performance benchmarking for DSP

### Out of Scope

- Building new plugins during this project — focus is on system improvement
- Changing JUCE version — staying on JUCE 8
- Abandoning GSD hybrid approach — the phase model is valuable
- Rewriting from scratch — iterative improvement of existing system
- Planning for Tier 1 fixes — adds overhead without value
- Git LFS migration — not needed after cleanup
- Agent teams for implementation stages — file conflicts with overlapping files
- 1M context for all agents — 2x cost, most agents work within 200K
- Nested agent teams — not supported by Claude Code

## Context

### Current System

The Plugin Freedom System is a JUCE 8-based audio plugin development framework with:

**Stages (5):**
- 0-ideation: Creative brief and parameter specification
- 1-foundation: CMake setup, APVTS parameters, shell plugin
- 2-dsp: Audio processing implementation
- 3-gui: WebView UI integration
- 4-polish: Testing, validation, finishing

**Phases per stage (6):**
- discuss: Gather context and clarify approach
- research: Investigate implementation patterns (parallel teams for complex plugins)
- plan: Create task breakdown (Phase 0.6 for Tier 2/3)
- execute: Run stage-specific agent (TaskCompleted hooks validate each task)
- verify: Validate goal achievement
- handoff: Stage transition with schema validation

**Agents (13 core + 3 team agents):**
- foundation-shell-agent: Stage 1 implementation
- dsp-agent: Stage 2 DSP implementation (effort: max)
- gui-agent: Stage 3 UI integration
- ui-design-agent: WebView mockup creation
- ui-finalization-agent: WebView implementation files
- research-planning-agent: Stage 0 architecture/planning (effort: max)
- validation-agent: Build and validation checks
- troubleshoot-agent: Deep research for build failures
- polish-agent: Stage 4 finishing
- critic agents: DSP/UI/Architecture quality validation
- research-lead: Orchestrates parallel research teams
- dynamic-researcher: Spawned for domain-specific parallel research
- critic-orchestrator: Manages parallel critic review with unified reporting

**Module System:**
- Reusable components: preset-manager, webview-relay-manager, tuning engines, etc.
- CMake integration via OuariconModules.cmake
- Commands: /modules (list, add, remove, create, upgrade, info)
- Semver versioning with customization detection

### Progress Summary

**v1.0 — Foundation (Phases 1-7):**
- 13 specialized agents with JSON Schema contracts
- State tracking with checkpoints and recovery
- Quality gates and structured handoffs
- Module system with semver and dependency tracking

**v1.1 — Cleanup (Phases 8-9):**
- Repository 636MB → 58MB (91% reduction)
- Planning workflow for complex improvements

**v1.2 — Intelligence (Phases 10-13):**
- Resource discovery with weighted scoring (63ms)
- Auto-injection of research context to agents (4K token budget)
- Accountability tracking across 11 agents
- Self-maintaining manifest with staleness detection

**v1.3 — System Modernization (Phases 14-17):**
- Opus 4.6 platform migration (effort-level tuning, adaptive thinking)
- Multi-layer context persistence (compaction snapshots, DIGEST.json, agent memory)
- GSD state deduplication (8 workflows migrated to gsd-tools CLI)
- Agent teams for parallel research and critic review
- Context compliance verification and TaskCompleted hooks

**v1.4 — System Hygiene & Quality Gates (Phases 18-22):**
- Dead code purge: 10 .sh hooks, hooks.json, 3 dead agents, deprecated registry, __pycache__
- 3 quality gates activated: SubagentStop, research frontmatter, resource index regen
- Research corpus standardized (64 docs, 5-field frontmatter) with 10 gap-fill deep-dives
- Skill consolidation (plugin-phases -> plugin-workflow) and agent memory seed patterns
- Agent memory write-back mechanism with deduplication and 10KB cap

### Remaining Concerns

**Deployment:**
- Windows installer automation not yet implemented
- Cross-platform CI/CD incomplete

**Intelligence:**
- No cross-plugin knowledge transfer yet
- Decision provenance chains not implemented

## Constraints

- **Framework**: JUCE 8 — proven foundation, not changing
- **Workflow**: GSD hybrid approach — discuss→research→plan→execute→verify phase model is valuable
- **Platform**: macOS primary, cross-platform via GitHub Actions CI/CD
- **UI Tech**: WebView-based (open to alternatives but working)
- **Tooling**: Claude Code with slash commands and specialized agents
- **Model**: Claude Opus 4.6 — primary model for all agents
- **Agent Teams**: Read-heavy work only (research, review) — no implementation parallelization

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Agents-first approach | Fix the foundation before workflow details | ✓ Good — 13 agents with contracts |
| Full system scope | Holistic improvements vs. piecemeal fixes | ✓ Good — 85+ requirements across 4 milestones |
| Collaborative workflow model | User guides, agents execute | ✓ Good — GSD phases working well |
| JSON Schema draft 2020-12 | Modern schema with strict validation | ✓ Good — adopted in v1.0 |
| git-filter-repo over BFG | Path filtering required for plugin builds | ✓ Good — 91% size reduction |
| Phase 0.6 for Tier 2/3 only | Preserve fast path for simple fixes | ✓ Good — no overhead for Tier 1 |
| Outcome-focused planning tasks | Resilient to code restructuring | ✓ Good — adopted in v1.1 |
| Static manifest + keyword matching | Faster, simpler, more reliable than vector search for 27 docs | ✓ Good — 63ms discovery |
| Warning-level accountability | False positives would block valid work | ✓ Good — no workflow interruption |
| Orchestrator-level discovery | Avoids hook timeout constraints | ✓ Good — integrated in 8 skill files |
| 4,000 token budget cap | Prevent context window exhaustion | ✓ Good — max observed 3,478 tokens |
| Co-located schema at .claude/ | Discovery system is a .claude concern, not .planning/ | ✓ Good — clean separation |
| PostToolUse hook for manifest | Auto-regeneration on research file writes | ✓ Good — zero manual maintenance |
| Effort profiles as convention doc | Claude Code has no per-agent effort API at runtime | ✓ Good — clear guidance without false infrastructure |
| Agent teams for read-only work only | File conflicts make parallel implementation unsafe | ✓ Good — research/review parallelized safely |
| Subagents for critics (not teams) | Critics are independent read-only reviewers | ✓ Good — simpler than full team orchestration |
| gsd-tools for all state operations | Eliminates manual STATE.md parsing errors | ✓ Good — 8 workflows migrated |
| Tokenized keyword matching for context compliance | Semantic analysis would be too slow for CLI pre-check | ✓ Good — fast deterministic layer + agent override |
| Canary plugin testing after every change | 35+ production plugins depend on system stability | ✓ Good — O-SimpleReverb confirmed no regressions |
| Full system review before cleanup milestone | Data-driven prioritization of hygiene work | ✓ Good — 632K token review found 15 recommendations, 9 quick wins |
| Remove validation cache rather than activate | Dead infrastructure adds confusion; validation runs fast enough | ✓ Good — clean deletion, no perf regression |
| 10KB cap on agent memory files | Prevent unbounded growth, force manual curation | ✓ Good — sustainable long-term pattern |
| Write-back deduplication via key phrases | Avoid re-appending known patterns to memory files | ✓ Good — prevents memory bloat across sessions |
| 5-field minimum frontmatter standard | Consistent metadata across all 64 research docs | ✓ Good — validator hook enforces going forward |

---
*Last updated: 2026-04-27 — Phase 25 (package-docs) complete: canonical .doricolib + cross-platform installer bundling (8 plugins × macOS + Windows STRICT-PASS) + internal-dev technical reference. v1.5 milestone ready for ship audit; 1 HUMAN-UAT item carries forward (Windows multi-account UAC test for Inno Setup `{userappdata}` semantics).*
