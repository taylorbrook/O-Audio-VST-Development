# plugin-planning Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-01-30

## Purpose

Handle Stage 0 (Research & Planning) through subagent delegation, creating foundation contracts (ARCHITECTURE.md, ROADMAP.md) that guide implementation.

## This Agent DOES

- Orchestrate Stage 0 with GSD-style discuss/research/plan cycle
- Delegate to research-planning-agent subagent
- Create DSP architecture specification (ARCHITECTURE.md)
- Create implementation roadmap with complexity score (ROADMAP.md)
- Validate preconditions (BRIEF.md, parameter-spec.md must exist)
- Present decision menu after planning completes

## This Agent DOES NOT

- Create creative briefs (use plugin-ideation instead)
- Execute implementation stages 1-4 (use plugin-workflow instead)
- Design UI mockups (use ui-mockup instead)
- Improve completed plugins (use plugin-improve instead)
- Conduct deep technical research (use deep-research instead)

## Input Requirements

See: `.claude/schemas/agent-contracts/plugin-planning.input.json`

## Output Guarantees

See: `.claude/schemas/agent-contracts/plugin-planning.output.json`

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | plugin-ideation | BRIEF.md | After ideation completes |
| Receives from | ui-mockup | parameter-spec.md | After UI finalization |
| Outputs to | plugin-workflow | ARCHITECTURE.md, ROADMAP.md | When user selects "Start implementation" |
| Outputs to | deep-research | Research request | When user selects "Deep research" |
| Outputs to | ui-mockup | Planning context | When user selects UI design |

## Tool Inventory

1. Task - Invoke research-planning-agent subagent
2. Read - Load BRIEF.md (creative vision)
3. Read - Load parameter-spec.md or parameter-spec-draft.md
4. Read - Load mockup YAML files (if exist)
5. Write - Create ARCHITECTURE.md (via subagent)
6. Write - Create ROADMAP.md (via subagent)
7. Write - Create Stage 0 CONTEXT.md
8. Edit - Update STATUS.md with Stage 0 progress
9. Bash - Git commits for Stage 0 artifacts
11. WebSearch - Professional plugin research
12. Grep - Search existing implementations
13. Glob - Find reference files

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| plugin-ideation | plugin-ideation captures creative vision (BRIEF.md); plugin-planning creates technical specification (ARCHITECTURE.md, ROADMAP.md). Check what exists: if no BRIEF.md, use ideation first. |
| deep-research | plugin-planning does research within Stage 0 context; deep-research is for complex investigations that need multi-level autonomous research. Use deep-research for novel/complex problems that Stage 0 research cannot resolve. |
| plugin-workflow | plugin-planning handles Stage 0; plugin-workflow handles Stages 1-4. Planning creates contracts, workflow implements them. |
