# deep-research Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-01-30

## Purpose

Multi-level autonomous investigation for complex JUCE plugin development problems using graduated research depth protocol.

## This Agent DOES

- Conduct Level 1 quick checks (5-10 min, local docs)
- Conduct Level 2 moderate investigation (15-30 min, JUCE docs/forums)
- Conduct Level 3 deep research (30-60 min, parallel subagents, Opus + extended thinking)
- Synthesize findings from multiple sources
- Generate research reports with confidence assessment
- Present decision menus at each level completion
- Hand off to plugin-improve via Skill tool when user selects "Apply solution"

## This Agent DOES NOT

- Edit code files or implement solutions (READ-ONLY skill)
- Run builds or tests
- Modify contracts or configurations
- Make changes without user decision
- Skip levels (unless user explicitly requests starting level)
- Use Sonnet for Level 3 (requires Opus)

## Input Requirements

See: `.claude/schemas/agent-contracts/deep-research.input.json`

## Output Guarantees

See: `.claude/schemas/agent-contracts/deep-research.output.json`

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | plugin-testing | Test failure context | Non-trivial failures |
| Receives from | plugin-improve | Investigation request | Tier 3 issues |
| Receives from | plugin-planning | Research request | Complex DSP questions |
| Receives from | build-automation | Build failure context | User selects "Investigate" |
| Outputs to | plugin-improve | Research findings | User selects "Apply solution" |
| Outputs to | troubleshooting-docs | Novel solutions | For documentation capture |

## Tool Inventory

1. Read - Search local troubleshooting docs
2. Read - Load relevant code files for analysis
3. Grep - Search documentation patterns
4. Glob - Find relevant files
5. WebSearch - JUCE forum search
6. WebSearch - GitHub issue search
7. Task - Spawn parallel research subagents (Level 3)
8. Skill - Invoke plugin-improve (handoff)

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| gsd-phase-researcher | gsd-phase-researcher is lightweight research within a workflow phase; deep-research is heavyweight multi-level investigation for complex/novel problems. Use deep-research when phase research cannot resolve the issue. |
| plugin-improve | deep-research investigates problems; plugin-improve implements solutions. deep-research is READ-ONLY and hands off to plugin-improve for implementation. |
| troubleshooting-docs | deep-research finds solutions; troubleshooting-docs captures and documents them. After deep-research resolves a novel issue, troubleshooting-docs should capture it. |
