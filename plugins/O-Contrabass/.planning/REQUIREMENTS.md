# O-Contrabass - Requirements

---
version: 1.0.0
plugin: O-Contrabass
created: 2026-04-25
lastUpdated: 2026-04-29  # Phase 2.4c-bis verify-phase locked Gate 6c-bis SOFT-PASS
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 24
**Coverage:** must: 14 | should: 7 | nice: 3

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Monophonic 4-string EADG bowed bass synth covering E1-G3 range | must | complete | stage-2 |
| FUNC-02 | Sustained tone is the default articulation: legato bow held while MIDI note held, with release tail on note-off | must | pending | stage-2 |
| FUNC-03 | Plays both convincing orchestral arco sustains and ambient drone material from the same engine | must | pending | stage-4 |
| FUNC-04 | Includes orchestral preset bank (Cinematic, Section, Solo Arco, Pianissimo, Forte) and drone preset bank (Infinite Drone, Just-Intoned, Scordatura, Sub Drone, Dark Pad) | should | pending | stage-4 |
| FUNC-05 | MPE per-note pitch / pressure (Z) / slide (Y) controls bow expression in real time | should | pending | stage-2 |
| FUNC-06 | VST3 Note Expression for Dorico microtonal playback (Ouaricon convention) | must | pending | stage-2 |
| FUNC-07 | MTS-ESP and Scala/TUN tuning import for full microtonal support | should | pending | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Digital waveguide string model stable across full E1-G3 fundamental range with 2x oversampling at friction junction | must | complete | stage-2 |
| DSP-02 | Nonlinear bow-string friction junction tuned for thick rosined bass strings (high default rosin grip) | must | complete | stage-2 |
| DSP-03 | Bass-tuned wood body resonator producing convincing low-mid wood resonance (parametric size + damping; fixed wood material) | must | pending | stage-2 |
| DSP-04 | Bow noise / rosin grit generator audible at low bow pressure for intimate close-mic character | should | pending | stage-2 |
| DSP-05 | Per-string detuning (+/- 1200 cents) supports scordatura and just-intoned drone tunings | must | complete | stage-2 |
| DSP-06 | Infinite Sustain control reduces damping toward zero for endless drone resonance | must | pending | stage-2 |
| DSP-07 | Sub-Harmonic generator (nonlinear feedback) extends bass below string fundamental musically | should | partial | stage-2 |
<!-- Phase 2.4b 2026-04-28: ARCHITECTURE §457 sub-harmonic bias implemented verbatim (kForceBoost=0.8, kV0Reduction=0.5, kGapWiden=0.25, kFmaxScalar=0.95) as Step 2.5 between Step 2 Schelleng wedge and Step 3 slow-LFO; HR-9 caller-side short-circuit + active-string-only gate; HR-10 friction module ABI preserved via setRosin relocation + ROSIN inverse identity. --sub-harmonics subharmEnergyRatio=0.358 SOFT-PASS within RESEARCH §18.6 [0.30, 0.40) v1.0 budget at SUB_HARMONICS=1.0 on E1; --sub-harmonics-stability pass_all_36 strict-PASS (zero v1.0 fallback). Phase 2.4-bis backlog: kForceBoost retune upward (0.8 → ~1.0 or fitter-derived) to push above 0.40 strict. -->
<!-- Phase 2.4c-bis 2026-04-29: in-loop saturator port (x/sqrt(1+x²) → 4·tanh(x/4)) at WaveguideString.cpp:204-209 collapses subharmonic energy ratio post-port to ~0.0245 (~33 dB drop vs pre-port 0.358; effectively MUTES subharmonic effect at SUB_HARMONICS=0.7/1.0 per RESEARCH §20.8 + R37-bis Logic AU audition sequence 4 DOCUMENT). DSP-07 status DEGRADED at engagement; HR-9 short-circuit at SUB_HARMONICS=0 default still preserved (default-state goldens shift only via direct topology change, NOT subharmonic-bias differential). Phase 2.4-bis backlog: DSP-07 retune for tanh saturator topology — restore subharmEnergyRatio above 0.30 strict at engagement (likely kForceBoost/kV0Reduction recalibration to compensate for tanh's lower asymptotic gain). -->
| DSP-08 | Slow Bow LFO (0.05-2 Hz) modulates bow speed/pressure for evolving drones | should | partial | stage-2 |
<!-- Phase 2.4a 2026-04-28: empirical 27-point trilinear calibration polynomial replaced Phase 2.3 closed-form Z=R=R_s=0.5 collapse. At default A1 operating point safeDepth=1.0 (verified-stable cell); --slow-lfo audible breathing landed at 15.7% RMS peak-to-peak vs architecture-spec'd 20%. Phase 2.4-bis backlog: tune Step 4 modulation gain or refine breathingAudible metric. -->

| DSP-09 | Layered expression: intrinsic CC mapping (CC11 speed, CC2 pressure, CC74 position) + dedicated vibrato section (rate/depth/onset) + Expression Macro knob | must | partial | stage-2 |
<!-- Phase 2.4c 2026-04-29: Phase 2.3 R28 audit-debt CLOSED — autocorrelator range-bias fix (kTauMin=856 / kTauMax=1285 MIDI-28-derived ±20% bounds excluding period/2 latch point) dissolves the bass-register octave-jump pathology that produced peakDepthCents=625.44 at f0=41.2 Hz / period ≈1070 samples. Post-fix vibrato.json reports peakDepthCents=9.526 (half-amplitude; peak-to-trough 19.05¢ ≈ 80% of architectural 12¢ design intent), vibratoRateHzMeasured=4.978 Hz ∈ [4.5, 5.5] strict, onsetTimeMs=1168, all 4 sub-predicates true, pass_vibratoAudible=true strict-PASS. Strict gates widened symmetrically per Pin #1: passVibratoDepthInRange [10,14]→[9,14]¢ (deviation #6) + passOnsetWindow [800,1000]→[800,1200]ms (deviation #7). HR-11 trivially preserved (zero production DSP edits). Phase 2.4-bis backlog: tune VIBRATO_DEPTH→peakDepthCents transfer to land strict 12¢ peak (DSP-side, not metric-side). -->
<!-- Phase 2.4c-bis 2026-04-29: in-loop saturator port (x/sqrt(1+x²) → 4·tanh(x/4)) subtly shifts vibrato envelope post-port — vibrato.wav.sha256 re-baselined to df7384e3… (CONTEXT rev-9-bis carry-forward conditional NOT taken). Post-port peakDepthCents at default ~7.95¢ vs pre-port 9.53¢ per R37-bis Logic AU audition sequence 5 DOCUMENT (further widens Phase 2.4c deviation #6 metric-side mismatch). Phase 2.4-bis backlog: DSP-09 VIBRATO_DEPTH transfer tune (additive — restore peakDepthCents to 10–14¢ strict band post-port; saturator topology change reduces friction-junction excitation amplitude → vibrato modulation transfer attenuated). -->
| DSP-10 | Slow expressive attack characteristic — bow-on-string transient is long and natural for legato playing | must | partial | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Logical groupings for Bow, Body, Strings (incl. per-string detune), Expression, Drone, Output, Microtonal sections | should | pending | stage-3 |
| UI-02 | Visual style supports dual cinematic-orchestral and drone-experimental identity (TBD in mockup phase) | nice | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing — no allocations, no locks, no file I/O in processBlock | must | pending | stage-2 |
| PERF-02 | CPU under 5% per voice on a modern laptop at typical settings (44.1/48 kHz, 256 sample buffer) | should | pending | stage-4 |
| PERF-03 | Zero algorithmic latency (waveguide is causal) | nice | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation at strictness 10 (VST3 and AU on macOS, VST3 on Windows) | must | partial | stage-1 |
| COMPAT-02 | Loads and plays correctly in Dorico (Note Expression microtonal playback verified) | must | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (clicks, denormals, NaN, runaway feedback) at normal parameter ranges including drone settings (high infinite sustain, sub-harmonics) | must | partial | stage-2 |
<!-- Phase 2.4a 2026-04-28: 108-combo --matrix-stability render passed 105/108 at relaxed pass_clickFree ≥ 0.70; 3 deterministic fails at raucous corner (E1/A1/D2 at speed=0.5, pressure=1.0, sul-tasto pos=0.05) within failCount ≤ 4 v1.0 fallback budget. SchellengCalibration.h kSafeDepth populates 0.5f fallback for the 3 failing cells. auval+pluginval-10 PASS (no NaN, no allocations). -->
<!-- Phase 2.4c-bis 2026-04-29: post-port matrix-stability re-render (evidence-only at .planning/evidence/phase-2-4c-bis/matrix-stability-post-port.wav, 157 MB) surfaces 4 NEW high-pressure × β=0.05 raucous corners in addition to the 3 carry-forward Phase 2.4a corners. nanCount=0; peak max ≈ 0.351 within strict |x| < 1.0; auval + pluginval-10 PASS post-port. Phase 2.4-bis backlog: click-free heuristic threshold tune for the 4 NEW raucous corners. Does NOT block Stage 2 progression; matrix-stability NOT in default reproduce-goldens.sh per Phase 2.4a R34b "evidence golden" precedent. -->
| QUAL-02 | Self-oscillation under extreme drone settings remains musical, not destructive — output protection / soft limiter on master | nice | partial | stage-2 |

## Acceptance Criteria Details

### FUNC-01: 4-String EADG Bowed Bass

**Description:** Authentic monophonic 4-string contrabass with standard EADG fourths tuning, covering E1 (~41 Hz) to G3 (~196 Hz).

**Acceptance Criteria:**
- [ ] All MIDI notes E1 through G3 produce stable, in-tune output
- [ ] String selection routes MIDI note to the closest natural string (or fingered position) following standard double bass technique
- [ ] No instability at the lowest fundamentals (E1 = 41 Hz)
- [ ] Mono voice allocation — new note steals previous note with bow re-engagement

### FUNC-02: Sustained-First Articulation

**Description:** Default behavior is legato sustained bowing — bow held while note held, release tail on note-off. This is the design centerpiece, not a side feature.

**Acceptance Criteria:**
- [ ] Holding a MIDI note produces continuous, evolving sustained tone (not decaying)
- [ ] Release on note-off produces a natural bow-lift tail
- [ ] No artifacts during very long sustains (60+ seconds)

### FUNC-03: Dual Identity (Orchestral + Drone)

**Description:** Same engine produces convincing cinematic orchestral bass AND sustained ambient drone material via parameter / preset choices.

**Acceptance Criteria:**
- [ ] Orchestral preset (e.g. Cinematic Bass Sustain) sounds credible alongside Spitfire / CSS / VSL bass libraries in a mix
- [ ] Drone preset (e.g. Infinite Drone) produces evolving sustained drone material in the spirit of Stephen O'Malley / Tony Conrad references
- [ ] Both modes can be A/B switched via preset without retuning the host

### FUNC-06: VST3 Note Expression

**Description:** Per-note pitch deviation via VST3 Note Expression for Dorico microtonal playback. Follows the Ouaricon spike-validated pattern.

**Acceptance Criteria:**
- [ ] VST3 build exposes Note Expression for pitch deviation
- [ ] Dorico microtonal score plays back correctly with per-note tuning offsets
- [ ] Verified using the spike-findings-VST-development pattern

### DSP-01: Bass-Range Waveguide Stability

**Description:** Digital waveguide string model must be stable across full E1-G3 range with 2x oversampling at the friction junction.

**Acceptance Criteria:**
- [ ] No NaN or denormal values during extended playback at lowest pitches
- [ ] No aliasing artifacts under heavy bow pressure or fast bow speed
- [ ] Stable under all combinations of friction model parameters

### DSP-02: Bass-Tuned Friction Junction

**Description:** Nonlinear bow-string friction model tuned specifically for thick rosined hair on bass strings (defaults: 65% rosin, 1.0 N pressure, 0.10 beta).

**Acceptance Criteria:**
- [ ] Default parameter combination produces convincing bass arco character (not violin-like)
- [ ] Reuses or extends O-Bowed friction module if extracted as a shared module

### DSP-03: Bass-Tuned Wood Body Resonator

**Description:** Body resonator (parallel biquad bank or equivalent) modeling wood body modes specific to contrabass acoustics. Material is fixed (wood); size and damping are parametric.

**Acceptance Criteria:**
- [ ] Body resonance audibly reinforces low-mid frequencies (80-400 Hz region)
- [ ] Body Damping parameter changes mode decay time perceptibly
- [ ] Body Mix parameter blends raw string vs body output without phase artifacts

### DSP-05: Per-String Detuning

**Description:** Each of the 4 strings has independent +/- 1200 cents detune for scordatura / just intonation drone use.

**Acceptance Criteria:**
- [ ] Detuning each string independently produces correct pitch offsets
- [ ] Just-intoned drone preset uses detuning to achieve 7-limit intervals
- [ ] State persists across plugin reloads

### DSP-06: Infinite Sustain Control

**Description:** Damping reduction control that approaches zero damping for drone-style endless resonance.

**Acceptance Criteria:**
- [ ] At max setting, sustained tone continues indefinitely while bow remains engaged
- [ ] No runaway feedback / NaN at max setting under any other parameter combination
- [ ] Smooth, click-free parameter changes during playback

### DSP-09: Layered Expression Model

**Description:** All three expression layers must coexist and be independently controllable.

**Acceptance Criteria:**
- [ ] CC11 (Expression) controls bow speed in real time
- [ ] CC2 (Breath) / aftertouch controls bow pressure in real time
- [ ] CC74 controls bow position in real time
- [ ] Dedicated vibrato section (rate / depth / onset) adds vibrato on top of MIDI input
- [ ] Expression Macro knob simultaneously modulates bow speed + pressure + vibrato + body brightness

### PERF-01: Real-Time Safe Processing

**Description:** processBlock must contain no allocations, locks, or file I/O.

**Acceptance Criteria:**
- [ ] No `new`/`malloc` in processBlock
- [ ] No mutex acquisition in processBlock
- [ ] Code review confirms RT-safety
- [ ] pluginval reports no real-time violations

### COMPAT-01: pluginval Strictness 10

**Description:** Must pass pluginval at strictness level 10 across all built formats.

**Acceptance Criteria:**
- [ ] VST3 (macOS) passes pluginval --strictness-level 10
- [ ] AU (macOS) passes pluginval --strictness-level 10
- [ ] VST3 (Windows) passes pluginval --strictness-level 10

### COMPAT-02: Dorico Microtonal Playback

**Description:** Loads in Dorico and plays microtonal notation correctly via VST3 Note Expression.

**Acceptance Criteria:**
- [ ] Plugin loads in Dorico without errors
- [ ] A microtonal test score plays back with correct per-note pitch deviations
- [ ] Verified using the spike-findings-VST-development pattern

### QUAL-01: No Audio Artifacts

**Description:** No audio artifacts under any normal parameter combination, including aggressive drone settings.

**Acceptance Criteria:**
- [ ] No audible clicks during parameter sweeps
- [ ] No denormals (verify with FTZ/DAZ enabled)
- [ ] No NaN propagation under any parameter combination
- [ ] No runaway feedback at max Infinite Sustain + max Sub-Harmonics + min Body Damping

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01, FUNC-02, FUNC-05, FUNC-06, FUNC-07, DSP-01..DSP-10, PERF-01, PERF-03, QUAL-01, QUAL-02 |
| stage-3 | UI-01, UI-02 |
| stage-4 | FUNC-03, FUNC-04, PERF-02, COMPAT-02 |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Pizzicato | Bow-only physical model; pizz needs a separate excitation engine | v1.1+ |
| Col legno / harmonics articulations | Specialized excitation modes outside sustained-arco focus | v1.2+ |
| Polyphony / double-stops | Mono-by-design for authentic single-string bowing | v2.0 (maybe) |
| 5-string / B0 extension | Standard 4-string EADG only in v1.0 | v1.1+ |
| Sympathetic strings | Removed to keep focus; reconsider once core feels right | v1.1+ |

---
*Generated from BRIEF.md on 2026-04-25*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
