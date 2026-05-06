---
phase: 11-context-injection-pipeline
plan: 02
subsystem: agent-orchestration
tags: [inject-context, skill-integration, research-context, stage-agents, Task-invocation]
requires:
  - phase: 11-01
    provides: "inject-context.py utility script"
  - phase: 10
    provides: "resource-index.json, discover-resources.py"
provides:
  - "All skill orchestrators call inject-context.py before stage agent Task() invocations"
  - "Agent prompts receive research content inline without manual file reading"
  - "Stage-specific patterns auto-injected for stages 1-3"
  - "ROADMAP Phase 11 marked complete with updated success criteria"
affects:
  - "Phase 12: Accountability agents can now assume research_context is present in prompts"
  - "Phase 13: Maintenance tooling can verify injection works across all skill files"
tech-stack:
  added: []
  patterns:
    - "inject-context.py subprocess call before every Task() invocation"
    - "researchContext/research_context variable embedded inline in agent prompts"
    - "Domain-to-stage mapping for improve-milestone agent dispatch"
key-files:
  modified:
    - ".claude/skills/plugin-workflow/SKILL.md"
    - ".claude/skills/plugin-workflow/references/stage-1-foundation-shell.md"
    - ".claude/skills/plugin-workflow/references/stage-2-dsp.md"
    - ".claude/skills/plugin-workflow/references/stage-3-gui.md"
    - ".claude/skills/plugin-planning/SKILL.md"
    - ".claude/skills/plugin-planning/references/subagent-invocation.md"
    - ".claude/skills/improve-milestone/SKILL.md"
    - ".claude/skills/improve-milestone/references/phase-agents.md"
    - ".planning/ROADMAP.md"
key-decisions:
  - "research_context embedded directly in Task() prompt string (not separate parameter)"
  - "For phased implementations (stage 2b, 3b), inject context once before the loop, not per-phase"
  - "Domain-to-stage mapping in improve-milestone: dsp->2, gui->3, polish->4, general->0"
  - "Required Reading instructions completely removed (replaced by inline injection)"
patterns-established:
  - "inject-context.py call pattern: python3 .claude/scripts/inject-context.py --stage N --agent NAME --plugin PLUGIN"
  - "Graceful degradation: empty inject-context.py output is harmless in prompt strings"
duration: 4min
completed: 2026-02-05
---

# Phase 11 Plan 02: Skill Integration + ROADMAP Update Summary

**Connected inject-context.py to all 8 skill/reference files at 7+ Task() invocation points, replacing Required Reading instructions with inline research context injection**

## Performance

- Duration: 4 minutes
- Tasks: 3/3 complete (2 implementation + 1 verification-only)
- Commits: 2 task commits (Task 3 was verification-only, no files changed)
- Files modified: 9

## Accomplishments

1. **plugin-workflow integration (stages 1-4):** Added inject-context.py calls to SKILL.md execute phase and all 4 stage reference files (stage-1-foundation-shell.md, stage-2-dsp.md single+phased, stage-3-gui.md single+phased). Replaced all "Required Reading: Load stage-N-patterns.md" instructions with `${researchContext}` inline embedding.

2. **plugin-planning integration (stage 0):** Added inject-context.py call to SKILL.md dispatch sequence (new step 2) and subagent-invocation.md (new Research Context Injection section). Embedded `{research_context}` at end of prompt template.

3. **improve-milestone integration:** Added domain-to-stage mapping and inject-context.py call to SKILL.md Phase 4 Execute section. Added Research Context Injection section to phase-agents.md with `{research_context}` in the dsp-agent example prompt.

4. **ROADMAP updated:** Marked Phase 11 plans complete, updated progress table, confirmed success criteria wording already reflects "inject-context.py" and "skill orchestrators" (no "SubagentStart hook" references).

5. **End-to-end verification:** All 4 INJT requirements verified with evidence:
   - INJT-01: 8 files reference inject-context.py, all 5 stage/agent combinations produce valid output
   - INJT-02: research_context/researchContext embedded in all 8 Task() invocation files
   - INJT-03: Stages 1-3 auto-inject stage-N-patterns.md; Required Reading removed from all files
   - INJT-04: Max output ~3,043 tokens (Stage 2 dsp-agent), all within 4,000 token budget

## Task Commits

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Add injection to plugin-workflow and plugin-planning | d558732 | 6 skill/reference files |
| 2 | Add injection to improve-milestone and update ROADMAP | 80f3e0b | 2 skill/reference files + ROADMAP.md |
| 3 | End-to-end integration verification | (verification-only) | No file changes |

## Files Created/Modified

**Modified (9 files):**
- `.claude/skills/plugin-workflow/SKILL.md` -- inject-context.py call in run_execute_phase(), research_context in prompt
- `.claude/skills/plugin-workflow/references/stage-1-foundation-shell.md` -- researchContext injection, Required Reading removed
- `.claude/skills/plugin-workflow/references/stage-2-dsp.md` -- researchContext in both single-pass and phased Task() calls
- `.claude/skills/plugin-workflow/references/stage-3-gui.md` -- researchContext in both single-pass and phased Task() calls
- `.claude/skills/plugin-planning/SKILL.md` -- inject-context.py call added as step 2 in dispatch
- `.claude/skills/plugin-planning/references/subagent-invocation.md` -- Research Context Injection section + {research_context} in prompt
- `.claude/skills/improve-milestone/SKILL.md` -- domain-to-stage mapping + inject-context.py call
- `.claude/skills/improve-milestone/references/phase-agents.md` -- Research Context Injection section + {research_context} in prompt
- `.planning/ROADMAP.md` -- Phase 11 marked complete, progress table updated

## Decisions Made

1. **Inline embedding over separate parameter:** research_context is a simple string interpolation in the Task() prompt, not a separate named parameter. This ensures zero overhead if inject-context.py returns empty.

2. **Single injection per stage loop:** For phased implementations (stage-2-dsp 2b and stage-3-gui 3b), the inject-context.py call happens once before the for-loop, not inside each iteration. Same stage context applies to all phases within a stage.

3. **Domain-to-stage mapping for improve-milestone:** Maps execute agent names to discovery stages: dsp-agent=2, gui-agent=3, polish-agent=4, general-purpose=0. This enables the improvement workflow to leverage the same discovery system as the implementation workflow.

4. **Complete removal of Required Reading:** All "Required Reading: Load stage-N-patterns.md" instructions, "Read Required Reading (MANDATORY)" steps, and "CRITICAL: Read Required Reading BEFORE implementation" lines were removed. Stage patterns are now auto-injected by inject-context.py with higher quality (excerpts inline vs. file path instruction).

## Deviations from Plan

None -- plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None. All changes are to orchestration documentation files. inject-context.py (from Plan 01) is already in place.

## Next Phase Readiness

Phase 11 is now complete. Phase 12 (Accountability & Validation) can proceed:
- All skill orchestrators now inject research context into agent prompts
- The `resources_consulted` field can be added to agent report schemas (ACCT-01)
- SubagentStop hooks can compare injected resources against reported consultations (ACCT-03)
- The inject-context.py output includes resource paths that agents can reference in their reports

## Self-Check: PASSED
