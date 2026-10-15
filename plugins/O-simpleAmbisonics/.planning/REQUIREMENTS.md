# O-simpleAmbisonics - Requirements

---
version: 1.0.0
plugin: O-simpleAmbisonics
created: 2026-10-11
lastUpdated: 2026-10-14
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 27
**Coverage:** must: 16 | should: 9 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Complete ambisonic chain inside one stereo-out plugin: mono source → SH encoder → SH-domain rotation → decoder → L/R; no multichannel bus required | must | complete | stage-2 |
| FUNC-02 | Source positioned by Azimuth (-180…+180°), Elevation (-90…+90°) and Distance (0.2–20 m) | must | complete | stage-2 |
| FUNC-03 | Ambisonic order selectable 1 / 2 / 3 (4 / 9 / 16 channels), switchable while audio runs | must | complete | stage-2 |
| FUNC-04 | Scene rotation via Yaw (must), Pitch and Roll (should), applied in the ambisonic domain after encoding so the scene rotates, not the source | must | complete | stage-2 |
| FUNC-05 | Decoder selectable: Binaural (HRTF, headphones) or Stereo (plain amplitude decode for speakers/projector) | must | complete | stage-2 |
| FUNC-06 | Orbit: automatic azimuth motion with an on/off switch and a rate (0.02–2 Hz) so movement is audible without DAW automation | should | complete | stage-2 |
| FUNC-07 | Stereo input accepted and summed to the single mono source (Input Mode: Mono sum / Left only); mono input accepted | must | complete | stage-1 |
| FUNC-08 | Concept-preset tour (Point Source FOA, Order 1→2→3, Behind You, Overhead, Rotate the Room, Far Away / Up Close, Slow Orbit, Speakers vs Headphones) loadable as factory presets | should | complete | stage-4 |
| FUNC-09 | Usable as a lightweight creative binaural panner with orbit motion for headphone mixes | nice | complete | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | SH encoder in AmbiX convention (ACN order, SN3D normalisation) for orders 1–3; coefficients match the reference table in `research/ambisonics-encoding-deep-dive.md` within 1e-5 | must | complete | stage-2 |
| DSP-02 | SH-domain rotation matrix per order (yaw/pitch/roll), mathematically equivalent to re-encoding the source at the rotated direction | must | complete | stage-2 |
| DSP-03 | HRTF binaural decoder (MagLS or virtual-loudspeaker + HRTF; Stage 0 decides) from an embedded, precomputed filter set per order; partitioned convolution; latency reported via `setLatencySamples()` | must | complete | stage-2 |
| DSP-04 | Stereo decode mode (±30° virtual pair or equivalent) without HRTF | should | complete | stage-2 |
| DSP-05 | Distance model: inverse-distance gain with near-field clamp plus a gentle air-absorption low-pass; monotonic and explainable | should | complete | stage-2 |
| DSP-06 | Angle parameters smoothed along the shortest path around ±180° (no long-way sweep, no zipper noise); distance/gain smoothed | must | complete | stage-2 |
| DSP-07 | Order and decoder changes are click-free (crossfaded filter-bank swap; unused channels zeroed, buffers pre-sized for 16 channels) | must | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Live B-format channel view: one meter per ambisonic channel labelled by ACN index/name with its spherical-harmonic pattern icon, rows added/removed with order (the headline visual) | must | complete | stage-3 |
| UI-02 | Source-on-a-sphere view: draggable puck in top-down and side views, distance as radius, listener head with front marker; dragging sets azimuth/elevation; rotation shown as the scene compass turning | must | complete | stage-3 |
| UI-03 | Spatial-resolution polar plot of the panning function for the current direction, visibly sharpening from order 1 to 3 | should | complete | stage-3 |
| UI-04 | Live signal-path diagram (source → encoder → rotation → decoder → L/R) with active stage highlighted and channel count updating with order | should | complete | stage-3 |
| UI-05 | Decoder view showing the virtual-loudspeaker / HRTF directions in use | nice | complete | stage-3 |
| UI-06 | On-hover pedagogical tooltips on every control (W/X/Y/Z meaning, order vs channels, scene vs source rotation, ACN/SN3D, front/back confusion) | should | complete | stage-3 |
| UI-07 | Single-page, projector-readable layout laid out left-to-right as the signal path, consistent with the O-simple* family | must | complete | stage-3 |
| UI-08 | Preset tour selectable from the UI | should | complete | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations/locks in processBlock); meters and position cross to UI via lock-free handoff | must | complete | stage-2 |
| PERF-02 | 3rd-order binaural decode (16 channels × 2 ears) runs without dropouts at 64–1024 sample buffers on an Apple-silicon laptop; convolution sized for the maximum host block | must | complete | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | complete | stage-1 |
| COMPAT-02 | Stereo-in/stereo-out (and mono-in) bus layout loads and runs identically in Logic Pro and Ableton Live with no multichannel configuration | must | complete | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges (no clicks on order/decoder switch, no zipper on angle sweeps, no NaN/denormal) | must | complete | stage-4 |
| QUAL-02 | Localisation sanity on headphones: front, back, left, right, above are audibly distinct; left/right ILD sign matches the AmbiX convention (+90° azimuth = left) | must | complete | stage-4 |

## Acceptance Criteria Details

### FUNC-01: Complete chain in one plugin

**Description:** Mono source → encoder → rotation → decoder → L/R, all inside a stereo effect.

**Acceptance Criteria:**
- [x] Plugin declares only stereo/mono input and stereo output buses — auval caps `[1, 2] [2, 2]` (Stage 2 / 2.1 verify, 2026-10-12)
- [x] With Order 1, Azimuth +90°, Binaural decoder, a test tone is louder in the left ear than the right — 22.88 dB L/R under the 2.1 placeholder (Stage 2 / 2.1 verify, 2026-10-12); with the real SADIE II leg: white noise LEFT louder by +12.31 dB on the installed VST3, impulse ILD +12.28 dB with ITD +36 samples (L leads), harness DSP-03 identical (Stage 2 / 2.3 verify, 2026-10-13)
- [x] Bypassing rotation (all 0°) and setting Yaw = -Azimuth brings the source back to front (audibly and in the meters) — harness frame `shGain == refSh16 (0, 0)` 2.2e-8 with |L − R| = 0 at order 3; pedalboard on the installed VST3: az +90 / 37 / −150 with the opposite yaw gives |L − R| ≤ 1.5e-8 and matches the front render (Stage 2 / 2.2 verify, 2026-10-12); the convolver consumes the rotated bus: yaw +90 on a front source == azimuth +90 in Binaural at order 3, max diff 0.0 (Stage 2 / 2.3 verify, 2026-10-13)

### FUNC-02: Source position

**Acceptance Criteria:**
- [x] Azimuth, Elevation, Distance are automatable host parameters with the stated ranges and units — pedalboard −180…180 / −90…90 / 0.2…20 m (Stage 2 / 2.1 verify, 2026-10-12)
- [x] Elevation +90° yields a W/Z-dominated encode (X, Y ≈ 0) at order 1 — DSP-01 grid at el ±90 (2.07e-07 vs the generic table), DSP-04 e = ±90 → L == R == 0.5f bit (Stage 2 / 2.1 verify, 2026-10-12)

### FUNC-03: Order select

**Acceptance Criteria:**
- [x] Orders 1, 2, 3 produce 4, 9, 16 active channels in the meter view — `MeterFrame.orderActive` 4 / 9 / 16, `shGain[active..15] == 0` (Stage 2 / 2.1 verify, 2026-10-12); the drawn view is UI-01 (stage-3)
- [x] Switching order during playback produces no click — bus-tap second-difference ratio ≤ 2.0 (0.9999 / 1.0000; negative control 66.9), bands arrive exactly at 0 / Y (Stage 2 / 2.1 verify, 2026-10-12); the decoder filter-bank swap: DSP-07 order 1→3 / 3→1 ratios 0.998–1.004 vs the same-motion reference at 64 and 1024 blocks, 1.00–1.53 on the installed VST3 vs the static pair (snapped control 67×) (Stage 2 / 2.3 verify, 2026-10-13)

### FUNC-04: Scene rotation

**Acceptance Criteria:**
- [x] Yaw rotates the decoded image while the source puck stays fixed — frame `azEff / elEff` bit-equal to the host parameters under yaw while `azRot` moves (harness), yaw ψ on a front source bit-equal to azimuth ψ on the installed VST3 with the azimuth host parameter untouched (pedalboard V2 / V3); pitch + raises the front and roll + raises the left: cardinal probes elRot 90.00000 ×3, pedalboard V4 / V5 anchors exact (Stage 2 / 2.2 verify, 2026-10-12)
- [x] Rotated encode equals direct encode at the rotated direction within 1e-4 (DSP-02 gate) — max 2.6e-7 / 5.2e-7 / 7.2e-7 at orders 1 / 2 / 3 over 200 tuples (Stage 2 / 2.2 verify, 2026-10-12)

### FUNC-05: Decoder select

**Acceptance Criteria:**
- [x] Binaural and Stereo modes both produce output; switching is click-free — Binaural ≠ Stereo by 1.3988 (harness) / 0.75 (installed impulse); B→S→B click ratios 1.0000–1.0007 at 64 / 1024 blocks vs the same-motion reference (negative control 12–28×); on the installed VST3 the residual ratio reads 1.000 (snapped control 41–68×); level delta at the front, order 3: +0.004 dB harness / −0.005 dB pedalboard (Stage 2 / 2.3 verify, 2026-10-13)
- [x] Reported latency equals the binaural convolution partition size; Stereo mode reports the same latency (or compensates) so switching does not shift time — `getLatencySamples() == 64` = kPartition in both modes at 44.1 / 48 / 96 k; the stereo leg runs through `FixedDelay<64>`; pedalboard: the Stereo impulse at the compensated index 100 (0.75 exactly) and the Binaural onset at 100 + the header onset constant exactly, at every rate (Stage 2 / 2.3 verify, 2026-10-13)

### FUNC-07: Input handling

**Acceptance Criteria:**
- [x] Stereo input is summed to mono (-6 dB pan law or equivalent, documented) before encoding — `Mono sum` = 0.5·(L+R); harness P1 bit-equal, pedalboard exact (Stage 1 verify, 2026-10-12)
- [x] Mono-in/stereo-out layout accepted by pluginval and Logic — auval caps `[1, 2] [2, 2]`, pluginval s10 VST3 + AU, harness P1b mono bus (Stage 1 verify); Logic hands-on repeats under COMPAT-02

### DSP-01: Encoder correctness

**Acceptance Criteria:**
- [x] Unit gate compares all 16 SN3D coefficients at a grid of directions against the reference table, max abs error < 1e-5 — 13 × 7 grid vs an independent double table, max 2.07e-07 (Stage 2 / 2.1 verify, 2026-10-12)
- [x] W channel is direction-independent (constant 1.0 under SN3D) — W == 1.0f at every grid point (Stage 2 / 2.1 verify, 2026-10-12)

### DSP-02: Rotation correctness

**Acceptance Criteria:**
- [x] For random (az, el, yaw, pitch, roll), rotate(encode(az, el)) == encode(rotated direction) within 1e-4 at every order — 200 seeded tuples × orders 1 / 2 / 3: 2.613e-07 / 5.248e-07 / 7.157e-07; block orthogonality 5.6e-7; transpose trap FAILs as expected (1.974); identity exact in float (digest-2.1 `9189d911a1fe98d5` reproduced); yaw ≡ azimuth bit-equal on the installed VST3 (Stage 2 / 2.2 verify, 2026-10-12)

### DSP-03: Binaural decoder

**Acceptance Criteria:**
- [x] Filter sets for orders 1/2/3 embedded as binary data; no file I/O at runtime — `Source/dsp/BinauralFilterData.h`: `inline constexpr float kMagLsFilters{1,2,3}` (SADIE II D1 KU100 MagLS, 256 taps at 48 k, sha256-pinned source, regenerates byte-identical); the installed plugin's impulse responses equal Σ Y·h from the header floats within 5.6e-8 (Stage 2 / 2.3 verify, 2026-10-13)
- [x] `getLatencySamples()` equals the convolution's intrinsic latency; offline render of an impulse confirms it — 64 in both modes at 44.1 / 48 / 96 k; impulse onsets 248 / 214 at 48 k (= 100 + 64 + the table onset 84 / 50, exactly the generator constant), 237 / 210 at 44.1 k, 332 / 263 at 96 k; UPOLS null vs direct-form ≤ 1.4e-7 (Stage 2 / 2.3 verify, 2026-10-13)
- [x] A source at +90° produces an interaural level difference with the correct sign and an ITD consistent with the HRTF set — ILD +12.28 / +13.79 / +13.87 dB (L louder, AmbiX) and ITD +36 / +38 / +37 samples (L leads, LP-xcorr below 0.8·f_a) at orders 1 / 2 / 3, mirrored at −90° (−11.89 / −13.98 / −14.58 dB, −36 / −38 / −37); harness and pedalboard agree to the printed digit (Stage 2 / 2.3 verify, 2026-10-13)

### DSP-06: Angle smoothing

**Acceptance Criteria:**
- [x] A jump from +170° to -170° passes through 180°, not through 0° — harness DSP-06a trajectory + pedalboard V7 on the installed VST3 (Stage 2 / 2.1 verify, 2026-10-12)
- [x] No zipper noise in a 1-second full-circle azimuth sweep — second-difference click ratio 1.0003 / 1.0000 vs a matched static control (negative control 24.8) (Stage 2 / 2.1 verify, 2026-10-12)
- [x] Extended to yaw / pitch / roll — 360°/s yaw sweep bus-tap ratio 1.127 ≤ 2.0 on all 16 channels (negative control 49–156), 0 → 180 yaw step 1.0003 vs the same-motion reference (negative control 98–544); on the installed VST3 the sweep's normalised second-difference ratio reads 1.000 (negative control 24.8) (Stage 2 / 2.2 verify, 2026-10-12)

### DSP-07: Click-free switching

**Acceptance Criteria:**
- [x] Order 1→3 and Binaural→Stereo switches mid-tone show no discontinuity above the click threshold — 100 Hz at az +90°, Binaural: order 1→3 / 3→1 ratios 0.9984 / 1.0037 and 1.0004 / 0.9995 at 64 blocks, 1.0001–1.0004 at 1024, vs the same-motion reference (ceiling 2.0; snapped-bank negative controls 11–179×); B→S→B 1.0000–1.0007 (negative 12–28×); spectral liveness +6.28 dB; queued latest-wins request null 3.1e-7; on the installed VST3 1.00–1.53 vs the static pair with snapped controls 41–68× (Stage 2 / 2.3 verify, 2026-10-13)
- [x] All buffers allocated in `prepareToPlay()` for 16 channels and the maximum block size — every `BinauralConvolver` vector assigned in `prepare()` (spectra for the three sets, FDL for K(fs)); the convolver is grid-sized and the processor slices, so oversized host blocks are bit-equal (P3e prep 64 / feed 4096); FDL decays to exact 0; PERF-01 0 allocations with the convolver live (Stage 2 / 2.3 verify, 2026-10-13)

### UI-01: B-format channel view

**Acceptance Criteria:**
- [x] 4 / 9 / 16 labelled meters appear for orders 1 / 2 / 3 — frame-probe A3 (bands 2/3/4, 16 SN3D icons), harness PL2 (Stage 3 verify, 2026-10-14)
- [x] Moving the source from front to left visibly swaps energy from X to Y while W stays constant — PL2 left Y 1000 / X 0; frame-probe A1 W / X / Y 0.997 / 0.997 / 0, `fuma` negative control FAILS (Stage 3 verify, 2026-10-14)

### UI-02: Source-on-a-sphere view

**Acceptance Criteria:**
- [x] Dragging the puck updates Azimuth/Elevation host parameters (gesture begin/end correct) — frame-probe A6 / A7 one gesture each, 0 rear samples across ±180; real-host recording carried to Stage 4 hands-on (Stage 3 verify, 2026-10-14)
- [x] Yaw rotates the compass/scene overlay, not the puck — frame-probe A5 (puck unchanged, rose turns), A9 pole hold (Stage 3 verify, 2026-10-14)

### UI-07: Layout

**Acceptance Criteria:**
- [x] Single page, no modal menus; all controls readable at projector distance — 1180×780 design frame, scale 0.6–1.5 (ST3 / ST4, verify_3_state S1–S3); visual checkpoint approved by the user 2026-10-13 (Stage 3 verify, 2026-10-14)
- [x] Section order matches the signal path left-to-right — Source | Encoder | Rotation | Decoder | Output, A11 stations (Stage 3 verify, 2026-10-14)

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [x] Allocation gate on `processBlock` at all orders and both decoders — global new/delete counter 0 across 200 calls, liveness 1 (Stage 2 / 2.1 verify, 2026-10-12); re-gated with yaw / pitch / roll moving, 0 allocations (Stage 2 / 2.2 verify, 2026-10-12); re-gated with the convolver live and the order / decoder fades inside the window, 0 allocations (Stage 2 / 2.3 verify, 2026-10-13)
- [x] Meter/position data crosses via lock-free FIFO or atomics — `TripleBuffer<MeterFrame>`, two atomics, no locks (inspection) (Stage 2 / 2.1 verify, 2026-10-12)

### PERF-02: CPU budget

**Acceptance Criteria:**
- [x] Order 3 binaural at 64-sample buffer renders faster than real time with margin (measured in Release) — `steady_clock`, 480 000 samples, best of 3: 203× at execute, 193× / 197× at verify (under a concurrent pluginval) at 48 k / 64 blocks; 195–206× at 1024; 72–76× at 96 k (K 8); gate ≥ 20 (Stage 2 / 2.3 verify, 2026-10-13)
- [x] Block sizes larger than the prepared size are handled without overrun — the engine is grid-sized (no host-sized buffer) and the processor slices; P3e: prepare 64 then feed 4096 == prepare 4096 == flat, bit-equal, in Binaural with rotation + orbit (Stage 2 / 2.3 verify, 2026-10-13)

### FUNC-08: Concept-preset tour

**Acceptance Criteria:**
- [x] The 8 concepts load as factory presets (Init + 8, one table `PresetRecipes.h`) from the lesson row and the header preset bar — G-FACTORY[table / init / apply / defs / skew-NC], G-LESSON[walk 0 1 2 0 / restart / owned] + NC, G-ORDER3-DIST 22 states 0 violations + NC, G-PRESET-STATE[identity / restore], G-PRESET-USER + NC, G-PRESET-NAME 9 / 9, G-PRESET-WALK; verify_3_state S4 / S5 on the installed VST3; presets never move Output Level or Input Mode; CP1 visual checkpoint signed off by the user (Stage 4 execute, 2026-10-14)

### FUNC-09: Creative binaural panner

**Acceptance Criteria:**
- [x] Slow Orbit on a source inside a headphone mix is usable as a creative panner — Taylor's CP3 listening, installed v1.0.0 (Stage 4 execute CP3, 2026-10-14)

### COMPAT-01: pluginval

**Acceptance Criteria:**
- [x] pluginval strictness 10 passes for VST3 and AU — installed bundles, exit 0 SUCCESS both (Stage 1 verify, 2026-10-12)

### COMPAT-02: Host behaviour

**Acceptance Criteria:**
- [x] Loads on a stereo and a mono track in Logic Pro and Ableton Live with no I/O configuration — Taylor's CP3 hands-on on the installed v1.0.0: mono + stereo tracks in Logic and Ableton, audio passes, latency compensated, Save field receives keystrokes in both hosts; carried Logic list R1–R3 + general and the Standalone list all pass (Stage 4 execute CP3, 2026-10-14)
- [x] `auval -v` passes — `auval -v aufx OSAm OuDv` AU VALIDATION SUCCEEDED on the installed v1.0.0; pluginval s10 ×2 VST3 + ×2 AU SUCCESS (Stage 4 execute T12, 2026-10-14); Windows VST3 (MSVC, WebView2 static) builds and passes pluginval s10 in CI — run 38675387588, 25 / 25 groups, 0 FAILED (Stage 4 execute T13, 2026-10-14)

### QUAL-01: No artifacts

**Acceptance Criteria:**
- [x] No NaN/denormal under any parameter extreme — pluginval s10 ×2, 16-corner harness render at 48 k / 96 k, 32-corner pedalboard render all finite (Stage 2 / 2.1 verify, 2026-10-12); ±180° on all three rotation angles at 48 k / 96 k + gimbal θ = 90° under a yaw sweep finite, 32 rotation corners on the installed VST3 finite (Stage 2 / 2.2 verify, 2026-10-12); with the HRTF leg: harness 8 + 8 Binaural corners at 48 k / 96 k, gimbal under a yaw sweep, headroom corner (+14.5 dBFS, finite), 32 Binaural corners on the installed VST3 (48 k / 96 k × orders × pole sources × 0.2 / 20 m × ±180 rotation, orbit 2 Hz) finite and live (Stage 2 / 2.3 verify, 2026-10-13); a NaN / Inf input sample no longer latches the air LP — `S4 WR-02` 4 / 4 renders finite and bit-equal to the zeroed input, NC latches (Stage 4 execute, 2026-10-14)
- [x] No clicks on preset load — G-PRESET-CLICK: 12 transitions (8 bank loads, then orderStep 0 → 1 → 2 → 0), 9 ≤ 2.0 vs the static pair; the 3 arms with legitimate smoothed moves (Overhead 2.58, Speakers vs Headphones 2.16, the next orderStep click 4.33 static) read 0.9999 / 1.0001 / 1.0003 vs the same-motion reference (ceiling 2.0); splice NC 268.1 / 10.4 / 62.1 FAILS as designed; Input Mode switch 1.0026 (`S4 WR-04`, NC 579) (Stage 4 execute, 2026-10-14)

### QUAL-02: Localisation sanity

**Acceptance Criteria:**
- [x] Human listening check: front/back/left/right/above distinguishable on headphones at order 3 — Taylor, Binaural, order 3, 1 m, dry mono source: all five directions distinct (Stage 4 execute CP3, 2026-10-14)
- [x] +90° azimuth is louder in the LEFT ear (AmbiX sign convention) — Taylor by ear (CP3, 2026-10-14), plus the DSP-03 ILD-sign gate (ILD +12.28 / +13.87 dB, Stage 2.3)

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
