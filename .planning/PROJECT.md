# Plugin Freedom System Overhaul

## What This Is

A comprehensive overhaul of Ouaricon Audio's JUCE plugin development system — the agents, workflows, skills, state tracking, and module system that enable collaborative creation of professional audio plugins. This is a meta-project about improving the tooling, not building a specific plugin.

## Core Value

Reliable collaborative workflow that produces professional-quality plugins — where the user guides key decisions and agents execute quality work that doesn't require constant rework.

## Current State

**Latest shipped:** v1.3 System Modernization (2026-02-10)
**Current milestone:** v1.4 System Hygiene & Quality Gates

**System overview:**
- 17 phases completed across 4 milestones
- 51 plans executed
- 85+ requirements satisfied
- Repository: 58MB (91% reduction from 636MB)
- Clone time: 4 seconds
- 27 research documents indexed with 10-field frontmatter
- All agents running on Opus 4.6 with effort-level tuning
- Multi-layer context persistence (compaction snapshots, DIGEST.json, agent memory)
- Agent teams available for parallel research and review
- gsd-tools CLI handles all state operations

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

### Active

## Current Milestone: v1.4 System Hygiene & Quality Gates

**Goal:** Address all findings from the full system review — activate dormant quality gates, remove dead code, fix research governance gaps, consolidate overlapping subsystems, and add structural improvements for agent memory and validation.

**Target features:**
- Activate 3 dormant hooks (SubagentStop, research frontmatter, resource index regen) in settings.json
- Remove all dead code (816 lines of .sh hooks, vestigial hooks.json, 3 dead agents, deprecated registry)
- Fix resource index 50% coverage gap and standardize research doc frontmatter
- Merge overlapping skills (plugin-phases into plugin-workflow)
- Clean up infrastructure (__pycache__ gitignore, empty agent memory, documentation-only files)
- Add agent memory write-back mechanism
- Evaluate and activate validation cache and schema enforcement

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

---
*Last updated: 2026-03-05 after v1.4 milestone start*
