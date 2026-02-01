---
name: plugin-plan
description: Create execution plan with task breakdown
skill: plugin-phases
args: "[plugin_name?] [stage?] [--skip-discuss] [--skip-research]"
---

# /plugin-plan

Create a detailed execution plan with task breakdown for a stage. This plan guides the execute phase.

## Usage

```
/plugin-plan [plugin_name] [stage]        # Specific plugin and stage
/plugin-plan [stage]                      # Focused plugin, specific stage
/plugin-plan                              # Focused plugin, current stage
/plugin-plan [name] [stage] --skip-discuss --skip-research
```

## Arguments

- `plugin_name` - Plugin to plan (optional, defaults to focused)
- `stage` - Stage to plan: `0-ideation`, `1-foundation`, `2-dsp`, `3-gui`, `4-polish`
- `--skip-discuss` - Skip discuss phase, use existing context
- `--skip-research` - Skip research phase

## Behavior

1. Loads CONTEXT.md and RESEARCH.md (if exist)
2. Creates structured plan with:
   - Goal statement
   - Numbered task breakdown
   - Files to create/modify
   - Dependencies between tasks
   - Success criteria
3. Creates `stages/[N]-[name]/PLAN.md`
4. Updates phase progress

## Example

```
/plugin-plan O-IntonationPad 2-dsp

Creating execution plan for Stage 2 (DSP)
══════════════════════════════════════════════════════════════

Context: CONTEXT.md, RESEARCH.md loaded

PLAN.md
─────────────────────────────────────────────────────────────

## Goal

Implement the core DSP engine for O-IntonationPad: just intonation
chord generation, wavetable synthesis, and voice management.

## Tasks

1. [ ] Add scala-tuning-engine module dependency
   - Files: CMakeLists.txt, modules.json
   - Depends on: none

2. [ ] Create JIChordGenerator class
   - Files: Source/DSP/JIChordGenerator.h, .cpp
   - Depends on: Task 1

3. [ ] Create WavetableVoice class
   - Files: Source/DSP/WavetableVoice.h, .cpp
   - Depends on: Task 1

4. [ ] Implement voice management in Synthesiser
   - Files: PluginProcessor.cpp
   - Depends on: Tasks 2, 3

5. [ ] Connect parameters to DSP
   - Files: PluginProcessor.cpp
   - Depends on: Task 4

6. [ ] Add randomization (timing, detuning, inversions)
   - Files: Source/DSP/JIChordGenerator.cpp
   - Depends on: Task 2

7. [ ] Implement LFO → wavetable modulation
   - Files: Source/DSP/WavetableVoice.cpp
   - Depends on: Task 3

8. [ ] Add filter section
   - Files: PluginProcessor.cpp
   - Depends on: Task 4

## Success Criteria

- [ ] Playing single MIDI note produces multi-voice chord
- [ ] Chord voices tuned to JI ratios (verified with tuner)
- [ ] Wavetable position affects timbre
- [ ] LFO modulates wavetable smoothly
- [ ] All parameters connected and functional
- [ ] No audio glitches or clicks

✓ PLAN.md created (8 tasks)
Next: /plugin-execute O-IntonationPad 2-dsp
```

## Related Commands

- `/plugin-research` - Previous phase
- `/plugin-execute` - Next phase
- `/plugin-discuss` - Return to gather more context
