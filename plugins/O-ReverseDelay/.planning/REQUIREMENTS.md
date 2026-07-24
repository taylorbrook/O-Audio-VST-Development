# O-ReverseDelay - Requirements

---
version: 1.0.0
plugin: O-ReverseDelay
created: 2026-07-23
lastUpdated: 2026-07-23
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 14
**Coverage:** must: 9 | should: 4 | nice: 1

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Wet signal is a granular reverse smear: overlapping reversed grains, not chunked block reversal | must | pending | stage-2 |
| FUNC-02 | Delay time operates in Sync mode (host-tempo note divisions incl. dotted/triplet) and Free mode (ms) | must | pending | stage-2 |
| FUNC-03 | Feedback path regenerates the reverse tail through in-loop damping filters | must | pending | stage-2 |
| FUNC-04 | Stereo width control spreads grain placement across the stereo field | should | pending | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Grains are windowed (Hann or similar) and overlap per density setting — no clicks at grain boundaries | must | pending | stage-2 |
| DSP-02 | In-loop lowCut (20–2000 Hz) and highCut (500–20000 Hz) filters use RT-safe coefficient updates (ArrayCoefficients) | must | pending | stage-2 |
| DSP-03 | Feedback at 100% with damping engaged remains loop-stable (no runaway gain) | must | pending | stage-2 |
| DSP-04 | Time/frequency parameters use log-skewed NormalisableRange; presets authored in engineering units | should | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | All 10 parameters controllable from the plugin UI with value readouts from SliderState.getScaledValue() | should | pending | stage-3 |
| UI-02 | Sync/Free mode switch shows the relevant time control (noteDivision vs delayTime) | nice | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations/locks in processBlock) | must | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | complete | stage-1 |
| COMPAT-02 | Tempo sync degrades gracefully when host provides no BPM (falls back to free time) | must | pending | stage-2 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (clicks, NaN, zipper noise) at normal parameter ranges, including delay-time and mode changes | must | pending | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Granular reverse smear

**Description:** The wet path assembles overlapping reversed grains from a circular capture buffer, producing a continuous backwards smear rather than discrete reversed blocks.

**Acceptance Criteria:**
- [ ] Render harness: a transient input produces wet output whose envelope ramps up toward the transient's reverse position (reverse bloom), not a hard-gated block
- [ ] Grain playback direction is reversed (verified by autocorrelation/asymmetric-envelope probe on a single-grain render)

### FUNC-02: Sync + Free timing

**Acceptance Criteria:**
- [ ] In Sync mode at 120 BPM, 1/4 division yields 500 ms effective delay spacing (±1 block)
- [ ] In Free mode, delayTime knob sets spacing continuously 50–2000 ms

### FUNC-03: Damped feedback regeneration

**Acceptance Criteria:**
- [ ] Successive regenerations show measurable HF loss with highCut < 20 kHz and LF loss with lowCut > 20 Hz
- [ ] feedback = 0% produces exactly one generation of wet output

### DSP-01: Click-free grains

**Acceptance Criteria:**
- [ ] Offline render at default settings shows no discontinuities > -60 dBFS sample-to-sample step outside signal content
- [ ] Density sweep 0→100% during playback produces no clicks

### DSP-02: RT-safe damping filters

**Acceptance Criteria:**
- [ ] Filter coefficient updates use ArrayCoefficients assigned in place; no heap allocation on audio thread (code review + TSan/allocation guard)

### DSP-03: Loop stability

**Acceptance Criteria:**
- [ ] 60 s render at feedback 100%, default damping: output peak stays below 0 dBFS ceiling (soft-clip/energy cap engaged), no NaN/Inf

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [ ] No allocations, locks, or logging in processBlock (code review)
- [ ] pluginval strictness 10 passes 3 consecutive runs

### COMPAT-01: Validation

**Acceptance Criteria:**
- [x] auval passes (AU); pluginval passes VST3 and AU at strictness 10 *(verified 2026-07-23, stage-1)*

### COMPAT-02: No-tempo fallback

**Acceptance Criteria:**
- [ ] Render harness with no AudioPlayHead tempo: Sync mode falls back to free delayTime without crash or silence

### QUAL-01: Artifact-free

**Acceptance Criteria:**
- [ ] Automated sweeps of every parameter during playback produce no clicks, zipper noise, NaN, or Inf in output

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01..04, DSP-01..04, PERF-01, COMPAT-02, QUAL-01 |
| stage-3 | UI-01, UI-02 |
| stage-4 | COMPAT-*, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Pitch-shifted/shimmer feedback | Not selected during ideation; adds pitch-shifter complexity | v1.1+ |
| Chunked-reverse / mode switch | Granular smear chosen as the sole engine for v1.0 | v1.1+ |
| Freeze/infinite hold | Natural extension of granular engine, defer to keep v1.0 scoped | v1.1+ |

---
*Generated from BRIEF.md on 2026-07-23*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
