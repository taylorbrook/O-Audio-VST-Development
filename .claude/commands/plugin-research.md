---
name: plugin-research
description: Investigate implementation approach for a stage
skill: plugin-phases
args: "[plugin_name?] [stage?]"
---

# /plugin-research

Investigate implementation approach for a stage. Researches JUCE APIs, algorithms, existing modules, and potential pitfalls.

## Usage

```
/plugin-research [plugin_name] [stage]    # Specific plugin and stage
/plugin-research [stage]                  # Focused plugin, specific stage
/plugin-research                          # Focused plugin, current stage
```

## Arguments

- `plugin_name` - Plugin to research (optional, defaults to focused)
- `stage` - Stage to research: `0-ideation`, `1-foundation`, `2-dsp`, `3-gui`, `4-polish`

## Behavior

1. Loads CONTEXT.md from discuss phase (if exists)
2. Researches:
   - Relevant JUCE APIs and patterns
   - Algorithm approaches
   - Existing modules that could be reused
   - Pitfalls from troubleshooting knowledge base
3. Creates `stages/[N]-[name]/RESEARCH.md`
4. Updates phase progress

## Example

```
/plugin-research O-IntonationPad 2-dsp

Researching Stage 2 (DSP) implementation
══════════════════════════════════════════════════════════════

Investigating:
- JUCE Synthesiser voice management
- Just intonation ratio calculation algorithms
- Wavetable oscillator implementation patterns
- Module: scala-tuning-engine (potential reuse)

Research Findings:
─────────────────────────────────────────────────────────────

1. Voice Management
   - juce::Synthesiser handles polyphony automatically
   - Custom voice stealing: override findFreeVoice()
   - See: troubleshooting/patterns/synth-voice-management.md

2. JI Ratio Calculation
   - 5-limit ratios: 1/1, 9/8, 5/4, 4/3, 3/2, 5/3, 15/8
   - 7-limit adds: 7/4, 7/6, 21/16
   - Algorithm: factor into prime powers, compute ratio

3. Module Opportunity
   - scala-tuning-engine provides:
     ✓ Scala file parsing
     ✓ Frequency table generation
     ✓ Tonic transposition
   - Recommend: /module-add O-IntonationPad scala-tuning-engine

4. Wavetable Implementation
   - Use juce::dsp::LookupTableTransform for efficiency
   - Cubic interpolation recommended for quality
   - Anti-aliasing: band-limit at Nyquist/2

✓ RESEARCH.md created
Next: /plugin-plan O-IntonationPad 2-dsp
```

## Skip Flag

To skip research and go directly to planning:
```
/plugin-plan O-IntonationPad 2-dsp --skip-research
```

## Related Commands

- `/plugin-discuss` - Previous phase
- `/plugin-plan` - Next phase
- `/module-add` - Add discovered modules
