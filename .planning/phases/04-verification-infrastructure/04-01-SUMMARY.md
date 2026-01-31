---
phase: 04-verification-infrastructure
plan: 01
subsystem: verification
tags: [json-schema, bash, critic-loop, iteration-tracking, token-budget]

# Dependency graph
requires:
  - phase: 03-structured-handoffs
    provides: validate-handoff.sh and stage-transition-gate.sh patterns for script consistency
  - phase: 01-agent-contracts
    provides: JSON Schema draft 2020-12 conventions with additionalProperties false
provides:
  - critic-report.schema.json base schema with scores, issues, nextAction
  - critic-dsp-report.schema.json with thresholds 8/7/6
  - critic-ui-report.schema.json with thresholds 5/6
  - run-critic.sh orchestration with iteration tracking
  - .planning/verification/{plugin}/{stage}/ directory structure
affects:
  - 04-02 (domain-specific critic implementation)
  - 05-quality-gates (gate composition with critic validation)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "ScoreEntry pattern: score/threshold/passed/details for numeric scoring"
    - "Issue pattern: id/severity/category/location/description/fixSuggestion"
    - "Stage-to-critic mapping: 2-dsp -> dsp-critic, 3-gui -> ui-critic"
    - "Iteration awareness: Attempt N/3 display for agent urgency"

key-files:
  created:
    - .planning/workflow/schemas/critic-report.schema.json
    - .planning/workflow/schemas/critic-dsp-report.schema.json
    - .planning/workflow/schemas/critic-ui-report.schema.json
    - .planning/workflow/scripts/run-critic.sh
  modified: []

key-decisions:
  - "Self-contained domain schemas vs $ref composition: Domain schemas include all constraints inline for ajv-cli compatibility while documenting base schema relationship"
  - "DSP thresholds 8/7/6: realtime_safety (critical), buffer_handling (important), parameter_integration (can iterate)"
  - "UI thresholds 5/6: polish (low bar, iterative), consistency (moderate importance)"
  - "Token soft limit 50K: Warn but don't block per CONTEXT.md"

patterns-established:
  - "ScoreEntry: {score: 1-10, threshold: 1-10, passed: boolean, details: string}"
  - "Issue: {id: ^[A-Z]+-[0-9]+$, severity: error|warning, category, location, description, fixSuggestion minLength 10}"
  - "Orchestration exit codes: 0=PASSED, 1=NEEDS_FIXES, 2=ESCALATE, 3=usage error"
  - "Verification directory: .planning/verification/{plugin}/{stage}/"

# Metrics
duration: 5min
completed: 2026-01-31
---

# Phase 4 Plan 1: Critic Agent Infrastructure Summary

**Critic report schemas with numeric scoring (1-10) per domain category plus orchestration script with iteration tracking and token budget awareness**

## Performance

- **Duration:** 5 min
- **Started:** 2026-01-31T06:16:05Z
- **Completed:** 2026-01-31T06:20:48Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Created base critic-report.schema.json with ScoreEntry, Issue, nextAction, and tokenMetrics
- Created critic-dsp-report.schema.json requiring realtime_safety (8), buffer_handling (7), parameter_integration (6)
- Created critic-ui-report.schema.json requiring polish (5), consistency (6)
- Created run-critic.sh with stage-to-critic mapping, "Attempt N/3" display, and token budget warnings

## Task Commits

Each task was committed atomically:

1. **Task 1: Create critic report schemas** - `968af00` (feat)
2. **Task 2: Create critic orchestration script** - `ebafe17` (feat)

## Files Created/Modified
- `.planning/workflow/schemas/critic-report.schema.json` - Base schema with ScoreEntry, Issue definitions
- `.planning/workflow/schemas/critic-dsp-report.schema.json` - DSP domain with thresholds 8/7/6
- `.planning/workflow/schemas/critic-ui-report.schema.json` - UI domain with thresholds 5/6
- `.planning/workflow/scripts/run-critic.sh` - Orchestration with iteration and token tracking

## Decisions Made
- **Self-contained domain schemas:** Domain schemas (DSP, UI) include all constraints inline rather than using $ref to base schema. This ensures ajv-cli validation works without complex reference resolution while maintaining documentation of the base schema relationship.
- **Threshold calibration:** DSP uses higher thresholds (8/7/6) because real-time safety is critical for audio. UI uses lower thresholds (5/6) because polish can improve iteratively.
- **Token soft limit 50K:** Following CONTEXT.md "warn-not-block" principle, script warns when cumulative tokens exceed 50K but doesn't prevent iteration.

## Deviations from Plan
None - plan executed exactly as written.

## Issues Encountered
- **ajv-cli $ref resolution:** Initial attempt to use `allOf` with `$ref` to base schema failed with reference resolution errors. Resolved by making domain schemas self-contained with full constraint definitions while documenting base schema relationship in description field.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Critic report schemas ready for 04-02 domain critic implementation
- run-critic.sh provides framework for critic invocation
- Verification directory structure established for failure report persistence
- Integration point documented for Phase 5 quality gates

---
*Phase: 04-verification-infrastructure*
*Completed: 2026-01-31*
