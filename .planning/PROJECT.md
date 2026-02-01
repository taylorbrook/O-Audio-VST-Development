# Plugin Freedom System Overhaul

## What This Is

A comprehensive overhaul of Ouaricon Audio's JUCE plugin development system — the agents, workflows, skills, state tracking, and module system that enable collaborative creation of professional audio plugins. This is a meta-project about improving the tooling, not building a specific plugin.

## Core Value

Reliable collaborative workflow that produces professional-quality plugins — where the user guides key decisions and agents execute quality work that doesn't require constant rework.

## Current Milestone: v1.1 Cleanup & Workflow Polish

**Goal:** Clean up repository debt and enhance the plugin-improve workflow with a structured planning phase.

**Target features:**
- Repository cleanup (remove ~1.5GB build artifacts, standardize naming, organize structure)
- Add GSD-style Plan phase to plugin-improve workflow for complex improvements

## Requirements

### Validated

- JUCE 8 audio framework integration — existing
- GSD hybrid workflow (discuss→research→plan→execute→verify) — existing
- WebView-based UI architecture — existing
- Module system for code reuse — v1.0 (reliable with tracking)
- 13 specialized agents with contracts — v1.0
- State tracking via STATUS.md and registry — v1.0 (reconciliation working)
- Structured handoffs with schema validation — v1.0
- Quality gates at stage boundaries — v1.0
- Generator-critic loops — v1.0
- Domain expertise encoding (DSP, UI) — v1.0

### Active

- [ ] Remove build artifacts from git history (~1.5GB)
- [ ] Standardize plugin naming conventions
- [ ] Organize scattered documentation
- [ ] Update .gitignore for comprehensive coverage
- [ ] Add Plan phase to plugin-improve workflow (Tier 2/3 improvements)
- [ ] Create improvement plan template with task breakdown
- [ ] Add approach approval checkpoint before implementation

### Out of Scope

- Building new plugins during this project — focus is on system improvement
- Changing JUCE version — staying on JUCE 8
- Abandoning GSD hybrid approach — the phase model is valuable
- Rewriting from scratch — iterative improvement of existing system

## Context

### Current System

The Plugin Freedom System is a JUCE 8-based audio plugin development framework with:

**Stages (5):**
- 0-ideation: Creative brief and parameter specification
- 1-foundation: CMake setup, APVTS parameters, shell plugin
- 2-dsp: Audio processing implementation
- 3-gui: WebView UI integration
- 4-polish: Testing, validation, finishing

**Phases per stage (5):**
- discuss: Gather context and clarify approach
- research: Investigate implementation patterns
- plan: Create task breakdown
- execute: Run stage-specific agent
- verify: Validate goal achievement

**Existing Agents:**
- foundation-shell-agent: Stage 1 implementation
- dsp-agent: Stage 2 DSP implementation
- gui-agent: Stage 3 UI integration
- ui-design-agent: WebView mockup creation
- ui-finalization-agent: WebView implementation files
- research-planning-agent: Stage 0 architecture/planning
- validation-agent: Build and validation checks
- troubleshoot-agent: Deep research for build failures
- polish-agent: Stage 4 finishing

**Module System:**
- Reusable components: preset-manager, webview-relay-manager, tuning engines, etc.
- CMake integration via OuariconModules.cmake
- Commands: /module:add, /module:remove, /module:create, /module:upgrade

**Completed Plugins:**
- O-AnalogSaturation
- O-MultiBandCompressor
- Others

### Known Problems

**Agent Issues:**
- Specs may be incomplete or vague
- Boundaries unclear (agents try to do too much or too little)
- Possible missing agents for specialized work
- Output quality requires many iteration cycles

**Quality Gaps:**
- DSP algorithms sound amateur or have artifacts
- UI implementations look unpolished, behave inconsistently
- Stability issues (crashes, glitches, DAW compatibility)

**Workflow Friction:**
- Stage transitions aren't smooth
- State tracking breaks between sessions
- Handoffs between phases lack clear contracts
- Context gets lost, requiring re-explanation

**Module System:**
- Unreliable when implementing modules in plugins
- Update propagation across dependent plugins fails

## Constraints

- **Framework**: JUCE 8 — proven foundation, not changing
- **Workflow**: GSD hybrid approach — discuss→research→plan→execute→verify phase model is valuable
- **Platform**: macOS primary, cross-platform via GitHub Actions CI/CD
- **UI Tech**: WebView-based (open to alternatives but working)
- **Tooling**: Claude Code with slash commands and specialized agents

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Agents-first approach | Fix the foundation before workflow details | — Pending |
| Full system scope | Holistic improvements vs. piecemeal fixes | — Pending |
| Collaborative workflow model | User guides, agents execute | — Pending |

---
*Last updated: 2026-02-01 after milestone v1.1 initialization*
