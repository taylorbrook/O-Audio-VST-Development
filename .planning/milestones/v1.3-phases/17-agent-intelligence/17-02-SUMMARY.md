---
phase: 17-agent-intelligence
plan: 02
subsystem: agent-orchestration
tags: [critic-review, parallel-subagents, severity-ranking, approval-gates, json-schema]

# Dependency graph
requires:
  - phase: 17-agent-intelligence/01
    provides: "Research team orchestrator and dynamic researcher agent definitions"
provides:
  - "Critic orchestrator agent for parallel post-stage review"
  - "Architecture critic domain definition (contract alignment, module usage, naming, dependency direction)"
  - "Foundation critic domain definition (CMake, APVTS, build health, module integration)"
  - "Unified report merger utility (Python CLI + module)"
  - "Unified critic report JSON Schema (draft 2020-12)"
affects: [17-agent-intelligence/03, plugin-workflow, stage-gates]

# Tech tracking
tech-stack:
  added: [merge-critic-reports.py, critic-report-unified.schema.json]
  patterns: [parallel-subagent-critics, severity-normalization, blocker-enforcement, plan-approval-gates]

key-files:
  created:
    - .claude/agents/critic-orchestrator.md
    - .claude/critics/critic-architecture.md
    - .claude/critics/critic-foundation.md
    - .claude/hooks/merge-critic-reports.py
    - .planning/workflow/schemas/critic-report-unified.schema.json
  modified: []

key-decisions:
  - "Subagents for critics (not Agent Teams) since critics are independent read-only reviewers"
  - "Severity normalization: error -> blocker in unified report for consistent ranking"
  - "ARCH-NNN and FND-NNN issue ID prefixes for new critic domains"
  - "Complexity score formula: files*0.2 + dsp_files*1.0 + modules*0.5 + new_dsp*0.8 + processBlock*0.5"

patterns-established:
  - "Critic agent definition pattern: frontmatter with name/description/schema, scoring categories with thresholds, issue ID format, fix suggestion format"
  - "Unified report merger pattern: read JSONs, normalize severity, sort by rank, compute progression flag"
  - "Plan approval gate pattern: auto-approve low-risk (< 5 files, no DSP), gate complex (5+ files or DSP changes)"

# Metrics
duration: 4min
completed: 2026-02-10
---

# Phase 17 Plan 02: Parallel Critic Review System Summary

**Critic orchestrator with parallel subagent spawning, architecture and foundation critics, severity-ranked unified report merger, and plan approval gates with auto-approve/gate/escalation flow**

## Performance

- **Duration:** 4 min
- **Started:** 2026-02-10T06:53:01Z
- **Completed:** 2026-02-10T06:57:57Z
- **Tasks:** 2
- **Files created:** 5

## Accomplishments

- Critic orchestrator agent defined with stage-to-critic mapping table, parallel subagent spawning, severity enforcement (blockers block, warnings advise), and plan approval gates (auto-approve low-risk, gate complex, 3-rejection escalation)
- Architecture critic defined with 4 scoring categories (contract alignment 7/10, module usage 6/10, naming conventions 5/10, dependency direction 7/10)
- Foundation critic defined with 4 scoring categories (CMake correctness 8/10, APVTS integration 7/10, build health 8/10, module integration 6/10)
- Python merger utility reads individual critic reports, normalizes "error" to "blocker", sorts by severity rank, computes progression_allowed flag, outputs unified JSON -- importable as module and runnable as CLI
- Unified report schema (draft 2020-12) with UnifiedIssue definition enforcing severity enum, ID pattern, and minimum description/suggestion lengths

## Task Commits

Each task was committed atomically:

1. **Task 1: Create critic orchestrator and new critic domain definitions** - `b5ed306` (feat)
2. **Task 2: Create unified report merger and schema** - `daf8366` (feat)

## Files Created/Modified

- `.claude/agents/critic-orchestrator.md` - Critic spawning orchestrator with stage-to-critic mapping and approval gates
- `.claude/critics/critic-architecture.md` - Architecture alignment critic with contract/module/naming/dependency scoring
- `.claude/critics/critic-foundation.md` - Foundation/build critic with CMake/APVTS/build-health/module scoring
- `.claude/hooks/merge-critic-reports.py` - Python utility for merging critic reports into unified severity-ranked output
- `.planning/workflow/schemas/critic-report-unified.schema.json` - JSON Schema for unified critic report

## Decisions Made

- Used subagents (not Agent Teams) for critics since they are independent read-only reviewers with no inter-critic communication needed
- Normalized "error" severity to "blocker" in unified reports for consistent cross-critic severity ranking
- Chose ARCH-NNN and FND-NNN as issue ID prefixes for architecture and foundation critics respectively (following DSP-NNN and UI-NNN pattern)
- Defined complexity score formula for plan approval gate thresholds: `files*0.2 + dsp_files*1.0 + modules*0.5 + new_dsp*0.8 + processBlock*0.5`
- Foundation critic gets limited Bash access (build verification only: ninja -n, cmake --build) unlike other critics which are strictly Read/Grep/Glob

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- All four critic domains now available (DSP, UI, architecture, foundation) for the critic orchestrator to spawn
- Unified report schema ready for integration with stage transition gates
- Merger utility ready for CLI invocation by critic orchestrator
- Plan approval gate thresholds defined for integration into plan execution workflow
- Ready for Plan 03: TaskCompleted validation hooks and integration wiring

## Self-Check: PASSED

- All 5 created files verified on disk
- Both task commits (b5ed306, daf8366) verified in git log
- All 8 plan verification checks passed

---
*Phase: 17-agent-intelligence*
*Completed: 2026-02-10*
