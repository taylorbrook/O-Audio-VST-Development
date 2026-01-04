# Decision Menu Protocol

After successful lifecycle operations (install, uninstall, reset, destroy), check workflow mode first, then optionally present a numbered decision menu.

## When to Check Workflow Mode

Check workflow mode before presenting menu:
- Mode 1 (Installation) completes successfully
- Mode 2 (Uninstallation) completes successfully
- Mode 3 (Reset to Ideation) completes successfully
- Mode 4 (Destroy) completes with backup created

**Workflow Mode Detection:**
1. Read `.claude/preferences.json` for `workflow.mode` value
2. If "express": Skip menu, return control immediately
3. If "manual" (or missing): Present decision menu below

## Protocol Steps (Manual Mode Only)

1. Display completion statement with checkmark
2. Present numbered menu using inline numbered list format (see below)
3. STOP execution and wait for user choice
4. Do NOT auto-proceed or make assumptions
5. Only continue after receiving user selection (1-5)

**IMPORTANT:** Present this menu using inline numbered list format. DO NOT use AskUserQuestion tool. Wait for user to type their choice in the conversation.

## Menu Format

```
✓ [PluginName] [operation] completed successfully

What's next?
1. [Primary action] (recommended) → [Brief description]
2. [Secondary action] → [Brief description]
3. [Discovery option] → [Brief description]
4. [Alternative path] → [Brief description]
5. Other

Choose (1-5): _
```

## Mode-Specific Menus

### After Installation (Mode 1)

```
✓ [PluginName] installed successfully

What's next?
1. Test in DAW (recommended) → Load in Logic/Ableton/Reaper
2. Create another plugin → /dream
3. Document this plugin → Create user guide/manual
4. Share plugin (export build) → Future: Package for distribution
5. Other

Choose (1-5): _
```

**Response handlers**:
- **Option 1:** Provide DAW testing guidance (terminal - ends workflow)
- **Option 2:** Route to plugin-ideation skill
- **Option 3:** Suggest creating user manual in `.ideas/` directory
- **Option 4:** Provide build export instructions (future feature note)
- **Option 5:** Ask open-ended "What would you like to do?"

### After Uninstallation (Mode 2)

```
✓ [PluginName] uninstalled successfully (source code preserved)

What's next?
1. Rebuild plugin → /continue [PluginName]
2. Create new plugin → /dream
3. View uninstallation summary → Show paths removed
4. Other

Choose (1-4): _
```

### After Reset to Ideation (Mode 3)

```
✓ [PluginName] reset to ideation stage (idea/mockups preserved)

What's next?
1. Re-implement from Stage 0 → /plan [PluginName]
2. Modify creative brief → Edit .ideas/creative-brief.md
3. Create new plugin instead → /dream
4. Other

Choose (1-4): _
```

### After Destroy (Mode 4)

```
✓ [PluginName] destroyed completely (backup created)

What's next?
1. Verify backup → Show backup location and contents
2. Create new plugin → /dream
3. Restore from backup → Instructions for restore
4. Other

Choose (1-4): _
```

## Important Notes

- This menu is MANDATORY after successful operations
- Never skip or auto-select an option
- Wait for explicit user selection before continuing
- This is a system-wide checkpoint protocol, not lifecycle-specific
