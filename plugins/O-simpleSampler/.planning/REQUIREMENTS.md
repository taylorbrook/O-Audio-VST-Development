# O-simpleSampler - Requirements

---
version: 1.0.0
plugin: O-simpleSampler
created: 2026-06-25
lastUpdated: 2026-06-25
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 24
**Coverage:** must: 16 | should: 6 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Plays a buffered source recording polyphonically from MIDI, tuned to the keyboard relative to a Root Key | must | complete | stage-2 |
| FUNC-02 | Selectable curated built-in found-sounds embedded in the plugin for a frictionless demo | must | complete | stage-4 |
| FUNC-03 | Load-your-own audio via drag-drop onto the waveform and a file-picker fallback (the in-class "load one found sound" activity) | must | complete | stage-3 |
| FUNC-04 | Start/End controls isolate the played region of the recording | must | complete | stage-2 |
| FUNC-05 | Loop (off / forward / ping-pong) with crossfade sustains a short sound as a held note | must | complete | stage-2 |
| FUNC-06 | Reverse plays the region backwards | should | complete | stage-2 |
| FUNC-07 | Concept-isolating preset tour, each preset isolating one sampler move | should | complete | stage-3 |
| FUNC-08 | Tune (coarse semitones) and Fine (cents) transpose the sample independent of the keyboard | should | complete | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Pitch Mode toggle: Repitch (varispeed, pitch+time coupled) vs Stretch (pitch/time independent) — the headline pitch/time-independence lesson, audibly distinct | must | complete | stage-2 |
| DSP-02 | Fractional-read interpolation with anti-aliasing on upward transposition so high keys do not alias/buzz | must | complete | stage-2 |
| DSP-03 | Loop crossfade (equal-power) removes the per-loop seam click | must | complete | stage-2 |
| DSP-04 | Vintage macro: sample-rate decimation + bit-depth reduction (SP-1200 character); clean at zero | must | complete | stage-2 |
| DSP-05 | Resonant low-pass filter (cutoff + resonance) shapes the recording's tonal contour | must | complete | stage-2 |
| DSP-06 | Per-voice amp ADSR with velocity→amp routing | must | complete | stage-2 |
| DSP-07 | 16-voice polyphony with graceful voice-stealing | should | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Waveform editor with draggable start/end handles, shaded loop region with handles, live playhead, and root-key indication | must | complete | stage-3 |
| UI-02 | Repitch-vs-Stretch difference is made visible (playhead behaviour or live indicator) | should | complete | stage-3 |
| UI-03 | Live filter-response curve and animated amp-ADSR, sibling-consistent | should | complete | stage-3 |
| UI-04 | On-hover pedagogical tooltips on every control in plain language | nice | complete | stage-3 |
| UI-05 | Single-page, classroom/projector-readable layout consistent with the simple* siblings | nice | complete | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations or locks in processBlock; sample loading off the audio thread) | must | complete | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | complete | stage-4 |
| COMPAT-02 | Windows WebView2 flags set (NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1); second binary-data target uses a distinct NAMESPACE | must | complete | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges (no aliasing on high notes, no loop-seam clicks, no zipper on parameter moves) | must | complete | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Polyphonic keyboard playback tuned to a Root Key

**Description:** The instrument plays its source recording from incoming MIDI notes, polyphonically, with the sample tuned to the keyboard relative to a configurable Root Key (the key at which it plays at original pitch).

**Acceptance Criteria:**
- [ ] Playing the Root Key sounds the sample at its original pitch
- [ ] Notes above/below the Root Key transpose correctly (in Repitch mode, by varispeed)
- [ ] Multiple held notes sound simultaneously up to the voice limit

### FUNC-04: Start/End region isolation

**Description:** Start and End controls (and their waveform handles) isolate the useful region of the recording.

**Acceptance Criteria:**
- [ ] Moving Start/End changes the played region and is reflected on the waveform
- [ ] Playback begins at Start and ends/loops at End

### FUNC-05: Loop sustains a short sound

**Description:** Loop mode (off / forward / ping-pong) with a crossfade lets a short recording sustain indefinitely as a held note.

**Acceptance Criteria:**
- [ ] A sub-second sound held as a note sustains without dropping out when loop is on
- [ ] Loop region is editable and shaded on the waveform
- [ ] No audible click at the loop seam with crossfade applied

### DSP-01: Repitch vs Stretch (pitch/time independence)

**Description:** A Pitch Mode toggle switches between Repitch (varispeed: pitch and time coupled) and Stretch (pitch and duration independent), making pitch/time independence an audible A/B lesson.

**Acceptance Criteria:**
- [ ] In Repitch, a lower note audibly slows/lengthens the sound; a higher note speeds/shortens it
- [ ] In Stretch, a held note keeps its duration while pitch tracks the key
- [ ] The difference is obvious by toggling on a sustained sample

### DSP-04: Vintage (SP-1200) macro

**Description:** A single Vintage control applies sample-rate decimation and bit-depth reduction for the gritty low-fidelity character; fully clean at zero.

**Acceptance Criteria:**
- [ ] At 0% the output is bit-for-bit clean playback
- [ ] Increasing Vintage audibly adds quantization/aliasing grit
- [ ] No NaNs/denormals or runaway level across the range

### PERF-01: Real-time safe processing

**Description:** No heap allocations or locks occur in processBlock; sample loading happens off the audio thread.

**Acceptance Criteria:**
- [ ] Sanitizer/allocation check in processBlock is clean under continuous playback
- [ ] Loading a new sample does not glitch or block the audio thread

### UI-01: Waveform editor

**Description:** A full-width waveform editor is the primary surface, with draggable start/end and loop handles, a live playhead, and root-key indication.

**Acceptance Criteria:**
- [ ] Dragging handles updates Start/End/Loop parameters and vice-versa
- [ ] Playhead tracks the live read position during playback
- [ ] Loop region is visibly shaded

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01, COMPAT-02 |
| stage-2 | FUNC-01, FUNC-02, FUNC-04, FUNC-05, FUNC-06, FUNC-08, DSP-*, PERF-01, QUAL-01 |
| stage-3 | FUNC-03, FUNC-07, UI-* |
| stage-4 | COMPAT-*, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Slicing a break into hits / chop-to-pads | Keeps the one-irreducible-idea sibling discipline; wk05 lesson is the melodic keyboard sampler | Future "O-simpleSlicer" sibling |
| Internal step sequencer / pad grid / beat-making | Pure instrument played by the DAW's MIDI; no built-in sequencing | — |
| Multi-zone / velocity-layered multisampling | v1.0 is one recording tuned across the keys | v1.1+ |
| Effects (reverb/delay/chorus) | Keep signal path transparent for teaching | — |
| Tempo-sync / warp-to-grid | Stretch teaches independence; grid-lock is out of scope | v1.1+ |

---
*Generated from BRIEF.md on 2026-06-25*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
