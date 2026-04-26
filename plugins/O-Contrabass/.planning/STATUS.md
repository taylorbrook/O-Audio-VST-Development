---
plugin: O-Contrabass
stage: ideation
status: creative_brief_complete
last_updated: 2026-04-25
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief and requirements have been finalized for O-Contrabass. Ready to proceed to Stage 0 planning, UI mockup, or research.

## Completed So Far

**Ideation:** Complete
- Core concept defined: bass-only specialized 4-string contrabass physical model, sustained orchestral arco + ambient drone
- Differentiation from O-Bowed clarified (deep specialization vs general-purpose)
- Parameters specified across 7 sections (Bow, Body, Strings, Expression, Drone, Output, Microtonal)
- Sonic targets locked: deep wood body resonance, bow noise / rosin grit, slow expressive attack
- Use cases identified: orchestral mockup, cinematic pads, ambient drone, experimental drone, microtonal composition, MPE live performance
- Inspirations captured across orchestral, drone/experimental, and existing PM bass plugin categories
- Drone-first features designed in (infinite sustain, sub-harmonics, slow bow LFO, per-string detuning)
- Layered expression model defined (intrinsic CC + dedicated vibrato + macro)
- Full Ouaricon microtonal convention (Note Expression + MTS-ESP + Scala/TUN + MPE)
- Requirements extracted: 14 must / 7 should / 3 nice across FUNC, DSP, UI, PERF, COMPAT, QUAL

## Next Steps

1. Stage 0 planning — research DSP approach (waveguide stability at low fundamentals, friction model reuse from O-Bowed, body resonator design) and create architecture
2. UI mockup — visualize layout for the 7 parameter sections
3. Research drone-bass references and bass-range waveguide implementations

## Context to Preserve

**Key Decisions:**
- Plugin type: Synth (Physical Modeling Bowed Bass)
- Core concept: Specialized 4-string contrabass for sustained orchestral + drone use
- Polyphony: Mono (authentic single-string bowing)
- Range: E1-G3, standard 4-string EADG
- Body: fixed wood material (no morphing) — bass-tuned size and damping
- Expression: layered (intrinsic CC + dedicated vibrato section + macro knob)
- Microtonal: full Ouaricon convention
- Out of scope v1.0: pizz, col legno, polyphony, 5-string, sympathetic strings

**Relationship to O-Bowed:**
- O-Bowed remains the general-purpose bowed string synth
- O-Contrabass goes deep on bass register and drone use cases
- Friction model is a likely candidate for module extraction during implementation (shared with O-Bowed)

**Files Created:**
- plugins/O-Contrabass/.planning/BRIEF.md
- plugins/O-Contrabass/.planning/REQUIREMENTS.md
- plugins/O-Contrabass/.planning/STATUS.md
