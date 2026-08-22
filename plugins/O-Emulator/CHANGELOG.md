# Changelog — O-Emulator

All notable changes to O-Emulator are documented here.

## [1.0.1] — 2026-08-21

### Fixed

- **Age now audibly processes the sound instead of only adding a constant
  hiss** (user report: "the age parameter seems to just be the same static
  noise addition"). Root cause was three-fold — of Age's three designed
  components, only the noise bed was audible:
  - **Pitch drift was structurally neutered.** The ±15-cent / 0.3 Hz
    rate-domain walk needed ~±220 host samples of time storage against
    per-console rails of 9–64 samples, so the offset hit its rail within
    ~100 ms, the factor was forced back to 1.0 and the walk bounced —
    clipping the wobble below 1 cent. Redesigned as an offset-domain servo:
    a ~1.2 Hz random walk (σ normalized at prepare, rate-invariant) is
    tanh-bounded into a time-offset target inside 0.85 × the active rail ×
    age, tracked by a 2.5 Hz servo whose per-chunk delta IS the read-rate
    deviation (capped ±15 cents). Bounded by construction, no rail bounces,
    exactly 1.0 at age 0. Measured (harness G4): 9.0 cents of warble at
    age 100 vs 0.01 at age 0.
  - **Dulling was near-inaudible.** Corner map changed from
    ×(1 − 0.55·age/100) to ×2^(−2·age/100) — linear in octaves, ×0.25 at
    age 100 (was ×0.45), audible from mid ages.
  - Reported latency gains a +24-host-sample (48 kHz-scaled) drift-headroom
    term so PS1's shallow priming deepens (rail ~9 → ~33 samples); wet/dry
    alignment stays exact (priming targets the reported figure).

### Changed

- **The noise/hum bed is now program-dependent** (user direction: "it should
  be dynamic with the sound, not a constant hiss"). A per-chunk envelope
  follower on the wet pre-bed peak (5 ms attack; 150 ms hiss release, 400 ms
  hum release, rate-compensated, NaN-guarded) scales the bed:
  min(1, max(0, 4·env − 0.004)) — full bed at peaks ≥ −12 dBFS, proportional
  below, hard zero under −60 dBFS peaks, so the bed breathes with the
  program and silence stays exactly silent.

### Testing

- Render harness: full suite green. G2 rewritten for the program-dependent
  bed (burst excitation, hiss isolated on the L−R diff — pipeline residue
  and hum are mono and cancel; two new clauses assert the bed decays with
  the envelope and silence stays silent). G3 moved to the same burst/diff
  measurement. Digest anchors re-anchored (2.4 values retired +
  moved-asserted, v1.0.1 anchors recorded) — every wet render's bytes moved:
  enveloped bed, new dulling map, live drift trajectory, +24-sample latency.
- No parameter IDs, ranges, or state format changed; presets and sessions
  load unchanged.

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
