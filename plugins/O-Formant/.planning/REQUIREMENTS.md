# O-Formant - Requirements

---
version: 1.0.0
plugin: O-Formant
created: 2026-04-04
lastUpdated: 2026-04-04
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 26
**Coverage:** must: 14 | should: 8 | nice: 4

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Generate voice from LF glottal pulse model with Rd voice quality control (0.3-2.7) | must | pending | stage-2 |
| FUNC-02 | 5-formant parallel bandpass filter bank modeling vocal tract | must | pending | stage-2 |
| FUNC-03 | 2D XY vowel morph pad with 5 cardinal vowels at acoustic positions | must | pending | stage-2 |
| FUNC-04 | Shepard interpolation (p=2.5 default) with log-frequency domain blending | must | pending | stage-2 |
| FUNC-05 | Consonant noise injection via KLATT dual-branch parallel topology | must | pending | stage-2 |
| FUNC-06 | ADSR amplitude envelope per voice | must | pending | stage-2 |
| FUNC-07 | MPE support: pressure->breathiness, slide->vowel Y, velocity->attack character | should | pending | stage-2 |
| FUNC-08 | Legacy MIDI mode fallback via enableLegacyMode() | should | pending | stage-2 |
| FUNC-09 | Auto-consonant: plosive burst (10-25ms) on note-on with crossfade into vowel | should | pending | stage-2 |
| FUNC-10 | Vibrato LFO with rate, depth, and onset delay parameters | should | pending | stage-2 |
| FUNC-11 | Portamento/pitch glide between notes (0-1000ms) | nice | pending | stage-2 |
| FUNC-12 | Genre-based factory preset packs (cinematic, electronic, ambient, speech) | nice | pending | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | LF glottal source with Fant 1995 Rd-to-R-parameter regression | must | pending | stage-2 |
| DSP-02 | Custom biquad bandpass filters for formant bank (cache-local structs) | must | pending | stage-2 |
| DSP-03 | Block-rate coefficient updates every 32 samples | must | pending | stage-2 |
| DSP-04 | Aspiration noise mix controlled by breathiness parameter | must | pending | stage-2 |
| DSP-05 | Formant shift (semitones) and formant spread (spacing multiplier) | should | pending | stage-2 |
| DSP-06 | Two-layer smoothing: XY position (~30ms) + formant parameters (~20ms) | should | pending | stage-2 |
| DSP-07 | Anti-aliasing for glottal source (PolyBLEP minimum, mipmapped wavetable preferred) | should | pending | stage-2 |
| DSP-08 | Consonant tone shaping (dark/bright filter) and sibilance emphasis | should | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | 2D XY vowel morph pad with draggable cursor and vowel labels | must | pending | stage-3 |
| UI-02 | Real-time formant peaks overlay (F1-F5 frequency bars) on XY pad | nice | pending | stage-3 |
| UI-03 | Organized parameter layout: Glottal, Consonant, Character groups | nice | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock) | must | pending | stage-2 |
| PERF-02 | 16-voice polyphony at <5% single core CPU at 48kHz | must | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges | must | pending | stage-2 |

## Acceptance Criteria Details

### FUNC-01: LF Glottal Pulse Model

**Description:** Generate glottal excitation using the Liljencrants-Fant model with musical Rd control spanning pressed (0.3) through modal (1.0) to breathy (2.7).

**Acceptance Criteria:**
- [ ] Rd=1.0 produces recognizable modal voice quality when fed through formant bank
- [ ] Rd sweep from 0.3 to 2.7 produces continuous timbral change (no discontinuities)
- [ ] Glottal source at extreme Rd values (<0.3 or >2.7) remains stable (no NaN/inf)

### FUNC-02: 5-Formant Parallel Bandpass Bank

**Description:** Five parallel bandpass filters with independent frequency, bandwidth, and gain, modeling the human vocal tract resonances F1-F5.

**Acceptance Criteria:**
- [ ] Each of the 5 formants independently filters the source signal
- [ ] Formant frequencies match expected values for cardinal vowels (within 5% of research targets)
- [ ] Filter output is stable across full pitch range (C0-C8)

### FUNC-03: 2D XY Vowel Morph Pad

**Description:** XY control surface mapping 5 cardinal vowels (A, E, I, O, U) at their normalized acoustic positions with smooth interpolation between all points.

**Acceptance Criteria:**
- [ ] Moving cursor to each vowel position produces distinct, recognizable vowel timbre
- [ ] Smooth timbral transitions when moving between any two vowel positions
- [ ] Vowel positions match research coordinates (I=0.00,1.00 / E=0.31,0.43 / A=0.83,0.00 / O=1.00,0.35 / U=0.98,0.93)

### FUNC-04: Shepard Interpolation

**Description:** Modified inverse-distance weighting with configurable power parameter for vowel focus control.

**Acceptance Criteria:**
- [ ] Focus=1.0 produces washy blend of all vowels
- [ ] Focus=2.5 (default) produces natural smooth transitions
- [ ] Focus=6.0 snaps clearly to nearest vowel

### FUNC-05: Consonant Noise Injection

**Description:** Parallel noise branch with musical controls for fricatives, plosives, and sibilants following KLATT dual-branch topology.

**Acceptance Criteria:**
- [ ] Consonant Level at 0% produces no noise; at 100% produces clearly audible consonant character
- [ ] Consonant Tone sweeps from dark (/f/-like) to bright (/s/-like)
- [ ] Noise branch does not interfere with vowel formant clarity when both are active

### FUNC-06: ADSR Envelope

**Description:** Per-voice amplitude envelope with attack, decay, sustain, and release.

**Acceptance Criteria:**
- [ ] Note-on triggers attack/decay/sustain; note-off triggers release
- [ ] Envelope parameters respond to changes in real-time without clicks
- [ ] Fast attack (1ms) produces clean transient without pop

### DSP-01: LF Model Implementation

**Description:** Fant 1995 regression formulas converting Rd to derived R-parameters (Ra, Rk, Rg) for glottal pulse shape.

**Acceptance Criteria:**
- [ ] R-parameter values match Fant 1995 regression within 1% for Rd range 0.3-2.7
- [ ] Glottal pulse waveform visually matches published LF model plots at Rd=0.5, 1.0, 2.0

### DSP-02: Custom Biquad Formant Filters

**Description:** Cache-local biquad structs for formant filtering without JUCE ProcessorState overhead.

**Acceptance Criteria:**
- [ ] Filter coefficients produce correct bandpass response at all 5 formant presets
- [ ] No state corruption when processing 16 simultaneous voices

### PERF-01: Real-Time Safety

**Description:** No memory allocations, locks, or blocking operations in the audio callback.

**Acceptance Criteria:**
- [ ] processBlock contains zero heap allocations (verified via sanitizer or code review)
- [ ] No mutex locks in audio thread path

### PERF-02: CPU Budget

**Description:** 16 voices of full synthesis chain within acceptable CPU budget.

**Acceptance Criteria:**
- [ ] 16-voice polyphony at 48kHz uses <5% of single CPU core
- [ ] No audio dropouts at 256-sample buffer size with 16 voices

### COMPAT-01: Pluginval Validation

**Description:** Plugin passes automated validation for both VST3 and AU formats.

**Acceptance Criteria:**
- [ ] pluginval level 5 passes for VST3
- [ ] pluginval level 5 passes for AU
- [ ] No crashes during parameter randomization test

### QUAL-01: Audio Quality

**Description:** Clean audio output without artifacts at normal operating ranges.

**Acceptance Criteria:**
- [ ] No zipper noise during parameter changes (vowel morph, Rd sweep)
- [ ] No clicks or pops during voice allocation/stealing
- [ ] No aliasing audible below C5 at 44.1kHz sample rate

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01 through FUNC-11, DSP-01 through DSP-08, PERF-01, PERF-02, QUAL-01 |
| stage-3 | UI-01, UI-02, UI-03 |
| stage-4 | FUNC-12, COMPAT-01 (re-verify), all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Built-in reverb/chorus | User preference to add effects separately | v1.1+ |
| Cascade formant topology | Parallel sufficient for v1 | v2 |
| Formant sync mode | Creative extension, not core | v2 |
| Subharmonic/growl synthesis | Creative extension | v1.1+ |
| Nasal anti-formants | Requires cascade topology | v2 |

---
*Generated from BRIEF.md on 2026-04-04*
