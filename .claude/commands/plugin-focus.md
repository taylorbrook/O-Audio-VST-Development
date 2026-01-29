---
name: plugin:focus
description: Set active plugin context
skill: plugin-context
args: "[plugin_name]"
---

# /plugin:focus

Set or display the currently focused plugin. The focused plugin is used as the default for commands that accept an optional plugin name.

## Usage

```
/plugin:focus [plugin_name]    # Set focused plugin
/plugin:focus                  # Show current focus
```

## Arguments

- `plugin_name` - Plugin to focus on (optional)

## Examples

**Set focus:**
```
/plugin:focus O-IntonationPad

Focused on O-IntonationPad
Stage: 2-dsp, Phase: plan
```

**Show current focus:**
```
/plugin:focus

Currently focused: O-IntonationPad
Stage: 2-dsp, Phase: plan
```

## Behavior

Once focused, these commands will use the focused plugin by default:
- `/plugin:status` → `/plugin:status O-IntonationPad`
- `/plugin:discuss` → `/plugin:discuss O-IntonationPad`
- `/plugin:plan` → `/plugin:plan O-IntonationPad`
- `/continue` → `/continue O-IntonationPad`

## Related Commands

- `/plugin:list` - See all plugins
- `/plugin:status` - Detailed status
