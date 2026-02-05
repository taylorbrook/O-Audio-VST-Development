# Plugin Freedom System Overhaul

## What This Is

A comprehensive overhaul of Ouaricon Audio's JUCE plugin development system — the agents, workflows, skills, state tracking, and module system that enable collaborative creation of professional audio plugins. This is a meta-project about improving the tooling, not building a specific plugin.

## Core Value

Reliable collaborative workflow that produces professional-quality plugins — where the user guides key decisions and agents execute quality work that doesn't require constant rework.

## Current State

**Latest shipped:** v1.1 Cleanup & Workflow Polish (2026-02-02)
**Current milestone:** v1.2 Agent Intelligence & Resource Orchestration

## Current Milestone: v1.2 Agent Intelligence & Resource Orchestration

**Goal:** Ensure all agents are invoked appropriately, have access to the full knowledge base, and are required to use relevant resources with full traceability.

**Target features:**
- Resource discovery (auto-detect relevant docs based on task context)
- Mandatory resource injection (agents receive relevant resources before execution)
- Usage accountability (agents report what resources they consulted)
- Agent invocation audit (verify agents are called when they should be)
- Hook system (pre-inject context, post-validate usage)
- Full knowledge base access (research, patterns, templates, plugins, modules)

**System overview:**
- 9 phases completed across 2 milestones
- 25 plans executed
- Repository: 58MB (91% reduction from 636MB)
- Clone time: 4 seconds

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

### Active

- [ ] Resource discovery system (auto-detect relevant docs from task context)
- [ ] Mandatory resource injection for all stage agents
- [ ] Agent usage accountability (report resources consulted)
- [ ] Agent invocation audit (verify agents called when needed)
- [ ] Pre-agent hooks for context injection
- [ ] Post-agent hooks for usage validation
- [ ] Full knowledge base access (research, patterns, templates, plugins, modules)

### Deferred (v1.3+)

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

### v1.0 → v1.1 Progress

**Resolved in v1.0:**
- Agent contracts now explicit with JSON Schema validation
- State tracking reliable with checkpoints and recovery
- Structured handoffs with schema validation
- Quality gates at stage boundaries
- Domain expertise encoded (DSP real-time safety, UI thread safety)
- Module system reliable with tracking and semver

**Resolved in v1.1:**
- Repository bloat eliminated (636MB → 58MB)
- Clone time reduced (2 min → 4 sec)
- Planning workflow for complex improvements
- Conditional triggers preserve fast path for simple fixes

### Remaining Concerns

**Deployment:**
- Windows installer automation not yet implemented
- Cross-platform CI/CD incomplete

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

---
*Last updated: 2026-02-04 after v1.2 milestone start*
