---
name: continue
description: Resume plugin development from checkpoint (phase-aware)
argument-hint: "[PluginName?] [--express] [--skip-discuss] [--skip-research] [--skip-verify]"
skill: context-resume
---

# /continue

Resume plugin development from the last checkpoint. Restores context and continues at the exact stage and phase where work was paused.

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

## Behavior

1. **Resolve plugin name:**
   - If provided: Use specified plugin
   - If not provided: Use focused plugin from registry
   - If no focused plugin: Show menu of resumable plugins

2. **Load state:**
   - Read `plugins/[Name]/.planning/STATUS.md`
   - Parse current stage and phase from frontmatter
   - Read handoff context

3. **Determine workflow mode:**
   - Flag override (`--express` or `--manual`) takes priority
   - Otherwise use `express_mode` from STATUS.md
   - Default to "manual" if not set

4. **Present context summary:**
   ```
   Resuming O-IntonationPad
   ══════════════════════════════════════════════════════════════

   Stage: 2-dsp
   Phase: plan

   Completed:
   - Stage 1 (Foundation): ✓ all 5 phases
   - Stage 2 (DSP): discuss ✓, research ✓

   Handoff Context:
   ─────────────────────────────────────────────────────────────
   Working on: JI ratio calculation for chord generation
   Key decisions: 5-limit JI for triads, 7-limit for extensions

   Continue with:
   1. /plugin:plan O-IntonationPad 2-dsp (recommended)
   2. /plugin:research O-IntonationPad 2-dsp (re-research)
   3. /implement O-IntonationPad --express (auto-complete all)
   ```

5. **Route to continuation:**
   - For stages 1-4: Route to plugin-workflow skill
   - For stage 0 (ideation/planning): Route to appropriate skill

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

## Registry Integration

On resume:
1. Set plugin as focused in registry
2. Update `lastActivity` timestamp
3. Set status to "active" (if was "paused")

## No Resumable Work

**If no focused plugin and no plugin specified:**
```
No plugin currently focused.

Resumable plugins:
1. O-IntonationPad (Stage 2-dsp, Phase plan)
2. O-NewPlugin (Stage 1-foundation, Phase discuss)

Choose a plugin or use: /plugin:focus [name]
```

**If specified plugin has no handoff:**
```
O-SomePlugin doesn't have resumable state.

Status: ✅ Working (complete)

Options:
- /improve O-SomePlugin (add features/fix bugs)
- /plugin:status O-SomePlugin (view details)
```

## Phase-Aware Resume

The continue command resumes at the exact phase within a stage:

```
# If paused at Stage 2, Phase research:
/continue O-IntonationPad

Resuming at Stage 2 (DSP), Phase research
- Previous: discuss ✓
- Current: research →
- Remaining: plan, execute, verify
```

## Related Commands

- `/plugin:resume` - Alternative resume command (same behavior)
- `/plugin:status` - View status without resuming
- `/plugin:pause` - Pause current work
- `/implement` - Start or restart implementation
