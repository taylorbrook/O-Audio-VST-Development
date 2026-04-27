# Stage 2: DSP — Context (rev-2)

**Date:** 2026-04-26
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.1a closure (R7 commit) + Phase 2.1b opening (module extraction, Gate 2)**
**Supersedes:** rev-1 (Phase 2.1 broad discuss, dated 2026-04-26 earlier the same day) — rev-1 contracts that remain locked are inherited verbatim and not re-litigated.

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle is a continuation of the Phase 2.1 GSD cycle, called after Phase 2.1a-recovery verify returned ⚠️ PARTIAL (engine validated under bow-on, standard `pass_rms` invariant FALSE due to characterised saturator-tail decay, R7 commit deferred pending user Option A/B/C decision).

The cycle scope is **dual**: (1) close Phase 2.1a by resolving the Option A/B/C decision so R7 atomic commit can land, and (2) open Phase 2.1b by scoping the module extraction (`modules/synthesis/bow-friction/`) + O-Bowed regression bar so research+plan+execute can proceed against Gate 2.

Phase 2.1c (cascaded allpass dispersion, Gate 3) and Phases 2.2–2.6 remain out of scope for this discuss — each gets its own fresh GSD cycle once 2.1b verifies.

---

## Cycle Scope

### Part A — Phase 2.1a closure (R7 atomic commit)

**Decision:** **Option A** — accept Gate 1 PASS on bow-on validation, commit rev-3 verbatim.

Rationale per SUMMARY.md "Open Decisions" + VERIFICATION.md "Stage Verdict": rev-3 demonstrably retired the topology + LP + DCB risks (B1/B2/B3) and dropped the F4 betaScale fudge. Bow-on validation 4/4 invariants TRUE for 65 s at INFINITE_SUSTAIN = 1.0. auval/pluginval-10 PASS. The standard `pass_rms` invariant FALSE on the 5 s post-bow-off tail is a characterised low-amplitude cubic-loss in the in-loop algebraic saturator (`x/sqrt(1+x²)` × 2 rails × 41.2 RTs/s ≈ 10 %/s free-decay) — analytically derived in SUMMARY.md "R6 NOT INVOKED", not a bootstrapping failure, not a B1/B2/B3 regression, not a transcription bug. The primary Phase 2.1a-recovery goal (Helmholtz bootstrapping + 65 s stable sustain) is achieved.

**R7 atomic commit lands the following working-tree files in a single commit:**
- `plugins/O-Contrabass/CMakeLists.txt`
- `plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}` (Stage 1 carry-forward + Phase 2.1a synth wiring)
- `plugins/O-Contrabass/Source/PluginEditor.{h,cpp}` (Stage 1 stub)
- `plugins/O-Contrabass/Source/DSP/HyperbolicFriction.h`
- `plugins/O-Contrabass/Source/DSP/BowModel.{h,cpp}`
- `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}` (rev-3 split-rail + F2 LP + F3 DCB removal)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.{h,cpp}` (rev-3 F4 betaScale removal)
- `plugins/O-Contrabass/Source/OContrabassMPESynthesiser.h`
- `plugins/O-Contrabass/tests/render-harness/{CMakeLists.txt,main.cpp}`
- `plugins/O-Contrabass/.planning/parameter-spec.md` (promoted from draft)
- `plugins/O-Contrabass/.planning/stages/{1-foundation,2-dsp}/*` (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION/CHECKPOINT)
- `plugins/O-Contrabass/.planning/STATUS.md` updated post-commit

**Commit message body must explicitly note:**
- F1+F2+F3+F4 coupled fix (RESEARCH §11 root-cause).
- F3 deviates from ARCHITECTURE.md §"DC Blocker" — justified per RESEARCH §11.6 (F2 LP correctness obviates in-loop DCB).
- Gate 1 PASS on bow-on validation; standard harness `pass_rms` FALSE characterised as Phase 2.4 follow-up.

### Part B — Phase 2.1b opening (module extraction, Gate 2)

**Goal:** Extract the friction-junction support classes from O-Bowed into a shared module `modules/synthesis/bow-friction/`. Both O-Bowed (existing source-of-truth) and O-Contrabass (newly-validated consumer) switch to consuming the module simultaneously to avoid duplicate maintenance.

**Module surface (corrected from rev-1):**
- `HyperbolicFriction` — the friction junction class (lookup/eval; parametrised via `mu_s`, `mu_d`, `v_0`, `R_s`)
- `BowModel` — bow envelope state (attack/release, velocity ramp, applied bow force)

**Note:** rev-1 CONTEXT named `HyperbolicBowTable`, `BowState`, `SchellengGuard` — these are aspirational names; the actual O-Bowed source-of-truth has only `HyperbolicFriction` (HyperbolicFriction.h, 55 LOC) and `BowModel` (BowModel.h 51 LOC + .cpp 97 LOC). No `SchellengGuard` class exists. Phase 2.1b extracts what actually exists; Schelleng-guard logic (if needed in future) is a Phase 2.3 concern (Slow Bow LFO + Schelleng wedge clamp) and gets added to the module then.

**Gate 2 pass-bar (locked here):**
1. **O-Bowed bit-exact regression on canonical preset:** Render O-Bowed at one canonical preset (recommended: default A4 sustained, ~5 s at default bow params, INFINITE_SUSTAIN OFF, no detune/vibrato/sub-harmonics) **before** module extraction → save `o-bowed-pre-extraction-canonical.wav` as golden reference. Re-render after switching O-Bowed to consume the module → `o-bowed-post-extraction-canonical.wav`. `cmp` byte-equality required. (Friction module is pure value-class code: deterministic.)
2. O-Contrabass render-harness (bow-on-only, 65 s, INFINITE_SUSTAIN = 1.0) — must continue to PASS 4/4 invariants byte-identical to the Phase 2.1a-recovery reference (`/tmp/e1-bowon-only.json`).
3. `auval -v aumu OCbs OuDv` PASS for O-Contrabass.
4. `pluginval --strictness-level 10 --validate-in-process` PASS for both O-Bowed and O-Contrabass.
5. Both plugins build clean (no new warnings beyond pre-existing macOS-deprecation on `createWriterFor`).
6. `modules/registry.yaml` updated with `bow-friction` entry under `synthesis` category, version 1.0.0.

**Gate 2 atomic commit:** R15 (per PLAN rev-3 carry-forward numbering) lands all module-extraction files + both plugin CMakeLists switches + both plugin source switches in one commit, only on Gate 2 PASS.

---

## Requirements Confirmed (carry-forward from rev-1, scoped to Phase 2.1)

These are the Phase-2.1-relevant subsets of the locked contracts:

- **FUNC-02** (sustained tone, no runaway, no NaN): validated for E1 in 2.1a-recovery; carry-forward to 2.1b (module extraction must not regress).
- **DSP-01** (waveguide string, Lagrange3rd, 8192-sample buffer): implemented for E1 in 2.1a-recovery (split-rail per rev-3); 2.1b does NOT touch `WaveguideString` (saturator differs between O-Bowed `4·tanh(x/4)` and O-Contrabass `x/sqrt(1+x²)`; promoting WaveguideString to module would require saturator-template parameter; deferred indefinitely).
- **DSP-02** (hyperbolic friction junction, 2× oversampled): component-complete in 2.1a-recovery (inline copy with bass defaults); module-complete after 2.1b.
- **DSP-03** (cascaded allpass dispersion, M=4 for E-string): out of scope for 2.1b — stays in 2.1c.
- **DSP-04** (DC blocker R=0.999 + in-loop saturator): **deviation in effect** (F3 removed in-loop DCB); ARCHITECTURE.md §"DC Blocker" amendment deferred to end-of-Stage-2 verify.
- **PERF-01** (no allocations, no locks, no file I/O in `processBlock`): enforced; 2.1b extraction must not violate.
- **PERF-02** (< 5% CPU on M1): tracked.
- **PERF-03** (latency = oversampler only): in effect; 2.1b extraction must not change reported latency.
- **QUAL-01** (no audible clicks during parameter sweeps): validated for `INFINITE_SUSTAIN` at max in 2.1a-recovery; carry-forward.

Out of scope for this cycle (deferred to later Phase 2.x cycles per ROADMAP):
- A1/D2/G2 strings + per-string detune (Phase 2.2)
- Vibrato + Slow-Bow LFO + Schelleng wedge clamp (Phase 2.3)
- Sub-harmonic bias + 108-combo stability matrix (Phase 2.4) — saturator-tail dissipation re-evaluated here
- Body resonator + bow noise (Phase 2.5)
- Master saturator/limiter, stereo width, microtonal, MPE (Phase 2.6)

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**
- All 29 APVTS parameter IDs, ranges, skews — `parameter-spec.md` (sha256:c47fe736…)
- DSP architecture (`research/ARCHITECTURE.md`, sha256:3cb26814…) — F3 deviation flagged but amendment deferred to end-of-Stage-2 verify
- ROADMAP phasing (sha256:106639f6…)

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**
- `getLatencySamples()` is non-virtual — keep using `setLatencySamples()` in `prepareToPlay`.
- `juce::ScopedNoDenormals` at `processBlock` entry (mandatory).
- `IS_SYNTH TRUE` + output-only `BusesProperties` already in place from Stage 1.
- Both WebView2 flags already in place from Stage 1.

**Module-extraction constraints:**
- O-Bowed must show **bit-exact regression on the canonical preset** after switching to the shared `bow-friction` module (Gate 2.1).
- Both plugins switch simultaneously in the R15 atomic commit (no flag-day window where one consumes the module and the other does not).
- Module home: `modules/synthesis/bow-friction/` (registry category: `synthesis` — confirmed from rev-1, no registry-schema change required).
- Module name: `ouaricon_bow_friction` (matches existing `ouaricon_*` convention).
- Module surface: `HyperbolicFriction` + `BowModel` ONLY. No `WaveguideString`, no saturator, no `BowState`/`SchellengGuard` (don't exist as classes).

**Phase-2.1a-recovery state preservation:**
- The R7 commit must land verbatim — no source edits between SUMMARY.md's recorded state and the commit. Working-tree byte-stability already verified (verify-phase reproduction matched execute-phase JSON outputs byte-for-byte).
- Saturator-tail dissipation parked as Phase 2.4 follow-up + RESEARCH §12 footnote (added during Phase 2.1b research-phase or as a small standalone update — see "Open Questions" #1 below).

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Phase 2.1a `pass_rms` resolution | **Option A** — accept Gate 1 on bow-on validation, commit rev-3 verbatim | rev-3 retired B1/B2/B3 + F4 (the rev-3 PLAN's stated failure modes). Bow-on validation directly demonstrates the Phase 2.1a-recovery goal. Saturator-tail decay is a separate, lower-priority phenomenon — addressing it now expands scope beyond the gate-first cycle. |
| Saturator-tail follow-up parking | **Phase 2.4 follow-up note + RESEARCH §12 footnote** | Capture the analytical derivation (≈10%/s free-decay = x²/2 cubic-loss × 2 rails × 41.2 RTs/s) in a new RESEARCH §12 (Phase 2.4 follow-up tracker). Re-evaluate during Phase 2.4's 108-combo stability matrix if longer free-decay characterisation surfaces a real problem. Defers ARCHITECTURE saturator-choice litigation. |
| ARCHITECTURE.md §"DC Blocker" amendment | **Defer to end-of-Stage-2 verify** | Avoid mid-stage architecture churn. Amendment can reflect any output-path DCB additions surfacing in 2.4/2.5 once all Phase 2.x cycles complete. F3 deviation tracked in PLAN rev-3 + SUMMARY.md + VERIFICATION.md + R7 commit-message body — sufficient audit trail. |
| Phase 2.1b cycle scope | **2.1b only** — module extraction + O-Bowed bit-exact regression + both-plugins switch | Phase 2.1c (dispersion) and Phases 2.2–2.6 each get fresh GSD cycles. Matches rev-1 gate-first principle: each sub-phase must verify before the next opens. |
| Module home / name | **`modules/synthesis/bow-friction/`** + `ouaricon_bow_friction` (rev-1 carry-forward) | Confirmed from rev-1 §"Approach Decisions". Registry has no `dsp` category; `synthesis` is the closest semantic fit. No registry-schema change required. |
| Module extraction surface | **`HyperbolicFriction` + `BowModel`** (correction from rev-1) | rev-1's `HyperbolicBowTable`/`BowState`/`SchellengGuard` were aspirational; actual O-Bowed source-of-truth has only the two named classes. `WaveguideString` stays per-plugin (saturator differs between plugins). Cleanest surface; minimal coupling. |
| O-Bowed regression bar | **Bit-exact WAV diff on canonical preset** | Friction module is pure value-class code → deterministic. `cmp` byte-equality is the strongest, cheapest-to-automate regression bar. One canonical preset (default A4 sustained, ~5 s, no detune/vibrato/sub-harmonics, INFINITE_SUSTAIN OFF) is enough at Gate 2; multi-preset matrix can be added in a later cycle if needed. |
| Logic Pro AU smoke timing | **Post-R7-commit, before Phase 2.1b execute** | R7 lands first (auval/pluginval/harness already PASS — sufficient for engine commit). User auditions in Logic between R7 commit and 2.1b research-phase kick-off. If audition reveals an unexpected sonic issue, file as 2.1a bug and pause 2.1b. Fastest path; commit captures clean engine state. |
| Both-plugins switch model | **Atomic R15 commit** (both CMakeLists + both source-include switches) | No flag-day window where one plugin consumes the module and the other still has inline copy. Matches rev-1 §"Approach Decisions" intent. Risk: O-Contrabass build needs O-Bowed extraction to land — handled by sequencing R15 as a single commit covering both. |
| Phase 2.1b primary listening DAW | **Logic Pro (AU)** (rev-1 carry-forward) | Closest to validated O-Lyrica/O-Bowed reference workflow. AU is the stricter validation surface. Manual smoke after R15 commit confirms both plugins still sound right. |

---

## Phase 2.1b Test Criteria (locked)

These are the exit criteria that Phase 2.1b verify must satisfy (Gate 2):

- [ ] **O-Bowed pre-extraction render captured:** `o-bowed-pre-extraction-canonical.wav` saved before any source edits (golden reference).
- [ ] **O-Bowed post-extraction render bit-equal:** `cmp o-bowed-pre-extraction-canonical.wav o-bowed-post-extraction-canonical.wav` returns 0 (byte-equal).
- [ ] **O-Contrabass render-harness bow-on-only PASS:** 4/4 invariants TRUE, JSON byte-identical to `/tmp/e1-bowon-only.json` reference.
- [ ] **`auval -v aumu OCbs OuDv` PASS** for O-Contrabass.
- [ ] **`auval -v aumu OBow OuDv` PASS** for O-Bowed.
- [ ] **`pluginval --strictness-level 10 --validate-in-process` PASS** for both O-Bowed and O-Contrabass VST3.
- [ ] **Build clean** for both plugins (no new warnings beyond pre-existing macOS-deprecation on `createWriterFor`).
- [ ] **`modules/registry.yaml`** updated with `bow-friction` entry under `synthesis` category, version 1.0.0.
- [ ] **R15 atomic commit landed** — module + both plugin switches + registry update in one commit.

---

## Open Questions (for research phase)

The following five questions remain genuinely-open and are handed to Phase 2.1b research:

1. **Saturator-tail RESEARCH §12 update — when?** Add the analytical derivation (≈10%/s free-decay characterisation) to RESEARCH.md as a new §12 ("Phase 2.4 follow-up: saturator-tail dissipation"). Two timings:
   - (a) During 2.1b research-phase (cleanest — research-phase already opens RESEARCH.md), OR
   - (b) As a small standalone update with the R7 commit (keeps research-phase scope tight).
   **Recommend (a)** — research-phase naturally documents follow-ups; no need for a special-purpose commit.

2. **Module CMakeLists pattern selection.** rev-1 RESEARCH §1 confirmed two extant patterns:
   - Pattern A: `ouaricon_add_module(...)` (used by `note-expression` — per-format routing required)
   - Pattern B: explicit file refs (used by `scala-tuning-engine` — sibling-plugin convention)
   Phase 2.1b research must pick one and pattern-confirm against an existing module's CMakeLists structure. **Recommend Pattern A** (`ouaricon_add_module`) — friction module is single-format, no per-format routing complexity; matches the convention used by the lighter-weight `note-expression`.

3. **Header layout / public-API surface.** What headers does the module expose? Options:
   - `bow-friction/include/bow-friction/HyperbolicFriction.h` + `bow-friction/include/bow-friction/BowModel.h` (two headers, `bow-friction/HyperbolicFriction.h` namespace-qualified include path)
   - `bow-friction/include/bow-friction/bow-friction.h` (single umbrella header re-exporting both)
   - Mirror existing modules (`note-expression`, `scala-tuning-engine`) — research-phase pattern-confirms.
   **Recommend:** mirror existing modules' header convention — Phase 2.1b research-phase confirms.

4. **Plugin-side include-switch mechanics.** When both plugins switch from `#include "DSP/HyperbolicFriction.h"` → module include, does the inline-copy header file remain (as a thin shim re-including the module header for source-compatibility), or is it deleted entirely from the plugin source tree?
   **Recommend:** delete entirely. Cleaner state; no risk of include shadowing. Plugin sources change `#include` lines + lose two files; CMakeLists drops the corresponding `target_sources`.

5. **Bass-default propagation.** O-Contrabass currently sets `mu_s=0.85`, `mu_d=0.25`, `v_0=0.05 m/s` via `HyperbolicFriction.h` init list (verbatim port + 3 init-list edits per SUMMARY.md). After extraction, the module's `HyperbolicFriction` exposes default-construction defaults (likely O-Bowed's tighter values). Three options:
   - (a) Module exposes ctor / setter API; O-Contrabass voice calls setters in `prepareToPlay` to restore bass tuning.
   - (b) Module exposes a `BassPreset` factory (`HyperbolicFriction::createBassDefaults()`); O-Contrabass voice consumes the factory.
   - (c) Module ctor takes a `Defaults` struct; both plugins pass their own.
   **Recommend (a) — setter API:** smallest module surface change; reuses existing patterns (parameters get applied per-block anyway, so `prepareToPlay` setter is the natural injection point).

---

## Files / Artefacts to Produce in Phase 2.1b

**Module (new):**
- `modules/synthesis/bow-friction/CMakeLists.txt`
- `modules/synthesis/bow-friction/include/bow-friction/HyperbolicFriction.h` (extracted from `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h`)
- `modules/synthesis/bow-friction/include/bow-friction/BowModel.h` (extracted)
- `modules/synthesis/bow-friction/src/BowModel.cpp` (extracted)
- (Optional umbrella) `modules/synthesis/bow-friction/include/bow-friction/bow-friction.h`
- `modules/synthesis/bow-friction/registry-entry.yaml` (or equivalent — match existing module convention)

**Registry (updated):**
- `modules/registry.yaml` — add `bow-friction` entry under `synthesis` category, version 1.0.0.

**Plugin source changes (both plugins):**
- `plugins/O-Bowed/CMakeLists.txt` — add `bow-friction` module dependency, drop friction `target_sources`.
- `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h` — DELETED.
- `plugins/O-Bowed/Source/DSP/BowModel.{h,cpp}` — DELETED.
- `plugins/O-Bowed/Source/**/*.{h,cpp}` — `#include` line updates.
- `plugins/O-Contrabass/CMakeLists.txt` — add `bow-friction` module dependency, drop friction `target_sources`.
- `plugins/O-Contrabass/Source/DSP/HyperbolicFriction.h` — DELETED.
- `plugins/O-Contrabass/Source/DSP/BowModel.{h,cpp}` — DELETED.
- `plugins/O-Contrabass/Source/**/*.{h,cpp}` — `#include` line updates + `prepareToPlay` setter calls for bass defaults (per Open Question #5 recommendation).

**Test artefacts:**
- `o-bowed-pre-extraction-canonical.wav` — golden reference (captured BEFORE any source edits).
- `o-bowed-post-extraction-canonical.wav` — Gate 2.1 verification render.
- `cmp` invocation logged in SUMMARY.md.

**Verification artefacts:**
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` — append §12 (saturator-tail Phase 2.4 follow-up) + §13 (Phase 2.1b module-extraction research).
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` — append rev-4 (R8–R15 module-extraction tasks; carries forward verbatim from rev-3 §Phase 2.1b task bodies).
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` — append "Phase 2.1b verify" section with Gate 2 pass-bar evidence.

---

## Next Phase

Ready for: **research** phase — `/plugin-research O-Contrabass 2-dsp`

Research focus (rev-2):

1. **First action: R7 atomic commit lands.** Phase 2.1a-recovery work + Stage 1 carry-forward + parameter-spec promotion → single commit per CLAUDE.md commit conventions. Commit-message body explicitly notes F3 ARCHITECTURE deviation + saturator-tail Phase 2.4 follow-up. (Mechanical; could be done before research-phase opens, but cleaner to bundle with the research-phase scoping pass.)
2. **RESEARCH §12 update:** Document saturator-tail dissipation as Phase 2.4 follow-up (analytical derivation: x²/2 cubic-loss × 2 rails × 41.2 RTs/s ≈ 10%/s free-decay; expected impact in Phase 2.4's 108-combo matrix).
3. **Pattern-confirm module extraction** against `note-expression` (Pattern A — `ouaricon_add_module`) — confirm CMakeLists structure, registry-entry YAML schema, header layout convention.
4. **Resolve Open Questions #2–#5** (CMakeLists pattern, header layout, include-switch mechanics, bass-default propagation).
5. **Pre-flight bit-exact rendering test on O-Bowed CURRENT state** — render `o-bowed-pre-extraction-canonical.wav` BEFORE any source edits (this is the golden reference that R15's Gate 2.1 compares against). Log render command + JSON metadata + WAV checksum.
6. **Confirm O-Bowed canonical preset** — research-phase locks the exact preset (default A4 sustained, ~5 s, no detune/vibrato/sub-harmonics, INFINITE_SUSTAIN OFF) and renders the golden reference.

After research: plan-phase (PLAN.md rev-4) writes R8–R15 task breakdown verbatim against this CONTEXT + research findings; execute-phase performs the extraction + R15 atomic commit; verify-phase confirms Gate 2 invariants + Logic AU smoke.

---

## Audit Trail (rev-2 supersedes rev-1)

**rev-1 (earlier 2026-04-26):** Phase 2.1 broad discuss. Cycle scope = Phase 2.1 only (sub-phases a/b/c). 7 approach decisions, 5 open questions. Plan/research/execute consumed rev-1 directly through Phase 2.1a-recovery (rev-3 PLAN). Verify returned ⚠️ PARTIAL.

**rev-2 (this document):** Continuation discuss after Phase 2.1a-recovery verify. Closes Phase 2.1a (Option A, R7 commit, saturator-tail parked) + opens Phase 2.1b (module extraction, Gate 2). Phase 2.1c dispersion stays out of scope for the next research/plan/execute pass and gets its own fresh discuss after 2.1b verifies.

**Inherited verbatim from rev-1 (not re-litigated):**
- Cycle-scope gate-first principle
- Module home: `modules/synthesis/bow-friction/`
- Module name: `ouaricon_bow_friction`
- Phase 2.1 stability fallback strategy
- Phase 2.1 MIDI trigger model (real note-on → E1 voice)
- Stability test harness pattern (automated render-to-WAV + invariant checks)
- Primary listening DAW: Logic Pro (AU)
- Sample-rate strategy: internal 88.2 / 96 kHz at friction junction
- All 9 Phase 2.1 test criteria (carried into Gate 2 + Gate 3 as appropriate)

**Corrected from rev-1:**
- Module extraction surface: `HyperbolicFriction` + `BowModel` (NOT `HyperbolicBowTable`/`BowState`/`SchellengGuard` — those aren't real classes in the O-Bowed source-of-truth).

**New in rev-2:**
- Option A locked for Phase 2.1a closure
- Saturator-tail Phase 2.4 follow-up + RESEARCH §12 destination
- ARCH §"DC Blocker" amendment deferred to end-of-Stage-2 verify
- O-Bowed regression bar locked at bit-exact WAV diff on canonical preset
- Logic AU smoke timing locked at post-R7 / pre-2.1b-execute
- Both-plugins atomic R15 switch model
