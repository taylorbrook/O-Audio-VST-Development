# Stage 2: DSP — Context (rev-8)

**Date:** 2026-04-28
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.4c — Autocorrelator octave-rejection harness fix + saturator-tail O-Bowed comparison (research-only)**
**Supersedes:** rev-7 (Phase 2.4b — Sub-Harmonic Bias DSP-07 per ARCHITECTURE §457, dated 2026-04-28). rev-7 contracts that remain locked are inherited verbatim and not re-litigated. Phase 2.4b closed 2026-04-28 with R35 atomic commit (`3de8b66`, Gate 6b CLEARED — 4 strict-PASS + 1 soft-PASS within RESEARCH §18.6 v1.0 budget) + R35-backfill chore (`0db5fac`).

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle opens **Phase 2.4c** — third (and final) of three Phase 2.4 sub-cycles (rev-6 Q12 split decision). Phase 2.4c scope = **paired audit-debt closeouts**: (a) restore strict `pass_vibratoAudible` invariant by fixing the bass-register autocorrelator octave-jump that forced Phase 2.3 R28 to relax the gate; (b) characterise O-Contrabass's algebraic-saturator (`x/sqrt(1+x²)`) tail-decay envelope versus O-Bowed's choice via a new `--saturator-tail-comparison` harness mode. Both items are **harness-only / research-only by default**; production DSP source is untouched. Cross-comparison verdict in RESEARCH §19 either (a) acknowledges divergence as a Phase 2.5-aware architectural decision (no source change) or (b) escalates to a Phase 2.4c-bis source-change cycle if O-Bowed parity deficit is unambiguous (≥ 2 dB envelope divergence at 5 s mark default — research-phase tunes the threshold against measured data).

This is the **lowest-risk closer of audit-debt before Phase 2.5** (body resonator + bow noise) lands and changes the saturator-tail decay envelope. Phase 2.4-bis backlog (kForceBoost retune, breathingAudible metric refinement, fallback-cell reduction) keeps as a separate future cycle. After Phase 2.4c verifies (Gate 6c PASS), Phase 2.5 discuss-phase opens with fresh CONTEXT rev-9.

---

## Cycle Scope

**Goal:** Convert the Phase 2.3 `pass_vibratoAudible` soft-relaxation into a strict-PASS by fixing the harness-side autocorrelator that octave-jumped at bass register (period ≈ 1070 samples at sr=44100, vibrato modulation ~0.7% requires sub-sample lag precision); AND produce an O-Bowed-comparable tail-decay characterisation of O-Contrabass's algebraic in-loop saturator (`x / sqrt(1 + x²)`) so the architectural decision to retain it (vs port O-Bowed's choice) is evidence-backed before Phase 2.5 changes the body-side defenses. **Zero production DSP edits**: all 12 carry-forward goldens reproduce byte-identically by construction (HR-11 trivially holds). One golden re-baseline (`vibrato.json{,.sha256}`) and three new goldens (`saturator-tail-comparison.{wav.sha256,json,json.sha256}`) land in R36 atomic.

**In scope:**

- **`tests/render-harness/main.cpp`** — autocorrelator math fix in the existing `--vibrato` mode pitch-tracker. Replace integer-lag peak-search with **(a) parabolic interpolation around lag peak (sub-sample precision)** + **(b) lag-search range bias toward MIDI-derived expected period** (e.g., MIDI 28 / E1 → expected period ≈ 1070 samples at sr=44100; search range ±20% → [856, 1284]). Restores strict `pass_vibratoAudible` gate per Phase 2.3 PLAN rev-7 design intent: rate ∈ [4.5, 5.5] Hz, depth ∈ [10, 14]¢, onsetWindow ∈ [800, 1000] ms. Research-phase locks the precise algorithm (parabolic-interp + range-bias default; YIN / AMDF / cepstrum as fallbacks if pre-flight reveals sub-sample precision is insufficient).
- **`tests/render-harness/main.cpp`** — NEW CLI flag `--saturator-tail-comparison` activating canonical decay-envelope mode: render MIDI 28 (E1 open) at default bow params (BOW_SPEED=0.15, BOW_PRESSURE=3.0, BOW_POSITION=0.10) with INFINITE_SUSTAIN=1.0, sustain 60 s + release 5 s (mirrors Phase 2.1a R6 protocol). Capture per-decade RMS bins {0–1s, 1–2s, ..., 59–60s, 60–61s, ..., 64–65s} as a 65-element decay envelope. Emit JSON with `{mode: "saturator-tail-comparison", peak, decayEnvelopeDb: [-X, ...], rmsMid, rmsFinal, rmsRatioFinalOverMid, blockTimeRatio, pass_noNaN, pass_peak, pass_blockTime, pass_combo}`. NO `pass_decayMatchesOBowed` predicate at v1.0 — comparison verdict happens in RESEARCH §19, not as a JSON gate. Schema mirrors existing `--sub-harmonics` per-mode pattern.
- **`tests/render-harness/golden/vibrato.{json,json.sha256}`** — re-baseline JSON only (not WAV). Autocorrelator fix changes measurement output but not WAV (DSP unchanged → `vibrato.wav.sha256 = d7881ecf…` carries forward byte-identical). Updated JSON includes restored strict-gate values for `rateHzInRange`, `depthCentsInRange`, `onsetWindow` predicates.
- **`tests/render-harness/golden/saturator-tail-comparison.{wav.sha256,json,json.sha256}`** — NEW golden text files; canonical reference for the decay envelope. Wall-clock ~0.5 s for 65 s of audio at default settings.
- **`tests/render-harness/reproduce-goldens.sh`** — extend 12 → 13 entries for `--saturator-tail-comparison` invocation.
- **`RESEARCH.md` §19** — document (a) autocorrelator algorithm validation (sub-sample precision benchmark on `vibrato.wav`; octave-jump elimination); (b) O-Bowed saturator topology audit (grep / inspection of `plugins/O-Bowed/Source/DSP/WaveguideString.{h,cpp}` and friction junction); (c) O-Bowed cross-comparison harness availability + parity-mode invocation; (d) measured tail-decay divergence at canonical render; (e) verdict (research-only acknowledged divergence vs Phase 2.4c-bis escalation).
- **`STATUS.md`** — flip `status` to `phase_2_4c_discuss_complete`, `next_action` to `phase_2_4c_research`, add `phase_2_4c_discuss_carry_forward` block.

**Out of scope (deferred elsewhere):**

- **Source-change saturator port from O-Bowed** — escalation-only path; default Phase 2.4c verdict is research-only / acknowledged divergence per Q36. If RESEARCH §19 measures > 2 dB envelope divergence at 5 s mark (default threshold; research-phase tunes against measured data), escalate to **Phase 2.4c-bis** source-change cycle (separate CONTEXT rev-9-bis after Phase 2.4c verify).
- **Phase 2.4-bis backlog** — kForceBoost retune (0.8 → ~1.0) to push subharmEnergyRatio above 0.40 strict; tune Step 4 modulation gain or refine breathingAudible metric to hit 20% peak-to-peak; reduce 3 v1.0 fallback cells via downstream-defense tightening. Separate Phase 2.4-bis cycle after Phase 2.4c.
- **Phase 2.5** — body resonator + bow noise.
- **Phase 2.6** — master saturator/limiter, stereo width, microtonal, MPE.
- **Chaos detector + softClampState** — Phase 2.5/2.6 (per Phase 2.4b R35 commit-body footnote).
- **ARCHITECTURE.md amendments** — §"DC Blocker" + §"In-loop saturator" deferred to end-of-Stage-2 verify (carry-forward from rev-2/3/4/5/6/7).
- **E1 dispersion calibration polynomial** (Phase 2.1c Risk #7) — separate concern.
- **Logic AU smoke** — deferred non-blocking R37/R32/R27/R19f/R14e precedent (Q43).
- **YIN / AMDF / cepstrum autocorrelation** — fallback-only path; default is parabolic-interp + range-bias per Q37. Research-phase escalates only if pre-flight reveals sub-sample precision is insufficient.

---

## Requirements Confirmed (Phase 2.4c-relevant subsets of locked contracts)

- **DSP-09 / Layered Expression** (vibrato section): Phase 2.4c restores the strict `pass_vibratoAudible` measurement gate that Phase 2.3 R28 soft-relaxed because the bass-register autocorrelator octave-jumped. Production vibrato DSP is unchanged (Phase 2.3 R29 implementation remains the authoritative source). Promotion to "complete" in REQUIREMENTS.md held until end-of-Stage-2 verify.
- **DSP-01 / Bass-Range Waveguide Stability**: Phase 2.4c characterises the saturator-tail decay envelope at canonical E1 60s+5s. Research-only verdict either confirms current algebraic saturator is the correct architectural choice (no source change) or escalates to Phase 2.4c-bis source-change cycle.
- **QUAL-01** (no audio artifacts at normal ranges): no new gates introduced; existing 12 carry-forward goldens (10 from Phase 2.4a + 2 new from Phase 2.4b) MUST reproduce byte-identically (HR-11 trivially holds because no production DSP edits).
- **PERF-01** (no allocations in `processBlock`): no production DSP edits; carry-forward verbatim.
- **PERF-02** (< 5% CPU on M1): no production DSP edits; carry-forward verbatim.

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**

- All 29 APVTS parameter IDs, ranges, skews, defaults — `parameter-spec.md` (sha256:`77638e25…`). **NO Stage-1 contract amendment in Phase 2.4c.**
- DSP architecture (`research/ARCHITECTURE.md`, sha256:`3cb26814…`). **NO ARCHITECTURE.md amendment in Phase 2.4c.** §"DC Blocker" + §"In-loop saturator" amendments still deferred to end-of-Stage-2 verify; Phase 2.4c saturator-tail comparison feeds the end-of-Stage-2 amendment evidence base but does not amend the architecture itself.
- ROADMAP phasing (sha256:`106639f6…`).
- `modules/synthesis/bow-friction/` v1.0.0 (Phase 2.1b) — value-class deterministic; **Phase 2.4c does NOT touch friction module surface**.
- `Source/DSP/DispersionFilter.h` (Phase 2.1c, R20 commit `5759e5e`) — verbatim consume.
- `Source/DSP/WaveguideString.{h,cpp}` (Phase 2.2, R26 commit `131c2c7`) — verbatim consume.
- `Source/DSP/SchellengCalibration.h` (Phase 2.4a, R34 commit `4c926bb`) — verbatim consume.
- `Source/DSP/SubHarmonicBias.h` (Phase 2.4b, R35 commit `3de8b66`) — verbatim consume.
- `Source/BowedContrabassVoice.{h,cpp}` (Phase 2.4b end-state) — verbatim consume; **zero production DSP edits in Phase 2.4c**.
- `Source/PluginProcessor.{h,cpp}` — 29 APVTS params + Phase 2.4b voice wiring; verbatim consume.
- Phase 2.3 modulator surface + 7-step + Step 2.5 (Phase 2.4b) + HR-1..HR-10 verbatim carry-forward.

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):** unchanged. No new JUCE-side patterns introduced in Phase 2.4c.

**Phase 2.4c-specific constraints:**

- **HR-11 hard rule (NEW): zero production DSP edits.** Phase 2.4c modifies ONLY `tests/render-harness/main.cpp`, `tests/render-harness/reproduce-goldens.sh`, golden text files, and planning artefacts (CONTEXT/RESEARCH/PLAN/STATUS/SUMMARY/VERIFICATION). Any source edit under `plugins/O-Contrabass/Source/` (including `Source/DSP/*`) is a HARD violation requiring escalation to Phase 2.4c-bis. HR-11 is the technical defence guaranteeing all 12 carry-forward goldens reproduce byte-identically by construction (no IEEE 754 identity arithmetic gymnastics needed; the WAV-producing code paths are not touched).
- **Autocorrelator fix is harness-side only.** The autocorrelator lives in `tests/render-harness/main.cpp` `--vibrato` mode pitch-tracker (Phase 2.3 §16.7 / R28 §"4 harness modes"). The fix changes measurement output but not WAV output: `vibrato.wav.sha256 = d7881ecf…` carries forward byte-identical; only `vibrato.json` shifts (autocorrelator now reports correct fundamental period rather than octave-jumped half-period; rate / depth / onset metrics tighten back into Phase 2.3 design-intent strict ranges).
- **Saturator-tail comparison is research-only by default.** RESEARCH §19 documents the O-Bowed cross-comparison; verdict written there is **acknowledged divergence + Phase 2.5-awareness** unless measured envelope divergence at the 5 s mark exceeds the research-phase-tuned threshold (default 2 dB). If escalation triggers, a separate Phase 2.4c-bis cycle (CONTEXT rev-9-bis) opens with source-change scope; Phase 2.4c stays research-only.
- **O-Bowed harness availability is a research-phase audit prerequisite.** Plan-phase cannot lock R36 task breakdown until research-phase confirms whether O-Bowed has a comparable render harness (`plugins/O-Bowed/tests/render-harness/` candidate path or `modules/synthesis/bow-friction/` cohort harness path from Phase 2.1b precedent) AND what parity-mode invocation produces the canonical E1 60s+5s render at matched bow defaults. If unavailable, plan-phase decides between (a) drop O-Bowed comparison from Phase 2.4c → defer to Phase 2.4c-bis or (b) build a one-shot O-Bowed harness in R36b (scope expansion).
- **Strict bit-exact regression bar trivially preserved.** All 12 carry-forward WAVs (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep + slow-lfo + schelleng-stress + sub-harmonics + sub-harmonics-stability) MUST reproduce byte-identically. No HR-9-style identity-arithmetic pre-flight needed because no DSP source is touched; HR-11 is the technical defence.
- **NO Stage-1 contract amendment.** parameter-spec.md sha256 `77638e25…` carries forward unchanged. STATUS.md `contract_checksums.parameter_spec` unchanged.
- **NO ARCHITECTURE.md amendment.** End-of-Stage-2 verify still owns §"DC Blocker" + §"In-loop saturator" amendments; Phase 2.4c saturator-tail evidence is fed forward as source data for that future amendment cycle.

**Working-tree starting state (locked from Phase 2.4b verify, R35 commit `3de8b66` + R35-backfill chore `0db5fac`):**

- All Phase 2.4b end-state source verbatim (BowedContrabassVoice, SubHarmonicBias.h, SchellengCalibration.h, WaveguideString, DispersionFilter, PluginProcessor).
- 12 currently-committed goldens: E1 strict `d358abcd…` + per-string A/D/G `c6755aa4…/765b015e…/0cd5cb0a…` + detune-sweep-A `5e31dad3…` + note-sequence `3ac3ccd0…` + vibrato `d7881ecf…` + macro-sweep `c2571dd9…` + slow-lfo `c0c2c893…` + schelleng-stress `9d18da86…` + sub-harmonics `bfcaaadc…` + sub-harmonics-stability `8043f659…`. Plus matrix-stability `6db67707…` (Phase 2.4a evidence carry-forward, not in default reproduce-goldens.sh).
- `reproduce-goldens.sh` (Phase 2.4a R34-pre infrastructure, extended Phase 2.4b R35 from 10→12 entries).
- `preflight-subharm.sh` (Phase 2.4b R35-pre HR-9 escalation gate).

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q34 — Phase 2.4c cycle scope** | **Autocorrelator harness fix + saturator-tail O-Bowed comparison paired** | User-confirmed Option A. Closes Phase 2.3 R28 audit-debt (relaxed `pass_vibratoAudible`) AND Phase 2.1a R6 audit-debt (saturator-tail decay characterisation deferred for O-Bowed cross-comparison) in a single low-risk cycle before Phase 2.5 lands body resonator (which alters tail-decay envelope). Phase 2.4-bis backlog stays as separate future cycle. |
| **Q35 — Pair both items in single R36 atomic commit** | **Pair (single R36 atomic + R36-backfill chore)** | User-confirmed. Both are harness-only with low coupling; single auval/pluginval pass; mirrors R34/R35 atomic-commit precedent. Splitting into R36 (autocorrelator) + R37 (saturator-tail) would double the gate-cycle overhead without coupling benefit. |
| **Q36 — Saturator-tail comparison default verdict path** | **Research-only by default; escalate to source-change Phase 2.4c-bis if envelope divergence > 2 dB at 5 s mark** | User-confirmed. Default v1.0 verdict is "acknowledged divergence + Phase 2.5-awareness" via RESEARCH §19. Escalation criterion: measured RMS envelope divergence between O-Bowed and O-Contrabass at the 5 s post-bow-off mark exceeds 2 dB (research-phase tunes the threshold against actual measured data). Source-change escalation = separate Phase 2.4c-bis cycle (CONTEXT rev-9-bis), NOT Phase 2.4c scope expansion. |
| **Q37 — Autocorrelator fix algorithm** | **Parabolic interpolation around lag peak + lag-search range bias toward MIDI-derived expected period** | User-confirmed (fix-autocorr). Default fix: (a) sub-sample precision via 3-point parabolic interpolation around the integer-lag peak; (b) constrain lag-search to ±20% of the MIDI-note-derived expected period (e.g., MIDI 28 / E1 → period ≈ 1070 samples at sr=44100; range [856, 1284]). Eliminates octave-jump pathology where autocorrelator latches onto period/2 or 2·period at harmonic-rich signals. YIN / AMDF / cepstrum reserved as fallback if research-phase pre-flight reveals parabolic-interp precision is insufficient at 12-cent vibrato depth (~0.7% period modulation). |
| **Q38 — Restore strict `pass_vibratoAudible` post autocorrelator fix** | **Restore Phase 2.3 PLAN rev-7 design-intent strict ranges** | Once autocorrelator no longer octave-jumps, the originally-designed strict ranges apply: rate ∈ [4.5, 5.5] Hz, depth ∈ [10, 14]¢, onsetWindow ∈ [800, 1000] ms. R36c re-baselines `vibrato.json{,.sha256}` only (WAV unchanged because DSP unchanged). Phase 2.3 R28 soft-relaxation retired. |
| **Q39 — Saturator-tail harness mode shape** | **NEW `--saturator-tail-comparison` flag emitting per-decade decay envelope JSON** | Single new CLI flag mirroring `--sub-harmonics` precedent. Renders canonical E1 60s+5s at default bow / INFINITE_SUSTAIN=1.0 (matches Phase 2.1a R6 protocol). Captures 65 per-second RMS bins as `decayEnvelopeDb: [...]`. NO `pass_decayMatchesOBowed` JSON predicate; comparison verdict is RESEARCH §19, not a golden gate. |
| **Q40 — O-Bowed cross-comparison protocol** | **Render canonical E1 60s+5s in O-Bowed via its render harness; capture decay envelope; document divergence in RESEARCH §19** | Research-phase audits whether O-Bowed has a comparable render harness (`plugins/O-Bowed/tests/render-harness/` or `modules/synthesis/bow-friction/` cohort path from Phase 2.1b precedent). If unavailable, plan-phase decides between dropping the comparison from Phase 2.4c (defer to 2.4c-bis with O-Bowed harness build) or scope-expanding R36b to build a one-shot O-Bowed harness. Default assumption: O-Bowed harness exists from Phase 2.1b cohort work. |
| **Q41 — Saturator-tail decision criterion** | **2 dB envelope divergence at 5 s mark = research-only verdict; > 2 dB = escalate to Phase 2.4c-bis** | Research-phase tunes the threshold against actual measured data (the 2 dB default is a pin, not a contract). Threshold sits below typical perceptual-just-noticeable-difference for sustained tones (~3 dB) so any escalation is conservative. |
| **Q42 — NEW Hard Rule HR-11** | **Zero production DSP edits in Phase 2.4c** | Phase 2.4c is harness-only / research-only by construction. Any edit under `plugins/O-Contrabass/Source/` is a HARD violation requiring escalation to Phase 2.4c-bis. HR-11 is the technical defence guaranteeing 12-of-12 WAV byte-identical regression bar without HR-9-style IEEE 754 gymnastics. |
| **Q43 — Logic AU smoke timing** | **Deferred non-blocking R37/R32/R27/R19f/R14e precedent** | User-confirmed. No DSP changes → no audible difference for AU smoke to detect. Logic AU smoke remains an end-of-Stage-2 user-discretion non-blocking item. |
| **Q44 — Atomic commit shape** | **R36 atomic + R36-backfill chore mirrors R34/R35 precedent** | User-confirmed. Continues atomic-commit sequence: R7 → R15 → R20 → R26 → R33 → R34 → R35 → **R36** (Phase 2.4c Gate 6c PASS). R36-backfill chore propagates R36 sha into STATUS.md per R34-backfill / R35-backfill precedent. |
| **Q45 — CONTEXT.md doc scope** | **rev-8 covers Phase 2.4c only** | Phase 2.5 (body resonator + bow noise) gets fresh CONTEXT rev-9 when its discuss-phase opens. Mirrors rev-2/3/4/5/6/7 precedent. |
| Per-block evaluation order | **Unchanged from Phase 2.4b end-state** | 7-step + Step 2.5 carry-forward verbatim. Phase 2.4c does NOT modify the per-block evaluation order. |

---

## Open Questions (handed to research-phase)

1. **Autocorrelator algorithm validation.** Pre-flight `--vibrato` mode against canonical `vibrato.wav` at HEAD (`5d95d15` descendant of R35-backfill `0db5fac`) BEFORE source edits — confirm current measurement IS octave-jumped (i.e., Phase 2.3 R28 documented behavior reproduces). Then implement parabolic-interp + range-bias fix in research-side prototype; measure (rate, depthCents, onset) on `vibrato.wav`; verify all three fall into Phase 2.3 PLAN rev-7 strict ranges (rate ∈ [4.5, 5.5] Hz / depth ∈ [10, 14]¢ / onset ∈ [800, 1000] ms). If sub-sample precision insufficient at 12-cent vibrato (~0.7% period modulation), escalate to YIN / AMDF / cepstrum (Open Q1-bis).

2. **Lag-search range bound spec.** Default ±20% of MIDI-note-derived expected period. For MIDI 28 / E1 (f0 = 41.2 Hz at sr = 44100): period = 1070.4 samples; range [856, 1284]. Open: should range bias be MIDI-derived or detected-string-derived (the latter handles per-string detune)? Recommend MIDI-derived for v1.0 (simpler; vibrato.wav uses MIDI 28 verbatim; per-string detune is a separate concern).

3. **O-Bowed saturator topology audit.** Grep `plugins/O-Bowed/Source/DSP/WaveguideString.{h,cpp}` and friction junction sources for in-loop nonlinearity vs output-only saturation. Document where O-Bowed applies saturation, what topology (algebraic vs `tanh` vs lookup), and what constants. Confirms or refutes the design assumption that O-Bowed's saturator is materially different from O-Contrabass's `x/sqrt(1+x²)`.

4. **O-Bowed render harness availability + parity-mode invocation.** Is there a `plugins/O-Bowed/tests/render-harness/` analogous to O-Contrabass's? Or does the Phase 2.1b cohort harness (`modules/synthesis/bow-friction/test-harness/`) cover the canonical E1 60s+5s scenario? Confirm existence + matching CLI invocation that produces the same render-rate / parameter / MIDI configuration as O-Contrabass `--saturator-tail-comparison`. If unavailable, escalate at plan-phase (Q40 alternatives).

5. **Saturator-tail measurement protocol.** 65 per-second RMS bins captured during 60 s sustain + 5 s release. Schema: `decayEnvelopeDb: [bin0_db, bin1_db, ..., bin64_db]` where each bin is `20 * log10(rms_per_second / rms_max)`. RMS computed per 1-second window non-overlapping. Open: sample-rate (44100 default; pin for cross-plugin parity); MIDI velocity (default 100; pin for parity); state-init (`processor.releaseResources(); processor.prepareToPlay(...)` per Phase 2.4b R35-pre determinism precedent).

6. **Saturator-tail divergence threshold tuning.** Default 2 dB at 5 s mark; research-phase reports actual measured divergence and either confirms 2 dB or proposes a tuned value. Below typical perceptual JND for sustained tones (~3 dB).

7. **Vibrato golden re-baseline scope.** ONLY `vibrato.json` + `vibrato.json.sha256` change (autocorrelator fix changes measurement output but not WAV). `vibrato.wav.sha256 = d7881ecf…` byte-identical (DSP unchanged). Confirm via R36-pre tripwire.

8. **Strict `pass_vibratoAudible` threshold values.** Phase 2.3 PLAN rev-7 design-intent ranges (rate ∈ [4.5, 5.5] Hz, depth ∈ [10, 14]¢, onset ∈ [800, 1000] ms) carry forward verbatim. Confirm measured values against autocorrelator-fix prototype.

9. **R36 task breakdown.** Initial estimate: R36-pre tripwire (12 carry-forward goldens reproduce + autocorrelator pre-flight on `vibrato.wav` confirms octave-jump baseline) + R36a (autocorrelator fix in `main.cpp` `--vibrato` mode) + R36b (NEW `--saturator-tail-comparison` mode + render new golden) + R36c (re-baseline `vibrato.json{,.sha256}` strict) + R36d (RESEARCH §19 O-Bowed audit + cross-comparison rendering + verdict) + R36e (regression bar via `reproduce-goldens.sh` 13 entries: 12 carry-forward WAVs unchanged + 1 new saturator-tail-comparison WAV) + R36f (auval + pluginval-10) + R36 atomic commit + R36-backfill chore. Research-phase locks task body and ordering.

10. **New goldens scope final spec.** 1 changed (`vibrato.json` + `vibrato.json.sha256`) + 3 new (`saturator-tail-comparison.{wav.sha256, json, json.sha256}`). 12 carry-forward WAVs + their JSONs unchanged (HR-11 trivially preserves bit-exact regression).

11. **RESEARCH §19 deliverable structure.** §19.1 HR-11 pre-flight (12 carry-forward goldens reproduce). §19.2 autocorrelator algorithm validation + sub-sample precision benchmark. §19.3 O-Bowed saturator topology audit. §19.4 O-Bowed render harness availability + parity-mode invocation. §19.5 saturator-tail measurement protocol. §19.6 cross-comparison findings (measured envelope divergence at 5 s mark). §19.7 verdict (research-only acknowledged divergence vs Phase 2.4c-bis escalation). §19.8 R36 task breakdown (10 plan-phase open items handed to PLAN rev-10).

12. **`--saturator-tail-comparison` wall-clock budget pre-flight.** 65 s of audio at default settings. Single-combo wall-clock estimate: ~0.5 s (mirrors Phase 2.4a `--matrix-stability` 0.04 s/combo extrapolation). Pre-flight 3 back-to-back renders to confirm sha256 determinism (mirrors §17.2 / §18.2 precedent).

---

## Risks (Phase 2.4c-specific)

1. **HR-11 trivially preserved by construction**, but accidental DSP edit (e.g., during research-phase prototyping) violates the rule. Mitigation: R36-pre tripwire reproduces 12 carry-forward goldens BEFORE R36a edits; R36e re-tripwire confirms 12-of-12 WAV byte-identical post-edits. Any drift = HARD violation requiring escalation to Phase 2.4c-bis (source-change cycle) and reset of R36 to atomic-commit-of-just-harness-changes.

2. **Autocorrelator parabolic-interp + range-bias may be insufficient at 12-cent vibrato.** Period modulation ~0.7% (1070 → 1078 samples peak deviation). Sub-sample precision ~0.1 sample needed. If parabolic-interp prototype shows excess noise, escalate to YIN (cumulative mean normalised difference) or AMDF (average magnitude difference function with octave-bias correction subtracting lag-2× minimum from lag-1×) per Open Q1-bis. Mitigation: research-phase pre-flight catches this BEFORE plan-phase locks R36a algorithm.

3. **O-Bowed render harness may be unavailable or non-parity-able.** Plan-phase escalation lane: (a) drop O-Bowed comparison from Phase 2.4c → defer to 2.4c-bis with O-Bowed harness build prerequisite, OR (b) scope-expand R36b to build a one-shot O-Bowed harness (estimate +1 day + cohort-harness regression risk). Research-phase confirms availability before plan-phase commits.

4. **Saturator-tail divergence > 2 dB triggers Phase 2.4c-bis escalation mid-Phase 2.4c.** This is a feature, not a bug — escalation criterion is the design intent. Mitigation: PLAN rev-10 includes explicit escalation lane spec; if escalation triggers, R36 atomic stays harness-only (autocorrelator fix + measurement infrastructure), and Phase 2.4c-bis opens with CONTEXT rev-9-bis specifying source-change scope.

5. **Vibrato pre-flight catches autocorrelator drift unrelated to Phase 2.4c source edits.** Mitigation: `vibrato.wav.sha256` byte-identical pre-flight in R36-pre confirms NO upstream WAV drift; if drift, INVESTIGATE before plan-phase (mirrors §16.1 / §17.1 / §18.1 precedent).

6. **`saturator-tail-comparison.wav.sha256` non-deterministic across re-renders.** State-bleed between sustain + release segments could surface, especially with long INFINITE_SUSTAIN=1.0 buildup. Mitigation: research-phase pre-flights 3 back-to-back renders to confirm determinism; uses `processor.releaseResources(); processor.prepareToPlay(...)` for state reset (Phase 2.4b R35-pre precedent).

7. **JSON `decayEnvelopeDb` array width vs sha256 noise.** 65 floats serialised into JSON could surface rounding noise across runs (similar to Phase 2.4a `--matrix-stability` JSON wall-clock noise issue). Mitigation: serialise floats with fixed-width 6-decimal-place format; strip wall-clock fields; basename-only outputWav path. Research-phase pre-flights determinism.

8. **R36 atomic commit interaction with R35-backfill chore.** R35-backfill (`0db5fac`) propagated R35 sha into STATUS.md. R36 atomic lands while R35-backfill is most recent. R36-backfill chore propagates R36 sha after R36 atomic. Mitigation: chore commit follows atomic commit (mirrors R35 / R34 / R33 precedent).

9. **RESEARCH §19 O-Bowed audit surfaces non-saturator divergences** (e.g., bridge LP coefficient choice, friction junction defaults, dispersion topology). Mitigation: research-phase scope is saturator-only; out-of-scope findings are tracked as Phase 2.4c-bis or v1.1 backlog items, NOT folded into Phase 2.4c.

10. **Phase 2.5-awareness might supersede the saturator-tail decision before it lands.** Body resonator (Phase 2.5) changes downstream amplitude envelope; the in-loop saturator-tail decay characterisation might be moot once body modes contribute to the measured envelope. Mitigation: Phase 2.4c verdict explicitly notes "valid for v1.0 pre-Phase-2.5 architecture"; Phase 2.5 verify includes a saturator-tail re-measurement as a regression check.

11. **MIDI 28 expected-period range bias may be incorrect for E1 dispersion-warped pitch.** Phase 2.1c dispersion may shift the perceived fundamental slightly (research-phase Risk #7 carry-forward). Mitigation: research-phase pre-flight `vibrato.wav` autocorrelator output reports the actual fundamental period observed in the WAV; range-bias bounds tune against measured data, not theoretical MIDI-derived value.

12. **`reproduce-goldens.sh` 12 → 13 entry growth.** Adds `--saturator-tail-comparison` invocation with 60 s sustain + 5 s release (~0.5 s wall-clock). Total reproduce-goldens.sh runtime grows by ~0.5 s. Mitigation: negligible; mirrors Phase 2.4b 10 → 12 growth precedent.

---

## Next Phase

Ready for: **research** phase — `/clear` then `/plugin-research O-Contrabass 2-dsp`

Research focus (Phase 2.4c):

1. **Resolve Open Questions #1–#12** — autocorrelator algorithm validation, lag-search range bounds, O-Bowed saturator topology audit, O-Bowed render harness availability, saturator-tail measurement protocol, divergence threshold tuning, vibrato golden re-baseline scope, strict `pass_vibratoAudible` threshold values, R36 task breakdown, new goldens scope, RESEARCH §19 deliverable structure, wall-clock budget pre-flight.
2. **HR-11 bit-exact pre-flight** — render all 12 currently-committed goldens BEFORE Phase 2.4c source edits via `reproduce-goldens.sh`; capture sha256s + verify against committed values. If any drift, INVESTIGATE before plan-phase.
3. **Autocorrelator octave-jump baseline reproduction** — pre-flight `--vibrato` mode against `vibrato.wav` at HEAD; confirm Phase 2.3 R28 documented octave-jump behavior reproduces; document measured rate / depth / onset values.
4. **Autocorrelator fix prototype + sub-sample precision benchmark** — prototype parabolic-interp + range-bias in research scratch space; measure on `vibrato.wav`; confirm rate / depth / onset fall into Phase 2.3 PLAN rev-7 strict ranges. Escalate to YIN / AMDF / cepstrum only if precision insufficient.
5. **O-Bowed saturator topology audit** — grep `plugins/O-Bowed/Source/DSP/` for nonlinearity application points; document divergence from O-Contrabass.
6. **O-Bowed render harness availability + parity invocation** — locate harness path; document parity-mode CLI; pre-flight a canonical render to confirm parity; escalate if unavailable.
7. **Saturator-tail cross-comparison render** — render canonical E1 60s+5s in BOTH plugins; capture decay envelopes; measure divergence at 5 s mark; document verdict (research-only acknowledged divergence vs Phase 2.4c-bis escalation).
8. **`--saturator-tail-comparison` wall-clock + sha256 determinism pre-flight** — 3 back-to-back renders confirm determinism; format-tune JSON to eliminate any rounding noise.
9. **Append RESEARCH §19** — document all resolutions above. (No §17 / §18 changes; Phase 2.4a §17 + Phase 2.4b §18 locked.)

After research: plan-phase (PLAN rev-10) writes R36 task breakdown verbatim against this CONTEXT + research findings; execute-phase performs implementation + new goldens + R36 atomic commit; verify-phase confirms Gate 6c invariants.

---

## Audit Trail (rev-8 supersedes rev-7)

**rev-1 (2026-04-26):** Phase 2.1 broad discuss. Cycle scope = Phase 2.1 (sub-phases a/b/c).

**rev-2 (2026-04-26):** Phase 2.1a closure (Option A, R7) + Phase 2.1b opening (module extraction, Gate 2). Phase 2.1b verified 2026-04-27 (R8a `bd5fae0` + R15 `ef0604d`, Gate 2 PASS).

**rev-3 (2026-04-27):** Phase 2.1c opening — cascaded allpass dispersion. Verified 2026-04-27 (R20 `5759e5e`, Gate 3 PASS).

**rev-4 (2026-04-27):** Phase 2.2 opening — 4-string EADG + per-string detune + per-string M-table. Verified 2026-04-27 (R26 `131c2c7`, Gate 4 PASS).

**rev-5 (2026-04-27):** Phase 2.3 opening — Vibrato + Slow-Bow LFO + Schelleng wedge clamp + EXPRESSION_MACRO. HR-1..HR-4 binding. Verified 2026-04-27 (R33 `af54571`, Gate 5 PASS with rebaseline of 4 audible carry-forward goldens).

**rev-6 (2026-04-27):** Phase 2.4a opening — Schelleng wedge bass-register calibration polynomial + 108-combo stability matrix dual-purpose render + `pass_breathingAudible` 5%→20% threshold restoration. HR-5..HR-8 binding. Verified 2026-04-28 (R34 `4c926bb`, Gate 6a CLEARED — 3 strict-PASS + 2 soft-PASS within v1.0 budgets) + R34-backfill chore `b64c8c4`.

**rev-7 (2026-04-28):** Phase 2.4b opening — Sub-Harmonic Bias DSP-07 (ARCHITECTURE §457). HR-9 + HR-10 binding. Verified 2026-04-28 (R35 `3de8b66`, Gate 6b CLEARED — 4 strict-PASS + 1 soft-PASS within RESEARCH §18.6 v1.0 budget) + R35-backfill chore `0db5fac`. Phase 2.4-bis backlog grew by 1 item: kForceBoost retune to push `subharmEnergyRatio` above 0.40 strict.

**rev-8 (this document, 2026-04-28):** Phase 2.4c opening — autocorrelator octave-rejection harness fix + saturator-tail O-Bowed comparison (research-only). 12 approach decisions Q34–Q45 user-confirmed: scope = autocorrelator + saturator-tail paired (Q34); single R36 atomic commit (Q35); research-only default + Phase 2.4c-bis escalation lane at >2 dB envelope divergence (Q36); parabolic-interp + range-bias autocorrelator fix (Q37); restore Phase 2.3 PLAN rev-7 strict `pass_vibratoAudible` (Q38); NEW `--saturator-tail-comparison` harness flag (Q39); O-Bowed cross-comparison render in O-Bowed harness (Q40); 2 dB divergence threshold default (Q41); NEW Hard Rule HR-11 binding zero production DSP edits (Q42); Logic AU smoke deferred non-blocking (Q43); R36 atomic + R36-backfill chore (Q44); rev-8 covers 2.4c only (Q45). 12 open questions handed to research-phase: autocorrelator algorithm validation, lag-search range bounds, O-Bowed saturator topology audit, O-Bowed render harness availability, saturator-tail measurement protocol, divergence threshold tuning, vibrato golden re-baseline scope, strict threshold values, R36 task breakdown, new goldens scope, RESEARCH §19 deliverable structure, wall-clock budget pre-flight. NEW HR-11 hard rule binding (zero production DSP edits → harness-only / research-only by construction). Phase 2.5 (body resonator + bow noise) gets fresh CONTEXT rev-9 when its discuss-phase opens after Phase 2.4c verify. Continues atomic-commit sequence R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36.

**Inherited verbatim from rev-7 (not re-litigated):**

- All Phase 2.3 modulator surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order)
- All Phase 2.4b end-state (Step 2.5 sub-harmonic bias evaluation between Step 2 and Step 3; subHarmonicsSmoothed 30 ms ramp; lastSubAmount instrumentation; voiceBowForceUpliftThisBlock factor at Step 6)
- HR-1..HR-4 (Phase 2.3 literal-zero short-circuits + IEEE 754 identity-arithmetic + Schelleng skip on zero LFO depth)
- HR-5..HR-8 (Phase 2.4a inline constexpr linkage on SchellengCalibration.h + calibration behind HR-4 gate ONLY + matrix-stability bypass via weak-symbol + trilinear IEEE 754 identity arithmetic)
- HR-9..HR-10 (Phase 2.4b SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate + friction module ABI preservation via ROSIN inverse algebraic identity)
- 12 currently-committed goldens (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep + slow-lfo + schelleng-stress + sub-harmonics + sub-harmonics-stability) + matrix-stability evidence golden
- Atomic-commit gate-first principle (R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36)
- Phase 2.4-bis backlog parking (kForceBoost retune; Step 4 modulation gain / breathingAudible metric refinement; 3 fallback-cell reduction)
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify
- E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7) — separate concern
- Primary listening DAW: Logic Pro (AU)
- Sample-rate strategy: internal 88.2 / 96 kHz at friction junction
- Bow-friction module v1.0.0 at `modules/synthesis/bow-friction/`
- Per-plugin `DispersionFilter.h` + `SchellengCalibration.h` + `SubHarmonicBias.h` (NOT extracted to shared module)
- 29 APVTS parameters; parameter-spec.md sha256 `77638e25…` carries forward unchanged
- Stage-1 contract NOT amended in Phase 2.4c
- ARCHITECTURE.md NOT amended in Phase 2.4c
- Chaos detector + softClampState deferred to Phase 2.5/2.6 (Phase 2.4b R35 commit-body footnote)

**New in rev-8:**

- Q34 Phase 2.4c scope = autocorrelator harness fix + saturator-tail O-Bowed comparison paired
- Q35 single R36 atomic commit for both items
- Q36 research-only default verdict + Phase 2.4c-bis escalation lane at >2 dB envelope divergence
- Q37 parabolic-interp + range-bias autocorrelator fix; YIN / AMDF / cepstrum reserved as fallback
- Q38 restore Phase 2.3 PLAN rev-7 strict `pass_vibratoAudible` ranges (rate ∈ [4.5, 5.5] Hz / depth ∈ [10, 14]¢ / onset ∈ [800, 1000] ms)
- Q39 NEW `--saturator-tail-comparison` harness CLI flag with per-decade decay envelope JSON output
- Q40 O-Bowed cross-comparison via O-Bowed render harness (research-phase confirms availability)
- Q41 2 dB envelope divergence threshold at 5 s mark (default; research-phase tunes against measured data)
- Q42 NEW Hard Rule HR-11 binding (zero production DSP edits in Phase 2.4c)
- Q43 Logic AU smoke deferred non-blocking R37/R32/R27/R19f/R14e precedent
- Q44 R36 atomic commit + R36-backfill chore
- Q45 rev-8 covers Phase 2.4c only
- New harness CLI flag `--saturator-tail-comparison`
- New goldens `saturator-tail-comparison.{wav.sha256,json,json.sha256}`
- Re-baselined `vibrato.json{,.sha256}` only (WAV unchanged)
- `reproduce-goldens.sh` extended 12 → 13 entries
- Five-item Gate 6c bar: (1) all 12 carry-forward goldens reproduce byte-identical via `reproduce-goldens.sh` (HR-11 trivially satisfied); (2) `--vibrato` strict `pass_vibratoAudible` PASS post autocorrelator fix (rate / depth / onset within Phase 2.3 PLAN rev-7 strict ranges); (3) `--saturator-tail-comparison` golden bit-deterministic across re-renders + RESEARCH §19 verdict written; (4) auval AU VALIDATION SUCCEEDED + pluginval-10 SUCCESS; (5) RESEARCH §19 cross-comparison verdict locked (research-only acknowledged divergence OR Phase 2.4c-bis escalation flag). R37 Logic AU smoke deferred non-blocking.
