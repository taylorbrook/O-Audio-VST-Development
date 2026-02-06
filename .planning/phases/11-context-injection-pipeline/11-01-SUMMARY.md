---
phase: 11-context-injection-pipeline
plan: 01
subsystem: tooling
tags: [python, context-injection, token-budget, resource-discovery, xml-formatting, cli]
requires:
  - phase: 10-resource-index-and-discovery
    provides: "discover-resources.py, resource-index.json manifest"
provides:
  - "inject-context.py shared utility for agent prompt context injection"
  - "Token budget enforcement (4,000 default, configurable)"
  - "Stage pattern auto-injection for stages 1-3"
  - "Library API (inject_context function) and CLI interface"
affects:
  - "11-02: Skill orchestrator integration will call inject_context()"
  - "12+: Any future agent orchestration using research context"
tech-stack:
  added: []
  patterns:
    - "importlib.util for hyphenated Python module imports"
    - "Token estimation via len(text)/3.5 heuristic"
    - "XML-delimited context blocks for prompt injection"
    - "Tiered resource presentation (MUST-READ primary, supplementary mentions)"
key-files:
  created:
    - ".claude/scripts/inject-context.py"
key-decisions:
  - "80-token overhead reserved for XML framing (tags, headers, section labels)"
  - "Stage pattern files get first priority in token budget allocation"
  - "Primary resources capped at 800 tokens each, supplementary at 200 total"
  - "Graceful failure returns empty string (no workflow interruption)"
patterns-established:
  - "Context injection utility pattern: discover -> extract -> format -> budget-enforce"
  - "Stage-specific pattern auto-injection via STAGE_PATTERN_MAP dict"
duration: 4min
completed: 2026-02-05
---

# Phase 11 Plan 01: Context Injection Utility Summary
**Python utility bridging Phase 10 discovery to agent prompts with XML-formatted, token-budgeted research context blocks**

## Performance

- Duration: ~4 minutes
- Tasks: 2/2 completed
- All 10 verification criteria passed
- All 5 edge case tests passed

## Accomplishments

1. Created `.claude/scripts/inject-context.py` (380 lines) -- the core engine of Phase 11
2. Implemented full token budget management with 4,000-token default and configurable override
3. Stage pattern auto-injection for stages 1-3 with line-by-line truncation when exceeding 2,500-token cap
4. Content extraction from research docs with YAML frontmatter parsing and first-section inclusion
5. XML-delimited output format with MUST-READ (primary tier, score >= 0.75) and Supplementary tiers
6. Graceful failure on all error paths: empty string return, warnings to stderr
7. Both CLI and library interfaces functional and tested

## Task Commits

| Task | Name | Commit | Key Changes |
|------|------|--------|-------------|
| 1 | Create inject-context.py with content extraction, budget management, and CLI | 95fcd1b | New script: 378 lines, all core functions |
| 2 | Verify edge cases and budget enforcement | dee0898 | Fix: 80-token framing overhead for accurate budget |

## Files Created/Modified

### Created
- `.claude/scripts/inject-context.py` -- Context injection utility (380 lines, executable)

### Modified
- None (no existing files modified)

## Decisions Made

1. **Framing overhead of 80 tokens** -- XML tags, section headers, and intro text consume approximately 80 tokens of overhead. This is subtracted from the available budget before allocating to patterns and resources. Without this, budget enforcement was off by ~5 tokens for small budgets.

2. **Stage pattern first priority** -- Pattern files (stages 1-3) are loaded first and given up to 2,500 tokens. Remaining budget goes to research resources. This ensures stage-specific troubleshooting patterns are always present.

3. **800 tokens per primary resource** -- Each primary-tier resource gets up to 800 tokens of extracted content (title + summary + first section). This balances depth per resource against having multiple resources in the budget.

4. **200 tokens supplementary reserve** -- Supplementary resources get one-line mentions (title + summary + path) within a 200-token reserve. This ensures breadth even when primary resources consume most of the budget.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Token budget slightly exceeded for small budgets**
- **Found during:** Task 2, edge case 5 (budget override test)
- **Issue:** Budget 1000 produced 1005 tokens because XML framing overhead (tags, headers, section labels) was not subtracted from available budget
- **Fix:** Added `FRAMING_OVERHEAD = 80` constant, subtracted from `remaining_budget` at start of `inject_context()`
- **Files modified:** `.claude/scripts/inject-context.py`
- **Commit:** dee0898

## Issues Encountered

None. All verification tests passed after the framing overhead fix.

## User Setup Required

None. The script uses only stdlib + PyYAML (already installed). No new dependencies.

## Next Phase Readiness

**Ready for 11-02:** The `inject_context()` function is ready to be called by skill orchestrators. Integration pattern:

```python
import importlib.util
spec = importlib.util.spec_from_file_location("inject_context", ".claude/scripts/inject-context.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
context_block = mod.inject_context(plugin_name="O-EQ", stage=2, agent_type="dsp-agent")
```

**Token budget verified for all stage/agent combinations:**
- Stage 0 (research-planning-agent): 1,795 tokens
- Stage 1 (dsp-agent): 3,295 tokens
- Stage 2 (dsp-agent): 3,478 tokens
- Stage 3 (gui-agent): 3,062 tokens (stage-3-patterns.md truncated from ~3,317 to ~2,500)
- Stage 4 (polish-agent): 464 tokens

No blockers or concerns for Plan 02.

## Self-Check: PASSED
