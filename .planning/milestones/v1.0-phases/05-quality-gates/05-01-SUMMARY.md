---
phase: 05-quality-gates
plan: 01
subsystem: verification
tags: [quality-gates, bash, json-schema, ajv-cli, pluginval, critic, retry-logic]

# Dependency graph
requires:
  - phase: 04-verification-infrastructure
    provides: run-critic.sh, validate-handoff.sh, critic schemas
  - phase: 03-structured-handoffs
    provides: stage-transition-gate.sh, handoff schemas
provides:
  - gate-report.schema.json for gate execution reports
  - run-gate.sh unified gate orchestration script
  - Critical/advisory check separation
  - Retry logic with exponential backoff
  - Force bypass with justification logging
affects: [05-quality-gates, plugin-workflow, stage-transitions]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Critical vs advisory check separation (blocking vs reporting)
    - Automatic retry on first failure (transient issue handling)
    - Exponential backoff (2s, 4s, 8s)
    - Force bypass with mandatory audit logging
    - Stage-dependent critic applicability

key-files:
  created:
    - .planning/workflow/schemas/gate-report.schema.json
    - .planning/workflow/scripts/run-gate.sh

key-decisions:
  - "ISO 8601 timestamp via regex pattern (portable, no ajv-formats dependency)"
  - "Stage-dependent critics: DSP at 2+, UI at 3+"
  - "Placeholders for advisory checks (future work)"

patterns-established:
  - "Gate report schema: gate-report.schema.json defines execution result structure"
  - "Unified gate entry point: run-gate.sh orchestrates all checks at stage boundaries"
  - "Bypass audit: gate-bypasses.log tracks all --force usage with justification"

# Metrics
duration: 3min
completed: 2026-01-31
---

# Phase 5 Plan 1: Gate Infrastructure Summary

**Unified quality gate with critical/advisory split, automatic retry, and force bypass audit logging**

## Performance

- **Duration:** 3 min
- **Started:** 2026-01-31T07:21:58Z
- **Completed:** 2026-01-31T07:24:43Z
- **Tasks:** 2
- **Files created:** 2

## Accomplishments

- JSON Schema for gate execution reports (gate-report.schema.json)
- Unified gate script composing Phase 3/4 infrastructure (run-gate.sh)
- Critical/advisory check separation with stage-dependent critic applicability
- Automatic retry on first failure with exponential backoff (2s, 4s, 8s)
- Force bypass with mandatory justification logging to gate-bypasses.log

## Task Commits

Each task was committed atomically:

1. **Task 1: Create gate-report.schema.json** - `7a8d269` (feat)
2. **Task 2: Create run-gate.sh unified gate script** - `91361e6` (feat)

## Files Created

- `.planning/workflow/schemas/gate-report.schema.json` - JSON Schema for gate execution reports with CheckResult definition, bypass tracking, and review status
- `.planning/workflow/scripts/run-gate.sh` - Unified quality gate orchestration with critical/advisory split, retry logic, force bypass, and stage-dependent critic invocation

## Decisions Made

1. **ISO 8601 timestamp via regex pattern** - Used pattern instead of format "date-time" for portability (ajv-formats not required)
2. **Stage-dependent critics** - DSP critic critical at stage 2+, UI critic critical at stage 3+ (matches CONTEXT.md decisions)
3. **Advisory checks as placeholders** - Style, naming, docs marked as future work (plan scope focused on gate infrastructure)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- **ajv-cli format validation** - The "date-time" format requires ajv-formats package. Fixed by using regex pattern instead, which is more portable and doesn't require additional dependencies.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Gate infrastructure complete with run-gate.sh as single entry point
- Ready for plan 05-02 (code review integration)
- Existing scripts (validate-handoff.sh, run-critic.sh) successfully composed into gate
- Advisory checks are placeholders - can be implemented when style/naming/docs checks are defined

---
*Phase: 05-quality-gates*
*Completed: 2026-01-31*
