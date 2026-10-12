# O-simpleAmbisonics — Parameter Specification (DRAFT)

---
version: 0.1.0-draft
plugin: O-simpleAmbisonics
created: 2026-10-11
source: BRIEF.md parameter table
status: draft (full parameter-spec.md required before Stage 1 — produced by mockup finalization)
---

> **DRAFT** — extracted from BRIEF.md for Stage 0 complexity/architecture planning.
> Ranges are starting proposals to be validated by research. Items marked *(research)*
> are likely additions Stage 0 should confirm and fold into the final spec.

## Source (position / distance / motion)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Azimuth | `azimuth` | float | -180 to +180 | 0 | ° | Horizontal source direction. 0° front, +90° left (AmbiX), ±180° behind. THE primary control. Wraps — smoothing must take the shortest path around ±180°. |
| Elevation | `elevation` | float | -90 to +90 | 0 | ° | Vertical source direction. +90° overhead, -90° below. |
| Distance | `distance` | float | 0.2–20 | 1 | m (log) | Inverse-distance gain with near-field clamp plus gentle one-pole air-absorption LP that opens with proximity. |
| Orbit | `orbit` | bool | Off / On | Off | — | Automatic azimuth motion so movement is audible without DAW automation. |
| Orbit Rate | `orbitRate` | float | 0.02–2 | 0.1 | Hz (log) | One full circle per period. |
| Input Mode | `inputMode` | choice | Mono sum / Left only | Mono sum | — | Stereo input summed to the single mono source, or L taken alone. Mono input accepted directly. |

## Encoder

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Order | `order` | choice | 1 / 2 / 3 | 1 | — | Ambisonic order → 4 / 9 / 16 channels (ACN / SN3D, AmbiX). Starts at 1 so the step-up is the lesson. Always pre-allocate 16 channels; lower orders zero the unused ones. Order change = decoder filter-bank swap with crossfade (click-free). |

## Rotation (SH domain, applied after encoding)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Yaw | `yaw` | float | -180 to +180 | 0 | ° | Scene rotation about the vertical axis — spins the whole soundfield. Headline "rotate the room" control. Wraps. |
| Pitch | `pitch` | float | -180 to +180 | 0 | ° | Scene rotation about the left–right axis (tilt). Wraps. |
| Roll | `roll` | float | -180 to +180 | 0 | ° | Scene rotation about the front–back axis. Wraps. |

## Decoder / Output

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Decoder | `decoder` | choice | Binaural / Stereo | Binaural | — | Binaural = HRTF filter bank (embedded precomputed filters, partitioned convolution). Stereo = plain amplitude decode to a ±30° virtual pair for speakers / projector. Both modes must report equal latency. Switch must be click-free. |
| Output Level | `outputLevel` | float | -inf–0 | 0 | dB | Master output gain. |

## Open Research Items (Stage 0 to confirm / fold into final spec)

- **Source width / spread** *(research)* — point → diffuse; classic lesson but non-trivial DSP. Candidate v1.1 per BRIEF Out of Scope; confirm or defer.
- **Distance model details** *(research)* — near-field clamp value, air-absorption cutoff mapping, whether to expose a `distanceModel` on/off for before/after comparison.
- **Decoder weighting** *(research)* — basic / max-rE / in-phase. Classic lesson, possibly too deep for v1.0.
- **HRTF dataset** *(research)* — KU100 vs SADIE II; whether to offer more than one head. Offline precompute → embedded binary data; no runtime SOFA parsing.
- **Binaural decoder method** *(research)* — MagLS vs virtual-loudspeaker + HRTF; per-order filter sets (1/2/3).
- **Rotation matrix construction** *(research)* — recursive (Ivanic–Ruedenberg) vs Euler Z-Y-Z for orders 2–3; cost at order 3.
- **Head-locked vs scene-locked listener** *(research)* — for the Stereo decode.
- **B-format channel solo / audition** *(research)* — audition an individual ambisonic channel in isolation; pedagogical, possibly a UI-only mute matrix.
- **Convolution engine sizing** *(research)* — partition size vs max host block (oversampler-overrun pattern); CPU budget at order 3 / 64-sample buffers; latency via `setLatencySamples()` in `prepareToPlay()`.

## Deferred to v1.1+ (per BRIEF "Out of Scope")

- B-format / multichannel output and loudspeaker decoding to physical rigs
- Multiple sources / scene bus
- Head tracking, custom SOFA loading, HRTF personalisation
- Room / reverb / early reflections
- Orders above 3

---
*Draft generated from BRIEF.md on 2026-10-11. Replace with full parameter-spec.md at mockup finalization before Stage 1.*
