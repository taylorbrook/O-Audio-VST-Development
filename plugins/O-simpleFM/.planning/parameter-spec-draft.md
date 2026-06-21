# O-simpleFM — Parameter Specification (DRAFT)

---
version: 0.1.0-draft
plugin: O-simpleFM
created: 2026-06-20
source: BRIEF.md parameter table
status: draft (full parameter-spec.md required before Stage 1 — produced by mockup finalization)
---

> **DRAFT** — extracted from BRIEF.md for Stage 0 complexity/architecture planning.
> Ranges are starting proposals to be validated by research. Items marked *(research)*
> are likely additions Stage 0 should confirm and fold into the final spec.

## Core FM Controls

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Ratio (C:M) | `ratio` | float | 0.5–16 | 1.0 | × | Modulator freq = carrier × ratio. Integer = harmonic, non-integer = inharmonic. THE defining FM control. Optional integer-snap. |
| Modulation Index | `modIndex` | float | 0–20 | 0 | index | Depth of phase/freq modulation; controls sideband count / brightness. |
| Feedback | `feedback` | float | 0–100 | 0 | % | Modulator self-feedback; enriches/roughens spectrum toward saw/noise. |

## Modulator Envelope (ADSR → index)

| Param | ID | Type | Range | Default | Unit |
|-------|----|------|-------|---------|------|
| Mod Attack | `modAttack` | float | 0–5 | 0.005 | s |
| Mod Decay | `modDecay` | float | 0–5 | 0.3 | s |
| Mod Sustain | `modSustain` | float | 0–100 | 80 | % |
| Mod Release | `modRelease` | float | 0–5 | 0.1 | s |

## Amplitude Envelope (ADSR → carrier output)

| Param | ID | Type | Range | Default | Unit |
|-------|----|------|-------|---------|------|
| Amp Attack | `ampAttack` | float | 0–5 | 0.005 | s |
| Amp Decay | `ampDecay` | float | 0–5 | 0.3 | s |
| Amp Sustain | `ampSustain` | float | 0–100 | 80 | % |
| Amp Release | `ampRelease` | float | 0–5 | 0.1 | s |

## Oscillator / Output

| Param | ID | Type | Range | Default | Notes |
|-------|----|------|-------|---------|-------|
| Carrier Waveform | `carrierWave` | choice | sine / tri / saw / square | sine | Sine default keeps FM math clean for teaching. |
| Modulator Waveform | `modWave` | choice | sine / tri / saw / square | sine | Sine default. |
| Output Level | `outputLevel` | float | -inf–0 | 0 | dB | Master output gain. |

## Likely Additions — confirm in Stage 0 research *(research)*

- `modEnvToIndex` — mod-envelope → index amount (depth of envelope's effect on index)
- `velToIndex` — velocity → modulation index amount
- Modulator frequency mode — fixed-frequency (Hz) vs ratio-locked
- Fine detune (modulator and/or carrier)
- Key tracking of index
- Master tuning
- Polyphony / voice count (proposed 8–16)
- Optional LFO / vibrato

## Open Decisions for Research

1. **Modulation index semantics** — expose as raw index, modulator output level, or Hz deviation? Pick the most pedagogically transparent and label clearly.
2. **Ratio control** — continuous with optional integer-snap (compare harmonic vs inharmonic cleanly).
3. **Polyphony** — confirm voice count (8–16 fine for a teaching tool).
4. **Index range ceiling** — confirm ~20 is the right max for the teaching sweep without excessive aliasing.

---
*Draft generated from BRIEF.md on 2026-06-20. Replace with full parameter-spec.md at mockup finalization before Stage 1.*
