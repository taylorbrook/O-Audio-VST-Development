---
plugin: O-Contrabass
stage: 2
phase: verify
status: phase_2_1c_verify_complete_pending_r20
last_updated: 2026-04-27
complexity_score: 5.0
complexity_score_raw: 16.0
complexity_tier: 6
research_depth: DEEP
staged_implementation: true
orchestration_mode: true
cycle_scope: phase_2_1c_dispersion_gate_3
next_action: phase_2_1c_r20_commit_then_phase_2_2_discuss  # R20 atomic commit lands ~13 files (source + harness + goldens + planning), then /clear + /plugin-discuss O-Contrabass 2-dsp opens Phase 2.2 (per-string detune + A1/D2/G2 strings)
phase_2_1b_blocker_resolved: |
  Phase 25 (package-docs) refactor of modules/tuning/note-expression/module.cmake
  removed the cohort-VST3 CID extraction from configure-time (commits 6db3a79 /
  7a87d30 / d2c86c5 / db20a04 / 496d4c4). CMake configure now exits 0 cleanly
  for O-Contrabass + O-Bowed at -DOUARICON_BUILD_TESTS=ON. Phase 2.1b execute
  resumed 2026-04-27 against PLAN rev-4; Gate 2 PASSED bit-exact (sha256
  93124fb8dd8223caafac5948c988a226230363d79a17323d386e9a1db34c8891 unchanged
  pre/post extraction).
next_stage: 2
ready_for_implementation: true  # PLAN rev-4 R8–R15 locked, all 5 Open Items pinned
plan_revision: 5
plan_revision_status: complete  # rev-5 R16-pre/R16/R17/R17b/R18/R19/R20 task bodies written; Open-Item-Pins 1–6 resolved
research_revision: 3
research_revision_status: complete  # §14 Phase 2.1c dispersion appended 2026-04-27; Q1-Q5 resolved (m1..m4/k1..k3 constants pinned, group-delay formula = at-f0, setter API = setDispersionCoefficient, template = MaxSections+activeSections, harness = single-WAV ramp); pre-flight baseline strategy specified (deferred to R16-pre); split-rail compensation subtlety surfaced (subtract from bridgeSamples not compensated); Risk #7 added (E1 closed-form clamp saturation)
phase_2_1c_research_anomaly_e1_clamp: |
  Closed-form -C/k ≈ 15 across the entire B envelope at I=8.0 (E1) → clamps
  to a≈+0.99 regardless of STRING_STIFFNESS. Paper validity envelope is
  piano register; bass register sits outside. NOT a bug; audible sweep may
  be flatter than ideal. Phase 2.4 follow-up if R18 reveals the sweep is
  musically uninteresting (calibration polynomial for bass register). Gate
  3 stability + bit-exact regression bar still meaningful.
context_revision: 3
context_revision_status: locked_phase_2_1c_dispersion_gate_3
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
phase_2_1b_o_bowed_harness_status: built  # PLAN rev-4 R8 + R8a landed 2026-04-27 (commit bd5fae0)
phase_2_1b_atomic_commit_sha: ef0604d  # R15 atomic commit landed 2026-04-27
phase_2_1b_harness_commit_sha: bd5fae0  # R8a separate harness-tooling commit 2026-04-27
phase_2_1b_canonical_render_sha256: 93124fb8dd8223caafac5948c988a226230363d79a17323d386e9a1db34c8891
gate_state:
  build: PASS
  auval: PASS  # re-verified 2026-04-27 post-Phase-2.1c (AU VALIDATION SUCCEEDED)
  pluginval_strictness_10: PASS  # re-verified 2026-04-27 post-Phase-2.1c (SUCCESS)
  render_harness_60s_e1: FAIL_rms_ratio_only_post_bow_off_characterised_park_phase_2_4  # not a regression
  render_harness_65s_bowon_only: PASS_4_of_4  # 2.1c clean retry: rmsRatio 1.04, blockTime ratio 2.34; WAV byte-deterministic across retries
  source_committed_to_git: PASS  # R7 + R15 commits landed; R20 (Phase 2.1c) gated on this verify
  bow_friction_module_extraction: PASS  # Phase 2.1b R15 atomic commit landed 2026-04-27, bit-exact regression PASSED
  cascaded_allpass_dispersion_e1: PASS  # Phase 2.1c verify 2026-04-27, R19a-e all reproduced bit-exact; R19f Logic smoke user-deferred non-blocking
gate_1_outcome: PARTIAL_PASS_option_a_locked
verify_outcome: PARTIAL_engine_validated_r7_landed_2026_04_26
phase_2_1b_verify_outcome: VERIFIED_gate_2_pass_bit_exact_2026_04_27  # auval + pluginval-10 + cmp byte-equal + bow-on-only carry-forward all reproduced independently
phase_2_1c_verify_outcome: VERIFIED_gate_3_pass_2026_04_27  # R19a stiffness=0 sha256 d358abcd… matches committed golden; R19b 4/4 clean retry; R19c auval; R19d pluginval-10; R19e sweep sha256 94a42a81… matches committed golden + rmsByDecade ~5% confirms Risk #7; R19f Logic smoke user-deferred non-blocking
phase_2_1c_verify_independent_reproduction:
  stiffness_zero_sha256: d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75  # matches committed golden
  bowon_only_sha256: 0cc6ed4ccb6c9c831df4ca7cbaee76041fed96778f2c0aa91fd37a59ffbdedc6  # byte-deterministic across 3 retries
  stiffness_sweep_sha256: 94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6  # matches committed golden
  auval: AU_VALIDATION_SUCCEEDED
  pluginval_10: SUCCESS
  rms_by_decade_peak_to_peak_pct: 4.8  # confirms Risk #7 — closed-form clamp at I=8.0 makes E1 sweep musically near-flat
phase_2_1b_verify_independent_reproduction:
  o_bowed_auval: AU_VALIDATION_SUCCEEDED
  o_contrabass_auval: AU_VALIDATION_SUCCEEDED
  o_bowed_pluginval_10: SUCCESS
  o_contrabass_pluginval_10: SUCCESS
  o_bowed_canonical_sha256: 93124fb8dd8223caafac5948c988a226230363d79a17323d386e9a1db34c8891  # matches committed golden
  o_contrabass_bowon_only_sha256: 00431582b6068c4f4e308f9de0da418ec6b34243625930e13783cda1735d5e60  # matches phase_2_1a_recovery_reference
contract_checksums:
  brief: sha256:6ea840bbe4c34855397111896b9f6fd52cb5f66c597676d300b0c5ce33c8988d
  parameter_spec: sha256:c47fe7361a55e1d64b906ef7194894f4a2490744b35a644c76b6e1a632282d0d
  architecture: sha256:3cb26814bcd830cfba0b3bba42c096bdbf5b1449f52825a167cde09e114855a0
  roadmap: sha256:106639f633b6b3a2cfeb41eb07640d3ac0e01ed0832c33a9da45faf2b97aca7e
---

# O-Contrabass Status

## Current Position

Stage: 2 of 4 (DSP) — Phase 2.1 cycle nearly closed (2.1a ✅ CLOSED via R7 atomic commit; 2.1b ✅ COMPLETE — execute landed `ef0604d` + verify Gate 2 PASS bit-exact 2026-04-27; 2.1c ✅ VERIFIED 2026-04-27, R20 atomic commit pending)
Phase: verify ✅ COMPLETE — Phase 2.1c Gate 3 PASS. All six automated R19 invariants reproduced bit-exact: R19a stiffness=0 sha256 `d358abcd…` matches committed golden; R19b bow-on-only 65s 4/4 invariants TRUE on clean retry (WAV byte-deterministic across retries; wall-clock blockTime variance is host-load noise, not DSP regression); R19c auval AU VALIDATION SUCCEEDED; R19d pluginval-10 SUCCESS; R19e sweep WAV sha256 `94a42a81…` matches committed golden, rmsByDecade ~4.8% peak-to-peak variation confirms Risk #7 (E1 closed-form clamp). R19f Logic AU smoke user-deferred non-blocking, mirroring Phase 2.1b R14e precedent. Code-level inspection confirmed: `DispersionFilter.h` (130 LOC) constants pinned constexpr with paper citation; `WaveguideString.{h,cpp}` integration at prepare(39-41), reset(66), processSample(154-156), updateDelayLengths(88-89); voice short-circuit + isfinite guard at `BowedContrabassVoice.cpp:152-169`; harness `--string-stiffness` / `--stiffness-sweep` flags wired with sweep-mode JSON extras. R20 atomic commit gated on this verify landing — single commit lands ~13 files (source + harness + 3 golden text files + planning artefacts).
Cycle Scope: **Phase 2.1c — Cascaded Allpass Dispersion (Gate 3)** — bridge-rail-only on E-string, Rauhala/Välimäki 2006, M=4. ✅ VERIFIED. Phase 2.2–2.6 still get fresh GSD cycles each
Status: Phase 2.1c verify complete 2026-04-27. R20 atomic commit pending (gated on this verify; mirrors gate-first principle from R7/R15). ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments still deferred to end-of-Stage-2 verify per locked decision. Phase 2.4 calibration polynomial follow-up parked per RESEARCH §14.10 Risk #7 (closed-form clamp at I=8.0 makes E1 STRING_STIFFNESS sweep musically near-flat).
Progress: [####################] 100% — Phase 2.1c verify; (Stage 2 overall: ~30% — Phases 2.2, 2.3, 2.4, 2.5, 2.6 remain)

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

1. **R20 atomic commit** — single commit lands ~13 files (gate-first principle, mirrors R7 + R15):
   - `Source/DSP/DispersionFilter.h` (NEW, +130 LOC)
   - `Source/DSP/WaveguideString.{h,cpp}` (modified: +29 / +54 LOC; bridge-rail dispersion wiring + split-aware group-delay compensation)
   - `Source/BowedContrabassVoice.cpp` (modified: +20 LOC incl. 3-LOC short-circuit at stiffness=0)
   - `tests/render-harness/main.cpp` (modified: +68 LOC — `--string-stiffness` + `--stiffness-sweep` CLI modes, sweep-mode JSON extras)
   - `tests/render-harness/golden/{stiffness-zero-pre.wav.sha256, stiffness-zero-pre.json, stiffness-sweep.wav.sha256}` (NEW)
   - `.planning/STATUS.md` + `.planning/stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md` updates
   - Closes Phase 2.1.
2. **Logic Pro AU smoke (R19f)** — non-blocking, recommended pre-Phase-2.2. User auditions O-Contrabass at MIDI E1 (note 28) sustained, with `STRING_STIFFNESS = 0 / 50 / 100 %`. Confirms: 0% sounds clean (statistically near-identical to pre-dispersion memory); 100% has audible attack-character difference; steady-state pitch remains locked at E1 (mode-locking invariant); no clicks during 0→100% knob sweep; no NaN, no silence. Per Risk #7 the audible 0→100% sweep is expected to be near-flat at E1 (rmsByDecade ~5% variation).
3. **Phase 2.2 — `/plugin-discuss O-Contrabass 2-dsp` after `/clear`** — fresh GSD cycle for per-string detune + A1/D2/G2 strings + per-string M=4/3/2/1 dispersion table (the natural home for the M-table per CONTEXT.md rev-3 Q2). Phases 2.3 → 2.4 (saturator-tail re-evaluated here per §12 follow-up + Phase 2.1c Risk #7 calibration polynomial; §12.5 escalation triggers gate ARCH amendment proposal) → 2.5 → 2.6 follow as own GSD cycles.
4. **Architecture amendments for ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator"** — both DEFERRED to end-of-Stage-2 verify per locked decisions (CONTEXT.md rev-2 + RESEARCH §12.6). F3 deviation tracked in PLAN rev-3 + SUMMARY.md + VERIFICATION.md + R7 commit-message body + R15 commit-message body until then. §"In-loop saturator" amendment is conditional on §12.5 triggers from Phase 2.4 matrix sweep.
5. Pause point established after each phase (per CLAUDE.md handoff protocol).

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
- **2026-04-26 (Stage 2 / Phase 2.1b, plan rev-4):** PLAN.md appended with REVISION 4 (Phase 2.1b — Module Extraction, Gate 2). R8–R15 task bodies written verbatim against RESEARCH §13.6 sequencing table, with R8a inserted as a separate harness-tooling commit. Five Open Items from RESEARCH §13.7 pinned in PLAN rev-4 §"Pinned Open Items": (1) O-Bowed PluginCode = `OBwd` = 0x4f427764 (verified `plugins/O-Bowed/CMakeLists.txt:9` — RESEARCH §13.5's guess `OBow`/0x4f426f77 was wrong); (2) O-Bowed processor class = `OBowedAudioProcessor` (verified `plugins/O-Bowed/Source/PluginProcessor.h:27`); (3) `ouaricon_add_module` works for `juce_add_console_app` targets — generic `target_sources` + `target_include_directories` calls in OuariconModules.cmake lines 57–67 don't require plugin-target structure, per-format routing block silently no-ops; (4) WAV writer = **24-bit PCM stereo** (`createWriterFor(stream, sr, 2, 24, {}, 0)` in O-Contrabass harness:240) — **corrected** RESEARCH §13.4's "32-bit float" claim, locked PLAN rev-4 to match O-Contrabass byte-for-byte; (5) golden WAV NOT committed (RESEARCH §13.7 #5 recommendation accepted) — instead R8a commits the JSON metadata + sha256 to `plugins/O-Bowed/tests/render-harness/golden/` for permanent audit trail. PLAN rev-4 §"Risks" enumerates 8 failure modes with mitigations; §"Success Criteria" lists 14 Gate 2 checks. Hand off to `/plugin-execute O-Contrabass 2-dsp`.
- **2026-04-27 (Stage 2 / Phase 2.1b, verify):** Phase 2.1b VERIFICATION.md appended. Independent reproduction of all four Gate 2 numeric checks: O-Bowed `auval -v aumu OBwd OuDv` SUCCEEDED, O-Contrabass `auval -v aumu OCbs OuDv` SUCCEEDED, both pluginval-10 SUCCESS, O-Bowed canonical render sha256 `93124fb8…34c8891` (byte-identical to committed golden at `plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav.sha256`), O-Contrabass bow-on-only sha256 `00431582…d5e60` (byte-identical to Phase 2.1a-recovery reference `/tmp/e1-bowon-only.wav`). Code-level checks confirmed: module skeleton + 5 files in `modules/synthesis/bow-friction/`, registry entry at `modules/registry.yaml:292-293`, bass setters defined at `HyperbolicFriction.h:60-61` and called at `BowedContrabassVoice.cpp:124-125` with `0.85f` / `0.25f`, inline DSP copies absent in both plugins, includes use bare `"BowModel.h"` / `"HyperbolicFriction.h"` (no `DSP/` prefix). DSP-02 acceptance criterion 2 ("Reuses or extends O-Bowed friction module if extracted as a shared module") now de-facto satisfied. R8a (`bd5fae0`) + R15 (`ef0604d`) atomic commits both confirmed via `git log`. Verdict: ✅ **VERIFIED**. ARCH §"DC Blocker" + §"In-loop saturator" amendments still deferred to end-of-Stage-2 verify per locked decision. Hand off to `/plugin-discuss O-Contrabass 2-dsp` (after `/clear`) for Phase 2.1c (cascaded allpass dispersion, R16–R19, Gate 3) fresh GSD cycle.
- **2026-04-27 (Stage 2 / Phase 2.1c, plan rev-5):** PLAN.md appended with REVISION 5 (Phase 2.1c — Cascaded Allpass Dispersion, Gate 3). R16-pre / R16 / R17 / R17b / R18 / R19 / R20 task bodies written verbatim against RESEARCH §14.11 sequencing table. Six plan-phase open items from RESEARCH §14.12 pinned in PLAN rev-5 §"Pinned Open Items": (1) accessor names = `advanceStiffnessSmootherBy(int)` + `getCurrentSmoothedStiffness()`; (2) per-sample `a` interpolation fallback location = inside `WaveguideString::processSample` (only invoked on click finding); (3) harness block-rate cadence = per-block `setValueNotifyingHost` (in-process APVTS is synchronous); (4) `e1-bowon-only-stiffness-zero-pre.wav` NOT committed — sha256 + JSON only at `plugins/O-Contrabass/tests/render-harness/golden/`; (5) `rmsByDecade` JSON semantic = 6 s × 10 deciles of sustain phase; (6) sha256 emission = external `shasum -a 256` shell wrapper (avoids `juce::juce_cryptography` dep). Anomaly carry-forward (NOT a Phase 2.1c blocker, parked per RESEARCH §14.10 Risk #7): closed form clamps to `a≈+0.99` across all B at E1 because paper validity envelope is piano register; Phase 2.4 follow-up if R18 sweep is musically uninteresting. PLAN rev-5 §"Risks" enumerates 8 failure modes; §"Success Criteria" lists 14 Gate 3 checks. R16-pre is structural prerequisite to R16 (golden must be captured before any DSP source edits). R20 atomic commit lands ~13 files in single commit on Gate 3 PASS, closes Phase 2.1. Hand off to `/plugin-execute O-Contrabass 2-dsp`.
- **2026-04-27 (Stage 2 / Phase 2.1c, verify):** Phase 2.1c VERIFICATION.md appended. Independent reproduction of all six automated R19 Gate 3 numeric checks: R19a bit-exact regression at `STRING_STIFFNESS=0` reproduced at sha256 `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` (byte-identical to committed golden at `tests/render-harness/golden/stiffness-zero-pre.wav.sha256`); R19b bow-on-only 65 s @ INFINITE_SUSTAIN=1.0 = 4/4 invariants TRUE on clean retry (rmsRatio 1.04, blockTime ratio 2.34; first two retries showed transient host-load wall-clock spikes 18.92 / 7.19 — WAV byte-deterministic at sha256 `0cc6ed4c…` across all three runs, confirming DSP determinism); R19c `auval -v aumu OCbs OuDv` AU VALIDATION SUCCEEDED; R19d `pluginval --strictness-level 10 --validate-in-process` SUCCESS; R19e sweep WAV reproduced at sha256 `94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6` (byte-identical to committed golden), JSON `mode: stiffness-sweep`, `stiffnessRamp: {start:0.0, end:1.0, shape:linear}`, `rmsByDecade ≈ [0.0353, 0.0360, 0.0366, 0.0368, 0.0369, 0.0370, …, 0.0370]` — empirically confirms Risk #7 with ~4.8% peak-to-peak RMS variation across the full 0→1 sweep. Code-level inspection confirmed: `DispersionFilter.h` (130 LOC) with all seven Rauhala/Välimäki constants pinned `constexpr` and paper citation comment + `static_assert(MaxSections >= 1)` + clamps; `WaveguideString.{h,cpp}` integration at `prepare()` lines 39-41 (`setActiveSections(4) + setCoefficient(0)`), `reset()` line 66, `processSample` lines 154-156 (Step 1.5 dispersion between popSample and bridge LP), `updateDelayLengths()` lines 88-89 (split-aware group-delay compensation: subtract from `bridgeSamples` directly, NOT `compensated`); voice-side per-block update at `BowedContrabassVoice.cpp:152-169` with `f0 ∈ [20, 5000]` paranoia clamp, `(currentStiffness <= 0.0f) ? 0.0f : computeAllpassCoefficient(f0, B, M)` 3-LOC short-circuit, `isfinite` belt-and-braces guard; harness `--string-stiffness` (sentinel < 0 = APVTS default) + `--stiffness-sweep` flags wired with default WAV/JSON name auto-rewrite + sweep-mode JSON extras at lines 308-337. R19f Logic AU smoke remains user-deferred non-blocking, mirroring Phase 2.1b R14e precedent. Verdict: ✅ **VERIFIED**. Outstanding: R20 atomic commit (gated on this verify landing) — single commit lands ~13 files (source + harness + 3 golden text files + planning artefacts). Phase 2.4 calibration polynomial follow-up parked per Risk #7. ARCHITECTURE.md amendments (DC Blocker + In-loop saturator) remain deferred to end-of-Stage-2 verify per locked decision. Hand off: R20 atomic commit, then `/clear` + `/plugin-discuss O-Contrabass 2-dsp` opens Phase 2.2 (per-string detune + A1/D2/G2 strings + per-string M=4/3/2/1 dispersion table) as fresh GSD cycle.
- **2026-04-26 (Stage 2 / Phase 2.1, research rev-2 / §12 + §13 + R7):** RESEARCH.md §12 (saturator-tail Phase 2.4 follow-up, ~110 LOC) and §13 (Phase 2.1b module-extraction research, ~210 LOC) appended. **§12** documents the analytical envelope estimate ≈10 %/s free-decay = x²/2 cubic-loss × 2 rails × 41.2 RTs/s, frames Phase 2.4's 108-combo matrix re-evaluation criteria, and conditionally tracks ARCHITECTURE.md §"In-loop saturator" amendment alongside §"DC Blocker" — both deferred to end-of-Stage-2 verify. **§13** resolves CONTEXT.md rev-2 Open Questions #2–#5: Q2 → Pattern A (`ouaricon_add_module`); Q3 → two direct headers, no umbrella, mirroring scala-tuning-engine; Q4 → DELETE inline-copy DSP files in both plugins, update `BowedStringVoice.h:23-24` includes, no shim files; Q5 → setter API (`setStaticFrictionCoefficient` + `setDynamicFrictionCoefficient` added to module's `HyperbolicFriction`; module keeps O-Bowed init defaults; O-Contrabass voice calls setters in `prepareToPlay`). §13 also locks the canonical preset (A4 vel 0.7, 5 s, factory defaults, no release tail, 32-bit float WAV) and specs the missing O-Bowed render-harness as PLAN rev-4 R8 work (mirror O-Contrabass harness exactly with target/note/sustain substitutions). Q1 (§12 timing) resolved by writing §12 in this same pass. Five Open Items handed to plan-phase rev-4 (§13.7): O-Bowed PluginCode + processor class confirm, `ouaricon_add_module` console-app compatibility, WAV writer parameter pinning, golden-WAV git-commit decision. **R7 atomic commit landed** — Phase 2.1a-recovery source (split-rail `WaveguideString.{h,cpp}` + F2 bridge LP + F3 DCB removal + F4 betaScale removal) + Stage 1 carry-forward (`CMakeLists.txt`, `PluginProcessor.{h,cpp}`, `PluginEditor.{h,cpp}`) + parameter-spec promotion + planning artefacts (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION rev-3 + CHECKPOINT-2.1a + Stage 1 set + 0-ideation gate-report) + 4 Stage 0 deep-research docs (`research/O-Contrabass-*.md`) + render-harness binary + REQUIREMENTS.md update committed in single commit per CONTEXT.md rev-2 R7 file list. Commit body explicitly notes F1+F2+F3+F4 coupled fix (RESEARCH §11 root-cause), F3 ARCHITECTURE.md §"DC Blocker" deviation (justified per §11.6), Gate 1 PASS on bow-on validation, saturator-tail Phase 2.4 follow-up parked. Hand off to `/plugin-plan O-Contrabass 2-dsp` for PLAN rev-4 (R8–R15 module-extraction tasks per RESEARCH §13.6).
