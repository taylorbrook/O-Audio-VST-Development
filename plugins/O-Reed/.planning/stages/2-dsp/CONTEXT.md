# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Requirements Confirmed

- Full mass-spring-damper reed ODE from Phase 3.1 (no static reed table intermediate)
- True conical waveguide sections (Strategy C) from the start (no Strategy B correction filter)
- No CPU budget constraint -- quality first, optimize in Phase 3.5
- Phase ordering confirmed: 3.1 -> 3.2 -> 3.3 -> 3.4 -> 3.5
- O-Bowed is also in development and should NOT be treated as a reference pattern -- independent implementation

## Constraints Identified

- Strategy C increases Phase 3.1 complexity (spherical wave scaling in bore waveguide from day one)
- Dynamic reed in 3.1 means the nonlinear junction must be solved from the first phase
- No CPU ceiling means oversampling and solver quality can be aggressive during development

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Bore taper strategy | Strategy C (true conical sections) | User wants accuracy from the start, no intermediate correction filter |
| Reed model in Phase 3.1 | Full mass-spring-damper ODE | User wants the real character immediately, not a simplified approximation |
| CPU budget | Unconstrained during dev | Quality over performance; optimization pass in Phase 3.5 |
| Phase ordering | 3.1 -> 3.2 -> 3.3 -> 3.4 -> 3.5 | Each phase builds on previous, confirmed as sound |
| Reference pattern | Independent of O-Bowed | O-Bowed is still in development; don't couple designs |

## Revised Phase 3.1 Scope

With Strategy C bore and dynamic reed pulled forward, Phase 3.1 now includes:

- **Reed:** Full mass-spring-damper ODE with symplectic Euler discretization
- **Junction:** Polynomial approximation of Bernoulli flow (Psi=0 for single-reed in 3.1)
- **Bore:** Bidirectional delay lines with Thiran allpass, spherical wave scaling for conical support (bore_character=0 default = cylindrical behavior)
- **Bell reflection:** First-order allpass reflection filter
- **Viscothermal loss:** One-pole lowpass per delay line loop
- **Input:** Breath pressure from velocity/CC2, note frequency -> delay length
- **Output:** Mono summed to stereo

Phase 3.2 then focuses on activating Psi confinement, bore morphing (bore_character > 0), breath noise, and mouthpiece volume -- components that extend the core engine rather than replace it.

## Parameters Active Per Phase

| Phase | New Parameters |
|-------|---------------|
| 3.1 | BREATH_PRESSURE, EMBOUCHURE, REED_HARDNESS, REED_OPENING, REED_MASS, REED_DAMPING, BORE_CHARACTER, BELL_SIZE, BORE_DIAMETER, BORE_LENGTH, OUTPUT_GAIN |
| 3.2 | DOUBLE_REED (Psi), AIR_NOISE, MOUTHPIECE_VOL, BORE_PROFILE |
| 3.3 | TONE_HOLE_CUTOFF, REGISTER_HOLE, VIBRATO_DEPTH/RATE/SOURCE, GROWL_AMOUNT, FLUTTER_TONGUE, SUBTONE, ATTACK_CHIFF, POLY_MODE, MAX_VOICES |
| 3.4 | INFINITE_SUSTAIN, REVERSE_BORE, DUAL_BORE, DRONE_PITCH, FEEDBACK_PATH |
| 3.5 | REFERENCE_PITCH, TUNING_SYSTEM, OVERSAMPLING, INSTRUMENT_PRESET |

## Open Questions

- None. All decisions resolved.

## Next Phase

Ready for: research phase
