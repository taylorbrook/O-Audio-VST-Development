---
name: continue
description: Resume plugin development from checkpoint
---

# /continue

When user runs `/continue [PluginName?]`, YOU MUST invoke the context-resume skill using the Skill tool.

**IMPORTANT: Use this exact invocation:**

```
Skill({ skill: "context-resume" })
```

DO NOT manually read handoff files or present summaries. The context-resume skill handles all of this.

## Behavior

**Command-line flags:**
- `--express`: Force express mode for resumed workflow
- `--manual`: Force manual mode for resumed workflow

**Flag parsing:**
```bash
# Parse plugin name and flags from arguments
# Input examples:
#   "/continue PluginName"
#   "/continue PluginName --express"
#   "/continue PluginName --manual"

PLUGIN_NAME=""
FLAG_MODE=""

for arg in "$@"; do
  case "$arg" in
    --express)
      FLAG_MODE="express"
      ;;
    --manual)
      FLAG_MODE="manual"
      ;;
    /continue)
      # Skip command itself
      ;;
    *)
      PLUGIN_NAME="$arg"
      ;;
  esac
done
```

**Mode determination on resume:**
1. Check for flag override (--express or --manual)
2. If flag present: Use flag mode (override saved mode)
3. If no flag: Read workflow_mode from .continue-here.md
4. If field missing: Default to "manual"

**Pass mode to continuation skill:**
```bash
# Set environment variables before invoking context-resume
if [ -n "$FLAG_MODE" ]; then
  export WORKFLOW_MODE="$FLAG_MODE"
  echo "Workflow mode: $FLAG_MODE (from flag, overriding saved mode)"
else
  # Read mode from .continue-here.md (handled by context-resume skill)
  echo "Workflow mode: (resuming with saved mode)"
fi
```

<preconditions enforcement="blocking">
  <check target="handoff_files" condition="at_least_one_exists">
    Search for `.continue-here.md` files in priority order:

    **Without plugin name:**
    1. `plugins/[Name]/.continue-here.md` (implementation/planning/ideation)
    2. `plugins/[Name]/.ideas/mockups/.continue-here.md` (mockup iteration)

    Present interactive menu if multiple found:
    ```
    Which plugin would you like to resume?

    1. [PluginName1]
       Stage [N] ([StageName]) • Active development • [time] ago

    2. [PluginName2]
       Mockup v[N] ready • Ready to build • [time] ago

    3. [PluginName3]
       Creative brief complete • Not started • [time] ago
    ```

    **With plugin name:**
    Load directly: `plugins/[PluginName]/.continue-here.md`

    IF none found: See Error Handling section below.
  </check>
</preconditions>

## State Contract

<state_contract>
  <reads>
    - `.continue-here.md` (one of 3 locations by priority)
    - PLUGINS.md (status verification)
    - Recent git commits (for plugin only)
    - Contract files (if workflow stage 0-1)
    - Source files (if mentioned in handoff)
    - Research notes (if Stage 0-1)
    - UI mockups (if applicable)
  </reads>

  <writes>
    NONE - This command is READ-ONLY for state files.
    Continuation skills handle all state updates.
  </writes>

  <loads_before_routing>
    Load contracts and context files BEFORE invoking continuation skill.
    This provides full context for skill execution.
  </loads_before_routing>
</state_contract>

## After Loading

Present summary:
```
Resuming [PluginName] at Stage [N]...

Summary of completed work:
- [Stage 1] Foundation set up
- [Stage 2] Plugin loads in DAW
- [Stage 3.1] Core DSP implemented

Current status:
Working on Stage [N] ([Description]).
[Current state description]

Next steps:
1. [Primary next action]
2. [Alternative action]

Ready to continue?
```

Wait for confirmation, then resume workflow at exact continuation point.

## Error Handling

**No handoff files found:**

IF no handoff exists for any plugin:
- Display: "No resumable work found"
- Explain: Handoffs created after ideation, mockup, during implementation, or after improvements
- Suggest: `/dream` (explore ideas) or `/implement` (build new plugin)

IF specific [PluginName] doesn't have handoff:
- Display: "[PluginName] doesn't have a handoff file"
- Explain: Plugin may be complete (Stage 3), not started, or handoff removed
- Suggest: Check `PLUGINS.md`, `/improve [PluginName]`, or `/implement [PluginName]`

See `context-resume` skill's `error-recovery.md` for additional scenarios.

## Routes To

**Skill:** context-resume

The skill handles:
- Reading `.continue-here.md` files
- Parsing current stage and status
- Summarizing completed work
- Loading relevant context
- Proposing next steps
- Continuing workflow from checkpoint
