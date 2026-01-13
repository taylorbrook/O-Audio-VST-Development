# OuariconSimpleReverb Notes

## Status
- **Current Status:** 🚧 Stage 0
- **Version:** 1.0.0 (planned)
- **Type:** Audio Effect (Reverb)
- **Complexity:** 4.2 (Complex)

## Lifecycle Timeline

- **2026-01-13:** Creative brief created - lightweight reverb for instrument chains
- **2026-01-13 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 4.2, Phase-based implementation)

## Known Issues

None (not yet implemented)

## Additional Notes

### Concept
Lightweight, CPU-efficient reverb designed to add subtle color and realism to instrument chains. Removes the "in-a-box" feel without dominating the mix.

### Parameters (6 total)
1. **Type** (Dropdown): Booth, Room, Hall, Spring, Plate, Ambient
2. **Character** (Knob): Warm ← → Bright
3. **Wet** (Knob): Reverb signal level
4. **Dry** (Knob): Original signal level
5. **Decay** (Knob): Reverb tail length
6. **Size** (Knob): Virtual room dimensions

### Design
- Aesthetic: Ouaricon Naturalist (botanical theme)
- Priority: CPU efficiency over complexity
- Future: Module version for embedding in other VSTs

### Sound Character
- Subtle, musical, transparent
- Natural early reflections
- Smooth decay
- Not a special effect reverb - utility focused

### DSP Architecture
**Core Components:**
- **Reverb Engine:** juce::dsp::Reverb (6 type variations)
- **Character Filter:** juce::dsp::IIR::Filter (warm/bright/neutral)
- **Dry/Wet Mixer:** juce::dsp::DryWetMixer (independent dry/wet gains)

**CPU Target:** ~20-30% single core at 48kHz (lightweight)

### Implementation Plan
**Strategy:** Phase-based implementation (Complex plugin, score 4.2)

**DSP Phases:**
1. Phase 3.1: Core Processing (Room reverb + dry/wet)
2. Phase 3.2: Type Switching (6 types)
3. Phase 3.3: Character Control (warm/bright filter)

**GUI Phases:**
1. Phase 4.1: Layout and Basic Controls (Ouaricon Naturalist aesthetic)
2. Phase 4.2: Parameter Binding (6 parameters)

### References
- Creative brief: `plugins/OuariconSimpleReverb/.ideas/creative-brief.md`
- DSP architecture: `plugins/OuariconSimpleReverb/.ideas/architecture.md`
- Implementation plan: `plugins/OuariconSimpleReverb/.ideas/plan.md`
- Reference plugins: FlutterVerb, DriveVerb, LushVerb

### Next Steps
Run `/implement OuariconSimpleReverb` to begin Stage 1 (Foundation + Shell)
