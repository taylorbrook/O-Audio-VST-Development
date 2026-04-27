---
plugin: O-Contrabass
stage: 2
phase: research
status: phase_2_1_research_rev2_complete_R7_landed
last_updated: 2026-04-26
complexity_score: 5.0
complexity_score_raw: 16.0
complexity_tier: 6
research_depth: DEEP
staged_implementation: true
orchestration_mode: true
cycle_scope: phase_2_1a_closed_phase_2_1b_open
next_action: plan_phase_rev4  # /plugin-plan O-Contrabass 2-dsp
next_stage: 2
ready_for_implementation: false  # plan+execute pending for 2.1b (R8–R15)
plan_revision: 3
plan_revision_status: rev_4_pending  # rev-3 landed via R7; rev-4 carries forward §13.6 task table for 2.1b
research_revision: 2
research_revision_status: complete  # §12 saturator-tail appended; §13 module-extraction Q2-Q5 resolved + canonical preset locked
context_revision: 2
context_revision_status: locked_phase_2_1a_close_plus_2_1b_open
checkpoint: stages/2-dsp/CHECKPOINT-2.1a.md
recovery_research: stages/2-dsp/RESEARCH.md#10-re-research-after-phase-21a-harness-failure-2026-04-26
execute_summary: stages/2-dsp/SUMMARY.md
verify_report: stages/2-dsp/VERIFICATION.md
r1_diagnostic_result: FAIL  # peak=-32.6dBFS, rmsMid=2.23e-8, rmsFinal=0.0, pass_rms=false
phase_2_1a_decision: option_a_accept_gate_1_commit_rev3_verbatim
phase_2_1a_followup_park: phase_2_4_with_research_section_12_footnote
arch_amendment_dc_blocker_timing: end_of_stage_2_verify
arch_amendment_inloop_saturator_timing: end_of_stage_2_verify_conditional_on_phase_2_4_triggers  # see RESEARCH §12.5
phase_2_1b_regression_bar: bit_exact_wav_diff_canonical_preset
phase_2_1b_module_surface: hyperbolic_friction_plus_bow_model_only
phase_2_1b_au_smoke_timing: post_r7_commit_pre_2_1b_execute
phase_2_1b_canonical_preset: a4_vel0_7_5s_factory_defaults_no_release  # see RESEARCH §13.4
phase_2_1b_o_bowed_harness_status: not_yet_built  # PLAN rev-4 R8 mirrors O-Contrabass harness; see RESEARCH §13.5
gate_state:
  build: PASS
  auval: PASS  # re-verified 2026-04-26 by /plugin-verify
  pluginval_strictness_10: PASS  # re-verified 2026-04-26 by /plugin-verify
  render_harness_60s_e1: FAIL_rms_ratio_only_post_bow_off_characterised_park_phase_2_4  # not a regression
  render_harness_65s_bowon_only: PASS_4_of_4  # reproduced byte-identically
  source_committed_to_git: PASS  # R7 atomic commit landed 2026-04-26
gate_1_outcome: PARTIAL_PASS_option_a_locked
verify_outcome: PARTIAL_engine_validated_r7_landed_2026_04_26
contract_checksums:
  brief: sha256:6ea840bbe4c34855397111896b9f6fd52cb5f66c597676d300b0c5ce33c8988d
  parameter_spec: sha256:c47fe7361a55e1d64b906ef7194894f4a2490744b35a644c76b6e1a632282d0d
  architecture: sha256:3cb26814bcd830cfba0b3bba42c096bdbf5b1449f52825a167cde09e114855a0
  roadmap: sha256:106639f633b6b3a2cfeb41eb07640d3ac0e01ed0832c33a9da45faf2b97aca7e
---

# O-Contrabass Status

## Current Position

Stage: 2 of 4 (DSP) — Phase 2.1 cycle in progress (2.1a ✅ CLOSED via R7 atomic commit; 2.1b research ✅ COMPLETE; 2.1b plan/execute pending; 2.1c not started)
Phase: research (rev-2 / Phase 2.1a-close + Phase 2.1b-open) **COMPLETE — RESEARCH §12 (saturator-tail Phase 2.4 follow-up) and §13 (Phase 2.1b module-extraction Q2-Q5 resolution + canonical preset + O-Bowed harness spec) appended 2026-04-26; R7 atomic commit landed; ready for plan-phase rev-4**
Cycle Scope: **Phase 2.1a closure (R7 commit ✅ landed) + Phase 2.1b opening (module extraction, Gate 2)** — Phase 2.1c (dispersion, Gate 3) and Phases 2.2–2.6 still get fresh GSD cycles each
Status: R7 atomic commit landed 2026-04-26 — Phase 2.1a-recovery source (split-rail WaveguideString, F2 bridge LP fix, F3 DCB removal, F4 betaScale removal) + Stage 1 carry-forward (CMakeLists, PluginProcessor stub, PluginEditor stub) + parameter-spec promotion + planning artefacts (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION rev-3 + CHECKPOINT-2.1a + Stage 1 set) + 4 Stage 0 deep-research docs + render-harness binary now committed. RESEARCH §12 (saturator-tail Phase 2.4 follow-up — analytical envelope ≈10%/s = x²/2 × 2 rails × 41.2 RTs/s) and §13 (module-extraction Q2-Q5: Pattern A `ouaricon_add_module`, no umbrella header, delete inline copies, setter-API for bass defaults; canonical preset = A4 vel 0.7 5s factory defaults; O-Bowed render-harness spec mirrors O-Contrabass) appended.
Progress: [##################..] 90%

## Completed So Far

**Ideation:** Complete
- Core concept defined: bass-only specialized 4-string contrabass physical model, sustained orchestral arco + ambient drone
- Differentiation from O-Bowed clarified (deep specialization vs general-purpose)
- Parameters specified across 8 sections (Bow / Body / Strings / Detune / Expression / Drone / Output / Microtonal) — 29 total
- Sonic targets locked: deep wood body resonance, bow noise / rosin grit, slow expressive attack
- Drone-first features designed in (infinite sustain, sub-harmonics, slow-bow LFO, per-string detuning)
- Layered expression model defined (intrinsic CC + dedicated vibrato + Expression Macro)
- Full Ouaricon microtonal convention (Note Expression + MTS-ESP + Scala/TUN + MPE)
- Requirements extracted: 14 must / 7 should / 3 nice across FUNC, DSP, UI, PERF, COMPAT, QUAL

**Stage 0:** Complete (Research & Planning)
- Synthesized 4 deep-research documents (synthesis + waveguide-stability + body-acoustics + drone-and-subharmonics) into canonical architecture
- ARCHITECTURE.md documented with 11 required sections (immutable contract for Stages 1-4)
- All 29 parameters mapped to DSP components (no orphans, no unmapped components)
- 9 distinct DSP components specified with JUCE class assignments
- Integration analysis: feature dependencies, parameter interactions, processing order, thread boundaries
- 5 high-risk components documented with fallback architectures
- 6 architecture decisions recorded with rationale and alternatives
- Complexity score: 5.0 (capped from raw 16.0) — Tier 6 (Deep)
- Implementation strategy: phased across all stages with 6 DSP sub-phases
- 6 open decisions resolved with recommendations and defer-to-v1.1 flags
- ROADMAP.md documents Stage 1 (Foundation) → Stage 2 (DSP, 6 phases) → Stage 3 (GUI, 3 phases) → Stage 4 (Polish)
- Estimated total effort: 10–15 days

## Next Steps

1. **Stage 2 / Phase 2.1b PLAN rev-4** — Run `/plugin-plan O-Contrabass 2-dsp`
   - CONTEXT.md rev-2 + RESEARCH §13 are locked. Plan-phase agenda: write PLAN rev-4 R8–R15 task breakdown verbatim against RESEARCH §13.6 sequencing table (R8: build O-Bowed render-harness mirroring O-Contrabass + capture `o-bowed-pre-extraction-canonical.wav` golden reference; R8a: separate commit for harness tooling; R9–R11: module skeleton + verbatim file copy + registry entry; R12–R13: both-plugins CMakeLists + include + delete inline copies + O-Contrabass setter calls; R14: build + auval + pluginval + bit-exact `cmp` + bow-on-only re-render; R15: atomic commit on Gate 2 PASS).
   - Five Open Items remain for plan-phase pinning (see RESEARCH §13.7): O-Bowed `JucePlugin_PluginCode`, O-Bowed processor class name, `ouaricon_add_module` console-app compatibility, WAV writer parameter pinning, golden-WAV git-commit decision.
2. **After plan:** EXECUTE Phase 2.1b — module extraction, both-plugins atomic switch, R15 atomic commit on Gate 2 PASS — `/plugin-execute O-Contrabass 2-dsp`.
3. **After execute:** VERIFY Gate 2 — `/plugin-verify O-Contrabass 2-dsp`. Gate 2 pass-bar: O-Bowed bit-exact regression (`cmp` byte-equality) + O-Contrabass harness bow-on-only (4/4 invariants byte-identical to `/tmp/e1-bowon-only.json` reference) + auval (both plugins) + pluginval-10 (both plugins) + clean build + registry update + R15 commit.
4. **Logic Pro AU smoke** — non-blocking, between R7 commit (now landed) and 2.1b execute kick-off. User auditions O-Contrabass at default E1 sustained tone + bow-position sweep.
5. **After Phase 2.1b verifies:** Phase 2.1c (cascaded allpass dispersion, R16–R19, **Gate 3**) starts fresh GSD cycle.
6. **After Phase 2.1 verifies (full):** start fresh GSD cycles for Phase 2.2 → 2.3 → 2.4 (saturator-tail re-evaluated here per §12 follow-up; §12.5 escalation triggers gate ARCH amendment proposal) → 2.5 → 2.6.
7. **Architecture amendments for ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator"** — both DEFERRED to end-of-Stage-2 verify per locked decisions (CONTEXT.md rev-2 + RESEARCH §12.6). F3 deviation tracked in PLAN rev-3 + SUMMARY.md + VERIFICATION.md + R7 commit-message body until then. §"In-loop saturator" amendment is conditional on §12.5 triggers from Phase 2.4 matrix sweep.
8. Pause point established after each phase (per CLAUDE.md handoff protocol).

## Context to Preserve

**Architecture highlights:**
- 4-string EADG digital waveguide (E1-G3) with `juce::dsp::DelayLine<float, Lagrange3rd>` (8192-sample buffer)
- Hyperbolic friction junction at 2x oversampling (`filterHalfBandPolyphaseIIR`)
- Cascaded allpass dispersion per string (M=4/3/2/1 for E/A/D/G)
- 8-mode parallel biquad body bank (Askenfelt-derived, Body Size scales freq, Q invariant)
- 3-band BPF bow noise (700/1500/3000 Hz) summed AFTER body resonator
- Drone features: Infinite Sustain (loop gain 0.997 → 0.99995), Sub-Harmonics (period-doubling friction bias), Slow-Bow LFO (Schelleng-aware diagonal modulation)
- Vibrato modulates delay-line length (physically correct; Lagrange3rd absorbs cleanly)
- Master saturator (polynomial) + zero-latency feedforward limiter (-1 dBFS)
- Ouaricon microtonal: priority Note Expression > MTS-ESP > Scala/TUN > MPE pitch-bend > 12-TET

**Module dependencies:**
- `modules/tuning/scala-tuning-engine` v2.1.0 (existing) — Scala/TUN + MTS-ESP
- `modules/tuning/note-expression` (existing) — VST3 Note Expression helper + JUCE patch
- `modules/dsp/bow-friction` (TO BE CREATED in Phase 2.1b) — extracted from O-Bowed `HyperbolicFriction.h`

**Reference plugins:**
- O-Bowed — friction junction, waveguide string, body resonator (extraction source)
- O-Lyrica — Note Expression integration pattern (BowedStringVoice template)

**Performance projection:** ~3.2% CPU on M1 (well under PERF-02's 5% target)

**Highest-risk component:** Friction Junction at E1 + max INFINITE_SUSTAIN + max SUB_HARMONICS — represents ~50% of project risk. Phase 2.1 must validate stability before any further features added.

**Key constraints (from juce8-critical-patterns + memory file):**
- `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` mandatory
- `BusesProperties` output-only in constructor
- `JUCE-NE-PATCH` must be applied to `~/JUCE/` for Note Expression
- `getLatencySamples()` is NOT virtual in JUCE 8 — use `setLatencySamples()` in `prepareToPlay`
- WebView2 needs both `NEEDS_WEBVIEW2 TRUE` AND `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
- Resource provider receives PATHS, not full URLs

**Open decisions resolved (see ROADMAP.md §"Open Decisions Resolved"):**
1. Friction tier: Hyperbolic only in v1.0 (defer elasto-plastic to v1.1)
2. Module extraction: DURING Stage 2 (mid-Phase 2.1)
3. Authentic Arco wolf toggle: defer to v1.1
4. Sub-harmonic max depth: 1 octave (f0/2)
5. Body Size mapping: full 1/4 → 4/4 span
6. Wood variants: single fixed wood for v1.0 (defer 2-variant to v1.1)

## Files Created

- `plugins/O-Contrabass/.planning/BRIEF.md` (Stage Ideation)
- `plugins/O-Contrabass/.planning/REQUIREMENTS.md` (Stage Ideation)
- `plugins/O-Contrabass/.planning/parameter-spec-draft.md` (Stage Ideation, 29 parameters)
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` (Stage 0 — DSP contract, 11 sections)
- `plugins/O-Contrabass/.planning/ROADMAP.md` (Stage 0 — implementation plan with phase breakdown)
- `plugins/O-Contrabass/.planning/stages/0-ideation/CONTEXT.md` (Stage 0 — discuss findings)
- `plugins/O-Contrabass/.planning/parameter-spec.md` (Stage 1 — promoted from draft 2026-04-26, identical content)
- `plugins/O-Contrabass/.planning/stages/1-foundation/CONTEXT.md` (Stage 1 — discuss synthesis)
- `plugins/O-Contrabass/.planning/stages/1-foundation/RESEARCH.md` (Stage 1 — pattern confirmation pass)
- `plugins/O-Contrabass/.planning/stages/1-foundation/PLAN.md` (Stage 1 — 9-task execution plan)
- `plugins/O-Contrabass/CMakeLists.txt` (Stage 1 execute — 79 lines)
- `plugins/O-Contrabass/Source/PluginProcessor.h` (Stage 1 execute — 56 lines)
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` (Stage 1 execute — 156 lines, 29 APVTS params + processor shell)
- `plugins/O-Contrabass/Source/PluginEditor.h` (Stage 1 execute — 27 lines, stub)
- `plugins/O-Contrabass/Source/PluginEditor.cpp` (Stage 1 execute — 24 lines, stub)
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` (Stage 2 / Phase 2.1 — discuss synthesis)
- `plugins/O-Contrabass/.planning/STATUS.md` (this file)

## Lifecycle Timeline

- **2026-04-25 (Stage Ideation):** Creative brief and requirements documented (FUNC, DSP, UI, PERF, COMPAT, QUAL across 24 reqs).
- **2026-04-25 (Stage 0):** Research & Planning complete — ARCHITECTURE.md and ROADMAP.md documented (Complexity 5.0, Tier 6, phased strategy with 6 DSP sub-phases).
- **2026-04-26 (Stage 1, discuss):** parameter-spec.md promoted from draft (same checksum). Stage 1 CONTEXT.md written — zero open questions surfaced; all build/APVTS/latency decisions inherited from locked Stage 0 contracts.
- **2026-04-26 (Stage 1, research):** RESEARCH.md written — pattern-confirmation pass against O-Bells (CMake), O-Bowed (APVTS / prepareToPlay), O-Lyrica (note-expression integration). PLUGIN_CODE `OCbs` reserved (no collision). Two refinements: (1) oversampler is voice-level, defer allocation to Stage 2 entirely; (2) `note-expression` uses `ouaricon_add_module` (per-format routing required), `scala-tuning-engine` uses explicit file refs (sibling-plugin convention).
- **2026-04-26 (Stage 1, plan):** PLAN.md written — 9-task breakdown: CMake (Pattern A for note-expression + Pattern B for scala-tuning-engine, both WebView2 flags, 13 JUCE modules, header-after-link order); PluginProcessor.h (no `getLatencySamples` override); 29-parameter `createParameterLayout()` table with explicit skews per RESEARCH.md §2.2; processor.cpp shell (output-only `BusesProperties`, `setLatencySamples(0)`, `ScopedNoDenormals`, APVTS XML state); minimal editor stub; macOS build + install + `auval` + pluginval strictness 10 + 5-DAW smoke (Logic, Ableton, Reaper, Dorico, Cubase). Estimated effort 2–4 h.
- **2026-04-26 (Stage 1, execute):** 5 files written verbatim from PLAN.md — `CMakeLists.txt` (79 LOC), `PluginProcessor.{h,cpp}` (56 + 156 LOC), `PluginEditor.{h,cpp}` (27 + 24 LOC). All 29 APVTS parameters implemented with UPPER_SNAKE_CASE IDs matching parameter-spec.md (sha256:c47fe736…) verbatim. No `getLatencySamples` override; `setLatencySamples(0)` in `prepareToPlay`. Both WebView2 flags set in CMake. Pattern A (`ouaricon_add_module`) for note-expression, Pattern B (explicit file refs) for scala-tuning-engine. PLUGIN_CODE `OCbs` reserved. Code-level success criteria (no-override / WebView2-flags / param-IDs) PASS by inspection. Build / install / auval / pluginval / 5-DAW verification deferred — execute subagent had no shell access. Hand off to `/plugin-verify O-Contrabass 1-foundation` for the shell-driven exit gate.
- **2026-04-26 (Stage 1, verify):** Stage 1 ✅ VERIFIED. Re-ran auval (`aumu OCbs OuDv` → SUCCEEDED) and pluginval strictness 10 (→ SUCCESS) against installed binaries. Code-level constraints re-checked: 29 unique ParameterIDs (verified by grep+sort uniq); single comment-only mention of `getLatencySamples` in PluginProcessor.h (line 49) — no override; both WebView2 flags present; output-only BusesProperties confirmed by pluginval bus listing (0 input / 2 output ch). COMPAT-01 marked PARTIAL — macOS VST3+AU verified, Windows deferred to Stage 4 per ROADMAP cross-platform plan. 5-DAW manual smoke marked optional/non-blocking (silent placeholder + stub editor have no behavior beyond what auval+pluginval cover; revisit at Stage 3 / Stage 4). VERIFICATION.md written.
- **2026-04-26 (Stage 2 / Phase 2.1, discuss):** CONTEXT.md written. Cycle scope = Phase 2.1 only (gate-first). 7 approach decisions: (1) cycle scope = Phase 2.1, (2) module extraction confirmed mid-Phase 2.1 per ROADMAP, (3) module home = `modules/synthesis/bow-friction/` (deviation from ROADMAP `modules/dsp/` — registry has no `dsp` category; module-name `ouaricon_bow_friction`), (4) risk fallback = clamp `INFINITE_SUSTAIN` ceiling if E1+drone-mode fails, (5) MIDI strategy = real note-on → E1 voice (MPESynthesiser), (6) stability harness = automated render-to-WAV + invariant checks (reusable for Phase 2.4), (7) listening DAW = Logic Pro (AU). 5 open questions handed to research: harness selection, O-Bowed regression bar, MPESynthesiser timing, oversampler placement, `INFINITE_SUSTAIN` curve.
- **2026-04-26 (Stage 2 / Phase 2.1, plan rev-2 / recovery):** PLAN.md rewritten as rev-2 after harness FAIL on rev-1 single-rail topology. Recovery sequence locked from RESEARCH.md §10 (R1–R5): R1 pre-flight diagnostic (single-rail + 2× injection smoke test, 30 min), R2 split-rail rewrite of `WaveguideString.{h,cpp}` mirroring O-Bowed canonical Smith two-port scattering with O-Contrabass loop ordering (dispersion → bridge LP → in-loop saturator → DC blocker → write) on the bridge rail only and `-1` boundary on the nut rail, R3 drop `betaScale` fudge at `BowedContrabassVoice.cpp:223-229`, R4 rebuild + auval + pluginval level-10 + render-harness rerun (Gate 1), R5 atomic commit. Phase 2.1b (R6–R13: module extraction + O-Bowed bit-compare, Gate 2) and Phase 2.1c (R14–R17: dispersion + final verification, Gate 3) carry forward verbatim from rev-1 with the explicit clarification that dispersion lives on the bridge rail only. Net code delta from rev-1 working tree: ~+30 LOC `WaveguideString.{h,cpp}`, ~−7 LOC `BowedContrabassVoice.cpp`, everything else (`BowModel`, `HyperbolicFriction`, `OContrabassMPESynthesiser`, oversampler, `PluginProcessor`, render-harness, CMake) untouched. No contract violation (ARCHITECTURE.md / ROADMAP.md / parameter-spec.md / BRIEF.md silent on rail count; CONTEXT.md "single-rail" was a discuss-phase advisory guess, overridden by re-research).
- **2026-04-26 (Stage 2 / Phase 2.1, execute rev-2 R1):** R1 pre-flight diagnostic ran (single-line edit at `WaveguideString.cpp:144`: `incoming + newVelocity` → `incoming + 2.0f * newVelocity`). Harness FAILED: peak −32.6 dBFS (vs rev-1 baseline −39 dBFS, +6 dB transient bump consistent with naive linear superposition during attack), `rmsMid_s5_s6` = 2.23e-8, `rmsFinal_lastSecond` = 0.0, `pass_rms` = false. Per PLAN rev-2 R1 fail-action and Risks #1, R2–R5 NOT executed. Diagnostic edit reverted; working tree byte-identical to start-of-execute. PLAN rev-2 superseded. SUMMARY.md / CHECKPOINT-2.1a.md updated with six-hypothesis re-research priority order. Topology-only hypothesis (rev-1 §10) FALSIFIED — single-rail with 2× injection compensation is NOT mathematically equivalent to split-rail at the cold-start bootstrapping step.
- **2026-04-26 (Stage 2 / Phase 2.1, research rev-2 / §11):** RESEARCH.md §11 written (365 LOC appended; total file 1289 LOC). Three compounding bugs identified: **B1** single-rail topology cannot bootstrap Helmholtz (sample-by-sample equilibrium trace shows x → v_bow constant in sticking, pushed value → 0 after DCB transient → silent stable equilibrium); **B2** bridge LP recurrence at `WaveguideString.cpp:162` erroneously multiplies `g` into the feedback term (`y = g·(...·x + g·p·y_prev)`), inflating DC gain from intended `g` to `g·(1−p)/(1−g·p) ≈ 1` at high sustain — NEW finding, not in §10 hypothesis list, discovered via line-by-line comparison against O-Bowed `WaveguideString.cpp:94-95`; **B3** in-loop DCB (added to ARCHITECTURE.md as workaround for B2) actively suppresses cold-start sticking-regime injection. Six §10/SUMMARY hypotheses dispositioned: H1 (sign convention) disproven (round-trip closure correct), H3 (saturator) disproven (linear at −32.6 dBFS), H4 (BowModel sr) disproven (`prepare(spec_at_2x.sampleRate)` correct), H6 (first-tick envelope) dissolved (envelope behaves correctly; transient peak is sticking-injection + DCB transient, not envelope timing), H2 (DCB) reframed → B3, H5 (Schelleng) reframed → B1 + latent for Phase 2.4. PLAN rev-3 deliverable: F1 split-rail (~+45 LOC, formerly §10 R1), F2 bridge LP recurrence fix (1-line: drop `g` from feedback), F3 remove in-loop DCB (~−6 LOC, ARCH.md §"DC Blocker" deviation justified per §11.6 — once F2 fixes LP DC gain to `g`, in-loop DCB is redundant and harmful), F4 drop `betaScale` fudge in voice (~−2 LOC, formerly §10 R5). Verification: V1 render-harness rerun (Gate 1: pass_rms must transition from false → true; expected `rmsMid_s5_s6` ≈ 0.05–0.20), V3 auval + pluginval-10 (Gate 2). V2 instrumentation hook (per-sample `v_delta / frictionVelocity` CSV trace) optional, only if V1 fails. Hand off to `/plugin-plan O-Contrabass 2-dsp` for PLAN rev-3.
- **2026-04-26 (Stage 2 / Phase 2.1a-recovery, verify):** ⚠️ PARTIAL VERIFICATION written to `stages/2-dsp/VERIFICATION.md`. Independently re-ran auval (`aumu OCbs OuDv` → SUCCEEDED), pluginval strictness 10 (→ SUCCESS), and render-harness under both scenarios — bow-on-only 65s (4/4 invariants TRUE, ratio 1.04, byte-identical to /tmp/e1-bowon-only.json) and standard 60s+5s (3/4 invariants TRUE, pass_rms FALSE, byte-identical to /tmp/e1-max-sustain-r3.json). Code-level F1/F2/F3/F4 confirmed in source by grep + inspection: split-rail (`bridgeDelay`+`neckDelay`), F2 LP recurrence at WaveguideString.cpp:144-148 matching O-Bowed canonical, zero DCB references, zero `betaScale`/`setStringImpedance` references. Goal-backward analysis: B1/B2/B3 pathologies retired; Helmholtz bootstrapping + 65s sustained drone at INFINITE_SUSTAIN=1.0 achieved. Standard `pass_rms` FALSE characterised as in-loop saturator's low-amplitude cubic loss during 4-5s post-bow-off tail (not a B1/B2/B3 regression, not a transcription error). Requirements: 1 component-complete (DSP-02 friction junction), 9 partial (E1-only or component-only progress against multi-phase requirements), 9 deferred to later Phase 2.x cycles, 0 failed. **R7 atomic commit STILL DEFERRED** — Phase 2.1a source files (DSP/, BowedContrabassVoice, OContrabassMPESynthesiser, PluginProcessor, render-harness, CMakeLists, plus all uncommitted Stage 1 source) live on disk only; git log shows last commit is `557c009 docs(O-Contrabass): Stage 0 - research & planning complete`. Verify recommends Option A (commit verbatim) per SUMMARY's analysis. Logic AU smoke deferred to user. Hand off to user for Option A/B/C decision; once decided, R7 commit lands the working tree, then Phase 2.1b — `/plugin-discuss O-Contrabass 2-dsp` (or continue Phase 2.1 cycle with fresh discuss for 2.1b).
- **2026-04-26 (Stage 2 / Phase 2.1, plan rev-3):** PLAN.md rewritten as rev-3 from RESEARCH §11 root-cause analysis. Tasks restructured into a single coupled-fix sequence — **no pre-flight sub-gate** (rev-2 R1 was a flawed test per §11.7; the analytical equilibrium trace in §11.1 supplies what the pre-flight was meant to prove). R1 implements F1 split-rail (replace single `delayLine` with `bridgeDelay` + `neckDelay`, mirror O-Bowed `processSample` structure with O-Contrabass's algebraic saturator and bridge-rail-only loop chain, full sign-convention contract spelled out); R2 implements F2 bridge LP fix as a tracked line-level diff (`y = g·(1−p)·x + p·y_prev + leak`, drop `g` from feedback so DC gain = g exactly); R3 implements F3 in-loop DCB removal (drop `dcX1`/`dcY1`/`kDCBlockerR` members, drop DCB lines from `processSample` and `reset`; ARCHITECTURE.md §"DC Blocker" deviation explicitly flagged in PLAN preamble + commit-message body + "Why F3 deviates" section); R4 implements F4 betaScale fudge removal (−7 LOC at `BowedContrabassVoice.cpp:223-229`); R5 runs Gate 1 (auval + pluginval-10 + render-harness, expected `pass_rms` TRUE with `rmsMid_s5_s6` ≈ 0.05–0.20); R6 is the OPTIONAL V2 instrumentation hook (per-sample CSV trace of `v_delta`, `frictionVelocity`, `|v_delta|/frictionVelocity`, `newVelocity`, `toBridge`, `toNeck`, `bridgeFiltered`), only invoked on R5 V1 fail, NOT shipped/committed in normal flow; R7 atomic commit. Phase 2.1b (R8–R15: module extraction, **Gate 2**) and Phase 2.1c (R16–R19: dispersion + final verification, **Gate 3**) carry forward verbatim from rev-1/rev-2 task bodies, with the explicit clarification that the bridge-rail loop chain under rev-3 omits the in-loop DCB (per F3). Net code delta: ~+38 LOC `WaveguideString.{h,cpp}`, ~−2 LOC `BowedContrabassVoice.cpp`. Architecture amendment recommendation (post-verify, out of scope for execute): update ARCHITECTURE.md §"DC Blocker" to reflect the corrected LP form and the output-path DCB option. Hand off to `/plugin-execute O-Contrabass 2-dsp`.
- **2026-04-26 (Stage 2 / Phase 2.1a-close + 2.1b-open, discuss rev-2):** CONTEXT.md rev-2 written. Cycle scope = Phase 2.1a closure (R7 atomic commit) + Phase 2.1b opening (module extraction, Gate 2). 9 approach decisions locked: (1) **Option A** — accept Gate 1 PASS on bow-on validation, commit rev-3 verbatim (F3 ARCHITECTURE deviation tracked in commit body); (2) saturator-tail dissipation parked as Phase 2.4 follow-up + RESEARCH §12 footnote (analytical derivation: x²/2 cubic-loss × 2 rails × 41.2 RTs/s ≈ 10%/s free-decay); (3) ARCHITECTURE.md §"DC Blocker" amendment deferred to end-of-Stage-2 verify; (4) Phase 2.1b cycle scope = 2.1b only (2.1c gets its own cycle); (5) module home/name = `modules/synthesis/bow-friction/` + `ouaricon_bow_friction` (rev-1 carry-forward); (6) **module surface corrected** = `HyperbolicFriction` + `BowModel` only (rev-1's `HyperbolicBowTable`/`BowState`/`SchellengGuard` are not real classes in O-Bowed); (7) O-Bowed regression bar = bit-exact WAV diff on canonical preset (default A4 sustained, ~5s, no detune/vibrato/sub-harmonics, INFINITE_SUSTAIN OFF); (8) Logic Pro AU smoke timing = post-R7-commit, pre-2.1b-execute (non-blocking for R7); (9) both-plugins switch = atomic R15 commit (no flag-day window). 5 open questions handed to research-phase: RESEARCH §12 timing, CMakeLists pattern A/B selection, header layout, include-switch mechanics (delete inline-copy headers vs shim), bass-default propagation API. Phase 2.1c (dispersion) and Phases 2.2–2.6 stay out of scope. Hand off to `/plugin-research O-Contrabass 2-dsp` for rev-2 research-phase (R7 commit + §12/§13 appends + Open Question resolution + golden-reference render of O-Bowed pre-extraction).
- **2026-04-26 (Stage 2 / Phase 2.1, research rev-2 / §12 + §13 + R7):** RESEARCH.md §12 (saturator-tail Phase 2.4 follow-up, ~110 LOC) and §13 (Phase 2.1b module-extraction research, ~210 LOC) appended. **§12** documents the analytical envelope estimate ≈10 %/s free-decay = x²/2 cubic-loss × 2 rails × 41.2 RTs/s, frames Phase 2.4's 108-combo matrix re-evaluation criteria, and conditionally tracks ARCHITECTURE.md §"In-loop saturator" amendment alongside §"DC Blocker" — both deferred to end-of-Stage-2 verify. **§13** resolves CONTEXT.md rev-2 Open Questions #2–#5: Q2 → Pattern A (`ouaricon_add_module`); Q3 → two direct headers, no umbrella, mirroring scala-tuning-engine; Q4 → DELETE inline-copy DSP files in both plugins, update `BowedStringVoice.h:23-24` includes, no shim files; Q5 → setter API (`setStaticFrictionCoefficient` + `setDynamicFrictionCoefficient` added to module's `HyperbolicFriction`; module keeps O-Bowed init defaults; O-Contrabass voice calls setters in `prepareToPlay`). §13 also locks the canonical preset (A4 vel 0.7, 5 s, factory defaults, no release tail, 32-bit float WAV) and specs the missing O-Bowed render-harness as PLAN rev-4 R8 work (mirror O-Contrabass harness exactly with target/note/sustain substitutions). Q1 (§12 timing) resolved by writing §12 in this same pass. Five Open Items handed to plan-phase rev-4 (§13.7): O-Bowed PluginCode + processor class confirm, `ouaricon_add_module` console-app compatibility, WAV writer parameter pinning, golden-WAV git-commit decision. **R7 atomic commit landed** — Phase 2.1a-recovery source (split-rail `WaveguideString.{h,cpp}` + F2 bridge LP + F3 DCB removal + F4 betaScale removal) + Stage 1 carry-forward (`CMakeLists.txt`, `PluginProcessor.{h,cpp}`, `PluginEditor.{h,cpp}`) + parameter-spec promotion + planning artefacts (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION rev-3 + CHECKPOINT-2.1a + Stage 1 set + 0-ideation gate-report) + 4 Stage 0 deep-research docs (`research/O-Contrabass-*.md`) + render-harness binary + REQUIREMENTS.md update committed in single commit per CONTEXT.md rev-2 R7 file list. Commit body explicitly notes F1+F2+F3+F4 coupled fix (RESEARCH §11 root-cause), F3 ARCHITECTURE.md §"DC Blocker" deviation (justified per §11.6), Gate 1 PASS on bow-on validation, saturator-tail Phase 2.4 follow-up parked. Hand off to `/plugin-plan O-Contrabass 2-dsp` for PLAN rev-4 (R8–R15 module-extraction tasks per RESEARCH §13.6).
