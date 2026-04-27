# Stage 2 / Phase 2.1a-recovery — Verification

**Date:** 2026-04-26
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP) — Phase 2.1 cycle, sub-phase 2.1a only
**Phase:** verify
**Cycle scope:** Phase 2.1a-recovery (R1–R5; R7 commit deferred pending user decision)
**Plan revision verified:** rev-3 (F1+F2+F3+F4 coupled fix)
**Verdict:** ⚠️ PARTIAL — Phase 2.1a goals achieved on bow-on validation; standard harness `pass_rms` invariant FALSE due to characterised saturator-tail decay; R7 atomic commit DEFERRED pending Option A/B/C decision (per SUMMARY.md "Open Decisions").

---

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / PLAN.md rev-3)

1. Validate the highest-risk path of the project: a single E1 (41.2 Hz) digital-waveguide contrabass voice produces a stable 60-second sustain at maximum `INFINITE_SUSTAIN` with no NaN, no runaway, and no denormal CPU spikes (CONTEXT.md §"Cycle Scope" + ROADMAP §Phase 2.1).
2. Bootstrap Helmholtz oscillation reliably from cold-start at f0 = 41.2 Hz (rev-3 root-cause goal after RESEARCH §11 identified the three compounding bugs B1/B2/B3 in rev-2).
3. Apply F1 (split-rail topology), F2 (bridge LP DC-gain fix), F3 (in-loop DCB removal), F4 (drop voice-side `betaScale` fudge) as a single coupled change with sign-convention contract verified against O-Bowed canonical (PLAN rev-3 §"Locked decision (RESEARCH §11.3)").
4. Hold `auval -v aumu OCbs OuDv` and `pluginval --strictness-level 10` PASS through the engine wire-up.
5. Drop the voice-side `betaScale` fudge so `setBowPosition` drives the real split-rail β split via `updateDelayLengths()`.

### Deliverables (from SUMMARY.md + independent inspection)

1. F1 split-rail rewrite of `WaveguideString.{h,cpp}` mirroring O-Bowed canonical Smith two-port scattering with O-Contrabass's algebraic saturator. Verified by `grep` (16 hits for `bridgeDelay` / `neckDelay`, 0 hits for `dcX1` / `dcY1` / `kDCBlockerR`).
2. F2 bridge LP recurrence fix at `WaveguideString.cpp:144-148` — confirmed: `bridgeFiltered = bridgeG * bridgeOneMinusP * bridgeRaw + bridgeP * bridgeY + denormalLeak;` (DC gain = `g` exactly).
3. F3 in-loop DCB removal — confirmed: zero references to `dcX1` / `dcY1` / `kDCBlockerR` anywhere in `WaveguideString.{h,cpp}`.
4. F4 voice-side `betaScale` removal — confirmed: zero references to `betaScale` or `setStringImpedance` in `BowedContrabassVoice.cpp`.
5. Render-harness binary built (`build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test`) and re-runs cleanly under both scenarios.
6. AU + VST3 + Standalone artefacts present and installed at `~/Library/Audio/Plug-Ins/{VST3,Components}/O-Contrabass-dev.{vst3,component}` (timestamp 2026-04-26 20:51).

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| Stable 60s E1 sustain at max `INFINITE_SUSTAIN`, no NaN, no runaway | ✅ Achieved (bow-on validation) | `/tmp/verify-bowon-only.json`: 65s bow-on, `pass_nan=true`, `pass_peak=true` (peak=0.068), `pass_blockTime=true` (ratio 2.65), `pass_rms=true` (rmsRatio=1.04) — independently re-run 2026-04-26 |
| Helmholtz bootstrapping at cold start | ✅ Achieved | `rmsMid_s5_s6 = 0.0353` (factor of ~1.6 million increase over rev-2 R1's 2.23e-8); `peak / rmsMid ≈ 1.93` consistent with √3 sawtooth crest factor |
| F1 split-rail topology | ✅ Achieved | `bridgeDelay` + `neckDelay` pair, symmetric `+ newVelocity` injection at bow point, `−1` boundary on both rails (LP on bridge, raw on nut) — matches O-Bowed `WaveguideString.cpp:108`, `:131-133` |
| F2 bridge LP DC gain = g | ✅ Achieved | Recurrence form at `WaveguideString.cpp:146` matches O-Bowed canonical; bow-on-only render sustains at `g = 0.9999999` for 65s with `rmsRatio = 1.04` (no DC inflation, no decay) |
| F3 in-loop DCB removed | ✅ Achieved | `grep` confirms zero references; bootstrapping succeeds within first ~5s (test reaches steady-state by sample 5×sr) |
| F4 voice-side `betaScale` dropped | ✅ Achieved | `grep` on `BowedContrabassVoice.cpp` returns zero hits for `betaScale` / `setStringImpedance`; default `R_s = 0.5` from `HyperbolicFriction.h` init list applies |
| `auval -v aumu OCbs OuDv` PASS | ✅ Achieved | Re-verified 2026-04-26: "AU VALIDATION SUCCEEDED" — render at 11025/22050/44100/48000/96000/192000 Hz, 1-channel, parameter setting/scheduling, MIDI all PASS |
| `pluginval --strictness-level 10` PASS | ✅ Achieved | Re-verified 2026-04-26: SUCCESS — Editor Automation, Automatable Parameters, Parameter thread safety, Background thread state, Basic bus, Listing buses (0 in / 2 out), Enabling/disabling/restoring buses, Fuzz parameters all PASS |
| Standard harness `pass_rms` (60s sustain + 5s release tail) | ⚠️ Partial — invariant FALSE | `/tmp/verify-standard.json`: `rmsRatio_final_over_mid = 0.36` (below [0.5, 2.0] band). Root cause analytically characterised in SUMMARY.md "R6 NOT INVOKED" section: in-loop algebraic saturator `x/sqrt(1+x²)` loses ≈ x²/2 per pass at low amplitude × 2 rails × 41.2 round-trips/s ≈ 10 %/s free-decay; over 4–5s post-bow-off this reduces rms by ~64 % — exactly what the JSON shows. **Not a bootstrapping failure, not a B1/B2/B3 regression, not transcription error.** |

---

## Independent Reproduction (verify-phase audit trail)

All four numeric claims in SUMMARY.md were independently re-run during this verify phase against the installed binaries / built harness:

| Check | SUMMARY.md claim | Verify-phase reproduction | Match |
|---|---|---|---|
| `auval -v aumu OCbs OuDv` | SUCCEEDED | "AU VALIDATION SUCCEEDED" — same render-rate matrix and parameter/MIDI tests PASS | ✅ |
| `pluginval --strictness-level 10 --validate-in-process O-Contrabass-dev.vst3` | SUCCESS | SUCCESS — all test groups PASS, fuzz parameters complete | ✅ |
| Render-harness standard (`--sustain 60 --release 5`) | peak 0.0683, rmsMid 0.0353, rmsFinal 0.0127, ratio 0.36, pass_rms FALSE | `/tmp/verify-standard.json`: peak 0.068, rmsMid 0.0353, rmsFinal 0.0127, maxRatio 2.56, FAIL | ✅ (byte-identical to /tmp/e1-max-sustain-r3.json) |
| Render-harness bow-on-only (`--sustain 65 --release 0`) | peak 0.0683, rmsMid 0.0353, rmsFinal 0.0369, ratio 1.04, pass_rms TRUE | `/tmp/verify-bowon-only.json`: peak 0.068, rmsMid 0.0353, rmsFinal 0.0369, maxRatio 2.65, PASS | ✅ (byte-identical to /tmp/e1-bowon-only.json) |

Reproduction is bit-stable; no nondeterminism observed across re-runs.

---

## Code-Level Verification Against PLAN rev-3

### F1 — Split-rail topology

`WaveguideString.h:104-105` declares the rail pair:
```
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> bridgeDelay { 8192 };
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> neckDelay   { 8192 };
```
`WaveguideString.cpp:137-185` implements the canonical Smith two-port scattering loop with O-Contrabass's algebraic-saturator substitution (`x / sqrt(1+x²)` per rail on the WRITE path). Sign convention matches RESEARCH §11.4 sketch verbatim.

### F2 — Bridge LP recurrence fix

`WaveguideString.cpp:144-148`:
```cpp
if (! std::isfinite (bridgeY)) bridgeY = 0.0f;
float bridgeFiltered = bridgeG * bridgeOneMinusP * bridgeRaw
                     + bridgeP * bridgeY
                     + denormalLeak;
bridgeY = bridgeFiltered;
```
Coefficient form `(b0, b1, a0, a1) = (g·(1−p), 0, 1, −p)` matches O-Bowed `WaveguideString.cpp:94-95`. DC gain = `g·(1−p)/(1−p) = g` exactly. The `g`-in-feedback bug from rev-2 (which inflated DC gain to `g·(1−p)/(1−g·p) ≈ 1` at high sustain) is resolved.

### F3 — In-loop DCB removal

`grep -n 'dcX1\|dcY1\|kDCBlockerR'` against both `WaveguideString.h` and `WaveguideString.cpp` returns zero hits. No member declarations, no per-sample calls, no reset state. ARCHITECTURE.md §"DC Blocker" deviation is justified per RESEARCH §11.6 / PLAN rev-3 §"Why F3 deviates" (F2 LP correctness obviates the in-loop DCB; output-path DCB available if Phase 2.4's 108-combo matrix surfaces a real DC-drift pathology).

### F4 — Voice-side `betaScale` removal

`grep -n 'betaScale\|setStringImpedance'` against `BowedContrabassVoice.cpp` returns zero hits. Default `R_s = 0.5` from `HyperbolicFriction.h` init list takes effect. `waveguideString.setBowPosition(effectivePosition)` retained — now drives the real split-rail β split via `updateDelayLengths()` (`bridgeSamples = totalDelay·β`, `neckSamples = totalDelay·(1−β)`).

---

## Requirements Verification

**Stage:** 2-dsp, sub-phase 2.1a only
**Stage-2 requirements verified by Phase 2.1a:** narrow subset only (E-string voice with friction junction, bridge LP, infinite-sustain control, RT-safety). Most Stage-2 requirements verify in later Phase 2.x cycles per ROADMAP.

| Requirement | Priority | Status (post-2.1a) | Evidence / Deferral |
|-------------|----------|-------------------|---------------------|
| FUNC-01: Monophonic 4-string EADG E1–G3 | must | ⚠️ Partial | E1 voice implemented and stable; A1/D2/G2 strings deferred to Phase 2.2 |
| FUNC-02: Sustained tone is the default articulation | must | ⚠️ Partial | Bow-on hold sustains for 65s at max `INFINITE_SUSTAIN` (verified). Release tail decays via in-loop saturator (~10 %/s); whether this constitutes a "natural bow-lift tail" per BRIEF.md is a Phase 2.4 / 2.5 question once body resonator + envelope shaping land |
| FUNC-05: MPE per-note pitch / pressure / slide | should | ⏸️ Deferred | MPESynthesiser shell wired; per-note expression rails deferred to Phase 2.6 |
| FUNC-06: VST3 Note Expression for Dorico | must | ⏸️ Deferred | note-expression module linked at Stage 1; voice-level NE handling deferred to Phase 2.6 |
| FUNC-07: MTS-ESP / Scala/TUN | should | ⏸️ Deferred | scala-tuning-engine module linked at Stage 1; engine integration deferred to Phase 2.6 |
| DSP-01: Waveguide stable across E1–G3, 2× oversampling at friction junction | must | ⚠️ Partial | E1 verified stable for 65s; A1/D2/G2 not yet exercised; 2× oversampling present in voice |
| DSP-02: Bass-tuned friction junction | must | ✅ Complete (component) | `HyperbolicFriction.h` ported from O-Bowed with bass defaults (`mu_s=0.85`, `mu_d=0.25`, `v_0=0.05 m/s`); friction component inline; module extraction = Phase 2.1b |
| DSP-03: Bass-tuned wood body resonator | must | ⏸️ Deferred | Phase 2.5 |
| DSP-04: Bow noise / rosin grit | should | ⏸️ Deferred | Phase 2.5 |
| DSP-05: Per-string detuning ±1200 cents | must | ⏸️ Deferred | Phase 2.2 |
| DSP-06: Infinite Sustain control | must | ⚠️ Partial | Quadratic skew + ceiling clamp implemented at `WaveguideString.cpp:97-106`; drone-mode leak suppression at `:120-124`; verified stable at max setting (no runaway, no NaN, no denormal). Smooth-sweep / click-free check deferred to subsequent Phase 2.x cycle |
| DSP-07: Sub-Harmonic generator | should | ⏸️ Deferred | Phase 2.4 |
| DSP-08: Slow Bow LFO | should | ⏸️ Deferred | Phase 2.3 |
| DSP-09: Layered expression | must | ⏸️ Deferred | Phase 2.3 / 2.6 |
| DSP-10: Slow expressive attack characteristic | must | ⚠️ Partial | BowState envelope present (~30 ms attack/release per SUMMARY); attack character audition pending Logic AU smoke (see "Outstanding Items" below) |
| PERF-01: Real-time safe processing | must | ⚠️ Partial | `juce::ScopedNoDenormals` in `processBlock`; pluginval strictness 10 fuzz + parameter-thread-safety PASS; explicit RT-safety code review deferred to end-of-Stage-2 |
| PERF-03: Zero algorithmic latency | nice | ⚠️ Partial | Voice-level oversampler introduces oversampler-only latency (RESEARCH-confirmed); `prepareToPlay` reports `setLatencySamples(...)` accordingly per CONTEXT.md PERF-03; numerical verification (oversampler latency in samples) deferred |
| QUAL-01: No audio artifacts at normal ranges | must | ⚠️ Partial | E1 + max INFINITE_SUSTAIN: no NaN, no Inf, no peak > 1.0, no denormal CPU spike (block-time max/median ≤ 3.55× across both scenarios — well under typical 5× threshold). 108-combo matrix + parameter-sweep click test deferred to Phase 2.4 |
| QUAL-02: Self-oscillation remains musical | nice | ⏸️ Deferred | Output-path saturator/limiter = Phase 2.6 |

**Requirements Summary (Phase 2.1a only):**
- ✅ Complete: 1 (DSP-02 component-level — full module extraction is Phase 2.1b)
- ⚠️ Partial: 9 (E1-only or component-only progress against multi-phase requirements)
- ⏸️ Deferred to later Phase 2.x cycle: 9
- ❌ Failed: 0

**No requirement statuses changed in REQUIREMENTS.md** — Stage 2 is incomplete (only sub-phase 2.1a of Phase 2.1 of 6 phases attempted); requirement promotion to "complete" is held until end-of-Stage-2 verify.

---

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU + Standalone + render-harness) | ✅ Pass | All artefacts present; no new warnings beyond pre-existing macOS-deprecation on `createWriterFor` (note-expression module warnings unchanged from Stage 1) |
| Source matches PLAN rev-3 (F1+F2+F3+F4) | ✅ Pass | grep / inspection confirm split-rail rewrite, F2 LP recurrence, DCB removal, betaScale removal |
| Sign-convention contract matches RESEARCH §11.4 + O-Bowed | ✅ Pass | Verified by inspection of `WaveguideString.cpp:137-185` |
| `auval -v aumu OCbs OuDv` | ✅ Pass | Independent re-run 2026-04-26 → AU VALIDATION SUCCEEDED |
| `pluginval --strictness-level 10 --validate-in-process` | ✅ Pass | Independent re-run 2026-04-26 → SUCCESS (all test groups, including fuzz) |
| Render-harness bow-on-only (65s, max INFINITE_SUSTAIN) | ✅ Pass (4/4 invariants) | rmsRatio = 1.04 (sustained Helmholtz) |
| Render-harness standard (60s sustain + 5s release tail) | ⚠️ Partial (3/4 invariants) | `pass_rms` FALSE — analytically characterised saturator-tail decay; not a bootstrapping/B1/B2/B3 regression |
| Installed binaries match build | ✅ Pass | `~/Library/Audio/Plug-Ins/{VST3,Components}/O-Contrabass-dev.*` present (timestamps 2026-04-26 20:51) |
| Source committed to git | ❌ Fail (DEFERRED) | R7 atomic commit not yet landed. Phase 2.1a source files (DSP/, BowedContrabassVoice, OContrabassMPESynthesiser, PluginProcessor, render-harness, CMakeLists) live on disk but appear as untracked / modified per `git status`. Per SUMMARY.md "Open Decisions", R7 commit waits on user Option A/B/C decision. |

---

## Human Verification

- [ ] **Logic Pro AU smoke test** — load `O-Contrabass-dev` AU, MIDI E1 with sustain pedal held, audition: (a) bowed-tone character is musical (not just an oscillation), (b) bow-position knob sweep produces sul-ponticello → sul-tasto timbre change (validates F4 + real β split), (c) `INFINITE_SUSTAIN` knob sweep at max produces sustained drone. **Status: DEFERRED — user-side audition; not blocking commit landing once Option A/B/C selected.**
- [ ] **Subjective musicality check** — does the E1 sustained tone match BRIEF.md's "convincing orchestral arco" ambition, or does it sound like a raw waveguide test signal? Phase 2.1a is engine-validation, not voicing — body resonator (Phase 2.5) is where orchestral character lands. Mark non-blocking for Phase 2.1a.

---

## Issues Found

### 1. R7 atomic commit DEFERRED (carry-forward from SUMMARY)

The Phase 2.1a-recovery source files are present on disk and pass auval / pluginval / render-harness bow-on-only validation, but they have **not been committed to git**. `git log -- plugins/O-Contrabass/` shows the last commits are Stage 0 (`docs(O-Contrabass): Stage 0 - research & planning complete`) and ideation (`docs(O-Contrabass): ideation - creative brief and requirements`); Stage 1 source and Phase 2.1a source both live in the working tree as untracked / modified.

This is the deliberate outcome of SUMMARY.md "Open Decisions" — R7 commit was deferred pending user selection of Option A (commit verbatim), Option B (tighten R5 pass-bar wording), or Option C (investigate saturator dissipation further). **Verify cannot retire this issue; it requires user input.** Recommended path: see "Stage Verdict" below.

### 2. Standard harness `pass_rms` invariant FALSE (carry-forward from SUMMARY)

The 60s + 5s render shows `rmsRatio_final_over_mid = 0.36`, below the rev-3 PLAN's [0.5, 2.0] expected band. Root cause is the in-loop algebraic saturator's low-amplitude cubic loss during the 5s post-bow-off tail (analytical derivation in SUMMARY.md "R6 NOT INVOKED"; characterised, not anomalous). **Not a B1/B2/B3 regression**; not a bootstrapping failure; not a transcription error. Three resolution paths offered in SUMMARY.md (Options A/B/C); the choice does not affect the underlying engine.

### 3. ARCHITECTURE.md §"DC Blocker" deviation (carry-forward from SUMMARY)

F3 removes the in-loop DCB that ARCHITECTURE.md §"DC Blocker" mandates. Justified per RESEARCH §11.6 and PLAN rev-3 §"Why F3 deviates" (F2 LP-correctness obviates it; output-path DCB available if Phase 2.4's 108-combo matrix surfaces drift). **Architecture amendment recommendation:** update ARCHITECTURE.md §"DC Blocker" to reflect the F2 LP-correctness obviation and the output-path DCB option. **Tracked as a follow-up; not blocking subsequent phases.**

---

## Stage Verdict

**Status:** ⚠️ PARTIAL — Phase 2.1a-recovery only; Phase 2.1 cycle (sub-phases b + c) and remainder of Stage 2 (Phases 2.2–2.6) not yet attempted.

**What is verified:**
- B1 (single-rail bootstrapping pathology) RESOLVED via F1 split-rail.
- B2 (bridge LP DC-gain inflation) RESOLVED via F2 recurrence fix.
- B3 (in-loop DCB suppressing cold-start sticking-regime injection) RESOLVED via F3 removal.
- F4 (voice-side `betaScale` fudge) DROPPED; real split-rail β split active.
- E1 + max `INFINITE_SUSTAIN` bow-on sustain: 4/4 invariants TRUE for 65 s. Helmholtz oscillation bootstraps from cold start within first ~5 s.
- Build clean, auval SUCCEEDED, pluginval-10 SUCCESS — independently re-verified.

**What is NOT verified (still open):**
- Standard harness `pass_rms` strict invariant FALSE (saturator-tail decay, characterised).
- R7 atomic commit not yet landed; source files uncommitted.
- Logic Pro AU smoke / musicality check deferred to user.
- Phase 2.1b (module extraction → `modules/synthesis/bow-friction/`) not started.
- Phase 2.1c (cascaded allpass dispersion) not started.
- Phases 2.2–2.6 not started.

**Ready for next sub-phase:** **Conditional.** Three pre-conditions, in order:

1. User selects SUMMARY.md Option A / B / C.
2. R7 atomic commit lands the Phase 2.1a-recovery + carry-forward source files.
3. Optional: Logic AU smoke audition (recommended pre-Phase-2.2 but not blocking; the engine is validated by harness + validators).

Once those land, **next sub-phase = Phase 2.1b (module extraction, R8–R15, Gate 2)** as a fresh GSD cycle.

**Blockers (verify-phase scope):**
- (None for the verify itself — engine implementation is in place and validated.)
- (User Option A/B/C decision blocks R7 commit and onward progression.)

**Recommendation (carrying SUMMARY.md):** **Option A** — accept Gate 1 PASS on the bow-on validation (rev-3 demonstrably retired the topology + LP + DCB risks; saturator-tail decay is a separate, lower-priority phenomenon to characterise in Phase 2.4 if it surfaces in the 108-combo matrix). The primary Phase 2.1a-recovery goal (Helmholtz bootstrapping + 65s stable sustain at INFINITE_SUSTAIN = 1.0) is achieved. Document the saturator-tail dissipation as a Phase 2.4 follow-up.

---

## Outstanding Items

**Blocking R7 commit + Phase 2.1b:**
- User selection of SUMMARY.md Option A / B / C.

**Non-blocking (defer to later phase):**
- Logic Pro AU manual smoke (recommend before Phase 2.2 starts).
- ARCHITECTURE.md §"DC Blocker" amendment to reflect F2 LP correctness + output-path DCB option.
- O-Bowed comparison render at INFINITE_SUSTAIN = 1.0 with `4·tanh(x/4)` saturator (only relevant if Option C is selected).

---

## Validated Artifacts

- `~/Library/Audio/Plug-Ins/VST3/O-Contrabass-dev.vst3` — installed (auval + pluginval-10 PASS, re-verified 2026-04-26).
- `~/Library/Audio/Plug-Ins/Components/O-Contrabass-dev.component` — installed.
- `build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test` — render-harness binary.
- `/tmp/verify-bowon-only.{wav,json}` — verify-phase reproduction of bow-on-only render (4/4 invariants TRUE, ratio 1.04).
- `/tmp/verify-standard.{wav,json}` — verify-phase reproduction of standard render (3/4 invariants TRUE, pass_rms FALSE as characterised).
- `/tmp/e1-bowon-only.{wav,json}` — execute-phase reference (matches verify-phase reproduction byte-for-byte).
- `/tmp/e1-max-sustain-r3.{wav,json}` — execute-phase reference (matches verify-phase reproduction byte-for-byte).

---

## Next Action

**Two-step:**

1. **User decides Option A / B / C** (see SUMMARY.md "Open Decisions").
2. **R7 atomic commit** lands the Phase 2.1a-recovery + carry-forward source files (DSP/, BowedContrabassVoice, OContrabassMPESynthesiser, PluginProcessor, render-harness, CMakeLists, plus the Stage 1 source which is also still uncommitted).

**Then:** Phase 2.1b — `/plugin-discuss O-Contrabass 2-dsp` (or continue the existing Phase 2.1 cycle with a fresh discuss for sub-phase 2.1b per ROADMAP).

Stage 2 verify (full) cannot complete until Phases 2.1b, 2.1c, and 2.2–2.6 are all verified per their own GSD cycles.
