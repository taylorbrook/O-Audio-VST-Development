# Stage 2: DSP Phase 3.4 - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User (Taylor Brook), Claude
**Phase:** Advanced Friction + Impossible Physics

## Requirements Confirmed

- ElastoPlasticFriction (enhanced tier): bristle state variable z, stick-slip hysteresis, attack "bite"
- ThermalFriction (quality tier): rosin temperature tracking, glass transition at ~49°C, sustained-tone evolution
- Newton-Raphson solver: 4-6 iterations max, bail with previous solution on divergence
- Friction tier selector: new choice parameter (frictionTier: Core/Enhanced/Quality) — 23rd APVTS parameter
- REVERSED_FRICTION (0-100%): blends normal and inverted friction curve, applies to ALL friction tiers
- SUB_HARMONICS (0-100%): per-voice asymmetric clipping in waveguide feedback path at half-frequency
- Drone strings stay on core (HyperbolicFriction) only — no tier switching for CPU budget
- Both enhanced AND quality tiers implemented in this phase (no deferral)

## Constraints Identified

- Newton-Raphson must converge reliably — clamp to 6 iterations, fallback to previous rho
- Elasto-plastic passivity: velocity-dependent damping fix (sigma_1_eff = sigma_1 * (1 + v_rel^2))
- Bristle displacement z and temperature T_contact are per-voice state — no cross-thread concerns
- Enhanced tier ~3x CPU of core, quality tier ~5x CPU of core (per ROADMAP estimates)
- Drones use core tier only (4 drones × core = ~8% vs 4 drones × quality = ~40%)
- Must preserve existing core tier behavior exactly (baseline regression check)
- Reversed friction is a curve transform wrapping any underlying friction model
- Sub-harmonics injection is inside waveguide loop, per-voice, pre-body resonator

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Friction tier selector | New choice parameter (A) | Sound difference, not just quality — users should hear and choose |
| Reversed friction scope | All tiers | Curve inversion is a transform that wraps any model |
| Sub-harmonics scope | Per-voice | Inside waveguide loop; processor-level would muddy polyphony |
| Drone friction tier | Core only | Background texture — enhanced/quality reserved for voiced strings, saves CPU |
| Both tiers in 3.4 | Yes | Tiered friction is THE differentiator — no deferral |

## Architecture Implications

### New Files Expected
- `Source/DSP/ElastoPlasticFriction.h` — enhanced tier with bristle state
- `Source/DSP/ThermalFriction.h` — quality tier extending elasto-plastic with temperature
- `Source/DSP/SubHarmonicsGenerator.h` — per-voice sub-octave injection

### Modified Files Expected
- `Source/BowedStringVoice.h/cpp` — friction tier dispatch, reversed friction blend, sub-harmonics in waveguide loop
- `Source/PluginProcessor.h/cpp` — new frictionTier choice parameter, read reversedFriction/subHarmonics params
- `Source/PluginEditor.h/cpp` — relay + attachment for frictionTier choice parameter
- `CMakeLists.txt` — add new source files

### Friction Tier Dispatch (per-voice)
- Voice reads `frictionTier` atomic int (0=core, 1=enhanced, 2=quality)
- Each voice owns instances of all three friction models
- Only the selected tier's `computeReflectionCoefficient()` is called per sample
- Tier switching: reset bristle/temperature state on switch to avoid stale state artifacts

### Reversed Friction Transform
- After computing rho from any tier: `rho_final = lerp(rho, 1.0 - rho, reversedAmount)`
- At 0%: normal behavior
- At 50%: flattened (reduced dynamic range)
- At 100%: fully inverted curve — synthetic excitation

### Sub-Harmonics in Waveguide Loop
- Asymmetric soft clipping on waveguide feedback signal
- Clips positive peaks harder than negative → introduces even harmonics at f/2
- SUB_HARMONICS (0-100%) controls wet/dry blend of clipped signal
- At 0%: bypassed (no processing cost)
- At 100%: prominent sub-octave content
- Applied per-voice inside waveguide `processSample()`, before bridge filter

## Parameters Status After Phase 3.4

**Connected (20/23):**
bowSpeed, bowPressure, bowPosition, rosin, brightness, infiniteSustain, outputLevel, bodyMaterial, bodySize, width, stringCount, stringTuning1-4, sympatheticAmount, sympatheticCount, reversedFriction, subHarmonics, frictionTier (NEW)

**Not Yet Connected (3):**
referencePitch, tuningSystem, bowNoise → Phase 3.5

## Open Questions (for research phase)

- Exact elasto-plastic ODE formulation: which variant of the LuGre/Dupont model?
- Newton-Raphson initial guess strategy for enhanced/quality tiers
- Thermal model time constants: heating rate, cooling rate, glass transition curve shape
- Sub-harmonics: exact clipping function (tanh asymmetric, polynomial, waveshaper table?)
- Bristle state reset strategy on tier switch: hard reset vs exponential decay to zero?
- Should frictionTier parameter be integer (0/1/2) or choice with string labels?

## Next Phase

Ready for: research phase
