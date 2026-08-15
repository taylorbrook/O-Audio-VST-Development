# O-Tapestop - Requirements

---
version: 1.0.0
plugin: O-Tapestop
created: 2026-08-15
lastUpdated: 2026-08-15
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 14
**Coverage:** must: 9 | should: 4 | nice: 1

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Engage param triggers spin-down; disengage triggers spin-up with resync to live audio | must | pending | stage-2 |
| FUNC-02 | Stop and Scratch modes selectable; Scratch plays the drawn speed envelope once per engage | must | pending | stage-2 |
| FUNC-03 | Stop/start times settable as tempo divisions (1/16–4 bars) or free ms (10 ms–8 s) via syncMode | must | pending | stage-2 |
| FUNC-04 | Output is silent while fully stopped and engaged | should | pending | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Varispeed playback (pitch+time coupled) over grain engine; no clicks across speed ramps or direction flips | must | pending | stage-2 |
| DSP-02 | stopCurve/startCurve morph ramp shape linear↔exponential; default matches x² tape physics | must | pending | stage-2 |
| DSP-03 | Spin-up uses fall-behind → accelerate → crossfade-skip resync; post-resync output is sample-aligned with dry | must | pending | stage-2 |
| DSP-04 | Bipolar scratch envelope supports reverse playback | should | pending | stage-2 |
| DSP-05 | toneTrack LPF tracks playback ratio with RT-safe coefficient updates | should | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Drawable speed-vs-time envelope editor for Scratch mode | should | pending | stage-3 |
| UI-02 | Prominent engage control usable as a live performance gesture | nice | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock) | must | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges; block-size invariant output | must | pending | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Engage trigger

**Description:** Automatable engage parameter drives the full stop/start lifecycle.

**Acceptance Criteria:**
- [ ] Host automation of engage produces spin-down then, on release, spin-up with resync
- [ ] Rapid engage/release mid-ramp reverses direction without clicks
- [ ] UI click on engage behaves identically to host automation

### FUNC-02: Modes

**Acceptance Criteria:**
- [ ] Stop mode uses stopTime/startTime + curves; Scratch mode plays scratchEnvelope over envLength
- [ ] Mode switch while disengaged never audibly disturbs the dry signal

### FUNC-03: Timing

**Acceptance Criteria:**
- [ ] Sync mode divisions track host tempo changes
- [ ] Free mode times match wall-clock within one block

### DSP-01: Click-free varispeed

**Acceptance Criteria:**
- [ ] Render-harness sweep of engage at multiple stopTimes shows no discontinuities above windowing floor
- [ ] 512-vs-4096 block renders are bit-identical (grain read latched after capture write)

### DSP-02: Curve control

**Acceptance Criteria:**
- [ ] Playback-ratio trace at 50% matches x² within tolerance; 0%/100% are audibly distinct linear/exponential shapes

### DSP-03: Resync

**Acceptance Criteria:**
- [ ] After spin-up completes, output equals the dry input (null test post-crossfade window)

### PERF-01 / COMPAT-01 / QUAL-01

**Acceptance Criteria:**
- [ ] No allocations/locks in processBlock (audit)
- [ ] pluginval strictness 10 passes VST3 + AU
- [ ] No NaN/Inf on pathological input (silence, DC, full-scale impulse) with sticky-state probes

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01..04, DSP-01..05, PERF-01, QUAL-01 |
| stage-3 | UI-01, UI-02 |
| stage-4 | COMPAT-*, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| MIDI note triggering | Engage param chosen as sole trigger for v1.0 simplicity | v1.1+ |
| XY/platter scratch control with inertia physics | Drawable envelope chosen for v1.0 | v1.1+ |
| Multiband / harmonic locking (full Path C) | Deliberately "Path C lite" | future plugin |
| Freeze-drone stop state | Silence chosen for v1.0 | v1.1+ |

---
*Generated from BRIEF.md on 2026-08-15*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
