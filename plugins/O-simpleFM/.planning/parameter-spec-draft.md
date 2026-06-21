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
| Ratio (C:M) | `ratio` | float | 0.5–16 | 1.0 | × | Modulator freq = carrier × ratio (ratio mode). Integer = harmonic, non-integer = inharmonic. THE defining FM control. Optional integer-snap. |
| Ratio Snap | `ratioSnap` | bool | off/on | off | — | Integer-snap toggle for clean harmonic vs inharmonic comparison (DSP-05). |
| Modulator Fixed Mode | `modFixedMode` | bool | Ratio/Fixed | Ratio | — | When Fixed, modulator runs at `modFixedHz` (does NOT track pitch) — teaches inharmonic/clangorous timbres. |
| Modulator Fixed Hz | `modFixedHz` | float | 1–8000 | 220 | Hz | Fixed modulator frequency (log skew); active only when `modFixedMode` = Fixed. |
| Modulation Index | `modIndex` | float | 0–20 | 0 | index | Depth of phase/freq modulation; controls sideband count / brightness. |
| Mod Env → Index | `modEnvToIndex` | float | 0–100 | 100 | % | Multiplicative depth of mod-envelope effect on index (headline feature). |
| Velocity → Index | `velToIndex` | float | 0–100 | 0 | % | Opt-in velocity-to-index amount (DSP-06). |
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

## Output

| Param | ID | Type | Range | Default | Notes |
|-------|----|------|-------|---------|-------|
| Output Level | `outputLevel` | float | -inf–0 dB | 0 | Master output gain. |

> **Operators are sine-only in v1.0** — no carrier/modulator waveform selectors. Sine keeps the FM math clean for teaching and removes the heaviest anti-aliasing cost. Non-sine operators (DSP-04) are deferred to v1.1.

## Resolved Decisions (Stage 0 — 2026-06-20)

1. **Modulation index semantics** — raw radian index `I` (Bessel argument), 0–20, perceptual taper (`I = 20·norm^1.7`), displayed linearly. Optional read-only `Δf = I·f_m` Hz readout. (Most pedagogically transparent; carrier-null at I≈2.405 is the marquee teaching annotation.)
2. **Ratio control** — continuous, with optional integer-snap (`ratioSnap`) applied at read site.
3. **Polyphony** — 16 voices; lifetime gated on amp-envelope activity only.
4. **Index range ceiling** — 20 max; key-tracked Carson's-rule ceiling caps effective index per note to prevent aliasing.
5. **Modulator frequency mode** — **ADOPTED in v1.0**: `modFixedMode` toggle + `modFixedHz` (Ratio vs Fixed).
6. **Operator waveforms** — **sine-only in v1.0**; non-sine (DSP-04) deferred to v1.1.

## Deferred to v1.1

- Non-sine operator waveforms (`carWave`/`modWave`, DSP-04)
- Fine detune (`fineCents`)
- Master tuning (`masterTune`)
- Optional LFO / vibrato

---
*Draft generated from BRIEF.md on 2026-06-20; updated with Stage 0 scope decisions. Replace with full parameter-spec.md at mockup finalization before Stage 1.*
