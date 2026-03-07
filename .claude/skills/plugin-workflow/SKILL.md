---
name: plugin-workflow
description: Orchestrates JUCE plugin implementation through stages 1-4 with GSD phase cycles. Each stage runs discuss → research → plan → execute → verify. Use for full orchestration (/implement) or individual phase commands (/plugin-discuss, /plugin-research, /plugin-plan, /plugin-execute, /plugin-verify).
allowed-tools:
  - Task # REQUIRED - All phases invoke subagents
  - Bash # For git commits, builds
  - Read # For contracts and state
  - Write # For documentation
  - Edit # For state updates
  - AskUserQuestion # For discuss phase
commands:
  - name: plugin-discuss
    description: Interactive context gathering for a stage
    args: "[plugin_name?] [stage?] [--skip-*]"
  - name: plugin-research
    description: Investigate implementation approach
    args: "[plugin_name?] [stage?] [--skip-*]"
  - name: plugin-plan
    description: Create execution plan with task breakdown
    args: "[plugin_name?] [stage?] [--skip-*]"
  - name: plugin-execute
    description: Run stage-specific agent
    args: "[plugin_name?] [stage?] [--skip-*]"
  - name: plugin-verify
    description: Validate goal achievement
    args: "[plugin_name?] [stage?] [--skip-*]"
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

**Purpose:** Orchestrate stages 1-4 of JUCE plugin implementation with GSD phase cycles. Each stage runs a full discuss → research → plan → execute → verify cycle before advancing. Supports both full orchestration via `/implement` and granular control via individual phase commands (`/plugin-discuss`, `/plugin-research`, `/plugin-plan`, `/plugin-execute`, `/plugin-verify`).

## Overview

Implementation stages:
- **Stage 1: Foundation** - CMake, project structure, APVTS parameters
- **Stage 2: DSP** - Audio processing, algorithms
- **Stage 3: GUI** - WebView UI, parameter binding
- **Stage 4: Polish** - Presets, optimization, edge cases

Each stage runs 5 phases:
```
┌─────────────────────────────────────────────────────────────┐
│                        STAGE N                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────┐   ┌──────────┐   ┌──────┐   ┌─────────┐       │
│  │ discuss │ → │ research │ → │ plan │ → │ execute │ →     │
│  └────┬────┘   └────┬─────┘   └──────┘   └────┬────┘       │
│       │             │                          │            │
│       │ skippable   │ skippable                │            │
│       ▼             ▼                          ▼            │
│  CONTEXT.md    RESEARCH.md              SUMMARY.md         │
│                                                             │
│                                         ┌────────┐         │
│                                       → │ verify │ → DONE  │
│                                         └────┬───┘         │
│                                              │              │
│                                              │ skippable    │
│                                              ▼              │
│                                      VERIFICATION.md       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                      ADVANCE TO STAGE N+1
```

## Phase Delegation

Each phase invokes a specialized agent via Task tool:

| Phase | Agent | Output |
|-------|-------|--------|
| discuss | plugin-discuss-agent | CONTEXT.md |
| research | gsd-phase-researcher (simple) OR research-lead (complex) | RESEARCH.md |
| plan | gsd-planner | PLAN.md |
| execute | Stage-specific agent | SUMMARY.md |
| critic-review | critic-orchestrator (post-execute, pre-verify) | Unified critic report |
| verify | gsd-verifier + validation-agent | VERIFICATION.md |

**Execute phase agents:**
- Stage 0 → plugin-ideation
- Stage 1 → foundation-shell-agent
- Stage 2 → dsp-agent
- Stage 3 → gui-agent
- Stage 4 → polish-agent

## Main Orchestration Loop

```python
def orchestrate_implementation(plugin_name, start_stage, skip_phases, workflow_mode):
    """Main orchestration loop for plugin implementation.

    workflow_mode: "manual" | "express" | "auto"
    CTXP-03: Auto mode generates all planning artifacts without user interaction.
    """

    stages = ["1-foundation", "2-dsp", "3-gui", "4-polish"]
    phases = ["discuss", "research", "plan", "execute", "verify"]

    # Load current state from STATUS.md
    current_state = load_plugin_state(plugin_name)
    current_stage = current_state.stage or start_stage
    current_phase = current_state.phase or "discuss"

    # Find starting position
    stage_idx = stages.index(current_stage)

    for stage in stages[stage_idx:]:
        # Create stage directory
        ensure_stage_directory(plugin_name, stage)

        # Update STATUS.md
        update_status_md(plugin_name, stage=stage, phase="discuss")

        for phase in phases:
            # Check if phase should be skipped
            if phase in skip_phases and phase != "plan" and phase != "execute":
                mark_phase_skipped(plugin_name, stage, phase)
                continue

            # Update current phase
            update_status_md(plugin_name, stage, phase)

            # Run phase
            result = run_phase(plugin_name, stage, phase)

            if result.status == "error":
                # Drop to manual mode on error (express and auto both fall back)
                present_error_menu(plugin_name, stage, phase, result.error)
                return  # Exit, user will /continue

            # Phase checkpoint
            commit_phase(plugin_name, stage, phase)

            # Present menu (manual mode) or auto-advance (express/auto mode)
            if workflow_mode == "manual":
                action = present_phase_menu(plugin_name, stage, phase)
                if action == "pause":
                    create_handoff(plugin_name, stage, phase)
                    return
            # express and auto modes always auto-advance at phase checkpoints

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

Gather context through interactive questioning, auto-generate from existing docs (express mode), or fully auto-generate from contracts (auto mode).

- **Manual mode:** Run interactive questioning via plugin-discuss-agent to produce CONTEXT.md
- **Express mode:** Auto-advance but still run each phase normally (may pause at decisions)
- **Auto mode:** Auto-generate CONTEXT.md directly from existing contracts (BRIEF.md, parameter-spec.md, ARCHITECTURE.md, previous stage VERIFICATION.md) without any user interaction. No questions are asked. CONTEXT.md is compiled from existing documents and marked as auto-generated.

```python
def run_discuss_phase(plugin_name, stage, workflow_mode):
    if workflow_mode == "auto":
        # Auto-generate CONTEXT.md from contracts without interaction
        brief = read_file(f"plugins/{plugin_name}/.planning/BRIEF.md")
        params = read_file(f"plugins/{plugin_name}/.planning/parameter-spec.md")
        arch = read_file(f"plugins/{plugin_name}/.planning/research/ARCHITECTURE.md")
        prev_verification = read_file_safe(f"plugins/{plugin_name}/.planning/stages/{prev_stage}/VERIFICATION.md")
        context = compile_context_from_contracts(brief, params, arch, prev_verification)
        context.source = "Auto-Generated from existing contracts (no interactive session)"
        write_context_md(plugin_name, stage, context)
    elif workflow_mode == "express":
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

Investigate implementation approach. In auto mode, invoke the research agent non-interactively.

- **Manual/Express mode:** Run research agent normally (may ask clarifying questions in manual)
- **Auto mode:** Invoke the research agent with an explicit non-interactive directive. The research agent reads CONTEXT.md and produces RESEARCH.md without asking clarifying questions. When workflow mode is auto, append to the research agent prompt: "Mode: Non-interactive (auto mode) -- do not ask clarifying questions."

```python
def run_research_phase(plugin_name, stage, workflow_mode="manual"):
    auto_directive = ""
    if workflow_mode == "auto":
        auto_directive = "\n\nMode: Non-interactive (auto mode) -- do not ask clarifying questions."

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
        {auto_directive}
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

### Auto Mode Output

**Auto mode auto-generates planning artifacts (CONTEXT.md, RESEARCH.md, PLAN.md) without interaction, then runs execute and verify normally. Like express mode, auto mode STOPS at stage boundaries to present handoff.**

```
/implement O-IntonationPad --auto

--- Stage 1: Foundation ---
  [auto] discuss -> Context auto-generated from BRIEF.md + contracts
  [auto] research -> Research generated non-interactively
  [auto] plan -> PLAN.md auto-generated
  [auto] execute -> foundation-shell-agent running...
         ✓ CMakeLists.txt created
         ✓ PluginProcessor.cpp created
         ✓ Parameters implemented
  [auto] verify -> Build successful, parameters validated
  ✓ Stage 1 complete

---
## ▶ Next Up
**Stage 2: DSP** — Audio processing and algorithms

**Step 1:** `/clear` — fresh context window
**Step 2:** `/implement O-IntonationPad --auto`
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

### Phase Out of Order

```
Error: Cannot run 'execute' phase - 'plan' phase not complete.

Current state: Stage 2-dsp, Phase: discuss

Run these phases first:
1. /plugin-discuss O-IntonationPad 2-dsp
2. /plugin-research O-IntonationPad 2-dsp (or --skip-research)
3. /plugin-plan O-IntonationPad 2-dsp
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

Supported skip flags (from `/implement` command or individual phase commands):
- `--skip-discuss` → Skip discuss phase, auto-generate CONTEXT.md
- `--skip-research` → Skip research phase, proceed to plan
- `--skip-verify` → Skip verify phase (not recommended)

**Cannot skip:** plan, execute

**Note:** `--auto` auto-generates discuss and research phases (producing CONTEXT.md and RESEARCH.md from contracts), unlike `--skip-discuss`/`--skip-research` which skip those phases entirely without producing artifacts.

**Individual phase command example:**
```
/plugin-execute O-IntonationPad 2-dsp --skip-discuss --skip-research
```
When used with individual phase commands, skip flags mark the specified earlier phases as skipped and proceed to the invoked phase directly.

## Research Team Integration

Research at Stage 0 uses one of two paths based on plugin complexity:

**Simple plugins (complexity 1-3, fewer than 5 parameters, no custom DSP):**
- Use standard sequential `gsd-phase-researcher` (existing behavior, unchanged)
- Single researcher produces RESEARCH.md directly

**Complex plugins (complexity 4+, custom DSP algorithms, 10+ parameters, novel techniques):**
- Use `research-lead` agent which spawns 2-3 parallel researchers via Agent Teams
- Research-lead analyzes creative brief, assigns domains dynamically at runtime
- Researchers produce independent findings, then debate conflicts (max 3 rounds)
- If conflicts unresolved after 3 rounds: BLOCK planning and present both positions to user
- Output: merged research document with synthesis section
- Graceful degradation: if Agent Teams experimental feature fails, fall back to sequential subagent research (each researcher runs as subagent, debate via shared files)

The choice is made automatically by the workflow orchestrator based on creative brief analysis during the research phase. See [research-team-protocol.md](references/research-team-protocol.md) for the full protocol.

## Post-Stage Critic Review

After each stage execution (Stage 1, 2, 3, 4), a critic review step runs before the verify phase:

```
┌─────────┐   ┌──────────┐   ┌──────┐   ┌─────────┐   ┌───────────────┐   ┌────────┐
│ discuss │ → │ research │ → │ plan │ → │ execute │ → │ critic-review │ → │ verify │
└─────────┘   └──────────┘   └──────┘   └─────────┘   └───────────────┘   └────────┘
```

**Flow:**
1. Execute stage agent (existing behavior)
2. Invoke `critic-orchestrator` agent to spawn parallel domain critics as subagents
3. Collect unified review report (severity-ranked: blocker > warning > note)
4. **If any blocker-severity findings:** Return to execute phase with fix list (do NOT proceed to verify)
5. **If no blockers:** Proceed to verify phase (existing behavior)

The critic-orchestrator determines which critics to run based on the stage-to-critic mapping table. See [critic-review-protocol.md](references/critic-review-protocol.md) for the full protocol.

## Summary Template Auto-Selection

After plan execution completes, the summary template is selected based on task metrics:

| Template | Conditions |
|----------|-----------|
| `summary-minimal.md` | 1-2 tasks AND fewer than 3 files modified AND no DSP/GUI tasks |
| `summary-standard.md` | 2-3 tasks OR 3-6 files modified (default) |
| `summary-complex.md` | 3+ tasks AND 7+ files modified OR any task involved DSP algorithm implementation OR task required gap closure |

Template paths: `/Users/taylorbrook/.claude/get-shit-done/templates/summary-{variant}.md`

Selection is automatic based on the metrics collected during execution. If no template variant files exist, fall back to the standard `summary.md` template.

## Integration Points

**Invoked by:**
- `/implement` command (full orchestration)
- `/continue` command (resume)
- `/plugin-discuss` command (individual phase)
- `/plugin-research` command (individual phase)
- `/plugin-plan` command (individual phase)
- `/plugin-execute` command (individual phase)
- `/plugin-verify` command (individual phase)

**Invokes via Task tool:**
- plugin-discuss-agent (discuss phase)
- gsd-phase-researcher (research phase, simple plugins)
- research-lead (research phase, complex plugins -- see Research Team Integration above)
- gsd-planner (plan phase)
- foundation-shell-agent, dsp-agent, gui-agent, polish-agent (execute phase)
- critic-orchestrator (post-execute review -- see Post-Stage Critic Review above)
- gsd-verifier, validation-agent (verify phase)

**Reads:**
- `plugins/[Name]/.planning/*`

**Writes:**
- `plugins/[Name]/.planning/STATUS.md`
- `plugins/[Name]/.planning/stages/*/CONTEXT.md`
- `plugins/[Name]/.planning/stages/*/RESEARCH.md`
- `plugins/[Name]/.planning/stages/*/PLAN.md`
- `plugins/[Name]/.planning/stages/*/SUMMARY.md`
- `plugins/[Name]/.planning/stages/*/VERIFICATION.md`
- `PLUGINS.md`

## Reference Files

- [checkpoint-protocol.md](references/checkpoint-protocol.md) - Git commit patterns
- [dispatcher-pattern.md](references/dispatcher-pattern.md) - Stage dispatch logic
- [state-management.md](references/state-management.md) - Registry and STATUS.md
- [validation-integration.md](references/validation-integration.md) - Validation agent
- [error-handling.md](references/error-handling.md) - Error recovery
- [workflow-mode.md](references/workflow-mode.md) - Express vs manual
- [research-team-protocol.md](references/research-team-protocol.md) - Parallel research team spawning and debate format
- [critic-review-protocol.md](references/critic-review-protocol.md) - Post-stage critic review and severity enforcement
- [branching-strategy.md](references/branching-strategy.md) - Configurable branching modes (none/phase/milestone)
