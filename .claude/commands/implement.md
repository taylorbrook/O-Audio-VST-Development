---
name: implement
description: Build plugin through implementation stages 1-4 with GSD phase cycles
argument-hint: "[PluginName?] [--express] [--auto] [--skip-discuss] [--skip-research] [--skip-verify]"
skill: plugin-workflow
---

# /implement

Build a plugin through implementation stages 1-4, with each stage running a full GSD phase cycle (discuss → research → plan → execute → verify).

## Usage

```
/implement [plugin_name] [flags]    # Specific plugin with options
/implement [flags]                  # Focused plugin with options
/implement                          # Focused plugin, default options
```

## Arguments

- `plugin_name` - Plugin to implement (optional, defaults to focused)

## Flags

| Flag | Description |
|------|-------------|
| `--express` | Auto-progress through all stages and phases |
| `--auto` | Auto-generate discuss/research/plan without user interaction |
| `--manual` | Show decision menus at each checkpoint |
| `--skip-discuss` | Skip discuss phase (use existing context) |
| `--skip-research` | Skip research phase |
| `--skip-verify` | Skip verify phase (not recommended) |

## Prerequisites

Planning (Stage 0) must be complete. Required files:
- `plugins/[Name]/.planning/research/ARCHITECTURE.md`
- `plugins/[Name]/.planning/ROADMAP.md`
- `plugins/[Name]/.planning/parameter-spec.md`

If missing, run `/plan [PluginName]` first.

## Stages and Phases

Each stage runs a full GSD cycle:

```
┌─────────────────────────────────────────────────────────────┐
│ Stage 1: Foundation                                         │
│   discuss → research → plan → execute → verify              │
│   Agent: foundation-shell-agent                             │
│   Output: CMake, project structure, APVTS parameters        │
├─────────────────────────────────────────────────────────────┤
│ Stage 2: DSP                                                │
│   discuss → research → plan → execute → verify              │
│   Agent: dsp-agent                                          │
│   Output: Audio processing, algorithms                      │
├─────────────────────────────────────────────────────────────┤
│ Stage 3: GUI                                                │
│   discuss → research → plan → execute → verify              │
│   Agent: gui-agent                                          │
│   Output: WebView UI, parameter binding                     │
├─────────────────────────────────────────────────────────────┤
│ Stage 4: Polish                                             │
│   discuss → research → plan → execute → verify              │
│   Agent: polish-agent                                       │
│   Output: Presets, optimization, edge cases                 │
└─────────────────────────────────────────────────────────────┘
```

## Workflow Mode

**Mode precedence:**
1. Command-line flag (`--express` or `--manual`)
2. `.claude/preferences.json` (workflow.mode)
3. Default: "manual"

**Express mode:**
- Auto-progresses through all phases and stages
- Creates CONTEXT.md from existing docs (skips interactive discuss)
- Drops to manual mode on any error
- Final menu always appears after Stage 4

**Manual mode:**
- Presents decision menus after each phase
- Interactive discuss phase with questions
- Full control over progression

**Auto mode (`--auto`):**
- Auto-generates CONTEXT.md from existing contracts (BRIEF.md, parameter-spec.md)
- Auto-runs research phase non-interactively
- Auto-generates PLAN.md from research output
- Execute and verify phases run normally
- Falls back to manual mode on any error
- Different from express: express auto-advances but still runs each phase normally; auto generates planning artifacts without interaction

## Behavior

1. **Resolve plugin name** (use focused if not specified)
2. **Verify preconditions** (Stage 0 complete, contracts exist)
4. **Determine workflow mode** (flag > preferences > default)
5. **For each stage 1-4:**
   - Run discuss phase (or skip if `--skip-discuss`)
   - Run research phase (or skip if `--skip-research`)
   - Run plan phase (always)
   - Run execute phase (always)
   - Run verify phase (or skip if `--skip-verify`)
   - Update STATUS.md
   - Present checkpoint menu (manual) or auto-advance (express)
6. **On completion:** Update PLUGINS.md to ✅ Working

## Examples

```bash
# Implement focused plugin with express mode
/implement --express

# Implement specific plugin manually
/implement O-IntonationPad --manual

# Skip optional phases for faster iteration
/implement O-IntonationPad --express --skip-discuss --skip-research

# Fully automated: generate plans and implement without interaction
/implement O-IntonationPad --auto

# Resume from where you left off
/continue O-IntonationPad --express
```

## Phase Output Files

Each stage creates phase artifacts in `plugins/[Name]/.planning/stages/[N]-[name]/`:

| Phase | Output | Description |
|-------|--------|-------------|
| discuss | CONTEXT.md | Requirements, constraints, decisions |
| research | RESEARCH.md | API patterns, algorithms, modules |
| plan | PLAN.md | Task breakdown, success criteria |
| execute | SUMMARY.md | What was implemented |
| verify | VERIFICATION.md | Goal achievement, technical validation |

## Error Handling

**Build failure:**
```
✗ Build failed at Stage 2, Phase execute

Options:
1. View build log
2. Investigate with /research
3. Re-run execute phase
4. Pause for manual fix
```

**Verification failure:**
```
⚠ Verification failed for Stage 2 (DSP)

Issues:
- processBlock not generating audio

Options:
1. View VERIFICATION.md details
2. Re-run execute phase
3. Investigate issue
4. Accept with warning
```

## Pause & Resume

Pause workflow at any time. State is saved to:
- `plugins/[Name]/.planning/STATUS.md`
- PLUGINS.md

Resume with `/continue [PluginName]` or `/plugin-resume [PluginName]`

## Integration

**Precedes:** `/install-plugin`, `/package`, `/publish`
**Related:** `/continue`, `/plugin-status`, `/plugin-pause`
