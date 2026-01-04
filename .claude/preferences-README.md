# Workflow Preferences

This file controls how the Plugin Freedom System handles checkpoint decision gates during plugin implementation.

## Configuration: `.claude/preferences.json`

### Settings

#### `workflow.mode`
**Type:** String ("express" | "manual")
**Default:** "manual"

Controls workflow automation level:

- **"manual"** (default): Present decision menu after each stage (0, 1, 2, 3). You must manually select "Continue to next stage" at each checkpoint.
  - Time cost: ~2-3 minutes per plugin (4 decision points)
  - Use when: Learning the system, troubleshooting, or want control over each stage

- **"express"**: Automatically progress through all stages without decision menus. Final menu still appears after Stage 3 validation completes.
  - Time savings: 2-3 minutes per plugin (0 intermediate decisions)
  - Use when: Building straightforward plugins, comfortable with the workflow, want speed

**Example:**
```json
{
  "workflow": {
    "mode": "express"
  }
}
```

#### `workflow.auto_test`
**Type:** Boolean
**Default:** false
**Status:** DEPRECATED (validation now automatic during Stage 3)

This setting is preserved for backward compatibility but no longer affects workflow behavior. Validation now runs automatically at the end of Stage 3 as part of unified validation.

**Previous behavior (old 5-stage system):**
- Controlled whether pluginval ran automatically after separate validation stage
- **New behavior:** Validation is automatic and integrated into Stage 3 completion

**Example:**
```json
{
  "workflow": {
    "mode": "express",
    "auto_test": true
  }
}
```

#### `workflow.auto_install`
**Type:** Boolean
**Default:** false

Install plugin to system folders automatically after Stage 3 completes.

- **true**: After Stage 3 validation passes, automatically invoke plugin-lifecycle skill (Mode 1: Installation)
- **false**: Present "Install to system folders" option in final menu (manual decision)

**Prerequisite:** Stage 3 validation must pass first

**Installation targets:**
- `~/Library/Audio/Plug-Ins/VST3/[PluginName].vst3`
- `~/Library/Audio/Plug-Ins/Components/[PluginName].component`

**Example:**
```json
{
  "workflow": {
    "mode": "express",
    "auto_test": true,
    "auto_install": true
  }
}
```

#### `workflow.auto_package`
**Type:** Boolean
**Default:** false

Create PKG installer automatically after installation.

- **true**: After installation succeeds, automatically invoke plugin-packaging skill
- **false**: Present "Create installer package" option in final menu (manual decision)

**Prerequisite:** Plugin must be installed first (auto_install=true or manual install)

**Output:** `plugins/[PluginName]/dist/[PluginName]-by-TACHES.pkg`

**Example:**
```json
{
  "workflow": {
    "mode": "express",
    "auto_test": true,
    "auto_install": true,
    "auto_package": true
  }
}
```

## Command-Line Flags

You can override preferences.json for a single run using flags:

### `/implement [PluginName] --express`
Force express mode (auto-progress) for this workflow only.

**Example:**
```
/implement GainPlugin --express
```

**Effect:** Ignores preferences.json, uses express mode for this run.

### `/implement [PluginName] --manual`
Force manual mode (show menus) for this workflow only.

**Example:**
```
/implement GainPlugin --manual
```

**Effect:** Ignores preferences.json, uses manual mode for this run.

### Precedence

**Order of precedence (highest to lowest):**
1. Command-line flag (`--express` or `--manual`)
2. preferences.json (`workflow.mode`)
3. Default (`"manual"`)

**Examples:**

| Flag | preferences.json | Effective Mode | Source |
|------|-----------------|----------------|--------|
| `--express` | `"manual"` | `"express"` | Flag wins |
| `--manual` | `"express"` | `"manual"` | Flag wins |
| (none) | `"express"` | `"express"` | Preferences |
| (none) | (missing file) | `"manual"` | Default |

## Use Cases

### Scenario 1: Learning the System
**Goal:** Understand what happens at each stage

**Configuration:**
```json
{
  "workflow": {
    "mode": "manual"
  }
}
```

**Workflow:**
- Decision menu after each stage
- Review code, test builds, inspect changes
- Full control over progression

---

### Scenario 2: Building Simple Plugins
**Goal:** Quickly create gain plugins, filters, basic effects

**Configuration:**
```json
{
  "workflow": {
    "mode": "express"
  }
}
```

**Workflow:**
- Stages auto-flow: 0 → 1 → 2 → 3
- Final menu appears (install? package? done?)
- Time savings: 2-3 minutes

---

### Scenario 3: Full Automation
**Goal:** Build, test, install, and package without manual steps

**Configuration:**
```json
{
  "workflow": {
    "mode": "express",
    "auto_test": true,
    "auto_install": true,
    "auto_package": true
  }
}
```

**Workflow:**
1. Stages auto-flow (0 → 1 → 2 → 3)
2. Final menu appears
3. After menu selection, installation happens automatically
4. After install, PKG created automatically
5. Complete workflow with minimal decisions

**When to use:** Building multiple similar plugins, production workflows, CI/CD scenarios

---

### Scenario 4: Testing a Specific Plugin
**Goal:** Override express mode for one plugin

**Command:**
```
/implement ComplexPlugin --manual
```

**Effect:**
- Uses manual mode despite preferences.json having `mode: "express"`
- Allows careful inspection of complex DSP or GUI code
- Preferences file unchanged (one-off override)

---

## Safety & Error Handling

### Express Mode Interruption

If any error occurs during express mode, the workflow automatically drops to manual mode:

**Errors that interrupt:**
- Build failures (CMake, compilation, linking)
- Test failures (pluginval errors)
- Installation failures (permissions, missing files)
- Packaging failures (prerequisites not met)

**After interruption:**
- Error menu presented
- You investigate/fix the issue
- Resume with `/continue [PluginName]`
- Continues in manual mode (safety first)

**Example:**
```
✗ Build failed at Stage 2

Errors:
- PluginProcessor.cpp:124: 'processBlock' is not a member of 'juce::AudioProcessor'

What should I do?
1. Investigate error (deep-research)
2. Show full build log
3. Review code
4. Manual fix (pause workflow)
5. Other

Choose (1-5): _
```

### Invalid Configuration

If preferences.json contains errors:

**Malformed JSON:**
```
Warning: preferences.json is invalid JSON, using manual mode
```
Workflow continues in manual mode.

**Invalid mode value:**
```json
{
  "workflow": {
    "mode": "turbo"  // Invalid
  }
}
```
```
Warning: workflow.mode must be 'express' or 'manual', using manual mode
```
Workflow continues in manual mode.

**Missing file:**
No warning (silent default to manual mode).

---

## Resume Behavior

When you pause and resume a workflow:

### Mode Preservation

**Scenario:**
1. Start: `/implement PluginName` (mode: express)
2. Pause at Stage 2 (Ctrl+C)
3. Resume: `/continue PluginName`

**Result:**
- Resume in express mode (mode preserved in `.continue-here.md`)
- Stage 2 continues → auto-progress to Stage 3

### Mode Override on Resume

**Scenario:**
1. Start: `/implement PluginName` (mode: manual)
2. Pause at Stage 2
3. Resume: `/continue PluginName --express`

**Result:**
- Resume in express mode (flag overrides saved mode)
- Stage 2 continues → auto-progress to Stage 3
- `.continue-here.md` updated to `workflow_mode: express`

---

## Backward Compatibility

**No preferences.json file:**
- System defaults to `mode: "manual"`
- All checkpoint menus appear (current behavior)
- 100% identical to existing workflow

**No breaking changes:**
- Existing commands work unchanged
- Old `.continue-here.md` files work (default to manual if mode missing)
- No migration required

---

## Quick Start

### Enable Express Mode

1. **Edit `.claude/preferences.json`:**
   ```json
   {
     "workflow": {
       "mode": "express"
     }
   }
   ```

2. **Run workflow:**
   ```
   /implement MyPlugin
   ```

3. **Observe:**
   ```
   ✓ Build System Ready → Implementing Audio Engine...
   ✓ Audio Engine Working → Adding GUI...
   ✓ UI Integrated → Running Validation...
   ✓ Plugin Complete

   What's next?
   1. Install to system folders
   2. Run validation tests
   3. Create installer package
   4. Pause here
   5. Other
   ```

### Disable Express Mode

1. **Edit `.claude/preferences.json`:**
   ```json
   {
     "workflow": {
       "mode": "manual"
     }
   }
   ```

2. **Run workflow:**
   ```
   /implement MyPlugin
   ```

3. **Observe:**
   ```
   ✓ Build System Ready

   What's next?
   1. Continue to Stage 2 (recommended)
   2. Test build
   3. Pause workflow
   4. Review code
   5. Other

   Choose (1-5): _
   ```

---

## Troubleshooting

### "Mode not taking effect"

**Problem:** Changed preferences.json but still seeing menus

**Solution:** Check precedence order
- Are you using a flag? (`--manual` overrides preferences)
- Is the file valid JSON? (syntax errors → default to manual)
- Is the mode value correct? ("express" or "manual" only)

### "Want express mode for one plugin only"

**Problem:** Don't want to change preferences.json

**Solution:** Use flag override
```
/implement PluginName --express
```
Temporary override, preferences unchanged.

### "Workflow paused, want to change mode"

**Problem:** Started in manual mode, want to switch to express

**Solution:** Use flag on resume
```
/continue PluginName --express
```
Switches to express mode for remainder of workflow.

### "Build error, stuck in manual mode"

**Problem:** Express mode dropped to manual after error, won't re-enable

**Expected behavior:** Safety feature (errors require manual intervention)

**Solution:**
1. Fix the error
2. Resume: `/continue PluginName --express` (force express mode)
3. Workflow continues in express mode

---

## Summary

| Preference | Values | Default | Effect |
|------------|--------|---------|--------|
| `workflow.mode` | "express", "manual" | "manual" | Auto-progress vs menus |
| `workflow.auto_test` | true, false | false | DEPRECATED (validation automatic) |
| `workflow.auto_install` | true, false | false | Auto-install to system |
| `workflow.auto_package` | true, false | false | Auto-create PKG |

**Time savings with express mode:** 2-3 minutes per plugin (eliminates 4 decision points)

**Safety:** Express mode drops to manual on any error (no silent failures)

**Flexibility:** Command-line flags override preferences for one-off control
