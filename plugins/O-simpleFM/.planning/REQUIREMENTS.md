# O-simpleFM - Requirements

---
version: 1.0.0
plugin: O-simpleFM
created: 2026-06-20
lastUpdated: 2026-06-20
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 22
**Coverage:** must: 14 | should: 6 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | 2-operator FM voice (one carrier + one modulator) that produces audible FM timbres from MIDI input | must | pending | stage-2 |
| FUNC-02 | Polyphonic, MIDI-keyboard playable (voice count confirmed in Stage 0) | must | pending | stage-2 |
| FUNC-03 | Modulator self-feedback path that audibly enriches/roughens the spectrum as it increases | must | pending | stage-2 |
| FUNC-04 | Independent modulator ADSR envelope routed to modulation index so timbre evolves over a note | must | pending | stage-2 |
| FUNC-05 | Independent amplitude ADSR envelope on the carrier output | must | pending | stage-2 |
| FUNC-06 | Educational preset tour: named presets that each isolate one FM concept (e.g. bell/inharmonic, brass/index-env) | should | pending | stage-3 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | C:M ratio control changes modulator frequency relative to carrier; integer vs non-integer audibly maps to harmonic vs inharmonic | must | pending | stage-2 |
| DSP-02 | Modulation index/depth control changes sideband richness/brightness across its full range without artifacts | must | pending | stage-2 |
| DSP-03 | Sine-based phase-modulation operators with stable feedback (one-sample-delay averaging) | must | pending | stage-2 |
| DSP-04 | Selectable carrier and modulator waveforms (sine default) | should | pending | stage-2 |
| DSP-05 | Optional integer-snap on ratio for clean harmonic/inharmonic comparison | should | pending | stage-2 |
| DSP-06 | Additional standard FM parameters surfaced by Stage 0 research integrated (e.g. velocity→index, mod-env→index amount, output level) | should | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Live spectrum analyzer showing sidebands appear/multiply in real time as index/ratio/feedback change | must | pending | stage-3 |
| UI-02 | Live waveform/oscilloscope showing the output waveform morphing in real time | must | pending | stage-3 |
| UI-03 | Live operator routing diagram reflecting carrier/modulator + feedback signal flow | should | pending | stage-3 |
| UI-04 | On-hover pedagogical tooltips on every parameter with plain-language explanation + concrete example | must | pending | stage-3 |
| UI-05 | Single-page, uncluttered, classroom-readable layout (no deep menus) | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock; lock-free FIFO feeds visualizations) | must | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |
| COMPAT-02 | Windows WebView2 configured (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) so UI renders cross-platform | must | pending | stage-1 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (clicks/aliasing/instability) across normal parameter ranges, including high index/feedback | must | pending | stage-2 |

## Acceptance Criteria Details

### FUNC-01: 2-operator FM voice
**Description:** A single carrier modulated by a single modulator produces audible FM timbres from MIDI.
**Acceptance Criteria:**
- [ ] Playing a MIDI note with index > 0 produces audibly different timbre than index = 0
- [ ] Carrier pitch tracks MIDI note number across the keyboard
- [ ] Voice silences correctly on note-off after release

### FUNC-03: Modulator feedback
**Description:** Self-feedback on the modulator enriches the spectrum.
**Acceptance Criteria:**
- [ ] Increasing feedback from 0→max audibly adds harmonics/roughness
- [ ] Feedback path remains stable (no runaway/NaN) at maximum setting

### FUNC-04: Modulator envelope → index
**Description:** Dedicated modulator ADSR shapes index over the note.
**Acceptance Criteria:**
- [ ] A bright-attack/mellow-sustain envelope is clearly audible as timbre change over time
- [ ] Modulator envelope is independent of the amplitude envelope

### DSP-01: C:M ratio
**Description:** Ratio maps to harmonic vs inharmonic timbre.
**Acceptance Criteria:**
- [ ] Integer ratios (1:1, 2:1, 3:1) produce harmonic spectra
- [ ] Non-integer ratios (e.g. 1.41:1) produce inharmonic/bell-like spectra
- [ ] Ratio change is reflected live in the spectrum analyzer

### DSP-02: Modulation index
**Description:** Index controls sideband richness without artifacts.
**Acceptance Criteria:**
- [ ] Raising index visibly adds sideband pairs in the spectrum analyzer
- [ ] No aliasing/zipper noise when sweeping index in real time

### UI-01: Live spectrum analyzer
**Description:** Real-time FFT clearly shows FM sidebands.
**Acceptance Criteria:**
- [ ] Discrete sidebands are individually visible at low/moderate index
- [ ] Display updates smoothly while parameters are swept
- [ ] Visualization is fed by a lock-free path (no audio-thread allocation)

### UI-04: Pedagogical tooltips
**Description:** Every parameter explains itself on hover.
**Acceptance Criteria:**
- [ ] Hovering any parameter shows a short plain-language description
- [ ] At least the core params (ratio, index, feedback, envelopes) include a concrete example

### COMPAT-02: Windows WebView2
**Description:** Cross-platform WebView rendering.
**Acceptance Criteria:**
- [ ] Both `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` are set
- [ ] UI renders (not blank) on Windows VST3

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01, COMPAT-02 |
| stage-2 | FUNC-01..05, DSP-*, PERF-01, QUAL-01 |
| stage-3 | UI-*, FUNC-06 |
| stage-4 | COMPAT-*, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| 4-op/6-op + algorithms | Dilutes pedagogical focus on FM fundamentals | future "O-FM" sibling |
| Built-in effects (reverb/delay) | Keep signal path transparent for teaching | v1.1+ |
| Modulation matrix / LFO networks | Beyond "essential common parameters" scope | v1.1+ |
| A/B compare snapshot | Considered, deferred | v1.1+ |

---
*Generated from BRIEF.md on 2026-06-20*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
