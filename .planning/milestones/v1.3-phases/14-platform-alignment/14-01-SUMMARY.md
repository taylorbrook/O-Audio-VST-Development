---
phase: 14-platform-alignment
plan: 01
subsystem: infra
tags: [opus-4.6, agent-config, effort-profiles, platform-migration]

# Dependency graph
requires: []
provides:
  - "Clean agent/critic definitions free of deprecated model/thinking config"
  - "agent-profiles.json central effort configuration for all 13 agents"
  - "PLAT-01 (adaptive thinking migration) satisfied for agent definitions"
  - "PLAT-03 (zero assistant prefills) verified"
  - "PLAT-04 (effort profiles replace model selection) satisfied"
  - "PLAT-07 (DSP/research agents always on Opus) documented via max effort"
affects: [14-02, 14-03, 14-04]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Effort profiles as convention documentation (not runtime config)"
    - "All agents run Opus 4.6 — effort level is the only tuning knob"

key-files:
  created:
    - ".claude/agent-profiles.json"
  modified:
    - ".claude/agents/dsp-agent.md"
    - ".claude/agents/research-planning-agent.md"
    - ".claude/agents/troubleshoot-agent.md"
    - ".claude/agents/validation-agent.md"
    - ".claude/agents/foundation-shell-agent.md"
    - ".claude/agents/gui-agent.md"
    - ".claude/agents/ui-design-agent.md"
    - ".claude/agents/ui-finalization-agent.md"
    - ".claude/agents/aesthetics-agent.md"
    - ".claude/agents/music-theory-agent.md"
    - ".claude/critics/critic-dsp.md"
    - ".claude/critics/critic-ui.md"

key-decisions:
  - "Effort profiles stored as convention doc, not runtime config (Claude Code has no per-agent effort API)"
  - "dsp-agent and research-planning-agent at max effort, troubleshoot-agent also at max"
  - "Unwrapped extended_thinking tags in research-planning-agent, preserving instructional content"
  - "Replaced deprecated 'extended thinking' references with neutral language ('deep analysis', 'think carefully')"

patterns-established:
  - "Agent effort documentation: agent-profiles.json as single source of truth"
  - "No model: frontmatter in agent/critic definitions"
  - "No XML thinking configuration blocks in agent definitions"

# Metrics
duration: 5min
completed: 2026-02-09
---

# Phase 14 Plan 01: Agent Config Cleanup Summary

**Stripped deprecated model/thinking config from 12 agent and critic files, created agent-profiles.json with 13 effort profiles**

## Performance

- **Duration:** 5 min
- **Started:** 2026-02-09T06:45:40Z
- **Completed:** 2026-02-09T06:51:12Z
- **Tasks:** 2
- **Files modified:** 13 (12 modified + 1 created)

## Accomplishments
- Removed `model:` frontmatter from 10 agents and 2 critics (12 total; polish-agent had none)
- Deleted `<model_selection>` and `<model_and_thinking>` sections from dsp-agent
- Unwrapped 6 `<extended_thinking>` tag pairs in research-planning-agent (kept instructional inner text)
- Removed "Use Extended Thinking" section and references from troubleshoot-agent
- Removed Extended Thinking tools guidance and budget references from research-planning-agent
- Updated validation-agent body text to reference "Opus 4.6 with adaptive thinking"
- Created agent-profiles.json with 13 profiles documenting intended effort levels

## Task Commits

Each task was committed atomically:

1. **Task 1: Strip model frontmatter and deprecated thinking sections** - `b170ccc` (refactor)
2. **Task 2: Create agent-profiles.json central effort configuration** - `791ee97` (feat)

## Files Created/Modified
- `.claude/agent-profiles.json` - Central effort profile configuration (13 agents, effort + rationale)
- `.claude/agents/dsp-agent.md` - Removed model: sonnet, <model_selection>, <model_and_thinking>, "Use extended thinking"
- `.claude/agents/research-planning-agent.md` - Removed model: sonnet, unwrapped 6 <extended_thinking> tags, removed tools guidance + notes reference
- `.claude/agents/troubleshoot-agent.md` - Removed model: opus, "Use Extended Thinking" section, 2 body text references
- `.claude/agents/validation-agent.md` - Removed model: opus, updated "Opus model" to "Opus 4.6 with adaptive thinking"
- `.claude/agents/foundation-shell-agent.md` - Removed model: sonnet
- `.claude/agents/gui-agent.md` - Removed model: sonnet
- `.claude/agents/ui-design-agent.md` - Removed model: sonnet
- `.claude/agents/ui-finalization-agent.md` - Removed model: sonnet
- `.claude/agents/aesthetics-agent.md` - Removed model: sonnet
- `.claude/agents/music-theory-agent.md` - Removed model: sonnet
- `.claude/critics/critic-dsp.md` - Removed model: opus
- `.claude/critics/critic-ui.md` - Removed model: opus

## Decisions Made
- Effort profiles stored as convention documentation, not runtime config (Claude Code has no per-agent effort API)
- dsp-agent and research-planning-agent set to max effort; troubleshoot-agent also at max
- Unwrapped `<extended_thinking>` tags rather than deleting content (preserved instructional value)
- Replaced "extended thinking" references with neutral wording: "deep analysis", "think carefully"
- polish-agent confirmed to have no `model:` field (no change needed)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Additional deprecated reference in troubleshoot-agent line 684**
- **Found during:** Task 1 verification grep
- **Issue:** Line 684 contained "Extended thinking synthesis (Opus model, 15k budget)" which was not in the plan's explicit line references
- **Fix:** Changed to "Deep analysis synthesis"
- **Files modified:** .claude/agents/troubleshoot-agent.md
- **Verification:** Re-ran grep for "extended thinking" - zero results
- **Committed in:** b170ccc (Task 1 commit)

**2. [Rule 1 - Bug] Additional deprecated reference in troubleshoot-agent line 728**
- **Found during:** Task 1 baseline scan
- **Issue:** Line 728 contained "Synthesizes findings with extended thinking"
- **Fix:** Changed to "Synthesizes findings with deep analysis"
- **Files modified:** .claude/agents/troubleshoot-agent.md
- **Verification:** Re-ran grep for "extended thinking" - zero results
- **Committed in:** b170ccc (Task 1 commit)

---

**Total deviations:** 2 auto-fixed (2 bugs - additional deprecated references not explicitly listed in plan)
**Impact on plan:** Both auto-fixes necessary for verification to pass. No scope creep.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- All agent/critic definitions are clean of deprecated config
- agent-profiles.json provides single source of truth for effort levels
- Ready for 14-02 (skill/workflow reference cleanup) which builds on this foundation
- Note: 8 skill files have pre-existing unstaged changes with related deprecated references (out of scope for this plan, covered by 14-02)

---
## Self-Check: PASSED

- All 14 files verified present on disk
- Both task commits verified in git log (b170ccc, 791ee97)
- SUMMARY.md verified present

---
*Phase: 14-platform-alignment*
*Completed: 2026-02-09*
