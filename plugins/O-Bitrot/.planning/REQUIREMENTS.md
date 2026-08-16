# O-Bitrot - Requirements

---
version: 1.0.0
plugin: O-Bitrot
created: 2026-08-14
lastUpdated: 2026-08-16
stage2Verified: 2026-08-15
stage3Verified: 2026-08-15
stage4Verified: 2026-08-16
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 18
**Coverage:** must: 12 | should: 5 | nice: 1

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Clocked stochastic state machine re-rolls degradation state per clock tick over a shared circular buffer | must | complete | stage-2 |
| FUNC-02 | All six degradation families implemented: tape, CD skip, vinyl, packet loss, codec, crush | must | complete | stage-2 |
| FUNC-03 | Clock runs tempo-synced (1/16–1 bar) by default with a free-running Hz mode | must | complete | stage-2 |
| FUNC-04 | Seeded deterministic randomness: identical seed + input + params produces identical output; reseed control rolls new variations | must | complete | stage-2 |
| FUNC-05 | Per-module enable + parameter sections (no macro dice knobs) | must | complete | stage-2 |
| FUNC-06 | Global dry/wet mix and hardEdges (crossfade bypass) toggle | should | complete | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Tape: interval-table speed bends and tape-stops, all rate transitions ramped (default ~150 ms, adjustable) | must | complete | stage-2 |
| DSP-02 | CD skip: severity ladder — LPF concealment dip, ms mute + tick, hard-edged segment loop with synthesized restart chirp | must | complete | stage-2 |
| DSP-03 | Vinyl: revolution-quantized jumps (1.8 s @ 33⅓ / 1.33 s @ 45) with synthesized pops; pitch never changes | must | complete | stage-2 |
| DSP-04 | Packet loss: Gilbert–Elliott 2-state Markov over 20 ms packets with 4 concealment modes | must | complete | stage-2 |
| DSP-05 | Codec: 300–3400 Hz BP → 8 kHz downsample → μ-law round trip; GSM 06.10 mode via vendored libgsm with latency reported | must | complete | stage-2 |
| DSP-06 | Crush: fractional-bit quantization + fractional-hold SRR with interpolated latch; all sweeps zipper-free (smoothed targets, no phase resets) | must | complete | stage-2 |
| DSP-07 | Envelope-driven dynamic bit depth with duck/pump polarity, per-sample follower | should | complete | stage-2 |
| DSP-08 | TPDF dither knob (0–2 LSB) and S&H jitter on the crush stage | nice | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Six per-module panels plus global strip (clock, seed/dice, hard edges, mix) | should | complete | stage-3 |
| UI-02 | Reseed dice button and clock sync/free toggle | should | complete | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock) | must | complete | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | complete | stage-1 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No unintended audio artifacts: jumps crossfaded 1–5 ms unless hardEdges; no NaN/Inf under pathological input | must | complete | stage-2 |
| QUAL-02 | Block-size invariance: 512-vs-4096 renders match (per-sample followers, per-subsystem RNG streams) | should | complete | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Clocked stochastic state machine

**Description:** One continuously-written circular buffer feeds per-channel variable-rate read heads; a clock tick probabilistically re-rolls the failure state per enabled module.

**Acceptance Criteria:**
- [ ] Glitch events land only on clock-tick boundaries (rendered output shows event onsets quantized to the clock grid)
- [ ] Circular buffer span covers max vinyl revolution (1.8 s) + ramp headroom, enforced by static_assert

### FUNC-02: Six degradation families

**Acceptance Criteria:**
- [ ] Each family independently enabled produces its documented artifact class in a render-harness probe
- [ ] All families disabled = bit-transparent passthrough (minus reported latency)

### FUNC-03: Clock sync/free

**Acceptance Criteria:**
- [ ] Sync mode: event grid follows host tempo changes
- [ ] Free mode: event rate matches the Hz setting independent of tempo

### FUNC-04: Seeded randomness

**Acceptance Criteria:**
- [ ] Two offline renders with the same seed/input/params are bit-identical
- [ ] Reseed produces a different event sequence; seed round-trips through save/restore state

### DSP-01: Tape bends

**Acceptance Criteria:**
- [ ] Speed transitions show continuous ramps (no steps) in an instantaneous-frequency probe
- [ ] Tape-stop reaches silence via ramp to zero rate, no click

### DSP-02: CD skip ladder

**Acceptance Criteria:**
- [ ] Loop mode repeats a segment at exact intervals with a chirp at each restart, then jumps forward on recovery

### DSP-03: Vinyl jumps

**Acceptance Criteria:**
- [ ] Jump distances are integer multiples of the revolution period for the selected RPM
- [ ] No pitch change across a jump (autocorrelation pitch probe)

### DSP-04: Packet loss

**Acceptance Criteria:**
- [ ] Loss events cluster in bursts (measured burst-length distribution vs geometric expectation)
- [ ] All 4 concealment modes audibly distinct in harness renders

### DSP-05: Codec chain

**Acceptance Criteria:**
- [ ] μ-law path bandwidth limited to ~300–3400 Hz with level-dependent quantization noise
- [ ] GSM mode reports its framing latency via setLatencySamples in prepareToPlay

### DSP-06: Zipper-free crush

**Acceptance Criteria:**
- [ ] Full-range bits/rate sweeps pass a liveness-gated zipper probe (param verified wired before the no-zipper assertion)
- [ ] Fractional (non-integer) rates and bit depths render without warble/periodicity error

### PERF-01 / COMPAT-01 / QUAL-01

**Acceptance Criteria:**
- [ ] No allocations, locks, or logging in processBlock
- [ ] pluginval strictness 10 passes on VST3 and AU
- [ ] Pathological input (DC, silence, full-scale square, NaN injection at input) never produces sticky NaN/Inf output

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-*, DSP-*, PERF-01, QUAL-* |
| stage-3 | UI-* |
| stage-4 | COMPAT-*, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Macro dice knobs / global chaos macro | Per-module control chosen in ideation | v1.1+ |
| STFT MP3-artifact (holes/birdies) codec mode | Research assigns spectral territory to O-Lossy | O-Lossy |
| Bit-flip / wrong-decode PCM corruption modes | Additive flavor, not core to v1.0 | v1.1+ |
| Freeze/latch of a captured glitch pattern | Seeded reseed chosen instead | v1.1+ |

---
*Generated from BRIEF.md on 2026-08-14*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
