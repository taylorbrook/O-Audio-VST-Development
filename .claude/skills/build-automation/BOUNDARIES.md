# build-automation Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-01-30

## Purpose

Orchestrate plugin builds and installation via build script with comprehensive failure handling.

## This Agent DOES

- Execute build script (scripts/build-and-install.sh) with appropriate flags
- Handle context-aware build flags (--no-install for Stage 2, full build for others)
- Present structured 5-option failure menu on build errors
- Display context-appropriate success menus based on invoking stage
- Clear AU/VST caches before installation
- Create build logs in logs/[PluginName]/

## This Agent DOES NOT

- Continue workflow after completion (returns control to invoker)
- Update STATUS.md or PLUGINS.md (invoker's responsibility)
- Implement code fixes on build failure (presents menu for user decision)
- Auto-retry failed builds without user decision
- Run tests (use plugin-testing instead)
- Handle plugin installation independently (use plugin-lifecycle for standalone installation)

## Input Requirements

See: `.claude/schemas/agent-contracts/build-automation.input.json`

## Output Guarantees

See: `.claude/schemas/agent-contracts/build-automation.output.json`

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | plugin-workflow | Build request | During execute phase |
| Receives from | plugin-improve | Build request | After implementation |
| Receives from | plugin-lifecycle | Build request | For verification builds |
| Outputs to | troubleshooter | Build log | When user selects "Investigate" |
| Outputs to | invoking skill | Success/failure status | Always after completion |

## Tool Inventory

1. Bash - Execute build script
2. Bash - Clear AU cache (killall AudioComponentRegistrar)
3. Bash - Remove old plugin binaries
4. Bash - Copy new binaries to system folders
5. Read - Parse CMakeLists.txt for PRODUCT_NAME
6. Read - Extract error context from build log
7. Edit - Update build log
8. Write - Create build log file

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| plugin-lifecycle | build-automation handles the build process; plugin-lifecycle handles installation as a lifecycle operation. build-automation may invoke installation as part of build, but standalone installation uses plugin-lifecycle. |
| plugin-workflow | plugin-workflow orchestrates the overall workflow and invokes build-automation for build operations. build-automation is a leaf skill that returns control to invoker. |
