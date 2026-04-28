---
title: "O-Bassoon Stage 1 (Foundation) — Discuss Phase Findings"
created: 2026-04-27
last_verified: 2026-04-27
juce_version: "8.0.4"
summary: "Discuss-phase context for O-Bassoon Stage 1 (Foundation). Locks the 10-parameter APVTS spec, confirms both shared modules (note-expression v1.1.0 and scala-tuning-engine v2.1.0) wire in headless from day 1, and commits Windows WebView CMake flags up-front. BassoonVoice ships as a silent stub; Phase 2.1 is first audio."
domain: workflow
type: guide
keywords:
  - stage-1
  - foundation
  - bassoon
  - apvts
  - cmake
  - note-expression
  - scala-tuning-engine
  - juce8
stages: [1]
agents: [build, foundation-shell]
---

# O-Bassoon — Stage 1 Context (Discuss Phase)

## Discussion Summary

**Date:** 2026-04-27
**Participants:** User, Claude
**Inputs reviewed:** BRIEF.md, REQUIREMENTS.md, parameter-spec-draft.md, ROADMAP.md, ARCHITECTURE.md, Stage 0 CONTEXT.md, O-Lyrica + O-Wind CMakeLists.txt for module-wiring patterns.

This Stage 1 discuss closes Stage 0 follow-ups #3 (TuningEngine API path) and #4 (canonical CMake module function name). Stage 0 follow-up #2 (reference bassoon recording) is intentionally deferred to Phase 2.2 kickoff. Follow-up #1 (UI mockup) remains a Stage 3 blocker only.

---

## Requirements Confirmed

- **APVTS parameter set is locked** for Stage 1 to the 10 parameters in `parameter-spec-draft.md` (9 `AudioParameterFloat` + 1 `AudioParameterInt voice_count`). IDs, ranges, and defaults are frozen as drafted. No aftertouch→vibrato param at v1.0 (D4 stands).
- **Both shared modules wire in Stage 1**, headless:
  - `note-expression` v1.1.0 via `ouaricon_add_module(O-Bassoon note-expression)` (matches O-Lyrica pattern, `plugins/O-Lyrica/CMakeLists.txt:80`).
  - `scala-tuning-engine` v2.1.0 via direct file references to `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/{TuningEngine,ScaleGenerator,EmbeddedTunings,TuningExporter}.cpp` (matches O-Wind pattern, `plugins/O-Wind/CMakeLists.txt:45-48`).
  - `Ouaricon::NoteExpression::VST3Extensions` is a long-lived `PluginProcessor` member; `getVST3ClientExtensions()` returns its address.
  - NE drain (`Ouaricon::NoteExpression::updatePendingFromEvents`) runs at top of `processBlock` from Stage 1 — voices ignore the deltas (silent stub) until Phase 2.4.
  - `Ouaricon::TuningEngine` is a `PluginProcessor` member, default-constructed (12-TET, A4=440); raw pointer passed to each voice for future use.
- **Windows WebView CMake flags committed in Stage 1**:
  - `NEEDS_WEB_BROWSER TRUE` in `juce_add_plugin(...)`.
  - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` as a target compile-definition.
  - No WebView C++ in Stage 1 — `PluginEditor` is `juce::GenericAudioProcessorEditor` placeholder. The flags lock Windows static-linking on day 1 and avoid a CMakeLists rewrite at Stage 3.
- **`BassoonVoice` ships as a silent stub** in Stage 1: `canPlaySound`/`startNote`/`stopNote`/`pitchWheelMoved`/`controllerMoved`/`renderNextBlock` exist but voice writes nothing to the output buffer. Phase 2.1 is the first phase that produces audio.
- **No O-Reed sources referenced** anywhere in `plugins/O-Bassoon/` (DSP-07 verified at Stage 1 via grep).
- **`BassoonSound` is a single shared instance** registered with the synth; `appliesToNote`/`appliesToChannel` both return `true`.
- **16 `BassoonVoice` instances pre-allocated** in `prepareToPlay` (matches the maximum of `voice_count`, even though default cap is 8 — voice manager enforces the cap; pre-allocation prevents `processBlock` allocation if the user raises the cap to 16).

---

## Constraints Identified

- **No allocations in `processBlock`** (PERF-01 — verified at Stage 2, but Stage 1 sets the discipline: all per-voice state pre-allocated in `prepareToPlay`).
- **JUCE-NE-PATCH marker check is CMake-time** (built into `note-expression` module). Stage 1 build will fail if the patch is absent — that is the intended behavior.
- **`getLatencySamples()` is non-virtual in JUCE 8** (project memory) — do not override; modal synthesis is feed-forward, latency = 0, no `setLatencySamples()` call needed.
- **Output-only `BusesProperties`** (synth — no input bus, per `juce8-critical-patterns.md` #4).
- **`juce_generate_juce_header(O-Bassoon)`** must be called *after* `target_link_libraries(...)` (per `juce8-critical-patterns.md` #22).
- **`IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE`** in `juce_add_plugin(...)`.
- **Sample-rate / buffer-size / latency**: No special handling. Mode-bank coefficients recomputed on `prepareToPlay` and on per-note frequency changes. Any host SR / buffer size is supported. No `setLatencySamples()`.

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Parameter spec lock** | Freeze parameter-spec-draft.md as-is (10 params) | All Stage 0 open questions on params resolved; aftertouch deferred (D4) |
| **note-expression wiring** | `ouaricon_add_module(O-Bassoon note-expression)` | Canonical macro confirmed in O-Lyrica CMakeLists.txt:80 |
| **scala-tuning-engine wiring** | Direct `cpp/*.cpp` file refs (4 files) | O-Wind precedent (CMakeLists.txt:45-48); engine is not yet packaged as an `ouaricon_add_module`-style module |
| **NE drain at Stage 1** | Yes — drain runs from day 1, voices ignore | Forces module pathing + JUCE-NE-PATCH marker to validate at Stage 1 build |
| **TuningEngine at Stage 1** | Wired as PluginProcessor member, raw pointer to voices, default 12-TET A4=440 | D6 — engine present from day 1; v1.1 adds UI without architectural change |
| **Windows WebView flags** | `NEEDS_WEB_BROWSER TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` in Stage 1 | Avoids CMake churn at Stage 3; project memory shows 34/35 plugins missing this — fix by default |
| **PluginEditor at Stage 1** | `juce::GenericAudioProcessorEditor` placeholder | Auto-renders the 10 APVTS params for early manual smoke test; replaced wholesale at Stage 3 |
| **BassoonVoice at Stage 1** | Silent stub (no audio output) | Phase 2.1 is first audio; Stage 1 acceptance is "no crash, no audio" |
| **Voice pre-allocation** | 16 voices allocated in `prepareToPlay` | Matches max `voice_count`; voice manager enforces runtime cap |
| **`BassoonSound`** | Single shared instance, `appliesToNote/Channel` → true | Standard `juce::Synthesiser` pattern (matches O-Lyrica `HarpSynthSound`) |
| **Sample rate / buffer / latency** | Standard `prepareToPlay`, no `setLatencySamples()` | Modal synthesis is feed-forward; no host-compensation needed |
| **Reference bassoon recording** | Defer to Phase 2.2 kickoff | Not a Stage 1 blocker; sourcing is a Phase 2.2 input |

---

## Stage 1 Acceptance Criteria (carried from ROADMAP.md)

- [ ] `cmake --build build --target O-Bassoon_VST3` succeeds (macOS)
- [ ] `cmake --build build --target O-Bassoon_AU` succeeds (macOS)
- [ ] `cmake --build build --config Release --target O-Bassoon_VST3` succeeds (Windows — verify static linking works at build time even if not run-tested every cycle)
- [ ] `pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon.vst3` passes
- [ ] Plugin loads in Ableton / Logic without crash; appears in instrument category
- [ ] No O-Reed source references — `grep -rn "O-Reed\|OReed" plugins/O-Bassoon/` is empty (DSP-07)
- [ ] All 10 APVTS parameters appear in the host's parameter list with correct names, ranges, and defaults
- [ ] Plays silence (no audio bug, no crash) when MIDI notes are sent
- [ ] `getVST3ClientExtensions()` returns non-null `Ouaricon::NoteExpression::VST3Extensions*`
- [ ] JUCE-NE-PATCH CMake-time marker check passes

**Verifies requirements:** COMPAT-01 (pluginval pass), DSP-07 (no O-Reed dependency)

---

## Open Questions

None blocking Stage 1.

**Carried from Stage 0, resolved here:**
- ✅ TuningEngine API path: `modules/tuning/scala-tuning-engine/cpp/TuningEngine.{h,cpp}` (header path TBD at research; O-Wind's include directory is `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp` per CMakeLists.txt:58 — will mirror).
- ✅ Module CMake function: `ouaricon_add_module(<target> note-expression)` for NE module; direct file references for scala-tuning-engine.

**Carried from Stage 0, deferred:**
- Reference bassoon C3 recording → Phase 2.2 kickoff.
- UI mockup → Stage 3 prerequisite (parallel-eligible with Stages 1-2).

---

## Files Stage 1 Will Create

- `plugins/O-Bassoon/CMakeLists.txt`
- `plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}`
- `plugins/O-Bassoon/Source/PluginEditor.{h,cpp}` (Generic placeholder)
- `plugins/O-Bassoon/Source/BassoonVoice.{h,cpp}` (silent stub)
- `plugins/O-Bassoon/Source/BassoonSound.h`

## Files Stage 1 Will Reference (read-only)

- `modules/tuning/note-expression/` (linked via `ouaricon_add_module`)
- `modules/tuning/scala-tuning-engine/cpp/{TuningEngine,ScaleGenerator,EmbeddedTunings,TuningExporter}.{h,cpp}` (direct refs)
- `plugins/O-Lyrica/{CMakeLists.txt,Source/PluginProcessor.cpp,Source/HarpSynthVoice.{h,cpp},Source/HarpSynthSound.h}` (templates)
- `plugins/O-Wind/{CMakeLists.txt,Source/FluteSynthVoice.{h,cpp}}` (TuningEngine wiring template)

---

## Next Phase

Ready for: **research** phase (`/plugin-research O-Bassoon 1-foundation`)

Research-phase scope is light for Stage 1 (no novel DSP) — primary research questions are:
1. Confirm exact `Ouaricon::TuningEngine` constructor / public API (read header).
2. Confirm exact `Ouaricon::NoteExpression::VST3Extensions` constructor signature + `updatePendingFromEvents` call site (read header / O-Lyrica usage).
3. Confirm `juce_add_plugin` argument list against the latest JUCE 8.0.4 + project conventions.

If the research questions are mechanical enough to skip, `/plugin-plan O-Bassoon 1-foundation` is acceptable — note the alternative below.
