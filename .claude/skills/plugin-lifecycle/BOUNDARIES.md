# plugin-lifecycle Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-01-30

## Purpose

Manage complete plugin lifecycle - install, uninstall, reset, destroy with proper state tracking and safety features.

## This Agent DOES

- Install plugins to system folders (Mode 1: ~/Library/Audio/Plug-Ins/)
- Uninstall plugins from system folders while preserving source (Mode 2)
- Reset plugins to ideation state, preserving idea/mockups (Mode 3)
- Destroy plugins completely with backup (Mode 4)
- Clear DAW caches (Ableton, Logic Pro)
- Verify permissions and installation integrity
- Update PLUGINS.md and NOTES.md status

## This Agent DOES NOT

- Build plugins (use build-automation instead)
- Run tests (use plugin-testing instead)
- Continue to next workflow stage (terminal skill)
- Modify source code (use plugin-improve instead)
- Create new plugins (use plugin-ideation instead)
- Handle workflow orchestration (use plugin-workflow instead)

## Input Requirements

See: `.claude/schemas/agent-contracts/plugin-lifecycle.input.json`

## Output Guarantees

See: `.claude/schemas/agent-contracts/plugin-lifecycle.output.json`

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | plugin-workflow | Install request | After Stage 4 completion |
| Receives from | plugin-improve | Install request | After successful improvement |
| Receives from | User | Lifecycle command | /install-plugin, /uninstall, /reset-to-ideation, /destroy |
| Outputs to | None | Terminal skill | Returns control to caller |

## Tool Inventory

1. Bash - Copy binaries to system folders (Mode 1)
2. Bash - Remove binaries from system folders (Mode 2)
3. Bash - Remove implementation directories (Mode 3)
4. Bash - Remove all plugin files (Mode 4)
5. Bash - Set file permissions (chmod 755)
6. Bash - Clear AU cache (killall AudioComponentRegistrar)
7. Bash - Verify file existence and sizes
8. Read - Extract PRODUCT_NAME from CMakeLists.txt
9. Read - Check current plugin status
10. Write - Create backup metadata (Mode 4)
11. Edit - Update PLUGINS.md status
12. Edit - Update NOTES.md timeline

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| build-automation | build-automation handles the build process and may install as part of build; plugin-lifecycle handles installation as a standalone lifecycle operation. For explicit install/uninstall commands, use plugin-lifecycle. |
| plugin-workflow | plugin-workflow orchestrates implementation; plugin-lifecycle handles post-implementation lifecycle operations. plugin-workflow may invoke plugin-lifecycle for installation after Stage 4. |
