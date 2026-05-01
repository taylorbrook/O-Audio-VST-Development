# Stage 2: DSP — Context (rev-10)

**Date:** 2026-04-30
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.5 — Body Resonator (8-mode static-Q biquad bank) + Bow Noise Generator (3-band BPF + slip bursts)**
**Supersedes:** rev-9-bis (Phase 2.4c-bis — source-change in-loop saturator port from `x / sqrt(1 + x²)` to `sat · tanh(x / sat)` with `sat = 4.0f` on both rails of `Source/DSP/WaveguideString.cpp:204–206`, dated 2026-04-29). rev-9-bis closed 2026-04-29 with R36-bis atomic commit (`1044bed41574be5d0714983f7910cac8bda2edec`, Gate 6c-bis SOFT-PASS at bin 64 = −7.97 dB / `|Δ| = 0.80 dB` vs O-Bowed reference −7.17 dB; 5/5 invariants cleared at R36-bis-e/f post-audition; HR-1..HR-10 preserved; HR-11 retired). All rev-9-bis contracts that remain locked are inherited verbatim and not re-litigated.

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle activates **Phase 2.5** — body resonator (8-mode parallel biquad bank, static-Q v1.0) + bow noise generator (3-band BPF + per-period slip bursts). Phase 2.5 is the first cycle that lands a NEW DSP block (not a tune of an existing block) since Phase 2.4b's sub-harmonic bias Step 2.5. It also closes two BRIEF.md "must"/"should" requirements: **DSP-03 (Bass-tuned wood body resonator — must)** and **DSP-04 (Bow noise / rosin grit — should)**.

The cycle is bounded by ARCHITECTURE §"Body Resonator" (8-mode bank verbatim: 60 / 98 / 115 / 175 / 235 / 340 / 700 / 1200 Hz with default Q + gain table per Askenfelt 1982 / Rossing 2010), §"Bow Noise Generator" (3-band BPF at 700 / 1500 / 3000 Hz + per-period slip bursts decay 0.999 + sums AFTER body), and §"Per-block processing order" (steps 15 + 16 inserted between waveguide downsample and master output). All five Stage-1 parameters that drive Phase 2.5 — `BRIGHTNESS`, `BOW_NOISE`, `BODY_SIZE`, `BODY_DAMPING`, `BODY_MIX` — are already declared in PluginProcessor.cpp (parameter-spec.md sha256 `77638e25…`, no Stage-1 amendment).

Phase 2.5 is the **most dramatic audible-character cycle in Stage 2** so far. The waveguide-only output (current state from R36-bis post-port) is a raw bowed-string signal; body resonator + bow noise transforms it into the first "convincing orchestral arco" candidate per BRIEF.md's stated identity. Saturator-tail regression re-measurement (carry-forward from rev-8 risk #10 / rev-9-bis Phase 2.5-awareness risk) is non-blocking evidence — the body bank is at host rate and downstream of the saturator, so coupling effects are characterized rather than gated.

R38 Logic AU audition is **BLOCKING** before R37 atomic commit (mirrors R37-bis Phase 2.4c-bis precedent — audible source-edit → BLOCKING audition). User A/Bs raw-string from `1044bed` (R36-bis post-port) vs body-engaged from working-tree before R37 lands.

Wolf-region suppression (ARCHITECTURE §"Body Resonator" default-ON Mode #2 Q drop on fundamental lock within ±15 cents for >150 ms) is **deferred to v1.1 in Phase 2.5** alongside the already-deferred Authentic Arco toggle (ARCHITECTURE Open Decision §3) — this is a CONTEXT-explicit deviation from ARCHITECTURE §"Body Resonator" wolf-region default, justified by runtime fundamental-lock-detection nondeterminism risk (autocorrelator-style, the same class that bit Phase 2.4c). v1.0 ships static-Q body bank; if perceptual feedback in DAW audition reveals wolf-tone issues, schedule Phase 2.5-bis or v1.1 cycle.

After Phase 2.5 verifies (Gate 7 PASS), Phase 2.6 (master saturator + zero-latency limiter + stereo width + microtonal + MPE + Note Expression) opens with fresh CONTEXT **rev-11**.

---

## Cycle Scope

**Goal:** Add Phase 2.5 body resonator (8-mode parallel biquad bank, static Q, bass-tuned modes per Askenfelt 1982 / Rossing 2010) + bow noise generator (3-band BPF + per-period slip bursts) as new Steps 8 (body) and 9 (bow noise) in the per-block processing order, AFTER waveguide downsample and BEFORE the (Phase 2.6) master output chain. Land all five Stage-1 body/noise parameters into a working DSP path (`BRIGHTNESS`, `BOW_NOISE`, `BODY_SIZE`, `BODY_DAMPING`, `BODY_MIX`); keep DSP-07 retune backlog parked; ship static-Q body bank (wolf-region suppression deferred to v1.1). Single R37 atomic commit lands the source edits (new headers + voice/processor wiring) + 13 re-baselined audible goldens + matrix-stability evidence re-render + saturator-tail-comparison regression-evidence golden + RESEARCH §20 verdict subsection + STATUS / SUMMARY / VERIFICATION / CONTEXT updates. R37-backfill chore propagates R37 sha into STATUS.md per R34/R35/R36/R36-bis backfill precedent.

**In scope:**

- **`plugins/O-Contrabass/Source/DSP/BodyResonator.{h,cpp}`** — NEW per-plugin header + (optional) cpp pair, **verbatim copy from O-Bowed `Source/DSP/BodyResonator.{h,cpp}`** with bass-tuned mode table substitution. 8 parallel `juce::dsp::IIR::Filter<float>` biquads with bass mode set:
  | # | Mode | Freq (Hz) | Q | Gain (dB) |
  |---|------|-----------|---|-----------|
  | 1 | A0 (Helmholtz / f-hole) | 60 | 14 | −2 |
  | 2 | T1 / B1− (main wood) | 98 | 11 | 0 |
  | 3 | B1+ (corpus) | 115 | 9 | −1 |
  | 4 | Cluster low | 175 | 8 | −3 |
  | 5 | Cluster mid | 235 | 7 | −4 |
  | 6 | Cluster high | 340 | 6 | −5 |
  | 7 | Bridge cluster | 700 | 5 | −7 |
  | 8 | Bridge hill | 1200 | 2.5 | −6 |
  Coefficients per ARCHITECTURE §"Body Resonator (Parallel Biquad Bank)": `fc[i] = jlimit(20, sr · 0.45, defaultFreq[i] / size_scalar)`, `Q[i] = defaultQ[i] · max(0.15, 1 − 0.85·damping)`, `g[i] = decibelsToGain(defaultGainDb[i] + 1.5·(s − 0.75))`. 35 Hz HP on dry path. Wet/dry mix: `out = (1 − mix) · highpass35Hz(in) + mix · wet`. Per-block coefficient recompute; 30 ms `SmoothedValue` on Size/Damping/Mix. **Wolf-region suppression NOT implemented in v1.0** — Mode #2 Q stays at 11 statically. Module extraction (shared `modules/synthesis/body-resonator/`) deferred post-v1.0 (mirrors DispersionFilter / SchellengCalibration / SubHarmonicBias per-plugin precedent).

- **`plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h`** — NEW per-plugin header, **verbatim copy from O-Bowed `Source/DSP/BowNoiseGenerator.h`** with bass-spectral target substitution. 3-band BPF at **700 Hz / 1500 Hz / 3000 Hz** (Q ≈ 1.0–1.5 each, bass close-mic spectral target per ARCHITECTURE §"Bow Noise Generator" — vs violin's 1–8 kHz). Envelope: `bowEnergy = clamp(0, 1, |v_bow| · F_bow / (v_ref · F_ref))` with `v_ref = 0.3 m/s`, `F_ref = 2.0 N`. Per-period slip bursts on Helmholtz slip-detection at zero-crossing of friction force from stick to slip; exponential decay `decay = 0.999` at 48 kHz. Sums AFTER body resonator. Default `BOW_NOISE = 0.35` (heavier than O-Bowed default per BRIEF.md "intimate close-mic" sonic target — already declared in parameter-spec.md).

- **`plugins/O-Contrabass/Source/BowedContrabassVoice.{h,cpp}`** — wire body resonator + bow noise as new processing steps. Per-block evaluation order extension (verbatim from ARCHITECTURE §"Per-block processing order" steps 15–16):
  - Step 7 end-state from rev-9-bis (in-loop saturator port + downsample) carry-forward verbatim.
  - **NEW Step 8:** Body resonator — 8-mode parallel bank + 35 Hz dry HP path → wet/dry mix. Coefficient recompute at block-start (read smoothed Size/Damping/Mix BEFORE compute). Filter state continues across blocks (no state reset per block).
  - **NEW Step 9:** Bow noise — 3-band BPF + slip bursts driven by `bowEnergy` envelope. Sums into voice output AFTER body wet/dry mix.
  - Slip-detection trigger: zero-crossing of friction force from stick to slip — research-phase confirms exact integration site (likely from `frictionModel.lastForceState()` accessor or equivalent O-Bowed pattern).
  - Master saturator + limiter + stereo width + output gain stay deferred to **Phase 2.6**. Phase 2.5 voice output goes directly to bus (host-rate, no master chain).
  - HR-9 short-circuit at `SUB_HARMONICS = 0` (Step 2.5) preserved verbatim. HR-10 friction module ABI preservation (`setRosin` relocation + ROSIN inverse identity) preserved verbatim.

- **`plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}`** — APVTS reads + voice-side parameter routing for `BRIGHTNESS`, `BOW_NOISE`, `BODY_SIZE`, `BODY_DAMPING`, `BODY_MIX`. All 5 already declared at PluginProcessor.cpp:52, :60, :62, :64, :66 (parameter-spec.md sha256 `77638e25…` carries forward unchanged). Voice receives raw atomics + `SmoothedValue` ramps internally; processor itself does NOT process body/noise.

- **`plugins/O-Contrabass/tests/render-harness/golden/{stiffness-zero-pre, string-A, string-D, string-G, detune-sweep-A, note-sequence, macro-sweep, slow-lfo, schelleng-stress, sub-harmonics, sub-harmonics-stability, vibrato, saturator-tail-comparison}.{wav.sha256, json, json.sha256}`** — **13 audible goldens re-baseline**. Body bank + bow noise materially shift output spectrum on every audible golden (steady-state energy balance + new spectral layer post-waveguide). vibrato carries forward conditional only IF research-phase pre-flight shows `vibrato.wav.sha256` byte-identical post-body (low probability — body coupling shifts output; default expectation is re-baseline). saturator-tail-comparison re-baselines because body bank is downstream and shifts the post-saturator decay envelope at bin 64.

- **`plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.{wav.sha256, json.sha256, json}`** — re-render **evidence-only** (NOT committed as a re-baselined golden in R37; mirrors Phase 2.4a R34b + Phase 2.4c-bis R36-bis evidence-golden pattern). Document raucous-corner cells (current 3 fails carry-forward from Phase 2.4a): stabilise / hold / NEW corners surfacing. Body bank is a post-saturator linear filter — NEW raucous corners would be regression risk (low-probability; biquads are L2-bounded with Q drops gracefully under damping).

- **`plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`** — 13 entries unchanged in count. Only sha256 values change inside per-entry comparisons (13 audible re-baselines).

- **`plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §20 (NEW)** — append Phase 2.5 verdict subsection with sub-sections:
  - §20.1 BodyResonator extraction from O-Bowed (file/line provenance, bass-tuned mode table substitution, source delta size)
  - §20.2 BowNoiseGenerator extraction from O-Bowed (file/line provenance, bass-spectral targets substitution)
  - §20.3 Voice integration — Step 8 (body) + Step 9 (bow noise) wiring; slip-detection accessor identification
  - §20.4 Saturator-tail regression re-measurement under body coupling — bin 64 vs Phase 2.4c-bis R36-bis baseline (−7.97 dB); verify §19.3.3 analytic bound (≤2 dB at canonical amplitude) holds under body-coupled topology
  - §20.5 13-audible-golden re-baseline sha256 measurements (post-execute scratch-space pre-flight)
  - §20.6 Matrix-stability post-Phase-2.5 evidence (raucous corners stabilise / hold / NEW)
  - §20.7 Sub-harmonics post-body coupling re-measurement (`subharmEnergyRatio` at body output vs Phase 2.4c-bis `0.358` at waveguide output; document landed value, evidence-only)
  - §20.8 R38 Logic AU audition outcome (BLOCKING)
  - §20.9 R37 9-task breakdown (R37-pre / R37a / R37b / R37c / R37d / R37e / R37f / R37 atomic / R37-backfill)
  - §20.10 Verdict — body+noise WORKED (DSP-03 + DSP-04 CLOSED) OR WORKED-PARTIALLY (Phase 2.5-bis backlog logged; e.g., wolf-tone audible at G2 sustained → schedule wolf-suppression cycle) OR REGRESSION (body coupling reveals saturator-tail divergence > 2 dB; investigate before R37 atomic)
  - §20.11 Carry-forward to Phase 2.6 (master saturator + limiter + width + microtonal + MPE + Note Expression) and end-of-Stage-2 verify (§"DC Blocker" + §"In-loop saturator" amendments) and v1.1 backlog (wolf-region suppression + Authentic Arco toggle + chaos detector + softClampState)

- **`plugins/O-Contrabass/.planning/STATUS.md`** — flip `phase` per cycle progression; flip `status` to `phase_2_5_discuss_complete`, `next_action` to `phase_2_5_research`; add `phase_2_5_discuss_carry_forward` block; bump `cycle_scope` to phase_2_5_body_resonator_plus_bow_noise_v1_zero_static_q_bank.

- **`plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` rev-12** — Phase 2.5 plan-phase output, supersedes rev-11 with post-research instantiation. R37 task breakdown verbatim from RESEARCH §20.9 + research-phase pre-flight findings.

- **`plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md`, `VERIFICATION.md`** — append Phase 2.5 sections at execute / verify phases respectively. VERIFICATION.md flips DSP-03 (must) and DSP-04 (should) from "⏸️ Deferred (Phase 2.5)" to "✅ Complete" or "⚠️ Partial" depending on Gate 7 outcome.

- **`plugins/O-Contrabass/CMakeLists.txt`** — add `Source/DSP/BodyResonator.cpp` to source list (if `.cpp` present; `.h`-only is also acceptable per O-Bowed pattern). `Source/DSP/BowNoiseGenerator.h` is `.h`-only (no CMake update needed if header-only).

**Out of scope (deferred elsewhere):**

- **Phase 2.4-bis backlog** — kForceBoost retune (DSP-07; subharmEnergyRatio above 0.40 strict; currently 0.358 SOFT-PASS post-tanh-port), Step 4 modulation gain / breathingAudible metric refinement (DSP-08; 20% peak-to-peak), VIBRATO_DEPTH→peakDepthCents transfer tune (DSP-09 from 2.4c deviation #6; strict 12¢ peak), 3 v1.0 fallback-cell reduction (Phase 2.4a). **All stay parked.** Strict body+noise scope keeps the source delta unambiguous and the saturator-tail-regression evidence base clean.
- **Wolf-region suppression** (ARCHITECTURE §"Body Resonator" Mode #2 Q drop on fundamental lock within ±15 cents for >150 ms) — deferred to v1.1 alongside Authentic Arco toggle. Static-Q body bank in Phase 2.5. **DEVIATION FROM ARCHITECTURE §"Body Resonator" wolf-region default ON** — flagged explicitly in §"Approach Decisions" Q55. Justification: runtime fundamental-lock-detection nondeterminism risk (autocorrelator-style, same class that bit Phase 2.4c). If wolf-tone audible at G2 sustained during R38 audition or post-Phase 2.6 user testing, schedule Phase 2.5-bis cycle.
- **Master saturator + zero-latency feedforward limiter + stereo width + output gain** — Phase 2.6.
- **Microtonal (Scala / TUN / MTS-ESP) + MPE + Note Expression** — Phase 2.6.
- **Chaos detector + softClampState** — v1.1 (deferred from Phase 2.4b R35 commit-body footnote; rev-9-bis carried forward "Phase 2.5/2.6"; now explicitly v1.1 since Phase 2.5 stays scope-strict).
- **ARCHITECTURE.md amendments** — §"DC Blocker" + §"In-loop saturator" stay deferred to end-of-Stage-2 verify (post-Phase-2.6). Phase 2.4c-bis post-port saturator-tail golden + Phase 2.4c pre-port reference (`c7e845ea…` from `115dbf4` worktree) feed the §"In-loop saturator" amendment evidence base; Phase 2.5 saturator-tail regression measurement also feeds it (post-body coupling characterisation).
- **E1 dispersion calibration polynomial follow-up** (Phase 2.1c Risk #7) — separate `a(B, I)` cascaded-allpass concern; v1.1 or post-v1.0.
- **Per-string `--vibrato` / `--saturator-tail-comparison` variants** — v1.1 or Phase 2.4-bis.
- **Body Resonator shared-module extraction** (`modules/synthesis/body-resonator/`) — post-v1.0 refactor. v1.0 uses per-plugin `Source/DSP/BodyResonator.{h,cpp}` (DispersionFilter / SchellengCalibration / SubHarmonicBias precedent).
- **Bow Noise shared-module extraction** (`modules/synthesis/bow-noise/`) — post-v1.0 refactor. v1.0 uses per-plugin `Source/DSP/BowNoiseGenerator.h`.
- **HR-12 (any new hard rule)** — none introduced (Q59). ARCHITECTURE §"Oversampling Strategy" already locks the host-rate body-bank boundary; ARCHITECTURE §"Body Resonator: Fixed Wood vs Morphable" already locks single-material; HR-1..HR-10 cover existing invariants.
- **Stage-1 contract amendment** — parameter-spec.md sha256 `77638e25…` carries forward unchanged (5 body/noise params already declared).
- **ARCHITECTURE.md amendment in this cycle** — body resonator + bow noise IS architecture verbatim; no ARCHITECTURE delta. Wolf-region deferral is a CONTEXT-flagged deviation, NOT an ARCHITECTURE amendment (wolf default-ON intent preserved for v1.1 reactivation).
- **Output-path saturator/limiter** — Phase 2.6.
- **R38 deferred-non-blocking audition path** — overridden (Q57). R38 is BLOCKING (mirrors R37-bis precedent; first NEW DSP block since 2.4b → BLOCKING).

---

## Requirements Confirmed (Phase 2.5-relevant subsets of locked contracts)

- **DSP-03 / Bass-Tuned Wood Body Resonator** (must, currently pending): Phase 2.5 lands the 8-mode parallel biquad bank (60 / 98 / 115 / 175 / 235 / 340 / 700 / 1200 Hz), 35 Hz HP on dry path, parametric Size/Damping/Mix with 30 ms smoothing. Static-Q in v1.0 (wolf-region suppression deferred). Acceptance criteria from ROADMAP §"Phase 2.5 Test Criteria":
  - 8 spectral peaks at default frequencies with correct relative gains (impulse response check at execute-phase pre-flight).
  - BODY_SIZE 0% → 100% sweep: peaks slide smoothly, no zipper noise.
  - BODY_DAMPING sweep: ring-down time changes audibly without clicks.
  - BODY_MIX at low frequencies (E1 = 41 Hz): smooth amplitude blend, no comb-filter teeth.
  - Wolf region (G2 sustained, default damping): slight bloom but no audible beating ← v1.0 may exhibit beating since wolf-suppression deferred; document outcome at R38 audition; if audible → schedule Phase 2.5-bis.
  - Orchestral character: A/B vs reference orchestral library bass sustain at G2 → "in same sonic family" subjective bar.

- **DSP-04 / Bow Noise / Rosin Grit** (should, currently pending): Phase 2.5 lands the 3-band BPF (700 / 1500 / 3000 Hz) + per-period slip bursts (decay 0.999) summed AFTER body. Acceptance criteria from ROADMAP §"Phase 2.5 Test Criteria":
  - BOW_NOISE 0% → 100%: noise level audible at low pressure, fades to silence at zero.
  - Bow direction reversal: brief noise burst (5–15 ms wideband decay) — slip-burst trigger validation.

- **DSP-01 / Bass-Range Waveguide Stability**: body bank + bow noise are downstream of waveguide; stability invariant (no NaN, no peak > 1.0, no runaway, no denormal CPU spike) MUST hold across all 4 strings × INFINITE_SUSTAIN ∈ [0, 1] post-body-coupling; verified via re-rendered matrix-stability evidence + 13 audible re-baseline goldens reproducing byte-identically across re-renders + auval / pluginval-10 SUCCESS.

- **DSP-09 / Layered Expression** (vibrato section): saturator + body bank are downstream of vibrato modulator path (vibrato modulates BOW_SPEED at Step 4; body bank at NEW Step 8). Phase 2.4c R36c restored-strict-with-deviations metrics (`peakDepthCents=9.526`, `vibratoRateHzMeasured=4.978 Hz`, `onsetTimeMs=1168`) MUST reproduce on post-Phase-2.5 WAV within tolerance OR be re-baselined if body bank subtly shifts the vibrato envelope. Research-phase pre-flights this. Default expectation: re-baseline (body bank colors output spectrum for any audible signal).

- **QUAL-01** (no audio artifacts at normal ranges): Gate 7 includes 13-audible-golden bit-deterministic-across-re-renders bar (HR-9-style IEEE 754 identity-arithmetic NOT applicable — body+noise are NEW DSP blocks, not no-op gates; goldens become NEW reference values). Body-coefficient updates click-free per ARCHITECTURE §"Body Resonator Click-Free Coefficient Updates" (per-block recompute + 30 ms `SmoothedValue`).

- **PERF-01** (no allocations in `processBlock`): body bank is `juce::dsp::IIR::Filter<float>` (RT-safe, no allocations); bow noise is `juce::Random` + `juce::dsp::IIR::Filter` (RT-safe). pluginval-10 fuzz + Parameter thread safety re-confirm. `juce::ScopedNoDenormals` already in place.

- **PERF-02** (< 5% CPU on M1): ARCHITECTURE §"Performance" estimates body bank ~0.4% + bow noise ~0.4%; cumulative Phase 2.5 increment is ~0.8% (well within 5% target with all prior phases summing to ~3.2% pre-Phase-2.5).

- **PERF-03** (zero algorithmic latency): body bank is parallel biquad (zero group delay at design frequencies per ARCHITECTURE §"Body Resonator → Bow Noise" integration note). Bow noise is post-summed (no delay). No `setLatencySamples()` change in Phase 2.5.

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**

- All 29 APVTS parameter IDs, ranges, skews, defaults — `parameter-spec.md` (sha256:`77638e25…`). **NO Stage-1 contract amendment in Phase 2.5.** All 5 Phase-2.5-relevant params (`BRIGHTNESS`, `BOW_NOISE`, `BODY_SIZE`, `BODY_DAMPING`, `BODY_MIX`) already declared.
- DSP architecture (`research/ARCHITECTURE.md`, sha256:`3cb26814…`). **NO ARCHITECTURE.md amendment in Phase 2.5** — body resonator + bow noise IS architecture verbatim; wolf-region deferral is a CONTEXT-flagged deviation, NOT an ARCHITECTURE amendment. §"In-loop saturator" + §"DC Blocker" amendments stay deferred to end-of-Stage-2 verify (post-Phase-2.6).
- ROADMAP phasing (sha256:`106639f6…`).
- `modules/synthesis/bow-friction/` v1.0.0 (Phase 2.1b) — value-class deterministic; **Phase 2.5 does NOT touch friction module surface**.
- `Source/DSP/DispersionFilter.h` (Phase 2.1c, R20 commit `5759e5e`) — verbatim consume.
- `Source/DSP/SchellengCalibration.h` (Phase 2.4a, R34 commit `4c926bb`) — verbatim consume.
- `Source/DSP/SubHarmonicBias.h` (Phase 2.4b, R35 commit `3de8b66`) — verbatim consume.
- `Source/DSP/WaveguideString.cpp` (Phase 2.4c-bis, R36-bis commit `1044bed`) — post-port `sat · tanh(x / sat)` saturator at `:204–206` verbatim consume; **Phase 2.5 does NOT touch WaveguideString**.
- `Source/BowedContrabassVoice.{h,cpp}` (Phase 2.4b end-state) — Step 1–7 + Step 2.5 carry-forward verbatim; Phase 2.5 ONLY appends new Step 8 (body) + Step 9 (bow noise).
- `Source/PluginProcessor.{h,cpp}` — 29 APVTS params + Phase 2.4b voice wiring; Phase 2.5 adds voice-side body/noise param routing only (no new params, no new PluginProcessor logic — voice owns body+noise instances).
- Phase 2.3 modulator surface + 7-step + Step 2.5 (Phase 2.4b) + HR-1..HR-10 verbatim carry-forward.
- HR-11 (Phase 2.4c zero-production-DSP-edits) — **RETIRED** at Phase 2.4c-bis. Audit history preserves binding for Phase 2.4c only.
- Per-block evaluation order Step 1–7 + Step 2.5 — unchanged. Phase 2.5 ONLY APPENDS new Step 8 + Step 9 between waveguide downsample and (Phase 2.6) master output.

**Phase 2.5-specific constraints:**

- **Source-edit scope: 4 production files** — `Source/DSP/BodyResonator.{h,cpp}` (NEW), `Source/DSP/BowNoiseGenerator.h` (NEW), `Source/BowedContrabassVoice.{h,cpp}` (M; append Step 8/9 + body/noise instance ownership + smoothed param ramps), `Source/PluginProcessor.{h,cpp}` (M; voice-side routing for 5 body/noise param atomics). `Source/DSP/WaveguideString.cpp`, `Source/DSP/DispersionFilter.h`, `Source/DSP/SchellengCalibration.h`, `Source/DSP/SubHarmonicBias.h`, `Source/BowedMPESynthesiser.{h,cpp}` all carry forward verbatim. Audit hook: `git diff --stat HEAD -- plugins/O-Contrabass/Source/` MUST report the EXACT 4-file set at R37-pre + R37-e tripwires (NEW additions count as modifications per `--stat`).
- **`Source/DSP/BodyResonator.{h,cpp}` source provenance**: verbatim copy from O-Bowed `Source/DSP/BodyResonator.{h,cpp}` (research-phase confirms file paths + line range). Bass-tuned mode table substitution is the ONLY semantic delta (8 frequency / Q / gain triplets per the table above). Wolf-region suppression code path (if present in O-Bowed) is **NOT ported** in Phase 2.5 (Q55 deferral). Mode-count and biquad-bank topology unchanged.
- **`Source/DSP/BowNoiseGenerator.h` source provenance**: verbatim copy from O-Bowed `Source/DSP/BowNoiseGenerator.h` with bass-spectral target substitution (700 / 1500 / 3000 Hz vs O-Bowed's likely violin-targeted 1500 / 3000 / 5000 Hz or similar; research-phase confirms exact O-Bowed values). Slip-burst trigger source identification is research-phase pre-flight (likely `frictionModel.lastForceState()` accessor or equivalent).
- **13 audible goldens re-baseline; matrix-stability re-render evidence-only.** All audible goldens re-baseline because body bank + bow noise are post-waveguide spectral colorers active for any audible signal. saturator-tail-comparison re-baselines because body bank is downstream and shifts post-saturator decay envelope at bin 64 — NEW post-Phase-2.5 reference becomes the input-to-§"In-loop saturator"-amendment evidence base alongside Phase 2.4c-bis R36-bis (`5c45d176…`) and Phase 2.4c R36 (`c7e845ea…`).
- **Saturator-tail regression measurement at canonical operating point**: post-Phase-2.5 `--saturator-tail-comparison` bin 64 should remain WITHIN soft-band [−8.17, −6.17] dB rel max from Phase 2.4c-bis Q47 contract. Body bank is a linear filter (no nonlinear backflow into saturator); analytical expectation: bin 64 shifts within ~±1 dB depending on `BODY_MIX` (default 0.80 = 80% wet) coloring of the decay tail. If post-body bin 64 falls outside [−9, −5] dB rel max → flag as evidence for §"In-loop saturator" amendment + investigate (NOT a Gate 7 BLOCKER unless > 4 dB shift indicates body-coupling instability, which would be regression risk).
- **Verifying §19.3.3 analytic bound (≤ 2 dB at canonical amplitude)** under post-port saturator + body-coupling: research-phase computes and documents bound preservation OR characterized residual divergence. NOT a Gate 7 BLOCKER (evidence-only); feeds end-of-Stage-2 §"In-loop saturator" amendment.
- **R38 Logic AU smoke is BLOCKING.** First NEW DSP block since Phase 2.4b R35 sub-harmonic bias (and that was minimal); body resonator + bow noise are dramatic audible-character transformations. User auditions raw-string state from `1044bed` checkout (or `git stash`-based pre-execute reference) vs body-engaged from working-tree in Logic Pro (AU) before R37 atomic commit lands. Mirrors R37-bis Phase 2.4c-bis precedent. PASS criteria: post-Phase-2.5 sounds "convincing orchestral arco bass" per BRIEF.md DSP-03 + DSP-04 acceptance — wood body resonance present, bow-noise texture audible at low pressure without overwhelming the tone, no unexpected character changes (transient artifacts, peak-amplitude shifts, vibrato envelope distortion at body coupling, harmonic spectrum changes from bow-noise-into-body-mistakenly).
- **Strict bit-deterministic-across-re-renders bar** for all 13 audible re-baselines. Research-phase pre-flights 3 back-to-back renders to confirm sha256 stability before plan-phase locks expected sha256s. If non-deterministic, escalate (likely needs explicit RNG seed determinism in `BowNoiseGenerator.h` — per ARCHITECTURE §"Special Considerations: Performance" `juce::Random`-seeded white noise; seed must be voice-construction-time fixed, NOT runtime random).
- **NO Stage-1 contract amendment.** parameter-spec.md sha256 `77638e25…` carries forward unchanged.
- **NO ARCHITECTURE.md amendment.** End-of-Stage-2 verify still owns §"DC Blocker" + §"In-loop saturator" amendments (post-Phase-2.6). Wolf-region deferral is CONTEXT-flagged, not ARCHITECTURE-amended.
- **Wolf-region suppression NOT in v1.0.** Static-Q body bank. Mode #2 Q stays at 11. Wolf-suppression code path — if present in O-Bowed `BodyResonator.cpp` — is excluded from the verbatim-copy port (research-phase identifies + documents the exclusion line range). Reactivation path: Phase 2.5-bis or v1.1 cycle restores wolf-suppression with appropriate Hard Rule for fundamental-lock-detection determinism.
- **Phase 2.4-bis backlog stays parked.** Strict body+noise scope.

**Working-tree starting state (locked from Phase 2.4c-bis verify, R36-bis commit `1044bed41574be5d0714983f7910cac8bda2edec` + R36-bis-backfill chore `1dfca9d`):**

- All Phase 2.4c-bis end-state source verbatim (BowedContrabassVoice, SubHarmonicBias.h, SchellengCalibration.h, WaveguideString.cpp post-port `sat·tanh(x/sat)` at `:204–206`, DispersionFilter, PluginProcessor, MPE synthesizer).
- 13 currently-committed reproduce-goldens.sh entries (12 carry-forward through Phase 2.4b-end-state with Phase 2.4c-bis re-baselined sha256s + 1 saturator-tail-comparison post-port `5c45d176…`).
- `reproduce-goldens.sh` (Phase 2.4a R34-pre infrastructure, extended Phase 2.4b R35 + Phase 2.4c R36 + Phase 2.4c-bis R36-bis).
- `preflight-subharm.sh` (Phase 2.4b R35-pre HR-9 escalation gate).
- `vibrato.json.sha256` (Phase 2.4c-bis post-port carry-forward; Phase 2.5 default expectation: re-baseline).
- `matrix-stability.{wav.sha256, json.sha256, json}` (Phase 2.4a evidence + Phase 2.4c-bis post-port evidence at `09cbf15f…` archived to `.planning/evidence/phase-2-4c-bis/`; NOT in default reproduce-goldens.sh).
- O-Bowed harness Option B extension landed at Phase 2.4c R36b (`canonical-preset.wav.sha256` byte-identical when flags absent).
- Phase 2.4c pre-port reference saturator-tail golden `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb` lives in git history at `115dbf4` (NOT in working tree); reproducible via `git worktree add /tmp/oc-pre-port 115dbf4` for end-of-Stage-2 §"In-loop saturator" amendment evidence base.

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q53 — Phase 2.5 cycle scope shape** | **Single Phase 2.5 cycle** (body + bow noise together; R37 atomic) | User-confirmed (Recommended). Bow noise is conceptually small (3 BPFs + slip bursts driven by existing `bowEnergy`); splitting into 2.5a / 2.5b doubles ceremony for marginal isolation gain. ROADMAP §"Phase 2.5" already bundles them. R37 atomic commit lands both DSP blocks + 13 re-baseline goldens. |
| **Q54 — Body + Bow Noise code source** | **Per-plugin verbatim copy from O-Bowed `Source/DSP/BodyResonator.{h,cpp}` and `BowNoiseGenerator.h`** with bass-tuned mode table + bass-spectral target substitution | User-confirmed (Recommended). Mirrors DispersionFilter / SchellengCalibration / SubHarmonicBias precedent (per-plugin headers, no shared module). ARCHITECTURE §"Body Resonator Click-Free" explicitly recommends "reuse BodyResonator.cpp pattern from O-Bowed (already validated)". Module extraction (`modules/synthesis/body-resonator/`, `modules/synthesis/bow-noise/`) deferred post-v1.0 — O-Bowed has morphable wood + likely violin-targeted bow-noise spectral defaults; reconciliation now would force divergence. |
| **Q55 — Wolf-region suppression in v1.0** | **Skip — ship static-Q body bank** (Mode #2 Q stays at 11; no fundamental-lock detection); deferred to v1.1 alongside Authentic Arco toggle | User-confirmed (Recommended). Fundamental-lock detection introduces runtime nondeterminism (autocorrelator-style risk that bit Phase 2.4c). ARCHITECTURE §"Open Decision §3" already defers Authentic Arco toggle to v1.1; wolf-suppression is the same complexity class. **DEVIATION FROM ARCHITECTURE §"Body Resonator" wolf-region default ON** — flagged explicitly. v1.0 ships static-Q; if wolf-tone audible at G2 sustained during R38 audition or post-Phase 2.6 user testing, schedule Phase 2.5-bis or v1.1 cycle to add wolf-suppression with appropriate Hard Rule. |
| **Q56 — Audible golden re-baseline scope** | **All 13 audible re-baseline + matrix-stability re-render evidence + saturator-tail-comparison re-baseline** | User-confirmed (Recommended). Body resonator + bow noise are post-waveguide spectral colorers active for any audible signal — every audible golden shifts. saturator-tail-comparison re-baselines because body bank is downstream and shifts the post-saturator decay envelope at bin 64. Vibrato carries forward only IF research-phase pre-flight shows `vibrato.wav.sha256` byte-identical post-body (low probability; default = re-baseline). Matrix-stability is evidence-only (not a re-baselined golden), tracking raucous-corner cells. |
| **Q57 — R38 Logic AU smoke timing** | **BLOCKING audition before R37 atomic commit** | User-confirmed (Recommended). Body resonator + bow noise are the most dramatic audible-character changes since Stage-2 open. First-time "convincing orchestral arco bass" validation against BRIEF.md ambition. Mirrors R37-bis Phase 2.4c-bis precedent (audible source-edit → BLOCKING audition). User A/Bs raw-string from `1044bed` (R36-bis post-port) vs body-engaged from working-tree in Logic Pro before R37 lands. Catches subjective issues (wolf-tone audibility, bow-noise level mis-calibration, body coupling distortion) that objective measurements miss. |
| **Q58 — DSP-07 retune absorption** | **Stay parked (Phase 2.4-bis backlog)** | User-confirmed (Recommended). Phase 2.5 = body + noise scope strict. DSP-07 is friction-tuning concern (subharmEnergyRatio above 0.40 strict; currently 0.358 SOFT-PASS post-tanh-port), unrelated to body/noise DSP blocks. Folding it pollutes scope and complicates the saturator-tail-regression-vs-body-coupling evidence base. Phase 2.5-bis or post-v1.0 Phase 2.4-bis cycle. |
| **Q59 — New Hard Rule** | **None — ARCHITECTURE already locks** | User-confirmed (Recommended). HR candidates considered: (a) "body bank runs at host rate, 2x oversampling boundary at friction junction only" — already locked by ARCHITECTURE §"Oversampling Strategy"; (b) "wolf-region suppression deferred to v1.1" — Q55 carries this in CONTEXT, doesn't need HR-12. No new HR over-engineers the cycle. HR-1..HR-10 carry forward; HR-11 retired (Phase 2.4c-only binding preserved in audit history). |
| **Q60 — Atomic commit shape** | **R37 atomic + R37-backfill chore** | User-confirmed (Recommended). Continues atomic-commit sequence: R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → **R37** (Phase 2.5 Gate 7 PASS). R37-backfill chore propagates R37 sha into STATUS.md per R34 / R35 / R36 / R36-bis backfill precedent. Note: R37 was previously reserved for Phase 2.4c Logic AU smoke deferred-non-blocking; reservation retired at 2.4c-bis (R37-bis was BLOCKING). R37 number now available for Phase 2.5. |
| **Q61 — CONTEXT.md rev numbering** | **rev-10** | User-confirmed (Recommended). Pre-locked by rev-9-bis §"Sequencing post-2.4c-bis": Phase 2.5 opens fresh CONTEXT rev-10 (NOT rev-9; rev-9 number skipped intentionally — rev-9-bis preserved escalation-off-rev-8 audit-trail signal). |
| Per-block evaluation order | **Step 1–7 + Step 2.5 unchanged from Phase 2.4c-bis end-state; APPEND Step 8 (body) + Step 9 (bow noise) verbatim from ARCHITECTURE §"Per-block processing order" steps 15–16** | 7-step + Step 2.5 + Step 7-end-state (post-port saturator + downsample) carry-forward verbatim. Phase 2.5 ONLY APPENDS new processing steps after waveguide-output and before (Phase 2.6) master chain. |
| Saturator-tail regression check | **YES — re-measure at execute-phase pre-flight + verify-phase Gate 7** | Carry-forward from rev-8 risk #10 / rev-9-bis Phase 2.5-awareness risk. Body bank is downstream of saturator; coupling characterized via `--saturator-tail-comparison` re-render. Verify §19.3.3 analytic bound (≤ 2 dB at canonical amplitude) holds under post-port + body-coupled topology. NOT a Gate 7 BLOCKER unless > 4 dB shift indicates body-coupling instability. |

---

## Open Questions (handed to research-phase)

1. **Pre-Phase-2.5 repro tripwire.** Render 13 reproduce-goldens.sh entries at HEAD (`1dfca9d` descendant or current main) BEFORE source edits — confirm bit-identical reproduction against committed sha256s. Mirrors R36-bis-pre / R36-pre tripwire pattern. Catches any upstream drift introduced between R36-bis-backfill chore (`1dfca9d`) and Phase 2.5 discuss-phase open.

2. **O-Bowed `Source/DSP/BodyResonator.{h,cpp}` source provenance audit.** Identify exact O-Bowed source path + line range; document: (a) is it `.h` + `.cpp` or `.h`-only? (b) what is the public API (constructor, `prepare()`, `process(juce::AudioBuffer<float>&)`, `setSize/Damping/Mix(float)` setters)? (c) what mode-table data structure is used (`std::array<float, 8>` triplets, struct array, etc.)? (d) is wolf-region suppression code present, and at what line range (so the verbatim copy can EXCLUDE it cleanly)? (e) is `juce::dsp::ProcessSpec` consumed for sample-rate awareness? Source delta size = O-Bowed file LOC + bass-tuned mode table substitution + (if applicable) wolf-suppression-code excision = ≈ 100–250 LOC NEW per file.

3. **O-Bowed `Source/DSP/BowNoiseGenerator.h` source provenance audit.** Identify exact path + LOC; document: (a) public API (constructor, `prepare()`, `processBlock(...)` or per-sample API, `setNoiseLevel(float)` setter, slip-burst trigger accessor); (b) BPF center frequencies (likely 1500 / 3000 / 5000 Hz violin-targeted; bass substitution is 700 / 1500 / 3000 Hz); (c) `juce::Random` seed determinism strategy (constructor-time fixed seed → bit-deterministic across re-renders; runtime random → escalation needed); (d) slip-burst trigger source — does it accept an externally-driven slip-detect signal or compute internally from friction force? (e) `bowEnergy` parameter source — passed in or computed internally?

4. **Slip-burst trigger source integration.** ARCHITECTURE §"Bow Noise Generator" specifies "Per-period slip bursts on Helmholtz slip-detection (zero-crossing of friction force from stick to slip)". Research-phase confirms: (a) does `modules/synthesis/bow-friction/` v1.0.0 expose a per-sample `lastForceState()` accessor that BowNoiseGenerator can sample for zero-crossing detection? OR (b) does BowNoiseGenerator compute slip-detection internally from `(v_b, F_bow, beta)`? OR (c) is slip-detection implemented at voice level and an event/flag is passed to BowNoiseGenerator? Decision affects voice-side wiring complexity at Step 9.

5. **Saturator-tail regression measurement protocol under body coupling.** Re-render `--saturator-tail-comparison` mode post-Phase-2.5; measure bin 64 dB rel max; compare against Phase 2.4c-bis R36-bis baseline (−7.97 dB). Document expected vs measured shift in RESEARCH §20.4. §19.3.3 analytic bound (≤ 2 dB at canonical amplitude) verification: does post-port + body-coupling preserve the bound? NOT a Gate 7 BLOCKER unless > 4 dB shift signals body-coupling instability. If outside [−9, −5] dB rel max → flag as evidence for §"In-loop saturator" amendment + investigate before R37 atomic.

6. **13-audible-golden re-baseline strategy.** All 13 affected goldens get fresh sha256s simultaneously. Research-phase pre-flights 3 back-to-back renders per golden to confirm bit-deterministic across re-renders. Predicted sha256s NOT committed to PLAN rev-12 (would require source change to measure); plan-phase pin: "Predicted sha256s pre-flighted at execute-phase scratch space; landed values become R37 goldens."

7. **`juce::Random` seed determinism in BowNoiseGenerator.** Bit-deterministic-across-re-renders is the regression bar. Research-phase confirms seed is constructor-time fixed (not runtime random); if O-Bowed implementation uses runtime seed, Phase 2.5 substitutes constructor-time deterministic seed (e.g., `juce::Random{42}` or voice-index-derived seed). Document determinism strategy in RESEARCH §20.2.

8. **Vibrato golden carry-forward vs re-baseline determination.** Body resonator is downstream of vibrato modulator path (vibrato modulates BOW_SPEED at Step 4; body bank at NEW Step 8). Question: does body bank's wet/dry mix at default 80% wet shift the autocorrelator peak metric (`peakDepthCents=9.526`)? Default expectation: re-baseline (body bank colors output spectrum). Research-phase confirms: re-render `--vibrato` mode post-Phase-2.5; if metrics reproduce within ±0.05¢ / ±0.005 Hz / ±2 ms AND `vibrato.wav.sha256` byte-identical → carry forward. If shift → re-baseline (already counted in 13 audible re-baselines above).

9. **Matrix-stability post-Phase-2.5 re-render verdict structure.** Re-render is evidence-only (NOT committed as a re-baselined golden). Document in RESEARCH §20.6 whether: (a) raucous-corner cells stabilise (3 → fewer fails; body bank's wet/dry coloring may smooth raucous transients); (b) raucous-corner cells hold (3 fails unchanged); (c) NEW raucous corners surface (regression risk; investigate before R37 atomic — biquads are L2-bounded so this is low-probability). If (c), block plan-phase pending root-cause.

10. **Sub-harmonics + sub-harmonics-stability post-body coupling.** `subharmEnergyRatio` Phase 2.4c-bis landed value `0.358` SOFT-PASS at waveguide output (pre-body). Body bank is a parallel bandpass at host rate; `BODY_MIX = 0.80` default routes 80% through wet path. Question: does body bank's lowpass-tilted spectral coloring preserve subharmonic energy ratio? Research-phase pre-flights — measure post-body `subharmEnergyRatio` in `--sub-harmonics` mode. Document landed value in RESEARCH §20.7. Evidence-only; not a Gate 7 BLOCKER. If drops below 0.30 → flag as evidence for Phase 2.4-bis backlog priority bump.

11. **R37 task breakdown.** Initial estimate: R37-pre tripwire (13-entry reproduce-goldens.sh PASS at HEAD + O-Bowed source provenance audit + `juce::Random` seed determinism check) + R37a (BodyResonator.{h,cpp} verbatim copy + bass-tuned mode table substitution + wolf-suppression-code excision) + R37b (BowNoiseGenerator.h verbatim copy + bass-spectral-target substitution + deterministic-seed substitution) + R37c (BowedContrabassVoice integration — Step 8 + Step 9 + body/noise instance ownership + smoothed param ramps + slip-burst trigger wiring) + R37d (re-baseline 13 audible goldens + saturator-tail-comparison re-baseline + matrix-stability re-render evidence + sub-harmonics post-body measurement) + R37e (regression bar via 13-entry `reproduce-goldens.sh` against new sha256s + 4-file source audit hook) + R37f (auval + pluginval-10) + R38 Logic AU audition (BLOCKING) + R37 atomic commit + R37-backfill chore. Research-phase locks task body and ordering.

12. **R38 audition protocol.** Pre-Phase-2.5 reference: render raw-string state from `1044bed` (R36-bis post-port) checkout via `git worktree add /tmp/oc-pre-2-5 1044bed` + build O-Contrabass-pre-2-5 plugin variant. Post-Phase-2.5 reference: render body-engaged from working-tree O-Contrabass-dev. Audition: A/B sustained E1 + per-string MIDI 33/38/43 + tail-decay character at MIDI lift + bow-noise audibility at low pressure + sustained G2 (wolf-region check) + orchestral-character A/B vs reference orchestral library bass. PASS criteria: post-Phase-2.5 sounds "convincing orchestral arco bass" per BRIEF.md DSP-03 + DSP-04 — wood body resonance present, bow-noise texture audible at low pressure without overwhelming the tone, no unexpected character changes. If post-body reveals wolf-tone audibility at G2 sustained → DOCUMENT (carry to v1.1 Phase 2.5-bis backlog), do NOT block R37 atomic. If post-body reveals body-coupling distortion or transient artifacts → ESCALATE before R37 atomic.

13. **RESEARCH §20 deliverable structure.** §20 Phase 2.5 verdict subsection. 11 sub-sections per "In Scope" §20.1–§20.11 list above. Independent of §19.7.6 / §19.7.7 (Phase 2.4c / 2.4c-bis verdicts); §20 closes the loop on Phase 2.5 (DSP-03 + DSP-04 closure or partial closure with backlog).

14. **CMakeLists.txt source-list update.** Research-phase confirms whether `Source/DSP/BodyResonator.cpp` exists in O-Bowed (vs `.h`-only). If `.cpp` present, plan-phase locks CMakeLists.txt source-list addition. If `.h`-only, no CMake update needed. `Source/DSP/BowNoiseGenerator.h` is `.h`-only per O-Bowed convention (no CMake update).

---

## Risks (Phase 2.5-specific)

1. **Body bank coefficient instability at low frequencies (E1 = 41 Hz region).** ARCHITECTURE §"Body Resonator (Parallel Biquad Bank)" specifies `fc[i] = jlimit(20, sr · 0.45, defaultFreq[i] / size_scalar)` — Mode 1 (A0, 60 Hz default) at `BODY_SIZE = 1.0` shifts to `60 / 1.15 = 52 Hz`; at 44.1 kHz host rate, biquad coefficient stability at 52 Hz with Q=14 is tight (pole near unit circle). Mitigation: ARCHITECTURE-spec'd `jlimit(20, ...)` floor + research-phase pre-flight numerical-stability check + 30 ms `SmoothedValue` on Size prevents instantaneous coefficient swap.

2. **Click-free coefficient updates fail.** Naïve coefficient swap in biquad at block boundary causes filter-state inconsistency → audible click. Mitigation: per-block recompute + 30 ms `SmoothedValue` (ARCHITECTURE §"Body Resonator Click-Free Coefficient Updates" Recommendation 1, validated in O-Bowed). Research-phase pre-flight tests BODY_SIZE / DAMPING / MIX sweep at execute-phase scratch — listen for clicks during ramp.

3. **`juce::Random`-driven bow noise fails bit-deterministic-across-re-renders bar.** If O-Bowed implementation uses runtime random seed (e.g., `juce::Random::getSystemRandom()`), each render produces different bow-noise sample sequence → sha256 nondeterminism. Mitigation: research-phase confirms seed strategy; substitute constructor-time fixed seed (e.g., `juce::Random{42}` or voice-index-derived) in Phase 2.5 BowNoiseGenerator. If escalation needed (e.g., O-Bowed implementation deeply tangled with runtime random), source delta grows beyond verbatim-copy.

4. **Wolf-region resonance audible at G2 sustained without suppression.** Q55 deferral ships static-Q body bank; G2 is in the wolf region (F2–B2 per ARCHITECTURE). Mode #2 Q=11 at `BODY_DAMPING = 0.40` default → `Q_eff = 11 · (1 − 0.85·0.40) = 11 · 0.66 = 7.26` — moderate Q at 98 Hz center; G2 fundamental = 98 Hz exactly! → potential beating. Mitigation: R38 audition explicitly tests G2 sustained; if audible beating → DOCUMENT (Phase 2.5-bis backlog), do NOT block R37 atomic. Worst-case workaround: user can mitigate via `BODY_DAMPING` increase to 0.60+. If Phase 2.5-bis is needed mid-cycle, escalation lane available.

5. **Body coupling shifts saturator-tail bin 64 outside soft-band [−8.17, −6.17] dB.** Body bank is a linear filter (no nonlinear backflow to saturator), but `BODY_MIX = 0.80` default routes 80% through bandpass-filtered path → spectral re-shaping of decay envelope. Mitigation: research-phase pre-flight measures post-body bin 64; documents shift; verify §19.3.3 analytic bound. If outside [−9, −5] dB rel max → flag for §"In-loop saturator" amendment evidence base + investigate before R37 atomic. NOT a Gate 7 BLOCKER unless > 4 dB shift indicates body-coupling instability (low-probability).

6. **13-audible-golden re-baseline drift across runs.** Body bank coefficient float-precision differences across runs (unlikely on M1 Xcode 26.3 — biquad coefficient compute is deterministic) OR `juce::Random` seed nondeterminism (Risk #3). Mitigation: research-phase pre-flights 3 back-to-back renders per golden + verifies sha256 bit-stability. If non-deterministic on `juce::Random`, fix at substitution (Risk #3 mitigation). If non-deterministic on biquad coefficients, escalate (likely a `prepare()` ordering issue — coefficients computed at non-deterministic call site).

7. **Vibrato golden re-baseline (Open Q8).** Default expectation: re-baseline (already counted in 13). Risk: if vibrato `peakDepthCents` shifts by > 0.10¢ post-body, perceptual change is potentially audible (vibrato is sensitive to envelope coloring). Mitigation: R38 audition explicitly A/Bs vibrato character pre-vs-post body; if perceptual shift > "subtle," escalate before R37 atomic.

8. **Matrix-stability post-Phase-2.5 reveals NEW raucous corners** (Open Q9 case (c)). Body bank is L2-bounded linear filter — extremely low-probability. If surfaces, root-cause investigation blocks plan-phase. Most likely culprit: `BODY_MIX = 0.0` → 100% dry path through 35 Hz HP could amplify low-frequency residual (counter-intuitive — HP attenuates below 35 Hz, so this is true low-probability).

9. **Sub-harmonics post-body coupling drops `subharmEnergyRatio` significantly** (Open Q10). Body bank is bandpass-tilted at low frequencies (Modes 1–3 below 200 Hz with high gain) — actually BIASES toward subharmonic preservation (subharmonics live at f_0/2 = 20.5 Hz for E1; HP35 dry path attenuates this BUT body bank's Mode 1 at 60 Hz amplifies harmonics ≥ first sub-octave). Net effect: likely PRESERVES or BOOSTS `subharmEnergyRatio` at body output. Mitigation: research-phase measures + documents. Likely benign within 0.30–0.45 range. If drops below 0.30 → Phase 2.4-bis backlog priority bump (NOT blocking 2.5 closure).

10. **R38 BLOCKING audition reveals subjective issue.** Mitigation: Phase 2.5 stays open (atomic-commit deferred) until subjective issues resolved. Resolution paths: (a) bow-noise level recalibration (BPF Q tweak, default `BOW_NOISE` reduction from 0.35); (b) body bank gain attenuation (Modes 1–3 gain dB shift); (c) bass-spectral target re-confirmation (research-phase consults Askenfelt 1982 / Rossing 2010 for mode magnitudes); (d) escalate to Phase 2.5-bis (e.g., wolf-suppression early implementation) if structural; (e) revert R37 source edits and close 2.5 as research-only acknowledged-divergence (very low-probability — body+noise is well-precedented in O-Bowed).

11. **Audit-hook drift mid-cycle.** R37-pre + R37-e audit hooks (`git diff --stat HEAD -- plugins/O-Contrabass/Source/`) MUST report exactly the EXACT 4-file set (Source/DSP/BodyResonator.{h,cpp} NEW; Source/DSP/BowNoiseGenerator.h NEW; Source/BowedContrabassVoice.{h,cpp} M; Source/PluginProcessor.{h,cpp} M) at R37-pre + R37-e tripwires (NEW additions count as modifications per `--stat`). ANY other file modification = scope-expansion HARD violation. Mitigation: tripwire pattern.

12. **`juce::dsp::IIR::Filter<float>` RT-safety concern.** JUCE biquad `process()` calls `processSample()` which is RT-safe (no allocations, no system calls); coefficient update is `setCoefficients(Coefficients)` which atomically swaps internal coefficient struct. Mitigation: pluginval-10 fuzz + Parameter thread safety re-confirm; `juce::ScopedNoDenormals` already in place. Coefficients are recomputed in audio thread (no thread-boundary).

13. **Phase 2.6-awareness.** Master saturator + zero-latency limiter (Phase 2.6) is the FINAL nonlinear stage; body bank's wet output goes through it. Subjective bar at Phase 2.5 R38 audition is "convincing orchestral arco" WITHOUT master chain — Phase 2.5 voice output goes directly to bus (host-rate, no master saturator/limiter). User audition character at Phase 2.5 will differ from final v1.0 character (post-Phase-2.6). Mitigation: R38 audition acknowledges this explicitly — body+noise character validation, NOT final-output validation. Phase 2.6 verify includes orchestral-character re-validation under master chain.

14. **Body Mix at BODY_MIX = 0.0 (100% dry) audible mismatch.** ARCHITECTURE specifies dry path = 35 Hz HP-filtered raw waveguide output. At E1 = 41 Hz, 35 Hz HP attenuates ~−3 dB at fundamental. User expectation might be "0% body = unfiltered raw string" — actual behavior is "0% body = HP-filtered raw string." Mitigation: document in RESEARCH §20.3 + flag at R38 audition. ARCHITECTURE-correctly: HP35 is in dry path to prevent sub-A0 phase-comb artifacts during mix; this is intentional. If user audition reveals subjective issue, schedule Phase 2.5-bis with HP cutoff lowered (e.g., 20 Hz) or HP made conditional on `BODY_MIX > 0`.

15. **Phase 2.5 verify regression check on Phase 2.4c-bis Q47 SOFT-PASS contract.** Q47 contract: post-port saturator-tail bin 64 ∈ [−8.17, −6.17] dB rel max (soft-PASS) at canonical operating point. Phase 2.5 body coupling MAY shift this. Documented as evidence-only at Phase 2.5 (carry-forward from rev-8 risk #10); if shift > 1 dB → flag in RESEARCH §20.4 + carry-forward to end-of-Stage-2 §"In-loop saturator" amendment evidence base.

---

## Gate 7 Five-Item Success Criteria (preliminary; PLAN rev-12 locks)

1. **Bit-deterministic regression bar** — all 13 reproduce-goldens.sh entries reproduce byte-identical via post-Phase-2.5 sha256s. 13 audible goldens get NEW reference sha256s. saturator-tail-comparison re-baselines with NEW post-Phase-2.5 sha256. Audit hook reports exactly 4 file changes (Source/DSP/BodyResonator.{h,cpp} NEW; Source/DSP/BowNoiseGenerator.h NEW; Source/BowedContrabassVoice.{h,cpp} M; Source/PluginProcessor.{h,cpp} M) at R37-pre + R37-e tripwires.
2. **DSP-03 + DSP-04 acceptance** — body bank impulse response shows 8 spectral peaks at 60/98/115/175/235/340/700/1200 Hz with correct relative gains; BODY_SIZE / DAMPING / MIX sweeps zipper-free; BOW_NOISE 0%→100% sweep produces audible noise level rising to overwhelming, fades to silence at 0; bow direction reversal produces 5–15 ms wideband noise burst (slip-burst validation). Wolf-region (G2 sustained) audibility documented (NOT blocking — Q55 deferral).
3. **`auval` + `pluginval-10` PASS** — auval AU VALIDATION SUCCEEDED full render-rate matrix; pluginval --strictness-level 10 SUCCESS full battery (Editor Automation, Automatable Parameters, Parameter thread safety, Background thread state, Bus enable/disable, Restoring default layout, Fuzz parameters all complete).
4. **R38 Logic AU audition** — user-confirmed audition CONFIRMS post-Phase-2.5 character is "convincing orchestral arco bass" per BRIEF.md DSP-03 + DSP-04 acceptance bar (BLOCKING — R37 atomic does NOT land until R38 audition CONFIRMED). Pre-Phase-2.5 reference (raw-string from `1044bed` worktree) vs post-Phase-2.5 (body-engaged from working-tree) A/B in Logic Pro AU.
5. **RESEARCH §20 verdict locked** — Phase 2.5 verdict written: WORKED (DSP-03 + DSP-04 CLOSED, Phase 2.6 unblocked) OR WORKED-PARTIALLY (Phase 2.5-bis backlog logged for wolf-suppression / bow-noise recalibration / etc.) OR REGRESSION (body coupling reveals saturator-tail divergence > 4 dB OR matrix-stability NEW raucous corners; Phase 2.5-bis escalation flag LOCKED before R37 atomic).

---

## Next Phase

Ready for: **research** phase — `/clear` then `/plugin-research O-Contrabass 2-dsp`

Research focus (Phase 2.5):

1. **Resolve Open Questions #1–#14** — pre-Phase-2.5 repro tripwire, O-Bowed BodyResonator + BowNoiseGenerator source provenance, slip-burst trigger source, saturator-tail regression measurement protocol, 13-audible-golden re-baseline strategy, `juce::Random` seed determinism, vibrato carry-forward determination, matrix-stability post-Phase-2.5 verdict structure, sub-harmonics post-body coupling, R37 task breakdown, R38 audition protocol, RESEARCH §20 deliverable structure, CMakeLists.txt source-list update.
2. **Pre-Phase-2.5 repro tripwire** — render all 13 reproduce-goldens.sh entries at HEAD; verify byte-identical against committed sha256s. If any drift, INVESTIGATE before plan-phase.
3. **O-Bowed source provenance audit** — read `plugins/O-Bowed/Source/DSP/BodyResonator.{h,cpp}` + `BowNoiseGenerator.h`; document API, mode-table data structure, wolf-suppression code presence + line range (for excision), bass-spectral-target deltas, `juce::Random` seed strategy. Lock verbatim-copy LOC budget per file in RESEARCH §20.1 / §20.2.
4. **Slip-burst trigger source** — confirm whether `modules/synthesis/bow-friction/` v1.0.0 exposes a per-sample `lastForceState()` accessor, OR slip-detection is implemented inside BowNoiseGenerator from `(v_b, F_bow, beta)`, OR slip-detect signal must be plumbed at voice level. Document integration site in RESEARCH §20.3.
5. **Saturator-tail regression measurement** — apply Phase 2.5 in research-phase scratch (NOT committed); render `--saturator-tail-comparison`; measure bin 64 dB; compare against Phase 2.4c-bis baseline (−7.97 dB). Document in RESEARCH §20.4. Verify §19.3.3 analytic bound preservation. If outside [−9, −5] dB → flag for amendment evidence base.
6. **`juce::Random` seed determinism check** — pre-flight 3 back-to-back renders of `--canonical-preset` mode with bow-noise active; verify sha256 stability. If non-deterministic, lock substitution to constructor-time fixed seed in plan-phase.
7. **Body-coefficient stability at low-frequency edge cases** — pre-flight BODY_SIZE = 1.0 + BODY_DAMPING = 0.0 (Mode 1 at 52 Hz, Q ≈ 14, sr = 44.1 kHz) for numerical stability; pre-flight BODY_SIZE = 0.0 + BODY_DAMPING = 1.0 (Mode 8 at 1412 Hz, Q ≈ 0.375 minimum) for filter degeneracy.
8. **Vibrato carry-forward pre-flight** — render `--vibrato` mode against post-Phase-2.5 WAV; verify autocorrelator metrics reproduce within tolerance; confirm `vibrato.wav.sha256` byte-identical OR document re-baseline.
9. **Matrix-stability post-Phase-2.5 measurement** — re-render 108-combo matrix; document raucous-corner cells stable / unchanged / new.
10. **Sub-harmonics post-body coupling measurement** — render `--sub-harmonics` + `--sub-harmonics-stability`; document landed `subharmEnergyRatio` at body output.
11. **R38 audition protocol pre-write** — document Logic AU smoke audition steps for execute-phase user reference (incl. `git worktree add /tmp/oc-pre-2-5 1044bed` checkout protocol for pre-Phase-2.5 reference render + O-Contrabass-pre-2-5 plugin variant build).
12. **Append RESEARCH §20** — document all resolutions above. (No §19.7.7 changes; rev-9-bis verdict locked.)

After research: plan-phase (PLAN rev-12) writes R37 task breakdown verbatim against this CONTEXT + research findings; execute-phase performs implementation + 13 re-baselined goldens + R37 atomic commit (BLOCKED on R38 Logic AU audition); verify-phase confirms Gate 7 invariants + flips DSP-03 / DSP-04 status in VERIFICATION.md.

---

## Audit Trail (rev-10 supersedes rev-9-bis)

**rev-1 (2026-04-26):** Phase 2.1 broad discuss. Cycle scope = Phase 2.1 (sub-phases a/b/c).
**rev-2 (2026-04-26):** Phase 2.1a closure (Option A, R7) + Phase 2.1b opening (module extraction, Gate 2). Phase 2.1b verified 2026-04-27 (R8a `bd5fae0` + R15 `ef0604d`, Gate 2 PASS).
**rev-3 (2026-04-27):** Phase 2.1c opening — cascaded allpass dispersion. Verified 2026-04-27 (R20 `5759e5e`, Gate 3 PASS).
**rev-4 (2026-04-27):** Phase 2.2 opening — 4-string EADG + per-string detune + per-string M-table. Verified 2026-04-27 (R26 `131c2c7`, Gate 4 PASS).
**rev-5 (2026-04-27):** Phase 2.3 opening — Vibrato + Slow-Bow LFO + Schelleng wedge clamp + EXPRESSION_MACRO. HR-1..HR-4 binding. Verified 2026-04-27 (R33 `af54571`, Gate 5 PASS with rebaseline of 4 audible carry-forward goldens).
**rev-6 (2026-04-27):** Phase 2.4a opening — Schelleng wedge bass-register calibration polynomial + 108-combo stability matrix dual-purpose render + `pass_breathingAudible` 5%→20% threshold restoration. HR-5..HR-8 binding. Verified 2026-04-28 (R34 `4c926bb`, Gate 6a CLEARED — 3 strict-PASS + 2 soft-PASS within v1.0 budgets) + R34-backfill chore `b64c8c4`.
**rev-7 (2026-04-28):** Phase 2.4b opening — Sub-Harmonic Bias DSP-07 (ARCHITECTURE §457). HR-9 + HR-10 binding. Verified 2026-04-28 (R35 `3de8b66`, Gate 6b CLEARED — 4 strict-PASS + 1 soft-PASS within RESEARCH §18.6 v1.0 budget) + R35-backfill chore `0db5fac`.
**rev-8 (2026-04-28):** Phase 2.4c opening — autocorrelator octave-rejection harness fix + saturator-tail O-Bowed comparison (research-only). HR-11 binding (zero production DSP edits). Verified 2026-04-29 (R36 `115dbf4`, Gate 6c CLEARED via escalation lane — 5/5 invariants; §19.7.6 escalation flag LOCKED on measured 5.92 dB envelope divergence > 2 dB Q41 threshold + ~3 dB perceptual JND) + R36-backfill chore `7835904`.
**rev-9-bis (2026-04-29):** Phase 2.4c-bis opening — source-change in-loop saturator port from O-Bowed (`sat · tanh(x / sat)` with `sat = 4.0f`) to O-Contrabass `Source/DSP/WaveguideString.cpp:204–206` replacing algebraic `x / sqrt(1 + x²)` on both rails. 7 approach decisions Q46–Q52. HR-11 retired. Verified 2026-04-29 (R36-bis `1044bed`, Gate 6c-bis SOFT-PASS at bin 64 = −7.97 dB / `|Δ| = 0.80 dB` vs O-Bowed reference; 5/5 invariants cleared; HR-1..HR-10 preserved) + R36-bis-backfill chore `1dfca9d`.

**rev-10 (this document, 2026-04-30):** Phase 2.5 opening — body resonator (8-mode static-Q parallel biquad bank, bass-tuned modes per Askenfelt 1982 / Rossing 2010) + bow noise generator (3-band BPF at 700/1500/3000 Hz + per-period slip bursts, decay 0.999) as NEW Steps 8 + 9 in per-block evaluation order, AFTER waveguide downsample, BEFORE (Phase 2.6) master chain. Closes BRIEF.md DSP-03 (must) + DSP-04 (should). **9 approach decisions Q53–Q61 user-confirmed**: single Phase 2.5 cycle (Q53); per-plugin verbatim copy from O-Bowed `Source/DSP/BodyResonator.{h,cpp}` + `BowNoiseGenerator.h` (Q54); wolf-region suppression deferred to v1.1 — static-Q body bank in v1.0 (Q55); 13 audible goldens re-baseline + matrix-stability evidence + saturator-tail-comparison re-baseline (Q56); R38 BLOCKING Logic AU audition before R37 atomic commit (Q57); DSP-07 retune stays parked (Q58); no new Hard Rule — HR-1..HR-10 carry-forward, HR-11 retired (Q59); R37 atomic + R37-backfill chore (Q60); CONTEXT rev-10 numbering convention (Q61). **14 open questions handed to research-phase**: pre-Phase-2.5 repro tripwire, O-Bowed source provenance audit (BodyResonator + BowNoiseGenerator), slip-burst trigger source, saturator-tail regression measurement, 13-audible-golden re-baseline strategy, `juce::Random` seed determinism, vibrato carry-forward determination, matrix-stability post-Phase-2.5 verdict structure, sub-harmonics post-body coupling, R37 task breakdown, R38 audition protocol, RESEARCH §20 deliverable structure, CMakeLists.txt source-list update, body-coefficient stability at low-frequency edge cases. **DEVIATION FROM ARCHITECTURE §"Body Resonator"**: wolf-region default-ON suppression deferred to v1.1 (Q55) — runtime fundamental-lock-detection nondeterminism risk. **NO new HR introduced.** Phase 2.6 (master saturator + limiter + width + microtonal + MPE + Note Expression) gets fresh CONTEXT rev-11 post-Phase-2.5 verify. Continues atomic-commit sequence R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → **R37**.

**Inherited verbatim from rev-9-bis (not re-litigated):**

- All Phase 2.3 modulator surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order)
- All Phase 2.4b end-state (Step 2.5 sub-harmonic bias evaluation between Step 2 and Step 3; subHarmonicsSmoothed 30 ms ramp; lastSubAmount instrumentation; voiceBowForceUpliftThisBlock factor at Step 6)
- All Phase 2.4c-bis end-state (`Source/DSP/WaveguideString.cpp:204–206` post-port `sat · tanh(x / sat)` saturator with `sat = 4.0f` on both rails)
- HR-1..HR-4 (Phase 2.3 literal-zero short-circuits + IEEE 754 identity-arithmetic + Schelleng skip on zero LFO depth)
- HR-5..HR-8 (Phase 2.4a inline constexpr linkage on SchellengCalibration.h + calibration behind HR-4 gate ONLY + matrix-stability bypass via weak-symbol + trilinear IEEE 754 identity arithmetic)
- HR-9..HR-10 (Phase 2.4b SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate + friction module ABI preservation via ROSIN inverse algebraic identity)
- HR-11 RETIRED (Phase 2.4c-only binding preserved in audit history)
- 13 currently-committed reproduce-goldens.sh entries (12 carry-forward through Phase 2.4b + 1 saturator-tail-comparison Phase 2.4c-bis post-port `5c45d176…`) + matrix-stability evidence golden + Phase 2.4c pre-port reference saturator-tail golden `c7e845ea…` in git history at `115dbf4`
- O-Bowed harness Option B extension (canonical-preset.wav.sha256 byte-identical when flags absent)
- Atomic-commit gate-first principle (R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37)
- Phase 2.4-bis backlog parking (kForceBoost retune; Step 4 modulation gain / breathingAudible metric refinement; 3 fallback-cell reduction; VIBRATO_DEPTH→peakDepthCents transfer tune)
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify (post-Phase-2.6); Phase 2.4c pre-port reference + Phase 2.4c-bis post-port + Phase 2.5 body-coupled saturator-tail measurements feed §"In-loop saturator" amendment evidence base
- E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7) — separate concern; v1.1 or post-v1.0
- Primary listening DAW: Logic Pro (AU)
- Sample-rate strategy: internal 88.2 / 96 kHz at friction junction; body bank + bow noise at host rate (NOT oversampled per ARCHITECTURE §"Oversampling Strategy")
- Bow-friction module v1.0.0 at `modules/synthesis/bow-friction/`
- Per-plugin `DispersionFilter.h` + `SchellengCalibration.h` + `SubHarmonicBias.h` (NOT extracted to shared module); Phase 2.5 adds per-plugin `BodyResonator.{h,cpp}` + `BowNoiseGenerator.h` to this convention
- 29 APVTS parameters; parameter-spec.md sha256 `77638e25…` carries forward unchanged (5 body/noise params already declared)
- Stage-1 contract NOT amended in Phase 2.5
- ARCHITECTURE.md NOT amended in Phase 2.5
- Chaos detector + softClampState deferred to v1.1 (was "Phase 2.5/2.6" in rev-9-bis carry-forward; explicitly v1.1 now since Phase 2.5 stays scope-strict)

**New in rev-10:**

- Q53 Phase 2.5 cycle scope shape = single cycle (body + bow noise together; R37 atomic)
- Q54 Body + Bow Noise code source = per-plugin verbatim copy from O-Bowed with bass-target substitution
- Q55 Wolf-region suppression deferred to v1.1; static-Q body bank in v1.0 (DEVIATION FROM ARCHITECTURE §"Body Resonator" wolf-region default-ON; flagged explicitly)
- Q56 13 audible re-baseline + matrix evidence + saturator-tail-comparison re-baseline
- Q57 R38 BLOCKING Logic AU audition before R37 atomic commit
- Q58 DSP-07 retune stays parked (Phase 2.4-bis backlog)
- Q59 No new Hard Rule introduced; HR-1..HR-10 carry-forward; HR-11 retired
- Q60 R37 atomic + R37-backfill chore
- Q61 CONTEXT rev-10 numbering convention
- 4 NEW production source files in scope: `Source/DSP/BodyResonator.{h,cpp}` (NEW), `Source/DSP/BowNoiseGenerator.h` (NEW); 2 modified files: `Source/BowedContrabassVoice.{h,cpp}` (M; append Step 8/9 + body/noise instances), `Source/PluginProcessor.{h,cpp}` (M; voice-side body/noise param routing)
- NEW Step 8 (body bank + dry HP path + wet/dry mix) + NEW Step 9 (bow noise sum) appended to per-block evaluation order, verbatim from ARCHITECTURE §"Per-block processing order" steps 15–16
- 13 audible goldens re-baseline (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + macro-sweep + slow-lfo + schelleng-stress + sub-harmonics + sub-harmonics-stability + vibrato + saturator-tail-comparison + canonical-preset)
- Matrix-stability re-render evidence-only (NOT committed as re-baselined golden)
- RESEARCH §20 NEW Phase 2.5 verdict subsection (11 sub-sections)
- Five-item Gate 7 bar: (1) bit-deterministic 13-entry reproduce-goldens.sh + 4-file source audit hook; (2) DSP-03 + DSP-04 acceptance (impulse response + sweeps + slip-burst validation); (3) auval + pluginval-10 SUCCESS; (4) R38 Logic AU audition CONFIRMS post-Phase-2.5 character (BLOCKING); (5) RESEARCH §20 verdict locked.
- Saturator-tail regression measurement at execute-phase pre-flight + verify-phase Gate 7 (carry-forward from rev-8 risk #10 / rev-9-bis Phase 2.5-awareness)
- Body bank low-frequency coefficient stability check (Mode 1 at 52 Hz worst case, Q=14)
- `juce::Random` seed determinism check (constructor-time fixed seed for bit-deterministic re-renders)
- Slip-burst trigger source identification (from bow-friction module accessor or internal compute)
- Wolf-region deferral DEVIATION explicitly flagged in CONTEXT (NOT an ARCHITECTURE amendment; intent preserved for v1.1 reactivation)
