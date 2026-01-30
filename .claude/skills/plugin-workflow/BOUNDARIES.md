# plugin-workflow Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-01-30

## Purpose

Orchestrate stages 1-4 of JUCE plugin implementation with GSD phase cycles (discuss, research, plan, execute, verify).

## This Agent DOES

- Run full implementation workflow from Stage 1 (Foundation) through Stage 4 (Polish)
- Execute GSD phase cycles within each stage
- Delegate to stage-specific subagents (foundation-shell-agent, dsp-agent, gui-agent, polish-agent)
- Update workflow state (registry, STATUS.md, PLUGINS.md)
- Handle express mode (auto-advance) and manual mode (decision menus)
- Manage phase/stage checkpoints and git commits

## This Agent DOES NOT

- Create creative briefs or ideate new plugins (use plugin-ideation instead)
- Create ARCHITECTURE.md or ROADMAP.md (use plugin-planning instead)
- Improve completed plugins (use plugin-improve instead)
- Run manual testing outside workflow (use plugin-testing instead)
- Design UI mockups (use ui-mockup instead)
- Install/uninstall plugins (use plugin-lifecycle instead)
- Conduct deep research investigations (use deep-research instead)

## Input Requirements

See: `.claude/schemas/agent-contracts/plugin-workflow.input.json`

## Output Guarantees

See: `.claude/schemas/agent-contracts/plugin-workflow.output.json`

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | plugin-planning | ARCHITECTURE.md, ROADMAP.md | After Stage 0 completes |
| Receives from | context-resume | STATUS.md | When resuming paused workflow |
| Outputs to | validation-agent | Stage artifacts | After each stage execute phase |
| Outputs to | build-automation | Build request | During execute phases |
| Outputs to | plugin-lifecycle | Plugin binaries | After Stage 4 completion |

## Tool Inventory

1. Task - Invoke stage subagents (foundation-shell-agent, dsp-agent, gui-agent, polish-agent)
2. Task - Invoke phase agents (plugin-discuss-agent, gsd-phase-researcher, gsd-planner, gsd-verifier, validation-agent)
3. Bash - Git commits at phase/stage boundaries
4. Bash - Build operations via build-automation
5. Read - Load contracts (ARCHITECTURE.md, ROADMAP.md, parameter-spec.md)
6. Read - Load state (registry, STATUS.md)
7. Write - Create stage documentation (CONTEXT.md, RESEARCH.md, PLAN.md, SUMMARY.md, VERIFICATION.md)
8. Edit - Update state files (registry, STATUS.md, PLUGINS.md)
9. AskUserQuestion - Discuss phase questioning (manual mode)

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| plugin-improve | plugin-workflow is for initial implementation (Stages 1-4); plugin-improve is for post-completion changes to working plugins. Check plugin status: if Working/Installed, use plugin-improve. |
| plugin-planning | plugin-planning handles Stage 0 (creates ARCHITECTURE.md, ROADMAP.md); plugin-workflow handles Stages 1-4 (implementation). |
| validation-agent | validation-agent is a subagent invoked BY plugin-workflow during verify phase; it does not operate independently. |
