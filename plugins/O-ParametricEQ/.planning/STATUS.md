---
plugin: O-ParametricEQ
stage: 0
status: complete
last_updated: 2026-11-12 15:30:00
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: invoke_foundation_shell_agent
next_stage: 1
ready_for_implementation: true
blocked_on: parameter-spec.md (mockup finalization)
contract_checksums:
  brief: sha256:8d3242ebdc9a07001f5e25e759969d4d4b84d5798ecfcbff0f6502f0ebbf8edb
  parameter_spec: sha256:d2e3f4a88cf089fb654852f71005f6692018d72c57a827fcfd54998055798e44
  architecture: sha256:559de652fde7ef79d8231112913833f86522bcf58cf8ee21e61f8e48f7706fe4
  roadmap: sha256:41be732176b54c37a13036ce9f3f2dd923a161b1220d919f9dc6b760b211b3ad
---

# O-ParametricEQ Status

## Current Position

Stage: 0 of 4 (Ideation / Research and Planning) — complete
Status: Research and planning complete, ready for implementation once `parameter-spec.md` exists
Progress: [##..................] 10%

`parameter_spec` checksum above is of `parameter-spec-draft.md`; replace it when mockup finalization writes `parameter-spec.md`.

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

## Next Steps

1. Create and finalize the UI mockup: `/start O-ParametricEQ` → option 3. This writes `parameter-spec.md`, which is still required before Stage 1. The mockup must adopt the layout in `research/ARCHITECTURE.md` "Parameter Mapping".
2. Answer the items under "Needs the user's call before Stage 1" in `stages/0-ideation/CONTEXT.md` (items 3 and 6 change `parameter-spec.md`).
3. Stage 1: Foundation — `/implement O-ParametricEQ`
4. Pause here

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
