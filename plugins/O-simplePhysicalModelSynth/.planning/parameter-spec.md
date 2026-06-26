# O-simplePhysicalModelSynth — Parameter Specification

---
version: 1.0.0
plugin: O-simplePhysicalModelSynth
created: 2026-06-26
source: parameter-spec-draft.md (promoted) + Stage-0 resolved decisions (research/ARCHITECTURE.md, stages/0-ideation/CONTEXT.md)
status: FINAL — zero-drift contract for Stage 1 APVTS
param_count: 17
---

> **FINAL** — This is the binding zero-drift contract. The 17 parameter IDs, types,
> ranges, and defaults below are locked. Stage 1 (Foundation) implements this APVTS
> exactly; Stages 2–4 must not drift from these IDs. The `(research)` markers from the
> draft are resolved (see **Resolved DSP Mappings** at the bottom) and folded in here.

## Parameter Summary

| # | Param | ID | Type | Range | Default | Unit |
|---|-------|----|------|-------|---------|------|
| 1 | Excitation Type | `excitationType` | choice | Pluck / Strike / Bow | Pluck (0) | — |
| 2 | Excitation Position | `excitationPosition` | float | 0–100 | 25 | % |
| 3 | Excitation Color | `excitationColor` | float | 0–100 | 60 | % |
| 4 | Bow Force | `bowForce` | float | 0–100 | 50 | % |
| 5 | Resonator Type | `resonatorType` | choice | String / Modal | String (0) | — |
| 6 | String Model | `stringModel` | choice | Karplus-Strong / Waveguide | Karplus-Strong (0) | — |
| 7 | Inharmonicity | `inharmonicity` | float | 0–100 | 0 | % |
| 8 | Mode Brightness | `modeBrightness` | float | 0–100 | 50 | % |
| 9 | Damping | `damping` | float | 0–100 | 60 | % |
| 10 | Decay | `decay` | float | 0–100 | 70 | % |
| 11 | Material | `material` | float | 0–100 | 30 | % |
| 12 | Coarse Tune | `coarseTune` | int | -24–+24 | 0 | st |
| 13 | Fine Tune | `fineTune` | float | -100–+100 | 0 | cents |
| 14 | Amp Attack | `ampAttack` | float | 0–2 | 0.001 | s |
| 15 | Amp Release | `ampRelease` | float | 0–5 | 0.2 | s |
| 16 | Velocity → Brightness | `velToBrightness` | float | 0–100 | 60 | % |
| 17 | Output Level | `outputLevel` | float | -60–0 | -6 | dB |

---

## Excitation (how energy is injected)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Excitation Type | `excitationType` | choice | Pluck / Strike / Bow | Pluck | — | How energy enters. Pluck = broadband noise burst (canonical KS); Strike = band-limited raised-cosine impulse/mallet; Bow = sustained stick-slip friction. THE "swap the exciter" control (FUNC-02). |
| Excitation Position | `excitationPosition` | float | 0–100 | 25 | % | Where along the string energy enters (comb on the exciter in KS v1.0; true pickup point if Waveguide ships). |
| Excitation Color | `excitationColor` | float | 0–100 | 60 | % | Brightness/hardness of the exciter (noise-burst / impulse low-pass; soft mallet ↔ hard). |
| Bow Force | `bowForce` | float | 0–100 | 50 | % | (Bow only) friction pressure driving the memoryless STK stick-slip — affects attack noise and harmonic richness. Single Bow control in v1.0 (Bow Speed deferred to v1.1). |

## Resonator (what carries the energy)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Resonator Type | `resonatorType` | choice | String / Modal | String | — | THE engine switch. String = Karplus-Strong (harmonic); Modal = parallel resonant biquad bank (inharmonic bars/bells), driven by the same exciter (FUNC-03/FUNC-04). |
| String Model | `stringModel` | choice | Karplus-Strong / Waveguide | Karplus-Strong | — | (String only) single-delay KS (v1.0 must) vs dual-delay waveguide (`nice`/Stage-2.4, deferrable without contract break). Waveguide enables true Excitation Position + traveling-wave view. |
| Inharmonicity | `inharmonicity` | float | 0–100 | 0 | % | (Modal only) stretches mode spacing `f_k = f0·k·√(1+B·k²)` from harmonic bar (low B) toward inharmonic bell (high B). The defining modal control (DSP-05). |
| Mode Brightness | `modeBrightness` | float | 0–100 | 50 | % | (Modal only) upper-mode amplitude/decay tilt — how bright/metallic the struck body is. |

## Material / Damping (how energy is lost)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Damping | `damping` | float | 0–100 | 60 | % | Loop low-pass cutoff — removes HF energy each pass; tone darkens as it decays. Bright steel ↔ muted nylon (DSP-03). 0%→darkest (~2 kHz), 100%→brightest (~10 kHz). |
| Decay | `decay` | float | 0–100 | 70 | % | Feedback gain / ring time. Maps to loop feedback **clamped < 1** for stability (DSP-03, QUAL-01). 0%→short (~0.93), 100%→long (~0.995). |
| Material | `material` | float | 0–100 | 30 | % | One-knob macro co-moving Damping + Decay for the single-gesture steel↔nylon sweep (DSP-07). Writes both Damping and Decay so the two knobs visibly track. |

## Tuning

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Coarse Tune | `coarseTune` | int | -24–+24 | 0 | st | Transpose in semitones. |
| Fine Tune | `fineTune` | float | -100–+100 | 0 | cents | Fine pitch. |

> Pitch comes from the played MIDI note via delay length (SR ÷ N) or modal mode set,
> tuned with a first-order Thiran all-pass fractional-delay interpolator + loop-filter
> group-delay compensation (DSP-02) so high notes stay in tune.

## Amp + Dynamics

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Amp Attack | `ampAttack` | float | 0–2 | 0.001 | s | Output amplitude attack (mostly Bow / shaping; the body's decay is intrinsic to the model). |
| Amp Release | `ampRelease` | float | 0–5 | 0.2 | s | Output amplitude release / note-off damping. |
| Velocity → Brightness | `velToBrightness` | float | 0–100 | 60 | % | How much harder playing brightens/strengthens the excitation — the model's dynamic response (FUNC-06). |

## Output

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Output Level | `outputLevel` | float | -60–0 | -6 | dB | Master output gain. Implemented as a -60..0 dB gain (the draft's "-inf" floor realized as -60 dB). |

---

## Implementation Notes (Stage 1 APVTS)

- **Param-ID namespace:** Define IDs in a `ParamIDs` namespace (O-simpleFM pattern). No bare
  `end`/`begin` symbols (none here). Use plain string IDs exactly as listed above.
- **Choice params:** `excitationType` {Pluck, Strike, Bow}, `resonatorType` {String, Modal},
  `stringModel` {Karplus-Strong, Waveguide} via `AudioParameterChoice`. Default index 0 for each.
- **Int param:** `coarseTune` via `AudioParameterInt` (-24..24).
- **Float params:** `AudioParameterFloat` with `NormalisableRange`. Percent params use a plain
  0–100 linear range. `outputLevel` -60..0 dB linear in dB.
- **Version hint:** start with `ParameterID{ id, 1 }` (version 1) for forward-compat.
- **Stage 1 scope:** APVTS + state save/load + silent 16-voice `juce::Synthesiser` shell only.
  No DSP yet — params exist and persist but drive nothing until Stage 2.

## Resolved DSP Mappings (from Stage 0 — for Stage 2, not Stage 1)

These were the draft's open `(research)` items, resolved in `stages/0-ideation/CONTEXT.md`.
They are recorded here so the final param ranges are understood, but they are **DSP-side
mappings implemented in Stage 2** — Stage 1 only exposes the raw 0–100 / dB / semitone params.

1. **Fractional-delay tuning** — first-order all-pass via `dsp::DelayLine<float, Thiran>` +
   loop-filter group-delay compensation (O-Lyrica / O-Bowed).
2. **Modal modes** — N=8; `f_k = f0·k·√(1+B·k²)` (B from `inharmonicity`); per-mode decay from
   O-Bells `DECAY_MULTIPLIERS`. Bar ≈ low B, bell ≈ high B.
3. **Material macro curve** — steel↔nylon: log-frequency cutoff lerp (~10 kHz → ~2 kHz) +
   linear feedback lerp (0.995 → 0.93); macro writes both Damping and Decay.
4. **Excitation design** — Pluck = filtered noise burst; Strike = raised-cosine band-limited
   impulse (no DC click); Bow = memoryless STK friction (sustains). DC-blocker on the sum.
5. **Waveguide vs KS** — KS = v1.0 must; Position via exciter comb; Waveguide = `nice`/deferrable.
6. **Polyphony** — 16 voices (`juce::Synthesiser`).
7. **Viz tap** — single global lead-voice tap; lock-free ring + atomics; no per-voice viz.
8. **Bow 2nd control** — none in v1.0 (`bowForce` only).

## Deferred to v1.1+ (per BRIEF Out of Scope)

- Blow / tube resonator (winds, brass)
- Sculpture-level component modeling (movable pickups, multi-string)
- Sympathetic / coupled strings, body convolution
- Built-in effects (reverb/delay/chorus)
- Deep modulation matrix / LFOs beyond velocity→brightness
- Bow Speed control

---
*Promoted from parameter-spec-draft.md on 2026-06-26 with Stage-0 resolved decisions folded in. This is the FINAL zero-drift contract for Stage 1.*
