# O-simplePhysicalModelSynth — Parameter Specification (DRAFT)

---
version: 0.1.0-draft
plugin: O-simplePhysicalModelSynth
created: 2026-06-26
source: BRIEF.md parameter table
status: draft (full parameter-spec.md required before Stage 1 — produced by mockup finalization)
---

> **DRAFT** — extracted from BRIEF.md for Stage 0 complexity/architecture planning.
> Ranges are starting proposals to be validated by research. Items marked *(research)*
> are open questions Stage 0 should confirm and fold into the final spec.

## Excitation (how energy is injected)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Excitation Type | `excitationType` | choice | Pluck / Strike / Bow | Pluck | — | How energy enters. Pluck = broadband noise burst (canonical KS); Strike = filtered impulse/mallet; Bow = sustained stick-slip friction. THE "swap the exciter" control (FUNC-02). |
| Excitation Position | `excitationPosition` | float | 0–100 | 25 | % | Where along the string energy enters (comb / waveguide pickup point). Most meaningful on the waveguide string. |
| Excitation Color | `excitationColor` | float | 0–100 | 60 | % | Brightness/hardness of the exciter (noise-burst / impulse low-pass; soft mallet ↔ hard). |
| Bow Force | `bowForce` | float | 0–100 | 50 | % | (Bow only) friction pressure driving the stick-slip — affects attack noise and harmonic richness. *(research: whether Bow needs a 2nd control — speed vs force.)* |

## Resonator (what carries the energy)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Resonator Type | `resonatorType` | choice | String / Modal | String | — | THE engine switch. String = Karplus-Strong/waveguide (harmonic); Modal = decaying sinusoids (inharmonic bars/bells) (FUNC-03). |
| String Model | `stringModel` | choice | Karplus-Strong / Waveguide | Karplus-Strong | — | (String only) single-delay KS vs dual-delay waveguide. Waveguide enables Excitation Position + traveling-wave view (DSP-06, `nice`). *(research: confirm waveguide scope for v1.0.)* |
| Inharmonicity | `inharmonicity` | float | 0–100 | 0 | % | (Modal only) stretches mode spacing from harmonic (bar) toward inharmonic (bell). The defining modal control (DSP-05). |
| Mode Brightness | `modeBrightness` | float | 0–100 | 50 | % | (Modal only) balance/decay of upper modes — how bright/metallic the struck body is. |

## Material / Damping (how energy is lost)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Damping | `damping` | float | 0–100 (dark ↔ bright) | 60 | % | Loop low-pass cutoff — removes a little HF energy each pass; tone darkens as it decays. Bright steel ↔ muted nylon (DSP-03). *(research: map % → cutoff Hz curve.)* |
| Decay | `decay` | float | 0–100 (short ↔ long) | 70 | % | Feedback gain / ring time. Maps to feedback ≈ 0.80–0.999, **clamped < 1** for stability (DSP-03, QUAL-01). *(research: % → feedback taper.)* |
| Material | `material` | float | 0–100 (steel ↔ nylon) | 30 | % | One-knob macro co-moving Damping + Decay for the single-gesture steel↔nylon sweep (DSP-07). *(research: define the co-move curve.)* |

## Tuning

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Coarse Tune | `coarseTune` | int | -24–+24 | 0 | st | Transpose. |
| Fine Tune | `fineTune` | float | -100–+100 | 0 | cents | Fine pitch. |

> Pitch itself comes from the played MIDI note via delay length (SR ÷ N) or modal mode set,
> tuned with a fractional-delay all-pass interpolator (DSP-02) so high notes stay in tune.

## Amp + Dynamics

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Amp Attack | `ampAttack` | float | 0–2 | 0.001 | s | Output amplitude attack (mostly Bow / shaping; the body's decay is intrinsic to the model). |
| Amp Release | `ampRelease` | float | 0–5 | 0.2 | s | Output amplitude release / note-off damping. |
| Velocity → Brightness | `velToBrightness` | float | 0–100 | 60 | % | How much harder playing brightens/strengthens the excitation — the model's dynamic response (FUNC-06). |

## Output

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Output Level | `outputLevel` | float | -inf–0 | -6 | dB | Master output gain. |

## Research Must Confirm (Stage 0)

These open items from BRIEF.md feed the architecture + final spec:

1. **Fractional-delay tuning method** — all-pass interpolation for accurate KS/waveguide pitch (DSP-02). O-Lyrica is the in-house reference.
2. **Modal mode count + default mode sets** — bar vs bell mode tables; per-mode freq/amp/decay relationships (higher modes quieter, faster-decaying).
3. **Material macro curve** — the (cutoff, feedback) co-move that makes the steel↔nylon sweep audibly the demo's move.
4. **Excitation design** — DC-safe, band-limited noise burst / impulse; stable, sustaining Bow stick-slip drive (no click/buzz/alias — DSP-08, QUAL-01).
5. **Waveguide-vs-KS string scope** — whether the dual-delay waveguide (and meaningful Excitation Position) ships in v1.0 or is `nice`/deferred (DSP-06).
6. **Polyphony** — confirm 16 voices (matches O-simpleFM / O-simpleAdditive); visualization follows the most recent / loudest voice (FUNC-05).
7. **Visualization tap** — per-voice vs global loop-state tap feeding the animated loop/flow diagram via lock-free FIFO (UI-02, PERF-01).
8. **Bow second control** — whether to add a Bow Speed param alongside Bow Force.

## Deferred to v1.1+ (per BRIEF Out of Scope)

- Blow / tube resonator (winds, brass)
- Sculpture-level component modeling (movable pickups, multi-string)
- Sympathetic / coupled strings, body convolution
- Built-in effects (reverb/delay/chorus)
- Deep modulation matrix / LFOs beyond velocity→brightness

---
*Draft generated from BRIEF.md on 2026-06-26. Replace with full parameter-spec.md at mockup finalization before Stage 1.*
