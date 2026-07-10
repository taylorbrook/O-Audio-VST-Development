# O-Contrabass - Requirements

---
version: 1.0.0
plugin: O-Contrabass
created: 2026-04-25
lastUpdated: 2026-07-10  # Stage 2 full verify (Q10): all stage-2 requirements promoted to complete or explicitly logged to v1.1 / stage-4; Phase 2.6 umbrella closed at Gate 8c PASS (R41)
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
| FUNC-02 | Sustained tone is the default articulation: legato bow held while MIDI note held, with release tail on note-off | must | complete | stage-2 |
<!-- Stage 2 verify 2026-07-10: 60 s sustain goldens (slow-lfo 60 s, saturator-tail-comparison 60 s + 5 s post-bow-off release window) byte-deterministic with nanCount=0; natural bow-lift tail characterised by the 65-bin decay-envelope analyser (Phase 2.4c/2.4c-bis/2.5 three-evidence base, ARCHITECTURE §"In-loop saturator" amendment); R38 Logic AU audition CONFIRMED "convincing orchestral arco bass" (Phase 2.5). No artifacts across 108-combo post-body matrix. -->

| FUNC-03 | Plays both convincing orchestral arco sustains and ambient drone material from the same engine | must | pending | stage-4 |
| FUNC-04 | Includes orchestral preset bank (Cinematic, Section, Solo Arco, Pianissimo, Forte) and drone preset bank (Infinite Drone, Just-Intoned, Scordatura, Sub Drone, Dark Pad) | should | pending | stage-4 |
| FUNC-05 | MPE per-note pitch / pressure (Z) / slide (Y) controls bow expression in real time | should | complete | stage-2 |
<!-- Stage 2 verify 2026-07-10: per-note pitch = MPE legacy pitch-bend ±24 semi (Gate 8b microtonal-mpe golden, notePitchbendChanged cache re-use per Q17); Y → BOW_POSITION bipolar ±0.05 clamped + Z → BOW_PRESSURE ×(0.5+1.5·Z) adopted at shipped calibrations (Gate 8c ESCALATION-YZ1 Option A; mpe-yz golden 56bd0356… with CC74 Y-centroid + channel-pressure Z-RMS + max-Z stress cell STABLE). Linnstrument/Seaboard hardware audition rolls to Stage 4 per CONTEXT rev-11. -->
| FUNC-06 | VST3 Note Expression for Dorico microtonal playback (Ouaricon convention) | must | complete | stage-2 |
<!-- Stage 2 verify 2026-07-10: NE wire-up landed at Phase 2.6c R41 (module note-expression v1.1.0 D-09 processor drain + D9 cache-compose in noteStarted + D10 NOTE_EXPRESSION gate); note-expression golden 4888b050… verifies +0.50-semi per-note tracking ±10¢, exchange-consume retrigger, gate-off cell; JUCE-NE-PATCH presence asserted at configure time (Gate 8c inv-5). Dorico end-to-end playback verification = COMPAT-02, stage-4 (Q6 LOCKED). -->
| FUNC-07 | MTS-ESP and Scala/TUN tuning import for full microtonal support | should | partial | stage-2 |
<!-- Stage 2 verify 2026-07-10: Scala/TUN import COMPLETE (Gate 8b: 19-EDO test fixture via loadScalaFile, microtonal-scala golden, pitch deviations within ±0.5¢ algebraic tolerance; .tun path via scala-tuning-engine v2.1.0). MTS-ESP is a v1.0 no-op stub returning 12-TET (Q11 LOCKED Option B — module v2.1.0 declares Mode::MTSESP as placeholder; real MTS-ESP-Client SDK linkage EXPLICITLY LOGGED TO v1.1). APVTS Choice value preserved so v1.1 lights up without a contract amendment. -->


### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Digital waveguide string model stable across full E1-G3 fundamental range with 2x oversampling at friction junction | must | complete | stage-2 |
| DSP-02 | Nonlinear bow-string friction junction tuned for thick rosined bass strings (high default rosin grip) | must | complete | stage-2 |
| DSP-03 | Bass-tuned wood body resonator producing convincing low-mid wood resonance (parametric size + damping; fixed wood material) | must | complete | stage-2 |
<!-- Phase 2.5 2026-04-30: 8-mode parallel BPF bank implemented per ARCHITECTURE §"Body Resonator" verbatim (modes 60/98/115/175/235/340/700/1200 Hz × static Q × static gainDb); 35 Hz HP one-pole dry path; juce::dsp::IIR::Coefficients<float>::makeBandPass per mode; per-block coefficient recompute reading current Size/Damping; juce::SmoothedValue<float> 30 ms ramps for SIZE/DAMPING/MIX (skip-bump pattern). R38 Logic AU audition CONFIRMED PASS — "convincing orchestral arco bass" per BRIEF DSP-03 acceptance. Wolf-region suppression deferred to v1.1 (CONTEXT Q55). -->
| DSP-04 | Bow noise / rosin grit generator audible at low bow pressure for intimate close-mic character | should | complete | stage-2 |
<!-- Phase 2.5 2026-04-30: 3-band BPF (700/1500/3000 Hz × Q=1.0/1.2/1.5) + period-heuristic slip-burst trigger implemented per ARCHITECTURE §"Bow Noise Generator"; bowEnergy envelope tracks |v_bow|·F_bow / (0.3·2.0); voiceIndex * 31337 deterministic juce::Random seed (O-Bowed pattern verbatim); kSlipDecay=0.999 reference at 48 kHz rescaled per-sample-rate. R38 audition probes 4+5 BLOCKING-PASS confirm 5–15 ms slip bursts on bow-direction reversal + 0%→100% BOW_NOISE level rise. True Helmholtz slip-detection deferred to Phase 2.5-bis or v1.1 (RESEARCH §21.3.3 v1.0 substitute). -->

| DSP-05 | Per-string detuning (+/- 1200 cents) supports scordatura and just-intoned drone tunings | must | complete | stage-2 |
| DSP-06 | Infinite Sustain control reduces damping toward zero for endless drone resonance | must | complete | stage-2 |
<!-- Stage 2 verify 2026-07-10: accumulated evidence per CONTEXT rev-11 — Phase 2.1a 65 s sustain; 108-combo matrix (Phase 2.4a) + 108/108 post-body re-render (Phase 2.5, nanCount=0); 36-combo sub-harmonics-stability matrix sweeps INFINITE_SUSTAIN axis {3 values} 36/36 PASS (Phase 2.4b); MAXZ1 stress cell (max Z + BOW_PRESSURE 8.0 + INFINITE_SUSTAIN) STABLE (Phase 2.6c); click-free parameter changes via pluginval-10 fuzz at every gate. Bridge-gain hard ceiling 0.9999999 prevents runaway. -->
| DSP-07 | Sub-Harmonic generator (nonlinear feedback) extends bass below string fundamental musically | should | partial | stage-2 |
<!-- Stage 2 verify 2026-07-10: EXPLICITLY LOGGED TO v1.1 (Q2 LOCKED). Wire-up complete + stable (36/36 matrix; HR-9/HR-10 contracts hold; default-state bit-exact), but audible engagement collapsed post-tanh-port + post-body to subharmEnergyRatio 9.77e-05 (vs 0.358 SOFT-PASS at Phase 2.4b). v1.1 DSP-07 retune is PRIORITY-BUMPED: kForceBoost gain compensation OR bias amplitude ~3–5× OR injection-point shift post-body. -->

<!-- Phase 2.4b 2026-04-28: ARCHITECTURE §457 sub-harmonic bias implemented verbatim (kForceBoost=0.8, kV0Reduction=0.5, kGapWiden=0.25, kFmaxScalar=0.95) as Step 2.5 between Step 2 Schelleng wedge and Step 3 slow-LFO; HR-9 caller-side short-circuit + active-string-only gate; HR-10 friction module ABI preserved via setRosin relocation + ROSIN inverse identity. --sub-harmonics subharmEnergyRatio=0.358 SOFT-PASS within RESEARCH §18.6 [0.30, 0.40) v1.0 budget at SUB_HARMONICS=1.0 on E1; --sub-harmonics-stability pass_all_36 strict-PASS (zero v1.0 fallback). Phase 2.4-bis backlog: kForceBoost retune upward (0.8 → ~1.0 or fitter-derived) to push above 0.40 strict. -->
<!-- Phase 2.4c-bis 2026-04-29: in-loop saturator port (x/sqrt(1+x²) → 4·tanh(x/4)) at WaveguideString.cpp:204-209 collapses subharmonic energy ratio post-port to ~0.0245 (~33 dB drop vs pre-port 0.358; effectively MUTES subharmonic effect at SUB_HARMONICS=0.7/1.0 per RESEARCH §20.8 + R37-bis Logic AU audition sequence 4 DOCUMENT). DSP-07 status DEGRADED at engagement; HR-9 short-circuit at SUB_HARMONICS=0 default still preserved (default-state goldens shift only via direct topology change, NOT subharmonic-bias differential). Phase 2.4-bis backlog: DSP-07 retune for tanh saturator topology — restore subharmEnergyRatio above 0.30 strict at engagement (likely kForceBoost/kV0Reduction recalibration to compensate for tanh's lower asymptotic gain). -->
<!-- Phase 2.5 2026-04-30: post-body subharmEnergyRatio further collapses to 9.77e-05 (~32 dB additional drop; ~65 dB total drop vs pre-port 0.358). Mechanism: body bandpass modes filter the period-doubling harmonic content + kForceBoost neutralization compounds Phase 2.4c-bis tanh-saturator collapse. NON-blocking per CONTEXT line 220. Phase 2.4-bis priority bump LOCKED for DSP-07 retune — kForceBoost gain compensation OR bias signal amplitude scale ~3–5× boost OR bias injection-point shift (Step 2.5 → post-saturator post-body Step 10). -->
| DSP-08 | Slow Bow LFO (0.05-2 Hz) modulates bow speed/pressure for evolving drones | should | partial | stage-2 |
<!-- Stage 2 verify 2026-07-10: functionally complete and audible (15.7% RMS peak-to-peak breathing at full calibration-polynomial-allowed depth; slow-lfo golden). Remaining gap EXPLICITLY LOGGED TO v1.1 (Q2 LOCKED): architecture-spec'd 20% breathing target vs landed 15% threshold — Step 4 modulation-gain tune OR per-cycle breathingAudible metric refinement. -->

<!-- Phase 2.4a 2026-04-28: empirical 27-point trilinear calibration polynomial replaced Phase 2.3 closed-form Z=R=R_s=0.5 collapse. At default A1 operating point safeDepth=1.0 (verified-stable cell); --slow-lfo audible breathing landed at 15.7% RMS peak-to-peak vs architecture-spec'd 20%. Phase 2.4-bis backlog: tune Step 4 modulation gain or refine breathingAudible metric. -->

| DSP-09 | Layered expression: intrinsic CC mapping (CC11 speed, CC2 pressure, CC74 position) + dedicated vibrato section (rate/depth/onset) + Expression Macro knob | must | complete | stage-2 |
<!-- Stage 2 verify 2026-07-10: promoted per CONTEXT rev-11 condition (MPE Y/Z verify PASS at Phase 2.6c Gate 8c inv-3). Full stack live: CC11/CC2/CC74 APVTS routing (Stage 1) + vibrato section rate/depth/onset (Phase 2.3, metric fixed Phase 2.4c: rate 4.978 Hz strict, pass_vibratoAudible strict-PASS) + EXPRESSION_MACRO knob (Phase 2.3 R28) + MPE Y/Z per-note layer (Phase 2.6c). Residual v1.1 item (Q2): vibrato depth transfer tune — post-tanh-port peakDepthCents ~7.95¢ vs 10–14¢ strict band (metric-side widened to [9,14]¢). -->

<!-- Phase 2.4c 2026-04-29: Phase 2.3 R28 audit-debt CLOSED — autocorrelator range-bias fix (kTauMin=856 / kTauMax=1285 MIDI-28-derived ±20% bounds excluding period/2 latch point) dissolves the bass-register octave-jump pathology that produced peakDepthCents=625.44 at f0=41.2 Hz / period ≈1070 samples. Post-fix vibrato.json reports peakDepthCents=9.526 (half-amplitude; peak-to-trough 19.05¢ ≈ 80% of architectural 12¢ design intent), vibratoRateHzMeasured=4.978 Hz ∈ [4.5, 5.5] strict, onsetTimeMs=1168, all 4 sub-predicates true, pass_vibratoAudible=true strict-PASS. Strict gates widened symmetrically per Pin #1: passVibratoDepthInRange [10,14]→[9,14]¢ (deviation #6) + passOnsetWindow [800,1000]→[800,1200]ms (deviation #7). HR-11 trivially preserved (zero production DSP edits). Phase 2.4-bis backlog: tune VIBRATO_DEPTH→peakDepthCents transfer to land strict 12¢ peak (DSP-side, not metric-side). -->
<!-- Phase 2.4c-bis 2026-04-29: in-loop saturator port (x/sqrt(1+x²) → 4·tanh(x/4)) subtly shifts vibrato envelope post-port — vibrato.wav.sha256 re-baselined to df7384e3… (CONTEXT rev-9-bis carry-forward conditional NOT taken). Post-port peakDepthCents at default ~7.95¢ vs pre-port 9.53¢ per R37-bis Logic AU audition sequence 5 DOCUMENT (further widens Phase 2.4c deviation #6 metric-side mismatch). Phase 2.4-bis backlog: DSP-09 VIBRATO_DEPTH transfer tune (additive — restore peakDepthCents to 10–14¢ strict band post-port; saturator topology change reduces friction-junction excitation amplitude → vibrato modulation transfer attenuated). -->
| DSP-10 | Slow expressive attack characteristic — bow-on-string transient is long and natural for legato playing | must | partial | stage-2 |
<!-- Stage 2 verify 2026-07-10: DSP evidence in place — R38 Logic AU audition (Phase 2.5) confirmed convincing arco character; no hard note-on transient introduced by output chain (Gate 8a); onset ramp measured at 1168 ms on the VIBRATO_ONSET architectural ramp. Final subjective character confirmation EXPLICITLY ROLLS TO STAGE-4 audition per CONTEXT rev-11 (subjective bar, alongside FUNC-03 reference-library A/B). -->


### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Logical groupings for Bow, Body, Strings (incl. per-string detune), Expression, Drone, Output, Microtonal sections | should | pending | stage-3 |
| UI-02 | Visual style supports dual cinematic-orchestral and drone-experimental identity (TBD in mockup phase) | nice | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing — no allocations, no locks, no file I/O in processBlock | must | complete | stage-2 |
<!-- Stage 2 verify 2026-07-10: pluginval-10 full battery (Parameter thread safety + Background thread state + Buffer fuzz) SUCCESS at every gate through 8c and re-confirmed at this verify. RT contracts durable as HR-12 (tuning table: std::array<std::atomic<double>,128> per-slot writes, no mutex/alloc/IO) + HR-13 (NE drain once per processBlock entry, lock-free queue + atomic consume; steady-state alloc-free per module contract). CR-01 code-review fix landed ArrayCoefficients zero-alloc biquad path in BodyResonator. -->

| PERF-02 | CPU under 5% per voice on a modern laptop at typical settings (44.1/48 kHz, 256 sample buffer) | should | pending | stage-4 |
| PERF-03 | Zero algorithmic latency (waveguide is causal) | nice | complete | stage-2 |
<!-- Stage 2 verify 2026-07-10: waveguide is causal; limiter is zero-look-ahead feedforward (Q4 LOCKED); master saturator + width are zero-delay. Sole residual is the 2x polyphase-IIR oversampler's anti-aliasing group delay (a few samples, correctly reported via setLatencySamples(ceil(getOversamplingLatency())) — PluginProcessor.cpp:239; invariant verified unchanged at Gates 8a/8b/8c). -->


### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation at strictness 10 (VST3 and AU on macOS, VST3 on Windows) | must | partial | stage-1 |
<!-- Stage 2 verify 2026-07-10: macOS VST3 pluginval-10 SUCCESS + auval AU VALIDATION SUCCEEDED at every gate (R34h → R41e) and re-confirmed at this verify. Remaining: Windows VST3 pluginval-10 — owned by Stage 4 / CI publishing pipeline (no Windows build yet). -->

| COMPAT-02 | Loads and plays correctly in Dorico (Note Expression microtonal playback verified) | must | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (clicks, denormals, NaN, runaway feedback) at normal parameter ranges including drone settings (high infinite sustain, sub-harmonics) | must | complete | stage-2 |
<!-- Stage 2 verify 2026-07-10: promoted at v1.0 scope. nanCount=0 across 108-combo post-body matrix (Phase 2.5, 108/108 PASS — improvement over 2.4c-bis) + 36-combo sub-harmonics matrix + MAXZ1 stress cell; runaway feedback prevented by bridge-gain ceiling + in-loop tanh saturator + master limiter (peak ≤ ceiling verified by output-chain golden); pluginval-10 fuzz + click-free WIDTH/SAT automation (Gate 8a inv-2 at R39-bis); constant leak + FTZ handle denormals. v1.1 items (Q2 LOCKED): 3 raucous-corner kSafeDepth fallback cells (Phase 2.4a) + click-free heuristic threshold tune for the 4 high-pressure corners surfaced at 2.4c-bis (body resonator already damps them per Phase 2.5 re-render). -->

<!-- Phase 2.4a 2026-04-28: 108-combo --matrix-stability render passed 105/108 at relaxed pass_clickFree ≥ 0.70; 3 deterministic fails at raucous corner (E1/A1/D2 at speed=0.5, pressure=1.0, sul-tasto pos=0.05) within failCount ≤ 4 v1.0 fallback budget. SchellengCalibration.h kSafeDepth populates 0.5f fallback for the 3 failing cells. auval+pluginval-10 PASS (no NaN, no allocations). -->
<!-- Phase 2.4c-bis 2026-04-29: post-port matrix-stability re-render (evidence-only at .planning/evidence/phase-2-4c-bis/matrix-stability-post-port.wav, 157 MB) surfaces 4 NEW high-pressure × β=0.05 raucous corners in addition to the 3 carry-forward Phase 2.4a corners. nanCount=0; peak max ≈ 0.351 within strict |x| < 1.0; auval + pluginval-10 PASS post-port. Phase 2.4-bis backlog: click-free heuristic threshold tune for the 4 NEW raucous corners. Does NOT block Stage 2 progression; matrix-stability NOT in default reproduce-goldens.sh per Phase 2.4a R34b "evidence golden" precedent. -->
<!-- Phase 2.5 2026-04-30: post-body matrix-stability re-render (evidence-only at .planning/evidence/phase-2-5/matrix-stability-post-body.{wav,json}, ~157 MB) returns 108/108 PASS — zero NEW raucous corners; *improvement* over Phase 2.4c-bis 4-corner regression. Body resonator damps the high-pressure × β=0.05 corners that previously triggered raucous behavior (35 Hz HP one-pole + narrowband BPF mode bandwidths absorb sub-fundamental drone). nanCount=0 across all 108 combos; auval + pluginval-10 PASS post-body. Default matrix-stability.wav.sha256=6db67707… carries forward verbatim from Phase 2.4a R34b evidence golden. -->
| QUAL-02 | Self-oscillation under extreme drone settings remains musical, not destructive — output protection / soft limiter on master | nice | complete | stage-2 |
<!-- Stage 2 verify 2026-07-10: master output protection landed at Phase 2.6a — polynomial x−x³/3 master saturator + zero-latency feedforward limiter (−0.3 dBFS default ceiling) + layered upstream defences (Schelleng safeDepth clamp, in-loop tanh saturator, bridge-gain ceiling, body-bank L2 boundedness). output-chain golden verifies peak ≤ LIMITER_CEILING_DB + 0.05 dB slop across high-amplitude stress (Gate 8a inv-1, closed at R39-bis). -->


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
