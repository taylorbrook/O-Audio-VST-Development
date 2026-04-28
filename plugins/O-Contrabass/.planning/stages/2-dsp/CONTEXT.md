# Stage 2: DSP — Context (rev-6)

**Date:** 2026-04-27
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.4a — Schelleng Wedge Bass-Register Calibration Polynomial + 108-Combo Stability Matrix + `pass_breathingAudible` 5%→20% Threshold Restoration**
**Supersedes:** rev-5 (Phase 2.3 — Vibrato + Slow-Bow LFO + Schelleng Wedge Clamp + EXPRESSION_MACRO, dated 2026-04-27). rev-5 contracts that remain locked are inherited verbatim and not re-litigated. Phase 2.3 closed 2026-04-27 with R33 atomic commit (`af54571`, Gate 5 PASS with rebaseline of 4 audible carry-forward goldens; strict E1 + detune-sweep-A unchanged; HR-1..HR-4 IEEE 754 identity-arithmetic preserved).

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle opens **Phase 2.4a** — first of three Phase 2.4 sub-cycles. Phase 2.4 as scoped at end of Phase 2.3 verify bundles 6 heterogeneous items (Schelleng calibration + breathingAudible threshold + sub-harmonic + 108-combo matrix + saturator-tail re-evaluation + autocorrelator harness fix). User selected **split into 2.4a/2.4b/2.4c** mirroring Phase 2.1a/b/c precedent. Each sub-phase is a tight gate-first GSD cycle with its own atomic commit (R34 → R35 → R36 sequence continuing R7 → R15 → R20 → R26 → R33).

**Phase 2.4a sub-cycle = friction-junction wedge math:** empirical Schelleng wedge calibration polynomial fit per-string from a 108-combo bass-register stability matrix render, plus restoration of the architecture-spec'd `pass_breathingAudible ≥ 20%` threshold (currently softened to 5% v1.0 as Phase 2.3 parking decision). The 108-combo render is **dual-purpose**: (1) supplies coefficient data for per-string `safeDepth(v_b, F_bow, β)` polynomial fit, (2) supplies QUAL-01 click-free / pass_noNaN / pass_peak / pass_blockTime stability evidence across the bass operating envelope. Single render serves both purposes; ~9 min wall-clock budget.

**Phase 2.4b** (sub-harmonic bias DSP-07 per ARCHITECTURE §457) and **Phase 2.4c** (autocorrelator octave-rejection harness fix + saturator-tail O-Bowed comparison harness) get fresh CONTEXT.md revs (rev-7, rev-8) when they open. This rev-6 documents Phase 2.4a only.

After Phase 2.4a verifies (Gate 6a PASS), Phase 2.4b discuss-phase opens.

---

## Cycle Scope

**Goal:** Replace the Phase 2.3 architecture-verbatim closed-form Schelleng wedge (Z=R=R_s=0.5 dimensionless collapse) with an empirically-fit per-string calibration polynomial that produces non-zero `safeDepth` at default bass-register operating points (currently `clampedDepthMean=0.0` silences slow-LFO at MIDI 28-43). Polynomial coefficients fit from 108-combo bass-register stability matrix render (4 strings × 3 BOW_SPEED × 3 BOW_PRESSURE × 3 BOW_POSITION). Same render gates QUAL-01 click-free + pass_noNaN + pass_peak + pass_blockTime stability across the bass envelope. After polynomial lands, restore `pass_breathingAudible ≥ 20%` (architecture-spec'd, RESEARCH §16.7.2) at the new operating-point baseline; re-baseline `--slow-lfo` golden against calibrated wedge (Phase 2.1c R19a / Phase 2.3 4-golden re-baseline precedent).

**In scope:**

- **`Source/BowedContrabassVoice.{h,cpp}`** — replace Phase 2.3 inline Schelleng wedge math (RESEARCH §16.3 closed-form `fMin = (Z²·v_b) / (2·R·β²·Δμ)`, `fMax = (2·Z·v_b) / (β·Δμ)` with `Z=R=R_s=0.5` collapse) with `safeDepth = schellengCalibration[stringIdx](v_b, F_bow, β)` table lookup. Per-string polynomial form: piecewise (or polynomial — research-phase to lock final form) coefficient array `constexpr float schellengCoeffs[4][N]` indexed by string. **HR-4 (Schelleng wedge skip on SLOW_LFO_DEPTH=0)** preserved verbatim — calibration polynomial is invoked ONLY when wedge gate fires (rawSlowLfoDepth > 0.0f), so strict E1 (`d358abcd…`) + Phase 2.3 modulators-off goldens reproduce byte-identically. `lastSafeDepth.store(...)` instrumentation hook signature unchanged.
- **`Source/DSP/SchellengCalibration.h`** (new) — header-only constexpr table + 4 lookup functions `safeDepthForStringE1/A1/D2/G2(v_b, F_bow, β) → float`. ~150 LOC. Per-string lookup table mirrors Phase 2.2 per-string dispersion M-table (M=4/3/2/1) precedent. NOT extracted to shared module (per-plugin per-string bass-register-specific).
- **`tests/render-harness/main.cpp`** — add CLI flag `--matrix-stability` activating 108-combo render mode. Iterates 4 strings × {BOW_SPEED ∈ [0.05, 0.15, 0.5]} × {BOW_PRESSURE ∈ [1.0, 3.0, 7.0]} × {BOW_POSITION ∈ [0.05, 0.10, 0.20]} at INFINITE_SUSTAIN=1.0, SLOW_LFO_DEPTH=1.0, SLOW_LFO_RATE=0.3 Hz, sustain 5 s per combo. Emit JSON with `mode: "matrix-stability"`, per-combo `{stringIdx, bowSpeed, bowPressure, bowPosition, peak, rmsContinuity, blockTimeRatio, clampedDepthMean, pass_noNaN, pass_peak, pass_clickFree, pass_blockTime}`, plus aggregate `pass_all_108`. Schema mirrors existing `--detune-sweep` per-combo pattern.
- **`tests/render-harness/main.cpp`** — restore `--slow-lfo` mode `pass_breathingAudible` threshold from 5% (Phase 2.3 v1.0 soft-pass) to 20% (architecture-spec'd RESEARCH §16.7.2). One-line constant change.
- **`tests/render-harness/golden/`** — re-baseline `slow-lfo.{wav.sha256,json}` against post-calibration wedge (old `3768dd15…` retired; new sha256 captured in R34 atomic commit). Add new golden text file `matrix-stability.{wav.sha256,json}` (sha256 of concatenated 108-combo render output + JSON aggregate).
- **`Source/DSP/SchellengCalibration.h` polynomial coefficients** — derived offline from 108-combo render data via fitting workflow (Python/Octave/Excel external tool, research-phase to specify). Resulting constexpr arrays land in `SchellengCalibration.h`.

**Carry-forward goldens (regression bar invariant 1 of Gate 6a):**
- Strict E1 modulators-off (`d358abcd…`) — HR-1..HR-4 preserve bit-exact regardless of calibration polynomial because all modulators evaluate to literal-zero arithmetic at default values.
- Phase 2.2 detune-sweep-A (`5e31dad3…`) — wedge math never invoked (no slow-LFO).
- Phase 2.3 modulators-off renders (the 4 audible carry-forward goldens re-baselined in Phase 2.3 verify: string-A `c6755aa4…`, string-D `765b015e…`, string-G `0cd5cb0a…`, note-sequence `3ac3ccd0…`) — wedge math never invoked.
- Phase 2.3 `vibrato.{wav.sha256,json}` (`d7881ecf…`) — wedge math never invoked (SLOW_LFO_DEPTH=0 in vibrato mode).
- Phase 2.3 `schelleng-stress.{wav.sha256,json}` — **WILL change** because wedge math IS invoked at SLOW_LFO_DEPTH=1.0 + extreme bow params. Re-baseline alongside `slow-lfo` golden in R34 atomic commit; new sha256 captured.
- Phase 2.3 `macro-sweep.{wav.sha256,json}` — wedge math never invoked (SLOW_LFO_DEPTH=0 in macro-sweep mode).

**Out of scope (deferred to Phase 2.4b/2.4c, 2.5, 2.6, end-of-Stage-2):**
- Sub-harmonic bias (Phase 2.4b, fresh GSD cycle)
- Autocorrelator octave-rejection vibrato harness fix (Phase 2.4c)
- Saturator-tail O-Bowed comparison harness + saturator-choice decision (Phase 2.4c)
- Body resonator + bow noise (Phase 2.5)
- Master saturator/limiter, stereo width, microtonal, MPE (Phase 2.6)
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments (end-of-Stage-2 verify per locked decision)
- E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7) — separate concern; this Phase 2.4a touches Schelleng wedge friction-junction math, NOT dispersion-cascade `a(B, I)` math

---

## Requirements Confirmed (Phase 2.4a-relevant subsets of locked contracts)

- **DSP-08** (Slow Bow LFO 0.05–2 Hz, Schelleng-aware): Phase 2.3 implemented modulator + wedge math; current `clampedDepthMean=0.0` silences slow-LFO at bass register defaults (RESEARCH §16.3 anomaly). Phase 2.4a calibration polynomial restores audible breathing — `pass_breathingAudible ≥ 20%` becomes the audible regression bar.
- **QUAL-01** (no audible clicks during parameter sweeps, no artifacts at normal ranges): 108-combo matrix is the primary gate. All 108 combos must satisfy `pass_noNaN + pass_peak ≤ 1.0 + pass_clickFree (rmsContinuity ≥ 0.85) + pass_blockTime (ratio ≤ 5.0)`. Aggregate `pass_all_108` becomes Gate 6a invariant 4.
- **QUAL-02** (extreme drone settings remain musical): `--matrix-stability` includes extreme combos (BOW_PRESSURE=7.0 N + BOW_SPEED=0.05 m/s) at SLOW_LFO_DEPTH=1.0. Calibration polynomial must clamp depth at extremes (`safeDepth < 1.0` near wedge boundary) without driving to literal zero everywhere (which is the Phase 2.3 anomaly). Gate 6a invariant 4 catches both pathologies.
- **DSP-06** (Infinite Sustain control): smooth-sweep / click-free check against INFINITE_SUSTAIN=1.0 indirectly satisfied by `--matrix-stability` (all 108 combos render at INFINITE_SUSTAIN=1.0 sustain).
- **PERF-01** (no allocations in `processBlock`): SchellengCalibration.h is constexpr table lookup (4 polynomial evaluations per block, one per string but only active string's wedge math fires per HR-4 gate). Zero allocations.
- **PERF-02** (< 5% CPU on M1): polynomial evaluation is ~10 multiplies + adds per block. <0.05% CPU added on top of Phase 2.3 baseline.

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**

- All 29 APVTS parameter IDs, ranges, skews, defaults — `parameter-spec.md` (sha256:`77638e25…`, post Phase 2.3 R33 amendment for VIBRATO_DEPTH 12.0→0.0 + EXPRESSION_MACRO 0.50→0.0 default flips). Phase 2.4a does NOT amend Stage-1 contract.
- DSP architecture (`research/ARCHITECTURE.md`, sha256:`3cb26814…`) — F3 deviation (no in-loop DCB) + saturator-tail tracking carry forward; ARCHITECTURE amendment still deferred to end-of-Stage-2 verify.
- ROADMAP phasing (sha256:`106639f6…`).
- `modules/synthesis/bow-friction/` v1.0.0 (Phase 2.1b) — value-class deterministic; Phase 2.4a does NOT touch friction module surface.
- `Source/DSP/DispersionFilter.h` (Phase 2.1c, R20 commit `5759e5e`) — Phase 2.4a consumes verbatim; no edits.
- `Source/DSP/WaveguideString.{h,cpp}` topology + per-instance config surface (Phase 2.2, R26 commit `131c2c7`) — Phase 2.4a consumes verbatim; no edits.
- Phase 2.3 modulator-layer surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order / HR-1..HR-4) — Phase 2.4a swaps **only** the inline Schelleng wedge math at Step 2 of the 7-step order, behind the existing HR-4 gate. All other modulator code unchanged.
- E1 dispersion calibration polynomial (Phase 2.1c Risk #7) — out-of-scope; that's a separate `a(B, I)` cascaded-allpass concern, not friction-junction wedge math.

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**

- `juce::ScopedNoDenormals` at `processBlock` entry — already in place, unchanged.
- `juce::SmoothedValue<float, Linear>` chain on macro destinations + per-string detune — unchanged.
- `juce::dsp::DelayLine<float, Lagrange3rd>` per-sample `setDelaySamples()` — unchanged.

**Phase 2.4a-specific constraints:**

- **HR-2 + HR-4 preservation paramount.** SchellengCalibration polynomial is invoked ONLY inside the existing HR-4 `if (rawSlowLfoDepth > 0.0f)` gate. At SLOW_LFO_DEPTH=0, polynomial is never evaluated — `lastSafeDepth.store(0.0f)` runs unconditionally (pin #4) before the gate. HR-2 (slow-LFO literal-zero short-circuit + phase non-advance at zero depth) is preserved verbatim from Phase 2.3.
- **Per-string lookup table form.** 4 separate constexpr coefficient arrays indexed by string, mirroring Phase 2.2 per-string dispersion M-table. Final polynomial degree + breakpoint count derived in research-phase from 108-combo data. Initial guess: 2-piece quadratic per axis, ~20 coefficients per string × 4 strings = 80 constexpr floats. Research-phase to confirm.
- **108-combo matrix dual-purpose.** Single render serves both calibration coefficient fit AND QUAL-01 stability gate. Wall-clock ~9 min (108 × 5 s sustain ≈ 540 s render time + JSON serialisation). Per-combo render path mirrors existing `--detune-sweep` iteration pattern.
- **Re-baseline of `--slow-lfo` + `--schelleng-stress` goldens.** Phase 2.1c R19a / Phase 2.3 4-golden precedent. Old `3768dd15…` and current `--schelleng-stress` sha256 retired in R34 atomic commit; new sha256s captured against calibrated wedge. `pass_breathingAudible ≥ 20%` becomes the audible regression bar at the new baseline.
- **Strict bit-exact regression bar unchanged.** E1 (`d358abcd…`) + detune-sweep-A (`5e31dad3…`) + Phase 2.3 modulators-off goldens (`c6755aa4…`, `765b015e…`, `0cd5cb0a…`, `3ac3ccd0…`) + `vibrato.wav.sha256` (`d7881ecf…`) + `macro-sweep.wav.sha256` MUST reproduce byte-identically. HR-2 + HR-4 gates ensure SchellengCalibration polynomial is never executed in any of these renders.
- **No Stage-1 contract amendment.** parameter-spec.md sha256 `77638e25…` carries forward unchanged. STATUS.md `contract_checksums.parameter_spec` unchanged.
- **No ARCHITECTURE.md amendment.** Calibration polynomial is an implementation detail of the architecture-spec'd Schelleng wedge clamp — closed-form formula in §"Slow-Bow LFO" stays as the conceptual reference; per-string polynomial replaces the dimensionless collapse at implementation-time. ARCHITECTURE.md may receive a footnote at end-of-Stage-2 verify documenting the bass-register calibration; not in this Phase 2.4a scope.
- **108-combo matrix axes locked at discuss-phase** (see Approach Decisions Q15 below). Research-phase does NOT re-litigate axis values; research-phase derives polynomial form + fitting workflow.

**Working-tree starting state (locked from Phase 2.3 verify, R33 commit `af54571`):**

- `Source/BowedContrabassVoice.{h,cpp}` — 4-string voice with `std::array<WaveguideString, 4>`, vibrato section (active-string-only modulation per HR-1), Slow-Bow LFO with inline closed-form Schelleng wedge (HR-4 gated, RESEARCH §16.3 closed-form `Z=R=R_s=0.5` collapse — TO BE REPLACED in Phase 2.4a), 4× macro SmoothedValues (HR-3 IEEE 754 identity arithmetic), 7-step per-block evaluation order
- `Source/DSP/WaveguideString.{h,cpp}` — split-rail with M=4/3/2/1 dispersion (Phase 2.1c rev-3 + Phase 2.2 R26)
- `Source/DSP/DispersionFilter.h` (130 LOC) — public API consumed verbatim
- `Source/PluginProcessor.{h,cpp}` — 29 APVTS parameters incl. all 4 `DETUNE_*`, ACTIVE_STRINGS, all Phase 2.3 modulator/macro params (Stage 1 + Phase 2.2 + Phase 2.3 R33)
- `modules/synthesis/bow-friction/` v1.0.0 — module is value-class deterministic
- 6 carry-forward goldens (E1 strict + detune-sweep-A + per-string A/D/G + note-sequence) + 4 Phase 2.3 mode goldens (vibrato + slow-lfo + schelleng-stress + macro-sweep). 2 of the 4 Phase 2.3 mode goldens (`slow-lfo` + `schelleng-stress`) re-baselined in this Phase 2.4a R34 atomic commit.

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q12 — Phase 2.4 cycle scope** | **Split 2.4a/2.4b/2.4c** | Heterogeneous 6-item bundle. 2.4a = friction-junction wedge math (Schelleng calibration + breathingAudible threshold + 108-combo matrix dual-purpose). 2.4b = sub-harmonic bias (independent DSP feature). 2.4c = harness/test infrastructure (autocorrelator fix + saturator-tail O-Bowed comparison). Mirrors Phase 2.1a/b/c precedent; each sub-phase is tight gate-first commit (R34/R35/R36). User-confirmed. |
| **Q13 — Schelleng calibration derivation** | **Empirical fit from 108-combo render** | Most rigorous; preserves architecture's safety intent. Couples cleanly with item Q15 (matrix is dual-purpose). Per-string `safeDepth(v_b, F_bow, β)` polynomial replaces the paper's piano-tuned `Z=R=R_s=0.5` closed-form collapse that produced `clampedDepthMean=0.0` at bass register defaults. User-confirmed. |
| **Q14 — Calibration polynomial form** | **Per-string lookup table** (4 polynomials, one per string E1/A1/D2/G2) | Mirrors Phase 2.2 per-string dispersion M-table (M=4/3/2/1) precedent. Each string has different impedance + register; per-string fit captures bass-vs-low-mid distinction. ~80 constexpr floats total. New header `Source/DSP/SchellengCalibration.h`. NOT extracted to shared module. User-confirmed. |
| **Q15 — 108-combo matrix axes** | **4 strings × 3 BOW_SPEED × 3 BOW_PRESSURE × 3 BOW_POSITION = 108** | Speed: {0.05, 0.15, 0.5} m/s (low / default / high). Pressure: {1.0, 3.0, 7.0} N (default / mid-stress / extreme). Position: {0.05, 0.10, 0.20} (sul tasto / default / sul ponticello). INFINITE_SUSTAIN=1.0 fixed, SLOW_LFO_DEPTH=1.0 fixed, SLOW_LFO_RATE=0.3 Hz fixed. Each combo renders 5 s sustain at MIDI {28,33,38,43} per string. Total ~9 min wall-clock. Pass: `pass_noNaN + pass_peak ≤ 1.0 + pass_blockTime ratio ≤ 5.0 + pass_clickFree (rmsContinuity ≥ 0.85)` for all 108. User-confirmed at discuss-phase (no research-phase re-litigation). |
| **Q16 — Matrix render dual-purpose** | **Single render serves calibration + stability gate** | 108-combo render in 2.4a serves BOTH calibration polynomial coefficient fit AND QUAL-01 stability evidence. 2.4a Gate 6a invariants include the matrix pass set. 2.4c reduces to [autocorrelator harness fix + saturator-tail O-Bowed comparison]. Lower wall-clock cost; couples calibration evidence with stability evidence in same artefact. User-confirmed. |
| **Q17 — `pass_breathingAudible` threshold** | **Restore to 20%** (architecture-spec'd, RESEARCH §16.7.2) | Phase 2.3 softened to 5% as v1.0 soft-pass. After calibration polynomial lands, restore architecture-spec'd 20%. Locked target — if calibration polynomial misses, that's a Phase 2.4a verify failure, not a threshold-relaxation event. User-confirmed. |
| **Q18 — `--slow-lfo` golden strategy** | **Re-baseline against calibrated wedge** | Phase 2.1c R19a / Phase 2.3 4-golden precedent. Old `3768dd15…` retired; new sha256 captured against post-calibration wedge with `pass_breathingAudible ≥ 20%`. `--schelleng-stress` golden also re-baselines (wedge math IS invoked at SLOW_LFO_DEPTH=1.0). All other goldens (E1 strict + detune-sweep-A + per-string + note-sequence + vibrato + macro-sweep) unchanged because HR-2/HR-4 gates ensure wedge math never executes. User-confirmed. |
| **Q19 — Atomic commit strategy** | **R34 atomic commit on Gate 6a PASS** | Continues sequence: R7 → R15 → R20 → R26 → R33 → R34. Lands ~10-15 files (2 source: BowedContrabassVoice.{h,cpp} + 1 new header SchellengCalibration.h + harness CLI flag + 2 re-baselined golden text + 1 new matrix-stability golden text + planning artefacts + STATUS.md). Phase 2.4b/2.4c get separate R35/R36 atomic commits. User-confirmed. |
| **Q20 — Saturator-tail decision tree** | **Defer branch decision to research-phase** (Phase 2.4c) | NOT in this Phase 2.4a scope. Phase 2.4c discuss-phase opens after 2.4a verifies; that cycle's research-phase §17 evaluates O-Bowed comparison data + recommends (i) swap to tanh / (ii) keep current / (iii) move out-of-loop. User-confirmed (deferred to 2.4c). |
| **Q21 — Logic AU smoke for 2.4a** | **Deferred non-blocking** (R32 / R27 / R19f precedent) | Automated Gate 6a invariants (bit-exact strict + modulators-off goldens + matrix pass set + new `--slow-lfo` golden + `pass_breathingAudible ≥ 20%`) gate the verify. Logic AU smoke is user-discretion non-blocking (5 s slow-LFO audition at MIDI 33 confirms audible breathing). User-confirmed. |
| **Q22 — CONTEXT.md doc scope** | **rev-6 covers Phase 2.4a only** | Phase 2.4b (sub-harmonic) gets rev-7 when discuss-phase opens after 2.4a verify. Phase 2.4c (autocorrelator + saturator) gets rev-8 when discuss-phase opens after 2.4b verify. Mirrors rev-2/rev-3/rev-4 (Phase 2.1a/b/c) precedent. Each rev is one tight cycle. User-confirmed. |
| Per-block evaluation order | **Unchanged from Phase 2.3** (7-step locked) | Step 2 (Schelleng wedge) HR-4 gate fires polynomial lookup instead of closed-form math. All other 6 steps unchanged. |
| Polynomial fitting workflow | **External offline tool** (research-phase to specify Python/Octave/Excel pipeline) | 108-combo render emits per-combo `clampedDepthMean` + `peakPostMaster` + stability data as JSON; offline tool ingests JSON, fits per-string polynomial, emits `constexpr float[]` coefficient arrays as `.h` snippet for paste into SchellengCalibration.h. Research-phase finalises tool choice + fit method (least-squares polynomial regression, B-spline, piecewise quadratic, etc.) |

---

## Open Questions (handed to research-phase)

1. **Polynomial form + degree.** Per-string lookup table (Q14 locked). Open: degree of polynomial per axis (linear, quadratic, cubic), breakpoint structure (single polynomial vs piecewise), interpolation method between breakpoints (linear, smoothstep, B-spline). Research-phase derives from inspection of 108-combo data scatter. Initial guess: 2-piece quadratic per axis with linear interpolation = ~20 coefficients per string × 4 strings = 80 constexpr floats.

2. **Polynomial fitting tool + workflow.** Q14 locks per-string table form; Q19 locks external offline tool. Open: which tool (Python `numpy.polyfit` / `scipy.optimize.curve_fit` / Octave `polyfit` / Excel `LINEST`)? What input (108-combo JSON parsed how)? What output format (raw `.h` snippet ready for paste, or `.json` consumed by build-time codegen)? Research-phase recommends tool + commits the offline workflow as a `tools/schelleng-fit/` script.

3. **108-combo render output schema.** `--matrix-stability` mode emits per-combo `{stringIdx, bowSpeed, bowPressure, bowPosition, peak, rmsContinuity, blockTimeRatio, clampedDepthMean, pass_*}`. Open: precise field names + JSON aggregate schema. Research-phase finalises against existing `--detune-sweep` per-combo pattern. Also: does aggregate include polynomial-fit-quality metrics (R², residual variance) computed inline, or is fit-quality assessed only by offline tool?

4. **Fit quality acceptance criteria.** Open: what R² / residual-variance thresholds gate the polynomial as "good enough" to ship? Research-phase derives from inspection of 108-combo data variance. Conservative default: R² ≥ 0.90 per string, max residual ≤ 0.10 in safeDepth space.

5. **Bit-exact regression pre-flight.** Phase 2.3 verify discovered post-R31 source edit caused 4 audible carry-forward goldens to drift bit-for-bit. Mitigation: pre-flight render BEFORE Phase 2.4a source edits captures all 8 currently-committed goldens (E1 strict + detune-sweep-A + per-string A/D/G + note-sequence + vibrato + macro-sweep — the 6 NOT being re-baselined in 2.4a) at current source. If any drift, escalate to bisection BEFORE Phase 2.4a source edits begin. Research-phase runs this pre-flight as RESEARCH §17.1.

6. **`--matrix-stability` MIDI note per combo.** Each combo runs at sustain 5 s for one string. Open: which MIDI note per string? Recommend: open string MIDI notes (28/33/38/43) so combo data reflects open-string Schelleng wedge per string. Alternative: middle-of-range per string (E.g. 30/35/40/45). Research-phase locks.

7. **Wedge math invocation in `--matrix-stability`.** Research-phase confirms: at SLOW_LFO_DEPTH=1.0 + SLOW_LFO_RATE=0.3 Hz, does the 5 s sustain provide enough cycles for `clampedDepthMean` to be representative? 0.3 Hz × 5 s = 1.5 cycles. Borderline. Alternative: SLOW_LFO_RATE=1.0 Hz (5 cycles per combo, wider sample of slow-LFO phase coverage). Research-phase checks clampedDepthMean variance against cycle count.

8. **Wall-clock budget validation.** 108 combos × 5 s sustain = 540 s render time + JSON serialisation overhead. Block-time ratio ≤ 5.0 means ~2700 s wall-clock = 45 min worst case (vs ~9 min initial estimate at typical block-time ratio ≤ 1.0). Research-phase pre-flights one combo to measure actual block-time ratio at extreme settings; revisits matrix iteration count if ratio is >2x estimate.

9. **Matrix pass criteria threshold tuning.** Currently `pass_clickFree` rmsContinuity ≥ 0.85, `pass_blockTime` ratio ≤ 5.0. Open: are these thresholds tight enough at extreme combos (BOW_PRESSURE=7.0 + BOW_SPEED=0.05 + SLOW_LFO_DEPTH=1.0)? Research-phase pre-flights extreme combo + checks observed values.

10. **`SchellengCalibration.h` API shape.** Open: 4 separate functions `safeDepthForStringE1/A1/D2/G2(v_b, F_bow, β)` vs single function `safeDepthForString(stringIdx, v_b, F_bow, β)` with internal table dispatch? Recommend: single function with `constexpr` table dispatch — cleaner call site in BowedContrabassVoice (`schellengCalibration::safeDepthForString(activeStringIndex, v_b, F_bow, β)`). Research-phase confirms.

11. **Pattern-confirm against O-Bowed.** O-Bowed has its own slow-LFO + Schelleng wedge math (architecture-derived from same paper). Confirm: does O-Bowed exhibit the same `clampedDepthMean=0.0` anomaly at default treble register, or is the bass-register-only? If O-Bowed is unaffected, calibration is bass-specific (good — confirms per-string-table approach). If O-Bowed also exhibits the anomaly, the closed-form is broken everywhere and we may want a shared module. Research-phase checks.

---

## Risks (Phase 2.4a-specific)

1. **Bit-exact regression failure on the 6 modulators-off / no-wedge goldens.** Mitigation: pre-flight (Open Question #5) captures all 8 currently-committed goldens BEFORE Phase 2.4a source edits. HR-2 + HR-4 gates are the technical defence — wedge math never executes in any of these renders. If regression breaks, it means Phase 2.4a source edits perturbed source structure in an unrelated way (e.g., variable layout, optimisation inlining). Escalate by isolating SchellengCalibration.h source to a separate translation unit; or by adding `__attribute__((noinline))` to polynomial lookup; or by rolling back to Phase 2.3 source structure with calibration as a pure constexpr swap-in.

2. **Calibration polynomial under-fits at extreme combos.** Mitigation: research-phase Open Question #4 specifies fit-quality acceptance criteria. If R² < 0.90 at any string, escalate to higher-degree polynomial or piecewise-with-more-breakpoints. Worst case: heuristic floor fallback (Q13 alternate) for any string where polynomial under-fits.

3. **Calibration polynomial over-fits at the 27 sampled grid points.** Risk of polynomial doing fine at the 27 (3×3×3) sampled combos per string but pathological at off-grid points. Mitigation: research-phase Open Question #4 includes off-grid spot-checks in fit-quality acceptance — sample at midpoints between grid points, verify polynomial is monotonic and bounded.

4. **108-combo wall-clock budget overrun** (Open Question #8). Mitigation: research-phase pre-flights one combo at extreme settings to measure actual block-time ratio. If 9 min estimate is >2x off, either (a) reduce matrix to 81 combos (3×3×3×3) OR (b) parallelise harness invocation across combos (each combo is independent process invocation).

5. **`pass_breathingAudible ≥ 20%` polynomial fit fails on one or more strings.** If calibration polynomial allows audible breathing on E1/A1/D2 but not G2 (or vice versa), Phase 2.4a verify fails. Mitigation: per-string polynomial form (Q14) means per-string fit quality is independently tunable. If one string under-fits, Phase 2.4a remediation path is to re-fit that string with higher polynomial degree (or piecewise-with-more-breakpoints) without disturbing others.

6. **`--matrix-stability` discovers a real instability** (NaN, peak > 1.0, click) at one or more of the 108 combos. This is the `pass_all_108` failure mode. Mitigation: this is a feature, not a bug — matrix is the QUAL-01 click-free / stability gate. Failure escalates to root-cause analysis (which combo, which DSP path, why instability). Phase 2.4a remediation may involve friction-junction guards (algebraic saturator clamp tightening, energy-clamp loop-gain reduction at extremes). Worst case: Phase 2.4a scope expands or 2.4a delivers polynomial only + 2.4a-bis cycle handles instability remediation.

7. **Polynomial fitting tool dependency.** Adding a Python/Octave tool to repo introduces a build-time-but-not-runtime dependency. Mitigation: tool runs offline (developer machine), not in CI. `tools/schelleng-fit/` script + commit-time README explaining how to re-run if matrix is re-rendered. Output (`.h` snippet) is committed to source; tool only re-invoked if calibration data changes.

8. **`--schelleng-stress` golden re-baseline introduces second uncharacterised drift mechanism.** Phase 2.3 verify already had 4 carry-forward goldens drift bit-for-bit (latent drift mechanism uncharacterised). Phase 2.4a re-baselines `--slow-lfo` + `--schelleng-stress` against calibrated wedge — these are EXPECTED to change because wedge math changes. Mitigation: NEW post-calibration sha256s captured in R34 atomic commit; documented as expected re-baseline (not anomalous drift). Phase 2.3's 4 carry-forward goldens (string-A/D/G/note-sequence) MUST reproduce byte-identically against current `c6755aa4…` etc.; if any drift, escalate (Phase 2.3 latent drift mechanism re-surfacing).

9. **Polynomial coefficient floats in source vs header.** Constexpr arrays in header introduce ODR risk if header is included in multiple translation units. Mitigation: header guards + `inline constexpr` C++17 syntax. Phase 2.2 dispersion table uses same pattern; precedent confirmed.

---

## Next Phase

Ready for: **research** phase — `/clear` then `/plugin-research O-Contrabass 2-dsp`

Research focus (Phase 2.4a):

1. **Resolve Open Questions #1–#11** — polynomial form/degree, fitting tool/workflow, JSON schema, fit quality acceptance, bit-exact pre-flight (RESEARCH §17.1), MIDI note per combo, wedge cycle count adequacy, wall-clock budget validation, matrix pass-criteria thresholds, SchellengCalibration.h API shape, O-Bowed pattern confirm.
2. **Bit-exact pre-flight (Open Question #5)** — render all 8 currently-committed goldens BEFORE Phase 2.4a source edits; capture sha256s + verify against committed values. If any drift, INVESTIGATE before plan-phase. This is the equivalent of Phase 2.3 RESEARCH §16.1 pre-flight that discovered `d358abcd…` was byte-identical with EXPRESSION_MACRO default flipped.
3. **108-combo single-combo wall-clock pre-flight (Open Question #8)** — render ONE combo at extreme settings (BOW_PRESSURE=7.0, BOW_SPEED=0.05, SLOW_LFO_DEPTH=1.0, MIDI 28); measure block-time ratio + wall-clock per combo. Extrapolate to 108-combo total wall-clock; revisit matrix size if budget overrun.
4. **Polynomial fitting tool selection** — Python `numpy.polyfit` is the default recommendation; research-phase commits `tools/schelleng-fit/fit.py` (or equivalent) + README + sample output.
5. **Pattern-confirm against O-Bowed** Schelleng wedge implementation; document whether bass-register anomaly is shared across plugins.
6. **Update RESEARCH.md** — append §17 documenting all the resolutions above. (No §16 changes; Phase 2.3 §16 is locked.)

After research: plan-phase (PLAN rev-8) writes R34 task breakdown verbatim against this CONTEXT + research findings; execute-phase performs the implementation + 108-combo render + polynomial fit + R34 atomic commit; verify-phase confirms Gate 6a invariants (1–5) + R37 Logic AU smoke (deferred non-blocking per Q21).

---

## Audit Trail (rev-6 supersedes rev-5)

**rev-1 (earlier 2026-04-26):** Phase 2.1 broad discuss. Cycle scope = Phase 2.1 (sub-phases a/b/c).

**rev-2 (later 2026-04-26):** Phase 2.1a closure (Option A, R7 commit) + Phase 2.1b opening (module extraction, Gate 2). Phase 2.1b verified 2026-04-27 (R8a `bd5fae0` + R15 `ef0604d` atomic commits, Gate 2 PASS bit-exact).

**rev-3 (2026-04-27):** Phase 2.1c opening — cascaded allpass dispersion, Gate 3. Phase 2.1c verified 2026-04-27 (R20 atomic commit `5759e5e`, Gate 3 PASS).

**rev-4 (2026-04-27):** Phase 2.2 opening — 4-string EADG bank + per-string detune + per-string M=4/3/2/1 dispersion table + MIDI→string mapping + ACTIVE_STRINGS + 5 ms string-switching crossfade. Phase 2.2 verified 2026-04-27 (R26 atomic commit `131c2c7`, Gate 4 PASS).

**rev-5 (2026-04-27):** Phase 2.3 opening — Vibrato + Slow-Bow LFO + Schelleng wedge clamp + EXPRESSION_MACRO. 11 approach decisions (Q1–Q11), 4 hard rules HR-1..HR-4 binding. Phase 2.3 verified 2026-04-27 (R33 atomic commit `af54571`, Gate 5 PASS with re-baseline of 4 audible carry-forward goldens).

**rev-6 (this document, 2026-04-27):** Phase 2.4a opening — Schelleng wedge bass-register calibration polynomial + 108-combo stability matrix dual-purpose render + `pass_breathingAudible` 5%→20% threshold restoration. 11 approach decisions (Q12–Q22 user-confirmed: split 2.4a/2.4b/2.4c; empirical fit from 108-combo render; per-string lookup table polynomial form; 4×3×3×3 matrix axes locked at discuss-phase; single render dual-purpose; 20% threshold restoration; re-baseline `slow-lfo` + `schelleng-stress` goldens; R34 atomic commit; saturator-tail decision deferred to Phase 2.4c; Logic AU smoke deferred non-blocking; rev-6 covers 2.4a only). 11 open questions handed to research-phase: polynomial form/degree, fitting tool/workflow, JSON schema, fit quality criteria, bit-exact pre-flight (RESEARCH §17.1), MIDI note per combo, wedge cycle count, wall-clock budget, matrix pass thresholds, SchellengCalibration.h API, O-Bowed pattern confirm.

**Inherited verbatim from rev-5 (not re-litigated):**
- All Phase 2.3 modulator surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order)
- HR-1..HR-4 hard rules (literal-zero short-circuits + IEEE 754 identity-arithmetic + Schelleng skip on zero LFO depth)
- `lastSafeDepth.store(...)` instrumentation hook signature (pin #4 from PLAN rev-7)
- 6 carry-forward goldens (E1 strict + detune-sweep-A + per-string A/D/G + note-sequence)
- 4 Phase 2.3 mode goldens (vibrato + slow-lfo + schelleng-stress + macro-sweep) — `slow-lfo` + `schelleng-stress` re-baseline in 2.4a R34
- Atomic-commit gate-first principle (R7 → R15 → R20 → R26 → R33 → R34)
- Saturator-tail Phase 2.4c follow-up parking + RESEARCH §12 footnote (now scoped to 2.4c, not 2.4a)
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify
- E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7) — separate concern from Phase 2.4a Schelleng wedge calibration; that's `a(B, I)` cascaded-allpass math, this is friction-junction wedge math
- Primary listening DAW: Logic Pro (AU)
- Sample-rate strategy: internal 88.2 / 96 kHz at friction junction
- Bow-friction module v1.0.0 at `modules/synthesis/bow-friction/` (Phase 2.1b)
- Per-plugin `DispersionFilter.h` (NOT extracted to shared module)
- 29 APVTS parameters incl. all Phase 2.3 modulator/macro params; parameter-spec.md sha256:`77638e25…` carries forward unchanged
- Stage-1 contract NOT amended in Phase 2.4a (no parameter-spec.md edit, no contract_checksums update)

**New in rev-6:**
- Q12 Phase 2.4 split into 2.4a/2.4b/2.4c sub-cycles (mirrors 2.1a/b/c)
- Q13 empirical fit from 108-combo render (most rigorous)
- Q14 per-string lookup table polynomial form (4 polys, mirrors per-string M-table)
- Q15 108-combo matrix axes locked: 4 strings × 3 BOW_SPEED × 3 BOW_PRESSURE × 3 BOW_POSITION at INFINITE_SUSTAIN=1.0, SLOW_LFO_DEPTH=1.0, SLOW_LFO_RATE=0.3 Hz
- Q16 single 108-combo render dual-purpose (calibration + stability gate)
- Q17 `pass_breathingAudible` 5%→20% threshold restoration (architecture-spec'd)
- Q18 re-baseline `slow-lfo` + `schelleng-stress` goldens against calibrated wedge
- Q19 R34 atomic commit on Gate 6a PASS (continues R7→R15→R20→R26→R33→R34)
- Q20 saturator-tail decision deferred to Phase 2.4c
- Q21 Logic AU smoke deferred non-blocking (R32 / R27 / R19f precedent)
- Q22 rev-6 covers Phase 2.4a only (rev-7 / rev-8 written when 2.4b / 2.4c open)
- New `Source/DSP/SchellengCalibration.h` header with per-string constexpr coefficient arrays
- New harness CLI flag `--matrix-stability` for 108-combo render
- New golden text file `matrix-stability.{wav.sha256,json}`
- Five-item Gate 6a bar: (1) all 8 carry-forward goldens (E1 strict + detune-sweep-A + per-string A/D/G + note-sequence + vibrato + macro-sweep) byte-identical; (2) re-baselined `--slow-lfo` golden + `pass_breathingAudible ≥ 20%`; (3) re-baselined `--schelleng-stress` golden; (4) `--matrix-stability` `pass_all_108=true`; (5) auval + pluginval-10. R37 Logic AU smoke deferred non-blocking.
