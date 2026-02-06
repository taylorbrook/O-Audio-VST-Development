---
phase: 12-accountability-validation
plan: 02
subsystem: agent-accountability
tags: [resource-accountability, hook-integration, agent-instructions, warning-only]

dependency-graph:
  requires: ["12-01"]
  provides: ["agent-accountability-pipeline", "hook-accountability-integration"]
  affects: ["13-xx"]

tech-stack:
  added: []
  patterns: ["warning-only validation", "stderr real-time feedback", "universal accountability instructions"]

key-files:
  created: []
  modified:
    - .claude/agents/dsp-agent.md
    - .claude/agents/gui-agent.md
    - .claude/agents/foundation-shell-agent.md
    - .claude/agents/research-planning-agent.md
    - .claude/agents/polish-agent.md
    - .claude/agents/troubleshoot-agent.md
    - .claude/agents/ui-design-agent.md
    - .claude/agents/ui-finalization-agent.md
    - .claude/agents/validation-agent.md
    - .claude/agents/aesthetics-agent.md
    - .claude/agents/music-theory-agent.md
    - .claude/hooks/SubagentStop.sh

decisions:
  - id: accountability-block-placement
    decision: "Place accountability instructions BEFORE JSON report section in each agent"
    rationale: "Agents see the instructions before constructing their report, maximizing compliance"
  - id: hook-placement-before-relevance-check
    decision: "Place accountability validation BEFORE the relevance check early-exit in SubagentStop.sh"
    rationale: "research-planning-agent and polish-agent would exit before reaching accountability if placed after esac; must run for all 5 AGENT_STAGE_MAP agents"
  - id: separate-plugin-name-variable
    decision: "Use PLUGIN_NAME_ACCT for accountability block to avoid collision with PLUGIN_NAME used by Layer 0/1"
    rationale: "The existing PLUGIN_NAME extraction happens after the relevance check; accountability needs its own extraction before that check"

metrics:
  duration: "~2 minutes"
  completed: 2026-02-06
---

# Phase 12 Plan 02: Agent Instructions & Hook Integration Summary

**One-liner:** All 11 agents get resource accountability reporting instructions; SubagentStop hook calls validator for real-time warnings on stderr.

## What Was Done

### Task 1: Agent Markdown Updates (11 files)

Added identical `<resource_accountability>` instruction block to all 11 agent markdown files. The block instructs agents to include a `resources_consulted` field in their JSON reports when they receive a `<research_context>` block in their prompt.

For the 5 main stage agents (dsp-agent, gui-agent, foundation-shell-agent, research-planning-agent, polish-agent), also updated one example JSON success report to show `resources_consulted` as a top-level field alongside `agent`, `status`, `outputs`.

The 6 additional agents (troubleshoot-agent, ui-design-agent, ui-finalization-agent, validation-agent, aesthetics-agent, music-theory-agent) received only the instruction block -- they do not currently have formal JSON report examples that need updating.

### Task 2: SubagentStop Hook Integration (1 file)

Integrated the accountability validator (from Plan 01) into SubagentStop.sh with key structural decision: the accountability block runs BEFORE the relevance check that gates Layer 0/1 validation to only foundation-shell-agent, dsp-agent, and gui-agent. This ensures research-planning-agent and polish-agent (which are in the validator's AGENT_STAGE_MAP but not in the stage-specific validation list) still get accountability validation.

The integration:
- Extracts `agent_type` (with `subagent_name` fallback) and `agent_transcript_path` from hook input JSON
- Calls `validate-resource-accountability.py` with agent type, plugin name, and transcript path
- Routes stderr warnings to the user's terminal for real-time visibility
- Intentionally ignores the exit code -- accountability never blocks workflow
- Uses a separate `PLUGIN_NAME_ACCT` variable to avoid collision with the existing `PLUGIN_NAME` extraction that happens later

## Task Commits

| Task | Name | Commit | Files Changed |
|------|------|--------|---------------|
| 1 | Add resource accountability instructions to all agent markdown files | c9aee97 | 11 agent .md files |
| 2 | Integrate accountability validator into SubagentStop hook | 450dd2b | SubagentStop.sh |

## Decisions Made

1. **Accountability block placement:** Placed BEFORE JSON report section in each agent (not at end of file) so agents see instructions before constructing their report.

2. **Hook placement before relevance check:** The existing relevance check exits early for non-stage-specific agents. Accountability must run before this check to cover all 5 agents in AGENT_STAGE_MAP.

3. **Separate plugin name variable:** Used `PLUGIN_NAME_ACCT` instead of reusing `PLUGIN_NAME` to avoid variable collision with the Layer 0/1 section that extracts `PLUGIN_NAME` after the relevance check.

## Deviations from Plan

None -- plan executed exactly as written.

## Verification Results

| Check | Result |
|-------|--------|
| All 11 agents contain `resources_consulted` | PASS (11/11) |
| All 11 agents contain `resource_accountability` | PASS (11/11) |
| dsp-agent.md has >= 3 references to `resources_consulted` | PASS (3) |
| SubagentStop.sh bash syntax valid | PASS |
| SubagentStop.sh calls validate-resource-accountability.py | PASS |
| Existing validators unchanged | PASS (6 references intact) |
| No hooks.json changes needed | PASS |

## Success Criteria Met

- ACCT-02: All 5 stage agents + 6 additional agents have reporting instructions
- ACCT-03: SubagentStop hook logs warnings when MUST-READ resources skipped (warning only, never blocks)
- Real-time visibility: warnings appear on stderr during workflow execution
- No regression: existing hook validation passes unchanged

## Next Phase Readiness

Phase 12 is now complete (2/2 plans). The full accountability pipeline is operational:
1. Schema extended with `resources_consulted` field (Plan 01)
2. Validator script created with warning-only guarantee (Plan 01)
3. All agents instructed to report resource usage (Plan 02)
4. Hook integration calls validator for real-time feedback (Plan 02)

Ready to proceed to Phase 13 or project completion.

## Self-Check: PASSED
