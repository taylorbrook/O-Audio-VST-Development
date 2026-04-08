# Stage 2: DSP Phase 3.2 - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude
**Scope:** Dynamic Reed Psi + Breath Noise + Mouthpiece Volume

## Phase 3.2 Goal

Activate Guillemain Psi confinement for double-reed morphing, add flow-dependent breath noise, and implement full mouthpiece chamber compliance. This extends the Phase 3.1 core engine with three physically-grounded additions that unlock the single-reed to double-reed continuum and add the breath texture that separates professional PM from toy models.

## Requirements Confirmed

- **Guillemain Psi confinement:** Modify Bernoulli flow equation in ReedModel to include the Psi confinement term from BRIEF.md. DOUBLE_REED param (0-1) maps to Psi (0 to ~0.8). At Psi=0 behavior must be identical to Phase 3.1 (regression safety).
- **Breath noise:** Flow-dependent turbulence noise injected at p_mouth, amplitude scaled by |u_reed|. Bandpass-filtered white noise (500-6000 Hz). AIR_NOISE param (0-1) controls mix level.
- **Mouthpiece volume:** Full mouthpiece chamber with acoustic mass + compliance (two state variables). MOUTHPIECE_VOL param (0-1) controls effective chamber volume. Adds sub-resonance and pitch correction.
- **BORE_PROFILE:** Param exists in APVTS but only "Simple" path active in 3.2. Multi-segment deferred to Phase 3.4.
- **Quality first, no CPU ceiling** — same as Phase 3.1.

## Constraints Identified

- Psi must not destabilize the reed-bore feedback loop at extreme values — clamp or soft-limit if needed
- Breath noise amplitude must track flow velocity to be physically correct (no constant-level noise)
- Mouthpiece volume must not introduce delay that throws off pitch accuracy — compensate in bore delay length
- Phase 3.1 behavior at default params (Psi=0, airNoise=0, mouthpieceVol=0) must be preserved exactly

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Psi implementation | Direct Guillemain equation in ReedModel::processSample | BRIEF equation: `u = sign(dp) * S_i * sqrt(2*|dp| / (rho * (1 + Psi * alpha^2 * S_i^2 / S_r^2)))`. Psi=0 collapses to current code |
| Noise injection point | p_mouth (pre-reed), amplitude scaled by |u_reed| | Guillemain (2005) approach + flow-dependent amplitude for physical accuracy. Turbulence scales with Reynolds number at constriction |
| Noise filtering | Bandpass 500-6000 Hz white noise | Matches measured breath noise spectrum for reed instruments |
| Mouthpiece volume model | Full chamber: acoustic mass + compliance (2 state vars) | Mass term differentiates clarinet barrel from sax neck. Two variables trivial cost for significant timbral accuracy |
| Bore profile | Simple only in 3.2, multi-segment deferred to 3.4 | Multi-segment is substantial; better treated alongside dual bore with proper attention |

## Components to Implement

### 1. Psi Confinement (modify ReedModel.h)

Current Bernoulli (line 128-131):
```cpp
float u_reed = copysign(1.0f, dp) * S_opening * sqrt(2.0f * abs_dp / rho_air);
```

New Guillemain Bernoulli:
```cpp
float alpha = S_opening / S_reed;  // S_reed = reed channel cross-section area
float psi_term = 1.0f + psi * alpha * alpha * S_opening * S_opening / (S_reed * S_reed);
float u_reed = copysign(1.0f, dp) * S_opening * sqrt(2.0f * abs_dp / (rho_air * psi_term));
```

- `psi` = 0 -> psi_term = 1 -> identical to Phase 3.1
- `psi` > 0 -> reduces flow, adds confinement character
- Need `S_reed` (reed channel reference area) as derived parameter from bore diameter

### 2. Breath Noise Generator (new: BreathNoise.h)

- White noise source (simple PRNG or juce::Random)
- Bandpass filter: second-order BPF at ~2000 Hz center, Q~1.5 (covers 500-6000 Hz)
- Output amplitude = `airNoise * |u_reed_prev|` (flow-dependent)
- Injected: `p_mouth_noisy = p_mouth + noiseGain * filteredNoise`
- Per-voice instance (independent noise per voice in poly mode)

### 3. Mouthpiece Volume / Chamber (new: MouthpieceChamber.h or inline in voice)

Acoustic model:
```
V_m = mouthpiece volume (m^3)
L_m = mouthpiece length (m)
A_m = mouthpiece cross-section area (m^2)

Compliance: C_m = V_m / (rho * c^2)
Mass: M_m = rho * L_m / A_m

State: p_chamber, u_chamber (pressure and flow in chamber)

dp_chamber/dt = (u_reed - u_bore) / C_m
du_bore/dt = (p_chamber - p_bore_minus) / M_m
```

- MOUTHPIECE_VOL 0-1 maps to V_m (0 = bypass, small effect at low values, noticeable sub-resonance at high)
- At MOUTHPIECE_VOL = 0: bypass the chamber entirely (p_bore_plus = reed output directly)
- Insertion point: between ReedModel output and BoreWaveguide input

### 4. Parameter Wiring

New params active in renderNextBlock:
- `pDoubleReed->load()` -> Psi value for ReedModel
- `pAirNoise->load()` -> noise mix level for BreathNoise
- `pMouthpieceVol->load()` -> chamber volume for MouthpieceChamber

## Parameters Active (Phase 3.2 cumulative)

| Phase | Parameters |
|-------|-----------|
| 3.1 (existing) | BREATH_PRESSURE, EMBOUCHURE, REED_HARDNESS, REED_OPENING, REED_MASS, REED_DAMPING, BORE_CHARACTER, BELL_SIZE, BORE_DIAMETER, BORE_LENGTH, OUTPUT_GAIN |
| 3.2 (new) | DOUBLE_REED (Psi), AIR_NOISE, MOUTHPIECE_VOL |
| Total active | 14 |

## Files to Modify/Create

| File | Action | Change |
|------|--------|--------|
| Source/DSP/ReedModel.h | Modify | Add Psi param to ReedParams, add Psi confinement term to Bernoulli flow, add S_reed derivation |
| Source/DSP/BreathNoise.h | Create | New header-only class: PRNG + BPF + flow-dependent amplitude |
| Source/DSP/MouthpieceChamber.h | Create | New header-only class: compliance + mass two-state model |
| Source/ReedWindVoice.h | Modify | Add BreathNoise and MouthpieceChamber members |
| Source/ReedWindVoice.cpp | Modify | Wire 3 new params, insert noise and chamber into sample loop |

## Test Criteria (from ROADMAP + additions)

- [ ] DOUBLE_REED (Psi) at 0: identical to Phase 3.1 output (regression)
- [ ] DOUBLE_REED at 0.4: audibly different — nasal, oboe-like character
- [ ] DOUBLE_REED at 0.7+: piercing, zurna/shehnai character
- [ ] Psi stable across full 0-1 range (no blowups)
- [ ] AIR_NOISE at 0: no noise added (identical to Phase 3.1)
- [ ] AIR_NOISE at moderate: audible breathiness, scales with playing intensity
- [ ] AIR_NOISE at high: noisy/airy without dominating tone
- [ ] Noise quiet during silence, loud during fortissimo (flow-dependent)
- [ ] MOUTHPIECE_VOL at 0: bypass, identical to Phase 3.1
- [ ] MOUTHPIECE_VOL at moderate: subtle pitch lowering and sub-resonance coloring
- [ ] MOUTHPIECE_VOL at high: noticeable Helmholtz effect
- [ ] All three new features interact correctly (Psi + noise + chamber simultaneously)
- [ ] Reed model stable across full combined parameter range
- [ ] No clicks during parameter changes
- [ ] VST3 + AU build zero errors
- [ ] auval passes

## Open Questions

None. All decisions resolved.

## Next Phase

Ready for: research phase
