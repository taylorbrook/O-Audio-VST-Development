---
name: plugin-execute
description: Run stage-specific implementation agent
skill: plugin-phases
args: "[plugin_name?] [stage?] [--force?]"
---

# /plugin-execute

Execute the implementation plan using the stage-specific agent. This is where code gets written.

## Usage

```
/plugin-execute [plugin_name] [stage]           # Specific plugin and stage
/plugin-execute [stage]                         # Focused plugin, specific stage
/plugin-execute                                 # Focused plugin, current stage
/plugin-execute [plugin_name] [stage] --force   # Bypass gate with justification
```

## Arguments

- `plugin_name` - Plugin to execute (optional, defaults to focused)
- `stage` - Stage to execute: `0-ideation`, `1-foundation`, `2-dsp`, `3-gui`, `4-polish`
- `--force` - Bypass quality gate if it fails (requires justification)

## Prerequisites

**Plan phase must be complete.** PLAN.md is required.

## Quality Gate Check

Before executing a stage, a quality gate validates the previous stage is complete.

**Gate invocation:**
```bash
.planning/workflow/scripts/run-gate.sh [plugin] [from-stage] [stage]
```

**Stage transitions:**
| Target Stage | From Stage | Gate Checks |
|--------------|------------|-------------|
| 1-foundation | 0-ideation | Schema, artifacts |
| 2-dsp | 1-foundation | Schema, build, artifacts |
| 3-gui | 2-dsp | Schema, build, pluginval, DSP critic |
| 4-polish | 3-gui | Schema, build, pluginval, DSP + UI critics |

**Gate results:**
- Exit 0 (PASSED): Proceed with stage execution
- Exit 1 (BLOCKED): Stop with "Gate blocked. Fix issues or use --force."
- Exit 2 (BYPASSED): Log warning and proceed (when --force used)

**Using --force:**
When the gate fails but you need to proceed, use `--force`. This:
1. Prompts for justification (required)
2. Logs bypass to `gate-bypasses.log`
3. Generates report with bypass details
4. Proceeds with stage execution

**NOTE:** `--force` should be rare. Address gate failures when possible.

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
2. **Runs quality gate** (run-gate.sh) for stage transition
   - If PASSED: continue
   - If BLOCKED: stop (unless --force)
   - If BYPASSED: log warning and continue
3. Loads all context (CONTEXT.md, RESEARCH.md, PLAN.md)
4. Invokes stage-specific agent
5. Agent executes tasks from PLAN.md
6. Creates `stages/[N]-[name]/SUMMARY.md`
7. Updates phase progress

## Example

```
/plugin-execute O-IntonationPad 2-dsp

Running quality gate: 1-foundation -> 2-dsp
══════════════════════════════════════════════════════════════
CRITICAL CHECKS
  [schema] PASSED (45ms)
  [build] PASSED (2341ms)

GATE PASSED

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

Next: /plugin-verify O-IntonationPad 2-dsp
```

**Example with --force (gate bypass):**

```
/plugin-execute O-IntonationPad 2-dsp --force

Running quality gate: 1-foundation -> 2-dsp
══════════════════════════════════════════════════════════════
CRITICAL CHECKS
  [schema] PASSED (45ms)
  [build] FAILED (1234ms)

GATE BLOCKED

Gate blocked, but --force requested
Failed checks: build
Justification (required): > Build fails due to missing module, will fix after stage

GATE BYPASSED
Bypass logged to: gate-bypasses.log

Executing Stage 2 (DSP) plan...
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

- `/plugin-plan` - Previous phase
- `/plugin-verify` - Next phase
- `/plugin-status` - Check progress
