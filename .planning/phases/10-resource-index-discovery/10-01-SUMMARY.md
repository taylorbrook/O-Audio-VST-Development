---
phase: 10-resource-index-discovery
plan: 01
subsystem: discovery
tags: [json-schema, yaml, frontmatter, validation, pyyaml, jsonschema, hooks]

# Dependency graph
requires:
  - phase: none
    provides: "First plan in v1.2 - no prior phase dependencies"
provides:
  - "JSON Schema (Draft 2020-12) defining valid resource-index.json manifest entries"
  - "YAML frontmatter structural validator for research documents"
  - "PostToolUse hook registration for automatic frontmatter validation"
  - "jsonschema pip package installed for downstream manifest validation"
affects: [10-02 frontmatter retrofit, 10-03 manifest generator, 10-04 discovery script]

# Tech tracking
tech-stack:
  added: [jsonschema 4.26.0]
  patterns: [YAML frontmatter validation, JSON Schema Draft 2020-12 manifest definition]

key-files:
  created:
    - .claude/resource-index.schema.json
    - .claude/hooks/validators/validate-research-frontmatter.py
  modified:
    - .claude/hooks/hooks.json

key-decisions:
  - "Co-located schema with manifest at .claude/resource-index.schema.json (not .planning/workflow/schemas/)"
  - "Frontmatter validator uses only PyYAML (no jsonschema import) to stay within hook timeout budget"
  - "Validator self-filters: silently passes non-research files, so broad write|edit matcher is safe"

patterns-established:
  - "Research document metadata: 7 required YAML frontmatter fields (title, summary, domain, type, keywords, stages, agents)"
  - "Keyword format: lowercase alphanumeric with hyphens only (^[a-z0-9-]+$)"
  - "DocumentEntry schema: path-keyed entries with enum-constrained domain/type/agents"

# Metrics
duration: 2min
completed: 2026-02-05
---

# Phase 10 Plan 01: Schema & Validation Infrastructure Summary

**JSON Schema (Draft 2020-12) for resource-index.json manifest with DocumentEntry definitions, plus YAML frontmatter validator registered as PostToolUse hook using PyYAML**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-05T20:01:59Z
- **Completed:** 2026-02-05T20:04:12Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Created JSON Schema defining the complete structure for resource-index.json with DocumentEntry type (path, title, summary, domain, type, keywords, stages, agents)
- Built a 233-line frontmatter validation script that checks all 7 required fields, validates enum values, enforces keyword format, and validates stage/agent constraints
- Registered validator as a PostToolUse hook that fires on write/edit operations with 3000ms timeout
- Installed jsonschema 4.26.0 for downstream manifest validation (generator and discovery scripts)

## Task Commits

Each task was committed atomically:

1. **Task 1: Create JSON Schema for resource-index.json and install jsonschema** - `f4ea0b2` (feat)
2. **Task 2: Create frontmatter validation script** - `f8a526a` (feat)
3. **Task 3: Register frontmatter validator in hooks.json** - `d7775e1` (feat)

## Files Created/Modified
- `.claude/resource-index.schema.json` - JSON Schema Draft 2020-12 defining manifest structure with DocumentEntry type
- `.claude/hooks/validators/validate-research-frontmatter.py` - YAML frontmatter structural validator (233 lines, executable)
- `.claude/hooks/hooks.json` - Added PostToolUse hook entry for frontmatter validation

## Decisions Made
- **Schema location:** Co-located schema at `.claude/resource-index.schema.json` alongside future manifest, not in `.planning/workflow/schemas/`. The discovery system is a `.claude` concern.
- **No jsonschema in hooks:** Frontmatter validator deliberately avoids importing jsonschema to stay within 3000ms hook timeout. Uses only PyYAML + regex for structural checks.
- **Self-filtering validator:** Validator silently passes (exit 0) for non-research files and README.md, so the broad `write|edit` matcher safely covers all tool operations without false positives.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Schema and validator are in place for Plan 02 (frontmatter retrofit of ~26 research documents)
- All existing research documents correctly trigger frontmatter errors when validated (confirmed with test runs)
- jsonschema package is available for Plan 03 (manifest generator) and Plan 04 (discovery script)

---
*Phase: 10-resource-index-discovery*
*Completed: 2026-02-05*
