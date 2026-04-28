# O-Bassoon - Requirements

---
version: 1.0.2
plugin: O-Bassoon
created: 2026-04-27
lastUpdated: 2026-04-27 (Phase 2.1 verify ✅ VERIFIED — PERF-01 + FUNC-03 complete; FUNC-01/04, DSP-01, QUAL-01/02 partial)
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 18
**Coverage:** must: 12 | should: 5 | nice: 1

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Plays sustained bassoon-like tones via modal synthesis | must | partial | stage-2 |
| FUNC-02 | Polyphonic playback with 1-16 voice cap (default 8) | must | pending | stage-2 |
| FUNC-03 | Extended pitch range C1-C6 supported | must | complete | stage-2 |
| FUNC-04 | Long-tone-friendly amplitude envelope (attack, sustain, release) | must | partial | stage-2 |
| FUNC-05 | Voice stealing when polyphony exceeded (oldest-note priority) | should | pending | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Modal-synthesis voice: bank of damped resonators tuned to bassoon spectrum | must | partial | stage-2 |
| DSP-02 | Vibrato: rate 0-10 Hz, depth 0-100 cents, onset delay 0-2000 ms | must | pending | stage-2 |
| DSP-03 | Tone / brightness control via modal damping (0-1 normalized) | must | pending | stage-2 |
| DSP-04 | Breath / dynamics control reads CC2 and velocity, scales loudness 0-1 | must | pending | stage-2 |
| DSP-05 | Attack-character control morphs onset between soft pad and tongued articulation | should | pending | stage-2 |
| DSP-06 | Microtonal pitch: VST3 Note Expression (pitch ID 0x00000003) + MPE channel pitch-bend | must | pending | stage-2 |
| DSP-07 | Does NOT depend on or reuse code from O-Reed | must | complete | stage-1 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | UI exposes all v1.0 parameters with Ouaricon family visual language | should | pending | stage-3 |
| UI-02 | UI mockup designed and approved before Stage 3 implementation | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe processing (no allocations in processBlock) | must | complete | stage-2 |
| PERF-02 | 8-voice polyphony at 48 kHz / 256 buffer stays under 25% CPU on M-series Mac | should | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU on macOS, VST3 on Windows) | must | partial | stage-1 |
| COMPAT-02 | Dorico microtonal playback via Note Expression, on par with O-Lyrica | must | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (clicks, NaN/inf, aliasing) at normal parameter ranges | must | partial | stage-2 |
| QUAL-02 | Stable long-tone behavior — no drift, runaway resonance, or denormal slowdown over 60-second hold | nice | partial | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Sustained bassoon-like tones via modal synthesis

**Description:** Voice produces a recognizable bassoon-character timbre using a modal synthesis algorithm (sum of damped resonators), suitable for sustained playback.

**Acceptance Criteria:**
- [ ] A held note produces a stable spectrum identifiable as bassoon-like (verified by spectral inspection vs. real bassoon recording)
- [ ] Note can sustain indefinitely without amplitude drift or instability
- [ ] No reed self-oscillation behavior present (excitation is impulse + filtered noise only)

### FUNC-02: Polyphonic playback

**Description:** 1-16 voices selectable; default 8.

**Acceptance Criteria:**
- [ ] Playing 8 simultaneous notes produces 8 distinct voices
- [ ] Voice count parameter changes take effect on next note-on
- [ ] No crashes or stuck notes when exceeding voice cap

### FUNC-03: Extended pitch range C1-C6

**Description:** Plays correctly across C1-C6 MIDI range.

**Acceptance Criteria:**
- [ ] All notes C1-C6 sound at correct pitch
- [ ] Modal bank reconfigures correctly across the range (no detuned resonators at extremes)

### FUNC-04: Long-tone amplitude envelope

**Description:** Attack/sustain/release envelope tuned for long-tone use; release does not click.

**Acceptance Criteria:**
- [ ] Attack time parameter responds 0-2000 ms
- [ ] Release time parameter responds 0-3000 ms
- [ ] No audible click at note-off across release range

### DSP-01: Modal-synthesis voice

**Description:** Each voice is a bank of damped resonators (parallel 2nd-order modal filters) tuned to bassoon spectral data.

**Acceptance Criteria:**
- [ ] Voice implementation contains a documented mode bank (frequency, gain, decay per mode)
- [ ] Mode bank is pre-allocated in prepareToPlay (no allocations in processBlock)

### DSP-02: Vibrato

**Acceptance Criteria:**
- [ ] Rate parameter 0-10 Hz, audibly modulates pitch at correct rate
- [ ] Depth parameter 0-100 cents, measurable by tuner
- [ ] Onset parameter 0-2000 ms, vibrato fades in after delay

### DSP-06: Microtonal pitch

**Description:** Per-note pitch via VST3 Note Expression (Dorico) and MPE channel pitch-bend (DAWs).

**Acceptance Criteria:**
- [ ] Note Expression pitch ID 0x00000003 received and applied per-note
- [ ] MPE channel pitch-bend applied per-note in MPE mode
- [ ] Dorico microtonal score plays at correct microtonal pitches (manual verification)

### DSP-07: No O-Reed dependency

**Acceptance Criteria:**
- [ ] CMakeLists.txt has no reference to O-Reed sources
- [ ] No `#include` references to O-Reed headers
- [ ] Plugin builds and runs with O-Reed deleted from the workspace

### COMPAT-01: pluginval

**Acceptance Criteria:**
- [ ] pluginval --strictness 10 passes for VST3 (macOS, Windows)
- [ ] pluginval --strictness 10 passes for AU (macOS)

### COMPAT-02: Dorico microtonal playback

**Acceptance Criteria:**
- [ ] Loaded into Dorico via Playback Template, plays a microtonal test score at correct pitches
- [ ] Behavior matches O-Lyrica's Dorico playback parity test

### PERF-01: Real-time safe

**Acceptance Criteria:**
- [ ] No `new`, `malloc`, `std::vector::push_back`, or other allocating calls in processBlock or audio-thread paths
- [ ] No locks or blocking calls in processBlock

### QUAL-01: No artifacts

**Acceptance Criteria:**
- [ ] No NaN/inf in output across full parameter sweep
- [ ] No clicks at note-on / note-off / parameter change
- [ ] No aliasing at high-pitch / extreme-vibrato combinations

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01, DSP-07 |
| stage-2 | FUNC-*, DSP-01..06, PERF-*, QUAL-* |
| stage-3 | UI-* |
| stage-4 | COMPAT-02, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Reed self-oscillation modeling | Explicitly out (avoid O-Reed complexity) | Possibly v2.0 if revisited |
| Key-click sample layer | Long-tone focus, not articulation focus | v1.1+ |
| Preset browser | Defer to post-1.0 polish | v1.1 |
| Aftertouch → vibrato modulation | Decide in Stage 0 if confirmed | v1.0 stretch or v1.1 |

---
*Generated from BRIEF.md on 2026-04-27*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
