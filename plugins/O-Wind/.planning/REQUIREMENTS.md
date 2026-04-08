# O-Wind - Requirements

---
version: 1.0.0
plugin: O-Wind
created: 2026-04-04
lastUpdated: 2026-04-05
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 25
**Coverage:** must: 13 | should: 8 | nice: 4

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Jet-drive excitation model (Verge 1995) with tanh saturation at labium | must | complete | stage-2 |
| FUNC-02 | Bidirectional bore waveguide with Thiran fractional delay | must | complete | stage-2 |
| FUNC-03 | Tier 1 tone holes: bore-length-switching with 2-5ms crossfade | must | complete | stage-2 |
| FUNC-04 | Breath pressure with nonlinear pressure^1.5 curve | must | complete | stage-2 |
| FUNC-05 | Embouchure control modulating jet delay ratio (0.3-0.6 of bore) | must | complete | stage-2 |
| FUNC-06 | Overblowing via jet velocity increase (register transitions) | must | complete | stage-2 |
| FUNC-07 | Turbulence noise injection scaled by jet velocity squared | must | complete | stage-2 |
| FUNC-08 | 4 core instrument presets (Concert Flute, Shakuhachi, Bansuri, Native American Flute) | must | complete | stage-2 |
| FUNC-09 | Pressure vibrato (NOT pitch vibrato) with rate 2-8 Hz | should | complete | stage-2 |
| FUNC-10 | DC blocker in waveguide loop | must | complete | stage-2 |
| FUNC-11 | Impossible physics: Infinite Sustain, Reversed Jet, Sub-Harmonics | should | complete | stage-2 |
| FUNC-12 | Tier 2 tone holes: Keefe 3-port scattering junctions (enhancement) | nice | complete | stage-2 |
| FUNC-13 | Bore end reflection filter (one-pole lowpass) and radiation filter (highpass) | must | complete | stage-2 |
| FUNC-14 | Viscothermal loss filter within bore waveguide | should | complete | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Jet delay line using Lagrange3rd interpolation (real-time modulatable) | must | complete | stage-2 |
| DSP-02 | Bore delay line using Thiran allpass interpolation | must | complete | stage-2 |
| DSP-03 | 2x oversampling for jet nonlinearity section | should | complete | stage-2 |
| DSP-04 | 8-voice max polyphony, 4-voice default | should | complete | stage-2 |
| DSP-05 | Stereo decorrelation (Width parameter, shared pattern with O-Bowed) | should | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | All parameters accessible and controllable via plugin GUI | should | complete | stage-3 |
| UI-02 | Instrument preset selector | should | complete | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock) | must | complete | stage-2 |
| PERF-02 | CPU per voice <2.5% (simple model) at 44.1kHz | nice | verified | stage-4 |
| PERF-03 | Oversampling latency correctly reported to host via setLatencySamples | nice | verified | stage-4 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation level 10 (VST3 and AU, DSP + GUI) | must | verified | stage-4 |
| COMPAT-02 | MIDI/MPE support: CC2 breath, CC74/Slide embouchure, pitch bend, aftertouch | should | complete | stage-2 |
| COMPAT-03 | Wind controller compatible (EWI, Aerophone, Sylphyo) via CC2 mapping | nice | verified | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges | must | complete | stage-2 |
| QUAL-02 | Stable oscillation startup and register transitions without clicks | should | complete | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Jet-Drive Excitation Model

**Description:** Implement the Verge jet-drive model with tanh saturation at the jet-labium interaction point. The jet reads bore feedback from the previous sample and injects energy through the nonlinear saturation function.

**Acceptance Criteria:**
- [ ] tanh saturation limits signal amplitude (no runaway oscillation)
- [ ] Jet responds to breath pressure changes within 5ms
- [ ] Model self-oscillates with nonzero breath pressure and bore feedback

### FUNC-02: Bidirectional Bore Waveguide

**Description:** Implement the bore as a bidirectional digital waveguide using JUCE DelayLine with Thiran interpolation for accurate fractional delay tuning.

**Acceptance Criteria:**
- [ ] Pitch tracks MIDI notes accurately across C4-C7 range (within 5 cents)
- [ ] Thiran interpolation provides flat amplitude response
- [ ] Bore delay recalculates correctly on sample rate change

### FUNC-03: Tier 1 Tone Holes

**Description:** MIDI note maps to effective bore delay length. Crossfade over 2-5ms on fingering change to prevent clicks.

**Acceptance Criteria:**
- [ ] Note changes produce correct pitch within 5ms
- [ ] No audible clicks during fast note transitions (trills)
- [ ] Legato and staccato articulations both work cleanly

### FUNC-04: Breath Pressure Curve

**Description:** Breath pressure maps through a nonlinear pressure^1.5 curve for natural dynamics feel.

**Acceptance Criteria:**
- [ ] Low breath values produce airy/breathy tone
- [ ] High breath values produce full, projected tone
- [ ] CC2 and aftertouch both control breath pressure

### FUNC-05: Embouchure Control

**Description:** Embouchure parameter modulates jet delay ratio between 0.3-0.6 of bore delay, controlling register and timbre.

**Acceptance Criteria:**
- [ ] Low embouchure = focused/bright tone
- [ ] High embouchure = spread/dark tone
- [ ] MPE Y (CC74/Slide) controls embouchure in real-time

### FUNC-06: Overblowing

**Description:** Increasing jet velocity (via breath pressure + embouchure) causes the model to lock onto higher harmonics (octave, twelfth).

**Acceptance Criteria:**
- [ ] Model transitions from fundamental to 2nd harmonic with increased pressure
- [ ] Hysteresis prevents unstable register flickering
- [ ] Register transitions are musically usable (not random)

### FUNC-07: Turbulence Noise

**Description:** Band-limited noise injection at the embouchure, amplitude scaled by jet velocity squared.

**Acceptance Criteria:**
- [ ] Louder playing produces proportionally more noise (quadratic scaling)
- [ ] Noise spectrum shaped by low-pass filter (~2-6 kHz range)
- [ ] Breath Noise parameter controls overall noise gain

### FUNC-08: Core Instrument Presets

**Description:** Four pre-tuned instrument parameter sets that capture the distinct character of each flute type.

**Acceptance Criteria:**
- [ ] Concert Flute: Clear, projecting, moderate breathiness
- [ ] Shakuhachi: Breathy, wide embouchure range, end-blown character
- [ ] Bansuri: Bamboo warmth, smooth pitch slides
- [ ] Native American Flute: Very breathy, meditative, fixed embouchure

### FUNC-10: DC Blocker

**Description:** DC blocking filter inside the waveguide feedback loop to prevent DC accumulation.

**Acceptance Criteria:**
- [ ] No DC offset in output signal after sustained playing
- [ ] Cutoff low enough to be inaudible (~7 Hz at 44.1kHz)

### FUNC-13: Bore End Reflection and Radiation Filters

**Description:** One-pole lowpass for bore end reflection (frequency-dependent reflection) and highpass radiation filter for output.

**Acceptance Criteria:**
- [ ] High frequencies radiate more (realistic radiation characteristic)
- [ ] Reflection filter provides appropriate bore feedback spectrum

### PERF-01: Real-Time Safe Processing

**Description:** No memory allocations, locks, or blocking operations in the audio callback.

**Acceptance Criteria:**
- [ ] processBlock contains no new/delete/malloc/free calls
- [ ] No mutex locks in audio thread
- [ ] No file I/O or string operations in audio thread

### COMPAT-01: Pluginval Validation

**Description:** Plugin passes pluginval at strictness level 10 for both VST3 and AU formats.

**Acceptance Criteria:**
- [ ] pluginval --strictness-level 10 passes for VST3
- [ ] pluginval --strictness-level 10 passes for AU
- [ ] No crashes during rapid parameter automation

### QUAL-01: No Audio Artifacts

**Description:** Clean audio output across normal parameter ranges without clicks, pops, denormals, or instability.

**Acceptance Criteria:**
- [ ] No clicks during note on/off transitions
- [ ] No denormal stalls (flush-to-zero enabled)
- [ ] Stable output at extreme parameter combinations

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 (level 5) |
| stage-2 | FUNC-*, DSP-*, PERF-01, QUAL-* |
| stage-3 | UI-* |
| stage-4 | COMPAT-01 (level 10 DSP+GUI), COMPAT-03, PERF-02, PERF-03 |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Tier 2 tone holes (Keefe scattering) | Enhancement tier, not core | v1.1+ |
| Expansion presets (Recorder, Pan Flute, Piccolo, Ocarina) | Post-launch content | v1.1+ |
| Scala/MTS-ESP tuning integration | Shared module not yet extracted | v1.1+ |
| WebView UI with model visualization | UI design phase separate | v1.0 (stage-3) |

---
*Generated from BRIEF.md on 2026-04-04*
