---
name: plugin-phases
description: GSD-style phase commands for granular control over plugin development. Discuss, research, plan, execute, and verify phases for each stage.
allowed-tools:
  - Task
  - Read
  - Write
  - Edit
  - Bash
  - AskUserQuestion
commands:
  - name: plugin:discuss
    description: Interactive context gathering for a stage
    args: "[plugin_name?] [stage?] [--skip-*]"
  - name: plugin:research
    description: Investigate implementation approach
    args: "[plugin_name?] [stage?] [--skip-*]"
  - name: plugin:plan
    description: Create execution plan with task breakdown
    args: "[plugin_name?] [stage?] [--skip-*]"
  - name: plugin:execute
    description: Run stage-specific agent
    args: "[plugin_name?] [stage?] [--skip-*]"
  - name: plugin:verify
    description: Validate goal achievement
    args: "[plugin_name?] [stage?] [--skip-*]"
---

# plugin-phases Skill

**Purpose:** GSD-style granular phase control for plugin development. Each stage (ideation, foundation, dsp, gui, polish) runs through 5 phases: discuss → research → plan → execute → verify.

## Phase Overview

| Phase | Agent | Output | Skippable |
|-------|-------|--------|-----------|
| discuss | plugin-discuss-agent | CONTEXT.md | Yes |
| research | gsd-phase-researcher | RESEARCH.md | Yes |
| plan | gsd-planner | PLAN.md | No |
| execute | stage-specific | SUMMARY.md | No |
| verify | gsd-verifier | VERIFICATION.md | Yes |

**Stage-specific execute agents:**
- Stage 0 (ideation): plugin-ideation
- Stage 1 (foundation): foundation-shell-agent
- Stage 2 (dsp): dsp-agent
- Stage 3 (gui): gui-agent
- Stage 4 (polish): polish-agent

## Commands

### /plugin:discuss [plugin_name?] [stage?]

Interactive context gathering for a stage. Asks questions to build understanding before implementation.

**Implementation:**
1. Resolve plugin and stage (use focused plugin, current stage if not specified)
2. Validate phase progression (should be at discuss or earlier)
3. Load existing context (BRIEF.md, ARCHITECTURE.md, previous stage summaries)
4. Invoke plugin-discuss-agent via Task tool:
   ```
   Task(
     subagent_type="general-purpose",
     prompt="""
     You are a plugin-discuss-agent gathering context for stage [stage] of [plugin_name].

     Stage: [stage_name]
     Description: [stage_description]

     Existing context:
     - BRIEF.md: [summary]
     - Previous stage summary: [summary]

     Ask 3-5 clarifying questions to understand:
     - Specific requirements for this stage
     - User preferences and constraints
     - Potential edge cases or concerns

     After gathering answers, create CONTEXT.md with structured summary.
     """
   )
   ```
5. Agent creates `stages/[N]-[name]/CONTEXT.md`
6. Update STATUS.md phase progress
7. Update registry
8. Present completion menu:
   ```
   ✓ Discuss phase complete for Stage 2 (DSP)

   CONTEXT.md created with:
   - JI ratio calculation requirements
   - 5-limit vs 7-limit decision
   - Voice leading preferences

   Next:
   1. /plugin:research O-IntonationPad 2-dsp (recommended)
   2. /plugin:plan O-IntonationPad 2-dsp (skip research)
   3. /plugin:status O-IntonationPad
   ```

### /plugin:research [plugin_name?] [stage?]

Investigate implementation approach. Researches patterns, algorithms, and existing code.

**Implementation:**
1. Resolve plugin and stage
2. Validate phase progression
3. Load CONTEXT.md from discuss phase (if exists)
4. Invoke gsd-phase-researcher via Task tool:
   ```
   Task(
     subagent_type="gsd-phase-researcher",
     prompt="""
     Research implementation approach for [plugin_name] stage [stage].

     Context from discuss phase:
     [CONTEXT.md content]

     Research:
     1. Relevant JUCE patterns and APIs
     2. Algorithm approaches
     3. Existing module code that could be reused
     4. Potential pitfalls from troubleshooting docs

     Output: RESEARCH.md with findings and recommendations.
     """
   )
   ```
5. Agent creates `stages/[N]-[name]/RESEARCH.md`
6. Update STATUS.md and registry
7. Present completion menu

### /plugin:plan [plugin_name?] [stage?]

Create execution plan with detailed task breakdown.

**Implementation:**
1. Resolve plugin and stage
2. Validate phase progression
3. Load CONTEXT.md and RESEARCH.md (if exist)
4. Invoke gsd-planner via Task tool:
   ```
   Task(
     subagent_type="gsd-planner",
     prompt="""
     Create execution plan for [plugin_name] stage [stage].

     Context:
     [CONTEXT.md content]

     Research findings:
     [RESEARCH.md content]

     Create PLAN.md with:
     1. Goal statement
     2. Task breakdown (numbered steps)
     3. Files to create/modify
     4. Dependencies between tasks
     5. Success criteria
     """
   )
   ```
5. Agent creates `stages/[N]-[name]/PLAN.md`
6. Update STATUS.md and registry
7. Present completion menu

### /plugin:execute [plugin_name?] [stage?]

Run the stage-specific implementation agent.

**Implementation:**
1. Resolve plugin and stage
2. Validate phase progression (plan phase must be complete)
3. Load all phase artifacts (CONTEXT.md, RESEARCH.md, PLAN.md)
4. Determine agent based on stage:
   ```
   stage_agents = {
     "0-ideation": "plugin-ideation",
     "1-foundation": "foundation-shell-agent",
     "2-dsp": "dsp-agent",
     "3-gui": "gui-agent",
     "4-polish": "polish-agent"
   }
   ```
5. Invoke appropriate agent via Task tool with:
   - PLAN.md tasks
   - BRIEF.md parameters
   - ARCHITECTURE.md (stages 1-4)
   - Required Reading patterns
6. Agent implements and creates `stages/[N]-[name]/SUMMARY.md`
7. Update STATUS.md and registry
8. Present completion menu

### /plugin:verify [plugin_name?] [stage?]

Validate that stage goals were achieved.

**Implementation:**
1. Resolve plugin and stage
2. Validate phase progression (execute must be complete)
3. Load PLAN.md (goals) and SUMMARY.md (what was done)
4. Invoke gsd-verifier via Task tool:
   ```
   Task(
     subagent_type="gsd-verifier",
     prompt="""
     Verify stage [stage] achievement for [plugin_name].

     Goals from PLAN.md:
     [goal statement and success criteria]

     What was built (SUMMARY.md):
     [summary content]

     Verify:
     1. All success criteria met
     2. No regressions introduced
     3. Code compiles and runs
     4. Manual verification checklist

     Output: VERIFICATION.md with pass/fail and details.
     """
   )
   ```
5. Also invoke validation-agent for technical validation
6. Combine results into `stages/[N]-[name]/VERIFICATION.md`
7. Update STATUS.md and registry
8. If all phases complete, advance to next stage
9. Present completion menu

## Skip Flags

Phases can be skipped with flags:
- `--skip-discuss` - Skip discuss phase (use existing context)
- `--skip-research` - Skip research phase (jump to planning)
- `--skip-verify` - Skip verify phase (trust implementation)

**Cannot skip:** plan, execute

**Example:**
```
/plugin:execute O-IntonationPad 2-dsp --skip-discuss --skip-research
```

**Implementation:**
1. Parse skip flags from args
2. Update registry with skipped phases
3. Mark phases as "skipped" in STATUS.md (not "complete")
4. Proceed to next non-skipped phase

## Phase State Transitions

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

## Integration Points

**Invokes via Task tool:**
- plugin-discuss-agent (discuss phase)
- gsd-phase-researcher (research phase)
- gsd-planner (plan phase)
- Stage-specific agents (execute phase)
- gsd-verifier + validation-agent (verify phase)

**Reads:**
- `plugins/[name]/.planning/STATUS.md`
- `plugins/[name]/.planning/BRIEF.md`
- `plugins/[name]/.planning/research/ARCHITECTURE.md`
- `plugins/[name]/.planning/stages/[N]-[name]/*.md`
- `.claude/plugin-registry.json`

**Writes:**
- `plugins/[name]/.planning/stages/[N]-[name]/CONTEXT.md`
- `plugins/[name]/.planning/stages/[N]-[name]/RESEARCH.md`
- `plugins/[name]/.planning/stages/[N]-[name]/PLAN.md`
- `plugins/[name]/.planning/stages/[N]-[name]/SUMMARY.md`
- `plugins/[name]/.planning/stages/[N]-[name]/VERIFICATION.md`
- `plugins/[name]/.planning/STATUS.md`
- `.claude/plugin-registry.json`

## Error Handling

**Phase out of order:**
```
Error: Cannot run 'execute' phase - 'plan' phase not complete.

Current state: Stage 2-dsp, Phase: discuss

Run these phases first:
1. /plugin:discuss O-IntonationPad 2-dsp
2. /plugin:research O-IntonationPad 2-dsp (or --skip-research)
3. /plugin:plan O-IntonationPad 2-dsp
```

**Verification fails:**
```
⚠ Verification failed for Stage 2 (DSP)

Issues found:
- processBlock not generating audio output
- Parameter 'voiceCount' not connected to DSP

Options:
1. View VERIFICATION.md details
2. Re-run execute phase
3. Investigate with /research
4. Manual fix (pause workflow)
```

## Express Mode Integration

When express mode is enabled, all 5 phases run automatically:
```
/implement O-IntonationPad --express

Stage 2: DSP
  [auto] discuss → Using CONTEXT.md from previous work
  [auto] research → DSP patterns loaded
  [auto] plan → PLAN.md generated (8 tasks)
  [auto] execute → dsp-agent running...
         ✓ JI ratio calculator implemented
         ✓ Chord generator implemented
         ✓ Voice management implemented
  [auto] verify → All checks passed
  → Advancing to Stage 3
```

**On any error:** Drops to manual mode, presents error menu.
