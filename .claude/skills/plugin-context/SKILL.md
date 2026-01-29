---
name: plugin-context
description: Manage plugin development contexts - list, focus, status, pause, resume. Core commands for multi-plugin workflow management.
allowed-tools:
  - Read
  - Write
  - Edit
  - Bash
  - AskUserQuestion
commands:
  - name: plugin:list
    description: List all plugins with stage/phase status
  - name: plugin:focus
    description: Set active plugin context
    args: "[plugin_name]"
  - name: plugin:status
    description: Detailed phase breakdown for a plugin
    args: "[plugin_name?]"
  - name: plugin:pause
    description: Checkpoint current work, create handoff document
    args: "[plugin_name?]"
  - name: plugin:resume
    description: Restore context from handoff and continue work
    args: "[plugin_name?]"
---

# plugin-context Skill

**Purpose:** Core plugin context management for multi-plugin workflows. Enables switching between plugins, viewing status, and managing work sessions.

## Commands

### /plugin:list

List all plugins with their current stage and phase.

**Output format:**
```
Plugin Registry
═══════════════════════════════════════════════════════════════

  PLUGIN              STAGE           PHASE      STATUS
  ─────────────────────────────────────────────────────────────
→ O-IntonationPad     2-dsp           plan       active
  O-Bass              complete        -          released
  O-Tremolo           complete        -          released
  O-Lyrica            4-polish        execute    active

Legend: → = focused
```

**Implementation:**
1. Run: `python3 .claude/scripts/plugin-registry.py list`
2. Format output as table
3. Mark focused plugin with arrow

### /plugin:focus [plugin_name]

Set the active plugin context. Subsequent commands without explicit plugin name will use this context.

**Implementation:**
1. Validate plugin exists in registry
2. Run: `python3 .claude/scripts/plugin-registry.py focus [plugin_name]`
3. Update STATUS.md frontmatter: `focused: true`
4. Clear `focused: true` from previously focused plugin's STATUS.md
5. Confirm: "Focused on [plugin_name] at stage [X], phase [Y]"

**If no plugin_name provided:**
Display currently focused plugin or "(none)"

### /plugin:status [plugin_name?]

Show detailed phase breakdown for a plugin.

**Arguments:**
- `plugin_name`: Plugin to show status for (defaults to focused plugin)

**Output format:**
```
O-IntonationPad Status
══════════════════════════════════════════════════════════════

Stage: 2 of 4 (DSP)
Phase: plan
Mode: express

Phase Progress
──────────────────────────────────────────────────────────────

  Stage 0: Ideation                               ✓ COMPLETE
  ├── discuss ✓  research ✓  plan ✓  execute ✓  verify ✓

  Stage 1: Foundation                             ✓ COMPLETE
  ├── discuss ✓  research ✓  plan ✓  execute ✓  verify ✓

  Stage 2: DSP                                    → IN PROGRESS
  ├── discuss ✓  research ✓  plan →  execute ·  verify ·

  Stage 3: GUI                                    · PENDING
  ├── discuss ·  research ·  plan ·  execute ·  verify ·

  Stage 4: Polish                                 · PENDING
  ├── discuss ·  research ·  plan ·  execute ·  verify ·

Module Dependencies: scala-tuning-engine@1.0.0

Resume: /continue O-IntonationPad
Next: /plugin:plan O-IntonationPad 2-dsp
```

**Implementation:**
1. Resolve plugin name (use focused if not provided)
2. Read STATUS.md from `plugins/[name]/.planning/STATUS.md`
3. Parse frontmatter for current state
4. Parse phase progress tables
5. Format output

### /plugin:pause [plugin_name?]

Checkpoint current work and create handoff document for later resumption.

**Implementation:**
1. Resolve plugin name
2. Read current state from STATUS.md
3. Prompt for handoff context:
   - "What were you working on?"
   - "Any key decisions or context to preserve?"
   - "Any blockers or concerns?"
4. Update STATUS.md Handoff Context section
5. Update registry status to "paused"
6. Git commit: "checkpoint: [plugin_name] paused at stage [X] phase [Y]"
7. Confirm: "Paused [plugin_name]. Resume with /plugin:resume [plugin_name]"

### /plugin:resume [plugin_name?]

Restore context from handoff and continue work.

**Implementation:**
1. Resolve plugin name
2. Read STATUS.md handoff context
3. Read last stage's artifacts (CONTEXT.md, PLAN.md, etc.)
4. Update registry status to "active"
5. Set as focused plugin
6. Present context summary:
   ```
   Resuming O-IntonationPad
   ═══════════════════════════════════════════════════════════

   Stage: 2-dsp
   Phase: plan

   Handoff Context:
   - Working on JI ratio calculation for chord generation
   - Decided to use 5-limit JI for triads, 7-limit for extensions

   Continue with:
   1. /plugin:plan O-IntonationPad 2-dsp (recommended)
   2. /plugin:discuss O-IntonationPad 2-dsp
   3. /plugin:status O-IntonationPad
   ```

## State Management

### Plugin Registry
Global state stored in `.claude/plugin-registry.json`:
- Focused plugin tracking
- All plugins with stage/phase
- Module dependencies

### STATUS.md
Plugin-local state in `plugins/[name]/.planning/STATUS.md`:
- Detailed phase progress tables
- Handoff context
- Module dependencies
- Accumulated decisions

**Source of truth:** STATUS.md is authoritative. Registry is synced from STATUS.md.

## Integration Points

**Reads:**
- `.claude/plugin-registry.json`
- `plugins/[name]/.planning/STATUS.md`

**Writes:**
- `.claude/plugin-registry.json`
- `plugins/[name]/.planning/STATUS.md`

**Used by:**
- All `/plugin:*` commands
- `/continue` (routes through plugin:resume)
- `/implement` (checks focused context)

## Error Handling

**Plugin not found:**
```
Error: Plugin 'FooBar' not found.

Available plugins:
- O-IntonationPad (active)
- O-Bass (released)
- O-Tremolo (released)

Create new plugin with: /start FooBar
```

**No focused plugin:**
```
No plugin currently focused.

Use /plugin:focus [name] or /plugin:list to see available plugins.
```

**Registry out of sync:**
```
Warning: Registry out of sync with STATUS.md

Syncing from STATUS.md...
Updated: stage 2-dsp → 3-gui, phase execute → discuss

Use /reconcile for full state verification.
```
