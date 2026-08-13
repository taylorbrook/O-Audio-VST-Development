---
name: continue
description: Resume work from last session checkpoint with validation
argument-hint: "[PluginName?] [--express] [--skip-discuss] [--skip-research] [--skip-verify]"
skill: context-resume
---

# /continue

Resume the Plugin Freedom System session from where you left off. Restores context from checkpoint and validates state before proceeding.

## Usage

```
/continue [plugin_name] [flags]    # Resume specific plugin
/continue [flags]                  # Resume focused plugin
/continue                          # Resume focused plugin, default options
```

## Arguments

- `plugin_name` - Plugin to resume (optional, defaults to focused)

## Flags

| Flag | Description |
|------|-------------|
| `--express` | Force express mode (override saved mode) |
| `--manual` | Force manual mode (override saved mode) |
| `--skip-discuss` | Skip discuss phases on resume |
| `--skip-research` | Skip research phases on resume |
| `--skip-verify` | Skip verify phases on resume |

## Process

1. **Load checkpoint skill**
   @.claude/skills/session-checkpoint/SKILL.md

2. **Load active plugin state**
   Read `PLUGINS.md` for the plugin roster and status column, then read
   `plugins/{plugin}/.planning/STATUS.md` for per-plugin state. STATUS.md is
   authoritative. Identify the focused plugin via `focused: true` in STATUS.md
   frontmatter (or use the specified plugin name).

3. **Run state validation**
   @.claude/skills/state-validation/SKILL.md
   If issues found: present recovery options before proceeding
   If healthy: continue to restoration

4. **Restore from checkpoint**
   Load `checkpoints/{plugin}/latest.json`
   Parse checkpoint state
   Load referenced context files

5. **Display resume summary**
   Show: plugin, stage, phase, last task, next task
   Show: files modified in last session
   Show: next command to run

6. **Ready prompt**
   End with clear next action

## Validation on Resume

Validation runs automatically. If inconsistencies detected:
- Show STATE VALIDATION REPORT
- Load recovery skill: @.claude/skills/state-recovery/SKILL.md
- Present options and wait for user choice
- Re-validate after recovery
- Then proceed with resume

## Context Loading (Capped)

Only load essential context to preserve context budget:
- STATUS.md frontmatter
- Current PLAN.md (if in execute phase)
- Current CONTEXT.md (if exists)
- Last checkpoint state

Do NOT load:
- All prior SUMMARYs
- Other plugins' state
- Historical checkpoints

## Behavior

1. **Resolve plugin name:**
   - If provided: Use specified plugin
   - If not provided: Use focused plugin from registry
   - If no focused plugin: Show menu of resumable plugins

2. **Load state from checkpoint:**
   - Read `plugins/[Name]/.planning/STATUS.md`
   - Read `.planning/workflow/checkpoints/{plugin}/latest.json`
   - Parse current stage, phase, and task progress from checkpoint
   - Read handoff context

3. **Determine workflow mode:**
   - Flag override (`--express` or `--manual`) takes priority
   - Otherwise use `express_mode` from STATUS.md
   - Default to "manual" if not set

4. **Present context summary and next command**

## Output Format

**Successful resume with checkpoint:**
```
SESSION RESUMED
===============
Plugin: O-IntonationPad
Stage: 2-dsp
Phase: execute

Last completed: Task 2: Create oscillator base class
Next task: Task 3: Implement wavetable loading

Files from last session:
- Source/DSP/Oscillator.h
- Source/DSP/Oscillator.cpp

Handoff context: Oscillator base class with virtual processBlock()

State: healthy (validated)

Ready to continue. Next: complete Task 3 or /status for context.
```

**Resume without checkpoint (first session):**
```
SESSION RESUMED
===============
Plugin: O-IntonationPad
Stage: 2-dsp
Phase: execute

No checkpoint found - loaded state from STATUS.md

Current state:
- Stage: 2-dsp
- Phase: execute

State: healthy (validated)

Next: Run /plugin:execute to begin execution
```

**Resume after context clear:**
```
RESUMING SESSION
================
Last session ended at: 2026-01-30T14:30:00Z
Reason: Context clear for fresh execution

Plugin: O-IntonationPad
Stage: 2-dsp
Phase: execute
Plan: 02-01-PLAN.md

Next step: Run /plugin:execute to continue plan execution

Copy command: /plugin:execute
```

**Resume with validation issues:**
```
SESSION RESUME - VALIDATION REQUIRED
====================================

STATE VALIDATION REPORT
=======================
Status: inconsistent
Timestamp: 2026-01-31T10:00:00Z

Checks Run: 12
Passed: 11
Failed: 1

ISSUES FOUND:
-------------
1. [ERROR] stage_consistency: Registry and STATUS.md disagree
   Registry value: 1-foundation
   STATUS.md value: 2-dsp
   Recoverable: YES

RECOVERY OPTIONS:
1. Sync registry from STATUS.md (recommended)
2. Sync STATUS.md from registry
3. Manual repair

Select option [1-3] to proceed with resume:
```

## Examples

```bash
# Resume focused plugin
/continue

# Resume specific plugin with express mode
/continue O-IntonationPad --express

# Resume and skip optional phases
/continue O-IntonationPad --express --skip-discuss --skip-research

# Force manual mode (overrides saved express mode)
/continue O-IntonationPad --manual
```

## No Resumable Work

**If no focused plugin and no plugin specified:**
```
No plugin currently focused.

Resumable plugins:
1. O-IntonationPad (Stage 2-dsp, Phase plan)
2. O-NewPlugin (Stage 1-foundation, Phase discuss)

Choose a plugin or use: /plugin-focus [name]
```

**If specified plugin has no handoff:**
```
O-SomePlugin doesn't have resumable state.

Status: Working (complete)

Options:
- /improve O-SomePlugin (add features/fix bugs)
- /plugin-status O-SomePlugin (view details)
```

## Phase-Aware Resume

The continue command resumes at the exact phase within a stage:

```
# If paused at Stage 2, Phase research:
/continue O-IntonationPad

Resuming at Stage 2 (DSP), Phase research
- Previous: discuss completed
- Current: research (in progress)
- Remaining: plan, execute, verify
```

## State Integration

On resume, write to `plugins/{plugin}/.planning/STATUS.md` frontmatter:
1. Set `focused: true` (and clear `focused: true` from the previously focused plugin's STATUS.md)
2. Update `lastActivity` timestamp
3. Set `status: active` (if it was `paused`)
4. Reflect the status change in that plugin's `PLUGINS.md` row if it changed

## Related Commands

- `/reconcile` - Manual validation trigger
- `/focus {plugin}` - Switch to different plugin
- `/status` - Full context dump
- `/plugin-resume` - Alternative resume command (same behavior)
- `/plugin-status` - View status without resuming
- `/plugin-pause` - Pause current work
- `/implement` - Start or restart implementation
