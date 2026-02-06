---
name: plugin-workflow
description: Orchestrates JUCE plugin implementation through stages 1-4 with GSD phase cycles. Each stage runs discuss → research → plan → execute → verify. Use when implementing plugins after planning completes, or when resuming with /continue command. Invoked by /implement command.
allowed-tools:
  - Task # REQUIRED - All phases invoke subagents
  - Bash # For git commits, builds
  - Read # For contracts and state
  - Write # For documentation
  - Edit # For state updates
  - AskUserQuestion # For discuss phase
preconditions:
  - ARCHITECTURE.md must exist at plugins/[Name]/.planning/research/ (from /plan)
  - ROADMAP.md must exist at plugins/[Name]/.planning/ (from /plan)
  - Status must be 🚧 Planning Complete OR resuming from 🚧 Stage 1+
  - Plugin must NOT be ✅ Working or 📦 Installed (use /improve instead)
---

## Contract

- Input schema: `.claude/schemas/agent-contracts/plugin-workflow.input.json`
- Output schema: `.claude/schemas/agent-contracts/plugin-workflow.output.json`
- Boundaries: See `BOUNDARIES.md` in this directory

## Contract Validation

Before processing any request, validate inputs against the contract:

1. **Load schema:** `.claude/schemas/agent-contracts/plugin-workflow.input.json`
2. **Validate:** Check all required fields present, types match, constraints satisfied
3. **On violation:** Stop immediately. Report error using format from `.claude/skills/contract-validation/SKILL.md`:
   ```
   CONTRACT VIOLATION: plugin-workflow input invalid

   Field: {field_name}
   Expected: {from_schema}
   Received: {actual_value}

   Fix: {actionable_instruction}
   ```
4. **On success:** Proceed to main agent logic

See `.claude/skills/contract-validation/SKILL.md` for full validation protocol.

# plugin-workflow Skill

**Purpose:** Orchestrate stages 1-4 of JUCE plugin implementation with GSD phase cycles. Each stage runs a full discuss → research → plan → execute → verify cycle before advancing.

## Overview

Implementation stages:
- **Stage 1: Foundation** - CMake, project structure, APVTS parameters
- **Stage 2: DSP** - Audio processing, algorithms
- **Stage 3: GUI** - WebView UI, parameter binding
- **Stage 4: Polish** - Presets, optimization, edge cases

Each stage runs 5 phases:
```
┌─────────┐   ┌──────────┐   ┌──────┐   ┌─────────┐   ┌────────┐
│ discuss │ → │ research │ → │ plan │ → │ execute │ → │ verify │
└─────────┘   └──────────┘   └──────┘   └─────────┘   └────────┘
```

## Phase Delegation

Each phase invokes a specialized agent via Task tool:

| Phase | Agent | Output |
|-------|-------|--------|
| discuss | plugin-discuss-agent | CONTEXT.md |
| research | gsd-phase-researcher | RESEARCH.md |
| plan | gsd-planner | PLAN.md |
| execute | Stage-specific agent | SUMMARY.md |
| verify | gsd-verifier + validation-agent | VERIFICATION.md |

**Execute phase agents:**
- Stage 1 → foundation-shell-agent
- Stage 2 → dsp-agent
- Stage 3 → gui-agent
- Stage 4 → polish-agent

## Main Orchestration Loop

```python
def orchestrate_implementation(plugin_name, start_stage, skip_phases, express_mode):
    """Main orchestration loop for plugin implementation."""

    stages = ["1-foundation", "2-dsp", "3-gui", "4-polish"]
    phases = ["discuss", "research", "plan", "execute", "verify"]

    # Load current state from registry
    current_state = load_plugin_state(plugin_name)
    current_stage = current_state.stage or start_stage
    current_phase = current_state.phase or "discuss"

    # Find starting position
    stage_idx = stages.index(current_stage)

    for stage in stages[stage_idx:]:
        # Create stage directory
        ensure_stage_directory(plugin_name, stage)

        # Update registry
        update_registry(plugin_name, stage=stage, phase="discuss")

        for phase in phases:
            # Check if phase should be skipped
            if phase in skip_phases and phase != "plan" and phase != "execute":
                mark_phase_skipped(plugin_name, stage, phase)
                continue

            # Update current phase
            update_registry(plugin_name, phase=phase)
            update_status_md(plugin_name, stage, phase)

            # Run phase
            result = run_phase(plugin_name, stage, phase)

            if result.status == "error":
                # Drop to manual mode on error
                present_error_menu(plugin_name, stage, phase, result.error)
                return  # Exit, user will /continue

            # Phase checkpoint
            commit_phase(plugin_name, stage, phase)

            # Present menu (manual mode) or auto-advance (express mode)
            if not express_mode:
                action = present_phase_menu(plugin_name, stage, phase)
                if action == "pause":
                    create_handoff(plugin_name, stage, phase)
                    return

        # Stage complete - HANDOFF POINT
        mark_stage_complete(plugin_name, stage)
        commit_stage(plugin_name, stage)

        # CRITICAL: Always present handoff at stage boundary
        # See .claude/references/handoff-protocol.md
        present_stage_handoff(plugin_name, stage)  # Includes /clear instruction
        return  # STOP - do not auto-advance

    # All stages complete
    mark_plugin_complete(plugin_name)
    present_completion_handoff(plugin_name)  # Final handoff with /install-plugin
```

## Phase Execution

### Discuss Phase

Gather context through interactive questioning (or auto-generate from existing docs in express mode).

```python
def run_discuss_phase(plugin_name, stage, express_mode):
    if express_mode:
        # Auto-generate CONTEXT.md from existing docs
        context = compile_context_from_docs(plugin_name, stage)
        write_context_md(plugin_name, stage, context)
    else:
        # Interactive questioning via plugin-discuss-agent
        invoke_task(
            subagent_type="general-purpose",
            prompt=f"""
            You are a plugin-discuss-agent gathering context for {stage} of {plugin_name}.

            Load and review:
            - BRIEF.md (creative vision)
            - ARCHITECTURE.md (DSP design)
            - Previous stage VERIFICATION.md (if exists)

            Ask 3-5 clarifying questions about this stage's requirements.
            After answers, create stages/{stage}/CONTEXT.md.
            """
        )
    return PhaseResult(status="success")
```

### Research Phase

Investigate implementation approach.

```python
def run_research_phase(plugin_name, stage):
    invoke_task(
        subagent_type="gsd-phase-researcher",
        prompt=f"""
        Research implementation approach for {plugin_name} stage {stage}.

        Context: Load stages/{stage}/CONTEXT.md

        Research:
        1. Relevant JUCE APIs and patterns
        2. Algorithm approaches
        3. Existing modules that could be reused
        4. Pitfalls from troubleshooting knowledge base

        Output: stages/{stage}/RESEARCH.md
        """
    )
    return PhaseResult(status="success")
```

### Plan Phase

Create execution plan with task breakdown.

```python
def run_plan_phase(plugin_name, stage):
    invoke_task(
        subagent_type="gsd-planner",
        prompt=f"""
        Create execution plan for {plugin_name} stage {stage}.

        Load:
        - stages/{stage}/CONTEXT.md
        - stages/{stage}/RESEARCH.md (if exists)
        - ARCHITECTURE.md
        - ROADMAP.md

        Create stages/{stage}/PLAN.md with:
        1. Goal statement
        2. Numbered task breakdown
        3. Files to create/modify
        4. Dependencies between tasks
        5. Success criteria
        """
    )
    return PhaseResult(status="success")
```

### Execute Phase

Run stage-specific implementation agent.

```python
def run_execute_phase(plugin_name, stage):
    # Map stage to agent
    stage_agents = {
        "1-foundation": "foundation-shell-agent",
        "2-dsp": "dsp-agent",
        "3-gui": "gui-agent",
        "4-polish": "polish-agent"
    }
    agent = stage_agents[stage]

    # Load contracts and plan
    contracts = load_contracts(plugin_name)
    plan = read_file(f"plugins/{plugin_name}/.planning/stages/{stage}/PLAN.md")

    # Get research context via injection utility
    stage_number = int(stage[0])
    research_context = run_bash(
        f"python3 .claude/scripts/inject-context.py --stage {stage_number} --agent {agent} --plugin {plugin_name}"
    )

    # Invoke agent
    result = invoke_task(
        subagent_type=agent,
        prompt=f"""
        Implement {stage} for {plugin_name}.

        PLAN.md tasks:
        {plan}

        Contracts:
        - BRIEF.md: {contracts.brief_summary}
        - ARCHITECTURE.md: {contracts.arch_summary}
        - parameter-spec.md: {contracts.params}

        {research_context}

        After implementation, create stages/{stage}/SUMMARY.md.
        """
    )

    if result.status == "error":
        return PhaseResult(status="error", error=result.error)

    return PhaseResult(status="success")
```

### Verify Phase

Validate goal achievement through goal-backward analysis.

```python
def run_verify_phase(plugin_name, stage):
    # Run goal verification
    invoke_task(
        subagent_type="gsd-verifier",
        prompt=f"""
        Verify {stage} achievement for {plugin_name}.

        Load:
        - stages/{stage}/PLAN.md (goals and success criteria)
        - stages/{stage}/SUMMARY.md (what was built)

        Verify each success criterion is met.
        Output: stages/{stage}/VERIFICATION.md with pass/fail for each goal.
        """
    )

    # Run technical validation
    validation_result = invoke_task(
        subagent_type="validation-agent",
        prompt=f"Validate {plugin_name} after {stage} completion."
    )

    if validation_result.status == "FAIL" and not validation_result.continue_to_next_stage:
        return PhaseResult(status="error", error=validation_result.issues)

    return PhaseResult(status="success")
```

## State Management

### Registry Updates

Update `.claude/plugin-registry.json` at every phase transition:

```python
def update_registry(plugin_name, stage=None, phase=None, status=None):
    registry = load_registry()
    plugin = registry["plugins"][plugin_name]

    if stage:
        plugin["stage"] = stage
    if phase:
        plugin["phase"] = phase
    if status:
        plugin["status"] = status

    plugin["lastActivity"] = datetime.now().isoformat()
    save_registry(registry)
```

### STATUS.md Updates

Update `plugins/[Name]/.planning/STATUS.md` with phase progress:

```markdown
## Phase Progress

### Stage 2: DSP
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-01-29 | |
| research | ✓ | 2026-01-29 | |
| plan | → | | |
| execute | | | |
| verify | | | |
```

### Git Commits

Commit after each phase and stage:

```bash
# Phase commit
git add plugins/${PLUGIN_NAME}/.planning/stages/${STAGE}/
git commit -m "phase: ${PLUGIN_NAME} ${STAGE}/${PHASE} complete"

# Stage commit
git add plugins/${PLUGIN_NAME}/
git commit -m "stage: ${PLUGIN_NAME} ${STAGE} complete"
```

## Decision Menus

### Phase Completion Menu (Manual Mode)

```
━━━ PHASE COMPLETE ━━━

**[PluginName]** — Stage 2 (DSP) / research phase

RESEARCH.md created with:
- JI ratio calculation algorithms
- scala-tuning-engine module recommendation
- Voice management patterns

━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ▶ Next Up

**plan phase** — Create execution plan

**Step 1:** `/clear` — fresh context window (recommended)
**Step 2:** `/implement [PluginName]`

━━━━━━━━━━━━━━━━━━━━━━━━━━━

**Also available:**

- Re-run research phase
- View RESEARCH.md
- Pause workflow
- Other

━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### Stage Completion Menu (Handoff Point)

**CRITICAL: This is a handoff point. Present clean continuation format, do NOT auto-proceed.**

```
---

## ✓ Stage 2 (DSP) Complete

**[PluginName]** — All 5 phases verified

| Phase | Status |
|-------|--------|
| discuss | ✓ |
| research | ✓ |
| plan | ✓ |
| execute | ✓ |
| verify | ✓ |

---

## ▶ Next Up

**Stage 3: GUI** — WebView UI integration and parameter binding

**Step 1:** `/clear` — fresh context window
**Step 2:** `/implement [PluginName]`

---

**Also available:**

- `/test [PluginName]` → Run additional tests
- Review stage artifacts
- Save for later (handoff file created)

---
```

**Do NOT auto-advance to Stage 3.** Present the handoff with explicit two-step format and STOP.

See: `.claude/references/handoff-protocol.md` for standard handoff format.

### Express Mode Output

**Express mode auto-advances PHASES within a stage but STOPS at stage boundaries to present handoff.**

See: `.claude/references/handoff-protocol.md` for handoff format.

```
/implement O-IntonationPad --express

━━━ Stage 1: Foundation ━━━
  [auto] discuss → Context compiled from BRIEF.md
  [auto] research → Foundation patterns loaded
  [auto] plan → PLAN.md generated (6 tasks)
  [auto] execute → foundation-shell-agent running...
         ✓ CMakeLists.txt created
         ✓ PluginProcessor.cpp created
         ✓ Parameters implemented
  [auto] verify → Build successful, parameters validated
  ✓ Stage 1 complete

---
## ▶ Next Up
**Stage 2: DSP** — Audio processing and algorithms

**Step 1:** `/clear` — fresh context window
**Step 2:** `/implement O-IntonationPad`
---
```

**STOP - do not auto-advance across stage boundaries.** Each stage completion presents a handoff.

## Error Handling

### Build Failure

```
✗ Build failed at Stage 2, Phase execute

Error: undefined reference to 'JIChordGenerator::calculateRatio'

Options:
1. View full build log
2. Investigate with /research
3. Re-run execute phase
4. Pause for manual fix
5. Other

Choose (1-5): _
```

### Verification Failure

```
⚠ Verification failed for Stage 2 (DSP)

Issues found:
- ✗ Chord voices not tuned to JI ratios
  Expected: 3:2 fifth ratio
  Actual: 1.498 (equal temperament)

Options:
1. View VERIFICATION.md details
2. Re-run execute phase with fix
3. Investigate issue
4. Accept with warning (not recommended)
5. Other

Choose (1-5): _
```

## Preconditions

Before starting Stage 1, verify:
- `plugins/[Name]/.planning/research/ARCHITECTURE.md` exists
- `plugins/[Name]/.planning/ROADMAP.md` exists
- `plugins/[Name]/.planning/parameter-spec.md` exists
- Plugin status is 🚧 Stage 0 or 🚧 Stage 1-4

**If contracts missing:** Block with instructions to run `/plan [PluginName]`

## Skip Flags

Supported skip flags (from `/implement` command):
- `--skip-discuss` → Skip discuss phase, auto-generate CONTEXT.md
- `--skip-research` → Skip research phase, proceed to plan
- `--skip-verify` → Skip verify phase (not recommended)

**Cannot skip:** plan, execute

## Reference Files

- [checkpoint-protocol.md](references/checkpoint-protocol.md) - Git commit patterns
- [dispatcher-pattern.md](references/dispatcher-pattern.md) - Stage dispatch logic
- [state-management.md](references/state-management.md) - Registry and STATUS.md
- [validation-integration.md](references/validation-integration.md) - Validation agent
- [error-handling.md](references/error-handling.md) - Error recovery
- [workflow-mode.md](references/workflow-mode.md) - Express vs manual

## Integration Points

**Invoked by:**
- `/implement` command
- `/continue` command

**Invokes via Task tool:**
- plugin-discuss-agent (discuss phase)
- gsd-phase-researcher (research phase)
- gsd-planner (plan phase)
- foundation-shell-agent, dsp-agent, gui-agent, polish-agent (execute phase)
- gsd-verifier, validation-agent (verify phase)

**Reads:**
- `.claude/plugin-registry.json`
- `plugins/[Name]/.planning/*`

**Writes:**
- `.claude/plugin-registry.json`
- `plugins/[Name]/.planning/STATUS.md`
- `plugins/[Name]/.planning/stages/*/CONTEXT.md`
- `plugins/[Name]/.planning/stages/*/RESEARCH.md`
- `plugins/[Name]/.planning/stages/*/PLAN.md`
- `plugins/[Name]/.planning/stages/*/SUMMARY.md`
- `plugins/[Name]/.planning/stages/*/VERIFICATION.md`
- `PLUGINS.md`
