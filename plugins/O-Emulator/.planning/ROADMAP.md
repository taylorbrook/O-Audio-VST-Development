# O-Emulator - Implementation Plan

**Date:** 2026-08-20
complexity_score: 5.0
**Complexity Score:** 5.0 (Complex — capped)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 5 parameters (5/5 = 1.0, cap 2.0) = **1.0**
- **Algorithms:** 13 DSP components = **13**
  - Console Pipeline Manager (engine abstraction + config tables)
  - BRR codec (SNES), SPU-ADPCM codec (PS1), DPCM codec (NES), 4-bit wave quantizer (GB), 8-bit DAC + ladder effect (Genesis)
  - Console-domain resampler (AA decimation + Gaussian/ZOH upsampling)
  - SPU reverb (register-model port, 22.05 kHz domain)
  - Output-stage model, Age model, Crush macro mapping, DryWetMixer, Console-switch crossfader
- **Features:** 2 points
  - Feedback loops (+1) — SPU reverb wall/comb feedback network
  - Modulation systems (+1) — Age drift random-walk LFO on resample ratio, hum oscillators
- **Total:** 16.0 → **capped at 5.0**

---

## Stages

- Stage 0: Research & Planning ✓ (this document)
- Stage 1: Foundation ← Next (build system, APVTS, shell)
- Stage 2: DSP (4 phases)
- Stage 3: GUI (2 phases)
- Stage 4: Validation (presets, pluginval, changelog)

---

## Complex Implementation (Score 5.0)

### Stage 2: DSP Phases

#### Phase 2.1: Engine Skeleton + SNES End-to-End

**Goal:** Prove the whole architecture on one console: fixed-chunk FIFO infrastructure, console-domain resampling, BRR round-trip, Gaussian upsampling, output stage, latency reporting, dry/wet mix.

**Components:**
- Input/output FIFO + fixed-chunk feeder (block-size invariance by construction)
- AA lowpass (IIR ArrayCoefficients) + Lagrange downsampler + phase-continuous ratio handling
- BRR closed-loop encoder/decoder (16-sample blocks, filters 0–3, shift 0–12)
- 4-tap Gaussian interpolator (512-entry S-DSP table)
- SNES output stage (LP + soft clip), DC blocker
- `DryWetMixer` with `setWetLatency` == `setLatencySamples` figure
- `crush` wired to drive + shift-floor; `mix` wired

**Test Criteria:**
- [ ] Plugin loads (VST3 + AU), audio passes, no crashes
- [ ] Render harness: digest identical at block sizes 64/512/4096 (PERF-02)
- [ ] Mix 0% is bit-transparent minus reported latency (FUNC-02)
- [ ] White-noise render shows 32 kHz alias/Gaussian-rolloff signature (DSP-02)
- [ ] Crush min vs. max measurably different output (liveness-gated probe)
- [ ] Pathological input probe (silence, DC, full-scale noise, denormals) → bounded finite output (QUAL-01)

---

#### Phase 2.2: PS1 + SPU Reverb

**Goal:** Second codec (validates engine abstraction) + the highest-risk component (SPU reverb) in all modes.

**Components:**
- SPU-ADPCM encoder/decoder (28-sample blocks, 5 filter pairs) via shared encoder skeleton
- SPU reverb at fixed 22.05 kHz (Hall preset registers, comb + wall feedback + 2×APF, stereo-cross)
- Reverb send tap (post-codec) + return summing; `reverb` parameter wired
- isfinite guard with non-sticky reset on the reverb network

**Test Criteria:**
- [ ] PS1 mode audibly/measurably distinct from SNES on same source (FUNC-01 partial)
- [ ] Impulse-response tail matches SPU reference character: short, murky, comb/APF structure (DSP-04)
- [ ] Reverb send works in both implemented modes (FUNC-03 partial)
- [ ] Feedback network stable at reverb 100% + crush 100% for 60 s noise (no growth, no NaN)
- [ ] Block-size invariance still holds with reverb engaged

---

#### Phase 2.3: NES / Game Boy / Genesis + Console Switching

**Goal:** Three simple quantizer consoles + click-safe mode switching.

**Components:**
- NES DPCM state machine (7-bit counter, ±2 steps, 16-entry timer-rate table walked by crush)
- GB 4-bit mid-tread quantizer with per-level DAC error + ZOH
- Genesis 8-bit quantizer + Nuked-OPN2 ladder-effect offset + crush rate-hold
- Per-console output-stage configs (corners, clip styles)
- 30 ms equal-power console crossfader; constant worst-case latency across modes

**Test Criteria:**
- [ ] All 5 modes audibly distinct, mode-appropriate on same material (FUNC-01)
- [ ] Per-console output filter corners measurably differ (DSP-03)
- [ ] Console switch mid-playback: no clicks, pops, stuck audio, or latency renegotiation (FUNC-04)
- [ ] NES DC offset fully blocked at output (dry/wet sum clean)
- [ ] Reverb works in all 5 modes (FUNC-03 complete)

---

#### Phase 2.4: Age Model + Crush Polish

**Goal:** Hardware-condition model and final macro-curve tuning.

**Components:**
- Noise floor (host-rate domain, white→LP, level-normalized) + 60 Hz hum family
- Filter dulling (output LP corner × age) + drift random-walk on resample ratio (per-sample integrated)
- Per-console crush curve tuning (drive, shift floor, rate walks, AA-open breakpoint) with micro-fades on integer steps
- Parameter smoothing audit (no zipper on any knob sweep — liveness-gated sweep probes)

**Test Criteria:**
- [ ] Age min vs. max measurably different in all 5 modes; scales continuously (DSP-05)
- [ ] Noise-bed level invariant across host sample rates 44.1/48/96/192 kHz
- [ ] Drift produces bounded ±15 cent wobble, no resampler instability
- [ ] Offline bounce digest == real-time digest (per-sample mod integration)
- [ ] All four macro knobs pass min≠max liveness probe in all 5 modes (FUNC-02)

---

### Stage 3: GUI Phases

#### Phase 3.1: Layout and Basic Controls

**Goal:** Integrate finalized mockup; console selector focal element + 4 macro knobs.

**Components:**
- Mockup HTML → `Source/ui/public/index.html`; WebView setup per juce8-critical-patterns (#8, #9, #11, #13, #21)
- Console selector (choice relay / `getComboBoxState`), 4 knobs via `WebSliderRelay` (3-arg attachments, #12)
- Relative-drag knobs (#16), `getScaledValue` readouts (house pattern)

**Test Criteria:**
- [ ] WebView opens at correct size; layout matches mockup
- [ ] All controls render and respond; ES6 module loading correct
- [ ] Console selector switches modes from UI

#### Phase 3.2: Binding Completeness + Polish

**Goal:** Two-way binding, automation, preset behavior.

**Components:**
- Host automation → UI updates; preset changes update all elements (customLoad revision-counter pattern if one-shot pushes go stale)
- Value formatting; any mode-dependent UI accents from mockup

**Test Criteria:**
- [ ] Knob moves change DSP; automation updates UI; preset load updates everything
- [ ] No frozen knobs, no undefined callback values
- [ ] grep-diff native-function bridge audit clean (house pattern)

---

### Implementation Flow

- Stage 1: Foundation — CMake, APVTS (5 params incl. 5-entry choice), stereo effect shell, pluginval smoke (COMPAT-01)
- Stage 2: DSP — Phases 2.1 → 2.2 → 2.3 → 2.4 (git commit per phase; render-harness gate per phase)
- Stage 3: GUI — Phases 3.1 → 3.2
- Stage 4: Validation — factory presets (per-console signatures, UI-02), pluginval strictness 10 VST3+AU, CHANGELOG, docs

---

## Implementation Notes

### Thread Safety
- Atomic parameter reads per block; all engines/buffers pre-allocated in `prepareToPlay`; zero allocation/locks/file-I/O in `processBlock` (PERF-01 audit item).
- No background thread; no custom messaging.

### Performance
- Total estimated < 10% single core @ 48 kHz (codec search ~0.2%, reverb ~2%, resampling/filters ~5%). Not a bottleneck — correctness and invariance are the gates.

### Latency
- Nominal total ≈ 100–130 samples @ 48 kHz (~2.3 ms): AA group delay + 32-sample FIFO chunk + worst codec block (28 @ 22.05 kHz) + interpolator history.
- Computed exactly in `prepareToPlay`; `setLatencySamples(N)` (non-virtual in JUCE 8) mirrored into `DryWetMixer::setWetLatency(N)`. Constant across console modes (worst case).

### Denormal Protection
- `juce::ScopedNoDenormals`; integer codec domains inherently safe; reverb/one-pole guards with non-sticky isfinite reset (keep coefficients, reset state only).

### Known Challenges
- SPU reverb register-model port is the highest-risk item — fallback: tuned Schroeder network at 22.05 kHz (documented in ARCHITECTURE.md Implementation Risks)
- Closed-loop ADPCM encoders must use decoded history, not clean history — fallback: open-loop encoders
- Block-size invariance depends on never deriving state from host block boundaries — fixed-chunk FIFO pattern from house render-harness plugins
- GPL provenance: implement codecs/tables from published specs, never port blargg/Nuked GPL code

---

## References

- Creative brief: `plugins/O-Emulator/.planning/BRIEF.md`
- Requirements: `plugins/O-Emulator/.planning/REQUIREMENTS.md`
- Parameter spec (draft): `plugins/O-Emulator/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Emulator/.planning/research/ARCHITECTURE.md`
- UI mockup: pending (`/dream O-Emulator` mockup phase before Stage 1 finalizes parameter-spec.md)

Reference plugins in this repo:
- O-Tapestop / O-Bitrot — fixed-chunk invariance + render-harness gating patterns
- O-Contrabass — latency reporting + DryWetMixer wet-latency pairing
- Any recent WebView plugin (O-Prism) — WebView binding checklist
