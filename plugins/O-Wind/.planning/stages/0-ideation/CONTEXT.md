# O-Wind Stage 0: Research & Planning Context

**Date:** 2026-04-04
**Agent:** research-planning-agent
**Duration:** Stage 0 research and planning session

---

## Discuss Phase Findings

### Key Architectural Decisions

1. **One-directional jet exciter eliminates the highest-risk component from O-Bowed.** The jet-drive model reads bore output from the previous sample and injects new energy -- no circular dependency, no Newton-Raphson solver. This is the single biggest simplification over O-Bowed.

2. **The bore IS the body.** Unlike bowed strings (where the body resonator is separate from the string), in flutes the bore waveguide with its tone holes and loss filters IS the resonant body. This eliminates O-Bowed's entire 8-biquad morphable body resonator system -- a major reduction in complexity and CPU.

3. **Two-tier tone hole architecture provides a safe implementation path.** Tier 1 (bore length switching) is trivial and produces correct pitches. Tier 2 (Keefe scattering junctions) adds realism but is complex. By making Tier 2 optional, we can ship a complete instrument even if the advanced model proves too difficult.

4. **tanh over cubic for jet nonlinearity.** STK uses cubic `x - x^3` which diverges for |x| > 1. tanh saturates gracefully and matches the physical model (Verge 1995). Stability benefit outweighs minimal CPU difference.

5. **Pressure vibrato, not pitch vibrato.** Real flute vibrato is breath-driven, producing correlated pitch + amplitude modulation. This is what SWAM and all serious implementations use.

### Research Synthesis

Three pre-existing research documents provided comprehensive coverage:
- **flute-physical-modeling-synthesis.md**: Complete jet-drive physics, bore modeling, tone holes, instrument variants with 18 academic references. This document alone covers 90% of the algorithmic research needs.
- **O-Wind-market-research.md**: Competitive landscape showing SWAM Flutes ($249) as market leader with clear gaps in ethnic flute modeling and affordable pricing.
- **flute-waveguide-juce8-implementation.md**: JUCE 8 class mapping already completed, including per-sample API verification, CPU estimates, and O-Bowed comparison.

The research phase was primarily a synthesis and validation exercise rather than a full web-search cycle (matching the learned pattern from O-Bowed).

### Complexity Assessment

Raw score 11.6 (capped at 5.0). While still maximum complexity, the raw score is significantly lower than O-Bowed (18.0). The difference comes from:
- 13 parameters vs 22 (fewer controls to manage)
- No iterative solver (eliminates highest-complexity component)
- No body resonator (eliminates morphable coefficient system)
- No sympathetic coupling (eliminates passive string bank)
- Simpler excitation model (one-directional vs bidirectional)

### Risk Profile

Overall risk is MEDIUM (vs O-Bowed's HIGH). The highest-risk component is overblowing register stability, which is well-documented in literature with known solutions. The core waveguide model has been proven stable in STK for 30+ years.

### Implementation Strategy

Four DSP phases:
1. **Minimal oscillation** -- validate jet + bore + feedback produces sound
2. **Expression** -- add noise, vibrato, oversampling, polyphony
3. **Creative features** -- impossible physics, instrument presets, stereo
4. **Advanced** -- Tier 2 tone holes, expansion presets, MPE, tuning

Phase 3.4 is explicitly optional. Phases 3.1-3.3 produce a complete, shippable instrument.

### Shared Module Reuse

Confirmed shared modules from O-Bowed:
- Tuning engine (Scala/MTS-ESP) at `modules/tuning/scala-tuning-engine`
- Stereo width processor (mid-side pattern)
- MIDI/MPE routing and voice allocation patterns
- Parameter smoothing patterns (SmoothedValue)
- SynthesiserVoice pattern (from O-Lyrica with TuningEngine + APVTS pointers)

### Market Differentiation

The strongest differentiator is ethnic flute coverage: no competitor offers dedicated physically-modeled shakuhachi, bansuri, or Native American flute. SWAM VariFlute can approximate these but requires user configuration. O-Wind delivers pre-tuned, authentic models out of the box.

---

## Constraints and Gotchas

- JUCE 8 `getLatencySamples()` is NOT virtual -- must use `setLatencySamples()` in prepareToPlay()
- IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE required in CMakeLists.txt (critical pattern #22)
- BusesProperties output-only for synth (no input bus -- critical pattern #4)
- DelayLine with Thiran is stateful, unsuitable for fast modulation -- use for bore (discrete pitch changes), Lagrange3rd for jet delay (continuous modulation)
- juce_generate_juce_header() must come after target_link_libraries (critical pattern #1)

---

## Files Created

- `plugins/O-Wind/.planning/research/ARCHITECTURE.md` -- Complete DSP architecture (14 core components, full signal flow, risk assessment)
- `plugins/O-Wind/.planning/ROADMAP.md` -- Implementation plan (4 DSP phases, 3 GUI phases)
- `plugins/O-Wind/.planning/stages/0-ideation/CONTEXT.md` -- This file
