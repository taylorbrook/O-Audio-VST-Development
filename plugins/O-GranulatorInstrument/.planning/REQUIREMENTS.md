# O-GranulatorInstrument - Requirements

---
version: 1.0.0
plugin: O-GranulatorInstrument
created: 2026-11-06
lastUpdated: 2026-11-09
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 24
**Coverage:** must: 15 | should: 7 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | MIDI instrument: notes spawn polyphonic voices, each running grain clouds across the enabled streams | must | partial | stage-2 |
| FUNC-02 | One audio file per stream slot (4 slots), loaded by drag-drop or a file browser; slots may share a file | must | partial | stage-2 |
| FUNC-03 | Key-tracked pitch relative to the per-stream root note, with keyTrack amount and pitch-bend range | must | partial | stage-2 |
| FUNC-04 | Per-stream scan: position, scan rate, scan mode (free/retrigger/ping-pong), loop region | must | partial | stage-2 |
| FUNC-05 | Voice modes Poly / Mono / Legato with glide | should | pending | stage-2 |
| FUNC-06 | File references persist in state; a missing file shows relink without crashing; factory sources resolve from the content folder | must | pending | stage-2 |
| FUNC-07 | Record-in: capture sidechain input into a target slot (manual / threshold / next-note trigger) | should | pending | stage-2 |
| FUNC-08 | Small factory source set plus presets that load out of the box | should | pending | stage-4 |
| FUNC-09 | Scale-quantised pitch spread, degree masks and the custom scale, as O-Granulator | must | partial | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Grain engine parity with O-Granulator streams (size, density, jitter, spray, window morph, reverse, stereo spread, gain rand) on file sources | must | complete | stage-2 |
| DSP-02 | Per-voice amp ADSR with velocity sensitivity; grains continue during release | must | partial | stage-2 |
| DSP-03 | Per-grain FX (SVF filter, drive, crush, ring) inherited | must | partial | stage-2 |
| DSP-04 | Feedback into per-stream working copies, with Freeze and Reset; voice-count-aware runaway guard | must | complete | stage-2 |
| DSP-05 | Output reverb/diffuser inherited | should | complete | stage-2 |
| DSP-06 | Mod matrix (16 slots): LFOs (global/per-voice), 2 mod ADSRs, velocity, key, note-random, mod wheel, aftertouch, MPE pressure/slide/bend, 4 macros | must | partial | stage-2 |
| DSP-07 | Files resampled to the session rate on load; band-limited reads when pitched up | must | partial | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | O-Granulator v2 visual language; per-stream waveform lanes with drag-drop, loop handles, scan playhead and grain dots | should | pending | stage-3 |
| UI-02 | Drag-to-modulate onto knobs; the MOD page shows instrument sources | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations/locks in processBlock, incl. voice steal and file swap) | must | partial | stage-2 |
| PERF-02 | Shared grain pool with a global cap and a defined CPU budget at 16 voices × 4 streams | must | partial | stage-2 |
| PERF-03 | File load and decode off the audio thread; atomic swap + retired-buffer reaper that works while the host idles | must | partial | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | complete | stage-1 |
| COMPAT-02 | MPE / VST3 note expression per the O-Lyrica reference | nice | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges (incl. voice steal, note-off, file swap, reset) | must | partial | stage-2 |
| QUAL-02 | Sidechain absent or unrouted (AU) degrades cleanly: record-in disabled, no crash or noise | nice | pending | stage-4 |

## Acceptance Criteria Details

### FUNC-01: Polyphonic grain clouds
- [ ] Holding a 4-note chord produces 4 independently pitched clouds
- [ ] Notes beyond the polyphony limit steal with a fade (no click above the κ/step gate threshold)

### FUNC-02: Per-stream file slots
- [ ] Each of the 4 slots loads WAV/AIFF/FLAC by drag-drop and by browser
- [ ] Loading while notes sound causes no click, dropout or allocation on the audio thread

### FUNC-03: Key-tracked pitch
- [ ] At the root note with keyTrack 100 %, the grain pitch equals the file pitch (±1 cent)
- [ ] keyTrack 0 % keeps the pitch fixed across all keys

### FUNC-04: Scan
- [ ] scanRate 100 % traverses the loop region in real time; 0 % holds still
- [ ] Retrigger restarts at `position` on each note; Free-run does not

### FUNC-06: File state
- [ ] Save/reopen restores all 4 files, loop regions and the scale state
- [ ] A missing file shows relink and the plugin stays stable

### FUNC-09: Scales
- [ ] Scale quantisation and degree masks behave identically to O-Granulator

### DSP-01: Engine parity
- [x] With 1 voice, a fixed key and scan 0, a stream renders equivalently to the O-Granulator stream on the same frozen material (stated tolerance)

### DSP-02: Amp envelope
- [ ] ADSR stages are audible and click-free; velocity scales level per velToAmp

### DSP-03: Per-grain FX
- [ ] Each FX audibly applies per grain; no per-grain allocation (2.3 verify: automated arm green — FV2–4, PS5–7, PF nulls, FA3, FB1–4, FZ1–2; "audibly" = HANDS-ON-2.3 rows 1–7, deferred post-Stage-3)

### DSP-04: Feedback
- [x] Feedback 100 % with 16 voices never exceeds the limiter ceiling or produces NaN (2.4 verify: FL1 0 bound violations, CL1 knee 1.965 ≤ 1.995, CL2 FZ1, FL6; runaway guard FL2)
- [x] Reset restores the pristine file; Freeze halts writes (2.4 verify: RS1/RS2 == pristine incl. while frozen, FL3 writes frozen)

### DSP-05: Reverb
- [x] Reverb inherited from O-Granulator (2.4 verify: PR1–12/PRk nulls ≤ −96.8 dB vs the parent; RV1–RV3 audible / tail fade / cold re-enable; listening confirmation = HANDS-ON-2.4 row 7, deferred)

### DSP-06: Modulation
- [ ] Each listed source routes to a continuous destination with bipolar depth (2.5a verify: every 2.5a source routes — SD1 17/17, DR1 125/125, parity PL/PM/PE ≤ −143.0 dB vs the parent; open: MPE per-member pressure/slide/bend = 2.5b)
- [x] Per-voice sources modulate each voice independently (2.5a verify: PV1 3 voices 0 ulp, VL1–VL3 phase reset + allocation-order invariance; feel = HANDS-ON-2.5a, deferred)

### DSP-07: Resampling / anti-aliasing
- [ ] A 44.1 k file in a 96 k session plays at the correct pitch
- [ ] +24 st grains show no aliasing above the gate threshold

### PERF-01 / PERF-03: RT safety
- [ ] Alloc gate: 0 allocations in processBlock across note on/off, steal, file swap and reset

### PERF-02: CPU budget
- [ ] The documented budget is met at 16 voices × 4 streams with the default grainLimit (2.4 verify: worst case C25w 26.59 % ≤ 40 % with chains + reverb; 2.5a verify: final worst case with 16 per-voice routes C25m 26.81 % ≤ 40 %, typical + 4 routes C25tm 6.37 % ≤ 12 %; open: typical on factory presets Stage 4)

### COMPAT-01: pluginval
- [x] pluginval strictness 10 passes for VST3 and AU

### QUAL-01: Artifacts
- [ ] Click gates pass for note-off, steal, file swap, reset and freeze toggle

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01–07, FUNC-09, DSP-*, PERF-*, QUAL-01 |
| stage-3 | UI-* |
| stage-4 | FUNC-08, COMPAT-02, QUAL-02, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Multi-sample pools / per-stream crossfade | Scope; one file per stream covers v1 | v1.1+ |
| Keys → position slicing | Different instrument model | v1.1+ |
| Record-in driving followers | Followers removed for v1 | v1.1+ |
| Formant-preserving scan | Research-heavy | v2 |

---
*Generated from BRIEF.md on 2026-11-06*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
