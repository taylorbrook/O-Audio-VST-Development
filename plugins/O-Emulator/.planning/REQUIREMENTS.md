# O-Emulator - Requirements

---
version: 1.0.0
plugin: O-Emulator
created: 2026-08-20
lastUpdated: 2026-08-21
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 15
**Coverage:** must: 10 | should: 4 | nice: 1

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Console selector switches the full emulation pipeline (codec, rate, interpolation, output stage) between SNES, PS1, NES, Game Boy, and Genesis modes | must | complete | stage-2 |
| FUNC-02 | Four macro knobs (Crush, Age, Reverb, Mix) shape character within the selected console mode | must | complete | stage-2 |
| FUNC-03 | Console reverb is routable in every console mode, not just PS1 | should | complete | stage-2 |
| FUNC-04 | Switching consoles mid-playback is click-safe (no pops or stuck audio) | should | complete | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Authentic codec round-trips per console: SNES BRR encode→decode, PS1 SPU-ADPCM, NES DPCM, Game Boy 4-bit wave quantization, Genesis 8-bit low-rate DAC playback | must | complete | stage-2 |
| DSP-02 | Per-console fixed internal sample rates with resampling into/out of the console domain, including each console's authentic interpolation (e.g. SNES 4-tap Gaussian) | must | complete | stage-2 |
| DSP-03 | Per-console output stage model: DAC/output lowpass, DAC nonlinearity, headroom clipping | must | complete | stage-2 |
| DSP-04 | SPU-style reverb algorithm faithful to PS1-era character (murky, short, metallic) | must | complete | stage-2 |
| DSP-05 | Age model: noise floor, hum, filter dulling, and drift scale continuously with the Age knob | should | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Console selector is the focal UI element with four macro knobs (Crush, Age, Reverb, Mix) | should | partial | stage-3 |
| UI-02 | Factory presets showcasing each console's signature sound | nice | complete | stage-4 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock) | must | complete | stage-2 |
| PERF-02 | Block-size invariant output (offline bounce matches real-time playback) | must | complete | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | complete | stage-1 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No unintended audio artifacts: codec artifacts are the product, but no NaN/inf, denormal stalls, or level blowups at any parameter combination | must | complete | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Console selector switches the full pipeline

**Description:** Selecting a console swaps codec, internal rate, interpolation, and output-stage model as one coherent mode.

**Acceptance Criteria:**
- [x] Each of the 5 modes produces audibly distinct, mode-appropriate output on the same source material
- [x] Rendered output per mode changes codec behavior AND resampling rate (verified in render harness)

### FUNC-02: Macro knobs

**Description:** Crush, Age, Reverb, Mix each audibly and measurably affect the signal in every console mode.

**Acceptance Criteria:**
- [x] Each knob at min vs. max produces measurably different render output in all 5 modes (liveness-gated probe)
- [x] Mix at 0% is bit-transparent dry (minus latency compensation)

### DSP-01: Authentic codec round-trips

**Description:** Audio genuinely passes through each console's encode→decode path.

**Acceptance Criteria:**
- [x] SNES mode: BRR 4-bit ADPCM block encode→decode implemented (16-sample blocks, filter modes 0–3)
- [x] PS1 mode: SPU-ADPCM 4-bit round-trip implemented (28-sample blocks)
- [x] NES/GB/Genesis modes: DPCM delta, 4-bit wave, and 8-bit quantization paths respectively

### DSP-02: Fixed rates + authentic interpolation

**Acceptance Criteria:**
- [x] SNES domain runs at 32 kHz with 4-tap Gaussian interpolation on output
- [x] Spectral analysis of white-noise render shows the expected per-console alias/rolloff signature

### DSP-03: Output stage model

**Acceptance Criteria:**
- [x] Per-console output filter corner measurably differs between modes
- [x] Hot input clips in the console domain without NaN or sticky states

### DSP-04: SPU reverb

**Acceptance Criteria:**
- [x] Reverb tail character matches SPU reference behavior (comb/all-pass structure, short murky decay)
- [x] Reverb send works in all 5 console modes

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [x] No heap allocation, locks, or file I/O in processBlock (code audit)
- [x] Codec buffers pre-allocated in prepareToPlay

### PERF-02: Block-size invariance

**Acceptance Criteria:**
- [x] Render harness output digest identical across block sizes (64/512/4096) at same settings

### COMPAT-01: pluginval

**Acceptance Criteria:**
- [x] pluginval strictness 10 passes for VST3 and AU on macOS (verified 2026-08-20, Stage 1 — re-run independently at verify phase)

### QUAL-01: No unintended artifacts

**Acceptance Criteria:**
- [x] Pathological input probe (silence, DC, full-scale noise, NaN-adjacent denormals) produces bounded, finite output at all parameter extremes

### UI-01: Console selector focal + 4 macro knobs

**Acceptance Criteria (stage-3 verify, 2026-08-21):**
- [x] Selector focal (5 segments) + CRUSH/AGE/REVERB/MIX knobs implemented per CONTEXT sketch at 620×430
- [x] All 5 params bound two-way via relays; `data-param` IDs match parameter-spec.md exactly; bridge audit 0↔0 native fns
- [x] Render harness digests identical to Stage-2 baseline (GUI touched no DSP); pluginval 10 + auval re-pass
- [ ] HUMAN: live automation → UI refresh, preset-load refresh, visual/interaction pass (folds into Stage-4 DAW pass)

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01..04, DSP-01..05, PERF-01..02, QUAL-01 |
| stage-3 | UI-01 (partial — human gates pending) |
| stage-4 | UI-02 (deferred from stage-3), COMPAT-*, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| CRT TV / small-speaker simulation | Explicitly deselected during ideation | v1.1+ |
| Adjustable sample rate per console | Authentic fixed rates chosen for v1.0 | v1.1+ |
| Additional systems (N64, arcade, PS2) | Scope control — 5 consoles at launch | v1.1+ |
| SNES echo-buffer delay as separate authentic algorithm | Single SPU-style reverb covers all modes in v1.0 | v1.1+ |

---
*Generated from BRIEF.md on 2026-08-20*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
