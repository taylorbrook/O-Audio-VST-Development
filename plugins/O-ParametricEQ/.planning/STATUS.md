---
plugin: O-ParametricEQ
stage: 2
phase: research
status: stage_2_phase_2_2_researched
last_updated: 2026-11-13 23:59:30
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: stage_2_plan_phase_2_2
next_stage: 2
ready_for_implementation: true
ui_scaffolding_phase_complete: true
latest_mockup_version: 1
mockup_finalized: true
finalized_version: 1
brief_updated_from_mockup: true
mockup_version_synced: 1
brief_update_timestamp: "2026-11-13T02:26:56Z"
contract_checksums:
  brief: sha256:6fbb1b97c9c64ca3f159a409f807f13c74cb16dab8d6ab14b6a1a13c2e6991ff
  parameter_spec: sha256:60e7cf7fc335929259b4930d156a4f0d91cfa3b0a9f26d751ec31c5328c6229f
  architecture: sha256:bf70384089e2e8de134a9351e8f46273fff9ad5ce9b9aba333abd327e3cae92e
  roadmap: sha256:c138ad57eaea2128940b6dbdf5b12053bf8bd04a17e108a718d25dba2e3f62fb
---

# O-ParametricEQ Status

## Current Position

Stage: 2 of 4 (DSP) — discuss ✓ 2026-11-12 (`stages/2-dsp/CONTEXT.md`, D6–D11); research 2.1 / 2.1b ✓ 2026-11-12 (`stages/2-dsp/RESEARCH-2.1.md`); plan 2.1 / 2.1b ✓ 2026-11-13 (`stages/2-dsp/PLAN-2.1.md`, `PLAN-2.1b.md`); execute 2.1 ✓ 2026-11-13 (`stages/2-dsp/SUMMARY-2.1.md`, designer harness 28 PASS / 0 FAIL / 6 INFO); verify 2.1 ✓ 2026-11-13 (`stages/2-dsp/VERIFICATION-2.1.md`, VERIFIED); execute 2.1b **BLOCKED** 2026-11-13 (`stages/2-dsp/SUMMARY-2.1b.md`: harness 47 PASS / 1 FAIL, C1 continuity; uncommitted); research on the C1 failure ✓ 2026-11-13 (`stages/2-dsp/RESEARCH-2.1b-continuity.md`); plan 2.1b rework ✓ 2026-11-13 (`stages/2-dsp/PLAN-2.1b-rework.md`, 10 tasks); execute 2.1b rework ✓ 2026-11-13 (`stages/2-dsp/SUMMARY-2.1b-rework.md`, designer harness 54 PASS / 0 FAIL / 12 INFO); verify 2.1b ✓ 2026-11-13 (`stages/2-dsp/VERIFICATION-2.1b.md`, VERIFIED); research 2.2 ✓ 2026-11-13 (`stages/2-dsp/RESEARCH-2.2.md`); next is plan for Phase 2.2. Stage 1 (Foundation): VERIFIED
Status: 0.1.0 built and installed (VST3 + AU + Standalone); harness 36 PASS / 0 FAIL, pluginval 10 and auval pass (`stages/1-foundation/SUMMARY.md`). Verified 2026-11-12 by an independent re-run on a fresh compile and reinstalled bundles, plus a mutation run (`stages/1-foundation/VERIFICATION.md`); next is Stage 2 discuss
Progress: [####................] 20%

`parameter_spec` checksum above is of `parameter-spec.md` (written by mockup finalization, 2026-11-12; 196 host parameters). `parameter-spec-draft.md` is kept for reference and is superseded.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined
- Parameters specified
- UI vision captured
- Use cases identified
- Requirements extracted with acceptance criteria

**Stage 0:** ✓ Complete
- Plugin type defined: Effect (clean parametric EQ, 24 bands, zero latency, minimum phase)
- Design candidates modelled numerically: 9 (see `stages/0-ideation/CONTEXT.md`)
- Papers read in full: Vicanek 2016, 2019, 2024
- JUCE modules identified: juce_audio_basics, juce_audio_processors, juce_audio_plugin_client, juce_audio_utils, juce_core, juce_data_structures, juce_dsp (FFT and windowing only), juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
- Suite modules: preset-manager 1.1.1, eb-garamond 1.0.0, vu-meter 1.0.0
- DSP feasibility verified in a float64 model (not yet in C++)
- Parameter layout fixed: 196 host parameters, IDs `b01_…b24_`, global block first
- Thresholds set: DSP-01 tolerance, DSP-05 noise floor, PERF-02 CPU budget
- Complexity score: 5.0
- Strategy: staged implementation (Stage 2 in 6 phases, Stage 3 in 3 phases)
- **Stage 0:** Research & Planning complete - ARCHITECTURE.md and ROADMAP.md documented (Complexity 5.0)

**UI mockup (Phase A, design iteration):** v1 design approved and finalized 2026-11-12; brief synced
- `mockups/v1-ui.yaml` + `mockups/v1-ui-test.html` (960 × 600, resizable 800 × 500 – 1600 × 1000, paper ground, dragonfly plate)
- Phase B (implementation scaffolding) complete 2026-11-12: `mockups/v1-ui.html`, `v1-i18n.js`, `v1-PluginEditor-TEMPLATE.h/.cpp`, `v1-CMakeLists-SNIPPET.txt`, `v1-integration-checklist.md`; `parameter-spec.md` written (196 host parameters per ARCHITECTURE.md "Parameter Mapping")
- Stage 3 decisions carried from the mockup's open questions are listed in `mockups/v1-integration-checklist.md`

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-11-12 | |
| research | ✓ | 2026-11-12 | |
| plan | ✓ | 2026-11-12 | |
| execute | ✓ | 2026-11-12 | |
| verify | ✓ | 2026-11-12 | |

### Stage 2: DSP
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss (stage-wide) | ✓ | 2026-11-12 | |
| research 2.1 / 2.1b | ✓ | 2026-11-12 | |
| plan 2.1 / 2.1b | ✓ | 2026-11-13 | |
| execute 2.1 | ✓ | 2026-11-13 | |
| verify 2.1 | ✓ | 2026-11-13 | |
| execute 2.1b | ✗ blocked (C1) | 2026-11-13 | |
| research 2.1b continuity | ✓ | 2026-11-13 | |
| plan 2.1b rework | ✓ | 2026-11-13 | |
| execute 2.1b rework | ✓ | 2026-11-13 | |
| verify 2.1b | ✓ | 2026-11-13 | |
| research 2.2 | ✓ | 2026-11-13 | |

## Next Steps

1. ~~Create and finalize the UI mockup~~ Done 2026-11-12: mockup v1 finalized, implementation scaffolding generated, `parameter-spec.md` written with the layout in `research/ARCHITECTURE.md` "Parameter Mapping".
2. ~~Answer the items under "Needs the user's call before Stage 1"~~ Items 3 and 6 answered 2026-11-12 (196 parameters, `b01_` IDs; no spec change). Items 1, 2, 4, 5, 7, 8 are deferred to Stage 2 discuss.
3. ~~Stage 1 research~~ Done 2026-11-12: `stages/1-foundation/RESEARCH.md`. Carry to Stage 2 discuss: skewed raw defaults are 999.99988 Hz and 0.99999976 (normalised 0.5 exactly).
4. ~~Stage 1 plan~~ Done 2026-11-12: `stages/1-foundation/PLAN.md`. Plan decisions to note: an invalid state blob leaves the state unchanged (deviates from ARCHITECTURE "→ defaults"); `uiLanguage` is a root attribute; save always writes `stateVersion="1"`.
5. ~~Stage 1 execute~~ Done 2026-11-12: `stages/1-foundation/SUMMARY.md`. Gate 0→1 bypassed with `--force` (no CMakeLists before Stage 1). AU channel capabilities `[1, 1] [2, 2]` are now frozen.
6. ~~Stage 1 verify~~ Done 2026-11-12: `stages/1-foundation/VERIFICATION.md`, VERIFIED. COMPAT-01 complete; DSP-07 partial (state format and layout met; the `Band::dynOffsetDb` hook is Phase 2.2). Carry to Stage 2: RESEARCH pitfall 2 is wrong for JUCE 8.0.15 (a missing parameter already loads at its default through `valueTreeChildAdded`; the default fill is redundant and S5 does not gate it). Hands-on DAW smoke (three items in VERIFICATION) is open and not blocking.
7. ~~Stage 2 discuss~~ Done 2026-11-12: `stages/2-dsp/CONTEXT.md`. The six deferred calls are answered: D6 slope inert on Bell / Notch / Band Pass, D8 tilt ±g/2, D9 Q 1.0 = Butterworth, D10 keep the in-band skirt above Nyquist, D11 listen = the band's region of the input, all as recommended. **D7 departs from the recommendation: the accuracy polish is in v1** (target near the second-order optimum, about 0.15 / 0.30 / 0.70 dB at ±6 / ±12 / ±30 dB at 44.1 kHz, one section per bell; bells and shelves including the weak spots; hybrid with the closed form as seed and fallback). Consequence: a new Phase 2.1b between 2.1 and 2.2; the plan phase amends `ROADMAP.md` and `ARCHITECTURE.md` ("Accuracy backlog (not v1)", G1 tolerance) and refreshes their checksums. Neither contract was edited in discuss.
8. ~~Stage 2 research, Phase 2.1 / 2.1b~~ Done 2026-11-12: `stages/2-dsp/RESEARCH-2.1.md` (model and scripts in `stages/2-dsp/research-2.1/`). Stage 0 tables reproduced within 0.01 dB; the closed form passes G1 at the Stage 0 tolerance at five rates. Polish: anchored Gauss-Newton on six log parameters, 26-point grid with points tied to the peak, two iterations, blend weights in place of switches. Model at 44.1 kHz: bell 0.04 / 0.08 / 0.19 dB at ±6 / ±12 / ±30 (seed 0.34 / 0.61 / 0.98); the section-plus-corrector optimum is 0.005 / 0.009 / 0.021, far below D7a's single-biquad reference. Reciprocity kept (polish the boost, invert). Cost 0.8 µs per polished section (seed 35 ns), measured in a C++ port. **Calls for the plan:** every-tick polish puts 24 sweeping 96 dB/oct shelves at about 46 % of a core (PERF-02's cut case is unaffected); proposed G1 tolerance for 2.1b `0.05 + 0.012 |g|` at 44.1 / 48 kHz; the first-order shelf section stays unpolished; one weak spot stays poor (12 dB/oct, Q 2–10, ±30 dB, corner above 10 kHz).
9. ~~Stage 2 plan, Phase 2.1 / 2.1b~~ Done 2026-11-13: `stages/2-dsp/PLAN-2.1.md` (9 tasks) and `PLAN-2.1b.md` (8 tasks). Research's calls taken as recommended: G1 for 2.1b is `0.05 + 0.012 |g|` at 44.1 / 48 kHz and `0.03 + 0.002 |g|` at 88.2 kHz and above; the polish runs on every control tick (24 sweeping 96 dB/oct shelves become a G10 INFO row, correction hold is the documented fallback, read at the 2.2 smoke measurement); the first-order shelf section stays unpolished; the one stubborn weak spot is reported, not gated. **Contracts amended** (D7): `ROADMAP.md` (Phase 2.1b, 7 phases, risks) and `research/ARCHITECTURE.md` (Amendments table, "Accuracy polish (v1, Phase 2.1b)", G1, G10, Performance, notch / band pass above the clamp); checksums above refreshed. Stage 0 decision 14 carries an amendment note. Other plan decisions: the designer is std-only C++17 headers in `Source/dsp/`; the Python model is promoted to `tests/model/` and writes the designs goldens; a G1 failure in 2.1 stops and escalates before any Vicanek fallback is written; neither phase touches the processor or the plugin binary.
10. ~~Stage 2 execute, Phase 2.1~~ Done 2026-11-13: `stages/2-dsp/SUMMARY-2.1.md`. Designer is four std-only headers in `Source/dsp/`; designer harness 28 PASS / 0 FAIL / 6 INFO; MLR passes G1 in C++ at five rates and the RBJ control fails; the C++ matches the Python golden bit-exactly (162,855 rows, 30 MB, committed at that size on Taylor's call); all negative controls fire; Stage 1 harness still 36 / 0. Gate 1→2 passed with the human review checkpoint skipped (`--skip-review`, justification recorded). For verify: G2s cannot measure the High Cut stop-band slope at 6 dB/oct (11 cells skipped); the agent-written code was gated, not read line by line. Plugin binary unchanged (0.1.0), no install, no tag.
11. ~~Stage 2 verify, Phase 2.1~~ Done 2026-11-13: `stages/2-dsp/VERIFICATION-2.1.md`, VERIFIED. Fresh recompile 28 / 0 / 6, output identical to SUMMARY; model, C++ dump and committed golden share one SHA-1; an independent time-domain scorer reproduces the shelf G1 cells to three decimals; the four `Source/dsp` headers were read line by line (no defect). Mutation run: 15 of 15 caught, but 7 only by the golden. **Carry to 2.1b execute (harness hardening, non-blocking):** G2s reads its nominal slope from `eq::slopeOrder` (use literals); N1's 1e-6 limit passes the naive form (4.8e-7; tighten to 1e-9); cut accuracy against the prototype is INFO only (matched high-pass and the 0.75 π match point are golden-only; add ceilings); P1's notch clamp check is one-sided; the 11 High Cut cells G2s skips need a shortened-span slope measurement (verify measured them: within 0.43 dB/oct of analog at 6 dB/oct). `design()` does not guard its inputs: 2.2 must pass sanitised values. No requirement closes here (DSP-01 closes at 2.1b verify).
12. Stage 2 execute, Phase 2.1b — **BLOCKED 2026-11-13**: `stages/2-dsp/SUMMARY-2.1b.md`. The polish is written (`Source/dsp/Polish.h`, `PolishConstants.h`; `Designer::design` polished, `designSeed` the 2.1 form) and equals the model (D2 2.95e-10); every accuracy row passes (G1b at five rates, NW1 0 of 178 000, R2, F2, S1, TL2, A2, B2); harness 47 PASS / 1 FAIL / 11 INFO. **C1 fails: the polished map jumps (15 dB) for resonant shelf sections with the corner near or above Nyquist** (the fit switches basin; accept weight 1 → 0 between neighbours). Bells and the gated grids are clean. Per PLAN-2.1b T7 / R1 no constant or threshold was changed. A distance-from-seed fade was probed and does not fix it. **Nothing is committed** (files listed in the SUMMARY; the 17 MB polished golden changes if the algorithm does).
13. ~~Decide the fix route, then research~~ Done 2026-11-13 (route 1): `stages/2-dsp/RESEARCH-2.1b-continuity.md`, bench in `stages/2-dsp/research-2.1b/cont_bench.cpp` (bit-identical to `Designer::design` with default knobs). The jump is a diverging fit (ln mB +19.5 in one step), and off the gated grid a polished section can be +54 dB worse than its seed; both live where section Q >= 4 and the upper pair is above 0.9 Nyquist (seed 5-12 dB off). **Recommended: a re-centre rule for shelf sections on the seed side (pair above 0.95 pi designed there with an effective Q, faded in from pair Q 1.5 to 4) plus a smooth limit of 0.35 log units on every polish step.** Bench at 44.1 / 48 kHz: 0 jumps on 5340 lines (shipped 19 on 2700), 0 runaway sections, worst excess over the seed +0.67 dB, gated G1b rows keep their margins, stubborn cell 4.46 -> 0.45 dB. **Calls for the plan:** C1's 2x-of-seed limit (about 1 line in 1000 reads 2.0-2.4x, continuous); polish off below 44.1 kHz (not continuous there with or without the fix); step limit 0.35 or 0.5; the re-centre changes the Phase 2.1 seed (model, 30 MB designs golden, 2.1 rows) unless used as a polish start point only. Nothing in Source, harness, model or goldens was changed; the 2.1b execute files are still uncommitted.
   **Decided by Taylor 2026-11-13 (all four as recommended):** (1) C1's ratio limit becomes 4x of the seed, the shrink test (0.5) stays binding; (2) no polish below 44.1 kHz, decided once per sample rate in `Designer::prepare`; (3) the shelf re-centre is a seed change (Phase 2.1 model, designs golden and 2.1 rows are redone), not a polish start point; (4) step limit 0.35.
14. ~~Plan the 2.1b rework~~ Done 2026-11-13: `stages/2-dsp/PLAN-2.1b-rework.md` (10 tasks, six waves). Shelf re-centre in the model and `Prototype.h` (designs golden regenerated and audited line by line against the verified one), step limit 0.35 and the 44.1 kHz rate gate in the polish, polished golden regenerated. Harness: C1 at 4x with the shrink test binding on 1101 lines (the first 341 unchanged, nine families), C1-hi at 88.2 / 96 / 192 kHz, NW2 (random full-range never-worse, ceiling +1.0 dB), RA1 (runaway count 0), PR1 (no polish below 44.1 kHz), P1s (re-centre boundaries), and a negative control that rebuilds the first build and must fail NW2, RA1 and C1. **Contracts amended** (`research/ARCHITECTURE.md`: Amendments, shelf / tilt frequency policy, polish limit / rate / Continuity, known limits, G1; `ROADMAP.md`: Phase 2.1b components and criteria); checksums above refreshed. **Plan calls to note:** the NW2 ceiling (+1.0 dB against a measured +0.67) is the plan's number; I-weak stays INFO with WARN ceilings; the blocked execute's files and the rework go in one commit; a red C1 / NW2 / RA1 / G1b with green goldens stops the session, nothing is retuned.
15. ~~Execute the 2.1b rework~~ Done 2026-11-13: `stages/2-dsp/SUMMARY-2.1b-rework.md`. Designer harness 54 PASS / 0 FAIL / 12 INFO. C1 0 violations on 1101 lines (worst 2.17x, 1 line above 2x; the two former failing lines 0.98x and 1.34x), C1-hi 0 of 216, NW2 +0.660 / +0.270 dB at 44.1 / 48 kHz over 2.6 million sections per rate (ceiling +1.0), RA1 0, PR1 and P1s green, D1 bit-exact and D2 4.5e-13 against the regenerated goldens (designs audit: 4800 changed lines, all in the rule's region), every G1b cell in tolerance (72 dB/oct ±30 dB at 44.1 kHz 0.247 against 0.410), `--neg first-build` fires on all three rows. **Plan gap, decided by Taylor in the session:** T1's "Shelf 48 dB/oct, Q 1, 10–20 k" line moved with the seed (0.84 / 1.72 / 4.01 → 0.84 / 1.85 / 4.33); that ARCHITECTURE "Model results" row and the T1 expectation were amended and the `architecture` checksum refreshed. The blocked 2.1b files and the rework are in one commit. No processor code, no binary change, no tag.
16. ~~Verify Phase 2.1b~~ Done 2026-11-13: `stages/2-dsp/VERIFICATION-2.1b.md`, VERIFIED. Fresh recompile 54 / 0 / 12, the 2.1b block identical to the SUMMARY; both goldens regenerate byte-identically from the model (and `designs.tsv` from the C++ dump); every negative control fires; an independent audit of the seed change against the 2.1 golden gives 4800 = 4800 lines, 0 differences either way. Mutation run, 17 mutants: 12 of 16 caught by a physics row. **Gate gaps carried (non-blocking):** the step limit is golden-only (compiled out, C1 reads 3.46x and NW2 +0.78 dB, both inside the limits; research's 're-centre alone leaves jumps' is not reproduced on the harness lines); the accept weight's improvement factor is never active on any tested input. **DSP-01 → partial** (design side met; latency 0 and rendered G1 at 2.2).
17. ~~Research for Phase 2.2~~ Done 2026-11-13: `stages/2-dsp/RESEARCH-2.2.md`, bench in `stages/2-dsp/research-2.2/engine_bench.cpp` (prototype engine over the real `Source/dsp` headers). The recurrences realise the closed form: rendered impulse response within 2.8e-8 dB in double and 5.8e-4 dB in float above -80 dB (22 settings, 8 types, 44.1 / 96 kHz). **Three gates do not survive the float output as written:** G4 (c) (-130 dB re input peak reads -120.5 dB with 24 Mid bells at +6 dB; the engine is at -265 dB), G8 (a 96 dB/oct High Cut at 10 Hz has a first sample of 2e-47, below float), and the "float → double → float is exact" premise (false for denormal samples under flush-to-zero: untouched lanes must not be converted). A 0 dB gain band is not bit-transparent without the 2.4 sleep. CPU: PERF-02 case 4.6 % section-major (the model's figure), 2.4 % sample-major; 24 shelves x 96 dB/oct settled 4.7 %; design 4.67 µs per 96 dB/oct shelf band (33.6 % for 24 sweeping, model 46 %). **Calls for the plan:** C1 and C2 are Taylor's (amend ARCHITECTURE G4 (c) and G8); C3–C6 are the plan's (FUNC-02 row scope and render length, tail 2.0 with row L4, sample-major loop, the 2.2 behaviours that 2.3 / 2.4 replace). Nothing in Source, the harnesses, the model or the goldens was changed.
18. Plan Phase 2.2: `/plugin-plan O-ParametricEQ 2-dsp`.

## Context to Preserve

**Key Decisions (2026-11-12, ideation):**
- Plugin type: Effect (clean parametric EQ); transparent counterpart to O-AnalogEQ
- Long-term target: rival FabFilter Pro-Q; v1 = essentials at the highest quality
- Phase modes: zero-latency minimum phase only (linear / natural phase deferred)
- Dynamic EQ: not in v1, but band engine, parameter layout and state format leave room for it
- 24 bands, 6–96 dB/oct, eight filter types
- Per-band placement Stereo / L / R / Mid / Side; mono and stereo layouts only
- Assumed in v1 (stated at the finalize gate, not objected to): spectrum analyzer, output gain, auto gain

**Key Decisions (2026-11-12, Stage 0):**
- Filter design: matched pole and zero pairs plus a first-order corrector (DC and Nyquist exact); matched cuts; Vicanek 2019 first-order shelves. Fallback: Vicanek 2016 bell, Vicanek 2024 Butterworth shelves.
- Realisation: trapezoidal SVF + TPT one-pole, double state, parameters `(g, k, mL, mB, mH)`; no biquad coefficients
- Smoothing: 20 ms ramps, global 16-sample control grid, per-sample parameter interpolation
- Structural changes (type, slope, placement, enable, bypass): two-slot 20 ms crossfade; neutral-gain sleep
- Slope: cuts, shelves and tilt honour 6–96 dB/oct; Bell, Notch, Band Pass ignore it in v1
- Placement: serial on L/R, Mid / Side by difference injection; inactive bands skipped
- Display: message-thread designer pushes section parameters; page evaluates the closed form
- Auto gain: curve-derived, deterministic, on the audio thread
- Dynamic EQ: 72 parameters not registered; IDs and version hint 2 reserved; `stateVersion` 1
- Analyzer mode is UI state; global `bypass` is the host bypass parameter
- Implementation strategy: phased; designer and its offline gate (Phase 2.1) before any processor code

**Files Created:**
- plugins/O-ParametricEQ/.planning/BRIEF.md
- plugins/O-ParametricEQ/.planning/REQUIREMENTS.md
- plugins/O-ParametricEQ/.planning/parameter-spec-draft.md
- plugins/O-ParametricEQ/.planning/research/ARCHITECTURE.md
- plugins/O-ParametricEQ/.planning/ROADMAP.md
- plugins/O-ParametricEQ/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-ParametricEQ/.planning/mockups/v1-ui.yaml
- plugins/O-ParametricEQ/.planning/mockups/v1-ui-test.html
- plugins/O-ParametricEQ/.planning/parameter-spec.md
- plugins/O-ParametricEQ/.planning/mockups/v1-ui.html
- plugins/O-ParametricEQ/.planning/mockups/v1-i18n.js
- plugins/O-ParametricEQ/.planning/mockups/v1-PluginEditor-TEMPLATE.h
- plugins/O-ParametricEQ/.planning/mockups/v1-PluginEditor-TEMPLATE.cpp
- plugins/O-ParametricEQ/.planning/mockups/v1-CMakeLists-SNIPPET.txt
- plugins/O-ParametricEQ/.planning/mockups/v1-integration-checklist.md
- plugins/O-ParametricEQ/.planning/stages/1-foundation/CONTEXT.md
- plugins/O-ParametricEQ/.planning/stages/1-foundation/RESEARCH.md
- plugins/O-ParametricEQ/.planning/stages/1-foundation/PLAN.md
- plugins/O-ParametricEQ/CMakeLists.txt
- plugins/O-ParametricEQ/CHANGELOG.md
- plugins/O-ParametricEQ/Source/PluginProcessor.h
- plugins/O-ParametricEQ/Source/PluginProcessor.cpp
- plugins/O-ParametricEQ/tests/render-harness/main.cpp
- plugins/O-ParametricEQ/tests/golden/check_spec.py
- plugins/O-ParametricEQ/tests/golden/params.tsv
- plugins/O-ParametricEQ/.planning/stages/1-foundation/SUMMARY.md
- plugins/O-ParametricEQ/.planning/stages/1-foundation/VERIFICATION.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/CONTEXT.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/RESEARCH-2.1.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/research-2.1/ (model scripts and C++ timing port)
- plugins/O-ParametricEQ/.planning/stages/2-dsp/PLAN-2.1.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/PLAN-2.1b.md
- plugins/O-ParametricEQ/Source/dsp/EqTypes.h
- plugins/O-ParametricEQ/Source/dsp/ResponseEval.h
- plugins/O-ParametricEQ/Source/dsp/Prototype.h
- plugins/O-ParametricEQ/Source/dsp/Designer.h
- plugins/O-ParametricEQ/tests/designer-harness/main.cpp
- plugins/O-ParametricEQ/tests/model/eqmodel.py
- plugins/O-ParametricEQ/tests/model/dump_designs.py
- plugins/O-ParametricEQ/tests/model/README.md
- plugins/O-ParametricEQ/tests/golden/designs.tsv
- plugins/O-ParametricEQ/.planning/stages/2-dsp/SUMMARY-2.1.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/VERIFICATION-2.1.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/RESEARCH-2.1b-continuity.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/research-2.1b/cont_bench.cpp
- plugins/O-ParametricEQ/.planning/stages/2-dsp/PLAN-2.1b-rework.md
- plugins/O-ParametricEQ/Source/dsp/Polish.h
- plugins/O-ParametricEQ/Source/dsp/PolishConstants.h
- plugins/O-ParametricEQ/tests/model/polish_model.py
- plugins/O-ParametricEQ/tests/model/dump_polish.py
- plugins/O-ParametricEQ/tests/golden/designs-polished.tsv
- plugins/O-ParametricEQ/.planning/stages/2-dsp/SUMMARY-2.1b.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/SUMMARY-2.1b-rework.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/VERIFICATION-2.1b.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/RESEARCH-2.2.md
- plugins/O-ParametricEQ/.planning/stages/2-dsp/research-2.2/engine_bench.cpp
