# O-Reed Stage 0: Research & Planning Context

**Date:** 2026-04-04
**Agent:** research-planning-agent
**Status:** Complete

---

## Discuss Phase Findings

### Key Architecture Decisions

**1. Guillemain Psi Parameter as Core Differentiator**
The confined jet parameter (Psi) from Guillemain (2004) is the single most important architectural choice. It allows one continuous parameter to morph between single-reed and double-reed excitation character. No competitor implements this -- SWAM uses separate products for single and double reeds. This was validated by the research synthesis document which confirms the mathematical framework is well-understood and computationally cheap (a single additional term in the Bernoulli flow equation).

**2. Strategy B for Conical Bore (Correction Filter)**
After analyzing three bore implementation strategies from the DSP research, Strategy B (cylindrical waveguide + spectral correction filter) was chosen as the starting approach. Rationale:
- Strategy A (STK Saxofony variable blow position) is too crude for a serious PM synth
- Strategy B provides good timbral accuracy with smooth real-time bore morphing
- Strategy C (true conical sections) deferred to v2.0 -- junction instabilities at taper transitions add risk without proportional quality gain for initial release
- The key advantage of Strategy B: bore taper becomes a single filter coefficient that can be smoothly interpolated, enabling real-time cylindrical-to-conical morphing

**3. MPESynthesiser over Synthesiser**
Reed instruments are fundamentally expressive -- breath pressure, embouchure, pitch bending are continuous dimensions. MPESynthesiser with enableLegacyMode() provides per-note expression when available and falls back to standard MIDI CC routing otherwise. This is the same pattern used in O-Formant and O-Bowed.

**4. Mono-First Design**
Reed instruments are fundamentally monophonic. The architecture prioritizes mono mode quality (4x oversampling, NR solver, smooth legato) while keeping poly mode available for sound design. This means quality-tiered processing: mono gets the best algorithm (Newton-Raphson), poly gets the fast one (polynomial approximation).

**5. Static Reed -> Dynamic Reed Progression**
The DSP research recommends starting with the STK-style static reed table (piecewise linear with clamp), then upgrading to the full mass-spring-damper ODE. This is validated by the Phase 3.1 -> 3.2 progression: Phase 3.1 proves the bore waveguide works with a known-good excitation, Phase 3.2 adds the dynamic reed and Psi without risking the bore implementation.

### Complexity Assessment

Raw complexity score is 16.4 (capped at 5.0), comparable to O-Bowed (18.0 raw). The complexity comes from:
- 35 parameters (most of any Ouaricon plugin)
- 9+ DSP components, all custom or lightly wrapping JUCE primitives
- Nonlinear reed-bore coupling (implicit equation solving)
- Bore morphing (cylindrical to conical to reverse)
- Full MPE + breath controller integration
- Microtonal tuning system

This is a Tier 6 complexity plugin requiring 5 DSP phases and 3 GUI phases.

### Research Synthesis

Four pre-existing research documents provided the foundation for this architecture:
1. **reed-physical-modeling-dsp.md** -- Complete algorithmic reference with C++ pseudocode for reed model, Bernoulli flow, bore waveguide, tone holes, and all extended techniques
2. **O-Reed-market-research.md** -- Confirmed zero competition in non-Western PM reed instruments (VST3/AU), validated $129 price point, identified SWAM weaknesses
3. **O-Reed-acoustic-properties-reed-instruments.md** -- Bore dimensions and reed properties for 12+ instruments, confirming parameter ranges for instrument presets
4. **O-Reed-research-synthesis.md** -- Unified architecture with instrument preset table and 4-stage DSP roadmap (adapted to 5 phases for pipeline compatibility)

The research was comprehensive enough that minimal additional web search was needed. The architecture is primarily a synthesis and JUCE API mapping exercise from these documents.

### Risk Assessment

**Highest risk:** Reed-bore nonlinear junction (~40% of project risk)
- Implicit coupling between reed ODE and bore pressure
- Guillemain Psi extends the nonlinearity -- must verify stability across 0-1.0
- Mitigation: start with static reed (proven STK approach), add dynamics incrementally

**Medium risk:** Conical bore correction filter
- Strategy B is an approximation
- May not capture all conical dynamics at extreme taper values
- Mitigation: Strategy A fallback is always available; Strategy C upgrade path documented

**Lower risk:** All other components are well-understood DSP primitives (delay lines, filters, LFOs) with JUCE class support.

### Constraints and Approach

- **IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE** required in CMakeLists.txt (per juce8-critical-patterns.md)
- **Output-only BusesProperties** (instrument, no audio input bus)
- **Shared tuning module** at modules/tuning/scala-tuning-engine (proven in O-Prism, O-Lyrica, O-Bowed)
- **getLatencySamples() NOT virtual** in JUCE 8 -- use setLatencySamples() only
- **enableLegacyMode()** is on MPESynthesiserBase, not MPESynthesiser directly
- **MPESynthesiserVoice has 6 pure virtual methods:** noteStarted(), noteStopped(bool), notePressureChanged(), notePitchbendChanged(), noteTimbreChanged(), renderNextBlock()

---

## Files Created

- `plugins/O-Reed/.planning/research/ARCHITECTURE.md` -- Complete DSP architecture (16 components, full signal flow)
- `plugins/O-Reed/.planning/ROADMAP.md` -- Implementation plan (5 DSP phases + 3 GUI phases)
- `plugins/O-Reed/.planning/stages/0-ideation/CONTEXT.md` -- This file
- `plugins/O-Reed/.planning/STATUS.md` -- Stage tracking
