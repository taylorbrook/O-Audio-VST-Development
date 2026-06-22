# O-simpleAdditive - Requirements

---
version: 1.0.0
plugin: O-simpleAdditive
created: 2026-06-22
lastUpdated: 2026-06-22
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 22
**Coverage:** must: 13 | should: 7 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Polyphonic additive synth: builds a pitched tone by summing 16 harmonic partials whose per-partial amplitudes are user-controlled (drawbars) | must | pending | stage-2 |
| FUNC-02 | Wavetable dimension: a scan/morph control interpolates the spectrum from Frame A toward a second target spectrum (Frame B) | must | pending | stage-2 |
| FUNC-03 | Scan position can be driven manually, by an LFO, and by a mod-envelope so timbre evolves across a held note | must | pending | stage-2 |
| FUNC-04 | Classic-waveform preset tour (sine / sawtooth / square / odd-harmonics-only) loadable as factory presets | should | pending | stage-2 |
| FUNC-05 | 16-voice polyphony with standard MIDI note on/off and amp-envelope voice lifetime | should | pending | stage-2 |
| FUNC-06 | Playable/musical enough to double as a simple additive instrument (organ-ish, hollow, evolving pad timbres) | nice | pending | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Sum-of-sines additive engine: 16 partials at integer multiples of the fundamental, per-partial amplitude, producing a single-cycle waveshape | must | pending | stage-2 |
| DSP-02 | Anti-aliasing: partials above Nyquist are band-limited/attenuated per note so high keys stay clean (no buzz/aliasing) | must | pending | stage-2 |
| DSP-03 | Frame A → Frame B spectral morph driven by scan position, smooth (no zipper noise) under modulation | must | pending | stage-2 |
| DSP-04 | Spectral-decay macro: one control makes higher partials decay faster than lower ones over the note, audibly and visibly | must | pending | stage-2 |
| DSP-05 | Bit-depth quantization (2–16 bits, plus clean/off) applied to the summed waveform for early-digital grit | should | pending | stage-2 |
| DSP-06 | Per-voice amplitude ADSR; mod-envelope ADSR routable to scan position | must | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | 16 harmonic drawbars as the dominant control surface; the bars double as the live spectrum display | must | pending | stage-3 |
| UI-02 | Live summed-waveform scope (time-domain) that redraws as drawbars/scan move, shown beside the spectrum | must | pending | stage-3 |
| UI-03 | On-hover pedagogical tooltips on every control (plain-language explanation + concrete example) | should | pending | stage-3 |
| UI-04 | Single-page, projector-readable, uncluttered layout consistent with O-simpleFM | should | pending | stage-3 |
| UI-05 | Preset tour selectable from the UI (sine/saw/square/odd-only) | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations/locks in processBlock); visualization data crosses to UI via lock-free handoff | must | pending | stage-2 |
| PERF-02 | 16-voice polyphony renders without dropouts at typical buffer sizes | should | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |
| COMPAT-02 | Windows WebView2 configured correctly (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`); UI renders on Windows | should | pending | stage-3 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (aliasing, zipper, clicks) at normal parameter ranges and across the keyboard | must | pending | stage-2 |
| QUAL-02 | Spectrum display and scope accurately reflect what is heard (visual matches audio) | should | pending | stage-3 |

## Acceptance Criteria Details

### FUNC-01: Additive partial-summing engine

**Description:** Build a pitched tone by summing 16 harmonic partials with user-controlled per-partial amplitudes.

**Acceptance Criteria:**
- [ ] Raising/lowering each of the 16 drawbars changes the corresponding harmonic's level audibly and in the spectrum display
- [ ] Fundamental-only yields a pure sine; all-harmonics-at-1/n approximates a sawtooth
- [ ] Tone is correctly pitched to incoming MIDI note number

### FUNC-02: Scan/morph wavetable dimension

**Description:** Scan interpolates the spectrum from Frame A toward Frame B.

**Acceptance Criteria:**
- [ ] At scan=0 the output equals Frame A; at scan=100% it equals Frame B
- [ ] Intermediate scan values produce a smooth spectral morph between the two
- [ ] The waveform scope visibly morphs as scan moves

### FUNC-03: Scan modulation sources

**Acceptance Criteria:**
- [ ] Manual scan knob moves the morph pointer
- [ ] LFO sweeps the scan position across a held note at the set rate/depth
- [ ] Mod-envelope moves the scan once per note by the set amount

### DSP-02: Anti-aliasing

**Acceptance Criteria:**
- [ ] Partials above Nyquist are removed/attenuated per note
- [ ] High notes (e.g. C7+) produce no audible aliasing with all 16 drawbars up

### DSP-04: Spectral-decay macro

**Acceptance Criteria:**
- [ ] At 0 the partial balance holds steady over the note; as it increases, higher partials decay faster than lower ones
- [ ] The effect is visible in the live spectrum (tilt over time) and audible (tone darkens over the note)

### DSP-05: Bit-depth quantization

**Acceptance Criteria:**
- [ ] Lowering bit depth introduces audible quantization grit
- [ ] At max/off the output is clean (no added quantization character)

### UI-01: Drawbar spectrum

**Acceptance Criteria:**
- [ ] 16 bars are the primary control surface and reflect current per-partial levels
- [ ] The same bars read as the live spectrum

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [ ] No allocations, locks, or file I/O in processBlock
- [ ] Visualization data reaches the UI via a lock-free FIFO/atomic handoff

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01/02/03/05, DSP-*, PERF-*, QUAL-01 |
| stage-3 | UI-*, COMPAT-02, QUAL-02 |
| stage-4 | FUNC-04/06, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Full per-partial envelopes (32+ controls) | Slider wall fights the pedagogy; spectral-decay macro stands in | v1.1+ |
| Multi-frame wavetable bank (>2 frames) | Keep v1.0 the clean A→B morph | v1.1+ |
| FFT analysis / resynthesis from recorded audio | Plugin demonstrates synthesis direction only | future |
| Effects (reverb/delay/chorus) | Keep signal path transparent for teaching | out |
| Non-sine partials / modulation matrix / formant filter | Out of pedagogical core | out |

---
*Generated from BRIEF.md on 2026-06-22*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
