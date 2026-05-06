---
phase: 12-accountability-validation
plan: 01
subsystem: agent-accountability
tags: [schema, validation, accountability, resource-tracking, jsonl, subagent-reports]
requires:
  - "10-04 (discover-resources.py scoring and tier system)"
  - "11-01 (inject-context.py context injection pipeline)"
provides:
  - "Extended subagent report schema with resources_consulted field"
  - "Accountability validator script for MUST-READ resource tracking"
affects:
  - "12-02 (hook integration will call validate-resource-accountability.py)"
  - "All pipeline agents (will report resources_consulted in their reports)"
tech-stack:
  added: []
  patterns:
    - "importlib.util for hyphenated module imports (validator -> discover-resources.py)"
    - "Warning-only validation (always exit 0, stderr warnings)"
    - "JSONL transcript parsing with multi-strategy extraction"
key-files:
  created:
    - ".claude/hooks/validators/validate-resource-accountability.py"
  modified:
    - ".claude/schemas/subagent-report.json"
key-decisions:
  - id: "12-01-A"
    decision: "resources_consulted as top-level property (not inside outputs)"
    rationale: "Resources are agent-level metadata, not stage-specific output data"
  - id: "12-01-B"
    decision: "Multi-strategy transcript parsing (direct JSON, code blocks, embedded objects)"
    rationale: "Agent reports may be formatted differently across pipeline configurations"
  - id: "12-01-C"
    decision: "Tolerate plain string entries in resources_consulted alongside object entries"
    rationale: "Defensive parsing prevents validation failure on minor format differences"
duration: "~2 minutes"
completed: "2026-02-06"
---

# Phase 12 Plan 01: Schema Extension and Accountability Validator Summary

**One-liner:** Optional resources_consulted field in subagent schema + JSONL transcript parser that warns on skipped MUST-READ resources via discover() primary tier comparison

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~2 minutes |
| Tasks completed | 2/2 |
| Deviations | 0 |

## Accomplishments

1. **Extended subagent report schema** -- Added `resources_consulted` as an optional top-level array property with required `path` (string) and optional `relevance` (string) per item. Backward compatible: existing reports without the field still validate. `additionalProperties: false` on items enforces strict structure.

2. **Created resource accountability validator** -- Script at `.claude/hooks/validators/validate-resource-accountability.py` that:
   - Imports `discover()` from `discover-resources.py` via importlib.util
   - Maps 5 core agents to pipeline stages via `AGENT_STAGE_MAP`
   - Discovers primary-tier (MUST-READ) resources for the agent's stage/role
   - Parses JSONL transcripts with 3 extraction strategies (direct JSON, code blocks, embedded objects)
   - Compares discovered MUST-READs against self-reported `resources_consulted` paths
   - Emits stderr warnings for gaps (skipped resources or missing field)
   - ALWAYS exits 0 with top-level exception handler

## Task Commits

| Task | Name | Commit | Key Files |
|------|------|--------|-----------|
| 1 | Extend subagent report schema | 3fac0e4 | .claude/schemas/subagent-report.json |
| 2 | Create resource accountability validator | 9c8a1be | .claude/hooks/validators/validate-resource-accountability.py |

## Files Created/Modified

**Created:**
- `.claude/hooks/validators/validate-resource-accountability.py` -- 215 lines, accountability validator

**Modified:**
- `.claude/schemas/subagent-report.json` -- Added 19 lines for resources_consulted property

## Decisions Made

| ID | Decision | Rationale |
|----|----------|-----------|
| 12-01-A | resources_consulted as top-level property (not inside outputs) | Resources are agent-level metadata, not stage-specific output data |
| 12-01-B | Multi-strategy transcript parsing (direct JSON, code blocks, embedded objects) | Agent reports may be formatted differently across pipeline configurations |
| 12-01-C | Tolerate plain string entries in resources_consulted alongside object entries | Defensive parsing prevents validation failure on minor format differences |

## Deviations from Plan

None -- plan executed exactly as written.

## Issues Encountered

None.

## Next Phase Readiness

Plan 12-02 (Hook Integration) can proceed immediately:
- Schema is extended and validated
- Validator script is tested and importable
- `validate()` function accepts `(agent_type, plugin_name, transcript_path)` matching expected hook interface
- Script always exits 0, safe for hook timeout constraints
- No blockers or concerns

## Self-Check: PASSED
