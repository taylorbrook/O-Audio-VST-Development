# improve-milestone Boundaries

**Version:** 1.0.0
**Last Updated:** 2026-02-02

## Purpose

Manage complex improvements to completed plugins using GSD-style phase cycles (discuss → research → plan → execute → verify) with context clearing between phases.

## This Agent DOES

- Orchestrate 5-phase improvement cycle
- Track milestone state across context windows
- Delegate to domain-specific agents for execute phase
- Create backups before implementation
- Determine version bump type during planning
- Update CHANGELOG and create git tags
- Support pause/resume of multi-session work
- Clear context between phases to prevent overflow

## This Agent DOES NOT

- Modify plugins still in development (use plugin-workflow)
- Handle simple improvements (use plugin-improve for Tier 1-2)
- Skip the planning phase (required for milestones)
- Execute without backup verification
- Create new plugins (use plugin-ideation)
- Conduct research without structured output (use deep-research for ad-hoc)

## When to Use This vs /improve

| Scenario | Command | Reason |
|----------|---------|--------|
| Typo fix | /improve | Simple, single-context |
| Add parameter | /improve | Tier 1-2, manageable |
| New effect module | /improve-milestone | Multi-file, needs planning |
| UI redesign | /improve-milestone | Research-heavy, multi-session |
| Architecture refactor | /improve-milestone | High-risk, needs verification |
| Bug fix | /improve | Usually Tier 1-2 |
| Performance optimization | /improve-milestone | Research-heavy, measurement needed |

## Input Requirements

See: `.claude/schemas/agent-contracts/improve-milestone.input.json`

Required:
- `plugin_name`: Plugin to improve (must be Working/Installed)
- `milestone_description`: What to improve (or resume existing)

Optional:
- `skip_phases`: Array of phases to skip (discuss, research, verify)
- `resume`: Boolean to resume existing milestone

## Output Guarantees

See: `.claude/schemas/agent-contracts/improve-milestone.output.json`

Guaranteed on success:
- CONTEXT.md, RESEARCH.md, PLAN.md, SUMMARY.md, VERIFICATION.md
- STATUS.yaml with complete phase tracking
- Version bump applied
- CHANGELOG entry added
- Git commit and tag created
- Registry activeMilestone cleared

## Handoff Points

| Direction | Agent | Artifact | Condition |
|-----------|-------|----------|-----------|
| Receives from | /improve | Tier 3 detection | User chooses milestone |
| Receives from | /continue | activeMilestone | Resume existing |
| Receives from | User | Direct command | /improve-milestone |
| Delegates to | general-purpose | Phase work | discuss, research, plan, verify |
| Delegates to | dsp-agent | Execute work | DSP domain detected |
| Delegates to | gui-agent | Execute work | GUI domain detected |
| Delegates to | build-automation | Build/test | verify phase |

## Tool Inventory

**Orchestrator tools:**
1. Read - Load STATUS.yaml, registry.json, phase outputs
2. Write - Create phase outputs, update STATUS.yaml
3. Edit - Update registry.json, CHANGELOG.md
4. Bash - Git operations, backup creation
5. Task - Delegate to phase agents
6. AskUserQuestion - Phase completion menus

**Delegated operations:**
- Phase execution → appropriate agent
- Build/test → build-automation
- Installation → plugin-lifecycle

## Phase Agent Selection

| Phase | Agent Type | Rationale |
|-------|------------|-----------|
| discuss | general-purpose | Conversational, requirement gathering |
| research | general-purpose | Exploratory, needs flexibility |
| plan | general-purpose | Analysis, domain detection |
| execute | **domain-specific** | DSP/GUI patterns, real-time rules |
| verify | general-purpose | Comparison, validation logic |

**Execute phase agent selection:**
- `dsp-agent`: processBlock, filters, gain, audio processing
- `gui-agent`: WebView, relays, attachments, CSS
- `polish-agent`: presets, optimization, pluginval (if agent exists)
- `general-purpose`: mixed domain or unclear

## Overlap Resolution

| Similar Agent | How to Decide |
|---------------|---------------|
| plugin-improve | plugin-improve for Tier 1-2 (single context); improve-milestone for Tier 3 (phase cycles). /improve auto-detects and suggests. |
| deep-research | deep-research for ad-hoc investigation; improve-milestone research phase for structured milestone work. |
| plugin-workflow | plugin-workflow for initial implementation (new plugins); improve-milestone for improvements to completed plugins. |
| plugin-workflow | plugin-workflow for implementation stage phases; improve-milestone for post-completion improvement phases. |

## State Persistence

State survives context clearing:

**Primary state:** `plugins/[Name]/.planning/improvements/[slug]/STATUS.yaml`
- Current phase and status
- Detected domain and agent
- Version information
- Timestamps

**Secondary state:** `.planning/workflow/registry.json`
- `activeMilestone` field for quick lookup

**Phase outputs:** Individual markdown files in improvement directory
- Consumed by subsequent phases
- Preserved for documentation
