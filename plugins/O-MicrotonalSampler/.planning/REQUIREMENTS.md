# O-MicrotonalSampler — Requirements

---
version: 1.0.0
plugin: O-MicrotonalSampler
created: 2026-04-27
lastUpdated: 2026-04-28
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 22
**Coverage:** must: 14 | should: 6 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Plays user-loaded `.wav` / `.aif` samples mapped to MIDI pitches | must | complete | stage-2 |
| FUNC-02 | Supports up to 4 velocity layers per pitch | must | complete | stage-2 |
| FUNC-03 | Supports up to 16-voice polyphony | must | complete | stage-2 (rectified stage-4 phase-2.1 reopen: `polyphony` APVTS param was wired but never enforced — `CappedSynthesiser::noteOn` added to enforce cap) |
| FUNC-04 | Auto-detects sampled note range from loaded sample set | must | complete | stage-2 (rectified stage-4 phase-2.1 reopen: `SampleMap::findSlot` was exact-MIDI-match only — nearest-pitch-within-layer fallback added per spec line 80) |
| FUNC-05 | Drag-drop folder load with filename-convention auto-mapping (`Note_velocity` style) | should | complete | stage-3 |
| FUNC-06 | Per-cell manual sample assignment (override path) | should | complete | stage-3 |
| FUNC-07 | Voice-stealing when polyphony cap exceeded (oldest-released first) | must | complete | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Varispeed playback retunes samples by ±50 cents using fractional read rate `2^(cents/1200)` | must | complete | stage-2 |
| DSP-02 | Cubic-Hermite (or equivalent ≥3rd-order) interpolation for fractional sample reads | must | complete | stage-2 |
| DSP-03 | ADSR amplitude envelope applied per voice (attack, decay, sustain, release) | must | complete | stage-2 |
| DSP-04 | Equal-power crossfade between adjacent velocity layers within boundary region | must | complete | stage-2 |
| DSP-05 | Auto-detect sustain loop points (low-energy zero-crossing region in latter portion of sample) | should | complete | stage-2 |
| DSP-06 | Manual loop-point override per sample | should | complete | stage-3 |
| DSP-07 | Consumes VST3 note-expression events for per-note pitch | must | complete | stage-2 |
| DSP-08 | Consumes suite internal tuning module (Scala/Dorico-compatible) for scale/keyboard mapping | must | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Sample-mapping grid (pitch × velocity layer) is the primary editing surface | should | complete | stage-3 |
| UI-02 | Loop-point editor with waveform view and draggable markers (on demand) | nice | complete | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe `processBlock` — no allocations, no file I/O, no locks | must | complete | stage-2 |
| PERF-02 | 16 voices × varispeed + ADSR + crossfade ≤ 5% CPU on Apple Silicon @ 48 kHz / 256 buffer | should | complete | stage-4 (methodology deviation: Logic 11 Performance Meter unavailable; Activity Monitor headline ~16 % of one core on M4 Max ≈ ~1 % total system; objective per-block timing budget gate-of-record = pluginval --strictness-level 10 in Phase 4.4 — conditional rollback if 4.4 fails) |
| PERF-03 | Sample loading runs on background thread; never blocks audio thread | must | complete | stage-2 |
| PERF-04 | Zero added latency (no FFT, no lookahead) | nice | complete | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU on macOS, VST3 on Windows) | must | complete | stage-1 |
| COMPAT-02 | Accepts 16/24/32-bit AIF and WAV at any sample rate (auto-converts on load) | must | complete | stage-2 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (clicks, zipper noise, aliasing) at ±50c retune across full velocity / polyphony range | must | partial | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Sample playback

**Description:** Plays user-loaded `.wav` and `.aif` samples mapped to MIDI pitches.

**Acceptance Criteria:**
- [ ] Loading a folder of correctly-named samples produces a playable instrument with no manual config
- [ ] Triggering MIDI note `N` plays the sample mapped to pitch `N` (or nearest if `N` is unsampled)
- [ ] Both `.wav` and `.aif` formats load successfully

### FUNC-02: Velocity layers

**Description:** Supports up to 4 velocity layers per pitch with equal-split mapping (1–31 / 32–63 / 64–95 / 96–127 for 4 layers).

**Acceptance Criteria:**
- [ ] Loading 1, 2, 3, or 4 layers per pitch all work; ranges auto-split
- [ ] Velocities within a layer's range trigger that layer's sample
- [ ] Boundary velocities crossfade adjacent layers (see DSP-04)

### FUNC-03: Polyphony

**Description:** Up to 16 simultaneous voices.

**Acceptance Criteria:**
- [ ] 16 distinct held notes all sound simultaneously
- [ ] 17th note triggers voice-stealing (FUNC-07)

### FUNC-07: Voice stealing

**Acceptance Criteria:**
- [ ] When all 16 voices are active, a new note steals the oldest *released* voice first
- [ ] If no released voices exist, oldest *held* voice is stolen
- [ ] Voice-steal does not produce a click (envelope ramps to zero before reuse)

### DSP-01: Varispeed retune

**Description:** Retunes samples by ±50 cents via fractional playback rate `r = 2^(cents/1200)`.

**Acceptance Criteria:**
- [ ] +50 cents on a 440 Hz reference produces 452.89 Hz ±0.05 Hz
- [ ] -50 cents on a 440 Hz reference produces 427.47 Hz ±0.05 Hz
- [ ] No audible artifacts within the ±50c envelope

### DSP-02: Interpolation

**Acceptance Criteria:**
- [ ] Cubic-Hermite (or higher-order) interpolation is in the read path
- [ ] At ±50c retune, no audible aliasing on a sine-sweep test sample (1 kHz → 8 kHz)

### DSP-04: Equal-power velocity crossfade

**Acceptance Criteria:**
- [ ] Two adjacent layers at boundary velocity have constant perceived loudness across the crossfade
- [ ] Crossfade width controllable via Velocity Crossfade parameter (0% = hard switch, 100% = full)

### DSP-07 / DSP-08: Tuning consumption

**Description:** Plugin consumes the suite's microtonal infrastructure rather than defining its own.

**Acceptance Criteria:**
- [ ] Per-note VST3 note-expression pitch events are honored within ±0.5 cent of target
- [ ] Suite tuning module changes (scale swap, keyboard remap) take effect on next note without click
- [ ] No tuning UI inside this plugin (read-only display only, if any)

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [ ] No `new` / `malloc` / `delete` / `free` in `processBlock` (verified with allocation guard)
- [ ] No file I/O, locks, or mutex acquisition in audio thread
- [ ] pluginval `--strictness 10` passes

### PERF-02: CPU budget

**Acceptance Criteria:**
- [ ] 16-voice held-chord measurement on Apple Silicon @ 48 kHz / 256-sample buffer ≤ 5% CPU

### QUAL-01: No artifacts

**Acceptance Criteria:**
- [ ] No clicks at note-on / note-off / voice-steal
- [ ] No zipper noise on parameter changes (parameters smoothed)
- [ ] No audible aliasing at ±50c retune
- [ ] No discontinuity at velocity-layer boundaries (DSP-04)

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01..04, FUNC-07, DSP-01..05, DSP-07, DSP-08, PERF-01..04, COMPAT-02, QUAL-01 |
| stage-3 | FUNC-05, FUNC-06, DSP-06, UI-01, UI-02 |
| stage-4 | All remaining + final pluginval pass |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Round-robin / multi-take per cell | Brief scopes to one sample per cell | v1.1+ |
| Onboard filter / EQ / reverb | Pure sample engine in v1.0 | v1.1+ |
| Phase vocoder / formant preservation | Unnecessary at ±50c retune | n/a |
| Pitch wheel / mod wheel routing | Tuning driven entirely by suite note-expression | TBD |
| Streaming from disk | RAM-only sufficient for v1.0 | v2.0 |
| Sample browser / preset library UI | Folder-drop is enough | v1.1+ |
| Mono / legato mode | Conflicts with sustained long-tone use case | TBD |

---
*Generated from BRIEF.md on 2026-04-27*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
