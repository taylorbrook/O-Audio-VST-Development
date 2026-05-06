---
phase: "04"
plan: "02"
subsystem: verification-infrastructure
tags: [critics, dsp-validation, ui-validation, quality-gates]

requires:
  - 04-01: "Critic report schemas (critic-dsp-report.schema.json, critic-ui-report.schema.json)"
  - 04-01: "Critic orchestration script (run-critic.sh)"

provides:
  - critic-dsp: "DSP domain critic template with 5 scoring categories"
  - critic-ui: "UI domain critic template with 5 scoring categories"
  - plugin-critique: "On-demand critic invocation command"

affects:
  - Phase 5: "Quality gates invoke critics at stage transitions"
  - plugin-workflow: "Stage completion triggers automatic critique"
  - plugin-improve: "Fix iterations use critic feedback"

tech-stack:
  added: []
  patterns:
    - domain-critic-pattern: "Encode domain expertise in structured critic templates"
    - fix-suggestion-mandate: "Every issue requires actionable fix approach"
    - tiered-thresholds: "DSP stricter (8/7/6) than UI (5/6) for appropriate gate behavior"

key-files:
  created:
    - ".claude/critics/critic-dsp.md"
    - ".claude/critics/critic-ui.md"
    - ".claude/commands/plugin-critique.md"
  modified: []

key-decisions:
  - id: "domain-threshold-calibration"
    decision: "DSP thresholds 8/7/6 for realtime/buffer/params; UI thresholds 5/6/7 for polish/consistency/thread-safety"
    rationale: "DSP bugs cause immediate audio glitches; UI polish is iterative"
    alternatives: ["Equal thresholds across domains", "Higher UI thresholds"]
  - id: "mandatory-fix-suggestions"
    decision: "Every issue in critic reports MUST include fixSuggestion field with actionable approach"
    rationale: "Prevents vague feedback loops; agent can act on specific guidance"
    alternatives: ["Optional suggestions", "Code snippets instead of approaches"]
  - id: "thread-safety-required"
    decision: "Thread safety is a required category in UI critic with threshold 7"
    rationale: "Member declaration order bugs cause release-build crashes"
    alternatives: ["Optional category", "Lower threshold"]

metrics:
  duration: "4 minutes"
  completed: "2026-01-31"
---

# Phase 04 Plan 02: Domain-Specific Critics Summary

Two domain critics encoding professional expertise for DSP (real-time safety) and UI (polish standards), plus /plugin-critique command for on-demand invocation.

## What Was Built

### DSP Critic (critic-dsp.md)

Domain-specific critic for Stage 2 validation with 5 scoring categories:

| Category | Threshold | Purpose |
|----------|-----------|---------|
| Real-Time Safety | 8/10 | No allocations, locks, I/O in processBlock |
| Buffer Handling | 7/10 | Zero-length checks, preallocation, ScopedNoDenormals |
| Parameter Integration | 6/10 | All params connected and affecting DSP |
| Numerical Stability | 6/10 | Denormal handling, DC blocking (optional) |
| Architecture Alignment | 5/10 | Matches ARCHITECTURE.md design (optional) |

**Key features:**
- Explicit violation checklists (memory allocation patterns, lock patterns, I/O patterns)
- Evidence requirements (file:line locations for each issue)
- Mandatory fix suggestions describing approach, not code
- Escalation criteria for stuck iterations

### UI Critic (critic-ui.md)

Domain-specific critic for Stage 3 validation with 5 scoring categories:

| Category | Threshold | Purpose |
|----------|-----------|---------|
| Polish | 5/10 | Professional appearance, not placeholder quality |
| Consistency | 6/10 | Uniform control behavior and styling |
| Accessibility | 5/10 | Labels, contrast, keyboard nav (optional) |
| Responsiveness | 5/10 | Layout adaptation (optional) |
| Thread Safety | 7/10 | Member order, APVTS patterns (required) |

**Key features:**
- Lower thresholds than DSP (polish is iterative)
- Thread safety critical (7 threshold) - member order bugs cause crashes
- Mockup comparison guidelines for visual evaluation
- Fix suggestions for visual and code issues

### /plugin-critique Command (plugin-critique.md)

On-demand critic invocation with:
- Stage validation (2-dsp or 3-gui only)
- Iteration tracking (--attempt 1/2/3)
- Integration with run-critic.sh for orchestration
- Structured output format (scores, issues, next action)
- Token budget awareness (--token-count)

## Deviations from Plan

None - plan executed exactly as written.

## Patterns Established

### Domain Critic Pattern
- Each domain has dedicated critic template
- Scoring categories with numeric thresholds
- Pass/fail per category, overall status aggregated
- Schema-conforming JSON reports

### Fix Suggestion Mandate
- Every issue MUST include fixSuggestion field
- Describes approach, NOT code snippets
- Enables agent to act on specific guidance
- Prevents vague feedback loops

### Tiered Threshold Pattern
- Critical domains (DSP) have higher thresholds
- Iterative domains (UI polish) have lower thresholds
- Some categories optional (informational)
- Thread safety always critical (crash prevention)

## Integration Points

### With run-critic.sh (04-01)
- Command invokes script with arguments
- Script handles iteration tracking and file persistence
- Script provides no-progress detection
- Script determines exit codes for gate composition

### With Schemas (04-01)
- DSP reports conform to critic-dsp-report.schema.json
- UI reports conform to critic-ui-report.schema.json
- Schemas enforce required fields and formats

### With Future Phase 5 (Quality Gates)
- stage-transition-gate.sh will invoke critics
- Gate composition pattern from 03-02 applies
- Critics run automatically at stage boundaries

## Verification

All plan verification criteria confirmed:
- [x] DSP critic covers real-time safety with threshold 8
- [x] DSP critic covers buffer handling with threshold 7
- [x] DSP critic covers parameter integration with threshold 6
- [x] UI critic covers polish with threshold 5
- [x] UI critic covers consistency with threshold 6
- [x] UI critic covers thread safety with threshold 7
- [x] Both critics require fix suggestions
- [x] /plugin-critique validates stage argument
- [x] /plugin-critique integrates with run-critic.sh

## Next Phase Readiness

Phase 4 complete. Ready for Phase 5 (Quality Gates):
- [x] Critic schemas exist (04-01)
- [x] Orchestration script exists (04-01)
- [x] Domain critics exist (04-02)
- [x] On-demand invocation exists (04-02)

Phase 5 will:
- Create stage-transition-gate.sh composing existing validation + critics
- Integrate gates into plugin-workflow skill
- Add gate bypass with --force pattern
