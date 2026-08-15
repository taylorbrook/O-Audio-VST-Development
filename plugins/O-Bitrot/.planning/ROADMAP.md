# O-Bitrot - Implementation Plan

**Date:** 2026-08-15
**Complexity Score:** 5.0 (Complex — raw 12.0, capped)
**Strategy:** Phase-based implementation (staged)

---

## Complexity Factors

- **Parameters:** 31 parameters → 31/5 = 6.2, capped at **2.0**
- **Algorithms:** 9 DSP components = **9**
  - MediaClock (sync/free tick engine)
  - CaptureRing + ReadHead engine (variable-rate reads, jump crossfades)
  - TapeTransport (ramped varispeed bends/stops)
  - CDSkip (CIRC ladder + chirp/tick synthesis)
  - VinylTransport (revolution jumps + pop synthesis)
  - PacketLossStage (Gilbert–Elliott + 4 concealments)
  - CodecStage (phone chain: BP → 8 kHz → μ-law/GSM)
  - CrushStage (fractional-hold SRR + jitter)
  - QuantStage (fractional bits + TPDF dither + env-driven depth)
- **Features:** **1** point
  - Modulation system: per-sample envelope follower driving dynamic bit depth (+1)
  - (No FFT, no feedback loops, no multiband, no external MIDI)
- **Total:** 2.0 + 9 + 1 = 12.0 → **capped at 5.0**

---

## Stages

- Stage 0: Research & Planning ✓ (this document + `research/ARCHITECTURE.md`)
- Stage 1: Foundation ← Next (build system, APVTS 31 params, passthrough shell, COMPAT-01 gate)
- Stage 2: DSP — **5 phases** (below)
- Stage 3: GUI — **3 phases** (below)
- Stage 4: Polish/Validation — presets, pluginval strictness 10, changelog

---

## Stage 2: DSP Phases

### Phase 2.1: Engine Core + Tape

**Goal:** The infrastructure everything rides on — ring, clock, RNG bank, crossfaded read head,
latency scheme, dry/wet — proven with the Tape family.

**Components:** CaptureRing (static_assert span), ReadHead + `clampAndScheduleJump()` choke point,
MediaClock (sync via updateBeatSync port + free mode, sample-accurate split-block ticks), RngBank
(splitmix-derived streams, reseed-on-change), arbitration skeleton, TapeTransport (bends + stops,
TAPE_RAMP), DryWetMixer with constant 20 ms latency scheme, HARD_EDGES, enable fades.

**Test Criteria:**
- [ ] All-off passthrough is bit-transparent minus reported latency (FUNC-02 probe)
- [ ] Tape bends/stops show continuous instantaneous-frequency ramps, no clicks (DSP-01)
- [ ] Same seed ⇒ bit-identical offline renders; different seed ⇒ different events (FUNC-04)
- [ ] 512-vs-4096 block renders bit-identical (QUAL-02)
- [ ] Sync mode follows host tempo; free mode matches Hz (FUNC-03)
- [ ] Offline DSP render harness in place (repo Stage-2 gate pattern)

### Phase 2.2: CD Skip + Vinyl

**Goal:** Remaining read-head transport families + artifact synthesis.

**Components:** CDSkip ladder (TPT-LPF conceal dip, mute+tick, segment loop + chirp, recovery
jump-forward), VinylTransport (revolution-quantized jumps, locked groove, pop synth), full
arbitration across three families.

**Test Criteria:**
- [ ] Loop mode repeats at exact CD_SEGMENT intervals with restart chirp, jumps forward on recovery (DSP-02)
- [ ] Vinyl jump distances are integer revolution multiples; autocorrelation shows no pitch change (DSP-03)
- [ ] Event onsets quantized to the clock grid (FUNC-01)
- [ ] Multi-family collisions arbitrate deterministically per seed

### Phase 2.3: Packet Loss

**Goal:** GE Markov overlay with all four concealments.

**Components:** 20 ms packet grid, GE state machine (π_B/E[B] mapping), Silence/Repeat/Decay/
Substitute (AMDF period estimate with Decay fallback), packet-boundary crossfades.

**Test Criteria:**
- [ ] Burst-length distribution matches geometric expectation (DSP-04)
- [ ] Four concealment modes audibly distinct in harness renders
- [ ] Block-size invariance still holds with packet grid active

### Phase 2.4: Crush + Quant

**Goal:** SRR + quantizer stage, fully sweepable.

**Components:** CrushStage (fractional-hold latch, target-smoothed rate, jitter), QuantStage
(fractional mid-tread bits, TPDF dither, per-sample env follower with duck/pump, NaN hygiene).

**Test Criteria:**
- [ ] Full-range bits/rate sweeps pass a **liveness-gated** zipper probe (DSP-06)
- [ ] Fractional rates/depths render without warble/periodicity error
- [ ] Env-driven depth behaves per polarity; offline bounce matches realtime (DSP-07, QUAL-02)
- [ ] Pathological input (DC, silence, full-scale square, NaN injection) never yields sticky NaN/Inf (QUAL-01)

### Phase 2.5: Codec (μ-law + GSM)

**Goal:** Phone chain, vendored libgsm, latency verified — the isolated-risk phase.

**Components:** mono sum, 300/3400 Hz IIR cascades (ArrayCoefficients), 8 kHz latch grid, μ-law
round trip, vendored `third_party/libgsm` (license file check, static lib target), 160-sample frame
round trip, mode crossfade, path delay alignment, CODEC_MIX.

**Test Criteria:**
- [ ] μ-law path bandwidth ~300–3400 Hz with level-dependent quantization noise (DSP-05)
- [ ] GSM frame round trip verified in harness before integration; latency = reported 20 ms in all modes
- [ ] libgsm license file vendored and recorded; no GPL code anywhere
- [ ] pluginval-strictness-10 local run clean (repo pattern: catches latent NaN)

---

## Stage 3: GUI Phases

### Phase 3.1: Layout and Basic Controls

**Goal:** Six-panel + global-strip WebView layout from finalized mockup.

**Components:** mockup HTML → `Source/ui/public/index.html`, WebView setup (unique_ptr order,
resource provider explicit mapping, `check_native_interop.js`, `NEEDS_WEB_BROWSER TRUE`), panel
scaffolding.

**Test Criteria:**
- [ ] WebView opens at correct size; six panels + global strip render per mockup (UI-01)
- [ ] ES6 module loading correct (`type="module"`, imported `getSliderState`)

### Phase 3.2: Parameter Binding

**Goal:** All 31 params two-way bound.

**Components:** relays/attachments (3-arg JUCE 8 form), `getToggleState` for the 7 Bools,
`getComboBoxState` for the 5 Choices, readouts via `SliderState.getScaledValue()` (skew-safe),
relative-drag knobs, enable-dimming of panels.

**Test Criteria:**
- [ ] Every control moves DSP; host automation and preset load update UI
- [ ] Grep-diff getNativeFunction vs withNativeFunction — no bridge gaps (repo pattern)

### Phase 3.3: Dice, Clock Toggle, Polish

**Goal:** Reseed dice button + sync/free UX (UI-02).

**Components:** dice button → random 0–9999 written to SEED via param API; CLOCK_MODE toggle showing
either SYNC_DIV or FREE_RATE control; seed readout; final visual polish.

**Test Criteria:**
- [ ] Dice rolls new seed, seed persists through save/restore (FUNC-04)
- [ ] Sync/free toggle swaps the visible clock control without dead params

---

## Implementation Flow

- Stage 1: Foundation — CMake, APVTS (versioned ParameterIDs, 31 params), passthrough, pluginval (COMPAT-01)
- Stage 2: DSP — phases 2.1 → 2.5 (each phase = git commit + harness probes green)
- Stage 3: GUI — phases 3.1 → 3.3
- Stage 4: Validation — factory presets (author in engineering units + convertTo0to1, repo pattern),
  pluginval strictness 10 (VST3 + AU), CHANGELOG, NOTES

---

## Implementation Notes

### Thread Safety
- Cached `std::atomic<float>*` param pointers; no locks/allocations/logging in processBlock (PERF-01).
- Reseed = message-thread parameter write; audio thread detects seed change per block.

### Performance
- ≈ 10–15 % single core @ 48 kHz worst case (all families + GSM). No FFT, no oversampling.

### Latency
- Constant `ceil(0.020·fs)` samples reported once in `prepareToPlay()` (`setLatencySamples`);
  all wet paths aligned; `DryWetMixer::setWetLatency` aligns dry. Fallback documented in
  ARCHITECTURE.md Implementation Risks if 20 ms constant is rejected at verify.

### Denormals / NaN
- `ScopedNoDenormals`; follower input sanitized; TPT filters for swept cutoffs; fixed IIR
  coefficients computed in prepareToPlay only.

### Known Challenges (with repo precedents)
- Sample-accurate mid-block state transitions → split-block processing (QUAL-02).
- RNG consumption order must be block-size independent → consume only at ticks/packets, one stream
  per subsystem (`pattern_rng_stream_interleave_blocksize`).
- Write-then-read ring order (`pattern_grain_read_before_capture_write_blocksize`).
- `AudioParameterChoice` needs ≥ 2 entries even in the GSM-fallback scenario.
- libgsm on MSVC: compile as separate C target with relaxed warnings.

---

## References

- Creative brief: `plugins/O-Bitrot/.planning/BRIEF.md`
- Requirements: `plugins/O-Bitrot/.planning/REQUIREMENTS.md`
- Parameter spec (draft): `plugins/O-Bitrot/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Bitrot/.planning/research/ARCHITECTURE.md`
- Research base: `research/glitch-effects/degradation-dsp-deep-dive.md`,
  `research/glitch-effects/multi-effect-sequencer-reuse-audit.md`

**Reference plugins:**
- O-Polystutter — capture ring, varispeed read loop, anti-click crossfade stack, updateBeatSync
- O-ReverseDelay — absolute-index CaptureBuffer ring
- O-TapeStop concept / TapeDegrader — ramped-rate tape behavior precedents
