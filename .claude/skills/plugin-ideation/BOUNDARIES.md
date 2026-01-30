# plugin-ideation Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-01-30

## Purpose

Adaptive brainstorming for plugin concepts and improvements through interactive questioning and creative brief generation.

## This Agent DOES

- Gather creative vision through adaptive questioning (4-question batches)
- Generate creative briefs (BRIEF.md) for new plugins
- Route between new plugin mode and improvement mode
- Update PLUGINS.md with new plugin entries
- Create initial STATUS.md for new plugins
- Support continuous iteration until user says "finalize"

## This Agent DOES NOT

- Implement any code changes (use plugin-improve instead)
- Create technical specifications like ARCHITECTURE.md (use plugin-planning instead)
- Modify existing plugin code (use plugin-improve instead)
- Run builds or tests (use build-automation or plugin-testing instead)
- Design UI mockups (use ui-mockup instead)
- Execute the improvement proposal (use plugin-improve instead)

## Input Requirements

See: `.claude/schemas/agent-contracts/plugin-ideation.input.json`

## Output Guarantees

See: `.claude/schemas/agent-contracts/plugin-ideation.output.json`

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | User | Free-form description | /start command or natural language |
| Receives from | plugin-improve | Brainstorm request | When improvement request is vague |
| Outputs to | plugin-planning | BRIEF.md | After new plugin ideation completes |
| Outputs to | ui-mockup | Creative vision | Via parallel workflow path |
| Outputs to | plugin-improve | Improvement proposal | After improvement ideation completes |

## Tool Inventory

1. Read - Check PLUGINS.md for existing plugin (mode detection)
2. Read - Load existing plugin context for improvement mode
3. Write - Create BRIEF.md (new plugin mode)
4. Write - Create STATUS.md (initial state)
5. Write - Create improvement proposal (improvement mode)
6. Bash - Check plugin directory existence
7. Edit - Update PLUGINS.md with new entry

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| plugin-improve | plugin-ideation explores ideas through brainstorming; plugin-improve implements specific changes. If request is vague, plugin-improve routes to plugin-ideation for brainstorming. |
| plugin-planning | plugin-ideation captures creative vision (BRIEF.md); plugin-planning creates technical specifications (ARCHITECTURE.md, ROADMAP.md). Ideation comes first. |
