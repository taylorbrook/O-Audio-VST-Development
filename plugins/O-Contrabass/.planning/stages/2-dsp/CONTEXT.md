# Stage 2: DSP — Context (rev-5)

**Date:** 2026-04-27
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.3 — Vibrato + Slow-Bow LFO + Schelleng Wedge Clamp + EXPRESSION_MACRO**
**Supersedes:** rev-4 (Phase 2.2 — 4-string bank + per-string detune + per-string dispersion table, dated 2026-04-27). rev-4 contracts that remain locked are inherited verbatim and not re-litigated.

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle opens Phase 2.3 — addition of the modulator + macro layer on top of the validated 4-string EADG bank. Phase 2.2 closed 2026-04-27 with R26 atomic commit (`131c2c7`, Gate 4 PASS): 4 `WaveguideString` instances inside one mono voice, per-string detune in delay-samples space, per-string M=4/3/2/1 dispersion table, MIDI→string mapping with ACTIVE_STRINGS clamp, 5 ms equal-power crossfade on note-on string transitions, and 5 byte-equal goldens carrying forward as the Phase 2.3 strict regression bar (`d358abcd…` strict + per-string A/D/G + detune-sweep-A + note-sequence E→A→D→G→E).

The Phase 2.3 scope is a single coupled cycle: bass-tuned vibrato section (sine LFO + S-curve onset envelope, modulating active string's delay-line length), Slow-Bow LFO (0.05–2 Hz diagonal speed/pressure modulation, 23° pressure phase-lag), Schelleng wedge headroom clamp (per-block, inline in voice), and EXPRESSION_MACRO knob (multiplicative layer onto bow speed/pressure/vibrato + brightness offset). All four items are control-rate modulator logic with shared per-block evaluation order — natural single integration unit.

After Phase 2.3 verifies, Phase 2.4 (sub-harmonics + 108-combo stability matrix) opens as a fresh GSD cycle.

---

## Cycle Scope

**Goal:** Add the 4 modulator/macro layers on top of the Phase 2.2 voice. Vibrato modulates the **active string's** delay-line length per sample (idle strings' delays untouched — preserves bit-exact regression). Slow-Bow LFO modulates voice-level bow speed + pressure with 23° phase-lag; depth is depth-clamped to 80% of the per-block Schelleng wedge headroom. Schelleng wedge `fMin`/`fMax` are computed once per block from current `v_b`/`F_bow`/`beta`, inline in `BowedContrabassVoice` (no new helper class). EXPRESSION_MACRO is a 20 ms `SmoothedValue<Linear>` 0–1 knob layering proportional gains onto bow speed (×1.0–1.4), bow pressure (×1.0–1.6), vibrato depth (×1.0–1.3), and a +0–500 Hz offset on the BRIGHTNESS bridge-LP frequency. All Phase 2.2 goldens must reproduce byte-identically with all modulators at zero (regression bar invariant 1 of Gate 5).

**In scope:**
- `Source/BowedContrabassVoice.{h,cpp}` — add per-voice `vibratoPhase` / `vibratoOnsetTimerSeconds` / `slowLfoPhase` floats; add 4× SmoothedValue<Linear> for `effectiveBowSpeed` / `effectiveBowPressure` / `effectiveVibratoDepth` / `effectiveBrightnessHz` (20 ms ramps); add `noteOffFadeOutTimerSeconds` for vibrato fast-fade. Per-block evaluation order: (1) read raw APVTS BOW_SPEED / BOW_PRESSURE / BOW_POSITION / VIBRATO_RATE / VIBRATO_DEPTH / VIBRATO_ONSET / SLOW_LFO_RATE / SLOW_LFO_DEPTH / EXPRESSION_MACRO / BRIGHTNESS; (2) compute Schelleng wedge `fMin`/`fMax` from raw bow params + active string's `beta`; (3) compute slow-LFO speedMod / pressureMod with depth clamped to `min(rawDepth, 0.8 × headroom)`; (4) apply slow-LFO multiplicatively to bow speed × (1 + 0.6·s) and bow pressure × (1 + 0.5·p); (5) layer macro multiplicatively onto bow speed / pressure / vibrato depth / brightness offset; (6) push to SmoothedValues. Per-sample (active string only): advance `vibratoPhase += 2π·effectiveVibratoRate / sr_internal`; advance `vibratoOnsetTimer`; compute `vibratoOnsetGate ∈ [0, 1]` from S-curve fade-in over 300 ms after onset delay; compute `vibratoCents = effectiveVibratoDepth · vibratoOnsetGate · sin(vibratoPhase)`; combine with active string's smoothed detune in delay-samples space → `setDelaySamples(...)` on active string only.
- `Source/PluginProcessor.cpp` — change EXPRESSION_MACRO default from `0.50f` → `0.0f` in `createParameterLayout()` (Q7a decision; preserves Phase 2.2 bit-exact regression bar). All other 28 parameters unchanged.
- `tests/render-harness/main.cpp` — add CLI flags for the 4 new gate harnesses:
  - `--vibrato` activates vibrato test mode at MIDI 28 with VIBRATO_DEPTH=12¢, VIBRATO_RATE=5 Hz, VIBRATO_ONSET=600 ms; emit JSON with `mode: "vibrato"`, FFT-derived peak depth in cents, time-to-full-onset (windowed RMS), `pass_vibratoDepthInRange` (10¢–14¢), `pass_onsetWindow` (~900 ms), `pass_rmsContinuity` (≥ 0.90).
  - `--slow-lfo` activates slow-LFO test mode at MIDI 33 (A1) with SLOW_LFO_DEPTH=0.5, SLOW_LFO_RATE=0.3 Hz, 60 s; emit JSON with `mode: "slow-lfo"`, `rmsByDecade` (10 deciles, peak-to-peak ≥ 20% required for `pass_breathingAudible`), `pass_rmsContinuity` (≥ 0.90).
  - `--schelleng-stress` activates wedge stress test at MIDI 28 with BOW_PRESSURE=7.0 N + BOW_SPEED=0.05 m/s + SLOW_LFO_DEPTH=1.0, 30 s; emit JSON with `mode: "schelleng-stress"`, `peakPostMaster` (must be ≤ 0 dBFS), `clampedDepthMean` (instrumentation hook reports mean clamped depth over render — must be < 0.5 to confirm headroom-clamp engaged), `pass_noNaN` (true).
  - `--macro-sweep` activates EXPRESSION_MACRO ramp 0 → 1.0 linear over the configured sustain at MIDI 38 (D2); emit JSON with `mode: "macro-sweep"`, `rmsByDecade`, `pass_rmsContinuity` (≥ 0.85 — looser because macro intentionally changes loudness), `pass_rmsRampDirection` (final-decade RMS > first-decade RMS by 10–30%).
- `tests/render-harness/golden/` — add 4 new golden text files: `vibrato.{wav.sha256,json}`, `slow-lfo.{wav.sha256,json}`, `schelleng-stress.{wav.sha256,json}`, `macro-sweep.{wav.sha256,json}`. WAVs NOT committed; sha256 + JSON only (Phase 2.1c / 2.2 convention).
- Phase 2.2 goldens (5 files) carry forward verbatim as Gate 5 invariant 1 (regression bar).

**Out of scope (deferred to later Phase 2.x cycles):**
- Sub-harmonic bias + 108-combo stability matrix (Phase 2.4)
- E1 calibration polynomial follow-up (Phase 2.4 per RESEARCH §14.10 Risk #7)
- Body resonator + bow noise (Phase 2.5)
- Master saturator/limiter, stereo width, microtonal, MPE (Phase 2.6)
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments (end-of-Stage-2 verify per locked decision)
- Saturator-tail dissipation review (Phase 2.4 per RESEARCH §12 footnote)

---

## Requirements Confirmed (Phase 2.3-relevant subsets of locked contracts)

- **DSP-09** (layered expression model: intrinsic CC + dedicated vibrato section + Expression Macro): primary deliverable. Vibrato rate/depth/onset all materialise; CC11/CC2/CC74 mapping comes in Phase 2.6 (microtonal + MPE); macro 4-destination layering materialises here.
- **DSP-10** (slow expressive attack): vibrato onset envelope (S-curve fade-in over 300 ms, default onset 600 ms) directly satisfies the "long natural bow-on-string transient" contract. Onset envelope is per-note (re-armed every note-on per Q3).
- **DSP-08** (Slow Bow LFO 0.05–2 Hz): primary deliverable. Diagonal speed/pressure modulation, 23° pressure phase-lag, Schelleng-wedge depth clamp.
- **FUNC-02** (sustained-first articulation, 60+ s no artifacts): carry-forward; `--slow-lfo` 60 s harness extends the bar to modulator-active sustain.
- **PERF-01** (no allocations in `processBlock`): enforced. All smoothers / phase counters / onset timers preallocated in `prepareToPlay`. Per-sample vibrato modulation on active string only (Q2) keeps idle-string code paths cold.
- **PERF-02** (< 5% CPU on M1): tracked. Per-block evaluation order is constant-time (~50 LOC of arithmetic). Per-sample additions: 1× sin (vibrato phase), 1× delay-line `setDelaySamples` (already in Phase 2.2 detune path, just adds vibrato cents to the existing detune cents). No new per-sample dispatch overhead. Estimated ~0.3% CPU added by Phase 2.3.
- **PERF-03** (latency = oversampler only): unchanged. All Phase 2.3 logic is control-rate or per-sample-on-existing-paths. No new look-ahead.
- **QUAL-01** (no audible clicks during parameter sweeps): explicit Gate 5 invariants — vibrato sustained-tone harness rmsContinuity ≥ 0.90; slow-LFO sustained-tone rmsContinuity ≥ 0.90; macro-sweep rmsContinuity ≥ 0.85 (looser per Q8 derivation); strict byte-equal regression on all 5 Phase 2.2 goldens at modulators-off.
- **QUAL-02** (extreme drone settings remain musical): Schelleng wedge clamp directly satisfies — slow-LFO depth auto-attenuated when bow params are near wedge boundary, no friction-junction zero-crossing artifacts. `--schelleng-stress` Gate 5 invariant 4 catches failure (peak ≤ 0 dBFS, no NaN, headroom clamp engaged).

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**
- All 29 APVTS parameter IDs, ranges, skews — `parameter-spec.md` (sha256:c47fe736…) **EXCEPT** the EXPRESSION_MACRO default value flip from 0.50 → 0.00 per Q7a. This is a single-default-value change to PluginProcessor.cpp's `createParameterLayout()` body; parameter ID, range, skew, and metadata all unchanged. parameter-spec.md and contract_checksums.parameter_spec must be updated in the R28 atomic commit. **Note this as a Stage-1 contract amendment in execute-phase commit body.**
- DSP architecture (`research/ARCHITECTURE.md`, sha256:3cb26814…) — F3 deviation (no in-loop DCB) + saturator-tail tracking carry forward; ARCHITECTURE amendment still deferred to end-of-Stage-2 verify.
- ROADMAP phasing (sha256:106639f6…)
- `modules/synthesis/bow-friction/` v1.0.0 (Phase 2.1b) — module is value-class deterministic; Phase 2.3 does NOT touch friction.
- `Source/DSP/DispersionFilter.h` (Phase 2.1c, R20 commit `5759e5e`) — Phase 2.3 consumes verbatim; no edits.
- `Source/DSP/WaveguideString.{h,cpp}` topology + per-instance config surface (Phase 2.2, R26 commit `131c2c7`) — Phase 2.3 consumes verbatim; no edits. Vibrato modulates the active string's delay length via existing `setDelaySamples()` API.

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**
- `getLatencySamples()` non-virtual — keep using `setLatencySamples()` in `prepareToPlay`; no Phase 2.3 latency change.
- `juce::ScopedNoDenormals` at `processBlock` entry (mandatory); already in place.
- `juce::SmoothedValue<float, Linear>` 20 ms ramp for macro-effective bow speed / pressure / vibrato depth / brightness offset (architecture line 152 + 567).
- `juce::dsp::DelayLine<float, Lagrange3rd>` per-sample `setDelaySamples()` during vibrato + detune ramp is JUCE-validated for click-free continuous modulation (Phase 2.2 detune path is the precedent; Phase 2.3 just stacks vibrato cents onto detune cents).

**Phase 2.3-specific constraints:**
- **Active-string-only vibrato modulation** (Q2) — vibrato adjusts ONLY `strings[activeStringIndex]`'s delay length per sample. Idle strings' `setDelaySamples()` is called once per note-on/string-switch (carrying detune only) and not modulated again until the string becomes active or detune changes. This preserves the Phase 2.2 strict byte-equal regression bar at VIBRATO_DEPTH=0.
- **Vibrato lifecycle: re-arm onset, carry phase** (Q3) — `noteStarted()` resets `vibratoOnsetTimerSeconds = 0.0f` (re-arms the S-curve onset envelope) but does NOT reset `vibratoPhase` (continuous sine across note boundaries to avoid mid-program phase jumps). Note-off triggers a 150 ms fast fade-out on the onset gate (faster than bowing tail per architecture line 127).
- **Schelleng wedge inline in voice** (Q4) — ~8 LOC of per-block math at `renderNextBlock` start. CONTEXT rev-2 explicitly de-classed `SchellengGuard` ("not a real class"); no new header.
- **Anti-correlation guard proportional** (Q5) — vibrato rate offset = `+0.13f * SLOW_LFO_DEPTH` (Hz). Click-free; offset scales with audibility. At SLOW_LFO_DEPTH=0 the offset is 0; at 1.0 the offset is +0.13 Hz.
- **EXPRESSION_MACRO default 0.0** (Q7a) — parameter default flipped from architecture-spec'd 0.50 to 0.00 to preserve Phase 2.2 strict byte-equal regression bar. Out-of-the-box character = pure base parameters (no macro lift). v1.0 ships this default; macro-lifted character available via user knob movement or future presets (Phase 4.1+).
- **EXPRESSION_MACRO multiplicands verbatim from architecture** (Q6 default) — `effectiveSpeed = bowSpeed × (1.0f + 0.4f * macro)`, `effectivePressure = bowPressure × (1.0f + 0.6f * macro)`, `effectiveVibratoDepth = vibratoDepth × (1.0f + 0.3f * macro)`, `brightnessOffset = 500.0f * macro` (added to BRIGHTNESS in Hz before the bridge-LP `p` calculation).
- **Inline modulator helpers** (Q10) — `VibratoLFO` and `SlowBowLFO` logic stays inline in `BowedContrabassVoice.cpp` for v1.0. Estimated ~30 LOC each. No extraction to `Source/DSP/`. Revisit only if execute-phase exceeds ~60 LOC each.
- **Anti-correlation guard verification = listening-test-only** (Q8) — automated Gate 5 bar has 8 invariants (no FFT-bin spectral test for 5:1 beating); R32 Logic AU smoke covers it qualitatively.
- **All 5 Phase 2.2 goldens MUST reproduce byte-identically with modulators at zero** — this is Gate 5 invariant 1, the strict regression bar. With VIBRATO_DEPTH=0, SLOW_LFO_DEPTH=0, EXPRESSION_MACRO=0 (new default), every code path added in Phase 2.3 must early-return or evaluate to mathematical no-op. Implementation MUST follow these hard rules:
  - **HR-1:** When `effectiveVibratoDepth = 0` (because either VIBRATO_DEPTH=0 or macro multiplier evaluates to 1.0 with depth=0), the vibrato cents calculation must be pure-zero output, NOT a near-zero floating-point result. Check sin-result against literal 0 if depth is 0.
  - **HR-2:** When `SLOW_LFO_DEPTH = 0`, the slow-LFO speedMod/pressureMod must be pure-zero, AND the multiplicative apply must early-return or use `1.0f + 0.0f = 1.0f` exactly (no phase advance, no headroom clamp eval — early return at the top of slow-LFO block).
  - **HR-3:** When `EXPRESSION_MACRO = 0`, all 4 macro multiplicands evaluate to `1.0f + 0.0f = 1.0f` exactly, and brightnessOffset = `500.0f * 0.0f = 0.0f` exactly. SmoothedValue chain must not introduce drift — verify with bit-exact regression test.
  - **HR-4:** Schelleng wedge eval is per-block; when SLOW_LFO_DEPTH=0, skip the entire wedge math (early return) — even if the math is mathematically no-op, the floating-point operations could perturb processor state non-deterministically.

**Working-tree starting state (locked from Phase 2.2 verify, R26 commit `131c2c7`):**
- `Source/BowedContrabassVoice.{h,cpp}` — 4-string voice with `std::array<WaveguideString, 4>`, per-string detune SmoothedValue, MIDI→string mapping, ACTIVE_STRINGS clamp, 5 ms equal-power crossfade
- `Source/DSP/WaveguideString.{h,cpp}` — split-rail with M=4/3/2/1 dispersion (Phase 2.1c rev-3 + Phase 2.2 R26)
- `Source/DSP/DispersionFilter.h` (130 LOC) — public API consumed verbatim
- `Source/PluginProcessor.{h,cpp}` — 29 APVTS parameters incl. all 4 `DETUNE_*` and `ACTIVE_STRINGS` (Stage 1 + Phase 2.2)
- `modules/synthesis/bow-friction/` v1.0.0 — module is value-class deterministic
- 5 Phase 2.2 goldens carry forward as Gate 5 invariant 1 (sha256s `d358abcd…`, `aa88f4c3…`, `d0ef8087…`, `524d2186…`, `5e31dad3…`, `2a731edb…`)

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q1 — Cycle scope** | **Single Phase 2.3 cycle** = Vibrato + Slow-Bow LFO + Schelleng wedge clamp + EXPRESSION_MACRO | Macro destinations (bow speed/pressure/vibrato/brightness) all live in Phase 2.3's territory; deferring to Phase 2.6 forces late-stage cross-phase coupling. All four are control-rate modulator logic with shared per-block evaluation order. User-confirmed. |
| **Q2 — Vibrato target across 4 strings** | **Active string only** | Idle strings stay cold-decoupled per Phase 2.2 topology (~3 vs 12 multiplies/sample). Preserves bit-exact regression bar at VIBRATO_DEPTH=0. User-confirmed. |
| **Q3 — Vibrato lifecycle on note-on** | **Re-arm onset envelope, carry phase** | Each note's onset is per-note semantically (architecture line 125: 600 ms onset from note-on). Sine LFO phase carries forward to avoid mid-program phase jumps on legato. Note-off → 150 ms fast fade-out on onset gate (architecture line 127). User-confirmed. |
| **Q4 — Schelleng wedge implementation** | **Inline in `BowedContrabassVoice`** (~8 LOC per-block) | CONTEXT rev-2 explicitly de-classed `SchellengGuard`. Voice already owns BowModel + bow param smoothers. No new header surface. User-confirmed. |
| **Q5 — Anti-correlation guard transition** | **Proportional**: vibrato rate offset = `+0.13f * SLOW_LFO_DEPTH` Hz | Click-free at all transitions. Offset scales with audibility — at LFO=0, offset=0; at LFO=1, offset=+0.13 Hz. Cleanest math, no arbitrary thresholds. User-confirmed. |
| **Q6 — Macro multiplicands** | **Architecture verbatim**: speed × (1.0+0.4·m), pressure × (1.0+0.6·m), vibrato depth × (1.0+0.3·m), brightness +500·m Hz | Architecture line 567 spec'd. No override. Default. |
| **Q7a — EXPRESSION_MACRO default value** | **0.0** (changed from architecture-spec'd 0.5) | Knob at zero = literal no-op (×1.0 multipliers + 0 Hz brightness offset). Preserves Phase 2.2 strict byte-equal regression bar. parameter-spec.md flagged as Stage-1 contract amendment in R28 atomic commit body. User-confirmed. |
| **Q8 — Anti-correlation guard verification** | **Listening-test-only** (R32 Logic AU smoke) | FFT-bin spectral test for 5:1 beating is hard to automate cleanly at bass register (1 Hz noise floor). Drop from automated bar; covered qualitatively in listening test. User-confirmed. |
| **Q9 — Atomic commit unit** | **R28 Phase 2.3 atomic commit** lands on Gate 5 PASS | Continues sequence: R7 → R15 → R20 → R26 → R28. Lands ~2 source files (PluginProcessor.cpp + BowedContrabassVoice.{h,cpp}) + harness + 4 new golden text files + parameter-spec.md amendment + planning artefacts. Carry-forward principle. |
| **Q10 — Helper class extraction** | **Inline in `BowedContrabassVoice`** | Vibrato + Slow-LFO logic ~30 LOC each, tightly coupled to voice state. No reuse benefit (these are O-Contrabass-specific bass-tuned values). Revisit if execute-phase exceeds ~60 LOC each. User-confirmed. |
| **Q11 — Primary listening DAW** | **Logic Pro (AU)** carry-forward | R19f / R27 precedent. Manual smoke audition: vibrato on sustained note + slow-LFO sweep + macro sweep. |
| Per-block evaluation order | (1) read raw APVTS → (2) Schelleng wedge → (3) slow-LFO with depth clamp → (4) apply LFO multiplicatively to bow speed/pressure → (5) layer macro → (6) push to SmoothedValues → (7) per-sample loop | Schelleng wedge depends on raw bow params (not macro-lifted) per architecture line 555 ("LFO depth must be clamped per-block based on current F_bow/v_b/beta"). Macro layers AFTER LFO so macro-lifted bow params don't feed back into wedge calc. |
| Vibrato note-off fade-out duration | 150 ms (mid-range of architecture's 100–200 ms) | Faster than bowing tail (~5 s post-bow-off per Phase 2.1a) to avoid pulsing decay. Linear ramp on `vibratoOnsetGate` from current value → 0 over 150 ms. |
| Hard rules for bit-exact preservation | HR-1 zero-vibrato-depth literal check; HR-2 zero-LFO-depth early return; HR-3 zero-macro literal arithmetic; HR-4 zero-LFO-depth Schelleng skip | All four modulators must early-return-on-zero-depth or evaluate to literal mathematical no-op. SmoothedValue chain audited for bit-exact at default values. |
| Phase 2.3 listening test sequence (R32) | (1) MIDI 38 (D2) sustained + VIBRATO_DEPTH 0→25¢ ramp at 5 Hz vibrato → (2) MIDI 33 (A1) sustained + SLOW_LFO_DEPTH 0→1.0 ramp at 0.3 Hz → (3) MIDI 28 (E1) extreme bow params (PRESSURE 7 N + SPEED 0.05 m/s) + SLOW_LFO_DEPTH=1 (Schelleng stress) → (4) MIDI 38 (D2) + EXPRESSION_MACRO 0→1.0 ramp → (5) E1+VIBRATO+SLOW_LFO together (anti-correlation guard audition) | Covers all 4 modulator paths + the anti-correlation guard qualitative verification. ~60 s total. |

---

## Open Questions (handed to research-phase)

1. **Vibrato S-curve onset envelope formula.** Architecture line 125: "S-curve fade-in over 300 ms (half-cosine ramp)". Recommend: `gate(t) = 0.5f - 0.5f * cos(π · t / 0.3f)` for `t ∈ [0, 0.3]`, where `t = max(0, vibratoOnsetTimerSeconds - VIBRATO_ONSET_seconds)`. Research-phase confirms or specifies alternate (e.g., 5th-order smoothstep `t² · (3 - 2t)` is also common; half-cosine matches architecture text more literally).
2. **Schelleng wedge bass-register validity.** Phase 2.1c Risk #7 documented closed-form coefficient clamp at E1 (paper validity envelope is piano register, bass sits outside). Pre-flight question: does the wedge `fMin`/`fMax` calculation produce a meaningful headroom at E1+drone defaults, or does it always-clamp to zero (silencing slow-LFO entirely)? Research-phase computes headroom value at: MIDI 28, BOW_SPEED=0.15 m/s, BOW_PRESSURE=1.0 N, BOW_POSITION=0.10, INFINITE_SUSTAIN=0.5 (drone-ish defaults). If headroom < 0.1, slow-LFO is effectively silenced at bass register and we need a Phase 2.4 calibration polynomial (analogous to Risk #7's E1 dispersion clamp).
3. **Vibrato + detune stacking math.** Active string has both detune (Phase 2.2 SmoothedValue in delay-samples space) and vibrato (Phase 2.3 sine modulation in cents). Stacking: `effectiveDelaySamples = baseDelay × 2^(-detuneCents/1200) × 2^(-vibratoCents/1200)`. Recommend: combine cents first (`totalCents = detuneCents + vibratoCents`), then single `2^(-totalCents/1200)` multiply, then convert to delay samples. Lagrange3rd absorbs both modulations cleanly. Research-phase confirms math + documents per-sample call sequence in `BowedContrabassVoice::renderNextBlockOscillator()` or wherever.
4. **EXPRESSION_MACRO smoothing window for brightness offset.** 20 ms `SmoothedValue<Linear>` is the carry-forward default (architecture line 567). But brightness offset is a 0→500 Hz step at macro=0→1; 500 Hz over 20 ms = 25 kHz/s ramp on the bridge-LP `p` coefficient. Research-phase confirms: does this introduce zipper noise on the BRIGHTNESS sweep, or is 20 ms enough? Falls back to 50 ms if 20 ms zippers (matches body-bank pattern at architecture line 522).
5. **Per-block evaluation order final lock.** CONTEXT line above proposes (1)→(7). Research-phase finalises exact pseudocode + edge cases (e.g., when the 5 ms string-switching crossfade is active, are vibrato and slow-LFO advancing on both old + new strings or only the new active one? Recommend: phase counters advance once per block regardless of string state; only the active string applies the modulation). Key: vibrato and slow-LFO are voice-level (one phase counter each), not per-string.
6. **Harness JSON schemas** for `--vibrato`, `--slow-lfo`, `--schelleng-stress`, `--macro-sweep`. Research-phase finalises schema field names + threshold values, analogous to Phase 2.2's `mode: "detune-sweep"` / `mode: "note-sequence"` patterns. Open: how is "FFT-derived peak depth in cents" computed in the vibrato test — autocorrelation pitch tracking? Cepstral analysis? Or simpler peak-detection on instantaneous frequency from zero-crossings? Recommend: FFT bin shift on a 4096-sample sliding window centred on sustained portion; pitch deviation = `bin_at_peak / bin_at_baseline`. Cheap and robust at bass register.
7. **Bit-exact preservation audit.** With all 4 hard rules (HR-1 to HR-4) implemented, does the SmoothedValue<Linear> chain introduce any floating-point drift at default values (VIBRATO_DEPTH=0, SLOW_LFO_DEPTH=0, EXPRESSION_MACRO=0)? Research-phase pre-flight: capture Phase 2.2 golden render with the modified PluginProcessor.cpp (EXPRESSION_MACRO default=0.0, no other changes) BEFORE any Phase 2.3 source edits — sha256 should still match `d358abcd…` (because architecture-spec'd 0.5 wasn't actually wired into Phase 2.2's path; macro doesn't exist yet). Then capture the same render after Phase 2.3 source edits — sha256 must still match `d358abcd…`. If it doesn't, hard rules need tightening.
8. **Macro destination interaction with Schelleng wedge.** Per the locked per-block evaluation order, macro is applied AFTER slow-LFO (step 5 of 7). Implication: macro multiplies the LFO-modulated bow speed/pressure, so even at high macro the LFO depth was already wedge-clamped against the *raw* bow params. But macro lifts the *effective* bow params past the raw-params-derived wedge — could push effective params outside the wedge, causing instability. Research-phase: does the Schelleng wedge need to be re-evaluated against the macro-lifted params (move wedge eval to step 6 of 7)? Recommend: NO — macro is a "performance lift" that's user-controlled; if user dials macro to extreme + extreme bow params, the friction junction's other guards (algebraic saturator, energy clamp) catch instability. Re-evaluating wedge against macro-lifted params would gut the macro's effect at high settings. Research-phase confirms.
9. **`vibratoOnsetTimerSeconds` initialisation on `prepareToPlay`.** Voice is reset on prepareToPlay; vibratoOnsetTimer should start at... 0 (first note will trigger fresh onset)? Or at VIBRATO_ONSET seconds (so the first note has no onset delay, vibrato starts immediately if user re-loads plugin)? Recommend: 0 — every fresh note (including the first after reset) gets the full onset envelope. Matches per-note semantics of Q3.
10. **Stage-1 contract amendment process for EXPRESSION_MACRO default change.** parameter-spec.md needs the default value updated from 0.50 → 0.00. New sha256 needs to land in STATUS.md `contract_checksums.parameter_spec`. R28 atomic commit body explicitly notes this as a Stage-1 contract amendment justified by Q7a's regression-bar preservation. Research-phase: any other downstream artefacts that reference the old 0.50 default that need updating? (e.g., parameter-spec-draft.md, ROADMAP §"Open Decisions", BRIEF.md). Recommend: grep all .md files for "0.5" near "EXPRESSION_MACRO"; update only canonical artefacts (parameter-spec.md, STATUS.md checksums); leave historical artefacts (parameter-spec-draft.md, BRIEF.md) untouched as audit trail.

---

## Risks (Phase 2.3-specific)

1. **Bit-exact regression failure when modulators land.** Mitigation: hard rules HR-1 to HR-4 enforce literal-zero arithmetic at default values; research-phase Open Question #7 pre-flight verifies this empirically before any source edits. If regression breaks despite hard rules, escalate by moving modulator computation behind explicit `if (anyModulatorActive)` guard at the top of `renderNextBlockOscillator()`.
2. **Schelleng wedge always-clamps at bass register** (Open Question #2). Mitigation: research-phase pre-flight derives headroom value at default bass params. If headroom < 0.1, document as Phase 2.4 calibration polynomial follow-up and either (a) ship Phase 2.3 with slow-LFO effectively silenced at bass + flag in user manual, or (b) replace closed-form wedge with empirical bass-register table. Decision deferred to research-phase.
3. **Brightness offset zipper at 20 ms smoothing.** Mitigation: research-phase Open Question #4 derives whether 20 ms is sufficient for 500 Hz step. Fallback: 50 ms (body-bank pattern). Gate 5 invariant 5 (macro-sweep rmsContinuity ≥ 0.85) catches failure.
4. **Vibrato + detune Lagrange3rd interpolation accumulation error.** Mitigation: at extreme detune (-1200¢) + extreme vibrato (50¢), total cents = -1250¢ → delay-sample factor `2^(1250/1200) ≈ 2.05`. Lagrange3rd is validated for click-free up to delay-sample factor changes within the 8192-sample buffer. Phase 2.2 detune-sweep ±1200¢ harness already exercised this; Phase 2.3 vibrato is a small modulation on top. Gate 5 invariant 2 (vibrato rmsContinuity ≥ 0.90) catches accumulation failure.
5. **Per-block Schelleng wedge eval introducing per-sample CPU spike.** Mitigation: ~8 LOC of arithmetic at block start (fMin/fMax/headroom = 3 divides + 4 multiplies + 1 min). Cost per block, not per sample. Estimated <0.1% CPU on M1.
6. **Macro layering interaction with vibrato onset envelope.** Effective vibrato depth = base depth × (1 + 0.3·macro) × onsetGate. If macro is automated during the onset window, effective depth has compound modulation. Mitigation: this is by design — macro is a UX layer; user expects it to interact with vibrato character. No special handling; document in user manual.
7. **EXPRESSION_MACRO default change auditability.** Stage-1 contract amendment risk: changing parameter-spec.md mid-Stage-2 violates the "locked contract" pattern. Mitigation: Q7a's regression-bar preservation rationale is documented in CONTEXT rev-5 (this doc), R28 atomic commit body, and SUMMARY.md. checksum update is single-step and traceable.
8. **`--schelleng-stress` harness false-positives** (clamped depth not detectable from audio output alone). Mitigation: harness instrumentation hook exposes `clampedDepthMean` via JSON (set from voice's per-block `safeDepth` value, summed and averaged over render duration). This is already a Phase 2.1c precedent (`rmsByDecade` instrumentation hook). Research-phase finalises hook signature.
9. **Slow-LFO at very low rate (0.05 Hz) over short renders.** A 60 s render at 0.05 Hz is 3 cycles — visible RMS modulation. A 30 s render at 0.05 Hz is 1.5 cycles — possibly insufficient for `pass_breathingAudible` (≥20% peak-to-peak). Mitigation: `--slow-lfo` harness pins SLOW_LFO_RATE=0.3 Hz over 60 s = 18 cycles, well-sampled.

---

## Next Phase

Ready for: **research** phase — `/clear` then `/plugin-research O-Contrabass 2-dsp`

Research focus (Phase 2.3):

1. **Resolve Open Questions #1–#10** — vibrato S-curve formula, Schelleng wedge bass-register pre-flight (Risk #7-style), vibrato+detune stacking pseudocode, brightness offset smoothing window, per-block evaluation order final pseudocode, harness JSON schemas, bit-exact preservation pre-flight render, macro/wedge interaction policy, vibratoOnsetTimer init value, Stage-1 contract amendment grep audit.
2. **Pre-flight bit-exact baseline render** — capture sha256 at: MIDI 28, sustain 60 s, INFINITE_SUSTAIN=1.0, STRING_STIFFNESS=0, default everything else, with the modified PluginProcessor.cpp (EXPRESSION_MACRO default=0.0) but BEFORE any Phase 2.3 source edits. Expected sha256: `d358abcd…` (Phase 2.2 strict regression bar carry-forward). If sha256 differs, that's a research-phase finding requiring investigation BEFORE plan-phase.
3. **Pattern-confirm against O-Bowed.** O-Bowed has its own vibrato section with different parameter values (architecture line 119–128 in O-Contrabass spec'd specifically for bass). Confirm: does O-Bowed's vibrato live in `BowedStringVoice` similarly inline, or in a separate helper class? If separate, the inline-vs-extract decision (Q10) deserves a pattern-fidelity check. If inline, Q10 is reinforced.
4. **Pattern-confirm `--vibrato` / `--slow-lfo` / `--schelleng-stress` / `--macro-sweep` flags** against existing harness CLI shape (existing flags: `--note`, `--velocity`, `--sustain`, `--release`, `--infinite-sustain`, `--string-stiffness`, `--stiffness-sweep`, `--string`, `--detune-sweep`, `--note-sequence`). New flags follow same parsing pattern.
5. **Update RESEARCH.md** — append §16 documenting all the resolutions above. (No §12/§13/§14/§15 changes; those are Phase 2.4 follow-up + 2.1b/2.1c/2.2 history.)

After research: plan-phase (PLAN rev-7) writes R28+ task breakdown verbatim against this CONTEXT + research findings; execute-phase performs the implementation + R28 atomic commit; verify-phase confirms Gate 5 invariants (1–8) + R32 Logic AU smoke.

---

## Audit Trail (rev-5 supersedes rev-4)

**rev-1 (earlier 2026-04-26):** Phase 2.1 broad discuss. Cycle scope = Phase 2.1 (sub-phases a/b/c).

**rev-2 (later 2026-04-26):** Phase 2.1a closure (Option A, R7 commit) + Phase 2.1b opening (module extraction, Gate 2). Phase 2.1b verified 2026-04-27 (R8a `bd5fae0` + R15 `ef0604d` atomic commits, Gate 2 PASS bit-exact).

**rev-3 (2026-04-27):** Phase 2.1c opening — cascaded allpass dispersion, Gate 3. Phase 2.1c verified 2026-04-27 (R20 atomic commit `5759e5e`, Gate 3 PASS).

**rev-4 (2026-04-27):** Phase 2.2 opening — 4-string EADG bank + per-string detune + per-string M=4/3/2/1 dispersion table + MIDI→string mapping + ACTIVE_STRINGS + 5 ms string-switching crossfade. Phase 2.2 verified 2026-04-27 (R26 atomic commit `131c2c7`, Gate 4 PASS).

**rev-5 (this document, 2026-04-27):** Phase 2.3 opening — Vibrato + Slow-Bow LFO + Schelleng wedge clamp + EXPRESSION_MACRO. 11 approach decisions (Q1–Q11 user-confirmed: single Phase 2.3 cycle covers all four items; vibrato modulates active string only; vibrato lifecycle re-arms onset and carries phase; Schelleng inline in voice; anti-correlation proportional 0.13×depth; macro multiplicands architecture-verbatim; **EXPRESSION_MACRO default flipped 0.5 → 0.0** to preserve bit-exact regression; anti-correlation guard verification = listening-test-only; R28 atomic commit; inline helpers; Logic AU primary; plus per-block evaluation order, vibrato note-off fade, 4 hard rules HR-1 to HR-4 for bit-exact preservation, R32 listening test sequence). 10 open questions handed to research-phase: vibrato S-curve formula, Schelleng wedge bass-register validity, vibrato+detune stacking, brightness smoothing window, per-block eval pseudocode, harness JSON schemas, bit-exact pre-flight, macro/wedge interaction policy, vibratoOnsetTimer init, Stage-1 contract amendment grep audit.

**Inherited verbatim from rev-4 (not re-litigated):**
- Single voice with `std::array<WaveguideString, 4>` (not 4 voices in MPESynthesiser)
- MIDI→string mapping: highest open-string-at-or-below-the-note, thresholds 28/33/38/43, ACTIVE_STRINGS clamp with remap-to-highest-active-string
- 5 ms equal-power crossfade at voice mix-bus on note-on string transitions; only on note-on (not pitchbend/portamento)
- Per-string detune in delay-samples space, 20 ms SmoothedValue<Linear>
- Per-string dispersion M-table (E=4, A=3, D=2, G=1) with B prefactors (1e-4, 7e-5, 5e-5, 3e-5)
- Idle strings always tick (popSample/pushSample run; only friction injection gates by activeStringIndex)
- All 5 Phase 2.2 goldens (`d358abcd…` strict + per-string A/D/G + detune-sweep-A + note-sequence)
- Atomic-commit gate-first principle (R7 → R15 → R20 → R26 → R28)
- Saturator-tail Phase 2.4 follow-up parking + RESEARCH §12 footnote
- Phase 2.4 calibration polynomial follow-up parking (Risk #7, E1 closed-form clamp)
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify
- Primary listening DAW: Logic Pro (AU)
- Sample-rate strategy: internal 88.2 / 96 kHz at friction junction
- Bow-friction module v1.0.0 at `modules/synthesis/bow-friction/` (Phase 2.1b)
- Per-plugin `DispersionFilter.h` (NOT extracted to shared module)

**New in rev-5:**
- Q1 single Phase 2.3 cycle = Vibrato + Slow-Bow + Schelleng + Macro (all four items in one coupled cycle)
- Q2 vibrato active-string-only (preserves bit-exact regression bar; ~3 vs 12 multiplies/sample)
- Q3 vibrato lifecycle: re-arm onset envelope on every note-on, carry sine phase forward, note-off 150 ms fade
- Q4 Schelleng wedge inline in voice (no `SchellengGuard` helper class — CONTEXT rev-2 decision)
- Q5 anti-correlation guard proportional: vibrato rate += 0.13 Hz × SLOW_LFO_DEPTH (click-free, scales with audibility)
- Q6 macro multiplicands architecture-verbatim: speed × (1+0.4·m), pressure × (1+0.6·m), vibrato depth × (1+0.3·m), brightness +500·m Hz
- **Q7a EXPRESSION_MACRO default flipped 0.5 → 0.0** (Stage-1 contract amendment; rationale: preserves Phase 2.2 strict byte-equal regression bar)
- Q8 anti-correlation guard verification = listening-test-only (no FFT-bin spectral test in automated bar)
- Q10 inline modulator helpers in voice (no `VibratoLFO.h` / `SlowBowLFO.h` extraction)
- Q11 R32 Logic AU smoke listening sequence (vibrato + slow-LFO + Schelleng-stress + macro sweep + anti-correlation audition)
- Per-block evaluation order locked: raw APVTS → Schelleng → slow-LFO with depth clamp → apply LFO → layer macro → push to SmoothedValues → per-sample loop
- 4 hard rules HR-1 to HR-4 binding for bit-exact preservation at modulators-off
- Eight-item Gate 5 bar: (1) all 5 Phase 2.2 goldens strict byte-equal; (2) vibrato sustained-tone harness; (3) slow-LFO sustained-tone harness; (4) Schelleng wedge stress test; (5) macro-sweep harness; (6) auval; (7) pluginval-10; (8) R32 Logic AU smoke (user-deferred non-blocking)
- 4 new harness CLI flags: `--vibrato`, `--slow-lfo`, `--schelleng-stress`, `--macro-sweep`
- 4 new golden text files: `vibrato.{wav.sha256,json}`, `slow-lfo.{wav.sha256,json}`, `schelleng-stress.{wav.sha256,json}`, `macro-sweep.{wav.sha256,json}`
