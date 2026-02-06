---
phase: 13
plan: 03
subsystem: maintenance-tooling
tags: [staleness-detection, hook-automation, manifest-regeneration, freshness-verification, resource-pipeline]
dependencies:
  requires: [13-01, 13-02]
  provides: [MAINT-01-auto-regeneration, MAINT-03-staleness-warnings, batch-verification-script]
  affects: [agent-prompt-injection, research-document-lifecycle]
tech-stack:
  added: []
  patterns: [PostToolUse-hook-chaining, regex-based-frontmatter-update, staleness-annotation-pipeline]
key-files:
  created:
    - .claude/hooks/regenerate-manifest.sh
    - .claude/scripts/verify-freshness.py
  modified:
    - .claude/scripts/inject-context.py
    - .claude/scripts/discover-resources.py
    - .claude/hooks/hooks.json
key-decisions:
  - id: 13-03-001
    description: "Staleness annotations appended to title line for stale resources only (not all resources)"
  - id: 13-03-002
    description: "Hook always exits 0 via trap, errors swallowed to never block agent workflow"
  - id: 13-03-003
    description: "verify-freshness.py uses regex line replacement (not YAML round-trip) to preserve formatting"
  - id: 13-03-004
    description: "Threshold override via _set_threshold() helper to avoid Python global-before-use SyntaxError"
metrics:
  duration: "202s"
  started: "2026-02-06T21:31:49Z"
  completed: "2026-02-06"
---

# Phase 13 Plan 03: Staleness Detection, Hook Automation, and Batch Verification Summary

Staleness detection (90-day threshold) integrated into injection pipeline with stderr warnings and inline annotations; PostToolUse hook auto-regenerates resource-index.json on research file writes; batch verification script provides --check and --all modes for document freshness management.

## Performance

| Metric | Value |
|--------|-------|
| Duration | 202s |
| Started | 2026-02-06T21:31:49Z |
| Ended | 2026-02-06T21:35:11Z |
| Tasks | 2/2 |
| Files created | 2 |
| Files modified | 3 |

## Accomplishments

1. **Staleness detection in inject-context.py**: Added `STALENESS_THRESHOLD_DAYS = 90` constant, `check_staleness()` function that handles ISO date strings and `datetime.date` objects, and inline annotations in `format_context_block()` for both primary and supplementary resources. Stale resources get `(verified: YYYY-MM-DD, N days ago)` appended to their title line, plus stderr warnings.

2. **last_verified passthrough in discover-resources.py**: Added `last_verified` field to each result dict in the `discover()` function, sourced from `doc.get("last_verified", "")`. This enables downstream staleness checking in inject-context.py.

3. **PostToolUse hook for manifest regeneration**: Created `regenerate-manifest.sh` that fires on write/edit operations, filters for `research/*.md` paths (excluding README.md), and calls `generate-resource-index.py`. Uses `trap 'exit 0' EXIT ERR` to guarantee non-blocking behavior.

4. **Hook registration in hooks.json**: Added third PostToolUse entry with `write|edit` matcher and 3000ms timeout, matching the existing frontmatter validation hook pattern.

5. **Batch verification script**: Created `verify-freshness.py` with three modes: `--check` (reports stale docs, exits 1 if any found), `--all` (batch-updates all docs), and positional args (updates specific files). Supports `--juce-version` override (default 8.0.4) and `--threshold` override (default 90 days).

## Task Commits

| Task | Name | Commit | Key Files |
|------|------|--------|-----------|
| 1 | Add last_verified to discovery + staleness detection to injection | 9431d7a | discover-resources.py, inject-context.py |
| 2 | Create hook, register in hooks.json, create verification script | e94cc9d | regenerate-manifest.sh, hooks.json, verify-freshness.py |

## Files Created

| File | Purpose |
|------|---------|
| `.claude/hooks/regenerate-manifest.sh` | PostToolUse hook for auto manifest regeneration on research writes |
| `.claude/scripts/verify-freshness.py` | Batch freshness verification and update utility |

## Files Modified

| File | Changes |
|------|---------|
| `.claude/scripts/inject-context.py` | Added `STALENESS_THRESHOLD_DAYS`, `check_staleness()`, staleness annotations in `format_context_block()` |
| `.claude/scripts/discover-resources.py` | Added `last_verified` to result dicts in `discover()` |
| `.claude/hooks/hooks.json` | Added PostToolUse entry for `regenerate-manifest.sh` |

## Decisions Made

1. **Staleness annotations on title line only**: Annotations (`verified: YYYY-MM-DD, N days ago`) appended only to stale resources, not all. This keeps the output clean for fresh resources while making stale ones visually distinct.

2. **Hook exit-0 guarantee via trap**: Using `trap 'exit 0' EXIT ERR` ensures the hook never blocks the agent workflow, even if the generator script fails or the path is invalid.

3. **Regex-based line replacement in verify-freshness.py**: Uses `re.sub(r"^(last_verified:\s*).*$", ...)` rather than YAML round-trip parsing. This preserves exact frontmatter formatting, comments, and field ordering.

4. **_set_threshold() helper**: Python 3.14 enforces that `global` declarations must precede any use of the variable name in the same scope. The `_set_threshold()` helper avoids this SyntaxError while still allowing CLI --threshold overrides.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Python 3.14 global declaration SyntaxError**
- **Found during:** Task 2, verify-freshness.py initial testing
- **Issue:** Using `STALENESS_THRESHOLD_DAYS` in argparse default parameter value, then declaring `global STALENESS_THRESHOLD_DAYS` later in same function, triggers SyntaxError in Python 3.14 (stricter than 3.12)
- **Fix:** Extracted `_set_threshold(days)` helper function with the global declaration, called from main()
- **Files modified:** `.claude/scripts/verify-freshness.py`
- **Commit:** e94cc9d

## Issues Encountered

None beyond the auto-fixed Python 3.14 global declaration issue.

## Next Phase Readiness

Plan 13-03 completes MAINT-01 (auto-generation triggered by file writes), MAINT-03 (staleness warnings), and the script-assisted verification path.

**Ready for 13-04:** All pipeline components are in place. The staleness detection integrates cleanly with the existing injection pipeline. The hook chains correctly with the existing frontmatter validator hook (both fire on write|edit). The batch verification script is available for scheduled or manual freshness audits.

**Requirements satisfied:**
- MAINT-01: Manifest auto-regenerates on research file writes via PostToolUse hook
- MAINT-03: Stale resources flagged with annotations and stderr warnings during injection
- Script-assisted verification: verify-freshness.py --check and --all modes operational

## Self-Check: PASSED
