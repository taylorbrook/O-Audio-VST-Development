# O-simpleAmbisonics - Requirements

---
version: 1.0.0
plugin: O-simpleAmbisonics
created: 2026-10-11
lastUpdated: 2026-10-11
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 27
**Coverage:** must: 16 | should: 9 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Complete ambisonic chain inside one stereo-out plugin: mono source → SH encoder → SH-domain rotation → decoder → L/R; no multichannel bus required | must | pending | stage-2 |
| FUNC-02 | Source positioned by Azimuth (-180…+180°), Elevation (-90…+90°) and Distance (0.2–20 m) | must | pending | stage-2 |
| FUNC-03 | Ambisonic order selectable 1 / 2 / 3 (4 / 9 / 16 channels), switchable while audio runs | must | pending | stage-2 |
| FUNC-04 | Scene rotation via Yaw (must), Pitch and Roll (should), applied in the ambisonic domain after encoding so the scene rotates, not the source | must | pending | stage-2 |
| FUNC-05 | Decoder selectable: Binaural (HRTF, headphones) or Stereo (plain amplitude decode for speakers/projector) | must | pending | stage-2 |
| FUNC-06 | Orbit: automatic azimuth motion with an on/off switch and a rate (0.02–2 Hz) so movement is audible without DAW automation | should | pending | stage-2 |
| FUNC-07 | Stereo input accepted and summed to the single mono source (Input Mode: Mono sum / Left only); mono input accepted | must | pending | stage-1 |
| FUNC-08 | Concept-preset tour (Point Source FOA, Order 1→2→3, Behind You, Overhead, Rotate the Room, Far Away / Up Close, Slow Orbit, Speakers vs Headphones) loadable as factory presets | should | pending | stage-4 |
| FUNC-09 | Usable as a lightweight creative binaural panner with orbit motion for headphone mixes | nice | pending | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | SH encoder in AmbiX convention (ACN order, SN3D normalisation) for orders 1–3; coefficients match the reference table in `research/ambisonics-encoding-deep-dive.md` within 1e-5 | must | pending | stage-2 |
| DSP-02 | SH-domain rotation matrix per order (yaw/pitch/roll), mathematically equivalent to re-encoding the source at the rotated direction | must | pending | stage-2 |
| DSP-03 | HRTF binaural decoder (MagLS or virtual-loudspeaker + HRTF; Stage 0 decides) from an embedded, precomputed filter set per order; partitioned convolution; latency reported via `setLatencySamples()` | must | pending | stage-2 |
| DSP-04 | Stereo decode mode (±30° virtual pair or equivalent) without HRTF | should | pending | stage-2 |
| DSP-05 | Distance model: inverse-distance gain with near-field clamp plus a gentle air-absorption low-pass; monotonic and explainable | should | pending | stage-2 |
| DSP-06 | Angle parameters smoothed along the shortest path around ±180° (no long-way sweep, no zipper noise); distance/gain smoothed | must | pending | stage-2 |
| DSP-07 | Order and decoder changes are click-free (crossfaded filter-bank swap; unused channels zeroed, buffers pre-sized for 16 channels) | must | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Live B-format channel view: one meter per ambisonic channel labelled by ACN index/name with its spherical-harmonic pattern icon, rows added/removed with order (the headline visual) | must | pending | stage-3 |
| UI-02 | Source-on-a-sphere view: draggable puck in top-down and side views, distance as radius, listener head with front marker; dragging sets azimuth/elevation; rotation shown as the scene compass turning | must | pending | stage-3 |
| UI-03 | Spatial-resolution polar plot of the panning function for the current direction, visibly sharpening from order 1 to 3 | should | pending | stage-3 |
| UI-04 | Live signal-path diagram (source → encoder → rotation → decoder → L/R) with active stage highlighted and channel count updating with order | should | pending | stage-3 |
| UI-05 | Decoder view showing the virtual-loudspeaker / HRTF directions in use | nice | pending | stage-3 |
| UI-06 | On-hover pedagogical tooltips on every control (W/X/Y/Z meaning, order vs channels, scene vs source rotation, ACN/SN3D, front/back confusion) | should | pending | stage-3 |
| UI-07 | Single-page, projector-readable layout laid out left-to-right as the signal path, consistent with the O-simple* family | must | pending | stage-3 |
| UI-08 | Preset tour selectable from the UI | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations/locks in processBlock); meters and position cross to UI via lock-free handoff | must | pending | stage-2 |
| PERF-02 | 3rd-order binaural decode (16 channels × 2 ears) runs without dropouts at 64–1024 sample buffers on an Apple-silicon laptop; convolution sized for the maximum host block | must | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |
| COMPAT-02 | Stereo-in/stereo-out (and mono-in) bus layout loads and runs identically in Logic Pro and Ableton Live with no multichannel configuration | must | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges (no clicks on order/decoder switch, no zipper on angle sweeps, no NaN/denormal) | must | pending | stage-2 |
| QUAL-02 | Localisation sanity on headphones: front, back, left, right, above are audibly distinct; left/right ILD sign matches the AmbiX convention (+90° azimuth = left) | must | pending | stage-4 |

## Acceptance Criteria Details

### FUNC-01: Complete chain in one plugin

**Description:** Mono source → encoder → rotation → decoder → L/R, all inside a stereo effect.

**Acceptance Criteria:**
- [ ] Plugin declares only stereo/mono input and stereo output buses
- [ ] With Order 1, Azimuth +90°, Binaural decoder, a test tone is louder in the left ear than the right
- [ ] Bypassing rotation (all 0°) and setting Yaw = -Azimuth brings the source back to front (audibly and in the meters)

### FUNC-02: Source position

**Acceptance Criteria:**
- [ ] Azimuth, Elevation, Distance are automatable host parameters with the stated ranges and units
- [ ] Elevation +90° yields a W/Z-dominated encode (X, Y ≈ 0) at order 1

### FUNC-03: Order select

**Acceptance Criteria:**
- [ ] Orders 1, 2, 3 produce 4, 9, 16 active channels in the meter view
- [ ] Switching order during playback produces no click (gate: peak discontinuity below -60 dBFS relative)

### FUNC-04: Scene rotation

**Acceptance Criteria:**
- [ ] Yaw rotates the decoded image while the source puck stays fixed
- [ ] Rotated encode equals direct encode at the rotated direction within 1e-4 (DSP-02 gate)

### FUNC-05: Decoder select

**Acceptance Criteria:**
- [ ] Binaural and Stereo modes both produce output; switching is click-free
- [ ] Reported latency equals the binaural convolution partition size; Stereo mode reports the same latency (or compensates) so switching does not shift time

### FUNC-07: Input handling

**Acceptance Criteria:**
- [ ] Stereo input is summed to mono (-6 dB pan law or equivalent, documented) before encoding
- [ ] Mono-in/stereo-out layout accepted by pluginval and Logic

### DSP-01: Encoder correctness

**Acceptance Criteria:**
- [ ] Unit gate compares all 16 SN3D coefficients at a grid of directions against the reference table, max abs error < 1e-5
- [ ] W channel is direction-independent (constant 1.0 under SN3D)

### DSP-02: Rotation correctness

**Acceptance Criteria:**
- [ ] For random (az, el, yaw, pitch, roll), rotate(encode(az, el)) == encode(rotated direction) within 1e-4 at every order

### DSP-03: Binaural decoder

**Acceptance Criteria:**
- [ ] Filter sets for orders 1/2/3 embedded as binary data; no file I/O at runtime
- [ ] `getLatencySamples()` equals the convolution's intrinsic latency; offline render of an impulse confirms it
- [ ] A source at +90° produces an interaural level difference with the correct sign and an ITD consistent with the HRTF set

### DSP-06: Angle smoothing

**Acceptance Criteria:**
- [ ] A jump from +170° to -170° passes through 180°, not through 0° (gate on the smoothed trajectory)
- [ ] No zipper noise in a 1-second full-circle azimuth sweep (spectral flatness / click gate)

### DSP-07: Click-free switching

**Acceptance Criteria:**
- [ ] Order 1→3 and Binaural→Stereo switches mid-tone show no discontinuity above the click threshold
- [ ] All buffers allocated in `prepareToPlay()` for 16 channels and the maximum block size

### UI-01: B-format channel view

**Acceptance Criteria:**
- [ ] 4 / 9 / 16 labelled meters appear for orders 1 / 2 / 3
- [ ] Moving the source from front to left visibly swaps energy from X to Y while W stays constant

### UI-02: Source-on-a-sphere view

**Acceptance Criteria:**
- [ ] Dragging the puck updates Azimuth/Elevation host parameters (gesture begin/end correct)
- [ ] Yaw rotates the compass/scene overlay, not the puck

### UI-07: Layout

**Acceptance Criteria:**
- [ ] Single page, no modal menus; all controls readable at projector distance
- [ ] Section order matches the signal path left-to-right

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [ ] Allocation gate on `processBlock` at all orders and both decoders
- [ ] Meter/position data crosses via lock-free FIFO or atomics

### PERF-02: CPU budget

**Acceptance Criteria:**
- [ ] Order 3 binaural at 64-sample buffer renders faster than real time with margin (measured in Release)
- [ ] Block sizes larger than the prepared size are handled without overrun

### COMPAT-01: pluginval

**Acceptance Criteria:**
- [ ] pluginval strictness 10 passes for VST3 and AU

### COMPAT-02: Host behaviour

**Acceptance Criteria:**
- [ ] Loads on a stereo and a mono track in Logic Pro and Ableton Live with no I/O configuration
- [ ] `auval -v` passes

### QUAL-01: No artifacts

**Acceptance Criteria:**
- [ ] No NaN/denormal under any parameter extreme (pluginval s10 + offline sweep)
- [ ] No clicks on preset load

### QUAL-02: Localisation sanity

**Acceptance Criteria:**
- [ ] Human listening check: front/back/left/right/above distinguishable on headphones at order 3
- [ ] +90° azimuth is louder in the LEFT ear (AmbiX sign convention)

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01, FUNC-07 |
| stage-2 | FUNC-01–06, DSP-*, PERF-*, QUAL-01 |
| stage-3 | UI-* |
| stage-4 | FUNC-08, FUNC-09, COMPAT-02, QUAL-02, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| B-format / multichannel output, loudspeaker decoding | Audience is on headphones in Logic/Ableton; O-Octagon / O-Orbit cover rigs | v1.x (Reaper passthrough) |
| Multiple sources / scene bus | One source keeps the lesson clean; stack instances instead | v2+ |
| Head tracking, custom SOFA loading | Complexity without pedagogical gain for v1 | v2+ |
| Room / reverb / early reflections | Keep the chain transparent | — |
| Source width / spread, max-rE / in-phase decoder weighting | Candidate lessons once research sizes the DSP | v1.1 |
| Orders above 3 | Lesson complete by 3rd; CPU cost without visible gain | — |

---
*Generated from BRIEF.md on 2026-10-11*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
