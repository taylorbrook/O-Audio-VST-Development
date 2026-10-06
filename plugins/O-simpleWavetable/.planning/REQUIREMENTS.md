# O-simpleWavetable - Requirements

---
version: 1.0.0
plugin: O-simpleWavetable
created: 2026-10-05
lastUpdated: 2026-10-06 (stage-2 verify)
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 30
**Coverage:** must: 22 | should: 7 | nice: 1

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Single wavetable oscillator plays each note by looping the frame at Position, at the note's pitch | must | complete | stage-2 |
| FUNC-02 | Five built-in 32-frame banks: Sine→Saw, Sine→Square, Pulse Width, Formant, Drive | must | complete | stage-2 |
| FUNC-03 | Import audio slices a file into consecutive 2048-sample frames (≤ 256, mono-summed, no resampling) | must | complete | stage-2 |
| FUNC-04 | Imported bank persists in plugin state (project/preset reopen restores it) | must | complete | stage-2 |
| FUNC-05 | Global LFO hard-wired to Position: rate (free Hz / tempo sync), depth, 5 shapes | must | complete | stage-2 |
| FUNC-06 | Per-voice mod ADSR with bipolar amount hard-wired to Position | must | complete | stage-2 |
| FUNC-07 | Poly (16 voices) / Mono (last-note priority) voice modes with amp ADSR and velocity → amplitude | must | complete | stage-2 |
| FUNC-08 | Factory presets, each isolating one concept (stepped vs smooth, alias demo, drive sweep, vowel pad, low bit) | should | pending | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Interpolation Off snaps to the nearest frame; On crossfades adjacent frames. In-cycle phase read is interpolated either way | must | complete | stage-2 |
| DSP-02 | Band-limiting On reads per-octave band-limited mipmaps; Off reads raw frames (audible aliasing at high pitch) | must | complete | stage-2 |
| DSP-03 | Bit Depth quantizes oscillator output post-interpolation, Full / 16 → 3 bits | must | complete | stage-2 |
| DSP-04 | Bank frames DC-removed and normalized to a common peak | must | complete | stage-2 |
| DSP-05 | Effective Position = clamp(Position + LFO + Env, 0, 1), smoothed (no zipper with Interpolation On) | must | complete | stage-2 |
| DSP-06 | Bank generation, import decode and mipmap build run off the audio thread with an RT-safe swap | must | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Bank panel: 3D/stacked view of all frames, current frame highlighted, Position marker | must | pending | stage-3 |
| UI-02 | Current-cycle panel: the interpolated, bit-quantized cycle being read now | must | pending | stage-3 |
| UI-03 | Harmonics panel: live bar graph of harmonics 1–32 of the cycle being heard (incl. band-limit level) | must | pending | stage-3 |
| UI-04 | Import button showing source filename and frame count | must | pending | stage-3 |
| UI-05 | Plain-language tooltips on every control, localized per suite convention | should | pending | stage-3 |
| UI-06 | Projector-readable single page, consistent with O-simpleFM / O-simpleAdditive | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations/locks in processBlock) | must | complete | stage-2 |
| PERF-02 | 16 voices at 44.1–96 kHz well within a typical CPU budget | should | pending | stage-4 |
| PERF-03 | Visual panels update smoothly (≥ 30 fps) without stalling the audio thread | should | pending | stage-3 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | complete | stage-1 |
| COMPAT-02 | Windows VST3 build with WebView2 static linking | should | pending | stage-4 |
| COMPAT-03 | Import accepts WAV / AIFF / FLAC at any sample rate | should | complete | stage-2 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges (beyond the deliberate ones: stepping, aliasing, quantization) | must | partial | stage-2 |
| QUAL-02 | Band-limiting On is alias-free (aliases ≥ 60 dB down) up to C8 on the brightest frame | must | complete | stage-2 |
| QUAL-03 | No clicks on bank switch, import swap, or toggle changes | must | complete | stage-2 |
| QUAL-04 | Each teaching contrast is clearly audible (stepped vs smooth, aliasing vs clean, bit depth) | nice | pending | stage-4 |

## Acceptance Criteria Details

### FUNC-01: Wavetable oscillator
- [x] A held note's fundamental matches MIDI pitch within ±1 cent across the keyboard
- [x] Changing Position changes timbre without changing pitch

### FUNC-02: Built-in banks
- [x] Each bank has 32 frames of 2048 samples
- [x] Sine→Saw frame *k* contains harmonics 1…*k* (FFT check); frame 1 is a pure sine
- [x] Sine→Square contains odd harmonics only (even harmonics ≥ 60 dB down)
- [x] Drive frames show rising odd-harmonic content monotonically across the bank

### FUNC-03: Import
- [x] A 10 s 44.1 kHz file yields min(⌊samples/2048⌋, 256) frames
- [x] Import runs without audio dropouts while notes are held

### FUNC-04: Import persistence
- [x] Save/reload of the project restores the imported bank bit-identically (or within the chosen encoding's tolerance) and its filename

### FUNC-05: LFO
- [x] At depth > 0, Position visibly and audibly sweeps at the set rate; tempo sync locks to host BPM
- [x] All 5 shapes selectable; one global phase (all voices scan together)

### FUNC-06: Mod envelope
- [x] With Env Amount > 0, each new note sweeps Position by the envelope shape; negative amount sweeps downward

### FUNC-07: Voices
- [x] Poly plays 16 simultaneous notes; Mono plays one with last-note priority
- [x] Amp ADSR and velocity shape amplitude

### DSP-01: Interpolation
- [x] Off: a slow Position sweep produces 32 discrete timbre steps on a built-in bank
- [x] On: the same sweep morphs continuously (no step discontinuities in spectrum over time)

### DSP-02: Band-limiting
- [x] Off: Sine→Saw last frame at C7 shows aliased components below the fundamental
- [x] On: same note shows no components above Nyquist folding back (see QUAL-02)
- **QUAL-02 note (2026-10-05, Stage 2 D-B — NAMED EXCEPTION):** the strict gates (C8 ≤ −100 dB inharmonic, A0–C8 sweep ≤ −70 dB at 44.1/48/96 kHz) run on Drive 32, Pulse frame 1 and a Saw-1023. Narrow pulses (Pulse 16/24/32) are judged on an **equal-RMS metric** instead: worst alias ≤ −60 dB relative to an equal-RMS sine (measured −67.0 dB). Their energy is spread over many weak harmonics, so measuring aliases against the strongest single harmonic makes them look louder than they sound.

### DSP-03: Bit depth
- [x] At 3 bits the output has 8 amplitude levels; Full is bit-identical to bypass

### DSP-04: Normalization
- [x] All frames in a bank share peak level ±0.5 dB; DC ≤ −80 dB

### DSP-05: Effective position
- [x] The sum is clamped to [0, 1]; knob moves produce no zipper noise with Interpolation On

### DSP-06: Off-thread build
- [x] No allocation on the audio thread during bank switch or import (alloc gate)

### UI-01..04
- [ ] Bank highlight and marker follow the effective Position, including LFO/env motion
- [ ] Cycle panel shows the stair-steps at low bit depth
- [ ] Harmonics bars match an offline FFT of the cycle being heard
- [ ] Import shows filename and frame count

### PERF-01 / COMPAT-01 / QUAL-01..03
- **QUAL-01 partial (2026-10-06 stage-2 verify):** two confirmed clicks in normal playing: a voice-steal hard stop (W1) and the Mono retrigger velocity step (W2). See stages/2-dsp/VERIFICATION.md.
- [x] Alloc gate passes in processBlock (stage-2, 2026-10-06: 0 allocs)
- [x] pluginval passes VST3 + AU (stage-1, 2026-10-05, strictness 10)
- [x] No clicks on bank switch / import / toggles (click detector; import-check ratio worst 1.31)
- [x] Band-limited aliasing ≥ 60 dB down up to C8 (−114.3 dB strict; D-B pulses −67.0 dB equal-RMS)

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01..07, DSP-*, PERF-01, QUAL-01..03, COMPAT-03 |
| stage-3 | UI-*, PERF-03 |
| stage-4 | FUNC-08, PERF-02, COMPAT-02, QUAL-04, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Drawing your own frames | Keeps v1.0 focused on the scan mechanism | v1.1+ |
| Filters, unison/detune, warp modes, FX, 2nd oscillator | They hide the mechanism being taught | — |
| Modulation matrix / destinations other than Position | Position is the only idea being taught | — |
| Pitch-synchronous / resampled import | Literal slicing is the lesson | v1.1+ |

---
*Generated from BRIEF.md on 2026-10-05*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
