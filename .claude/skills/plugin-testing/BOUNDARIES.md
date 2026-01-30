# plugin-testing Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-01-30

## Purpose

Validate audio plugins through automated tests, pluginval, or manual DAW testing (manual testing skill only).

## This Agent DOES

- Run automated tests (Mode 1: ~2 min)
- Run pluginval validation at strictness level 10 (Mode 2: ~5-10 min)
- Guide manual DAW testing with customized checklists (Mode 3: ~30-60 min)
- Parse test results and generate reports
- Update PLUGINS.md test status
- Delegate to deep-research for non-trivial test failures

## This Agent DOES NOT

- Run automatic validation during workflow stages (validation-agent handles that)
- Fix code issues directly (presents investigation menu, delegates to deep-research)
- Build plugins (uses build-automation via Mode 2)
- Continue workflow after testing (returns control to invoker)
- Install plugins after testing (use plugin-lifecycle instead)

## Input Requirements

See: `.claude/schemas/agent-contracts/plugin-testing.input.json`

## Output Guarantees

See: `.claude/schemas/agent-contracts/plugin-testing.output.json`

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | User | /test command | Manual testing request |
| Receives from | plugin-improve | Test request | After improvement implementation |
| Outputs to | deep-research | Test failure context | When user selects "Investigate failures" |
| Outputs to | User | Test report | After any mode completes |

## Tool Inventory

1. Read - Load PLUGINS.md (verify plugin exists)
2. Read - Load parameter-spec.md (generate customized checklist)
3. Read - Check Tests/ directory existence
4. Bash - Build and run automated tests
5. Bash - Run pluginval validation
6. Bash - Check pluginval installation
7. Task - Delegate to deep-research for complex failures
8. Edit - Update STATUS.md with test status
9. Edit - Update PLUGINS.md test_status column

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| validation-agent | validation-agent runs automatically during workflow (after each stage); plugin-testing is for manual testing via /test command. Use plugin-testing for explicit testing requests outside workflow. |
| deep-research | plugin-testing identifies failures; deep-research investigates complex/non-trivial issues. If test failure is documented in troubleshooting.md, plugin-testing handles it. If novel, delegate to deep-research. |
