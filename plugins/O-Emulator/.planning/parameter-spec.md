# O-Emulator - Parameter Specification

## Overview

Authentic retro console audio emulation — any signal runs through genuine codec encode→decode round-trips, fixed console-domain sample rates with authentic interpolation, and per-console output-stage models for five classic systems (SNES / PS1 / NES / Game Boy / Genesis). One console selector plus four macro character knobs (Crush, Age, Reverb, Mix).

**Status:** BINDING contract for Stage 1+ (promoted 2026-08-20 from `parameter-spec-draft.md` with Stage 0 deltas from `research/ARCHITECTURE.md` Parameter Mapping folded in). The UI mockup phase was **deliberately skipped** — this spec was finalized directly from the draft + architecture per user decision; any future UI work may refine layout/labels but NOT parameter IDs, types, ranges, or defaults.

## Parameters

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| `console` | Console | Choice | SNES / PS1 / NES / Game Boy / Genesis (5 choices) | SNES | - | Selects the emulated system as one coherent pipeline: codec (BRR / SPU-ADPCM / DPCM / 4-bit wave / 8-bit DAC), fixed internal rate (32000 / 22050 / 33144 / 16384 / 26320 Hz), output interpolation (4-tap Gaussian for SNES+PS1, zero-order hold for NES/GB/Genesis), and output-stage model. Change triggers a 30 ms equal-power engine crossfade; reported latency is constant (max over all modes), so switching never re-reports latency. |
| `crush` | Crush | Float | 0.0 - 100.0 | 50.0 | % | Codec intensity macro — per-console curve touching encoder drive (0 → +12 dB), BRR/SPU shift floor, NES DPCM timer-rate walk (33.1 kHz → 4.2 kHz), GB level reduction (16 → 4 steps), Genesis DAC update rate (26.3 kHz → 8 kHz), and AA pre-filter opening at ≥ 80% (dosed aliasing). At 0 the signal STILL passes the codec round-trip — subtle color, not bypass. |
| `age` | Age | Float | 0.0 - 100.0 | 20.0 | % | Hardware-condition model: noise floor (−78 → −48 dB, ramps in above ~5%), mains hum 60/120/180 Hz (−80 → −54 dB), output-LP dulling (corner ×1.0 → ×0.45), and resample-ratio drift (0 → ±15 cents, ~0.3 Hz bounded random walk). |
| `reverb` | Reverb | Float | 0.0 - 100.0 | 0.0 | % | SPU reverb send level. PS1 register-model reverb (Hall preset) at a fixed 22.05 kHz half-rate domain, routable in every console mode. Send tap is post-codec (the reverb hears the degraded signal); return joins before the output stage. 0 = reverb fully bypassed (tail rendered only while send was recently > 0). |
| `mix` | Mix | Float | 0.0 - 100.0 | 100.0 | % | Parallel dry/wet blend via `juce::dsp::DryWetMixer`, linear mixing rule. Dry path is latency-compensated (`setWetLatency` = the exact `setLatencySamples` figure), so mix 0% is bit-transparent minus latency (FUNC-02). The Age bed (noise/hum) is wet-path only and is bypassed at mix 0%. |

## Parameter Count Summary

- Global: 5 (1 choice + 4 floats)
- **Total: 5**

## Design Notes

- **Skews:** all floats linear (per ARCHITECTURE.md Parameter Mapping). No skewed ranges in v1.0.
- **Smoothing:** the four float parameters are smoothed (`SmoothedValue`, ~20 ms). `console` is discrete — no smoothing; the 30 ms equal-power crossfade between pre-allocated engines handles click safety (equal-power, not Hann-complement — the two engine outputs are uncorrelated program).
- **Choice parameter constraint:** `console` has 5 entries — satisfies the AudioParameterChoice ≥ 2-choices constraint (repo critical pattern).
- **Crush integer-step behaviors** (NES timer-rate table, GB level steps): the knob is continuous with internal stepping quantized to step boundaries + 5 ms micro-fades — no UI detents in v1.0 (the mockup-deferred open question in ARCHITECTURE.md, resolved here as continuous since the mockup phase was skipped).
- **ASCII-safe host-facing labels:** all parameter/choice strings are plain ASCII (`Game Boy`, `Genesis`, ...) — `juce::String(const char*)` is ASCII-only (repo pattern).
- **No sample-rate parameter:** the console choice IS the rate (brief). All console rates are absolute; resample ratios recomputed in `prepareToPlay` for any host rate 44.1–192 kHz.
- **Latency:** one constant figure ≈ 100–130 samples @ 48 kHz (~2.3 ms), computed exactly in `prepareToPlay`, reported via `setLatencySamples(N)` (JUCE 8: non-virtual setter) and mirrored into `DryWetMixer::setWetLatency(N)`. Never re-reported on console switch or drift wobble.
- **Bounded at extremes:** Crush 100 + Age 100 stays bounded structurally — the codec domain clips at int16 rails before the output stage (QUAL-01).
- **State:** APVTS parameters only in v1.0; no non-parameter state. Preset-manager module added at Stage 3/4 per house pattern.
- **Reverb preset is fixed** (PsyQ "Hall" register table); a preset selector is explicitly out of scope for v1.0 (REQUIREMENTS.md) — registers are data, trivial to add in v1.1.

## Source

Draft extracted from `BRIEF.md` at ideation (2026-08-20); promoted to binding spec 2026-08-20 directly from `parameter-spec-draft.md` + `research/ARCHITECTURE.md` Parameter Mapping, skipping the UI mockup phase by user decision. Draft and architecture were verified consistent (5 params, identical IDs/ranges/defaults; ARCHITECTURE.md design-sync check reported no conflicts).
