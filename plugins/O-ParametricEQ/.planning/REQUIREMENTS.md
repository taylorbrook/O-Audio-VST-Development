# O-ParametricEQ - Requirements

---
version: 1.0.0
plugin: O-ParametricEQ
created: 2026-11-12
lastUpdated: 2026-11-12
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 27
**Coverage:** must: 18 | should: 7 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Up to 24 independent bands, created and removed on demand | must | pending | stage-2 |
| FUNC-02 | Eight filter types per band: Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch, Band Pass, Tilt Shelf | must | pending | stage-2 |
| FUNC-03 | Per-band placement: Stereo, Left, Right, Mid, Side | must | pending | stage-2 |
| FUNC-04 | Per-band bypass and global click-free bypass | must | pending | stage-2 |
| FUNC-05 | Output gain and gain scale | must | pending | stage-2 |
| FUNC-06 | Auto gain compensates the curve's loudness change | should | pending | stage-2 |
| FUNC-07 | Presets via the suite preset-manager, with a factory set | should | pending | stage-4 |
| FUNC-08 | Band solo / listen | should | pending | stage-3 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Analog-matched magnitude response for bell and shelf up to Nyquist, without oversampling | must | pending | stage-2 |
| DSP-02 | Zero latency, minimum phase | must | pending | stage-2 |
| DSP-03 | Slopes of 6, 12, 18, 24, 36, 48, 72 and 96 dB/oct on every type that supports them | must | pending | stage-2 |
| DSP-04 | Click-free under automation and fast drags of every parameter, including type, slope, placement and enable | must | pending | stage-2 |
| DSP-05 | Numerically stable and quiet at low frequency / high Q at all supported sample rates | must | pending | stage-2 |
| DSP-06 | Parameter ranges: frequency 10 Hz – 30 kHz, gain ±30 dB, Q 0.025 – 40 | must | pending | stage-2 |
| DSP-07 | Band engine and state format leave room for dynamic EQ without breaking v1 sessions | should | partial | stage-1 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Interactive response display: click to create a band, drag frequency / gain, scroll Q | must | pending | stage-3 |
| UI-02 | Displayed curve equals the true filter response at the session sample rate | must | pending | stage-3 |
| UI-03 | Real-time spectrum analyzer: Off / Pre / Post / Pre+Post | should | pending | stage-3 |
| UI-04 | Selected-band strip with typed value entry and double-click reset | should | pending | stage-3 |
| UI-05 | Ouaricon house style, French and zh-Hans localisation | should | pending | stage-4 |
| UI-06 | Output level meter | nice | pending | stage-3 |
| UI-07 | Per-band curve fills coloured by band | nice | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations or locks in processBlock) | must | pending | stage-2 |
| PERF-02 | 24 bands at 96 dB/oct stay within a CPU budget set in Stage 0; disabled bands cost nothing | must | pending | stage-4 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) and auval | must | complete | stage-1 |
| COMPAT-02 | Mono and stereo layouts; 44.1 – 192 kHz; macOS VST3 + AU, Windows VST3 | must | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges; all bands at 0 dB / disabled is bit-transparent | must | pending | stage-2 |

## Acceptance Criteria Details

### FUNC-01: 24 on-demand bands

**Acceptance Criteria:**
- [ ] 24 bands can be enabled simultaneously and each affects the output independently
- [ ] Enabling or removing a band mid-playback produces no click
- [ ] Band state survives save / reload and preset recall

### FUNC-02: Filter types

**Acceptance Criteria:**
- [ ] Each of the eight types renders a measured response matching its design target
- [ ] Changing type on a live band produces no click or level jump beyond the response change itself

### FUNC-03: Per-band placement

**Acceptance Criteria:**
- [ ] A Left band leaves the right channel bit-identical to input (and vice versa)
- [ ] A Mid band leaves the side signal unchanged (and vice versa), within numerical tolerance
- [ ] On a mono layout, placement is inert and the band processes the single channel

### FUNC-04: Bypass

**Acceptance Criteria:**
- [ ] Global bypass output nulls against input
- [ ] Toggling band or global bypass during a sustained tone produces no click

### FUNC-05: Output gain and gain scale

**Acceptance Criteria:**
- [ ] Output gain is accurate to ±0.05 dB across its range
- [ ] Gain scale 0 % yields a flat response for all gain-bearing bands; 200 % doubles their dB gain

### DSP-01: Analog-matched response

**Acceptance Criteria:**
- [ ] At 44.1 kHz, bell and shelf magnitude responses match the analog prototype within a tolerance set in Stage 0, measured up to 20 kHz, for centre frequencies up to 20 kHz
- [ ] No oversampling is used and reported latency stays 0

### DSP-02: Zero latency, minimum phase

**Acceptance Criteria:**
- [ ] `getLatencySamples()` returns 0 in every configuration
- [ ] An impulse through any configuration has no pre-ringing

### DSP-03: Slopes

**Acceptance Criteria:**
- [ ] Each slope setting measures its nominal dB/oct in the stop band for low cut and high cut
- [ ] Slope changes on a live band are click-free

### DSP-04: Click-free modulation

**Acceptance Criteria:**
- [ ] Full-range automation sweeps of frequency, gain and Q on a sustained low-frequency sine pass the suite click gate
- [ ] Stepped changes of type, slope, placement and enable pass the same gate
- [ ] No NaN / Inf and no instability under random parameter stress at all block sizes

### DSP-05: Stability and noise

**Acceptance Criteria:**
- [ ] A 10 Hz, Q 40 bell at 192 kHz stays stable and its noise floor stays below a threshold set in Stage 0
- [ ] Silence in gives silence out (no denormal CPU spikes, no residual tone)

### DSP-06: Parameter ranges

**Acceptance Criteria:**
- [ ] All ranges reachable from UI, automation and typed entry
- [ ] Frequencies above Nyquist at low sample rates are handled without instability

### UI-01: Interactive display

**Acceptance Criteria:**
- [ ] Clicking empty curve space creates a band at that frequency / gain
- [ ] Dragging a node updates frequency and gain with host automation gestures; scroll changes Q

### UI-02: True curve

**Acceptance Criteria:**
- [ ] Displayed summed curve matches the measured plugin response within 0.1 dB at every supported sample rate

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [ ] Allocation gate reports zero allocations in processBlock under parameter stress
- [ ] No locks or file / message-thread calls on the audio thread

### PERF-02: CPU budget

**Acceptance Criteria:**
- [ ] 24 bands at 96 dB/oct at 48 kHz measure within the Stage 0 budget
- [ ] CPU with zero enabled bands is near the pass-through baseline

### COMPAT-01: Validation

**Acceptance Criteria:**
- [x] pluginval strictness 10 passes for VST3 and AU
- [x] `auval` passes

### COMPAT-02: Layouts, rates, platforms

**Acceptance Criteria:**
- [ ] Mono and stereo instantiate and process correctly in Logic and at least one VST3 host
- [ ] Renders are correct at 44.1, 48, 88.2, 96 and 192 kHz
- [ ] Windows VST3 CI build passes pluginval

### QUAL-01: Transparency

**Acceptance Criteria:**
- [ ] With no bands enabled and output gain 0 dB, output is bit-identical to input
- [ ] No audible artifacts in listening tests across factory presets

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01, DSP-07 (state format and layout; the band-engine half is checked in Phase 2.2) |
| stage-2 | FUNC-01–06, DSP-01–06, PERF-01, QUAL-01 |
| stage-3 | FUNC-08, UI-01–04, UI-06, UI-07 |
| stage-4 | FUNC-07, UI-05, PERF-02, COMPAT-02, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Linear phase mode | v1 concentrates on a flawless zero-latency engine | v1.1+ |
| Natural phase mode | Same | v1.x+ |
| Dynamic EQ | Static first; engine and state reserve room | v1.x |
| Surround layouts | Mono / stereo first | v1.x+ |
| Brickwall slope | Beyond the 96 dB/oct essentials | v1.x+ |
| EQ match, spectral dynamics, external-sidechain analyzer | Flagship-parity features, not essentials | v2 |

---
*Generated from BRIEF.md on 2026-11-12*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
