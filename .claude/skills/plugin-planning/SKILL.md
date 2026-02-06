---
name: plugin-planning
description: Orchestrates Stage 0 research and planning for JUCE plugins - creates ARCHITECTURE.md and ROADMAP.md through subagent delegation. Use when BRIEF.md exists and plugin needs DSP architecture specification, complexity assessment, or implementation planning. Invoke via /plan command, natural language (plan architecture, research DSP), or as first implementation step.
allowed-tools:
  - Read # For contracts and references
  - Write # For ARCHITECTURE.md, ROADMAP.md
  - Edit # For state updates (PLUGINS.md, STATUS.md)
  - Bash # For git commits, precondition checks
  - Task # For subagent delegation
  - WebSearch # For professional plugin research
  - Grep # For searching existing implementations
  - Glob # For finding reference files
preconditions:
  - BRIEF.md must exist in plugins/[Name]/.planning/
  - Parameter specification must exist (parameter-spec.md OR parameter-spec-draft.md) in plugins/[Name]/.planning/
  - Plugin must NOT already be past Stage 0
---

## Contract

- Input schema: `.claude/schemas/agent-contracts/plugin-planning.input.json`
- Output schema: `.claude/schemas/agent-contracts/plugin-planning.output.json`
- Boundaries: See `BOUNDARIES.md` in this directory

## Contract Validation

Before processing any request, validate inputs against the contract:

1. **Load schema:** `.claude/schemas/agent-contracts/plugin-planning.input.json`
2. **Validate:** Check all required fields present, types match, constraints satisfied
3. **On violation:** Stop immediately. Report error using format from `.claude/skills/contract-validation/SKILL.md`:
   ```
   CONTRACT VIOLATION: plugin-planning input invalid

   Field: {field_name}
   Expected: {from_schema}
   Received: {actual_value}

   Fix: {actionable_instruction}
   ```
4. **On success:** Proceed to main agent logic

See `.claude/skills/contract-validation/SKILL.md` for full validation protocol.

# plugin-planning Skill

**Purpose:** Handle Stage 0 (Research & Planning - consolidated) through subagent delegation. This skill creates the foundation contracts (ARCHITECTURE.md, ROADMAP.md) that guide implementation using the GSD-style discuss → research → plan → execute → verify cycle.

**Invoked by:** `/plan` command or as first step of `/implement` workflow

**Planning location:** `plugins/[Name]/.planning/` (plugin-local, not repo root)

---

## Entry Point

**Check preconditions first:** See [references/preconditions.md](references/preconditions.md) for detailed validation logic.

Quick validation:
1. BRIEF.md must exist at `plugins/[Name]/.planning/`
2. Parameter specification required (parameter-spec.md OR parameter-spec-draft.md) in `.planning/`
3. Plugin status must be ≤ Stage 0 (not already in implementation)
4. Detect existing contracts (research/ARCHITECTURE.md, ROADMAP.md) for resume logic

If all preconditions pass → proceed to Stage 0

---

## Stage 0: Research & Planning

**Goal:** Create DSP architecture specification (ARCHITECTURE.md) AND implementation roadmap (ROADMAP.md)
**Duration:** 5-35 minutes (complexity-dependent)
**Implementation:** Delegated to research-planning-agent subagent via Task tool

**GSD Cycle for Stage 0:**
```
DISCUSS (BRIEF.md exists) → RESEARCH → PLAN → outputs: ARCHITECTURE.md, ROADMAP.md
```

**Dispatch pattern:**

1. Read contracts in parallel (use parallel Read calls in single tool invocation):
   - plugins/[Name]/.planning/BRIEF.md
   - plugins/[Name]/.planning/parameter-spec.md OR parameter-spec-draft.md
   - plugins/[Name]/.planning/mockups/*.yaml (if exists)

2. Get research context for Stage 0:
   ```python
   research_context = run_bash(
       f"python3 .claude/scripts/inject-context.py --stage 0 --agent research-planning-agent --plugin {plugin_name}"
   )
   ```

3. Invoke subagent (include research_context in prompt):
   ```
   Task(subagent_type="research-planning-agent", description="[prompt with contracts and {research_context}]", model="sonnet")
   ```

   See [references/subagent-invocation.md](references/subagent-invocation.md) for detailed prompt construction.

4. After subagent returns, execute checkpoint protocol:
   - Read subagent's return message
   - Verify ARCHITECTURE.md and ROADMAP.md created
   - Present decision menu (use assets/decision-menu-stage-0.md template)
   - WAIT for user response

**Subagent executes:** Full Stage 0 protocol from research-planning-agent.md

**Outputs:**
- `plugins/[Name]/.planning/research/ARCHITECTURE.md` (see assets/architecture-template.md)
- `plugins/[Name]/.planning/ROADMAP.md` (see assets/plan-template.md)
- `plugins/[Name]/.planning/stages/0-ideation/CONTEXT.md` (discuss output)
- `plugins/[Name]/.planning/STATUS.md` (updated with stage progress)

**Decision menu (HANDOFF POINT):** Use assets/decision-menu-stage-0.md template

**CRITICAL: This is a handoff point. Present clean continuation format, do NOT auto-proceed.**

After presenting the handoff menu:
- Do NOT invoke any skills directly
- Present the menu with full `/implement [PluginName]` command
- Include `/clear` instruction
- STOP and wait for user to run the command in fresh context

See `.claude/references/handoff-protocol.md` for handoff protocol details.

**Note:** research-planning-agent runs in fresh context (5-35 min session doesn't pollute orchestrator)

**Validation before proceeding to Stage 1:**

1. Verify ARCHITECTURE.md exists with all required sections (Core Components, Processing Chain, Parameter Mapping)
2. Verify ROADMAP.md exists with complexity score and implementation strategy
3. If verification fails: Display error and return to Stage 0
4. When user chooses option 1: Delegate to plugin-workflow skill (plugin-workflow owns Stages 1-4)

---

## Reference Files

- [references/preconditions.md](references/preconditions.md) - Detailed precondition validation logic
- [references/subagent-invocation.md](references/subagent-invocation.md) - Subagent dispatch pattern and prompt construction
- [references/state-updates.md](references/state-updates.md) - State file update protocol
- [references/git-operations.md](references/git-operations.md) - Commit patterns

**Templates:**
- `assets/architecture-template.md` - DSP architecture contract template
- `assets/plan-template.md` - Implementation plan template
- `assets/decision-menu-stage-0.md` - Decision menu template
