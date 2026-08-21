# Changelog — O-Emulator

All notable changes to O-Emulator are documented here.

## [1.0.0] — 2026-08-21

Initial release. O-Emulator is a stereo retro-console audio emulation effect:
five complete console pipelines — SNES (BRR 4-bit, 32 kHz, Gaussian), PS1
(SPU-ADPCM, 22.05 kHz, Gaussian), NES (DPCM, 33.144 kHz, ZOH), Game Boy
(4-bit wave, 16.384 kHz, ZOH) and Genesis (8-bit DAC, 26.32 kHz, ZOH) — each
modelled as one coherent chain of codec, fixed internal rate, interpolation
and output stage, selected by a single **Console** switch that rides a 30 ms
click-safe crossfade.

Four macro knobs shape the sound: **Crush** (drive, integer step reduction
with micro-fades, anti-alias opening), **Age** (noise/hum bed, dulling,
pitch drift), **Reverb** (PS1 SPU hall send) and **Mix** (latency-compensated
dry/wet). Constant worst-case latency is reported to the host, so the dry
path stays sample-aligned at every setting.

### Added

- **Five console pipelines** behind one selector, with a per-console accent
  theme, spec readout and 5-segment switch in a naturalist
  engraved-field-guide UI (620×430 WebView).
- **16 factory presets** — two signatures per console (one clean, one
  aged/crushed) plus six cross-console utilities (Lo-Fi Drums, Parallel Grit,
  Reverb Chamber, Subtle Glue, Tape Wash, Crush Extreme). Flat alphabetical
  list; every preset stores all five parameters.
- **Preset band** (preset-manager v1.0.6): prev/next stepping, name readout,
  native save/load dialogs, and a two-click armed delete with 2.5 s
  auto-disarm. The current preset name survives a DAW session save/reload.
- **Knob interaction suite**: relative drag, shift-fine, mouse wheel,
  double-click typed value entry, Alt/Option-click reset to default.
- **Validation**: render-harness (52 checks, digest-anchored), pluginval
  strictness 10 (VST3 + AU), auval.
