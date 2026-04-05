# O-Bowed Stage 0: Research & Planning Context

**Date:** 2026-04-04
**Agent:** research-planning-agent
**Complexity Score:** 5.0 (Maximum)

---

## Key Decisions Made

### 1. General-Purpose Bowed String Synthesizer (Not Instrument Emulator)

The creative brief and research synthesis both confirm: O-Bowed is a bowed string SYNTHESIZER, not a violin/cello emulator. This means:
- Violin, cello, erhu are presets within a continuous parameter space
- Body morphing is a core feature, not an add-on
- "Impossible physics" parameters are first-class controls
- Competing with SWAM on realism is a losing strategy -- compete on morphability and sound design

### 2. Tiered Friction: Core Always Active, Enhanced/Quality Optional

The friction model is the most important and most risky DSP component. Decision: implement as three additive tiers:
- **Core (hyperbolic):** Always active, O(1) per sample, memoryless. Based on physics-derived curve, not STK's empirical 4th-power.
- **Enhanced (+ elasto-plastic):** Adds bristle state for attack "bite" and hysteresis. NR solver adds ~3x CPU.
- **Quality (+ thermal):** Adds temperature tracking for sustained tone evolution. ~5x CPU.

Rationale: Users choose quality vs CPU tradeoff. Core tier must sound good on its own -- it's the baseline for all users.

### 3. Morphable Body Resonator via Biquad Bank (Not Convolution)

Body morphing is identified as the #1 market differentiator (no competitor offers this). Decision: 8 parallel biquads with coefficient interpolation between preset banks.

Convolution would be more accurate for a single instrument but is not continuously morphable. The biquad approach trades per-instrument accuracy for the ability to smoothly morph between membrane (erhu), wood (violin/cello), metal, and glass body types.

### 4. O-Lyrica Pattern for Voice Management

O-Lyrica provides a proven pattern: `juce::SynthesiserVoice` with APVTS pointer, WaveguideString class with dual delay lines, TuningEngine pointer, SympatheticResonanceEngine. Reuse this pattern rather than inventing new architecture.

### 5. Phased DSP Implementation (5 Phases)

At complexity 5.0, single-pass DSP implementation is infeasible. The 5 phases build incrementally:
1. Core waveguide + basic friction (validate basic bowed sound)
2. Body resonator + stereo (validate instrument character)
3. Multi-string + sympathetic (validate ensemble capabilities)
4. Advanced friction + impossible physics (validate advanced features)
5. Oversampling + tuning + optimization (validate performance)

Each phase produces a testable intermediate state. Phase 1 must produce a recognizable bowed string tone before proceeding.

---

## Constraints Identified

### CPU Budget
- Target: <6% for 2 strings core tier (erhu-like configuration)
- Maximum: ~25% for 4 strings quality tier + 12 sympathetic
- The friction model at quality tier is ~5x core -- expensive for multi-string
- 2x oversampling adds ~2.2x multiplier
- Sympathetic strings must have gating (skip if below energy threshold)

### Nonlinear Solver Risk
- Newton-Raphson can diverge for extreme parameter combinations
- Friedlander ambiguity (3 intersections) requires hysteresis tracking
- Elasto-plastic model has passivity risk without velocity-dependent damping fix
- Mitigation: always have core tier as fallback, clamp NR iterations, use previous solution on divergence

### JUCE 8 Build Requirements
- `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` in CMakeLists.txt (critical -- without these, no MIDI routing)
- Output-only BusesProperties (no audio input bus for instruments)
- `juce_dsp` module required for DelayLine, IIR::Filter, Oversampling
- `juce_gui_extra` for WebView UI
- `juce_generate_juce_header()` after target_link_libraries
- WebView: `NEEDS_WEB_BROWSER TRUE`, `JUCE_WEB_BROWSER=1`

### Shared Module Dependency
- `modules/tuning/scala-tuning-engine` at v2.1.0 exists and is proven (O-Prism, O-Lyrica)
- Must link via CMake, not duplicate code
- MTS-ESP support depends on optional runtime library

---

## Research Sources Used

All 4 pre-existing research documents were consumed in full:

1. **O-Bowed-research-synthesis.md** -- Provided the unified 10-section architecture covering strategic direction, modular design, friction tiers, waveguide topology, body resonance design, performance targets, parameter mapping, and implementation roadmap.

2. **bow-string-friction-models.md** -- Provided complete equations and C++ pseudocode for all 4 friction models (hyperbolic, STK bow table, elasto-plastic, thermal), waveguide scattering junction math, Newton-Raphson solver, Friedlander ambiguity resolution, Schelleng diagram equations, and CPU cost estimates.

3. **O-Bowed-market-research.md** -- Provided competitive analysis against SWAM, Soliste, String Studio VS-3, Preparation 2. Confirmed body morphing as uncontested differentiator. Identified erhu as blue-ocean market opportunity.

4. **O-Bowed-acoustic-instrument-research.md** -- Provided instrument-specific data: violin/cello/erhu/nyckelharpa string properties, body mode frequencies, bow parameter ranges, sympathetic string configurations, membrane resonator acoustics, and feasibility matrix.

---

## Open Questions for Implementation

1. **Body coefficient tuning:** The preset coefficient banks in ARCHITECTURE.md are approximate starting points from acoustic research. They will need empirical tuning during DSP Phase 3.2 by listening and A/B comparing against real instrument recordings.

2. **Vibrato implementation:** The creative brief mentions vibrato but doesn't include explicit vibrato rate/depth parameters. Research synthesis suggests LFO modulating neckDelay length. May need to add VIBRATO_RATE and VIBRATO_DEPTH parameters during implementation, or derive from CC1 (mod wheel).

3. **Wolf tone:** The cello preset should exhibit wolf tone coupling (~175 Hz) for authenticity. This can be achieved by tuning body resonator Q values but may require a dedicated coupling mechanism. Document as optional Phase 3.4 feature.

4. **Articulations:** The research mentions detache, spiccato, tremolo, martele as future articulation types. These are v1.x features, not v1.0. The bow envelope attack/release time combined with velocity mapping provides basic articulation variety.

5. **MTS-ESP runtime dependency:** MTS-ESP requires loading a dynamic library at runtime. If unavailable, the option should be grayed out in UI. Test on systems without MTS-ESP installed.
