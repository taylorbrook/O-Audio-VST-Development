# O-ParametricEQ - Creative Brief

## Overview

**Type:** Effect (Parametric EQ)
**Core Concept:** A clean, surgical 24-band parametric EQ with analog-matched curves at zero latency — the essentials of a flagship EQ, built to the highest quality.
**Status:** 💡 Ideated
**Created:** 2026-11-12

## Vision

O-ParametricEQ is Ouaricon Audio's reference equalizer. The long-term target is to rival FabFilter Pro-Q; v1 deliberately ships a narrower feature set and puts the effort into getting the fundamentals right: filter accuracy, transparency, automation smoothness, and a fast curve-first workflow.

It is the clean counterpart to O-AnalogEQ (v1.5.7, analog-coloured). O-ParametricEQ adds no colour, no saturation, no modelled circuit behaviour — what the curve shows is what the signal gets.

**v1 quality bar:**

- **Analog-matched magnitude response.** Bells and shelves keep their shape up to Nyquist at 44.1 / 48 kHz (no bilinear cramping), without oversampling and without latency.
- **Zero latency, minimum phase.** One processing mode, done properly.
- **Click-free modulation.** Any parameter — frequency, gain, Q, slope, type, channel placement, band on/off — can be automated or dragged fast without zipper noise, clicks, or instability, including steep cuts at low frequencies.
- **The displayed curve is the true response.** The drawn curve is computed from the actual filter coefficients at the current sample rate, not from an idealised prototype.
- **Low noise and numerical headroom.** Stable, quiet filters at very low frequencies and high Q, at all supported sample rates.

**Scope decisions (2026-11-12):**

| Decision | v1 | Later |
|----------|----|-------|
| Phase modes | Zero-latency minimum phase only | Linear phase, natural phase |
| Dynamic EQ | Static only — band engine and parameter layout reserve room for it | Dynamic bell / shelf bands in v1.x without breaking saved sessions |
| Bands | 24, created on demand | — |
| Slopes | 6–96 dB/oct | Brickwall |
| Channel placement | Per band: Stereo / Left / Right / Mid / Side; mono and stereo layouts | Surround layouts |

## Parameters

**Per band (×24):**

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Enabled | Off / On | Off | Whether the band exists. Bands are created by clicking the curve. |
| Bypass | Off / On | Off | Mutes the band's effect without removing it. |
| Type | Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch, Band Pass, Tilt Shelf | Bell | Filter shape. |
| Frequency | 10 Hz – 30 kHz (log) | 1 kHz | Centre / corner frequency. |
| Gain | −30 to +30 dB | 0 dB | Bell, shelf and tilt gain. Unused by cut, notch and band pass. |
| Q | 0.025 – 40 (log) | 1.0 | Bandwidth / resonance. |
| Slope | 6, 12, 18, 24, 36, 48, 72, 96 dB/oct | 12 dB/oct | Steepness, on every type that supports it. |
| Placement | Stereo, Left, Right, Mid, Side | Stereo | Which channel component the band processes. |

Reserved per band for dynamic EQ (not exposed in v1; the parameter layout must leave room): dynamic range, threshold, auto-threshold.

**Global:**

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Output Gain | −36 to +36 dB | 0 dB | Post-EQ level trim. |
| Auto Gain | Off / On | Off | Compensates the loudness change the curve introduces. |
| Gain Scale | 0 – 200 % | 100 % | Scales the gain of all bands together. |
| Analyzer | Off / Pre / Post / Pre+Post | Post | Real-time spectrum display source. |
| Bypass | Off / On | Off | Click-free global bypass. |

Exact IDs, ranges, skews and the automation layout for 24 bands are settled in Stage 0.

## UI Concept

*Synced from finalized mockup v1 (2026-11-12); `mockups/v1-ui.yaml` is the source of truth for UI details.*

**Window:** 960 × 600 default, resizable 800 × 500 to 1600 × 1000, aspect not locked. Header (40 px) and band strip (76 px) keep their heights; the response display takes all extra space.
**Layout:** A large interactive frequency-response display is the instrument (820 × 440 at default). Bands are nodes on the curve: click to create, drag for frequency and gain, scroll for Q, double-click to reset gain. Under the display, a 24-chip band row (empty / exists / bypassed / selected) and a display-range selector (±6 / ±12 / ±30 dB). A docked bottom strip for the selected band: type, frequency, gain, Q, slope, placement segments, and icon buttons for band bypass, listen and delete. Slope is disabled for Bell / Notch / Band Pass; gain for Low Cut / High Cut / Notch / Band Pass. Header: title, preset manager, analyzer mode, global bypass. Right column (124 px): output meter, output gain, gain scale, auto gain.
**Visual Style:** Ouaricon Naturalist house style (`ouaricon-naturalist-001`), EB Garamond with tabular lining figures. The display is the paper: sepia grid, ink summed curve, watercolour-wash band fills, graphite analyzer wash, and an emperor dragonfly plate (hairlines, 30 % opacity) behind the grid — production needs a real engraving. Bands are coloured by index from an 8-pigment cycle (ochre, madder, verdigris, indigo, sienna, sap green, tyrian, prussian blue). Placement badge on the node: L / R / M / S, none for Stereo (strip segments LR / L / R / M / S).
**UI state, not host parameters:** analyzer mode, display range, selected band, listen, window size.
**Carried to Stage 3 (as built in v1, open to change):** single summed curve across all placements; header controls 6 px from the top edge; reflow resize needs a DAW check at min / default / max; typed entry opens after the 240 ms double-click window; clicking an empty chip enables that band at its stored values.
**Key Elements:**
- Real-time spectrum analyzer behind the curve (pre / post)
- Per-band curve fills plus the summed response
- Band solo / listen while dragging
- Value entry by typing; double-click to reset
- Output level meter

## Use Cases

- Surgical correction on individual tracks: resonances, rumble, harshness, with steep cuts and narrow notches.
- Broad tonal shaping on buses and the mix bus where transparency matters.
- Mid/side and left/right work: tightening low-end in the side channel, opening the top of the mid.
- Automated EQ moves (filter sweeps, section-by-section tone changes) that must stay artifact-free.
- The default "first EQ you reach for" in the Ouaricon suite, alongside O-AnalogEQ for colour.

## Inspirations

- FabFilter Pro-Q 3 / 4 — the long-term benchmark for workflow and filter quality
- DMG EQuilibrium, Kirchhoff-EQ, TDR Nova / SlickEQ — clean-EQ quality references
- O-AnalogEQ — sibling; this plugin is its transparent counterpart

## Technical Notes

- **Curve matching:** analog-matched (de-cramped) minimum-phase designs for bell and shelf at zero latency; candidate approaches to evaluate in Stage 0 include matched-magnitude biquad designs (Orfanidis, Vicanek) against oversampling. Zero latency rules out look-ahead and FIR correction.
- **Modulation-safe topology:** filter structures that stay stable and click-free under fast coefficient changes (TPT / state-variable forms), with per-sample or sub-block parameter smoothing. Suite precedent: biquad family switches click and a low-pass parked near Nyquist explodes — use TPT SVF and clamp to Nyquist.
- **Steep slopes:** 6–96 dB/oct as cascaded sections; slope and type changes must crossfade or otherwise avoid discontinuities.
- **Precision:** double-precision filter state for low-frequency / high-Q stability.
- **Display accuracy:** curve evaluated from the live coefficients at the session sample rate; analyzer FFT off the audio thread.
- **Forward compatibility:** band engine and state format designed so dynamic EQ, linear phase and surround can be added without breaking v1 sessions or automation.
- **Suite fit:** preset-manager, eb-garamond and vu-meter modules; French and zh-Hans localisation per suite practice. The existing `analog-eq-unit` module is coloured and not the basis for this engine.
- Real-time safe: no allocation or locks in processBlock; reports 0 samples latency.

## Next Steps

- [ ] Research and plan the DSP architecture (`/plan O-ParametricEQ`)
- [ ] Create UI mockup (`/start O-ParametricEQ` → option 3)
- [ ] Start implementation (`/implement O-ParametricEQ`)
