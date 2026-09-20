---
title: "Idiophone Partial Models — Tube, Plate, Bowl, Glass"
created: 2026-09-20
juce_version: "8.0.14"
summary: "Modal frequency-ratio and amplitude tables for four non-bell idiophones (tubular bell, free circular plate, singing bowl, wine glass), each with its provenance grade, mapped onto O-Bells' 8-partial voice for the v4.7.0 partialModel parameter."
domain: dsp
type: research
keywords:
  - modal-synthesis
  - tubular-bell
  - circular-plate
  - singing-bowl
  - wine-glass
  - partial-ratios
  - o-bells
stages: [0, 2]
agents: [dsp, research]
---

# Idiophone Partial Models — Tube, Plate, Bowl, Glass

Written for O-Bells v4.7.0 (`improvements/preset-differentiation-v4.6-v4.8.md`, Step 3, RC-4).
The existing bell research (`modal-synthesis-bells-academic-research.md`) covers church bells and
the free-free BAR series (1 : 2.76 : 5.40 : 8.93) only. This file adds the four bodies the
`partialModel` parameter needs.

**Read the provenance column.** The frequency ratios are physics; the amplitude tables are
DESIGN values — strike spectra depend on where and with what the body is hit, and no source
gives a canonical set. Do not cite the amplitudes as measurements.

| Grade | Meaning |
|---|---|
| **C** | Computed in-session from the governing equation; reproduces the published values |
| **M** | Measured values quoted from a paper, confirmed by search this session |
| **F** | Read off a published figure by eye (±3 %) |
| **R** | Recalled from the literature, NOT re-verified against the paper text; cross-checked against an M or F value |
| **X** | Extrapolated beyond the published modes |
| **D** | Design value (no physical source) |

---

## 1. Tubular bell (orchestral chime)

A chime is a free-free tube: transverse modes ∝ (2n+1)² = 9 : 25 : 49 : 81 : 121 : 169 : 225 : 289.
Modes 4, 5, 6 stand in the ratio 81 : 121 : 169 ≈ 2 : 3 : 4.2, close enough to harmonic that the
ear assigns a virtual **strike pitch one octave below mode 4**. There is NO physical partial at
the strike pitch. (Rossing, *Science of Percussion Instruments*; HyperPhysics "Tubular Bells" —
ratios **M**, the (2n+1)² law itself is textbook beam theory.)

Normalised to the strike pitch (mode 4 = 2.000, unit = 40.5):

| Mode | (2n+1)² | Ratio to strike pitch |
|---|---|---|
| 1 | 9 | 0.222 — inaudible in practice, omitted |
| 2 | 25 | 0.617 |
| 3 | 49 | 1.210 |
| 4 | 81 | 2.000 |
| 5 | 121 | 2.988 |
| 6 | 169 | 4.173 |
| 7 | 225 | 5.556 |
| 8 | 289 | 7.136 |

## 2. Free circular plate

Thin flat plate, free edge, Poisson ν = 0.33. Roots of the free-edge frequency equation
(Leissa, *Vibration of Plates*, NASA SP-160, §2.1.3), solved numerically this session — grade **C**.
λ² reproduces Leissa's tabulated 5.253 / 9.084 / 12.23 / 20.52 to 0.2 %.
(m = nodal diameters, n = nodal circles.)

| Mode (m,n) | λ² | Ratio |
|---|---|---|
| (2,0) | 5.262 | 1.000 |
| (0,1) | 9.069 | 1.723 |
| (3,0) | 12.244 | 2.327 |
| (1,1) | 20.513 | 3.898 |
| (4,0) | 21.527 | 4.091 |
| (5,0) | 33.062 | 6.283 |
| (2,1) | 35.243 | 6.698 |
| (0,2) | 38.507 | 7.318 |
| (6,0) | 46.809 | 8.896 |

Character: dense and non-harmonic, with the 3.90 / 4.09 and 6.28 / 6.70 near-pairs that make a
struck plate "clang" rather than ring. Real gongs add amplitude-dependent pitch glide
(Rossing & Fletcher, JASA 73(1), 1983) — out of scope here.

## 3. Singing bowl

Flexural (m,0) modes of an axisymmetric shell, m = 2…6. Measured — grade **M**:
Inácio, Henrique & Antunes, "The Dynamics of Tibetan Singing Bowls", *Acta Acustica* 92 (2006):

    1 : 2.77 : 5.18 : 8.12 : 11.53

The thin-ring law f ∝ m(m²−1)/√(m²+1) gives 1 : 2.83 : 5.42 : 8.77 : 12.87; the measured/ring
quotient falls ≈ 0.028 per mode (0.979, 0.956, 0.926, 0.896). Continuing that trend — grade **X**:
m = 7 → 15.4, m = 8 → 19.6.

Real bowls split every mode into a doublet a few Hz apart (slight asymmetry), which is the slow
beating of a struck bowl. Not modelled by the table; Shimmer / Unison cover it.

## 4. Wine glass / glass harmonica

Same (m,0) family, but the stem-clamped, tall-walled cup pulls the series well below the ring law.

- (3,0) : (2,0) across three measured glasses: 2.13 (Jundt et al., JASA 119(6), 2006, Fig. 3a — **F**),
  2.37 and 2.52 (two further glasses, quoted in search results this session — **M**).
- Series used — grade **R**: 1 : 2.32 : 4.25 : 6.63 : 9.38 (Rossing, "Acoustics of the glass
  harmonica", JASA 95, 1994, as recalled). Cross-check against Jundt Fig. 3a (peaks ≈ 0.94, 1.91,
  3.62, 5.74 kHz → 1 : 2.04 : 3.86 : 6.13): within 6–9 %, i.e. inside the glass-to-glass spread
  the (3,0) figures above already show. **If anyone gets the 1994 paper, re-verify this row.**
- Measured/ring quotient: 0.820, 0.784, 0.756, 0.729 → **X**: m = 7 → 12.4, m = 8 → 15.7.

Jundt Fig. 3a impulse-response peak heights, empty glass (**F**): 0.72 : 0.13 : 0.27 : 0.27 for
m = 2…5 → 1 : 0.18 : 0.38 : 0.38. The only measured amplitude data in this file.

---

## 5. Mapping onto the O-Bells voice (8 partial slots)

Engine constraints that shape the mapping:

1. **Slot 1 is the tuned prime, ratio 1.000, in every model.** `startNote()` sets each partial to
   `detune × ratio`, so slot 1 is the pitch the TuningEngine / Note Expression controls, and
   `primeLevel` acts on it.
2. **Slots 0–1 are the "hum partials"** — the decay law gives them the long hum stage, and
   `humLevel` / `primeLevel` / `humFollow` address them by index.
3. Tables ascend, because per-slot decay multipliers, bloom bands and strike-position tilt all
   assume slot index ≈ spectral position.

Consequences:

- **Tubular.** Slot 0 = mode 2 (0.617), slots 2–7 = modes 3–8. Slot 1 (1.000) is a SYNTHETIC
  prime at the virtual strike pitch — a real chime has nothing there. It is kept quiet (0.30) so
  the 2 : 3 : 4.2 group carries the pitch as it does in the instrument; `primeLevel` down gives
  the literal tube, up gives a firmer pitch.
- **Plate / Bowl / Glass.** The lowest mode IS the pitch, so slots 1–7 = modes 1–7 and nothing
  physical sits below the prime. Slot 0 keeps the engine's sub-octave hum (0.5) at −18 dB (0.12)
  — an ENGINE convention so `humLevel` stays live in every model, not physics.

### Ratio tables (slot 0 … 7)

| Model | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Tubular | 0.617 | 1.000 | 1.210 | 2.000 | 2.988 | 4.173 | 5.556 | 7.136 |
| Plate | 0.500 | 1.000 | 1.723 | 2.327 | 3.898 | 4.091 | 6.283 | 6.698 |
| Bowl | 0.500 | 1.000 | 2.770 | 5.180 | 8.120 | 11.530 | 15.400 | 19.600 |
| Glass | 0.500 | 1.000 | 2.320 | 4.250 | 6.630 | 9.380 | 12.400 | 15.700 |

### Amplitude tables (slot 0 … 7) — grade D, except Glass slots 1–4 (F)

Replace Classic's `1/(p+1)`. In code each row is scaled so its sum equals Classic's
(Σ 1/(p+1) = 2.718), keeping model switches roughly level-matched.

| Model | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | Rationale |
|---|---|---|---|---|---|---|---|---|---|
| Tubular | 0.25 | 0.30 | 0.55 | 1.00 | 0.85 | 0.65 | 0.40 | 0.25 | Modes 4–6 dominate; quiet synthetic prime |
| Plate | 0.12 | 1.00 | 0.80 | 0.90 | 0.60 | 0.65 | 0.45 | 0.40 | Dense, slowly falling — the "clang" |
| Bowl | 0.12 | 1.00 | 0.70 | 0.35 | 0.18 | 0.09 | 0.05 | 0.03 | (2,0) and (3,0) carry the sound; steep fall |
| Glass | 0.12 | 1.00 | 0.18 | 0.38 | 0.38 | 0.15 | 0.08 | 0.04 | Slots 1–4 from Jundt Fig. 3a; weak (3,0) is the glass signature |

### Nyquist

Bowl reaches 19.6× and Glass 15.7× the played pitch (Classic tops out at 9.5×), and the octave
layer doubles that. Partials above 0.45 · fs must be culled at note-on for these models.

---

## Sources

- T. D. Rossing, *Science of Percussion Instruments*, World Scientific, 2000 — chimes, plates.
- HyperPhysics, "Tubular Bells": http://hyperphysics.phy-astr.gsu.edu/hbase/Music/tbell.html
- A. W. Leissa, *Vibration of Plates*, NASA SP-160, 1969 — free circular plate frequency equation.
- T. D. Rossing, N. H. Fletcher, "Nonlinear vibrations in plates and gongs", JASA 73(1), 1983:
  https://www.phys.unsw.edu.au/music/people/publications/Rossingetal1983.pdf
- O. Inácio, L. Henrique, J. Antunes, "The Dynamics of Tibetan Singing Bowls", Acta Acustica 92, 2006:
  http://projects.itn.pt/VibroImpacts/Ref_A.pdf
- G. Jundt et al., "Vibrational modes of partly filled wine glasses", JASA 119(6), 2006:
  https://phys.unsw.edu.au/music/people/publications/Jundtetal2006.pdf
- T. D. Rossing, "Acoustics of the glass harmonica", JASA 95, 1994 (recalled, see §4).
