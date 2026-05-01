# O-Contrabass - Parameter Specification (Draft)

## Overview

Specialized 4-string contrabass physical model (digital waveguide + nonlinear bow-string friction junction) purpose-built for sustained orchestral arco and ambient drone. Bass-only DSP (E1–G3 fundamental range) with first-class drone features (infinite sustain, sub-harmonics, slow bow LFO), bass-tuned wood body resonator, and full Ouaricon microtonal convention.

Source: extracted from `BRIEF.md` (2026-04-25). Refine into full `parameter-spec.md` after UI mockup.

## Parameters

### Primary Controls (Tier 1)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| BOW_SPEED | Bow Speed | Float | 0.02 - 1.5 | 0.15 | m/s | Bow velocity (v_b) — main loudness/dynamics; slower default for bass. CC11 / Expression. |
| BOW_PRESSURE | Bow Pressure | Float | 0.05 - 8.0 | 1.0 | N | Normal bow force (F_bow) — tone quality, attack character; higher default for thick strings. CC2 / Breath / Aftertouch. |
| BOW_POSITION | Bow Position (beta) | Float | 0.02 - 0.25 | 0.10 | - | Contact point — sul ponticello (low) to sul tasto (high); closer to bridge for fundamental richness. CC74 / MPE Y. |
| BRIGHTNESS | Brightness | Float | 80.0 - 12000.0 | 4500.0 | Hz | Bridge filter cutoff — overall tonal balance. |
| OUTPUT_GAIN | Output Level | Float | -60.0 - 12.0 | 0.0 | dB | Master output level. |

### Secondary Controls (Tier 2)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| ROSIN | Rosin | Float | 0.0 - 1.0 | 0.65 | - | Friction curve grip (mu_s/mu_d ratio + v_0); higher default for thick rosined bass hair. |
| BOW_NOISE | Bow Noise | Float | 0.0 - 1.0 | 0.35 | - | Bow hair / contact noise level for intimate close-mic character. |
| BODY_SIZE | Body Size | Float | 0.0 - 1.0 | 0.75 | - | Resonant frequency scaling within bass range (small bass to full 7/8). |
| BODY_DAMPING | Body Damping | Float | 0.0 - 1.0 | 0.40 | - | Body mode decay — low for sustain, high for tighter attack. |
| BODY_MIX | Body Mix | Float | 0.0 - 1.0 | 0.80 | - | Wet/dry of body resonator vs raw string. |

### String Configuration (Tier 3)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| STRING_TENSION | String Tension | Float | 0.0 - 1.0 | 0.50 | - | Affects timbre and stiffness (thick bass strings). |
| STRING_STIFFNESS | String Stiffness | Float | 0.0 - 1.0 | 0.30 | - | Inharmonicity / dispersion — low for ideal string, higher for character. |
| ACTIVE_STRINGS | Active Strings | Int | 1 - 4 | 4 | - | Number of strings the bow can engage. |

### Per-String Detune (4-string EADG, scordatura / just-intonation drones)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| DETUNE_E | E String Detune | Float | -1200.0 - 1200.0 | 0.0 | cents | Independent pitch offset for E1 string. |
| DETUNE_A | A String Detune | Float | -1200.0 - 1200.0 | 0.0 | cents | Independent pitch offset for A1 string. |
| DETUNE_D | D String Detune | Float | -1200.0 - 1200.0 | 0.0 | cents | Independent pitch offset for D2 string. |
| DETUNE_G | G String Detune | Float | -1200.0 - 1200.0 | 0.0 | cents | Independent pitch offset for G2 string. |

### Expression

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| VIBRATO_RATE | Vibrato Rate | Float | 0.1 - 12.0 | 5.0 | Hz | Dedicated vibrato section frequency (Mick 2025 bass mean: 5.17 Hz). |
| VIBRATO_DEPTH | Vibrato Depth | Float | 0.0 - 50.0 | 0.0 | cents | Vibrato pitch deviation. (Phase 2.3 Stage-1 contract amendment: default flipped 12.0 → 0.0 to preserve Phase 2.2 strict byte-equal regression bar — HR-1 short-circuit; mirrors EXPRESSION_MACRO Q7a precedent. User raises knob for vibrato character.) |
| VIBRATO_ONSET | Vibrato Onset | Float | 0.0 - 3000.0 | 600.0 | ms | Delay before vibrato fades in (orchestral realism). |
| SLOW_LFO_RATE | Slow Bow LFO Rate | Float | 0.05 - 2.0 | 0.3 | Hz | Drone modulation — slow swell rate. |
| SLOW_LFO_DEPTH | Slow Bow LFO Depth | Float | 0.0 - 1.0 | 0.0 | - | Depth — modulates bow speed/pressure for evolving drones (Schelleng-aware). |
| EXPRESSION_MACRO | Expression Macro | Float | 0.0 - 1.0 | 0.0 | - | Single-knob layered expression — bow speed + pressure + vibrato + body brightness. (Phase 2.3 Q7a Stage-1 contract amendment: default flipped 0.50 → 0.0 to preserve Phase 2.2 strict byte-equal regression bar; architecture-spec'd 0.50 default would, once macro DSP lands, produce a non-zero brightness offset and bow-param multiplier at rest.) |

### Drone Features

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| INFINITE_SUSTAIN | Infinite Sustain | Float | 0.0 - 1.0 | 0.0 | - | Reduces damping toward zero — endless resonance. Loop gain capped at 0.99995 for stability. |
| SUB_HARMONICS | Sub-Harmonics | Float | 0.0 - 1.0 | 0.0 | - | Friction-junction operating-point bias toward period-doubling regime — musical bass extension. |

### Output

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| WIDTH | Width | Float | 0.0 - 2.0 | 1.0 | - | Stereo spread (0 = mono, 2 = 200% wide). |

### Output Chain (Phase 2.6a additions)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| MASTER_SAT_AMOUNT | Master Saturator | Float | 0.0 - 1.0 | 0.50 | - | Wet/dry mix of polynomial x − x³/3 saturator (Phase 2.6a). Soft-clip at ~−3 dBFS. Default 50%. |
| LIMITER_CEILING_DB | Limiter Ceiling | Float | -6.0 - 0.0 | -0.3 | dB | Zero-latency feedforward limiter ceiling (Phase 2.6a). 3 ms attack / 50 ms release per CONTEXT rev-11 Q4. Default -0.3 dBFS. |

### Microtonal Tuning (Ouaricon Convention)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| REFERENCE_PITCH | Reference Pitch | Float | 220.0 - 880.0 | 440.0 | Hz | A4 reference frequency. |
| TUNING_SYSTEM | Tuning System | Choice | 0 - 2 | 2 | - | Scala/TUN / MTS-ESP / 12-TET. |
| NOTE_EXPRESSION | Note Expression | Bool | false - true | true | - | VST3 Note Expression for Dorico microtonal playback (spike-validated pattern). |

## MIDI / MPE Mapping

| Channel | Target Parameter |
|---------|------------------|
| CC11 (Expression) | BOW_SPEED |
| CC2 (Breath) / Channel Aftertouch | BOW_PRESSURE |
| CC74 / MPE Slide (Y) | BOW_POSITION |
| MPE Pitch Bend (X) | Per-note pitch (combined with Note Expression) |
| MPE Pressure (Z) | BOW_PRESSURE (per-note) |
| Note Velocity | Initial bow attack character |

## Parameter Count Summary

- Tier 1 (Primary): 5
- Tier 2 (Secondary): 5
- Tier 3 (String Configuration): 3
- Per-String Detune: 4
- Expression: 6
- Drone Features: 2
- Output: 1
- Output Chain (Phase 2.6a): 2
- Microtonal: 3
- **Total: 31**

## Design Notes

- **Monophonic** (one bow, one string at a time — authentic single-string playing).
- **Sustained-first articulation:** bow held while MIDI note held, release tail on note-off. CC11 controls dynamic shape over the held note.
- **Bass-tuned defaults** throughout: slower bow speed (0.15 vs violin's ~0.3), higher pressure (1.0 N vs ~0.5), more rosin grip (0.65), heavier bow noise (0.35), beta closer to bridge (0.10).
- **Drone defaults to off:** Infinite Sustain and Sub-Harmonics start at 0.0; preset banks engage them.
- **No sympathetic strings** in v1.0 (deferred to v1.1+ per BRIEF out-of-scope list).
- **Material is fixed wood** (no morphable material — that's O-Bowed's territory; O-Contrabass is deeply specialized).

## Source

Extracted from `BRIEF.md` on 2026-04-26 to unblock Stage 0 planning. Will be superseded by full `parameter-spec.md` after UI mockup phase.

## Audit Trail

### Stage 1 → Phase 2.6a (parameter-spec contract amendments)

- Phase 2.3 R28 (2026-04-29): VIBRATO_DEPTH default flipped 12.0 → 0.0 (HR-1 short-circuit; Phase 2.2 strict byte-equal regression bar). EXPRESSION_MACRO default flipped 0.50 → 0.0 (Q7a). Sha bump deferred (informally tracked in this section); next sha-bump at Phase 2.6a R39d.
- Phase 2.6a R39d (2026-05-XX): NEW MASTER_SAT_AMOUNT + LIMITER_CEILING_DB per CONTEXT rev-11 §"Phase 2.6a — Output chain" + Q4 LOCKED limiter ceiling. Total parameter count 29 → 31.
