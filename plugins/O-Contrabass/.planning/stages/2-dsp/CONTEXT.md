# Stage 2: DSP — Context (rev-9-bis)

**Date:** 2026-04-29
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.4c-bis — Source-change in-loop saturator port (`x / sqrt(1 + x²)` → `sat · tanh(x / sat)` with `sat = 4.0f`)**
**Supersedes:** rev-8 (Phase 2.4c — autocorrelator octave-rejection harness fix + saturator-tail O-Bowed comparison research-only, dated 2026-04-28). rev-8 contracts that remain locked are inherited verbatim and not re-litigated. Phase 2.4c closed 2026-04-29 with R36 atomic commit (`115dbf4`, Gate 6c CLEARED via escalation lane — 5/5 invariants; §19.7.6 escalation flag LOCKED on measured **5.92 dB** envelope divergence at the 5-s post-bow-off mark > 2 dB Q41 threshold) + R36-backfill chore (`7835904`).

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle activates **Phase 2.4c-bis** — the source-change escalation lane pre-written at PLAN rev-10 §"Contingency — Phase 2.4c-bis Escalation Lane". Triggered by the Phase 2.4c R36d measured saturator-tail divergence: O-Contrabass `−13.0948 dB rel max` vs O-Bowed `−7.17 dB rel max` at bin 64 (5-s post-bow-off mark) → **|Δ| = 5.92 dB** (≫ 2 dB Q41 threshold; approaches/exceeds ~3 dB perceptual JND for sustained tones). The §19.3.3 analytic prediction (≤ 2 dB at canonical bow operating amplitude) was invalidated by cumulative energy-dissipation rate over the 4-s release window magnifying small per-cycle saturator-curvature differences.

Phase 2.4c-bis lifts HR-11 (zero production DSP edits) so the in-loop saturator can be ported from O-Bowed's `sat · tanh(x / sat)` topology to O-Contrabass's `Source/DSP/WaveguideString.cpp:204–206` (replacing the algebraic `x / sqrt(1 + x²)` on both rails). All other locks carry forward verbatim (HR-1..HR-10; 29 APVTS parameters; ARCHITECTURE.md; ROADMAP; bow-friction module v1.0.0; per-block evaluation order). 9 audible goldens re-baseline because the saturator participates in steady-state energy balance for any sustained excitation. Vibrato carries forward conditionally (saturator port is upstream of vibrato modulator path — modulation runs at Step 4, saturator at Step 7; research-phase pre-flight confirms vibrato.wav.sha256 byte-identical OR re-baselines). Matrix-stability re-render is evidence-only (not committed as a re-baselined golden; informs end-of-Stage-2 §"In-loop saturator" amendment).

Convergence target: post-port bin 64 ∈ **[−7.67, −6.67] dB rel max** — tight ±0.5 dB parity with the O-Bowed reference (−7.17 dB). This is sub-JND headroom; pre-port was 5.92 dB divergent, so the win is parity, not just sub-perceptual.

This is the **first audible source-edit since Phase 2.4b R35** (and that was minimal sub-harmonic bias). R37-bis Logic AU smoke gates the R36-bis atomic commit (BLOCKING audition of pre-port `c7e845ea…` reference vs post-port render in Logic Pro before commit lands), departing from the R37/R32/R27/R19f/R14e/R34h/R35 deferred-non-blocking precedent. After Phase 2.4c-bis verifies (Gate 6c-bis PASS), Phase 2.5 (body resonator + bow noise) opens with fresh CONTEXT **rev-10** (NOT rev-9; rev-9 number is skipped per skeleton sequencing — rev-9-bis preserves the "escalation off rev-8" audit-trail signal).

---

## Cycle Scope

**Goal:** Replace the in-loop algebraic saturator (`x / sqrt(1 + x²)`) with O-Bowed's hyperbolic-tangent topology (`sat · tanh(x / sat)` with `sat = 4.0f`) on **both rails** of `Source/DSP/WaveguideString.cpp:204–206` (Step 7, pre-pushSample), so the post-port saturator-tail decay envelope at the canonical bass operating point converges to within ±0.5 dB of the O-Bowed reference at bin 64 (target: [−7.67, −6.67] dB rel max; pre-port was −13.0948 dB rel max). Re-baseline all 9 audible goldens because the saturator participates in steady-state energy balance for sustained excitation. Single R36-bis atomic commit lands the 4-LOC source edit + 9 re-baselined audible goldens + new post-port saturator-tail-comparison golden + RESEARCH §19.7.7 verdict subsection + STATUS / SUMMARY / VERIFICATION / CONTEXT updates. R36-bis-backfill chore propagates R36-bis sha into STATUS.md per R34/R35/R36 backfill precedent.

**In scope:**

- **`plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:204–206`** — replace algebraic saturator with hyperbolic-tangent. New code:
  ```cpp
  // Step 7: In-loop hyperbolic-tangent saturator on each rail (post-port; mirrors O-Bowed sat=4.0f topology).
  constexpr float sat = 4.0f;
  toBridge = sat * std::tanh (toBridge / sat);
  toNeck   = sat * std::tanh (toNeck   / sat);
  ```
  Net delta: ≈ −2 / +4 LOC. Both rails get identical port (no asymmetric variants). NO other DSP source touched. Audit hook: `git diff --stat HEAD -- plugins/O-Contrabass/Source/` MUST report exactly **1 file** modified at R36-bis pre-flight.

- **`plugins/O-Contrabass/tests/render-harness/golden/{stiffness-zero-pre, string-A, string-D, string-G, detune-sweep-A, note-sequence, macro-sweep, slow-lfo, schelleng-stress, sub-harmonics, sub-harmonics-stability}.{wav.sha256, json, json.sha256}`** — **9 audible goldens** get fresh sha256s (saturator participates in steady-state energy balance). Note: `stiffness-zero-pre` = E1 strict golden. Sub-harmonics + sub-harmonics-stability re-baseline because subharmonic energy ratio depends on saturator curvature. Predicted sha256s NOT pre-computed at CONTEXT (would require source change to measure); execute-phase scratch-space pre-flight produces landed values that become R36-bis goldens.

- **`plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.{wav.sha256, json, json.sha256}`** — re-baseline to post-port output. Phase 2.4c R36 sha256 `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb` becomes the historical pre-port reference (preserved in git history at `115dbf4`; not re-emitted as a separate golden file). Post-port target: `decayEnvelopeDb[64] ∈ [−7.67, −6.67]` dB rel max.

- **`plugins/O-Contrabass/tests/render-harness/golden/vibrato.{wav.sha256, json, json.sha256}`** — **CONDITIONAL carry-forward**. Saturator port is upstream of vibrato modulator path (vibrato modulates BOW_SPEED at Step 4; saturator runs at Step 7). Research-phase pre-flight resolves: if post-port `--vibrato` mode reproduces Phase 2.4c R36c metrics (`peakDepthCents=9.526`, `vibratoRateHzMeasured=4.978 Hz`, `onsetTimeMs=1168`) within tolerance (±0.05¢ / ±0.005 Hz / ±2 ms) AND vibrato.wav.sha256 byte-identical → carry forward verbatim. If shift → re-baseline (10 audible re-baselines instead of 9). vibrato.json.sha256 = `2c4b3a7fa752f7f45437126101709a3a650c5b9aefc42aa513be4006da8e1a7d` is the conditional anchor.

- **`plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.{wav.sha256, json.sha256, json}`** — re-render **evidence-only**. NOT committed as a re-baselined golden in R36-bis (mirrors Phase 2.4a R34b "evidence golden, not in default reproduce-goldens.sh" pattern). Informs end-of-Stage-2 verify §"In-loop saturator" amendment evidence base. RESEARCH §19.7.7 documents whether: (a) raucous-corner cells stabilise (3 → fewer fails); (b) raucous-corner cells hold (3 fails unchanged); (c) NEW raucous corners surface (regression risk; investigate before R36-bis atomic).

- **`plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`** — entry count unchanged (13). Only sha256 values change inside per-entry comparisons (9 audible re-baselines + saturator-tail-comparison; vibrato carry-forward or re-baseline conditional).

- **`plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §19.7.7 (NEW)** — append post-port verdict subsection with sub-sections:
  - §19.7.7.1 source delta verification (4-LOC diff at WaveguideString.cpp:204–206)
  - §19.7.7.2 post-port saturator-tail decay envelope key bins (bin 0, bin 5, bin 60, bin 64)
  - §19.7.7.3 measured |Δ| at bin 64 vs O-Bowed reference
  - §19.7.7.4 9-audible-golden re-baseline sha256s
  - §19.7.7.5 vibrato carry-forward vs re-baseline determination
  - §19.7.7.6 matrix-stability post-port evidence
  - §19.7.7.7 sub-harmonics post-port measurements
  - §19.7.7.8 R37-bis Logic AU audition outcome
  - §19.7.7.9 verdict — port WORKED (audit-debt CLOSED) OR port WORKED-PARTIALLY (Phase 2.4-bis backlog) OR port DID-NOT-CONVERGE (escalate Phase 2.4c-bis-bis; low-probability — `tanh` is the canonical soft-saturator topology)
  - §19.7.7.10 evidence base for end-of-Stage-2 §"In-loop saturator" amendment

- **`plugins/O-Contrabass/.planning/STATUS.md`** — flip `phase` per cycle progression; flip `status` to `phase_2_4c_bis_discuss_complete`, `next_action` to `phase_2_4c_bis_research`; add `phase_2_4c_bis_discuss_carry_forward` block.

- **`plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` rev-11** — Phase 2.4c-bis plan-phase output, supersedes rev-10 with post-skeleton instantiation. Inherits R36-bis task breakdown verbatim from skeleton + research-phase pre-flight findings.

- **`plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md`, `VERIFICATION.md`** — append Phase 2.4c-bis sections at execute / verify phases respectively.

**Out of scope (deferred elsewhere):**

- **Phase 2.4-bis backlog** — kForceBoost retune (DSP-07; subharmEnergyRatio above 0.40 strict), Step 4 modulation gain / breathingAudible metric refinement (DSP-08; 20% peak-to-peak), VIBRATO_DEPTH→peakDepthCents transfer tune (DSP-09 from 2.4c deviation #6; strict 12¢ peak), 3 v1.0 fallback-cell reduction (Phase 2.4a). **All stay parked.** Strict saturator-only scope keeps the source delta unambiguous and clean for the §"In-loop saturator" amendment evidence base.
- **Phase 2.5** — body resonator + bow noise. Opens fresh CONTEXT **rev-10** post-2.4c-bis verify (per skeleton sequencing).
- **Phase 2.6** — master saturator/limiter, stereo width, microtonal, MPE, NE, MTS-ESP.
- **Chaos detector + softClampState** — Phase 2.5/2.6 (carry-forward from rev-7/rev-8 commit-body footnotes).
- **ARCHITECTURE.md amendments** — §"DC Blocker" + §"In-loop saturator" stay deferred to end-of-Stage-2 verify. Phase 2.4c-bis post-port saturator-tail golden + Phase 2.4c pre-port reference (`c7e845ea…`) feed the §"In-loop saturator" amendment evidence base together.
- **E1 dispersion calibration polynomial follow-up** (Phase 2.1c Risk #7) — separate `a(B, I)` cascaded-allpass concern.
- **Logic AU smoke deferred-non-blocking precedent** — overridden for Phase 2.4c-bis (R37-bis is BLOCKING; first audible source-edit since R35 warrants user audition).
- **Per-string `--vibrato` / `--saturator-tail-comparison` variants** — v1.1 or Phase 2.4-bis.
- **Output-path saturator/limiter** — Phase 2.6.
- **HR-12 (any new hard rule)** — skeleton's "no new HR introduced" stance confirmed (Q50). Port is straightforward formula swap; HR-1..HR-10 cover existing invariants.
- **Stage-1 contract amendment** — parameter-spec.md sha256 `77638e25…` carries forward unchanged.
- **ARCHITECTURE.md amendment in this cycle** — Phase 2.4c-bis is the implementation; the amendment lands at end-of-Stage-2.
- **Two-call-site audit follow-through scope expansion** — IF research-phase audit (Open Q3) reveals O-Bowed has saturator at TWO in-loop sites (`:135–139` AND `:217–219`), plan-phase scope-expands R36-bis to two saturator landings in O-Contrabass; otherwise scope stays single-site `:204–206`.

---

## Requirements Confirmed (Phase 2.4c-bis-relevant subsets of locked contracts)

- **DSP-01 / Bass-Range Waveguide Stability**: Phase 2.4c-bis ports the in-loop saturator topology to match O-Bowed reference. Stability invariant (no NaN, no peak > 1.0, no runaway, no denormal CPU spike) MUST hold across all 4 strings × INFINITE_SUSTAIN ∈ [0, 1]; verified via re-rendered matrix-stability evidence + 9 audible re-baseline goldens reproducing byte-identically across re-renders + auval/pluginval-10 SUCCESS.
- **DSP-09 / Layered Expression** (vibrato section): saturator port is upstream of vibrato modulator path; Phase 2.4c R36c restored-strict-with-deviations metrics (peakDepthCents=9.526, vibratoRateHzMeasured=4.978 Hz, onsetTimeMs=1168) MUST reproduce on post-port WAV within tolerance OR be re-baselined if the saturator subtly shifts the vibrato envelope. Research-phase pre-flights this.
- **QUAL-01** (no audio artifacts at normal ranges): Gate 6c-bis includes 9-audible-golden bit-deterministic-across-re-renders bar (HR-9-style IEEE 754 identity-arithmetic NOT applicable — this is a topology swap, not a no-op gate; goldens become NEW reference values, not byte-identical to Phase 2.4c values).
- **PERF-01** (no allocations in `processBlock`): saturator port is `std::tanh` which is RT-safe (no allocations, no system calls); pluginval-10 fuzz + Parameter thread safety re-confirm. `juce::ScopedNoDenormals` already in place.
- **PERF-02** (< 5% CPU on M1): `std::tanh` is ~3-5× slower than `std::sqrt` on M1 in float; per-block CPU impact is two `tanh` calls per sample on each rail = negligible (~0.5 µs/block at sr=44100 / blockSize=512). Pluginval-10 re-confirms no RT-safety regression.

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**

- All 29 APVTS parameter IDs, ranges, skews, defaults — `parameter-spec.md` (sha256:`77638e25…`). **NO Stage-1 contract amendment in Phase 2.4c-bis.**
- DSP architecture (`research/ARCHITECTURE.md`, sha256:`3cb26814…`). **NO ARCHITECTURE.md amendment in Phase 2.4c-bis** — saturator port is implementation-detail update; §"In-loop saturator" amendment lands at end-of-Stage-2 verify with both pre-port (Phase 2.4c R36 `c7e845ea…`) AND post-port (Phase 2.4c-bis R36-bis NEW sha256) goldens as evidence base.
- ROADMAP phasing (sha256:`106639f6…`).
- `modules/synthesis/bow-friction/` v1.0.0 (Phase 2.1b) — value-class deterministic; **Phase 2.4c-bis does NOT touch friction module surface**.
- `Source/DSP/DispersionFilter.h` (Phase 2.1c, R20 commit `5759e5e`) — verbatim consume.
- `Source/DSP/SchellengCalibration.h` (Phase 2.4a, R34 commit `4c926bb`) — verbatim consume.
- `Source/DSP/SubHarmonicBias.h` (Phase 2.4b, R35 commit `3de8b66`) — verbatim consume.
- `Source/BowedContrabassVoice.{h,cpp}` (Phase 2.4b end-state) — verbatim consume; **zero edits in Phase 2.4c-bis**.
- `Source/PluginProcessor.{h,cpp}` — 29 APVTS params + Phase 2.4b voice wiring; verbatim consume.
- Phase 2.3 modulator surface + 7-step + Step 2.5 (Phase 2.4b) + HR-1..HR-10 verbatim carry-forward.
- HR-11 (Phase 2.4c zero-production-DSP-edits) — **RETIRED** at Phase 2.4c-bis. Audit history preserves the rule binding for Phase 2.4c only.
- Per-block evaluation order — unchanged. Phase 2.4c-bis modifies only the per-sample saturator topology in Step 7.

**Phase 2.4c-bis-specific constraints:**

- **Source edits limited to `Source/DSP/WaveguideString.cpp:204–206`.** No other DSP source touched — voice, friction, dispersion, calibration, sub-harmonic, processor, MPE synthesizer all carry forward verbatim. Audit hook: `git diff --stat HEAD -- plugins/O-Contrabass/Source/` MUST report exactly 1 file (WaveguideString.cpp) modified at R36-bis pre-flight + R36-bis-e re-tripwire.
- **Both rails get identical port** (toBridge + toNeck). No asymmetric saturator variants. Trivially verified via grep audit hook: `grep -c "sat \* std::tanh" Source/DSP/WaveguideString.cpp` MUST return 2 (or 4 if Open Q3 audit triggers two-call-site scope expansion).
- **9 audible goldens re-baseline; vibrato carries forward conditional; matrix-stability re-render evidence-only.** sub-harmonics + sub-harmonics-stability re-baseline because subharmonic energy ratio depends on saturator curvature.
- **Convergence target: post-port bin 64 ∈ [−7.67, −6.67] dB rel max** (±0.5 dB parity with O-Bowed reference −7.17 dB at the 5-s post-bow-off mark). Tighter than Q41 perceptual-JND budget; sub-JND headroom for confidence.
  - **Strict-PASS:** post-port bin 64 ∈ [−7.67, −6.67] dB.
  - **Soft-PASS (widening to ±1.0 dB):** post-port bin 64 ∈ [−8.17, −6.17] dB; document residual divergence as Phase 2.4-bis backlog item.
  - **Escalate (no-PASS):** post-port bin 64 outside [−8.17, −6.17] dB; investigate `sat` constant tune, two-call-site asymmetry, or alternative topology before plan-phase commits.
- **R37-bis Logic AU smoke is BLOCKING.** First audible source-edit since R35; user auditions pre-port (`c7e845ea…` reference rendered from `115dbf4` checkout) vs post-port render in Logic Pro (AU) before R36-bis atomic commit lands. Departure from R37/R32/R27/R19f/R14e/R34h/R35 deferred-non-blocking precedent.
- **Strict bit-deterministic-across-re-renders bar** for all 9 audible re-baselines (and saturator-tail-comparison). Research-phase pre-flights 3 back-to-back renders to confirm sha256 stability before plan-phase locks expected sha256s. If non-deterministic, escalate (likely needs explicit `tanhf` or LUT — would expand source delta).
- **NO Stage-1 contract amendment.** parameter-spec.md sha256 `77638e25…` carries forward unchanged. STATUS.md `contract_checksums.parameter_spec` unchanged.
- **NO ARCHITECTURE.md amendment.** End-of-Stage-2 verify still owns §"DC Blocker" + §"In-loop saturator" amendments.
- **Phase 2.4-bis backlog stays parked.** Strict saturator-only scope.

**Working-tree starting state (locked from Phase 2.4c verify, R36 commit `115dbf4` + R36-backfill chore `7835904`):**

- All Phase 2.4c end-state source verbatim (BowedContrabassVoice, SubHarmonicBias.h, SchellengCalibration.h, WaveguideString, DispersionFilter, PluginProcessor, MPE synthesizer).
- 13 currently-committed reproduce-goldens.sh entries (12 carry-forward through Phase 2.4b + 1 new Phase 2.4c saturator-tail-comparison `c7e845ea…`).
- `reproduce-goldens.sh` (Phase 2.4a R34-pre infrastructure, extended Phase 2.4b R35 + Phase 2.4c R36).
- `preflight-subharm.sh` (Phase 2.4b R35-pre HR-9 escalation gate).
- `vibrato.json.sha256 = 2c4b3a7f…` (Phase 2.4c R36 anchor; conditional carry-forward — research-phase pre-flight resolves).
- `matrix-stability.{wav.sha256 = 6db67707…, json.sha256, json}` (Phase 2.4a evidence, NOT in default reproduce-goldens.sh).
- O-Bowed harness Option B extension landed at Phase 2.4c R36b (`canonical-preset.wav.sha256 = 93124fb8…` byte-identical when flags absent).

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q46 — Phase 2.4c-bis cycle scope** | **Pre-written rev-9-bis skeleton verbatim** | User-confirmed (Recommended). Port `sat · tanh(x / sat)` with `sat = 4.0f` from O-Bowed:135–139 (and possibly :217–219 — research-phase audit) to O-Contrabass:204–206 (in-loop both rails); HR-11 lifted; HR-1..HR-10 carry-forward; R36-bis atomic; 9 audible re-baseline + new sat-tail goldens; vibrato carry-forward conditional; matrix-stability re-render evidence-only. Skeleton is one-action-away by design (PLAN rev-10 §"Contingency"). |
| **Q47 — Gate 6c-bis convergence target** | **\|Δ\| ≤ 0.5 dB at bin 64 vs O-Bowed reference (−7.17 dB)** | User-confirmed (Recommended). Tighter than skeleton's <2 dB. Pre-port was 5.92 dB divergent; the win is parity, not just sub-perceptual. Sub-JND headroom for confidence. Target window: bin 64 ∈ [−7.67, −6.67] dB rel max. Soft-PASS widening to ±1.0 dB available if post-port lands in [−8.17, −6.17] with documented residual divergence (Phase 2.4-bis backlog item). |
| **Q48 — Phase 2.4-bis backlog absorption** | **Strict saturator-only** | User-confirmed (Recommended). Phase 2.4-bis backlog (kForceBoost retune, Step 4 / breathingAudible tune, VIBRATO_DEPTH transfer tune) stays parked. Mixing in DSP-side tuning makes the source delta ambiguous and complicates the §"In-loop saturator" amendment evidence base. |
| **Q49 — R37-bis Logic AU smoke timing** | **BLOCKING audition before R36-bis atomic commit** | User-confirmed (Recommended). Saturator port is the first audible source-edit since Phase 2.4b R35. User auditions pre-port (`c7e845ea…` reference rendered from `115dbf4`) vs post-port render in Logic Pro (AU) before R36-bis lands. Departs from R37/R32/R27/R19f/R14e/R34h/R35 deferred-non-blocking precedent. Catches `tanh` saturator producing unexpected character change beyond pure decay-envelope shape. |
| **Q50 — New Hard Rule** | **None — skeleton's "no new HR" stance** | User-confirmed (Recommended). Port is a 4-LOC formula swap; both expressions (`x/sqrt(1+x²)` and `4·tanh(x/4)`) are odd-symmetric, monotonic, soft-saturating; HR-1..HR-10 cover the existing invariants. Adding HR-12 over-engineers the change. |
| **Q51 — CONTEXT.md rev numbering** | **rev-9-bis** | User-confirmed (Recommended). Matches PLAN rev-10 §"Contingency" pre-write convention. Signals "escalation off rev-8" in audit trail. Phase 2.5 then opens rev-10 (per skeleton §"Sequencing post-2.4c-bis"). |
| **Q52 — Atomic commit shape** | **R36-bis atomic + R36-bis-backfill chore** | User-confirmed (Recommended). Continues atomic-commit sequence: R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → **R36-bis** (Phase 2.4c-bis Gate 6c-bis PASS). R36-bis-backfill chore propagates R36-bis sha into STATUS.md per R34/R35/R36 backfill precedent. R37-reservation for Logic AU smoke deferred-non-blocking is retired (R37-bis is BLOCKING for this cycle). |
| Per-block evaluation order | **Unchanged from Phase 2.4c end-state** | 7-step + Step 2.5 carry-forward verbatim. Phase 2.4c-bis does NOT modify the per-block evaluation order; only the per-sample saturator topology in Step 7 changes. |

---

## Open Questions (handed to research-phase)

1. **Pre-port repro tripwire.** Render 13 reproduce-goldens.sh entries at HEAD (`7835904` descendant or current main) BEFORE source edits — confirm bit-identical reproduction against committed sha256s. Mirrors R36-pre tripwire pattern. Catches any upstream drift introduced between R36-backfill chore (`7835904`) and Phase 2.4c-bis discuss-phase open.

2. **Saturator port scratch-space prototype + convergence pre-flight.** Apply the 4-LOC port to `Source/DSP/WaveguideString.cpp:204–206` in research-phase scratch (NOT committed); render the canonical saturator-tail-comparison golden; measure bin 64 dB; confirm convergence within ±0.5 dB of O-Bowed reference (−7.17 dB). If convergence within [−7.67, −6.67]: PASS path locked. If [−8.17, −7.67] ∪ [−6.67, −6.17]: soft-PASS with documented widening + Phase 2.4-bis backlog item. If outside [−8.17, −6.17]: investigate (sat constant ≠ 4.0f? coefficient deviation? two-call-site asymmetry in O-Bowed?) before plan-phase.

3. **Two-call-site audit in O-Bowed.** O-Bowed has saturator at TWO sites: `WaveguideString.cpp:135–139` AND `:217–219`. Why two sites? Are both in-loop pre-pushSample, or is one out-of-loop / output-path / different scattering junction? Confirm O-Contrabass `WaveguideString.cpp:204–206` is the architectural equivalent of which O-Bowed site (likely `:135–139` based on inline `// Soft saturation prevents numerical blowup` comment context). Plan-phase scope-expands to two-call-site port if O-Bowed `:217–219` is also in-loop AND O-Contrabass has a corresponding architectural site. Default scope: single-site port at `:204–206`.

4. **Pre-port saturator-tail reference preservation.** Phase 2.4c R36 sha256 `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb` lives in git history at `115dbf4` (NOT in working-tree post-R36-bis). Research-phase confirms re-render reproducibility from `115dbf4` checkout for end-of-Stage-2 §"In-loop saturator" amendment evidence base. Document checkout-and-render protocol in RESEARCH §19.7.7.

5. **9-audible-golden re-baseline strategy.** All 9 affected goldens get fresh sha256s simultaneously (since the saturator participates in steady-state energy balance for any sustained excitation). Research-phase pre-flights 3 back-to-back renders per golden to confirm bit-deterministic across re-renders. Predicted sha256s NOT committed to PLAN rev-11 (would require source change to measure); plan-phase pin: "Predicted sha256s pre-flighted at execute-phase scratch space; landed values become R36-bis goldens."

6. **Vibrato golden carry-forward vs re-baseline determination.** Saturator port is upstream of vibrato modulator path (vibrato modulates BOW_SPEED at Step 4; saturator runs at Step 7). Question: does the post-port saturator topology subtly shift the vibrato envelope's autocorrelation peak (i.e., does `peakDepthCents=9.526` hold post-port within IEEE 754 tolerance, or shift)? Research-phase pre-flight `--vibrato` mode against post-port WAV; if metrics reproduce within ±0.05¢ / ±0.005 Hz / ±2 ms AND vibrato.wav.sha256 byte-identical (`d7881ecf…`), vibrato golden carries forward verbatim. If shift, vibrato re-baselines too (10 audible re-baselines instead of 9).

7. **Matrix-stability re-render verdict structure.** Re-render is evidence-only (NOT committed as a re-baselined golden). Document in RESEARCH §19.7.7.6 whether: (a) raucous-corner cells stabilise (3 → fewer fails; reduce v1.0 fallback from `0.5` → `1.0` for some cells; carry into Phase 2.4-bis backlog as scope reduction); (b) raucous-corner cells hold (3 fails unchanged; v1.0 fallback intact); (c) NEW raucous corners surface (regression risk; investigate before R36-bis atomic). If (c), block plan-phase pending root-cause.

8. **Sub-harmonics + sub-harmonics-stability re-baseline.** Both depend on saturator curvature. Research-phase confirms `subharmEnergyRatio` post-port behavior (Phase 2.4b R35 landed 0.358 soft-PASS at default operating point with algebraic saturator; post-port `tanh` may shift this either direction). Document landed value in RESEARCH §19.7.7.7; if drops below 0.30, escalate to investigate (likely benign; tanh is steeper at low amplitude where subharmonics live, may help).

9. **R36-bis task breakdown.** Initial estimate: R36-bis-pre tripwire (13-entry reproduce-goldens.sh PASS at HEAD + scratch-space port prototype convergence check + two-call-site audit) + R36-bis-a (port + 4-LOC source edit at Source/DSP/WaveguideString.cpp:204–206) + R36-bis-b (re-baseline 9 audible goldens + saturator-tail-comparison + matrix-stability re-render evidence + sub-harmonics measurement) + R36-bis-c (RESEARCH §19.7.7 verdict + post-port measurement table) + R36-bis-d (R37-bis Logic AU smoke audition — BLOCKING — pre-port from `115dbf4` checkout vs post-port from working-tree) + R36-bis-e (regression bar via 13-entry `reproduce-goldens.sh` against new sha256s; HR-11-style audit hook re-runs zero-edit-outside-WaveguideString.cpp guard) + R36-bis-f (auval + pluginval-10) + R36-bis atomic commit + R36-bis-backfill chore. Research-phase locks task body and ordering.

10. **Vibrato carry-forward conditional R36-bis split.** If research-phase pre-flight (Open Q6) shows vibrato.wav.sha256 byte-identical post-port: 9 audible goldens re-baseline; vibrato + vibrato.json + vibrato.json.sha256 carry forward verbatim. If vibrato ALSO needs re-baseline: 10 audible goldens re-baseline + vibrato.json metric-shift documented in RESEARCH §19.7.7.5. Plan-phase locks the conditional outcome based on research-phase pre-flight.

11. **R37-bis audition protocol.** Pre-port reference: render `c7e845ea…` from `115dbf4` checkout into Logic Pro (AU) via O-Contrabass-dev.component. Post-port reference: render post-port from working-tree O-Contrabass-dev.component. Audition: A/B sustained E1 + per-string MIDI 33/38/43 + tail-decay character at MIDI lift. PASS criteria: post-port sounds "more O-Bowed-like + smoother decay tail" without unexpected character changes (e.g., transient artifacts, peak-amplitude shifts, vibrato envelope distortion, harmonic spectrum changes). User audition is the bar. If post-port reveals unexpected character change, escalate before R36-bis atomic.

12. **RESEARCH §19.7.7 deliverable structure.** §19.7.7 Phase 2.4c-bis post-port verdict subsection. 10 sub-sections per "In Scope" §19.7.7.1–§19.7.7.10 list above. Independent of §19.7.6 (which locked the escalation flag); §19.7.7 closes the loop by either confirming the port WORKED (audit-debt CLOSED) or characterising residual divergence.

---

## Risks (Phase 2.4c-bis-specific)

1. **Saturator port introduces unexpected character change** beyond pure decay-envelope shape (e.g., transient distortion, vibrato envelope shift, harmonic spectrum shift). Mitigation: R36-bis-pre scratch prototype (Open Q2) + R37-bis BLOCKING Logic AU audition before atomic commit. User audition catches subjective issues; objective measurements (auval, pluginval-10, re-rendered goldens bit-deterministic-across-re-renders) catch RT-safety + numeric issues.

2. **Convergence misses ±0.5 dB target window** (post-port bin 64 outside [−7.67, −6.67]). Mitigation: research-phase pre-flight (Open Q2) catches this BEFORE plan-phase commits. Three resolution paths: (a) sat constant tune (`sat=4.0f` → `sat=3.5f` or `sat=4.5f`); (b) accept widened band [−8.17, −6.17] as soft-PASS + Phase 2.4-bis backlog item; (c) escalate to Phase 2.4c-bis-bis with alternative topology (lookup table, polynomial, or O-Bowed `:217–219` second-call-site investigation).

3. **Two-call-site asymmetry in O-Bowed** (Open Q3). O-Bowed has saturator at `:135–139` AND `:217–219`. If both are in-loop pre-pushSample at architecturally-equivalent O-Contrabass sites, single-site port at `:204–206` is incomplete. Mitigation: research-phase audit catches this; plan-phase scope-expands if needed. If scope-expansion required, source delta grows to ≈ 8 LOC across two locations in WaveguideString.cpp; HR-11-equivalent audit hook still reports exactly 1 file modified.

4. **9-audible-golden re-baseline drift across runs.** Saturator port may introduce non-determinism (e.g., compiler-emitted `std::tanh` vs LUT-based; different M1 toolchains may emit different bit-exact `tanh` results). Mitigation: research-phase pre-flights 3 back-to-back renders per golden + verifies sha256 bit-stability. If non-deterministic, escalate (likely needs explicit `tanhf` or LUT — would expand source delta).

5. **Vibrato golden carry-forward fails** (Open Q6). If post-port vibrato envelope subtly shifts the autocorrelation peak position, vibrato golden re-baselines too (10 audible re-baselines instead of 9). Plan-phase locks conditional outcome; research-phase decides.

6. **Matrix-stability post-port reveals NEW raucous corners** (Open Q7 case (c)). Regression risk — `tanh` is monotonic + smooth + bounded, so this is low-probability. If it surfaces, root-cause investigation blocks plan-phase.

7. **Sub-harmonics post-port `subharmEnergyRatio` drops significantly** (Open Q8). Steeper-at-low-amplitude `tanh` may either help (more subharmonic generation at bow-pressure transients) or hurt (less low-amplitude curvature). Mitigation: research-phase measures + documents. Likely benign within 0.30–0.40 range. If drops below 0.30, separate sub-harmonics-stability re-baseline may shift status (not blocking 2.4c-bis closure; carry into Phase 2.4-bis backlog).

8. **R37-bis BLOCKING audition reveals subjective issue.** Mitigation: Phase 2.4c-bis stays open (atomic-commit deferred) until subjective issues resolved. Resolution paths: (a) sat constant tune (research-phase scratch prototype); (b) escalate to Phase 2.4c-bis-bis alternative topology; (c) re-open Phase 2.4c verdict path (acknowledge divergence as architectural; revert R36-bis source edits; close 2.4c-bis as research-only acknowledged-divergence). Path (c) requires user input.

9. **Audit-hook drift mid-cycle.** R36-bis-pre + R36-bis-e audit hooks (`git diff --stat HEAD -- plugins/O-Contrabass/Source/`) MUST report exactly 1 file (WaveguideString.cpp) modified; ANY other file modification = scope-expansion HARD violation. Mitigation: tripwire pattern.

10. **`std::tanh` RT-safety concern.** `std::tanh` on macOS / linux libm is not RT-strict (potential for system-call entry on denormals). Mitigation: pluginval-10 fuzz + Parameter thread safety re-confirm; `juce::ScopedNoDenormals` already in place; if pluginval flags issue, escalate to LUT-based `tanh` or polynomial approximation (expands source delta beyond 4 LOC).

11. **Phase 2.5-awareness.** Body resonator (Phase 2.5) changes downstream amplitude envelope; the in-loop saturator-tail decay characterisation might be moot once body modes contribute. Mitigation: Phase 2.4c-bis verdict explicitly notes "valid for v1.0 pre-Phase-2.5 architecture"; Phase 2.5 verify includes a saturator-tail re-measurement as regression check (carry-forward from rev-8 risk #10).

12. **R36-bis atomic-commit interaction with R36-backfill chore.** R36-backfill (`7835904`) most-recent at HEAD. R36-bis atomic lands while R36-backfill is most-recent. R36-bis-backfill chore propagates R36-bis sha after R36-bis atomic (mirrors R36-backfill / R35-backfill / R34-backfill precedent).

13. **Pre-port reference re-render reproducibility** (Open Q4). `c7e845ea…` was rendered at Phase 2.4c R36 against working-tree at `115dbf4`. Re-rendering from a clean `115dbf4` checkout (e.g., `git worktree add /tmp/oc-pre-port 115dbf4`) MUST reproduce byte-identical sha256. If not, the pre-port reference is non-reproducible and the §"In-loop saturator" amendment evidence base is compromised. Mitigation: research-phase pre-flights `git worktree` + render protocol; documents in RESEARCH §19.7.7.

---

## Gate 6c-bis Five-Item Success Criteria (preliminary; PLAN rev-11 locks)

1. **Bit-deterministic regression bar** — all 13 reproduce-goldens.sh entries reproduce byte-identical via post-port sha256s (or 14 if vibrato also re-baselines per Open Q6 / Open Q10). 9 audible goldens get NEW reference sha256s; saturator-tail-comparison gets NEW post-port sha256; vibrato carry-forward conditional. HR-11-style audit hook reports exactly 1 file (WaveguideString.cpp) modified at R36-bis-pre + R36-bis-e tripwires.
2. **Convergence target** — `--saturator-tail-comparison` post-port `decayEnvelopeDb[64]` ∈ [−7.67, −6.67] dB rel max (strict-PASS) OR ∈ [−8.17, −6.17] dB rel max (soft-PASS with documented residual divergence + Phase 2.4-bis backlog item) + RESEARCH §19.7.7 verdict written.
3. **`auval` + `pluginval-10` PASS** — auval AU VALIDATION SUCCEEDED full render-rate matrix; pluginval --strictness-level 10 SUCCESS full battery (Editor Automation, Automatable Parameters, Parameter thread safety, Background thread state, Bus enable/disable, Restoring default layout, Fuzz parameters all complete).
4. **R37-bis Logic AU audition** — user-confirmed audition CONFIRMS post-port character is acceptable (no unexpected character changes; saturator port produces "more O-Bowed-like + smoother decay tail" subjective improvement). BLOCKING — R36-bis atomic does NOT land until R37-bis audition CONFIRMED.
5. **RESEARCH §19.7.7 verdict locked** — Phase 2.4c-bis verdict written: port WORKED (audit-debt CLOSED, end-of-Stage-2 §"In-loop saturator" amendment evidence base ready) OR port WORKED-PARTIALLY (Phase 2.4-bis backlog item logged) OR port DID-NOT-CONVERGE (Phase 2.4c-bis-bis escalation flag LOCKED — low-probability per analytic match, `tanh` is the canonical soft-saturator topology).

---

## Next Phase

Ready for: **research** phase — `/clear` then `/plugin-research O-Contrabass 2-dsp`

Research focus (Phase 2.4c-bis):

1. **Resolve Open Questions #1–#12** — pre-port repro tripwire, scratch-space port prototype convergence, two-call-site audit in O-Bowed, pre-port reference preservation, 9-audible-golden re-baseline strategy, vibrato carry-forward determination, matrix-stability re-render verdict structure, sub-harmonics post-port measurement, R36-bis task breakdown, vibrato carry-forward conditional, R37-bis audition protocol, RESEARCH §19.7.7 deliverable structure.
2. **Pre-port repro tripwire** — render all 13 reproduce-goldens.sh entries at HEAD; verify byte-identical against committed sha256s. If any drift, INVESTIGATE before plan-phase.
3. **Scratch-space port prototype** — apply 4-LOC port; render canonical saturator-tail-comparison; measure bin 64; confirm ±0.5 dB convergence (target [−7.67, −6.67] dB rel max). If outside, escalate to alternative resolution path (sat constant tune, widened band, two-call-site audit, alternative topology).
4. **Two-call-site audit in O-Bowed** — confirm `:135–139` is in-loop pre-pushSample (architectural equivalent of O-Contrabass `:204–206`); confirm `:217–219` is the second in-loop saturator site OR out-of-loop output-path. If second in-loop site exists at architecturally-equivalent O-Contrabass location, scope-expand R36-bis to two saturator landings.
5. **Vibrato carry-forward pre-flight** — render `--vibrato` mode against post-port WAV; verify autocorrelator metrics reproduce within tolerance; confirm vibrato.wav.sha256 byte-identical OR document re-baseline.
6. **Matrix-stability post-port measurement** — re-render 108-combo matrix; document raucous-corner cells stable / unchanged / new.
7. **Sub-harmonics + sub-harmonics-stability post-port measurement** — render both; document landed `subharmEnergyRatio`.
8. **R37-bis audition protocol pre-write** — document Logic AU smoke audition steps for execute-phase user reference (incl. `git worktree` checkout protocol for pre-port reference render).
9. **Append RESEARCH §19.7.7** — document all resolutions above. (No §19.7.6 changes; rev-9 verdict locked.)

After research: plan-phase (PLAN rev-11) writes R36-bis task breakdown verbatim against this CONTEXT + research findings; execute-phase performs implementation + 9 (or 10) re-baselined goldens + R36-bis atomic commit (BLOCKED on R37-bis Logic AU audition); verify-phase confirms Gate 6c-bis invariants.

---

## Audit Trail (rev-9-bis supersedes rev-8)

**rev-1 (2026-04-26):** Phase 2.1 broad discuss. Cycle scope = Phase 2.1 (sub-phases a/b/c).
**rev-2 (2026-04-26):** Phase 2.1a closure (Option A, R7) + Phase 2.1b opening (module extraction, Gate 2). Phase 2.1b verified 2026-04-27 (R8a `bd5fae0` + R15 `ef0604d`, Gate 2 PASS).
**rev-3 (2026-04-27):** Phase 2.1c opening — cascaded allpass dispersion. Verified 2026-04-27 (R20 `5759e5e`, Gate 3 PASS).
**rev-4 (2026-04-27):** Phase 2.2 opening — 4-string EADG + per-string detune + per-string M-table. Verified 2026-04-27 (R26 `131c2c7`, Gate 4 PASS).
**rev-5 (2026-04-27):** Phase 2.3 opening — Vibrato + Slow-Bow LFO + Schelleng wedge clamp + EXPRESSION_MACRO. HR-1..HR-4 binding. Verified 2026-04-27 (R33 `af54571`, Gate 5 PASS with rebaseline of 4 audible carry-forward goldens).
**rev-6 (2026-04-27):** Phase 2.4a opening — Schelleng wedge bass-register calibration polynomial + 108-combo stability matrix dual-purpose render + `pass_breathingAudible` 5%→20% threshold restoration. HR-5..HR-8 binding. Verified 2026-04-28 (R34 `4c926bb`, Gate 6a CLEARED — 3 strict-PASS + 2 soft-PASS within v1.0 budgets) + R34-backfill chore `b64c8c4`.
**rev-7 (2026-04-28):** Phase 2.4b opening — Sub-Harmonic Bias DSP-07 (ARCHITECTURE §457). HR-9 + HR-10 binding. Verified 2026-04-28 (R35 `3de8b66`, Gate 6b CLEARED — 4 strict-PASS + 1 soft-PASS within RESEARCH §18.6 v1.0 budget) + R35-backfill chore `0db5fac`.
**rev-8 (2026-04-28):** Phase 2.4c opening — autocorrelator octave-rejection harness fix + saturator-tail O-Bowed comparison (research-only). HR-11 binding (zero production DSP edits). Verified 2026-04-29 (R36 `115dbf4`, Gate 6c CLEARED via escalation lane — 5/5 invariants; §19.7.6 escalation flag LOCKED on measured 5.92 dB envelope divergence > 2 dB Q41 threshold + ~3 dB perceptual JND) + R36-backfill chore `7835904`.

**rev-9-bis (this document, 2026-04-29):** Phase 2.4c-bis opening — source-change in-loop saturator port from O-Bowed (`sat · tanh(x / sat)` with `sat = 4.0f`) to O-Contrabass `Source/DSP/WaveguideString.cpp:204–206` replacing algebraic `x / sqrt(1 + x²)` on both rails. Triggered by Phase 2.4c §19.7.6 escalation flag LOCKED. **7 approach decisions Q46–Q52 user-confirmed**: skeleton verbatim cycle scope (Q46); ±0.5 dB convergence target at bin 64 vs O-Bowed reference −7.17 dB (Q47); strict saturator-only scope, Phase 2.4-bis backlog parked (Q48); BLOCKING R37-bis Logic AU audition before R36-bis atomic commit (Q49); no new HR introduced — HR-1..HR-10 carry-forward, HR-11 retired (Q50); CONTEXT rev-9-bis numbering convention (Q51); R36-bis atomic + R36-bis-backfill chore (Q52). **13 open questions handed to research-phase**: pre-port repro tripwire, scratch-space port prototype convergence, two-call-site audit in O-Bowed, pre-port reference preservation, 9-audible-golden re-baseline strategy, vibrato carry-forward determination, matrix-stability re-render verdict structure, sub-harmonics post-port measurement, R36-bis task breakdown, vibrato carry-forward conditional, R37-bis audition protocol, RESEARCH §19.7.7 deliverable structure, pre-port reference re-render reproducibility. **HR-11 RETIRED** (binding limited to Phase 2.4c only; Phase 2.4c-bis is the source-change escalation cycle by design). Phase 2.5 (body resonator + bow noise) gets fresh CONTEXT rev-10 (NOT rev-9; skeleton sequencing). Continues atomic-commit sequence R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → **R36-bis**.

**Inherited verbatim from rev-8 (not re-litigated):**

- All Phase 2.3 modulator surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order)
- All Phase 2.4b end-state (Step 2.5 sub-harmonic bias evaluation between Step 2 and Step 3; subHarmonicsSmoothed 30 ms ramp; lastSubAmount instrumentation; voiceBowForceUpliftThisBlock factor at Step 6)
- HR-1..HR-4 (Phase 2.3 literal-zero short-circuits + IEEE 754 identity-arithmetic + Schelleng skip on zero LFO depth)
- HR-5..HR-8 (Phase 2.4a inline constexpr linkage on SchellengCalibration.h + calibration behind HR-4 gate ONLY + matrix-stability bypass via weak-symbol + trilinear IEEE 754 identity arithmetic)
- HR-9..HR-10 (Phase 2.4b SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate + friction module ABI preservation via ROSIN inverse algebraic identity)
- 13 currently-committed reproduce-goldens.sh entries (12 carry-forward through Phase 2.4b + 1 new Phase 2.4c saturator-tail-comparison `c7e845ea…`) + matrix-stability evidence golden
- vibrato.json.sha256 = `2c4b3a7f…` (Phase 2.4c R36 anchor; conditional carry-forward — research-phase pre-flight resolves)
- O-Bowed harness Option B extension (canonical-preset.wav.sha256 = `93124fb8…` byte-identical when flags absent)
- Atomic-commit gate-first principle (R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis)
- Phase 2.4-bis backlog parking (kForceBoost retune; Step 4 modulation gain / breathingAudible metric refinement; 3 fallback-cell reduction; VIBRATO_DEPTH→peakDepthCents transfer tune)
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify
- E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7) — separate concern
- Primary listening DAW: Logic Pro (AU)
- Sample-rate strategy: internal 88.2 / 96 kHz at friction junction
- Bow-friction module v1.0.0 at `modules/synthesis/bow-friction/`
- Per-plugin `DispersionFilter.h` + `SchellengCalibration.h` + `SubHarmonicBias.h` (NOT extracted to shared module)
- 29 APVTS parameters; parameter-spec.md sha256 `77638e25…` carries forward unchanged
- Stage-1 contract NOT amended in Phase 2.4c-bis
- ARCHITECTURE.md NOT amended in Phase 2.4c-bis
- Chaos detector + softClampState deferred to Phase 2.5/2.6 (Phase 2.4b R35 commit-body footnote)

**New in rev-9-bis:**

- Q46 Phase 2.4c-bis cycle scope = pre-written rev-9-bis skeleton verbatim (saturator port from O-Bowed)
- Q47 Convergence target = |Δ| ≤ 0.5 dB at bin 64 vs O-Bowed reference −7.17 dB (target window [−7.67, −6.67]; soft-PASS widening to [−8.17, −6.17])
- Q48 Strict saturator-only — Phase 2.4-bis backlog stays parked
- Q49 BLOCKING R37-bis Logic AU audition before R36-bis atomic commit
- Q50 No new Hard Rule introduced; HR-1..HR-10 carry-forward; HR-11 retired
- Q51 CONTEXT rev-9-bis numbering convention
- Q52 R36-bis atomic + R36-bis-backfill chore
- HR-11 RETIRED (binding limited to Phase 2.4c only)
- 4-LOC source edit at `Source/DSP/WaveguideString.cpp:204–206` (`x/sqrt(1+x²)` → `sat·tanh(x/sat)` with `sat=4.0f`)
- 9 audible goldens re-baseline (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + macro-sweep + slow-lfo + schelleng-stress + sub-harmonics + sub-harmonics-stability)
- Saturator-tail-comparison golden re-baseline (post-port bin 64 ∈ [−7.67, −6.67] dB target)
- Vibrato carry-forward conditional (research-phase pre-flight resolves)
- Matrix-stability re-render evidence-only (NOT committed as re-baselined golden)
- RESEARCH §19.7.7 NEW post-port verdict subsection (10 sub-sections)
- Five-item Gate 6c-bis bar: (1) bit-deterministic 13-entry reproduce-goldens.sh (or 14 if vibrato re-baselines) + HR-11-style audit hook reports exactly 1 file modified; (2) saturator-tail post-port bin 64 ∈ [−7.67, −6.67] dB strict-PASS (or [−8.17, −6.17] soft-PASS) + RESEARCH §19.7.7 verdict written; (3) auval + pluginval-10 SUCCESS; (4) R37-bis Logic AU audition CONFIRMS post-port character acceptable (BLOCKING); (5) RESEARCH §19.7.7 Phase 2.4c-bis verdict locked.
