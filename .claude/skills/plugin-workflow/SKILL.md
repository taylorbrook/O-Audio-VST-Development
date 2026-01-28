---
name: plugin-workflow
description: Orchestrates JUCE plugin implementation through stages 1-3 (Foundation, DSP, GUI) using subagent delegation with automatic validation after each stage. Use when implementing plugins after planning completes, or when resuming with /continue command. Invoked by /implement command.
allowed-tools:
  - Task # REQUIRED - All stages 1-3 MUST invoke subagents
  - Bash # For git commits
  - Read # For contracts
  - Write # For documentation
  - Edit # For state updates
preconditions:
  - ARCHITECTURE.md must exist at plugins/[Name]/.planning/research/ (from /plan)
  - ROADMAP.md must exist at plugins/[Name]/.planning/ (from /plan)
  - Status must be 🚧 Planning Complete OR resuming from 🚧 Stage 1+
  - Plugin must NOT be ✅ Working or 📦 Installed (use /improve instead)
---

# plugin-workflow Skill

**Purpose:** Pure orchestrator for stages 1-3 of JUCE plugin implementation with automatic validation after each stage. This skill delegates to specialized subagents and validation-agent for continuous quality assurance.

## Overview

Implementation milestones:
- **Build System Ready** (Stage 1): Create build system and implement parameters (foundation-shell-agent)
- **Audio Engine Working** (Stage 2): Implement audio processing (dsp-agent)
- **UI Integrated** (Stage 3): Connect WebView interface to audio engine (gui-agent)

Stage 0 (Research & Planning) is handled by `plugin-planning` skill.

After Stage 3 completes, plugin is ready for installation (no separate validation stage - validation is automatic and continuous).

## Delegation Protocol

**CRITICAL:** Stages 1-3 MUST invoke subagents via Task tool. This skill is a pure orchestrator and NEVER implements plugin code directly.

**Delegation sequence for every stage:**
1. Load contracts in parallel (ARCHITECTURE.md, ROADMAP.md, parameter-spec.md, BRIEF.md) from `plugins/[Name]/.planning/`
2. Read Required Reading (juce8-critical-patterns.md) once at workflow start
3. Construct minimal prompt with plugin name + stage + Required Reading
4. Invoke subagent via Task tool
5. After subagent returns, invoke validation-agent (ALL stages 1-3)
6. Execute checkpoint protocol (see references/checkpoint-protocol.md)

**Stage routing:**
- Stage 1 → foundation-shell-agent
- Stage 2 → dsp-agent
- Stage 3 → gui-agent

**Validation routing:**
After each stage completes, validation-agent runs automatically with enhanced runtime validation (compile-time + runtime tests). If validation fails with `continue_to_next_stage: false`, workflow BLOCKS until issues resolved.

## Preconditions

Before starting Stage 1, verify these contract files exist (plugin-local paths):
- `plugins/$PLUGIN_NAME/.planning/research/ARCHITECTURE.md` (from Stage 0)
- `plugins/$PLUGIN_NAME/.planning/ROADMAP.md` (from Stage 0)
- `plugins/$PLUGIN_NAME/.planning/BRIEF.md` (from ideation)
- `plugins/$PLUGIN_NAME/.planning/parameter-spec.md` (from UI mockup finalization)

**If parameter-spec-draft.md exists but parameter-spec.md missing:**
Block with message: "Draft parameters found, but full specification required. Complete UI mockup workflow to generate parameter-spec.md. Run: /start [PluginName] → option 2 (Full UI mockup first)"

**If contracts missing:**
Block and instruct user to run `/plan [PluginName]` to complete Stage 0.

See [references/precondition-checks.md](references/precondition-checks.md) for implementation.

## Resume Entry Point

When resuming via `/continue [PluginName]`:

1. Verify state integrity (see references/state-management.md#verifyStateIntegrity)
2. Parse `plugins/[Name]/.planning/STATUS.md` for current stage and workflow mode
3. Verify contracts unchanged since last checkpoint (checksums match)
4. Verify git working directory clean
5. Verify PLUGINS.md status matches STATUS.md stage

**If all checks pass:** Resume at stage specified in STATUS.md
**If any fail:** Present recovery menu (reconcile / clean working directory / review changes)

## Workflow Mode

Determine whether to auto-progress (express mode) or present menus (manual mode).

**Mode sources (priority order):**
1. Environment variables: `WORKFLOW_MODE=express|manual`
2. STATUS.md field (for resumed workflows)
3. Default to "manual"

**Express mode behavior:**
- Auto-progress through stages without menus
- Drops to manual on ANY error (build failures, validation failures, etc.)
- Final menu always appears after Stage 3 (plugin complete)

See [references/workflow-mode.md](references/workflow-mode.md) for implementation.

## Stage Dispatcher

**Entry point:** Called by /implement or /continue after plugin-planning completes.

**Dispatch flow:**
1. Verify state integrity → BLOCK if corrupted (exit 2 → run /reconcile)
2. Check preconditions → BLOCK if failed
3. **Automatic brief sync** (before Stage 1 only, if mockup exists) → See [references/creative-brief-sync.md](references/creative-brief-sync.md)
4. Route to subagent based on stage
5. Pass contracts and Required Reading to subagent
6. Wait for subagent completion
7. Invoke validation-agent with enhanced runtime validation
8. Execute checkpoint protocol

See [references/dispatcher-pattern.md](references/dispatcher-pattern.md) for full algorithm.

## Phase-Aware Dispatch

For Stages 2-3 with complexity ≥3, use phase-aware dispatch to incrementally implement complex plugins.

**When to use:**
- Stage 2 (DSP) or Stage 3 (GUI)
- Complexity score ≥3 (from ROADMAP.md)
- ROADMAP.md contains phase markers (### Phase 2.X or ### Phase 3.X)

**How it works:**
1. Detect phases by scanning ROADMAP.md for phase markers
2. Loop through phases sequentially (Phase 2.1 → 2.2 → 2.3...)
3. Invoke subagent once per phase with phase-specific prompt
4. Run validation-agent after each phase
5. Execute checkpoint protocol after each phase
6. Present decision menu showing progress ("Phase 2 of 4 complete")

**CRITICAL:** Never send "Implement ALL phases" to subagent. This caused DrumRoulette Stage 2 compilation errors. Phase-aware dispatch is MANDATORY for complex plugins.

See [references/phase-aware-dispatch.md](references/phase-aware-dispatch.md) for detailed algorithm.

## Checkpoint Protocol

After EVERY subagent return, execute this 6-step sequence:

1. **Verify state update:** Check subagent updated STATUS.md and PLUGINS.md
2. **Fallback state update:** If verification fails, orchestrator updates state
3. **Invoke validation:** Run validation-agent for ALL stages 1-3 (BLOCKING on runtime failures)
4. **Commit stage:** Auto-commit all changes with git
5. **Verify checkpoint:** Validate all steps completed successfully
6. **Handle checkpoint:** Present menu (manual mode) or auto-progress (express mode)

**Checkpoint applies to:**
- Simple plugins (complexity ≤2): After stages 1, 2, 3
- Complex plugins (complexity ≥3): After stage 1 AND after EACH DSP/GUI phase (2.X, 3.X)

See [references/checkpoint-protocol.md](references/checkpoint-protocol.md) for implementation.

## Validation Integration

Stages 1-3 invoke validation-agent with enhanced runtime validation after subagent completes:
- **BLOCKING on runtime failures:** If `status: FAIL` and `continue_to_next_stage: false`, workflow stops
- Runs compile-time checks (contract matching, implementation correctness)
- Runs runtime tests (load plugin, process audio, parameter changes) when binary available
- Returns JSON report with status, checks, recommendation
- Max 500 tokens per report

**Blocking behavior:**
- If validation passes (PASS/WARNING): Continue to next stage
- If validation fails with `continue_to_next_stage: true`: Present warning, allow continuation
- If validation fails with `continue_to_next_stage: false`: BLOCK workflow, present error menu

See [references/validation-integration.md](references/validation-integration.md) for functions.

## Subagent Handoff Protocol

Subagents update state files AND return JSON report:

```json
{
  "status": "success" | "error",
  "stage": 1-3,
  "completionStatement": "...",
  "filesCreated": [...],
  "nextSteps": [...],
  "stateUpdated": true | false,
  "stateUpdateError": "..." (optional)
}
```

**Verification:**
1. Check `stateUpdated` field in JSON report
2. If true: Verify STATUS.md actually changed
3. If false/missing: Trigger orchestrator fallback

**Fallback:** Orchestrator reads current state, updates fields, writes back.

See [references/state-management.md](references/state-management.md) for fallback implementation.

## Required Reading Injection

Each stage receives a focused subset of patterns to reduce context size:

- **Stage 1:** `troubleshooting/patterns/stage-1-patterns.md` (7 patterns - build/config)
- **Stage 2:** `troubleshooting/patterns/stage-2-patterns.md` (3 patterns - DSP/threading)
- **Stage 3:** `troubleshooting/patterns/stage-3-patterns.md` (14 patterns - WebView/GUI)

**Implementation:**
1. Subagents read their stage-specific patterns file themselves
2. Full 22-pattern file available at `juce8-critical-patterns.md` for edge cases
3. No need to pass patterns in orchestrator prompts - agents self-load

## Reference Files

Each stage has detailed documentation in references/:

- [stage-1-foundation-shell.md](references/stage-1-foundation-shell.md) - foundation-shell-agent prompt template
- [stage-2-dsp.md](references/stage-2-dsp.md) - dsp-agent prompt template
- [stage-3-gui.md](references/stage-3-gui.md) - gui-agent prompt template
- [state-management.md](references/state-management.md) - State functions
- [dispatcher-pattern.md](references/dispatcher-pattern.md) - Routing logic
- [precondition-checks.md](references/precondition-checks.md) - Contract validation
- [phase-aware-dispatch.md](references/phase-aware-dispatch.md) - Complex plugin handling
- [workflow-mode.md](references/workflow-mode.md) - Express vs manual mode
- [checkpoint-protocol.md](references/checkpoint-protocol.md) - 6-step checkpoint sequence
- [validation-integration.md](references/validation-integration.md) - Validation-agent functions (enhanced runtime validation)
- [creative-brief-sync.md](references/creative-brief-sync.md) - Automatic brief update from mockup
- [error-handling.md](references/error-handling.md) - Error patterns and recovery
- [integration-contracts.md](references/integration-contracts.md) - Component contracts

## Integration Points

**Invoked by:**
- `/implement` command (after plugin-planning completes)
- `/continue` command (for stages 1-3)
- `context-resume` skill (when resuming implementation)

**Invokes via Task tool:**
- `foundation-shell-agent` (Stage 1) - REQUIRED
- `dsp-agent` (Stage 2) - REQUIRED
- `gui-agent` (Stage 3) - REQUIRED
- `validation-agent` (Stages 1-3) - REQUIRED, BLOCKING on runtime failures

**Also invokes:**
- `build-automation` skill (build verification)
- `plugin-lifecycle` skill (if user chooses to install)

**Reads (contracts from plugins/[Name]/.planning/):**
- research/ARCHITECTURE.md, ROADMAP.md, BRIEF.md, parameter-spec.md

**Creates/Updates:**
- STATUS.md (stage progress, in plugins/[Name]/.planning/)
- stages/[N]-[name]/CONTEXT.md (discuss phase output for each stage)
- stages/[N]-[name]/PLAN.md (execution plan for each stage)
- stages/[N]-[name]/SUMMARY.md (completion summary for each stage)
- stages/[N]-[name]/VERIFICATION.md (verify phase output for each stage)

**Updates:**
- PLUGINS.md (status after each stage)
- STATUS.md (after each stage)

**Deletes after Stage 3:**
- Nothing - STATUS.md is preserved as project history

## Error Handling

**Contract files missing before Stage 1:**
Block and instruct user to run `/plan [PluginName]`.

**Build fails during subagent execution:**
Subagent returns error. Present menu:
1. Investigate (deep-research)
2. Show code
3. Show build output
4. Manual fix (resume with /continue)

**State mismatch detected (exit 2):**
BLOCKING error - user must run `/reconcile [PluginName]` to fix.

**Validation fails with continue_to_next_stage: false:**
BLOCKING error. Present menu with investigation options. Workflow cannot proceed until issues resolved.

**Validation fails with continue_to_next_stage: true:**
Present warning, allow user to decide whether to continue or fix issues first.

See [references/error-handling.md](references/error-handling.md) for detailed patterns.

## Decision Menu Protocol

**Use inline numbered menus for:**
- After EVERY stage completion (checkpoint gates)
- Build failure recovery
- Test failure investigation
- Phase completion (for complex plugins)

**Format:**
```
✓ [Milestone name]

What's next?

1. [Next milestone action] (recommended)
2. [Run tests] - Verify implementation
3. [Pause workflow] - Resume anytime
4. [Review code] - See what was implemented
5. Other

Choose (1-5): _
```

**Express mode:** Skip menus and auto-progress to next stage (except final stage).
**Manual mode:** ALWAYS wait for user response.

## Success Criteria

Workflow succeeds when:
- All subagents (stages 1-3) invoked successfully via Task tool
- Plugin compiles without errors at each stage
- All validation passes (or explicitly allowed to continue with warnings)
- All stages completed in sequence (1 → 2 → 3)
- Decision menus presented after EVERY stage (manual mode)
- PLUGINS.md updated to ✅ Working after Stage 3
- STATUS.md shows complete progress history
- Each stage has CONTEXT.md, PLAN.md, SUMMARY.md, VERIFICATION.md
- Git history shows atomic commits for each stage

## Anti-Patterns

Common pitfalls to AVOID:

**CRITICAL:**
- ❌ Implementing stage logic directly in orchestrator
- ✓ ALWAYS use Task tool to invoke appropriate subagent

**CRITICAL:**
- ❌ Sending "Implement ALL phases" to subagent for Stages 2-3
- ✓ ALWAYS detect phases in plan.md and loop through them one at a time

**CRITICAL:**
- ❌ Proceeding to next stage when validation fails with continue_to_next_stage: false
- ✓ BLOCK workflow and present error menu until issues resolved

**HIGH:**
- ❌ Not verifying subagent updated state
- ✓ Check stateUpdated field, verify STATUS.md changed, fallback if needed

**HIGH:**
- ❌ Skipping phase detection for Stages 2-3 when complexity ≥3
- ✓ Read ROADMAP.md to check for phases BEFORE invoking dsp-agent or gui-agent

**HIGH:**
- ❌ Skipping validation after subagent completes
- ✓ ALWAYS invoke validation-agent after each stage (1-3)

**MEDIUM:**
- ❌ Not injecting Required Reading to subagents
- ✓ Agents read stage-specific patterns (stage-1/2/3-patterns.md) to prevent repeat mistakes
