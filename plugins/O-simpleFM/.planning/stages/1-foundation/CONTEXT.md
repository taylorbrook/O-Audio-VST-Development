# Stage 1 (Foundation) — CONTEXT

**Plugin:** O-simpleFM
**Stage:** 1 of 4 — Foundation / Shell
**Date:** 2026-06-20
**Source:** Auto-generated from locked contracts (BRIEF.md, ARCHITECTURE.md, ROADMAP.md, parameter-spec-draft.md). Manual mode — no interactive questions: Stage 0 locked all scope decisions, so Foundation has no open gray areas.

## Goal

A buildable, host-loadable **silent synth shell**: correct CMake (synth + MIDI-in + cross-platform WebView2 flags), the complete 17-parameter APVTS, plugin-info/bus plumbing, and state save/load. No audio yet (first sound is Stage 2 / Phase 2.1), no WebView UI yet (Stage 3) — a `GenericAudioProcessorEditor` exposes the params for testing.

## Requirements addressed this stage

- **COMPAT-01/02 (build):** VST3 + AU (+ Standalone) build on macOS; CMake carries the Windows WebView2 static-linking flags so Stage 3 needs no CMake rework.
- **Parameter contract:** all 17 core params from ARCHITECTURE.md → Parameter Mapping, with exact IDs / ranges / defaults / skews.
- **MIDI instrument plumbing:** `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE`, output-only stereo bus (critical-pattern #22 — omitting yields a silent/no-MIDI plugin).
- **State persistence:** APVTS value-tree serialize via `getStateInformation`/`setStateInformation` (suite standard).

## Locked decisions inherited from Stage 0 (do not revisit)

- 17 core params; sine-only operators in v1.0; `modFixedMode`/`modFixedHz` adopted; `fineCents`/`masterTune`/non-sine waveforms deferred to v1.1.
- 16-voice polyphony; PM (radians) convention; raw radian index `I` 0–20 with `I = 20·norm^1.7` taper.
- These shape the param ranges below but the **DSP that consumes them is Stage 2** — Foundation only declares the parameters.

## Out of scope for Stage 1

- Any audio rendering / voice DSP (Stage 2).
- WebView UI, relays, attachments, binary data, spectrum/scope (Stage 3).
- Presets, optimization (Stage 4).

## Constraints

- Follow ARCHITECTURE.md exactly (immutable contract).
- Real-time-safe processBlock even while silent: `ScopedNoDenormals`, clear buffer, no allocation.
- Match Ouaricon suite conventions (O-Bassoon synth template; `OuariconModules.cmake`).
- Unique `PLUGIN_CODE` = `OSiF` (`OuFm` already taken by another plugin).

## Success criteria

1. `ninja O-simpleFM_VST3 O-simpleFM_AU` builds clean.
2. AU validates: `auval -a | grep -i simplefm` lists it; `auval -v aumu OSiF Ouar` passes.
3. Loads in a DAW as an **instrument**; accepts MIDI (no audio expected yet).
4. All 17 parameters appear (GenericAudioProcessorEditor) with correct ranges/defaults.
5. Save/recall a session preserves parameter values.
