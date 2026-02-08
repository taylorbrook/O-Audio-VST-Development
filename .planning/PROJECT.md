# Plugin Freedom System Overhaul

## What This Is

A comprehensive overhaul of Ouaricon Audio's JUCE plugin development system — the agents, workflows, skills, state tracking, and module system that enable collaborative creation of professional audio plugins. This is a meta-project about improving the tooling, not building a specific plugin.

## Core Value

Reliable collaborative workflow that produces professional-quality plugins — where the user guides key decisions and agents execute quality work that doesn't require constant rework.

## Current State

**Latest shipped:** v1.2 Agent Intelligence & Resource Orchestration (2026-02-06)
**Current milestone:** v1.3 System Modernization (Opus 4.6 + GSD Alignment)

**System overview:**
- 13 phases completed across 3 milestones
- 38 plans executed
- 62+ requirements satisfied
- Repository: 58MB (91% reduction from 636MB)
- Clone time: 4 seconds
- 27 research documents indexed with 10-field frontmatter
- Resource discovery, injection, and accountability operational

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
- ✓ Resource discovery system (weighted scoring, 63ms, stage/role/keyword matching) — v1.2
- ✓ Context injection pipeline (auto-inject relevant research within 4K token budget) — v1.2
- ✓ Agent usage accountability (resources_consulted field, warning-level validation) — v1.2
- ✓ Self-maintaining manifest (auto-regeneration hooks, 90-day staleness detection) — v1.2
- ✓ 27 research documents with 10-field YAML frontmatter — v1.2
- ✓ Graceful degradation (agents proceed if manifest/discovery fails) — v1.2

### Active

- [ ] Opus 4.6 agent teams — leverage multi-agent orchestration for parallel/collaborative workflows
- [ ] Agent quality improvements — use improved reasoning for better first-pass output
- [ ] Context persistence — reduce information loss between sessions and across agents
- [ ] GSD feature adoption — integrate new GSD 1.18.0 features the system doesn't use yet
- [ ] GSD deduplication — remove custom code that GSD now handles natively
- [ ] System audit — thorough review of all agents, workflows, skills against current capabilities

### Deferred (v1.3+)

- [ ] Cross-plugin knowledge transfer (reference successful implementations)
- [ ] Decision provenance chains (trace decisions to source research docs)
- [ ] Module-research cross-referencing
- [ ] Windows installer automation (NSIS via GitHub Actions)
- [ ] Add/Remove Programs integration
- [ ] Plugin naming standardization

### Out of Scope

- Building new plugins during this project — focus is on system improvement
- Changing JUCE version — staying on JUCE 8
- Abandoning GSD hybrid approach — the phase model is valuable
- Rewriting from scratch — iterative improvement of existing system
- Planning for Tier 1 fixes — adds overhead without value
- Git LFS migration — not needed after cleanup

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
- research: Investigate implementation patterns
- plan: Create task breakdown (Phase 0.6 for Tier 2/3)
- execute: Run stage-specific agent
- verify: Validate goal achievement
- handoff: Stage transition with schema validation

**Existing Agents (13):**
- foundation-shell-agent: Stage 1 implementation
- dsp-agent: Stage 2 DSP implementation
- gui-agent: Stage 3 UI integration
- ui-design-agent: WebView mockup creation
- ui-finalization-agent: WebView implementation files
- research-planning-agent: Stage 0 architecture/planning
- validation-agent: Build and validation checks
- troubleshoot-agent: Deep research for build failures
- polish-agent: Stage 4 finishing
- critic agents: DSP/UI quality validation
- Plus contract validation, state recovery

**Module System:**
- Reusable components: preset-manager, webview-relay-manager, tuning engines, etc.
- CMake integration via OuariconModules.cmake
- Commands: /module:add, /module:remove, /module:create, /module:upgrade
- Semver versioning with customization detection

**Completed Plugins:**
- O-AnalogSaturation
- O-MultiBandCompressor
- O-Freeze (v1.0.0)
- O-Detune (in development)
- O-Bells (in development)

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
- 27 research docs with 10-field YAML frontmatter

### Remaining Concerns

**Deployment:**
- Windows installer automation not yet implemented
- Cross-platform CI/CD incomplete

**Intelligence:**
- No cross-plugin knowledge transfer yet
- Decision provenance chains not implemented

**Future Enhancements:**
- Automated performance benchmarking for DSP
- CI/CD integration for plugin validation on commit
- Automatic rebuild propagation when modules update

## Constraints

- **Framework**: JUCE 8 — proven foundation, not changing
- **Workflow**: GSD hybrid approach — discuss→research→plan→execute→verify phase model is valuable
- **Platform**: macOS primary, cross-platform via GitHub Actions CI/CD
- **UI Tech**: WebView-based (open to alternatives but working)
- **Tooling**: Claude Code with slash commands and specialized agents
- **Model**: Claude Opus 4.6 — primary model for all agents

## Current Milestone: v1.3 System Modernization (Opus 4.6 + GSD Alignment)

**Goal:** Audit and modernize the Plugin Freedom System to leverage Opus 4.6 capabilities (agent teams, improved reasoning) and align with GSD 1.18.0 features, eliminating custom code where the framework now provides native support.

**Target improvements:**
- Opus 4.6 agent teams for parallel/collaborative agent workflows
- Smarter agents with less rework needed
- Better context persistence across sessions
- GSD feature adoption and deduplication of custom overlapping code

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Agents-first approach | Fix the foundation before workflow details | ✓ Good — 13 agents with contracts |
| Full system scope | Holistic improvements vs. piecemeal fixes | ✓ Good — 35 requirements in v1.0, 13 in v1.1 |
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

---
*Last updated: 2026-02-08 after v1.3 milestone started*
