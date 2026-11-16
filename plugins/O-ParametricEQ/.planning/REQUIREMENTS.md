# O-ParametricEQ - Requirements

---
version: 1.0.0
plugin: O-ParametricEQ
created: 2026-11-12
lastUpdated: 2026-11-15
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 27
**Coverage:** must: 18 | should: 7 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Up to 24 independent bands, created and removed on demand | must | partial | stage-2 |
| FUNC-02 | Eight filter types per band: Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch, Band Pass, Tilt Shelf | must | partial | stage-2 |
| FUNC-03 | Per-band placement: Stereo, Left, Right, Mid, Side | must | complete | stage-2 |
| FUNC-04 | Per-band bypass and global click-free bypass | must | complete | stage-2 |
| FUNC-05 | Output gain and gain scale | must | complete | stage-2 |
| FUNC-06 | Auto gain compensates the curve's loudness change | should | complete | stage-2 |
| FUNC-07 | Presets via the suite preset-manager, with a factory set | should | pending | stage-4 |
| FUNC-08 | Band solo / listen | should | pending | stage-3 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Analog-matched magnitude response for bell and shelf up to Nyquist, without oversampling | must | complete | stage-2 |
| DSP-02 | Zero latency, minimum phase | must | complete | stage-2 |
| DSP-03 | Slopes of 6, 12, 18, 24, 36, 48, 72 and 96 dB/oct on every type that supports them | must | complete | stage-2 |
| DSP-04 | Click-free under automation and fast drags of every parameter, including type, slope, placement and enable | must | complete | stage-2 |
| DSP-05 | Numerically stable and quiet at low frequency / high Q at all supported sample rates | must | complete | stage-2 |
| DSP-06 | Parameter ranges: frequency 10 Hz – 30 kHz, gain ±30 dB, Q 0.025 – 40 | must | complete | stage-2 |
| DSP-07 | Band engine and state format leave room for dynamic EQ without breaking v1 sessions | should | complete | stage-1 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Interactive response display: click to create a band, drag frequency / gain, scroll Q | must | complete | stage-3 |
| UI-02 | Displayed curve equals the true filter response at the session sample rate | must | complete | stage-3 |
| UI-03 | Real-time spectrum analyzer: Off / Pre / Post / Pre+Post | should | pending | stage-3 |
| UI-04 | Selected-band strip with typed value entry and double-click reset | should | complete | stage-3 |
| UI-05 | Ouaricon house style, French and zh-Hans localisation | should | pending | stage-4 |
| UI-06 | Output level meter | nice | pending | stage-3 |
| UI-07 | Per-band curve fills coloured by band | nice | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations or locks in processBlock) | must | complete | stage-2 |
| PERF-02 | 24 bands at 96 dB/oct stay within a CPU budget set in Stage 0; disabled bands cost nothing | must | pending | stage-4 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) and auval | must | complete | stage-1 |
| COMPAT-02 | Mono and stereo layouts; 44.1 – 192 kHz; macOS VST3 + AU, Windows VST3 | must | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges; all bands at 0 dB / disabled is bit-transparent | must | partial | stage-2 |

## Acceptance Criteria Details

### FUNC-01: 24 on-demand bands

**Acceptance Criteria:**
- [x] 24 bands can be enabled simultaneously and each affects the output independently
- [x] Enabling or removing a band mid-playback produces no click (Phase 2.4)
- [ ] Band state survives save / reload and preset recall

### FUNC-02: Filter types

**Acceptance Criteria:**
- [x] Each of the eight types renders a measured response matching its design target
- [ ] Changing type on a live band produces no click or level jump beyond the response change itself

### FUNC-03: Per-band placement

**Acceptance Criteria:**
- [x] A Left band leaves the right channel bit-identical to input (and vice versa)
- [x] A Mid band leaves the side signal unchanged (and vice versa), within numerical tolerance
- [x] On a mono layout, placement is inert and the band processes the single channel

### FUNC-04: Bypass

**Acceptance Criteria:**
- [x] Global bypass output nulls against input
- [x] Toggling band or global bypass during a sustained tone produces no click (Phase 2.4)

### FUNC-05: Output gain and gain scale

**Acceptance Criteria:**
- [x] Output gain is accurate to ±0.05 dB across its range (Phase 2.5: XR-g12o worst 1.59e-7 dB)
- [x] Gain scale 0 % yields a flat response for all gain-bearing bands; 200 % doubles their dB gain

### FUNC-06: Auto gain

_Complete 2026-11-15 (`stages/2-dsp/VERIFICATION-2.5.md`)._

**Acceptance Criteria:**
- [x] G12: the negative pink-weighted mean power gain of the target curve (512 cell midpoints, 40 Hz – min (16 kHz, 0.45 fs)), clamped to [−18, +12] dB
- [x] Toggling auto gain and sweeping a band under it are click-free (G3)
- [x] Offline and real-time renders are bit-identical

### DSP-01: Analog-matched response

_Complete 2026-11-13 (`stages/2-dsp/VERIFICATION-2.2.md`): design side at the Phase 2.1b G1 tolerance (2.1b); rendered engine equals the closed form and stays inside that tolerance against the analog prototype; latency 0, no oversampling._

**Acceptance Criteria:**
- [x] At 44.1 kHz, bell and shelf magnitude responses match the analog prototype within a tolerance set in Stage 0, measured up to 20 kHz, for centre frequencies up to 20 kHz
- [x] No oversampling is used and reported latency stays 0

### DSP-02: Zero latency, minimum phase

**Acceptance Criteria:**
- [x] `getLatencySamples()` returns 0 in every configuration
- [x] An impulse through any configuration has no pre-ringing

### DSP-03: Slopes

**Acceptance Criteria:**
- [x] Each slope setting measures its nominal dB/oct in the stop band for low cut and high cut
- [x] Slope changes on a live band are click-free (Phase 2.4: G2 live and the G3 slope rows; stop-band burst open as a listening item, VERIFICATION-2.4 Issue 1)

### DSP-04: Click-free modulation

**Acceptance Criteria:**
- [x] Full-range automation sweeps of frequency, gain and Q on a sustained low-frequency sine pass the suite click gate (G3 sweep rows as amended 2026-11-13; Phase 2.3)
- [x] Stepped changes of type, slope, placement and enable pass the same gate (G3 step rows as amended 2026-11-14; Phase 2.4)
- [x] No NaN / Inf and no instability under random parameter stress at all block sizes (G5, 600 s at 8 rates, with structural fades and bypass; Phase 2.4)

### DSP-05: Stability and noise

_Complete 2026-11-14 (`stages/2-dsp/VERIFICATION-2.3.md`): G7 as amended; residual -235.4 dBFS in double and -159.5 dBFS on the float output, ring-down 3.419 dB/s to -280.9 dBFS in the 60th second, silence costs 1.00x the block time with noise._

**Acceptance Criteria:**
- [x] A 10 Hz, Q 40 bell at 192 kHz stays stable and its noise floor stays below a threshold set in Stage 0
- [x] Silence in gives silence out (no denormal CPU spikes, no residual tone)

### DSP-06: Parameter ranges

**Acceptance Criteria:**
- [x] All ranges reachable from UI, automation and typed entry
- [x] Frequencies above Nyquist at low sample rates are handled without instability

### UI-01: Interactive display

**Acceptance Criteria:**
- [x] Clicking empty curve space creates a band at that frequency / gain
- [x] Dragging a node updates frequency and gain with host automation gestures; scroll changes Q

### UI-02: True curve

**Acceptance Criteria:**
- [x] Displayed summed curve matches the measured plugin response within 0.1 dB at every supported sample rate

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [x] Allocation gate reports zero allocations in processBlock under parameter stress (Phase 2.3, A2; with the analyzer on, rings saturated and listen switching at Phase 2.6, A5 / XT-alloc)
- [x] No locks or file / message-thread calls on the audio thread

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
- [x] With no bands enabled and output gain 0 dB, output is bit-identical to input
- [ ] No audible artifacts in listening tests across factory presets

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01, DSP-07 (state format and layout; the band-engine half closed at Phase 2.2 verify) |
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

*Phase 2.2 verify, 2026-11-13 (`stages/2-dsp/VERIFICATION-2.2.md`): FUNC-03, DSP-01, DSP-02, DSP-07 complete; FUNC-01, FUNC-02, FUNC-04, FUNC-05, DSP-03, DSP-06, PERF-01, QUAL-01 partial (their click-free, modulation, output-stage, UI and listening criteria belong to Phases 2.3 - 2.5 and Stages 3 - 4).*

*Phase 2.3 verify, 2026-11-14 (`stages/2-dsp/VERIFICATION-2.3.md`): DSP-05 complete; DSP-04 partial (sweeps met, structural changes are 2.4); PERF-01 stays partial until G6 re-runs with the analyzer on (2.6).*

*Phase 2.4 verify, 2026-11-14 (`stages/2-dsp/VERIFICATION-2.4.md`): DSP-03, DSP-04 and FUNC-04 are complete. FUNC-01, FUNC-05, QUAL-01 and PERF-01 stay partial, waiting on presets, output gain, listening and the analyzer G6. Open: the stop-band burst on a steep cut's slope change (Issue 1), held for the hands-on listen.*

*Phase 2.5 verify, 2026-11-15 (`stages/2-dsp/VERIFICATION-2.5.md`): FUNC-05 and FUNC-06 are complete. QUAL-01 and PERF-01 stay partial, waiting on listening and the analyzer G6 (2.6).*

*Phase 2.6 verify, 2026-11-15 (`stages/2-dsp/VERIFICATION-2.6.md`), Stage 2 closed: PERF-01 complete. FUNC-01, FUNC-02, DSP-06 and QUAL-01 stay partial; their remaining criteria are Stage 3 / 4 or the hands-on listen. Engine halves met for FUNC-08 (listen), UI-02 (G9 (a), 2.1e-3 dB) and UI-03 (taps, analyzer); those requirements close in Stage 3.*

*Phase 3.1 verify, 2026-11-15 (`stages/3-gui/VERIFICATION-3.1.md`): no status change. Bindings, presets (C2), disabled states and the transient half of FUNC-08 (`setListenBand` mapping, editor-close reset) proven; UI-01, UI-02, UI-04 close in 3.2, FUNC-08, UI-03, UI-06, UI-07 in 3.3.*

*Phase 3.2 verify, 2026-11-16 (`stages/3-gui/VERIFICATION-3.2.md`): UI-01, UI-02, UI-04 and DSP-06 complete. UI-02 by the chain G9 (a) 2.03e-3 dB + G9 (b) 1.37e-8 dB at all five rates and mono; UI-01 gestures proven in-process (Logic / VST3 lane recording is a hands-on row); UI-04 / DSP-06 by E-ends 20 / 20 exact and E-dblclick. FUNC-08, UI-03, UI-06, UI-07 close in 3.3.*
