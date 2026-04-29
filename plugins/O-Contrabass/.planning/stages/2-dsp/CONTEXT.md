# Stage 2: DSP — Context (rev-7)

**Date:** 2026-04-28
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.4b — Sub-Harmonic Bias DSP-07 (ARCHITECTURE §457 period-doubling friction-junction parameter biasing)**
**Supersedes:** rev-6 (Phase 2.4a — Schelleng Wedge Bass-Register Calibration Polynomial + 108-Combo Stability Matrix + `pass_breathingAudible` Threshold Restoration, dated 2026-04-27). rev-6 contracts that remain locked are inherited verbatim and not re-litigated. Phase 2.4a closed 2026-04-28 with R34 atomic commit (`4c926bb`, Gate 6a CLEARED — 3 strict-PASS + 2 soft-PASS within v1.0 budgets) + R34-backfill chore (`b64c8c4`).

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle opens **Phase 2.4b** — second of three Phase 2.4 sub-cycles (rev-6 Q12 split decision). Phase 2.4b scope = **sub-harmonic bias DSP-07 only**: ARCHITECTURE §457 friction-junction parameter biasing toward Schelleng `F_max` regime, producing audible period-doubling f0/2 content as a musical bass-extension feature. Bias is applied as voice-level pre-friction parameter shift on `F_bow`, `v_0`, `mu_s` — clamped via reuse of the just-landed Phase 2.4a `SchellengCalibration` trilinear table. 30 ms `SmoothedValue` on `SUB_HARMONICS` parameter. Active-string-only (HR-1 vibrato precedent). Chaos detector (lag-2 RMS auto-back-off) + softClampState energy clamp deferred to Phase 2.5/2.6. Phase 2.4-bis backlog (breathingAudible metric refinement + 3 fallback-cell reduction) keeps as separate future cycle.

**Phase 2.4c** (autocorrelator octave-rejection harness fix + saturator-tail O-Bowed comparison harness) gets fresh CONTEXT rev-8 when 2.4c discuss-phase opens after 2.4b verifies.

After Phase 2.4b verifies (Gate 6b PASS), Phase 2.4c discuss-phase opens.

---

## Cycle Scope

**Goal:** Implement DSP-07 sub-harmonic bias per ARCHITECTURE §457 — at `SUB_HARMONICS = 1.0` on E1 (MIDI 28), produce E(f0/2) / E(f0) ≥ 0.10 spectral energy ratio measured over the last 2 s of a 5 s sustain at default bow params, while preserving QUAL-01 stability across 36 stress-test combos (4 strings × 3 INFINITE_SUSTAIN × 3 SUB_HARMONICS at default BODY_DAMPING). Bias applied voice-level pre-friction in `BowedContrabassVoice.cpp` HR-9-gated path; F_max ceiling sourced from Phase 2.4a `schelleng::safeDepthForString(...)` table (semantic mapping research-phase to lock — recommended: stable cells (1.0) allow full `kForceBoost=1.8`, fallback cells (0.5) clamp `kForceBoost` to 1.0 effective no-op). All 10 currently-committed goldens MUST reproduce byte-identically because at `SUB_HARMONICS=0` (default) HR-9 IEEE 754 identity arithmetic fires `subAmount=0 → F_bow*=1.0, v_0*=1.0, gap-multiplier=1.0` short-circuit + active-string-only gate.

**In scope:**

- **`Source/BowedContrabassVoice.{h,cpp}`** — add voice-level sub-harmonic bias evaluation in the per-block 7-step order (Phase 2.3 7-step + Phase 2.4a wedge math swap), inserted as Step 2.5 between Step 2 (Schelleng wedge / SchellengCalibration trilinear lookup) and Step 3 (slow-LFO modulation apply). Per-block computes `subAmount = subHarmonicsSmoothed.getNextValue()`; HR-9 short-circuits the entire bias path at `subAmount == 0.0f`. New `juce::SmoothedValue<float, Linear> subHarmonicsSmoothed` member with 30 ms ramp time (architecture §457). New `std::atomic<float> lastSubAmount { 0.0f }` instrumentation hook (mirrors Phase 2.3 `lastSafeDepth` pin #4). Active-string-only bias evaluation gates on `activeStringIndex` (HR-9 mirroring HR-1).
- **`Source/DSP/SubHarmonicBias.h`** (new, ~80 LOC) — header-only `inline constexpr` bias function `applyBias(float subAmount, int stringIdx, float v_b, float beta, float& F_bow, float& v_0, float& mu_s, float mu_d) noexcept`. Implements ARCHITECTURE §457 verbatim with `kForceBoost = 1.8`, `kV0Reduction = 0.5`, `kGapWiden = 0.25`, `kFmaxScalar = 0.95`. F_max ceiling sourced via `schelleng::safeDepthForString(stringIdx, v_b, F_bow, beta)` lookup mapped to a `kForceBoost` scaling factor (research-phase locks the stable→1.0 / fallback→1.0 effective-no-op mapping). Mirrors Phase 2.4a `SchellengCalibration.h` per-plugin precedent — NOT extracted to shared module per ARCHITECTURE §765 ("O-Contrabass-specific, should NOT bleed back into O-Bowed defaults").
- **`Source/PluginProcessor.cpp`** — add APVTS attachment for `SUB_HARMONICS` parameter ID (already declared at line 104 with default 0.0 ∈ [0, 1.0]) into voice's `subHarmonicsSmoothed.setTargetValue(...)` per-block. NO Stage-1 contract amendment (parameter-spec.md unchanged at `77638e25…`).
- **`tests/render-harness/main.cpp`** — add CLI flag `--sub-harmonics` activating audible f0/2 mode: render MIDI 28 (E1 open) at `SUB_HARMONICS = 1.0`, default bow params (BOW_SPEED=0.15, BOW_PRESSURE=3.0, BOW_POSITION=0.10), sustain 5 s. Capture last 2 s and run a `juce::dsp::FFT` (size 65536, Hann window) computing `E(f0) = sum of magnitude² over bins around 41.2 Hz ± 0.5 Hz` and `E(f0/2) = same around 20.6 Hz ± 0.5 Hz`. Emit JSON with `{mode: "sub-harmonics", peak, rmsContinuity, blockTimeRatio, subharmEnergyRatio, pass_noNaN, pass_peak, pass_clickFree, pass_blockTime, pass_subharmAudible}`. `pass_subharmAudible = (subharmEnergyRatio >= 0.10)`. Schema mirrors existing `--slow-lfo` per-mode pattern.
- **`tests/render-harness/main.cpp`** — add CLI flag `--sub-harmonics-stability` activating 36-combo render mode: 4 strings × 3 INFINITE_SUSTAIN ∈ {0.0, 0.5, 1.0} × 3 SUB_HARMONICS ∈ {0.0, 0.5, 1.0} at default BODY_DAMPING + default bow params, sustain 5 s per combo at MIDI {28, 33, 38, 43} per stringIdx. Per-combo `{stringIdx, infiniteSustain, subHarmonics, peak, rmsContinuity, blockTimeRatio, pass_noNaN, pass_peak, pass_clickFree, pass_blockTime}` + aggregate `pass_all_36` + `failCount`. Single concatenated WAV with 0.5 s silence buffers (mirrors Phase 2.4a `--matrix-stability` precedent). Wall-clock budget ~3-4 min.
- **`tests/render-harness/golden/`** — add NEW golden text files `sub-harmonics.{wav.sha256,json,json.sha256}` and `sub-harmonics-stability.{wav.sha256,json,json.sha256}`. NO re-baseline of existing 10 goldens (HR-9 IEEE 754 identity arithmetic preserves bit-exact regression at `SUB_HARMONICS=0` default).
- **`tests/render-harness/reproduce-goldens.sh`** — extend to include 12 goldens (10 carry-forward + 2 new). Continues Phase 2.4a R34-pre tripwire infrastructure.

**Carry-forward goldens (Gate 6b regression bar — invariant 1):**
- E1 strict modulators-off `d358abcd…` — bit-exact (HR-9 short-circuit at SUB_HARMONICS=0).
- Phase 2.2 detune-sweep-A `5e31dad3…` — bit-exact (HR-9).
- Phase 2.3 modulators-off renders (string-A `c6755aa4…`, string-D `765b015e…`, string-G `0cd5cb0a…`, note-sequence `3ac3ccd0…`) — bit-exact (HR-9).
- Phase 2.3 `vibrato.wav.sha256` `d7881ecf…` — bit-exact (HR-9).
- Phase 2.3 `macro-sweep.wav.sha256` — bit-exact (HR-9).
- Phase 2.4a re-baselined `slow-lfo.wav.sha256` (post-calibration) — bit-exact (HR-9).
- Phase 2.4a re-baselined `schelleng-stress.wav.sha256` (post-calibration) — bit-exact (HR-9).
- Phase 2.4a `matrix-stability.wav.sha256` `6db67707…` — bit-exact (HR-9; matrix-stability harness mode renders SUB_HARMONICS=0 across all 108 combos).

**Out of scope (deferred to Phase 2.4c, 2.5, 2.6, end-of-Stage-2, Phase 2.4-bis):**
- Chaos detector (lag-2 RMS > lag-1 RMS auto-back-off) — ARCHITECTURE §457 marks as "optional"; deferred to Phase 2.5/2.6 (relies on Schelleng F_max clamp + algebraic saturator + loop-gain ceiling 0.9999999 as v1.0 layered defenses).
- Energy clamp `softClampState` (threshold 0.85, ceiling 1.0) — ROADMAP §Phase 2.4 deliverable; deferred to Phase 2.5/2.6 (current algebraic saturator `x/sqrt(1+x²)` covers the role at v1.0).
- Autocorrelator octave-rejection vibrato harness fix (Phase 2.4c).
- Saturator-tail O-Bowed comparison harness + saturator-choice decision (Phase 2.4c).
- Body resonator + bow noise (Phase 2.5).
- Master saturator/limiter, stereo width, microtonal, MPE (Phase 2.6).
- Phase 2.4-bis backlog (breathingAudible metric refinement OR Step 4 modulation gain tune-up to reach 20% peak-to-peak; reduce 3 v1.0 fallback cells via downstream-defense tightening).
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments (end-of-Stage-2 verify).
- E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7).

---

## Requirements Confirmed (Phase 2.4b-relevant subsets of locked contracts)

- **DSP-07** (Sub-Harmonic generator extends bass below string fundamental musically): Phase 2.4b implements ARCHITECTURE §457 bias formula. Acceptance bar = audible f0/2 content at `SUB_HARMONICS=1.0` on E1 with `subharmEnergyRatio = E(f0/2) / E(f0) >= 0.10` measured via FFT over last 2 s of 5 s sustain. ROADMAP §Phase 2.4 "audible f0/2 perceived as 'weight' at 50%" interpreted: 10% energy ratio at 100% gives perceptual headroom; 50% setting need not satisfy a separate threshold (stress tested in 36-combo for stability only). Promotion to "complete" in REQUIREMENTS.md held until end-of-Stage-2 verify.
- **QUAL-01** (no audible clicks during parameter sweeps, no artifacts at normal ranges including drone settings): 36-combo `--sub-harmonics-stability` matrix is the primary gate. All 36 combos must satisfy `pass_noNaN + pass_peak ≤ 1.0 + pass_clickFree (rmsContinuity ≥ 0.85) + pass_blockTime (ratio ≤ 5.0)`. Aggregate `pass_all_36 = true` OR `failCount ≤ 2` v1.0 fallback budget (mirrors 2.4a 105/108 + failCount≤4 precedent at smaller scale). NO BODY_DAMPING axis — body resonator is Phase 2.5; full 108-combo with BODY_DAMPING revisits at end-of-Stage-2 once Phase 2.5 lands.
- **QUAL-02** (extreme drone settings remain musical): `--sub-harmonics-stability` includes extreme combos (SUB_HARMONICS=1.0 + INFINITE_SUSTAIN=1.0). F_max clamp via SchellengCalibration table prevents chaotic regime; layered defenses (algebraic saturator + loop-gain ceiling) catch instability.
- **DSP-06** (Infinite Sustain control): `--sub-harmonics-stability` includes INFINITE_SUSTAIN=1.0 across 12 of 36 combos.
- **DSP-08** (Slow Bow LFO Schelleng-aware): unchanged — sub-harmonic bias is independent of slow-LFO; both can be active concurrently. Phase 2.4b verify renders all goldens at SLOW_LFO_DEPTH=0 default (HR-2 carry-forward).
- **PERF-01** (no allocations in `processBlock`): SubHarmonicBias.h is `inline constexpr`; bias evaluation is ~5 multiplies + 1 add + 1 SchellengCalibration table lookup per block. Zero allocations.
- **PERF-02** (< 5% CPU on M1): bias evaluation <0.1% CPU added on top of Phase 2.4a baseline.

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**

- All 29 APVTS parameter IDs, ranges, skews, defaults — `parameter-spec.md` (sha256:`77638e25…`). `SUB_HARMONICS` already declared at PluginProcessor.cpp:104 with default 0.0 ∈ [0, 1.0]. **NO Stage-1 contract amendment in Phase 2.4b.**
- DSP architecture (`research/ARCHITECTURE.md`, sha256:`3cb26814…`) — §457 sub-harmonic bias formula consumed verbatim; F3 deviation (no in-loop DCB) + saturator-tail tracking carry forward; ARCHITECTURE amendment still deferred to end-of-Stage-2 verify.
- ROADMAP phasing (sha256:`106639f6…`).
- `modules/synthesis/bow-friction/` v1.0.0 (Phase 2.1b) — value-class deterministic; **Phase 2.4b does NOT touch friction module surface** (per Q24: per-plugin in BowedContrabassVoice + new SubHarmonicBias.h).
- `Source/DSP/DispersionFilter.h` (Phase 2.1c, R20 commit `5759e5e`) — verbatim consume.
- `Source/DSP/WaveguideString.{h,cpp}` (Phase 2.2, R26 commit `131c2c7`) — verbatim consume.
- `Source/DSP/SchellengCalibration.h` (Phase 2.4a, R34 commit `4c926bb`) — verbatim consume; `safeDepthForString(...)` API surface unchanged.
- Phase 2.3 modulator surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order / HR-1..HR-4) — Phase 2.4b inserts Step 2.5 (sub-harmonic bias evaluation) between Step 2 and Step 3 without disturbing existing 7-step semantics. Existing HR-1..HR-4 + Phase 2.4a HR-5..HR-8 verbatim.

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**

- `juce::ScopedNoDenormals` at `processBlock` entry — already in place, unchanged.
- `juce::SmoothedValue<float, Linear>` chain on macro destinations + per-string detune — unchanged. NEW: `subHarmonicsSmoothed` follows same pattern (30 ms ramp time, `setCurrentAndTargetValue(0.0f)` in `prepareToPlay`, `setTargetValue` UNCONDITIONAL each block per Phase 2.3 pin #11 precedent).
- `juce::dsp::DelayLine<float, Lagrange3rd>` per-sample `setDelaySamples()` — unchanged.
- `juce::dsp::FFT` size 65536 Hann-windowed for `subharmEnergyRatio` measurement in `--sub-harmonics` harness mode (research-phase locks window choice + bin-selection edge cases).

**Phase 2.4b-specific constraints:**

- **HR-9 hard rule (NEW): SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate.** At `SUB_HARMONICS=0` (default in all 10 carry-forward goldens), HR-9 short-circuits the entire bias path BEFORE any arithmetic. Implementation: `if (subAmount == 0.0f) { lastSubAmount.store(0.0f); return; }` at Step 2.5 entry. Bias is invoked ONLY for the active string (mirrors HR-1 vibrato) — never for crossfade-shadowed strings. Combined with `subHarmonicsSmoothed.setCurrentAndTargetValue(0.0f)` in `prepareToPlay` AND default APVTS value of 0.0, HR-9 guarantees all 10 carry-forward goldens reproduce byte-identically.
- **Bias terms are IEEE 754 identity-arithmetic at subAmount=0 BEFORE HR-9 short-circuit reaches them** (defensive belt-and-suspenders): `F_bow *= (1.0f + 0.8f * 0.0f) = F_bow * 1.0f` exact; `v_0 *= (1.0f - 0.5f * 0.0f) = v_0 * 1.0f` exact; `gap *= (1.0f + 0.25f * 0.0f) = gap * 1.0f` exact. Identity arithmetic redundant with HR-9 short-circuit but provides defense if a future refactor removes the short-circuit gate.
- **F_max ceiling via SchellengCalibration reuse.** Bias's `F_bow` clamp invokes `schelleng::safeDepthForString(activeStringIndex, v_b, F_bow_pre_bias, beta)` — output ∈ {0.5, 1.0}. Mapping to `kForceBoost` scaling: `effectiveBoost = subAmount * 0.8f * safeDepth` (stable cells 1.0 → full bias; fallback cells 0.5 → half-bias). Research-phase locks the precise mapping function and validates against §17 fitting data variance.
- **Active-string-only bias.** Crossfade window: bias for `crossfadePrevStringIndex` is gated off (not biased), only `activeStringIndex` is biased. 5 ms equal-power crossfade ramp from Phase 2.2 carries forward; bias contributes to active string only during the entire crossfade.
- **Strict bit-exact regression bar unchanged.** All 10 carry-forward goldens (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep + slow-lfo + schelleng-stress) MUST reproduce byte-identically. HR-9 is the technical defence.
- **NO Stage-1 contract amendment.** parameter-spec.md sha256 `77638e25…` carries forward unchanged. STATUS.md `contract_checksums.parameter_spec` unchanged.
- **NO ARCHITECTURE.md amendment.** Bias formula is implementation of §457 verbatim; deferred chaos detector + softClampState tracked in commit-message body until end-of-Stage-2 verify.

**Working-tree starting state (locked from Phase 2.4a verify, R34 commit `4c926bb`):**

- `Source/BowedContrabassVoice.{h,cpp}` — 4-string voice with Phase 2.3 7-step + Phase 2.4a Step 2 SchellengCalibration trilinear lookup behind HR-4 gate + HR-7 matrix-stability bypass.
- `Source/DSP/SchellengCalibration.h` (auto-generated from matrix.json sha256 `625505cf…`, 105/108 stable + 3 fallback cells).
- `Source/DSP/WaveguideString.{h,cpp}` (Phase 2.2 R26).
- `Source/DSP/DispersionFilter.h` (Phase 2.1c R20).
- `Source/PluginProcessor.{h,cpp}` — 29 APVTS params (Stage 1 + Phase 2.2 + Phase 2.3 + Phase 2.4a no-op).
- `modules/synthesis/bow-friction/` v1.0.0 (Phase 2.1b) — verbatim consume.
- 10 currently-committed goldens + reproduce-goldens.sh (Phase 2.4a infrastructure).

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q23 — Phase 2.4b cycle scope** | **Sub-harmonic bias DSP-07 only** | Tight cycle mirrors Phase 2.4a precedent. Phase 2.4-bis backlog (breathingAudible metric refinement, 3 fallback-cell reduction) keeps as separate Phase 2.4-bis cycle. Chaos detector + softClampState defer to Phase 2.5/2.6. User-confirmed. |
| **Q24 — Bias application site** | **Per-plugin in `BowedContrabassVoice.cpp` pre-friction + new `Source/DSP/SubHarmonicBias.h` header** | Mirrors Phase 2.4a per-plugin SchellengCalibration.h precedent. Friction module v1.0.0 untouched (zero ABI churn, no module version bump). Architecture §765 says "O-Contrabass-specific, should NOT bleed back into O-Bowed defaults." Header gives clean separation analogous to DispersionFilter.h. User-confirmed. |
| **Q25 — F_max clamp data source** | **Reuse Phase 2.4a `schelleng::safeDepthForString()` table** | Natural reuse of just-landed work. 108-combo bass-register data already calibrated against `pass_noNaN + pass_peak + pass_clickFree + pass_blockTime`. Stable cells (1.0) allow full bias; fallback cells (0.5) clamp effective boost to half. One less calibration polynomial to fit. User-confirmed. |
| **Q26 — Stress test matrix scope** | **36-combo `--sub-harmonics-stability` (4 strings × 3 INFINITE_SUSTAIN × 3 SUB_HARMONICS); chaos detector deferred** | BODY_DAMPING axis blocked by Phase 2.5 dependency; v1.0 ships 36-combo evidence + Phase 2.5 at end-of-Stage-2 picks up full 108. Chaos detector parks to Phase 2.5/2.6 (relies on Schelleng F_max clamp + algebraic saturator + loop-gain ceiling). ~3-4 min wall-clock budget. User-confirmed. |
| **Q27 — `pass_subharmAudible` measurement** | **FFT energy ratio E(f0/2)/E(f0) ≥ 0.10 at SUB_HARMONICS=1.0 on E1** | Numeric, deterministic, falsifiable. FFT size 65536 Hann-windowed over last 2 s of MIDI 28 sustain at default bow. Architecture §457 "audible f0/2 perceived as weight" interpreted as ≥10% spectral energy at f0/2 (~20.6 Hz) relative to f0 (~41.2 Hz) at full SUB_HARMONICS. Adds ~30 LOC FFT analyzer to harness via `juce::dsp::FFT`. User-confirmed. |
| **Q28 — Logic AU smoke timing** | **Deferred non-blocking R37 precedent** | Mirrors R32 / R27 / R19f / R14e. Automated Gate 6b invariants gate verify; Logic AU smoke is user-discretion non-blocking (MIDI 28 + SUB_HARMONICS 0→1.0 ramp at 30 s; MIDI 33 + SUB+SUS=1.0 chaos check). User-confirmed. |
| **Q29 — Golden-file scope** | **Carry-forward 10 + add 2 new** (`--sub-harmonics` + `--sub-harmonics-stability`) | 10 currently-committed goldens MUST reproduce byte-identically (HR-9 IEEE 754 identity arithmetic). 2 new goldens land in R35 atomic commit alongside source delta. Plus matrix-stability `6db67707…` carries forward. User-confirmed. |
| **Q30 — 30 ms `SmoothedValue` on SUB_HARMONICS** | **Confirmed per architecture §457 implementation note** | New `subHarmonicsSmoothed` member with 30 ms ramp time. Initialised via `setCurrentAndTargetValue(0.0f)` in `prepareToPlay` (HR-9 strict-default precondition). `setTargetValue` UNCONDITIONAL each block per Phase 2.3 pin #11 precedent. |
| **Q31 — Active-string-only bias** | **Confirmed (HR-1 vibrato precedent)** | Bias evaluated only for `activeStringIndex`; crossfade-shadowed strings unbiased. 5 ms equal-power crossfade window from Phase 2.2 carries forward. Architecture §557 "bias INSIDE the junction" — junction is per-string, bias is per-string-active. |
| **Q32 — Atomic commit** | **R35 atomic commit on Gate 6b PASS** | Continues sequence: R7 → R15 → R20 → R26 → R33 → R34 → R35. Lands ~12-15 files (2 source: BowedContrabassVoice.{h,cpp} + 1 new header SubHarmonicBias.h + harness CLI flag + 2 new golden text + reproduce-goldens.sh extension + planning artefacts + STATUS.md). R35-backfill chore commit per R34 / R33 precedent. |
| **Q33 — CONTEXT.md doc scope** | **rev-7 covers Phase 2.4b only** | Phase 2.4c (autocorrelator + saturator-tail) gets rev-8 when discuss-phase opens after 2.4b verify. Mirrors rev-2/rev-3/rev-4/rev-5/rev-6 (Phase 2.1a/b/c, 2.3, 2.4a) precedent. |
| Per-block evaluation order | **7-step + Step 2.5 inserted (Phase 2.4b)** | Step 2.5 (sub-harmonic bias evaluation) sits between Step 2 (Schelleng wedge / SchellengCalibration trilinear) and Step 3 (slow-LFO modulation apply). HR-9 short-circuits entire Step 2.5 at `subAmount == 0.0f`. Step 2.5 reads pre-bias `F_bow / v_0 / mu_s / mu_d` from Step 1 raw APVTS or smoothed values; writes biased values that feed downstream macro layer + per-sample friction call. |
| Bias application order | **Bias → Schelleng F_max clamp → friction lookup (architecture §557 verbatim)** | F_max clamp is INSIDE the bias function (post bias-multiply, pre return). Order matches §457 implementation note + §557 "bias INSIDE the junction" wording. SchellengCalibration table read happens once per block per active string. |

---

## Open Questions (handed to research-phase)

1. **SchellengCalibration→F_max ceiling mapping function.** 2.4a table returns `safeDepth ∈ {0.5, 1.0}` as a slow-LFO depth multiplier; 2.4b needs an `F_bow` ceiling. Recommended mapping: `effectiveBoost = subAmount * 0.8f * safeDepth` (stable→full 0.8, fallback→half 0.4). Alternatives: (a) use `safeDepth` as kForceBoost scalar (`kForceBoost = 1.8f * safeDepth`), (b) treat fallback cells as hard zero (`if (safeDepth < 1.0f) bias=0`), (c) introduce a new fitting pass for `F_bow ceiling = f(stringIdx, v_b, F_bow, beta)` mirroring 2.4a polynomial. Research-phase locks against acceptance criterion (Open Q3 below).

2. **FFT analyzer specifics.** FFT size + window + bin selection for `subharmEnergyRatio` measurement. Defaults: size 65536 Hann, bin width 0.673 Hz at sr=44100. f0 (41.2 Hz) bin 61.2; f0/2 (20.6 Hz) bin 30.6. Energy = magnitude² summed over 3 bins centered on each (capture spectral leakage). Open: window choice (Hann vs Blackman-Harris), bin-width tradeoff vs sustain length, log-magnitude vs linear-magnitude energy. Research-phase pre-flights at SUB_HARMONICS=0 baseline (subharmEnergyRatio expected ≈ 0 — system noise floor only) and at SUB_HARMONICS=1.0 against bias formula prediction.

3. **`pass_subharmAudible` threshold tuning.** Default 0.10 ratio at SUB_HARMONICS=1.0 on E1. Research-phase pre-flights: render at SUB_HARMONICS=1.0 with bias formula coefficients verbatim (`kForceBoost=1.8`, `kV0Reduction=0.5`, `kGapWiden=0.25`, `kFmaxScalar=0.95`); measure resulting subharmEnergyRatio; if measured value < 0.10, escalate (relax threshold to architecture-spec'd "audible" wording OR re-tune coefficients OR escalate to Phase 2.4-bis style soft-pass with documented v1.0 budget).

4. **HR-9 bit-exact pre-flight.** Render all 10 currently-committed goldens BEFORE Phase 2.4b source edits via canonical default-duration invocations (reproduce-goldens.sh from Phase 2.4a R34-pre tripwire). Capture sha256s; verify against committed values. If any drift, escalate before plan-phase. Mirrors §17.1 / §16.1 precedent.

5. **`--sub-harmonics-stability` single-combo wall-clock pre-flight.** Render ONE combo at extreme settings (E1, SUB_HARMONICS=1.0, INFINITE_SUSTAIN=1.0) — measure block-time ratio + wall-clock per combo. Extrapolate to 36-combo total wall-clock; revisit matrix size if budget overrun (mirrors §17.2). Initial estimate: 36 × 5 s sustain ≈ 180 s wall-clock at typical block-time ratio ≤ 1.0.

6. **`--sub-harmonics-stability` MIDI note per combo.** Each stringIdx renders at one note. Recommend: open-string MIDI 28/33/38/43 (4 strings), mirroring Phase 2.4a `--matrix-stability` precedent. Research-phase confirms (alternative: middle-of-range per string).

7. **`--sub-harmonics-stability` pass criteria.** Default thresholds (rmsContinuity ≥ 0.85, blockTimeRatio ≤ 5.0) carry forward from Phase 2.4a. Research-phase pre-flights extreme combo to confirm thresholds are tight enough at SUB_HARMONICS=1.0 + INFINITE_SUSTAIN=1.0.

8. **SubHarmonicBias.h API shape.** Recommended single function `applyBias(float subAmount, int stringIdx, float v_b, float beta, float& F_bow, float& v_0, float& mu_s, float mu_d)`. Open: should `mu_d` be const reference (read-only) or non-const (also biased)? Architecture §457 only mutates `mu_s`; gap widening is via `mu_s = mu_d + gap * (1.0f + 0.25 * subAmount)` keeping `mu_d` constant. Research-phase confirms.

9. **Bias evaluation context — voice-level vs per-string.** Active-string-only (Q31) means bias evaluated once per block for `activeStringIndex`. Open: does bias's input `v_b` (bow velocity) and `F_bow` (bow force) come from voice-level smoothed values OR per-string-instance? At v1.0 with single bow-position pipeline, voice-level inputs feed all strings — bias inputs are voice-level. Research-phase locks input wiring.

10. **R35 task breakdown.** Initial estimate mirrors R34: R35-pre tripwire + R35a (harness `--sub-harmonics` + `--sub-harmonics-stability` flags + FFT analyzer) + R35b (render new goldens + capture sha256s) + R35c (author SubHarmonicBias.h) + R35d (BowedContrabassVoice integration: Step 2.5 + subHarmonicsSmoothed + lastSubAmount instrumentation) + R35e (regression bar via reproduce-goldens.sh) + R35f (auval + pluginval-10) + R35 atomic commit + R35-backfill chore. Research-phase locks task body and ordering.

11. **Per-string SUB_HARMONICS variation.** Architecture §457 specifies a single global `subAmount`. Open: does ROADMAP / BRIEF imply per-string sub-harmonic variation (e.g., E1-only sub bias, others muted)? Research-phase audits BRIEF + ROADMAP. Recommended: single global SUB_HARMONICS applied to active string only via Q31.

12. **Architectural footnote on chaos detector deferral.** Track in R35 commit-message body that lag-2 RMS chaos detector + softClampState deferred to Phase 2.5/2.6 per Q26. NOT an ARCHITECTURE.md amendment. Research-phase confirms commit-body wording template.

---

## Risks (Phase 2.4b-specific)

1. **HR-9 bit-exact regression failure on 10 carry-forward goldens.** Mitigation: pre-flight (Open Q4) captures all 10 currently-committed goldens BEFORE Phase 2.4b source edits. HR-9 short-circuit + IEEE 754 identity arithmetic + active-string-only gate are technical defences. If regression breaks, isolate SubHarmonicBias.h to separate translation unit OR add `__attribute__((noinline))` OR roll back to source structure with bias as pure constexpr swap-in. Phase 2.3 latent-drift risk re-surfacing surface — mitigated by canonical reproduce-goldens.sh invocation pattern.

2. **SchellengCalibration→F_max mapping semantic mismatch.** 2.4a table returns slow-LFO depth multiplier; 2.4b needs F_bow ceiling. Mapping research-phase locks (Open Q1). If recommended `effectiveBoost = subAmount * 0.8f * safeDepth` under-clamps at fallback cells (3 of 108 cells where safeDepth=0.5), bias could produce raucous-corner instability at those operating points. Mitigation: 36-combo `--sub-harmonics-stability` exercises these regimes; `failCount ≤ 2` v1.0 budget gives headroom.

3. **`pass_subharmAudible` threshold 0.10 may be too lax or too strict at default coefficients.** Research-phase pre-flights at SUB_HARMONICS=1.0 with `kForceBoost=1.8` etc. — measured ratio either confirms 10% threshold OR informs coefficient adjustment OR informs threshold relaxation (Phase 2.4-bis style soft-pass with v1.0 budget). Mitigation: research-phase Open Q3 + escalation path documented at discuss-phase.

4. **Period-doubling chaotic regime at SUB_HARMONICS=1.0 + INFINITE_SUSTAIN=1.0 on extreme bow params.** No chaos detector at v1.0 (deferred Q26). Layered defences: (a) SchellengCalibration F_max clamp inside bias, (b) algebraic saturator `x/sqrt(1+x²)` per rail, (c) loop-gain ceiling 0.9999999 hard-clamp. Mitigation: 36-combo stability matrix exercises 9 SUB+SUS=high combos; if any combo NaN/peak>1.0/runaway, escalate to chaos detector Phase 2.4-bis OR reduce `kForceBoost` from 1.8 → 1.4 (architecture §661 fallback 1).

5. **Active-string-only bias under crossfade.** During the 5 ms equal-power crossfade window between strings, bias for `activeStringIndex` fires on the new active string but the previous-active-shadow continues to ring out unbiased. Risk: audible "switch" event when SUB_HARMONICS > 0 mid-note-change (HR-9 active-string gate analogous to HR-1 vibrato — Phase 2.3 verified vibrato gates didn't audibly switch; same expected for bias). Mitigation: bias rampup via `subHarmonicsSmoothed.setTargetValue(...)` 30 ms ramp absorbs the discontinuity; research-phase confirms no audible click via stress-test combos that include note transitions.

6. **`subHarmonicsSmoothed.setTargetValue` UNCONDITIONAL each block at default 0.0.** Phase 2.3 macroSmoothed pin #11 precedent. If condition guards `setTargetValue` (e.g., `if (newValue != currentTarget) setTargetValue(...)`), denormal accumulation in smoother state can drift over very long sessions. Mitigation: pin into PLAN rev-9 preamble.

7. **SUB_HARMONICS default 0.0 audit.** All 10 current golden render configs MUST use default SUB_HARMONICS=0. Mitigation: research-phase audits each render config in main.cpp — confirm no mode passes a non-zero SUB_HARMONICS to the harness. If any mode passes >0, that golden becomes a re-baseline candidate (escalation to Phase 2.4b R35 atomic commit body).

8. **Bias's F_max clamp interaction with HR-4 (Schelleng wedge skip on SLOW_LFO_DEPTH=0).** Bias's F_max clamp via SchellengCalibration is INDEPENDENT of HR-4 wedge gate. At SLOW_LFO_DEPTH=0 (HR-4 short-circuits wedge math), bias still invokes safeDepthForString — but that lookup is read-only and cheap. No interaction conflict. Mitigation: research-phase confirms HR-4 + HR-9 are independent gates.

9. **Period-doubling spectral content shifts FFT bin selection.** At SUB_HARMONICS=1.0, period-doubling regime produces broadband transients in addition to f0/2 fundamental. FFT energy at f0/2 ± 0.5 Hz captures fundamental but not the sideband structure. Research-phase pre-flights to confirm `subharmEnergyRatio = E(f0/2_3bins) / E(f0_3bins) >= 0.10` is achievable with bias coefficients verbatim.

10. **R35 atomic commit interaction with R34-backfill chore.** R34-backfill chore `b64c8c4` propagated R34 sha into STATUS.md per R33 precedent. R35 atomic commit lands while R34-backfill is the most recent in-tree commit. R35-backfill chore propagates R35 sha into STATUS.md. Mitigation: R35-backfill chore commit follows R35 atomic commit (mirrors R34-backfill / R33-backfill / R26 precedent).

11. **`kForceBoost = 1.8` cap matches architecture §1.3 default.** Bias formula `F_bow *= 1.0f + 0.8f * subAmount` produces F_bow×1.8 at subAmount=1.0 — matches kForceBoost cap. Research-phase confirms mapping (no separate kForceBoost constant; 0.8 multiplier embedded in bias formula).

12. **Phase 2.4-bis backlog crowding.** Phase 2.4-bis open items (breathingAudible metric refinement, 3 fallback-cell reduction) not folded into Phase 2.4b. Risk: if Phase 2.4b verify exposes that a Phase 2.4-bis change would have made bias safer (e.g., 3 fallback cells become bias-affected operating points), escalate to Phase 2.4-bis cycle BEFORE Phase 2.4b R35 atomic commit. Mitigation: research-phase identifies fallback-cell intersection with `--sub-harmonics-stability` 36 combos; if intersection causes failures, Phase 2.4b scope expands or 2.4-bis lifts forward.

---

## Next Phase

Ready for: **research** phase — `/clear` then `/plugin-research O-Contrabass 2-dsp`

Research focus (Phase 2.4b):

1. **Resolve Open Questions #1–#12** — F_max mapping function, FFT analyzer specifics, threshold tuning, HR-9 pre-flight, wall-clock pre-flight, MIDI per combo, pass criteria, SubHarmonicBias.h API, voice-level vs per-string inputs, R35 task breakdown, per-string variation, commit-body footnote.
2. **HR-9 bit-exact pre-flight (Open Q4)** — render all 10 currently-committed goldens BEFORE Phase 2.4b source edits via reproduce-goldens.sh; capture sha256s + verify against committed values. If any drift, INVESTIGATE before plan-phase.
3. **Single-combo wall-clock pre-flight (Open Q5)** — render ONE combo at extreme settings; measure block-time ratio + wall-clock per combo; extrapolate to 36-combo total.
4. **SUB_HARMONICS=1.0 spectral pre-flight (Open Q2/Q3)** — render baseline at SUB_HARMONICS=0 (system noise floor) and SUB_HARMONICS=1.0 with §457 coefficients verbatim; measure subharmEnergyRatio; confirm 10% threshold OR escalate.
5. **SchellengCalibration mapping decision (Open Q1)** — analyze 105/108 + 3 fallback cells; lock mapping function; document v1.0 fallback behavior at 3 cells.
6. **Append RESEARCH §18** — document all resolutions above. (No §17 changes; Phase 2.4a §17 locked.)

After research: plan-phase (PLAN rev-9) writes R35 task breakdown verbatim against this CONTEXT + research findings; execute-phase performs implementation + new goldens + R35 atomic commit; verify-phase confirms Gate 6b invariants.

---

## Audit Trail (rev-7 supersedes rev-6)

**rev-1 (2026-04-26):** Phase 2.1 broad discuss. Cycle scope = Phase 2.1 (sub-phases a/b/c).

**rev-2 (2026-04-26):** Phase 2.1a closure (Option A, R7) + Phase 2.1b opening (module extraction, Gate 2). Phase 2.1b verified 2026-04-27 (R8a `bd5fae0` + R15 `ef0604d`, Gate 2 PASS).

**rev-3 (2026-04-27):** Phase 2.1c opening — cascaded allpass dispersion. Verified 2026-04-27 (R20 `5759e5e`, Gate 3 PASS).

**rev-4 (2026-04-27):** Phase 2.2 opening — 4-string EADG + per-string detune + per-string M-table. Verified 2026-04-27 (R26 `131c2c7`, Gate 4 PASS).

**rev-5 (2026-04-27):** Phase 2.3 opening — Vibrato + Slow-Bow LFO + Schelleng wedge clamp + EXPRESSION_MACRO. HR-1..HR-4 binding. Verified 2026-04-27 (R33 `af54571`, Gate 5 PASS with rebaseline of 4 audible carry-forward goldens).

**rev-6 (2026-04-27):** Phase 2.4a opening — Schelleng wedge bass-register calibration polynomial + 108-combo stability matrix dual-purpose render + `pass_breathingAudible` 5%→20% threshold restoration. HR-5..HR-8 binding. Verified 2026-04-28 (R34 `4c926bb`, Gate 6a CLEARED — 3 strict-PASS + 2 soft-PASS within v1.0 budgets) + R34-backfill chore `b64c8c4`. Phase 2.4-bis backlog logged: tune Step 4 modulation gain to hit 20% peak-to-peak OR refine breathingAudible per-cycle metric; reduce v1.0 fallback cells via downstream-defense tightening.

**rev-7 (this document, 2026-04-28):** Phase 2.4b opening — Sub-Harmonic Bias DSP-07 (ARCHITECTURE §457). 11 approach decisions Q23–Q33 user-confirmed: scope = bias DSP-07 only (Q23); per-plugin in BowedContrabassVoice + new SubHarmonicBias.h header (Q24); F_max ceiling via Phase 2.4a SchellengCalibration table reuse (Q25); 36-combo `--sub-harmonics-stability` matrix without BODY_DAMPING + chaos detector deferred (Q26); FFT energy ratio E(f0/2)/E(f0) ≥ 0.10 at SUB_HARMONICS=1.0 on E1 (Q27); Logic AU smoke deferred non-blocking R37 (Q28); carry-forward 10 + 2 new goldens (Q29); 30 ms SmoothedValue on SUB_HARMONICS confirmed (Q30); active-string-only bias HR-1 precedent (Q31); R35 atomic commit + R35-backfill chore (Q32); rev-7 covers 2.4b only (Q33). 12 open questions handed to research-phase: SchellengCalibration→F_max mapping, FFT specs, threshold tuning, HR-9 pre-flight, wall-clock pre-flight, MIDI per combo, pass criteria, SubHarmonicBias.h API, voice-level vs per-string inputs, R35 task breakdown, per-string variation, commit-body footnote. NEW Hard Rule HR-9 binding: SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate. Phase 2.4c (autocorrelator + saturator-tail) gets fresh CONTEXT rev-8 when discuss-phase opens after 2.4b verify. Continues atomic-commit sequence R7 → R15 → R20 → R26 → R33 → R34 → R35.

**Inherited verbatim from rev-6 (not re-litigated):**

- All Phase 2.3 modulator surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order)
- HR-1..HR-4 hard rules (literal-zero short-circuits + IEEE 754 identity-arithmetic + Schelleng skip on zero LFO depth)
- HR-5..HR-8 hard rules (Phase 2.4a — `inline constexpr` linkage on SchellengCalibration.h; calibration behind HR-4 gate ONLY; matrix-stability bypass via weak-symbol; trilinear IEEE 754 identity arithmetic)
- `lastSafeDepth.store(...)` instrumentation hook signature (pin #4 from PLAN rev-7)
- 10 currently-committed goldens (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep + slow-lfo re-baselined + schelleng-stress re-baselined)
- Phase 2.4a `matrix-stability.{wav.sha256,json}` `6db67707…` golden
- Atomic-commit gate-first principle (R7 → R15 → R20 → R26 → R33 → R34 → R35)
- Saturator-tail Phase 2.4c follow-up parking
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify
- E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7) — separate concern
- Primary listening DAW: Logic Pro (AU)
- Sample-rate strategy: internal 88.2 / 96 kHz at friction junction
- Bow-friction module v1.0.0 at `modules/synthesis/bow-friction/`
- Per-plugin `DispersionFilter.h` + `SchellengCalibration.h` (NOT extracted to shared module)
- 29 APVTS parameters; parameter-spec.md sha256 `77638e25…` carries forward unchanged
- Stage-1 contract NOT amended in Phase 2.4b
- ARCHITECTURE.md NOT amended in Phase 2.4b
- Phase 2.4-bis backlog parked (separate future cycle)

**New in rev-7:**

- Q23 Phase 2.4b scope = sub-harmonic bias DSP-07 only
- Q24 per-plugin in BowedContrabassVoice.cpp + new `Source/DSP/SubHarmonicBias.h` header
- Q25 SchellengCalibration trilinear table reuse for F_max ceiling
- Q26 36-combo `--sub-harmonics-stability` (4 strings × 3 INFINITE_SUSTAIN × 3 SUB_HARMONICS)
- Q27 FFT energy ratio `E(f0/2)/E(f0) >= 0.10` at SUB_HARMONICS=1.0 on E1
- Q28 Logic AU smoke deferred non-blocking
- Q29 carry-forward 10 + 2 new goldens (`--sub-harmonics` + `--sub-harmonics-stability`)
- Q30 30 ms `subHarmonicsSmoothed` SmoothedValue
- Q31 active-string-only bias (HR-1 precedent)
- Q32 R35 atomic commit + R35-backfill chore
- Q33 rev-7 covers 2.4b only
- NEW HR-9 hard rule (SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate)
- New `Source/DSP/SubHarmonicBias.h` header (~80 LOC)
- New harness CLI flags `--sub-harmonics` + `--sub-harmonics-stability`
- New goldens `sub-harmonics.{wav.sha256,json,json.sha256}` + `sub-harmonics-stability.{wav.sha256,json,json.sha256}`
- Step 2.5 (sub-harmonic bias evaluation) inserted into per-block 7-step order
- Five-item Gate 6b bar: (1) all 10 carry-forward goldens (8 + 2 Phase 2.4a re-baselined) byte-identical via reproduce-goldens.sh; (2) new `--sub-harmonics` golden + `pass_subharmAudible >= 0.10`; (3) new `--sub-harmonics-stability` `pass_all_36 = true` OR `failCount ≤ 2` v1.0 budget; (4) auval AU VALIDATION SUCCEEDED + pluginval-10 SUCCESS; (5) matrix-stability `6db67707…` carries forward byte-identical. R37 Logic AU smoke deferred non-blocking.
