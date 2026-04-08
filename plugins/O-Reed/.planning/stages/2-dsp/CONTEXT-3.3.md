# Stage 2: DSP Phase 3.3 - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude
**Scope:** Tone Holes + Expression + Legato (full Keefe, all components, bore-preserving legato)

## Phase 3.3 Goal

Add Keefe three-port scattering tone holes to the bore waveguide, register hole for overblowing, vibrato/growl/flutter/subtone expression modifiers, and bore-preserving mono legato. This phase transforms the raw reed-bore engine into a playable instrument with spectral shaping, expression, and voice management.

## Requirements Confirmed

### Tone Hole Lattice (Full Keefe)
- **4 virtual tone holes** as Keefe three-port scattering junctions inserted into the bore waveguide
- BoreWaveguide must be restructured from 2 delay lines (1 forward + 1 backward) into 5 segments (5 forward + 5 backward) with 4 junctions between them
- Each junction has continuous opening 0-1 controlled by TONE_HOLE_CUTOFF param
- TONE_HOLE_CUTOFF (200-8000 Hz) controls the spectral cutoff: maps to progressive hole openings from bell-end toward reed-end (higher cutoff = more holes closed = brighter)
- Hole opening distribution: lower cutoff opens more holes (darker, more radiation loss), higher cutoff closes more (brighter, less loss)
- Radiated output from open tone holes contributes to final output (mixed with bell radiation)
- When all holes closed (cutoff=8000), behavior identical to Phase 3.2 (regression)

### Register Hole
- **Single scattering junction** near reed end (between segment 0 and 1, ~10% from reed)
- REGISTER_HOLE (0-1) controls opening: 0=closed (normal), 1=fully open
- Cylindrical bore (boreCharacter~0): forces 3rd harmonic (overblows at 12th)
- Conical bore (boreCharacter~1): forces 2nd harmonic (overblows at octave)
- Behavior auto-adapts via bore geometry — no special logic needed, physics handles it

### Vibrato System
- **Sine LFO**, 3 modulation targets selected by VIBRATO_SOURCE choice param:
  - 0 (Lip): modulates embouchure — jaw vibrato, most common
  - 1 (Breath): modulates mouth pressure — diaphragm vibrato
  - 2 (Throat): modulates bore impedance (scale factor) — classical oboe style
- VIBRATO_DEPTH (0-1) controls amplitude, VIBRATO_RATE (1-10 Hz) controls frequency
- At VIBRATO_DEPTH=0: no modulation (regression)

### Growl Oscillator
- **Secondary sine oscillator** at ~120 Hz (male vocal fundamental)
- Modulates mouth pressure: `p_growl = p_mouth * (1 + growlAmount * sin(2*pi*120*t) * 0.3)`
- GROWL_AMOUNT (0-1) controls coupling strength
- Low: subtle beating. High: multiphonic sum/difference tones
- At GROWL_AMOUNT=0: no modulation (regression)

### Flutter Tongue
- **~25 Hz smoothed square LFO** modulating breath pressure
- FLUTTER_TONGUE (0-1) controls depth (0-40% pressure modulation)
- Smoothed square = sine passed through soft clip to get squarish but alias-free
- At FLUTTER_TONGUE=0: no modulation (regression)

### Subtone Mode
- **Parameter modifier** (not a separate DSP block)
- SUBTONE (0-1) simultaneously:
  - Increases lip damping: `g_eff *= (1 + subtone * 5.0)`
  - Reduces effective mouth pressure: `p_mouth *= (1 - subtone * 0.3)`
  - Increases breath noise: `airNoise_eff = airNoise + subtone * 0.3` (clamped to 1)
- Result: reed oscillates without beating, soft airy tone (saxophone subtone technique)
- At SUBTONE=0: no modification (regression)

### Mono Legato (Bore-Preserving)
- **POLY_MODE choice**: Mono (0) / Poly (1)
- In Mono mode with overlapping notes: DO NOT reset DSP on noteStarted()
  - Keep bore state (delay lines, filters, energy), breath state, reed state
  - Only retune bore.setFrequency(newFreq) — smooth pitch transition via existing bore frequency smoothing
  - Don't re-trigger breath envelope (legato = continuous air column)
- In Mono mode with gap: normal noteStarted() reset + breath attack
- Detection: voice checks if bore has active energy (isActive) when noteStarted() fires
- In Poly mode: normal independent voice behavior (up to MAX_VOICES)

### Voice Management
- **POLY_MODE** read in PluginProcessor or voice to control legato behavior
- **MAX_VOICES** — currently 16 voices always allocated. For mono mode, only 1 voice should sound. MPESynthesiser handles voice stealing, but mono legato needs custom logic in noteStarted().
- Polyphonic mode: existing behavior (independent voices, MPE voice allocation)

## Constraints

- All new features bypass at default values (0) — Phase 3.2 behavior preserved
- Tone hole restructure must not break existing bore tuning — total delay across all 5 segments must equal the single-pair delay from Phase 3.2
- Register hole must not cause instability — scattering coefficient bounded
- Expression modifiers are pre-reed modulations only — no changes inside the bore loop (except vibrato source=throat which modulates scale factor)
- Legato transitions must be click-free — bore frequency smoothing (~50ms) handles this
- No heap allocation in audio path

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Tone hole model | Full Keefe three-port scattering | User chose full Keefe over simplified spectral filter. Physically accurate, restructures bore into 5 segments |
| Phase scope | All components in one phase | Expression modifiers are trivial (~30 lines each), splitting adds overhead |
| Legato approach | Bore-preserving | Keep bore state on legato, just retune delay lines. Physically correct continuous air column |
| Bore segment split | 5 segments: 10%/20%/20%/25%/25% | Register hole near reed, 4 tone holes spread across middle-to-bell |
| Tone hole opening logic | TONE_HOLE_CUTOFF maps to progressive opening | Higher cutoff = more holes closed = brighter. Bell-end holes open first as cutoff lowers |
| Tone hole radiation | Mixed into output alongside bell radiation | Open holes radiate sound; contributes to overall output |
| Vibrato implementation | Per-sample sine LFO in voice | Trivial cost, modulates existing params |
| Growl frequency | Fixed ~120 Hz | Male vocal fundamental, good for all instrument ranges |
| Flutter waveform | Smoothed square (tanh-clipped sine) | Alias-free, squarish character matching real flutter tongue |
| Subtone implementation | Parameter modifier in renderNextBlock | Adjusts existing params before reed model, no new DSP class needed |
| Mono voice handling | Legato flag in noteStarted() based on bore energy | If bore.getEnergy() > threshold when new note starts = legato (don't reset) |

## Components to Implement

### 1. ToneHole (new: DSP/ToneHole.h)
- Keefe three-port scattering junction
- Members: `opening` (0-1), `Y_hole` (admittance), bore segment admittance
- `processSample(p_forward_in, p_backward_in)` -> returns `{p_forward_out, p_backward_out, p_radiated}`
- Scattering matrix depends on `opening * baseAdmittance` relative to bore admittance

### 2. BoreWaveguide restructure
- Replace single forward/backward delay pair with 5 segments
- Each segment: forward + backward Thiran delay
- Segment delay lengths sum to total bore delay
- Distribution: [10%, 20%, 20%, 25%, 25%] (register hole | hole1 | hole2 | hole3 | hole4 | bell)
- processSample() now iterates through segments with junctions between them
- Bell and viscothermal filters remain at the bell end (segment 4 output)
- Conical scaling applied per-segment (scaled by segment position)

### 3. Expression modifiers (inline in ReedWindVoice)
- Vibrato: `float vibMod = vibratoDepth * sin(2*pi*vibratoRate*t)` applied to lip/breath/throat per source
- Growl: `p_mouth *= (1 + growlAmount * sin(2*pi*120*t) * 0.3)` when growlAmount > 0
- Flutter: `p_mouth *= (1 - flutterTongue * 0.4 * tanhSquare(25Hz))` when flutterTongue > 0
- Subtone: modify embouchure_eff, p_mouth_eff, airNoise_eff before reed

### 4. Legato logic in ReedWindVoice
- In noteStarted(): check bore energy > threshold AND mono mode
- If legato: retune bore only, don't reset DSP
- If not legato: full reset (existing behavior)

## Parameters Active (Phase 3.3 cumulative)

| Phase | Parameters | Count |
|-------|-----------|-------|
| 3.1 (existing) | breathPressure, embouchure, reedHardness, reedOpening, reedMass, reedDamping, boreCharacter, bellSize, boreDiameter, boreLength, outputGain | 11 |
| 3.2 (existing) | airNoise, doubleReed, mouthpieceVol | 3 |
| 3.3 (new) | toneHoleCutoff, registerHole, vibratoDepth, vibratoRate, vibratoSource, growlAmount, flutterTongue, subtone, polyMode, maxVoices | 10 |
| **Total active** | | **24** |

Note: attackChiff and boreLength already active from Phase 3.1 (breathEnv.noteOn and bore.updateParams). Not counted as new.

## Files to Modify/Create

| File | Action | Change |
|------|--------|--------|
| Source/DSP/ToneHole.h | Create | Keefe three-port scattering junction (header-only) |
| Source/DSP/BoreWaveguide.h | Major modify | Restructure into 5 segments with tone hole junctions + register hole |
| Source/ReedWindVoice.h | Modify | Add vibrato/growl/flutter phase state, legato flag |
| Source/ReedWindVoice.cpp | Major modify | Wire 10 new params, expression modifiers, subtone, legato logic, tone hole params |
| Source/PluginProcessor.cpp | Minor modify | Poly mode handling (optional — may be voice-level only) |

## Test Criteria

### Tone Holes
- [ ] TONE_HOLE_CUTOFF at 8000 Hz (all closed): identical to Phase 3.2
- [ ] TONE_HOLE_CUTOFF sweep 8000->200: progressive spectral darkening
- [ ] TONE_HOLE_CUTOFF at low values: audible radiation from holes, darker tone
- [ ] Register hole at 0: normal fundamental
- [ ] Register hole at 1 + cylindrical bore: jumps to 12th (3rd harmonic)
- [ ] Register hole at 1 + conical bore: jumps to octave (2nd harmonic)

### Expression
- [ ] Vibrato lip: audible pitch+brightness wobble at set rate
- [ ] Vibrato breath: audible dynamics wobble
- [ ] Vibrato throat: audible timbral wobble
- [ ] All vibrato at depth=0: no effect
- [ ] Growl low: subtle beating
- [ ] Growl high: multiphonic texture
- [ ] Flutter tongue: ~25 Hz pressure oscillation audible
- [ ] Subtone: soft, airy, no reed beating
- [ ] All expression at 0: no effect (regression)

### Voice Management
- [ ] Mono legato: overlapping notes produce smooth pitch transition (no re-attack)
- [ ] Mono with gap: normal attack on new note
- [ ] Poly mode: multiple notes play simultaneously
- [ ] No clicks during legato transitions

### General
- [ ] VST3 + AU build zero errors
- [ ] auval passes
- [ ] No clicks during any parameter changes
- [ ] CPU acceptable with tone holes active

## Open Questions

None. All decisions resolved.

## Next Phase

Ready for: research phase
