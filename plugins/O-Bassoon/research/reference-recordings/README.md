# O-Bassoon Reference Recordings

## Source

VSCO 2 Community Edition (VSCO-2-CE), `Woodwinds/Bassoon/sus/`. CC0 public domain.
Repository: https://github.com/sgossner/VSCO-2-CE — see `LICENSE.md`.

## Files

| File | Format | Duration | Original |
|---|---|---|---|
| `bassoon-c3-sustain-v1.wav` | 16-bit / 44.1 kHz stereo PCM | ~8.7 s | `PSBassoon_C3_v1_1.wav` |
| `bassoon-c3-sustain-v2.wav` | 16-bit / 44.1 kHz stereo PCM | ~8.7 s | `PSBassoon_C3_v2_1.wav` |

The "v1" / "v2" suffix in the original filenames is presumed to be two velocity
layers of the same C3 sustain — the harder-played layer (v2) tends to expose
upper-partial energy more aggressively, useful for Phase 2.2 amplitude-shaping
tuning.

**Note on octave convention (D4 in RESEARCH.md):** VSCO-2-CE filename "C3" likely
refers to scientific pitch (C3 = MIDI 48 = 130.81 Hz). Tuner-confirm during
audition before relying on the file as a Phase 2.2 reference. If the measured
fundamental is closer to 261 Hz, the file is bassoon-mid-C (MIDI 60) and should
be re-archived as `bassoon-c4-sustain-*.wav`.

## Intended Use

Phase 2.2 (modal voice tuning) consumes these recordings as A/B listening
references and as the spectrum target for partial-ratio + amplitude tuning.
Phase 2.1 archives them ahead of time so the listening loop has zero setup
friction in Phase 2.2.

**This directory is NOT shipped with the plugin** — reference recordings stay
local-only under `research/`.

## Audition Notes (Phase 2.1 — pre-Phase 2.2)

Pending — to be filled in during the Phase 2.1 manual Logic verification step
(Gate 1 items 9–10). At minimum:

- Tuner-measured fundamental: ___ Hz (rename file to match if needed)
- Vibrato level: ___ (none / mild / moderate / heavy)
- Attack transient: ___ (clean / breath-noise / multiphonic onset)
- Room tone: ___ (anechoic / dry studio / room reverb)

## Phase 2.1 Spectrum Baseline

`phase-2.1-baseline-c3-spectrum.png` — Voxengo SPAN snapshot of the placeholder
integer-harmonic O-Bassoon voice at C3, captured during Gate 1 verification.
This is the **before** image for the Phase 2.2 partial-tuning A/B.

Capture procedure (per RESEARCH.md §1 OQ#10):
- Logic project at 48 kHz / 256-sample buffer
- O-Bassoon AU on a stereo software-instrument track, holding a sustained MIDI C3
- SPAN as the last AU effect: Stereo mode, Block 8192, Hann window, 4.5 dB/oct slope, Infinite average
- Wait 3 s for steady-state, screenshot the SPAN window
