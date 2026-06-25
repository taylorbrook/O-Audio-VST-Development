# O-simpleGrain — Parameter Specification (DRAFT)

---
version: 0.1.0-draft
plugin: O-simpleGrain
created: 2026-06-24
source: BRIEF.md parameter table
status: draft (full parameter-spec.md required before Stage 1 — produced by mockup finalization)
---

> **DRAFT** — extracted from BRIEF.md for Stage 0 complexity/architecture planning.
> Ranges are starting proposals to be validated by research. Items marked *(research)*
> are open questions Stage 0 should confirm and fold into the final spec.

## Source

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Source Sample | `sourceSample` | choice | curated list (fire, voice, water, piano hit, …) | fire | — | Which built-in short sound is granulated. THE raw material. Built-ins embedded as binary. |
| Load… (user file) | *(action)* | — | drag-drop / file picker | — | — | Load-your-own short source. macOS WebView content-streaming drag-drop (`juce::Base64::convertFromBase64`, NOT `MemoryBlock::fromBase64Encoding`) + picker fallback. Not an APVTS param. |

## Grain

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Grain Size | `grainSize` | float | 2–200 ms | 30 | ms | Length of each grain. THE buzz↔fragments control: a few ms = pitched buzz, tens of ms = recognizable fragments. Defining granular control. |
| Density | `density` | float | 1–200 grains/s | 40 | grains/s | Grains fired per second; with grain size sets overlap depth. *(research: expose as grains/sec vs grain period vs overlap factor — show a live **overlap readout** either way.)* |
| Position | `position` | float | 0–100% of source | 50 | % | Read position in the source — the playhead / freeze point. |
| Scan / Time-Stretch | `scan` | float | -200 – +200% | 0 | % | Speed the read head moves through the source. 0% = held; <100% = stretched; negative = reverse. Pairs with Freeze. |
| Freeze | `freeze` | bool | off / on | off | — | Pins the read head on the current instant and sustains indefinitely. Headline class move (one moment → held pad). Must be zipper-free. |

## Window Shape

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Window Shape | `windowShape` | choice | rectangular / triangular / Welch / Gaussian / Hann | Hann | — | Per-grain amplitude envelope — the single most audible low-level control. Rectangular adds a real broadband click per grain (teaching artifact, not a bug). Five class-figure shapes, precomputed LUTs. |

## Spray & Scatter

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Pitch Spray | `pitchSpray` | float | 0 – ±12 st | 0 | st | Random per-grain transposition — makes a frozen texture shimmer instead of buzzing on one repeated grain. |
| Position Spray | `positionSpray` | float | 0–100% | 0 | % | Random per-grain read position — scatters reads across the source so no two grains are identical. |
| Scatter (period randomness) | `scatter` | float | 0–100% | 0 | % | Randomizes the grain period. 0% = synchronous (constant, pitched, discrete sidebands); high = asynchronous (noisy cloud). The sync↔async axis in one knob. |
| Grain Pitch | `grainPitch` | float | -24 – +24 st | 0 | st | Global transposition of grains (independent of MIDI). *(research: MIDI-key→grain coupling — key-tracked resample vs gate-only.)* |

## Amplitude Envelope (per-voice ADSR)

| Param | ID | Type | Range | Default | Unit |
|-------|----|------|-------|---------|------|
| Amp Attack | `ampAttack` | float | 0–5 s | 0.01 | s |
| Amp Decay | `ampDecay` | float | 0–5 s | 0.3 | s |
| Amp Sustain | `ampSustain` | float | 0–100% | 80 | % |
| Amp Release | `ampRelease` | float | 0–5 s | 0.4 | s |

## Output

| Param | ID | Type | Range | Default | Notes |
|-------|----|------|-------|---------|-------|
| Output Level | `outputLevel` | float | -inf – 0 dB | 0 | Master output gain. |

## Likely Additions / Confirmations — Stage 0 Research

These are open questions from the BRIEF to resolve and fold into the final spec. Most are
engine/UI config rather than new user params; flagged here so architecture planning accounts for them.

- **Density vs overlap exposure** — grains/sec vs grain period vs overlap factor; settle the control and the live overlap-readout formula.
- **MIDI-key → grain-pitch coupling** — key-tracked transposition (resample) vs gate-only. Affects whether `grainPitch` + MIDI sum or MIDI only gates.
- **Fractional-read interpolation + anti-aliasing** — band-limit on upward transposition so high pitch-spray grains stay clean (DSP-08).
- **Per-grain stereo pan spray** — `panSpray` *(research, keep minimal/optional)*.
- **Grain-count / CPU meter** — live readout (pedagogical: density × grain size × polyphony = cost). Readout, not a param (UI-05).
- **Max simultaneous grain cap + graceful voice/grain-stealing** — bound the grain pool so high density × size × poly cannot xrun (PERF-02).
- **Polyphony** — proposing 8 (granular is heavier than FM/additive); confirm against grain budget. Engine config, not a param.
- **Source-length cap** — for built-ins and loaded files.
- **Velocity routing** — to amp and/or density: `velToAmp` / `velToDensity` *(research)*.

## Out of Scope (v1.0)

Spectral STFT (freeze/blur/filter → O-simpleSpectral), phase-vocoder/tempo-locked stretch,
live-input recording, effects (reverb/delay/chorus), deep mod matrix / LFO networks,
multi-sample / multi-layer sources. See REQUIREMENTS.md "Out of Scope".

---
*Draft generated from BRIEF.md on 2026-06-24. Replace with full parameter-spec.md at mockup finalization before Stage 1.*
