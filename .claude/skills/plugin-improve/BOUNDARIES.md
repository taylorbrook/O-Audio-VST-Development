# plugin-improve Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-01-30

## Purpose

Fix bugs and add features to completed plugins with versioning, backups, regression testing, and changelog automation.

## This Agent DOES

- Make changes to plugins with status "Working" or "Installed"
- Create backups before any modifications
- Determine version bump type (PATCH/MINOR/MAJOR)
- Investigate root causes (Tier 1-3 investigation)
- Delegate to deep-research for complex issues (Tier 3)
- Update CHANGELOG.md with version entries
- Detect and handle breaking changes
- Support rollback via backup restoration

## This Agent DOES NOT

- Modify plugins still in development (use plugin-workflow instead)
- Create new plugins from scratch (use plugin-ideation instead)
- Handle initial implementation (use plugin-workflow instead)
- Conduct research without context (use deep-research instead)
- Skip backup verification before changes

## Input Requirements

See: `.claude/schemas/agent-contracts/plugin-improve.input.json`

## Output Guarantees

See: `.claude/schemas/agent-contracts/plugin-improve.output.json`

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | User | /improve command | Direct improvement request |
| Receives from | plugin-ideation | Improvement proposal | After improvement brainstorming |
| Receives from | deep-research | Research findings | Via handoff protocol |
| Outputs to | plugin-ideation | Brainstorm request | When request is vague |
| Outputs to | deep-research | Investigation request | For Tier 3 complex issues |
| Outputs to | build-automation | Build request | After implementation |
| Outputs to | plugin-testing | Test request | After build succeeds |
| Outputs to | plugin-lifecycle | Install request | If user requests installation |

## Tool Inventory

1. Read - Load current state (CHANGELOG.md, PLUGINS.md, git log)
2. Read - Check backup existence
3. Write - Create backup
4. Write - Update CHANGELOG.md
5. Edit - Modify source files
6. Edit - Update PLUGINS.md (version, status, last_updated)
7. Edit - Update NOTES.md (status, timeline)
8. Bash - Git operations (stage, commit, tag)
9. Bash - Verify backup integrity
10. Task - Delegate to deep-research (Tier 3 investigation)
11. Task - Delegate to build-automation (build phase)
12. Task - Delegate to plugin-testing (test phase)
13. Task - Delegate to plugin-lifecycle (installation)

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| plugin-workflow | plugin-improve is for post-completion changes to working plugins; plugin-workflow is for initial implementation. Check plugin status: if "Working" or "Installed", use plugin-improve. If "In Development", use plugin-workflow. |
| plugin-ideation | plugin-improve implements specific changes; plugin-ideation explores ideas through brainstorming. If improvement request is vague, plugin-improve routes to plugin-ideation first. |
| deep-research | plugin-improve handles straightforward fixes (Tier 1-2); deep-research handles complex/novel problems (Tier 3). plugin-improve auto-detects tier and delegates when needed. |
