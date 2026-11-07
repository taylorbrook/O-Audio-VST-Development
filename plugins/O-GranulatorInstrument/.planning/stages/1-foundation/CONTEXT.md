# O-GranulatorInstrument — Stage 1 (Foundation) — CONTEXT

**Date:** 2026-11-07
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:** `parameter-spec.md` (LOCKED, 261 params — NOT the draft), `BRIEF.md`, `REQUIREMENTS.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` § Stage 1, `mockups/v1-CMakeLists.txt`, `mockups/v1-integration-checklist.md`

## Goal

The plugin builds (VST3 + AU + Standalone). All 261 parameters are registered in contract order. The output is silent. The buses and MIDI are correct. pluginval strictness 10 passes on VST3 + AU, and auval passes (COMPAT-01).

## Decisions (locked this session)

### D8 — Shell editor: GenericAudioProcessorEditor through Stage 2
- `createEditor()` returns `juce::GenericAudioProcessorEditor` until Phase 3.1. Phase 3.1 swaps in `v1-PluginEditor`.
- The WebView build plumbing is wired in Stage 1 so that Stage 3 is an editor swap, not a CMake rework:
  - `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `juce_generate_juce_header`
  - the module lines from `v1-CMakeLists.txt`
- `v1-ui.html`, `v1-i18n.js` and `v1-PluginEditor.{h,cpp}` stay in `mockups/`. Do **not** copy UI files into `Source/ui/` in Stage 1. A half-landed UI with a comment-only editor fails repo check-i18n rule [8] (pattern_check_i18n_rule8_fails_stage1_placeholder_editor). If the binary-data target needs the resource map, keep it consistent with what is actually served, or defer the UI binary data to 3.1.
- The render harness compiles the processor with `JUCE_WEB_BROWSER=0` and guards `createEditor` under `#if JUCE_WEB_BROWSER`, following the parent.

### D9 — Sidechain bus: keep it on both formats, fall back only on gate failure
- `withInput ("Sidechain", stereo, true)` + `withOutput ("Output", stereo, true)` on VST3 and AU.
- `isBusesLayoutSupported`: the input may be disabled, mono or stereo; the output must be stereo.
- VST3: a `VST3Extensions` subclass (note-expression) with `getPluginHasMainInput() = false`, so the input types as `kAux`.
- **Fallback rule:** if pluginval 10 or auval rejects the input bus, and only then, drop the input bus on **VST3 only** (AU keeps it; Logic sidechain Record-in is the primary path). Record the drop as a deviation in SUMMARY-1 and in this file, together with the failing gate output. The UI's "unavailable" state (D6) already covers a host with no bus. Do not drop the bus pre-emptively.

### D10 — `polyphony` description: keep the mockup wording
- The wording is "…steals the oldest voice with a fade." It is already the text in the LOCKED body and in `v1-i18n.js`, so there is no contract or i18n change.
- The behaviour is ARCHITECTURE §3: oldest-first, protecting the lowest and highest held notes, preferring released voices, with the quietest steal spare ended only when every spare is still fading.
- The "Draft vs mockup" item 1 in `parameter-spec.md` is resolved as "keep the mockup". Planning may add that note to the spec **header** only (above the LOCKED BODY marker). The body hash must not move.

## Requirements carried from ROADMAP / ARCHITECTURE / spec

- **CMake** from `mockups/v1-CMakeLists.txt`:
  - `PLUGIN_CODE OuGI` (re-verify that it is unused at plan time), `VERSION 0.1.0`, `FORMATS VST3 AU Standalone`
  - `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, C++20
  - `juce_audio_formats`, `juce_audio_utils`, `juce_dsp`
  - modules: grain-engine, pitch-collections, reverb-unit, preset-manager, eb-garamond, note-expression, webview-drop-streaming
- **NE module configure check** passes (`JUCE-NE-PATCH` marker present).
- **`GiParamIds.h`:**
  - loop-generated IDs (`sN_`, `menvN_`, `lfoN_`, `macroN`, `modM_`)
  - `juce::ParameterID { id, 1 }` on every parameter
  - registration order = the spec's "Full ID list"
  - types 176 Float / 70 Choice / 15 Bool
  - **append-only destination table (126)** and **source table (18)**
  - Never a lambda-mapped `NormalisableRange` (WebView frontend). `setSkewForCentre` rows are checked against the spec's skew column.
  - The stepped-range-zero trap: interval 0 unless the spec says "interval 1".
- **processBlock:**
  - `ScopedNoDenormals`
  - clears every output channel
  - reads channel counts from the buses, never assumed (mono-out/AudioChannelSet bitset traps)
  - consumes MIDI without acting on it
  - the sidechain never reaches the output
- **`setLatencySamples (0)`** in `prepareToPlay()` only.
- **State scaffold:**
  - `<Engine>` holds `seed` (int64 — never `getLargeIntValue` on a uint64), `stateVersion`, `modDestTableVersion = 1` and `uiLanguage`
  - an empty `<Slots>` child
  - a round-trip through `getStateInformation` / `setStateInformation` keeps every parameter and every `<Engine>` field
- **No parameter writes from listeners or from the audio thread.** Actions are native functions (Stage 3).
- **Render harness scaffold:**
  - lives at `tests/render-harness/main.cpp`, off by default, mirroring O-Granulator's CMake option
  - MIDI script injection
  - a `loadSlotSync` test-hook **signature** (a stub until 2.1)
  - seed injection
  - a synthetic `AudioPlayHead`

## Test criteria (Stage 1 gate)

- [ ] VST3 + AU + Standalone build. auval PASS. pluginval strictness 10 VST3 + AU, run both with the sidechain enabled and with it disabled (or the D9 fallback, recorded).
- [ ] Sidechain-only render (stereo + mono sidechain, MIDI notes present) → output bitwise zero.
- [ ] Parameter count == 261. The ID order matches the spec's Full ID list exactly (golden). Type split 176 / 70 / 15.
- [ ] The destination table golden test (126, order frozen) and the source table golden test (18) pass.
- [ ] Spot-check the ranges, defaults and skews against the spec, including every `setSkewForCentre` row and every interval-1 row.
- [ ] State round-trip: params + `<Engine>` (int64 seed > INT32_MAX survives) + an empty `<Slots>`.
- [ ] The NE configure check passes.

## Out of scope for Stage 1

- DSP and the grain engine fork (Stage 2; the fork happens at Phase 2.1 start)
- The WebView editor integration (Phase 3.1)
- Factory sources and presets, and i18n gates on a live UI (Stage 4)

## Status of the blockers

- **O-Granulator 3.3 is closed:** `1455e501` (VERIFIED) and `00e462b8` (the last 3.3 Source change). Phase 2.1's fork pin can now be set to the 3.3-closing commit. Confirm the exact pin SHA at 2.1 discuss. The parent's Stage 4 work (AGC/saturator restage, D43) may touch leaves after the pin, so record it in FORK-MANIFEST drift review.
- Stage 1 itself has no blockers.

## Risks to flag

- **Instrument + input bus host quirks (VST3 `kAux`):** handled by D9.
- **261 parameters in GenericAudioProcessorEditor:** the editor is long but functional. It is only a test surface, so it is not a UX concern.
- **The 126-entry `modM_dest` host menu:** accepted (ROADMAP Known Challenges).
- **CRLF:** the spec files are LF. Byte-compared golden dumps write `"\n"` explicitly (`File::replaceWithText` defaults to CRLF).

## Next Phase

Ready for: research phase (`/plugin-research O-GranulatorInstrument 1-foundation`), or go straight to plan. Stage 1 closely mirrors the parent's Stage 1.
