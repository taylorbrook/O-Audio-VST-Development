# ui-mockup Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-01-30

## Purpose

Orchestrate WebView UI mockup workflow - delegate design iteration to ui-design-agent and implementation scaffolding to ui-finalization-agent.

## This Agent DOES

- Gather UI requirements through tiered questions (Critical, Visual, Polish)
- Delegate mockup generation to ui-design-agent
- Support iterative design refinement (v1, v2, v3...)
- Generate implementation scaffolding (5 files) via ui-finalization-agent
- Create parameter-spec.md from finalized mockup (v1)
- Integrate with aesthetic library for template reuse

## This Agent DOES NOT

- Generate mockup files directly (always delegates to subagents)
- Implement the UI in C++ (use gui-agent in plugin-workflow instead)
- Create DSP or audio processing logic (use plugin-workflow instead)
- Modify existing plugin UI code (use plugin-improve instead)
- Handle plugin lifecycle operations (use plugin-lifecycle instead)

## Input Requirements

See: `.claude/schemas/agent-contracts/ui-mockup.input.json`

## Output Guarantees

See: `.claude/schemas/agent-contracts/ui-mockup.output.json`

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | plugin-ideation | Creative vision | Via parallel workflow path |
| Receives from | plugin-planning | Planning context | When UI design requested |
| Receives from | plugin-improve | Redesign request | For UI improvements |
| Outputs to | ui-design-agent | Requirements | Phase A design iteration |
| Outputs to | ui-finalization-agent | Approved design | Phase B implementation |
| Outputs to | ui-template-library | Aesthetic | When user saves template |
| Outputs to | plugin-planning | parameter-spec.md | After v1 finalization |

## Tool Inventory

1. Read - Load creative brief (BRIEF.md)
2. Read - Load aesthetic manifest
3. Read - Load existing parameter-spec.md
4. Task - Invoke ui-design-agent (Phase A)
5. Task - Invoke ui-finalization-agent (Phase B)
6. AskUserQuestion - Gather requirements (Phases 1-3)
7. AskUserQuestion - Present decision menus

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| gui-agent | ui-mockup creates the design specification and scaffolding; gui-agent (invoked by plugin-workflow Stage 3) implements the actual C++ integration. Use ui-mockup for design, gui-agent for implementation. |
| aesthetic-dreaming | ui-mockup creates plugin-specific mockups; aesthetic-dreaming creates reusable aesthetic templates without creating a plugin. |
| plugin-improve | ui-mockup handles UI design phase; plugin-improve handles implementation changes to working plugins. If redesigning existing plugin UI, plugin-improve may invoke ui-mockup. |
