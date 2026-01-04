# Invocation Protocol

How this skill is invoked and how to determine the correct mode.

## Mode Detection

When invoked, determine the mode by examining the trigger:

### Mode 1: Installation
**Triggers:**
- Command: `/install-plugin [PluginName]`
- From plugin-workflow: After Stage 3 validation completes
- From plugin-improve: User accepts reinstallation offer
- Natural language: "Install [PluginName]", "Deploy [PluginName]"

**Action:** Extract plugin name, execute installation workflow

### Mode 2: Uninstallation
**Triggers:**
- Command: `/uninstall [PluginName]`
- Natural language: "Uninstall [PluginName]", "Remove [PluginName] from system"

**Action:** Extract plugin name, execute uninstallation workflow

### Mode 3: Reset to Ideation
**Triggers:**
- Command: `/reset-to-ideation [PluginName]`
- Natural language: "Reset [PluginName] to ideation", "Start over with [PluginName]"

**Action:** Extract plugin name, execute reset workflow

### Mode 4: Destroy
**Triggers:**
- Command: `/destroy [PluginName]`
- Natural language: "Destroy [PluginName]", "Delete everything for [PluginName]"

**Action:** Extract plugin name, execute destroy workflow (with confirmation gate)

### Mode: Menu
**Triggers:**
- Command: `/clean [PluginName]`
- Natural language: "Clean up [PluginName]", "Manage [PluginName] lifecycle"

**Action:** Extract plugin name, present interactive menu, wait for selection, route to chosen mode

## Invocation Context Examples

### Example 1: Direct Command
```
User: /install-plugin TapeAge
```
- Trigger: Direct command
- Mode: 1 (Installation)
- Plugin name: "TapeAge"
- Action: Execute installation workflow immediately

### Example 2: From plugin-workflow
```
plugin-workflow completes Stage 3 → invokes plugin-lifecycle with:
- Context: "Stage 3 complete for [PluginName]"
- Mode: 1 (Installation)
- Plugin name: From context
- Action: Execute installation workflow automatically
```

### Example 3: From plugin-improve
```
plugin-improve completes changes → offers:
"Reinstall [PluginName] to system folders? (y/n)"
- IF yes → Mode: 1 (Installation)
- IF no → Return control to user
```

### Example 4: Natural Language
```
User: "Install TapeAge to my DAW"
```
- Trigger: Natural language
- Mode: 1 (Installation)
- Plugin name: Parsed from utterance ("TapeAge")
- Action: Execute installation workflow

### Example 5: Interactive Menu
```
User: /clean DrumRoulette
```
- Trigger: /clean command
- Mode: Menu
- Plugin name: "DrumRoulette"
- Action: Present menu, wait for selection, route to chosen mode (1/2/3/4)

## Return Protocol

This is a **terminal skill** - it does not invoke other skills after completion.

After completing any operation (install/uninstall/reset/destroy):

1. Present decision menu (see [decision-menu-protocol.md](decision-menu-protocol.md))
2. Wait for user response (do not auto-proceed)
3. Handle user selection:
   - If user selects "Create another plugin" → Route to plugin-ideation
   - If user selects "Test in DAW" → Provide testing instructions, end workflow
   - If user selects "Other" → Ask open-ended question, end workflow
4. Return control to user

**Never:**
- Auto-proceed to next action
- Invoke another skill without user selection
- Skip the decision menu

## Integration Points

**This skill is invoked by:**
- `/install-plugin` command (Mode 1)
- `/uninstall` command (Mode 2)
- `/reset-to-ideation` command (Mode 3)
- `/destroy` command (Mode 4)
- `/clean` command (Mode: Menu)
- `plugin-workflow` skill (Mode 1 after Stage 3)
- `plugin-improve` skill (Mode 1 after changes, if user confirms)
- Natural language requests

**This skill invokes:**
- None directly (terminal skill)
- Routes to other skills via user selection in decision menu:
  - `plugin-ideation` (if user selects "Create another plugin")
