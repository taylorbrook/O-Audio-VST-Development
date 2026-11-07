# O-Granulator - Requirements

---
version: 1.0.0
plugin: O-Granulator
created: 2026-11-03
lastUpdated: 2026-11-06
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 30
**Coverage:** must: 20 | should: 8 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Live input continuously recorded into a ring buffer (1–30 s active length) | must | complete | stage-2 |
| FUNC-02 | Freeze latch holds buffer contents; unfreeze resumes recording without clicks | must | complete | stage-2 |
| FUNC-03 | Four independent grain streams reading the shared buffer, each with enable/level/pan | must | complete | stage-2 |
| FUNC-04 | Free (grains/s) and tempo-synced density/size modes with onset jitter | must | complete | stage-2 |
| FUNC-05 | Feedback network: grain output written back to buffer through tone filter, saturation, diffuser | must | complete | stage-2 |
| FUNC-06 | Modulation matrix routing LFOs, env follower, pitch follower, onset follower, macros to continuous params | must | complete | stage-2 |
| FUNC-07 | Dry/wet mix, input and output gain | must | complete | stage-2 |
| FUNC-08 | Onset follower can trigger grain bursts | should | complete | stage-2 |
| FUNC-09 | Factory presets covering ambient, harmoniser, rhythmic, feedback-cascade use cases | should | complete | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Per-grain pitch ±24 st with random spread and scale quantisation | must | complete | stage-2 |
| DSP-02 | Band-limited reads for pitched-up grains (grain-engine BandlimitedReader) | must | complete | stage-2 |
| DSP-03 | Window morph Hann→Tukey→Trapezoid→Exponential with skew; overlap-normalised level | must | complete | stage-2 |
| DSP-04 | Per-grain reverse probability, position spray, size/gain randomisation, stereo spread | must | complete | stage-2 |
| DSP-05 | Per-grain FX: SVF (LP/BP/HP) with cutoff randomisation, drive, bitcrush with probability, ring mod with randomised carrier | must | complete | stage-2 |
| DSP-06 | Feedback loop gain bounded — no runaway at feedback 100 % with any settings | must | complete | stage-2 |
| DSP-07 | Output reverb/diffuser stage | should | complete | stage-2 |
| DSP-08 | Pitch follower (YIN/MPM-class) usable as mod source | should | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | All parameters reachable; per-stream controls organised by stream selector/strips | must | complete | stage-3 |
| UI-02 | Buffer waveform display with write head, per-stream position/spray regions, live grain indicators | should | complete | stage-3 |
| UI-03 | Mod matrix view showing source → destination → depth; mod amount visible on targets | should | complete | stage-3 |
| UI-04 | EN/FR/zh-Hans localisation consistent with the suite | nice | complete | stage-4 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations/locks in processBlock, incl. grain spawn/steal) | must | complete | stage-2 |
| PERF-02 | Shared grain pool (~128) with graceful cap; CPU budget defined in planning and met at 48 kHz/128 samples | must | complete | stage-2 |
| PERF-03 | Block-size invariant output (seeded null across block sizes) | should | complete | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU), strictness 10 | must | complete | stage-1 |
| COMPAT-02 | State save/restore round-trips all parameters and mod matrix routings | must | complete | stage-4 |
| COMPAT-03 | Mono and stereo I/O; Windows VST3 build | should | complete-pending-CI | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (clicks, zipper) at normal parameter ranges, including freeze toggle and grain steal | must | complete | stage-2 |
| QUAL-02 | No NaN/denormal output under extreme settings (max feedback, max resonance, max density) | must | complete | stage-2 |
| QUAL-03 | Per-sample smoothing on audible continuous params and mod-matrix targets | must | complete | stage-2 |
| QUAL-04 | Hands-on listening pass in DAW across use cases | nice | complete | stage-4 |

## Acceptance Criteria Details

### FUNC-01: Live ring buffer
- [ ] Input audible in grain output within one grain onset at position 0
- [ ] bufferLength change does not click or read stale/invalid regions

### FUNC-02: Freeze
- [ ] With freeze on, buffer contents byte-identical across 10 s of new input
- [ ] Freeze on/off transitions produce no step discontinuity (click gate vs same-motion control)

### FUNC-03: Four streams
- [ ] Each stream independently audible with distinct params; disabled streams spawn no grains
- [ ] Streams share one pool; total grains never exceed pool cap

### FUNC-04: Free + sync timing
- [ ] Synced onsets land on host grid within 1 sample at jitter 0
- [ ] Free density matches set rate within 2 % over 10 s at jitter 0

### FUNC-05: Feedback network
- [ ] Feedback > 0 audibly recirculates grains; tone/saturation/diffusion each change loop output
- [ ] Feedback with freeze behaves per the planning decision (documented)

### FUNC-06: Mod matrix
- [ ] Every source can modulate any continuous destination; depth bipolar
- [ ] Removing a routing returns the target to its base value without a step

### FUNC-07: Mix / gain
- [ ] mix 0 % = dry null (latency-compensated); mix 100 % = no dry

### DSP-01: Pitch + quantisation
- [ ] Quantised grains land on scale degrees within 1 cent
- [ ] ±24 st extremes render without aliasing beyond DSP-02 limits

### DSP-02: Band-limited reads
- [ ] +24 st grain on a swept sine: aliased energy ≥ 60 dB below fundamental

### DSP-03: Window morph
- [ ] Output RMS stays within ±1 dB across the window morph range at constant density/size

### DSP-04: Grain randomisation
- [ ] reverseProb 0/100 % yields 0/100 % reversed grains; 50 % within statistical bounds
- [ ] Spray/spread 0 produce deterministic positions/centre pan

### DSP-05: Per-grain FX
- [ ] Filter, drive, crush, ring each audibly per-grain (randomised values differ between grains)
- [ ] Per-grain FX state reset on spawn — no bleed from previous grain in slot

### DSP-06: Feedback bounded
- [ ] 60 s at feedback 100 %, max density, pitch +12: peak ≤ +6 dBFS, no NaN

### PERF-01: RT safety
- [ ] Allocation gate shows zero allocations in processBlock over spawn/steal/freeze/matrix edits

### PERF-02: CPU budget
- [ ] Worst-case preset within the budget set in planning; pool cap engages instead of overload

### COMPAT-01: pluginval
- [ ] VST3 + AU pass strictness 10; auval PASS

### COMPAT-02: State
- [ ] All params + matrix routings restored after save/reopen (fresh instance)

### QUAL-01 / QUAL-02 / QUAL-03
- [ ] Click gates pass on freeze toggle, grain steal, param sweeps
- [ ] No NaN/Inf/denormals under extreme-settings stress render
- [ ] Sweeps of position/pitch/cutoff/mix free of zipper noise

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01–08, DSP-*, PERF-*, QUAL-01–03 |
| stage-3 | UI-01–03 |
| stage-4 | FUNC-09, UI-04, COMPAT-02–03, QUAL-04 |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Serial stream routing (stream feeds stream) | Complexity; parallel streams chosen | v1.1+ |
| Loop-record capture mode | Ring + freeze chosen | v1.1+ |
| MIDI-triggered/pitched grains | Density + sync chosen | v1.1+ |
| Sample/file loading | Live-input effect | — |

---
*Generated from BRIEF.md on 2026-11-03*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
