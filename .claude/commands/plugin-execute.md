---
name: plugin:execute
description: Run stage-specific implementation agent
skill: plugin-phases
args: "[plugin_name?] [stage?]"
---

# /plugin:execute

Execute the implementation plan using the stage-specific agent. This is where code gets written.

## Usage

```
/plugin:execute [plugin_name] [stage]     # Specific plugin and stage
/plugin:execute [stage]                   # Focused plugin, specific stage
/plugin:execute                           # Focused plugin, current stage
```

## Arguments

- `plugin_name` - Plugin to execute (optional, defaults to focused)
- `stage` - Stage to execute: `0-ideation`, `1-foundation`, `2-dsp`, `3-gui`, `4-polish`

## Prerequisites

**Plan phase must be complete.** PLAN.md is required.

## Stage Agents

| Stage | Agent | What it does |
|-------|-------|--------------|
| 0-ideation | plugin-ideation | Creates BRIEF.md and parameter-spec.md |
| 1-foundation | foundation-shell-agent | CMake, project structure, APVTS |
| 2-dsp | dsp-agent | Audio processing implementation |
| 3-gui | gui-agent | WebView UI and parameter binding |
| 4-polish | polish-agent | Testing, presets, optimization |

## Behavior

1. Verifies PLAN.md exists
2. Loads all context (CONTEXT.md, RESEARCH.md, PLAN.md)
3. Invokes stage-specific agent
4. Agent executes tasks from PLAN.md
5. Creates `stages/[N]-[name]/SUMMARY.md`
6. Updates phase progress

## Example

```
/plugin:execute O-IntonationPad 2-dsp

Executing Stage 2 (DSP) plan
══════════════════════════════════════════════════════════════

Loading: PLAN.md (8 tasks)
Agent: dsp-agent

Progress:
─────────────────────────────────────────────────────────────

[1/8] Adding scala-tuning-engine module...
      ✓ CMakeLists.txt updated
      ✓ modules.json created

[2/8] Creating JIChordGenerator class...
      ✓ Source/DSP/JIChordGenerator.h
      ✓ Source/DSP/JIChordGenerator.cpp

[3/8] Creating WavetableVoice class...
      ✓ Source/DSP/WavetableVoice.h
      ✓ Source/DSP/WavetableVoice.cpp

[4/8] Implementing voice management...
      ✓ PluginProcessor.cpp updated

[5/8] Connecting parameters to DSP...
      ✓ Parameter bindings added

[6/8] Adding randomization...
      ✓ Timing, detuning, inversion randomization

[7/8] Implementing LFO modulation...
      ✓ LFO → wavetable position

[8/8] Adding filter section...
      ✓ Low-pass filter integrated

Building...
      ✓ Compilation successful

✓ Stage 2 (DSP) execution complete
✓ SUMMARY.md created

Next: /plugin:verify O-IntonationPad 2-dsp
```

## Error Handling

If execution fails (build error, etc.):
```
✗ Build failed at task 4

Options:
1. View build log
2. Investigate error
3. Re-run from failed task
4. Pause for manual fix
```

## Related Commands

- `/plugin:plan` - Previous phase
- `/plugin:verify` - Next phase
- `/plugin:status` - Check progress
