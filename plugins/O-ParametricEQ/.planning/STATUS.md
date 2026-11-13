---
plugin: O-ParametricEQ
stage: 1
phase: research
status: stage_1_research_complete
last_updated: 2026-11-12 22:15:00
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: plugin_plan_stage_1
next_stage: 1
ready_for_implementation: true
blocked_on: none
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
  architecture: sha256:559de652fde7ef79d8231112913833f86522bcf58cf8ee21e61f8e48f7706fe4
  roadmap: sha256:41be732176b54c37a13036ce9f3f2dd923a161b1220d919f9dc6b760b211b3ad
---

# O-ParametricEQ Status

## Current Position

Stage: 1 of 4 (Foundation) — research complete
Status: Stage 1 researched (`stages/1-foundation/RESEARCH.md`); next is plan
Progress: [##..................] 10%

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
| plan | → | | |
| execute | | | |
| verify | | | |

## Next Steps

1. ~~Create and finalize the UI mockup~~ Done 2026-11-12: mockup v1 finalized, implementation scaffolding generated, `parameter-spec.md` written with the layout in `research/ARCHITECTURE.md` "Parameter Mapping".
2. ~~Answer the items under "Needs the user's call before Stage 1"~~ Items 3 and 6 answered 2026-11-12 (196 parameters, `b01_` IDs; no spec change). Items 1, 2, 4, 5, 7, 8 are deferred to Stage 2 discuss.
3. ~~Stage 1 research~~ Done 2026-11-12: `stages/1-foundation/RESEARCH.md`. Carry to Stage 2 discuss: skewed raw defaults are 999.99988 Hz and 0.99999976 (normalised 0.5 exactly).
4. Stage 1 plan — `/plugin-plan O-ParametricEQ`
5. Pause here

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
