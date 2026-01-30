# Plugin Freedom System Overhaul

## What This Is

A comprehensive overhaul of Ouaricon Audio's JUCE plugin development system — the agents, workflows, skills, state tracking, and module system that enable collaborative creation of professional audio plugins. This is a meta-project about improving the tooling, not building a specific plugin.

## Core Value

Reliable collaborative workflow that produces professional-quality plugins — where the user guides key decisions and agents execute quality work that doesn't require constant rework.

## Requirements

### Validated

- JUCE 8 audio framework integration — existing
- GSD hybrid workflow (discuss→research→plan→execute→verify) — existing
- WebView-based UI architecture — existing
- Module system for code reuse — existing (but unreliable)
- 9 specialized agents for different stages — existing (but incomplete)
- State tracking via STATUS.md and registry — existing (but breaks)

### Active

- [ ] Audit existing agents for spec completeness, boundary clarity, and gaps
- [ ] Define clear contracts (inputs/outputs) for each agent
- [ ] Add missing agents where specialized work lacks dedicated support
- [ ] Improve DSP agent to produce professional-quality audio processing
- [ ] Improve UI agent to produce polished, consistent interfaces
- [ ] Fix stability issues (crashes, DAW compatibility, WebView lifecycle)
- [ ] Fix module system reliability (add/update across plugins)
- [ ] Improve stage transitions with clear handoffs and context preservation
- [ ] Fix state persistence so pause/resume works reliably
- [ ] Reduce rework loops through better verification and quality gates

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
*Last updated: 2026-01-29 after initialization*
