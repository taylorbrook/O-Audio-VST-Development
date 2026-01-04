---
name: implement
description: Build plugin through implementation stages 1-4
argument-hint: [PluginName?]
allowed-tools: Bash(test:*)
---

# /implement

When user runs `/implement [PluginName?]`, invoke the plugin-workflow skill to build the plugin (stages 1-3).

<prerequisite>
  Planning (Stage 0) must be completed first via `/plan` command.
</prerequisite>

## Preconditions

<preconditions enforcement="blocking">
  <decision_gate type="status_verification" source="PLUGINS.md">
    <allowed_state status="🚧 Stage 0" description="Planning complete">
      Start implementation at Stage 1
    </allowed_state>

    <allowed_state status="🚧 Stage 1-3" description="In progress">
      Resume implementation at current stage
    </allowed_state>

    <blocked_state status="💡 Ideated" action="redirect">
      <error_message>
        [PluginName] planning is not complete.

        Run /plan [PluginName] first to complete Stage 0 (Research & Planning):
        - Creates architecture.md (DSP specification)
        - Creates plan.md (implementation strategy)

        Then run /implement to build (stages 1-3).
      </error_message>
    </blocked_state>

    <blocked_state status="✅ Working" action="redirect">
      <error_message>
        [PluginName] is already implemented and working.

        Use /improve [PluginName] to make changes or add features.
      </error_message>
    </blocked_state>
  </decision_gate>

  <decision_gate type="contract_verification" blocking="true">
    <required_contracts>
      <contract path="plugins/${PLUGIN_NAME}/.ideas/architecture.md" created_by="Stage 0"/>
      <contract path="plugins/${PLUGIN_NAME}/.ideas/plan.md" created_by="Stage 0"/>
      <contract path="plugins/${PLUGIN_NAME}/.ideas/parameter-spec.md" created_by="ideation"/>
    </required_contracts>

    <validation_command>
      # See .claude/skills/plugin-workflow/references/precondition-checks.sh for reusable check functions
      test -f "plugins/${PLUGIN_NAME}/.ideas/architecture.md" &amp;&amp; \
      test -f "plugins/${PLUGIN_NAME}/.ideas/plan.md" &amp;&amp; \
      test -f "plugins/${PLUGIN_NAME}/.ideas/parameter-spec.md"
    </validation_command>

    <on_failure action="BLOCK">
      <error_message>
        ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        ✗ BLOCKED: Missing planning artifacts
        ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

        Implementation requires complete planning contracts:

        Required contracts:
        [✓/✗] architecture.md - [exists/MISSING]
        [✓/✗] plan.md - [exists/MISSING]
        [✓/✗] parameter-spec.md - [exists/MISSING]

        HOW TO UNBLOCK:
        1. Run: /plan [PluginName]
           - Completes Stage 0 (Research) → architecture.md
           - Completes Stage 0 (Planning) → plan.md

        2. If parameter-spec.md missing:
           - Run: /dream [PluginName]
           - Create and finalize UI mockup
           - Finalization generates parameter-spec.md

        Once all contracts exist, /implement will proceed.
      </error_message>
    </on_failure>
  </decision_gate>
</preconditions>

## Behavior

**If no plugin name provided:**
1. Read PLUGINS.md and filter for plugins with status 🚧 Stage 0 or 🚧 Stage 1-4
2. Present numbered menu of eligible plugins with current stage
3. Wait for user selection

**If plugin name provided:**
1. Parse plugin name and flags from arguments
2. Read preferences.json (if exists) and determine workflow mode
3. Apply flag precedence (flag > preferences > default)
4. Verify preconditions (status check, contract verification)
5. If preconditions pass: Invoke plugin-workflow skill via Skill tool with mode context
6. If preconditions fail: Display blocking error and stop

## Workflow Mode & Preferences

**Command-line flags:**
- `--express`: Force express mode (auto-progress through stages)
- `--manual`: Force manual mode (show decision menus)

**Precedence order:**
1. Command-line flag (--express or --manual)
2. `.claude/preferences.json` (workflow.mode)
3. Default ("manual")

**Flag parsing:**
```bash
# Parse plugin name and flags from arguments
# Input examples:
#   "/implement PluginName"
#   "/implement PluginName --express"
#   "/implement PluginName --manual"

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
    /implement)
      # Skip command itself
      ;;
    *)
      PLUGIN_NAME="$arg"
      ;;
  esac
done
```

**Preferences reading:**
```bash
# Read preferences.json (if exists)
PREFS_MODE="manual"  # default
AUTO_TEST="false"
AUTO_INSTALL="false"
AUTO_PACKAGE="false"

if [ -f ".claude/preferences.json" ]; then
  # Validate JSON before parsing
  if jq empty .claude/preferences.json 2>/dev/null; then
    PREFS_MODE=$(jq -r '.workflow.mode // "manual"' .claude/preferences.json)
    AUTO_TEST=$(jq -r '.workflow.auto_test // false' .claude/preferences.json)
    AUTO_INSTALL=$(jq -r '.workflow.auto_install // false' .claude/preferences.json)
    AUTO_PACKAGE=$(jq -r '.workflow.auto_package // false' .claude/preferences.json)

    # Validate mode value
    if [[ "$PREFS_MODE" != "express" && "$PREFS_MODE" != "manual" ]]; then
      echo "Warning: workflow.mode must be 'express' or 'manual', using manual mode"
      PREFS_MODE="manual"
    fi
  else
    echo "Warning: preferences.json is invalid JSON, using manual mode"
    PREFS_MODE="manual"
  fi
fi

# Determine effective mode (flag > preferences > default)
EFFECTIVE_MODE="${FLAG_MODE:-$PREFS_MODE}"

# Display mode selection
if [ -n "$FLAG_MODE" ]; then
  echo "Workflow mode: $EFFECTIVE_MODE (from flag)"
else
  echo "Workflow mode: $EFFECTIVE_MODE (from preferences)"
fi
```

**Pass mode to plugin-workflow:**

Before invoking the plugin-workflow skill, set environment variables for mode propagation:

```bash
export WORKFLOW_MODE="$EFFECTIVE_MODE"
export AUTO_TEST="$AUTO_TEST"
export AUTO_INSTALL="$AUTO_INSTALL"
export AUTO_PACKAGE="$AUTO_PACKAGE"
```

The plugin-workflow skill reads these variables to control checkpoint behavior:
- Express mode: Auto-progress through stages (no intermediate menus)
- Manual mode: Present decision menus at each checkpoint (current behavior)

See `.claude/skills/plugin-workflow/SKILL.md` for checkpoint protocol details.

**Status parsing (ROBUST implementation):**
```
CRITICAL: Always parse status from FULL ENTRY section (canonical source), not registry table.

Implementation:
1. Find full entry: grep -A 10 "^### ${PLUGIN_NAME}$" PLUGINS.md
2. Extract status line: grep "^\*\*Status:\*\*"
3. Strip Markdown: sed 's/\*\*//g; s/__//g; s/_//g; s/Status://g'
4. Parse stage: Extract "Stage N" or "Stage N.M" pattern

Example:
  Input: **Status:** 🚧 **Stage 3.1**
  After strip: 🚧 Stage 3.1
  Parsed stage: 4.1

Why this approach:
- Full entry is canonical source (always most accurate)
- Registry table is derived view (can drift out of sync)
- Handles all Markdown formatting variations (**bold**, _italic_, etc.)
- Prevents false-negative blocks from registry drift
```

**Registry consistency:**
If implementing status checks in bash/scripts, use the `getPluginStatus()` and `validateRegistryConsistency()` functions from `.claude/skills/plugin-workflow/references/state-management.md` to ensure accurate parsing and detect drift.

**Skill invocation:**
Invoke the plugin-workflow skill with the plugin name and starting stage. The skill handles stages 1-4 implementation using the subagent dispatcher pattern.

## The Implementation Stages

<skill_delegation>
  <command_responsibility>
    This command is a ROUTING LAYER:
    1. Verify preconditions (status check, contract verification)
    2. Invoke plugin-workflow skill via Skill tool
    3. Return control to user
  </command_responsibility>

  <skill_responsibility ref="plugin-workflow">
    The plugin-workflow skill orchestrates stages 1-3:

    Stage 1 (Foundation) → Stage 2 (DSP) → Stage 3 (GUI with automatic validation)

    Each stage uses specialized subagent, follows checkpoint protocol (commit, state update, decision menu).

    For stage details, see .claude/skills/plugin-workflow/SKILL.md
  </skill_responsibility>

  <handoff_point>
    Command completes immediately after invoking plugin-workflow skill.
    All subsequent interaction (stage progression, decision menus) happens within skill context.
  </handoff_point>
</skill_delegation>

## Decision Menus

<decision_menus>
  Plugin-workflow skill presents decision menus at each stage completion.
  Decision menu format documented in plugin-workflow skill references.
  Command does NOT present menus - this is skill's responsibility after delegation.
</decision_menus>

## Pause & Resume

If user pauses:
- .continue-here.md updated with current stage
- PLUGINS.md status updated
- Changes committed

Resume with `/continue [PluginName]`

## Output

<expected_output>
  By completion, plugin has:
  - Compiling VST3 + AU plugins
  - Working audio processing + functional WebView UI
  - Pluginval compliant with factory presets
  - Complete git history for all stages
</expected_output>

## Workflow Integration

Complete plugin development flow:
1. `/dream [PluginName]` - Creative brief + UI mockup
2. `/plan [PluginName]` - Research and planning (Stage 0)
3. `/implement [PluginName]` - Build plugin (Stages 1-3)
4. `/install-plugin [PluginName]` - Deploy to system folders
